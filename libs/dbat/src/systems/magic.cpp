/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/iterate.hpp"
#include "dbat/consts/aligns.hpp"
#include "dbat/config.hpp"
#include "dbat/magic.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/feats.hpp"
#include "dbat/mobact.hpp"
#include "dbat/fight.hpp"
#include "dbat/affect.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static void perform_mag_groups(GameWorld *g, int level, struct char_data *ch, struct char_data *tch, int spellnum);

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(GameWorld *g)
{
  for (char_data *i : each_affected(g)) {
    for (auto it = char_affects(i).begin(); it != char_affects(i).end();) {
      struct affected_type *af         = &*it++;
      const struct affected_type *next = it == char_affects(i).end() ? nullptr : &*it;
      if (af->duration >= 1)
        af->duration--;
      else if (af->duration == 0) {
        if (af->type > 0)
          if (!next || (next->type != af->type) || (next->duration > 0)) {
            if (g->spell_info[af->type].wear_off_msg)
              send_to_char(g, i, "%s\r\n", g->spell_info[af->type].wear_off_msg);
            if (char_get_speedboost(i) > 0 && af->type == SPELL_HAYASA) {
              char_set_speedboost(i, 0);
            }
          }
        affect_remove(g, i, af);
      }
    }
  }
}

/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */

bool mag_newsaves(struct char_data *ch, struct char_data *victim, int spellnum, int level, int cast_stat)
{
  return false;
}

/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
int mag_damage(int level, struct char_data *ch, struct char_data *victim,
               int spellnum)
{
  return 0;
}

/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
 */

constexpr int MAX_SPELL_AFFECTS = 5; /* change if more needed */

void mag_affects(GameWorld *g, int level, struct char_data *ch, struct char_data *victim,
                 int spellnum)
{
  struct affected_type af[MAX_SPELL_AFFECTS] = {};
  bool accum_affect = false, accum_duration = false;
  const char *to_vict = nullptr, *to_room = nullptr;
  int i;

  if (victim == nullptr || ch == nullptr)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type      = spellnum;
    af[i].bitvector = 0;
    af[i].modifier  = 0;
    af[i].location  = APPLY_NONE;
  }

  if (mag_newsaves(ch, victim, spellnum, level, char_get_aff_abils_intel(ch))) {
    if (IS_SET(g->spell_info[spellnum].save_flags, MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
      send_to_char(g, victim, "@g*save*@y You avoid any lasting affects.@n\r\n");
      return;
    }
  }

  switch (spellnum) {

    case SPELL_CHILL_TOUCH:
      af[0].location = APPLY_STR;
      af[0].duration = 24;
      af[0].modifier = -1;
      accum_duration = true;
      to_vict        = "You feel your strength wither!";
      break;

    case SPELL_MAGE_ARMOR:
      af[0].location = APPLY_AC;
      af[0].modifier = 40;
      af[0].duration = 1 * char_get_level(ch);
      accum_duration = false;
      to_vict        = "You feel someone protecting you.";
      break;

    case SPELL_BLESS:
      af[0].location = APPLY_ACCURACY;
      af[0].modifier = 2;
      af[0].duration = 6;

      af[1].location = APPLY_WILL;
      af[1].modifier = 1;
      af[1].duration = 6;

      accum_duration = true;
      to_vict        = "You feel righteous.";
      break;

    case SPELL_BLINDNESS:
      if (char_mob_flagged(victim, MOB_NOBLIND)) {
        send_to_char(g, ch, "You fail.\r\n");
        return;
      }

      af[0].location  = APPLY_ACCURACY;
      af[0].modifier  = -4;
      af[0].duration  = 2;
      af[0].bitvector = AFF_BLIND;

      af[1].location  = APPLY_AC;
      af[1].modifier  = -4;
      af[1].duration  = 2;
      af[1].bitvector = AFF_BLIND;

      to_room = "$n seems to be blinded!";
      to_vict = "You have been blinded!";
      break;

    case SPELL_BANE:
      af[0].location  = APPLY_ACCURACY;
      af[0].duration  = 1 + (level / 2);
      af[0].modifier  = -1;
      af[0].bitvector = AFF_CURSE;

      af[1].location  = APPLY_WILL;
      af[1].duration  = 1 + (level / 2);
      af[1].modifier  = -1;
      af[1].bitvector = AFF_CURSE;

      accum_duration = true;
      accum_affect   = true;
      to_room        = "$n briefly glows red!";
      to_vict        = "You feel very uncomfortable.";
      break;

    case SPELL_BESTOW_CURSE:
      af[0].location  = APPLY_STR;
      af[0].duration  = -1;
      af[0].modifier  = -6;
      af[0].bitvector = AFF_CURSE;

      af[1].location  = APPLY_ACCURACY;
      af[1].duration  = -1;
      af[1].modifier  = -4;
      af[1].bitvector = AFF_CURSE;

      accum_duration = false;
      accum_affect   = false;
      to_room        = "$n briefly glows red!";
      to_vict        = "You feel very uncomfortable.";
      break;

    case SPELL_DETECT_ALIGN:
      af[0].duration  = 12 + level;
      af[0].bitvector = AFF_DETECT_ALIGN;
      accum_duration  = true;
      to_vict         = "Your eyes tingle.";
      break;

    case SPELL_SEE_INVIS:
      af[0].duration  = 12 + level;
      af[0].bitvector = AFF_DETECT_INVIS;
      accum_duration  = true;
      to_vict         = "Your eyes tingle.";
      break;

    case SPELL_DETECT_MAGIC:
      af[0].duration  = 12 + level;
      af[0].bitvector = AFF_DETECT_MAGIC;
      accum_duration  = true;
      to_vict         = "Your eyes tingle.";
      break;

    case SPELL_FAERIE_FIRE:
      af[0].location = APPLY_AC;
      af[0].modifier = -1; /*should make target easier to hit */
      af[0].duration = 3;
      accum_duration = false;
      to_vict        = "Your body flickers with a purplish light.";
      to_room        = "$n's body flickers with with a purplish light.";
      break;

    case SPELL_DARKVISION:
      af[0].duration  = 12 + level;
      af[0].bitvector = AFF_INFRAVISION;
      accum_duration  = true;
      to_vict         = "Your eyes glow red.";
      to_room         = "$n's eyes glow red.";
      break;

    case SPELL_INVISIBLE:
      if (!victim)
        victim = ch;

      af[0].duration  = 12 + (level / 4);
      af[0].modifier  = 4;
      af[0].location  = APPLY_AC;
      af[0].bitvector = AFF_INVISIBLE;
      accum_duration  = true;
      to_vict         = "You vanish.";
      to_room         = "$n slowly fades out of existence.";
      break;

    case SPELL_POISON:

      af[0].location  = APPLY_STR;
      af[0].duration  = level;
      af[0].modifier  = -2;
      af[0].bitvector = AFF_POISON;
      to_vict         = "You feel very sick.";
      to_room         = "$n gets violently ill!";
      break;

    case SPELL_PROT_FROM_EVIL:
      af[0].duration  = 24;
      af[0].bitvector = AFF_PROTECT_GOOD;
      accum_duration  = true;
      to_vict         = "You feel invulnerable!";
      break;

    case SPELL_SANCTUARY:
      af[0].duration  = 4;
      af[0].bitvector = AFF_SANCTUARY;

      accum_duration = true;
      to_vict        = "A white aura momentarily surrounds you.";
      to_room        = "$n is surrounded by a white aura.";
      break;

    case SPELL_SLEEP:
      if (!CONFIG_PK_ALLOWED && !char_is_npc(ch) && !char_is_npc(victim))
        return;
      if (char_mob_flagged(victim, MOB_NOSLEEP))
        return;

      af[0].duration  = 4 + (level / 4);
      af[0].bitvector = AFF_SLEEP;

      if (char_get_position(victim) > POS_SLEEPING) {
        send_to_char(g, victim, "You feel very sleepy...  Zzzz......\r\n");
        act(g, "$n goes to sleep.", true, victim, nullptr, nullptr, TO_ROOM);
        char_set_position(victim, POS_SLEEPING);
      }
      break;

    case SPELL_HAYASA:
      if (!CONFIG_PK_ALLOWED && !char_is_npc(ch) && !char_is_npc(victim))
        return;
      if (char_mob_flagged(victim, MOB_NOSLEEP))
        return;

      af[0].duration  = 4 + (level / 4);
      af[0].bitvector = AFF_SLEEP;

      if (char_get_position(victim) > POS_SLEEPING) {
        send_to_char(g, victim, "You feel very sleepy...  Zzzz......\r\n");
        act(g, "$n goes to sleep.", true, victim, nullptr, nullptr, TO_ROOM);
        char_set_position(victim, POS_SLEEPING);
      }
      break;

    case SPELL_BULL_STRENGTH:
      af[0].location = APPLY_STR;
      af[0].duration = level;
      af[0].modifier = 1 + (rand_number(g, 1, 4));
      accum_duration = false;
      accum_affect   = false;
      to_vict        = "You feel stronger!";
      break;

    case SPELL_SENSE_LIFE:
      to_vict         = "Your feel your awareness improve.";
      af[0].duration  = level;
      af[0].bitvector = AFF_SENSE_LIFE;
      accum_duration  = true;
      break;

    case SPELL_WATERWALK:
      af[0].duration  = 24;
      af[0].bitvector = AFF_WATERWALK;
      accum_duration  = true;
      to_vict         = "You feel webbing between your toes.";
      break;

    case SPELL_STONESKIN:
      af[0].duration = 1 * char_get_level(ch);
      accum_duration = false;
      to_vict        = "Your skin hardens into stone!";
      break;
  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (char_is_npc(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (char_aff_flagged(victim, af[i].bitvector)) {
        send_to_char(g, ch, "%s", CONFIG_NOEFFECT.c_str());
        return;
      }

  /*
   * If the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim, spellnum) && !(accum_duration || accum_affect)) {
    send_to_char(g, ch, "%s", CONFIG_NOEFFECT.c_str());
    return;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector || (af[i].location != APPLY_NONE))
      affect_join(g, victim, af + i, accum_duration, false, accum_affect, false);

  if (to_vict != nullptr)
    act(g, to_vict, false, victim, nullptr, ch, TO_CHAR);
  if (to_room != nullptr)
    act(g, to_room, true, victim, nullptr, ch, TO_ROOM);
}

/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */
static void perform_mag_groups(GameWorld *g, int level, struct char_data *ch,
                               struct char_data *tch, int spellnum)
{
  switch (spellnum) {
    case SPELL_MASS_HEAL:
      mag_points(g, level, ch, tch, SPELL_HEAL);
      break;
    case SPELL_GROUP_ARMOR:
      mag_affects(g, level, ch, tch, SPELL_MAGE_ARMOR);
      break;
    case SPELL_GROUP_RECALL:
      spell_recall(g, level, ch, tch, nullptr, nullptr);
      break;
  }
}

/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */
void mag_groups(GameWorld *g, int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *k;

  if (ch == nullptr)
    return;

  if (!char_aff_flagged(ch, AFF_GROUP))
    return;
  if (char_get_master(g, ch) != nullptr)
    k = char_get_master(g, ch);
  else
    k = ch;
  for (char_data *fol : char_followers(g, k)) {
    tch = fol;
    if (char_get_in_room(tch) != char_get_in_room(ch))
      continue;
    if (!char_aff_flagged(tch, AFF_GROUP))
      continue;
    if (ch == tch)
      continue;
    perform_mag_groups(g, level, ch, tch, spellnum);
  }

  if ((k != ch) && char_aff_flagged(k, AFF_GROUP))
    perform_mag_groups(g, level, ch, k, spellnum);
  perform_mag_groups(g, level, ch, ch, spellnum);
}

/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented.
 */
void mag_masses(GameWorld *g, int level, struct char_data *ch, int spellnum)
{

  for (char_data *tch : room_people(g, char_get_in_room(ch))) {
    if (tch == ch)
      continue;

    switch (spellnum) {
    }
  }
}

/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
 */
void mag_areas(GameWorld *g, int level, struct char_data *ch, int spellnum)
{
  const char *to_char = nullptr, *to_room = nullptr;

  if (ch == nullptr)
    return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum) {
    case SPELL_EARTHQUAKE:
      to_char = "You gesture and the earth begins to shake all around you!";
      to_room = "$n gracefully gestures and the earth begins to shake violently!";
      break;
  }

  if (to_char != nullptr)
    act(g, to_char, false, ch, nullptr, nullptr, TO_CHAR);
  if (to_room != nullptr)
    act(g, to_room, false, ch, nullptr, nullptr, TO_ROOM);

  for (char_data *tch : room_people(g, char_get_in_room(ch))) {

    /*
     * The skips: 1: the caster
     *            2: immortals
     *            3: if no pk on this mud, skips over all players
     *            4: pets (charmed NPCs)
     */

    if (tch == ch)
      continue;
    if (char_adm_flagged(tch, ADM_NODAMAGE))
      continue;
    if (!CONFIG_PK_ALLOWED && !char_is_npc(ch) && !char_is_npc(tch))
      continue;
    if (!char_is_npc(ch) && char_is_npc(tch) && char_aff_flagged(tch, AFF_CHARM))
      continue;

    /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
    mag_damage(level, ch, tch, spellnum);
  }
}

/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 */

void mag_summons(int level, struct char_data *ch, struct obj_data *obj, int spellnum, char *arg)
{
}

void mag_points(GameWorld *g, int level, struct char_data *ch, struct char_data *victim,
                int spellnum)
{
  int healing = 0;
  int tmp;

  if (victim == nullptr)
    return;

  switch (spellnum) {
    case SPELL_CURE_LIGHT:
      healing = dice(g, 1, 8) + MIN(level, 5);
      send_to_char(g, victim, "You feel better.\r\n");
      break;
    case SPELL_CURE_CRITIC:
      healing = dice(g, 4, 8) + MIN(level, 20);
      send_to_char(g, victim, "You feel a lot better!\r\n");
      break;
    case SPELL_HEAL:
      healing = 100 + dice(g, 3, 8);
      send_to_char(g, victim, "A warm feeling floods your body.\r\n");
      if (char_aff_flagged(ch, AFF_CDEATH)) {
        send_to_char(g, ch, "Your nerves settle slightly\r\n");
      }
      break;
    case SPELL_SENSU:
      if (char_get_conditions(g, victim, HUNGER) > -1) {
        char_set_conditions(g, victim, HUNGER, 48);
      }
      restore(g, victim, true);
      break;

    case ART_WHOLENESS_OF_BODY:
      healing = getEffMaxPL(g, victim) - getCurPL(g, victim);
      healing = MAX(0, healing);
      tmp     = char_get_ki(ch) / 2;
      if (tmp > healing)
        tmp = healing;
      else {
        healing = tmp;
      }
      char_set_ki(ch, char_get_ki(ch) - (tmp * 2));
      break;
  }
  update_pos(g, victim);
}

void mag_unaffects(GameWorld *g, int level, struct char_data *ch, struct char_data *victim,
                   int spellnum)
{
  int spell = 0, msg_not_affected = true;
  const char *to_vict = nullptr, *to_room = nullptr;

  if (victim == nullptr)
    return;

  switch (spellnum) {
    case SPELL_HEAL:
      /*
     * Heal also restores health, so don't give the "no effect" message
     * if the target isn't afflicted by the 'blindness' spell.
     */
      msg_not_affected = false;
      [[fallthrough]];
    case SPELL_REMOVE_BLINDNESS:
      spell   = SPELL_BLINDNESS;
      to_vict = "Your vision returns!";
      to_room = "There's a momentary gleam in $n's eyes.";
      break;
    case SPELL_NEUTRALIZE_POISON:
      spell   = SPELL_POISON;
      to_vict = "A warm feeling runs through your body!";
      to_room = "$n looks better.";
      break;
    case SPELL_REMOVE_CURSE:
      spell   = SPELL_BESTOW_CURSE;
      to_vict = "You don't feel so unlucky.";
      break;
    default:
      return;
  }

  if (!affected_by_spell(victim, spell)) {
    if (msg_not_affected)
      send_to_char(g, ch, "%s", CONFIG_NOEFFECT.c_str());
    return;
  }

  affect_from_char(g, victim, spell);
  if (to_vict != nullptr)
    act(g, to_vict, false, victim, nullptr, ch, TO_CHAR);
  if (to_room != nullptr)
    act(g, to_room, true, victim, nullptr, ch, TO_ROOM);
}

void mag_alter_objs(GameWorld *g, int level, struct char_data *ch, struct obj_data *obj,
                    int spellnum)
{
  const char *to_char = nullptr, *to_room = nullptr;

  if (obj == nullptr)
    return;

  switch (spellnum) {
    case SPELL_BLESS:
      if (!obj_extra_flagged(obj, ITEM_BLESS) &&
          (obj_get_weight(obj) <= 5 * level)) {
        obj_extra_set(obj, ITEM_BLESS, true);
        to_char = "$p glows briefly.";
      }
      break;
    case SPELL_INVISIBLE:
      if (!obj_extra_flagged(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
        obj_extra_set(obj, ITEM_INVISIBLE, true);
        to_char = "$p vanishes.";
      }
      break;
    case SPELL_POISON:
      if (((obj_get_type_flag(obj) == ITEM_DRINKCON) ||
           (obj_get_type_flag(obj) == ITEM_FOUNTAIN) ||
           (obj_get_type_flag(obj) == ITEM_FOOD)) &&
          !obj_get_value(obj, VAL_FOOD_POISON)) {
        obj_set_value(obj, VAL_FOOD_POISON, 1);
        to_char = "$p steams briefly.";
      }
      break;
    case SPELL_REMOVE_CURSE:
      if (obj_extra_flagged(obj, ITEM_NODROP)) {
        obj_extra_set(obj, ITEM_NODROP, false);
        if (obj_get_type_flag(obj) == ITEM_WEAPON)
          obj_set_value(obj, VAL_WEAPON_DAMSIZE, obj_get_value(obj, VAL_WEAPON_DAMSIZE) + 1);
        to_char = "$p briefly glows blue.";
      }
      break;
    case SPELL_NEUTRALIZE_POISON:
      if (((obj_get_type_flag(obj) == ITEM_DRINKCON) ||
           (obj_get_type_flag(obj) == ITEM_FOUNTAIN) ||
           (obj_get_type_flag(obj) == ITEM_FOOD)) &&
          obj_get_value(obj, VAL_FOOD_POISON)) {
        obj_set_value(obj, VAL_FOOD_POISON, 0);
        to_char = "$p steams briefly.";
      }
      break;
  }

  if (to_char == nullptr)
    send_to_char(g, ch, "%s", CONFIG_NOEFFECT.c_str());
  else
    act(g, to_char, true, ch, obj, nullptr, TO_CHAR);

  if (to_room != nullptr)
    act(g, to_room, true, ch, obj, nullptr, TO_ROOM);
  else if (to_char != nullptr)
    act(g, to_char, true, ch, obj, nullptr, TO_ROOM);
}

void mag_creations(GameWorld *g, int level, struct char_data *ch, int spellnum)
{
  struct obj_data *tobj;
  obj_vnum z;

  if (ch == nullptr)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used. */

  switch (spellnum) {
    case SPELL_CREATE_FOOD:
      z = 10;
      break;
    default:
      send_to_char(g, ch, "Spell unimplemented, it would seem.\r\n");
      return;
  }

  if (!(tobj = read_object(g, z))) {
    send_to_char(g, ch, "I seem to have goofed.\r\n");
    mud_log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
            spellnum, z);
    return;
  }
  add_unique_id(g, tobj);
  obj_to_char(g, tobj, ch);
  act(g, "$n creates $p.", false, ch, tobj, nullptr, TO_ROOM);
  act(g, "You create $p.", false, ch, tobj, nullptr, TO_CHAR);
  load_otrigger(g, tobj);
}

void mag_affectsv(GameWorld *g, int level, struct char_data *ch, struct char_data *victim,
                  int spellnum)
{
  struct affected_type af[MAX_SPELL_AFFECTS] = {};
  bool accum_affect = false, accum_duration = false;
  const char *to_vict = nullptr, *to_room = nullptr;
  int i;

  if (victim == nullptr || ch == nullptr)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type      = spellnum;
    af[i].bitvector = 0;
    af[i].modifier  = 0;
    af[i].location  = APPLY_NONE;
  }

  if (mag_newsaves(ch, victim, spellnum, level, char_get_aff_abils_intel(ch))) {
    if (IS_SET(g->spell_info[spellnum].save_flags, MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
      send_to_char(g, victim, "@g*save*@y You avoid any lasting affects.@n\r\n");
      return;
    }
  }

  switch (spellnum) {
    case SPELL_PARALYZE:
      af[0].duration  = level / 2;
      af[0].bitvector = AFF_PARALYZE;
      accum_duration  = false;
      to_vict         = "You feel your limbs freeze!";
      to_room         = "$n suddenly freezes in place!";
      break;
    case ART_STUNNING_FIST:
      af[0].duration  = 1;
      af[0].bitvector = AFF_STUNNED;
      accum_duration  = false;
      to_vict         = "You are in a stunned daze!";
      to_room         = "$n is stunned.";
      break;
    case ART_EMPTY_BODY:
      af[0].duration  = char_get_ki(ch) / 10;
      af[0].bitvector = AFF_ETHEREAL;
      accum_duration  = false;
      to_vict         = "You switch to the ethereal plane.";
      to_room         = "$n disappears.";
      break;
    case ART_QUIVERING_PALM:
      if (char_get_level(ch) <= char_get_level(victim)) {
        send_to_char(g, ch, "They are too high level for that.\r\n");
        return;
      }
      af[0].duration  = MAX(6, 20 - level);
      af[0].bitvector = AFF_CDEATH;
      accum_duration  = false;
      to_vict         = "You feel death closing in.";
      break;
    case SPELL_RESISTANCE:
      af[0].duration = 12;
      af[0].location = APPLY_ALLSAVES;
      af[0].modifier = 1;
      accum_duration = false;
      to_vict        = "You glow briefly with a silvery light.";
      to_room        = "$n glows briefly with a silvery light.";
      break;
    case SPELL_DAZE:
      if (char_get_race_level(victim) < 5)
        af[0].bitvector = AFF_NEXTNOACTION;
      accum_duration = false;
      to_vict        = "You are struck dumb by a flash of bright light!";
      to_room        = "$n is struck dumb by a flash of bright light!";
      break;
    case SPELL_SUMMON_MONSTER_I:
    case SPELL_SUMMON_MONSTER_II:
    case SPELL_SUMMON_MONSTER_III:
    case SPELL_SUMMON_MONSTER_IV:
    case SPELL_SUMMON_MONSTER_V:
    case SPELL_SUMMON_MONSTER_VI:
    case SPELL_SUMMON_MONSTER_VII:
    case SPELL_SUMMON_MONSTER_VIII:
    case SPELL_SUMMON_MONSTER_IX:
      af[0].duration  = level;
      af[0].bitvector = AFF_SUMMONED;
      accum_duration  = false;
      to_vict         = "You are summoned to assist $N!";
      to_room         = "$n appears, ready for action.";
      break;

    case SPELL_FLARE:
      if (char_mob_flagged(victim, MOB_NOBLIND)) {
        send_to_char(g, ch, "You fail.\r\n");
        return;
      }
      af[0].location  = APPLY_ACCURACY;
      af[0].modifier  = -1;
      af[0].duration  = 2;
      af[0].bitvector = AFF_BLIND;

      to_room = "$n seems to be dazzled!";
      to_vict = "You have been dazzled!";
      break;

    case SPELL_FIRE_SHIELD:
      af[0].duration  = level * 5;
      af[0].bitvector = AFF_FIRE_SHIELD;
      to_room         = "$n is engulfed in a firey shield!";
      to_vict         = "You are engulfed in a firey shield!";
      break;
  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (char_is_npc(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (char_aff_flagged(victim, af[i].bitvector)) {
        send_to_char(g, ch, "%s", CONFIG_NOEFFECT.c_str());
        return;
      }
  /*
   * If the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim, spellnum) && !(accum_duration || accum_affect)) {
    send_to_char(g, ch, "%s", CONFIG_NOEFFECT.c_str());
    return;
  }

  if (to_vict != nullptr)
    act(g, to_vict, false, victim, nullptr, ch, TO_CHAR);
  if (to_room != nullptr)
    act(g, to_room, true, victim, nullptr, ch, TO_ROOM);
}
