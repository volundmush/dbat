/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <array>
#include <cstdlib>
#include "dbat/name_table.hpp"

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/utils.hpp"
#include "dbat/weather.hpp"
#include "dbat/command.hpp"
#include "dbat/act.comm.hpp"
#include "dbat/act.social.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.other.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/boards.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/class.hpp"
#include "dbat/random.hpp"
#include "dbat/relocate.hpp"
#include "dbat/search.hpp"
#include "dbat/game_world.hpp"

namespace {
  static constexpr auto languages_entries = std::to_array<std::string_view>({"common", "elven", "gnomish", "dwarven", "halfling", "orc", "druid", "draconic"});
  static constexpr name_table languages(languages_entries);

  /* How ch is named on the channels and in tells: by name if an immortal, else by account */
  std::string_view public_name(struct char_data *ch) { return char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch); }

  /* ", in an unfamiliar tongue," unless the listener knows what ch speaks */
  std::string_view tongue(struct char_data *listener, struct char_data *ch)
  {
    return char_get_skill(listener, char_get_speaking(ch)) ? "," : ", in an unfamiliar tongue,";
  }

  void list_languages(GameWorld *g, struct char_data *ch)
  {
    int a = 0;
    send_to_char(g, ch, "Languages:\r\n[");
    for (int i = MIN_LANGUAGES; i <= MAX_LANGUAGES; i++)
      if (char_get_skill(ch, i))
        send_to_char(g, ch, "{} {}{}{}", a++ != 0 ? "," : "", char_get_speaking(ch) == i ? "@r" : "@n", languages[i - MIN_LANGUAGES], "@n");
    send_to_char(g, ch, "{} ]\r\n", a == 0 ? " None!" : "");
  }

  /* What's said, as heard by someone who doesn't know the language: every letter one of the language's */
  void garble_text(GameWorld *g, std::string &string, int known, int lang)
  {
    if (known)
      return;
    std::string_view letters = lang == SKILL_LANG_DWARVEN ? "hprstwxyz" : lang == SKILL_LANG_ELVEN ? "aefhilnopstu" : "aehiopstuwxyz";
    for (char &c : string)
      if (isalpha(c))
        c = letters[rand_number(g, 0, (int)letters.size() - 1)];
  }
} // namespace

ACMD(do_voice)
{
  std::string text(skip_spaces(argument));

  if (char_is_npc(ch))
    return;
  if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
    send_to_char(g, ch, "You're mute. You don't need to describe your voice.\r\n");
    return;
  }
  if (text.empty()) {
    send_to_char(g, ch, "What are you changing your voice description to?\r\n");
    return;
  }
  if (text.size() > 75) {
    send_to_char(g, ch, "Your voice description can not be longer than 75 characters.\r\n");
    return;
  }
  if (text.contains("@")) {
    send_to_char(g, ch, "You can not use colorcode in voice descriptions.\r\n");
    return;
  }
  bool changing = !char_get_voice(ch).empty();
  if (changing && char_get_rp(ch) < 1) {
    send_to_char(g, ch, "Your voice has already been set. You will need at least 1 RPP to be able to change it.\r\n");
    return;
  }
  send_to_char(g, ch, "Your voice has now been set to: {}\r\n", text);
  char_set_voice(ch, text);
  if (!changing)
    return;
  char_set_rp(ch, char_get_rp(ch) - 1);
  char_get_desc(ch)->acct.rpp = char_get_rp(ch);
  save_account(char_get_desc(ch));
  send_to_char(g, ch, "@D(@cRPP@W: @w-1@D)@n\n\n");
}

ACMD(do_languages)
{
  if (!CONFIG_ENABLE_LANGUAGES) {
    send_to_char(g, ch, "But everyone already understands everyone else!\r\n");
    return;
  }
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    list_languages(g, ch);
    return;
  }
  int known = search_block(arg, languages, false);
  if (known < 0 || MIN_LANGUAGES + known > MAX_LANGUAGES || !char_get_skill(ch, MIN_LANGUAGES + known)) {
    send_to_char(g, ch, "You do not know of any such language.\r\n");
    return;
  }
  char_set_speaking(ch, MIN_LANGUAGES + known);
  send_to_char(g, ch, "You now speak {}.\r\n", languages[known]);
}

ACMD(do_osay)
{
  argument = skip_spaces(argument);

  if (char_is_npc(ch))
    return;
  if (argument.empty()) {
    send_to_char(g, ch, "Yes, but WHAT do you want to osay?\r\n");
    return;
  }
  std::string mine   = str_format("@WYou @D[@mOSAY@D] @W'@w{}@W'@n", argument);
  std::string theirs = str_format("@W{} @D[@mOSAY@D] @W'@w{}@W'@n", char_prf_flagged(g, ch, pref_flag::hide) ? "Anonymous" : public_name(ch), argument);
  act_triple(g, {mine.c_str(), {}, theirs.c_str()}, ch, nullptr, nullptr);
}

namespace {
  /* Takes "to <name> " out of what is said, if it is there. */
  bool strip_address(std::string &text, std::string_view name)
  {
    std::string address = str_format("to {} ", name);
    if (!text.contains(address))
      return false;
    search_replace(text, address, "");
    return true;
  }

  /* Who in the room, if anyone, ch is talking to. Later ones win. */
  struct char_data *addressed(GameWorld *g, struct char_data *ch, std::string &text)
  {
    struct char_data *to = nullptr;
    for (char_data *tch : room_people(g, char_get_in_room(ch))) {
      if (tch == ch || !char_get_desc(tch))
        continue;
      if (strip_address(text, char_get_name(tch))) {
        to = tch;
        continue;
      }
      if (char_is_npc(tch) || char_is_npc(ch) || readIntro(ch, tch) != 1)
        continue;
      if (strip_address(text, get_i_name(ch, tch)))
        to = tch;
    }
    return to;
  }

  std::string_view say_verb(const std::string &text)
  {
    switch (text.back()) {
      case '!':
        return "exclaim";
      case '?':
        return "ask";
      default:
        return "say";
    }
  }
} // namespace

ACMD(do_say)
{
  std::string text(skip_spaces(argument));

  if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
    if (char_get_in_room(ch) > 160) {
      send_to_char(g, ch, "You are mute and unable to talk though.\r\n");
      return;
    }
    send_to_char(g, ch, "You are mute and unable to talk though. You will be allowed to just for MUD School.");
  }

  if (text.empty()) {
    send_to_char(g, ch, "Yes, but WHAT do you want to say?\r\n");
    return;
  }

  std::string_view verb      = say_verb(text);
  struct char_data *sch = addressed(g, ch, text);
  if (!sch) {
    act(g, str_format("@w$n @W{}s, '@C{}@W'@n", verb, text), true, ch, nullptr, nullptr, act_to::room);
  } else {
    act(g, str_format("@w$n @Wsays to @gyou@W, '@C{}@W'@n", text), true, ch, nullptr, sch, act_to::vict);
    act(g, str_format("@w$n @Wsays to @g$N@W, '@C{}@W'@n", text), true, ch, nullptr, sch, act_to::notvict);
  }

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, pref_flag::norepeat)) {
    send_to_char(g, ch, CONFIG_OK);
  } else {
    std::string said = str_format("@WYou {}, '@C{}@W'@n\r\n", verb, text);
    send_to_char(g, ch, said);
    add_history(g, ch, said, HIST_SAY);
    /* quirk: with norepeat on, Shenron doesn't hear wishes */
    if (!shenron_hears(g, ch, text))
      return;
  }

  /* trigger check */
  speech_mtrigger(g, ch, text);
  speech_wtrigger(g, ch, text);
  if (!g->SHENRON || char_get_in_room(ch) != g->DRAGONR)
    mob_talk(g, ch, text.c_str());
}

namespace {
  /* One of ch's group hears what ch told it */
  void tell_group_member(GameWorld *g, struct char_data *ch, struct char_data *who, std::string &said, const std::string &as_act, bool garbled)
  {
    if (!char_aff_flagged(who, aff_flag::group) || who == ch || !char_is_awake(who))
      return;
    if (!CONFIG_ENABLE_LANGUAGES) {
      act(g, as_act, true, ch, nullptr, who, garbled ? act_to::vict | act_option::even_asleep : act_to::vict);
      return;
    }
    /* quirk: the garbling of what's said piles up, from each follower to the next */
    if (garbled && !char_is_npc(ch) && !char_is_npc(who))
      garble_text(g, said, char_get_skill(who, char_get_speaking(ch)), char_get_speaking(ch));
    /* quirk: the leader hears it in green, the followers in white */
    send_to_char(g, who, "{}@W tells the group{} @W'{}{}@W'@n\r\n", char_can_see(g, who, ch) ? char_get_name(ch) : "Someone", tongue(who, ch), garbled ? "" : "@G", said);
  }
} // namespace

ACMD(do_gsay)
{
  argument = skip_spaces(argument);

  if (!char_aff_flagged(ch, aff_flag::group)) {
    send_to_char(g, ch, "But you are not the member of a group!\r\n");
    return;
  }
  if (char_in_arena(g, ch)) {
    send_to_char(g, ch, "Lol, no.\r\n");
    return;
  }
  if (argument.empty()) {
    send_to_char(g, ch, "Yes, but WHAT do you want to group-say?\r\n");
    return;
  }

  struct char_data *leader = char_get_master(g, ch) ? char_get_master(g, ch) : ch;
  std::string said(argument);
  std::string as_act = str_format("$n@W tells the group @W'@G{}@W'@n\r\n", said);

  tell_group_member(g, ch, leader, said, as_act, false);
  for (char_data *fol : char_followers(g, leader))
    tell_group_member(g, ch, fol, said, as_act, true);

  if (char_prf_flagged(g, ch, pref_flag::norepeat))
    send_to_char(g, ch, CONFIG_OK);
  else
    send_to_char(g, ch, "@WYou tell the group, '@G{}@W'@n\r\n", argument);
}

namespace {
  void perform_tell(GameWorld *g, struct char_data *ch, struct char_data *vict, std::string_view arg)
  {
    std::string heard;
    if (CONFIG_ENABLE_LANGUAGES)
      heard = str_format("@[13]{} tells you{} '{}@[13]'@n\r\n", char_can_see(g, vict, ch) ? char_get_name(ch) : "Someone", tongue(vict, ch), arg);
    else if (char_is_npc(ch))
      heard = str_format("@Y{}@Y tells you '{}'@n\r\n", char_get_name(ch), arg);
    else if (char_get_admlevel(vict) < 1)
      heard = str_format("@Y{}@Y tells you '{}'@n\r\n", public_name(ch), arg);
    else
      heard = str_format("@Y{}({})@Y tells you '{}'@n\r\n", char_get_user(ch), char_get_name(ch), arg);
    send_to_char(g, vict, heard);
    /* a mob's tell is kept in the history only when languages are on */
    if (CONFIG_ENABLE_LANGUAGES || !char_is_npc(ch))
      add_history(g, vict, heard, HIST_TELL);

    if (char_is_npc(ch) || char_prf_flagged(g, ch, pref_flag::norepeat)) {
      send_to_char(g, ch, CONFIG_OK);
    } else {
      std::string told = str_format("@YYou tell {}, '{}'@n\r\n", public_name(vict), arg);
      if (char_get_admlevel(ch) < 5 && char_get_admlevel(vict) < 5 && !char_is_npc(vict))
        send_to_imm(g, "@GTELL: @C{}@G tells @c{}, @W'@w{}@W'@n", public_name(ch), public_name(vict), arg);
      send_to_char(g, ch, told);
      add_history(g, ch, told, HIST_TELL);
    }

    if (!char_is_npc(vict) && !char_is_npc(ch))
      char_set_last_tell(g, vict, char_get_idnum(ch));
  }

  /* Whether ch may tell vict something, saying why not */
  bool is_tell_ok(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    bool mortal = char_get_admlevel(ch) < 1, to_mortal = char_get_admlevel(vict) < 1, player = !char_is_npc(ch);

    if (ch == vict)
      send_to_char(g, ch, "You try to tell yourself something.\r\n");
    else if (player && char_get_level(ch) < 3 && to_mortal)
      send_to_char(g, ch, "You need to be level 3 or higher to send or receive tells");
    else if (player && char_prf_flagged(g, ch, pref_flag::afk) && mortal)
      send_to_char(g, ch, "You can't send tells when AFK.\r\n");
    else if (player && char_prf_flagged(g, vict, pref_flag::afk)) {
      if (!mortal) /* an immortal reaches them anyway */
        return true;
      send_to_char(g, ch, "They are AFK right now, try later.\r\n");
    }
    else if (!char_is_npc(vict) && char_get_level(vict) < 3 && mortal)
      send_to_char(g, ch, "They need to be level 3 or higher to send or receive tells");
    else if (player && char_prf_flagged(g, ch, pref_flag::notell) && to_mortal)
      send_to_char(g, ch, "You can't tell other people while you have notell on.\r\n");
    else if (room_flagged(g, char_get_in_room(ch), room_flag::soundproof) && to_mortal)
      send_to_char(g, ch, "The walls seem to absorb your words.\r\n");
    else if (char_is_npc(vict))
      send_to_char(g, ch, "You can't send tells to mobs.\r\n");
    else if (!char_get_desc(vict)) /* linkless */
      act(g, "$E's linkless at the moment.", false, ch, nullptr, vict, act_to::actor | act_option::even_asleep);
    else if (char_plr_flagged(vict, player_flag::writing))
      act(g, "$E's writing a message right now; try again later.", false, ch, nullptr, vict, act_to::actor | act_option::even_asleep);
    else if ((mortal && char_prf_flagged(g, vict, pref_flag::notell)) || room_flagged(g, char_get_in_room(vict), room_flag::soundproof))
      act(g, "$E can't hear you.", false, ch, nullptr, vict, act_to::actor | act_option::even_asleep);
    else
      return true;
    return false;
  }

  /* The player playing whose account is `name`, or has it in it, or an immortal whose name does; the first found */
  struct char_data *player_by_account(GameWorld *g, const std::string &name)
  {
    descriptor_data *d = find_first(each_desc(g), [&](descriptor_data *k) {
      if (desc_get_connected(k) != CON_PLAYING || char_is_npc(k->character) || k->acct.user.empty())
        return false;
      return str_eq_nocase(k->acct.user, name) || until_nul(k->acct.user).contains(until_nul(name)) ||
             ((str_eq_nocase(char_get_name(k->character), name) || char_get_name(k->character).contains(name)) && char_get_admlevel(k->character) > 0);
    });
    return d ? d->character : nullptr;
  }
} // namespace

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  std::string_view rest = argument;
  std::string who       = take_word(rest);
  std::string what(skip_spaces(rest));

  if (who.empty() || what.empty()) {
    send_to_char(g, ch, "Who do you wish to tell what??\r\n");
    return;
  }
  struct char_data *vict;
  if (!char_is_npc(ch)) {
    CAP(who);
    if (!(vict = player_by_account(g, who))) {
      send_to_char(g, ch, "No user around with that name.");
      return;
    }
  } else if (!(vict = get_player_vis(g, ch, who, nullptr, find_flag::char_world))) {
    send_to_char(g, ch, CONFIG_NOPERSON);
    return;
  }
  if (is_tell_ok(g, ch, vict))
    perform_tell(g, ch, vict, what.c_str());
}

ACMD(do_reply)
{
  if (char_is_npc(ch))
    return;
  if (room_flagged(g, char_get_in_room(ch), room_flag::hbtc)) {
    send_to_char(g, ch, "This is a different dimension!\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), room_flag::past)) {
    send_to_char(g, ch, "This is the past, you can't send tells!\r\n");
    return;
  }
  std::string text(skip_spaces(argument));
  if (char_get_last_tell(g, ch) == NOBODY) {
    send_to_char(g, ch, "You have nobody to reply to!\r\n");
    return;
  }
  if (text.empty()) {
    send_to_char(g, ch, "What is your reply?\r\n");
    return;
  }
  /* Found by idnum, so a reply reaches someone who has logged out and back in */
  struct char_data *tch = find_first(each_char(g), [&](char_data *c) { return !char_is_npc(c) && char_get_idnum(c) == char_get_last_tell(g, ch); });
  if (!tch)
    send_to_char(g, ch, "They are no longer playing.\r\n");
  else if (is_tell_ok(g, ch, tch))
    perform_tell(g, ch, tch, text.c_str());
}

namespace {
  struct quiet_talk {
    int subcmd;
    std::string_view sing, plur, others;
  };

  constexpr auto QUIET_TALKS = std::to_array<quiet_talk>({
      {SCMD_WHISPER, "whisper to", "whispers to", "$n whispers something to $N."              },
      {SCMD_ASK,     "ask",        "asks",        "$n asks $N a question."                    },
      {-1,           "oops",       "oopses",      "$n is tongue-tied trying to speak with $N."},
  });

  /* The letters lost from a whisper overheard so well, any of three ways */
  constexpr std::array<std::array<std::string_view, 3>, 3> MUFFLED = {{
      {"ahemopyjkdw", "erkmopyjkdw", "srcqluizt"},
      {"bfgvjk",     "dymhst",      "arno"     },
      {"qoig",       "aeio",        "kmb"      },
  }};

  /* What's overheard, to this level of 0 to 3; quirk: the whisper itself loses the letters, one listener after another */
  std::string_view overhear(GameWorld *g, std::string &buf, int level)
  {
    if (level == 3)
      return until_nul(buf); /* as its c_str() was read */
    const std::array<std::string_view, 3> &ways = MUFFLED[level];
    std::string_view lost   = rand_number(g, 1, 10) >= 5 ? ways[0] : rand_number(g, 1, 10) >= 5 ? ways[1] : ways[2];
    for (char c : lost) {
      search_replace(buf, std::string(1, c), "..");
      search_replace(buf, std::string(1, (char)toupper(c)), "..");
    }
    return until_nul(buf);
  }

  /* By how far a listener's skill falls short of the roll, how much they hear */
  struct hearing {
    int short_by;
    std::string_view how;
  };

  constexpr auto HEARINGS = std::to_array<hearing>({
      {0,  "everything"},
      {10, "a lot of"  },
      {20, "some of"   },
      {30, "little of" },
  });

  void handle_whisper(GameWorld *g, std::string &buf, struct char_data *ch, struct char_data *vict)
  {
    for (char_data *tch : room_people(g, char_get_in_room(ch))) {
      if (char_is_npc(tch) || tch == ch || tch == vict || !char_get_skill(tch, SKILL_LISTEN))
        continue;
      int skill = char_get_skill(tch, SKILL_LISTEN);
      int roll1 = rand_number(g, 10, 30);
      int roll  = rand_number(g, roll1, 110);
      auto heard = std::ranges::find_if(HEARINGS, [&](const hearing &h) { return skill + h.short_by >= roll; });
      if (heard == std::end(HEARINGS)) {
        send_to_char(g, tch, "@WYou were unable to overhear anything that was whispered.@n\r\n");
        continue;
      }
      if (heard == std::begin(HEARINGS))
        send_to_char(g, tch, "@WYou overhear everything whispered, @W'@m{}@W'@n\r\n", overhear(g, buf, 3));
      else
        send_to_char(g, tch, "@WYou overhear {} what is whispered, @W'@m{}@W'@n\r\n", heard->how, overhear(g, buf, 3 - (int)(heard - std::begin(HEARINGS))));
    }
  }
} // namespace

ACMD(do_spec_comm)
{
  if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
    send_to_char(g, ch, "You are mute and unable to talk though.\r\n");
    return;
  }
  const quiet_talk &how = *std::ranges::find_if(QUIET_TALKS, [&](const quiet_talk &t) { return t.subcmd == subcmd || t.subcmd < 0; });

  std::string_view rest = argument;
  std::string who       = take_word(rest);
  std::string what(skip_spaces(rest));

  if (who.empty() || what.empty()) {
    send_to_char(g, ch, "Whom do you want to {}.. and what??\r\n", how.sing);
    return;
  }
  struct char_data *vict = get_char_vis(g, ch, who, nullptr, find_flag::char_room);
  if (!vict) {
    send_to_char(g, ch, CONFIG_NOPERSON);
    return;
  }
  if (vict == ch) {
    send_to_char(g, ch, "You can't get your mouth close enough to your ear...\r\n");
    return;
  }

  std::string heard;
  if (CONFIG_ENABLE_LANGUAGES) {
    std::string garbled = what;
    garble_text(g, garbled, char_get_skill(vict, char_get_speaking(ch)), char_get_speaking(ch));
    heard = str_format("$n {} you{} '{}'", how.plur, tongue(vict, ch), garbled);
  } else {
    heard = str_format("@c$n @W{} you '@m{}@W'@n", how.plur, what);
  }
  act(g, heard, false, ch, nullptr, vict, act_to::vict);

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, pref_flag::norepeat))
    send_to_char(g, ch, CONFIG_OK);
  else
    act(g, str_format("@WYou {} @C$N@W, '@m{}@W'@n\r\n", how.sing, what), true, ch, nullptr, vict, act_to::actor);

  act(g, how.others, false, ch, nullptr, vict, act_to::notvict);
  if (subcmd == SCMD_WHISPER)
    handle_whisper(g, what, ch, vict);
}

namespace {
  struct obj_data *board_at_hand(GameWorld *g, struct char_data *ch)
  {
    auto is_board          = [&](obj_data *o) { return obj_get_type_flag(o) == ITEM_BOARD; };
    struct obj_data *board = find_first(char_inventory(g, ch), is_board);
    return board ? board : find_first(room_contents(g, char_get_in_room(ch)), is_board);
  }

  /* The paper and pen for write, from what was named and what's held; false if they can't be found, having said why */
  bool writing_things(GameWorld *g, struct char_data *ch, std::string &papername, std::string &penname, struct obj_data *&paper, struct obj_data *&pen)
  {
    if (!penname.empty()) { /* both named */
      if (!(paper = get_obj_in_list_vis(g, ch, papername, nullptr, char_get_inventory(ch)))) {
        send_to_char(g, ch, "You have no {}.\r\n", papername);
        return false;
      }
      if (!(pen = get_obj_in_list_vis(g, ch, penname, nullptr, char_get_inventory(ch)))) {
        send_to_char(g, ch, "You have no {}.\r\n", penname);
        return false;
      }
      return true;
    }
    /* one named, the other held */
    if (!(paper = get_obj_in_list_vis(g, ch, papername, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "There is no {} in your inventory.\r\n", papername);
      return false;
    }
    bool named_pen = obj_get_type_flag(paper) == ITEM_PEN;
    if (!named_pen && obj_get_type_flag(paper) != ITEM_NOTE) {
      send_to_char(g, ch, "That thing has nothing to do with writing.\r\n");
      return false;
    }
    struct obj_data *held = char_get_equipment(ch, WEAR_WIELD2);
    if (!held) {
      send_to_char(g, ch, "You can't write with {} {} alone.\r\n", AN(papername), papername);
      return false;
    }
    if (!char_can_see_obj(g, ch, held)) {
      send_to_char(g, ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
      return false;
    }
    if (named_pen) {
      pen   = paper;
      paper = held;
    } else {
      pen = held;
    }
    return true;
  }
} // namespace

ACMD(do_write)
{
  /* before we do anything, lets see if there's a board involved. */
  if (struct obj_data *board = board_at_hand(g, ch)) {
    write_board_message(g, obj_get_vnum(g, board), ch, argument);
    return;
  }

  std::string_view rest = argument;
  std::string papername = take_arg(rest);
  std::string penname   = take_arg(rest);

  if (!char_get_desc(ch))
    return;
  if (papername.empty()) {
    send_to_char(g, ch, "write on [what] with [what pen?]\r\n");
    return;
  }
  struct obj_data *paper = nullptr, *pen = nullptr;
  if (!writing_things(g, ch, papername, penname, paper, pen))
    return;
  if (obj_get_type_flag(pen) != ITEM_PEN) {
    act(g, "$p is no good for writing with.", false, ch, pen, nullptr, act_to::actor);
    return;
  }
  if (obj_get_type_flag(paper) != ITEM_NOTE) {
    act(g, "You can't write on $p.", false, ch, paper, nullptr, act_to::actor);
    return;
  }
  /* Something on it, display it as that's in input buffer. */
  if (!obj_get_action_description(paper).empty()) {
    send_to_char(g, ch, "There's something written on it already:\r\n");
    send_to_char(g, ch, "{}", obj_get_action_description(paper));
  }
  act(g, "$n begins to jot down a note.", true, ch, nullptr, nullptr, act_to::room);
  obj_extra_set(paper, extra_flag::unique_save, true);
  send_editor_help(g, char_get_desc(ch));
  string_write(char_get_desc(ch), obj_action_description_slot(paper), MAX_NOTE_LENGTH, 0);
}

ACMD(do_page)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string buf2(skip_spaces(rest));

  if (char_is_npc(ch)) {
    send_to_char(g, ch, "Monsters can't page.. go away.\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "Whom do you wish to page?\r\n");
    return;
  }
  std::string buf = str_format("\007\007*$n* {}", buf2);
  if (str_eq_nocase(arg, "all")) {
    if (!char_adm_flagged(ch, admin_flag::tellall)) {
      send_to_char(g, ch, "You will never be godly enough to do that!\r\n");
      return;
    }
    for (descriptor_data *d : each_desc(g))
      if (desc_get_connected(d) == CON_PLAYING && d->character)
        act(g, buf, false, ch, nullptr, d->character, act_to::vict);
    return;
  }
  struct char_data *vict = get_char_vis(g, ch, arg, nullptr, find_flag::char_world);
  if (!vict) {
    send_to_char(g, ch, "There is no such person in the game!\r\n");
    return;
  }
  act(g, buf, false, ch, nullptr, vict, act_to::vict);
  if (char_prf_flagged(g, ch, pref_flag::norepeat))
    send_to_char(g, ch, CONFIG_OK);
  else
    act(g, buf, false, ch, nullptr, vict, act_to::actor);
}

/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

namespace {
  /* A channel, by its subcommand: the preference that keeps it from being heard, its history, the messages for
     one who can't use it, its name and for one not on it, and its colour */
  struct channel {
    pref_flag off_flag;
    int history;
    std::string_view cant, name, not_on, colour;
  };

  constexpr auto CHANNELS = std::to_array<channel>({
      {pref_flag::nomusic, HIST_HOLLER,  "You cannot music!!\r\n",      "@D[@mMUSIC@D]", "You aren't even on the channel!\r\n",   "@[10]"},
      {pref_flag::deaf,    HIST_SHOUT,   "You cannot shout!!\r\n",      "shout",         "Turn off your noshout flag first!\r\n", "@[9]" },
      {pref_flag::nogoss,  HIST_GOSSIP,  "You cannot ooc!!\r\n",        "@D[@BOOC@D]",   "You aren't even on the channel!\r\n",   "@[10]"},
      {pref_flag::noauct,  HIST_AUCTION, "You cannot newbie!!\r\n",     "newbie",        "You aren't even on the channel!\r\n",   "@[11]"},
      {pref_flag::nogratz, HIST_GRATS,   "You cannot congratulate!\r\n", "congrat",      "You aren't even on the channel!\r\n",   "@[12]"},
      {pref_flag::brief,   -1,           "You cannot gemote!!\r\n",     "gemote",        "You aren't even on the channel!\r\n",   "@[10]"}, /* quirk: reads pref_flag::brief (0), meant as no flag */
  });

  /* Whether whoever is at i hears ch on the channel */
  bool hears(GameWorld *g, struct char_data *ch, descriptor_data *i, int subcmd)
  {
    struct char_data *who = i->character;
    if (desc_get_connected(i) != CON_PLAYING || i == char_get_desc(ch) || !who)
      return false;
    if (!char_is_npc(who) && (char_prf_flagged(g, who, CHANNELS[subcmd].off_flag) || char_plr_flagged(who, player_flag::writing)))
      return false;
    if (room_flagged(g, char_get_in_room(who), room_flag::soundproof))
      return false;
    /* a shout carries through the zone, to those awake */
    return subcmd != SCMD_SHOUT || (room_get_zone(g, char_get_in_room(ch)) == room_get_zone(g, char_get_in_room(who)) && char_is_awake(who));
  }

  /* What `who` hears of ch on the channel, as act() takes it */
  std::string heard_as(GameWorld *g, struct char_data *ch, struct char_data *who, int subcmd, std::string &text)
  {
    const channel &c = CHANNELS[subcmd];
    if (CONFIG_ENABLE_LANGUAGES) {
      /* quirk: the garbling of what's said piles up, from each listener to the next */
      garble_text(g, text, char_get_skill(who, char_get_speaking(ch)), char_get_speaking(ch));
      return str_format("{}{} {}s{} '{}@n'{}", c.colour, public_name(ch), c.name, tongue(who, ch), text, c.colour);
    }
    if (subcmd == SCMD_SHOUT && char_get_in_room(who) != char_get_in_room(ch))
      return str_format("{}@WSomeone nearby {}s@W, '@w{}@W'@n{}", c.colour, c.name, text, c.colour);
    if (subcmd == SCMD_SHOUT)
      return str_format("{}@W$n@W {}s@W, '@w{}@W'@n{}", c.colour, c.name, text, c.colour);
    if (char_get_admlevel(ch) > 0)
      return str_format("{}@W$n {}s@W, '@w{}@W'@n{}", c.colour, c.name, text, c.colour);
    if (!char_is_npc(ch) && char_prf_flagged(g, ch, pref_flag::hide) && ch != who) {
      if (char_get_admlevel(who) < ADMLVL_IMMORT)
        return str_format("{}@WAnonymous Player {}s@W, '@w{}@W'@n{}", c.colour, c.name, text, c.colour);
      return str_format("{}@W{}(H) {}s@W, '@w{}@W'@n{}", c.colour, char_get_admlevel(who) > 0 ? char_get_name(ch) : char_get_user(ch), c.name, text, c.colour);
    }
    if (char_get_admlevel(who) > 0)
      return str_format("{}@W{} ($n) {}s@W, '@w{}@W'@n{}", c.colour, char_get_user(ch), c.name, text, c.colour);
    return str_format("{}@W{} {}s@W, '@w{}@W'@n{}", c.colour, char_get_user(ch), c.name, text, c.colour);
  }

  /* Three channel lines in a row freeze a mortal, who is told so with everyone else */
  void count_spam(GameWorld *g, struct char_data *ch)
  {
    if (char_get_spam(ch) >= 3 && char_get_admlevel(ch) < 1) {
      send_to_imm(g, "SPAMMING: {} has been frozen for spamming!\r\n", char_get_name(ch));
      send_to_all(g, "@rSPAMMING@D: @C{}@w has been frozen for spamming, let that be a lesson to 'em.@n\r\n", char_get_name(ch));
      char_plr_set(ch, player_flag::frozen, true);
      char_set_freeze_level(g, ch, 1);
    } else if (char_get_spam(ch) < 3) {
      char_set_spam(ch, char_get_spam(ch) + 1);
    }
  }
} // namespace

ACMD(do_gen_comm)
{
  /* to keep pets, etc from being ordered to shout */
  if (!char_get_desc(ch))
    return;
  if (char_plr_flagged(ch, player_flag::noshout)) {
    send_to_char(g, ch, "{}", CHANNELS[subcmd].cant);
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), room_flag::soundproof)) {
    send_to_char(g, ch, "The walls seem to absorb your words.\r\n");
    return;
  }
  if (subcmd == SCMD_SHOUT && char_get_bonuses(ch, BONUS_MUTE) > 0) {
    send_to_char(g, ch, "You are mute and are incapable of speech.\r\n");
    return;
  }

  std::string text(skip_spaces(argument));
  if (subcmd == SCMD_GOSSIP && text[0] == '*')
    subcmd = SCMD_GEMOTE;
  if (subcmd == SCMD_GEMOTE) {
    do_gmote(g, ch, text[0] == '*' || text[0] == ':' ? std::string_view(text).substr(1) : std::string_view(text), 0, 1);
    return;
  }

  const channel &c = CHANNELS[subcmd];
  /* level_can_shout defined in config.c */
  if (char_get_level(ch) < CONFIG_LEVEL_CAN_SHOUT) {
    send_to_char(g, ch, "You must be at least level {:d} before you can {}.\r\n", CONFIG_LEVEL_CAN_SHOUT, c.name);
    return;
  }
  if (!char_is_npc(ch) && char_prf_flagged(g, ch, c.off_flag)) {
    send_to_char(g, ch, "{}", c.not_on);
    return;
  }
  if (text.empty()) {
    send_to_char(g, ch, "Yes, {}, fine, {} we must, but WHAT???\r\n", c.name, c.name);
    return;
  }
  delete_doubledollar(text);

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, pref_flag::norepeat)) {
    send_to_char(g, ch, CONFIG_OK);
  } else {
    std::string said = str_format("{}@WYou {}@W, '@w{}@W'{}@n\r\n", c.colour, c.name, text, c.colour);
    send_to_char(g, ch, said);
    add_history(g, ch, said, c.history);
  }

  for (descriptor_data *i : each_desc(g)) {
    if (!hears(g, ch, i, subcmd))
      continue;
    std::string heard = heard_as(g, ch, i->character, subcmd, text);
    /* hears() leaves only those act() sends to, so there is always a message */
    if (const std::string *sent = act(g, heard, false, ch, nullptr, i->character, act_to::vict | act_option::even_asleep))
      add_history(g, i->character, *sent, c.history);
  }
  count_spam(g, ch);
}

ACMD(do_respond)
{
  if (char_is_npc(ch)) {
    send_to_char(g, ch, "As a mob, you never bothered to learn to read or write.\r\n");
    return;
  }
  struct obj_data *board = board_at_hand(g, ch);
  if (!board) {
    send_to_char(g, ch, "Sorry, you may only reply to messages posted on a board.\r\n");
    return;
  }
  std::string_view rest = argument;
  std::string number    = take_arg(rest);
  if (number.empty()) {
    send_to_char(g, ch, "Respond to what?\r\n");
    return;
  }
  int mnum;
  if (!isdigit(number[0]) || !(mnum = parse_int(number))) {
    send_to_char(g, ch, "You must type the number of the message you wish to reply to.\r\n");
    return;
  }
  board_respond(g, obj_get_vnum(g, board), ch, mnum);
}
