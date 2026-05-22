const std = @import("std");

fn getSourceFiles(b: *std.Build, base_path: []const u8, ext: []const u8) []const []const u8 {
    const allocator = b.allocator;
    const io = b.graph.io;
    const empty: []const []const u8 = &[_][]const u8{};

    var dir = b.build_root.handle.openDir(io, base_path, .{ .iterate = true }) catch return empty;
    defer dir.close(io);

    var walker = dir.walk(allocator) catch {
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

    return files[0..count];
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const zlua = b.dependency("zlua", .{
        .target = target,
        .optimize = optimize,
        .lang = .lua55,
    });

    const zlua_module = zlua.module("zlua");

    const translate_cdb = b.addTranslateC(.{
        .root_source_file = b.path("libs/db/include/dbat/db/db.h"),
        .target = target,
        .optimize = optimize,
    });
    translate_cdb.addIncludePath(b.path("libs/db/include"));

    const mod_cdb = translate_cdb.createModule();

    // C library - libs/db
    const mod_dbat_db = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
        .root_source_file = b.path("libs/db/src/root.zig"),
        .imports = &.{.{ .name = "cdb", .module = mod_cdb }},
    });

    mod_dbat_db.addIncludePath(b.path("libs/db/include"));
    const db_files = getSourceFiles(b, "libs/db/src", ".cpp");
    mod_dbat_db.addCSourceFiles(.{ .files = db_files, .flags = &[_][]const u8{ "-std=gnu++23", "-w", "-g" } });

    const mod_dbat_db_zig = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
        .root_source_file = b.path("libs/db/src/root.zig"),
        .imports = &.{.{ .name = "cdb", .module = mod_cdb }},
    });

    const dbat_db = b.addLibrary(.{
        .name = "dbat_db",
        .linkage = .static,
        .root_module = mod_dbat_db,
    });

    const translate_game = b.addTranslateC(.{
        .root_source_file = b.path("libs/game/include/dbat/game/game.h"),
        .target = target,
        .optimize = optimize,
    });
    translate_game.addIncludePath(b.path("libs/db/include"));
    translate_game.addIncludePath(b.path("libs/game/include"));

    const mod_cgame = translate_game.createModule();

    const mod_dbat_game = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
        .root_source_file = b.path("libs/game/src/root.zig"),
        .imports = &.{
            .{ .name = "cgame", .module = mod_cgame },
            .{ .name = "zlua", .module = zlua_module },
        },
    });
    mod_dbat_game.addIncludePath(b.path("libs/db/include"));
    mod_dbat_game.addIncludePath(b.path("libs/game/include"));
    const game_files = getSourceFiles(b, "libs/game/src", ".cpp");
    mod_dbat_game.addCSourceFiles(.{ .files = game_files, .flags = &[_][]const u8{ "-std=gnu++23", "-w", "-g", "-DPATH_MAX=4096" } });
    mod_dbat_game.linkLibrary(dbat_db);

    const dbat_game = b.addLibrary(.{
        .name = "dbat_game",
        .linkage = .static,
        .root_module = mod_dbat_game,
    });

    // Executable - Zig entrypoint with legacy C++ runner.
    const circle_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
        .root_source_file = b.path("apps/server/src/main.zig"),
        .imports = &.{
            .{ .name = "db", .module = mod_dbat_db_zig },
            .{ .name = "zlua", .module = zlua_module },
        },
    });
    circle_mod.addIncludePath(b.path("libs/db/include"));
    circle_mod.addIncludePath(b.path("libs/game/include"));
    circle_mod.addCSourceFiles(.{ .files = &[_][]const u8{"apps/server/src/run.cpp"}, .flags = &[_][]const u8{ "-std=gnu++23", "-w", "-g", "-DPATH_MAX=4096" } });
    circle_mod.linkLibrary(dbat_game);

    const exe = b.addExecutable(.{
        .name = "dbat",
        .root_module = circle_mod,
    });

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
