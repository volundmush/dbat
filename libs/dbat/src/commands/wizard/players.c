/* ************************************************************************
*   File: wizard/players.c                                  Part of CircleMUD *
*  Usage: God commands that change players                                *
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

/* local functions */
static void print_lockout(struct char_data *ch);
static int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
static void perform_immort_invis(struct char_data *ch, int level);
static void lockWrite(struct char_data *ch, char *name);

static void print_lockout(struct char_data *ch)
{
  if (char_is_npc(ch))
    return;

  FILE *file;
  char fname[40], filler[50], line[256], buf[MAX_STRING_LENGTH * 4];
  int count = 0, first = TRUE;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
    send_to_char(ch, "The lockout file does not exist.");
    return;
  }
  if (!(file = fopen(fname, "r"))) {
    send_to_char(ch, "The lockout file does not exist.");
    return;
  }
  snprintf(buf, sizeof(buf), "@b------------------[ @RLOCKOUT @b]------------------@n\n");
  while (!feof(file)) {
    get_line(file, line);
    sscanf(line, "%s\n", filler);
    if (first != TRUE && strstr(buf, filler) == NULL) {
      if (count == 0) {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%-23s@D|@n", filler);
        count = 1;
      } else {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%-23s\n", filler);
        count = 0;
      }
    } else if (first == TRUE) {
      snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%-23s@D|@n", filler);
      first = FALSE;
      count = 1;
    }
    *filler = '\0';
  }
  if (count == 1) {
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "\n");
  }
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "@b------------------[ @RLOCKOUT @b]------------------@n\n");

  send_to_char(ch, "%s", buf);

  fclose(file);
}

ACMD(do_approve)
{

  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict = NULL;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What player do you want to approve as having an acceptable bio?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "That player is not in the game.\r\n");
    return;
  }

  if (char_plr_flagged(vict, PLR_BIOGR)) {
    send_to_char(ch, "They have already been approved. If this was made in error inform Iovan.\r\n");
    return;
  }
  char_plr_set(vict, PLR_BIOGR, true);
  send_to_char(ch, "They have now been approved.\r\n");
  return;
}

static void lockWrite(struct char_data *ch, char *name)
{
  struct atomic_file af;
  FILE *file;
  char fname[40], filler[50], line[256];
  char *names[500] = {NULL};
  FILE *fl;
  int count = 0, x = 0, found = FALSE;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
    send_to_char(ch, "The lockout file does not exist.");
    return;
  }
  if (!(file = fopen(fname, "r"))) {
    send_to_char(ch, "The lockout file does not exist.");
    return;
  }
  while (!feof(file) || count < 498) {
    get_line(file, line);
    sscanf(line, "%s\n", filler);
    names[count] = strdup(filler);
    count++;
    *filler = '\0';
  }
  fclose(file);

  /* Write Introduction File */

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout"))
    return;

  if (!(fl = atomic_open(&af, fname, false))) {
    mud_log("ERROR: could not save Lockout File, %s.", fname);
    return;
  }

  while (x < count) {
    if (x == 0 || strcasecmp(names[x - 1], names[x])) {
      if (strcasecmp(names[x], CAP(name))) {
        fprintf(fl, "%s\n", CAP(names[x]));
      } else {
        found = TRUE;
      }
    }
    x++;
  }
  if (found == FALSE) {
    fprintf(fl, "%s\n", CAP(name));
    send_to_all("@rLOCKOUT@D: @WThe character, @C%s@W, was locked out of the MUD by @c%s@W.@n\r\n", CAP(name), char_get_name(ch));
    mud_log("LOCKOUT: %s sentenced by %s.", CAP(name), char_get_name(ch));
    log_imm_action("LOCKOUT: %s sentenced by %s.", CAP(name), char_get_name(ch));
  } else {
    send_to_all("@rLOCKOUT@D: @WThe character, @C%s@W, has had lockout removed by @c%s@W.@n\r\n", CAP(name), char_get_name(ch));
    mud_log("LOCKOUT: %s sentenced by %s.", CAP(name), char_get_name(ch));
    log_imm_action("LOCKOUT: %s sentenced by %s.", CAP(name), char_get_name(ch));
  }

  atomic_commit(&af);
  return;
}

ACMD(do_reward)
{
  struct char_data *vict = NULL;
  struct descriptor_data *k;
  int amt = 0;

  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: reward (target) (amount)\r\nThis is either a positive number or a negative.\r\n");
    return;
  }

  if (!readUserIndex(arg)) {
    send_to_char(ch, "That is not a recognised user file.\r\n");
    return;
  }

  amt = atoi(arg2);

  if (amt == 0) {
    send_to_char(ch, "That is pointless don't you think? Try an amount higher than 0.\r\n");
    return;
  }

  for (k = descriptor_list; k; k = k->next) {
    if (char_is_npc(k->character))
      continue;
    if (desc_get_connected(k) != CON_PLAYING)
      continue;
    if (!strcasecmp(k->user, arg))
      vict = k->character;
  }

  if (amt > 0) {
    if (vict != NULL) {
      send_to_char(ch, "@WYou award @C%s @D(@G%d@D)@W RP points.@n\r\n", char_get_name(vict), amt);
      send_to_char(vict, "@D[@YROLEPLAY@D] @WYou have been awarded @D(@G%d@D)@W RP points by @C%s@W.@n\r\n", amt, char_get_name(ch));
      send_to_imm("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      log_imm_action("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      vict->rp += amt;
      char_set_rp(vict, vict->rp);
      if (amt <= 29) {
        char_set_trp(vict, char_get_trp(vict) + (amt));
      }
      vict->desc->rpp += amt;
      userWrite(vict->desc, 0, 0, 0, "index");
    } else {
      send_to_char(ch, "@WYou award user @C%s @D(@G%d@D)@W RP points.@n\r\n", arg, amt);
      send_to_imm("ROLEPLAY: User %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      log_imm_action("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      userWrite(NULL, 0, amt, 0, arg);
    }
  } else {
    if (vict != NULL) {
      send_to_char(ch, "@WYou deduct @D(@G%d@D)@W RP points from @C%s@W.@n\r\n", amt, char_get_name(vict));
      send_to_char(vict, "@D[@YROLEPLAY@D] @C%s@W deducts @D(@G%d@D)@W RP points from you.@n\r\n", char_get_name(ch), amt);
      send_to_imm("ROLEPLAY: %s has had %d RP points deducted by %s.", char_get_name(vict), amt, char_get_name(ch));
      log_imm_action("ROLEPLAY: %s has had %d RP points deducted by %s.", char_get_name(vict), amt, char_get_name(ch));
      vict->rp += amt;
      char_set_rp(vict, vict->rp);
      vict->desc->rpp += amt;
      userWrite(vict->desc, 0, 0, 0, "index");
    } else {
      send_to_char(ch, "@WYou deduct @D(@G%d@D)@W RP points from user @C%s@W.@n\r\n", amt, arg);
      send_to_imm("ROLEPLAY: User %s has had %d RP points deducted by %s.", arg, amt, char_get_name(ch));
      log_imm_action("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      userWrite(NULL, 0, amt, 0, arg);
    }
  }
}

ACMD(do_rbank)
{
  struct char_data *vict = NULL;
  struct descriptor_data *k;
  int amt = 0;

  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: rbank (target) (amount)\r\nThis is either a positive number or a negative.\r\n");
    return;
  }

  if (!readUserIndex(arg)) {
    send_to_char(ch, "That is not a recognised user file.\r\n");
    return;
  }

  amt = atoi(arg2);

  if (amt == 0) {
    send_to_char(ch, "That is pointless don't you think? Try an amount higher than 0.\r\n");
    return;
  }

  for (k = descriptor_list; k; k = k->next) {
    if (char_is_npc(k->character))
      continue;
    if (desc_get_connected(k) != CON_PLAYING)
      continue;
    if (!strcasecmp(k->user, arg))
      vict = k->character;
  }

  if (amt > 0) {
    if (vict != NULL) {
      send_to_char(ch, "@WYou put @D(@G%d@D)@W RP points into @C%s@W's Bank.@n\r\n", amt, char_get_name(vict));
      send_to_char(vict, "@D[@YROLEPLAY@D] @WYou have had @D(@G%d@D)@W RP points added to your RPP Bank by @C%s@W.@n\r\n", amt, char_get_name(ch));
      send_to_imm("ROLEPLAY: %s has had %d RP points put into their RPP Bank by %s.", arg, amt, char_get_name(ch));
      log_imm_action("ROLEPLAY: %s has had %d RP points put into their RPP Bank by %s.", arg, amt, char_get_name(ch));
      vict->rbank += amt;
      char_set_rbank(vict, vict->rbank);
      if (amt <= 29) {
        char_set_trp(vict, char_get_trp(vict) + (amt));
      }
      vict->desc->rbank += amt;
      userWrite(vict->desc, 0, 0, 0, "index");
    } else {
      send_to_char(ch, "@WYou put @D(@G%d@D)@W RP points into @C%s@W's Bank.@n\r\n", amt, arg);
      send_to_imm("ROLEPLAY: %s has had %d RP points put into their RPP Bank by %s.", arg, amt, char_get_name(ch));
      log_imm_action("ROLEPLAY: %s has had %d RP points put into their RPP Bank by %s.", arg, amt, char_get_name(ch));
      userWrite(NULL, 0, 0, amt, arg);
    }
  } else {
    if (vict != NULL) {
      send_to_char(ch, "@WYou deduct @D(@G%d@D)@W RP points from @C%s@W's Bank.@n\r\n", amt, char_get_name(vict));
      send_to_char(vict, "@D[@YROLEPLAY@D] @C%s@W deducts @D(@G%d@D)@W RP points from your RPP Bank.@n\r\n", char_get_name(ch), amt);
      send_to_imm("ROLEPLAY: %s has had %d RP points deducted from their RPP Bank by %s.", char_get_name(vict), amt, char_get_name(ch));
      log_imm_action("ROLEPLAY: %s has had %d RP points deducted from their RPP Bank by %s.", char_get_name(vict), amt, char_get_name(ch));
      vict->rbank += amt;
      char_set_rbank(vict, vict->rbank);
      vict->desc->rbank += amt;
      userWrite(vict->desc, 0, 0, 0, "index");
    } else {
      send_to_char(ch, "@WYou deduct @D(@G%d@D)@W RP points from @C%s@W's Bank.@n\r\n", amt, arg);
      send_to_imm("ROLEPLAY: %s has had %d RP points deducted from their RPP Bank by %s.", arg, amt, char_get_name(ch));
      log_imm_action("ROLEPLAY: %s has had %d RP points put deducted from RPP Bank by %s.", arg, amt, char_get_name(ch));
      userWrite(NULL, 0, 0, amt, arg);
    }
  }
}

ACMD(do_permission)
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "You want to @Grestrict@n or @Gunrestrict@n?\r\n");
    return;
  }

  if (!*arg2 && !strcasecmp("unrestrict", arg)) {
    send_to_char(ch, "You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
    return;
  }
  if (!strcasecmp("unrestrict", arg)) {
    if (!strcasecmp("saiyan", arg2)) {
      send_to_char(ch, "You have unrestricted saiyans for the very next character creation.\r\n");
      send_to_imm("PERMISSION: %s unrestricted saiyans.", char_get_name(ch));
      SAIYAN_ALLOWED = TRUE;
    } else if (!strcasecmp("majin", arg2)) {
      send_to_char(ch, "You have unrestricted majins for the very next character creation.\r\n");
      send_to_imm("PERMISSION: %s unrestricted majins.", char_get_name(ch));
      MAJIN_ALLOWED = TRUE;
    } else {
      send_to_char(ch, "You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
      return;
    }
  } else if (!strcasecmp("restrict", arg)) {
    send_to_char(ch, "You have restricted character creation to standard race slection.\r\n");
    send_to_imm("PERMISSION: %s restricted races again.", char_get_name(ch));
    MAJIN_ALLOWED = FALSE;
  } else {
    send_to_char(ch, "You want to @Grestrict@n or @Gunrestrict@n?\r\n");
    return;
  }
}

ACMD(do_interest)
{
  if (char_get_admlevel(ch) < 5) {
    send_to_char(ch, "Huh!?\r\n");
    return;
  }
  if (INTERESTTIME > 0) {
    char *tmstr;
    tmstr                        = (char *)asctime(localtime(&INTERESTTIME));
    *(tmstr + strlen(tmstr) - 1) = '\0';
    send_to_char(ch, "INTEREST TIME: [%s]\r\n", tmstr);
    return;
  }
  send_to_char(ch, "Interest time has been initiated!\r\n");
  INTERESTTIME = time(0) + 86400;
  LASTINTEREST = time(0) + 86400;
  return;
}

ACMD(do_hell)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: lockout (character)\n"
                     "        lockout list\r\n");
    return;
  }

  if (!strcasecmp(arg, "Iovan") || !strcasecmp(arg, "iovan") || !strcasecmp(arg, "Fahl") || !strcasecmp(arg, "fahl") || !strcasecmp(arg, "Xyron") || !strcasecmp(arg, "xyron") || !strcasecmp(arg, "Samael") || !strcasecmp(arg, "samael")) {
    send_to_char(ch, "What are you smoking? You can't lockout senior imms.\r\n");
    return;
  }

  if (!strcasecmp(arg, "list")) {
    print_lockout(ch);
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    lockWrite(ch, arg);
    return;
  }
  struct descriptor_data *d = vict->desc;
  Crash_rentsave(vict, 0);
  extract_char(vict);
  lockWrite(ch, char_get_name(vict));
  if (d && desc_get_connected(d) != CON_PLAYING) {
    desc_set_connected(d, CON_CLOSE);
    vict->desc->character = NULL;
    vict->desc            = NULL;
  }
  return;
  return;
}

ACMD(do_advance)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
  int newlevel, oldlevel;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, "That player is not here.\r\n");
      return;
    }
  } else {
    send_to_char(ch, "Advance who?\r\n");
    return;
  }

  if (char_is_npc(victim)) {
    send_to_char(ch, "NO!  Not on NPC's.\r\n");
    return;
  }
  if (!*level) {
    send_to_char(ch, "[ 1 - 100 | demote ]\r\n");
    return;
  }
  if ((newlevel = atoi(level)) <= 0) {
    if (!strcasecmp("demote", level)) {
      victim->level  = 1;
      victim->basepl = 150;
      victim->baseki = 150;
      victim->basest = 150;
      send_to_char(ch, "They have now been demoted!\r\n");
      send_to_char(victim, "You were demoted to level 1!\r\n");
      return;
    }
    send_to_char(ch, "That's not a level!\r\n");
    return;
  }
  if (newlevel > 100) {
    send_to_char(ch, "100 is the highest possible level.\r\n");
    return;
  }
  if (newlevel == char_get_level(victim)) {
    send_to_char(ch, "They are already at that level.\r\n");
    return;
  }
  oldlevel = char_get_level(victim);
  if (newlevel < char_get_level(victim)) {
    send_to_char(ch, "You cannot demote a player.\r\n");
  } else {
    act("$n makes some strange gestures.\r\n"
        "A strange feeling comes upon you, like a giant hand, light comes down\r\n"
        "from above, grabbing your body, which begins to pulse with colored\r\n"
        "lights from inside.\r\n\r\n"
        "Your head seems to be filled with demons from another plane as your\r\n"
        "body dissolves to the elements of time and space itself.\r\n\r\n"
        "Suddenly a silent explosion of light snaps you back to reality.\r\n\r\n"
        "You feel slightly different.",
        FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(ch, "%s", CONFIG_OK);

  if (newlevel < oldlevel)
    mud_log("(GC) %s demoted %s from level %d to %d.",
            char_get_name(ch), char_get_name(victim), oldlevel, newlevel);
  else
    mud_log("(GC) %s has advanced %s to level %d (from %d)",
            char_get_name(ch), char_get_name(victim), newlevel, oldlevel);

  gain_exp_regardless(victim,
                      level_exp(victim, newlevel) - char_get_exp(victim));
  save_char(victim);
}

ACMD(do_handout)
{
  struct descriptor_data *j;

  if (char_get_admlevel(ch) < 3) {
    send_to_char(ch, "You can't do that.\r\n");
    return;
  }

  for (j = descriptor_list; j; j = j->next) {
    if (!desc_is_playing(j) || ch == j->character || char_get_admlevel(j->character) > 0)
      continue;
    if (char_is_npc(j->character))
      continue;
    else {
      char_set_practice_points(j->character, char_get_practice_points(j->character) + (10));
    }
  }

  send_to_all("@g%s@G hands out 10 practice sessions to everyone!@n\r\n", char_get_name(ch));
  mud_log("%s gave a handout of 10 PS to everyone.", char_get_name(ch));
  log_imm_action("HANDOUT: %s has handed out 10 PS to everyone.", char_get_name(ch));
}

ACMD(do_restore)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct descriptor_data *j;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to restore?\r\n");
  else if (is_abbrev(buf, "all")) {
    send_to_imm("[Log: %s restored all.]", char_get_name(ch));
    log_imm_action("RESTORE: %s has restored all players.", char_get_name(ch));
    for (j = descriptor_list; j; j = j->next) {
      if (!desc_is_playing(j) || !(vict = j->character))
        continue;
      restore_by(vict, ch);
    }
    send_to_char(ch, "Okay.\r\n");
  } else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (!char_is_npc(vict) && ch != vict && char_get_admlevel(vict) >= char_get_admlevel(ch))
    send_to_char(ch, "They don't need your help.\r\n");
  else {
    restore_by(vict, ch);
    send_to_char(ch, "%s", CONFIG_OK);
    send_to_imm("[Log: %s restored %s.]", char_get_name(ch), char_get_name(vict));
    log_imm_action("RESTORE: %s has restored %s.", char_get_name(ch), char_get_name(vict));
  }
}

void perform_immort_vis(struct char_data *ch)
{
  char_set_invis_level(ch, 0);
}

static void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  for (tch = world[char_get_in_room(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (char_get_admlevel(tch) >= char_get_invis_level(ch) && char_get_admlevel(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
          tch, TO_VICT);
    if (char_get_admlevel(tch) < char_get_invis_level(ch) && char_get_admlevel(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
          tch, TO_VICT);
  }

  char_set_invis_level(ch, level);
  send_to_char(ch, "Your invisibility level is %d.\r\n", level);
}

ACMD(do_invis)
{
  char arg[MAX_INPUT_LENGTH];
  int level;

  if (char_is_npc(ch)) {
    send_to_char(ch, "You can't do that!\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (char_get_invis_level(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, char_get_admlevel(ch));
  } else {
    level = atoi(arg);
    if (level > char_get_admlevel(ch))
      send_to_char(ch, "You can't go invisible above your own level.\r\n");
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}

/*
 *  General fn for wizcommands of the sort: cmd <player>
 */
ACMD(do_wizutil)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int taeller;
  long result;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Yes, but for whom?!?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "There is no such player.\r\n");
  else if (char_is_npc(vict))
    send_to_char(ch, "You can't do that to a mob!\r\n");
  else if (char_get_admlevel(vict) > char_get_admlevel(ch))
    send_to_char(ch, "Hmmm...you'd better not.\r\n");
  else {
    switch (subcmd) {
      case SCMD_REROLL:
        send_to_char(ch, "Rerolling is not possible at this time, bug Iovan about it...\r\n");
        mud_log("(GC) %s has rerolled %s.", char_get_name(ch), char_get_name(vict));
        send_to_char(ch, "New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
                     char_get_aff_abils_str(vict), char_get_aff_abils_intel(vict), char_get_aff_abils_wis(vict),
                     char_get_aff_abils_dex(vict), char_get_aff_abils_con(vict), char_get_aff_abils_cha(vict));
        break;
      case SCMD_PARDON:
        if (!char_plr_flagged(vict, PLR_THIEF) && !char_plr_flagged(vict, PLR_KILLER)) {
          send_to_char(ch, "Your victim is not flagged.\r\n");
          return;
        }
        char_plr_set(vict, PLR_THIEF, false);
        char_plr_set(vict, PLR_KILLER, false);
        send_to_char(ch, "Pardoned.\r\n");
        send_to_char(vict, "You have been pardoned by the Gods!\r\n");
        mudlog(LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "(GC) %s pardoned by %s", char_get_name(vict), char_get_name(ch));
        break;
      case SCMD_NOTITLE:
        result = char_plr_toggle(vict, PLR_NOTITLE);
        mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "(GC) Notitle %s for %s by %s.",
               ONOFF(result), char_get_name(vict), char_get_name(ch));
        send_to_char(ch, "(GC) Notitle %s for %s by %s.\r\n", ONOFF(result), char_get_name(vict), char_get_name(ch));
        break;
      case SCMD_SQUELCH:
        result = char_plr_toggle(vict, PLR_NOSHOUT);
        mudlog(LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "(GC) Squelch %s for %s by %s.",
               ONOFF(result), char_get_name(vict), char_get_name(ch));
        send_to_char(ch, "(GC) Mute turned %s for %s by %s.\r\n", ONOFF(result), char_get_name(vict), char_get_name(ch));
        send_to_all("@D[@RMUTE@D] @C%s@W has had mute turned @r%s@W by @C%s@W.\r\n", char_get_name(vict), ONOFF(result), char_get_name(ch));
        break;
      case SCMD_FREEZE:
        if (ch == vict) {
          send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
          return;
        }
        if (char_get_admlevel(ch) <= char_get_admlevel(vict)) {
          send_to_char(ch, "Pfft...\r\n");
          return;
        }
        if (char_plr_flagged(vict, PLR_FROZEN)) {
          send_to_char(ch, "Your victim is already pretty cold.\r\n");
          return;
        }
        char_plr_set(vict, PLR_FROZEN, true);
        char_set_freeze_level(vict, char_get_admlevel(ch));
        send_to_char(vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
        send_to_char(ch, "Frozen.\r\n");
        act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
        mudlog(LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "(GC) %s frozen by %s.", char_get_name(vict), char_get_name(ch));
        break;
      case SCMD_THAW:
        if (!char_plr_flagged(vict, PLR_FROZEN)) {
          send_to_char(ch, "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
          return;
        }
        if (char_get_freeze_level(vict) > char_get_admlevel(ch)) {
          send_to_char(ch, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
                       char_get_freeze_level(vict), char_get_name(vict), char_hmhr(vict));
          return;
        }
        mudlog(LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "(GC) %s un-frozen by %s.", char_get_name(vict), char_get_name(ch));
        char_plr_set(vict, PLR_FROZEN, false);
        send_to_char(vict, "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
        send_to_char(ch, "Thawed.\r\n");
        act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
        break;
      case SCMD_UNAFFECT:
        if (vict->affected || char_get_affected_by(vict)) {
          while (vict->affected)
            affect_remove(vict, vict->affected);
          for (taeller = 0; taeller < AF_ARRAY_MAX; taeller++)
            char_get_affected_by(vict)[taeller] = 0;
          send_to_char(vict, "There is a brief flash of light!\r\nYou feel slightly different.\r\n");
          send_to_char(ch, "All spells removed.\r\n");
        } else {
          send_to_char(ch, "Your victim does not have any affections!\r\n");
          return;
        }
        break;
      default:
        mud_log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
        /*  SYSERR_DESC:
       *  This is the same as the unhandled case in do_gen_ps(), but this
       *  function handles 'reroll', 'pardon', 'freeze', etc.
       */
        break;
    }
    save_char(vict);
  }
}

/***************** The do_set function ***********************************/

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC   0
#define BINARY 1
#define NUMBER 2

#define SET_OR_REMOVE(flagset, flags)            \
  {                                              \
    if (on) SET_BIT_AR(flagset, flags);          \
    else if (off) REMOVE_BIT_AR(flagset, flags); \
  }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

/* The set options available */
static const struct set_struct {
  const char *cmd;
  const char level;
  const char pcnpc;
  const char type;
} set_fields[] = {
    {"brief",      ADMLVL_GOD,     PC,   BINARY}, /* 0 */
    {"invstart",   ADMLVL_GOD,     PC,   BINARY}, /* 1 */
    {"title",      ADMLVL_GOD,     PC,   MISC  },
    {"nosummon",   ADMLVL_GRGOD,   PC,   BINARY},
    {"maxpl",      ADMLVL_BUILDER, BOTH, NUMBER},
    {"maxki",      ADMLVL_BUILDER, BOTH, NUMBER}, /* 5 */
    {"maxst",      ADMLVL_BUILDER, BOTH, NUMBER},
    {"pl",         ADMLVL_BUILDER, BOTH, NUMBER},
    {"ki",         ADMLVL_BUILDER, BOTH, NUMBER},
    {"sta",        ADMLVL_BUILDER, BOTH, NUMBER},
    {"align",      ADMLVL_BUILDER, BOTH, NUMBER}, /* 10 */
    {"str",        ADMLVL_BUILDER, BOTH, NUMBER},
    {"stradd",     ADMLVL_IMPL,    BOTH, NUMBER},
    {"int",        ADMLVL_BUILDER, BOTH, NUMBER},
    {"wis",        ADMLVL_BUILDER, BOTH, NUMBER},
    {"dex",        ADMLVL_BUILDER, BOTH, NUMBER}, /* 15 */
    {"con",        ADMLVL_BUILDER, BOTH, NUMBER},
    {"cha",        ADMLVL_BUILDER, BOTH, NUMBER},
    {"ac",         ADMLVL_IMPL,    BOTH, NUMBER},
    {"zenni",      ADMLVL_BUILDER, BOTH, NUMBER},
    {"bank",       ADMLVL_BUILDER, PC,   NUMBER}, /* 20 */
    {"exp",        ADMLVL_BUILDER, BOTH, NUMBER},
    {"accuracy",   ADMLVL_VICE,    BOTH, NUMBER},
    {"damage",     ADMLVL_VICE,    BOTH, NUMBER},
    {"invis",      ADMLVL_IMPL,    PC,   NUMBER},
    {"nohassle",   ADMLVL_VICE,    PC,   BINARY}, /* 25 */
    {"frozen",     ADMLVL_FREEZE,  PC,   BINARY},
    {"practices",  ADMLVL_BUILDER, PC,   NUMBER},
    {"lessons",    ADMLVL_IMPL,    PC,   NUMBER},
    {"drunk",      ADMLVL_GOD,     BOTH, MISC  },
    {"hunger",     ADMLVL_BUILDER, BOTH, MISC  }, /* 30 */
    {"thirst",     ADMLVL_BUILDER, BOTH, MISC  },
    {"killer",     ADMLVL_GOD,     PC,   BINARY},
    {"thief",      ADMLVL_GOD,     PC,   BINARY},
    {"level",      ADMLVL_GRGOD,   BOTH, NUMBER},
    {"room",       ADMLVL_IMPL,    BOTH, NUMBER}, /* 35 */
    {"roomflag",   ADMLVL_VICE,    PC,   BINARY},
    {"siteok",     ADMLVL_VICE,    PC,   BINARY},
    {"deleted",    ADMLVL_IMPL,    PC,   BINARY},
    {"class",      ADMLVL_VICE,    BOTH, MISC  },
    {"nowizlist",  ADMLVL_GOD,     PC,   BINARY}, /* 40 */
    {"quest",      ADMLVL_GOD,     PC,   BINARY},
    {"loadroom",   ADMLVL_VICE,    PC,   MISC  },
    {"color",      ADMLVL_GOD,     PC,   BINARY},
    {"idnum",      ADMLVL_IMPL,    PC,   NUMBER},
    {"passwd",     ADMLVL_IMPL,    PC,   MISC  }, /* 45 */
    {"nodelete",   ADMLVL_GOD,     PC,   BINARY},
    {"sex",        ADMLVL_VICE,    BOTH, MISC  },
    {"age",        ADMLVL_VICE,    BOTH, NUMBER},
    {"height",     ADMLVL_GOD,     BOTH, NUMBER},
    {"weight",     ADMLVL_GOD,     BOTH, NUMBER}, /* 50 */
    {"olc",        ADMLVL_GRGOD,   PC,   MISC  },
    {"race",       ADMLVL_VICE,    PC,   MISC  },
    {"trains",     ADMLVL_VICE,    PC,   NUMBER},
    {"feats",      ADMLVL_VICE,    PC,   NUMBER},
    {"ethic",      ADMLVL_GOD,     BOTH, NUMBER}, /* 55 */
    {"unused1",    ADMLVL_GRGOD,   BOTH, NUMBER},
    {"unused2",    ADMLVL_GRGOD,   BOTH, NUMBER},
    {"adminlevel", ADMLVL_GRGOD,   PC,   NUMBER},
    {"hairl",      ADMLVL_VICE,    PC,   NUMBER},
    {"hairs",      ADMLVL_VICE,    PC,   NUMBER},
    {"hairc",      ADMLVL_VICE,    PC,   NUMBER},
    {"skin",       ADMLVL_VICE,    PC,   NUMBER},
    {"eye",        ADMLVL_VICE,    PC,   NUMBER},
    {"basepl",     ADMLVL_BUILDER, BOTH, NUMBER},
    {"baseki",     ADMLVL_BUILDER, BOTH, NUMBER},
    {"basest",     ADMLVL_BUILDER, BOTH, NUMBER},
    {"droom",      ADMLVL_GRGOD,   BOTH, NUMBER},
    {"absorbs",    ADMLVL_GRGOD,   BOTH, NUMBER},
    {"ugp",        ADMLVL_GRGOD,   BOTH, NUMBER},
    {"aura",       ADMLVL_IMMORT,  BOTH, NUMBER},
    {"trp",        ADMLVL_GRGOD,   BOTH, NUMBER},
    {"boost",      ADMLVL_GRGOD,   BOTH, NUMBER},
    {"multi",      ADMLVL_IMPL,    PC,   BINARY},
    {"deaths",     ADMLVL_BUILDER, BOTH, NUMBER},
    {"user",       ADMLVL_IMPL,    PC,   MISC  },
    {"phase",      ADMLVL_IMMORT,  PC,   NUMBER},
    {"racial",     ADMLVL_IMMORT,  PC,   NUMBER},
    {"slots",      ADMLVL_IMMORT,  PC,   NUMBER},
    {"feature",    ADMLVL_IMMORT,  PC,   BINARY},
    {"tclass",     ADMLVL_VICE,    PC,   NUMBER},
    {"clones",     ADMLVL_IMPL,    PC,   NUMBER},
    {"armor",      ADMLVL_IMPL,    PC,   NUMBER},
    {"\n",         0,              BOTH, MISC  }
};

static int perform_set(struct char_data *ch, struct char_data *vict, int mode,
                       char *val_arg)
{
  int i, on = 0, off = 0;
  int64_t value = 0;
  room_rnum rnum;
  room_vnum rvnum;
  int chosen_race;
  int chosen_sensei;

  /* Check to make sure all the levels are correct */
  if (char_get_admlevel(ch) != ADMLVL_IMPL) {
    if (!char_is_npc(vict) && char_get_admlevel(ch) <= char_get_admlevel(vict) && vict != ch) {
      send_to_char(ch, "Maybe that's not such a great idea...\r\n");
      return (0);
    }
  }
  if (char_get_admlevel(ch) < set_fields[mode].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return (0);
  }

  /* Make sure the PC/NPC is correct */
  if (char_is_npc(vict) && !(set_fields[mode].pcnpc & NPC)) {
    send_to_char(ch, "You can't do that to a beast!\r\n");
    return (0);
  }
  if (!char_is_npc(vict) && !(set_fields[mode].pcnpc & PC)) {
    send_to_char(ch, "That can only be done to a beast!\r\n");
    return (0);
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char(ch, "Value must be 'on' or 'off'.\r\n");
      return (0);
    }
    send_to_char(ch, "%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), char_get_name(vict));
  } else if (set_fields[mode].type == NUMBER) {
    char *ptr;

    value = strtoll(val_arg, &ptr, 10);
    /*    value = atoi(val_arg); */
    send_to_char(ch, "%s's %s set to %" I64T ".\r\n", char_get_name(vict), set_fields[mode].cmd, value);
  } else
    send_to_char(ch, "%s", CONFIG_OK);

  switch (mode) {
    case 0:
      SET_OR_REMOVE(char_get_pref(vict), PRF_BRIEF);
      break;
    case 1:
      SET_OR_REMOVE(char_get_act(vict), PLR_INVSTART);
      break;
    case 2:
      set_title(vict, val_arg);
      send_to_char(ch, "%s's title is now: %s\r\n", char_get_name(vict), char_get_title(vict));
      break;
    case 3:
      SET_OR_REMOVE(char_get_pref(vict), PRF_SUMMONABLE);
      send_to_char(ch, "Nosummon %s for %s.\r\n", ONOFF(!on), char_get_name(vict));
      break;
    case 4:
      vict->max_hit = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set maxpl for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set maxpl for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 5:
      vict->max_mana = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set maxki for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set maxki for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 6:
      vict->max_move = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set maxsta for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set maxsta for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 7: /* pl: the legacy hit field it set no longer exists */
    case 8: /* ki: the legacy mana field it set no longer exists */
    case 9: /* st: the legacy move field it set no longer exists */
      send_to_char(ch, "This does nothing at the moment.\r\n");
      break;
    case 10:
      char_set_alignment(vict, RANGE(-1000, 1000));
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set align for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set align for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(vict);
      break;
    case 11:
      RANGE(0, 100);
      vict->real_abils.str = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set str for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set str for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(vict);
      break;
    case 12:
      send_to_char(ch, "Setting str_add does nothing now.\r\n");
      /* vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
       break; */
      break;
    case 13:
      RANGE(0, 100);
      vict->real_abils.intel = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set intel for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set intel for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(vict);
      break;
    case 14:
      RANGE(0, 100);
      vict->real_abils.wis = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set wis for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set wis for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(vict);
      break;
    case 15:
      RANGE(0, 100);
      vict->real_abils.dex = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set dex for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set dex for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(vict);
      break;
    case 16:
      RANGE(0, 100);
      vict->real_abils.con = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set con for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set con for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(vict);
      break;
    case 17:
      RANGE(0, 100);
      vict->real_abils.cha = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set speed for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set speed for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(vict);
      break;
    case 18:
      vict->armor = RANGE(-100, 500);
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set armor index for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set armor index for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(vict);
      break;
    case 19:
      char_set_gold(vict, RANGE(0, 100000000));
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set zenni for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set zenni for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 20:
      char_set_bank_gold(vict, RANGE(0, 100000000));
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set bank for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set bank for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 21:
      vict->exp = RANGE(0, 50000000);
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set exp for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set exp for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 22:
      send_to_char(ch, "This does nothing at the moment.\r\n");
      break;
    case 23: /* damage mod no longer exists */
      send_to_char(ch, "This does nothing at the moment.\r\n");
      break;
    case 24:
      if (char_get_admlevel(ch) < ADMLVL_IMPL && ch != vict) {
        send_to_char(ch, "You aren't godly enough for that!\r\n");
        return (0);
      }
      char_set_invis_level(vict, RANGE(0, char_get_admlevel(vict)));
      break;
    case 25:
      if (char_get_admlevel(ch) < ADMLVL_IMPL && ch != vict) {
        send_to_char(ch, "You aren't godly enough for that!\r\n");
        return (0);
      }
      SET_OR_REMOVE(char_get_pref(vict), PRF_NOHASSLE);
      break;
    case 26:
      if (ch == vict && on) {
        send_to_char(ch, "Better not -- could be a long winter!\r\n");
        return (0);
      }
      SET_OR_REMOVE(char_get_act(vict), PLR_FROZEN);
      break;
    case 27:
    case 28:
      if (char_get_class_level(vict)) {
        char_set_practice_points(vict, RANGE(0, 10000));
        mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set PS for %s.", char_get_name(ch), char_get_name(vict));
        log_imm_action("SET: %s has set PS for %s.", char_get_name(ch), char_get_name(vict));
      } else {
        /* race practices (skill_points) no longer exist */
        send_to_char(ch, "This does nothing at the moment.\r\n");
      }
      break;
    case 29:
    case 30:
    case 31:
      if (!strcasecmp(val_arg, "off")) {
        char_set_conditions(vict, (mode - 29), -1); /* warning: magic number here */
        send_to_char(ch, "%s's %s now off.\r\n", char_get_name(vict), set_fields[mode].cmd);
      } else if (is_number(val_arg)) {
        value = atoi(val_arg);
        RANGE(0, 48);
        char_set_conditions(vict, (mode - 29), value); /* and here too */
        send_to_char(ch, "%s's %s set to %" I64T ".\r\n", char_get_name(vict), set_fields[mode].cmd, value);
      } else {
        send_to_char(ch, "Must be 'off' or a value from 0 to 48.\r\n");
        return (0);
      }
      break;
    case 32:
      SET_OR_REMOVE(char_get_act(vict), PLR_KILLER);
      break;
    case 33:
      SET_OR_REMOVE(char_get_act(vict), PLR_THIEF);
      break;
    case 34:
      if (!char_is_npc(vict) && value > 100) {
        send_to_char(ch, "You can't do that.\r\n");
        return (0);
      }
      value       = MAX(0, value);
      vict->level = value;
      break;
    case 35:
      if ((rnum = real_room(value)) == NOWHERE) {
        send_to_char(ch, "No room exists with that number.\r\n");
        return (0);
      }
      if (char_get_in_room(vict) != NOWHERE) /* Another Eric Green special. */
        char_from_room(vict);
      char_to_room(vict, rnum);
      break;
    case 36:
      SET_OR_REMOVE(char_get_pref(vict), PRF_ROOMFLAGS);
      break;
    case 37:
      SET_OR_REMOVE(char_get_act(vict), PLR_SITEOK);
      break;
    case 38:
      SET_OR_REMOVE(char_get_act(vict), PLR_DELETED);
      break;
    case 39:
      if ((chosen_sensei = sensei_find(val_arg)) == CLASS_UNDEFINED) {
        send_to_char(ch, "That is not a class.\r\n");
        return (0);
      }
      value         = char_get_class_ranks(vict, char_get_class(vict));
      vict->chclass = chosen_sensei;
      break;
    case 40:
      SET_OR_REMOVE(char_get_act(vict), PLR_NOWIZLIST);
      break;
    case 41:
      SET_OR_REMOVE(char_get_pref(vict), PRF_QUEST);
      break;
    case 42:
      if (!strcasecmp(val_arg, "off")) {
        char_plr_set(vict, PLR_LOADROOM, false);
        char_set_load_room(vict, NOWHERE);
      } else if (is_number(val_arg)) {
        rvnum = atoi(val_arg);
        if (real_room(rvnum) != NOWHERE) {
          char_plr_set(vict, PLR_LOADROOM, true);
          char_set_load_room(vict, rvnum);
          send_to_char(ch, "%s will enter at room #%d.\r\n", char_get_name(vict), char_get_load_room(vict));
        } else {
          send_to_char(ch, "That room does not exist!\r\n");
          return (0);
        }
      } else {
        send_to_char(ch, "Must be 'off' or a room's virtual number.\r\n");
        return (0);
      }
      break;
    case 43:
      SET_OR_REMOVE(char_get_pref(vict), PRF_COLOR);
      break;
    case 44:
      if (char_get_idnum(ch) == 0 || char_is_npc(vict))
        return (0);
      char_set_idnum(vict, value);
      break;
    case 45:
      if (char_get_idnum(ch) > 1) {
        send_to_char(ch, "Please don't use this command, yet.\r\n");
        return (0);
      }
      if (char_get_admlevel(ch) < 10) {
        send_to_char(ch, "NO.\r\n");
        return (0);
      }
      break;
    case 46:
      SET_OR_REMOVE(char_get_act(vict), PLR_NODELETE);
      break;
    case 47:
      if ((i = search_block(val_arg, genders, FALSE)) < 0) {
        send_to_char(ch, "Must be 'male', 'female', or 'neutral'.\r\n");
        return (0);
      }
      char_set_sex(vict, i);
      break;
    case 48:                            /* set age */
      if (value < 2 || value > 20000) { /* Arbitrary limits. */
        send_to_char(ch, "Ages 2 to 20000 accepted.\r\n");
        return (0);
      }
      /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
      vict->time.birth = time(0) - (value * SECS_PER_MUD_YEAR);
      break;

    case 49: /* Blame/Thank Rick Glover. :) */
      char_set_height(vict, value);
      affect_total(vict);
      break;

    case 50:
      char_set_weight(vict, value);
      affect_total(vict);
      break;

    case 51:
      if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions"))
        char_set_olc_zone(vict, AEDIT_PERMISSION);
      else if (is_abbrev(val_arg, "hedit"))
        char_set_olc_zone(vict, HEDIT_PERMISSION);
      else if (is_abbrev(val_arg, "off"))
        char_set_olc_zone(vict, NOWHERE);
      else if (!is_number(val_arg)) {
        send_to_char(ch, "Value must be either 'socials', 'actions', 'hedit', 'off' or a zone number.\r\n");
        return (0);
      } else
        char_set_olc_zone(vict, atoi(val_arg));
      break;

    case 52:
      if (char_is_npc(vict)) {
        chosen_race = race_find(val_arg, char_get_sex(ch), false);
      } else {
        chosen_race = race_find(val_arg, char_get_sex(ch), true);
      }
      if (chosen_race == RACE_UNDEFINED) {
        send_to_char(ch, "That is not a valid race for them.\r\n");
        return (0);
      }
      vict->race = chosen_race;
      racial_body_parts(vict);
      break;

    case 53:
      char_set_ability_trains(vict, RANGE(0, 500));
      break;

    case 54:
      char_set_feat_points(vict, RANGE(0, 500));
      break;

    case 55:
      affect_total(vict);
      break;

    case 56:
      affect_total(vict);
      break;

    case 57:
      affect_total(vict);
      break;

    case 58:
      if (char_get_admlevel(vict) >= char_get_admlevel(ch) && vict != ch) {
        send_to_char(ch, "Permission denied.\r\n");
        return (0);
      }
      if (value < ADMLVL_NONE || value > char_get_admlevel(ch)) {
        send_to_char(ch, "You can't set it to that.\r\n");
        return (0);
      }
      if (char_get_admlevel(vict) == value)
        return (1);
      admin_set(vict, value);
      break;

    case 59:
      if (value < 0 || value >= 6) {
        send_to_char(ch, "You can't set it to that.\r\n");
        return (0);
      }
      char_set_hairl(vict, value);
      return (1);
      break;

    case 60:
      if (value < 0 || value >= 13) {
        send_to_char(ch, "You can't set it to that.\r\n");
        return (0);
      }
      char_set_hairs(vict, value);
      return (1);
      break;

    case 61:
      if (value < 0 || value >= 15) {
        send_to_char(ch, "You can't set it to that.\r\n");
        return (0);
      }
      char_set_hairc(vict, value);
      return (1);
      break;

    case 62:
      if (value < 0 || value >= 12) {
        send_to_char(ch, "You can't set it to that.\r\n");
        return (0);
      }
      char_set_skin(vict, value);
      return (1);
      break;

    case 63:
      if (value < 0 || value >= 13) {
        send_to_char(ch, "You can't set it to that.\r\n");
        return (0);
      }
      char_set_eye(vict, value);
      return (1);
      break;

    case 64:
      vict->basepl = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set basepl for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set basepl for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 65:
      vict->baseki = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set baseki for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set baseki for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 66:
      vict->basest = value;
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set basest for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set basest for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 67:
      char_set_droom(vict, RANGE(0, 20000));
      break;

    case 68:
      char_set_absorbs(vict, RANGE(0, 3));
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set absorbs for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set absorbs for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 69:
      char_set_upgrade(vict, char_get_upgrade(vict) + (RANGE(1, 1000)));
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set upgrade points for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set upgrade points for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 70:
      char_set_aura(vict, RANGE(0, 8));
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set aura for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set aura for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 71:
      send_to_char(ch, "Use the reward command.\r\n");
      break;
    case 72:
      char_set_boosts(vict, RANGE(-1000, 1000));
      break;
    case 73:
      SET_OR_REMOVE(char_get_act(vict), PLR_MULTP);
      break;
    case 74:
      char_set_dcount(vict, RANGE(-1000, 1000));
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set death count for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set death count for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 75:
      send_to_char(ch, "No.");
      break;
    case 76:
      if (vict->desc != NULL) {
        star_phase(vict, RANGE(0, 2));
      } else {
        send_to_char(ch, "They aren't even in the game!\r\n");
      }
      break;
    case 77:
      char_set_racial_pref(vict, RANGE(1, 3));
      break;

    case 78:
      char_set_skill_slots(vict, RANGE(1, 1000));
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set skill slots for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set skill slots for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 79:
      char_set_feature(vict, "");
      break;

    case 80:
      char_set_transclass(vict, RANGE(1, 3));
      mudlog(LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(ch)), TRUE, "SET: %s has set transformation class for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action("SET: %s has set transformation class for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 81:
      char_set_clones(vict, RANGE(1, 3));
      send_to_char(ch, "Done.\r\n");
      break;

    case 82:
      char_set_armor(vict, RANGE(0, 1000000));
      send_to_char(ch, "Done.\r\n");
      break;

    default:
      send_to_char(ch, "Can't set that!\r\n");
      return (0);
  }

  return (1);
}

ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop(argument, name, buf);

  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!strcasecmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!strcasecmp(name, "mob"))
    half_chop(buf, name, buf);

  half_chop(buf, field, buf);

  if (!*name || !*field) {
    send_to_char(ch, "Usage: set <victim> <field> <value>\r\n");
    return;
  }

  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
      }
    } else { /* is_mob */
      if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
        send_to_char(ch, "There is no such creature.\r\n");
        return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    CREATE(cbuf->player_specials, struct player_special_data, 1);
    if ((player_i = load_char(name, cbuf)) > -1) {
      if (char_get_admlevel(cbuf) >= char_get_admlevel(ch)) {
        free_char(cbuf);
        send_to_char(ch, "Sorry, you can't do that.\r\n");
        return;
      }
      vict = cbuf;
    } else {
      free_char(cbuf);
      send_to_char(ch, "There is no such player.\r\n");
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strcmp(field, set_fields[mode].cmd))
      break;
  if (*(set_fields[mode].cmd) == '\n')
    for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
      if (!strncmp(field, set_fields[mode].cmd, len))
        break;

  /* perform the set */
  retval = perform_set(ch, vict, mode, buf);

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !char_is_npc(vict))
      save_char(vict);
    if (is_file) {
      char_set_pfilepos(cbuf, player_i);
      save_char(cbuf);
      send_to_char(ch, "Saved in file.\r\n");
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}

ACMD(do_chown)
{
  struct char_data *victim;
  struct obj_data *obj;
  char buf2[MAX_INPUT_LENGTH];
  char buf3[MAX_INPUT_LENGTH];
  int i, k = 0;

  two_arguments(argument, buf2, buf3);

  if (!*buf2)
    send_to_char(ch, "Syntax: chown <object> <character>.\r\n");
  else if (!(victim = get_char_vis(ch, buf3, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No one by that name here.\r\n");
  else if (victim == ch)
    send_to_char(ch, "Are you sure you're feeling ok?\r\n");
  else if (char_get_level(victim) >= char_get_level(ch))
    send_to_char(ch, "That's really not such a good idea.\r\n");
  else if (!*buf3)
    send_to_char(ch, "Syntax: chown <object> <character>.\r\n");
  else {
    for (i = 0; i < NUM_WEARS; i++) {
      if (char_get_equipment(victim, i) && char_can_see_obj(ch, char_get_equipment(victim, i)) &&
          isname(buf2, char_get_equipment(victim, i)->name)) {
        obj_to_char(unequip_char(victim, i), victim);
        k = 1;
      }
    }

    if (!(obj = get_obj_in_list_vis(victim, buf2, NULL, victim->carrying))) {
      if (!k && !(obj = get_obj_in_list_vis(victim, buf2, NULL, victim->carrying))) {
        send_to_char(ch, "%s does not appear to have the %s.\r\n", char_get_name(victim), buf2);
        return;
      }
    }

    act("@n$n makes a magical gesture and $p@n flies from $N to $m.", FALSE, ch, obj, victim, TO_NOTVICT);
    act("@n$n makes a magical gesture and $p@n flies away from you to $m.", FALSE, ch, obj, victim, TO_VICT);
    act("@nYou make a magical gesture and $p@n flies away from $N to you.", FALSE, ch, obj, victim, TO_CHAR);

    obj_from_char(obj);
    obj_to_char(obj, ch);
    save_char(ch);
    save_char(victim);
  }
}
