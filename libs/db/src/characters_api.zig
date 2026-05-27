const cdb = @import("cdb");
const std = @import("std");
const characters = @import("characters.zig");
const bitflags = @import("flags.zig");
const obj_api = @import("objects_api.zig");
const lua_api = @import("lua_api.zig");
const modifiers_api = @import("modifiers_api.zig");

const TransformData = struct {
    // this is a placeholder for now.
};

const DerivedData = struct {
    value: i64,
};

const SkillData = struct {
    base: i64,
    perf: i64,
};

const meter_scale: i64 = 1_000_000;

const CharacterData = struct {
    stats: std.StringHashMap(i64),
    deriveds: std.StringHashMap(DerivedData),
    modifiers: modifiers_api.ModifierCache,
    deriveds_dirty: bool,
    transforms: std.StringHashMap(TransformData),
    meters: std.StringHashMap(i64),
    skills: std.StringHashMap(SkillData),

    pub fn init(alloc: std.mem.Allocator) CharacterData {
        return CharacterData{
            .stats = std.StringHashMap(i64).init(alloc),
            .deriveds = std.StringHashMap(DerivedData).init(alloc),
            .modifiers = modifiers_api.ModifierCache.init(alloc),
            .deriveds_dirty = true,
            .transforms = std.StringHashMap(TransformData).init(alloc),
            .meters = std.StringHashMap(i64).init(alloc),
            .skills = std.StringHashMap(SkillData).init(alloc),
        };
    }

    pub fn deinit(self: *CharacterData) void {
        var stats = self.stats.keyIterator();
        while (stats.next()) |key| std.heap.page_allocator.free(key.*);
        self.stats.deinit();
        var deriveds = self.deriveds.keyIterator();
        while (deriveds.next()) |key| std.heap.page_allocator.free(key.*);
        self.deriveds.deinit();
        self.modifiers.deinit();
        self.transforms.deinit();
        var meters = self.meters.keyIterator();
        while (meters.next()) |key| std.heap.page_allocator.free(key.*);
        self.meters.deinit();
        var skills = self.skills.keyIterator();
        while (skills.next()) |key| std.heap.page_allocator.free(key.*);
        self.skills.deinit();
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
    if (!validMobRnum(ch.nr)) return cdb.NOTHING;
    return cdb.mob_index[@intCast(ch.nr)].vnum;
}

pub export fn char_vnum_set(ch: *cdb.char_data, vnum: cdb.mob_vnum) void {
    ch.nr = cdb.real_mobile(vnum);
}

pub export fn char_room_get(ch: *cdb.char_data) [*c]cdb.room_data {
    if (!validRoomRnum(ch.in_room)) return null;
    return &cdb.world[@intCast(ch.in_room)];
}

pub export fn char_room_vnum_get(ch: *cdb.char_data) cdb.room_vnum {
    const room = char_room_get(ch);
    if (room == null) return cdb.NOWHERE;
    return room.*.number;
}

pub export fn char_zone_get(ch: *cdb.char_data) [*c]cdb.zone_data {
    const room = char_room_get(ch);
    if (room == null) return null;
    return &cdb.zone_table[@intCast(room.*.zone)];
}

pub export fn char_zone_vnum_get(ch: *cdb.char_data) cdb.zone_vnum {
    const zone = char_zone_get(ch);
    if (zone == null) return cdb.NOWHERE;
    return zone.*.number;
}

pub export fn char_room_vnum_set(ch: *cdb.char_data, vnum: cdb.room_vnum) void {
    ch.in_room = cdb.real_room(vnum);
}

pub export fn char_name_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.name;
}

pub export fn char_name_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.name, protoString(ch, .name), value);
}

pub export fn char_description_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.description;
}

pub export fn char_description_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.description, protoString(ch, .description), value);
}

pub export fn char_short_description_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.short_descr;
}

pub export fn char_short_description_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.short_descr, protoString(ch, .short_descr), value);
}

pub export fn char_long_description_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.long_descr;
}

pub export fn char_long_description_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.long_descr, protoString(ch, .long_descr), value);
}

pub export fn char_title_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.title;
}

pub export fn char_title_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.title, protoString(ch, .title), value);
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

pub export fn char_inventory_iterate(ch: *cdb.char_data, recursive: bool, func: ?obj_api.ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    _ = obj_api.objContentsListIterate(ch.carrying, recursive, callback, ctx);
}

pub export fn char_equipment_iterate(ch: *cdb.char_data, recursive: bool, func: ?obj_api.ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    for (ch.equipment) |obj| {
        if (obj == null) continue;
        if (!callback(&obj.*, ctx)) return;
        if (recursive and !obj_api.objContentsListIterate(obj.*.contains, true, callback, ctx)) return;
    }
}

pub export fn char_inventory_count(ch: *cdb.char_data, recursive: bool) usize {
    var count: usize = 0;
    var current = ch.carrying;
    while (current != null) : (current = current.*.next_content) {
        count += 1;
        if (recursive) count += obj_api.obj_inventory_count(&current.*, true);
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

pub export fn char_inventory_get(ch: *cdb.char_data, pos: usize) [*c]cdb.obj_data {
    var index: usize = 0;
    var current = ch.carrying;
    while (current != null) : (current = current.*.next_content) {
        if (index == pos) return current;
        index += 1;
    }
    return null;
}

pub export fn char_equipment_get(ch: *cdb.char_data, pos: usize) [*c]cdb.obj_data {
    if (pos >= ch.equipment.len) return null;
    return ch.equipment[pos];
}

fn validMobRnum(rnum: cdb.mob_rnum) bool {
    return rnum != cdb.NOBODY and rnum >= 0 and rnum <= cdb.top_of_mobt and cdb.mob_index != null;
}

fn validRoomRnum(rnum: cdb.room_rnum) bool {
    return rnum != cdb.NOWHERE and rnum >= 0 and rnum <= cdb.top_of_world and cdb.world != null;
}

fn validProto(ch: *cdb.char_data) bool {
    return validMobRnum(ch.nr) and cdb.mob_proto != null;
}

const StringField = enum { name, description, short_descr, long_descr, title };

fn protoString(ch: *cdb.char_data, field: StringField) [*c]u8 {
    if (!validProto(ch)) return null;
    const proto = &cdb.mob_proto[@intCast(ch.nr)];
    return switch (field) {
        .name => proto.name,
        .description => proto.description,
        .short_descr => proto.short_descr,
        .long_descr => proto.long_descr,
        .title => proto.title,
    };
}

fn replaceString(ch: *cdb.char_data, field: *[*c]u8, proto_value: [*c]u8, value: ?[*:0]const u8) void {
    _ = ch;
    const new_value = if (value) |new_string| strdup(new_string) orelse return else null;
    if (field.* != null and field.* != proto_value) std.c.free(field.*);
    field.* = new_value;
}

pub export fn char_stat_get(ch: *cdb.char_data, stat: ?[*:0]const u8) i64 {
    const name = statName(stat) orelse return 0;
    return charStatGetName(ch, name);
}

fn charStatGetName(ch: *cdb.char_data, name: []const u8) i64 {
    const definition = lua_api.statDefinition(name) orelse return 0;
    if (ch.zigdata == null) return definition.default_value;
    const zigdata: *CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    return zigdata.stats.get(name) orelse definition.default_value;
}

pub export fn char_stat_set(ch: *cdb.char_data, stat: ?[*:0]const u8, value: i64) i64 {
    const name = statName(stat) orelse return 0;
    const definition = lua_api.statDefinition(name) orelse return 0;
    const clamped = clampStat(value, definition);

    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    if (zigdata.stats.getPtr(name)) |existing| {
        existing.* = clamped;
        invalidateDeriveds(zigdata);
        return clamped;
    }

    const owned_name = std.heap.page_allocator.dupe(u8, name) catch return 0;
    zigdata.stats.put(owned_name, clamped) catch {
        std.heap.page_allocator.free(owned_name);
        return 0;
    };
    invalidateDeriveds(zigdata);
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

pub export fn char_legacy_modifier(ch: *cdb.char_data, location: c_int, specific: c_int) i64 {
    _ = ch;
    _ = location;
    _ = specific;
    return 0;
}

pub export fn char_der_get_base(ch: *cdb.char_data, stat: ?[*:0]const u8) i64 {
    const name = statName(stat) orelse return 0;
    return charDerGetBaseName(ch, name);
}

fn charDerGetBaseName(ch: *cdb.char_data, name: []const u8) i64 {
    const definition = lua_api.derivedDefinition(name) orelse return 0;
    if (lua_api.calculateDerivedBase(ch, name)) |value| return value;
    return charStatGetName(ch, definition.baseStat(name));
}

pub export fn char_der_get_total(ch: *cdb.char_data, stat: ?[*:0]const u8) i64 {
    const name = statName(stat) orelse return 0;
    return charDerGetTotalName(ch, name);
}

fn charDerGetTotalName(ch: *cdb.char_data, name: []const u8) i64 {
    const definition = lua_api.derivedDefinition(name) orelse return 0;
    const zigdata = char_ensure_zigdata(ch) orelse return 0;

    if (!zigdata.deriveds_dirty) {
        if (zigdata.deriveds.get(name)) |cached| return cached.value;
    }

    if (zigdata.modifiers.dirty) zigdata.modifiers.rebuild(ch);
    if (!definition.no_modifiers) addLegacyDerivedModifiers(ch, zigdata, name, definition);
    const total = calculateDerivedTotal(ch, zigdata, name, definition);
    cacheDerived(zigdata, name, total);
    return total;
}

pub export fn char_der_invalidate(ch: *cdb.char_data) void {
    const zigdata = char_ensure_zigdata(ch) orelse return;
    invalidateDeriveds(zigdata);
}

fn calculateDerivedTotal(ch: *cdb.char_data, zigdata: *CharacterData, name: []const u8, definition: lua_api.DerivedDefinition) i64 {
    var value = charDerGetBaseName(ch, name);
    var flat: i64 = 0;
    var percent: i64 = 0;
    var min_override: ?i64 = null;
    var max_override: ?i64 = null;
    var set_override: ?i64 = null;

    if (!definition.no_modifiers) {
        if (zigdata.modifiers.modifiersFor("derived", name)) |modifiers| {
            for (modifiers) |modifier| {
                switch (modifier.kind) {
                    .flat => flat += modifier.value,
                    .percent => percent += modifier.value,
                    .multiplier => {},
                    .override_min => min_override = if (min_override) |current| @max(current, modifier.value) else modifier.value,
                    .override_max => max_override = if (max_override) |current| @min(current, modifier.value) else modifier.value,
                    .set => set_override = modifier.value,
                }
            }

            value += flat;
            if (percent != 0) value += @divTrunc(value * percent, modifiers_api.scale);
            for (modifiers) |modifier| {
                if (modifier.kind == .multiplier) value = @divTrunc(value * modifier.value, modifiers_api.scale);
            }
        } else {
            value += flat;
            if (percent != 0) value += @divTrunc(value * percent, modifiers_api.scale);
        }
    }

    if (definition.min_value) |min| value = @max(value, min);
    if (definition.max_value) |max| value = @min(value, max);
    if (min_override) |min| value = @max(value, min);
    if (max_override) |max| value = @min(value, max);
    if (set_override) |set| value = set;
    return value;
}

fn addLegacyDerivedModifiers(ch: *cdb.char_data, zigdata: *CharacterData, name: []const u8, definition: lua_api.DerivedDefinition) void {
    for (definition.legacy_modifiers[0..definition.legacy_modifier_count]) |modifier| {
        modifiers_api.addLegacyDerivedFlat(&zigdata.modifiers, ch, name, modifier.location, modifier.specific);
    }
}

fn cacheDerived(zigdata: *CharacterData, name: []const u8, value: i64) void {
    if (zigdata.deriveds_dirty) {
        clearDerivedCache(zigdata);
        zigdata.deriveds_dirty = false;
    }

    if (zigdata.deriveds.getPtr(name)) |existing| {
        existing.* = .{ .value = value };
        return;
    }

    const owned_name = std.heap.page_allocator.dupe(u8, name) catch return;
    zigdata.deriveds.put(owned_name, .{ .value = value }) catch {
        std.heap.page_allocator.free(owned_name);
    };
}

fn invalidateDeriveds(zigdata: *CharacterData) void {
    zigdata.deriveds_dirty = true;
    zigdata.modifiers.invalidate();
}

fn clearDerivedCache(zigdata: *CharacterData) void {
    var keys = zigdata.deriveds.keyIterator();
    while (keys.next()) |key| std.heap.page_allocator.free(key.*);
    zigdata.deriveds.clearRetainingCapacity();
}

pub export fn char_meter_get(ch: *cdb.char_data, meter: ?[*:0]const u8) i64 {
    const name = statName(meter) orelse return 0;
    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    return zigdata.meters.get(name) orelse meter_scale;
}

pub export fn char_meter_set(ch: *cdb.char_data, meter: ?[*:0]const u8, value: i64) i64 {
    const name = statName(meter) orelse return 0;
    const clamped = clampMeter(value);

    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    if (zigdata.meters.getPtr(name)) |existing| {
        existing.* = clamped;
        syncLegacyMeter(ch, name, clamped);
        return clamped;
    }

    const owned_name = std.heap.page_allocator.dupe(u8, name) catch return 0;
    zigdata.meters.put(owned_name, clamped) catch {
        std.heap.page_allocator.free(owned_name);
        return 0;
    };
    syncLegacyMeter(ch, name, clamped);
    return clamped;
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
    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    const current = zigdata.meters.get(name) orelse meter_scale;
    return @divTrunc(current * max, meter_scale);
}

fn charMeterMaxName(ch: *cdb.char_data, name: []const u8) i64 {
    const definition = lua_api.meterDefinition(name) orelse return 0;
    return charDerGetTotalName(ch, definition.derivedStat(name));
}

fn clampMeter(value: i64) i64 {
    return @min(@max(value, 0), meter_scale);
}

fn syncLegacyMeter(ch: *cdb.char_data, name: []const u8, value: i64) void {
    const as_float = meterFixedToFloat(value);
    if (std.mem.eql(u8, name, "powerlevel")) ch.health = as_float;
    if (std.mem.eql(u8, name, "ki")) ch.energy = as_float;
    if (std.mem.eql(u8, name, "stamina")) ch.stamina = as_float;
    if (std.mem.eql(u8, name, "lifeforce")) ch.life = as_float;
}

pub fn meterFloatToFixed(value: f64) i64 {
    if (std.math.isNan(value)) return 0;
    const clamped = @min(@max(value, 0.0), 1.0);
    return @intFromFloat(clamped * @as(f64, @floatFromInt(meter_scale)));
}

pub fn meterFixedToFloat(value: i64) f64 {
    return @as(f64, @floatFromInt(clampMeter(value))) / @as(f64, @floatFromInt(meter_scale));
}
