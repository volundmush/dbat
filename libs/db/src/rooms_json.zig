const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");
const extradesc_json = @import("extradesc_json.zig");
const dgscripts_json = @import("dgscripts_json.zig");
const exits_json = @import("exits_json.zig");

pub const JsonValue = jsonx.JsonValue;
pub const DeserializeOptions = struct {
    c_allocator: std.mem.Allocator = std.heap.c_allocator,
};

pub fn serializeRoom(allocator: std.mem.Allocator, room: *cdb.room_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putSlice(&object, allocator, "kind", "room");
    try jsonx.putInt(&object, allocator, "id", cdb.room_id_get(room));
    try jsonx.putInt(&object, allocator, "sector", cdb.room_sector_type_get(room));
    try jsonx.putString(&object, allocator, "name", cdb.room_name_get(room));
    try jsonx.putString(&object, allocator, "description", cdb.room_description_get(room));
    try jsonx.putNonEmpty(&object, allocator, "extra_descriptions", try extradesc_json.serializeExtraDescriptions(allocator, room.ex_description));
    try jsonx.putNonEmpty(&object, allocator, "proto_script", try dgscripts_json.serializeProtoScript(allocator, room.proto_script));
    try jsonx.putNonEmpty(&object, allocator, "exits", try exits_json.serializeRoomExits(allocator, room));
    try jsonx.putNonEmpty(&object, allocator, "flags", try jsonx.serializeFlags(allocator, room, cdb.NUM_ROOM_FLAGS, roomFlagged));
    return object;
}

pub fn deserializeRoom(room: *cdb.room_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;

    if (try jsonx.intField(value, "id", cdb.room_vnum)) |v| cdb.room_id_set(room, v);
    if (try jsonx.intField(value, "sector", c_int)) |v| cdb.room_sector_type_set(room, v);
    try setStringField(options.c_allocator, room, value, "name", cdb.room_name_set);
    try setStringField(options.c_allocator, room, value, "description", cdb.room_description_set);
    if (jsonx.field(value, "extra_descriptions")) |items| try extradesc_json.deserializeExtraDescriptions(&room.ex_description, items);
    if (jsonx.field(value, "proto_script")) |items| try dgscripts_json.deserializeProtoScript(&room.proto_script, items);
    if (jsonx.field(value, "flags")) |flags| try jsonx.deserializeFlags(room, flags, cdb.NUM_ROOM_FLAGS, roomFlagSet);
}

fn roomFlagged(room: *cdb.room_data, pos: c_int) bool {
    return cdb.room_flagged(room, pos) != 0;
}

fn roomFlagSet(room: *cdb.room_data, pos: c_int, flag: bool) void {
    cdb.room_flag_set(room, pos, flag);
}

fn nul(value: []const u8) ![*:0]const u8 {
    return try std.heap.c_allocator.dupeZ(u8, value);
}

fn setStringField(allocator: std.mem.Allocator, room: *cdb.room_data, object: JsonValue, key: []const u8, comptime setter: anytype) !void {
    const value = try jsonx.stringFieldAlloc(allocator, object, key) orelse return;
    defer allocator.free(value);
    setter(room, try nul(value));
}
