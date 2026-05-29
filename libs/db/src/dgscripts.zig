const cdb = @import("cdb");
const std = @import("std");

const TrigProtoEntry = struct {
    proto: ?*cdb.trig_data = null,
    count: usize = 0,
};

const TrigProtoMap = std.AutoHashMap(cdb.trig_vnum, TrigProtoEntry);

var allocator: std.mem.Allocator = undefined;
var trig_proto_map: TrigProtoMap = undefined;

pub fn init(init_allocator: std.mem.Allocator) void {
    allocator = init_allocator;
    trig_proto_map = TrigProtoMap.init(allocator);
}

pub fn deinit() void {
    trig_proto_map.deinit();
}

const TrigProtoIterator = struct {
    iter: TrigProtoMap.Iterator,
};

pub export fn trig_proto_iterator_create() ?*anyopaque {
    const iterator = allocator.create(TrigProtoIterator) catch return null;
    iterator.* = .{ .iter = trig_proto_map.iterator() };
    return iterator;
}

pub export fn trig_proto_next(iterator_ptr: ?*anyopaque) ?*cdb.trig_data {
    const iterator: *TrigProtoIterator = @ptrCast(@alignCast(iterator_ptr orelse return null));
    while (iterator.iter.next()) |entry| {
        if (entry.value_ptr.*.proto) |ptr| {
            return ptr;
        }
    }
    return null;
}

pub export fn trig_proto_iterator_free(iterator_ptr: ?*anyopaque) void {
    const iterator = iterator_ptr orelse return;
    allocator.destroy(@as(*TrigProtoIterator, @ptrCast(@alignCast(iterator))));
}

pub export fn trig_proto_get(vnum: cdb.trig_vnum) ?*cdb.trig_data {
    return if (trig_proto_map.get(vnum)) |entry| entry.proto else null;
}

pub export fn trig_proto_by_id(vnum: cdb.trig_vnum) ?*cdb.trig_data {
    return trig_proto_get(vnum);
}

pub export fn trig_proto_count() usize {
    var total: usize = 0;
    var it = trig_proto_map.valueIterator();
    while (it.next()) |entry| {
        if (entry.*.proto != null) total += 1;
    }
    return total;
}

pub export fn trig_proto_put(vnum: cdb.trig_vnum, trig: ?*cdb.trig_data) void {
    if (trig) |ptr| {
        const entry = trig_proto_map.getOrPut(vnum) catch return;
        if (!entry.found_existing) {
            entry.value_ptr.* = .{ .proto = ptr };
            return;
        }
        entry.value_ptr.*.proto = ptr;
        return;
    }

    if (trig_proto_map.getPtr(vnum)) |entry| {
        entry.proto = null;
        if (entry.count == 0) {
            _ = trig_proto_map.remove(vnum);
        }
    }
}

pub export fn trig_proto_delete(vnum: cdb.trig_vnum) void {
    if (trig_proto_map.getPtr(vnum)) |entry| {
        entry.proto = null;
        if (entry.count == 0) {
            _ = trig_proto_map.remove(vnum);
        }
    }
}

pub export fn trig_proto_count_increment(vnum: cdb.trig_vnum) void {
    const entry = trig_proto_map.getOrPut(vnum) catch return;
    if (!entry.found_existing) {
        entry.value_ptr.* = .{ .count = 1 };
        return;
    }
    entry.value_ptr.*.count += 1;
}

pub export fn trig_proto_count_get(vnum: cdb.trig_vnum) usize {
    return if (trig_proto_map.get(vnum)) |entry| entry.count else 0;
}

pub export fn trig_proto_count_decrement(vnum: cdb.trig_vnum) void {
    const entry = trig_proto_map.getPtr(vnum) orelse return;
    if (entry.count == 0) return;
    entry.count -= 1;
    if (entry.count == 0 and entry.proto == null) {
        _ = trig_proto_map.remove(vnum);
    }
}
