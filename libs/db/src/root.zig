const std = @import("std");

pub const cdb = @import("cdb");
pub const characters = @import("characters.zig");
pub const characters_api = @import("characters_api.zig");
pub const objects = @import("objects.zig");
pub const objects_api = @import("objects_api.zig");
pub const rooms_api = @import("rooms_api.zig");
pub const exits_api = @import("exits_api.zig");
pub const exits_json = @import("exits_json.zig");
pub const shops_api = @import("shops_api.zig");
pub const guilds_api = @import("guilds_api.zig");
pub const zones_api = @import("zones_api.zig");
pub const lua_api = @import("lua_api.zig");
pub const json_api = @import("json_api.zig");
pub const flags_json = @import("flags_json.zig");
pub const extradesc_json = @import("extradesc_json.zig");
pub const affected_json = @import("affected_json.zig");
pub const rooms_json = @import("rooms_json.zig");
pub const objects_json = @import("objects_json.zig");
pub const characters_json = @import("characters_json.zig");
pub const zones_json = @import("zones_json.zig");
pub const shops_json = @import("shops_json.zig");
pub const guilds_json = @import("guilds_json.zig");
pub const players_json = @import("players_json.zig");
pub const assembly_json = @import("assembly_json.zig");
pub const dgscripts_json = @import("dgscripts_json.zig");

// This stupid comptime and its function ensures that the C API functions aren't optimized out because Zig doesn't call them directly. They are called from C, so we have to force them to be included in the final binary.
comptime {
    forceApiExports(characters_api);
    forceApiExports(objects_api);
    forceApiExports(rooms_api);
    forceApiExports(exits_api);
    forceApiExports(shops_api);
    forceApiExports(guilds_api);
    forceApiExports(zones_api);
    forceApiExports(json_api);
    std.testing.refAllDecls(flags_json);
    std.testing.refAllDecls(extradesc_json);
    std.testing.refAllDecls(dgscripts_json);
    std.testing.refAllDecls(exits_json);
    std.testing.refAllDecls(rooms_json);
    std.testing.refAllDecls(objects_json);
    std.testing.refAllDecls(zones_json);
    std.testing.refAllDecls(characters_json);
    std.testing.refAllDecls(shops_json);
    std.testing.refAllDecls(guilds_json);
    std.testing.refAllDecls(players_json);
}

fn forceApiExports(comptime module: type) void {
    inline for (std.meta.declarations(module)) |decl| {
        _ = @field(module, decl.name);
    }
}

pub fn init(allocator: std.mem.Allocator, io: std.Io) !void {
    characters.init(allocator);
    objects.init(allocator);
    json_api.init(io);
    players_json.init(io);
    try lua_api.init(allocator, io);

    try lua_api.load_lua();
}

pub fn deinit() void {
    lua_api.deinit();
    objects.deinit();
    characters.deinit();
}
