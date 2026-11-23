/* ************************************************************************
*   File: wizard/server.c                                  Part of CircleMUD *
*  Usage: God commands that control the server                            *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/utils.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/comm.hpp"
#include "dbat/genolc.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/copyover.hpp"

/* local variables */

// definitions
ACMD(do_lag)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: lag (target) (number of seconds)\r\n");
    return;
  }

  int num = parse_int(arg2);
  if (num <= 0 || num > 5) {
    send_to_char(g, ch, "Keep it between 1 to 5 seconds please.\r\n");
    return;
  }

  bool found = false;
  for (descriptor_data *d : each_desc(g)) {
    if (!d->character || char_get_name(d->character).empty() || !str_eq_nocase(char_get_name(d->character), arg)) /* or still at the login menus */
      continue;
    if (char_get_admlevel(d->character) > char_get_admlevel(ch)) {
      send_to_char(g, ch, "Sorry, you've been outranked.\r\n");
      continue;
    }
    char_set_wait(d->character, num * PULSE_1SEC);
    found = true;
  }
  if (!found)
    send_to_char(g, ch, "That player isn't around.\r\n");
}

namespace {
  /* What "shutdown <option>" announces and does */
  struct shutdown_mode {
    const char *option; /* "" for none */
    const char *logged; /* "(GC) <this> by <who>." */
    const char *announced;
    const char *touched; /* the file left for autorun, or null */
    int reboot;
  };
  const shutdown_mode SHUTDOWN_MODES[] = {
    {"", "Shutdown", "Shutting down.\r\n", nullptr, 0},
    {"reboot", "Reboot", "Rebooting.. come back in a minute or two.\r\n", FASTBOOT_FILE, 1},
    {"die", "Shutdown", "Shutting down for maintenance.\r\n", KILLSCRIPT_FILE, 0},
    {"now", "Shutdown NOW", "Rebooting.. come back in a minute or two.\r\n", nullptr, 2 /* do not autosave olc */},
    {"pause", "Shutdown", "Shutting down for maintenance.\r\n", PAUSE_FILE, 0},
  };
} // namespace

ACMD(do_shutdown)
{
  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char(g, ch, "If you want to shut something down, say so!\r\n");
    return;
  }
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  auto mode = std::ranges::find_if(SHUTDOWN_MODES, [&](const shutdown_mode &m) { return arg.empty() ? !*m.option : str_eq_nocase(arg, m.option); });
  if (mode == std::end(SHUTDOWN_MODES)) {
    send_to_char(g, ch, "Unknown shutdown option.\r\n");
    return;
  }
  mud_log("(GC) {} by {}.", mode->logged, char_get_name(ch));
  send_to_all(g, mode->announced);
  if (mode->touched)
    io::touch(mode->touched);
  g->circle_shutdown = 1;
  if (mode->reboot)
    g->circle_reboot = mode->reboot;
}


/* (c) 1996-97 Erwin S. Andreasen <erwin@pip.dknet.dk> */
ACMD(do_copyover)
{
  std::string arg;
  int secs;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  if (arg.empty()) {
    execute_copyover(g);
  } else if (is_abbrev(arg, "cancel") || is_abbrev(arg, "stop")) {
    if (!g->copyover_timer) {
      send_to_char(g, ch, "A timed copyover has not been started!\r\n");
    } else {
      g->copyover_timer = 0;
      game_info(g, "Copyover cancelled");
    }
  } else if (is_abbrev(arg, "help")) {
    send_to_char(g, ch, "COPYOVER\r\n\r\n");
    send_to_char(g, ch, "Usage: @ycopyover@n           - Perform an immediate copyover\r\n       @ycopyover <seconds>@n - Start a timed copyover\r\n       @ycopyover cancel@n    - Stop a timed copyover\r\n\r\n");
    send_to_char(g, ch, "A timed copyover will produce an automatic warning when it starts, and then\r\n");
    send_to_char(g, ch, "every minute.  During the last minute, there will be more frequent warnings.\r\n");
  } else {
    secs = parse_int(arg);
    if (!secs || secs < 0) {
      send_to_char(g, ch, "Type @ycopyover help@n for usage info.");
    } else {
      g->copyover_timer = secs;
      mud_log("-- Timed Copyover started by {} - {:d} seconds until copyover --", char_get_name(ch), secs);
      if (secs >= 60) {
        if (secs % 60) {
          game_info(g, "A copyover will be performed in {:d} minutes and {:d} seconds.", (g->copyover_timer / 60), (g->copyover_timer % 60));
        } else {
          game_info(g, "A copyover will be performed in {:d} minute{}.", (g->copyover_timer / 60), (g->copyover_timer / 60) > 1 ? "s" : "");
        }
      } else {
        game_info(g, "A copyover will be performed in {:d} seconds.", g->copyover_timer);
      }
    }
  }
}


ACMD(do_dc)
{
  std::string arg;
  struct descriptor_data *d;
  int num_to_dc;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  if (!(num_to_dc = parse_int(arg))) {
    send_to_char(g, ch, "Usage: DC <user number> (type USERS for a list)\r\n");
    return;
  }
  d = find_first(each_desc(g), [&](descriptor_data *conn) { return conn->desc_num == num_to_dc; });
  if (!d) {
    send_to_char(g, ch, "No such connection.\r\n");
    return;
  }
  if (d->character && char_get_admlevel(d->character) >= char_get_admlevel(ch)) {
    if (!char_can_see(g, ch, d->character))
      send_to_char(g, ch, "No such connection.\r\n");
    else
      send_to_char(g, ch, "Umm.. maybe that's not such a good idea...\r\n");
    return;
  }

  /* We used to just close the socket here using close(), but
   * various people pointed out this could cause a crash if you're
   * closing the person below you on the descriptor list.  Just setting
   * to CON_CLOSE leaves things in a massively inconsistent state so I
   * had to add this new flag to the descriptor. -je
   *
   * It is a much more logical extension for a CON_DISCONNECT to be used
   * for in-game socket closes and CON_CLOSE for out of game closings.
   * This will retain the stability of the close_me hack while being
   * neater in appearance. -gg 12/1/97
   *
   * For those unlucky souls who actually manage to get disconnected
   * by two different immortals in the same 1/10th of a second, we have
   * the below 'if' check. -gg 12/17/99
   */
  if (desc_get_connected(d) == CON_DISCONNECT || desc_get_connected(d) == CON_CLOSE)
    send_to_char(g, ch, "They're already being disconnected.\r\n");
  else {
    /*
     * Remember that we can disconnect people not in the game and
     * that rather confuses the code when it expected there to be
     * a character context.
     */
    if (desc_get_connected(d) == CON_PLAYING)
      desc_set_connected(d, CON_DISCONNECT);
    else
      desc_set_connected(d, CON_CLOSE);

    send_to_char(g, ch, "Connection #{:d} closed.\r\n", num_to_dc);
    mud_log("(GC) Connection closed by {}.", char_get_name(ch));
  }
}

ACMD(do_wizlock)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    if (g->circle_restrict == 0)
      send_to_char(g, ch, "The game is currently completely open.\r\n");
    else if (g->circle_restrict == 1)
      send_to_char(g, ch, "The game is currently closed to new players.\r\n");
    else
      send_to_char(g, ch, "Only level {:d} and above may enter the game currently.\r\n", g->circle_restrict);
    return;
  }

  int value = parse_int(arg);
  if (value < 0 || value > 101) {
    send_to_char(g, ch, "Invalid wizlock value.\r\n");
    return;
  }
  g->circle_restrict = value;
  std::string_view name = char_get_name(ch);
  switch (value) {
    case 0:
      send_to_char(g, ch, "The game is now completely open.\r\n");
      send_to_all(g, "@RWIZLOCK@D: @WThe game has been completely opened by @C{}@W.@n", name);
      mud_log("WIZLOCK: The game has been completely opened by {}.", name);
      break;
    case 1:
      send_to_char(g, ch, "The game is now closed to new players.\r\n");
      send_to_all(g, "@RWIZLOCK@D: @WThe game is now closed to new players by @C{}@W.@n", name);
      mud_log("WIZLOCK: The game is now closed to new players by {}.", name);
      break;
    case 101:
      send_to_char(g, ch, "The game is now closed to non-imms.\r\n");
      send_to_all(g, "@RWIZLOCK@D: @WThe game is now closed to non-imms by @C{}@W.@n", name);
      mud_log("WIZLOCK: The game is now closed to non-imms by {}.", name);
      break;
    default:
      send_to_char(g, ch, "Only level {:d}+ may enter the game now.\r\n", value);
      send_to_all(g, "@RWIZLOCK@D: @WLevel {:d}+ only can enter the game now, thanks to @C{}@W.@n", value, name);
      mud_log("WIZLOCK: Level {:d}+ only can enter the game now, thanks to {}.", value, name);
      break;
  }
}

ACMD(do_saveall)
{
  if (char_get_admlevel(ch) < ADMLVL_BUILDER)
    send_to_char(g, ch, "You are not holy enough to use this privelege.\n\r");
  else {
    save_all(g);
    House_save_all(g);
    send_to_char(g, ch, "World and house files saved.\n\r");
  }
}

ACMD(do_wizupdate)
{
  run_autowiz(g);
  send_to_char(g, ch, "Wizlists updated.\n\r");
}
