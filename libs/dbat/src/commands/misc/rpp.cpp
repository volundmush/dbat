/* ************************************************************************
*  File: misc/rpp.c                                    Part of DBAT       *
*  Usage: RPP rewards, store and preferences                              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                   ---                                   *
*  This is an original file created by me for Dragonball Advent Truth     *
*  original credits maintained where relevant for act.other.c as this is  *
*  practically an act.other.c part two - Iovan 3/20/2011                  *
************************************************************************ */
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/consts/fish.hpp"
#include "dbat/consts/recipes.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/random.hpp"
#include "dbat/affect.hpp"
#include "dbat/search.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/extract.hpp"
#include "dbat/relocate.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/db.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/spells.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/combat.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/fight.hpp"
#include "dbat/class.hpp"
#include "dbat/act.item.hpp"
#include "dbat/game_world.hpp"

#include <algorithm>
#include <span>

#include "misc.hpp"

namespace {
  constexpr const char *INSTILL_SYNTAX = "Syntax: instill (token) (target)\r\n";

  /* The specializations a player may choose, once */
  struct specialization {
    const char *name;
    int preference;
    const char *chosen;
  };
  constexpr specialization SPECIALIZATIONS[] = {
    {"throw", PREFERENCE_THROWING, "You will now favor throwing weapons as fighting specialization. You're sure to nail it.\r\n"},
    {"hand", PREFERENCE_H2H, "You will now favor your body as your fighting specialization. Your body is ready.\r\n"},
    {"ki", PREFERENCE_KI, "You will now favor your ki energy as your fighting specialization. I expect more than a few smoldering craters.\r\n"},
    {"weapon", PREFERENCE_WEAPON, "You will now favor your weapons as your fighting specialization. Let the blood fly!\r\n"},
  };

  /* What the RPP item store sells, by choice number */
  struct rpp_item {
    int cost;
    int weight;        /* that must still fit in ch's carrying */
    int count;         /* items, likewise */
    int min_level;
    obj_vnum first;    /* the vnums handed over, first to last... */
    obj_vnum last;
    obj_vnum doubled;  /* ...two of each from this one on; 0: none */
    int heavy;         /* the item's weight set to this; 0: as it is */
    bool bank_wording; /* quirk: the receipt says the RPP came from the Bank */
    bool crowded_receipt; /* quirk: carrying too many items sends the receipt instead */
  };
  constexpr rpp_item RPP_STORE[] = {
    {20, 26, 13, 50, 1110, 1119, 1117, 0, false, false},     /* Stardust Equipment Set */
    {5, 2, 1, 40, 1120, 1120, 0, 0, true, false},            /* Platinum Masamune */
    {5, 2, 1, 40, 1121, 1121, 0, 0, false, false},           /* Obsidian Dirk */
    {5, 2, 1, 40, 1122, 1122, 0, 0, false, false},           /* Emerald Javelin */
    {5, 2, 1, 40, 1123, 1123, 0, 0, false, true},            /* Ivory Cane */
    {5, 2, 1, 40, 1124, 1124, 0, 0, false, false},           /* Hyper X65 Cannon */
    {5, 2, 1, 40, 1125, 1125, 0, 0, false, false},           /* Jagged Rock */
    {8, 10000000, 1, 0, 1126, 1126, 0, 10000000, false, false}, /* Kachin Mountain */
    {15, 2, 1, 0, 1127, 1127, 0, 0, false, false},           /* Spar Booster */
  };

  void receipt(GameWorld *g, struct char_data *ch, const rpp_item &item)
  {
    if (item.bank_wording)
      send_to_char(g, ch, "@R{:d}@W RPP from your Bank paid for your selection. Enjoy!@n\r\n", item.cost);
    else
      send_to_char(g, ch, "@R{:d}@W RPP paid for your selection. Enjoy!@n\r\n", item.cost);
  }

  void hand_over(GameWorld *g, struct char_data *ch, obj_vnum vnum, int heavy)
  {
    struct obj_data *obj = read_object(g, vnum);
    if (heavy)
      obj_set_weight(obj, heavy);
    obj_to_char(g, obj, ch);
    obj_set_size(obj, get_size(ch));
  }
} // namespace

ACMD(do_preference)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: preference (throw | weapon | hand | ki)\r\n");
    return;
  }
  if (char_get_preference(ch) > 0) {
    send_to_char(g, ch, "You've already chosen a specialization. No going back.\r\n");
    return;
  }

  auto found = std::ranges::find_if(SPECIALIZATIONS, [&](const specialization &s) { return str_eq_nocase(arg, s.name); });
  if (found == std::end(SPECIALIZATIONS)) {
    send_to_char(g, ch, "Syntax: preference (throw | weapon | hand | ki)\r\n");
    return;
  }
  send_to_char(g, ch, found->chosen);
  char_set_preference(ch, found->preference);
  if (found->preference != PREFERENCE_THROWING)
    return;
  /* Throwers get better at it */
  if (char_get_skills_base(ch, SKILL_THROW) <= 90)
    char_set_skills_base(ch, SKILL_THROW, char_get_skills_base(ch, SKILL_THROW) + 10);
  else if (char_get_skills_base(ch, SKILL_THROW) < 100)
    char_set_skills_base(ch, SKILL_THROW, 100);
}

void rpp_feature(GameWorld *g, struct char_data *ch, const std::string &arg)
{
  constexpr int cost = 2;

  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: rpp 13 (description)\nExample: rpp 13 a large red scar on his face\nDisplayed to others: He has a large red scar on his face.\r\n");
    return;
  }
  if (arg.size() > 60) {
    send_to_char(g, ch, "Please limit it to 60 characters.\r\n");
    return;
  }
  if (cost > char_get_rbank(ch)) {
    send_to_char(g, ch, "You do not have enough RPP in your Bank for that!\r\n");
    return;
  }

  bool change      = char_get_feature(ch);
  const char *sex  = char_get_sex(ch) == SEX_FEMALE ? "She" : char_get_sex(ch) == SEX_MALE ? "He" : "It";
  std::string line = str_format("...{} has {}.", sex, arg);
  char_set_rbank(ch, char_get_rbank(ch) - cost);
  send_to_char(g, ch, "@R{:d}@W RPP paid for your selection. Enjoy!@n\r\n", cost);
  send_to_char(g, ch, "You now have the following line underneath you when someone sees you in a room as:\n@C{}@n\r\n", line);
  char_set_feature(ch, line.c_str());
  if (change) {
    send_to_imm(g, "{} has altered their extra description. Make sure the reason is legit! If it is then reimb them 2 RPP.\r\n", char_get_user(ch));
    send_to_char(g, ch, "The immortals have been notified about this change. It had better have been for a good reason.\r\n");
  }
  mud_log("{} RPP Feature: '{}' Check for rule compliance.", char_get_user(ch), line);
}

ACMD(do_instill)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, INSTILL_SYNTAX);
    return;
  }
  auto found_token = find_carried(g, ch, arg, INSTILL_SYNTAX);
  if (reject(g, ch, found_token))
    return;
  auto found_obj = find_carried(g, ch, arg2, INSTILL_SYNTAX);
  if (reject(g, ch, found_obj))
    return;
  struct obj_data *token = *found_token, *obj = *found_obj;

  if (!obj_extra_flagged(token, ITEM_TOKEN)) {
    send_to_char(g, ch, "That is not a token.\r\n");
    return;
  }
  if (obj_extra_flagged(token, ITEM_FORGED)) {
    send_to_char(g, ch, "That token is a forgery!\r\n");
    return;
  }
  if (!wearable_obj(obj)) {
    send_to_char(g, ch, "You can only instill tokens into equipment.\r\n");
    return;
  }
  if (!obj_extra_flagged(obj, ITEM_SLOT1) && !obj_extra_flagged(obj, ITEM_SLOT2)) {
    send_to_char(g, ch, "That piece of equipment does not have any slots.\r\n");
    return;
  }
  if (obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "That piece of equipment has already had its token slots filled. This can not be reversed.");
    return;
  }

  std::span<obj_affected_type> affects{obj_get_affected(obj), MAX_OBJ_AFFECT};
  int stat     = obj_get_affected(token)[0].location;
  auto same    = std::ranges::find(affects, stat, &obj_affected_type::location);
  auto empty   = std::ranges::find(affects, 0, &obj_affected_type::location);
  if (same == affects.end() && empty == affects.end()) {
    send_to_char(g, ch, "This already has as many different stats as it can hold.\r\n");
    return;
  }

  act(g, "@GYou instill the token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", true, ch, obj, nullptr, TO_CHAR);
  act(g, "@g$n@G instills a token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", true, ch, obj, nullptr, TO_ROOM);
  int raise = obj_get_affected(token)[0].modifier;
  extract_obj(g, token);

  if (obj_extra_flagged(obj, ITEM_SLOT1))
    obj_extra_set(obj, ITEM_SLOTS_FILLED, true);
  else if (!obj_extra_flagged(obj, ITEM_SLOT_ONE))
    obj_extra_set(obj, ITEM_SLOT_ONE, true);
  else
    obj_extra_set(obj, ITEM_SLOTS_FILLED, true);

  /* Raise the stat where the equipment already has it, or add it in the first empty place */
  if (same != affects.end()) {
    same->modifier += raise;
  } else {
    empty->location = stat;
    empty->modifier = raise;
  }
}

ACMD(do_arena)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_in_arena(g, ch)) {
    send_to_char(g, ch, "You are too busy competing to be a spectator.\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: arena (fighter number of participant)\r\n        arena look\r\n        arena scan\r\n        arena stop\r\n");
    return;
  }
  if (str_eq_nocase(arg, "stop")) {
    send_to_char(g, ch, "You stop viewing what's going on in the arena.\r\n");
    char_prf_set(g, ch, PRF_ARENAWATCH, false);
    char_set_arenawatch(ch, -1);
    return;
  }
  if (char_get_in_room(ch) != 17875) {
    send_to_char(g, ch, "You are not close enough to the arena floor to see it.\r\n");
    return;
  }

  if (str_eq_nocase(arg, "look")) {
    if (!char_prf_flagged(g, ch, PRF_ARENAWATCH)) {
      send_to_char(g, ch, "You are not even watching anyone in the arena.\r\n");
      return;
    }
    if (arena_watch(g, ch) != NOWHERE)
      look_at_room(g, arena_watch(g, ch), ch, 0);
    return;
  }

  if (str_eq_nocase(arg, "scan")) {
    bool found = false;
    send_to_char(g, ch, "@D---@CFighters in the arena@D---@n\r\n");
    for (descriptor_data *d : each_desc(g)) {
      if (desc_get_connected(d) != CON_PLAYING || !char_in_arena(g, d->character))
        continue;
      act(g, str_format("@YFighter Number@D: @w{:d}, $N.@n", char_get_idnum(d->character)).c_str(), true, ch, nullptr, d->character, TO_CHAR);
      found = true;
    }
    if (!found)
      send_to_char(g, ch, "@wNone.@n\r\n");
    return;
  }

  int num = parse_int(arg);
  if (num < 0) {
    send_to_char(g, ch, "That is not a valid fighter number\r\n");
    return;
  }
  bool fighting = std::ranges::any_of(each_desc(g), [&](descriptor_data *d) {
    return desc_get_connected(d) == CON_PLAYING && char_get_idnum(d->character) == num && char_in_arena(g, d->character);
  });
  if (!fighting) {
    send_to_char(g, ch, "A fighter with such a number was not found in the arena.\r\n");
    return;
  }
  act(g, "@wYou start watching the action surrounding that particular fighter in the arena.@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@C$n@w starts watching the action in the arena.@n", true, ch, nullptr, nullptr, TO_ROOM);
  char_prf_set(g, ch, PRF_ARENAWATCH, true);
  char_set_arenawatch(ch, num);
}

/* This handles displaying the rpp item store to a player. */
void disp_rpp_store(GameWorld *g, struct char_data *ch)
{
  send_to_char(g, ch, "@m                        RPP Item Store@n\n");
  send_to_char(g, ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  send_to_char(g, ch, "@GItem Name                      @gRPP Cost        @cChoice Number   @yMin Lvl@n\n");
  send_to_char(g, ch, "@WStardust Equipment Set         @D[@Y20@D]            @D[ @C1@D]            @w50@n\n");
  send_to_char(g, ch, "@WPlatinum Masamune (Sword Skill)@D[@Y 5@D]            @D[ @C2@D]            @w40@n\n");
  send_to_char(g, ch, "@WObsidian Dirk (Dagger Skill)   @D[@Y 5@D]            @D[ @C3@D]            @w40@n\n");
  send_to_char(g, ch, "@WEmerald Javelin (Spear Skill)  @D[@Y 5@D]            @D[ @C4@D]            @w40@n\n");
  send_to_char(g, ch, "@WIvory Cane (Club Skill)        @D[@Y 5@D]            @D[ @C5@D]            @w40@n\n");
  send_to_char(g, ch, "@WHyper X65 Cannon (Gun Skill)   @D[@Y 5@D]            @D[ @C6@D]            @w40@n\n");
  send_to_char(g, ch, "@WJagged Rock (Brawl skill)      @D[@Y 5@D]            @D[ @C7@D]            @w40@n\n");
  send_to_char(g, ch, "@WKachin Mountain                @D[@Y 8@D]            @D[ @C8@D]@n\n");
  send_to_char(g, ch, "@WSpar Booster                   @D[@Y15@D]            @D[ @C9@D]@n\n");
  send_to_char(g, ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  send_to_char(g, ch, "@wSyntax: rpp 12 (choice number)@n\r\n");
}

/* This handles buying an item from the rpp item store. */
void handle_rpp_store(GameWorld *g, struct char_data *ch, int choice)
{
  if (choice < 1 || choice > static_cast<int>(std::size(RPP_STORE))) {
    send_to_char(g, ch, "That is not a selection option!\r\n");
    return;
  }
  const rpp_item &item = RPP_STORE[choice - 1];

  if (char_get_rp(ch) < item.cost) {
    send_to_char(g, ch, "You do not have enough RPP to afford that option.\r\n");
    return;
  }
  if (char_get_carry_weight(ch) + item.weight > getMaxCarryWeight(g, ch)) {
    send_to_char(g, ch, "You can not carry that much weight at this moment.\r\n");
    return;
  }
  if (char_get_carry_items(ch) + item.count > char_can_carry_n(ch)) {
    if (item.crowded_receipt)
      send_to_char(g, ch, "@R{:d}@W RPP from your Bank paid for your selection. Enjoy!@n\r\n", item.cost);
    else
      send_to_char(g, ch, "You have too many items on you to carry anymore at this moment.\r\n");
    return;
  }
  if (char_get_level(ch) < item.min_level) {
    send_to_char(g, ch, "You are below the minimum level to equip it.\r\n");
    return;
  }

  for (obj_vnum vnum = item.first; vnum <= item.last; vnum++) {
    hand_over(g, ch, vnum, item.heavy);
    if (item.doubled && vnum >= item.doubled)
      hand_over(g, ch, vnum, item.heavy);
  }
  char_set_rp(ch, char_get_rp(ch) - item.cost);
  char_get_desc(ch)->rpp = char_get_rp(ch);
  userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
  save_char(g, ch);
  receipt(g, ch, item);
  send_to_imm(g, "RPP Purchase: {} {:d}", char_get_name(ch), item.cost);
}
