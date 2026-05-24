const cdb = @import("cdb");
const std = @import("std");

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub export fn exit_dest_get(exit: *cdb.room_direction_data) [*c]cdb.room_data {
    if (exit.to_room == cdb.NOWHERE or cdb.world == null) return null;
    return &cdb.world[@intCast(exit.to_room)];
}

pub export fn exit_general_description_get(exit: *cdb.room_direction_data) [*c]const u8 {
    return exit.general_description;
}

pub export fn exit_general_description_set(exit: *cdb.room_direction_data, desc: ?[*:0]const u8) void {
    replaceString(&exit.general_description, desc);
}

pub export fn exit_keyword_get(exit: *cdb.room_direction_data) [*c]const u8 {
    return exit.keyword;
}

pub export fn exit_keyword_set(exit: *cdb.room_direction_data, keyword: ?[*:0]const u8) void {
    replaceString(&exit.keyword, keyword);
}

pub export fn exit_info_get(exit: *cdb.room_direction_data) i16 {
    return exit.exit_info;
}

pub export fn exit_info_set(exit: *cdb.room_direction_data, info: i16) void {
    exit.exit_info = info;
}

pub export fn exit_key_get(exit: *cdb.room_direction_data) cdb.obj_vnum {
    return exit.key;
}

pub export fn exit_key_set(exit: *cdb.room_direction_data, key: cdb.obj_vnum) void {
    exit.key = key;
}

pub export fn exit_to_room_get(exit: *cdb.room_direction_data) cdb.room_vnum {
    return roomVnumFromRnum(exit.to_room);
}

pub export fn exit_to_room_set(exit: *cdb.room_direction_data, to_room: cdb.room_vnum) void {
    exit.to_room = cdb.real_room(to_room);
}

pub export fn exit_dclock_get(exit: *cdb.room_direction_data) c_int {
    return exit.dclock;
}

pub export fn exit_dclock_set(exit: *cdb.room_direction_data, dclock: c_int) void {
    exit.dclock = dclock;
}

pub export fn exit_dchide_get(exit: *cdb.room_direction_data) c_int {
    return exit.dchide;
}

pub export fn exit_dchide_set(exit: *cdb.room_direction_data, dchide: c_int) void {
    exit.dchide = dchide;
}

pub export fn exit_dcskill_get(exit: *cdb.room_direction_data) c_int {
    return exit.dcskill;
}

pub export fn exit_dcskill_set(exit: *cdb.room_direction_data, dcskill: c_int) void {
    exit.dcskill = dcskill;
}

pub export fn exit_dcmove_get(exit: *cdb.room_direction_data) c_int {
    return exit.dcmove;
}

pub export fn exit_dcmove_set(exit: *cdb.room_direction_data, dcmove: c_int) void {
    exit.dcmove = dcmove;
}

pub export fn exit_failsavetype_get(exit: *cdb.room_direction_data) c_int {
    return exit.failsavetype;
}

pub export fn exit_failsavetype_set(exit: *cdb.room_direction_data, failsavetype: c_int) void {
    exit.failsavetype = failsavetype;
}

pub export fn exit_dcfailsave_get(exit: *cdb.room_direction_data) c_int {
    return exit.dcfailsave;
}

pub export fn exit_dcfailsave_set(exit: *cdb.room_direction_data, dcfailsave: c_int) void {
    exit.dcfailsave = dcfailsave;
}

pub export fn exit_failroom_get(exit: *cdb.room_direction_data) c_int {
    return roomVnumFromRnum(exit.failroom);
}

pub export fn exit_failroom_set(exit: *cdb.room_direction_data, failroom: cdb.room_vnum) void {
    exit.failroom = cdb.real_room(failroom);
}

pub export fn exit_totalfailroom_get(exit: *cdb.room_direction_data) c_int {
    return roomVnumFromRnum(exit.totalfailroom);
}

pub export fn exit_totalfailroom_set(exit: *cdb.room_direction_data, totalfailroom: cdb.room_vnum) void {
    exit.totalfailroom = cdb.real_room(totalfailroom);
}

fn replaceString(field: *[*c]u8, value: ?[*:0]const u8) void {
    const new_value = if (value) |new_string| strdup(new_string) orelse return else null;
    if (field.* != null) std.c.free(field.*);
    field.* = new_value;
}

fn roomVnumFromRnum(rnum: cdb.room_rnum) cdb.room_vnum {
    if (rnum == cdb.NOWHERE or cdb.world == null) return cdb.NOWHERE;
    return cdb.world[@intCast(rnum)].number;
}
