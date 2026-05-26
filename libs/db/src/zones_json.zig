const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");

pub const JsonValue = jsonx.JsonValue;
pub const DeserializeOptions = struct { c_allocator: std.mem.Allocator = std.heap.c_allocator };

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;

pub fn serializeZone(allocator: std.mem.Allocator, zone: *cdb.zone_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putSlice(&object, allocator, "kind", "zone");
    try jsonx.putInt(&object, allocator, "id", cdb.zone_id_get(zone));
    try jsonx.putString(&object, allocator, "name", cdb.zone_name_get(zone));
    try jsonx.putString(&object, allocator, "builders", cdb.zone_builders_get(zone));
    try jsonx.putInt(&object, allocator, "lifespan", cdb.zone_lifespan_get(zone));
    try jsonx.putInt(&object, allocator, "bottom", cdb.zone_bottom_get(zone));
    try jsonx.putInt(&object, allocator, "top", cdb.zone_top_get(zone));
    try jsonx.putInt(&object, allocator, "reset_mode", cdb.zone_reset_mode_get(zone));
    try jsonx.putInt(&object, allocator, "min_level", cdb.zone_min_level_get(zone));
    try jsonx.putInt(&object, allocator, "max_level", cdb.zone_max_level_get(zone));
    try jsonx.put(&object, allocator, "flags", try jsonx.serializeFlags(allocator, zone, 128, zoneFlagged));
    try jsonx.put(&object, allocator, "reset_commands", try serializeResetCommands(allocator, zone));
    return object;
}

pub fn deserializeZone(zone: *cdb.zone_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    if (try jsonx.intField(value, "id", cdb.zone_vnum)) |v| cdb.zone_id_set(zone, v);
    if (try jsonx.stringField(value, "name")) |v| try setString(options.c_allocator, zone, v, cdb.zone_name_set);
    if (try jsonx.stringField(value, "builders")) |v| try setString(options.c_allocator, zone, v, cdb.zone_builders_set);
    if (try jsonx.intField(value, "lifespan", c_int)) |v| cdb.zone_lifespan_set(zone, v);
    if (try jsonx.intField(value, "bottom", cdb.room_vnum)) |v| cdb.zone_bottom_set(zone, v);
    if (try jsonx.intField(value, "top", cdb.room_vnum)) |v| cdb.zone_top_set(zone, v);
    if (try jsonx.intField(value, "reset_mode", c_int)) |v| cdb.zone_reset_mode_set(zone, v);
    if (try jsonx.intField(value, "min_level", c_int)) |v| cdb.zone_min_level_set(zone, v);
    if (try jsonx.intField(value, "max_level", c_int)) |v| cdb.zone_max_level_set(zone, v);
    if (jsonx.field(value, "flags")) |flags| try jsonx.deserializeFlags(zone, flags, 128, zoneFlagSet);
    if (jsonx.field(value, "reset_commands")) |commands| try deserializeResetCommands(zone, options, commands);
}

pub fn serializeResetCommands(allocator: std.mem.Allocator, zone: *cdb.zone_data) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    if (zone.cmd == null) return .{ .array = array };

    var index: usize = 0;
    while (true) : (index += 1) {
        const command = cdb.zone_command_get(zone, index);
        if (command == null or cdb.zone_command_type_get(command) == 'S') break;
        try array.append(try serializeResetCommand(allocator, command));
    }
    return .{ .array = array };
}

pub fn deserializeResetCommands(zone: *cdb.zone_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .array) return error.ExpectedArray;

    freeResetCommands(zone.cmd);
    zone.cmd = @ptrCast(@alignCast(calloc(value.array.items.len + 1, @sizeOf(cdb.reset_com)) orelse return error.OutOfMemory));

    for (value.array.items, 0..) |item, index| {
        try deserializeResetCommand(&zone.cmd[index], options, item);
    }
    zone.cmd[value.array.items.len].command = 'S';
}

pub fn serializeResetCommand(allocator: std.mem.Allocator, command: *cdb.reset_com) !JsonValue {
    var object = jsonx.newObject(allocator);
    const command_type = [_]u8{cdb.zone_command_type_get(command)};
    try jsonx.putSlice(&object, allocator, "command", &command_type);
    try jsonx.putBool(&object, allocator, "if_flag", cdb.zone_command_if_flag_get(command));
    try jsonx.put(&object, allocator, "args", try serializeResetCommandArgs(allocator, command));
    try jsonx.putInt(&object, allocator, "line", cdb.zone_command_line_get(command));
    try jsonx.putString(&object, allocator, "sarg1", cdb.zone_command_sarg1_get(command));
    try jsonx.putString(&object, allocator, "sarg2", cdb.zone_command_sarg2_get(command));
    return object;
}

pub fn deserializeResetCommand(command: *cdb.reset_com, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;

    if (try jsonx.stringField(value, "command")) |v| {
        if (v.len == 0) return error.ExpectedString;
        cdb.zone_command_type_set(command, v[0]);
    }
    if (try jsonx.boolField(value, "if_flag")) |v| cdb.zone_command_if_flag_set(command, v);
    if (jsonx.field(value, "args")) |args| try deserializeResetCommandArgs(command, args);
    if (try jsonx.intField(value, "line", c_int)) |v| cdb.zone_command_line_set(command, v);
    if (try jsonx.stringField(value, "sarg1")) |v| try setCommandString(options.c_allocator, command, v, cdb.zone_command_sarg1_set);
    if (try jsonx.stringField(value, "sarg2")) |v| try setCommandString(options.c_allocator, command, v, cdb.zone_command_sarg2_set);
}

fn serializeResetCommandArgs(allocator: std.mem.Allocator, command: *cdb.reset_com) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    for (0..5) |index| try array.append(.{ .integer = cdb.zone_command_arg_get(command, index) });
    return .{ .array = array };
}

fn deserializeResetCommandArgs(command: *cdb.reset_com, value: JsonValue) !void {
    if (value != .array) return error.ExpectedArray;
    for (value.array.items, 0..) |item, index| {
        if (index >= 5) break;
        if (item != .integer) return error.ExpectedInteger;
        cdb.zone_command_arg_set(command, index, std.math.cast(c_int, item.integer) orelse return error.IntegerOutOfRange);
    }
}

fn freeResetCommands(commands: [*c]cdb.reset_com) void {
    if (commands == null) return;
    var index: usize = 0;
    while (commands[index].command != 'S') : (index += 1) {
        if (commands[index].sarg1 != null) std.c.free(commands[index].sarg1);
        if (commands[index].sarg2 != null) std.c.free(commands[index].sarg2);
    }
    std.c.free(commands);
}

fn setString(allocator: std.mem.Allocator, zone: *cdb.zone_data, value: []const u8, comptime setter: anytype) !void {
    const z = try allocator.dupeZ(u8, value);
    defer allocator.free(z);
    setter(zone, z);
}

fn setCommandString(allocator: std.mem.Allocator, command: *cdb.reset_com, value: []const u8, comptime setter: anytype) !void {
    const z = try allocator.dupeZ(u8, value);
    defer allocator.free(z);
    setter(command, z);
}

fn zoneFlagged(zone: *cdb.zone_data, pos: c_int) bool {
    return cdb.zone_flagged(zone, pos);
}
fn zoneFlagSet(zone: *cdb.zone_data, pos: c_int, value: bool) void {
    cdb.zone_flag_set(zone, pos, value);
}
