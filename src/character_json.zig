const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");
const bitflags = @import("flags.zig");
const dgscripts_json = @import("dgscript_json.zig");
const characters_api = @import("character_api.zig");
const lua_api = @import("lua_api.zig");
const intern_mod = @import("intern.zig");

pub const JsonValue = jsonx.JsonValue;

pub const CharacterJsonMode = enum { player, npc, npc_prototype };
pub const DeserializeOptions = struct {
    c_allocator: std.mem.Allocator = std.heap.c_allocator,
    mode: CharacterJsonMode = .npc,
};

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;

pub fn serializeCharacter(allocator: std.mem.Allocator, ch: *cdb.char_data, mode: CharacterJsonMode) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putSlice(&object, allocator, "kind", switch (mode) {
        .player => "player",
        .npc => "npc",
        .npc_prototype => "npc_prototype",
    });

    var meters = jsonx.newObject(allocator);

    try jsonx.putString(&object, allocator, "name", cdb.char_name_get(ch));
    try jsonx.putString(&object, allocator, "description", cdb.char_description_get(ch));
    try jsonx.putString(&object, allocator, "short_description", cdb.char_short_description_get(ch));
    try jsonx.putString(&object, allocator, "long_description", cdb.char_long_description_get(ch));
    try jsonx.putString(&object, allocator, "title", cdb.char_title_get(ch));
    try jsonx.putInt(&object, allocator, "class", cdb.char_class_get(ch));
    try jsonx.putInt(&object, allocator, "race", cdb.char_race_get(ch));
    try jsonx.putInt(&object, allocator, "size", cdb.char_size_get(ch));
    try jsonx.putInt(&object, allocator, "sex", cdb.char_sex_get(ch));
    try jsonx.putInt(&object, allocator, "admin_level", cdb.char_admlevel_get(ch));
    try jsonx.putNonEmpty(&object, allocator, "admin_flags", try jsonx.serializeFlags(allocator, ch, 128, admFlagged));

    if (mode != .npc_prototype) {
        // fields common to all instances, including players.
        try jsonx.putInt(&object, allocator, "id", cdb.char_id_get(ch));

        try jsonx.putInt(&object, allocator, "hair_length", ch.hairl);
        try jsonx.putInt(&object, allocator, "hair_style", ch.hairs);
        try jsonx.putInt(&object, allocator, "hair_color", ch.hairc);
        try jsonx.putInt(&object, allocator, "skin", ch.skin);
        try jsonx.putInt(&object, allocator, "eye", ch.eye);
        try jsonx.putInt(&object, allocator, "distinguishing_feature", ch.distfea);
        try jsonx.putInt(&object, allocator, "aura", ch.aura);
        try jsonx.putInt(&object, allocator, "rage_meter", ch.rage_meter);
        try jsonx.putInt(&object, allocator, "mimic", ch.mimic);
        try jsonx.putInt(&object, allocator, "hometown", ch.hometown);
        try jsonx.putNonEmpty(&object, allocator, "bodyparts", try jsonx.serializeFlags(allocator, ch, cdb.NUM_AFF_FLAGS, bodypartFlagged));
        try jsonx.putNonEmpty(&object, allocator, "affected_by", try jsonx.serializeFlags(allocator, ch, cdb.NUM_AFF_FLAGS, affectedFlagged));
    }

    if (mode == .npc) {
        try jsonx.putInt(&object, allocator, "personality", ch.personality);
    }

    if (mode == .npc_prototype) {
        try jsonx.putNonEmpty(&object, allocator, "proto_script", try dgscripts_json.serializeProtoScript(allocator, ch.proto_script));
    }

    if (mode != .player) {
        // fields common to npcs and npc_prototypes
        try jsonx.putInt(&object, allocator, "proto_id", cdb.char_proto_id_get(ch));

        try jsonx.putNonEmpty(&object, allocator, "mob_flags", try jsonx.serializeFlags(allocator, ch, cdb.NUM_MOB_FLAGS, actFlagged));
    }

    if (mode == .player) {
        try jsonx.putInt(&object, allocator, "idnum", ch.idnum);
        try jsonx.putString(&object, allocator, "host", ch.host);
        try jsonx.putInt(&object, allocator, "load_room", ch.load_room);
        try jsonx.putInt(&object, allocator, "wimp_level", ch.wimp_level);
        try jsonx.putInt(&object, allocator, "freeze_level", ch.freeze_level);
        try jsonx.putInt(&object, allocator, "invis_level", ch.invis_level);
        try jsonx.putInt(&object, allocator, "bad_passwords", ch.bad_pws);
        try jsonx.putInt(&object, allocator, "olc_zone", ch.olc_zone);
        try jsonx.putString(&object, allocator, "poofin", ch.poofin);
        try jsonx.putString(&object, allocator, "poofout", ch.poofout);
        try jsonx.putInt(&object, allocator, "murder", ch.murder);
        try jsonx.putInt(&object, allocator, "racial_pref", ch.racial_pref);
        try jsonx.putInt(&object, allocator, "speaking", ch.speaking);
        try jsonx.putNonEmpty(&object, allocator, "pref_flags", try jsonx.serializeFlags(allocator, ch, 128, prefFlagged));
        try jsonx.put(&object, allocator, "color_choices", try serializeStringArray(allocator, ch.color_choices[0..]));
        try jsonx.putString(&object, allocator, "user", ch.loguser);
        try jsonx.putString(&object, allocator, "clan", ch.clan);
        try jsonx.putString(&object, allocator, "feature", ch.feature);
        try jsonx.putString(&object, allocator, "rdisplay", ch.rdisplay);
        try jsonx.putString(&object, allocator, "voice", ch.voice);
        try jsonx.put(&object, allocator, "time", try serializeTime(allocator, ch));
        try jsonx.putInt(&object, allocator, "lastint", ch.lastint);
        try jsonx.putInt(&object, allocator, "sleeptime", ch.sleeptime);
        try jsonx.putInt(&object, allocator, "cooldown", ch.cooldown);
        try jsonx.putInt(&object, allocator, "backstabcool", ch.backstabcool);
        try jsonx.putInt(&object, allocator, "con_cooldown", ch.con_cooldown);
        try jsonx.putInt(&object, allocator, "sd_cooldown", ch.con_sdcooldown);
        try jsonx.putInt(&object, allocator, "gooptime", ch.gooptime);
        try jsonx.putInt(&object, allocator, "death_type", ch.death_type);
        try jsonx.putInt(&object, allocator, "droom", ch.droom);
        try jsonx.putInt(&object, allocator, "deathtime", ch.deathtime);
        try jsonx.putInt(&object, allocator, "reward_time", ch.rewtime);
        try jsonx.putInt(&object, allocator, "transclass", ch.transclass);
        try jsonx.putInt(&object, allocator, "preference", ch.preference);
        try jsonx.putInt(&object, allocator, "rp", ch.rp);
        try jsonx.putInt(&object, allocator, "total_rp", ch.trp);
        try jsonx.putInt(&object, allocator, "clank_rank", ch.crank);
        try jsonx.putInt(&object, allocator, "last_play", ch.lastpl);
        // boosts is now a stat (migrated); no longer serialized here
        try jsonx.putInt(&object, allocator, "absorbs", ch.absorbs);
        try jsonx.putInt(&object, allocator, "ingest_learned", ch.ingestLearned);
        try jsonx.putInt(&object, allocator, "radar1", ch.radar1);
        try jsonx.putInt(&object, allocator, "radar2", ch.radar2);
        try jsonx.putInt(&object, allocator, "radar3", ch.radar3);
        try jsonx.putNonEmpty(&object, allocator, "player_flags", try jsonx.serializeFlags(allocator, ch, cdb.NUM_PLR_FLAGS, actFlagged));
        try jsonx.putNonEmpty(&object, allocator, "skills", try serializeSkills(allocator, ch));
        try jsonx.put(&object, allocator, "lboard", try serializeIntArray(allocator, ch.lboard[0..]));
        // limbs are now stored as stats (limb_right_arm, limb_left_arm, etc.)
        try jsonx.put(&object, allocator, "genome", try serializeIntArray(allocator, ch.genome[0..]));
        try jsonx.put(&object, allocator, "bonuses", try serializeIntArray(allocator, ch.bonuses[0..]));
        try jsonx.put(&object, allocator, "transcost", try serializeIntArray(allocator, ch.transcost[0..]));
    }

    try jsonx.putNonEmpty(&object, allocator, "stats", try serializeStats(allocator, ch));
    if (meters.object.count() > 0) try jsonx.put(&object, allocator, "meters", meters);
    try jsonx.putNonEmpty(&object, allocator, "conditions", try serializeConditions(allocator, ch));
    try jsonx.putNonEmpty(&object, allocator, "scripts", try serializeCharScripts(allocator, ch));
    try jsonx.putNonEmpty(&object, allocator, "transformations", try serializeTransformations(allocator, ch));

    return object;
}

pub fn serializeMobPrototype(allocator: std.mem.Allocator, proto: *cdb.mob_proto_data) !JsonValue {
    var ch: cdb.char_data = std.mem.zeroes(cdb.char_data);
    mobProtoToCharacter(&ch, proto);
    return serializeCharacter(allocator, &ch, .npc_prototype);
}

pub fn deserializeMobPrototype(proto: *cdb.mob_proto_data, options: DeserializeOptions, value: JsonValue) !void {
    var ch: cdb.char_data = std.mem.zeroes(cdb.char_data);
    ch.proto_id = proto.id;
    try deserializeCharacter(&ch, .{ .c_allocator = options.c_allocator, .mode = .npc_prototype }, value);
    characterToMobProto(proto, &ch);
}

fn mobProtoToCharacter(ch: *cdb.char_data, proto: *const cdb.mob_proto_data) void {
    ch.proto_id = proto.id;
    ch.name = proto.name;
    ch.short_descr = proto.short_descr;
    ch.long_descr = proto.long_descr;
    ch.description = proto.description;
    ch.title = proto.title;
    ch.size = proto.size;
    ch.sex = proto.sex;
    ch.race = proto.race;
    ch.chclass = proto.chclass;
    ch.mob_specials = proto.mob_specials;
    ch.speaking = proto.speaking;
    ch.act = proto.act;
    ch.affected_by = proto.affected_by;
    ch.proto_script = proto.proto_script;
    cdb.mob_proto_stats_copy_to_char(@constCast(proto), ch);
}

fn characterToMobProto(proto: *cdb.mob_proto_data, ch: *const cdb.char_data) void {
    proto.id = ch.proto_id;
    proto.name = ch.name;
    proto.short_descr = ch.short_descr;
    proto.long_descr = ch.long_descr;
    proto.description = ch.description;
    proto.title = ch.title;
    proto.size = ch.size;
    proto.sex = ch.sex;
    proto.race = ch.race;
    proto.chclass = ch.chclass;
    proto.mob_specials = ch.mob_specials;
    proto.speaking = ch.speaking;
    proto.act = ch.act;
    proto.affected_by = ch.affected_by;
    proto.proto_script = ch.proto_script;
    cdb.char_stats_copy_to_mob_proto(@constCast(ch), proto);
}

pub fn deserializeCharacter(ch: *cdb.char_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;

    // general shared fields go here.
    // These must precede the others because class must be set for other things to work right.
    try setStringField(options.c_allocator, value, "name", ch, cdb.char_name_set);
    try setStringField(options.c_allocator, value, "description", ch, cdb.char_description_set);
    try setStringField(options.c_allocator, value, "short_description", ch, cdb.char_short_description_set);
    try setStringField(options.c_allocator, value, "long_description", ch, cdb.char_long_description_set);
    try setStringField(options.c_allocator, value, "title", ch, cdb.char_title_set);
    if (try jsonx.intField(value, "class", c_int)) |v| cdb.char_class_set(ch, v);
    if (try jsonx.intField(value, "race", c_int)) |v| cdb.char_race_set(ch, v);
    if (try jsonx.intField(value, "size", c_int)) |v| cdb.char_size_set(ch, v);
    if (try jsonx.intField(value, "sex", c_int)) |v| cdb.char_sex_set(ch, v);
    if (try jsonx.intField(value, "admin_level", c_int)) |v| cdb.char_admlevel_set(ch, v);
    if (jsonx.field(value, "admin_flags")) |flags| try jsonx.deserializeFlags(ch, flags, 128, admFlagSet);
    if (jsonx.field(value, "stats")) |stats| try deserializeStats(ch, stats);

    if (options.mode != .npc_prototype) {
        // fields common to all instances, including players.
        if (try jsonx.intField(value, "id", i64)) |v| cdb.char_id_set(ch, v);
        if (try jsonx.intField(value, "hair_length", i8)) |v| ch.hairl = v;
        if (try jsonx.intField(value, "hair_style", i8)) |v| ch.hairs = v;
        if (try jsonx.intField(value, "hair_color", i8)) |v| ch.hairc = v;
        if (try jsonx.intField(value, "skin", i8)) |v| ch.skin = v;
        if (try jsonx.intField(value, "eye", i8)) |v| ch.eye = v;
        if (try jsonx.intField(value, "distinguishing_feature", i8)) |v| ch.distfea = v;
        if (try jsonx.intField(value, "aura", c_int)) |v| ch.aura = v;
        if (try jsonx.intField(value, "rage_meter", c_int)) |v| ch.rage_meter = v;
        if (try jsonx.intField(value, "mimic", c_int)) |v| ch.mimic = v;
        if (try jsonx.intField(value, "hometown", cdb.room_vnum)) |v| ch.hometown = v;
        if (jsonx.field(value, "bodyparts")) |flags| try jsonx.deserializeFlags(ch, flags, cdb.NUM_AFF_FLAGS, bodypartFlagSet);
        if (jsonx.field(value, "affected_by")) |flags| try jsonx.deserializeFlags(ch, flags, cdb.NUM_AFF_FLAGS, affectedFlagSet);
        if (jsonx.field(value, "conditions")) |conditions| try deserializeConditions(ch, conditions);
        if (jsonx.field(value, "scripts")) |scripts| try deserializeCharScripts(ch, scripts);
        if (jsonx.field(value, "transformations")) |transformations| try deserializeTransformations(ch, transformations);
    }

    if (options.mode == .npc) {
        if (try jsonx.intField(value, "personality", c_int)) |v| ch.personality = v;
    }

    if (options.mode == .npc_prototype) {
        if (jsonx.field(value, "proto_script")) |items| try dgscripts_json.deserializeProtoScript(&ch.proto_script, items);
    }

    if (options.mode != .player) {
        // Fields for only npcs and npc prototypes.
        if (try jsonx.intField(value, "proto_id", cdb.mob_vnum)) |v| cdb.char_proto_id_set(ch, v);
        if (jsonx.field(value, "mob_flags")) |flags| try jsonx.deserializeFlags(ch, flags, cdb.NUM_MOB_FLAGS, actFlagSet);
    }

    if (options.mode == .player) {
        // player-specific fields go here.
        if (try jsonx.intField(value, "idnum", i32)) |v| ch.idnum = v;
        try setPointerStringField(options.c_allocator, value, "host", &ch.host, setPlayerString);
        if (try jsonx.intField(value, "load_room", cdb.room_vnum)) |v| ch.load_room = v;
        if (try jsonx.intField(value, "wimp_level", c_int)) |v| ch.wimp_level = v;
        if (try jsonx.intField(value, "freeze_level", i8)) |v| ch.freeze_level = v;
        if (try jsonx.intField(value, "invis_level", i16)) |v| ch.invis_level = v;
        if (try jsonx.intField(value, "bad_passwords", u8)) |v| ch.bad_pws = v;
        if (try jsonx.intField(value, "olc_zone", c_int)) |v| ch.olc_zone = v;
        try setPointerStringField(options.c_allocator, value, "poofin", &ch.poofin, setPlayerString);
        try setPointerStringField(options.c_allocator, value, "poofout", &ch.poofout, setPlayerString);
        if (try jsonx.intField(value, "murder", c_int)) |v| ch.murder = v;
        if (try jsonx.intField(value, "racial_pref", c_int)) |v| ch.racial_pref = v;
        if (try jsonx.intField(value, "speaking", c_int)) |v| ch.speaking = v;
        if (jsonx.field(value, "pref_flags")) |flags| try jsonx.deserializeFlags(ch, flags, 128, prefFlagSet);
        if (jsonx.field(value, "color_choices")) |items| try deserializeStringArray(options.c_allocator, ch.color_choices[0..], items);
        try setPointerStringField(options.c_allocator, value, "user", &ch.loguser, setRawString);
        try setPointerStringField(options.c_allocator, value, "clan", &ch.clan, setRawString);
        try setPointerStringField(options.c_allocator, value, "feature", &ch.feature, setRawString);
        try setPointerStringField(options.c_allocator, value, "rdisplay", &ch.rdisplay, setRawString);
        try setPointerStringField(options.c_allocator, value, "voice", &ch.voice, setRawString);
        if (jsonx.field(value, "time")) |time| try deserializeTime(ch, time);
        if (try jsonx.intField(value, "lastint", cdb.time_t)) |v| ch.lastint = v;
        if (try jsonx.intField(value, "sleeptime", c_int)) |v| ch.sleeptime = v;
        if (try jsonx.intField(value, "cooldown", c_int)) |v| ch.cooldown = v;
        if (try jsonx.intField(value, "backstabcool", c_int)) |v| ch.backstabcool = v;
        if (try jsonx.intField(value, "con_cooldown", c_int)) |v| ch.con_cooldown = v;
        if (try jsonx.intField(value, "sd_cooldown", c_int)) |v| ch.con_sdcooldown = v;
        if (try jsonx.intField(value, "gooptime", c_int)) |v| ch.gooptime = v;
        if (try jsonx.intField(value, "death_type", c_int)) |v| ch.death_type = v;
        if (try jsonx.intField(value, "droom", cdb.room_vnum)) |v| ch.droom = v;
        if (try jsonx.intField(value, "deathtime", cdb.time_t)) |v| ch.deathtime = v;
        if (try jsonx.intField(value, "reward_time", cdb.time_t)) |v| ch.rewtime = v;
        if (try jsonx.intField(value, "transclass", c_int)) |v| ch.transclass = v;
        if (try jsonx.intField(value, "preference", c_int)) |v| ch.preference = v;
        if (try jsonx.intField(value, "rp", c_int)) |v| ch.rp = v;
        if (try jsonx.intField(value, "total_rp", c_int)) |v| ch.trp = v;
        if (try jsonx.intField(value, "clank_rank", c_int)) |v| ch.crank = v;
        if (try jsonx.intField(value, "last_play", cdb.time_t)) |v| ch.lastpl = v;
        if (try jsonx.intField(value, "boosts", c_int)) |v| _ = cdb.char_stat_set(ch, "boosts", @intCast(v)); // migrate old field into stat system
        if (try jsonx.intField(value, "absorbs", c_int)) |v| ch.absorbs = v;
        if (try jsonx.intField(value, "ingest_learned", c_int)) |v| ch.ingestLearned = v;
        if (try jsonx.intField(value, "radar1", cdb.room_vnum)) |v| ch.radar1 = v;
        if (try jsonx.intField(value, "radar2", cdb.room_vnum)) |v| ch.radar2 = v;
        if (try jsonx.intField(value, "radar3", cdb.room_vnum)) |v| ch.radar3 = v;
        if (jsonx.field(value, "player_flags")) |flags| try jsonx.deserializeFlags(ch, flags, cdb.NUM_PLR_FLAGS, actFlagSet);
        if (jsonx.field(value, "skills")) |skills| try deserializeSkills(ch, skills);
        if (jsonx.field(value, "lboard")) |items| try deserializeIntArray(ch.lboard[0..], items);
        if (jsonx.field(value, "limbs")) |items| try migrateOldLimbs(ch, items);
        if (jsonx.field(value, "genome")) |items| try deserializeIntArray(ch.genome[0..], items);
        if (jsonx.field(value, "bonuses")) |items| try deserializeIntArray(ch.bonuses[0..], items);
        if (jsonx.field(value, "transcost")) |items| try deserializeIntArray(ch.transcost[0..], items);
    }
}

fn setString(allocator: std.mem.Allocator, ch: *cdb.char_data, value: []const u8, comptime setter: anytype) !void {
    const z = try allocator.dupeZ(u8, value);
    defer allocator.free(z);
    setter(ch, z);
}

fn setStringField(allocator: std.mem.Allocator, object: JsonValue, key: []const u8, ctx: anytype, comptime setter: anytype) !void {
    const value = try jsonx.stringFieldAlloc(allocator, object, key) orelse return;
    defer allocator.free(value);
    try setString(allocator, ctx, value, setter);
}

fn setPointerStringField(allocator: std.mem.Allocator, object: JsonValue, key: []const u8, ptr: anytype, comptime setter: anytype) !void {
    const value = try jsonx.stringFieldAlloc(allocator, object, key) orelse return;
    defer allocator.free(value);
    try setter(allocator, ptr, value);
}

fn serializeStats(allocator: std.mem.Allocator, ch: *cdb.char_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    var maybe_iter = characters_api.characterStatIterator(ch);
    if (maybe_iter) |*iter| {
        while (iter.next()) |entry| {
            try jsonx.putInt(&object, allocator, entry.name, entry.value);
        }
    }
    return object;
}

fn putMeter(object: *JsonValue, allocator: std.mem.Allocator, name: []const u8, value: f64) !void {
    try jsonx.putFloat(object, allocator, name, value);
}

fn deserializeStats(ch: *cdb.char_data, stats: JsonValue) !void {
    if (stats != .object) return error.ExpectedObject;
    var it = stats.object.iterator();
    while (it.next()) |entry| {
        const value = switch (entry.value_ptr.*) {
            .integer => |v| v,
            else => return error.ExpectedInteger,
        };
        const z_name = try std.heap.c_allocator.dupeZ(u8, entry.key_ptr.*);
        defer std.heap.c_allocator.free(z_name);
        _ = cdb.char_stat_set(ch, z_name, value);
    }
}

fn serializeConditions(allocator: std.mem.Allocator, ch: *cdb.char_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    if (ch.zigdata == null) return object;
    const data: *characters_api.CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var it = data.conditions.iterator();
    while (it.next()) |entry| {
        const cname = intern_mod.nameOf(entry.key_ptr.*);
        const definition = lua_api.conditionDefinition(cname) orelse continue;
        if (!definition.persistent) continue;
        try jsonx.put(&object, allocator, cname, try serializeCondition(allocator, ch, cname, entry.value_ptr));
    }
    return object;
}

fn serializeCondition(allocator: std.mem.Allocator, ch: *cdb.char_data, name: []const u8, condition: *characters_api.ConditionInstance) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putInt(&object, allocator, "stacks", condition.stacks);
    const name_z = try std.heap.page_allocator.dupeZ(u8, name);
    defer std.heap.page_allocator.free(name_z);
    try jsonx.putInt(&object, allocator, "duration", cdb.char_condition_duration_get(ch, name_z.ptr));

    var sources = jsonx.JsonArray.init(allocator);
    for (condition.sources.items) |source| {
        var source_object = jsonx.newObject(allocator);
        try jsonx.putSlice(&source_object, allocator, "category", source.category);
        try jsonx.putSlice(&source_object, allocator, "id", source.id);
        try sources.append(source_object);
    }
    if (sources.items.len > 0) try jsonx.put(&object, allocator, "sources", .{ .array = sources });

    var numbers = jsonx.newObject(allocator);
    var number_it = condition.numbers.iterator();
    while (number_it.next()) |entry| try jsonx.putInt(&numbers, allocator, entry.key_ptr.*, entry.value_ptr.*);
    if (numbers.object.count() > 0) try jsonx.put(&object, allocator, "numbers", numbers);

    var strings = jsonx.newObject(allocator);
    var string_it = condition.strings.iterator();
    while (string_it.next()) |entry| try jsonx.putSlice(&strings, allocator, entry.key_ptr.*, entry.value_ptr.*);
    if (strings.object.count() > 0) try jsonx.put(&object, allocator, "strings", strings);
    return object;
}

fn deserializeConditions(ch: *cdb.char_data, conditions: JsonValue) !void {
    if (conditions != .object) return error.ExpectedObject;
    var it = conditions.object.iterator();
    while (it.next()) |entry| {
        const id = entry.key_ptr.*;
        const item = entry.value_ptr.*;
        if (item != .object) return error.ExpectedObject;
        const id_z = try std.heap.page_allocator.dupeZ(u8, id);
        defer std.heap.page_allocator.free(id_z);
        // Add silently so on_apply fires AFTER variables are restored below.
        if (jsonx.field(item, "sources")) |sources| {
            if (sources == .array and sources.array.items.len > 0) {
                try deserializeConditionSources(ch, id_z.ptr, sources);
            } else {
                _ = cdb.char_condition_add_silent(ch, id_z.ptr, "json", "conditions");
            }
        } else {
            _ = cdb.char_condition_add_silent(ch, id_z.ptr, "json", "conditions");
        }
        if (try jsonx.intField(item, "stacks", i64)) |v| _ = cdb.char_condition_stacks_set(ch, id_z.ptr, v);
        if (try jsonx.intField(item, "duration", i64)) |v| _ = cdb.char_condition_duration_set(ch, id_z.ptr, v);
        if (jsonx.field(item, "numbers")) |numbers| try deserializeConditionNumbers(ch, id, numbers);
        if (jsonx.field(item, "strings")) |strings| try deserializeConditionStrings(ch, id, strings);
        // Fire on_apply now that all variables are set.
        cdb.char_condition_notify_applied(ch, id_z.ptr);
    }
}

fn deserializeConditionSources(ch: *cdb.char_data, id_z: [*:0]const u8, sources: JsonValue) !void {
    if (sources != .array) return error.ExpectedArray;
    for (sources.array.items) |source| {
        if (source != .object) return error.ExpectedObject;
        const category = try jsonx.stringFieldAlloc(std.heap.page_allocator, source, "category") orelse continue;
        defer std.heap.page_allocator.free(category);
        const source_id = try jsonx.stringFieldAlloc(std.heap.page_allocator, source, "id") orelse continue;
        defer std.heap.page_allocator.free(source_id);
        const category_z = try std.heap.page_allocator.dupeZ(u8, category);
        defer std.heap.page_allocator.free(category_z);
        const source_z = try std.heap.page_allocator.dupeZ(u8, source_id);
        defer std.heap.page_allocator.free(source_z);
        _ = cdb.char_condition_add_silent(ch, id_z, category_z.ptr, source_z.ptr);
    }
}

fn deserializeConditionNumbers(ch: *cdb.char_data, id: []const u8, numbers: JsonValue) !void {
    if (numbers != .object) return error.ExpectedObject;
    const id_z = try std.heap.page_allocator.dupeZ(u8, id);
    defer std.heap.page_allocator.free(id_z);
    var it = numbers.object.iterator();
    while (it.next()) |entry| {
        if (entry.value_ptr.* != .integer) return error.ExpectedInteger;
        const key_z = try std.heap.page_allocator.dupeZ(u8, entry.key_ptr.*);
        defer std.heap.page_allocator.free(key_z);
        _ = cdb.char_condition_number_set(ch, id_z.ptr, key_z.ptr, entry.value_ptr.integer);
    }
}

fn deserializeConditionStrings(ch: *cdb.char_data, id: []const u8, strings: JsonValue) !void {
    if (strings != .object) return error.ExpectedObject;
    const id_z = try std.heap.page_allocator.dupeZ(u8, id);
    defer std.heap.page_allocator.free(id_z);
    var it = strings.object.iterator();
    while (it.next()) |entry| {
        if (entry.value_ptr.* != .string) return error.ExpectedString;
        const key_z = try std.heap.page_allocator.dupeZ(u8, entry.key_ptr.*);
        defer std.heap.page_allocator.free(key_z);
        const value_z = try std.heap.page_allocator.dupeZ(u8, entry.value_ptr.string);
        defer std.heap.page_allocator.free(value_z);
        _ = cdb.char_condition_string_set(ch, id_z.ptr, key_z.ptr, value_z.ptr);
    }
}

fn serializeTransformations(allocator: std.mem.Allocator, ch: *cdb.char_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    if (ch.zigdata == null) return object;
    const data: *characters_api.CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var it = data.transforms.iterator();
    while (it.next()) |entry| try jsonx.put(&object, allocator, intern_mod.nameOf(entry.key_ptr.*), try serializeTransformation(allocator, entry.value_ptr));
    return object;
}

fn serializeTransformation(allocator: std.mem.Allocator, transform: *characters_api.TransformData) !JsonValue {
    var object = jsonx.newObject(allocator);

    var numbers = jsonx.newObject(allocator);
    var number_it = transform.numbers.iterator();
    while (number_it.next()) |entry| try jsonx.putInt(&numbers, allocator, entry.key_ptr.*, entry.value_ptr.*);
    if (numbers.object.count() > 0) try jsonx.put(&object, allocator, "numbers", numbers);

    var strings = jsonx.newObject(allocator);
    var string_it = transform.strings.iterator();
    while (string_it.next()) |entry| try jsonx.putSlice(&strings, allocator, entry.key_ptr.*, entry.value_ptr.*);
    if (strings.object.count() > 0) try jsonx.put(&object, allocator, "strings", strings);
    return object;
}

fn deserializeTransformations(ch: *cdb.char_data, transformations: JsonValue) !void {
    if (transformations != .object) return error.ExpectedObject;
    var it = transformations.object.iterator();
    while (it.next()) |entry| {
        const id = entry.key_ptr.*;
        const item = entry.value_ptr.*;
        if (item != .object) return error.ExpectedObject;
        const id_z = try std.heap.page_allocator.dupeZ(u8, id);
        defer std.heap.page_allocator.free(id_z);
        _ = cdb.char_transform_add(ch, id_z.ptr);
        if (jsonx.field(item, "numbers")) |numbers| try deserializeTransformationNumbers(ch, id, numbers);
        if (jsonx.field(item, "strings")) |strings| try deserializeTransformationStrings(ch, id, strings);
    }
}

fn deserializeTransformationNumbers(ch: *cdb.char_data, id: []const u8, numbers: JsonValue) !void {
    if (numbers != .object) return error.ExpectedObject;
    const id_z = try std.heap.page_allocator.dupeZ(u8, id);
    defer std.heap.page_allocator.free(id_z);
    var it = numbers.object.iterator();
    while (it.next()) |entry| {
        if (entry.value_ptr.* != .integer) return error.ExpectedInteger;
        const key_z = try std.heap.page_allocator.dupeZ(u8, entry.key_ptr.*);
        defer std.heap.page_allocator.free(key_z);
        _ = cdb.char_transform_number_set(ch, id_z.ptr, key_z.ptr, entry.value_ptr.integer);
    }
}

fn deserializeTransformationStrings(ch: *cdb.char_data, id: []const u8, strings: JsonValue) !void {
    if (strings != .object) return error.ExpectedObject;
    const id_z = try std.heap.page_allocator.dupeZ(u8, id);
    defer std.heap.page_allocator.free(id_z);
    var it = strings.object.iterator();
    while (it.next()) |entry| {
        if (entry.value_ptr.* != .string) return error.ExpectedString;
        const key_z = try std.heap.page_allocator.dupeZ(u8, entry.key_ptr.*);
        defer std.heap.page_allocator.free(key_z);
        const value_z = try std.heap.page_allocator.dupeZ(u8, entry.value_ptr.string);
        defer std.heap.page_allocator.free(value_z);
        _ = cdb.char_transform_string_set(ch, id_z.ptr, key_z.ptr, value_z.ptr);
    }
}

fn serializeIntArray(allocator: std.mem.Allocator, values: anytype) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    for (values) |value| try array.append(.{ .integer = @intCast(value) });
    return .{ .array = array };
}

const limb_stat_names = [4][*:0]const u8{
    "limb_right_arm", "limb_left_arm", "limb_right_leg", "limb_left_leg",
};

fn migrateOldLimbs(ch: *cdb.char_data, json: JsonValue) !void {
    if (json != .array) return error.ExpectedArray;
    for (json.array.items, 0..) |item, index| {
        if (index >= 4) break;
        if (item != .integer) return error.ExpectedInteger;
        const val: i64 = @intCast(item.integer);
        _ = characters_api.char_stat_set(ch, limb_stat_names[index], val);
    }
}

fn deserializeIntArray(values: anytype, json: JsonValue) !void {
    if (json != .array) return error.ExpectedArray;
    for (json.array.items, 0..) |item, index| {
        if (index >= values.len) break;
        if (item != .integer) return error.ExpectedInteger;
        values[index] = std.math.cast(@TypeOf(values[index]), item.integer) orelse return error.IntegerOutOfRange;
    }
}

fn serializeStringArray(allocator: std.mem.Allocator, values: anytype) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    for (values) |value| {
        if (value) |ptr| {
            try array.append(.{ .string = try allocator.dupe(u8, std.mem.span(ptr)) });
        } else {
            try array.append(.null);
        }
    }
    return .{ .array = array };
}

fn deserializeStringArray(allocator: std.mem.Allocator, values: anytype, json: JsonValue) !void {
    if (json != .array) return error.ExpectedArray;
    for (json.array.items, 0..) |item, index| {
        if (index >= values.len) break;
        if (item == .null) continue;
        const value = (try jsonx.stringValueAlloc(allocator, item)) orelse continue;
        defer allocator.free(value);
        try setRawString(allocator, &values[index], value);
    }
}

fn serializeTime(allocator: std.mem.Allocator, ch: *cdb.char_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putInt(&object, allocator, "birth", ch.time.birth);
    try jsonx.putInt(&object, allocator, "created", ch.time.created);
    try jsonx.putInt(&object, allocator, "max_age", ch.time.maxage);
    try jsonx.putInt(&object, allocator, "played", ch.time.played);
    try jsonx.putInt(&object, allocator, "logon", ch.time.logon);
    return object;
}

fn deserializeTime(ch: *cdb.char_data, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    if (try jsonx.intField(value, "birth", cdb.time_t)) |v| ch.time.birth = v;
    if (try jsonx.intField(value, "created", cdb.time_t)) |v| ch.time.created = v;
    if (try jsonx.intField(value, "max_age", cdb.time_t)) |v| ch.time.maxage = v;
    if (try jsonx.intField(value, "played", cdb.time_t)) |v| ch.time.played = v;
    if (try jsonx.intField(value, "logon", cdb.time_t)) |v| ch.time.logon = v;
}

fn serializeSkills(allocator: std.mem.Allocator, ch: *cdb.char_data) !JsonValue {
    // TODO: replace numeric keys with names once the global skill table is exposed through the DB ABI.
    var object = jsonx.newObject(allocator);
    for (ch.skills[0..], 0..) |skill, index| {
        if (cdb.spell_info[index].name == null) continue; // skip skills with no name, which are not real skills.
        if (skill.base == 0 and skill.perf == 0) continue;
        const key = try std.fmt.allocPrint(allocator, "{}", .{index});
        var skill_object = jsonx.newObject(allocator);
        if (skill.base != 0) try jsonx.putInt(&skill_object, allocator, "base", skill.base);
        if (skill.perf != 0) try jsonx.putInt(&skill_object, allocator, "perf", skill.perf);
        try jsonx.put(&object, allocator, key, skill_object);
    }
    return object;
}

fn deserializeSkills(ch: *cdb.char_data, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    var iter = value.object.iterator();
    while (iter.next()) |entry| {
        const index = std.fmt.parseInt(usize, entry.key_ptr.*, 10) catch continue;
        if (index >= ch.skills.len) continue;
        if (cdb.spell_info[index].name == null) continue; // skip skills with no name, which are not real skills.
        const skill = entry.value_ptr.*;
        if (skill != .object) return error.ExpectedObject;
        if (try jsonx.intField(skill, "base", i8)) |v| ch.skills[index].base = v;
        if (try jsonx.intField(skill, "perf", i8)) |v| ch.skills[index].perf = v;
    }
}

fn setPlayerString(allocator: std.mem.Allocator, field: *[*c]u8, value: []const u8) !void {
    try setRawString(allocator, field, value);
}

fn setRawString(allocator: std.mem.Allocator, field: *[*c]u8, value: []const u8) !void {
    const z = try allocator.dupeZ(u8, value);
    if (field.* != null) std.c.free(field.*);
    field.* = z.ptr;
}

fn validClassIndex(chclass: c_int) bool {
    return chclass >= 0 and chclass < cdb.NUM_CLASSES;
}

fn admFlagged(ch: *cdb.char_data, pos: c_int) bool {
    return cdb.char_admflagged(ch, pos);
}
fn admFlagSet(ch: *cdb.char_data, pos: c_int, value: bool) void {
    cdb.char_admflag_set(ch, pos, value);
}

fn actFlagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(ch.act[0..], pos);
}

fn actFlagSet(ch: *cdb.char_data, pos: c_int, value: bool) void {
    bitflags.set(ch.act[0..], pos, value);
}

fn affectedFlagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(ch.affected_by[0..], pos);
}

fn affectedFlagSet(ch: *cdb.char_data, pos: c_int, value: bool) void {
    bitflags.set(ch.affected_by[0..], pos, value);
}

fn bodypartFlagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(ch.bodyparts[0..], pos);
}

fn bodypartFlagSet(ch: *cdb.char_data, pos: c_int, value: bool) void {
    bitflags.set(ch.bodyparts[0..], pos, value);
}

fn prefFlagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(ch.pref[0..], pos);
}

fn prefFlagSet(ch: *cdb.char_data, pos: c_int, value: bool) void {
    bitflags.set(ch.pref[0..], pos, value);
}

fn serializeCharScripts(allocator: std.mem.Allocator, ch: *cdb.char_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    if (ch.zigdata == null) return object;
    const data: *characters_api.CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var it = data.scripts.iterator();
    while (it.next()) |entry| {
        const sname = intern_mod.nameOf(entry.key_ptr.*);
        const definition = lua_api.scriptDefinition("character_scripts", sname) orelse continue;
        if (!definition.persistent) continue;
        try jsonx.put(&object, allocator, sname, try serializeScriptInstance(allocator, entry.value_ptr));
    }
    return object;
}

fn serializeScriptInstance(allocator: std.mem.Allocator, instance: *const characters_api.ScriptInstance) !JsonValue {
    var object = jsonx.newObject(allocator);
    var numbers = jsonx.newObject(allocator);
    var number_it = instance.numbers.iterator();
    while (number_it.next()) |entry| try jsonx.putInt(&numbers, allocator, entry.key_ptr.*, entry.value_ptr.*);
    if (numbers.object.count() > 0) try jsonx.put(&object, allocator, "numbers", numbers);

    var strings = jsonx.newObject(allocator);
    var string_it = instance.strings.iterator();
    while (string_it.next()) |entry| try jsonx.putSlice(&strings, allocator, entry.key_ptr.*, entry.value_ptr.*);
    if (strings.object.count() > 0) try jsonx.put(&object, allocator, "strings", strings);
    return object;
}

fn deserializeCharScripts(ch: *cdb.char_data, scripts: JsonValue) !void {
    if (scripts != .object) return error.ExpectedObject;
    var it = scripts.object.iterator();
    while (it.next()) |entry| {
        const id = entry.key_ptr.*;
        const item = entry.value_ptr.*;
        const id_z = try std.heap.page_allocator.dupeZ(u8, id);
        defer std.heap.page_allocator.free(id_z);
        _ = cdb.char_script_add(ch, id_z.ptr);
        if (item != .object) continue;
        if (jsonx.field(item, "numbers")) |nums| {
            if (nums == .object) {
                var nit = nums.object.iterator();
                while (nit.next()) |ne| {
                    if (ne.value_ptr.* == .integer) {
                        const kz = try std.heap.page_allocator.dupeZ(u8, ne.key_ptr.*);
                        defer std.heap.page_allocator.free(kz);
                        cdb.char_script_number_set(ch, id_z.ptr, kz.ptr, ne.value_ptr.*.integer);
                    }
                }
            }
        }
        if (jsonx.field(item, "strings")) |strs| {
            if (strs == .object) {
                var sit = strs.object.iterator();
                while (sit.next()) |se| {
                    if (se.value_ptr.* == .string) {
                        const kz = try std.heap.page_allocator.dupeZ(u8, se.key_ptr.*);
                        defer std.heap.page_allocator.free(kz);
                        const vz = try std.heap.page_allocator.dupeZ(u8, se.value_ptr.*.string);
                        defer std.heap.page_allocator.free(vz);
                        cdb.char_script_text_set(ch, id_z.ptr, kz.ptr, vz.ptr);
                    }
                }
            }
        }
    }
}
