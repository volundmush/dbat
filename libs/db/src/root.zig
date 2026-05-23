const std = @import("std");

pub const cdb = @import("cdb");
pub const characters = @import("characters.zig");
pub const objects = @import("objects.zig");
pub const objects_api = @import("objects_api.zig");
pub const rooms_api = @import("rooms_api.zig");
pub const exits_api = @import("exits_api.zig");

pub fn init(allocator: std.mem.Allocator) void {
    characters.init(allocator);
    objects.init(allocator);
}

pub fn deinit() void {
    objects.deinit();
    characters.deinit();
}
