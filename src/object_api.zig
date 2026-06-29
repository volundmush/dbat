const cdb = @import("cdb");
const std = @import("std");
const bitflags = @import("flags.zig");
const intern_mod = @import("intern.zig");
const lua_api = @import("lua_api.zig");
const script_instance_mod = @import("script_instance.zig");

pub const ObjIterFn = *const fn (*cdb.obj_data, ?*anyopaque) callconv(.c) bool;

const ScriptInstance = script_instance_mod.ScriptInstance;
const ScriptId = intern_mod.InternedId;
const ScriptMap = std.AutoHashMap(ScriptId, ScriptInstance);

var obj_script_allocator: std.mem.Allocator = undefined;
var obj_scripts_global: std.AutoHashMap(i64, ScriptMap) = undefined;

pub fn init(alloc: std.mem.Allocator) void {
    obj_script_allocator = alloc;
    obj_scripts_global = std.AutoHashMap(i64, ScriptMap).init(alloc);
}

pub fn deinit() void {
    var outer = obj_scripts_global.iterator();
    while (outer.next()) |entry| {
        var inner = entry.value_ptr.iterator();
        while (inner.next()) |se| {
            se.value_ptr.deinit(obj_script_allocator);
        }
        entry.value_ptr.deinit();
    }
    obj_scripts_global.deinit();
}

pub fn freeObjScripts(obj_id: i64) void {
    if (obj_scripts_global.fetchRemove(obj_id)) |kv| {
        var map = kv.value;
        var it = map.iterator();
        while (it.next()) |entry| entry.value_ptr.deinit(obj_script_allocator);
        map.deinit();
    }
}

fn getObjScriptMap(obj: *cdb.obj_data) ?*ScriptMap {
    return obj_scripts_global.getPtr(cdb.obj_id_get(obj));
}

fn getOrCreateObjScriptMap(obj: *cdb.obj_data) ?*ScriptMap {
    const obj_id = cdb.obj_id_get(obj);
    const result = obj_scripts_global.getOrPut(obj_id) catch return null;
    if (!result.found_existing) {
        result.value_ptr.* = ScriptMap.init(obj_script_allocator);
    }
    return result.value_ptr;
}

pub export fn obj_script_add(obj: *cdb.obj_data, script_id: ?[*:0]const u8) bool {
    const id_z = script_id orelse return false;
    const id = std.mem.span(id_z);
    const interned = intern_mod.lookup(id) orelse return false;
    const map = getOrCreateObjScriptMap(obj) orelse return false;
    if (map.contains(interned)) return false;
    const instance = ScriptInstance.init(obj_script_allocator, intern_mod.nameOf(interned));
    map.put(interned, instance) catch return false;
    lua_api.callObjScriptHook(obj, id, "on_apply", null);
    return true;
}

pub export fn obj_script_remove(obj: *cdb.obj_data, script_id: ?[*:0]const u8, reason: ?[*:0]const u8) bool {
    const id_z = script_id orelse return false;
    const id = std.mem.span(id_z);
    const interned = intern_mod.lookup(id) orelse return false;
    const map = getObjScriptMap(obj) orelse return false;
    if (!map.contains(interned)) return false;
    const reason_str: ?[]const u8 = if (reason) |r| std.mem.span(r) else null;
    lua_api.callObjScriptHook(obj, id, "on_remove", reason_str);
    if (map.fetchRemove(interned)) |kv| {
        var instance = kv.value;
        instance.deinit(obj_script_allocator);
    }
    return true;
}

pub export fn obj_script_has(obj: *cdb.obj_data, script_id: ?[*:0]const u8) bool {
    const id_z = script_id orelse return false;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return false;
    const map = getObjScriptMap(obj) orelse return false;
    return map.contains(interned);
}

pub export fn obj_script_number_get(obj: *cdb.obj_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8) i64 {
    const id_z = script_id orelse return 0;
    const key_z = key orelse return 0;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return 0;
    const map = getObjScriptMap(obj) orelse return 0;
    const instance = map.getPtr(interned) orelse return 0;
    return instance.numbers.get(std.mem.span(key_z)) orelse 0;
}

pub export fn obj_script_number_set(obj: *cdb.obj_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8, value: i64) void {
    const id_z = script_id orelse return;
    const key_z = key orelse return;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return;
    const map = getObjScriptMap(obj) orelse return;
    const instance = map.getPtr(interned) orelse return;
    instance.numbers.put(lua_api.internString(std.mem.span(key_z)), value) catch {};
}

pub export fn obj_script_text_get(obj: *cdb.obj_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8) [*c]const u8 {
    const id_z = script_id orelse return null;
    const key_z = key orelse return null;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return null;
    const map = getObjScriptMap(obj) orelse return null;
    const instance = map.getPtr(interned) orelse return null;
    return @ptrCast(instance.strings.get(std.mem.span(key_z)) orelse return null);
}

pub export fn obj_script_text_set(obj: *cdb.obj_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8, value: ?[*:0]const u8) void {
    const id_z = script_id orelse return;
    const key_z = key orelse return;
    const val_z = value orelse return;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return;
    const map = getObjScriptMap(obj) orelse return;
    const instance = map.getPtr(interned) orelse return;
    const key_str = lua_api.internString(std.mem.span(key_z));
    const val_copy = obj_script_allocator.dupe(u8, std.mem.span(val_z)) catch return;
    if (instance.strings.fetchPut(key_str, val_copy) catch null) |old| {
        obj_script_allocator.free(old.value);
    }
}

pub export fn obj_script_event_dispatch(obj: *cdb.obj_data, script_id: ?[*:0]const u8, event_name: ?[*:0]const u8) void {
    const id_z = script_id orelse return;
    const ev_z = event_name orelse return;
    lua_api.callObjScriptEventHook(obj, std.mem.span(id_z), std.mem.span(ev_z));
}

pub const ObjScriptEntry = struct { name: []const u8 };

pub const ObjScriptIterator = struct {
    inner: ScriptMap.Iterator,

    pub fn next(self: *ObjScriptIterator) ?ObjScriptEntry {
        const kv = self.inner.next() orelse return null;
        return .{ .name = intern_mod.nameOf(kv.key_ptr.*) };
    }
};

pub fn objectScriptIterator(obj: *const cdb.obj_data) ?ObjScriptIterator {
    const map = obj_scripts_global.getPtr(cdb.obj_id_get(@constCast(obj))) orelse return null;
    return .{ .inner = map.iterator() };
}

pub fn getObjScriptInstance(obj: *const cdb.obj_data, script_id: []const u8) ?*ScriptInstance {
    const interned = intern_mod.lookup(script_id) orelse return null;
    const map = obj_scripts_global.getPtr(cdb.obj_id_get(@constCast(obj))) orelse return null;
    return map.getPtr(interned);
}

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub export fn obj_proto_self_id_get(proto: *cdb.obj_proto_data) cdb.obj_vnum { return proto.id; }
pub export fn obj_proto_self_id_set(proto: *cdb.obj_proto_data, id: cdb.obj_vnum) void { proto.id = id; }

pub export fn obj_id_get(obj: *cdb.obj_data) i64 {
    return obj.id;
}

pub export fn obj_id_set(obj: *cdb.obj_data, id: i64) void {
    obj.id = @intCast(id);
}

pub export fn obj_proto_id_get(obj: *cdb.obj_data) cdb.obj_vnum {
    return obj_vnum_get(obj);
}

pub export fn obj_proto_id_set(obj: *cdb.obj_data, vnum: cdb.obj_vnum) void {
    obj_vnum_set(obj, vnum);
}

pub export fn obj_vnum_get(obj: *cdb.obj_data) cdb.obj_vnum {
    return obj.proto_id;
}

pub export fn obj_vnum_set(obj: *cdb.obj_data, vnum: cdb.obj_vnum) void {
    obj.proto_id = vnum;
}

pub export fn obj_room_get(obj: *cdb.obj_data) [*c]cdb.room_data {
    return cdb.room_by_id(obj.in_room);
}

pub export fn obj_room_vnum_get(obj: *cdb.obj_data) cdb.room_vnum {
    const room = obj_room_get(obj);
    if (room == null) return cdb.NOWHERE;
    return room.*.id;
}

pub export fn obj_room_vnum_set(obj: *cdb.obj_data, vnum: cdb.room_vnum) void {
    obj.in_room = cdb.room_vnum_check(vnum);
}

pub export fn obj_room_loaded_get(obj: *cdb.obj_data) cdb.room_vnum {
    return obj.room_loaded;
}

pub export fn obj_room_loaded_set(obj: *cdb.obj_data, vnum: cdb.room_vnum) void {
    obj.room_loaded = vnum;
}

pub export fn obj_value_get(obj: *cdb.obj_data, pos: usize) c_int {
    if (pos >= obj.value.len) return 0;
    return obj.value[pos];
}

pub export fn obj_value_mod(obj: *cdb.obj_data, pos: usize, delta: c_int) c_int {
    if (pos >= obj.value.len) return 0;
    obj.value[pos] += delta;
    return obj.value[pos];
}

pub export fn obj_value_set(obj: *cdb.obj_data, pos: usize, value: c_int) void {
    if (pos >= obj.value.len) return;
    obj.value[pos] = value;
}

pub export fn obj_type_get(obj: *cdb.obj_data) i8 {
    return obj.type_flag;
}

pub export fn obj_type_set(obj: *cdb.obj_data, obj_type: i8) void {
    obj.type_flag = obj_type;
}

pub export fn obj_level_get(obj: *cdb.obj_data) c_int {
    return obj.level;
}

pub export fn obj_level_set(obj: *cdb.obj_data, level: c_int) void {
    obj.level = level;
}

pub export fn obj_wear_flagged(obj: *cdb.obj_data, pos: c_int) bool {
    return bitflags.get(&obj.wear_flags, pos);
}

pub export fn obj_wear_flag_toggle(obj: *cdb.obj_data, pos: c_int) bool {
    return bitflags.toggle(&obj.wear_flags, pos);
}

pub export fn obj_wear_flag_set(obj: *cdb.obj_data, pos: c_int, value: bool) void {
    bitflags.set(&obj.wear_flags, pos, value);
}

pub export fn obj_extra_flagged(obj: *cdb.obj_data, pos: c_int) bool {
    return bitflags.get(&obj.extra_flags, pos);
}

pub export fn obj_extra_flag_toggle(obj: *cdb.obj_data, pos: c_int) bool {
    return bitflags.toggle(&obj.extra_flags, pos);
}

pub export fn obj_extra_flag_set(obj: *cdb.obj_data, pos: c_int, value: bool) void {
    if (pos == cdb.ITEM_BROKEN) {
        obj_broken_set(obj, value);
        return;
    }
    bitflags.set(&obj.extra_flags, pos, value);
}

pub export fn obj_broken_set(obj: *cdb.obj_data, value: bool) void {
    const was_broken = bitflags.get(&obj.extra_flags, cdb.ITEM_BROKEN);
    bitflags.set(&obj.extra_flags, cdb.ITEM_BROKEN, value);
    if (!value or was_broken) return;

    const wearer = obj.worn_by orelse return;
    const pos = obj.worn_on;
    if (pos < 0) return;
    _ = cdb.act("@W$p@W falls apart and you remove it.@n", cdb.FALSE, wearer, obj, null, cdb.TO_CHAR);
    _ = cdb.act("@W$p@W falls apart and @C$n@W remove it.@n", cdb.FALSE, wearer, obj, null, cdb.TO_ROOM);
    cdb.perform_remove(wearer, pos);
}

pub export fn obj_aff_flagged(obj: *cdb.obj_data, pos: c_int) bool {
    return bitflags.get(&obj.bitvector, pos);
}

pub export fn obj_aff_flag_toggle(obj: *cdb.obj_data, pos: c_int) bool {
    return bitflags.toggle(&obj.bitvector, pos);
}

pub export fn obj_aff_flag_set(obj: *cdb.obj_data, pos: c_int, value: bool) void {
    bitflags.set(&obj.bitvector, pos, value);
}

pub export fn obj_weight_get(obj: *cdb.obj_data) i64 {
    return obj.weight;
}

pub export fn obj_weight_get_contained(obj: *cdb.obj_data) i64 {
    var total: i64 = 0;
    var count: usize = 0;
    const ids = obj_contents_ids(obj, &count) orelse return 0;
    defer obj_contents_ids_free(ids);
    for (ids[0..count]) |id| {
        const child = cdb.obj_by_id(id) orelse continue;
        total += obj_weight_get_total(child);
    }
    return total;
}

pub export fn obj_weight_get_total(obj: *cdb.obj_data) i64 {
    return obj.weight + obj_weight_get_contained(obj);
}

pub export fn obj_weight_mod(obj: *cdb.obj_data, delta: i64) i64 {
    obj.weight += delta;
    return obj.weight;
}

pub export fn obj_weight_set(obj: *cdb.obj_data, weight: i64) void {
    obj.weight = weight;
}

pub export fn obj_cost_get(obj: *cdb.obj_data) c_int {
    return obj.cost;
}

pub export fn obj_cost_mod(obj: *cdb.obj_data, delta: c_int) c_int {
    obj.cost += delta;
    return obj.cost;
}

pub export fn obj_cost_set(obj: *cdb.obj_data, cost: c_int) void {
    obj.cost = cost;
}

pub export fn obj_timer_get(obj: *cdb.obj_data) c_int {
    return obj.timer;
}

pub export fn obj_timer_mod(obj: *cdb.obj_data, delta: c_int) c_int {
    obj.timer += delta;
    return obj.timer;
}

pub export fn obj_timer_set(obj: *cdb.obj_data, timer: c_int) void {
    obj.timer = timer;
}

pub export fn obj_size_get(obj: *cdb.obj_data) c_int {
    return obj.size;
}

pub export fn obj_size_set(obj: *cdb.obj_data, size: c_int) void {
    obj.size = size;
}

pub export fn obj_name_get(obj: *cdb.obj_data) [*c]const u8 {
    return obj.name;
}

pub export fn obj_name_set(obj: *cdb.obj_data, value: ?[*:0]const u8) void {
    replaceString(&obj.name, value);
}

pub export fn obj_description_get(obj: *cdb.obj_data) [*c]const u8 {
    return obj.description;
}

pub export fn obj_description_set(obj: *cdb.obj_data, value: ?[*:0]const u8) void {
    replaceString(&obj.description, value);
}

pub export fn obj_short_description_get(obj: *cdb.obj_data) [*c]const u8 {
    return obj.short_description;
}

pub export fn obj_short_description_set(obj: *cdb.obj_data, value: ?[*:0]const u8) void {
    replaceString(&obj.short_description, value);
}

pub export fn obj_action_description_get(obj: *cdb.obj_data) [*c]const u8 {
    return obj.action_description;
}

pub export fn obj_action_description_set(obj: *cdb.obj_data, value: ?[*:0]const u8) void {
    replaceString(&obj.action_description, value);
}

pub export fn obj_carried_by_get(obj: *cdb.obj_data) i64 {
    return charId(obj.carried_by);
}

pub export fn obj_carried_by_set(obj: *cdb.obj_data, ch: [*c]cdb.char_data) void {
    obj.carried_by = ch;
}

pub export fn obj_worn_by_get(obj: *cdb.obj_data) i64 {
    return charId(obj.worn_by);
}

pub export fn obj_worn_by_set(obj: *cdb.obj_data, ch: [*c]cdb.char_data) void {
    obj.worn_by = ch;
}

pub export fn obj_worn_on_get(obj: *cdb.obj_data) i16 {
    return obj.worn_on;
}

pub export fn obj_worn_on_set(obj: *cdb.obj_data, pos: i16) void {
    obj.worn_on = pos;
}

pub export fn obj_in_obj_get(obj: *cdb.obj_data) i64 {
    return objId(obj.in_obj);
}

pub export fn obj_in_obj_set(obj: *cdb.obj_data, in_obj: [*c]cdb.obj_data) void {
    obj.in_obj = in_obj;
}

extern fn obj_contents_ids(container: *cdb.obj_data, out_count: *usize) ?[*]i64;
extern fn obj_contents_ids_free(ptr: ?[*]i64) void;

pub export fn obj_inventory_count(obj: *cdb.obj_data, recursive: bool) usize {
    var ids_count: usize = 0;
    const ids = obj_contents_ids(obj, &ids_count) orelse return 0;
    defer obj_contents_ids_free(ids);
    if (!recursive) return ids_count;
    var total = ids_count;
    for (ids[0..ids_count]) |id| {
        const child = cdb.obj_by_id(id) orelse continue;
        total += obj_inventory_count(child, true);
    }
    return total;
}

pub export fn obj_inventory_get(obj: *cdb.obj_data, count: *usize) ?[*]i64 {
    return obj_contents_ids(obj, count);
}

pub fn objContentsIterate(container: *cdb.obj_data, recursive: bool, func: ObjIterFn, ctx: ?*anyopaque) bool {
    var ids_count: usize = 0;
    const ids = obj_contents_ids(container, &ids_count) orelse return true;
    defer obj_contents_ids_free(ids);
    for (ids[0..ids_count]) |id| {
        const obj = cdb.obj_by_id(id) orelse continue;
        if (!func(obj, ctx)) return false;
        if (recursive and !objContentsIterate(obj, true, func, ctx)) return false;
    }
    return true;
}

pub fn objContentsListIterate(obj: [*c]cdb.obj_data, recursive: bool, func: ObjIterFn, ctx: ?*anyopaque) bool {
    if (obj == null) return true;
    return objContentsIterate(@ptrCast(obj), recursive, func, ctx);
}

pub export fn obj_contents_list_iterate(obj: [*c]cdb.obj_data, recursive: bool, func: ?ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    _ = objContentsListIterate(obj, recursive, callback, ctx);
}

pub export fn obj_inventory_iterate(obj: *cdb.obj_data, recursive: bool, func: ?ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    _ = objContentsIterate(obj, recursive, callback, ctx);
}

pub export fn obj_next_content_get(obj: *cdb.obj_data) [*c]cdb.obj_data {
    _ = obj;
    return null;
}

pub export fn obj_contains_get(obj: *cdb.obj_data) [*c]cdb.obj_data {
    _ = obj;
    return null;
}

pub export fn obj_sitting_get(obj: *cdb.obj_data) i64 {
    return charId(obj.sitting);
}

pub export fn obj_sitting_set(obj: *cdb.obj_data, ch: [*c]cdb.char_data) void {
    obj.sitting = ch;
}

fn replaceString(field: *[*c]u8, value: ?[*:0]const u8) void {
    const new_value = if (value) |new_string| strdup(new_string) orelse return else null;
    if (field.* != null) std.c.free(field.*);
    field.* = new_value;
}

fn charId(ch: [*c]cdb.char_data) i64 {
    if (ch == null) return 0;
    return ch.*.id;
}

fn objId(obj: [*c]cdb.obj_data) i64 {
    if (obj == null) return 0;
    return obj.*.id;
}
