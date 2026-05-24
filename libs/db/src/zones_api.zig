const cdb = @import("cdb");
const std = @import("std");
const bitflags = @import("flags.zig");

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub export fn zone_id_get(zone: *cdb.zone_data) cdb.zone_vnum { return zone.number; }
pub export fn zone_id_set(zone: *cdb.zone_data, id: cdb.zone_vnum) void { zone.number = id; }
pub export fn zone_name_get(zone: *cdb.zone_data) [*c]const u8 { return zone.name; }
pub export fn zone_name_set(zone: *cdb.zone_data, value: ?[*:0]const u8) void { replaceString(&zone.name, value); }
pub export fn zone_builders_get(zone: *cdb.zone_data) [*c]const u8 { return zone.builders; }
pub export fn zone_builders_set(zone: *cdb.zone_data, value: ?[*:0]const u8) void { replaceString(&zone.builders, value); }
pub export fn zone_lifespan_get(zone: *cdb.zone_data) c_int { return zone.lifespan; }
pub export fn zone_lifespan_set(zone: *cdb.zone_data, lifespan: c_int) void { zone.lifespan = lifespan; }
pub export fn zone_age_get(zone: *cdb.zone_data) c_int { return zone.age; }
pub export fn zone_age_set(zone: *cdb.zone_data, age: c_int) void { zone.age = age; }
pub export fn zone_bottom_get(zone: *cdb.zone_data) cdb.room_vnum { return zone.bot; }
pub export fn zone_bottom_set(zone: *cdb.zone_data, bottom: cdb.room_vnum) void { zone.bot = bottom; }
pub export fn zone_top_get(zone: *cdb.zone_data) cdb.room_vnum { return zone.top; }
pub export fn zone_top_set(zone: *cdb.zone_data, top: cdb.room_vnum) void { zone.top = top; }
pub export fn zone_reset_mode_get(zone: *cdb.zone_data) c_int { return zone.reset_mode; }
pub export fn zone_reset_mode_set(zone: *cdb.zone_data, mode: c_int) void { zone.reset_mode = mode; }
pub export fn zone_min_level_get(zone: *cdb.zone_data) c_int { return zone.min_level; }
pub export fn zone_min_level_set(zone: *cdb.zone_data, level: c_int) void { zone.min_level = level; }
pub export fn zone_max_level_get(zone: *cdb.zone_data) c_int { return zone.max_level; }
pub export fn zone_max_level_set(zone: *cdb.zone_data, level: c_int) void { zone.max_level = level; }
pub export fn zone_flagged(zone: *cdb.zone_data, pos: c_int) bool { return bitflags.get(&zone.zone_flags, pos); }
pub export fn zone_flag_toggle(zone: *cdb.zone_data, pos: c_int) bool { return bitflags.toggle(&zone.zone_flags, pos); }
pub export fn zone_flag_set(zone: *cdb.zone_data, pos: c_int, value: bool) void { bitflags.set(&zone.zone_flags, pos, value); }
pub export fn zone_command_get(zone: *cdb.zone_data, index: usize) [*c]cdb.reset_com { if (zone.cmd == null) return null; return &zone.cmd[index]; }

pub export fn zone_command_type_get(cmd: *cdb.reset_com) u8 { return cmd.command; }
pub export fn zone_command_type_set(cmd: *cdb.reset_com, command: u8) void { cmd.command = command; }
pub export fn zone_command_if_flag_get(cmd: *cdb.reset_com) bool { return cmd.if_flag; }
pub export fn zone_command_if_flag_set(cmd: *cdb.reset_com, value: bool) void { cmd.if_flag = value; }
pub export fn zone_command_arg_get(cmd: *cdb.reset_com, index: usize) c_int { return switch (index) { 0 => cmd.arg1, 1 => cmd.arg2, 2 => cmd.arg3, 3 => cmd.arg4, 4 => cmd.arg5, else => 0 }; }
pub export fn zone_command_arg_set(cmd: *cdb.reset_com, index: usize, value: c_int) void { switch (index) { 0 => cmd.arg1 = value, 1 => cmd.arg2 = value, 2 => cmd.arg3 = value, 3 => cmd.arg4 = value, 4 => cmd.arg5 = value, else => {} } }
pub export fn zone_command_line_get(cmd: *cdb.reset_com) c_int { return cmd.line; }
pub export fn zone_command_line_set(cmd: *cdb.reset_com, line: c_int) void { cmd.line = line; }
pub export fn zone_command_sarg1_get(cmd: *cdb.reset_com) [*c]const u8 { return cmd.sarg1; }
pub export fn zone_command_sarg1_set(cmd: *cdb.reset_com, value: ?[*:0]const u8) void { replaceString(&cmd.sarg1, value); }
pub export fn zone_command_sarg2_get(cmd: *cdb.reset_com) [*c]const u8 { return cmd.sarg2; }
pub export fn zone_command_sarg2_set(cmd: *cdb.reset_com, value: ?[*:0]const u8) void { replaceString(&cmd.sarg2, value); }

fn replaceString(field: *[*c]u8, value: ?[*:0]const u8) void { const new_value = if (value) |s| strdup(s) orelse return else null; if (field.* != null) std.c.free(field.*); field.* = new_value; }
