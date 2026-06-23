const std = @import("std");
const zlua = @import("zlua");
const cdb = @import("cdb");
const characters_lua = @import("character_lua.zig");
const objects_lua = @import("object_lua.zig");
const rooms_lua = @import("room_lua.zig");
const zones_lua = @import("zone_lua.zig");
const shops_lua = @import("shop_lua.zig");
const guilds_lua = @import("guild_lua.zig");
const dgscripts_lua = @import("dgscript_lua.zig");
const consts_lua = @import("consts_lua.zig");
const modifiers_api = @import("modifiers_api.zig");
const intern_mod = @import("intern.zig");

const Lua = zlua.Lua;

const LuaRepl = struct {
    descriptor: *cdb.descriptor_data,
    env_ref: i32,
};

const lua_root = "lua";

const Category = struct { namespace: []const u8, name: []const u8, dir: []const u8 };
const categories = [_]Category{
    .{ .namespace = "characters", .name = "commands",          .dir = "characters/commands" },
    .{ .namespace = "characters", .name = "conditions",        .dir = "characters/conditions" },
    .{ .namespace = "characters", .name = "derived",           .dir = "characters/derived" },
    .{ .namespace = "characters", .name = "modifiers",         .dir = "characters/modifiers" },
    .{ .namespace = "characters", .name = "meters",            .dir = "characters/meters" },
    .{ .namespace = "characters", .name = "pcommands",         .dir = "characters/pcommands" },
    .{ .namespace = "characters", .name = "races",             .dir = "characters/races" },
    .{ .namespace = "characters", .name = "senseis",           .dir = "characters/senseis" },
    .{ .namespace = "characters", .name = "skills",            .dir = "characters/skills" },
    .{ .namespace = "characters", .name = "stats",             .dir = "characters/stats" },
    .{ .namespace = "characters", .name = "transformations",   .dir = "characters/transformations" },
    .{ .namespace = "characters", .name = "attacks",           .dir = "characters/attacks" },
    .{ .namespace = "characters", .name = "character_scripts", .dir = "characters/scripts" },
    .{ .namespace = "objects",    .name = "object_scripts",    .dir = "objects/scripts" },
    .{ .namespace = "rooms",      .name = "room_scripts",      .dir = "rooms/scripts" },
    .{ .namespace = "zones",      .name = "zone_scripts",      .dir = "zones/scripts" },
};

var allocator: std.mem.Allocator = undefined;
var io: std.Io = undefined;
var lua_state: ?*Lua = null;
var initialized = false;

pub fn isInitialized() bool {
    return initialized;
}

pub fn getLuaState() ?*Lua {
    return lua_state;
}
var loaded_entries: usize = 0;
var active_repl: ?*LuaRepl = null;
pub var definition_cache: DefinitionCache = undefined;
var definition_cache_initialized = false;

pub const StatDefinition = struct {
    default_value: i64 = 0,
    min_value: ?i64 = null,
    max_value: ?i64 = null,
};

pub const ModifierTarget = struct {
    category: [64]u8 = undefined,
    category_len: usize = 0,
    id: [64]u8 = undefined,
    id_len: usize = 0,
};

pub const DerivedDefinition = struct {
    pub const max_legacy_modifiers = 16;
    pub const max_modifier_targets = 8;

    pub const LegacyModifier = struct {
        location: c_int,
        specific: c_int,
    };

    base_stat_storage: [64]u8 = undefined,
    base_stat_len: usize = 0,
    legacy_modifiers: [max_legacy_modifiers]LegacyModifier = undefined,
    legacy_modifier_count: usize = 0,
    modifier_targets: [max_modifier_targets]ModifierTarget = undefined,
    modifier_target_count: usize = 0,
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
    linked_condition_storage: [64:0]u8 = [_:0]u8{0} ** 64,
    linked_condition_len: usize = 0,

    pub fn derivedStat(self: *const MeterDefinition, fallback: []const u8) []const u8 {
        if (self.derived_stat_len == 0) return fallback;
        return self.derived_stat_storage[0..self.derived_stat_len];
    }

    pub fn linkedCondition(self: *const MeterDefinition) ?[:0]const u8 {
        if (self.linked_condition_len == 0) return null;
        return self.linked_condition_storage[0..self.linked_condition_len :0];
    }
};

pub const ConditionDefinition = struct {
    persistent: bool = false,
    stackable: bool = false,
};

const ConditionMetadata = struct {
    definition: ConditionDefinition,
    tags: std.StringHashMap(void),
    exclusive_tags: std.StringHashMap(void),
    legacy_affects: std.AutoHashMap(i32, void),

    fn init(alloc: std.mem.Allocator, definition: ConditionDefinition) ConditionMetadata {
        return .{
            .definition = definition,
            .tags = std.StringHashMap(void).init(alloc),
            .exclusive_tags = std.StringHashMap(void).init(alloc),
            .legacy_affects = std.AutoHashMap(i32, void).init(alloc),
        };
    }

    fn deinit(self: *ConditionMetadata) void {
        self.tags.deinit();
        self.exclusive_tags.deinit();
        self.legacy_affects.deinit();
    }
};

pub const ScriptDefinition = struct { persistent: bool };

const DefinitionCache = struct {
    stats: std.StringHashMap(StatDefinition),
    derived: std.StringHashMap(DerivedDefinition),
    meters: std.StringHashMap(MeterDefinition),
    conditions: std.StringHashMap(ConditionMetadata),
    character_scripts: std.StringHashMap(ScriptDefinition),
    object_scripts: std.StringHashMap(ScriptDefinition),
    room_scripts: std.StringHashMap(ScriptDefinition),
    zone_scripts: std.StringHashMap(ScriptDefinition),

    fn init(alloc: std.mem.Allocator) DefinitionCache {
        return .{
            .stats = std.StringHashMap(StatDefinition).init(alloc),
            .derived = std.StringHashMap(DerivedDefinition).init(alloc),
            .meters = std.StringHashMap(MeterDefinition).init(alloc),
            .conditions = std.StringHashMap(ConditionMetadata).init(alloc),
            .character_scripts = std.StringHashMap(ScriptDefinition).init(alloc),
            .object_scripts = std.StringHashMap(ScriptDefinition).init(alloc),
            .room_scripts = std.StringHashMap(ScriptDefinition).init(alloc),
            .zone_scripts = std.StringHashMap(ScriptDefinition).init(alloc),
        };
    }

    fn clear(self: *DefinitionCache) void {
        self.stats.clearRetainingCapacity();
        self.derived.clearRetainingCapacity();
        self.meters.clearRetainingCapacity();
        var conditions = self.conditions.iterator();
        while (conditions.next()) |entry| entry.value_ptr.deinit();
        self.conditions.clearRetainingCapacity();
        self.character_scripts.clearRetainingCapacity();
        self.object_scripts.clearRetainingCapacity();
        self.room_scripts.clearRetainingCapacity();
        self.zone_scripts.clearRetainingCapacity();
    }

    fn deinit(self: *DefinitionCache) void {
        self.clear();
        self.stats.deinit();
        self.derived.deinit();
        self.meters.deinit();
        self.conditions.deinit();
        self.character_scripts.deinit();
        self.object_scripts.deinit();
        self.room_scripts.deinit();
        self.zone_scripts.deinit();
    }
};

pub fn init(alloc: std.mem.Allocator, runtime_io: std.Io) !void {
    allocator = alloc;
    io = runtime_io;
    definition_cache = DefinitionCache.init(alloc);
    definition_cache_initialized = true;
    lua_state = try Lua.init(alloc);
    initialized = true;

    const lua = lua_state.?;
    configureStandardLibraries(lua);
    registerReplPrint(lua);
    registerDbatModule(lua);
    try lua.doFile(lua_root ++ "/bootstrap.lua");
}

pub fn deinit() void {
    if (!initialized) return;
    lua_state.?.deinit();
    lua_state = null;
    initialized = false;
    loaded_entries = 0;
    definition_cache.deinit();
    definition_cache_initialized = false;
}

pub fn internString(text: []const u8) []const u8 {
    if (text.len == 0) return text;
    const id = intern_mod.intern(text);
    return intern_mod.nameOf(id);
}

pub export fn lua_reload() bool {
    if (!initialized or active_repl != null) return false;
    lua_state.?.deinit();
    lua_state = Lua.init(allocator) catch return false;
    const lua = lua_state.?;
    configureStandardLibraries(lua);
    registerReplPrint(lua);
    registerDbatModule(lua);
    lua.doFile(lua_root ++ "/bootstrap.lua") catch return false;
    load_lua() catch return false;
    return true;
}

pub fn state() *Lua {
    return lua_state.?;
}

pub fn load_lua() !void {
    loaded_entries = 0;
    definition_cache.clear();
    inline for (categories) |cat| {
        std.log.info("loading Lua category: {s}/{s}", .{ cat.namespace, cat.name });
        try loadCategory(cat.namespace, cat.name, cat.dir);
    }
    std.log.info("loaded {} Lua entries", .{loaded_entries});
}

pub fn loadedCount() usize {
    return loaded_entries;
}

pub fn runFile(path: []const u8) !bool {
    if (!initialized) return error.NotInitialized;
    const lua = lua_state.?;
    const top = lua.getTop();
    defer lua.setTop(top);

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

    if (lua.isBoolean(-1)) return lua.toBoolean(-1);
    return true;
}

pub fn pushThing(category: []const u8, slug: []const u8) !bool {
    const lua = lua_state.?;
    const top = lua.getTop();
    errdefer lua.setTop(top);

    if (lua.getGlobal("dbat") != .table) {
        lua.setTop(top);
        return false;
    }
    if (lua.getField(-1, "get") != .function) {
        lua.setTop(top);
        return false;
    }
    lua.remove(-2); // dbat table (get is a closure, doesn't need it on stack)
    _ = lua.pushString(category);
    _ = lua.pushString(slug);
    lua.protectedCall(.{ .args = 2, .results = 1 }) catch {
        lua.setTop(top);
        return false;
    };
    if (lua.isNil(-1)) {
        lua.setTop(top);
        return false;
    }
    return true;
}

pub fn pop(count: i32) void {
    lua_state.?.pop(count);
}

pub export fn char_cmd_execute(ch: *cdb.char_data, command: ?[*:0]const u8, arguments: ?[*:0]const u8) bool {
    if (!initialized) return false;
    const command_name = spanNonEmpty(command) orelse return false;
    const argument_text = if (arguments) |ptr| std.mem.span(ptr) else "";
    if (!(pushThing("commands", command_name) catch return false)) return false;

    const lua = lua_state.?;
    const top = lua.getTop();
    defer lua.setTop(top - 1);

    if (!lua.isTable(-1)) return false;
    const command_index = lua.getTop();
    const slug = optionalStringField(command_index, "id") orelse command_name;
    const alias = commandAlias(command_index, command_name) catch command_name;
    defer if (alias.ptr != command_name.ptr) allocator.free(alias);

    if (!callCommandGate(ch, command_index, "can_see", slug)) return false;
    if (!callCommandGate(ch, command_index, "can_execute", slug)) return false;

    if (lua.getField(command_index, "execute") != .function) {
        lua.pop(1);
        std.log.err("command {s} has no execute function", .{slug});
        return false;
    }

    pushCommandContext(lua, ch, slug, alias, argument_text) catch {
        lua.pop(1);
        return false;
    };

    lua.protectedCall(.{ .args = 1, .results = 1 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("command {s}.execute failed: {s}", .{ slug, message });
        lua.pop(1);
        return false;
    };
    defer lua.pop(1);
    if (lua.isBoolean(-1) and !lua.toBoolean(-1)) return false;
    return true;
}

// Call dbat.characters.pcommand_try(ch, full_input) — returns true if handled.
pub export fn char_pcommand_try(ch: *cdb.char_data, full_input: [*:0]const u8) bool {
    return callCharacterDispatch(ch, "pcommand_try", full_input, null);
}

// Call dbat.characters.command_fallback(ch, cmd_word, arguments) — returns true if handled.
pub export fn char_command_fallback(ch: *cdb.char_data, cmd_word: [*:0]const u8, arguments: [*:0]const u8) bool {
    return callCharacterDispatch(ch, "command_fallback", cmd_word, arguments);
}

fn callCharacterDispatch(ch: *cdb.char_data, comptime fn_name: [:0]const u8, first_arg: [*:0]const u8, second_arg: ?[*:0]const u8) bool {
    if (!initialized) return false;
    const lua = lua_state orelse return false;
    const top = lua.getTop();

    if (lua.getGlobal("dbat") != .table) { lua.setTop(top); return false; }
    if (lua.getField(-1, "characters") != .table) { lua.setTop(top); return false; }
    if (lua.getField(-1, fn_name) != .function) { lua.setTop(top); return false; }
    lua.remove(-2); // characters table
    lua.remove(-2); // dbat table

    characters_lua.pushCharacter(lua, ch.id);
    _ = lua.pushString(std.mem.span(first_arg));
    const nargs: u8 = if (second_arg) |s| blk: {
        _ = lua.pushString(std.mem.span(s));
        break :blk 3;
    } else 2;

    lua.protectedCall(.{ .args = nargs, .results = 1 }) catch {
        const msg = lua.toString(-1) catch "unknown error";
        std.log.err("callCharacterDispatch({s}): {s}", .{ fn_name, msg });
        lua.setTop(top);
        return false;
    };
    const result = if (lua.isBoolean(-1)) lua.toBoolean(-1) else !lua.isNoneOrNil(-1);
    lua.setTop(top);
    return result;
}

fn spanNonEmpty(value: ?[*:0]const u8) ?[]const u8 {
    const ptr = value orelse return null;
    const text = std.mem.span(ptr);
    if (text.len == 0) return null;
    return text;
}

fn callCommandGate(ch: *cdb.char_data, command_index: i32, comptime field_name: [:0]const u8, slug: []const u8) bool {
    const lua = lua_state.?;
    if (lua.getField(command_index, field_name) != .function) {
        lua.pop(1);
        return true;
    }
    characters_lua.pushCharacter(lua, ch.id);
    lua.protectedCall(.{ .args = 1, .results = 1 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("command {s}.{s} failed: {s}", .{ slug, field_name, message });
        lua.pop(1);
        return false;
    };
    defer lua.pop(1);
    if (lua.isBoolean(-1)) return lua.toBoolean(-1);
    return !lua.isNoneOrNil(-1);
}

fn commandAlias(command_index: i32, input: []const u8) ![]const u8 {
    const lua = lua_state.?;
    if (lua.getField(command_index, "aliases") != .table) {
        lua.pop(1);
        return try lowerAscii(input);
    }
    defer lua.pop(1);

    var pos: zlua.Integer = 1;
    while (lua.getIndex(-1, pos) != .nil) : (pos += 1) {
        defer lua.pop(1);
        if (!lua.isTable(-1)) continue;
        if (aliasMatches(-1, input)) |sensitive| {
            if (sensitive) return input;
            return try lowerAscii(input);
        }
    }
    lua.pop(1);
    return try lowerAscii(input);
}

fn aliasMatches(index: i32, input: []const u8) ?bool {
    const alias = commandAliasName(index) orelse return null;
    const min = commandAliasMin(index) orelse alias.len;
    const sensitive = commandAliasSensitive(index);
    if (input.len < min or input.len > alias.len) return null;
    const prefix = alias[0..input.len];
    const matches = if (sensitive) std.mem.eql(u8, input, prefix) else std.ascii.eqlIgnoreCase(input, prefix);
    return if (matches) sensitive else null;
}

fn commandAliasName(index: i32) ?[]const u8 {
    const by_name = optionalStringField(index, "name");
    if (by_name) |value| return value;
    return tableString(index, 1);
}

fn commandAliasMin(index: i32) ?usize {
    if (optionalIntegerField(index, "min")) |value| return std.math.cast(usize, value) orelse null;
    const lua = lua_state.?;
    if (lua.getIndex(index, 2) == .nil) {
        lua.pop(1);
        return null;
    }
    defer lua.pop(1);
    const value = lua.toInteger(-1) catch return null;
    return std.math.cast(usize, value) orelse null;
}

fn commandAliasSensitive(index: i32) bool {
    if (optionalBoolField(index, "sensitive")) |value| return value;
    const lua = lua_state.?;
    if (lua.getIndex(index, 3) == .nil) {
        lua.pop(1);
        return false;
    }
    defer lua.pop(1);
    if (!lua.isBoolean(-1)) return false;
    return lua.toBoolean(-1);
}

fn lowerAscii(input: []const u8) ![]const u8 {
    const result = try allocator.dupe(u8, input);
    for (result) |*char| char.* = std.ascii.toLower(char.*);
    return result;
}

fn pushCommandContext(lua: *Lua, ch: *cdb.char_data, command: []const u8, alias: []const u8, arguments: []const u8) !void {
    lua.newTable();

    characters_lua.pushCharacter(lua, ch.id);
    lua.setField(-2, "ch");
    characters_lua.pushCharacter(lua, ch.id);
    lua.setField(-2, "actor");
    _ = lua.pushString(command);
    lua.setField(-2, "command");
    _ = lua.pushString(alias);
    lua.setField(-2, "alias");
    _ = lua.pushString(arguments);
    lua.setField(-2, "arguments");
    try pushArgParams(lua, arguments);
    lua.setField(-2, "argparams");
}

fn pushArgParams(lua: *Lua, arguments: []const u8) !void {
    lua.newTable();
    _ = lua.pushString(arguments);
    lua.setField(-2, "raw");

    if (std.mem.indexOfScalar(u8, arguments, '=')) |equals_index| {
        lua.pushBoolean(true);
        lua.setField(-2, "equals");
        const left = arguments[0..equals_index];
        const right = arguments[equals_index + 1 ..];
        _ = lua.pushString(left);
        lua.setField(-2, "lsargs");
        _ = lua.pushString(right);
        lua.setField(-2, "rsargs");
        try pushTokens(lua, left);
        lua.setField(-2, "left_tokens");
        try pushTokens(lua, right);
        lua.setField(-2, "right_tokens");
    } else {
        lua.pushBoolean(false);
        lua.setField(-2, "equals");
        _ = lua.pushString(arguments);
        lua.setField(-2, "lsargs");
        _ = lua.pushString("");
        lua.setField(-2, "rsargs");
        try pushTokens(lua, arguments);
        lua.setField(-2, "left_tokens");
        lua.newTable();
        lua.setField(-2, "right_tokens");
    }

    try pushTokens(lua, arguments);
    lua.setField(-2, "tokens");
}

fn pushTokens(lua: *Lua, text: []const u8) !void {
    lua.newTable();
    var iter = std.mem.tokenizeAny(u8, text, " \t\r\n");
    var index: zlua.Integer = 1;
    while (iter.next()) |token| : (index += 1) {
        _ = lua.pushString(token);
        lua.setIndex(-2, index);
    }
}

pub fn statDefinition(name: []const u8) ?StatDefinition {
    if (!initialized or name.len == 0) return null;
    return definition_cache.stats.get(name);
}

pub fn derivedDefinition(name: []const u8) ?DerivedDefinition {
    if (!initialized or name.len == 0) return null;
    return definition_cache.derived.get(name);
}

pub fn callLuaDerTotal(ch: *cdb.char_data, name: []const u8) ?i64 {
    if (!initialized or name.len == 0) return null;
    const lua = lua_state orelse return null;
    // Stack: [ch]
    characters_lua.pushCharacter(lua, ch.id);
    // Stack: [ch, fn]
    if (lua.getField(-1, "der_total") != .function) {
        lua.pop(2);
        return null;
    }
    // Stack: [ch, fn, ch]
    lua.pushValue(-2);
    const name_z = std.heap.page_allocator.dupeZ(u8, name) catch {
        lua.pop(3);
        return null;
    };
    defer std.heap.page_allocator.free(name_z);
    // Stack: [ch, fn, ch, name] → call(args=2,results=1) → [ch, result]
    _ = lua.pushString(name_z);
    lua.protectedCall(.{ .args = 2, .results = 1 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("ch:der_total({s}) failed: {s}", .{ name, message });
        lua.pop(2);
        return null;
    };
    // Stack: [ch, result]
    const result = lua.toInteger(-1) catch null;
    lua.pop(2);
    return result;
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
    return definition_cache.meters.get(name);
}

pub fn conditionDefinition(name: []const u8) ?ConditionDefinition {
    if (!initialized or name.len == 0) return null;
    const metadata = definition_cache.conditions.get(name) orelse {
        if (!(pushThing("conditions", name) catch return null)) return null;
        defer pop(1);
        return .{
            .persistent = optionalBoolField(-1, "persistent") orelse false,
            .stackable = optionalBoolField(-1, "stackable") orelse false,
        };
    };
    return metadata.definition;
}

pub fn conditionHasLegacyAffect(name: []const u8, aff_flag: i32) bool {
    if (!initialized or name.len == 0) return false;
    const metadata = definition_cache.conditions.get(name) orelse {
        if (!(pushThing("conditions", name) catch return false)) return false;
        defer pop(1);
        const lua = lua_state.?;
        if (lua.getField(-1, "legacy_affects") != .table) {
            lua.pop(1);
            return false;
        }
        defer lua.pop(1);
        const table_index = lua.getTop();
        var pos: zlua.Integer = 1;
        while (lua.getIndex(table_index, pos) != .nil) : (pos += 1) {
            defer lua.pop(1);
            if (lua.toInteger(-1) catch null) |val| {
                if (val == aff_flag) return true;
            }
        }
        return false;
    };
    return metadata.legacy_affects.contains(aff_flag);
}

pub fn conditionHasTag(name: []const u8, tag: []const u8) bool {
    if (!initialized or name.len == 0 or tag.len == 0) return false;
    const metadata = definition_cache.conditions.get(name) orelse {
        if (!(pushThing("conditions", name) catch return false)) return false;
        defer pop(1);
        return hasStringInField(-1, "tags", tag);
    };
    return metadata.tags.contains(tag);
}

pub fn conditionsConflict(new_condition: []const u8, active_condition: []const u8) bool {
    if (!initialized or new_condition.len == 0 or active_condition.len == 0) return false;
    const metadata = definition_cache.conditions.get(new_condition) orelse {
        if (!(pushThing("conditions", new_condition) catch return false)) return false;
        defer pop(1);
        const lua = lua_state.?;
        if (lua.getField(-1, "exclusive_tags") != .table) {
            lua.pop(1);
            return false;
        }
        defer lua.pop(1);
        const table_index = lua.getTop();
        var pos: zlua.Integer = 1;
        while (lua.getIndex(table_index, pos) != .nil) : (pos += 1) {
            const tag = lua.toString(-1) catch {
                lua.pop(1);
                continue;
            };
            const found = conditionHasTag(active_condition, tag);
            lua.pop(1);
            if (found) return true;
        }
        return false;
    };
    var it = metadata.exclusive_tags.keyIterator();
    while (it.next()) |tag| if (conditionHasTag(active_condition, tag.*)) return true;
    return false;
}

pub fn callConditionHook(ch: *cdb.char_data, condition: []const u8, comptime hook_name: [:0]const u8, reason: ?[]const u8) void {
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
    const nargs: i32 = if (reason) |r| blk: {
        _ = lua.pushString(r);
        break :blk 3;
    } else 2;
    lua.protectedCall(.{ .args = nargs, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("condition {s}.{s} failed: {s}", .{ condition, hook_name, message });
        lua.pop(1);
    };
}

pub fn callConditionEventHook(ch: *cdb.char_data, condition: []const u8, event_name: []const u8) void {
    if (!initialized or condition.len == 0) return;
    if (!(pushThing("conditions", condition) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, "on_event") != .function) {
        lua.pop(1);
        return;
    }
    characters_lua.pushCharacter(lua, ch.id);
    characters_lua.pushCondition(lua, ch.id, condition);
    _ = lua.pushString(event_name);
    lua.protectedCall(.{ .args = 3, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("condition {s}.on_event({s}) failed: {s}", .{ condition, event_name, message });
        lua.pop(1);
    };
}

pub fn callCharScriptHook(ch: *cdb.char_data, script_id: []const u8, comptime hook_name: [:0]const u8, reason: ?[]const u8) void {
    if (!initialized or script_id.len == 0) return;
    if (!(pushThing("character_scripts", script_id) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, hook_name) != .function) {
        lua.pop(1);
        return;
    }
    characters_lua.pushCharacter(lua, ch.id);
    characters_lua.pushCharacterScript(lua, ch.id, script_id);
    const nargs: i32 = if (reason) |r| blk: {
        _ = lua.pushString(r);
        break :blk 3;
    } else 2;
    lua.protectedCall(.{ .args = nargs, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("char script {s}.{s} failed: {s}", .{ script_id, hook_name, message });
        lua.pop(1);
    };
}

pub fn callCharScriptEventHook(ch: *cdb.char_data, script_id: []const u8, event_name: []const u8) void {
    if (!initialized or script_id.len == 0) return;
    if (!(pushThing("character_scripts", script_id) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, "on_event") != .function) {
        lua.pop(1);
        return;
    }
    characters_lua.pushCharacter(lua, ch.id);
    characters_lua.pushCharacterScript(lua, ch.id, script_id);
    _ = lua.pushString(event_name);
    lua.protectedCall(.{ .args = 3, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("char script {s}.on_event({s}) failed: {s}", .{ script_id, event_name, message });
        lua.pop(1);
    };
}

pub fn callObjScriptHook(obj: *cdb.obj_data, script_id: []const u8, comptime hook_name: [:0]const u8, reason: ?[]const u8) void {
    if (!initialized or script_id.len == 0) return;
    if (!(pushThing("object_scripts", script_id) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, hook_name) != .function) {
        lua.pop(1);
        return;
    }
    objects_lua.pushObject(lua, cdb.obj_id_get(obj));
    objects_lua.pushObjectScript(lua, cdb.obj_id_get(obj), script_id);
    const nargs: i32 = if (reason) |r| blk: {
        _ = lua.pushString(r);
        break :blk 3;
    } else 2;
    lua.protectedCall(.{ .args = nargs, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("obj script {s}.{s} failed: {s}", .{ script_id, hook_name, message });
        lua.pop(1);
    };
}

pub fn callObjScriptEventHook(obj: *cdb.obj_data, script_id: []const u8, event_name: []const u8) void {
    if (!initialized or script_id.len == 0) return;
    if (!(pushThing("object_scripts", script_id) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, "on_event") != .function) {
        lua.pop(1);
        return;
    }
    objects_lua.pushObject(lua, cdb.obj_id_get(obj));
    objects_lua.pushObjectScript(lua, cdb.obj_id_get(obj), script_id);
    _ = lua.pushString(event_name);
    lua.protectedCall(.{ .args = 3, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("obj script {s}.on_event({s}) failed: {s}", .{ script_id, event_name, message });
        lua.pop(1);
    };
}

pub fn callRoomScriptHook(room: *cdb.room_data, script_id: []const u8, comptime hook_name: [:0]const u8, reason: ?[]const u8) void {
    if (!initialized or script_id.len == 0) return;
    if (!(pushThing("room_scripts", script_id) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, hook_name) != .function) {
        lua.pop(1);
        return;
    }
    rooms_lua.pushRoom(lua, cdb.room_vnum_get(room));
    rooms_lua.pushRoomScript(lua, cdb.room_vnum_get(room), script_id);
    const nargs: i32 = if (reason) |r| blk: {
        _ = lua.pushString(r);
        break :blk 3;
    } else 2;
    lua.protectedCall(.{ .args = nargs, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("room script {s}.{s} failed: {s}", .{ script_id, hook_name, message });
        lua.pop(1);
    };
}

pub fn callRoomScriptEventHook(room: *cdb.room_data, script_id: []const u8, event_name: []const u8) void {
    if (!initialized or script_id.len == 0) return;
    if (!(pushThing("room_scripts", script_id) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, "on_event") != .function) {
        lua.pop(1);
        return;
    }
    rooms_lua.pushRoom(lua, cdb.room_vnum_get(room));
    rooms_lua.pushRoomScript(lua, cdb.room_vnum_get(room), script_id);
    _ = lua.pushString(event_name);
    lua.protectedCall(.{ .args = 3, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("room script {s}.on_event({s}) failed: {s}", .{ script_id, event_name, message });
        lua.pop(1);
    };
}

pub fn callZoneScriptHook(zone: *cdb.zone_data, script_id: []const u8, comptime hook_name: [:0]const u8, reason: ?[]const u8) void {
    if (!initialized or script_id.len == 0) return;
    if (!(pushThing("zone_scripts", script_id) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, hook_name) != .function) {
        lua.pop(1);
        return;
    }
    zones_lua.pushZone(lua, cdb.zone_id_get(zone));
    zones_lua.pushZoneScript(lua, cdb.zone_id_get(zone), script_id);
    const nargs: i32 = if (reason) |r| blk: {
        _ = lua.pushString(r);
        break :blk 3;
    } else 2;
    lua.protectedCall(.{ .args = nargs, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("zone script {s}.{s} failed: {s}", .{ script_id, hook_name, message });
        lua.pop(1);
    };
}

pub fn callZoneScriptEventHook(zone: *cdb.zone_data, script_id: []const u8, event_name: []const u8) void {
    if (!initialized or script_id.len == 0) return;
    if (!(pushThing("zone_scripts", script_id) catch return)) return;
    defer pop(1);

    const lua = lua_state.?;
    if (lua.getField(-1, "on_event") != .function) {
        lua.pop(1);
        return;
    }
    zones_lua.pushZone(lua, cdb.zone_id_get(zone));
    zones_lua.pushZoneScript(lua, cdb.zone_id_get(zone), script_id);
    _ = lua.pushString(event_name);
    lua.protectedCall(.{ .args = 3, .results = 0 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("zone script {s}.on_event({s}) failed: {s}", .{ script_id, event_name, message });
        lua.pop(1);
    };
}

pub fn emitCharacterModifiers(ch: *cdb.char_data, cache: *modifiers_api.ModifierCache) void {
    if (!initialized) return;
    const lua = lua_state.?;
    const top = lua.getTop();
    defer lua.setTop(top);

    characters_lua.pushCharacter(lua, ch.id);
    if (lua.getField(-1, "modifiers") != .function) {
        lua.pop(2);
        return;
    }
    lua.pushValue(-2);
    lua.protectedCall(.{ .args = 1, .results = 1 }) catch |err| {
        const message = lua.toString(-1) catch @errorName(err);
        std.log.err("ch:modifiers() failed: {s}", .{message});
        return;
    };
    defer lua.pop(1);
    if (!lua.isTable(-1)) return;

    var pos: zlua.Integer = 1;
    while (lua.getIndex(-1, pos) != .nil) : (pos += 1) {
        defer lua.pop(1);
        if (!lua.isTable(-1)) continue;
        addModifierFromLua(cache, "character", "self", -1);
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

fn addModifierFromLua(cache: *modifiers_api.ModifierCache, source_category: []const u8, source_id: []const u8, index: i32) void {
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
    const label = optionalStringField(index, "label") orelse source_id;

    cache.add(.{
        .source_category = internString(source_category),
        .source_id = internString(source_id),
        .target_category = internString(target_category),
        .target_id = internString(target_id),
        .kind = kind,
        .value = value,
        .label = internString(label),
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
    return hasStringInField(index, "tags", tag);
}

fn hasStringInField(index: i32, comptime field_name: [:0]const u8, value: []const u8) bool {
    const lua = lua_state.?;
    if (lua.getField(index, field_name) != .table) {
        lua.pop(1);
        return false;
    }
    defer lua.pop(1);

    var pos: zlua.Integer = 1;
    while (lua.getIndex(-1, pos) != .nil) : (pos += 1) {
        const item = lua.toString(-1) catch {
            lua.pop(1);
            continue;
        };
        const found = std.mem.eql(u8, item, value);
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

fn cacheReturnedValue(category: []const u8, id: []const u8, index: i32) !void {
    _ = intern_mod.intern(id);
    if (std.mem.eql(u8, category, "stats")) return cacheStatDefinition(id, index);
    if (std.mem.eql(u8, category, "derived")) return cacheDerivedDefinition(id, index);
    if (std.mem.eql(u8, category, "meters")) return cacheMeterDefinition(id, index);
    if (std.mem.eql(u8, category, "conditions")) return cacheConditionDefinition(id, index);
    if (std.mem.eql(u8, category, "attacks")) return; // full def lives in Lua registry; id already interned above
    if (std.mem.eql(u8, category, "character_scripts")) return cacheScriptDefinition(&definition_cache.character_scripts, id, index);
    if (std.mem.eql(u8, category, "object_scripts")) return cacheScriptDefinition(&definition_cache.object_scripts, id, index);
    if (std.mem.eql(u8, category, "room_scripts")) return cacheScriptDefinition(&definition_cache.room_scripts, id, index);
    if (std.mem.eql(u8, category, "zone_scripts")) return cacheScriptDefinition(&definition_cache.zone_scripts, id, index);
}

fn cacheScriptDefinition(cache: *std.StringHashMap(ScriptDefinition), id: []const u8, index: i32) !void {
    const di = lua_state.?.absIndex(index);
    try cache.put(internString(id), .{
        .persistent = optionalBoolField(di, "persistent") orelse false,
    });
}

pub fn scriptDefinition(comptime field: []const u8, id: []const u8) ?ScriptDefinition {
    return @field(definition_cache, field).get(id);
}

pub fn derivedCount() usize {
    return definition_cache.derived.count();
}

fn cacheStatDefinition(id: []const u8, index: i32) !void {
    const definition_index = lua_state.?.absIndex(index);
    try definition_cache.stats.put(internString(id), .{
        .default_value = optionalIntegerField(definition_index, "default_value") orelse 0,
        .min_value = optionalIntegerField(definition_index, "min_value"),
        .max_value = optionalIntegerField(definition_index, "max_value"),
    });
}

fn cacheDerivedDefinition(id: []const u8, index: i32) !void {
    const definition_index = lua_state.?.absIndex(index);
    var definition = DerivedDefinition{
        .min_value = optionalIntegerField(definition_index, "min_value"),
        .max_value = optionalIntegerField(definition_index, "max_value"),
    };
    if (optionalStringField(definition_index, "base_stat")) |base_stat| {
        const len = @min(base_stat.len, definition.base_stat_storage.len);
        @memcpy(definition.base_stat_storage[0..len], base_stat[0..len]);
        definition.base_stat_len = len;
    }
    definition.no_modifiers = hasTag(definition_index, "no_modifiers");
    readLegacyModifiers(definition_index, &definition);
    readModifierTargets(definition_index, &definition);
    try definition_cache.derived.put(internString(id), definition);
}

fn readModifierTargets(index: i32, definition: *DerivedDefinition) void {
    const lua = lua_state.?;
    if (lua.getField(index, "modifier_targets") != .table) {
        lua.pop(1);
        return;
    }
    defer lua.pop(1);

    var pos: zlua.Integer = 1;
    while (definition.modifier_target_count < DerivedDefinition.max_modifier_targets) : (pos += 1) {
        if (lua.getIndex(-1, pos) == .nil) {
            lua.pop(1);
            return;
        }
        defer lua.pop(1);
        if (!lua.isTable(-1)) continue;

        const target = &definition.modifier_targets[definition.modifier_target_count];

        const cat_type = lua.getIndex(-1, 1);
        defer lua.pop(1);
        if (cat_type == .string) {
            const cat = lua.toString(-1) catch continue;
            const cat_len = @min(cat.len, target.category.len);
            @memcpy(target.category[0..cat_len], cat[0..cat_len]);
            target.category_len = cat_len;
        } else continue;

        const id_type = lua.getIndex(-1, 2);
        defer lua.pop(1);
        if (id_type == .string) {
            const val = lua.toString(-1) catch continue;
            const val_len = @min(val.len, target.id.len);
            @memcpy(target.id[0..val_len], val[0..val_len]);
            target.id_len = val_len;
        } else continue;

        definition.modifier_target_count += 1;
    }
}

fn cacheMeterDefinition(id: []const u8, index: i32) !void {
    const definition_index = lua_state.?.absIndex(index);
    var definition = MeterDefinition{};
    const derived_stat = optionalStringField(definition_index, "derived_stat") orelse optionalStringField(definition_index, "derived");
    if (derived_stat) |value| {
        const len = @min(value.len, definition.derived_stat_storage.len);
        @memcpy(definition.derived_stat_storage[0..len], value[0..len]);
        definition.derived_stat_len = len;
    }
    if (optionalStringField(definition_index, "linked_condition")) |value| {
        const len = @min(value.len, definition.linked_condition_storage.len);
        @memcpy(definition.linked_condition_storage[0..len], value[0..len]);
        definition.linked_condition_len = len;
    }
    try definition_cache.meters.put(internString(id), definition);
}

fn cacheConditionDefinition(id: []const u8, index: i32) !void {
    const definition_index = lua_state.?.absIndex(index);
    var metadata = ConditionMetadata.init(allocator, .{
        .persistent = optionalBoolField(definition_index, "persistent") orelse false,
        .stackable = optionalBoolField(definition_index, "stackable") orelse false,
    });
    errdefer metadata.deinit();
    try cacheStringSet(definition_index, "tags", &metadata.tags);
    try cacheStringSet(definition_index, "exclusive_tags", &metadata.exclusive_tags);
    try cacheLegacyAffectSet(definition_index, "legacy_affects", &metadata.legacy_affects);
    try definition_cache.conditions.put(internString(id), metadata);
}

fn cacheStringSet(index: i32, comptime field_name: [:0]const u8, set: *std.StringHashMap(void)) !void {
    const lua = lua_state.?;
    if (lua.getField(index, field_name) != .table) {
        lua.pop(1);
        return;
    }
    defer lua.pop(1);

    var pos: zlua.Integer = 1;
    while (lua.getIndex(-1, pos) != .nil) : (pos += 1) {
        defer lua.pop(1);
        const value = lua.toString(-1) catch continue;
        try set.put(internString(value), {});
    }
}

fn cacheLegacyAffectSet(index: i32, comptime field_name: [:0]const u8, set: *std.AutoHashMap(i32, void)) !void {
    const lua = lua_state.?;
    if (lua.getField(index, field_name) != .table) {
        lua.pop(1);
        return;
    }
    defer lua.pop(1);

    var pos: zlua.Integer = 1;
    while (lua.getIndex(-1, pos) != .nil) : (pos += 1) {
        defer lua.pop(1);
        const value = lua.toInteger(-1) catch continue;
        try set.put(@intCast(value), {});
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

    characters_lua.register(lua);
    objects_lua.register(lua);
    rooms_lua.register(lua);
    zones_lua.register(lua);
    shops_lua.register(lua);
    guilds_lua.register(lua);
    dgscripts_lua.register(lua);
    consts_lua.register(lua);

    lua.pushFunction(zlua.wrap(luaLog));
    lua.setField(-2, "log");

    lua.pushFunction(zlua.wrap(luaTime));
    lua.setField(-2, "time");

    lua.pushFunction(zlua.wrap(luaWeather));
    lua.setField(-2, "weather");

    lua.pushFunction(zlua.wrap(luaConditionHasTag));
    lua.setField(-2, "condition_has_tag");

    lua.pushFunction(zlua.wrap(luaAddCommas));
    lua.setField(-2, "add_commas");
    lua.pushFunction(zlua.wrap(luaAddCommas));
    lua.setField(-2, "format_number");

    lua.pushFunction(zlua.wrap(luaAxionDice));
    lua.setField(-2, "axion_dice");

    lua.pushFunction(zlua.wrap(luaReadObject));
    lua.setField(-2, "read_object");

    lua.pushFunction(zlua.wrap(luaSendToImm));
    lua.setField(-2, "send_to_imm");

    lua.pushFunction(zlua.wrap(luaLogImmAction));
    lua.setField(-2, "log_imm_action");

    lua.pushFunction(zlua.wrap(luaEachPlayer));
    lua.setField(-2, "each_player");

    lua.pushBoolean(cdb.config_info.play.load_into_inventory != 0);
    lua.setField(-2, "load_into_inventory");

    registerTestModule(lua);

    return 1;
}

fn registerTestModule(lua: *Lua) void {
    lua.newTable();
    lua.pushFunction(zlua.wrap(luaTestModeEnabled));
    lua.setField(-2, "mode_enabled");
    lua.pushFunction(zlua.wrap(luaTestMobProtoExists));
    lua.setField(-2, "mob_proto_exists");
    lua.pushFunction(zlua.wrap(luaTestObjProtoExists));
    lua.setField(-2, "obj_proto_exists");
    lua.pushFunction(zlua.wrap(luaTestLoadedLuaEntries));
    lua.setField(-2, "loaded_lua_entries");
    lua.setField(-2, "test");
}

fn luaTestModeEnabled(lua: *Lua) i32 {
    lua.pushBoolean(cdb.config_info.test_mode);
    return 1;
}

fn luaTestMobProtoExists(lua: *Lua) i32 {
    const vnum = lua.toInteger(1) catch {
        lua.pushBoolean(false);
        return 1;
    };
    const mob_vnum = std.math.cast(cdb.mob_vnum, vnum) orelse {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(cdb.mob_proto_by_id(mob_vnum) != null);
    return 1;
}

fn luaTestObjProtoExists(lua: *Lua) i32 {
    const vnum = lua.toInteger(1) catch {
        lua.pushBoolean(false);
        return 1;
    };
    const obj_vnum = std.math.cast(cdb.obj_vnum, vnum) orelse {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(cdb.obj_proto_by_id(obj_vnum) != null);
    return 1;
}

fn luaTestLoadedLuaEntries(lua: *Lua) i32 {
    lua.pushInteger(@intCast(loaded_entries));
    return 1;
}

fn luaAddCommas(lua: *Lua) i32 {
    const number = lua.toInteger(1) catch lua.typeError(1, "integer");
    const value = std.math.cast(i64, number) orelse lua.raiseErrorStr("number out of range", .{});
    const formatted = cdb.add_commas(value);
    if (formatted == null) {
        lua.pushNil();
    } else {
        _ = lua.pushString(std.mem.span(formatted));
    }
    return 1;
}

fn luaConditionHasTag(lua: *Lua) i32 {
    const condition = lua.toString(1) catch "";
    const tag = lua.toString(2) catch "";
    lua.pushBoolean(conditionHasTag(condition, tag));
    return 1;
}

fn luaTime(lua: *Lua) i32 {
    lua.newTable();
    lua.pushInteger(cdb.time_info.hours);
    lua.setField(-2, "hours");
    lua.pushInteger(cdb.time_info.day);
    lua.setField(-2, "day");
    lua.pushInteger(cdb.time_info.month);
    lua.setField(-2, "month");
    lua.pushInteger(cdb.time_info.year);
    lua.setField(-2, "year");
    return 1;
}

fn luaWeather(lua: *Lua) i32 {
    lua.newTable();
    lua.pushInteger(cdb.weather_info.pressure);
    lua.setField(-2, "pressure");
    lua.pushInteger(cdb.weather_info.change);
    lua.setField(-2, "change");
    lua.pushInteger(cdb.weather_info.sky);
    lua.setField(-2, "sky");
    lua.pushInteger(cdb.weather_info.sunlight);
    lua.setField(-2, "sunlight");
    return 1;
}

fn luaAxionDice(lua: *Lua) i32 {
    const adjust: c_int = if (lua.isNoneOrNil(1)) 0 else @intCast(lua.toInteger(1) catch 0);
    lua.pushInteger(cdb.axion_dice(adjust));
    return 1;
}

fn luaReadObject(lua: *Lua) i32 {
    const vnum: c_int = @intCast(lua.toInteger(1) catch lua.typeError(1, "integer"));
    const obj = cdb.read_object(vnum, 1); // 1 = VIRTUAL
    if (obj) |o| {
        objects_lua.pushObject(lua, cdb.obj_id_get(o));
    } else {
        lua.pushNil();
    }
    return 1;
}

fn luaLog(lua: *Lua) i32 {
    const message = lua.toString(1) catch "";
    std.log.info("lua: {s}", .{message});
    return 0;
}

fn luaSendToImm(lua: *Lua) i32 {
    const msg = lua.toString(1) catch return 0;
    cdb.char_send_to_imm(msg.ptr);
    return 0;
}

fn luaLogImmAction(lua: *Lua) i32 {
    const msg = lua.toString(1) catch return 0;
    cdb.char_log_imm_action(msg.ptr);
    return 0;
}

fn luaEachPlayer(lua: *Lua) i32 {
    if (!lua.isFunction(1)) lua.typeError(1, "function");
    var desc = cdb.descriptor_list;
    while (desc != null) {
        const d: *cdb.descriptor_data = @ptrCast(desc.?);
        desc = d.next;
        if (d.connected != cdb.CON_PLAYING) continue;
        const ch = d.character orelse continue;
        lua.pushValue(1);
        characters_lua.pushCharacter(lua, cdb.char_id_get(@ptrCast(ch)));
        lua.protectedCall(.{ .args = 1, .results = 0 }) catch {};
    }
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

fn loadCategoryDir(namespace: []const u8, name: []const u8, dir_path: []const u8) !void {
    var d = std.Io.Dir.cwd().openDir(io, dir_path, .{ .iterate = true }) catch |err| switch (err) {
        error.FileNotFound => return,
        else => return err,
    };
    defer d.close(io);

    var iter = d.iterate();
    while (try iter.next(io)) |entry| {
        if (entry.kind == .directory) {
            const sub = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ dir_path, entry.name });
            defer allocator.free(sub);
            try loadCategoryDir(namespace, name, sub);
        } else if (entry.kind == .file) {
            if (!std.mem.endsWith(u8, entry.name, ".lua")) continue;
            const slug = entry.name[0 .. entry.name.len - ".lua".len];
            const path = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ dir_path, entry.name });
            defer allocator.free(path);
            try loadThing(namespace, name, slug, path);
        }
    }
}

fn loadCategory(comptime namespace: []const u8, comptime name: []const u8, comptime dir: []const u8) !void {
    const dir_path = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ lua_root, dir });
    defer allocator.free(dir_path);
    try loadCategoryDir(namespace, name, dir_path);
}

fn loadThing(namespace: []const u8, category: []const u8, slug: []const u8, path: []const u8) !void {
    const lua = lua_state.?;
    lua.setTop(0);
    errdefer lua.setTop(0);

    const path_z = try allocator.dupeZ(u8, path);
    defer allocator.free(path_z);

    const load_result = switch (zlua.lang) {
        .lua51, .luajit => lua.loadFile(path_z),
        else => lua.loadFile(path_z, .text),
    };
    load_result catch |err| {
        reportLuaError(path, err);
        return err;
    };

    lua.protectedCall(.{ .results = 1 }) catch |err| {
        reportLuaError(path, err);
        return err;
    };

    // Use the `id` field from the returned table as the registry key if present,
    // so that subdirectory files with id != filename slug don't collide.
    const effective_slug = blk: {
        if (lua.getField(1, "id") == .string) {
            const id_str = try lua.toString(-1);
            const owned = try allocator.dupe(u8, id_str);
            lua.pop(1);
            break :blk owned;
        }
        lua.pop(1);
        break :blk try allocator.dupe(u8, slug);
    };
    defer allocator.free(effective_slug);

    try registerReturnedValue(namespace, category, effective_slug, path);
    loaded_entries += 1;
    lua.setTop(0);
}

fn registerReturnedValue(namespace: []const u8, category: []const u8, slug: []const u8, path: []const u8) !void {
    const lua = lua_state.?;
    try cacheReturnedValue(category, slug, 1);

    const namespace_z = try allocator.dupeZ(u8, namespace);
    defer allocator.free(namespace_z);
    const category_z = try allocator.dupeZ(u8, category);
    defer allocator.free(category_z);
    const slug_z = try allocator.dupeZ(u8, slug);
    defer allocator.free(slug_z);
    const path_z = try allocator.dupeZ(u8, path);
    defer allocator.free(path_z);

    _ = lua.getGlobal("dbat");
    _ = lua.getField(-1, "_register");
    lua.remove(-2);
    _ = lua.pushString(namespace_z);
    _ = lua.pushString(category_z);
    _ = lua.pushString(slug_z);
    _ = lua.pushString(path_z);
    lua.pushValue(1);

    lua.protectedCall(.{ .args = 5, .results = 0 }) catch |err| {
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
