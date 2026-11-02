/*************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/consts/maximums.h"
#include "dbat/consts/songs.h"
#include "dbat/help.h"
#include "dbat/utils.h"
#include "dbat/ban.h"
#include "dbat/stringutils.h"
#include "dbat/comm.h"
#include "dbat/config.h"
#include "dbat/maputils.h"
#include "dbat/weather.h"
#include "dbat/act.wizard.h"
#include "dbat/act.misc.h"
#include "dbat/house.h"
#include "dbat/act.other.h"
#include "dbat/broadcast.h"
#include "dbat/handler.h"
#include "dbat/dg_scripts.h"
#include "dbat/act.item.h"
#include "dbat/interpreter.h"
#include "dbat/random.h"
#include "dbat/act.informative.h"
#include "dbat/dg_event.h"
#include "dbat/mobact.h"
#include "dbat/magic.h"

#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/object_utils.h"
#include "dbat/object_systems.h"
#include "dbat/extract.h"
#include "dbat/fileop.h"
#include "dbat/relocate.h"

#include "dbat/objsave.h"
#include "dbat/genolc.h"
#include "dbat/class.h"
#include "dbat/combat.h"
#include "dbat/modify.h"
#include "dbat/fight.h"
#include "dbat/local_limits.h"
#include "dbat/mail.h"
#include "dbat/races.h"
#include "dbat/room_utils.h"
#include "dbat/screen.h"
#include "nanny_internal.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <sys/resource.h>
#include <execinfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <poll.h>

static void check_idle_menu(void);
static void check_idle_passwords(void);
static void game_loop(int cmmother_desc);
static struct in_addr *get_bind_addr();
static bool get_from_q(struct txt_q *queue, char *dest, int *aliased);
static int get_max_players(void);
static void heartbeat(int heart_pulse);
static void init_descriptor(struct descriptor_data *newd, int desc);
static int init_socket(uint16_t cmport);
static int new_descriptor(int s);
static void nonblock(int s);
static bool open_logfile(const char *filename, FILE *stderr_fp);
static bool perform_subst(struct descriptor_data *t, char *orig, char *subst);
static int process_input(struct descriptor_data *t);
static int process_output(struct descriptor_data *t);
static void record_usage(void);
static int set_sendbuf(int s);
static void signal_setup(void);
constexpr int64_t NS_PER_SEC = 1000000000;
constexpr int64_t PULSE_NS   = OPT_USEC * 1000LL; /* one pass of the game loop */

static int64_t monotonic_ns(void);
static void sleep_until(int64_t when);

/* externs */

/* local globals */
static struct txt_block *bufpool = 0;         /* pool of large output buffers */
int buf_largecount               = 0;         /* # of large buffers which exist */
int buf_overflows                = 0;         /* # of overflows of output */
int buf_switches                 = 0;         /* # of switches from small to large buf */
int circle_shutdown              = 0;         /* clean shutdown */
int circle_reboot                = 0;         /* reboot the game after a shutdown */
int no_specials                  = 0;         /* Suppress ass. of special routines */
static int max_players           = 0;         /* max descriptors available */
static int tics_passed           = 0;         /* for extern checkpointing */
int scheck                       = 0;         /* for syntax checking mode */
static volatile sig_atomic_t reread_wizlist;  /* signal: SIGUSR1 */
static volatile sig_atomic_t emergency_unban; /* signal: SIGUSR2 */
static volatile sig_atomic_t shutdown_signal; /* signal: SIGHUP, SIGINT or SIGTERM (its number) */
FILE *logfile                    = nullptr;   /* Where to send the log messages. */
static const char *text_overflow = "**OVERFLOW**\r\n";
static int dg_act_check; /* toggle for act_trigger */
unsigned long pulse = 0; /* number of pulses since game start */
bool fCopyOver;          /* Are we booting in copyover mode? */
uint16_t port;
int mother_desc;
char *last_act_message = nullptr;

/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/

/* Reload players after a copyover */
static void copyover_recover()
{
  struct descriptor_data *d;
  FILE *fp;
  char host[128];
  int desc, player_i, fields;
  bool fOld;
  char name[128];
  char username[100];
  int saved_loadroom = NOWHERE;
  int set_loadroom   = NOWHERE;

  mud_log("Copyover recovery initiated");
  PCOUNTDAY = time(0) + 60;
  fp        = fopen(COPYOVER_FILE, "r");

  if (!fp) {
    perror("copyover_recover:fopen");
    mud_log("Copyover file not found. Exitting.\n\r");
    exit(1);
  }

  unlink(COPYOVER_FILE); /* In case it crashes - doesn't prevent reading */
  for (;;) {
    fOld   = true;
    fields = fscanf(fp, "%d %127s %127s %d %99s\n", &desc, name, host, &saved_loadroom, username);
    if (fields >= 1 && desc == -1) /* the end marker */
      break;
    if (fields != 5) { /* a truncated file: stop rather than reuse the last line forever */
      mud_log("SYSERR: copyover_recover: malformed or truncated copyover file; stopping after the last complete entry.");
      break;
    }

    /* Write something, and check if it goes error-free */
    if (write_to_descriptor(desc, "\n\rFolding initiated...\n\r") < 0) {
      close(desc); /* nope */
      continue;
    }

    /* create a new descriptor */
    CREATE(d, struct descriptor_data, 1);
    init_descriptor(d, desc); /* set up various stuff */

    snprintf(d->host, sizeof(d->host), "%s", host);
    d->next         = descriptor_list;
    descriptor_list = d;

    d->connected = CON_CLOSE;

    /* Now, find the pfile */

    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;

    if ((player_i = load_char(name, d->character)) >= 0) {
      char_set_pfilepos(d->character, player_i);
      if (!char_plr_flagged(d->character, PLR_DELETED)) {
        char_plr_set(d->character, PLR_WRITING, false);
        char_plr_set(d->character, PLR_MAILING, false);
        char_plr_set(d->character, PLR_CRYO, false);
        userLoad(d, username);
      }
      /*else
       fOld = FALSE;*/
    } else
      fOld = false;

    if (!fOld) /* Player file not found?! */ {
      write_to_descriptor(desc, "\n\rSomehow, your character was lost during the folding. Sorry.\n\r");
      close_socket(d);
    } else {
      write_to_descriptor(desc, "\n\rFolding complete.\n\r");
      set_loadroom = char_get_load_room(d->character);
      char_set_load_room(d->character, saved_loadroom);
      enter_player_game(d);
      char_set_load_room(d->character, set_loadroom);
      d->connected = CON_PLAYING;
      look_at_room(char_get_in_room(d->character), d->character, 0);
      if (char_aff_flagged(d->character, AFF_HAYASA)) {
        char_set_speedboost(d->character, char_get_speedcalc(d->character) * 0.5);
      }
    }
  }
  fclose(fp);
}

/* Init sockets, run game, and cleanup sockets */
void init_game(uint16_t cmport)
{
  /* We don't want to restart if we crash before we get up. */
  touch(KILLSCRIPT_FILE);

  circle_srandom(time(0));

  mud_log("Finding player limit.");
  max_players = get_max_players();

  if (!fCopyOver) { /* If copyover mother_desc is already set up */
    mud_log("Opening mother connection.");
    mother_desc = init_socket(cmport);
  }

  event_init();

  /* set up hash table for find_char() */
  init_lookup_table();

  boot_db();

  FILE *mapfile;
  int rowcounter, colcounter;
  int vnum_read;

  mud_log("Signal trapping.");
  signal_setup();

  mud_log("Loading Space Map. ");

  //Load the map vnums from a file into an array
  if (!(mapfile = fopen("../data/surface.map", "r"))) {
    mud_log("SYSERR: Unable to open ../data/surface.map: %s", strerror(errno));
    exit(1);
  }

  for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++) {
    for (colcounter = 0; colcounter <= MAP_COLS; colcounter++) {
      fscanf(mapfile, "%d", &vnum_read);
      mapnums[rowcounter][colcounter] = real_room(vnum_read);
    }
  }

  fclose(mapfile);

  /* Load the toplist */
  topLoad();

  /* If we made it this far, we will be able to restart without problem. */
  remove(KILLSCRIPT_FILE);

  if (fCopyOver) /* reload players */
    copyover_recover();

  mud_log("Entering game loop.");

  game_loop(mother_desc);

  Crash_save_all();

  mud_log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);

  close(mother_desc);

  if (circle_reboot != 2)
    save_all();

  mud_log("Saving current MUD time.");
  save_mud_time(&time_info);

  if (circle_reboot) {
    mud_log("Rebooting.");
    exit(52); /* what's so great about HHGTTG, anyhow? */
  }
  mud_log("Normal termination of game.");
}

/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
static int init_socket(uint16_t cmport)
{
  int s;
  struct sockaddr_in sa;
  int opt;

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("SYSERR: Error creating socket");
    exit(1);
  }

  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt REUSEADDR");
    exit(1);
  }

  set_sendbuf(s);

  {
    struct linger ld;

    ld.l_onoff  = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld)) < 0)
      perror("SYSERR: setsockopt SO_LINGER"); /* Not fatal I suppose. */
  }

  /* Clear the structure */
  memset((char *)&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;
  sa.sin_port   = htons(cmport);
  sa.sin_addr   = *(get_bind_addr());

  if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
    perror("SYSERR: bind");
    close(s);
    exit(1);
  }
  nonblock(s);
  if (listen(s, SOMAXCONN) < 0) {
    perror("SYSERR: listen");
    close(s);
    exit(1);
  }
  return (s);
}

static int get_max_players(void)
{

  int max_descs = 0;
  const char *how;

  /*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
  {
    struct rlimit limit;

    /* find the limit of file descs */
    how = "rlimit";
    if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling getrlimit");
      exit(1);
    }

    /* set the current to the maximum */
    limit.rlim_cur = limit.rlim_max;
    if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling setrlimit");
      exit(1);
    }
    if (limit.rlim_max == RLIM_INFINITY)
      max_descs = CONFIG_MAX_PLAYING + NUM_RESERVED_DESCS;
    else
      max_descs = MIN(CONFIG_MAX_PLAYING + NUM_RESERVED_DESCS, limit.rlim_max);
  }

  /* now calculate max _players_ based on max descs */
  max_descs = MIN(CONFIG_MAX_PLAYING, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    mud_log("SYSERR: Non-positive max player limit!  (Set at %d using %s).",
            max_descs, how);
    exit(1);
  }
  mud_log("   Setting player limit to %d using %s.", max_descs, how);
  return (max_descs);
}

/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
static void game_loop(int cmmother_desc)
{
  int64_t last_time, process_time, now; /* monotonic nanoseconds; last_time is when the last pulse was due */
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d;
  int missed_pulses, aliased;
  struct pollfd *fds = nullptr; /* [0] is the mother socket, then one per descriptor, in list order */
  size_t fds_alloc   = 0, nfds;

  last_time = monotonic_ns();

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) {

    /* SIGHUP/SIGINT/SIGTERM: leave the loop so init_game() saves everyone and closes down properly */
    if (shutdown_signal) {
      mud_log("Received signal %d (SIGHUP, SIGINT or SIGTERM).  Shutting down...", (int)shutdown_signal);
      circle_shutdown = 1;
      break;
    }

    /* Sleep if we don't have any connections */
    if (descriptor_list == nullptr) {
      struct pollfd mother = {.fd = cmmother_desc, .events = POLLIN};

      mud_log("No connections.  Going to sleep.");
      if (poll(&mother, 1, -1) < 0) {
        if (errno == EINTR)
          mud_log("Waking up to process signal.");
        else
          perror("SYSERR: poll coma");
      } else
        mud_log("New connection.  Waking up.");
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
    nfds = 1;
    for (d = descriptor_list; d; d = d->next)
      nfds++;
    if (nfds > fds_alloc) {
      fds_alloc = nfds + 16;
      RECREATE(fds, struct pollfd, fds_alloc);
    }
    fds[0] = (struct pollfd){.fd = cmmother_desc, .events = POLLIN};
    nfds   = 1;
    for (d = descriptor_list; d; d = d->next)
      fds[nfds++] = (struct pollfd){.fd = d->descriptor, .events = POLLIN | POLLOUT};

    while (poll(fds, nfds, 0) < 0) {
      if (errno != EINTR) { /* a signal arriving here is no reason to stop */
        perror("SYSERR: poll");
        free(fds);
        return;
      }
    }
    nfds = 1;
    for (d = descriptor_list; d; d = d->next)
      d->poll_revents = fds[nfds++].revents;

    /* If there are new connections waiting, accept them. (They are polled from the next pulse on.) */
    if (fds[0].revents & POLLIN)
      new_descriptor(cmmother_desc);

    /* Kick out sockets in an error state, and hung-up ones with nothing left to read */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if ((d->poll_revents & (POLLERR | POLLNVAL)) ||
          ((d->poll_revents & POLLHUP) && !(d->poll_revents & POLLIN))) {
        d->poll_revents = 0;
        close_socket(d);
      }
    }

    /* Process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d->poll_revents & POLLIN)
        if (process_input(d) < 0)
          close_socket(d);
    }

    /* Process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;

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
        d->character->timer = 0;
        if (desc_get_connected(d) == CON_PLAYING && char_get_was_in_room(d->character) != NOWHERE) {
          if (char_get_in_room(d->character) != NOWHERE)
            char_from_room(d->character);
          char_to_room(d->character, char_get_was_in_room(d->character));
          char_set_was_in_room(d->character, NOWHERE);
          act("$n has returned.", true, d->character, 0, 0, TO_ROOM);
        }
        char_set_wait(d->character, 1);
      }
      d->has_prompt = false;

      if (d->str) /* Writing boards, mail, etc. */
        string_add(d, comm);
      else if (desc_get_connected(d) != CON_PLAYING) /* In menus, etc. */
        nanny(d, comm);
      else {                                           /* else: we're playing normally. */
        if (aliased)                                   /* To prevent recursive aliases. */
          d->has_prompt = true;                        /* To get newline before next cmd output. */
        else if (perform_alias(d, comm, sizeof(comm))) /* Run it through aliasing system */
          get_from_q(&d->input, comm, &aliased);
        command_interpreter(d->character, comm); /* Send it to interpreter */
      }
    }

    /* Send queued output out to the operating system (ultimately to user). */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (*(d->output) && (d->poll_revents & POLLOUT)) {
        /* Output for this player is ready */
        if (process_output(d) < 0) {
          // commented out close_socket since process_output already does this, so having it here causes a double-free crash
          //close_socket(d);
          mud_log("ERROR: Tried to send output to dead socket!");
        } else
          d->has_prompt = 1;
      }
    }

    /* Print prompts for other descriptors who had no other output */
    for (d = descriptor_list; d; d = d->next) {
      if (!d->has_prompt) {
        write_to_output(d, "@n");
        /*write_to_descriptor(d->descriptor, make_prompt(d), d->comp);*/
        d->has_prompt = true;
      }
    }

    /* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (desc_get_connected(d) == CON_CLOSE || desc_get_connected(d) == CON_DISCONNECT)
        close_socket(d);
    }

    /*
     * Now, we execute as many pulses as necessary--just one if we haven't
     * missed any pulses, or make up for lost time if we missed a few
     * pulses by sleeping for too long.
     */
    missed_pulses++;

    if (missed_pulses <= 0) {
      mud_log("SYSERR: **BAD** MISSED_PULSES NONPOSITIVE (%d), TIME GOING BACKWARDS!!", missed_pulses);
      missed_pulses = 1;
    }

    /* If we missed more than 30 seconds worth of pulses, just do 30 secs */
    if (missed_pulses > 30 * PASSES_PER_SEC) {
      mud_log("SYSERR: Missed %d seconds worth of pulses.", missed_pulses / PASSES_PER_SEC);
      missed_pulses = 30 * PASSES_PER_SEC;
    }

    /* Now execute the heartbeat functions */
    while (missed_pulses--)
      heartbeat(++pulse);

    /* Check for any signals we may have received. */
    if (reread_wizlist) {
      reread_wizlist = false;
      mudlog(LOG_CMP, ADMLVL_IMMORT, true, "Signal received - rereading wizlists.");
      reboot_wizlists();
    }
    if (emergency_unban) {
      emergency_unban = false;
      mudlog(LOG_BRF, ADMLVL_IMMORT, true, "Received SIGUSR2 - completely unrestricting game (emergent)");
      free_ban_list();
      circle_restrict = 0;
      num_invalid     = 0;
    }

    /* Update tics for deadlock protection */
    tics_passed++;
  }
  free(fds);
}

static void heartbeat(int heart_pulse)
{
  static int mins_since_crashsave = 0;

  event_process();

  if (!(heart_pulse % PULSE_DG_SCRIPT))
    script_trigger_check();

  if (!(heart_pulse % PULSE_ZONE))
    zone_update();

  if (!(heart_pulse % PULSE_IDLEPWD)) /* 15 seconds */
    check_idle_passwords();

  if (!(heart_pulse % (PULSE_1SEC * 60))) /* 15 seconds */
    check_idle_menu();

  if (!(heart_pulse % (PULSE_1SEC * 60 * 60))) /* 1 hour */
    dg_log_uid_stats();

  if (!(heart_pulse % (PULSE_IDLEPWD / 15))) { /* 1 second */
    //dball_load();
  }
  if (!(heart_pulse % (PULSE_2SEC))) {
    base_update();
    fish_update();
  }

  if (!(heart_pulse % (PULSE_1SEC * 15))) {
    handle_songs();
  }

  if (!(heart_pulse % (PULSE_1SEC)))
    wishSYS();

  if (!(heart_pulse % PULSE_MOBILE))
    mobile_activity();

  if (!(heart_pulse % PULSE_AUCTION))
    check_auction();

  if (!(heart_pulse % (PULSE_IDLEPWD / 15))) {
    fight_stack();
  }
  if (!(heart_pulse % ((PULSE_IDLEPWD / 15) * 2))) {
    if (rand_number(1, 2) == 2) {
      homing_update();
    }
    huge_update();
    broken_update();
    /*update_mob_absorb();*/
  }

  if (!(heart_pulse % (1 * PASSES_PER_SEC))) { /* EVERY second */
    copyover_check();
  }

  if (!(heart_pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
    weather_and_time(1);
    check_time_triggers();
    affect_update();
  }
  if (!(heart_pulse % ((SECS_PER_MUD_HOUR / 3) * PASSES_PER_SEC))) {
    point_update();
  }

  if (CONFIG_AUTO_SAVE && !(heart_pulse % PULSE_AUTOSAVE)) { /* 1 minute */
    if (++mins_since_crashsave >= CONFIG_AUTOSAVE_TIME) {
      mins_since_crashsave = 0;
      Crash_save_all();
      House_save_all();
    }
  }

  if (!(heart_pulse % PULSE_USAGE))
    record_usage();

  if (!(heart_pulse % PULSE_TIMESAVE))
    save_mud_time(&time_info);

  if (!(heart_pulse % (30 * PASSES_PER_SEC))) {
    timed_dt(nullptr);
  }

  /* Every pulse! Don't want them to stink the place up... */
  extract_pending_chars();
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

static void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (desc_is_playing(d))
      sockets_playing++;
  }

  mud_log("nusage: %-3d sockets connected, %-3d sockets playing",
          sockets_connected, sockets_playing);
}

static char *make_prompt(struct descriptor_data *d)
{
  static char prompt[MAX_PROMPT_LENGTH];
  struct obj_data *chair = nullptr;
  int flagged            = false;

  /* Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h). snprintf() returns the length it
     wanted, so len is clamped: it must never pass the end, or the next append is handed a negative size. */

  if (d->str) {
    if (desc_get_connected(d) == CON_EXDESC) {
      strcpy(prompt, "Enter Description(/h for editor help)> ");
    } else if (char_plr_flagged(d->character, PLR_WRITING) && !char_plr_flagged(d->character, PLR_MAILING)) {
      strcpy(prompt, "Enter Message(/h for editor help)> ");
    } else if (char_plr_flagged(d->character, PLR_MAILING)) {
      strcpy(prompt, "Enter Mail Message(/h for editor help)> ");
    } else {
      strcpy(prompt, "Enter Message> ");
    }
  } else if (desc_get_connected(d) == CON_PLAYING && !char_is_npc(d->character)) {
    int count;
    size_t len = 0;

    *prompt = '\0';

    if (char_get_invis_level(d->character) && len < sizeof(prompt)) {
      count = snprintf(prompt + len, sizeof(prompt) - len, "i%d ", char_get_invis_level(d->character));
      if (count >= 0)
        len = MIN(len + count, sizeof(prompt) - 1);
    }
    /* show only when below 25% */
    if (char_prf_flagged(d->character, PRF_DISPAUTO) && char_get_level(d->character) >= 500 && len < sizeof(prompt)) {
      struct char_data *ch = d->character;
      if (getCurPL(ch) << 2 < getEffMaxPL(ch)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "PL: %" PRId64 " ", getCurPL(ch));
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if ((getCurST(ch)) << 2 < getMaxST(ch) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "STA: %" PRId64 " ", (getCurST(ch)));
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_ki(ch) << 2 < char_get_max_ki(ch) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "KI: %" PRId64 " ", char_get_ki(ch));
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
    } else { /* not auto prompt */
      if (len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@w");
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_plr_flagged(d->character, PLR_SELFD) && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RSELF-D@r: @w%s@D]@n", char_plr_flagged(d->character, PLR_SELFD2) ? "READY" : "PREP");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_is_halfbreed(d->character) && !char_plr_flagged(d->character, PLR_FURY) && char_prf_flagged(d->character, PRF_FURY)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mFury@W: @r%d@D]@w", char_get_fury(d->character));
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_is_halfbreed(d->character) && char_plr_flagged(d->character, PLR_FURY) && char_prf_flagged(d->character, PRF_FURY)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mFury@W: @rENGAGED@D]@w");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (has_mail(char_get_idnum(d->character)) && !char_prf_flagged(d->character, PRF_NMWARN) && (char_get_admlevel(d->character) > 0) && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "CHECK MAIL - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_kaioken(d->character) > 0 && char_get_admlevel(d->character) > 0) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "KAIOKEN X%d - ", char_get_kaioken(d->character));
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_powerattack(d->character) > 0) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "%s - ", song_types[char_get_powerattack(d->character)]);
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_kaioken(d->character) > 0 && char_get_admlevel(d->character) <= 0) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "KAIOKEN X%d - ", char_get_kaioken(d->character));
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (has_mail(char_get_idnum(d->character)) && (char_get_admlevel(d->character) <= 0) && !char_prf_flagged(d->character, PRF_NMWARN) && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "CHECK MAIL - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (d->snooping && d->snooping->character != nullptr && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Snooping: (%s) - ", char_get_name(d->snooping->character));
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_drag(d->character) && char_get_drag(d->character) != nullptr && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Dragging: (%s) - ", char_get_name(char_get_drag(d->character)));
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_prf_flagged(d->character, PRF_BUILDWALK) && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "BUILDWALKING - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_aff_flagged(d->character, AFF_FLYING) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "FLYING - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_aff_flagged(d->character, AFF_HIDE) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "HIDING - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_plr_flagged(d->character, PLR_SPAR) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "SPARRING - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_plr_flagged(d->character, PLR_NOSHOUT) && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "MUTED - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 51 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Bash) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 52 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Headbutt) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 56 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Tailwhip) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 0 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Punch) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 1 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Kick) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 2 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Elbow) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 3 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Knee) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 4 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Roundhouse) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 5 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Uppercut) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 6 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Slam) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_combo(d->character) == 8 && len < sizeof(prompt)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Heeldrop) - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_prf_flagged(d->character, PRF_AFK) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "AFK - ");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_plr_flagged(d->character, PLR_FISHING) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        count   = snprintf(prompt + len, sizeof(prompt) - len, "FISHING -");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (flagged == true && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@n\n");
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if ((char_get_sits(d->character) && char_plr_flagged(d->character, PLR_HEALT)) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        chair   = char_get_sits(d->character);
        count   = snprintf(prompt + len, sizeof(prompt) - len, "@c<@CFloating inside a healing tank@c>@n\r\n");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if ((char_get_sits(d->character) && char_get_position(d->character) == POS_SITTING) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        chair   = char_get_sits(d->character);
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Sitting on: %s\r\n", chair->short_description);
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if ((char_get_sits(d->character) && char_get_position(d->character) == POS_RESTING) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        chair   = char_get_sits(d->character);
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Resting on: %s\r\n", chair->short_description);
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if ((char_get_sits(d->character) && char_get_position(d->character) == POS_SLEEPING) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        chair   = char_get_sits(d->character);
        count   = snprintf(prompt + len, sizeof(prompt) - len, "Sleeping on: %s\r\n", chair->short_description);
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_aff_flagged(d->character, AFF_POSITION) && len < sizeof(prompt) && !char_prf_flagged(d->character, PRF_NODEC)) {
        chair   = char_get_sits(d->character);
        count   = snprintf(prompt + len, sizeof(prompt) - len, "(Best Position)\r\n");
        flagged = true;
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_get_charge(d->character) < getMaxKI(d->character) * .01 && char_get_charge(d->character) > 0) {
        char_set_charge(d->character, 0);
      }
      if (char_get_charge(d->character) > 0) {
        int64_t charge = char_get_charge(d->character);
        if (!char_prf_flagged(d->character, PRF_NODEC) && !char_prf_flagged(d->character, PRF_DISPERC)) {
          if (charge >= getMaxKI(d->character)) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G==@D<@RMAX@D>@G===@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .95) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=========-@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .9) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=========@g-@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .85) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G========-@g-@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .80) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G========@g--@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .75) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=======-@g--@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .70) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=======@g---@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .65) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G======-@g---@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .60) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G======@g----@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .55) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=====-@g----@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .50) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=====@g-----@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .45) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G====-@g-----@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .40) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G====@g------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .35) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G===-@g------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .30) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G===@g-------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .25) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G==-@g-------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .20) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G==@g--------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .15) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=-@g--------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .10) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=@g---------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (charge >= getMaxKI(d->character) * .05) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G-@g---------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@g----------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          }
        }
        if (char_prf_flagged(d->character, PRF_DISPERC) && !char_prf_flagged(d->character, PRF_NODEC)) {
          if (char_get_charge(d->character) > 0) {
            int64_t perc = (char_get_charge(d->character) * 100) / getMaxKI(d->character);
            count        = snprintf(prompt + len, sizeof(prompt) - len, "@D[@BCharge@Y: @C%" PRId64 "%s@D]@n\n", perc, "%");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          }
        }
        if (char_prf_flagged(d->character, PRF_NODEC)) {
          if (charge > 0) {
            int64_t perc = (charge * 100) / getMaxKI(d->character);
            count        = snprintf(prompt + len, sizeof(prompt) - len, "Ki is charged to %" PRId64 " percent.\n", perc);
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          }
        }
      }
      if (char_aff_flagged(d->character, AFF_FIRESHIELD)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D(@rF@RI@YR@rE@RS@YH@rI@RE@YL@rD@D)@n\n");
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_aff_flagged(d->character, AFF_SANCTUARY)) {
        if (char_prf_flagged(d->character, PRF_DISPERC) && !char_prf_flagged(d->character, PRF_NODEC)) {
          if (char_get_barrier(d->character) > 0) {
            int64_t perc = (char_get_barrier(d->character) * 100) / getMaxKI(d->character);
            count        = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GBarrier@Y: @B%" PRId64 "%s@D]@n\n", perc, "%");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          }
        }

        if (!char_prf_flagged(d->character, PRF_NODEC) && !char_prf_flagged(d->character, PRF_DISPERC)) {
          if (char_get_barrier(d->character) >= getMaxKI(d->character) * .75) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C==MAX==@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .70) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=======@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .65) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C======-@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .60) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C======@c-@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .55) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=====-@c-@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .50) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=====@c--@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .45) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C====-@c--@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .40) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C====@c---@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .35) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C===-@c---@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .30) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C===@c----@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .25) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C==-@c----@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .20) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C==@c-----@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .15) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=-@c-----@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .10) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=@c------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) >= getMaxKI(d->character) * .05) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C-@c------@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          } else if (char_get_barrier(d->character) < getMaxKI(d->character) * .05) {
            count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C--Low-@D]@n\n");
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          }
        }
        if (char_prf_flagged(d->character, PRF_NODEC)) {
          if (char_get_barrier(d->character) > 0) {
            int64_t perc = (char_get_barrier(d->character) * 100) / getMaxKI(d->character);
            count        = snprintf(prompt + len, sizeof(prompt) - len, "A barrier charged to %" PRId64 " percent surrounds you.@n\n", perc);
            if (count >= 0)
              len = MIN(len + count, sizeof(prompt) - 1);
          }
        }
      }
      if (!char_prf_flagged(d->character, PRF_DISPERC)) {
        if (char_prf_flagged(d->character, PRF_DISPHP) && len < sizeof(prompt)) {
          const char *col      = "n";
          struct char_data *ch = d->character;
          if (getMaxPL(ch) > getMaxPLTrans(ch))
            col = "g";
          else if (isWeightedPL(ch))
            col = "m";
          else if (getCurHealthPercent(ch) > .5)
            col = "c";
          else if (getCurHealthPercent(ch) > .1)
            col = "y";
          else if (getCurHealthPercent(ch) <= .1)
            col = "r";

          if ((count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @%s%s@D]@n", col, add_commas(getCurPL(ch)))) > 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        }
      } else if (char_prf_flagged(d->character, PRF_DISPHP)) {

        struct char_data *ch = d->character;
        double perc          = ((double)getCurHealth(ch) / (double)getMaxPLTrans(ch)) * 100;
        const char *col      = "n";
        if (perc > 100)
          col = "g";
        else if (perc >= 70)
          col = "c";
        else if (perc >= 51)
          col = "Y";
        else if (perc >= 20)
          col = "y";
        else
          col = "r";

        if ((count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @%s%d%s@D]@n", col, (int)perc, "@w%")) > 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (!char_prf_flagged(d->character, PRF_DISPERC)) {
        if (char_prf_flagged(d->character, PRF_DISPKI) && len < sizeof(prompt) &&
            (getCurKI(d->character)) > getMaxKI(d->character) / 2) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @c%s@D]@n", add_commas((getCurKI(d->character))));
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (char_prf_flagged(d->character, PRF_DISPKI) && len < sizeof(prompt) &&
                   (getCurKI(d->character)) > getMaxKI(d->character) / 10) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @y%s@D]@n", add_commas((getCurKI(d->character))));
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (char_prf_flagged(d->character, PRF_DISPKI) && len < sizeof(prompt) &&
                   (getCurKI(d->character)) <= getMaxKI(d->character) / 10) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @r%s@D]@n", add_commas((getCurKI(d->character))));
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        }
      } else if (char_prf_flagged(d->character, PRF_DISPKI)) {
        int64_t power = (getCurKI(d->character)), maxpower = getMaxKI(d->character);
        int perc = 0;
        if (power <= 0) {
          power = 1;
        }
        if (maxpower <= 0) {
          maxpower = 1;
        }
        perc = (power * 100) / maxpower;
        if (perc > 100) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @G%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (perc >= 70) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @c%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (perc >= 51) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @Y%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (perc >= 20) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @y%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @r%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        }
      }
      if (!char_prf_flagged(d->character, PRF_DISPERC)) {
        if (char_prf_flagged(d->character, PRF_DISPMOVE) && len < sizeof(prompt) &&
            (getCurST(d->character)) > getMaxST(d->character) / 2) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @c%s@D]@n", add_commas((getCurST(d->character))));
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (char_prf_flagged(d->character, PRF_DISPMOVE) && len < sizeof(prompt) &&
                   (getCurST(d->character)) > getMaxST(d->character) / 10) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @y%s@D]@n", add_commas((getCurST(d->character))));
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (char_prf_flagged(d->character, PRF_DISPMOVE) && len < sizeof(prompt) &&
                   (getCurST(d->character)) <= getMaxST(d->character) / 10) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @r%s@D]@n", add_commas((getCurST(d->character))));
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        }
      } else if (char_prf_flagged(d->character, PRF_DISPMOVE)) {
        int64_t power = (getCurST(d->character)), maxpower = getMaxST(d->character);
        int perc = 0;
        if (power <= 0) {
          power = 1;
        }
        if (maxpower <= 0) {
          maxpower = 1;
        }
        perc = (power * 100) / maxpower;
        if (perc > 100) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @G%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (perc >= 70) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @c%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (perc >= 51) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @Y%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else if (perc >= 20) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @y%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        } else {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @r%d%s@D]@n", perc, "@w%");
          if (count >= 0)
            len = MIN(len + count, sizeof(prompt) - 1);
        }
      }
      if (char_prf_flagged(d->character, PRF_DISPTNL) && len < sizeof(prompt) && char_get_level(d->character) < 100) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@yTNL@Y: @W%s@D]@n", add_commas(level_exp(d->character, char_get_level(d->character) + 1) - char_get_exp(d->character)));
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_prf_flagged(d->character, PRF_DISTIME) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@W%2d %s@D]@n", (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12), time_info.hours >= 12 ? "PM" : "AM");
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_prf_flagged(d->character, PRF_DISGOLD) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@YZen@y: @W%s@D]@n", add_commas(char_get_gold(d->character)));
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_prf_flagged(d->character, PRF_DISPRAC) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mPS@y: @W%s@D]@n", add_commas(char_get_practice_points(d->character)));
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (char_prf_flagged(d->character, PRF_DISHUTH) && len < sizeof(prompt)) {
        int hun = char_get_conditions(d->character, HUNGER), thir = char_get_conditions(d->character, THIRST);
        count = snprintf(prompt + len, sizeof(prompt) - len, "\n@D[@mHung@y:");
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
        if (hun >= 48) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WFull@D]@n");
        } else if (hun >= 40) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WAlmost Full@D]@n");
        } else if (hun >= 30) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WNeed Snack@D]@n");
        } else if (hun >= 20) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WHungry@D]@n");
        } else if (hun >= 15) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WVery Hungry@D]@n");
        } else if (hun >= 10) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WAlmost Starving@D]@n");
        } else if (hun >= 5) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WNear Starving@D]@n");
        } else if (hun >= 0) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WStarving@D]@n");
        } else {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WN/A@D]@n");
        }
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
        if (thir >= 48) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WQuenched@D]@n");
        } else if (thir >= 40) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WNeed Sip@D]@n");
        } else if (thir >= 30) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WNeed Drink@D]@n");
        } else if (thir >= 20) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WThirsty@D]@n");
        } else if (thir >= 15) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WVery Thirsty@D]@n");
        } else if (thir >= 10) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WAlmost Dehydrated@D]@n");
        } else if (thir >= 5) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WNear Dehydration@D]@n");
        } else if (thir >= 0) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WDehydrated@D]@n");
        } else {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WN/A@D]@n");
        }
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (len < sizeof(prompt) && has_group(d->character) && !char_prf_flagged(d->character, PRF_GHEALTH)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "\n%s", report_party_health(d->character));
        if (d->character->temp_prompt)
          free(d->character->temp_prompt);
        if (count >= 0)
          len = MIN(len + count, sizeof(prompt) - 1);
      }
      if (len < sizeof(prompt))
        count = snprintf(prompt + len, sizeof(prompt) - len, "\n"); /* strncat: OK */
    }
    if (len < sizeof(prompt) && len < 5)
      strncat(prompt, ">\n", sizeof(prompt) - len - 1); /* strncat: OK */

  } else if (desc_get_connected(d) == CON_PLAYING && char_is_npc(d->character))
    snprintf(prompt, sizeof(prompt), "%s>\n", CAP(char_get_name(d->character)));
  else
    *prompt = '\0';

  return (prompt);
}

/*
 * NOTE: 'txt' must be at most MAX_INPUT_LENGTH big.
 */
void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
  struct txt_block *newt;

  CREATE(newt, struct txt_block, 1);
  newt->text    = strdup(txt);
  newt->aliased = aliased;
  if (!aliased)
    queue->typed++;

  /* queue empty? */
  if (!queue->head)
    queue->head = queue->tail = newt;
  else {
    queue->tail->next = newt;
    queue->tail       = newt;
  }
}

/*
 * NOTE: 'dest' must be at least MAX_INPUT_LENGTH big.
 */
static bool get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return false;

  strcpy(dest, queue->head->text); /* strcpy: OK (mutual MAX_INPUT_LENGTH) */
  *aliased = queue->head->aliased;
  if (!*aliased)
    queue->typed--;

  tmp         = queue->head;
  queue->head = queue->head->next;
  free(tmp->text);
  free(tmp);

  return true;
}

/* Throw away the commands a descriptor has typed but not yet run. */
static void flush_input_queue(struct descriptor_data *d)
{
  while (d->input.head) {
    struct txt_block *tmp = d->input.head;
    d->input.head         = d->input.head->next;
    free(tmp->text);
    free(tmp);
  }
  d->input.typed = 0;
}

/* Empty the queues before closing connection */
static void flush_queues(struct descriptor_data *d)
{
  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool               = d->large_outbuf;
    d->large_outbuf       = nullptr;
    d->output             = d->small_outbuf;
  }
  flush_input_queue(d);
}

/* Add a new string to a player's output queue. For outside use. */
size_t write_to_output(struct descriptor_data *t, const char *txt, ...)
{
  va_list args;
  size_t left;

  va_start(args, txt);
  left = vwrite_to_output(t, txt, args);
  va_end(args);

  return left;
}

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

/* Color replacement arrays. Orig. Renx -- 011100, now modified */
static const char *ANSI[] = {
    "@",
    AA_NORMAL,
    AA_NORMAL ANSISEPSTR AF_BLACK,
    AA_NORMAL ANSISEPSTR AF_BLUE,
    AA_NORMAL ANSISEPSTR AF_GREEN,
    AA_NORMAL ANSISEPSTR AF_CYAN,
    AA_NORMAL ANSISEPSTR AF_RED,
    AA_NORMAL ANSISEPSTR AF_MAGENTA,
    AA_NORMAL ANSISEPSTR AF_YELLOW,
    AA_NORMAL ANSISEPSTR AF_WHITE,
    AA_BOLD ANSISEPSTR AF_BLACK,
    AA_BOLD ANSISEPSTR AF_BLUE,
    AA_BOLD ANSISEPSTR AF_GREEN,
    AA_BOLD ANSISEPSTR AF_CYAN,
    AA_BOLD ANSISEPSTR AF_RED,
    AA_BOLD ANSISEPSTR AF_MAGENTA,
    AA_BOLD ANSISEPSTR AF_YELLOW,
    AA_BOLD ANSISEPSTR AF_WHITE,
    AB_BLACK,
    AB_BLUE,
    AB_GREEN,
    AB_CYAN,
    AB_RED,
    AB_MAGENTA,
    AB_YELLOW,
    AB_WHITE,
    AA_BLINK,
    AA_UNDERLINE,
    AA_BOLD,
    AA_REVERSE,
    "!"};

static const char CCODE[] = "@ndbgcrmywDBGCRMYW01234567luoex!";
/*
  Codes are:      @n - normal
  @d - black      @D - gray           @0 - background black
  @b - blue       @B - bright blue    @1 - background blue
  @g - green      @G - bright green   @2 - background green
  @c - cyan       @C - bright cyan    @3 - background cyan
  @r - red        @R - bright red     @4 - background red
  @m - magneta    @M - bright magneta @5 - background magneta
  @y - yellow     @Y - bright yellow  @6 - background yellow
  @w - white      @W - bright white   @7 - background white
  @x - random
Extra codes:      @l - blink          @o - bold
  @u - underline  @e - reverse video  @@ - single @

  @[num] - user color choice num, [] are required
*/
static const char RANDOM_COLORS[] = "bgcrmywBGCRMWY";

#define NEW_STRING_LENGTH (size_t)(dest_char - save_pos)
static size_t proc_colors(char *txt, size_t maxlen, int parse, char **choices)
{
  char *dest_char, *source_char, *save_pos;
  const char *color_char, *replacement = nullptr;
  int i, temp_color;
  size_t wanted;

  if (!txt || !strchr(txt, '@')) /* skip out if no color codes     */
    return strlen(txt);

  source_char = txt;
  CREATE(dest_char, char, maxlen);
  save_pos = dest_char;
  /* Leave room for the terminating NUL: every step below writes at most one byte, except a color substitution,
   * which checks its own length. */
  for (; *source_char && (NEW_STRING_LENGTH + 1 < maxlen);) {
    /* no color code - just copy */
    if (*source_char != '@') {
      *dest_char++ = *source_char++;
      continue;
    }

    /* if we get here we have a color code */

    source_char++; /* source_char now points to the code */

    /* look for a random color code picks a random number between 1 and 14 */
    if (*source_char == 'x') {
      temp_color   = (rand() % 14);
      *source_char = RANDOM_COLORS[temp_color];
    }

    if (*source_char == '\0') { /* string was terminated with color code - just put it in */
      *dest_char++ = '@';
      /* source_char will now point to '\0' in the for() check */
      continue;
    }

    if (!parse) { /* not parsing, just skip the code, unless it's @@ */
      if (*source_char == '@') {
        *dest_char++ = '@';
      }
      if (*source_char == '[') { /* Multi-character code */
        source_char++;
        while (*source_char && isdigit(*source_char))
          source_char++;
        if (!*source_char)
          source_char--;
      }
      source_char++; /* skip to next (non-colorcode) char */
      continue;
    }

    /* parse the color code */
    if (*source_char == '[') { /* User configurable color */
      source_char++;
      if (*source_char) {
        i = atoi(source_char);
        if (i < 0 || i >= NUM_COLOR)
          i = COLOR_NORMAL;
        replacement = default_color_choices[i];
        if (choices && choices[i])
          replacement = choices[i];
        while (*source_char && isdigit(*source_char))
          source_char++;
        if (!*source_char)
          source_char--;
      }
    } else if (*source_char == 'n') {
      replacement = default_color_choices[COLOR_NORMAL];
      if (choices && choices[COLOR_NORMAL])
        replacement = choices[COLOR_NORMAL];
    } else {
      for (i = 0; CCODE[i] != '!'; i++) { /* do we find it ? */
        if ((*source_char) == CCODE[i]) { /* if so :*/
          replacement = ANSI[i];
          break;
        }
      }
    }
    if (replacement) {
      if (NEW_STRING_LENGTH + strlen(replacement) + strlen(ANSISTART) + 1 < maxlen) { /* only substitute if there's room for the whole code */
        if (isdigit(replacement[0]))
          for (color_char = ANSISTART; *color_char;)
            *dest_char++ = *color_char++;
        for (color_char = replacement; *color_char;)
          *dest_char++ = *color_char++;
        if (isdigit(replacement[0]))
          *dest_char++ = ANSIEND;
      }
      replacement = nullptr;
    }
    /* If we couldn't find any correct color code, or we found it and
    * substituted above, let's just process the next character.
    * - Welcor
    */
    source_char++;

  } /* for loop */

  /* make sure output is NULL - terminated */
  *dest_char = '\0';

  wanted = strlen(source_char); /* see if we wanted more space */
  strncpy(txt, save_pos, maxlen - 1);
  wanted += NEW_STRING_LENGTH;
  free(save_pos); /* plug memory leak */

  return wanted;
}
#undef NEW_STRING_LENGTH

/* Add a new string to a player's output queue. */
size_t vwrite_to_output(struct descriptor_data *t, const char *format, va_list args)
{
  /* As big as a descriptor's whole output buffer, so one long text (help, lists) is never cut short here. */
  static char txt[LARGE_BUFSIZE];
  size_t wantsize;
  int size;

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufspace == 0)
    return (0);

  wantsize = size = vsnprintf(txt, sizeof(txt), format, args);
  if (t->character)
    wantsize = size = proc_colors(txt, sizeof(txt), COLOR_ON(t->character), COLOR_CHOICES(t->character));
  /* If exceeding the size of the buffer, truncate it for the overflow message */
  if (size < 0 || wantsize >= sizeof(txt)) {
    size = sizeof(txt) - 1;
    strcpy(txt + size - strlen(text_overflow), text_overflow); /* strcpy: OK */
  }

  /*
   * If the text is too big to fit into even a large buffer, truncate
   * the new text to make it fit.  (This will switch to the overflow
   * state automatically because t->bufspace will end up 0.)
   */
  if (size + t->bufptr + 1 > LARGE_BUFSIZE) {
    size      = LARGE_BUFSIZE - t->bufptr - 1;
    txt[size] = '\0';
    if (t->character)
      char_set_overf(t->character, true);
    buf_overflows++;
  }

  /*
   * If we have enough space, just write to buffer and that's it! If the
   * text just barely fits, then it's switched to a large buffer instead
   * (unless it already is the large buffer: then it fills it, and we are in
   * the overflow state).
   */
  if (t->bufspace > size || (t->large_outbuf && t->bufspace == size)) {
    strcpy(t->output + t->bufptr, txt); /* strcpy: OK (size checked above) */
    t->bufspace -= size;
    t->bufptr += size;
    return (t->bufspace);
  }

  buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (bufpool != nullptr) {
    t->large_outbuf = bufpool;
    bufpool         = bufpool->next;
  } else { /* else create a new one */
    CREATE(t->large_outbuf, struct txt_block, 1);
    CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
    buf_largecount++;
  }

  strcpy(t->large_outbuf->text, t->output); /* strcpy: OK (size checked previously) */
  t->output = t->large_outbuf->text;        /* make big buffer primary */
  strcat(t->output, txt);                   /* strcat: OK (size checked) */

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);

  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;

  return (t->bufspace);
}

void free_bufpool(void)
{
  struct txt_block *tmp;

  while (bufpool) {
    tmp = bufpool->next;
    if (bufpool->text)
      free(bufpool->text);
    free(bufpool);
    bufpool = tmp;
  }
}

/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */

/*
 * get_bind_addr: Return a struct in_addr that should be used in our
 * call to bind().  If the user has specified a desired binding
 * address, we try to bind to it; otherwise, we bind to INADDR_ANY.
 * Note that inet_aton() is preferred over inet_addr() so we use it if
 * we can.  If neither is available, we always bind to INADDR_ANY.
 */

static struct in_addr *get_bind_addr()
{
  static struct in_addr bind_addr;

  /* Clear the structure */
  memset((char *)&bind_addr, 0, sizeof(bind_addr));

  /* If DLFT_IP is unspecified, use INADDR_ANY */
  if (CONFIG_DFLT_IP == nullptr) {
    bind_addr.s_addr = htonl(INADDR_ANY);
  } else {
    /* If the parsing fails, use INADDR_ANY */
    if (!inet_aton(CONFIG_DFLT_IP, &bind_addr)) {
      mud_log("SYSERR: DFLT_IP of %s appears to be an invalid IP address",
              CONFIG_DFLT_IP);
      bind_addr.s_addr = htonl(INADDR_ANY);
    }
  }

  /* Put the address that we've finally decided on into the logs */
  if (bind_addr.s_addr == htonl(INADDR_ANY))
    mud_log("Binding to all IP interfaces on this host.");
  else
    mud_log("Binding only to IP address %s", inet_ntoa(bind_addr));

  return (&bind_addr);
}

/* Sets the kernel's send buffer size for the descriptor */
static int set_sendbuf(int s)
{
  int opt = MAX_SOCK_BUF;

  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt SNDBUF");
    return (-1);
  }

  return (0);
}

/* Initialize a descriptor */
static void init_descriptor(struct descriptor_data *newd, int desc)
{
  static int last_desc = 0; /* last descriptor number */

  newd->descriptor = desc;
  newd->output     = newd->small_outbuf;
  newd->bufspace   = SMALL_BUFSIZE - 1;
  newd->login_time = time(0);
  newd->has_prompt = 1; /* prompt is part of greetings */
  desc_set_connected(newd, CON_GET_USER);
  CREATE(newd->history, char *, HISTORY_SIZE);
  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;
}

static void set_color(struct descriptor_data *d)
{
  if (d->character == nullptr) {
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;
  }
  char_prf_set(d->character, PRF_COLOR, true);
  write_to_output(d, "%s", GREETANSI);
  write_to_output(d, "\r\n@w                  Welcome to Dragonball Advent Truth\r\n");
  write_to_output(d, "@D                 ---(@CPeak Logon Count Today@W: @w%4d@D)---@n\r\n", PCOUNT);
  write_to_output(d, "@D                 ---(@CHighest Logon Count   @W: @w%4d@D)---@n\r\n", HIGHPCOUNT);
  write_to_output(d, "@D                 ---(@CTotal Era %d Characters@W: @w%4s@D)---@n\r\n", CURRENT_ERA, add_commas(ERAPLAYERS));
  write_to_output(d, "\r\n@cEnter your desired username or the username you have already made.\n@CEnter Username:@n\r\n");
  d->user  = strdup("Empty");
  d->pass  = strdup("Empty");
  d->email = strdup("Empty");
  d->tmp1  = strdup("Empty");
  d->tmp2  = strdup("Empty");
  d->tmp3  = strdup("Empty");
  d->tmp4  = strdup("Empty");
  d->tmp5  = strdup("Empty");
  return;
}

static int new_descriptor(int s)
{
  int desc;
  int sockets_connected = 0;
  socklen_t i;
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  int from_same_ip = 0, keepalive = 1;

  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *)&peer, &i)) == INVALID_SOCKET) {
    perror("SYSERR: accept");
    return (-1);
  }
  /* keep it from blocking */
  nonblock(desc);

  /* set the send buffer size */
  if (set_sendbuf(desc) < 0) {
    close(desc);
    return (0);
  }
  /* notice connections that silently went away (NAT timeouts, crashed clients) */
  if (setsockopt(desc, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0)
    perror("SYSERR: setsockopt SO_KEEPALIVE");

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= CONFIG_MAX_PLAYING) {
    write_to_descriptor(desc, "Sorry, CircleMUD is full right now... please try again later!\r\n");
    close(desc);
    return (0);
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);

  /* The site is the numeric address. No reverse DNS: the lookup blocks the whole game, and the name it
     returns is chosen by whoever controls the address, so it is no basis for bans. */
  if (!inet_ntop(AF_INET, &peer.sin_addr, newd->host, sizeof(newd->host)))
    strcpy(newd->host, "unknown");

  /* one host may not take all the slots */
  for (struct descriptor_data *other = descriptor_list; other; other = other->next)
    if (!strcmp(other->host, newd->host))
      from_same_ip++;
  if (from_same_ip >= MAX_CONNECTIONS_PER_IP) {
    write_to_descriptor(desc, "Too many connections from your address. Please close some and try again.\r\n");
    close(desc);
    mudlog(LOG_CMP, ADMLVL_GOD, true, "Connection from [%s] refused: already %d connections from it", newd->host, from_same_ip);
    free(newd);
    return (0);
  }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
    close(desc);
    mudlog(LOG_CMP, ADMLVL_GOD, true, "Connection attempt denied from [%s]", newd->host);
    free(newd);
    return (0);
  }

  /* initialize descriptor data */
  init_descriptor(newd, desc);

  /* prepend to list */
  newd->next      = descriptor_list;
  descriptor_list = newd;

  set_color(newd);

  return (0);
}

/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 *
 * 32 int8_tGARBAGE_SPACE in MAX_SOCK_BUF used for:
 *	 2 bytes: prepended \r\n
 *	14 bytes: overflow message
 *	 2 bytes: extra \r\n for non-comapct
 *      14 bytes: unused
 */
static int process_output(struct descriptor_data *t)
{
  char i[MAX_SOCK_BUF], *osb = i + 2;
  int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n"); /* strcpy: OK (for 'MAX_SOCK_BUF >= 3') */

  /* now, append the 'real' output */
  strcpy(osb, t->output); /* strcpy: OK (t->output:LARGE_BUFSIZE < osb:MAX_SOCK_BUF-2) */

  /* if we're in the overflow state, notify the user */
  if (t->bufspace == 0)
    strcat(osb, "**OVERFLOW**\r\n"); /* strcpy: OK (osb:MAX_SOCK_BUF-2 reserves space) */

  /* add the extra CRLF if the person isn't in compact mode */
  if (desc_get_connected(t) == CON_PLAYING && t->character && !char_is_npc(t->character) && !char_prf_flagged(t->character, PRF_COMPACT))
    strcat(osb, "\r\n"); /* strcpy: OK (osb:MAX_SOCK_BUF-2 reserves space) */

  /* add a prompt */

  strcat(i, make_prompt(t)); /* strcpy: OK (i:MAX_SOCK_BUF reserves space) */
  if (desc_get_connected(t) == CON_PLAYING) {
    proc_colors(i, sizeof(i), COLOR_ON(t->character), COLOR_CHOICES(t->character));
  }

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (t->has_prompt) {
    t->has_prompt = false;
    result        = write_to_descriptor(t->descriptor, i);
    if (result >= 2)
      result -= 2;
  } else
    result = write_to_descriptor(t->descriptor, osb);

  if (result < 0) { /* Oops, fatal error. Bye! */
    close_socket(t);
    return (-1);
  } else if (result == 0) /* Socket buffer full. Try later. */
    return (0);

  /* Handle snooping: prepend "% " and send to snooper. */
  if (t->snoop_by)
    write_to_output(t->snoop_by, "\nvvvvvvvvvvvvv[Snoop]vvvvvvvvvvvvv\n%s\n^^^^^^^^^^^^^[Snoop]^^^^^^^^^^^^^\n", t->output);
  /* The common case: all saved output was handed off to the kernel buffer. */
  if (result >= t->bufptr) {
    /*
     * if we were using a large buffer, put the large buffer on the buffer pool
     * and switch back to the small one
     */
    if (t->large_outbuf) {
      t->large_outbuf->next = bufpool;
      bufpool               = t->large_outbuf;
      t->large_outbuf       = nullptr;
      t->output             = t->small_outbuf;
    }
    /* reset total bufspace back to that of a small buffer */
    t->bufspace  = SMALL_BUFSIZE - 1;
    t->bufptr    = 0;
    *(t->output) = '\0';

    /*
     * If the overflow message or prompt were partially written, try to save
     * them. There will be enough space for them if this is true.  'result'
     * is effectively unsigned here anyway.
     */
    if ((unsigned int)result < strlen(osb)) {
      size_t savetextlen = strlen(osb + result);

      strcat(t->output, osb + result);
      t->bufptr -= savetextlen;
      t->bufspace += savetextlen;
    }

  } else {
    /* Not all data in buffer sent.  result < output buffersize. */

    memmove(t->output, t->output + result, strlen(t->output + result) + 1); /* overlapping, so not strcpy */
    t->bufptr -= result;
    t->bufspace += result;
  }

  return (result);
}

/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

/* perform_socket_write for all Non-Windows platforms */
static ssize_t perform_socket_write(int desc, const char *txt, size_t length)
{
  ssize_t result = 0;

  do
    result = write(desc, txt, length);
  while (result < 0 && errno == EINTR);

  if (result > 0) {
    /* Write was successful. */
    return (result);
  }

  if (result == 0) {
    /* This should never happen! */
    mud_log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return (-1);
  }

  /*
   * result < 0, so an error was encountered - is it transient?
   * Unfortunately, different systems use different constants to
   * indicate this.
   */

  if (errno == EAGAIN) /* EWOULDBLOCK is the same value on Linux */
    return (0);

  /* Looks like the error was fatal.  Too bad. */
  return (-1);
}

/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level write() until all
 * the text has been delivered to the OS, or until an error is
 * encountered.
 *
 * Returns:
 * >=0  If all is well and good.
 *  -1  If an error was encountered, so that the player should be cut off.
 */
int write_to_descriptor(int desc, const char *txt)
{
  ssize_t bytes_written;
  size_t total = strlen(txt), write_total = 0;

  while (total > 0) {
    bytes_written = perform_socket_write(desc, txt, total);

    if (bytes_written < 0) {
      /* Fatal error.  Disconnect the player. */
      perror("SYSERR: Write to socket");
      return (-1);
    }
    if (bytes_written == 0) {
      /* Temporary failure -- socket buffer full. */
      return (write_total);
    }
    txt += bytes_written;
    total -= bytes_written;
    write_total += bytes_written;
  }

  return (write_total);
}

/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
static ssize_t perform_socket_read(int desc, char *read_point, size_t space_left)
{
  ssize_t ret;

  ret = read(desc, read_point, space_left);

  /* Read was successful. */
  if (ret > 0)
    return (ret);

  /* read() returned 0, meaning we got an EOF. */
  if (ret == 0) {
    mud_log("WARNING: EOF on socket read (connection broken by peer)");
    return (-1);
  }

  /*
   * read returned a value < 0: there was an error
   */

  if (errno == EINTR)
    return (0);

  if (errno == EAGAIN) /* EWOULDBLOCK is the same value on Linux */
    return (0);

  if (errno == ECONNRESET)
    return (-1);
  /*
   * We don't know what happened, cut them off. This qualifies for
   * a SYSERR because we have no idea what happened at this point.
   */
  perror("SYSERR: perform_socket_read: about to lose connection");
  return (-1);
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 *
 * Ever wonder why 'tmp' had '+8' on it?  The crusty old code could write
 * MAX_INPUT_LENGTH+1 bytes to 'tmp' if there was a '$' as the final
 * character in the input buffer.  This would also cause 'space_left' to
 * drop to -1, which wasn't very happy in an unsigned variable.  Argh.
 * So to fix the above, 'tmp' lost the '+8' since it doesn't need it
 * and the code has been changed to reserve space by accepting one less
 * character. (Do you really need 256 characters on a line?)
 * -gg 1/21/2000
 */
static int process_input(struct descriptor_data *t)
{
  int buf_length, failed_subst;
  ssize_t bytes_read;
  size_t space_left;
  char *ptr, *read_point, *write_point, *nl_pos = nullptr;
  char tmp[MAX_INPUT_LENGTH];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

  do {
    if (space_left <= 0) {
      mud_log("WARNING: process_input: about to close connection: input overflow");
      return (-1);
    }

    bytes_read = perform_socket_read(t->descriptor, read_point, space_left);

    if (bytes_read < 0) /* Error, disconnect them. */
      return (-1);
    else if (bytes_read == 0) /* Just blocking, no problems. */
      return (0);

    /* check for compression response, if still expecting something */
    /* note: this will bork if the user is giving lots of input when he first connects */
    /* he shouldn't be doing this, and for the sake of efficiency, the read buffer isn't searched */
    /* (ie. it assumes that read_point[0] will be IAC, etc.) */

    *(read_point + bytes_read) = '\0'; /* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
        nl_pos = ptr;

    read_point += bytes_read;
    space_left -= bytes_read;

    /*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
  } while (nl_pos == nullptr);

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != nullptr) {
    write_point = tmp;
    space_left  = MAX_INPUT_LENGTH - 1;

    /* The '> 1' reserves room for a '$ => $$' expansion. */
    for (ptr = read_point; (space_left > 1) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b' || *ptr == 127) { /* handle backspacing or delete key */
        if (write_point > tmp) {
          if (*(--write_point) == '$') {
            write_point--;
            space_left += 2;
          } else
            space_left++;
        }
      } else if (isascii(*ptr) && isprint(*ptr)) {
        if ((*(write_point++) = *ptr) == '$') { /* copy one character */
          *(write_point++) = '$';               /* if it's a $, double it */
          space_left -= 2;
        } else
          space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      snprintf(buffer, sizeof(buffer), "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer) < 0)
        return (-1);
    }
    if (t->snoop_by)
      write_to_output(t->snoop_by, "%% %s\r\n", tmp);
    failed_subst = 0;

    if (*tmp == '!' && !(*(tmp + 1))) /* Redo last command. */
      strcpy(tmp, t->last_input);     /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
    else if (*tmp == '!' && *(tmp + 1)) {
      char *commandln  = (tmp + 1);
      int starting_pos = t->history_pos,
          cnt          = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

      skip_spaces(&commandln);
      for (; cnt != starting_pos; cnt--) {
        if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
          strcpy(tmp, t->history[cnt]); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
          strcpy(t->last_input, tmp);   /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
          write_to_output(t, "%s\r\n", tmp);
          break;
        }
        if (cnt == 0) /* At top, loop to bottom. */
          cnt = HISTORY_SIZE;
      }
    } else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
        strcpy(t->last_input, tmp); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
    } else {
      strcpy(t->last_input, tmp); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
      if (t->history[t->history_pos])
        free(t->history[t->history_pos]);       /* Clear the old line. */
      t->history[t->history_pos] = strdup(tmp); /* Save the new. */
      if (++t->history_pos >= HISTORY_SIZE)     /* Wrap to top. */
        t->history_pos = 0;
    }
    /* the '--' command flushes the queue - Jamdog - 9th May 2007 */
    if ((*tmp == '-') && (*(tmp + 1) == '-') && !(*(tmp + 2))) {
      write_to_output(t, "All queued commands cancelled.\r\n");
      flush_input_queue(t); /* only the commands: pending output stays */
      failed_subst = 1;     /* and don't queue the -- itself */
    }
    if (!failed_subst) {
      /* commands run at most one per pulse; a client sending far faster is flooding */
      if (t->input.typed >= MAX_QUEUED_INPUT) {
        write_to_descriptor(t->descriptor, "\r\nToo many commands waiting to run; disconnecting.\r\n");
        mudlog(LOG_CMP, ADMLVL_GOD, true, "Input flood from [%s]: more than %d commands queued; disconnecting.", t->host, MAX_QUEUED_INPUT);
        return (-1);
      }
      write_to_q(tmp, &t->input, 0);
    }

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = nullptr; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
        nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return (1);
}

/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
static bool perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
  char newsub[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    write_to_output(t, "Invalid substitution.\r\n");
    return true;
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    write_to_output(t, "Invalid substitution.\r\n");
    return true;
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(newsub, orig, strpos - orig); /* strncpy: OK (newsub:MAX_INPUT_LENGTH+5 > orig:MAX_INPUT_LENGTH) */
  newsub[strpos - orig] = '\0';

  /* now, the replacement string */
  strncat(newsub, second, MAX_INPUT_LENGTH - strlen(newsub) - 1); /* strncpy: OK */

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(newsub, strpos + strlen(first), MAX_INPUT_LENGTH - strlen(newsub) - 1); /* strncpy: OK */

  /* terminate the string in case of an overflow from strncat */
  newsub[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, newsub); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */

  return false;
}

/* Frees the account strings: real ones after login, or the "Empty" placeholders set_color() made. */
static void free_user(struct descriptor_data *d)
{
  if (d->user_freed == 1) {
    return;
  }
  d->user_freed = 1;

  if (d->user && strcasecmp(d->user, "Empty"))
    mud_log("Freeing User: %s", d->user);

  free(d->user);
  free(d->pass);
  free(d->email);
  for (int n = 1; n <= MAX_ACCOUNT_SLOTS; n++) {
    char **slot = account_slot(d, n);
    free(*slot);
    *slot = nullptr;
  }
  d->user = d->pass = d->email = nullptr;
}

void close_socket(struct descriptor_data *d)
{
  struct descriptor_data *temp;

  REMOVE_FROM_LIST(d, descriptor_list, next, temp);
  close(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = nullptr;

  if (d->snoop_by) {
    write_to_output(d->snoop_by, "Your victim is no longer among us.\r\n");
    d->snoop_by->snooping = nullptr;
  }

  if (d->character) {
    /* If we're switched, this resets the mobile taken. */
    d->character->desc = nullptr;

    /* Plug memory leak, from Eric Green. */
    if (!char_is_npc(d->character) && char_plr_flagged(d->character, PLR_MAILING) && d->str) {
      if (*(d->str))
        free(*(d->str));
      free(d->str);
      d->str = nullptr;
    } else if (d->backstr && !char_is_npc(d->character) && !char_plr_flagged(d->character, PLR_WRITING)) {
      free(d->backstr); /* editing description ... not olc */
      d->backstr = nullptr;
    }
    if (desc_is_playing(d) || desc_get_connected(d) == CON_DISCONNECT) {
      struct char_data *link_challenged = d->original ? d->original : d->character;

      /* We are guaranteed to have a person. */
      act("$n has lost $s link.", true, link_challenged, 0, 0, TO_ROOM);
      save_char(link_challenged);
      mudlog(LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(link_challenged)), true, "Closing link to: %s.", char_get_name(link_challenged));
    } else {
      free_char(d->character);
      d->character = nullptr; /* (free_char() can't clear it: ->desc was cut above) */
    }
  } else
    mudlog(LOG_CMP, ADMLVL_IMMORT, true, "Losing descriptor without char.");

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = nullptr;

  /* Clear the command history. */
  if (d->history) {
    int cnt;
    for (cnt = 0; cnt < HISTORY_SIZE; cnt++)
      if (d->history[cnt])
        free(d->history[cnt]);
    free(d->history);
  }

  if (d->obj_name)
    free(d->obj_name);
  if (d->obj_short)
    free(d->obj_short);
  if (d->obj_long)
    free(d->obj_long);

  free_user(d);
  free(d->loadplay); /* the account slot being loaded, if any */
  d->loadplay = nullptr;

  /*. Kill any OLC stuff (every editor, and character creation's stat points, keep theirs in d->olc) .*/
  if (d->olc)
    cleanup_olc(d, CLEANUP_ALL);

  free(d);
}

static void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (desc_get_connected(d) != CON_PASSWORD && desc_get_connected(d) != CON_GET_EMAIL && desc_get_connected(d) != CON_NEWPASSWD)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      continue;
    }
    write_to_output(d, "\r\nTimed out... goodbye.\r\n");
    desc_set_connected(d, CON_CLOSE);
  }
}

static void check_idle_menu(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (desc_get_connected(d) != CON_MENU && desc_get_connected(d) != CON_GET_USER && desc_get_connected(d) != CON_UMENU)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      write_to_output(d, "\r\nYou are about to be disconnected due to inactivity in 60 seconds.\r\n");
      continue;
    }
    write_to_output(d, "\r\nTimed out... goodbye.\r\n");
    desc_set_connected(d, CON_CLOSE);
  }
}

/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

static void nonblock(int s)
{
  int flags;

  flags = fcntl(s, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

/* ******************************************************************
*  signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

static void reread_wizlists(int sig)
{
  reread_wizlist = true;
}

static void unrestrict_game(int sig)
{
  emergency_unban = true;
}

/* clean up our zombie kids to avoid defunct processes */
static void reap(int sig)
{
  while (waitpid(-1, nullptr, WNOHANG) > 0);

  signal(SIGCHLD, reap);
}

/* Dying anyway... */
static void checkpointing(int sig)
{
#ifndef MEMORY_DEBUG
  if (!tics_passed) {
    mud_log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
    abort();
  } else
    tics_passed = 0;
#endif
}

/* A fatal signal (crash, abort, or the deadlock checkpoint) is being delivered: write which one and a
 * backtrace to the log using only async-signal-safe calls, then re-raise it with the default action so
 * the process still dies and dumps core as before. */
static void fatal_signal(int sig)
{
  char msg[48] = "SYSERR: fatal signal ";
  size_t len   = strlen(msg);
  char digits[12];
  int nd = 0;

  for (int v = sig; v > 0 && nd < (int)sizeof(digits); v /= 10)
    digits[nd++] = '0' + v % 10;
  while (nd > 0 && len < sizeof(msg) - 2)
    msg[len++] = digits[--nd];
  msg[len++] = '\n';

  int fd = logfile ? fileno(logfile) : STDERR_FILENO;
  if (write(fd, msg, len) < 0) { /* nothing else we can do */
  }
  log_backtrace();

  signal(sig, SIG_DFL);
  raise(sig);
}

/* SIGHUP, SIGINT, SIGTERM: ask the game loop to shut down cleanly (saving everyone). Only sets a flag:
 * logging or exiting from inside a signal handler is not safe. */
static void hupsig(int sig)
{
  shutdown_signal = sig;
}

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

static void signal_setup(void)
{
  struct itimerval itime;
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.
   */
  interval.tv_sec   = 180;
  interval.tv_usec  = 0;
  itime.it_interval = interval;
  itime.it_value    = interval;
  setitimer(ITIMER_VIRTUAL, &itime, nullptr);
  signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  signal(SIGHUP, hupsig);
  signal(SIGCHLD, reap);
  signal(SIGINT, hupsig);
  signal(SIGTERM, hupsig);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGALRM, SIG_IGN);

  /* crashes: log a backtrace, then die (and dump core) as before */
  {
    void *warm[1];
    backtrace(warm, 1); /* load the unwinder now, so the handler doesn't have to allocate */
  }
  signal(SIGSEGV, fatal_signal);
  signal(SIGBUS, fatal_signal);
  signal(SIGFPE, fatal_signal);
  signal(SIGILL, fatal_signal);
  signal(SIGABRT, fatal_signal);

  /* allow core dumps up to the hard limit */
  struct rlimit core;
  if (getrlimit(RLIMIT_CORE, &core) == 0) {
    core.rlim_cur = core.rlim_max;
    setrlimit(RLIMIT_CORE, &core);
  }
}

/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */

int arena_watch(struct char_data *ch)
{

  struct descriptor_data *d;
  int found = false, room = NOWHERE;

  for (d = descriptor_list; d; d = d->next) {
    if (desc_get_connected(d) != CON_PLAYING)
      continue;

    if (char_in_arena(d->character)) {
      if (char_get_arenawatch(ch) == char_get_idnum(d->character)) {
        found = true;
        room  = rroom_get_vnum(char_get_in_room(d->character));
      }
    }
  }

  if (found == false) {
    char_prf_set(ch, PRF_ARENAWATCH, false);
    char_set_arenawatch(ch, -1);
    return (NOWHERE);
  }
  return (room);
}

void send_to_all(const char *messg, ...)
{
  struct descriptor_data *i;
  va_list args;

  if (messg == nullptr)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING)
      continue;

    va_start(args, messg);
    vwrite_to_output(i, messg, args);
    va_end(args);
  }
}

void send_to_outdoor(const char *messg, ...)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    va_list args;

    if (desc_get_connected(i) != CON_PLAYING || i->character == nullptr)
      continue;
    if (!char_is_awake(i->character) || !char_is_outside(i->character))
      continue;

    va_start(args, messg);
    vwrite_to_output(i, messg, args);
    va_end(args);
  }
}

void send_to_moon(const char *messg, ...)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    va_list args;

    if (desc_get_connected(i) != CON_PLAYING || i->character == nullptr)
      continue;
    if (!char_is_awake(i->character) || !char_has_moon(i->character))
      continue;

    va_start(args, messg);
    vwrite_to_output(i, messg, args);
    va_end(args);
  }
}

void send_to_planet(int type, int planet, const char *messg, ...)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    va_list args;

    if (desc_get_connected(i) != CON_PLAYING || i->character == nullptr)
      continue;
    if (!char_is_awake(i->character) || !rroom_flagged(char_get_in_room(i->character), planet))
      continue;
    else {
      if (type == 0) {
        va_start(args, messg);
        vwrite_to_output(i, messg, args);
        va_end(args);
      } else if (char_is_outside(i->character) && char_get_skill(i->character, SKILL_SPOT) >= axion_dice(-5)) {
        va_start(args, messg);
        vwrite_to_output(i, messg, args);
        va_end(args);
      }
    }
  }
}

void send_to_room(room_rnum room, const char *messg, ...)
{
  struct char_data *i;
  va_list args;

  if (messg == nullptr)
    return;

  for (i = world[room].people; i; i = i->next_in_room) {
    if (!i->desc)
      continue;

    va_start(args, messg);
    vwrite_to_output(i->desc, messg, args);
    va_end(args);
  }

  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    if (desc_get_connected(d) != CON_PLAYING)
      continue;

    if (char_prf_flagged(d->character, PRF_ARENAWATCH)) {
      if (arena_watch(d->character) == room) {
        char buf[2000];
        *buf = '\0';
        sprintf(buf, "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n", messg);
        va_start(args, messg);
        vwrite_to_output(d, buf, args);
        va_end(args);
      }
    }
    if (char_get_listenroom(d->character) > 0) {
      int roll = rand_number(1, 101);
      if (char_get_listenroom(d->character) == room && char_get_skill(d->character, SKILL_EAVESDROP) > roll) {
        char buf[1000];
        *buf = '\0';
        sprintf(buf, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", messg);
        va_start(args, messg);
        vwrite_to_output(d, buf, args);
        va_end(args);
      }
    }
  }
}

static const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression)  \
  if ((pointer) == nullptr) i = ACTNULL; \
  else i = (expression);

/* higher-level communication: the act() function */
static void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj, struct char_data *to)
{
  const char *i = nullptr;
  char lbuf[MAX_STRING_LENGTH], *buf, *j;
  bool uppercasenext                = false;
  const struct char_data *dg_victim = nullptr;
  const struct obj_data *dg_target  = nullptr;
  const char *dg_arg                = nullptr;

  buf = lbuf;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
        case 'n':
          i = char_pers(ch, to);
          break;
        case 'N':
          CHECK_NULL(vict_obj, char_pers(((struct char_data *)vict_obj), to));
          dg_victim = (const struct char_data *)vict_obj;
          break;
        case 'm':
          i = char_hmhr(ch);
          break;
        case 'M':
          CHECK_NULL(vict_obj, char_hmhr((const struct char_data *)vict_obj));
          dg_victim = (const struct char_data *)vict_obj;
          break;
        case 's':
          i = char_hshr(ch);
          break;
        case 'S':
          CHECK_NULL(vict_obj, char_hshr((const struct char_data *)vict_obj));
          dg_victim = (const struct char_data *)vict_obj;
          break;
        case 'e':
          i = char_hssh(ch);
          break;
        case 'E':
          CHECK_NULL(vict_obj, char_hssh((const struct char_data *)vict_obj));
          dg_victim = (const struct char_data *)vict_obj;
          break;
        case 'o':
          CHECK_NULL(obj, obj_fname_for(obj, to));
          break;
        case 'O':
          CHECK_NULL(vict_obj, obj_fname_for((const struct obj_data *)vict_obj, to));
          dg_target = (const struct obj_data *)vict_obj;
          break;
        case 'p':
          CHECK_NULL(obj, obj_shortdesc_for(obj, to));
          break;
        case 'P':
          CHECK_NULL(vict_obj, obj_shortdesc_for((const struct obj_data *)vict_obj, to));
          dg_target = (const struct obj_data *)vict_obj;
          break;
        case 'a':
          CHECK_NULL(obj, obj_sana(obj));
          break;
        case 'A':
          CHECK_NULL(vict_obj, obj_sana((const struct obj_data *)vict_obj));
          dg_target = (const struct obj_data *)vict_obj;
          break;
        case 'T':
          CHECK_NULL(vict_obj, (const char *)vict_obj);
          dg_arg = (const char *)vict_obj;
          break;
        case 't':
          CHECK_NULL(obj, (char *)obj);
          break;
        case 'F':
          CHECK_NULL(vict_obj, fname((const char *)vict_obj));
          break;
        /* uppercase previous word */
        case 'u':
          for (j = buf; j > lbuf && !isspace((int)*(j - 1)); j--);
          if (j != buf)
            *j = toupper(*j);
          i = "";
          break;
        /* uppercase next word */
        case 'U':
          uppercasenext = true;
          i             = "";
          break;
        case '$':
          i = "$";
          break;
        default:
          return;
          break;
      }
      while ((*buf = *(i++))) {
        if (uppercasenext && !isspace((int)*buf)) {
          *buf          = toupper(*buf);
          uppercasenext = false;
        }
        buf++;
      }
      orig++;
    } else if (!(*(buf++) = *(orig++))) {
      break;
    } else if (uppercasenext && !isspace((int)*(buf - 1))) {
      *(buf - 1)    = toupper(*(buf - 1));
      uppercasenext = false;
    }
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

  if (to->desc)
    write_to_output(to->desc, "%s", CAP(lbuf));

  if ((char_is_npc(to) && dg_act_check) && (to != ch))
    act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);

  if (last_act_message)
    free(last_act_message);
  last_act_message = strdup(lbuf);
}

/* act()/sub_write(): should ch receive this message? (to_sleeping: deliver even to sleeping characters) */
bool sendok(struct char_data *ch, int to_sleeping)
{
  return (ch->desc || script_has_type(ch->script, MTRIG_ACT)) && (to_sleeping || char_is_awake(ch)) && !char_plr_flagged(ch, PLR_WRITING);
}

char *act(const char *str, int hide_invisible, struct char_data *ch,
          struct obj_data *obj, const void *vict_obj, int type)
{
  struct char_data *to;
  int to_sleeping, res_sneak, res_hide, dcval = 0, resskill = 0;

  if (!str || !*str)
    return nullptr;

  /* Warning: the following TO_SLEEP code is a hack. I wanted to be able to tell
   * act to deliver a message regardless of sleep without adding an additional 
   * argument.  TO_SLEEP is 128 (a single bit high up).  It's ONLY legal to 
   * combine TO_SLEEP with one other TO_x command.  It's not legal to combine 
   * TO_x's with each other otherwise. TO_SLEEP only works because its value 
   * "happens to be" a single bit; do not change it to something else.  In 
   * short, it is a hack.  The same applies to TO_*RESIST.  */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((to_sleeping = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;

  if ((res_sneak = (type & TO_SNEAKRESIST)))
    type &= ~TO_SNEAKRESIST;

  if ((res_hide = (type & TO_HIDERESIST)))
    type &= ~TO_HIDERESIST;

  if (res_sneak && char_aff_flagged(ch, AFF_SNEAK)) {
    dcval = roll_skill(ch, SKILL_MOVE_SILENTLY); /* How difficult to counter? */
    if (char_get_skill(ch, SKILL_BALANCE))
      dcval += char_get_skill(ch, SKILL_BALANCE) / 10;
    if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 5 || char_get_genome(ch, 1) == 5)) {
      dcval += 10;
    }
    resskill = SKILL_SPOT; /* Skill used to resist      */
  } else if (res_hide && char_aff_flagged(ch, AFF_HIDE)) {
    dcval = roll_skill(ch, SKILL_HIDE);
    if (char_get_skill(ch, SKILL_BALANCE))
      dcval += char_get_skill(ch, SKILL_BALANCE) / 10;
    resskill = SKILL_SPOT;
  }

  /* this is a hack as well - DG_NO_TRIG is 256 -- Welcor */
  /* If the bit is set, unset dg_act_check, thus the ! below */
  if (!(dg_act_check = !IS_SET(type, DG_NO_TRIG)))
    REMOVE_BIT(type, DG_NO_TRIG);

  if (type == TO_CHAR) {
    if (ch && sendok(ch, to_sleeping) && (!resskill || (roll_skill(ch, resskill) >= dcval))) {
      perform_act(str, ch, obj, vict_obj, ch);
      return last_act_message;
    }
    return nullptr;
  }

  if (type == TO_VICT) {
    if ((to = (struct char_data *)vict_obj) != nullptr && sendok(to, to_sleeping) && (!resskill || (roll_skill(to, resskill) >= dcval))) {
      perform_act(str, ch, obj, vict_obj, to);
      return last_act_message;
    }
    return nullptr;
  }

  if (type == TO_GMOTE) {
    struct descriptor_data *i;
    char buf[MAX_STRING_LENGTH];
    for (i = descriptor_list; i; i = i->next) {
      if (!i->connected && i->character &&
          !char_prf_flagged(i->character, PRF_NOGOSS) &&
          !char_plr_flagged(i->character, PLR_WRITING) &&
          !rroom_flagged(char_get_in_room(i->character), ROOM_SOUNDPROOF)) {

        sprintf(buf, "@y%s@n", str);
        perform_act(buf, ch, obj, vict_obj, i->character);
        char buf2[MAX_STRING_LENGTH];
        snprintf(buf2, sizeof(buf2), "%s\r\n", buf);
        add_history(i->character, buf2, HIST_GOSSIP);
      }
    }
    return last_act_message;
  }

  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  if (ch && char_get_in_room(ch) != NOWHERE)
    to = world[char_get_in_room(ch)].people;
  else if (obj && obj_get_in_room(obj) != NOWHERE)
    to = world[obj_get_in_room(obj)].people;
  else {
    return nullptr;
  }

  if ((type & TO_ROOM)) {
    struct descriptor_data *d;

    for (d = descriptor_list; d; d = d->next) {
      if (desc_get_connected(d) != CON_PLAYING)
        continue;

      if (ch != nullptr) {
        if (char_in_arena(ch)) {
          if (char_prf_flagged(d->character, PRF_ARENAWATCH)) {
            if (arena_watch(d->character) == rroom_get_vnum(char_get_in_room(ch))) {
              char buf3[2000];
              *buf3 = '\0';
              sprintf(buf3, "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n", str);
              perform_act(buf3, ch, obj, vict_obj, d->character);
            }
          }
        }
      }
      if (char_get_listenroom(d->character) > 0) {
        int roll = rand_number(1, 101);
        if (!resskill || (roll_skill(d->character, resskill) >= dcval)) {
          if (ch != nullptr && char_get_listenroom(d->character) == rroom_get_vnum(char_get_in_room(ch)) && char_get_skill(d->character, SKILL_EAVESDROP) > roll) {
            char buf3[1000];
            *buf3 = '\0';
            sprintf(buf3, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", str);
            perform_act(buf3, ch, obj, vict_obj, d->character);
          } else if (obj != nullptr && char_get_listenroom(d->character) == rroom_get_vnum(obj_get_in_room(obj)) && char_get_skill(d->character, SKILL_EAVESDROP) > roll) {
            char buf3[1000];
            *buf3 = '\0';
            sprintf(buf3, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", str);
            perform_act(buf3, ch, obj, vict_obj, d->character);
          }
        }
      }
    }
  }

  for (; to; to = to->next_in_room) {
    if (!sendok(to, to_sleeping) || (to == ch))
      continue;
    if (hide_invisible && ch && !char_can_see(to, ch))
      continue;
    if (type != TO_ROOM && to == vict_obj)
      continue;
    if (resskill && roll_skill(to, resskill) < dcval)
      continue;
    perform_act(str, ch, obj, vict_obj, to);
  }
  return last_act_message;
}

/* Prefer the file over the descriptor. */
void setup_log(const char *filename, int fd)
{
  FILE *s_fp;

  s_fp = stderr;

  if (filename == nullptr || *filename == '\0') {
    /* No filename, set us up with the descriptor we just opened. */
    logfile = s_fp;
    puts("Using file descriptor for logging.");
    return;
  }

  /* We honor the default filename first. */
  if (open_logfile(filename, s_fp))
    return;

  /* Well, that failed but we want it logged to a file so try a default. */
  if (open_logfile("log/syslog", s_fp))
    return;

  /* Ok, one last shot at a file. */
  if (open_logfile("syslog", s_fp))
    return;

  /* Erp, that didn't work either, just die. */
  puts("SYSERR: Couldn't open anything to log to, giving up.");
  exit(1);
}

static bool open_logfile(const char *filename, FILE *stderr_fp)
{
  if (stderr_fp) /* freopen() the descriptor. */
    logfile = freopen(filename, "w", stderr_fp);
  else
    logfile = fopen(filename, "w");

  if (logfile) {
    printf("Using log file '%s'%s.\n",
           filename, stderr_fp ? " with redirection" : "");
    return (true);
  }

  printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
  return (false);
}

/* Orders a typed help topic against an entry's keywords: a prefix of them matches. */
static int help_prefix_order(const void *key, const void *elem)
{
  const char *entry = key;

  return strncasecmp(entry, ((const struct help_index_element *)elem)->keywords, strlen(entry));
}

void show_help(struct descriptor_data *t, const char *entry)
{
  struct help_index_element *found;

  if (!help_table)
    return;
  if (!(found = bsearch(entry, help_table, top_of_helpt + 1, sizeof(*help_table), help_prefix_order)))
    return;
  while (found > help_table && !help_prefix_order(entry, found - 1)) /* the first of the matches */
    found--;
  write_to_output(t, "\r\n%s\r\n", found->entry);
}

/* Thx to Jamie Nelson of 4D for this contribution */
void send_to_range(room_vnum start, room_vnum finish, const char *messg, ...)
{
  struct char_data *i;
  va_list args;
  int j;

  if (start > finish) {
    mud_log("send_to_range passed start room value greater then finish.");
    return;
  }
  if (messg == nullptr)
    return;

  for (j = 0; j < top_of_world; j++) {
    if (rroom_get_vnum(j) >= start && rroom_get_vnum(j) <= finish) {
      for (i = world[j].people; i; i = i->next_in_room) {
        if (!i->desc)
          continue;

        va_start(args, messg);
        vwrite_to_output(i->desc, messg, args);
        va_end(args);
      }
    }
  }
}
