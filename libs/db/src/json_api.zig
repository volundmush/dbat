const std = @import("std");
const cdb = @import("cdb");
const rooms_json = @import("rooms_json.zig");
const exits_json = @import("exits_json.zig");
const characters_json = @import("characters_json.zig");
const objects_json = @import("objects_json.zig");
const zones_json = @import("zones_json.zig");
const shops_json = @import("shops_json.zig");
const guilds_json = @import("guilds_json.zig");
const dgscripts_json = @import("dgscripts_json.zig");

var global_io: std.Io = undefined;
var has_io = false;

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;

pub fn init(io: std.Io) void {
    global_io = io;
    has_io = true;
}

pub export fn json_export_room(vnum: cdb.room_vnum, filename: ?[*:0]const u8) c_int {
    exportRoom(vnum, cString(filename)) catch return -1;
    return 0;
}

pub export fn json_export_rooms(folder: ?[*:0]const u8) c_int {
    exportRooms(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_export_room_exits(vnum: cdb.room_vnum, filename: ?[*:0]const u8) c_int {
    exportRoomExits(vnum, cString(filename)) catch return -1;
    return 0;
}

pub export fn json_export_all_room_exits(folder: ?[*:0]const u8) c_int {
    exportAllRoomExits(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_export_npc_prototype(vnum: cdb.mob_vnum, filename: ?[*:0]const u8) c_int {
    exportNpcPrototype(vnum, cString(filename)) catch return -1;
    return 0;
}

pub export fn json_export_npc_prototypes(folder: ?[*:0]const u8) c_int {
    exportNpcPrototypes(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_export_obj_prototype(vnum: cdb.obj_vnum, filename: ?[*:0]const u8) c_int {
    exportObjPrototype(vnum, cString(filename)) catch return -1;
    return 0;
}

pub export fn json_export_obj_prototypes(folder: ?[*:0]const u8) c_int {
    exportObjPrototypes(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_export_zone(vnum: cdb.zone_vnum, filename: ?[*:0]const u8) c_int {
    exportZone(vnum, cString(filename)) catch return -1;
    return 0;
}

pub export fn json_export_zones(folder: ?[*:0]const u8) c_int {
    exportZones(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_export_shop(vnum: cdb.shop_vnum, filename: ?[*:0]const u8) c_int {
    exportShop(vnum, cString(filename)) catch return -1;
    return 0;
}

pub export fn json_export_shops(folder: ?[*:0]const u8) c_int {
    exportShops(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_export_guild(vnum: cdb.guild_vnum, filename: ?[*:0]const u8) c_int {
    exportGuild(vnum, cString(filename)) catch return -1;
    return 0;
}

pub export fn json_export_guilds(folder: ?[*:0]const u8) c_int {
    exportGuilds(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_export_dgscript(vnum: cdb.trig_vnum, filename: ?[*:0]const u8) c_int {
    exportDgScript(vnum, cString(filename)) catch return -1;
    return 0;
}

pub export fn json_export_dgscripts(folder: ?[*:0]const u8) c_int {
    exportDgScripts(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_export_all(folder: ?[*:0]const u8) c_int {
    exportAll(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_import_all(folder: ?[*:0]const u8) void {
    importAll(cString(folder)) catch return;
}

pub export fn json_import_zones(folder: ?[*:0]const u8) c_int {
    importZones(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_import_rooms(folder: ?[*:0]const u8) c_int {
    importRooms(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_import_room_exits(folder: ?[*:0]const u8) c_int {
    importRoomExits(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_import_dgscripts(folder: ?[*:0]const u8) c_int {
    importDgScripts(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_import_npc_prototypes(folder: ?[*:0]const u8) c_int {
    importNpcPrototypes(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_import_obj_prototypes(folder: ?[*:0]const u8) c_int {
    importObjPrototypes(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_import_shops(folder: ?[*:0]const u8) c_int {
    importShops(cString(folder)) catch return -1;
    return 0;
}

pub export fn json_import_guilds(folder: ?[*:0]const u8) c_int {
    importGuilds(cString(folder)) catch return -1;
    return 0;
}

fn exportRoom(vnum: cdb.room_vnum, filename: []const u8) !void {
    const room = cdb.room_by_id(vnum);
    if (room == null) return error.NotFound;
    try writeJsonFile(filename, rooms_json.serializeRoom, .{room});
}

fn exportRooms(folder: []const u8) !void {
    try ensureFolder(folder);
    if (cdb.world == null) return;
    if (cdb.top_of_world < 0) return;
    var index: usize = 0;
    while (index <= @as(usize, @intCast(cdb.top_of_world))) : (index += 1) {
        const room = ptrAt(cdb.room_data, cdb.world, index);
        const path = try assetPath(folder, room.number);
        defer std.heap.page_allocator.free(path);
        try exportRoom(room.number, path);
    }
}

fn exportRoomExits(vnum: cdb.room_vnum, filename: []const u8) !void {
    const room = cdb.room_by_id(vnum);
    if (room == null) return error.NotFound;
    try writeJsonFile(filename, exits_json.serializeRoomExits, .{room});
}

fn exportAllRoomExits(folder: []const u8) !void {
    try ensureFolder(folder);
    if (cdb.world == null) return;
    if (cdb.top_of_world < 0) return;
    var index: usize = 0;
    while (index <= @as(usize, @intCast(cdb.top_of_world))) : (index += 1) {
        const room = ptrAt(cdb.room_data, cdb.world, index);
        const path = try assetPath(folder, room.number);
        defer std.heap.page_allocator.free(path);
        try exportRoomExits(room.number, path);
    }
}

fn exportNpcPrototype(vnum: cdb.mob_vnum, filename: []const u8) !void {
    const mob = cdb.mob_proto_by_id(vnum);
    if (mob == null) return error.NotFound;
    try writeJsonFile(filename, characters_json.serializeCharacter, .{ mob, characters_json.CharacterJsonMode.npc_prototype });
}

fn exportNpcPrototypes(folder: []const u8) !void {
    try ensureFolder(folder);
    if (cdb.mob_proto == null or cdb.mob_index == null) return;
    if (cdb.top_of_mobt < 0) return;
    var index: usize = 0;
    while (index <= @as(usize, @intCast(cdb.top_of_mobt))) : (index += 1) {
        const path = try assetPath(folder, cdb.mob_index[index].vnum);
        defer std.heap.page_allocator.free(path);
        try exportNpcPrototype(cdb.mob_index[index].vnum, path);
    }
}

fn exportObjPrototype(vnum: cdb.obj_vnum, filename: []const u8) !void {
    const obj = cdb.obj_proto_by_id(vnum);
    if (obj == null) return error.NotFound;
    try writeJsonFile(filename, objects_json.serializeObject, .{ obj, objects_json.ObjectJsonMode.prototype });
}

fn exportObjPrototypes(folder: []const u8) !void {
    try ensureFolder(folder);
    if (cdb.obj_proto == null or cdb.obj_index == null) return;
    if (cdb.top_of_objt < 0) return;
    var index: usize = 0;
    while (index <= @as(usize, @intCast(cdb.top_of_objt))) : (index += 1) {
        const path = try assetPath(folder, cdb.obj_index[index].vnum);
        defer std.heap.page_allocator.free(path);
        try exportObjPrototype(cdb.obj_index[index].vnum, path);
    }
}

fn exportZone(vnum: cdb.zone_vnum, filename: []const u8) !void {
    const zone = cdb.zone_by_id(vnum);
    if (zone == null) return error.NotFound;
    try writeJsonFile(filename, zones_json.serializeZone, .{zone});
}

fn exportZones(folder: []const u8) !void {
    try ensureFolder(folder);
    if (cdb.zone_table == null) return;
    if (cdb.top_of_zone_table < 0) return;
    var index: usize = 0;
    while (index <= @as(usize, @intCast(cdb.top_of_zone_table))) : (index += 1) {
        const zone = ptrAt(cdb.zone_data, cdb.zone_table, index);
        const path = try assetPath(folder, zone.number);
        defer std.heap.page_allocator.free(path);
        try exportZone(zone.number, path);
    }
}

fn exportShop(vnum: cdb.shop_vnum, filename: []const u8) !void {
    const shop = cdb.shop_by_id(vnum);
    if (shop == null) return error.NotFound;
    try writeJsonFile(filename, shops_json.serializeShop, .{shop});
}

fn exportShops(folder: []const u8) !void {
    try ensureFolder(folder);
    if (cdb.shop_index == null) return;
    if (cdb.top_shop < 0) return;
    var index: usize = 0;
    while (index <= @as(usize, @intCast(cdb.top_shop))) : (index += 1) {
        const shop = ptrAt(cdb.shop_data, cdb.shop_index, index);
        const path = try assetPath(folder, shop.vnum);
        defer std.heap.page_allocator.free(path);
        try exportShop(shop.vnum, path);
    }
}

fn exportGuild(vnum: cdb.guild_vnum, filename: []const u8) !void {
    const guild = cdb.guild_by_id(vnum);
    if (guild == null) return error.NotFound;
    try writeJsonFile(filename, guilds_json.serializeGuild, .{guild});
}

fn exportGuilds(folder: []const u8) !void {
    try ensureFolder(folder);
    if (cdb.guild_index == null) return;
    if (cdb.top_guild < 0) return;
    var index: usize = 0;
    while (index <= @as(usize, @intCast(cdb.top_guild))) : (index += 1) {
        const guild = ptrAt(cdb.guild_data, cdb.guild_index, index);
        const path = try assetPath(folder, guild.vnum);
        defer std.heap.page_allocator.free(path);
        try exportGuild(guild.vnum, path);
    }
}

fn exportDgScript(vnum: cdb.trig_vnum, filename: []const u8) !void {
    const trigger = triggerByVnum(vnum) orelse return error.NotFound;
    try writeJsonFile(filename, dgscripts_json.serializeTrigger, .{trigger});
}

fn exportDgScripts(folder: []const u8) !void {
    try ensureFolder(folder);
    if (cdb.trig_index == null) return;
    if (cdb.top_of_trigt <= 0) return;
    var index: usize = 0;
    while (index < @as(usize, @intCast(cdb.top_of_trigt))) : (index += 1) {
        const entry = cdb.trig_index[index];
        if (entry != null and entry.*.proto != null) {
            const path = try assetPath(folder, entry.*.vnum);
            defer std.heap.page_allocator.free(path);
            try exportDgScript(entry.*.vnum, path);
        }
    }
}

fn exportAll(folder: []const u8) !void {
    try ensureFolder(folder);
    const rooms = try childPath(folder, "rooms");
    defer std.heap.page_allocator.free(rooms);
    const exits = try childPath(folder, "exits");
    defer std.heap.page_allocator.free(exits);
    const npc_prototypes = try childPath(folder, "npc_prototypes");
    defer std.heap.page_allocator.free(npc_prototypes);
    const obj_prototypes = try childPath(folder, "obj_prototypes");
    defer std.heap.page_allocator.free(obj_prototypes);
    const zones = try childPath(folder, "zones");
    defer std.heap.page_allocator.free(zones);
    const shops = try childPath(folder, "shops");
    defer std.heap.page_allocator.free(shops);
    const guilds = try childPath(folder, "guilds");
    defer std.heap.page_allocator.free(guilds);
    const dgscripts = try childPath(folder, "dgscripts");
    defer std.heap.page_allocator.free(dgscripts);

    try exportRooms(rooms);
    try exportAllRoomExits(exits);
    try exportNpcPrototypes(npc_prototypes);
    try exportObjPrototypes(obj_prototypes);
    try exportZones(zones);
    try exportShops(shops);
    try exportGuilds(guilds);
    try exportDgScripts(dgscripts);
}

fn importAll(folder: []const u8) !void {
    const zones = try childPath(folder, "zones");
    defer std.heap.page_allocator.free(zones);
    const rooms = try childPath(folder, "rooms");
    defer std.heap.page_allocator.free(rooms);
    const exits = try childPath(folder, "exits");
    defer std.heap.page_allocator.free(exits);
    const dgscripts = try childPath(folder, "dgscripts");
    defer std.heap.page_allocator.free(dgscripts);
    const npc_prototypes = try childPath(folder, "npc_prototypes");
    defer std.heap.page_allocator.free(npc_prototypes);
    const obj_prototypes = try childPath(folder, "obj_prototypes");
    defer std.heap.page_allocator.free(obj_prototypes);
    const shops = try childPath(folder, "shops");
    defer std.heap.page_allocator.free(shops);
    const guilds = try childPath(folder, "guilds");
    defer std.heap.page_allocator.free(guilds);

    try importZones(zones);
    try importRooms(rooms);
    try importRoomExits(exits);
    renumberRoomExits();
    try importDgScripts(dgscripts);
    try importNpcPrototypes(npc_prototypes);
    try importObjPrototypes(obj_prototypes);
    try importShops(shops);
    try importGuilds(guilds);
    renumberZoneCommands();
}

const JsonFile = struct {
    vnum: cdb.IDXTYPE,
    path: []const u8,
};

const Progress = struct {
    label: []const u8,
    total: usize,
    next_percent: usize = 10,

    fn init(label: []const u8, total: usize) Progress {
        std.log.info("JSON import {s}: {d} files", .{ label, total });
        if (total == 0) std.log.info("JSON import {s}: complete", .{label});
        return .{ .label = label, .total = total };
    }

    fn tick(self: *Progress, index: usize) void {
        if (self.total == 0) return;
        const done = index + 1;
        const percent = done * 100 / self.total;
        while (percent >= self.next_percent and self.next_percent <= 100) : (self.next_percent += 10) {
            std.log.info("JSON import {s}: {d}% ({d}/{d})", .{ self.label, self.next_percent, done, self.total });
        }
    }
};

fn logImportFileError(label: []const u8, file: JsonFile, err: anyerror) void {
    std.log.err("JSON import {s} failed for {s} (vnum {d}): {s}", .{ label, file.path, file.vnum, @errorName(err) });
}

fn importZones(folder: []const u8) !void {
    const files = try listJsonFiles(folder);
    var progress = Progress.init("zones", files.len);
    cdb.zone_table = try allocCArray(cdb.zone_data, files.len);
    cdb.top_of_zone_table = if (files.len == 0) -1 else @intCast(files.len - 1);

    for (files, 0..) |file, index| {
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const value = readJsonValue(arena.allocator(), file.path) catch |err| {
            logImportFileError("zones", file, err);
            return err;
        };
        const zone = ptrAt(cdb.zone_data, cdb.zone_table, index);
        zones_json.deserializeZone(zone, .{}, value) catch |err| {
            logImportFileError("zones", file, err);
            return err;
        };
        progress.tick(index);
    }
}

fn importRooms(folder: []const u8) !void {
    const files = try listJsonFiles(folder);
    var progress = Progress.init("rooms", files.len);
    cdb.world = try allocCArray(cdb.room_data, files.len);
    cdb.top_of_world = if (files.len == 0) -1 else @intCast(files.len - 1);
    resetHtree(&cdb.room_htree);

    for (files, 0..) |file, index| {
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const value = readJsonValue(arena.allocator(), file.path) catch |err| {
            logImportFileError("rooms", file, err);
            return err;
        };
        const room = ptrAt(cdb.room_data, cdb.world, index);
        room.number = @intCast(file.vnum);
        room.zone = zoneRnumForRoom(room.number);
        rooms_json.deserializeRoom(room, .{}, value) catch |err| {
            logImportFileError("rooms", file, err);
            return err;
        };
        room.zone = zoneRnumForRoom(room.number);
        cdb.htree_add(cdb.room_htree, room.number, @intCast(index));
        progress.tick(index);
    }
}

fn importRoomExits(folder: []const u8) !void {
    const files = try listJsonFiles(folder);
    var progress = Progress.init("exits", files.len);
    for (files, 0..) |file, index| {
        const room = cdb.room_by_id(@intCast(file.vnum));
        if (room == null) continue;
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const value = readJsonValue(arena.allocator(), file.path) catch |err| {
            logImportFileError("exits", file, err);
            return err;
        };
        exits_json.deserializeRoomExits(room, value) catch |err| {
            logImportFileError("exits", file, err);
            return err;
        };
        progress.tick(index);
    }
}

fn importDgScripts(folder: []const u8) !void {
    const files = try listJsonFiles(folder);
    var progress = Progress.init("dgscripts", files.len);
    cdb.trig_index = try allocCArray([*c]cdb.index_data, files.len);
    cdb.top_of_trigt = @intCast(files.len);

    for (files, 0..) |file, index| {
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const value = readJsonValue(arena.allocator(), file.path) catch |err| {
            logImportFileError("dgscripts", file, err);
            return err;
        };

        const entry = try allocCOne(cdb.index_data);
        const trigger = try allocCOne(cdb.trig_data);
        entry.vnum = @intCast(file.vnum);
        entry.number = 0;
        entry.func = null;
        entry.proto = trigger;
        trigger.nr = @intCast(index);
        dgscripts_json.deserializeTrigger(trigger, value) catch |err| {
            logImportFileError("dgscripts", file, err);
            return err;
        };
        trigger.nr = @intCast(index);
        cdb.trig_index[index] = entry;
        progress.tick(index);
    }
}

fn importNpcPrototypes(folder: []const u8) !void {
    const files = try listJsonFiles(folder);
    var progress = Progress.init("npc_prototypes", files.len);
    cdb.mob_proto = try allocCArray(cdb.char_data, files.len);
    cdb.mob_index = try allocCArray(cdb.index_data, files.len);
    cdb.top_of_mobt = if (files.len == 0) -1 else @intCast(files.len - 1);
    resetHtree(&cdb.mob_htree);

    for (files, 0..) |file, index| {
        cdb.mob_index[index].vnum = @intCast(file.vnum);
        cdb.mob_index[index].number = 0;
        cdb.mob_index[index].func = null;
        cdb.htree_add(cdb.mob_htree, @intCast(file.vnum), @intCast(index));
    }

    for (files, 0..) |file, index| {
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const value = readJsonValue(arena.allocator(), file.path) catch |err| {
            logImportFileError("npc_prototypes", file, err);
            return err;
        };
        const mob = ptrAt(cdb.char_data, cdb.mob_proto, index);
        mob.nr = @intCast(index);
        mob.player_specials = &cdb.dummy_mob;
        characters_json.deserializeCharacter(mob, .{ .mode = .npc_prototype }, value) catch |err| {
            logImportFileError("npc_prototypes", file, err);
            return err;
        };
        mob.nr = @intCast(index);
        mob.player_specials = &cdb.dummy_mob;
        progress.tick(index);
    }
}

fn importObjPrototypes(folder: []const u8) !void {
    const files = try listJsonFiles(folder);
    var progress = Progress.init("obj_prototypes", files.len);
    cdb.obj_proto = try allocCArray(cdb.obj_data, files.len);
    cdb.obj_index = try allocCArray(cdb.index_data, files.len);
    cdb.top_of_objt = if (files.len == 0) -1 else @intCast(files.len - 1);
    resetHtree(&cdb.obj_htree);

    for (files, 0..) |file, index| {
        cdb.obj_index[index].vnum = @intCast(file.vnum);
        cdb.obj_index[index].number = 0;
        cdb.obj_index[index].func = null;
        cdb.htree_add(cdb.obj_htree, @intCast(file.vnum), @intCast(index));
    }

    for (files, 0..) |file, index| {
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const value = readJsonValue(arena.allocator(), file.path) catch |err| {
            logImportFileError("obj_prototypes", file, err);
            return err;
        };
        const obj = ptrAt(cdb.obj_data, cdb.obj_proto, index);
        obj.item_number = @intCast(index);
        objects_json.deserializeObject(obj, .{ .mode = .prototype }, value) catch |err| {
            logImportFileError("obj_prototypes", file, err);
            return err;
        };
        obj.item_number = @intCast(index);
        progress.tick(index);
    }
}

fn importShops(folder: []const u8) !void {
    const files = try listJsonFiles(folder);
    var progress = Progress.init("shops", files.len);
    cdb.shop_index = try allocCArray(cdb.shop_data, files.len);
    cdb.top_shop = if (files.len == 0) -1 else @intCast(files.len - 1);

    for (files, 0..) |file, index| {
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const value = readJsonValue(arena.allocator(), file.path) catch |err| {
            logImportFileError("shops", file, err);
            return err;
        };
        const shop = ptrAt(cdb.shop_data, cdb.shop_index, index);
        shop.vnum = @intCast(file.vnum);
        shops_json.deserializeShop(shop, .{}, value) catch |err| {
            logImportFileError("shops", file, err);
            return err;
        };
        progress.tick(index);
    }
}

fn importGuilds(folder: []const u8) !void {
    const files = try listJsonFiles(folder);
    var progress = Progress.init("guilds", files.len);
    cdb.guild_index = try allocCArray(cdb.guild_data, files.len);
    cdb.top_guild = if (files.len == 0) -1 else @intCast(files.len - 1);

    for (files, 0..) |file, index| {
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const value = readJsonValue(arena.allocator(), file.path) catch |err| {
            logImportFileError("guilds", file, err);
            return err;
        };
        const guild = ptrAt(cdb.guild_data, cdb.guild_index, index);
        guild.vnum = @intCast(file.vnum);
        guilds_json.deserializeGuild(guild, .{}, value) catch |err| {
            logImportFileError("guilds", file, err);
            return err;
        };
        progress.tick(index);
    }
}

fn renumberRoomExits() void {
    if (cdb.world == null or cdb.top_of_world < 0) return;
    var room_index: usize = 0;
    while (room_index <= @as(usize, @intCast(cdb.top_of_world))) : (room_index += 1) {
        const room = ptrAt(cdb.room_data, cdb.world, room_index);
        for (0..cdb.NUM_OF_DIRS) |dir| {
            const exit = room.dir_option[dir];
            if (exit == null) continue;
            if (exit.*.to_room != cdb.NOWHERE) exit.*.to_room = cdb.real_room(exit.*.to_room);
        }
    }
}

fn renumberZoneCommands() void {
    if (cdb.zone_table == null or cdb.top_of_zone_table < 0) return;
    var zone_index: usize = 0;
    while (zone_index <= @as(usize, @intCast(cdb.top_of_zone_table))) : (zone_index += 1) {
        const zone = ptrAt(cdb.zone_data, cdb.zone_table, zone_index);
        if (zone.cmd == null) continue;
        var command_index: usize = 0;
        while (zone.cmd[command_index].command != 'S') : (command_index += 1) {
            const command: *cdb.reset_com = @ptrCast(&zone.cmd[command_index]);
            switch (command.command) {
                'M' => {
                    command.arg1 = cdb.real_mobile(command.arg1);
                    command.arg3 = cdb.real_room(command.arg3);
                },
                'O' => {
                    command.arg1 = cdb.real_object(command.arg1);
                    if (command.arg3 != cdb.NOWHERE) command.arg3 = cdb.real_room(command.arg3);
                },
                'G', 'E' => command.arg1 = cdb.real_object(command.arg1),
                'P' => {
                    command.arg1 = cdb.real_object(command.arg1);
                    command.arg3 = cdb.real_object(command.arg3);
                },
                'D' => command.arg1 = cdb.real_room(command.arg1),
                'R' => {
                    command.arg1 = cdb.real_room(command.arg1);
                    command.arg2 = cdb.real_object(command.arg2);
                },
                'T' => {
                    command.arg2 = realTrigger(command.arg2);
                    command.arg3 = cdb.real_room(command.arg3);
                },
                'V' => command.arg3 = cdb.real_room(command.arg3),
                else => {},
            }
        }
    }
}

fn realTrigger(vnum: cdb.trig_vnum) cdb.trig_rnum {
    if (cdb.trig_index == null or cdb.top_of_trigt <= 0) return cdb.NOTHING;
    var index: usize = 0;
    while (index < @as(usize, @intCast(cdb.top_of_trigt))) : (index += 1) {
        const entry = cdb.trig_index[index];
        if (entry != null and entry.*.vnum == vnum) return @intCast(index);
    }
    return cdb.NOTHING;
}

fn writeJsonFile(filename: []const u8, comptime serializer: anytype, args: anytype) !void {
    if (!has_io) return error.NotInitialized;
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();
    const value = try @call(.auto, serializer, .{allocator} ++ args);

    var out: std.Io.Writer.Allocating = .init(allocator);
    try std.json.Stringify.value(value, .{ .whitespace = .indent_2 }, &out.writer);
    try out.writer.writeByte('\n');
    try std.Io.Dir.cwd().writeFile(global_io, .{ .sub_path = filename, .data = out.written() });
}

fn readJsonValue(allocator: std.mem.Allocator, path: []const u8) !std.json.Value {
    if (!has_io) return error.NotInitialized;
    const bytes = try std.Io.Dir.cwd().readFileAlloc(global_io, path, allocator, .unlimited);
    return try std.json.parseFromSliceLeaky(std.json.Value, allocator, bytes, .{});
}

fn listJsonFiles(folder: []const u8) ![]JsonFile {
    if (!has_io) return error.NotInitialized;
    var dir = try std.Io.Dir.cwd().openDir(global_io, folder, .{ .iterate = true });
    defer dir.close(global_io);

    var list = std.array_list.Managed(JsonFile).init(std.heap.page_allocator);
    var iter = dir.iterate();
    while (try iter.next(global_io)) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.name, ".json")) continue;
        const stem = entry.name[0 .. entry.name.len - ".json".len];
        const vnum = std.fmt.parseInt(cdb.IDXTYPE, stem, 10) catch continue;
        try list.append(.{ .vnum = vnum, .path = try childPath(folder, entry.name) });
    }
    const files = try list.toOwnedSlice();
    std.mem.sort(JsonFile, files, {}, jsonFileLessThan);
    return files;
}

fn jsonFileLessThan(_: void, lhs: JsonFile, rhs: JsonFile) bool {
    return lhs.vnum < rhs.vnum;
}

fn allocCArray(comptime T: type, count: usize) ![*c]T {
    if (count == 0) return null;
    return @ptrCast(@alignCast(calloc(count, @sizeOf(T)) orelse return error.OutOfMemory));
}

fn allocCOne(comptime T: type) !*T {
    return @ptrCast(@alignCast(calloc(1, @sizeOf(T)) orelse return error.OutOfMemory));
}

fn ptrAt(comptime T: type, ptr: [*c]T, index: usize) *T {
    return @ptrCast(&ptr[index]);
}

fn resetHtree(tree: *[*c]cdb.htree_node) void {
    if (tree.* != null) cdb.htree_free(tree.*);
    tree.* = cdb.htree_init();
}

fn zoneRnumForRoom(vnum: cdb.room_vnum) cdb.zone_rnum {
    if (cdb.zone_table == null or cdb.top_of_zone_table < 0) return cdb.NOWHERE;
    var index: usize = 0;
    while (index <= @as(usize, @intCast(cdb.top_of_zone_table))) : (index += 1) {
        const zone = ptrAt(cdb.zone_data, cdb.zone_table, index);
        if (vnum >= zone.bot and vnum <= zone.top) return @intCast(index);
    }
    return cdb.NOWHERE;
}

fn ensureFolder(folder: []const u8) !void {
    if (!has_io) return error.NotInitialized;
    try std.Io.Dir.cwd().createDirPath(global_io, folder);
}

fn assetPath(folder: []const u8, vnum: anytype) ![]const u8 {
    return try std.fmt.allocPrint(std.heap.page_allocator, "{s}/{}.json", .{ folder, vnum });
}

fn childPath(folder: []const u8, child: []const u8) ![]const u8 {
    return try std.fmt.allocPrint(std.heap.page_allocator, "{s}/{s}", .{ folder, child });
}

fn triggerByVnum(vnum: cdb.trig_vnum) ?*cdb.trig_data {
    if (cdb.trig_index == null) return null;
    if (cdb.top_of_trigt <= 0) return null;
    var index: usize = 0;
    while (index < @as(usize, @intCast(cdb.top_of_trigt))) : (index += 1) {
        const entry = cdb.trig_index[index];
        if (entry != null and entry.*.vnum == vnum) return entry.*.proto;
    }
    return null;
}

fn cString(value: ?[*:0]const u8) []const u8 {
    if (value) |ptr| return std.mem.span(ptr);
    return "";
}
