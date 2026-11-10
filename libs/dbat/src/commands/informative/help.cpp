/* help.c: help, commands, toggles, colour, history, time and weather
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include <cstdlib>

#include "dbat/args.hpp"
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
static void space_to_minus(char *str);

/* local globals */

ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  /* day in [1..30] */
  day = g->time_info.day + 1;

  /* 30 days in a month, 6 days a week */
  weekday = day % 6;

  send_to_char(g, ch, "It is %d o'clock %s, on %s.\r\n",
               (g->time_info.hours % 12 == 0) ? 12 : (g->time_info.hours % 12),
               g->time_info.hours >= 12 ? "PM" : "AM", weekdays[weekday]);

  /*
   * Peter Ajamian <peter@PAJAMIAN.DHS.ORG> supplied the following as a fix
   * for a bug introduced in the ordinal display that caused 11, 12, and 13
   * to be incorrectly displayed as 11st, 12nd, and 13rd.  Nate Winters
   * <wintersn@HOTMAIL.COM> had already submitted a fix, but it hard-coded a
   * limit on ordinal display which I want to avoid.	-dak
   */

  suf = "th";

  if (((day % 100) / 10) != 1) {
    switch (day % 10) {
      case 1:
        suf = "st";
        break;
      case 2:
        suf = "nd";
        break;
      case 3:
        suf = "rd";
        break;
    }
  }

  send_to_char(g, ch, "The %d%s Day of the %s, Year %d.\r\n",
               day, suf, month_name[g->time_info.month], g->time_info.year);
}

ACMD(do_weather)
{
  const char *sky_look[] = {
      "cloudless",
      "cloudy",
      "rainy",
      "lit by flashes of lightning"};

  if (char_is_outside(g, ch)) {
    send_to_char(g, ch, "The sky is %s and %s.\r\n", sky_look[g->weather_info.sky],
                 g->weather_info.change >= 0 ? "you feel a warm wind from south" : "your foot tells you bad weather is due");
    if (char_adm_flagged(ch, ADM_KNOWWEATHER))
      send_to_char(g, ch, "Pressure: %d (change: %d), Sky: %d (%s)\r\n",
                   g->weather_info.pressure,
                   g->weather_info.change,
                   g->weather_info.sky,
                   sky_look[g->weather_info.sky]);
  } else
    send_to_char(g, ch, "You have no feeling about the weather at all.\r\n");
}

/* puts -'s instead of spaces */
static void space_to_minus(char *str)
{
  while ((str = strchr(str, ' ')) != nullptr)
    *str = '-';
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
  char buf[MAX_STRING_LENGTH * 4];
  int mid = 0;

  if (!char_get_desc(ch))
    return;

  std::string text(skip_spaces(argument));

  if (g->help_table.empty()) {
    send_to_char(g, ch, "No help available.\r\n");
    return;
  }

  if (text.empty()) {
    if (char_get_admlevel(ch) < ADMLVL_IMMORT)
      send_to_char(g, ch, "%s", g->help.c_str());
    else
      send_to_char(g, ch, "%s", g->ihelp.c_str());
    return;
  }

  space_to_minus(text.data());

  if ((mid = search_help(g, text.c_str(), char_get_admlevel(ch))) == NOWHERE) {
    int i, found = 0;
    send_to_char(g, ch, "There is no help on that word.\r\n");
    if (char_get_admlevel(ch) < 3) {
      mudlog(g, LOG_NRM, MAX(ADMLVL_IMPL, char_is_npc(ch) ? 0 : char_get_invis_level(g, ch)), true, "%s tried to get help on %s", char_get_name(ch), text.c_str());
    }
    for (i = 0; i < (int)g->help_table.size(); i++) {
      if (g->help_table[i].min_level > char_get_admlevel(ch))
        continue;
      /* To help narrow down results, if they don't start with the same letters, move on */
      if (text[0] != *g->help_table[i].keywords.c_str())
        continue;
      if (levenshtein_distance(text.c_str(), g->help_table[i].keywords.c_str()) <= 2) {
        if (!found) {
          send_to_char(g, ch, "\r\nDid you mean:\r\n");
          found = 1;
        }
        send_to_char(g, ch, "  %s\r\n", g->help_table[i].keywords.c_str());
      }
    }
    return;
  }
  if (g->help_table[mid].min_level > char_get_admlevel(ch)) {
    send_to_char(g, ch, "There is no help on that word.\r\n");
    return;
  }
  snprintf(buf, sizeof(buf), "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", g->help_table[mid].entry.c_str());
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  if (char_get_admlevel(ch) > 0) {
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "@WHelp File Level@w: @D(@R%d@D)@n\n", g->help_table[mid].min_level);
  }
  send_to_char(g, ch, "%s", buf);
}

/* Shows one of the fixed text files (credits, news, motd, ...) */
ACMD(do_gen_ps)
{
  std::string arg;
  char bum[10000];
  std::string_view rest = argument;
  arg                   = take_arg(rest);

  switch (subcmd) {
    case SCMD_CREDITS:
      send_to_char(g, ch, "%s", g->credits.c_str());
      break;
    case SCMD_NEWS:
      send_to_char(g, ch, "%s", g->news.c_str());
      char_set_lastpl(ch, game_now(g));
      break;
    case SCMD_INFO:
      send_to_char(g, ch, "%s", g->info.c_str());
      break;
    case SCMD_WIZLIST:
      send_to_char(g, ch, "%s", g->wizlist.c_str());
      break;
    case SCMD_IMMLIST:
      send_to_char(g, ch, "%s", g->immlist.c_str());
      break;
    case SCMD_HANDBOOK:
      send_to_char(g, ch, "%s", g->handbook.c_str());
      break;
    case SCMD_POLICIES:
      snprintf(bum, sizeof(bum), "--------------------\r\n%s\r\n--------------------\r\n", g->policies.c_str());
      send_to_char(g, ch, "%s", bum);
      break;
    case SCMD_MOTD:
      send_to_char(g, ch, "%s", g->motd.c_str());
      break;
    case SCMD_IMOTD:
      send_to_char(g, ch, "%s", g->imotd.c_str());
      break;
    case SCMD_CLEAR:
      send_to_char(g, ch, "\033[H\033[J");
      break;
    case SCMD_VERSION:
      break;
    case SCMD_WHOAMI:
      send_to_char(g, ch, "%s\r\n", char_get_name(ch));
      break;
    default:
      mud_log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
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

static const char *const ctypes[] = {
    "off", "on", "\n"};

ACMD(do_color)
{
  std::string arg;
  int tp;

  /*if (IS_NPC(ch))
    return;*/

  std::string_view rest = argument;
  arg                   = take_word(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Usage: color [ off | on ]\r\n");
    return;
  }
  if ((tp = search_block(arg, ctypes, false)) == -1) {
    send_to_char(g, ch, "Usage: color [ off | on ]\r\n");
    return;
  }
  switch (tp) {
    case C_OFF:
      char_prf_set(g, ch, PRF_COLOR, false);
      break;
    case C_ON:
      char_prf_set(g, ch, PRF_COLOR, true);
      break;
  }
  send_to_char(g, ch, "Your color is now @o%s@n.\r\n", ctypes[tp]);
}

ACMD(do_toggle)
{
  char buf2[4];

  if (char_is_npc(ch))
    return;

  if (char_get_wimp_level(g, ch) == 0)
    strcpy(buf2, "OFF"); /* strcpy: OK */
  else
    snprintf(buf2, sizeof(buf2), "%-3.3d", char_get_wimp_level(g, ch)); /* sprintf: OK */

  if (char_get_admlevel(ch)) {
    send_to_char(g, ch,
                 "      Buildwalk: %-3s    "
                 "Clear Screen in OLC: %-3s\r\n",
                 ONOFF(char_prf_flagged(g, ch, PRF_BUILDWALK)),
                 ONOFF(char_prf_flagged(g, ch, PRF_CLS)));

    send_to_char(g, ch,
                 "      No Hassle: %-3s    "
                 "      Holylight: %-3s    "
                 "     Room Flags: %-3s\r\n",
                 ONOFF(char_prf_flagged(g, ch, PRF_NOHASSLE)),
                 ONOFF(char_prf_flagged(g, ch, PRF_HOLYLIGHT)),
                 ONOFF(char_prf_flagged(g, ch, PRF_ROOMFLAGS)));
  }

  send_to_char(g, ch,
               "Hit Pnt Display: %-3s    "
               "     Brief Mode: %-3s    "
               " Summon Protect: %-3s\r\n"

               "   Move Display: %-3s    "
               "   Compact Mode: %-3s    "
               "       On Quest: %-3s\r\n"

               "    Exp Display: %-3s    "
               "         NoTell: %-3s    "
               "   Repeat Comm.: %-3s\r\n"

               "     Ki Display: %-3s    "
               "           Deaf: %-3s    "
               "     Wimp Level: %-3s\r\n"

               " Gossip Channel: %-3s    "
               "Auction Channel: %-3s    "
               "  Grats Channel: %-3s\r\n"

               "      Auto Loot: %-3s    "
               "      Auto Gold: %-3s    "
               "    Color Level: %s\r\n"

               "     Auto Split: %-3s    "
               "       Auto Sac: %-3s    "
               "       Auto Mem: %-3s\r\n"

               "     View Order: %-3s    "
               "    Auto Assist: %-3s    "
               " Auto Show Exit: %-3s\r\n"

               "    TNL Display: %-3s    ",

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
               buf2,

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

  if (CONFIG_ENABLE_COMPRESSION) {
    send_to_char(g, ch, "    Compression: %-3s\r\n", ONOFF(!char_prf_flagged(g, ch, PRF_NOCOMPRESS)));
  }
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
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;
  std::string arg;
  char label[MAX_INPUT_LENGTH];

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (!arg.empty()) {
    if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD)) || char_is_npc(vict)) {
      send_to_char(g, ch, "Who is that?\r\n");
      return;
    }
    if (char_get_level(ch) < char_get_level(vict)) {
      send_to_char(g, ch, "You can't see the commands of people above your level.\r\n");
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  send_to_char(g, ch, "The following %s%s are available to %s:\r\n",
               wizhelp ? "privileged " : "",
               socials ? "socials" : "commands",
               vict == ch ? "you" : char_get_name(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; g->complete_cmd_info[g->cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) {
    i = g->cmd_sort_info[cmd_num];

    if (g->complete_cmd_info[i].minimum_level < 0 ||
        char_get_level(vict) < g->complete_cmd_info[i].minimum_level)
      continue;

    if (g->complete_cmd_info[i].minimum_admlevel < 0 ||
        char_get_admlevel(vict) < g->complete_cmd_info[i].minimum_admlevel)
      continue;

    if ((g->complete_cmd_info[i].minimum_admlevel >= ADMLVL_IMMORT) != wizhelp)
      continue;

    if (!wizhelp && socials != (g->complete_cmd_info[i].command_pointer == do_action ||
                                g->complete_cmd_info[i].command_pointer == do_insult))
      continue;

    if (check_disabled(g, &g->complete_cmd_info[i]))
      snprintf(label, sizeof(label), "(%s)", g->complete_cmd_info[i].command);
    else
      snprintf(label, sizeof(label), "%s", g->complete_cmd_info[i].command);

    send_to_char(g, ch, "%-11s%s", label, no++ % 7 == 0 ? "\r\n" : "");
  }

  if (no % 7 != 1)
    send_to_char(g, ch, "\r\n");
}

ACMD(do_history)
{
  std::string arg;
  int type;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  type = search_block(arg, history_types, false);
  if (arg.empty() || type < 0) {
    int i;

    send_to_char(g, ch, "Usage: history <");
    for (i = 0; *history_types[i] != '\n'; i++) {
      if ((i != 3 && char_get_admlevel(ch) <= 0) || char_get_admlevel(ch) >= 1) {
        send_to_char(g, ch, " %s ", history_types[i]);
      }
      if (*history_types[i + 1] == '\n') {
        send_to_char(g, ch, ">\r\n");
      } else {
        if ((i != 3 && char_get_admlevel(ch) <= 0) || char_get_admlevel(ch) >= 1) {
          send_to_char(g, ch, "|");
        }
      }
    }
    return;
  }

  std::deque<std::string> *history = char_comm_hist(g, ch, type);
  if (history && !history->empty() && !history->front().empty()) {
    for (const std::string &line : *history)
      send_to_char(g, ch, "%s", line.c_str());
/* Make this a 1 if you want history to cear after viewing */
#if 0 
      free_history(ch, type);
#endif
  } else
    send_to_char(g, ch, "You have no history in that channel.\r\n");
}

void add_history(GameWorld *g, struct char_data *ch, const char *str, int type)
{
  char time_str[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  std::deque<std::string> *history;
  time_t ct;

  if (char_is_npc(ch) || !(history = char_comm_hist(g, ch, type)))
    return;

  ct = game_now(g);
  strftime(time_str, sizeof(time_str), "%H:%M ", localtime(&ct));

  snprintf(buf, sizeof(buf), "%s%s", time_str, str);

  history->push_back(buf);
  while (history->size() > HIST_LENGTH)
    history->pop_front();
  /* add this history message to ALL */
  if (type != HIST_ALL)
    add_history(g, ch, str, HIST_ALL);
}
