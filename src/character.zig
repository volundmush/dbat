const cdb = @import("cdb");
const std = @import("std");
const event_queue = @import("event_queue.zig");

const IdSet = std.AutoHashMap(i64, void);
const IdList = std.ArrayListUnmanaged(i64);
const CharCallback = *const fn (*cdb.char_data) callconv(.c) void;
const MobProtoEntry = struct {
    proto: ?*cdb.mob_proto_data = null,
    special: cdb.SpecialFunc = null,
    count: usize = 0,
};
const MobProtoMap = std.AutoHashMap(cdb.mob_vnum, MobProtoEntry);

var allocator: std.mem.Allocator = undefined;
var chars_by_id: std.AutoHashMap(i64, *cdb.char_data) = undefined;
var subscriptions_by_list: std.StringHashMap(IdSet) = undefined;
var mob_proto_map: MobProtoMap = undefined;
var extract_pending: std.array_list.Managed(i64) = undefined;
var char_inventory_map: std.AutoHashMap(i64, IdList) = undefined;
var char_followers_map: std.AutoHashMap(i64, IdList) = undefined;
var char_clones_map: std.AutoHashMap(i64, IdList) = undefined;
var command_queues: std.AutoHashMap(i64, std.ArrayListUnmanaged([]u8)) = undefined;

pub fn init(init_allocator: std.mem.Allocator) void {
    allocator = init_allocator;
    chars_by_id = std.AutoHashMap(i64, *cdb.char_data).init(allocator);
    subscriptions_by_list = std.StringHashMap(IdSet).init(allocator);
    mob_proto_map = MobProtoMap.init(allocator);
    extract_pending = std.array_list.Managed(i64).init(allocator);
    char_inventory_map = std.AutoHashMap(i64, IdList).init(allocator);
    char_followers_map = std.AutoHashMap(i64, IdList).init(allocator);
    char_clones_map = std.AutoHashMap(i64, IdList).init(allocator);
    command_queues = std.AutoHashMap(i64, std.ArrayListUnmanaged([]u8)).init(allocator);
}

pub fn deinit() void {
    deinitSubscriptions();
    chars_by_id.deinit();
    mob_proto_map.deinit();
    extract_pending.deinit();
    {
        var it = char_inventory_map.valueIterator();
        while (it.next()) |list| list.deinit(allocator);
        char_inventory_map.deinit();
    }
    {
        var it = char_followers_map.valueIterator();
        while (it.next()) |list| list.deinit(allocator);
        char_followers_map.deinit();
    }
    {
        var it = char_clones_map.valueIterator();
        while (it.next()) |list| list.deinit(allocator);
        char_clones_map.deinit();
    }
    {
        var it = command_queues.valueIterator();
        while (it.next()) |q| {
            for (q.items) |s| allocator.free(s);
            q.deinit(allocator);
        }
        command_queues.deinit();
    }
}

// Queue a character id for end-of-tick extraction. Callers guard against
// double-queueing via the NOTDEADYET flags.
pub export fn char_extract_pending_add(id: i64) void {
    extract_pending.append(id) catch {};
}

// Hand the pending-extraction ids to the caller (malloc'd; free with
// char_subscribe_ids_free) and clear the queue. Ids queued while the caller
// drains are picked up by the next call.
pub export fn char_extract_pending_take(out_count: *usize) ?[*]i64 {
    const count = extract_pending.items.len;
    if (count == 0) {
        out_count.* = 0;
        return null;
    }
    const mem = std.c.malloc(count * @sizeOf(i64)) orelse {
        out_count.* = 0;
        return null;
    };
    const ids: [*]i64 = @ptrCast(@alignCast(mem));
    @memcpy(ids[0..count], extract_pending.items);
    extract_pending.clearRetainingCapacity();
    out_count.* = count;
    return ids;
}

pub export fn char_by_id(id: i64) ?*cdb.char_data {
    return chars_by_id.get(id) orelse null;
}

pub export fn char_register_id(id: i64, ch: ?*cdb.char_data) c_int {
    const ptr = ch orelse {
        char_unregister_id(id);
        return 0;
    };

    chars_by_id.put(id, ptr) catch return -1;
    return 0;
}

pub export fn char_unregister_id(id: i64) void {
    char_clear_subscriptions(id);
    _ = event_queue.cancelOwner(event_queue.OWNER_CHAR, id, null);
    if (char_inventory_map.fetchRemove(id)) |kv| {
        var list = kv.value;
        list.deinit(allocator);
    }
    if (char_followers_map.fetchRemove(id)) |kv| {
        var list = kv.value;
        list.deinit(allocator);
    }
    if (char_clones_map.fetchRemove(id)) |kv| {
        var list = kv.value;
        list.deinit(allocator);
    }
    if (command_queues.fetchRemove(id)) |kv| {
        var q = kv.value;
        for (q.items) |s| allocator.free(s);
        q.deinit(allocator);
    }
    _ = chars_by_id.remove(id);
}

pub export fn char_subscribe(id: i64, list_name: ?[*:0]const u8) c_int {
    const name = listNameSlice(list_name) orelse return -2;
    if (name.len == 0) return -2;

    var id_set = subscriptions_by_list.getPtr(name) orelse blk: {
        const owned_name = allocator.dupe(u8, name) catch return -1;
        var new_set = IdSet.init(allocator);
        subscriptions_by_list.put(owned_name, new_set) catch {
            allocator.free(owned_name);
            new_set.deinit();
            return -1;
        };
        break :blk subscriptions_by_list.getPtr(name).?;
    };

    id_set.put(id, {}) catch return -1;
    return 0;
}

pub export fn char_unsubscribe(id: i64, list_name: ?[*:0]const u8) void {
    const name = listNameSlice(list_name) orelse return;
    unsubscribe(id, name);
}

pub export fn char_clear_subscriptions(id: i64) void {
    var empty_names: [64][]const u8 = undefined;
    var empty_count: usize = 0;

    var it = subscriptions_by_list.iterator();
    while (it.next()) |entry| {
        if (entry.value_ptr.remove(id)) {
            if (entry.value_ptr.count() == 0 and empty_count < 64) {
                empty_names[empty_count] = entry.key_ptr.*;
                empty_count += 1;
            }
        }
    }
    for (empty_names[0..empty_count]) |name| {
        if (subscriptions_by_list.fetchRemove(name)) |removed| {
            var val = removed.value;
            val.deinit();
            allocator.free(removed.key);
        }
    }
}

pub export fn char_for_each(list_name: ?[*:0]const u8, callback: ?CharCallback) void {
    const name = listNameSlice(list_name) orelse return;
    const cb = callback orelse return;
    const id_set = subscriptions_by_list.getPtr(name) orelse return;

    var it = id_set.keyIterator();
    while (it.next()) |id_ptr| {
        if (char_by_id(id_ptr.*)) |ch| {
            cb(ch);
        }
    }
}

pub export fn char_subscribe_add(ch: *cdb.char_data, tag: ?[*:0]const u8) c_int {
    return char_subscribe(cdb.char_id_get(ch), tag);
}

pub export fn char_subscribe_remove(ch: *cdb.char_data, tag: ?[*:0]const u8) void {
    char_unsubscribe(cdb.char_id_get(ch), tag);
}

pub export fn char_unsubscribe_all(ch: *cdb.char_data) void {
    char_clear_subscriptions(cdb.char_id_get(ch));
}

pub export fn char_subscribe_ids(tag: ?[*:0]const u8, out_count: *usize) ?[*]i64 {
    const name = listNameSlice(tag) orelse {
        out_count.* = 0;
        return null;
    };
    const id_set = subscriptions_by_list.getPtr(name) orelse {
        out_count.* = 0;
        return null;
    };
    const count = id_set.count();
    const mem = std.c.malloc(count * @sizeOf(i64)) orelse {
        out_count.* = 0;
        return null;
    };
    const ids: [*]i64 = @ptrCast(@alignCast(mem));
    var i: usize = 0;
    var it = id_set.keyIterator();
    while (it.next()) |id_ptr| {
        ids[i] = id_ptr.*;
        i += 1;
    }
    out_count.* = count;
    return ids;
}

pub export fn char_subscribe_ids_free(ptr: ?[*]i64) void {
    std.c.free(@as(?*anyopaque, @ptrCast(ptr)));
}

// Snapshot of every live character id. Free with char_subscribe_ids_free.
pub export fn char_all_ids(out_count: *usize) ?[*]i64 {
    const count = chars_by_id.count();
    if (count == 0) {
        out_count.* = 0;
        return null;
    }
    const mem = std.c.malloc(count * @sizeOf(i64)) orelse {
        out_count.* = 0;
        return null;
    };
    const ids: [*]i64 = @ptrCast(@alignCast(mem));
    var i: usize = 0;
    var it = chars_by_id.keyIterator();
    while (it.next()) |id_ptr| {
        ids[i] = id_ptr.*;
        i += 1;
    }
    out_count.* = count;
    return ids;
}

// Same snapshot sorted newest-first (descending id). Ids are assigned
// monotonically at creation, so this reproduces the old head-inserted
// character_list ordering exactly.
pub export fn char_all_ids_newest(out_count: *usize) ?[*]i64 {
    const ids = char_all_ids(out_count) orelse return null;
    std.mem.sort(i64, ids[0..out_count.*], {}, std.sort.desc(i64));
    return ids;
}

const MobProtoIterator = struct {
    iter: MobProtoMap.Iterator,
};

const CharIterator = struct {
    iter: std.AutoHashMap(i64, *cdb.char_data).ValueIterator,
};

pub export fn char_iterator_create() ?*anyopaque {
    const iterator = allocator.create(CharIterator) catch return null;
    iterator.* = .{ .iter = chars_by_id.valueIterator() };
    return iterator;
}

pub export fn char_next(iterator_ptr: ?*anyopaque) ?*cdb.char_data {
    const iterator: *CharIterator = @ptrCast(@alignCast(iterator_ptr orelse return null));
    return (iterator.iter.next() orelse return null).*;
}

pub export fn char_iterator_free(iterator_ptr: ?*anyopaque) void {
    const iterator = iterator_ptr orelse return;
    allocator.destroy(@as(*CharIterator, @ptrCast(@alignCast(iterator))));
}

pub export fn mob_proto_iterator_create() ?*anyopaque {
    const iterator = allocator.create(MobProtoIterator) catch return null;
    iterator.* = .{ .iter = mob_proto_map.iterator() };
    return iterator;
}

pub export fn mob_proto_next(iterator_ptr: ?*anyopaque) ?*cdb.mob_proto_data {
    const iterator: *MobProtoIterator = @ptrCast(@alignCast(iterator_ptr orelse return null));
    while (iterator.iter.next()) |entry| {
        if (entry.value_ptr.*.proto) |ptr| {
            return ptr;
        }
    }
    return null;
}

pub export fn mob_proto_iterator_free(iterator_ptr: ?*anyopaque) void {
    const iterator = iterator_ptr orelse return;
    allocator.destroy(@as(*MobProtoIterator, @ptrCast(@alignCast(iterator))));
}

pub export fn mob_proto_get(vnum: cdb.mob_vnum) ?*cdb.mob_proto_data {
    return if (mob_proto_map.get(vnum)) |entry| entry.proto else null;
}

pub export fn mob_proto_count() usize {
    var total: usize = 0;
    var it = mob_proto_map.valueIterator();
    while (it.next()) |entry| {
        if (entry.*.proto != null) total += 1;
    }
    return total;
}

pub export fn mob_proto_put(vnum: cdb.mob_vnum, mob: ?*cdb.mob_proto_data) void {
    if (mob) |ptr| {
        const entry = mob_proto_map.getOrPut(vnum) catch return;
        if (!entry.found_existing) {
            entry.value_ptr.* = .{ .proto = ptr };
            return;
        }
        entry.value_ptr.*.proto = ptr;
        return;
    }

    if (mob_proto_map.getPtr(vnum)) |entry| {
        entry.proto = null;
        entry.special = null;
        if (entry.count == 0) {
            _ = mob_proto_map.remove(vnum);
        }
    }
}

pub export fn mob_proto_delete(vnum: cdb.mob_vnum) void {
    if (mob_proto_map.getPtr(vnum)) |entry| {
        entry.proto = null;
        entry.special = null;
        if (entry.count == 0) {
            _ = mob_proto_map.remove(vnum);
        }
    }
}

pub export fn mob_proto_special_get(vnum: cdb.mob_vnum) cdb.SpecialFunc {
    return if (mob_proto_map.get(vnum)) |entry| entry.special else null;
}

pub export fn mob_proto_special_set(vnum: cdb.mob_vnum, func: cdb.SpecialFunc) void {
    const entry = mob_proto_map.getOrPut(vnum) catch return;
    if (!entry.found_existing) {
        entry.value_ptr.* = .{ .special = func };
        return;
    }
    entry.value_ptr.*.special = func;
}

pub export fn mob_proto_count_increment(vnum: cdb.mob_vnum) void {
    const entry = mob_proto_map.getOrPut(vnum) catch return;
    if (!entry.found_existing) {
        entry.value_ptr.* = .{ .count = 1 };
        return;
    }
    entry.value_ptr.*.count += 1;
}

pub export fn mob_proto_count_get(vnum: cdb.mob_vnum) usize {
    return if (mob_proto_map.get(vnum)) |entry| entry.count else 0;
}

pub export fn mob_proto_count_decrement(vnum: cdb.mob_vnum) void {
    const entry = mob_proto_map.getPtr(vnum) orelse return;
    if (entry.count == 0) return;
    entry.count -= 1;
    if (entry.count == 0 and (entry.proto == null and entry.special == null)) {
        _ = mob_proto_map.remove(vnum);
    }
}

// --- Character inventory tracking ---

pub export fn char_inventory_add(ch: *cdb.char_data, obj: *cdb.obj_data) void {
    const ch_id = cdb.char_id_get(ch);
    const obj_id = cdb.obj_id_get(obj);
    const entry = char_inventory_map.getOrPut(ch_id) catch return;
    if (!entry.found_existing) entry.value_ptr.* = IdList.empty;
    entry.value_ptr.append(allocator, obj_id) catch {};
}

pub export fn char_inventory_remove(ch: *cdb.char_data, obj: *cdb.obj_data) void {
    const ch_id = cdb.char_id_get(ch);
    const obj_id = cdb.obj_id_get(obj);
    const list = char_inventory_map.getPtr(ch_id) orelse return;
    for (list.items, 0..) |item, i| {
        if (item == obj_id) {
            _ = list.swapRemove(i);
            return;
        }
    }
}

pub export fn char_inventory_ids(ch: *cdb.char_data, out_count: *usize) ?[*]i64 {
    const ch_id = cdb.char_id_get(ch);
    const list = char_inventory_map.getPtr(ch_id) orelse {
        out_count.* = 0;
        return null;
    };
    const count = list.items.len;
    if (count == 0) {
        out_count.* = 0;
        return null;
    }
    const mem = std.c.malloc(count * @sizeOf(i64)) orelse {
        out_count.* = 0;
        return null;
    };
    const ids: [*]i64 = @ptrCast(@alignCast(mem));
    @memcpy(ids[0..count], list.items);
    out_count.* = count;
    return ids;
}

pub export fn char_inventory_ids_free(ptr: ?[*]i64) void {
    std.c.free(@as(?*anyopaque, @ptrCast(ptr)));
}

pub export fn char_inventory_first(ch: *cdb.char_data) ?*cdb.obj_data {
    const ch_id = cdb.char_id_get(ch);
    const list = char_inventory_map.getPtr(ch_id) orelse return null;
    if (list.items.len == 0) return null;
    return cdb.obj_by_id(list.items[0]);
}

pub export fn char_inventory_count_simple(ch: *cdb.char_data) usize {
    const ch_id = cdb.char_id_get(ch);
    const list = char_inventory_map.getPtr(ch_id) orelse return 0;
    return list.items.len;
}

pub export fn char_inventory_move_after(ch: *cdb.char_data, obj: *cdb.obj_data, after: ?*cdb.obj_data) void {
    const ch_id = cdb.char_id_get(ch);
    const list = char_inventory_map.getPtr(ch_id) orelse return;
    const obj_id = cdb.obj_id_get(obj);
    const obj_idx = for (list.items, 0..) |id, i| {
        if (id == obj_id) break i;
    } else return;
    _ = list.orderedRemove(obj_idx);
    if (after) |after_obj| {
        const after_id = cdb.obj_id_get(after_obj);
        const after_idx = for (list.items, 0..) |id, i| {
            if (id == after_id) break i;
        } else {
            list.insert(allocator, 0, obj_id) catch {};
            return;
        };
        list.insert(allocator, after_idx + 1, obj_id) catch {};
    } else {
        list.insert(allocator, 0, obj_id) catch {};
    }
}

// --- Follower tracking ---

pub export fn char_follower_add(master: *cdb.char_data, follower: *cdb.char_data) void {
    const master_id = cdb.char_id_get(master);
    const follower_id = cdb.char_id_get(follower);
    const entry = char_followers_map.getOrPut(master_id) catch return;
    if (!entry.found_existing) entry.value_ptr.* = IdList.empty;
    entry.value_ptr.append(allocator, follower_id) catch {};
}

pub export fn char_follower_remove(master: *cdb.char_data, follower: *cdb.char_data) void {
    const master_id = cdb.char_id_get(master);
    const follower_id = cdb.char_id_get(follower);
    const list = char_followers_map.getPtr(master_id) orelse return;
    for (list.items, 0..) |item, i| {
        if (item == follower_id) {
            _ = list.swapRemove(i);
            return;
        }
    }
}

pub export fn char_follower_ids(master: *cdb.char_data, out_count: *usize) ?[*]i64 {
    const master_id = cdb.char_id_get(master);
    const list = char_followers_map.getPtr(master_id) orelse {
        out_count.* = 0;
        return null;
    };
    const count = list.items.len;
    if (count == 0) {
        out_count.* = 0;
        return null;
    }
    const mem = std.c.malloc(count * @sizeOf(i64)) orelse {
        out_count.* = 0;
        return null;
    };
    const ids: [*]i64 = @ptrCast(@alignCast(mem));
    @memcpy(ids[0..count], list.items);
    out_count.* = count;
    return ids;
}

pub export fn char_follower_count(master: *cdb.char_data) usize {
    const master_id = cdb.char_id_get(master);
    const list = char_followers_map.getPtr(master_id) orelse return 0;
    return list.items.len;
}

pub export fn char_follower_ids_free(ptr: ?[*]i64) void {
    std.c.free(@as(?*anyopaque, @ptrCast(ptr)));
}

// --- Clone tracking (original → list of clone char ids) ---

pub export fn char_clone_add(original: *cdb.char_data, clone: *cdb.char_data) void {
    const orig_id = cdb.char_id_get(original);
    const clone_id = cdb.char_id_get(clone);
    const entry = char_clones_map.getOrPut(orig_id) catch return;
    if (!entry.found_existing) entry.value_ptr.* = IdList.empty;
    entry.value_ptr.append(allocator, clone_id) catch {};
}

pub export fn char_clone_remove(original: *cdb.char_data, clone: *cdb.char_data) void {
    const orig_id = cdb.char_id_get(original);
    const clone_id = cdb.char_id_get(clone);
    const list = char_clones_map.getPtr(orig_id) orelse return;
    for (list.items, 0..) |item, i| {
        if (item == clone_id) {
            _ = list.swapRemove(i);
            return;
        }
    }
}

pub export fn char_clone_count(original: *cdb.char_data) usize {
    const orig_id = cdb.char_id_get(original);
    const list = char_clones_map.getPtr(orig_id) orelse return 0;
    return list.items.len;
}

pub export fn char_clone_ids(original: *cdb.char_data, out_count: *usize) ?[*]i64 {
    const orig_id = cdb.char_id_get(original);
    const list = char_clones_map.getPtr(orig_id) orelse {
        out_count.* = 0;
        return null;
    };
    const count = list.items.len;
    if (count == 0) {
        out_count.* = 0;
        return null;
    }
    const mem = std.c.malloc(count * @sizeOf(i64)) orelse {
        out_count.* = 0;
        return null;
    };
    const ids: [*]i64 = @ptrCast(@alignCast(mem));
    @memcpy(ids[0..count], list.items);
    out_count.* = count;
    return ids;
}

pub export fn char_clone_ids_free(ptr: ?[*]i64) void {
    std.c.free(@as(?*anyopaque, @ptrCast(ptr)));
}

fn unsubscribe(id: i64, name: []const u8) void {
    removeIdFromList(id, name);
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
}

fn listNameSlice(list_name: ?[*:0]const u8) ?[]const u8 {
    const ptr = list_name orelse return null;
    return std.mem.span(ptr);
}

// --- Per-character command queue ---

pub export fn char_command_enqueue(ch: *cdb.char_data, cmd: [*:0]const u8) void {
    const id = cdb.char_id_get(ch);
    const text = std.mem.span(cmd);
    const owned = allocator.dupe(u8, text) catch return;
    const q = command_queues.getPtr(id) orelse blk: {
        command_queues.put(id, std.ArrayListUnmanaged([]u8).empty) catch {
            allocator.free(owned);
            return;
        };
        break :blk command_queues.getPtr(id).?;
    };
    q.append(allocator, owned) catch {
        allocator.free(owned);
    };
}

pub export fn char_command_dequeue(ch: *cdb.char_data) ?[*:0]u8 {
    const id = cdb.char_id_get(ch);
    const q = command_queues.getPtr(id) orelse return null;
    if (q.items.len == 0) return null;
    const slice = q.orderedRemove(0);
    defer allocator.free(slice);
    // Return a malloc-allocated sentinel-terminated copy the caller must free.
    const mem: [*:0]u8 = @ptrCast(std.c.malloc(slice.len + 1) orelse return null);
    @memcpy(mem[0..slice.len], slice);
    mem[slice.len] = 0;
    return mem;
}

pub export fn char_command_has_pending(ch: *cdb.char_data) bool {
    const id = cdb.char_id_get(ch);
    const q = command_queues.getPtr(id) orelse return false;
    return q.items.len > 0;
}

pub export fn char_command_clear(ch: *cdb.char_data) void {
    const id = cdb.char_id_get(ch);
    const q = command_queues.getPtr(id) orelse return;
    for (q.items) |s| allocator.free(s);
    q.clearRetainingCapacity();
}
