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

static constexpr std::string_view routine_bits_entries[] = {"DAMAGE", "AFFECTS", "UNAFFECTS", "POINTS", "ALTER_OBJS", "GROUPS", "MASSES", "AREAS", "SUMMONS",
                                                            "CREATIONS", "MANUAL", "AFFECTSV", "ACTION_FREE", "ACTION_PARTIAL", "ACTION_FULL", "NEXTSTRIKE",
                                                            "TOUCH_MELEE", "TOUCH_RANGED"};
static_assert(std::size(routine_bits_entries) == flag_count<mag_routine>);
constinit const name_table routine_bits(routine_bits_entries);
static constexpr std::string_view target_bits_entries[] = {"IGNORE", "CHAR_ROOM", "CHAR_WORLD", "FIGHT_SELF", "FIGHT_VICT", "SELF_ONLY", "NOT_SELF",
                                                           "OBJ_INV", "OBJ_ROOM", "OBJ_WORLD", "OBJ_EQUIP"};
static_assert(std::size(target_bits_entries) == flag_count<spell_target>);
constinit const name_table target_bits(target_bits_entries);
static constexpr std::string_view save_bits_entries[] = {"FORT", "REFLEX", "WILL", "HALF", "NONE", "PARTIAL"};
static_assert(std::size(save_bits_entries) == flag_count<mag_save>);
constinit const name_table save_bits(save_bits_entries);
static constexpr std::string_view component_bits_entries[] = {"DIVINE_FOCUS", "EXP_COST", "FOCUS", "MATERIAL", "SOMATIC", "VERBAL"};
static_assert(std::size(component_bits_entries) == flag_count<mag_component>);
constinit const name_table component_bits(component_bits_entries);
static constexpr std::string_view skill_kind_bits_entries[] = {"SPELL", "SKILL", "LANG", "WEAPON", "ART"};
static_assert(std::size(skill_kind_bits_entries) == flag_count<skill_kind>);
constinit const name_table skill_kind_bits(skill_kind_bits_entries);
static constexpr std::string_view skill_flag_bits_entries[] = {"NEEDTRAIN", "STRMOD", "DEXMOD", "CONMOD", "INTMOD", "WISMOD", "CHAMOD", "ARMORBAD", "ARMORALL", "TIER1", "TIER2", "TIER3", "TIER4", "TIER5"};
static_assert(std::size(skill_flag_bits_entries) == flag_count<skill_flag>);
constinit const name_table skill_flag_bits(skill_flag_bits_entries);

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
  std::string_view org{};
  std::string_view news{};
};

const std::string_view unused_spellname = "!UNUSED!"; /* its data() marks an unused entry */

static void mag_nextstrike(int level, struct char_data *caster, int spellnum)
{
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and < SKILL_TABLE_SIZE
 */
std::string_view skill_name(GameWorld *g, int num)
{
  if (num > 0 && num < SKILL_TABLE_SIZE)
    return (g->spell_info[num].name);
  else if (num == -1)
    return ("UNUSED");
  else
    return ("UNDEFINED");
}

int find_skill_num(GameWorld *g, std::string_view name, flag_set<skill_kind> sktype)
{
  for (int skindex = 0; skindex < SKILL_TABLE_SIZE; skindex++) {
    const spell_info_type &skill = g->spell_info[skindex];
    if (!skill.skilltype.intersects(sktype))
      continue;
    if (is_abbrev(name, skill.name))
      return skindex;

    /* each word of name abbreviating the matching word of the skill's */
    bool ok                     = true;
    std::string_view skill_rest = skill.name, name_rest = name;
    std::string first = take_word(skill_rest), first2 = take_word(name_rest);
    while (!first.empty() && !first2.empty() && ok) {
      if (!is_abbrev(first2, first))
        ok = false;
      first  = take_word(skill_rest);
      first2 = take_word(name_rest);
    }
    if (ok && first2.empty())
      return skindex;
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
               struct obj_data *ovict, int spellnum, int level, int casttype, std::string_view arg)
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

  if (room_flagged(g, char_get_in_room(caster), room_flag::peaceful) && char_get_admlevel(caster) < ADMLVL_IMPL &&
      (SINFO.violent || SINFO.routines.test(mag_routine::damage))) {
    send_to_char(g, caster, "A flash of white light fills the room, dispelling your violent magic!\r\n");
    act(g, "White light from no particular source suddenly fills the room, then vanishes.", false, caster, nullptr, nullptr, act_to::room);
    return (0);
  }

  if (SINFO.routines.test(mag_routine::nextstrike) && casttype != CAST_STRIKE) {
    mag_nextstrike(level, caster, spellnum);
    return 1;
  }

  if (SINFO.routines.test(mag_routine::damage))
    if (mag_damage(level, caster, cvict, spellnum) == -1)
      return (-1); /* Successful and target died, don't cast again. */

  if (SINFO.routines.test(mag_routine::affects))
    mag_affects(g, level, caster, cvict, spellnum);

  if (SINFO.routines.test(mag_routine::unaffects))
    mag_unaffects(g, level, caster, cvict, spellnum);

  if (SINFO.routines.test(mag_routine::points))
    mag_points(g, level, caster, cvict, spellnum);

  if (SINFO.routines.test(mag_routine::alter_objs))
    mag_alter_objs(g, level, caster, ovict, spellnum);

  if (SINFO.routines.test(mag_routine::groups))
    mag_groups(g, level, caster, spellnum);

  if (SINFO.routines.test(mag_routine::masses))
    mag_masses(g, level, caster, spellnum);

  if (SINFO.routines.test(mag_routine::areas))
    mag_areas(g, level, caster, spellnum);

  if (SINFO.routines.test(mag_routine::creations))
    mag_creations(g, level, caster, spellnum);

  if (SINFO.routines.test(mag_routine::manual))
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

  if (SINFO.routines.test(mag_routine::affectsv))
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
  int i;
  struct char_data *tch = nullptr;
  struct obj_data *tobj = nullptr;

  std::string arg = take_arg(argument);

  std::optional<find_flag> k = generic_find(g, arg, find_flag::char_room | find_flag::obj_inv | find_flag::obj_room | find_flag::obj_equip, ch, &tch, &tobj);

  switch (obj_get_type_flag(obj)) {
    case ITEM_STAFF:
      act(g, "You tap $p three times on the ground.", false, ch, obj, nullptr, act_to::actor);
      if (!obj_get_action_description(obj).empty())
        act(g, obj_get_action_description(obj), false, ch, obj, nullptr, act_to::room);
      else
        act(g, "$n taps $p three times on the ground.", false, ch, obj, nullptr, act_to::room);

      if (obj_get_value(obj, VAL_STAFF_CHARGES) <= 0) {
        send_to_char(g, ch, "It seems powerless.\r\n");
        act(g, "Nothing seems to happen.", false, ch, obj, nullptr, act_to::room);
      } else {
        obj_set_value(obj, VAL_STAFF_CHARGES, obj_get_value(obj, VAL_STAFF_CHARGES) - 1);
        char_aff_set(ch, aff_flag::nextnoaction, true);
        /* Level to cast spell at. */
        const int level = obj_get_value(obj, VAL_STAFF_LEVEL) ? obj_get_value(obj, VAL_STAFF_LEVEL) : DEFAULT_STAFF_LVL;

        /*
       * Problem : Area/mass spells on staves can cause crashes.
       * Solution: Remove the special nature of area/mass spells on staves.
       * Problem : People like that behavior.
       * Solution: We special case the area/mass spells here.
       */
        if (spell_has_routine(g, obj_get_value(obj, VAL_STAFF_SPELL), mag_routine::masses | mag_routine::areas)) {
          i = count_members(room_people(g, char_get_in_room(ch)));
          while (i-- > 0)
            call_magic(g, ch, nullptr, nullptr, obj_get_value(obj, VAL_STAFF_SPELL), level, CAST_STAFF);
        } else {
          for (char_data *vict : room_people(g, char_get_in_room(ch))) {
            if (ch != vict)
              call_magic(g, ch, vict, nullptr, obj_get_value(obj, VAL_STAFF_SPELL), level, CAST_STAFF);
          }
        }
      }
      break;
    case ITEM_WAND:
      if (k == find_flag::char_room) {
        if (tch == ch) {
          act(g, "You point $p at yourself.", false, ch, obj, nullptr, act_to::actor);
          act(g, "$n points $p at $mself.", false, ch, obj, nullptr, act_to::room);
        } else {
          act(g, "You point $p at $N.", false, ch, obj, tch, act_to::actor);
          if (!obj_get_action_description(obj).empty())
            act(g, obj_get_action_description(obj), false, ch, obj, tch, act_to::room);
          else
            act(g, "$n points $p at $N.", true, ch, obj, tch, act_to::room);
        }
      } else if (tobj != nullptr) {
        act(g, "You point $p at $P.", false, ch, obj, tobj, act_to::actor);
        if (!obj_get_action_description(obj).empty())
          act(g, obj_get_action_description(obj), false, ch, obj, tobj, act_to::room);
        else
          act(g, "$n points $p at $P.", true, ch, obj, tobj, act_to::room);
      } else if (g->spell_info[obj_get_value(obj, VAL_WAND_SPELL)].routines.intersects(mag_routine::areas | mag_routine::masses)) {
        /* Wands with area spells don't need to be pointed. */
        act(g, "You point $p outward.", false, ch, obj, nullptr, act_to::actor);
        act(g, "$n points $p outward.", true, ch, obj, nullptr, act_to::room);
      } else {
        act(g, "At what should $p be pointed?", false, ch, obj, nullptr, act_to::actor);
        return;
      }

      if (obj_get_value(obj, VAL_WAND_CHARGES) <= 0) {
        send_to_char(g, ch, "It seems powerless.\r\n");
        act(g, "Nothing seems to happen.", false, ch, obj, nullptr, act_to::room);
        return;
      }
      obj_set_value(obj, VAL_WAND_CHARGES, obj_get_value(obj, VAL_WAND_CHARGES) - 1);
      char_aff_set(ch, aff_flag::nextnoaction, true);
      if (obj_get_value(obj, VAL_WAND_LEVEL))
        call_magic(g, ch, tch, tobj, obj_get_value(obj, VAL_WAND_SPELL),
                   obj_get_value(obj, VAL_WAND_LEVEL), CAST_WAND);
      else
        call_magic(g, ch, tch, tobj, obj_get_value(obj, VAL_WAND_SPELL),
                   DEFAULT_WAND_LVL, CAST_WAND);
      break;
    case ITEM_SCROLL:
      if (!arg.empty()) {
        if (!k) {
          act(g, "There is nothing to here to affect with $p.", false,
              ch, obj, nullptr, act_to::actor);
          return;
        }
      } else
        tch = ch;

      act(g, "You recite $p which dissolves.", true, ch, obj, nullptr, act_to::actor);
      if (!obj_get_action_description(obj).empty())
        act(g, obj_get_action_description(obj), false, ch, obj, tch, act_to::room);
      else
        act(g, "$n recites $p.", false, ch, obj, nullptr, act_to::room);

      char_aff_set(ch, aff_flag::nextnoaction, true);
      for (i = 1; i <= 3; i++)
        if (call_magic(g, ch, tch, tobj, obj_get_value(obj, i),
                       obj_get_value(obj, VAL_SCROLL_LEVEL), CAST_SCROLL) <= 0)
          break;

      if (obj != nullptr)
        extract_obj(g, obj);
      break;
    case ITEM_POTION:
      tch = ch;

      if (!consume_otrigger(g, obj, ch, consume_cmd::quaff)) /* check trigger */
        return;

      act(g, "You swallow $p.", false, ch, obj, nullptr, act_to::actor);
      if (!obj_get_action_description(obj).empty())
        act(g, obj_get_action_description(obj), false, ch, obj, nullptr, act_to::room);
      else
        act(g, "$n swallows $p.", true, ch, obj, nullptr, act_to::room);

      char_aff_set(ch, aff_flag::nextnoaction, true);
      for (i = 1; i <= 3; i++)
        if (call_magic(g, ch, ch, nullptr, obj_get_value(obj, i),
                       obj_get_value(obj, VAL_POTION_LEVEL), CAST_POTION) <= 0)
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
flag_set<skill_kind> skill_type(GameWorld *g, int snum)
{
  return g->spell_info[snum].skilltype;
}

/* Assign the spells on boot up */
static void spello(GameWorld *g, int spl, std::string_view name, int max_mana, int min_mana, int mana_change, int minpos, flag_set<spell_target> targets, bool violent, flag_set<mag_routine> routines, flag_set<mag_save> save_flags, flag_set<mag_component> comp_flags, std::string_view wearoff, int cmspell_level, int school, int domain)
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
  g->spell_info[spl].skilltype    = skill_kind::spell;
  g->spell_info[spl].flags        = {};
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
  g->spell_info[spl].targets      = {};
  g->spell_info[spl].violent      = false;
  g->spell_info[spl].routines     = {};
  g->spell_info[spl].name         = unused_spellname;
  g->spell_info[spl].skilltype    = {};
  g->spell_info[spl].flags        = {};
  g->spell_info[spl].save_flags   = {};
  g->spell_info[spl].comp_flags   = {};
  g->spell_info[spl].spell_level  = 0;
  g->spell_info[spl].school       = 0;
  g->spell_info[spl].domain       = 0;
}

static void skillo(GameWorld *g, int skill, std::string_view name, flag_set<skill_flag> flags)
{
  spello(g, skill, name, 0, 0, 0, 0, {}, false, {}, {}, {}, {}, 0, 0, 0);
  g->spell_info[skill].skilltype = skill_kind::skill;
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
         spell_target::obj_room, false,
         mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         3, SCHOOL_NECROMANCY, DOMAIN_DEATH);

  spello(g, SPELL_MAGE_ARMOR, "mage armor", 30, 15, 3, POS_FIGHTING,
         spell_target::char_room, false,
         mag_routine::action_full | mag_routine::affects, {}, {},
         "You feel less protected.",
         1, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_BLESS, "blessed", 35, 5, 3, POS_STANDING,
         spell_target::char_room | spell_target::obj_inv, false,
         mag_routine::action_full | mag_routine::affects | mag_routine::alter_objs, {}, {},
         "You feel less righteous.",
         1, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SKILL_SPIRITCONTROL, "spirit control", 35, 5, 3, POS_STANDING,
         spell_target::char_room | spell_target::obj_inv, false,
         mag_routine::action_full | mag_routine::affects | mag_routine::alter_objs, {}, {},
         "You no longer have full control of your spirit.",
         1, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_BLINDNESS, "blindness", 35, 25, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, false,
         mag_routine::action_full | mag_routine::affects, mag_save::fort | mag_save::none, {},
         "You feel a cloak of blindness dissolve.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNIVERSAL);

  spello(g, SKILL_SOLARF, "blind", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "You are no longer blind!",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_BURNING_HANDS, "burning hands", 30, 10, 3, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true,
         mag_routine::action_full | mag_routine::damage, mag_save::reflex | mag_save::half, {},
         {},
         1, SCHOOL_TRANSMUTATION, DOMAIN_FIRE);

  spello(g, SPELL_CALL_LIGHTNING, "call lightning", 40, 25, 3, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true,
         mag_routine::action_full | mag_routine::damage, mag_save::reflex | mag_save::half, {},
         {},
         3, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_INFLICT_CRITIC, "inflict critic", 30, 10, 2, POS_FIGHTING,
         spell_target::char_room, true,
         mag_routine::action_full | mag_routine::damage, mag_save::will | mag_save::half, {},
         {},
         4, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_INFLICT_LIGHT, "inflict light", 30, 10, 2, POS_FIGHTING,
         spell_target::char_room, true,
         mag_routine::action_full | mag_routine::damage, mag_save::will | mag_save::half, {},
         {},
         1, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_CHARM, "charm person", 75, 50, 2, POS_FIGHTING,
         spell_target::char_room | spell_target::not_self, true,
         mag_routine::action_full | mag_routine::manual, mag_save::will | mag_save::none, {},
         "You feel more self-confident.",
         1, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED);

  spello(g, SPELL_CHILL_TOUCH, "chill touch", 30, 10, 3, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true,
         mag_routine::action_full | mag_routine::damage | mag_routine::affects, mag_save::fort | mag_save::partial, {},
         "You feel your strength return.",
         1, SCHOOL_NECROMANCY, DOMAIN_UNDEFINED);

  spello(g, SPELL_COLOR_SPRAY, "color spray", 30, 15, 3, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true,
         mag_routine::action_full | mag_routine::damage, mag_save::will | mag_save::none, {},
         {},
         1, SCHOOL_ILLUSION, DOMAIN_UNDEFINED);

  spello(g, SPELL_CONTROL_WEATHER, "control weather", 75, 25, 5, POS_STANDING,
         spell_target::ignore, false,
         mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         7, SCHOOL_TRANSMUTATION, DOMAIN_AIR);

  spello(g, SPELL_CREATE_FOOD, "create food", 30, 5, 4, POS_STANDING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::creations, {}, {},
         {},
         3, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_CREATE_WATER, "create water", 30, 5, 4, POS_STANDING,
         spell_target::obj_inv | spell_target::obj_equip, false, mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_REMOVE_BLINDNESS, "remove blindness", 30, 5, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::unaffects, {}, {},
         {},
         3, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_CURE_CRITIC, "cure critic", 30, 10, 2, POS_FIGHTING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::points, {}, {},
         {},
         4, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_CURE_LIGHT, "cure light", 30, 10, 2, POS_FIGHTING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::points, {}, {},
         {},
         1, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_BESTOW_CURSE, "bestow curse", 80, 50, 2, POS_FIGHTING,
         spell_target::char_room, true,
         mag_routine::affects, mag_save::will | mag_save::none, {},
         "You feel more optimistic.",
         8, SCHOOL_NECROMANCY, DOMAIN_DESTRUCTION);

  spello(g, SPELL_BANE, "bane", 80, 50, 2, POS_FIGHTING,
         spell_target::char_room, true,
         mag_routine::affects, mag_save::will | mag_save::none, {},
         "You feel more optimistic.",
         8, SCHOOL_ENCHANTMENT, DOMAIN_CHARM);

  spello(g, SPELL_DETECT_ALIGN, "detect alignment", 20, 10, 2, POS_STANDING,
         spell_target::char_room | spell_target::self_only, false, mag_routine::action_full | mag_routine::affects, {}, {},
         "You feel less aware.",
         1, SCHOOL_DIVINATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_SEE_INVIS, "see invisibility", 20, 10, 2, POS_STANDING,
         spell_target::char_room | spell_target::self_only, false, mag_routine::action_full | mag_routine::affects, {}, {},
         "Your eyes stop tingling.",
         2, SCHOOL_DIVINATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_DETECT_MAGIC, "detect magic", 20, 10, 2, POS_STANDING,
         spell_target::char_room | spell_target::self_only, false, mag_routine::action_full | mag_routine::affects, {}, {},
         "The detect magic wears off.",
         0, SCHOOL_UNIVERSAL, DOMAIN_UNIVERSAL);

  spello(g, SPELL_DETECT_POISON, "detect poison", 15, 5, 1, POS_STANDING,
         spell_target::char_room | spell_target::obj_inv | spell_target::obj_room, false, mag_routine::action_full | mag_routine::manual, {}, {},
         "The detect poison wears off.",
         0, SCHOOL_DIVINATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_DISPEL_EVIL, "dispel evil", 40, 25, 3, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true, mag_routine::action_full | mag_routine::damage, mag_save::will | mag_save::none, {},
         {},
         5, SCHOOL_UNDEFINED, DOMAIN_GOOD);

  spello(g, SPELL_DISPEL_GOOD, "dispel good", 40, 25, 3, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true, mag_routine::action_full | mag_routine::damage, mag_save::will | mag_save::none, {},
         {},
         5, SCHOOL_UNDEFINED, DOMAIN_EVIL);

  spello(g, SPELL_EARTHQUAKE, "earthquake", 40, 25, 3, POS_FIGHTING,
         spell_target::ignore, true, mag_routine::action_full | mag_routine::areas, mag_save::reflex | mag_save::half, {},
         {},
         8, SCHOOL_UNDEFINED, DOMAIN_DESTRUCTION | DOMAIN_EARTH);

  spello(g, SPELL_ENCHANT_WEAPON, "enchant weapon", 150, 100, 10, POS_STANDING,
         spell_target::obj_inv, false, mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         9, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_ENERGY_DRAIN, "energy drain", 40, 25, 1, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true,
         mag_routine::damage | mag_routine::manual, mag_save::fort | mag_save::none, {}, {},
         9, SCHOOL_NECROMANCY, DOMAIN_UNIVERSAL);

  spello(g, SPELL_GROUP_ARMOR, "group armor", 50, 30, 2, POS_STANDING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::groups, {}, {},
         {},
         5, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_FAERIE_FIRE, "faerie fire", 20, 10, 2, POS_STANDING,
         spell_target::char_world | spell_target::not_self, false, mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         1, SCHOOL_EVOCATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_FIREBALL, "fireball", 40, 30, 2, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true, mag_routine::action_full | mag_routine::damage, mag_save::reflex | mag_save::half, {},
         {},
         3, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_MASS_HEAL, "mass heal", 80, 60, 5, POS_STANDING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::groups, {}, {},
         {},
         6, SCHOOL_CONJURATION, DOMAIN_HEALING);

  spello(g, SPELL_HARM, "harm", 75, 45, 3, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true, mag_routine::action_full | mag_routine::damage, mag_save::fort | mag_save::none, {},
         {},
         6, SCHOOL_UNDEFINED, DOMAIN_DESTRUCTION);

  spello(g, SPELL_HEAL, "heal", 60, 40, 3, POS_FIGHTING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::points | mag_routine::unaffects, {}, {},
         {},
         6, SCHOOL_UNDEFINED, DOMAIN_HEALING);

  spello(g, SPELL_SENSU, "sensu", 1, 0, 3, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::points, {}, {},
         {},
         6, SCHOOL_DIVINATION, DOMAIN_HEALING);

  spello(g, SPELL_IDENTIFY, "identify", 50, 25, 5, POS_STANDING,
         spell_target::obj_inv | spell_target::obj_room, false, mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         2, SCHOOL_DIVINATION, DOMAIN_MAGIC);

  spello(g, SPELL_DARKVISION, "darkvision", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::self_only, false, mag_routine::action_full | mag_routine::affects, {}, {},
         "Your night vision seems to fade.",
         2, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_INVISIBLE, "invisibility", 35, 25, 1, POS_STANDING,
         spell_target::char_room | spell_target::obj_inv | spell_target::obj_room, false, mag_routine::action_full | mag_routine::affects | mag_routine::alter_objs, {}, {},
         "You feel yourself exposed.",
         2, SCHOOL_ILLUSION, DOMAIN_TRICKERY);

  spello(g, SPELL_LIGHTNING_BOLT, "lightning bolt", 30, 15, 1, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true, mag_routine::action_full | mag_routine::damage, mag_save::reflex | mag_save::half, {},
         {},
         3, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_LOCATE_OBJECT, "locate object", 25, 20, 1, POS_STANDING,
         spell_target::obj_world, false, mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         3, SCHOOL_DIVINATION, DOMAIN_TRAVEL);

  spello(g, SPELL_MAGIC_MISSILE, "magic missile", 25, 10, 3, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true, mag_routine::action_full | mag_routine::damage, {}, {},
         {},
         1, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_PARALYZE, "stone", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "Your body is no longer petrified.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_HASSHUKEN, "hasshuken", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "Your arms slow down.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_CURSE, "curse", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "You are no longer cursed!",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_MIGHT, "might", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "Your strength fades.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_PARALYZE, "paralyze", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "Your feel like you are able to move again.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_POISON, "poison", 50, 20, 3, POS_STANDING,
         spell_target::char_room | spell_target::not_self | spell_target::obj_inv, true,
         mag_routine::action_full | mag_routine::affects | mag_routine::alter_objs, mag_save::fort | mag_save::none, {},
         "You feel like you got over something.",
         4, SCHOOL_NECROMANCY, DOMAIN_UNIVERSAL);

  spello(g, SKILL_POISON, "dark metamorphosis", 50, 20, 3, POS_STANDING,
         spell_target::char_room | spell_target::not_self | spell_target::obj_inv, true,
         mag_routine::action_full | mag_routine::affects | mag_routine::alter_objs, mag_save::fort | mag_save::none, {},
         "Your dark metamorphosis fades.",
         4, SCHOOL_NECROMANCY, DOMAIN_UNIVERSAL);

  spello(g, SKILL_POISON, "healing glow", 50, 20, 3, POS_STANDING,
         spell_target::char_room | spell_target::not_self | spell_target::obj_inv, true,
         mag_routine::action_full | mag_routine::affects | mag_routine::alter_objs, mag_save::fort | mag_save::none, {},
         "Your healing glow fades.",
         4, SCHOOL_NECROMANCY, DOMAIN_UNIVERSAL);

  spello(g, SKILL_ENLIGHTEN, "enlighten", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "You feel less wise.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_GENIUS, "genius", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "You am dumb dumbner now.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SKILL_FLEX, "flex", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "You feel less agile.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_PORTAL, "portal", 75, 75, 0, POS_STANDING,
         spell_target::char_world | spell_target::not_self, false, mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         7, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_PROT_FROM_EVIL, "protection from evil", 40, 10, 3, POS_STANDING,
         spell_target::char_room | spell_target::self_only, false, mag_routine::action_full | mag_routine::affects, {}, {},
         "You feel less protected.",
         1, SCHOOL_ABJURATION, DOMAIN_GOOD);

  spello(g, SPELL_REMOVE_CURSE, "remove curse", 45, 25, 5, POS_STANDING,
         spell_target::char_room | spell_target::obj_inv | spell_target::obj_equip, false,
         mag_routine::action_full | mag_routine::unaffects | mag_routine::alter_objs, {}, {},
         {},
         3, SCHOOL_ABJURATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_NEUTRALIZE_POISON, "neutralize poison", 40, 8, 4, POS_STANDING,
         spell_target::char_room | spell_target::obj_inv | spell_target::obj_room, false, mag_routine::action_full | mag_routine::unaffects | mag_routine::alter_objs, {}, {},
         {},
         4, SCHOOL_CONJURATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_SANCTUARY, "sanctuary", 110, 85, 5, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         "The white aura around your body fades.",
         9, SCHOOL_UNDEFINED, DOMAIN_PROTECTION);

  spello(g, SPELL_SENSE_LIFE, "sense life", 20, 10, 2, POS_STANDING,
         spell_target::char_room | spell_target::self_only, false, mag_routine::action_full | mag_routine::affects, {}, {},
         "You feel less aware of your surroundings.",
         2, SCHOOL_DIVINATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_SHOCKING_GRASP, "shocking grasp", 30, 15, 3, POS_FIGHTING,
         spell_target::char_room | spell_target::fight_vict, true, mag_routine::action_full | mag_routine::damage, {}, {},
         {},
         1, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SLEEP, "sleep", 40, 25, 5, POS_FIGHTING,
         spell_target::char_room, true, mag_routine::action_full | mag_routine::affects, mag_save::will | mag_save::none, {},
         "You feel like you can wake up again.",
         1, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED);

  spello(g, SPELL_HAYASA, "hayasa", 40, 25, 5, POS_FIGHTING,
         spell_target::char_room, true, mag_routine::action_full | mag_routine::affects, mag_save::will | mag_save::none, {},
         "You feel your speed decrease as Hayasa fades.",
         1, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED);

  spello(g, SKILL_TSKIN, "tough skin", 25, 10, 1, POS_STANDING,
         spell_target::char_room | spell_target::not_self, true, mag_routine::action_full | mag_routine::affectsv, mag_save::fort | mag_save::none, {},
         "Your skin isn't quite so thick anymore.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_BULL_STRENGTH, "bull strength", 35, 30, 1, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         "You feel weaker.",
         2, SCHOOL_TRANSMUTATION, DOMAIN_STRENGTH);

  spello(g, SPELL_SUMMON, "summon", 75, 50, 3, POS_STANDING,
         spell_target::char_world | spell_target::not_self, false, mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         7, SCHOOL_CONJURATION, DOMAIN_UNIVERSAL);

  spello(g, SPELL_TELEPORT, "teleport", 75, 50, 3, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         5, SCHOOL_TRANSMUTATION, DOMAIN_TRAVEL);

  spello(g, SPELL_WATERWALK, "waterwalk", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         "Your feet seem less buoyant.",
         3, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_WORD_OF_RECALL, "word of recall", 20, 10, 2, POS_FIGHTING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::manual, {}, {},
         {},
         6, SCHOOL_UNDEFINED, DOMAIN_UNIVERSAL);

  spello(g, SPELL_RESISTANCE, "resistance", 40, 20, 0, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affectsv,
         {}, mag_component::material | mag_component::somatic | mag_component::verbal | mag_component::divine_focus,
         {}, 0, SCHOOL_ABJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_ACID_SPLASH, "acid splash", 40, 20, 0, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects,
         {}, mag_component::somatic | mag_component::verbal,
         {},
         0, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_DAZE, "daze", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affectsv,
         mag_save::will | mag_save::none, mag_component::material | mag_component::somatic | mag_component::verbal,
         {}, 0, SCHOOL_ENCHANTMENT, DOMAIN_UNDEFINED);

  spello(g, SPELL_FLARE, "flare", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affectsv,
         mag_save::fort | mag_save::none, mag_component::verbal,
         {},
         0, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_RAY_OF_FROST, "ray of frost", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects,
         {}, mag_component::somatic | mag_component::verbal,
         {},
         0, SCHOOL_EVOCATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_DISRUPT_UNDEAD, "disrupt undead", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects,
         {}, mag_component::somatic | mag_component::verbal,
         {},
         0, SCHOOL_NECROMANCY, DOMAIN_UNDEFINED);

  /*spello(SPELL_LESSER_GLOBE_OF_INVUL, "lesser globe of invulnerability", 40, 20, 2, POS_STANDING,
	spell_target::char_room, FALSE, mag_routine::action_full | mag_routine::affects, 
	0, mag_component::material | mag_component::somatic | mag_component::verbal,
	NULL,
	0, SCHOOL_ABJURATION, DOMAIN_UNDEFINED); */

  /*spello(SPELL_MINOR_CREATION, "minor creation", 40, 20, 2, POS_STANDING,
	spell_target::char_room, FALSE, mag_routine::action_full | mag_routine::affects, 0, 0,
	NULL,
	0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED); */

  spello(g, SPELL_SUMMON_MONSTER_I, "summon monster i", 40, 20, 2, POS_FIGHTING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         1, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_II, "summon monster ii", 40, 20, 2, POS_FIGHTING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         2, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_III, "summon monster iii", 40, 20, 2, POS_FIGHTING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         3, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_IV, "summon monster iv", 40, 20, 2, POS_FIGHTING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         4, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_V, "summon monster v", 40, 20, 2, POS_FIGHTING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         5, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_VI, "summon monster vi", 40, 20, 2, POS_FIGHTING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         6, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_VII, "summon monster vii", 40, 20, 2, POS_FIGHTING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         7, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_VIII, "summon monster viii", 40, 20, 2, POS_FIGHTING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         8, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_SUMMON_MONSTER_IX, "summon monster ix", 40, 20, 2, POS_FIGHTING,
         spell_target::ignore, false, mag_routine::action_full | mag_routine::summons, {}, {},
         {},
         9, SCHOOL_CONJURATION, DOMAIN_UNDEFINED);

  spello(g, SPELL_FIRE_SHIELD, "fire shield", 40, 20, 2, POS_FIGHTING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affectsv, {}, {},
         {},
         4, SCHOOL_EVOCATION, DOMAIN_FIRE);

  spello(g, SPELL_ICE_STORM, "ice storm", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_SHOUT, "shout", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_FEAR, "fear", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_CLOUDKILL, "cloudkill", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_MAJOR_CREATION, "major creation", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_HOLD_MONSTER, "hold monster", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_CONE_OF_COLD, "cone of cold", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_ANIMAL_GROWTH, "animal growth", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_BALEFUL_POLYMORPH, "baleful polymorph", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);

  spello(g, SPELL_PASSWALL, "passwall", 40, 20, 2, POS_STANDING,
         spell_target::char_room, false, mag_routine::action_full | mag_routine::affects, {}, {},
         {},
         0, SCHOOL_UNDEFINED, DOMAIN_UNDEFINED);
  /*
   * These spells are currently not used, not implemented, and not castable.
   * Values for the 'breath' spells are filled in assuming a dragon's breath.
   */

  spello(g, SPELL_FIRE_BREATH, "fire breath", 0, 0, 0, POS_SITTING,
         spell_target::ignore, true, {}, {}, {},
         {},
         0, 0, 0);

  spello(g, SPELL_GAS_BREATH, "gas breath", 0, 0, 0, POS_SITTING,
         spell_target::ignore, true, {}, {}, {},
         {},
         0, 0, 0);

  spello(g, SPELL_FROST_BREATH, "frost breath", 0, 0, 0, POS_SITTING,
         spell_target::ignore, true, {}, {}, {},
         {},
         0, 0, 0);

  spello(g, SPELL_ACID_BREATH, "acid breath", 0, 0, 0, POS_SITTING,
         spell_target::ignore, true, {}, {}, {},
         {},
         0, 0, 0);

  spello(g, SPELL_LIGHTNING_BREATH, "lightning breath", 0, 0, 0, POS_SITTING,
         spell_target::ignore, true, {}, {}, {},
         {},
         0, 0, 0);

  /* you might want to name this one something more fitting to your theme -Welcor*/
  spello(g, SPELL_DG_AFFECT, "Script-inflicted", 0, 0, 0, POS_SITTING,
         spell_target::ignore, true, {}, {}, {},
         {},
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
  skillo(g, SKILL_FLEX, "flex", skill_flag::chamod | skill_flag::needtrain);
  skillo(g, SKILL_GENIUS, "genius", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_ENLIGHTEN, "enlighten", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_TSKIN, "tough skin", skill_flag::strmod | skill_flag::armorall);
  skillo(g, SKILL_KAIOKEN, "kaioken", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_BLESS, "bless", skill_flag::wismod | skill_flag::needtrain);
  skillo(g, SKILL_CURSE, "curse", skill_flag::wismod | skill_flag::needtrain);
  skillo(g, SKILL_POISON, "poison", skill_flag::wismod | skill_flag::needtrain);
  skillo(g, SKILL_VIGOR, "vigor", skill_flag::wismod | skill_flag::needtrain);
  skillo(g, SKILL_POSE, "special pose", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_HASSHUKEN, "hasshuken", skill_flag::intmod | skill_flag::needtrain);

  /* Effect Skills */
  skillo(g, SKILL_GARDENING, "gardening", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_EXTRACT, "extract", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_RUNIC, "runic", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_COMMUNE, "commune", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_SOLARF, "solar flare", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_MIGHT, "might", skill_flag::strmod | skill_flag::needtrain);
  skillo(g, SKILL_BALANCE, "balance", skill_flag::dexmod | skill_flag::armorall);
  skillo(g, SKILL_BUILD, "build", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_CONCENTRATION, "concentration", skill_flag::conmod);
  skillo(g, SKILL_SPOT, "spot", skill_flag::wismod);
  skillo(g, SKILL_FIRST_AID, "first aid", skill_flag::wismod | skill_flag::needtrain);
  skillo(g, SKILL_DISGUISE, "disguise", skill_flag::chamod);
  skillo(g, SKILL_ESCAPE_ARTIST, "escape", skill_flag::dexmod | skill_flag::armorall);
  skillo(g, SKILL_APPRAISE, "appraise", skill_flag::intmod);
  skillo(g, SKILL_HEAL, "heal", skill_flag::wismod | skill_flag::armorbad);
  skillo(g, SKILL_FORGERY, "forgery", skill_flag::intmod);
  skillo(g, SKILL_HIDE, "hide", skill_flag::dexmod | skill_flag::armorall);
  skillo(g, SKILL_LISTEN, "listen", skill_flag::wismod);
  skillo(g, SKILL_EAVESDROP, "eavesdrop", skill_flag::intmod);
  skillo(g, SKILL_CURE, "cure poison", skill_flag::wismod | skill_flag::needtrain);
  skillo(g, SKILL_OPEN_LOCK, "open lock", skill_flag::dexmod | skill_flag::needtrain | skill_flag::armorbad);
  skillo(g, SKILL_REGENERATE, "regenerate", skill_flag::conmod | skill_flag::needtrain);
  skillo(g, SKILL_KEEN, "keen sight", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_SEARCH, "search", skill_flag::intmod);
  skillo(g, SKILL_MOVE_SILENTLY, "move silently", skill_flag::dexmod | skill_flag::armorall);
  skillo(g, SKILL_ABSORB, "absorb", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_SLEIGHT_OF_HAND, "sleight of hand", skill_flag::dexmod | skill_flag::armorall);
  skillo(g, SKILL_INGEST, "ingest", skill_flag::strmod | skill_flag::needtrain);
  skillo(g, SKILL_REPAIR, "fix", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_SENSE, "sense", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_SURVIVAL, "survival", skill_flag::wismod | skill_flag::needtrain);
  skillo(g, SKILL_YOIK, "yoikominminken", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_CREATE, "create", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_SPIT, "stone spit", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_POTENTIAL, "potential release", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_TELEPATHY, "telepathy", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_FOCUS, "focus", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_INSTANTT, "instant transmission", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);

  /* Weapon Skills */
  skillo(g, SKILL_SWORD, "sword", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_DAGGER, "dagger", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_CLUB, "club", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_SPEAR, "spear", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_GUN, "gun", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_BRAWL, "brawl", skill_flag::intmod | skill_flag::needtrain);

  /* Defensive Skills */
  skillo(g, SKILL_DODGE, "dodge", skill_flag::chamod | skill_flag::needtrain);
  skillo(g, SKILL_PARRY, "parry", skill_flag::dexmod | skill_flag::needtrain);
  skillo(g, SKILL_BLOCK, "block", skill_flag::dexmod | skill_flag::needtrain);
  skillo(g, SKILL_ZANZOKEN, "zanzoken", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_BARRIER, "barrier", skill_flag::intmod | skill_flag::needtrain);

  /* Offensive Skills */
  skillo(g, SKILL_THROW, "throw", skill_flag::dexmod | skill_flag::needtrain);
  skillo(g, SKILL_PUNCH, "punch", skill_flag::strmod | skill_flag::needtrain);
  skillo(g, SKILL_KICK, "kick", skill_flag::strmod | skill_flag::needtrain);
  skillo(g, SKILL_ELBOW, "elbow", skill_flag::strmod | skill_flag::needtrain);
  skillo(g, SKILL_KNEE, "knee", skill_flag::strmod | skill_flag::needtrain);
  skillo(g, SKILL_ROUNDHOUSE, "roundhouse", skill_flag::strmod | skill_flag::needtrain);
  skillo(g, SKILL_UPPERCUT, "uppercut", skill_flag::strmod | skill_flag::needtrain);
  skillo(g, SKILL_SLAM, "slam", skill_flag::strmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_HEELDROP, "heeldrop", skill_flag::strmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_KIBALL, "kiball", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_KIBLAST, "kiblast", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_BEAM, "beam", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_TSUIHIDAN, "tsuihidan", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_SHOGEKIHA, "shogekiha", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_RENZO, "renzokou energy dan", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_MASENKO, "masenko", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_DODONPA, "dodonpa", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_GALIKGUN, "galik gun", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_KAMEHAMEHA, "kamehameha", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_DEATHBEAM, "deathbeam", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_ERASER, "eraser cannon", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_TSLASH, "twin slash", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_PSYBLAST, "psychic blast", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_HONOO, "honoo", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_DUALBEAM, "dual beam", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_ROGAFUFUKEN, "rogafufuken", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_BAKUHATSUHA, "bakuhatsuha", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_KIENZAN, "kienzan", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_TRIBEAM, "tribeam", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_SBC, "special beam cannon", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_FINALFLASH, "final flash", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_CRUSHER, "crusher ball", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_DDSLASH, "darkness dragon slash", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_PBARRAGE, "psychic barrage", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_HELLFLASH, "hell flash", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_HELLSPEAR, "hell spear blast", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_KAKUSANHA, "kakusanha", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_SCATTER, "scatter shot", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_BIGBANG, "big bang", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_PSLASH, "phoenix slash", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_DEATHBALL, "deathball", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_SPIRITBALL, "spirit ball", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_GENKIDAMA, "genki dama", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier5);
  skillo(g, SKILL_GENOCIDE, "genocide", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier5);
  skillo(g, SKILL_DUALWIELD, "dual wield", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_TWOHAND, "twohand", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_STYLE, "fighting arts", skill_flag::intmod);
  skillo(g, SKILL_KURA, "kuraiiro seiki", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_TAILWHIP, "tailwhip", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier1);
  skillo(g, SKILL_KOUSENGAN, "kousengan", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier1);
  skillo(g, SKILL_TAISHA, "taisha reiki", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_PARALYZE, "paralyze", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_INFUSE, "infuse", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_ROLL, "roll", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_TRIP, "trip", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_GRAPPLE, "grapple", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_WSPIKE, "water spikes", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_SELFD, "self destruct", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_SPIRAL, "spiral comet", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_BREAKER, "star breaker", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_MIMIC, "mimic", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_WRAZOR, "water razor", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_KOTEIRU, "koteiru bakuha", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_DIMIZU, "dimizu toride", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_HYOGA_KABE, "hyoga kabe", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_WELLSPRING, "wellspring", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_AQUA_BARRIER, "aqua barrier", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_WARP, "warp pool", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_HSPIRAL, "hell spiral", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_ARMOR, "nanite armor", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_FIRESHIELD, "fireshield", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_COOKING, "cooking", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_SEISHOU, "seishou enko", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier2);
  skillo(g, SKILL_SILK, "silk", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_BASH, "bash", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_HEADBUTT, "headbutt", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_ENSNARE, "ensnare", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_STARNOVA, "starnova", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_MALICE, "malice breaker", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_ZEN, "zen blade strike", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier3);
  skillo(g, SKILL_SUNDER, "sundering force", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_WITHER, "wither", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_METAMORPH, "dark metamorphosis", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_HAYASA, "hayasa", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_ENERGIZE, "energize throwing", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_PURSUIT, "pursuit", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_HEALGLOW, "healing glow", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_HANDLING, "handling", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_MYSTICMUSIC, "mystic music", skill_flag::intmod | skill_flag::needtrain);
  skillo(g, SKILL_LIGHTGRENADE, "light grenade", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_MULTIFORM, "multiform", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier1);
  skillo(g, SKILL_SPIRITCONTROL, "spirit control", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier1);
  skillo(g, SKILL_BALEFIRE, "balefire", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier4);
  skillo(g, SKILL_BLESSEDHAMMER, "blessed hammer", skill_flag::intmod | skill_flag::needtrain | skill_flag::tier1);
}

bool spell_has_routine(GameWorld *g, int spellnum, flag_set<mag_routine> routines)
{
  return g->spell_info[spellnum].routines.intersects(routines);
}
