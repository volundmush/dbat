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
/* Tells everyone playing (or building) about the copyover */
static void game_info(GameWorld *g, std::string_view text);
template <class... Args>
  requires(sizeof...(Args) > 0)
static void game_info(GameWorld *g, std::format_string<Args...> fmt, Args &&...args)
{
  game_info(g, str_format(fmt, std::forward<Args>(args)...));
}

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
    if (!d->character || !char_get_name(d->character) || !str_eq_nocase(char_get_name(d->character), arg)) /* or still at the login menus */
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
    touch(mode->touched);
  g->circle_shutdown = 1;
  if (mode->reboot)
    g->circle_reboot = mode->reboot;
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

static void game_info(GameWorld *g, std::string_view text)
{
  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) != CON_PLAYING && (desc_get_connected(i) != CON_REDIT && desc_get_connected(i) != CON_OEDIT && desc_get_connected(i) != CON_MEDIT))
      continue;
    if (!(i->character))
      continue;

    write_to_output(g, i, "@r-@R=@D<@GCOPYOVER@D>@R=@r- @W");
    write_to_output(g, i, text);
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
    file_print(fp, "{:d} {} {} {:d} {}\n", d->descriptor, char_get_name(player), *d->host ? d->host : "unknown", room,
               !d->user.empty() ? d->user.c_str() : "Empty");
  }

  /* After the end marker, so that an older server ignores it. What else outlives the process goes here. */
  file_print(fp, "-1 restrict {:d}\n", g->circle_restrict);
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
  char exe[PATH_MAX];
  std::string fold;
  std::string mother; /* lasts until the exec */
  char **args;
  int num_args = 0;

  if (!copyover_exe_path(exe, sizeof(exe)) || access(exe, X_OK) < 0) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: Copyover aborted: can't run the server binary: {}", strerror(errno));
    return;
  }
  if (!write_copyover_file(g)) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: Copyover aborted: can't write {}: {}", COPYOVER_FILE, strerror(errno));
    return;
  }

  /* Consider changing all saved areas here, if you use OLC */
  save_all(g);
  save_mud_time(g, &g->time_info);
  House_save_all(g);
  save_players_without_connections(g);

  fold = "\t\x1B[1;31m \007\007\007The universe stops for a moment as space and time fold.\x1B[0;0m\r\n";
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
    write_to_descriptor(d->descriptor, fold.c_str());
  }

  /* Everything the game was started with, so that it comes back the same, and in the same place. */
  args = new char *[g->launch_argc + 2]();
  mother           = str_format("-C{:d}", g->mother_desc);
  args[num_args++] = g->launch_argv[0];
  args[num_args++] = mother.data();
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
  if (!g->copyover_timer)
    return;
  g->copyover_timer--;
  int left = g->copyover_timer;
  if (!left) {
    execute_copyover(g);
    return; /* only if it was aborted */
  }
  /* Each minute, then every ten seconds of the last minute, then every five of the last thirty */
  if (left > 59) {
    if (left % 60 == 0)
      game_info(g, "A copyover will be performed in {:d} minute{}.", left / 60, left / 60 > 1 ? "s" : "");
  } else if (left % (left > 29 ? 10 : 5) == 0) {
    game_info(g, "A copyover will be performed in {:d} seconds.", left);
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
  const char *name   = char_get_name(ch);
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
