const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");
const bitflags = @import("flags.zig");
const extradesc_json = @import("extradesc_json.zig");
const dgscripts_json = @import("dgscripts_json.zig");

pub const JsonValue = jsonx.JsonValue;

pub const ObjectJsonMode = enum { prototype, instance };
pub const DeserializeOptions = struct {
    c_allocator: std.mem.Allocator = std.heap.c_allocator,
    mode: ObjectJsonMode = .instance,
    preserve_id: bool = true,
};

pub fn serializeObject(allocator: std.mem.Allocator, obj: *cdb.obj_data, mode: ObjectJsonMode) !JsonValue {
    var object = jsonx.newObject(allocator);

    try jsonx.putSlice(&object, allocator, "kind", if (mode == .prototype) "object_prototype" else "object");

    if (mode == .instance) {
        try jsonx.putInt(&object, allocator, "id", cdb.obj_id_get(obj));
    }

    try jsonx.putInt(&object, allocator, "proto_id", cdb.obj_proto_id_get(obj));
    try jsonx.putInt(&object, allocator, "type", cdb.obj_type_get(obj));
    try jsonx.putInt(&object, allocator, "level", cdb.obj_level_get(obj));
    try jsonx.putInt(&object, allocator, "weight", cdb.obj_weight_get(obj));
    try jsonx.putInt(&object, allocator, "cost", cdb.obj_cost_get(obj));
    try jsonx.putInt(&object, allocator, "timer", cdb.obj_timer_get(obj));
    try jsonx.putInt(&object, allocator, "size", cdb.obj_size_get(obj));
    try jsonx.putString(&object, allocator, "name", cdb.obj_name_get(obj));
    try jsonx.putString(&object, allocator, "description", cdb.obj_description_get(obj));
    try jsonx.putString(&object, allocator, "short_description", cdb.obj_short_description_get(obj));
    try jsonx.putString(&object, allocator, "action_description", cdb.obj_action_description_get(obj));
    try jsonx.putNonEmpty(&object, allocator, "extra_descriptions", try extradesc_json.serializeExtraDescriptions(allocator, obj.ex_description));
    if (mode == .prototype) {
        try jsonx.putNonEmpty(&object, allocator, "proto_script", try dgscripts_json.serializeProtoScript(allocator, obj.proto_script));
    }
    try jsonx.put(&object, allocator, "values", try serializeValues(allocator, obj));
    try jsonx.putNonEmpty(&object, allocator, "wear_flags", try jsonx.serializeFlags(allocator, obj, 128, wearFlagged));
    try jsonx.putNonEmpty(&object, allocator, "extra_flags", try jsonx.serializeFlags(allocator, obj, 128, extraFlagged));
    try jsonx.putNonEmpty(&object, allocator, "affect_flags", try jsonx.serializeFlags(allocator, obj, 128, affFlagged));

    return object;
}

pub fn serializeObjectPrototype(allocator: std.mem.Allocator, obj: *cdb.obj_proto_data) !JsonValue {
    var object = jsonx.newObject(allocator);

    try jsonx.putSlice(&object, allocator, "kind", "object_prototype");
    try jsonx.putInt(&object, allocator, "proto_id", obj.vnum);
    try jsonx.putInt(&object, allocator, "type", obj.type_flag);
    try jsonx.putInt(&object, allocator, "level", obj.level);
    try jsonx.putInt(&object, allocator, "weight", obj.weight);
    try jsonx.putInt(&object, allocator, "cost", obj.cost);
    try jsonx.putInt(&object, allocator, "timer", obj.timer);
    try jsonx.putInt(&object, allocator, "size", obj.size);
    try jsonx.putString(&object, allocator, "name", obj.name);
    try jsonx.putString(&object, allocator, "description", obj.description);
    try jsonx.putString(&object, allocator, "short_description", obj.short_description);
    try jsonx.putString(&object, allocator, "action_description", obj.action_description);
    try jsonx.putNonEmpty(&object, allocator, "extra_descriptions", try extradesc_json.serializeExtraDescriptions(allocator, obj.ex_description));
    try jsonx.putNonEmpty(&object, allocator, "proto_script", try dgscripts_json.serializeProtoScript(allocator, obj.proto_script));
    try jsonx.put(&object, allocator, "values", try serializeProtoValues(allocator, obj));
    try jsonx.putNonEmpty(&object, allocator, "wear_flags", try jsonx.serializeFlags(allocator, obj, 128, protoWearFlagged));
    try jsonx.putNonEmpty(&object, allocator, "extra_flags", try jsonx.serializeFlags(allocator, obj, 128, protoExtraFlagged));
    try jsonx.putNonEmpty(&object, allocator, "affect_flags", try jsonx.serializeFlags(allocator, obj, 128, protoAffFlagged));

    return object;
}

pub fn deserializeObject(obj: *cdb.obj_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;

    if (options.mode == .instance and options.preserve_id) {
        if (try jsonx.intField(value, "id", i64)) |v| cdb.obj_id_set(obj, v);
    }

    if (try jsonx.intField(value, "proto_id", cdb.obj_vnum)) |v| cdb.obj_proto_id_set(obj, v);
    if (try jsonx.intField(value, "type", i8)) |v| cdb.obj_type_set(obj, v);
    if (try jsonx.intField(value, "level", c_int)) |v| cdb.obj_level_set(obj, v);
    if (try jsonx.intField(value, "weight", i64)) |v| cdb.obj_weight_set(obj, v);
    if (try jsonx.intField(value, "cost", c_int)) |v| cdb.obj_cost_set(obj, v);
    if (try jsonx.intField(value, "timer", c_int)) |v| cdb.obj_timer_set(obj, v);
    if (try jsonx.intField(value, "size", c_int)) |v| cdb.obj_size_set(obj, v);
    try setStringField(options.c_allocator, obj, value, "name", cdb.obj_name_set);
    try setStringField(options.c_allocator, obj, value, "description", cdb.obj_description_set);
    try setStringField(options.c_allocator, obj, value, "short_description", cdb.obj_short_description_set);
    try setStringField(options.c_allocator, obj, value, "action_description", cdb.obj_action_description_set);
    if (jsonx.field(value, "extra_descriptions")) |items| try extradesc_json.deserializeExtraDescriptions(&obj.ex_description, items);

    if (options.mode == .prototype) {
        if (jsonx.field(value, "proto_script")) |items| try dgscripts_json.deserializeProtoScript(&obj.proto_script, items);
    }

    if (jsonx.field(value, "values")) |values| try deserializeValues(obj, values);
    if (jsonx.field(value, "wear_flags")) |flags| try jsonx.deserializeFlags(obj, flags, 128, wearFlagSet);
    if (jsonx.field(value, "extra_flags")) |flags| try jsonx.deserializeFlags(obj, flags, 128, extraFlagSet);
    if (jsonx.field(value, "affect_flags")) |flags| try jsonx.deserializeFlags(obj, flags, 128, affFlagSet);
}

pub fn deserializeObjectPrototype(obj: *cdb.obj_proto_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;

    if (try jsonx.intField(value, "proto_id", cdb.obj_vnum)) |v| obj.vnum = v;
    if (try jsonx.intField(value, "type", i8)) |v| obj.type_flag = v;
    if (try jsonx.intField(value, "level", c_int)) |v| obj.level = v;
    if (try jsonx.intField(value, "weight", i64)) |v| obj.weight = v;
    if (try jsonx.intField(value, "cost", c_int)) |v| obj.cost = v;
    if (try jsonx.intField(value, "timer", c_int)) |v| obj.timer = v;
    if (try jsonx.intField(value, "size", c_int)) |v| obj.size = v;
    try setProtoStringField(options.c_allocator, obj, value, "name", &obj.name);
    try setProtoStringField(options.c_allocator, obj, value, "description", &obj.description);
    try setProtoStringField(options.c_allocator, obj, value, "short_description", &obj.short_description);
    try setProtoStringField(options.c_allocator, obj, value, "action_description", &obj.action_description);
    if (jsonx.field(value, "extra_descriptions")) |items| try extradesc_json.deserializeExtraDescriptions(&obj.ex_description, items);
    if (jsonx.field(value, "proto_script")) |items| try dgscripts_json.deserializeProtoScript(&obj.proto_script, items);
    if (jsonx.field(value, "values")) |values| try deserializeProtoValues(obj, values);
    if (jsonx.field(value, "wear_flags")) |flags| try jsonx.deserializeFlags(obj, flags, 128, protoWearFlagSet);
    if (jsonx.field(value, "extra_flags")) |flags| try jsonx.deserializeFlags(obj, flags, 128, protoExtraFlagSet);
    if (jsonx.field(value, "affect_flags")) |flags| try jsonx.deserializeFlags(obj, flags, 128, protoAffFlagSet);
}

fn serializeValues(allocator: std.mem.Allocator, obj: *cdb.obj_data) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    for (0..cdb.NUM_OBJ_VAL_POSITIONS) |pos| try array.append(.{ .integer = cdb.obj_value_get(obj, pos) });
    return .{ .array = array };
}

fn deserializeValues(obj: *cdb.obj_data, value: JsonValue) !void {
    if (value != .array) return error.ExpectedArray;
    for (value.array.items, 0..) |item, pos| {
        if (pos >= cdb.NUM_OBJ_VAL_POSITIONS) break;
        if (item != .integer) return error.ExpectedInteger;
        cdb.obj_value_set(obj, pos, std.math.cast(c_int, item.integer) orelse return error.IntegerOutOfRange);
    }
}

fn serializeProtoValues(allocator: std.mem.Allocator, obj: *cdb.obj_proto_data) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    for (0..cdb.NUM_OBJ_VAL_POSITIONS) |pos| try array.append(.{ .integer = @intCast(obj.value[pos]) });
    return .{ .array = array };
}

fn deserializeProtoValues(obj: *cdb.obj_proto_data, value: JsonValue) !void {
    if (value != .array) return error.ExpectedArray;
    for (value.array.items, 0..) |item, pos| {
        if (pos >= cdb.NUM_OBJ_VAL_POSITIONS) break;
        if (item != .integer) return error.ExpectedInteger;
        obj.value[pos] = std.math.cast(c_int, item.integer) orelse return error.IntegerOutOfRange;
    }
}

fn setString(allocator: std.mem.Allocator, obj: *cdb.obj_data, value: []const u8, comptime setter: anytype) !void {
    const z = try allocator.dupeZ(u8, value);
    defer allocator.free(z);
    setter(obj, z);
}

fn setStringField(allocator: std.mem.Allocator, obj: *cdb.obj_data, object: JsonValue, key: []const u8, comptime setter: anytype) !void {
    const value = try jsonx.stringFieldAlloc(allocator, object, key) orelse return;
    defer allocator.free(value);
    try setString(allocator, obj, value, setter);
}

fn setProtoStringField(allocator: std.mem.Allocator, obj: *cdb.obj_proto_data, object: JsonValue, key: []const u8, field: *[*c]u8) !void {
    _ = obj;
    const value = try jsonx.stringFieldAlloc(allocator, object, key) orelse return;
    defer allocator.free(value);
    const z = try allocator.dupeZ(u8, value);
    if (field.* != null) std.c.free(field.*);
    field.* = z.ptr;
}

fn wearFlagged(obj: *cdb.obj_data, pos: c_int) bool {
    return cdb.obj_wear_flagged(obj, pos);
}
fn wearFlagSet(obj: *cdb.obj_data, pos: c_int, value: bool) void {
    cdb.obj_wear_flag_set(obj, pos, value);
}
fn extraFlagged(obj: *cdb.obj_data, pos: c_int) bool {
    return cdb.obj_extra_flagged(obj, pos);
}
fn extraFlagSet(obj: *cdb.obj_data, pos: c_int, value: bool) void {
    cdb.obj_extra_flag_set(obj, pos, value);
}
fn affFlagged(obj: *cdb.obj_data, pos: c_int) bool {
    return cdb.obj_aff_flagged(obj, pos);
}
fn affFlagSet(obj: *cdb.obj_data, pos: c_int, value: bool) void {
    cdb.obj_aff_flag_set(obj, pos, value);
}

fn protoWearFlagged(obj: *cdb.obj_proto_data, pos: c_int) bool {
    return bitflags.get(obj.wear_flags[0..], pos);
}
fn protoWearFlagSet(obj: *cdb.obj_proto_data, pos: c_int, value: bool) void {
    bitflags.set(obj.wear_flags[0..], pos, value);
}
fn protoExtraFlagged(obj: *cdb.obj_proto_data, pos: c_int) bool {
    return bitflags.get(obj.extra_flags[0..], pos);
}
fn protoExtraFlagSet(obj: *cdb.obj_proto_data, pos: c_int, value: bool) void {
    bitflags.set(obj.extra_flags[0..], pos, value);
}
fn protoAffFlagged(obj: *cdb.obj_proto_data, pos: c_int) bool {
    return bitflags.get(obj.bitvector[0..], pos);
}
fn protoAffFlagSet(obj: *cdb.obj_proto_data, pos: c_int, value: bool) void {
    bitflags.set(obj.bitvector[0..], pos, value);
}
