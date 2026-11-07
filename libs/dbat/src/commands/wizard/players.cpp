/* ************************************************************************
*   File: wizard/players.c                                  Part of CircleMUD *
*  Usage: God commands that change players                                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <climits>
#include <cstdlib>

#include <unistd.h>

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

/* local functions */
static void print_lockout(GameWorld *g, struct char_data *ch);
static bool perform_set(GameWorld *g, struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
static void perform_immort_invis(GameWorld *g, struct char_data *ch, int level);
static void lockWrite(GameWorld *g, struct char_data *ch, char *name);

static void print_lockout(GameWorld *g, struct char_data *ch)
{
  if (char_is_npc(ch))
    return;

  FILE *file;
  char fname[40], filler[50], line[256], buf[MAX_STRING_LENGTH * 4];
  int count = 0, first = true;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
    send_to_char(g, ch, "The lockout file does not exist.");
    return;
  }
  if (!(file = fopen(fname, "r"))) {
    send_to_char(g, ch, "The lockout file does not exist.");
    return;
  }
  snprintf(buf, sizeof(buf), "@b------------------[ @RLOCKOUT @b]------------------@n\n");
  while (!feof(file)) {
    get_line(file, line);
    sscanf(line, "%49s\n", filler);
    if (first != true && strstr(buf, filler) == nullptr) {
      if (count == 0) {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%-23s@D|@n", filler);
        count = 1;
      } else {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%-23s\n", filler);
        count = 0;
      }
    } else if (first == true) {
      snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%-23s@D|@n", filler);
      first = false;
      count = 1;
    }
    *filler = '\0';
  }
  if (count == 1) {
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "\n");
  }
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "@b------------------[ @RLOCKOUT @b]------------------@n\n");

  send_to_char(g, ch, "%s", buf);

  fclose(file);
}

ACMD(do_approve)
{

  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict = nullptr;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "What player do you want to approve as having an acceptable bio?\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "That player is not in the game.\r\n");
    return;
  }

  if (char_plr_flagged(vict, PLR_BIOGR)) {
    send_to_char(g, ch, "They have already been approved. If this was made in error inform Iovan.\r\n");
    return;
  }
  char_plr_set(vict, PLR_BIOGR, true);
  send_to_char(g, ch, "They have now been approved.\r\n");
  return;
}

static void lockWrite(GameWorld *g, struct char_data *ch, char *name)
{
  struct atomic_file af;
  FILE *file;
  char fname[40], filler[50], line[256];
  char *names[500] = {nullptr};
  FILE *fl;
  int count = 0, x = 0, found = false;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
    send_to_char(g, ch, "The lockout file does not exist.");
    return;
  }
  if (!(file = fopen(fname, "r"))) {
    send_to_char(g, ch, "The lockout file does not exist.");
    return;
  }
  while (!feof(file) || count < 498) {
    get_line(file, line);
    sscanf(line, "%49s\n", filler);
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
        found = true;
      }
    }
    x++;
  }
  if (found == false) {
    fprintf(fl, "%s\n", CAP(name));
    send_to_all(g, "@rLOCKOUT@D: @WThe character, @C%s@W, was locked out of the MUD by @c%s@W.@n\r\n", CAP(name), char_get_name(ch));
    mud_log("LOCKOUT: %s sentenced by %s.", CAP(name), char_get_name(ch));
    log_imm_action(g, "LOCKOUT: %s sentenced by %s.", CAP(name), char_get_name(ch));
  } else {
    send_to_all(g, "@rLOCKOUT@D: @WThe character, @C%s@W, has had lockout removed by @c%s@W.@n\r\n", CAP(name), char_get_name(ch));
    mud_log("LOCKOUT: %s sentenced by %s.", CAP(name), char_get_name(ch));
    log_imm_action(g, "LOCKOUT: %s sentenced by %s.", CAP(name), char_get_name(ch));
  }

  atomic_commit(&af);
  return;
}

ACMD(do_reward)
{
  struct char_data *vict = nullptr;
  int amt = 0;

  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "Syntax: reward (target) (amount)\r\nThis is either a positive number or a negative.\r\n");
    return;
  }

  if (!readUserIndex(arg)) {
    send_to_char(g, ch, "That is not a recognised user file.\r\n");
    return;
  }

  amt = atoi(arg2);

  if (amt == 0) {
    send_to_char(g, ch, "That is pointless don't you think? Try an amount higher than 0.\r\n");
    return;
  }

  for (descriptor_data *k : each_desc(g)) {
    if (desc_get_connected(k) != CON_PLAYING)
      continue;
    if (char_is_npc(k->character))
      continue;
    if (!strcasecmp(k->user, arg))
      vict = k->character;
  }

  if (amt > 0) {
    if (vict != nullptr) {
      send_to_char(g, ch, "@WYou award @C%s @D(@G%d@D)@W RP points.@n\r\n", char_get_name(vict), amt);
      send_to_char(g, vict, "@D[@YROLEPLAY@D] @WYou have been awarded @D(@G%d@D)@W RP points by @C%s@W.@n\r\n", amt, char_get_name(ch));
      send_to_imm(g, "ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      log_imm_action(g, "ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      char_set_rp(vict, char_get_rp(vict) + amt);
      char_set_rp(vict, char_get_rp(vict));
      if (amt <= 29) {
        char_set_trp(vict, char_get_trp(vict) + (amt));
      }
      char_get_desc(vict)->rpp += amt;
      userWrite(g, char_get_desc(vict), 0, 0, 0, "index");
    } else {
      send_to_char(g, ch, "@WYou award user @C%s @D(@G%d@D)@W RP points.@n\r\n", arg, amt);
      send_to_imm(g, "ROLEPLAY: User %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      log_imm_action(g, "ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      userWrite(g, nullptr, 0, amt, 0, arg);
    }
  } else {
    if (vict != nullptr) {
      send_to_char(g, ch, "@WYou deduct @D(@G%d@D)@W RP points from @C%s@W.@n\r\n", amt, char_get_name(vict));
      send_to_char(g, vict, "@D[@YROLEPLAY@D] @C%s@W deducts @D(@G%d@D)@W RP points from you.@n\r\n", char_get_name(ch), amt);
      send_to_imm(g, "ROLEPLAY: %s has had %d RP points deducted by %s.", char_get_name(vict), amt, char_get_name(ch));
      log_imm_action(g, "ROLEPLAY: %s has had %d RP points deducted by %s.", char_get_name(vict), amt, char_get_name(ch));
      char_set_rp(vict, char_get_rp(vict) + amt);
      char_set_rp(vict, char_get_rp(vict));
      char_get_desc(vict)->rpp += amt;
      userWrite(g, char_get_desc(vict), 0, 0, 0, "index");
    } else {
      send_to_char(g, ch, "@WYou deduct @D(@G%d@D)@W RP points from user @C%s@W.@n\r\n", amt, arg);
      send_to_imm(g, "ROLEPLAY: User %s has had %d RP points deducted by %s.", arg, amt, char_get_name(ch));
      log_imm_action(g, "ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, char_get_name(ch));
      userWrite(g, nullptr, 0, amt, 0, arg);
    }
  }
}

ACMD(do_permission)
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(g, ch, "You want to @Grestrict@n or @Gunrestrict@n?\r\n");
    return;
  }

  if (!*arg2 && !strcasecmp("unrestrict", arg)) {
    send_to_char(g, ch, "You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
    return;
  }
  if (!strcasecmp("unrestrict", arg)) {
    if (!strcasecmp("saiyan", arg2)) {
      send_to_char(g, ch, "You have unrestricted saiyans for the very next character creation.\r\n");
      send_to_imm(g, "PERMISSION: %s unrestricted saiyans.", char_get_name(ch));
      g->SAIYAN_ALLOWED = true;
    } else if (!strcasecmp("majin", arg2)) {
      send_to_char(g, ch, "You have unrestricted majins for the very next character creation.\r\n");
      send_to_imm(g, "PERMISSION: %s unrestricted majins.", char_get_name(ch));
      g->MAJIN_ALLOWED = true;
    } else {
      send_to_char(g, ch, "You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
      return;
    }
  } else if (!strcasecmp("restrict", arg)) {
    send_to_char(g, ch, "You have restricted character creation to standard race slection.\r\n");
    send_to_imm(g, "PERMISSION: %s restricted races again.", char_get_name(ch));
    g->MAJIN_ALLOWED = false;
  } else {
    send_to_char(g, ch, "You want to @Grestrict@n or @Gunrestrict@n?\r\n");
    return;
  }
}

ACMD(do_interest)
{
  if (char_get_admlevel(ch) < 5) {
    send_to_char(g, ch, "Huh!?\r\n");
    return;
  }
  if (g->INTERESTTIME > 0) {
    char *tmstr;
    tmstr                        = asctime(localtime(&g->INTERESTTIME));
    *(tmstr + strlen(tmstr) - 1) = '\0';
    send_to_char(g, ch, "INTEREST TIME: [%s]\r\n", tmstr);
    return;
  }
  send_to_char(g, ch, "Interest time has been initiated!\r\n");
  g->INTERESTTIME = game_now(g) + 86400;
  g->LASTINTEREST = game_now(g) + 86400;
  return;
}

ACMD(do_hell)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "Syntax: lockout (character)\n"
                     "        lockout list\r\n");
    return;
  }

  if (!strcasecmp(arg, "Iovan") || !strcasecmp(arg, "iovan") || !strcasecmp(arg, "Fahl") || !strcasecmp(arg, "fahl") || !strcasecmp(arg, "Xyron") || !strcasecmp(arg, "xyron") || !strcasecmp(arg, "Samael") || !strcasecmp(arg, "samael")) {
    send_to_char(g, ch, "What are you smoking? You can't lockout senior imms.\r\n");
    return;
  }

  if (!strcasecmp(arg, "list")) {
    print_lockout(g, ch);
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    lockWrite(g, ch, arg);
    return;
  }
  struct descriptor_data *d = char_get_desc(vict);
  Crash_rentsave(g, vict, 0);
  extract_char(g, vict);
  lockWrite(g, ch, char_get_name(vict));
  if (d && desc_get_connected(d) != CON_PLAYING) {
    desc_set_connected(d, CON_CLOSE);
    char_get_desc(vict)->character = nullptr;
    char_set_desc(vict, nullptr);
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
    if (!(victim = get_char_vis(g, ch, name, nullptr, FIND_CHAR_WORLD))) {
      send_to_char(g, ch, "That player is not here.\r\n");
      return;
    }
  } else {
    send_to_char(g, ch, "Advance who?\r\n");
    return;
  }

  if (char_is_npc(victim)) {
    send_to_char(g, ch, "NO!  Not on NPC's.\r\n");
    return;
  }
  if (!*level) {
    send_to_char(g, ch, "[ 1 - 100 | demote ]\r\n");
    return;
  }
  if ((newlevel = atoi(level)) <= 0) {
    if (!strcasecmp("demote", level)) {
      char_set_class_level(victim, 1);
      char_set_basepl(victim, 150);
      char_set_baseki(victim, 150);
      char_set_basest(victim, 150);
      send_to_char(g, ch, "They have now been demoted!\r\n");
      send_to_char(g, victim, "You were demoted to level 1!\r\n");
      return;
    }
    send_to_char(g, ch, "That's not a level!\r\n");
    return;
  }
  if (newlevel > 100) {
    send_to_char(g, ch, "100 is the highest possible level.\r\n");
    return;
  }
  if (newlevel == char_get_level(victim)) {
    send_to_char(g, ch, "They are already at that level.\r\n");
    return;
  }
  oldlevel = char_get_level(victim);
  if (newlevel < char_get_level(victim)) {
    send_to_char(g, ch, "You cannot demote a player.\r\n");
  } else {
    act(g, "$n makes some strange gestures.\r\n"
        "A strange feeling comes upon you, like a giant hand, light comes down\r\n"
        "from above, grabbing your body, which begins to pulse with colored\r\n"
        "lights from inside.\r\n\r\n"
        "Your head seems to be filled with demons from another plane as your\r\n"
        "body dissolves to the elements of time and space itself.\r\n\r\n"
        "Suddenly a silent explosion of light snaps you back to reality.\r\n\r\n"
        "You feel slightly different.",
        false, ch, nullptr, victim, TO_VICT);
  }

  send_to_char(g, ch, "%s", CONFIG_OK);

  if (newlevel < oldlevel)
    mud_log("(GC) %s demoted %s from level %d to %d.",
            char_get_name(ch), char_get_name(victim), oldlevel, newlevel);
  else
    mud_log("(GC) %s has advanced %s to level %d (from %d)",
            char_get_name(ch), char_get_name(victim), newlevel, oldlevel);

  gain_exp_regardless(g, victim,
                      level_exp(victim, newlevel) - char_get_exp(victim));
  save_char(g, victim);
}

ACMD(do_handout)
{

  if (char_get_admlevel(ch) < 3) {
    send_to_char(g, ch, "You can't do that.\r\n");
    return;
  }

  for (descriptor_data *j : each_desc(g)) {
    if (!desc_is_playing(j) || ch == j->character || char_get_admlevel(j->character) > 0)
      continue;
    if (char_is_npc(j->character))
      continue;
    else {
      char_set_practice_points(g, j->character, char_get_practice_points(g, j->character) + (10));
    }
  }

  send_to_all(g, "@g%s@G hands out 10 practice sessions to everyone!@n\r\n", char_get_name(ch));
  mud_log("%s gave a handout of 10 PS to everyone.", char_get_name(ch));
  log_imm_action(g, "HANDOUT: %s has handed out 10 PS to everyone.", char_get_name(ch));
}

ACMD(do_restore)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(g, ch, "Whom do you wish to restore?\r\n");
  else if (is_abbrev(buf, "all")) {
    send_to_imm(g, "[Log: %s restored all.]", char_get_name(ch));
    log_imm_action(g, "RESTORE: %s has restored all players.", char_get_name(ch));
    for (descriptor_data *j : each_desc(g)) {
      if (!desc_is_playing(j) || !(vict = j->character))
        continue;
      restore_by(g, vict, ch);
    }
    send_to_char(g, ch, "Okay.\r\n");
  } else if (!(vict = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_WORLD)))
    send_to_char(g, ch, "%s", CONFIG_NOPERSON);
  else if (!char_is_npc(vict) && ch != vict && char_get_admlevel(vict) >= char_get_admlevel(ch))
    send_to_char(g, ch, "They don't need your help.\r\n");
  else {
    restore_by(g, vict, ch);
    send_to_char(g, ch, "%s", CONFIG_OK);
    send_to_imm(g, "[Log: %s restored %s.]", char_get_name(ch), char_get_name(vict));
    log_imm_action(g, "RESTORE: %s has restored %s.", char_get_name(ch), char_get_name(vict));
  }
}

void perform_immort_vis(GameWorld *g, struct char_data *ch)
{
  char_set_invis_level(g, ch, 0);
}

static void perform_immort_invis(GameWorld *g, struct char_data *ch, int level)
{

  for (char_data *tch : room_people(g, char_get_in_room(ch))) {
    if (tch == ch)
      continue;
    if (char_get_admlevel(tch) >= char_get_invis_level(g, ch) && char_get_admlevel(tch) < level)
      act(g, "You blink and suddenly realize that $n is gone.", false, ch, nullptr,
          tch, TO_VICT);
    if (char_get_admlevel(tch) < char_get_invis_level(g, ch) && char_get_admlevel(tch) >= level)
      act(g, "You suddenly realize that $n is standing beside you.", false, ch, nullptr,
          tch, TO_VICT);
  }

  char_set_invis_level(g, ch, level);
  send_to_char(g, ch, "Your invisibility level is %d.\r\n", level);
}

ACMD(do_invis)
{
  char arg[MAX_INPUT_LENGTH];
  int level;

  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You can't do that!\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (char_get_invis_level(g, ch) > 0)
      perform_immort_vis(g, ch);
    else
      perform_immort_invis(g, ch, char_get_admlevel(ch));
  } else {
    level = atoi(arg);
    if (level > char_get_admlevel(ch))
      send_to_char(g, ch, "You can't go invisible above your own level.\r\n");
    else if (level < 1)
      perform_immort_vis(g, ch);
    else
      perform_immort_invis(g, ch, level);
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
    send_to_char(g, ch, "Yes, but for whom?!?\r\n");
  else if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD)))
    send_to_char(g, ch, "There is no such player.\r\n");
  else if (char_is_npc(vict))
    send_to_char(g, ch, "You can't do that to a mob!\r\n");
  else if (char_get_admlevel(vict) > char_get_admlevel(ch))
    send_to_char(g, ch, "Hmmm...you'd better not.\r\n");
  else {
    switch (subcmd) {
      case SCMD_REROLL:
        send_to_char(g, ch, "Rerolling is not possible at this time, bug Iovan about it...\r\n");
        mud_log("(GC) %s has rerolled %s.", char_get_name(ch), char_get_name(vict));
        send_to_char(g, ch, "New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
                     char_get_aff_abils_str(vict), char_get_aff_abils_intel(vict), char_get_aff_abils_wis(vict),
                     char_get_aff_abils_dex(vict), char_get_aff_abils_con(vict), char_get_aff_abils_cha(vict));
        break;
      case SCMD_PARDON:
        if (!char_plr_flagged(vict, PLR_THIEF) && !char_plr_flagged(vict, PLR_KILLER)) {
          send_to_char(g, ch, "Your victim is not flagged.\r\n");
          return;
        }
        char_plr_set(vict, PLR_THIEF, false);
        char_plr_set(vict, PLR_KILLER, false);
        send_to_char(g, ch, "Pardoned.\r\n");
        send_to_char(g, vict, "You have been pardoned by the Gods!\r\n");
        mudlog(g, LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s pardoned by %s", char_get_name(vict), char_get_name(ch));
        break;
      case SCMD_NOTITLE:
        result = char_plr_toggle(vict, PLR_NOTITLE);
        mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) Notitle %s for %s by %s.",
               ONOFF(result), char_get_name(vict), char_get_name(ch));
        send_to_char(g, ch, "(GC) Notitle %s for %s by %s.\r\n", ONOFF(result), char_get_name(vict), char_get_name(ch));
        break;
      case SCMD_SQUELCH:
        result = char_plr_toggle(vict, PLR_NOSHOUT);
        mudlog(g, LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) Squelch %s for %s by %s.",
               ONOFF(result), char_get_name(vict), char_get_name(ch));
        send_to_char(g, ch, "(GC) Mute turned %s for %s by %s.\r\n", ONOFF(result), char_get_name(vict), char_get_name(ch));
        send_to_all(g, "@D[@RMUTE@D] @C%s@W has had mute turned @r%s@W by @C%s@W.\r\n", char_get_name(vict), ONOFF(result), char_get_name(ch));
        break;
      case SCMD_FREEZE:
        if (ch == vict) {
          send_to_char(g, ch, "Oh, yeah, THAT'S real smart...\r\n");
          return;
        }
        if (char_get_admlevel(ch) <= char_get_admlevel(vict)) {
          send_to_char(g, ch, "Pfft...\r\n");
          return;
        }
        if (char_plr_flagged(vict, PLR_FROZEN)) {
          send_to_char(g, ch, "Your victim is already pretty cold.\r\n");
          return;
        }
        char_plr_set(vict, PLR_FROZEN, true);
        char_set_freeze_level(g, vict, char_get_admlevel(ch));
        send_to_char(g, vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
        send_to_char(g, ch, "Frozen.\r\n");
        act(g, "A sudden cold wind conjured from nowhere freezes $n!", false, vict, nullptr, nullptr, TO_ROOM);
        mudlog(g, LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s frozen by %s.", char_get_name(vict), char_get_name(ch));
        break;
      case SCMD_THAW:
        if (!char_plr_flagged(vict, PLR_FROZEN)) {
          send_to_char(g, ch, "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
          return;
        }
        if (char_get_freeze_level(g, vict) > char_get_admlevel(ch)) {
          send_to_char(g, ch, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
                       char_get_freeze_level(g, vict), char_get_name(vict), char_hmhr(vict));
          return;
        }
        mudlog(g, LOG_BRF, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "(GC) %s un-frozen by %s.", char_get_name(vict), char_get_name(ch));
        char_plr_set(vict, PLR_FROZEN, false);
        send_to_char(g, vict, "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
        send_to_char(g, ch, "Thawed.\r\n");
        act(g, "A sudden fireball conjured from nowhere thaws $n!", false, vict, nullptr, nullptr, TO_ROOM);
        break;
      case SCMD_UNAFFECT:
        if (char_get_affected(vict) || char_get_affected_by(vict)) {
          while (char_get_affected(vict))
            affect_remove(g, vict, char_get_affected(vict));
          for (taeller = 0; taeller < AF_ARRAY_MAX; taeller++)
            char_get_affected_by(vict)[taeller] = 0;
          send_to_char(g, vict, "There is a brief flash of light!\r\nYou feel slightly different.\r\n");
          send_to_char(g, ch, "All spells removed.\r\n");
        } else {
          send_to_char(g, ch, "Your victim does not have any affections!\r\n");
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
    save_char(g, vict);
  }
}

/***************** The do_set function ***********************************/

constexpr int PC   = 1;
constexpr int NPC  = 2;
constexpr int BOTH = 3;

constexpr int MISC   = 0;
constexpr int BINARY = 1;
constexpr int NUMBER = 2;

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

static bool perform_set(GameWorld *g, struct char_data *ch, struct char_data *vict, int mode,
                        char *val_arg)
{
  int i, on = 0, off = 0;
  int64_t value = 0;
  room_vnum rnum;
  room_vnum rvnum;
  int chosen_race;
  int chosen_sensei;

  /* Check to make sure all the levels are correct */
  if (char_get_admlevel(ch) != ADMLVL_IMPL) {
    if (!char_is_npc(vict) && char_get_admlevel(ch) <= char_get_admlevel(vict) && vict != ch) {
      send_to_char(g, ch, "Maybe that's not such a great idea...\r\n");
      return false;
    }
  }
  if (char_get_admlevel(ch) < set_fields[mode].level) {
    send_to_char(g, ch, "You are not godly enough for that!\r\n");
    return false;
  }

  /* Make sure the PC/NPC is correct */
  if (char_is_npc(vict) && !(set_fields[mode].pcnpc & NPC)) {
    send_to_char(g, ch, "You can't do that to a beast!\r\n");
    return false;
  }
  if (!char_is_npc(vict) && !(set_fields[mode].pcnpc & PC)) {
    send_to_char(g, ch, "That can only be done to a beast!\r\n");
    return false;
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char(g, ch, "Value must be 'on' or 'off'.\r\n");
      return false;
    }
    send_to_char(g, ch, "%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), char_get_name(vict));
  } else if (set_fields[mode].type == NUMBER) {
    char *ptr;

    value = strtoll(val_arg, &ptr, 10);
    /*    value = atoi(val_arg); */
    send_to_char(g, ch, "%s's %s set to %" PRId64 ".\r\n", char_get_name(vict), set_fields[mode].cmd, value);
  } else
    send_to_char(g, ch, "%s", CONFIG_OK);

  switch (mode) {
    case 0:
      SET_OR_REMOVE(char_get_pref(g, vict), PRF_BRIEF);
      break;
    case 1:
      SET_OR_REMOVE(char_get_act(vict), PLR_INVSTART);
      break;
    case 2:
      set_title(g, vict, val_arg);
      send_to_char(g, ch, "%s's title is now: %s\r\n", char_get_name(vict), char_get_title(vict));
      break;
    case 3:
      SET_OR_REMOVE(char_get_pref(g, vict), PRF_SUMMONABLE);
      send_to_char(g, ch, "Nosummon %s for %s.\r\n", ONOFF(!on), char_get_name(vict));
      break;
    case 4:
      char_set_max_hit(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set maxpl for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set maxpl for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 5:
      char_set_max_mana(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set maxki for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set maxki for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 6:
      char_set_max_move(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set maxsta for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set maxsta for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 7: /* pl: the legacy hit field it set no longer exists */
    case 8: /* ki: the legacy mana field it set no longer exists */
    case 9: /* st: the legacy move field it set no longer exists */
      send_to_char(g, ch, "This does nothing at the moment.\r\n");
      break;
    case 10:
      char_set_alignment(vict, RANGE(-1000, 1000));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set align for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set align for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(g, vict);
      break;
    case 11:
      RANGE(0, 100);
      char_set_real_abils_str(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set str for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set str for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(g, vict);
      break;
    case 12:
      send_to_char(g, ch, "Setting str_add does nothing now.\r\n");
      /* vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
       break; */
      break;
    case 13:
      RANGE(0, 100);
      char_set_real_abils_intel(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set intel for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set intel for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(g, vict);
      break;
    case 14:
      RANGE(0, 100);
      char_set_real_abils_wis(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set wis for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set wis for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(g, vict);
      break;
    case 15:
      RANGE(0, 100);
      char_set_real_abils_dex(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set dex for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set dex for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(g, vict);
      break;
    case 16:
      RANGE(0, 100);
      char_set_real_abils_con(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set con for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set con for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(g, vict);
      break;
    case 17:
      RANGE(0, 100);
      char_set_real_abils_cha(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set speed for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set speed for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(g, vict);
      break;
    case 18:
      char_set_armor(vict, RANGE(-100, 500));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set armor index for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set armor index for %s.", char_get_name(ch), char_get_name(vict));
      affect_total(g, vict);
      break;
    case 19:
      char_set_gold(vict, RANGE(0, 100000000));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set zenni for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set zenni for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 20:
      char_set_bank_gold(vict, RANGE(0, 100000000));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set bank for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set bank for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 21:
      char_set_exp(vict, RANGE(0, 50000000));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set exp for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set exp for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 22:
      send_to_char(g, ch, "This does nothing at the moment.\r\n");
      break;
    case 23: /* damage mod no longer exists */
      send_to_char(g, ch, "This does nothing at the moment.\r\n");
      break;
    case 24:
      if (char_get_admlevel(ch) < ADMLVL_IMPL && ch != vict) {
        send_to_char(g, ch, "You aren't godly enough for that!\r\n");
        return false;
      }
      char_set_invis_level(g, vict, RANGE(0, char_get_admlevel(vict)));
      break;
    case 25:
      if (char_get_admlevel(ch) < ADMLVL_IMPL && ch != vict) {
        send_to_char(g, ch, "You aren't godly enough for that!\r\n");
        return false;
      }
      SET_OR_REMOVE(char_get_pref(g, vict), PRF_NOHASSLE);
      break;
    case 26:
      if (ch == vict && on) {
        send_to_char(g, ch, "Better not -- could be a long winter!\r\n");
        return false;
      }
      SET_OR_REMOVE(char_get_act(vict), PLR_FROZEN);
      break;
    case 27:
    case 28:
      if (char_get_class_level(vict)) {
        char_set_practice_points(g, vict, RANGE(0, 10000));
        mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set PS for %s.", char_get_name(ch), char_get_name(vict));
        log_imm_action(g, "SET: %s has set PS for %s.", char_get_name(ch), char_get_name(vict));
      } else {
        /* race practices (skill_points) no longer exist */
        send_to_char(g, ch, "This does nothing at the moment.\r\n");
      }
      break;
    case 29:
    case 30:
    case 31:
      if (!strcasecmp(val_arg, "off")) {
        char_set_conditions(g, vict, (mode - 29), -1); /* warning: magic number here */
        send_to_char(g, ch, "%s's %s now off.\r\n", char_get_name(vict), set_fields[mode].cmd);
      } else if (is_number(val_arg)) {
        value = atoi(val_arg);
        RANGE(0, 48);
        char_set_conditions(g, vict, (mode - 29), value); /* and here too */
        send_to_char(g, ch, "%s's %s set to %" PRId64 ".\r\n", char_get_name(vict), set_fields[mode].cmd, value);
      } else {
        send_to_char(g, ch, "Must be 'off' or a value from 0 to 48.\r\n");
        return false;
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
        send_to_char(g, ch, "You can't do that.\r\n");
        return false;
      }
      value       = MAX(0, value);
      char_set_class_level(vict, value);
      break;
    case 35:
      if ((rnum = real_room(g, value)) == NOWHERE) {
        send_to_char(g, ch, "No room exists with that number.\r\n");
        return false;
      }
      if (char_get_in_room(vict) != NOWHERE) /* Another Eric Green special. */
        char_from_room(g, vict);
      char_to_room(g, vict, rnum);
      break;
    case 36:
      SET_OR_REMOVE(char_get_pref(g, vict), PRF_ROOMFLAGS);
      break;
    case 37:
      SET_OR_REMOVE(char_get_act(vict), PLR_SITEOK);
      break;
    case 38:
      SET_OR_REMOVE(char_get_act(vict), PLR_DELETED);
      break;
    case 39:
      if ((chosen_sensei = sensei_find(val_arg)) == CLASS_UNDEFINED) {
        send_to_char(g, ch, "That is not a class.\r\n");
        return false;
      }
      value         = char_get_class_ranks(vict, char_get_class(vict));
      char_set_class(vict, chosen_sensei);
      break;
    case 40:
      SET_OR_REMOVE(char_get_act(vict), PLR_NOWIZLIST);
      break;
    case 41:
      SET_OR_REMOVE(char_get_pref(g, vict), PRF_QUEST);
      break;
    case 42:
      if (!strcasecmp(val_arg, "off")) {
        char_plr_set(vict, PLR_LOADROOM, false);
        char_set_load_room(g, vict, NOWHERE);
      } else if (is_number(val_arg)) {
        rvnum = atoi(val_arg);
        if (real_room(g, rvnum) != NOWHERE) {
          char_plr_set(vict, PLR_LOADROOM, true);
          char_set_load_room(g, vict, rvnum);
          send_to_char(g, ch, "%s will enter at room #%d.\r\n", char_get_name(vict), char_get_load_room(g, vict));
        } else {
          send_to_char(g, ch, "That room does not exist!\r\n");
          return false;
        }
      } else {
        send_to_char(g, ch, "Must be 'off' or a room's virtual number.\r\n");
        return false;
      }
      break;
    case 43:
      SET_OR_REMOVE(char_get_pref(g, vict), PRF_COLOR);
      break;
    case 44:
      if (char_get_idnum(ch) == 0 || char_is_npc(vict))
        return false;
      char_set_idnum(vict, value);
      break;
    case 45:
      if (char_get_idnum(ch) > 1) {
        send_to_char(g, ch, "Please don't use this command, yet.\r\n");
        return false;
      }
      if (char_get_admlevel(ch) < 10) {
        send_to_char(g, ch, "NO.\r\n");
        return false;
      }
      break;
    case 46:
      SET_OR_REMOVE(char_get_act(vict), PLR_NODELETE);
      break;
    case 47:
      if ((i = search_block(val_arg, genders, false)) < 0) {
        send_to_char(g, ch, "Must be 'male', 'female', or 'neutral'.\r\n");
        return false;
      }
      char_set_sex(vict, i);
      break;
    case 48:                            /* set age */
      if (value < 2 || value > 20000) { /* Arbitrary limits. */
        send_to_char(g, ch, "Ages 2 to 20000 accepted.\r\n");
        return false;
      }
      /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
      char_set_time_birth(vict, game_now(g) - (value * SECS_PER_MUD_YEAR));
      break;

    case 49: /* Blame/Thank Rick Glover. :) */
      char_set_height(vict, value);
      affect_total(g, vict);
      break;

    case 50:
      char_set_weight(vict, value);
      affect_total(g, vict);
      break;

    case 51:
      if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions"))
        char_set_olc_zone(g, vict, AEDIT_PERMISSION);
      else if (is_abbrev(val_arg, "hedit"))
        char_set_olc_zone(g, vict, HEDIT_PERMISSION);
      else if (is_abbrev(val_arg, "off"))
        char_set_olc_zone(g, vict, NOWHERE);
      else if (!is_number(val_arg)) {
        send_to_char(g, ch, "Value must be either 'socials', 'actions', 'hedit', 'off' or a zone number.\r\n");
        return false;
      } else
        char_set_olc_zone(g, vict, atoi(val_arg));
      break;

    case 52:
      if (char_is_npc(vict)) {
        chosen_race = race_find(val_arg, char_get_sex(ch), false);
      } else {
        chosen_race = race_find(val_arg, char_get_sex(ch), true);
      }
      if (chosen_race == RACE_UNDEFINED) {
        send_to_char(g, ch, "That is not a valid race for them.\r\n");
        return false;
      }
      char_set_race(vict, chosen_race);
      racial_body_parts(vict);
      break;

    case 53:
      char_set_ability_trains(g, vict, RANGE(0, 500));
      break;

    case 54:
      char_set_feat_points(g, vict, RANGE(0, 500));
      break;

    case 55:
      affect_total(g, vict);
      break;

    case 56:
      affect_total(g, vict);
      break;

    case 57:
      affect_total(g, vict);
      break;

    case 58:
      if (char_get_admlevel(vict) >= char_get_admlevel(ch) && vict != ch) {
        send_to_char(g, ch, "Permission denied.\r\n");
        return false;
      }
      if (value < ADMLVL_NONE || value > char_get_admlevel(ch)) {
        send_to_char(g, ch, "You can't set it to that.\r\n");
        return false;
      }
      if (char_get_admlevel(vict) == value)
        return true;
      admin_set(g, vict, value);
      break;

    case 59:
      if (value < 0 || value >= 6) {
        send_to_char(g, ch, "You can't set it to that.\r\n");
        return false;
      }
      char_set_hairl(vict, value);
      return true;
      break;

    case 60:
      if (value < 0 || value >= 13) {
        send_to_char(g, ch, "You can't set it to that.\r\n");
        return false;
      }
      char_set_hairs(vict, value);
      return true;
      break;

    case 61:
      if (value < 0 || value >= 15) {
        send_to_char(g, ch, "You can't set it to that.\r\n");
        return false;
      }
      char_set_hairc(vict, value);
      return true;
      break;

    case 62:
      if (value < 0 || value >= 12) {
        send_to_char(g, ch, "You can't set it to that.\r\n");
        return false;
      }
      char_set_skin(vict, value);
      return true;
      break;

    case 63:
      if (value < 0 || value >= 13) {
        send_to_char(g, ch, "You can't set it to that.\r\n");
        return false;
      }
      char_set_eye(vict, value);
      return true;
      break;

    case 64:
      char_set_basepl(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set basepl for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set basepl for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 65:
      char_set_baseki(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set baseki for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set baseki for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 66:
      char_set_basest(vict, value);
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set basest for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set basest for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 67:
      char_set_droom(vict, RANGE(0, 20000));
      break;

    case 68:
      char_set_absorbs(vict, RANGE(0, 3));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set absorbs for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set absorbs for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 69:
      char_set_upgrade(vict, char_get_upgrade(vict) + (RANGE(1, 1000)));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set upgrade points for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set upgrade points for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 70:
      char_set_aura(vict, RANGE(0, 8));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set aura for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set aura for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 71:
      send_to_char(g, ch, "Use the reward command.\r\n");
      break;
    case 72:
      char_set_boosts(vict, RANGE(-1000, 1000));
      break;
    case 73:
      SET_OR_REMOVE(char_get_act(vict), PLR_MULTP);
      break;
    case 74:
      char_set_dcount(vict, RANGE(-1000, 1000));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set death count for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set death count for %s.", char_get_name(ch), char_get_name(vict));
      break;
    case 75:
      send_to_char(g, ch, "No.");
      break;
    case 76:
      if (char_get_desc(vict) != nullptr) {
        star_phase(g, vict, RANGE(0, 2));
      } else {
        send_to_char(g, ch, "They aren't even in the game!\r\n");
      }
      break;
    case 77:
      char_set_racial_pref(g, vict, RANGE(1, 3));
      break;

    case 78:
      char_set_skill_slots(vict, RANGE(1, 1000));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set skill slots for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set skill slots for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 79:
      char_set_feature(vict, "");
      break;

    case 80:
      char_set_transclass(vict, RANGE(1, 3));
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: %s has set transformation class for %s.", char_get_name(ch), char_get_name(vict));
      log_imm_action(g, "SET: %s has set transformation class for %s.", char_get_name(ch), char_get_name(vict));
      break;

    case 81:
      char_set_clones(vict, RANGE(1, 3));
      send_to_char(g, ch, "Done.\r\n");
      break;

    case 82:
      char_set_armor(vict, RANGE(0, 1000000));
      send_to_char(g, ch, "Done.\r\n");
      break;

    default:
      send_to_char(g, ch, "Can't set that!\r\n");
      return false;
  }

  return true;
}

ACMD(do_set)
{
  struct char_data *vict = nullptr, *cbuf = nullptr;
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
    send_to_char(g, ch, "Usage: set <victim> <field> <value>\r\n");
    return;
  }

  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(g, ch, name, nullptr, FIND_CHAR_WORLD))) {
        send_to_char(g, ch, "There is no such player.\r\n");
        return;
      }
    } else { /* is_mob */
      if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_WORLD))) {
        send_to_char(g, ch, "There is no such creature.\r\n");
        return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    cbuf = new char_data();
    clear_char(cbuf);
    *char_player_specials_slot(cbuf) = new player_special_data();
    if ((player_i = load_char(g, name, cbuf)) > -1) {
      if (char_get_admlevel(cbuf) >= char_get_admlevel(ch)) {
        free_char(g, cbuf);
        send_to_char(g, ch, "Sorry, you can't do that.\r\n");
        return;
      }
      vict = cbuf;
    } else {
      free_char(g, cbuf);
      send_to_char(g, ch, "There is no such player.\r\n");
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
  retval = perform_set(g, ch, vict, mode, buf);

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !char_is_npc(vict))
      save_char(g, vict);
    if (is_file) {
      char_set_pfilepos(cbuf, player_i);
      save_char(g, cbuf);
      send_to_char(g, ch, "Saved in file.\r\n");
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(g, cbuf);
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
    send_to_char(g, ch, "Syntax: chown <object> <character>.\r\n");
  else if (!(victim = get_char_vis(g, ch, buf3, nullptr, FIND_CHAR_WORLD)))
    send_to_char(g, ch, "No one by that name here.\r\n");
  else if (victim == ch)
    send_to_char(g, ch, "Are you sure you're feeling ok?\r\n");
  else if (char_get_level(victim) >= char_get_level(ch))
    send_to_char(g, ch, "That's really not such a good idea.\r\n");
  else if (!*buf3)
    send_to_char(g, ch, "Syntax: chown <object> <character>.\r\n");
  else {
    for (i = 0; i < NUM_WEARS; i++) {
      if (char_get_equipment(victim, i) && char_can_see_obj(g, ch, char_get_equipment(victim, i)) &&
          isname(buf2, obj_get_name(char_get_equipment(victim, i)))) {
        obj_to_char(g, unequip_char(g, victim, i), victim);
        k = 1;
      }
    }

    if (!(obj = get_obj_in_list_vis(g, victim, buf2, nullptr, char_get_inventory(victim)))) {
      if (!k && !(obj = get_obj_in_list_vis(g, victim, buf2, nullptr, char_get_inventory(victim)))) {
        send_to_char(g, ch, "%s does not appear to have the %s.\r\n", char_get_name(victim), buf2);
        return;
      }
    }

    act(g, "@n$n makes a magical gesture and $p@n flies from $N to $m.", false, ch, obj, victim, TO_NOTVICT);
    act(g, "@n$n makes a magical gesture and $p@n flies away from you to $m.", false, ch, obj, victim, TO_VICT);
    act(g, "@nYou make a magical gesture and $p@n flies away from $N to you.", false, ch, obj, victim, TO_CHAR);

    obj_from_char(g, obj);
    obj_to_char(g, obj, ch);
    save_char(g, ch);
    save_char(g, victim);
  }
}
