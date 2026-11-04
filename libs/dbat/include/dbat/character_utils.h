#pragma once
#include "consts/autoexit.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "consts/types.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/sex.h"
#include "consts/prefflags.h"
#include "consts/playerflags.h"
#include "consts/adminflags.h"
#include "consts/mobflags.h"

#include "characters.h"
#include "flags.h"
#include "characters.h"
#include "log.h"
#include "races.h"
#include "class.h"
#include "weather.h"

/* Player-only data (player_specials). On an NPC these log the caller with a backtrace; reads then give
 * 0/NULL and writes are ignored. */
bitvector_t *char_get_pref(GameWorld *g, const struct char_data *ch);
int char_get_gauntlet(GameWorld *g, const struct char_data *ch);
void char_set_gauntlet(GameWorld *g, struct char_data *ch, int v);
int char_get_tlevel(GameWorld *g, const struct char_data *ch);
void char_set_tlevel(GameWorld *g, struct char_data *ch, int v);
struct char_data *char_get_carrying(const struct char_data *ch);
void char_set_carrying(GameWorld *g, struct char_data *ch, struct char_data *v);
struct char_data *char_get_carried_by(const struct char_data *ch);
void char_set_carried_by(GameWorld *g, struct char_data *ch, struct char_data *v);
int char_get_racial_pref(const struct char_data *ch);
void char_set_racial_pref(GameWorld *g, struct char_data *ch, int v);
int8_t char_get_conditions(GameWorld *g, const struct char_data *ch, int i);
void char_set_conditions(GameWorld *g, struct char_data *ch, int i, int8_t v);
room_vnum char_get_load_room(GameWorld *g, const struct char_data *ch);
void char_set_load_room(GameWorld *g, struct char_data *ch, room_vnum v);
int char_get_practice_points(GameWorld *g, const struct char_data *ch);
void char_set_practice_points(GameWorld *g, struct char_data *ch, int v);
int char_get_ability_trains(GameWorld *g, const struct char_data *ch);
void char_set_ability_trains(GameWorld *g, struct char_data *ch, int v);
int char_get_trainstr(GameWorld *g, const struct char_data *ch);
void char_set_trainstr(GameWorld *g, struct char_data *ch, int v);
int char_get_trainint(GameWorld *g, const struct char_data *ch);
void char_set_trainint(GameWorld *g, struct char_data *ch, int v);
int char_get_traincon(GameWorld *g, const struct char_data *ch);
void char_set_traincon(GameWorld *g, struct char_data *ch, int v);
int char_get_trainwis(GameWorld *g, const struct char_data *ch);
void char_set_trainwis(GameWorld *g, struct char_data *ch, int v);
int char_get_trainagl(GameWorld *g, const struct char_data *ch);
void char_set_trainagl(GameWorld *g, struct char_data *ch, int v);
int char_get_trainspd(GameWorld *g, const struct char_data *ch);
void char_set_trainspd(GameWorld *g, struct char_data *ch, int v);
int16_t char_get_invis_level(GameWorld *g, const struct char_data *ch);
void char_set_invis_level(GameWorld *g, struct char_data *ch, int16_t v);
int char_get_wimp_level(GameWorld *g, const struct char_data *ch);
void char_set_wimp_level(GameWorld *g, struct char_data *ch, int v);
int8_t char_get_freeze_level(GameWorld *g, const struct char_data *ch);
void char_set_freeze_level(GameWorld *g, struct char_data *ch, int8_t v);
uint8_t char_get_bad_pws(GameWorld *g, const struct char_data *ch);
void char_set_bad_pws(GameWorld *g, struct char_data *ch, uint8_t v);
char *char_get_poofin(GameWorld *g, const struct char_data *ch);
void char_set_poofin(GameWorld *g, struct char_data *ch, char *v);
char *char_get_poofout(GameWorld *g, const struct char_data *ch);
void char_set_poofout(GameWorld *g, struct char_data *ch, char *v);
int char_get_olc_zone(GameWorld *g, const struct char_data *ch);
void char_set_olc_zone(GameWorld *g, struct char_data *ch, zone_vnum v);
struct alias_data *char_get_aliases(GameWorld *g, const struct char_data *ch);
void char_set_aliases(GameWorld *g, struct char_data *ch, struct alias_data *v);
int32_t char_get_last_tell(GameWorld *g, const struct char_data *ch);
void char_set_last_tell(GameWorld *g, struct char_data *ch, int32_t v);
char *char_get_host(GameWorld *g, const struct char_data *ch);
void char_set_host(GameWorld *g, struct char_data *ch, char *v);
struct txt_block *char_get_comm_hist(GameWorld *g, const struct char_data *ch, int i);
void char_set_comm_hist(GameWorld *g, struct char_data *ch, int i, struct txt_block *v);
int char_get_speaking(const struct char_data *ch);
void char_set_speaking(struct char_data *ch, int v);
int char_get_feat_points(GameWorld *g, const struct char_data *ch);
void char_set_feat_points(GameWorld *g, struct char_data *ch, int v);

/* Derived character values that used to be macros. */
int char_get_age_years(GameWorld *g, struct char_data *ch);
int char_can_carry_n(const struct char_data *ch);
const char *char_true_race_name(const struct char_data *ch);
const char *char_sensei_name(const struct char_data *ch);
const char *char_sensei_name_lower(const struct char_data *ch);
const char *char_sensei_style(const struct char_data *ch);
const char *char_class_abbr(const struct char_data *ch);
const char *char_race_abbr(const struct char_data *ch);

bitvector_t *char_get_act(const struct char_data *ch);
bitvector_t *char_get_affected_by(const struct char_data *ch);
bitvector_t *char_get_admflags(const struct char_data *ch);

/* Return the gauntlet highest room for ch */

bool char_is_npc(const struct char_data *ch);
bool char_is_mob(GameWorld *g, const struct char_data *ch);

bool char_mob_flagged(const struct char_data *ch, int flag);
bool char_plr_flagged(const struct char_data *ch, int flag);
bool char_aff_flagged(const struct char_data *ch, int flag);
bool char_prf_flagged(GameWorld *g, const struct char_data *ch, int flag);
bool char_adm_flagged(const struct char_data *ch, int flag);
bool char_body_flagged(const struct char_data *ch, int flag);
bool char_is_affected(const struct char_data *ch, int skill);

bool char_plr_toggle(struct char_data *ch, int flag);
bool char_prf_toggle(GameWorld *g, struct char_data *ch, int flag);
bool char_aff_toggle(struct char_data *ch, int flag);
bool char_mob_toggle(struct char_data *ch, int flag);

/* Flag setters. PLR bits are ignored on NPCs, and MOB bits on PCs (except MOB_ISNPC);
 * both kinds share char_data::act. */
void char_plr_set(struct char_data *ch, int flag, bool on);
void char_mob_set(struct char_data *ch, int flag, bool on);
void char_aff_set(struct char_data *ch, int flag, bool on);
void char_prf_set(GameWorld *g, struct char_data *ch, int flag, bool on);
void char_body_set(struct char_data *ch, int flag, bool on);

/* new define for quick check */
bool char_is_dead(const struct char_data *ch);

room_rnum char_get_in_room(const struct char_data *ch);
void char_set_in_room(struct char_data *ch, room_rnum v);
zone_vnum char_in_zone(GameWorld *g, const struct char_data *ch);
room_rnum char_get_was_in_room(const struct char_data *ch);
void char_set_was_in_room(struct char_data *ch, room_rnum v);

char *char_get_pc_name(const struct char_data *ch);
void char_set_pc_name(struct char_data *ch, char *v);
char *char_get_name(const struct char_data *ch);
const char *char_get_title(const struct char_data *ch);
int char_get_starphase(const struct char_data *ch);
void char_set_starphase(struct char_data *ch, int v);
int char_get_mimic(const struct char_data *ch);
char *char_get_voice(const struct char_data *ch);
void char_set_voice(struct char_data *ch, char *v);
int char_get_transclass(const struct char_data *ch);
void char_set_transclass(struct char_data *ch, int v);
const char *char_get_feature(const struct char_data *ch);
void char_set_feature(struct char_data *ch, const char *v);
const char *char_get_user(const struct char_data *ch);
char *char_get_loguser(const struct char_data *ch);
void char_set_loguser(struct char_data *ch, char *v);
int char_get_admlevel(const struct char_data *ch);
void char_set_admlevel(struct char_data *ch, int v);
int char_get_class_level(const struct char_data *ch);
void char_set_class_level(struct char_data *ch, int v);
int char_get_level_adj(const struct char_data *ch);
void char_set_level_adj(struct char_data *ch, int v);
int char_get_race_level(const struct char_data *ch);
void char_set_race_level(struct char_data *ch, int v);
int char_get_level(const struct char_data *ch);
int char_get_pfilepos(const struct char_data *ch);
void char_set_pfilepos(struct char_data *ch, int v);

int char_get_class(const struct char_data *ch);
int char_get_class_ranks(const struct char_data *ch, int whichclass);
int char_get_race(const struct char_data *ch);
int8_t char_get_hairl(const struct char_data *ch);
void char_set_hairl(struct char_data *ch, int8_t v);
int8_t char_get_hairc(const struct char_data *ch);
void char_set_hairc(struct char_data *ch, int8_t v);
int8_t char_get_hairs(const struct char_data *ch);
void char_set_hairs(struct char_data *ch, int8_t v);
int8_t char_get_skin(const struct char_data *ch);
void char_set_skin(struct char_data *ch, int8_t v);
int8_t char_get_eye(const struct char_data *ch);
void char_set_eye(struct char_data *ch, int8_t v);
int8_t char_get_distfea(const struct char_data *ch);
void char_set_distfea(struct char_data *ch, int8_t v);
room_vnum char_get_hometown(const struct char_data *ch);
void char_set_hometown(struct char_data *ch, room_vnum v);
uint8_t char_get_weight(const struct char_data *ch);
void char_set_weight(struct char_data *ch, uint8_t v);
uint8_t char_get_height(const struct char_data *ch);
void char_set_height(struct char_data *ch, uint8_t v);
int char_get_pc_height(GameWorld *g, struct char_data *ch);
int char_get_pc_weight(GameWorld *g, struct char_data *ch);
int8_t char_get_sex(const struct char_data *ch);
void char_set_sex(struct char_data *ch, int8_t v);
int char_get_rp(const struct char_data *ch);
void char_set_rp(struct char_data *ch, int v);
int char_get_rbank(const struct char_data *ch);
void char_set_rbank(struct char_data *ch, int v);
int char_get_trp(const struct char_data *ch);
void char_set_trp(struct char_data *ch, int v);
int64_t char_get_suppression(const struct char_data *ch);
void char_set_suppression(struct char_data *ch, int64_t v);
int64_t char_get_suppressed(const struct char_data *ch);
void char_set_suppressed(struct char_data *ch, int64_t v);
const char *char_get_rdisplay(const struct char_data *ch);
void char_set_rdisplay(struct char_data *ch, const char *v);

int8_t char_get_aff_abils_str(const struct char_data *ch);
void char_set_aff_abils_str(struct char_data *ch, int8_t v);
/*
 * We could define GET_ADD to be ((GET_STR(ch) > 18) ?
 *                                ((GET_STR(ch) - 18) * 10) : 0)
 * but it's better to leave it undefined and fix the places that call
 * GET_ADD to use the new semantics for abilities.
 *                               - Elie Rosenblum 13/Dec/2003
 */
/* The old define: */
/* #define GET_ADD(ch)     ((ch)->aff_abils.str_add) */
int8_t char_get_aff_abils_dex(const struct char_data *ch);
void char_set_aff_abils_dex(struct char_data *ch, int8_t v);
int8_t char_get_aff_abils_intel(const struct char_data *ch);
void char_set_aff_abils_intel(struct char_data *ch, int8_t v);
int8_t char_get_aff_abils_wis(const struct char_data *ch);
void char_set_aff_abils_wis(struct char_data *ch, int8_t v);
int8_t char_get_aff_abils_con(const struct char_data *ch);
void char_set_aff_abils_con(struct char_data *ch, int8_t v);
int8_t char_get_aff_abils_cha(const struct char_data *ch);
void char_set_aff_abils_cha(struct char_data *ch, int8_t v);

double char_get_mutboost(GameWorld *g, struct char_data *ch);
double char_get_speedi(GameWorld *g, struct char_data *ch);
double char_get_speedcalc(GameWorld *g, struct char_data *ch);
double char_get_speedbonus(GameWorld *g, struct char_data *ch);
double char_get_speedvar(GameWorld *g, struct char_data *ch);
double char_get_speedvem(GameWorld *g, struct char_data *ch);
long char_get_speedint(GameWorld *g, struct char_data *ch);
bool char_is_weighted(GameWorld *g, struct char_data *ch);

int64_t char_get_exp(const struct char_data *ch);
void char_set_exp(struct char_data *ch, int64_t v);
/*
 * Changed GET_AC to GET_ARMOR so that code with GET_AC will need to be
 * looked at to see if it needs to change before being converted to use
 * GET_ARMOR
 */
bool char_is_spoiled(const struct char_data *ch);
int char_get_death_type(const struct char_data *ch);
void char_set_death_type(struct char_data *ch, int v);
int char_get_sleeptime(const struct char_data *ch);
void char_set_sleeptime(struct char_data *ch, int v);
int char_get_foodr(const struct char_data *ch);
void char_set_foodr(struct char_data *ch, int v);
int char_get_altitude(const struct char_data *ch);
void char_set_altitude(struct char_data *ch, int v);
int64_t char_get_charge(const struct char_data *ch);
void char_set_charge(struct char_data *ch, int64_t v);
int64_t char_get_chargeto(const struct char_data *ch);
void char_set_chargeto(struct char_data *ch, int64_t v);
int char_get_armor(const struct char_data *ch);
void char_set_armor(struct char_data *ch, int v);
int64_t char_get_ki(const struct char_data *ch);
void char_set_ki(struct char_data *ch, int64_t v);
int64_t char_get_max_ki(const struct char_data *ch);
void char_set_max_ki(struct char_data *ch, int64_t v);
room_vnum char_get_droom(const struct char_data *ch);
void char_set_droom(struct char_data *ch, room_vnum v);
int char_get_overf(const struct char_data *ch);
void char_set_overf(struct char_data *ch, int v);
int char_get_spam(const struct char_data *ch);
void char_set_spam(struct char_data *ch, int v);
time_t char_get_lastpl(const struct char_data *ch);
void char_set_lastpl(struct char_data *ch, time_t v);
time_t char_get_deathtime(const struct char_data *ch);
void char_set_deathtime(struct char_data *ch, time_t v);
time_t char_get_rewtime(const struct char_data *ch);
void char_set_rewtime(struct char_data *ch, time_t v);
int char_get_dcount(const struct char_data *ch);
void char_set_dcount(struct char_data *ch, int v);
time_t char_get_lboard(const struct char_data *ch, int i);
void char_set_lboard(struct char_data *ch, int i, time_t v);
// why is this i-1? Because whoever wrote it didn't know how C arrays work.
// We'll be replacing the limb system anyways so fuck this macro.
int char_get_limb_condition(const struct char_data *ch, int i);
void char_set_limb_condition(struct char_data *ch, int i, int v);
int char_get_powerattack(const struct char_data *ch);
void char_set_powerattack(struct char_data *ch, int v);
int char_get_bonuses(const struct char_data *ch, int i);
void char_set_bonuses(struct char_data *ch, int i, int v);
int char_get_transcost(const struct char_data *ch, int i);
void char_set_transcost(struct char_data *ch, int i, int v);
int char_get_ccpoints(const struct char_data *ch);
void char_set_ccpoints(struct char_data *ch, int v);
int char_get_negcount(const struct char_data *ch);
void char_set_negcount(struct char_data *ch, int v);
int char_get_genome(const struct char_data *ch, int i);
void char_set_genome(struct char_data *ch, int i, int v);
bool char_has_genome(const struct char_data *ch, int i);
int char_get_combo(const struct char_data *ch);
void char_set_combo(struct char_data *ch, int v);
int char_get_lastattack(const struct char_data *ch);
void char_set_lastattack(struct char_data *ch, int v);
int char_get_combhits(const struct char_data *ch);
void char_set_combhits(struct char_data *ch, int v);
int char_get_aura(const struct char_data *ch);
void char_set_aura(struct char_data *ch, int v);
room_vnum char_get_radar1(const struct char_data *ch);
void char_set_radar1(struct char_data *ch, room_vnum v);
room_vnum char_get_radar2(const struct char_data *ch);
void char_set_radar2(struct char_data *ch, room_vnum v);
room_vnum char_get_radar3(const struct char_data *ch);
void char_set_radar3(struct char_data *ch, room_vnum v);
int char_get_ping(const struct char_data *ch);
void char_set_ping(struct char_data *ch, int v);
int char_get_skill_slots(const struct char_data *ch);
void char_set_skill_slots(struct char_data *ch, int v);
int char_get_tail_growth(const struct char_data *ch);
void char_set_tail_growth(struct char_data *ch, int v);
int char_get_rage_meter(const struct char_data *ch);
void char_set_rage_meter(struct char_data *ch, int v);
int char_get_personality(const struct char_data *ch);
void char_set_personality(struct char_data *ch, int v);
int char_get_combine(const struct char_data *ch);
void char_set_combine(struct char_data *ch, int v);
int char_get_preference(const struct char_data *ch);
void char_set_preference(struct char_data *ch, int v);
int char_get_relax_count(const struct char_data *ch);
void char_set_relax_count(struct char_data *ch, int v);
int char_get_blesslvl(const struct char_data *ch);
void char_set_blesslvl(struct char_data *ch, int v);
int char_get_asb(const struct char_data *ch);
void char_set_asb(struct char_data *ch, int v);
int char_get_regen(const struct char_data *ch);
void char_set_regen(struct char_data *ch, int v);
double char_get_blessbonus(GameWorld *g, struct char_data *ch);
double char_get_poself(struct char_data *ch);
double char_get_posebonus(GameWorld *g, struct char_data *ch);
double char_get_lifebonus(GameWorld *g, struct char_data *ch);
double char_get_lifebonuses(GameWorld *g, struct char_data *ch);
int char_get_lifeperc(const struct char_data *ch);
void char_set_lifeperc(struct char_data *ch, int v);
int char_get_speedboost(const struct char_data *ch);
void char_set_speedboost(struct char_data *ch, int v);
int char_get_backstabcool(const struct char_data *ch);
void char_set_backstabcool(struct char_data *ch, int v);
int char_get_con_cooldown(const struct char_data *ch);
void char_set_con_cooldown(struct char_data *ch, int v);
int64_t char_get_barrier(const struct char_data *ch);
void char_set_barrier(struct char_data *ch, int64_t v);
int char_get_gold(const struct char_data *ch);
void char_set_gold(struct char_data *ch, int v);
int char_get_kaioken(const struct char_data *ch);
void char_set_kaioken(struct char_data *ch, int v);
int char_get_boosts(const struct char_data *ch);
void char_set_boosts(struct char_data *ch, int v);
int char_get_majinize(const struct char_data *ch);
void char_set_majinize(struct char_data *ch, int v);
int64_t char_get_majinizer(const struct char_data *ch);
void char_set_majinizer(struct char_data *ch, int64_t v);
short char_get_fury(const struct char_data *ch);
void char_set_fury(struct char_data *ch, short v);
int char_get_upgrade(const struct char_data *ch);
void char_set_upgrade(struct char_data *ch, int v);
int char_get_forgeting(const struct char_data *ch);
void char_set_forgeting(struct char_data *ch, int v);
int char_get_forgetcount(const struct char_data *ch);
void char_set_forgetcount(struct char_data *ch, int v);
int char_get_bank_gold(const struct char_data *ch);
void char_set_bank_gold(struct char_data *ch, int v);
int char_get_accuracy(const struct char_data *ch);
void char_set_accuracy(struct char_data *ch, int v);
int char_get_fishstate(const struct char_data *ch);
void char_set_fishstate(struct char_data *ch, int v);
int char_get_accuracy_mod(const struct char_data *ch);
void char_set_accuracy_mod(struct char_data *ch, int v);
int64_t char_get_moltexp(const struct char_data *ch);
void char_set_moltexp(struct char_data *ch, int64_t v);
int char_get_moltlevel(const struct char_data *ch);
void char_set_moltlevel(struct char_data *ch, int v);
int char_get_con_sdcooldown(const struct char_data *ch);
void char_set_con_sdcooldown(struct char_data *ch, int v);
int char_get_ingestLearned(const struct char_data *ch);
void char_set_ingestLearned(struct char_data *ch, int v);
int8_t char_get_position(const struct char_data *ch);
void char_set_position(struct char_data *ch, int8_t v);
int32_t char_get_idnum(const struct char_data *ch);
void char_set_idnum(struct char_data *ch, int32_t v);
int32_t char_get_id(const struct char_data *ch);
void char_set_id(struct char_data *ch, int32_t v);
int char_get_carry_weight(const struct char_data *ch);
void char_set_carry_weight(struct char_data *ch, int v);
int8_t char_get_carry_items(const struct char_data *ch);
void char_set_carry_items(struct char_data *ch, int8_t v);
struct char_data *char_get_fighting(const struct char_data *ch);
void char_set_fighting(struct char_data *ch, struct char_data *v);
int char_get_combatexpertise(const struct char_data *ch);
void char_set_combatexpertise(struct char_data *ch, int v);
int char_get_alignment(const struct char_data *ch);
void char_set_alignment(struct char_data *ch, int v);
struct obj_data *char_get_sits(const struct char_data *ch);
void char_set_sits(struct char_data *ch, struct obj_data *v);
struct char_data *char_get_mindlink(const struct char_data *ch);
void char_set_mindlink(struct char_data *ch, struct char_data *v);
int char_get_linker(const struct char_data *ch);
void char_set_linker(struct char_data *ch, int v);
int char_get_lasthit(const struct char_data *ch);
void char_set_lasthit(struct char_data *ch, int v);
struct char_data *char_get_drag(const struct char_data *ch);
void char_set_drag(struct char_data *ch, struct char_data *v);
struct char_data *char_get_dragged(const struct char_data *ch);
void char_set_dragged(struct char_data *ch, struct char_data *v);
struct char_data *char_get_grappling(const struct char_data *ch);
void char_set_grappling(struct char_data *ch, struct char_data *v);
struct char_data *char_get_grappled(const struct char_data *ch);
void char_set_grappled(struct char_data *ch, struct char_data *v);
int char_get_grap(const struct char_data *ch);
void char_set_grap(struct char_data *ch, int v);
struct char_data *char_get_original(const struct char_data *ch);
void char_set_original(struct char_data *ch, struct char_data *v);
short char_get_clones(const struct char_data *ch);
void char_set_clones(struct char_data *ch, short v);
struct char_data *char_get_defender(const struct char_data *ch);
void char_set_defender(struct char_data *ch, struct char_data *v);
struct char_data *char_get_defending(const struct char_data *ch);
void char_set_defending(struct char_data *ch, struct char_data *v);
struct char_data *char_get_blocks(const struct char_data *ch);
void char_set_blocks(struct char_data *ch, struct char_data *v);
struct char_data *char_get_blocked(const struct char_data *ch);
void char_set_blocked(struct char_data *ch, struct char_data *v);
struct char_data *char_get_absorbing(const struct char_data *ch);
void char_set_absorbing(struct char_data *ch, struct char_data *v);
struct char_data *char_get_absorbby(const struct char_data *ch);
void char_set_absorbby(struct char_data *ch, struct char_data *v);
room_vnum char_get_listenroom(const struct char_data *ch);
void char_set_listenroom(struct char_data *ch, room_vnum v);
int char_get_eavesdir(const struct char_data *ch);
void char_set_eavesdir(struct char_data *ch, int v);
int char_get_absorbs(const struct char_data *ch);
void char_set_absorbs(struct char_data *ch, int v);
time_t char_get_lastint(const struct char_data *ch);
void char_set_lastint(struct char_data *ch, time_t v);

int8_t char_get_skills_mod(const struct char_data *ch, int i);
void char_set_skills_mod(struct char_data *ch, int i, int8_t v);
int8_t char_get_skills_perf(const struct char_data *ch, int i);
void char_set_skills_perf(struct char_data *ch, int i, int8_t v);
int8_t char_get_skills_base(const struct char_data *ch, int i);
void char_set_skills_base(struct char_data *ch, int i, int8_t v);
int char_get_skill(const struct char_data *ch, int i);
bitvector_t *char_get_bodyparts(const struct char_data *ch);

struct obj_data *char_get_equipment(const struct char_data *ch, int i);
void char_set_equipment(struct char_data *ch, int i, struct obj_data *v);

spec_proc_func char_get_mob_spec(GameWorld *g, const struct char_data *ch);
mob_rnum char_get_nr(const struct char_data *ch);
void char_set_nr(struct char_data *ch, mob_rnum v);
mob_vnum char_get_mob_vnum(GameWorld *g, const struct char_data *mob);

int8_t char_get_mob_specials_default_pos(const struct char_data *ch);
void char_set_mob_specials_default_pos(struct char_data *ch, int8_t v);
memory_rec *char_get_mob_specials_memory(const struct char_data *ch);
void char_set_mob_specials_memory(struct char_data *ch, memory_rec *v);
int char_get_cooldown(const struct char_data *ch);
void char_set_cooldown(struct char_data *ch, int v);

/* STRENGTH_APPLY_INDEX is no longer needed with the death of GET_ADD */
/* #define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch) ==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        ) */

bool char_is_awake(const struct char_data *ch);
bool char_can_see_in_dark(GameWorld *g, const struct char_data *ch);

bool char_is_good(const struct char_data *ch);
bool char_is_evil(const struct char_data *ch);
bool char_is_neutral(const struct char_data *ch);

bool char_in_arena(GameWorld *g, const struct char_data *ch);
int char_get_arenawatch(const struct char_data *ch);
void char_set_arenawatch(struct char_data *ch, int v);

/* These three deprecated. */
/* New, preferred macro. */
int char_get_wait(const struct char_data *ch);
void char_set_wait(struct char_data *ch, int v);

/* Can subject see character "obj"? */
bool char_can_see(GameWorld *g, const struct char_data *sub, const struct char_data *obj);

/* End of CAN_SEE */

/* Is anyone carrying this object and if so, are they visible? */

bool char_can_see_obj(GameWorld *g, const struct char_data *sub, const struct obj_data *obj);

bool char_can_get_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj);

const char *char_pers(GameWorld *g, struct char_data *ch, struct char_data *vict);

const char *obj_shortdesc_for(GameWorld *g, const struct obj_data *obj, const struct char_data *vict);

const char *obj_fname_for(GameWorld *g, const struct obj_data *obj, const struct char_data *vict);

struct room_direction_data *char_get_exit(GameWorld *g, const struct char_data *ch, int door);
void char_set_exit(GameWorld *g, struct char_data *ch, int door, struct room_direction_data *v);
struct room_direction_data *char_get_exit2(GameWorld *g, const struct char_data *ch, int door);

bool char_can_go(GameWorld *g, const struct char_data *ch, int door);

bool char_is_roshi(const struct char_data *ch);
bool char_is_piccolo(const struct char_data *ch);
bool char_is_krane(const struct char_data *ch);
bool char_is_nail(const struct char_data *ch);
bool char_is_bardock(const struct char_data *ch);
bool char_is_ginyu(const struct char_data *ch);
bool char_is_frieza(const struct char_data *ch);
bool char_is_tapion(const struct char_data *ch);
bool char_is_andsix(const struct char_data *ch);
bool char_is_dabura(const struct char_data *ch);
bool char_is_kabito(const struct char_data *ch);
bool char_is_jinto(const struct char_data *ch);
bool char_is_tsuna(const struct char_data *ch);
bool char_is_kurzak(const struct char_data *ch);

bool char_is_assassin(const struct char_data *ch);
bool char_is_blackguard(const struct char_data *ch);
bool char_is_dragon_disciple(const struct char_data *ch);
bool char_is_duelist(const struct char_data *ch);
bool char_is_dwarven_defender(const struct char_data *ch);
bool char_is_eldritch_knight(const struct char_data *ch);
bool char_is_hierophant(const struct char_data *ch);
bool char_is_horizon_walker(const struct char_data *ch);
bool char_is_loremaster(const struct char_data *ch);
bool char_is_mystic_theurge(const struct char_data *ch);
bool char_is_shadowdancer(const struct char_data *ch);
bool char_is_thaumaturgist(const struct char_data *ch);

int char_gold_carry(const struct char_data *ch);
bool char_is_shadow_dragon1(GameWorld *g, const struct char_data *ch);
bool char_is_shadow_dragon2(GameWorld *g, const struct char_data *ch);
bool char_is_shadow_dragon3(GameWorld *g, const struct char_data *ch);
bool char_is_shadow_dragon4(GameWorld *g, const struct char_data *ch);
bool char_is_shadow_dragon5(GameWorld *g, const struct char_data *ch);
bool char_is_shadow_dragon6(GameWorld *g, const struct char_data *ch);
bool char_is_shadow_dragon7(GameWorld *g, const struct char_data *ch);
bool char_can_grand_master(const struct char_data *ch);
bool char_is_humanoid(const struct char_data *ch);
bool char_is_restricted_race(const struct char_data *ch);
bool char_is_cheap_race(const struct char_data *ch);
bool char_is_nonptrans(const struct char_data *ch);
bool char_is_transformed(const struct char_data *ch);
#define BIRTH_PHASE (g->time_info.day <= 15)
#define LIFE_PHASE  (!BIRTH_PHASE && g->time_info.day <= 22)
#define DEATH_PHASE (!BIRTH_PHASE && !LIFE_PHASE)
bool char_moon_ok(GameWorld *g, const struct char_data *ch);
#define MOON_TIME (g->time_info.hours >= 21 || g->time_info.hours <= 4)
#define MOON_DATE (g->time_info.day == 19 || g->time_info.day == 20 || g->time_info.day == 21)
bool MOON_TIMECHECK(GameWorld *g);
bool char_ether_stream(GameWorld *g, const struct char_data *ch);
bool char_has_moon(GameWorld *g, const struct char_data *ch);
bool char_has_arms(const struct char_data *ch);

bool char_is_human(const struct char_data *ch);
bool char_is_saiyan(const struct char_data *ch);
bool char_is_icer(const struct char_data *ch);
bool char_is_konatsu(const struct char_data *ch);
bool char_is_namek(const struct char_data *ch);
bool char_is_mutant(const struct char_data *ch);
bool char_is_kanassan(const struct char_data *ch);
bool char_is_halfbreed(const struct char_data *ch);
bool char_is_bio(const struct char_data *ch);
bool char_is_android(const struct char_data *ch);
bool char_is_demon(const struct char_data *ch);
bool char_is_majin(const struct char_data *ch);
bool char_is_kai(const struct char_data *ch);
bool char_is_truffle(const struct char_data *ch);
bool char_is_hoshijin(const struct char_data *ch);
bool char_is_animal(const struct char_data *ch);
bool char_is_saiba(const struct char_data *ch);
bool char_is_serpent(const struct char_data *ch);
bool char_is_ogre(const struct char_data *ch);
bool char_is_yardratian(const struct char_data *ch);
bool char_is_arlian(const struct char_data *ch);
bool char_is_dragon(const struct char_data *ch);
bool char_is_mechanical(const struct char_data *ch);

bool char_is_female(const struct char_data *ch);

bool char_is_outside(GameWorld *g, const struct char_data *ch);

const char *char_hshr(const struct char_data *ch);
const char *char_hssh(const struct char_data *ch);
const char *char_hmhr(const struct char_data *ch);
const char *char_mafe(const struct char_data *ch);

/* returns the number of spells per slot */

int roll_stats(GameWorld *g, struct char_data *ch, int type, int bonus);
const char *juggleRaceName(char_data *ch, bool capitalized);
void restore_by(GameWorld *g, char_data *ch, char_data *healer);
void restore(GameWorld *g, char_data *ch, bool announce);
void resurrect(GameWorld *g, char_data *ch, int mode);
void ghostify(GameWorld *g, char_data *ch);
void teleport_to(GameWorld *g, char_data *ch, IDXTYPE rnum);
bool in_past(GameWorld *g, char_data *ch);
bool is_newbie(char_data *ch);
bool in_northran(GameWorld *g, char_data *ch);
int64_t calc_soft_cap(char_data *ch);
bool is_soft_cap(char_data *ch, int64_t type, long double mult);
bool calcGravCost(GameWorld *g, char_data *ch, int64_t num);

int64_t getCurHealth(GameWorld *g, char_data *ch);
int64_t getMaxHealth(GameWorld *g, char_data *ch);
double getCurHealthPercent(GameWorld *g, char_data *ch);
int64_t getPercentOfMaxHealth(GameWorld *g, char_data *ch, double amt);
bool isFullHealth(char_data *ch);
int64_t setCurHealth(char_data *ch, int64_t amt);
int64_t setCurHealthPercent(GameWorld *g, char_data *ch, double amt);
int64_t incCurHealth(GameWorld *g, char_data *ch, int64_t amt, bool limit_max);
int64_t decCurHealth(GameWorld *g, char_data *ch, int64_t amt, int64_t floor);
int64_t incCurHealthPercent(GameWorld *g, char_data *ch, double amt, bool limit_max);
int64_t decCurHealthPercent(GameWorld *g, char_data *ch, double amt, int64_t floor);
void restoreHealth(char_data *ch, bool announce);

int64_t getMaxPL(GameWorld *g, char_data *ch);
int64_t getMaxPLTrans(GameWorld *g, char_data *ch);
int64_t getCurPL(GameWorld *g, char_data *ch);
int64_t getBasePL(char_data *ch);
int64_t getEffBasePL(char_data *ch);

int64_t getCurKI(GameWorld *g, char_data *ch);
int64_t getMaxKI(GameWorld *g, char_data *ch);
int64_t getBaseKI(char_data *ch);
int64_t getEffBaseKI(char_data *ch);
double getCurKIPercent(GameWorld *g, char_data *ch);
int64_t getPercentOfMaxKI(GameWorld *g, char_data *ch, double amt);
bool isFullKI(char_data *ch);
int64_t setCurKIPercent(GameWorld *g, char_data *ch, double amt);
int64_t incCurKI(GameWorld *g, char_data *ch, int64_t amt, bool limit_max);
int64_t decCurKI(GameWorld *g, char_data *ch, int64_t amt, int64_t floor);
int64_t incCurKIPercent(GameWorld *g, char_data *ch, double amt, bool limit_max);
int64_t decCurKIPercent(GameWorld *g, char_data *ch, double amt, int64_t floor);
void restoreKI(char_data *ch, bool announce);

int64_t getCurST(GameWorld *g, char_data *ch);
int64_t getMaxST(GameWorld *g, char_data *ch);
int64_t getBaseST(char_data *ch);
int64_t getEffBaseST(char_data *ch);
double getCurSTPercent(GameWorld *g, char_data *ch);
bool isFullST(char_data *ch);
int64_t setCurSTPercent(GameWorld *g, char_data *ch, double amt);
int64_t incCurST(GameWorld *g, char_data *ch, int64_t amt, bool limit_max);
int64_t decCurST(GameWorld *g, char_data *ch, int64_t amt, int64_t floor);
int64_t incCurSTPercent(GameWorld *g, char_data *ch, double amt, bool limit_max);
int64_t decCurSTPercent(GameWorld *g, char_data *ch, double amt, int64_t floor);
void restoreST(char_data *ch, bool announce);

int64_t getCurLF(GameWorld *g, char_data *ch);
int64_t getMaxLF(GameWorld *g, char_data *ch);
double getCurLFPercent(char_data *ch);
bool isFullLF(char_data *ch);
int64_t incCurLF(GameWorld *g, char_data *ch, int64_t amt, bool limit_max);
int64_t decCurLF(GameWorld *g, char_data *ch, int64_t amt, int64_t floor);
int64_t incCurLFPercent(GameWorld *g, char_data *ch, double amt, bool limit_max);
int64_t decCurLFPercent(GameWorld *g, char_data *ch, double amt, int64_t floor);
void restoreLF(char_data *ch, bool announce);

bool isFullVitals(char_data *ch);
void restoreVitals(char_data *ch, bool announce);

int64_t gainBasePL(char_data *ch, int64_t amt, bool trans_mult);
int64_t gainBaseKI(char_data *ch, int64_t amt, bool trans_mult);
int64_t gainBaseST(char_data *ch, int64_t amt, bool trans_mult);

int64_t loseBasePL(char_data *ch, int64_t amt, bool trans_mult);

int64_t gainBasePLPercent(char_data *ch, double amt, bool trans_mult);
void gainBaseAllPercent(char_data *ch, double amt, bool trans_mult);

int64_t loseBasePLPercent(char_data *ch, double amt, bool trans_mult);

void cureStatusKnockedOut(GameWorld *g, char_data *ch, bool announce);
void setStatusKnockedOut(char_data *ch);

int64_t getMaxCarryWeight(GameWorld *g, char_data *ch);
int64_t getCurCarriedWeight(char_data *ch);

int64_t getEffMaxPL(GameWorld *g, char_data *ch);
bool isWeightedPL(GameWorld *g, char_data *ch);

void apply_kaioken(GameWorld *g, char_data *ch, int times, bool announce);
void remove_kaioken(GameWorld *g, char_data *ch, int8_t announce);

void dispel_ash(GameWorld *g, struct char_data *ch);
bool has_group(struct char_data *ch);
const char *report_party_health(GameWorld *g, struct char_data *ch);
bool sec_roll_check(GameWorld *g, struct char_data *ch);
int get_measure(struct char_data *ch, int height, int weight);
int trans_req(struct char_data *ch, int trans);
const char *disp_align(struct char_data *ch);
void senseCreate(struct char_data *ch);
int read_sense_memory(GameWorld *g, struct char_data *ch, struct char_data *vict);
void sense_memory_write(GameWorld *g, struct char_data *ch, struct char_data *vict);
bool roll_pursue(GameWorld *g, struct char_data *ch, struct char_data *vict);
void reveal_hiding(GameWorld *g, struct char_data *ch, int type);
bool block_calc(GameWorld *g, struct char_data *ch);
int64_t molt_threshold(struct char_data *ch);
void handle_evolution(GameWorld *g, struct char_data *ch, int64_t dmg);
void demon_refill_lf(GameWorld *g, struct char_data *ch, int64_t num);
void mob_talk(GameWorld *g, struct char_data *ch, const char *speech);
bool is_sparring(struct char_data *ch);
const char *introd_calc(struct char_data *ch);
int64_t gear_exp(GameWorld *g, struct char_data *ch, int64_t exp);
void purge_homing(GameWorld *g, struct char_data *ch);
void improve_skill(GameWorld *g, struct char_data *ch, int skill, int num);
void stop_follower(GameWorld *g, struct char_data *ch);
void die_follower(GameWorld *g, struct char_data *ch);
void add_follower(GameWorld *g, struct char_data *ch, struct char_data *leader);
bool circle_follow(struct char_data *ch, struct char_data *victim);

void update_pos(GameWorld *g, struct char_data *victim);

bool perform_get_from_room(GameWorld *g, struct char_data *ch, struct obj_data *obj);
void perform_wear(GameWorld *g, struct char_data *ch, struct obj_data *obj, int where);
void perform_remove(GameWorld *g, struct char_data *ch, int pos);

bool can_grav(GameWorld *g, struct char_data *ch);
bool check_skill(GameWorld *g, struct char_data *ch, int skill);
bool check_points(GameWorld *g, struct char_data *ch, int64_t ki, int64_t st);
void pcost(GameWorld *g, struct char_data *ch, double ki, int64_t st);
bool limb_ok(GameWorld *g, struct char_data *ch, int type);
int64_t physical_cost(GameWorld *g, struct char_data *ch, int skill);

int init_skill(GameWorld *g, struct char_data *ch, int snum);
int chance_to_hit(GameWorld *g, struct char_data *ch);
int handle_speed(GameWorld *g, struct char_data *ch, struct char_data *vict);
struct time_info_data *age(GameWorld *g, struct char_data *ch);
const char *get_i_name(struct char_data *ch, struct char_data *vict);

void assign_affect(GameWorld *g, struct char_data *ch, int aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis, int spd);
bool know_skill(GameWorld *g, struct char_data *ch, int skill);

size_t send_to_char(GameWorld *g, struct char_data *ch, const char *messg, ...) __attribute__((format(printf, 3, 4)));
void admin_set(GameWorld *g, struct char_data *ch, int value);
char *sense_location(GameWorld *g, struct char_data *ch);
void null_affect(GameWorld *g, struct char_data *ch, int aff_flag);
bool is_affected(struct char_data *ch, int aff_flag);
void remove_affect(GameWorld *g, struct char_data *ch, int aff_flag);
bool planet_check(GameWorld *g, struct char_data *ch, struct char_data *vict);
/* Player autoexit levels: used as an index to exitlevels           */

int char_exit_level(GameWorld *g, const struct char_data *ch);

int char_get_bank_interest(const struct char_data *ch);

void char_lose_tail(GameWorld *g, char_data *ch);
bool char_has_tail(char_data *ch);
void char_gain_tail(GameWorld *g, char_data *ch, bool announce);

room_vnum sensei_start_room(int s_id);
room_vnum sensei_location_id(int s_id);

int race_get_size(int r_id);
bool race_is_playable(int r_id);
bool race_is_people(int r_id);
bool can_kill(GameWorld *g, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int num);

/* Raw fields, used by OLC and the mob files (formerly genmob.h macros). */
char *char_get_alias(const struct char_data *ch);
void char_set_alias(struct char_data *ch, char *v);
char *char_get_sdesc(const struct char_data *ch);
void char_set_sdesc(struct char_data *ch, char *v);
char *char_get_ldesc(const struct char_data *ch);
void char_set_ldesc(struct char_data *ch, char *v);
char *char_get_ddesc(const struct char_data *ch);
void char_set_ddesc(struct char_data *ch, char *v);
int8_t char_get_mob_specials_damnodice(const struct char_data *ch);
void char_set_mob_specials_damnodice(struct char_data *ch, int8_t v);
int8_t char_get_mob_specials_damsizedice(const struct char_data *ch);
void char_set_mob_specials_damsizedice(struct char_data *ch, int8_t v);
int8_t char_get_mob_specials_attack_type(const struct char_data *ch);
void char_set_mob_specials_attack_type(struct char_data *ch, int8_t v);

/* Field accessors */
struct affected_type *char_get_affected(const struct char_data *ch);
void char_set_affected(struct char_data *ch, struct affected_type *v);
int char_get_aggtimer(const struct char_data *ch);
void char_set_aggtimer(struct char_data *ch, int v);
void char_set_baseki(struct char_data *ch, int64_t v);
void char_set_basepl(struct char_data *ch, int64_t v);
void char_set_basest(struct char_data *ch, int64_t v);
struct obj_data *char_get_inventory(const struct char_data *ch);
void char_set_inventory(struct char_data *ch, struct obj_data *v);
void char_set_class(struct char_data *ch, int v);
int char_get_choice(const struct char_data *ch);
void char_set_choice(struct char_data *ch, int v);
struct descriptor_data *char_get_desc(const struct char_data *ch);
void char_set_desc(struct char_data *ch, struct descriptor_data *v);
void char_set_energy(struct char_data *ch, double v);
struct follow_type *char_get_followers(const struct char_data *ch);
void char_set_followers(struct char_data *ch, struct follow_type *v);
int char_get_gooptime(const struct char_data *ch);
void char_set_gooptime(struct char_data *ch, int v);
double char_get_health(const struct char_data *ch);
void char_set_health(struct char_data *ch, double v);
void char_set_life(struct char_data *ch, double v);
int char_get_raw_lifebonus(const struct char_data *ch);
void char_set_raw_lifebonus(struct char_data *ch, int v);
int *char_get_limbs(struct char_data *ch);
struct char_data *char_get_master(const struct char_data *ch);
void char_set_master(struct char_data *ch, struct char_data *v);
int32_t char_get_master_id(const struct char_data *ch);
void char_set_master_id(struct char_data *ch, int32_t v);
int64_t char_get_max_hit(const struct char_data *ch);
void char_set_max_hit(struct char_data *ch, int64_t v);
void char_set_max_mana(struct char_data *ch, int64_t v);
int64_t char_get_max_move(const struct char_data *ch);
void char_set_max_move(struct char_data *ch, int64_t v);
struct script_memory *char_get_memory(const struct char_data *ch);
void char_set_memory(struct char_data *ch, struct script_memory *v);
void char_set_mimic(struct char_data *ch, int v);
int char_get_mobcharge(const struct char_data *ch);
void char_set_mobcharge(struct char_data *ch, int v);
struct char_data *char_get_next(const struct char_data *ch);
void char_set_next(struct char_data *ch, struct char_data *v);
struct char_data *char_get_next_affect(const struct char_data *ch);
void char_set_next_affect(struct char_data *ch, struct char_data *v);
struct char_data *char_get_next_fighting(const struct char_data *ch);
void char_set_next_fighting(struct char_data *ch, struct char_data *v);
struct char_data *char_get_next_in_room(const struct char_data *ch);
void char_set_next_in_room(struct char_data *ch, struct char_data *v);
struct player_special_data *char_get_player_specials(const struct char_data *ch);
void char_set_player_specials(struct char_data *ch, struct player_special_data *v);
struct char_data *char_get_poisonby(const struct char_data *ch);
void char_set_poisonby(struct char_data *ch, struct char_data *v);
struct trig_proto_list *char_get_proto_script(const struct char_data *ch);
void char_set_proto_script(struct char_data *ch, struct trig_proto_list *v);
void char_set_race(struct char_data *ch, int v);
int8_t char_get_real_abils_cha(const struct char_data *ch);
void char_set_real_abils_cha(struct char_data *ch, int8_t v);
int8_t char_get_real_abils_con(const struct char_data *ch);
void char_set_real_abils_con(struct char_data *ch, int8_t v);
int8_t char_get_real_abils_dex(const struct char_data *ch);
void char_set_real_abils_dex(struct char_data *ch, int8_t v);
int8_t char_get_real_abils_intel(const struct char_data *ch);
void char_set_real_abils_intel(struct char_data *ch, int8_t v);
int8_t char_get_real_abils_str(const struct char_data *ch);
void char_set_real_abils_str(struct char_data *ch, int8_t v);
int8_t char_get_real_abils_wis(const struct char_data *ch);
void char_set_real_abils_wis(struct char_data *ch, int8_t v);
struct script_data *char_get_script(const struct char_data *ch);
void char_set_script(struct char_data *ch, struct script_data *v);
int char_get_size(const struct char_data *ch);
void char_set_size(struct char_data *ch, int v);
void char_set_stamina(struct char_data *ch, double v);
char *char_get_temp_prompt(const struct char_data *ch);
int char_get_throws(const struct char_data *ch);
void char_set_throws(struct char_data *ch, int v);
time_t char_get_time_birth(const struct char_data *ch);
void char_set_time_birth(struct char_data *ch, time_t v);
time_t char_get_time_created(const struct char_data *ch);
void char_set_time_created(struct char_data *ch, time_t v);
time_t char_get_time_logon(const struct char_data *ch);
void char_set_time_logon(struct char_data *ch, time_t v);
time_t char_get_time_maxage(const struct char_data *ch);
void char_set_time_maxage(struct char_data *ch, time_t v);
time_t char_get_time_played(const struct char_data *ch);
void char_set_time_played(struct char_data *ch, time_t v);
int char_get_timer(const struct char_data *ch);
void char_set_timer(struct char_data *ch, int v);
char *char_get_raw_title(const struct char_data *ch);
void char_set_raw_title(struct char_data *ch, char *v);

/* A field's address, for code that fills the field in later or unlinks through it */
struct script_data **char_script_slot(struct char_data *ch);
struct script_memory **char_memory_slot(struct char_data *ch);
struct affected_type **char_affected_slot(struct char_data *ch);
struct player_special_data **char_player_specials_slot(struct char_data *ch);
struct obj_data **char_inventory_slot(struct char_data *ch);
char **char_description_slot(struct char_data *ch);
char **char_long_descr_slot(struct char_data *ch);
char **char_short_descr_slot(struct char_data *ch);
char **char_name_slot(struct char_data *ch);
int8_t *char_real_abils_str_slot(struct char_data *ch);
int8_t *char_real_abils_intel_slot(struct char_data *ch);
int8_t *char_real_abils_wis_slot(struct char_data *ch);
int8_t *char_real_abils_dex_slot(struct char_data *ch);
int8_t *char_real_abils_con_slot(struct char_data *ch);
int8_t *char_real_abils_cha_slot(struct char_data *ch);

/* The equipment slots as an array. */
struct obj_data **char_get_equipment_array(struct char_data *ch);

/* Unlinking a character from the lists that run through it */
void char_unlink_person(struct char_data **head, struct char_data *ch);
void char_unlink_from_affect_list(GameWorld *g, struct char_data *ch);
void char_unlink_from_combat_list(GameWorld *g, struct char_data *ch);

/* Resets the affected stats to the raw ones. */
void char_reset_aff_abils(struct char_data *ch);
void char_copy_time(struct char_data *to, const struct char_data *from);
