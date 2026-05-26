const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");

pub const JsonValue = jsonx.JsonValue;

pub const CharacterJsonMode = enum { player, npc, npc_prototype };
pub const DeserializeOptions = struct {
    c_allocator: std.mem.Allocator = std.heap.c_allocator,
    mode: CharacterJsonMode = .npc,
};

pub fn serializeCharacter(allocator: std.mem.Allocator, ch: *cdb.char_data, mode: CharacterJsonMode) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putSlice(&object, allocator, "kind", switch (mode) {
        .player => "player",
        .npc => "npc",
        .npc_prototype => "npc_prototype",
    });

    // Instructions for AI AGENT:
    // there is a "stats" object that'll store key-value pairs of string->int64_t, like "stats": {"strength": 50, ...}
    // there is a "meters" object that stores key-value pairs of string->float, like "meters": {"powerlevel": 0.5, ...}

    // END AI AGENT INSTRUCTIONS for JSON preparations.

    if (mode != .npc_prototype) {
        // fields common to all instances, including players.
        try jsonx.putInt(&object, allocator, "id", cdb.char_id_get(ch));
        // INSTRUCTIONS FOR AI AGENT BELOW HERE:
        // char_data::bodyparts: a bitflags field.

        // char_data::{hairl, hairs, hairc, skin, eye, distfea, aura, distfea, position, skill_slots, gold, bank_gold, suppression, tail_growth, rage_meter, fury, mimic, altitude, kaioken, hometown}: these are direct fields.

        // char_data::{health, energy, stamina, life}: these are "meters" named "powerlevel", "ki", "stamina", and "lifeforce". They go in a "meters" object, like "meters": {"powerlevel": 1.0, "ki": 0.7, ...}

        // char_data::exp: this is a "stats" named "experience". it can be ommitted if zero.

        // END AI AGENT INSTRUCTIONS for non-npc_prototype modes
    }

    if (mode == .npc) {
        // npc specific fields go here.

        // INSTRUCTIONS FOR AI AGENT BELOW HERE:

        // char_data::{personality}: these are direct fields.

        // END AI AGENT INSTRUCTIONS for npc instance mode
    }

    if (mode == .npc_prototype) {
        // npc prototype specific fields go here.

        // INSTRUCTIONS FOR AI AGENT BELOW HERE:
        // char_data::proto_script is needed here...

        // END AI AGENT INSTRUCTIONS for npc_prototype mode
    }

    if (mode != .player) {
        // fields common to npcs and npc_prototypes
        try jsonx.putInt(&object, allocator, "proto_id", cdb.char_proto_id_get(ch));

        // INSTRUCTIONS FOR AI AGENT BELOW HERE:
        // When deserializing, make sure that player_specials is set to &dummy_mob.

        // char_data::act is serialized as mob_flags
    }

    if (mode == .player) {
        // player-specific fields go here.

        // INSTRUCTIONS FOR AI AGENT BELOW HERE:
        // When deserializing, must instantiate a player_special_data struct and assign its pointer to ch->player_specials.

        // player_special_data::{load_room, wimp_level, freeze_level, invis_level, poofin, poofout, murder, racial_pref, speaking, page_length}: these are direct fields

        // player_special_data::pref: a bit flags
        // player_special_data::conditions as ordered array

        // player_special_data::class_skill_points[NUM_CLASSES]: this is GET_PRACTICES, aka class_skill_points[ch->chclass]. save as "stats": { "practices": ... }

        // player_special_data::train* (trainint, trainspd, etc): save as "stats": {"train_intelligence": value, "train_speed": value, ...} these are named stats.
        // End player_special_data section. The other fields on it can be ignored.

        // char_data::skills: serialize as a json object. We'll use the loaded global skills table for the names of skills. so for instance, "skills": {"punch": {"base": 50, "perf": 0}, ...}, ignore the mod field on skill_data, it's runtime only. Note that there are far many more slots in the array than exist, so you'll have to iterate the global skills to know which ones to save.
        // Only save skills where base or perf is nonzero. You can omit zero fields.

        // char_data::{rdisplay, voice, forgeting, forgetcount, lastint, sleeptime, cooldown, backstabcool, con_cooldown, gooptime, death_type, dcount, droom, deathtime, majinizer, majinize, transclass, preference, relax_count, radar1, radar2, radar3}: these are direct fields.

        // char_data::act is saved as player_flags, which is a bitfield.

        // char_data::upgrade: consider this a "stat" named "upgrades", like "strength". Only include if nonzero.

        // char_data::{lboard, limbs, genome, bonuses, transcost}: these are direct arrays.

        // char_data::{moltexp, moltlevel}: these are "stats" named "molt_experience" and "molt_level". Only serialize if nonzero.

        // END AI AGENT INSTRUCTIONS for player mode

    }

    // general shared fields go down here.
    // AI AGENT INSTRUCTIONS:
    // char_data::{basepl, baseki, basest}: these are "stats" named "powerlevel", "ki", and "stamina"

    // char_data::real_abils maps the "stats" object, keeping in mind that cha is "speed" and dex is "agility". the others are "strength", "intelligence", "wisdom", and "constitution". these should be stored as their full names, not abbreviated.

    // char_data::{height, weight, alignment}: these are direct fields

    // END AI AGENT INSTRUCTIONS for general shared fields.

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
    try jsonx.put(&object, allocator, "admin_flags", try jsonx.serializeFlags(allocator, ch, 128, admFlagged));

    return object;
}

pub fn deserializeCharacter(ch: *cdb.char_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;

    if (options.mode != .npc_prototype) {
        // fields common to all instances, including players.
        if (try jsonx.intField(value, "id", i64)) |v| cdb.char_id_set(ch, v);
    }

    if (options.mode != .player) {
        // Fields for only npcs and npc prototypes.
        if (try jsonx.intField(value, "proto_id", cdb.mob_vnum)) |v| cdb.char_proto_id_set(ch, v);
    }

    if (options.mode == .player) {
        // player-specific fields go here.
    }

    // general shared fields go down here.
    if (try jsonx.stringField(value, "name")) |v| try setString(options.c_allocator, ch, v, cdb.char_name_set);
    if (try jsonx.stringField(value, "description")) |v| try setString(options.c_allocator, ch, v, cdb.char_description_set);
    if (try jsonx.stringField(value, "short_description")) |v| try setString(options.c_allocator, ch, v, cdb.char_short_description_set);
    if (try jsonx.stringField(value, "long_description")) |v| try setString(options.c_allocator, ch, v, cdb.char_long_description_set);
    if (try jsonx.stringField(value, "title")) |v| try setString(options.c_allocator, ch, v, cdb.char_title_set);
    if (try jsonx.intField(value, "class", c_int)) |v| cdb.char_class_set(ch, v);
    if (try jsonx.intField(value, "race", c_int)) |v| cdb.char_race_set(ch, v);
    if (try jsonx.intField(value, "size", c_int)) |v| cdb.char_size_set(ch, v);
    if (try jsonx.intField(value, "sex", c_int)) |v| cdb.char_sex_set(ch, v);
    if (try jsonx.intField(value, "admin_level", c_int)) |v| cdb.char_admlevel_set(ch, v);
    if (jsonx.field(value, "admin_flags")) |flags| try jsonx.deserializeFlags(ch, flags, 128, admFlagSet);
}

fn setString(allocator: std.mem.Allocator, ch: *cdb.char_data, value: []const u8, comptime setter: anytype) !void {
    const z = try allocator.dupeZ(u8, value);
    defer allocator.free(z);
    setter(ch, z);
}

fn admFlagged(ch: *cdb.char_data, pos: c_int) bool {
    return cdb.char_admflagged(ch, pos);
}
fn admFlagSet(ch: *cdb.char_data, pos: c_int, value: bool) void {
    cdb.char_admflag_set(ch, pos, value);
}
