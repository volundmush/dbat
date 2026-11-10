/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/config.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/utils.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/fight.hpp"
#include "dbat/act.other.hpp"
#include "dbat/class.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/* extern globals */

/* local globals */

/* local functions */
static void mag_nextstrike(int level, struct char_data *caster, int spellnum);

static void unused_spell(GameWorld *g, int spl);

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

struct syllable {
  const char *org{};
  const char *news{};
};

const char *const unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */

static void mag_nextstrike(int level, struct char_data *caster, int spellnum)
{
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and < SKILL_TABLE_SIZE
 */
const char *skill_name(GameWorld *g, int num)
{
  if (num > 0 && num < SKILL_TABLE_SIZE)
    return (g->spell_info[num].name);
  else if (num == -1)
    return ("UNUSED");
  else
    return ("UNDEFINED");
}

int find_skill_num(GameWorld *g, char *name, int sktype)
{
  int skindex, ok;
  char *temp, *temp2;
  char first[MAX_INPUT_LENGTH], first2[MAX_INPUT_LENGTH], tempbuf[256];

  for (skindex = 0; skindex < SKILL_TABLE_SIZE; skindex++) {
    if (is_abbrev(name, g->spell_info[skindex].name) && (g->spell_info[skindex].skilltype & sktype)) {
      return (skindex);
    }

    ok = true;
    strlcpy(tempbuf, g->spell_info[skindex].name, sizeof(tempbuf)); /* strlcpy: OK */
    temp  = any_one_arg(tempbuf, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
        ok = false;
      temp  = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2 && (g->spell_info[skindex].skilltype & sktype)) {
      return (skindex);
    }
  }

  return (-1);
}

/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(GameWorld *g, struct char_data *caster, struct char_data *cvict,
               struct obj_data *ovict, int spellnum, int level, int casttype, char *arg)
{
  long ovict_id;

  if (spellnum < 0 || spellnum >= SKILL_TABLE_SIZE)
    return (0);

  /* Any of these scripts can purge the object or force the caster away. */
  ovict_id = ovict ? obj_get_id(ovict) : 0;
  if (!cast_wtrigger(g, caster, cvict, ovict, spellnum) || char_is_dead(caster) || (ovict && !find_obj(g, ovict_id)))
    return 0;
  if (!cast_otrigger(g, caster, ovict, spellnum))
    return 0;
  if (!cast_mtrigger(g, caster, cvict, spellnum) || char_is_dead(caster) || (ovict && !find_obj(g, ovict_id)))
    return 0;

  if (room_flagged(g, char_get_in_room(caster), ROOM_PEACEFUL) && char_get_admlevel(caster) < ADMLVL_IMPL &&
      (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE))) {
    send_to_char(g, caster, "A flash of white light fills the room, dispelling your violent magic!\r\n");
    act(g, "White light from no particular source suddenly fills the room, then vanishes.", false, caster, nullptr, nullptr, TO_ROOM);
    return (0);
  }

  if (IS_SET(SINFO.routines, MAG_NEXTSTRIKE) && casttype != CAST_STRIKE) {
    mag_nextstrike(level, caster, spellnum);
    return 1;
  }

  if (IS_SET(SINFO.routines, MAG_DAMAGE))
    if (mag_damage(level, caster, cvict, spellnum) == -1)
      return (-1); /* Successful and target died, don't cast again. */

  if (IS_SET(SINFO.routines, MAG_AFFECTS))
    mag_affects(g, level, caster, cvict, spellnum);

  if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
    mag_unaffects(g, level, caster, cvict, spellnum);

  if (IS_SET(SINFO.routines, MAG_POINTS))
    mag_points(g, level, caster, cvict, spellnum);

  if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
    mag_alter_objs(g, level, caster, ovict, spellnum);

  if (IS_SET(SINFO.routines, MAG_GROUPS))
    mag_groups(g, level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_MASSES))
    mag_masses(g, level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_AREAS))
    mag_areas(g, level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_SUMMONS))
    mag_summons(level, caster, ovict, spellnum, arg);

  if (IS_SET(SINFO.routines, MAG_CREATIONS))
    mag_creations(g, level, caster, spellnum);

  if (IS_SET(SINFO.routines, MAG_MANUAL))
    switch (spellnum) {
      case SPELL_CHARM: MANUAL_SPELL(g, spell_charm); break;
      case SPELL_CREATE_WATER: MANUAL_SPELL(g, spell_create_water); break;
      case SPELL_DETECT_POISON: MANUAL_SPELL(g, spell_detect_poison); break;
      case SPELL_ENCHANT_WEAPON: MANUAL_SPELL(g, spell_enchant_weapon); break;
      case SPELL_IDENTIFY: MANUAL_SPELL(g, spell_identify); break;
      case SPELL_LOCATE_OBJECT: MANUAL_SPELL(g, spell_locate_object); break;
      case SPELL_SUMMON: MANUAL_SPELL(g, spell_summon); break;
      case SPELL_WORD_OF_RECALL: MANUAL_SPELL(g, spell_recall); break;
      case SPELL_TELEPORT: MANUAL_SPELL(g, spell_teleport); break;
      case SPELL_PORTAL: MANUAL_SPELL(g, spell_portal); break;
      case ART_ABUNDANT_STEP: MANUAL_SPELL(g, art_abundant_step); break;
    }

  if (IS_SET(SINFO.routines, MAG_AFFECTSV))
    mag_affectsv(g, level, caster, cvict, spellnum);

  return (1);
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */
void mag_objectmagic(GameWorld *g, struct char_data *ch, struct obj_data *obj, std::string_view argument)
{
  int i, k;
  struct char_data *tch = nullptr;
  struct obj_data *tobj = nullptr;

  std::string arg = take_arg(argument);

  k = generic_find(g, arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tch, &tobj);

  switch (obj_get_type_flag(obj)) {
    case ITEM_STAFF:
      act(g, "You tap $p three times on the ground.", false, ch, obj, nullptr, TO_CHAR);
      if (obj_get_action_description(obj))
        act(g, obj_get_action_description(obj), false, ch, obj, nullptr, TO_ROOM);
      else
        act(g, "$n taps $p three times on the ground.", false, ch, obj, nullptr, TO_ROOM);

      if (obj_get_value(obj, VAL_STAFF_CHARGES) <= 0) {
        send_to_char(g, ch, "It seems powerless.\r\n");
        act(g, "Nothing seems to happen.", false, ch, obj, nullptr, TO_ROOM);
      } else {
        obj_set_value(obj, VAL_STAFF_CHARGES, obj_get_value(obj, VAL_STAFF_CHARGES) - 1);
        char_aff_set(ch, AFF_NEXTNOACTION, true);
        /* Level to cast spell at. */
        k = obj_get_value(obj, VAL_STAFF_LEVEL) ? obj_get_value(obj, VAL_STAFF_LEVEL) : DEFAULT_STAFF_LVL;

        /*
       * Problem : Area/mass spells on staves can cause crashes.
       * Solution: Remove the special nature of area/mass spells on staves.
       * Problem : People like that behavior.
       * Solution: We special case the area/mass spells here.
       */
        if (spell_has_routine(g, obj_get_value(obj, VAL_STAFF_SPELL), MAG_MASSES | MAG_AREAS)) {
          i = count_members(room_people(g, char_get_in_room(ch)));
          while (i-- > 0)
            call_magic(g, ch, nullptr, nullptr, obj_get_value(obj, VAL_STAFF_SPELL), k, CAST_STAFF, nullptr);
        } else {
          for (char_data *vict : room_people(g, char_get_in_room(ch))) {
            if (ch != vict)
              call_magic(g, ch, vict, nullptr, obj_get_value(obj, VAL_STAFF_SPELL), k, CAST_STAFF, nullptr);
          }
        }
      }
      break;
    case ITEM_WAND:
      if (k == FIND_CHAR_ROOM) {
        if (tch == ch) {
          act(g, "You point $p at yourself.", false, ch, obj, nullptr, TO_CHAR);
          act(g, "$n points $p at $mself.", false, ch, obj, nullptr, TO_ROOM);
        } else {
          act(g, "You point $p at $N.", false, ch, obj, tch, TO_CHAR);
          if (obj_get_action_description(obj))
            act(g, obj_get_action_description(obj), false, ch, obj, tch, TO_ROOM);
          else
            act(g, "$n points $p at $N.", true, ch, obj, tch, TO_ROOM);
        }
      } else if (tobj != nullptr) {
        act(g, "You point $p at $P.", false, ch, obj, tobj, TO_CHAR);
        if (obj_get_action_description(obj))
          act(g, obj_get_action_description(obj), false, ch, obj, tobj, TO_ROOM);
        else
          act(g, "$n points $p at $P.", true, ch, obj, tobj, TO_ROOM);
      } else if (IS_SET(g->spell_info[obj_get_value(obj, VAL_WAND_SPELL)].routines, MAG_AREAS | MAG_MASSES)) {
        /* Wands with area spells don't need to be pointed. */
        act(g, "You point $p outward.", false, ch, obj, nullptr, TO_CHAR);
        act(g, "$n points $p outward.", true, ch, obj, nullptr, TO_ROOM);
      } else {
        act(g, "At what should $p be pointed?", false, ch, obj, nullptr, TO_CHAR);
        return;
      }

      if (obj_get_value(obj, VAL_WAND_CHARGES) <= 0) {
        send_to_char(g, ch, "It seems powerless.\r\n");
        act(g, "Nothing seems to happen.", false, ch, obj, nullptr, TO_ROOM);
        return;
      }
      obj_set_value(obj, VAL_WAND_CHARGES, obj_get_value(obj, VAL_WAND_CHARGES) - 1);
      char_aff_set(ch, AFF_NEXTNOACTION, true);
      if (obj_get_value(obj, VAL_WAND_LEVEL))
        call_magic(g, ch, tch, tobj, obj_get_value(obj, VAL_WAND_SPELL),
                   obj_get_value(obj, VAL_WAND_LEVEL), CAST_WAND, nullptr);
      else
        call_magic(g, ch, tch, tobj, obj_get_value(obj, VAL_WAND_SPELL),
                   DEFAULT_WAND_LVL, CAST_WAND, nullptr);
      break;
    case ITEM_SCROLL:
      if (!arg.empty()) {
        if (!k) {
          act(g, "There is nothing to here to affect with $p.", false,
              ch, obj, nullptr, TO_CHAR);
          return;
        }
      } else
        tch = ch;

      act(g, "You recite $p which dissolves.", true, ch, obj, nullptr, TO_CHAR);
      if (obj_get_action_description(obj))
        act(g, obj_get_action_description(obj), false, ch, obj, tch, TO_ROOM);
      else
        act(g, "$n recites $p.", false, ch, obj, nullptr, TO_ROOM);

      char_aff_set(ch, AFF_NEXTNOACTION, true);
      for (i = 1; i <= 3; i++)
        if (call_magic(g, ch, tch, tobj, obj_get_value(obj, i),
                       obj_get_value(obj, VAL_SCROLL_LEVEL), CAST_SCROLL, nullptr) <= 0)
          break;

      if (obj != nullptr)
        extract_obj(g, obj);
      break;
    case ITEM_POTION:
      tch = ch;

      if (!consume_otrigger(g, obj, ch, OCMD_QUAFF)) /* check trigger */
        return;

      act(g, "You swallow $p.", false, ch, obj, nullptr, TO_CHAR);
      if (obj_get_action_description(obj))
        act(g, obj_get_action_description(obj), false, ch, obj, nullptr, TO_ROOM);
      else
        act(g, "$n swallows $p.", true, ch, obj, nullptr, TO_ROOM);

      char_aff_set(ch, AFF_NEXTNOACTION, true);
      for (i = 1; i <= 3; i++)
        if (call_magic(g, ch, ch, nullptr, obj_get_value(obj, i),
                       obj_get_value(obj, VAL_POTION_LEVEL), CAST_POTION, nullptr) <= 0)
          break;

      if (obj != nullptr)
        extract_obj(g, obj);
      break;
    default:
      mud_log("SYSERR: Unknown object_type {:d} in mag_objectmagic.",
              obj_get_type_flag(obj));
      break;
  }
}

/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 */

/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */
int skill_type(GameWorld *g, int snum)
{
  return g->spell_info[snum].skilltype;
}

/* Assign the spells on boot up */
static void spello(GameWorld *g, int spl, const char *name, int max_mana, int min_mana, int mana_change, int minpos, int targets, int violent, int routines, int save_flags, int comp_flags, const char *wearoff, int cmspell_level, int school, int domain)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    g->spell_info[spl].min_level[i] = CONFIG_LEVEL_CAP;
  for (i = 0; i < NUM_RACES; i++)
    g->spell_info[spl].race_can_learn[i] = CONFIG_LEVEL_CAP;
  g->spell_info[spl].mana_max     = max_mana;
  g->spell_info[spl].mana_min     = min_mana;
  g->spell_info[spl].mana_change  = mana_change;
  g->spell_info[spl].ki_max       = 0;
  g->spell_info[spl].ki_min       = 0;
  g->spell_info[spl].ki_change    = 0;
  g->spell_info[spl].min_position = minpos;
  g->spell_info[spl].targets      = targets;
  g->spell_info[spl].violent      = violent;
  g->spell_info[spl].routines     = routines;
  g->spell_info[spl].name         = name;
  g->spell_info[spl].wear_off_msg = wearoff;
  g->spell_info[spl].skilltype    = SKTYPE_SPELL;
  g->spell_info[spl].flags        = 0;
  g->spell_info[spl].save_flags   = save_flags;
  g->spell_info[spl].comp_flags   = comp_flags;
  g->spell_info[spl].spell_level  = cmspell_level;
  g->spell_info[spl].school       = school;
  g->spell_info[spl].domain       = domain;
}

static void unused_spell(GameWorld *g, int spl)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++) {
    g->spell_info[spl].min_level[i]       = CONFIG_LEVEL_CAP;
    g->spell_info[spl].can_learn_skill[i] = SKLEARN_CROSSCLASS;
  }
  for (i = 0; i < NUM_RACES; i++)
    g->spell_info[spl].race_can_learn[i] = SKLEARN_CROSSCLASS;
  g->spell_info[spl].mana_max     = 0;
  g->spell_info[spl].mana_min     = 0;
  g->spell_info[spl].mana_change  = 0;
  g->spell_info[spl].ki_max       = 0;
  g->spell_info[spl].ki_min       = 0;
  g->spell_info[spl].ki_change    = 0;
  g->spell_info[spl].min_position = 0;
  g->spell_info[spl].targets      = 0;
  g->spell_info[spl].violent      = 0;
  g->spell_info[spl].routines     = 0;
  g->spell_info[spl].name         = unused_spellname;
  g->spell_info[spl].skilltype    = SKTYPE_NONE;
  g->spell_info[spl].flags        = 0;
  g->spell_info[spl].save_flags   = 0;
  g->spell_info[spl].comp_flags   = 0;
  g->spell_info[spl].spell_level  = 0;
  g->spell_info[spl].school       = 0;
  g->spell_info[spl].domain       = 0;
}

static void skillo(GameWorld *g, int skill, const char *name, int flags)
{
  spello(g, skill, name, 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr, 0, 0, 0);
  g->spell_info[skill].skilltype = SKTYPE_SKILL;
  g->spell_info[skill].flags     = flags;
}

/*
 * Arguments for spello calls:
 *
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.hpp (such as SPELL_HEAL).
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * spellname: The name of the spell.
 *
 * school: The school of the spell.
 *
 * domain: The domain of the spell.
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

/*
 * NOTE: SPELL LEVELS ARE NO LONGER ASSIGNED HERE AS OF Circle 3.0 bpl9.
 * In order to make this cleaner, as well as to make adding new classes
 * much easier, spell levels are now assigned in class.c.  You only need
 * a spello() call to define a new spell; to decide who gets to use a spell
 * or skill, look in class.c.  -JE 5 Feb 1996
 */

void mag_assign_spells(GameWorld *g)
{
  int i;

  /* Do not change the loop below. */
  for (i = 0; i < SKILL_TABLE_SIZE; i++)
    unused_spell(g, i);
  /* Do not change the loop above. */

  spello(g, SPELL_ANIMATE_DEAD, "animate dead", 35, 10, 3, POS_STANDING,
         TAR_OBJ_ROOM, false,
         MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         3, SCHOOL_NECROMANCY, DOMAIN_DEATH);

  spello(g, SPELL_MAGE_ARMOR, "mage armor", 30, 15, 3, POS_FIGHTING,
         TAR_CHAR_ROOM, false,
         MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "You feel less protected.",
         1, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_BLESS, "blessed", 35, 5, 3, POS_STANDING,
         TAR_CHAR_ROOM | TAR_OBJ_INV, false,
         MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, 0, 0,
         "You feel less righteous.",
         1, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SKILL_SPIRITCONTROL, "spirit control", 35, 5, 3, POS_STANDING,
         TAR_CHAR_ROOM | TAR_OBJ_INV, false,
         MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, 0, 0,
         "You no longer have full control of your spirit.",
         1, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_BLINDNESS, "blindness", 35, 25, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, false,
         MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "You feel a cloak of blindness dissolve.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNIVERSAL);

  spello(g, SKILL_SOLARF, "blind", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "You are no longer blind!",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_BURNING_HANDS, "burning hands", 30, 10, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
         MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
         nullptr,
         1, SCHOOL_TRANSMUTATION, DOMAIN_FIRE);

  spello(g, SPELL_CALL_LIGHTNING, "call lightning", 40, 25, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
         MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
         nullptr,
         3, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_INFLICT_CRITIC, "inflict critic", 30, 10, 2, POS_FIGHTING,
         TAR_CHAR_ROOM, true,
         MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_HALF, 0,
         nullptr,
         4, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_INFLICT_LIGHT, "inflict light", 30, 10, 2, POS_FIGHTING,
         TAR_CHAR_ROOM, true,
         MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_HALF, 0,
         nullptr,
         1, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_CHARM, "charm person", 75, 50, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true,
         MAG_ACTION_FULL | MAG_MANUAL, MAGSAVE_WILL | MAGSAVE_NONE, 0,
         "You feel more self-confident.",
         1, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED);

  spello(g, SPELL_CHILL_TOUCH, "chill touch", 30, 10, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
         MAG_ACTION_FULL | MAG_DAMAGE | MAG_AFFECTS, MAGSAVE_FORT | MAGSAVE_PARTIAL, 0,
         "You feel your strength return.",
         1, SCHOOL_NECROMANCY, DOMAIN_UNDEFINED);

  spello(g, SPELL_COLOR_SPRAY, "color spray", 30, 15, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
         MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_NONE, 0,
         nullptr,
         1, SCHOOL_ILLUSION, DOMAIN_UNDEFINED);

  spello(g, SPELL_CONTROL_WEATHER, "control weather", 75, 25, 5, POS_STANDING,
         TAR_IGNORE, false,
         MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         7, SCHOOL_TRANSMUTATION, DOMAIN_AIR);

  spello(g, SPELL_CREATE_FOOD, "create food", 30, 5, 4, POS_STANDING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_CREATIONS, 0, 0,
         nullptr,
         3, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_CREATE_WATER, "create water", 30, 5, 4, POS_STANDING,
         TAR_OBJ_INV | TAR_OBJ_EQUIP, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_REMOVE_BLINDNESS, "remove blindness", 30, 5, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_UNAFFECTS, 0, 0,
         nullptr,
         3, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_CURE_CRITIC, "cure critic", 30, 10, 2, POS_FIGHTING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
         nullptr,
         4, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_CURE_LIGHT, "cure light", 30, 10, 2, POS_FIGHTING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
         nullptr,
         1, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_BESTOW_CURSE, "bestow curse", 80, 50, 2, POS_FIGHTING,
         TAR_CHAR_ROOM, true,
         MAG_AFFECTS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
         "You feel more optimistic.",
         8, SCHOOL_NECROMANCY, DOMAIN_DESTRUCTION);

  spello(g, SPELL_BANE, "bane", 80, 50, 2, POS_FIGHTING,
         TAR_CHAR_ROOM, true,
         MAG_AFFECTS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
         "You feel more optimistic.",
         8, SCHOOL_ENCHANTMENT, DOMAIN_CHARM);

  spello(g, SPELL_DETECT_ALIGN, "detect alignment", 20, 10, 2, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "You feel less aware.",
         1, SCHOOL_DIVINATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_SEE_INVIS, "see invisibility", 20, 10, 2, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "Your eyes stop tingling.",
         2, SCHOOL_DIVINATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_DETECT_MAGIC, "detect magic", 20, 10, 2, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "The detect magic wears off.",
         0, SCHOOL_UNIVERSAL, DOMAIN_UNIVERSAL);

  spello(g, SPELL_DETECT_POISON, "detect poison", 15, 5, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         "The detect poison wears off.",
         0, SCHOOL_DIVINATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_DISPEL_EVIL, "dispel evil", 40, 25, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_NONE, 0,
         nullptr,
         5, SCHOOL_UNDEFINED, DOMAIN_GOOD);

  spello(g, SPELL_DISPEL_GOOD, "dispel good", 40, 25, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_WILL | MAGSAVE_NONE, 0,
         nullptr,
         5, SCHOOL_UNDEFINED, DOMAIN_EVIL);

  spello(g, SPELL_EARTHQUAKE, "earthquake", 40, 25, 3, POS_FIGHTING,
         TAR_IGNORE, true, MAG_ACTION_FULL | MAG_AREAS, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
         nullptr,
         8, SCHOOL_UNDEFINED, DOMAIN_DESTRUCTION | DOMAIN_EARTH);

  spello(g, SPELL_ENCHANT_WEAPON, "enchant weapon", 150, 100, 10, POS_STANDING,
         TAR_OBJ_INV, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         9, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_ENERGY_DRAIN, "energy drain", 40, 25, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true,
         MAG_DAMAGE | MAG_MANUAL, MAGSAVE_FORT | MAGSAVE_NONE, 0, nullptr,
         9, SCHOOL_NECROMANCY, DOMAIN_UNIVERSAL);

  spello(g, SPELL_GROUP_ARMOR, "group armor", 50, 30, 2, POS_STANDING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_GROUPS, 0, 0,
         nullptr,
         5, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_FAERIE_FIRE, "faerie fire", 20, 10, 2, POS_STANDING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         1, SCHOOL_EVOCATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_FIREBALL, "fireball", 40, 30, 2, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
         nullptr,
         3, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_MASS_HEAL, "mass heal", 80, 60, 5, POS_STANDING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_GROUPS, 0, 0,
         nullptr,
         6, SCHOOL_CONJURATION, DOMAIN_HEALING);

  spello(g, SPELL_HARM, "harm", 75, 45, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         nullptr,
         6, SCHOOL_UNDEFINED, DOMAIN_DESTRUCTION);

  spello(g, SPELL_HEAL, "heal", 60, 40, 3, POS_FIGHTING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS | MAG_UNAFFECTS, 0, 0,
         nullptr,
         6, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_SENSU, "sensu", 1, 0, 3, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_POINTS, 0, 0,
         nullptr,
         6, SCHOOL_DIVINATION, DOMAIN_HEALING);

  spello(g, SPELL_IDENTIFY, "identify", 50, 25, 5, POS_STANDING,
         TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         2, SCHOOL_DIVINATION, DOMAIN_MAGIC);

  spello(g, SPELL_DARKVISION, "darkvision", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "Your night vision seems to fade.",
         2, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_INVISIBLE, "invisibility", 35, 25, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, 0, 0,
         "You feel yourself exposed.",
         2, SCHOOL_ILLUSION, DOMAIN_TRICKERY);

  spello(g, SPELL_LIGHTNING_BOLT, "lightning bolt", 30, 15, 1, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, MAGSAVE_REFLEX | MAGSAVE_HALF, 0,
         nullptr,
         3, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_LOCATE_OBJECT, "locate object", 25, 20, 1, POS_STANDING,
         TAR_OBJ_WORLD, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         3, SCHOOL_DIVINATION, DOMAIN_TRAVEL);

  spello(g, SPELL_MAGIC_MISSILE, "magic missile", 25, 10, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, 0, 0,
         nullptr,
         1, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_PARALYZE, "stone", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "Your body is no longer petrified.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_HASSHUKEN, "hasshuken", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "Your arms slow down.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_CURSE, "curse", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "You are no longer cursed!",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_MIGHT, "might", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "Your strength fades.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_PARALYZE, "paralyze", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "Your feel like you are able to move again.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_POISON, "poison", 50, 20, 3, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, true,
         MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "You feel like you got over something.",
         4, SCHOOL_NECROMANCY, DOMAIN_UNIVERSAL);

  spello(g, SKILL_POISON, "dark metamorphosis", 50, 20, 3, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, true,
         MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "Your dark metamorphosis fades.",
         4, SCHOOL_NECROMANCY, DOMAIN_UNIVERSAL);

  spello(g, SKILL_POISON, "healing glow", 50, 20, 3, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, true,
         MAG_ACTION_FULL | MAG_AFFECTS | MAG_ALTER_OBJS, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "Your healing glow fades.",
         4, SCHOOL_NECROMANCY, DOMAIN_UNIVERSAL);

  spello(g, SKILL_ENLIGHTEN, "enlighten", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "You feel less wise.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_GENIUS, "genius", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "You am dumb dumbner now.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_FLEX, "flex", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "You feel less agile.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_PORTAL, "portal", 75, 75, 0, POS_STANDING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         7, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_PROT_FROM_EVIL, "protection from evil", 40, 10, 3, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "You feel less protected.",
         1, SCHOOL_ABJURATION, DOMAIN_GOOD);

  spello(g, SPELL_REMOVE_CURSE, "remove curse", 45, 25, 5, POS_STANDING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, false,
         MAG_ACTION_FULL | MAG_UNAFFECTS | MAG_ALTER_OBJS, 0, 0,
         nullptr,
         3, SCHOOL_ABJURATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_NEUTRALIZE_POISON, "neutralize poison", 40, 8, 4, POS_STANDING,
         TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, false, MAG_ACTION_FULL | MAG_UNAFFECTS | MAG_ALTER_OBJS, 0, 0,
         nullptr,
         4, SCHOOL_CONJURATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_SANCTUARY, "sanctuary", 110, 85, 5, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "The white aura around your body fades.",
         9, SCHOOL_UNDEFINED, DOMAIN_PROTECTION);

  spello(g, SPELL_SENSE_LIFE, "sense life", 20, 10, 2, POS_STANDING,
         TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "You feel less aware of your surroundings.",
         2, SCHOOL_DIVINATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_SHOCKING_GRASP, "shocking grasp", 30, 15, 3, POS_FIGHTING,
         TAR_CHAR_ROOM | TAR_FIGHT_VICT, true, MAG_ACTION_FULL | MAG_DAMAGE, 0, 0,
         nullptr,
         1, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SLEEP, "sleep", 40, 25, 5, POS_FIGHTING,
         TAR_CHAR_ROOM, true, MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
         "You feel like you can wake up again.",
         1, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED);

  spello(g, SPELL_HAYASA, "hayasa", 40, 25, 5, POS_FIGHTING,
         TAR_CHAR_ROOM, true, MAG_ACTION_FULL | MAG_AFFECTS, MAGSAVE_WILL | MAGSAVE_NONE, 0,
         "You feel your speed decrease as Hayasa fades.",
         1, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED);

  spello(g, SKILL_TSKIN, "tough skin", 25, 10, 1, POS_STANDING,
         TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_ACTION_FULL | MAG_AFFECTSV, MAGSAVE_FORT | MAGSAVE_NONE, 0,
         "Your skin isn't quite so thick anymore.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_BULL_STRENGTH, "bull strength", 35, 30, 1, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "You feel weaker.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_STRENGTH);

  spello(g, SPELL_SUMMON, "summon", 75, 50, 3, POS_STANDING,
         TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         7, SCHOOL_CONJURATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_TELEPORT, "teleport", 75, 50, 3, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         5, SCHOOL_TRANSMUTATION, DOMAIN_TRAVEL);

  spello(g, SPELL_WATERWALK, "waterwalk", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         "Your feet seem less buoyant.",
         3, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_WORD_OF_RECALL, "word of recall", 20, 10, 2, POS_FIGHTING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_MANUAL, 0, 0,
         nullptr,
         6, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_RESISTANCE, "resistance", 40, 20, 0, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTSV,
         0, MAGCOMP_MATERIAL | MAGCOMP_SOMATIC | MAGCOMP_VERBAL | MAGCOMP_DIVINE_FOCUS,
         nullptr, 0, SCHOOL_ABJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_ACID_SPLASH, "acid splash", 40, 20, 0, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS,
         0, MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
         nullptr,
         0, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_DAZE, "daze", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTSV,
         MAGSAVE_WILL | MAGSAVE_NONE, MAGCOMP_MATERIAL | MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
         nullptr, 0, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED);

  spello(g, SPELL_FLARE, "flare", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTSV,
         MAGSAVE_FORT | MAGSAVE_NONE, MAGCOMP_VERBAL,
         nullptr,
         0, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_RAY_OF_FROST, "ray of frost", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS,
         0, MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
         nullptr,
         0, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_DISRUPT_UNDEAD, "disrupt undead", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS,
         0, MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
         nullptr,
         0, SCHOOL_NECROMANCY, DOMAIN_UNDEFINED);

  /*spello(SPELL_LESSER_GLOBE_OF_INVUL, "lesser globe of invulnerability", 40, 20, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_ACTION_FULL | MAG_AFFECTS, 
	0, MAGCOMP_MATERIAL | MAGCOMP_SOMATIC | MAGCOMP_VERBAL,
	NULL,
	0, SCHOOL_ABJURATION, DOMAIN_UNDEFINED); */

  /*spello(SPELL_MINOR_CREATION, "minor creation", 40, 20, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
	NULL,
	0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED); */

  spello(g, SPELL_SUMMON_MONSTER_I, "summon monster i", 40, 20, 2, POS_FIGHTING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         1, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_II, "summon monster ii", 40, 20, 2, POS_FIGHTING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         2, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_III, "summon monster iii", 40, 20, 2, POS_FIGHTING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         3, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_IV, "summon monster iv", 40, 20, 2, POS_FIGHTING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         4, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_V, "summon monster v", 40, 20, 2, POS_FIGHTING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         5, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_VI, "summon monster vi", 40, 20, 2, POS_FIGHTING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         6, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_VII, "summon monster vii", 40, 20, 2, POS_FIGHTING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         7, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_VIII, "summon monster viii", 40, 20, 2, POS_FIGHTING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         8, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_IX, "summon monster ix", 40, 20, 2, POS_FIGHTING,
         TAR_IGNORE, false, MAG_ACTION_FULL | MAG_SUMMONS, 0, 0,
         nullptr,
         9, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_FIRE_SHIELD, "fire shield", 40, 20, 2, POS_FIGHTING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTSV, 0, 0,
         nullptr,
         4, SCHOOL_EVOCATION, DOMAIN_FIRE);

  spello(g, SPELL_ICE_STORM, "ice storm", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_SHOUT, "shout", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_FEAR, "fear", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_CLOUDKILL, "cloudkill", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_MAJOR_CREATION, "major creation", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_HOLD_MONSTER, "hold monster", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_CONE_OF_COLD, "cone of cold", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_ANIMAL_GROWTH, "animal growth", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_BALEFUL_POLYMORPH, "baleful polymorph", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_PASSWALL, "passwall", 40, 20, 2, POS_STANDING,
         TAR_CHAR_ROOM, false, MAG_ACTION_FULL | MAG_AFFECTS, 0, 0,
         nullptr,
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);
  /*
   * These spells are currently not used, not implemented, and not castable.
   * Values for the 'breath' spells are filled in assuming a dragon's breath.
   */

  spello(g, SPELL_FIRE_BREATH, "fire breath", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, true, 0, 0, 0,
         nullptr,
         0, 0, 0);

  spello(g, SPELL_GAS_BREATH, "gas breath", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, true, 0, 0, 0,
         nullptr,
         0, 0, 0);

  spello(g, SPELL_FROST_BREATH, "frost breath", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, true, 0, 0, 0,
         nullptr,
         0, 0, 0);

  spello(g, SPELL_ACID_BREATH, "acid breath", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, true, 0, 0, 0,
         nullptr,
         0, 0, 0);

  spello(g, SPELL_LIGHTNING_BREATH, "lightning breath", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, true, 0, 0, 0,
         nullptr,
         0, 0, 0);

  /* you might want to name this one something more fitting to your theme -Welcor*/
  spello(g, SPELL_DG_AFFECT, "Script-inflicted", 0, 0, 0, POS_SITTING,
         TAR_IGNORE, true, 0, 0, 0,
         nullptr,
         0, 0, 0);

  /*
   * Declaration of skills - this actually doesn't do anything except
   * set it up so that immortals can use these skills by default.  The
   * min level to use the skill for other classes is set up in class.c.
   */

  /*
   * skillo does spello and then marks the skill as a new style skill with
   * the appropriate flags.
   */
  /* Buff/Neg Skills */
  skillo(g, SKILL_FLEX, "flex", SKFLAG_CHAMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_GENIUS, "genius", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_ENLIGHTEN, "enlighten", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_TSKIN, "tough skin", SKFLAG_STRMOD | SKFLAG_ARMORALL);
  skillo(g, SKILL_KAIOKEN, "kaioken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_BLESS, "bless", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_CURSE, "curse", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_POISON, "poison", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_VIGOR, "vigor", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_POSE, "special pose", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_HASSHUKEN, "hasshuken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);

  /* Effect Skills */
  skillo(g, SKILL_GARDENING, "gardening", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_EXTRACT, "extract", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_RUNIC, "runic", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_COMMUNE, "commune", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SOLARF, "solar flare", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_MIGHT, "might", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_BALANCE, "balance", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
  skillo(g, SKILL_BUILD, "build", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_CONCENTRATION, "concentration", SKFLAG_CONMOD);
  skillo(g, SKILL_SPOT, "spot", SKFLAG_WISMOD);
  skillo(g, SKILL_FIRST_AID, "first aid", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_DISGUISE, "disguise", SKFLAG_CHAMOD);
  skillo(g, SKILL_ESCAPE_ARTIST, "escape", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
  skillo(g, SKILL_APPRAISE, "appraise", SKFLAG_INTMOD);
  skillo(g, SKILL_HEAL, "heal", SKFLAG_WISMOD | SKFLAG_ARMORBAD);
  skillo(g, SKILL_FORGERY, "forgery", SKFLAG_INTMOD);
  skillo(g, SKILL_HIDE, "hide", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
  skillo(g, SKILL_LISTEN, "listen", SKFLAG_WISMOD);
  skillo(g, SKILL_EAVESDROP, "eavesdrop", SKFLAG_INTMOD);
  skillo(g, SKILL_CURE, "cure poison", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_OPEN_LOCK, "open lock", SKFLAG_DEXMOD | SKFLAG_NEEDTRAIN | SKFLAG_ARMORBAD);
  skillo(g, SKILL_REGENERATE, "regenerate", SKFLAG_CONMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_KEEN, "keen sight", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SEARCH, "search", SKFLAG_INTMOD);
  skillo(g, SKILL_MOVE_SILENTLY, "move silently", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
  skillo(g, SKILL_ABSORB, "absorb", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SLEIGHT_OF_HAND, "sleight of hand", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
  skillo(g, SKILL_INGEST, "ingest", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_REPAIR, "fix", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SENSE, "sense", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SURVIVAL, "survival", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_YOIK, "yoikominminken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_CREATE, "create", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SPIT, "stone spit", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_POTENTIAL, "potential release", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_TELEPATHY, "telepathy", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_FOCUS, "focus", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_INSTANTT, "instant transmission", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);

  /* Weapon Skills */
  skillo(g, SKILL_SWORD, "sword", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_DAGGER, "dagger", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_CLUB, "club", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SPEAR, "spear", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_GUN, "gun", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_BRAWL, "brawl", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);

  /* Defensive Skills */
  skillo(g, SKILL_DODGE, "dodge", SKFLAG_CHAMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_PARRY, "parry", SKFLAG_DEXMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_BLOCK, "block", SKFLAG_DEXMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_ZANZOKEN, "zanzoken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_BARRIER, "barrier", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);

  /* Offensive Skills */
  skillo(g, SKILL_THROW, "throw", SKFLAG_DEXMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_PUNCH, "punch", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_KICK, "kick", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_ELBOW, "elbow", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_KNEE, "knee", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_ROUNDHOUSE, "roundhouse", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_UPPERCUT, "uppercut", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SLAM, "slam", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_HEELDROP, "heeldrop", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_KIBALL, "kiball", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_KIBLAST, "kiblast", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_BEAM, "beam", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_TSUIHIDAN, "tsuihidan", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SHOGEKIHA, "shogekiha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_RENZO, "renzokou energy dan", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_MASENKO, "masenko", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_DODONPA, "dodonpa", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_GALIKGUN, "galik gun", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_KAMEHAMEHA, "kamehameha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_DEATHBEAM, "deathbeam", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_ERASER, "eraser cannon", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_TSLASH, "twin slash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_PSYBLAST, "psychic blast", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_HONOO, "honoo", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_DUALBEAM, "dual beam", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_ROGAFUFUKEN, "rogafufuken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_BAKUHATSUHA, "bakuhatsuha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_KIENZAN, "kienzan", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_TRIBEAM, "tribeam", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_SBC, "special beam cannon", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_FINALFLASH, "final flash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_CRUSHER, "crusher ball", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_DDSLASH, "darkness dragon slash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_PBARRAGE, "psychic barrage", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_HELLFLASH, "hell flash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_HELLSPEAR, "hell spear blast", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_KAKUSANHA, "kakusanha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_SCATTER, "scatter shot", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_BIGBANG, "big bang", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_PSLASH, "phoenix slash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_DEATHBALL, "deathball", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_SPIRITBALL, "spirit ball", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_GENKIDAMA, "genki dama", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER5);
  skillo(g, SKILL_GENOCIDE, "genocide", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER5);
  skillo(g, SKILL_DUALWIELD, "dual wield", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_TWOHAND, "twohand", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_STYLE, "fighting arts", SKFLAG_INTMOD);
  skillo(g, SKILL_KURA, "kuraiiro seiki", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_TAILWHIP, "tailwhip", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
  skillo(g, SKILL_KOUSENGAN, "kousengan", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
  skillo(g, SKILL_TAISHA, "taisha reiki", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_PARALYZE, "paralyze", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_INFUSE, "infuse", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_ROLL, "roll", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_TRIP, "trip", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_GRAPPLE, "grapple", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_WSPIKE, "water spikes", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_SELFD, "self destruct", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_SPIRAL, "spiral comet", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_BREAKER, "star breaker", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_MIMIC, "mimic", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_WRAZOR, "water razor", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_KOTEIRU, "koteiru bakuha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_DIMIZU, "dimizu toride", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_HYOGA_KABE, "hyoga kabe", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_WELLSPRING, "wellspring", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_AQUA_BARRIER, "aqua barrier", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_WARP, "warp pool", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_HSPIRAL, "hell spiral", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_ARMOR, "nanite armor", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_FIRESHIELD, "fireshield", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_COOKING, "cooking", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_SEISHOU, "seishou enko", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
  skillo(g, SKILL_SILK, "silk", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_BASH, "bash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_HEADBUTT, "headbutt", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_ENSNARE, "ensnare", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_STARNOVA, "starnova", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_MALICE, "malice breaker", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_ZEN, "zen blade strike", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
  skillo(g, SKILL_SUNDER, "sundering force", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_WITHER, "wither", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_METAMORPH, "dark metamorphosis", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_HAYASA, "hayasa", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_ENERGIZE, "energize throwing", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_PURSUIT, "pursuit", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_HEALGLOW, "healing glow", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_HANDLING, "handling", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_MYSTICMUSIC, "mystic music", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
  skillo(g, SKILL_LIGHTGRENADE, "light grenade", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_MULTIFORM, "multiform", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
  skillo(g, SKILL_SPIRITCONTROL, "spirit control", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
  skillo(g, SKILL_BALEFIRE, "balefire", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
  skillo(g, SKILL_BLESSEDHAMMER, "blessed hammer", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
}

bool spell_has_routine(GameWorld *g, int spellnum, int flag)
{
  return IS_SET(g->spell_info[spellnum].routines, flag);
}
