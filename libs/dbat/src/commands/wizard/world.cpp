/* ************************************************************************
*   File: wizard/world.c                                  Part of CircleMUD *
*  Usage: God commands that move and change things                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/act.wizard.h"
#include "dbat/consts/maximums.h"
#include "dbat/players.h"
#include "dbat/utils.h"

#include <unistd.h>
#include <limits.h>

#include "dbat/interpreter.h"
#include "dbat/time.h"

#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/object_utils.h"
#include "dbat/zone_utils.h"
#include "dbat/room_utils.h"
#include "dbat/stringutils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/fileop.h"
#include "dbat/extract.h"
#include "dbat/affect.h"

#include "dbat/config.h"
#include "dbat/act.other.h"
#include "dbat/maputils.h"
#include "dbat/broadcast.h"
#include "dbat/handler.h"
#include "dbat/act.item.h"
#include "dbat/act.informative.h"
#include "dbat/weather.h"
#include "dbat/assemblies.h"
#include "dbat/house.h"
#include "dbat/comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/races.h"
#include "dbat/class.h"
#include "dbat/spells.h"
#include "dbat/improved-edit.h"
#include "dbat/objsave.h"
#include "dbat/feats.h"
#include "dbat/fight.h"
#include "dbat/genolc.h"
#include "dbat/screen.h"
#include "dbat/local_limits.h"
#include "dbat/shop.h"
#include "dbat/guild.h"
#include "dbat/spell_parser.h"
#include "dbat/atomicfile.h"
#include "dbat/game_world.h"

/* local functions */
static void stop_snooping(GameWorld *g, struct char_data *ch);

ACMD(do_transobj)
{

  struct obj_data *obj;
  struct char_data *vict;
  struct descriptor_data *d;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!char_is_npc(ch) && char_get_admlevel(ch) < 1) {
    send_to_char(g, ch, "Huh!?");
    return;
  }

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "Syntax: transo (object) (target)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You want to send what?\r\n");
    return;
  }
  if (!strcasecmp("all", arg2)) {
    obj_vnum num          = obj_get_vnum(g, obj);
    struct obj_data *obj2 = nullptr;

    act(g, "You send $p to everyone in the game.", true, ch, obj, 0, TO_CHAR);

    for (d = g->descriptors; d; d = d->next) {
      if (!desc_is_playing(d))
        continue;
      if (char_is_npc(d->character))
        continue;
      else if (d->character == ch)
        continue;
      else {
        act(g, "$N sends $p across the universe to you.", true, d->character, obj, ch, TO_CHAR);
        obj2 = read_object(g, num, VIRTUAL);
        obj_to_char(g, obj2, d->character);
      }
    }
  } else if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "That player is not in the game.\r\n");
    return;
  } else {
    act(g, "You send $p to $N.", true, ch, obj, vict, TO_CHAR);
    act(g, "$n sends $p across the universe to you.", true, ch, obj, vict, TO_VICT);
    obj_from_char(g, obj);
    obj_to_char(g, obj, vict);
    return;
  }
}

ACMD(do_recall)
{
  if (char_get_admlevel(ch) < 1) {
    send_to_char(g, ch, "You are not an immortal!\r\n");
  } else {
    send_to_char(g, ch, "You disappear in a burst of light!\r\n");
    act(g, "$n disappears in a burst of light!", false, ch, 0, 0, TO_ROOM);
    if (real_room(g, 2) != NOWHERE) {
      char_from_room(g, ch);
      char_to_room(g, ch, real_room(g, 2));
      look_at_room(g, char_get_in_room(ch), ch, 0);
      char_set_load_room(g, ch, rroom_get_vnum(g, char_get_in_room(ch)));
    }
  }
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(GameWorld *g, struct char_data *ch, char *rawroomstr)
{
  room_rnum location = NOWHERE;
  char roomstr[MAX_INPUT_LENGTH];
  struct room_data *rm;

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char(g, ch, "You must supply a room number or name.\r\n");
    return (NOWHERE);
  }

  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    if ((location = real_room(g, (room_vnum)atoi(roomstr))) == NOWHERE) {
      send_to_char(g, ch, "No room exists with that number.\r\n");
      return (NOWHERE);
    }
  } else {
    struct char_data *target_mob;
    struct obj_data *target_obj;
    char *mobobjstr = roomstr;
    int num;

    num = get_number(&mobobjstr);
    if ((target_mob = get_char_vis(g, ch, mobobjstr, &num, FIND_CHAR_WORLD)) != nullptr) {
      if ((location = char_get_in_room(target_mob)) == NOWHERE) {
        send_to_char(g, ch, "That character is currently lost.\r\n");
        return (NOWHERE);
      }
    } else if ((target_obj = get_obj_vis(g, ch, mobobjstr, &num)) != nullptr) {
      if (obj_get_in_room(target_obj) != NOWHERE)
        location = obj_get_in_room(target_obj);
      else if (obj_get_carried_by(target_obj) && char_get_in_room(obj_get_carried_by(target_obj)) != NOWHERE)
        location = char_get_in_room(obj_get_carried_by(target_obj));
      else if (obj_get_worn_by(target_obj) && char_get_in_room(obj_get_worn_by(target_obj)) != NOWHERE)
        location = char_get_in_room(obj_get_worn_by(target_obj));

      if (location == NOWHERE) {
        send_to_char(g, ch, "That object is currently not in a room.\r\n");
        return (NOWHERE);
      }
    }

    if (location == NOWHERE) {
      send_to_char(g, ch, "Nothing exists by that name.\r\n");
      return (NOWHERE);
    }
  }

  /* a location has been found -- if you're >= GRGOD, no restrictions. */
  if (char_get_admlevel(ch) >= ADMLVL_VICE)
    return (location);

  rm = &g->rooms[location];

  if ((!can_edit_zone(g, ch, room_get_zone(rm)) && char_get_admlevel(ch) < ADMLVL_GOD) && zone_flagged(g, room_get_zone(rm), ZONE_QUEST)) {
    send_to_char(g, ch, "This target is in a quest zone.\r\n");
    return (NOWHERE);
  }

  if ((char_get_admlevel(ch) < ADMLVL_VICE) && zone_flagged(g, room_get_zone(rm), ZONE_NOIMMORT)) {
    send_to_char(g, ch, "This target is in a zone closed to all.\r\n");
    return (NOWHERE);
  }

  if (rroom_flagged(g, location, ROOM_GODROOM))
    send_to_char(g, ch, "You are not godly enough to use that room!\r\n");
  else
    return (location);

  return (NOWHERE);
}

ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  room_rnum location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char(g, ch, "You must supply a room number or a name.\r\n");
    return;
  }

  if (!*command) {
    send_to_char(g, ch, "What do you want to do there?\r\n");
    return;
  }

  if ((location = find_target_room(g, ch, buf)) == NOWHERE)
    return;

  /* a location has been found. */
  original_loc = char_get_in_room(ch);
  char_from_room(g, ch);
  char_to_room(g, ch, location);
  command_interpreter(g, ch, command);

  /* check if the char is still there */
  if (char_get_in_room(ch) == location) {
    char_from_room(g, ch);
    char_to_room(g, ch, original_loc);
  }
}

ACMD(do_goto)
{
  char buf[MAX_STRING_LENGTH];
  room_rnum location;

  if ((location = find_target_room(g, ch, argument)) == NOWHERE)
    return;
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "They are inside a healing tank!\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "$n %s", char_get_poofout(g, ch) ? char_get_poofout(g, ch) : "disappears in a puff of smoke.");
  act(g, buf, true, ch, 0, 0, TO_ROOM);

  char_from_room(g, ch);
  char_to_room(g, ch, location);

  snprintf(buf, sizeof(buf), "$n %s", char_get_poofin(g, ch) ? char_get_poofin(g, ch) : "appears with an ear-splitting bang.");
  act(g, buf, true, ch, 0, 0, TO_ROOM);

  look_at_room(g, char_get_in_room(ch), ch, 0);
  enter_wtrigger(g, &g->rooms[char_get_in_room(ch)], ch, -1);
}

ACMD(do_trans)
{
  char buf[MAX_INPUT_LENGTH];
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(g, ch, "Whom do you wish to transfer?\r\n");
  else if (strcasecmp("all", buf)) {
    if (!(victim = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_WORLD)))
      send_to_char(g, ch, "%s", CONFIG_NOPERSON);
    else if (victim == ch)
      send_to_char(g, ch, "That doesn't make much sense, does it?\r\n");
    else {
      if ((char_get_admlevel(ch) < char_get_admlevel(victim)) && !char_is_npc(victim)) {
        send_to_char(g, ch, "Go transfer someone your own size.\r\n");
        return;
      }
      if (char_plr_flagged(victim, PLR_HEALT)) {
        send_to_char(g, ch, "They are inside a healing tank!\r\n");
        return;
      }
      act(g, "$n disappears in a mushroom cloud.", false, victim, 0, 0, TO_ROOM);
      char_from_room(g, victim);
      char_to_room(g, victim, char_get_in_room(ch));
      act(g, "$n arrives from a puff of smoke.", false, victim, 0, 0, TO_ROOM);
      act(g, "$n has transferred you!", false, ch, 0, victim, TO_VICT);
      look_at_room(g, char_get_in_room(victim), victim, 0);
      enter_wtrigger(g, &g->rooms[char_get_in_room(victim)], victim, -1);
    }
  } else { /* Trans All */
    if (!char_adm_flagged(ch, ADM_TRANSALL)) {
      send_to_char(g, ch, "I think not.\r\n");
      return;
    }

    for (i = g->descriptors; i; i = i->next)
      if (desc_get_connected(i) == CON_PLAYING && i->character && i->character != ch) {
        victim = i->character;
        if (char_get_admlevel(victim) >= char_get_admlevel(ch))
          continue;
        act(g, "$n disappears in a mushroom cloud.", false, victim, 0, 0, TO_ROOM);
        char_from_room(g, victim);
        char_to_room(g, victim, char_get_in_room(ch));
        act(g, "$n arrives from a puff of smoke.", false, victim, 0, 0, TO_ROOM);
        act(g, "$n has transferred you!", false, ch, 0, victim, TO_VICT);
        look_at_room(g, char_get_in_room(victim), victim, 0);
        enter_wtrigger(g, &g->rooms[char_get_in_room(victim)], victim, -1);
      }
    send_to_char(g, ch, "%s", CONFIG_OK);
  }
}

ACMD(do_teleport)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char(g, ch, "Whom do you wish to teleport?\r\n");
  else if (!(victim = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_WORLD)))
    send_to_char(g, ch, "%s", CONFIG_NOPERSON);
  else if (victim == ch)
    send_to_char(g, ch, "Use 'goto' to teleport yourself.\r\n");
  else if (char_get_admlevel(victim) >= char_get_admlevel(ch))
    send_to_char(g, ch, "Maybe you shouldn't do that.\r\n");
  else if (!*buf2)
    send_to_char(g, ch, "Where do you wish to send this person?\r\n");
  else if ((target = find_target_room(g, ch, buf2)) != NOWHERE) {
    if (char_plr_flagged(victim, PLR_HEALT)) {
      send_to_char(g, ch, "They are inside a healing tank!\r\n");
      return;
    }
    send_to_char(g, ch, "%s", CONFIG_OK);
    act(g, "$n disappears in a puff of smoke.", false, victim, 0, 0, TO_ROOM);
    char_from_room(g, victim);
    char_to_room(g, victim, target);
    act(g, "$n arrives from a puff of smoke.", false, victim, 0, 0, TO_ROOM);
    act(g, "$n has teleported you!", false, ch, 0, (char *)victim, TO_VICT);
    look_at_room(g, char_get_in_room(victim), victim, 0);
    enter_wtrigger(g, &g->rooms[char_get_in_room(victim)], victim, -1);
  }
}

void snoop_check(struct char_data *ch)
{
  /*  This short routine is to ensure that characters that happen
   *  to be snooping (or snooped) and get advanced/demoted will
   *  not be snooping/snooped someone of a higher/lower level (and
   *  thus, not entitled to be snooping.
   */
  if (!ch || !char_get_desc(ch))
    return;
  if (char_get_desc(ch)->snooping &&
      (char_get_admlevel(char_get_desc(ch)->snooping->character) >= char_get_admlevel(ch))) {
    char_get_desc(ch)->snooping->snoop_by = nullptr;
    char_get_desc(ch)->snooping           = nullptr;
  }

  if (char_get_desc(ch)->snoop_by &&
      (char_get_admlevel(ch) >= char_get_admlevel(char_get_desc(ch)->snoop_by->character))) {
    char_get_desc(ch)->snoop_by->snooping = nullptr;
    char_get_desc(ch)->snoop_by           = nullptr;
  }
}

static void stop_snooping(GameWorld *g, struct char_data *ch)
{
  if (!char_get_desc(ch)->snooping)
    send_to_char(g, ch, "You aren't snooping anyone.\r\n");
  else {
    send_to_char(g, ch, "You stop snooping.\r\n");
    char_get_desc(ch)->snooping->snoop_by = nullptr;
    char_get_desc(ch)->snooping           = nullptr;
  }
}

ACMD(do_snoop)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim, *tch;

  if (!char_get_desc(ch))
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(g, ch);
  else if (!(victim = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD)))
    send_to_char(g, ch, "No such person around.\r\n");
  else if (!char_get_desc(victim))
    send_to_char(g, ch, "There's no link.. nothing to snoop.\r\n");
  else if (victim == ch)
    stop_snooping(g, ch);
  else if (char_get_desc(victim)->snoop_by)
    send_to_char(g, ch, "Busy already. \r\n");
  else if (char_get_desc(victim)->snooping == char_get_desc(ch))
    send_to_char(g, ch, "Don't be stupid.\r\n");
  else {
    if (char_get_desc(victim)->original)
      tch = char_get_desc(victim)->original;
    else
      tch = victim;

    if (char_get_admlevel(tch) >= char_get_admlevel(ch)) {
      send_to_char(g, ch, "You can't.\r\n");
      return;
    }
    send_to_char(g, ch, "%s", CONFIG_OK);

    if (char_get_desc(ch)->snooping)
      char_get_desc(ch)->snooping->snoop_by = nullptr;

    char_get_desc(ch)->snooping     = char_get_desc(victim);
    char_get_desc(victim)->snoop_by = char_get_desc(ch);
  }
}

ACMD(do_switch)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  one_argument(argument, arg);

  if (char_get_desc(ch)->original)
    send_to_char(g, ch, "You're already switched.\r\n");
  else if (!*arg)
    send_to_char(g, ch, "Switch with who?\r\n");
  else if (!(victim = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD)))
    send_to_char(g, ch, "No such character.\r\n");
  else if (ch == victim)
    send_to_char(g, ch, "Hee hee... we are jolly funny today, eh?\r\n");
  else if (char_get_desc(victim))
    send_to_char(g, ch, "You can't do that, the body is already in use!\r\n");
  else if (!(char_is_npc(victim) || char_adm_flagged(ch, ADM_SWITCHMORTAL)))
    send_to_char(g, ch, "You aren't holy enough to use a mortal's body.\r\n");
  else if (char_get_admlevel(ch) < ADMLVL_VICE && rroom_flagged(g, char_get_in_room(victim), ROOM_GODROOM))
    send_to_char(g, ch, "You are not godly enough to use that room!\r\n");
  else {
    send_to_char(g, ch, "%s", CONFIG_OK);

    char_get_desc(ch)->character = victim;
    char_get_desc(ch)->original  = ch;

    char_set_desc(victim, char_get_desc(ch));
    char_set_desc(ch, nullptr);
  }
}

ACMD(do_return)
{
  if (char_get_desc(ch) && char_get_desc(ch)->original) {
    send_to_char(g, ch, "You return to your original body.\r\n");

    /*
     * If someone switched into your original body, disconnect them.
     *   - JE 2/22/95
     *
     * Zmey: here we put someone switched in our body to disconnect state
     * but we must also NULL his pointer to our character, otherwise
     * close() will damage our character's pointer to our descriptor
     * (which is assigned below in this function). 12/17/99
     */
    if (char_get_desc(char_get_desc(ch)->original)) {
      char_get_desc(char_get_desc(ch)->original)->character = nullptr;
      desc_set_connected(char_get_desc(char_get_desc(ch)->original), CON_DISCONNECT);
    }

    /* Now our descriptor points to our original body. */
    char_get_desc(ch)->character = char_get_desc(ch)->original;
    char_get_desc(ch)->original  = nullptr;

    /* And our body's pointer to descriptor now points to our descriptor. */
    char_set_desc(char_get_desc(ch)->character, char_get_desc(ch));
    char_set_desc(ch, nullptr);
  }
}

ACMD(do_load)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
  int i = 0, n = 1;

  one_argument(two_arguments(argument, buf, buf2), buf3);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(g, ch, "Usage: load { obj | mob } <vnum> (amt)\r\n");
    return;
  }
  if (!is_number(buf2) || !is_number(buf3)) {
    send_to_char(g, ch, "That is not a number.\r\n");
    return;
  }

  if (atoi(buf3) > 0) {
    if (atoi(buf3) >= 100) {
      n = 100;
    } else if (atoi(buf3) < 100) {
      n = atoi(buf3);
    }
  } else {
    n = 1;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob = nullptr;
    mob_rnum r_num;

    if ((r_num = real_mobile(g, atoi(buf2))) == NOBODY) {
      send_to_char(g, ch, "There is no monster with that number.\r\n");
      return;
    }
    for (i = 0; i < n; i++) {
      mob = read_mobile(g, r_num, REAL);
      char_to_room(g, mob, char_get_in_room(ch));

      act(g, "$n makes a quaint, magical gesture with one hand.", true, ch, 0, 0, TO_ROOM);
      act(g, "$n has created $N!", false, ch, 0, mob, TO_ROOM);
      act(g, "You create $N.", false, ch, 0, mob, TO_CHAR);
      load_mtrigger(g, mob);
    }
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

    if ((r_num = real_object(g, atoi(buf2))) == NOTHING) {
      send_to_char(g, ch, "There is no object with that number.\r\n");
      return;
    }
    for (i = 0; i < n; i++) {
      obj = read_object(g, r_num, REAL);
      add_unique_id(g, obj);
      if (char_get_admlevel(ch) > 0) {
        send_to_imm(g, "LOAD: %s has loaded a %s", char_get_name(ch), obj_get_short_description(obj));
        log_imm_action(g, "LOAD: %s has loaded a %s", char_get_name(ch), obj_get_short_description(obj));
      }
      if (CONFIG_LOAD_INVENTORY)
        obj_to_char(g, obj, ch);
      else
        obj_to_room(g, obj, char_get_in_room(ch));
      act(g, "$n makes a strange magical gesture.", true, ch, 0, 0, TO_ROOM);
      act(g, "$n has created $p!", false, ch, obj, 0, TO_ROOM);
      act(g, "You create $p.", false, ch, obj, 0, TO_CHAR);
      load_otrigger(g, obj);
    }
  } else
    send_to_char(g, ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct obj_data *obj;

  one_argument(argument, buf);

  /* argument supplied. destroy single object or char */
  if (*buf) {
    if ((vict = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_WORLD)) != nullptr) {
      if (!char_is_npc(vict) && (char_get_admlevel(ch) <= char_get_admlevel(vict))) {
        send_to_char(g, ch, "Fuuuuuuuuu!\r\n");
        return;
      }
      act(g, "$n disintegrates $N.", false, ch, 0, vict, TO_NOTVICT);
      if (!char_is_npc(vict)) {
        send_to_all(g, "@R%s@r purges @R%s's@r sorry ass right off the MUD!@n\r\n", char_get_name(ch), char_get_name(vict));
      }

      if (!char_is_npc(vict)) {
        mudlog(g, LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s has purged %s.", char_get_name(ch), char_get_name(vict));
        log_imm_action(g, "PURGED: %s burned %s's sorry ass off the MUD!", char_get_name(ch), char_get_name(vict));
        if (char_get_desc(vict)) {
          desc_set_connected(char_get_desc(vict), CON_CLOSE);
          char_get_desc(vict)->character = nullptr;
          char_set_desc(vict, nullptr);
        }
      }
      extract_char(g, vict);
    } else if ((obj = get_obj_in_list_vis(g, ch, buf, nullptr, rroom_get_contents(g, char_get_in_room(ch)))) != nullptr) {
      act(g, "$n destroys $p.", false, ch, obj, 0, TO_ROOM);
      extract_obj(g, obj);
    } else {
      send_to_char(g, ch, "Nothing here by that name.\r\n");
      return;
    }

    send_to_char(g, ch, "%s", CONFIG_OK);
  } else { /* no argument. clean out the room */
    int i;

    act(g, "$n gestures... You are surrounded by scorching flames!",
        false, ch, 0, 0, TO_ROOM);
    send_to_room(g, char_get_in_room(ch), "The world seems a little cleaner.\r\n");

    for (vict = rroom_get_people(g, char_get_in_room(ch)); vict; vict = char_get_next_in_room(vict)) {
      if (!char_is_npc(vict))
        continue;

      delete_inv_backup(vict);

      /* Dump inventory. */
      while (char_get_inventory(vict))
        extract_obj(g, char_get_inventory(vict));

      /* Dump equipment. */
      for (i = 0; i < NUM_WEARS; i++)
        if (char_get_equipment(vict, i))
          extract_obj(g, char_get_equipment(vict, i));

      /* Dump character. */
      extract_char(g, vict);
    }

    /* Clear the ground. */
    while (rroom_get_contents(g, char_get_in_room(ch)))
      extract_obj(g, rroom_get_contents(g, char_get_in_room(ch)));
  }
}

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char arg[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH + 32];

  half_chop(argument, arg, to_force);

  snprintf(buf1, sizeof(buf1), "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char(g, ch, "Whom do you wish to force do what?\r\n");
  else if (!char_adm_flagged(ch, ADM_FORCEMASS) || (strcasecmp("all", arg) && strcasecmp("room", arg))) {
    if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD)))
      send_to_char(g, ch, "%s", CONFIG_NOPERSON);
    else if (!char_is_npc(vict) && char_get_admlevel(ch) <= char_get_admlevel(vict))
      send_to_char(g, ch, "No, no, no!\r\n");
    else {
      send_to_char(g, ch, "%s", CONFIG_OK);
      act(g, buf1, true, ch, nullptr, vict, TO_VICT);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s forced %s to %s", char_get_name(ch), char_get_name(vict), to_force);
      command_interpreter(g, vict, to_force);
    }
  } else if (!strcasecmp("room", arg)) {
    send_to_char(g, ch, "%s", CONFIG_OK);
    mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s forced room %d to %s",
           char_get_name(ch), rroom_get_vnum(g, char_get_in_room(ch)), to_force);

    for (vict = rroom_get_people(g, char_get_in_room(ch)); vict; vict = next_force) {
      next_force = char_get_next_in_room(vict);
      if (!char_is_npc(vict) && char_get_admlevel(vict) >= char_get_admlevel(ch))
        continue;
      act(g, buf1, true, ch, nullptr, vict, TO_VICT);
      command_interpreter(g, vict, to_force);
    }
  } else { /* force all */
    send_to_char(g, ch, "%s", CONFIG_OK);
    mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s forced all to %s", char_get_name(ch), to_force);

    for (i = g->descriptors; i; i = next_desc) {
      next_desc = i->next;

      if (desc_get_connected(i) != CON_PLAYING || !(vict = i->character) || (!char_is_npc(vict) && char_get_admlevel(vict) >= char_get_admlevel(ch)))
        continue;
      act(g, buf1, true, ch, nullptr, vict, TO_VICT);
      command_interpreter(g, vict, to_force);
    }
  }
}

ACMD(do_zreset)
{
  char arg[MAX_INPUT_LENGTH];
  zone_rnum i;
  zone_vnum j;

  one_argument(argument, arg);

  if (*arg == '*') {
    if (char_get_admlevel(ch) < ADMLVL_VICE) {
      send_to_char(g, ch, "You do not have permission to reset the entire world.\r\n");
      return;
    }
    for (i = 0; i <= g->top_zone; i++) {
      if (i < 200) {
        reset_zone(g, i);
      }
    }
    send_to_char(g, ch, "Reset world.\r\n");
    mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "(GC) %s reset all MUD zones.", char_get_name(ch));
    log_imm_action(g, "RESET: %s has reset all MUD zones.", char_get_name(ch));
    return;
  } else if (*arg == '.' || !*arg)
    i = rroom_get_zone(g, char_get_in_room(ch));
  else {
    j = atoi(arg);
    for (i = 0; i <= g->top_zone; i++)
      if (g->zones[i].number == j)
        break;
  }
  if (i <= g->top_zone && (can_edit_zone(g, ch, i) || char_get_admlevel(ch) > ADMLVL_IMMORT)) {
    reset_zone(g, i);
    send_to_char(g, ch, "Reset zone #%d: %s.\r\n", g->zones[i].number, g->zones[i].name);
    mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "(GC) %s reset zone %d (%s)", char_get_name(ch), g->zones[i].number, g->zones[i].name);
    log_imm_action(g, "RESET: %s has reset zone #%d: %s.", char_get_name(ch), g->zones[i].number, g->zones[i].name);
  } else
    send_to_char(g, ch, "You do not have permission to reset this zone. Try %d.\r\n", char_get_olc_zone(g, ch));
}

ACMD(do_peace)
{
  struct char_data *vict, *next_v;
  send_to_room(g, char_get_in_room(ch), "Everything is quite peaceful now.\r\n");

  for (vict = rroom_get_people(g, char_get_in_room(ch)); vict; vict = next_v) {
    next_v = char_get_next_in_room(vict);
    if (char_get_admlevel(vict) > char_get_admlevel(ch))
      continue;
    stop_fighting(g, vict);
    char_set_position(vict, POS_SITTING);
  }
  stop_fighting(g, ch);
  char_set_position(ch, POS_STANDING);
}

ACMD(do_raise)
{
  struct char_data *vict = nullptr;
  char name[MAX_INPUT_LENGTH];

  one_argument(argument, name);

  if (char_get_admlevel(ch) < ADMLVL_BUILDER && !char_is_npc(ch)) {
    return;
  }

  if (!(vict = get_player_vis(g, ch, name, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "There is no such player.\r\n");
    return;
  }

  if (char_is_npc(vict)) {
    send_to_char(g, ch, "Sorry, only players get spirits.\r\n");
    return;
  }

  if (!char_aff_flagged(vict, AFF_SPIRIT)) {
    send_to_char(g, ch, "But they aren't even dead!\r\n");
    return;
  }

  if (char_get_admlevel(ch) <= 0) {
    resurrect(g, vict, 0);
  } else {
    resurrect(g, vict, 1);
  }

  send_to_char(g, ch, "@wYou return %s from the @Bspirit@w world, to the world of the living!@n\r\n", char_get_name(vict));
  send_to_char(g, vict, "@wYour @Bspirit@w has been returned to the world of the living by %s!@n\r\n", char_get_name(ch));

  send_to_imm(g, "Log: %s has raised %s from the dead.", char_get_name(ch), char_get_name(vict));
  log_imm_action(g, "RAISE: %s has raised %s from the dead.", char_get_name(ch), char_get_name(vict));
}

ACMD(do_zpurge)
{
  struct obj_data *obj, *next_obj;
  struct char_data *mob, *next_mob;
  IDXTYPE i, zone;
  zone_rnum stored = -1;
  int found        = false;
  room_vnum room;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    zone = g->zones[rroom_get_zone(g, char_get_in_room(ch))].number;
  } else {
    zone = atoi(arg);
  }

  for (i = 0; i <= g->top_zone && !found; i++) {
    if (g->zones[i].number == zone) {
      stored = i;
      found  = true;
    }
  }

  if (!found || !can_edit_zone(g, ch, zone)) {
    send_to_char(g, ch, "You cannot purge that zone. Try %d.\r\n", char_get_olc_zone(g, ch));
    return;
  }

  for (room = genolc_zone_bottom(g, stored); room <= g->zones[stored].top; room++) {
    if ((i = real_room(g, room)) != NOWHERE) {
      for (mob = rroom_get_people(g, i); mob; mob = next_mob) {
        next_mob = char_get_next_in_room(mob);
        if (char_is_npc(mob)) {
          extract_char(g, mob);
        }
      }

      for (obj = rroom_get_contents(g, i); obj; obj = next_obj) {
        next_obj = obj_get_next_content(obj);
        extract_obj(g, obj);
      }
    }
  }

  send_to_char(g, ch, "All mobiles and objects in zone %d purged.\r\n", zone);
  mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s has purged zone %d.", char_get_name(ch), zone);
}
