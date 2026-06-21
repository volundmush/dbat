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
#include "consts/maximums.h"

#include "act.wizard.h"
#include "character_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "comm.h"
#include "db.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "dg_scripts.h"
#include "dgscript_impl.h"
#include "fight.h"
#include "flags.h"
#include "handler.h"
#include "interpreter.h"
#include "log.h"
#include "object_impl.h"
#include "object_macros.h"
#include "object_utils.h"
#include "room_api.h"
#include "room_db.h"
#include "room_impl.h"
#include "zone_db.h"

#include "extract.h"
#include "random.h"
#include "relocate.h"
#include "search.h"

#include "consts/admlevel.h"
#include "consts/directions.h"
#include "consts/mobflags.h"
#include "consts/races.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>

#include "iterate.hpp"
/*
 * Local functions.
 */
void mob_log(char_data *mob, const char *format, ...);

ACMD(do_masound);
ACMD(do_mkill);
ACMD(do_mheal);
ACMD(do_mjunk);
ACMD(do_mechoaround);
ACMD(do_msend);
ACMD(do_mecho);
ACMD(do_mzoneecho);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mteleport);
ACMD(do_mdamage);
ACMD(do_mforce);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mdoor);
ACMD(do_mfollow);
ACMD(do_mrecho);

/* attaches mob's name and vnum to msg and sends it to script_log */
void mob_log(char_data *mob, const char *format, ...) {
  va_list args;
  char output[MAX_STRING_LENGTH];

  snprintf(output, sizeof(output), "Mob (%s, VNum %d):: %s", GET_SHORT(mob),
           GET_MOB_VNUM(mob), format);

  va_start(args, format);
  script_vlog(output, args);
  va_end(args);
}

/*
** macro to determine if a mob is permitted to use these commands
*/
#define MOB_OR_IMPL(ch)                                                        \
  (IS_NPC(ch) &&                                                               \
   (!(ch)->desc || GET_ADMLEVEL((ch)->desc->original) >= ADMLVL_IMPL))

/* mob commands */

/* prints the argument to all the rooms aroud the mobile */
ACMD(do_masound) {
  struct room_data *was_in_room;
  int door;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (!*argument) {
    mob_log(ch, "masound called with no argument");
    return;
  }

  skip_spaces(&argument);

  was_in_room = char_room_get(ch);
  room_exits_iterate(was_in_room, [&](auto door, auto ex) {
    auto dest = exit_dest_get(ex);
    if (!dest)
      return true;

    IN_ROOM(ch) = room_vnum_get(dest);
    sub_write(argument, ch, TRUE, TO_ROOM);
    return true;
  });

  IN_ROOM(ch) = room_vnum_get(was_in_room);
}

/* Heals a stat of the mob */
ACMD(do_mheal) {
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    mob_log(ch, "mheal called without an argument");
    return;
  }

  int64_t amount = 0;
  double num = atoi(arg2);
  double perc = num * 0.01;

  amount = GET_MAX_HIT(ch) * perc;

  if (!strcasecmp(arg, "pl")) {
    incCurHealthPercent(ch, num);
  } else if (!strcasecmp(arg, "ki")) {
    incCurKIPercent(ch, num);
  } else if (!strcasecmp(arg, "st")) {
    incCurSTPercent(ch, num);
  } else {
    mob_log(ch, "mheal called with wrong argument [pl | ki | st]");
    return;
  }
}

/* lets the mobile kill any player or mobile without murder*/
ACMD(do_mkill) {
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  char buf[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(ch, "mkill called with no argument");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(arg))) {
      mob_log(ch, "mkill: victim (%s) not found", arg);
      return;
    }
  } else if (!(victim = get_char_room_vis(ch, arg, NULL))) {
    mob_log(ch, "mkill: victim (%s) not found", arg);
    return;
  }

  if (victim == ch) {
    mob_log(ch, "mkill: victim is self");
    return;
  }

  if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_NOHASSLE)) {
    mob_log(ch, "mkill: target has nohassle on");
    return;
  }

  sprintf(buf, "%s", GET_NAME(victim));
  if (IS_HUMANOID(ch)) {
    switch (rand_number(1, 7)) {
    case 1:
      char_cmd_execute(ch, "punch", buf);
      break;
    case 2:
      char_cmd_execute(ch, "kick", buf);
      break;
    case 3:
      char_cmd_execute(ch, "elbow", buf);
      break;
    case 4:
      char_cmd_execute(ch, "knee", buf);
      break;
    case 5:
      char_cmd_execute(ch, "kick", buf);
      break;
    default:
      char_cmd_execute(ch, "punch", buf);
      break;
    } // end switch
  } else {
    char_cmd_execute(ch, "bite", buf);
  } // end humanoid if
  return;
}

/*
 * lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 */
ACMD(do_mjunk) {
  char arg[MAX_INPUT_LENGTH];
  int pos, junk_all = 0;
  obj_data *obj;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(ch, "mjunk called with no argument");
    return;
  }

  if (!strcasecmp(arg, "all"))
    junk_all = 1;

  if ((find_all_dots(arg) != FIND_INDIV) && !junk_all) {
    /* Thanks to Carlos Myers for fixing the line below */
    if ((pos = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) >= 0) {
      extract_obj(unequip_char(ch, pos));
      return;
    }
    if ((obj = get_obj_in_list_vis(ch, arg, NULL, inv_for_char(ch))) != NULL)
      extract_obj(obj);
    return;
  } else {
    char_inventory_iterate(ch, [&](auto obj) {
      if (arg[3] == '\0' || isname(arg + 4, obj->name)) {
        extract_obj(obj);
      }
      return true;
    });
    /* Thanks to Carlos Myers for fixing the line below */
    while ((pos = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) >= 0)
      extract_obj(unequip_char(ch, pos));
  }
  return;
}

/* prints the message to everyone in the room other than the mob and victim */
ACMD(do_mechoaround) {
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  char *p;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  p = one_argument(argument, arg);
  skip_spaces(&p);

  if (!*arg) {
    mob_log(ch, "mechoaround called with no argument");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(arg))) {
      mob_log(ch, "mechoaround: victim (%s) does not exist", arg);
      return;
    }
  } else if (!(victim = get_char_room_vis(ch, arg, NULL))) {
    mob_log(ch, "mechoaround: victim (%s) does not exist", arg);
    return;
  }

  char buf[MAX_STRING_LENGTH];

  sprintf(buf, p);
  search_replace(buf, GET_NAME(victim), "$n");
  act(buf, TRUE, victim, 0, 0, TO_ROOM);
  /*sub_write(p, victim, TRUE, TO_ROOM);*/
}

/* sends the message to only the victim */
ACMD(do_msend) {
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  char *p;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  p = one_argument(argument, arg);
  skip_spaces(&p);

  if (!*arg) {
    mob_log(ch, "msend called with no argument");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(arg))) {
      mob_log(ch, "msend: victim (%s) does not exist", arg);
      return;
    }
  } else if (!(victim = get_char_room_vis(ch, arg, NULL))) {
    mob_log(ch, "msend: victim (%s) does not exist", arg);
    return;
  }

  sub_write(p, victim, TRUE, TO_CHAR);
}

/* prints the message to the room at large */
ACMD(do_mecho) {
  char *p;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (!*argument) {
    mob_log(ch, "mecho called with no arguments");
    return;
  }
  p = argument;
  skip_spaces(&p);

  sub_write(p, ch, TRUE, TO_ROOM);
}

ACMD(do_mzoneecho) {
  struct zone_data *zone;
  char room_number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

  msg = any_one_arg(argument, room_number);
  skip_spaces(&msg);

  if (!*room_number || !*msg)
    mob_log(ch, "mzoneecho called with too few args");

  else if (!(zone = zone_by_id(virtual_zone_by_thing(atoi(room_number)))))
    mob_log(ch, "mzoneecho called for nonexistant zone");

  else {
    sprintf(buf, "%s\r\n", msg);
    send_to_zone(buf, zone);
  }
}

/*
 * lets the mobile load an item or mobile.  All items
 * are loaded into inventory, unless it is NO-TAKE.
 */
ACMD(do_mload) {
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int number = 0;
  char_data *mob;
  obj_data *object;
  char *target;
  char_data *tch;
  obj_data *cnt;
  int pos;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL)
    return;

  target = two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0)) {
    mob_log(ch, "mload: bad syntax");
    return;
  }

  /* load mob to target room - Jamie Nelson, April 13 2004 */
  if (is_abbrev(arg1, "mob")) {
    struct room_data *room;
    if (!target || !*target) {
      room = char_room_get(ch);
    } else {
      if (!isdigit(*target) || (room = room_by_id(atoi(target))) == NULL) {
        mob_log(ch,
                "mload: room target vnum doesn't exist "
                "(loading mob vnum %d to room %s)",
                number, target);
        return;
      }
    }
    if ((mob = read_mobile(number, VIRTUAL)) == NULL) {
      mob_log(ch, "mload: bad mob vnum");
      return;
    }
    char_to_room(mob, room);
    if (SCRIPT(ch)) { /* It _should_ have, but it might be detached. */
      char buf[MAX_INPUT_LENGTH];
      sprintf(buf, "%cC%d", UID_CHAR, GET_ID(mob));
      add_var(&(SCRIPT(ch)->global_vars), "lastloaded", buf, 0);
    }
    load_mtrigger(mob);
  }

  else if (is_abbrev(arg1, "obj")) {
    if ((object = read_object(number, VIRTUAL)) == NULL) {
      mob_log(ch, "mload: bad object vnum");
      return;
    }
    if (SCRIPT(ch)) { /* It _should_ have, but it might be detached. */
      char buf[MAX_INPUT_LENGTH];
      sprintf(buf, "%cO%d", UID_CHAR, GET_ID(object));
      add_var(&(SCRIPT(ch)->global_vars), "lastloaded", buf, 0);
    }
    randomize_eq(object);
    /* special handling to make objects able to load on a person/in a
     * container/worn etc. */
    if (!target || !*target) {
      if (CAN_WEAR(object, ITEM_WEAR_TAKE)) {
        obj_to_char(object, ch);
      } else {
        obj_to_room(object, char_room_get(ch));
      }
      load_otrigger(object);
      return;
    }
    two_arguments(target, arg1, arg2); /* recycling ... */
    tch = (arg1 != NULL && *arg1 == UID_CHAR)
              ? get_char(arg1)
              : get_char_room_vis(ch, arg1, NULL);
    if (tch) {
      if (arg2 != NULL && *arg2 && (pos = find_eq_pos_script(arg2)) >= 0 &&
          !GET_EQ(tch, pos) && can_wear_on_pos(object, pos)) {
        equip_char(tch, object, pos);
        load_otrigger(object);
        return;
      }
      obj_to_char(object, tch);
      load_otrigger(object);
      return;
    }
    cnt = (arg1 != NULL && *arg1 == UID_CHAR) ? get_obj(arg1)
                                              : get_obj_vis(ch, arg1, NULL);
    if (cnt && GET_OBJ_TYPE(cnt) == ITEM_CONTAINER) {
      obj_to_obj(object, cnt);
      load_otrigger(object);
      return;
    }
    /* neither char nor container found - just dump it in room */
    obj_to_room(object, char_room_get(ch));
    load_otrigger(object);
    return;
  }

  else
    mob_log(ch, "mload: bad type");
}

/*
 * lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room.  It can purge
 *  itself, but this will be the last command it does.
 */
ACMD(do_mpurge) {
  char arg[MAX_INPUT_LENGTH];
  char_data *victim;
  obj_data *obj;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    /* 'purge' */
    char_data *vnext;

    room_people_iterate(char_room_get(ch), [&](auto victim) {
      if (IS_NPC(victim) && victim != ch)
        extract_char(victim);
      return true;
    });

    room_contents_iterate(char_room_get(ch), [&](auto obj) {
      extract_obj(obj);
      return true;
    });

    return;
  }

  if (*arg == UID_CHAR)
    victim = get_char(arg);
  else
    victim = get_char_room_vis(ch, arg, NULL);

  if (victim == NULL) {
    if (*arg == UID_CHAR)
      obj = get_obj(arg);
    else
      obj = get_obj_vis(ch, arg, NULL);

    if (obj) {
      extract_obj(obj);
      obj = NULL;
    } else
      mob_log(ch, "mpurge: bad argument");

    return;
  }

  if (!IS_NPC(victim)) {
    mob_log(ch, "mpurge: purging a PC");
    return;
  }

  if (victim == ch)
    dg_owner_purged = 1;

  extract_char(victim);
}

/* lets the mobile goto any location it wishes that is not private */
ACMD(do_mgoto) {
  char arg[MAX_INPUT_LENGTH];
  struct room_data *location;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(ch, "mgoto called with no argument");
    return;
  }

  if ((location = find_target_room(ch, arg)) == NULL && GET_MOB_VNUM(ch) != 3) {
    mob_log(ch, "mgoto: invalid location");
    return;
  } else if ((location = find_target_room(ch, arg)) == NULL) {
    return;
  }

  if (FIGHTING(ch))
    stop_fighting(ch);

  char_from_room(ch);
  char_to_room(ch, location);
  enter_wtrigger(char_room_get(ch), ch, -1);
}

/* lets the mobile do a command at another location. Very useful */
ACMD(do_mat) {
  char arg[MAX_INPUT_LENGTH];
  struct room_data *location, *original;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  argument = one_argument(argument, arg);

  if (!*arg || !*argument) {
    mob_log(ch, "mat: bad argument");
    return;
  }

  if ((location = find_target_room(ch, arg)) == NULL) {
    mob_log(ch, "mat: invalid location");
    return;
  }

  original = char_room_get(ch);
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, argument);

  /* See if 'ch' still exists before continuing! Handles 'at XXXX quit' case. */
  if (char_room_get(ch) == location) {
    char_from_room(ch);
    char_to_room(ch, original);
  }
}

/*
 * lets the mobile transfer people.  the all argument transfers
 * everyone in the current room to the specified location
 */
ACMD(do_mteleport) {
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct room_data *target;
  char_data *vict, *next_ch;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  argument = two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    mob_log(ch, "mteleport: bad syntax");
    return;
  }

  target = find_target_room(ch, arg2);

  if (target == NULL) {
    mob_log(ch, "mteleport target is an invalid room");
    return;
  }

  if (!strcasecmp(arg1, "all")) {
    if (target == char_room_get(ch)) {
      mob_log(ch, "mteleport all target is itself");
      return;
    }

    room_people_iterate(char_room_get(ch), [&](auto vict) {
      if (valid_dg_target(vict, DG_ALLOW_GODS)) {
        char_from_room(vict);
        char_to_room(vict, target);
        enter_wtrigger(char_room_get(ch), ch, -1);
      }
      return true;
    });
  } else {
    if (*arg1 == UID_CHAR) {
      if (!(vict = get_char(arg1))) {
        mob_log(ch, "mteleport: victim (%s) does not exist", arg1);
        return;
      }
    } else if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD))) {
      mob_log(ch, "mteleport: victim (%s) does not exist", arg1);
      return;
    }

    if (valid_dg_target(ch, DG_ALLOW_GODS)) {
      char_from_room(vict);
      char_to_room(vict, target);
      enter_wtrigger(char_room_get(ch), ch, -1);
    }
  }
}

ACMD(do_mdamage) {
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
  int dam = 0;
  char_data *vict;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  two_arguments(argument, name, amount);

  /* who cares if it's a number ? if not it'll just be 0 */
  if (!*name || !*amount) {
    mob_log(ch, "mdamage: bad syntax");
    return;
  }

  dam = atoi(amount);
  if (*name == UID_CHAR) {
    if (!(vict = get_char(name))) {
      mob_log(ch, "mdamage: victim (%s) does not exist", name);
      return;
    }
  } else if (!(vict = get_char_room_vis(ch, name, NULL))) {
    mob_log(ch, "mdamage: victim (%s) does not exist", name);
    return;
  }
  script_damage(vict, dam);
}

/*
 * lets the mobile force someone to do something.  must be mortal level
 * and the all argument only affects those in the room with the mobile
 */
ACMD(do_mforce) {
  char arg[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
    return;

  argument = one_argument(argument, arg);

  if (!*arg || !*argument) {
    mob_log(ch, "mforce: bad syntax");
    return;
  }

  if (!strcasecmp(arg, "all")) {
    struct descriptor_data *i;
    char_data *vch;

    for (i = descriptor_list; i; i = i->next) {
      if (!i->character)
        continue;

      if ((i->character != ch) && !i->connected &&
          (char_room_get(i->character) == char_room_get(ch))) {
        vch = i->character;
        if (GET_LEVEL(vch) < GET_LEVEL(ch) && CAN_SEE(ch, vch) &&
            valid_dg_target(vch, 0)) {
          command_interpreter(vch, argument);
        }
      }
    }
  } else {
    char_data *victim;

    if (*arg == UID_CHAR) {
      if (!(victim = get_char(arg))) {
        mob_log(ch, "mforce: victim (%s) does not exist", arg);
        return;
      }
    } else if ((victim = get_char_room_vis(ch, arg, NULL)) == NULL) {
      mob_log(ch, "mforce: no such victim");
      return;
    }

    if (victim == ch) {
      mob_log(ch, "mforce: forcing self");
      return;
    }

    if (valid_dg_target(victim, 0))
      command_interpreter(victim, argument);
  }
}

/* place someone into the mob's memory list */
ACMD(do_mremember) {
  char_data *victim;
  struct script_memory *mem;
  char arg[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
    return;

  argument = one_argument(argument, arg);

  if (!*arg) {
    mob_log(ch, "mremember: bad syntax");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(arg))) {
      mob_log(ch, "mremember: victim (%s) does not exist", arg);
      return;
    }
  } else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    mob_log(ch, "mremember: victim (%s) does not exist", arg);
    return;
  }

  /* create a structure and add it to the list */
  CREATE(mem, struct script_memory, 1);
  if (!SCRIPT_MEM(ch))
    SCRIPT_MEM(ch) = mem;
  else {
    struct script_memory *tmpmem = SCRIPT_MEM(ch);
    while (tmpmem->next)
      tmpmem = tmpmem->next;
    tmpmem->next = mem;
  }

  /* fill in the structure */
  mem->id = GET_ID(victim);
  if (argument && *argument) {
    mem->cmd = strdup(argument);
  }
}

/* remove someone from the list */
ACMD(do_mforget) {
  char_data *victim;
  struct script_memory *mem, *prev;
  char arg[MAX_INPUT_LENGTH];

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc && (GET_ADMLEVEL(ch->desc->original) < ADMLVL_IMPL))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    mob_log(ch, "mforget: bad syntax");
    return;
  }

  if (*arg == UID_CHAR) {
    if (!(victim = get_char(arg))) {
      mob_log(ch, "mforget: victim (%s) does not exist", arg);
      return;
    }
  } else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    mob_log(ch, "mforget: victim (%s) does not exist", arg);
    return;
  }

  mem = SCRIPT_MEM(ch);
  prev = NULL;
  while (mem) {
    if (mem->id == GET_ID(victim)) {
      if (mem->cmd)
        free(mem->cmd);
      if (prev == NULL) {
        SCRIPT_MEM(ch) = mem->next;
        free(mem);
        mem = SCRIPT_MEM(ch);
      } else {
        prev->next = mem->next;
        free(mem);
        mem = prev->next;
      }
    } else {
      prev = mem;
      mem = mem->next;
    }
  }
}

/* transform into a different mobile */
ACMD(do_mtransform) {
  char arg[MAX_INPUT_LENGTH];
  char_data *m, tmpmob;
  obj_data *obj[NUM_WEARS] = {};
  int pos;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (ch->desc) {
    send_to_char(ch,
                 "You've got no VNUM to return to, dummy! try 'switch'\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg)
    mob_log(ch, "mtransform: missing argument");
  else if (!isdigit(*arg) && *arg != '-')
    mob_log(ch, "mtransform: bad argument");
  else {
    if (isdigit(*arg))
      m = read_mobile(atoi(arg), VIRTUAL);
    else {
      m = read_mobile(atoi(arg + 1), VIRTUAL);
    }
    if (m == NULL) {
      mob_log(ch, "mtransform: bad mobile vnum");
      return;
    }

    /* move new obj info over to old object and delete new obj */

    char_equipment_iterate(ch, [&](auto pos, auto eq) {
      obj[pos] = unequip_char(ch, pos);
      return true;
    });

    /* put the mob in the same room as ch so extract will work */
    char_to_room(m, char_room_get(ch));

    memcpy(&tmpmob, m, sizeof(*m));

    /* Thanks to Russell Ryan for this fix. RRfon we need to copy the
       the strings so we don't end up free'ing the prototypes later */
    if (m->name)
      tmpmob.name = strdup(m->name);
    if (m->title)
      tmpmob.title = strdup(m->title);
    if (m->short_descr)
      tmpmob.short_descr = strdup(m->short_descr);
    if (m->long_descr)
      tmpmob.long_descr = strdup(m->long_descr);
    if (m->description)
      tmpmob.description = strdup(m->description);

    tmpmob.id = ch->id;
    tmpmob.proto_script = ch->proto_script;
    tmpmob.script = ch->script;
    tmpmob.memory = ch->memory;
    GET_WAS_IN(&tmpmob) = GET_WAS_IN(ch);
    char_stat_set(&tmpmob, "money", GET_GOLD(ch));
    char_position_set(&tmpmob, GET_POS(ch));
    char_fighting_set(&tmpmob, FIGHTING(ch));
    memcpy(ch, &tmpmob, sizeof(*ch));

    for (pos = 0; pos < NUM_WEARS; pos++) {
      if (obj[pos])
        equip_char(ch, obj[pos], pos);
    }

    extract_char(m);
  }
}

ACMD(do_mdoor) {
  char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
  char field[MAX_INPUT_LENGTH], *value;
  room_data *rm;
  struct room_direction_data *newexit;
  int dir, fd, to_room;

  const char *door_field[] = {"purge", "description", "flags", "key",
                              "name",  "room",        "\n"};

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  argument = two_arguments(argument, target, direction);
  value = one_argument(argument, field);
  skip_spaces(&value);

  if (!*target || !*direction || !*field) {
    mob_log(ch, "mdoor called with too few args");
    return;
  }

  if ((rm = get_room(target)) == NULL) {
    mob_log(ch, "mdoor: invalid target");
    return;
  }

  if ((dir = search_block(direction, dirs, FALSE)) == -1) {
    mob_log(ch, "mdoor: invalid direction");
    return;
  }

  if ((fd = search_block(field, door_field, FALSE)) == -1) {
    mob_log(ch, "odoor: invalid field");
    return;
  }

  newexit = room_dir_option_get(rm, dir);

  /* purge exit */
  if (fd == 0) {
    if (newexit) {
      if (exit_general_description_get(newexit))
        free((char*)exit_general_description_get(newexit));
      if (exit_keyword_get(newexit))
        free((char*)exit_keyword_get(newexit));
      free(newexit);
      rm->dir_option[dir] = NULL;
    }
  }

  else {
    if (!newexit) {
      CREATE(newexit, struct room_direction_data, 1);
      rm->dir_option[dir] = newexit;
    }

    switch (fd) {
    case 1: /* description */
      {
        char desc_buf[MAX_INPUT_LENGTH + 3];
        snprintf(desc_buf, sizeof(desc_buf), "%s\r\n", value);
        exit_general_description_set(newexit, desc_buf);
      }
      break;
    case 2: /* flags       */
      exit_info_set(newexit, (int16_t)asciiflag_conv(value));
      break;
    case 3: /* key         */
      exit_key_set(newexit, atoi(value));
      break;
    case 4: /* name        */
      exit_keyword_set(newexit, value);
      break;
    case 5: /* room        */
      if ((to_room = room_vnum_check(atoi(value))) != NOWHERE)
        exit_to_room_vnum_set(newexit, to_room);
      else
        mob_log(ch, "mdoor: invalid door target");
      break;
    }
  }
}

ACMD(do_mfollow) {
  char buf[MAX_INPUT_LENGTH];
  struct char_data *leader;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  one_argument(argument, buf);

  if (!*buf) {
    mob_log(ch, "mfollow: bad syntax");
    return;
  }

  if (*buf == UID_CHAR) {
    if (!(leader = get_char(buf))) {
      mob_log(ch, "mfollow: victim (%s) does not exist", buf);
      return;
    }
  } else if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    mob_log(ch, "mfollow: victim (%s) not found", buf);
    return;
  }

  if (MASTER(ch) == leader) /* already following */
    return;

  if (AFF_FLAGGED(ch, AFF_CHARM) && (MASTER(ch))) /* can't override charm */
    return;

  /* stop following someone else first */
  if (MASTER(ch)) {
    char_follower_remove(MASTER(ch), ch);
    char_following_set(ch, NULL);
  }

  if (ch == leader)
    return;

  if (circle_follow(ch, leader)) {
    mob_log(ch, "mfollow: Following in circles.");
    return;
  }

  char_following_set(ch, leader);
  char_follower_add(leader, ch);
}

/* prints the message to everyone in the range of numbers */
/* Thx to Jamie Nelson of 4D for this contribution */
ACMD(do_mrecho) {
  char start[MAX_INPUT_LENGTH], finish[MAX_INPUT_LENGTH], *msg;

  if (!MOB_OR_IMPL(ch)) {
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }
  msg = two_arguments(argument, start, finish);

  skip_spaces(&msg);

  if (!*msg || !*start || !*finish || !is_number(start) || !is_number(finish))
    mob_log(ch, "mrecho called with too few args");
  else
    send_to_range(atoi(start), atoi(finish), "%s\r\n", msg);
}
