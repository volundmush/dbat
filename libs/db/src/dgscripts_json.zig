const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");

pub const JsonValue = jsonx.JsonValue;

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;
extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

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

pub fn serializeTrigger(allocator: std.mem.Allocator, trigger: *cdb.trig_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putSlice(&object, allocator, "kind", "trigger");
    try jsonx.putInt(&object, allocator, "id", triggerVnum(trigger));
    try jsonx.putString(&object, allocator, "name", trigger.name);
    try jsonx.putInt(&object, allocator, "attach_type", trigger.attach_type);
    try jsonx.putInt(&object, allocator, "trigger_type", trigger.trigger_type);
    try jsonx.putInt(&object, allocator, "narg", trigger.narg);
    try jsonx.putString(&object, allocator, "arglist", trigger.arglist);
    try jsonx.put(&object, allocator, "lines", try serializeCommandLines(allocator, trigger.cmdlist));
    return object;
}

pub fn deserializeTrigger(trigger: *cdb.trig_data, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    if (try jsonx.stringField(value, "name")) |v| trigger.name = try duplicate(v);
    if (try jsonx.intField(value, "attach_type", u8)) |v| trigger.attach_type = v;
    if (try jsonx.intField(value, "trigger_type", cdb.bitvector_t)) |v| trigger.trigger_type = v;
    if (try jsonx.intField(value, "narg", c_int)) |v| trigger.narg = v;
    if (try jsonx.stringField(value, "arglist")) |v| trigger.arglist = try duplicate(v);
    if (jsonx.field(value, "lines")) |lines| try deserializeCommandLines(&trigger.cmdlist, lines);
    trigger.curr_state = trigger.cmdlist;
}

fn triggerVnum(trigger: *cdb.trig_data) cdb.mob_vnum {
    if (trigger.nr < 0 or cdb.trig_index == null) return cdb.NOBODY;
    return cdb.trig_index[@intCast(trigger.nr)].*.vnum;
}

fn serializeCommandLines(allocator: std.mem.Allocator, head: [*c]cdb.cmdlist_element) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    var current = head;
    while (current != null) : (current = current.*.next) {
        if (current.*.cmd == null) {
            try array.append(.null);
        } else {
            try array.append(.{ .string = try allocator.dupe(u8, std.mem.span(current.*.cmd)) });
        }
    }
    return .{ .array = array };
}

fn deserializeCommandLines(target: *[*c]cdb.cmdlist_element, value: JsonValue) !void {
    if (value != .array) return error.ExpectedArray;
    freeCommandLines(target.*);
    target.* = null;

    var tail: ?*cdb.cmdlist_element = null;
    for (value.array.items) |item| {
        const text = switch (item) {
            .string => |s| s,
            .null => "",
            else => return error.ExpectedString,
        };
        const node = try newCommandLine(text);
        if (tail) |last| {
            last.next = node;
        } else {
            target.* = node;
        }
        tail = node;
    }
}

fn newCommandLine(text: []const u8) !*cdb.cmdlist_element {
    const node: *cdb.cmdlist_element = @ptrCast(@alignCast(calloc(1, @sizeOf(cdb.cmdlist_element)) orelse return error.OutOfMemory));
    node.cmd = try duplicate(text);
    node.original = null;
    node.next = null;
    return node;
}

fn freeCommandLines(head: [*c]cdb.cmdlist_element) void {
    var current = head;
    while (current != null) {
        const next = current.*.next;
        if (current.*.cmd != null) std.c.free(current.*.cmd);
        std.c.free(current);
        current = next;
    }
}

fn duplicate(value: []const u8) ![*:0]u8 {
    const z = try std.heap.c_allocator.dupeZ(u8, value);
    defer std.heap.c_allocator.free(z);
    return strdup(z) orelse error.OutOfMemory;
}
