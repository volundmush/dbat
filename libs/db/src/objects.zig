const cdb = @import("cdb");
const std = @import("std");

const IdSet = std.AutoHashMap(i64, void);
const ListSet = std.StringHashMap(void);
const ObjCallback = *const fn (*cdb.obj_data) callconv(.c) void;
const ObjProtoEntry = struct {
    proto: ?*cdb.obj_proto_data = null,
    special: cdb.SpecialFunc = null,
    count: usize = 0,
};
const ObjProtoMap = std.AutoHashMap(cdb.obj_vnum, ObjProtoEntry);

var allocator: std.mem.Allocator = undefined;
var objs_by_id: std.AutoHashMap(i64, *cdb.obj_data) = undefined;
var subscriptions_by_list: std.StringHashMap(IdSet) = undefined;
var lists_by_id: std.AutoHashMap(i64, ListSet) = undefined;
var obj_proto_map: ObjProtoMap = undefined;

pub fn init(init_allocator: std.mem.Allocator) void {
    allocator = init_allocator;
    objs_by_id = std.AutoHashMap(i64, *cdb.obj_data).init(allocator);
    subscriptions_by_list = std.StringHashMap(IdSet).init(allocator);
    lists_by_id = std.AutoHashMap(i64, ListSet).init(allocator);
    obj_proto_map = ObjProtoMap.init(allocator);
}

pub fn deinit() void {
    deinitSubscriptions();
    objs_by_id.deinit();
    obj_proto_map.deinit();
}

pub export fn obj_by_id(id: i64) ?*cdb.obj_data {
    return objs_by_id.get(id) orelse null;
}

pub export fn obj_register_id(id: i64, obj: ?*cdb.obj_data) c_int {
    const ptr = obj orelse {
        obj_clear_subscriptions(id);
        _ = objs_by_id.remove(id);
        return 0;
    };

    objs_by_id.put(id, ptr) catch return -1;
    return 0;
}

pub export fn obj_unregister_id(id: i64) void {
    obj_clear_subscriptions(id);
    _ = objs_by_id.remove(id);
}

pub export fn obj_subscribe(id: i64, list_name: ?[*:0]const u8) c_int {
    const name = listNameSlice(list_name) orelse return -2;
    if (name.len == 0) return -2;

    var id_set = subscriptions_by_list.getPtr(name) orelse blk: {
        const owned_name = allocator.dupe(u8, name) catch return -1;
        errdefer allocator.free(owned_name);

        var new_set = IdSet.init(allocator);
        errdefer new_set.deinit();

        subscriptions_by_list.put(owned_name, new_set) catch return -1;
        break :blk subscriptions_by_list.getPtr(name).?;
    };

    if (id_set.contains(id)) return 0;

    var list_set = lists_by_id.getPtr(id) orelse blk: {
        var new_set = ListSet.init(allocator);
        errdefer new_set.deinit();

        lists_by_id.put(id, new_set) catch return -1;
        break :blk lists_by_id.getPtr(id).?;
    };

    if (list_set.contains(name)) {
        id_set.put(id, {}) catch return -1;
        return 0;
    }

    const owned_name = allocator.dupe(u8, name) catch return -1;
    errdefer allocator.free(owned_name);

    id_set.put(id, {}) catch return -1;
    list_set.put(owned_name, {}) catch {
        _ = id_set.remove(id);
        return -1;
    };

    return 0;
}

pub export fn obj_unsubscribe(id: i64, list_name: ?[*:0]const u8) void {
    const name = listNameSlice(list_name) orelse return;
    unsubscribe(id, name);
}

pub export fn obj_clear_subscriptions(id: i64) void {
    var removed = lists_by_id.fetchRemove(id) orelse return;
    defer removed.value.deinit();

    var it = removed.value.keyIterator();
    while (it.next()) |name_ptr| {
        removeIdFromList(id, name_ptr.*);
        allocator.free(name_ptr.*);
    }
}

pub export fn obj_for_each(list_name: ?[*:0]const u8, callback: ?ObjCallback) void {
    const name = listNameSlice(list_name) orelse return;
    const cb = callback orelse return;
    const id_set = subscriptions_by_list.getPtr(name) orelse return;

    var it = id_set.keyIterator();
    while (it.next()) |id_ptr| {
        if (obj_by_id(id_ptr.*)) |obj| {
            cb(obj);
        }
    }
}

const ObjProtoIterator = struct {
    iter: ObjProtoMap.Iterator,
};

pub export fn obj_proto_iterator_create() ?*anyopaque {
    const iterator = allocator.create(ObjProtoIterator) catch return null;
    iterator.* = .{ .iter = obj_proto_map.iterator() };
    return iterator;
}

pub export fn obj_proto_next(iterator_ptr: ?*anyopaque) ?*cdb.obj_proto_data {
    const iterator: *ObjProtoIterator = @ptrCast(@alignCast(iterator_ptr orelse return null));
    while (iterator.iter.next()) |entry| {
        if (entry.value_ptr.*.proto) |ptr| {
            return ptr;
        }
    }
    return null;
}

pub export fn obj_proto_iterator_free(iterator_ptr: ?*anyopaque) void {
    const iterator = iterator_ptr orelse return;
    allocator.destroy(@as(*ObjProtoIterator, @ptrCast(@alignCast(iterator))));
}

pub export fn obj_proto_get(vnum: cdb.obj_vnum) ?*cdb.obj_proto_data {
    return if (obj_proto_map.get(vnum)) |entry| entry.proto else null;
}

pub export fn obj_proto_count() usize {
    var total: usize = 0;
    var it = obj_proto_map.valueIterator();
    while (it.next()) |entry| {
        if (entry.*.proto != null) total += 1;
    }
    return total;
}

pub export fn obj_proto_put(vnum: cdb.obj_vnum, obj: ?*cdb.obj_proto_data) void {
    if (obj) |ptr| {
        const entry = obj_proto_map.getOrPut(vnum) catch return;
        if (!entry.found_existing) {
        entry.value_ptr.* = .{ .proto = ptr };
        return;
    }
    entry.value_ptr.*.proto = ptr;
    return;
    }

    if (obj_proto_map.getPtr(vnum)) |entry| {
        entry.proto = null;
        entry.special = null;
        if (entry.count == 0) {
            _ = obj_proto_map.remove(vnum);
        }
    }
}

pub export fn obj_proto_delete(vnum: cdb.obj_vnum) void {
    if (obj_proto_map.getPtr(vnum)) |entry| {
        entry.proto = null;
        entry.special = null;
        if (entry.count == 0) {
            _ = obj_proto_map.remove(vnum);
        }
    }
}

pub export fn obj_proto_special_get(vnum: cdb.obj_vnum) cdb.SpecialFunc {
    return if (obj_proto_map.get(vnum)) |entry| entry.special else null;
}

pub export fn obj_proto_special_set(vnum: cdb.obj_vnum, func: cdb.SpecialFunc) void {
    const entry = obj_proto_map.getOrPut(vnum) catch return;
    if (!entry.found_existing) {
        entry.value_ptr.* = .{ .special = func };
        return;
    }
    entry.value_ptr.*.special = func;
}

pub export fn obj_proto_count_increment(vnum: cdb.obj_vnum) void {
    const entry = obj_proto_map.getOrPut(vnum) catch return;
    if (!entry.found_existing) {
        entry.value_ptr.* = .{ .count = 1 };
        return;
    }
    entry.value_ptr.*.count += 1;
}

pub export fn obj_proto_count_get(vnum: cdb.obj_vnum) usize {
    return if (obj_proto_map.get(vnum)) |entry| entry.count else 0;
}

pub export fn obj_proto_count_decrement(vnum: cdb.obj_vnum) void {
    const entry = obj_proto_map.getPtr(vnum) orelse return;
    if (entry.count == 0) return;
    entry.count -= 1;
    if (entry.count == 0 and entry.proto == null and entry.special == null) {
        _ = obj_proto_map.remove(vnum);
    }
}

fn unsubscribe(id: i64, name: []const u8) void {
    removeIdFromList(id, name);

    const list_set = lists_by_id.getPtr(id) orelse return;
    if (list_set.fetchRemove(name)) |removed| {
        allocator.free(removed.key);
    }
    if (list_set.count() == 0) {
        list_set.deinit();
        _ = lists_by_id.remove(id);
    }
}

fn removeIdFromList(id: i64, name: []const u8) void {
    const id_set = subscriptions_by_list.getPtr(name) orelse return;
    _ = id_set.remove(id);

    if (id_set.count() == 0) {
        id_set.deinit();
        if (subscriptions_by_list.fetchRemove(name)) |removed| {
            allocator.free(removed.key);
        }
    }
}

fn deinitSubscriptions() void {
    var list_it = subscriptions_by_list.iterator();
    while (list_it.next()) |entry| {
        allocator.free(entry.key_ptr.*);
        entry.value_ptr.deinit();
    }
    subscriptions_by_list.deinit();

    var id_it = lists_by_id.valueIterator();
    while (id_it.next()) |list_set| {
        var key_it = list_set.keyIterator();
        while (key_it.next()) |key_ptr| {
            allocator.free(key_ptr.*);
        }
        list_set.deinit();
    }
    lists_by_id.deinit();
}

fn listNameSlice(list_name: ?[*:0]const u8) ?[]const u8 {
    const ptr = list_name orelse return null;
    return std.mem.span(ptr);
}
