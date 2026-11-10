/* ************************************************************************
*   File: wizard/comm.c                                  Part of CircleMUD *
*  Usage: God communication and news commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <climits>
#include <cstdlib>

#include <unistd.h>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/players.hpp"
#include "dbat/utils.hpp"
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
#include "dbat/objsave.hpp"
#include "dbat/feats.hpp"
#include "dbat/fight.hpp"
#include "dbat/genolc.hpp"
#include "dbat/screen.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/shop.hpp"
#include "dbat/guild.hpp"
#include "dbat/spell_parser.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"

/* Read the news entry */
ACMD(do_news)
{

  if (char_is_npc(ch)) {
    return;
  }

  FILE *fl;
  const char *filename;
  char line[256];
  char buf[MAX_STRING_LENGTH], title[256], lastline[256];
  std::string arg;
  int entries = 0, lookup = 0, nr, found = false, first = true, exit = false;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  filename = NEWS_FILE;

  /* Check the file for the entry so we may edit it if need be*/

  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: news (number | list)\r\n");
    return;
  }

  lookup = parse_int(arg);

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
  } else if (str_eq_nocase(arg, "list")) {
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

  if (argument.empty()) {
    send_to_char(g, ch, "Syntax: newsedit (title)\r\n");
    return;
  }
  if (argument.size() > 50) {
    send_to_char(g, ch, "Limit of 50 characters for title.\r\n");
    return;
  }
  if (argument.contains("#")) {
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
  char *tmstr;
  time_t mytime                = game_now(g);
  tmstr                        = asctime(localtime(&mytime));
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
  act(g, "$n begins to edit the news.", true, ch, nullptr, nullptr, TO_ROOM);
  send_to_char(g, ch, "@D----------------------=[@GNews Edit@D]=----------------------@n\n");
  send_to_char(g, ch, " @RRemember that using # in newsedit is not possible. That\n");
  send_to_char(g, ch, "character will be eaten because it is required for the news\n");
  send_to_char(g, ch, "file as a delimiter. Also if you want to create an empty line\n");
  send_to_char(g, ch, "between paragraphs you will need to enter a single space and\n");
  send_to_char(g, ch, "not just push enter. Happy editing!@n\n");
  send_to_char(g, ch, "@D---------------------------------------------------------@n\n");
  send_editor_help(g, char_get_desc(ch));
  char_get_desc(ch)->newsbuf = skip_spaces(argument);
  g->TOP_OF_NEWS       = lookup;
  g->LASTNEWS          = lookup;
  char_get_desc(ch)->draft.clear();
  string_write(char_get_desc(ch), &char_get_desc(ch)->draft, 2000, 0);
  desc_set_connected(char_get_desc(ch), CON_NEWSEDIT);
}

ACMD(do_echo)
{
  std::string text(skip_spaces(argument));
  bool NoName = false;

  if (text.empty()) {
    send_to_char(g, ch, "Yes.. but what?\r\n");
    return;
  }

  std::string buf;
  char name[128];
  int found              = false;
  struct char_data *tch  = nullptr;

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch)
      continue;
    if (found == false) {
      snprintf(name, sizeof(name), "*%s", char_get_name(vict));
      if (strstr(text.c_str(), CAP(name))) {
        found = true;
        tch   = vict;
      }
      if (found == false && !char_is_npc(vict)) {
        if (readIntro(ch, vict) == 1) {
          snprintf(name, sizeof(name), "*%s", get_i_name(ch, vict));
          if (strstr(text.c_str(), CAP(name))) {
            found = true;
            tch   = vict;
          }
        }
      }
    }
  }

  if (subcmd == SCMD_SMOTE) {
    if (text.find('#') == std::string::npos) {
      NoName = true;
    }
    buf = text;
    search_replace(buf, "#", "$n");
    search_replace(buf, "&1", "'@C");
    search_replace(buf, "&2", "@w'");
    if (found == true) {
      search_replace(buf, name, "$N");
    } else if (buf.find('*') != std::string::npos) {
      search_replace(buf, "*", "");
    }
  } else if (subcmd == SCMD_EMOTE) {
    buf = "$n " + text;
    search_replace(buf, "#", "$n");
    search_replace(buf, "&1", "'@C");
    search_replace(buf, "&2", "@w'");
    if (found == true) {
      search_replace(buf, name, "$N");
    }
  } else {
    buf = text;
  }
  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT)) {
    send_to_char(g, ch, "%s", CONFIG_OK.c_str());
  }
  if (NoName == true) {
    char blom[MAX_INPUT_LENGTH];
    snprintf(blom, sizeof(blom), "@D(@GOOC@W: @gSmote by user %s@D)@n", char_is_npc(ch) ? char_get_name(ch) : char_get_user(ch));
    act(g, blom, false, ch, nullptr, nullptr, TO_ROOM);
  }
  if (found == false) {
    act(g, buf.c_str(), false, ch, nullptr, nullptr, TO_CHAR);
    act(g, buf.c_str(), false, ch, nullptr, nullptr, TO_ROOM);
  } else {
    act(g, buf.c_str(), false, ch, nullptr, tch, TO_CHAR);
    act(g, buf.c_str(), false, ch, nullptr, tch, TO_NOTVICT);
    search_replace(buf, "$N", "you");
    act(g, buf.c_str(), false, ch, nullptr, tch, TO_VICT);
  }
}

ACMD(do_send)
{
  std::string arg, buf;
  struct char_data *vict;

  std::string_view rest = argument;
  arg                   = take_word(rest);
  buf                   = skip_spaces(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Send what to who?\r\n");
    return;
  }
  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "%s", CONFIG_NOPERSON.c_str());
    return;
  }
  send_to_char(g, vict, "%s\r\n", buf.c_str());
  if (char_prf_flagged(g, ch, PRF_NOREPEAT))
    send_to_char(g, ch, "Sent.\r\n");
  else
    send_to_char(g, ch, "You send '%s' to %s.\r\n", buf.c_str(), char_get_name(vict));
}

static const char *const logtypes[] = {
    "off", "brief", "normal", "complete", "\n"};

ACMD(do_syslog)
{
  std::string arg;
  int tp;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Your syslog is currently %s.\r\n",
                 logtypes[(char_prf_flagged(g, ch, PRF_LOG1) ? 1 : 0) + (char_prf_flagged(g, ch, PRF_LOG2) ? 2 : 0)]);
    return;
  }
  if ((tp = search_block(arg, logtypes, false)) == -1) {
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

  std::string text(skip_spaces(argument));
  delete_doubledollar(text);

  if (text.empty())
    send_to_char(g, ch, "That must be a mistake...\r\n");
  else {
    for (descriptor_data *pt : each_desc(g))
      if (desc_is_playing(pt) && pt->character && pt->character != ch)
        send_to_char(g, pt->character, "%s\r\n", text.c_str());

    if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT))
      send_to_char(g, ch, "%s", CONFIG_OK.c_str());
    else
      send_to_char(g, ch, "%s\r\n", text.c_str());
  }
}

ACMD(do_ginfo)
{

  std::string text(skip_spaces(argument));
  delete_doubledollar(text);

  if (text.empty())
    send_to_char(g, ch, "That must be a mistake...\r\n");
  else {
    for (descriptor_data *pt : each_desc(g))
      if (desc_is_playing(pt) && pt->character && pt->character != ch)
        send_to_char(g, pt->character, "@D[@GINFO@D] @g%s@n\r\n", text.c_str());

    if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT))
      send_to_char(g, ch, "%s", CONFIG_OK.c_str());
    else
      send_to_char(g, ch, "@D[@GINFO@D] @g%s@n\r\n", text.c_str());
  }
}

ACMD(do_poofset)
{
  std::string *msg;

  if (char_is_npc(ch))
    return;
  switch (subcmd) {
    case SCMD_POOFIN: msg = &char_get_player_specials(ch)->poofin; break;
    case SCMD_POOFOUT: msg = &char_get_player_specials(ch)->poofout; break;
    default: return;
  }

  *msg = skip_spaces(argument);

  send_to_char(g, ch, "%s", CONFIG_OK.c_str());
}

ACMD(do_wiznet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
      buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
  char emote = false;
  int level  = ADMLVL_IMMORT;

  std::string text(skip_spaces(argument));
  delete_doubledollar(text);
  std::string_view msg = text; /* what's left to send */

  if (msg.empty()) {
    send_to_char(g, ch, "Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n        wiznet @<level> *<emotetext> | wiz @\r\n");
    return;
  }
  switch (msg[0]) {
    case '*':
      emote = true;
      [[fallthrough]];
    case '#': {
      std::string_view after = msg.substr(1), peek = after;
      if (is_number(take_arg(peek))) { /* fill words are passed over here, but not in the level taken below */
        std::string number = take_word(after);
        msg                = skip_spaces(after);
        level              = MAX(parse_int(number), ADMLVL_IMMORT);
        if (level > char_get_admlevel(ch)) {
          send_to_char(g, ch, "You can't wizline above your own level.\r\n");
          return;
        }
      } else if (emote)
        msg.remove_prefix(1);
      break;
    }

    case '@':
      send_to_char(g, ch, "God channel status:\r\n");
      for (descriptor_data *d : each_desc(g)) {
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
      msg.remove_prefix(1);
      break;
    default:
      break;
  }
  if (char_prf_flagged(g, ch, PRF_NOWIZ)) {
    send_to_char(g, ch, "You are offline!\r\n");
    return;
  }
  std::string said(skip_spaces(msg));

  if (said.empty()) {
    send_to_char(g, ch, "Don't bother the gods like that!\r\n");
    return;
  }
  if (level > ADMLVL_IMMORT) {
    snprintf(buf1, sizeof(buf1), "@c%s@D: <@C%d@D> @G%s%s@n\r\n", char_get_name(ch), level, emote ? "<--- " : "", said.c_str());
    snprintf(buf2, sizeof(buf1), "@cSomeone@D: <@C%d@D> @G%s%s@n\r\n", level, emote ? "<--- " : "", said.c_str());
  } else {
    snprintf(buf1, sizeof(buf1), "@c%s@D: @G%s%s@n\r\n", char_get_name(ch), emote ? "<--- " : "", said.c_str());
    snprintf(buf2, sizeof(buf1), "@cSomeone@D: @G%s%s@n\r\n", emote ? "<--- " : "", said.c_str());
  }

  for (descriptor_data *d : each_desc(g)) {
    if (desc_is_playing(d) && (char_get_admlevel(d->character) >= level) &&
        (!char_prf_flagged(g, d->character, PRF_NOWIZ)) &&
        (!char_plr_flagged(d->character, PLR_WRITING | PLR_MAILING)) && (d != char_get_desc(ch) || !(char_prf_flagged(g, d->character, PRF_NOREPEAT)))) {
      if (char_can_see(g, d->character, ch)) {
        send_to_char(g, d->character, "%s", buf1);
        add_history(g, d->character, buf1, HIST_WIZNET);
      } else {
        send_to_char(g, d->character, "%s", buf2);
        add_history(g, d->character, buf2, HIST_WIZNET);
      }
    }
  }

  if (char_prf_flagged(g, ch, PRF_NOREPEAT))
    send_to_char(g, ch, "%s", CONFIG_OK.c_str());
}
