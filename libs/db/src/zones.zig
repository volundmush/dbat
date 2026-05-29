const cdb = @import("cdb");
const std = @import("std");

const ZoneMap = std.AutoHashMap(cdb.zone_vnum, *cdb.zone_data);

var allocator: std.mem.Allocator = undefined;
var zone_map: ZoneMap = undefined;

pub fn init(init_allocator: std.mem.Allocator) void {
    allocator = init_allocator;
    zone_map = ZoneMap.init(allocator);
}

pub fn deinit() void {
    zone_map.deinit();
}

const ZoneIterator = struct {
    iter: ZoneMap.ValueIterator,
};

pub export fn zone_iterator_create() ?*anyopaque {
    const iterator = allocator.create(ZoneIterator) catch return null;
    iterator.* = .{ .iter = zone_map.valueIterator() };
    return iterator;
}

pub export fn zone_next(iterator_ptr: ?*anyopaque) ?*cdb.zone_data {
    const iterator: *ZoneIterator = @ptrCast(@alignCast(iterator_ptr orelse return null));
    const next_ptr = iterator.iter.next() orelse return null;
    return next_ptr.*;
}

pub export fn zone_iterator_free(iterator_ptr: ?*anyopaque) void {
    const iterator = iterator_ptr orelse return;
    allocator.destroy(@as(*ZoneIterator, @ptrCast(@alignCast(iterator))));
}

pub export fn zone_put(vnum: cdb.zone_vnum, zone: ?*cdb.zone_data) void {
    if (zone) |ptr| {
        zone_map.put(vnum, ptr) catch return;
    } else {
        _ = zone_map.remove(vnum);
    }
}

pub export fn zone_delete(vnum: cdb.zone_vnum) void {
    _ = zone_map.remove(vnum);
}

pub export fn zone_count() usize {
    return zone_map.count();
}

pub export fn zone_get(vnum: cdb.zone_vnum) ?*cdb.zone_data {
    return zone_map.get(vnum) orelse null;
}
