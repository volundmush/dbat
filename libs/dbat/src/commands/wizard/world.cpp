/* ************************************************************************
*   File: wizard/world.c                                  Part of CircleMUD *
*  Usage: God commands that move and change things                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <climits>
#include <cstdlib>

#include <unistd.h>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/players.hpp"
#include "dbat/utils.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/time.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/fileop.hpp"
#include "dbat/extract.hpp"
#include "dbat/affect.hpp"
#include "dbat/config.hpp"
#include "dbat/act.other.hpp"
#include "dbat/maputils.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/handler.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/weather.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/house.hpp"
#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/races.hpp"
#include "dbat/class.hpp"
#include "dbat/spells.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/objsave.hpp"
#include "dbat/feats.hpp"
#include "dbat/fight.hpp"
#include "dbat/genolc.hpp"
#include "dbat/screen.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/shop.hpp"
#include "dbat/guild.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static void stop_snooping(GameWorld *g, struct char_data *ch);

ACMD(do_transobj)
{

  struct obj_data *obj;
  struct char_data *vict;
  std::string arg;
  std::string arg2;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  if (!char_is_npc(ch) && char_get_admlevel(ch) < 1) {
    send_to_char(g, ch, "Huh!?");
    return;
  }

  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: transo (object) (target)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You want to send what?\r\n");
    return;
  }
  if (str_eq_nocase("all", arg2)) {
    obj_vnum num          = obj_get_vnum(g, obj);
    struct obj_data *obj2 = nullptr;

    act(g, "You send $p to everyone in the game.", true, ch, obj, nullptr, TO_CHAR);

    for (descriptor_data *d : each_desc(g)) {
      if (!desc_is_playing(d))
        continue;
      if (char_is_npc(d->character))
        continue;
      else if (d->character == ch)
        continue;
      else {
        act(g, "$N sends $p across the universe to you.", true, d->character, obj, ch, TO_CHAR);
        obj2 = read_object(g, num);
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
    act(g, "$n disappears in a burst of light!", false, ch, nullptr, nullptr, TO_ROOM);
    if (room_of(g, 2)) {
      char_from_room(g, ch);
      char_to_room(g, ch, 2);
      look_at_room(g, char_get_in_room(ch), ch, 0);
      char_set_load_room(g, ch, char_get_in_room(ch));
    }
  }
}

/* take a string, and return a room vnum, or NOWHERE.. used for goto, at, etc.  -je 4/6/93 */
room_vnum find_target_room(GameWorld *g, struct char_data *ch, std::string_view rawroomstr)
{
  room_vnum location = NOWHERE;
  struct room_data *rm;

  std::string roomstr = take_arg(rawroomstr);

  if (roomstr.empty()) {
    send_to_char(g, ch, "You must supply a room number or name.\r\n");
    return (NOWHERE);
  }

  if (isdigit(roomstr[0]) && roomstr.find('.') == std::string::npos) {
    if (!room_of(g, location = parse_int(roomstr))) {
      send_to_char(g, ch, "No room exists with that number.\r\n");
      return (NOWHERE);
    }
  } else {
    struct char_data *target_mob;
    struct obj_data *target_obj;
    std::string mobobjstr = roomstr;
    int num;

    num = get_number(mobobjstr);
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

  rm = room_of(g, location);

  if ((!can_edit_zone(g, ch, room_get_zone(rm)) && char_get_admlevel(ch) < ADMLVL_GOD) && zone_flagged(g, room_get_zone(rm), ZONE_QUEST)) {
    send_to_char(g, ch, "This target is in a quest zone.\r\n");
    return (NOWHERE);
  }

  if ((char_get_admlevel(ch) < ADMLVL_VICE) && zone_flagged(g, room_get_zone(rm), ZONE_NOIMMORT)) {
    send_to_char(g, ch, "This target is in a zone closed to all.\r\n");
    return (NOWHERE);
  }

  if (room_flagged(g, location, ROOM_GODROOM))
    send_to_char(g, ch, "You are not godly enough to use that room!\r\n");
  else
    return (location);

  return (NOWHERE);
}

ACMD(do_at)
{
  std::string command, buf;
  room_vnum location, original_loc;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  buf                   = take_word(rest);
  command               = skip_spaces(rest);
  if (buf.empty()) {
    send_to_char(g, ch, "You must supply a room number or a name.\r\n");
    return;
  }

  if (command.empty()) {
    send_to_char(g, ch, "What do you want to do there?\r\n");
    return;
  }

  if ((location = find_target_room(g, ch, buf)) == NOWHERE)
    return;

  /* a location has been found. */
  original_loc = char_get_in_room(ch);
  char_from_room(g, ch);
  char_to_room(g, ch, location);
  command_interpreter(g, ch, command.data());

  /* check if the char is still there */
  if (char_get_in_room(ch) == location) {
    char_from_room(g, ch);
    char_to_room(g, ch, original_loc);
  }
}

ACMD(do_goto)
{
  char buf[MAX_STRING_LENGTH];
  room_vnum location;

  if ((location = find_target_room(g, ch, argument)) == NOWHERE)
    return;
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "They are inside a healing tank!\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "$n %s", char_get_poofout(g, ch) ? char_get_poofout(g, ch) : "disappears in a puff of smoke.");
  act(g, buf, true, ch, nullptr, nullptr, TO_ROOM);

  char_from_room(g, ch);
  char_to_room(g, ch, location);

  snprintf(buf, sizeof(buf), "$n %s", char_get_poofin(g, ch) ? char_get_poofin(g, ch) : "appears with an ear-splitting bang.");
  act(g, buf, true, ch, nullptr, nullptr, TO_ROOM);

  look_at_room(g, char_get_in_room(ch), ch, 0);
  enter_wtrigger(g, room_of(g, char_get_in_room(ch)), ch, -1);
}

ACMD(do_trans)
{
  std::string buf;
  struct char_data *victim;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  buf                   = take_arg(rest);
  if (buf.empty())
    send_to_char(g, ch, "Whom do you wish to transfer?\r\n");
  else if (!str_eq_nocase("all", buf)) {
    if (!(victim = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_WORLD)))
      send_to_char(g, ch, "%s", CONFIG_NOPERSON.c_str());
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
      act(g, "$n disappears in a mushroom cloud.", false, victim, nullptr, nullptr, TO_ROOM);
      char_from_room(g, victim);
      char_to_room(g, victim, char_get_in_room(ch));
      act(g, "$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
      act(g, "$n has transferred you!", false, ch, nullptr, victim, TO_VICT);
      look_at_room(g, char_get_in_room(victim), victim, 0);
      enter_wtrigger(g, room_of(g, char_get_in_room(victim)), victim, -1);
    }
  } else { /* Trans All */
    if (!char_adm_flagged(ch, ADM_TRANSALL)) {
      send_to_char(g, ch, "I think not.\r\n");
      return;
    }

    for (descriptor_data *i : each_desc(g))
      if (desc_get_connected(i) == CON_PLAYING && i->character && i->character != ch) {
        victim = i->character;
        if (char_get_admlevel(victim) >= char_get_admlevel(ch))
          continue;
        act(g, "$n disappears in a mushroom cloud.", false, victim, nullptr, nullptr, TO_ROOM);
        char_from_room(g, victim);
        char_to_room(g, victim, char_get_in_room(ch));
        act(g, "$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
        act(g, "$n has transferred you!", false, ch, nullptr, victim, TO_VICT);
        look_at_room(g, char_get_in_room(victim), victim, 0);
        enter_wtrigger(g, room_of(g, char_get_in_room(victim)), victim, -1);
      }
    send_to_char(g, ch, "%s", CONFIG_OK.c_str());
  }
}

ACMD(do_teleport)
{
  std::string buf, buf2;
  struct char_data *victim;
  room_vnum target;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  buf                   = take_arg(rest);
  buf2                  = take_arg(rest);

  if (buf.empty())
    send_to_char(g, ch, "Whom do you wish to teleport?\r\n");
  else if (!(victim = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_WORLD)))
    send_to_char(g, ch, "%s", CONFIG_NOPERSON.c_str());
  else if (victim == ch)
    send_to_char(g, ch, "Use 'goto' to teleport yourself.\r\n");
  else if (char_get_admlevel(victim) >= char_get_admlevel(ch))
    send_to_char(g, ch, "Maybe you shouldn't do that.\r\n");
  else if (buf2.empty())
    send_to_char(g, ch, "Where do you wish to send this person?\r\n");
  else if ((target = find_target_room(g, ch, buf2)) != NOWHERE) {
    if (char_plr_flagged(victim, PLR_HEALT)) {
      send_to_char(g, ch, "They are inside a healing tank!\r\n");
      return;
    }
    send_to_char(g, ch, "%s", CONFIG_OK.c_str());
    act(g, "$n disappears in a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
    char_from_room(g, victim);
    char_to_room(g, victim, target);
    act(g, "$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
    act(g, "$n has teleported you!", false, ch, nullptr, (char *)victim, TO_VICT);
    look_at_room(g, char_get_in_room(victim), victim, 0);
    enter_wtrigger(g, room_of(g, char_get_in_room(victim)), victim, -1);
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
  std::string arg;
  struct char_data *victim, *tch;

  if (!char_get_desc(ch))
    return;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  arg                   = take_arg(rest);

  if (arg.empty())
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
    send_to_char(g, ch, "%s", CONFIG_OK.c_str());

    if (char_get_desc(ch)->snooping)
      char_get_desc(ch)->snooping->snoop_by = nullptr;

    char_get_desc(ch)->snooping     = char_get_desc(victim);
    char_get_desc(victim)->snoop_by = char_get_desc(ch);
  }
}

ACMD(do_switch)
{
  std::string arg;
  struct char_data *victim;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  arg                   = take_arg(rest);

  if (char_get_desc(ch)->original)
    send_to_char(g, ch, "You're already switched.\r\n");
  else if (arg.empty())
    send_to_char(g, ch, "Switch with who?\r\n");
  else if (!(victim = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD)))
    send_to_char(g, ch, "No such character.\r\n");
  else if (ch == victim)
    send_to_char(g, ch, "Hee hee... we are jolly funny today, eh?\r\n");
  else if (char_get_desc(victim))
    send_to_char(g, ch, "You can't do that, the body is already in use!\r\n");
  else if (!(char_is_npc(victim) || char_adm_flagged(ch, ADM_SWITCHMORTAL)))
    send_to_char(g, ch, "You aren't holy enough to use a mortal's body.\r\n");
  else if (char_get_admlevel(ch) < ADMLVL_VICE && room_flagged(g, char_get_in_room(victim), ROOM_GODROOM))
    send_to_char(g, ch, "You are not godly enough to use that room!\r\n");
  else {
    send_to_char(g, ch, "%s", CONFIG_OK.c_str());

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
  std::string buf, buf2, buf3;
  int i = 0, n = 1;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  buf                   = take_arg(rest);
  buf2                  = take_arg(rest);
  buf3                  = take_arg(rest);

  if (buf.empty() || buf2.empty() || !isdigit(buf2[0])) {
    send_to_char(g, ch, "Usage: load { obj | mob } <vnum> (amt)\r\n");
    return;
  }
  if (!is_number(buf2) || !is_number(buf3)) {
    send_to_char(g, ch, "That is not a number.\r\n");
    return;
  }

  if (parse_int(buf3) > 0) {
    if (parse_int(buf3) >= 100) {
      n = 100;
    } else if (parse_int(buf3) < 100) {
      n = parse_int(buf3);
    }
  } else {
    n = 1;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob = nullptr;
    mob_vnum r_num;

    if (!mob_proto_of(g, r_num = parse_int(buf2))) {
      send_to_char(g, ch, "There is no monster with that number.\r\n");
      return;
    }
    for (i = 0; i < n; i++) {
      mob = read_mobile(g, r_num);
      char_to_room(g, mob, char_get_in_room(ch));

      act(g, "$n makes a quaint, magical gesture with one hand.", true, ch, nullptr, nullptr, TO_ROOM);
      act(g, "$n has created $N!", false, ch, nullptr, mob, TO_ROOM);
      act(g, "You create $N.", false, ch, nullptr, mob, TO_CHAR);
      load_mtrigger(g, mob);
    }
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_vnum r_num;

    if (!obj_proto_of(g, r_num = parse_int(buf2))) {
      send_to_char(g, ch, "There is no object with that number.\r\n");
      return;
    }
    for (i = 0; i < n; i++) {
      obj = read_object(g, r_num);
      add_unique_id(g, obj);
      if (char_get_admlevel(ch) > 0) {
        send_to_imm(g, "LOAD: %s has loaded a %s", char_get_name(ch), obj_get_short_description(obj));
        log_imm_action(g, "LOAD: %s has loaded a %s", char_get_name(ch), obj_get_short_description(obj));
      }
      if (CONFIG_LOAD_INVENTORY)
        obj_to_char(g, obj, ch);
      else
        obj_to_room(g, obj, char_get_in_room(ch));
      act(g, "$n makes a strange magical gesture.", true, ch, nullptr, nullptr, TO_ROOM);
      act(g, "$n has created $p!", false, ch, obj, nullptr, TO_ROOM);
      act(g, "You create $p.", false, ch, obj, nullptr, TO_CHAR);
      load_otrigger(g, obj);
    }
  } else
    send_to_char(g, ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  std::string buf;
  struct char_data *vict;
  struct obj_data *obj;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  buf                   = take_arg(rest);

  /* argument supplied. destroy single object or char */
  if (!buf.empty()) {
    if ((vict = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_WORLD)) != nullptr) {
      if (!char_is_npc(vict) && (char_get_admlevel(ch) <= char_get_admlevel(vict))) {
        send_to_char(g, ch, "Fuuuuuuuuu!\r\n");
        return;
      }
      act(g, "$n disintegrates $N.", false, ch, nullptr, vict, TO_NOTVICT);
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
    } else if ((obj = get_obj_in_list_vis(g, ch, buf, nullptr, room_get_contents(g, char_get_in_room(ch)))) != nullptr) {
      act(g, "$n destroys $p.", false, ch, obj, nullptr, TO_ROOM);
      extract_obj(g, obj);
    } else {
      send_to_char(g, ch, "Nothing here by that name.\r\n");
      return;
    }

    send_to_char(g, ch, "%s", CONFIG_OK.c_str());
  } else { /* no argument. clean out the room */
    int i;

    act(g, "$n gestures... You are surrounded by scorching flames!",
        false, ch, nullptr, nullptr, TO_ROOM);
    send_to_room(g, char_get_in_room(ch), "The world seems a little cleaner.\r\n");

    for (char_data *tch : room_people(g, char_get_in_room(ch))) {
      if (!char_is_npc(tch))
        continue;

      delete_inv_backup(tch);

      /* Dump inventory. */
      while (char_get_inventory(tch))
        extract_obj(g, first_obj(g, char_get_inventory(tch)));

      /* Dump equipment. */
      for (i = 0; i < NUM_WEARS; i++)
        if (char_get_equipment(tch, i))
          extract_obj(g, char_get_equipment(tch, i));

      /* Dump character. */
      extract_char(g, tch);
    }

    /* Clear the ground. */
    while (room_get_contents(g, char_get_in_room(ch)))
      extract_obj(g, first_obj(g, room_get_contents(g, char_get_in_room(ch))));
  }
}

ACMD(do_force)
{
  struct char_data *vict;
  char buf1[MAX_INPUT_LENGTH + 32];
  std::string arg, to_force;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  arg                   = take_word(rest);
  to_force              = skip_spaces(rest);

  snprintf(buf1, sizeof(buf1), "$n has forced you to '%s'.", to_force.c_str());

  if (arg.empty() || to_force.empty())
    send_to_char(g, ch, "Whom do you wish to force do what?\r\n");
  else if (!char_adm_flagged(ch, ADM_FORCEMASS) || (!str_eq_nocase("all", arg) && !str_eq_nocase("room", arg))) {
    if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD)))
      send_to_char(g, ch, "%s", CONFIG_NOPERSON.c_str());
    else if (!char_is_npc(vict) && char_get_admlevel(ch) <= char_get_admlevel(vict))
      send_to_char(g, ch, "No, no, no!\r\n");
    else {
      send_to_char(g, ch, "%s", CONFIG_OK.c_str());
      act(g, buf1, true, ch, nullptr, vict, TO_VICT);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s forced %s to %s", char_get_name(ch), char_get_name(vict), to_force.c_str());
      command_interpreter(g, vict, to_force.data());
    }
  } else if (str_eq_nocase("room", arg)) {
    send_to_char(g, ch, "%s", CONFIG_OK.c_str());
    mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s forced room %d to %s",
           char_get_name(ch), char_get_in_room(ch), to_force.c_str());

    for (char_data *tch : room_people(g, char_get_in_room(ch))) {
      if (!char_is_npc(tch) && char_get_admlevel(tch) >= char_get_admlevel(ch))
        continue;
      act(g, buf1, true, ch, nullptr, tch, TO_VICT);
      command_interpreter(g, tch, to_force.data());
    }
  } else { /* force all */
    send_to_char(g, ch, "%s", CONFIG_OK.c_str());
    mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s forced all to %s", char_get_name(ch), to_force.c_str());

    for (descriptor_data *i : each_desc(g)) {

      if (desc_get_connected(i) != CON_PLAYING || !(vict = i->character) || (!char_is_npc(vict) && char_get_admlevel(vict) >= char_get_admlevel(ch)))
        continue;
      act(g, buf1, true, ch, nullptr, vict, TO_VICT);
      command_interpreter(g, vict, to_force.data());
    }
  }
}

ACMD(do_zreset)
{
  std::string arg;
  struct zone_data *i;
  zone_vnum j;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  arg                   = take_arg(rest);

  if (arg[0] == '*') {
    if (char_get_admlevel(ch) < ADMLVL_VICE) {
      send_to_char(g, ch, "You do not have permission to reset the entire world.\r\n");
      return;
    }
    int place = 0; /* the first 200 zones in vnum order, as when zones were an array */
    for (struct zone_data *zone : g->zones.all())
      if (place++ < 200)
        reset_zone(g, zone);
    send_to_char(g, ch, "Reset world.\r\n");
    mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "(GC) %s reset all MUD zones.", char_get_name(ch));
    log_imm_action(g, "RESET: %s has reset all MUD zones.", char_get_name(ch));
    return;
  } else if (arg[0] == '.' || arg.empty())
    i = room_get_zone(g, char_get_in_room(ch));
  else {
    j = parse_int(arg);
    i = find_zone(g, j);
  }
  if (i && (can_edit_zone(g, ch, i) || char_get_admlevel(ch) > ADMLVL_IMMORT)) {
    reset_zone(g, i);
    send_to_char(g, ch, "Reset zone #%d: %s.\r\n", i->number, i->name.c_str());
    mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "(GC) %s reset zone %d (%s)", char_get_name(ch), i->number, i->name.c_str());
    log_imm_action(g, "RESET: %s has reset zone #%d: %s.", char_get_name(ch), i->number, i->name.c_str());
  } else
    send_to_char(g, ch, "You do not have permission to reset this zone. Try %d.\r\n", char_get_olc_zone(g, ch));
}

ACMD(do_peace)
{
  send_to_room(g, char_get_in_room(ch), "Everything is quite peaceful now.\r\n");

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
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
  std::string name;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  name                  = take_arg(rest);

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
  IDXTYPE i, zone;
  struct zone_data *stored = nullptr;
  int found        = false;
  room_vnum room;
  std::string arg;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  arg                   = take_arg(rest);

  if (arg.empty()) {
    zone = room_get_zone(g, char_get_in_room(ch))->number;
  } else {
    zone = parse_int(arg);
  }

  stored = find_zone(g, zone);
  found  = stored != nullptr;

  if (!found || !can_edit_zone(g, ch, stored)) {
    send_to_char(g, ch, "You cannot purge that zone. Try %d.\r\n", char_get_olc_zone(g, ch));
    return;
  }

  for (room = genolc_zone_bottom(g, stored); room <= stored->top; room++) {
    if (room_of(g, i = room)) {
      for (char_data *mob : room_people(g, i)) {
        if (char_is_npc(mob)) {
          extract_char(g, mob);
        }
      }

      for (obj_data *obj : room_contents(g, i)) {
        extract_obj(g, obj);
      }
    }
  }

  send_to_char(g, ch, "All mobiles and objects in zone %d purged.\r\n", zone);
  mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s has purged zone %d.", char_get_name(ch), zone);
}
