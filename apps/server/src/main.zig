const std = @import("std");
const db = @import("db");

extern fn run_circle(argc: c_int, argv: [*][*:0]u8) c_int;

pub fn main(init: std.process.Init) u8 {
    db.init(init.gpa, init.io) catch {
        std.process.fatal("failed to initialize database", .{});
    };
    defer db.deinit();

    const args = init.minimal.args.vector;
    if (args.len > std.math.maxInt(c_int)) {
        std.process.fatal("too many command line arguments", .{});
    }

    const argv: [*][*:0]u8 = @ptrCast(@constCast(args.ptr));
    const status = run_circle(@intCast(args.len), argv);
    if (status < 0 or status > std.math.maxInt(u8)) return 1;
    return @intCast(status);
}
