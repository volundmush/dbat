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

// Legacy Macros

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
  (*(((ch)->player_specials == &dummy_mob) ? (mud_log("OHNO: Mob using '" #var "' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var) (var)
#endif

bitvector_t *char_get_act(const struct char_data *ch);
#define PRF_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pref))
bitvector_t *char_get_affected_by(const struct char_data *ch);
bitvector_t *char_get_admflags(const struct char_data *ch);

/* Return the gauntlet highest room for ch */
#define GET_GAUNTLET(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->gauntlet))

bool char_is_npc(const struct char_data *ch);
bool char_is_mob(const struct char_data *ch);

bool char_mob_flagged(const struct char_data *ch, int flag);
bool char_plr_flagged(const struct char_data *ch, int flag);
bool char_aff_flagged(const struct char_data *ch, int flag);
bool char_prf_flagged(const struct char_data *ch, int flag);
bool char_adm_flagged(const struct char_data *ch, int flag);
bool char_body_flagged(const struct char_data *ch, int flag);
bool char_is_affected(const struct char_data *ch, int skill);

bool char_plr_toggle(struct char_data *ch, int flag);
bool char_prf_toggle(struct char_data *ch, int flag);
bool char_aff_toggle(struct char_data *ch, int flag);
bool char_mob_toggle(struct char_data *ch, int flag);

/* Flag setters. PLR bits are ignored on NPCs, and MOB bits on PCs (except MOB_ISNPC);
 * both kinds share char_data::act. */
void char_plr_set(struct char_data *ch, int flag, bool on);
void char_mob_set(struct char_data *ch, int flag, bool on);
void char_aff_set(struct char_data *ch, int flag, bool on);
void char_prf_set(struct char_data *ch, int flag, bool on);
void char_adm_set(struct char_data *ch, int flag, bool on);
void char_body_set(struct char_data *ch, int flag, bool on);

/* new define for quick check */
bool char_is_dead(const struct char_data *ch);

room_rnum char_get_in_room(const struct char_data *ch);
void char_set_in_room(struct char_data *ch, room_rnum v);
zone_vnum char_in_zone(const struct char_data *ch);
room_rnum char_get_was_in_room(const struct char_data *ch);
void char_set_was_in_room(struct char_data *ch, room_rnum v);
#define GET_AGE(ch) (age(ch)->year)

char *char_get_pc_name(const struct char_data *ch);
void char_set_pc_name(struct char_data *ch, char *v);
char *char_get_name(const struct char_data *ch);
char *char_get_title(const struct char_data *ch);
int char_get_starphase(const struct char_data *ch);
void char_set_starphase(struct char_data *ch, int v);
int char_get_mimic(const struct char_data *ch);
char *char_get_voice(const struct char_data *ch);
void char_set_voice(struct char_data *ch, char *v);
int char_get_transclass(const struct char_data *ch);
void char_set_transclass(struct char_data *ch, int v);
char *char_get_feature(const struct char_data *ch);
void char_set_feature(struct char_data *ch, char *v);
char *char_get_user(const struct char_data *ch);
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
int char_get_chclasses(const struct char_data *ch, int i);
void char_set_chclasses(struct char_data *ch, int i, int v);
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
int char_get_pc_height(struct char_data *ch);
int char_get_pc_weight(struct char_data *ch);
int8_t char_get_sex(const struct char_data *ch);
void char_set_sex(struct char_data *ch, int8_t v);
#define GET_TLEVEL(ch)  ((ch)->player_specials->tlevel)
#define CARRYING(ch)    ((ch)->player_specials->carrying)
#define CARRIED_BY(ch)  ((ch)->player_specials->carried_by)
#define RACIAL_PREF(ch) ((ch)->player_specials->racial_pref)
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
char *char_get_rdisplay(const struct char_data *ch);
void char_set_rdisplay(struct char_data *ch, char *v);

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

double char_get_mutboost(struct char_data *ch);
double char_get_speedi(struct char_data *ch);
double char_get_speedcalc(struct char_data *ch);
double char_get_speedbonus(struct char_data *ch);
double char_get_speedvar(struct char_data *ch);
double char_get_speedvem(struct char_data *ch);
bool char_is_grap(const struct char_data *ch);
long char_get_speedint(struct char_data *ch);
bool char_is_inferior(const struct char_data *ch);
bool char_is_weighted(struct char_data *ch);

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
#define GET_HIT(ch)      (getCurPL(ch))
#define GET_MAX_HIT(ch)  (getEffMaxPL(ch))
#define GET_MAX_MOVE(ch) (getMaxST(ch))
#define GET_MAX_MANA(ch) (getMaxKI(ch))
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
double char_get_blessbonus(struct char_data *ch);
double char_get_poself(struct char_data *ch);
double char_get_posebonus(struct char_data *ch);
double char_get_lifebonus(struct char_data *ch);
double char_get_lifebonuses(struct char_data *ch);
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
int16_t char_get_armorcheck(const struct char_data *ch);
void char_set_armorcheck(struct char_data *ch, int16_t v);
int16_t char_get_armorcheckall(const struct char_data *ch);
void char_set_armorcheckall(struct char_data *ch, int16_t v);
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

#define GET_COND(ch, i)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->conditions[(i)]))
#define GET_LOADROOM(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_room))
#define GET_PRACTICES(ch, ...) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->practice_points))
#define GET_TRAINS(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->ability_trains))
#define GET_TRAINSTR(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainstr))
#define GET_TRAININT(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainint))
#define GET_TRAINCON(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->traincon))
#define GET_TRAINWIS(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainwis))
#define GET_TRAINAGL(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainagl))
#define GET_TRAINSPD(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainspd))
#define GET_INVIS_LEV(ch)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->invis_level))
#define GET_WIMP_LEV(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->wimp_level))
#define GET_FREEZE_LEV(ch)     CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->freeze_level))
#define GET_BAD_PWS(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->bad_pws))
#define GET_TALK(ch, i)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->talks[i]))
#define POOFIN(ch)             CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)            CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_OLC_ZONE(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->olc_zone))
#define GET_LAST_OLC_TARG(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)  CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define GET_HOST(ch)           CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->host))
#define GET_HISTORY(ch, i)     CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->comm_hist[i]))

int8_t char_get_skills_mod(const struct char_data *ch, int i);
int8_t char_get_skills_perf(const struct char_data *ch, int i);
void char_set_skills_perf(struct char_data *ch, int i, int8_t v);
#define SET_SKILL_BONUS(ch, i, value) \
  do {                                \
    (ch)->skills[i].mod = value;      \
  } while (0)
#define SET_SKILL_PERF(ch, i, value) \
  do {                               \
    (ch)->skills[i].perf = value;    \
  } while (0)
int8_t char_get_skills_base(const struct char_data *ch, int i);
void char_set_skills_base(struct char_data *ch, int i, int8_t v);
int char_get_skill(const struct char_data *ch, int i);
#define SET_SKILL(ch, i, val)   \
  do {                          \
    (ch)->skills[i].base = val; \
  } while (0)
bitvector_t *char_get_bodyparts(const struct char_data *ch);

struct obj_data *char_get_equipment(const struct char_data *ch, int i);
void char_set_equipment(struct char_data *ch, int i, struct obj_data *v);

spec_proc_func char_get_mob_spec(const struct char_data *ch);
mob_rnum char_get_nr(const struct char_data *ch);
void char_set_nr(struct char_data *ch, mob_rnum v);
mob_vnum char_get_mob_vnum(const struct char_data *mob);

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

#define CAN_CARRY_W(ch) getMaxCarryWeight(ch)
#define CAN_CARRY_N(ch) 50
bool char_is_awake(const struct char_data *ch);
bool char_can_see_in_dark(const struct char_data *ch);

bool char_is_good(const struct char_data *ch);
bool char_is_evil(const struct char_data *ch);
bool char_is_lawful(const struct char_data *ch);
bool char_is_chaotic(const struct char_data *ch);
bool char_is_neutral(const struct char_data *ch);

bool char_in_arena(const struct char_data *ch);
int char_get_arenawatch(const struct char_data *ch);
void char_set_arenawatch(struct char_data *ch, int v);

/* These three deprecated. */
#define WAIT_STATE(ch, cycle)   \
  do {                          \
    char_set_wait(ch, (cycle)); \
  } while (0)
/* New, preferred macro. */
int char_get_wait(const struct char_data *ch);
void char_set_wait(struct char_data *ch, int v);

#define SENDOK(ch) (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
                    (to_sleeping || char_is_awake(ch)) &&            \
                    !char_plr_flagged((ch), PLR_WRITING))

bool char_light_ok(const struct char_data *sub);

bool char_invis_ok(const struct char_data *sub, const struct char_data *obj);

bool char_mort_can_see(const struct char_data *sub, const struct char_data *obj);

bool char_imm_can_see(const struct char_data *sub, const struct char_data *obj);

bool char_is_self(const struct char_data *sub, const struct char_data *obj);

/* Can subject see character "obj"? */
bool char_can_see(const struct char_data *sub, const struct char_data *obj);

bool char_not_hidden(const struct char_data *ch);
/* End of CAN_SEE */

bool char_invis_ok_obj(const struct char_data *sub, const struct obj_data *obj);

/* Is anyone carrying this object and if so, are they visible? */
bool char_can_see_obj_carrier(const struct char_data *sub, const struct obj_data *obj);

bool char_mort_can_see_obj(const struct char_data *sub, const struct obj_data *obj);

bool char_can_see_obj(const struct char_data *sub, const struct obj_data *obj);

bool char_can_carry_obj(struct char_data *ch, struct obj_data *obj);

bool char_can_get_obj(struct char_data *ch, struct obj_data *obj);

bool char_disg(const struct char_data *ch, const struct char_data *vict);

bool char_introd(struct char_data *ch, struct char_data *vict);

bool char_iswiz(const struct char_data *ch, const struct char_data *vict);

const char *char_pers(struct char_data *ch, struct char_data *vict);

char *obj_shortdesc_for(const struct obj_data *obj, const struct char_data *vict);

char *obj_fname_for(const struct obj_data *obj, const struct char_data *vict);

struct room_direction_data *char_get_exit(const struct char_data *ch, int door);
void char_set_exit(struct char_data *ch, int door, struct room_direction_data *v);
struct room_direction_data *char_get_exit2(const struct char_data *ch, int door);

bool char_can_go(const struct char_data *ch, int door);

#define RACE(ch)              (juggleRaceName(ch, true))
#define LRACE(ch)             (juggleRaceName(ch, false))
#define TRUE_RACE(ch)         (pc_race_types[ch->race])
#define SENSEI_NAME(ch)       (pc_class_types[ch->chclass])
#define SENSEI_NAME_LOWER(ch) (class_names[ch->chclass])
#define SENSEI_STYLE(ch)      (sensei_style[ch->chclass])

#define CLASS_ABBR(ch) (class_abbrevs[ch->chclass])
#define RACE_ABBR(ch)  (race_abbrevs[ch->race])

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
bool char_is_shadow_dragon1(const struct char_data *ch);
bool char_is_shadow_dragon2(const struct char_data *ch);
bool char_is_shadow_dragon3(const struct char_data *ch);
bool char_is_shadow_dragon4(const struct char_data *ch);
bool char_is_shadow_dragon5(const struct char_data *ch);
bool char_is_shadow_dragon6(const struct char_data *ch);
bool char_is_shadow_dragon7(const struct char_data *ch);
bool char_can_grand_master(const struct char_data *ch);
bool char_is_humanoid(const struct char_data *ch);
bool char_is_restricted_race(const struct char_data *ch);
bool char_is_cheap_race(const struct char_data *ch);
bool char_spar_train(const struct char_data *ch);
bool char_is_nonptrans(const struct char_data *ch);
bool char_is_fullpssj(const struct char_data *ch);
bool char_is_transformed(const struct char_data *ch);
#define BIRTH_PHASE (time_info.day <= 15)
#define LIFE_PHASE  (!BIRTH_PHASE && time_info.day <= 22)
#define DEATH_PHASE (!BIRTH_PHASE && !LIFE_PHASE)
bool char_moon_ok(const struct char_data *ch);
bool char_oozaru_ok(const struct char_data *ch);
bool char_is_oozaru_race(const struct char_data *ch);
#define MOON_TIME (time_info.hours >= 21 || time_info.hours <= 4)
#define MOON_DATE (time_info.day == 19 || time_info.day == 20 || time_info.day == 21)
bool MOON_TIMECHECK();
bool char_ether_stream(const struct char_data *ch);
bool char_has_moon(const struct char_data *ch);
bool char_has_arms(const struct char_data *ch);
bool char_has_legs(const struct char_data *ch);

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

bool char_is_male(const struct char_data *ch);
bool char_is_female(const struct char_data *ch);

bool char_is_outside(const struct char_data *ch);

bool char_outside_roomflag(const struct char_data *ch);

bool char_outside_secttype(const struct char_data *ch);

#define SPEAKING(ch) ((ch)->player_specials->speaking)

char *char_hshr(const struct char_data *ch);
char *char_hssh(const struct char_data *ch);
char *char_hmhr(const struct char_data *ch);
char *char_mafe(const struct char_data *ch);

/* returns the number of spells per slot */
#define GET_SPELL_LEVEL(ch, i) ((ch)->player_specials->spell_level[i])

#define GET_BAB(ch) char_get_accuracy(ch)
#define SET_FEAT(ch, i, value)                          \
  do {                                                  \
    CHECK_PLAYER_SPECIAL((ch), (ch)->feats[i]) = value; \
  } while (0)
#define GET_FEAT_POINTS(ch) (ch->player_specials->feat_points)

#define GET_MURDER(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->murder))

#define GET_PAGE_LENGTH(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->page_length))

int roll_stats(struct char_data *ch, int type, int bonus);
const char *juggleRaceName(char_data *ch, bool capitalized);
void restore_by(char_data *ch, char_data *healer);
void restore(char_data *ch, bool announce);
void resurrect(char_data *ch, int mode);
void ghostify(char_data *ch);
void teleport_to(char_data *ch, IDXTYPE rnum);
bool in_room_range(char_data *ch, IDXTYPE low_rnum, IDXTYPE high_rnum);
bool in_past(char_data *ch);
bool is_newbie(char_data *ch);
bool in_northran(char_data *ch);
bool can_tolerate_gravity(char_data *ch, int grav);
int calcTier(char_data *ch);
int64_t calc_soft_cap(char_data *ch);
bool is_soft_cap(char_data *ch, int64_t type, long double mult);
int calcGravCost(char_data *ch, int64_t num);

int64_t getCurHealth(char_data *ch);
int64_t getMaxHealth(char_data *ch);
double getCurHealthPercent(char_data *ch);
int64_t getPercentOfMaxHealth(char_data *ch, double amt);
bool isFullHealth(char_data *ch);
int64_t setCurHealth(char_data *ch, int64_t amt);
int64_t setCurHealthPercent(char_data *ch, double amt);
int64_t incCurHealth(char_data *ch, int64_t amt, bool limit_max);
int64_t decCurHealth(char_data *ch, int64_t amt, int64_t floor);
int64_t incCurHealthPercent(char_data *ch, double amt, bool limit_max);
int64_t decCurHealthPercent(char_data *ch, double amt, int64_t floor);
void restoreHealth(char_data *ch, bool announce);

int64_t getMaxPL(char_data *ch);
int64_t getMaxPLTrans(char_data *ch);
int64_t getCurPL(char_data *ch);
int64_t getBasePL(char_data *ch);
int64_t getEffBasePL(char_data *ch);
double getCurPLPercent(char_data *ch);
int64_t getPercentOfMaxPL(char_data *ch, double amt);
bool isFullPL(char_data *ch);

int64_t getCurKI(char_data *ch);
int64_t getMaxKI(char_data *ch);
int64_t getBaseKI(char_data *ch);
int64_t getEffBaseKI(char_data *ch);
double getCurKIPercent(char_data *ch);
int64_t getPercentOfMaxKI(char_data *ch, double amt);
bool isFullKI(char_data *ch);
int64_t setCurKIPercent(char_data *ch, double amt);
int64_t incCurKI(char_data *ch, int64_t amt, bool limit_max);
int64_t decCurKI(char_data *ch, int64_t amt, int64_t floor);
int64_t incCurKIPercent(char_data *ch, double amt, bool limit_max);
int64_t decCurKIPercent(char_data *ch, double amt, int64_t floor);
void restoreKI(char_data *ch, bool announce);

int64_t getCurST(char_data *ch);
int64_t getMaxST(char_data *ch);
int64_t getBaseST(char_data *ch);
int64_t getEffBaseST(char_data *ch);
double getCurSTPercent(char_data *ch);
bool isFullST(char_data *ch);
int64_t setCurSTPercent(char_data *ch, double amt);
int64_t incCurST(char_data *ch, int64_t amt, bool limit_max);
int64_t decCurST(char_data *ch, int64_t amt, int64_t floor);
int64_t incCurSTPercent(char_data *ch, double amt, bool limit_max);
int64_t decCurSTPercent(char_data *ch, double amt, int64_t floor);
void restoreST(char_data *ch, bool announce);

int64_t getCurLF(char_data *ch);
int64_t getMaxLF(char_data *ch);
double getCurLFPercent(char_data *ch);
bool isFullLF(char_data *ch);
int64_t incCurLF(char_data *ch, int64_t amt, bool limit_max);
int64_t decCurLF(char_data *ch, int64_t amt, int64_t floor);
int64_t incCurLFPercent(char_data *ch, double amt, bool limit_max);
int64_t decCurLFPercent(char_data *ch, double amt, int64_t floor);
void restoreLF(char_data *ch, bool announce);

bool isFullVitals(char_data *ch);
void restoreVitals(char_data *ch, bool announce);
void restoreStatus(char_data *ch, bool announce);
void restoreLimbs(char_data *ch, bool announce);

int64_t gainBasePL(char_data *ch, int64_t amt, bool trans_mult);
int64_t gainBaseKI(char_data *ch, int64_t amt, bool trans_mult);
int64_t gainBaseST(char_data *ch, int64_t amt, bool trans_mult);

int64_t loseBasePL(char_data *ch, int64_t amt, bool trans_mult);

int64_t gainBasePLPercent(char_data *ch, double amt, bool trans_mult);
int64_t gainBaseKIPercent(char_data *ch, double amt, bool trans_mult);
int64_t gainBaseSTPercent(char_data *ch, double amt, bool trans_mult);
void gainBaseAllPercent(char_data *ch, double amt, bool trans_mult);

int64_t loseBasePLPercent(char_data *ch, double amt, bool trans_mult);

void cureStatusKnockedOut(char_data *ch, bool announce);
void cureStatusBurn(char_data *ch, bool announce);
void cureStatusPoison(char_data *ch, bool announce);
void setStatusKnockedOut(char_data *ch);

int64_t getMaxCarryWeight(char_data *ch);
int64_t getCurGearWeight(char_data *ch);
int64_t getCurCarriedWeight(char_data *ch);

double speednar(char_data *ch);
int64_t getEffMaxPL(char_data *ch);
bool isWeightedPL(char_data *ch);

void apply_kaioken(char_data *ch, int times, bool announce);
void remove_kaioken(char_data *ch, int8_t announce);

void dispel_ash(struct char_data *ch);
int has_group(struct char_data *ch);
const char *report_party_health(struct char_data *ch);
int sec_roll_check(struct char_data *ch);
int get_measure(struct char_data *ch, int height, int weight);
int trans_req(struct char_data *ch, int trans);
const char *disp_align(struct char_data *ch);
void senseCreate(struct char_data *ch);
int read_sense_memory(struct char_data *ch, struct char_data *vict);
void sense_memory_write(struct char_data *ch, struct char_data *vict);
int roll_pursue(struct char_data *ch, struct char_data *vict);
void reveal_hiding(struct char_data *ch, int type);
int block_calc(struct char_data *ch);
int64_t molt_threshold(struct char_data *ch);
int armor_evolve(struct char_data *ch);
void handle_evolution(struct char_data *ch, int64_t dmg);
void demon_refill_lf(struct char_data *ch, int64_t num);
void mob_talk(struct char_data *ch, const char *speech);
int mob_respond(struct char_data *ch, struct char_data *vict, const char *speech);
bool is_sparring(struct char_data *ch);
char *introd_calc(struct char_data *ch);
int64_t gear_exp(struct char_data *ch, int64_t exp);
void purge_homing(struct char_data *ch);
void improve_skill(struct char_data *ch, int skill, int num);
void stop_follower(struct char_data *ch);
void die_follower(struct char_data *ch);
void add_follower(struct char_data *ch, struct char_data *leader);
bool circle_follow(struct char_data *ch, struct char_data *victim);

void advance_level(struct char_data *ch, int whichclass);
void set_title(struct char_data *ch, char *title);
void gain_exp(struct char_data *ch, int64_t gain);
void gain_exp_regardless(struct char_data *ch, int gain);
void gain_condition(struct char_data *ch, int condition, int value);
void update_pos(struct char_data *victim);

int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void perform_remove(struct char_data *ch, int pos);

int can_grav(struct char_data *ch);
int check_skill(struct char_data *ch, int skill);
int check_points(struct char_data *ch, int64_t ki, int64_t st);
void pcost(struct char_data *ch, double ki, int64_t st);
int limb_ok(struct char_data *ch, int type);
int64_t physical_cost(struct char_data *ch, int skill);

int init_skill(struct char_data *ch, int snum);
int chance_to_hit(struct char_data *ch);
int handle_speed(struct char_data *ch, struct char_data *vict);
struct time_info_data *age(struct char_data *ch);
const char *get_i_name(struct char_data *ch, struct char_data *vict);

void assign_affect(struct char_data *ch, int aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis, int spd);
int know_skill(struct char_data *ch, int skill);

size_t send_to_char(struct char_data *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
void admin_set(struct char_data *ch, int value);
char *sense_location(struct char_data *ch);
void null_affect(struct char_data *ch, int aff_flag);
bool is_affected(struct char_data *ch, int aff_flag);
void remove_affect(struct char_data *ch, int aff_flag);
int planet_check(struct char_data *ch, struct char_data *vict);
/* Player autoexit levels: used as an index to exitlevels           */

int char_exit_level(const struct char_data *ch);

int char_get_bank_interest(const struct char_data *ch);

bool race_has_tail(int r_id);
void char_lose_tail(char_data *ch);
bool char_has_tail(char_data *ch);
void char_gain_tail(char_data *ch, bool announce);

room_vnum sensei_start_room(int s_id);
room_vnum sensei_location_id(int s_id);
int sensei_grav_tolerance(int s_id);

int race_get_size(int r_id);
int race_is_playable(int r_id);
int race_is_people(int r_id);
int can_kill(struct char_data *ch, struct char_data *vict, struct obj_data *obj, int num);
