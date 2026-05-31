const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");
const bitflags = @import("flags.zig");
const dgscripts_json = @import("dgscripts_json.zig");
const characters_api = @import("characters_api.zig");
const lua_api = @import("lua_api.zig");

pub const JsonValue = jsonx.JsonValue;

pub const CharacterJsonMode = enum { player, npc, npc_prototype };
pub const DeserializeOptions = struct {
    c_allocator: std.mem.Allocator = std.heap.c_allocator,
    mode: CharacterJsonMode = .npc,
};

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;
extern fn affect_to_char(ch: *cdb.char_data, af: *cdb.affected_type) void;
extern fn affectv_to_char(ch: *cdb.char_data, af: *cdb.affected_type) void;

pub fn serializeCharacter(allocator: std.mem.Allocator, ch: *cdb.char_data, mode: CharacterJsonMode) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putSlice(&object, allocator, "kind", switch (mode) {
        .player => "player",
        .npc => "npc",
        .npc_prototype => "npc_prototype",
    });

    var stats = jsonx.newObject(allocator);
    var meters = jsonx.newObject(allocator);

    try putStat(&stats, allocator, "powerlevel", ch.basepl, false);
    try putStat(&stats, allocator, "ki", ch.baseki, false);
    try putStat(&stats, allocator, "stamina", ch.basest, false);
    try putStat(&stats, allocator, "strength", ch.real_abils.str, false);
    try putStat(&stats, allocator, "intelligence", ch.real_abils.intel, false);
    try putStat(&stats, allocator, "wisdom", ch.real_abils.wis, false);
    try putStat(&stats, allocator, "agility", ch.real_abils.dex, false);
    try putStat(&stats, allocator, "constitution", ch.real_abils.con, false);
    try putStat(&stats, allocator, "speed", ch.real_abils.cha, false);
    try putStat(&stats, allocator, "height", ch.height, false);
    try putStat(&stats, allocator, "weight", ch.weight, false);
    try putStat(&stats, allocator, "alignment", ch.alignment, false);

    try jsonx.putString(&object, allocator, "name", cdb.char_name_get(ch));
    try jsonx.putString(&object, allocator, "description", cdb.char_description_get(ch));
    try jsonx.putString(&object, allocator, "short_description", cdb.char_short_description_get(ch));
    try jsonx.putString(&object, allocator, "long_description", cdb.char_long_description_get(ch));
    try jsonx.putString(&object, allocator, "title", cdb.char_title_get(ch));
    try jsonx.putInt(&object, allocator, "class", cdb.char_class_get(ch));
    try jsonx.putInt(&object, allocator, "race", cdb.char_race_get(ch));
    try jsonx.putInt(&object, allocator, "size", cdb.char_size_get(ch));
    try jsonx.putInt(&object, allocator, "sex", cdb.char_sex_get(ch));
    try jsonx.putInt(&object, allocator, "level", ch.level);
    try jsonx.putInt(&object, allocator, "admin_level", cdb.char_admlevel_get(ch));
    try jsonx.putNonEmpty(&object, allocator, "admin_flags", try jsonx.serializeFlags(allocator, ch, 128, admFlagged));
    try jsonx.putInt(&object, allocator, "height", ch.height);
    try jsonx.putInt(&object, allocator, "weight", ch.weight);

    if (mode != .npc_prototype) {
        // fields common to all instances, including players.
        try jsonx.putInt(&object, allocator, "id", cdb.char_id_get(ch));
        try putStat(&stats, allocator, "money", ch.gold, true);
        try putStat(&stats, allocator, "money_bank", ch.bank_gold, true);
        try putStat(&stats, allocator, "experience", ch.exp, true);
        try putMeter(&meters, allocator, "powerlevel", ch.health);
        try putMeter(&meters, allocator, "ki", ch.energy);
        try putMeter(&meters, allocator, "stamina", ch.stamina);
        try putMeter(&meters, allocator, "lifeforce", ch.life);
        try jsonx.putInt(&object, allocator, "hair_length", ch.hairl);
        try jsonx.putInt(&object, allocator, "hair_style", ch.hairs);
        try jsonx.putInt(&object, allocator, "hair_color", ch.hairc);
        try jsonx.putInt(&object, allocator, "skin", ch.skin);
        try jsonx.putInt(&object, allocator, "eye", ch.eye);
        try jsonx.putInt(&object, allocator, "distinguishing_feature", ch.distfea);
        try jsonx.putInt(&object, allocator, "aura", ch.aura);
        try jsonx.putInt(&object, allocator, "position", ch.position);
        try putStat(&stats, allocator, "skill_slots", ch.skill_slots, true);
        try putStat(&stats, allocator, "suppression", ch.suppression, true);
        try putStat(&stats, allocator, "fury", ch.fury, true);
        try putStat(&stats, allocator, "kaioken", ch.kaioken, true);
        try jsonx.putInt(&object, allocator, "suppression", ch.suppression);
        try jsonx.putInt(&object, allocator, "tail_growth", ch.tail_growth);
        try jsonx.putInt(&object, allocator, "rage_meter", ch.rage_meter);
        try jsonx.putInt(&object, allocator, "fury", ch.fury);
        try jsonx.putInt(&object, allocator, "mimic", ch.mimic);
        try jsonx.putInt(&object, allocator, "altitude", ch.altitude);
        try jsonx.putInt(&object, allocator, "kaioken", ch.kaioken);
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
        try jsonx.put(&object, allocator, "conditions", try serializeIntArray(allocator, ch.conditions[0..]));
        try jsonx.put(&object, allocator, "class_practices", try serializeIntArray(allocator, ch.class_skill_points[0..]));
        try jsonx.put(&object, allocator, "color_choices", try serializeStringArray(allocator, ch.color_choices[0..]));
        if (validClassIndex(ch.chclass)) {
            const class_index: usize = @intCast(ch.chclass);
            try putStat(&stats, allocator, "practices", ch.class_skill_points[class_index], true);
        }
        try putStat(&stats, allocator, "train_strength", ch.trainstr, true);
        try putStat(&stats, allocator, "train_intelligence", ch.trainint, true);
        try putStat(&stats, allocator, "train_constitution", ch.traincon, true);
        try putStat(&stats, allocator, "train_wisdom", ch.trainwis, true);
        try putStat(&stats, allocator, "train_agility", ch.trainagl, true);
        try putStat(&stats, allocator, "train_speed", ch.trainspd, true);
        try jsonx.putString(&object, allocator, "user", ch.loguser);
        try jsonx.putString(&object, allocator, "clan", ch.clan);
        try jsonx.putString(&object, allocator, "feature", ch.feature);
        try jsonx.putString(&object, allocator, "rdisplay", ch.rdisplay);
        try jsonx.putString(&object, allocator, "voice", ch.voice);
        try jsonx.put(&object, allocator, "time", try serializeTime(allocator, ch));
        try jsonx.putInt(&object, allocator, "forgeting", ch.forgeting);
        try jsonx.putInt(&object, allocator, "forgetcount", ch.forgetcount);
        try jsonx.putInt(&object, allocator, "lastint", ch.lastint);
        try jsonx.putInt(&object, allocator, "sleeptime", ch.sleeptime);
        try jsonx.putInt(&object, allocator, "cooldown", ch.cooldown);
        try jsonx.putInt(&object, allocator, "backstabcool", ch.backstabcool);
        try jsonx.putInt(&object, allocator, "con_cooldown", ch.con_cooldown);
        try jsonx.putInt(&object, allocator, "sd_cooldown", ch.con_sdcooldown);
        try jsonx.putInt(&object, allocator, "gooptime", ch.gooptime);
        try jsonx.putInt(&object, allocator, "death_type", ch.death_type);
        try putStat(&stats, allocator, "death_count", ch.dcount, true);
        try jsonx.putInt(&object, allocator, "droom", ch.droom);
        try jsonx.putInt(&object, allocator, "deathtime", ch.deathtime);
        try jsonx.putInt(&object, allocator, "reward_time", ch.rewtime);
        try jsonx.putInt(&object, allocator, "majinizer", ch.majinizer);
        try jsonx.putInt(&object, allocator, "majinize", ch.majinize);
        try jsonx.putInt(&object, allocator, "transclass", ch.transclass);
        try jsonx.putInt(&object, allocator, "preference", ch.preference);
        try jsonx.putInt(&object, allocator, "relax_count", ch.relax_count);
        try jsonx.putInt(&object, allocator, "rp", ch.rp);
        try jsonx.putInt(&object, allocator, "total_rp", ch.trp);
        try jsonx.putInt(&object, allocator, "clank_rank", ch.crank);
        try jsonx.putInt(&object, allocator, "last_play", ch.lastpl);
        try jsonx.putInt(&object, allocator, "boosts", ch.boosts);
        try putStat(&stats, allocator, "life_percent", ch.lifeperc, true);
        try jsonx.putInt(&object, allocator, "life_percent", ch.lifeperc);
        try jsonx.putInt(&object, allocator, "damage_mod", ch.damage_mod);
        try jsonx.putInt(&object, allocator, "absorbs", ch.absorbs);
        try jsonx.putInt(&object, allocator, "ingest_learned", ch.ingestLearned);
        try jsonx.putInt(&object, allocator, "bless_level", ch.blesslvl);
        try jsonx.putInt(&object, allocator, "radar1", ch.radar1);
        try jsonx.putInt(&object, allocator, "radar2", ch.radar2);
        try jsonx.putInt(&object, allocator, "radar3", ch.radar3);
        try jsonx.putNonEmpty(&object, allocator, "player_flags", try jsonx.serializeFlags(allocator, ch, cdb.NUM_PLR_FLAGS, actFlagged));
        try jsonx.putNonEmpty(&object, allocator, "affects", try serializeAffects(allocator, ch.affected));
        try putStat(&stats, allocator, "upgrades", ch.upgrade, true);
        try putStat(&stats, allocator, "molt_experience", ch.moltexp, true);
        try putStat(&stats, allocator, "molt_level", ch.moltlevel, true);
        try putStat(&stats, allocator, "armor", ch.armor, true);
        try jsonx.putNonEmpty(&object, allocator, "skills", try serializeSkills(allocator, ch));
        try jsonx.put(&object, allocator, "lboard", try serializeIntArray(allocator, ch.lboard[0..]));
        try jsonx.put(&object, allocator, "limbs", try serializeIntArray(allocator, ch.limb_condition[0..]));
        try jsonx.put(&object, allocator, "genome", try serializeIntArray(allocator, ch.genome[0..]));
        try jsonx.put(&object, allocator, "bonuses", try serializeIntArray(allocator, ch.bonuses[0..]));
        try jsonx.put(&object, allocator, "transcost", try serializeIntArray(allocator, ch.transcost[0..]));
    }

    if (stats.object.count() > 0) try jsonx.put(&object, allocator, "stats", stats);
    if (meters.object.count() > 0) try jsonx.put(&object, allocator, "meters", meters);
    try jsonx.putNonEmpty(&object, allocator, "conditions_v2", try serializeConditions(allocator, ch));

    return object;
}

pub fn serializeMobPrototype(allocator: std.mem.Allocator, proto: *cdb.mob_proto_data) !JsonValue {
    var ch: cdb.char_data = std.mem.zeroes(cdb.char_data);
    mobProtoToCharacter(&ch, proto);
    return serializeCharacter(allocator, &ch, .npc_prototype);
}

pub fn deserializeMobPrototype(proto: *cdb.mob_proto_data, options: DeserializeOptions, value: JsonValue) !void {
    var ch: cdb.char_data = std.mem.zeroes(cdb.char_data);
    ch.vnum = proto.vnum;
    try deserializeCharacter(&ch, .{ .c_allocator = options.c_allocator, .mode = .npc_prototype }, value);
    characterToMobProto(proto, &ch);
}

fn mobProtoToCharacter(ch: *cdb.char_data, proto: *const cdb.mob_proto_data) void {
    ch.vnum = proto.vnum;
    ch.name = proto.name;
    ch.short_descr = proto.short_descr;
    ch.long_descr = proto.long_descr;
    ch.description = proto.description;
    ch.title = proto.title;
    ch.size = proto.size;
    ch.sex = proto.sex;
    ch.race = proto.race;
    ch.chclass = proto.chclass;
    ch.alignment = proto.alignment;
    ch.weight = proto.weight;
    ch.height = proto.height;
    ch.level = proto.level;
    ch.race_level = proto.race_level;
    ch.level_adj = proto.level_adj;
    ch.gold = proto.gold;
    ch.exp = proto.exp;
    ch.basepl = proto.basepl;
    ch.baseki = proto.baseki;
    ch.basest = proto.basest;
    ch.armor = proto.armor;
    ch.real_abils = proto.real_abils;
    ch.aff_abils = proto.aff_abils;
    ch.mob_specials = proto.mob_specials;
    ch.position = proto.position;
    ch.speaking = proto.speaking;
    ch.act = proto.act;
    ch.affected_by = proto.affected_by;
    ch.admflags = proto.admflags;
    ch.admlevel = proto.admlevel;
    ch.proto_script = proto.proto_script;
}

fn characterToMobProto(proto: *cdb.mob_proto_data, ch: *const cdb.char_data) void {
    proto.vnum = ch.vnum;
    proto.name = ch.name;
    proto.short_descr = ch.short_descr;
    proto.long_descr = ch.long_descr;
    proto.description = ch.description;
    proto.title = ch.title;
    proto.size = ch.size;
    proto.sex = ch.sex;
    proto.race = ch.race;
    proto.chclass = ch.chclass;
    proto.alignment = ch.alignment;
    proto.weight = ch.weight;
    proto.height = ch.height;
    proto.level = ch.level;
    proto.race_level = ch.race_level;
    proto.level_adj = ch.level_adj;
    proto.gold = ch.gold;
    proto.exp = ch.exp;
    proto.basepl = ch.basepl;
    proto.baseki = ch.baseki;
    proto.basest = ch.basest;
    proto.armor = ch.armor;
    proto.real_abils = ch.real_abils;
    proto.aff_abils = ch.aff_abils;
    proto.mob_specials = ch.mob_specials;
    proto.position = ch.position;
    proto.speaking = ch.speaking;
    proto.act = ch.act;
    proto.affected_by = ch.affected_by;
    proto.admflags = ch.admflags;
    proto.admlevel = ch.admlevel;
    proto.proto_script = ch.proto_script;
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
    if (try jsonx.intField(value, "level", c_int)) |v| ch.level = v;
    if (jsonx.field(value, "admin_flags")) |flags| try jsonx.deserializeFlags(ch, flags, 128, admFlagSet);
    if (try jsonx.intField(value, "height", u8)) |v| ch.height = v;
    if (try jsonx.intField(value, "weight", u8)) |v| ch.weight = v;
    if (try jsonx.intField(value, "alignment", c_int)) |v| {
        ch.alignment = v;
        _ = cdb.char_stat_set(ch, "alignment", v);
    }
    if (jsonx.field(value, "stats")) |stats| try deserializeSharedStats(ch, stats);

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
        if (try jsonx.intField(value, "position", i8)) |v| ch.position = v;
        if (try jsonx.intField(value, "skill_slots", c_int)) |v| {
            ch.skill_slots = v;
            _ = cdb.char_stat_set(ch, "skill_slots", v);
        }
        if (try jsonx.intField(value, "suppression", i64)) |v| ch.suppression = v;
        if (try jsonx.intField(value, "tail_growth", c_int)) |v| ch.tail_growth = v;
        if (try jsonx.intField(value, "rage_meter", c_int)) |v| ch.rage_meter = v;
        if (try jsonx.intField(value, "fury", c_short)) |v| ch.fury = v;
        if (try jsonx.intField(value, "mimic", c_int)) |v| ch.mimic = v;
        if (try jsonx.intField(value, "altitude", c_int)) |v| ch.altitude = v;
        if (try jsonx.intField(value, "kaioken", c_int)) |v| ch.kaioken = v;
        if (try jsonx.intField(value, "hometown", cdb.room_vnum)) |v| ch.hometown = v;
        if (jsonx.field(value, "bodyparts")) |flags| try jsonx.deserializeFlags(ch, flags, cdb.NUM_AFF_FLAGS, bodypartFlagSet);
        if (jsonx.field(value, "affected_by")) |flags| try jsonx.deserializeFlags(ch, flags, cdb.NUM_AFF_FLAGS, affectedFlagSet);
        if (jsonx.field(value, "meters")) |meters| try deserializeMeters(ch, meters);
        if (jsonx.field(value, "conditions_v2")) |conditions| try deserializeConditions(ch, conditions);
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
        if (jsonx.field(value, "conditions")) |items| try deserializeIntArray(ch.conditions[0..], items);
        if (jsonx.field(value, "class_practices")) |items| try deserializeIntArray(ch.class_skill_points[0..], items);
        if (jsonx.field(value, "color_choices")) |items| try deserializeStringArray(options.c_allocator, ch.color_choices[0..], items);
        if (jsonx.field(value, "stats")) |stats| try deserializePlayerStats(ch, stats);
        try setPointerStringField(options.c_allocator, value, "user", &ch.loguser, setRawString);
        try setPointerStringField(options.c_allocator, value, "clan", &ch.clan, setRawString);
        try setPointerStringField(options.c_allocator, value, "feature", &ch.feature, setRawString);
        try setPointerStringField(options.c_allocator, value, "rdisplay", &ch.rdisplay, setRawString);
        try setPointerStringField(options.c_allocator, value, "voice", &ch.voice, setRawString);
        if (jsonx.field(value, "time")) |time| try deserializeTime(ch, time);
        if (try jsonx.intField(value, "forgeting", c_int)) |v| ch.forgeting = v;
        if (try jsonx.intField(value, "forgetcount", c_int)) |v| ch.forgetcount = v;
        if (try jsonx.intField(value, "lastint", cdb.time_t)) |v| ch.lastint = v;
        if (try jsonx.intField(value, "sleeptime", c_int)) |v| ch.sleeptime = v;
        if (try jsonx.intField(value, "cooldown", c_int)) |v| ch.cooldown = v;
        if (try jsonx.intField(value, "backstabcool", c_int)) |v| ch.backstabcool = v;
        if (try jsonx.intField(value, "con_cooldown", c_int)) |v| ch.con_cooldown = v;
        if (try jsonx.intField(value, "sd_cooldown", c_int)) |v| ch.con_sdcooldown = v;
        if (try jsonx.intField(value, "gooptime", c_int)) |v| ch.gooptime = v;
        if (try jsonx.intField(value, "death_type", c_int)) |v| ch.death_type = v;
        if (try jsonx.intField(value, "dcount", c_int)) |v| {
            ch.dcount = v;
            _ = cdb.char_stat_set(ch, "death_count", v);
        }
        if (try jsonx.intField(value, "droom", cdb.room_vnum)) |v| ch.droom = v;
        if (try jsonx.intField(value, "deathtime", cdb.time_t)) |v| ch.deathtime = v;
        if (try jsonx.intField(value, "reward_time", cdb.time_t)) |v| ch.rewtime = v;
        if (try jsonx.intField(value, "majinizer", i64)) |v| ch.majinizer = v;
        if (try jsonx.intField(value, "majinize", c_int)) |v| ch.majinize = v;
        if (try jsonx.intField(value, "transclass", c_int)) |v| ch.transclass = v;
        if (try jsonx.intField(value, "preference", c_int)) |v| ch.preference = v;
        if (try jsonx.intField(value, "relax_count", c_int)) |v| ch.relax_count = v;
        if (try jsonx.intField(value, "rp", c_int)) |v| ch.rp = v;
        if (try jsonx.intField(value, "total_rp", c_int)) |v| ch.trp = v;
        if (try jsonx.intField(value, "clank_rank", c_int)) |v| ch.crank = v;
        if (try jsonx.intField(value, "last_play", cdb.time_t)) |v| ch.lastpl = v;
        if (try jsonx.intField(value, "boosts", c_int)) |v| ch.boosts = v;
        if (try jsonx.intField(value, "life_percent", c_int)) |v| ch.lifeperc = v;
        if (try jsonx.intField(value, "damage_mod", c_int)) |v| ch.damage_mod = v;
        if (try jsonx.intField(value, "armor", c_int)) |v| ch.armor = v;
        if (try jsonx.intField(value, "absorbs", c_int)) |v| ch.absorbs = v;
        if (try jsonx.intField(value, "ingest_learned", c_int)) |v| ch.ingestLearned = v;
        if (try jsonx.intField(value, "bless_level", c_int)) |v| ch.blesslvl = v;
        if (try jsonx.intField(value, "radar1", cdb.room_vnum)) |v| ch.radar1 = v;
        if (try jsonx.intField(value, "radar2", cdb.room_vnum)) |v| ch.radar2 = v;
        if (try jsonx.intField(value, "radar3", cdb.room_vnum)) |v| ch.radar3 = v;
        if (jsonx.field(value, "player_flags")) |flags| try jsonx.deserializeFlags(ch, flags, cdb.NUM_PLR_FLAGS, actFlagSet);
        if (jsonx.field(value, "affects")) |items| try deserializeAffects(ch, items);
        if (jsonx.field(value, "skills")) |skills| try deserializeSkills(ch, skills);
        if (jsonx.field(value, "lboard")) |items| try deserializeIntArray(ch.lboard[0..], items);
        if (jsonx.field(value, "limbs")) |items| try deserializeIntArray(ch.limb_condition[0..], items);
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

fn putStat(object: *JsonValue, allocator: std.mem.Allocator, name: []const u8, value: anytype, omit_zero: bool) !void {
    if (omit_zero and value == 0) return;
    try jsonx.putInt(object, allocator, name, value);
}

fn putMeter(object: *JsonValue, allocator: std.mem.Allocator, name: []const u8, value: f64) !void {
    try jsonx.putFloat(object, allocator, name, value);
}

fn deserializeSharedStats(ch: *cdb.char_data, stats: JsonValue) !void {
    if (stats != .object) return error.ExpectedObject;
    if (try jsonx.intField(stats, "powerlevel", i64)) |v| {
        ch.basepl = v;
        _ = cdb.char_stat_set(ch, "powerlevel", v);
    }
    if (try jsonx.intField(stats, "ki", i64)) |v| {
        ch.baseki = v;
        _ = cdb.char_stat_set(ch, "ki", v);
    }
    if (try jsonx.intField(stats, "stamina", i64)) |v| {
        ch.basest = v;
        _ = cdb.char_stat_set(ch, "stamina", v);
    }
    if (try jsonx.intField(stats, "strength", i8)) |v| {
        ch.real_abils.str = v;
        _ = cdb.char_stat_set(ch, "strength", v);
    }
    if (try jsonx.intField(stats, "intelligence", i8)) |v| {
        ch.real_abils.intel = v;
        _ = cdb.char_stat_set(ch, "intelligence", v);
    }
    if (try jsonx.intField(stats, "wisdom", i8)) |v| {
        ch.real_abils.wis = v;
        _ = cdb.char_stat_set(ch, "wisdom", v);
    }
    if (try jsonx.intField(stats, "agility", i8)) |v| {
        ch.real_abils.dex = v;
        _ = cdb.char_stat_set(ch, "agility", v);
    }
    if (try jsonx.intField(stats, "constitution", i8)) |v| {
        ch.real_abils.con = v;
        _ = cdb.char_stat_set(ch, "constitution", v);
    }
    if (try jsonx.intField(stats, "speed", i8)) |v| {
        ch.real_abils.cha = v;
        _ = cdb.char_stat_set(ch, "speed", v);
    }
    if (try jsonx.intField(stats, "height", u8)) |v| {
        ch.height = v;
        _ = cdb.char_stat_set(ch, "height", v);
    }
    if (try jsonx.intField(stats, "weight", u8)) |v| {
        ch.weight = v;
        _ = cdb.char_stat_set(ch, "weight", v);
    }
    if (try jsonx.intField(stats, "alignment", c_int)) |v| {
        ch.alignment = v;
        _ = cdb.char_stat_set(ch, "alignment", v);
    }
    if (try jsonx.intField(stats, "money", c_int)) |v| {
        ch.gold = v;
        _ = cdb.char_stat_set(ch, "money", v);
    }
    if (try jsonx.intField(stats, "money_bank", c_int)) |v| {
        ch.bank_gold = v;
        _ = cdb.char_stat_set(ch, "money_bank", v);
    }
    if (try jsonx.intField(stats, "experience", i64)) |v| {
        ch.exp = v;
        _ = cdb.char_stat_set(ch, "experience", v);
    }
    if (try jsonx.intField(stats, "suppression", i64)) |v| {
        ch.suppression = v;
        _ = cdb.char_stat_set(ch, "suppression", v);
    }
    if (try jsonx.intField(stats, "fury", c_short)) |v| {
        ch.fury = v;
        _ = cdb.char_stat_set(ch, "fury", v);
    }
    if (try jsonx.intField(stats, "kaioken", c_int)) |v| {
        ch.kaioken = v;
        _ = cdb.char_stat_set(ch, "kaioken", v);
    }
}

fn deserializePlayerStats(ch: *cdb.char_data, stats: JsonValue) !void {
    if (stats != .object) return error.ExpectedObject;
    if (try jsonx.intField(stats, "practices", c_int)) |v| if (validClassIndex(ch.chclass)) {
        const class_index: usize = @intCast(ch.chclass);
        ch.class_skill_points[class_index] = v;
        _ = cdb.char_stat_set(ch, "practices", v);
    };
    if (try jsonx.intField(stats, "train_strength", c_int)) |v| {
        ch.trainstr = v;
        _ = cdb.char_stat_set(ch, "train_strength", v);
    }
    if (try jsonx.intField(stats, "train_intelligence", c_int)) |v| {
        ch.trainint = v;
        _ = cdb.char_stat_set(ch, "train_intelligence", v);
    }
    if (try jsonx.intField(stats, "train_constitution", c_int)) |v| {
        ch.traincon = v;
        _ = cdb.char_stat_set(ch, "train_constitution", v);
    }
    if (try jsonx.intField(stats, "train_wisdom", c_int)) |v| {
        ch.trainwis = v;
        _ = cdb.char_stat_set(ch, "train_wisdom", v);
    }
    if (try jsonx.intField(stats, "train_agility", c_int)) |v| {
        ch.trainagl = v;
        _ = cdb.char_stat_set(ch, "train_agility", v);
    }
    if (try jsonx.intField(stats, "train_speed", c_int)) |v| {
        ch.trainspd = v;
        _ = cdb.char_stat_set(ch, "train_speed", v);
    }
    if (try jsonx.intField(stats, "upgrades", c_int)) |v| {
        ch.upgrade = v;
        _ = cdb.char_stat_set(ch, "upgrades", v);
    }
    if (try jsonx.intField(stats, "molt_experience", i64)) |v| {
        ch.moltexp = v;
        _ = cdb.char_stat_set(ch, "molt_experience", v);
    }
    if (try jsonx.intField(stats, "molt_level", c_int)) |v| {
        ch.moltlevel = v;
        _ = cdb.char_stat_set(ch, "molt_level", v);
    }
    if (try jsonx.intField(stats, "armor", c_int)) |v| {
        ch.armor = v;
        _ = cdb.char_stat_set(ch, "armor", v);
    }
    if (try jsonx.intField(stats, "skill_slots", c_int)) |v| {
        ch.skill_slots = v;
        _ = cdb.char_stat_set(ch, "skill_slots", v);
    }
    if (try jsonx.intField(stats, "death_count", c_int)) |v| {
        ch.dcount = v;
        _ = cdb.char_stat_set(ch, "death_count", v);
    }
    if (try jsonx.intField(stats, "life_percent", c_int)) |v| {
        ch.lifeperc = v;
        _ = cdb.char_stat_set(ch, "life_percent", v);
    }
}

fn deserializeMeters(ch: *cdb.char_data, meters: JsonValue) !void {
    if (meters != .object) return error.ExpectedObject;
    if (try jsonx.floatField(meters, "powerlevel", f64)) |v| {
        ch.health = v;
        _ = cdb.char_meter_set(ch, "powerlevel", characters_api.meterFloatToFixed(v));
    }
    if (try jsonx.floatField(meters, "ki", f64)) |v| {
        ch.energy = v;
        _ = cdb.char_meter_set(ch, "ki", characters_api.meterFloatToFixed(v));
    }
    if (try jsonx.floatField(meters, "stamina", f64)) |v| {
        ch.stamina = v;
        _ = cdb.char_meter_set(ch, "stamina", characters_api.meterFloatToFixed(v));
    }
    if (try jsonx.floatField(meters, "lifeforce", f64)) |v| {
        ch.life = v;
        _ = cdb.char_meter_set(ch, "lifeforce", characters_api.meterFloatToFixed(v));
    }
}

fn serializeConditions(allocator: std.mem.Allocator, ch: *cdb.char_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    if (ch.zigdata == null) return object;
    const data: *characters_api.CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var it = data.conditions.iterator();
    while (it.next()) |entry| {
        const definition = lua_api.conditionDefinition(entry.key_ptr.*) orelse continue;
        if (!definition.persistent) continue;
        try jsonx.put(&object, allocator, entry.key_ptr.*, try serializeCondition(allocator, entry.value_ptr));
    }
    return object;
}

fn serializeCondition(allocator: std.mem.Allocator, condition: *characters_api.ConditionInstance) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putInt(&object, allocator, "stacks", condition.stacks);
    try jsonx.putInt(&object, allocator, "duration", condition.duration);

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
        if (jsonx.field(item, "sources")) |sources| {
            if (sources == .array and sources.array.items.len > 0) {
                try deserializeConditionSources(ch, id_z.ptr, sources);
            } else {
                _ = cdb.char_condition_add(ch, id_z.ptr, "json", "conditions_v2");
            }
        } else {
            _ = cdb.char_condition_add(ch, id_z.ptr, "json", "conditions_v2");
        }
        if (try jsonx.intField(item, "stacks", i64)) |v| _ = cdb.char_condition_stacks_set(ch, id_z.ptr, v);
        if (try jsonx.intField(item, "duration", i64)) |v| _ = cdb.char_condition_duration_set(ch, id_z.ptr, v);
        if (jsonx.field(item, "numbers")) |numbers| try deserializeConditionNumbers(ch, id, numbers);
        if (jsonx.field(item, "strings")) |strings| try deserializeConditionStrings(ch, id, strings);
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
        _ = cdb.char_condition_add(ch, id_z, category_z.ptr, source_z.ptr);
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

fn serializeIntArray(allocator: std.mem.Allocator, values: anytype) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    for (values) |value| try array.append(.{ .integer = @intCast(value) });
    return .{ .array = array };
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

fn serializeLevels(allocator: std.mem.Allocator, ch: *cdb.char_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putInt(&object, allocator, "class", ch.level);
    try jsonx.putInt(&object, allocator, "race", ch.race_level);
    try jsonx.putInt(&object, allocator, "adjustment", ch.level_adj);
    return object;
}

fn deserializeLevels(ch: *cdb.char_data, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    if (try jsonx.intField(value, "class", c_int)) |v| ch.level = v;
    if (try jsonx.intField(value, "race", c_int)) |v| ch.race_level = v;
    if (try jsonx.intField(value, "adjustment", c_int)) |v| ch.level_adj = v;
}

fn serializeAffects(allocator: std.mem.Allocator, head: ?*cdb.affected_type) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    var current = head;
    while (current) |af| : (current = af.next) {
        var object = jsonx.newObject(allocator);
        try jsonx.putInt(&object, allocator, "type", af.type);
        try jsonx.putInt(&object, allocator, "duration", af.duration);
        try jsonx.putInt(&object, allocator, "modifier", af.modifier);
        try jsonx.putInt(&object, allocator, "location", af.location);
        try jsonx.putInt(&object, allocator, "bitvector", af.bitvector);
        try jsonx.putInt(&object, allocator, "specific", af.specific);
        try array.append(object);
    }
    return .{ .array = array };
}

fn deserializeAffects(ch: *cdb.char_data, value: JsonValue) !void {
    if (value != .array) return error.ExpectedArray;
    for (value.array.items) |item| {
        if (item != .object) return error.ExpectedObject;
        var af: cdb.affected_type = std.mem.zeroes(cdb.affected_type);
        if (try jsonx.intField(item, "type", i16)) |v| af.type = v;
        if (try jsonx.intField(item, "duration", i16)) |v| af.duration = v;
        if (try jsonx.intField(item, "modifier", c_int)) |v| af.modifier = v;
        if (try jsonx.intField(item, "location", c_int)) |v| af.location = v;
        if (try jsonx.intField(item, "bitvector", cdb.bitvector_t)) |v| af.bitvector = v;
        if (try jsonx.intField(item, "specific", c_int)) |v| af.specific = v;
        if (af.type == 0) continue;
        affect_to_char(ch, &af);
    }
}

fn serializeSkills(allocator: std.mem.Allocator, ch: *cdb.char_data) !JsonValue {
    // TODO: replace numeric keys with names once the global skill table is exposed through the DB ABI.
    var object = jsonx.newObject(allocator);
    for (ch.skills[0..], 0..) |skill, index| {
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
