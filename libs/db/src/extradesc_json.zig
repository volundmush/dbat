const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");

pub const JsonValue = jsonx.JsonValue;

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;
extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub fn serializeExtraDescriptions(allocator: std.mem.Allocator, head: [*c]cdb.extra_descr_data) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    var current = head;
    while (current != null) : (current = current.*.next) {
        var object = jsonx.newObject(allocator);
        try jsonx.putString(&object, allocator, "keyword", current.*.keyword);
        try jsonx.putString(&object, allocator, "description", current.*.description);
        try array.append(object);
    }
    return .{ .array = array };
}

pub fn deserializeExtraDescriptions(target: *[*c]cdb.extra_descr_data, value: JsonValue) !void {
    if (value != .array) return error.ExpectedArray;

    freeExtraDescriptions(target.*);
    target.* = null;

    var tail: ?*cdb.extra_descr_data = null;
    for (value.array.items) |item| {
        if (item != .object) return error.ExpectedObject;
        const keyword = try jsonx.stringFieldAlloc(std.heap.c_allocator, item, "keyword") orelse try std.heap.c_allocator.dupe(u8, "");
        defer std.heap.c_allocator.free(keyword);
        const description = try jsonx.stringFieldAlloc(std.heap.c_allocator, item, "description") orelse try std.heap.c_allocator.dupe(u8, "");
        defer std.heap.c_allocator.free(description);
        const node = try newExtraDescription(keyword, description);
        if (tail) |last| {
            last.next = node;
        } else {
            target.* = node;
        }
        tail = node;
    }
}

fn newExtraDescription(keyword: []const u8, description: []const u8) !*cdb.extra_descr_data {
    const node: *cdb.extra_descr_data = @ptrCast(@alignCast(calloc(1, @sizeOf(cdb.extra_descr_data)) orelse return error.OutOfMemory));
    node.keyword = try duplicate(keyword);
    node.description = try duplicate(description);
    node.next = null;
    return node;
}

fn duplicate(value: []const u8) ![*:0]u8 {
    const z = try std.heap.c_allocator.dupeZ(u8, value);
    defer std.heap.c_allocator.free(z);
    return strdup(z) orelse error.OutOfMemory;
}

fn freeExtraDescriptions(head: [*c]cdb.extra_descr_data) void {
    var current = head;
    while (current != null) {
        const next = current.*.next;
        if (current.*.keyword != null) std.c.free(current.*.keyword);
        if (current.*.description != null) std.c.free(current.*.description);
        std.c.free(current);
        current = next;
    }
}
