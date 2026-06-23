/*************************************************************************
 *   File: comm.c                                        Part of CircleMUD *
 *  Usage: Communication, socket handling, main(), central game loop       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "consts/affflags.h"
#include "consts/constates.h"
#include "consts/maximums.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/songs.h"
#include "help.h"

#include "act.informative.h"
#include "act.item.h"
#include "act.misc.h"
#include "act.other.h"
#include "act.wizard.h"
#include "act.social.h"
#include "ban.h"
#include "ban_impl.h"
#include "boards.h"
#include "character_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "comm.h"
#include "config.h"
#include "config_db.h"
#include "consts/admlevel.h"
#include "consts/applies.h"
#include "consts/roomflags.h"
#include "consts/sex.h"
#include "consts/triggers.h"
#include "db.h"
#include "descriptor_db.h"
#include "descriptor_impl.h"
#include "descriptor_macros.h"
#include "dg_comm.h"
#include "dg_scripts.h"
#include "dgscript_impl.h"
#include "flags.h"
#include "handler.h"
#include "house.h"
#include "interpreter.h"
#include "magic.h"
#include "maputils.h"
#include "mobact.h"
#include "oasis.h"
#include "object_api.h"
#include "object_impl.h"
#include "object_macros.h"
#include "object_systems.h"
#include "random.h"
#include "room_api.h"
#include "room_db.h"
#include "stringutils.h"
#include "util_macros.h"
#include "weather.h"
#include "weather_db.h"

#include "iterate.hpp"

#include "extract.h"
#include "fileop.h"
#include "relocate.h"

#include "clan.h"
#include "class.h"
#include "combat.h"
#include "fight.h"
#include "genolc.h"
#include "local_limits.h"
#include "log.h"
#include "mail.h"
#include "modify.h"
#include "net.h"
#include "objsave.h"
#include "races_plus.h"
#include "screen.h"
#include "sensei.h"

#include "event_queue_api.h"

#include <cstdlib>
#include <cstring>
#include <time.h>

#include <arpa/inet.h>
#include <errno.h>
#include <linux/limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* externs */

int passcomm(struct char_data *ch, char *comm);

/* local globals */
struct txt_block *bufpool = 0; /* pool of large output buffers */
int buf_largecount = 0;        /* # of large buffers which exist */
int buf_overflows = 0;         /* # of overflows of output */
int buf_switches = 0;          /* # of switches from small to large buf */
int circle_shutdown = 0;       /* clean shutdown */
int circle_reboot = 0;         /* reboot the game after a shutdown */
int no_specials = 0;           /* Suppress ass. of special routines */
int max_players = 0;           /* max descriptors available */
int tics_passed = 0;           /* for extern checkpointing */
int scheck = 0;                /* for syntax checking mode */
struct timeval null_time;      /* zero-valued time structure */
int8_t reread_wizlist;         /* signal: SIGUSR1 */
int8_t emergency_unban;        /* signal: SIGUSR2 */
FILE *logfile = NULL;          /* Where to send the log messages. */
const char *text_overflow = "**OVERFLOW**\r\n";
int dg_act_check;        /* toggle for act_trigger */
unsigned long pulse = 0; /* number of pulses since game start */
bool fCopyOver;          /* Are we booting in copyover mode? */
uint16_t port;
char *last_act_message = NULL;

/***********************************************************************
 *  main game loop and related stuff                                    *
 ***********************************************************************/
int enter_player_game(struct descriptor_data *d);

void copyover_recover_begin(void) {
  mud_log("Copyover recovery initiated");
  PCOUNTDAY = time(0) + 60;
}

const char *copyover_descriptor_character_name(const struct descriptor_data *d) {
  if (!d || !d->character)
    return "";
  return GET_NAME(d->character);
}

int copyover_descriptor_saved_loadroom(const struct descriptor_data *d) {
  if (!d || !d->character)
    return 300;
  if (char_room_vnum_get(d->character) > 1)
    return char_room_vnum_get(d->character);
  if (char_room_vnum_get(d->character) <= 1 && GET_WAS_IN(d->character) > 1)
    return GET_WAS_IN(d->character);
  return 300;
}

void copyover_recover_descriptor(socklen_t desc, const char *name,
                                 const char *host, int saved_loadroom,
                                 const char *username,
                                 struct net_connection *conn) {
  struct descriptor_data *d;
  int player_i;
  bool fOld;
  int set_loadroom = NOWHERE;

  fOld = TRUE;

  /* Write something, and check if it goes error-free */
  if (write_to_descriptor(desc, "\n\rFolding initiated...\n\r") < 0) {
    if (conn)
      net_connection_destroy(conn);
    close(desc); /* nope */
    return;
  }

  /* create a new descriptor */
  CREATE(d, struct descriptor_data, 1);
  memset((char *)d, 0, sizeof(struct descriptor_data));
  init_descriptor(d, desc); /* set up various stuff */
  d->conn = conn;
  if (conn)
    net_connection_descriptor_set(conn, d);

  strncpy(d->host, host ? host : "", HOST_LENGTH);
  d->host[HOST_LENGTH] = '\0';
  d->next = descriptor_list;
  descriptor_list = d;

  d->connected = CON_CLOSE;

  /* Now, find the pfile */

  CREATE(d->character, struct char_data, 1);
  clear_char(d->character);
  d->character->desc = d;

  if ((player_i = load_char(name, d->character)) >= 0) {
    GET_PFILEPOS(d->character) = player_i;
    if (!PLR_FLAGGED(d->character, PLR_DELETED)) {
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRYO);
      userLoad(d, const_cast<char *>(username ? username : "Empty"));
    }
    /*else
      fOld = FALSE;*/
  } else
    fOld = FALSE;

  if (!fOld) /* Player file not found?! */ {
    write_to_descriptor(desc, "\n\rSomehow, your character was lost during "
                              "the folding. Sorry.\n\r");
    close_socket(d);
  } else {
    write_to_descriptor(desc, "\n\rFolding complete.\n\r");
    set_loadroom = GET_LOADROOM(d->character);
    GET_LOADROOM(d->character) = saved_loadroom;
    enter_player_game(d);
    GET_LOADROOM(d->character) = set_loadroom;
    d->connected = CON_PLAYING;
    look_at_room(char_room_get(d->character), d->character, 0);
    if (AFF_FLAGGED(d->character, AFF_HAYASA)) {
      char_condition_apply(d->character, "hayasa", "entry", "copyover_sync");
    }
  }
}

/* Reload players after a copyover */
void copyover_recover() {
  mud_log("Copyover recovery initiated");
  PCOUNTDAY = time(0) + 60;
  if (!net_copyover_recover(COPYOVER_FILE)) {
    mud_log("Copyover recovery failed. Exitting.\n\r");
    exit(1);
  }
}

void load_spacemap() {
  FILE *mapfile;
  int rowcounter, colcounter;
  int vnum_read;
  mud_log("Loading Space Map. ");

  // Load the map vnums from a file into an array
  mapfile = fopen("data/surface.map", "r");

  for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++) {
    for (colcounter = 0; colcounter <= MAP_COLS; colcounter++) {
      fscanf(mapfile, "%d", &vnum_read);
      mapnums[rowcounter][colcounter] = vnum_read;
    }
  }

  fclose(mapfile);
}

void load_race_sensei() {
  dbat::race::load_races();
  dbat::sensei::load_sensei();
}



int get_max_players(void) {

  int max_descs = 0;
  const char *method;

  /*
   * First, we'll try using getrlimit/setrlimit.  This will probably work
   * on most systems.  HAS_RLIMIT is defined in sysdep.h.
   */
  {
    struct rlimit limit;

    /* find the limit of file descs */
    method = "rlimit";
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
        max_descs, method);
    exit(1);
  }
  mud_log("   Setting player limit to %d using %s.", max_descs, method);
  return (max_descs);
}


static void connections_handle_commands() {
  char comm[MAX_INPUT_LENGTH];
  int aliased;

  struct descriptor_data *d, *next_d;
  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;

    if (!d->input.head)
      continue;

    struct char_data *ch = d->character;

    if (d->str) { /* Writing boards, mail, etc. */
      get_from_q(&d->input, comm, &aliased);
      string_add(d, comm);
      continue;
    }

    if (STATE(d) != CON_PLAYING) { /* In menus, login screens, etc. */
      get_from_q(&d->input, comm, &aliased);
      nanny(d, comm);
      continue;
    }

    if (!ch)
      continue;

    /* TRY LUA PCOMMANDS — bypass wait state entirely */
    if (char_pcommand_try(ch, d->input.head->text)) {
      get_from_q(&d->input, comm, &aliased); /* consume */
      d->has_prompt = FALSE;
      continue;
    }

    /* Player is actively sending a regular command: reset idle timer and
       pull character back from the void if needed. */
    ch->timer = 0;
    if (GET_WAS_IN(ch) != NOWHERE) {
      if (char_room_get(ch) != NULL)
        char_from_room(ch);
      char_to_room(ch, room_by_id(GET_WAS_IN(ch)));
      GET_WAS_IN(ch) = NOWHERE;
      act("$n has returned.", TRUE, ch, 0, 0, TO_ROOM);
    }

    get_from_q(&d->input, comm, &aliased);

    if (!aliased && perform_alias(d, comm, sizeof(comm)))
      get_from_q(&d->input, comm, &aliased);

    if (GET_WAIT_STATE(ch) == 0 && !char_command_has_pending(ch)) {
      /* Fast path: no wait, nothing queued — execute this tick, no delay. */
      d->has_prompt = FALSE;
      command_interpreter(ch, comm);
    } else {
      /* Deferred: wait_state active or prior commands queued. Don't touch
         has_prompt — nothing has executed yet so no prompt should fire. */
      char_command_enqueue(ch, comm);
    }
  }
}

void game_legacy_process_commands(void) { connections_handle_commands(); }

void game_legacy_send_outputs(void) {
  struct descriptor_data *d, *next_d;
  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (*(d->output)) {
      if (process_output(d) < 0) {
        mud_log("ERROR: Tried to send output to dead socket!");
      } else
        d->has_prompt = 1;
    }
  }

  for (d = descriptor_list; d; d = d->next) {
    if (!d->has_prompt) {
      write_to_output(d, "@n");
      d->has_prompt = TRUE;
    }
  }
}

static void connections_close_pending() {
  struct descriptor_data *d, *next_d;
  for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT)
        close_socket(d);
    }
}

void game_legacy_close_pending(void) { connections_close_pending(); }

void game_legacy_post_tick(void) {
  if (reread_wizlist) {
    reread_wizlist = FALSE;
    mudlog(CMP, ADMLVL_IMMORT, TRUE, "Signal received - rereading wizlists.");
    reboot_wizlists();
  }
  if (emergency_unban) {
    emergency_unban = FALSE;
    mudlog(BRF, ADMLVL_IMMORT, TRUE,
           "Received SIGUSR2 - completely unrestricting game (emergent)");
    ban_list = NULL;
    circle_restrict = 0;
    num_invalid = 0;
  }

  tics_passed++;
}

// --- Event queue handler wrappers ---
// Each ignores context (int, int64_t, int64_t) — these are global game-state events.

static void ev_wishSYS(int, int64_t, int64_t) { wishSYS(); }

static void ev_char_condition_update(int, int64_t, int64_t) {
  copyover_check();
}

static void ev_base_update(int, int64_t, int64_t) {
  base_update();
}

static void ev_script_trigger_check(int, int64_t, int64_t) {
  script_trigger_check();
}

static void ev_check_auction(int, int64_t, int64_t) { check_auction(); }
/* ev_handle_songs removed — song ticking moved to mystic_melody condition */
static void ev_check_idle_passwords(int, int64_t, int64_t) { check_idle_passwords(); }
static void ev_check_idle_menu(int, int64_t, int64_t) { check_idle_menu(); }
static void ev_fight_stack(int, int64_t, int64_t) { fight_stack(); }

static void ev_homing_huge_broken(int, int64_t, int64_t) {
  if (rand_number(1, 2) == 2) homing_update();
  huge_update();
  broken_update();
}

static void ev_mobile_activity(int, int64_t, int64_t) { mobile_activity(); }
static void ev_point_update(int, int64_t, int64_t) { point_update(); }

static void ev_weather_time_affects(int, int64_t, int64_t) {
  weather_and_time(1);
  check_time_triggers();
}

static void ev_autosave(int, int64_t, int64_t) {
  static int mins_since_crashsave = 0;
  if (!CONFIG_AUTO_SAVE) return;
  clan_update();
  if (++mins_since_crashsave >= CONFIG_AUTOSAVE_TIME) {
    mins_since_crashsave = 0;
    Crash_save_all();
    House_save_all();
  }
}

static void ev_record_usage(int, int64_t, int64_t) { record_usage(); }
static void ev_save_mud_time(int, int64_t, int64_t) { save_mud_time(&time_info); }

static void ev_process_character_commands(int, int64_t, int64_t) {
  char_iterate_all([](struct char_data *ch) {
    if (GET_WAIT_STATE(ch) > 0)
      GET_WAIT_STATE(ch)--;

    if (GET_WAIT_STATE(ch) == 0 && char_command_has_pending(ch)) {
      char *cmd = char_command_dequeue(ch);
      if (cmd) {
        if (ch->desc) ch->desc->has_prompt = FALSE;
        command_interpreter(ch, cmd);
        free(cmd);
      }
    }
    return true;
  });
}

void event_queue_register_heartbeat_events() {
  const int64_t now = event_queue_now_ms();

  // Fixed intervals (milliseconds)
  event_schedule_c(now + 100LL, 100LL, ev_process_character_commands, EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + EQ_MS_1SEC,  EQ_MS_1SEC,  ev_wishSYS,               EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + EQ_MS_1SEC,  EQ_MS_1SEC,  ev_char_condition_update, EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + EQ_MS_2SEC,  EQ_MS_2SEC,  ev_base_update,           EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + EQ_MS_15SEC, EQ_MS_15SEC, ev_check_auction,         EQ_CTX_NONE, 0, 0);
  /* ev_handle_songs removed — song ticking moved to mystic_melody condition */
  event_schedule_c(now + EQ_MS_1MIN,  EQ_MS_1MIN,  ev_check_idle_menu,       EQ_CTX_NONE, 0, 0);

  // Config-driven intervals (convert pulse count to ms: pulses * 100)
  const int64_t dg_ms     = (int64_t)PULSE_DG_SCRIPT * 100;
  const int64_t idle_ms   = (int64_t)PULSE_IDLEPWD    * 100;
  const int64_t mobile_ms = (int64_t)PULSE_MOBILE     * 100;
  const int64_t fight_ms  = idle_ms / 15;

  event_schedule_c(now + dg_ms,          dg_ms,          ev_script_trigger_check, EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + idle_ms,         idle_ms,         ev_check_idle_passwords, EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + fight_ms,        fight_ms,        ev_fight_stack,          EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + fight_ms * 2,    fight_ms * 2,    ev_homing_huge_broken,   EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + mobile_ms,       mobile_ms,       ev_mobile_activity,      EQ_CTX_NONE, 0, 0);

  // Mud-time intervals (SECS_PER_MUD_HOUR is in real seconds)
  const int64_t point_ms  = (int64_t)(SECS_PER_MUD_HOUR / 3) * 1000;
  const int64_t hour_ms   = (int64_t)SECS_PER_MUD_HOUR        * 1000;

  event_schedule_c(now + point_ms, point_ms, ev_point_update,         EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + hour_ms,  hour_ms,  ev_weather_time_affects, EQ_CTX_NONE, 0, 0);

  // Long-interval admin events
  const int64_t autosave_ms  = (int64_t)PULSE_AUTOSAVE  * 100;
  const int64_t usage_ms     = (int64_t)PULSE_USAGE      * 100;
  const int64_t timesave_ms  = (int64_t)PULSE_TIMESAVE   * 100;

  event_schedule_c(now + autosave_ms, autosave_ms, ev_autosave,      EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + usage_ms,    usage_ms,    ev_record_usage,  EQ_CTX_NONE, 0, 0);
  event_schedule_c(now + timesave_ms, timesave_ms, ev_save_mud_time, EQ_CTX_NONE, 0, 0);
}

/* ******************************************************************
 *  general utility stuff (for local use)                            *
 ****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
void timediff(struct timeval *rslt, struct timeval *a, struct timeval *b) {
  if (a->tv_sec < b->tv_sec)
    *rslt = null_time;
  else if (a->tv_sec == b->tv_sec) {
    if (a->tv_usec < b->tv_usec)
      *rslt = null_time;
    else {
      rslt->tv_sec = 0;
      rslt->tv_usec = a->tv_usec - b->tv_usec;
    }
  } else { /* a->tv_sec > b->tv_sec */
    rslt->tv_sec = a->tv_sec - b->tv_sec;
    if (a->tv_usec < b->tv_usec) {
      rslt->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
      rslt->tv_sec--;
    } else
      rslt->tv_usec = a->tv_usec - b->tv_usec;
  }
}

/*
 * Add 2 time values.
 *
 * Patch sent by "d. hall" <dhall@OOI.NET> to fix 'static' usage.
 */
void timeadd(struct timeval *rslt, struct timeval *a, struct timeval *b) {
  rslt->tv_sec = a->tv_sec + b->tv_sec;
  rslt->tv_usec = a->tv_usec + b->tv_usec;

  while (rslt->tv_usec >= 1000000) {
    rslt->tv_usec -= 1000000;
    rslt->tv_sec++;
  }
}

void record_usage(void) {
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (IS_PLAYING(d))
      sockets_playing++;
  }

  mud_log("nusage: %-3d sockets connected, %-3d sockets playing", sockets_connected,
      sockets_playing);
}

/* Append formatted text to a fixed-size prompt buffer, advancing len. */
static void papp(char *p, size_t max, size_t &len, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));
static void papp(char *p, size_t max, size_t &len, const char *fmt, ...) {
  if (len >= max) return;
  va_list ap;
  va_start(ap, fmt);
  int c = vsnprintf(p + len, max - len, fmt, ap);
  va_end(ap);
  if (c > 0) len += (size_t)c;
}

/* Status-flag bar line. Writes status tokens, then "@n\n" if any were written. */
static void prompt_status_flags(struct descriptor_data *d, struct char_data *ch,
                                  char *p, size_t max, size_t &len) {
  bool flagged = false;
#define PFLAG(fmt, ...) do { papp(p, max, len, fmt, ##__VA_ARGS__); flagged = true; } while(0)

  if (PLR_FLAGGED(ch, PLR_SELFD))
    PFLAG("@D[@RSELF-D@r: @w%s@D]@n",
          PLR_FLAGGED(ch, PLR_SELFD2) ? "READY" : "PREP");

  if (IS_HALFBREED(ch) && PRF_FLAGGED(ch, PRF_FURY)) {
    if (char_condition_has(ch, "halfbreed_fury"))
      PFLAG("@D[@mFury@W: @rENGAGED@D]@w");
    else
      PFLAG("@D[@mFury@W: @r%d@D]@w", GET_FURY(ch));
  }

  if (has_mail(GET_IDNUM(ch)) && !PRF_FLAGGED(ch, PRF_NMWARN))
    PFLAG("CHECK MAIL - ");

  if (GET_KAIOKEN(ch) > 0)
    PFLAG("KAIOKEN X%d - ", GET_KAIOKEN(ch));

  if (char_condition_has(ch, "mystic_melody")) {
    int64_t song_id = char_condition_number_get(ch, "mystic_melody", "song");
    if (song_id > 0 && song_id < 12)
      PFLAG("%s - ", song_types[(int)song_id]);
  }

  if (d->snooping && d->snooping->character)
    PFLAG("Snooping: (%s) - ", GET_NAME(d->snooping->character));

  if (DRAGGING(ch))
    PFLAG("Dragging: (%s) - ", GET_NAME(DRAGGING(ch)));

  if (PRF_FLAGGED(ch, PRF_BUILDWALK))
    PFLAG("BUILDWALKING - ");

  if (!PRF_FLAGGED(ch, PRF_NODEC)) {
    if (char_condition_has(ch, "flying")) PFLAG("FLYING - ");
    if (AFF_FLAGGED(ch, AFF_HIDE))       PFLAG("HIDING - ");
    if (PLR_FLAGGED(ch, PLR_SPAR))       PFLAG("SPARRING - ");
  }

  if (PLR_FLAGGED(ch, PLR_NOSHOUT))
    PFLAG("MUTED - ");

  {
    auto combo = char_condition_has(ch, "combo")
                   ? char_condition_number_get(ch, "combo", "state") : -1;
    const char *cname = nullptr;
    switch ((int)combo) {
    case 0:  cname = "Punch";      break;
    case 1:  cname = "Kick";       break;
    case 2:  cname = "Elbow";      break;
    case 3:  cname = "Knee";       break;
    case 4:  cname = "Roundhouse"; break;
    case 5:  cname = "Uppercut";   break;
    case 6:  cname = "Slam";       break;
    case 8:  cname = "Heeldrop";   break;
    case 51: cname = "Bash";       break;
    case 52: cname = "Headbutt";   break;
    case 56: cname = "Tailwhip";   break;
    }
    if (cname) PFLAG("Combo (%s) - ", cname);
  }

  if (!PRF_FLAGGED(ch, PRF_NODEC)) {
    if (PRF_FLAGGED(ch, PRF_AFK))             PFLAG("AFK - ");
    if (char_condition_has(ch, "fishing"))    PFLAG("FISHING -");
  }

#undef PFLAG
  if (flagged)
    papp(p, max, len, "@n\n");
}

/* Sitting / healing-tank / position-advantage lines. */
static void prompt_sitting_status(struct char_data *ch, char *p, size_t max,
                                    size_t &len) {
  if (PRF_FLAGGED(ch, PRF_NODEC)) return;
  if (SITS(ch) && PLR_FLAGGED(ch, PLR_HEALT))
    papp(p, max, len, "@c<@CFloating inside a healing tank@c>@n\r\n");
  else if (SITS(ch) && GET_POS(ch) == POS_SITTING)
    papp(p, max, len, "Sitting on: %s\r\n", SITS(ch)->short_description);
  else if (SITS(ch) && GET_POS(ch) == POS_RESTING)
    papp(p, max, len, "Resting on: %s\r\n", SITS(ch)->short_description);
  else if (SITS(ch) && GET_POS(ch) == POS_SLEEPING)
    papp(p, max, len, "Sleeping on: %s\r\n", SITS(ch)->short_description);
  if (AFF_FLAGGED(ch, AFF_POSITION))
    papp(p, max, len, "(Best Position)\r\n");
}

/* Ki charge bar in bar/percent/nodec display modes. */
static void prompt_charge_bar(struct char_data *ch, char *p, size_t max,
                                size_t &len) {
  if (GET_CHARGE(ch) < GET_MAX_MANA(ch) * .01 && GET_CHARGE(ch) > 0)
    char_charge_set(ch, 0);
  if (GET_CHARGE(ch) <= 0) return;

  int64_t charge = GET_CHARGE(ch), ki_max = GET_MAX_MANA(ch);

  if (PRF_FLAGGED(ch, PRF_NODEC)) {
    papp(p, max, len, "Ki is charged to %" I64T " percent.\n",
         (charge * 100) / ki_max);
    return;
  }
  if (PRF_FLAGGED(ch, PRF_DISPERC)) {
    papp(p, max, len, "@D[@BCharge@Y: @C%" I64T "%s@D]@n\n",
         (charge * 100) / ki_max, "%");
    return;
  }

  static const struct { double t; const char *bar; } charge_bars[] = {
    {1.00, "@CCharge @D[@G==@D<@RMAX@D>@G===@D]@n\n"},
    {0.95, "@CCharge @D[@G=========-@D]@n\n"},
    {0.90, "@CCharge @D[@G=========@g-@D]@n\n"},
    {0.85, "@CCharge @D[@G========-@g-@D]@n\n"},
    {0.80, "@CCharge @D[@G========@g--@D]@n\n"},
    {0.75, "@CCharge @D[@G=======-@g--@D]@n\n"},
    {0.70, "@CCharge @D[@G=======@g---@D]@n\n"},
    {0.65, "@CCharge @D[@G======-@g---@D]@n\n"},
    {0.60, "@CCharge @D[@G======@g----@D]@n\n"},
    {0.55, "@CCharge @D[@G=====-@g----@D]@n\n"},
    {0.50, "@CCharge @D[@G=====@g-----@D]@n\n"},
    {0.45, "@CCharge @D[@G====-@g-----@D]@n\n"},
    {0.40, "@CCharge @D[@G====@g------@D]@n\n"},
    {0.35, "@CCharge @D[@G===-@g------@D]@n\n"},
    {0.30, "@CCharge @D[@G===@g-------@D]@n\n"},
    {0.25, "@CCharge @D[@G==-@g-------@D]@n\n"},
    {0.20, "@CCharge @D[@G==@g--------@D]@n\n"},
    {0.15, "@CCharge @D[@G=-@g--------@D]@n\n"},
    {0.10, "@CCharge @D[@G=@g---------@D]@n\n"},
    {0.05, "@CCharge @D[@G-@g---------@D]@n\n"},
    {0.00, "@CCharge @D[@g----------@D]@n\n"},
  };
  for (auto &e : charge_bars) {
    if (charge >= ki_max * e.t) {
      papp(p, max, len, "%s", e.bar);
      break;
    }
  }
}

/* Barrier/sanctuary indicator in bar/percent/nodec display modes. */
static void prompt_barrier_bar(struct char_data *ch, char *p, size_t max,
                                 size_t &len) {
  if (AFF_FLAGGED(ch, AFF_FIRESHIELD))
    papp(p, max, len, "@D(@rF@RI@YR@rE@RS@YH@rI@RE@YL@rD@D)@n\n");

  if (!AFF_FLAGGED(ch, AFF_SANCTUARY)) return;

  int64_t barrier = GET_BARRIER(ch), ki_max = GET_MAX_MANA(ch);

  if (PRF_FLAGGED(ch, PRF_NODEC)) {
    if (barrier > 0)
      papp(p, max, len, "A barrier charged to %" I64T
                        " percent surrounds you.@n\n",
           (barrier * 100) / ki_max);
    return;
  }
  if (PRF_FLAGGED(ch, PRF_DISPERC)) {
    if (barrier > 0)
      papp(p, max, len, "@D[@GBarrier@Y: @B%" I64T "%s@D]@n\n",
           (barrier * 100) / ki_max, "%");
    return;
  }

  static const struct { double t; const char *bar; } barrier_bars[] = {
    {0.75, "@BBarrier @D[@C==MAX==@D]@n\n"},
    {0.70, "@BBarrier @D[@C=======@D]@n\n"},
    {0.65, "@BBarrier @D[@C======-@D]@n\n"},
    {0.60, "@BBarrier @D[@C======@c-@D]@n\n"},
    {0.55, "@BBarrier @D[@C=====-@c-@D]@n\n"},
    {0.50, "@BBarrier @D[@C=====@c--@D]@n\n"},
    {0.45, "@BBarrier @D[@C====-@c--@D]@n\n"},
    {0.40, "@BBarrier @D[@C====@c---@D]@n\n"},
    {0.35, "@BBarrier @D[@C===-@c---@D]@n\n"},
    {0.30, "@BBarrier @D[@C===@c----@D]@n\n"},
    {0.25, "@BBarrier @D[@C==-@c----@D]@n\n"},
    {0.20, "@BBarrier @D[@C==@c-----@D]@n\n"},
    {0.15, "@BBarrier @D[@C=-@c-----@D]@n\n"},
    {0.10, "@BBarrier @D[@C=@c------@D]@n\n"},
    {0.05, "@BBarrier @D[@C-@c------@D]@n\n"},
    {0.00, "@BBarrier @D[@C--Low-@D]@n\n"},
  };
  for (auto &e : barrier_bars) {
    if (barrier >= ki_max * e.t) {
      papp(p, max, len, "%s", e.bar);
      break;
    }
  }
}

/* Powerlevel display (value or percent mode). */
static void prompt_pl(struct char_data *ch, char *p, size_t max, size_t &len) {
  if (!PRF_FLAGGED(ch, PRF_DISPERC)) {
    const char *col = isWeightedPL(ch)            ? "m"
                    : getCurHealthPercent(ch) > .5 ? "c"
                    : getCurHealthPercent(ch) > .1 ? "y"
                    :                                "r";
    papp(p, max, len, "@D[@RPL@n@Y: @%s%s@D]@n", col, add_commas(getCurPL(ch)));
  } else {
    double perc = ((double)getCurHealth(ch) / (double)getMaxPL(ch)) * 100;
    const char *col = perc > 100 ? "g" : perc >= 70 ? "c"
                    : perc >= 51 ? "Y" : perc >= 20 ? "y" : "r";
    papp(p, max, len, "@D[@RPL@n@Y: @%s%d%s@D]@n", col, (int)perc, "@w%");
  }
}

/* Ki display (value or percent mode). */
static void prompt_ki(struct char_data *ch, char *p, size_t max, size_t &len) {
  int64_t cur = getCurKI(ch), ki_max = GET_MAX_MANA(ch);
  if (!PRF_FLAGGED(ch, PRF_DISPERC)) {
    const char *col = cur > ki_max / 2 ? "c" : cur > ki_max / 10 ? "y" : "r";
    papp(p, max, len, "@D[@CKI@Y: @%s%s@D]@n", col, add_commas(cur));
  } else {
    int64_t pw = std::max(cur, (int64_t)1), mx = std::max(ki_max, (int64_t)1);
    int perc = (int)((pw * 100) / mx);
    const char *col = perc > 100 ? "G" : perc >= 70 ? "c"
                    : perc >= 51 ? "Y" : perc >= 20 ? "y" : "r";
    papp(p, max, len, "@D[@CKI@n@Y: @%s%d%s@D]@n", col, perc, "@w%");
  }
}

/* Stamina display (value or percent mode). */
static void prompt_sta(struct char_data *ch, char *p, size_t max, size_t &len) {
  int64_t cur = getCurST(ch), st_max = GET_MAX_MOVE(ch);
  if (!PRF_FLAGGED(ch, PRF_DISPERC)) {
    const char *col = cur > st_max / 2 ? "c" : cur > st_max / 10 ? "y" : "r";
    papp(p, max, len, "@D[@GSTA@Y: @%s%s@D]@n", col, add_commas(cur));
  } else {
    int64_t pw = std::max(cur, (int64_t)1), mx = std::max(st_max, (int64_t)1);
    int perc = (int)((pw * 100) / mx);
    const char *col = perc > 100 ? "G" : perc >= 70 ? "c"
                    : perc >= 51 ? "Y" : perc >= 20 ? "y" : "r";
    papp(p, max, len, "@D[@GSTA@n@Y: @%s%d%s@D]@n", col, perc, "@w%");
  }
}

/* TNL, time, gold, practices, hunger/thirst, party health. */
static void prompt_extras(struct char_data *ch, char *p, size_t max, size_t &len) {
  if (PRF_FLAGGED(ch, PRF_DISPTNL) && GET_LEVEL(ch) < 100)
    papp(p, max, len, "@D[@yTNL@Y: @W%s@D]@n",
         add_commas(level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch)));

  if (PRF_FLAGGED(ch, PRF_DISTIME))
    papp(p, max, len, "@D[@W%2d %s@D]@n",
         time_info.hours % 12 == 0 ? 12 : time_info.hours % 12,
         time_info.hours >= 12 ? "PM" : "AM");

  if (PRF_FLAGGED(ch, PRF_DISGOLD))
    papp(p, max, len, "@D[@YZen@y: @W%s@D]@n", add_commas(GET_GOLD(ch)));

  if (PRF_FLAGGED(ch, PRF_DISPRAC))
    papp(p, max, len, "@D[@mPS@y: @W%s@D]@n",
         add_commas(GET_PRACTICES(ch, GET_CLASS(ch))));

  if (PRF_FLAGGED(ch, PRF_DISHUTH)) {
    int hun  = char_stat_get(ch, "hunger");
    int thir = char_stat_get(ch, "thirst");

    papp(p, max, len, "\n@D[@mHung@y:");
    const char *hlab = hun >= 48 ? " @WFull@D]@n"
                     : hun >= 40 ? " @WAlmost Full@D]@n"
                     : hun >= 30 ? " @WNeed Snack@D]@n"
                     : hun >= 20 ? " @WHungry@D]@n"
                     : hun >= 10 ? " @WAlmost Starving@D]@n"
                     : hun >= 5  ? " @WNear Starving@D]@n"
                     : hun >= 0  ? " @WStarving@D]@n"
                     :             " @WN/A@D]@n";
    papp(p, max, len, "%s", hlab);

    const char *tlab = thir >= 48 ? "@D[@mThir@y: @WQuenched@D]@n"
                     : thir >= 40 ? "@D[@mThir@y: @WNeed Sip@D]@n"
                     : thir >= 30 ? "@D[@mThir@y: @WNeed Drink@D]@n"
                     : thir >= 20 ? "@D[@mThir@y: @WThirsty@D]@n"
                     : thir >= 10 ? "@D[@mThir@y: @WAlmost Dehydrated@D]@n"
                     : thir >= 5  ? "@D[@mThir@y: @WNear Dehydration@D]@n"
                     : thir >= 0  ? "@D[@mThir@y: @WDehydrated@D]@n"
                     :              "@D[@mThir@y: @WN/A@D]@n";
    papp(p, max, len, "%s", tlab);
  }

  if (has_group(ch) && !PRF_FLAGGED(ch, PRF_GHEALTH)) {
    papp(p, max, len, "\n%s", report_party_health(ch));
    if (ch->temp_prompt) {
      free(ch->temp_prompt);
      ch->temp_prompt = nullptr;
    }
  }
}

char *make_prompt(struct descriptor_data *d) {
  static char prompt[MAX_PROMPT_LENGTH];

  if (d->str) {
    if (STATE(d) == CON_EXDESC)
      strcpy(prompt, "Enter Description(/h for editor help)> ");
    else if (PLR_FLAGGED(d->character, PLR_WRITING) && !PLR_FLAGGED(d->character, PLR_MAILING))
      strcpy(prompt, "Enter Message(/h for editor help)> ");
    else if (PLR_FLAGGED(d->character, PLR_MAILING))
      strcpy(prompt, "Enter Mail Message(/h for editor help)> ");
    else
      strcpy(prompt, "Enter Message> ");
    return prompt;
  }

  if (STATE(d) == CON_PLAYING && IS_NPC(d->character)) {
    snprintf(prompt, sizeof(prompt), "%s>\n", CAP(GET_NAME(d->character)));
    return prompt;
  }

  if (STATE(d) != CON_PLAYING) {
    *prompt = '\0';
    return prompt;
  }

  /* CON_PLAYING && !IS_NPC */
  {
    struct char_data *ch = d->character;
    size_t len = 0;
    *prompt = '\0';

    if (GET_INVIS_LEV(ch))
      papp(prompt, sizeof(prompt), len, "i%d ", GET_INVIS_LEV(ch));

    if (PRF_FLAGGED(ch, PRF_DISPAUTO) && GET_LEVEL(ch) >= 500) {
      if (GET_HIT(ch) << 2 < GET_MAX_HIT(ch))
        papp(prompt, sizeof(prompt), len, "PL: %" I64T " ", GET_HIT(ch));
      if (getCurST(ch) << 2 < GET_MAX_MOVE(ch))
        papp(prompt, sizeof(prompt), len, "STA: %" I64T " ", getCurST(ch));
      if (getCurKI(ch) << 2 < getMaxKI(ch))
        papp(prompt, sizeof(prompt), len, "KI: %" I64T " ", getCurKI(ch));
    } else {
      papp(prompt, sizeof(prompt), len, "@w");
      prompt_status_flags(d, ch, prompt, sizeof(prompt), len);
      prompt_sitting_status(ch, prompt, sizeof(prompt), len);
      prompt_charge_bar(ch, prompt, sizeof(prompt), len);
      prompt_barrier_bar(ch, prompt, sizeof(prompt), len);
      prompt_pl(ch, prompt, sizeof(prompt), len);
      prompt_ki(ch, prompt, sizeof(prompt), len);
      prompt_sta(ch, prompt, sizeof(prompt), len);
      prompt_extras(ch, prompt, sizeof(prompt), len);
      papp(prompt, sizeof(prompt), len, "\n");
    }

    if (len < 5)
      strncat(prompt, ">\n", sizeof(prompt) - len - 1);
  }

  return prompt;
}

/*
 * NOTE: 'txt' must be at most MAX_INPUT_LENGTH big.
 */
void write_to_q(const char *txt, struct txt_q *queue, int aliased) {
  struct txt_block *newt;

  CREATE(newt, struct txt_block, 1);
  newt->text = strdup(txt);
  newt->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    newt->next = NULL;
    queue->head = queue->tail = newt;
  } else {
    queue->tail->next = newt;
    queue->tail = newt;
    newt->next = NULL;
  }
}

/*
 * NOTE: 'dest' must be at least MAX_INPUT_LENGTH big.
 */
int get_from_q(struct txt_q *queue, char *dest, int *aliased) {
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return (0);

  strcpy(dest, queue->head->text); /* strcpy: OK (mutual MAX_INPUT_LENGTH) */
  *aliased = queue->head->aliased;

  tmp = queue->head;
  queue->head = queue->head->next;
  free(tmp->text);
  free(tmp);

  return (1);
}

/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d) {
  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (d->input.head) {
    struct txt_block *tmp = d->input.head;
    d->input.head = d->input.head->next;
    free(tmp->text);
    free(tmp);
  }
}

/* Add a new string to a player's output queue. For outside use. */
size_t write_to_output(struct descriptor_data *t, const char *txt, ...) {
  va_list args;
  size_t left;

  va_start(args, txt);
  left = vwrite_to_output(t, txt, args);
  va_end(args);

  return left;
}

extern "C" void desc_send_text(struct descriptor_data *d, const char *text) {
  write_to_output(d, "%s", text ? text : "");
}

extern "C" void desc_send_textf(struct descriptor_data *d, const char *format,
                                ...) {
  va_list args;

  va_start(args, format);
  vwrite_to_output(d, format ? format : "", args);
  va_end(args);
}

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

/* Color replacement arrays. Orig. Renx -- 011100, now modified */
char *ANSI[] = {"@",
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

const char CCODE[] = "@ndbgcrmywDBGCRMYW01234567luoex!";
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
const char RANDOM_COLORS[] = "bgcrmywBGCRMWY";

#define NEW_STRING_LENGTH (size_t)(dest_char - save_pos)
size_t proc_colors(char *txt, size_t maxlen, int parse, char **choices) {
  extern char *default_color_choices[NUM_COLOR + 1];
  char *dest_char, *source_char, *color_char, *save_pos, *replacement = NULL;
  int i, temp_color;
  size_t wanted;

  if (!txt || !strchr(txt, '@')) /* skip out if no color codes     */
    return strlen(txt);

  source_char = txt;
  CREATE(dest_char, char, maxlen);
  save_pos = dest_char;
  for (; *source_char && (NEW_STRING_LENGTH < maxlen);) {
    /* no color code - just copy */
    if (*source_char != '@') {
      *dest_char++ = *source_char++;
      continue;
    }

    /* if we get here we have a color code */

    source_char++; /* source_char now points to the code */

    /* look for a random color code picks a random number between 1 and 14 */
    if (*source_char == 'x') {
      temp_color = (rand() % 14);
      *source_char = RANDOM_COLORS[temp_color];
    }

    if (*source_char ==
        '\0') { /* string was terminated with color code - just put it in */
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
      if (NEW_STRING_LENGTH + strlen(replacement) + strlen(ANSISTART) + 1 <
          maxlen) { /* only substitute if there's room for the whole code */
        if (isdigit(replacement[0]))
          for (color_char = ANSISTART; *color_char;)
            *dest_char++ = *color_char++;
        for (color_char = replacement; *color_char;)
          *dest_char++ = *color_char++;
        if (isdigit(replacement[0]))
          *dest_char++ = ANSIEND;
      }
      replacement = NULL;
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
  free(save_pos); /* plug memory leak */

  return NEW_STRING_LENGTH + wanted;
}
#undef NEW_STRING_LENGTH

/* Add a new string to a player's output queue. */
size_t vwrite_to_output(struct descriptor_data *t, const char *format,
                        va_list args) {
  static char txt[MAX_STRING_LENGTH];
  size_t wantsize;
  int size;

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufspace == 0)
    return (0);

  wantsize = size = vsnprintf(txt, sizeof(txt), format, args);
  if (t->character)
    wantsize = size = proc_colors(txt, sizeof(txt), COLOR_ON(t->character),
                                  COLOR_CHOICES(t->character));
  /* If exceeding the size of the buffer, truncate it for the overflow message
   */
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
    size = LARGE_BUFSIZE - t->bufptr - 1;
    txt[size] = '\0';
    buf_overflows++;
  }

  /*
   * If we have enough space, just write to buffer and that's it! If the
   * text just barely fits, then it's switched to a large buffer instead.
   */
  if (t->bufspace > size) {
    strcpy(t->output + t->bufptr, txt); /* strcpy: OK (size checked above) */
    t->bufspace -= size;
    t->bufptr += size;
    return (t->bufspace);
  }

  buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (bufpool != NULL) {
    t->large_outbuf = bufpool;
    bufpool = bufpool->next;
  } else { /* else create a new one */
    CREATE(t->large_outbuf, struct txt_block, 1);
    CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
    buf_largecount++;
  }

  strcpy(t->large_outbuf->text,
         t->output);                 /* strcpy: OK (size checked previously) */
  t->output = t->large_outbuf->text; /* make big buffer primary */
  strcat(t->output, txt);            /* strcat: OK (size checked) */

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);

  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;

  return (t->bufspace);
}

void free_bufpool(void) {
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


/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socklen_t s) {
  int opt = MAX_SOCK_BUF;

  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt SNDBUF");
    return (-1);
  }

  return (0);
}

/* Initialize a descriptor */
void init_descriptor(struct descriptor_data *newd, int desc) {
  static int last_desc = 0; /* last descriptor number */

  newd->descriptor = desc;
  newd->idle_tics = 0;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->login_time = time(0);
  *newd->output = '\0';
  newd->bufptr = 0;
  newd->has_prompt = 1; /* prompt is part of greetings */
  STATE(newd) = CON_GET_USER;
  CREATE(newd->history, char *, HISTORY_SIZE);
  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;
}

void set_color(struct descriptor_data *d) {
  if (d->character == NULL) {
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    d->character->desc = d;
  }
  SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR);
  write_to_output(d, GREETANSI);
  write_to_output(
      d, "\r\n@w                  Welcome to Dragonball Advent Truth\r\n");
  write_to_output(
      d, "@D                 ---(@CPeak Logon Count Today@W: @w%4d@D)---@n\r\n",
      PCOUNT);
  write_to_output(
      d, "@D                 ---(@CHighest Logon Count   @W: @w%4d@D)---@n\r\n",
      HIGHPCOUNT);
  write_to_output(
      d,
      "@D                 ---(@CTotal Era %d Characters@W: @w%4s@D)---@n\r\n",
      CURRENT_ERA, add_commas(ERAPLAYERS));
  write_to_output(d, "\r\n@cEnter your desired username or the username you "
                     "have already made.\n@CEnter Username:@n\r\n");
  d->user = strdup("Empty");
  d->pass = strdup("Empty");
  d->email = strdup("Empty");
  d->tmp1 = strdup("Empty");
  d->tmp2 = strdup("Empty");
  d->tmp3 = strdup("Empty");
  d->tmp4 = strdup("Empty");
  d->tmp5 = strdup("Empty");
  return;
}

int new_descriptor(socklen_t s) {
  socklen_t desc;
  int sockets_connected = 0;
  socklen_t i;
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from = NULL;

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

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= CONFIG_MAX_PLAYING) {
    write_to_descriptor(
        desc,
        "Sorry, CircleMUD is full right now... please try again later!\r\n");
    close(desc);
    return (0);
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);

  /* find the sitename */
  if (CONFIG_NS_IS_SLOW ||
      !(from = gethostbyaddr((char *)&peer.sin_addr, sizeof(peer.sin_addr),
                             AF_INET))) {

    /* resolution failed */

    /* find the numeric site address */
    strncpy(newd->host, (char *)inet_ntoa(peer.sin_addr),
            HOST_LENGTH); /* strncpy: OK (n->host:HOST_LENGTH+1) */
    newd->host[HOST_LENGTH] = '\0';
  } else {
    strncpy(newd->host, from->h_name,
            HOST_LENGTH); /* strncpy: OK (n->host:HOST_LENGTH+1) */
    newd->host[HOST_LENGTH] = '\0';
  }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
    close(desc);
    mudlog(CMP, ADMLVL_GOD, TRUE, "Connection attempt denied from [%s]",
           newd->host);
    free(newd);
    return (0);
  }

  /* initialize descriptor data */
  init_descriptor(newd, desc);

  /* prepend to list */
  newd->next = descriptor_list;
  descriptor_list = newd;

  set_color(newd);

  return (0);
}

struct descriptor_data *descriptor_accept_connection(socklen_t desc,
                                                     struct net_connection *conn) {
  int sockets_connected = 0;
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  socklen_t peer_len = sizeof(peer);

  nonblock(desc);

  if (set_sendbuf(desc) < 0) {
    close(desc);
    if (conn)
      net_connection_destroy(conn);
    return NULL;
  }

  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= CONFIG_MAX_PLAYING) {
    const char *full_msg =
        "Sorry, CircleMUD is full right now... please try again later!\r\n";
    write(desc, full_msg, strlen(full_msg));
    close(desc);
    if (conn)
      net_connection_destroy(conn);
    return NULL;
  }

  CREATE(newd, struct descriptor_data, 1);
  memset((char *)newd, 0, sizeof(struct descriptor_data));

  if (getpeername(desc, (struct sockaddr *)&peer, &peer_len) == 0) {
    strncpy(newd->host, (char *)inet_ntoa(peer.sin_addr), HOST_LENGTH);
    newd->host[HOST_LENGTH] = '\0';
  } else {
    strncpy(newd->host, "unknown", HOST_LENGTH);
    newd->host[HOST_LENGTH] = '\0';
  }

  if (isbanned(newd->host) == BAN_ALL) {
    mudlog(CMP, ADMLVL_GOD, TRUE, "Connection attempt denied from [%s]",
           newd->host);
    close(desc);
    free(newd);
    if (conn)
      net_connection_destroy(conn);
    return NULL;
  }

  init_descriptor(newd, desc);
  newd->conn = conn;
  if (conn)
    net_connection_descriptor_set(conn, newd);

  newd->next = descriptor_list;
  descriptor_list = newd;

  set_color(newd);
  return newd;
}

void descriptor_fd_inherit_across_exec(socklen_t fd) {
  int flags = fcntl(fd, F_GETFD);
  if (flags < 0)
    return;
  fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC);
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
int process_output(struct descriptor_data *t) {
  char i[MAX_SOCK_BUF], *osb = i + 2;
  int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n"); /* strcpy: OK (for 'MAX_SOCK_BUF >= 3') */

  /* now, append the 'real' output */
  strcpy(osb, t->output); /* strcpy: OK (t->output:LARGE_BUFSIZE <
                             osb:MAX_SOCK_BUF-2) */

  /* if we're in the overflow state, notify the user */
  if (t->bufspace == 0)
    strcat(osb, "**OVERFLOW**\r\n"); /* strcpy: OK (osb:MAX_SOCK_BUF-2 reserves
                                        space) */

  /* add the extra CRLF if the person isn't in compact mode */
  if (STATE(t) == CON_PLAYING && t->character && !IS_NPC(t->character) &&
      !PRF_FLAGGED(t->character, PRF_COMPACT))
    strcat(osb, "\r\n"); /* strcpy: OK (osb:MAX_SOCK_BUF-2 reserves space) */

  /* add a prompt */

  strcat(i, make_prompt(t)); /* strcpy: OK (i:MAX_SOCK_BUF reserves space) */
  if (STATE(t) == CON_PLAYING) {
    proc_colors(i, sizeof(i), COLOR_ON(t->character),
                COLOR_CHOICES(t->character));
  }

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (t->has_prompt) {
    t->has_prompt = FALSE;
    result = write_to_descriptor(t->descriptor, i);
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
    write_to_output(t->snoop_by,
                    "\nvvvvvvvvvvvvv[Snoop]vvvvvvvvvvvvv\n%s\n^^^^^^^^^^^^^["
                    "Snoop]^^^^^^^^^^^^^\n",
                    t->output);
  /* The common case: all saved output was handed off to the kernel buffer. */
  if (result >= t->bufptr) {
    /*
     * if we were using a large buffer, put the large buffer on the buffer pool
     * and switch back to the small one
     */
    if (t->large_outbuf) {
      t->large_outbuf->next = bufpool;
      bufpool = t->large_outbuf;
      t->large_outbuf = NULL;
      t->output = t->small_outbuf;
    }
    /* reset total bufspace back to that of a small buffer */
    t->bufspace = SMALL_BUFSIZE - 1;
    t->bufptr = 0;
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

    strcpy(t->output, t->output + result); /* strcpy: OK (overlap) */
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
ssize_t perform_socket_write(socklen_t desc, const char *txt, size_t length) {
  ssize_t result = 0;

  int tmp, cnt, bytes_copied;

  result = write(desc, txt, length);

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

  if (errno == EAGAIN)
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
int write_to_descriptor(socklen_t desc, const char *txt) {
  ssize_t bytes_written;
  size_t total = strlen(txt), write_total = 0;

  if (net_connection_send_fd(desc, txt, total))
    return total;

  while (total > 0) {
    bytes_written = perform_socket_write(desc, txt, total);

    if (bytes_written < 0) {
      /* Fatal error.  Disconnect the player. */
      perror("SYSERR: Write to socket");
      return (-1);
    } else if (bytes_written == 0) {
      /* Temporary failure -- socket buffer full. */
      return (write_total);
    } else {
      txt += bytes_written;
      total -= bytes_written;
      write_total += bytes_written;
    }
  }

  return (write_total);
}

/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t perform_socket_read(socklen_t desc, char *read_point,
                            size_t space_left) {
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

  if (errno == EAGAIN)
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
int descriptor_process_bytes(struct descriptor_data *t, const char *bytes,
                             size_t len) {
  int buf_length, failed_subst;
  size_t space_left;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
  char tmp[MAX_INPUT_LENGTH];

  buf_length = strlen(t->inbuf);
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;
  if (len > space_left) {
    mud_log("WARNING: descriptor_process_bytes: about to close connection: input overflow");
    return (-1);
  }

  memcpy(t->inbuf + buf_length, bytes, len);
  t->inbuf[buf_length + len] = '\0';

  for (ptr = t->inbuf + buf_length; *ptr && !nl_pos; ptr++)
    if (ISNEWL(*ptr))
      nl_pos = ptr;

  if (nl_pos == NULL)
    return (0);

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 1;

    for (ptr = read_point; (space_left > 1) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b' || *ptr == 127) {
        if (write_point > tmp) {
          if (*(--write_point) == '$') {
            write_point--;
            space_left += 2;
          } else
            space_left++;
        }
      } else if (isascii(*ptr) && isprint(*ptr)) {
        if ((*(write_point++) = *ptr) == '$') {
          *(write_point++) = '$';
          space_left -= 2;
        } else
          space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      snprintf(buffer, sizeof(buffer), "Line too long.  Truncated to:\r\n%s\r\n",
               tmp);
      if (write_to_descriptor(t->descriptor, buffer) < 0)
        return (-1);
    }
    if (t->snoop_by)
      write_to_output(t->snoop_by, "%% %s\r\n", tmp);
    failed_subst = 0;

    if (*tmp == '!' && !(*(tmp + 1)))
      strcpy(tmp, t->last_input);
    else if (*tmp == '!' && *(tmp + 1)) {
      char *commandln = (tmp + 1);
      int starting_pos = t->history_pos,
          cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

      skip_spaces(&commandln);
      for (; cnt != starting_pos; cnt--) {
        if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
          strcpy(tmp, t->history[cnt]);
          strcpy(t->last_input, tmp);
          write_to_output(t, "%s\r\n", tmp);
          break;
        }
        if (cnt == 0)
          cnt = HISTORY_SIZE;
      }
    } else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
        strcpy(t->last_input, tmp);
    } else {
      strcpy(t->last_input, tmp);
      if (t->history[t->history_pos])
        free(t->history[t->history_pos]);
      t->history[t->history_pos] = strdup(tmp);
      if (++t->history_pos >= HISTORY_SIZE)
        t->history_pos = 0;
    }

    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    while (ISNEWL(*nl_pos))
      nl_pos++;

    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
        nl_pos = ptr;
  }

  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return (1);
}

int process_input(struct descriptor_data *t) {
  int buf_length, failed_subst;
  ssize_t bytes_read;
  size_t space_left;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
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
    /* note: this will bork if the user is giving lots of input when he first
     * connects */
    /* he shouldn't be doing this, and for the sake of efficiency, the read
     * buffer isn't searched */
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
     * I attempt to compensate by always returning after the _first_ read,
     * instead of looping forever until a read returns -1.  This simulates
     * non-blocking I/O because the result is we never call read unless we know
     * from select() that data is ready (process_input is only called if select
     * indicates that this descriptor is in the read set).  JE 2/23/95.
     */
  } while (nl_pos == NULL);

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 1;

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

      snprintf(buffer, sizeof(buffer),
               "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer) < 0)
        return (-1);
    }
    if (t->snoop_by)
      write_to_output(t->snoop_by, "%% %s\r\n", tmp);
    failed_subst = 0;

    if (*tmp == '!' && !(*(tmp + 1))) /* Redo last command. */
      strcpy(tmp, t->last_input); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
    else if (*tmp == '!' && *(tmp + 1)) {
      char *commandln = (tmp + 1);
      int starting_pos = t->history_pos,
          cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

      skip_spaces(&commandln);
      for (; cnt != starting_pos; cnt--) {
        if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
          strcpy(tmp,
                 t->history[cnt]); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
          strcpy(t->last_input,
                 tmp); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
          write_to_output(t, "%s\r\n", tmp);
          break;
        }
        if (cnt == 0) /* At top, loop to bottom. */
          cnt = HISTORY_SIZE;
      }
    } else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
        strcpy(t->last_input,
               tmp); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
    } else {
      strcpy(t->last_input, tmp); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
      if (t->history[t->history_pos])
        free(t->history[t->history_pos]);       /* Clear the old line. */
      t->history[t->history_pos] = strdup(tmp); /* Save the new. */
      if (++t->history_pos >= HISTORY_SIZE)     /* Wrap to top. */
        t->history_pos = 0;
    }

    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
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
int perform_subst(struct descriptor_data *t, char *orig, char *subst) {
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
    return (1);
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    write_to_output(t, "Invalid substitution.\r\n");
    return (1);
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(newsub, orig,
          strpos - orig); /* strncpy: OK (newsub:MAX_INPUT_LENGTH+5 >
                             orig:MAX_INPUT_LENGTH) */
  newsub[strpos - orig] = '\0';

  /* now, the replacement string */
  strncat(newsub, second,
          MAX_INPUT_LENGTH - strlen(newsub) - 1); /* strncpy: OK */

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(newsub, strpos + strlen(first),
            MAX_INPUT_LENGTH - strlen(newsub) - 1); /* strncpy: OK */

  /* terminate the string in case of an overflow from strncat */
  newsub[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, newsub); /* strcpy: OK (by mutual MAX_INPUT_LENGTH) */

  return (0);
}

void free_user(struct descriptor_data *d) {
  if (d->user_freed == 1) {
    return;
  }

  if (d->user == NULL) {
    send_to_imm("ERROR: free_user called but no user to free!");
    return;
  }
  d->user_freed = 1;

  if (!strcasecmp(d->user, "Empty"))
    return;

  mud_log("Freeing User: %s", d->user);

  /* Free up all the user data as needed */
  if (d->user) {
    free(d->user);
  }
  if (d->pass) {
    free(d->pass);
  }
  if (d->email) {
    free(d->email);
  }
  if (d->tmp1) {
    free(d->tmp1);
  }
  if (d->tmp2) {
    free(d->tmp2);
  }
  if (d->tmp3) {
    free(d->tmp3);
  }
  if (d->tmp4) {
    free(d->tmp4);
  }
  if (d->tmp5) {
    free(d->tmp5);
  }
}

void close_socket(struct descriptor_data *d) {
  struct descriptor_data *temp;

  REMOVE_FROM_LIST(d, descriptor_list, next, temp);
  if (d->conn) {
    net_connection_descriptor_set(d->conn, NULL);
    net_connection_destroy(d->conn);
    d->conn = NULL;
  }
  close(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    write_to_output(d->snoop_by, "Your victim is no longer among us.\r\n");
    d->snoop_by->snooping = NULL;
  }

  if (d->character) {
    /* If we're switched, this resets the mobile taken. */
    d->character->desc = NULL;

    /* Plug memory leak, from Eric Green. */
    if (!IS_NPC(d->character) && PLR_FLAGGED(d->character, PLR_MAILING) &&
        d->str) {
      if (*(d->str))
        free(*(d->str));
      free(d->str);
      d->str = NULL;
    } else if (d->backstr && !IS_NPC(d->character) &&
               !PLR_FLAGGED(d->character, PLR_WRITING)) {
      free(d->backstr); /* editing description ... not olc */
      d->backstr = NULL;
    }
    if (IS_PLAYING(d) || STATE(d) == CON_DISCONNECT) {
      struct char_data *link_challenged =
          d->original ? d->original : d->character;

      /* We are guaranteed to have a person. */
      act("$n has lost $s link.", TRUE, link_challenged, 0, 0, TO_ROOM);
      save_char(link_challenged);
      mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(link_challenged)), TRUE,
             "Closing link to: %s.", GET_NAME(link_challenged));
    } else {
      free_char(d->character);
    }
  } else
    mudlog(CMP, ADMLVL_IMMORT, TRUE, "Losing descriptor without char.");

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = NULL;

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

  /*. Kill any OLC stuff .*/
  switch (d->connected) {
  case CON_OEDIT:
  case CON_IEDIT:
  case CON_REDIT:
  case CON_ZEDIT:
  case CON_MEDIT:
  case CON_SEDIT:
  case CON_TEDIT:
  case CON_AEDIT:
  case CON_TRIGEDIT:
    cleanup_olc(d, CLEANUP_ALL);
    break;
  default:
    break;
  }

  free(d);
}

void check_idle_passwords(void) {
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_EMAIL &&
        STATE(d) != CON_NEWPASSWD)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      continue;
    } else {
      write_to_output(d, "\r\nTimed out... goodbye.\r\n");
      STATE(d) = CON_CLOSE;
    }
  }
}

void check_idle_menu(void) {
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_MENU && STATE(d) != CON_GET_USER &&
        STATE(d) != CON_UMENU)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      write_to_output(d, "\r\nYou are about to be disconnected due to "
                         "inactivity in 60 seconds.\r\n");
      continue;
    } else {
      write_to_output(d, "\r\nTimed out... goodbye.\r\n");
      STATE(d) = CON_CLOSE;
    }
  }
}

/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

void nonblock(socklen_t s) {
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

void reread_wizlists(int sig) { reread_wizlist = TRUE; }

void unrestrict_game(int sig) { emergency_unban = TRUE; }

/* clean up our zombie kids to avoid defunct processes */
void reap(int sig) {
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  signal(SIGCHLD, reap);
}

/* Dying anyway... */
void checkpointing(int sig) {
#ifndef MEMORY_DEBUG
  if (!tics_passed) {
    mud_log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop "
        "suspected)");
    abort();
  } else
    tics_passed = 0;
#endif
}

/* Dying anyway... */
void hupsig(int sig) {
  mud_log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  exit(1); /* perhaps something more elegant should
            * substituted */
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

void signal_setup(void) {
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
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  signal(SIGHUP, hupsig);
  signal(SIGCHLD, reap);
  signal(SIGINT, hupsig);
  signal(SIGTERM, hupsig);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGALRM, SIG_IGN);
}

/* ****************************************************************
 *       Public routines for system-to-player-communication        *
 **************************************************************** */

int arena_watch(struct char_data *ch) {

  struct descriptor_data *d;
  int found = FALSE, room = NOWHERE;

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING)
      continue;

    if (IN_ARENA(d->character)) {
      if (ARENA_IDNUM(ch) == GET_IDNUM(d->character)) {
        found = TRUE;
        room = char_room_vnum_get(d->character);
      }
    }
  }

  if (found == FALSE) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
    ARENA_IDNUM(ch) = -1;
    return (NOWHERE);
  } else {
    return (room);
  }
}

void send_to_eaves(const char *messg, struct char_data *tch, ...) {
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING)
      continue;

    int roll = rand_number(1, 101);
    if (GET_EAVESDROP(d->character) == char_room_vnum_get(tch) &&
        GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
      char buf[1000];
      char buf2[1000];
      *buf = '\0';
      sprintf(buf2, "@W%s %s\r\n", PERS(d->character, tch), messg);
      sprintf(buf, "-----Eavesdrop-----\r\n%s-----Eavesdrop-----\r\n", buf2);
      send_to_char(d->character, buf);
    }
  }
}

void send_to_all(const char *messg, ...) {
  struct descriptor_data *i;
  va_list args;

  if (messg == NULL)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;

    va_start(args, messg);
    vwrite_to_output(i, messg, args);
    va_end(args);
  }
}

void send_to_outdoor(const char *messg, ...) {
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    va_list args;

    if (STATE(i) != CON_PLAYING || i->character == NULL)
      continue;
    if (!AWAKE(i->character) || !OUTSIDE(i->character))
      continue;

    va_start(args, messg);
    vwrite_to_output(i, messg, args);
    va_end(args);
  }
}

void send_to_moon(const char *messg, ...) {
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    va_list args;

    if (STATE(i) != CON_PLAYING || i->character == NULL)
      continue;
    if (!AWAKE(i->character) || !HAS_MOON(i->character))
      continue;

    va_start(args, messg);
    vwrite_to_output(i, messg, args);
    va_end(args);
  }
}

void send_to_planet(int type, int planet, const char *messg, ...) {
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    va_list args;

    if (STATE(i) != CON_PLAYING || i->character == NULL)
      continue;
    if (!AWAKE(i->character) ||
        !room_flagged(char_room_get(i->character), planet))
      continue;
    else {
      if (type == 0) {
        va_start(args, messg);
        vwrite_to_output(i, messg, args);
        va_end(args);
      } else if (OUTSIDE(i->character) &&
                 GET_SKILL(i->character, SKILL_SPOT) >= axion_dice(-5)) {
        va_start(args, messg);
        vwrite_to_output(i, messg, args);
        va_end(args);
      }
    }
  }
}

void send_to_room(struct room_data *room, const char *messg, ...) {
  if (messg == NULL)
    return;

  char buf[MAX_STRING_LENGTH];
  va_list args;
  va_start(args, messg);
  vsnprintf(buf, sizeof(buf), messg, args);
  va_end(args);

  room_people_iterate(room, [&](struct char_data *i) {
    if (i->desc)
      write_to_output(i->desc, "%s", buf);
    return true;
  });

  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING)
      continue;

    if (PRF_FLAGGED(d->character, PRF_ARENAWATCH)) {
      if (arena_watch(d->character) == room_vnum_get(room)) {
        char buf[2000];
        *buf = '\0';
        sprintf(buf,
                "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n",
                messg);
        va_start(args, messg);
        vwrite_to_output(d, buf, args);
        va_end(args);
      }
    }
    if (GET_EAVESDROP(d->character) > 0) {
      int roll = rand_number(1, 101);
      if (GET_EAVESDROP(d->character) == room_vnum_get(room) &&
          GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
        char buf[1000];
        *buf = '\0';
        sprintf(buf, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n",
                messg);
        va_start(args, messg);
        vwrite_to_output(d, buf, args);
        va_end(args);
      }
    }
  }
}

const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression)                                        \
  if ((pointer) == NULL)                                                       \
    i = ACTNULL;                                                               \
  else                                                                         \
    i = (expression);

/* higher-level communication: the act() function */
void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj,
                 const void *vict_obj, struct char_data *to) {
  const char *i = NULL;
  char lbuf[MAX_STRING_LENGTH], *buf, *j;
  bool uppercasenext = FALSE;
  const struct char_data *dg_victim = NULL;
  const struct obj_data *dg_target = NULL;
  const char *dg_arg = NULL;

  buf = lbuf;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
      case 'n':
        i = PERS(ch, to);
        break;
      case 'N':
        CHECK_NULL(vict_obj, PERS(((struct char_data *)vict_obj), to));
        dg_victim = (const struct char_data *)vict_obj;
        break;
      case 'm':
        i = HMHR(ch);
        break;
      case 'M':
        CHECK_NULL(vict_obj, HMHR((const struct char_data *)vict_obj));
        dg_victim = (const struct char_data *)vict_obj;
        break;
      case 's':
        i = HSHR(ch);
        break;
      case 'S':
        CHECK_NULL(vict_obj, HSHR((const struct char_data *)vict_obj));
        dg_victim = (const struct char_data *)vict_obj;
        break;
      case 'e':
        i = HSSH(ch);
        break;
      case 'E':
        CHECK_NULL(vict_obj, HSSH((const struct char_data *)vict_obj));
        dg_victim = (const struct char_data *)vict_obj;
        break;
      case 'o':
        CHECK_NULL(obj, OBJN(obj, to));
        break;
      case 'O':
        CHECK_NULL(vict_obj, OBJN((struct obj_data *)vict_obj, to));
        dg_target = (const struct obj_data *)vict_obj;
        break;
      case 'p':
        CHECK_NULL(obj, OBJS(obj, to));
        break;
      case 'P':
        CHECK_NULL(vict_obj, OBJS((struct obj_data *)vict_obj, to));
        dg_target = (const struct obj_data *)vict_obj;
        break;
      case 'a':
        CHECK_NULL(obj, SANA(obj));
        break;
      case 'A':
        CHECK_NULL(vict_obj, SANA((const struct obj_data *)vict_obj));
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
        for (j = buf; j > lbuf && !isspace((int)*(j - 1)); j--)
          ;
        if (j != buf)
          *j = UPPER(*j);
        i = "";
        break;
      /* uppercase next word */
      case 'U':
        uppercasenext = TRUE;
        i = "";
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
          *buf = UPPER(*buf);
          uppercasenext = FALSE;
        }
        buf++;
      }
      orig++;
    } else if (!(*(buf++) = *(orig++))) {
      break;
    } else if (uppercasenext && !isspace((int)*(buf - 1))) {
      *(buf - 1) = UPPER(*(buf - 1));
      uppercasenext = FALSE;
    }
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

  if (to->desc)
    write_to_output(to->desc, "%s", CAP(lbuf));

  if ((IS_NPC(to) && dg_act_check) && (to != ch))
    act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);

  if (last_act_message)
    free(last_act_message);
  last_act_message = strdup(lbuf);
}

char *act(const char *str, int hide_invisible, struct char_data *ch,
          struct obj_data *obj, const void *vict_obj, int type) {
  struct char_data *to;
  struct room_data *act_room = nullptr;
  int to_sleeping, res_sneak, res_hide, dcval = 0, resskill = 0;

  if (!str || !*str)
    return NULL;

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

  if (res_sneak && AFF_FLAGGED(ch, AFF_SNEAK)) {
    dcval = roll_skill(ch, SKILL_MOVE_SILENTLY); /* How difficult to counter? */
    if (GET_SKILL(ch, SKILL_BALANCE))
      dcval += GET_SKILL(ch, SKILL_BALANCE) / 10;
    if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 5 || GET_GENOME(ch, 1) == 5)) {
      dcval += 10;
    }
    resskill = SKILL_SPOT; /* Skill used to resist      */
  } else if (res_hide && AFF_FLAGGED(ch, AFF_HIDE)) {
    dcval = roll_skill(ch, SKILL_HIDE);
    if (GET_SKILL(ch, SKILL_BALANCE))
      dcval += GET_SKILL(ch, SKILL_BALANCE) / 10;
    resskill = SKILL_SPOT;
  }

  /* this is a hack as well - DG_NO_TRIG is 256 -- Welcor */
  /* If the bit is set, unset dg_act_check, thus the ! below */
  if (!(dg_act_check = !IS_SET(type, DG_NO_TRIG)))
    REMOVE_BIT(type, DG_NO_TRIG);

  if (type == TO_CHAR) {
    if (ch && SENDOK(ch) &&
        (!resskill || (roll_skill(ch, resskill) >= dcval))) {
      perform_act(str, ch, obj, vict_obj, ch);
      return last_act_message;
    }
    return NULL;
  }

  if (type == TO_VICT) {
    if ((to = (struct char_data *)vict_obj) != NULL && SENDOK(to) &&
        (!resskill || (roll_skill(to, resskill) >= dcval))) {
      perform_act(str, ch, obj, vict_obj, to);
      return last_act_message;
    }
    return NULL;
  }

  if (type == TO_GMOTE) {
    struct descriptor_data *i;
    char buf[MAX_STRING_LENGTH];
    for (i = descriptor_list; i; i = i->next) {
      if (!i->connected && i->character &&
          !PRF_FLAGGED(i->character, PRF_NOGOSS) &&
          !PLR_FLAGGED(i->character, PLR_WRITING) &&
          !room_flagged(char_room_get(i->character), ROOM_SOUNDPROOF)) {

        sprintf(buf, "@y%s@n", str);
        perform_act(buf, ch, obj, vict_obj, i->character);
        char buf2[MAX_STRING_LENGTH];
        sprintf(buf2, "%s\r\n", buf);
        add_history(i->character, buf2, HIST_GOSSIP);
      }
    }
    return last_act_message;
  }

  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  if (ch && char_room_get(ch) != NULL)
    act_room = char_room_get(ch);
  else if (obj && obj_room_get(obj) != NULL)
    act_room = obj_room_get(obj);
  else {
    return NULL;
  }

  if ((type & TO_ROOM)) {
    struct descriptor_data *d;

    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING)
        continue;

      if (ch != NULL) {
        if (IN_ARENA(ch)) {
          if (PRF_FLAGGED(d->character, PRF_ARENAWATCH)) {
            if (arena_watch(d->character) == char_room_vnum_get(ch)) {
              char buf3[2000];
              *buf3 = '\0';
              sprintf(buf3,
                      "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@"
                      "n\r\n",
                      str);
              perform_act(buf3, ch, obj, vict_obj, d->character);
            }
          }
        }
      }
      if (GET_EAVESDROP(d->character) > 0) {
        int roll = rand_number(1, 101);
        if (!resskill || (roll_skill(d->character, resskill) >= dcval)) {
          if (ch != NULL &&
              GET_EAVESDROP(d->character) == char_room_vnum_get(ch) &&
              GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
            char buf3[1000];
            *buf3 = '\0';
            sprintf(buf3,
                    "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n",
                    str);
            perform_act(buf3, ch, obj, vict_obj, d->character);
          } else if (obj != NULL &&
                     GET_EAVESDROP(d->character) == obj_room_vnum_get(obj) &&
                     GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
            char buf3[1000];
            *buf3 = '\0';
            sprintf(buf3,
                    "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n",
                    str);
            perform_act(buf3, ch, obj, vict_obj, d->character);
          }
        }
      }
    }
  }

  room_people_iterate(act_room, [&](struct char_data *rp) {
    if (!SENDOK(rp) || (rp == ch)) return true;
    if (hide_invisible && ch && !CAN_SEE(rp, ch)) return true;
    if (type != TO_ROOM && rp == vict_obj) return true;
    if (resskill && roll_skill(rp, resskill) < dcval) return true;
    perform_act(str, ch, obj, vict_obj, rp);
    return true;
  });
  return last_act_message;
}
/* Prefer the file over the descriptor. */
void setup_log(const char *filename, int fd) {
  FILE *s_fp;

  s_fp = stderr;

  if (filename == NULL || *filename == '\0') {
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

int open_logfile(const char *filename, FILE *stderr_fp) {
  if (stderr_fp) /* freopen() the descriptor. */
    logfile = freopen(filename, "w", stderr_fp);
  else
    logfile = fopen(filename, "w");

  if (logfile) {
    printf("Using log file '%s'%s.\n", filename,
           stderr_fp ? " with redirection" : "");
    return (TRUE);
  }

  printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
  return (FALSE);
}

/*
 * This may not be pretty but it keeps game_loop() neater than if it was inline.
 */

void circle_sleep(struct timeval *timeout) {
  if (select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, timeout) < 0) {
    if (errno != EINTR) {
      perror("SYSERR: Select sleep");
      exit(1);
    }
  }
}

void show_help(struct descriptor_data *t, const char *entry) {
  int chk, bot, top, mid, minlen;
  char buf[MAX_STRING_LENGTH];

  if (!help_table)
    return;

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(entry);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      return;
    } else if (!(chk = strncasecmp(entry, help_table[mid].keywords, minlen))) {
      while ((mid > 0) && (!(chk = strncasecmp(
                                 entry, help_table[mid - 1].keywords, minlen))))
        mid--;
      write_to_output(t, "\r\n");
      snprintf(buf, sizeof(buf), "%s\r\n[ PRESS RETURN TO CONTINUE ]",
               help_table[mid].entry);
      write_to_output(t, buf);
      return;
    } else {
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
    }
  }
}

/* Thx to Jamie Nelson of 4D for this contribution */
void send_to_range(room_vnum start, room_vnum finish, const char *messg, ...) {
  int j;

  if (start > finish) {
    mud_log("send_to_range passed start room value greater then finish.");
    return;
  }
  if (messg == NULL)
    return;

  char buf[MAX_STRING_LENGTH];
  va_list args;
  va_start(args, messg);
  vsnprintf(buf, sizeof(buf), messg, args);
  va_end(args);

  for (j = start; j <= finish; j++) {
    auto room = room_by_id(j);
    if (!room) continue;
    room_people_iterate(room, [&](struct char_data *i) {
      if (i->desc)
        write_to_output(i->desc, "%s", buf);
      return true;
    });
  }
}

int passcomm(struct char_data *ch, char *comm) {

  if (!strcasecmp(comm, "score")) {
    return TRUE;
  } else if (!strcasecmp(comm, "sco")) {
    return TRUE;
  } else if (!strcasecmp(comm, "ooc")) {
    return TRUE;
  } else if (!strcasecmp(comm, "newbie")) {
    return TRUE;
  } else if (!strcasecmp(comm, "newb")) {
    return TRUE;
  } else if (!strcasecmp(comm, "look")) {
    return TRUE;
  } else if (!strcasecmp(comm, "lo")) {
    return TRUE;
  } else if (!strcasecmp(comm, "l")) {
    return TRUE;
  } else if (!strcasecmp(comm, "status")) {
    return TRUE;
  } else if (!strcasecmp(comm, "stat")) {
    return TRUE;
  } else if (!strcasecmp(comm, "sta")) {
    return TRUE;
  } else if (!strcasecmp(comm, "tell")) {
    return TRUE;
  } else if (!strcasecmp(comm, "reply")) {
    return TRUE;
  } else if (!strcasecmp(comm, "say")) {
    return TRUE;
  } else if (!strcasecmp(comm, "osay")) {
    return TRUE;
  } else {
    return FALSE;
  }
}

void cleanup_game_world() {
  mud_log("Clearing game world.");
  destroy_db();

  mud_log("Clearing other memory.");
  free_bufpool();                        /* comm.c */
  free_player_index();                   /* players.c */
  clear_free_list();                     /* mail.c */
  free_mail_index();                     /* mail.c */
  free_text_files();                     /* db.c */
  clear_boards();                        /* boards.c */
  free(cmd_sort_info);                   /* act.informative.c */
  free_command_list();                   /* act.informative.c */
  free_social_messages();                /* act.social.c */
  free_help_table();                     /* db.c */
  Free_Invalid_List();                   /* ban.c */
  free_strings(&config_info, OASIS_CFG); /* oasis_delete.c */
  free_disabled();                       /* interpreter.c */
  free_save_list();                      /* genolc.c */

  if (last_act_message)
    free(last_act_message);

  /* probably should free the entire config here.. */
  free(CONFIG_CONFFILE);

  mud_log("Done.");
}
