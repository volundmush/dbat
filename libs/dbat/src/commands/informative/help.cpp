/* help.c: help, commands, toggles, colour, history, time and weather
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/utils.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/consts/time.hpp"
#include "dbat/help.hpp"
#include "dbat/command.hpp"
#include "dbat/flags.hpp"
#include "dbat/affect.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/random.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/fileop.hpp"
#include "dbat/races.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.social.hpp"
#include "dbat/maputils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/boards.hpp"
#include "dbat/screen.hpp"
#include "dbat/mail.hpp"
#include "dbat/guild.hpp"
#include "dbat/disabled.hpp"
#include "dbat/game_world.hpp"

#include "informative_internal.hpp"

static int sort_commands_helper(const void *a, const void *b, void *world);

namespace {
  /* 1st, 2nd, 3rd, 4th ... 11th, 12th, 13th ... 21st */
  const char *ordinal_suffix(int n)
  {
    if ((n % 100) / 10 == 1)
      return "th";
    switch (n % 10) {
      case 1:
        return "st";
      case 2:
        return "nd";
      case 3:
        return "rd";
      default:
        return "th";
    }
  }
} // namespace

ACMD(do_time)
{
  int day     = g->time_info.day + 1; /* day in [1..30] */
  int weekday = day % 6;              /* 30 days in a month, 6 days a week */

  send_to_char(g, ch, "It is {:d} o'clock {}, on {}.\r\n",
               (g->time_info.hours % 12 == 0) ? 12 : (g->time_info.hours % 12),
               g->time_info.hours >= 12 ? "PM" : "AM", weekdays[weekday]);

  send_to_char(g, ch, "The {:d}{} Day of the {}, Year {:d}.\r\n",
               day, ordinal_suffix(day), month_name[g->time_info.month], g->time_info.year);
}

ACMD(do_weather)
{
  static constexpr const char *sky_look[] = {"cloudless", "cloudy", "rainy", "lit by flashes of lightning"};

  if (char_is_outside(g, ch)) {
    send_to_char(g, ch, "The sky is {} and {}.\r\n", sky_look[g->weather_info.sky],
                 g->weather_info.change >= 0 ? "you feel a warm wind from south" : "your foot tells you bad weather is due");
    if (char_adm_flagged(ch, ADM_KNOWWEATHER))
      send_to_char(g, ch, "Pressure: {:d} (change: {:d}), Sky: {:d} ({})\r\n",
                   g->weather_info.pressure,
                   g->weather_info.change,
                   g->weather_info.sky,
                   sky_look[g->weather_info.sky]);
  } else
    send_to_char(g, ch, "You have no feeling about the weather at all.\r\n");
}

int search_help(GameWorld *g, const char *argument, int level)
{
  int chk, bot, top, mid, minlen;

  bot    = 0;
  top    = (int)g->help_table.size() - 1;
  minlen = strlen(argument);

  while (bot <= top) {
    mid = (bot + top) / 2;

    if (!(chk = strncasecmp(argument, g->help_table[mid].keywords.c_str(), minlen))) {
      while ((mid > 0) && !strncasecmp(argument, g->help_table[mid - 1].keywords.c_str(), minlen))
        mid--;

      while (level < g->help_table[mid].min_level && mid < (bot + top) / 2)
        mid++;

      if (strncasecmp(argument, g->help_table[mid].keywords.c_str(), minlen))
        break;

      return mid;
    } else if (chk > 0)
      bot = mid + 1;
    else
      top = mid - 1;
  }
  return NOWHERE;
}

ACMD(do_help)
{
  if (!char_get_desc(ch))
    return;

  std::string text(skip_spaces(argument));

  if (g->help_table.empty()) {
    send_to_char(g, ch, "No help available.\r\n");
    return;
  }

  if (text.empty()) {
    if (char_get_admlevel(ch) < ADMLVL_IMMORT)
      send_to_char(g, ch, g->help);
    else
      send_to_char(g, ch, g->ihelp);
    return;
  }

  std::ranges::replace(text, ' ', '-');

  int mid = search_help(g, text.c_str(), char_get_admlevel(ch));
  if (mid == NOWHERE) {
    send_to_char(g, ch, "There is no help on that word.\r\n");
    if (char_get_admlevel(ch) < 3)
      mudlog(g, LOG_NRM, MAX(ADMLVL_IMPL, char_is_npc(ch) ? 0 : char_get_invis_level(g, ch)), true, "{} tried to get help on {}", char_get_name(ch), text);
    /* the entries ch may read that start with the same letter and are within two edits of it */
    bool found = false;
    for (const auto &entry : g->help_table) {
      if (entry.min_level > char_get_admlevel(ch) || text[0] != *entry.keywords.c_str())
        continue;
      if (levenshtein_distance(text.c_str(), entry.keywords.c_str()) > 2)
        continue;
      if (!found)
        send_to_char(g, ch, "\r\nDid you mean:\r\n");
      found = true;
      send_to_char(g, ch, "  {}\r\n", entry.keywords);
    }
    return;
  }
  if (g->help_table[mid].min_level > char_get_admlevel(ch)) {
    send_to_char(g, ch, "There is no help on that word.\r\n");
    return;
  }
  std::string buf = "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n";
  buf += g->help_table[mid].entry;
  buf += "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n";
  if (char_get_admlevel(ch) > 0)
    buf += str_format("@WHelp File Level@w: @D(@R{:d}@D)@n\n", g->help_table[mid].min_level);
  send_to_char(g, ch, buf);
}

/* Shows one of the fixed text files (credits, news, motd, ...) */
ACMD(do_gen_ps)
{
  switch (subcmd) {
    case SCMD_CREDITS:
      send_to_char(g, ch, g->credits);
      break;
    case SCMD_NEWS:
      send_to_char(g, ch, g->news);
      char_set_lastpl(ch, game_now(g));
      break;
    case SCMD_INFO:
      send_to_char(g, ch, g->info);
      break;
    case SCMD_WIZLIST:
      send_to_char(g, ch, g->wizlist);
      break;
    case SCMD_IMMLIST:
      send_to_char(g, ch, g->immlist);
      break;
    case SCMD_HANDBOOK:
      send_to_char(g, ch, g->handbook);
      break;
    case SCMD_POLICIES:
      send_to_char(g, ch, str_format("--------------------\r\n{}\r\n--------------------\r\n", g->policies));
      break;
    case SCMD_MOTD:
      send_to_char(g, ch, g->motd);
      break;
    case SCMD_IMOTD:
      send_to_char(g, ch, g->imotd);
      break;
    case SCMD_CLEAR:
      send_to_char(g, ch, "\033[H\033[J");
      break;
    case SCMD_VERSION:
      break;
    case SCMD_WHOAMI:
      send_to_char(g, ch, "{}\r\n", char_get_name(ch));
      break;
    default:
      mud_log("SYSERR: Unhandled case in do_gen_ps. ({:d})", subcmd);
      /*  SYSERR_DESC:
     *  General page string function for such things as 'credits', 'news',
     *  'wizlist', 'clear', 'version'.  This occurs when a call is made to
     *  this routine that is not one of the predefined calls.  To correct
     *  it, either a case needs to be added into the function to account for
     *  the subcmd that is being passed to it, or the call to the function
     *  needs to have the correct subcmd put into place.
     */
      return;
  }
}

static const char *const ctypes[] = {"off", "on", "\n"}; /* by C_OFF and C_ON */

ACMD(do_color)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);

  int tp = arg.empty() ? -1 : search_block(arg, ctypes, false);
  if (tp == -1) {
    send_to_char(g, ch, "Usage: color [ off | on ]\r\n");
    return;
  }
  char_prf_set(g, ch, PRF_COLOR, tp == C_ON);
  send_to_char(g, ch, "Your color is now @o{}@n.\r\n", ctypes[tp]);
}

ACMD(do_toggle)
{
  if (char_is_npc(ch))
    return;

  std::string wimp = char_get_wimp_level(g, ch) == 0 ? "OFF" : str_format("{:<3}", printf_digits(char_get_wimp_level(g, ch), 3));

  if (char_get_admlevel(ch)) {
    send_to_char(g, ch,
                 "      Buildwalk: {:<3}    Clear Screen in OLC: {:<3}\r\n",
                 ONOFF(char_prf_flagged(g, ch, PRF_BUILDWALK)),
                 ONOFF(char_prf_flagged(g, ch, PRF_CLS)));

    send_to_char(g, ch,
                 "      No Hassle: {:<3}          Holylight: {:<3}         Room Flags: {:<3}\r\n",
                 ONOFF(char_prf_flagged(g, ch, PRF_NOHASSLE)),
                 ONOFF(char_prf_flagged(g, ch, PRF_HOLYLIGHT)),
                 ONOFF(char_prf_flagged(g, ch, PRF_ROOMFLAGS)));
  }

  send_to_char(g, ch,
               "Hit Pnt Display: {:<3}         Brief Mode: {:<3}     Summon Protect: {:<3}\r\n   Move Display: {:<3}       Compact Mode: {:<3}           On Quest: {:<3}\r\n    Exp Display: {:<3}             NoTell: {:<3}       Repeat Comm.: {:<3}\r\n     Ki Display: {:<3}               Deaf: {:<3}         Wimp Level: {:<3}\r\n Gossip Channel: {:<3}    Auction Channel: {:<3}      Grats Channel: {:<3}\r\n      Auto Loot: {:<3}          Auto Gold: {:<3}        Color Level: {}\r\n     Auto Split: {:<3}           Auto Sac: {:<3}           Auto Mem: {:<3}\r\n     View Order: {:<3}        Auto Assist: {:<3}     Auto Show Exit: {:<3}\r\n    TNL Display: {:<3}    ",

               ONOFF(char_prf_flagged(g, ch, PRF_DISPHP)),
               ONOFF(char_prf_flagged(g, ch, PRF_BRIEF)),
               ONOFF(!char_prf_flagged(g, ch, PRF_SUMMONABLE)),

               ONOFF(char_prf_flagged(g, ch, PRF_DISPMOVE)),
               ONOFF(char_prf_flagged(g, ch, PRF_COMPACT)),
               YESNO(char_prf_flagged(g, ch, PRF_QUEST)),

               ONOFF(char_prf_flagged(g, ch, PRF_DISPEXP)),
               ONOFF(char_prf_flagged(g, ch, PRF_NOTELL)),
               YESNO(!char_prf_flagged(g, ch, PRF_NOREPEAT)),

               ONOFF(char_prf_flagged(g, ch, PRF_DISPKI)),
               YESNO(char_prf_flagged(g, ch, PRF_DEAF)),
               wimp,

               ONOFF(!char_prf_flagged(g, ch, PRF_NOGOSS)),
               ONOFF(!char_prf_flagged(g, ch, PRF_NOAUCT)),
               ONOFF(!char_prf_flagged(g, ch, PRF_NOGRATZ)),

               ONOFF(char_prf_flagged(g, ch, PRF_AUTOLOOT)),
               ONOFF(char_prf_flagged(g, ch, PRF_AUTOGOLD)),
               ctypes[COLOR_LEV(g, ch)],

               ONOFF(char_prf_flagged(g, ch, PRF_AUTOSPLIT)),
               ONOFF(char_prf_flagged(g, ch, PRF_AUTOSAC)),
               ONOFF(char_prf_flagged(g, ch, PRF_AUTOMEM)),

               ONOFF(char_prf_flagged(g, ch, PRF_VIEWORDER)),
               ONOFF(char_prf_flagged(g, ch, PRF_AUTOASSIST)),
               ONOFF(char_prf_flagged(g, ch, PRF_AUTOEXIT)),

               ONOFF(char_prf_flagged(g, ch, PRF_DISPTNL)));

  if (CONFIG_ENABLE_COMPRESSION)
    send_to_char(g, ch, "    Compression: {:<3}\r\n", ONOFF(!char_prf_flagged(g, ch, PRF_NOCOMPRESS)));
}

static int sort_commands_helper(const void *a, const void *b, void *world)
{
  GameWorld *g = static_cast<GameWorld *>(world);
  return strcmp(g->complete_cmd_info[*(const int *)a].sort_as,
                g->complete_cmd_info[*(const int *)b].sort_as);
}

void sort_commands(GameWorld *g)
{
  int a, num_of_cmds = 0;

  while (g->complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++; /* \n */

  delete[] g->cmd_sort_info; /* the previous list's, when aedit rebuilds the commands */
  g->cmd_sort_info = new int[num_of_cmds]();

  for (a = 0; a < num_of_cmds; a++)
    g->cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort_r(g->cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper, g);
}

ACMD(do_commands)
{
  std::string_view rest  = argument;
  std::string arg        = take_arg(rest);
  struct char_data *vict = ch;

  if (!arg.empty()) {
    auto who = find_char(g, ch, arg, FIND_CHAR_WORLD, "Who is that?\r\n").and_then([](struct char_data *v) -> parsed<struct char_data *> {
      if (char_is_npc(v))
        return std::unexpected("Who is that?\r\n");
      return v;
    });
    if (reject(g, ch, who))
      return;
    vict = *who;
    if (char_get_level(ch) < char_get_level(vict)) {
      send_to_char(g, ch, "You can't see the commands of people above your level.\r\n");
      return;
    }
  }

  bool socials = subcmd == SCMD_SOCIALS;
  bool wizhelp = subcmd == SCMD_WIZHELP;

  send_to_char(g, ch, "The following {}{} are available to {}:\r\n",
               wizhelp ? "privileged " : "",
               socials ? "socials" : "commands",
               vict == ch ? "you" : char_get_name(vict));

  /* in sorted order, from 1 to pass over 'RESERVED' */
  int no = 1;
  for (int cmd_num = 1; g->complete_cmd_info[g->cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) {
    const auto &info = g->complete_cmd_info[g->cmd_sort_info[cmd_num]];

    if (info.minimum_level < 0 || char_get_level(vict) < info.minimum_level)
      continue;
    if (info.minimum_admlevel < 0 || char_get_admlevel(vict) < info.minimum_admlevel)
      continue;
    if ((info.minimum_admlevel >= ADMLVL_IMMORT) != wizhelp)
      continue;
    if (!wizhelp && socials != (info.command_pointer == do_action || info.command_pointer == do_insult))
      continue;

    std::string label = check_disabled(g, &info) ? str_format("({})", info.command) : std::string(info.command);
    send_to_char(g, ch, "{:<11}{}", label, no++ % 7 == 0 ? "\r\n" : "");
  }

  if (no % 7 != 1)
    send_to_char(g, ch, "\r\n");
}

ACMD(do_history)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  int type = search_block(arg, history_types, false);
  if (arg.empty() || type < 0) {
    send_to_char(g, ch, "Usage: history <");
    for (int i = 0; *history_types[i] != '\n'; i++) {
      bool shown = i != 3 || char_get_admlevel(ch) >= 1; /* the fourth is for immortals */
      if (shown)
        send_to_char(g, ch, " {} ", history_types[i]);
      if (*history_types[i + 1] == '\n')
        send_to_char(g, ch, ">\r\n");
      else if (shown)
        send_to_char(g, ch, "|");
    }
    return;
  }

  std::deque<std::string> *history = char_comm_hist(g, ch, type);
  if (!history || history->empty() || history->front().empty()) {
    send_to_char(g, ch, "You have no history in that channel.\r\n");
    return;
  }
  for (const std::string &line : *history)
    send_to_char(g, ch, line);
}

void add_history(GameWorld *g, struct char_data *ch, const char *str, int type)
{
  char time_str[MAX_STRING_LENGTH];
  std::string buf;
  std::deque<std::string> *history;
  time_t ct;

  if (char_is_npc(ch) || !(history = char_comm_hist(g, ch, type)))
    return;

  ct = game_now(g);
  strftime(time_str, sizeof(time_str), "%H:%M ", localtime(&ct));

  buf = str_format("{}{}", time_str, str);

  history->push_back(buf.c_str());
  while (history->size() > HIST_LENGTH)
    history->pop_front();
  /* add this history message to ALL */
  if (type != HIST_ALL)
    add_history(g, ch, str, HIST_ALL);
}
