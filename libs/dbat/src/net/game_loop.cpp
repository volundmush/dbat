/*************************************************************************
*   File: game_loop.cpp                                 Part of CircleMUD *
*  Usage: The main game loop and the heartbeat                           *
*         (formerly part of comm.c)                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>

#include "dbat/iterate.hpp"
#include "dbat/arrays.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/act.other.hpp"
#include "dbat/act.item.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_event.hpp"
#include "dbat/mobact.hpp"
#include "dbat/magic.hpp"
#include "dbat/object_systems.hpp"
#include "dbat/fileop.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/objsave.hpp"
#include "dbat/genolc.hpp"
#include "dbat/combat.hpp"
#include "dbat/fight.hpp"
#include "dbat/local_limits.hpp"

#include "comm_internal.hpp"

static void game_loop(GameWorld *g);
static void record_usage(GameWorld *g);
constexpr int64_t NS_PER_SEC = 1000000000;
constexpr int64_t PULSE_NS   = OPT_USEC * 1000LL; /* one pass of the game loop */

static int64_t monotonic_ns(void);
static void sleep_until(int64_t when);

/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/

/* Init sockets, run game, and cleanup sockets */
void init_game(GameWorld *g, uint16_t cmport)
{
  /* We don't want to restart if we crash before we get up. */
  io::touch(KILLSCRIPT_FILE);

  circle_srandom(g, game_now(g));

  net::startup();

  mud_log("Finding player limit.");
  g->max_players = get_max_players(g);

  if (!g->fCopyOver) { /* After a copyover, the listener came across with the -C option */
    mud_log("Opening mother connection.");
    g->listener = net::listen_on(CONFIG_DFLT_IP, cmport);
  }

  event_init(g);

  boot_db(g);

  FILE *mapfile;
  int rowcounter, colcounter;
  room_vnum vnum_read;

  mud_log("Signal trapping.");
  signal_setup();

  mud_log("Loading Space Map. ");

  //Load the map vnums from a file into an array
  if (!(mapfile = fopen("../data/surface.map", "r"))) {
    mud_log("SYSERR: Unable to open ../data/surface.map: {}", strerror(errno));
    exit(1);
  }

  for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++) {
    for (colcounter = 0; colcounter <= MAP_COLS; colcounter++) {
      g->mapnums[rowcounter][colcounter] = fscanf(mapfile, "%d", &vnum_read) == 1 && room_of(g, vnum_read) ? vnum_read : NOWHERE;
    }
  }

  fclose(mapfile);

  /* Load the toplist */
  topLoad(g);

  /* If we made it this far, we will be able to restart without problem. */
  remove(KILLSCRIPT_FILE);

  if (g->fCopyOver) /* reload players */
    copyover_recover(g);

  mud_log("Entering game loop.");

  game_loop(g);

  Crash_save_all(g);

  mud_log("Closing all sockets.");
  close_all_sockets(g);

  g->listener.close();

  if (g->circle_reboot != 2)
    save_all(g);

  mud_log("Saving current MUD time.");
  save_mud_time(g, &g->time_info);

  if (g->circle_reboot) {
    mud_log("Rebooting.");
    exit(52); /* what's so great about HHGTTG, anyhow? */
  }
  mud_log("Normal termination of game.");
}

/* Runs one queued line of input for each descriptor that is ready for it. */
int game_read_input(GameWorld *g, struct descriptor_data *d)
{
  return process_input(g, d);
}

int game_send_output(GameWorld *g, struct descriptor_data *d)
{
  return process_output(g, d);
}

void game_process_commands(GameWorld *g)
{
  char comm[MAX_INPUT_LENGTH];
  int aliased;

  for (descriptor_data *d : each_desc(g)) {

    /*
     * Not combined to retain --(d->wait) behavior. -gg 2/20/98
     * If no wait state, no subtraction.  If there is a wait
     * state then 1 is subtracted. Therefore we don't go less
     * than 0 ever and don't require an 'if' bracket. -gg 2/27/99
     */

    if (d->character) {
      char_set_wait(d->character, char_get_wait(d->character) - ((char_get_wait(d->character) > 0)));

      if (char_get_wait(d->character)) {
        continue;
      }
    }

    if (!get_from_q(&d->input, comm, &aliased))
      continue;

    if (d->character) {
      /* Reset the idle timer & pull char back from void if necessary */
      char_set_timer(d->character, 0);
      if (desc_get_connected(d) == CON_PLAYING && char_get_was_in_room(d->character) != NOWHERE) {
        if (char_get_in_room(d->character) != NOWHERE)
          char_from_room(g, d->character);
        char_to_room(g, d->character, char_get_was_in_room(d->character));
        char_set_was_in_room(d->character, NOWHERE);
        act(g, "$n has returned.", true, d->character, nullptr, nullptr, TO_ROOM);
      }
      char_set_wait(d->character, 1);
    }
    d->has_prompt = false;

    if (d->str) /* Writing boards, mail, etc. */
      string_add(g, d, comm);
    else if (desc_get_connected(d) != CON_PLAYING) /* In menus, etc. */
      nanny(g, d, comm);
    else {                                           /* else: we're playing normally. */
      if (aliased)                                   /* To prevent recursive aliases. */
        d->has_prompt = true;                        /* To get newline before next cmd output. */
      else if (perform_alias(g, d, comm, sizeof(comm))) /* Run it through aliasing system */
        get_from_q(&d->input, comm, &aliased);
      command_interpreter(g, d->character, comm); /* Send it to interpreter */
    }
  }
}

/* Marks the end of a pass over the input: prompts anyone left without output, closes anyone who is leaving. */
void game_finish_descriptors(GameWorld *g)
{

  /* Print prompts for other descriptors who had no other output */
  for (descriptor_data *d : each_desc(g)) {
    if (!d->has_prompt) {
      write_to_output(g, d, "@n");
      /*write_to_descriptor(d->descriptor, make_prompt(d), d->comp);*/
      d->has_prompt = true;
    }
  }

  /* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
  for (descriptor_data *d : each_desc(g)) {
    if (desc_get_connected(d) == CON_CLOSE || desc_get_connected(d) == CON_DISCONNECT)
      close_socket(g, d);
  }
}

/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
static void game_loop(GameWorld *g)
{
  int64_t last_time, process_time, now; /* monotonic nanoseconds; last_time is when the last pulse was due */
  int missed_pulses;
  net::poll_set polled; /* [0] is the listener, then one per descriptor, in list order */

  last_time = monotonic_ns();

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!g->circle_shutdown) {

    /* SIGHUP/SIGINT/SIGTERM: leave the loop so init_game() saves everyone and closes down properly */
    if (shutdown_signal) {
      mud_log("Received signal {:d} (SIGHUP, SIGINT or SIGTERM).  Shutting down...", (int)shutdown_signal);
      g->circle_shutdown = 1;
      break;
    }

    /* Sleep if we don't have any connections */
    if (g->descriptors.empty()) {
      net::poll_set listener;
      listener.add(g->listener.handle(), false);

      mud_log("No connections.  Going to sleep.");
      switch (listener.wait(-1)) {
        case net::poll_set::outcome::interrupted:
          mud_log("Waking up to process signal.");
          break;
        case net::poll_set::outcome::failed:
          net::report_error("SYSERR: poll coma");
          break;
        case net::poll_set::outcome::ready:
          mud_log("New connection.  Waking up.");
          break;
      }
      last_time = monotonic_ns();
      if (shutdown_signal)
        continue;
    }

    /*
     * At this point, we have completed all input, output and heartbeat
     * activity from the previous iteration, so we have to put ourselves
     * to sleep until the next 0.1 second tick.  The first step is to
     * calculate how long we took processing the previous iteration.
     */

    now          = monotonic_ns();
    process_time = MAX(now - last_time, 0);

    /*
     * If we were asleep for more than one pass, count missed pulses and sleep
     * until we're resynchronized with the next upcoming pulse.
     */
    missed_pulses = process_time / PULSE_NS;
    process_time %= PULSE_NS;

    /* Sleep until the next pulse is due */
    last_time = now - process_time + PULSE_NS;
    sleep_until(last_time);

    /* Poll (without blocking) for new connections, input, output and hangups */
    polled.clear();
    polled.add(g->listener.handle(), false);
    for (descriptor_data *d : each_desc(g))
      polled.add(d->sock.handle(), true);

    for (net::poll_set::outcome waited; (waited = polled.wait(0)) != net::poll_set::outcome::ready;) {
      if (waited == net::poll_set::outcome::failed) { /* a signal arriving here is no reason to stop */
        net::report_error("SYSERR: poll");
        return;
      }
    }
    size_t polled_at = 1;
    for (descriptor_data *d : each_desc(g))
      d->ready = polled.at(polled_at++);

    /* If there are new connections waiting, accept them. (They are polled from the next pulse on.) */
    if (polled.at(0).readable)
      new_descriptor(g);

    /* Kick out sockets in an error state, and hung-up ones with nothing left to read */
    for (descriptor_data *d : each_desc(g)) {
      if (d->ready.failed || (d->ready.hung_up && !d->ready.readable)) {
        d->ready = {};
        close_socket(g, d);
      }
    }

    /* Process descriptors with input pending */
    for (descriptor_data *d : each_desc(g)) {
      if (d->ready.readable)
        if (process_input(g, d) < 0)
          close_socket(g, d);
    }

    game_process_commands(g);

    /* Send queued output out to the operating system (ultimately to user). */
    for (descriptor_data *d : each_desc(g)) {
      if (*(d->output) && d->ready.writable) {
        /* Output for this player is ready */
        if (process_output(g, d) < 0) {
          // commented out close_socket since process_output already does this, so having it here causes a double-free crash
          //close_socket(d);
          mud_log("ERROR: Tried to send output to dead socket!");
        } else
          d->has_prompt = 1;
      }
    }

    game_finish_descriptors(g);

    /*
     * Now, we execute as many pulses as necessary--just one if we haven't
     * missed any pulses, or make up for lost time if we missed a few
     * pulses by sleeping for too long.
     */
    missed_pulses++;

    if (missed_pulses <= 0) {
      mud_log("SYSERR: **BAD** MISSED_PULSES NONPOSITIVE ({:d}), TIME GOING BACKWARDS!!", missed_pulses);
      missed_pulses = 1;
    }

    /* If we missed more than 30 seconds worth of pulses, just do 30 secs */
    if (missed_pulses > 30 * PASSES_PER_SEC) {
      mud_log("SYSERR: Missed {:d} seconds worth of pulses.", missed_pulses / PASSES_PER_SEC);
      missed_pulses = 30 * PASSES_PER_SEC;
    }

    /* Now execute the heartbeat functions */
    while (missed_pulses--)
      game_heartbeat(g, ++g->pulse);

    /* Check for any signals we may have received. */
    if (reread_wizlist) {
      reread_wizlist = false;
      mudlog(g, LOG_CMP, ADMLVL_IMMORT, true, "Signal received - rereading wizlists.");
      reboot_wizlists(g);
    }
    if (emergency_unban) {
      emergency_unban = false;
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "Received SIGUSR2 - completely unrestricting game (emergent)");
      free_ban_list(g);
      g->circle_restrict = 0;
      g->num_invalid     = 0;
    }

    /* Update tics for deadlock protection */
    tics_passed++;
  }
}

void game_heartbeat(GameWorld *g, int heart_pulse)
{

  event_process(g);

  if (!(heart_pulse % PULSE_DG_SCRIPT))
    script_trigger_check(g);

  if (!(heart_pulse % PULSE_ZONE))
    zone_update(g);

  if (!(heart_pulse % PULSE_IDLEPWD)) /* 15 seconds */
    check_idle_passwords(g);

  if (!(heart_pulse % (PULSE_1SEC * 60))) /* 15 seconds */
    check_idle_menu(g);

  if (!(heart_pulse % (PULSE_1SEC * 60 * 60))) /* 1 hour */
    dg_log_uid_stats(g);

  if (!(heart_pulse % (PULSE_IDLEPWD / 15))) { /* 1 second */
    //dball_load();
  }
  if (!(heart_pulse % (PULSE_2SEC))) {
    base_update(g);
    fish_update(g);
  }

  if (!(heart_pulse % (PULSE_1SEC * 15))) {
    handle_songs(g);
  }

  if (!(heart_pulse % (PULSE_1SEC)))
    wishSYS(g);

  if (!(heart_pulse % PULSE_MOBILE))
    mobile_activity(g);

  if (!(heart_pulse % PULSE_AUCTION))
    check_auction(g);

  if (!(heart_pulse % (PULSE_IDLEPWD / 15))) {
    fight_stack(g);
  }
  if (!(heart_pulse % ((PULSE_IDLEPWD / 15) * 2))) {
    if (rand_number(g, 1, 2) == 2) {
      homing_update(g);
    }
    huge_update(g);
    broken_update(g);
    /*update_mob_absorb();*/
  }

  if (!(heart_pulse % (1 * PASSES_PER_SEC))) { /* EVERY second */
    copyover_check(g);
  }

  if (!(heart_pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
    weather_and_time(g, 1);
    check_time_triggers(g);
    affect_update(g);
  }
  if (!(heart_pulse % ((SECS_PER_MUD_HOUR / 3) * PASSES_PER_SEC))) {
    point_update(g);
  }

  if (CONFIG_AUTO_SAVE && !(heart_pulse % PULSE_AUTOSAVE)) { /* 1 minute */
    if (++g->mins_since_crashsave >= CONFIG_AUTOSAVE_TIME) {
      g->mins_since_crashsave = 0;
      Crash_save_all(g);
      House_save_all(g);
    }
  }

  if (!(heart_pulse % PULSE_USAGE))
    record_usage(g);

  if (!(heart_pulse % PULSE_TIMESAVE))
    save_mud_time(g, &g->time_info);

  if (!(heart_pulse % (30 * PASSES_PER_SEC))) {
    timed_dt(g, nullptr);
  }

  /* Every pulse! Don't want them to stink the place up... */
  finish_extractions(g);
}

/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/* The monotonic clock in nanoseconds: unaffected by changes to the system time. */
static int64_t monotonic_ns(void)
{
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec * NS_PER_SEC + now.tv_nsec;
}

/* Sleeps until the monotonic clock reaches when (nanoseconds), through any signals. */
static void sleep_until(int64_t when)
{
  struct timespec t = {.tv_sec = when / NS_PER_SEC, .tv_nsec = when % NS_PER_SEC};

  while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr) == EINTR);
}

static void record_usage(GameWorld *g)
{
  int sockets_connected = 0, sockets_playing = 0;

  for (descriptor_data *d : each_desc(g)) {
    sockets_connected++;
    if (desc_is_playing(d))
      sockets_playing++;
  }

  mud_log("nusage: {:<3d} sockets connected, {:<3d} sockets playing",
          sockets_connected, sockets_playing);
}
