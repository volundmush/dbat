const cdb = @import("cdb");
const std = @import("std");
const bitflags = @import("flags.zig");
const obj_api = @import("objects_api.zig");

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub export fn room_id_get(room: *cdb.room_data) cdb.room_vnum {
    return room.number;
}

pub export fn room_id_set(room: *cdb.room_data, id: cdb.room_vnum) void {
    room.number = id;
}

pub export fn room_vnum_get(room: *cdb.room_data) cdb.room_vnum {
    return room.number;
}

pub export fn room_vnum_set(room: *cdb.room_data, vnum: cdb.room_vnum) void {
    room.number = vnum;
}

pub export fn room_zone_get(room: *cdb.room_data) [*c]cdb.zone_data {
    if (room.zone == cdb.NOWHERE or cdb.zone_table == null) return null;
    return &cdb.zone_table[@intCast(room.zone)];
}

pub export fn room_zone_vnum_get(room: *cdb.room_data) cdb.zone_vnum {
    if (room.zone == cdb.NOWHERE or cdb.zone_table == null) return cdb.NOWHERE;
    return cdb.zone_table[@intCast(room.zone)].number;
}

pub export fn room_zone_set(room: *cdb.room_data, vnum: cdb.zone_vnum) void {
    room.zone = cdb.real_zone(vnum);
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

pub export fn room_geffect_mod(room: *cdb.room_data, delta: c_int) void {
    room.geffect += delta;
}

pub export fn room_geffect_set(room: *cdb.room_data, geffect: c_int) void {
    room.geffect = geffect;
}

pub export fn room_dir_option_get(room: *cdb.room_data, dir: c_int) [*c]cdb.room_direction_data {
    if (dir < 0) return null;
    const index: usize = @intCast(dir);
    if (index >= room.dir_option.len) return null;
    return room.dir_option[index];
}

pub export fn room_people_get(room: *cdb.room_data) [*c]cdb.char_data {
    return room.people;
}

pub export fn room_contents_get(room: *cdb.room_data) [*c]cdb.obj_data {
    return room.contents;
}

fn replaceString(field: *[*c]u8, value: ?[*:0]const u8) void {
    const new_value = if (value) |new_string| strdup(new_string) orelse return else null;
    if (field.* != null) std.c.free(field.*);
    field.* = new_value;
}

pub export fn room_contents_iterate(room: *cdb.room_data, recursive: bool, func: ?obj_api.ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    var current = room.contents;
    while (current != null) {
        const next = current.*.next_content;
        if (!callback(&current.*, ctx)) return;
        if (recursive and !obj_api.objContentsListIterate(current.*.contains, true, callback, ctx)) return;
        current = next;
    }
}

pub export fn room_people_iterate(room: *cdb.room_data, func: cdb.char_iter_fn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    var current = room.people;
    while (current != null) {
        const next = current.*.next_in_room;
        if (!callback(&current.*, ctx)) return;
        current = next;
    }
}
