const std = @import("std");
const zlua = @import("zlua");
const cdb = @import("cdb");
const bitflags = @import("flags.zig");
const objects_lua = @import("object_lua.zig");
const rooms_lua = @import("room_lua.zig");
const zones_lua = @import("zone_lua.zig");
const lua_meta = @import("lua_meta.zig");
const character_api = @import("character_api.zig");
const lua_api = @import("lua_api.zig");
const modifiers_api = @import("modifiers_api.zig");
const intern_mod = @import("intern.zig");

const Lua = zlua.Lua;
const character_metatable = "dbat.Character";

extern fn roll_skill(ch: *cdb.char_data, snum: c_int) c_int;
extern fn find_skill_num(name: [*c]u8, sktype: c_int) c_int;
extern fn char_condition_count(ch: *cdb.char_data) usize;
extern fn char_condition_name_at(ch: *cdb.char_data, index: usize) ?[*:0]const u8;
extern fn event_schedule_lua_char_update(fire_at: i64, interval: i64, kind: ?[*:0]const u8, char_id: i64) u64;
extern fn eq_cancel_owner(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn eq_owner_count(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn eq_owner_next_ms(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn event_queue_now_ms() i64;
extern fn shop_keeper(ch: *cdb.char_data, me: ?*anyopaque, cmd: c_int, argument: [*c]u8) c_int;
extern fn limb_ok(ch: *cdb.char_data, type: c_int) c_int;
const mob_proto_metatable = "dbat.MobPrototype";
const condition_metatable = "dbat.Condition";
const char_script_metatable = "dbat.CharacterScript";

const CharacterHandle = extern struct {
    id: i64,
};

const MobProtoHandle = extern struct {
    vnum: cdb.mob_vnum,
};

const ConditionHandle = extern struct {
    character_id: i64,
    condition: [64:0]u8,
};

const CharScriptHandle = extern struct {
    character_id: i64,
    script: [64:0]u8,
};

const sex_neutral: c_int = 0;
const sex_male: c_int = 1;
const sex_female: c_int = 2;

pub fn register(lua: *Lua) void {
    registerCharacterMetatable(lua);
    registerMobProtoMetatable(lua);
    registerConditionMetatable(lua);
    registerCharScriptMetatable(lua);

    lua.newTable();
    lua.pushFunction(zlua.wrap(luaCharacterById));
    lua.setField(-2, "by_id");
    lua.pushFunction(zlua.wrap(luaCharactersAll));
    lua.setField(-2, "all");
    lua.pushFunction(zlua.wrap(luaCharactersBySubscription));
    lua.setField(-2, "by_subscription");
    lua.setField(-2, "characters");

    lua.newTable();
    lua.pushFunction(zlua.wrap(luaMobProtoById));
    lua.setField(-2, "by_id");
    lua.pushFunction(zlua.wrap(luaMobProtosAll));
    lua.setField(-2, "all");
    lua.setField(-2, "mob_protos");
}

fn luaCharacterById(lua: *Lua) i32 {
    const id = lua.toInteger(1) catch {
        lua.pushNil();
        return 1;
    };

    if (cdb.char_by_id(id) == null) {
        lua.pushNil();
        return 1;
    }

    pushCharacter(lua, id);
    return 1;
}

fn luaCharactersAll(lua: *Lua) i32 {
    lua.newTable();
    const iterator = cdb.char_iterator_create();
    defer cdb.char_iterator_free(iterator);

    var index: usize = 1;
    while (cdb.char_next(iterator)) |ch| {
        if (cdb.char_is_extracted(ch)) continue;
        pushCharacter(lua, cdb.char_id_get(ch));
        lua.setIndex(-2, @intCast(index));
        index += 1;
    }

    return valueIterator(lua);
}

fn luaCharactersBySubscription(lua: *Lua) i32 {
    const tag = lua.toString(1) catch {
        lua.newTable();
        return valueIterator(lua);
    };

    var count: usize = 0;
    const ids = cdb.char_subscribe_ids(tag.ptr, &count);
    defer cdb.char_subscribe_ids_free(ids);

    lua.newTable();
    var index: usize = 1;
    if (ids) |id_slice| {
        for (id_slice[0..count]) |id| {
            if (cdb.char_by_id(id) == null) continue;
            pushCharacter(lua, id);
            lua.setIndex(-2, @intCast(index));
            index += 1;
        }
    }

    return valueIterator(lua);
}

fn registerCharacterMetatable(lua: *Lua) void {
    lua.newMetatable(character_metatable) catch {
        lua.pop(1);
        return;
    };

    lua.pushValue(-1);
    lua.setField(-2, "__index");

    addMethod(lua, "__tostring", luaCharacterToString);
    addMethod(lua, "reftype", luaCharacterRefType);
    addMethod(lua, "is_npc", luaCharacterIsNpc);
    addMethod(lua, "valid", luaCharacterValid);
    addMethod(lua, "is_extracted", luaCharacterIsExtracted);
    addMethod(lua, "is_same", luaCharacterIsSame);
    addMethod(lua, "__eq", luaCharacterIsSame);
    addMethod(lua, "update", luaCharacterUpdate);
    addMethod(lua, "send", luaCharacterSend);
    addMethod(lua, "send_raw", luaCharacterSendText);
    addMethod(lua, "extract", luaCharacterExtract);
    addMethod(lua, "can_see_in_dark", luaCharacterCanSeeInDark);
    addMethod(lua, "can_see_char", luaCharacterCanSeeChar);
    addMethod(lua, "can_see_obj", luaCharacterCanSeeObj);
    addMethod(lua, "perform_get_from_room", luaCharacterPerformGetFromRoom);
    addMethod(lua, "id_get", luaCharacterIdGet);
    addMethod(lua, "proto_id_get", luaCharacterProtoIdGet);
    addMethod(lua, "proto_id_set", luaCharacterProtoIdSet);
    addMethod(lua, "vnum_get", luaCharacterVnumGet);
    addMethod(lua, "vnum_set", luaCharacterVnumSet);
    addMethod(lua, "room_vnum_get", luaCharacterRoomVnumGet);
    addMethod(lua, "room_vnum_set", luaCharacterRoomVnumSet);
    addMethod(lua, "room_get", luaCharacterRoomGet);
    addMethod(lua, "from_room", luaCharacterFromRoom);
    addMethod(lua, "to_room", luaCharacterToRoom);
    addMethod(lua, "unequip", luaCharacterUnequip);
    addMethod(lua, "zone_vnum_get", luaCharacterZoneVnumGet);
    addMethod(lua, "zone_get", luaCharacterZoneGet);
    addMethod(lua, "reveal_hiding", luaCharacterRevealHiding);
    addMethod(lua, "fly_zone", luaCharacterFlyZone);
    addMethod(lua, "is_planet_zenith", luaCharacterIsPlanetZenith);
    addMethod(lua, "sense_location", luaCharacterSenseLocation);
    addMethod(lua, "die", luaCharacterDie);
    addMethod(lua, "release_charge", luaCharacterReleaseCharge);
    addMethod(lua, "send_to_sense", luaCharacterSendToSense);
    addMethod(lua, "send_to_scouter", luaCharacterSendToScouter);
    addMethod(lua, "name_get", luaCharacterNameGet);
    addMethod(lua, "name_set", luaCharacterNameSet);
    addMethod(lua, "description_get", luaCharacterDescriptionGet);
    addMethod(lua, "description_set", luaCharacterDescriptionSet);
    addMethod(lua, "short_description_get", luaCharacterShortDescriptionGet);
    addMethod(lua, "short_description_set", luaCharacterShortDescriptionSet);
    addMethod(lua, "long_description_get", luaCharacterLongDescriptionGet);
    addMethod(lua, "long_description_set", luaCharacterLongDescriptionSet);
    addMethod(lua, "title_get", luaCharacterTitleGet);
    addMethod(lua, "title_set", luaCharacterTitleSet);
    addMethod(lua, "sensei_get", luaCharacterSenseiGet);
    addMethod(lua, "sensei_set", luaCharacterSenseiSet);
    addMethod(lua, "race_get", luaCharacterRaceGet);
    addMethod(lua, "race_set", luaCharacterRaceSet);
    addMethod(lua, "size_get", luaCharacterSizeGet);
    addMethod(lua, "size_set", luaCharacterSizeSet);
    addMethod(lua, "size_mod", luaCharacterSizeMod);
    addMethod(lua, "sex_get", luaCharacterSexGet);
    addMethod(lua, "sex_set", luaCharacterSexSet);
    addMethod(lua, "position_get", luaCharacterPositionGet);
    addMethod(lua, "position_set", luaCharacterPositionSet);
    addMethod(lua, "is_fighting", luaCharacterIsFighting);
    addMethod(lua, "admin_level_get", luaCharacterAdminLevelGet);
    addMethod(lua, "admin_level_set", luaCharacterAdminLevelSet);
    addMethod(lua, "admin_level_mod", luaCharacterAdminLevelMod);
    addMethod(lua, "admin_flagged", luaCharacterAdminFlagged);
    addMethod(lua, "admin_flag_set", luaCharacterAdminFlagSet);
    addMethod(lua, "admin_flag_toggle", luaCharacterAdminFlagToggle);
    addMethod(lua, "player_flagged", luaCharacterPlayerFlagged);
    addMethod(lua, "player_flag_set", luaCharacterPlayerFlagSet);
    addMethod(lua, "player_flag_toggle", luaCharacterPlayerFlagToggle);
    addMethod(lua, "pref_flagged", luaCharacterPrefFlagged);
    addMethod(lua, "pref_flag_set", luaCharacterPrefFlagSet);
    addMethod(lua, "pref_flag_toggle", luaCharacterPrefFlagToggle);
    addMethod(lua, "aff_flagged", luaCharacterAffFlagged);
    addMethod(lua, "body_flagged", luaCharacterBodyFlagged);
    addMethod(lua, "user_get", luaCharacterUserGet);
    addMethod(lua, "stat_get", luaCharacterStatGet);
    addMethod(lua, "stat_set", luaCharacterStatSet);
    addMethod(lua, "stat_mod", luaCharacterStatMod);
    addMethod(lua, "der_base", luaCharacterDerivedBase);
    addMethod(lua, "der_total", luaCharacterDerivedTotal);
    addMethod(lua, "der_invalidate", luaCharacterDerivedInvalidate);
    addMethod(lua, "modifiers_for", luaCharacterModifiersFor);
    addMethod(lua, "legacy_modifier", luaCharacterLegacyModifier);
    addMethod(lua, "modifier_gen", luaCharacterModifierGen);
    addMethod(lua, "meter_get", luaCharacterMeterGet);
    addMethod(lua, "meter_set", luaCharacterMeterSet);
    addMethod(lua, "meter_mod", luaCharacterMeterMod);
    addMethod(lua, "meter_set_int", luaCharacterMeterSetInt);
    addMethod(lua, "meter_mod_int", luaCharacterMeterModInt);
    addMethod(lua, "meter_current", luaCharacterMeterCurrent);
    addMethod(lua, "meter_max", luaCharacterMeterMax);
    addMethod(lua, "skill_base_get", luaCharacterSkillBaseGet);
    addMethod(lua, "skill_base_set", luaCharacterSkillBaseSet);
    addMethod(lua, "skill_base_mod", luaCharacterSkillBaseMod);
    addMethod(lua, "skill_modifier_get", luaCharacterSkillModifierGet);
    addMethod(lua, "skill_total_get", luaCharacterSkillTotalGet);
    addMethod(lua, "skill_get", luaCharacterSkillTotalGet);
    addMethod(lua, "init_skill", luaCharacterInitSkill);
    addMethod(lua, "roll_skill", luaCharacterRollSkill);
    addMethod(lua, "skill_perf_get", luaCharacterSkillPerfGet);
    addMethod(lua, "skill_perf_set", luaCharacterSkillPerfSet);
    addMethod(lua, "skill_perf_mod", luaCharacterSkillPerfMod);
    addMethod(lua, "condition_has", luaCharacterConditionHas);
    addMethod(lua, "condition_has_tag", luaCharacterConditionHasTag);
    addMethod(lua, "condition_active_with_tag", luaCharacterConditionActiveWithTag);
    addMethod(lua, "condition_add", luaCharacterConditionAdd);
    addMethod(lua, "condition_apply", luaCharacterConditionApply);
    addMethod(lua, "condition_apply_variables", luaCharacterConditionApplyVariables);
    addMethod(lua, "condition_apply_number", luaCharacterConditionApplyNumber);
    addMethod(lua, "condition_apply_with_duration", luaCharacterConditionApplyWithDuration);
    addMethod(lua, "condition_remove", luaCharacterConditionRemove);
    addMethod(lua, "condition_remove_tag", luaCharacterConditionRemoveTag);
    addMethod(lua, "condition", luaCharacterCondition);
    addMethod(lua, "condition_number_get", luaCharacterConditionNumberGet);
    addMethod(lua, "condition_number_set", luaCharacterConditionNumberSet);
    addMethod(lua, "condition_number_mod", luaCharacterConditionNumberMod);
    addMethod(lua, "condition_string_get", luaCharacterConditionStringGet);
    addMethod(lua, "condition_string_set", luaCharacterConditionStringSet);
    addMethod(lua, "transform_has", luaCharacterTransformHas);
    addMethod(lua, "transform_add", luaCharacterTransformAdd);
    addMethod(lua, "transform_remove", luaCharacterTransformRemove);
    addMethod(lua, "transform_unlocked", luaCharacterTransformUnlocked);
    addMethod(lua, "transform_unlock", luaCharacterTransformUnlock);
    addMethod(lua, "transform_number_get", luaCharacterTransformNumberGet);
    addMethod(lua, "transform_number_set", luaCharacterTransformNumberSet);
    addMethod(lua, "transform_number_mod", luaCharacterTransformNumberMod);
    addMethod(lua, "transform_string_get", luaCharacterTransformStringGet);
    addMethod(lua, "transform_string_set", luaCharacterTransformStringSet);
    addMethod(lua, "inventory_count", luaCharacterInventoryCount);
    addMethod(lua, "equipment_count", luaCharacterEquipmentCount);
    addMethod(lua, "inventory_get", luaCharacterInventoryGet);
    addMethod(lua, "equipment_get", luaCharacterEquipmentGet);
    addMethod(lua, "inventory", luaCharacterInventoryGet);
    addMethod(lua, "equipment", luaCharacterEquipmentGet);
    addMethod(lua, "is_outside", luaCharacterIsOutside);
    addMethod(lua, "sits_get", luaCharacterSitsGet);
    addMethod(lua, "sits_set", luaCharacterSitsSet);
    addMethod(lua, "conditions", luaCharacterConditions);
    addMethod(lua, "command_queue_clear", luaCharacterCommandQueueClear);
    addMethod(lua, "command_enqueue", luaCharacterCommandEnqueue);
    addMethod(lua, "event_schedule", luaCharacterEventSchedule);
    addMethod(lua, "event_cancel", luaCharacterEventCancel);
    addMethod(lua, "event_count", luaCharacterEventCount);
    addMethod(lua, "event_remaining_ms", luaCharacterEventRemainingMs);
    addMethod(lua, "time_played", luaCharacterTimePlayed);
    addMethod(lua, "age_years", luaCharacterAgeYears);
    addMethod(lua, "clan_get", luaCharacterClanGet);
    addMethod(lua, "rp_get", luaCharacterRpGet);
    addMethod(lua, "rp_set", luaCharacterRpSet);
    addMethod(lua, "rp_save", luaCharacterRpSave);
    addMethod(lua, "radar1_get", luaCharacterRadar1Get);
    addMethod(lua, "radar1_set", luaCharacterRadar1Set);
    addMethod(lua, "radar2_get", luaCharacterRadar2Get);
    addMethod(lua, "radar2_set", luaCharacterRadar2Set);
    addMethod(lua, "radar3_get", luaCharacterRadar3Get);
    addMethod(lua, "radar3_set", luaCharacterRadar3Set);
    addMethod(lua, "has_arms", luaCharacterHasArms);
    addMethod(lua, "player_id_get", luaCharacterPlayerIdGet);
    addMethod(lua, "height_cm", luaCharacterHeightCm);
    addMethod(lua, "weight_kg", luaCharacterWeightKg);
    addMethod(lua, "align_str", luaCharacterAlignStr);
    addMethod(lua, "level_exp", luaCharacterLevelExp);
    addMethod(lua, "rpp_to_level", luaCharacterRppToLevel);
    addMethod(lua, "molt_threshold", luaCharacterMoltThreshold);
    addMethod(lua, "limbcond_get", luaCharacterLimbCondGet);
    addMethod(lua, "limbcond_set", luaCharacterLimbCondSet);
    addMethod(lua, "limb_ok", luaCharacterLimbOk);
    addMethod(lua, "gain_tail", luaCharacterGainTail);
    addMethod(lua, "has_tail", luaCharacterHasTail);
    addMethod(lua, "lose_tail", luaCharacterLoseTail);
    addMethod(lua, "remove_limb", luaCharacterRemoveLimb);
    addMethod(lua, "wielded_weapon_type", luaCharacterWieldedWeaponType);
    addMethod(lua, "charge_get", luaCharacterChargeGet);
    addMethod(lua, "charge_set", luaCharacterChargeSet);
    addMethod(lua, "barrier_get", luaCharacterBarrierGet);
    addMethod(lua, "voice_get", luaCharacterVoiceGet);
    addMethod(lua, "distfea_get", luaCharacterDistfeaGet);
    addMethod(lua, "rdisplay_get", luaCharacterRdisplayGet);
    addMethod(lua, "feature_get", luaCharacterFeatureGet);
    addMethod(lua, "feature_set", luaCharacterFeatureSet);
    addMethod(lua, "bring_to_cap", luaCharacterBringToCap);
    addMethod(lua, "rpp_custom_equip_launch", luaCharacterRppCustomEquipLaunch);
    addMethod(lua, "rpp_restring_launch", luaCharacterRppRestringLaunch);
    addMethod(lua, "absorbs_get", luaCharacterAbsorbsGet);
    addMethod(lua, "absorbs_set", luaCharacterAbsorbsSet);
    addMethod(lua, "absorbs_mod", luaCharacterAbsorbsMod);
    addMethod(lua, "handle_ingest_learn", luaCharacterHandleIngestLearn);
    addMethod(lua, "mimic_get", luaCharacterMimicGet);
    addMethod(lua, "mimic_set", luaCharacterMimicSet);
    addMethod(lua, "backstab_cooldown", luaCharacterBackstabCooldown);
    addMethod(lua, "preference_get", luaCharacterPreferenceGet);
    addMethod(lua, "preference_set", luaCharacterPreferenceSet);
    addMethod(lua, "genome_get", luaCharacterGenomeGet);
    addMethod(lua, "wait_set", luaCharacterWaitSet);
    addMethod(lua, "cooldown_get", luaCharacterCooldownGet);
    addMethod(lua, "cooldown_set", luaCharacterCooldownSet);
    addMethod(lua, "selfdestruct_cooldown_get", luaCharacterSelfdestructCooldownGet);
    addMethod(lua, "selfdestruct_cooldown_set", luaCharacterSelfdestructCooldownSet);
    addMethod(lua, "inventory_find_vnum", luaCharacterInventoryFindVnum);
    addMethod(lua, "know_skill", luaCharacterKnowSkill);
    addMethod(lua, "improve_skill", luaCharacterImproveSkill);
    addMethod(lua, "defending_for_get", luaCharacterDefendingForGet);
    addMethod(lua, "defending_for_set", luaCharacterDefendingForSet);
    addMethod(lua, "defended_by_get", luaCharacterDefendedByGet);
    addMethod(lua, "defended_by_set", luaCharacterDefendedBySet);
    addMethod(lua, "aura_get", luaCharacterAuraGet);
    addMethod(lua, "aura_set", luaCharacterAuraSet);
    addMethod(lua, "hairl_get", luaCharacterHairlGet);
    addMethod(lua, "hairl_set", luaCharacterHairlSet);
    addMethod(lua, "hairs_get", luaCharacterHairsGet);
    addMethod(lua, "hairs_set", luaCharacterHairsSet);
    addMethod(lua, "hairc_get", luaCharacterHaircGet);
    addMethod(lua, "hairc_set", luaCharacterHaircSet);
    addMethod(lua, "skin_get", luaCharacterSkinGet);
    addMethod(lua, "skin_set", luaCharacterSkinSet);
    addMethod(lua, "eye_get", luaCharacterEyeGet);
    addMethod(lua, "eye_set", luaCharacterEyeSet);
    addMethod(lua, "distfea_set", luaCharacterDistfeaSet);
    addMethod(lua, "sleeptime_get", luaCharacterSleepcountGet);
    addMethod(lua, "has_group", luaCharacterHasGroup);
    addMethod(lua, "has_mail", luaCharacterHasMail);
    addMethod(lua, "starphase_get", luaCharacterStarphaseGet);
    addMethod(lua, "soft_cap", luaCharacterSoftCap);
    addMethod(lua, "is_soft_capped", luaCharacterIsSoftCapped);
    addMethod(lua, "gain_exp", luaCharacterGainExp);
    addMethod(lua, "gain_condition", luaCharacterGainCondition);
    addMethod(lua, "mob_flagged", luaCharacterMobFlagged);
    addMethod(lua, "mob_flag_set", luaCharacterMobFlagSet);
    addMethod(lua, "is_shopkeeper", luaCharacterIsShopkeeper);
    addMethod(lua, "is_soft_cap", luaCharacterIsSoftCapType);
    addMethod(lua, "following_get", luaCharacterFollowingGet);
    addMethod(lua, "group_bonus", luaCharacterGroupBonus);
    addMethod(lua, "start_fighting", luaCharacterStartFighting);
    addMethod(lua, "stop_fighting", luaCharacterStopFighting);
    addMethod(lua, "news_pending", luaCharacterNewsPending);
    addMethod(lua, "intro_known", luaCharacterIntroKnown);
    addMethod(lua, "get_intro_name", luaCharacterGetIntroName);
    addMethod(lua, "introd_calc", luaCharacterIntrodCalc);
    addMethod(lua, "bonus_flagged", luaCharacterBonusFlagged);
    addMethod(lua, "aff_flag_set", luaCharacterAffFlagSet);
    addMethod(lua, "barrier_set", luaCharacterBarrierSet);
    addMethod(lua, "carry_drop", luaCharacterCarryDrop);
    addMethod(lua, "land", luaCharacterLand);
    addMethod(lua, "arena_idnum_get", luaCharacterArenaIdnumGet);
    addMethod(lua, "arena_idnum_set", luaCharacterArenaIdnumSet);
    addMethod(lua, "droom_get", luaCharacterDroomGet);
    addMethod(lua, "droom_set", luaCharacterDroomSet);
    addMethod(lua, "dragging_get", luaCharacterDraggingGet);
    addMethod(lua, "dragging_set", luaCharacterDraggingSet);
    addMethod(lua, "being_dragged_get", luaCharacterBeingDraggedGet);
    addMethod(lua, "being_dragged_set", luaCharacterBeingDraggedSet);
    addMethod(lua, "carrying_char_get", luaCharacterCarryingCharGet);
    addMethod(lua, "carrying_char_set", luaCharacterCarryingCharSet);
    addMethod(lua, "carried_by_char_get", luaCharacterCarriedByCharGet);
    addMethod(lua, "carried_by_char_set", luaCharacterCarriedByCharSet);
    addMethod(lua, "poofin_get", luaCharacterPoofInGet);
    addMethod(lua, "poofout_get", luaCharacterPoofOutGet);
    addMethod(lua, "loadroom_get", luaCharacterLoadRoomGet);
    addMethod(lua, "loadroom_set", luaCharacterLoadRoomSet);
    addMethod(lua, "look_at_room", luaCharacterLookAtRoom);
    addMethod(lua, "look_at_specific_room", luaCharacterLookAtSpecificRoom);
    addMethod(lua, "restore", luaCharacterRestore);
    addMethod(lua, "find_target_room", luaCharacterFindTargetRoom);
    addMethod(lua, "flee", luaCharacterFlee);
    addMethod(lua, "followers_each", luaCharacterFollowersEach);
    addMethod(lua, "add_follower", luaCharacterAddFollower);
    addMethod(lua, "stop_follower", luaCharacterStopFollower);
    addMethod(lua, "circle_follow", luaCharacterCircleFollow);
    addMethod(lua, "clones", luaCharacterClones);
    addMethod(lua, "clone_count", luaCharacterCloneCount);
    addMethod(lua, "clone_add", luaCharacterCloneAdd);
    addMethod(lua, "conditions_active", luaCharacterConditionsActive);
    addMethod(lua, "condition_number_vars", luaCharacterConditionNumberVars);
    addMethod(lua, "condition_string_vars", luaCharacterConditionStringVars);

    // Combat-pointer getters for room display
    addMethod(lua, "fighting_get", luaCharacterFightingGet);
    addMethod(lua, "grappling_get", luaCharacterGrapplingGet);
    addMethod(lua, "grappling_set", luaCharacterGrapplingSet);
    addMethod(lua, "grappled_get", luaCharacterGrappledGet);
    addMethod(lua, "grappled_set", luaCharacterGrappledSet);
    addMethod(lua, "graptype_get", luaCharacterGraptypeGet);
    addMethod(lua, "absorbing_get", luaCharacterAbsorbingGet);
    addMethod(lua, "absorbing_set", luaCharacterAbsorbingSet);
    addMethod(lua, "absorbed_by_get", luaCharacterAbsorbedByGet);
    addMethod(lua, "absorbed_by_set", luaCharacterAbsorbedBySet);
    addMethod(lua, "mindlinked_get", luaCharacterMindlinkedGet);
    addMethod(lua, "mindlinked_set", luaCharacterMindlinkedSet);
    addMethod(lua, "linker_get", luaCharacterLinkerGet);
    addMethod(lua, "linker_set", luaCharacterLinkerSet);
    addMethod(lua, "blocking_get", luaCharacterBlockingGet);
    addMethod(lua, "blocking_set", luaCharacterBlockingSet);
    addMethod(lua, "blocked_by_get", luaCharacterBlockedByGet);
    addMethod(lua, "blocked_by_set", luaCharacterBlockedBySet);
    addMethod(lua, "can_kill", luaCharacterCanKill);
    addMethod(lua, "lastatk_get", luaCharacterLastAtkGet);
    addMethod(lua, "lastatk_set", luaCharacterLastAtkSet);
    addMethod(lua, "carry_weight_get", luaCharacterCarryWeightGet);
    addMethod(lua, "carry_weight_max", luaCharacterCarryWeightMax);
    addMethod(lua, "wimp_level_get", luaCharacterWimpLevelGet);
    addMethod(lua, "wimp_level_set", luaCharacterWimpLevelSet);
    addMethod(lua, "send_to_worlds", luaCharacterSendToWorlds);
    addMethod(lua, "dispel_ash", luaCharacterDispelAsh);
    addMethod(lua, "restore_announced", luaCharacterRestoreAnnounced);
    addMethod(lua, "cure_knocked_out", luaCharacterCureKnockedOut);
    // Misc display fields
    addMethod(lua, "timer_get", luaCharacterTimerGet);
    addMethod(lua, "has_connection", luaCharacterHasConnection);
    addMethod(lua, "default_position_get", luaCharacterDefaultPositionGet);
    addMethod(lua, "eavesdrop_get", luaCharacterEavesdropGet);
    addMethod(lua, "eavesdrop_set", luaCharacterEavesdropSet);
    addMethod(lua, "eavesdrop_dir_get", luaCharacterEavesdropDirGet);
    addMethod(lua, "eavesdrop_dir_set", luaCharacterEavesdropDirSet);
    addMethod(lua, "rdisplay_clear", luaCharacterRdisplayClear);
    addMethod(lua, "slot_count", luaCharacterSlotCount);
    addMethod(lua, "check_special", luaCharacterCheckSpecial);
    addMethod(lua, "script_add", luaCharacterScriptAdd);
    addMethod(lua, "script_remove", luaCharacterScriptRemove);
    addMethod(lua, "script_has", luaCharacterScriptHas);
    addMethod(lua, "script", luaCharacterScript);
    addMethod(lua, "scripts", luaCharacterScripts);
    addMethod(lua, "script_number_get", luaCharacterScriptNumberGet);
    addMethod(lua, "script_number_set", luaCharacterScriptNumberSet);
    addMethod(lua, "script_text_get", luaCharacterScriptTextGet);
    addMethod(lua, "script_text_set", luaCharacterScriptTextSet);

    lua_meta.mergeMethods(lua, "lua.characters.character");

    lua.pop(1);
}

fn registerMobProtoMetatable(lua: *Lua) void {
    lua.newMetatable(mob_proto_metatable) catch {
        lua.pop(1);
        return;
    };

    lua.pushValue(-1);
    lua.setField(-2, "__index");

    addMethod(lua, "__tostring", luaMobProtoToString);
    addMethod(lua, "reftype", luaMobProtoRefType);
    addMethod(lua, "valid", luaMobProtoValid);
    addMethod(lua, "vnum_get", luaMobProtoVnumGet);
    addMethod(lua, "name_get", luaMobProtoNameGet);
    addMethod(lua, "short_description_get", luaMobProtoShortDescrGet);
    addMethod(lua, "has_trig", luaMobProtoHasTrig);
    addMethod(lua, "spawn", luaMobProtoSpawn);

    lua.pop(1);
}

fn registerConditionMetatable(lua: *Lua) void {
    lua.newMetatable(condition_metatable) catch {
        lua.pop(1);
        return;
    };
    lua.pushValue(-1);
    lua.setField(-2, "__index");
    addMethod(lua, "id", luaConditionId);
    addMethod(lua, "stacks", luaConditionStacks);
    addMethod(lua, "stacks_set", luaConditionStacksSet);
    addMethod(lua, "duration", luaConditionDuration);
    addMethod(lua, "duration_set", luaConditionDurationSet);
    addMethod(lua, "number_get", luaConditionNumberGet);
    addMethod(lua, "number_set", luaConditionNumberSet);
    addMethod(lua, "number_mod", luaConditionNumberMod);
    addMethod(lua, "string_get", luaConditionStringGet);
    addMethod(lua, "string_set", luaConditionStringSet);
    addMethod(lua, "schedule_event", luaConditionScheduleEvent);
    addMethod(lua, "cancel_event", luaConditionCancelEvent);
    addMethod(lua, "event_pending", luaConditionEventPending);
    addMethod(lua, "event_next_ms", luaConditionEventNextMs);
    addMethod(lua, "schedule_expire", luaConditionScheduleExpire);
    addMethod(lua, "remaining_ms", luaConditionRemainingMs);
    addMethod(lua, "remaining_secs", luaConditionRemainingSecs);
    lua.pop(1);
}

fn registerCharScriptMetatable(lua: *Lua) void {
    lua.newMetatable(char_script_metatable) catch {
        lua.pop(1);
        return;
    };
    lua.pushValue(-1);
    lua.setField(-2, "__index");
    addMethod(lua, "id", luaCharScriptId);
    addMethod(lua, "number_get", luaCharScriptNumberGet);
    addMethod(lua, "number_set", luaCharScriptNumberSet);
    addMethod(lua, "number_mod", luaCharScriptNumberMod);
    addMethod(lua, "text_get", luaCharScriptTextGet);
    addMethod(lua, "text_set", luaCharScriptTextSet);
    addMethod(lua, "schedule_event", luaCharScriptScheduleEvent);
    addMethod(lua, "cancel_event", luaCharScriptCancelEvent);
    addMethod(lua, "event_pending", luaCharScriptEventPending);
    addMethod(lua, "event_next_ms", luaCharScriptEventNextMs);
    lua.pop(1);
}

fn addMethod(lua: *Lua, comptime name: [:0]const u8, comptime function: anytype) void {
    lua.pushFunction(zlua.wrap(function));
    lua.setField(-2, name);
}

pub fn pushCharacter(lua: *Lua, id: i64) void {
    const handle = lua.newUserdata(CharacterHandle, 0);
    handle.* = .{ .id = id };
    _ = lua.getMetatableRegistry(character_metatable);
    lua.setMetatable(-2);
}

pub fn pushMobProto(lua: *Lua, vnum: cdb.mob_vnum) void {
    const handle = lua.newUserdata(MobProtoHandle, 0);
    handle.* = .{ .vnum = vnum };
    _ = lua.getMetatableRegistry(mob_proto_metatable);
    lua.setMetatable(-2);
}

pub fn pushCondition(lua: *Lua, character_id: i64, condition: []const u8) void {
    const handle = lua.newUserdata(ConditionHandle, 0);
    handle.character_id = character_id;
    handle.condition = std.mem.zeroes([64:0]u8);
    const len = @min(condition.len, handle.condition.len - 1);
    @memcpy(handle.condition[0..len], condition[0..len]);
    _ = lua.getMetatableRegistry(condition_metatable);
    lua.setMetatable(-2);
}

fn checkCharacterHandle(lua: *Lua) *CharacterHandle {
    return lua.testUserdata(CharacterHandle, 1, character_metatable) catch {
        lua.raiseErrorStr("expected dbat.Character", .{});
    };
}

pub fn checkCharacter(lua: *Lua) *cdb.char_data {
    return checkCharacterAt(lua, 1);
}

pub fn checkCharacterAt(lua: *Lua, index: i32) *cdb.char_data {
    const handle = lua.testUserdata(CharacterHandle, index, character_metatable) catch {
        lua.raiseErrorStr("expected dbat.Character", .{});
    };
    return characterByHandleId(handle.id) orelse {
        lua.raiseErrorStr("stale dbat.Character handle for character %d", .{handle.id});
    };
}

fn characterByHandleId(id: i64) ?*cdb.char_data {
    return cdb.char_by_id(id);
}

fn checkMobProtoHandle(lua: *Lua) *MobProtoHandle {
    return lua.testUserdata(MobProtoHandle, 1, mob_proto_metatable) catch {
        lua.raiseErrorStr("expected dbat.MobPrototype", .{});
    };
}

fn checkMobProto(lua: *Lua) *cdb.mob_proto_data {
    const handle = checkMobProtoHandle(lua);
    return cdb.mob_proto_by_id(handle.vnum) orelse {
        lua.raiseErrorStr("stale dbat.MobPrototype handle for mobile prototype %d", .{handle.vnum});
    };
}

fn checkConditionHandle(lua: *Lua) *ConditionHandle {
    return lua.testUserdata(ConditionHandle, 1, condition_metatable) catch {
        lua.raiseErrorStr("expected dbat.Condition", .{});
    };
}

fn conditionCharacter(lua: *Lua, handle: *ConditionHandle) *cdb.char_data {
    return characterByHandleId(handle.character_id) orelse lua.raiseErrorStr("stale dbat.Condition character", .{});
}

fn conditionName(handle: *ConditionHandle) [*:0]const u8 {
    return @ptrCast(&handle.condition);
}

fn integer(lua: *Lua, index: i32) zlua.Integer {
    return lua.toInteger(index) catch lua.typeError(index, "integer");
}

fn boolean(lua: *Lua, index: i32) bool {
    if (!lua.isBoolean(index)) lua.typeError(index, "boolean");
    return lua.toBoolean(index);
}

fn string(lua: *Lua, index: i32) [:0]const u8 {
    return lua.toString(index) catch lua.typeError(index, "string");
}

fn pushCString(lua: *Lua, value: [*c]const u8) void {
    if (value == null) {
        lua.pushNil();
        return;
    }
    _ = lua.pushString(std.mem.span(value));
}

fn intCastOrError(lua: *Lua, comptime T: type, value: zlua.Integer, label: [:0]const u8) T {
    return std.math.cast(T, value) orelse lua.raiseErrorStr("%s out of range", .{label.ptr});
}

fn legacyForDefinitionId(lua: *Lua, comptime category: [:0]const u8, id: [:0]const u8) ?c_int {
    const top = lua.getTop();
    defer lua.setTop(top);

    if (lua.getGlobal("dbat") != .table) return null;
    if (lua.getField(-1, "characters") != .table) return null;
    if (lua.getField(-1, "registry") != .table) return null;
    if (lua.getField(-1, category) != .table) return null;
    if (lua.getField(-1, id) != .table) return null;
    if (lua.getField(-1, "legacy_id") != .number) return null;
    const legacy_id = lua.toInteger(-1) catch return null;
    return std.math.cast(c_int, legacy_id) orelse null;
}

fn pushDefinitionIdForLegacy(lua: *Lua, comptime category: [:0]const u8, legacy_id: c_int) bool {
    const top = lua.getTop();
    var id_buf: [128]u8 = undefined;
    var id_len: usize = 0;
    var found = false;

    if (lua.getGlobal("dbat") == .table and
        lua.getField(-1, "characters") == .table and
        lua.getField(-1, "registry") == .table and
        lua.getField(-1, category) == .table)
    {
        lua.pushNil();
        while (lua.next(-2)) {
            defer lua.pop(1);
            if (!lua.isTable(-1)) continue;
            if (lua.getField(-1, "legacy_id") != .number) {
                lua.pop(1);
                continue;
            }
            const value = lua.toInteger(-1) catch blk: {
                lua.pop(1);
                break :blk null;
            };
            lua.pop(1);
            if (value == null or value.? != legacy_id) continue;
            if (lua.getField(-1, "id") != .string) {
                lua.pop(1);
                continue;
            }
            const id = lua.toString(-1) catch "";
            lua.pop(1);
            id_len = @min(id.len, id_buf.len);
            @memcpy(id_buf[0..id_len], id[0..id_len]);
            found = true;
            break;
        }
    }

    lua.setTop(top);
    if (!found) return false;
    _ = lua.pushString(id_buf[0..id_len]);
    return true;
}

fn luaCharacterIsNpc(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    lua.pushBoolean(cdb.char_is_npc(ch));
    return 1;
}

fn luaCharacterValid(lua: *Lua) i32 {
    const handle = checkCharacterHandle(lua);
    lua.pushBoolean(characterByHandleId(handle.id) != null);
    return 1;
}

fn luaCharacterIsExtracted(lua: *Lua) i32 {
    const handle = checkCharacterHandle(lua);
    const ch = characterByHandleId(handle.id);
    lua.pushBoolean(ch == null or cdb.char_is_extracted(ch.?));
    return 1;
}

fn luaCharacterIsSame(lua: *Lua) i32 {
    const left = checkCharacterHandle(lua);
    const right = lua.testUserdata(CharacterHandle, 2, character_metatable) catch {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(left.id == right.id);
    return 1;
}

fn luaCharacterUpdate(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const kind = if (lua.isNoneOrNil(2)) "manual" else string(lua, 2);
    const seconds: i64 = if (lua.isNoneOrNil(3)) 0 else intCastOrError(lua, i64, integer(lua, 3), "update seconds");
    // Route "condition:<id>:<event>" kinds to targeted on_event dispatch rather than broadcast on_update.
    if (std.mem.startsWith(u8, kind, "condition:")) {
        const rest = kind["condition:".len..];
        if (std.mem.indexOfScalar(u8, rest, ':')) |sep| {
            const cond_id = rest[0..sep];
            const event_name = rest[sep + 1 ..];
            var cond_buf: [128:0]u8 = undefined;
            var ev_buf: [128:0]u8 = undefined;
            if (cond_id.len < cond_buf.len and event_name.len < ev_buf.len) {
                @memcpy(cond_buf[0..cond_id.len], cond_id);
                cond_buf[cond_id.len] = 0;
                @memcpy(ev_buf[0..event_name.len], event_name);
                ev_buf[event_name.len] = 0;
                cdb.char_condition_event_dispatch(ch, &cond_buf, &ev_buf);
                return 0;
            }
        }
    }
    cdb.char_condition_update_with_context(ch, kind, 0, seconds);
    return 0;
}

fn luaCharacterToString(lua: *Lua) i32 {
    const handle = checkCharacterHandle(lua);
    if (characterByHandleId(handle.id)) |ch| {
        _ = lua.pushFString("dbat.Character(%d, %s)", .{ handle.id, cdb.char_name_get(ch) });
    } else {
        _ = lua.pushFString("dbat.Character(%d, stale)", .{handle.id});
    }
    return 1;
}

fn luaCharacterRefType(lua: *Lua) i32 {
    _ = checkCharacter(lua);
    _ = lua.pushString("character");
    return 1;
}

fn luaCharacterSend(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (ch.desc != null) cdb.desc_send_text(ch.desc, string(lua, 2));
    return 0;
}

fn luaCharacterSendText(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const text = string(lua, 2);
    _ = cdb.send_to_char(ch, "%s", text.ptr);
    return 0;
}

fn luaCharacterExtract(lua: *Lua) i32 {
    cdb.extract_char(checkCharacter(lua));
    return 0;
}

fn luaCharacterCanSeeInDark(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_can_see_in_dark(checkCharacter(lua)));
    return 1;
}

fn luaCharacterCanSeeChar(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_can_see_char(checkCharacter(lua), checkCharacterAt(lua, 2)));
    return 1;
}

fn luaCharacterCanSeeObj(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_can_see_obj(checkCharacter(lua), objects_lua.checkObjectAt(lua, 2)));
    return 1;
}

fn luaCharacterPerformGetFromRoom(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const obj = objects_lua.checkObjectAt(lua, 2);
    lua.pushBoolean(cdb.perform_get_from_room(ch, obj) != 0);
    return 1;
}

fn luaCharacterIsOutside(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_is_outside(checkCharacter(lua)));
    return 1;
}

fn luaCharacterSitsGet(lua: *Lua) i32 {
    const obj = cdb.char_sits_get(checkCharacter(lua));
    if (obj == null) {
        lua.pushNil();
        return 1;
    }
    objects_lua.pushObject(lua, cdb.obj_id_get(obj));
    return 1;
}

fn luaCharacterSitsSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (lua.isNoneOrNil(2)) {
        cdb.char_sits_set(ch, null);
    } else {
        cdb.char_sits_set(ch, objects_lua.checkObjectAt(lua, 2));
    }
    return 0;
}

fn luaCharacterConditions(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const count = char_condition_count(ch);
    lua.newTable();
    var i: usize = 0;
    while (i < count) : (i += 1) {
        const name = char_condition_name_at(ch, i) orelse {
            i += 1;
            continue;
        };
        _ = lua.pushString(std.mem.span(name));
        lua.setIndex(-2, @intCast(i + 1));
    }
    return 1;
}

fn luaCharacterIdGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_id_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterProtoIdGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_proto_id_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterProtoIdSet(lua: *Lua) i32 {
    cdb.char_proto_id_set(checkCharacter(lua), intCastOrError(lua, cdb.mob_vnum, integer(lua, 2), "mobile proto id"));
    return 0;
}

fn luaCharacterVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_vnum_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterVnumSet(lua: *Lua) i32 {
    cdb.char_vnum_set(checkCharacter(lua), intCastOrError(lua, cdb.mob_vnum, integer(lua, 2), "mobile vnum"));
    return 0;
}

fn luaCharacterRoomVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_room_vnum_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterRoomVnumSet(lua: *Lua) i32 {
    cdb.char_room_vnum_set(checkCharacter(lua), intCastOrError(lua, cdb.room_vnum, integer(lua, 2), "room vnum"));
    return 0;
}

fn luaCharacterRoomGet(lua: *Lua) i32 {
    const room = cdb.char_room_get(checkCharacter(lua));
    if (room == null) {
        lua.pushNil();
        return 1;
    }
    rooms_lua.pushRoom(lua, room.*.id);
    return 1;
}

fn luaCharacterFromRoom(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (cdb.char_room_get(ch) != null) cdb.char_from_room(ch);
    return 0;
}

fn luaCharacterToRoom(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const room = rooms_lua.checkRoomAt(lua, 2);
    if (cdb.char_room_get(ch) != null) cdb.char_from_room(ch);
    cdb.char_to_room(ch, room);
    return 0;
}

fn luaCharacterUnequip(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const pos = intCastOrError(lua, c_int, integer(lua, 2), "equipment position");
    const obj = cdb.unequip_char(ch, pos);
    if (obj == null) {
        lua.pushNil();
        return 1;
    }
    objects_lua.pushObject(lua, cdb.obj_id_get(obj));
    return 1;
}

fn luaMobProtoById(lua: *Lua) i32 {
    const vnum = lua.toInteger(1) catch {
        lua.pushNil();
        return 1;
    };
    const mob_vnum = std.math.cast(cdb.mob_vnum, vnum) orelse {
        lua.pushNil();
        return 1;
    };
    if (cdb.mob_proto_by_id(mob_vnum) == null) {
        lua.pushNil();
        return 1;
    }
    pushMobProto(lua, mob_vnum);
    return 1;
}

fn luaMobProtoToString(lua: *Lua) i32 {
    const handle = checkMobProtoHandle(lua);
    _ = checkMobProto(lua);
    _ = lua.pushFString("dbat.MobPrototype(%d)", .{handle.vnum});
    return 1;
}

fn luaMobProtoRefType(lua: *Lua) i32 {
    _ = checkMobProto(lua);
    _ = lua.pushString("mob_prototype");
    return 1;
}

fn luaMobProtoValid(lua: *Lua) i32 {
    const handle = checkMobProtoHandle(lua);
    lua.pushBoolean(cdb.mob_proto_by_id(handle.vnum) != null);
    return 1;
}

fn luaMobProtoVnumGet(lua: *Lua) i32 {
    lua.pushInteger(checkMobProtoHandle(lua).vnum);
    return 1;
}

fn luaMobProtoSpawn(lua: *Lua) i32 {
    const handle = checkMobProtoHandle(lua);
    _ = checkMobProto(lua);
    const mob = cdb.mob_spawn(handle.vnum);
    if (mob == null) {
        lua.pushNil();
        return 1;
    }

    if (!lua.isNoneOrNil(2)) {
        const room = rooms_lua.checkRoomAt(lua, 2);
        cdb.char_to_room(mob, room);
    }

    pushCharacter(lua, cdb.char_id_get(mob));
    return 1;
}

fn luaCharacterZoneVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_zone_vnum_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterZoneGet(lua: *Lua) i32 {
    const zone = cdb.char_zone_get(checkCharacter(lua));
    if (zone == null) {
        lua.pushNil();
        return 1;
    }
    zones_lua.pushZone(lua, cdb.zone_id_get(zone));
    return 1;
}

fn luaCharacterFlyZone(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const msg = lua.toString(2) catch lua.typeError(2, "string");
    const zone = cdb.char_zone_get(ch) orelse return 0;
    cdb.fly_zone(zone, @constCast(msg.ptr), ch);
    return 0;
}

fn luaCharacterIsPlanetZenith(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_planet_zenith(checkCharacter(lua)));
    return 1;
}

fn luaCharacterSenseLocation(lua: *Lua) i32 {
    const loc = cdb.sense_location(checkCharacter(lua));
    if (loc) |s| {
        _ = lua.pushString(std.mem.span(s));
    } else {
        lua.pushNil();
    }
    return 1;
}

fn luaCharacterRevealHiding(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const reveal_type = if (lua.getTop() >= 2) intCastOrError(lua, c_int, integer(lua, 2), "reveal type") else 0;
    cdb.reveal_hiding(ch, reveal_type);
    return 0;
}

fn luaCharacterDie(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const killer_id: i64 = if (lua.isNoneOrNil(2))
        0
    else if (lua.typeOf(2) == .userdata)
        checkCharacterAt(lua, 2).id
    else
        integer(lua, 2);
    cdb.char_die(ch, killer_id);
    return 0;
}

fn luaCharacterReleaseCharge(lua: *Lua) i32 {
    lua.pushBoolean(cdb.release_charge(checkCharacter(lua)));
    return 1;
}

fn luaCharacterSendToSense(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const sense_type = intCastOrError(lua, c_int, integer(lua, 2), "sense type");
    const text = string(lua, 3);
    cdb.send_to_sense(sense_type, @constCast(text.ptr), ch);
    return 0;
}

fn luaCharacterSendToScouter(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const text = string(lua, 2);
    const num = if (lua.getTop() >= 3) intCastOrError(lua, c_int, integer(lua, 3), "scouter num") else 1;
    const scouter_type = if (lua.getTop() >= 4) intCastOrError(lua, c_int, integer(lua, 4), "scouter type") else 0;
    cdb.send_to_scouter(@constCast(text.ptr), ch, num, scouter_type);
    return 0;
}

fn luaCharacterNameGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_name_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterNameSet(lua: *Lua) i32 {
    cdb.char_name_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterDescriptionGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_description_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterDescriptionSet(lua: *Lua) i32 {
    cdb.char_description_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterShortDescriptionGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_short_description_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterShortDescriptionSet(lua: *Lua) i32 {
    cdb.char_short_description_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterLongDescriptionGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_long_description_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterLongDescriptionSet(lua: *Lua) i32 {
    cdb.char_long_description_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterTitleGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_title_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterTitleSet(lua: *Lua) i32 {
    cdb.char_title_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterSenseiGet(lua: *Lua) i32 {
    const legacy_id = cdb.char_class_get(checkCharacter(lua));
    if (!pushDefinitionIdForLegacy(lua, "senseis", legacy_id)) lua.pushNil();
    return 1;
}

fn luaCharacterSenseiSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const id = string(lua, 2);
    const legacy_id = legacyForDefinitionId(lua, "senseis", id) orelse lua.raiseErrorStr("unknown sensei '%s'", .{id.ptr});
    cdb.char_class_set(ch, legacy_id);
    return 0;
}

fn luaCharacterRaceGet(lua: *Lua) i32 {
    const legacy_id = cdb.char_race_get(checkCharacter(lua));
    if (!pushDefinitionIdForLegacy(lua, "races", legacy_id)) lua.pushNil();
    return 1;
}

fn luaCharacterRaceSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const id = string(lua, 2);
    const legacy_id = legacyForDefinitionId(lua, "races", id) orelse lua.raiseErrorStr("unknown race '%s'", .{id.ptr});
    cdb.char_race_set(ch, legacy_id);
    return 0;
}

fn luaCharacterSizeGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_size_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterSizeSet(lua: *Lua) i32 {
    cdb.char_size_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "size"));
    return 0;
}

fn luaCharacterSizeMod(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const value = cdb.char_size_get(ch) + intCastOrError(lua, c_int, integer(lua, 2), "size delta");
    cdb.char_size_set(ch, value);
    lua.pushInteger(value);
    return 1;
}

fn luaCharacterSexGet(lua: *Lua) i32 {
    const sex = switch (cdb.char_sex_get(checkCharacter(lua))) {
        sex_neutral => "neutral",
        sex_male => "male",
        sex_female => "female",
        else => "neutral",
    };
    _ = lua.pushString(sex);
    return 1;
}

fn luaCharacterSexSet(lua: *Lua) i32 {
    const value = string(lua, 2);
    const sex: c_int = if (std.mem.eql(u8, value, "neutral"))
        sex_neutral
    else if (std.mem.eql(u8, value, "male"))
        sex_male
    else if (std.mem.eql(u8, value, "female"))
        sex_female
    else
        lua.raiseErrorStr("unknown sex '%s'", .{value.ptr});
    cdb.char_sex_set(checkCharacter(lua), sex);
    return 0;
}

fn luaCharacterPositionGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_position_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterIsFighting(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_fighting_get(checkCharacter(lua)) != null);
    return 1;
}

fn luaCharacterPositionSet(lua: *Lua) i32 {
    cdb.char_position_set(checkCharacter(lua), integer(lua, 2));
    return 0;
}

fn luaCharacterAdminLevelGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_admlevel_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterAdminLevelSet(lua: *Lua) i32 {
    cdb.char_admlevel_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "admin level"));
    return 0;
}

fn luaCharacterAdminLevelMod(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const value = cdb.char_admlevel_get(ch) + intCastOrError(lua, c_int, integer(lua, 2), "admin level delta");
    cdb.char_admlevel_set(ch, value);
    lua.pushInteger(value);
    return 1;
}

fn luaCharacterAdminFlagged(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_admflagged(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "admin flag")));
    return 1;
}

fn luaCharacterAdminFlagSet(lua: *Lua) i32 {
    cdb.char_admflag_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "admin flag"), boolean(lua, 3));
    return 0;
}

fn luaCharacterAdminFlagToggle(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_admflag_toggle(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "admin flag")));
    return 1;
}

fn luaCharacterPlayerFlagged(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_plrflagged(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "player flag")));
    return 1;
}

fn luaCharacterPlayerFlagSet(lua: *Lua) i32 {
    cdb.char_plrflag_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "player flag"), boolean(lua, 3));
    return 0;
}

fn luaCharacterPlayerFlagToggle(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_plrflag_toggle(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "player flag")));
    return 1;
}

fn luaCharacterPrefFlagged(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_prfflagged(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "pref flag")));
    return 1;
}

fn luaCharacterPrefFlagSet(lua: *Lua) i32 {
    cdb.char_prfflag_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "pref flag"), boolean(lua, 3));
    return 0;
}

fn luaCharacterPrefFlagToggle(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_prfflag_toggle(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "pref flag")));
    return 1;
}

fn luaCharacterUserGet(lua: *Lua) i32 {
    const result = cdb.char_user_get(checkCharacter(lua));
    if (result == null) {
        lua.pushNil();
    } else {
        _ = lua.pushString(std.mem.sliceTo(result, 0));
    }
    return 1;
}

fn luaCharacterStatGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_stat_get(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterStatSet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_stat_set(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "stat value")));
    return 1;
}

fn luaCharacterStatMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_stat_mod(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "stat delta")));
    return 1;
}

fn luaCharacterDerivedBase(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_der_base_get(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterDerivedTotal(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_der_total_get(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterDerivedInvalidate(lua: *Lua) i32 {
    cdb.char_der_invalidate(checkCharacter(lua));
    return 0;
}

fn luaCharacterModifiersFor(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const category = string(lua, 2);
    const id = string(lua, 3);
    const zigdata = character_api.char_ensure_zigdata(ch) orelse {
        lua.newTable();
        lua.pushInteger(0);
        lua.setField(-2, "flat");
        lua.pushInteger(0);
        lua.setField(-2, "percent");
        lua.newTable();
        lua.setField(-2, "multipliers");
        lua.pushNil();
        lua.setField(-2, "min");
        lua.pushNil();
        lua.setField(-2, "max");
        lua.pushNil();
        lua.setField(-2, "set");
        return 1;
    };

    if (zigdata.modifiers.dirty) {
        zigdata.modifiers.rebuild(ch);
        lua_api.emitCharacterModifiers(ch, &zigdata.modifiers);
    }

    var flat: i64 = 0;
    var percent: i64 = 0;
    var multipliers: std.ArrayListUnmanaged(i64) = .empty;
    defer multipliers.deinit(std.heap.page_allocator);
    var min_override: ?i64 = null;
    var max_override: ?i64 = null;
    var set_override: ?i64 = null;

    character_api.accumulateDerivedModifiers(&zigdata.modifiers, category, id, &flat, &percent, &multipliers, &min_override, &max_override, &set_override);

    lua.newTable();
    lua.pushInteger(flat);
    lua.setField(-2, "flat");
    lua.pushInteger(percent);
    lua.setField(-2, "percent");
    lua.newTable();
    for (multipliers.items, 0..) |m, i| {
        lua.pushInteger(m);
        lua.setIndex(-2, @intCast(i + 1));
    }
    lua.setField(-2, "multipliers");
    if (min_override) |v| {
        lua.pushInteger(v);
    } else {
        lua.pushNil();
    }
    lua.setField(-2, "min");
    if (max_override) |v| {
        lua.pushInteger(v);
    } else {
        lua.pushNil();
    }
    lua.setField(-2, "max");
    if (set_override) |v| {
        lua.pushInteger(v);
    } else {
        lua.pushNil();
    }
    lua.setField(-2, "set");
    return 1;
}

fn luaCharacterLegacyModifier(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const location = @as(c_int, @intCast(integer(lua, 2)));
    const specific = @as(c_int, @intCast(integer(lua, 3)));
    lua.pushInteger(cdb.char_legacy_modifier(ch, location, specific));
    return 1;
}

fn luaCharacterModifierGen(lua: *Lua) i32 {
    lua.pushInteger(@intCast(cdb.char_modifier_gen_get(checkCharacter(lua))));
    return 1;
}

fn luaCharacterMeterGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_meter_get(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterMeterSet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_meter_set(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "meter value")));
    return 1;
}

fn luaCharacterMeterMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_meter_mod(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "meter delta")));
    return 1;
}

fn luaCharacterMeterSetInt(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_meter_set_int(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "meter current")));
    return 1;
}

fn luaCharacterMeterModInt(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_meter_mod_int(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "meter current delta")));
    return 1;
}

fn luaCharacterMeterCurrent(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_meter_current(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterMeterMax(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_meter_max(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterSkillBaseGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_skill_base_get(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterSkillBaseSet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_skill_base_set(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "skill base")));
    return 1;
}

fn luaCharacterSkillBaseMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_skill_base_mod(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "skill base delta")));
    return 1;
}

fn luaCharacterSkillModifierGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_skill_modifier_get(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterSkillTotalGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_skill_total_get(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterInitSkill(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const name = lua.toString(2) catch lua.typeError(2, "string");
    var buf: [256:0]u8 = undefined;
    const len = @min(name.len, buf.len - 1);
    @memcpy(buf[0..len], name[0..len]);
    buf[len] = 0;
    const SKTYPE_SKILL: c_int = 1 << 1;
    const snum = find_skill_num(&buf, SKTYPE_SKILL);
    if (snum < 0) {
        lua.pushInteger(0);
    } else {
        lua.pushInteger(cdb.init_skill(ch, snum));
    }
    return 1;
}

fn luaCharacterRollSkill(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const name = lua.toString(2) catch lua.typeError(2, "string");
    var buf: [256:0]u8 = undefined;
    const len = @min(name.len, buf.len - 1);
    @memcpy(buf[0..len], name[0..len]);
    buf[len] = 0;
    const SKTYPE_SKILL: c_int = 1 << 1;
    const snum = find_skill_num(&buf, SKTYPE_SKILL);
    lua.pushInteger(roll_skill(ch, snum));
    return 1;
}

fn luaCharacterSkillPerfGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_skill_perf_get(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterSkillPerfSet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_skill_perf_set(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "skill performance")));
    return 1;
}

fn luaCharacterSkillPerfMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_skill_perf_mod(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "skill performance delta")));
    return 1;
}

fn luaCharacterConditionHas(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_condition_has(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterConditionHasTag(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_condition_has_tag(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterConditionActiveWithTag(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const condition = cdb.char_condition_active_with_tag(ch, string(lua, 2));
    if (condition == null) {
        lua.pushNil();
        return 1;
    }
    pushCondition(lua, cdb.char_id_get(ch), std.mem.span(condition));
    return 1;
}

fn luaCharacterConditionAdd(lua: *Lua) i32 {
    const source_category = if (lua.isNoneOrNil(3)) "lua" else string(lua, 3);
    const source_id = if (lua.isNoneOrNil(4)) "unknown" else string(lua, 4);
    lua.pushBoolean(cdb.char_condition_add(checkCharacter(lua), string(lua, 2), source_category, source_id));
    return 1;
}

fn luaCharacterConditionApply(lua: *Lua) i32 {
    const source_category = if (lua.isNoneOrNil(3)) "lua" else string(lua, 3);
    const source_id = if (lua.isNoneOrNil(4)) "unknown" else string(lua, 4);
    lua.pushBoolean(cdb.char_condition_apply(checkCharacter(lua), string(lua, 2), source_category, source_id));
    return 1;
}

fn luaCharacterConditionApplyVariables(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const condition = string(lua, 2);
    var numbers = collectNumberArgs(lua, 3);
    defer numbers.deinit();
    var strings = collectStringArgs(lua, 4);
    defer strings.deinit();
    const source_category = if (lua.isNoneOrNil(5)) "lua" else string(lua, 5);
    const source_id = if (lua.isNoneOrNil(6)) "unknown" else string(lua, 6);
    lua.pushBoolean(cdb.char_condition_apply_with_variables(ch, condition, source_category, source_id, numbers.items.ptr, numbers.items.len, strings.items.ptr, strings.items.len));
    return 1;
}

fn luaCharacterConditionApplyNumber(lua: *Lua) i32 {
    const source_category = if (lua.isNoneOrNil(5)) "lua" else string(lua, 5);
    const source_id = if (lua.isNoneOrNil(6)) "unknown" else string(lua, 6);
    lua.pushBoolean(cdb.char_condition_apply_with_number(checkCharacter(lua), string(lua, 2), source_category, source_id, string(lua, 3), intCastOrError(lua, i64, integer(lua, 4), "condition number")));
    return 1;
}

fn luaCharacterConditionApplyWithDuration(lua: *Lua) i32 {
    const source_category = if (lua.isNoneOrNil(5)) "lua" else string(lua, 5);
    const source_id = if (lua.isNoneOrNil(6)) "unknown" else string(lua, 6);
    const duration = intCastOrError(lua, i64, integer(lua, 4), "duration");
    lua.pushBoolean(cdb.char_condition_apply_with_duration(checkCharacter(lua), string(lua, 2), source_category, source_id, duration));
    return 1;
}

fn luaCharacterConditionRemove(lua: *Lua) i32 {
    const reason = if (lua.isNoneOrNil(3)) "removed" else string(lua, 3);
    lua.pushBoolean(cdb.char_condition_remove(checkCharacter(lua), string(lua, 2), reason));
    return 1;
}

fn collectNumberArgs(lua: *Lua, index: i32) std.array_list.Managed(cdb.condition_number_arg) {
    var result = std.array_list.Managed(cdb.condition_number_arg).init(std.heap.page_allocator);
    if (lua.isNoneOrNil(index)) return result;
    if (!lua.isTable(index)) lua.typeError(index, "table");

    lua.pushNil();
    while (lua.next(index)) {
        const key = lua.toString(-2) catch {
            lua.pop(1);
            continue;
        };
        const value = lua.toInteger(-1) catch {
            lua.pop(1);
            continue;
        };
        result.append(.{ .key = @ptrCast(key.ptr), .value = intCastOrError(lua, i64, value, "condition number") }) catch lua.raiseErrorStr("out of memory", .{});
        lua.pop(1);
    }
    return result;
}

fn collectStringArgs(lua: *Lua, index: i32) std.array_list.Managed(cdb.condition_string_arg) {
    var result = std.array_list.Managed(cdb.condition_string_arg).init(std.heap.page_allocator);
    if (lua.isNoneOrNil(index)) return result;
    if (!lua.isTable(index)) lua.typeError(index, "table");

    lua.pushNil();
    while (lua.next(index)) {
        const key = lua.toString(-2) catch {
            lua.pop(1);
            continue;
        };
        const value = lua.toString(-1) catch {
            lua.pop(1);
            continue;
        };
        result.append(.{ .key = @ptrCast(key.ptr), .value = @ptrCast(value.ptr) }) catch lua.raiseErrorStr("out of memory", .{});
        lua.pop(1);
    }
    return result;
}

fn luaCharacterConditionRemoveTag(lua: *Lua) i32 {
    const reason = if (lua.isNoneOrNil(3)) "removed" else string(lua, 3);
    lua.pushInteger(cdb.char_condition_remove_tag(checkCharacter(lua), string(lua, 2), reason));
    return 1;
}

fn luaCharacterCondition(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const condition = string(lua, 2);
    if (!cdb.char_condition_has(ch, condition)) {
        lua.pushNil();
        return 1;
    }
    pushCondition(lua, cdb.char_id_get(ch), condition);
    return 1;
}

fn luaCharacterConditionNumberGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_condition_number_get(checkCharacter(lua), string(lua, 2), string(lua, 3)));
    return 1;
}

fn luaCharacterConditionNumberSet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_condition_number_set(checkCharacter(lua), string(lua, 2), string(lua, 3), intCastOrError(lua, i64, integer(lua, 4), "condition number")));
    return 1;
}

fn luaCharacterConditionNumberMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_condition_number_mod(checkCharacter(lua), string(lua, 2), string(lua, 3), intCastOrError(lua, i64, integer(lua, 4), "condition number delta")));
    return 1;
}

fn luaCharacterConditionStringGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_condition_string_get(checkCharacter(lua), string(lua, 2), string(lua, 3)));
    return 1;
}

fn luaCharacterConditionStringSet(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_condition_string_set(checkCharacter(lua), string(lua, 2), string(lua, 3), string(lua, 4)));
    return 1;
}

fn luaCharacterTransformHas(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_transform_has(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterTransformAdd(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_transform_add(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterTransformRemove(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_transform_remove(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterTransformUnlocked(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_transform_unlocked(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterTransformUnlock(lua: *Lua) i32 {
    const source = if (lua.isNoneOrNil(3)) "lua" else string(lua, 3);
    lua.pushBoolean(cdb.char_transform_unlock(checkCharacter(lua), string(lua, 2), source));
    return 1;
}

fn luaCharacterTransformNumberGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_transform_number_get(checkCharacter(lua), string(lua, 2), string(lua, 3)));
    return 1;
}

fn luaCharacterTransformNumberSet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_transform_number_set(checkCharacter(lua), string(lua, 2), string(lua, 3), intCastOrError(lua, i64, integer(lua, 4), "transform number")));
    return 1;
}

fn luaCharacterTransformNumberMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_transform_number_mod(checkCharacter(lua), string(lua, 2), string(lua, 3), intCastOrError(lua, i64, integer(lua, 4), "transform number delta")));
    return 1;
}

fn luaCharacterTransformStringGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_transform_string_get(checkCharacter(lua), string(lua, 2), string(lua, 3)));
    return 1;
}

fn luaCharacterTransformStringSet(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_transform_string_set(checkCharacter(lua), string(lua, 2), string(lua, 3), string(lua, 4)));
    return 1;
}

fn luaConditionId(lua: *Lua) i32 {
    _ = lua.pushString(std.mem.span(conditionName(checkConditionHandle(lua))));
    return 1;
}

fn luaConditionStacks(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    lua.pushInteger(cdb.char_condition_stacks_get(conditionCharacter(lua, handle), conditionName(handle)));
    return 1;
}

fn luaConditionStacksSet(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    lua.pushInteger(cdb.char_condition_stacks_set(conditionCharacter(lua, handle), conditionName(handle), intCastOrError(lua, i64, integer(lua, 2), "condition stacks")));
    return 1;
}

fn luaConditionDuration(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    lua.pushInteger(cdb.char_condition_duration_get(conditionCharacter(lua, handle), conditionName(handle)));
    return 1;
}

fn luaConditionDurationSet(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    lua.pushInteger(cdb.char_condition_duration_set(conditionCharacter(lua, handle), conditionName(handle), intCastOrError(lua, i64, integer(lua, 2), "condition duration")));
    return 1;
}

fn luaConditionNumberGet(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    lua.pushInteger(cdb.char_condition_number_get(conditionCharacter(lua, handle), conditionName(handle), string(lua, 2)));
    return 1;
}

fn luaConditionNumberSet(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    lua.pushInteger(cdb.char_condition_number_set(conditionCharacter(lua, handle), conditionName(handle), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "condition number")));
    return 1;
}

fn luaConditionNumberMod(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    lua.pushInteger(cdb.char_condition_number_mod(conditionCharacter(lua, handle), conditionName(handle), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "condition number delta")));
    return 1;
}

fn luaConditionStringGet(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    pushCString(lua, cdb.char_condition_string_get(conditionCharacter(lua, handle), conditionName(handle), string(lua, 2)));
    return 1;
}

fn luaConditionStringSet(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    lua.pushBoolean(cdb.char_condition_string_set(conditionCharacter(lua, handle), conditionName(handle), string(lua, 2), string(lua, 3)));
    return 1;
}

// Build "condition:<cond_id>:<event>" into a stack buffer. Returns a sentinel-terminated slice.
fn condEventKind(buf: *[192:0]u8, cond: []const u8, event: []const u8) ?[:0]u8 {
    const prefix = "condition:";
    const total = prefix.len + cond.len + 1 + event.len;
    if (total >= buf.len) return null;
    @memcpy(buf[0..prefix.len], prefix);
    @memcpy(buf[prefix.len..][0..cond.len], cond);
    buf[prefix.len + cond.len] = ':';
    @memcpy(buf[prefix.len + cond.len + 1 ..][0..event.len], event);
    buf[total] = 0;
    return buf[0..total :0];
}

// cond:schedule_event(ch, name, delay_ms [, interval_ms])
fn luaConditionScheduleEvent(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    const ch = conditionCharacter(lua, handle);
    const event_name = string(lua, 2);
    const delay_ms = intCastOrError(lua, i64, integer(lua, 3), "delay_ms");
    const interval_ms: i64 = if (lua.isNoneOrNil(4)) 0 else intCastOrError(lua, i64, integer(lua, 4), "interval_ms");
    const cond = std.mem.span(conditionName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = condEventKind(&buf, cond, event_name) orelse {
        lua.pushInteger(0);
        return 1;
    };
    // Cancel any existing event with this kind first (idempotent).
    _ = eq_cancel_owner(1, cdb.char_id_get(ch), kind.ptr);
    const fire_at = event_queue_now_ms() + delay_ms;
    const id = event_schedule_lua_char_update(fire_at, interval_ms, kind.ptr, cdb.char_id_get(ch));
    lua.pushInteger(@intCast(id));
    return 1;
}

// cond:cancel_event(ch, name)
fn luaConditionCancelEvent(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    const ch = conditionCharacter(lua, handle);
    const event_name = string(lua, 2);
    const cond = std.mem.span(conditionName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = condEventKind(&buf, cond, event_name) orelse return 0;
    _ = eq_cancel_owner(1, cdb.char_id_get(ch), kind.ptr);
    return 0;
}

// cond:event_pending(ch, name) -> bool
fn luaConditionEventPending(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    const ch = conditionCharacter(lua, handle);
    const event_name = string(lua, 2);
    const cond = std.mem.span(conditionName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = condEventKind(&buf, cond, event_name) orelse {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(eq_owner_count(1, cdb.char_id_get(ch), kind.ptr) > 0);
    return 1;
}

// cond:event_next_ms(ch, name) -> i64 or -1
fn luaConditionEventNextMs(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    const ch = conditionCharacter(lua, handle);
    const event_name = string(lua, 2);
    const cond = std.mem.span(conditionName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = condEventKind(&buf, cond, event_name) orelse {
        lua.pushInteger(-1);
        return 1;
    };
    lua.pushInteger(eq_owner_next_ms(1, cdb.char_id_get(ch), kind.ptr));
    return 1;
}

// cond:schedule_expire(ch, secs) — schedule a one-shot "expire" event
fn luaConditionScheduleExpire(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    const ch = conditionCharacter(lua, handle);
    const secs = intCastOrError(lua, i64, integer(lua, 2), "duration_secs");
    const cond = std.mem.span(conditionName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = condEventKind(&buf, cond, "expire") orelse return 0;
    _ = eq_cancel_owner(1, cdb.char_id_get(ch), kind.ptr);
    if (secs > 0)
        _ = event_schedule_lua_char_update(event_queue_now_ms() + secs * 1000, 0, kind.ptr, cdb.char_id_get(ch));
    return 0;
}

// cond:remaining_ms(ch) -> relative ms until "expire" event, or -1 if permanent
fn luaConditionRemainingMs(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    const ch = conditionCharacter(lua, handle);
    const cond = std.mem.span(conditionName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = condEventKind(&buf, cond, "expire") orelse {
        lua.pushInteger(-1);
        return 1;
    };
    lua.pushInteger(eq_owner_next_ms(1, cdb.char_id_get(ch), kind.ptr));
    return 1;
}

// cond:remaining_secs(ch) -> seconds until "expire" event (floored), or -1 if permanent
fn luaConditionRemainingSecs(lua: *Lua) i32 {
    const handle = checkConditionHandle(lua);
    const ch = conditionCharacter(lua, handle);
    const cond = std.mem.span(conditionName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = condEventKind(&buf, cond, "expire") orelse {
        lua.pushInteger(-1);
        return 1;
    };
    const ms = eq_owner_next_ms(1, cdb.char_id_get(ch), kind.ptr);
    lua.pushInteger(if (ms < 0) -1 else @divFloor(ms, 1000));
    return 1;
}

fn luaCharacterInventoryCount(lua: *Lua) i32 {
    const recursive = if (lua.isNoneOrNil(2)) false else boolean(lua, 2);
    lua.pushInteger(@intCast(cdb.char_inventory_count(checkCharacter(lua), recursive)));
    return 1;
}

fn luaCharacterEquipmentCount(lua: *Lua) i32 {
    const recursive = if (lua.isNoneOrNil(2)) false else boolean(lua, 2);
    lua.pushInteger(@intCast(cdb.char_equipment_count(checkCharacter(lua), recursive)));
    return 1;
}

fn luaCharacterInventoryGet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (lua.isNoneOrNil(2)) return pushInventoryIterator(lua, ch);

    var count: usize = 0;
    const ids = cdb.char_inventory_get(ch, &count);
    defer if (ids) |_| std.c.free(@as(?*anyopaque, @ptrCast(ids)));

    const pos = intCastOrError(lua, usize, integer(lua, 2), "inventory index");
    if (ids == null or pos >= count) {
        lua.pushNil();
        return 1;
    }
    objects_lua.pushObject(lua, ids[pos]);
    return 1;
}

fn luaCharacterEquipmentGet(lua: *Lua) i32 {
    if (lua.isNoneOrNil(2)) return pushEquipmentIterator(lua, checkCharacter(lua));

    const obj = cdb.char_equipment_get(checkCharacter(lua), intCastOrError(lua, usize, integer(lua, 2), "equipment index"));
    if (obj == null) {
        lua.pushNil();
        return 1;
    }
    objects_lua.pushObject(lua, cdb.obj_id_get(obj));
    return 1;
}

fn pushInventoryIterator(lua: *Lua, ch: *cdb.char_data) i32 {
    var count: usize = 0;
    const ids = cdb.char_inventory_get(ch, &count);
    defer if (ids) |_| std.c.free(@as(?*anyopaque, @ptrCast(ids)));

    lua.newTable();
    for (0..count) |i| {
        if (ids) |ptr| {
            objects_lua.pushObject(lua, ptr[i]);
            lua.setIndex(-2, @intCast(i + 1));
        }
    }
    return valueIterator(lua);
}

fn pushEquipmentIterator(lua: *Lua, ch: *cdb.char_data) i32 {
    lua.newTable();
    var pos: usize = 0;
    while (true) : (pos += 1) {
        if (pos >= cdb.NUM_WEARS) break;
        const obj = cdb.char_equipment_get(ch, pos);
        if (obj == null) continue;
        objects_lua.pushObject(lua, cdb.obj_id_get(obj));
        lua.setIndex(-2, @intCast(pos));
    }
    return pairsIterator(lua);
}

fn valueIterator(lua: *Lua) i32 {
    _ = lua.getGlobal("dbat");
    _ = lua.getField(-1, "_values");
    lua.remove(-2);
    lua.insert(-2);
    lua.protectedCall(.{ .args = 1, .results = 1 }) catch lua.raiseErrorStr("failed to create value iterator", .{});
    return 1;
}

fn pairsIterator(lua: *Lua) i32 {
    _ = lua.getGlobal("pairs");
    lua.insert(-2);
    lua.protectedCall(.{ .args = 1, .results = 3 }) catch lua.raiseErrorStr("failed to create pairs iterator", .{});
    return 3;
}

fn luaCharacterCommandQueueClear(lua: *Lua) i32 {
    cdb.char_command_clear(checkCharacter(lua));
    return 0;
}

fn luaCharacterCommandEnqueue(lua: *Lua) i32 {
    cdb.char_command_enqueue(checkCharacter(lua), string(lua, 2));
    return 0;
}

// entity:event_schedule(kind, delay_ms [, interval_ms]) → event_id
fn luaCharacterEventSchedule(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const kind = string(lua, 2);
    const delay_ms = integer(lua, 3);
    const interval_ms: i64 = if (lua.typeOf(4) == .number) @intCast(integer(lua, 4)) else 0;
    const now = event_queue_now_ms();
    const id = event_schedule_lua_char_update(now + delay_ms, interval_ms, kind.ptr, cdb.char_id_get(ch));
    lua.pushInteger(@intCast(id));
    return 1;
}

// entity:event_cancel(kind) → count cancelled
fn luaCharacterEventCancel(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const kind = string(lua, 2);
    const n = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_CHAR), cdb.char_id_get(ch), kind.ptr);
    lua.pushInteger(n);
    return 1;
}

// entity:event_count([kind]) → integer
fn luaCharacterEventCount(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const kind: ?[*:0]const u8 = if (lua.typeOf(2) == .string) string(lua, 2).ptr else null;
    const n = eq_owner_count(@as(c_int, cdb.EQ_OWNER_CHAR), cdb.char_id_get(ch), kind);
    lua.pushInteger(n);
    return 1;
}

// entity:event_remaining_ms([kind]) → integer or -1
fn luaCharacterEventRemainingMs(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const kind: ?[*:0]const u8 = if (lua.typeOf(2) == .string) string(lua, 2).ptr else null;
    const ms = eq_owner_next_ms(@as(c_int, cdb.EQ_OWNER_CHAR), cdb.char_id_get(ch), kind);
    lua.pushInteger(ms);
    return 1;
}

fn luaCharacterTimePlayed(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).time.played);
    return 1;
}

fn luaCharacterAgeYears(lua: *Lua) i32 {
    lua.pushInteger(cdb.age(checkCharacter(lua))[0].year);
    return 1;
}

fn luaCharacterClanGet(lua: *Lua) i32 {
    pushCString(lua, checkCharacter(lua).clan);
    return 1;
}

fn luaCharacterRpGet(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).rp);
    return 1;
}

fn luaCharacterRpSet(lua: *Lua) i32 {
    cdb.char_rp_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "rp value"));
    return 0;
}

fn luaCharacterRpSave(lua: *Lua) i32 {
    cdb.char_rp_save(checkCharacter(lua));
    return 0;
}

fn luaCharacterHeightCm(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const raw = intCastOrError(lua, c_int, cdb.char_der_total_get(ch, "height"), "height");
    lua.pushInteger(cdb.get_measure(ch, raw, 0));
    return 1;
}

fn luaCharacterWeightKg(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const raw = intCastOrError(lua, c_int, cdb.char_der_total_get(ch, "weight"), "weight");
    lua.pushInteger(cdb.get_measure(ch, 0, raw));
    return 1;
}

fn luaCharacterAlignStr(lua: *Lua) i32 {
    pushCString(lua, cdb.disp_align(checkCharacter(lua)));
    return 1;
}

fn luaCharacterLevelExp(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const level = intCastOrError(lua, c_int, integer(lua, 2), "level");
    lua.pushInteger(cdb.char_level_exp(ch, level));
    return 1;
}

fn luaCharacterRppToLevel(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_rpp_to_level(checkCharacter(lua)));
    return 1;
}

fn luaCharacterMoltThreshold(lua: *Lua) i32 {
    lua.pushInteger(cdb.molt_threshold(checkCharacter(lua)));
    return 1;
}

fn luaCharacterAffFlagged(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_affflagged(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "aff flag")));
    return 1;
}

fn luaCharacterBodyFlagged(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_bodyflagged(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "body slot")));
    return 1;
}

fn luaCharacterLimbCondGet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const n = intCastOrError(lua, c_int, integer(lua, 2), "limb index");
    lua.pushInteger(cdb.char_limbcond_get(ch, n));
    return 1;
}
fn luaCharacterLimbCondSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const n = intCastOrError(lua, c_int, integer(lua, 2), "limb index");
    const val = intCastOrError(lua, c_int, integer(lua, 3), "limb value");
    cdb.char_limbcond_set(ch, n, val);
    return 0;
}
fn luaCharacterLimbOk(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const n = intCastOrError(lua, c_int, integer(lua, 2), "limb type");
    lua.pushBoolean(limb_ok(ch, n) != 0);
    return 1;
}

fn luaCharacterGainTail(lua: *Lua) i32 {
    cdb.char_gain_tail(checkCharacter(lua), false);
    return 0;
}

fn luaCharacterHasTail(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_has_tail(checkCharacter(lua)));
    return 1;
}

fn luaCharacterLoseTail(lua: *Lua) i32 {
    cdb.char_lose_tail(checkCharacter(lua));
    return 0;
}

fn luaCharacterRemoveLimb(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const n = intCastOrError(lua, c_int, integer(lua, 2), "limb index");
    cdb.remove_limb(ch, n);
    return 0;
}

fn luaCharacterWieldedWeaponType(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const weapon = cdb.char_equipment_get(ch, cdb.WEAR_WIELD1) orelse {
        lua.pushNil();
        return 1;
    };
    const dam_val = cdb.obj_value_get(weapon, 3); // VAL_WEAPON_DAMTYPE = 3
    const attack_type = dam_val + cdb.TYPE_HIT;
    const type_name: [:0]const u8 = switch (attack_type) {
        cdb.TYPE_SLASH => "slash",
        cdb.TYPE_PIERCE => "pierce",
        cdb.TYPE_STAB => "stab",
        cdb.TYPE_CRUSH => "crush",
        cdb.TYPE_BLUDGEON => "bludgeon",
        cdb.TYPE_BITE => "bite",
        cdb.TYPE_CLAW => "claw",
        cdb.TYPE_WHIP => "whip",
        cdb.TYPE_POUND => "pound",
        else => "hit",
    };
    _ = lua.pushString(type_name);
    return 1;
}

fn luaCharacterChargeGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_charge_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterChargeSet(lua: *Lua) i32 {
    cdb.char_charge_set(checkCharacter(lua), intCastOrError(lua, i64, integer(lua, 2), "charge"));
    return 0;
}

fn luaCharacterBarrierGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_barrier_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterVoiceGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_voice_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterDistfeaGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_distfea_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterRdisplayGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_rdisplay_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterFeatureGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_feature_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterFeatureSet(lua: *Lua) i32 {
    cdb.char_feature_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterBringToCap(lua: *Lua) i32 {
    cdb.char_bring_to_cap(checkCharacter(lua));
    return 0;
}

fn luaCharacterRadar1Get(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_radar1_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterRadar1Set(lua: *Lua) i32 {
    cdb.char_radar1_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "radar1 vnum"));
    return 0;
}
fn luaCharacterRadar2Get(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_radar2_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterRadar2Set(lua: *Lua) i32 {
    cdb.char_radar2_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "radar2 vnum"));
    return 0;
}
fn luaCharacterRadar3Get(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_radar3_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterRadar3Set(lua: *Lua) i32 {
    cdb.char_radar3_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "radar3 vnum"));
    return 0;
}
fn luaCharacterHasArms(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_has_arms(checkCharacter(lua)));
    return 1;
}
fn luaCharacterPlayerIdGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_idnum_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterRppCustomEquipLaunch(lua: *Lua) i32 {
    cdb.char_rpp_custom_equip_launch(checkCharacter(lua));
    return 0;
}

fn luaCharacterRppRestringLaunch(lua: *Lua) i32 {
    cdb.char_rpp_restring_launch(checkCharacter(lua), objects_lua.checkObjectAt(lua, 2));
    return 0;
}

fn luaCharacterAbsorbsGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_absorbs_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterAbsorbsSet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_absorbs_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "absorbs")));
    return 1;
}
fn luaCharacterAbsorbsMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_absorbs_mod(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "absorbs delta")));
    return 1;
}

fn luaCharacterHandleIngestLearn(lua: *Lua) i32 {
    cdb.char_handle_ingest_learn(checkCharacter(lua), checkCharacterAt(lua, 2));
    return 0;
}

fn luaCharacterMimicGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_mimic_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterMimicSet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_mimic_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "mimic race")));
    return 1;
}

fn luaCharacterBackstabCooldown(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_backstab_cooldown_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterPreferenceGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_preference_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterPreferenceSet(lua: *Lua) i32 {
    cdb.char_preference_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "preference value"));
    return 0;
}

fn luaCharacterGenomeGet(lua: *Lua) i32 {
    const slot = intCastOrError(lua, c_int, integer(lua, 2), "genome slot");
    lua.pushInteger(cdb.char_genome_get(checkCharacter(lua), slot));
    return 1;
}

fn luaCharacterWaitSet(lua: *Lua) i32 {
    cdb.char_wait_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "pulses"));
    return 0;
}

fn luaCharacterCheckSpecial(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const dir = intCastOrError(lua, c_int, integer(lua, 2), "dir");
    var throwaway: [512:0]u8 = std.mem.zeroes([512:0]u8);
    lua.pushBoolean(cdb.special(ch, dir + 1, &throwaway) != 0);
    return 1;
}

fn luaCharacterCooldownGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_cooldown_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterCooldownSet(lua: *Lua) i32 {
    cdb.char_cooldown_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "cooldown"));
    return 0;
}

fn luaCharacterSelfdestructCooldownGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_selfdestruct_cooldown_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterSelfdestructCooldownSet(lua: *Lua) i32 {
    cdb.char_selfdestruct_cooldown_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "selfdestruct cooldown"));
    return 0;
}

fn luaCharacterInventoryFindVnum(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const vnum = intCastOrError(lua, cdb.obj_vnum, integer(lua, 2), "vnum");
    const flags: c_int = if (lua.isNoneOrNil(3)) 0 else intCastOrError(lua, c_int, integer(lua, 3), "search flags");
    const obj = cdb.char_inventory_search_vnum(ch, vnum, false, flags);
    if (obj) |o| objects_lua.pushObject(lua, cdb.obj_id_get(o)) else lua.pushNil();
    return 1;
}

fn luaCharacterKnowSkill(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_know_skill(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterImproveSkill(lua: *Lua) i32 {
    const flag: c_int = if (lua.isNoneOrNil(3)) 0 else intCastOrError(lua, c_int, integer(lua, 3), "flag");
    cdb.char_improve_skill(checkCharacter(lua), string(lua, 2), flag);
    return 0;
}

fn luaCharacterDefendingForGet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const id = cdb.char_condition_number_get(ch, "defending_for", "target_id");
    if (id == 0 or cdb.char_by_id(id) == null) {
        lua.pushNil();
        return 1;
    }
    pushCharacter(lua, id);
    return 1;
}

fn luaCharacterDefendingForSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    _ = cdb.char_condition_remove(ch, "defending_for", "update");
    if (!lua.isNoneOrNil(2)) {
        const vict = checkCharacterAt(lua, 2);
        _ = cdb.char_condition_apply_with_number(ch, "defending_for", "combat", "defend", "target_id", cdb.char_id_get(vict));
    }
    return 0;
}

fn luaCharacterDefendedByGet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const id = cdb.char_condition_number_get(ch, "defended_by", "target_id");
    if (id == 0 or cdb.char_by_id(id) == null) {
        lua.pushNil();
        return 1;
    }
    pushCharacter(lua, id);
    return 1;
}

fn luaCharacterDefendedBySet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    _ = cdb.char_condition_remove(ch, "defended_by", "update");
    if (!lua.isNoneOrNil(2)) {
        const defender = checkCharacterAt(lua, 2);
        _ = cdb.char_condition_apply_with_number(ch, "defended_by", "combat", "defend", "target_id", cdb.char_id_get(defender));
    }
    return 0;
}

fn luaCharacterAuraGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_aura_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterAuraSet(lua: *Lua) i32 {
    cdb.char_aura_set(checkCharacter(lua), @intCast(integer(lua, 2)));
    return 0;
}

fn luaCharacterHairlGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_hairl_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterHairlSet(lua: *Lua) i32 {
    cdb.char_hairl_set(checkCharacter(lua), @intCast(integer(lua, 2)));
    return 0;
}

fn luaCharacterHairsGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_hairs_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterHairsSet(lua: *Lua) i32 {
    cdb.char_hairs_set(checkCharacter(lua), @intCast(integer(lua, 2)));
    return 0;
}

fn luaCharacterHaircGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_hairc_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterHaircSet(lua: *Lua) i32 {
    cdb.char_hairc_set(checkCharacter(lua), @intCast(integer(lua, 2)));
    return 0;
}

fn luaCharacterSkinGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_skin_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterSkinSet(lua: *Lua) i32 {
    cdb.char_skin_set(checkCharacter(lua), @intCast(integer(lua, 2)));
    return 0;
}

fn luaCharacterEyeGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_eye_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterEyeSet(lua: *Lua) i32 {
    cdb.char_eye_set(checkCharacter(lua), @intCast(integer(lua, 2)));
    return 0;
}
fn luaCharacterDistfeaSet(lua: *Lua) i32 {
    cdb.char_distfea_set(checkCharacter(lua), @intCast(integer(lua, 2)));
    return 0;
}

fn luaCharacterSleepcountGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_sleeptime_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterHasGroup(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_has_group(checkCharacter(lua)));
    return 1;
}

fn luaCharacterHasMail(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_has_mail(checkCharacter(lua)));
    return 1;
}

fn luaCharacterStarphaseGet(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).starphase);
    return 1;
}

fn luaCharacterSoftCap(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_soft_cap(checkCharacter(lua)));
    return 1;
}

fn luaCharacterIsSoftCapped(lua: *Lua) i32 {
    lua.pushBoolean(cdb.is_soft_cap(checkCharacter(lua), 0));
    return 1;
}

fn luaCharacterGainExp(lua: *Lua) i32 {
    cdb.gain_exp(checkCharacter(lua), intCastOrError(lua, i64, integer(lua, 2), "exp gain"));
    return 0;
}

fn luaCharacterGainCondition(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const cond = intCastOrError(lua, c_int, integer(lua, 2), "condition");
    const value = intCastOrError(lua, c_int, integer(lua, 3), "value");
    cdb.gain_condition(ch, cond, value);
    return 0;
}

fn luaCharacterMobFlagged(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const flag = intCastOrError(lua, c_int, integer(lua, 2), "mob flag");
    lua.pushBoolean(cdb.flag_test(@ptrCast(&ch.act), cdb.MOB_ISNPC) != 0 and
        bitflags.get(ch.act[0..], flag));
    return 1;
}

fn luaCharacterMobFlagSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const flag = intCastOrError(lua, c_int, integer(lua, 2), "mob flag");
    bitflags.set(ch.act[0..], flag, boolean(lua, 3));
    return 0;
}

fn luaCharacterIsShopkeeper(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    lua.pushBoolean(cdb.mob_proto_special_get(ch.proto_id) == shop_keeper);
    return 1;
}

fn luaCharacterIsSoftCapType(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const cap_type = intCastOrError(lua, i64, integer(lua, 2), "soft cap type");
    lua.pushBoolean(cdb.is_soft_cap(ch, cap_type));
    return 1;
}

fn luaCharacterFollowingGet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (cdb.char_following_get(ch)) |leader|
        pushCharacter(lua, cdb.char_id_get(leader))
    else
        lua.pushNil();
    return 1;
}

fn luaCharacterGroupBonus(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const bonus_type = intCastOrError(lua, c_int, integer(lua, 2), "bonus type");
    lua.pushInteger(cdb.group_bonus(ch, bonus_type));
    return 1;
}

fn luaCharacterStartFighting(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (lua.isNoneOrNil(2)) return 0;
    const target = checkCharacterAt(lua, 2);
    cdb.set_fighting(ch, target);
    return 0;
}

fn luaCharacterStopFighting(lua: *Lua) i32 {
    cdb.stop_fighting(checkCharacter(lua));
    return 0;
}

fn luaCharacterFlee(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const dir = if (lua.getTop() >= 2) lua.toString(2) catch "" else "";
    var buf: [128]u8 = undefined;
    const len = @min(dir.len, buf.len - 1);
    @memcpy(buf[0..len], dir[0..len]);
    buf[len] = 0;
    _ = cdb.char_cmd_execute(ch, @constCast("flee"), &buf);
    return 0;
}

fn luaCharacterFollowersEach(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (lua.typeOf(2) != .function) lua.typeError(2, "function");
    var count: usize = 0;
    const ids = cdb.char_follower_ids(ch, &count) orelse return 0;
    defer cdb.char_follower_ids_free(ids);
    for (ids[0..count]) |id| {
        lua.pushValue(2);
        pushCharacter(lua, id);
        lua.protectedCall(.{ .args = 1, .results = 0 }) catch {};
    }
    return 0;
}

fn luaCharacterAddFollower(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const leader = checkCharacterAt(lua, 2);
    cdb.add_follower(ch, leader);
    return 0;
}

fn luaCharacterStopFollower(lua: *Lua) i32 {
    cdb.stop_follower(checkCharacter(lua));
    return 0;
}

fn luaCharacterCircleFollow(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const target = checkCharacterAt(lua, 2);
    lua.pushBoolean(cdb.circle_follow(ch, target));
    return 1;
}

fn luaCharacterClones(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    var count: usize = 0;
    const ids = cdb.char_clone_ids(ch, &count) orelse {
        lua.newTable();
        return valueIterator(lua);
    };
    defer cdb.char_clone_ids_free(ids);
    lua.newTable();
    for (0..count) |i| {
        pushCharacter(lua, ids[i]);
        lua.setIndex(-2, @intCast(i + 1));
    }
    return valueIterator(lua);
}

fn luaCharacterCloneCount(lua: *Lua) i32 {
    lua.pushInteger(@intCast(cdb.char_clone_count(checkCharacter(lua))));
    return 1;
}

fn luaCharacterCloneAdd(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const clone = checkCharacterAt(lua, 2);
    cdb.char_clone_add(ch, clone);
    return 0;
}

fn luaCharacterConditionsActive(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    lua.newTable();
    if (ch.zigdata == null) return 1;
    const zigdata: *character_api.CharacterData = @ptrCast(@alignCast(ch.zigdata.?));
    var it = zigdata.conditions.keyIterator();
    while (it.next()) |id_ptr| {
        const name = intern_mod.nameOf(id_ptr.*);
        _ = lua.pushString(name);
        lua.pushBoolean(true);
        lua.setTable(-3);
    }
    return 1;
}

fn luaCharacterConditionNumberVars(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const cond_name = string(lua, 2);
    lua.newTable();
    const instance = character_api.conditionGetByName(ch, cond_name) orelse return 1;
    var it = instance.numbers.iterator();
    while (it.next()) |entry| {
        _ = lua.pushString(entry.key_ptr.*);
        lua.pushInteger(entry.value_ptr.*);
        lua.setTable(-3);
    }
    return 1;
}

fn luaCharacterConditionStringVars(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const cond_name = string(lua, 2);
    lua.newTable();
    const instance = character_api.conditionGetByName(ch, cond_name) orelse return 1;
    var it = instance.strings.iterator();
    while (it.next()) |entry| {
        _ = lua.pushString(entry.key_ptr.*);
        _ = lua.pushString(entry.value_ptr.*);
        lua.setTable(-3);
    }
    return 1;
}

fn luaCharacterNewsPending(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_news_pending(checkCharacter(lua)));
    return 1;
}

fn luaCharacterIntroKnown(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const vict = checkCharacterAt(lua, 2);
    lua.pushInteger(cdb.char_intro_known(ch, vict));
    return 1;
}

fn luaCharacterGetIntroName(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const vict = checkCharacterAt(lua, 2);
    const name = cdb.char_intro_name_get(ch, vict);
    if (name) |n| _ = lua.pushString(std.mem.span(n)) else lua.pushNil();
    return 1;
}

fn luaCharacterIntrodCalc(lua: *Lua) i32 {
    const result = cdb.char_introd_calc(checkCharacter(lua));
    if (result) |r| _ = lua.pushString(std.mem.span(r)) else lua.pushNil();
    return 1;
}

fn luaCharacterBonusFlagged(lua: *Lua) i32 {
    const n = intCastOrError(lua, c_int, integer(lua, 2), "bonus index");
    lua.pushBoolean(cdb.char_bonus_flagged(checkCharacter(lua), n));
    return 1;
}
fn luaCharacterAffFlagSet(lua: *Lua) i32 {
    const flag = intCastOrError(lua, c_int, integer(lua, 2), "aff flag");
    cdb.char_affflag_set(checkCharacter(lua), flag, boolean(lua, 3));
    return 0;
}
fn luaCharacterBarrierSet(lua: *Lua) i32 {
    cdb.char_barrier_set(checkCharacter(lua), integer(lua, 2));
    return 0;
}
fn luaCharacterCarryDrop(lua: *Lua) i32 {
    const t: c_int = if (lua.isNoneOrNil(2)) 0 else intCastOrError(lua, c_int, integer(lua, 2), "carry drop type");
    cdb.char_carry_drop(checkCharacter(lua), t);
    return 0;
}
fn luaCharacterLand(lua: *Lua) i32 {
    cdb.char_land(checkCharacter(lua));
    return 0;
}
fn luaCharacterArenaIdnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_arena_idnum_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterArenaIdnumSet(lua: *Lua) i32 {
    cdb.char_arena_idnum_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "arena idnum"));
    return 0;
}
fn luaCharacterDroomGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_droom_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterDroomSet(lua: *Lua) i32 {
    cdb.char_droom_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "droom vnum"));
    return 0;
}

fn luaCharacterDraggingGet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const id = cdb.char_condition_number_get(ch, "dragging", "target_id");
    if (id == 0 or cdb.char_by_id(id) == null) {
        lua.pushNil();
        return 1;
    }
    pushCharacter(lua, id);
    return 1;
}
fn luaCharacterDraggingSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    _ = cdb.char_condition_remove(ch, "dragging", "update");
    if (!lua.isNoneOrNil(2)) {
        const vict = checkCharacterAt(lua, 2);
        _ = cdb.char_condition_apply_with_number(ch, "dragging", "movement", "drag", "target_id", cdb.char_id_get(vict));
    }
    return 0;
}
fn luaCharacterBeingDraggedGet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const id = cdb.char_condition_number_get(ch, "being_dragged", "target_id");
    if (id == 0 or cdb.char_by_id(id) == null) {
        lua.pushNil();
        return 1;
    }
    pushCharacter(lua, id);
    return 1;
}
fn luaCharacterBeingDraggedSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    _ = cdb.char_condition_remove(ch, "being_dragged", "update");
    if (!lua.isNoneOrNil(2)) {
        const dragger = checkCharacterAt(lua, 2);
        _ = cdb.char_condition_apply_with_number(ch, "being_dragged", "movement", "drag", "target_id", cdb.char_id_get(dragger));
    }
    return 0;
}
fn luaCharacterCarryingCharGet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const id = cdb.char_condition_number_get(ch, "carrying_char", "target_id");
    if (id == 0 or cdb.char_by_id(id) == null) {
        lua.pushNil();
        return 1;
    }
    pushCharacter(lua, id);
    return 1;
}
fn luaCharacterCarryingCharSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    _ = cdb.char_condition_remove(ch, "carrying_char", "update");
    if (lua.typeOf(2) != .nil) {
        const carried = checkCharacterAt(lua, 2);
        _ = cdb.char_condition_apply_with_number(ch, "carrying_char", "movement", "carry", "target_id", cdb.char_id_get(carried));
    }
    return 0;
}
fn luaCharacterCarriedByCharGet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const id = cdb.char_condition_number_get(ch, "carried_by_char", "target_id");
    if (id == 0 or cdb.char_by_id(id) == null) {
        lua.pushNil();
        return 1;
    }
    pushCharacter(lua, id);
    return 1;
}
fn luaCharacterCarriedByCharSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    _ = cdb.char_condition_remove(ch, "carried_by_char", "update");
    if (lua.typeOf(2) != .nil) {
        const carrier = checkCharacterAt(lua, 2);
        _ = cdb.char_condition_apply_with_number(ch, "carried_by_char", "movement", "carry", "target_id", cdb.char_id_get(carrier));
    }
    return 0;
}

fn luaCharacterPoofInGet(lua: *Lua) i32 {
    const s = cdb.char_poofin_get(checkCharacter(lua));
    if (s == null) lua.pushNil() else _ = lua.pushString(std.mem.span(s.?));
    return 1;
}
fn luaCharacterPoofOutGet(lua: *Lua) i32 {
    const s = cdb.char_poofout_get(checkCharacter(lua));
    if (s == null) lua.pushNil() else _ = lua.pushString(std.mem.span(s.?));
    return 1;
}
fn luaCharacterLoadRoomGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_loadroom_get(checkCharacter(lua)));
    return 1;
}
fn luaCharacterLoadRoomSet(lua: *Lua) i32 {
    cdb.char_loadroom_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "loadroom vnum"));
    return 0;
}
fn luaCharacterLookAtRoom(lua: *Lua) i32 {
    cdb.char_look_at_room(checkCharacter(lua));
    return 0;
}
fn luaCharacterLookAtSpecificRoom(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const room = rooms_lua.checkRoomAt(lua, 2);
    cdb.char_look_at_specific_room(ch, room);
    return 0;
}
fn luaCharacterRestore(lua: *Lua) i32 {
    const vict = checkCharacter(lua);
    const healer = checkCharacterAt(lua, 2);
    cdb.char_restore(vict, healer);
    return 0;
}
fn luaCharacterFindTargetRoom(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const room = cdb.char_find_target_room(ch, string(lua, 2));
    if (room == null) {
        lua.pushNil();
        return 1;
    }
    rooms_lua.pushRoom(lua, cdb.room_vnum_get(room.?));
    return 1;
}
fn luaMobProtoNameGet(lua: *Lua) i32 {
    const proto = checkMobProto(lua);
    pushCString(lua, proto.name);
    return 1;
}
fn luaMobProtoShortDescrGet(lua: *Lua) i32 {
    const proto = checkMobProto(lua);
    pushCString(lua, proto.short_descr);
    return 1;
}
fn luaMobProtoHasTrig(lua: *Lua) i32 {
    const proto = checkMobProto(lua);
    lua.pushBoolean(proto.proto_script != null);
    return 1;
}
fn luaMobProtosAll(lua: *Lua) i32 {
    lua.newTable();
    const iterator = cdb.mob_proto_iterator_create();
    defer cdb.mob_proto_iterator_free(iterator);
    var index: zlua.Integer = 1;
    while (true) {
        const proto_c = cdb.mob_proto_next(iterator);
        if (proto_c == null) break;
        const proto: *cdb.mob_proto_data = @ptrCast(@alignCast(proto_c));
        pushMobProto(lua, proto.id);
        lua.setIndex(-2, index);
        index += 1;
    }
    return valueIterator(lua);
}

// Combat-pointer getters — return Character or nil
fn luaCharacterFightingGet(lua: *Lua) i32 {
    const t = cdb.char_fighting_get(checkCharacter(lua));
    if (t) |v| pushCharacter(lua, cdb.char_id_get(v)) else lua.pushNil();
    return 1;
}
fn luaCharacterGrapplingGet(lua: *Lua) i32 {
    const t = cdb.char_grappling_get(checkCharacter(lua));
    if (t) |v| pushCharacter(lua, cdb.char_id_get(v)) else lua.pushNil();
    return 1;
}
fn luaCharacterGrappledGet(lua: *Lua) i32 {
    const t = cdb.char_grappled_get(checkCharacter(lua));
    if (t) |v| pushCharacter(lua, cdb.char_id_get(v)) else lua.pushNil();
    return 1;
}
fn luaCharacterGrapplingSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const graptype: c_int = if (lua.getTop() >= 3) @intCast(lua.toInteger(3) catch 0) else 0;
    if (lua.typeOf(2) == .nil)
        cdb.char_grappling_set(ch, null, graptype)
    else
        cdb.char_grappling_set(ch, checkCharacterAt(lua, 2), graptype);
    return 0;
}
fn luaCharacterGrappledSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const graptype: c_int = if (lua.getTop() >= 3) @intCast(lua.toInteger(3) catch 0) else 0;
    if (lua.typeOf(2) == .nil)
        cdb.char_grappled_set(ch, null, graptype)
    else
        cdb.char_grappled_set(ch, checkCharacterAt(lua, 2), graptype);
    return 0;
}
fn luaCharacterGraptypeGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_graptype_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterCanKill(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const vict = checkCharacterAt(lua, 2);
    const mode: c_int = if (lua.isNoneOrNil(3)) 1 else intCastOrError(lua, c_int, integer(lua, 3), "can_kill mode");
    lua.pushBoolean(cdb.can_kill(ch, vict, null, mode) != 0);
    return 1;
}
fn luaCharacterLastAtkGet(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).lastattack);
    return 1;
}
fn luaCharacterLastAtkSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    ch.*.lastattack = @intCast(lua.toInteger(2) catch lua.typeError(2, "integer"));
    return 0;
}
fn luaCharacterCarryWeightGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.getCurCarriedWeight(checkCharacter(lua)));
    return 1;
}
fn luaCharacterCarryWeightMax(lua: *Lua) i32 {
    lua.pushInteger(cdb.getMaxCarryWeight(checkCharacter(lua)));
    return 1;
}
fn luaCharacterWimpLevelGet(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).wimp_level);
    return 1;
}
fn luaCharacterWimpLevelSet(lua: *Lua) i32 {
    checkCharacter(lua).wimp_level = intCastOrError(lua, c_int, integer(lua, 2), "wimp level");
    return 0;
}
fn luaCharacterSendToWorlds(lua: *Lua) i32 {
    cdb.send_to_worlds(checkCharacter(lua));
    return 0;
}
fn luaCharacterDispelAsh(lua: *Lua) i32 {
    cdb.dispel_ash(checkCharacter(lua));
    return 0;
}
fn luaCharacterRestoreAnnounced(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const announce = lua.toBoolean(2);
    cdb.restoreVitalsAnnounced(ch, announce);
    return 0;
}
fn luaCharacterCureKnockedOut(lua: *Lua) i32 {
    cdb.cureStatusKnockedOutAnnounced(checkCharacter(lua), true);
    return 0;
}
fn luaCharacterAbsorbingGet(lua: *Lua) i32 {
    const t = cdb.char_absorbing_get(checkCharacter(lua));
    if (t) |v| pushCharacter(lua, cdb.char_id_get(v)) else lua.pushNil();
    return 1;
}
fn luaCharacterAbsorbingSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (lua.typeOf(2) == .nil)
        cdb.char_absorbing_set(ch, null)
    else
        cdb.char_absorbing_set(ch, checkCharacterAt(lua, 2));
    return 0;
}
fn luaCharacterAbsorbedByGet(lua: *Lua) i32 {
    const t = cdb.char_absorbed_by_get(checkCharacter(lua));
    if (t) |v| pushCharacter(lua, cdb.char_id_get(v)) else lua.pushNil();
    return 1;
}
fn luaCharacterAbsorbedBySet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (lua.typeOf(2) == .nil)
        cdb.char_absorbed_by_set(ch, null)
    else
        cdb.char_absorbed_by_set(ch, checkCharacterAt(lua, 2));
    return 0;
}
fn luaCharacterMindlinkedGet(lua: *Lua) i32 {
    const t = cdb.char_mindlinked_get(checkCharacter(lua));
    if (t) |v| pushCharacter(lua, cdb.char_id_get(v)) else lua.pushNil();
    return 1;
}
fn luaCharacterMindlinkedSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (lua.typeOf(2) == .nil)
        cdb.char_mindlinked_set(ch, null)
    else
        cdb.char_mindlinked_set(ch, checkCharacterAt(lua, 2));
    return 0;
}
fn luaCharacterLinkerGet(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).linker);
    return 1;
}
fn luaCharacterLinkerSet(lua: *Lua) i32 {
    checkCharacter(lua).linker = intCastOrError(lua, c_int, integer(lua, 2), "linker");
    return 0;
}
fn luaCharacterBlockingGet(lua: *Lua) i32 {
    const t = cdb.char_blocking_get(checkCharacter(lua));
    if (t) |v| pushCharacter(lua, cdb.char_id_get(v)) else lua.pushNil();
    return 1;
}
fn luaCharacterBlockingSet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (lua.typeOf(2) == .nil)
        cdb.char_blocking_set(ch, null)
    else
        cdb.char_blocking_set(ch, checkCharacterAt(lua, 2));
    return 0;
}
fn luaCharacterBlockedByGet(lua: *Lua) i32 {
    const t = cdb.char_blocked_by_get(checkCharacter(lua));
    if (t) |v| pushCharacter(lua, cdb.char_id_get(v)) else lua.pushNil();
    return 1;
}
fn luaCharacterBlockedBySet(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    if (lua.typeOf(2) == .nil)
        cdb.char_blocked_by_set(ch, null)
    else
        cdb.char_blocked_by_set(ch, checkCharacterAt(lua, 2));
    return 0;
}

// Misc display field accessors
fn luaCharacterTimerGet(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).timer);
    return 1;
}
fn luaCharacterHasConnection(lua: *Lua) i32 {
    lua.pushBoolean(checkCharacter(lua).desc != null);
    return 1;
}
fn luaCharacterDefaultPositionGet(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).mob_specials.default_pos);
    return 1;
}
fn luaCharacterEavesdropGet(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).listenroom);
    return 1;
}
fn luaCharacterEavesdropSet(lua: *Lua) i32 {
    checkCharacter(lua).listenroom = intCastOrError(lua, c_int, integer(lua, 2), "listenroom");
    return 0;
}
fn luaCharacterEavesdropDirGet(lua: *Lua) i32 {
    lua.pushInteger(checkCharacter(lua).eavesdir);
    return 1;
}
fn luaCharacterEavesdropDirSet(lua: *Lua) i32 {
    checkCharacter(lua).eavesdir = intCastOrError(lua, c_int, integer(lua, 2), "eavesdir");
    return 0;
}
fn luaCharacterRdisplayClear(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const rdis = ch.rdisplay;
    if (rdis != null and !std.mem.eql(u8, std.mem.span(rdis.?), "Empty")) {
        ch.rdisplay = @constCast(@as([*:0]const u8, "Empty"));
    }
    return 0;
}
fn luaCharacterSlotCount(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_slot_count(checkCharacter(lua)));
    return 1;
}

// ---- CharacterScript handle ----

pub fn pushCharacterScript(lua: *Lua, char_id: i64, script_id: []const u8) void {
    const handle = lua.newUserdata(CharScriptHandle, 0);
    handle.character_id = char_id;
    handle.script = std.mem.zeroes([64:0]u8);
    const len = @min(script_id.len, handle.script.len - 1);
    @memcpy(handle.script[0..len], script_id[0..len]);
    _ = lua.getMetatableRegistry(char_script_metatable);
    lua.setMetatable(-2);
}

fn checkCharScriptHandle(lua: *Lua) *CharScriptHandle {
    return lua.testUserdata(CharScriptHandle, 1, char_script_metatable) catch {
        lua.raiseErrorStr("expected dbat.CharacterScript", .{});
    };
}

fn charScriptCharacter(lua: *Lua, handle: *CharScriptHandle) *cdb.char_data {
    return characterByHandleId(handle.character_id) orelse lua.raiseErrorStr("stale dbat.CharacterScript character", .{});
}

fn charScriptName(handle: *CharScriptHandle) [*:0]const u8 {
    return @ptrCast(&handle.script);
}

fn scriptEventKind(buf: *[192:0]u8, script: []const u8, event: []const u8) ?[:0]u8 {
    const prefix = "script:";
    const total = prefix.len + script.len + 1 + event.len;
    if (total >= buf.len) return null;
    @memcpy(buf[0..prefix.len], prefix);
    @memcpy(buf[prefix.len..][0..script.len], script);
    buf[prefix.len + script.len] = ':';
    @memcpy(buf[prefix.len + script.len + 1 ..][0..event.len], event);
    buf[total] = 0;
    return buf[0..total :0];
}

fn luaCharScriptId(lua: *Lua) i32 {
    _ = lua.pushString(std.mem.span(charScriptName(checkCharScriptHandle(lua))));
    return 1;
}

fn luaCharScriptNumberGet(lua: *Lua) i32 {
    const handle = checkCharScriptHandle(lua);
    lua.pushInteger(cdb.char_script_number_get(charScriptCharacter(lua, handle), charScriptName(handle), string(lua, 2)));
    return 1;
}

fn luaCharScriptNumberSet(lua: *Lua) i32 {
    const handle = checkCharScriptHandle(lua);
    cdb.char_script_number_set(charScriptCharacter(lua, handle), charScriptName(handle), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "script number"));
    return 0;
}

fn luaCharScriptNumberMod(lua: *Lua) i32 {
    const handle = checkCharScriptHandle(lua);
    const ch = charScriptCharacter(lua, handle);
    const name = charScriptName(handle);
    const key = string(lua, 2);
    const delta = intCastOrError(lua, i64, integer(lua, 3), "script number delta");
    const old = cdb.char_script_number_get(ch, name, key);
    cdb.char_script_number_set(ch, name, key, old + delta);
    lua.pushInteger(old + delta);
    return 1;
}

fn luaCharScriptTextGet(lua: *Lua) i32 {
    const handle = checkCharScriptHandle(lua);
    pushCString(lua, cdb.char_script_text_get(charScriptCharacter(lua, handle), charScriptName(handle), string(lua, 2)));
    return 1;
}

fn luaCharScriptTextSet(lua: *Lua) i32 {
    const handle = checkCharScriptHandle(lua);
    cdb.char_script_text_set(charScriptCharacter(lua, handle), charScriptName(handle), string(lua, 2), string(lua, 3));
    return 0;
}

fn luaCharScriptScheduleEvent(lua: *Lua) i32 {
    const handle = checkCharScriptHandle(lua);
    const ch = charScriptCharacter(lua, handle);
    const event_name = string(lua, 2);
    const delay_ms = intCastOrError(lua, i64, integer(lua, 3), "delay_ms");
    const interval_ms: i64 = if (lua.isNoneOrNil(4)) 0 else intCastOrError(lua, i64, integer(lua, 4), "interval_ms");
    const script = std.mem.span(charScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = scriptEventKind(&buf, script, event_name) orelse {
        lua.pushInteger(0);
        return 1;
    };
    _ = eq_cancel_owner(1, cdb.char_id_get(ch), kind.ptr);
    const id = event_schedule_lua_char_update(event_queue_now_ms() + delay_ms, interval_ms, kind.ptr, cdb.char_id_get(ch));
    lua.pushInteger(@intCast(id));
    return 1;
}

fn luaCharScriptCancelEvent(lua: *Lua) i32 {
    const handle = checkCharScriptHandle(lua);
    const ch = charScriptCharacter(lua, handle);
    const event_name = string(lua, 2);
    const script = std.mem.span(charScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = scriptEventKind(&buf, script, event_name) orelse return 0;
    _ = eq_cancel_owner(1, cdb.char_id_get(ch), kind.ptr);
    return 0;
}

fn luaCharScriptEventPending(lua: *Lua) i32 {
    const handle = checkCharScriptHandle(lua);
    const ch = charScriptCharacter(lua, handle);
    const event_name = string(lua, 2);
    const script = std.mem.span(charScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = scriptEventKind(&buf, script, event_name) orelse {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(eq_owner_count(1, cdb.char_id_get(ch), kind.ptr) > 0);
    return 1;
}

fn luaCharScriptEventNextMs(lua: *Lua) i32 {
    const handle = checkCharScriptHandle(lua);
    const ch = charScriptCharacter(lua, handle);
    const event_name = string(lua, 2);
    const script = std.mem.span(charScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = scriptEventKind(&buf, script, event_name) orelse {
        lua.pushInteger(-1);
        return 1;
    };
    lua.pushInteger(eq_owner_next_ms(1, cdb.char_id_get(ch), kind.ptr));
    return 1;
}

// ---- Character-level script entity methods ----

fn luaCharacterScriptAdd(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_script_add(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterScriptRemove(lua: *Lua) i32 {
    const reason: [*:0]const u8 = if (lua.isNoneOrNil(3)) "removed" else string(lua, 3);
    lua.pushBoolean(cdb.char_script_remove(checkCharacter(lua), string(lua, 2), reason));
    return 1;
}

fn luaCharacterScriptHas(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_script_has(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterScript(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const script_id = string(lua, 2);
    if (!cdb.char_script_has(ch, script_id)) {
        lua.pushNil();
        return 1;
    }
    pushCharacterScript(lua, cdb.char_id_get(ch), script_id);
    return 1;
}

fn luaCharacterScripts(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    lua.newTable();
    var maybe_iter = character_api.characterScriptIterator(ch);
    if (maybe_iter) |*iter| {
        var i: zlua.Integer = 1;
        while (iter.next()) |entry| {
            pushCharacterScript(lua, cdb.char_id_get(ch), entry.name);
            lua.setIndex(-2, i);
            i += 1;
        }
    }
    return valueIterator(lua);
}

fn luaCharacterScriptNumberGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_script_number_get(checkCharacter(lua), string(lua, 2), string(lua, 3)));
    return 1;
}

fn luaCharacterScriptNumberSet(lua: *Lua) i32 {
    cdb.char_script_number_set(checkCharacter(lua), string(lua, 2), string(lua, 3), intCastOrError(lua, i64, integer(lua, 4), "script number"));
    return 0;
}

fn luaCharacterScriptTextGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_script_text_get(checkCharacter(lua), string(lua, 2), string(lua, 3)));
    return 1;
}

fn luaCharacterScriptTextSet(lua: *Lua) i32 {
    cdb.char_script_text_set(checkCharacter(lua), string(lua, 2), string(lua, 3), string(lua, 4));
    return 0;
}
