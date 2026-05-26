const std = @import("std");
const zlua = @import("zlua");
const characters_lua = @import("characters_lua.zig");
const objects_lua = @import("objects_lua.zig");
const rooms_lua = @import("rooms_lua.zig");
const zones_lua = @import("zones_lua.zig");
const shops_lua = @import("shops_lua.zig");
const guilds_lua = @import("guilds_lua.zig");

const Lua = zlua.Lua;

const lua_root = "lua";
const categories = [_][]const u8{
    "commands",
    "derived",
    "modifiers",
    "ocommands",
    "races",
    "senseis",
    "skills",
    "stats",
    "transformations",
};

const bootstrap: [:0]const u8 =
    \\local dbat = require("dbat")
    \\dbat.registry = dbat.registry or {}
    \\
    \\function dbat._register(category, slug, path, value)
    \\  if value == nil then
    \\    error(path .. " returned nil")
    \\  end
    \\
    \\  local bucket = dbat.registry[category]
    \\  if bucket == nil then
    \\    bucket = {}
    \\    dbat.registry[category] = bucket
    \\  end
    \\
    \\  if type(value) == "table" then
    \\    value.id = value.id or slug
    \\    value._path = value._path or path
    \\    value._category = value._category or category
    \\  end
    \\
    \\  if bucket[slug] ~= nil then
    \\    error("duplicate lua entry: " .. category .. "/" .. slug)
    \\  end
    \\
    \\  bucket[slug] = value
    \\  return value
    \\end
    \\
    \\function dbat.get(category, slug)
    \\  local bucket = dbat.registry[category]
    \\  if bucket == nil then return nil end
    \\  return bucket[slug]
    \\end
    \\
    \\function dbat.category(category)
    \\  return dbat.registry[category] or {}
    \\end
;

var allocator: std.mem.Allocator = undefined;
var io: std.Io = undefined;
var lua_state: ?*Lua = null;
var initialized = false;
var loaded_entries: usize = 0;

pub fn init(alloc: std.mem.Allocator, runtime_io: std.Io) !void {
    allocator = alloc;
    io = runtime_io;
    lua_state = try Lua.init(alloc);
    initialized = true;

    const lua = lua_state.?;
    configureStandardLibraries(lua);
    registerDbatModule(lua);
    try lua.doString(bootstrap);
}

pub fn deinit() void {
    if (!initialized) return;
    lua_state.?.deinit();
    lua_state = null;
    initialized = false;
    loaded_entries = 0;
}

pub fn state() *Lua {
    return lua_state.?;
}

pub fn load_lua() !void {
    loaded_entries = 0;
    inline for (categories) |category| {
        try loadCategory(category);
    }
    std.log.info("loaded {} Lua entries", .{loaded_entries});
}

pub fn loadedCount() usize {
    return loaded_entries;
}

pub fn pushThing(category: []const u8, slug: []const u8) !bool {
    const lua = lua_state.?;
    const top = lua.getTop();
    errdefer lua.setTop(top);

    const category_z = try allocator.dupeZ(u8, category);
    defer allocator.free(category_z);
    const slug_z = try allocator.dupeZ(u8, slug);
    defer allocator.free(slug_z);

    if (lua.getGlobal("dbat") != .table) {
        lua.setTop(top);
        return false;
    }
    if (lua.getField(-1, "registry") != .table) {
        lua.setTop(top);
        return false;
    }
    if (lua.getField(-1, category_z) != .table) {
        lua.setTop(top);
        return false;
    }
    if (lua.getField(-1, slug_z) == .nil) {
        lua.setTop(top);
        return false;
    }

    lua.remove(-2); // category table
    lua.remove(-2); // registry table
    lua.remove(-2); // dbat table
    return true;
}

pub fn pop(count: i32) void {
    lua_state.?.pop(count);
}

fn configureStandardLibraries(lua: *Lua) void {
    lua.openLibs();

    // Sandbox levers for later. Uncomment these to remove broad standard-library access.
    // lua.pushNil(); lua.setGlobal("io");
    // lua.pushNil(); lua.setGlobal("os");
    // lua.pushNil(); lua.setGlobal("debug");
    // lua.pushNil(); lua.setGlobal("package");
    // lua.pushNil(); lua.setGlobal("load");
    // lua.pushNil(); lua.setGlobal("dofile");
    // lua.pushNil(); lua.setGlobal("require");
}

fn registerDbatModule(lua: *Lua) void {
    lua.requireF("dbat", zlua.wrap(openDbat), true);
    lua.pop(1);
}

fn openDbat(lua: *Lua) i32 {
    lua.newTable();

    lua.newTable();
    lua.setField(-2, "registry");

    characters_lua.register(lua);
    objects_lua.register(lua);
    rooms_lua.register(lua);
    zones_lua.register(lua);
    shops_lua.register(lua);
    guilds_lua.register(lua);

    lua.pushFunction(zlua.wrap(luaLog));
    lua.setField(-2, "log");

    return 1;
}

fn luaLog(lua: *Lua) i32 {
    const message = lua.toString(1) catch "";
    std.log.info("lua: {s}", .{message});
    return 0;
}

fn loadCategory(comptime category: []const u8) !void {
    const dir_path = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ lua_root, category });
    defer allocator.free(dir_path);

    var dir = std.Io.Dir.cwd().openDir(io, dir_path, .{ .iterate = true }) catch |err| switch (err) {
        error.FileNotFound => return,
        else => return err,
    };
    defer dir.close(io);

    var iter = dir.iterate();
    while (try iter.next(io)) |entry| {
        if (entry.kind != .file) continue;
        if (!std.mem.endsWith(u8, entry.name, ".lua")) continue;

        const slug = entry.name[0 .. entry.name.len - ".lua".len];
        const path = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ dir_path, entry.name });
        defer allocator.free(path);

        try loadThing(category, slug, path);
    }
}

fn loadThing(category: []const u8, slug: []const u8, path: []const u8) !void {
    const lua = lua_state.?;
    lua.setTop(0);
    errdefer lua.setTop(0);

    const path_z = try allocator.dupeZ(u8, path);
    defer allocator.free(path_z);

    switch (zlua.lang) {
        .lua51, .luajit => try lua.loadFile(path_z),
        else => try lua.loadFile(path_z, .text),
    }

    lua.protectedCall(.{ .results = 1 }) catch |err| {
        reportLuaError(path, err);
        return err;
    };

    try registerReturnedValue(category, slug, path);
    loaded_entries += 1;
    lua.setTop(0);
}

fn registerReturnedValue(category: []const u8, slug: []const u8, path: []const u8) !void {
    const lua = lua_state.?;

    const category_z = try allocator.dupeZ(u8, category);
    defer allocator.free(category_z);
    const slug_z = try allocator.dupeZ(u8, slug);
    defer allocator.free(slug_z);
    const path_z = try allocator.dupeZ(u8, path);
    defer allocator.free(path_z);

    _ = lua.getGlobal("dbat");
    _ = lua.getField(-1, "_register");
    lua.remove(-2);
    _ = lua.pushString(category_z);
    _ = lua.pushString(slug_z);
    _ = lua.pushString(path_z);
    lua.pushValue(1);

    lua.protectedCall(.{ .args = 4, .results = 0 }) catch |err| {
        reportLuaError(path, err);
        return err;
    };
}

fn reportLuaError(path: []const u8, err: anyerror) void {
    const lua = lua_state.?;
    const message = lua.toString(-1) catch @errorName(err);
    std.log.err("Lua error in {s}: {s}", .{ path, message });
}
