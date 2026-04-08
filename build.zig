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
    const mod_dbat_db = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true, .root_source_file = b.path("libs/db/src/db.zig") });

    mod_dbat_db.addIncludePath(b.path("libs/db/include"));
    const db_files = getSourceFiles(b, "libs/db/src", ".c");
    mod_dbat_db.addCSourceFiles(.{ .files = db_files, .flags = &[_][]const u8{"-std=gnu23"} });

    const dbat_db = b.addLibrary(.{
        .name = "dbat_db",
        .linkage = .static,
        .root_module = mod_dbat_db,
    });
    _ = dbat_db;
}
