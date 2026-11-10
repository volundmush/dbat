/**************************************************************************
*  File: dg_misc.c                                                        *
*  Usage: contains general functions for script usage.                    *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include <cstdlib>

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

/* cast a spell; can be called by mobiles, objects and rooms, and no   */
/* level check is required. Note that mobs should generally use the    */
/* normal 'cast' command (which must be patched to allow mobs to cast  */
/* spells) as the spell system is designed to have a character caster, */
/* and this cast routine may overlook certain issues.                  */
/* LIMITATION: a target MUST exist for the spell unless the spell is   */
/* set to TAR_IGNORE. Also, group spells are not permitted             */
/* code borrowed from do_cast() */
void do_dg_cast(GameWorld *g, void *go, struct script_data *sc, trig_data *trig,
                int type, char *cmd)
{
  struct char_data *caster      = nullptr;
  struct char_data *tch         = nullptr;
  struct obj_data *tobj         = nullptr;
  struct room_data *caster_room = nullptr;
  char *s, *t;
  int spellnum, target = 0;
  char buf2[MAX_STRING_LENGTH], orig_cmd[MAX_INPUT_LENGTH];

  /* need to get the caster or the room of the temporary caster */
  switch (type) {
    case MOB_TRIGGER:
      caster = (struct char_data *)go;
      break;
    case WLD_TRIGGER:
      caster_room = (struct room_data *)go;
      break;
    case OBJ_TRIGGER:
      caster_room = dg_room_of_obj(g, (struct obj_data *)go);
      if (!caster_room) {
        script_log(g, "dg_do_cast: unknown room for object-caster!");
        return;
      }
      break;
    default:
      script_log(g, "dg_do_cast: unknown trigger type!");
      return;
  }

  strcpy(orig_cmd, cmd);
  /* get: blank, spell name, target name */
  s = strtok(cmd, "'");
  if (s == nullptr) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_cast needs spell name.",
               trig->name.c_str(), trig_get_vnum(g, trig));
    return;
  }
  s = strtok(nullptr, "'");
  if (s == nullptr) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_cast needs spell name in `'s.",
               trig->name.c_str(), trig_get_vnum(g, trig));
    return;
  }
  t = strtok(nullptr, "\0");

  /* spellnum = search_block(s, spells, 0); */
  spellnum = find_skill_num(g, s, SKTYPE_SPELL);
  if ((spellnum < 0) || (spellnum >= SKILL_TABLE_SIZE || (!IS_SET(skill_type(g, spellnum), SKTYPE_SPELL)))) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_cast: invalid spell name ({})",
               trig->name.c_str(), trig_get_vnum(g, trig), orig_cmd);
    return;
  }

  /* Find the target */
  if (t != nullptr) {
    one_argument(strcpy(buf2, t), t);
    skip_spaces(&t);
  }
  if (IS_SET(SINFO.targets, TAR_IGNORE)) {
    target = true;
  } else if (t != nullptr && *t) {
    if (!target &&
        (IS_SET(SINFO.targets, TAR_CHAR_ROOM) ||
         IS_SET(SINFO.targets, TAR_CHAR_WORLD))) {
      if ((tch = get_char(g, t)) != nullptr)
        target = true;
    }

    if (!target &&
        (IS_SET(SINFO.targets, TAR_OBJ_INV) ||
         IS_SET(SINFO.targets, TAR_OBJ_EQUIP) ||
         IS_SET(SINFO.targets, TAR_OBJ_ROOM) ||
         IS_SET(SINFO.targets, TAR_OBJ_WORLD))) {
      if ((tobj = get_obj(g, t)) != nullptr)
        target = true;
    }

    if (!target) {
      script_log(g, "Trigger: {}, VNum {:d}. dg_cast: target not found ({})",
                 trig->name.c_str(), trig_get_vnum(g, trig), orig_cmd);
      return;
    }
  }

  if (IS_SET(SINFO.routines, MAG_GROUPS)) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_cast: group spells not permitted ({})",
               trig->name.c_str(), trig_get_vnum(g, trig), orig_cmd);
    return;
  }

  if (!caster) {
    caster = read_mobile(g, DG_CASTER_PROXY);
    if (!caster) {
      script_log(g, "dg_cast: Cannot load the caster mob!");
      return;
    }
    /* set the caster's name to that of the object, or the gods.... */
    if (type == OBJ_TRIGGER)
      char_set_sdesc(caster, obj_get_short_description((struct obj_data *)go));
    else if (type == WLD_TRIGGER)
      char_set_sdesc(caster, "The gods");
    caster_room->people.add(char_get_id(caster)); /* there, without char_to_room()'s side effects */
    char_set_in_room(caster, room_get_number(caster_room));
    call_magic(g, caster, tch, tobj, spellnum, DG_SPELL_LEVEL, CAST_SPELL, t);
    extract_char(g, caster);
  } else
    call_magic(g, caster, tch, tobj, spellnum, char_get_level(caster), CAST_SPELL, t);
}

/* modify an affection on the target. affections can be of the AFF_x  */
/* variety or APPLY_x type. APPLY_x's have an integer value for them  */
/* while AFF_x's have boolean values. In any case, the duration MUST  */
/* be non-zero.                                                       */
/* usage:  apply <target> <property> <value> <duration>               */
constexpr int APPLY_TYPE  = 1;
constexpr int AFFECT_TYPE = 2;
void do_dg_affect(GameWorld *g, void *go, struct script_data *sc, trig_data *trig, int script_type, char *cmd)
{
  struct char_data *ch = nullptr;
  int value = 0, duration = 0;
  char junk[MAX_INPUT_LENGTH]; /* will be set to "dg_affect" */
  char charname[MAX_INPUT_LENGTH], property[MAX_INPUT_LENGTH];
  char value_p[MAX_INPUT_LENGTH], duration_p[MAX_INPUT_LENGTH];
  int i = 0, type = 0;
  struct affected_type af = {};

  half_chop(cmd, junk, cmd);
  half_chop(cmd, charname, cmd);
  half_chop(cmd, property, cmd);
  half_chop(cmd, value_p, duration_p);

  /* make sure all parameters are present */
  if (!*charname || !*property ||
      !*value_p || !*duration_p) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_affect usage: <target> <property> <value> <duration>",
               trig->name.c_str(), trig_get_vnum(g, trig));
    return;
  }

  value    = atoi(value_p);
  duration = atoi(duration_p);
  if (duration <= 0) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_affect: need positive duration!",
               trig->name.c_str(), trig_get_vnum(g, trig));
    script_log(g, "Line was: dg_affect {} {} {} {} ({:d})",
               charname, property, value_p, duration_p, duration);
    return;
  }

  /* find the property -- first search apply_types */
  i = 0;
  while (strcasecmp(apply_types[i], "\n")) {
    if (!strcasecmp(apply_types[i], property)) {
      type = APPLY_TYPE;
      break;
    }
    i++;
  }

  if (!type) { /* search affect_types now */
    i = 0;
    while (strcasecmp(affected_bits[i], "\n")) {
      if (!strcasecmp(affected_bits[i], property)) {
        type = AFFECT_TYPE;
        break;
      }
      i++;
    }
  }

  if (!type) { /* property not found */
    script_log(g, "Trigger: {}, VNum {:d}. dg_affect: unknown property '{}'!",
               trig->name.c_str(), trig_get_vnum(g, trig), property);
    return;
  }

  /* locate the target */
  ch = get_char(g, charname);
  if (!ch) {
    script_log(g, "Trigger: {}, VNum {:d}. dg_affect: cannot locate target!",
               trig->name.c_str(), trig_get_vnum(g, trig));
    return;
  }

  if (!strcasecmp(value_p, "off")) {
    affect_from_char(g, ch, SPELL_DG_AFFECT);
    return;
  }

  /* add the affect */
  af.type     = SPELL_DG_AFFECT;
  af.duration = duration - 1;
  af.modifier = value;

  if (type == APPLY_TYPE) {
    af.location  = i;
    af.bitvector = 0;
  } else {
    af.location  = 0;
    af.bitvector = i;
  }

  affect_to_char(g, ch, &af);
}

static void send_char_pos(GameWorld *g, struct char_data *ch, int dam)
{
  switch (char_get_position(ch)) {
    case POS_MORTALLYW:
      act(g, "$n is mortally wounded, and will die soon, if not aided.", true, ch, nullptr, nullptr, TO_ROOM);
      send_to_char(g, ch, "You are mortally wounded, and will die soon, if not aided.\r\n");
      break;
    case POS_INCAP:
      act(g, "$n is incapacitated and will slowly die, if not aided.", true, ch, nullptr, nullptr, TO_ROOM);
      send_to_char(g, ch, "You are incapacitated and will slowly die, if not aided.\r\n");
      break;
    case POS_STUNNED:
      act(g, "$n is stunned, but will probably regain consciousness again.", true, ch, nullptr, nullptr, TO_ROOM);
      send_to_char(g, ch, "You're stunned, but will probably regain consciousness again.\r\n");
      break;
    case POS_DEAD:
      act(g, "$n is dead!  R.I.P.", false, ch, nullptr, nullptr, TO_ROOM);
      send_to_char(g, ch, "You are dead!  Sorry...\r\n");
      break;
    default: /* >= POSITION SLEEPING */
      if (dam > (getEffMaxPL(g, ch) >> 2))
        act(g, "That really did HURT!", false, ch, nullptr, nullptr, TO_CHAR);
      if (getCurPL(g, ch) < (getEffMaxPL(g, ch) >> 2))
        send_to_char(g, ch, "@rYou wish that your wounds would stop BLEEDING so much!@n\r\n");
  }
}

/* Used throughout the xxxcmds.c files for checking if a char
 * can be targetted
 * - allow_gods is false when called by %force%, for instance,
 * while true for %teleport%.  -- Welcor
 */
bool valid_dg_target(GameWorld *g, struct char_data *ch, int bitvector)
{
  if (char_get_in_room(ch) == NOWHERE)
    return false; /* on the way out of the game, such as someone who has just quit */
  if (char_is_npc(ch))
    return true; /* all npcs are allowed as targets */
  else if (char_get_admlevel(ch) < ADMLVL_IMMORT)
    return true; /* as well as all mortals */
  else if (!IS_SET(bitvector, DG_ALLOW_GODS) &&
           (char_get_admlevel(ch) >= 2 && !char_prf_flagged(g, ch, PRF_TEST))) /* LVL_GOD has the advance command. Can't allow them to be forced. */
    return false;                                                           /* but not always the highest gods */
  else if (!char_prf_flagged(g, ch, PRF_NOHASSLE) || char_prf_flagged(g, ch, PRF_TEST))
    return true; /* the ones in between as allowed as long as they have no-hassle off.   */
  else
    return false; /* The rest are gods with nohassle on... */
}

void script_damage(GameWorld *g, struct char_data *vict, int dam)
{
  if (char_adm_flagged(vict, ADM_NODAMAGE) && (dam > 0)) {
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
const char *const dg_door_fields[] = {
    "purge",
    "description",
    "flags",
    "key",
    "name",
    "room",
    "\n"};

/*
 * Sets field (an index into dg_door_fields) of exit dir of room rm to value, creating the
 * exit first if needed; "purge" removes the exit. Returns false if field is "room" and
 * value is not a room vnum, so the caller can log it.
 */
bool dg_door_apply(GameWorld *g, room_data *rm, int dir, int field, char *value)
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
    case 2: /* flags       */
      newexit->exit_info = (int16_t)asciiflag_conv(value);
      break;
    case 3: /* key         */
      newexit->key = atoi(value);
      break;
    case 4: /* name        */
      newexit->keyword = value;
      break;
    case 5: /* room        */
      if (room_of(g, to_room = atoi(value)))
        newexit->to_room = to_room;
      else
        return false;
      break;
  }
  return true;
}
