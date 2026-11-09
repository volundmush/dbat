/* 
************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <cerrno>
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/act.social.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/social.hpp"
#include "dbat/config.hpp"
#include "dbat/db.hpp"
#include "dbat/comm.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/handler.hpp"
#include "dbat/random.hpp"
#include "dbat/game_world.hpp"

/* local functions */
static char *fread_action(FILE *fl, int nr);
static int find_action(GameWorld *g, int cmd);
static int social_sort_order(const void *a, const void *b);

ACMD(do_action)
{
  std::string arg, part;
  int act_nr;
  struct social_messg *action;
  struct char_data *vict;
  struct obj_data *targ;

  if ((act_nr = find_action(g, cmd)) < 0) {
    send_to_char(g, ch, "That action is not supported.\r\n");
    return;
  }

  action = &g->soc_mess_list[act_nr];

  if (!argument || !*argument) {
    send_to_char(g, ch, "%s\r\n", action->char_no_arg);
    act(g, action->others_no_arg, action->hide, ch, nullptr, nullptr, TO_ROOM);
    return;
  }

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  part                  = take_arg(rest);

  if ((!action->char_body_found) && !part.empty()) {
    send_to_char(g, ch, "Sorry, this social does not support body parts.\r\n");
    return;
  }

  if (!action->char_found) /* the target word only counts for socials that take one */
    arg.clear();

  vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM);
  if (!vict) {
    if (action->char_obj_found) {
      targ = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
      if (!targ) targ = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch)));
      if (targ) {
        act(g, action->char_obj_found, action->hide, ch, targ, nullptr, TO_CHAR);
        act(g, action->others_obj_found, action->hide, ch, targ, nullptr, TO_ROOM);
        return;
      }
    }
    if (action->not_found)
      send_to_char(g, ch, "%s\r\n", action->not_found);
    else
      send_to_char(g, ch, "I don't see anything by that name here.\r\n");
    return;
  }

  if (vict == ch) {
    if (action->char_auto)
      send_to_char(g, ch, "%s\r\n", action->char_auto);
    else
      send_to_char(g, ch, "Erm, no.\r\n");
    act(g, action->others_auto, action->hide, ch, nullptr, nullptr, TO_ROOM);
    return;
  }

  if (char_get_position(vict) < action->min_victim_position)
    act(g, "$N is not in a proper position for that.", false, ch, nullptr, vict, TO_CHAR | TO_SLEEP);
  else {
    if (!part.empty()) {
      act(g, action->char_body_found, 0, ch, static_cast<struct obj_data *>(static_cast<void *>(part.data())) /* act() reads it as $t text */, vict, TO_CHAR | TO_SLEEP);
      act(g, action->others_body_found, action->hide, ch, static_cast<struct obj_data *>(static_cast<void *>(part.data())) /* act() reads it as $t text */, vict, TO_NOTVICT);
      act(g, action->vict_body_found, action->hide, ch, static_cast<struct obj_data *>(static_cast<void *>(part.data())) /* act() reads it as $t text */, vict, TO_VICT);
    } else {
      act(g, action->char_found, 0, ch, nullptr, vict, TO_CHAR | TO_SLEEP);
      act(g, action->others_found, action->hide, ch, nullptr, vict, TO_NOTVICT);
      act(g, action->vict_found, action->hide, ch, nullptr, vict, TO_VICT);
    }
  }
}

ACMD(do_insult)
{
  std::string arg;
  struct char_data *victim;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  arg                   = take_arg(rest);

  if (!arg.empty()) {
    if (!(victim = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM)))
      send_to_char(g, ch, "Can't hear you!\r\n");
    else {
      if (victim != ch) {
        send_to_char(g, ch, "You insult %s.\r\n", char_get_name(victim));

        switch (rand_number(g, 0, 2)) {
          case 0:
            if (char_get_sex(ch) == SEX_MALE) {
              if (char_get_sex(victim) == SEX_MALE)
                act(g, "$n accuses you of fighting like a woman!", false, ch, nullptr, victim, TO_VICT);
              else
                act(g, "$n says that women can't fight.", false, ch, nullptr, victim, TO_VICT);
            } else { /* Ch == Woman */
              if (char_get_sex(victim) == SEX_MALE)
                act(g, "$n accuses you of having the smallest... (brain?)",
                    false, ch, nullptr, victim, TO_VICT);
              else
                act(g, "$n tells you that you'd lose a beauty contest against a troll.",
                    false, ch, nullptr, victim, TO_VICT);
            }
            break;
          case 1:
            act(g, "$n calls your mother a bitch!", false, ch, nullptr, victim, TO_VICT);
            break;
          default:
            act(g, "$n tells you to get lost!", false, ch, nullptr, victim, TO_VICT);
            break;
        } /* end switch */

        act(g, "$n insults $N.", true, ch, nullptr, victim, TO_NOTVICT);
      } else { /* ch == victim */
        send_to_char(g, ch, "You feel insulted.\r\n");
      }
    }
  } else
    send_to_char(g, ch, "I'm sure you don't want to insult *everybody*...\r\n");
}

void boot_social_messages(GameWorld *g)
{
  FILE *fl;
  int nr = 0, hide, min_char_pos, min_pos, min_lvl, curr_soc = -1;
  char next_soc[MAX_STRING_LENGTH], sorted[MAX_INPUT_LENGTH];

  if (CONFIG_NEW_SOCIALS == true) {
    /* open social file */
    if (!(fl = fopen(SOCMESS_FILE_NEW, "r"))) {
      mud_log("SYSERR: can't open socials file '%s': %s", SOCMESS_FILE_NEW, strerror(errno));
      /*  SYSERR_DESC:
       *  This error, from boot_social_messages(), occurs when the server
       *  fails to open the file containing the social messages.  The error
       *  at the end will indicate the reason why.
       */
      exit(1);
    }
    /* count socials */
    *next_soc = '\0';
    while (!feof(fl)) {
      fgets(next_soc, MAX_STRING_LENGTH, fl);
      if (*next_soc == '~') g->top_of_socialt++;
    }
  } else { /* old style */

    /* open social file */
    if (!(fl = fopen(SOCMESS_FILE, "r"))) {
      mud_log("SYSERR: can't open socials file '%s': %s", SOCMESS_FILE, strerror(errno));
      exit(1);
    }
    /* count socials */
    while (!feof(fl)) {
      fgets(next_soc, MAX_STRING_LENGTH, fl);
      if (*next_soc == '\n' || *next_soc == '\r') g->top_of_socialt++; /* all socials are followed by a blank line */
    }
  }

  mud_log("Social table contains %d socials.", g->top_of_socialt);
  if (g->top_of_socialt < 0) { /* a game with no socials can still run */
    fclose(fl);
    return;
  }
  rewind(fl);

  g->soc_mess_list = new social_messg[g->top_of_socialt + 1]();

  /* now read 'em */
  for (;;) {
    fscanf(fl, " %64935s ", next_soc); /* MAX_STRING_LENGTH */
    if (*next_soc == '$') break;
    if (curr_soc >= g->top_of_socialt) {
      mud_log("SYSERR: more socials than were counted in the social file, near '%s'", next_soc);
      exit(1);
    }

    if (CONFIG_NEW_SOCIALS == true) {
      if (fscanf(fl, " %16383s %d %d %d %d \n", /* MAX_INPUT_LENGTH */
                 sorted, &hide, &min_char_pos, &min_pos, &min_lvl) != 5) {
        mud_log("SYSERR: format error in social file near social '%s'", next_soc);
        /*  SYSERR_DESC:
       *  From boot_social_messages(), this error is output when the
       *  server is expecting to find the remainder of the first line of the
       *  social ('hide' and 'minimum position').  These must follow the
       *  name of the social with a single space such as: 'accuse 0 5\n'.
       *  This error often occurs when one of the numbers is missing or the
       *  social name has a space in it (i.e., 'bend over').
       */
        exit(1);
      }
      curr_soc++;
      g->soc_mess_list[curr_soc].command             = strdup(next_soc + 1);
      g->soc_mess_list[curr_soc].sort_as             = strdup(sorted);
      g->soc_mess_list[curr_soc].hide                = hide;
      g->soc_mess_list[curr_soc].min_char_position   = min_char_pos;
      g->soc_mess_list[curr_soc].min_victim_position = min_pos;
      g->soc_mess_list[curr_soc].min_level_char      = min_lvl;
    } else { /* old style */
      if (fscanf(fl, " %d %d \n", &hide, &min_pos) != 2) {
        mud_log("SYSERR: format error in social file near social '%s'", next_soc);
        exit(1);
      }
      curr_soc++;
      g->soc_mess_list[curr_soc].command             = strdup(next_soc);
      g->soc_mess_list[curr_soc].sort_as             = strdup(next_soc);
      g->soc_mess_list[curr_soc].hide                = hide;
      g->soc_mess_list[curr_soc].min_char_position   = POS_RESTING;
      g->soc_mess_list[curr_soc].min_victim_position = min_pos;
      g->soc_mess_list[curr_soc].min_level_char      = 0;
    }

    g->soc_mess_list[curr_soc].char_no_arg   = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].others_no_arg = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].char_found    = fread_action(fl, nr);

    /* if no char_found, the rest is to be ignored */
    if (CONFIG_NEW_SOCIALS == false && !g->soc_mess_list[curr_soc].char_found)
      continue;

    g->soc_mess_list[curr_soc].others_found = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].vict_found   = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].not_found    = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].char_auto    = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].others_auto  = fread_action(fl, nr);

    if (CONFIG_NEW_SOCIALS == false)
      continue;

    g->soc_mess_list[curr_soc].char_body_found   = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].others_body_found = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].vict_body_found   = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].char_obj_found    = fread_action(fl, nr);
    g->soc_mess_list[curr_soc].others_obj_found  = fread_action(fl, nr);
  }

  /* close file & set top */
  fclose(fl);
  g->top_of_socialt = curr_soc;
}

void create_command_list(GameWorld *g)
{
  int i, j, k;

  /* free up old command list */
  if (g->complete_cmd_info)
    free_command_list(g);

  /* re check the sort on the socials */
  if (g->soc_mess_list)
    qsort(g->soc_mess_list, g->top_of_socialt + 1, sizeof(*g->soc_mess_list), social_sort_order);

  /* count the commands in the command list */
  i = 0;
  while (*cmd_info[i].command != '\n') i++;
  i++;

  g->complete_cmd_info = new command_info[g->top_of_socialt + i + 2]();

  /* this loop sorts the socials and commands together into one big list */
  i = 0;
  j = 0;
  k = 0;

  while (*cmd_info[i].command != '\n') {
    g->complete_cmd_info[k++] = cmd_info[i++];
  }

  while (j <= g->top_of_socialt) {
    g->soc_mess_list[j].act_nr               = k;
    g->complete_cmd_info[k].command          = g->soc_mess_list[j].command;
    g->complete_cmd_info[k].sort_as          = g->soc_mess_list[j].sort_as;
    g->complete_cmd_info[k].minimum_position = g->soc_mess_list[j].min_char_position;
    g->complete_cmd_info[k].command_pointer  = do_action;
    g->complete_cmd_info[k].minimum_level    = g->soc_mess_list[j++].min_level_char;
    g->complete_cmd_info[k].minimum_admlevel = ADMLVL_NONE;
    g->complete_cmd_info[k++].subcmd         = 0;
  }

  g->complete_cmd_info[k].command          = strdup("\n");
  g->complete_cmd_info[k].sort_as          = strdup("zzzzzzz");
  g->complete_cmd_info[k].minimum_position = 0;
  g->complete_cmd_info[k].command_pointer  = nullptr;
  g->complete_cmd_info[k].minimum_level    = 0;
  g->complete_cmd_info[k].minimum_admlevel = 0;
  g->complete_cmd_info[k].subcmd           = 0;
  mud_log("Command info rebuilt, %d total commands.", k);
}

void free_command_list(GameWorld *g)
{
  int k;

  if (!g->complete_cmd_info)
    return;
  /* entries point at cmd_info's and the socials' strings, except the terminator made by create_command_list() */
  /* by the function pointer: aedit may just have freed the strings of the socials */
  for (k = 1; g->complete_cmd_info[k].command_pointer; k++);
  free(const_cast<char *>(g->complete_cmd_info[k].command));
  free(const_cast<char *>(g->complete_cmd_info[k].sort_as));
  delete[] g->complete_cmd_info;
  g->complete_cmd_info = nullptr;
}

static char *fread_action(FILE *fl, int nr)
{
  char buf[MAX_STRING_LENGTH];

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl)) {
    mud_log("SYSERR: fread_action: unexpected EOF near action #%d", nr);
    /*  SYSERR_DESC:
     *  fread_action() will fail if it discovers an end of file marker
     *  before it is able to read in the expected string.  This can be
     *  caused by a truncated socials file.
     */
    exit(1);
  }
  if (*buf == '#')
    return (nullptr);

  buf[strlen(buf) - 1] = '\0';
  return (strdup(buf));
}

void free_social_messages(GameWorld *g)
{
  struct social_messg *mess;
  int i;

  for (i = 0; i <= g->top_of_socialt; i++) {
    mess = &g->soc_mess_list[i];
    free_action(mess);
  }
  delete[] g->soc_mess_list;
}

void free_action(struct social_messg *mess)
{
  if (mess->command) free(mess->command);
  if (mess->sort_as) free(mess->sort_as);
  if (mess->char_no_arg) free(mess->char_no_arg);
  if (mess->others_no_arg) free(mess->others_no_arg);
  if (mess->char_found) free(mess->char_found);
  if (mess->others_found) free(mess->others_found);
  if (mess->vict_found) free(mess->vict_found);
  if (mess->char_body_found) free(mess->char_body_found);
  if (mess->others_body_found) free(mess->others_body_found);
  if (mess->vict_body_found) free(mess->vict_body_found);
  if (mess->not_found) free(mess->not_found);
  if (mess->char_auto) free(mess->char_auto);
  if (mess->others_auto) free(mess->others_auto);
  if (mess->char_obj_found) free(mess->char_obj_found);
  if (mess->others_obj_found) free(mess->others_obj_found);
  *mess = {};
}

static int social_sort_order(const void *a, const void *b)
{
  return strcasecmp(((const struct social_messg *)a)->sort_as, ((const struct social_messg *)b)->sort_as);
}

static int social_order(const void *key, const void *social)
{
  int k = *(const int *)key, v = ((const struct social_messg *)social)->act_nr;

  return (k > v) - (k < v);
}

/* soc_mess_list is in act_nr order: create_command_list() numbers it that way. */
static int find_action(GameWorld *g, int cmd)
{
  struct social_messg *found;

  if (g->top_of_socialt < 0)
    return -1;
  found = (struct social_messg *)bsearch(&cmd, g->soc_mess_list, g->top_of_socialt + 1, sizeof(*g->soc_mess_list), social_order);
  return found ? found - g->soc_mess_list : -1;
}

ACMD(do_gmote)
{
  int act_nr, length;
  std::string arg, buf;
  char out[MAX_INPUT_LENGTH];
  struct social_messg *action;
  struct char_data *vict = nullptr;

  std::string_view rest = argument ? argument : ""; /* (some callers pass null) */
  buf                   = take_word(rest);
  arg                   = skip_spaces(rest);

  if (subcmd)
    for (length = buf.size(), cmd = 0; *g->complete_cmd_info[cmd].command != '\n'; cmd++)
      if (!strncmp(g->complete_cmd_info[cmd].command, buf.c_str(), length))
        break;

  if ((act_nr = find_action(g, cmd)) < 0) {
    snprintf(out, sizeof(out), "@D[@BOOC@D: @g%s %s@n@D]", char_get_admlevel(ch) < 1 ? char_get_user(ch) : char_get_name(ch), argument);
    act(g, out, false, ch, nullptr, vict, TO_GMOTE);
    return;
  }

  if (room_flagged(g, char_get_in_room(ch), ROOM_SOUNDPROOF)) {
    send_to_char(g, ch, "The walls seem to absorb your actions.\r\n");
    return;
  }

  action = &g->soc_mess_list[act_nr];

  if (!action->char_found)
    arg.clear();

  if (arg.empty()) {
    if (!action->others_no_arg || !*action->others_no_arg) {
      send_to_char(g, ch, "Who are you going to do that to?\r\n");
      return;
    }
    snprintf(out, sizeof(out), "@D[@BOOC@D: @g%s@D]@n", action->others_no_arg);
  } else if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "%s\r\n", action->not_found);
    return;
  } else if (vict == ch) {
    if (!action->others_auto || !*action->others_auto) {
      send_to_char(g, ch, "%s\r\n", action->char_auto);
      return;
    }
    snprintf(out, sizeof(out), "@D[@BOOC@D: @g%s@D]@n", action->others_auto);
  } else {
    if (char_get_position(vict) < action->min_victim_position) {
      act(g, "$N is not in a proper position for that.",
          false, ch, nullptr, vict, TO_CHAR | TO_SLEEP);
      return;
    }
    snprintf(out, sizeof(out), "@D[@BOOC@D: @g%s@D]@n", action->others_found);
  }
  act(g, out, false, ch, nullptr, vict, TO_GMOTE);
}
