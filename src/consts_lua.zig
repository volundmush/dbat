const std = @import("std");
const ct = @import("consts_table.zig");
const cdb = @import("cdb");
const zlua = @import("zlua");
const Lua = zlua.Lua;

fn pushConstTable(lua: *Lua, comptime entries: anytype) void {
    lua.newTable();
    inline for (std.meta.fields(@TypeOf(entries))) |field| {
        lua.pushInteger(@intCast(@field(entries, field.name)));
        lua.setField(-2, field.name);
    }
}

// Builds a 0-indexed Lua table from a null-or-"\n"-terminated C string array.
// Accepts any fixed-size or slice array of [*c]const u8 (or [*c]u8).
// Indices match the C constants (e.g. wear_where[WEAR_WIELD1] where WEAR_WIELD1==16).
fn pushStringArray(lua: *Lua, arr: anytype) void {
    lua.newTable();
    // For fixed-size arrays ([N]T), use the compile-time length as a bound.
    // For unsized C pointers ([*c]T), rely solely on the "\n" sentinel.
    const max_len: usize = switch (@typeInfo(@TypeOf(arr))) {
        .array => |a| a.len,
        else => 500,
    };
    var i: usize = 0;
    while (i < max_len) : (i += 1) {
        const entry = arr[i];
        if (entry == null) break;
        const s = std.mem.span(entry);
        if (std.mem.eql(u8, s, "\n")) break;
        _ = lua.pushString(s);
        lua.setIndex(-2, @intCast(i));
    }
}

pub fn register(lua: *Lua) void {
    lua.newTable(); // becomes dbat.consts

    // Integer constant tables (enum-style)
    pushConstTable(lua, ct.prf_flags);       lua.setField(-2, "prf_flags");
    pushConstTable(lua, ct.player_flags);    lua.setField(-2, "player_flags");
    pushConstTable(lua, ct.admin_flags);     lua.setField(-2, "admin_flags");
    pushConstTable(lua, ct.adm_levels);      lua.setField(-2, "adm_levels");
    pushConstTable(lua, ct.mob_flags);       lua.setField(-2, "mob_flags");
    pushConstTable(lua, ct.aff_flags);       lua.setField(-2, "aff_flags");
    pushConstTable(lua, ct.room_flags);      lua.setField(-2, "room_flags");
    pushConstTable(lua, ct.exit_flags);      lua.setField(-2, "exit_flags");
    pushConstTable(lua, ct.zone_flags);      lua.setField(-2, "zone_flags");
    pushConstTable(lua, ct.positions);       lua.setField(-2, "positions");
    pushConstTable(lua, ct.races);           lua.setField(-2, "races");
    pushConstTable(lua, ct.sexes);           lua.setField(-2, "sexes");
    pushConstTable(lua, ct.con_states);      lua.setField(-2, "con_states");
    pushConstTable(lua, ct.applies);         lua.setField(-2, "applies");
    pushConstTable(lua, ct.sizes);           lua.setField(-2, "sizes");
    pushConstTable(lua, ct.aligns);          lua.setField(-2, "aligns");
    pushConstTable(lua, ct.bonuses);         lua.setField(-2, "bonuses");
    pushConstTable(lua, ct.color_choices);   lua.setField(-2, "color_choices");
    pushConstTable(lua, ct.death_types);     lua.setField(-2, "death_types");
    pushConstTable(lua, ct.fish_states);     lua.setField(-2, "fish_states");
    pushConstTable(lua, ct.fight_prefs);     lua.setField(-2, "fight_prefs");
    pushConstTable(lua, ct.history_types);   lua.setField(-2, "history_types");
    pushConstTable(lua, ct.ocarina_songs);   lua.setField(-2, "ocarina_songs");
    pushConstTable(lua, ct.attack_types);    lua.setField(-2, "attack_types");
    pushConstTable(lua, ct.sector_types);    lua.setField(-2, "sector_types");
    pushConstTable(lua, ct.assembly_types);  lua.setField(-2, "assembly_types");
    pushConstTable(lua, ct.auction_states);  lua.setField(-2, "auction_states");
    pushConstTable(lua, ct.wield_types);     lua.setField(-2, "wield_types");
    pushConstTable(lua, ct.crit_types);      lua.setField(-2, "crit_types");
    pushConstTable(lua, ct.liquid_types);    lua.setField(-2, "liquid_types");
    pushConstTable(lua, ct.materials);       lua.setField(-2, "materials");
    pushConstTable(lua, ct.spell_levels);    lua.setField(-2, "spell_levels");
    pushConstTable(lua, ct.magic_domains);   lua.setField(-2, "magic_domains");
    pushConstTable(lua, ct.magic_schools);   lua.setField(-2, "magic_schools");
    pushConstTable(lua, ct.classes);         lua.setField(-2, "classes");
    pushConstTable(lua, ct.senseis);         lua.setField(-2, "senseis");
    pushConstTable(lua, ct.skills);          lua.setField(-2, "skills");
    pushConstTable(lua, ct.mob_trig_types);  lua.setField(-2, "mob_trig_types");
    pushConstTable(lua, ct.obj_trig_types);  lua.setField(-2, "obj_trig_types");
    pushConstTable(lua, ct.wld_trig_types);  lua.setField(-2, "wld_trig_types");
    pushConstTable(lua, ct.obj_cmd_types);   lua.setField(-2, "obj_cmd_types");
    pushConstTable(lua, ct.trig_states);     lua.setField(-2, "trig_states");
    pushConstTable(lua, ct.wear_positions);  lua.setField(-2, "wear_positions");
    pushConstTable(lua, ct.item_wear_flags); lua.setField(-2, "item_wear_flags");
    pushConstTable(lua, ct.item_types);      lua.setField(-2, "item_types");
    pushConstTable(lua, ct.item_extra_flags); lua.setField(-2, "item_extra_flags");
    pushConstTable(lua, ct.container_flags); lua.setField(-2, "container_flags");
    pushConstTable(lua, ct.distfea);         lua.setField(-2, "distfea");
    pushConstTable(lua, ct.aura_colors);     lua.setField(-2, "aura_colors");
    pushConstTable(lua, ct.eye_colors);      lua.setField(-2, "eye_colors");
    pushConstTable(lua, ct.hair_length);     lua.setField(-2, "hair_length");
    pushConstTable(lua, ct.hair_color);      lua.setField(-2, "hair_color");
    pushConstTable(lua, ct.hair_style);      lua.setField(-2, "hair_style");
    pushConstTable(lua, ct.skin_colors);     lua.setField(-2, "skin_colors");
    pushConstTable(lua, ct.annual_phases);   lua.setField(-2, "annual_phases");
    pushConstTable(lua, ct.directions);      lua.setField(-2, "directions");
    pushConstTable(lua, ct.player_conds);    lua.setField(-2, "player_conds");

    // String name arrays (0-indexed, matching C constants)
    // Equipment / Item
    pushStringArray(lua, cdb.wear_where);        lua.setField(-2, "wear_where");
    pushStringArray(lua, cdb.equipment_types);   lua.setField(-2, "equipment_types");
    pushStringArray(lua, cdb.item_types);        lua.setField(-2, "item_type_names");
    pushStringArray(lua, cdb.wear_bits);         lua.setField(-2, "item_wear_flag_names");
    pushStringArray(lua, cdb.extra_bits);        lua.setField(-2, "item_extra_flag_names");
    pushStringArray(lua, cdb.container_bits);    lua.setField(-2, "container_flag_names");
    pushStringArray(lua, cdb.material_names);    lua.setField(-2, "material_names");
    pushStringArray(lua, cdb.drinks);            lua.setField(-2, "drink_names");
    pushStringArray(lua, cdb.drinknames);        lua.setField(-2, "drink_full_names");
    pushStringArray(lua, cdb.color_liquid);      lua.setField(-2, "liquid_colors");
    pushStringArray(lua, cdb.fullness);          lua.setField(-2, "fullness_names");
    // Character
    pushStringArray(lua, cdb.size_names);        lua.setField(-2, "size_names");
    pushStringArray(lua, cdb.genders);           lua.setField(-2, "gender_names");
    pushStringArray(lua, cdb.alignments);        lua.setField(-2, "alignment_names");
    pushStringArray(lua, cdb.position_types);    lua.setField(-2, "position_names");
    pushStringArray(lua, cdb.race_names);        lua.setField(-2, "race_names");
    pushStringArray(lua, cdb.pc_race_types);     lua.setField(-2, "pc_race_names");
    pushStringArray(lua, cdb.d_race_types);      lua.setField(-2, "disguised_race_names");
    pushStringArray(lua, cdb.race_abbrevs);      lua.setField(-2, "race_abbrevs");
    pushStringArray(lua, cdb.sensei_style);      lua.setField(-2, "sensei_styles");
    pushStringArray(lua, cdb.pc_class_types);    lua.setField(-2, "class_names");
    pushStringArray(lua, cdb.class_names);       lua.setField(-2, "class_long_names");
    pushStringArray(lua, cdb.class_abbrevs);     lua.setField(-2, "class_abbrevs");
    pushStringArray(lua, cdb.affected_bits);     lua.setField(-2, "aff_flag_names");
    pushStringArray(lua, cdb.apply_types);       lua.setField(-2, "apply_names");
    pushStringArray(lua, cdb.player_bits);       lua.setField(-2, "player_flag_names");
    pushStringArray(lua, cdb.preference_bits);   lua.setField(-2, "prf_flag_names");
    pushStringArray(lua, cdb.action_bits);       lua.setField(-2, "mob_flag_names");
    // World
    pushStringArray(lua, cdb.dirs);              lua.setField(-2, "direction_names");
    pushStringArray(lua, cdb.abbr_dirs);         lua.setField(-2, "direction_abbrevs");
    pushStringArray(lua, cdb.sector_types);      lua.setField(-2, "sector_type_names");
    pushStringArray(lua, cdb.room_bits);         lua.setField(-2, "room_flag_names");
    // Combat / Skills
    pushStringArray(lua, cdb.attack_names);      lua.setField(-2, "attack_names");
    pushStringArray(lua, cdb.attack_names_comp); lua.setField(-2, "attack_display_names");
    pushStringArray(lua, cdb.weapon_type);       lua.setField(-2, "weapon_type_names");
    pushStringArray(lua, cdb.armor_type);        lua.setField(-2, "armor_type_names");
    pushStringArray(lua, cdb.wield_names);       lua.setField(-2, "wield_names");
    pushStringArray(lua, cdb.crit_type);         lua.setField(-2, "crit_type_names");
    // Magic
    pushStringArray(lua, cdb.spell_schools);     lua.setField(-2, "spell_school_names");
    // Admin / Meta
    pushStringArray(lua, cdb.connected_types);   lua.setField(-2, "con_state_names");
    pushStringArray(lua, cdb.admin_flags);       lua.setField(-2, "admin_flag_names");
    pushStringArray(lua, cdb.admin_level_names); lua.setField(-2, "admin_level_names");
    pushStringArray(lua, cdb.trig_types);        lua.setField(-2, "mob_trig_names");
    pushStringArray(lua, cdb.otrig_types);       lua.setField(-2, "obj_trig_names");
    pushStringArray(lua, cdb.wtrig_types);       lua.setField(-2, "wld_trig_names");
    // Social / Flavor
    pushStringArray(lua, cdb.song_types);        lua.setField(-2, "song_names");
    pushStringArray(lua, cdb.history_types);     lua.setField(-2, "history_channel_names");
    pushStringArray(lua, cdb.cchoice_names);     lua.setField(-2, "color_choice_names");
    // Appearance
    pushStringArray(lua, cdb.eye_types);         lua.setField(-2, "eye_color_names");
    pushStringArray(lua, cdb.aura_types);        lua.setField(-2, "aura_color_names");
    pushStringArray(lua, cdb.hairl_types);       lua.setField(-2, "hair_length_names");
    pushStringArray(lua, cdb.hairc_types);       lua.setField(-2, "hair_color_names");
    pushStringArray(lua, cdb.hairs_types);       lua.setField(-2, "hair_style_names");
    pushStringArray(lua, cdb.skin_types);        lua.setField(-2, "skin_color_names");

    // Timing constants
    lua.pushInteger(300);  lua.setField(-2, "secs_per_mud_hour");
    lua.newTable();
    lua.pushInteger(10);  lua.setField(-2, "one_sec");
    lua.pushInteger(20);  lua.setField(-2, "two_sec");
    lua.setField(-2, "pulses");

    // Bound constants for iterating string arrays
    lua.pushInteger(cdb.NUM_WEARS);          lua.setField(-2, "NUM_WEARS");
    lua.pushInteger(cdb.NUM_SIZES);          lua.setField(-2, "NUM_SIZES");
    lua.pushInteger(cdb.NUM_SEX);            lua.setField(-2, "NUM_SEX");
    lua.pushInteger(cdb.NUM_RACES);          lua.setField(-2, "NUM_RACES");
    lua.pushInteger(cdb.NUM_ALIGNS);         lua.setField(-2, "NUM_ALIGNS");
    lua.pushInteger(cdb.NUM_OF_DIRS);        lua.setField(-2, "NUM_OF_DIRS");
    lua.pushInteger(cdb.NUM_MATERIALS);      lua.setField(-2, "NUM_MATERIALS");
    lua.pushInteger(cdb.NUM_ITEM_TYPES);     lua.setField(-2, "NUM_ITEM_TYPES");
    lua.pushInteger(cdb.NUM_ITEM_FLAGS);     lua.setField(-2, "NUM_ITEM_FLAGS");
    lua.pushInteger(cdb.NUM_MOB_FLAGS);      lua.setField(-2, "NUM_MOB_FLAGS");
    lua.pushInteger(cdb.NUM_AFF_FLAGS);      lua.setField(-2, "NUM_AFF_FLAGS");
    lua.pushInteger(cdb.NUM_ROOM_FLAGS);     lua.setField(-2, "NUM_ROOM_FLAGS");
    lua.pushInteger(cdb.NUM_APPLIES);        lua.setField(-2, "NUM_APPLIES");
    lua.pushInteger(cdb.NUM_PLR_FLAGS);      lua.setField(-2, "NUM_PLR_FLAGS");
    lua.pushInteger(cdb.NUM_PRF_FLAGS);      lua.setField(-2, "NUM_PRF_FLAGS");

    // BFS pathfinding result constants (from graph.h / room:find_first_step)
    lua.newTable();
    lua.pushInteger(cdb.BFS_ERROR);        lua.setField(-2, "ERROR");
    lua.pushInteger(cdb.BFS_ALREADY_THERE); lua.setField(-2, "ALREADY_THERE");
    lua.pushInteger(cdb.BFS_TO_FAR);       lua.setField(-2, "TOO_FAR");
    lua.pushInteger(cdb.BFS_NO_PATH);      lua.setField(-2, "NO_PATH");
    lua.setField(-2, "bfs");

    lua.setField(-2, "consts");
}
