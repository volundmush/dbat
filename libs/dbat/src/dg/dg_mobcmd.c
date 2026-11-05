/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/
/**************************************************************************
*  File: dg_mobcmd.c                                                      *
*  Usage: contains the mobile script commands.                            *
*                                                                         *
*                                                                         *
*  $Author: N'Atas-ha/Mark A. Heilpern/egreen/Welcor $                    *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/
#include "dbat/consts/maximums.h"

#include "dbat/dg_scripts.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/act.wizard.h"
#include "dbat/fight.h"
#include "dbat/character_utils.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/extract.h"
#include "dbat/random.h"
#include "dbat/stringutils.h"
#include "dbat/act.offensive.h"

#include <stdarg.h>
#include <stdio.h>
#include "dbat/game_world.h"
/*
 * Local functions.
 */
static void mob_log(GameWorld *g, char_data *mob, const char *format, ...) __attribute__((format(printf, 3, 4)));

/* attaches mob's name and vnum to msg and sends it to script_log */
static void mob_log(GameWorld *g, char_data *mob, const char *format, ...)
{
  va_list args;
  char output[MAX_STRING_LENGTH];

  va_start(args, format);
  vsnprintf(output, sizeof(output), format, args);
  va_end(args);

  script_log(g, "Mob (%s, VNum %d):: %s", char_get_sdesc(mob), char_get_mob_vnum(g, mob), output);
}

/*
** macro to determine if a mob is permitted to use these commands
*/
#define MOB_OR_IMPL(ch) \
  (char_is_npc(ch) && (!char_get_desc(ch) || char_get_admlevel(char_get_desc(ch)->original) >= ADMLVL_IMPL))

/*
 * The check every mob command starts with: only NPCs may use them, and a player switched
 * into one only if an implementor. check_charm also refuses a charmed mob, so players
 * can't order their followers to run these. Returns false if the command must not run.
 */
static bool mob_cmd_allowed(GameWorld *g, char_data *ch, bool check_charm)
{
  if (!MOB_OR_IMPL(ch)) {
    send_to_char(g, ch, "Huh?!?\r\n");
    return false;
  }
  if (check_charm && char_aff_flagged(ch, AFF_CHARM))
    return false;
  return true;
}

/* mob commands */

/* prints the argument to all the rooms aroud the mobile */
ACMD(do_masound)
{
  room_rnum was_in_room;
  int door;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  if (!*argument) {
    mob_log(g, ch, "masound called with no argument");
    return;
  }

  skip_spaces(&argument);

  was_in_room = char_get_in_room(ch);
  for (door = 0; door < NUM_OF_DIRS; door++) {
    struct room_direction_data *newexit;

    if (((newexit = rroom_get_dir_option(g, was_in_room, door)) != nullptr) &&
        newexit->to_room != NOWHERE && newexit->to_room != was_in_room) {
      char_set_in_room(ch, newexit->to_room);
      sub_write(g, argument, ch, true, TO_ROOM);
    }
  }

  char_set_in_room(ch, was_in_room);
}

/* Heals a stat of the mob */
ACMD(do_mheal)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  if (!mob_cmd_allowed(g, ch, false))
    return;

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    mob_log(g, ch, "mheal called without an argument");
    return;
  }

  double num = atoi(arg2);

  if (!strcasecmp(arg, "pl")) {
    incCurHealthPercent(g, ch, num, true);
  } else if (!strcasecmp(arg, "ki")) {
    incCurKIPercent(g, ch, num, true);
  } else if (!strcasecmp(arg, "st")) {
    incCurSTPercent(g, ch, num, true);
  } else {
    mob_log(g, ch, "mheal called with wrong argument [pl | ki | st]");
    return;
  }
}

/* lets the mobile kill any player or mobile without murder*/
ACMD(do_mkill)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  char buf[MAX_INPUT_LENGTH];

  if (!mob_cmd_allowed(g, ch, true))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(g, ch, "mkill called with no argument");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(g, arg))) {
      mob_log(g, ch, "mkill: victim (%s) not found", arg);
      return;
    }
  } else if (!(victim = get_char_room_vis(g, ch, arg, nullptr))) {
    mob_log(g, ch, "mkill: victim (%s) not found", arg);
    return;
  }

  if (victim == ch) {
    mob_log(g, ch, "mkill: victim is self");
    return;
  }

  if (!char_is_npc(victim) && char_prf_flagged(g, victim, PRF_NOHASSLE)) {
    mob_log(g, ch, "mkill: target has nohassle on");
    return;
  }

  sprintf(buf, "%s", char_get_name(victim));
  if (char_is_humanoid(ch)) {
    switch (rand_number(g, 1, 7)) {
      case 1:
        do_punch(g, ch, buf, 0, 0);
        break;
      case 2:
        do_kick(g, ch, buf, 0, 0);
        break;
      case 3:
        do_elbow(g, ch, buf, 0, 0);
        break;
      case 4:
        do_knee(g, ch, buf, 0, 0);
        break;
      case 5:
        do_kick(g, ch, buf, 0, 0);
        break;
      default:
        do_punch(g, ch, buf, 0, 0);
        break;
    } //end switch
  } else {
    do_bite(g, ch, buf, 0, 0);
  } //end humanoid if
  return;
}

/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
ACMD(do_mjunk)
{
  char arg[MAX_INPUT_LENGTH];
  int pos, junk_all = 0;
  obj_data *obj;
  obj_data *obj_next;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(g, ch, "mjunk called with no argument");
    return;
  }

  if (!strcasecmp(arg, "all")) junk_all = 1;

  if ((find_all_dots(arg) != FIND_INDIV) && !junk_all) {
    /* Thanks to Carlos Myers for fixing the line below */
    if ((pos = get_obj_pos_in_equip_vis(g, ch, arg, nullptr, char_get_equipment_array(ch))) >= 0) {
      extract_obj(g, unequip_char(g, ch, pos));
      return;
    }
    if ((obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch))) != nullptr)
      extract_obj(g, obj);
    return;
  }
  for (obj = char_get_inventory(ch); obj != nullptr; obj = obj_next) {
    obj_next = obj_get_next_content(obj);
    if (arg[3] == '\0' || isname(arg + 4, obj_get_name(obj))) {
      extract_obj(g, obj);
    }
  }
  /* Thanks to Carlos Myers for fixing the line below */
  while ((pos = get_obj_pos_in_equip_vis(g, ch, arg, nullptr, char_get_equipment_array(ch))) >= 0)
    extract_obj(g, unequip_char(g, ch, pos));
  return;
}

/* prints the message to everyone in the room other than the mob and victim */
ACMD(do_mechoaround)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  char *p;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  p = one_argument(argument, arg);
  skip_spaces(&p);

  if (!*arg) {
    mob_log(g, ch, "mechoaround called with no argument");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(g, arg))) {
      mob_log(g, ch, "mechoaround: victim (%s) does not exist", arg);
      return;
    }
  } else if (!(victim = get_char_room_vis(g, ch, arg, nullptr))) {
    mob_log(g, ch, "mechoaround: victim (%s) does not exist", arg);
    return;
  }

  char buf[MAX_STRING_LENGTH];

  sprintf(buf, "%s", p);
  search_replace(buf, char_get_name(victim), "$n");
  act(g, buf, true, victim, 0, 0, TO_ROOM);
  /*sub_write(p, victim, TRUE, TO_ROOM);*/
}

/* sends the message to only the victim */
ACMD(do_msend)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  char *p;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  p = one_argument(argument, arg);
  skip_spaces(&p);

  if (!*arg) {
    mob_log(g, ch, "msend called with no argument");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(g, arg))) {
      mob_log(g, ch, "msend: victim (%s) does not exist", arg);
      return;
    }
  } else if (!(victim = get_char_room_vis(g, ch, arg, nullptr))) {
    mob_log(g, ch, "msend: victim (%s) does not exist", arg);
    return;
  }

  sub_write(g, p, victim, true, TO_CHAR);
}

/* prints the message to the room at large */
ACMD(do_mecho)
{
  char *p;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  if (!*argument) {
    mob_log(g, ch, "mecho called with no arguments");
    return;
  }
  p = argument;
  skip_spaces(&p);

  sub_write(g, p, ch, true, TO_ROOM);
}

ACMD(do_mzoneecho)
{
  zone_rnum zone;
  char room_number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  msg = any_one_arg(argument, room_number);
  skip_spaces(&msg);

  if (!*room_number || !*msg)
    mob_log(g, ch, "mzoneecho called with too few args");

  else if ((zone = real_zone_by_thing(g, atoi(room_number))) == NOWHERE)
    mob_log(g, ch, "mzoneecho called for nonexistant zone");

  else {
    sprintf(buf, "%s\r\n", msg);
    send_to_zone(g, buf, zone);
  }
}

/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE.
 */
ACMD(do_mload)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  IDXTYPE number = 0;
  char_data *mob;
  obj_data *object;
  char *target;
  char_data *tch;
  obj_data *cnt;
  int pos;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  target = two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
    mob_log(g, ch, "mload: bad syntax");
    return;
  }

  /* load mob to target room - Jamie Nelson, April 13 2004 */
  if (is_abbrev(arg1, "mob")) {
    room_rnum rnum;
    if (!target || !*target) {
      rnum = char_get_in_room(ch);
    } else {
      if (!isdigit(*target) || (rnum = real_room(g, atoi(target))) == NOWHERE) {
        mob_log(g, ch, "mload: room target vnum doesn't exist "
                    "(loading mob vnum %d to room %s)",
                number, target);
        return;
      }
    }
    if ((mob = read_mobile(g, number, VIRTUAL)) == nullptr) {
      mob_log(g, ch, "mload: bad mob vnum");
      return;
    }
    char_to_room(g, mob, rnum);
    if (char_get_script(ch)) { /* It _should_ have, but it might be detached. */
      char buf[MAX_INPUT_LENGTH];
      sprintf(buf, "%c%d", UID_CHAR, char_get_id(mob));
      add_var(&(char_get_script(ch)->global_vars), "lastloaded", buf, 0);
    }
    load_mtrigger(g, mob);
  }

  else if (is_abbrev(arg1, "obj")) {
    if ((object = read_object(g, number, VIRTUAL)) == nullptr) {
      mob_log(g, ch, "mload: bad object vnum");
      return;
    }
    if (char_get_script(ch)) { /* It _should_ have, but it might be detached. */
      char buf[MAX_INPUT_LENGTH];
      sprintf(buf, "%c%d", UID_CHAR, obj_get_id(object));
      add_var(&(char_get_script(ch)->global_vars), "lastloaded", buf, 0);
    }
    randomize_eq(g, object);
    /* special handling to make objects able to load on a person/in a container/worn etc. */
    if (!target || !*target) {
      add_unique_id(g, object);
      if (obj_wear_flagged(object, ITEM_WEAR_TAKE)) {
        obj_to_char(g, object, ch);
      } else {
        obj_to_room(g, object, char_get_in_room(ch));
      }
      load_otrigger(g, object);
      return;
    }
    two_arguments(target, arg1, arg2); /* recycling ... */
    tch = (*arg1 == UID_CHAR) ? get_char(g, arg1) : get_char_room_vis(g, ch, arg1, nullptr);
    if (tch) {
      if (*arg2 &&
          (pos = find_eq_pos_script(arg2)) >= 0 &&
          !char_get_equipment(tch, pos) &&
          can_wear_on_pos(object, pos)) {
        equip_char(g, tch, object, pos);
        load_otrigger(g, object);
        return;
      }
      obj_to_char(g, object, tch);
      load_otrigger(g, object);
      return;
    }
    cnt = (*arg1 == UID_CHAR) ? get_obj(g, arg1) : get_obj_vis(g, ch, arg1, nullptr);
    if (cnt && obj_get_type_flag(cnt) == ITEM_CONTAINER) {
      obj_to_obj(g, object, cnt);
      load_otrigger(g, object);
      return;
    }
    /* neither char nor container found - just dump it in room */
    add_unique_id(g, object);
    obj_to_room(g, object, char_get_in_room(ch));
    load_otrigger(g, object);
    return;
  }

  else
    mob_log(g, ch, "mload: bad type");
}

/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
ACMD(do_mpurge)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  obj_data *obj;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    /* 'purge' */
    char_data *vnext;
    obj_data *obj_next;

    for (victim = rroom_get_people(g, char_get_in_room(ch)); victim; victim = vnext) {
      vnext = char_get_next_in_room(victim);
      if (char_is_npc(victim) && victim != ch)
        extract_char(g, victim);
    }

    for (obj = rroom_get_contents(g, char_get_in_room(ch)); obj; obj = obj_next) {
      obj_next = obj_get_next_content(obj);
      extract_obj(g, obj);
    }

    return;
  }

  if (*arg == UID_CHAR)
    victim = get_char(g, arg);
  else victim = get_char_room_vis(g, ch, arg, nullptr);

  if (victim == nullptr) {
    if (*arg == UID_CHAR)
      obj = get_obj(g, arg);
    else
      obj = get_obj_vis(g, ch, arg, nullptr);

    if (obj) {
      extract_obj(g, obj);
      obj = nullptr;
    } else
      mob_log(g, ch, "mpurge: bad argument");

    return;
  }

  if (!char_is_npc(victim)) {
    mob_log(g, ch, "mpurge: purging a PC");
    return;
  }

  if (victim == ch) g->dg_owner_purged = 1;

  extract_char(g, victim);
}

/* lets the mobile goto any location it wishes that is not private */
ACMD(do_mgoto)
{
  char arg[MAX_INPUT_LENGTH];
  room_rnum location;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(g, ch, "mgoto called with no argument");
    return;
  }

  if ((location = find_target_room(g, ch, arg)) == NOWHERE && char_get_mob_vnum(g, ch) != 3) {
    mob_log(g, ch, "mgoto: invalid location");
    return;
  }
  if ((location = find_target_room(g, ch, arg)) == NOWHERE) {
    return;
  }

  if (char_get_fighting(ch))
    stop_fighting(g, ch);

  char_from_room(g, ch);
  char_to_room(g, ch, location);
  enter_wtrigger(g, &g->rooms[char_get_in_room(ch)], ch, -1);
}

/* lets the mobile do a command at another location. Very useful */
ACMD(do_mat)
{
  char arg[MAX_INPUT_LENGTH];
  room_rnum location, original;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  argument = one_argument(argument, arg);

  if (!*arg || !*argument) {
    mob_log(g, ch, "mat: bad argument");
    return;
  }

  if ((location = find_target_room(g, ch, arg)) == NOWHERE) {
    mob_log(g, ch, "mat: invalid location");
    return;
  }

  original = char_get_in_room(ch);
  char_from_room(g, ch);
  char_to_room(g, ch, location);
  command_interpreter(g, ch, argument);

  /* See if 'ch' still exists before continuing! Handles 'at XXXX quit' case. */
  if (char_get_in_room(ch) == location) {
    char_from_room(g, ch);
    char_to_room(g, ch, original);
  }
}

/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
ACMD(do_mteleport)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  room_rnum target;
  char_data *vict, *next_ch;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  argument = two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    mob_log(g, ch, "mteleport: bad syntax");
    return;
  }

  target = find_target_room(g, ch, arg2);

  if (target == NOWHERE) {
    mob_log(g, ch, "mteleport target is an invalid room");
    return;
  }

  if (!strcasecmp(arg1, "all")) {
    if (target == char_get_in_room(ch)) {
      mob_log(g, ch, "mteleport all target is itself");
      return;
    }

    for (vict = rroom_get_people(g, char_get_in_room(ch)); vict; vict = next_ch) {
      next_ch = char_get_next_in_room(vict);

      if (valid_dg_target(g, vict, DG_ALLOW_GODS)) {
        char_from_room(g, vict);
        char_to_room(g, vict, target);
        enter_wtrigger(g, &g->rooms[char_get_in_room(ch)], ch, -1);
      }
    }
  } else {
    if (*arg1 == UID_CHAR) {
      if (!(vict = get_char(g, arg1))) {
        mob_log(g, ch, "mteleport: victim (%s) does not exist", arg1);
        return;
      }
    } else if (!(vict = get_char_vis(g, ch, arg1, nullptr, FIND_CHAR_WORLD))) {
      mob_log(g, ch, "mteleport: victim (%s) does not exist", arg1);
      return;
    }

    if (valid_dg_target(g, vict, DG_ALLOW_GODS)) {
      char_from_room(g, vict);
      char_to_room(g, vict, target);
      enter_wtrigger(g, &g->rooms[char_get_in_room(vict)], vict, -1);
    }
  }
}

ACMD(do_mdamage)
{
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
  int dam = 0;
  char_data *vict;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  two_arguments(argument, name, amount);

  /* who cares if it's a number ? if not it'll just be 0 */
  if (!*name || !*amount) {
    mob_log(g, ch, "mdamage: bad syntax");
    return;
  }

  dam = atoi(amount);
  if (*name == UID_CHAR) {
    if (!(vict = get_char(g, name))) {
      mob_log(g, ch, "mdamage: victim (%s) does not exist", name);
      return;
    }
  } else if (!(vict = get_char_room_vis(g, ch, name, nullptr))) {
    mob_log(g, ch, "mdamage: victim (%s) does not exist", name);
    return;
  }
  script_damage(g, vict, dam);
}

/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
ACMD(do_mforce)
{
  char arg[MAX_INPUT_LENGTH];

  if (!mob_cmd_allowed(g, ch, true))
    return;

  argument = one_argument(argument, arg);

  if (!*arg || !*argument) {
    mob_log(g, ch, "mforce: bad syntax");
    return;
  }

  if (!strcasecmp(arg, "all")) {
    struct descriptor_data *i;
    char_data *vch;

    for (i = g->descriptors; i; i = i->next) {
      if (!i->character) continue;

      if ((i->character != ch) && !i->connected &&
          (char_get_in_room(i->character) == char_get_in_room(ch))) {
        vch = i->character;
        if (char_get_level(vch) < char_get_level(ch) && char_can_see(g, ch, vch) &&
            valid_dg_target(g, vch, 0)) {
          command_interpreter(g, vch, argument);
        }
      }
    }
  } else {
    char_data *victim;

    if (*arg == UID_CHAR) {
      if (!(victim = get_char(g, arg))) {
        mob_log(g, ch, "mforce: victim (%s) does not exist", arg);
        return;
      }
    } else if ((victim = get_char_room_vis(g, ch, arg, nullptr)) == nullptr) {
      mob_log(g, ch, "mforce: no such victim");
      return;
    }

    if (victim == ch) {
      mob_log(g, ch, "mforce: forcing self");
      return;
    }

    if (valid_dg_target(g, victim, 0))
      command_interpreter(g, victim, argument);
  }
}

/* place someone into the mob's memory list */
ACMD(do_mremember)
{
  char_data *victim;
  struct script_memory *mem;
  char arg[MAX_INPUT_LENGTH];

  if (!mob_cmd_allowed(g, ch, true))
    return;

  argument = one_argument(argument, arg);

  if (!*arg) {
    mob_log(g, ch, "mremember: bad syntax");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(g, arg))) {
      mob_log(g, ch, "mremember: victim (%s) does not exist", arg);
      return;
    }
  } else if (!(victim = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    mob_log(g, ch, "mremember: victim (%s) does not exist", arg);
    return;
  }

  /* create a structure and add it to the list */
  CREATE(mem, struct script_memory, 1);
  if (!char_get_memory(ch)) char_set_memory(ch, mem);
  else {
    struct script_memory *tmpmem = char_get_memory(ch);
    while (tmpmem->next) tmpmem = tmpmem->next;
    tmpmem->next = mem;
  }

  /* fill in the structure */
  mem->id = char_get_id(victim);
  if (argument && *argument) {
    mem->cmd = strdup(argument);
  }
}

/* remove someone from the list */
ACMD(do_mforget)
{
  char_data *victim;
  struct script_memory *mem, *prev;
  char arg[MAX_INPUT_LENGTH];

  if (!mob_cmd_allowed(g, ch, true))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(g, ch, "mforget: bad syntax");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(g, arg))) {
      mob_log(g, ch, "mforget: victim (%s) does not exist", arg);
      return;
    }
  } else if (!(victim = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    mob_log(g, ch, "mforget: victim (%s) does not exist", arg);
    return;
  }

  mem  = char_get_memory(ch);
  prev = nullptr;
  while (mem) {
    if (mem->id == char_get_id(victim)) {
      if (mem->cmd) free(mem->cmd);
      if (prev == nullptr) {
        char_set_memory(ch, mem->next);
        free(mem);
        mem = char_get_memory(ch);
      } else {
        prev->next = mem->next;
        free(mem);
        mem = prev->next;
      }
    } else {
      prev = mem;
      mem  = mem->next;
    }
  }
}

/* transform into a different mobile */
ACMD(do_mtransform)
{
  char arg[MAX_INPUT_LENGTH];
  char_data *m, tmpmob;
  obj_data *obj[NUM_WEARS];
  mob_rnum this_rnum = char_get_nr(ch);
  int pos;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  if (char_get_desc(ch)) {
    send_to_char(g, ch, "You've got no VNUM to return to, dummy! try 'switch'\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg)
    mob_log(g, ch, "mtransform: missing argument");
  else if (!isdigit(*arg) && *arg != '-')
    mob_log(g, ch, "mtransform: bad argument");
  else {
    if (isdigit(*arg))
      m = read_mobile(g, atoi(arg), VIRTUAL);
    else {
      m = read_mobile(g, atoi(arg + 1), VIRTUAL);
    }
    if (m == nullptr) {
      mob_log(g, ch, "mtransform: bad mobile vnum");
      return;
    }

    /* move new obj info over to old object and delete new obj */

    for (pos = 0; pos < NUM_WEARS; pos++) {
      if (char_get_equipment(ch, pos))
        obj[pos] = unequip_char(g, ch, pos);
      else
        obj[pos] = nullptr;
    }

    /* put the mob in the same room as ch so extract will work */
    char_to_room(g, m, char_get_in_room(ch));

    memcpy(&tmpmob, m, sizeof(*m));

    /* Thanks to Russell Ryan for this fix. RRfon we need to copy the
       the strings so we don't end up free'ing the prototypes later */
    if (char_get_alias(m))
      char_set_alias(&tmpmob, strdup(char_get_alias(m)));
    if (char_get_raw_title(m))
      char_set_raw_title(&tmpmob, strdup(char_get_raw_title(m)));
    if (char_get_sdesc(m))
      char_set_sdesc(&tmpmob, strdup(char_get_sdesc(m)));
    if (char_get_ldesc(m))
      char_set_ldesc(&tmpmob, strdup(char_get_ldesc(m)));
    if (char_get_ddesc(m))
      char_set_ddesc(&tmpmob, strdup(char_get_ddesc(m)));

    char_set_id(&tmpmob, char_get_id(ch));
    char_set_affected(&tmpmob, char_get_affected(ch));
    char_set_inventory(&tmpmob, char_get_inventory(ch));
    char_set_proto_script(&tmpmob, char_get_proto_script(ch));
    char_set_script(&tmpmob, char_get_script(ch));
    char_set_memory(&tmpmob, char_get_memory(ch));
    char_set_next_in_room(&tmpmob, char_get_next_in_room(ch));
    char_set_next(&tmpmob, char_get_next(ch));
    char_set_next_fighting(&tmpmob, char_get_next_fighting(ch));
    char_set_followers(&tmpmob, char_get_followers(ch));
    char_set_master(&tmpmob, char_get_master(ch));

    char_set_was_in_room(&tmpmob, char_get_was_in_room(ch));
    char_set_gold(&tmpmob, char_get_gold(ch));
    char_set_position(&tmpmob, char_get_position(ch));
    char_set_carry_weight(&tmpmob, char_get_carry_weight(ch));
    char_set_carry_items(&tmpmob, char_get_carry_items(ch));
    char_set_fighting(&tmpmob, char_get_fighting(ch));
    memcpy(ch, &tmpmob, sizeof(*ch));

    for (pos = 0; pos < NUM_WEARS; pos++) {
      if (obj[pos])
        equip_char(g, ch, obj[pos], pos);
    }

    char_set_nr(ch, this_rnum);
    extract_char(g, m);
  }
}

ACMD(do_mdoor)
{
  char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
  char field[MAX_INPUT_LENGTH], *value;
  room_data *rm;
  int dir, fd;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  argument = two_arguments(argument, target, direction);
  value    = one_argument(argument, field);
  skip_spaces(&value);

  if (!*target || !*direction || !*field) {
    mob_log(g, ch, "mdoor called with too few args");
    return;
  }

  if ((rm = get_room(g, target)) == nullptr) {
    mob_log(g, ch, "mdoor: invalid target");
    return;
  }

  if ((dir = search_block(direction, dirs, false)) == -1) {
    mob_log(g, ch, "mdoor: invalid direction");
    return;
  }

  if ((fd = search_block(field, dg_door_fields, false)) == -1) {
    mob_log(g, ch, "odoor: invalid field");
    return;
  }

  if (!dg_door_apply(g, rm, dir, fd, value))
    mob_log(g, ch, "mdoor: invalid door target");
}

ACMD(do_mfollow)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *leader;
  struct follow_type *j, *k;

  if (!mob_cmd_allowed(g, ch, true))
    return;

  one_argument(argument, buf);

  if (!*buf) {
    mob_log(g, ch, "mfollow: bad syntax");
    return;
  }

  if (*buf == UID_CHAR) {
    if (!(leader = get_char(g, buf))) {
      mob_log(g, ch, "mfollow: victim (%s) does not exist", buf);
      return;
    }
  } else if (!(leader = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_ROOM))) {
    mob_log(g, ch, "mfollow: victim (%s) not found", buf);
    return;
  }

  if (char_get_master(ch) == leader) /* already following */
    return;

  if (char_aff_flagged(ch, AFF_CHARM) && (char_get_master(ch))) /* can't override charm */
    return;

  /* stop following someone else first */
  if (char_get_master(ch)) {
    if (char_get_followers(char_get_master(ch))->follower == ch) { /* Head of follower-list? */
      k                     = char_get_followers(char_get_master(ch));
      char_set_followers(char_get_master(ch), k->next);
      free(k);
    } else { /* locate follower who is not head of list */
      for (k = char_get_followers(char_get_master(ch)); k->next->follower != ch; k = k->next);

      j       = k->next;
      k->next = j->next;
      free(j);
    }
    char_set_master(ch, nullptr);
  }

  if (ch == leader)
    return;

  if (circle_follow(ch, leader)) {
    mob_log(g, ch, "mfollow: Following in circles.");
    return;
  }

  char_set_master(ch, leader);

  CREATE(k, struct follow_type, 1);

  k->follower       = ch;
  k->next           = char_get_followers(leader);
  char_set_followers(leader, k);
}

/* prints the message to everyone in the range of numbers */
/* Thx to Jamie Nelson of 4D for this contribution */
ACMD(do_mrecho)
{
  char start[MAX_INPUT_LENGTH], finish[MAX_INPUT_LENGTH], *msg;

  if (!mob_cmd_allowed(g, ch, false))
    return;
  msg = two_arguments(argument, start, finish);

  skip_spaces(&msg);

  if (!*msg || !*start || !*finish || !is_number(start) || !is_number(finish))
    mob_log(g, ch, "mrecho called with too few args");
  else
    send_to_range(g, atoi(start), atoi(finish), "%s\r\n", msg);
}
