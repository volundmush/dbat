const std = @import("std");

pub const cdb = @import("cdb");
pub const characters = @import("characters.zig");
pub const characters_api = @import("characters_api.zig");
pub const objects = @import("objects.zig");
pub const objects_api = @import("objects_api.zig");
pub const rooms_api = @import("rooms_api.zig");
pub const exits_api = @import("exits_api.zig");
pub const shops_api = @import("shops_api.zig");
pub const guilds_api = @import("guilds_api.zig");
pub const zones_api = @import("zones_api.zig");
pub const lua_api = @import("lua_api.zig");

// This stupid comptime and its function ensures that the C API functions aren't optimized out because Zig doesn't call them directly. They are called from C, so we have to force them to be included in the final binary.
comptime {
    forceApiExports(characters_api);
    forceApiExports(objects_api);
    forceApiExports(rooms_api);
    forceApiExports(exits_api);
    forceApiExports(shops_api);
    forceApiExports(guilds_api);
    forceApiExports(zones_api);
}

fn forceApiExports(comptime module: type) void {
    inline for (std.meta.declarations(module)) |decl| {
        _ = @field(module, decl.name);
    }
}

pub fn init(allocator: std.mem.Allocator, io: std.Io) !void {
    characters.init(allocator);
    objects.init(allocator);
    try lua_api.init(allocator, io);

    try lua_api.load_lua();
}

pub fn deinit() void {
    lua_api.deinit();
    objects.deinit();
    characters.deinit();
}
