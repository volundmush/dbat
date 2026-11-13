/* scouter.c: scouters, SNET and eavesdropping
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <climits>
#include <cstdlib>

#include <sys/stat.h>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.other.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/consts/auction.hpp"
#include "dbat/consts/aligns.hpp"
#include "dbat/utils.hpp"
#include "dbat/flags.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/random.hpp"
#include "dbat/affect.hpp"
#include "dbat/relocate.hpp"
#include "dbat/extract.hpp"
#include "dbat/races.hpp"
#include "dbat/fileop.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/combat.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/weather.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/graph.hpp"
#include "dbat/alias.hpp"
#include "dbat/spells.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/fight.hpp"
#include "dbat/class.hpp"
#include "dbat/shop.hpp"
#include "dbat/feats.hpp"
#include "dbat/guild.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/objsave.hpp"
#include "dbat/mail.hpp"
#include "dbat/game_world.hpp"

#include "other_internal.hpp"

namespace {
  constexpr const char *SENSELESS  = "Oh that makes a lot of sense...\r\n";
  constexpr const char *ARTIFICIAL = "You can't touch the mind of such an artificial being.\r\n";
  constexpr const char *MINDLESS   = "You can't touch the mind of such a thing.\r\n";

  /* What any use of telepathy costs */
  void telepathy_cost(GameWorld *g, struct char_data *ch)
  {
    decCurKI(g, ch, getMaxKI(g, ch) / 40, 0);
  }

  /* telepathy far: looking at the room vict is in through their eyes */
  void telepathy_far(GameWorld *g, struct char_data *ch, std::string &target)
  {
    if (char_is_npc(ch))
      return;
    auto found = find_char(g, ch, target, FIND_CHAR_WORLD, "Look through who's eyes?\r\n");
    if (reject(g, ch, found))
      return;
    struct char_data *vict = *found;
    const char *refusal    = nullptr;
    if (vict == ch)
      refusal = SENSELESS;
    else if (char_is_npc(vict))
      refusal = MINDLESS;
    else if (char_get_admlevel(vict) > char_get_admlevel(ch))
      refusal = "Their mental power oustrips your's by unfathomable measurements!\r\n";
    else if (char_aff_flagged(ch, AFF_SHOCKED))
      refusal = "Your mind has been shocked by telepathic feedback! You are not able to use telepathy right now.\r\n";
    else if (char_is_android(vict))
      refusal = ARTIFICIAL;
    else if (char_get_skill(vict, SKILL_TELEPATHY) +char_get_aff_abils_intel(vict) > char_get_skill(ch, SKILL_TELEPATHY) + char_get_aff_abils_intel(ch))
      refusal = "They throw off your attempt with their own telepathic abilities!\r\n";
    else if (char_get_in_room(ch) == char_get_in_room(vict))
      refusal = "They are in the same room as you!\r\n";
    else if (char_aff_flagged(vict, AFF_BLIND))
      refusal = "They are blind!\r\n";
    else if (char_plr_flagged(vict, PLR_EYEC))
      refusal = "Their eyes are closed!\r\n";
    if (refusal) {
      send_to_char(g, ch, refusal);
      return;
    }
    look_at_room(g, char_get_in_room(vict), ch, 0);
    send_to_char(g, ch, "You see all this through their eyes!\r\n");
    if (char_get_aff_abils_intel(vict) > char_get_aff_abils_intel(ch))
      send_to_char(g, ch, "You feel like someone was using your mind for something...\r\n");
    telepathy_cost(g, ch);
  }

  /* telepathy link: breaking the link ch has, or making one with vict */
  void telepathy_link(GameWorld *g, struct char_data *ch, std::string &target)
  {
    if (char_is_npc(ch))
      return;
    if (struct char_data *linked = char_get_mindlink(g, ch)) {
      act_triple(g, {"@CYou remove the link your mind had with @w$N.@n", "@w$n@C removes the link $s mind had with yours.@n", nullptr, true}, ch, nullptr, linked);
      char_set_mindlink(linked, nullptr);
      char_set_mindlink(ch, nullptr);
      char_set_linker(ch, 0);
      return;
    }
    auto found = find_char(g, ch, target, FIND_CHAR_WORLD, "Link with the mind of who?\r\n");
    if (reject(g, ch, found))
      return;
    struct char_data *vict = *found;
    const char *refusal    = nullptr;
    if (vict == ch)
      refusal = SENSELESS;
    else if (char_is_npc(vict))
      refusal = MINDLESS;
    else if (char_is_android(vict))
      refusal = ARTIFICIAL;
    else if (char_get_skill(vict, SKILL_TELEPATHY))
      refusal = "Kinda pointless when you are both telepathic huh?\r\n";
    else if (char_get_mindlink(g, vict))
      refusal = "Someone else is already telepathically linked with them.\r\n";
    if (refusal) {
      send_to_char(g, ch, refusal);
      return;
    }
    if (char_get_skill(ch, SKILL_TELEPATHY) < axion_dice(g, char_get_aff_abils_intel(vict) * 0.1)) {
      act(g, "@R$n@r tried to link $s mind with yours, but you manage to force a break in the link!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@R$N@r manages to sense the intrusion and with $S intelligence push you out!@n", false, ch, nullptr, vict, TO_CHAR);
      return;
    }
    act_triple(g, {"@CYou link your mind with @w$N.@n", "@w$n@C links $s mind with yours. You can speak your thoughts to $m with 'think'.@n", nullptr, true}, ch, nullptr, vict);
    send_to_char(g, vict, "@wIf this is undesirable, Try: meditate break@n\r\n");
    char_set_mindlink(vict, ch);
    char_set_mindlink(ch, vict);
    char_set_linker(ch, 1);
  }

  /* The word for an alignment: the first whose bound it is above */
  constexpr struct {
    int above;
    const char *word;
  } ALIGNMENTS[] = {
      {999, "Saint         "}, {750, "Extremely Good"}, {500, "Really Good   "}, {250, "Good          "},
      {100, "Pretty Good   "}, {50, "Sorta Good    "}, {-50, "Neutral       "}, {-100, "Sorta Evil    "},
      {-500, "Pretty Evil   "}, {-751, "Evil          "}, {INT_MIN, "Extremely Evil"},
  };

  /* telepathy read: what vict, in the room, is */
  void telepathy_read(GameWorld *g, struct char_data *ch, std::string &target)
  {
    auto found = find_char(g, ch, target, FIND_CHAR_ROOM, "Read the mind of who?\r\n");
    if (reject(g, ch, found))
      return;
    struct char_data *vict = *found;
    if (vict == ch) {
      send_to_char(g, ch, SENSELESS);
      return;
    }
    if (char_is_android(vict)) {
      send_to_char(g, ch, ARTIFICIAL);
      return;
    }
    if (axion_dice(g, 0) > char_get_skill(ch, SKILL_TELEPATHY)) {
      telepathy_cost(g, ch);
      act(g, "@wYou attempt to read $N's@w mind, but fail to see it clearly.@n", true, ch, nullptr, vict, TO_CHAR);
      if (rand_number(g, 1, 15) >= 14 && !char_aff_flagged(ch, AFF_SHOCKED)) {
        act(g, "@MYour mind has been shocked!@n", true, ch, nullptr, nullptr, TO_CHAR);
        char_aff_set(ch, AFF_SHOCKED, true);
      } else
        improve_skill(g, ch, SKILL_TELEPATHY, 0);
      return;
    }
    if (char_get_skill(vict, SKILL_TELEPATHY) >= char_get_skill(ch, SKILL_TELEPATHY) && rand_number(g, 1, 2) == 2) {
      telepathy_cost(g, ch);
      act_triple(g, {"@wYou fail to read @c$N's@w mind and they seemed to have noticed the attempt!@n", "@C$n@w attempts to read your mind, but you resist and force $m out!@n", nullptr, true}, ch, nullptr, vict);
      improve_skill(g, ch, SKILL_TELEPATHY, 0);
      return;
    }
    send_to_char(g, ch, "@wYou peer into their mind:\r\n");
    telepathy_cost(g, ch);
    send_to_char(g, ch, "@GName      @D: @W{}@n\r\n", char_get_name(vict));
    send_to_char(g, ch, "@GRace      @D: @W{}@n\r\n", char_true_race_name(vict));
    send_to_char(g, ch, "@GSensei    @D: @W{}@n\r\n", char_sensei_name(vict));
    send_to_char(g, ch, "@GStr       @D: @W{:d}@n\r\n", char_get_aff_abils_str(vict));
    send_to_char(g, ch, "@GCon       @D: @W{:d}@n\r\n", char_get_aff_abils_con(vict));
    send_to_char(g, ch, "@GInt       @D: @W{:d}@n\r\n", char_get_aff_abils_intel(vict));
    send_to_char(g, ch, "@GWis       @D: @W{:d}@n\r\n", char_get_aff_abils_wis(vict));
    send_to_char(g, ch, "@GSpd       @D: @W{:d}@n\r\n", char_get_aff_abils_cha(vict));
    send_to_char(g, ch, "@GAgi       @D: @W{:d}@n\r\n", char_get_aff_abils_dex(vict));
    send_to_char(g, ch, "@GZenni     @D: @W{}@n\r\n", add_commas(char_get_gold(vict)));
    send_to_char(g, ch, "@GBank Zenni@D: @W{}@n\r\n", add_commas(char_get_bank_gold(vict)));
    int alignment = char_get_alignment(vict);
    send_to_char(g, ch, "@GAlignment @D: @w{}@n\r\n", std::ranges::find_if(ALIGNMENTS, [&](const auto &a) { return alignment > a.above; })->word);
    improve_skill(g, ch, SKILL_TELEPATHY, 0);
  }

  /* telepathy (target) (message), or to whoever ch is linked with, all of what they typed */
  void telepathy_send(GameWorld *g, struct char_data *ch, std::string &target, std::string_view message, std::string_view argument)
  {
    struct char_data *vict = char_get_mindlink(g, ch);
    if (!vict) {
      auto found = find_char(g, ch, target, FIND_CHAR_WORLD, "Send your thoughts to who?\r\n");
      if (reject(g, ch, found))
        return;
      vict = *found;
    } else
      message = argument;
    if (vict == ch) {
      send_to_char(g, ch, SENSELESS);
      return;
    }
    if (char_is_android(vict)) {
      send_to_char(g, ch, ARTIFICIAL);
      return;
    }
    send_to_char(g, ch, "@WYou tell @c{}@W telepathically, @w'@C{}@w'@n\r\n", char_get_name(vict), message);
    send_to_char(g, vict, "@c{}@W talks to you telepathically, @w'@C{}@w'@n\r\n", char_get_name(ch), message);
    send_to_imm(g, "@GTELEPATHY: @C{}@G telepaths @c{}, @W'@w{}@W'@n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch), char_get_admlevel(vict) > 0 ? char_get_name(vict) : char_get_user(vict), message);
    telepathy_cost(g, ch);
  }
} // namespace

ACMD(do_telepathy)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string arg2(skip_spaces(rest));

  if (!know_skill(g, ch, SKILL_TELEPATHY))
    return;
  if (char_in_arena(g, ch)) {
    send_to_char(g, ch, "Lol, no.\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: telepathy [ read ] (target)\r\n        telepathy [ link ] (target)\r\n        telepathy [  far ] (target)\r\n        telepathy (target) (message)\r\n");
    return;
  }
  if (getCurKI(g, ch) < getMaxKI(g, ch) / 40) {
    send_to_char(g, ch, "You do not have enough ki to focus your mental abilities.\r\n");
    return;
  }
  if (arg == "far")
    telepathy_far(g, ch, arg2);
  else if (arg == "link")
    telepathy_link(g, ch, arg2);
  else if (arg == "read")
    telepathy_read(g, ch, arg2);
  else
    telepathy_send(g, ch, arg, arg2, argument);
}

ACMD(do_eavesdrop)
{
  std::string_view rest = argument;
  std::string buf       = take_arg(rest);

  if (char_get_listenroom(ch) > 0) {
    send_to_char(g, ch, "You stop eavesdropping.\r\n");
    char_set_listenroom(ch, 0);
    char_set_eavesdir(ch, -1);
    return;
  }
  if (buf.empty()) {
    send_to_char(g, ch, "In which direction would you like to eavesdrop?\r\n");
    return;
  }
  int dir = search_block(buf, dirs, false);
  if (dir < 0) {
    send_to_char(g, ch, "Which directions is that?\r\n");
    return;
  }
  if (!know_skill(g, ch, SKILL_EAVESDROP))
    return;
  struct room_direction_data *exit = char_get_exit(g, ch, dir);
  if (!exit)
    send_to_char(g, ch, "There is not a room there...\r\n");
  else if (IS_SET(exit->exit_info, EX_CLOSED) && exit_keyword(exit))
    send_to_char(g, ch, "The {} is closed.\r\n", fname(exit_keyword(exit)));
  else {
    char_set_listenroom(ch, exit->to_room);
    char_set_eavesdir(ch, dir);
    send_to_char(g, ch, "Okay.\r\n");
  }
}

/* Takes off the first thing ch can no longer wear: broken, or held by an arm they've lost */
void check_eq(GameWorld *g, struct char_data *ch)
{
  for (int i = 0; i < NUM_WEARS; i++) {
    struct obj_data *obj = char_get_equipment(ch, i);
    if (!obj)
      continue;
    const char *to_char = nullptr, *to_room = nullptr;
    if (obj_extra_flagged(obj, ITEM_BROKEN)) {
      to_char = "@W$p@W falls apart and you remove it.@n";
      to_room = "@W$p@W falls apart and @C$n@W remove it.@n";
    } else if (obj == char_get_equipment(ch, WEAR_WIELD1) && char_get_limb_condition(ch, 0) <= 0) {
      to_char = "@WWithout your right arm you let go of @c$p@W!@n";
      to_room = "@C$n@W lets go of @c$p@W!@n";
    } else if (obj == char_get_equipment(ch, WEAR_WIELD2) && char_get_limb_condition(ch, 1) <= 0) {
      to_char = "@WWithout your left arm you let go of @c$p@W!@n";
      to_room = "@C$n@W lets go of @c$p@W!@n";
    } else
      continue;
    act(g, to_char, false, ch, obj, nullptr, TO_CHAR);
    act(g, to_room, false, ch, obj, nullptr, TO_ROOM);
    perform_remove(g, ch, i);
    return;
  }
}

namespace {
  bool has_scanner(GameWorld *g, struct char_data *ch)
  {
    return std::ranges::any_of(char_inventory(g, ch), [&](obj_data *obj) { return obj_get_vnum(g, obj) == 13600; });
  }

  /* Where a scouter's message can't come from */
  const char *snet_blocked(GameWorld *g, struct char_data *ch)
  {
    room_vnum room = char_get_in_room(ch);
    if (room_flagged(g, room, ROOM_HBTC))
      return "This is a different dimension!\r\n";
    if (char_in_arena(g, ch))
      return "Lol, no.\r\n";
    if (room_flagged(g, room, ROOM_PAST))
      return "This is the past, you can't talk on scouter net!\r\n";
    if (room_flagged(g, room, ROOM_HELL))
      return "The fire eats your transmission!\r\n";
    if (room >= 19800 && room <= 19899)
      return "Your signal will not be able to escape the walls of the pocket dimension.\r\n";
    return nullptr;
  }

  /* Whether a mortal with a scouter is where ch's message can reach them */
  bool snet_reaches(GameWorld *g, struct char_data *ch, struct descriptor_data *d)
  {
    room_vnum from = char_get_in_room(ch), there = char_get_in_room(d->character);
    if (desc_get_connected(d) != CON_PLAYING || there == from)
      return false;
    if (room_flagged(g, there, ROOM_HBTC) || room_flagged(g, there, ROOM_PAST))
      return false;
    /* the afterlife only hears the afterlife, and the living only the living */
    if (room_flagged(g, there, ROOM_RHELL) != room_flagged(g, from, ROOM_RHELL) || room_flagged(g, there, ROOM_AL) != room_flagged(g, from, ROOM_AL))
      return false;
    if (char_get_position(d->character) == POS_SLEEPING)
      return false;
    return !(there >= 19800 && there <= 19899);
  }
} // namespace

ACMD(do_snet)
{
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string arg2(skip_spaces(rest));

  if (const char *blocked = snet_blocked(g, ch)) {
    send_to_char(g, ch, blocked);
    return;
  }
  struct obj_data *obj = char_get_equipment(ch, WEAR_EYE);
  if (!obj) {
    send_to_char(g, ch, "You do not have a scouter on.\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "[Syntax] snet < [1-999] | check | #(scouter number) | * message | message>\r\n");
    return;
  }

  int call = -1;
  if (arg.find('#') != std::string::npos) {
    std::erase(arg, '#');
    call = parse_int(arg);
    if (call <= -1) {
      send_to_char(g, ch, "Call what personal scouter number?\r\n");
      return;
    }
  }
  if (str_eq_nocase(arg, "check")) {
    send_to_char(g, ch, "Your personal scouter number is: {:d}\r\n", char_get_id(ch));
    return;
  }
  int channel = call <= -1 ? parse_int(arg) : 0;
  if (channel > 0) {
    obj_set_scoutfreq(obj, std::min(channel, 999));
    act(g, "@wYou push some buttons on $p@w and change its channel.", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@w pushes some buttons on $p@w and changes its channel.", true, ch, obj, nullptr, TO_ROOM);
    return;
  }
  if (char_get_bonuses(ch, BONUS_MUTE) > 0) {
    send_to_char(g, ch, "You are unable to speak though.\r\n");
    return;
  }
  if (obj_get_scoutfreq(obj) == 0)
    obj_set_scoutfreq(obj, 1);

  /* said on the scouter's frequency, broadcast to all of them ("*"), or a call to one scouter by its number */
  enum { BAND, BROADCAST, CALL } kind = call > -1 ? CALL : str_eq_nocase(arg, "*") ? BROADCAST : BAND;
  int freq                            = obj_get_scoutfreq(obj);
  std::string voice                   = char_get_voice(ch) ? char_get_voice(ch) : "A generic voice";
  /* quirk: CAP() capitalises arg itself, so on a band ch's own echo below is capitalised only if someone heard it */
  auto said = [&]() -> std::string { return kind == BAND ? str_format("{} {}", CAP(arg), arg2) : CAP(arg2); };
  std::string band      = str_format("@D[@WSNET FREQ@D: @Y{:d}@D]", freq);
  std::string broadcast = str_format("@D[@WSNET FREQ@D: @Y{:d} @mBroadcast@D]", freq);
  bool reached          = false;

  for (descriptor_data *d : each_desc(g)) {
    struct char_data *who = d->character;
    if (!who || who == ch)
      continue;
    /* It will always be heard and logged for admin. */
    if (char_get_admlevel(who) > 0) {
      std::string tag  = kind == BAND ? band : kind == BROADCAST ? broadcast : str_format("@D[@WCall to @R#@Y{:d}@D]", call);
      std::string hist = str_format("@C{} ({}) is heard, {} @G{}@n\r\n", voice, char_get_name(ch), tag, said());
      send_to_char(g, who, hist);
      add_history(g, who, hist.c_str(), HIST_SNET);
      continue;
    }
    if (!snet_reaches(g, ch, d))
      continue;
    struct obj_data *theirs = char_get_equipment(who, WEAR_EYE);
    if (!theirs)
      continue;
    if (obj_get_scoutfreq(theirs) == 0)
      obj_set_scoutfreq(theirs, 1);
    std::string tag;
    if (kind == BAND && obj_get_scoutfreq(theirs) == freq)
      tag = band;
    else if (kind == BROADCAST)
      tag = broadcast;
    else if (kind == CALL && char_get_id(who) == call) {
      tag     = str_format("@D[@R#@W{:d} @Ycalling YOU@D]", char_get_id(ch));
      reached = true;
    } else
      continue;
    std::string hist = str_format("@C{} is heard @W(@c{}@W), {} @G{}@n\r\n", voice, readIntro(who, ch) == 1 ? get_i_name(who, ch) : "Unknown", tag, said());
    send_to_char(g, who, "{}", hist);
    add_history(g, who, hist.c_str(), HIST_SNET);
    if (has_scanner(g, who))
      send_to_char(g, who, "@WScanner@D: @Y{}@n\r\n", sense_location(g, ch));
  }

  reveal_hiding(g, ch, 3);
  std::string hist;
  if (kind == BAND)
    hist = str_format("@CYou {} @G{} {}@n\r\n", band, arg, arg2);
  else if (kind == BROADCAST)
    hist = str_format("@CYou {} @G{}@n\r\n", broadcast, CAP(arg2));
  else
    hist = str_format("@CYou call @D[@R#@W{:d}@D] @G{}@n\r\n", call, CAP(arg2));
  send_to_char(g, ch, "{}", hist);
  add_history(g, ch, hist.c_str(), HIST_SNET);
  act(g, str_format("@C$n@W says into $s scouter, '@G@G{}@W'@n\r\n", said()).c_str(), true, ch, nullptr, nullptr, TO_ROOM);
  if (kind == CALL) {
    if (!reached)
      send_to_char(g, ch, "@mThe transmission didn't reach them.@n\r\n");
  } else if (room_flagged(g, char_get_in_room(ch), ROOM_RHELL) || room_flagged(g, char_get_in_room(ch), ROOM_AL))
    send_to_char(g, ch, "@mThe transmission only reaches those who are in the afterlife.@n\r\n");
}

namespace {
  /* What each kind of scouter can't read without overloading */
  constexpr struct {
    int flag;
    int64_t limit;
  } SCOUTER_LIMITS[] = {{ITEM_BSCOUTER, 150000}, {ITEM_MSCOUTER, 5000000}, {ITEM_ASCOUTER, 15000000}};

  bool beyond_scouter(GameWorld *g, struct obj_data *scouter, struct char_data *vict)
  {
    return std::ranges::any_of(SCOUTER_LIMITS, [&](const auto &s) { return obj_extra_flagged(scouter, s.flag) && getCurPL(g, vict) >= s.limit; });
  }

  /* The lines of a scouter's reading */
  constexpr const char *READING_EDGE  = "@D|@1                                  @n@D|@n\r\n";
  constexpr const char *READING_PL    = "@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D:";
  constexpr const char *READING_KI    = "@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D:";
  constexpr const char *READING_ST    = "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D:";
  constexpr const char *READING_EXTRA = "@D|@1@GE@g@1x@Gt@g@1r@Ga I@g@1nf@Go @D:";

  void reading_top(GameWorld *g, struct char_data *ch)
  {
    send_to_char(g, ch, "@D,==================================|@n\r\n");
    send_to_char(g, ch, READING_EDGE);
    send_to_char(g, ch, "@D|@1@RReading target...                 @n@D|@n\r\n");
    send_to_char(g, ch, READING_EDGE);
  }

  void reading_line(GameWorld *g, struct char_data *ch, const char *label, std::string_view value)
  {
    send_to_char(g, ch, "{} @Y{:>21}@n@D|@n\r\n", label, value);
  }

  void reading_error(GameWorld *g, struct char_data *ch, const char *label)
  {
    send_to_char(g, ch, "{}                 @RERROR@n@D|@n\r\n", label);
  }

  /* How tired a stamina, in percent, looks: the first it's below */
  constexpr struct {
    long double below;
    const char *word;
  } STAMINAS[] = {{10, "Exhausted"}, {25, "Extremely Tired"}, {50, "Very Tired"}, {75, "Tired"}, {90, "Winded"}, {100, "Untired"}};

  /* What a scouter notices about someone beyond their numbers */
  constexpr struct {
    int aff; /* or -1 for the PLR_SELFD flag */
    const char *what;
  } NOTICED[] = {{AFF_ZANZOKEN, "Zanzoken Prepared"}, {AFF_HASS, "Accelerated Arms"}, {AFF_HEALGLOW, "Healing Glow Prepared"}, {AFF_POISON, "Poisoned"}, {-1, "Explosive Energy"}};

  /* scouter scan: everyone on ch's planet, and the way to them or where they are */
  void scouter_scan(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    int count = 0;
    for (descriptor_data *d : each_desc(g)) {
      struct char_data *who = d->character;
      if (desc_get_connected(d) != CON_PLAYING || who == ch || char_is_android(who) || !planet_check(g, ch, who))
        continue;
      std::string pathway;
      switch (int dir = find_first_step(g, char_get_in_room(ch), char_get_in_room(who))) {
        case BFS_ERROR:
          pathway = "@rERROR";
          break;
        case BFS_ALREADY_THERE:
          pathway = "@RHERE";
          break;
        case BFS_NO_PATH: /* quirk: these two are sent at once, and the reading after gets no way at all */
          send_to_char(g, ch, "@MUNKNOWN");
          break;
        default:
          send_to_char(g, ch, "@G{}\r\n", dirs[dir]);
          break;
      }
      bool same_zone    = char_in_zone(g, ch) == char_in_zone(g, who);
      const char *where = sense_location(g, who);
      if (beyond_scouter(g, obj, who))
        send_to_char(g, ch, "@D<@GPowerlevel Detected@D:@w ?????????@D> @w---> @C{}@n\r\n", same_zone ? pathway.c_str() : where);
      else
        send_to_char(g, ch, "@D<@GPowerlevel Detected@D: [@Y{}@D]@w ---> @C{}@n\r\n", add_commas(getCurPL(g, who)), same_zone ? pathway.c_str() : where);
      ++count;
    }
    if (count == 0)
      send_to_char(g, ch, "You didn't detect anyone of notice.\r\n");
    else
      send_to_char(g, ch, "{:d} powerlevels detected.\r\n", count);
  }
} // namespace

ACMD(do_scouter)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no available arms!\r\n");
    return;
  }
  struct obj_data *obj = char_get_equipment(ch, WEAR_EYE);
  if (!obj) {
    send_to_char(g, ch, "You do not even have a scouter!");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "[Syntax] scouter < target | scan>\r\n");
    return;
  }
  reveal_hiding(g, ch, 3);
  if (str_eq_nocase("scan", arg)) {
    scouter_scan(g, ch, obj);
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "They don't seem to be here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  constexpr act_msgs POINTED{nullptr, "$n points $s scouter at you.", "$n points $s scouter at $N."};

  if (char_is_android(vict)) {
    act_triple(g, POINTED, ch, nullptr, vict);
    reading_top(g, ch);
    reading_error(g, ch, READING_PL);
    reading_error(g, ch, READING_KI);
    reading_error(g, ch, READING_ST);
    send_to_char(g, ch, READING_EDGE);
    reading_error(g, ch, READING_EXTRA);
    send_to_char(g, ch, READING_EDGE);
    send_to_char(g, ch, "@D`==================================|@n\r\n");
    return;
  }
  if (beyond_scouter(g, obj, vict)) {
    act_triple(g, POINTED, ch, nullptr, vict);
    perform_remove(g, ch, WEAR_EYE);
    send_to_char(g, ch, "Your scouter overloads and explodes!\r\n");
    act(g, "$n's scouter explodes!", false, ch, nullptr, nullptr, TO_ROOM);
    extract_obj(g, obj);
    save_char(g, ch);
    return;
  }
  long double percent = (long double)std::max<int64_t>(getCurST(g, vict), 1) / (long double)std::max<int64_t>(getMaxST(g, vict), 1) * 100;
  act_triple(g, POINTED, ch, nullptr, vict);
  reading_top(g, ch);
  reading_line(g, ch, READING_PL, add_commas(getCurPL(g, vict)));
  if (!char_is_npc(vict))
    reading_line(g, ch, READING_KI, add_commas(char_get_charge(vict)));
  else
    reading_line(g, ch, READING_KI, add_commas(char_get_mobcharge(vict) * rand_number(g, char_get_level(ch) * 50, char_get_level(ch) * 200)));
  auto tiredness = std::ranges::find_if(STAMINAS, [&](const auto &s) { return percent < s.below; });
  reading_line(g, ch, READING_ST, tiredness == std::end(STAMINAS) ? "Energetic" : tiredness->word);
  send_to_char(g, ch, READING_EDGE);
  send_to_char(g, ch, "{} ", READING_EXTRA);
  bool noticed = false;
  for (const auto &n : NOTICED) {
    if (n.aff >= 0 ? !char_aff_flagged(vict, n.aff) : !char_plr_flagged(vict, PLR_SELFD))
      continue;
    send_to_char(g, ch, "{}@Y{:>21}@n@D|@n\n", noticed ? "@D|@1             " : "", n.what);
    noticed = true;
  }
  if (!noticed)
    send_to_char(g, ch, "@Y{:>21}@n@D|@n\n", "None Detected.");
  send_to_char(g, ch, READING_EDGE);
  send_to_char(g, ch, "@D`==================================|@n\r\n");
}
