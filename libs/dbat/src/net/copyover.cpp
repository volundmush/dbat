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
#include <cstring>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/objsave.hpp"
#include "dbat/genolc.hpp"

#include "dbat/io/writer.hpp"
#include "dbat/copyover.hpp"
#include "comm_internal.hpp"

/*
 * The sockets a copyover brings across that nobody claimed, because of a damaged file for instance.
 * Their players would otherwise wait forever on a game that doesn't know they're there.
 */
static void close_orphaned_sockets(GameWorld *g)
{
  struct rlimit limit;
  int closed = 0, highest = 1024;

  if (getrlimit(RLIMIT_NOFILE, &limit) == 0 && limit.rlim_cur != RLIM_INFINITY)
    highest = limit.rlim_cur;
  for (int fd = STDERR_FILENO + 1; fd < highest; fd++) {
    struct stat info;
    bool claimed = fd == g->listener.handle();

    if (fstat(fd, &info) < 0 || !S_ISSOCK(info.st_mode))
      continue;
    for (descriptor_data *d : each_desc(g)) {
      if (claimed)
        break;
      claimed = d->sock.handle() == fd;
    }
    if (!claimed) {
      write_to_descriptor(fd, "\n\rSomething went wrong during the folding. Please log back in.\n\r");
      close(fd);
      closed++;
    }
  }
  if (closed)
    mud_log("SYSERR: copyover_recover: closed {:d} connections that weren't in the copyover file.", closed);
}

/* Brings one player back from a line of the copyover file. */
static void copyover_recover_player(GameWorld *g, int desc, char *name, const char *host, room_vnum saved_loadroom, char *username)
{
  struct descriptor_data *d;
  struct stat info;
  room_vnum set_loadroom;
  int player_i;
  bool fOld = true;

  if (fstat(desc, &info) < 0 || !S_ISSOCK(info.st_mode)) {
    mud_log("SYSERR: copyover_recover: descriptor {:d} for {} isn't a connection.", desc, name);
    return;
  }

  /* Write something, and check if it goes error-free */
  if (write_to_descriptor(desc, "\n\rFolding initiated...\n\r") < 0) {
    close(desc); /* nope */
    return;
  }

  /* create a new descriptor */
  d = new descriptor_data();
  init_descriptor(g, d, net::socket(desc)); /* set up various stuff */
  set_empty_account(d);        /* what a new connection has, for userLoad() to fill in */

  d->host = std::string_view(host).substr(0, HOST_LENGTH);
  g->descriptors.add(d->desc_num, d);

  d->connected = CON_CLOSE;

  /* Now, find the pfile */

  d->character = new char_data();
  clear_char(d->character);
  char_make_player_specials(d->character);
  char_set_desc(d->character, d);

  if ((player_i = load_char(g, name, d->character)) >= 0 && !char_plr_flagged(d->character, PLR_DELETED)) {
    char_set_pfilepos(d->character, player_i);
    char_plr_set(d->character, PLR_WRITING, false);
    char_plr_set(d->character, PLR_MAILING, false);
    char_plr_set(d->character, PLR_CRYO, false);
    userLoad(d, username);
  } else
    fOld = false;

  if (!fOld) /* Player file not found?! */ {
    write_to_descriptor(desc, "\n\rSomehow, your character was lost during the folding. Sorry.\n\r");
    close_socket(g, d);
  } else {
    write_to_descriptor(desc, "\n\rFolding complete.\n\r");
    set_loadroom = char_get_load_room(g, d->character);
    char_set_load_room(g, d->character, saved_loadroom);
    enter_player_game(g, d);
    char_set_load_room(g, d->character, set_loadroom);
    d->connected = CON_PLAYING;
    look_at_room(g, char_get_in_room(d->character), d->character, 0);
    if (char_aff_flagged(d->character, AFF_HAYASA)) {
      char_set_speedboost(d->character, char_get_speedcalc(g, d->character) * 0.5);
    }
  }
}

/* Reload players after a copyover */
void copyover_recover(GameWorld *g)
{
  FILE *fp;
  char line[512], name[128], host[128], username[100];
  int desc, restrict_level;
  room_vnum saved_loadroom;

  mud_log("Copyover recovery initiated");
  g->PCOUNTDAY = game_now(g) + 60;
  fp        = fopen(COPYOVER_FILE, "r");

  if (!fp) {
    /* Carry on: anyone already connected gets told below, and everyone else can still log in. */
    mud_log("SYSERR: copyover_recover: can't open {}: {}", COPYOVER_FILE, strerror(errno));
    close_orphaned_sockets(g);
    return;
  }

  unlink(COPYOVER_FILE); /* In case it crashes - doesn't prevent reading */
  while (fgets(line, sizeof(line), fp)) {
    if (sscanf(line, "%d", &desc) == 1 && desc == -1) { /* the end marker */
      if (sscanf(line, "%*d restrict %d", &restrict_level) == 1)
        g->circle_restrict = restrict_level;
      break;
    }
    if (sscanf(line, "%d %127s %127s %d %99s", &desc, name, host, &saved_loadroom, username) != 5) {
      mud_log("SYSERR: copyover_recover: skipping a malformed line in {}.", COPYOVER_FILE);
      continue;
    }
    copyover_recover_player(g, desc, name, host, saved_loadroom, username);
  }
  fclose(fp);
  close_orphaned_sockets(g);
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
      write_to_descriptor(d->sock.handle(), "\n\rSorry, we are rebooting. Come back in a few seconds.\n\r");
      close_socket(g, d); /* throw'em out */
      continue;
    }
    if (!d->out.text.empty())
      write_to_descriptor(d->sock.handle(), d->out.text.c_str()); /* whatever they haven't been sent yet */
    if (desc_get_connected(d) != CON_PLAYING)
      write_to_descriptor(d->sock.handle(), "\n\rWhatever you were editing has been lost.\n\r");
    Crash_rentsave(g, player, 0);
    save_char(g, player);
    write_to_descriptor(d->sock.handle(), fold.c_str());
  }

  /* Everything the game was started with, so that it comes back the same, and in the same place. */
  args = new char *[g->launch_argc + 2]();
  mother           = str_format("-C{:d}", g->listener.handle());
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
