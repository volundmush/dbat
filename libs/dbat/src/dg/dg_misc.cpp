/**************************************************************************
*  File: dg_misc.c                                                        *
*  Usage: contains general functions for script usage.                    *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include <cstdlib>
#include "dbat/name_table.hpp"
#include <optional>

#include "dbat/io/reader.hpp"
#include "dbat/args.hpp"

#include "dbat/consts/maximums.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/db.hpp"
#include "dbat/spells.hpp"
#include "dbat/fight.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/extract.hpp"
#include "dbat/affect.hpp"
#include "dbat/game_world.hpp"

/* copied from spell_parser.c: */
#define SINFO g->spell_info[spellnum]

/* The next piece of text between single quotes, as strtok(text, "'") reads it: past any quotes, up to the next
   quote or the end, which the following piece starts after */
static std::optional<std::string_view> take_quote_piece(std::string_view &rest)
{
  size_t start = rest.find_first_not_of('\'');
  if (start == std::string_view::npos) {
    rest = {};
    return std::nullopt;
  }
  size_t stop           = rest.find('\'', start);
  std::string_view piece = rest.substr(start, stop == std::string_view::npos ? std::string_view::npos : stop - start);
  rest                   = stop == std::string_view::npos ? std::string_view{} : rest.substr(stop + 1);
  return piece;
}

/* cast a spell; can be called by mobiles, objects and rooms, and no   */
/* level check is required. Note that mobs should generally use the    */
/* normal 'cast' command (which must be patched to allow mobs to cast  */
/* spells) as the spell system is designed to have a character caster, */
/* and this cast routine may overlook certain issues.                  */
/* LIMITATION: a target MUST exist for the spell unless the spell is   */
/* set to spell_target::ignore. Also, group spells are not permitted             */
/* cast a spell; can be called by mobiles, objects and rooms, and no   */
/* level check is required. Note that mobs should generally use the    */
/* normal 'cast' command (which must be patched to allow mobs to cast  */
/* spells) as the spell system is designed to have a character caster, */
/* and this cast routine may overlook certain issues.                  */
/* LIMITATION: a target MUST exist for the spell unless the spell is   */
/* set to spell_target::ignore. Also, group spells are not permitted             */
/* code borrowed from do_cast() */
void do_dg_cast(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, std::string_view cmd)
{
  struct char_data *caster      = nullptr;
  struct char_data *tch         = nullptr;
  struct obj_data *tobj         = nullptr;
  struct room_data *caster_room = nullptr;
  int spellnum; bool target = false;
  std::optional<std::string> victim; /* the target's name, if anything follows the spell */

  /* need to get the caster or the room of the temporary caster */
  if (auto *mob = std::get_if<char_data *>(&go))
    caster = *mob;
  else if (auto *room = std::get_if<room_data *>(&go))
    caster_room = *room;
  else if (!(caster_room = dg_room_of_obj(g, std::get<obj_data *>(go)))) {
    script_log(g, "dg_do_cast: unknown room for object-caster!");
    return;
  }

  /* get: blank, spell name, target name */
  std::string_view rest = cmd;
  if (!take_quote_piece(rest)) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_cast needs spell name.",
               trig->name, trig_get_vnum(g, trig));
    return;
  }
  std::optional<std::string_view> spell = take_quote_piece(rest);
  if (!spell) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_cast needs spell name in `'s.",
               trig->name, trig_get_vnum(g, trig));
    return;
  }

  /* spellnum = search_block(s, spells, 0); */
  std::string spell_name(*spell);
  spellnum = find_skill_num(g, spell_name, skill_kind::spell);
  if ((spellnum < 0) || (spellnum >= SKILL_TABLE_SIZE || (!skill_type(g, spellnum).test(skill_kind::spell)))) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_cast: invalid spell name ({})",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  /* Find the target */
  if (!rest.empty())
    victim = take_arg(rest);
  if (SINFO.targets.test(spell_target::ignore)) {
    target = true;
  } else if (victim && !victim->empty()) {
    if (!target &&
        (SINFO.targets.test(spell_target::char_room) ||
         SINFO.targets.test(spell_target::char_world))) {
      if ((tch = get_char(g, *victim)) != nullptr)
        target = true;
    }

    if (!target &&
        (SINFO.targets.test(spell_target::obj_inv) ||
         SINFO.targets.test(spell_target::obj_equip) ||
         SINFO.targets.test(spell_target::obj_room) ||
         SINFO.targets.test(spell_target::obj_world))) {
      if ((tobj = get_obj(g, *victim)) != nullptr)
        target = true;
    }

    if (!target) {
      script_log(g, "Trigger: {}, VNum {:d}. dg_cast: target not found ({})",
                 trig->name, trig_get_vnum(g, trig), cmd);
      return;
    }
  }

  if (SINFO.routines.test(mag_routine::groups)) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_cast: group spells not permitted ({})",
               trig->name, trig_get_vnum(g, trig), cmd);
    return;
  }

  std::string_view arg = victim ? std::string_view(*victim) : std::string_view();
  if (!caster) {
    caster = read_mobile(g, DG_CASTER_PROXY);
    if (!caster) {
      script_log(g, "dg_cast: Cannot load the caster mob!");
      return;
    }
    /* set the caster's name to that of the object, or the gods.... */
    if (auto *obj = std::get_if<obj_data *>(&go)) {
      char_set_sdesc(caster, obj_get_short_description(*obj));
    } else if (std::holds_alternative<room_data *>(go))
      char_set_sdesc(caster, "The gods");
    caster_room->people.add(char_get_id(caster)); /* there, without char_to_room()'s side effects */
    char_set_in_room(caster, room_get_number(caster_room));
    call_magic(g, caster, tch, tobj, spellnum, DG_SPELL_LEVEL, CAST_SPELL, arg);
    extract_char(g, caster);
  } else
    call_magic(g, caster, tch, tobj, spellnum, char_get_level(caster), CAST_SPELL, arg);
}

/* modify an affection on the target. affections can be of the AFF_x  */
/* variety or APPLY_x type. APPLY_x's have an integer value for them  */
/* while AFF_x's have boolean values. In any case, the duration MUST  */
/* be non-zero.                                                       */
/* usage:  apply <target> <property> <value> <duration>               */
constexpr int APPLY_TYPE  = 1;
constexpr int AFFECT_TYPE = 2;
void do_dg_affect(GameWorld *g, owner_ptr go, struct script_data *sc, trig_data *trig, std::string_view cmd)
{
  struct char_data *ch = nullptr;
  int value = 0, duration = 0;
  int i = 0, type = 0;
  struct affected_type af = {};

  std::string_view rest  = cmd;
  take_word(rest); /* "dg_affect" */
  std::string charname   = take_word(rest);
  std::string property   = take_word(rest);
  std::string value_p    = take_word(rest);
  std::string duration_p = std::string(skip_spaces(rest)); /* the rest, as it is */

  /* make sure all parameters are present */
  if (charname.empty() || property.empty() || value_p.empty() || duration_p.empty()) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_affect usage: <target> <property> <value> <duration>",
               trig->name, trig_get_vnum(g, trig));
    return;
  }

  value    = parse_int(value_p);
  duration = parse_int(duration_p);
  if (duration <= 0) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_affect: need positive duration!",
               trig->name, trig_get_vnum(g, trig));
    script_log(g, "Line was: dg_affect {} {} {} {} ({:d})",
               charname, property, value_p, duration_p, duration);
    return;
  }

  /* find the property -- first search apply_types */
  i = 0;
  while (i < NUM_APPLIES) {
    if (str_eq_nocase(apply_types[i], property)) {
      type = APPLY_TYPE;
      break;
    }
    i++;
  }

  if (!type) { /* search affect_types now */
    i = 0;
    while (std::cmp_less(i, flag_count<aff_flag>)) {
      if (str_eq_nocase(affected_bits[i], property)) {
        type = AFFECT_TYPE;
        break;
      }
      i++;
    }
  }

  if (!type) { /* property not found */
    script_log(g, "Trigger: {}, VNum {:d}. dg_affect: unknown property '{}'!",
               trig->name, trig_get_vnum(g, trig), property);
    return;
  }

  /* locate the target */
  ch = get_char(g, charname);
  if (!ch) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_affect: cannot locate target!",
               trig->name, trig_get_vnum(g, trig));
    return;
  }

  if (str_eq_nocase(value_p, "off")) {
    affect_from_char(g, ch, SPELL_DG_AFFECT);
    return;
  }

  /* add the affect */
  af.type     = SPELL_DG_AFFECT;
  af.duration = duration - 1;
  af.modifier = value;

  if (type == APPLY_TYPE) {
    af.location  = i;
    af.bitvector = aff_flag::dontuse;
  } else {
    af.location  = 0;
    af.bitvector = *flag_from_index<aff_flag>(i); /* found among the affects' names above */
  }

  affect_to_char(g, ch, &af);
}

static void send_char_pos(GameWorld *g, struct char_data *ch, int dam)
{
  switch (char_get_position(ch)) {
    case POS_MORTALLYW:
      act(g, "$n is mortally wounded, and will die soon, if not aided.", true, ch, nullptr, nullptr, act_to::room);
      send_to_char(g, ch, "You are mortally wounded, and will die soon, if not aided.\r\n");
      break;
    case POS_INCAP:
      act(g, "$n is incapacitated and will slowly die, if not aided.", true, ch, nullptr, nullptr, act_to::room);
      send_to_char(g, ch, "You are incapacitated and will slowly die, if not aided.\r\n");
      break;
    case POS_STUNNED:
      act(g, "$n is stunned, but will probably regain consciousness again.", true, ch, nullptr, nullptr, act_to::room);
      send_to_char(g, ch, "You're stunned, but will probably regain consciousness again.\r\n");
      break;
    case POS_DEAD:
      act(g, "$n is dead!  R.I.P.", false, ch, nullptr, nullptr, act_to::room);
      send_to_char(g, ch, "You are dead!  Sorry...\r\n");
      break;
    default: /* >= POSITION SLEEPING */
      if (dam > (getEffMaxPL(g, ch) >> 2))
        act(g, "That really did HURT!", false, ch, nullptr, nullptr, act_to::actor);
      if (getCurPL(g, ch) < (getEffMaxPL(g, ch) >> 2))
        send_to_char(g, ch, "@rYou wish that your wounds would stop BLEEDING so much!@n\r\n");
  }
}

/* Used throughout the xxxcmds.c files for checking if a char
 * can be targetted
 * - gods is refuse when called by %force%, for instance,
 * while allow for %teleport%.  -- Welcor
 */
bool valid_dg_target(GameWorld *g, struct char_data *ch, dg_gods gods)
{
  if (char_get_in_room(ch) == NOWHERE)
    return false; /* on the way out of the game, such as someone who has just quit */
  if (char_is_npc(ch))
    return true; /* all npcs are allowed as targets */
  else if (char_get_admlevel(ch) < ADMLVL_IMMORT)
    return true; /* as well as all mortals */
  else if (gods == dg_gods::refuse &&
           (char_get_admlevel(ch) >= 2 && !char_prf_flagged(g, ch, pref_flag::test))) /* LVL_GOD has the advance command. Can't allow them to be forced. */
    return false;                                                           /* but not always the highest gods */
  else if (!char_prf_flagged(g, ch, pref_flag::nohassle) || char_prf_flagged(g, ch, pref_flag::test))
    return true; /* the ones in between as allowed as long as they have no-hassle off.   */
  else
    return false; /* The rest are gods with nohassle on... */
}

void script_damage(GameWorld *g, struct char_data *vict, int dam)
{
  if (char_adm_flagged(vict, admin_flag::nodamage) && (dam > 0)) {
    send_to_char(g, vict, "Being the cool immortal you are, you sidestep a trap, obviously placed to kill you.\r\n");
    return;
  }

  decCurHealth(g, vict, dam, 0);

  update_pos(g, vict);
  send_char_pos(g, vict, dam);

  if (char_get_position(vict) == POS_DEAD) {
    if (!char_is_npc(vict))
      mudlog(g, LOG_BRF, 0, true, "{} killed by script at {}",
             char_get_name(vict), room_get_name(g, char_get_in_room(vict)));
    die(g, vict, nullptr);
  }
}

/* the fields a script's door command (mdoor, odoor, wdoor) can change, numbered as dg_door_apply() takes them */
static constexpr std::string_view dg_door_fields_entries[] = {
    "purge",
    "description",
    "flags",
    "key",
    "name",
    "room"};
static_assert(std::size(dg_door_fields_entries) == NUM_DOOR_FIELDS);
constinit const name_table dg_door_fields(dg_door_fields_entries);

/*
 * Sets field (an index into dg_door_fields) of exit dir of room rm to value, creating the
 * exit first if needed; "purge" removes the exit. Returns false if field is "room" and
 * value is not a room vnum, so the caller can log it.
 */
bool dg_door_apply(GameWorld *g, room_data *rm, int dir, int field, std::string_view value)
{
  struct room_direction_data *newexit = room_get_dir_option(rm, dir);
  room_vnum to_room;

  /* purge exit */
  if (field == 0) {
    if (newexit)
      room_set_dir_option(rm, dir, nullptr);
    return true;
  }

  if (!newexit) {
    newexit = new room_direction_data();
    room_set_dir_option(rm, dir, newexit);
  }

  switch (field) {
    case 1: /* description */
      newexit->general_description = std::string(value) + "\r\n";
      break;
    case 2: { /* flags       */
      std::string flags(value);
      /* quirk: the field was 16 bits, so a letter past p is dropped and p (bit 15) reads as every bit from 15 up */
      const auto word = static_cast<int16_t>(io::ascii_flags(flags, 0, "asciiflag_conv"));
      newexit->exit_info.set_word(0, static_cast<bitvector_t>(word));
      break;
    }
    case 3: /* key         */
      newexit->key = parse_int(value);
      break;
    case 4: /* name        */
      newexit->keyword = value;
      break;
    case 5: /* room        */
      if (room_of(g, to_room = parse_int(value)))
        newexit->to_room = to_room;
      else
        return false;
      break;
  }
  return true;
}
