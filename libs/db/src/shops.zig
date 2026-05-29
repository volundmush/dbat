const cdb = @import("cdb");
const std = @import("std");

const ShopMap = std.AutoHashMap(cdb.shop_vnum, *cdb.shop_data);

var allocator: std.mem.Allocator = undefined;
var shop_map: ShopMap = undefined;

pub fn init(init_allocator: std.mem.Allocator) void {
    allocator = init_allocator;
    shop_map = ShopMap.init(allocator);
}

pub fn deinit() void {
    shop_map.deinit();
}

const ShopIterator = struct {
    iter: ShopMap.ValueIterator,
};

pub export fn shop_iterator_create() ?*anyopaque {
    const iterator = allocator.create(ShopIterator) catch return null;
    iterator.* = .{ .iter = shop_map.valueIterator() };
    return iterator;
}

pub export fn shop_next(iterator_ptr: ?*anyopaque) ?*cdb.shop_data {
    const iterator: *ShopIterator = @ptrCast(@alignCast(iterator_ptr orelse return null));
    const next_ptr = iterator.iter.next() orelse return null;
    return next_ptr.*;
}

pub export fn shop_iterator_free(iterator_ptr: ?*anyopaque) void {
    const iterator = iterator_ptr orelse return;
    allocator.destroy(@as(*ShopIterator, @ptrCast(@alignCast(iterator))));
}

pub export fn shop_put(vnum: cdb.shop_vnum, shop: ?*cdb.shop_data) void {
    if (shop) |ptr| {
        shop_map.put(vnum, ptr) catch return;
    } else {
        _ = shop_map.remove(vnum);
    }
}

pub export fn shop_by_id(vnum: cdb.shop_vnum) ?*cdb.shop_data {
    return shop_map.get(vnum);
}

pub export fn shop_delete(vnum: cdb.shop_vnum) void {
    _ = shop_map.remove(vnum);
}

pub export fn shop_count() usize {
    return shop_map.count();
}
