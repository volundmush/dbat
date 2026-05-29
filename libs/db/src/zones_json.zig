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
    try jsonx.putNonEmpty(&object, allocator, "flags", try jsonx.serializeFlags(allocator, zone, 128, zoneFlagged));
    try jsonx.putNonEmpty(&object, allocator, "reset_commands", try serializeResetCommands(allocator, zone));
    return object;
}

pub fn deserializeZone(zone: *cdb.zone_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    if (try jsonx.intField(value, "id", cdb.zone_vnum)) |v| cdb.zone_id_set(zone, v);
    try setStringField(options.c_allocator, zone, value, "name", cdb.zone_name_set);
    try setStringField(options.c_allocator, zone, value, "builders", cdb.zone_builders_set);
    if (try jsonx.intField(value, "lifespan", c_int)) |v| cdb.zone_lifespan_set(zone, v);
    if (try jsonx.intField(value, "bottom", cdb.room_vnum)) |v| cdb.zone_bottom_set(zone, v);
    if (try jsonx.intField(value, "top", cdb.room_vnum)) |v| cdb.zone_top_set(zone, v);
    if (try jsonx.intField(value, "reset_mode", c_int)) |v| cdb.zone_reset_mode_set(zone, v);
    if (try jsonx.intField(value, "min_level", c_int)) |v| cdb.zone_min_level_set(zone, v);
    if (try jsonx.intField(value, "max_level", c_int)) |v| cdb.zone_max_level_set(zone, v);
    if (jsonx.field(value, "flags")) |flags| try jsonx.deserializeFlags(zone, flags, 128, zoneFlagSet);
    if (jsonx.field(value, "reset_commands")) |commands| {
        try deserializeResetCommands(zone, options, commands);
    } else {
        try setEmptyResetCommands(zone);
    }
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
        const command: *cdb.reset_com = @ptrCast(&zone.cmd[index]);
        try deserializeResetCommand(command, options, item);
        cdb.zone_command_line_set(command, @intCast(index + 1));
    }
    zone.cmd[value.array.items.len].command = 'S';
}

fn setEmptyResetCommands(zone: *cdb.zone_data) !void {
    freeResetCommands(zone.cmd);
    zone.cmd = @ptrCast(@alignCast(calloc(1, @sizeOf(cdb.reset_com)) orelse return error.OutOfMemory));
    zone.cmd[0].command = 'S';
}

pub fn serializeResetCommand(allocator: std.mem.Allocator, command: *cdb.reset_com) !JsonValue {
    var object = jsonx.newObject(allocator);
    const command_type = [_]u8{cdb.zone_command_type_get(command)};
    try jsonx.putSlice(&object, allocator, "command", &command_type);
    try jsonx.putBool(&object, allocator, "if_flag", cdb.zone_command_if_flag_get(command));
    try jsonx.put(&object, allocator, "args", try serializeResetCommandArgs(allocator, command));
    try jsonx.putString(&object, allocator, "sarg1", cdb.zone_command_sarg1_get(command));
    try jsonx.putString(&object, allocator, "sarg2", cdb.zone_command_sarg2_get(command));
    return object;
}

pub fn deserializeResetCommand(command: *cdb.reset_com, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;

    if (try jsonx.stringFieldAlloc(options.c_allocator, value, "command")) |v| {
        defer options.c_allocator.free(v);
        if (v.len == 0) return error.ExpectedString;
        cdb.zone_command_type_set(command, v[0]);
    }
    if (try jsonx.boolField(value, "if_flag")) |v| cdb.zone_command_if_flag_set(command, v);
    if (jsonx.field(value, "args")) |args| try deserializeResetCommandArgs(command, args);
    try setCommandStringField(options.c_allocator, command, value, "sarg1", cdb.zone_command_sarg1_set);
    try setCommandStringField(options.c_allocator, command, value, "sarg2", cdb.zone_command_sarg2_set);
}

fn serializeResetCommandArgs(allocator: std.mem.Allocator, command: *cdb.reset_com) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    var args = [_]c_int{
        cdb.zone_command_arg_get(command, 0),
        cdb.zone_command_arg_get(command, 1),
        cdb.zone_command_arg_get(command, 2),
        cdb.zone_command_arg_get(command, 3),
        cdb.zone_command_arg_get(command, 4),
    };
    denumberResetCommandArgs(cdb.zone_command_type_get(command), &args);
    for (args) |arg| try array.append(.{ .integer = arg });
    return .{ .array = array };
}

fn denumberResetCommandArgs(command: u8, args: *[5]c_int) void {
    switch (command) {
        'M' => {
            args[0] = args[0];
            args[2] = args[2];
        },
        'O' => {
            args[0] = args[0];
            if (args[2] != cdb.NOWHERE) args[2] = args[2];
        },
        'G', 'E' => {
            args[0] = args[0];
        },
        'P' => {
            args[0] = args[0];
            args[2] = args[2];
        },
        'D' => {
            args[0] = args[0];
        },
        'R' => {
            args[0] = args[0];
            args[1] = args[1];
        },
        'T' => {
            args[1] = trigVnumFromRnum(args[1]);
            args[2] = args[2];
        },
        'V' => {
            args[2] = args[2];
        },
        else => {},
    }
}

fn trigVnumFromRnum(rnum: c_int) c_int {
    if (rnum == cdb.NOTHING or rnum < 0 or rnum >= cdb.top_of_trigt or cdb.trig_index == null) return rnum;
    const trigger = cdb.trig_index[@intCast(rnum)];
    if (trigger == null) return rnum;
    return trigger.*.vnum;
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

fn setStringField(allocator: std.mem.Allocator, zone: *cdb.zone_data, object: JsonValue, key: []const u8, comptime setter: anytype) !void {
    const value = try jsonx.stringFieldAlloc(allocator, object, key) orelse return;
    defer allocator.free(value);
    try setString(allocator, zone, value, setter);
}

fn setCommandString(allocator: std.mem.Allocator, command: *cdb.reset_com, value: []const u8, comptime setter: anytype) !void {
    const z = try allocator.dupeZ(u8, value);
    defer allocator.free(z);
    setter(command, z);
}

fn setCommandStringField(allocator: std.mem.Allocator, command: *cdb.reset_com, object: JsonValue, key: []const u8, comptime setter: anytype) !void {
    const value = try jsonx.stringFieldAlloc(allocator, object, key) orelse return;
    defer allocator.free(value);
    try setCommandString(allocator, command, value, setter);
}

fn zoneFlagged(zone: *cdb.zone_data, pos: c_int) bool {
    return cdb.zone_flagged(zone, pos);
}
fn zoneFlagSet(zone: *cdb.zone_data, pos: c_int, value: bool) void {
    cdb.zone_flag_set(zone, pos, value);
}
