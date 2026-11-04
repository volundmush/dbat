/* ************************************************************************
*   File: wizard/server.c                                  Part of CircleMUD *
*  Usage: God commands that control the server                            *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/act.wizard.h"
#include "dbat/consts/maximums.h"
#include "dbat/players.h"
#include "dbat/utils.h"

#include <unistd.h>
#include <limits.h>

#include "dbat/interpreter.h"
#include "dbat/time.h"

#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/object_utils.h"
#include "dbat/zone_utils.h"
#include "dbat/room_utils.h"
#include "dbat/stringutils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/fileop.h"
#include "dbat/extract.h"
#include "dbat/affect.h"

#include "dbat/config.h"
#include "dbat/act.other.h"
#include "dbat/maputils.h"
#include "dbat/broadcast.h"
#include "dbat/handler.h"
#include "dbat/act.item.h"
#include "dbat/act.informative.h"
#include "dbat/weather.h"
#include "dbat/assemblies.h"
#include "dbat/house.h"
#include "dbat/comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/races.h"
#include "dbat/class.h"
#include "dbat/spells.h"
#include "dbat/improved-edit.h"
#include "dbat/objsave.h"
#include "dbat/feats.h"
#include "dbat/fight.h"
#include "dbat/genolc.h"
#include "dbat/screen.h"
#include "dbat/local_limits.h"
#include "dbat/shop.h"
#include "dbat/guild.h"
#include "dbat/spell_parser.h"
#include "dbat/atomicfile.h"
#include "dbat/game_world.h"

/* local functions */
static void execute_copyover(GameWorld *g);
static void game_info(GameWorld *g, const char *format, ...) __attribute__((format(printf, 2, 3)));

/* local variables */

// definitions
ACMD(do_lag)
{

  struct descriptor_data *d;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "Syntax: lag (target) (number of seconds)\r\n");
    return;
  }

  int found = false, num = atoi(arg2);

  if (num <= 0 || num > 5) {
    send_to_char(g, ch, "Keep it between 1 to 5 seconds please.\r\n");
    return;
  }

  for (d = g->descriptors; d; d = d->next) {
    if (!d->character) {
      continue;
    }
    if (!strcasecmp(CAP(char_get_name(d->character)), CAP(arg))) {
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
  char arg[MAX_INPUT_LENGTH];

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char(g, ch, "If you want to shut something down, say so!\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    mud_log("(GC) Shutdown by %s.", char_get_name(ch));
    send_to_all(g, "Shutting down.\r\n");
    g->circle_shutdown = 1;
  } else if (!strcasecmp(arg, "reboot")) {
    mud_log("(GC) Reboot by %s.", char_get_name(ch));
    send_to_all(g, "Rebooting.. come back in a minute or two.\r\n");
    touch(FASTBOOT_FILE);
    g->circle_shutdown = g->circle_reboot = 1;
  } else if (!strcasecmp(arg, "die")) {
    mud_log("(GC) Shutdown by %s.", char_get_name(ch));
    send_to_all(g, "Shutting down for maintenance.\r\n");
    touch(KILLSCRIPT_FILE);
    g->circle_shutdown = 1;
  } else if (!strcasecmp(arg, "now")) {
    mud_log("(GC) Shutdown NOW by %s.", char_get_name(ch));
    send_to_all(g, "Rebooting.. come back in a minute or two.\r\n");
    g->circle_shutdown = 1;
    g->circle_reboot   = 2; /* do not autosave olc */
  } else if (!strcasecmp(arg, "pause")) {
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
  const char *suffix = " (deleted)";
  size_t slen        = strlen(suffix);
  if ((size_t)len > slen && !strcmp(out + len - slen, suffix))
    out[len - slen] = '\0';
  return true;
}

static void game_info(GameWorld *g, const char *format, ...)
{
  struct descriptor_data *i;
  va_list args;
  char messg[MAX_STRING_LENGTH];

  if (format == nullptr)
    return;

  snprintf(messg, sizeof(messg), "@r-@R=@D<@GCOPYOVER@D>@R=@r- @W");

  for (i = g->descriptors; i; i = i->next) {
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
  char arg[MAX_INPUT_LENGTH];
  int secs;

  one_argument(argument, arg);
  if (!*arg) {
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
    send_to_char(g, ch, "every minute.  During the last minute, there will be a warning every 15 seconds.\r\n");
  } else {
    secs = atoi(arg);
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

static void execute_copyover(GameWorld *g)
{
  FILE *fp;
  struct descriptor_data *d, *d_next;
  char buf[100], buf2[100];

  fp = fopen(COPYOVER_FILE, "w");

  if (!fp) {
    send_to_imm(g, "Copyover file not writeable, aborted.\n\r");
    return;
  }

  /* Consider changing all saved areas here, if you use OLC */
  save_all(g);
  save_mud_time(g, &g->time_info);
  snprintf(buf, sizeof(buf), "\t\x1B[1;31m \007\007\007The universe stops for a moment as space and time fold.\x1B[0;0m\r\n");
  /* For each playing descriptor, save its state */
  for (d = g->descriptors; d; d = d_next) {
    struct char_data *och = d->character;
    d_next                = d->next; /* We delete from the list , so need to save this */
    if (!d->character || d->connected > CON_PLAYING) {
      write_to_descriptor(d->descriptor, "\n\rSorry, we are rebooting. Come back in a few seconds.\n\r");
      close_socket(g, d); /* throw'em out */
    } else {
      if (rroom_get_vnum(g, char_get_in_room(och)) > 1) {
        fprintf(fp, "%d %s %s %d %s\n", d->descriptor, char_get_name(och), d->host, rroom_get_vnum(g, char_get_in_room(och)), d->user);
      } else if (rroom_get_vnum(g, char_get_in_room(och)) <= 1 && rroom_get_vnum(g, char_get_was_in_room(och)) > 1) {
        fprintf(fp, "%d %s %s %d %s\n", d->descriptor, char_get_name(och), d->host, rroom_get_vnum(g, char_get_was_in_room(och)), d->user);
      } else {
        fprintf(fp, "%d %s %s 300 %s\n", d->descriptor, char_get_name(och), d->host, d->user);
      }
      mud_log("printing descriptor name and host of connected players");
      /* save och */
      Crash_rentsave(g, och, 0);
      save_char(g, och);
      write_to_descriptor(d->descriptor, buf);
    }
  }

  fprintf(fp, "-1\n");
  fclose(fp);

  /* Close reserve and other always-open files and release other resources
     since we are now using ASCII pfiles, closing the player_fl would crash
     the game, since it's no longer around, so I commented it out. I'll
     leave the code here, for historical reasons -spl
     fclose(player_fl); */

  /* exec - descriptors are inherited */

  snprintf(buf, sizeof(buf), "%d", g->port);
  snprintf(buf2, sizeof(buf2), "-C%d", g->mother_desc);
  char exe[PATH_MAX];
  if (!copyover_exe_path(exe, sizeof(exe)))
    strlcpy(exe, "zig-out/bin/dbat", sizeof(exe));
  chdir("..");
  execl(exe, "dbat", buf2, buf, (char *)nullptr);
  /* Failed - sucessful exec will not return */

  perror("do_copyover: execl");
  send_to_imm(g, "Copyover FAILED!\n\r");

  exit(1); /* too much trouble to try to recover! */
}

void copyover_check(GameWorld *g)
{
  if (!g->copyover_timer) return;
  g->copyover_timer--;
  if (!g->copyover_timer) {
    execute_copyover(g);
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
  char arg[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char(g, ch, "Usage: DC <user number> (type USERS for a list)\r\n");
    return;
  }
  for (d = g->descriptors; d && d->desc_num != num_to_dc; d = d->next);

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
  char arg[MAX_INPUT_LENGTH];
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > 101) {
      send_to_char(g, ch, "Invalid wizlock value.\r\n");
      return;
    }
    g->circle_restrict = value;
    when            = "now";
  } else
    when = "currently";

  if (*arg) {
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
  if (!*arg) {
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
