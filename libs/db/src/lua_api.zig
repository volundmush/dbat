const std = @import("std");
const zlua = @import("zlua");
const cdb = @import("cdb");
const characters_lua = @import("characters_lua.zig");
const objects_lua = @import("objects_lua.zig");
const rooms_lua = @import("rooms_lua.zig");
const zones_lua = @import("zones_lua.zig");
const shops_lua = @import("shops_lua.zig");
const guilds_lua = @import("guilds_lua.zig");
const modifiers_api = @import("modifiers_api.zig");

const Lua = zlua.Lua;

const LuaRepl = struct {
    descriptor: *cdb.descriptor_data,
    env_ref: i32,
};

const lua_root = "lua";
const categories = [_][]const u8{
    "commands",
    "conditions",
    "derived",
    "modifiers",
    "meters",
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
    \\
    \\function dbat._values(list)
    \\  local index = 0
    \\  return function()
    \\    index = index + 1
    \\    return list[index]
    \\  end
    \\end
;

var allocator: std.mem.Allocator = undefined;
var io: std.Io = undefined;
var lua_state: ?*Lua = null;
var initialized = false;
var loaded_entries: usize = 0;
var active_repl: ?*LuaRepl = null;

pub const StatDefinition = struct {
    default_value: i64 = 0,
    min_value: ?i64 = null,
    max_value: ?i64 = null,
};

pub const DerivedDefinition = struct {
    pub const max_legacy_modifiers = 16;

    pub const LegacyModifier = struct {
        location: c_int,
        specific: c_int,
    };

    base_stat_storage: [64]u8 = undefined,
    base_stat_len: usize = 0,
    legacy_modifiers: [max_legacy_modifiers]LegacyModifier = undefined,
    legacy_modifier_count: usize = 0,
    no_modifiers: bool = false,
    min_value: ?i64 = null,
    max_value: ?i64 = null,

    pub fn baseStat(self: *const DerivedDefinition, fallback: []const u8) []const u8 {
        if (self.base_stat_len == 0) return fallback;
        return self.base_stat_storage[0..self.base_stat_len];
    }
};

pub const MeterDefinition = struct {
    derived_stat_storage: [64]u8 = undefined,
    derived_stat_len: usize = 0,

    pub fn derivedStat(self: *const MeterDefinition, fallback: []const u8) []const u8 {
        if (self.derived_stat_len == 0) return fallback;
        return self.derived_stat_storage[0..self.derived_stat_len];
    }
};

pub const ConditionDefinition = struct {
    persistent: bool = false,
    stackable: bool = false,
};

pub fn init(alloc: std.mem.Allocator, runtime_io: std.Io) !void {
    allocator = alloc;
    io = runtime_io;
    lua_state = try Lua.init(alloc);
    initialized = true;

    const lua = lua_state.?;
    configureStandardLibraries(lua);
    registerReplPrint(lua);
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

pub fn statDefinition(name: []const u8) ?StatDefinition {
    if (!initialized or name.len == 0) return null;
    if (!(pushThing("stats", name) catch return null)) return null;
    defer pop(1);

    return .{
        .default_value = optionalIntegerField(-1, "default_value") orelse 0,
        .min_value = optionalIntegerField(-1, "min_value"),
        .max_value = optionalIntegerField(-1, "max_value"),
    };
}

pub fn derivedDefinition(name: []const u8) ?DerivedDefinition {
    if (!initialized or name.len == 0) return null;
    if (!(pushThing("derived", name) catch return null)) return null;
    defer pop(1);

    var definition = DerivedDefinition{
        .min_value = optionalIntegerField(-1, "min_value"),
        .max_value = optionalIntegerField(-1, "max_value"),
    };
    if (optionalStringField(-1, "base_stat")) |base_stat| {
        const len = @min(base_stat.len, definition.base_stat_storage.len);
        @memcpy(definition.base_stat_storage[0..len], base_stat[0..len]);
        definition.base_stat_len = len;
    }
    definition.no_modifiers = hasTag(-1, "no_modifiers");
    readLegacyModifiers(-1, &definition);
    return definition;
}

pub fn calculateDerivedBase(ch: *cdb.char_data, name: []const u8) ?i64 {
    if (!initialized or name.len == 0) return null;
    if (!(pushThing("derived", name) catch return null)) return null;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, "calculate_base") != .function) {
        lua.pop(1);
        return null;
    }

    characters_lua.pushCharacter(lua, ch.id);
    lua.protectedCall(.{ .args = 1, .results = 1 }) catch |err| {
        const error_text = lua.toString(-1) catch @errorName(err);
        std.log.err("derived {s} calculate_base failed: {s}", .{ name, error_text });
        lua.pop(1);
        return null;
    };
    defer lua.pop(1);
    return lua.toInteger(-1) catch null;
}

pub fn meterDefinition(name: []const u8) ?MeterDefinition {
    if (!initialized or name.len == 0) return null;
    if (!(pushThing("meters", name) catch return null)) return null;
    defer pop(1);

    var definition = MeterDefinition{};
    const derived_stat = optionalStringField(-1, "derived_stat") orelse optionalStringField(-1, "derived");
    if (derived_stat) |value| {
        const len = @min(value.len, definition.derived_stat_storage.len);
        @memcpy(definition.derived_stat_storage[0..len], value[0..len]);
        definition.derived_stat_len = len;
    }
    return definition;
}

pub fn conditionDefinition(name: []const u8) ?ConditionDefinition {
    if (!initialized or name.len == 0) return null;
    if (!(pushThing("conditions", name) catch return null)) return null;
    defer pop(1);
    return .{
        .persistent = optionalBoolField(-1, "persistent") orelse false,
        .stackable = optionalBoolField(-1, "stackable") orelse false,
    };
}

pub fn callConditionHook(ch: *cdb.char_data, condition: []const u8, comptime hook_name: [:0]const u8) void {
    if (!initialized or condition.len == 0) return;
    if (!(pushThing("conditions", condition) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, hook_name) != .function) {
        lua.pop(1);
        return;
    }
    characters_lua.pushCharacter(lua, ch.id);
    characters_lua.pushCondition(lua, ch.id, condition);
    lua.protectedCall(.{ .args = 2, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("condition {s}.{s} failed: {s}", .{ condition, hook_name, message });
        lua.pop(1);
    };
}

pub fn emitConditionModifiers(ch: *cdb.char_data, cache: *modifiers_api.ModifierCache, condition: []const u8) void {
    if (!initialized or condition.len == 0) return;
    if (!(pushThing("conditions", condition) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, "modifiers") != .function) {
        lua.pop(1);
        return;
    }
    characters_lua.pushCharacter(lua, ch.id);
    characters_lua.pushCondition(lua, ch.id, condition);
    lua.protectedCall(.{ .args = 2, .results = 1 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("condition {s}.modifiers failed: {s}", .{ condition, message });
        lua.pop(1);
        return;
    };
    defer lua.pop(1);
    if (!lua.isTable(-1)) return;

    var pos: zlua.Integer = 1;
    while (lua.getIndex(-1, pos) != .nil) : (pos += 1) {
        defer lua.pop(1);
        if (!lua.isTable(-1)) continue;
        addModifierFromLua(cache, condition, -1);
    }
}

fn optionalIntegerField(index: i32, comptime field_name: [:0]const u8) ?i64 {
    const lua = lua_state.?;
    const field_type = lua.getField(index, field_name);
    defer lua.pop(1);
    if (field_type == .nil) return null;
    return lua.toInteger(-1) catch null;
}

fn optionalStringField(index: i32, comptime field_name: [:0]const u8) ?[]const u8 {
    const lua = lua_state.?;
    const field_type = lua.getField(index, field_name);
    defer lua.pop(1);
    if (field_type == .nil) return null;
    const value = lua.toString(-1) catch return null;
    if (value.len == 0) return null;
    return value;
}

fn optionalBoolField(index: i32, comptime field_name: [:0]const u8) ?bool {
    const lua = lua_state.?;
    const field_type = lua.getField(index, field_name);
    defer lua.pop(1);
    if (field_type == .nil) return null;
    if (field_type != .boolean) return null;
    return lua.toBoolean(-1);
}

fn addModifierFromLua(cache: *modifiers_api.ModifierCache, condition: []const u8, index: i32) void {
    const lua = lua_state.?;
    if (lua.getField(index, "target") != .table) {
        lua.pop(1);
        return;
    }
    const target_category = tableString(-1, 1) orelse {
        lua.pop(1);
        return;
    };
    const target_id = tableString(-1, 2) orelse {
        lua.pop(1);
        return;
    };
    lua.pop(1);
    const kind_text = optionalStringField(index, "kind") orelse "flat";
    const kind = parseModifierKind(kind_text) orelse return;
    const value = optionalIntegerField(index, "value") orelse 0;
    const label = optionalStringField(index, "label") orelse condition;

    cache.add(.{
        .source_category = allocator.dupe(u8, "condition") catch return,
        .source_id = allocator.dupe(u8, condition) catch return,
        .target_category = allocator.dupe(u8, target_category) catch return,
        .target_id = allocator.dupe(u8, target_id) catch return,
        .kind = kind,
        .value = value,
        .label = allocator.dupe(u8, label) catch return,
        .owned_strings = true,
    }) catch {};
}

fn parseModifierKind(text: []const u8) ?modifiers_api.ModifierKind {
    if (std.mem.eql(u8, text, "flat")) return .flat;
    if (std.mem.eql(u8, text, "percent")) return .percent;
    if (std.mem.eql(u8, text, "multiplier")) return .multiplier;
    if (std.mem.eql(u8, text, "override_min")) return .override_min;
    if (std.mem.eql(u8, text, "override_max")) return .override_max;
    if (std.mem.eql(u8, text, "set")) return .set;
    return null;
}

fn tableString(index: i32, pos: zlua.Integer) ?[]const u8 {
    const lua = lua_state.?;
    const value_type = lua.getIndex(index, pos);
    defer lua.pop(1);
    if (value_type == .nil) return null;
    return lua.toString(-1) catch null;
}

fn hasTag(index: i32, tag: []const u8) bool {
    const lua = lua_state.?;
    if (lua.getField(index, "tags") != .table) {
        lua.pop(1);
        return false;
    }
    defer lua.pop(1);

    var pos: zlua.Integer = 1;
    while (lua.getIndex(-1, pos) != .nil) : (pos += 1) {
        const value = lua.toString(-1) catch {
            lua.pop(1);
            continue;
        };
        const found = std.mem.eql(u8, value, tag);
        lua.pop(1);
        if (found) return true;
    }
    lua.pop(1);
    return false;
}

fn readLegacyModifiers(index: i32, definition: *DerivedDefinition) void {
    const lua = lua_state.?;
    if (lua.getField(index, "legacy_modifiers") != .table) {
        lua.pop(1);
        return;
    }
    defer lua.pop(1);

    var pos: zlua.Integer = 1;
    while (definition.legacy_modifier_count < DerivedDefinition.max_legacy_modifiers) : (pos += 1) {
        if (lua.getIndex(-1, pos) == .nil) {
            lua.pop(1);
            break;
        }
        if (!lua.isTable(-1)) {
            lua.pop(1);
            continue;
        }

        const location = tableInteger(-1, 1) orelse {
            lua.pop(1);
            continue;
        };
        const specific = tableInteger(-1, 2) orelse 0;
        const location_int = std.math.cast(c_int, location) orelse {
            lua.pop(1);
            continue;
        };
        const specific_int = std.math.cast(c_int, specific) orelse {
            lua.pop(1);
            continue;
        };
        definition.legacy_modifiers[definition.legacy_modifier_count] = .{ .location = location_int, .specific = specific_int };
        definition.legacy_modifier_count += 1;
        lua.pop(1);
    }
}

fn tableInteger(index: i32, pos: zlua.Integer) ?i64 {
    const lua = lua_state.?;
    const value_type = lua.getIndex(index, pos);
    defer lua.pop(1);
    if (value_type == .nil) return null;
    return lua.toInteger(-1) catch null;
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

fn registerReplPrint(lua: *Lua) void {
    lua.pushFunction(zlua.wrap(luaReplPrint));
    lua.setGlobal("print");
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

fn luaReplPrint(lua: *Lua) i32 {
    const repl = active_repl orelse {
        logPrintArguments(lua);
        return 0;
    };

    const top = lua.getTop();
    cdb.desc_send_text(repl.descriptor, "@c> @n");
    var i: i32 = 1;
    while (i <= top) : (i += 1) {
        if (i > 1) cdb.desc_send_text(repl.descriptor, "\t");
        sendLuaValue(repl.descriptor, lua, i);
    }
    cdb.desc_send_text(repl.descriptor, "\r\n");
    return 0;
}

fn logPrintArguments(lua: *Lua) void {
    const top = lua.getTop();
    var i: i32 = 1;
    while (i <= top) : (i += 1) {
        const text = lua.toStringEx(i);
        std.log.info("lua: {s}", .{text});
        lua.pop(1);
    }
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

pub export fn lua_repl_launch(d: *cdb.descriptor_data) void {
    if (d.lua_repl == null) {
        d.lua_repl = createRepl(d) orelse return;
    }

    d.connected = cdb.CON_LUA;
    cdb.desc_send_text(d, "@GLua REPL@n started. Type @Yexit@n, @Yquit@n, or @Yclose@n to return to normal play.\r\n");
    cdb.desc_send_text(d, "@c< @n");
}

pub export fn lua_repl_close(d: *cdb.descriptor_data) void {
    if (active_repl != null and active_repl.?.descriptor == d) active_repl = null;
    if (d.lua_repl) |ptr| {
        const repl: *LuaRepl = @ptrCast(@alignCast(ptr));
        lua_state.?.unref(zlua.registry_index, repl.env_ref);
        allocator.destroy(repl);
        d.lua_repl = null;
    }
    d.connected = cdb.CON_PLAYING;
    cdb.desc_send_text(d, "Lua REPL closed.\r\n");
}

pub export fn lua_repl_parse(d: *cdb.descriptor_data, arg: [*:0]const u8) void {
    const line = std.mem.trim(u8, std.mem.span(arg), " \t\r\n");
    if (line.len == 0) {
        cdb.desc_send_text(d, "@c< @n");
        return;
    }

    if (isReplExit(line)) {
        lua_repl_close(d);
        return;
    }

    const repl = ensureRepl(d) orelse return;
    active_repl = repl;
    defer active_repl = null;

    echoReplInput(d, line);
    evalReplLine(d, line);
    cdb.desc_send_text(d, "@c< @n");
}

fn echoReplInput(d: *cdb.descriptor_data, line: []const u8) void {
    const line_z = allocator.dupeZ(u8, line) catch {
        cdb.desc_send_text(d, "<out of memory>\r\n");
        return;
    };
    defer allocator.free(line_z);
    cdb.desc_send_text(d, line_z.ptr);
    cdb.desc_send_text(d, "\r\n");
}

fn ensureRepl(d: *cdb.descriptor_data) ?*LuaRepl {
    if (d.lua_repl) |ptr| return @ptrCast(@alignCast(ptr));

    const repl = createRepl(d) orelse return null;
    d.lua_repl = repl;
    return repl;
}

fn createRepl(d: *cdb.descriptor_data) ?*LuaRepl {
    const lua = lua_state orelse {
        cdb.desc_send_text(d, "Lua is not initialized.\r\n");
        return null;
    };

    const env_ref = createReplEnv(d, lua) orelse return null;
    errdefer lua.unref(zlua.registry_index, env_ref);

    const repl = allocator.create(LuaRepl) catch {
        cdb.desc_send_text(d, "Unable to start Lua REPL.\r\n");
        return null;
    };
    repl.* = .{ .descriptor = d, .env_ref = env_ref };
    return repl;
}

fn createReplEnv(d: *cdb.descriptor_data, lua: *Lua) ?i32 {
    const top = lua.getTop();
    defer lua.setTop(top);

    lua.newTable();
    lua.newTable();
    lua.pushGlobalTable();
    lua.setField(-2, "__index");
    lua.setMetatable(-2);

    if (d.character != null) {
        characters_lua.pushCharacter(lua, cdb.char_id_get(d.character));
        lua.setField(-2, "me");
    }

    return lua.ref(zlua.registry_index);
}

fn isReplExit(line: []const u8) bool {
    return std.ascii.eqlIgnoreCase(line, "exit") or
        std.ascii.eqlIgnoreCase(line, "quit") or
        std.ascii.eqlIgnoreCase(line, "close");
}

fn evalReplLine(d: *cdb.descriptor_data, line: []const u8) void {
    const lua = lua_state orelse {
        cdb.desc_send_text(d, "Lua is not initialized.\r\n");
        return;
    };
    const repl = ensureRepl(d) orelse return;

    const top = lua.getTop();
    defer lua.setTop(top);

    if (tryEvalExpression(d, lua, repl, line)) return;
    evalStatement(d, lua, repl, line);
}

fn tryEvalExpression(d: *cdb.descriptor_data, lua: *Lua, repl: *LuaRepl, line: []const u8) bool {
    const expr = std.fmt.allocPrint(allocator, "return {s}", .{line}) catch {
        cdb.desc_send_text(d, "Out of memory.\r\n");
        return true;
    };
    defer allocator.free(expr);
    const expr_z = allocator.dupeZ(u8, expr) catch {
        cdb.desc_send_text(d, "Out of memory.\r\n");
        return true;
    };
    defer allocator.free(expr_z);

    const top = lua.getTop();
    lua.loadString(expr_z) catch {
        lua.setTop(top);
        return false;
    };
    setChunkEnv(lua, repl) catch {
        cdb.desc_send_text(d, "Unable to bind REPL environment.\r\n");
        lua.setTop(top);
        return true;
    };

    lua.protectedCall(.{ .results = zlua.mult_return }) catch |err| {
        sendLuaError(d, lua, err);
        lua.setTop(top);
        return true;
    };

    sendLuaResults(d, lua, top);
    return true;
}

fn evalStatement(d: *cdb.descriptor_data, lua: *Lua, repl: *LuaRepl, line: []const u8) void {
    const chunk = allocator.dupeZ(u8, line) catch {
        cdb.desc_send_text(d, "Out of memory.\r\n");
        return;
    };
    defer allocator.free(chunk);

    const top = lua.getTop();
    lua.loadString(chunk) catch |err| {
        sendLuaError(d, lua, err);
        lua.setTop(top);
        return;
    };
    setChunkEnv(lua, repl) catch {
        cdb.desc_send_text(d, "Unable to bind REPL environment.\r\n");
        lua.setTop(top);
        return;
    };

    lua.protectedCall(.{ .results = zlua.mult_return }) catch |err| {
        sendLuaError(d, lua, err);
        lua.setTop(top);
        return;
    };

    sendLuaResults(d, lua, top);
}

fn setChunkEnv(lua: *Lua, repl: *LuaRepl) !void {
    _ = lua.getIndexRaw(zlua.registry_index, repl.env_ref);
    _ = try lua.setUpvalue(-2, 1);
}

fn sendLuaResults(d: *cdb.descriptor_data, lua: *Lua, previous_top: i32) void {
    const result_count = lua.getTop() - previous_top;
    if (result_count <= 0) return;

    var i: i32 = previous_top + 1;
    while (i <= lua.getTop()) : (i += 1) {
        cdb.desc_send_text(d, "@c> @n");
        sendLuaValue(d, lua, i);
        cdb.desc_send_text(d, "\r\n");
    }
}

fn sendLuaValue(d: *cdb.descriptor_data, lua: *Lua, index: i32) void {
    const text = lua.toStringEx(index);
    cdb.desc_send_text(d, text.ptr);
    lua.pop(1);
}

fn sendLuaError(d: *cdb.descriptor_data, lua: *Lua, err: anyerror) void {
    cdb.desc_send_text(d, "@c> @n@RLua error:@n ");
    const message = lua.toString(-1) catch @errorName(err);
    cdb.desc_send_text(d, message.ptr);
    cdb.desc_send_text(d, "\r\n");
}
