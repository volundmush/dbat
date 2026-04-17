#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "characters.h"
#include "flags.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/sex.h"
#include "consts/prefflags.h"
#include "consts/playerflags.h"
#include "consts/adminflags.h"
#include "consts/mobflags.h"

extern struct player_special_data dummy_mob;

struct descriptor_data;
struct obj_data;
struct follow_type;

// =============================================================================
// Classification (NPC/PC)
// =============================================================================

bool char_is_npc(const struct char_data *ch);
bool char_is_mob(const struct char_data *ch);
mob_rnum char_get_rnum(const struct char_data *ch);
mob_vnum char_get_vnum(const struct char_data *ch);

// =============================================================================
// Identity & Names
// =============================================================================

const char *char_get_name(const struct char_data *ch);
const char *char_get_pc_name(const struct char_data *ch);
const char *char_get_title(const struct char_data *ch);
const char *char_get_short_desc(const struct char_data *ch);
const char *char_get_long_desc(const struct char_data *ch);
const char *char_get_description(const struct char_data *ch);
const char *char_get_clan(const struct char_data *ch);
const char *char_get_voice(const struct char_data *ch);
const char *char_get_feature(const struct char_data *ch);

void char_set_name(struct char_data *ch, const char *name);
void char_set_title(struct char_data *ch, const char *title);
void char_set_clan(struct char_data *ch, const char *clan);

// =============================================================================
// Basic Attributes
// =============================================================================

int char_get_sex(const struct char_data *ch);
void char_set_sex(struct char_data *ch, int sex);

int char_get_size(const struct char_data *ch);
void char_set_size(struct char_data *ch, int size);

int char_get_race(const struct char_data *ch);
void char_set_race(struct char_data *ch, int race);

uint8_t char_get_weight(const struct char_data *ch);
void char_set_weight(struct char_data *ch, uint8_t weight);

uint8_t char_get_height(const struct char_data *ch);
void char_set_height(struct char_data *ch, uint8_t height);

// =============================================================================
// Appearance (race-specific lookups)
// =============================================================================

int char_get_hair_length(const struct char_data *ch);
int char_get_hair_style(const struct char_data *ch);
int char_get_hair_color(const struct char_data *ch);
int char_get_skin_color(const struct char_data *ch);
int char_get_eye_color(const struct char_data *ch);
int char_get_distinguishing_feature(const struct char_data *ch);
int char_get_aura(const struct char_data *ch);

void char_set_hair_length(struct char_data *ch, int val);
void char_set_hair_style(struct char_data *ch, int val);
void char_set_hair_color(struct char_data *ch, int val);
void char_set_skin_color(struct char_data *ch, int val);
void char_set_eye_color(struct char_data *ch, int val);
void char_set_distinguishing_feature(struct char_data *ch, int val);
void char_set_aura(struct char_data *ch, int val);

int char_get_pc_height(const struct char_data *ch);
int char_get_pc_weight(const struct char_data *ch);

// =============================================================================
// Position & Room
// =============================================================================

room_rnum char_get_in_room(const struct char_data *ch);
void char_set_in_room(struct char_data *ch, room_rnum room);

room_rnum char_get_was_in_room(const struct char_data *ch);
void char_set_was_in_room(struct char_data *ch, room_rnum room);

room_vnum char_get_home(const struct char_data *ch);
void char_set_home(struct char_data *ch, room_vnum home);

int char_get_position(const struct char_data *ch);
void char_set_position(struct char_data *ch, int pos);

bool char_is_awake(const struct char_data *ch);
bool char_is_dead(const struct char_data *ch);

int char_get_wait(const struct char_data *ch);
void char_set_wait(struct char_data *ch, int wait);

int char_get_timer(const struct char_data *ch);
void char_set_timer(struct char_data *ch, int timer);

int char_get_timer2(const struct char_data *ch);
void char_set_timer2(struct char_data *ch, int timer);

// =============================================================================
// Level & Class
// =============================================================================

int char_get_class_level(const struct char_data *ch);
int char_get_class(const struct char_data *ch);
int char_get_class_ranks(const struct char_data *ch, int class_num);
int char_get_class_nonepic(const struct char_data *ch, int class_num);
int char_get_class_epic(const struct char_data *ch, int class_num);
int char_get_race_level(const struct char_data *ch);
int char_get_level_adj(const struct char_data *ch);
int char_get_level(const struct char_data *ch);
int char_get_admlevel(const struct char_data *ch);
int char_get_pfilepos(const struct char_data *ch);
int32_t char_get_idnum(const struct char_data *ch);

void char_set_class(struct char_data *ch, int chclass);
void char_set_admlevel(struct char_data *ch, int level);
void char_set_level(struct char_data *ch, int level);

// =============================================================================
// Core Stats
// =============================================================================

int8_t char_get_str(const struct char_data *ch);
int8_t char_get_dex(const struct char_data *ch);
int8_t char_get_intel(const struct char_data *ch);
int8_t char_get_wis(const struct char_data *ch);
int8_t char_get_con(const struct char_data *ch);
int8_t char_get_cha(const struct char_data *ch);

void char_set_str(struct char_data *ch, int8_t val);
void char_set_dex(struct char_data *ch, int8_t val);
void char_set_intel(struct char_data *ch, int8_t val);
void char_set_wis(struct char_data *ch, int8_t val);
void char_set_con(struct char_data *ch, int8_t val);
void char_set_cha(struct char_data *ch, int8_t val);

// =============================================================================
// Vital Stats (hit/mana/move/ki)
// =============================================================================

int64_t char_get_hit(const struct char_data *ch);
int64_t char_get_max_hit(const struct char_data *ch);
int64_t char_get_mana(const struct char_data *ch);
int64_t char_get_max_mana(const struct char_data *ch);
int64_t char_get_move(const struct char_data *ch);
int64_t char_get_max_move(const struct char_data *ch);
int64_t char_get_ki(const struct char_data *ch);
int64_t char_get_max_ki(const struct char_data *ch);

void char_set_hit(struct char_data *ch, int64_t val);
void char_inc_hit(struct char_data *ch, int64_t delta);
void char_set_max_hit(struct char_data *ch, int64_t val);
void char_set_mana(struct char_data *ch, int64_t val);
void char_inc_mana(struct char_data *ch, int64_t delta);
void char_set_max_mana(struct char_data *ch, int64_t val);
void char_set_move(struct char_data *ch, int64_t val);
void char_inc_move(struct char_data *ch, int64_t delta);
void char_set_max_move(struct char_data *ch, int64_t val);
void char_set_ki(struct char_data *ch, int64_t val);
void char_inc_ki(struct char_data *ch, int64_t delta);
void char_set_max_ki(struct char_data *ch, int64_t val);

// =============================================================================
// Combat Stats
// =============================================================================

int char_get_armor(const struct char_data *ch);
void char_set_armor(struct char_data *ch, int val);

int char_get_accuracy(const struct char_data *ch);
void char_set_accuracy(struct char_data *ch, int val);
int char_get_accuracy_mod(const struct char_data *ch);
void char_set_accuracy_mod(struct char_data *ch, int val);

int char_get_damage_mod(const struct char_data *ch);
void char_set_damage_mod(struct char_data *ch, int val);

int char_get_alignment(const struct char_data *ch);
void char_set_alignment(struct char_data *ch, int val);
int char_get_ethic_alignment(const struct char_data *ch);
void char_set_ethic_alignment(struct char_data *ch, int val);

// =============================================================================
// Resources
// =============================================================================

int char_get_gold(const struct char_data *ch);
void char_set_gold(struct char_data *ch, int val);
void char_inc_gold(struct char_data *ch, int delta);

int char_get_bank_gold(const struct char_data *ch);
void char_set_bank_gold(struct char_data *ch, int val);
void char_inc_bank_gold(struct char_data *ch, int delta);

int64_t char_get_exp(const struct char_data *ch);
void char_set_exp(struct char_data *ch, int64_t val);
void char_inc_exp(struct char_data *ch, int64_t delta);

// =============================================================================
// NPC-specific
// =============================================================================

int8_t char_get_attack_type(const struct char_data *ch);
void char_set_attack_type(struct char_data *ch, int8_t type);

int8_t char_get_default_pos(const struct char_data *ch);
void char_set_default_pos(struct char_data *ch, int8_t pos);

int8_t char_get_damnodice(const struct char_data *ch);
int8_t char_get_damsizedice(const struct char_data *ch);

// =============================================================================
// Limb/Body Parts
// =============================================================================

int char_get_limb(const struct char_data *ch, int limb);
void char_set_limb(struct char_data *ch, int limb, int val);

int char_get_limb_condition(const struct char_data *ch, int limb);
void char_set_limb_condition(struct char_data *ch, int limb, int val);

bool char_has_arms(const struct char_data *ch);
bool char_has_legs(const struct char_data *ch);

// =============================================================================
// Carrying
// =============================================================================

int char_get_carry_weight(const struct char_data *ch);
void char_set_carry_weight(struct char_data *ch, int weight);
void char_inc_carry_weight(struct char_data *ch, int delta);

int char_get_carry_items(const struct char_data *ch);
void char_set_carry_items(struct char_data *ch, int items);

// =============================================================================
// Equipment
// =============================================================================

struct obj_data *char_get_equipment(const struct char_data *ch, int pos);
void char_set_equipment(struct char_data *ch, int pos, struct obj_data *obj);
struct obj_data *char_get_carrying(const struct char_data *ch);
void char_set_carrying(struct char_data *ch, struct obj_data *obj);

// =============================================================================
// Fighting
// =============================================================================

struct char_data *char_get_fighting(const struct char_data *ch);
void char_set_fighting(struct char_data *ch, struct char_data *victim);

struct follow_type *char_get_followers(const struct char_data *ch);
struct char_data *char_get_master(const struct char_data *ch);
void char_set_master(struct char_data *ch, struct char_data *master);
int32_t char_get_master_id(const struct char_data *ch);

// =============================================================================
// List Links
// =============================================================================

struct char_data *char_get_next_in_room(const struct char_data *ch);
struct char_data *char_get_next(const struct char_data *ch);
struct char_data *char_get_next_fighting(const struct char_data *ch);

void char_set_next_in_room(struct char_data *ch, struct char_data *next);
void char_set_next(struct char_data *ch, struct char_data *next);
void char_set_next_fighting(struct char_data *ch, struct char_data *next);

// =============================================================================
// Player Specials (PC-only, safe for NPCs via dummy_mob)
// =============================================================================

bitvector_t *char_get_pref(const struct char_data *ch);
int char_get_cond(const struct char_data *ch, int cond);
void char_set_cond(struct char_data *ch, int cond, int value);
void char_inc_cond(struct char_data *ch, int cond, int delta);

int char_get_loadroom(const struct char_data *ch);
void char_set_loadroom(struct char_data *ch, int room);

int char_get_invis_level(const struct char_data *ch);
void char_set_invis_level(struct char_data *ch, int level);

int char_get_wimp_level(const struct char_data *ch);
void char_set_wimp_level(struct char_data *ch, int level);

int char_get_gauntlet(const struct char_data *ch);
void char_set_gauntlet(struct char_data *ch, int val);

int char_get_trains(const struct char_data *ch);
void char_set_trains(struct char_data *ch, int val);
void char_inc_trains(struct char_data *ch, int delta);

int char_get_olc_zone(const struct char_data *ch);
void char_set_olc_zone(struct char_data *ch, int zone);

int char_get_speaking(const struct char_data *ch);
void char_set_speaking(struct char_data *ch, int lang);

int char_get_racial_pref(const struct char_data *ch);
void char_set_racial_pref(struct char_data *ch, int pref);

const char *char_get_host(const struct char_data *ch);
void char_set_host(struct char_data *ch, const char *host);

const char *char_get_poofin(const struct char_data *ch);
void char_set_poofin(struct char_data *ch, const char *poof);
const char *char_get_poofout(const struct char_data *ch);
void char_set_poofout(struct char_data *ch, const char *poof);

// =============================================================================
// Flags (using flag_test/flag_set)
// =============================================================================

bool char_mob_flagged(const struct char_data *ch, int flag);
bool char_plr_flagged(const struct char_data *ch, int flag);
bool char_aff_flagged(const struct char_data *ch, int flag);
bool char_prf_flagged(const struct char_data *ch, int flag);
bool char_adm_flagged(const struct char_data *ch, int flag);
bool char_body_flagged(const struct char_data *ch, int flag);

void char_set_mob_flag(struct char_data *ch, int flag, bool value);
void char_set_plr_flag(struct char_data *ch, int flag, bool value);
void char_set_aff_flag(struct char_data *ch, int flag, bool value);
void char_set_prf_flag(struct char_data *ch, int flag, bool value);
void char_set_adm_flag(struct char_data *ch, int flag, bool value);
void char_set_body_flag(struct char_data *ch, int flag, bool value);

bool char_toggle_plr_flag(struct char_data *ch, int flag);
bool char_toggle_aff_flag(struct char_data *ch, int flag);

// =============================================================================
// Skills
// =============================================================================

int8_t char_get_skill(const struct char_data *ch, int skill);
int8_t char_get_skill_base(const struct char_data *ch, int skill);
int8_t char_get_skill_bonus(const struct char_data *ch, int skill);
int8_t char_get_skill_perf(const struct char_data *ch, int skill);

void char_set_skill(struct char_data *ch, int skill, int8_t value);
void char_set_skill_bonus(struct char_data *ch, int skill, int8_t value);
void char_set_skill_perf(struct char_data *ch, int skill, int8_t value);
void char_inc_skill(struct char_data *ch, int skill, int8_t delta);

int8_t char_get_feat(const struct char_data *ch, int feat);
void char_set_feat(struct char_data *ch, int feat, int8_t value);

// =============================================================================
// Genome/Mutation
// =============================================================================

int char_get_genome(const struct char_data *ch, int index);
void char_set_genome(struct char_data *ch, int index, int val);

// =============================================================================
// Transformation/Form
// =============================================================================

int char_get_mimic(const struct char_data *ch);
void char_set_mimic(struct char_data *ch, int val);

int char_get_transform_class(const struct char_data *ch);
void char_set_transform_class(struct char_data *ch, int val);

int char_get_personality(const struct char_data *ch);
void char_set_personality(struct char_data *ch, int val);

int char_get_trans_cost(const struct char_data *ch, int index);
void char_set_trans_cost(struct char_data *ch, int index, int val);

int char_get_starphase(const struct char_data *ch);
void char_set_starphase(struct char_data *ch, int val);

int char_get_fury(const struct char_data *ch);
void char_set_fury(struct char_data *ch, short val);

int char_get_majinized(const struct char_data *ch);
void char_set_majinized(struct char_data *ch, int val);

int char_get_speedboost(const struct char_data *ch);
void char_set_speedboost(struct char_data *ch, int val);

int char_get_rage_meter(const struct char_data *ch);
void char_set_rage_meter(struct char_data *ch, int val);

int char_get_tail_growth(const struct char_data *ch);
void char_set_tail_growth(struct char_data *ch, int val);

int char_get_absorbs(const struct char_data *ch);
void char_set_absorbs(struct char_data *ch, int val);

int char_get_boosts(const struct char_data *ch);
void char_set_boosts(struct char_data *ch, int val);

int char_get_upgrade(const struct char_data *ch);
void char_set_upgrade(struct char_data *ch, int val);

int char_get_kaioken(const struct char_data *ch);
void char_set_kaioken(struct char_data *ch, int val);

// =============================================================================
// Boosts/Energy
// =============================================================================

int64_t char_get_base_pl(const struct char_data *ch);
void char_set_base_pl(struct char_data *ch, int64_t val);

int64_t char_get_base_ki(const struct char_data *ch);
void char_set_base_ki(struct char_data *ch, int64_t val);

int64_t char_get_base_st(const struct char_data *ch);
void char_set_base_st(struct char_data *ch, int64_t val);

int64_t char_get_charge(const struct char_data *ch);
void char_set_charge(struct char_data *ch, int64_t val);

int64_t char_get_chargeto(const struct char_data *ch);
void char_set_chargeto(struct char_data *ch, int64_t val);

int64_t char_get_barrier(const struct char_data *ch);
void char_set_barrier(struct char_data *ch, int64_t val);

int64_t char_get_suppression(const struct char_data *ch);
void char_set_suppression(struct char_data *ch, int64_t val);

int64_t char_get_suppressed(const struct char_data *ch);
void char_set_suppressed(struct char_data *ch, int64_t val);

int64_t char_get_lifeforce(const struct char_data *ch);
void char_set_lifeforce(struct char_data *ch, int64_t val);

// =============================================================================
// Combat State
// =============================================================================

struct char_data *char_get_grappling(const struct char_data *ch);
void char_set_grappling(struct char_data *ch, struct char_data *victim);

struct char_data *char_get_grappled(const struct char_data *ch);
void char_set_grappled(struct char_data *ch, struct char_data *victim);

int char_get_grap_type(const struct char_data *ch);
void char_set_grap_type(struct char_data *ch, int type);

struct char_data *char_get_blocks(const struct char_data *ch);
void char_set_blocks(struct char_data *ch, struct char_data *victim);

struct char_data *char_get_blocked(const struct char_data *ch);
void char_set_blocked(struct char_data *ch, struct char_data *victim);

struct char_data *char_get_absorbing(const struct char_data *ch);
void char_set_absorbing(struct char_data *ch, struct char_data *victim);

struct char_data *char_get_absorbedby(const struct char_data *ch);
void char_set_absorbedby(struct char_data *ch, struct char_data *victim);

struct char_data *char_get_defender(const struct char_data *ch);
void char_set_defender(struct char_data *ch, struct char_data *victim);

struct char_data *char_get_defending(const struct char_data *ch);
void char_set_defending(struct char_data *ch, struct char_data *victim);

struct char_data *char_get_drag(const struct char_data *ch);
void char_set_drag(struct char_data *ch, struct char_data *victim);

struct char_data *char_get_dragged(const struct char_data *ch);
void char_set_dragged(struct char_data *ch, struct char_data *victim);

struct char_data *char_get_original(const struct char_data *ch);
void char_set_original(struct char_data *ch, struct char_data *orig);

struct char_data *char_get_poisonby(const struct char_data *ch);
void char_set_poisonby(struct char_data *ch, struct char_data *victim);

// =============================================================================
// Grapple/Block/Sit helpers
// =============================================================================

bool char_is_grappling(const struct char_data *ch);
bool char_is_grappled(const struct char_data *ch);

// =============================================================================
// Descriptor
// =============================================================================

struct descriptor_data *char_get_desc(const struct char_data *ch);
void char_set_desc(struct char_data *ch, struct descriptor_data *d);

// =============================================================================
// Radar/Ship
// =============================================================================

room_vnum char_get_radar1(const struct char_data *ch);
void char_set_radar1(struct char_data *ch, room_vnum vnum);

room_vnum char_get_radar2(const struct char_data *ch);
void char_set_radar2(struct char_data *ch, room_vnum vnum);

room_vnum char_get_radar3(const struct char_data *ch);
void char_set_radar3(struct char_data *ch, room_vnum vnum);

int char_get_ship(const struct char_data *ch);
void char_set_ship(struct char_data *ch, int ship);

room_vnum char_get_ship_room(const struct char_data *ch);
void char_set_ship_room(struct char_data *ch, room_vnum room);

room_vnum char_get_droom(const struct char_data *ch);
void char_set_droom(struct char_data *ch, room_vnum room);

// =============================================================================
// Time/Logs
// =============================================================================

time_t char_get_lastpl(const struct char_data *ch);
void char_set_lastpl(struct char_data *ch, time_t t);

time_t char_get_deathtime(const struct char_data *ch);
void char_set_deathtime(struct char_data *ch, time_t t);

time_t char_get_rewtime(const struct char_data *ch);
void char_set_rewtime(struct char_data *ch, time_t t);

time_t char_get_lastint(const struct char_data *ch);
void char_set_lastint(struct char_data *ch, time_t t);

const char *char_get_loguser(const struct char_data *ch);
void char_set_loguser(struct char_data *ch, const char *user);

// =============================================================================
// Admin/Moderation
// =============================================================================

int char_get_arenawatch(const struct char_data *ch);
void char_set_arenawatch(struct char_data *ch, int val);

bool char_is_admin(const struct char_data *ch, int min_level);

// =============================================================================
// Game State
// =============================================================================

int char_get_death_type(const struct char_data *ch);
void char_set_death_type(struct char_data *ch, int type);

int char_get_choice(const struct char_data *ch);
void char_set_choice(struct char_data *ch, int choice);

int char_get_sleeptime(const struct char_data *ch);
void char_set_sleeptime(struct char_data *ch, int val);

int char_get_foodr(const struct char_data *ch);
void char_set_foodr(struct char_data *ch, int val);

int char_get_altitude(const struct char_data *ch);
void char_set_altitude(struct char_data *ch, int val);

int char_get_cranking(const struct char_data *ch);
void char_set_cranking(struct char_data *ch, int val);

int char_get_overflow(const struct char_data *ch);
void char_set_overflow(struct char_data *ch, int val);

int char_get_spam(const struct char_data *ch);
void char_set_spam(struct char_data *ch, int val);

int char_get_cooldown(const struct char_data *ch);
void char_set_cooldown(struct char_data *ch, int val);

int char_get_song(const struct char_data *ch);
void char_set_song(struct char_data *ch, short song);

short char_get_clones(const struct char_data *ch);
void char_set_clones(struct char_data *ch, short val);

int char_get_combine(const struct char_data *ch);
void char_set_combine(struct char_data *ch, int val);

int char_get_linker(const struct char_data *ch);
void char_set_linker(struct char_data *ch, int val);

int char_get_fish_state(const struct char_data *ch);
void char_set_fish_state(struct char_data *ch, int val);

int char_get_throws(const struct char_data *ch);
void char_set_throws(struct char_data *ch, int val);

int char_get_relax_count(const struct char_data *ch);
void char_set_relax_count(struct char_data *ch, int val);

int char_get_preference(const struct char_data *ch);
void char_set_preference(struct char_data *ch, int val);

int char_get_agg_timer(const struct char_data *ch);
void char_set_agg_timer(struct char_data *ch, int val);

// =============================================================================
// Experience/Leveling
// =============================================================================

int64_t char_get_moltexp(const struct char_data *ch);
void char_set_moltexp(struct char_data *ch, int64_t val);

int char_get_moltlevel(const struct char_data *ch);
void char_set_moltlevel(struct char_data *ch, int val);

int64_t char_get_majinizer(const struct char_data *ch);
void char_set_majinizer(struct char_data *ch, int64_t val);

int char_get_skill_slots(const struct char_data *ch);
void char_set_skill_slots(struct char_data *ch, int val);

// =============================================================================
// Board/History
// =============================================================================

int char_get_board(const struct char_data *ch, int index);
void char_set_board(struct char_data *ch, int index, int val);

int char_get_lastattack(const struct char_data *ch);
void char_set_lastattack(struct char_data *ch, int val);

int char_get_combhits(const struct char_data *ch);
void char_set_combhits(struct char_data *ch, int val);

int char_get_combo(const struct char_data *ch);
void char_set_combo(struct char_data *ch, int val);

int char_get_ping(const struct char_data *ch);
void char_set_ping(struct char_data *ch, int val);

// =============================================================================
// Saving Throws
// =============================================================================

int char_get_save_base(const struct char_data *ch, int type);
void char_set_save_base(struct char_data *ch, int type, int16_t val);

int char_get_save_mod(const struct char_data *ch, int type);
void char_set_save_mod(struct char_data *ch, int type, int16_t val);

// =============================================================================
// Spell/Magic
// =============================================================================

int16_t char_get_spellfail(const struct char_data *ch);
void char_set_spellfail(struct char_data *ch, int16_t val);

int16_t char_get_armorcheck(const struct char_data *ch);
void char_set_armorcheck(struct char_data *ch, int16_t val);

int16_t char_get_armorcheckall(const struct char_data *ch);
void char_set_armorcheckall(struct char_data *ch, int16_t val);

// =============================================================================
// Misc Bonuses
// =============================================================================

int char_get_bonus(const struct char_data *ch, int index);
void char_set_bonus(struct char_data *ch, int index, int val);

int char_get_ccpoints(const struct char_data *ch);
void char_set_ccpoints(struct char_data *ch, int val);

int char_get_negcount(const struct char_data *ch);
void char_set_negcount(struct char_data *ch, int val);

int char_get_lifebonus(const struct char_data *ch);
void char_set_lifebonus(struct char_data *ch, int val);

int char_get_asb(const struct char_data *ch);
void char_set_asb(struct char_data *ch, int val);

int char_get_regen(const struct char_data *ch);
void char_set_regen(struct char_data *ch, int val);

int char_get_rbank(const struct char_data *ch);
void char_set_rbank(struct char_data *ch, int val);

int char_get_lifepperc(const struct char_data *ch);
void char_set_lifepperc(struct char_data *ch, int val);

int char_get_blesslvl(const struct char_data *ch);
void char_set_blesslvl(struct char_data *ch, int val);

int char_get_gooptime(const struct char_data *ch);
void char_set_gooptime(struct char_data *ch, int val);

int char_get_sdc_cooldown(const struct char_data *ch);
void char_set_sdc_cooldown(struct char_data *ch, int val);

int char_get_backstab_cooldown(const struct char_data *ch);
void char_set_backstab_cooldown(struct char_data *ch, int val);

int char_get_con_cooldown(const struct char_data *ch);
void char_set_con_cooldown(struct char_data *ch, int val);

int char_get_forgetting(const struct char_data *ch);
void char_set_forgetting(struct char_data *ch, int val);

int char_get_forgetcount(const struct char_data *ch);
void char_set_forgetcount(struct char_data *ch, int val);

short char_get_stupidkiss(const struct char_data *ch);
void char_set_stupidkiss(struct char_data *ch, short val);

int char_get_btime(const struct char_data *ch);
void char_set_btime(struct char_data *ch, short val);

int char_get_armor_last(const struct char_data *ch);
void char_set_armor_last(struct char_data *ch, int val);

int char_get_ingest_learned(const struct char_data *ch);
void char_set_ingest_learned(struct char_data *ch, int val);

// =============================================================================
// RP/Social
// =============================================================================

int char_get_rp(const struct char_data *ch);
void char_set_rp(struct char_data *ch, int val);

int char_get_trp(const struct char_data *ch);
void char_set_trp(struct char_data *ch, int val);

int char_get_eavesdrop(const struct char_data *ch);
void char_set_eavesdrop(struct char_data *ch, int room);

int char_get_eavesdir(const struct char_data *ch);
void char_set_eavesdir(struct char_data *ch, int dir);

int char_get_mob_charge(const struct char_data *ch);
void char_set_mob_charge(struct char_data *ch, int val);

// =============================================================================
// Display/UI
// =============================================================================

const char *char_get_rdisplay(const struct char_data *ch);
void char_set_rdisplay(struct char_data *ch, const char *display);

const char *char_get_temp_prompt(const struct char_data *ch);
void char_set_temp_prompt(struct char_data *ch, const char *prompt);

// =============================================================================
// Radar
// =============================================================================

const char *char_get_rdisplay(const struct char_data *ch);

// =============================================================================
// Special/Memory
// =============================================================================

memory_rec *char_get_memory(const struct char_data *ch);
void char_set_memory(struct char_data *ch, memory_rec *mem);
void char_add_memory(struct char_data *ch, int32_t id);
void char_remove_memory(struct char_data *ch, int32_t id);

// =============================================================================
// Eavesdropping
// =============================================================================

room_vnum char_get_listenroom(const struct char_data *ch);
void char_set_listenroom(struct char_data *ch, room_vnum room);

// =============================================================================
// Waiting/Special States
// =============================================================================

int char_get_powerattack(const struct char_data *ch);
void char_set_powerattack(struct char_data *ch, int val);

int char_get_combat_expertise(const struct char_data *ch);
void char_set_combat_expertise(struct char_data *ch, int val);

int char_get_cooldown_mob(const struct char_data *ch);
void char_set_cooldown_mob(struct char_data *ch, int val);

// =============================================================================
// ID
// =============================================================================

int32_t char_get_id(const struct char_data *ch);
void char_set_id(struct char_data *ch, int32_t id);

// =============================================================================
// File Position
// =============================================================================

int char_get_pfilepos(const struct char_data *ch);
void char_set_pfilepos(struct char_data *ch, int pos);

// =============================================================================
// Life/Death
// =============================================================================

const struct time_data *char_get_time(const struct char_data *ch);
int char_get_lifeperc(const struct char_data *ch);
void char_set_lifeperc(struct char_data *ch, int val);

// =============================================================================
// NewItem (NPC)
// =============================================================================

int char_get_newitem(const struct char_data *ch);
void char_set_newitem(struct char_data *ch, int val);

// =============================================================================
// Races & Classes (Predicates)
// =============================================================================

bool char_is_race(const struct char_data *ch, int race_id);
bool char_is_class(const struct char_data *ch, int class_id);
bool char_has_class_rank(const struct char_data *ch, int class_num);

bool char_is_male(const struct char_data *ch);
bool char_is_female(const struct char_data *ch);
bool char_is_neuter(const struct char_data *ch);

// =============================================================================
// Alignment (Predicates)
// =============================================================================

bool char_is_good(const struct char_data *ch);
bool char_is_evil(const struct char_data *ch);
bool char_is_neutral(const struct char_data *ch);
bool char_is_lawful(const struct char_data *ch);
bool char_is_chaotic(const struct char_data *ch);
bool char_is_eneutral(const struct char_data *ch);

// =============================================================================
// Special Races
// =============================================================================

bool char_is_inferior(const struct char_data *ch);
bool char_is_oozaru_race(const struct char_data *ch);
bool char_is_transformed(const struct char_data *ch);
bool char_is_fullpssj(const struct char_data *ch);
bool char_is_nonptrans(const struct char_data *ch);
bool char_is_humanoid(const struct char_data *ch);
bool char_is_weighted(const struct char_data *ch);
bool char_is_spoiled(const struct char_data *ch);


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
	(*(((ch)->player_specials == &dummy_mob) ? (log("OHNO: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define MOB_FLAGS(ch)	((ch)->act)
#define PLR_FLAGS(ch)	((ch)->act)
#define PRF_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pref))
#define AFF_FLAGS(ch)	((ch)->affected_by)
#define ADM_FLAGS(ch)	((ch)->admflags)

/* Return the gauntlet highest room for ch */ 
#define GET_GAUNTLET(ch)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->gauntlet))

#define IS_NPC(ch)	(IS_SET_AR(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)	(IS_NPC(ch) && GET_MOB_RNUM(ch) <= top_of_mobt && \
				GET_MOB_RNUM(ch) != NOBODY)

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET_AR(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET_AR(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET_AR(AFF_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET_AR(PRF_FLAGS(ch), (flag)))
#define ADM_FLAGGED(ch, flag) (IS_SET_AR(ADM_FLAGS(ch), (flag)))
#define BODY_FLAGGED(ch, flag) (IS_SET_AR(BODY_PARTS(ch), (flag)))
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(PLR_FLAGS(ch), (flag))) & Q_BIT(flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(PRF_FLAGS(ch), (flag))) & Q_BIT(flag))
#define ADM_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(ADM_FLAGS(ch), (flag))) & Q_BIT(flag))
#define AFF_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(AFF_FLAGS(ch), (flag))) & Q_BIT(flag))

/* new define for quick check */
#define DEAD(ch) (PLR_FLAGGED((ch), PLR_NOTDEADYET) || MOB_FLAGGED((ch), MOB_NOTDEADYET))


#define IN_ROOM(ch)	((ch)->in_room)
#define IN_ZONE(ch)   (zone_table[(world[(IN_ROOM(ch))].zone)].number)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch)->year)

#define GET_PC_NAME(ch)	((ch)->name)
#define GET_NAME(ch)    (IS_NPC(ch) ? \
			 (ch)->short_descr : GET_PC_NAME(ch))
#define GET_TITLE(ch)   ((ch)->desc ? ((ch)->desc->title ? (ch)->desc->title : "[Unset Title]") : "@D[@GNew User@D]")
#define GET_USER_TITLE(d) ((d)->title)
#define GET_PHASE(ch)   ((ch)->starphase)
#define GET_MIMIC(ch)   ((ch)->mimic)
#define GET_VOICE(ch)   ((ch)->voice)
#define GET_CLAN(ch)    ((ch)->clan)
#define GET_TRANSCLASS(ch) ((ch)->transclass)
#define GET_FEATURE(ch) ((ch)->feature)
#define GET_USER(ch)    ((ch)->desc ? ((ch)->desc->user ? (ch)->desc->user : "NOUSER") : "NOUSER")
#define GET_LOG_USER(ch) ((ch)->loguser)
#define GET_CRANK(ch)   ((ch)->crank)
#define GET_ADMLEVEL(ch)	((ch)->admlevel)
#define GET_CLASS_LEVEL(ch)	((ch)->level)
#define GET_LEVEL_ADJ(ch)	((ch)->level_adj)
#define GET_HITDICE(ch)		((ch)->race_level)
#define GET_LEVEL(ch)	(GET_CLASS_LEVEL(ch) + GET_LEVEL_ADJ(ch) + GET_HITDICE(ch))
#define GET_PFILEPOS(ch)((ch)->pfilepos)

#define GET_CLASS(ch)   ((ch)->chclass ? (ch)->chclass : 0)
#define GET_CLASS_NONEPIC(ch, whichclass) ((ch)->chclasses[whichclass])
#define GET_CLASS_EPIC(ch, whichclass) ((ch)->epicclasses[whichclass])
#define GET_CLASS_RANKS(ch, whichclass) (GET_CLASS_NONEPIC(ch, whichclass) + \
                                         GET_CLASS_EPIC(ch, whichclass))
#define GET_RACE(ch)    ((ch)->race)
#define GET_HAIRL(ch)   ((ch)->hairl)
#define GET_HAIRC(ch)   ((ch)->hairc)
#define GET_HAIRS(ch)   ((ch)->hairs)
#define GET_SKIN(ch)    ((ch)->skin)
#define GET_EYE(ch)     ((ch)->eye)
#define GET_DISTFEA(ch) ((ch)->distfea)
#define GET_HOME(ch)	((ch)->hometown)
#define GET_WEIGHT(ch)  ((ch)->weight)
#define GET_HEIGHT(ch)  ((ch)->height)
#define GET_PC_HEIGHT(ch)	(!IS_NPC(ch) ? age(ch)->year <= 10 ? (int)((ch)->height * 0.68) : age(ch)->year <= 12 ? (int)((ch)->height * 0.72) : age(ch)->year <= 14 ? (int)((ch)->height * 0.85) : age(ch)->year <= 16 ? (int)((ch)->height * 0.92) : (ch)->height : (ch)->height)
#define GET_PC_WEIGHT(ch)	(!IS_NPC(ch) ? age(ch)->year <= 10 ? (int)((ch)->weight * 0.48) : age(ch)->year <= 12 ? (int)((ch)->weight * 0.55) : age(ch)->year <= 14 ? (int)((ch)->weight * 0.7) : age(ch)->year <= 16 ? (int)((ch)->weight * 0.85) : (ch)->weight : (ch)->weight)
#define GET_SEX(ch)	((ch)->sex)
#define GET_TLEVEL(ch)	((ch)->player_specials->tlevel)
#define CARRYING(ch)    ((ch)->player_specials->carrying)
#define CARRIED_BY(ch)  ((ch)->player_specials->carried_by)
#define RACIAL_PREF(ch) ((ch)->player_specials->racial_pref)
#define GET_RP(ch)      ((ch)->rp)
#define GET_RBANK(ch)   ((ch)->rbank)
#define GET_TRP(ch)     ((ch)->trp)
#define GET_SUPPRESS(ch) ((ch)->suppression)
#define GET_SUPP(ch)    ((ch)->suppressed)
#define GET_RDISPLAY(ch) ((ch)->rdisplay)

#define GET_STR(ch)     ((ch)->aff_abils.str)
/*
 * We could define GET_ADD to be ((GET_STR(ch) > 18) ?
 *                                ((GET_STR(ch) - 18) * 10) : 0)
 * but it's better to leave it undefined and fix the places that call
 * GET_ADD to use the new semantics for abilities.
 *                               - Elie Rosenblum 13/Dec/2003
 */
/* The old define: */
/* #define GET_ADD(ch)     ((ch)->aff_abils.str_add) */
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

#define GET_MUTBOOST(ch) (IS_MUTANT(ch) ? ((GET_GENOME(ch, 0) == 1 || GET_GENOME(ch, 1) == 1) ? (GET_SPEEDCALC(ch) + GET_SPEEDBONUS(ch) + GET_SPEEDBOOST(ch)) * 0.3 : 0) : 0)
#define GET_SPEEDI(ch)  (GET_SPEEDCALC(ch) + GET_SPEEDBONUS(ch) + GET_SPEEDBOOST(ch) + GET_MUTBOOST(ch))
#define GET_SPEEDCALC(ch) (IS_GRAP(ch) ? GET_CHA(ch) : (IS_INFERIOR(ch) ? (AFF_FLAGGED(ch, AFF_FLYING) ? (GET_SPEEDVAR(ch) * 1.25) : GET_SPEEDVAR(ch)) : GET_SPEEDVAR(ch)))
#define GET_SPEEDBONUS(ch) (IS_ARLIAN(ch) ? AFF_FLAGGED(ch, AFF_SHELL) ? GET_SPEEDVAR(ch) * -0.5 : (IS_MALE(ch) ? (AFF_FLAGGED(ch, AFF_FLYING) ? (GET_SPEEDVAR(ch) * 0.5) : 0) : 0) : 0)
#define GET_SPEEDVAR(ch) (GET_SPEEDVEM(ch) > GET_CHA(ch) ? GET_SPEEDVEM(ch) : GET_CHA(ch))
#define GET_SPEEDVEM(ch) (GET_SPEEDINT(ch) - (GET_SPEEDINT(ch) * speednar(ch)))
#define IS_GRAP(ch)     (GRAPPLING(ch) || GRAPPLED(ch))
#define GET_SPEEDINT(ch) (IS_BIO(ch) ? ((GET_CHA(ch) * GET_DEX(ch)) * (GET_MAX_HIT(ch) / 1200) / 1200) + (GET_CHA(ch) * (GET_KAIOKEN(ch) * 100)) : ((GET_CHA(ch) * GET_DEX(ch)) * (GET_MAX_HIT(ch) / 1000) / 1000) + (GET_CHA(ch) * (GET_KAIOKEN(ch) * 100)))
#define IS_INFERIOR(ch) (IS_KONATSU(ch) || IS_DEMON(ch))
#define IS_WEIGHTED(ch) (getEffMaxPL(ch) < GET_MAX_HIT(ch))


#define GET_EXP(ch)	  ((ch)->exp)
/*
 * Changed GET_AC to GET_ARMOR so that code with GET_AC will need to be
 * looked at to see if it needs to change before being converted to use
 * GET_ARMOR
 */
#define SPOILED(ch)       ((ch)->time.played > 86400)
#define GET_DEATH_TYPE(ch) ((ch)->death_type)
#define GET_SLEEPT(ch)    ((ch)->sleeptime)
#define GET_FOODR(ch)     ((ch)->foodr)
#define GET_ALT(ch)       ((ch)->altitude)
#define GET_CHARGE(ch)    ((ch)->charge)
#define GET_CHARGETO(ch)  ((ch)->chargeto)
#define GET_ARMOR(ch)     ((ch)->armor)
#define GET_ARMOR_LAST(ch) ((ch)->armor_last)
#define GET_HIT(ch)	  (getCurPL(ch))
#define GET_MAX_HIT(ch)	  (getEffMaxPL(ch))
#define GET_MAX_MOVE(ch)  (getMaxST(ch))
#define GET_MAX_MANA(ch)  (getMaxKI(ch))
#define GET_KI(ch)	  ((ch)->ki)
#define GET_MAX_KI(ch)    ((ch)->max_ki)
#define GET_DROOM(ch)     ((ch)->droom)
#define GET_OVERFLOW(ch)  ((ch)->overf)
#define GET_SPAM(ch)      ((ch)->spam)
#define GET_SHIP(ch)      ((ch)->ship)
#define GET_SHIPROOM(ch)  ((ch)->shipr)
#define GET_LPLAY(ch)     ((ch)->lastpl)
#define GET_DTIME(ch)     ((ch)->deathtime)
#define GET_RTIME(ch)     ((ch)->rewtime)
#define GET_DCOUNT(ch)    ((ch)->dcount)
#define GET_BOARD(ch, i)  ((ch)->lboard[i])
#define GET_LIMBS(ch, i)  ((ch)->limbs[i])
// why is this i-1? Because whoever wrote it didn't know how C arrays work.
// We'll be replacing the limb system anyways so fuck this macro.
#define GET_LIMBCOND(ch, i) ((ch)->limb_condition[i-1])
#define GET_SONG(ch)      ((ch)->powerattack)
#define GET_BONUS(ch, i)  ((ch)->bonuses[i])
#define GET_TRANSCOST(ch, i) ((ch)->transcost[i])
#define GET_CCPOINTS(ch)  ((ch)->ccpoints)
#define GET_NEGCOUNT(ch)  ((ch)->negcount)
#define GET_GENOME(ch, i)    ((ch)->genome[i])
#define HAS_GENOME(ch, i) ((ch)->genome[0] == (i) || (ch)->genome[1] == (i))
#define COMBO(ch)         ((ch)->combo)
#define LASTATK(ch)       ((ch)->lastattack)
#define COMBHITS(ch)      ((ch)->combhits)
#define GET_AURA(ch)      ((ch)->aura)
#define GET_RADAR1(ch)    ((ch)->radar1)
#define GET_RADAR2(ch)    ((ch)->radar2)
#define GET_RADAR3(ch)    ((ch)->radar3)
#define GET_PING(ch)      ((ch)->ping)
#define GET_SLOTS(ch)     ((ch)->skill_slots)
#define GET_TGROWTH(ch)   ((ch)->tail_growth)
#define GET_RMETER(ch)    ((ch)->rage_meter)
#define GET_PERSONALITY(ch) ((ch)->personality)
#define GET_COMBINE(ch)   ((ch)->combine)
#define GET_PREFERENCE(ch) ((ch)->preference)
#define GET_RELAXCOUNT(ch) ((ch)->relax_count)
#define GET_BLESSLVL(ch)  ((ch)->blesslvl)
#define GET_ASB(ch)       ((ch)->asb)
#define GET_REGEN(ch)     ((ch)->regen)
#define GET_BLESSBONUS(ch) (AFF_FLAGGED(ch, AFF_BLESS) ? (GET_BLESSLVL(ch) >= 100 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.1 : GET_BLESSLVL(ch) >= 60 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.05 : GET_BLESSLVL(ch) >= 40 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.02 : 0) : 0) 
#define GET_POSELF(ch)    (!IS_NPC(ch) ? PLR_FLAGGED(ch, PLR_POSE) ? GET_SKILL(ch, SKILL_POSE) >= 100 ? 0.15 : GET_SKILL(ch, SKILL_POSE) >= 60 ? 0.1 : GET_SKILL(ch, SKILL_POSE) >= 40 ? 0.05 : 0 : 0 : 0)
#define GET_POSEBONUS(ch) (((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * GET_POSELF(ch))
#define GET_LIFEBONUS(ch) (IS_ARLIAN(ch) ? ((GET_MAX_MANA(ch) * 0.01) * (GET_MOLT_LEVEL(ch) / 100)) + ((GET_MAX_MOVE(ch) * 0.01) * (GET_MOLT_LEVEL(ch) / 100)) : 0)
#define GET_LIFEBONUSES(ch) ((ch)->lifebonus > 0 ? (GET_LIFEBONUS(ch) + GET_BLESSBONUS(ch) + GET_POSEBONUS(ch)) * (((ch)->lifebonus + 100) * 0.01) : (GET_LIFEBONUS(ch) + GET_BLESSBONUS(ch) + GET_POSEBONUS(ch)))
#define GET_LIFEPERC(ch)  ((ch)->lifeperc)
#define GET_STUPIDKISS(ch) ((ch)->stupidkiss)
#define GET_SPEEDBOOST(ch) ((ch)->speedboost)
#define GET_BACKSTAB_COOL(ch) ((ch)->backstabcool)
#define GET_COOLDOWN(ch)  ((ch)->con_cooldown)
#define GET_BARRIER(ch)   ((ch)->barrier)
#define GET_GOLD(ch)	  ((ch)->gold)
#define GET_KAIOKEN(ch)   ((ch)->kaioken)
#define GET_BOOSTS(ch)    ((ch)->boosts)
#define MAJINIZED(ch)     ((ch)->majinize)
#define GET_MAJINIZED(ch) ((ch)->majinizer)
#define GET_FURY(ch)      ((ch)->fury)
#define GET_BTIME(ch)     ((ch)->btime)
#define GET_UP(ch)        ((ch)->upgrade)
#define GET_FORGETING(ch) ((ch)->forgeting)
#define GET_FORGET_COUNT(ch) ((ch)->forgetcount)
#define GET_BANK_GOLD(ch) ((ch)->bank_gold)
#define GET_POLE_BONUS(ch) ((ch)->accuracy)
#define GET_FISHSTATE(ch)  ((ch)->fishstate)
#define GET_FISHD(ch)     ((ch)->accuracy_mod)
#define GET_DAMAGE_MOD(ch) ((ch)->damage_mod)
#define GET_SPELLFAIL(ch) ((ch)->spellfail)
#define GET_ARMORCHECK(ch) ((ch)->armorcheck)
#define GET_ARMORCHECKALL(ch) ((ch)->armorcheckall)
#define GET_MOLT_EXP(ch)  ((ch)->moltexp)
#define GET_MOLT_LEVEL(ch) ((ch)->moltlevel)
#define GET_SDCOOLDOWN(ch) ((ch)->con_sdcooldown)
#define GET_INGESTLEARNED(ch) ((ch)->ingestLearned)
#define GET_POS(ch)		((ch)->position)
#define GET_IDNUM(ch)		((ch)->idnum)
#define GET_ID(x)		((x)->id)
#define IS_CARRYING_W(ch)	((ch)->carry_weight)
#define IS_CARRYING_N(ch)	((ch)->carry_items)
#define FIGHTING(ch)		((ch)->fighting)
#define GET_POWERATTACK(ch)	((ch)->powerattack)
#define GET_GROUPKILLS(ch)	((ch)->combatexpertise)
#define GET_SAVE_BASE(ch, i)	((ch)->saving_throw[i])
#define GET_SAVE_MOD(ch, i)	((ch)->apply_saving_throw[i])
#define GET_SAVE(ch, i)		(GET_SAVE_BASE(ch, i) + GET_SAVE_MOD(ch, i))
#define GET_ALIGNMENT(ch)	((ch)->alignment)
#define GET_ETHIC_ALIGNMENT(ch)	((ch)->alignment_ethic)
#define SITS(ch)                ((ch)->sits)
#define MINDLINK(ch)            ((ch)->mindlink)
#define LINKER(ch)              ((ch)->linker)
#define LASTHIT(ch)             ((ch)->lasthit)
#define DRAGGING(ch)            ((ch)->drag)
#define DRAGGED(ch)             ((ch)->dragged)
#define GRAPPLING(ch)           ((ch)->grappling)
#define GRAPPLED(ch)            ((ch)->grappled)
#define GRAPTYPE(ch)            ((ch)->grap)
#define GET_ORIGINAL(ch)        ((ch)->original)
#define GET_CLONES(ch)          ((ch)->clones)
#define GET_DEFENDER(ch)        ((ch)->defender)
#define GET_DEFENDING(ch)       ((ch)->defending)
#define BLOCKS(ch)              ((ch)->blocks)
#define BLOCKED(ch)             ((ch)->blocked)
#define ABSORBING(ch)           ((ch)->absorbing)
#define ABSORBBY(ch)            ((ch)->absorbby)
#define GET_EAVESDROP(ch)       ((ch)->listenroom)
#define GET_EAVESDIR(ch)        ((ch)->eavesdir)
#define GET_ABSORBS(ch)         ((ch)->absorbs)
#define GET_LINTEREST(ch)       ((ch)->lastint)

#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->conditions[(i)]))
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_room))
#define GET_PRACTICES(ch,cl)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->class_skill_points[cl]))
#define GET_RACE_PRACTICES(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->skill_points))
#define GET_TRAINS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->ability_trains))
#define GET_TRAINSTR(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainstr))
#define GET_TRAININT(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainint))
#define GET_TRAINCON(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->traincon))
#define GET_TRAINWIS(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainwis))
#define GET_TRAINAGL(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainagl))
#define GET_TRAINSPD(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainspd))
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->invis_level))
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->wimp_level))
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->freeze_level))
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->bad_pws))
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->talks[i]))
#define POOFIN(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_OLC_ZONE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->olc_zone))
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define GET_HOST(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->host))
#define GET_HISTORY(ch, i)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->comm_hist[i]))

#define GET_SKILL_BONUS(ch, i)		(ch->skillmods[i])
#define GET_SKILL_PERF(ch, i)           (ch->skillperfs[i])
#define SET_SKILL_BONUS(ch, i, value)	do { (ch)->skillmods[i] = value; } while (0)
#define SET_SKILL_PERF(ch, i, value)    do { (ch)->skillperfs[i] = value; } while (0)
#define GET_SKILL_BASE(ch, i)		(ch->skills[i])
#define GET_SKILL(ch, i)		((ch)->skills[i] + GET_SKILL_BONUS(ch, i))
#define SET_SKILL(ch, i, val)		do { (ch)->skills[i] = val; } while(0)
#define BODY_PARTS(ch)  ((ch)->bodyparts)

#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_MOB_SPEC(ch)	(IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].vnum : NOBODY)

#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define MEMORY(ch)		((ch)->mob_specials.memory)
#define MOB_COOLDOWN(ch)        ((ch)->cooldown)

/* STRENGTH_APPLY_INDEX is no longer needed with the death of GET_ADD */
/* #define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch) ==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        ) */

#define CAN_CARRY_W(ch) (getMaxCarryWeight(ch))
#define CAN_CARRY_N(ch) (50)
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)) || (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 4 || GET_GENOME(ch, 1) == 4)) || PLR_FLAGGED(ch, PLR_AURALIGHT))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 50)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -50)
#define IS_LAWFUL(ch)   (GET_ETHIC_ALIGNMENT(ch) >= 350)
#define IS_CHAOTIC(ch)  (GET_ETHIC_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_ENEUTRAL(ch) (!IS_LAWFUL(ch) && !IS_CHAOTIC(ch))
#define ALIGN_TYPE(ch)	((IS_GOOD(ch) ? 0 : (IS_EVIL(ch) ? 6 : 3)) + \
                         (IS_LAWFUL(ch) ? 0 : (IS_CHAOTIC(ch) ? 2 : 1)))

#define IN_ARENA(ch)   (GET_ROOM_VNUM(IN_ROOM(ch)) >= 17800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 17874)
#define ARENA_IDNUM(ch) ((ch)->arenawatch)

/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define CHECK_WAIT(ch)                ((ch)->wait > 0)
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)

#define SENDOK(ch)    (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
                      (to_sleeping || AWAKE(ch)) && \
                      !PLR_FLAGGED((ch), PLR_WRITING))


#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && !PLR_FLAGGED(sub, PLR_EYEC) && \
   (IS_LIGHT(IN_ROOM(sub)) || AFF_FLAGGED((sub), AFF_INFRAVISION) || (IS_MUTANT(sub) && (GET_GENOME(sub, 0) == 4 || GET_GENOME(sub, 1) == 4)) || PLR_FLAGGED(sub, PLR_AURALIGHT)) )

#define INVIS_OK(sub, obj) \
 (!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED(sub,AFF_DETECT_INVIS))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_ADMLEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
   IMM_CAN_SEE(sub, obj) && (NOT_HIDDEN(obj) || GET_ADMLEVEL(sub) > 0)))

#define NOT_HIDDEN(ch) (!AFF_FLAGGED(ch, AFF_HIDE))
/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  (!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

#define MORT_CAN_SEE_OBJ(sub, obj) \
  ((LIGHT_OK(sub) || obj->carried_by == sub || obj->worn_by) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))

#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && !SITTING(obj) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define DISG(ch, vict) ((!PLR_FLAGGED(ch, PLR_DISGUISED)) || \
   (PLR_FLAGGED(ch, PLR_DISGUISED) && (GET_ADMLEVEL(vict) > 0 || IS_NPC(vict))))

#define INTROD(ch, vict) (ch == vict || readIntro(ch, vict) == 1 || (IS_NPC(vict) || IS_NPC(ch) || (GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(vict) > 0)))

#define ISWIZ(ch, vict) (ch == vict || GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(vict) > 0 || IS_NPC(vict) || IS_NPC(ch))

#define PERS(ch, vict) ((DISG(ch, vict) ? (CAN_SEE(vict, ch) ? (INTROD(vict, ch) ? (ISWIZ(ch, vict) ? GET_NAME(ch) :\
                        get_i_name(vict, ch)) : introd_calc(ch)) : "Someone") :\
                        TRUE_RACE(ch)))

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")

#define EXIT(ch, door)  (world[IN_ROOM(ch)].dir_option[door])
#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door]) 
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])


#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

#define RACE(ch)      (juggleRaceName(ch, true))
#define LRACE(ch)     (juggleRaceName(ch, false))
#define TRUE_RACE(ch) (pc_race_types[ch->race])
#define SENSEI_NAME(ch) (pc_class_types[ch->chclass])
#define SENSEI_NAME_LOWER(ch) (class_names[ch->chclass])
#define SENSEI_STYLE(ch) (sensei_style[ch->chclass])

#define CLASS_ABBR(ch) (class_abbrevs[ch->chclass])
#define RACE_ABBR(ch) (race_abbrevs[ch->race])

#define IS_ROSHI(ch)            (GET_CLASS(ch) == CLASS_ROSHI)
#define IS_PICCOLO(ch)          (GET_CLASS(ch) == CLASS_PICCOLO)
#define IS_KRANE(ch)            (GET_CLASS(ch) == CLASS_KRANE)
#define IS_NAIL(ch)             (GET_CLASS(ch) == CLASS_NAIL)
#define IS_BARDOCK(ch)          (GET_CLASS(ch) == CLASS_BARDOCK)
#define IS_GINYU(ch)            (GET_CLASS(ch) == CLASS_GINYU)
#define IS_FRIEZA(ch)           (GET_CLASS(ch) == CLASS_FRIEZA)
#define IS_TAPION(ch)           (GET_CLASS(ch) == CLASS_TAPION)
#define IS_ANDSIX(ch)           (GET_CLASS(ch) == CLASS_ANDSIX)
#define IS_DABURA(ch)           (GET_CLASS(ch) == CLASS_DABURA)
#define IS_KABITO(ch)           (GET_CLASS(ch) == CLASS_KABITO)
#define IS_JINTO(ch)            (GET_CLASS(ch) == CLASS_JINTO)
#define IS_TSUNA(ch)            (GET_CLASS(ch) == CLASS_TSUNA)
#define IS_KURZAK(ch)           (GET_CLASS(ch) == CLASS_KURZAK)

#define IS_ASSASSIN(ch)         (GET_CLASS_RANKS(ch, CLASS_ASSASSIN) > 0)
#define IS_BLACKGUARD(ch)       (GET_CLASS_RANKS(ch, CLASS_BLACKGUARD) > 0)
#define IS_DRAGON_DISCIPLE(ch)  (GET_CLASS_RANKS(ch, CLASS_DRAGON_DISCIPLE) > 0)
#define IS_DUELIST(ch)          (GET_CLASS_RANKS(ch, CLASS_DUELIST) > 0)
#define IS_DWARVEN_DEFENDER(ch) (GET_CLASS_RANKS(ch, CLASS_DWARVEN_DEFENDER) > 0)
#define IS_ELDRITCH_KNIGHT(ch)  (GET_CLASS_RANKS(ch, CLASS_ELDRITCH_KNIGHT) > 0)
#define IS_HIEROPHANT(ch)       (GET_CLASS_RANKS(ch, CLASS_HIEROPHANT) > 0)
#define IS_HORIZON_WALKER(ch)   (GET_CLASS_RANKS(ch, CLASS_HORIZON_WALKER) > 0)
#define IS_LOREMASTER(ch)       (GET_CLASS_RANKS(ch, CLASS_LOREMASTER) > 0)
#define IS_MYSTIC_THEURGE(ch)   (GET_CLASS_RANKS(ch, CLASS_MYSTIC_THEURGE) > 0)
#define IS_SHADOWDANCER(ch)     (GET_CLASS_RANKS(ch, CLASS_SHADOWDANCER) > 0)
#define IS_THAUMATURGIST(ch)    (GET_CLASS_RANKS(ch, CLASS_THAUMATURGIST) > 0)


#define GOLD_CARRY(ch)		(GET_LEVEL(ch) < 100 ? (GET_LEVEL(ch) < 50 ? GET_LEVEL(ch) * 10000 : 500000) : 50000000)
#define IS_SHADOW_DRAGON1(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON1_VNUM)
#define IS_SHADOW_DRAGON2(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON2_VNUM)
#define IS_SHADOW_DRAGON3(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON3_VNUM)
#define IS_SHADOW_DRAGON4(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON4_VNUM)
#define IS_SHADOW_DRAGON5(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON5_VNUM)
#define IS_SHADOW_DRAGON6(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON6_VNUM)
#define IS_SHADOW_DRAGON7(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON7_VNUM)
#define CAN_GRAND_MASTER(ch)    (IS_HUMAN(ch))
#define IS_HUMANOID(ch)         (!IS_SERPENT(ch) && !IS_ANIMAL(ch))
#define IS_ROBOT(ch)            (IS_ANDROID(ch) || IS_MECHANICAL(ch))
#define RESTRICTED_RACE(ch)     (IS_MAJIN(ch) || IS_SAIYAN(ch) || IS_BIO(ch) || IS_HOSHIJIN(ch))
#define CHEAP_RACE(ch)          (IS_TRUFFLE(ch) || IS_MUTANT(ch) || IS_KONATSU(ch) || IS_DEMON(ch) || IS_KANASSAN(ch))
#define SPAR_TRAIN(ch)          (FIGHTING(ch) && !IS_NPC(ch) && PLR_FLAGGED(ch, PLR_SPAR) &&\
                                 !IS_NPC(FIGHTING(ch)) && PLR_FLAGGED(FIGHTING(ch), PLR_SPAR))
#define IS_NONPTRANS(ch)        (IS_HUMAN(ch) || ((IS_SAIYAN(ch) || IS_HALFBREED(ch)) && !IS_FULLPSSJ(ch) && !PLR_FLAGGED(ch, PLR_LSSJ) && !PLR_FLAGGED(ch, PLR_OOZARU)) ||\
                                 IS_NAMEK(ch) || IS_MUTANT(ch) || IS_ICER(ch) ||\
                                 IS_KAI(ch) || IS_KONATSU(ch) || IS_DEMON(ch) || IS_KANASSAN(ch))
#define IS_FULLPSSJ(ch)             ((IS_SAIYAN(ch) && PLR_FLAGGED(ch, PLR_FPSSJ) && PLR_FLAGGED(ch, PLR_TRANS1)) ||\
                                 (IS_HALFBREED(ch) && PLR_FLAGGED(ch, PLR_FPSSJ) && PLR_FLAGGED(ch, PLR_TRANS1)))
#define IS_TRANSFORMED(ch)      (PLR_FLAGGED(ch, PLR_TRANS1) || PLR_FLAGGED(ch, PLR_TRANS2) ||\
                                 PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4) ||\
                                 PLR_FLAGGED(ch, PLR_TRANS5) || PLR_FLAGGED(ch, PLR_TRANS6) ||\
                                 PLR_FLAGGED(ch, PLR_OOZARU))
#define BIRTH_PHASE             (time_info.day <= 15)
#define LIFE_PHASE              (!BIRTH_PHASE && time_info.day <= 22)
#define DEATH_PHASE             (!BIRTH_PHASE && !LIFE_PHASE)
#define MOON_OK(ch)             (HAS_MOON(ch) && MOON_TIMECHECK() && OOZARU_OK(ch))
#define OOZARU_OK(ch)           (OOZARU_RACE(ch) && PLR_FLAGGED(ch, PLR_STAIL) && !IS_TRANSFORMED(ch))
#define OOZARU_RACE(ch)         (IS_SAIYAN(ch) || IS_HALFBREED(ch))
#define MOON_TIME               (time_info.hours >= 21 || time_info.hours <= 4)
#define MOON_DATE               (time_info.day == 19 || time_info.day == 20 || time_info.day == 21)
bool MOON_TIMECHECK();
#define ETHER_STREAM(ch)        (ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_AETHER) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NAMEK) || PLANET_ZENITH(IN_ROOM(ch)))
#define HAS_MOON(ch)            (ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH) ||\
                                 ROOM_FLAGGED(IN_ROOM(ch), ROOM_FRIGID) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_AETHER))
#define HAS_ARMS(ch)            (((IS_NPC(ch) && (MOB_FLAGGED(ch, MOB_LARM) || \
                                 MOB_FLAGGED(ch, MOB_RARM))) || GET_LIMBCOND(ch, 1) > 0 || \
                                 GET_LIMBCOND(ch, 2) > 0 || \
                                 PLR_FLAGGED(ch, PLR_CRARM) || \
                                 PLR_FLAGGED(ch, PLR_CLARM)) && \
                                 ((!GRAPPLING(ch) && !GRAPPLED(ch)) || \
                                 (GRAPPLING(ch) && GRAPTYPE(ch) == 3) || \
                                 (GRAPPLED(ch) && GRAPTYPE(ch) != 1 && GRAPTYPE(ch) != 4)))
#define HAS_LEGS(ch)            (((IS_NPC(ch) && (MOB_FLAGGED(ch, MOB_LLEG) || \
                                 MOB_FLAGGED(ch, MOB_RLEG))) || GET_LIMBCOND(ch, 3) > 0 || \
                                 GET_LIMBCOND(ch, 4) > 0 || \
                                 PLR_FLAGGED(ch, PLR_CRLEG) || \
                                 PLR_FLAGGED(ch, PLR_CLLEG)) && \
                                 ((!GRAPPLING(ch) && !GRAPPLED(ch)) || \
                                 (GRAPPLING(ch) && GRAPTYPE(ch) == 3) || \
                                 (GRAPPLED(ch) && GRAPTYPE(ch) != 1)))

#define IS_HUMAN(ch)            (GET_RACE(ch) == RACE_HUMAN)
#define IS_SAIYAN(ch)           (GET_RACE(ch) == RACE_SAIYAN)
#define IS_ICER(ch)             (GET_RACE(ch) == RACE_ICER)
#define IS_KONATSU(ch)          (GET_RACE(ch) == RACE_KONATSU)
#define IS_NAMEK(ch)            (GET_RACE(ch) == RACE_NAMEK)
#define IS_MUTANT(ch)           (GET_RACE(ch) == RACE_MUTANT)
#define IS_KANASSAN(ch)         (GET_RACE(ch) == RACE_KANASSAN)
#define IS_HALFBREED(ch)        (GET_RACE(ch) == RACE_HALFBREED)
#define IS_BIO(ch)              (GET_RACE(ch) == RACE_BIO)
#define IS_ANDROID(ch)          (GET_RACE(ch) == RACE_ANDROID)
#define IS_DEMON(ch)            (GET_RACE(ch) == RACE_DEMON)
#define IS_MAJIN(ch)            (GET_RACE(ch) == RACE_MAJIN)
#define IS_KAI(ch)              (GET_RACE(ch) == RACE_KAI)
#define IS_TRUFFLE(ch)          (GET_RACE(ch) == RACE_TRUFFLE)
#define IS_HOSHIJIN(ch)         (GET_RACE(ch) == RACE_HOSHIJIN)
#define IS_ANIMAL(ch)           (GET_RACE(ch) == RACE_ANIMAL)
#define IS_SAIBA(ch)              (GET_RACE(ch) == RACE_SAIBA)
#define IS_SERPENT(ch)            (GET_RACE(ch) == RACE_SERPENT)
#define IS_OGRE(ch)            (GET_RACE(ch) == RACE_OGRE)
#define IS_YARDRATIAN(ch)         (GET_RACE(ch) == RACE_YARDRATIAN)
#define IS_ARLIAN(ch)           (GET_RACE(ch) == RACE_ARLIAN)
#define IS_DRAGON(ch)           (GET_RACE(ch) == RACE_DRAGON)
#define IS_MECHANICAL(ch)          (GET_RACE(ch) == RACE_MECHANICAL)
#define IS_SPIRIT(ch)           (GET_RACE(ch) == RACE_SPIRIT)
#define IS_UNDEAD(ch)           (IS_AFFECTED(ch, AFF_UNDEAD))

#define IS_MALE(ch)             (GET_SEX(ch) == SEX_MALE)
#define IS_FEMALE(ch)           (GET_SEX(ch) == SEX_FEMALE)
#define IS_NEUTER(ch)           (!IS_MALE(ch) && !IS_FEMALE(ch))

#define OUTSIDE(ch)	(OUTSIDE_ROOMFLAG(ch) && OUTSIDE_SECTTYPE(ch))

#define OUTSIDE_ROOMFLAG(ch)	(!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS) && \
			 !ROOM_FLAGGED(IN_ROOM(ch), ROOM_UNDERGROUND) && \
                          !ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE))

#define OUTSIDE_SECTTYPE(ch)	((SECT(IN_ROOM(ch)) != SECT_INSIDE) && \
                         (SECT(IN_ROOM(ch)) != SECT_UNDERWATER) && \
                          (SECT(IN_ROOM(ch)) != SECT_IMPORTANT) && \
                           (SECT(IN_ROOM(ch)) != SECT_SHOP) && \
                            (SECT(IN_ROOM(ch)) != SECT_SPACE))

#define DIRT_ROOM(ch) (OUTSIDE_SECTTYPE(ch) && ((SECT(IN_ROOM(ch)) != SECT_WATER_NOSWIM) && \
                       (SECT(IN_ROOM(ch)) != SECT_WATER_SWIM)))

#define SPEAKING(ch)     ((ch)->player_specials->speaking)

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")
#define MAFE(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "male":"female") : "questionably gendered")


#define GET_SPELLMEM(ch, i)	((ch->player_specials->spellmem[i]))
#define GET_MEMCURSOR(ch)	((ch->player_specials->memcursor))
/* returns the number of spells per slot */
#define GET_SPELL_LEVEL(ch, i)	((ch)->player_specials->spell_level[i])
#define IS_ARCANE(ch)		(IS_WIZARD(ch))
#define IS_DIVINE(ch)		(IS_CLERIC(ch))
#define HAS_FEAT(ch, i)		((ch)->feats[i])
#define HAS_COMBAT_FEAT(ch,i,j)	IS_SET_AR((ch)->combat_feats[(i)], (j))
#define SET_COMBAT_FEAT(ch,i,j)	SET_BIT_AR((ch)->combat_feats[(i)], (j))
#define HAS_SCHOOL_FEAT(ch,i,j)	IS_SET((ch)->school_feats[(i)], (j))
#define SET_SCHOOL_FEAT(ch,i,j)	SET_BIT((ch)->school_feats[(i)], (j))
#define GET_BAB(ch)		GET_POLE_BONUS(ch)
#define SET_FEAT(ch, i, value)	do { CHECK_PLAYER_SPECIAL((ch), (ch)->feats[i]) = value; } while(0)
#define GET_SPELL_MASTERY_POINTS(ch) \
				(ch->player_specials->spell_mastery_points)
#define GET_FEAT_POINTS(ch)	(ch->player_specials->feat_points)
#define GET_EPIC_FEAT_POINTS(ch) \
				(ch->player_specials->epic_feat_points)
#define GET_CLASS_FEATS(ch,cl)	(ch->player_specials->class_feat_points[cl])
#define GET_EPIC_CLASS_FEATS(ch,cl) \
				(ch->player_specials->epic_class_feat_points[cl])
#define IS_EPIC_LEVEL(ch)	(GET_CLASS_LEVEL(ch) >= 20)
#define HAS_CRAFT_SKILL(ch,i,j)	IS_SET_AR((ch)->craft_skill[(i)], (j))
#define SET_CRAFT_SKILL(ch,i,j)	SET_BIT_AR((ch)->craft_skill[(i)], (j))
#define HAS_KNOWLEDGE_SKILL(ch,i,j)	IS_SET_AR((ch)->knowledge_skill[(i)], (j))
#define SET_KNOWLEDGE_SKILL(ch,i,j)	SET_BIT_AR((ch)->knowledge_skill[(i)], (j))
#define HAS_PROFESSION_SKILL(ch,i,j)	IS_SET_AR((ch)->profession_skill[(i)], (j))
#define SET_PROFESSION_SKILL(ch,i,j)	SET_BIT_AR((ch)->profession_skill[(i)], (j))

#define MOB_LOADROOM(ch)      ((ch)->hometown)  /*hometown not used for mobs*/
#define GET_MURDER(ch)          CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->murder))

#define GET_PAGE_LENGTH(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->page_length))
