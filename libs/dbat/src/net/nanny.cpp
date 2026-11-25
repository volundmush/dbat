/* ************************************************************************
*   File: nanny.c                                       Part of CircleMUD *
*  Usage: connection state machine: login, account menu, game menu        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>


#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/boards.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/config.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/objsave.hpp"
#include "dbat/ban.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/mail.hpp"
#include "dbat/alias.hpp"
#include "dbat/players.hpp"
#include "dbat/assedit.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/aedit.hpp"
#include "dbat/hedit.hpp"
#include "dbat/hsedit.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/reader.hpp"

#include "nanny_internal.hpp"

static void payout(GameWorld *g, int num);

/* Recounts the players online (above level 3) and updates the all-time record.
   Returns the record as it stood before the count raised it. */
static int update_player_count(GameWorld *g)
{
  int count = 0, oldcount = g->HIGHPCOUNT;

  for (descriptor_data *kd : each_desc(g)) {
    if (!kd->character) {
      continue;
    }
    if (!char_is_npc(kd->character) && char_get_level(kd->character) > 3) {
      count += 1;
    }

    if (count > g->PCOUNT) {
      g->PCOUNT = count;
    }

    if (g->PCOUNT >= g->HIGHPCOUNT) {
      oldcount   = g->HIGHPCOUNT;
      g->HIGHPCOUNT = g->PCOUNT;
      g->PCOUNTDATE = game_now(g);
    }
  }
  return oldcount;
}

/* RPP rewards for a player count near, at, or above the record. */
static void pay_player_count_rewards(GameWorld *g, int oldcount)
{
  if (g->PCOUNT < g->HIGHPCOUNT && g->PCOUNT >= g->HIGHPCOUNT - 4) {
    payout(g, 0);
  }
  if (g->PCOUNT == g->HIGHPCOUNT) {
    payout(g, 1);
  }
  if (g->PCOUNT > oldcount) {
    payout(g, 2);
  }
}

/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */

/* The name typed, leading spaces passed over, if it's one or more letters and nothing else */
static std::optional<std::string> parse_name(std::string_view typed)
{
  size_t start = 0;

  while (start < typed.size() && std::isspace(static_cast<unsigned char>(typed[start])))
    start++;
  typed.remove_prefix(start);
  if (typed.empty() || !std::ranges::all_of(typed, [](unsigned char c) { return std::isalpha(c); }))
    return std::nullopt;
  return std::string(typed);
}

/* ---- Notices at login ------------------------------------------------- */

/* Interest owed since the last payout, for one to three days. Reconnecting pays the bank's own rate; entering from the
   menu pays 2% (at most 7,500 a day), and says so even when that's nothing. */
static void pay_login_interest(GameWorld *g, struct char_data *ch, bool reconnecting) /* quirk: two rates */
{
  if (g->LASTINTEREST == 0 || g->LASTINTEREST <= char_get_lastint(ch))
    return;
  int diff = (g->LASTINTEREST - char_get_lastint(ch));
  int days = MAX(1, MIN(3, diff / 86400));
  char_set_lastint(ch, g->LASTINTEREST);

  int per_day;
  if (reconnecting) {
    if (!(per_day = char_get_bank_interest(ch)))
      return;
  } else {
    if (char_get_bank_gold(ch) <= 0)
      return;
    per_day = MIN(7500, (char_get_bank_gold(ch) / 100) * 2);
  }
  int total = per_day * days;
  char_set_bank_gold(ch, char_get_bank_gold(ch) + (total));
  send_to_char(g, ch, "Interest happened while you were away, {:d} times.\r\n@cBank Interest@D: @Y{}@n\r\n", days, add_commas(total));
}

/* Mail, the boards, the news and interest. */
static void login_notices(GameWorld *g, struct descriptor_data *d, bool reconnecting)
{
  if (has_mail(g, char_get_idnum(d->character))) {
    if (reconnecting)
      write_to_output(g, d, "You have mail waiting.\r\n");
    else
      send_to_char(g, d->character, "\r\nYou have mail waiting.\r\n");
  }
  board_login_notices(g, d->character);
  if (g->NEWSUPDATE > char_get_lastpl(d->character))
    send_to_char(g, d->character, "\r\n@GThe NEWS file has been updated, type 'news {:d}' to see the latest entry or 'news list' to see available entries.@n\r\n", g->LASTNEWS);
  pay_login_interest(g, d->character, reconnecting);
}

/* ---- Logging in to a character already in the game ------------------- */

/* How d takes over a body already in the game. */
enum class takeover { none, reconnect, usurp, unswitch };

/* Disconnects every other connection controlling the character d logs in as, and returns the body d should take, if
   any: an immortal's that was switched away from, or one in use. */
static char_data *close_other_connections(GameWorld *g, struct descriptor_data *d, takeover *how)
{
  struct char_data *target = nullptr;
  int id                   = char_get_idnum(d->character);

  for (descriptor_data *k : each_desc(g)) {
    if (k == d)
      continue;

    if (k->original && (char_get_idnum(k->original) == id)) {
      /* Original descriptor was switched, booting it and restoring normal body control. */
      write_to_output(g, d, "\r\nMultiple login detected -- disconnecting.\r\n");
      desc_set_connected(k, CON_CLOSE);
      if (!target) {
        target = k->original;
        *how   = takeover::unswitch;
      }
      if (k->character)
        char_set_desc(k->character, nullptr);
      k->character = nullptr;
      k->original  = nullptr;
    } else if (k->character && char_get_idnum(k->character) == id && k->original) {
      /* Character taking over their own body, while an immortal was switched to it. */
      do_return(g, k->character, "", 0, 0);
    } else if (k->character && char_get_idnum(k->character) == id) {
      /* Character taking over their own body. */
      if (!target && desc_get_connected(k) == CON_PLAYING) {
        write_to_output(g, k, "\r\nThis body has been usurped!\r\n");
        if (k->snoop_by) {
          k->snoop_by->snooping = d;
          d->snoop_by           = k->snoop_by;
          k->snoop_by           = nullptr;
        }
        target = k->character;
        *how   = takeover::usurp;
      }
      char_set_desc(k->character, nullptr);
      k->character = nullptr;
      k->original  = nullptr;
      write_to_output(g, k, "\r\nMultiple login detected -- disconnecting.\r\n");
      desc_set_connected(k, CON_CLOSE);
    }
  }
  return target;
}

/* Of the character's bodies left with no connection, keeps the first as the target if there's none yet, and
   extracts the rest (theoretically there are none). */
static char_data *sweep_linkless_bodies(GameWorld *g, int id, char_data *target, takeover *how)
{
  for (char_data *ch : each_char(g)) {
    if (char_is_npc(ch) || char_get_idnum(ch) != id || char_get_desc(ch) || ch == target)
      continue;
    if (!target) {
      target = ch;
      *how   = takeover::reconnect;
      continue;
    }
    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (char_get_in_room(ch) != NOWHERE)
      char_from_room(g, ch);
    char_to_room(g, ch, 1);
    extract_char(g, ch);
  }
  return target;
}

static void reconnected(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "Reconnecting.\r\n");
  pay_player_count_rewards(g, update_player_count(g));
  char_set_time_logon(d->character, game_now(g));
  act(g, "$n has reconnected.", true, d->character, nullptr, nullptr, act_to::room);
  mudlog(g, LOG_NRM, MAX(ADMLVL_NONE, char_get_invis_level(g, d->character)), true, "{} [{}] has reconnected.", char_get_name(d->character), d->host);
  char_set_rp(d->character, d->acct.rpp);
  login_notices(g, d, true);
}

static void usurped(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "You take over your own body, already in use!\r\n");
  act(g, "$n suddenly keels over in pain, surrounded by a white aura...\r\n"
      "$n's body has been taken over by a new spirit!",
      true, d->character, nullptr, nullptr, act_to::room);
  char_set_rp(d->character, d->acct.rpp);
  mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, d->character)), true,
         "{} has re-logged in ... disconnecting old socket.", char_get_name(d->character));
}

static void unswitched(GameWorld *g, struct descriptor_data *d)
{
  write_to_output(g, d, "Reconnecting to unswitched char.");
  mudlog(g, LOG_NRM, MAX(ADMLVL_IMMORT, char_get_invis_level(g, d->character)), true, "{} [{}] has reconnected.", char_get_name(d->character), d->host);
}

/* Disconnects anyone else in the character d has logged in as. If its body is in the game, d takes it over and is
   playing: true. Otherwise false, and logging in goes on. */
static bool perform_dupe_check(GameWorld *g, struct descriptor_data *d)
{
  takeover how     = takeover::none;
  char_data *target = close_other_connections(g, d, &how);
  target            = sweep_linkless_bodies(g, char_get_idnum(d->character), target, &how);

  if (!target)
    return false;

  free_char(g, d->character); /* get rid of the old char */
  d->character = target;
  char_set_desc(d->character, d);
  d->original = nullptr;
  char_set_timer(d->character, 0);
  char_plr_set(d->character, player_flag::mailing, false);
  char_plr_set(d->character, player_flag::writing, false);
  char_aff_set(d->character, aff_flag::group, false);
  desc_set_connected(d, CON_PLAYING);

  switch (how) {
    case takeover::none:
      break;
    case takeover::reconnect:
      reconnected(g, d);
      break;
    case takeover::usurp:
      usurped(g, d);
      break;
    case takeover::unswitch:
      unswitched(g, d);
      break;
  }
  return true;
}

/* Where a player loading in starts: their saved room, else the immortal or mortal start (the frozen start for the
   frozen), else the Void. */
static room_vnum start_room(GameWorld *g, struct char_data *ch)
{
  IDXTYPE load_room = char_get_load_room(g, ch);

  /* If char was saved with NOWHERE, or the room is gone... */
  if (!room_of(g, load_room))
    load_room = char_get_admlevel(ch) ? CONFIG_IMMORTAL_START : CONFIG_MORTAL_START;
  if (char_plr_flagged(ch, player_flag::frozen))
    load_room = CONFIG_FROZEN_START;
  if (!room_of(g, load_room)) /* a builder deleted the start room */
    load_room = 0;            /* The Void */
  return load_room;
}

/* Puts the player in the game and their start room, loads what they saved (returning Crash_load's result), and
   takes back their charmed followers. */
static int place_player(GameWorld *g, struct descriptor_data *d)
{
  struct char_data *ch = d->character;

  /* We have to place the character in a room before equipping them or equip_char() will gripe about the person in
     NOWHERE. */
  room_vnum load_room = start_room(g, ch);

  /* registered first, as rooms and inventories list their members by id */
  char_set_id(ch, char_get_idnum(ch));
  char_register(g, ch);
  char_to_room(g, ch, load_room);
  int load_result = Crash_load(g, ch);
  char_get_player_specials(ch)->host = d->host;
  read_saved_vars(ch);
  for (char_data *check : each_char(g))
    if (!char_get_master(g, check) && char_is_npc(check) && char_get_master_id(check) == char_get_idnum(ch) &&
        char_aff_flagged(check, aff_flag::charm) && !circle_follow(g, check, ch))
      add_follower(g, check, ch);
  save_char(g, ch);
  return load_result;
}

/* What doesn't last from one session to the next: healed limbs, combat and transient states. */
static void reset_session_state(GameWorld *g, struct char_data *ch)
{
  static const struct {
    player_flag flag;
    int limb;
  } regrown[] = {
      {player_flag::rarm, 0},
      {player_flag::larm, 1},
      {player_flag::lleg, 3},
      {player_flag::rleg, 2},
  };

  for (const auto &r : regrown)
    if (char_plr_flagged(ch, r.flag)) {
      char_set_limb_condition(ch, r.limb, 100);
      char_plr_set(ch, r.flag, false);
    }
  char_set_combine(ch, -1);
  char_set_sleeptime(ch, 8);
  char_set_foodr(ch, 2);
  char_set_altitude(ch, char_aff_flagged(ch, aff_flag::flying) ? 1 : 0);
  for (aff_flag aff : {aff_flag::position, aff_flag::sanctuary, aff_flag::zanzoken})
    if (char_aff_flagged(ch, aff))
      char_aff_set(ch, aff, false);
  if (char_plr_flagged(ch, player_flag::knocked))
    char_plr_set(ch, player_flag::knocked, false);
  if (char_is_android(ch) && !char_aff_flagged(ch, aff_flag::infravision))
    char_aff_set(ch, aff_flag::infravision, true);
  char_set_absorbing(ch, nullptr);
  char_set_absorbby(ch, nullptr);
  char_set_sits(ch, nullptr);
  char_set_blocked(ch, nullptr);
  char_set_blocks(ch, nullptr);
  char_set_overf(ch, false);
  char_set_spam(ch, 0);
  char_set_rage_meter(ch, 0);
  if (char_affects(ch).empty() && char_aff_flagged(ch, aff_flag::healglow))
    char_aff_set(ch, aff_flag::healglow, false);
  char_set_speedboost(ch, char_aff_flagged(ch, aff_flag::hayasa) ? char_get_speedcalc(g, ch) * 0.5 : 0);
  if (char_get_trp(ch) < char_get_rp(ch))
    char_set_trp(ch, char_get_rp(ch));
  if (char_is_namek(ch) && char_get_conditions(g, ch, HUNGER) >= 0)
    char_set_conditions(g, ch, HUNGER, -1);
  if (char_plr_flagged(ch, player_flag::healt))
    char_plr_set(ch, player_flag::healt, false);
}

/* What the race brings with it: the moon's and the stars' forms, and the skills it starts with. */
static void apply_race(GameWorld *g, struct char_data *ch)
{
  if (char_moon_ok(g, ch))
    oozaru_transform(g, ch);
  else
    oozaru_revert(g, ch);
  if (char_is_hoshijin(ch))
    star_phase(g, ch, g->time_info.day <= 14 ? 1 : g->time_info.day <= 21 ? 2 : 0);

  if (char_is_icer(ch) && !char_get_skill(ch, SKILL_TAILWHIP)) {
    int numb = rand_number(g, 20, 30);
    char_set_skills_base(ch, SKILL_TAILWHIP, numb);
  } else if (!char_is_icer(ch) && char_get_skill(ch, SKILL_TAILWHIP)) {
    char_set_skills_base(ch, SKILL_TAILWHIP, 0);
  }
  if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 9 || char_get_genome(ch, 1) == 9) && !char_get_skill(ch, SKILL_TELEPATHY))
    char_set_skills_base(ch, SKILL_TELEPATHY, 50);
  if (char_is_bio(ch) && (char_get_genome(ch, 0) == 7 || char_get_genome(ch, 1) == 7) && !char_get_skill(ch, SKILL_TELEPATHY) && !char_get_skill(ch, SKILL_FOCUS)) {
    char_set_skills_base(ch, SKILL_TELEPATHY, 30);
    char_set_skills_base(ch, SKILL_FOCUS, 30);
  }
}

/* Loads the player into the game (used by copyover_recover too). Returns Crash_load's result. */
int enter_player_game(GameWorld *g, struct descriptor_data *d)
{
  struct char_data *ch = d->character;

  reset_char(g, ch);
  read_aliases(g, ch);
  racial_body_parts(ch);
  if (char_plr_flagged(ch, player_flag::invstart))
    char_set_invis_level(g, ch, char_get_level(ch));

  int load_result = place_player(g, d);
  if (!d->acct.customfile) {
    customCreate(d);
    save_account(d);
  }
  reset_session_state(g, ch);

  if (readIntro(ch, ch) == 2)
    introCreate(ch);
  if (read_sense_memory(g, ch, ch) == 2)
    senseCreate(ch);
  if (char_get_admlevel(ch) > 0)
    d->acct.level = 1;
  char_set_rp(ch, d->acct.rpp);
  char_set_rbank(ch, d->acct.rbank);

  apply_race(g, ch);
  char_set_combo(ch, -1);
  return load_result;
}

/* A builder can delete any of the rooms that players get sent to, while the game is running. */
static room_vnum room_or_void(GameWorld *g, room_vnum vnum)
{
  return room_of(g, vnum) ? vnum : 0;
}

/* RPP for everyone playing (not staff) not yet paid for this payout: 1 for a player count within 4 of the record, 2
   for tying it, 3 for a new one. A payout lasts a day, unless a better one comes. */
static void payout(GameWorld *g, int num)
{
  static constexpr std::string_view why[] = {
      "@D[@G+ 1 RPP@D] @cA total logon count within 4 of the highest has been achieved.@n\r\n",
      "@D[@G+ 2 RPP@D] @cThe total logon count has tied with the highest ever.@n\r\n",
      "@D[@G+ 3 RPP@D] @cA new logon count record has been achieved!@n\r\n",
  };

  if (g->LASTPAYOUT == 0 || num > g->LASTPAYTYPE || g->LASTPAYOUT <= game_now(g)) {
    g->LASTPAYOUT  = game_now(g) + 86400;
    g->LASTPAYTYPE = num;
  }
  int rpp = MIN(num, 2) + 1;
  for (descriptor_data *k : each_desc(g)) {
    if (desc_is_playing(k) && char_get_admlevel(k->character) <= 0 && char_get_rewtime(k->character) < g->LASTPAYOUT) {
      k->acct.rpp += rpp;
      char_set_rp(k->character, k->acct.rpp);
      char_set_trp(k->character, char_get_trp(k->character) + (rpp));
      save_account(k);
      send_to_char(g, k->character, "{}", why[rpp - 1]);
      char_set_rewtime(k->character, g->LASTPAYOUT);
    }
  }
}

/* A character name: letters only, 2 to MAX_NAME_LENGTH of them, allowed, and not a fill or reserved word. The name
   is copied into name. */
static bool acceptable_name(GameWorld *g, std::string_view typed, std::string &name)
{
  std::optional<std::string> parsed = parse_name(typed);

  if (!parsed)
    return false;
  name = std::move(*parsed);
  return name.size() >= 2 && name.size() <= MAX_NAME_LENGTH && Valid_Name(g, name.data()) && !is_fill_word(name) &&
         !is_reserved_word(name);
}

/* The account's slot names a character with no player file: empty it. */
static void character_missing(GameWorld *g, struct descriptor_data *d)
{
  empty_account_slots_named(d, d->loadplay.c_str());
  d->loadplay.clear();
  save_account(d);
  enter_state(g, d, CON_UMENU);
  write_to_output(g, d, "Character missing. Emptying slot.\r\n");
}

/* A new character in the empty slot chosen: on to creation. */
static void start_new_character(GameWorld *g, struct descriptor_data *d, std::string &name)
{
  /* Check for multiple creations of a character. */
  if (!Valid_Name(g, name.data())) {
    write_to_output(g, d, "Invalid name, please try another.\r\nName: ");
    return;
  }
  char_set_alias(d->character, CAP(name));
  char_set_rp(d->character, d->acct.rpp);
  d->acct.slots[d->writenew - 1] = char_get_alias(d->character);
  save_account(d);
  d->chargen = {};
  enter_state(g, d, CON_QRACE);
}

/* A deleted character's name: its old files go, so the new character doesn't get its equipment, and creation starts
   over. */
static void remake_deleted_character(GameWorld *g, struct descriptor_data *d, std::string &name)
{
  if (int player_i = get_ptable_by_name(g, name.c_str()); player_i >= 0)
    remove_player(g, player_i);

  /* We get a false positive from the original deleted character. */
  free_char(g, d->character);
  d->character = nullptr; /* Valid_Name looks at every connection's character */
  /* Check for multiple creations... */
  if (!Valid_Name(g, name.data())) {
    write_to_output(g, d, "@YInvalid name@n, please try @Canother.@n\r\nName: ");
    return;
  }
  d->character = new char_data();
  clear_char(d->character);
  char_make_player_specials(d->character);
  char_set_desc(d->character, d);
  char_set_alias(d->character, CAP(name));
  /* no place in the player index: the old one's emptied, and finishing creation makes a new one */
  char_prf_set(g, d->character, pref_flag::color, true);
  char_set_rp(d->character, d->acct.rpp);
  d->chargen = {};
  enter_state(g, d, CON_QRACE);
}

/* An existing character loaded: refused for the site or the wizlock, put into its body if that's in the game, or on
   to the menu. */
static void log_in_character(GameWorld *g, struct descriptor_data *d)
{
  /* undo it just in case they are set */
  char_plr_set(d->character, player_flag::writing, false);
  char_plr_set(d->character, player_flag::mailing, false);
  char_plr_set(d->character, player_flag::cryo, false);
  char_aff_set(d->character, aff_flag::group, false);
  if (isbanned(g, d->host) == BAN_SELECT && !char_plr_flagged(d->character, player_flag::siteok)) {
    write_to_output(g, d, "Sorry, this char has not been cleared for login from your site!\r\n");
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Connection attempt for {} denied from {}", char_get_name(d->character), d->host);
    desc_set_connected(d, CON_CLOSE);
    return;
  }
  if (char_get_level(d->character) < g->circle_restrict && g->circle_restrict < 101) {
    enter_state(g, d, CON_UMENU);
    write_to_output(g, d, "The game is temporarily restricted to at least {:d} level.\r\n", g->circle_restrict);
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Request for character load denied for {} [{}] (wizlock)", char_get_name(d->character), d->host);
    return;
  }
  /* check and make sure no other copies of this player are logged in */
  if (perform_dupe_check(g, d))
    return;
  d->idle_tics = 0;
  write_to_output(g, d, "\r\n{}\r\n{}", g->motd, CONFIG_MENU);
  char_set_rp(d->character, d->acct.rpp);
  desc_set_connected(d, CON_MENU);
}

/* CON_GET_NAME: the name of a new character for an empty slot, or (after Return) the character in the slot chosen */
static void state_get_name(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  std::string tmp_name;
  int player_i;

  if (d->character) {
    free_char(g, d->character);
  }
  if (!d->character) {
    d->character = new char_data();
    clear_char(d->character);
    char_make_player_specials(d->character);
    char_set_desc(d->character, d);
    char_prf_set(g, d->character, pref_flag::color, true);
  }
  if (d->writenew > 0) {
    if (arg.empty()) {
      write_to_output(g, d, "Enter name: ");
      return;
    }
    d->loadplay = arg;
  }
  if (!acceptable_name(g, d->loadplay, tmp_name)) {
    write_to_output(g, d, "Invalid name, please try another.\r\nName: ");
    return;
  }

  player_i = load_char(g, tmp_name, d->character);
  if (player_i < 0) {
    if (d->writenew <= 0)
      character_missing(g, d);
    else /* player unknown -- make new character */
      start_new_character(g, d, tmp_name);
    return;
  }
  if (d->writenew > 0) {
    enter_state(g, d, CON_UMENU);
    write_to_output(g, d, "That character is already taken.\r\n");
    d->writenew = 0;
    return;
  }
  char_set_pfilepos(d->character, player_i);
  if (char_plr_flagged(d->character, player_flag::deleted))
    remake_deleted_character(g, d, tmp_name);
  else
    log_in_character(g, d);
}

/* CON_NAME_CNFRM: wait for conf. of new name */
static void state_name_cnfrm(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  if (toupper(first_char(arg)) == 'Y') {
    if (isbanned(g, d->host) >= BAN_NEW) {
      mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Request for new char {} denied from [{}] (siteban)", char_get_pc_name(d->character), d->host);
      write_to_output(g, d, "Sorry, new characters are not allowed from your site!\r\n");
      desc_set_connected(d, CON_CLOSE);
      return;
    }
    if (g->circle_restrict) {
      write_to_output(g, d, "Sorry, new players can't be created at the moment.\r\n");
      mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Request for new char {} denied from [{}] (wizlock)", char_get_pc_name(d->character), d->host);
      desc_set_connected(d, CON_CLOSE);
      return;
    }
    d->idle_tics = 0;
    write_to_output(g, d, "@MNew character.@n\r\nGive me a @gpassword@n for @C{}@n: ", char_get_pc_name(d->character));
    desc_set_connected(d, CON_NEWPASSWD);
  } else if (first_char(arg) == 'n' || first_char(arg) == 'N') {
    write_to_output(g, d, "Okay, what IS it, then? ");
    char_set_alias(d->character, "");
    desc_set_connected(d, CON_GET_NAME);
  } else
    write_to_output(g, d, "Please type Yes or No: ");
}

/* CON_GET_USER */
static void state_get_user(GameWorld *g, struct descriptor_data *d, std::string_view typed)
{
  if (isbanned(g, d->host)) {
    write_to_output(g, d, "You have been banned. Have a nice day.\r\n");
    desc_set_connected(d, CON_CLOSE);
    return;
  }
  if (typed.empty()) {
    write_to_output(g, d, "Enter your desired username or the username you have already made.\nUsername?\r\n");
    return;
  }
  if (str_eq_nocase(typed, "index")) {
    write_to_output(g, d, "Try again, username?\r\n");
    return;
  }
  std::string arg(typed);
  CAP(arg); /* the name is capitalised from here on */
  if (!Valid_Name(g, arg)) {
    write_to_output(g, d, "Invalid name. Username?\r\n");
    return;
  }
  /* the name becomes a file name under data/user, so letters only */
  if (!std::ranges::all_of(arg, [](unsigned char c) { return std::isalpha(c); })) {
    write_to_output(g, d, "Letters only: no spaces, numbers or punctuation. Username?\r\n");
    return;
  }
  if (arg.size() < 3) {
    write_to_output(g, d, "Name must at least be 3 characters long, username?\r\n");
    return;
  }
  if (arg.size() > 10) {
    write_to_output(g, d, "Name must be at most 10 characters long, username?\r\n");
    return;
  }
  if (!readUserIndex(arg)) {
    if (g->circle_restrict == 101) {
      write_to_output(g, d, "The mud is locked for mortals at this time.\r\n");
      write_to_output(g, d, "Please check: www.advent-truth.com/forum for further details.\r\n");
      send_to_imm(g, "{} rejected by wizlock", arg);
      desc_set_connected(d, CON_CLOSE);
    } else {
      d->acct.user = arg;
      write_to_output(g, d, "You want you user name to be, {}?\r\n", d->acct.user);
      write_to_output(g, d, "Yes or no: \n");
      desc_set_connected(d, CON_USER_CONF);
    }
  } else {
    if (g->circle_restrict == 101) {
      d->acct.user = arg;
      userLoad(d, d->acct.user);
      if (d->acct.level == 0) {
        write_to_output(g, d, "The mud is locked for mortals at this time.\r\n");
        send_to_imm(g, "{} rejected by wizlock", d->acct.user);
        desc_set_connected(d, CON_CLOSE);
      } else {
        write_to_output(g, d, "Password: \r\n");
        send_to_imm(g, "Username, {}, logging in.", arg);
        desc_set_connected(d, CON_PASSWORD);
      }
    } else {
      userLoad(d, arg);
      write_to_output(g, d, "Password: \r\n");
      send_to_imm(g, "Username, {}, logging in.", arg);
      desc_set_connected(d, CON_PASSWORD);
    }
  }
}

/* CON_USER_CONF */
static void state_user_conf(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  if (arg.empty()) {
    write_to_output(g, d, "You want your user name to be, {}?\r\n", d->acct.user);
    write_to_output(g, d, "Yes or no: \n");
    return;
  }
  if (str_eq_nocase(arg, "yes") || str_eq_nocase(arg, "y")) {
    write_to_output(g, d, "User Account, {}, created.\r\nEnter Email:\n", d->acct.user);
    write_to_output(g, d, "Remember your email must be valid and matching the example given.\n");
    write_to_output(g, d, "[Example: iovan@@advent-truth.com]\n");
    send_to_imm(g, "Username, {}, creating.", CAP(d->acct.user));
    desc_set_connected(d, CON_GET_EMAIL);
  } else if (str_eq_nocase(arg, "no") || str_eq_nocase(arg, "n")) {
    d->acct.user = "Empty";
    write_to_output(g, d, "Enter Username: \n");
    desc_set_connected(d, CON_GET_USER);
  } else {
    write_to_output(g, d, "You want you user name to be, {}?\r\n", d->acct.user);
    write_to_output(g, d, "Yes or no: \n");
    return;
  }
}

/* User files hold one value per line, read back as a single word; get_line() skips lines starting with '*'. */
static bool fits_user_file(std::string_view value, size_t max_length)
{
  return !value.starts_with('*') && value.size() <= max_length && value.find_first_of(" \t") == std::string_view::npos;
}

/* CON_GET_EMAIL */
static void state_get_email(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  if (readUserIndex(d->acct.user) && arg.empty()) {
    write_to_output(g, d, "Email, or M for menu?\r\n");
    return;
  }
  if (arg.empty()) {
    write_to_output(g, d, "Email?\r\n");
    return;
  }
  if (str_eq_nocase(arg, "M") && readUserIndex(d->acct.user)) {
    enter_state(g, d, CON_UMENU);
  } else if (!arg.contains(".com") && !arg.contains(".net") && !arg.contains(".org")) {
    write_to_output(g, d, "Improper email format missing '.com' or '.net' or '.org'. Email?\r\n");
    return;
  } else if (!fits_user_file(arg, MAX_EMAIL_LENGTH)) {
    write_to_output(g, d, "Emails can be at most {:d} characters, with no spaces, and can't start with '*'. Email?\r\n", MAX_EMAIL_LENGTH);
    return;
  } else if (readUserIndex(d->acct.user)) {
    d->acct.email = arg;
    search_replace(d->acct.email, "@", "<AT>");
    save_account(d);
    enter_state(g, d, CON_UMENU);
  } else {
    d->acct.email = arg;
    search_replace(d->acct.email, "@", "<AT>");
    write_to_output(g, d, "Your email is: {}\n", d->acct.email);
    write_to_output(g, d, "Password: \r\n");
    desc_set_connected(d, CON_NEWPASSWD);
  }
}

/* CON_NEWPASSWD */
static void state_newpasswd(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  if (readUserIndex(d->acct.user) && arg.empty()) {
    write_to_output(g, d, "Password, or M for menu?\r\n");
    return;
  }
  if (arg.empty()) {
    write_to_output(g, d, "Password?\r\n");
    return;
  }
  if (str_eq_nocase(arg, "M") && readUserIndex(d->acct.user)) {
    enter_state(g, d, CON_UMENU);
  } else if (arg.size() > MAX_PWD_LENGTH && readUserIndex(d->acct.user)) {
    write_to_output(g, d, "Password is too long. Password, or M for menu?\r\n");
    return;
  } else if (arg.size() > MAX_PWD_LENGTH) {
    write_to_output(g, d, "Password is too long. Password?\r\n");
    return;
  } else if (!fits_user_file(arg, MAX_PWD_LENGTH)) {
    write_to_output(g, d, "Passwords can't contain spaces or start with '*'. Password{}?\r\n", readUserIndex(d->acct.user) ? ", or M for menu" : "");
    return;
  } else if (readUserIndex(d->acct.user)) {
    write_to_output(g, d, "Your password and user account have been fully saved.\r\n");
    d->acct.pass = arg;
    save_account(d);
    enter_state(g, d, CON_UMENU);
  } else {
    write_to_output(g, d, "Your password and user account have been fully saved.\r\n");
    d->acct.pass = arg;
    userCreate(d);
    enter_state(g, d, CON_UMENU);
  }
}

/* CON_PASSWORD */
static void state_password(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{

  if (arg.empty()) {
    write_to_output(g, d, "Enter Password or Return:\r\n");
    write_to_output(g, d, "(Return will ask for a different username)\r\n");
    return;
  }
  if (str_eq_nocase("return", arg)) {
    d->acct.user = "Empty";
    write_to_output(g, d, "Username?\r\n");
    desc_set_connected(d, CON_GET_USER);
    return;
  }
  if (str_eq_nocase(d->acct.pass, arg)) {
    for (descriptor_data *k : each_desc(g)) {
      if (k == d)
        continue;
      if (k->acct.user.empty())
        continue;
      if (d->acct.user.empty())
        continue;
      if (str_eq_nocase(k->acct.user, d->acct.user)) {
        if (desc_get_connected(k) == CON_PLAYING) {
          desc_set_connected(k, CON_DISCONNECT);
          write_to_output(g, k, "Your account has been usurped by someone who knows its password!@n");
        } else {
          desc_set_connected(k, CON_CLOSE);
          write_to_output(g, k, "Your account has been usurped by someone who knows its password!@n");
        }
      }
    }
    enter_state(g, d, CON_UMENU);
  } else {
    send_to_imm(g, "Username, {}, password failure!", CAP(d->acct.user));
    mud_log("{} BAD PASSWORD from [{}]", CAP(d->acct.user), d->host);
    /* counted per connection, so switching usernames does not reset it */
    if (++d->bad_pws >= CONFIG_MAX_BAD_PWS) {
      write_to_output(g, d, "Too many wrong passwords. Disconnecting.\r\n");
      mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Disconnecting [{}] after {:d} wrong passwords (last tried for {}).", d->host, d->bad_pws, CAP(d->acct.user));
      desc_set_connected(d, CON_CLOSE);
      return;
    }
    write_to_output(g, d, "Password is wrong. Password or Return?\r\n");
    write_to_output(g, d, "(Return will ask for a different username)\r\n");
    return;
  }
}

/* Account menu B: a fourth or fifth character slot for 15 RPP. */
static void buy_slot(GameWorld *g, struct descriptor_data *d)
{
  if (d->acct.total != 3 && d->acct.total != 4) {
    userRead(g, d);
    write_to_output(g, d, "You can't have more than 5 character slots.\n");
  } else if (d->acct.rpp < 15) {
    userRead(g, d);
    write_to_output(g, d, "You need at least 15 RPP to purchase a new character slot.\n");
  } else {
    d->acct.rpp -= 15;
    d->acct.total++;
    save_account(d);
    userRead(g, d);
    write_to_output(g, d, "New slot purchased, -15 RPP.\n");
  }
}

/* Account menu 1-5: create a character in an empty slot, or load the one in it. */
static void choose_slot(GameWorld *g, struct descriptor_data *d, int n)
{
  if (n < 1 || n > MAX_ACCOUNT_SLOTS) {
    userRead(g, d);
    write_to_output(g, d, "That is not an option.\n");
    return;
  }
  if (n > 3 && d->acct.total < n) {
    userRead(g, d);
    write_to_output(g, d, "You only have {:d} character slots avaialable!\r\n", d->acct.total);
    return;
  }
  const std::string &name = d->acct.slots[n - 1];
  if (str_eq_nocase(name, "Empty")) {
    write_to_output(g, d, "Enter New Character Name: \n");
    d->writenew = n;
    desc_set_connected(d, CON_GET_NAME);
  } else if (lockRead(name) && d->acct.level <= 0) {
    write_to_output(g, d, "That character has been locked out for rule violations. Play another character.\n");
  } else {
    write_to_output(g, d, "Loading character, {}.\r\n***Press Return***\n", name);
    d->loadplay = name;
    d->writenew = 0;
    desc_set_connected(d, CON_GET_NAME);
  }
}

/* The account menu's options other than the slots, each a letter in either case. */
static const struct {
  std::string_view key;
  void (*choose)(GameWorld *g, struct descriptor_data *d);
} account_options[] = {
    {"Q", [](GameWorld *g, descriptor_data *d) {
       write_to_output(g, d, "Thanks for visiting!\n");
       desc_set_connected(d, CON_CLOSE);
     }},
    {"P", [](GameWorld *g, descriptor_data *d) {
       write_to_output(g, d, "Enter New Password, or M for menu::\n");
       desc_set_connected(d, CON_NEWPASSWD);
     }},
    {"C", [](GameWorld *g, descriptor_data *d) {
       write_to_output(g, d, "\n");
       customRead(g, d, 0, ""); /* the name is only read for type 1 */
       write_to_output(g, d, "\r\n@n--Press Enter--\n@n");
     }},
    {"E", [](GameWorld *g, descriptor_data *d) {
       write_to_output(g, d, "Enter New Email, or M for menu:\n");
       desc_set_connected(d, CON_GET_EMAIL);
     }},
    {"D", [](GameWorld *g, descriptor_data *d) {
       write_to_output(g, d, "Are you sure you want to delete your user file and all its characters? Yes or no:\n");
       desc_set_connected(d, CON_DELCNF1);
     }},
    {"B", buy_slot},
};

/* CON_UMENU */
static void state_umenu(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  if (arg.empty()) {
    userRead(g, d);
    return;
  }
  for (const auto &option : account_options)
    if (str_eq_nocase(arg, option.key)) {
      option.choose(g, d);
      return;
    }
  choose_slot(g, d, static_cast<int>(io::c_atol(arg)));
}

/* CON_RMOTD: read CR after printing motd */
static void state_rmotd(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  write_to_output(g, d, CONFIG_MENU);
  desc_set_connected(d, CON_MENU);
}

/* Where a player entering from the menu ends up: if they were put in the Void or room 1, their saved room (the
   Void if it's gone), else room 300; otherwise they look around where they are. */
static void settle_entered_player(GameWorld *g, struct char_data *ch)
{
  if (char_get_in_room(ch) <= 1 && char_get_load_room(g, ch) != NOWHERE) {
    char_from_room(g, ch);
    char_to_room(g, ch, room_or_void(g, char_get_load_room(g, ch)));
  } else if (char_get_in_room(ch) <= 1) {
    char_from_room(g, ch);
    char_to_room(g, ch, room_or_void(g, 300));
  } else {
    look_at_room(g, char_get_in_room(ch), ch, 0);
  }
}

/* Menu 1: into the game. */
static void enter_game(GameWorld *g, struct descriptor_data *d)
{
  if (lockRead(char_get_name(d->character)) && d->acct.level <= 0) {
    write_to_output(g, d, "That character has been locked out for rule violations. Play another character.\n");
    return;
  }
  int load_result = enter_player_game(g, d);
  send_to_char(g, d->character, CONFIG_WELC_MESSG);
  act(g, "$n has entered the game.", true, d->character, nullptr, nullptr, act_to::room);

  int oldcount = update_player_count(g);
  char_set_time_logon(d->character, game_now(g));
  greet_mtrigger(g, d->character, -1);
  greet_memory_mtrigger(g, d->character);
  desc_set_connected(d, CON_PLAYING);
  pay_player_count_rewards(g, oldcount);

  if (char_get_level(d->character) == 0) {
    do_start(g, d->character);
    send_to_char(g, d->character, CONFIG_START_MESSG);
  }
  settle_entered_player(g, d->character);
  login_notices(g, d, false);
  if (!char_is_android(d->character)) {
    send_to_sense(g, 0, "You sense someone appear suddenly", d->character);
    std::string blip = str_format("@D[@GBlip@D]@Y {}\r\n@RSomeone has suddenly entered your scouter detection range!@n.",
                                  add_commas(getCurPL(g, d->character)));
    send_to_scouter(g, blip.c_str(), d->character, 0, 0);
  }
  if (load_result == 2) { /* rented items lost */
    send_to_char(g, d->character, "\r\n\007You could not afford your rent!\r\nYour possesions have been donated to the Salvation Army!\r\n");
  }
  d->has_prompt = false;
  /* We've updated to 3.1 - some bits might be set wrongly: */
  char_prf_set(g, d->character, pref_flag::buildwalk, false);
  if (!char_get_equipment(d->character, WEAR_WIELD1) && char_plr_flagged(d->character, player_flag::thandw))
    char_plr_set(d->character, player_flag::thandw, false);
}

/* CON_MENU: get selection from main menu */
static void state_menu(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  switch (first_char(arg)) {
    case '0':
      write_to_output(g, d, "Goodbye.\r\n");
      desc_set_connected(d, CON_CLOSE);
      break;

    case '1':
      enter_game(g, d);
      break;

    case '2':
      if (!char_get_ddesc(d->character).empty()) {
        write_to_output(g, d, "Current description:\r\n{}", char_get_ddesc(d->character));
      }
      write_to_output(g, d, "Enter the new text you'd like others to see when they look at you.\r\n");
      send_editor_help(g, d);
      string_edit_begin(d, char_description_slot(d->character), EXDSCR_LENGTH);
      desc_set_connected(d, CON_EXDESC);
      break;

    case '3':
      enter_state(g, d, CON_UMENU);
      break;

    case '4':
      write_to_output(g, d, "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\nARE YOU ABSOLUTELY SURE?\r\n\r\nPlease type \"yes\" to confirm: ");
      desc_set_connected(d, CON_DELCNF2);
      break;

    default:
      write_to_output(g, d, "\r\nThat's not a menu choice!\r\n{}\r\n{}", g->motd, CONFIG_MENU);
      break;
  }
}

/* CON_DELCNF1 */
static void state_delcnf1(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  if (arg == "yes" || arg == "YES") {
    write_to_output(g, d, "Your user and character files have been deleted. Good bye.\n");
    userDelete(g, d);
    desc_set_connected(d, CON_CLOSE);
  } else if (arg == "no" || arg == "NO") {
    enter_state(g, d, CON_UMENU);
    write_to_output(g, d, "Nothing was deleted. Phew.\n");
  } else {
    write_to_output(g, d, "Clearly type yes or no. Yes to delete and no to return to the menu.\nYes or no:\n");
    return;
  }
}

/* RPP back for a deleted character: 2 for every 10 levels, once past level 19 (past 40 for the races that level free
   that far; none for the restricted races), and the race's own refund past level 1. */
static void refund_deleted_character(GameWorld *g, struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  int level            = char_get_level(ch);
  bool cheap           = char_is_cheap_race(ch);

  if (cheap ? level > 40 : level > 19 && !char_is_restricted_race(ch)) {
    int refund = level / 10 * 2;
    write_to_output(g, d, "@D[@g{:d} RPP refunded to your account for your character's levels.@D]@n\r\n", refund);
    d->acct.rpp += refund;
  }
  if (cheap && level <= 40)
    write_to_output(g, d, "@D[@gSince your race doesn't cost RPP to level before 40 you are refunded 0 RPP.@D]@n\r\n");
  int refund = race_rpp_refund(char_get_race(ch));
  if (refund && level > 1) {
    write_to_output(g, d, "@D[@g{:d} RPP refunded to your account for your {} character.@D]@n\r\n", refund, char_true_race_name(ch));
    d->acct.rpp += refund;
  }
}

/* CON_DELCNF2 */
static void state_delcnf2(GameWorld *g, struct descriptor_data *d, std::string_view arg)
{
  int player_i;

  if (arg == "yes" || arg == "YES") {
    if (char_plr_flagged(d->character, player_flag::frozen)) {
      write_to_output(g, d, "You try to kill yourself, but the ice stops you.\r\nCharacter not deleted.\r\n\r\n");
      desc_set_connected(d, CON_CLOSE);
      return;
    }
    if (char_get_admlevel(d->character) < ADMLVL_GRGOD)
      char_plr_set(d->character, player_flag::deleted, true);
    save_char(g, d->character);
    Crash_delete_file(char_get_name(d->character));
    /* If the selfdelete_fastwipe flag is set (in config.c), remove all
     the player's immediately
  */
    if (selfdelete_fastwipe)
      if ((player_i = get_ptable_by_name(g, char_get_name(d->character))) >= 0) {
        g->player_table[player_i].flags.set(pindex_flag::selfdelete);
        remove_player(g, player_i);
      }
    delete_aliases(char_get_name(d->character));
    delete_variables(char_get_name(d->character));
    delete_inv_backup(d->character);
    write_to_output(g, d, "Character '{}' deleted!\r\n", char_get_name(d->character));
    refund_deleted_character(g, d);
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "User {} has deleted character {} (lev {:d}).", d->acct.user, char_get_name(d->character), char_get_level(d->character));

    empty_account_slots_named(d, char_get_name(d->character));
    save_account(d);
    enter_state(g, d, CON_UMENU);
    return;
  }
  write_to_output(g, d, "\r\nCharacter not deleted.\r\n{}\r\n{}", g->motd, CONFIG_MENU);
  desc_set_connected(d, CON_MENU);
}

static const auto login_states = std::to_array<con_state>({
    {CON_GET_NAME,   state_get_name  },
    {CON_NAME_CNFRM, state_name_cnfrm},
    {CON_GET_USER,   state_get_user  },
    {CON_USER_CONF,  state_user_conf },
    {CON_GET_EMAIL,  state_get_email },
    {CON_NEWPASSWD,  state_newpasswd },
    {CON_PASSWORD,   state_password  },
    {CON_UMENU,      state_umenu,      userRead},
    {CON_RMOTD,      state_rmotd     },
    {CON_MENU,       state_menu      },
    {CON_DELCNF1,    state_delcnf1   },
    {CON_DELCNF2,    state_delcnf2   },
});

/* The editors' states, each answered by context help first */
static const auto olc_states = std::to_array<con_state>({
    {CON_OEDIT,    olc_parse,       nullptr, true},
    {CON_IEDIT,    olc_parse,       nullptr, true},
    {CON_ZEDIT,    olc_parse,       nullptr, true},
    {CON_SEDIT,    olc_parse,       nullptr, true},
    {CON_MEDIT,    olc_parse,       nullptr, true},
    {CON_REDIT,    olc_parse,       nullptr, true},
    {CON_CEDIT,    olc_parse,       nullptr, true},
    {CON_AEDIT,    olc_parse,       nullptr, true},
    {CON_TRIGEDIT, olc_parse,       nullptr, true},
    {CON_ASSEDIT,  olc_parse,       nullptr, true},
    {CON_GEDIT,    olc_parse,       nullptr, true},
    {CON_HEDIT,    olc_parse,       nullptr, true},
    {CON_HSEDIT,   olc_parse,       nullptr, true},
    {CON_POBJ,     pobj_edit_parse, nullptr, true},
});

/* The state's row: the editors', then the login menus', then character creation's. */
static const con_state *find_state(int state)
{
  for (std::span<const con_state> part : {std::span<const con_state>(olc_states), std::span<const con_state>(login_states), CHARGEN_STATES})
    for (const con_state &s : part)
      if (s.state == state)
        return &s;
  return nullptr;
}

void enter_state(GameWorld *g, struct descriptor_data *d, int state)
{
  desc_set_connected(d, state);
  if (const con_state *s = find_state(state); s && s->prompt)
    s->prompt(g, d);
}

void nanny(GameWorld *g, struct descriptor_data *d, std::string_view line)
{
  /* as long as the input buffer it once went through holds, and up to a '\0', where the C string it was ended */
  std::string_view arg = skip_spaces(line).substr(0, MAX_INPUT_LENGTH - 1);
  arg                  = arg.substr(0, arg.find('\0'));

  if (d->character == nullptr) {
    d->character = new char_data();
    clear_char(d->character);
    char_make_player_specials(d->character);
    char_set_desc(d->character, d);
  }

  if (const con_state *s = find_state(desc_get_connected(d))) {
    /* send context-sensitive help if need be */
    if (s->olc && context_help(g, d, arg))
      return;
    s->parse(g, d, arg);
    return;
  }

  /*
   * It's possible, if enough pulses are missed, to kick someone off
   * while they are at the password prompt. We'll just defer to let
   * the game_loop() axe them.
   */
  if (desc_get_connected(d) == CON_CLOSE)
    return;

  mud_log("SYSERR: Nanny: illegal state of con'ness ({:d}) for '{}'; closing connection.",
          desc_get_connected(d), d->character ? char_get_name(d->character) : "<unknown>");
  desc_set_connected(d, CON_DISCONNECT); /* Safest to do. */
}
