const std = @import("std");

pub const player_files_json = @import("player_files_json.zig");

pub fn init(io: std.Io) void {
    player_files_json.init(io);
}

comptime {
    std.testing.refAllDecls(player_files_json);
}
