const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");

pub const JsonValue = jsonx.JsonValue;
pub const DeserializeOptions = struct { c_allocator: std.mem.Allocator = std.heap.c_allocator };

pub fn serializeGuild(allocator: std.mem.Allocator, guild: *cdb.guild_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putSlice(&object, allocator, "kind", "guild");
    try jsonx.putInt(&object, allocator, "id", cdb.guild_id_get(guild));
    try jsonx.putFloat(&object, allocator, "charge", cdb.guild_charge_get(guild));
    try jsonx.putString(&object, allocator, "no_such_skill", cdb.guild_no_such_skill_get(guild));
    try jsonx.putString(&object, allocator, "not_enough_gold", cdb.guild_not_enough_gold_get(guild));
    try jsonx.putInt(&object, allocator, "min_level", cdb.guild_min_level_get(guild));
    try jsonx.putInt(&object, allocator, "guildmaster", cdb.guild_master_get(guild));
    try jsonx.putInt(&object, allocator, "open", cdb.guild_open_get(guild));
    try jsonx.putInt(&object, allocator, "close", cdb.guild_close_get(guild));
    try jsonx.putNonEmpty(&object, allocator, "skills", try serializeEnabledIndexes(allocator, guild, guild.skills.len, cdb.guild_skill_get));
    try jsonx.putNonEmpty(&object, allocator, "feats", try serializeEnabledIndexes(allocator, guild, guild.feats.len, cdb.guild_feat_get));
    try jsonx.putNonEmpty(&object, allocator, "with_who", try jsonx.serializeFlags(allocator, guild, 128, guildTradeFlagged));
    return object;
}

pub fn deserializeGuild(guild: *cdb.guild_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    if (try jsonx.intField(value, "id", cdb.guild_vnum)) |v| cdb.guild_id_set(guild, v);
    if (try jsonx.floatField(value, "charge", f32)) |v| cdb.guild_charge_set(guild, v);
    if (try jsonx.stringField(value, "no_such_skill")) |v| try setString(options.c_allocator, guild, v, cdb.guild_no_such_skill_set);
    if (try jsonx.stringField(value, "not_enough_gold")) |v| try setString(options.c_allocator, guild, v, cdb.guild_not_enough_gold_set);
    if (try jsonx.intField(value, "min_level", c_int)) |v| cdb.guild_min_level_set(guild, v);
    if (try jsonx.intField(value, "guildmaster", cdb.mob_vnum)) |v| cdb.guild_master_set(guild, v);
    if (try jsonx.intField(value, "open", c_int)) |v| cdb.guild_open_set(guild, v);
    if (try jsonx.intField(value, "close", c_int)) |v| cdb.guild_close_set(guild, v);
    if (jsonx.field(value, "skills")) |items| try deserializeEnabledIndexes(guild, guild.skills.len, items, cdb.guild_skill_set);
    if (jsonx.field(value, "feats")) |items| try deserializeEnabledIndexes(guild, guild.feats.len, items, cdb.guild_feat_set);
    if (jsonx.field(value, "with_who")) |flags| try jsonx.deserializeFlags(guild, flags, 128, guildTradeFlagSet);
}

fn serializeEnabledIndexes(allocator: std.mem.Allocator, guild: *cdb.guild_data, len: usize, comptime getter: anytype) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    for (0..len) |index| {
        if (getter(guild, index)) try array.append(.{ .integer = @intCast(index) });
    }
    return .{ .array = array };
}

fn deserializeEnabledIndexes(guild: *cdb.guild_data, len: usize, value: JsonValue, comptime setter: anytype) !void {
    if (value != .array) return error.ExpectedArray;
    for (0..len) |index| setter(guild, index, false);
    for (value.array.items) |item| {
        if (item != .integer) return error.ExpectedInteger;
        const index = std.math.cast(usize, item.integer) orelse return error.IntegerOutOfRange;
        if (index >= len) return error.IntegerOutOfRange;
        setter(guild, index, true);
    }
}

fn setString(allocator: std.mem.Allocator, guild: *cdb.guild_data, value: []const u8, comptime setter: anytype) !void {
    const z = try allocator.dupeZ(u8, value);
    defer allocator.free(z);
    setter(guild, z);
}

fn guildTradeFlagged(guild: *cdb.guild_data, pos: c_int) bool {
    return cdb.guild_trade_flagged(guild, pos);
}

fn guildTradeFlagSet(guild: *cdb.guild_data, pos: c_int, value: bool) void {
    cdb.guild_trade_flag_set(guild, pos, value);
}
