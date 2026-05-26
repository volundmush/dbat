const std = @import("std");

pub const JsonValue = std.json.Value;
pub const JsonObject = std.json.ObjectMap;
pub const JsonArray = std.json.Array;

pub const JsonError = error{
    ExpectedObject,
    ExpectedArray,
    ExpectedString,
    ExpectedInteger,
    ExpectedBool,
    IntegerOutOfRange,
} || std.mem.Allocator.Error;

pub fn newObject(allocator: std.mem.Allocator) JsonValue {
    return .{ .object = JsonObject.init(allocator) };
}

pub fn newArray(allocator: std.mem.Allocator) JsonValue {
    return .{ .array = JsonArray.init(allocator) };
}

pub fn put(object: *JsonValue, allocator: std.mem.Allocator, key: []const u8, value: JsonValue) !void {
    try object.object.put(allocator, key, value);
}

pub fn putString(object: *JsonValue, allocator: std.mem.Allocator, key: []const u8, value: [*c]const u8) !void {
    if (value == null) {
        try put(object, allocator, key, .null);
        return;
    }
    try put(object, allocator, key, .{ .string = try allocator.dupe(u8, std.mem.span(value)) });
}

pub fn putSlice(object: *JsonValue, allocator: std.mem.Allocator, key: []const u8, value: []const u8) !void {
    try put(object, allocator, key, .{ .string = try allocator.dupe(u8, value) });
}

pub fn putInt(object: *JsonValue, allocator: std.mem.Allocator, key: []const u8, value: anytype) !void {
    try put(object, allocator, key, .{ .integer = @intCast(value) });
}

pub fn putFloat(object: *JsonValue, allocator: std.mem.Allocator, key: []const u8, value: anytype) !void {
    try put(object, allocator, key, .{ .float = @floatCast(value) });
}

pub fn putBool(object: *JsonValue, allocator: std.mem.Allocator, key: []const u8, value: bool) !void {
    try put(object, allocator, key, .{ .bool = value });
}

pub fn getObject(value: JsonValue) JsonError!*const JsonObject {
    return switch (value) {
        .object => |*object| object,
        else => error.ExpectedObject,
    };
}

pub fn field(value: JsonValue, key: []const u8) ?JsonValue {
    if (value != .object) return null;
    return value.object.get(key);
}

pub fn stringField(value: JsonValue, key: []const u8) JsonError!?[]const u8 {
    const item = field(value, key) orelse return null;
    return switch (item) {
        .string => |s| s,
        .null => null,
        else => error.ExpectedString,
    };
}

pub fn intField(value: JsonValue, key: []const u8, comptime T: type) JsonError!?T {
    const item = field(value, key) orelse return null;
    const raw = switch (item) {
        .integer => |i| i,
        else => return error.ExpectedInteger,
    };
    return std.math.cast(T, raw) orelse error.IntegerOutOfRange;
}

pub fn floatField(value: JsonValue, key: []const u8, comptime T: type) JsonError!?T {
    const item = field(value, key) orelse return null;
    return switch (item) {
        .float => |f| @as(T, @floatCast(f)),
        .integer => |i| @as(T, @floatFromInt(i)),
        else => error.ExpectedInteger,
    };
}

pub fn boolField(value: JsonValue, key: []const u8) JsonError!?bool {
    const item = field(value, key) orelse return null;
    return switch (item) {
        .bool => |b| b,
        else => error.ExpectedBool,
    };
}

pub fn serializeFlags(allocator: std.mem.Allocator, ctx: anytype, max_flags: usize, comptime flagged: anytype) !JsonValue {
    var array = JsonArray.init(allocator);
    for (0..max_flags) |pos| {
        if (flagged(ctx, @intCast(pos))) try array.append(.{ .integer = @intCast(pos) });
    }
    return .{ .array = array };
}

pub fn deserializeFlags(ctx: anytype, value: JsonValue, max_flags: usize, comptime setFlag: anytype) JsonError!void {
    if (value != .array) return error.ExpectedArray;
    for (0..max_flags) |pos| setFlag(ctx, @intCast(pos), false);
    for (value.array.items) |item| {
        if (item != .integer) return error.ExpectedInteger;
        const pos = std.math.cast(usize, item.integer) orelse return error.IntegerOutOfRange;
        if (pos >= max_flags) return error.IntegerOutOfRange;
        setFlag(ctx, @intCast(pos), true);
    }
}
