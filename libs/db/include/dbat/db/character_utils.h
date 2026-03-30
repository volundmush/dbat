#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "characters.h"
#include "flags.h"

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

void char_set_class(struct char_data *ch, int class);
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

int *char_get_pref(const struct char_data *ch);
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
