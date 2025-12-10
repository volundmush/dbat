const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    var arena = std.heap.ArenaAllocator.init(b.allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    const circle = b.addExecutable(.{
        .name = "circle",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });
    circle.linkLibC();
    addCSourceTree(b, circle, allocator, "libs/dbat/src");
    circle.addCSourceFile(.{
        .file = b.path("apps/circle/src/main.c"),
        .flags = &.{"-std=c23"},
    });
    circle.addIncludePath(b.path("libs/dbat/include"));
    circle.addIncludePath(b.path("libs/dbat/private"));
    linkCommonDeps(circle, target);
    b.installArtifact(circle);

    const run_cmd = b.addRunArtifact(circle);
    if (b.args) |args| run_cmd.addArgs(args);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Build and run the circle binary");
    run_step.dependOn(&run_cmd.step);
}

fn addCSourceTree(b: *std.Build, step: *std.Build.Step.Compile, allocator: std.mem.Allocator, root: []const u8) void {
    const Walker = struct {
        b: *std.Build,
        step: *std.Build.Step.Compile,
        allocator: std.mem.Allocator,

        fn walk(self: @This(), path: []const u8) void {
            var dir = std.fs.cwd().openDir(path, .{ .iterate = true }) catch |err| {
                std.debug.panic("unable to open '{s}': {s}", .{ path, @errorName(err) });
            };
            defer dir.close();

            var it = dir.iterate();
            while (true) {
                const entry_or_null = it.next() catch |err| {
                    std.debug.panic("iteration failed in '{s}': {s}", .{ path, @errorName(err) });
                };
                const entry = entry_or_null orelse break;

                if (std.mem.eql(u8, entry.name, ".") or std.mem.eql(u8, entry.name, "..")) continue;
                const child_path = std.fs.path.join(self.allocator, &.{ path, entry.name }) catch {
                    std.debug.panic("unable to create path for '{s}/{s}'", .{ path, entry.name });
                };

                switch (entry.kind) {
                    .file => {
                        if (std.mem.endsWith(u8, entry.name, ".c")) {
                            self.step.addCSourceFile(.{
                                .file = self.b.path(child_path),
                                .flags = &.{"-std=c23"},
                            });
                        }
                    },
                    .directory => self.walk(child_path),
                    else => {},
                }
            }
        }
    };

    var walker = Walker{ .b = b, .step = step, .allocator = allocator };
    walker.walk(root);
}

fn linkCommonDeps(step: *std.Build.Step.Compile, target: std.Build.ResolvedTarget) void {
    switch (target.result.os.tag) {
        .windows => {
            step.linkSystemLibrary("ws2_32");
        },
        else => {
            step.linkSystemLibrary("m");
        },
    }
}
