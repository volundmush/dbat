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

#include "misc.hpp"

ACMD(do_preference)
{

  if (char_is_npc(ch))
    return;

  std::string arg;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg.empty()) {
    send_to_char_printf(g, ch, "Syntax: preference (throw | weapon | hand | ki)\r\n");
    return;
  }

  if (char_get_preference(ch) > 0) {
    send_to_char_printf(g, ch, "You've already chosen a specialization. No going back.\r\n");
    return;
  }

  if (str_eq_nocase(arg, "throw")) {
    send_to_char_printf(g, ch, "You will now favor throwing weapons as fighting specialization. You're sure to nail it.\r\n");
    char_set_preference(ch, PREFERENCE_THROWING);
    if (char_get_skills_base(ch, SKILL_THROW) <= 90) {
      char_set_skills_base(ch, SKILL_THROW, char_get_skills_base(ch, SKILL_THROW) + (10));
    } else if (char_get_skills_base(ch, SKILL_THROW) < 100) {
      char_set_skills_base(ch, SKILL_THROW, 100);
    }
    return;
  }
  if (str_eq_nocase(arg, "hand")) {
    send_to_char_printf(g, ch, "You will now favor your body as your fighting specialization. Your body is ready.\r\n");
    char_set_preference(ch, PREFERENCE_H2H);
    return;
  }
  if (str_eq_nocase(arg, "ki")) {
    send_to_char_printf(g, ch, "You will now favor your ki energy as your fighting specialization. I expect more than a few smoldering craters.\r\n");
    char_set_preference(ch, PREFERENCE_KI);
    return;
  }
  if (str_eq_nocase(arg, "weapon")) {
    send_to_char_printf(g, ch, "You will now favor your weapons as your fighting specialization. Let the blood fly!\r\n");
    char_set_preference(ch, PREFERENCE_WEAPON);
    return;
  }
  send_to_char_printf(g, ch, "Syntax: preference (throw | weapon | hand | ki)\r\n");
  return;
}

void rpp_feature(GameWorld *g, struct char_data *ch, const std::string &arg)
{
  int cost = 0, change = false;

  if (arg.empty()) {
    send_to_char_printf(g, ch, "Syntax: rpp 13 (description)\nExample: rpp 13 a large red scar on his face\nDisplayed to others: He has a large red scar on his face.\r\n");
    return;
  }

  if (arg.size() > 60) {
    send_to_char_printf(g, ch, "Please limit it to 60 characters.\r\n");
    return;
  }

  if (!char_get_feature(ch)) {
    cost = 2;
  } else {
    cost   = 2;
    change = true;
  }

  if (cost > char_get_rbank(ch)) {
    send_to_char_printf(g, ch, "You do not have enough RPP in your Bank for that!\r\n");
    return;
  }
  char sex[128], buf8[MAX_INPUT_LENGTH];
  snprintf(sex, sizeof(sex), "%s", char_get_sex(ch) == SEX_FEMALE ? "She" : char_get_sex(ch) == SEX_MALE ? "He"
                                                                                                         : "It");

  char_set_rbank(ch, char_get_rbank(ch) - (cost));
  snprintf(buf8, sizeof(buf8), "...%s has %s.", sex, arg.c_str());
  send_to_char_printf(g, ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
  send_to_char_printf(g, ch, "You now have the following line underneath you when someone sees you in a room as:\n@C%s@n\r\n", buf8);
  char_set_feature(ch, buf8);
  if (change == true) {
    send_to_imm_printf(g, "%s has altered their extra description. Make sure the reason is legit! If it is then reimb them 2 RPP.\r\n", char_get_user(ch));
    send_to_char_printf(g, ch, "The immortals have been notified about this change. It had better have been for a good reason.\r\n");
  }
  mud_log_printf("%s RPP Feature: '%s' Check for rule compliance.", char_get_user(ch), buf8);
  return;
}

ACMD(do_instill)
{

  if (char_is_npc(ch))
    return;

  struct obj_data *obj, *token;
  std::string arg, arg2;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  if (arg.empty() || arg2.empty()) {
    send_to_char_printf(g, ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!(token = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char_printf(g, ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg2, nullptr, char_get_inventory(ch)))) {
    send_to_char_printf(g, ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!obj_extra_flagged(token, ITEM_TOKEN)) {
    send_to_char_printf(g, ch, "That is not a token.\r\n");
    return;
  }

  if (obj_extra_flagged(token, ITEM_FORGED)) {
    send_to_char_printf(g, ch, "That token is a forgery!\r\n");
    return;
  }

  if (!wearable_obj(obj)) {
    send_to_char_printf(g, ch, "You can only instill tokens into equipment.\r\n");
    return;
  }

  if (!obj_extra_flagged(obj, ITEM_SLOT1) && !obj_extra_flagged(obj, ITEM_SLOT2)) {
    send_to_char_printf(g, ch, "That piece of equipment does not have any slots.\r\n");
    return;
  }

  if (obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char_printf(g, ch, "That piece of equipment has already had its token slots filled. This can not be reversed.");
    return;
  } else { /* It has at least one open slot. */
    int stat = 0, raise = 0;
    stat = obj_get_affected(token)[0].location;

    if (obj_get_affected(obj)[0].location != 0 && obj_get_affected(obj)[1].location != 0 && obj_get_affected(obj)[2].location != 0 && obj_get_affected(obj)[3].location != 0 && obj_get_affected(obj)[4].location != 0 && obj_get_affected(obj)[5].location != 0) {
      if (obj_get_affected(obj)[0].location != stat && obj_get_affected(obj)[1].location != stat && obj_get_affected(obj)[2].location != stat && obj_get_affected(obj)[3].location != stat && obj_get_affected(obj)[4].location != stat && obj_get_affected(obj)[5].location != stat) {
        send_to_char_printf(g, ch, "This already has as many different stats as it can hold.\r\n");
        return;
      }
    }

    act(g, "@GYou instill the token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@g$n@G instills a token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", true, ch, obj, nullptr, TO_ROOM);
    raise = obj_get_affected(token)[0].modifier;
    extract_obj(g, token);

    if (obj_extra_flagged(obj, ITEM_SLOT1))
      obj_extra_set(obj, ITEM_SLOTS_FILLED, true);
    else if (obj_extra_flagged(obj, ITEM_SLOT2) && !obj_extra_flagged(obj, ITEM_SLOT_ONE))
      obj_extra_set(obj, ITEM_SLOT_ONE, true);
    else if (obj_extra_flagged(obj, ITEM_SLOT2) && obj_extra_flagged(obj, ITEM_SLOT_ONE))
      obj_extra_set(obj, ITEM_SLOTS_FILLED, true);

    /* Check it's slots for the appropriate stat and add to it if possible */
    if (obj_get_affected(obj)[0].location == stat) {
      obj_get_affected(obj)[0].modifier += raise;
    } else if (obj_get_affected(obj)[1].location == stat) {
      obj_get_affected(obj)[1].modifier += raise;
    } else if (obj_get_affected(obj)[2].location == stat) {
      obj_get_affected(obj)[2].modifier += raise;
    } else if (obj_get_affected(obj)[3].location == stat) {
      obj_get_affected(obj)[3].modifier += raise;
    } else if (obj_get_affected(obj)[4].location == stat) {
      obj_get_affected(obj)[4].modifier += raise;
    } else if (obj_get_affected(obj)[5].location == stat) {
      obj_get_affected(obj)[5].modifier += raise;
    } else if (obj_get_affected(obj)[0].location == 0) { /* It's empty, put it here regardless */
      obj_get_affected(obj)[0].location = stat;
      obj_get_affected(obj)[0].modifier = raise;
    } else if (obj_get_affected(obj)[1].location == 0) { /* It's empty, put it here regardless */
      obj_get_affected(obj)[1].location = stat;
      obj_get_affected(obj)[1].modifier = raise;
    } else if (obj_get_affected(obj)[2].location == 0) { /* It's empty, put it here regardless */
      obj_get_affected(obj)[2].location = stat;
      obj_get_affected(obj)[2].modifier = raise;
    } else if (obj_get_affected(obj)[3].location == 0) { /* It's empty, put it here regardless */
      obj_get_affected(obj)[3].location = stat;
      obj_get_affected(obj)[3].modifier = raise;
    } else if (obj_get_affected(obj)[4].location == 0) { /* It's empty, put it here regardless */
      obj_get_affected(obj)[4].location = stat;
      obj_get_affected(obj)[4].modifier = raise;
    } else if (obj_get_affected(obj)[5].location == 0) { /* It's empty, put it here regardless */
      obj_get_affected(obj)[5].location = stat;
      obj_get_affected(obj)[5].modifier = raise;
    }
  }
}

ACMD(do_arena)
{

  std::string arg;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (char_in_arena(g, ch)) {
    send_to_char_printf(g, ch, "You are too busy competing to be a spectator.\r\n");
    return;
  }

  if (arg.empty()) {
    send_to_char_printf(g, ch, "Syntax: arena (fighter number of participant)\r\n        arena look\r\n        arena scan\r\n        arena stop\r\n");
    return;
  }
  if (str_eq_nocase(arg, "stop")) {
    send_to_char_printf(g, ch, "You stop viewing what's going on in the arena.\r\n");
    char_prf_set(g, ch, PRF_ARENAWATCH, false);
    char_set_arenawatch(ch, -1);
    return;
  }
  if (char_get_in_room(ch) != 17875) {
    send_to_char_printf(g, ch, "You are not close enough to the arena floor to see it.\r\n");
    return;
  }
  if (str_eq_nocase(arg, "look")) {
    if (!char_prf_flagged(g, ch, PRF_ARENAWATCH)) {
      send_to_char_printf(g, ch, "You are not even watching anyone in the arena.\r\n");
      return;
    }
    if (arena_watch(g, ch) != NOWHERE) {
      look_at_room(g, arena_watch(g, ch), ch, 0);
    }
  } else if (str_eq_nocase(arg, "scan")) {
    if (char_get_in_room(ch) == 17875) {
      int found = false;

      send_to_char_printf(g, ch, "@D---@CFighters in the arena@D---@n\r\n");
      for (descriptor_data *d : each_desc(g)) {
        if (desc_get_connected(d) != CON_PLAYING)
          continue;

        if (char_in_arena(g, d->character)) {
          char buf[MAX_INPUT_LENGTH];
          snprintf(buf, sizeof(buf), "@YFighter Number@D: @w%d, $N.@n", char_get_idnum(d->character));
          act(g, buf, true, ch, nullptr, d->character, TO_CHAR);
          found = true;
        }
      }

      if (found == false) {
        send_to_char_printf(g, ch, "@wNone.@n\r\n");
      }
    } else {
      send_to_char_printf(g, ch, "You are not close enough to see what fighters are in the arena.\r\n");
      return;
    }
  } else {
    int num = parse_int(arg);

    if (num < 0) {
      send_to_char_printf(g, ch, "That is not a valid fighter number\r\n");
      return;
    } else {
      int found = false;

      for (descriptor_data *d : each_desc(g)) {
        if (desc_get_connected(d) != CON_PLAYING)
          continue;

        if (char_get_idnum(d->character) == num) {
          if (char_in_arena(g, d->character)) {
            found = true;
          }
        }
      }

      if (found == true) {
        act(g, "@wYou start watching the action surrounding that particular fighter in the arena.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@C$n@w starts watching the action in the arena.@n", true, ch, nullptr, nullptr, TO_ROOM);
        char_prf_set(g, ch, PRF_ARENAWATCH, true);
        char_set_arenawatch(ch, num);
      } else {
        send_to_char_printf(g, ch, "A fighter with such a number was not found in the arena.\r\n");
        return;
      }
    } /* Secondary else end */
  } /* Main Else end */
} /* End of Arena Function */

/* This handles displaying the rpp item store to a player. */
void disp_rpp_store(GameWorld *g, struct char_data *ch)
{

  send_to_char_printf(g, ch, "@m                        RPP Item Store@n\n");
  send_to_char_printf(g, ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  send_to_char_printf(g, ch, "@GItem Name                      @gRPP Cost        @cChoice Number   @yMin Lvl@n\n");
  send_to_char_printf(g, ch, "@WStardust Equipment Set         @D[@Y20@D]            @D[ @C1@D]            @w50@n\n");
  send_to_char_printf(g, ch, "@WPlatinum Masamune (Sword Skill)@D[@Y 5@D]            @D[ @C2@D]            @w40@n\n");
  send_to_char_printf(g, ch, "@WObsidian Dirk (Dagger Skill)   @D[@Y 5@D]            @D[ @C3@D]            @w40@n\n");
  send_to_char_printf(g, ch, "@WEmerald Javelin (Spear Skill)  @D[@Y 5@D]            @D[ @C4@D]            @w40@n\n");
  send_to_char_printf(g, ch, "@WIvory Cane (Club Skill)        @D[@Y 5@D]            @D[ @C5@D]            @w40@n\n");
  send_to_char_printf(g, ch, "@WHyper X65 Cannon (Gun Skill)   @D[@Y 5@D]            @D[ @C6@D]            @w40@n\n");
  send_to_char_printf(g, ch, "@WJagged Rock (Brawl skill)      @D[@Y 5@D]            @D[ @C7@D]            @w40@n\n");
  send_to_char_printf(g, ch, "@WKachin Mountain                @D[@Y 8@D]            @D[ @C8@D]@n\n");
  send_to_char_printf(g, ch, "@WSpar Booster                   @D[@Y15@D]            @D[ @C9@D]@n\n");
  send_to_char_printf(g, ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");

  send_to_char_printf(g, ch, "@wSyntax: rpp 12 (choice number)@n\r\n");
}

/* This handles buying an item from the rpp item store. */
void handle_rpp_store(GameWorld *g, struct char_data *ch, int choice)
{
  struct obj_data *obj;
  obj_vnum objnum = 0;
  int cost        = 0;

  switch (choice) { /* Find the cost of their selection. */
    case 1:
      cost = 20;
      break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      cost = 5;
      break;
    case 8:
      cost = 8;
      break;
    case 9:
      cost = 15;
      break;
    default:
      send_to_char_printf(g, ch, "That is not a selection option!\r\n");
      return;
  }

  if (char_get_rp(ch) < cost) { /* They can't afford it. */
    send_to_char_printf(g, ch, "You do not have enough RPP to afford that option.\r\n");
    return;
  } else { /* They can afford it. */
    switch (choice) {
      case 1:
        if (char_get_carry_weight(ch) + 26 > getMaxCarryWeight(g, ch)) {
          send_to_char_printf(g, ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 13 > char_can_carry_n(ch)) {
          send_to_char_printf(g, ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 50) {
          send_to_char_printf(g, ch, "You are below the minimum level to equip it.\r\n");
        } else {
          for (objnum = 1110; objnum < 1120; objnum++) {
            if (objnum <= 1116) {
              obj = read_object(g, objnum);
              obj_to_char(g, obj, ch);
              obj_set_size(obj, get_size(ch));
              obj = nullptr;
            } else {
              obj = read_object(g, objnum);
              obj_to_char(g, obj, ch);
              obj_set_size(obj, get_size(ch));
              obj = nullptr;
              obj = read_object(g, objnum);
              obj_to_char(g, obj, ch);
              obj_set_size(obj, get_size(ch));
            }
          }
          char_set_rp(ch, char_get_rp(ch) - (cost));
          char_get_desc(ch)->rpp = char_get_rp(ch);
          userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
          save_char(g, ch);
          send_to_char_printf(g, ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm_printf(g, "RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 2:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(g, ch)) {
          send_to_char_printf(g, ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char_printf(g, ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char_printf(g, ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(g, 1120);
          obj_to_char(g, obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          char_get_desc(ch)->rpp = char_get_rp(ch);
          userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
          save_char(g, ch);
          send_to_char_printf(g, ch, "@R%d@W RPP from your Bank paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm_printf(g, "RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 3:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(g, ch)) {
          send_to_char_printf(g, ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char_printf(g, ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char_printf(g, ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(g, 1121);
          obj_to_char(g, obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          char_get_desc(ch)->rpp = char_get_rp(ch);
          userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
          save_char(g, ch);
          send_to_char_printf(g, ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm_printf(g, "RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 4:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(g, ch)) {
          send_to_char_printf(g, ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char_printf(g, ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char_printf(g, ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(g, 1122);
          obj_to_char(g, obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          char_get_desc(ch)->rpp = char_get_rp(ch);
          userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
          save_char(g, ch);
          send_to_char_printf(g, ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm_printf(g, "RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 5:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(g, ch)) {
          send_to_char_printf(g, ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char_printf(g, ch, "@R%d@W RPP from your Bank paid for your selection. Enjoy!@n\r\n", cost);
        } else if (char_get_level(ch) < 40) {
          send_to_char_printf(g, ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(g, 1123);
          obj_to_char(g, obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          char_get_desc(ch)->rpp = char_get_rp(ch);
          userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
          save_char(g, ch);
          send_to_char_printf(g, ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm_printf(g, "RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 6:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(g, ch)) {
          send_to_char_printf(g, ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char_printf(g, ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char_printf(g, ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(g, 1124);
          obj_to_char(g, obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          char_get_desc(ch)->rpp = char_get_rp(ch);
          userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
          save_char(g, ch);
          send_to_char_printf(g, ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm_printf(g, "RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 7:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(g, ch)) {
          send_to_char_printf(g, ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char_printf(g, ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char_printf(g, ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(g, 1125);
          obj_to_char(g, obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          char_get_desc(ch)->rpp = char_get_rp(ch);
          userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
          save_char(g, ch);
          send_to_char_printf(g, ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm_printf(g, "RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 8:
        if (char_get_carry_weight(ch) + 10000000 > getMaxCarryWeight(g, ch)) {
          send_to_char_printf(g, ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char_printf(g, ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else {
          obj = read_object(g, 1126);
          obj_set_weight(obj, 10000000);
          obj_to_char(g, obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          char_get_desc(ch)->rpp = char_get_rp(ch);
          userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
          save_char(g, ch);
          send_to_char_printf(g, ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm_printf(g, "RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 9:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(g, ch)) {
          send_to_char_printf(g, ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char_printf(g, ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else {
          obj = read_object(g, 1127);
          obj_to_char(g, obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          char_get_desc(ch)->rpp = char_get_rp(ch);
          userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
          save_char(g, ch);
          send_to_char_printf(g, ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm_printf(g, "RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
    } /* End switch */
  }
}
