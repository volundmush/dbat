const cdb = @import("cdb");
const std = @import("std");

const RoomMap = std.AutoHashMap(cdb.room_vnum, *cdb.room_data);

var allocator: std.mem.Allocator = undefined;
var room_map: RoomMap = undefined;

pub fn init(init_allocator: std.mem.Allocator) void {
    allocator = init_allocator;
    room_map = RoomMap.init(allocator);
}

pub fn deinit() void {
    room_map.deinit();
}

const RoomIterator = struct {
    iter: RoomMap.ValueIterator,
};

pub export fn room_iterator_create() ?*anyopaque {
    const iterator = allocator.create(RoomIterator) catch return null;
    iterator.* = .{ .iter = room_map.valueIterator() };
    return iterator;
}

pub export fn room_next(iterator_ptr: ?*anyopaque) ?*cdb.room_data {
    const iterator: *RoomIterator = @ptrCast(@alignCast(iterator_ptr orelse return null));
    const next_ptr = iterator.iter.next() orelse return null;
    return next_ptr.*;
}

pub export fn room_iterator_free(iterator_ptr: ?*anyopaque) void {
    const iterator = iterator_ptr orelse return;
    allocator.destroy(@as(*RoomIterator, @ptrCast(@alignCast(iterator))));
}

pub export fn room_put(vnum: cdb.room_vnum, room: ?*cdb.room_data) void {
    if (room) |ptr| {
        room_map.put(vnum, ptr) catch return;
    } else {
        _ = room_map.remove(vnum);
    }
}

pub export fn room_delete(vnum: cdb.room_vnum) void {
    _ = room_map.remove(vnum);
}

pub export fn room_count() usize {
    return room_map.count();
}

pub export fn room_get(vnum: cdb.room_vnum) ?*cdb.room_data {
    return room_map.get(vnum) orelse null;
}
