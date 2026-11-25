/*************************************************************************
*   File: copyover.cpp                                  Part of CircleMUD *
*  Usage: Copyover: the exec, and bringing the players back after it     *
*         (formerly part of comm.c and wizard/server.c)                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <cerrno>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <expected>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <system_error>
#include <vector>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/objsave.hpp"
#include "dbat/genolc.hpp"

#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/copyover.hpp"
#include "comm_internal.hpp"

/*
 * The sockets a copyover brings across that nobody claimed, because of a damaged file for instance.
 * Their players would otherwise wait forever on a game that doesn't know they're there.
 */
static void close_orphaned_sockets(GameWorld *g)
{
  int closed = 0;

  for (net::native_handle h : net::process::open_sockets()) {
    bool claimed = h == g->listener.handle();

    for (descriptor_data *d : each_desc(g)) {
      if (claimed)
        break;
      claimed = d->sock.handle() == h;
    }
    if (!claimed) {
      write_to_descriptor(h, "\n\rSomething went wrong during the folding. Please log back in.\n\r");
      net::socket(h).close();
      closed++;
    }
  }
  if (closed)
    mud_log("SYSERR: copyover_recover: closed {:d} connections that weren't in the copyover file.", closed);
}

/* One player to bring back, as a line of the copyover file has them */
struct folded_player {
  int socket;
  std::string name, host;
  room_vnum loadroom;
  std::string user;
};

/* Brings one player back from a line of the copyover file. */
static void copyover_recover_player(GameWorld *g, const folded_player &p)
{
  struct descriptor_data *d;
  room_vnum set_loadroom;
  int player_i;
  bool fOld = true;
  net::native_handle desc = p.socket;

  if (!net::process::is_socket(desc)) {
    mud_log("SYSERR: copyover_recover: descriptor {:d} for {} isn't a connection.", p.socket, p.name);
    return;
  }
  net::socket sock(desc);

  /* Write something, and check if it goes error-free */
  if (write_to_descriptor(desc, "\n\rFolding initiated...\n\r") < 0)
    return; /* nope: closed */

  /* create a new descriptor */
  d = new descriptor_data();
  init_descriptor(g, d, std::move(sock)); /* set up various stuff */
  set_empty_account(d);                   /* what a new connection has, for userLoad() to fill in */

  d->host = std::string_view(p.host).substr(0, HOST_LENGTH);
  g->descriptors.add(d->desc_num, d);

  d->connected = CON_CLOSE;

  /* Now, find the pfile */

  d->character = new char_data();
  clear_char(d->character);
  char_make_player_specials(d->character);
  char_set_desc(d->character, d);

  if ((player_i = load_char(g, p.name, d->character)) >= 0 && !char_plr_flagged(d->character, player_flag::deleted)) {
    char_set_pfilepos(d->character, player_i);
    char_plr_set(d->character, player_flag::writing, false);
    char_plr_set(d->character, player_flag::mailing, false);
    char_plr_set(d->character, player_flag::cryo, false);
    userLoad(d, p.user.c_str());
  } else
    fOld = false;

  if (!fOld) /* Player file not found?! */ {
    write_to_descriptor(desc, "\n\rSomehow, your character was lost during the folding. Sorry.\n\r");
    close_socket(g, d);
  } else {
    write_to_descriptor(desc, "\n\rFolding complete.\n\r");
    set_loadroom = char_get_load_room(g, d->character);
    char_set_load_room(g, d->character, p.loadroom);
    enter_player_game(g, d);
    char_set_load_room(g, d->character, set_loadroom);
    d->connected = CON_PLAYING;
    look_at_room(g, char_get_in_room(d->character), d->character, 0);
    if (char_aff_flagged(d->character, aff_flag::hayasa)) {
      char_set_speedboost(d->character, char_get_speedcalc(g, d->character) * 0.5);
    }
  }
}

/* The restriction after the end marker's -1 (" restrict N"), if it has one, as sscanf("%*d restrict %d") reads it */
static std::optional<int> marked_restriction(std::string_view rest)
{
  int level;

  rest.remove_prefix(std::min(rest.find_first_not_of(" \t\n\v\f\r"), rest.size()));
  if (rest.starts_with("restrict") && io::fields(rest.substr(8)).number(level))
    return level;
  return std::nullopt;
}

/* Reload players after a copyover */
void copyover_recover(GameWorld *g)
{
  mud_log("Copyover recovery initiated");
  g->PCOUNTDAY = game_now(g) + 60;

  io::loaded<io::text_reader> file = io::text_reader::open(COPYOVER_FILE);
  if (!file) {
    /* Carry on: anyone already connected gets told below, and everyone else can still log in. */
    mud_log("SYSERR: copyover_recover: can't open {}: {}", COPYOVER_FILE, std::strerror(errno));
    close_orphaned_sockets(g);
    return;
  }

  std::error_code ignored;
  std::filesystem::remove(COPYOVER_FILE, ignored); /* In case it crashes - doesn't prevent reading */
  /* Lines as fgets() read them into 512 bytes, which a longer one would have split (quirk, gone: the file never has
     one) */
  while (std::optional<std::string_view> line = file->raw_line()) {
    io::fields marker(*line);
    int desc;
    if (marker.number(desc) && desc == -1) { /* the end marker */
      if (std::optional<int> level = marked_restriction(marker.rest()))
        g->circle_restrict = *level;
      break;
    }
    folded_player p;
    io::fields f(*line);
    f.number(p.socket) && f.word(p.name, 127) && f.word(p.host, 127) && f.number(p.loadroom) && f.word(p.user, 99);
    if (f.count() != 5) {
      mud_log("SYSERR: copyover_recover: skipping a malformed line in {}.", COPYOVER_FILE);
      continue;
    }
    copyover_recover_player(g, p);
  }
  close_orphaned_sockets(g);
}

void game_info(GameWorld *g, std::string_view text)
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

/* Whoever a connection's player really is: someone switched into a mob is still themselves. */
static struct char_data *copyover_player(const struct descriptor_data *d)
{
  struct char_data *player = d->original ? d->original : d->character;

  return player && !char_is_npc(player) && desc_is_playing(d) ? player : nullptr;
}

/* Who to bring back, and how. Returns false, having changed nothing, if it can't all be written down. */
static bool write_copyover_file(GameWorld *g)
{
  io::text_writer out(COPYOVER_FILE);

  for (descriptor_data *d : each_desc(g)) {
    struct char_data *player = copyover_player(d);
    room_vnum room           = 300;

    if (!player)
      continue;
    if (char_get_in_room(player) > 1)
      room = char_get_in_room(player);
    else if (char_get_was_in_room(player) > 1)
      room = char_get_was_in_room(player);
    out.print("{:d} {} {} {:d} {}\n", d->sock.handle(), char_get_name(player), !d->host.empty() ? d->host.c_str() : "unknown", room,
               !d->acct.user.empty() ? d->acct.user.c_str() : "Empty");
  }

  /* After the end marker, so that an older server ignores it. What else outlives the process goes here. */
  out.print("-1 restrict {:d}\n", g->circle_restrict);
  return out.commit();
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
void execute_copyover(GameWorld *g)
{
  std::expected<std::string, std::string> exe = net::process::executable_path();
  if (!exe) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: Copyover aborted: can't run the server binary: {}", exe.error());
    return;
  }
  if (!write_copyover_file(g)) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: Copyover aborted: can't write {}: {}", COPYOVER_FILE, std::strerror(errno));
    return;
  }

  /* Consider changing all saved areas here, if you use OLC */
  save_all(g);
  save_mud_time(g, &g->time_info);
  House_save_all(g);
  save_players_without_connections(g);

  constexpr std::string_view fold = "\t\x1B[1;31m \007\007\007The universe stops for a moment as space and time fold.\x1B[0;0m\r\n";
  for (descriptor_data *d : each_desc(g)) {
    struct char_data *player = copyover_player(d);

    if (!player) {
      write_to_descriptor(d->sock.handle(), "\n\rSorry, we are rebooting. Come back in a few seconds.\n\r");
      close_socket(g, d); /* throw'em out */
      continue;
    }
    if (!d->out.text.empty())
      write_to_descriptor(d->sock.handle(), d->out.text.c_str()); /* whatever they haven't been sent yet (to a '\0') */
    if (desc_get_connected(d) != CON_PLAYING)
      write_to_descriptor(d->sock.handle(), "\n\rWhatever you were editing has been lost.\n\r");
    Crash_rentsave(g, player, 0);
    save_char(g, player);
    write_to_descriptor(d->sock.handle(), fold);
  }

  /* Everything the game was started with, so that it comes back the same, and in the same place. */
  std::vector<std::string> args{g->launch_args[0], str_format("-C{:d}", g->listener.handle())};
  for (const std::string &arg : g->launch_args | std::views::drop(1))
    if (!arg.starts_with("-C"))
      args.push_back(arg);
  std::error_code error;
  std::filesystem::current_path(g->launch_dir, error);
  if (error)
    file_print(stderr, "do_copyover: chdir: {}\n", error.message());

  /* The deadlock timer outlives exec, and would kill the new process if it ran out during boot. */
  net::process::stop_deadlock_timer();

  /* exec - the sockets are inherited */
  net::process::replace_process(*exe, args);
  /* Failed - sucessful exec will not return */

  std::perror("do_copyover: execv");
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
