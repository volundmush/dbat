/* ************************************************************************
*   File: wizard/players.c                                  Part of CircleMUD *
*  Usage: God commands that change players                                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iterator>

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
  void print_lockout(GameWorld *g, struct char_data *ch)
  {
    if (char_is_npc(ch))
      return;

    char fname[40], filler[50] = "", line[256] = "";
    FILE *file;
    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout") || !(file = fopen(fname, "r"))) {
      send_to_char(g, ch, "The lockout file does not exist.");
      return;
    }
    /* Two names to a line, each once */
    std::string buf = "@b------------------[ @RLOCKOUT @b]------------------@n\n";
    bool first = true, odd = false;
    while (!feof(file)) {
      get_line(file, line);
      sscanf(line, "%49s\n", filler);
      if (first || !strstr(buf.c_str(), filler)) {
        if (odd && !first)
          buf += str_format("{:<23}\n", filler);
        else
          buf += str_format("{:<23}@D|@n", filler);
        odd   = first || !odd;
        first = false;
      }
      *filler = '\0';
    }
    if (odd)
      buf += "\n";
    buf += "@b------------------[ @RLOCKOUT @b]------------------@n\n";
    send_to_char(g, ch, buf);
    fclose(file);
  }

  /* Locks name out, or lets them back in if they were */
  void lockWrite(GameWorld *g, struct char_data *ch, const char *name)
  {
    char fname[40], filler[50] = "", line[256] = "";
    std::string capped = name;
    CAP(capped);

    FILE *file;
    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout") || !(file = fopen(fname, "r"))) {
      send_to_char(g, ch, "The lockout file does not exist.");
      return;
    }
    /* At least 498 entries are read, the last name standing in for those past the end, as it always has */
    std::vector<std::string> names;
    while (!feof(file) || names.size() < 498) {
      get_line(file, line);
      sscanf(line, "%49s\n", filler);
      names.emplace_back(filler);
      *filler = '\0';
    }
    fclose(file);

    struct atomic_file af;
    FILE *fl;
    if (!(fl = atomic_open(&af, fname, false))) {
      mud_log("ERROR: could not save Lockout File, {}.", fname);
      return;
    }
    bool found = false;
    for (size_t x = 0; x < names.size(); x++) {
      if (x > 0 && !strcasecmp(names[x - 1].c_str(), names[x].c_str()))
        continue;
      if (strcasecmp(names[x].c_str(), capped.c_str()))
        file_print(fl, "{}\n", CAP(names[x]));
      else
        found = true;
    }
    if (!found) {
      file_print(fl, "{}\n", capped);
      send_to_all(g, "@rLOCKOUT@D: @WThe character, @C{}@W, was locked out of the MUD by @c{}@W.@n\r\n", capped, char_get_name(ch));
    } else {
      send_to_all(g, "@rLOCKOUT@D: @WThe character, @C{}@W, has had lockout removed by @c{}@W.@n\r\n", capped, char_get_name(ch));
    }
    mud_log("LOCKOUT: {} sentenced by {}.", capped, char_get_name(ch));
    log_imm_action(g, "LOCKOUT: {} sentenced by {}.", capped, char_get_name(ch));
    atomic_commit(&af);
  }

  void perform_immort_invis(GameWorld *g, struct char_data *ch, int level)
  {
    for (char_data *tch : room_people(g, char_get_in_room(ch))) {
      if (tch == ch)
        continue;
      if (char_get_admlevel(tch) >= char_get_invis_level(g, ch) && char_get_admlevel(tch) < level)
        act(g, "You blink and suddenly realize that $n is gone.", false, ch, nullptr, tch, TO_VICT);
      if (char_get_admlevel(tch) < char_get_invis_level(g, ch) && char_get_admlevel(tch) >= level)
        act(g, "You suddenly realize that $n is standing beside you.", false, ch, nullptr, tch, TO_VICT);
    }
    char_set_invis_level(g, ch, level);
    send_to_char(g, ch, "Your invisibility level is {:d}.\r\n", level);
  }

  /* The senior immortals, whom no one can lock out */
  constexpr const char *SENIOR_IMMORTALS[] = {"Iovan", "Fahl", "Xyron", "Samael"};
} // namespace

ACMD(do_approve)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "What player do you want to approve as having an acceptable bio?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_WORLD, "That player is not in the game.\r\n");
  if (reject(g, ch, found))
    return;
  if (char_plr_flagged(*found, PLR_BIOGR)) {
    send_to_char(g, ch, "They have already been approved. If this was made in error inform Iovan.\r\n");
    return;
  }
  char_plr_set(*found, PLR_BIOGR, true);
  send_to_char(g, ch, "They have now been approved.\r\n");
}

ACMD(do_reward)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: reward (target) (amount)\r\nThis is either a positive number or a negative.\r\n");
    return;
  }
  if (!readUserIndex(arg)) {
    send_to_char(g, ch, "That is not a recognised user file.\r\n");
    return;
  }
  int amt = parse_int(arg2);
  if (amt == 0) {
    send_to_char(g, ch, "That is pointless don't you think? Try an amount higher than 0.\r\n");
    return;
  }

  struct char_data *vict = nullptr; /* the last of the user's characters playing */
  for (descriptor_data *k : each_desc(g))
    if (desc_get_connected(k) == CON_PLAYING && !char_is_npc(k->character) && str_eq_nocase(k->user, arg))
      vict = k->character;

  if (!vict) { /* offline: the user file takes it */
    if (amt > 0) {
      send_to_char(g, ch, "@WYou award user @C{} @D(@G{:d}@D)@W RP points.@n\r\n", arg, amt);
      send_to_imm(g, "ROLEPLAY: User {} has been awarded {:d} RP points by {}.", arg, amt, char_get_name(ch));
    } else {
      send_to_char(g, ch, "@WYou deduct @D(@G{:d}@D)@W RP points from user @C{}@W.@n\r\n", amt, arg);
      send_to_imm(g, "ROLEPLAY: User {} has had {:d} RP points deducted by {}.", arg, amt, char_get_name(ch));
    }
    log_imm_action(g, "ROLEPLAY: {} has been awarded {:d} RP points by {}.", arg, amt, char_get_name(ch)); /* quirk: says awarded for a deduction too */
    userWrite(g, nullptr, 0, amt, 0, arg.c_str());
    return;
  }

  if (amt > 0) {
    send_to_char(g, ch, "@WYou award @C{} @D(@G{:d}@D)@W RP points.@n\r\n", char_get_name(vict), amt);
    send_to_char(g, vict, "@D[@YROLEPLAY@D] @WYou have been awarded @D(@G{:d}@D)@W RP points by @C{}@W.@n\r\n", amt, char_get_name(ch));
    send_to_imm(g, "ROLEPLAY: {} has been awarded {:d} RP points by {}.", arg, amt, char_get_name(ch));
    log_imm_action(g, "ROLEPLAY: {} has been awarded {:d} RP points by {}.", arg, amt, char_get_name(ch));
  } else {
    send_to_char(g, ch, "@WYou deduct @D(@G{:d}@D)@W RP points from @C{}@W.@n\r\n", amt, char_get_name(vict));
    send_to_char(g, vict, "@D[@YROLEPLAY@D] @C{}@W deducts @D(@G{:d}@D)@W RP points from you.@n\r\n", char_get_name(ch), amt);
    send_to_imm(g, "ROLEPLAY: {} has had {:d} RP points deducted by {}.", char_get_name(vict), amt, char_get_name(ch));
    log_imm_action(g, "ROLEPLAY: {} has had {:d} RP points deducted by {}.", char_get_name(vict), amt, char_get_name(ch));
  }
  char_set_rp(vict, char_get_rp(vict) + amt);
  if (amt > 0 && amt <= 29)
    char_set_trp(vict, char_get_trp(vict) + amt);
  char_get_desc(vict)->rpp += amt;
  userWrite(g, char_get_desc(vict), 0, 0, 0, "index");
}

ACMD(do_permission)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (str_eq_nocase("restrict", arg)) {
    send_to_char(g, ch, "You have restricted character creation to standard race slection.\r\n");
    send_to_imm(g, "PERMISSION: {} restricted races again.", char_get_name(ch));
    g->MAJIN_ALLOWED = false; /* quirk: saiyans stay allowed */
    return;
  }
  if (!str_eq_nocase("unrestrict", arg)) {
    send_to_char(g, ch, "You want to @Grestrict@n or @Gunrestrict@n?\r\n");
    return;
  }
  if (str_eq_nocase("saiyan", arg2)) {
    send_to_char(g, ch, "You have unrestricted saiyans for the very next character creation.\r\n");
    send_to_imm(g, "PERMISSION: {} unrestricted saiyans.", char_get_name(ch));
    g->SAIYAN_ALLOWED = true;
  } else if (str_eq_nocase("majin", arg2)) {
    send_to_char(g, ch, "You have unrestricted majins for the very next character creation.\r\n");
    send_to_imm(g, "PERMISSION: {} unrestricted majins.", char_get_name(ch));
    g->MAJIN_ALLOWED = true;
  } else {
    send_to_char(g, ch, "You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
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
    send_to_char(g, ch, "INTEREST TIME: [{}]\r\n", tmstr);
    return;
  }
  send_to_char(g, ch, "Interest time has been initiated!\r\n");
  g->INTERESTTIME = game_now(g) + 86400;
  g->LASTINTEREST = game_now(g) + 86400;
  return;
}

ACMD(do_hell)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: lockout (character)\n        lockout list\r\n");
    return;
  }
  if (std::ranges::any_of(SENIOR_IMMORTALS, [&](const char *name) { return str_eq_nocase(arg, name); })) {
    send_to_char(g, ch, "What are you smoking? You can't lockout senior imms.\r\n");
    return;
  }
  if (str_eq_nocase(arg, "list")) {
    print_lockout(g, ch);
    return;
  }

  struct char_data *vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD);
  if (!vict) {
    lockWrite(g, ch, arg.c_str());
    return;
  }
  struct descriptor_data *d = char_get_desc(vict);
  Crash_rentsave(g, vict, 0);
  extract_char(g, vict);
  lockWrite(g, ch, char_get_name(vict));
  if (d && desc_get_connected(d) != CON_PLAYING) {
    desc_set_connected(d, CON_CLOSE);
    d->character = nullptr;
    char_set_desc(vict, nullptr);
  }
}

ACMD(do_advance)
{
  struct char_data *victim;
  std::string name, level;
  int newlevel, oldlevel;

  std::string_view rest = argument;
  name                  = take_arg(rest);
  level                 = take_arg(rest);

  if (!name.empty()) {
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
  if (level.empty()) {
    send_to_char(g, ch, "[ 1 - 100 | demote ]\r\n");
    return;
  }
  if ((newlevel = parse_int(level)) <= 0) {
    if (str_eq_nocase("demote", level)) {
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

  send_to_char(g, ch, CONFIG_OK);

  if (newlevel < oldlevel)
    mud_log("(GC) {} demoted {} from level {:d} to {:d}.",
            char_get_name(ch), char_get_name(victim), oldlevel, newlevel);
  else
    mud_log("(GC) {} has advanced {} to level {:d} (from {:d})",
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
  for (descriptor_data *j : each_desc(g))
    if (desc_is_playing(j) && ch != j->character && char_get_admlevel(j->character) <= 0 && !char_is_npc(j->character))
      char_set_practice_points(g, j->character, char_get_practice_points(g, j->character) + 10);

  send_to_all(g, "@g{}@G hands out 10 practice sessions to everyone!@n\r\n", char_get_name(ch));
  mud_log("{} gave a handout of 10 PS to everyone.", char_get_name(ch));
  log_imm_action(g, "HANDOUT: {} has handed out 10 PS to everyone.", char_get_name(ch));
}

ACMD(do_restore)
{
  std::string_view rest = argument;
  std::string buf       = take_arg(rest);

  if (buf.empty()) {
    send_to_char(g, ch, "Whom do you wish to restore?\r\n");
    return;
  }
  if (is_abbrev(buf, "all")) {
    send_to_imm(g, "[Log: {} restored all.]", char_get_name(ch));
    log_imm_action(g, "RESTORE: {} has restored all players.", char_get_name(ch));
    for (descriptor_data *j : each_desc(g))
      if (desc_is_playing(j) && j->character)
        restore_by(g, j->character, ch);
    send_to_char(g, ch, "Okay.\r\n");
    return;
  }
  auto found = find_char(g, ch, buf, FIND_CHAR_WORLD, CONFIG_NOPERSON);
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (!char_is_npc(vict) && ch != vict && char_get_admlevel(vict) >= char_get_admlevel(ch)) {
    send_to_char(g, ch, "They don't need your help.\r\n");
    return;
  }
  restore_by(g, vict, ch);
  send_to_char(g, ch, CONFIG_OK);
  send_to_imm(g, "[Log: {} restored {}.]", char_get_name(ch), char_get_name(vict));
  log_imm_action(g, "RESTORE: {} has restored {}.", char_get_name(ch), char_get_name(vict));
}

void perform_immort_vis(GameWorld *g, struct char_data *ch)
{
  char_set_invis_level(g, ch, 0);
}

ACMD(do_invis)
{
  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You can't do that!\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    if (char_get_invis_level(g, ch) > 0)
      perform_immort_vis(g, ch);
    else
      perform_immort_invis(g, ch, char_get_admlevel(ch));
    return;
  }
  int level = parse_int(arg);
  if (level > char_get_admlevel(ch))
    send_to_char(g, ch, "You can't go invisible above your own level.\r\n");
  else if (level < 1)
    perform_immort_vis(g, ch);
  else
    perform_immort_invis(g, ch, level);
}

namespace {
  int wiz_log_level(GameWorld *g, struct char_data *ch)
  {
    return MAX(ADMLVL_GOD, char_get_invis_level(g, ch));
  }

  /* What each of the wizutil commands does to vict; false if nothing was done, and nothing is to be saved */
  bool reroll(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    send_to_char(g, ch, "Rerolling is not possible at this time, bug Iovan about it...\r\n");
    mud_log("(GC) {} has rerolled {}.", char_get_name(ch), char_get_name(vict));
    send_to_char(g, ch, "New stats: Str {:d}, Int {:d}, Wis {:d}, Dex {:d}, Con {:d}, Cha {:d}\r\n", char_get_aff_abils_str(vict), char_get_aff_abils_intel(vict),
                 char_get_aff_abils_wis(vict), char_get_aff_abils_dex(vict), char_get_aff_abils_con(vict), char_get_aff_abils_cha(vict));
    return true;
  }

  bool pardon(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    if (!char_plr_flagged(vict, PLR_THIEF) && !char_plr_flagged(vict, PLR_KILLER)) {
      send_to_char(g, ch, "Your victim is not flagged.\r\n");
      return false;
    }
    char_plr_set(vict, PLR_THIEF, false);
    char_plr_set(vict, PLR_KILLER, false);
    send_to_char(g, ch, "Pardoned.\r\n");
    send_to_char(g, vict, "You have been pardoned by the Gods!\r\n");
    mudlog(g, LOG_BRF, wiz_log_level(g, ch), true, "(GC) {} pardoned by {}", char_get_name(vict), char_get_name(ch));
    return true;
  }

  bool notitle(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    long result = char_plr_toggle(vict, PLR_NOTITLE);
    mudlog(g, LOG_NRM, wiz_log_level(g, ch), true, "(GC) Notitle {} for {} by {}.", ONOFF(result), char_get_name(vict), char_get_name(ch));
    send_to_char(g, ch, "(GC) Notitle {} for {} by {}.\r\n", ONOFF(result), char_get_name(vict), char_get_name(ch));
    return true;
  }

  bool squelch(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    long result = char_plr_toggle(vict, PLR_NOSHOUT);
    mudlog(g, LOG_BRF, wiz_log_level(g, ch), true, "(GC) Squelch {} for {} by {}.", ONOFF(result), char_get_name(vict), char_get_name(ch));
    send_to_char(g, ch, "(GC) Mute turned {} for {} by {}.\r\n", ONOFF(result), char_get_name(vict), char_get_name(ch));
    send_to_all(g, "@D[@RMUTE@D] @C{}@W has had mute turned @r{}@W by @C{}@W.\r\n", char_get_name(vict), ONOFF(result), char_get_name(ch));
    return true;
  }

  bool freeze(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    const char *refusal = nullptr;
    if (ch == vict)
      refusal = "Oh, yeah, THAT'S real smart...\r\n";
    else if (char_get_admlevel(ch) <= char_get_admlevel(vict))
      refusal = "Pfft...\r\n";
    else if (char_plr_flagged(vict, PLR_FROZEN))
      refusal = "Your victim is already pretty cold.\r\n";
    if (refusal) {
      send_to_char(g, ch, refusal);
      return false;
    }
    char_plr_set(vict, PLR_FROZEN, true);
    char_set_freeze_level(g, vict, char_get_admlevel(ch));
    send_to_char(g, vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
    send_to_char(g, ch, "Frozen.\r\n");
    act(g, "A sudden cold wind conjured from nowhere freezes $n!", false, vict, nullptr, nullptr, TO_ROOM);
    mudlog(g, LOG_BRF, wiz_log_level(g, ch), true, "(GC) {} frozen by {}.", char_get_name(vict), char_get_name(ch));
    return true;
  }

  bool thaw(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    if (!char_plr_flagged(vict, PLR_FROZEN)) {
      send_to_char(g, ch, "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
      return false;
    }
    if (char_get_freeze_level(g, vict) > char_get_admlevel(ch)) {
      send_to_char(g, ch, "Sorry, a level {:d} God froze {}... you can't unfreeze {}.\r\n", char_get_freeze_level(g, vict), char_get_name(vict), char_hmhr(vict));
      return false;
    }
    mudlog(g, LOG_BRF, wiz_log_level(g, ch), true, "(GC) {} un-frozen by {}.", char_get_name(vict), char_get_name(ch));
    char_plr_set(vict, PLR_FROZEN, false);
    send_to_char(g, vict, "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
    send_to_char(g, ch, "Thawed.\r\n");
    act(g, "A sudden fireball conjured from nowhere thaws $n!", false, vict, nullptr, nullptr, TO_ROOM);
    return true;
  }

  bool unaffect(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    if (char_affects(vict).empty() && !char_get_affected_by(vict)) {
      send_to_char(g, ch, "Your victim does not have any affections!\r\n");
      return false;
    }
    while (!char_affects(vict).empty())
      affect_remove(g, vict, &char_affects(vict).front());
    for (int i = 0; i < AF_ARRAY_MAX; i++)
      char_get_affected_by(vict)[i] = 0;
    send_to_char(g, vict, "There is a brief flash of light!\r\nYou feel slightly different.\r\n");
    send_to_char(g, ch, "All spells removed.\r\n");
    return true;
  }

  struct wizutil {
    int subcmd;
    bool (*apply)(GameWorld *, struct char_data *, struct char_data *);
  };
  constexpr wizutil WIZUTILS[] = {
    {SCMD_REROLL, reroll}, {SCMD_PARDON, pardon}, {SCMD_NOTITLE, notitle}, {SCMD_SQUELCH, squelch},
    {SCMD_FREEZE, freeze}, {SCMD_THAW, thaw},     {SCMD_UNAFFECT, unaffect},
  };
} // namespace

/* General fn for wizcommands of the sort: cmd <player> */
ACMD(do_wizutil)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Yes, but for whom?!?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_WORLD, "There is no such player.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (char_is_npc(vict)) {
    send_to_char(g, ch, "You can't do that to a mob!\r\n");
    return;
  }
  if (char_get_admlevel(vict) > char_get_admlevel(ch)) {
    send_to_char(g, ch, "Hmmm...you'd better not.\r\n");
    return;
  }
  auto util = std::ranges::find(WIZUTILS, subcmd, &wizutil::subcmd);
  if (util == std::end(WIZUTILS))
    mud_log("SYSERR: Unknown subcmd {:d} passed to do_wizutil ({})", subcmd, __FILE__);
  else if (!util->apply(g, ch, vict))
    return;
  save_char(g, vict);
}

/***************** The do_set function ***********************************/

namespace {
  constexpr int PC   = 1;
  constexpr int NPC  = 2;
  constexpr int BOTH = 3;

  constexpr int MISC   = 0;
  constexpr int BINARY = 1;
  constexpr int NUMBER = 2;

  /* One set being done: who to whom, and the value as each kind of field reads it. */
  struct setting {
    GameWorld *g;
    struct char_data *ch;
    struct char_data *vict;
    const std::string &val; /* as typed */
    const char *field;
    int64_t value; /* a NUMBER field's */
    bool on;       /* a BINARY field's */

    int64_t clamp(int64_t lo, int64_t hi) { return value = std::max(lo, std::min(hi, value)); }

    bool says(const char *text, bool done)
    {
      send_to_char(g, ch, "{}", text);
      return done;
    }

    /* Tells the immortals and the log, then works out the victim's stats again if asked to. */
    bool logged(const char *what, bool totals = false)
    {
      mudlog(g, LOG_NRM, MAX(ADMLVL_GOD, char_get_invis_level(g, ch)), true, "SET: {} has set {} for {}.", char_get_name(ch), what, char_get_name(vict));
      log_imm_action(g, "SET: {} has set {} for {}.", char_get_name(ch), what, char_get_name(vict));
      if (totals)
        affect_total(g, vict);
      return true;
    }

    bool not_godly_enough()
    {
      if (char_get_admlevel(ch) >= ADMLVL_IMPL || ch == vict)
        return false;
      send_to_char(g, ch, "You aren't godly enough for that!\r\n");
      return true;
    }
  };

  void set_bit(bitvector_t *flags, int flag, bool on)
  {
    if (on)
      SET_BIT_AR(flags, flag);
    else
      REMOVE_BIT_AR(flags, flag);
  }

  template <int Flag>
  bool pref(setting &s)
  {
    set_bit(char_get_pref(s.g, s.vict), Flag, s.on);
    return true;
  }

  template <int Flag>
  bool act_flag(setting &s)
  {
    set_bit(char_get_act(s.vict), Flag, s.on);
    return true;
  }

  bool does_nothing(setting &s) { return s.says("This does nothing at the moment.\r\n", true); }

  /* Drunk, hunger and thirst: off, or 0 to 48. */
  template <int Condition>
  bool condition(setting &s)
  {
    if (str_eq_nocase(s.val, "off")) {
      char_set_conditions(s.g, s.vict, Condition, -1);
      send_to_char(s.g, s.ch, "{}'s {} now off.\r\n", char_get_name(s.vict), s.field);
      return true;
    }
    if (!is_number(s.val))
      return s.says("Must be 'off' or a value from 0 to 48.\r\n", false);
    s.value = parse_int(s.val);
    char_set_conditions(s.g, s.vict, Condition, s.clamp(0, 48));
    send_to_char(s.g, s.ch, "{}'s {} set to {:d}.\r\n", char_get_name(s.vict), s.field, s.value);
    return true;
  }

  /* Looks from 0 to below Count. */
  template <void (*Set)(struct char_data *, int8_t), int Count>
  bool look(setting &s)
  {
    if (s.value < 0 || s.value >= Count)
      return s.says("You can't set it to that.\r\n", false);
    Set(s.vict, s.value);
    return true;
  }

  bool affects_again(setting &s)
  {
    affect_total(s.g, s.vict);
    return true;
  }

  bool cant_set(setting &s) { return s.says("Can't set that!\r\n", false); }

  bool practices(setting &s)
  {
    if (!char_get_class_level(s.vict))
      return does_nothing(s); /* race practices (skill_points) no longer exist */
    char_set_practice_points(s.g, s.vict, s.clamp(0, 10000));
    return s.logged("PS");
  }

  struct set_field {
    const char *name;
    int level;
    int pcnpc;
    int type;
    bool (*apply)(setting &s); /* true if something changed, to be saved */
  };

  /* Matched exactly, then by prefix, in this order. */
  constexpr set_field SET_FIELDS[] = {
      {"brief", ADMLVL_GOD, PC, BINARY, pref<PRF_BRIEF>},
      {"invstart", ADMLVL_GOD, PC, BINARY, act_flag<PLR_INVSTART>},
      {"title", ADMLVL_GOD, PC, MISC, [](setting &s) {
         set_title(s.g, s.vict, s.val.c_str());
         send_to_char(s.g, s.ch, "{}'s title is now: {}\r\n", char_get_name(s.vict), char_get_title(s.vict));
         return true;
       }},
      {"nosummon", ADMLVL_GRGOD, PC, BINARY, [](setting &s) {
         pref<PRF_SUMMONABLE>(s);
         send_to_char(s.g, s.ch, "Nosummon {} for {}.\r\n", ONOFF(!s.on), char_get_name(s.vict));
         return true;
       }},
      {"maxpl", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_max_hit(s.vict, s.value); return s.logged("maxpl"); }},
      {"maxki", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_max_mana(s.vict, s.value); return s.logged("maxki"); }},
      {"maxst", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_max_move(s.vict, s.value); return s.logged("maxsta"); }},
      {"pl", ADMLVL_BUILDER, BOTH, NUMBER, does_nothing},  /* the legacy hit field it set no longer exists */
      {"ki", ADMLVL_BUILDER, BOTH, NUMBER, does_nothing},  /* the legacy mana field it set no longer exists */
      {"sta", ADMLVL_BUILDER, BOTH, NUMBER, does_nothing}, /* the legacy move field it set no longer exists */
      {"align", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_alignment(s.vict, s.clamp(-1000, 1000)); return s.logged("align", true); }},
      {"str", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_real_abils_str(s.vict, s.clamp(0, 100)); return s.logged("str", true); }},
      {"stradd", ADMLVL_IMPL, BOTH, NUMBER, [](setting &s) { return s.says("Setting str_add does nothing now.\r\n", true); }},
      {"int", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_real_abils_intel(s.vict, s.clamp(0, 100)); return s.logged("intel", true); }},
      {"wis", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_real_abils_wis(s.vict, s.clamp(0, 100)); return s.logged("wis", true); }},
      {"dex", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_real_abils_dex(s.vict, s.clamp(0, 100)); return s.logged("dex", true); }},
      {"con", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_real_abils_con(s.vict, s.clamp(0, 100)); return s.logged("con", true); }},
      {"cha", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_real_abils_cha(s.vict, s.clamp(0, 100)); return s.logged("speed", true); }},
      {"ac", ADMLVL_IMPL, BOTH, NUMBER, [](setting &s) { char_set_armor(s.vict, s.clamp(-100, 500)); return s.logged("armor index", true); }},
      {"zenni", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_gold(s.vict, s.clamp(0, 100000000)); return s.logged("zenni"); }},
      {"bank", ADMLVL_BUILDER, PC, NUMBER, [](setting &s) { char_set_bank_gold(s.vict, s.clamp(0, 100000000)); return s.logged("bank"); }},
      {"exp", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_exp(s.vict, s.clamp(0, 50000000)); return s.logged("exp"); }},
      {"accuracy", ADMLVL_VICE, BOTH, NUMBER, does_nothing},
      {"damage", ADMLVL_VICE, BOTH, NUMBER, does_nothing}, /* damage mod no longer exists */
      {"invis", ADMLVL_IMPL, PC, NUMBER, [](setting &s) {
         if (s.not_godly_enough())
           return false;
         char_set_invis_level(s.g, s.vict, s.clamp(0, char_get_admlevel(s.vict)));
         return true;
       }},
      {"nohassle", ADMLVL_VICE, PC, BINARY, [](setting &s) { return !s.not_godly_enough() && pref<PRF_NOHASSLE>(s); }},
      {"frozen", ADMLVL_FREEZE, PC, BINARY, [](setting &s) {
         if (s.ch == s.vict && s.on)
           return s.says("Better not -- could be a long winter!\r\n", false);
         return act_flag<PLR_FROZEN>(s);
       }},
      {"practices", ADMLVL_BUILDER, PC, NUMBER, practices},
      {"lessons", ADMLVL_IMPL, PC, NUMBER, practices},
      {"drunk", ADMLVL_GOD, BOTH, MISC, condition<0>},
      {"hunger", ADMLVL_BUILDER, BOTH, MISC, condition<1>},
      {"thirst", ADMLVL_BUILDER, BOTH, MISC, condition<2>},
      {"killer", ADMLVL_GOD, PC, BINARY, act_flag<PLR_KILLER>},
      {"thief", ADMLVL_GOD, PC, BINARY, act_flag<PLR_THIEF>},
      {"level", ADMLVL_GRGOD, BOTH, NUMBER, [](setting &s) {
         if (!char_is_npc(s.vict) && s.value > 100)
           return s.says("You can't do that.\r\n", false);
         char_set_class_level(s.vict, std::max<int64_t>(0, s.value));
         return true;
       }},
      {"room", ADMLVL_IMPL, BOTH, NUMBER, [](setting &s) {
         room_vnum rnum = s.value;
         if (!room_of(s.g, rnum))
           return s.says("No room exists with that number.\r\n", false);
         if (char_get_in_room(s.vict) != NOWHERE) /* Another Eric Green special. */
           char_from_room(s.g, s.vict);
         char_to_room(s.g, s.vict, rnum);
         return true;
       }},
      {"roomflag", ADMLVL_VICE, PC, BINARY, pref<PRF_ROOMFLAGS>},
      {"siteok", ADMLVL_VICE, PC, BINARY, act_flag<PLR_SITEOK>},
      {"deleted", ADMLVL_IMPL, PC, BINARY, act_flag<PLR_DELETED>},
      {"class", ADMLVL_VICE, BOTH, MISC, [](setting &s) {
         int sensei = sensei_find(s.val.c_str());
         if (sensei == CLASS_UNDEFINED)
           return s.says("That is not a class.\r\n", false);
         char_set_class(s.vict, sensei);
         return true;
       }},
      {"nowizlist", ADMLVL_GOD, PC, BINARY, act_flag<PLR_NOWIZLIST>},
      {"quest", ADMLVL_GOD, PC, BINARY, pref<PRF_QUEST>},
      {"loadroom", ADMLVL_VICE, PC, MISC, [](setting &s) {
         if (str_eq_nocase(s.val, "off")) {
           char_plr_set(s.vict, PLR_LOADROOM, false);
           char_set_load_room(s.g, s.vict, NOWHERE);
           return true;
         }
         if (!is_number(s.val))
           return s.says("Must be 'off' or a room's virtual number.\r\n", false);
         room_vnum room = parse_int(s.val);
         if (!room_of(s.g, room))
           return s.says("That room does not exist!\r\n", false);
         char_plr_set(s.vict, PLR_LOADROOM, true);
         char_set_load_room(s.g, s.vict, room);
         send_to_char(s.g, s.ch, "{} will enter at room #{:d}.\r\n", char_get_name(s.vict), char_get_load_room(s.g, s.vict));
         return true;
       }},
      {"color", ADMLVL_GOD, PC, BINARY, pref<PRF_COLOR>},
      {"idnum", ADMLVL_IMPL, PC, NUMBER, [](setting &s) {
         if (char_get_idnum(s.ch) == 0 || char_is_npc(s.vict))
           return false;
         char_set_idnum(s.vict, s.value);
         return true;
       }},
      {"passwd", ADMLVL_IMPL, PC, MISC, [](setting &s) {
         if (char_get_idnum(s.ch) > 1)
           return s.says("Please don't use this command, yet.\r\n", false);
         if (char_get_admlevel(s.ch) < 10)
           return s.says("NO.\r\n", false);
         return true;
       }},
      {"nodelete", ADMLVL_GOD, PC, BINARY, act_flag<PLR_NODELETE>},
      {"sex", ADMLVL_VICE, BOTH, MISC, [](setting &s) {
         int sex = search_block(s.val, genders, false);
         if (sex < 0)
           return s.says("Must be 'male', 'female', or 'neutral'.\r\n", false);
         char_set_sex(s.vict, sex);
         return true;
       }},
      {"age", ADMLVL_VICE, BOTH, NUMBER, [](setting &s) {
         if (s.value < 2 || s.value > 20000) /* Arbitrary limits. */
           return s.says("Ages 2 to 20000 accepted.\r\n", false);
         /* May not display the exact age specified, due to the integer division used elsewhere. -gg 5/27/98 */
         char_set_time_birth(s.vict, game_now(s.g) - (s.value * SECS_PER_MUD_YEAR));
         return true;
       }},
      {"height", ADMLVL_GOD, BOTH, NUMBER, [](setting &s) { char_set_height(s.vict, s.value); return affects_again(s); }},
      {"weight", ADMLVL_GOD, BOTH, NUMBER, [](setting &s) { char_set_weight(s.vict, s.value); return affects_again(s); }},
      {"olc", ADMLVL_GRGOD, PC, MISC, [](setting &s) {
         if (is_abbrev(s.val, "socials") || is_abbrev(s.val, "actions"))
           char_set_olc_zone(s.g, s.vict, AEDIT_PERMISSION);
         else if (is_abbrev(s.val, "hedit"))
           char_set_olc_zone(s.g, s.vict, HEDIT_PERMISSION);
         else if (is_abbrev(s.val, "off"))
           char_set_olc_zone(s.g, s.vict, NOWHERE);
         else if (!is_number(s.val))
           return s.says("Value must be either 'socials', 'actions', 'hedit', 'off' or a zone number.\r\n", false);
         else
           char_set_olc_zone(s.g, s.vict, parse_int(s.val));
         return true;
       }},
      {"race", ADMLVL_VICE, PC, MISC, [](setting &s) {
         /* quirk: by the setter's sex, not the victim's */
         int race = race_find(s.val.c_str(), char_get_sex(s.ch), !char_is_npc(s.vict));
         if (race == RACE_UNDEFINED)
           return s.says("That is not a valid race for them.\r\n", false);
         char_set_race(s.vict, race);
         racial_body_parts(s.vict);
         return true;
       }},
      {"trains", ADMLVL_VICE, PC, NUMBER, [](setting &s) { char_set_ability_trains(s.g, s.vict, s.clamp(0, 500)); return true; }},
      {"feats", ADMLVL_VICE, PC, NUMBER, [](setting &s) { char_set_feat_points(s.g, s.vict, s.clamp(0, 500)); return true; }},
      {"ethic", ADMLVL_GOD, BOTH, NUMBER, affects_again},
      {"unused1", ADMLVL_GRGOD, BOTH, NUMBER, affects_again},
      {"unused2", ADMLVL_GRGOD, BOTH, NUMBER, affects_again},
      {"adminlevel", ADMLVL_GRGOD, PC, NUMBER, [](setting &s) {
         if (char_get_admlevel(s.vict) >= char_get_admlevel(s.ch) && s.vict != s.ch)
           return s.says("Permission denied.\r\n", false);
         if (s.value < ADMLVL_NONE || s.value > char_get_admlevel(s.ch))
           return s.says("You can't set it to that.\r\n", false);
         if (char_get_admlevel(s.vict) != s.value)
           admin_set(s.g, s.vict, s.value);
         return true;
       }},
      {"hairl", ADMLVL_VICE, PC, NUMBER, look<char_set_hairl, 6>},
      {"hairs", ADMLVL_VICE, PC, NUMBER, look<char_set_hairs, 13>},
      {"hairc", ADMLVL_VICE, PC, NUMBER, look<char_set_hairc, 15>},
      {"skin", ADMLVL_VICE, PC, NUMBER, look<char_set_skin, 12>},
      {"eye", ADMLVL_VICE, PC, NUMBER, look<char_set_eye, 13>},
      {"basepl", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_basepl(s.vict, s.value); return s.logged("basepl"); }},
      {"baseki", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_baseki(s.vict, s.value); return s.logged("baseki"); }},
      {"basest", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_basest(s.vict, s.value); return s.logged("basest"); }},
      {"droom", ADMLVL_GRGOD, BOTH, NUMBER, [](setting &s) { char_set_droom(s.vict, s.clamp(0, 20000)); return true; }},
      {"absorbs", ADMLVL_GRGOD, BOTH, NUMBER, [](setting &s) { char_set_absorbs(s.vict, s.clamp(0, 3)); return s.logged("absorbs"); }},
      {"ugp", ADMLVL_GRGOD, BOTH, NUMBER, [](setting &s) { char_set_upgrade(s.vict, char_get_upgrade(s.vict) + s.clamp(1, 1000)); return s.logged("upgrade points"); }},
      {"aura", ADMLVL_IMMORT, BOTH, NUMBER, [](setting &s) { char_set_aura(s.vict, s.clamp(0, 8)); return s.logged("aura"); }},
      {"trp", ADMLVL_GRGOD, BOTH, NUMBER, [](setting &s) { return s.says("Use the reward command.\r\n", true); }},
      {"boost", ADMLVL_GRGOD, BOTH, NUMBER, [](setting &s) { char_set_boosts(s.vict, s.clamp(-1000, 1000)); return true; }},
      {"multi", ADMLVL_IMPL, PC, BINARY, act_flag<PLR_MULTP>},
      {"deaths", ADMLVL_BUILDER, BOTH, NUMBER, [](setting &s) { char_set_dcount(s.vict, s.clamp(-1000, 1000)); return s.logged("death count"); }},
      {"user", ADMLVL_IMPL, PC, MISC, [](setting &s) { return s.says("No.", true); }},
      {"phase", ADMLVL_IMMORT, PC, NUMBER, [](setting &s) {
         if (char_get_desc(s.vict) == nullptr)
           return s.says("They aren't even in the game!\r\n", true);
         star_phase(s.g, s.vict, s.clamp(0, 2));
         return true;
       }},
      {"racial", ADMLVL_IMMORT, PC, NUMBER, [](setting &s) { char_set_racial_pref(s.g, s.vict, s.clamp(1, 3)); return true; }},
      {"slots", ADMLVL_IMMORT, PC, NUMBER, [](setting &s) { char_set_skill_slots(s.vict, s.clamp(1, 1000)); return s.logged("skill slots"); }},
      {"feature", ADMLVL_IMMORT, PC, BINARY, [](setting &s) { char_set_feature(s.vict, ""); return true; }},
      {"tclass", ADMLVL_VICE, PC, NUMBER, [](setting &s) { char_set_transclass(s.vict, s.clamp(1, 3)); return s.logged("transformation class"); }},
      {"clones", ADMLVL_IMPL, PC, NUMBER, [](setting &s) { char_set_clones(s.vict, s.clamp(1, 3)); return s.says("Done.\r\n", true); }},
      {"armor", ADMLVL_IMPL, PC, NUMBER, [](setting &s) { char_set_armor(s.vict, s.clamp(0, 1000000)); return s.says("Done.\r\n", true); }},
  };

  /* What a field no one has heard of is taken as. */
  constexpr set_field UNKNOWN_FIELD = {"", 0, BOTH, MISC, cant_set};

  const set_field &find_set_field(const std::string &field)
  {
    auto it = std::ranges::find_if(SET_FIELDS, [&](const set_field &f) { return field == f.name; });
    if (it == std::end(SET_FIELDS))
      it = std::ranges::find_if(SET_FIELDS, [&](const set_field &f) { return std::string_view(f.name).starts_with(field); });
    return it == std::end(SET_FIELDS) ? UNKNOWN_FIELD : *it;
  }

  bool perform_set(GameWorld *g, struct char_data *ch, struct char_data *vict, const set_field &f, const std::string &val_arg)
  {
    /* Check to make sure all the levels are correct */
    if (char_get_admlevel(ch) != ADMLVL_IMPL && !char_is_npc(vict) && char_get_admlevel(ch) <= char_get_admlevel(vict) && vict != ch) {
      send_to_char(g, ch, "Maybe that's not such a great idea...\r\n");
      return false;
    }
    if (char_get_admlevel(ch) < f.level) {
      send_to_char(g, ch, "You are not godly enough for that!\r\n");
      return false;
    }

    /* Make sure the PC/NPC is correct */
    if (char_is_npc(vict) && !(f.pcnpc & NPC)) {
      send_to_char(g, ch, "You can't do that to a beast!\r\n");
      return false;
    }
    if (!char_is_npc(vict) && !(f.pcnpc & PC)) {
      send_to_char(g, ch, "That can only be done to a beast!\r\n");
      return false;
    }

    setting s{g, ch, vict, val_arg, f.name, 0, false};
    if (f.type == BINARY) {
      bool off = val_arg == "off" || val_arg == "no";
      s.on     = val_arg == "on" || val_arg == "yes";
      if (!s.on && !off) {
        send_to_char(g, ch, "Value must be 'on' or 'off'.\r\n");
        return false;
      }
      send_to_char(g, ch, "{} {} for {}.\r\n", f.name, ONOFF(s.on), char_get_name(vict));
    } else if (f.type == NUMBER) {
      s.value = strtoll(val_arg.c_str(), nullptr, 10);
      send_to_char(g, ch, "{}'s {} set to {:d}.\r\n", char_get_name(vict), f.name, s.value);
    } else {
      send_to_char(g, ch, CONFIG_OK);
    }

    return f.apply(s);
  }
} // namespace

ACMD(do_set)
{
  std::string_view rest = argument;
  std::string name      = take_word(rest);
  rest                  = skip_spaces(rest);
  bool is_file = false, is_player = false;

  if (name == "file") {
    is_file = true;
    name    = take_word(rest);
    rest    = skip_spaces(rest);
  } else if (str_eq_nocase(name, "player")) {
    is_player = true;
    name      = take_word(rest);
    rest      = skip_spaces(rest);
  } else if (str_eq_nocase(name, "mob")) {
    name = take_word(rest);
    rest = skip_spaces(rest);
  }

  std::string field = take_word(rest);
  std::string value(skip_spaces(rest)); /* the rest of the line */

  if (name.empty() || field.empty()) {
    send_to_char(g, ch, "Usage: set <victim> <field> <value>\r\n");
    return;
  }

  /* find the target */
  struct char_data *vict = nullptr;
  int player_i           = 0;
  if (is_player) {
    if (!(vict = get_player_vis(g, ch, name, nullptr, FIND_CHAR_WORLD))) {
      send_to_char(g, ch, "There is no such player.\r\n");
      return;
    }
  } else if (!is_file) {
    if (!(vict = get_char_vis(g, ch, name, nullptr, FIND_CHAR_WORLD))) {
      send_to_char(g, ch, "There is no such creature.\r\n");
      return;
    }
  } else {
    /* try to load the player off disk */
    vict = new char_data();
    clear_char(vict);
    char_make_player_specials(vict);
    if ((player_i = load_char(g, name.c_str(), vict)) < 0) {
      free_char(g, vict);
      send_to_char(g, ch, "There is no such player.\r\n");
      return;
    }
    if (char_get_admlevel(vict) >= char_get_admlevel(ch)) {
      free_char(g, vict);
      send_to_char(g, ch, "Sorry, you can't do that.\r\n");
      return;
    }
  }

  /* save the character if a change was made */
  if (perform_set(g, ch, vict, find_set_field(field), value)) {
    if (is_file) {
      char_set_pfilepos(vict, player_i);
      save_char(g, vict);
      send_to_char(g, ch, "Saved in file.\r\n");
    } else if (!char_is_npc(vict)) {
      save_char(g, vict);
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(g, vict);
}

ACMD(do_chown)
{
  std::string_view rest = argument;
  std::string buf2      = take_arg(rest);
  std::string buf3      = take_arg(rest);

  if (buf2.empty()) {
    send_to_char(g, ch, "Syntax: chown <object> <character>.\r\n");
    return;
  }
  auto found = find_char(g, ch, buf3, FIND_CHAR_WORLD, "No one by that name here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *victim = *found;
  const char *refusal      = nullptr;
  if (victim == ch)
    refusal = "Are you sure you're feeling ok?\r\n";
  else if (char_get_level(victim) >= char_get_level(ch))
    refusal = "That's really not such a good idea.\r\n";
  else if (buf3.empty())
    refusal = "Syntax: chown <object> <character>.\r\n";
  if (refusal) {
    send_to_char(g, ch, refusal);
    return;
  }

  /* What they wear by that name comes off first */
  bool taken_off = false;
  for (int i = 0; i < NUM_WEARS; i++) {
    struct obj_data *worn = char_get_equipment(victim, i);
    if (worn && char_can_see_obj(g, ch, worn) && isname(buf2, obj_get_name(worn))) {
      obj_to_char(g, unequip_char(g, victim, i), victim);
      taken_off = true;
    }
  }
  /* then it's looked for, as the victim sees, among what they carry */
  struct obj_data *obj = get_obj_in_list_vis(g, victim, buf2, nullptr, char_get_inventory(victim));
  if (!obj && !taken_off) {
    send_to_char(g, ch, "{} does not appear to have the {}.\r\n", char_get_name(victim), buf2);
    return;
  } /* quirk: if something came off but can't be found, the gestures are made with nothing */

  act(g, "@n$n makes a magical gesture and $p@n flies from $N to $m.", false, ch, obj, victim, TO_NOTVICT);
  act(g, "@n$n makes a magical gesture and $p@n flies away from you to $m.", false, ch, obj, victim, TO_VICT);
  act(g, "@nYou make a magical gesture and $p@n flies away from $N to you.", false, ch, obj, victim, TO_CHAR);
  obj_from_char(g, obj);
  obj_to_char(g, obj, ch);
  save_char(g, ch);
  save_char(g, victim);
}
