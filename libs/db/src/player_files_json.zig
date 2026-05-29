const std = @import("std");
const cdb = @import("cdb");
const characters_json = @import("characters_json.zig");
const objects_json = @import("objects_json.zig");
const jsonx = @import("flags_json.zig");

const RENT_RENTED = 2;
const RENT_CRYO = 3;
const RENT_TIMEDOUT = 5;
const VIRTUAL = 1;

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;
extern fn time(tloc: ?*cdb.time_t) cdb.time_t;
extern fn create_obj() ?*cdb.obj_data;
extern fn read_object(nr: cdb.obj_vnum, type_: c_int) ?*cdb.obj_data;
extern fn obj_to_char(object: *cdb.obj_data, ch: *cdb.char_data) void;
extern fn obj_to_room(object: *cdb.obj_data, room: *cdb.room_data) void;
extern fn obj_to_obj(obj: *cdb.obj_data, obj_to: *cdb.obj_data) void;
extern fn json_save_char_for_objects(ch: *cdb.char_data) void;
extern fn json_obj_auto_equip(ch: *cdb.char_data, obj: *cdb.obj_data, location: c_int) void;
extern fn check_unique_id(obj: *cdb.obj_data) void;
extern fn add_unique_id(obj: *cdb.obj_data) void;
extern fn name_from_drinkcon(obj: *cdb.obj_data) void;
extern fn name_to_drinkcon(obj: *cdb.obj_data, type_: c_int) void;

var global_io: std.Io = undefined;
var has_io = false;

pub fn init(io: std.Io) void {
    global_io = io;
    has_io = true;
}

pub const PlayerObjects = struct {
    rent: RentInfo,
    equipment: std.json.Array,
    inventory: std.json.Array,
};

pub const RentInfo = struct {
    code: c_int,
    time: cdb.time_t,
    cost: c_int,
    gold: c_int,
    bank: c_int,
    items: c_int,
};

pub export fn json_player_save(path: [*:0]const u8, ch: *cdb.char_data) c_int {
    writePlayerJson(std.mem.span(path), ch) catch return -1;
    return 0;
}

pub export fn json_player_load(path: [*:0]const u8, ch: *cdb.char_data) c_int {
    loadPlayerJson(std.mem.span(path), ch) catch return -1;
    return 0;
}

pub export fn json_player_objects_save(path: [*:0]const u8, ch: *cdb.char_data, rentcode: c_int, cost: c_int) c_int {
    writePlayerObjectsJson(std.mem.span(path), ch, rentcode, cost) catch return -1;
    return 0;
}

pub export fn json_player_objects_load(path: [*:0]const u8, ch: *cdb.char_data) c_int {
    return loadPlayerObjectsJson(std.mem.span(path), ch) catch return -1;
}

pub export fn json_house_objects_save(path: [*:0]const u8, room_vnum: cdb.room_vnum) c_int {
    writeHouseObjectsJson(std.mem.span(path), room_vnum) catch return -1;
    return 0;
}

pub export fn json_house_objects_load(path: [*:0]const u8, room_vnum: cdb.room_vnum) c_int {
    loadHouseObjectsJson(std.mem.span(path), room_vnum) catch return -1;
    return 0;
}

fn writePlayerJson(path: []const u8, ch: *cdb.char_data) !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const json = try characters_json.serializeCharacter(arena.allocator(), ch, .player);
    try writeJsonFile(path, json);
}

fn loadPlayerJson(path: []const u8, ch: *cdb.char_data) !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const value = try readJsonFile(arena.allocator(), path);
    try characters_json.deserializeCharacter(ch, .{ .mode = .player }, value);
}

fn writePlayerObjectsJson(path: []const u8, ch: *cdb.char_data, rentcode: c_int, cost: c_int) !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();
    var object = std.json.Value{ .object = std.json.ObjectMap.empty };

    try object.object.put(allocator, "kind", .{ .string = "player_objects" });
    try object.object.put(allocator, "version", .{ .integer = 1 });
    try object.object.put(allocator, "rent", try serializeRentInfo(allocator, rentcode, cost, ch));
    try object.object.put(allocator, "equipment", try serializeEquipment(allocator, ch));
    try object.object.put(allocator, "inventory", try serializeInventory(allocator, ch));

    try writeJsonFile(path, object);
}

fn loadPlayerObjectsJson(path: []const u8, ch: *cdb.char_data) !c_int {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const value = try readJsonFile(arena.allocator(), path);
    return try deserializePlayerObjects(ch, value);
}

fn writeHouseObjectsJson(path: []const u8, room_vnum: cdb.room_vnum) !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();
    const room = cdb.room_by_id(room_vnum);
    if (room == null) return error.RoomNotFound;

    var object = std.json.Value{ .object = std.json.ObjectMap.empty };
    try object.object.put(allocator, "kind", .{ .string = "house_objects" });
    try object.object.put(allocator, "version", .{ .integer = 1 });
    try object.object.put(allocator, "room", .{ .integer = room_vnum });
    try object.object.put(allocator, "objects", try serializeRoomInventory(allocator, room.*.contents));

    try writeJsonFile(path, object);
}

fn loadHouseObjectsJson(path: []const u8, room_vnum: cdb.room_vnum) !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const value = try readJsonFile(arena.allocator(), path);
    try deserializeHouseObjects(room_vnum, value);
}

fn serializeRentInfo(allocator: std.mem.Allocator, rentcode: c_int, cost: c_int, ch: *cdb.char_data) !std.json.Value {
    var rent = std.json.Value{ .object = std.json.ObjectMap.empty };
    try rent.object.put(allocator, "code", .{ .integer = rentcode });
    try rent.object.put(allocator, "time", .{ .integer = @intCast(time(null)) });
    try rent.object.put(allocator, "cost", .{ .integer = cost });
    try rent.object.put(allocator, "gold", .{ .integer = ch.gold });
    try rent.object.put(allocator, "bank", .{ .integer = ch.bank_gold });
    try rent.object.put(allocator, "items", .{ .integer = 0 });
    return rent;
}

fn serializeEquipment(allocator: std.mem.Allocator, ch: *cdb.char_data) !std.json.Value {
    var array = std.json.Array.init(allocator);
    for (ch.equipment[0..], 0..) |obj, index| {
        if (obj) |item| try array.append(try serializeObjectTree(allocator, item, @intCast(index + 1)));
    }
    return .{ .array = array };
}

fn serializeInventory(allocator: std.mem.Allocator, ch: *cdb.char_data) !std.json.Value {
    var array = std.json.Array.init(allocator);
    var obj = ch.carrying;
    while (obj) |item| : (obj = item.*.next_content) {
        try array.append(try serializeObjectTree(allocator, item, 0));
    }
    return .{ .array = array };
}

fn serializeRoomInventory(allocator: std.mem.Allocator, contents: ?*cdb.obj_data) !std.json.Value {
    var array = std.json.Array.init(allocator);
    var obj = contents;
    while (obj) |item| : (obj = item.*.next_content) {
        if (cdb.obj_extra_flagged(item, cdb.ITEM_NORENT)) continue;
        try array.append(try serializeObjectTree(allocator, item, 0));
    }
    return .{ .array = array };
}

fn serializeObjectTree(allocator: std.mem.Allocator, obj: *cdb.obj_data, location: c_int) !std.json.Value {
    var object = try objects_json.serializeObject(allocator, obj, .instance);
    _ = object.object.swapRemove("id");
    try jsonx.putInt(&object, allocator, "location", location);
    try jsonx.putInt(&object, allocator, "weight", objectBaseWeight(obj));
    try jsonx.putInt(&object, allocator, "rent", obj.cost_per_day);
    try jsonx.putInt(&object, allocator, "generation", obj.generation);
    try jsonx.putInt(&object, allocator, "unique_id", obj.unique_id);
    try jsonx.putNonEmpty(&object, allocator, "object_affects", try serializeObjectAffects(allocator, obj));
    try jsonx.putNonEmpty(&object, allocator, "spellbook", try serializeSpellbook(allocator, obj));

    var contents = std.json.Array.init(allocator);
    var child = obj.contains;
    while (child) |item| : (child = item.*.next_content) {
        if (cdb.obj_extra_flagged(item, cdb.ITEM_NORENT)) continue;
        try contents.append(try serializeObjectTree(allocator, item, -1));
    }
    if (contents.items.len > 0) try jsonx.put(&object, allocator, "contains", .{ .array = contents });
    return object;
}

fn objectBaseWeight(obj: *cdb.obj_data) i64 {
    var contained: i64 = 0;
    var child = obj.contains;
    while (child) |item| : (child = item.*.next_content) contained += item.*.weight;
    return obj.weight - contained;
}

fn serializeObjectAffects(allocator: std.mem.Allocator, obj: *cdb.obj_data) !std.json.Value {
    var array = std.json.Array.init(allocator);
    for (obj.affected[0..]) |aff| {
        if (aff.location == 0 and aff.modifier == 0 and aff.specific == 0) continue;
        var item = std.json.Value{ .object = std.json.ObjectMap.empty };
        try jsonx.putInt(&item, allocator, "location", aff.location);
        try jsonx.putInt(&item, allocator, "modifier", aff.modifier);
        try jsonx.putInt(&item, allocator, "specific", aff.specific);
        try array.append(item);
    }
    return .{ .array = array };
}

fn serializeSpellbook(allocator: std.mem.Allocator, obj: *cdb.obj_data) !std.json.Value {
    var array = std.json.Array.init(allocator);
    if (obj.sbinfo == null) return .{ .array = array };
    for (0..cdb.SPELLBOOK_SIZE) |index| {
        const spell = obj.sbinfo[index];
        if (spell.spellname == 0) break;
        var item = std.json.Value{ .object = std.json.ObjectMap.empty };
        try jsonx.putInt(&item, allocator, "spell", spell.spellname);
        try jsonx.putInt(&item, allocator, "pages", spell.pages);
        try array.append(item);
    }
    return .{ .array = array };
}

fn deserializePlayerObjects(ch: *cdb.char_data, value: std.json.Value) !c_int {
    if (value != .object) return error.ExpectedObject;
    const rent = jsonx.field(value, "rent") orelse return error.ExpectedObject;
    const rentcode = try jsonx.intField(rent, "code", c_int) orelse 0;
    if (rentcode == RENT_RENTED or rentcode == RENT_TIMEDOUT) {
        // Preserve legacy Crash_load side effect before applying rented objects.
        json_save_char_for_objects(ch);
    }

    if (jsonx.field(value, "equipment")) |equipment| try deserializeObjectRoots(ch, equipment);
    if (jsonx.field(value, "inventory")) |inventory| try deserializeObjectRoots(ch, inventory);
    return if (rentcode == RENT_RENTED or rentcode == RENT_CRYO) 0 else 1;
}

fn deserializeObjectRoots(ch: *cdb.char_data, value: std.json.Value) !void {
    if (value != .array) return error.ExpectedArray;
    for (value.array.items) |item| {
        const location = try jsonx.intField(item, "location", c_int) orelse 0;
        const obj = try deserializeObjectTree(item);
        if (location > 0) {
            json_obj_auto_equip(ch, obj, location);
        } else {
            obj_to_char(obj, ch);
        }
    }
}

fn deserializeHouseObjects(room_vnum: cdb.room_vnum, value: std.json.Value) !void {
    if (value != .object) return error.ExpectedObject;
    const room = cdb.room_by_id(room_vnum);
    if (room == null) return error.RoomNotFound;
    const objects = jsonx.field(value, "objects") orelse return error.ExpectedArray;
    if (objects != .array) return error.ExpectedArray;
    for (objects.array.items) |item| {
        const obj = try deserializeObjectTree(item);
        obj_to_room(obj, room);
    }
}

fn deserializeObjectTree(value: std.json.Value) !*cdb.obj_data {
    if (value != .object) return error.ExpectedObject;
    const proto_id = try jsonx.intField(value, "proto_id", cdb.obj_vnum) orelse cdb.NOTHING;
    const obj = if (proto_id != cdb.NOTHING) read_object(proto_id, VIRTUAL) orelse try createUniqueObject() else try createUniqueObject();
    obj.ex_description = null;
    try objects_json.deserializeObject(obj, .{ .mode = .instance, .preserve_id = false }, value);
    if (try jsonx.intField(value, "rent", c_int)) |v| obj.cost_per_day = v;
    if (try jsonx.intField(value, "generation", cdb.time_t)) |v| obj.generation = v;
    if (try jsonx.intField(value, "unique_id", i64)) |v| obj.unique_id = v;
    if (jsonx.field(value, "object_affects")) |affects| try deserializeObjectAffects(obj, affects);
    if (jsonx.field(value, "spellbook")) |spellbook| try deserializeSpellbook(obj, spellbook);

    if (jsonx.field(value, "contains")) |contents| {
        if (contents != .array) return error.ExpectedArray;
        for (contents.array.items) |item| {
            const child = try deserializeObjectTree(item);
            obj_to_obj(child, obj);
        }
    }

    check_unique_id(obj);
    add_unique_id(obj);
    if (obj.type_flag == cdb.ITEM_DRINKCON) {
        name_from_drinkcon(obj);
        if (obj.value[1] != 0) name_to_drinkcon(obj, obj.value[2]);
    }
    if ((proto_id == 20099 or proto_id == 20098) and cdb.obj_extra_flagged(obj, cdb.ITEM_UNBREAKABLE)) {
        cdb.obj_extra_flag_set(obj, cdb.ITEM_UNBREAKABLE, false);
    }
    return obj;
}

fn createUniqueObject() !*cdb.obj_data {
    const obj = create_obj() orelse return error.OutOfMemory;
    obj.vnum = cdb.NOTHING;
    obj.size = cdb.SIZE_MEDIUM;
    return obj;
}

fn deserializeObjectAffects(obj: *cdb.obj_data, value: std.json.Value) !void {
    if (value != .array) return error.ExpectedArray;
    for (obj.affected[0..]) |*aff| {
        aff.location = cdb.APPLY_NONE;
        aff.modifier = 0;
        aff.specific = 0;
    }
    for (value.array.items, 0..) |item, index| {
        if (index >= obj.affected.len) break;
        if (item != .object) return error.ExpectedObject;
        if (try jsonx.intField(item, "location", c_int)) |v| obj.affected[index].location = v;
        if (try jsonx.intField(item, "modifier", c_int)) |v| obj.affected[index].modifier = v;
        if (try jsonx.intField(item, "specific", c_int)) |v| obj.affected[index].specific = v;
    }
}

fn deserializeSpellbook(obj: *cdb.obj_data, value: std.json.Value) !void {
    if (value != .array) return error.ExpectedArray;
    if (value.array.items.len == 0) return;
    if (obj.sbinfo == null) {
        obj.sbinfo = @ptrCast(@alignCast(calloc(cdb.SPELLBOOK_SIZE, @sizeOf(cdb.obj_spellbook_spell)) orelse return error.OutOfMemory));
    }
    for (0..cdb.SPELLBOOK_SIZE) |index| {
        obj.sbinfo[index].spellname = 0;
        obj.sbinfo[index].pages = 0;
    }
    for (value.array.items, 0..) |item, index| {
        if (index >= cdb.SPELLBOOK_SIZE) break;
        if (item != .object) return error.ExpectedObject;
        if (try jsonx.intField(item, "spell", c_int)) |v| obj.sbinfo[index].spellname = v;
        if (try jsonx.intField(item, "pages", c_int)) |v| obj.sbinfo[index].pages = v;
    }
}

fn writeJsonFile(path: []const u8, value: std.json.Value) !void {
    if (!has_io) return error.NotInitialized;
    var out: std.Io.Writer.Allocating = .init(std.heap.page_allocator);
    defer out.deinit();
    try std.json.Stringify.value(value, .{ .whitespace = .indent_2 }, &out.writer);
    try out.writer.writeByte('\n');
    try std.Io.Dir.cwd().writeFile(global_io, .{ .sub_path = path, .data = out.written() });
}

fn readJsonFile(allocator: std.mem.Allocator, path: []const u8) !std.json.Value {
    if (!has_io) return error.NotInitialized;
    const data = try std.Io.Dir.cwd().readFileAlloc(global_io, path, allocator, .limited(16 * 1024 * 1024));
    return try std.json.parseFromSliceLeaky(std.json.Value, allocator, data, .{});
}
