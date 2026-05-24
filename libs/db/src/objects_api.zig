const cdb = @import("cdb");
const std = @import("std");
const bitflags = @import("flags.zig");

pub const ObjIterFn = *const fn (*cdb.obj_data, ?*anyopaque) callconv(.c) bool;

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub export fn obj_id_get(obj: *cdb.obj_data) i64 {
    return obj.id;
}

pub export fn obj_id_set(obj: *cdb.obj_data, id: i64) void {
    obj.id = @intCast(id);
}

pub export fn obj_proto_id_get(obj: *cdb.obj_data) cdb.obj_vnum {
    return obj.item_number;
}

pub export fn obj_proto_id_set(obj: *cdb.obj_data, vnum: cdb.obj_vnum) void {
    obj.item_number = vnum;
}

pub export fn obj_vnum_get(obj: *cdb.obj_data) cdb.obj_vnum {
    if (!validObjRnum(obj.item_number)) return cdb.NOTHING;
    return cdb.obj_index[@intCast(obj.item_number)].vnum;
}

pub export fn obj_vnum_set(obj: *cdb.obj_data, vnum: cdb.obj_vnum) void {
    obj.item_number = cdb.real_object(vnum);
}

pub export fn obj_room_vnum_get(obj: *cdb.obj_data) cdb.room_vnum {
    if (obj.in_room == cdb.NOWHERE or cdb.world == null) return cdb.NOWHERE;
    return cdb.world[@intCast(obj.in_room)].number;
}

pub export fn obj_room_vnum_set(obj: *cdb.obj_data, vnum: cdb.room_vnum) void {
    obj.in_room = cdb.real_room(vnum);
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
    bitflags.set(&obj.extra_flags, pos, value);
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
    var current = obj.contains;
    while (current != null) : (current = current.*.next_content) {
        total += obj_weight_get_total(&current.*);
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
    replaceString(obj, &obj.name, protoString(obj, .name), value);
}

pub export fn obj_description_get(obj: *cdb.obj_data) [*c]const u8 {
    return obj.description;
}

pub export fn obj_description_set(obj: *cdb.obj_data, value: ?[*:0]const u8) void {
    replaceString(obj, &obj.description, protoString(obj, .description), value);
}

pub export fn obj_short_description_get(obj: *cdb.obj_data) [*c]const u8 {
    return obj.short_description;
}

pub export fn obj_short_description_set(obj: *cdb.obj_data, value: ?[*:0]const u8) void {
    replaceString(obj, &obj.short_description, protoString(obj, .short_description), value);
}

pub export fn obj_action_description_get(obj: *cdb.obj_data) [*c]const u8 {
    return obj.action_description;
}

pub export fn obj_action_description_set(obj: *cdb.obj_data, value: ?[*:0]const u8) void {
    replaceString(obj, &obj.action_description, protoString(obj, .action_description), value);
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

pub export fn obj_inventory_count(obj: *cdb.obj_data, recursive: bool) usize {
    var count: usize = 0;
    var current = obj.contains;
    while (current != null) : (current = current.*.next_content) {
        count += 1;
        if (recursive) count += obj_inventory_count(&current.*, true);
    }
    return count;
}

pub fn objContentsListIterate(obj: [*c]cdb.obj_data, recursive: bool, func: ObjIterFn, ctx: ?*anyopaque) bool {
    var current = obj;
    while (current != null) {
        const next = current.*.next_content;
        if (!func(&current.*, ctx)) return false;
        if (recursive and !objContentsListIterate(current.*.contains, true, func, ctx)) return false;
        current = next;
    }
    return true;
}

pub export fn obj_contents_list_iterate(obj: [*c]cdb.obj_data, recursive: bool, func: ?ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    _ = objContentsListIterate(obj, recursive, callback, ctx);
}

pub export fn obj_inventory_iterate(obj: *cdb.obj_data, recursive: bool, func: ?ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    _ = objContentsListIterate(obj.contains, recursive, callback, ctx);
}

pub export fn obj_sitting_get(obj: *cdb.obj_data) i64 {
    return charId(obj.sitting);
}

pub export fn obj_sitting_set(obj: *cdb.obj_data, ch: [*c]cdb.char_data) void {
    obj.sitting = ch;
}

fn validObjRnum(rnum: cdb.obj_rnum) bool {
    return rnum != cdb.NOTHING and rnum >= 0 and rnum <= cdb.top_of_objt and cdb.obj_index != null;
}

fn validProto(obj: *cdb.obj_data) bool {
    return validObjRnum(obj.item_number) and cdb.obj_proto != null;
}

const StringField = enum { name, description, short_description, action_description };

fn protoString(obj: *cdb.obj_data, field: StringField) [*c]u8 {
    if (!validProto(obj)) return null;
    const proto = &cdb.obj_proto[@intCast(obj.item_number)];
    return switch (field) {
        .name => proto.name,
        .description => proto.description,
        .short_description => proto.short_description,
        .action_description => proto.action_description,
    };
}

fn replaceString(obj: *cdb.obj_data, field: *[*c]u8, proto_value: [*c]u8, value: ?[*:0]const u8) void {
    _ = obj;
    const new_value = if (value) |new_string| strdup(new_string) orelse return else null;
    if (field.* != null and field.* != proto_value) std.c.free(field.*);
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
