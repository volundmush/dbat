const cdb = @import("cdb");
const std = @import("std");
const bitflags = @import("flags.zig");
const obj_api = @import("object_api.zig");
const intern_mod = @import("intern.zig");
const lua_api = @import("lua_api.zig");
const script_instance_mod = @import("script_instance.zig");

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;
extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;

pub export fn room_id_get(room: *cdb.room_data) cdb.room_vnum {
    return room.id;
}

pub export fn room_id_set(room: *cdb.room_data, id: cdb.room_vnum) void {
    room.id = id;
}

pub export fn room_vnum_get(room: *cdb.room_data) cdb.room_vnum {
    return room.id;
}

pub export fn room_vnum_set(room: *cdb.room_data, vnum: cdb.room_vnum) void {
    room.id = vnum;
}

pub export fn room_zone_get(room: *cdb.room_data) [*c]cdb.zone_data {
    const zone = cdb.zone_by_id(room.zone);
    if (zone == null) return null;
    return zone;
}

pub export fn room_zone_vnum_get(room: *cdb.room_data) cdb.zone_vnum {
    const zone = cdb.zone_by_id(room.zone);
    if (zone == null) return cdb.NOWHERE;
    return zone.*.id;
}

pub export fn room_zone_set(room: *cdb.room_data, vnum: cdb.zone_vnum) void {
    room.zone = vnum;
}

pub export fn room_sector_type_get(room: *cdb.room_data) c_int {
    return room.sector_type;
}

pub export fn room_sector_type_set(room: *cdb.room_data, sector_type: c_int) void {
    room.sector_type = sector_type;
}

pub export fn room_name_get(room: *cdb.room_data) [*c]const u8 {
    return room.name;
}

pub export fn room_name_set(room: *cdb.room_data, name: ?[*:0]const u8) void {
    replaceString(&room.name, name);
}

pub export fn room_description_get(room: *cdb.room_data) [*c]const u8 {
    return room.description;
}

pub export fn room_description_set(room: *cdb.room_data, description: ?[*:0]const u8) void {
    replaceString(&room.description, description);
}

pub export fn room_flagged(room: *cdb.room_data, pos: c_int) c_int {
    return if (bitflags.get(&room.room_flags, pos)) cdb.TRUE else cdb.FALSE;
}

pub export fn room_flag_toggle(room: *cdb.room_data, pos: c_int) bool {
    return bitflags.toggle(&room.room_flags, pos);
}

pub export fn room_flag_set(room: *cdb.room_data, pos: c_int, value: bool) void {
    bitflags.set(&room.room_flags, pos, value);
}

pub export fn room_light_get(room: *cdb.room_data) u16 {
    return room.light;
}

pub export fn room_light_mod(room: *cdb.room_data, delta: i16) void {
    const updated = @as(i32, room.light) + delta;
    room.light = if (updated <= 0) 0 else if (updated > std.math.maxInt(u16)) std.math.maxInt(u16) else @intCast(updated);
}

pub export fn room_light_set(room: *cdb.room_data, light: u16) void {
    room.light = light;
}

pub export fn room_func_get(room: *cdb.room_data) cdb.SpecialFunc {
    return room.func;
}

pub export fn room_func_set(room: *cdb.room_data, func: cdb.SpecialFunc) void {
    room.func = func;
}

pub export fn room_timed_get(room: *cdb.room_data) c_int {
    return room.timed;
}

pub export fn room_timed_mod(room: *cdb.room_data, delta: c_int) void {
    room.timed += delta;
}

pub export fn room_timed_set(room: *cdb.room_data, timed: c_int) void {
    room.timed = timed;
}

pub export fn room_dmg_get(room: *cdb.room_data) c_int {
    return room.dmg;
}

pub export fn room_dmg_mod(room: *cdb.room_data, delta: c_int) void {
    room.dmg += delta;
}

pub export fn room_dmg_set(room: *cdb.room_data, dmg: c_int) void {
    room.dmg = dmg;
}

pub export fn room_gravity_get(room: *cdb.room_data) c_int {
    return room.gravity;
}

pub export fn room_gravity_mod(room: *cdb.room_data, delta: c_int) void {
    room.gravity += delta;
}

pub export fn room_gravity_set(room: *cdb.room_data, gravity: c_int) void {
    room.gravity = gravity;
}

pub export fn room_geffect_get(room: *cdb.room_data) c_int {
    return room.geffect;
}

fn syncGeoEffectScript(room: *cdb.room_data) void {
    if (room.geffect > 0 and room.geffect < 6) {
        _ = room_script_add(room, "geo_effect");
    } else {
        _ = room_script_remove(room, "geo_effect", "inactive");
    }
}

pub export fn room_geffect_mod(room: *cdb.room_data, delta: c_int) void {
    room.geffect += delta;
    syncGeoEffectScript(room);
}

pub export fn room_geffect_set(room: *cdb.room_data, geffect: c_int) void {
    room.geffect = geffect;
    syncGeoEffectScript(room);
}

pub export fn room_dir_option_get(room: *cdb.room_data, dir: c_int) [*c]cdb.room_direction_data {
    if (dir < 0) return null;
    const index: usize = @intCast(dir);
    if (index >= room.dir_option.len) return null;
    return room.dir_option[index];
}

extern fn room_person_first(room: *cdb.room_data) ?*cdb.char_data;

pub export fn room_people_get(room: *cdb.room_data) [*c]cdb.char_data {
    return room_person_first(room);
}

pub export fn room_ex_description_get(room: *cdb.room_data) [*c]cdb.extra_descr_data {
    return room.ex_description;
}

pub export fn room_script_get(room: *cdb.room_data) [*c]cdb.script_data {
    return room.script;
}

pub export fn room_proto_script_get(room: *cdb.room_data) [*c]cdb.trig_proto_list {
    return room.proto_script;
}

pub export fn room_script_ensure(room: *cdb.room_data) [*c]cdb.script_data {
    if (room.script == null) {
        room.script = @ptrCast(@alignCast(calloc(1, @sizeOf(cdb.script_data)) orelse return null));
    }
    return room.script;
}

pub export fn room_script_set(room: *cdb.room_data, script: ?*cdb.script_data) void {
    room.script = script;
}

extern fn room_object_first(room: *cdb.room_data) ?*cdb.obj_data;
extern fn room_object_ids(room: *cdb.room_data, out_count: *usize) ?[*]i64;

pub export fn room_contents_get(room: *cdb.room_data) [*c]cdb.obj_data {
    return room_object_first(room);
}

pub export fn room_objects_get(room: *cdb.room_data, count: *usize) ?[*]i64 {
    return room_object_ids(room, count);
}

fn replaceString(field: *[*c]u8, value: ?[*:0]const u8) void {
    const new_value = if (value) |new_string| strdup(new_string) orelse return else null;
    if (field.* != null) std.c.free(field.*);
    field.* = new_value;
}

extern fn room_person_ids(room: *cdb.room_data, out_count: *usize) ?[*]i64;
extern fn room_person_ids_free(ptr: ?[*]i64) void;
extern fn room_object_ids_free(ptr: ?[*]i64) void;

pub export fn room_contents_iterate(room: *cdb.room_data, recursive: bool, func: ?obj_api.ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    var count: usize = 0;
    const ids = room_object_ids(room, &count) orelse return;
    defer room_object_ids_free(ids);
    for (ids[0..count]) |id| {
        const obj = cdb.obj_by_id(id) orelse continue;
        if (!callback(obj, ctx)) return;
        if (recursive and !obj_api.objContentsIterate(obj, true, callback, ctx)) return;
    }
}

pub export fn room_people_iterate(room: *cdb.room_data, func: cdb.char_iter_fn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    var count: usize = 0;
    const ids = room_person_ids(room, &count) orelse return;
    defer room_person_ids_free(ids);
    for (ids[0..count]) |id| {
        const ch = cdb.char_by_id(id) orelse continue;
        if (!callback(ch, ctx)) return;
    }
}

// ---- Room Script Storage ----

const ScriptInstance = script_instance_mod.ScriptInstance;
const ScriptId = intern_mod.InternedId;
const ScriptMap = std.AutoHashMap(ScriptId, ScriptInstance);

var room_script_allocator: std.mem.Allocator = undefined;
var room_scripts_global: std.AutoHashMap(i64, ScriptMap) = undefined;

pub fn init(alloc: std.mem.Allocator) void {
    room_script_allocator = alloc;
    room_scripts_global = std.AutoHashMap(i64, ScriptMap).init(alloc);
}

pub fn deinit() void {
    var outer = room_scripts_global.iterator();
    while (outer.next()) |entry| {
        var inner = entry.value_ptr.iterator();
        while (inner.next()) |se| {
            se.value_ptr.deinit(room_script_allocator);
        }
        entry.value_ptr.deinit();
    }
    room_scripts_global.deinit();
}

fn roomKey(room: *cdb.room_data) i64 {
    return @as(i64, @intCast(cdb.room_vnum_get(room)));
}

fn getRoomScriptMap(room: *cdb.room_data) ?*ScriptMap {
    return room_scripts_global.getPtr(roomKey(room));
}

fn getOrCreateRoomScriptMap(room: *cdb.room_data) ?*ScriptMap {
    const result = room_scripts_global.getOrPut(roomKey(room)) catch return null;
    if (!result.found_existing) {
        result.value_ptr.* = ScriptMap.init(room_script_allocator);
    }
    return result.value_ptr;
}

pub export fn room_script_add(room: *cdb.room_data, script_id: ?[*:0]const u8) bool {
    const id_z = script_id orelse return false;
    const id = std.mem.span(id_z);
    const interned = intern_mod.lookup(id) orelse return false;
    const map = getOrCreateRoomScriptMap(room) orelse return false;
    if (map.contains(interned)) return false;
    const instance = ScriptInstance.init(room_script_allocator, intern_mod.nameOf(interned));
    map.put(interned, instance) catch return false;
    lua_api.callRoomScriptHook(room, id, "on_apply", null);
    return true;
}

pub export fn room_script_remove(room: *cdb.room_data, script_id: ?[*:0]const u8, reason: ?[*:0]const u8) bool {
    const id_z = script_id orelse return false;
    const id = std.mem.span(id_z);
    const interned = intern_mod.lookup(id) orelse return false;
    const map = getRoomScriptMap(room) orelse return false;
    if (!map.contains(interned)) return false;
    const reason_str: ?[]const u8 = if (reason) |r| std.mem.span(r) else null;
    lua_api.callRoomScriptHook(room, id, "on_remove", reason_str);
    if (map.fetchRemove(interned)) |kv| {
        var instance = kv.value;
        instance.deinit(room_script_allocator);
    }
    return true;
}

pub export fn room_script_has(room: *cdb.room_data, script_id: ?[*:0]const u8) bool {
    const id_z = script_id orelse return false;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return false;
    const map = getRoomScriptMap(room) orelse return false;
    return map.contains(interned);
}

pub export fn room_script_number_get(room: *cdb.room_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8) i64 {
    const id_z = script_id orelse return 0;
    const key_z = key orelse return 0;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return 0;
    const map = getRoomScriptMap(room) orelse return 0;
    const instance = map.getPtr(interned) orelse return 0;
    return instance.numbers.get(std.mem.span(key_z)) orelse 0;
}

pub export fn room_script_number_set(room: *cdb.room_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8, value: i64) void {
    const id_z = script_id orelse return;
    const key_z = key orelse return;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return;
    const map = getRoomScriptMap(room) orelse return;
    const instance = map.getPtr(interned) orelse return;
    instance.numbers.put(lua_api.internString(std.mem.span(key_z)), value) catch {};
}

pub export fn room_script_text_get(room: *cdb.room_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8) [*c]const u8 {
    const id_z = script_id orelse return null;
    const key_z = key orelse return null;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return null;
    const map = getRoomScriptMap(room) orelse return null;
    const instance = map.getPtr(interned) orelse return null;
    return @ptrCast(instance.strings.get(std.mem.span(key_z)) orelse return null);
}

pub export fn room_script_text_set(room: *cdb.room_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8, value: ?[*:0]const u8) void {
    const id_z = script_id orelse return;
    const key_z = key orelse return;
    const val_z = value orelse return;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return;
    const map = getRoomScriptMap(room) orelse return;
    const instance = map.getPtr(interned) orelse return;
    const key_str = lua_api.internString(std.mem.span(key_z));
    const val_copy = room_script_allocator.dupe(u8, std.mem.span(val_z)) catch return;
    if (instance.strings.fetchPut(key_str, val_copy) catch null) |old| {
        room_script_allocator.free(old.value);
    }
}

pub export fn room_script_event_dispatch(room: *cdb.room_data, script_id: ?[*:0]const u8, event_name: ?[*:0]const u8) void {
    const id_z = script_id orelse return;
    const ev_z = event_name orelse return;
    lua_api.callRoomScriptEventHook(room, std.mem.span(id_z), std.mem.span(ev_z));
}

pub const RoomScriptEntry = struct { name: []const u8 };

pub const RoomScriptIterator = struct {
    inner: ScriptMap.Iterator,

    pub fn next(self: *RoomScriptIterator) ?RoomScriptEntry {
        const kv = self.inner.next() orelse return null;
        return .{ .name = intern_mod.nameOf(kv.key_ptr.*) };
    }
};

pub fn roomScriptIterator(room: *const cdb.room_data) ?RoomScriptIterator {
    const key = @as(i64, @intCast(cdb.room_vnum_get(@constCast(room))));
    const map = room_scripts_global.getPtr(key) orelse return null;
    return .{ .inner = map.iterator() };
}
