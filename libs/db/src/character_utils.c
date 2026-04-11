#include "dbat/db/character_utils.h"
#include <stdlib.h>
#include <string.h>
#include "dbat/db/consts/types.h"
#include "dbat/db/consts/positions.h"
#include "dbat/db/consts/playerflags.h"
#include "dbat/db/consts/mobflags.h"
#include "dbat/db/consts/sex.h"
#include "dbat/db/consts/races.h"

// =============================================================================
// Classification (NPC/PC)
// =============================================================================

bool char_is_npc(const struct char_data *ch)
{
  return flag_test(ch->act, MOB_ISNPC);
}

bool char_is_mob(const struct char_data *ch)
{
  return char_is_npc(ch) && char_get_rnum(ch) != NOBODY;
}

mob_rnum char_get_rnum(const struct char_data *ch)
{
  return ch->nr;
}

mob_vnum char_get_vnum(const struct char_data *ch)
{
  if (char_is_mob(ch)) {
    return mob_index[ch->nr].vnum;
  }
  return NOBODY;
}

// =============================================================================
// Identity & Names
// =============================================================================

const char *char_get_name(const struct char_data *ch)
{
  return char_is_npc(ch) ? ch->short_descr : ch->name;
}

const char *char_get_pc_name(const struct char_data *ch)
{
  return ch->name;
}

const char *char_get_title(const struct char_data *ch)
{
  return NULL;
}

const char *char_get_short_desc(const struct char_data *ch)
{
  return ch->short_descr;
}

const char *char_get_long_desc(const struct char_data *ch)
{
  return ch->long_descr;
}

const char *char_get_description(const struct char_data *ch)
{
  return ch->description;
}

const char *char_get_clan(const struct char_data *ch)
{
  return ch->clan;
}

const char *char_get_voice(const struct char_data *ch)
{
  return ch->voice;
}

const char *char_get_feature(const struct char_data *ch)
{
  return ch->feature;
}

void char_set_name(struct char_data *ch, const char *name)
{
  if (ch->name) {
    free(ch->name);
  }
  ch->name = strdup(name);
}

void char_set_title(struct char_data *ch, const char *title)
{
  // NOTE: title is stored in descriptor_data, not char_data.
  // This function requires the full descriptor_data definition.
  // Use ch->desc->title directly or include appropriate header.
}

void char_set_clan(struct char_data *ch, const char *clan)
{
  if (ch->clan) {
    free(ch->clan);
  }
  ch->clan = clan ? strdup(clan) : NULL;
}

// =============================================================================
// Basic Attributes
// =============================================================================

int char_get_sex(const struct char_data *ch)
{
  return ch->sex;
}

void char_set_sex(struct char_data *ch, int sex)
{
  ch->sex = sex;
}

int char_get_size(const struct char_data *ch)
{
  return ch->size;
}

void char_set_size(struct char_data *ch, int size)
{
  ch->size = size;
}

int char_get_race(const struct char_data *ch)
{
  return ch->race;
}

void char_set_race(struct char_data *ch, int race)
{
  ch->race = race;
}

uint8_t char_get_weight(const struct char_data *ch)
{
  return ch->weight;
}

void char_set_weight(struct char_data *ch, uint8_t weight)
{
  ch->weight = weight;
}

uint8_t char_get_height(const struct char_data *ch)
{
  return ch->height;
}

void char_set_height(struct char_data *ch, uint8_t height)
{
  ch->height = height;
}

// =============================================================================
// Appearance
// =============================================================================

int char_get_hair_length(const struct char_data *ch)
{
  return ch->hairl;
}

int char_get_hair_style(const struct char_data *ch)
{
  return ch->hairs;
}

int char_get_hair_color(const struct char_data *ch)
{
  return ch->hairc;
}

int char_get_skin_color(const struct char_data *ch)
{
  return ch->skin;
}

int char_get_eye_color(const struct char_data *ch)
{
  return ch->eye;
}

int char_get_distinguishing_feature(const struct char_data *ch)
{
  return ch->distfea;
}

int char_get_aura(const struct char_data *ch)
{
  return ch->aura;
}

void char_set_hair_length(struct char_data *ch, int val)
{
  ch->hairl = val;
}

void char_set_hair_style(struct char_data *ch, int val)
{
  ch->hairs = val;
}

void char_set_hair_color(struct char_data *ch, int val)
{
  ch->hairc = val;
}

void char_set_skin_color(struct char_data *ch, int val)
{
  ch->skin = val;
}

void char_set_eye_color(struct char_data *ch, int val)
{
  ch->eye = val;
}

void char_set_distinguishing_feature(struct char_data *ch, int val)
{
  ch->distfea = val;
}

void char_set_aura(struct char_data *ch, int val)
{
  ch->aura = val;
}

// =============================================================================
// Position & Room
// =============================================================================

room_rnum char_get_in_room(const struct char_data *ch)
{
  return ch->in_room;
}

void char_set_in_room(struct char_data *ch, room_rnum room)
{
  ch->in_room = room;
}

room_rnum char_get_was_in_room(const struct char_data *ch)
{
  return ch->was_in_room;
}

void char_set_was_in_room(struct char_data *ch, room_rnum room)
{
  ch->was_in_room = room;
}

room_vnum char_get_home(const struct char_data *ch)
{
  return ch->hometown;
}

void char_set_home(struct char_data *ch, room_vnum home)
{
  ch->hometown = home;
}

int char_get_position(const struct char_data *ch)
{
  return ch->position;
}

void char_set_position(struct char_data *ch, int pos)
{
  ch->position = pos;
}

bool char_is_awake(const struct char_data *ch)
{
  return char_get_position(ch) > POS_SLEEPING;
}

bool char_is_dead(const struct char_data *ch)
{
  return char_plr_flagged(ch, PLR_NOTDEADYET) || char_mob_flagged(ch, MOB_NOTDEADYET);
}

int char_get_wait(const struct char_data *ch)
{
  return ch->wait;
}

void char_set_wait(struct char_data *ch, int wait)
{
  ch->wait = wait;
}

int char_get_timer(const struct char_data *ch)
{
  return ch->timer;
}

void char_set_timer(struct char_data *ch, int timer)
{
  ch->timer = timer;
}

int char_get_timer2(const struct char_data *ch)
{
  return ch->timer;
}

void char_set_timer2(struct char_data *ch, int timer)
{
  ch->timer = timer;
}

// =============================================================================
// Level & Class
// =============================================================================

int char_get_class_level(const struct char_data *ch)
{
  return ch->level;
}

int char_get_class(const struct char_data *ch)
{
  return ch->chclass;
}

int char_get_class_ranks(const struct char_data *ch, int class_num)
{
  return ch->chclasses[class_num] + ch->epicclasses[class_num];
}

int char_get_class_nonepic(const struct char_data *ch, int class_num)
{
  return ch->chclasses[class_num];
}

int char_get_class_epic(const struct char_data *ch, int class_num)
{
  return ch->epicclasses[class_num];
}

int char_get_race_level(const struct char_data *ch)
{
  return ch->race_level;
}

int char_get_level_adj(const struct char_data *ch)
{
  return ch->level_adj;
}

int char_get_level(const struct char_data *ch)
{
  return char_get_class_level(ch) + char_get_level_adj(ch) + char_get_race_level(ch);
}

int char_get_admlevel(const struct char_data *ch)
{
  return ch->admlevel;
}

int32_t char_get_idnum(const struct char_data *ch)
{
  return ch->idnum;
}

void char_set_class(struct char_data *ch, int class_id)
{
  ch->chclass = class_id;
}

void char_set_admlevel(struct char_data *ch, int level)
{
  ch->admlevel = level;
}

void char_set_level(struct char_data *ch, int level)
{
  ch->level = level;
}

// =============================================================================
// Core Stats
// =============================================================================

int8_t char_get_str(const struct char_data *ch)
{
  return ch->aff_abils.str;
}

int8_t char_get_dex(const struct char_data *ch)
{
  return ch->aff_abils.dex;
}

int8_t char_get_intel(const struct char_data *ch)
{
  return ch->aff_abils.intel;
}

int8_t char_get_wis(const struct char_data *ch)
{
  return ch->aff_abils.wis;
}

int8_t char_get_con(const struct char_data *ch)
{
  return ch->aff_abils.con;
}

int8_t char_get_cha(const struct char_data *ch)
{
  return ch->aff_abils.cha;
}

void char_set_str(struct char_data *ch, int8_t val)
{
  ch->aff_abils.str = val;
}

void char_set_dex(struct char_data *ch, int8_t val)
{
  ch->aff_abils.dex = val;
}

void char_set_intel(struct char_data *ch, int8_t val)
{
  ch->aff_abils.intel = val;
}

void char_set_wis(struct char_data *ch, int8_t val)
{
  ch->aff_abils.wis = val;
}

void char_set_con(struct char_data *ch, int8_t val)
{
  ch->aff_abils.con = val;
}

void char_set_cha(struct char_data *ch, int8_t val)
{
  ch->aff_abils.cha = val;
}

// =============================================================================
// Vital Stats (hit/mana/move/ki)
// =============================================================================

int64_t char_get_hit(const struct char_data *ch)
{
  return ch->hit;
}

int64_t char_get_max_hit(const struct char_data *ch)
{
  return ch->max_hit;
}

int64_t char_get_mana(const struct char_data *ch)
{
  return ch->mana;
}

int64_t char_get_max_mana(const struct char_data *ch)
{
  return ch->max_mana;
}

int64_t char_get_move(const struct char_data *ch)
{
  return ch->move;
}

int64_t char_get_max_move(const struct char_data *ch)
{
  return ch->max_move;
}

int64_t char_get_ki(const struct char_data *ch)
{
  return ch->ki;
}

int64_t char_get_max_ki(const struct char_data *ch)
{
  return ch->max_ki;
}

void char_set_hit(struct char_data *ch, int64_t val)
{
  ch->hit = val;
}

void char_inc_hit(struct char_data *ch, int64_t delta)
{
  ch->hit += delta;
}

void char_set_max_hit(struct char_data *ch, int64_t val)
{
  ch->max_hit = val;
}

void char_set_mana(struct char_data *ch, int64_t val)
{
  ch->mana = val;
}

void char_inc_mana(struct char_data *ch, int64_t delta)
{
  ch->mana += delta;
}

void char_set_max_mana(struct char_data *ch, int64_t val)
{
  ch->max_mana = val;
}

void char_set_move(struct char_data *ch, int64_t val)
{
  ch->move = val;
}

void char_inc_move(struct char_data *ch, int64_t delta)
{
  ch->move += delta;
}

void char_set_max_move(struct char_data *ch, int64_t val)
{
  ch->max_move = val;
}

void char_set_ki(struct char_data *ch, int64_t val)
{
  ch->ki = val;
}

void char_inc_ki(struct char_data *ch, int64_t delta)
{
  ch->ki += delta;
}

void char_set_max_ki(struct char_data *ch, int64_t val)
{
  ch->max_ki = val;
}

// =============================================================================
// Combat Stats
// =============================================================================

int char_get_armor(const struct char_data *ch)
{
  return ch->armor;
}

void char_set_armor(struct char_data *ch, int val)
{
  ch->armor = val;
}

int char_get_accuracy(const struct char_data *ch)
{
  return ch->accuracy;
}

void char_set_accuracy(struct char_data *ch, int val)
{
  ch->accuracy = val;
}

int char_get_accuracy_mod(const struct char_data *ch)
{
  return ch->accuracy_mod;
}

void char_set_accuracy_mod(struct char_data *ch, int val)
{
  ch->accuracy_mod = val;
}

int char_get_damage_mod(const struct char_data *ch)
{
  return ch->damage_mod;
}

void char_set_damage_mod(struct char_data *ch, int val)
{
  ch->damage_mod = val;
}

int char_get_alignment(const struct char_data *ch)
{
  return ch->alignment;
}

void char_set_alignment(struct char_data *ch, int val)
{
  ch->alignment = val;
}

int char_get_ethic_alignment(const struct char_data *ch)
{
  return ch->alignment_ethic;
}

void char_set_ethic_alignment(struct char_data *ch, int val)
{
  ch->alignment_ethic = val;
}

// =============================================================================
// Resources
// =============================================================================

int char_get_gold(const struct char_data *ch)
{
  return ch->gold;
}

void char_set_gold(struct char_data *ch, int val)
{
  ch->gold = val;
}

void char_inc_gold(struct char_data *ch, int delta)
{
  ch->gold += delta;
}

int char_get_bank_gold(const struct char_data *ch)
{
  return ch->bank_gold;
}

void char_set_bank_gold(struct char_data *ch, int val)
{
  ch->bank_gold = val;
}

void char_inc_bank_gold(struct char_data *ch, int delta)
{
  ch->bank_gold += delta;
}

int64_t char_get_exp(const struct char_data *ch)
{
  return ch->exp;
}

void char_set_exp(struct char_data *ch, int64_t val)
{
  ch->exp = val;
}

void char_inc_exp(struct char_data *ch, int64_t delta)
{
  ch->exp += delta;
}

// =============================================================================
// NPC-specific
// =============================================================================

int8_t char_get_attack_type(const struct char_data *ch)
{
  return ch->mob_specials.attack_type;
}

void char_set_attack_type(struct char_data *ch, int8_t type)
{
  ch->mob_specials.attack_type = type;
}

int8_t char_get_default_pos(const struct char_data *ch)
{
  return ch->mob_specials.default_pos;
}

void char_set_default_pos(struct char_data *ch, int8_t pos)
{
  ch->mob_specials.default_pos = pos;
}

int8_t char_get_damnodice(const struct char_data *ch)
{
  return ch->mob_specials.damnodice;
}

int8_t char_get_damsizedice(const struct char_data *ch)
{
  return ch->mob_specials.damsizedice;
}

// =============================================================================
// Limb/Body Parts
// =============================================================================

int char_get_limb(const struct char_data *ch, int limb)
{
  return ch->limbs[limb];
}

void char_set_limb(struct char_data *ch, int limb, int val)
{
  ch->limbs[limb] = val;
}

int char_get_limb_condition(const struct char_data *ch, int limb)
{
  return ch->limb_condition[limb];
}

void char_set_limb_condition(struct char_data *ch, int limb, int val)
{
  ch->limb_condition[limb] = val;
}

bool char_has_arms(const struct char_data *ch)
{
  if (char_is_npc(ch) && (char_mob_flagged(ch, MOB_LARM) || char_mob_flagged(ch, MOB_RARM))) {
    return true;
  }
  if (char_get_limb_condition(ch, 1) > 0 || char_get_limb_condition(ch, 2) > 0) {
    return true;
  }
  if (char_plr_flagged(ch, PLR_CRARM) || char_plr_flagged(ch, PLR_CLARM)) {
    return true;
  }
  return false;
}

bool char_has_legs(const struct char_data *ch)
{
  if (char_is_npc(ch) && (char_mob_flagged(ch, MOB_LLEG) || char_mob_flagged(ch, MOB_RLEG))) {
    return true;
  }
  if (char_get_limb_condition(ch, 3) > 0 || char_get_limb_condition(ch, 4) > 0) {
    return true;
  }
  if (char_plr_flagged(ch, PLR_CRLEG) || char_plr_flagged(ch, PLR_CLLEG)) {
    return true;
  }
  return false;
}

// =============================================================================
// Carrying
// =============================================================================

int char_get_carry_weight(const struct char_data *ch)
{
  return ch->carry_weight;
}

void char_set_carry_weight(struct char_data *ch, int weight)
{
  ch->carry_weight = weight;
}

void char_inc_carry_weight(struct char_data *ch, int delta)
{
  ch->carry_weight += delta;
}

int char_get_carry_items(const struct char_data *ch)
{
  return ch->carry_items;
}

void char_set_carry_items(struct char_data *ch, int items)
{
  ch->carry_items = items;
}

// =============================================================================
// Equipment
// =============================================================================

struct obj_data *char_get_equipment(const struct char_data *ch, int pos)
{
  return ch->equipment[pos];
}

void char_set_equipment(struct char_data *ch, int pos, struct obj_data *obj)
{
  ch->equipment[pos] = obj;
}

struct obj_data *char_get_carrying(const struct char_data *ch)
{
  return ch->carrying;
}

void char_set_carrying(struct char_data *ch, struct obj_data *obj)
{
  ch->carrying = obj;
}

// =============================================================================
// Fighting
// =============================================================================

struct char_data *char_get_fighting(const struct char_data *ch)
{
  return ch->fighting;
}

void char_set_fighting(struct char_data *ch, struct char_data *victim)
{
  ch->fighting = victim;
}

struct follow_type *char_get_followers(const struct char_data *ch)
{
  return ch->followers;
}

struct char_data *char_get_master(const struct char_data *ch)
{
  return ch->master;
}

void char_set_master(struct char_data *ch, struct char_data *master)
{
  ch->master = master;
}

int32_t char_get_master_id(const struct char_data *ch)
{
  return ch->master_id;
}

// =============================================================================
// List Links
// =============================================================================

struct char_data *char_get_next_in_room(const struct char_data *ch)
{
  return ch->next_in_room;
}

struct char_data *char_get_next(const struct char_data *ch)
{
  return ch->next;
}

struct char_data *char_get_next_fighting(const struct char_data *ch)
{
  return ch->next_fighting;
}

void char_set_next_in_room(struct char_data *ch, struct char_data *next)
{
  ch->next_in_room = next;
}

void char_set_next(struct char_data *ch, struct char_data *next)
{
  ch->next = next;
}

void char_set_next_fighting(struct char_data *ch, struct char_data *next)
{
  ch->next_fighting = next;
}

// =============================================================================
// Player Specials
// =============================================================================

static struct player_special_data *char_get_player_specials(const struct char_data *ch)
{
  if (ch->player_specials == &dummy_mob) {
    return NULL;
  }
  return ch->player_specials;
}

bitvector_t *char_get_pref(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->pref : NULL;
}

int char_get_cond(const struct char_data *ch, int cond)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->conditions[cond] : 0;
}

void char_set_cond(struct char_data *ch, int cond, int value)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->conditions[cond] = value;
  }
}

void char_inc_cond(struct char_data *ch, int cond, int delta)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->conditions[cond] += delta;
  }
}

int char_get_loadroom(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->load_room : NOWHERE;
}

void char_set_loadroom(struct char_data *ch, int room)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->load_room = room;
  }
}

int char_get_invis_level(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->invis_level : 0;
}

void char_set_invis_level(struct char_data *ch, int level)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->invis_level = level;
  }
}

int char_get_wimp_level(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->wimp_level : 0;
}

void char_set_wimp_level(struct char_data *ch, int level)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->wimp_level = level;
  }
}

int char_get_gauntlet(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->gauntlet : 0;
}

void char_set_gauntlet(struct char_data *ch, int val)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->gauntlet = val;
  }
}

int char_get_trains(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->ability_trains : 0;
}

void char_set_trains(struct char_data *ch, int val)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->ability_trains = val;
  }
}

void char_inc_trains(struct char_data *ch, int delta)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->ability_trains += delta;
  }
}

int char_get_olc_zone(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->olc_zone : 0;
}

void char_set_olc_zone(struct char_data *ch, int zone)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->olc_zone = zone;
  }
}

int char_get_speaking(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->speaking : 0;
}

void char_set_speaking(struct char_data *ch, int lang)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->speaking = lang;
  }
}

int char_get_racial_pref(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->racial_pref : 0;
}

void char_set_racial_pref(struct char_data *ch, int pref)
{
  if (ch->player_specials != &dummy_mob) {
    ch->player_specials->racial_pref = pref;
  }
}

const char *char_get_host(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->host : NULL;
}

void char_set_host(struct char_data *ch, const char *host)
{
  if (ch->player_specials != &dummy_mob) {
    if (ch->player_specials->host) {
      free(ch->player_specials->host);
    }
    ch->player_specials->host = host ? strdup(host) : NULL;
  }
}

const char *char_get_poofin(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->poofin : NULL;
}

void char_set_poofin(struct char_data *ch, const char *poof)
{
  if (ch->player_specials != &dummy_mob) {
    if (ch->player_specials->poofin) {
      free(ch->player_specials->poofin);
    }
    ch->player_specials->poofin = poof ? strdup(poof) : NULL;
  }
}

const char *char_get_poofout(const struct char_data *ch)
{
  struct player_special_data *ps = char_get_player_specials(ch);
  return ps ? ps->poofout : NULL;
}

void char_set_poofout(struct char_data *ch, const char *poof)
{
  if (ch->player_specials != &dummy_mob) {
    if (ch->player_specials->poofout) {
      free(ch->player_specials->poofout);
    }
    ch->player_specials->poofout = poof ? strdup(poof) : NULL;
  }
}

// =============================================================================
// Flags
// =============================================================================

bool char_mob_flagged(const struct char_data *ch, int flag)
{
  return char_is_npc(ch) && flag_test(ch->act, flag);
}

bool char_plr_flagged(const struct char_data *ch, int flag)
{
  return !char_is_npc(ch) && flag_test(ch->act, flag);
}

bool char_aff_flagged(const struct char_data *ch, int flag)
{
  return flag_test(ch->affected_by, flag);
}

bool char_prf_flagged(const struct char_data *ch, int flag)
{
  bitvector_t *pref = char_get_pref(ch);
  return pref && flag_test(pref, flag);
}

bool char_adm_flagged(const struct char_data *ch, int flag)
{
  return flag_test(ch->admflags, flag);
}

bool char_body_flagged(const struct char_data *ch, int flag)
{
  return flag_test(ch->bodyparts, flag);
}

void char_set_mob_flag(struct char_data *ch, int flag, bool value)
{
  flag_set(ch->act, flag, value);
}

void char_set_plr_flag(struct char_data *ch, int flag, bool value)
{
  flag_set(ch->act, flag, value);
}

void char_set_aff_flag(struct char_data *ch, int flag, bool value)
{
  flag_set(ch->affected_by, flag, value);
}

void char_set_prf_flag(struct char_data *ch, int flag, bool value)
{
  bitvector_t *pref = char_get_pref(ch);
  if (pref) {
    flag_set(pref, flag, value);
  }
}

void char_set_adm_flag(struct char_data *ch, int flag, bool value)
{
  flag_set(ch->admflags, flag, value);
}

void char_set_body_flag(struct char_data *ch, int flag, bool value)
{
  flag_set(ch->bodyparts, flag, value);
}

bool char_toggle_plr_flag(struct char_data *ch, int flag)
{
  bitvector_t *pref = char_get_pref(ch);
  return pref ? flag_toggle(pref, flag) : false;
}

bool char_toggle_aff_flag(struct char_data *ch, int flag)
{
  return flag_toggle(ch->affected_by, flag);
}

// =============================================================================
// Skills
// =============================================================================

int8_t char_get_skill(const struct char_data *ch, int skill)
{
  return ch->skills[skill] + ch->skillmods[skill];
}

int8_t char_get_skill_base(const struct char_data *ch, int skill)
{
  return ch->skills[skill];
}

int8_t char_get_skill_bonus(const struct char_data *ch, int skill)
{
  return ch->skillmods[skill];
}

int8_t char_get_skill_perf(const struct char_data *ch, int skill)
{
  return ch->skillperfs[skill];
}

void char_set_skill(struct char_data *ch, int skill, int8_t value)
{
  ch->skills[skill] = value;
}

void char_set_skill_bonus(struct char_data *ch, int skill, int8_t value)
{
  ch->skillmods[skill] = value;
}

void char_set_skill_perf(struct char_data *ch, int skill, int8_t value)
{
  ch->skillperfs[skill] = value;
}

void char_inc_skill(struct char_data *ch, int skill, int8_t delta)
{
  ch->skills[skill] += delta;
}

int8_t char_get_feat(const struct char_data *ch, int feat)
{
  return ch->feats[feat];
}

void char_set_feat(struct char_data *ch, int feat, int8_t value)
{
  ch->feats[feat] = value;
}

// =============================================================================
// Genome
// =============================================================================

int char_get_genome(const struct char_data *ch, int index)
{
  return ch->genome[index];
}

void char_set_genome(struct char_data *ch, int index, int val)
{
  ch->genome[index] = val;
}

// =============================================================================
// Transformation/Form
// =============================================================================

int char_get_mimic(const struct char_data *ch)
{
  return ch->mimic;
}

void char_set_mimic(struct char_data *ch, int val)
{
  ch->mimic = val;
}

int char_get_transform_class(const struct char_data *ch)
{
  return ch->transclass;
}

void char_set_transform_class(struct char_data *ch, int val)
{
  ch->transclass = val;
}

int char_get_personality(const struct char_data *ch)
{
  return ch->personality;
}

void char_set_personality(struct char_data *ch, int val)
{
  ch->personality = val;
}

int char_get_trans_cost(const struct char_data *ch, int index)
{
  return ch->transcost[index];
}

void char_set_trans_cost(struct char_data *ch, int index, int val)
{
  ch->transcost[index] = val;
}

int char_get_starphase(const struct char_data *ch)
{
  return ch->starphase;
}

void char_set_starphase(struct char_data *ch, int val)
{
  ch->starphase = val;
}

int char_get_fury(const struct char_data *ch)
{
  return ch->fury;
}

void char_set_fury(struct char_data *ch, short val)
{
  ch->fury = val;
}

int char_get_majinized(const struct char_data *ch)
{
  return ch->majinize;
}

void char_set_majinized(struct char_data *ch, int val)
{
  ch->majinize = val;
}

int char_get_speedboost(const struct char_data *ch)
{
  return ch->speedboost;
}

void char_set_speedboost(struct char_data *ch, int val)
{
  ch->speedboost = val;
}

int char_get_rage_meter(const struct char_data *ch)
{
  return ch->rage_meter;
}

void char_set_rage_meter(struct char_data *ch, int val)
{
  ch->rage_meter = val;
}

int char_get_tail_growth(const struct char_data *ch)
{
  return ch->tail_growth;
}

void char_set_tail_growth(struct char_data *ch, int val)
{
  ch->tail_growth = val;
}

int char_get_absorbs(const struct char_data *ch)
{
  return ch->absorbs;
}

void char_set_absorbs(struct char_data *ch, int val)
{
  ch->absorbs = val;
}

int char_get_boosts(const struct char_data *ch)
{
  return ch->boosts;
}

void char_set_boosts(struct char_data *ch, int val)
{
  ch->boosts = val;
}

int char_get_upgrade(const struct char_data *ch)
{
  return ch->upgrade;
}

void char_set_upgrade(struct char_data *ch, int val)
{
  ch->upgrade = val;
}

int char_get_kaioken(const struct char_data *ch)
{
  return ch->kaioken;
}

void char_set_kaioken(struct char_data *ch, int val)
{
  ch->kaioken = val;
}

// =============================================================================
// Boosts/Energy
// =============================================================================

int64_t char_get_base_pl(const struct char_data *ch)
{
  return ch->basepl;
}

void char_set_base_pl(struct char_data *ch, int64_t val)
{
  ch->basepl = val;
}

int64_t char_get_base_ki(const struct char_data *ch)
{
  return ch->baseki;
}

void char_set_base_ki(struct char_data *ch, int64_t val)
{
  ch->baseki = val;
}

int64_t char_get_base_st(const struct char_data *ch)
{
  return ch->basest;
}

void char_set_base_st(struct char_data *ch, int64_t val)
{
  ch->basest = val;
}

int64_t char_get_charge(const struct char_data *ch)
{
  return ch->charge;
}

void char_set_charge(struct char_data *ch, int64_t val)
{
  ch->charge = val;
}

int64_t char_get_chargeto(const struct char_data *ch)
{
  return ch->chargeto;
}

void char_set_chargeto(struct char_data *ch, int64_t val)
{
  ch->chargeto = val;
}

int64_t char_get_barrier(const struct char_data *ch)
{
  return ch->barrier;
}

void char_set_barrier(struct char_data *ch, int64_t val)
{
  ch->barrier = val;
}

int64_t char_get_suppression(const struct char_data *ch)
{
  return ch->suppression;
}

void char_set_suppression(struct char_data *ch, int64_t val)
{
  ch->suppression = val;
}

int64_t char_get_suppressed(const struct char_data *ch)
{
  return ch->suppressed;
}

void char_set_suppressed(struct char_data *ch, int64_t val)
{
  ch->suppressed = val;
}

int64_t char_get_lifeforce(const struct char_data *ch)
{
  return ch->lifeforce;
}

void char_set_lifeforce(struct char_data *ch, int64_t val)
{
  ch->lifeforce = val;
}

// =============================================================================
// Combat State
// =============================================================================

struct char_data *char_get_grappling(const struct char_data *ch)
{
  return ch->grappling;
}

void char_set_grappling(struct char_data *ch, struct char_data *victim)
{
  ch->grappling = victim;
}

struct char_data *char_get_grappled(const struct char_data *ch)
{
  return ch->grappled;
}

void char_set_grappled(struct char_data *ch, struct char_data *victim)
{
  ch->grappled = victim;
}

int char_get_grap_type(const struct char_data *ch)
{
  return ch->grap;
}

void char_set_grap_type(struct char_data *ch, int type)
{
  ch->grap = type;
}

struct char_data *char_get_blocks(const struct char_data *ch)
{
  return ch->blocks;
}

void char_set_blocks(struct char_data *ch, struct char_data *victim)
{
  ch->blocks = victim;
}

struct char_data *char_get_blocked(const struct char_data *ch)
{
  return ch->blocked;
}

void char_set_blocked(struct char_data *ch, struct char_data *victim)
{
  ch->blocked = victim;
}

struct char_data *char_get_absorbing(const struct char_data *ch)
{
  return ch->absorbing;
}

void char_set_absorbing(struct char_data *ch, struct char_data *victim)
{
  ch->absorbing = victim;
}

struct char_data *char_get_absorbedby(const struct char_data *ch)
{
  return ch->absorbby;
}

void char_set_absorbedby(struct char_data *ch, struct char_data *victim)
{
  ch->absorbby = victim;
}

struct char_data *char_get_defender(const struct char_data *ch)
{
  return ch->defender;
}

void char_set_defender(struct char_data *ch, struct char_data *victim)
{
  ch->defender = victim;
}

struct char_data *char_get_defending(const struct char_data *ch)
{
  return ch->defending;
}

void char_set_defending(struct char_data *ch, struct char_data *victim)
{
  ch->defending = victim;
}

struct char_data *char_get_drag(const struct char_data *ch)
{
  return ch->drag;
}

void char_set_drag(struct char_data *ch, struct char_data *victim)
{
  ch->drag = victim;
}

struct char_data *char_get_dragged(const struct char_data *ch)
{
  return ch->dragged;
}

void char_set_dragged(struct char_data *ch, struct char_data *victim)
{
  ch->dragged = victim;
}

struct char_data *char_get_original(const struct char_data *ch)
{
  return ch->original;
}

void char_set_original(struct char_data *ch, struct char_data *orig)
{
  ch->original = orig;
}

struct char_data *char_get_poisonby(const struct char_data *ch)
{
  return ch->poisonby;
}

void char_set_poisonby(struct char_data *ch, struct char_data *victim)
{
  ch->poisonby = victim;
}

// =============================================================================
// Grapple Helpers
// =============================================================================

bool char_is_grappling(const struct char_data *ch)
{
  return char_get_grappling(ch) != NULL;
}

bool char_is_grappled(const struct char_data *ch)
{
  return char_get_grappled(ch) != NULL;
}

// =============================================================================
// Descriptor (requires full descriptor_data definition - use with care)
// =============================================================================

// Note: char_get_desc and char_set_desc require the full descriptor_data
// definition which is not available in libs/db. Use ch->desc directly
// or include the appropriate header from libs/game.

// =============================================================================
// Radar/Ship
// =============================================================================

room_vnum char_get_radar1(const struct char_data *ch)
{
  return ch->radar1;
}

void char_set_radar1(struct char_data *ch, room_vnum vnum)
{
  ch->radar1 = vnum;
}

room_vnum char_get_radar2(const struct char_data *ch)
{
  return ch->radar2;
}

void char_set_radar2(struct char_data *ch, room_vnum vnum)
{
  ch->radar2 = vnum;
}

room_vnum char_get_radar3(const struct char_data *ch)
{
  return ch->radar3;
}

void char_set_radar3(struct char_data *ch, room_vnum vnum)
{
  ch->radar3 = vnum;
}

int char_get_ship(const struct char_data *ch)
{
  return ch->ship;
}

void char_set_ship(struct char_data *ch, int ship)
{
  ch->ship = ship;
}

room_vnum char_get_ship_room(const struct char_data *ch)
{
  return ch->shipr;
}

void char_set_ship_room(struct char_data *ch, room_vnum room)
{
  ch->shipr = room;
}

room_vnum char_get_droom(const struct char_data *ch)
{
  return ch->droom;
}

void char_set_droom(struct char_data *ch, room_vnum room)
{
  ch->droom = room;
}

// =============================================================================
// Time/Logs
// =============================================================================

time_t char_get_lastpl(const struct char_data *ch)
{
  return ch->lastpl;
}

void char_set_lastpl(struct char_data *ch, time_t t)
{
  ch->lastpl = t;
}

time_t char_get_deathtime(const struct char_data *ch)
{
  return ch->deathtime;
}

void char_set_deathtime(struct char_data *ch, time_t t)
{
  ch->deathtime = t;
}

time_t char_get_rewtime(const struct char_data *ch)
{
  return ch->rewtime;
}

void char_set_rewtime(struct char_data *ch, time_t t)
{
  ch->rewtime = t;
}

time_t char_get_lastint(const struct char_data *ch)
{
  return ch->lastint;
}

void char_set_lastint(struct char_data *ch, time_t t)
{
  ch->lastint = t;
}

const char *char_get_loguser(const struct char_data *ch)
{
  return ch->loguser;
}

void char_set_loguser(struct char_data *ch, const char *user)
{
  if (ch->loguser) {
    free(ch->loguser);
  }
  ch->loguser = user ? strdup(user) : NULL;
}

// =============================================================================
// Admin/Moderation
// =============================================================================

int char_get_arenawatch(const struct char_data *ch)
{
  return ch->arenawatch;
}

void char_set_arenawatch(struct char_data *ch, int val)
{
  ch->arenawatch = val;
}

bool char_is_admin(const struct char_data *ch, int min_level)
{
  return char_get_admlevel(ch) >= min_level;
}

// =============================================================================
// Game State
// =============================================================================

int char_get_death_type(const struct char_data *ch)
{
  return ch->death_type;
}

void char_set_death_type(struct char_data *ch, int type)
{
  ch->death_type = type;
}

int char_get_choice(const struct char_data *ch)
{
  return ch->choice;
}

void char_set_choice(struct char_data *ch, int choice)
{
  ch->choice = choice;
}

int char_get_sleeptime(const struct char_data *ch)
{
  return ch->sleeptime;
}

void char_set_sleeptime(struct char_data *ch, int val)
{
  ch->sleeptime = val;
}

int char_get_foodr(const struct char_data *ch)
{
  return ch->foodr;
}

void char_set_foodr(struct char_data *ch, int val)
{
  ch->foodr = val;
}

int char_get_altitude(const struct char_data *ch)
{
  return ch->altitude;
}

void char_set_altitude(struct char_data *ch, int val)
{
  ch->altitude = val;
}

int char_get_cranking(const struct char_data *ch)
{
  return ch->crank;
}

void char_set_cranking(struct char_data *ch, int val)
{
  ch->crank = val;
}

int char_get_overflow(const struct char_data *ch)
{
  return ch->overf;
}

void char_set_overflow(struct char_data *ch, int val)
{
  ch->overf = val;
}

int char_get_spam(const struct char_data *ch)
{
  return ch->spam;
}

void char_set_spam(struct char_data *ch, int val)
{
  ch->spam = val;
}

int char_get_cooldown(const struct char_data *ch)
{
  return ch->cooldown;
}

void char_set_cooldown(struct char_data *ch, int val)
{
  ch->cooldown = val;
}

int char_get_song(const struct char_data *ch)
{
  return ch->powerattack;
}

void char_set_song(struct char_data *ch, short song)
{
  ch->powerattack = song;
}

short char_get_clones(const struct char_data *ch)
{
  return ch->clones;
}

void char_set_clones(struct char_data *ch, short val)
{
  ch->clones = val;
}

int char_get_combine(const struct char_data *ch)
{
  return ch->combine;
}

void char_set_combine(struct char_data *ch, int val)
{
  ch->combine = val;
}

int char_get_linker(const struct char_data *ch)
{
  return ch->linker;
}

void char_set_linker(struct char_data *ch, int val)
{
  ch->linker = val;
}

int char_get_fish_state(const struct char_data *ch)
{
  return ch->fishstate;
}

void char_set_fish_state(struct char_data *ch, int val)
{
  ch->fishstate = val;
}

int char_get_throws(const struct char_data *ch)
{
  return ch->throws;
}

void char_set_throws(struct char_data *ch, int val)
{
  ch->throws = val;
}

int char_get_relax_count(const struct char_data *ch)
{
  return ch->relax_count;
}

void char_set_relax_count(struct char_data *ch, int val)
{
  ch->relax_count = val;
}

int char_get_preference(const struct char_data *ch)
{
  return ch->preference;
}

void char_set_preference(struct char_data *ch, int val)
{
  ch->preference = val;
}

int char_get_agg_timer(const struct char_data *ch)
{
  return ch->aggtimer;
}

void char_set_agg_timer(struct char_data *ch, int val)
{
  ch->aggtimer = val;
}

// =============================================================================
// Experience/Leveling
// =============================================================================

int64_t char_get_moltexp(const struct char_data *ch)
{
  return ch->moltexp;
}

void char_set_moltexp(struct char_data *ch, int64_t val)
{
  ch->moltexp = val;
}

int char_get_moltlevel(const struct char_data *ch)
{
  return ch->moltlevel;
}

void char_set_moltlevel(struct char_data *ch, int val)
{
  ch->moltlevel = val;
}

int64_t char_get_majinizer(const struct char_data *ch)
{
  return ch->majinizer;
}

void char_set_majinizer(struct char_data *ch, int64_t val)
{
  ch->majinizer = val;
}

int char_get_skill_slots(const struct char_data *ch)
{
  return ch->skill_slots;
}

void char_set_skill_slots(struct char_data *ch, int val)
{
  ch->skill_slots = val;
}

// =============================================================================
// Board/History
// =============================================================================

int char_get_board(const struct char_data *ch, int index)
{
  return ch->lboard[index];
}

void char_set_board(struct char_data *ch, int index, int val)
{
  ch->lboard[index] = val;
}

int char_get_lastattack(const struct char_data *ch)
{
  return ch->lasthit;
}

void char_set_lastattack(struct char_data *ch, int val)
{
  ch->lasthit = val;
}

int char_get_combhits(const struct char_data *ch)
{
  return ch->combhits;
}

void char_set_combhits(struct char_data *ch, int val)
{
  ch->combhits = val;
}

int char_get_combo(const struct char_data *ch)
{
  return ch->combo;
}

void char_set_combo(struct char_data *ch, int val)
{
  ch->combo = val;
}

int char_get_ping(const struct char_data *ch)
{
  return ch->ping;
}

void char_set_ping(struct char_data *ch, int val)
{
  ch->ping = val;
}

// =============================================================================
// Saving Throws
// =============================================================================

int char_get_save_base(const struct char_data *ch, int type)
{
  return ch->saving_throw[type];
}

void char_set_save_base(struct char_data *ch, int type, int16_t val)
{
  ch->saving_throw[type] = val;
}

int char_get_save_mod(const struct char_data *ch, int type)
{
  return ch->apply_saving_throw[type];
}

void char_set_save_mod(struct char_data *ch, int type, int16_t val)
{
  ch->apply_saving_throw[type] = val;
}

// =============================================================================
// Spell/Magic
// =============================================================================

int16_t char_get_spellfail(const struct char_data *ch)
{
  return ch->spellfail;
}

void char_set_spellfail(struct char_data *ch, int16_t val)
{
  ch->spellfail = val;
}

int16_t char_get_armorcheck(const struct char_data *ch)
{
  return ch->armorcheck;
}

void char_set_armorcheck(struct char_data *ch, int16_t val)
{
  ch->armorcheck = val;
}

int16_t char_get_armorcheckall(const struct char_data *ch)
{
  return ch->armorcheckall;
}

void char_set_armorcheckall(struct char_data *ch, int16_t val)
{
  ch->armorcheckall = val;
}

// =============================================================================
// Misc Bonuses
// =============================================================================

int char_get_bonus(const struct char_data *ch, int index)
{
  return ch->bonuses[index];
}

void char_set_bonus(struct char_data *ch, int index, int val)
{
  ch->bonuses[index] = val;
}

int char_get_ccpoints(const struct char_data *ch)
{
  return ch->ccpoints;
}

void char_set_ccpoints(struct char_data *ch, int val)
{
  ch->ccpoints = val;
}

int char_get_negcount(const struct char_data *ch)
{
  return ch->negcount;
}

void char_set_negcount(struct char_data *ch, int val)
{
  ch->negcount = val;
}

int char_get_lifebonus(const struct char_data *ch)
{
  return ch->lifebonus;
}

void char_set_lifebonus(struct char_data *ch, int val)
{
  ch->lifebonus = val;
}

int char_get_asb(const struct char_data *ch)
{
  return ch->asb;
}

void char_set_asb(struct char_data *ch, int val)
{
  ch->asb = val;
}

int char_get_regen(const struct char_data *ch)
{
  return ch->regen;
}

void char_set_regen(struct char_data *ch, int val)
{
  ch->regen = val;
}

int char_get_rbank(const struct char_data *ch)
{
  return ch->rbank;
}

void char_set_rbank(struct char_data *ch, int val)
{
  ch->rbank = val;
}

int char_get_lifepperc(const struct char_data *ch)
{
  return ch->lifeperc;
}

void char_set_lifepperc(struct char_data *ch, int val)
{
  ch->lifeperc = val;
}

int char_get_blesslvl(const struct char_data *ch)
{
  return ch->blesslvl;
}

void char_set_blesslvl(struct char_data *ch, int val)
{
  ch->blesslvl = val;
}

int char_get_gooptime(const struct char_data *ch)
{
  return ch->gooptime;
}

void char_set_gooptime(struct char_data *ch, int val)
{
  ch->gooptime = val;
}

int char_get_sdc_cooldown(const struct char_data *ch)
{
  return ch->con_sdcooldown;
}

void char_set_sdc_cooldown(struct char_data *ch, int val)
{
  ch->con_sdcooldown = val;
}

int char_get_backstab_cooldown(const struct char_data *ch)
{
  return ch->backstabcool;
}

void char_set_backstab_cooldown(struct char_data *ch, int val)
{
  ch->backstabcool = val;
}

int char_get_con_cooldown(const struct char_data *ch)
{
  return ch->con_cooldown;
}

void char_set_con_cooldown(struct char_data *ch, int val)
{
  ch->con_cooldown = val;
}

int char_get_forgetting(const struct char_data *ch)
{
  return ch->forgeting;
}

void char_set_forgetting(struct char_data *ch, int val)
{
  ch->forgeting = val;
}

int char_get_forgetcount(const struct char_data *ch)
{
  return ch->forgetcount;
}

void char_set_forgetcount(struct char_data *ch, int val)
{
  ch->forgetcount = val;
}

short char_get_stupidkiss(const struct char_data *ch)
{
  return ch->stupidkiss;
}

void char_set_stupidkiss(struct char_data *ch, short val)
{
  ch->stupidkiss = val;
}

int char_get_btime(const struct char_data *ch)
{
  return ch->btime;
}

void char_set_btime(struct char_data *ch, short val)
{
  ch->btime = val;
}

int char_get_armor_last(const struct char_data *ch)
{
  return ch->armor_last;
}

void char_set_armor_last(struct char_data *ch, int val)
{
  ch->armor_last = val;
}

int char_get_ingest_learned(const struct char_data *ch)
{
  return ch->ingestLearned;
}

void char_set_ingest_learned(struct char_data *ch, int val)
{
  ch->ingestLearned = val;
}

// =============================================================================
// RP/Social
// =============================================================================

int char_get_rp(const struct char_data *ch)
{
  return ch->rp;
}

void char_set_rp(struct char_data *ch, int val)
{
  ch->rp = val;
}

int char_get_trp(const struct char_data *ch)
{
  return ch->trp;
}

void char_set_trp(struct char_data *ch, int val)
{
  ch->trp = val;
}

int char_get_eavesdrop(const struct char_data *ch)
{
  return ch->listenroom;
}

void char_set_eavesdrop(struct char_data *ch, int room)
{
  ch->listenroom = room;
}

int char_get_eavesdir(const struct char_data *ch)
{
  return ch->eavesdir;
}

void char_set_eavesdir(struct char_data *ch, int dir)
{
  ch->eavesdir = dir;
}

int char_get_mob_charge(const struct char_data *ch)
{
  return ch->mobcharge;
}

void char_set_mob_charge(struct char_data *ch, int val)
{
  ch->mobcharge = val;
}

// =============================================================================
// Display/UI
// =============================================================================

const char *char_get_rdisplay(const struct char_data *ch)
{
  return ch->rdisplay;
}

void char_set_rdisplay(struct char_data *ch, const char *display)
{
  if (ch->rdisplay) {
    free(ch->rdisplay);
  }
  ch->rdisplay = display ? strdup(display) : NULL;
}

const char *char_get_temp_prompt(const struct char_data *ch)
{
  return ch->temp_prompt;
}

void char_set_temp_prompt(struct char_data *ch, const char *prompt)
{
  if (ch->temp_prompt) {
    free(ch->temp_prompt);
  }
  ch->temp_prompt = prompt ? strdup(prompt) : NULL;
}

// =============================================================================
// Special/Memory
// =============================================================================

memory_rec *char_get_memory(const struct char_data *ch)
{
  return ch->mob_specials.memory;
}

void char_set_memory(struct char_data *ch, memory_rec *mem)
{
  ch->mob_specials.memory = mem;
}

// =============================================================================
// Listening
// =============================================================================

room_vnum char_get_listenroom(const struct char_data *ch)
{
  return ch->listenroom;
}

void char_set_listenroom(struct char_data *ch, room_vnum room)
{
  ch->listenroom = room;
}

// =============================================================================
// Waiting/Special States
// =============================================================================

int char_get_powerattack(const struct char_data *ch)
{
  return ch->powerattack;
}

void char_set_powerattack(struct char_data *ch, int val)
{
  ch->powerattack = val;
}

int char_get_combat_expertise(const struct char_data *ch)
{
  return ch->combatexpertise;
}

void char_set_combat_expertise(struct char_data *ch, int val)
{
  ch->combatexpertise = val;
}

int char_get_cooldown_mob(const struct char_data *ch)
{
  return ch->cooldown;
}

void char_set_cooldown_mob(struct char_data *ch, int val)
{
  ch->cooldown = val;
}

// =============================================================================
// ID
// =============================================================================

int32_t char_get_id(const struct char_data *ch)
{
  return ch->id;
}

void char_set_id(struct char_data *ch, int32_t id)
{
  ch->id = id;
}

// =============================================================================
// File Position
// =============================================================================

int char_get_pfilepos(const struct char_data *ch)
{
  return ch->pfilepos;
}

void char_set_pfilepos(struct char_data *ch, int pos)
{
  ch->pfilepos = pos;
}

// =============================================================================
// Life/Death
// =============================================================================

const struct time_data *char_get_time(const struct char_data *ch)
{
  return &ch->time;
}

int char_get_lifeperc(const struct char_data *ch)
{
  return ch->lifeperc;
}

void char_set_lifeperc(struct char_data *ch, int val)
{
  ch->lifeperc = val;
}

// =============================================================================
// NewItem (NPC)
// =============================================================================

int char_get_newitem(const struct char_data *ch)
{
  return ch->mob_specials.newitem;
}

void char_set_newitem(struct char_data *ch, int val)
{
  ch->mob_specials.newitem = val;
}

// =============================================================================
// Races & Classes (Predicates)
// =============================================================================

bool char_is_race(const struct char_data *ch, int race_id)
{
  return char_get_race(ch) == race_id;
}

bool char_is_class(const struct char_data *ch, int class_id)
{
  return char_get_class(ch) == class_id;
}

bool char_has_class_rank(const struct char_data *ch, int class_num)
{
  return char_get_class_ranks(ch, class_num) > 0;
}

bool char_is_male(const struct char_data *ch)
{
  return char_get_sex(ch) == SEX_MALE;
}

bool char_is_female(const struct char_data *ch)
{
  return char_get_sex(ch) == SEX_FEMALE;
}

bool char_is_neuter(const struct char_data *ch)
{
  return !char_is_male(ch) && !char_is_female(ch);
}

// =============================================================================
// Alignment (Predicates)
// =============================================================================

bool char_is_good(const struct char_data *ch)
{
  return char_get_alignment(ch) >= 50;
}

bool char_is_evil(const struct char_data *ch)
{
  return char_get_alignment(ch) <= -50;
}

bool char_is_neutral(const struct char_data *ch)
{
  return !char_is_good(ch) && !char_is_evil(ch);
}

bool char_is_lawful(const struct char_data *ch)
{
  return char_get_ethic_alignment(ch) >= 350;
}

bool char_is_chaotic(const struct char_data *ch)
{
  return char_get_ethic_alignment(ch) <= -350;
}

bool char_is_eneutral(const struct char_data *ch)
{
  return !char_is_lawful(ch) && !char_is_chaotic(ch);
}

// =============================================================================
// Special Races
// =============================================================================

bool char_is_inferior(const struct char_data *ch)
{
  return char_is_race(ch, RACE_KONATSU) || char_is_race(ch, RACE_DEMON);
}

bool char_is_oozaru_race(const struct char_data *ch)
{
  return char_is_race(ch, RACE_SAIYAN) || char_is_race(ch, RACE_HALFBREED);
}

bool char_is_transformed(const struct char_data *ch)
{
  return char_plr_flagged(ch, PLR_TRANS1) || char_plr_flagged(ch, PLR_TRANS2) ||
         char_plr_flagged(ch, PLR_TRANS3) || char_plr_flagged(ch, PLR_TRANS4) ||
         char_plr_flagged(ch, PLR_TRANS5) || char_plr_flagged(ch, PLR_TRANS6) ||
         char_plr_flagged(ch, PLR_OOZARU);
}

bool char_is_fullpssj(const struct char_data *ch)
{
  bool is_saiyan_half = (char_is_race(ch, RACE_SAIYAN) || char_is_race(ch, RACE_HALFBREED));
  return is_saiyan_half && char_plr_flagged(ch, PLR_FPSSJ) && char_plr_flagged(ch, PLR_TRANS1);
}

bool char_is_nonptrans(const struct char_data *ch)
{
  return char_is_race(ch, RACE_HUMAN) ||
         (char_is_oozaru_race(ch) && !char_is_fullpssj(ch) &&
          !char_plr_flagged(ch, PLR_LSSJ) && !char_plr_flagged(ch, PLR_OOZARU)) ||
         char_is_race(ch, RACE_NAMEK) || char_is_race(ch, RACE_MUTANT) ||
         char_is_race(ch, RACE_ICER) || char_is_race(ch, RACE_KAI) ||
         char_is_race(ch, RACE_KONATSU) || char_is_race(ch, RACE_DEMON) ||
         char_is_race(ch, RACE_KANASSAN);
}

bool char_is_humanoid(const struct char_data *ch)
{
  return !char_is_race(ch, RACE_SERPENT) && !char_is_race(ch, RACE_ANIMAL);
}

// =============================================================================
// Unused/wrapped fields
// =============================================================================

int char_get_spoiled(const struct char_data *ch)
{
  return char_get_time(ch)->played > 86400;
}
