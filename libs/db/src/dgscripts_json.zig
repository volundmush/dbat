const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");

pub const JsonValue = jsonx.JsonValue;

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;

pub fn serializeProtoScript(allocator: std.mem.Allocator, head: [*c]cdb.trig_proto_list) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    var current = head;
    while (current != null) : (current = current.*.next) {
        try array.append(.{ .integer = current.*.vnum });
    }
    return .{ .array = array };
}

pub fn deserializeProtoScript(target: *[*c]cdb.trig_proto_list, value: JsonValue) !void {
    if (value != .array) return error.ExpectedArray;

    freeProtoScript(target.*);
    target.* = null;

    var tail: ?*cdb.trig_proto_list = null;
    for (value.array.items) |item| {
        if (item != .integer) return error.ExpectedInteger;
        const node = try newProtoScriptNode(std.math.cast(c_int, item.integer) orelse return error.IntegerOutOfRange);
        if (tail) |last| {
            last.next = node;
        } else {
            target.* = node;
        }
        tail = node;
    }
}

fn newProtoScriptNode(vnum: c_int) !*cdb.trig_proto_list {
    const node: *cdb.trig_proto_list = @ptrCast(@alignCast(calloc(1, @sizeOf(cdb.trig_proto_list)) orelse return error.OutOfMemory));
    node.vnum = vnum;
    node.next = null;
    return node;
}

fn freeProtoScript(head: [*c]cdb.trig_proto_list) void {
    var current = head;
    while (current != null) {
        const next = current.*.next;
        std.c.free(current);
        current = next;
    }
}
