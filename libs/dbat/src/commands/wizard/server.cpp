/* ************************************************************************
*   File: wizard/server.c                                  Part of CircleMUD *
*  Usage: God commands that control the server                            *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <climits>
#include <cerrno>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <sys/time.h>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/players.hpp"
#include "dbat/utils.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/db.hpp"
#include "dbat/objsave.hpp"
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
#include "dbat/feats.hpp"
#include "dbat/fight.hpp"
#include "dbat/genolc.hpp"
#include "dbat/screen.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/shop.hpp"
#include "dbat/guild.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static void execute_copyover(GameWorld *g);
static void game_info(GameWorld *g, const char *format, ...) __attribute__((format(printf, 2, 3)));

/* local variables */

// definitions
ACMD(do_lag)
{

  std::string arg, arg2;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: lag (target) (number of seconds)\r\n");
    return;
  }

  int found = false, num = parse_int(arg2);

  if (num <= 0 || num > 5) {
    send_to_char(g, ch, "Keep it between 1 to 5 seconds please.\r\n");
    return;
  }

  for (descriptor_data *d : each_desc(g)) {
    if (!d->character || !char_get_name(d->character)) { /* still at the login menus */
      continue;
    }
    if (str_eq_nocase(char_get_name(d->character), arg)) {
      if (char_get_admlevel(d->character) > char_get_admlevel(ch)) {
        send_to_char(g, ch, "Sorry, you've been outranked.\r\n");
        continue;
      }
      switch (num) {
        case 1:
          char_set_wait(d->character, PULSE_1SEC);
          break;
        case 2:
          char_set_wait(d->character, PULSE_2SEC);
          break;
        case 3:
          char_set_wait(d->character, PULSE_3SEC);
          break;
        case 4:
          char_set_wait(d->character, PULSE_4SEC);
          break;
        case 5:
          char_set_wait(d->character, PULSE_5SEC);
          break;
      }
      found = true;
    }
  }

  if (found == false) {
    send_to_char(g, ch, "That player isn't around.\r\n");
    return;
  }
}

ACMD(do_shutdown)
{
  std::string arg;

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char(g, ch, "If you want to shut something down, say so!\r\n");
    return;
  }
  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty()) {
    mud_log("(GC) Shutdown by %s.", char_get_name(ch));
    send_to_all(g, "Shutting down.\r\n");
    g->circle_shutdown = 1;
  } else if (str_eq_nocase(arg, "reboot")) {
    mud_log("(GC) Reboot by %s.", char_get_name(ch));
    send_to_all(g, "Rebooting.. come back in a minute or two.\r\n");
    touch(FASTBOOT_FILE);
    g->circle_shutdown = g->circle_reboot = 1;
  } else if (str_eq_nocase(arg, "die")) {
    mud_log("(GC) Shutdown by %s.", char_get_name(ch));
    send_to_all(g, "Shutting down for maintenance.\r\n");
    touch(KILLSCRIPT_FILE);
    g->circle_shutdown = 1;
  } else if (str_eq_nocase(arg, "now")) {
    mud_log("(GC) Shutdown NOW by %s.", char_get_name(ch));
    send_to_all(g, "Rebooting.. come back in a minute or two.\r\n");
    g->circle_shutdown = 1;
    g->circle_reboot   = 2; /* do not autosave olc */
  } else if (str_eq_nocase(arg, "pause")) {
    mud_log("(GC) Shutdown by %s.", char_get_name(ch));
    send_to_all(g, "Shutting down for maintenance.\r\n");
    touch(PAUSE_FILE);
    g->circle_shutdown = 1;
  } else
    send_to_char(g, ch, "Unknown shutdown option.\r\n");
}

/* Path of the binary to re-exec on copyover: whatever file is now at the path this process was started
 * from (so a freshly rebuilt binary is picked up, whichever build system produced it). After a rebuild
 * replaces the file, the kernel reports the old path with a " (deleted)" suffix, which is stripped. */
static bool copyover_exe_path(char *out, size_t size)
{
  ssize_t len = readlink("/proc/self/exe", out, size - 1);
  if (len <= 0)
    return false;
  out[len]           = '\0';
  /* what a binary rebuilt while running looks like: the new one is what we want */
  const char *suffix = " (deleted)";
  size_t slen        = strlen(suffix);
  if ((size_t)len > slen && !strcmp(out + len - slen, suffix))
    out[len - slen] = '\0';
  return true;
}

static void game_info(GameWorld *g, const char *format, ...)
{
  va_list args;
  char messg[MAX_STRING_LENGTH];

  if (format == nullptr)
    return;

  snprintf(messg, sizeof(messg), "@r-@R=@D<@GCOPYOVER@D>@R=@r- @W");

  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) != CON_PLAYING && (desc_get_connected(i) != CON_REDIT && desc_get_connected(i) != CON_OEDIT && desc_get_connected(i) != CON_MEDIT))
      continue;
    if (!(i->character))
      continue;

    write_to_output(g, i, "%s", messg);
    va_start(args, format);
    vwrite_to_output(g, i, format, args);
    va_end(args);
    write_to_output(g, i, "@n\r\n@R>>>@GMake sure to pick up your bed items and save.@n\r\n");
  }
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
    send_to_char(g, ch, "Usage: @ycopyover@n           - Perform an immediate copyover\r\n"
                     "       @ycopyover <seconds>@n - Start a timed copyover\r\n"
                     "       @ycopyover cancel@n    - Stop a timed copyover\r\n\r\n");
    send_to_char(g, ch, "A timed copyover will produce an automatic warning when it starts, and then\r\n");
    send_to_char(g, ch, "every minute.  During the last minute, there will be more frequent warnings.\r\n");
  } else {
    secs = parse_int(arg);
    if (!secs || secs < 0) {
      send_to_char(g, ch, "Type @ycopyover help@n for usage info.");
    } else {
      g->copyover_timer = secs;
      mud_log("-- Timed Copyover started by %s - %d seconds until copyover --", char_get_name(ch), secs);
      if (secs >= 60) {
        if (secs % 60) {
          game_info(g, "A copyover will be performed in %d minutes and %d seconds.", (g->copyover_timer / 60), (g->copyover_timer % 60));
        } else {
          game_info(g, "A copyover will be performed in %d minute%s.", (g->copyover_timer / 60), (g->copyover_timer / 60) > 1 ? "s" : "");
        }
      } else {
        game_info(g, "A copyover will be performed in %d seconds.", g->copyover_timer);
      }
    }
  }
}

/* Whoever a connection's player really is: someone switched into a mob is still themselves. */
static struct char_data *copyover_player(const struct descriptor_data *d)
{
  struct char_data *player = d->original ? d->original : d->character;

  return player && !char_is_npc(player) && desc_is_playing(d) ? player : nullptr;
}

/* Who to bring back, and how. Returns false, having changed nothing, if it can't all be written down. */
static bool write_copyover_file(GameWorld *g)
{
  struct atomic_file af;
  FILE *fp = atomic_open(&af, COPYOVER_FILE, false);

  if (!fp)
    return false;

  for (descriptor_data *d : each_desc(g)) {
    struct char_data *player = copyover_player(d);
    room_vnum room           = 300;

    if (!player)
      continue;
    if (char_get_in_room(player) > 1)
      room = char_get_in_room(player);
    else if (char_get_was_in_room(player) > 1)
      room = char_get_was_in_room(player);
    fprintf(fp, "%d %s %s %d %s\n", d->descriptor, char_get_name(player), *d->host ? d->host : "unknown", room,
            !d->user.empty() ? d->user.c_str() : "Empty");
  }

  /* After the end marker, so that an older server ignores it. What else outlives the process goes here. */
  fprintf(fp, "-1 restrict %d\n", g->circle_restrict);
  return atomic_commit(&af);
}

/* Saves everyone in the game who has no connection to bring back: link-dead players. */
static void save_players_without_connections(GameWorld *g)
{
  for (char_data *ch : each_char(g)) {
    bool connected = char_get_desc(ch) != nullptr;

    for (descriptor_data *d : each_desc(g)) {
      if (connected)
        break;
      connected = d->original == ch;
    }
    if (!char_is_npc(ch) && !connected) {
      Crash_crashsave(g, ch);
      save_char(g, ch);
    }
  }
}

/*
 * Everything that can fail comes first, while nothing has changed and the game can simply carry on:
 * Crash_rentsave() takes each player's objects out of the game, so after that there's no way back.
 */
static void execute_copyover(GameWorld *g)
{
  struct itimerval no_timer = {{0, 0}, {0, 0}};
  char exe[PATH_MAX], fold[120], mother[32];
  char **args;
  int num_args = 0;

  if (!copyover_exe_path(exe, sizeof(exe)) || access(exe, X_OK) < 0) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: Copyover aborted: can't run the server binary: %s", strerror(errno));
    return;
  }
  if (!write_copyover_file(g)) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: Copyover aborted: can't write %s: %s", COPYOVER_FILE, strerror(errno));
    return;
  }

  /* Consider changing all saved areas here, if you use OLC */
  save_all(g);
  save_mud_time(g, &g->time_info);
  House_save_all(g);
  save_players_without_connections(g);

  snprintf(fold, sizeof(fold), "\t\x1B[1;31m \007\007\007The universe stops for a moment as space and time fold.\x1B[0;0m\r\n");
  for (descriptor_data *d : each_desc(g)) {
    struct char_data *player = copyover_player(d);

    if (!player) {
      write_to_descriptor(d->descriptor, "\n\rSorry, we are rebooting. Come back in a few seconds.\n\r");
      close_socket(g, d); /* throw'em out */
      continue;
    }
    if (d->output && *d->output)
      write_to_descriptor(d->descriptor, d->output); /* whatever they haven't been sent yet */
    if (desc_get_connected(d) != CON_PLAYING)
      write_to_descriptor(d->descriptor, "\n\rWhatever you were editing has been lost.\n\r");
    Crash_rentsave(g, player, 0);
    save_char(g, player);
    write_to_descriptor(d->descriptor, fold);
  }

  /* Everything the game was started with, so that it comes back the same, and in the same place. */
  args = new char *[g->launch_argc + 2]();
  snprintf(mother, sizeof(mother), "-C%d", g->mother_desc);
  args[num_args++] = g->launch_argv[0];
  args[num_args++] = mother;
  for (int i = 1; i < g->launch_argc; i++)
    if (strncmp(g->launch_argv[i], "-C", 2))
      args[num_args++] = g->launch_argv[i];
  args[num_args] = nullptr;
  if (chdir(g->launch_dir) < 0)
    perror("do_copyover: chdir");

  /* The deadlock timer outlives exec, and would kill the new process if it ran out during boot. */
  setitimer(ITIMER_VIRTUAL, &no_timer, nullptr);

  /* exec - descriptors are inherited, but anything still buffered here would be lost */
  fflush(nullptr);
  execv(exe, args);
  /* Failed - sucessful exec will not return */

  perror("do_copyover: execv");
  mud_log("SYSERR: Copyover failed after saving everyone. Rebooting.");
  exit(52); /* everything is saved, but objects have left the game: what autorun takes to mean reboot */
}

void copyover_check(GameWorld *g)
{
  if (!g->copyover_timer) return;
  g->copyover_timer--;
  if (!g->copyover_timer) {
    execute_copyover(g);
    return; /* only if it was aborted */
  }
  if (g->copyover_timer > 59) {
    if (g->copyover_timer % 60 == 0) {
      game_info(g, "A copyover will be performed in %d minute%s.", (g->copyover_timer / 60), (g->copyover_timer / 60) > 1 ? "s" : "");
    }
  } else {
    if (g->copyover_timer % 10 == 0 && g->copyover_timer > 29) {
      game_info(g, "A copyover will be performed in %d seconds.", (g->copyover_timer));
    }
    if (g->copyover_timer % 5 == 0 && g->copyover_timer <= 29) {
      game_info(g, "A copyover will be performed in %d seconds.", (g->copyover_timer));
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

    send_to_char(g, ch, "Connection #%d closed.\r\n", num_to_dc);
    mud_log("(GC) Connection closed by %s.", char_get_name(ch));
  }
}

ACMD(do_wizlock)
{
  std::string arg;
  int value;
  const char *when;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  if (!arg.empty()) {
    value = parse_int(arg);
    if (value < 0 || value > 101) {
      send_to_char(g, ch, "Invalid wizlock value.\r\n");
      return;
    }
    g->circle_restrict = value;
    when            = "now";
  } else
    when = "currently";

  if (!arg.empty()) {
    switch (g->circle_restrict) {
      case 0:
        send_to_char(g, ch, "The game is %s completely open.\r\n", when);
        send_to_all(g, "@RWIZLOCK@D: @WThe game has been completely opened by @C%s@W.@n", char_get_name(ch));
        mud_log("WIZLOCK: The game has been completely opened by %s.", char_get_name(ch));
        break;
      case 1:
        send_to_char(g, ch, "The game is %s closed to new players.\r\n", when);
        send_to_all(g, "@RWIZLOCK@D: @WThe game is %s closed to new players by @C%s@W.@n", when, char_get_name(ch));
        mud_log("WIZLOCK: The game is %s closed to new players by %s.", when, char_get_name(ch));
        break;
      case 101:
        send_to_char(g, ch, "The game is %s closed to non-imms.\r\n", when);
        send_to_all(g, "@RWIZLOCK@D: @WThe game is %s closed to non-imms by @C%s@W.@n", when, char_get_name(ch));
        mud_log("WIZLOCK: The game is %s closed to non-imms by %s.", when, char_get_name(ch));
        break;
      default:
        send_to_char(g, ch, "Only level %d+ may enter the game %s.\r\n", g->circle_restrict, when);
        send_to_all(g, "@RWIZLOCK@D: @WLevel %d+ only can enter the game %s, thanks to @C%s@W.@n", g->circle_restrict, when, char_get_name(ch));
        mud_log("WIZLOCK: Level %d+ only can enter the game %s, thanks to %s.", g->circle_restrict, when, char_get_name(ch));
        break;
    }
  }
  if (arg.empty()) {
    switch (g->circle_restrict) {
      case 0:
        send_to_char(g, ch, "The game is %s completely open.\r\n", when);
        break;
      case 1:
        send_to_char(g, ch, "The game is %s closed to new players.\r\n", when);
        break;
      default:
        send_to_char(g, ch, "Only level %d and above may enter the game %s.\r\n", g->circle_restrict, when);
        break;
    }
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
