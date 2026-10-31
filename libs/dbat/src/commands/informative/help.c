/* help.c: help, commands, toggles, colour, history, time and weather
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/consts/maximums.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"
#include "dbat/config.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/time.h"
#include "dbat/help.h"
#include "dbat/command.h"
#include "dbat/flags.h"
#include "dbat/affect.h"
#include "dbat/stringutils.h"
#include "dbat/random.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/room_utils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/fileop.h"
#include "dbat/races.h"
#include "dbat/interpreter.h"
#include "dbat/dg_scripts.h"
#include "dbat/extract.h"
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/vehicles.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/maputils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/boards.h"
#include "dbat/screen.h"
#include "dbat/mail.h"
#include "dbat/guild.h"
#include "dbat/disabled.h"
#include "informative_internal.h"

static int sort_commands_helper(const void *a, const void *b);
static void space_to_minus(char *str);

/* local globals */
int *cmd_sort_info;

ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  /* day in [1..30] */
  day = time_info.day + 1;

  /* 30 days in a month, 6 days a week */
  weekday = day % 6;

  send_to_char(ch, "It is %d o'clock %s, on %s.\r\n",
               (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
               time_info.hours >= 12 ? "PM" : "AM", weekdays[weekday]);

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

  send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
               day, suf, month_name[time_info.month], time_info.year);
}

ACMD(do_weather)
{
  const char *sky_look[] = {
      "cloudless",
      "cloudy",
      "rainy",
      "lit by flashes of lightning"};

  if (char_is_outside(ch)) {
    send_to_char(ch, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
                 weather_info.change >= 0 ? "you feel a warm wind from south" : "your foot tells you bad weather is due");
    if (char_adm_flagged(ch, ADM_KNOWWEATHER))
      send_to_char(ch, "Pressure: %d (change: %d), Sky: %d (%s)\r\n",
                   weather_info.pressure,
                   weather_info.change,
                   weather_info.sky,
                   sky_look[weather_info.sky]);
  } else
    send_to_char(ch, "You have no feeling about the weather at all.\r\n");
}

/* puts -'s instead of spaces */
static void space_to_minus(char *str)
{
  while ((str = strchr(str, ' ')) != NULL)
    *str = '-';
}

int search_help(const char *argument, int level)
{
  int chk, bot, top, mid, minlen;

  bot    = 0;
  top    = top_of_helpt;
  minlen = strlen(argument);

  while (bot <= top) {
    mid = (bot + top) / 2;

    if (!(chk = strncasecmp(argument, help_table[mid].keywords, minlen))) {
      while ((mid > 0) && !strncasecmp(argument, help_table[mid - 1].keywords, minlen))
        mid--;

      while (level < help_table[mid].min_level && mid < (bot + top) / 2)
        mid++;

      if (strncasecmp(argument, help_table[mid].keywords, minlen))
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

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!help_table) {
    send_to_char(ch, "No help available.\r\n");
    return;
  }

  if (!*argument) {
    if (char_get_admlevel(ch) < ADMLVL_IMMORT)
      page_string(ch->desc, help, 0);
    else
      page_string(ch->desc, ihelp, 0);
    return;
  }

  space_to_minus(argument);

  if ((mid = search_help(argument, char_get_admlevel(ch))) == NOWHERE) {
    int i, found = 0;
    send_to_char(ch, "There is no help on that word.\r\n");
    if (char_get_admlevel(ch) < 3) {
      mudlog(LOG_NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "%s tried to get help on %s", char_get_name(ch), argument);
    }
    for (i = 0; i <= top_of_helpt; i++) {
      if (help_table[i].min_level > char_get_admlevel(ch))
        continue;
      /* To help narrow down results, if they don't start with the same letters, move on */
      if (*argument != *help_table[i].keywords)
        continue;
      if (levenshtein_distance(argument, help_table[i].keywords) <= 2) {
        if (!found) {
          send_to_char(ch, "\r\nDid you mean:\r\n");
          found = 1;
        }
        send_to_char(ch, "  %s\r\n", help_table[i].keywords);
      }
    }
    return;
  }
  if (help_table[mid].min_level > char_get_admlevel(ch)) {
    send_to_char(ch, "There is no help on that word.\r\n");
    return;
  }
  sprintf(buf, "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  sprintf(buf + strlen(buf), "%s", help_table[mid].entry);
  sprintf(buf + strlen(buf), "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  if (char_get_admlevel(ch) > 0) {
    sprintf(buf + strlen(buf), "@WHelp File Level@w: @D(@R%d@D)@n\n", help_table[mid].min_level);
  }
  page_string(ch->desc, buf, 0);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  char arg[MAX_INPUT_LENGTH];
  char bum[10000];
  one_argument(argument, arg);

  switch (subcmd) {
    case SCMD_CREDITS:
      page_string(ch->desc, credits, 0);
      break;
    case SCMD_NEWS:
      page_string(ch->desc, news, 0);
      char_set_lastpl(ch, time(0));
      break;
    case SCMD_INFO:
      page_string(ch->desc, info, 0);
      break;
    case SCMD_WIZLIST:
      page_string(ch->desc, wizlist, 0);
      break;
    case SCMD_IMMLIST:
      page_string(ch->desc, immlist, 0);
      break;
    case SCMD_HANDBOOK:
      page_string(ch->desc, handbook, 0);
      break;
    case SCMD_POLICIES:
      sprintf(bum, "--------------------\r\n%s\r\n--------------------\r\n", policies);
      page_string(ch->desc, bum, 0);
      break;
    case SCMD_MOTD:
      page_string(ch->desc, motd, 0);
      break;
    case SCMD_IMOTD:
      page_string(ch->desc, imotd, 0);
      break;
    case SCMD_CLEAR:
      send_to_char(ch, "\033[H\033[J");
      break;
    case SCMD_VERSION:
      break;
    case SCMD_WHOAMI:
      send_to_char(ch, "%s\r\n", char_get_name(ch));
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

static const char *ctypes[] = {
    "off", "on", "\n"};

ACMD(do_color)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  /*if (IS_NPC(ch))
    return;*/

  any_one_arg(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usage: color [ off | on ]\r\n");
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char(ch, "Usage: color [ off | on ]\r\n");
    return;
  }
  switch (tp) {
    case C_OFF:
      char_prf_set(ch, PRF_COLOR, false);
      break;
    case C_ON:
      char_prf_set(ch, PRF_COLOR, true);
      break;
  }
  send_to_char(ch, "Your color is now @o%s@n.\r\n", ctypes[tp]);
}

ACMD(do_toggle)
{
  char buf2[4];

  if (char_is_npc(ch))
    return;

  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF"); /* strcpy: OK */
  else
    sprintf(buf2, "%-3.3d", GET_WIMP_LEV(ch)); /* sprintf: OK */

  if (char_get_admlevel(ch)) {
    send_to_char(ch,
                 "      Buildwalk: %-3s    "
                 "Clear Screen in OLC: %-3s\r\n",
                 ONOFF(char_prf_flagged(ch, PRF_BUILDWALK)),
                 ONOFF(char_prf_flagged(ch, PRF_CLS)));

    send_to_char(ch,
                 "      No Hassle: %-3s    "
                 "      Holylight: %-3s    "
                 "     Room Flags: %-3s\r\n",
                 ONOFF(char_prf_flagged(ch, PRF_NOHASSLE)),
                 ONOFF(char_prf_flagged(ch, PRF_HOLYLIGHT)),
                 ONOFF(char_prf_flagged(ch, PRF_ROOMFLAGS)));
  }

  send_to_char(ch,
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

               ONOFF(char_prf_flagged(ch, PRF_DISPHP)),
               ONOFF(char_prf_flagged(ch, PRF_BRIEF)),
               ONOFF(!char_prf_flagged(ch, PRF_SUMMONABLE)),

               ONOFF(char_prf_flagged(ch, PRF_DISPMOVE)),
               ONOFF(char_prf_flagged(ch, PRF_COMPACT)),
               YESNO(char_prf_flagged(ch, PRF_QUEST)),

               ONOFF(char_prf_flagged(ch, PRF_DISPEXP)),
               ONOFF(char_prf_flagged(ch, PRF_NOTELL)),
               YESNO(!char_prf_flagged(ch, PRF_NOREPEAT)),

               ONOFF(char_prf_flagged(ch, PRF_DISPKI)),
               YESNO(char_prf_flagged(ch, PRF_DEAF)),
               buf2,

               ONOFF(!char_prf_flagged(ch, PRF_NOGOSS)),
               ONOFF(!char_prf_flagged(ch, PRF_NOAUCT)),
               ONOFF(!char_prf_flagged(ch, PRF_NOGRATZ)),

               ONOFF(char_prf_flagged(ch, PRF_AUTOLOOT)),
               ONOFF(char_prf_flagged(ch, PRF_AUTOGOLD)),
               ctypes[COLOR_LEV(ch)],

               ONOFF(char_prf_flagged(ch, PRF_AUTOSPLIT)),
               ONOFF(char_prf_flagged(ch, PRF_AUTOSAC)),
               ONOFF(char_prf_flagged(ch, PRF_AUTOMEM)),

               ONOFF(char_prf_flagged(ch, PRF_VIEWORDER)),
               ONOFF(char_prf_flagged(ch, PRF_AUTOASSIST)),
               ONOFF(char_prf_flagged(ch, PRF_AUTOEXIT)),

               ONOFF(char_prf_flagged(ch, PRF_DISPTNL)));

  if (CONFIG_ENABLE_COMPRESSION) {
    send_to_char(ch, "    Compression: %-3s\r\n", ONOFF(!char_prf_flagged(ch, PRF_NOCOMPRESS)));
  }
}

static int sort_commands_helper(const void *a, const void *b)
{
  return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                complete_cmd_info[*(const int *)b].sort_as);
}

void sort_commands(void)
{
  int a, num_of_cmds = 0;

  while (complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++; /* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}

ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) || char_is_npc(vict)) {
      send_to_char(ch, "Who is that?\r\n");
      return;
    }
    if (char_get_level(ch) < char_get_level(vict)) {
      send_to_char(ch, "You can't see the commands of people above your level.\r\n");
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  send_to_char(ch, "The following %s%s are available to %s:\r\n",
               wizhelp ? "privileged " : "",
               socials ? "socials" : "commands",
               vict == ch ? "you" : char_get_name(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) {
    i = cmd_sort_info[cmd_num];

    if (complete_cmd_info[i].minimum_level < 0 ||
        char_get_level(vict) < complete_cmd_info[i].minimum_level)
      continue;

    if (complete_cmd_info[i].minimum_admlevel < 0 ||
        char_get_admlevel(vict) < complete_cmd_info[i].minimum_admlevel)
      continue;

    if ((complete_cmd_info[i].minimum_admlevel >= ADMLVL_IMMORT) != wizhelp)
      continue;

    if (!wizhelp && socials != (complete_cmd_info[i].command_pointer == do_action ||
                                complete_cmd_info[i].command_pointer == do_insult))
      continue;

    if (check_disabled(&complete_cmd_info[i]))
      sprintf(arg, "(%s)", complete_cmd_info[i].command);
    else
      sprintf(arg, "%s", complete_cmd_info[i].command);

    send_to_char(ch, "%-11s%s", arg, no++ % 7 == 0 ? "\r\n" : "");
  }

  if (no % 7 != 1)
    send_to_char(ch, "\r\n");
}

ACMD(do_history)
{
  char arg[MAX_INPUT_LENGTH];
  int type;

  one_argument(argument, arg);

  type = search_block(arg, history_types, FALSE);
  if (!*arg || type < 0) {
    int i;

    send_to_char(ch, "Usage: history <");
    for (i = 0; *history_types[i] != '\n'; i++) {
      if ((i != 3 && char_get_admlevel(ch) <= 0) || char_get_admlevel(ch) >= 1) {
        send_to_char(ch, " %s ", history_types[i]);
      }
      if (*history_types[i + 1] == '\n') {
        send_to_char(ch, ">\r\n");
      } else {
        if ((i != 3 && char_get_admlevel(ch) <= 0) || char_get_admlevel(ch) >= 1) {
          send_to_char(ch, "|");
        }
      }
    }
    return;
  }

  if (GET_HISTORY(ch, type) && GET_HISTORY(ch, type)->text && *GET_HISTORY(ch, type)->text) {
    struct txt_block *tmp;
    for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next)
      send_to_char(ch, "%s", tmp->text);
/* Make this a 1 if you want history to cear after viewing */
#if 0 
      free_history(ch, type);
#endif
  } else
    send_to_char(ch, "You have no history in that channel.\r\n");
}

void add_history(struct char_data *ch, char *str, int type)
{
  int i = 0;
  char time_str[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  struct txt_block *tmp;
  time_t ct;

  if (char_is_npc(ch))
    return;

  tmp = GET_HISTORY(ch, type);
  ct  = time(0);
  strftime(time_str, sizeof(time_str), "%H:%M ", localtime(&ct));

  sprintf(buf, "%s%s", time_str, str);

  if (!tmp) {
    CREATE(GET_HISTORY(ch, type), struct txt_block, 1);
    GET_HISTORY(ch, type)->text = strdup(buf);
  } else {
    while (tmp->next)
      tmp = tmp->next;
    CREATE(tmp->next, struct txt_block, 1);
    tmp->next->text = strdup(buf);

    for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next, i++);

    for (; i > HIST_LENGTH && GET_HISTORY(ch, type); i--) {
      tmp                   = GET_HISTORY(ch, type);
      GET_HISTORY(ch, type) = tmp->next;
      if (tmp->text)
        free(tmp->text);
      free(tmp);
    }
  }
  /* add this history message to ALL */
  if (type != HIST_ALL)
    add_history(ch, str, HIST_ALL);
}
