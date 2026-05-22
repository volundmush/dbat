const std = @import("std");
const db = @import("db");

const zlua = @import("zlua");
const Lua = zlua.Lua;

extern fn run_circle(argc: c_int, argv: [*][*:0]u8) c_int;

pub fn main(init: std.process.Init) u8 {
    var lua = Lua.init(init.gpa) catch |err| {
        std.log.err("failed to initialize Lua: {t}", .{err});
        return 1;
    };
    defer lua.deinit();

    db.init(init.gpa);
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
