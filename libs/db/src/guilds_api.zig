const cdb = @import("cdb");
const std = @import("std");
const bitflags = @import("flags.zig");

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub export fn guild_id_get(guild: *cdb.guild_data) cdb.guild_vnum {
    return guild.vnum;
}
pub export fn guild_id_set(guild: *cdb.guild_data, id: cdb.guild_vnum) void {
    guild.vnum = id;
}
pub export fn guild_skill_get(guild: *cdb.guild_data, skill: usize) bool {
    return skill < guild.skills.len and guild.skills[skill] != 0;
}
pub export fn guild_skill_set(guild: *cdb.guild_data, skill: usize, value: bool) void {
    if (skill >= guild.skills.len) return;
    guild.skills[skill] = if (value) 1 else 0;
}
pub export fn guild_feat_get(guild: *cdb.guild_data, feat: usize) bool {
    return feat < guild.feats.len and guild.feats[feat] != 0;
}
pub export fn guild_feat_set(guild: *cdb.guild_data, feat: usize, value: bool) void {
    if (feat >= guild.feats.len) return;
    guild.feats[feat] = if (value) 1 else 0;
}
pub export fn guild_charge_get(guild: *cdb.guild_data) f32 {
    return guild.charge;
}
pub export fn guild_charge_set(guild: *cdb.guild_data, charge: f32) void {
    guild.charge = charge;
}
pub export fn guild_no_such_skill_get(guild: *cdb.guild_data) [*c]const u8 {
    return guild.no_such_skill;
}
pub export fn guild_no_such_skill_set(guild: *cdb.guild_data, value: ?[*:0]const u8) void {
    replaceString(&guild.no_such_skill, value);
}
pub export fn guild_not_enough_gold_get(guild: *cdb.guild_data) [*c]const u8 {
    return guild.not_enough_gold;
}
pub export fn guild_not_enough_gold_set(guild: *cdb.guild_data, value: ?[*:0]const u8) void {
    replaceString(&guild.not_enough_gold, value);
}
pub export fn guild_min_level_get(guild: *cdb.guild_data) c_int {
    return guild.minlvl;
}
pub export fn guild_min_level_set(guild: *cdb.guild_data, level: c_int) void {
    guild.minlvl = level;
}
pub export fn guild_master_get(guild: *cdb.guild_data) cdb.mob_vnum {
    return guild.gm;
}
pub export fn guild_master_set(guild: *cdb.guild_data, vnum: cdb.mob_vnum) void {
    guild.gm = vnum;
}
pub export fn guild_trade_flagged(guild: *cdb.guild_data, pos: c_int) bool {
    return bitflags.get(&guild.with_who, pos);
}
pub export fn guild_trade_flag_toggle(guild: *cdb.guild_data, pos: c_int) bool {
    return bitflags.toggle(&guild.with_who, pos);
}
pub export fn guild_trade_flag_set(guild: *cdb.guild_data, pos: c_int, value: bool) void {
    bitflags.set(&guild.with_who, pos, value);
}
pub export fn guild_open_get(guild: *cdb.guild_data) c_int {
    return guild.open;
}
pub export fn guild_open_set(guild: *cdb.guild_data, value: c_int) void {
    guild.open = value;
}
pub export fn guild_close_get(guild: *cdb.guild_data) c_int {
    return guild.close;
}
pub export fn guild_close_set(guild: *cdb.guild_data, value: c_int) void {
    guild.close = value;
}
pub export fn guild_func_get(guild: *cdb.guild_data) cdb.SpecialFunc {
    return guild.func;
}
pub export fn guild_func_set(guild: *cdb.guild_data, func: cdb.SpecialFunc) void {
    guild.func = func;
}

fn replaceString(field: *[*c]u8, value: ?[*:0]const u8) void {
    const new_value = if (value) |s| strdup(s) orelse return else null;
    if (field.* != null) std.c.free(field.*);
    field.* = new_value;
}
