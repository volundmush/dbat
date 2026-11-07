/* ************************************************************************
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <cstdlib>

#include "dbat/iterate.hpp"
#include "dbat/act.item.hpp"
#include "dbat/consts/auction.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/recipes.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/random.hpp"
#include "dbat/extract.hpp"
#include "dbat/relocate.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/affect.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.other.hpp"
#include "dbat/act.comm.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/config.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/feats.hpp"
#include "dbat/guild.hpp"
#include "dbat/genzon.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/boards.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/game_world.hpp"

static int64_t max_carry_weight(GameWorld *g, struct char_data *ch);

/* global variables */

/* local vvariables  */

static const char *const auctioneer[AUC_BID + 1] = {

    "@D[@CAUCTION@c: @C$n@W puts $p@W up for sale at @Y%d@W zenni.@D]@n",
    "@D[@CAUCTION@c: @W$p@W at @Y%d@W zenni going once!@D]@n",
    "@D[@CAUCTION@c: @W$p@W at @Y%d@W zenni going twice!@D]@n",
    "@D[@CAUCTION@c: @WLast call: $p@W going for @Y%d@W zenni.@D]@n",
    "@D[@CAUCTION@c: @WUnfortunately $p@W is unsold, returning it to $n. @D]@n",
    "@D[@CAUCTION@c: @WSOLD! $p@W to @C$n@W for @Y%d@W zenni!@D]@n",
    "@D[@CAUCTION@c: @WSorry, @C$n@W has cancelled the auction.@D]@n",
    "@D[@CAUCTION@c: @WSorry, @C$n@W has left us, the auction can't go on.@D]@n",
    "@D[@CAUCTION@c: @WSorry, $p@W has been confiscated, shame on you $n.@D]@n",
    "@D[@CAUCTION@c: @C$n@W is selling $p@W for @Y%d@W zenni.@D]@n",
    "@D[@CAUCTION@c: @C$n@W bids @Y%d@W zenni on $p@W.@D]@n"};

/* local functions */
static void majin_gain(GameWorld *g, struct char_data *ch, int type);

static bool can_take_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj);
static void get_check_money(GameWorld *g, struct char_data *ch, struct obj_data *obj);
static void get_from_room(GameWorld *g, struct char_data *ch, char *arg, int howmany);
static void perform_give_gold(GameWorld *g, struct char_data *ch, struct char_data *vict, int amount);
static void perform_give(GameWorld *g, struct char_data *ch, struct char_data *vict, struct obj_data *obj);
static int perform_drop(GameWorld *g, struct char_data *ch, struct obj_data *obj, int8_t mode, const char *sname, room_rnum RDR);
static void perform_drop_gold(GameWorld *g, struct char_data *ch, int amount, int8_t mode, room_rnum RDR);
static struct char_data *give_find_vict(GameWorld *g, struct char_data *ch, char *arg);
static void perform_put(GameWorld *g, struct char_data *ch, struct obj_data *obj, struct obj_data *cont);
static void get_from_container(GameWorld *g, struct char_data *ch, struct obj_data *cont, char *arg, int mode, int howmany);
static void wear_message(GameWorld *g, struct char_data *ch, struct obj_data *obj, int where);
static void perform_get_from_container(GameWorld *g, struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int mode);
static int hands(struct char_data *ch);
static void start_auction(GameWorld *g, struct char_data *ch, struct obj_data *obj, int bid);
static void auc_stat(GameWorld *g, struct char_data *ch, struct obj_data *obj);
static void auc_send_to_all(GameWorld *g, const char *messg, bool buyer);
static bool has_housekey(GameWorld *g, struct char_data *ch, struct obj_data *obj);
static void harvest_plant(GameWorld *g, struct char_data *ch, struct obj_data *plant);
static bool can_harvest(GameWorld *g, struct obj_data *plant);
static char *find_exdesc_keywords(char *word, struct extra_descr_data *list);

/* local variables */
static char buf[MAX_STRING_LENGTH];

// definitions

ACMD(do_refuel)
{

  struct obj_data *controls;

  if (!(controls = find_control(g, ch))) {
    send_to_char(g, ch, "@wYou need to be in the cockpit to place a new fuel canister into the ship.\r\n");
    return;
  }

  struct obj_data *fuel = nullptr;

  for (obj_data *rep : char_inventory(g, ch)) {
    if (obj_get_vnum(g, rep) == 17290) {
      fuel = rep;
    }
  }

  if (fuel == nullptr) {
    send_to_char(g, ch, "You do not have any fuel canisters on you.\r\n");
    return;
  }

  int max = 0;

  if (obj_get_vnum(g, controls) >= 44000 && obj_get_vnum(g, controls) <= 44199) {
    max = 300;
  } else if (obj_get_vnum(g, controls) >= 44200 && obj_get_vnum(g, controls) <= 44499) {
    max = 500;
  } else if (obj_get_vnum(g, controls) >= 44200 && obj_get_vnum(g, controls) <= 44999) {
    max = 1000;
  }

  if (obj_get_value(controls, 2) == max) {
    send_to_char(g, ch, "The ship is full on fuel!\r\n");
    return;
  }

  if (obj_get_value(controls, 2) + (obj_get_weight(fuel) * 4) > max) {
    obj_set_value(controls, 2, max);
  } else {
    obj_set_value(controls, 2, obj_get_value(controls, 2) + ((obj_get_weight(fuel) * 4)));
  }

  extract_obj(g, fuel);

  send_to_char(g, ch, "You place the fuel canister into the ship. Within seconds the fuel has been extracted from the canister into the ships' internal tanks.\r\n");
}

/* The plants that can be harvested: what they yield, and how the yield is adjusted. */
static const struct harvest {
  obj_vnum plant, fruit;
  int bonus;  /* added to the yield */
  bool kills; /* harvesting kills the plant */
} harvests[] = {
    {250,   1,     0,  false}, /* at most 2 */
    {1129,  1131,  0,  true },
    {17210, 17212, 2,  true },
    {17211, 17213, 2,  true },
    {17214, 17215, 1,  true },
    {17216, 17217, 0,  true },
    {17218, 17219, 14, true },
    {17220, 17221, 0,  true }, /* a quarter */
    {17222, 17223, 0,  true }, /* plus 1 to 3 */
    {17224, 17225, 10, true },
    {17226, 17227, -8, true },
    {3702,  3703,  -2, true },
};

static const struct harvest *find_harvest(GameWorld *g, struct obj_data *plant)
{
  for (size_t i = 0; i < countof(harvests); i++)
    if (harvests[i].plant == obj_get_vnum(g, plant))
      return &harvests[i];
  return nullptr;
}

static bool can_harvest(GameWorld *g, struct obj_data *plant)
{
  return find_harvest(g, plant) != nullptr;
}

static void harvest_plant(GameWorld *g, struct char_data *ch, struct obj_data *plant)
{
  int extract = false, reward = rand_number(g, 5, 15), count = reward;
  struct obj_data *fruit = nullptr;

  if (obj_get_value(plant, VAL_SOILQ) > 7) {
    reward += 10;
    send_to_char(g, ch, "@GThe soil seems to have made the plant exteremely bountiful");
  } else if (obj_get_value(plant, VAL_SOILQ) >= 5) {
    reward += 6;
    send_to_char(g, ch, "@GThe soil seems to have made the plant very bountiful");
  } else if (obj_get_value(plant, VAL_SOILQ) >= 3) {
    reward += 4;
    send_to_char(g, ch, "@GThe soil seems to have made the plant bountiful");
  } else if (obj_get_value(plant, VAL_SOILQ) > 0) {
    reward += 2;
    send_to_char(g, ch, "@GThe soil seems to have made the plant a bit more bountiful");
  }

  int skill = char_get_skill(ch, SKILL_GARDENING);

  if (skill >= 100) {
    reward += 10;
    send_to_char(g, ch, " and your outstanding skill has helped the plant be more bountiful yet!@n\r\n");
  } else if (skill >= 90) {
    reward += 8;
    send_to_char(g, ch, " and your great skill has also helped the plant be more bountiful yet!@n\r\n");
  } else if (skill >= 80) {
    reward += 5;
    send_to_char(g, ch, " and your good skill has also helped the plant be more bountiful yet!@n\r\n");
  } else if (skill >= 50) {
    reward += 3;
    send_to_char(g, ch, " and your decent skill has also helped the plant be more bountiful yet!@n\r\n");
  } else if (skill >= 40) {
    reward += 2;
    send_to_char(g, ch, " and your mastery of the basics of gardening has also helped the plant be more bountiful yet!@n\r\n");
  } else if (skill >= 30) {
    reward += 1;
    send_to_char(g, ch, " and you somehow managed to make the plant slightly more bountiful with what little you know!@n\r\n");
  } else {
    send_to_char(g, ch, ".@n\r\n");
  }

  const struct harvest *h = find_harvest(g, plant);

  if (!h) {
    send_to_imm(g, "ERROR: Harvest plant called for illegitimate plant, VNUM %d.", obj_get_vnum(g, plant));
    return;
  }
  if (h->plant == 250)
    reward = MIN(reward, 2);
  else if (h->plant == 17220)
    reward -= reward * 0.75;
  else if (h->plant == 17222)
    reward += rand_number(g, 1, 3);
  reward += h->bonus;
  if (reward < 1) /* every harvest yields something */
    reward = 1;

  for (count = 0; count < reward; count++) {
    if (!(fruit = read_object(g, h->fruit, VIRTUAL))) {
      send_to_imm(g, "ERROR: Harvest plant %d yields missing object %d.", h->plant, h->fruit);
      return;
    }
    obj_to_char(g, fruit, ch);
  }
  send_to_char(g, ch, "@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, obj_get_short_description(fruit));
  extract = h->kills;

  if (extract == true) {
    send_to_char(g, ch, "@wThe harvesting process has killed the plant. Do not worry, this is normal for that type.@n\r\n");
    extract_obj(g, plant);
  } else {
    obj_set_value(plant, VAL_MATURITY, 3);
    obj_set_value(plant, VAL_GROWTH, 0);
  }
}

ACMD(do_garden)
{

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  two_arguments(argument, arg, arg2);

  if (!char_get_skill(ch, SKILL_GARDENING) && slot_count(ch) + 1 <= char_get_skill_slots(ch)) {
    int numb = rand_number(g, 8, 16);
    char_set_skills_base(ch, SKILL_GARDENING, numb);
    send_to_char(g, ch, "@GYou learn the very basics of gardening.\r\n");
  } else if (!char_get_skill(ch, SKILL_GARDENING) && slot_count(ch) + 1 > char_get_skill_slots(ch)) {
    send_to_char(g, ch, "You need additional skill slots to pick up the skill linked with this.\r\n");
    return;
  }

  if (*arg) {
    if (!strcasecmp(arg, "collect")) {
      int found = false;

      for (obj_data *obj2 : char_inventory(g, ch)) {
        if (obj_get_vnum(g, obj2) == 254 && !obj_extra_flagged(obj2, ITEM_BROKEN) && !obj_extra_flagged(obj2, ITEM_FORGED)) {
          found = true;
        }
      }
      if (found == false) {
        send_to_char(g, ch, "You need a shovel in order to collect soil.\r\n");
        return;
      }
      if (rroom_sect(g, char_get_in_room(ch)) != SECT_FOREST && rroom_sect(g, char_get_in_room(ch)) != SECT_FIELD && rroom_sect(g, char_get_in_room(ch)) != SECT_MOUNTAIN && rroom_sect(g, char_get_in_room(ch)) != SECT_HILLS) {
        send_to_char(g, ch, "You can not collect soil from this area.\r\n");
        return;
      }
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_FERTILE1)) {
        struct obj_data *soil = read_object(g, 255, VIRTUAL);
        obj_to_char(g, soil, ch);
        act(g, "@yYou sink your shovel into the soft ground and manage to dig up a pile of fertile soil!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of fertile soil!@n", true, ch, nullptr, nullptr, TO_ROOM);
        obj_set_value(soil, 0, 8);
        char_set_wait(ch, PULSE_4SEC);
        return;
      }
      if (rroom_flagged(g, char_get_in_room(ch), ROOM_FERTILE2)) {
        struct obj_data *soil = read_object(g, 255, VIRTUAL);
        obj_to_char(g, soil, ch);
        act(g, "@yYou sink your shovel into the soft ground and manage to dig up a pile of good soil!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of good soil!@n", true, ch, nullptr, nullptr, TO_ROOM);
        obj_set_value(soil, 0, rand_number(g, 5, 7));
        char_set_wait(ch, PULSE_4SEC);
        return;
      }
      struct obj_data *soil = read_object(g, 255, VIRTUAL);
      obj_to_char(g, soil, ch);
      act(g, "@yYou sink your shovel into the soft ground and manage to dig up a pile of soil!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of soil!@n", true, ch, nullptr, nullptr, TO_ROOM);
      obj_set_value(soil, 0, rand_number(g, 0, 4));
      char_set_wait(ch, PULSE_4SEC);
      return;
    }
  }

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "Syntax: garden (plant) ( water | harvest | dig | plant | pick )\r\n");
    send_to_char(g, ch, "Syntax: garden collect [Will collect soil from a room with soil.\r\n");
    return;
  }

  if (!rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN1) && !rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN2)) {
    send_to_char(g, ch, "You are not even in a garden!\r\n");
    return;
  }

  if (!strcasecmp(arg2, "plant")) {
    if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "What are you trying to plant?\r\n");
      send_to_char(g, ch, "Syntax: garden (plant in inventory) plant\r\n");
      return;
    }
  } else {
    if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "That plant doesn't seem to be here.\r\n");
      return;
    }
  }

  if (!obj) {
    send_to_char(g, ch, "What plant are you gardening?\r\n");
    return;
  }

  int64_t cost = (getMaxST(g, ch) * 0.005) + rand_number(g, 50, 150);
  int skill    = char_get_skill(ch, SKILL_GARDENING);

  if ((getCurST(g, ch)) < cost) {
    send_to_char(g, ch, "@WYou need at least @G%s@W stamina to garden.\r\n", add_commas(cost));
    return;
  }
  if (!strcasecmp(arg2, "water")) {
    struct obj_data *water = nullptr;
    int found = false;

    for (obj_data *obj2 : char_inventory(g, ch)) {
      if (obj_get_vnum(g, obj2) == 251 && !obj_extra_flagged(obj2, ITEM_FORGED)) {
        found = true;
        water = obj2;
      }
    }
    if (found == false) {
      send_to_char(g, ch, "You do not have any grow water!\r\n");
      return;
    }
    if (obj_get_value(obj, VAL_WATERLEVEL) >= 500) {
      send_to_char(g, ch, "You stop as you realize that the plant already has enough water.\r\n");
      return;
    }
    if (obj_get_value(obj, VAL_WATERLEVEL) <= -10) {
      send_to_char(g, ch, "The plant is dead!\r\n");
      return;
    }
    if (skill < axion_dice(g, 0)) {
      act(g, "@GAs you go to water @g$p@G you end up sloppily wasting about half of it on the ground.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@g$n@G takes a bottle of grow water and sloshes some of it on @g$p@G.@n", true, ch, obj, nullptr, TO_ROOM);

      decCurST(g, ch, cost, 0);
      obj_set_value(obj, VAL_WATERLEVEL, obj_get_value(obj, VAL_WATERLEVEL) + (40));
      if (obj_get_value(obj, VAL_WATERLEVEL) > 500) {
        obj_set_value(obj, VAL_WATERLEVEL, 500);
        send_to_char(g, ch, "@YThe plant is now at full water level.@n\r\n");
      }
      extract_obj(g, water);
      char_set_wait(ch, PULSE_3SEC);
      improve_skill(g, ch, SKILL_GARDENING, 0);
      return;
    }
    act(g, "@GYou calmly and expertly pour the grow water on @g$p@G.@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@g$n@G calmly and expertly pours some grow water on @g$p@G.@n", true, ch, obj, nullptr, TO_ROOM);
    decCurST(g, ch, cost, 0);
    obj_set_value(obj, VAL_WATERLEVEL, obj_get_value(obj, VAL_WATERLEVEL) + (225));
    if (obj_get_value(obj, VAL_WATERLEVEL) >= 500) {
      obj_set_value(obj, VAL_WATERLEVEL, 500);
      send_to_char(g, ch, "@YThe plant is now at full water level.@n\r\n");
    }
    extract_obj(g, water);
    char_set_wait(ch, PULSE_3SEC);
    improve_skill(g, ch, SKILL_GARDENING, 0);
    return;
  }
  if (!strcasecmp(arg2, "harvest")) {
    struct obj_data *clippers = nullptr;
    int found = false;

    for (obj_data *obj2 : char_inventory(g, ch)) {
      if (obj_get_vnum(g, obj2) == 253 && !obj_extra_flagged(obj2, ITEM_BROKEN) && !obj_extra_flagged(obj2, ITEM_FORGED)) {
        found    = true;
        clippers = obj2;
      }
    }
    if (found == false) {
      send_to_char(g, ch, "You do not have any working gardening clippers!\r\n");
      return;
    }
    if (can_harvest(g, obj) == false) {
      send_to_char(g, ch, "You can not harvest that plant. Instead, Syntax: garden (plant) (pick)\r\n");
      return;
    }
    if (obj_get_value(obj, VAL_WATERLEVEL) <= -10) {
      send_to_char(g, ch, "That plant is dead!\r\n");
      return;
    }
    if (obj_get_value(obj, VAL_MATURITY) < obj_get_value(obj, VAL_MAXMATURE)) {
      send_to_char(g, ch, "You stop as you realize that the plant isn't mature enough to harvest.\r\n");
      return;
    }
    if (skill < axion_dice(g, -5)) {
      act(g, "@GAs you go to harvest @g$p@G you end up cutting it in half instead!@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@g$n@G attempts to harvest @g$p@G with $s clippers, but accidently cuts the plant in half!@n", true, ch, obj, nullptr, TO_ROOM);
      decCurST(g, ch, cost, 0);
      extract_obj(g, obj);
      char_set_wait(ch, PULSE_3SEC);
      improve_skill(g, ch, SKILL_GARDENING, 0);
      return;
    }
    act(g, "@GYou calmly and expertly harvest @g$p@G.@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@g$n@G calmly and expertly harvests @g$p@G.@n", true, ch, obj, nullptr, TO_ROOM);
    decCurST(g, ch, cost, 0);
    obj_set_value(clippers, VAL_ALL_HEALTH, obj_get_value(clippers, VAL_ALL_HEALTH) - (1));
    if (obj_get_value(clippers, VAL_ALL_HEALTH) <= 0) {
      send_to_char(g, ch, "The clippers are now too dull to use.\r\n");
      return;
    }
    harvest_plant(g, ch, obj);
    char_set_wait(ch, PULSE_3SEC);
    improve_skill(g, ch, SKILL_GARDENING, 0);
    return;
  }
  if (!strcasecmp(arg2, "dig")) {
    int found = false;

    for (obj_data *obj2 : char_inventory(g, ch)) {
      if (obj_get_vnum(g, obj2) == 254 && !obj_extra_flagged(obj2, ITEM_BROKEN) && !obj_extra_flagged(obj2, ITEM_FORGED)) {
        found = true;
      }
    }
    if (found == false) {
      send_to_char(g, ch, "You do not have any working gardening shovels!\r\n");
      return;
    }
    act(g, "@GYou calmly dig up @g$p@G.@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@g$n@G calmly digs up @g$p@G.@n", true, ch, obj, nullptr, TO_ROOM);
    decCurST(g, ch, cost, 0);
    obj_from_room(g, obj);
    obj_to_char(g, obj, ch);
    obj_set_value(obj, VAL_SOILQ, 0);
    char_set_wait(ch, PULSE_3SEC);
    improve_skill(g, ch, SKILL_GARDENING, 0);
    return;
  }
  if (!strcasecmp(arg2, "plant")) {
    int found = false;

    for (obj_data *obj2 : char_inventory(g, ch)) {
      if (obj_get_vnum(g, obj2) == 254 && !obj_extra_flagged(obj2, ITEM_BROKEN) && !obj_extra_flagged(obj2, ITEM_FORGED)) {
        found = true;
      }
    }
    if (found == false) {
      send_to_char(g, ch, "You do not have any working gardening shovels!\r\n");
      return;
    }
    found                 = false;
    struct obj_data *soil = nullptr;

    for (obj_data *obj2 : char_inventory(g, ch)) {
      if (obj_get_vnum(g, obj2) == 255 && !obj_extra_flagged(obj2, ITEM_FORGED)) {
        found = true;
        soil  = obj2;
      }
    }
    if (found == false) {
      send_to_char(g, ch, "You don't have any real soil.\r\n");
    } else if (check_saveroom_count(g, ch, nullptr) > 7 && rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN1)) {
      send_to_char(g, ch, "This room already has all its planters full. Try digging up some plants.\r\n");
      return;
    } else if (check_saveroom_count(g, ch, nullptr) > 19 && rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN2)) {
      send_to_char(g, ch, "This room already has all its planters full. Try digging up some plants.\r\n");
      return;
    } else if (skill < axion_dice(g, -5)) {
      act(g, "@GYou end up digging a hole too shallow to hold @g$p@G. Better try again.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@g$n@G digs a very shallow hole in one of the planters and then realizes @g$p@G won't fit in it.@n", true, ch, obj, nullptr, TO_ROOM);
      decCurST(g, ch, cost, 0);
      char_set_wait(ch, PULSE_3SEC);
      improve_skill(g, ch, SKILL_GARDENING, 0);
      return;
    } else {
      act(g, "@GYou dig a proper sized hole and plant @g$p@G in it.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@g$n@G digs a proper sized hole in a planter and plants @g$p@G in it.@n", true, ch, obj, nullptr, TO_ROOM);
      obj_from_char(g, obj);
      obj_to_room(g, obj, char_get_in_room(ch));
      decCurST(g, ch, cost, 0);
      obj_set_value(obj, VAL_MAXMATURE, 6);
      obj_set_value(obj, VAL_MATGOAL, 200);
      obj_set_value(obj, VAL_SOILQ, obj_get_value(soil, 0));
      switch (obj_get_value(obj, VAL_SOILQ)) {
        case 1:
          obj_set_value(obj, VAL_MATGOAL, obj_get_value(obj, VAL_MATGOAL) - (10));
          break;
        case 2:
          obj_set_value(obj, VAL_MATGOAL, obj_get_value(obj, VAL_MATGOAL) - (15));
          break;
        case 3:
          obj_set_value(obj, VAL_MATGOAL, obj_get_value(obj, VAL_MATGOAL) - (20));
          break;
        case 4:
          obj_set_value(obj, VAL_MATGOAL, obj_get_value(obj, VAL_MATGOAL) - (25));
          break;
        case 5:
          obj_set_value(obj, VAL_MATGOAL, obj_get_value(obj, VAL_MATGOAL) - (50));
          break;
        case 6:
          obj_set_value(obj, VAL_MATGOAL, obj_get_value(obj, VAL_MATGOAL) - (60));
          break;
        case 7:
          obj_set_value(obj, VAL_MATGOAL, obj_get_value(obj, VAL_MATGOAL) - (70));
          break;
        default:
          obj_set_value(obj, VAL_MATGOAL, obj_get_value(obj, VAL_MATGOAL) - (80));
          break;
      }
      extract_obj(g, soil);
      char_set_wait(ch, PULSE_3SEC);
      improve_skill(g, ch, SKILL_GARDENING, 0);
    }
  } else if (!strcasecmp(arg2, "pick")) {
    if (!obj_extra_flagged(obj, ITEM_MATURE)) {
      send_to_char(g, ch, "You can't pick that type of plant. Syntax: garden (plant) harvest\r\n");
      return;
    }
    if (obj_get_value(obj, VAL_MATURITY) < obj_get_value(obj, VAL_MAXMATURE)) {
      send_to_char(g, ch, "That plant is not mature enough yet.\r\n");
      return;
    }
    if (skill < axion_dice(g, -5)) {
      act(g, "@GYou end up shredding @g$p@G with your clumsy unskilled hands.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@g$n@G grabs a hold of @g$p@G and shreds it in an attempt to pick it.@n", true, ch, obj, nullptr, TO_ROOM);
      return;
    }
    act(g, "@GYou grab a hold of @g$p@G and carefully pick it out of the soil.@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@g$n@G grabs a hold of @g$p@G and carefully picks it out of the soil.@n", true, ch, obj, nullptr, TO_ROOM);
    obj_from_room(g, obj);
    obj_to_char(g, obj, ch);
    decCurST(g, ch, cost, 0);
    char_set_wait(ch, PULSE_3SEC);
    improve_skill(g, ch, SKILL_GARDENING, 0);
    return;
  } else {
    send_to_char(g, ch, "Syntax: garden (plant) ( water | harvest | dig | plant | pick )\r\n");
    send_to_char(g, ch, "Syntax: garden collect [Will collect soil from a room with soil.\r\n");
    return;
  }
}

static bool has_housekey(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{

  for (obj_data *obj2 : char_inventory(g, ch)) {
    if (obj_extra_flagged(obj, ITEM_DUPLICATE))
      continue;
    if (obj_get_vnum(g, obj) == 18802) {
      if (obj_get_vnum(g, obj2) == 18800) {
        return true;
      }
    } else {
      if (obj_get_vnum(g, obj2) == obj_get_vnum(g, obj) - 1) {
        return true;
      }
    }
  }

  return false;
}

ACMD(do_pack)
{

  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(g, ch, "Pack up which type of house capsule?\nSyntax: pack (target)\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
    send_to_char(g, ch, "That house item doesn't seem to be around.\r\n");
    return;
  }
  struct obj_data *packed = nullptr;
  if ((obj_get_vnum(g, obj) >= 19090 && obj_get_vnum(g, obj) <= 19099) || obj_get_vnum(g, obj) == 11) {
    act(g, "@CYou push a hidden button on $p@C and a cloud of smoke erupts and covers it. As the smoke clears a small capsule can be seen on the ground.@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@c$n@C pushes a hidden button on $p@C and a cloud of smokes erupts and covers it. As the smoke clears a small capsule can be seen on the ground.@n", true, ch, obj, nullptr, TO_ROOM);
    if (obj_get_vnum(g, obj) == 11) {
      extract_obj(g, obj);
      packed = read_object(g, 19085, VIRTUAL);
      obj_to_room(g, packed, char_get_in_room(ch));
    } else {
      obj_vnum fnum = obj_get_vnum(g, obj) - 10;
      packed        = read_object(g, fnum, VIRTUAL);
      extract_obj(g, obj);
      obj_to_room(g, packed, char_get_in_room(ch));
    }
    return;
  }
  if (obj_get_vnum(g, obj) >= 18800 && obj_get_vnum(g, obj) <= 19199 && obj_get_type_flag(obj) == ITEM_VEHICLE) {
    if (!*arg2) {
      send_to_char(g, ch, "This will sell off your house and delete everything inside. Are you sure? If you are then enter the command again with a yes at the end.\nSyntax: pack (house) yes\r\n");
      return;
    }
    if (strcasecmp(arg2, "yes")) {
      send_to_char(g, ch, "This will sell off your house and delete everything inside. Are you sure? If you are then enter the command again with a yes at the end.\nSyntax: pack (house) yes\r\n");
      return;
    }
    if (has_housekey(g, ch, obj) == 0) {
      send_to_char(g, ch, "You do not own this house.\r\n");
      return;
    }
    act(g, "@CYou push a hidden button on $p@C and a cloud of smoke erupts and covers it. As the smoke clears a pile of money can be seen on the ground!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@c$n@C pushes a hidden button on $p@C and a cloud of smokes erupts and covers it. As the smoke clears a pile of money can be seen on the ground!@n", true, ch, obj, nullptr, TO_ROOM);
    int money = 0, count = 0;
    IDXTYPE rnum = obj_get_vnum(g, obj);
    if (obj_get_vnum(g, obj) >= 18800 && obj_get_vnum(g, obj) <= 18899) {
      if (rnum == 18802) {
        rnum = 18800;
      } else {
        rnum = rnum - 1;
      }
      money = 65000;
      while (count < 4) {
        while (rroom_get_contents(g, real_room(g, rnum)))
          extract_obj(g, rroom_get_contents(g, real_room(g, rnum)));
        count++;
        rnum++;
      }
    } else if (obj_get_vnum(g, obj) >= 18900 && obj_get_vnum(g, obj) <= 18999) {
      rnum  = rnum - 1;
      money = 150000;
      while (count < 4) {
        while (rroom_get_contents(g, real_room(g, rnum)))
          extract_obj(g, rroom_get_contents(g, real_room(g, rnum)));
        count++;
        rnum++;
      }
    } else if (obj_get_vnum(g, obj) >= 19100 && obj_get_vnum(g, obj) <= 19199) {
      rnum  = rnum - 1;
      money = 1000000;
      while (count < 4) {
        while (rroom_get_contents(g, real_room(g, rnum)))
          extract_obj(g, rroom_get_contents(g, real_room(g, rnum)));
        count++;
        rnum++;
      }
    }
    for (obj_data *obj2 : char_inventory(g, ch)) {
      if (obj_get_vnum(g, obj) == 18802) {
        if (obj_get_vnum(g, obj2) == 18800) {
          extract_obj(g, obj2);
        }
      } else {
        if (obj_get_vnum(g, obj2) == obj_get_vnum(g, obj) - 1) {
          extract_obj(g, obj2);
        }
      }
    }
    struct obj_data *money_obj = create_money(g, money);
    obj_to_room(g, money_obj, char_get_in_room(ch));
    extract_obj(g, obj);
    return;
  }
  send_to_char(g, ch, "That isn't something you can pack up!\r\n");
  return;
}

int check_insidebag(GameWorld *g, struct obj_data *cont, double mult)
{

  int count = 0, containers = 0;

  for (obj_data *inside : obj_contents(g, cont)) {
    if (obj_get_type_flag(inside) == ITEM_CONTAINER) {
      count++;
      count += check_insidebag(g, inside, mult);
      containers++;
    } else {
      count++;
    }
  }

  count = count * mult;
  count += containers;

  return (count);
}

int check_saveroom_count(GameWorld *g, struct char_data *ch, struct obj_data *cont)
{
  int count = 0;

  if (char_get_in_room(ch) == NOWHERE)
    return 0;
  else if (!rroom_flagged(g, char_get_in_room(ch), ROOM_HOUSE))
    return 0;

  for (obj_data *obj : room_contents(g, char_get_in_room(ch))) {
    count++;
    if (!obj_extra_flagged(obj, ITEM_CARDCASE)) {
      count += check_insidebag(g, obj, 0.5);
    }
  }

  if (cont != nullptr) {
    if (!obj_extra_flagged(cont, ITEM_CARDCASE)) {
      count += check_insidebag(g, cont, 0.5);
    }
    count++;
  }

  return (count);
}

ACMD(do_deploy)
{

  struct obj_data *obj = nullptr;
  int capsule = false, furniture = false;

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    for (obj_data *obj4 : char_inventory(g, ch)) {
      if (obj_get_vnum(g, obj4) == 4 || obj_get_vnum(g, obj4) == 5 || obj_get_vnum(g, obj4) == 6) {
        obj     = obj4;
        capsule = true;
      }
    }
  } else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "Syntax: deploy (no argument for houses)\nSyntax: deploy (target) <-- For furniture\r\n");
    return;
  }

  if (capsule == false && obj) {
    if (obj_get_vnum(g, obj) >= 19080 && obj_get_vnum(g, obj) <= 19099) {
      capsule   = true;
      furniture = true;
    } else {
      send_to_char(g, ch, "That is not a furniture capsule!\r\n");
      return;
    }
  }

  if (capsule == false) {
    send_to_char(g, ch, "You do not have any house type capsules to deploy.@n\r\n");
    return;
  }
  if (char_get_rp(ch) < 10 && furniture == false) {
    send_to_char(g, ch, "You are required to have (not spend) 10 RPP in order to place a house.\r\n");
    return;
  }
  if (furniture == true && (!rroom_flagged(g, char_get_in_room(ch), ROOM_HOUSE) || rroom_flagged(g, char_get_in_room(ch), ROOM_SHIP))) {
    send_to_char(g, ch, "You can't deploy house furniture capsules here.\r\n");
    return;
  }
  if (furniture == true && (rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN1) || rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN2))) {
    send_to_char(g, ch, "You can't deploy house furniture capsules here.\r\n");
    return;
  }
  if (furniture == false && (rroom_sect(g, char_get_in_room(ch)) == SECT_INSIDE || rroom_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM || rroom_sect(g, char_get_in_room(ch)) == SECT_WATER_SWIM || rroom_sect(g, char_get_in_room(ch)) == SECT_SPACE)) {
    send_to_char(g, ch, "You can not deploy that in this kind of area. Try an area more suitable for a house.\r\n");
    return;
  }

  if (furniture == true) {
    obj_vnum fnum = 0;
    if (obj_get_vnum(g, obj) == 19080) {
      fnum = 19090;
    } else if (obj_get_vnum(g, obj) == 19081) {
      fnum = 19091;
    } else if (obj_get_vnum(g, obj) == 19082) {
      fnum = 19092;
    } else if (obj_get_vnum(g, obj) == 19083) {
      fnum = 19093;
    } else if (obj_get_vnum(g, obj) == 19085) {
      fnum = 11;
    }
    if (fnum != 0) {
      struct obj_data *furn = read_object(g, fnum, VIRTUAL);
      act(g, "@CYou click the capsule's button and toss it to the floor. A puff of smoke erupts immediately and quickly dissipates to reveal, $p@C.@n", true, ch, furn, nullptr, TO_CHAR);
      act(g, "@c$n@C clicks a capsule's button and tosses it to the floor. A puff of smoke erupts immediately and quickly dissipates to reveal, $p@C.@n", true, ch, furn, nullptr, TO_ROOM);
      obj_to_room(g, furn, char_get_in_room(ch));
      extract_obj(g, obj);
      return;
    }
    send_to_imm(g, "ERROR: Furniture failed to deploy at %d.", rroom_get_vnum(g, char_get_in_room(ch)));
    return;
  }

  room_vnum rnum = 18800;
  int giveup = false, cont = false, found = false, type = 0;

  if (obj_get_vnum(g, obj) == 4) {
    type = 0;
  } else if (obj_get_vnum(g, obj) == 5) {
    rnum = 18900;
    type = 1;
  } else if (obj_get_vnum(g, obj) == 6) {
    rnum = 19100;
    type = 2;
  }

  room_vnum final = rnum + 99;

  while (giveup == false && cont == false) {
    for (obj_data *obj3 : room_contents(g, real_room(g, rnum))) {
      if (obj_get_vnum(g, obj3) == 18801) {
        found = true;
      }
    }
    if (found == true && rnum < final) {
      if (type == 0) {
        rnum += 4;
      } else {
        rnum += 5;
      }
      found = false;
    } else if (rnum >= final) {
      giveup = true;
    } else {
      cont = true;
    }
  } /* End while */

  if (cont == true) {
    struct obj_data *door = read_object(g, 18801, VIRTUAL);

    obj_set_value(door, 6, rroom_get_vnum(g, char_get_in_room(ch)));
    if (rnum != 18800)
      obj_set_value(door, 0, rnum + 1);
    else
      obj_set_value(door, 0, 18802);
    obj_set_value(door, 2, rnum);
    obj_to_room(g, door, real_room(g, rnum));
    struct obj_data *key = read_object(g, rnum, VIRTUAL);
    obj_to_char(g, key, ch);
    act(g, "@WYou click the capsule and toss it to the ground. A large cloud of smoke erupts from the capsule and after it clears a house is visible in its place!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W clicks a capsule and then tosses it to the ground. A large cloud of smoke erupts from the capsule and after it clears a house is visible in its place!@n", true, ch, nullptr, nullptr, TO_ROOM);
    struct obj_data *foun = read_object(g, 18803, VIRTUAL);
    obj_to_room(g, foun, real_room(g, rnum + 1));
    extract_obj(g, obj);
  } else {
    send_to_char(g, ch, "@ROOC@D: @wSorry for the inconvenience, but it appears there are no houses available. Please contact Iovan.@n\r\n");
    return;
  }
}

ACMD(do_twohand)
{

  if (char_get_grappling(g, ch) || char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are busy grappling with someone!\r\n");
    return;
  }

  if (char_get_absorbing(g, ch) || char_get_absorbby(g, ch)) {
    send_to_char(g, ch, "You are busy struggling with someone!\r\n");
    return;
  }

  if (!char_get_equipment(ch, WEAR_WIELD1) && !char_plr_flagged(ch, PLR_THANDW)) {
    send_to_char(g, ch, "You need to wield a sword to use this.\r\n");
    return;
  }
  if (char_get_equipment(ch, WEAR_WIELD2) && !char_plr_flagged(ch, PLR_THANDW)) {
    send_to_char(g, ch, "You have something in your offhand already and can't two hand wield your main weapon.\r\n");
    return;
  }
  if ((char_get_limb_condition(ch, (1) - 1) <= 0 || char_get_limb_condition(ch, (2) - 1) <= 0) && !char_plr_flagged(ch, PLR_THANDW)) {
    send_to_char(g, ch, "Kind of hard with only one arm...\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_THANDW)) {
    send_to_char(g, ch, "You stop wielding your weapon with both hands.\r\n");
    act(g, "$n stops wielding $s weapon with both hands.", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_THANDW, false);
    return;
  }
  send_to_char(g, ch, "You grab your weapon with both hands.\r\n");
  act(g, "$n starts wielding $s weapon with both hands.", true, ch, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_THANDW, true);
  return;
}

static void start_auction(GameWorld *g, struct char_data *ch, struct obj_data *obj, int bid)
{
  /* Take object from character and set variables */

  obj_from_char(g, obj);
  g->obj_selling = obj;
  g->ch_selling  = ch;
  g->ch_buying   = nullptr;
  g->curbid      = bid;

  /* Tell th character where his item went */
  snprintf(buf, sizeof(buf), "%s magicly flies away from your hands to be auctioned!\r\n", obj_get_short_description(g->obj_selling));
  CAP(buf);
  send_to_char(g, g->ch_selling, "%s", buf);

  /* Anounce the item is being sold */
  snprintf(buf, sizeof(buf), auctioneer[AUC_NULL_STATE], g->curbid);
  auc_send_to_all(g, buf, false);

  g->aucstat = AUC_OFFERING;
}

void check_auction(GameWorld *g)
{
  switch (g->aucstat) {
    case AUC_NULL_STATE:
      return;
    case AUC_OFFERING: {
      if (g->obj_selling == nullptr) {
        auc_send_to_all(g, "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n", false);
        g->curbid     = 0;
        g->ch_selling = nullptr;
        g->ch_buying  = nullptr;
        g->aucstat    = AUC_NULL_STATE;
        return;
      }
      snprintf(buf, sizeof(buf), auctioneer[AUC_OFFERING], g->curbid);
      CAP(buf);
      auc_send_to_all(g, buf, false);
      g->aucstat = AUC_GOING_ONCE;
      return;
    }
    case AUC_GOING_ONCE: {
      if (g->obj_selling == nullptr) {
        auc_send_to_all(g, "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n", false);
        g->curbid     = 0;
        g->ch_selling = nullptr;
        g->ch_buying  = nullptr;
        g->aucstat    = AUC_NULL_STATE;
        return;
      }

      snprintf(buf, sizeof(buf), auctioneer[AUC_GOING_ONCE], g->curbid);
      CAP(buf);
      auc_send_to_all(g, buf, false);
      g->aucstat = AUC_GOING_TWICE;
      return;
    }
    case AUC_GOING_TWICE: {
      if (g->obj_selling == nullptr) {
        auc_send_to_all(g, "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n", false);
        g->curbid     = 0;
        g->ch_selling = nullptr;
        g->ch_buying  = nullptr;
        g->aucstat    = AUC_NULL_STATE;
        return;
      }

      snprintf(buf, sizeof(buf), auctioneer[AUC_GOING_TWICE], g->curbid);
      CAP(buf);
      auc_send_to_all(g, buf, false);
      g->aucstat = AUC_LAST_CALL;
      return;
    }
    case AUC_LAST_CALL: {
      if (g->obj_selling == nullptr) {
        auc_send_to_all(g, "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n", false);
        g->curbid     = 0;
        g->ch_selling = nullptr;
        g->ch_buying  = nullptr;
        g->aucstat    = AUC_NULL_STATE;
        return;
      }
      if (g->ch_buying == nullptr) {

        snprintf(buf, sizeof(buf), auctioneer[AUC_LAST_CALL], g->curbid);

        CAP(buf);
        auc_send_to_all(g, buf, false);

        snprintf(buf, sizeof(buf), "%s flies out the sky and into your hands.\r\n", obj_get_short_description(g->obj_selling));
        CAP(buf);
        send_to_char(g, g->ch_selling, "%s", buf);
        obj_to_char(g, g->obj_selling, g->ch_selling);

        /* Reset auctioning values */
        g->obj_selling = nullptr;
        g->ch_selling  = nullptr;
        g->ch_buying   = nullptr;
        g->curbid      = 0;
        g->aucstat     = AUC_NULL_STATE;
        return;
      }

      snprintf(buf, sizeof(buf), auctioneer[AUC_SOLD], g->curbid);
      auc_send_to_all(g, buf, true);

      /* Give the object to the buyer */
      obj_to_char(g, g->obj_selling, g->ch_buying);
      snprintf(buf, sizeof(buf), "%s flies out the sky and into your hands, what a steal!\r\n", obj_get_short_description(g->obj_selling));
      CAP(buf);
      send_to_char(g, g->ch_buying, "%s", buf);

      snprintf(buf, sizeof(buf), "Congrats! You have sold %s for @Y%d@W zenni!\r\n", obj_get_short_description(g->obj_selling), g->curbid);
      send_to_char(g, g->ch_selling, "%s", buf);

      /* Give selling char the money for his stuff */
      if (char_get_gold(g->ch_selling) + g->curbid > char_gold_carry(g->ch_selling)) {
        send_to_char(g, g->ch_buying, "You couldn't hold all the zenni, so some of it was deposited for you.\r\n");
        int diff = 0;
        diff     = (char_get_gold(g->ch_selling) + g->curbid) - char_gold_carry(g->ch_selling);
        char_set_gold(g->ch_selling, char_gold_carry(g->ch_selling));
        char_set_bank_gold(g->ch_selling, char_get_bank_gold(g->ch_selling) + (diff));
      } else if (char_get_gold(g->ch_selling) + g->curbid <= char_gold_carry(g->ch_selling)) {
        char_set_gold(g->ch_selling, char_get_gold(g->ch_selling) + (g->curbid));
      }
      /* Reset auctioning values */
      g->obj_selling = nullptr;
      g->ch_selling  = nullptr;
      g->ch_buying   = nullptr;
      g->curbid      = 0;
      g->aucstat     = AUC_NULL_STATE;
      return;
    }
  }
}

ACMD(do_auction)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int bid = 0;

  two_arguments(argument, arg1, arg2);

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_HBTC)) {
    send_to_char(g, ch, "This is a different dimension!\r\n");
    return;
  }
  if (rroom_flagged(g, char_get_in_room(ch), ROOM_PAST)) {
    send_to_char(g, ch, "You are in the past!\r\n");
    return;
  }
  if (char_prf_flagged(g, ch, PRF_HIDE)) {
    send_to_char(g, ch, "The auctioneer will not accept items from hidden people.\r\n");
    return;
  }

  if (!*arg1) {
    send_to_char(g, ch, "Auction what?\r\n");
    send_to_char(g, ch, "[ Auction: <item> | <cancel> ]\r\n");
    return;
  }
  if (is_abbrev(arg1, "cancel") || is_abbrev(arg1, "stop")) {
    if ((ch != g->ch_selling && char_get_admlevel(ch) <= ADMLVL_GRGOD) || g->aucstat == AUC_NULL_STATE) {
      send_to_char(g, ch, "You're not even selling anything!\r\n");
      return;
    }
    if (ch == g->ch_selling) {
      stop_auction(g, AUC_NORMAL_CANCEL, nullptr);
      return;
    }
    stop_auction(g, AUC_WIZ_CANCEL, ch);
  } else if (is_abbrev(arg1, "stats") || is_abbrev(arg1, "identify")) {
    auc_stat(g, ch, g->obj_selling);
    return;
  } else if (!(obj = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch)))) {
    snprintf(buf, sizeof(buf), "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
    send_to_char(g, ch, "%s", buf);
    return;
  } else if (!*arg2) {
    snprintf(buf, sizeof(buf), "What should be the minimum bid?\r\n");
    send_to_char(g, ch, "%s", buf);
    return;
  } else if (*arg2 && (bid = atoi(arg2)) <= 0) {
    send_to_char(g, ch, "Come on? One zenni at least?\r\n");
    return;
  } else if (g->aucstat != AUC_NULL_STATE) {
    snprintf(buf, sizeof(buf), "Sorry, but %s is already auctioning %s at @Y%d@W zenni!\r\n", char_get_name(g->ch_selling),
             obj_get_short_description(g->obj_selling), bid);
    send_to_char(g, ch, "%s", buf);
    return;
  } else if (obj_extra_flagged(obj, ITEM_NOSELL)) {
    send_to_char(g, ch, "Sorry but you can't sell that!\r\n");
    return;
  } else if (obj_get_value(obj, VAL_CONTAINER_CORPSE) == 1) {
    send_to_char(g, ch, "Sorry but you can't sell that!\r\n");
    return;
  } else {
    send_to_char(g, ch, "Ok.\r\n");
    start_auction(g, ch, obj, bid);
    return;
  }
}

ACMD(do_bid)
{
  struct obj_data *obj2 = nullptr;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int found = false, list = 0, masterList = 0;
  room_vnum auct_room;

  auct_room = real_room(g, 80);

  if (char_is_npc(ch))
    return;

  if (!char_get_equipment(ch, WEAR_EYE)) {
    send_to_char(g, ch, "You need a scouter to make an auction bid.\r\n");
    return;
  }

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_HBTC)) {
    send_to_char(g, ch, "This is a different dimension!\r\n");
    return;
  }
  if (rroom_flagged(g, char_get_in_room(ch), ROOM_PAST)) {
    send_to_char(g, ch, "This is the past, nothing is being auctioned!\r\n");
    return;
  }

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(g, ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
    return;
  }
  for (obj_data *obj : room_contents(g, auct_room)) {
    if (obj) {
      list++;
    }
  }
  masterList = list;
  list       = 0;

  if (!strcasecmp(arg, "list")) {
    send_to_char(g, ch, "@Y                                   Auction@n\r\n");
    send_to_char(g, ch, "@c------------------------------------------------------------------------------@n\r\n");
    for (obj_data *obj : room_contents(g, auct_room)) {
      if (obj) {
        if (obj_get_aucter(obj) <= 0) {
          continue;
        }
        list++;
        if (obj_get_aucTime(obj) + 86400 > game_now(g) && obj_get_curBidder(obj) <= -1) {
          send_to_char(g, ch, "@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: @w%-*s@D][@GCost@W: @Y%s@D]@n\r\n", list, get_name_by_id(g, obj_get_aucter(obj)) != nullptr ? CAP(get_name_by_id(g, obj_get_aucter(obj))) : "Nobody", count_color_chars(obj_get_short_description(obj)) + 30, obj_get_short_description(obj), add_commas(obj_get_bid(obj)));
        } else if (obj_get_aucTime(obj) + 86400 > game_now(g) && obj_get_curBidder(obj) > -1) {
          send_to_char(g, ch, "@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: @w%-*s@D][@RTop Bid@W: %s @Y%s@D]@n\r\n", list, get_name_by_id(g, obj_get_aucter(obj)) != nullptr ? CAP(get_name_by_id(g, obj_get_aucter(obj))) : "Nobody", count_color_chars(obj_get_short_description(obj)) + 30, obj_get_short_description(obj), get_name_by_id(g, obj_get_curBidder(obj)) != nullptr ? CAP(get_name_by_id(g, obj_get_curBidder(obj))) : "Nobody", add_commas(obj_get_bid(obj)));
        } else if (obj_get_aucTime(obj) + 86400 < game_now(g) && obj_get_curBidder(obj) > -1) {
          send_to_char(g, ch, "@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: @w%-*s@D][@RBid Winner@W: %s @Y%s@D]@n\r\n", list, get_name_by_id(g, obj_get_aucter(obj)) != nullptr ? CAP(get_name_by_id(g, obj_get_aucter(obj))) : "Nobody", count_color_chars(obj_get_short_description(obj)) + 30, obj_get_short_description(obj), get_name_by_id(g, obj_get_curBidder(obj)) != nullptr ? CAP(get_name_by_id(g, obj_get_curBidder(obj))) : "Nobody", add_commas(obj_get_bid(obj)));
        } else {
          send_to_char(g, ch, "@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: @w%-*s@D][@RClosed@D]@n\r\n", list, get_name_by_id(g, obj_get_aucter(obj)) != nullptr ? CAP(get_name_by_id(g, obj_get_aucter(obj))) : "Nobody", count_color_chars(obj_get_short_description(obj)) + 30, obj_get_short_description(obj));
        }
        found = true;
      }
    }
    if (found == false) {
      send_to_char(g, ch, "No items are currently being auctioned.\r\n");
    }
    send_to_char(g, ch, "@c------------------------------------------------------------------------------@n\r\n");
  } else if (!strcasecmp(arg, "appraise")) {
    if (!*arg2) {
      send_to_char(g, ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
      send_to_char(g, ch, "What item number did you want to appraise?\r\n");
      return;
    }
    if (atoi(arg2) < 0 || atoi(arg2) > masterList) {
      send_to_char(g, ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
      send_to_char(g, ch, "That item number doesn't exist.\r\n");
      return;
    }

    for (obj_data *obj : room_contents(g, auct_room)) {
      if (obj) {
        if (obj_get_aucter(obj) <= 0) {
          continue;
        }
        list++;
        if (atoi(arg2) == list) {
          obj2 = obj;
        }
      }
    }
    if (!obj2) {
      send_to_char(g, ch, "That item number is not found.\r\n");
      return;
    }
    if (!char_get_skill(ch, SKILL_APPRAISE)) {
      send_to_char(g, ch, "You are unskilled at appraising.\r\n");
      return;
    }
    improve_skill(g, ch, SKILL_APPRAISE, 1);
    if (char_get_skill(ch, SKILL_APPRAISE) < rand_number(g, 1, 101)) {
      send_to_char(g, ch, "You look at the images for %s and fail to perceive its worth..\r\n", obj_get_short_description(obj2));
      act(g, "@c$n@w looks stumped about something they viewed on their scouter screen.@n", true, ch, nullptr, nullptr, TO_ROOM);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
    send_to_char(g, ch, "You look at images of the object on your scouter.\r\n");
    act(g, "@c$n@w looks at something on their scouter screen.@n", true, ch, nullptr, nullptr, TO_ROOM);
    send_to_char(g, ch, "@c------------------------------------------------------------------------\n");
    send_to_char(g, ch, "@GOwner       @W: @w%s@n\n", get_name_by_id(g, obj_get_aucter(obj2)) != nullptr ? CAP(get_name_by_id(g, obj_get_aucter(obj2))) : "Nobody");
    send_to_char(g, ch, "@GItem Name   @W: @w%s@n\n", obj_get_short_description(obj2));
    send_to_char(g, ch, "@GCurrent Bid @W: @Y%s@n\n", add_commas(obj_get_bid(obj2)));
    send_to_char(g, ch, "@GStore Value @W: @Y%s@n\n", add_commas(obj_get_cost(obj2)));
    send_to_char(g, ch, "@GItem Min LVL@W: @w%d@n\n", obj_get_level(obj2));
    if (obj_get_value(obj2, VAL_ALL_HEALTH) >= 100) {
      send_to_char(g, ch, "@GCondition   @W: @C%d%s@n\n", obj_get_value(obj2, VAL_ALL_HEALTH), "%");
    } else if (obj_get_value(obj2, VAL_ALL_HEALTH) >= 50) {
      send_to_char(g, ch, "@GCondition   @W: @y%d%s@n\n", obj_get_value(obj2, VAL_ALL_HEALTH), "%");
    } else if (obj_get_value(obj2, VAL_ALL_HEALTH) >= 1) {
      send_to_char(g, ch, "@GCondition   @W: @r%d%s@n\n", obj_get_value(obj2, VAL_ALL_HEALTH), "%");
    } else {
      send_to_char(g, ch, "@GCondition   @W: @D%d%s@n\n", obj_get_value(obj2, VAL_ALL_HEALTH), "%");
    }
    send_to_char(g, ch, "@GItem Weight @W: @w%s@n\n", add_commas(obj_get_weight(obj2)));
    char bits[MAX_STRING_LENGTH];
    sprintbitarray(obj_get_wear_flags(obj2), wear_bits, TW_ARRAY_MAX, bits, sizeof(bits));
    search_replace(bits, "TAKE", "");
    send_to_char(g, ch, "@GWear Loc.   @W:@w%s\n", bits);
    if (obj_get_type_flag(obj2) == ITEM_WEAPON) {
      if (obj_extra_flagged(obj2, ITEM_WEAPLVL1)) {
        send_to_char(g, ch, "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w5%s@D]@n\r\n", "%");
      } else if (obj_extra_flagged(obj2, ITEM_WEAPLVL2)) {
        send_to_char(g, ch, "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w10%s@D]@n\r\n", "%");
      } else if (obj_extra_flagged(obj2, ITEM_WEAPLVL3)) {
        send_to_char(g, ch, "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w20%s@D]@n\r\n", "%");
      } else if (obj_extra_flagged(obj2, ITEM_WEAPLVL4)) {
        send_to_char(g, ch, "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w30%s@D]@n\r\n", "%");
      } else if (obj_extra_flagged(obj2, ITEM_WEAPLVL5)) {
        send_to_char(g, ch, "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w50%s@D]@n\r\n", "%");
      }
    }
    int i, found_bonus = false;
    send_to_char(g, ch, "@GItem Size   @W:@w %s@n\r\n", size_names[obj_get_size(obj2)]);
    send_to_char(g, ch, "@GItem Bonuses@W:@w");
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if (obj_get_affected(obj2)[i].modifier) {
        sprinttype(obj_get_affected(obj2)[i].location, apply_types, buf, sizeof(buf));
        send_to_char(g, ch, "%s %+d to %s", found_bonus++ ? "," : "", obj_get_affected(obj2)[i].modifier, buf);
        switch (obj_get_affected(obj2)[i].location) {
          case APPLY_FEAT:
            send_to_char(g, ch, " (%s)", g->feat_list[obj_get_affected(obj2)[i].specific].name);
            break;
          case APPLY_SKILL:
            send_to_char(g, ch, " (%s)", g->spell_info[obj_get_affected(obj2)[i].specific].name);
            break;
        }
      }
    }
    if (!found_bonus)
      send_to_char(g, ch, " None@n");
    else
      send_to_char(g, ch, "@n");
    char buf2[MAX_STRING_LENGTH];
    sprintbitarray(obj_get_bitvector(obj2), affected_bits, AF_ARRAY_MAX, buf2, sizeof(buf2));
    send_to_char(g, ch, "\n@GSpecial     @W:@w %s\n", buf2);
    send_to_char(g, ch, "@c------------------------------------------------------------------------\n");
    return;
  } else {
    if (!*arg2) {
      send_to_char(g, ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
      send_to_char(g, ch, "What amount did you want to bid?\r\n");
      return;
    }
    if (atoi(arg) < 0 || atoi(arg) > masterList) {
      send_to_char(g, ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
      send_to_char(g, ch, "That item number is not found.\r\n");
      return;
    }

    for (obj_data *obj : room_contents(g, auct_room)) {
      if (obj) {
        if (obj_get_aucter(obj) <= 0) {
          continue;
        }
        list++;
        if (atoi(arg) == list) {
          obj2 = obj;
        }
      }
    }

    if (!obj2) {
      send_to_char(g, ch, "That item number is not found.\r\n");
      return;
    }
    if (obj_get_curBidder(obj2) == char_get_id(ch)) {
      send_to_char(g, ch, "You already have the highest bid.\r\n");
      return;
    }
    if (obj_get_aucter(obj2) == char_get_id(ch)) {
      send_to_char(g, ch, "You auctioned the item, go to the auction house and cancel if you can.\r\n");
      return;
    }
    if (obj_get_curBidder(obj2) > 0 && atoi(arg2) <= (obj_get_bid(obj2) + (obj_get_bid(obj2) * .1)) && obj_get_curBidder(obj2) > -1) {
      send_to_char(g, ch, "You have to bid at least 10 percent over the current bid.\r\n");
      return;
    }
    if (atoi(arg2) < obj_get_bid(obj2) && obj_get_curBidder(obj2) <= -1) {
      send_to_char(g, ch, "You have to bid at least the starting bid.\r\n");
      return;
    }
    if (atoi(arg2) > (((char_get_gold(ch) + char_get_bank_gold(ch)) / 100) * 50) + (char_get_gold(ch) + char_get_bank_gold(ch))) {
      send_to_char(g, ch, "You can not bid more than 150%s of your total money (on hand and in the bank).\r\n", "%");
      return;
    }
    if (obj_get_aucTime(obj2) + 86400 <= game_now(g)) {
      send_to_char(g, ch, "Bidding on that object has been closed.\r\n");
      return;
    }
    obj_set_bid(obj2, atoi(arg2));
    obj_set_curBidder(obj2, char_get_id(ch));
    auc_save(g);
    int bid = atoi(arg2);
    mud_log("AUCTION: %s has bid %s on %s", char_get_name(ch), obj_get_short_description(obj2), add_commas(bid));
    for (descriptor_data *d : each_desc(g)) {
      if (desc_get_connected(d) != CON_PLAYING || char_is_npc(d->character))
        continue;
      if (d->character == ch) {
        if (char_get_equipment(d->character, WEAR_EYE)) {
          send_to_char(g, d->character, "@RScouter Auction News@D: @GYou have bid @Y%s@G on @w%s@G@n\r\n", add_commas(obj_get_bid(obj2)), obj_get_short_description(obj2));
        }
        continue;
      }
      if (char_get_equipment(d->character, WEAR_EYE)) {
        send_to_char(g, d->character, "@RScouter Auction News@D: @GThe bid on, @w%s@G, has been raised to @Y%s@n\r\n", obj_get_short_description(obj2), add_commas(obj_get_bid(obj2)));
      }
    }
  }
}

void stop_auction(GameWorld *g, int type, struct char_data *ch)
{
  if (g->obj_selling == nullptr) {
    auc_send_to_all(g, "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n", false);
    g->curbid     = 0;
    g->ch_selling = nullptr;
    g->ch_buying  = nullptr;
    g->aucstat    = AUC_NULL_STATE;
    return;
  }
  switch (type) {

    case AUC_NORMAL_CANCEL: {

      snprintf(buf, sizeof(buf), "%s", auctioneer[AUC_NORMAL_CANCEL]);
      auc_send_to_all(g, buf, false);
      break;
    }
    case AUC_QUIT_CANCEL: {

      snprintf(buf, sizeof(buf), "%s", auctioneer[AUC_QUIT_CANCEL]);
      auc_send_to_all(g, buf, false);
      break;
    }
    case AUC_WIZ_CANCEL: {

      snprintf(buf, sizeof(buf), "%s", auctioneer[AUC_WIZ_CANCEL]);
      auc_send_to_all(g, buf, false);
      break;
    }
    default: {
      send_to_char(g, ch, "Sorry, that is an unrecognised cancel command, please report.");
      return;
    }
  }

  if (type != AUC_WIZ_CANCEL) {
    snprintf(buf, sizeof(buf), "%s flies out the sky and into your hands.\r\n", obj_get_short_description(g->obj_selling));
    CAP(buf);
    send_to_char(g, g->ch_selling, "%s", buf);
    obj_to_char(g, g->obj_selling, g->ch_selling);
  } else {
    snprintf(buf, sizeof(buf), "%s flies out the sky and into your hands.\r\n", obj_get_short_description(g->obj_selling));
    CAP(buf);
    send_to_char(g, ch, "%s", buf);
    obj_to_char(g, g->obj_selling, ch);
  }

  if (!(g->ch_buying == nullptr))
    char_set_gold(g->ch_buying, char_get_gold(g->ch_buying) + (g->curbid));

  g->obj_selling = nullptr;
  g->ch_selling  = nullptr;
  g->ch_buying   = nullptr;
  g->curbid      = 0;

  g->aucstat = AUC_NULL_STATE;
}

static void auc_stat(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{

  if (g->aucstat == AUC_NULL_STATE) {
    send_to_char(g, ch, "Nothing is being auctioned!\r\n");
    return;
  }
  if (ch == g->ch_selling) {
    send_to_char(g, ch, "You should have found that out BEFORE auctioning it!\r\n");
    return;
  }
  if (char_get_gold(ch) < 500) {
    send_to_char(g, ch, "You can't afford to find the stats on that, it costs 500 zenni!\r\n");
    return;
  }
  /* auctioneer tells the character the auction details */
  snprintf(buf, sizeof(buf), auctioneer[AUC_STAT], g->curbid);
  act(g, buf, true, g->ch_selling, obj, ch, TO_VICT | TO_SLEEP);
  char_set_gold(ch, char_get_gold(ch) - (500));

  /*call_magic(ch, NULL, obj_selling, SPELL_IDENTIFY, 30, CAST_SPELL);*/
}

static void auc_send_to_all(GameWorld *g, const char *messg, bool buyer)
{

  if (messg == nullptr)
    return;

  for (descriptor_data *i : each_desc(g)) {
    if (desc_get_connected(i) != CON_PLAYING)
      continue;
    if (rroom_flagged(g, char_get_in_room(i->character), ROOM_HBTC))
      continue;
    if (rroom_flagged(g, char_get_in_room(i->character), ROOM_PAST))
      continue;
    if (buyer)
      act(g, messg, true, g->ch_buying, g->obj_selling, i->character, TO_VICT | TO_SLEEP);
    else
      act(g, messg, true, g->ch_selling, g->obj_selling, i->character, TO_VICT | TO_SLEEP);
  }
}

ACMD(do_assemble)
{
  obj_vnum lVnum           = NOTHING;
  struct obj_data *pObject = nullptr;
  char text[MAX_STRING_LENGTH];

  int roll = 0;

  skip_spaces(&argument);

  struct obj_data *tool = nullptr;

  for (obj_data *tools : char_inventory(g, ch)) {
    if (obj_get_vnum(g, tools) == 386 && obj_get_value(tools, VAL_ALL_HEALTH) > 0) {
      tool = tools;
      act(g, "@WYou open up your toolkit and take out the necessary tools.@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W opens up $s toolkit and takes out the necessary tools.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  }

  if (*argument == '\0') {
    send_to_char(g, ch, "What would you like to %s?\r\n", CMD_NAME);
    return;
  }
  if ((lVnum = assemblyFindAssembly(g, argument)) < 0) {
    send_to_char(g, ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
    return;
  }
  if (assemblyGetType(g, lVnum) != subcmd) {
    send_to_char(g, ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
    return;
  }
  if (!assemblyCheckComponents(g, lVnum, ch, false)) {
    send_to_char(g, ch, "You haven't got all the things you need.\r\n");
    return;
  }
  if (rroom_flagged(g, char_get_in_room(ch), ROOM_SPACE)) {
    send_to_char(g, ch, "You can't do that in space.");
    return;
  }
  if (!char_get_skill(ch, SKILL_SURVIVAL) && !strcasecmp(argument, "campfire")) {
    send_to_char(g, ch, "You know nothing about building campfires.\r\n");
    return;
  }

  if (strstr(argument, "Signal") || strstr(argument, "signal")) {
    if (char_get_skill(ch, SKILL_BUILD) < 70) {
      send_to_char(g, ch, "You need at least a build skill level of 70.\r\n");
      return;
    }
  }

  if (tool == nullptr) {
    send_to_char(g, ch, "You wish you had tools, but make the best out of what you do have anyway...\r\n");
    roll = 20;
  }

  if (strcasecmp(argument, "campfire")) {
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_SPACE) || rroom_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM || rroom_sunken(g, char_get_in_room(ch))) {
      send_to_char(g, ch, "This area will not allow a fire to burn properly.\r\n");
      return;
    }

    if (char_get_skill(ch, SKILL_SURVIVAL) >= 90) {
      roll += axion_dice(g, 0);
    } else if (char_get_skill(ch, SKILL_SURVIVAL) < 90) {
      roll += axion_dice(g, 0);
    }
    improve_skill(g, ch, SKILL_BUILD, 1);
    if (char_get_skill(ch, SKILL_BUILD) <= roll) {
      if ((pObject = read_object(g, lVnum, VIRTUAL)) == nullptr) {
        send_to_char(g, ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
        return;
      }

      extract_obj(g, pObject);
      send_to_char(g, ch, "You start to %s %s %s, but mess up royally!\r\n", CMD_NAME, AN(argument), argument);

      if (assemblyCheckComponents(g, lVnum, ch, true)) {
        roll = 9001; /* Just a place holder */
      }
      return;
    }
  } else {
    if (char_get_skill(ch, SKILL_SURVIVAL) >= 90) {
      roll += axion_dice(g, 0);
    } else if (char_get_skill(ch, SKILL_SURVIVAL) < 90) {
      roll += axion_dice(g, -10);
    }
    improve_skill(g, ch, SKILL_BUILD, 1);
    if (char_get_skill(ch, SKILL_SURVIVAL) <= roll) {
      if ((pObject = read_object(g, lVnum, VIRTUAL)) == nullptr) {
        send_to_char(g, ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
        return;
      }

      extract_obj(g, pObject);
      send_to_char(g, ch, "You start to %s %s %s, but mess up royally!\r\n", CMD_NAME, AN(argument), argument);

      if (tool && rand_number(g, 1, 3) == 3 && obj_get_value(tool, VAL_ALL_HEALTH) > 0) {
        obj_set_value(tool, VAL_ALL_HEALTH, obj_get_value(tool, VAL_ALL_HEALTH) - (rand_number(g, 1, 5)));
        act(g, "@RYour toolset is looking a bit more worn.@n", true, ch, nullptr, nullptr, TO_CHAR);
        if (obj_get_value(tool, VAL_ALL_HEALTH) <= 0) {
          obj_set_value(tool, VAL_ALL_HEALTH, 0);
        }
      }
      if (assemblyCheckComponents(g, lVnum, ch, true)) {
        roll = 9001; /* Just a place holder */
      }
      return;
    }
  }

  if (axion_dice(g, 0) - (char_get_aff_abils_intel(ch) / 5) > 95) {
    if ((pObject = read_object(g, lVnum, VIRTUAL)) == nullptr) {
      send_to_char(g, ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
      return;
    }

    extract_obj(g, pObject);
    send_to_char(g, ch, "You start to %s %s %s, but forget a couple of steps. You take it apart and give up.\r\n", CMD_NAME, AN(argument), argument);
    char_set_wait(ch, PULSE_6SEC);
    return;
  }
  if (rand_number(g, 1, 100) >= 92) {
    if ((pObject = read_object(g, lVnum, VIRTUAL)) == nullptr) {
      send_to_char(g, ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
      return;
    }

    extract_obj(g, pObject);
    send_to_char(g, ch, "You start to %s %s %s, but put it together wrong and have to stop. You take it apart and give up.\r\n", CMD_NAME, AN(argument), argument);
    char_set_wait(ch, PULSE_4SEC);
    return;
  }

  /* Create the assembled object. */
  if ((pObject = read_object(g, lVnum, VIRTUAL)) == nullptr) {
    send_to_char(g, ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
    return;
  }
  add_unique_id(g, pObject);

  /* Now give the object to the character. */
  if (obj_get_vnum(g, pObject) != 1611) {
    obj_to_char(g, pObject, ch);
    if (char_is_truffle(ch)) {
      int count = 0, plused = false, hasstat = 0, failsafe = 0;

      while (count < 6) {
        if (obj_get_affected(pObject)[count].location > 0 && rand_number(g, 1, 6) <= 2 && plused == false) {
          obj_get_affected(pObject)[count].modifier += rand_number(g, 1, 3);
          plused = true;
        } else if (obj_get_affected(pObject)[count].location > 0) {
          hasstat += 1;
        }
        failsafe++;
        count++;
        if (plused == true) {
          send_to_char(g, ch, "@YYour intuitive skill with building has made this item even better!@n\r\n");
          count = 6;
        } else if (failsafe >= 12) {
          send_to_char(g, ch, "@yIt seems this item could not be upgraded with your truffle knowledge...@n\r\n");
          count = 6;
        } else if (failsafe == 11 && plused == false) {
          send_to_char(g, ch, "@YYour intuitive skill with building has made this item even better!@n\r\n");
          obj_get_affected(pObject)[count].location = rand_number(g, 1, 6);
          obj_get_affected(pObject)[count].modifier += rand_number(g, 1, 3);
          plused = true;
          count  = 6;
        } else if (count == 6 && hasstat > 0) {
          count = 0;
        }
      }
    }
  } else {
    obj_to_room(g, pObject, char_get_in_room(ch));
    obj_set_timer(pObject, (char_get_skill(ch, SKILL_SURVIVAL) * 0.12));
  }

  /*if (wearable_obj(pObject)) {
   GET_OBJ_SIZE(pObject) = get_size(ch);
  }*/

  /* Tell the character they made something. */
  snprintf(text, sizeof(text), "You %s $p.", CMD_NAME);
  act(g, text, false, ch, pObject, nullptr, TO_CHAR);

  /* Tell the room the character made something. */
  snprintf(text, sizeof(text), "$n %ss $p.", CMD_NAME);
  act(g, text, false, ch, pObject, nullptr, TO_ROOM);

  if (assemblyCheckComponents(g, lVnum, ch, true)) {
    roll = 9001; /* Just a place holder */
  }

  if (!char_is_truffle(ch) && axion_dice(g, 8) > char_get_skill(ch, SKILL_BUILD)) {
    send_to_char(g, ch, "@yYou've made an inferior product. Its value will be somewhat less.@n\r\n");
    obj_set_cost(pObject, obj_get_cost(pObject) - (obj_get_cost(pObject) * 0.25));
  } else if (char_is_truffle(ch) && axion_dice(g, 18) > char_get_skill(ch, SKILL_BUILD)) {
    send_to_char(g, ch, "@yYou've made an inferior product. Its value will be somewhat less.@n\r\n");
    obj_set_cost(pObject, obj_get_cost(pObject) - (obj_get_cost(pObject) * 0.12));
  } else if (char_is_truffle(ch) < char_get_skill(ch, SKILL_BUILD)) {
    send_to_char(g, ch, "@YYou've made an excellent product. Its value will be somewhat more.@n\r\n");
    obj_set_cost(pObject, obj_get_cost(pObject) + (obj_get_cost(pObject) * 0.12));
  }

  if (char_is_truffle(ch) && rand_number(g, 1, 5) >= 4 && obj_get_cost(pObject) >= 500) {
    if (char_get_level(ch) < 100 && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0) {
      int64_t gain = level_exp(ch, char_get_level(ch) + 1) * 0.011;
      send_to_char(g, ch, "@RExp Bonus@D: @G%s@n\r\n", add_commas(gain));
      gain_exp(g, ch, gain);
    } else {
      gain_exp(g, ch, 1375000);
      send_to_char(g, ch, "@RExp Bonus@D: @G%s@n\r\n", add_commas(1375000));
    }
  }
}

static void perform_put(GameWorld *g, struct char_data *ch, struct obj_data *obj,
                        struct obj_data *cont)
{

  int dball[7] = {20, 21, 22, 23, 24, 25, 26};

  if (!drop_otrigger(g, obj, ch))
    return;

  if (!obj) /* object might be extracted by drop_otrigger */
    return;
  if (obj_extra_flagged(cont, ITEM_FORGED)) {
    act(g, "$P is forged and won't hold anything.", false, ch, nullptr, cont, TO_CHAR);
    return;
  }
  if (obj_extra_flagged(cont, ITEM_SHEATH) && obj_get_type_flag(obj) != ITEM_WEAPON) {
    send_to_char(g, ch, "That is made to only hold weapons.\r\n");
    return;
  }
  if (obj_extra_flagged(cont, ITEM_SHEATH)) {
    int count = 0, minus = 0;
    for (obj_data *item : obj_contents(g, cont)) {
      minus += obj_get_weight(item);
      count++;
    }
    int holds = obj_get_weight(cont) - minus;
    if (count >= holds) {
      send_to_char(g, ch, "It can only hold %d weapon%s at a time.\r\n", holds, holds > 1 ? "s" : "");
      return;
    }
  }
  if ((obj_get_type_flag(cont) == ITEM_CONTAINER) && (obj_get_value(cont, VAL_CONTAINER_CAPACITY) == 0))
    act(g, "$p won't fit in $P.", false, ch, obj, cont, TO_CHAR);
  else if (obj_get_vnum(g, cont) >= 600 && obj_get_vnum(g, cont) <= 603)
    send_to_char(g, ch, "You can't put cards on a duel table. You have to @Gplay@n them.\r\n");
  else if ((obj_get_vnum(g, cont) == 697 || obj_get_vnum(g, cont) == 698 || obj_get_vnum(g, cont) == 682 || obj_get_vnum(g, cont) == 683 || obj_get_vnum(g, cont) == 684 || obj_extra_flagged(cont, ITEM_CARDCASE)) && !obj_extra_flagged(obj, ITEM_ANTI_HIEROPHANT))
    send_to_char(g, ch, "You can only put cards in a case.\r\n");
  else if ((obj_get_type_flag(cont) == ITEM_CONTAINER) &&
           (obj_get_value(cont, VAL_CONTAINER_CAPACITY) > 0) &&
           (obj_get_weight(cont) + obj_get_weight(obj) > obj_get_value(cont, VAL_CONTAINER_CAPACITY)))
    act(g, "$p won't fit in $P.", false, ch, obj, cont, TO_CHAR);
  else if (obj_extra_flagged(obj, ITEM_NODROP) && obj_get_in_room(cont) != NOWHERE)
    act(g, "You can't get $p out of your hand.", false, ch, obj, nullptr, TO_CHAR);
  else if (obj_get_vnum(g, obj) == dball[0] || obj_get_vnum(g, obj) == dball[1] || obj_get_vnum(g, obj) == dball[2] || obj_get_vnum(g, obj) == dball[3] || obj_get_vnum(g, obj) == dball[4] || obj_get_vnum(g, obj) == dball[5] || obj_get_vnum(g, obj) == dball[6])
    send_to_char(g, ch, "You can not bag dragon balls.\r\n");
  else if (obj_extra_flagged(obj, ITEM_NORENT))
    send_to_char(g, ch, "That isn't worth bagging. Better keep that close if you wanna keep it at all.\r\n");
  else if (!obj_get_carried_by(cont) && check_saveroom_count(g, ch, obj) > 150) {
    send_to_char(g, ch, "The save room can not hold anymore items. (150 max, count of items in containers is halved)\r\n");
  } else {
    obj_from_char(g, obj);
    obj_to_obj(g, obj, cont);

    if (!obj_extra_flagged(obj, ITEM_ANTI_HIEROPHANT)) {
      act(g, "$n puts $p in $P.", true, ch, obj, cont, TO_ROOM);
    } else {
      act(g, "$n puts an @DA@wd@cv@Ce@Wnt @DD@wu@ce@Cl @mC@Ma@Wr@wd@n in $P.", true, ch, obj, cont, TO_ROOM);
    }

    /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
    if (obj_extra_flagged(obj, ITEM_NODROP) && !obj_extra_flagged(cont, ITEM_NODROP)) {
      obj_extra_set(cont, ITEM_NODROP, true);
      act(g, "You get a strange feeling as you put $p in $P.", false,
          ch, obj, cont, TO_CHAR);
    } else
      act(g, "You put $p in $P.", false, ch, obj, cont, TO_CHAR);
    /* If object placed in portal or vehicle, move it to the portal destination */
    if ((obj_get_type_flag(cont) == ITEM_PORTAL) ||
        (obj_get_type_flag(cont) == ITEM_VEHICLE)) {
      obj_from_obj(g, obj);
      obj_to_room(g, obj, real_room(g, obj_get_value(cont, VAL_CONTAINER_CAPACITY)));
      if (obj_get_type_flag(cont) == ITEM_PORTAL) {
        act(g, "What? $U$p disappears from $P in a puff of smoke!",
            true, ch, obj, cont, TO_ROOM);
        act(g, "What? $U$p disappears from $P in a puff of smoke!",
            false, ch, obj, cont, TO_CHAR);
      }
    }
  }
}

/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  struct obj_data *obj, *cont;
  struct char_data *tmp_char;
  int obj_dotmode, cont_dotmode, found = 0, howmany = 1;
  char *theobj, *thecont;

  one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments */

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }

  if (*arg3 && is_number(arg1)) {
    howmany = atoi(arg1);
    theobj  = arg2;
    thecont = arg3;
  } else {
    theobj  = arg1;
    thecont = arg2;
  }
  obj_dotmode  = find_all_dots(theobj);
  cont_dotmode = find_all_dots(thecont);

  if (!*theobj)
    send_to_char(g, ch, "Put what in what?\r\n");
  else if (cont_dotmode != FIND_INDIV)
    send_to_char(g, ch, "You can only put things into one container at a time.\r\n");
  else if (!*thecont) {
    send_to_char(g, ch, "What do you want to put %s in?\r\n", obj_dotmode == FIND_INDIV ? "it" : "them");
  } else {
    generic_find(g, thecont, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
    if (!cont)
      send_to_char(g, ch, "You don't see %s %s here.\r\n", AN(thecont), thecont);
    else if ((obj_get_type_flag(cont) != ITEM_CONTAINER) &&
             (obj_get_type_flag(cont) != ITEM_PORTAL) &&
             (obj_get_type_flag(cont) != ITEM_VEHICLE))
      act(g, "$p is not a container.", false, ch, cont, nullptr, TO_CHAR);
    else if (obj_val_flagged(cont, CONT_CLOSED))
      send_to_char(g, ch, "You'd better open it first!\r\n");
    else {
      if (obj_dotmode == FIND_INDIV) { /* put <obj> <container> */
        if (!(obj = get_obj_in_list_vis(g, ch, theobj, nullptr, char_get_inventory(ch))))
          send_to_char(g, ch, "You aren't carrying %s %s.\r\n", AN(theobj), theobj);
        else if (obj == cont && howmany == 1)
          send_to_char(g, ch, "You attempt to fold it into itself, but fail.\r\n");
        else {
          for (obj_data *match : obj_list(g, obj)) {
            if (!howmany)
              break;
            if (!obj_named_and_visible(g, ch, theobj, match))
              continue;
            if (match != cont) {
              howmany--;
              perform_put(g, ch, match, cont);
            }
          }
        }
      } else {
        for (obj_data *item : char_inventory(g, ch)) {
          if (item != cont && char_can_see_obj(g, ch, item) &&
              (obj_dotmode == FIND_ALL || isname(theobj, obj_get_name(item)))) {
            found = 1;
            perform_put(g, ch, item, cont);
          }
        }
        if (!found) {
          if (obj_dotmode == FIND_ALL)
            send_to_char(g, ch, "You don't seem to have anything to put in it.\r\n");
          else
            send_to_char(g, ch, "You don't seem to have any %ss.\r\n", theobj);
        }
      }
    }
  }
}

static bool can_take_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{
  if (!(obj_wear_flagged(obj, ITEM_WEAR_TAKE))) {
    act(g, "$p: you can't take that!", false, ch, obj, nullptr, TO_CHAR);
    return false;
  }
  if (char_get_carry_items(ch) >= char_can_carry_n(ch)) {
    act(g, "$p: your arms are full!", false, ch, obj, nullptr, TO_CHAR);
    return false;
  }
  if (((getCurCarriedWeight(ch)) + obj_get_weight(obj)) > getMaxCarryWeight(g, ch)) {
    act(g, "$p: you can't carry that much weight.", false, ch, obj, nullptr, TO_CHAR);
    return false;
  }
  if ((obj_get_weight(obj) + rroom_get_gravity(g, char_get_in_room(ch))) + (getCurCarriedWeight(ch)) > getMaxCarryWeight(g, ch)) {
    act(g, "$p: you can't carry that much weight because of the gravity.", false, ch, obj, nullptr, TO_CHAR);
    return false;
  }
  return true;
}

static void get_check_money(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{
  int value = obj_get_value(obj, VAL_MONEY_SIZE);

  if (obj_get_type_flag(obj) != ITEM_MONEY || value <= 0)
    return;

  if (char_get_gold(ch) + value > char_gold_carry(ch)) {
    send_to_char(g, ch, "You can only carry %s zenni at your current level, and leave the rest.\r\n", add_commas(char_gold_carry(ch)));
    act(g, "@w$n @wdrops some onto the ground.@n", false, ch, nullptr, nullptr, TO_ROOM);
    extract_obj(g, obj);
    int diff = 0;
    diff     = (char_get_gold(ch) + value) - char_gold_carry(ch);
    obj      = create_money(g, diff);
    obj_to_room(g, obj, char_get_in_room(ch));
    char_set_gold(ch, char_gold_carry(ch));
    return;
  }

  char_set_gold(ch, char_get_gold(ch) + (value));
  extract_obj(g, obj);

  if (value == 1) {
    send_to_char(g, ch, "There was 1 zenni.\r\n");
  } else {
    send_to_char(g, ch, "There were %d zenni.\r\n", value);
    if (char_is_affected(ch, AFF_GROUP) && char_prf_flagged(g, ch, PRF_AUTOSPLIT)) {
      char split[MAX_INPUT_LENGTH];
      snprintf(split, sizeof(split), "%d", value);
      do_split(g, ch, split, 0, 0);
    }
  }
}

static void perform_get_from_container(GameWorld *g, struct char_data *ch, struct obj_data *obj,
                                       struct obj_data *cont, int mode)
{
  if (mode == FIND_OBJ_INV || mode == FIND_OBJ_EQUIP || can_take_obj(g, ch, obj)) {
    if (char_get_carry_items(ch) >= char_can_carry_n(ch)) {
      act(g, "$p: you can't hold any more items.", false, ch, obj, nullptr, TO_CHAR);
      return;
    }
    if (char_get_sits(g, ch) && obj_get_vnum(g, char_get_sits(g, ch)) > 603 && obj_get_vnum(g, char_get_sits(g, ch)) < 608 && obj_get_vnum(g, char_get_sits(g, ch)) - 4 != obj_get_vnum(g, cont) && obj_get_vnum(g, cont) > 599 && obj_get_vnum(g, cont) < 604) {
      send_to_char(g, ch, "You aren't playing at that table!\r\n");
      return;
    }
    if (get_otrigger(g, obj, ch)) {
      obj_from_obj(g, obj);
      obj_to_char(g, obj, ch);
      if (obj_extra_flagged(cont, ITEM_SHEATH)) {
        act(g, "You draw $p from $P.", false, ch, obj, cont, TO_CHAR);
        act(g, "$n draws $p from $P.", true, ch, obj, cont, TO_ROOM);
      } else {
        act(g, "You get $p from $P.", false, ch, obj, cont, TO_CHAR);
        act(g, "$n gets $p from $P.", true, ch, obj, cont, TO_ROOM);
      }
      if (obj_extra_flagged(obj, ITEM_HOT)) {
        if (char_get_bonuses(ch, BONUS_FIREPROOF) <= 0 && !char_is_demon(ch)) {
          decCurHealthPercent(g, ch, .25, 0);
          if (char_get_bonuses(ch, BONUS_FIREPRONE) > 0)
            decCurHealthPercent(g, ch, 1, 1);

          char_aff_set(ch, AFF_BURNED, true);
          act(g, "@RYou are burned by it!@n", true, ch, nullptr, nullptr, TO_CHAR);
          act(g, "@R$n@R is burned by it!@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
      }
      if (char_is_npc(ch)) {
        item_check(g, obj, ch);
      }
      get_check_money(g, ch, obj);
    }
  }
}

static void get_from_container(GameWorld *g, struct char_data *ch, struct obj_data *cont,
                               char *arg, int mode, int howmany)
{
  struct obj_data *obj;
  int obj_dotmode, found = 0;

  obj_dotmode = find_all_dots(arg);

  if (obj_val_flagged(cont, CONT_CLOSED))
    act(g, "$p is closed.", false, ch, cont, nullptr, TO_CHAR);
  else if (obj_dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, obj_get_contains(cont)))) {
      char out[MAX_STRING_LENGTH];

      snprintf(out, sizeof(out), "There doesn't seem to be %s %s in $p.", AN(arg), arg);
      act(g, out, false, ch, cont, nullptr, TO_CHAR);
    } else {
      for (obj_data *match : obj_list(g, obj)) {
        if (!obj_named_and_visible(g, ch, arg, match))
          continue;
        if (!howmany--)
          break;
        perform_get_from_container(g, ch, match, cont, mode);
      }
    }
  } else {
    if (obj_dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(g, ch, "Get all of what?\r\n");
      return;
    }
    for (obj_data *item : obj_contents(g, cont)) {
      if (char_can_see_obj(g, ch, item) &&
          (obj_dotmode == FIND_ALL || isname(arg, obj_get_name(item)))) {
        found = 1;
        perform_get_from_container(g, ch, item, cont, mode);
      }
    }
    if (!found) {
      if (obj_dotmode == FIND_ALL)
        act(g, "$p seems to be empty.", false, ch, cont, nullptr, TO_CHAR);
      else {
        char out[MAX_STRING_LENGTH];

        snprintf(out, sizeof(out), "You can't seem to find any %ss in $p.", arg);
        act(g, out, false, ch, cont, nullptr, TO_CHAR);
      }
    }
  }
}

bool perform_get_from_room(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{

  if (obj_get_sitting(g, obj)) {
    send_to_char(g, ch, "Someone is on that!\r\n");
    return false;
  }

  if (obj_extra_flagged(obj, ITEM_BURIED)) {
    send_to_char(g, ch, "Get what?\r\n");
    return false;
  }

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN1) || rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN2)) {
    send_to_char(g, ch, "You can't get things from a garden. Help garden.\r\n");
    return false;
  }

  if (can_take_obj(g, ch, obj) && get_otrigger(g, obj, ch)) {
    obj_from_room(g, obj);
    obj_to_char(g, obj, ch);
    act(g, "You get $p.", false, ch, obj, nullptr, TO_CHAR);
    act(g, "$n gets $p.", true, ch, obj, nullptr, TO_ROOM);

    if (obj_extra_flagged(obj, ITEM_HOT)) {
      if (char_get_bonuses(ch, BONUS_FIREPROOF) <= 0 && !char_is_demon(ch)) {
        decCurHealthPercent(g, ch, .25, 1);
        if (char_get_bonuses(ch, BONUS_FIREPRONE) > 0)
          decCurHealthPercent(g, ch, 1, 1);

        char_aff_set(ch, AFF_BURNED, true);
        act(g, "@RYou are burned by it!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act(g, "@R$n@R is burned by it!@n", true, ch, nullptr, nullptr, TO_ROOM);
      }
    }

    if (char_is_npc(ch))
      item_check(g, obj, ch);
    get_check_money(g, ch, obj);
  }
  return false;
}

static char *find_exdesc_keywords(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->keyword);

  return (nullptr);
}

static void get_from_room(GameWorld *g, struct char_data *ch, char *arg, int howmany)
{
  struct obj_data *obj;
  int dotmode, found = 0;
  char *descword;

  /* Are they trying to take something in a room extra description? */
  if (find_exdesc(arg, rroom_get_ex_description(g, char_get_in_room(ch))) != nullptr) {
    send_to_char(g, ch, "You can't take %s %s.\r\n", AN(arg), arg);
    return;
  }

  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV) {
    if ((descword = find_exdesc_keywords(arg, rroom_get_ex_description(g, char_get_in_room(ch)))) != nullptr)
      send_to_char(g, ch, "%s: you can't take that!\r\n", fname(descword));
    else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch)))))
      send_to_char(g, ch, "You don't see %s %s here.\r\n", AN(arg), arg);
    else {
      for (obj_data *match : obj_list(g, obj)) {
        if (!obj_named_and_visible(g, ch, arg, match))
          continue;
        if (!howmany--)
          break;
        perform_get_from_room(g, ch, match);
      }
    }
  } else {
    if (dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(g, ch, "Get all of what?\r\n");
      return;
    }
    for (obj_data *item : room_contents(g, char_get_in_room(ch))) {
      if (char_can_see_obj(g, ch, item) &&
          (dotmode == FIND_ALL || isname(arg, obj_get_name(item)))) {
        found = 1;
        perform_get_from_room(g, ch, item);
      }
    }
    if (!found) {
      if (dotmode == FIND_ALL)
        send_to_char(g, ch, "There doesn't seem to be anything here.\r\n");
      else
        send_to_char(g, ch, "You don't see any %ss here.\r\n", arg);
    }
  }
}

ACMD(do_get)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];

  int cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;

  one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments */

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }

  if (!*arg1)
    send_to_char(g, ch, "Get what?\r\n");
  else if (!*arg2)
    get_from_room(g, ch, arg1, 1);
  else if (is_number(arg1) && !*arg3)
    get_from_room(g, ch, arg2, atoi(arg1));
  else {
    int amount = 1;
    if (is_number(arg1)) {
      amount = atoi(arg1);
      strcpy(arg1, arg2); /* strcpy: OK (sizeof: arg1 == arg2) */
      strcpy(arg2, arg3); /* strcpy: OK (sizeof: arg2 == arg3) */
    }
    cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV) {
      mode = generic_find(g, arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
      if (!cont)
        send_to_char(g, ch, "You don't have %s %s.\r\n", AN(arg2), arg2);
      else if (obj_get_type_flag(cont) == ITEM_VEHICLE)
        send_to_char(g, ch, "You will need to enter it first.\r\n");
      else if ((obj_get_type_flag(cont) != ITEM_CONTAINER) &&
               !((obj_get_type_flag(cont) == ITEM_PORTAL) && (obj_val_flagged(cont, CONT_CLOSEABLE))))
        act(g, "$p is not a container.", false, ch, cont, nullptr, TO_CHAR);
      else
        get_from_container(g, ch, cont, arg1, mode, amount);
    } else {
      if (cont_dotmode == FIND_ALLDOT && !*arg2) {
        send_to_char(g, ch, "Get from all of what?\r\n");
        return;
      }
      for (obj_data *thing : char_inventory(g, ch))
        if (char_can_see_obj(g, ch, thing) &&
            (cont_dotmode == FIND_ALL || isname(arg2, obj_get_name(thing)))) {
          if (obj_get_type_flag(thing) == ITEM_CONTAINER) {
            found = 1;
            get_from_container(g, ch, thing, arg1, FIND_OBJ_INV, amount);
          } else if (cont_dotmode == FIND_ALLDOT) {
            found = 1;
            act(g, "$p is not a container.", false, ch, thing, nullptr, TO_CHAR);
          }
        }
      for (obj_data *item : room_contents(g, char_get_in_room(ch)))
        if (char_can_see_obj(g, ch, item) &&
            (cont_dotmode == FIND_ALL || isname(arg2, obj_get_name(item)))) {
          if (obj_get_type_flag(item) == ITEM_CONTAINER) {
            get_from_container(g, ch, item, arg1, FIND_OBJ_ROOM, amount);
            found = 1;
          } else if (cont_dotmode == FIND_ALLDOT) {
            act(g, "$p is not a container.", false, ch, item, nullptr, TO_CHAR);
            found = 1;
          }
        }
      if (!found) {
        if (cont_dotmode == FIND_ALL)
          send_to_char(g, ch, "You can't seem to find any containers.\r\n");
        else
          send_to_char(g, ch, "You can't seem to find any %ss here.\r\n", arg2);
      }
    }
  }
}

static void perform_drop_gold(GameWorld *g, struct char_data *ch, int amount,
                              int8_t mode, room_rnum RDR)
{
  struct obj_data *obj;

  if (amount <= 0)
    send_to_char(g, ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
  else if (char_get_gold(ch) < amount)
    send_to_char(g, ch, "You don't have that many zenni!\r\n");
  else {
    if (mode != SCMD_JUNK) {
      char_set_wait(ch, PULSE_1SEC); /* to prevent zenni-bombing */
      obj = create_money(g, amount);
      if (mode == SCMD_DONATE) {
        send_to_char(g, ch, "You throw some zenni into the air where it disappears in a puff of smoke!\r\n");
        act(g, "$n throws some gold into the air where it disappears in a puff of smoke!",
            false, ch, nullptr, nullptr, TO_ROOM);
        obj_to_room(g, obj, RDR);
        act(g, "$p suddenly appears in a puff of orange smoke!", 0, nullptr, obj, nullptr, TO_ROOM);
      } else {
        char out[MAX_STRING_LENGTH];

        if (!drop_wtrigger(g, obj, ch)) {
          extract_obj(g, obj);
          return;
        }

        snprintf(out, sizeof(out), "$n drops %s.", money_desc(amount));
        act(g, out, true, ch, nullptr, nullptr, TO_ROOM);

        send_to_char(g, ch, "You drop some zenni.\r\n");
        obj_to_room(g, obj, char_get_in_room(ch));
        if (char_get_admlevel(ch) > 0) {
          send_to_imm(g, "IMM DROP: %s dropped %s in room [%d]", char_get_name(ch), obj_get_short_description(obj), rroom_get_vnum(g, obj_get_in_room(obj)));
          log_imm_action(g, "IMM DROP: %s dropped %s in room [%d]", char_get_name(ch), obj_get_short_description(obj), rroom_get_vnum(g, obj_get_in_room(obj)));
          if (check_insidebag(g, obj, 0.0) > 1) {
            send_to_imm(g, "IMM DROP: Object contains %d other items.", check_insidebag(g, obj, 0.0));
            log_imm_action(g, "IMM DROP: Object contains %d other items.", check_insidebag(g, obj, 0.0));
          }
        }
      }
    } else {
      char out[MAX_STRING_LENGTH];

      snprintf(out, sizeof(out), "$n drops %s which disappears in a puff of smoke!", money_desc(amount));
      act(g, out, false, ch, nullptr, nullptr, TO_ROOM);

      send_to_char(g, ch, "You drop some zenni which disappears in a puff of smoke!\r\n");
    }
    char_set_gold(ch, char_get_gold(ch) - (amount));
  }
}

#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? "  It vanishes in a puff of smoke!" : "")

static int perform_drop(GameWorld *g, struct char_data *ch, struct obj_data *obj,
                        int8_t mode, const char *sname, room_rnum RDR)
{
  char out[MAX_STRING_LENGTH];
  int value;

  if (!drop_otrigger(g, obj, ch))
    return 0;

  if ((mode == SCMD_DROP) && !drop_wtrigger(g, obj, ch))
    return 0;

  if (obj_get_vnum(g, obj) == 17 || obj_get_vnum(g, obj) == 17998) {
    snprintf(out, sizeof(out), "You can't %s $p, it is grafted into your soul :P", sname);
    act(g, out, false, ch, obj, nullptr, TO_CHAR);
    return (0);
  }
  if (obj_get_vnum(g, obj) == 20 || obj_get_vnum(g, obj) == 21 || obj_get_vnum(g, obj) == 22 || obj_get_vnum(g, obj) == 23 || obj_get_vnum(g, obj) == 24 || obj_get_vnum(g, obj) == 25 || obj_get_vnum(g, obj) == 26) {
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_SPACE)) {
      snprintf(out, sizeof(out), "You can't %s $p in space!", sname);
      act(g, out, false, ch, obj, nullptr, TO_CHAR);
      return (0);
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN1) || rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN2)) {
      snprintf(out, sizeof(out), "You can't %s $p in here. Read help garden.", sname);
      act(g, out, false, ch, obj, nullptr, TO_CHAR);
      return (0);
    }
    if ((mode == SCMD_DROP) && obj_extra_flagged(obj, ITEM_NORENT)) {
      snprintf(out, sizeof(out), "You drop $p but it gets lost on the ground!");
      act(g, out, false, ch, obj, nullptr, TO_CHAR);
      obj_from_char(g, obj);
      extract_obj(g, obj);
      return (0);
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_NOINSTANT)) {
      snprintf(out, sizeof(out), "You can't %s $p in this protected area!", sname);
      act(g, out, false, ch, obj, nullptr, TO_CHAR);
      return (0);
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_SHIP)) {
      snprintf(out, sizeof(out), "You can't %s $p on a private ship!", sname);
      act(g, out, false, ch, obj, nullptr, TO_CHAR);
      return (0);
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_HOUSE)) {
      snprintf(out, sizeof(out), "You can't %s $p in a private house!", sname);
      act(g, out, false, ch, obj, nullptr, TO_CHAR);
      return (0);
    }
  }
  if (obj_extra_flagged(obj, ITEM_NODROP) && char_get_admlevel(ch) < 1) {
    snprintf(out, sizeof(out), "You can't %s $p, it must be CURSED!", sname);
    act(g, out, false, ch, obj, nullptr, TO_CHAR);
    return (0);
  }
  if ((mode == SCMD_DONATE || mode == SCMD_JUNK) && obj_extra_flagged(obj, ITEM_NODONATE)) {
    snprintf(out, sizeof(out), "You can't %s $p!", sname);
    act(g, out, false, ch, obj, nullptr, TO_CHAR);
    return (0);
  }

  snprintf(out, sizeof(out), "You %s $p.", sname);
  act(g, out, false, ch, obj, nullptr, TO_CHAR);

  snprintf(out, sizeof(out), "$n %ss $p.", sname);
  act(g, out, true, ch, obj, nullptr, TO_ROOM);

  obj_from_char(g, obj);

  switch (mode) {
    case SCMD_DROP:
      if (!obj_extra_flagged(obj, ITEM_UNBREAKABLE) && rroom_get_geffect(g, char_get_in_room(ch)) == 6) {
        act(g, "$p melts in the lava!", false, ch, obj, nullptr, TO_CHAR);
        act(g, "$p melts in the lava!", false, ch, obj, nullptr, TO_ROOM);
        extract_obj(g, obj);
      } else if (rroom_get_geffect(g, char_get_in_room(ch)) == 6) {
        act(g, "$p plops down on some cooled lava!", false, ch, obj, nullptr, TO_CHAR);
        act(g, "$p plops down on some cooled lava!", false, ch, obj, nullptr, TO_ROOM);
        obj_to_room(g, obj, char_get_in_room(ch));
        if (char_get_admlevel(ch) > 0) {
          send_to_imm(g, "IMM DROP: %s dropped %s in room [%d]", char_get_name(ch), obj_get_short_description(obj), rroom_get_vnum(g, obj_get_in_room(obj)));
          log_imm_action(g, "IMM DROP: %s dropped %s in room [%d]", char_get_name(ch), obj_get_short_description(obj), rroom_get_vnum(g, obj_get_in_room(obj)));
        }
      } else {
        obj_to_room(g, obj, char_get_in_room(ch));
        if (char_get_admlevel(ch) > 0) {
          send_to_imm(g, "IMM DROP: %s dropped %s in room [%d]", char_get_name(ch), obj_get_short_description(obj), rroom_get_vnum(g, obj_get_in_room(obj)));
          log_imm_action(g, "IMM DROP: %s dropped %s in room [%d]", char_get_name(ch), obj_get_short_description(obj), rroom_get_vnum(g, obj_get_in_room(obj)));
        }
      }
      return (0);
    case SCMD_DONATE:
      obj_to_room(g, obj, RDR);
      act(g, "$p suddenly appears in a puff a smoke!", false, nullptr, obj, nullptr, TO_ROOM);
      return (0);
    case SCMD_JUNK:
      value = MAX(1, MIN(200, obj_get_cost(obj) / 16));
      extract_obj(g, obj);
      return (value);
    default:
      mud_log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
      /*  SYSERR_DESC:
     *  This error comes from perform_drop() and is output when perform_drop()
     *  is called with an illegal 'mode' argument.
     */
      break;
  }

  return (0);
}

ACMD(do_drop)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  room_rnum RDR = 0;
  int8_t mode   = SCMD_DROP;
  int dotmode, multi, num_don_rooms;
  const char *sname;

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN1) || rroom_flagged(g, char_get_in_room(ch), ROOM_GARDEN2)) {
    send_to_char(g, ch, "You can not do that in a garden.\r\n");
    return;
  }

  switch (subcmd) {
    case SCMD_JUNK:
      sname = "junk";
      mode  = SCMD_JUNK;
      break;
    case SCMD_DONATE:
      sname = "donate";
      mode  = SCMD_DONATE;
      /* fail + double chance for room 1   */
      num_don_rooms = (CONFIG_DON_ROOM_1 != NOWHERE) * 2 +
                      (CONFIG_DON_ROOM_2 != NOWHERE) +
                      (CONFIG_DON_ROOM_3 != NOWHERE) + 1;
      switch (rand_number(g, 0, num_don_rooms)) {
        case 0:
          mode = SCMD_JUNK;
          break;
        case 1:
        case 2:
          RDR = real_room(g, CONFIG_DON_ROOM_1);
          break;
        case 3: RDR = real_room(g, CONFIG_DON_ROOM_2); break;
        case 4: RDR = real_room(g, CONFIG_DON_ROOM_3); break;
      }
      if (RDR == NOWHERE) {
        send_to_char(g, ch, "Sorry, you can't donate anything right now.\r\n");
        return;
      }
      break;
    default:
      sname = "drop";
      break;
  }

  argument = one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "What do you want to %s?\r\n", sname);
    return;
  }
  if (is_number(arg)) {
    multi = atoi(arg);
    one_argument(argument, arg);
    if (!strcasecmp("zenni", arg) || !strcasecmp("gold", arg))
      perform_drop_gold(g, ch, multi, mode, RDR);
    else if (multi <= 0)
      send_to_char(g, ch, "Yeah, that makes sense.\r\n");
    else if (!*arg)
      send_to_char(g, ch, "What do you want to %s %d of?\r\n", sname, multi);
    else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch))))
      send_to_char(g, ch, "You don't seem to have any %ss.\r\n", arg);
    else if (check_saveroom_count(g, ch, obj) > 150)
      send_to_char(g, ch, "The save room you are in can not hold anymore items! (150 max, count of items in containers is halved)\r\n");
    else {
      for (obj_data *match : obj_list(g, obj)) {
        if (!obj_named_and_visible(g, ch, arg, match))
          continue;
        perform_drop(g, ch, match, mode, sname, RDR);
        if (!--multi)
          break;
      }
    }
  } else {
    dotmode = find_all_dots(arg);

    /* Can't junk or donate all */
    if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
      if (subcmd == SCMD_JUNK)
        send_to_char(g, ch, "Go to the dump if you want to junk EVERYTHING!\r\n");
      else
        send_to_char(g, ch, "Go do the donation room if you want to donate EVERYTHING!\r\n");
      return;
    }
    if (dotmode == FIND_ALL) {
      int fail = false;

      if (!char_get_inventory(ch))
        send_to_char(g, ch, "You don't seem to be carrying anything.\r\n");
      else {
        for (obj_data *thing : char_inventory(g, ch)) {
          if (check_saveroom_count(g, ch, thing) > 150) {
            fail = true;
          } else {
            perform_drop(g, ch, thing, mode, sname, RDR);
          }
        }
        if (fail == true) {
          send_to_char(g, ch, "Some of the items couldn't be dropped into this save room. It is too full. (150 max, containers half the count inside)\r\n");
        }
      }
    } else if (dotmode == FIND_ALLDOT) {
      int fail = false;

      if (!*arg) {
        send_to_char(g, ch, "What do you want to %s all of?\r\n", sname);
        return;
      }
      if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch))))
        send_to_char(g, ch, "You don't seem to have any %ss.\r\n", arg);

      for (obj_data *match : obj_list(g, obj)) {
        if (!obj_named_and_visible(g, ch, arg, match))
          continue;
        obj = match;
        if (check_saveroom_count(g, ch, obj) > 150) {
          fail = true;
        } else {
          perform_drop(g, ch, obj, mode, sname, RDR);
        }
      }
      if (fail == true) {
        send_to_char(g, ch, "Some of the items couldn't be dropped into this save room. It is too full. (150 max, containers half the count inside)\r\n");
      }
    } else {
      if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
        send_to_char(g, ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      } else if (check_saveroom_count(g, ch, obj) > 150) {
        send_to_char(g, ch, "The item couldn't be dropped into this save room. It is too full. (150 max, containers half the count inside)\r\n");
      } else {
        perform_drop(g, ch, obj, mode, sname, RDR);
      }
    }
  }
}

static void perform_give(GameWorld *g, struct char_data *ch, struct char_data *vict,
                         struct obj_data *obj)
{
  if (!give_otrigger(g, obj, ch, vict))
    return;
  if (!receive_mtrigger(g, vict, ch, obj))
    return;

  if (obj_extra_flagged(obj, ITEM_NODROP)) {
    act(g, "You can't let go of $p!!  Yeech!", false, ch, obj, nullptr, TO_CHAR);
    return;
  }
  if (char_get_carry_items(vict) >= char_can_carry_n(vict)) {
    act(g, "$N seems to have $S hands full.", false, ch, nullptr, vict, TO_CHAR);
    if (char_is_npc(ch)) {
      act(g, "$n@n drops $p because you can't carry anymore.", true, ch, obj, vict, TO_VICT);
      act(g, "$n@n drops $p on the ground since $N's unable to carry it.", true, ch, obj, vict, TO_NOTVICT);
      obj_from_char(g, obj);
      obj_to_room(g, obj, char_get_in_room(ch));
    }
    return;
  }
  if (char_is_npc(vict) && (obj_extra_flagged(obj, ITEM_FORGED) || obj_extra_flagged(obj, ITEM_FORGED))) {
    act(g, "$n tries to hand $p to $N.", true, ch, obj, vict, TO_NOTVICT);
    do_say(g, vict, writable{"I don't want that piece of junk."}, 0, 0);
    return;
  }
  if (obj_get_weight(obj) + (getCurCarriedWeight(vict)) > getMaxCarryWeight(g, vict)) {
    act(g, "$E can't carry that much weight.", false, ch, nullptr, vict, TO_CHAR);
    if (char_is_npc(ch)) {
      act(g, "$n@n drops $p because you can't carry anymore.", true, ch, obj, vict, TO_VICT);
      act(g, "$n@n drops $p on the ground since $N's unable to carry it.", true, ch, obj, vict, TO_NOTVICT);
      obj_from_char(g, obj);
      obj_to_room(g, obj, char_get_in_room(ch));
    }
    return;
  }
  if ((obj_get_weight(obj) + rroom_get_gravity(g, char_get_in_room(vict))) + (getCurCarriedWeight(vict)) > getMaxCarryWeight(g, vict)) {
    act(g, "$E can't carry that much weight because of the gravity.", false, ch, nullptr, vict, TO_CHAR);
    if (char_is_npc(ch)) {
      act(g, "$n@n drops $p because you can't carry anymore.", true, ch, obj, vict, TO_VICT);
      act(g, "$n@n drops $p on the ground since $N's unable to carry it.", true, ch, obj, vict, TO_NOTVICT);
      obj_from_char(g, obj);
      obj_to_room(g, obj, char_get_in_room(ch));
    }
    return;
  }
  if (!char_is_npc(vict) && !char_is_npc(ch)) {
    if (char_prf_flagged(g, vict, PRF_NOGIVE)) {
      act(g, "$N refuses to accept $p at this time.", false, ch, obj, vict, TO_CHAR);
      act(g, "$n tries to give you $p, but you are refusing to be handed things.", false, ch, obj, vict, TO_VICT);
      act(g, "$n tries to give $N, $p, but $E is refusing to be handed things right now.", false, ch, obj, vict, TO_NOTVICT);
      return;
    }
  }
  obj_from_char(g, obj);
  obj_to_char(g, obj, vict);
  act(g, "You give $p to $N.", false, ch, obj, vict, TO_CHAR);
  act(g, "$n gives you $p.", false, ch, obj, vict, TO_VICT);
  act(g, "$n gives $p to $N.", true, ch, obj, vict, TO_NOTVICT);

  if (obj_extra_flagged(obj, ITEM_HOT)) {
    if (char_get_bonuses(vict, BONUS_FIREPROOF) <= 0 && !char_is_demon(vict)) {
      decCurHealthPercent(g, ch, .25, 1);
      if (char_get_bonuses(vict, BONUS_FIREPRONE) > 0)
        decCurHealthPercent(g, ch, 1, 1);

      char_aff_set(vict, AFF_BURNED, true);
      act(g, "@RYou are burned by it!@n", true, vict, nullptr, nullptr, TO_CHAR);
      act(g, "@R$n@R is burned by it!@n", true, vict, nullptr, nullptr, TO_ROOM);
    }
  }
}

/* utility function for give */
static struct char_data *give_find_vict(GameWorld *g, struct char_data *ch, char *arg)
{
  struct char_data *vict;

  skip_spaces(&arg);
  if (!*arg)
    send_to_char(g, ch, "To who?\r\n");
  else if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "%s", CONFIG_NOPERSON);
    if (char_is_npc(ch))
      send_to_imm(g, "Mob Give: Victim, %s, doesn't exist.", arg);
  } else if (vict == ch)
    send_to_char(g, ch, "What's the point of that?\r\n");
  else
    return (vict);

  return (nullptr);
}

static void perform_give_gold(GameWorld *g, struct char_data *ch, struct char_data *vict,
                              int amount)
{
  char out[MAX_STRING_LENGTH];

  if (amount <= 0) {
    send_to_char(g, ch, "Heh heh heh ... we are jolly funny today, eh?\r\n");
    return;
  }
  if ((char_get_gold(ch) < amount) && (char_is_npc(ch) || !char_adm_flagged(ch, ADM_MONEY))) {
    send_to_char(g, ch, "You don't have that much zenni!\r\n");
    return;
  }
  if (char_get_gold(vict) + amount > char_gold_carry(vict)) {
    send_to_char(g, ch, "They can't carry that much zenni.\r\n");
    return;
  }
  send_to_char(g, ch, "%s", CONFIG_OK);

  snprintf(out, sizeof(out), "$n gives you %d zenni.", amount);
  act(g, out, false, ch, nullptr, vict, TO_VICT);

  snprintf(out, sizeof(out), "$n gives %s to $N.", money_desc(amount));
  act(g, out, true, ch, nullptr, vict, TO_NOTVICT);

  if (char_is_npc(ch) || !char_adm_flagged(ch, ADM_MONEY))
    char_set_gold(ch, char_get_gold(ch) - (amount));
  char_set_gold(vict, char_get_gold(vict) + (amount));

  bribe_mtrigger(g, vict, ch, amount);
}

ACMD(do_give)
{
  char arg[MAX_STRING_LENGTH];
  int amount, dotmode;
  struct char_data *vict;
  struct obj_data *obj;

  argument = one_argument(argument, arg);

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }

  reveal_hiding(g, ch, 0);
  if (!*arg)
    send_to_char(g, ch, "Give what to who?\r\n");
  else if (is_number(arg)) {
    amount   = atoi(arg);
    argument = one_argument(argument, arg);
    if (!strcasecmp("zenni", arg) || !strcasecmp("gold", arg)) {
      one_argument(argument, arg);
      if ((vict = give_find_vict(g, ch, arg)) != nullptr) {
        perform_give_gold(g, ch, vict, amount);
        if (char_get_admlevel(ch) > 0 && !char_is_npc(vict)) {
          send_to_imm(g, "IMM GIVE: %s has given %s zenni to %s.", char_get_name(ch), add_commas(amount), char_get_name(vict));
          log_imm_action(g, "IMM GIVE: %s has given %s zenni to %s.", char_get_name(ch), add_commas(amount), char_get_name(vict));
        }
      }
      return;
    } else if (!*arg) /* Give multiple code. */
      send_to_char(g, ch, "What do you want to give %d of?\r\n", amount);
    else if (!(vict = give_find_vict(g, ch, argument)))
      return;
    else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch))))
      send_to_char(g, ch, "You don't seem to have any %ss.\r\n", arg);
    else {
      for (obj_data *match : obj_list(g, obj)) {
        if (!obj_named_and_visible(g, ch, arg, match))
          continue;
        if (!amount--)
          break;
        obj = match;
        perform_give(g, ch, vict, obj);
        if (char_get_admlevel(ch) > 0 && !char_is_npc(vict)) {
          send_to_imm(g, "IMM GIVE: %s has given %s to %s.", char_get_name(ch), obj_get_short_description(obj), char_get_name(vict));
          log_imm_action(g, "IMM GIVE: %s has given %s to %s.", char_get_name(ch), obj_get_short_description(obj), char_get_name(vict));
        }
      }
    }
  } else {
    char buf1[MAX_INPUT_LENGTH];

    one_argument(argument, buf1);
    if (!(vict = give_find_vict(g, ch, buf1)))
      return;
    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV) {
      if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch))))
        send_to_char(g, ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else {
        perform_give(g, ch, vict, obj);
        if (char_get_admlevel(ch) > 0 && !char_is_npc(vict)) {
          send_to_imm(g, "IMM GIVE: %s has given %s to %s.", char_get_name(ch), obj_get_short_description(obj), char_get_name(vict));
          log_imm_action(g, "IMM GIVE: %s has given %s to %s.", char_get_name(ch), obj_get_short_description(obj), char_get_name(vict));
        }
      }
    } else {
      if (dotmode == FIND_ALLDOT && !*arg) {
        send_to_char(g, ch, "All of what?\r\n");
        return;
      }
      if (!char_get_inventory(ch))
        send_to_char(g, ch, "You don't seem to be holding anything.\r\n");
      else
        for (obj_data *item : char_inventory(g, ch)) {
          if (char_can_see_obj(g, ch, item) &&
              ((dotmode == FIND_ALL || isname(arg, obj_get_name(item))))) {
            perform_give(g, ch, vict, item);
            if (char_get_admlevel(ch) > 0 && !char_is_npc(vict)) {
              send_to_imm(g, "IMM GIVE: %s has given %s to %s.", char_get_name(ch), obj_get_short_description(item), char_get_name(vict));
              log_imm_action(g, "IMM GIVE: %s has given %s to %s.", char_get_name(ch), obj_get_short_description(item), char_get_name(vict));
            }
          }
        }
    }
  }
}

void weight_change_object(GameWorld *g, struct obj_data *obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (obj_get_in_room(obj) != NOWHERE) {
    obj_set_weight(obj, obj_get_weight(obj) + (weight));
  } else if ((tmp_ch = obj_get_carried_by(obj))) {
    obj_from_char(g, obj);
    obj_set_weight(obj, obj_get_weight(obj) + (weight));
    obj_to_char(g, obj, tmp_ch);
  } else if ((tmp_obj = obj_get_in_obj(obj))) {
    obj_from_obj(g, obj);
    obj_set_weight(obj, obj_get_weight(obj) + (weight));
    obj_to_obj(g, obj, tmp_obj);
  } else {
    mud_log("SYSERR: Unknown attempt to subtract weight from an object.");
    /*  SYSERR_DESC:
     *  weight_change_object() outputs this error when weight is attempted to
     *  be removed from an object that is not carried or in another object.
     */
  }
}

void name_from_drinkcon(GameWorld *g, struct obj_data *obj)
{
  char *new_name, *cur_name, *next;
  const char *liqname;
  int liqlen, cpylen;

  if (!obj || (obj_get_type_flag(obj) != ITEM_DRINKCON && obj_get_type_flag(obj) != ITEM_FOUNTAIN))
    return;

  if (obj_get_value(obj, VAL_DRINKCON_LIQUID) < 0 || obj_get_value(obj, VAL_DRINKCON_LIQUID) >= NUM_LIQ_TYPES)
    return;
  liqname = drinknames[obj_get_value(obj, VAL_DRINKCON_LIQUID)];
  if (!isname(liqname, obj_get_name(obj))) {
    /*log("SYSERR: Can't remove liquid '%s' from '%s' (%d) item.", liqname, obj->name, obj->item_number);*/
    /*  SYSERR_DESC:
     *  From name_from_drinkcon(), this error comes about if the object
     *  noted (by keywords and item vnum) does not contain the liquid string
     *  being searched for.
     */
    return;
  }

  liqlen = strlen(liqname);
  new_name = str_alloc(strlen(obj_get_name(obj)) + 1); /* the name without one or more of its words */

  for (cur_name = obj_get_name(obj); cur_name; cur_name = next) {
    if (*cur_name == ' ')
      cur_name++;

    if ((next = strchr(cur_name, ' ')))
      cpylen = next - cur_name;
    else
      cpylen = strlen(cur_name);

    if (!strncasecmp(cur_name, liqname, liqlen))
      continue;

    if (*new_name)
      strcat(new_name, " ");
    size_t used = strlen(new_name);
    memcpy(new_name + used, cur_name, cpylen);
    new_name[used + cpylen] = '\0';
  }

  if (obj_get_item_number(obj) == NOTHING || obj_get_name(obj) != obj_get_name(&g->obj_proto[obj_get_item_number(obj)]))
    free(obj_get_name(obj));
  obj_set_name(obj, new_name);
}

void name_to_drinkcon(GameWorld *g, struct obj_data *obj, int type)
{
  char *new_name;
  size_t len;

  if (!obj || (obj_get_type_flag(obj) != ITEM_DRINKCON && obj_get_type_flag(obj) != ITEM_FOUNTAIN))
    return;
  if (type < 0 || type >= NUM_LIQ_TYPES)
    return;

  len = strlen(obj_get_name(obj)) + strlen(drinknames[type]) + 2;
  new_name = str_alloc(len);
  snprintf(new_name, len, "%s %s", obj_get_name(obj), drinknames[type]);

  if (obj_get_item_number(obj) == NOTHING || obj_get_name(obj) != obj_get_name(&g->obj_proto[obj_get_item_number(obj)]))
    free(obj_get_name(obj));

  obj_set_name(obj, new_name);
}

ACMD(do_drink)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *temp;
  struct affected_type af = {};
  int amount, weight, wasthirsty = 0;
  int on_ground = 0;

  one_argument(argument, arg);
  if (char_is_npc(ch)) {
    return;
  }
  if (char_is_android(ch) || char_get_conditions(g, ch, THIRST) < 0) {
    send_to_char(g, ch, "You need not drink!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_get_conditions(g, ch, HUNGER) <= 1 && char_get_conditions(g, ch, THIRST) >= 2 && !char_is_namek(ch) && char_get_genome(ch, 0) != 3 && char_get_genome(ch, 1) != 3) {
    send_to_char(g, ch, "You need to eat first!\r\n");
    return;
  }
  wasthirsty = char_get_conditions(g, ch, THIRST);
  if (!*arg && !char_is_npc(ch)) {
    char out[MAX_STRING_LENGTH];
    switch (rroom_sect(g, char_get_in_room(ch))) {
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
      case SECT_UNDERWATER:
        snprintf(out, sizeof(out), "$n takes a refreshing drink from the surrounding water.");
        act(g, out, true, ch, nullptr, nullptr, TO_ROOM);
        send_to_char(g, ch, "You take a refreshing drink from the surrounding water.\r\n");
        gain_condition(g, ch, THIRST, 1);
        if (char_get_skill(ch, SKILL_WELLSPRING) && (getCurKI(g, ch)) < getMaxKI(g, ch) && wasthirsty <= 30 && subcmd != SCMD_SIP) {

          incCurKI(g, ch, ((getMaxKI(g, ch) * 0.005) + (char_get_aff_abils_wis(ch) * rand_number(g, 80, 100))) * char_get_skill(ch, SKILL_WELLSPRING), true);

          send_to_char(g, ch, "You feel your ki return to full strength.\r\n");
        }
        if (char_get_conditions(g, ch, THIRST) >= 48)
          send_to_char(g, ch, "You don't feel thirsty anymore.\r\n");
        return;
      default:
        if (!rroom_sunken(g, char_get_in_room(ch))) {
          send_to_char(g, ch, "Drink from what?\r\n");
          return;
        }
        snprintf(out, sizeof(out), "$n takes a refreshing drink from the surrounding water.");
        act(g, out, true, ch, nullptr, nullptr, TO_ROOM);
        send_to_char(g, ch, "You take a refreshing drink from the surrounding water.\r\n");
        gain_condition(g, ch, THIRST, 1);
        if (char_get_skill(ch, SKILL_WELLSPRING) && !isFullKI(ch) && wasthirsty <= 30 && subcmd != SCMD_SIP) {
          if (incCurKI(g, ch, ((getMaxKI(g, ch) * 0.005) + (char_get_aff_abils_wis(ch) * rand_number(g, 80, 100))) * char_get_skill(ch, SKILL_WELLSPRING), true) == getMaxKI(g, ch)) {
            send_to_char(g, ch, "You feel your ki return to full strength.\r\n");
          } else {
            send_to_char(g, ch, "You feel your ki has rejuvenated.\r\n");
          }
        }
        if (char_get_conditions(g, ch, THIRST) >= 48)
          send_to_char(g, ch, "You don't feel thirsty anymore.\r\n");
        return;
        ;
    }
  }
  if (!(temp = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    if (!(temp = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "You can't find it!\r\n");
      return;
    } else
      on_ground = 1;
  }
  if ((obj_get_type_flag(temp) != ITEM_DRINKCON) &&
      (obj_get_type_flag(temp) != ITEM_FOUNTAIN)) {
    if (obj_get_vnum(g, temp) == 86 && on_ground != 1) {
      act(g, "@wYou uncork the $p and tip it to your lips. Drinking it down you feel a warmth flow through your body and your ki returns to full strength .@n", true, ch, temp, nullptr, TO_CHAR);
      act(g, "@C$n@w uncorks the $p and tips it to $s lips. Drinking it down and then smiling.@n", true, ch, temp, nullptr, TO_ROOM);
      obj_from_char(g, temp);
      extract_obj(g, temp);
      restoreKI(ch, true);
      gain_condition(g, ch, THIRST, 8);
      return;
    }
    if (obj_get_vnum(g, temp) == 86 && on_ground == 1) {
      send_to_char(g, ch, "You need to pick that up first.\r\n");
      return;
    }
    send_to_char(g, ch, "You can't drink from that!\r\n");
    return;
  }
  if (on_ground && (obj_get_type_flag(temp) == ITEM_DRINKCON)) {
    send_to_char(g, ch, "You have to be holding that to drink from it.\r\n");
    return;
  }
  if (obj_extra_flagged(temp, ITEM_BROKEN)) {
    send_to_char(g, ch, "Seems like it's broken!\r\n");
    return;
  }
  if (obj_extra_flagged(temp, ITEM_FORGED)) {
    send_to_char(g, ch, "Seems like it doesn't work, maybe it is fake...\r\n");
    return;
  }
  if (char_is_npc(ch)) { /* Cannot use GET_COND() on mobs. */
    act(g, "$n@w drinks from $p.", true, ch, temp, nullptr, TO_ROOM);
    obj_from_char(g, temp);
    extract_obj(g, temp);
    return;
  }
  if ((char_get_conditions(g, ch, DRUNK) > 10) && (char_get_conditions(g, ch, THIRST) > 0)) {
    /* The pig is drunk */
    send_to_char(g, ch, "You can't seem to get close enough to your mouth.\r\n");
    act(g, "$n tries to drink but misses $s mouth!", true, ch, nullptr, nullptr, TO_ROOM);
    return;
  }
  if (obj_get_value(temp, VAL_DRINKCON_HOWFULL) <= 0 && (obj_get_value(temp, VAL_DRINKCON_CAPACITY) >= 1)) {
    send_to_char(g, ch, "It's empty.\r\n");
    return;
  }

  if (!consume_otrigger(g, temp, ch, OCMD_DRINK)) /* check trigger */
    return;

  if (subcmd == SCMD_DRINK) {
    char out[MAX_STRING_LENGTH];

    snprintf(out, sizeof(out), "$n drinks %s from $p.", drinks[obj_get_value(temp, VAL_DRINKCON_LIQUID)]);
    act(g, out, true, ch, temp, nullptr, TO_ROOM);

    send_to_char(g, ch, "You drink the %s.\r\n", drinks[obj_get_value(temp, VAL_DRINKCON_LIQUID)]);
    if (obj_get_action_description(temp))
      act(g, obj_get_action_description(temp), true, ch, temp, nullptr, TO_CHAR);

    /* if (drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK] > 0)
      amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK];
    else
      amount = rand_number(3, 10);*/

    amount = 4;

  } else {
    act(g, "$n sips from $p.", true, ch, temp, nullptr, TO_ROOM);
    send_to_char(g, ch, "It tastes like %s.\r\n", drinks[obj_get_value(temp, VAL_DRINKCON_LIQUID)]);
    if (obj_get_value(temp, VAL_DRINKCON_POISON)) {
      send_to_char(g, ch, "It has a sickening taste! Better not eat it...");
      return;
    }
    amount = 1;
  }

  amount = MIN(amount, obj_get_value(temp, VAL_DRINKCON_HOWFULL));

  /* You can't subtract more than the object weighs
     Objects that are eternal (max capacity -1) don't get a
     weight subtracted */
  if (obj_get_value(temp, VAL_DRINKCON_CAPACITY) > 0) {
    weight = MIN(amount, obj_get_weight(temp));
    weight_change_object(g, temp, -weight); /* Subtract amount */
  }

  gain_condition(g, ch, DRUNK, drink_aff[obj_get_value(temp, VAL_DRINKCON_LIQUID)][DRUNK] * amount);
  gain_condition(g, ch, HUNGER, drink_aff[obj_get_value(temp, VAL_DRINKCON_LIQUID)][HUNGER] * amount);
  gain_condition(g, ch, THIRST, drink_aff[obj_get_value(temp, VAL_DRINKCON_LIQUID)][THIRST] * amount);
  if (char_get_foodr(ch) == 0 && subcmd != SCMD_SIP) {
    incCurST(g, ch, (getMaxST(g, ch) / 100) * amount, true);
    char_set_foodr(ch, 2);
    send_to_char(g, ch, "You feel rejuvinated by it.\r\n");
  }
  if (char_get_skill(ch, SKILL_WELLSPRING) && (getCurKI(g, ch)) < getMaxKI(g, ch) && wasthirsty <= 30 && subcmd != SCMD_SIP) {
    if (obj_get_value(temp, VAL_DRINKCON_LIQUID) == 0 || obj_get_value(temp, VAL_DRINKCON_LIQUID) == 14 || obj_get_value(temp, VAL_DRINKCON_LIQUID) == 15) {

      if (incCurKI(g, ch, ((getMaxKI(g, ch) * 0.005) + (char_get_aff_abils_wis(ch) * rand_number(g, 80, 100))) * char_get_skill(ch, SKILL_WELLSPRING), true) == getMaxKI(g, ch)) {
        send_to_char(g, ch, "You feel your ki return to full strength.\r\n");
      } else {
        send_to_char(g, ch, "You feel your ki has rejuvenated.\r\n");
      }
    }
  }

  if (char_get_conditions(g, ch, DRUNK) > 10)
    send_to_char(g, ch, "You feel drunk.\r\n");

  if (char_get_conditions(g, ch, THIRST) >= 48)
    send_to_char(g, ch, "You don't feel thirsty anymore.\r\n");

  if (obj_get_value(temp, VAL_DRINKCON_POISON) && (!char_is_mutant(ch) || (char_get_genome(ch, 0) != 7 && char_get_genome(ch, 1) != 7))) { /* The crap was poisoned 
! */
    send_to_char(g, ch, "Oops, it tasted rather strange!\r\n");
    act(g, "$n chokes and utters some strange sounds.", true, ch, nullptr, nullptr, TO_ROOM);

    af.type      = SPELL_POISON;
    af.duration  = amount * 3;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(g, ch, &af, false, false, false, false);
  }
  /* empty the container, and no longer poison.
     Only remove if it's max capacity > 0, not eternal */
  if (obj_get_value(temp, VAL_DRINKCON_CAPACITY) > 0) {
    obj_set_value(temp, VAL_DRINKCON_HOWFULL, obj_get_value(temp, VAL_DRINKCON_HOWFULL) - (amount));
    if (!obj_get_value(temp, VAL_DRINKCON_HOWFULL)) { /* The last bit */
      name_from_drinkcon(g, temp);
      obj_set_value(temp, VAL_DRINKCON_POISON, 0);
    }
  }
  return;
}

ACMD(do_eat)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *food;
  struct affected_type af = {};
  int amount;

  one_argument(argument, arg);

  if (char_is_npc(ch)) /* Cannot use GET_COND() on mobs. */
    return;

  if (char_is_android(ch) || char_get_conditions(g, ch, HUNGER) < 0) {
    send_to_char(g, ch, "You need not eat!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_POISON)) {
    send_to_char(g, ch, "You feel too sick from the poison to eat!\r\n");
    return;
  }
  if (!*arg) {
    send_to_char(g, ch, "Eat what?\r\n");
    return;
  }
  if (!(food = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    return;
  }
  if (char_get_conditions(g, ch, THIRST) <= 1 && char_get_conditions(g, ch, HUNGER) >= 2) {
    send_to_char(g, ch, "You need to drink first!\r\n");
    return;
  }
  if (subcmd == SCMD_TASTE && ((obj_get_type_flag(food) == ITEM_DRINKCON) ||
                               (obj_get_type_flag(food) == ITEM_FOUNTAIN))) {
    do_drink(g, ch, argument, 0, SCMD_SIP);
    return;
  }
  if (!(obj_get_type_flag(food) == ITEM_FOOD || obj_get_type_flag(food) == ITEM_YUM)) {
    send_to_char(g, ch, "You can't eat THAT!\r\n");
    return;
  }

  if (!consume_otrigger(g, food, ch, OCMD_EAT)) /* check trigger */
    return;

  int majin_gain_mode      = -100;
  double majin_st_recovery = 0.0;

  switch (obj_get_vnum(g, food)) {
    case 53:
      majin_gain_mode   = -1;
      majin_st_recovery = 0.033;
      break;
    case 93:
      majin_gain_mode   = 0;
      majin_st_recovery = 0.05;
      break;
    case 94:
      majin_gain_mode   = 1;
      majin_st_recovery = 0.1;
      break;
    case 95:
      majin_gain_mode   = 2;
      majin_st_recovery = 0.1;
      break;
  }

  if (majin_gain_mode != -100) {
    incCurLFPercent(g, ch, 0.01, true);
    incCurSTPercent(g, ch, majin_st_recovery, true);
    if (obj_extra_flagged(food, ITEM_FORGED)) {
      send_to_char(g, ch, "This is a forgery. You gain nothing!\r\n");
    } else {
      send_to_char(g, ch, "You feel more energetic!\r\n");
      majin_gain(g, ch, majin_gain_mode);
    }
    extract_obj(g, food);
    return;
  }

  if (char_get_conditions(g, ch, HUNGER) >= 48) {
    if (char_is_majin(ch)) {
      send_to_char(g, ch, "You are full, but there's always room for candy!\r\n");
    } else {
      send_to_char(g, ch, "You are full.\r\n");
    }
    return;
  }

  if (subcmd == SCMD_EAT) {
    act(g, "You eat some of $p.", false, ch, food, nullptr, TO_CHAR);
    if (obj_get_action_description(food))
      act(g, obj_get_action_description(food), false, ch, food, nullptr, TO_CHAR);
    act(g, "$n eats some of $p.", true, ch, food, nullptr, TO_ROOM);
  } else {
    act(g, "You nibble a little bit of $p.", false, ch, food, nullptr, TO_CHAR);
    act(g, "$n tastes a little bit of $p.", true, ch, food, nullptr, TO_ROOM);
    if (obj_get_value(food, VAL_FOOD_POISON)) {
      send_to_char(g, ch, "It has a sickening taste! Better not eat it...");
      return;
    }
  }

  // available hunger
  int foob    = 48 - char_get_conditions(g, ch, HUNGER);
  int maxfval = MAX(1, obj_get_value(food, VAL_FOOD_MAXFOODVAL));
  // amount that can be eaten - tasting only consumes 1 point, eating consumes as much as possible
  // VAL_FOOD_FOODVAL is the amount of nutrition remaining, while value 1 is the max possible it once had.
  amount               = MAX(1, MIN(foob, (subcmd == SCMD_EAT ? obj_get_value(food, VAL_FOOD_FOODVAL) : 1)));
  double percent_eaten = (double)amount / (double)maxfval;

  gain_condition(g, ch, HUNGER, amount);
  if (char_get_foodr(ch) == 0 && subcmd != SCMD_TASTE) {
    incCurSTPercent(g, ch, 0.01 * (double)amount, true);
    char_set_foodr(ch, 2);
    if (obj_extra_flagged(food, ITEM_YUM)) {
      incCurSTPercent(g, ch, .25, true);
    }
    send_to_char(g, ch, "You feel rejuvinated by it.\r\n");
  }

  if (subcmd != SCMD_TASTE) {
    int psbonus  = (int)((double)(maxfval + obj_get_value(food, VAL_FOOD_PSBONUS)) * percent_eaten);
    int expbonus = (int)((double)obj_get_value(food, VAL_FOOD_EXPBONUS) * ((char_get_level(ch) * 0.4) + 1) * percent_eaten);
    gain_exp(g, ch, expbonus);
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) + (psbonus));
    send_to_char(g, ch, "That was exceptionally delicious! @D[@mPS@D: @C+%d@D] [@gEXP@D: @G+%s@D]@n\r\n", psbonus, add_commas(expbonus));
    if (!obj_get_value(food, VAL_FOOD_POISON) && getCurPL(g, ch) < (getEffMaxPL(g, ch)) && subcmd != SCMD_TASTE) {
      if (obj_get_weight(food) < 6) {
        incCurHealthPercent(g, ch, .05, true);
      } else {
        incCurHealthPercent(g, ch, .1, true);
      }
      if (obj_extra_flagged(food, ITEM_YUM)) {
        incCurHealthPercent(g, ch, .2, true);
      }

      send_to_char(g, ch, "@MYou feel some of your strength return!@n\r\n");
    }
  }

  if (char_get_conditions(g, ch, HUNGER) >= 48) {
    if (char_is_majin(ch)) {
      send_to_char(g, ch, "You are full, but there's always room for candy!\r\n");
    } else {
      send_to_char(g, ch, "You are full.\r\n");
    }
  }

  if (obj_get_value(food, VAL_FOOD_POISON) && !char_adm_flagged(ch, ADM_NOPOISON)) {
    /* The crap was poisoned ! */
    send_to_char(g, ch, "Oops, that tasted rather strange!\r\n");
    act(g, "$n coughs and utters some strange sounds.", false, ch, nullptr, nullptr, TO_ROOM);

    af.type      = SPELL_POISON;
    af.duration  = amount * 2;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(g, ch, &af, false, false, false, false);
  }

  obj_set_value(food, VAL_FOOD_FOODVAL, obj_get_value(food, VAL_FOOD_FOODVAL) - (amount));
  if (obj_get_value(food, VAL_FOOD_FOODVAL) <= 0) {
    send_to_char(g, ch, "You finish the last bite.\r\n");
    extract_obj(g, food);
  }
}

static void majin_gain(GameWorld *g, struct char_data *ch, int type)
{
  if (!char_is_majin(ch) || char_is_npc(ch)) {
    return;
  }
  /* Rillao: transloc, add new transes here */
  if (is_soft_cap(ch, 0, 1.0)) {
    send_to_char(g, ch, "You can not gain anymore from candy consumption at your current level.\r\n");
    return;
  }

  int64_t st = 0, ki = 0, pl = 0;

  switch (type) {
    case -1:
      st = MIN(300000L, ((getBaseST(ch)) / 1200) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      pl = MIN(300000L, ((getBasePL(ch)) / 1200) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      ki = MIN(300000L, ((getBaseKI(ch)) / 1200) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      break;
    case 0:
      st = MIN(500000L, ((getBaseST(ch)) / 1200) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      pl = MIN(500000L, ((getBasePL(ch)) / 1200) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      ki = MIN(500000L, ((getBaseKI(ch)) / 1200) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      break;
    case 1:
      st = MIN(1200000L, ((getBaseST(ch)) / 1000) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      pl = MIN(1200000L, ((getBasePL(ch)) / 1000) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      ki = MIN(1200000L, ((getBaseKI(ch)) / 1000) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      break;
    case 2:
      st = MIN(1500000L, ((getBaseST(ch)) / 900) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      pl = MIN(1500000L, ((getBasePL(ch)) / 900) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      ki = MIN(1500000L, ((getBaseKI(ch)) / 900) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2));
      break;
  }
  gainBasePL(ch, pl, true);
  gainBaseKI(ch, ki, true);
  gainBaseST(ch, st, true);

  send_to_char(g, ch, "@mYou feel stronger after consuming the candy @D[@RPL@W: @r%s @CKi@D: @c%s @GSt@D: @g%s@D]@m!@n\r\n", add_commas(pl), add_commas(ki), add_commas(st));
}

ACMD(do_pour)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj = nullptr, *to_obj = nullptr;
  int amount = 0;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR) {
    if (!*arg1) { /* No arguments */
      send_to_char(g, ch, "From what do you want to pour?\r\n");
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You can't find it!\r\n");
      return;
    }
    if (obj_get_type_flag(from_obj) != ITEM_DRINKCON) {
      send_to_char(g, ch, "You can't pour from that!\r\n");
      return;
    }
  }
  if (subcmd == SCMD_FILL) {
    if (!*arg1) { /* no arguments */
      send_to_char(g, ch, "What do you want to fill?  And what are you filling it from?\r\n");
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You can't find it!\r\n");
      return;
    }
    if (obj_get_type_flag(to_obj) != ITEM_DRINKCON) {
      act(g, "You can't fill $p!", false, ch, to_obj, nullptr, TO_CHAR);
      return;
    }
    if (!*arg2) { /* no 2nd argument */
      act(g, "What do you want to fill $p from?", false, ch, to_obj, nullptr, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(g, ch, arg2, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
      return;
    }
    if (obj_get_type_flag(from_obj) != ITEM_FOUNTAIN && !obj_extra_flagged(from_obj, ITEM_BROKEN)) {
      act(g, "You can't fill something from $p.", false, ch, from_obj, nullptr, TO_CHAR);
      return;
    }
    if (obj_get_type_flag(from_obj) == ITEM_FOUNTAIN && obj_extra_flagged(from_obj, ITEM_BROKEN)) {
      act(g, "You can't fill something from a broken fountain.", false, ch, from_obj, nullptr, TO_CHAR);
      return;
    }
  }
  if (obj_get_value(from_obj, VAL_DRINKCON_HOWFULL) == 0) {
    act(g, "The $p is empty.", false, ch, from_obj, nullptr, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR) { /* pour */
    if (!*arg2) {
      send_to_char(g, ch, "Where do you want it?  Out or in what?\r\n");
      return;
    }
    if (!strcasecmp(arg2, "out")) {
      if (obj_get_value(from_obj, VAL_DRINKCON_CAPACITY) > 0) {
        act(g, "$n empties $p.", true, ch, from_obj, nullptr, TO_ROOM);
        act(g, "You empty $p.", false, ch, from_obj, nullptr, TO_CHAR);

        weight_change_object(g, from_obj, -obj_get_value(from_obj, VAL_DRINKCON_HOWFULL)); /* Empty */

        name_from_drinkcon(g, from_obj);
        obj_set_value(from_obj, VAL_DRINKCON_HOWFULL, 0);
        obj_set_value(from_obj, VAL_DRINKCON_LIQUID, 0);
        obj_set_value(from_obj, VAL_DRINKCON_POISON, 0);
      } else {
        send_to_char(g, ch, "You can't possibly pour that container out!\r\n");
      }

      return;
    }
    if (!(to_obj = get_obj_in_list_vis(g, ch, arg2, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You can't find it!\r\n");
      return;
    }
    if ((obj_get_type_flag(to_obj) != ITEM_DRINKCON) &&
        (obj_get_type_flag(to_obj) != ITEM_FOUNTAIN)) {
      send_to_char(g, ch, "You can't pour anything into that.\r\n");
      return;
    }
  }
  if (to_obj == from_obj) {
    send_to_char(g, ch, "A most unproductive effort.\r\n");
    return;
  }
  if ((obj_get_value(to_obj, VAL_DRINKCON_HOWFULL) != 0) &&
      (obj_get_value(to_obj, VAL_DRINKCON_LIQUID) != obj_get_value(from_obj, VAL_DRINKCON_LIQUID))) {
    send_to_char(g, ch, "There is already another liquid in it!\r\n");
    return;
  }
  if ((obj_get_value(to_obj, VAL_DRINKCON_CAPACITY) < 0) ||
      (!(obj_get_value(to_obj, VAL_DRINKCON_HOWFULL) < obj_get_value(to_obj, VAL_DRINKCON_CAPACITY)))) {
    send_to_char(g, ch, "There is no room for more.\r\n");
    return;
  }
  if (subcmd == SCMD_POUR)
    send_to_char(g, ch, "You pour the %s into the %s.", drinks[obj_get_value(from_obj, VAL_DRINKCON_LIQUID)], arg2);

  if (subcmd == SCMD_FILL) {
    act(g, "You gently fill $p from $P.", false, ch, to_obj, from_obj, TO_CHAR);
    act(g, "$n gently fills $p from $P.", true, ch, to_obj, from_obj, TO_ROOM);
  }
  /* New alias */
  if (obj_get_value(to_obj, VAL_DRINKCON_HOWFULL) == 0)
    name_to_drinkcon(g, to_obj, obj_get_value(from_obj, VAL_DRINKCON_LIQUID));

  /* First same type liq. */
  obj_set_value(to_obj, VAL_DRINKCON_LIQUID, obj_get_value(from_obj, VAL_DRINKCON_LIQUID));

  /* Then how much to pour */
  if (obj_get_value(from_obj, VAL_DRINKCON_CAPACITY) > 0) {
    obj_set_value(from_obj, VAL_DRINKCON_HOWFULL, obj_get_value(from_obj, VAL_DRINKCON_HOWFULL) - ((amount = (obj_get_value(to_obj, VAL_DRINKCON_CAPACITY) - obj_get_value(to_obj, VAL_DRINKCON_HOWFULL)))));

    obj_set_value(to_obj, VAL_DRINKCON_HOWFULL, obj_get_value(to_obj, VAL_DRINKCON_CAPACITY));

    if (obj_get_value(from_obj, VAL_DRINKCON_HOWFULL) < 0) { /* There was too little */
      obj_set_value(to_obj, VAL_DRINKCON_HOWFULL, obj_get_value(to_obj, VAL_DRINKCON_HOWFULL) + (obj_get_value(from_obj, VAL_DRINKCON_HOWFULL)));
      amount += obj_get_value(from_obj, VAL_DRINKCON_HOWFULL);
      name_from_drinkcon(g, from_obj);
      obj_set_value(from_obj, VAL_DRINKCON_HOWFULL, 0);
      obj_set_value(from_obj, VAL_DRINKCON_LIQUID, 0);
      obj_set_value(from_obj, VAL_DRINKCON_POISON, 0);
    }
  } else {
    obj_set_value(to_obj, VAL_DRINKCON_HOWFULL, obj_get_value(to_obj, VAL_DRINKCON_CAPACITY));
  }

  /* Then the poison boogie */
  obj_set_value(to_obj, VAL_DRINKCON_POISON, (obj_get_value(to_obj, VAL_DRINKCON_POISON) || obj_get_value(from_obj, VAL_DRINKCON_POISON)));

  /* And the weight boogie for non-eternal from_objects */
  if (obj_get_value(from_obj, VAL_DRINKCON_CAPACITY) > 0) {
    weight_change_object(g, from_obj, -amount);
  }
  weight_change_object(g, to_obj, amount); /* Add weight */
}

static void wear_message(GameWorld *g, struct char_data *ch, struct obj_data *obj, int where)
{
  const char *wear_messages[][2] = {
      {"$n lights $p and holds it.",
       "You light $p and hold it."                 },

      {"$n slides $p on to $s right ring finger.",
       "You slide $p on to your right ring finger."},

      {"$n slides $p on to $s left ring finger.",
       "You slide $p on to your left ring finger." },

      {"$n wears $p around $s neck.",
       "You wear $p around your neck."             },

      {"$n wears $p around $s neck.",
       "You wear $p around your neck."             },

      {"$n wears $p on $s body.",
       "You wear $p on your body."                 },

      {"$n wears $p on $s head.",
       "You wear $p on your head."                 },

      {"$n puts $p on $s legs.",
       "You put $p on your legs."                  },

      {"$n wears $p on $s feet.",
       "You wear $p on your feet."                 },

      {"$n puts $p on $s hands.",
       "You put $p on your hands."                 },

      {"$n wears $p on $s arms.",
       "You wear $p on your arms."                 },

      {"$n straps $p around $s arm as a shield.",
       "You start to use $p as a shield."          },

      {"$n wears $p about $s body.",
       "You wear $p around your body."             },

      {"$n wears $p around $s waist.",
       "You wear $p around your waist."            },

      {"$n puts $p on around $s right wrist.",
       "You put $p on around your right wrist."    },

      {"$n puts $p on around $s left wrist.",
       "You put $p on around your left wrist."     },

      {"$n wields $p.",
       "You wield $p."                             },

      {"$n grabs $p.",
       "You grab $p."                              },

      {"$n wears $p on $s back.",
       "You wear $p on your back."                 },

      {"$n puts $p in $s right ear.",
       "You put $p in your right ear."             },

      {"$n puts $p in $s left ear.",
       "You put $p in your left ear."              },

      {"$n wears $p as a cape.",
       "You wear $p as a cape."                    },

      {"$n covers $s left eye with $p.",
       "You wear $p over your left eye."           }
  };

  act(g, wear_messages[where][0], true, ch, obj, nullptr, TO_ROOM);
  act(g, wear_messages[where][1], false, ch, obj, nullptr, TO_CHAR);
}

static int hands(struct char_data *ch)
{
  int x;

  if (char_get_equipment(ch, WEAR_WIELD1)) {
    if (obj_extra_flagged(char_get_equipment(ch, WEAR_WIELD1), ITEM_2H) || wield_type(get_size(ch), char_get_equipment(ch, WEAR_WIELD1)) == WIELD_TWOHAND) {
      x = 2;
    } else
      x = 1;
  } else x = 0;

  if (char_get_equipment(ch, WEAR_WIELD2)) {
    if (obj_extra_flagged(char_get_equipment(ch, WEAR_WIELD2), ITEM_2H) || wield_type(get_size(ch), char_get_equipment(ch, WEAR_WIELD2)) == WIELD_TWOHAND) {
      x += 2;
    } else
      x += 1;
  }

  return x;
}

void perform_wear(GameWorld *g, struct char_data *ch, struct obj_data *obj, int where)
{
  /*
   * ITEM_WEAR_TAKE is used for objects that do not require special bits
   * to be put into that position (e.g. you can hold any object, not just
   * an object with a HOLD bit.)
   */

  int wear_bitvectors[] = {
      ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
      ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS,
      ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
      ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,
      ITEM_WEAR_TAKE, ITEM_WEAR_TAKE, ITEM_WEAR_PACK, ITEM_WEAR_EAR,
      ITEM_WEAR_EAR, ITEM_WEAR_SH, ITEM_WEAR_EYE};

  const char *already_wearing[] = {
      "You're already using a light.\r\n",
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
      "You're already wearing something on both of your ring fingers.\r\n",
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
      "You can't wear anything else around your neck.\r\n",
      "You're already wearing something on your body.\r\n",
      "You're already wearing something on your head.\r\n",
      "You're already wearing something on your legs.\r\n",
      "You're already wearing something on your feet.\r\n",
      "You're already wearing something on your hands.\r\n",
      "You're already wearing something on your arms.\r\n",
      "You're already using a shield.\r\n",
      "You're already wearing something about your body.\r\n",
      "You already have something around your waist.\r\n",
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
      "You're already wearing something around both of your wrists.\r\n",
      "You're already wielding a weapon.\r\n",
      "You're already holding something.\r\n",
      "You're already wearing something on your back.\r\n",
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
      "You're already wearing something in both ears.\r\n",
      "You're already wearing something on your shoulders.\r\n",
      "You're already wearing something as a scouter.\r\n"};

  /* first, make sure that the wear position is valid. */
  if (!obj_wear_flagged(obj, wear_bitvectors[where])) {
    act(g, "You can't wear $p there.", false, ch, obj, nullptr, TO_CHAR);
    return;
  }
  /* do they even have that wear slot by race? */
  if (!char_body_flagged(ch, where)) {
    send_to_char(g, ch, "Seems like your body type doesn't really allow that.\r\n");
    return;
  }
  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R) ||
      (where == WEAR_EAR_R) || (where == WEAR_WIELD1))
    if (char_get_equipment(ch, where))
      where++;

  /* checks for 2H sanity */
  if ((obj_extra_flagged(obj, ITEM_2H) || (wield_type(get_size(ch), obj) == WIELD_TWOHAND)) && hands(ch) > 0) {
    send_to_char(g, ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }
  if (where == WEAR_WIELD2 && char_plr_flagged(ch, PLR_THANDW)) {
    send_to_char(g, ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }

  if (((where == WEAR_WIELD1) ||
       (where == WEAR_WIELD2)) &&
      (hands(ch) > 1)) {
    send_to_char(g, ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }

  if (char_get_equipment(ch, where)) {
    send_to_char(g, ch, "%s", already_wearing[where]);
    return;
  }

  /* See if a trigger disallows it */
  if (!wear_otrigger(g, obj, ch, where) || (obj_get_carried_by(obj) != ch))
    return;

  if (obj_get_type_flag(obj) == ITEM_WEAPON && obj_extra_flagged(obj, ITEM_CUSTOM)) {
    if (char_get_level(ch) < 20) {
      send_to_char(g, ch, "You are not experienced enough to hold that.\r\n");
      return;
    }
  }

  /*
  if (GET_OBJ_TYPE(obj) != ITEM_LIGHT && GET_OBJ_SIZE(obj) > get_size(ch)) {
   send_to_char(ch, "Seems like it is too big for you.\r\n");
   return;
  }
  if (GET_OBJ_SIZE(obj) < get_size(ch) && GET_OBJ_TYPE(obj) != ITEM_LIGHT) {
   send_to_char(ch, "Seems like it is too small for you.\r\n");
   return;
  }
   */

  wear_message(g, ch, obj, where);
  obj_from_char(g, obj);
  equip_char(g, ch, obj, where);
}

int find_eq_pos(GameWorld *g, struct char_data *ch, struct obj_data *obj, char *arg)
{
  int where = -1;

  const char *keywords[] = {
      "!RESERVED!",
      "finger",
      "!RESERVED!",
      "neck",
      "!RESERVED!",
      "body",
      "head",
      "legs",
      "feet",
      "hands",
      "arms",
      "shield",
      "about",
      "waist",
      "wrist",
      "!RESERVED!",
      "!RESERVED!",
      "!RESERVED!",
      "back",
      "ear",
      "\r!RESERVED!",
      "shoulders",
      "scouter",
      "\n"};

  if (!arg || !*arg) {
    if (obj_wear_flagged(obj, ITEM_WEAR_FINGER) && char_body_flagged(ch, WEAR_FINGER_R)) return WEAR_FINGER_R;
    if (obj_wear_flagged(obj, ITEM_WEAR_NECK) && char_body_flagged(ch, WEAR_NECK_1)) return WEAR_NECK_1;
    if (obj_wear_flagged(obj, ITEM_WEAR_BODY) && char_body_flagged(ch, WEAR_BODY)) return WEAR_BODY;
    if (obj_wear_flagged(obj, ITEM_WEAR_HEAD) && char_body_flagged(ch, WEAR_HEAD)) return WEAR_HEAD;
    if (obj_wear_flagged(obj, ITEM_WEAR_LEGS) && char_body_flagged(ch, WEAR_LEGS)) return WEAR_LEGS;
    if (obj_wear_flagged(obj, ITEM_WEAR_FEET) && char_body_flagged(ch, WEAR_FEET)) return WEAR_FEET;
    if (obj_wear_flagged(obj, ITEM_WEAR_HANDS) && char_body_flagged(ch, WEAR_HANDS)) return WEAR_HANDS;
    if (obj_wear_flagged(obj, ITEM_WEAR_ARMS) && char_body_flagged(ch, WEAR_ARMS)) return WEAR_ARMS;
    if (obj_wear_flagged(obj, ITEM_WEAR_SHIELD) && char_body_flagged(ch, WEAR_WIELD2)) return WEAR_WIELD2;
    if (obj_wear_flagged(obj, ITEM_WEAR_ABOUT) && char_body_flagged(ch, WEAR_ABOUT)) return WEAR_ABOUT;
    if (obj_wear_flagged(obj, ITEM_WEAR_WAIST) && char_body_flagged(ch, WEAR_WAIST)) return WEAR_WAIST;
    if (obj_wear_flagged(obj, ITEM_WEAR_WRIST) && char_body_flagged(ch, WEAR_WRIST_R)) return WEAR_WRIST_R;
    if (obj_wear_flagged(obj, ITEM_WEAR_HOLD) && char_body_flagged(ch, WEAR_WIELD2)) return WEAR_WIELD2;
    if (obj_wear_flagged(obj, ITEM_WEAR_PACK) && char_body_flagged(ch, WEAR_BACKPACK)) return WEAR_BACKPACK;
    if (obj_wear_flagged(obj, ITEM_WEAR_EAR) && char_body_flagged(ch, WEAR_EAR_R)) return WEAR_EAR_R;
    if (obj_wear_flagged(obj, ITEM_WEAR_SH) && char_body_flagged(ch, WEAR_SH)) return WEAR_SH;
    if (obj_wear_flagged(obj, ITEM_WEAR_EYE) && char_body_flagged(ch, WEAR_EYE)) return WEAR_EYE;
  } else if (((where = search_block(arg, keywords, false)) < 0) || (*arg == '!')) {
    send_to_char(g, ch, "'%s'?  What part of your body is THAT?\r\n", arg);
    return -1;
  }
  return (where);
}

ACMD(do_wear)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int where, dotmode, items_worn = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(g, ch, "Wear what?\r\n");
    return;
  }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV)) {
    send_to_char(g, ch, "You can't specify the same body location for more than one item!\r\n");
    return;
  }
  if (dotmode == FIND_ALL) {
    for (obj_data *thing : char_inventory(g, ch)) {
      if (char_can_see_obj(g, ch, thing) && (where = find_eq_pos(g, ch, thing, nullptr)) >= 0) {
        if (char_get_level(ch) < obj_get_level(thing)) {
          act(g, "$p: you are not experienced enough to use that.",
              false, ch, thing, nullptr, TO_CHAR);
          send_to_char(g, ch, "You need to be at least %d level to use it.\r\n", obj_get_level(thing));
        } else if (obj_extra_flagged(thing, ITEM_BROKEN)) {
          act(g, "$p: it seems to be broken.", false, ch, thing, nullptr, TO_CHAR);
        } else if (obj_extra_flagged(thing, ITEM_FORGED)) {
          act(g, "$p: it seems to be fake...", false, ch, thing, nullptr, TO_CHAR);
        } else {
          items_worn++;
          if (!is_proficient_with_armor(ch, obj_get_value(thing, VAL_ARMOR_SKILL)) && obj_get_type_flag(thing) == ITEM_ARMOR)
            send_to_char(g, ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
          perform_wear(g, ch, thing, where);
        }
      }
    }
    if (!items_worn)
      send_to_char(g, ch, "You don't seem to have anything wearable.\r\n");
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg1) {
      send_to_char(g, ch, "Wear all of what?\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch))))
      send_to_char(g, ch, "You don't seem to have any %ss.\r\n", arg1);
    else if (char_get_level(ch) < obj_get_level(obj))
      send_to_char(g, ch, "You are not experienced enough to use that.\r\n");
    else
      for (obj_data *match : obj_list(g, obj)) {
        if (!obj_named_and_visible(g, ch, arg1, match))
          continue;
        obj = match;
        if ((where = find_eq_pos(g, ch, obj, nullptr)) >= 0) {
          if (!is_proficient_with_armor(ch, obj_get_value(obj, VAL_ARMOR_SKILL)) && obj_get_type_flag(obj) == ITEM_ARMOR)
            send_to_char(g, ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
          perform_wear(g, ch, obj, where);
        } else
          act(g, "You can't wear $p.", false, ch, obj, nullptr, TO_CHAR);
      }
  } else {
    if (!(obj = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch))))
      send_to_char(g, ch, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
    else if (obj_extra_flagged(obj, ITEM_BROKEN))
      send_to_char(g, ch, "But it seems to be broken!\r\n");
    else if (obj_extra_flagged(obj, ITEM_FORGED))
      send_to_char(g, ch, "But it seems to be fake!\r\n");
    else if (char_get_level(ch) < obj_get_level(obj))
      send_to_char(g, ch, "You are not experienced enough to use that.\r\n");
    else {
      if ((where = find_eq_pos(g, ch, obj, arg2)) >= 0) {
        if (!is_proficient_with_armor(ch, obj_get_value(obj, VAL_ARMOR_SKILL)) && obj_get_type_flag(obj) == ITEM_ARMOR)
          send_to_char(g, ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
        perform_wear(g, ch, obj, where);
      } else if (!*arg2)
        act(g, "You can't wear $p.", false, ch, obj, nullptr, TO_CHAR);
    }
  }
}

ACMD(do_wield)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(g, ch, "Wield what?\r\n");
  else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch))))
    send_to_char(g, ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else {
    if (!obj_wear_flagged(obj, ITEM_WEAR_WIELD))
      send_to_char(g, ch, "You can't wield that.\r\n");
    else if (obj_get_weight(obj) > max_carry_weight(g, ch))
      send_to_char(g, ch, "It's too heavy for you to use.\r\n");
    else if (obj_extra_flagged(obj, ITEM_BROKEN))
      send_to_char(g, ch, "But it seems to be broken!\r\n");
    else if (obj_extra_flagged(obj, ITEM_FORGED))
      send_to_char(g, ch, "But it seems to be fake!\r\n");
    else if (char_get_level(ch) < obj_get_level(obj))
      send_to_char(g, ch, "You are not experienced enough to use that.\r\n");
    else if (char_plr_flagged(ch, PLR_THANDW))
      send_to_char(g, ch, "You are holding a weapon with two hands right now!\r\n");
    else {
      if (!char_is_npc(ch) && !is_proficient_with_weapon(ch, obj_get_value(obj, VAL_WEAPON_SKILL)) && obj_get_type_flag(obj) == ITEM_ARMOR)
        send_to_char(g, ch, "You have no proficiency with this type of weapon.\r\nYour attack accuracy will be greatly reduced.\r\n");
      perform_wear(g, ch, obj, WEAR_WIELD1);
    }
  }
}

ACMD(do_grab)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(g, ch, "Hold what?\r\n");
  else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch))))
    send_to_char(g, ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else if (char_get_level(ch) < obj_get_level(obj))
    send_to_char(g, ch, "You are not experienced enough to use that.\r\n");
  else if (char_plr_flagged(ch, PLR_THANDW))
    send_to_char(g, ch, "You are wielding a weapon with both hands currently.\r\n");
  else {
    if (obj_get_type_flag(obj) == ITEM_LIGHT) {
      perform_wear(g, ch, obj, WEAR_WIELD2);
      if (obj_get_value(obj, VAL_LIGHT_HOURS) > 0 || obj_get_value(obj, VAL_LIGHT_HOURS) < 0) {
        act(g, "@wYou light $p@w.", true, ch, obj, nullptr, TO_CHAR);
        act(g, "@C$n@w lights $p@w.", true, ch, obj, nullptr, TO_ROOM);
      }
      if (obj_get_value(obj, VAL_LIGHT_HOURS) == 0) {
        act(g, "@wYou try to light $p@w but it is burnt out.", true, ch, obj, nullptr, TO_CHAR);
        act(g, "@C$n@w tries to light $p@w but nothing happens.", true, ch, obj, nullptr, TO_ROOM);
      }
    } else {
      if (!obj_wear_flagged(obj, ITEM_WEAR_HOLD) && obj_get_type_flag(obj) != ITEM_WAND &&
          obj_get_type_flag(obj) != ITEM_STAFF && obj_get_type_flag(obj) != ITEM_SCROLL &&
          obj_get_type_flag(obj) != ITEM_POTION)
        send_to_char(g, ch, "You can't hold that.\r\n");
      else
        perform_wear(g, ch, obj, WEAR_WIELD2);
    }
  }
}

void perform_remove(GameWorld *g, struct char_data *ch, int pos)
{
  struct obj_data *obj;

  int64_t previous = getCurPL(g, ch);

  if (!(obj = char_get_equipment(ch, pos)))
    mud_log("SYSERR: perform_remove: bad pos %d passed.", pos);
  /*  SYSERR_DESC:
     *  This error occurs when perform_remove() is passed a bad 'pos'
     *  (location) to remove an object from.
     */
  else if (obj_extra_flagged(obj, ITEM_NODROP) && char_get_admlevel(ch) < 1)
    act(g, "You can't remove $p, it must be CURSED!", false, ch, obj, nullptr, TO_CHAR);
  else if (char_get_carry_items(ch) >= char_can_carry_n(ch))
    act(g, "$p: your arms are full!", false, ch, obj, nullptr, TO_CHAR);
  else {
    if (!remove_otrigger(g, obj, ch))
      return;

    if (pos == WEAR_WIELD1 && char_plr_flagged(ch, PLR_THANDW)) {
      char_plr_set(ch, PLR_THANDW, false);
    }
    obj_to_char(g, unequip_char(g, ch, pos), ch);
    act(g, "You stop using $p.", false, ch, obj, nullptr, TO_CHAR);
    act(g, "$n stops using $p.", true, ch, obj, nullptr, TO_ROOM);
    if (previous > getCurPL(g, ch)) {
      char drop[MAX_INPUT_LENGTH];
      snprintf(drop, sizeof(drop), "@RYour powerlevel has dropped from removing $p@R! @D[@r-%s@D]\r\n", add_commas(previous - getCurPL(g, ch)));
      act(g, drop, false, ch, obj, nullptr, TO_CHAR);
    }
  }
}

ACMD(do_remove)
{
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  int i, dotmode, found = 0, msg;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "Remove what?\r\n");
    return;
  }
  /* lemme check for a board FIRST */
  auto is_board = [&](obj_data *o) { return obj_get_type_flag(o) == ITEM_BOARD; };
  obj           = find_first(char_inventory(g, ch), is_board);
  if (!obj)
    obj = find_first(room_contents(g, char_get_in_room(ch)), is_board);
  if (obj)
    found = 1;

  if (found) {
    if (!isdigit(*arg) || (!(msg = atoi(arg)))) {
      found = 0;
    } else {
      remove_board_msg(g, obj_get_vnum(g, obj), ch, msg);
    }
  }
  if (!found) {
    dotmode = find_all_dots(arg);

    if (dotmode == FIND_ALL) {
      found = 0;
      for (i = 0; i < NUM_WEARS; i++) {
        if (char_get_equipment(ch, i)) {
          perform_remove(g, ch, i);
          found = 1;
        }
      }
      if (!found) {
        send_to_char(g, ch, "You're not using anything.\r\n");
      }
    } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) {
        send_to_char(g, ch, "Remove all of what?\r\n");
      } else {
        found = 0;
        for (i = 0; i < NUM_WEARS; i++) {
          if (char_get_equipment(ch, i) && char_can_see_obj(g, ch, char_get_equipment(ch, i)) &&
              isname(arg, obj_get_name(char_get_equipment(ch, i)))) {
            perform_remove(g, ch, i);
            found = 1;
          }
        }
        if (!found) {
          send_to_char(g, ch, "You don't seem to be using any %ss.\r\n", arg);
        }
      }
    } else {
      if ((i = get_obj_pos_in_equip_vis(g, ch, arg, nullptr, char_get_equipment_array(ch))) < 0) {
        send_to_char(g, ch, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
      } else {
        perform_remove(g, ch, i);
      }
    }
  }
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
static int64_t max_carry_weight(GameWorld *g, struct char_data *ch)
{
  int64_t abil;
  int total;
  /*  abil = MAX(0, MIN(100, GET_STR(ch)));
  total = 1;
  while (abil > 30) {
    abil -= 10;
    total *= 4;
  }*/

  abil  = (getEffMaxPL(g, ch) / 200) + (char_get_aff_abils_str(ch) * 50);
  total = 1;
  return (total * abil);
}
