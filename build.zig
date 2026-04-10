const std = @import("std");

fn getSourceFiles(b: *std.Build, base_path: []const u8, ext: []const u8) []const []const u8 {
    const allocator = b.allocator;
    const empty: []const []const u8 = &[_][]const u8{};

    var dir = std.fs.cwd().openDir(base_path, .{ .iterate = true }) catch return empty;

    var walker = dir.walk(allocator) catch {
        dir.close();
        return empty;
    };
    defer walker.deinit();

    var capacity: usize = 16;
    var files = allocator.alloc([]const u8, capacity) catch return empty;
    var count: usize = 0;

    while (true) {
        const next_opt = walker.next() catch continue;
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

    dir.close();
    return files[0..count];
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // C library - libs/db
    const mod_dbat_db = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });

    mod_dbat_db.addIncludePath(b.path("libs/db/include"));
    const db_files = getSourceFiles(b, "libs/db/src", ".cpp");
    mod_dbat_db.addCSourceFiles(.{ .files = db_files, .flags = &[_][]const u8{ "-std=gnu++23", "-g" } });

    const dbat_db = b.addLibrary(.{
        .name = "dbat_db",
        .linkage = .static,
        .root_module = mod_dbat_db,
    });
    dbat_db.linkLibCpp();

    const mod_dbat_game = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
    mod_dbat_game.addIncludePath(b.path("libs/db/include"));
    mod_dbat_game.addIncludePath(b.path("libs/game/include"));
    const game_files = getSourceFiles(b, "libs/game/src", ".cpp");
    mod_dbat_game.addCSourceFiles(.{ .files = game_files, .flags = &[_][]const u8{ "-std=gnu++23", "-g", "-DPATH_MAX=4096" } });

    const dbat_game = b.addLibrary(.{
        .name = "dbat_game",
        .linkage = .static,
        .root_module = mod_dbat_game,
    });
    dbat_game.linkLibrary(dbat_db);
    dbat_game.linkLibCpp();

    // Executable - pure C app (circle)
    const circle_mod = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
    circle_mod.addIncludePath(b.path("libs/db/include"));
    circle_mod.addIncludePath(b.path("libs/game/include"));
    circle_mod.addCSourceFiles(.{ .files = &[_][]const u8{"apps/server/src/main.cpp"}, .flags = &[_][]const u8{ "-std=gnu++23", "-g", "-DPATH_MAX=4096" } });

    const exe = b.addExecutable(.{
        .name = "dbat",
        .root_module = circle_mod,
    });
    exe.linkLibrary(dbat_db);
    exe.linkLibrary(dbat_game);
    exe.linkLibCpp();

    b.installArtifact(dbat_db);
    b.installArtifact(dbat_game);
    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
}
