/* 
************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <span>
#include <string_view>

#include "dbat/args.hpp"
#include "dbat/act.social.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/social.hpp"
#include "dbat/config.hpp"
#include "dbat/db.hpp"
#include "dbat/comm.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/random.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static int find_action(GameWorld *g, int cmd);

ACMD(do_action)
{
  std::string arg, part;
  int act_nr;
  struct social_messg *action;
  struct char_data *vict;
  struct obj_data *targ;

  if ((act_nr = find_action(g, cmd)) < 0) {
    send_to_char(g, ch, "That action is not supported.\r\n");
    return;
  }

  action = &g->soc_mess_list[act_nr];

  if (argument.empty()) {
    send_to_char(g, ch, "{}\r\n", message_text(action->char_no_arg));
    act(g, message_text(action->others_no_arg), action->hide, ch, nullptr, nullptr, TO_ROOM);
    return;
  }

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  part                  = take_arg(rest);

  if ((!action->char_body_found) && !part.empty()) {
    send_to_char(g, ch, "Sorry, this social does not support body parts.\r\n");
    return;
  }

  if (!action->char_found) /* the target word only counts for socials that take one */
    arg.clear();

  vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM);
  if (!vict) {
    if (action->char_obj_found) {
      targ = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
      if (!targ) targ = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch)));
      if (targ) {
        act(g, message_text(action->char_obj_found), action->hide, ch, targ, nullptr, TO_CHAR);
        act(g, message_text(action->others_obj_found), action->hide, ch, targ, nullptr, TO_ROOM);
        return;
      }
    }
    if (action->not_found)
      send_to_char(g, ch, "{}\r\n", message_text(action->not_found));
    else
      send_to_char(g, ch, "I don't see anything by that name here.\r\n");
    return;
  }

  if (vict == ch) {
    if (action->char_auto)
      send_to_char(g, ch, "{}\r\n", message_text(action->char_auto));
    else
      send_to_char(g, ch, "Erm, no.\r\n");
    act(g, message_text(action->others_auto), action->hide, ch, nullptr, nullptr, TO_ROOM);
    return;
  }

  if (char_get_position(vict) < action->min_victim_position) {
    act(g, "$N is not in a proper position for that.", false, ch, nullptr, vict, TO_CHAR | TO_SLEEP);
    return;
  }
  /* The body part, if one is named, which act() reads as $t text */
  bool to_part         = !part.empty();
  struct obj_data *as_t = to_part ? static_cast<struct obj_data *>(static_cast<void *>(part.data())) : nullptr;
  act(g, message_text(to_part ? action->char_body_found : action->char_found), 0, ch, as_t, vict, TO_CHAR | TO_SLEEP);
  act(g, message_text(to_part ? action->others_body_found : action->others_found), action->hide, ch, as_t, vict, TO_NOTVICT);
  act(g, message_text(to_part ? action->vict_body_found : action->vict_found), action->hide, ch, as_t, vict, TO_VICT);
}

namespace {
  /* The first of the insults, by whether the insulter is male, then the insulted */
  constexpr const char *SEXED_INSULTS[2][2] = {
      {"$n tells you that you'd lose a beauty contest against a troll.", "$n accuses you of having the smallest... (brain?)"},
      {"$n says that women can't fight.",                                 "$n accuses you of fighting like a woman!"        },
  };
} // namespace

/* No command calls this any more: insult is out of the command table */
ACMD(do_insult)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "I'm sure you don't want to insult *everybody*...\r\n");
    return;
  }
  struct char_data *victim = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM);
  if (!victim) {
    send_to_char(g, ch, "Can't hear you!\r\n");
    return;
  }
  if (victim == ch) {
    send_to_char(g, ch, "You feel insulted.\r\n");
    return;
  }
  send_to_char(g, ch, "You insult {}.\r\n", char_get_name(victim));
  int roll           = rand_number(g, 0, 2);
  std::string_view insult = roll == 0   ? SEXED_INSULTS[char_get_sex(ch) == SEX_MALE][char_get_sex(victim) == SEX_MALE]
                       : roll == 1 ? "$n calls your mother a bitch!"
                                   : "$n tells you to get lost!";
  act(g, insult, false, ch, nullptr, victim, TO_VICT);
  act(g, "$n insults $N.", true, ch, nullptr, victim, TO_NOTVICT);
}

void create_command_list(GameWorld *g)
{
  int i, j, k;

  /* free up old command list */
  if (g->complete_cmd_info)
    free_command_list(g);

  /* re check the sort on the socials, ignoring case (stable, as glibc's qsort() sorted them) */
  if (g->soc_mess_list)
    std::ranges::stable_sort(std::span(g->soc_mess_list, g->top_of_socialt + 1), [](const social_messg &a, const social_messg &b) {
      return str_less_nocase(a.sort_as, b.sort_as);
    });

  /* count the commands in the command list */
  i = 0;
  while (cmd_info[i].command != "\n") i++;
  i++;

  g->complete_cmd_info = new command_info[g->top_of_socialt + i + 2]();

  /* this loop sorts the socials and commands together into one big list */
  i = 0;
  j = 0;
  k = 0;

  while (cmd_info[i].command != "\n") {
    g->complete_cmd_info[k++] = cmd_info[i++];
  }

  while (j <= g->top_of_socialt) {
    g->soc_mess_list[j].act_nr               = k;
    g->complete_cmd_info[k].command          = g->soc_mess_list[j].command;
    g->complete_cmd_info[k].sort_as          = g->soc_mess_list[j].sort_as;
    g->complete_cmd_info[k].minimum_position = g->soc_mess_list[j].min_char_position;
    g->complete_cmd_info[k].command_pointer  = do_action;
    g->complete_cmd_info[k].minimum_level    = g->soc_mess_list[j++].min_level_char;
    g->complete_cmd_info[k].minimum_admlevel = ADMLVL_NONE;
    g->complete_cmd_info[k++].subcmd         = 0;
  }

  g->complete_cmd_info[k].command          = "\n";
  g->complete_cmd_info[k].sort_as          = "zzzzzzz";
  g->complete_cmd_info[k].minimum_position = 0;
  g->complete_cmd_info[k].command_pointer  = nullptr;
  g->complete_cmd_info[k].minimum_level    = 0;
  g->complete_cmd_info[k].minimum_admlevel = 0;
  g->complete_cmd_info[k].subcmd           = 0;
  mud_log("Command info rebuilt, {:d} total commands.", k);
}

/* Its entries view cmd_info's and the socials' strings */
void free_command_list(GameWorld *g)
{
  delete[] g->complete_cmd_info;
  g->complete_cmd_info = nullptr;
}

void free_social_messages(GameWorld *g)
{
  delete[] g->soc_mess_list;
}

/* soc_mess_list is in act_nr order: create_command_list() numbers it that way. */
static int find_action(GameWorld *g, int cmd)
{
  if (g->top_of_socialt < 0)
    return -1;
  std::span socials(g->soc_mess_list, g->top_of_socialt + 1);
  auto found = std::ranges::lower_bound(socials, cmd, {}, &social_messg::act_nr);
  return found != socials.end() && found->act_nr == cmd ? static_cast<int>(found - socials.begin()) : -1;
}

ACMD(do_gmote)
{
  int act_nr;
  std::string arg, buf;
  std::string out;
  struct social_messg *action;
  struct char_data *vict = nullptr;

  std::string_view rest = argument;
  buf                   = take_word(rest);
  arg                   = skip_spaces(rest);

  if (subcmd)
    for (cmd = 0; g->complete_cmd_info[cmd].command != "\n"; cmd++)
      if (g->complete_cmd_info[cmd].command.starts_with(buf))
        break;

  if ((act_nr = find_action(g, cmd)) < 0) {
    out = str_format("@D[@BOOC@D: @g{} {}@n@D]", char_get_admlevel(ch) < 1 ? char_get_user(ch) : char_get_name(ch), argument);
    act(g, out, false, ch, nullptr, vict, TO_GMOTE);
    return;
  }

  if (room_flagged(g, char_get_in_room(ch), ROOM_SOUNDPROOF)) {
    send_to_char(g, ch, "The walls seem to absorb your actions.\r\n");
    return;
  }

  action = &g->soc_mess_list[act_nr];

  if (!action->char_found)
    arg.clear();

  std::string_view seen;
  if (arg.empty()) {
    if (message_text(action->others_no_arg).empty()) {
      send_to_char(g, ch, "Who are you going to do that to?\r\n");
      return;
    }
    seen = message_text(action->others_no_arg);
  } else if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "{}\r\n", message_text(action->not_found));
    return;
  } else if (vict == ch) {
    if (message_text(action->others_auto).empty()) {
      send_to_char(g, ch, "{}\r\n", message_text(action->char_auto));
      return;
    }
    seen = message_text(action->others_auto);
  } else if (char_get_position(vict) < action->min_victim_position) {
    act(g, "$N is not in a proper position for that.", false, ch, nullptr, vict, TO_CHAR | TO_SLEEP);
    return;
  } else {
    seen = message_text(action->others_found);
  }
  act(g, str_format("@D[@BOOC@D: @g{}@D]@n", seen), false, ch, nullptr, vict, TO_GMOTE);
}
