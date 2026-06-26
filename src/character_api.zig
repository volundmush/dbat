const cdb = @import("cdb");
const std = @import("std");
const characters = @import("character.zig");
const bitflags = @import("flags.zig");
const obj_api = @import("object_api.zig");
const lua_api = @import("lua_api.zig");
const modifiers_api = @import("modifiers_api.zig");
const zone_mod = @import("zone.zig");
const intern_mod = @import("intern.zig");
const script_instance_mod = @import("script_instance.zig");

extern fn event_schedule_lua_char_update(fire_at: i64, interval: i64, kind: ?[*:0]const u8, char_id: i64) u64;
extern fn eq_cancel_owner(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn eq_owner_next_ms(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn event_queue_now_ms() i64;

pub const TransformData = struct {
    id: []const u8,
    numbers: std.StringHashMap(i64),
    strings: std.StringHashMap([]const u8),

    pub fn init(alloc: std.mem.Allocator, id: []const u8) !TransformData {
        return .{
            .id = lua_api.internString(id),
            .numbers = std.StringHashMap(i64).init(alloc),
            .strings = std.StringHashMap([]const u8).init(alloc),
        };
    }

    pub fn deinit(self: *TransformData, alloc: std.mem.Allocator) void {
        self.numbers.deinit();
        var string_it = self.strings.iterator();
        while (string_it.next()) |entry| {
            alloc.free(entry.value_ptr.*);
        }
        self.strings.deinit();
    }
};

const ConditionNumberArg = extern struct {
    key: ?[*:0]const u8,
    value: i64,
};

const ConditionStringArg = extern struct {
    key: ?[*:0]const u8,
    value: ?[*:0]const u8,
};

const DerivedData = struct {
    value: i64,
};

const SkillData = struct {
    base: i64,
    perf: i64,
};

pub const ConditionSource = struct {
    category: []const u8,
    id: []const u8,
};

pub const ConditionInstance = struct {
    id: []const u8,
    stacks: i64 = 1,
    sources: std.array_list.Managed(ConditionSource),
    numbers: std.StringHashMap(i64),
    strings: std.StringHashMap([]const u8),

    pub fn init(alloc: std.mem.Allocator, id: []const u8) !ConditionInstance {
        return .{
            .id = lua_api.internString(id),
            .sources = std.array_list.Managed(ConditionSource).init(alloc),
            .numbers = std.StringHashMap(i64).init(alloc),
            .strings = std.StringHashMap([]const u8).init(alloc),
        };
    }

    pub fn deinit(self: *ConditionInstance, _: std.mem.Allocator) void {
        self.sources.deinit();
        self.numbers.deinit();
        self.strings.deinit();
    }
};

const meter_scale: i64 = 1_000_000;

fn condExpireKind(buf: *[192:0]u8, cond_name: []const u8) ?[:0]u8 {
    const prefix = "condition:";
    const suffix = ":expire";
    const total = prefix.len + cond_name.len + suffix.len;
    if (total >= buf.len) return null;
    @memcpy(buf[0..prefix.len], prefix);
    @memcpy(buf[prefix.len..][0..cond_name.len], cond_name);
    @memcpy(buf[prefix.len + cond_name.len ..][0..suffix.len], suffix);
    buf[total] = 0;
    return buf[0..total :0];
}

const StatId = intern_mod.InternedId;
const DerivedId = intern_mod.InternedId;

const StatStorage = struct {
    cache: std.AutoHashMap(StatId, i64),

    fn init(alloc: std.mem.Allocator) StatStorage {
        return .{ .cache = std.AutoHashMap(StatId, i64).init(alloc) };
    }

    fn deinit(self: *StatStorage) void {
        self.cache.deinit();
    }

    fn get(self: *const StatStorage, id: StatId) ?i64 {
        return self.cache.get(id);
    }

    fn set(self: *StatStorage, id: StatId, value: i64) void {
        self.cache.put(id, value) catch @panic("stat OOM");
    }

    fn clear(self: *StatStorage) void {
        self.cache.clearRetainingCapacity();
    }

    fn copyFrom(self: *StatStorage, other: *const StatStorage) void {
        self.cache.clearRetainingCapacity();
        var iter = other.cache.iterator();
        while (iter.next()) |kv| {
            self.cache.put(kv.key_ptr.*, kv.value_ptr.*) catch @panic("stat OOM");
        }
    }
};

fn statId(name: []const u8) ?StatId {
    return intern_mod.lookup(name);
}

const DerivedStorage = struct {
    cache: std.AutoHashMap(DerivedId, DerivedData),

    fn init(alloc: std.mem.Allocator) DerivedStorage {
        return .{ .cache = std.AutoHashMap(DerivedId, DerivedData).init(alloc) };
    }

    fn deinit(self: *DerivedStorage) void {
        self.cache.deinit();
    }

    fn get(self: *const DerivedStorage, id: DerivedId) ?DerivedData {
        return self.cache.get(id);
    }

    fn set(self: *DerivedStorage, id: DerivedId, value: DerivedData) void {
        self.cache.put(id, value) catch {};
    }

    fn clear(self: *DerivedStorage) void {
        self.cache.clearRetainingCapacity();
    }
};

fn derivedId(name: []const u8) ?DerivedId {
    return intern_mod.lookup(name);
}

pub const ScriptInstance = script_instance_mod.ScriptInstance;
const ScriptId = intern_mod.InternedId;

pub const CharacterData = struct {
    stats: StatStorage,
    deriveds: DerivedStorage,
    modifiers: modifiers_api.ModifierCache,
    deriveds_dirty: bool,
    modifier_gen: u64,
    transforms: std.AutoHashMap(intern_mod.InternedId, TransformData),
    meters: std.AutoHashMap(intern_mod.InternedId, i64),
    skills: std.AutoHashMap(intern_mod.InternedId, SkillData),
    conditions: std.AutoHashMap(intern_mod.InternedId, ConditionInstance),
    scripts: std.AutoHashMap(ScriptId, ScriptInstance),

    pub fn init(alloc: std.mem.Allocator) CharacterData {
        return CharacterData{
            .stats = StatStorage.init(alloc),
            .deriveds = DerivedStorage.init(alloc),
            .modifiers = modifiers_api.ModifierCache.init(alloc),
            .deriveds_dirty = true,
            .modifier_gen = 0,
            .transforms = std.AutoHashMap(intern_mod.InternedId, TransformData).init(alloc),
            .meters = std.AutoHashMap(intern_mod.InternedId, i64).init(alloc),
            .skills = std.AutoHashMap(intern_mod.InternedId, SkillData).init(alloc),
            .conditions = std.AutoHashMap(intern_mod.InternedId, ConditionInstance).init(alloc),
            .scripts = std.AutoHashMap(ScriptId, ScriptInstance).init(alloc),
        };
    }

    pub fn deinit(self: *CharacterData) void {
        self.stats.deinit();
        self.modifiers.deinit();
        var transforms = self.transforms.iterator();
        while (transforms.next()) |entry| {
            entry.value_ptr.deinit(std.heap.page_allocator);
        }
        self.transforms.deinit();
        self.meters.deinit();
        self.skills.deinit();
        var conditions = self.conditions.iterator();
        while (conditions.next()) |entry| {
            entry.value_ptr.deinit(std.heap.page_allocator);
        }
        self.conditions.deinit();
        var scripts = self.scripts.iterator();
        while (scripts.next()) |entry| {
            entry.value_ptr.deinit(std.heap.page_allocator);
        }
        self.scripts.deinit();
        self.deriveds.deinit();
    }
};

pub const CharacterStatEntry = struct {
    name: []const u8,
    value: i64,
};

pub const CharStatIterator = struct {
    inner: std.AutoHashMap(StatId, i64).Iterator,

    pub fn next(self: *CharStatIterator) ?CharacterStatEntry {
        const kv = self.inner.next() orelse return null;
        return .{ .name = intern_mod.nameOf(kv.key_ptr.*), .value = kv.value_ptr.* };
    }
};

pub fn characterStatIterator(ch: *const cdb.char_data) ?CharStatIterator {
    const ptr = ch.zigdata orelse return null;
    const data: *const CharacterData = @ptrCast(@alignCast(ptr));
    return .{ .inner = data.stats.cache.iterator() };
}

const MobProtoData = struct {
    stats: StatStorage,

    pub fn init(alloc: std.mem.Allocator) MobProtoData {
        return .{ .stats = StatStorage.init(alloc) };
    }

    pub fn deinit(self: *MobProtoData) void {
        self.stats.deinit();
    }
};

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub fn char_ensure_zigdata(ch: *cdb.char_data) ?*CharacterData {
    if (ch.zigdata == null) {
        const data = std.heap.page_allocator.create(CharacterData) catch return null;
        data.* = CharacterData.init(std.heap.page_allocator);
        ch.zigdata = data;
    }
    return @ptrCast(@alignCast(ch.zigdata.?));
}

pub export fn char_zig_free(ch: *cdb.char_data) void {
    if (ch.zigdata == null) return;
    const data: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    data.deinit();
    std.heap.page_allocator.destroy(data);
    ch.zigdata = null;
}

fn mobProtoEnsureZigdata(proto: *cdb.mob_proto_data) ?*MobProtoData {
    if (proto.zigdata == null) {
        const data = std.heap.page_allocator.create(MobProtoData) catch return null;
        data.* = MobProtoData.init(std.heap.page_allocator);
        proto.zigdata = data;
    }
    return @ptrCast(@alignCast(proto.zigdata.?));
}

pub export fn mob_proto_id_get(proto: *cdb.mob_proto_data) cdb.mob_vnum {
    return proto.id;
}
pub export fn mob_proto_id_set(proto: *cdb.mob_proto_data, id: cdb.mob_vnum) void {
    proto.id = id;
}

pub export fn mob_proto_zig_free(proto: *cdb.mob_proto_data) void {
    if (proto.zigdata == null) return;
    const data: *MobProtoData = @ptrCast(@alignCast(proto.zigdata.?));
    data.deinit();
    std.heap.page_allocator.destroy(data);
    proto.zigdata = null;
}

pub export fn mob_proto_stat_get(proto: *cdb.mob_proto_data, stat: ?[*:0]const u8) i64 {
    const name = statName(stat) orelse return 0;
    const definition = lua_api.statDefinition(name) orelse return 0;
    const id = statId(name) orelse return 0;
    if (proto.zigdata) |ptr| {
        const data: *MobProtoData = @ptrCast(@alignCast(ptr));
        if (data.stats.get(id)) |value| return value;
    }
    return definition.default_value;
}

pub export fn mob_proto_stat_set(proto: *cdb.mob_proto_data, stat: ?[*:0]const u8, value: i64) i64 {
    const name = statName(stat) orelse return 0;
    const definition = lua_api.statDefinition(name) orelse return 0;
    const id = statId(name) orelse return 0;
    const clamped = clampStat(value, definition);
    const data = mobProtoEnsureZigdata(proto) orelse return 0;
    data.stats.set(id, clamped);
    return clamped;
}

pub export fn mob_proto_stat_mod(proto: *cdb.mob_proto_data, stat: ?[*:0]const u8, mod: i64) i64 {
    return mob_proto_stat_set(proto, stat, mob_proto_stat_get(proto, stat) + mod);
}

pub export fn mob_proto_stats_copy_to_char(proto: *cdb.mob_proto_data, ch: *cdb.char_data) void {
    const data = char_ensure_zigdata(ch) orelse return;
    data.stats.clear();
    if (proto.zigdata) |ptr| {
        const proto_data: *MobProtoData = @ptrCast(@alignCast(ptr));
        data.stats.copyFrom(&proto_data.stats);
    }
    copyMobProtoLegacyStatsToChar(proto, ch);
    invalidateDeriveds(ch, data);
}

pub export fn char_stats_copy_to_mob_proto(ch: *cdb.char_data, proto: *cdb.mob_proto_data) void {
    const data = mobProtoEnsureZigdata(proto) orelse return;
    data.stats.clear();
    copyCharLegacyStatsToMobProto(ch, proto);
    if (ch.zigdata) |ptr| {
        const char_data: *CharacterData = @ptrCast(@alignCast(ptr));
        data.stats.copyFrom(&char_data.stats);
    }
}

fn copyMobProtoLegacyStatsToChar(proto: *cdb.mob_proto_data, ch: *cdb.char_data) void {
    _ = char_stat_set(ch, "strength", mob_proto_stat_get(proto, "strength"));
    _ = char_stat_set(ch, "intelligence", mob_proto_stat_get(proto, "intelligence"));
    _ = char_stat_set(ch, "wisdom", mob_proto_stat_get(proto, "wisdom"));
    _ = char_stat_set(ch, "agility", mob_proto_stat_get(proto, "agility"));
    _ = char_stat_set(ch, "constitution", mob_proto_stat_get(proto, "constitution"));
    _ = char_stat_set(ch, "speed", mob_proto_stat_get(proto, "speed"));
    _ = char_stat_set(ch, "height", mob_proto_stat_get(proto, "height"));
    _ = char_stat_set(ch, "weight", mob_proto_stat_get(proto, "weight"));
    _ = char_stat_set(ch, "money", mob_proto_stat_get(proto, "money"));
    _ = char_stat_set(ch, "alignment", mob_proto_stat_get(proto, "alignment"));
    _ = char_stat_set(ch, "experience", mob_proto_stat_get(proto, "experience"));
    _ = char_stat_set(ch, "powerlevel", mob_proto_stat_get(proto, "powerlevel"));
    _ = char_stat_set(ch, "ki", mob_proto_stat_get(proto, "ki"));
    _ = char_stat_set(ch, "stamina", mob_proto_stat_get(proto, "stamina"));
    _ = char_stat_set(ch, "armor", mob_proto_stat_get(proto, "armor"));
    _ = char_stat_set(ch, "level", mob_proto_stat_get(proto, "level"));
}

fn copyCharLegacyStatsToMobProto(ch: *cdb.char_data, proto: *cdb.mob_proto_data) void {
    _ = mob_proto_stat_set(proto, "strength", char_stat_get(ch, "strength"));
    _ = mob_proto_stat_set(proto, "intelligence", char_stat_get(ch, "intelligence"));
    _ = mob_proto_stat_set(proto, "wisdom", char_stat_get(ch, "wisdom"));
    _ = mob_proto_stat_set(proto, "agility", char_stat_get(ch, "agility"));
    _ = mob_proto_stat_set(proto, "constitution", char_stat_get(ch, "constitution"));
    _ = mob_proto_stat_set(proto, "speed", char_stat_get(ch, "speed"));
    _ = mob_proto_stat_set(proto, "height", char_stat_get(ch, "height"));
    _ = mob_proto_stat_set(proto, "weight", char_stat_get(ch, "weight"));
    _ = mob_proto_stat_set(proto, "money", char_stat_get(ch, "money"));
    _ = mob_proto_stat_set(proto, "alignment", char_stat_get(ch, "alignment"));
    _ = mob_proto_stat_set(proto, "experience", char_stat_get(ch, "experience"));
    _ = mob_proto_stat_set(proto, "powerlevel", char_stat_get(ch, "powerlevel"));
    _ = mob_proto_stat_set(proto, "ki", char_stat_get(ch, "ki"));
    _ = mob_proto_stat_set(proto, "stamina", char_stat_get(ch, "stamina"));
    _ = mob_proto_stat_set(proto, "armor", char_stat_get(ch, "armor"));
    _ = mob_proto_stat_set(proto, "level", char_stat_get(ch, "level"));
}

pub export fn char_id_get(ch: *cdb.char_data) i64 {
    return ch.id;
}

pub export fn char_id_set(ch: *cdb.char_data, id: i64) void {
    ch.id = @intCast(id);
}

pub export fn char_proto_id_get(ch: *cdb.char_data) cdb.mob_vnum {
    return char_vnum_get(ch);
}

pub export fn char_proto_id_set(ch: *cdb.char_data, vnum: cdb.mob_vnum) void {
    char_vnum_set(ch, vnum);
}

pub export fn char_vnum_get(ch: *cdb.char_data) cdb.mob_vnum {
    return ch.proto_id;
}

pub export fn char_vnum_set(ch: *cdb.char_data, vnum: cdb.mob_vnum) void {
    ch.proto_id = vnum;
}

pub export fn char_room_get(ch: *cdb.char_data) [*c]cdb.room_data {
    return cdb.room_by_id(ch.in_room);
}

pub export fn char_room_vnum_get(ch: *cdb.char_data) cdb.room_vnum {
    const room = char_room_get(ch);
    if (room == null) return cdb.NOWHERE;
    return room.*.id;
}

pub export fn char_zone_get(ch: *cdb.char_data) [*c]cdb.zone_data {
    const room = char_room_get(ch);
    if (room == null) return null;
    return cdb.room_zone_get(room);
}

pub export fn char_zone_vnum_get(ch: *cdb.char_data) cdb.zone_vnum {
    const zone = char_zone_get(ch);
    if (zone == null) return cdb.NOWHERE;
    return zone.*.id;
}

pub export fn char_room_vnum_set(ch: *cdb.char_data, vnum: cdb.room_vnum) void {
    ch.in_room = cdb.room_vnum_check(vnum);
}

pub export fn char_name_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.name;
}

pub export fn char_name_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(&ch.name, value);
}

pub export fn char_description_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.description;
}

pub export fn char_description_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(&ch.description, value);
}

pub export fn char_short_description_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.short_descr;
}

pub export fn char_short_description_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(&ch.short_descr, value);
}

pub export fn char_long_description_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.long_descr;
}

pub export fn char_long_description_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(&ch.long_descr, value);
}

pub export fn char_title_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.title;
}

pub export fn char_title_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(&ch.title, value);
}

pub export fn char_class_get(ch: *cdb.char_data) c_int {
    return ch.chclass;
}

pub export fn char_class_set(ch: *cdb.char_data, chclass: c_int) void {
    ch.chclass = chclass;
}

pub export fn char_race_get(ch: *cdb.char_data) c_int {
    return ch.race;
}

pub export fn char_race_set(ch: *cdb.char_data, race: c_int) void {
    ch.race = race;
}

pub export fn char_size_get(ch: *cdb.char_data) c_int {
    return ch.size;
}

pub export fn char_size_set(ch: *cdb.char_data, size: c_int) void {
    ch.size = size;
}

pub export fn char_sex_get(ch: *cdb.char_data) c_int {
    return ch.sex;
}

pub export fn char_sex_set(ch: *cdb.char_data, sex: c_int) void {
    ch.sex = @intCast(sex);
}

pub export fn char_admlevel_get(ch: *cdb.char_data) c_int {
    return ch.admlevel;
}

pub export fn char_admlevel_set(ch: *cdb.char_data, admlevel: c_int) void {
    ch.admlevel = admlevel;
}

pub export fn char_admflagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(&ch.admflags, pos);
}

pub export fn char_admflag_toggle(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.toggle(&ch.admflags, pos);
}

pub export fn char_admflag_set(ch: *cdb.char_data, pos: c_int, value: bool) void {
    bitflags.set(&ch.admflags, pos, value);
}

pub export fn char_plrflagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(ch.act[0..], pos);
}

pub export fn char_plrflag_toggle(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.toggle(ch.act[0..], pos);
}

pub export fn char_plrflag_set(ch: *cdb.char_data, pos: c_int, value: bool) void {
    bitflags.set(ch.act[0..], pos, value);
}

pub export fn char_prfflagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(ch.pref[0..], pos);
}

pub export fn char_prfflag_toggle(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.toggle(ch.pref[0..], pos);
}

pub export fn char_prfflag_set(ch: *cdb.char_data, pos: c_int, value: bool) void {
    bitflags.set(ch.pref[0..], pos, value);
}

pub export fn char_affflagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(ch.affected_by[0..], pos);
}

pub export fn char_bodyflagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(ch.bodyparts[0..], pos);
}

pub export fn char_user_get(ch: *cdb.char_data) [*c]const u8 {
    const desc = ch.desc;
    if (desc == null) return null;
    return desc.*.user;
}

pub export fn char_inventory_iterate(ch: *cdb.char_data, recursive: bool, func: ?obj_api.ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    var count: usize = 0;
    const ids = characters.char_inventory_ids(ch, &count) orelse return;
    defer characters.char_inventory_ids_free(ids);
    for (ids[0..count]) |id| {
        const obj = cdb.obj_by_id(id) orelse continue;
        if (!callback(obj, ctx)) return;
        if (recursive) _ = obj_api.objContentsIterate(obj, true, callback, ctx);
    }
}

pub export fn char_equipment_iterate(ch: *cdb.char_data, recursive: bool, func: ?obj_api.ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    for (ch.equipment) |obj| {
        if (obj == null) continue;
        if (!callback(&obj.*, ctx)) return;
        if (recursive and !obj_api.objContentsIterate(obj, true, callback, ctx)) return;
    }
}

pub export fn char_inventory_count(ch: *cdb.char_data, recursive: bool) usize {
    if (!recursive) return characters.char_inventory_count_simple(ch);
    var count: usize = 0;
    var ids_count: usize = 0;
    const ids = characters.char_inventory_ids(ch, &ids_count) orelse return 0;
    defer characters.char_inventory_ids_free(ids);
    for (ids[0..ids_count]) |id| {
        count += 1;
        const obj = cdb.obj_by_id(id) orelse continue;
        count += obj_api.obj_inventory_count(obj, true);
    }
    return count;
}

pub export fn char_equipment_count(ch: *cdb.char_data, recursive: bool) usize {
    var count: usize = 0;
    for (ch.equipment) |obj| {
        if (obj == null) continue;
        count += 1;
        if (recursive) count += obj_api.obj_inventory_count(&obj.*, true);
    }
    return count;
}

pub export fn char_inventory_get(ch: *cdb.char_data, count: *usize) ?[*]i64 {
    return characters.char_inventory_ids(ch, count);
}

pub export fn char_equipment_get(ch: *cdb.char_data, pos: usize) [*c]cdb.obj_data {
    if (pos >= ch.equipment.len) return null;
    return ch.equipment[pos];
}

fn replaceString(field: *[*c]u8, value: ?[*:0]const u8) void {
    const new_value = if (value) |new_string| strdup(new_string) orelse return else null;
    if (field.* != null) std.c.free(field.*);
    field.* = new_value;
}

pub export fn char_stat_get(ch: *cdb.char_data, stat: ?[*:0]const u8) i64 {
    const name = statName(stat) orelse return 0;
    return charStatGetName(ch, name);
}

fn charStatGetName(ch: *cdb.char_data, name: []const u8) i64 {
    const definition = lua_api.statDefinition(name) orelse return 0;
    const id = statId(name) orelse return 0;
    if (ch.zigdata) |ptr| {
        const zigdata: *CharacterData = @ptrCast(@alignCast(ptr));
        if (zigdata.stats.get(id)) |value| return value;
    }
    return definition.default_value;
}

pub export fn char_stat_set(ch: *cdb.char_data, stat: ?[*:0]const u8, value: i64) i64 {
    const name = statName(stat) orelse return 0;
    const definition = lua_api.statDefinition(name) orelse return 0;
    const id = statId(name) orelse return 0;
    const clamped = clampStat(value, definition);

    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    zigdata.stats.set(id, clamped);
    invalidateDeriveds(ch, zigdata);
    return clamped;
}

pub export fn char_stat_mod(ch: *cdb.char_data, stat: ?[*:0]const u8, mod: i64) i64 {
    const value = char_stat_get(ch, stat) + mod;
    return char_stat_set(ch, stat, value);
}

fn statName(stat: ?[*:0]const u8) ?[]const u8 {
    const ptr = stat orelse return null;
    const name = std.mem.span(ptr);
    if (name.len == 0) return null;
    return name;
}

fn clampStat(value: i64, definition: lua_api.StatDefinition) i64 {
    var result = value;
    if (definition.min_value) |min| result = @max(result, min);
    if (definition.max_value) |max| result = @min(result, max);
    return result;
}

pub export fn char_der_base_get(ch: *cdb.char_data, stat: ?[*:0]const u8) i64 {
    const name = statName(stat) orelse return 0;
    return charDerGetBaseName(ch, name);
}

fn charDerGetBaseName(ch: *cdb.char_data, name: []const u8) i64 {
    const definition = lua_api.derivedDefinition(name) orelse return 0;
    if (lua_api.calculateDerivedBase(ch, name)) |value| return value;
    return charStatGetName(ch, definition.baseStat(name));
}

pub export fn char_der_total_get(ch: *cdb.char_data, stat: ?[*:0]const u8) i64 {
    const name = statName(stat) orelse return 0;
    if (lua_api.callLuaDerTotal(ch, name)) |v| return v;
    if (lua_api.isInitialized()) return 0;
    return charDerGetTotalName(ch, name);
}

fn charDerGetTotalName(ch: *cdb.char_data, name: []const u8) i64 {
    const definition = lua_api.derivedDefinition(name) orelse return 0;
    const id = derivedId(name) orelse return 0;
    const zigdata = char_ensure_zigdata(ch) orelse return 0;

    if (!zigdata.deriveds_dirty) {
        if (zigdata.deriveds.get(id)) |cached| return cached.value;
    }

    if (zigdata.modifiers.dirty) {
        zigdata.modifiers.rebuild(ch);
        emitConditionModifiers(ch, zigdata);
    }
    const total = calculateDerivedTotal(ch, zigdata, name, definition);
    cacheDerived(zigdata, id, total);
    return total;
}

pub export fn char_der_invalidate(ch: *cdb.char_data) void {
    const zigdata = char_ensure_zigdata(ch) orelse return;
    invalidateDeriveds(ch, zigdata);
}

pub export fn char_modifier_gen_get(ch: *cdb.char_data) u64 {
    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    return zigdata.modifier_gen;
}

pub fn accumulateDerivedModifiers(cache: *modifiers_api.ModifierCache, category: []const u8, id: []const u8, flat: *i64, percent: *i64, multipliers: *std.ArrayListUnmanaged(i64), min_override: *?i64, max_override: *?i64, set_override: *?i64) void {
    if (cache.modifiersFor(category, id)) |modifiers| {
        for (modifiers) |modifier| {
            switch (modifier.kind) {
                .flat => flat.* += modifier.value,
                .percent => percent.* += modifier.value,
                .multiplier => multipliers.append(std.heap.page_allocator, modifier.value) catch {},
                .override_min => min_override.* = if (min_override.*) |current| @max(current, modifier.value) else modifier.value,
                .override_max => max_override.* = if (max_override.*) |current| @min(current, modifier.value) else modifier.value,
                .set => set_override.* = modifier.value,
            }
        }
    }
}

fn calculateDerivedTotal(ch: *cdb.char_data, zigdata: *CharacterData, name: []const u8, definition: lua_api.DerivedDefinition) i64 {
    var value = charDerGetBaseName(ch, name);
    var flat: i64 = 0;
    var percent: i64 = 0;
    var multipliers: std.ArrayListUnmanaged(i64) = .empty;
    var min_override: ?i64 = null;
    var max_override: ?i64 = null;
    var set_override: ?i64 = null;

    if (!definition.no_modifiers) {
        for (definition.legacy_modifiers[0..definition.legacy_modifier_count]) |lm| {
            flat += cdb.char_legacy_modifier(ch, lm.location, lm.specific);
        }
        accumulateDerivedModifiers(&zigdata.modifiers, "derived", name, &flat, &percent, &multipliers, &min_override, &max_override, &set_override);
        for (definition.modifier_targets[0..definition.modifier_target_count]) |target| {
            accumulateDerivedModifiers(&zigdata.modifiers, target.category[0..target.category_len], target.id[0..target.id_len], &flat, &percent, &multipliers, &min_override, &max_override, &set_override);
        }

        value += flat;
        if (percent != 0) value += @divTrunc(value * percent, modifiers_api.scale);
        for (multipliers.items) |mult| value = @divTrunc(value * mult, modifiers_api.scale);
        multipliers.deinit(std.heap.page_allocator);
    }

    if (definition.min_value) |min| value = @max(value, min);
    if (definition.max_value) |max| value = @min(value, max);
    if (min_override) |min| value = @max(value, min);
    if (max_override) |max| value = @min(value, max);
    if (set_override) |set| value = set;
    return value;
}

fn cacheDerived(zigdata: *CharacterData, id: DerivedId, value: i64) void {
    if (zigdata.deriveds_dirty) {
        clearDerivedCache(zigdata);
        zigdata.deriveds_dirty = false;
    }
    zigdata.deriveds.set(id, .{ .value = value });
}

fn invalidateDeriveds(ch: *cdb.char_data, zigdata: *CharacterData) void {
    _ = ch;
    zigdata.deriveds_dirty = true;
    zigdata.modifiers.invalidate();
    zigdata.modifier_gen +%= 1;
}

fn conditionChanged(ch: *cdb.char_data, zigdata: *CharacterData) void {
    invalidateDeriveds(ch, zigdata);
}

fn clearDerivedCache(zigdata: *CharacterData) void {
    zigdata.deriveds.clear();
}

pub export fn char_meter_get(ch: *cdb.char_data, meter: ?[*:0]const u8) i64 {
    const name = statName(meter) orelse return 0;
    const id = intern_mod.lookup(name) orelse return meter_scale;
    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    return zigdata.meters.get(id) orelse meter_scale;
}

pub export fn char_meter_full(ch: *cdb.char_data, meter: ?[*:0]const u8) bool {
    const name = statName(meter) orelse return false;
    const id = intern_mod.lookup(name) orelse return false;
    const zigdata = char_ensure_zigdata(ch) orelse return false;
    if (zigdata.meters.get(id)) |value| return value >= meter_scale;
    return false;
}

pub export fn char_meter_set(ch: *cdb.char_data, meter: ?[*:0]const u8, value: i64) i64 {
    const name = statName(meter) orelse return 0;
    const id = intern_mod.lookup(name) orelse return 0;
    const clamped = clampMeter(value);

    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    const old_value = zigdata.meters.get(id) orelse meter_scale;

    if (zigdata.meters.getPtr(id)) |existing| {
        existing.* = clamped;
    } else {
        zigdata.meters.put(id, clamped) catch return 0;
    }

    if (lua_api.meterDefinition(name)) |def| {
        if (def.linkedCondition()) |linked| {
            if (old_value >= meter_scale and clamped < meter_scale) {
                if (!char_condition_has(ch, linked.ptr))
                    _ = char_condition_apply(ch, linked.ptr, "regen", "meter_depleted");
            } else if (old_value < meter_scale and clamped >= meter_scale) {
                if (char_condition_has(ch, linked.ptr))
                    _ = char_condition_remove(ch, linked.ptr, "meter_full");
            }
        }
    }

    return clamped;
}

pub export fn char_meter_conditions_sync(ch: *cdb.char_data) void {
    const zigdata = char_ensure_zigdata(ch) orelse return;
    var it = lua_api.definition_cache.meters.iterator();
    while (it.next()) |entry| {
        const def = entry.value_ptr;
        const linked = def.linkedCondition() orelse continue;
        const id = intern_mod.lookup(entry.key_ptr.*) orelse continue;
        const current_pct = zigdata.meters.get(id) orelse meter_scale;
        if (current_pct < meter_scale) {
            if (!char_condition_has(ch, linked.ptr))
                _ = char_condition_apply(ch, linked.ptr, "regen", "meter_sync");
        } else {
            if (char_condition_has(ch, linked.ptr))
                _ = char_condition_remove(ch, linked.ptr, "meter_full");
        }
    }
}

pub export fn char_meter_mod(ch: *cdb.char_data, meter: ?[*:0]const u8, mod: i64) i64 {
    const value = char_meter_get(ch, meter) + mod;
    return char_meter_set(ch, meter, value);
}

pub export fn char_meter_set_int(ch: *cdb.char_data, meter: ?[*:0]const u8, value: i64) i64 {
    const name = statName(meter) orelse return 0;
    const max = charMeterMaxName(ch, name);
    if (max <= 0) return char_meter_set(ch, meter, 0);
    return char_meter_set(ch, meter, @divTrunc(value * meter_scale, max));
}

pub export fn char_meter_mod_int(ch: *cdb.char_data, meter: ?[*:0]const u8, mod: i64) i64 {
    const name = statName(meter) orelse return 0;
    return char_meter_set_int(ch, meter, charMeterCurrentName(ch, name) + mod);
}

pub export fn char_meter_current(ch: *cdb.char_data, meter: ?[*:0]const u8) i64 {
    const name = statName(meter) orelse return 0;
    return charMeterCurrentName(ch, name);
}

pub export fn char_meter_max(ch: *cdb.char_data, meter: ?[*:0]const u8) i64 {
    const name = statName(meter) orelse return 0;
    return charMeterMaxName(ch, name);
}

fn charMeterCurrentName(ch: *cdb.char_data, name: []const u8) i64 {
    const max = charMeterMaxName(ch, name);
    if (max <= 0) return 0;
    const id = intern_mod.lookup(name) orelse return 0;
    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    const current = zigdata.meters.get(id) orelse meter_scale;
    return @divTrunc(current * max, meter_scale);
}

fn charMeterMaxName(ch: *cdb.char_data, name: []const u8) i64 {
    const definition = lua_api.meterDefinition(name) orelse return 0;
    return charDerGetTotalName(ch, definition.derivedStat(name));
}

fn clampMeter(value: i64) i64 {
    return @min(@max(value, 0), meter_scale);
}

pub export fn char_skill_base_get(ch: *cdb.char_data, skill: ?[*:0]const u8) i64 {
    const index = skillIndex(skill) orelse return 0;
    return ch.skills[index].base;
}

pub export fn char_skill_base_set(ch: *cdb.char_data, skill: ?[*:0]const u8, value: i64) i64 {
    const index = skillIndex(skill) orelse return 0;
    const clamped = clampSkillByte(value);
    ch.skills[index].base = clamped;
    return clamped;
}

pub export fn char_skill_base_mod(ch: *cdb.char_data, skill: ?[*:0]const u8, mod: i64) i64 {
    const value = char_skill_base_get(ch, skill) + mod;
    return char_skill_base_set(ch, skill, value);
}

pub export fn char_skill_modifier_get(ch: *cdb.char_data, skill: ?[*:0]const u8) i64 {
    const index = skillIndex(skill) orelse return 0;
    const bonus = cdb.char_legacy_modifier(ch, cdb.APPLY_SKILL, @intCast(index));
    return bonus;
}

pub export fn char_skill_total_get(ch: *cdb.char_data, skill: ?[*:0]const u8) i64 {
    const index = skillIndex(skill) orelse return 0;
    const bonus = cdb.char_legacy_modifier(ch, cdb.APPLY_SKILL, @intCast(index));
    return @as(i64, ch.skills[index].base + bonus);
}

pub export fn char_skill_perf_get(ch: *cdb.char_data, skill: ?[*:0]const u8) i64 {
    const index = skillIndex(skill) orelse return 0;
    return ch.skills[index].perf;
}

pub export fn char_skill_perf_set(ch: *cdb.char_data, skill: ?[*:0]const u8, value: i64) i64 {
    const index = skillIndex(skill) orelse return 0;
    const clamped = clampSkillByte(value);
    ch.skills[index].perf = clamped;
    return clamped;
}

pub export fn char_skill_perf_mod(ch: *cdb.char_data, skill: ?[*:0]const u8, mod: i64) i64 {
    const value = char_skill_perf_get(ch, skill) + mod;
    return char_skill_perf_set(ch, skill, value);
}

fn skillIndex(skill: ?[*:0]const u8) ?usize {
    const name = statName(skill) orelse return null;
    var index: usize = 0;
    while (index < cdb.SKILL_TABLE_SIZE) : (index += 1) {
        const skill_name = cdb.spell_info[index].name;
        if (skill_name == null) continue;
        if (std.ascii.eqlIgnoreCase(name, std.mem.span(skill_name))) return index;
    }
    return null;
}

fn clampSkillByte(value: i64) i8 {
    return std.math.cast(i8, @min(@max(value, std.math.minInt(i8)), std.math.maxInt(i8))).?;
}

pub export fn char_condition_check_legacy_affect(ch: *cdb.char_data, aff_flag: c_int) bool {
    if (ch.zigdata == null) return false;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var it = zigdata.conditions.keyIterator();
    while (it.next()) |id_ptr| {
        if (lua_api.conditionHasLegacyAffect(intern_mod.nameOf(id_ptr.*), @intCast(aff_flag))) return true;
    }
    return false;
}

pub export fn char_condition_has(ch: *cdb.char_data, condition: ?[*:0]const u8) bool {
    const name = statName(condition) orelse return false;
    const id = intern_mod.lookup(name) orelse return false;
    if (ch.zigdata == null) return false;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    return zigdata.conditions.contains(id);
}

pub export fn char_condition_id_has_tag(condition: ?[*:0]const u8, tag: ?[*:0]const u8) bool {
    const condition_name = statName(condition) orelse return false;
    const tag_name = statName(tag) orelse return false;
    return lua_api.conditionHasTag(condition_name, tag_name);
}

pub export fn char_condition_has_tag(ch: *cdb.char_data, tag: ?[*:0]const u8) bool {
    return char_condition_active_with_tag(ch, tag) != null;
}

pub export fn char_condition_active_with_tag(ch: *cdb.char_data, tag: ?[*:0]const u8) [*c]const u8 {
    const tag_name = statName(tag) orelse return null;
    if (ch.zigdata == null) return null;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var it = zigdata.conditions.keyIterator();
    while (it.next()) |id_ptr| {
        const cname = intern_mod.nameOf(id_ptr.*);
        if (lua_api.conditionHasTag(cname, tag_name)) return @ptrCast(cname.ptr);
    }
    return null;
}

pub export fn char_condition_add(ch: *cdb.char_data, condition: ?[*:0]const u8, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8) bool {
    return char_condition_add_with_variables(ch, condition, source_category, source_id, null, 0, null, 0);
}

pub export fn char_condition_add_with_variables(ch: *cdb.char_data, condition: ?[*:0]const u8, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8, numbers: ?[*]const ConditionNumberArg, number_count: usize, strings: ?[*]const ConditionStringArg, string_count: usize) bool {
    const name = statName(condition) orelse return false;
    const definition = lua_api.conditionDefinition(name) orelse return false;
    const cond_id = intern_mod.intern(name);
    const zigdata = char_ensure_zigdata(ch) orelse return false;
    const is_new = !zigdata.conditions.contains(cond_id);

    if (is_new) {
        var instance = ConditionInstance.init(std.heap.page_allocator, name) catch {
            return false;
        };
        zigdata.conditions.put(cond_id, instance) catch {
            instance.deinit(std.heap.page_allocator);
            return false;
        };
    } else if (definition.stackable) {
        zigdata.conditions.getPtr(cond_id).?.stacks += 1;
    }

    if (zigdata.conditions.getPtr(cond_id)) |instance| {
        addConditionVariables(instance, numbers, number_count, strings, string_count) catch return false;
        addConditionSource(instance, source_category, source_id) catch {};
    }
    conditionChanged(ch, zigdata);
    lua_api.callConditionHook(ch, name, "on_apply", null);
    return true;
}

// Like char_condition_add but does NOT fire on_apply — use when you will set variables
// immediately after and then call char_condition_notify_applied.
pub export fn char_condition_add_silent(ch: *cdb.char_data, condition: ?[*:0]const u8, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8) bool {
    const name = statName(condition) orelse return false;
    const definition = lua_api.conditionDefinition(name) orelse return false;
    const cond_id = intern_mod.intern(name);
    const zigdata = char_ensure_zigdata(ch) orelse return false;
    const is_new = !zigdata.conditions.contains(cond_id);
    if (is_new) {
        var instance = ConditionInstance.init(std.heap.page_allocator, name) catch return false;
        zigdata.conditions.put(cond_id, instance) catch {
            instance.deinit(std.heap.page_allocator);
            return false;
        };
    } else if (definition.stackable) {
        zigdata.conditions.getPtr(cond_id).?.stacks += 1;
    }
    if (zigdata.conditions.getPtr(cond_id)) |instance| {
        addConditionSource(instance, source_category, source_id) catch {};
    }
    conditionChanged(ch, zigdata);
    return true;
}

// Fire on_apply for an already-added condition. Pair with char_condition_add_silent.
pub export fn char_condition_notify_applied(ch: *cdb.char_data, condition: ?[*:0]const u8) void {
    const name = statName(condition) orelse return;
    lua_api.callConditionHook(ch, name, "on_apply", null);
}

// Dispatch a named event to a specific condition's on_event hook.
// Idempotent if condition is not active on ch.
pub export fn char_condition_event_dispatch(ch: *cdb.char_data, condition: ?[*:0]const u8, event_name: ?[*:0]const u8) void {
    const name = statName(condition) orelse return;
    const ev = statName(event_name) orelse return;
    if (conditionGet(ch, condition) == null) return;
    lua_api.callConditionEventHook(ch, name, ev);
}

// Fire on_game_activate on every active condition on ch.
pub export fn char_condition_game_activate(ch: *cdb.char_data) void {
    if (ch.zigdata == null) return;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var names = std.array_list.Managed([:0]u8).init(std.heap.c_allocator);
    defer {
        for (names.items) |n| std.heap.c_allocator.free(n);
        names.deinit();
    }
    var it = zigdata.conditions.keyIterator();
    while (it.next()) |id_ptr| {
        names.append(std.heap.c_allocator.dupeZ(u8, intern_mod.nameOf(id_ptr.*)) catch return) catch return;
    }
    for (names.items) |name_z| {
        lua_api.callConditionHook(ch, std.mem.sliceTo(name_z, 0), "on_game_activate", null);
    }
}

// Fire on_game_deactivate on every active condition on ch.
pub export fn char_condition_game_deactivate(ch: *cdb.char_data) void {
    if (ch.zigdata == null) return;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var names = std.array_list.Managed([:0]u8).init(std.heap.c_allocator);
    defer {
        for (names.items) |n| std.heap.c_allocator.free(n);
        names.deinit();
    }
    var it = zigdata.conditions.keyIterator();
    while (it.next()) |id_ptr| {
        names.append(std.heap.c_allocator.dupeZ(u8, intern_mod.nameOf(id_ptr.*)) catch return) catch return;
    }
    for (names.items) |name_z| {
        lua_api.callConditionHook(ch, std.mem.sliceTo(name_z, 0), "on_game_deactivate", null);
    }
}

pub export fn char_condition_apply(ch: *cdb.char_data, condition: ?[*:0]const u8, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8) bool {
    return char_condition_apply_with_variables(ch, condition, source_category, source_id, null, 0, null, 0);
}

pub export fn char_condition_apply_with_variables(ch: *cdb.char_data, condition: ?[*:0]const u8, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8, numbers: ?[*]const ConditionNumberArg, number_count: usize, strings: ?[*]const ConditionStringArg, string_count: usize) bool {
    const name = statName(condition) orelse return false;
    if (lua_api.conditionDefinition(name) == null) return false;
    if (ch.zigdata) |ptr| {
        const zigdata: *CharacterData = @ptrCast(@alignCast(ptr));
        var to_remove = std.array_list.Managed([:0]u8).init(std.heap.c_allocator);
        defer {
            for (to_remove.items) |item| std.heap.c_allocator.free(item);
            to_remove.deinit();
        }

        var it = zigdata.conditions.keyIterator();
        while (it.next()) |id_ptr| {
            const cname = intern_mod.nameOf(id_ptr.*);
            if (std.mem.eql(u8, cname, name)) continue;
            if (!lua_api.conditionsConflict(name, cname)) continue;
            to_remove.append(std.heap.c_allocator.dupeZ(u8, cname) catch return false) catch return false;
        }

        for (to_remove.items) |item| _ = char_condition_remove(ch, item.ptr, "exclusive");
    }
    return char_condition_add_with_variables(ch, condition, source_category, source_id, numbers, number_count, strings, string_count);
}

pub export fn char_condition_apply_with_number(ch: *cdb.char_data, condition: ?[*:0]const u8, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8, key: ?[*:0]const u8, value: i64) bool {
    const args = [_]ConditionNumberArg{.{ .key = key, .value = value }};
    return char_condition_apply_with_variables(ch, condition, source_category, source_id, &args, args.len, null, 0);
}

pub export fn char_condition_apply_with_string(ch: *cdb.char_data, condition: ?[*:0]const u8, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8, key: ?[*:0]const u8, value: ?[*:0]const u8) bool {
    const args = [_]ConditionStringArg{.{ .key = key, .value = value }};
    return char_condition_apply_with_variables(ch, condition, source_category, source_id, null, 0, &args, args.len);
}

pub export fn char_condition_apply_with_duration(ch: *cdb.char_data, condition: ?[*:0]const u8, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8, duration: i64) bool {
    const name = statName(condition) orelse return false;
    if (lua_api.conditionDefinition(name) == null) return false;
    if (ch.zigdata) |ptr| {
        const zigdata: *CharacterData = @ptrCast(@alignCast(ptr));
        var to_remove = std.array_list.Managed([:0]u8).init(std.heap.c_allocator);
        defer {
            for (to_remove.items) |item| std.heap.c_allocator.free(item);
            to_remove.deinit();
        }
        var it = zigdata.conditions.keyIterator();
        while (it.next()) |id_ptr| {
            const cname = intern_mod.nameOf(id_ptr.*);
            if (std.mem.eql(u8, cname, name)) continue;
            if (!lua_api.conditionsConflict(name, cname)) continue;
            to_remove.append(std.heap.c_allocator.dupeZ(u8, cname) catch return false) catch return false;
        }
        for (to_remove.items) |item| _ = char_condition_remove(ch, item.ptr, "exclusive");
    }
    if (!char_condition_add_silent(ch, condition, source_category, source_id)) return false;
    _ = char_condition_duration_set(ch, condition, duration);
    char_condition_notify_applied(ch, condition);
    return true;
}

pub export fn char_condition_apply_with_numbers2(ch: *cdb.char_data, condition: ?[*:0]const u8, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8, key1: ?[*:0]const u8, value1: i64, key2: ?[*:0]const u8, value2: i64) bool {
    const args = [_]ConditionNumberArg{ .{ .key = key1, .value = value1 }, .{ .key = key2, .value = value2 } };
    return char_condition_apply_with_variables(ch, condition, source_category, source_id, &args, args.len, null, 0);
}

pub export fn char_condition_remove(ch: *cdb.char_data, condition: ?[*:0]const u8, reason: ?[*:0]const u8) bool {
    const name = statName(condition) orelse return false;
    return char_condition_remove_name(ch, name, reason);
}

fn char_condition_remove_name(ch: *cdb.char_data, name: []const u8, reason: ?[*:0]const u8) bool {
    if (ch.zigdata == null) return false;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    const cond_id = intern_mod.lookup(name) orelse return false;
    var removed = zigdata.conditions.fetchRemove(cond_id) orelse return false;
    removed.value.deinit(std.heap.page_allocator);
    conditionChanged(ch, zigdata);
    var exp_buf: [192:0]u8 = undefined;
    if (condExpireKind(&exp_buf, name)) |expire_kind|
        _ = eq_cancel_owner(1, cdb.char_id_get(ch), expire_kind.ptr);
    const reason_slice: ?[]const u8 = if (reason) |r| r[0..std.mem.len(r)] else null;
    lua_api.callConditionHook(ch, name, "on_remove", reason_slice);
    return true;
}

pub export fn char_condition_remove_tag(ch: *cdb.char_data, tag: ?[*:0]const u8, reason: ?[*:0]const u8) c_int {
    const tag_name = statName(tag) orelse return 0;
    if (ch.zigdata == null) return 0;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var to_remove = std.array_list.Managed([:0]u8).init(std.heap.c_allocator);
    defer {
        for (to_remove.items) |item| std.heap.c_allocator.free(item);
        to_remove.deinit();
    }

    var it = zigdata.conditions.keyIterator();
    while (it.next()) |id_ptr| {
        const cname = intern_mod.nameOf(id_ptr.*);
        if (!lua_api.conditionHasTag(cname, tag_name)) continue;
        to_remove.append(std.heap.c_allocator.dupeZ(u8, cname) catch return 0) catch return 0;
    }

    var removed: c_int = 0;
    for (to_remove.items) |item| {
        if (char_condition_remove(ch, item.ptr, reason)) removed += 1;
    }
    return removed;
}

pub export fn char_condition_update(ch: *cdb.char_data) void {
    _ = ch;
}

pub export fn char_condition_update_with_context(ch: *cdb.char_data, kind: ?[*:0]const u8, pulses: i64, seconds: i64) void {
    _ = ch;
    _ = kind;
    _ = pulses;
    _ = seconds;
}

pub export fn char_condition_stacks_get(ch: *cdb.char_data, condition: ?[*:0]const u8) i64 {
    const instance = conditionGet(ch, condition) orelse return 0;
    return instance.stacks;
}

pub export fn char_condition_stacks_set(ch: *cdb.char_data, condition: ?[*:0]const u8, value: i64) i64 {
    const instance = conditionGet(ch, condition) orelse return 0;
    instance.stacks = @max(0, value);
    conditionChanged(ch, @ptrCast(@alignCast(ch.zigdata.?)));
    return instance.stacks;
}

pub export fn char_condition_duration_get(ch: *cdb.char_data, condition: ?[*:0]const u8) i64 {
    const name = statName(condition) orelse return -1;
    var buf: [192:0]u8 = undefined;
    const kind = condExpireKind(&buf, name) orelse return -1;
    const next_ms = eq_owner_next_ms(1, cdb.char_id_get(ch), kind.ptr);
    if (next_ms < 0) return -1;
    return @max(0, @divFloor(next_ms, 1000));
}

pub export fn char_condition_duration_set(ch: *cdb.char_data, condition: ?[*:0]const u8, value: i64) i64 {
    const name = statName(condition) orelse return -1;
    var buf: [192:0]u8 = undefined;
    const kind = condExpireKind(&buf, name) orelse return -1;
    _ = eq_cancel_owner(1, cdb.char_id_get(ch), kind.ptr);
    if (value > 0) {
        _ = event_schedule_lua_char_update(event_queue_now_ms() + value * 1000, 0, kind.ptr, cdb.char_id_get(ch));
    } else if (value == 0) {
        _ = char_condition_remove(ch, condition, "expired");
    }
    return value;
}

pub export fn char_condition_number_get(ch: *cdb.char_data, condition: ?[*:0]const u8, key: ?[*:0]const u8) i64 {
    const instance = conditionGet(ch, condition) orelse return 0;
    const name = statName(key) orelse return 0;
    return instance.numbers.get(name) orelse 0;
}

pub export fn char_condition_number_set(ch: *cdb.char_data, condition: ?[*:0]const u8, key: ?[*:0]const u8, value: i64) i64 {
    const instance = conditionGet(ch, condition) orelse return 0;
    const name = statName(key) orelse return 0;
    putOwnedNumber(instance, name, value) catch return 0;
    conditionChanged(ch, @ptrCast(@alignCast(ch.zigdata.?)));
    return value;
}

pub export fn char_condition_number_mod(ch: *cdb.char_data, condition: ?[*:0]const u8, key: ?[*:0]const u8, mod: i64) i64 {
    const value = char_condition_number_get(ch, condition, key) + mod;
    return char_condition_number_set(ch, condition, key, value);
}

pub export fn char_condition_string_get(ch: *cdb.char_data, condition: ?[*:0]const u8, key: ?[*:0]const u8) [*c]const u8 {
    const instance = conditionGet(ch, condition) orelse return null;
    const name = statName(key) orelse return null;
    const value = instance.strings.get(name) orelse return null;
    return @ptrCast(value.ptr);
}

pub export fn char_condition_string_set(ch: *cdb.char_data, condition: ?[*:0]const u8, key: ?[*:0]const u8, value: ?[*:0]const u8) bool {
    const instance = conditionGet(ch, condition) orelse return false;
    const name = statName(key) orelse return false;
    const text = statName(value) orelse return false;
    putOwnedString(instance, name, text) catch return false;
    conditionChanged(ch, @ptrCast(@alignCast(ch.zigdata.?)));
    return true;
}

pub export fn char_transform_has(ch: *cdb.char_data, transform: ?[*:0]const u8) bool {
    const name = statName(transform) orelse return false;
    const id = intern_mod.lookup(name) orelse return false;
    if (ch.zigdata == null) return false;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    return zigdata.transforms.contains(id);
}

pub export fn char_transform_add(ch: *cdb.char_data, transform: ?[*:0]const u8) bool {
    const name = statName(transform) orelse return false;
    const id = intern_mod.intern(name);
    const zigdata = char_ensure_zigdata(ch) orelse return false;
    if (zigdata.transforms.contains(id)) return true;

    var data = TransformData.init(std.heap.page_allocator, name) catch {
        return false;
    };
    zigdata.transforms.put(id, data) catch {
        data.deinit(std.heap.page_allocator);
        return false;
    };
    return true;
}

pub export fn char_transform_remove(ch: *cdb.char_data, transform: ?[*:0]const u8) bool {
    const name = statName(transform) orelse return false;
    const id = intern_mod.lookup(name) orelse return false;
    if (ch.zigdata == null) return false;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var removed = zigdata.transforms.fetchRemove(id) orelse return false;
    removed.value.deinit(std.heap.page_allocator);
    return true;
}

pub export fn char_transform_unlocked(ch: *cdb.char_data, transform: ?[*:0]const u8) bool {
    return char_transform_number_get(ch, transform, "unlocked") != 0;
}

pub export fn char_transform_unlock(ch: *cdb.char_data, transform: ?[*:0]const u8, source: ?[*:0]const u8) bool {
    _ = statName(transform) orelse return false;
    if (!char_transform_add(ch, transform)) return false;
    _ = char_transform_number_set(ch, transform, "unlocked", 1);
    if (statName(source)) |source_text| {
        if (source_text.len > 0) _ = char_transform_string_set(ch, transform, "unlock_source", source);
    }
    return true;
}

pub export fn char_transform_number_get(ch: *cdb.char_data, transform: ?[*:0]const u8, key: ?[*:0]const u8) i64 {
    const data = transformGet(ch, transform) orelse return 0;
    const name = statName(key) orelse return 0;
    return data.numbers.get(name) orelse 0;
}

pub export fn char_transform_number_set(ch: *cdb.char_data, transform: ?[*:0]const u8, key: ?[*:0]const u8, value: i64) i64 {
    if (!char_transform_add(ch, transform)) return 0;
    const data = transformGet(ch, transform) orelse return 0;
    const name = statName(key) orelse return 0;
    putOwnedTransformNumber(data, name, value) catch return 0;
    return value;
}

pub export fn char_transform_number_mod(ch: *cdb.char_data, transform: ?[*:0]const u8, key: ?[*:0]const u8, mod: i64) i64 {
    const value = char_transform_number_get(ch, transform, key) + mod;
    return char_transform_number_set(ch, transform, key, value);
}

pub export fn char_transform_string_get(ch: *cdb.char_data, transform: ?[*:0]const u8, key: ?[*:0]const u8) [*c]const u8 {
    const data = transformGet(ch, transform) orelse return null;
    const name = statName(key) orelse return null;
    const value = data.strings.get(name) orelse return null;
    return @ptrCast(value.ptr);
}

pub export fn char_transform_string_set(ch: *cdb.char_data, transform: ?[*:0]const u8, key: ?[*:0]const u8, value: ?[*:0]const u8) bool {
    if (!char_transform_add(ch, transform)) return false;
    const data = transformGet(ch, transform) orelse return false;
    const name = statName(key) orelse return false;
    const text = statName(value) orelse return false;
    putOwnedTransformString(data, name, text) catch return false;
    return true;
}

fn conditionGet(ch: *cdb.char_data, condition: ?[*:0]const u8) ?*ConditionInstance {
    const name = statName(condition) orelse return null;
    return conditionGetName(ch, name);
}

pub fn conditionGetByName(ch: *cdb.char_data, name: []const u8) ?*ConditionInstance {
    return conditionGetName(ch, name);
}

fn conditionGetName(ch: *cdb.char_data, name: []const u8) ?*ConditionInstance {
    if (ch.zigdata == null) return null;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    const id = intern_mod.lookup(name) orelse return null;
    return zigdata.conditions.getPtr(id);
}

fn transformGet(ch: *cdb.char_data, transform: ?[*:0]const u8) ?*TransformData {
    const name = statName(transform) orelse return null;
    if (ch.zigdata == null) return null;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    const id = intern_mod.lookup(name) orelse return null;
    return zigdata.transforms.getPtr(id);
}

fn addConditionSource(instance: *ConditionInstance, source_category: ?[*:0]const u8, source_id: ?[*:0]const u8) !void {
    const category = if (source_category) |ptr| std.mem.span(ptr) else "unknown";
    const id = if (source_id) |ptr| std.mem.span(ptr) else "unknown";
    try instance.sources.append(.{
        .category = lua_api.internString(category),
        .id = lua_api.internString(id),
    });
}

fn addConditionVariables(instance: *ConditionInstance, numbers: ?[*]const ConditionNumberArg, number_count: usize, strings: ?[*]const ConditionStringArg, string_count: usize) !void {
    if (numbers) |items| {
        for (items[0..number_count]) |item| {
            const key = statName(item.key) orelse continue;
            try putOwnedNumber(instance, key, item.value);
        }
    }
    if (strings) |items| {
        for (items[0..string_count]) |item| {
            const key = statName(item.key) orelse continue;
            const value = statName(item.value) orelse continue;
            try putOwnedString(instance, key, value);
        }
    }
}

fn putOwnedNumber(instance: *ConditionInstance, key: []const u8, value: i64) !void {
    if (instance.numbers.getPtr(key)) |existing| {
        existing.* = value;
        return;
    }
    try instance.numbers.put(lua_api.internString(key), value);
}

fn putOwnedString(instance: *ConditionInstance, key: []const u8, value: []const u8) !void {
    if (instance.strings.getPtr(key)) |existing| {
        existing.* = lua_api.internString(value);
        return;
    }
    try instance.strings.put(lua_api.internString(key), lua_api.internString(value));
}

fn putOwnedTransformNumber(data: *TransformData, key: []const u8, value: i64) !void {
    if (data.numbers.getPtr(key)) |existing| {
        existing.* = value;
        return;
    }
    try data.numbers.put(lua_api.internString(key), value);
}

fn putOwnedTransformString(data: *TransformData, key: []const u8, value: []const u8) !void {
    if (data.strings.getPtr(key)) |existing| {
        std.heap.page_allocator.free(existing.*);
        existing.* = try std.heap.page_allocator.dupeZ(u8, value);
        return;
    }
    try data.strings.put(lua_api.internString(key), try std.heap.page_allocator.dupeZ(u8, value));
}

fn emitConditionModifiers(ch: *cdb.char_data, zigdata: *CharacterData) void {
    lua_api.emitCharacterModifiers(ch, &zigdata.modifiers);
}

pub export fn char_is_npc(ch: *cdb.char_data) bool {
    return cdb.flag_test(@ptrCast(&ch.act), cdb.MOB_ISNPC) != 0;
}

pub export fn char_next_in_room_get(ch: *cdb.char_data) [*c]cdb.char_data {
    _ = ch;
    return null;
}

pub export fn char_carrying_get(ch: *cdb.char_data) [*c]cdb.obj_data {
    _ = ch;
    return null;
}

pub export fn char_is_outside(ch: *cdb.char_data) bool {
    return cdb.char_outside_roomflag(ch) and cdb.char_outside_sector_type(ch);
}

pub export fn char_sits_get(ch: *cdb.char_data) ?*cdb.obj_data {
    return ch.sits;
}

pub export fn char_sits_set(ch: *cdb.char_data, obj: ?*cdb.obj_data) void {
    ch.sits = obj;
}

pub export fn char_position_get(ch: *cdb.char_data) i64 {
    const cond = "position";
    const key = "state";
    if (conditionGet(ch, cond)) |instance| {
        return instance.numbers.get(key) orelse 0;
    }
    _ = char_condition_apply_with_number(ch, cond, cond, cond, key, 8);
    return 8;
}

pub export fn char_position_set(ch: *cdb.char_data, value: i64) void {
    const cond = "position";
    const key = "state";
    _ = char_condition_apply_with_number(ch, cond, cond, cond, key, value);
}

pub export fn char_condition_count(ch: *cdb.char_data) usize {
    const ptr = ch.zigdata orelse return 0;
    const data: *CharacterData = @ptrCast(@alignCast(ptr));
    return data.conditions.count();
}

pub export fn char_condition_name_at(ch: *cdb.char_data, index: usize) ?[*:0]const u8 {
    const ptr = ch.zigdata orelse return null;
    const data: *CharacterData = @ptrCast(@alignCast(ptr));
    var it = data.conditions.keyIterator();
    var i: usize = 0;
    while (it.next()) |id_ptr| {
        if (i == index) return @ptrCast(intern_mod.nameOf(id_ptr.*).ptr);
        i += 1;
    }
    return null;
}

pub export fn char_apply_entry_conditions(ch: *cdb.char_data) void {
    const race: c_int = cdb.char_race_get(ch);

    // Mutant mutations (genome[] values 1-10)
    if (race == cdb.RACE_MUTANT) {
        const mutation_map = [_]?[*:0]const u8{
            null, // 0 = none
            "mutation_extreme_speed", // 1
            "mutation_cell_regen", // 2
            "mutation_extreme_reflexes", // 3
            "mutation_infravision", // 4
            "mutation_natural_camo", // 5
            "mutation_limb_regen", // 6
            "mutation_poisonous", // 7
            "mutation_rubbery_body", // 8
            "mutation_innate_telepathy", // 9
            "mutation_natural_energy", // 10
        };
        for (ch.genome) |g| {
            if (g > 0) {
                const idx: usize = @intCast(g);
                if (idx < mutation_map.len) {
                    if (mutation_map[idx]) |cid|
                        _ = cdb.char_condition_apply(ch, cid, "entry", "genome");
                }
            }
        }
    }

    // Bio-Android genomes (genome[] values 1-8)
    if (race == cdb.RACE_BIO) {
        const genome_map = [_]?[*:0]const u8{
            null, // 0 = none
            "genome_human", // 1
            "genome_saiyan", // 2
            "genome_namek", // 3
            "genome_icer", // 4
            "genome_truffle", // 5
            "genome_arlian", // 6
            "genome_kai", // 7
            "genome_konatsu", // 8
        };
        for (ch.genome) |g| {
            if (g > 0) {
                const idx: usize = @intCast(g);
                if (idx < genome_map.len) {
                    if (genome_map[idx]) |cid|
                        _ = cdb.char_condition_apply(ch, cid, "entry", "genome");
                }
            }
        }
    }

    // Bonuses (0-28) and Flaws (29-51)
    const bonus_map = [_][*:0]const u8{
        "bonus_thrifty",     "bonus_prodigy",    "bonus_quick_study", "bonus_diehard",
        "bonus_brawler",     "bonus_destroyer",  "bonus_hardworker",  "bonus_healer",
        "bonus_loyal",       "bonus_brawny",     "bonus_scholarly",   "bonus_sage",
        "bonus_agile",       "bonus_quick",      "bonus_sturdy",      "bonus_thickskin",
        "bonus_recipe",      "bonus_fireproof",  "bonus_powerhit",    "bonus_healthy",
        "bonus_insomniac",   "bonus_evasive",    "bonus_wall",        "bonus_accurate",
        "bonus_leech",       "bonus_gmemory",    "bonus_soft",        "bonus_late",
        "bonus_impulse",     "flaw_sickly",      "flaw_punchingbag",  "flaw_pushover",
        "flaw_poordepth",    "flaw_thinskin",    "flaw_fireprone",    "flaw_intolerant",
        "flaw_coward",       "flaw_arrogant",    "flaw_unfocused",    "flaw_slacker",
        "flaw_slow_learner", "flaw_masochistic", "flaw_mute",         "flaw_wimp",
        "flaw_dull",         "flaw_foolish",     "flaw_clumsy",       "flaw_slow",
        "flaw_frail",        "flaw_sadistic",    "flaw_loner",        "flaw_bmemory",
    };
    for (ch.bonuses, 0..) |val, i| {
        if (val != 0 and i < bonus_map.len) {
            _ = cdb.char_condition_apply(ch, bonus_map[i], "entry", "bonus");
        }
    }

    // Arlian shell sync (AFF_SHELL = 72)
    if (bitflags.get(ch.affected_by[0..], cdb.AFF_SHELL))
        _ = cdb.char_condition_apply(ch, "arlian_shell", "entry", "shell_sync");

    // Hayasa sync (AFF_HAYASA = 57)
    if (bitflags.get(ch.affected_by[0..], cdb.AFF_HAYASA))
        _ = cdb.char_condition_apply(ch, "hayasa", "entry", "hayasa_sync");

    // Fire "activate" event so Lua can schedule recurring needs/timers
    _ = event_schedule_lua_char_update(event_queue_now_ms(), 0, "activate", cdb.char_id_get(ch));
}

// ---- Status display bindings ----

const limb_stat_names = [4][*:0]const u8{
    "limb_right_arm", "limb_left_arm", "limb_right_leg", "limb_left_leg",
};

pub export fn char_limbcond_get(ch: *cdb.char_data, n: c_int) c_int {
    if (n < 1 or n > 4) return 0;
    return @intCast(char_stat_get(ch, limb_stat_names[@as(usize, @intCast(n)) - 1]));
}

pub export fn char_limbcond_set(ch: *cdb.char_data, n: c_int, val: c_int) void {
    if (n < 1 or n > 4) return;
    _ = char_stat_set(ch, limb_stat_names[@as(usize, @intCast(n)) - 1], @intCast(val));
    char_limb_healing_sync(ch);
}

pub export fn char_limb_healing_sync(ch: *cdb.char_data) void {
    if (ch.idnum == -1) return;
    for (1..5) |i| {
        const v = char_limbcond_get(ch, @intCast(i));
        if (v > 0 and v < 100) {
            if (!char_condition_has(ch, "limb_healing"))
                _ = char_condition_apply(ch, "limb_healing", "regen", "limb_damaged");
            return;
        }
    }
    if (char_condition_has(ch, "limb_healing"))
        _ = char_condition_remove(ch, "limb_healing", "limbs_healed");
}

pub export fn char_die(ch: *cdb.char_data, killer_id: i64) void {
    const killer: ?*cdb.char_data = if (killer_id == 0) null else cdb.char_by_id(killer_id);
    cdb.die(ch, killer);
}

pub export fn char_charge_get(ch: *cdb.char_data) i64 {
    return char_condition_number_get(ch, "charge", "amount");
}
pub export fn char_charge_set(ch: *cdb.char_data, val: i64) void {
    const clamped = @max(val, 0);
    if (clamped == 0) {
        _ = char_condition_number_set(ch, "charge", "amount", 0);
        return;
    }
    if (char_condition_has(ch, "charge")) {
        _ = char_condition_number_set(ch, "charge", "amount", clamped);
    } else {
        _ = char_condition_apply_with_numbers2(ch, "charge", "combat", "external", "holding", 1, "amount", clamped);
    }
}
pub export fn char_barrier_get(ch: *cdb.char_data) i64 {
    return char_condition_number_get(ch, "barrier", "amount");
}

pub export fn char_voice_get(ch: *cdb.char_data) ?[*:0]const u8 {
    return ch.voice;
}
pub export fn char_rdisplay_get(ch: *cdb.char_data) ?[*:0]const u8 {
    return ch.rdisplay;
}
pub export fn char_feature_get(ch: *cdb.char_data) ?[*:0]const u8 {
    return ch.feature;
}
pub export fn char_feature_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(&ch.feature, value);
}

pub export fn char_rp_set(ch: *cdb.char_data, value: c_int) void {
    ch.rp = value;
}

pub export fn char_radar1_get(ch: *cdb.char_data) c_int { return @intCast(ch.radar1); }
pub export fn char_radar1_set(ch: *cdb.char_data, vnum: c_int) void { ch.radar1 = @intCast(vnum); }
pub export fn char_radar2_get(ch: *cdb.char_data) c_int { return @intCast(ch.radar2); }
pub export fn char_radar2_set(ch: *cdb.char_data, vnum: c_int) void { ch.radar2 = @intCast(vnum); }
pub export fn char_radar3_get(ch: *cdb.char_data) c_int { return @intCast(ch.radar3); }
pub export fn char_radar3_set(ch: *cdb.char_data, vnum: c_int) void { ch.radar3 = @intCast(vnum); }
pub export fn char_idnum_get(ch: *cdb.char_data) c_int { return @intCast(ch.idnum); }

pub export fn char_absorbs_get(ch: *cdb.char_data) c_int {
    return ch.absorbs;
}
pub export fn char_mimic_get(ch: *cdb.char_data) c_int {
    return ch.mimic;
}
pub export fn char_backstab_cooldown_get(ch: *cdb.char_data) c_int {
    return ch.backstabcool;
}
pub export fn char_preference_get(ch: *cdb.char_data) c_int {
    return ch.preference;
}
pub export fn char_preference_set(ch: *cdb.char_data, value: c_int) void {
    ch.preference = value;
}
pub export fn char_genome_get(ch: *cdb.char_data, slot: c_int) c_int {
    const idx: usize = @intCast(slot);
    if (idx >= ch.genome.len) return 0;
    return ch.genome[idx];
}
pub export fn char_wait_set(ch: *cdb.char_data, pulses: c_int) void {
    if (ch.desc != null) ch.wait = pulses;
}
pub export fn char_cooldown_get(ch: *cdb.char_data) c_int {
    return ch.con_cooldown;
}
pub export fn char_cooldown_set(ch: *cdb.char_data, val: c_int) void {
    ch.con_cooldown = val;
}
pub export fn char_intro_known(ch: *cdb.char_data, vict: *cdb.char_data) c_int {
    return readIntro(ch, vict);
}
pub export fn char_intro_name_get(ch: *cdb.char_data, vict: *cdb.char_data) ?[*:0]const u8 {
    return get_i_name(ch, vict);
}
pub export fn char_introd_calc(ch: *cdb.char_data) ?[*:0]u8 {
    return introd_calc(ch);
}
pub export fn char_know_skill(ch: *cdb.char_data, skill_name: ?[*:0]const u8) bool {
    const index = skillIndex(skill_name) orelse return false;
    return cdb.know_skill(ch, @intCast(index)) != 0;
}
pub export fn char_improve_skill(ch: *cdb.char_data, skill_name: ?[*:0]const u8, flag: c_int) void {
    const index = skillIndex(skill_name) orelse return;
    cdb.improve_skill(ch, @intCast(index), flag);
}
pub export fn char_aura_get(ch: *cdb.char_data) c_int {
    return ch.aura;
}
pub export fn char_aura_set(ch: *cdb.char_data, value: c_int) void {
    ch.aura = value;
}
pub export fn char_hairl_get(ch: *cdb.char_data) c_int {
    return @intCast(ch.hairl);
}
pub export fn char_hairl_set(ch: *cdb.char_data, value: c_int) void {
    ch.hairl = @truncate(value);
}
pub export fn char_hairs_get(ch: *cdb.char_data) c_int {
    return @intCast(ch.hairs);
}
pub export fn char_hairs_set(ch: *cdb.char_data, value: c_int) void {
    ch.hairs = @truncate(value);
}
pub export fn char_hairc_get(ch: *cdb.char_data) c_int {
    return @intCast(ch.hairc);
}
pub export fn char_hairc_set(ch: *cdb.char_data, value: c_int) void {
    ch.hairc = @truncate(value);
}
pub export fn char_skin_get(ch: *cdb.char_data) c_int {
    return @intCast(ch.skin);
}
pub export fn char_skin_set(ch: *cdb.char_data, value: c_int) void {
    ch.skin = @truncate(value);
}
pub export fn char_eye_get(ch: *cdb.char_data) c_int {
    return @intCast(ch.eye);
}
pub export fn char_eye_set(ch: *cdb.char_data, value: c_int) void {
    ch.eye = @truncate(value);
}
pub export fn char_distfea_get(ch: *cdb.char_data) c_int {
    return @intCast(ch.distfea);
}
pub export fn char_distfea_set(ch: *cdb.char_data, value: c_int) void {
    ch.distfea = @truncate(value);
}
pub export fn char_sleeptime_get(ch: *cdb.char_data) c_int {
    return ch.sleeptime;
}

pub export fn char_has_group(ch: *cdb.char_data) bool {
    return cdb.has_group(ch) != 0;
}
pub export fn char_soft_cap(ch: *cdb.char_data) i64 {
    return cdb.calc_soft_cap(ch);
}

// extern declarations for C functions not in zig_api.h
extern fn readIntro(ch: *cdb.char_data, vict: *cdb.char_data) c_int;
extern fn get_i_name(ch: *cdb.char_data, vict: *cdb.char_data) ?[*:0]const u8;
extern fn introd_calc(ch: *cdb.char_data) ?[*:0]u8;
extern fn carry_drop(ch: *cdb.char_data, @"type": c_int) void;
extern fn look_at_room(room: *cdb.room_data, ch: *cdb.char_data, mode: c_int) void;
extern fn find_target_room(ch: *cdb.char_data, rawroomstr: [*:0]u8) ?*cdb.room_data;

pub export fn char_bonus_flagged(ch: *cdb.char_data, n: c_int) bool {
    const idx: usize = @intCast(n);
    if (idx >= ch.bonuses.len) return false;
    return ch.bonuses[idx] != 0;
}
pub export fn char_affflag_set(ch: *cdb.char_data, flag: c_int, val: bool) void {
    bitflags.set(ch.affected_by[0..], flag, val);
}
pub export fn char_barrier_set(ch: *cdb.char_data, val: i64) void {
    const clamped = @max(val, 0);
    if (clamped == 0) {
        _ = char_condition_number_set(ch, "barrier", "amount", 0);
        return;
    }
    if (char_condition_has(ch, "barrier")) {
        _ = char_condition_number_set(ch, "barrier", "amount", clamped);
    } else {
        _ = char_condition_apply_with_number(ch, "barrier", "combat", "external", "amount", clamped);
    }
}
pub export fn char_carry_drop(ch: *cdb.char_data, @"type": c_int) void {
    carry_drop(ch, @"type");
}
pub export fn char_land(ch: *cdb.char_data) void {
    if (cdb.char_condition_has(ch, "flying")) _ = cdb.char_condition_remove(ch, "flying", "land");
}
pub export fn char_arena_idnum_get(ch: *cdb.char_data) c_int {
    return @intCast(ch.arenawatch);
}
pub export fn char_arena_idnum_set(ch: *cdb.char_data, idnum: c_int) void {
    ch.arenawatch = @intCast(idnum);
}

pub export fn char_poofin_get(ch: *cdb.char_data) ?[*:0]const u8 {
    return ch.poofin;
}
pub export fn char_poofout_get(ch: *cdb.char_data) ?[*:0]const u8 {
    return ch.poofout;
}
pub export fn char_loadroom_get(ch: *cdb.char_data) c_int {
    return @intCast(ch.load_room);
}
pub export fn char_loadroom_set(ch: *cdb.char_data, vnum: c_int) void {
    ch.load_room = @intCast(vnum);
}
pub export fn char_droom_get(ch: *cdb.char_data) c_int {
    return @intCast(ch.droom);
}
pub export fn char_droom_set(ch: *cdb.char_data, vnum: c_int) void {
    ch.droom = @intCast(vnum);
}
pub export fn char_look_at_room(ch: *cdb.char_data) void {
    const room = cdb.char_room_get(ch) orelse return;
    look_at_room(room, ch, 0);
}
pub export fn char_look_at_specific_room(ch: *cdb.char_data, room: *cdb.room_data) void {
    look_at_room(room, ch, 0);
}
pub export fn char_restore(vict: *cdb.char_data, healer: *cdb.char_data) void {
    cdb.restore_by(vict, healer);
}
pub export fn char_find_target_room(ch: *cdb.char_data, arg: ?[*:0]const u8) ?*cdb.room_data {
    if (arg == null) return null;
    var buf: [256:0]u8 = undefined;
    const src = std.mem.span(arg.?);
    const len = @min(src.len, buf.len - 1);
    @memcpy(buf[0..len], src[0..len]);
    buf[len] = 0;
    return find_target_room(ch, &buf);
}

// ---- Script C API ----

fn getScriptData(ch: *cdb.char_data) ?*CharacterData {
    const ptr = ch.zigdata orelse return null;
    return @ptrCast(@alignCast(ptr));
}

fn scriptEventKind(buf: *[192:0]u8, script_id: []const u8, event_name: []const u8) ?[:0]u8 {
    const prefix = "script:";
    const sep = ":";
    const total = prefix.len + script_id.len + sep.len + event_name.len;
    if (total >= buf.len) return null;
    @memcpy(buf[0..prefix.len], prefix);
    @memcpy(buf[prefix.len..][0..script_id.len], script_id);
    @memcpy(buf[prefix.len + script_id.len ..][0..sep.len], sep);
    @memcpy(buf[prefix.len + script_id.len + sep.len ..][0..event_name.len], event_name);
    buf[total] = 0;
    return buf[0..total :0];
}

pub export fn char_script_add(ch: *cdb.char_data, script_id: ?[*:0]const u8) bool {
    const id_z = script_id orelse return false;
    const id = std.mem.span(id_z);
    const data = getScriptData(ch) orelse return false;
    const interned = intern_mod.lookup(id) orelse return false;
    if (data.scripts.contains(interned)) return false;
    const instance = ScriptInstance.init(std.heap.page_allocator, intern_mod.nameOf(interned));
    data.scripts.put(interned, instance) catch return false;
    lua_api.callCharScriptHook(ch, id, "on_apply", null);
    return true;
}

pub export fn char_script_remove(ch: *cdb.char_data, script_id: ?[*:0]const u8, reason: ?[*:0]const u8) bool {
    const id_z = script_id orelse return false;
    const id = std.mem.span(id_z);
    const data = getScriptData(ch) orelse return false;
    const interned = intern_mod.lookup(id) orelse return false;
    if (!data.scripts.contains(interned)) return false;
    const reason_str: ?[]const u8 = if (reason) |r| std.mem.span(r) else null;
    lua_api.callCharScriptHook(ch, id, "on_remove", reason_str);
    var prefix_buf: [192:0]u8 = undefined;
    const prefix = "script:";
    const total = prefix.len + id.len;
    if (total < prefix_buf.len) {
        @memcpy(prefix_buf[0..prefix.len], prefix);
        @memcpy(prefix_buf[prefix.len..][0..id.len], id);
        prefix_buf[total] = 0;
        _ = eq_cancel_owner(cdb.EQ_OWNER_CHAR, cdb.char_id_get(ch), &prefix_buf);
    }
    if (data.scripts.fetchRemove(interned)) |kv| {
        var instance = kv.value;
        instance.deinit(std.heap.page_allocator);
    }
    return true;
}

pub export fn char_script_has(ch: *cdb.char_data, script_id: ?[*:0]const u8) bool {
    const id_z = script_id orelse return false;
    const data = getScriptData(ch) orelse return false;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return false;
    return data.scripts.contains(interned);
}

pub export fn char_script_number_get(ch: *cdb.char_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8) i64 {
    const id_z = script_id orelse return 0;
    const key_z = key orelse return 0;
    const data = getScriptData(ch) orelse return 0;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return 0;
    const instance = data.scripts.getPtr(interned) orelse return 0;
    return instance.numbers.get(std.mem.span(key_z)) orelse 0;
}

pub export fn char_script_number_set(ch: *cdb.char_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8, value: i64) void {
    const id_z = script_id orelse return;
    const key_z = key orelse return;
    const data = getScriptData(ch) orelse return;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return;
    const instance = data.scripts.getPtr(interned) orelse return;
    instance.numbers.put(lua_api.internString(std.mem.span(key_z)), value) catch {};
}

pub export fn char_script_text_get(ch: *cdb.char_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8) [*c]const u8 {
    const id_z = script_id orelse return null;
    const key_z = key orelse return null;
    const data = getScriptData(ch) orelse return null;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return null;
    const instance = data.scripts.getPtr(interned) orelse return null;
    return @ptrCast(instance.strings.get(std.mem.span(key_z)) orelse return null);
}

pub export fn char_script_text_set(ch: *cdb.char_data, script_id: ?[*:0]const u8, key: ?[*:0]const u8, value: ?[*:0]const u8) void {
    const id_z = script_id orelse return;
    const key_z = key orelse return;
    const val_z = value orelse return;
    const data = getScriptData(ch) orelse return;
    const interned = intern_mod.lookup(std.mem.span(id_z)) orelse return;
    const instance = data.scripts.getPtr(interned) orelse return;
    const key_str = lua_api.internString(std.mem.span(key_z));
    const val_copy = std.heap.page_allocator.dupe(u8, std.mem.span(val_z)) catch return;
    if (instance.strings.fetchPut(key_str, val_copy) catch null) |old| {
        std.heap.page_allocator.free(old.value);
    }
}

pub export fn char_script_event_dispatch(ch: *cdb.char_data, script_id: ?[*:0]const u8, event_name: ?[*:0]const u8) void {
    const id_z = script_id orelse return;
    const ev_z = event_name orelse return;
    lua_api.callCharScriptEventHook(ch, std.mem.span(id_z), std.mem.span(ev_z));
}

pub const CharScriptEntry = struct { name: []const u8 };

pub const CharScriptIterator = struct {
    inner: std.AutoHashMap(ScriptId, ScriptInstance).Iterator,

    pub fn next(self: *CharScriptIterator) ?CharScriptEntry {
        const kv = self.inner.next() orelse return null;
        return .{ .name = intern_mod.nameOf(kv.key_ptr.*) };
    }
};

pub fn characterScriptIterator(ch: *const cdb.char_data) ?CharScriptIterator {
    const ptr = ch.zigdata orelse return null;
    const data: *const CharacterData = @ptrCast(@alignCast(ptr));
    return .{ .inner = data.scripts.iterator() };
}
