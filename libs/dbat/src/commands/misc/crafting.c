/* ************************************************************************
*  File: misc/crafting.c                                    Part of DBAT       *
*  Usage: Crafting, cooking, fishing and item commands                             *
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

/* local functions */
static int campfire_cook(int recipe);
static int valid_recipe(struct char_data *ch, int recipe, int type);
static int has_pole(struct char_data *ch);
static void catch_fish(struct char_data *ch, int quality);

ACMD(do_restring)
{

  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int pay = 0;

  one_argument(argument, arg);

  if (rroom_get_vnum(char_get_in_room(ch)) >= 178 && rroom_get_vnum(char_get_in_room(ch)) <= 184) {
    pay = 5000;
    if (char_get_gold(ch) < pay) {
      send_to_char(ch, "You need at least 5,000 zenni to initiate an equipment restring.\r\n");
      return;
    } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have a that equipment to restring in your inventory.\r\n");
      send_to_char(ch, "Syntax: restring (obj name)\r\n");
      return;
    } else if (obj_extra_flagged(obj, ITEM_CUSTOM)) {
      send_to_char(ch, "You can not restring a custom piece. Why? Because you already restrung it you dummy.\r\n");
      return;
    } else {
      desc_set_connected(ch->desc, CON_POBJ);
      char thename[MAX_INPUT_LENGTH], theshort[MAX_INPUT_LENGTH], thelong[MAX_INPUT_LENGTH];

      *thename  = '\0';
      *theshort = '\0';
      *thelong  = '\0';

      snprintf(thename, sizeof(thename), "%s", obj->name);
      snprintf(theshort, sizeof(theshort), "%s", obj->short_description);
      snprintf(thelong, sizeof(thelong), "%s", obj->description);

      ch->desc->obj_name   = strdup(thename);
      ch->desc->obj_was    = strdup(theshort);
      ch->desc->obj_short  = strdup(theshort);
      ch->desc->obj_long   = strdup(thelong);
      ch->desc->obj_point  = obj;
      ch->desc->obj_type   = 1;
      ch->desc->obj_weapon = 0;
      disp_restring_menu(ch->desc);
      ch->desc->obj_editflag = EDIT_RESTRING;
      ch->desc->obj_editval  = EDIT_RESTRING_MAIN;
      return;
    }
  }
}

ACMD(do_fish)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: fish ( cast | hook | reel | apply | stop)\r\n");
    return;
  }

  if (!strcasecmp(arg, "cast")) {
    if (char_plr_flagged(ch, PLR_FISHING)) {
      send_to_char(ch, "You are already fishing! Syntax: fish stop\r\n");
      return;
    } else if (!rroom_flagged(char_get_in_room(ch), ROOM_FISHING)) {
      send_to_char(ch, "This is not an area you can fish at.\r\n");
      return;
    } else if (char_aff_flagged(ch, AFF_FLYING)) {
      send_to_char(ch, "You can't fish while flying.\r\n");
      return;
    } else if (char_get_fighting(ch)) {
      send_to_char(ch, "You can't fish while fighting!\r\n");
      return;
    } else if (!char_get_equipment(ch, WEAR_WIELD2)) {
      send_to_char(ch, "You are not holding a fishing pole.\r\n");
      return;
    } else {
      struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
      if (obj_get_type_flag(pole) != ITEM_FISHPOLE) {
        send_to_char(ch, "You do not have a fishing pole in your hand!\r\n");
        return;
      } else if (obj_get_value(pole, 0) == 0) {
        send_to_char(ch, "There is no bait on your line!\r\n");
        return;
      }
      reveal_hiding(ch, 0);
      act("@CYou pull your arm back and then spring it forward, casting the baited line. A moment later there is a splash as the hook enters the water.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C pulls $s arm back and then springs it foward, casting the line of $s fishing pole into the water.@n", TRUE, ch, 0, 0, TO_ROOM);
      char_set_accuracy_mod(ch, rand_number(30, 80));
      char_plr_set(ch, PLR_FISHING, true);
      send_to_char(ch, "@D[@wDistance@D: @Y%d@D]@n\r\n", char_get_accuracy_mod(ch));
      return;
    }
  } else if (!strcasecmp(arg, "hook")) {
    if (!char_plr_flagged(ch, PLR_FISHING)) {
      send_to_char(ch, "You are not even fishing!\r\n");
      return;
    } else if (char_get_fishstate(ch) == FISH_NOFISH) {
      send_to_char(ch, "You do not have a fish biting on your line.\r\n");
      return;
    } else if (char_get_fishstate(ch) == FISH_REELING) {
      send_to_char(ch, "You are already trying to reel the fish in!\r\n");
      return;
    } else if (char_get_fishstate(ch) == FISH_HOOKED) {
      send_to_char(ch, "You already have the fish hooked! Reel it in!\r\n");
      return;
    } else if (axion_dice(-18) > char_get_accuracy(ch)) {
      reveal_hiding(ch, 0);
      act("@CYou pull hard but the fish spits the hook out a second before you pull! You return to waiting for a bite...@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C pulls hard on $s fishing line, but a moment later $e frowns and returns to fishing.@n", TRUE, ch, 0, 0, TO_ROOM);
      char_set_fishstate(ch, FISH_NOFISH);
      return;
    } else {
      reveal_hiding(ch, 0);
      act("@CYou pull hard on your line and feel that you have managed to hook the fish and start reeling it in!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C pulls hard on $s fishing line and starts to struggle with the fish on the other end!@n", TRUE, ch, 0, 0, TO_ROOM);
      char_set_fishstate(ch, FISH_REELING);
      return;
    }
  } else if (!strcasecmp(arg, "apply")) {
    if (!char_get_equipment(ch, WEAR_WIELD2)) {
      send_to_char(ch, "You are not holding a fishing pole.\r\n");
      return;
    } else {
      struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
      if (obj_get_type_flag(pole) != ITEM_FISHPOLE) {
        send_to_char(ch, "You do not have a fishing pole in your hand!\r\n");
        return;
      } else if (obj_get_value(pole, 0) != 0) {
        send_to_char(ch, "Your fishing pole already has bait on its hook.\r\n");
        return;
      } else {
        struct obj_data *bait;

        if (!*arg2) {
          send_to_char(ch, "Syntax: fish apply (bait)\r\n");
          return;
        }
        if (!(bait = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
          send_to_char(ch, "You don't have that bait.\r\n");
          return;
        } else if (obj_get_type_flag(bait) != ITEM_FISHBAIT) {
          send_to_char(ch, "That isn't fishing bait!\r\n");
          return;
        } else {
          reveal_hiding(ch, 0);
          act("@CYou carefully apply the $p@C to your hook.@n", TRUE, ch, bait, 0, TO_CHAR);
          act("@c$n@C carefully applies $p@C to $s fishing pole's hook.@n", TRUE, ch, bait, 0, TO_ROOM);
          obj_set_value(pole, 0, obj_get_cost(bait));
          extract_obj(bait);
          return;
        } /* End we applied bait */
      } /* End Applying bait */
    } /* end has pole */
  } else if (!strcasecmp(arg, "stop")) {
    if (!char_plr_flagged(ch, PLR_FISHING)) {
      send_to_char(ch, "You are not even fishing!\r\n");
      return;
    } else {
      reveal_hiding(ch, 0);
      act("@CYou reel in your line and stop fishing.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C reels in $s fishing line and stops fishing.@n", TRUE, ch, 0, 0, TO_ROOM);
      char_plr_set(ch, PLR_FISHING, false);
      char_set_fishstate(ch, FISH_NOFISH);
      char_set_accuracy_mod(ch, 0);
      return;
    }
  } else {
    send_to_char(ch, "Syntax: fish ( cast | hook | reel | apply | stop )\r\n");
    return;
  }
} /* End fish */

static int has_pole(struct char_data *ch)
{

  if (char_get_equipment(ch, WEAR_WIELD2)) {
    struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
    if (obj_get_type_flag(pole) == ITEM_FISHPOLE) {
      return (TRUE);
    }
  }

  return (FALSE);
}

void fish_update(void)
{

  struct char_data *i, *next_char, *ch = NULL;
  int quality = 0;

  for (i = character_list; i; i = next_char) {
    next_char = i->next;
    if (rroom_flagged(char_get_in_room(i), ROOM_FISHING)) {
      if (char_plr_flagged(i, PLR_FISHING) && has_pole(i) == TRUE) {
        ch = i;
        if (char_get_accuracy_mod(ch) <= 0 && char_get_fishstate(ch) == FISH_REELING) { /* We've caught it */
          if (char_get_accuracy(ch) >= rand_number(60, 100)) {
            quality = rand_number(0, 3) + rand_number(0, 3) + rand_number(0, 3);
          } else if (char_get_accuracy(ch) >= rand_number(45, 60)) {
            quality = rand_number(0, 3) + rand_number(0, 3);
          } else {
            quality = rand_number(0, 3);
          }
          catch_fish(ch, quality);
        } else if (rand_number(1, 5) >= 3) { /* Reeling section */
          if (char_get_fishstate(ch) == FISH_REELING && rand_number(1, 100) <= 80) {
            if (char_get_accuracy(ch) >= 80) {
              char_set_accuracy_mod(ch, char_get_accuracy_mod(ch) - (rand_number(6, 10)));
            } else if (char_get_accuracy(ch) >= 40) {
              char_set_accuracy_mod(ch, char_get_accuracy_mod(ch) - (rand_number(5, 8)));
            } else {
              char_set_accuracy_mod(ch, char_get_accuracy_mod(ch) - (rand_number(1, 4)));
            }
            act("@CYou reel the line on your pole some.@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C reels the line on $s pole slowly.@n", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char(ch, "@D[@wDistance@D: @Y%d@D]@n\r\n", char_get_accuracy_mod(ch) > 0 ? char_get_accuracy_mod(ch) : 0);
          } else if (char_get_fishstate(ch) == FISH_REELING && rand_number(1, 58) <= 55) {
            act("@CYou struggle as the fish fights against your attempts to reel it in!@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C struggles with the fish on the end of $s pole!@n", TRUE, ch, 0, 0, TO_ROOM);
          } else if (char_get_fishstate(ch) == FISH_REELING) { /* Lose the fish */
            act("@CYou feel the line go slack and realize you've lost the fish! You reel your line back in...@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C frowns and then begins to reel in $s line.@n", TRUE, ch, 0, 0, TO_ROOM);
            char_set_accuracy_mod(ch, 0);
            char_set_fishstate(ch, FISH_NOFISH);
            char_plr_set(ch, PLR_FISHING, false);
            if (has_pole(ch) == TRUE) {
              struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
              obj_set_value(pole, 0, 0);
            }
          } else if (char_get_fishstate(ch) == FISH_HOOKED && rand_number(1, 20) >= 12) {
            act("@CYou feel the line go slack and realize you've lost the fish! You reel your line back in...@n", TRUE, ch, 0, 0, TO_CHAR);
            act("@c$n@C frowns and then begins to reel in $s line.@n", TRUE, ch, 0, 0, TO_ROOM);
            char_set_accuracy_mod(ch, 0);
            char_set_fishstate(ch, FISH_NOFISH);
            char_plr_set(ch, PLR_FISHING, false);
          } else if (char_get_fishstate(ch) == FISH_BITE && rand_number(1, 20) >= 12) {
            act("@CYou feel as if the fish has stopped biting...@n", TRUE, ch, 0, 0, TO_CHAR);
            char_set_fishstate(ch, FISH_NOFISH);
          } else if (char_get_fishstate(ch) != FISH_HOOKED && char_get_fishstate(ch) != FISH_BITE && ((rroom_flagged(char_get_in_room(ch), ROOM_FISHFRESH) && rand_number(1, 10) >= 8) || (!rroom_flagged(char_get_in_room(ch), ROOM_FISHFRESH) && rand_number(1, 20) >= 18))) {
            act("@CYou feel a fish biting on your line! Better @Ghook@C it!@n", TRUE, ch, 0, 0, TO_CHAR);
            char_set_fishstate(ch, FISH_BITE);
          }
        } /* End reel section */
      } else if (char_plr_flagged(i, PLR_FISHING) && has_pole(i) == FALSE) { /* End of, Is Fishing */
        char_plr_set(i, PLR_FISHING, false);
        char_set_accuracy_mod(i, 0);
        char_set_fishstate(i, FISH_NOFISH);
      }
    } else { /* Is not in a fishing room */
      if (char_plr_flagged(i, PLR_FISHING)) {
        char_plr_set(i, PLR_FISHING, false);
        char_set_accuracy_mod(i, 0);
        char_set_fishstate(i, FISH_NOFISH);
      }
    }
  } /* End of for */
}

static void catch_fish(struct char_data *ch, int quality)
{
  struct obj_data *fish = NULL;
  int num               = 1000;

  if (rroom_flagged(char_get_in_room(ch), ROOM_FISHFRESH)) {
    if (rroom_flagged(char_get_in_room(ch), ROOM_EARTH)) {
      switch (rand_number(1, 10)) {
        case 1:
        case 2:
        case 3:
        case 4:
          num = 1000;
          break;
        case 5:
        case 6:
        case 7:
          num = 1001;
          break;
        case 8:
        case 9:
          num = 1002;
          break;
        case 10:
          num = 1003;
          break;
      }
    } else if (rroom_flagged(char_get_in_room(ch), ROOM_AETHER)) {
      switch (rand_number(1, 10)) {
        case 1:
        case 2:
        case 3:
        case 4:
          num = 1012;
          break;
        case 5:
        case 6:
        case 7:
          num = 1013;
          break;
        case 8:
        case 9:
          num = 1014;
          break;
        case 10:
          num = 1015;
          break;
      }
    }
  } else {
    if (rroom_flagged(char_get_in_room(ch), ROOM_EARTH)) {
      switch (rand_number(1, 10)) {
        case 1:
        case 2:
        case 3:
        case 4:
          num = 1004;
          break;
        case 5:
        case 6:
        case 7:
          num = 1005;
          break;
        case 8:
        case 9:
          num = 1006;
          break;
        case 10:
          num = 1007;
          break;
      }
    } else if (rroom_flagged(char_get_in_room(ch), ROOM_NAMEK)) {
      switch (rand_number(1, 10)) {
        case 1:
        case 2:
        case 3:
        case 4:
          num = 1008;
          break;
        case 5:
        case 6:
        case 7:
          num = 1009;
          break;
        case 8:
        case 9:
          num = 1010;
          break;
        case 10:
          num = 1011;
          break;
      }
    }
  }

  fish = read_object(num, VIRTUAL);

  if (!fish) {
    send_to_imm("Error: Fish success with no fish! Report to Iovan!\r\n");
    return;
  }

  act("@CYou manage to pull a $p@C from the water and onto the ground in front of you!@n", TRUE, ch, fish, 0, TO_CHAR);
  act("@c$n@C manages to pull a $p@C from the water and onto the ground in front of $m!@n", TRUE, ch, fish, 0, TO_ROOM);

  int weight = 1;

  if (quality <= 0 && rand_number(1, 20) >= 17) {
    quality += rand_number(2, 7);
  }

  struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);

  if (obj_get_value(pole, 0) * 2 >= axion_dice(0)) {
    quality += 2;
  } else if (obj_get_value(pole, 0) >= axion_dice(0)) {
    quality += 1;
  }

  switch (quality) {
    case 0:
    case 1:
      weight = rand_number(0, 2);
      break;
    case 2:
    case 3:
      weight = rand_number(3, 4);
      obj_set_cost(fish, obj_get_cost(fish) + (obj_get_cost(fish) * 0.20));
      obj_set_value(fish, 0, obj_get_value(fish, 0) + (1));
      break;
    case 4:
    case 5:
    case 6:
      weight = rand_number(5, 9);
      obj_set_cost(fish, obj_get_cost(fish) + (obj_get_cost(fish) * 0.5));
      obj_set_value(fish, 0, obj_get_value(fish, 0) + (3));
      break;
    default:
      weight = rand_number(10, 15);
      obj_set_cost(fish, obj_get_cost(fish) + (obj_get_cost(fish) * 2));
      obj_set_value(fish, 0, obj_get_value(fish, 0) + (5));
      break;
  }

  obj_set_weight(fish, obj_get_weight(fish) + (weight));

  obj_set_value(pole, 0, 0);
  obj_to_room(fish, char_get_in_room(ch));
  do_get(ch, (char[]){"fish"}, 0, 0);
  send_to_char(ch, "@D[@cFish Weight@D: @G%" I64T "@D]@n\r\n", obj_get_weight(fish));
  char_plr_set(ch, PLR_FISHING, false);
  char_set_accuracy_mod(ch, 0);
  char_set_fishstate(ch, FISH_NOFISH);
}

ACMD(do_extract)
{

  if (!char_get_skill(ch, SKILL_EXTRACT)) {
    send_to_char(ch, "You do not know how to extract!\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH], argu[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL, *obj2 = NULL;
  int skill = char_get_skill(ch, SKILL_EXTRACT), chance = axion_dice(0);

  half_chop(argument, arg, argu);
  two_arguments(argu, arg2, arg3);

  if (!*arg) {
    send_to_char(ch, "Syntax: extract (object)\r\n");
    send_to_char(ch, "Syntax: extract combine (bottle1) (bottle2)\r\n");
    return;
  }

  if (!strcasecmp(arg, "combine")) {
    if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
      send_to_char(ch, "You do not have the first bottle that you were wanting to combine.\r\n");
      return;
    }
    if (!(obj2 = get_obj_in_list_vis(ch, arg3, NULL, ch->carrying))) {
      send_to_char(ch, "You do not have the second bottle that you were wanting to combine.\r\n");
      return;
    }
    if (obj && obj2) {
      if (obj_get_vnum(obj) != 3424) {
        send_to_char(ch, "That is not an ink bottle!\r\n");
        return;
      } else if (obj_get_vnum(obj2) != 3424) {
        send_to_char(ch, "That is not an ink bottle!\r\n");
        return;
      } else if (obj_get_value(obj, 6) <= 0) {
        send_to_char(ch, "There isn't any ink in the first bottle!\r\n");
        return;
      } else if (obj_get_value(obj2, 6) <= 0) {
        send_to_char(ch, "There isn't any ink in the second bottle!\r\n");
        return;
      } else {
        if (obj_get_value(obj, 6) >= obj_get_value(obj2, 6)) {
          obj_set_value(obj, 6, obj_get_value(obj, 6) + (obj_get_value(obj2, 6)));
          if (obj_get_value(obj, 6) > 24) {
            obj_set_value(obj, 6, 24);
          }
          send_to_char(ch, "You combine the ink of the two bottles into one bottle, and discard the leftovers.\r\n");
          extract_obj(obj2);
        } else if (obj_get_value(obj2, 6) > obj_get_value(obj, 6)) {
          obj_set_value(obj2, 6, obj_get_value(obj2, 6) + (obj_get_value(obj, 6)));
          if (obj_get_value(obj2, 6) > 24) {
            obj_set_value(obj2, 6, 24);
          }
          send_to_char(ch, "You combine the ink of the two bottles into one bottle, and discard the leftovers.\r\n");
          extract_obj(obj);
        }
        return;
      }
    }
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You do not have that item.\r\n");
    return;
  } else {
    if (obj_get_vnum(obj) == 3425) {
      if (obj_get_value(obj, VAL_MAXMATURE) != 0 && obj_get_value(obj, VAL_MATURITY) < obj_get_value(obj, VAL_MAXMATURE)) {
        send_to_char(ch, "It's not mature enough to extract from!\r\n");
        return;
      }
      struct obj_data *bottle = NULL, *next_obj, *item;
      int found               = FALSE;

      for (item = ch->carrying; item; item = next_obj) {
        next_obj = item->next_content;
        if (obj_get_vnum(item) == 3423) {
          bottle = item;
          found  = TRUE;
        }
      }

      int64_t cost = ((getMaxKI(ch) * 0.35) + 500);

      if (found == FALSE) {
        send_to_char(ch, "You do not have an empty bottle to put the extracted ink in.\r\n");
        return;
      }

      int64_t extra = 0;

      if (obj_get_value(bottle, 6) + 4 >= 24)
        extra = getMaxKI(ch) * 0.5;

      cost += extra;

      if ((getCurKI(ch)) < cost) {
        send_to_char(ch, "You do not have enough ki! @D[@rNeeded@D: @R%s@D]@n\r\n", add_commas(cost));
        return;
      } else if (skill < chance) {
        decCurKI(ch, cost, 0);
        act("@WWith your ki flowing carefully into your hands you take a hold of the @G$p@W and begin to strip it of its leaves. Once it has been stripped you go to squeeze the ink carefully from the leaves into the bottle, but unfortunately the ink explodes into a mess instead!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W takes a hold of the @G$p@W and begins to strip it of its leaves. Once it has been stripped $e bundles up the leaves in $s hands and begins to squeeze. A nasty explosion of a mess is all that follows!@n", TRUE, ch, obj, 0, TO_ROOM);
        improve_skill(ch, SKILL_EXTRACT, 0);
        extract_obj(obj);
        char_set_wait(ch, PULSE_3SEC);
        return;
      } else {
        decCurKI(ch, cost, 0);
        act("@WWith your ki flowing carefully into your hands you take a hold of the @G$p@W and begin to strip it of its leaves. Once it has been stripped you go to squeeze the ink carefully from the leaves into the bottle, and manage to get every last drop of ink into it.@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W takes a hold of the @G$p@W and begins to strip it of its leaves. Once it has been stripped $e bundles up the leaves in $s hands and begins to squeeze ink carefully from the leaves into a bottle.@n", TRUE, ch, obj, 0, TO_ROOM);
        extract_obj(obj);
        obj_set_value(bottle, 6, obj_get_value(bottle, 6) + (rand_number(4, 6)));
        if (obj_get_value(bottle, 6) >= 24) {
          struct obj_data *filled = read_object(3424, VIRTUAL);
          extract_obj(bottle);
          obj_set_value(filled, 6, 24);
          obj_to_char(filled, ch);
          decCurKI(ch, 0, 0);
          act("@GAs the last of the ink fills the bottle you infuse a final burst of ki into the bottle.@n", TRUE, ch, filled, 0, TO_CHAR);
          act("@GAs the last of the ink fills the bottle @g$n@G infuses a final burst of ki into the bottle.@n ", TRUE, ch, filled, 0, TO_ROOM);
        } else {
          send_to_char(ch, "You will need to fill the bottle before giving it a final infusion of ki to complete the process.\r\n");
        }
        improve_skill(ch, SKILL_EXTRACT, 0);
        char_set_wait(ch, PULSE_3SEC);
      }
    } else {
      send_to_char(ch, "That is not something you can extract from.\r\n");
      return;
    }
  }
}

ACMD(do_runic)
{

  if (!char_get_skill(ch, SKILL_RUNIC)) {
    send_to_char(ch, "You do not know how to write down runes.\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int skill = char_get_skill(ch, SKILL_RUNIC), bonus = 0;

  two_arguments(argument, arg, arg2);

  if (char_get_fighting(ch)) {
    send_to_char(ch, "You are too busy fighting to write runes!\r\n");
    return;
  }

  if (!*arg || !*arg2) {
    send_to_char(ch, "Syntax: runic (target) (skill)\r\n");
    send_to_char(ch, "@D----@GRunic Skills@D----@n\r\n");
    send_to_char(ch, "@Rkenaz\n%s\n%s\n%s\n%s\n%s\n%s@n\n", skill >= 40 ? "@Galgiz" : "", skill >= 40 ? "@moagaz" : "", skill >= 50 ? "@CLaguz" : "", skill >= 60 ? "@Ywunjo" : "", skill >= 80 ? "@rpurisaz" : "", skill >= 100 ? "@mgebo" : "");
    return;
  }

  struct obj_data *obj, *next_obj, *bottle = NULL;
  int found = FALSE, amount = 0, brush = FALSE;

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (obj_get_vnum(obj) == 3424) {
      if (obj_get_value(obj, 6) > amount) {
        bottle = obj;
        found  = TRUE;
        amount = obj_get_value(bottle, 6);
      }
    }
  }

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (obj_get_vnum(obj) == 3427) {
      brush = TRUE;
    }
  }

  if (found == FALSE) {
    send_to_char(ch, "You do not have a bottle with enough ink in it.\r\n");
    return;
  } else if (brush == FALSE) {
    send_to_char(ch, "You do not have a brush!\r\n");
    return;
  }

  int64_t cost = getMaxKI(ch) * 0.05;
  int inkcost  = 0;

  if (char_is_hoshijin(ch))
    bonus = 10;
  else
    inkcost += 2;

  struct char_data *vict;

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "You can't seem to find that person.\r\n");
    return;
  } else if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to write runes.\r\n");
    return;
  } else if (skill + bonus < axion_dice(0) && rand_number(1, 5) == 5) {
    act("@BYou dip your brush into the ink, but as you infuse your ki you balance the flow wrong and end up destroying the ink bottle!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@b$n@B dips $s runic brush into a bottle filled with shimmering ink. @b$n@B appears to concentrate for a moment before a look of panic dons $s face. Just at that moment the bottle of ink explodes! Strange...@n", TRUE, ch, 0, 0, TO_ROOM);
    extract_obj(bottle);
    improve_skill(ch, SKILL_RUNIC, 1);
    decCurKI(ch, cost, 0);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (skill + bonus < axion_dice(0)) {
    decCurKI(ch, cost, 0);
    act("@BYou dip your brush into the ink, but as you infuse your ki you balance the flow wrong and end up evaporating some ink!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@b$n@B dips $s runic brush into a bottle filled with shimmering ink. @b$n@B appears to concentrate for a moment before some ink evaporates. Strange...@n", TRUE, ch, 0, 0, TO_ROOM);
    improve_skill(ch, SKILL_RUNIC, 1);
    obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (rand_number(1, 3)));
    if (obj_get_value(bottle, 6) < 0)
      obj_set_value(bottle, 6, 0);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "kenaz") || !strcasecmp(arg2, "Kenaz")) {
    inkcost += 1;
    if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CKenaz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CKenaz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.16;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou can now see in the dark! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_INFRAVISION, SKILL_RUNIC, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CKenaz@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CKenaz@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CKenaz@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.16;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou can now see in the dark! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_INFRAVISION, SKILL_RUNIC, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "algiz") || !strcasecmp(arg2, "Algiz")) {
    inkcost += 2;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CAlgiz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CAlgiz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.05;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now have Ethereal Armor! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_EARMOR, SKILL_PUNCH, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CAlgiz@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CAlgiz@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CAlgiz@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.05;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now have Ethereal Armor! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_EARMOR, SKILL_PUNCH, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "oagaz") || !strcasecmp(arg2, "Oagaz")) {
    inkcost += 3;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@COagaz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@COagaz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.04;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now are protected by Ethereal Chains! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_ECHAINS, SKILL_KNEE, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
  } else if (!strcasecmp(arg2, "laguz") || !strcasecmp(arg2, "Laguz")) {
    inkcost += 4;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CLaguz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CLaguz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.04;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now have water breathing! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_WATERBREATH, SKILL_SBC, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@COagaz@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@COagaz@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@COagaz@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.04;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou now are protected by Ethereal Chains! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_ECHAINS, SKILL_KNEE, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "wunjo") || !strcasecmp(arg2, "Wunjo")) {
    inkcost += 4;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CWunjo@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CWunjo@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.08;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou are now blessed with a deeper understanding of things you experience! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_WUNJO, SKILL_SLAM, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CWunjo@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CWunjo@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CWunjo@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.08;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou are now blessed with a deeper understanding of things you experience! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_WUNJO, SKILL_SLAM, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "purisaz") || !strcasecmp(arg2, "Purisaz")) {
    inkcost += 4;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CPurisaz@D'@B rune on your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CPurisaz@D'@B rune on $s skin.@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.06;
      if (duration < 1)
        duration = 1;
      send_to_char(vict, "@GYou feel as if your inner energy is more potent! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      assign_affect(vict, AFF_POTENT, SKILL_HEELDROP, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CPurisaz@D'@B rune on @b$N's@B skin!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CPurisaz@D'@B rune on @RYOUR@B skin.@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CPUrisaz@D'@B rune on @b$N's@B skin.@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      int duration = skill * 0.06;
      send_to_char(vict, "@GYou feel as if your inner energy is more potent! @D(@WLasts@D: @w%d@D)@n\r\n", duration);
      if (duration < 1)
        duration = 1;
      assign_affect(vict, AFF_POTENT, SKILL_HEELDROP, duration, 0, 0, 0, 0, 0, 0);
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else if (!strcasecmp(arg2, "gebo") || !strcasecmp(arg2, "Gebo")) {
    inkcost += 10;
    if (amount < inkcost) {
      send_to_char(ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R%d@D]@n\r\n", inkcost);
      return;
    } else if (vict == ch) {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CGebo@D'@B rune on your skin! The rune flashes out of existence immediately!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CGebo@D'@B rune on $s skin. The rune flashes out of existence immediately!@n", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      char_set_practice_points(vict, char_get_practice_points(vict) + (125));
      send_to_char(vict, "@GYou feel like you've just gained a lot of knowledge. Now if only you could apply it. @D[@m+125 PS@D]@n\r\n");
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    } else {
      decCurKI(ch, cost, 0);
      act("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@CGebo@D'@B rune on @b$N's@B skin! The rune flashes out of existence immediately!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CGebo@D'@B rune on @RYOUR@B skin. The rune flashes out of existence immediately!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@CGebo@D'@B rune on @b$N's@B skin. The rune flashes out of existence immediately!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      send_to_char(ch, "@D[@B%d@b ink used.@D]@n\r\n", inkcost);
      char_set_practice_points(vict, char_get_practice_points(vict) + (125));
      send_to_char(vict, "@GYou feel like you've just gained a lot of knowledge. Now if only you could apply it. @D[@m+125 PS@D]@n\r\n");
      obj_set_value(bottle, 6, obj_get_value(bottle, 6) - (inkcost));
      if (obj_get_value(bottle, 6) <= 0) {
        extract_obj(bottle);
        struct obj_data *empty = read_object(3423, VIRTUAL);
        obj_to_char(empty, ch);
      }
    }
    improve_skill(ch, SKILL_RUNIC, 1);
    char_set_wait(ch, PULSE_3SEC);
    return;
  } else {
    send_to_char(ch, "Syntax: runic (target) (skill)\r\n");
    send_to_char(ch, "@D----@GRunic Skills@D----@n\r\n");
    send_to_char(ch, "@Rkenaz\n%s\n%s\n%s\n%s\n%s@n\n", skill >= 40 ? "@Galgiz" : "", skill >= 40 ? "@moagaz" : "", skill >= 60 ? "@Ywunjo" : "", skill >= 80 ? "@rpurisaz" : "", skill >= 100 ? "@mgebo" : "");
    return;
  }
}

ACMD(do_resize)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  two_arguments(argument, arg, arg2);

  if (!char_get_skill(ch, SKILL_BUILD)) {
    send_to_char(ch, "You do not have the skill to resize equipment!\r\n");
    return;
  } else if (char_get_skill(ch, SKILL_BUILD) < 80) {
    send_to_char(ch, "Your build skill must be at least level 80 before you can resize equipment.\r\n");
    return;
  } else {
    if (!*arg || !*arg2) {
      send_to_char(ch, "Syntax: resize (obj) (small | medium)\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have that object!\r\n");
      return;
    } else {
      if (!wearable_obj(obj)) {
        send_to_char(ch, "That is not equipment! You can only resize equipment.\r\n");
        return;
      } else {
        if ((getCurST(ch)) < obj_get_weight(obj) + (getMaxST(ch) / 40)) {
          send_to_char(ch, "You do not have enough stamina to resize this object at this time.\r\n");
          return;
        } else if (!strcasecmp(arg2, "small")) {
          if (obj_get_size(obj) == SIZE_SMALL) {
            send_to_char(ch, "The equipment is already small sized.\r\n");
            return;
          } else {
            act("@WYou carefully adjust the size of @c$p@W.@n", TRUE, ch, obj, 0, TO_CHAR);
            act("@C$n@W carefully adjusts the size of @c$p@W.@n", TRUE, ch, obj, 0, TO_ROOM);
            obj_set_size(obj, SIZE_SMALL);
            decCurST(ch, obj_get_weight(obj) + (getMaxST(ch) / 40), 0);
          }
        } else if (!strcasecmp(arg2, "medium")) {
          if (obj_get_size(obj) == SIZE_MEDIUM) {
            send_to_char(ch, "The equipment is already medium sized.\r\n");
            return;
          } else {
            act("@WYou carefully adjust the size of @c$p@W.@n", TRUE, ch, obj, 0, TO_CHAR);
            act("@C$n@W carefully adjusts the size of @c$p@W.@n", TRUE, ch, obj, 0, TO_ROOM);
            obj_set_size(obj, SIZE_MEDIUM);
            decCurST(ch, obj_get_weight(obj) + (getMaxST(ch) / 40), 0);
          }
        } else {
          send_to_char(ch, "Syntax: resize (obj) (small | medium)\r\n");
        }
      }
    }
  }
}

/* This is the mortal dig command. */
ACMD(do_bury)
{

  if (!char_has_arms(ch)) {
    send_to_char(ch, "You have no arms!\r\n");
    return;
  }

  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(ch, "You are busy grappling with someone!\r\n");
    return;
  }

  if (char_get_absorbing(ch) || char_get_absorbby(ch)) {
    send_to_char(ch, "You are busy struggling with someone!\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: dig [bury (item) | uncover]\r\n");
    return;
  }

  if (rroom_sect(char_get_in_room(ch)) != SECT_FIELD && rroom_sect(char_get_in_room(ch)) != SECT_HILLS && rroom_sect(char_get_in_room(ch)) != SECT_FOREST && rroom_sect(char_get_in_room(ch)) != SECT_DESERT && rroom_sect(char_get_in_room(ch)) != SECT_MOUNTAIN) {
    send_to_char(ch, "You are not in a room with enough available dirt or sand to dig.\r\n");
    return;
  }

  struct obj_data *obj = NULL, *buried = NULL, *fobj = NULL, *next_obj;

  for (buried = world[char_get_in_room(ch)].contents; buried; buried = next_obj) {
    next_obj = buried->next_content;
    if (obj_extra_flagged(buried, ITEM_BURIED)) {
      fobj = buried;
    }
  }

  if (!strcasecmp(arg, "bury")) {
    if (!*arg2) {
      send_to_char(ch, "Bury what?\r\n");
      return;
    } else if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have that object to bury.\r\n");
      return;
    } else if (fobj != NULL) {
      send_to_char(ch, "There is already something buried near here.\r\n");
      return;
    } else {
      if (rroom_sect(char_get_in_room(ch)) != SECT_DESERT) {
        act("@yYou start digging in a spot of soft dirt. Once you have an appropriately sized hole you drop @G$p@y in and then cover it.@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@y starts digging in a spot of soft dirt. Once $e has an appropriately sized hole $e drops @G$p@y in and then covers it.@n", TRUE, ch, obj, 0, TO_ROOM);
      } else {
        act("@YYou start digging in a spot of soft sand. Once you have an appropriately sized hole you drop @G$p@Y in and then cover it.@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@Y starts digging in a spot of soft sand. Once $e has an appropriately sized hole $e drops @G$p@Y in and then covers it.@n", TRUE, ch, obj, 0, TO_ROOM);
      }
      obj_from_char(obj);
      obj_to_room(obj, char_get_in_room(ch));
      obj_extra_set(obj, ITEM_BURIED, true);
    }
  } else if (!strcasecmp(arg, "uncover")) {
    if (fobj == NULL) {
      send_to_char(ch, "There is nothing buried here.\r\n");
      return;
    } else {
      if (rroom_sect(char_get_in_room(ch)) != SECT_DESERT) {
        act("@yYou slowly dig and reveal @G$p@y buried in the dirt! You pull it out and set it on the ground before covering the hole back up.@n", TRUE, ch, fobj, 0, TO_CHAR);
        act("@C$n@y starts digging and shortly reveals @G$p@y buried in the dirt! Quickly $e pulls it out and sets it on the ground before covering the hole back up.@n", TRUE, ch, fobj, 0, TO_ROOM);
      } else {
        act("@YYou slowly dig and reveal @G$p@Y buried in the sand! You pull it out and set it on the ground before covering the hole back up.@n", TRUE, ch, fobj, 0, TO_CHAR);
        act("@C$n@Y starts digging and shortly reveals @G$p@Y buried in the sand! Quickly $e pulls it out and sets it on the ground before covering the hole back up.@n", TRUE, ch, fobj, 0, TO_ROOM);
      }
      obj_extra_set(fobj, ITEM_BURIED, false);
    }
  } else {
    send_to_char(ch, "Syntax: dig [bury (item) | uncover]\r\n");
    return;
  }
}

/* This determines of an object is a suitable bundle of silk or not */
int valid_silk(struct obj_data *obj)
{
  int value = 0;

  switch (obj_get_vnum(obj)) {
    case 16700:
    case 16701:
    case 16702:
    case 16703:
    case 16704:
    case 16708:
      value = 1;
      break;
  }

  return (value);
}

ACMD(do_silk)
{

  if (!know_skill(ch, SKILL_SILK)) {
    return;
  }

  struct obj_data *obj = NULL, *weave = NULL, *next_obj = NULL, *weaved = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);
  int prob = char_get_skill(ch, SKILL_SILK), perc = rand_number(1, 120);

  if (!*arg) {
    send_to_char(ch, "Syntax: silk (weave | bundle)\r\n");
    return;
  }

  if (!strcasecmp(arg, "weave")) {

    if (!*arg2) {
      send_to_char(ch, "Syntax: silk weave (head | wrist | belt)\r\n");
      return;
    }

    int found = FALSE, armor = 500, str = 0, intel = 0, olevel = 0;
    double price = 1;

    for (weave = ch->carrying; weave; weave = next_obj) {
      next_obj = weave->next_content;
      if (found == FALSE && valid_silk(weave) && !obj_extra_flagged(weave, ITEM_FORGED)) {
        found = TRUE;
        obj   = weave;
      }
    }

    if (found == FALSE) {
      send_to_char(ch, "You do not have an acceptable bundle of silk in your inventory!\r\n");
      return;
    } else {
      if (!strcasecmp(arg2, "head")) {
        if (prob <= perc) {
          act("@WYou attempt to weave $p@W into the desired piece but end up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@W attempts to weave $p@W into some type of clothing but ends up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
          return;
        } else {
          weaved = read_object(16705, VIRTUAL);
          obj_to_room(weaved, char_get_in_room(ch));
          if (obj_get_vnum(obj) == 16708) {
            armor *= 20;
            str    = 4;
            intel  = 4;
            price  = 4;
            olevel = 80;
          } else if (obj_get_vnum(obj) == 16700) {
            armor *= 15;
            str    = 3;
            intel  = 3;
            price  = 4;
            olevel = 75;
          } else if (obj_get_vnum(obj) == 16701) {
            armor *= 10;
            str    = 3;
            intel  = 3;
            price  = 3;
            olevel = 50;
          } else if (obj_get_vnum(obj) == 16702) {
            armor *= 6;
            str    = 2;
            intel  = 2;
            price  = 2;
            olevel = 25;
          } else if (obj_get_vnum(obj) == 16703) {
            armor *= 4;
            str    = 1;
            intel  = 1;
            price  = 1.5;
            olevel = 5;
          }
          weaved->affected[0].location = 17;
          weaved->affected[0].modifier = armor;
          obj_set_cost(weaved, obj_get_cost(weaved) * (price));
          obj_set_value(weaved, 0, olevel);
          weaved->level = olevel;
          if (str > 0) {
            weaved->affected[1].location = 1;
            weaved->affected[1].modifier = str;
          }
          if (intel > 0) {
            weaved->affected[2].location = 3;
            weaved->affected[2].modifier = intel;
          }
          act("@WYou attempt to weave the bundle and manage to create $p@W!@n", TRUE, ch, weaved, 0, TO_CHAR);
          act("@C$n@W attempts to weave a bundle into something and manages to create $p@W!@n", TRUE, ch, weaved, 0, TO_ROOM);
          do_get(ch, (char[]){"headsash"}, 0, 0);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
        }
      } else if (!strcasecmp(arg2, "wrist")) {
        if (prob <= perc) {
          act("@WYou attempt to weave $p@W into the desired piece but end up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@W attempts to weave $p@W into some type of clothing but ends up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
          return;
        } else {
          weaved = read_object(16706, VIRTUAL);
          obj_to_room(weaved, char_get_in_room(ch));
          if (obj_get_vnum(obj) == 16708) {
            armor *= 20;
            str    = 4;
            intel  = 4;
            price  = 4;
            olevel = 80;
          } else if (obj_get_vnum(obj) == 16700) {
            armor *= 15;
            str    = 3;
            intel  = 3;
            price  = 4;
            olevel = 75;
          } else if (obj_get_vnum(obj) == 16701) {
            armor *= 10;
            str    = 3;
            intel  = 3;
            price  = 3;
            olevel = 50;
          } else if (obj_get_vnum(obj) == 16702) {
            armor *= 6;
            str    = 2;
            intel  = 2;
            price  = 2;
            olevel = 25;
          } else if (obj_get_vnum(obj) == 16703) {
            armor *= 4;
            str    = 1;
            intel  = 1;
            price  = 1.5;
            olevel = 5;
          }
          weaved->affected[0].location = 17;
          weaved->affected[0].modifier = armor;
          obj_set_cost(weaved, obj_get_cost(weaved) * (price));
          obj_set_value(weaved, 0, olevel);
          weaved->level = olevel;
          if (str > 0) {
            weaved->affected[1].location = 1;
            weaved->affected[1].modifier = str;
          }
          if (intel > 0) {
            weaved->affected[2].location = 3;
            weaved->affected[2].modifier = intel;
          }
          act("@WYou attempt to weave the bundle and manage to create $p@W!@n", TRUE, ch, weaved, 0, TO_CHAR);
          act("@C$n@W attempts to weave a bundle into something and manages to create $p@W!@n", TRUE, ch, weaved, 0, TO_ROOM);
          do_get(ch, (char[]){"wristband"}, 0, 0);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
        }
      } else if (!strcasecmp(arg2, "belt")) {
        if (prob <= perc) {
          act("@WYou attempt to weave $p@W into the desired piece but end up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@W attempts to weave $p@W into some type of clothing but ends up ruining the entire bundle instead.@n", TRUE, ch, obj, 0, TO_ROOM);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
          return;
        } else {
          weaved = read_object(16707, VIRTUAL);
          obj_to_room(weaved, char_get_in_room(ch));
          if (obj_get_vnum(obj) == 16708) {
            armor *= 20;
            str    = 4;
            intel  = 4;
            price  = 4;
            olevel = 80;
          } else if (obj_get_vnum(obj) == 16700) {
            armor *= 15;
            str    = 3;
            intel  = 3;
            price  = 4;
            olevel = 75;
          } else if (obj_get_vnum(obj) == 16701) {
            armor *= 10;
            str    = 3;
            intel  = 3;
            price  = 3;
            olevel = 50;
          } else if (obj_get_vnum(obj) == 16702) {
            armor *= 6;
            str    = 2;
            intel  = 2;
            price  = 2;
            olevel = 25;
          } else if (obj_get_vnum(obj) == 16703) {
            armor *= 4;
            str    = 1;
            intel  = 1;
            price  = 1.5;
            olevel = 5;
          }
          weaved->affected[0].location = 17;
          weaved->affected[0].modifier = armor;
          obj_set_cost(weaved, obj_get_cost(weaved) * (price));
          obj_set_value(weaved, 0, olevel);
          weaved->level = olevel;
          if (str > 0) {
            weaved->affected[1].location = 1;
            weaved->affected[1].modifier = str;
          }
          if (intel > 0) {
            weaved->affected[2].location = 3;
            weaved->affected[2].modifier = intel;
          }
          act("@WYou attempt to weave the bundle and manage to create $p@W!@n", TRUE, ch, weaved, 0, TO_CHAR);
          act("@C$n@W attempts to weave a bundle into something and manages to create $p@W!@n", TRUE, ch, weaved, 0, TO_ROOM);
          do_get(ch, (char[]){"belt"}, 0, 0);
          extract_obj(obj);
          char_set_wait(ch, PULSE_4SEC);
        }
      } else {
        send_to_char(ch, "Syntax: silk weave (head | wrist | belt)");
        return;
      }
      return;
    } ////

  } else if (!strcasecmp(arg, "bundle")) {
    int64_t cost = ((getMaxKI(ch) * 0.01) * (prob * 0.20)) + (char_get_aff_abils_intel(ch) * char_get_level(ch));

    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to weave any bundles of silk.\r\n");
      return;
    } else {
      char_set_wait(ch, PULSE_3SEC);
      int super = FALSE, superoll = rand_number(1, 100);
      if (char_is_kurzak(ch)) {
        if (char_get_skill(ch, SKILL_SILK) >= 100) {
          if (8 > superoll) {
            super = TRUE;
          }
        } else if (char_get_skill(ch, SKILL_SILK) >= 60) {
          if (6 > superoll) {
            super = TRUE;
          }
        } else if (char_get_skill(ch, SKILL_SILK) >= 40) {
          if (3 > superoll) {
            super = TRUE;
          }
        }
      }
      if (super == TRUE) {
        obj = read_object(16708, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@YYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@Y piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        send_to_char(ch, "@YIt's SUPER grand!@n\r\n");
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc && prob >= 100) { /* Second Best Quality */
        obj = read_object(16700, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc && prob >= 90) { /* Great Quality */
        obj = read_object(16701, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc && prob >= 80) { /* Good Quality */
        obj = read_object(16702, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc && prob >= 50) { /* Decent Quality */
        obj = read_object(16703, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else if (prob > perc) { /* Bad Quality */
        obj = read_object(16704, VIRTUAL);
        obj_to_room(obj, char_get_in_room(ch));
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
      } else {
        act("@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You end up making a poorly formed puddle of goo...@n", TRUE, ch, obj, 0, TO_CHAR);
        act("@C$n@W seems to concentrate for a moment before spitting out a poorly formed puddle of goo...@n", TRUE, ch, obj, 0, TO_ROOM);
        decCurKI(ch, cost, 0);
        improve_skill(ch, SKILL_SILK, 1);
      }
    }
  } else {
    send_to_char(ch, "Syntax: silk (weave | bundle)\r\n");
    return;
  }
}

static int valid_recipe(struct char_data *ch, int recipe, int type)
{
  /* Plant Variables */
  int tomato = -1, cucumber = -1, onion = -1, greenbean = -1, garlic = -1, redpep = -1;
  int potato = -1, carrot = -1, brownmush = -1, lettuce = -1;
  /* Meat Variables  */
  int normmeat = -1, goodmeat = -1, normfish = -1, goodfish = -1, greatfish = -1, bestfish = -1;
  /* Store */
  int rice = -1, flour = -1, appleplum = -1, fberry = -1, carambola = -1;

  struct obj_data *obj2, *next_obj;
  int pass = FALSE;

  /* Determine ingredients needed for recipe */
  switch (recipe) {
    case RECIPE_TOMATO_SOUP:
      tomato = 2;
      break;
    case RECIPE_STEAK:
      normmeat = 1;
      break;
    case RECIPE_POTATO_SOUP:
      potato = 2;
      break;
    case RECIPE_VEGETABLE_SOUP:
      potato    = 1;
      tomato    = 1;
      carrot    = 1;
      greenbean = 1;
      onion     = 1;
      break;
    case RECIPE_MEAT_STEW:
      normmeat = 1;
      potato   = 1;
      tomato   = 1;
      garlic   = 1;
      break;
    case RECIPE_ROAST:
      normmeat  = 1;
      potato    = 2;
      garlic    = 1;
      onion     = 1;
      greenbean = 3;
      break;
    case RECIPE_CHILI_SOUP:
      normmeat = 1;
      redpep   = 4;
      tomato   = 2;
      break;
    case RECIPE_GRILLED_NORMFISH:
      normfish = 1;
      break;
    case RECIPE_GRILLED_GOODFISH:
      goodfish = 1;
      break;
    case RECIPE_GRILLED_GREATFISH:
      greatfish = 1;
      break;
    case RECIPE_GRILLED_BESTFISH:
      bestfish = 1;
      break;
    case RECIPE_COOKED_RICE:
      rice = 1;
      break;
    case RECIPE_SUSHI:
      rice     = 1;
      normfish = 1;
      break;
    case RECIPE_BREAD:
      flour = 1;
      break;
    case RECIPE_SALAD:
      tomato   = 1;
      cucumber = 1;
      carrot   = 1;
      lettuce  = 1;
      break;
    case RECIPE_APPLEPLUM:
      flour     = 1;
      appleplum = 1;
      break;
    case RECIPE_FBERRY_MUFFIN:
      flour  = 1;
      fberry = 1;
      break;
    case RECIPE_CARAMBOLA_BREAD:
      flour     = 1;
      carambola = 1;
      break;
  }

  if (type == 0) {
    /* Check for ingredients in inventory */
    for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
      next_obj = obj2->next_content;
      switch (obj_get_vnum(obj2)) {
        case RCP_TOMATO:
          if (tomato > 0) {
            tomato -= 1;
          }
          break;
        case RCP_NORMAL_MEAT:
          if (normmeat > 0) {
            normmeat -= 1;
          }
          break;
        case RCP_POTATO:
          if (potato > 0) {
            potato -= 1;
          }
          break;
        case RCP_ONION:
          if (onion > 0) {
            onion -= 1;
          }
          break;
        case RCP_CUCUMBER:
          if (cucumber > 0) {
            cucumber -= 1;
          }
          break;
        case RCP_CHILIPEPPER:
          if (redpep > 0) {
            redpep -= 1;
          }
          break;
        case RCP_CARROT:
          if (carrot > 0) {
            carrot -= 1;
          }
          break;
        case RCP_GREENBEAN:
          if (greenbean > 0) {
            greenbean -= 1;
          }
          break;
        case RCP_BLACKBASS:
        case RCP_FLOUNDER:
        case RCP_NARRI:
        case RCP_GRAVELREBOI:
          if (normfish > 0) {
            normfish -= 1;
          }
          break;
        case RCP_SILVERTROUT:
        case RCP_SILVEREEL:
        case RCP_VALBISH:
        case RCP_VOOSPIKE:
          if (goodfish > 0) {
            goodfish -= 1;
          }
          break;
        case RCP_STRIPEDBASS:
        case RCP_COBIA:
        case RCP_GUSBLAT:
        case RCP_SHADOWFISH:
          if (greatfish > 0) {
            greatfish -= 1;
          }
          break;
        case RCP_BLUECATFISH:
        case RCP_TAMBOR:
        case RCP_REPEEIL:
        case RCP_SHADEEEL:
          if (bestfish > 0) {
            bestfish -= 1;
          }
          break;
        case RCP_BROWNMUSH:
          if (brownmush > 0) {
            brownmush -= 1;
          }
          break;
        case RCP_GARLIC:
          if (garlic > 0) {
            garlic -= 1;
          }
          break;
        case RCP_RICE:
          if (rice > 0) {
            rice -= 1;
          }
          break;
        case RCP_FLOUR:
          if (flour > 0) {
            flour -= 1;
          }
          break;
        case RCP_LETTUCE:
          if (lettuce > 0) {
            lettuce -= 1;
          }
          break;
        case RCP_APPLEPLUM:
          if (appleplum > 0) {
            appleplum -= 1;
          }
          break;
        case RCP_FROZENBERRY:
          if (fberry > 0) {
            fberry -= 1;
          }
          break;
        case RCP_CARAMBOLA:
          if (carambola > 0) {
            carambola -= 1;
          }
          break;
      }
    }
  } else { /* We know the ingredients are there, remove and exit. */
    for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
      next_obj = obj2->next_content;
      switch (obj_get_vnum(obj2)) {
        case RCP_TOMATO:
          if (tomato > 0) {
            tomato -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_NORMAL_MEAT:
          if (normmeat > 0) {
            normmeat -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_POTATO:
          if (potato > 0) {
            potato -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_ONION:
          if (onion > 0) {
            onion -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_CUCUMBER:
          if (cucumber > 0) {
            cucumber -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_CHILIPEPPER:
          if (redpep > 0) {
            redpep -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_CARROT:
          if (carrot > 0) {
            carrot -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_GREENBEAN:
          if (greenbean > 0) {
            greenbean -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_BLACKBASS:
        case RCP_FLOUNDER:
        case RCP_NARRI:
        case RCP_GRAVELREBOI:
          if (normfish > 0) {
            normfish -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_SILVERTROUT:
        case RCP_SILVEREEL:
        case RCP_VALBISH:
        case RCP_VOOSPIKE:
          if (goodfish > 0) {
            goodfish -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_STRIPEDBASS:
        case RCP_COBIA:
        case RCP_GUSBLAT:
        case RCP_SHADOWFISH:
          if (greatfish > 0) {
            greatfish -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_BLUECATFISH:
        case RCP_TAMBOR:
        case RCP_REPEEIL:
        case RCP_SHADEEEL:
          if (bestfish > 0) {
            bestfish -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_BROWNMUSH:
          if (brownmush > 0) {
            brownmush -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_GARLIC:
          if (garlic > 0) {
            garlic -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_RICE:
          if (rice > 0) {
            rice -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_FLOUR:
          if (flour > 0) {
            flour -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_LETTUCE:
          if (lettuce > 0) {
            lettuce -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_APPLEPLUM:
          if (appleplum > 0) {
            appleplum -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_FROZENBERRY:
          if (fberry > 0) {
            fberry -= 1;
            extract_obj(obj2);
          }
          break;
        case RCP_CARAMBOLA:
          if (carambola > 0) {
            carambola -= 1;
            extract_obj(obj2);
          }
          break;
      }
    }
    return (TRUE);
    /* We'll exit here after removing the ingredients, for safety */
  }

  /* Make sure all ingredients were accounted for and pass if so */
  switch (recipe) {
    case RECIPE_TOMATO_SOUP:
      if (tomato == 0) {
        pass = TRUE;
      }
      break;
    case RECIPE_STEAK:
      if (normmeat == 0) {
        pass = TRUE;
      }
      break;
    case RECIPE_POTATO_SOUP:
      if (potato == 0) {
        pass = TRUE;
      }
      break;
    case RECIPE_VEGETABLE_SOUP:
      if (potato == 0 && tomato == 0 && carrot == 0 && greenbean == 0 && onion == 0) {
        pass = TRUE;
      }
      break;
    case RECIPE_MEAT_STEW:
      if (normmeat == 0 && potato == 0 && tomato == 0 && garlic == 0)
        pass = TRUE;
      break;
    case RECIPE_ROAST:
      if (normmeat == 0 && potato == 0 && garlic == 0 && onion == 0 && greenbean == 0)
        pass = TRUE;
      break;
    case RECIPE_CHILI_SOUP:
      if (normmeat == 0 && redpep == 0 && tomato == 0)
        pass = TRUE;
      break;
    case RECIPE_GRILLED_NORMFISH:
      if (normfish == 0)
        pass = TRUE;
      break;
    case RECIPE_GRILLED_GOODFISH:
      if (goodfish == 0)
        pass = TRUE;
      break;
    case RECIPE_GRILLED_GREATFISH:
      if (greatfish == 0)
        pass = TRUE;
      break;
    case RECIPE_GRILLED_BESTFISH:
      if (bestfish == 0)
        pass = TRUE;
      break;
    case RECIPE_COOKED_RICE:
      if (rice == 0)
        pass = TRUE;
      break;
    case RECIPE_SUSHI:
      if (rice == 0 && normfish == 0)
        pass = TRUE;
      break;
    case RECIPE_BREAD:
      if (flour == 0)
        pass = TRUE;
      break;
    case RECIPE_SALAD:
      if (tomato == 0 && cucumber == 0 && carrot == 0 && lettuce == 0)
        pass = TRUE;
      break;
    case RECIPE_APPLEPLUM:
      if (flour == 0 && appleplum == 0)
        pass = TRUE;
      break;
    case RECIPE_FBERRY_MUFFIN:
      if (flour == 0 && fberry == 0)
        pass = TRUE;
      break;
    case RECIPE_CARAMBOLA_BREAD:
      if (flour == 0 && carambola == 0)
        pass = TRUE;
      break;
  }

  if (pass == FALSE) {
    if (tomato > 0) {
      send_to_char(ch, "@WYou need @m%d@W tomato%s for this recipe.@n\r\n", tomato, tomato > 1 ? "es" : "");
    }
    if (potato > 0) {
      send_to_char(ch, "@WYou need @m%d@W potato%s for this recipe.@n\r\n", potato, potato > 1 ? "es" : "");
    }
    if (onion > 0) {
      send_to_char(ch, "@WYou need @m%d@W onion%s for this recipe.@n\r\n", onion, onion > 1 ? "s" : "");
    }
    if (appleplum > 0) {
      send_to_char(ch, "@WYou need @m%d@W appleplum%s for this recipe.@n\r\n", appleplum, appleplum > 1 ? "s" : "");
    }
    if (fberry > 0) {
      send_to_char(ch, "@WYou need @m%d@W frozen berry%s for this recipe.@n\r\n", fberry, fberry > 1 ? "s" : "");
    }
    if (carambola > 0) {
      send_to_char(ch, "@WYou need @m%d@W carambola%s for this recipe.@n\r\n", carambola, carambola > 1 ? "s" : "");
    }
    if (lettuce > 0) {
      send_to_char(ch, "@WYou need @m%d@W head%s of lettuce for this recipe.@n\r\n", lettuce, lettuce > 1 ? "s" : "");
    }
    if (flour > 0) {
      send_to_char(ch, "@WYou need @m%d@W cup%s of white flour for this recipe.@n\r\n", flour, flour > 1 ? "s" : "");
    }
    if (rice > 0) {
      send_to_char(ch, "@WYou need @m%d@W cup%s of white rice for this recipe.@n\r\n", rice, rice > 1 ? "s" : "");
    }
    if (garlic > 0) {
      send_to_char(ch, "@WYou need @m%d@W garlic clove%s for this recipe.@n\r\n", garlic, garlic > 1 ? "s" : "");
    }
    if (carrot > 0) {
      send_to_char(ch, "@WYou need @m%d@W carrot%s for this recipe.@n\r\n", carrot, carrot > 1 ? "s" : "");
    }
    if (cucumber > 0) {
      send_to_char(ch, "@WYou need @m%d@W cucumber%s for this recipe.@n\r\n", cucumber, cucumber > 1 ? "s" : "");
    }
    if (greenbean > 0) {
      send_to_char(ch, "@WYou need @m%d@W green bean%s for this recipe.@n\r\n", greenbean, greenbean > 1 ? "s" : "");
    }
    if (normmeat > 0) {
      send_to_char(ch, "@WYou need @m%d@W normal raw steak%s for this recipe.@n\r\n", normmeat, normmeat > 1 ? "s" : "");
    }
    if (goodmeat > 0) {
      send_to_char(ch, "@WYou need @m%d@W good raw steak%s for this recipe.@n\r\n", goodmeat, goodmeat > 1 ? "s" : "");
    }
    if (redpep > 0) {
      send_to_char(ch, "@WYou need @m%d@W chili pepper%s for this recipe.@n\r\n", redpep, redpep > 1 ? "s" : "");
    }
    if (normfish > 0) {
      send_to_char(ch, "@WYou need @m%d@W black bass, flounder, narri, or gravel reboi for this recipe.@n\r\n", normfish);
    }
    if (goodfish > 0) {
      send_to_char(ch, "@WYou need @m%d@W silver trout, silver eel, valbish, or voos pike for this recipe.@n\r\n", goodfish);
    }
    if (greatfish > 0) {
      send_to_char(ch, "@WYou need @m%d@W striped bass, cobia, gusblat, or shadowfish for this recipe.@n\r\n", greatfish);
    }
    if (bestfish > 0) {
      send_to_char(ch, "@WYou need @m%d@W blue catfish, tambor, repeeil, or shadeeel for this recipe.@n\r\n", bestfish);
    }
    if (brownmush > 0) {
      send_to_char(ch, "@WYou need @m%d@W brown mushroom%s for this recipe.@n\r\n", brownmush, brownmush > 1 ? "s" : "");
    }
    return (FALSE);
  } else {
    return (TRUE);
  }
}

static int campfire_cook(int recipe)
{

  switch (recipe) {
    case RECIPE_STEAK:
    case RECIPE_GRILLED_NORMFISH:
    case RECIPE_GRILLED_GOODFISH:
    case RECIPE_GRILLED_GREATFISH:
    case RECIPE_GRILLED_BESTFISH:
    case RECIPE_ROAST:
      return (TRUE);
      break;
  }

  return (FALSE);
}

ACMD(do_cook)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_is_npc(ch))
    return;

  int cook_elem = cook_element(char_get_in_room(ch));

  if (!cook_elem) {
    send_to_char(ch, "You need a campfire or Flambus Stove nearby to cook.\r\n");
    return;
  }

  if (!char_get_skill(ch, SKILL_COOKING)) {
    send_to_char(ch, "You don't even know the basics!\r\n");
    return;
  }

  int skill = char_get_skill(ch, SKILL_COOKING), prob = axion_dice(0);

  if (!*arg) {
    send_to_char(ch, "@D---------------------@RCooking@D---------------------@n\r\n");
    send_to_char(ch, "@Y 1@B) @CCooked Steak		@Y17@B) @CCarambola Bread@n\r\n");
    send_to_char(ch, "@Y 2@B) @CTomato Soup		@n\r\n");
    send_to_char(ch, "@Y 3@B) @CPotato Soup		@n\r\n");
    send_to_char(ch, "@Y 4@B) @CVegetable Soup		@n\r\n");
    send_to_char(ch, "@Y 5@B) @CMeat Stew			@n\r\n");
    send_to_char(ch, "@Y 6@B) @CChili Soup		@n\r\n");
    send_to_char(ch, "@Y 7@B) @CGrilled Fish		@n\r\n");
    send_to_char(ch, "@Y 8@B) @CGood Grilled Fish		@n\r\n");
    send_to_char(ch, "@Y 9@B) @CGreat Grilled Fish	@n\r\n");
    send_to_char(ch, "@Y10@B) @CMagnificent Grilled Fish	@n\r\n");
    send_to_char(ch, "@Y11@B) @CCooked White Rice		@n\r\n");
    send_to_char(ch, "@Y12@B) @CSushi			@n\r\n");
    send_to_char(ch, "@Y13@B) @CWhite Bread		@n\r\n");
    send_to_char(ch, "@Y14@B) @CBasic Salad		@n\r\n");
    send_to_char(ch, "@Y15@B) @CAppleplum Chasan		@n\r\n");
    send_to_char(ch, "@Y16@B) @CFrozen Berry Muffin	@n\r\n");
    send_to_char(ch, "@wSyntax: cook (recipe number)@n\r\n");
    return;
  } else {
    int num = atoi(arg), pass = FALSE;
    struct obj_data *meal = NULL;

    int recipe = -1;
    switch (num) {
      case 1:
        recipe = RECIPE_STEAK;
        prob += 8;
        break;
      case 2:
        recipe = RECIPE_TOMATO_SOUP;
        break;
      case 3:
        recipe = RECIPE_POTATO_SOUP;
        break;
      case 4:
        recipe = RECIPE_VEGETABLE_SOUP;
        break;
      case 5:
        recipe = RECIPE_MEAT_STEW;
        break;
      case 6:
        recipe = RECIPE_CHILI_SOUP;
        break;
      case 7:
        recipe = RECIPE_GRILLED_NORMFISH;
        prob += 6;
        break;
      case 8:
        recipe = RECIPE_GRILLED_GOODFISH;
        prob += 10;
        break;
      case 9:
        recipe = RECIPE_GRILLED_GREATFISH;
        prob += 12;
        break;
      case 10:
        recipe = RECIPE_GRILLED_BESTFISH;
        prob += 16;
        break;
      case 11:
        recipe = RECIPE_COOKED_RICE;
        break;
      case 12:
        recipe = RECIPE_SUSHI;
        break;
      case 13:
        recipe = RECIPE_BREAD;
        break;
      case 14:
        recipe = RECIPE_SALAD;
        break;
      case 15:
        recipe = RECIPE_APPLEPLUM;
        break;
      case 16:
        recipe = RECIPE_FBERRY_MUFFIN;
        break;
      case 17:
        recipe = RECIPE_CARAMBOLA_BREAD;
        break;
    }

    if (recipe == -1) {
      send_to_char(ch, "That is not a valid dish!\r\n");
      return;
    }

    if (!valid_recipe(ch, recipe, 0)) {
      return;
    } else if (cook_elem == 1 && !campfire_cook(recipe)) {
      send_to_char(ch, "You can not cook that dish over a campfire.\r\n");
      return;
    } else {
      valid_recipe(ch, recipe, 1);
      pass = TRUE;
    }
    if (pass == TRUE) {
      if (skill < prob) {
        act("@wYou screw up the preparation of the recipe and end up wasting the ingredients!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@w starts to prepare some food, but ends up ruining the ingredients instead!@n", TRUE, ch, 0, 0, TO_ROOM);
        improve_skill(ch, SKILL_COOKING, 0);
        char_set_wait(ch, PULSE_2SEC);
        return;
      } /* End failed to cook it right */

      int psbonus = 0, expbonus = 0;

      switch (num) {
        case 1:
          meal = read_object(MEAL_STEAK, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 5;
          break;
        case 2:
          meal = read_object(MEAL_TOMATO_SOUP, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 2;
          expbonus = 15;
          break;
        case 3:
          meal = read_object(MEAL_POTATO_SOUP, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 20;
          break;
        case 4:
          meal = read_object(MEAL_VEGETABLE_SOUP, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 3;
          expbonus = 45;
          break;
        case 5:
          meal = read_object(MEAL_MEAT_STEW, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 2;
          expbonus = 50;
          break;
        case 6:
          meal = read_object(MEAL_CHILI_SOUP, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 0;
          expbonus = 100;
          break;
        case 7:
          meal = read_object(MEAL_NORM_FISH, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 2;
          expbonus = 12;
          break;
        case 8:
          meal = read_object(MEAL_GOOD_FISH, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 3;
          expbonus = 40;
          break;
        case 9:
          meal = read_object(MEAL_GREAT_FISH, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 5;
          expbonus = 80;
          break;
        case 10:
          meal = read_object(MEAL_BEST_FISH, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 7;
          expbonus = 125;
          break;
        case 11:
          meal = read_object(MEAL_COOKED_RICE, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 8;
          break;
        case 12:
          meal = read_object(MEAL_SUSHI, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 2;
          expbonus = 20;
          break;
        case 13:
          meal = read_object(MEAL_BREAD, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 8;
          break;
        case 14:
          meal = read_object(MEAL_SALAD, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 5;
          expbonus = 8;
          break;
        case 15:
          meal = read_object(MEAL_APPLEPLUM, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 9;
          break;
        case 16:
          meal = read_object(MEAL_FBERRY_MUFFIN, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 3;
          expbonus = 12;
          break;
        case 17:
          meal = read_object(MEAL_CARAMBOLA_BREAD, VIRTUAL);
          obj_to_char(meal, ch);
          psbonus  = 1;
          expbonus = 9;
          break;
        default:
          send_to_char(ch, "That is not a valid dish!\r\n");
          return;
      }
      if (char_get_bonuses(ch, BONUS_RECIPE)) {
        psbonus += 1;
        expbonus += 3;
      }
      act("@wYou carefully prepare the ingredients and then start cooking them. After a while of patience  and skillful care you successfully make @D'@C$p@D'@w!@n", TRUE, ch, meal, 0, TO_CHAR);
      act("@C$n@w carefully prepares some ingredients and starts cooking them. After a while of patience and skillful care $e succeeds in making @D'@C$p@D'@w!@n", TRUE, ch, meal, 0, TO_ROOM);
      improve_skill(ch, SKILL_COOKING, 0);

      if (psbonus > 0) {
        if (skill * 0.10 > 0)
          psbonus = (skill * 0.10) * psbonus;
      }
      if (expbonus > 0) {
        expbonus = skill * expbonus;
      }

      obj_set_value(meal, VAL_FOOD_PSBONUS, psbonus);
      obj_set_value(meal, VAL_FOOD_EXPBONUS, expbonus);

      char_set_wait(ch, PULSE_2SEC);
    } /* End has ingredients */
  }
}

/* Feed senzu to someone you are grouped with. Why? TEAMWORK! */
ACMD(do_feed)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct obj_data *obj;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Feed a senzu to whom?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "That target isn't here.\r\n");
    return;
  }

  if (char_is_android(vict)) {
    send_to_char(ch, "They are unaffected by senzu beans.\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
    send_to_char(ch, "You need to give them a senzu.\r\n");
    return;
  }

  if (obj_get_type_flag(obj) != ITEM_POTION) {
    send_to_char(ch, "You can only feed senzu beans.\r\n");
    return;
  }

  if (obj_extra_flagged(obj, ITEM_FORGED)) {
    send_to_char(ch, "They can't swallow that, it is fake!\r\n");
    return;
  }

  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    send_to_char(ch, "They can't swallow that, it is broken!\r\n");
    return;
  }

  if (char_get_fighting(vict)) {
    send_to_char(ch, "They are a bit busy at the moment!\r\n");
    return;
  }

  if (vict->master != ch && ch->master != vict && ch->master != vict->master) {
    send_to_char(ch, "You need to be grouped with them first.\r\n");
    return;
  }

  if (!char_aff_flagged(vict, AFF_GROUP) || !char_aff_flagged(ch, AFF_GROUP)) {
    send_to_char(ch, "You need to be grouped with them first.\r\n");
    return;
  }

  act("@WYou take $p@W and pop it into @C$N@W's mouth!@n", TRUE, ch, obj, vict, TO_CHAR);
  act("@C$n@W takes $p@W and pops it into YOUR mouth!@n", TRUE, ch, obj, vict, TO_VICT);
  act("@C$n@W takes $p@W and pops it into @c$N@W's mouth!@n", TRUE, ch, obj, vict, TO_NOTVICT);
  mag_objectmagic(vict, obj, (char[]){""});
}

/* This allows players to decapitate a corpse for a sick trophy. */
ACMD(do_spoil)
{

  if (char_is_npc(ch))
    return;

  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int type = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What corpse do you want to decapitate?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[char_get_in_room(ch)].contents))) {
    send_to_char(ch, "No corpse around here by that name.\r\n");
    return;
  }

  if (obj_get_value(obj, VAL_CORPSE_HEAD) == 0) {
    send_to_char(ch, "That corpse is already missing its head.\r\n");
    return;
  }

  if (char_get_equipment(ch, WEAR_WIELD1)) {
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      type = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      type = 1;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD1), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      type = 1;
    }
  } else if (char_get_equipment(ch, WEAR_WIELD2)) {
    if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
      type = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
      type = 2;
    } else if (obj_get_value(char_get_equipment(ch, WEAR_WIELD2), VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
      type = 2;
    }
  }

  if (type == 0) {
    act("@C$n@W reaches down and @rtears@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    act("@WYou reach down and @rtear@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
  } else if (type == 1) {
    act("@C$n@W reaches down and @rcuts@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    act("@WYou reach down and @rcut@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
  } else if (type == 2) {
    act("@C$n@W reaches down and @rcuts@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_ROOM);
    act("@WYou reach down and @rcut@W the head off of @R$p@W!@n", TRUE, ch, obj, 0, TO_CHAR);
  }

  obj_set_value(obj, VAL_CORPSE_HEAD, 0);

  struct obj_data *body_part;
  char part[1000];
  char buf[1000];
  char buf2[1000];
  char buf3[1000];

  *part = '\0';
  *buf  = '\0';
  *buf2 = '\0';
  *buf3 = '\0';

  body_part              = create_obj();
  body_part->item_number = NOTHING;
  obj_set_in_room(body_part, NOWHERE);
  snprintf(part, sizeof(part), "%s", obj->name);
  search_replace(part, "headless", "");
  search_replace(part, "corpse", "");
  search_replace(part, "half", "");
  search_replace(part, "burnt", "");
  search_replace(part, "chunks", "");
  search_replace(part, "beaten", "");
  search_replace(part, "bloody", "");
  trim(part);
  snprintf(buf, sizeof(buf), "bloody head %s", part);
  snprintf(buf2, sizeof(buf2), "@wThe bloody head of %s@w is lying here@n", part);
  snprintf(buf3, sizeof(buf3), "@wThe bloody head of %s@w@n", part);

  body_part->name              = strdup(buf);
  body_part->description       = strdup(buf2);
  body_part->short_description = strdup(buf3);

  obj_set_type_flag(body_part, ITEM_OTHER);
  obj_wear_set(body_part, ITEM_WEAR_TAKE, true);
  obj_extra_set(body_part, ITEM_UNIQUE_SAVE, true);
  obj_set_value(body_part, 0, 0);
  obj_set_value(body_part, 1, 0);
  obj_set_value(body_part, 2, 0);
  obj_set_value(body_part, 3, 0);
  obj_set_value(body_part, 4, 1);
  obj_set_value(body_part, 5, 1);
  obj_set_weight(body_part, rand_number(4, 10));
  obj_set_cost_per_day(body_part, 0);
  add_unique_id(body_part);
  obj_to_room(body_part, char_get_in_room(ch));
  obj_from_room(body_part);
  obj_to_char(body_part, ch);
}
