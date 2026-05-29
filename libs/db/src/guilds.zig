const cdb = @import("cdb");
const std = @import("std");

const GuildMap = std.AutoHashMap(cdb.guild_vnum, *cdb.guild_data);

var allocator: std.mem.Allocator = undefined;
var guild_map: GuildMap = undefined;

pub fn init(init_allocator: std.mem.Allocator) void {
    allocator = init_allocator;
    guild_map = GuildMap.init(allocator);
}

pub fn deinit() void {
    guild_map.deinit();
}

const GuildIterator = struct {
    iter: GuildMap.ValueIterator,
};

pub export fn guild_iterator_create() ?*anyopaque {
    const iterator = allocator.create(GuildIterator) catch return null;
    iterator.* = .{ .iter = guild_map.valueIterator() };
    return iterator;
}

pub export fn guild_next(iterator_ptr: ?*anyopaque) ?*cdb.guild_data {
    const iterator: *GuildIterator = @ptrCast(@alignCast(iterator_ptr orelse return null));
    const next_ptr = iterator.iter.next() orelse return null;
    return next_ptr.*;
}

pub export fn guild_iterator_free(iterator_ptr: ?*anyopaque) void {
    const iterator = iterator_ptr orelse return;
    allocator.destroy(@as(*GuildIterator, @ptrCast(@alignCast(iterator))));
}

pub export fn guild_put(vnum: cdb.guild_vnum, guild: ?*cdb.guild_data) void {
    if (guild) |ptr| {
        guild_map.put(vnum, ptr) catch return;
    } else {
        _ = guild_map.remove(vnum);
    }
}

pub export fn guild_delete(vnum: cdb.guild_vnum) void {
    _ = guild_map.remove(vnum);
}

pub export fn guild_count() usize {
    return guild_map.count();
}
