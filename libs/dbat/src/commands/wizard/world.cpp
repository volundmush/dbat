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
#include "dbat/cmdargs.hpp"
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

namespace {
  /* victim, by ch's doing, to where ch is */
  void transfer(GameWorld *g, struct char_data *ch, struct char_data *victim)
  {
    act(g, "$n disappears in a mushroom cloud.", false, victim, nullptr, nullptr, TO_ROOM);
    char_from_room(g, victim);
    char_to_room(g, victim, char_get_in_room(ch));
    act(g, "$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
    act(g, "$n has transferred you!", false, ch, nullptr, victim, TO_VICT);
    look_at_room(g, char_get_in_room(victim), victim, 0);
    enter_wtrigger(g, room_of(g, char_get_in_room(victim)), victim, -1);
  }

  void stop_snooping(GameWorld *g, struct char_data *ch)
  {
    struct descriptor_data *d = char_get_desc(ch);
    if (!d->snooping) {
      send_to_char(g, ch, "You aren't snooping anyone.\r\n");
      return;
    }
    send_to_char(g, ch, "You stop snooping.\r\n");
    d->snooping->snoop_by = nullptr;
    d->snooping           = nullptr;
  }
} // namespace

ACMD(do_transobj)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (!char_is_npc(ch) && char_get_admlevel(ch) < 1) {
    send_to_char(g, ch, "Huh!?");
    return;
  }
  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: transo (object) (target)\r\n");
    return;
  }
  auto found = find_carried(g, ch, arg, "You want to send what?\r\n");
  if (reject(g, ch, found))
    return;
  struct obj_data *obj = *found;

  if (!str_eq_nocase("all", arg2)) {
    auto to = find_char(g, ch, arg2, FIND_CHAR_WORLD, "That player is not in the game.\r\n");
    if (reject(g, ch, to))
      return;
    act(g, "You send $p to $N.", true, ch, obj, *to, TO_CHAR);
    act(g, "$n sends $p across the universe to you.", true, ch, obj, *to, TO_VICT);
    obj_from_char(g, obj);
    obj_to_char(g, obj, *to);
    return;
  }

  /* Everyone gets a copy, made from its prototype */
  obj_vnum num = obj_get_vnum(g, obj); /* quirk: for something with none, everyone is told of it but gets nothing */
  act(g, "You send $p to everyone in the game.", true, ch, obj, nullptr, TO_CHAR);
  for (descriptor_data *d : each_desc(g)) {
    if (!desc_is_playing(d) || char_is_npc(d->character) || d->character == ch)
      continue;
    act(g, "$N sends $p across the universe to you.", true, d->character, obj, ch, TO_CHAR);
    obj_to_char(g, read_object(g, num), d->character);
  }
}

ACMD(do_recall)
{
  if (char_get_admlevel(ch) < 1) {
    send_to_char(g, ch, "You are not an immortal!\r\n");
    return;
  }
  send_to_char(g, ch, "You disappear in a burst of light!\r\n");
  act(g, "$n disappears in a burst of light!", false, ch, nullptr, nullptr, TO_ROOM);
  if (!room_of(g, 2))
    return;
  char_from_room(g, ch);
  char_to_room(g, ch, 2);
  look_at_room(g, char_get_in_room(ch), ch, 0);
  char_set_load_room(g, ch, char_get_in_room(ch));
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
  std::string_view rest = argument;
  std::string buf       = take_word(rest);
  std::string command(skip_spaces(rest));

  if (buf.empty()) {
    send_to_char(g, ch, "You must supply a room number or a name.\r\n");
    return;
  }
  if (command.empty()) {
    send_to_char(g, ch, "What do you want to do there?\r\n");
    return;
  }
  room_vnum location = find_target_room(g, ch, buf);
  if (location == NOWHERE)
    return;

  room_vnum original_loc = char_get_in_room(ch);
  char_from_room(g, ch);
  char_to_room(g, ch, location);
  command_interpreter(g, ch, command);
  if (char_get_in_room(ch) == location) { /* unless the command took them elsewhere */
    char_from_room(g, ch);
    char_to_room(g, ch, original_loc);
  }
}

ACMD(do_goto)
{
  room_vnum location = find_target_room(g, ch, argument);
  if (location == NOWHERE)
    return;
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "They are inside a healing tank!\r\n");
    return;
  }

  act(g, str_format("$n {}", char_get_poofout(g, ch) ? char_get_poofout(g, ch) : "disappears in a puff of smoke.").c_str(), true, ch, nullptr, nullptr, TO_ROOM);
  char_from_room(g, ch);
  char_to_room(g, ch, location);
  act(g, str_format("$n {}", char_get_poofin(g, ch) ? char_get_poofin(g, ch) : "appears with an ear-splitting bang.").c_str(), true, ch, nullptr, nullptr, TO_ROOM);
  look_at_room(g, char_get_in_room(ch), ch, 0);
  enter_wtrigger(g, room_of(g, char_get_in_room(ch)), ch, -1);
}

ACMD(do_trans)
{
  std::string_view rest = argument;
  std::string buf       = take_arg(rest);

  if (buf.empty()) {
    send_to_char(g, ch, "Whom do you wish to transfer?\r\n");
    return;
  }
  if (str_eq_nocase("all", buf)) {
    if (!char_adm_flagged(ch, ADM_TRANSALL)) {
      send_to_char(g, ch, "I think not.\r\n");
      return;
    }
    for (descriptor_data *i : each_desc(g))
      if (desc_get_connected(i) == CON_PLAYING && i->character && i->character != ch && char_get_admlevel(i->character) < char_get_admlevel(ch))
        transfer(g, ch, i->character);
    send_to_char(g, ch, CONFIG_OK);
    return;
  }

  auto found = find_char(g, ch, buf, FIND_CHAR_WORLD, CONFIG_NOPERSON);
  if (reject(g, ch, found))
    return;
  struct char_data *victim = *found;
  if (victim == ch) {
    send_to_char(g, ch, "That doesn't make much sense, does it?\r\n");
    return;
  }
  if (char_get_admlevel(ch) < char_get_admlevel(victim) && !char_is_npc(victim)) {
    send_to_char(g, ch, "Go transfer someone your own size.\r\n");
    return;
  }
  if (char_plr_flagged(victim, PLR_HEALT)) {
    send_to_char(g, ch, "They are inside a healing tank!\r\n");
    return;
  }
  transfer(g, ch, victim);
}

ACMD(do_teleport)
{
  std::string_view rest = argument;
  std::string buf       = take_arg(rest);
  std::string buf2      = take_arg(rest);

  if (buf.empty()) {
    send_to_char(g, ch, "Whom do you wish to teleport?\r\n");
    return;
  }
  auto found = find_char(g, ch, buf, FIND_CHAR_WORLD, CONFIG_NOPERSON);
  if (reject(g, ch, found))
    return;
  struct char_data *victim = *found;
  const char *refusal      = nullptr;
  if (victim == ch)
    refusal = "Use 'goto' to teleport yourself.\r\n";
  else if (char_get_admlevel(victim) >= char_get_admlevel(ch))
    refusal = "Maybe you shouldn't do that.\r\n";
  else if (buf2.empty())
    refusal = "Where do you wish to send this person?\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  room_vnum target = find_target_room(g, ch, buf2);
  if (target == NOWHERE)
    return;
  if (char_plr_flagged(victim, PLR_HEALT)) {
    send_to_char(g, ch, "They are inside a healing tank!\r\n");
    return;
  }
  send_to_char(g, ch, CONFIG_OK);
  act(g, "$n disappears in a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
  char_from_room(g, victim);
  char_to_room(g, victim, target);
  act(g, "$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
  act(g, "$n has teleported you!", false, ch, nullptr, victim, TO_VICT);
  look_at_room(g, char_get_in_room(victim), victim, 0);
  enter_wtrigger(g, room_of(g, char_get_in_room(victim)), victim, -1);
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

ACMD(do_snoop)
{
  struct descriptor_data *d = char_get_desc(ch);
  if (!d)
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    stop_snooping(g, ch);
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_WORLD, "No such person around.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *victim = *found;
  struct descriptor_data *vd = char_get_desc(victim);
  if (!vd) {
    send_to_char(g, ch, "There's no link.. nothing to snoop.\r\n");
    return;
  }
  if (victim == ch) {
    stop_snooping(g, ch);
    return;
  }
  if (vd->snoop_by) {
    send_to_char(g, ch, "Busy already. \r\n");
    return;
  }
  if (vd->snooping == d) {
    send_to_char(g, ch, "Don't be stupid.\r\n");
    return;
  }
  struct char_data *tch = vd->original ? vd->original : victim; /* whoever is really playing */
  if (char_get_admlevel(tch) >= char_get_admlevel(ch)) {
    send_to_char(g, ch, "You can't.\r\n");
    return;
  }
  send_to_char(g, ch, CONFIG_OK);
  if (d->snooping)
    d->snooping->snoop_by = nullptr;
  d->snooping  = vd;
  vd->snoop_by = d;
}

ACMD(do_switch)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  struct descriptor_data *d = char_get_desc(ch);

  if (d->original) {
    send_to_char(g, ch, "You're already switched.\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "Switch with who?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_WORLD, "No such character.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *victim = *found;
  const char *refusal      = nullptr;
  if (ch == victim)
    refusal = "Hee hee... we are jolly funny today, eh?\r\n";
  else if (char_get_desc(victim))
    refusal = "You can't do that, the body is already in use!\r\n";
  else if (!char_is_npc(victim) && !char_adm_flagged(ch, ADM_SWITCHMORTAL))
    refusal = "You aren't holy enough to use a mortal's body.\r\n";
  else if (char_get_admlevel(ch) < ADMLVL_VICE && room_flagged(g, char_get_in_room(victim), ROOM_GODROOM))
    refusal = "You are not godly enough to use that room!\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }
  send_to_char(g, ch, CONFIG_OK);
  d->character = victim;
  d->original  = ch;
  char_set_desc(victim, d);
  char_set_desc(ch, nullptr);
}

ACMD(do_return)
{
  struct descriptor_data *d = char_get_desc(ch);
  if (!d || !d->original)
    return;

  send_to_char(g, ch, "You return to your original body.\r\n");
  /* Whoever switched into the original body is disconnected, and let go of it, so that closing their connection
   * doesn't take the body's away (JE 2/22/95, Zmey 12/17/99) */
  if (struct descriptor_data *usurper = char_get_desc(d->original)) {
    usurper->character = nullptr;
    desc_set_connected(usurper, CON_DISCONNECT);
  }
  d->character = d->original;
  d->original  = nullptr;
  char_set_desc(d->character, d);
  char_set_desc(ch, nullptr);
}

ACMD(do_load)
{
  std::string_view rest = argument;
  std::string kind      = take_arg(rest);
  std::string number    = take_arg(rest);
  std::string amount    = take_arg(rest);

  if (kind.empty() || number.empty() || !isdigit(number[0])) {
    send_to_char(g, ch, "Usage: load { obj | mob } <vnum> (amt)\r\n");
    return;
  }
  if (!is_number(number) || !is_number(amount)) {
    send_to_char(g, ch, "That is not a number.\r\n");
    return;
  }
  int n = std::clamp(parse_int(amount), 1, 100);

  if (is_abbrev(kind, "mob")) {
    mob_vnum vnum = parse_int(number);
    if (!mob_proto_of(g, vnum)) {
      send_to_char(g, ch, "There is no monster with that number.\r\n");
      return;
    }
    for (int i = 0; i < n; i++) {
      struct char_data *mob = read_mobile(g, vnum);
      char_to_room(g, mob, char_get_in_room(ch));
      act(g, "$n makes a quaint, magical gesture with one hand.", true, ch, nullptr, nullptr, TO_ROOM);
      act(g, "$n has created $N!", false, ch, nullptr, mob, TO_ROOM);
      act(g, "You create $N.", false, ch, nullptr, mob, TO_CHAR);
      load_mtrigger(g, mob);
    }
    return;
  }
  if (!is_abbrev(kind, "obj")) {
    send_to_char(g, ch, "That'll have to be either 'obj' or 'mob'.\r\n");
    return;
  }
  obj_vnum vnum = parse_int(number);
  if (!obj_proto_of(g, vnum)) {
    send_to_char(g, ch, "There is no object with that number.\r\n");
    return;
  }
  for (int i = 0; i < n; i++) {
    struct obj_data *obj = read_object(g, vnum);
    add_unique_id(g, obj);
    if (char_get_admlevel(ch) > 0) {
      send_to_imm(g, "LOAD: {} has loaded a {}", char_get_name(ch), obj_get_short_description(obj));
      log_imm_action(g, "LOAD: {} has loaded a {}", char_get_name(ch), obj_get_short_description(obj));
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
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  std::string_view rest = argument;
  std::string buf       = take_arg(rest);

  if (buf.empty()) {
    act(g, "$n gestures... You are surrounded by scorching flames!", false, ch, nullptr, nullptr, TO_ROOM);
    send_to_room(g, char_get_in_room(ch), "The world seems a little cleaner.\r\n");
    for (char_data *tch : room_people(g, char_get_in_room(ch))) {
      if (!char_is_npc(tch))
        continue;
      delete_inv_backup(tch);
      while (char_get_inventory(tch))
        extract_obj(g, first_obj(g, char_get_inventory(tch)));
      for (int i = 0; i < NUM_WEARS; i++)
        if (char_get_equipment(tch, i))
          extract_obj(g, char_get_equipment(tch, i));
      extract_char(g, tch);
    }
    while (room_get_contents(g, char_get_in_room(ch)))
      extract_obj(g, first_obj(g, room_get_contents(g, char_get_in_room(ch))));
    return;
  }

  if (struct char_data *vict = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_WORLD)) {
    if (!char_is_npc(vict) && char_get_admlevel(ch) <= char_get_admlevel(vict)) {
      send_to_char(g, ch, "Fuuuuuuuuu!\r\n");
      return;
    }
    act(g, "$n disintegrates $N.", false, ch, nullptr, vict, TO_NOTVICT);
    if (!char_is_npc(vict)) {
      send_to_all(g, "@R{}@r purges @R{}'s@r sorry ass right off the MUD!@n\r\n", char_get_name(ch), char_get_name(vict));
      mudlog(g, LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) {} has purged {}.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "PURGED: {} burned {}'s sorry ass off the MUD!", char_get_name(ch), char_get_name(vict));
      if (struct descriptor_data *d = char_get_desc(vict)) {
        desc_set_connected(d, CON_CLOSE);
        d->character = nullptr;
        char_set_desc(vict, nullptr);
      }
    }
    extract_char(g, vict);
  } else if (struct obj_data *obj = get_obj_in_list_vis(g, ch, buf, nullptr, room_get_contents(g, char_get_in_room(ch)))) {
    act(g, "$n destroys $p.", false, ch, obj, nullptr, TO_ROOM);
    extract_obj(g, obj);
  } else {
    send_to_char(g, ch, "Nothing here by that name.\r\n");
    return;
  }
  send_to_char(g, ch, CONFIG_OK);
}

ACMD(do_force)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string to_force(skip_spaces(rest));
  std::string forced    = str_format("$n has forced you to '{}'.", to_force);
  int log_level         = MAX(ADMLVL_GOD, char_get_invis_level(g, ch));

  if (arg.empty() || to_force.empty()) {
    send_to_char(g, ch, "Whom do you wish to force do what?\r\n");
    return;
  }
  bool mass = char_adm_flagged(ch, ADM_FORCEMASS) && (str_eq_nocase("all", arg) || str_eq_nocase("room", arg));
  if (!mass) {
    auto found = find_char(g, ch, arg, FIND_CHAR_WORLD, CONFIG_NOPERSON);
    if (reject(g, ch, found))
      return;
    struct char_data *vict = *found;
    if (!char_is_npc(vict) && char_get_admlevel(ch) <= char_get_admlevel(vict)) {
      send_to_char(g, ch, "No, no, no!\r\n");
      return;
    }
    send_to_char(g, ch, CONFIG_OK);
    act(g, forced.c_str(), true, ch, nullptr, vict, TO_VICT);
    mudlog(g, LOG_NRM, log_level, true, "(GC) {} forced {} to {}", char_get_name(ch), char_get_name(vict), to_force);
    command_interpreter(g, vict, to_force);
    return;
  }

  send_to_char(g, ch, CONFIG_OK);
  if (str_eq_nocase("room", arg)) {
    mudlog(g, LOG_NRM, log_level, true, "(GC) {} forced room {:d} to {}", char_get_name(ch), char_get_in_room(ch), to_force);
    for (char_data *tch : room_people(g, char_get_in_room(ch))) {
      if (!char_is_npc(tch) && char_get_admlevel(tch) >= char_get_admlevel(ch))
        continue;
      act(g, forced.c_str(), true, ch, nullptr, tch, TO_VICT);
      command_interpreter(g, tch, to_force);
    }
    return;
  }
  mudlog(g, LOG_NRM, log_level, true, "(GC) {} forced all to {}", char_get_name(ch), to_force);
  for (descriptor_data *i : each_desc(g)) {
    struct char_data *vict = i->character;
    if (desc_get_connected(i) != CON_PLAYING || !vict || (!char_is_npc(vict) && char_get_admlevel(vict) >= char_get_admlevel(ch)))
      continue;
    act(g, forced.c_str(), true, ch, nullptr, vict, TO_VICT);
    command_interpreter(g, vict, to_force);
  }
}

ACMD(do_zreset)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

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
    mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "(GC) {} reset all MUD zones.", char_get_name(ch));
    log_imm_action(g, "RESET: {} has reset all MUD zones.", char_get_name(ch));
    return;
  }

  struct zone_data *zone = arg.empty() || arg[0] == '.' ? room_get_zone(g, char_get_in_room(ch)) : find_zone(g, parse_int(arg));
  if (!zone || (!can_edit_zone(g, ch, zone) && char_get_admlevel(ch) <= ADMLVL_IMMORT)) {
    send_to_char(g, ch, "You do not have permission to reset this zone. Try {:d}.\r\n", char_get_olc_zone(g, ch));
    return;
  }
  reset_zone(g, zone);
  send_to_char(g, ch, "Reset zone #{:d}: {}.\r\n", zone->number, zone->name);
  mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "(GC) {} reset zone {:d} ({})", char_get_name(ch), zone->number, zone->name);
  log_imm_action(g, "RESET: {} has reset zone #{:d}: {}.", char_get_name(ch), zone->number, zone->name);
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
  std::string_view rest = argument;
  std::string name      = take_arg(rest);

  if (char_get_admlevel(ch) < ADMLVL_BUILDER && !char_is_npc(ch))
    return;
  auto found = find_player(g, ch, name, FIND_CHAR_WORLD, "There is no such player.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (char_is_npc(vict)) {
    send_to_char(g, ch, "Sorry, only players get spirits.\r\n");
    return;
  }
  if (!char_aff_flagged(vict, AFF_SPIRIT)) {
    send_to_char(g, ch, "But they aren't even dead!\r\n");
    return;
  }

  resurrect(g, vict, char_get_admlevel(ch) > 0 ? 1 : 0);
  send_to_char(g, ch, "@wYou return {} from the @Bspirit@w world, to the world of the living!@n\r\n", char_get_name(vict));
  send_to_char(g, vict, "@wYour @Bspirit@w has been returned to the world of the living by {}!@n\r\n", char_get_name(ch));
  send_to_imm(g, "Log: {} has raised {} from the dead.", char_get_name(ch), char_get_name(vict));
  log_imm_action(g, "RAISE: {} has raised {} from the dead.", char_get_name(ch), char_get_name(vict));
}

ACMD(do_zpurge)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  IDXTYPE zone          = arg.empty() ? room_get_zone(g, char_get_in_room(ch))->number : parse_int(arg);
  struct zone_data *stored = find_zone(g, zone);

  if (!stored || !can_edit_zone(g, ch, stored)) {
    send_to_char(g, ch, "You cannot purge that zone. Try {:d}.\r\n", char_get_olc_zone(g, ch));
    return;
  }
  for (room_vnum room = genolc_zone_bottom(g, stored); room <= stored->top; room++) {
    if (!room_of(g, room))
      continue;
    for (char_data *mob : room_people(g, room))
      if (char_is_npc(mob))
        extract_char(g, mob);
    for (obj_data *obj : room_contents(g, room))
      extract_obj(g, obj);
  }
  send_to_char(g, ch, "All mobiles and objects in zone {:d} purged.\r\n", zone);
  mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) {} has purged zone {:d}.", char_get_name(ch), zone);
}
