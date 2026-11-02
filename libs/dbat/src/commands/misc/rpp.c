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
#include "dbat/act.misc.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/fish.h"
#include "dbat/consts/recipes.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"

#include "dbat/character_utils.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/descriptor_utils.h"

#include "dbat/random.h"
#include "dbat/affect.h"
#include "dbat/search.h"
#include "dbat/stringutils.h"
#include "dbat/extract.h"
#include "dbat/relocate.h"
#include "dbat/interpreter.h"

#include "dbat/db.h"

#include "dbat/broadcast.h"
#include "dbat/act.wizard.h"
#include "dbat/act.movement.h"
#include "dbat/act.informative.h"

#include "dbat/spells.h"
#include "dbat/comm.h"

#include "dbat/handler.h"
#include "dbat/combat.h"
#include "dbat/obj_edit.h"
#include "dbat/fight.h"
#include "dbat/class.h"
#include "dbat/act.item.h"
#include "misc.h"

ACMD(do_preference)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Syntax: preference (throw | weapon | hand | ki)\r\n");
    return;
  }

  if (char_get_preference(ch) > 0) {
    send_to_char(ch, "You've already chosen a specialization. No going back.\r\n");
    return;
  }

  if (!strcasecmp(arg, "throw")) {
    send_to_char(ch, "You will now favor throwing weapons as fighting specialization. You're sure to nail it.\r\n");
    char_set_preference(ch, PREFERENCE_THROWING);
    if (char_get_skills_base(ch, SKILL_THROW) <= 90) {
      char_set_skills_base(ch, SKILL_THROW, char_get_skills_base(ch, SKILL_THROW) + (10));
    } else if (char_get_skills_base(ch, SKILL_THROW) < 100) {
      char_set_skills_base(ch, SKILL_THROW, 100);
    }
    return;
  }
  if (!strcasecmp(arg, "hand")) {
    send_to_char(ch, "You will now favor your body as your fighting specialization. Your body is ready.\r\n");
    char_set_preference(ch, PREFERENCE_H2H);
    return;
  }
  if (!strcasecmp(arg, "ki")) {
    send_to_char(ch, "You will now favor your ki energy as your fighting specialization. I expect more than a few smoldering craters.\r\n");
    char_set_preference(ch, PREFERENCE_KI);
    return;
  }
  if (!strcasecmp(arg, "weapon")) {
    send_to_char(ch, "You will now favor your weapons as your fighting specialization. Let the blood fly!\r\n");
    char_set_preference(ch, PREFERENCE_WEAPON);
    return;
  }
  send_to_char(ch, "Syntax: preference (throw | weapon | hand | ki)\r\n");
  return;
}

void rpp_feature(struct char_data *ch, const char *arg)
{
  int cost = 0, change = FALSE;

  if (!*arg) {
    send_to_char(ch, "Syntax: rpp 13 (description)\nExample: rpp 13 a large red scar on his face\nDisplayed to others: He has a large red scar on his face.\r\n");
    return;
  }

  if (strlen(arg) > 60) {
    send_to_char(ch, "Please limit it to 60 characters.\r\n");
    return;
  }

  if (!char_get_feature(ch)) {
    cost = 2;
  } else {
    cost   = 2;
    change = TRUE;
  }

  if (cost > char_get_rbank(ch)) {
    send_to_char(ch, "You do not have enough RPP in your Bank for that!\r\n");
    return;
  }
  char sex[128], buf8[MAX_INPUT_LENGTH];
  snprintf(sex, sizeof(sex), "%s", char_get_sex(ch) == SEX_FEMALE ? "She" : char_get_sex(ch) == SEX_MALE ? "He"
                                                                                                         : "It");

  char_set_rbank(ch, char_get_rbank(ch) - (cost));
  snprintf(buf8, sizeof(buf8), "...%s has %s.", sex, arg);
  send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
  send_to_char(ch, "You now have the following line underneath you when someone sees you in a room as:\n@C%s@n\r\n", buf8);
  char_set_feature(ch, buf8);
  if (change == TRUE) {
    send_to_imm("%s has altered their extra description. Make sure the reason is legit! If it is then reimb them 2 RPP.\r\n", char_get_user(ch));
    send_to_char(ch, "The immortals have been notified about this change. It had better have been for a good reason.\r\n");
  }
  mud_log("%s RPP Feature: '%s' Check for rule compliance.", char_get_user(ch), buf8);
  return;
}

ACMD(do_instill)
{

  if (char_is_npc(ch))
    return;

  struct obj_data *obj, *token;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!(token = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
    send_to_char(ch, "Syntax: instill (token) (target)\r\n");
    return;
  }

  if (!obj_extra_flagged(token, ITEM_TOKEN)) {
    send_to_char(ch, "That is not a token.\r\n");
    return;
  }

  if (obj_extra_flagged(token, ITEM_FORGED)) {
    send_to_char(ch, "That token is a forgery!\r\n");
    return;
  }

  if (!wearable_obj(obj)) {
    send_to_char(ch, "You can only instill tokens into equipment.\r\n");
    return;
  }

  if (!obj_extra_flagged(obj, ITEM_SLOT1) && !obj_extra_flagged(obj, ITEM_SLOT2)) {
    send_to_char(ch, "That piece of equipment does not have any slots.\r\n");
    return;
  }

  if (obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(ch, "That piece of equipment has already had its token slots filled. This can not be reversed.");
    return;
  } else { /* It has at least one open slot. */
    int stat = 0, raise = 0;
    stat = token->affected[0].location;

    if (obj->affected[0].location != 0 && obj->affected[1].location != 0 && obj->affected[2].location != 0 && obj->affected[3].location != 0 && obj->affected[4].location != 0 && obj->affected[5].location != 0) {
      if (obj->affected[0].location != stat && obj->affected[1].location != stat && obj->affected[2].location != stat && obj->affected[3].location != stat && obj->affected[4].location != stat && obj->affected[5].location != stat) {
        send_to_char(ch, "This already has as many different stats as it can hold.\r\n");
        return;
      }
    }

    act("@GYou instill the token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", TRUE, ch, obj, 0, TO_CHAR);
    act("@g$n@G instills a token into @g$p@G. It glows @ggreen@G for a moment before returning to normal. The token disappears with the glow.@n", TRUE, ch, obj, 0, TO_ROOM);
    raise = token->affected[0].modifier;
    extract_obj(token);

    if (obj_extra_flagged(obj, ITEM_SLOT1))
      obj_extra_set(obj, ITEM_SLOTS_FILLED, true);
    else if (obj_extra_flagged(obj, ITEM_SLOT2) && !obj_extra_flagged(obj, ITEM_SLOT_ONE))
      obj_extra_set(obj, ITEM_SLOT_ONE, true);
    else if (obj_extra_flagged(obj, ITEM_SLOT2) && obj_extra_flagged(obj, ITEM_SLOT_ONE))
      obj_extra_set(obj, ITEM_SLOTS_FILLED, true);

    /* Check it's slots for the appropriate stat and add to it if possible */
    if (obj->affected[0].location == stat) {
      obj->affected[0].modifier += raise;
    } else if (obj->affected[1].location == stat) {
      obj->affected[1].modifier += raise;
    } else if (obj->affected[2].location == stat) {
      obj->affected[2].modifier += raise;
    } else if (obj->affected[3].location == stat) {
      obj->affected[3].modifier += raise;
    } else if (obj->affected[4].location == stat) {
      obj->affected[4].modifier += raise;
    } else if (obj->affected[5].location == stat) {
      obj->affected[5].modifier += raise;
    } else if (obj->affected[0].location == 0) { /* It's empty, put it here regardless */
      obj->affected[0].location = stat;
      obj->affected[0].modifier = raise;
    } else if (obj->affected[1].location == 0) { /* It's empty, put it here regardless */
      obj->affected[1].location = stat;
      obj->affected[1].modifier = raise;
    } else if (obj->affected[2].location == 0) { /* It's empty, put it here regardless */
      obj->affected[2].location = stat;
      obj->affected[2].modifier = raise;
    } else if (obj->affected[3].location == 0) { /* It's empty, put it here regardless */
      obj->affected[3].location = stat;
      obj->affected[3].modifier = raise;
    } else if (obj->affected[4].location == 0) { /* It's empty, put it here regardless */
      obj->affected[4].location = stat;
      obj->affected[4].modifier = raise;
    } else if (obj->affected[5].location == 0) { /* It's empty, put it here regardless */
      obj->affected[5].location = stat;
      obj->affected[5].modifier = raise;
    }
  }
}

ACMD(do_arena)
{

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_in_arena(ch)) {
    send_to_char(ch, "You are too busy competing to be a spectator.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Syntax: arena (fighter number of participant)\r\n        arena look\r\n        arena scan\r\n        arena stop\r\n");
    return;
  }
  if (!strcasecmp(arg, "stop")) {
    send_to_char(ch, "You stop viewing what's going on in the arena.\r\n");
    char_prf_set(ch, PRF_ARENAWATCH, false);
    char_set_arenawatch(ch, -1);
    return;
  }
  if (rroom_get_vnum(char_get_in_room(ch)) != 17875) {
    send_to_char(ch, "You are not close enough to the arena floor to see it.\r\n");
    return;
  }
  if (!strcasecmp(arg, "look")) {
    if (!char_prf_flagged(ch, PRF_ARENAWATCH)) {
      send_to_char(ch, "You are not even watching anyone in the arena.\r\n");
      return;
    }
    if (arena_watch(ch) != NOWHERE) {
      look_at_room(real_room(arena_watch(ch)), ch, 0);
    }
  } else if (!strcasecmp(arg, "scan")) {
    if (rroom_get_vnum(char_get_in_room(ch)) == 17875) {
      int found = FALSE;
      struct descriptor_data *d;

      send_to_char(ch, "@D---@CFighters in the arena@D---@n\r\n");
      for (d = descriptor_list; d; d = d->next) {
        if (desc_get_connected(d) != CON_PLAYING)
          continue;

        if (char_in_arena(d->character)) {
          char buf[MAX_INPUT_LENGTH];
          snprintf(buf, sizeof(buf), "@YFighter Number@D: @w%d, $N.@n", char_get_idnum(d->character));
          act(buf, TRUE, ch, 0, d->character, TO_CHAR);
          found = TRUE;
        }
      }

      if (found == FALSE) {
        send_to_char(ch, "@wNone.@n\r\n");
      }
    } else {
      send_to_char(ch, "You are not close enough to see what fighters are in the arena.\r\n");
      return;
    }
  } else {
    int num = atoi(arg);

    if (num < 0) {
      send_to_char(ch, "That is not a valid fighter number\r\n");
      return;
    } else {
      struct descriptor_data *d;
      int found = FALSE;

      for (d = descriptor_list; d; d = d->next) {
        if (desc_get_connected(d) != CON_PLAYING)
          continue;

        if (char_get_idnum(d->character) == num) {
          if (char_in_arena(d->character)) {
            found = TRUE;
          }
        }
      }

      if (found == TRUE) {
        act("@wYou start watching the action surrounding that particular fighter in the arena.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@w starts watching the action in the arena.@n", TRUE, ch, 0, 0, TO_ROOM);
        char_prf_set(ch, PRF_ARENAWATCH, true);
        char_set_arenawatch(ch, num);
      } else {
        send_to_char(ch, "A fighter with such a number was not found in the arena.\r\n");
        return;
      }
    } /* Secondary else end */
  } /* Main Else end */
} /* End of Arena Function */

/* This handles displaying the rpp item store to a player. */
void disp_rpp_store(struct char_data *ch)
{

  send_to_char(ch, "@m                        RPP Item Store@n\n");
  send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
  send_to_char(ch, "@GItem Name                      @gRPP Cost        @cChoice Number   @yMin Lvl@n\n");
  send_to_char(ch, "@WStardust Equipment Set         @D[@Y20@D]            @D[ @C1@D]            @w50@n\n");
  send_to_char(ch, "@WPlatinum Masamune (Sword Skill)@D[@Y 5@D]            @D[ @C2@D]            @w40@n\n");
  send_to_char(ch, "@WObsidian Dirk (Dagger Skill)   @D[@Y 5@D]            @D[ @C3@D]            @w40@n\n");
  send_to_char(ch, "@WEmerald Javelin (Spear Skill)  @D[@Y 5@D]            @D[ @C4@D]            @w40@n\n");
  send_to_char(ch, "@WIvory Cane (Club Skill)        @D[@Y 5@D]            @D[ @C5@D]            @w40@n\n");
  send_to_char(ch, "@WHyper X65 Cannon (Gun Skill)   @D[@Y 5@D]            @D[ @C6@D]            @w40@n\n");
  send_to_char(ch, "@WJagged Rock (Brawl skill)      @D[@Y 5@D]            @D[ @C7@D]            @w40@n\n");
  send_to_char(ch, "@WKachin Mountain                @D[@Y 8@D]            @D[ @C8@D]@n\n");
  send_to_char(ch, "@WSpar Booster                   @D[@Y15@D]            @D[ @C9@D]@n\n");
  send_to_char(ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");

  send_to_char(ch, "@wSyntax: rpp 12 (choice number)@n\r\n");
}

/* This handles buying an item from the rpp item store. */
void handle_rpp_store(struct char_data *ch, int choice)
{
  struct obj_data *obj;
  int objnum = 0, cost = 0;

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
      send_to_char(ch, "That is not a selection option!\r\n");
      return;
  }

  if (char_get_rp(ch) < cost) { /* They can't afford it. */
    send_to_char(ch, "You do not have enough RPP to afford that option.\r\n");
    return;
  } else { /* They can afford it. */
    switch (choice) {
      case 1:
        if (char_get_carry_weight(ch) + 26 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 13 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 50) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          for (objnum = 1110; objnum < 1120; objnum++) {
            if (objnum <= 1116) {
              obj = read_object(objnum, VIRTUAL);
              obj_to_char(obj, ch);
              obj_set_size(obj, get_size(ch));
              obj = NULL;
            } else {
              obj = read_object(objnum, VIRTUAL);
              obj_to_char(obj, ch);
              obj_set_size(obj, get_size(ch));
              obj = NULL;
              obj = read_object(objnum, VIRTUAL);
              obj_to_char(obj, ch);
              obj_set_size(obj, get_size(ch));
            }
          }
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 2:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1120, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP from your Bank paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 3:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1121, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 4:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1122, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 5:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "@R%d@W RPP from your Bank paid for your selection. Enjoy!@n\r\n", cost);
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1123, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 6:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1124, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 7:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else if (char_get_level(ch) < 40) {
          send_to_char(ch, "You are below the minimum level to equip it.\r\n");
        } else {
          obj = read_object(1125, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 8:
        if (char_get_carry_weight(ch) + 10000000 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else {
          obj = read_object(1126, VIRTUAL);
          obj_set_weight(obj, 10000000);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
      case 9:
        if (char_get_carry_weight(ch) + 2 > getMaxCarryWeight(ch)) {
          send_to_char(ch, "You can not carry that much weight at this moment.\r\n");
        } else if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(ch, "You have too many items on you to carry anymore at this moment.\r\n");
        } else {
          obj = read_object(1127, VIRTUAL);
          obj_to_char(obj, ch);
          obj_set_size(obj, get_size(ch));
          char_set_rp(ch, char_get_rp(ch) - (cost));
          ch->desc->rpp = char_get_rp(ch);
          userWrite(ch->desc, 0, 0, 0, "index");
          save_char(ch);
          send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", cost);
          send_to_imm("RPP Purchase: %s %d", char_get_name(ch), cost);
        }
        break;
    } /* End switch */
  }
}
