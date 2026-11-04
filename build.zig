const std = @import("std");

/// Flags shared by every C file in the server. Keep in sync with libs/dbat/CMakeLists.txt.
const c_flags = [_][]const u8{
    "-std=gnu23",
    "-g",
    "-fno-omit-frame-pointer",
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter",
    "-Wno-sign-compare",
    "-Wno-format-truncation", // snprintf truncating is the intended, bounded behaviour
    "-Wno-unused-result", // 45 unchecked fscanf/fgets/system results; tracked separately
    "-Wmissing-prototypes",
    "-Werror=format-security",
    "-Werror=return-type",
    "-Werror=implicit-function-declaration",
    "-Wshadow",
    "-Wredundant-decls",
    "-Wwrite-strings",
    // -Wlogical-op, -Wduplicated-cond and -Wduplicated-branches are GCC-only; the GCC CMake presets have them.
};

/// Collects every file under base_path ending in ext.
/// NOTE: the build runner caches this list and does not notice files being added or deleted, so after
/// adding or removing a .c file run `rm -rf .zig-cache` (otherwise: "file_open FileNotFound").
fn getSourceFiles(b: *std.Build, base_path: []const u8, ext: []const u8) []const []const u8 {
    const allocator = b.allocator;
    const io = b.graph.io;
    const empty: []const []const u8 = &[_][]const u8{};

    var dir = std.Io.Dir.cwd().openDir(io, base_path, .{ .iterate = true }) catch return empty;

    var walker = dir.walk(allocator) catch {
        dir.close(io);
        return empty;
    };
    defer walker.deinit();

    var capacity: usize = 16;
    var files = allocator.alloc([]const u8, capacity) catch return empty;
    var count: usize = 0;

    while (true) {
        const next_opt = walker.next(io) catch continue;
        if (next_opt) |entry| {
            if (entry.kind == .file and std.mem.endsWith(u8, entry.path, ext)) {
                if (count >= capacity) {
                    capacity *= 2;
                    files = allocator.realloc(files, capacity) catch return empty;
                }
                // Prepend base_path to get full relative path
                files[count] = std.fmt.allocPrint(allocator, "{s}/{s}", .{ base_path, entry.path }) catch return empty;
                count += 1;
            }
        } else {
            break;
        }
    }

    dir.close(io);
    return files[0..count];
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // C library - libs/dbat
    const mod_dbat = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
    mod_dbat.addIncludePath(b.path("libs/dbat/include"));
    const dbat_files = getSourceFiles(b, "libs/dbat/src", ".c");
    mod_dbat.addCSourceFiles(.{ .files = dbat_files, .flags = &c_flags });

    const dbat_lib = b.addLibrary(.{
        .name = "dbat",
        .linkage = .static,
        .root_module = mod_dbat,
    });

    // Executable - pure C app (circle)
    const circle_mod = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
    circle_mod.addIncludePath(b.path("libs/dbat/include"));
    circle_mod.addCSourceFiles(.{ .files = &[_][]const u8{"apps/server/src/main.c"}, .flags = &c_flags });

    const exe = b.addExecutable(.{
        .name = "dbat",
        .root_module = circle_mod,
    });
    circle_mod.linkLibrary(dbat_lib);
    exe.rdynamic = true; // export symbols so crash backtraces in the log show function names

    b.installArtifact(dbat_lib);
    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());

    run_cmd.addPassthruArgs();

    // `zig build check`: boot in syntax-check mode against data/ to validate the world files.
    const check_step = b.step("check", "Build, then run `dbat -c` against data/ to validate world files");
    const check_cmd = b.addRunArtifact(exe);
    check_cmd.addArgs(&.{ "-c", "9999" });
    check_cmd.has_side_effects = true;
    check_step.dependOn(&check_cmd.step);

    // `zig build test`: self-contained tests, needing no data/ files.
    const test_mod = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
    test_mod.addIncludePath(b.path("libs/dbat/include"));
    test_mod.addCSourceFiles(.{ .files = &[_][]const u8{"libs/dbat/tests/test_game_world.c"}, .flags = &c_flags });
    test_mod.linkLibrary(dbat_lib);
    const test_exe = b.addExecutable(.{
        .name = "dbat_test_game_world",
        .root_module = test_mod,
    });
    const test_step = b.step("test", "Build and run the tests");
    const test_cmd = b.addRunArtifact(test_exe);
    test_cmd.has_side_effects = true;
    test_step.dependOn(&test_cmd.step);

    // Tests that boot the fixture world through the harness.
    const commands_step = b.step("test-commands", "Run every command with awkward arguments (slow)");
    for ([_][]const u8{ "world", "bot", "commands" }) |name| {
        const mod = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
        mod.addIncludePath(b.path("libs/dbat/include"));
        mod.addIncludePath(b.path("libs/dbat/tests"));
        mod.addCSourceFiles(.{
            .files = &[_][]const u8{ b.fmt("libs/dbat/tests/test_{s}.c", .{name}), "libs/dbat/tests/harness.c" },
            .flags = &(c_flags ++ [_][]const u8{"-DDBAT_TEST_FIXTURE=\"libs/dbat/tests/fixtures/world\""}),
        });
        mod.linkLibrary(dbat_lib);
        const exe_test = b.addExecutable(.{ .name = b.fmt("dbat_test_{s}", .{name}), .root_module = mod });
        const run_test = b.addRunArtifact(exe_test);
        run_test.has_side_effects = true;
        run_test.setCwd(b.path(".")); // the fixture's path is relative to here
        (if (std.mem.eql(u8, name, "commands")) commands_step else test_step).dependOn(&run_test.step);
    }
}
