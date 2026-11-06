/* ************************************************************************
*   File: wizard/comm.c                                  Part of CircleMUD *
*  Usage: God communication and news commands                             *
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

/* Read the news entry */
ACMD(do_news)
{

  if (char_is_npc(ch)) {
    return;
  }

  FILE *fl;
  const char *filename;
  char line[256];
  char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], title[256], lastline[256];
  int entries = 0, lookup = 0, nr, found = false, first = true, exit = false;

  one_argument(argument, arg);
  filename = NEWS_FILE;

  /* Check the file for the entry so we may edit it if need be*/

  if (!*arg) {
    send_to_char(g, ch, "Syntax: news (number | list)\r\n");
    return;
  }

  lookup = atoi(arg);

  if (!(fl = fopen(filename, "r"))) {
    mud_log("SYSERR: opening news file for reading");
    return;
  }

  if (lookup > 0) {
    while (!feof(fl) && exit == false) {
      get_line(fl, line);
      if (*line == '#') {                    /* Count the entries */
        if (sscanf(line, "#%d", &nr) != 1) { /* Check the entry */
          continue;
        }
        if (nr != lookup) {
          entries++;
          continue;
        } else { /* One we want to read */
          sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
          snprintf(buf, sizeof(buf), "@w--------------------------------------------------------------\n@cNum@W: @D(@G%3d@D)                @cTitle@W: @g%-50s@n\n", nr, title);
          found = true;
          while (!feof(fl) && exit == false) {
            get_line(fl, line);
            if (*line != '#') { /* As long as it isn't a new entry*/
              if (first == true) {
                first = false;
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s\n@w--------------------------------------------------------------\n", line);
                snprintf(lastline, sizeof(lastline), "%s", line);
              } else if (!strcasecmp(line, lastline)) {
                continue;
              } else {
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s\n", line);
                snprintf(lastline, sizeof(lastline), "%s", line);
              }
            } else {
              exit = true;
            }
          } /* End write buffer with entry text */
        } /* End One We wanted to read */
      } /* End Check the entry */
    } /* End of main read while */
    fclose(fl);
  } else if (!strcasecmp(arg, "list")) {
    while (!feof(fl)) {
      get_line(fl, line);
      if (*line == '#') { /* Count the entries */
        entries++;
        if (sscanf(line, "#%d", &nr) != 1) { /* Check the entry */
          continue;
        } else { /* One we want to read */
          if (first == true) {
            sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
            snprintf(buf, sizeof(buf), "@wNews Entries (Newest at the bottom, to read an entry use 'news (number)')\n@D[@cNum@W: @D(@G%3d@D) @cTitle@W: @g%-50s@D]@n\n", nr, title);
            first = false;
          } else {
            sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "@D[@cNum@W: @D(@G%3d@D) @cTitle@W: @g%-50s@D]@n\n", nr, title);
          }
        }
      } /* End Check the entry */
    } /* End of main listwhile */
    fclose(fl);

    if (entries > 0) {
      char_set_lastpl(ch, game_now(g));
      char_set_wait(ch, PULSE_1SEC);
      send_to_char(g, ch, "%s", buf);
    } else {
      send_to_char(g, ch, "The news file is empty right now.\r\n");
    }
    *buf      = '\0';
    *title    = '\0';
    *lastline = '\0';
    return;
  } else {
    fclose(fl);
    send_to_char(g, ch, "Syntax: news (number | list)\r\n");
    return;
  }

  if (found == true) {
    send_to_char(g, ch, "%s\r\n", buf);
    char_set_lastpl(ch, game_now(g));
    *buf      = '\0';
    *title    = '\0';
    *lastline = '\0';
    char_set_wait(ch, PULSE_1SEC);
    return;
  }
  send_to_char(g, ch, "That news entry does not exist.\r\n");
  return;
}

/* Write the news entry */

ACMD(do_newsedit)
{

  if (char_get_admlevel(ch) < 1 || char_is_npc(ch)) {
    return;
  }

  FILE *fl;
  const char *filename;
  char line[256];
  int entries = 0, lookup = 0, lastentry = 0, nr;

  filename = NEWS_FILE;

  if (!*argument) {
    send_to_char(g, ch, "Syntax: newsedit (title)\r\n");
    return;
  }
  if (strlen(argument) > 50) {
    send_to_char(g, ch, "Limit of 50 characters for title.\r\n");
    return;
  }
  if (strstr(argument, "#")) {
    send_to_char(g, ch, "# is a forbidden character for news entries as it is used by the file system.\r\n");
    return;
  }

  /* Check the file for the entry so we may edit it if need be*/

  if (!(fl = fopen(filename, "r"))) {
    mud_log("SYSERR: Couldn't open news file for reading");
    return;
  }

  while (!feof(fl)) {
    get_line(fl, line);
    if (*line == '#') { /* Count the entries */
      entries++;
      if (sscanf(line, "#%d", &nr) != 1) {
        continue;
      }
      lastentry = nr;
    }
  } /* End of main read while */
  fclose(fl);

  /* Time to write the entry */
  struct {
    const char *cmd;
    char level;
    char **buffer;
    int size;
    const char *filename;
  } fields[] = {
      /* edit the lvls to your own needs */
      {"news", ADMLVL_IMMORT, &g->immlist, 2000, IMMLIST_FILE},
      {"\n",   0,             nullptr,  0,    nullptr     }
  };

  char *tmstr;
  time_t mytime                = game_now(g);
  tmstr                        = (char *)asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (lastentry == 0) {
    if (entries == 0) {
      lookup = 1;
    } else { /* Uh oh */
      send_to_imm(g, "ERROR: News file entries are disorganized. Report to Iovan for analysis.\r\n");
      return;
    }
  } else {
    lookup = lastentry + 1;
  }
  char *backstr = nullptr;
  act(g, "$n begins to edit the news.", true, ch, 0, 0, TO_ROOM);
  send_to_char(g, ch, "@D----------------------=[@GNews Edit@D]=----------------------@n\n");
  send_to_char(g, ch, " @RRemember that using # in newsedit is not possible. That\n");
  send_to_char(g, ch, "character will be eaten because it is required for the news\n");
  send_to_char(g, ch, "file as a delimiter. Also if you want to create an empty line\n");
  send_to_char(g, ch, "between paragraphs you will need to enter a single space and\n");
  send_to_char(g, ch, "not just push enter. Happy editing!@n\n");
  send_to_char(g, ch, "@D---------------------------------------------------------@n\n");
  send_editor_help(g, char_get_desc(ch));
  skip_spaces(&argument);
  char_get_desc(ch)->newsbuf = strdup(argument);
  g->TOP_OF_NEWS       = lookup;
  g->LASTNEWS          = lookup;
  string_write(char_get_desc(ch), fields[0].buffer, 2000, 0, backstr);
  desc_set_connected(char_get_desc(ch), CON_NEWSEDIT);
}

ACMD(do_echo)
{
  skip_spaces(&argument);
  bool NoName = false;

  if (!*argument) {
    send_to_char(g, ch, "Yes.. but what?\r\n");
    return;
  }

  char buf[MAX_INPUT_LENGTH];
  char name[128];
  int found              = false;
  struct char_data *vict = nullptr, *next_v = nullptr, *tch = nullptr;

  for (vict = rroom_get_people(g, char_get_in_room(ch)); vict; vict = next_v) {
    next_v = char_get_next_in_room(vict);
    if (vict == ch)
      continue;
    if (found == false) {
      snprintf(name, sizeof(name), "*%s", char_get_name(vict));
      if (strstr(argument, CAP(name))) {
        found = true;
        tch   = vict;
      }
      if (found == false && !char_is_npc(vict)) {
        if (readIntro(ch, vict) == 1) {
          snprintf(name, sizeof(name), "*%s", get_i_name(ch, vict));
          if (strstr(argument, CAP(name))) {
            found = true;
            tch   = vict;
          }
        }
      }
    }
  }

  if (subcmd == SCMD_SMOTE) {
    if (!strstr(argument, "#")) {
      NoName = true;
    }
    strlcpy(buf, argument, sizeof(buf));
    search_replace(buf, "#", "$n");
    search_replace(buf, "&1", "'@C");
    search_replace(buf, "&2", "@w'");
    if (found == true) {
      search_replace(buf, name, "$N");
    } else if (strstr(buf, "*")) {
      search_replace(buf, "*", "");
    }
  } else if (subcmd == SCMD_EMOTE) {
    snprintf(buf, sizeof(buf), "$n %s", argument);
    search_replace(buf, "#", "$n");
    search_replace(buf, "&1", "'@C");
    search_replace(buf, "&2", "@w'");
    if (found == true) {
      search_replace(buf, name, "$N");
    }
  } else {
    snprintf(buf, sizeof(buf), "%s", argument);
  }
  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT)) {
    send_to_char(g, ch, "%s", CONFIG_OK);
  }
  if (NoName == true) {
    char blom[MAX_INPUT_LENGTH];
    snprintf(blom, sizeof(blom), "@D(@GOOC@W: @gSmote by user %s@D)@n", char_is_npc(ch) ? char_get_name(ch) : char_get_user(ch));
    act(g, blom, false, ch, 0, 0, TO_ROOM);
  }
  if (found == false) {
    act(g, buf, false, ch, 0, 0, TO_CHAR);
    act(g, buf, false, ch, 0, 0, TO_ROOM);
  } else {
    act(g, buf, false, ch, 0, tch, TO_CHAR);
    act(g, buf, false, ch, 0, tch, TO_NOTVICT);
    search_replace(buf, "$N", "you");
    act(g, buf, false, ch, 0, tch, TO_VICT);
  }
}

ACMD(do_send)
{
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char(g, ch, "Send what to who?\r\n");
    return;
  }
  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "%s", CONFIG_NOPERSON);
    return;
  }
  send_to_char(g, vict, "%s\r\n", buf);
  if (char_prf_flagged(g, ch, PRF_NOREPEAT))
    send_to_char(g, ch, "Sent.\r\n");
  else
    send_to_char(g, ch, "You send '%s' to %s.\r\n", buf, char_get_name(vict));
}

static const char *const logtypes[] = {
    "off", "brief", "normal", "complete", "\n"};

ACMD(do_syslog)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(g, ch, "Your syslog is currently %s.\r\n",
                 logtypes[(char_prf_flagged(g, ch, PRF_LOG1) ? 1 : 0) + (char_prf_flagged(g, ch, PRF_LOG2) ? 2 : 0)]);
    return;
  }
  if (((tp = search_block(arg, logtypes, false)) == -1)) {
    send_to_char(g, ch, "Usage: syslog { Off | Brief | Normal | Complete }\r\n");
    return;
  }
  char_prf_set(g, ch, PRF_LOG1, false);
  char_prf_set(g, ch, PRF_LOG2, false);
  if (tp & 1) char_prf_set(g, ch, PRF_LOG1, true);
  if (tp & 2) char_prf_set(g, ch, PRF_LOG2, true);

  send_to_char(g, ch, "Your syslog is now %s.\r\n", logtypes[tp]);
}

ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char(g, ch, "That must be a mistake...\r\n");
  else {
    for (pt = g->descriptors; pt; pt = pt->next)
      if (desc_is_playing(pt) && pt->character && pt->character != ch)
        send_to_char(g, pt->character, "%s\r\n", argument);

    if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT))
      send_to_char(g, ch, "%s", CONFIG_OK);
    else
      send_to_char(g, ch, "%s\r\n", argument);
  }
}

ACMD(do_ginfo)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char(g, ch, "That must be a mistake...\r\n");
  else {
    for (pt = g->descriptors; pt; pt = pt->next)
      if (desc_is_playing(pt) && pt->character && pt->character != ch)
        send_to_char(g, pt->character, "@D[@GINFO@D] @g%s@n\r\n", argument);

    if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT))
      send_to_char(g, ch, "%s", CONFIG_OK);
    else
      send_to_char(g, ch, "@D[@GINFO@D] @g%s@n\r\n", argument);
  }
}

ACMD(do_poofset)
{
  char **msg;

  if (char_is_npc(ch))
    return;
  switch (subcmd) {
    case SCMD_POOFIN: msg = &char_get_player_specials(ch)->poofin; break;
    case SCMD_POOFOUT: msg = &char_get_player_specials(ch)->poofout; break;
    default: return;
  }

  skip_spaces(&argument);

  if (*msg)
    free(*msg);

  if (!*argument)
    *msg = nullptr;
  else
    *msg = strdup(argument);

  send_to_char(g, ch, "%s", CONFIG_OK);
}

ACMD(do_wiznet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
      buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32], *msg;
  struct descriptor_data *d;
  char emote = false;
  int level  = ADMLVL_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(g, ch, "Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n        wiznet @<level> *<emotetext> | wiz @\r\n");
    return;
  }
  switch (*argument) {
    case '*':
      emote = true;
      [[fallthrough]];
    case '#':
      one_argument(argument + 1, buf1);
      if (is_number(buf1)) {
        half_chop(argument + 1, buf1, argument);
        level = MAX(atoi(buf1), ADMLVL_IMMORT);
        if (level > char_get_admlevel(ch)) {
          send_to_char(g, ch, "You can't wizline above your own level.\r\n");
          return;
        }
      } else if (emote)
        argument++;
      break;

    case '@':
      send_to_char(g, ch, "God channel status:\r\n");
      for (d = g->descriptors; d; d = d->next) {
        if (desc_get_connected(d) != CON_PLAYING || char_get_admlevel(d->character) < ADMLVL_IMMORT)
          continue;
        if (!char_can_see(g, ch, d->character))
          continue;

        send_to_char(g, ch, "  %-*s%s%s%s\r\n", MAX_NAME_LENGTH, char_get_name(d->character),
                     char_plr_flagged(d->character, PLR_WRITING) ? " (Writing)" : "",
                     char_plr_flagged(d->character, PLR_MAILING) ? " (Writing mail)" : "",
                     char_prf_flagged(g, d->character, PRF_NOWIZ) ? " (Offline)" : "");
      }
      return;

    case '\\':
      ++argument;
      break;
    default:
      break;
  }
  if (char_prf_flagged(g, ch, PRF_NOWIZ)) {
    send_to_char(g, ch, "You are offline!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(g, ch, "Don't bother the gods like that!\r\n");
    return;
  }
  if (level > ADMLVL_IMMORT) {
    snprintf(buf1, sizeof(buf1), "@c%s@D: <@C%d@D> @G%s%s@n\r\n", char_get_name(ch), level, emote ? "<--- " : "", argument);
    snprintf(buf2, sizeof(buf1), "@cSomeone@D: <@C%d@D> @G%s%s@n\r\n", level, emote ? "<--- " : "", argument);
  } else {
    snprintf(buf1, sizeof(buf1), "@c%s@D: @G%s%s@n\r\n", char_get_name(ch), emote ? "<--- " : "", argument);
    snprintf(buf2, sizeof(buf1), "@cSomeone@D: @G%s%s@n\r\n", emote ? "<--- " : "", argument);
  }

  for (d = g->descriptors; d; d = d->next) {
    if (desc_is_playing(d) && (char_get_admlevel(d->character) >= level) &&
        (!char_prf_flagged(g, d->character, PRF_NOWIZ)) &&
        (!char_plr_flagged(d->character, PLR_WRITING | PLR_MAILING)) && (d != char_get_desc(ch) || !(char_prf_flagged(g, d->character, PRF_NOREPEAT)))) {
      if (char_can_see(g, d->character, ch)) {
        msg = strdup(buf1);
        send_to_char(g, d->character, "%s", buf1);
      } else {
        msg = strdup(buf2);
        send_to_char(g, d->character, "%s", buf2);
      }
      add_history(g, d->character, msg, HIST_WIZNET);
    }
  }

  if (char_prf_flagged(g, ch, PRF_NOREPEAT))
    send_to_char(g, ch, "%s", CONFIG_OK);
}
