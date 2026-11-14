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
#include "dbat/cmdargs.hpp"
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

namespace {
  constexpr const char *NEWS_SYNTAX = "Syntax: news (number | list)\r\n";

  /* The entry numbered lookup in the news file, as shown: its heading, then its text with repeated lines left out.
   * Returns whether it was found. */
  bool read_news_entry(FILE *fl, int lookup, std::string &buf)
  {
    char line[256], title[256] = "";
    std::string lastline;
    int nr;
    bool found = false, done = false, first = true;

    while (!feof(fl) && !done) {
      get_line(fl, line);
      if (*line != '#' || sscanf(line, "#%d", &nr) != 1 || nr != lookup)
        continue;
      sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
      buf   = str_format("@w--------------------------------------------------------------\n@cNum@W: @D(@G{:>3d}@D)                @cTitle@W: @g{:<50}@n\n", nr, title);
      found = true;
      while (!feof(fl) && !done) {
        get_line(fl, line);
        if (*line == '#') {
          done = true;
        } else if (first) {
          first = false;
          buf += str_format("{}\n@w--------------------------------------------------------------\n", line);
          lastline = line;
        } else if (strcasecmp(line, lastline.c_str())) {
          buf += str_format("{}\n", line);
          lastline = line;
        }
      }
    }
    return found;
  }

  /* The headings of the news file's entries; returns how many there are */
  int list_news(FILE *fl, std::string &buf)
  {
    char line[256], title[256] = "";
    int nr, entries = 0;

    while (!feof(fl)) {
      get_line(fl, line);
      if (*line != '#')
        continue;
      entries++;
      if (sscanf(line, "#%d", &nr) != 1)
        continue;
      sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
      if (buf.empty())
        buf = "@wNews Entries (Newest at the bottom, to read an entry use 'news (number)')\n";
      buf += str_format("@D[@cNum@W: @D(@G{:>3d}@D) @cTitle@W: @g{:<50}@D]@n\n", nr, title);
    }
    return entries;
  }

  /* The number the next news entry takes, or 0 if the file's numbering is broken */
  int next_news_number(FILE *fl)
  {
    char line[256];
    int nr, entries = 0, lastentry = 0;

    while (!feof(fl)) {
      get_line(fl, line);
      if (*line != '#')
        continue;
      entries++;
      if (sscanf(line, "#%d", &nr) == 1)
        lastentry = nr;
    }
    if (lastentry)
      return lastentry + 1;
    return entries == 0 ? 1 : 0;
  }

  /* The first other person in the room the text names as "*Name" (or by the name ch knows them by), and that name */
  struct char_data *named_in(GameWorld *g, struct char_data *ch, const std::string &text, std::string &name)
  {
    for (char_data *vict : room_people(g, char_get_in_room(ch))) {
      if (vict == ch)
        continue;
      name = str_format("*{}", char_get_name(vict));
      if (text.contains(CAP(name)))
        return vict;
      if (!char_is_npc(vict) && readIntro(ch, vict) == 1) {
        name = str_format("*{}", get_i_name(ch, vict));
        if (text.contains(CAP(name)))
          return vict;
      }
    }
    return nullptr;
  }

  /* An emote's text, with its markup turned to act()'s */
  void emote_markup(std::string &buf, struct char_data *named, const std::string &name)
  {
    search_replace(buf, "#", "$n");
    search_replace(buf, "&1", "'@C");
    search_replace(buf, "&2", "@w'");
    if (named)
      search_replace(buf, name.c_str(), "$N");
  }

  /* text to everyone playing, between before and after, and to ch unless they don't want it repeated */
  void announce(GameWorld *g, struct char_data *ch, std::string_view argument, const char *before, const char *after)
  {
    std::string text(skip_spaces(argument));
    delete_doubledollar(text);
    if (text.empty()) {
      send_to_char(g, ch, "That must be a mistake...\r\n");
      return;
    }
    for (descriptor_data *pt : each_desc(g))
      if (desc_is_playing(pt) && pt->character && pt->character != ch)
        send_to_char(g, pt->character, "{}{}{}\r\n", before, text, after);
    if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT))
      send_to_char(g, ch, CONFIG_OK);
    else
      send_to_char(g, ch, "{}{}{}\r\n", before, text, after);
  }

  const char *const logtypes[] = {"off", "brief", "normal", "complete", "\n"};
} // namespace

/* Read the news entry */
ACMD(do_news)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, NEWS_SYNTAX);
    return;
  }

  int lookup = parse_int(arg);
  FILE *fl   = fopen(NEWS_FILE, "r");
  if (!fl) {
    mud_log("SYSERR: opening news file for reading");
    return;
  }

  std::string buf;
  if (lookup > 0) {
    bool found = read_news_entry(fl, lookup, buf);
    fclose(fl);
    if (!found) {
      send_to_char(g, ch, "That news entry does not exist.\r\n");
      return;
    }
    send_to_char(g, ch, "{}\r\n", buf);
    char_set_lastpl(ch, game_now(g));
    char_set_wait(ch, PULSE_1SEC);
    return;
  }
  if (!str_eq_nocase(arg, "list")) {
    fclose(fl);
    send_to_char(g, ch, NEWS_SYNTAX);
    return;
  }
  int entries = list_news(fl, buf);
  fclose(fl);
  if (entries <= 0) {
    send_to_char(g, ch, "The news file is empty right now.\r\n");
    return;
  }
  char_set_lastpl(ch, game_now(g));
  char_set_wait(ch, PULSE_1SEC);
  send_to_char(g, ch, buf);
}

/* Write the news entry */
ACMD(do_newsedit)
{
  if (char_get_admlevel(ch) < 1 || char_is_npc(ch))
    return;

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

  FILE *fl = fopen(NEWS_FILE, "r");
  if (!fl) {
    mud_log("SYSERR: Couldn't open news file for reading");
    return;
  }
  int lookup = next_news_number(fl);
  fclose(fl);
  if (!lookup) {
    send_to_imm(g, "ERROR: News file entries are disorganized. Report to Iovan for analysis.\r\n");
    return;
  }

  act(g, "$n begins to edit the news.", true, ch, nullptr, nullptr, TO_ROOM);
  send_to_char(g, ch, "@D----------------------=[@GNews Edit@D]=----------------------@n\n");
  send_to_char(g, ch, " @RRemember that using # in newsedit is not possible. That\n");
  send_to_char(g, ch, "character will be eaten because it is required for the news\n");
  send_to_char(g, ch, "file as a delimiter. Also if you want to create an empty line\n");
  send_to_char(g, ch, "between paragraphs you will need to enter a single space and\n");
  send_to_char(g, ch, "not just push enter. Happy editing!@n\n");
  send_to_char(g, ch, "@D---------------------------------------------------------@n\n");
  struct descriptor_data *d = char_get_desc(ch);
  send_editor_help(g, d);
  d->newsbuf     = skip_spaces(argument);
  g->TOP_OF_NEWS = lookup;
  g->LASTNEWS    = lookup;
  d->draft.clear();
  string_write(d, &d->draft, 2000, 0);
  desc_set_connected(d, CON_NEWSEDIT);
}

ACMD(do_echo)
{
  std::string text(skip_spaces(argument));
  if (text.empty()) {
    send_to_char(g, ch, "Yes.. but what?\r\n");
    return;
  }

  std::string name;
  struct char_data *tch = named_in(g, ch, text, name);
  bool no_name          = false;
  std::string buf;

  if (subcmd == SCMD_SMOTE) {
    no_name = text.find('#') == std::string::npos;
    buf     = text;
    emote_markup(buf, tch, name);
    if (!tch && buf.find('*') != std::string::npos)
      search_replace(buf, "*", "");
  } else if (subcmd == SCMD_EMOTE) {
    buf = "$n " + text;
    emote_markup(buf, tch, name);
  } else {
    buf = text;
  }
  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NOREPEAT))
    send_to_char(g, ch, CONFIG_OK);
  if (no_name)
    act(g, str_format("@D(@GOOC@W: @gSmote by user {}@D)@n", char_is_npc(ch) ? char_get_name(ch) : char_get_user(ch)).c_str(), false, ch, nullptr, nullptr, TO_ROOM);
  if (!tch) {
    act(g, buf.c_str(), false, ch, nullptr, nullptr, TO_CHAR);
    act(g, buf.c_str(), false, ch, nullptr, nullptr, TO_ROOM);
    return;
  }
  act(g, buf.c_str(), false, ch, nullptr, tch, TO_CHAR);
  act(g, buf.c_str(), false, ch, nullptr, tch, TO_NOTVICT);
  search_replace(buf, "$N", "you");
  act(g, buf.c_str(), false, ch, nullptr, tch, TO_VICT);
}

ACMD(do_send)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string buf(skip_spaces(rest));

  if (arg.empty()) {
    send_to_char(g, ch, "Send what to who?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_WORLD, CONFIG_NOPERSON);
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  send_to_char(g, vict, "{}\r\n", buf);
  if (char_prf_flagged(g, ch, PRF_NOREPEAT))
    send_to_char(g, ch, "Sent.\r\n");
  else
    send_to_char(g, ch, "You send '{}' to {}.\r\n", buf, char_get_name(vict));
}

ACMD(do_syslog)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Your syslog is currently {}.\r\n",
                 logtypes[(char_prf_flagged(g, ch, PRF_LOG1) ? 1 : 0) + (char_prf_flagged(g, ch, PRF_LOG2) ? 2 : 0)]);
    return;
  }
  int tp = search_block(arg, logtypes, false);
  if (tp == -1) {
    send_to_char(g, ch, "Usage: syslog { Off | Brief | Normal | Complete }\r\n");
    return;
  }
  char_prf_set(g, ch, PRF_LOG1, (tp & 1) != 0);
  char_prf_set(g, ch, PRF_LOG2, (tp & 2) != 0);
  send_to_char(g, ch, "Your syslog is now {}.\r\n", logtypes[tp]);
}

ACMD(do_gecho)
{
  announce(g, ch, argument, "", "");
}

ACMD(do_ginfo)
{
  announce(g, ch, argument, "@D[@GINFO@D] @g", "@n");
}

ACMD(do_poofset)
{
  if (char_is_npc(ch))
    return;

  std::string *msg;
  switch (subcmd) {
    case SCMD_POOFIN: msg = &char_get_player_specials(ch)->poofin; break;
    case SCMD_POOFOUT: msg = &char_get_player_specials(ch)->poofout; break;
    default: return;
  }
  *msg = skip_spaces(argument);
  send_to_char(g, ch, CONFIG_OK);
}

ACMD(do_wiznet)
{
  bool emote = false;
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
        if (desc_get_connected(d) != CON_PLAYING || char_get_admlevel(d->character) < ADMLVL_IMMORT || !char_can_see(g, ch, d->character))
          continue;
        send_to_char(g, ch, "  {}{}{}{}\r\n", printf_width(char_get_name(d->character), MAX_NAME_LENGTH, true),
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

  std::string level_tag = level > ADMLVL_IMMORT ? str_format("<@C{:d}@D> ", level) : "";
  std::string seen      = str_format("@c{}@D: {}@G{}{}@n\r\n", char_get_name(ch), level_tag, emote ? "<--- " : "", said);
  std::string unseen    = str_format("@cSomeone@D: {}@G{}{}@n\r\n", level_tag, emote ? "<--- " : "", said);

  for (descriptor_data *d : each_desc(g)) {
    if (!desc_is_playing(d) || char_get_admlevel(d->character) < level || char_prf_flagged(g, d->character, PRF_NOWIZ) ||
        char_plr_flagged(d->character, PLR_WRITING | PLR_MAILING) || (d == char_get_desc(ch) && char_prf_flagged(g, d->character, PRF_NOREPEAT)))
      continue;
    const std::string &line = char_can_see(g, d->character, ch) ? seen : unseen;
    send_to_char(g, d->character, line);
    add_history(g, d->character, line.c_str(), HIST_WIZNET);
  }

  if (char_prf_flagged(g, ch, PRF_NOREPEAT))
    send_to_char(g, ch, CONFIG_OK);
}
