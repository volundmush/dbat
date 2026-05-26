const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");

pub const JsonValue = jsonx.JsonValue;

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;

pub fn serializeExit(allocator: std.mem.Allocator, exit: *cdb.room_direction_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putString(&object, allocator, "description", cdb.exit_general_description_get(exit));
    try jsonx.putString(&object, allocator, "keyword", cdb.exit_keyword_get(exit));
    try jsonx.putInt(&object, allocator, "flags", cdb.exit_info_get(exit));
    try jsonx.putInt(&object, allocator, "key", cdb.exit_key_get(exit));
    try jsonx.putInt(&object, allocator, "to_room", cdb.exit_to_room_get(exit));
    try jsonx.putInt(&object, allocator, "dclock", cdb.exit_dclock_get(exit));
    try jsonx.putInt(&object, allocator, "dchide", cdb.exit_dchide_get(exit));
    try jsonx.putInt(&object, allocator, "dcskill", cdb.exit_dcskill_get(exit));
    try jsonx.putInt(&object, allocator, "dcmove", cdb.exit_dcmove_get(exit));
    try jsonx.putInt(&object, allocator, "failsavetype", cdb.exit_failsavetype_get(exit));
    try jsonx.putInt(&object, allocator, "dcfailsave", cdb.exit_dcfailsave_get(exit));
    try jsonx.putInt(&object, allocator, "failroom", cdb.exit_failroom_get(exit));
    try jsonx.putInt(&object, allocator, "totalfailroom", cdb.exit_totalfailroom_get(exit));
    return object;
}

pub fn deserializeExit(exit: *cdb.room_direction_data, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    if (try jsonx.stringField(value, "description")) |v| cdb.exit_general_description_set(exit, try nul(v));
    if (try jsonx.stringField(value, "keyword")) |v| cdb.exit_keyword_set(exit, try nul(v));
    if (try jsonx.intField(value, "flags", i16)) |v| cdb.exit_info_set(exit, v);
    if (try jsonx.intField(value, "key", cdb.obj_vnum)) |v| cdb.exit_key_set(exit, v);
    if (try jsonx.intField(value, "to_room", cdb.room_vnum)) |v| cdb.exit_to_room_set(exit, v);
    if (try jsonx.intField(value, "dclock", c_int)) |v| cdb.exit_dclock_set(exit, v);
    if (try jsonx.intField(value, "dchide", c_int)) |v| cdb.exit_dchide_set(exit, v);
    if (try jsonx.intField(value, "dcskill", c_int)) |v| cdb.exit_dcskill_set(exit, v);
    if (try jsonx.intField(value, "dcmove", c_int)) |v| cdb.exit_dcmove_set(exit, v);
    if (try jsonx.intField(value, "failsavetype", c_int)) |v| cdb.exit_failsavetype_set(exit, v);
    if (try jsonx.intField(value, "dcfailsave", c_int)) |v| cdb.exit_dcfailsave_set(exit, v);
    if (try jsonx.intField(value, "failroom", cdb.room_vnum)) |v| cdb.exit_failroom_set(exit, v);
    if (try jsonx.intField(value, "totalfailroom", cdb.room_vnum)) |v| cdb.exit_totalfailroom_set(exit, v);
}

pub fn serializeRoomExits(allocator: std.mem.Allocator, room: *cdb.room_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    for (0..cdb.NUM_OF_DIRS) |dir| {
        const exit = cdb.room_dir_option_get(room, @intCast(dir));
        if (exit == null) continue;
        try jsonx.put(&object, allocator, directionName(dir), try serializeExit(allocator, exit));
    }
    return object;
}

pub fn deserializeRoomExits(room: *cdb.room_data, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    var iter = value.object.iterator();
    while (iter.next()) |entry| {
        const dir = directionIndex(entry.key_ptr.*) orelse continue;
        const item = entry.value_ptr.*;
        const exit = try ensureRoomExit(room, dir);
        try deserializeExit(exit, item);
    }
}

fn directionName(dir: usize) []const u8 {
    return switch (dir) {
        cdb.NORTH => "north",
        cdb.EAST => "east",
        cdb.SOUTH => "south",
        cdb.WEST => "west",
        cdb.UP => "up",
        cdb.DOWN => "down",
        cdb.NORTHWEST => "northwest",
        cdb.NORTHEAST => "northeast",
        cdb.SOUTHEAST => "southeast",
        cdb.SOUTHWEST => "southwest",
        cdb.INDIR => "in",
        cdb.OUTDIR => "out",
        else => "unknown",
    };
}

fn directionIndex(name: []const u8) ?usize {
    inline for (0..cdb.NUM_OF_DIRS) |dir| {
        if (std.mem.eql(u8, name, directionName(dir))) return dir;
    }
    return null;
}

fn ensureRoomExit(room: *cdb.room_data, dir: usize) !*cdb.room_direction_data {
    if (room.dir_option[dir] == null) {
        room.dir_option[dir] = @ptrCast(@alignCast(calloc(1, @sizeOf(cdb.room_direction_data)) orelse return error.OutOfMemory));
        room.dir_option[dir].*.to_room = cdb.NOWHERE;
        room.dir_option[dir].*.failroom = cdb.NOWHERE;
        room.dir_option[dir].*.totalfailroom = cdb.NOWHERE;
        room.dir_option[dir].*.key = -1;
    }
    return room.dir_option[dir];
}

fn nul(value: []const u8) ![*:0]const u8 {
    return try std.heap.c_allocator.dupeZ(u8, value);
}
