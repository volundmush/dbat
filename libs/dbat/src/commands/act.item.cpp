/* ************************************************************************
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <array>
#include <optional>
#include <algorithm>
#include "dbat/name_table.hpp"
#include <climits>
#include <cstdlib>
#include <iterator>
#include <ranges>
#include <span>

#include "dbat/args.hpp"
#include "dbat/auction.hpp"
#include "dbat/cmdargs.hpp"
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

struct char_data *auction_seller(GameWorld *g) { return char_by_id(g, g->ch_selling_uid); }
struct char_data *auction_buyer(GameWorld *g) { return char_by_id(g, g->ch_buying_uid); }
struct obj_data *auction_obj(GameWorld *g) { return obj_by_id(g, g->obj_selling_uid); }
void auction_set_seller(GameWorld *g, struct char_data *ch) { g->ch_selling_uid = ch ? char_get_id(ch) : 0; }
void auction_set_buyer(GameWorld *g, struct char_data *ch) { g->ch_buying_uid = ch ? char_get_id(ch) : 0; }
void auction_set_obj(GameWorld *g, struct obj_data *obj) { g->obj_selling_uid = obj ? obj_get_id(obj) : 0; }

namespace {
  /* The last of ch's things that pick chooses */
  template <class Pick> struct obj_data *last_carried(GameWorld *g, struct char_data *ch, Pick pick)
  {
    struct obj_data *found = nullptr;
    for (obj_data *obj : char_inventory(g, ch))
      if (pick(obj))
        found = obj;
    return found;
  }

  bool in_garden(GameWorld *g, room_vnum room)
  {
    return room_flagged(g, room, room_flag::garden1) || room_flagged(g, room, room_flag::garden2);
  }

  bool is_dragon_ball(GameWorld *g, struct obj_data *obj)
  {
    return obj_get_vnum(g, obj) >= 20 && obj_get_vnum(g, obj) <= 26;
  }

  /* A message to ch and one to the rest of the room, about obj */
  void tell_both(GameWorld *g, struct char_data *ch, struct obj_data *obj, std::string_view to_char, std::string_view to_room)
  {
    act_triple(g, {to_char, {}, to_room, true}, ch, obj, nullptr);
  }
} // namespace

/* refuel */

namespace {
  constexpr obj_vnum FUEL_CANISTER = 17290;

  /* How much fuel a ship holds, by its controls */
  int fuel_capacity(obj_vnum controls)
  {
    if (controls >= 44000 && controls <= 44199)
      return 300;
    if (controls >= 44200 && controls <= 44499)
      return 500;
    if (controls >= 44200 && controls <= 44999)
      return 1000;
    return 0;
  }
} // namespace

ACMD(do_refuel)
{
  struct obj_data *controls = find_control(g, ch);
  if (!controls) {
    send_to_char(g, ch, "@wYou need to be in the cockpit to place a new fuel canister into the ship.\r\n");
    return;
  }
  struct obj_data *fuel = last_carried(g, ch, [&](obj_data *obj) { return obj_get_vnum(g, obj) == FUEL_CANISTER; });
  if (!fuel) {
    send_to_char(g, ch, "You do not have any fuel canisters on you.\r\n");
    return;
  }
  int max = fuel_capacity(obj_get_vnum(g, controls));
  if (obj_get_value(controls, 2) == max) {
    send_to_char(g, ch, "The ship is full on fuel!\r\n");
    return;
  }
  obj_set_value(controls, 2, std::min<int64_t>(obj_get_value(controls, 2) + obj_get_weight(fuel) * 4, max));
  extract_obj(g, fuel);
  send_to_char(g, ch, "You place the fuel canister into the ship. Within seconds the fuel has been extracted from the canister into the ships' internal tanks.\r\n");
}

/* garden */

/* The plants that can be harvested: what they yield, and how the yield is adjusted. */
struct harvest {
  obj_vnum plant, fruit;
  int bonus;  /* added to the yield */
  bool kills; /* harvesting kills the plant */
};
static const auto harvests = std::to_array<harvest>({
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
});

static const struct harvest *find_harvest(GameWorld *g, struct obj_data *plant)
{
  auto h = std::ranges::find_if(harvests, [&](const harvest &row) { return row.plant == obj_get_vnum(g, plant); });
  return h == std::end(harvests) ? nullptr : &*h;
}

static bool can_harvest(GameWorld *g, struct obj_data *plant)
{
  return find_harvest(g, plant) != nullptr;
}

namespace {
  /* How much more a plant yields, by a measure of at least so much: the soil's, or the gardener's */
  struct bounty {
    int at_least, more;
    std::string_view says{};
  };

  constexpr auto SOIL_BOUNTY = std::to_array<bounty>({
      {8, 10, "@GThe soil seems to have made the plant exteremely bountiful"   },
      {5, 6,  "@GThe soil seems to have made the plant very bountiful"         },
      {3, 4,  "@GThe soil seems to have made the plant bountiful"              },
      {1, 2,  "@GThe soil seems to have made the plant a bit more bountiful"   },
  });

  constexpr auto SKILL_BOUNTY = std::to_array<bounty>({
      {100,     10, " and your outstanding skill has helped the plant be more bountiful yet!@n\r\n"                          },
      {90,      8,  " and your great skill has also helped the plant be more bountiful yet!@n\r\n"                           },
      {80,      5,  " and your good skill has also helped the plant be more bountiful yet!@n\r\n"                            },
      {50,      3,  " and your decent skill has also helped the plant be more bountiful yet!@n\r\n"                          },
      {40,      2,  " and your mastery of the basics of gardening has also helped the plant be more bountiful yet!@n\r\n"    },
      {30,      1,  " and you somehow managed to make the plant slightly more bountiful with what little you know!@n\r\n"   },
      {INT_MIN, 0,  ".@n\r\n"                                                                                              },
  });

  const bounty *bounty_of(std::span<const bounty> table, int measure)
  {
    auto b = std::ranges::find_if(table, [&](const bounty &row) { return measure >= row.at_least; });
    return b == table.end() ? nullptr : &*b;
  }
} // namespace

static void harvest_plant(GameWorld *g, struct char_data *ch, struct obj_data *plant)
{
  int reward = rand_number(g, 5, 15);

  if (const bounty *soil = bounty_of(SOIL_BOUNTY, obj_get_value(plant, VAL_SOILQ))) {
    reward += soil->more;
    send_to_char(g, ch, soil->says);
  }
  const bounty &skill = *bounty_of(SKILL_BOUNTY, char_get_skill(ch, SKILL_GARDENING));
  reward += skill.more;
  send_to_char(g, ch, skill.says);

  const struct harvest *h = find_harvest(g, plant);
  if (!h) {
    send_to_imm(g, "ERROR: Harvest plant called for illegitimate plant, VNUM {:d}.", obj_get_vnum(g, plant));
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

  struct obj_data *fruit = nullptr;
  for (int count = 0; count < reward; count++) {
    if (!(fruit = read_object(g, h->fruit))) {
      send_to_imm(g, "ERROR: Harvest plant {:d} yields missing object {:d}.", h->plant, h->fruit);
      return;
    }
    obj_to_char(g, fruit, ch);
  }
  send_to_char(g, ch, "@YYou harvest @D[@G{:d}@D]@Y @g{}@Y!@n\r\n", reward, obj_get_short_description(fruit));

  if (h->kills) {
    send_to_char(g, ch, "@wThe harvesting process has killed the plant. Do not worry, this is normal for that type.@n\r\n");
    extract_obj(g, plant);
  } else {
    obj_set_value(plant, VAL_MATURITY, 3);
    obj_set_value(plant, VAL_GROWTH, 0);
  }
}

namespace {
  constexpr obj_vnum GROW_WATER = 251, CLIPPERS = 253, SHOVEL = 254, SOIL = 255;

  /* The last carried of `vnum` that isn't forged, nor broken if it has to work. */
  struct obj_data *carried(GameWorld *g, struct char_data *ch, obj_vnum vnum, bool working)
  {
    return last_carried(g, ch, [&](obj_data *obj) {
      return obj_get_vnum(g, obj) == vnum && !(working && obj_extra_flagged(obj, extra_flag::broken)) && !obj_extra_flagged(obj, extra_flag::forged);
    });
  }

  /* False when ch has no gardening and no room to learn it. */
  bool knows_gardening(GameWorld *g, struct char_data *ch)
  {
    if (char_get_skill(ch, SKILL_GARDENING))
      return true;
    if (slot_count(ch) + 1 > char_get_skill_slots(ch)) {
      send_to_char(g, ch, "You need additional skill slots to pick up the skill linked with this.\r\n");
      return false;
    }
    char_set_skills_base(ch, SKILL_GARDENING, rand_number(g, 8, 16));
    send_to_char(g, ch, "@GYou learn the very basics of gardening.\r\n");
    return true;
  }

  void garden_syntax(GameWorld *g, struct char_data *ch)
  {
    send_to_char(g, ch, "Syntax: garden (plant) ( water | harvest | dig | plant | pick )\r\n");
    send_to_char(g, ch, "Syntax: garden collect [Will collect soil from a room with soil.\r\n");
  }

  struct soil_grade {
    std::optional<room_flag> flag; /* the room's, or none for any */
    std::string_view kind{};
    int lo, hi; /* its quality */
  };

  constexpr auto SOIL_GRADES = std::to_array<soil_grade>({
      {room_flag::fertile1, "fertile soil", 8, 8},
      {room_flag::fertile2, "good soil",    5, 7},
      {std::nullopt,  "soil",         0, 4},
  });

  void collect_soil(GameWorld *g, struct char_data *ch)
  {
    if (!carried(g, ch, SHOVEL, true)) {
      send_to_char(g, ch, "You need a shovel in order to collect soil.\r\n");
      return;
    }
    int sect = room_sect(g, char_get_in_room(ch));
    if (sect != SECT_FOREST && sect != SECT_FIELD && sect != SECT_MOUNTAIN && sect != SECT_HILLS) {
      send_to_char(g, ch, "You can not collect soil from this area.\r\n");
      return;
    }
    const soil_grade &grade = *std::ranges::find_if(SOIL_GRADES, [&](const soil_grade &s) { return !s.flag || room_flagged(g, char_get_in_room(ch), *s.flag); });
    struct obj_data *soil   = read_object(g, SOIL);
    obj_to_char(g, soil, ch);
    act(g, str_format("@yYou sink your shovel into the soft ground and manage to dig up a pile of {}!@n", grade.kind), true, ch, nullptr, nullptr, act_to::actor);
    act(g, str_format("@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of {}!@n", grade.kind), true, ch, nullptr, nullptr, act_to::room);
    obj_set_value(soil, 0, grade.lo == grade.hi ? grade.lo : rand_number(g, grade.lo, grade.hi));
    char_set_wait(ch, PULSE_4SEC);
  }

  /* Every bit of gardening done ends so. */
  void worked(GameWorld *g, struct char_data *ch)
  {
    char_set_wait(ch, PULSE_3SEC);
    improve_skill(g, ch, SKILL_GARDENING, 0);
  }

  /* One way of tending a plant: what to do with it, the stamina it costs and the gardener's skill. */
  struct tending {
    GameWorld *g;
    struct char_data *ch;
    struct obj_data *plant;
    int64_t cost;
    int skill;

    void tire() { decCurST(g, ch, cost, 0); }
    void tell(std::string_view to_char, std::string_view to_room) { tell_both(g, ch, plant, to_char, to_room); }
  };

  /* quirk: a careless watering only says the plant is full above 500, a careful one at 500 too */
  void add_water(tending &t, int amount, int full_at)
  {
    obj_set_value(t.plant, VAL_WATERLEVEL, obj_get_value(t.plant, VAL_WATERLEVEL) + amount);
    if (obj_get_value(t.plant, VAL_WATERLEVEL) >= full_at) {
      obj_set_value(t.plant, VAL_WATERLEVEL, 500);
      send_to_char(t.g, t.ch, "@YThe plant is now at full water level.@n\r\n");
    }
  }

  void water(tending &t)
  {
    struct obj_data *water = carried(t.g, t.ch, GROW_WATER, false);
    if (!water) {
      send_to_char(t.g, t.ch, "You do not have any grow water!\r\n");
      return;
    }
    if (obj_get_value(t.plant, VAL_WATERLEVEL) >= 500) {
      send_to_char(t.g, t.ch, "You stop as you realize that the plant already has enough water.\r\n");
      return;
    }
    if (obj_get_value(t.plant, VAL_WATERLEVEL) <= -10) {
      send_to_char(t.g, t.ch, "The plant is dead!\r\n");
      return;
    }
    if (t.skill < axion_dice(t.g, 0)) {
      t.tell("@GAs you go to water @g$p@G you end up sloppily wasting about half of it on the ground.@n", "@g$n@G takes a bottle of grow water and sloshes some of it on @g$p@G.@n");
      t.tire();
      add_water(t, 40, 501);
    } else {
      t.tell("@GYou calmly and expertly pour the grow water on @g$p@G.@n", "@g$n@G calmly and expertly pours some grow water on @g$p@G.@n");
      t.tire();
      add_water(t, 225, 500);
    }
    extract_obj(t.g, water);
    worked(t.g, t.ch);
  }

  void harvest(tending &t)
  {
    struct obj_data *clippers = carried(t.g, t.ch, CLIPPERS, true);
    if (!clippers) {
      send_to_char(t.g, t.ch, "You do not have any working gardening clippers!\r\n");
      return;
    }
    if (!can_harvest(t.g, t.plant)) {
      send_to_char(t.g, t.ch, "You can not harvest that plant. Instead, Syntax: garden (plant) (pick)\r\n");
      return;
    }
    if (obj_get_value(t.plant, VAL_WATERLEVEL) <= -10) {
      send_to_char(t.g, t.ch, "That plant is dead!\r\n");
      return;
    }
    if (obj_get_value(t.plant, VAL_MATURITY) < obj_get_value(t.plant, VAL_MAXMATURE)) {
      send_to_char(t.g, t.ch, "You stop as you realize that the plant isn't mature enough to harvest.\r\n");
      return;
    }
    if (t.skill < axion_dice(t.g, -5)) {
      t.tell("@GAs you go to harvest @g$p@G you end up cutting it in half instead!@n", "@g$n@G attempts to harvest @g$p@G with $s clippers, but accidently cuts the plant in half!@n");
      t.tire();
      extract_obj(t.g, t.plant);
      worked(t.g, t.ch);
      return;
    }
    t.tell("@GYou calmly and expertly harvest @g$p@G.@n", "@g$n@G calmly and expertly harvests @g$p@G.@n");
    t.tire();
    obj_set_value(clippers, VAL_ALL_HEALTH, obj_get_value(clippers, VAL_ALL_HEALTH) - 1);
    if (obj_get_value(clippers, VAL_ALL_HEALTH) <= 0) {
      /* quirk: the clippers dull before the harvest, which is lost */
      send_to_char(t.g, t.ch, "The clippers are now too dull to use.\r\n");
      return;
    }
    harvest_plant(t.g, t.ch, t.plant);
    worked(t.g, t.ch);
  }

  bool has_shovel(tending &t)
  {
    if (carried(t.g, t.ch, SHOVEL, true))
      return true;
    send_to_char(t.g, t.ch, "You do not have any working gardening shovels!\r\n");
    return false;
  }

  void dig(tending &t)
  {
    if (!has_shovel(t))
      return;
    t.tell("@GYou calmly dig up @g$p@G.@n", "@g$n@G calmly digs up @g$p@G.@n");
    t.tire();
    obj_from_room(t.g, t.plant);
    obj_to_char(t.g, t.plant, t.ch);
    obj_set_value(t.plant, VAL_SOILQ, 0);
    worked(t.g, t.ch);
  }

  /* How much sooner a plant matures, by the soil's quality from 1; any other is as 8. */
  constexpr auto SOIL_HASTE = std::to_array<int>({10, 15, 20, 25, 50, 60, 70});

  bool planters_full(GameWorld *g, struct char_data *ch)
  {
    return (check_saveroom_count(g, ch, nullptr) > 7 && room_flagged(g, char_get_in_room(ch), room_flag::garden1)) ||
           (check_saveroom_count(g, ch, nullptr) > 19 && room_flagged(g, char_get_in_room(ch), room_flag::garden2));
  }

  void plant(tending &t)
  {
    if (!has_shovel(t))
      return;
    struct obj_data *soil = carried(t.g, t.ch, SOIL, false);
    if (!soil) {
      send_to_char(t.g, t.ch, "You don't have any real soil.\r\n");
      return;
    }
    if (planters_full(t.g, t.ch)) {
      send_to_char(t.g, t.ch, "This room already has all its planters full. Try digging up some plants.\r\n");
      return;
    }
    if (t.skill < axion_dice(t.g, -5)) {
      t.tell("@GYou end up digging a hole too shallow to hold @g$p@G. Better try again.@n", "@g$n@G digs a very shallow hole in one of the planters and then realizes @g$p@G won't fit in it.@n");
      t.tire();
      worked(t.g, t.ch);
      return;
    }
    t.tell("@GYou dig a proper sized hole and plant @g$p@G in it.@n", "@g$n@G digs a proper sized hole in a planter and plants @g$p@G in it.@n");
    obj_from_char(t.g, t.plant);
    obj_to_room(t.g, t.plant, char_get_in_room(t.ch));
    t.tire();
    obj_set_value(t.plant, VAL_MAXMATURE, 6);
    obj_set_value(t.plant, VAL_SOILQ, obj_get_value(soil, 0));
    int quality = obj_get_value(t.plant, VAL_SOILQ);
    /* quirk: soil of quality 0 does as well as the best */
    obj_set_value(t.plant, VAL_MATGOAL, 200 - (quality >= 1 && quality <= 7 ? SOIL_HASTE[quality - 1] : 80));
    extract_obj(t.g, soil);
    worked(t.g, t.ch);
  }

  void pick(tending &t)
  {
    if (!obj_extra_flagged(t.plant, extra_flag::mature)) {
      send_to_char(t.g, t.ch, "You can't pick that type of plant. Syntax: garden (plant) harvest\r\n");
      return;
    }
    if (obj_get_value(t.plant, VAL_MATURITY) < obj_get_value(t.plant, VAL_MAXMATURE)) {
      send_to_char(t.g, t.ch, "That plant is not mature enough yet.\r\n");
      return;
    }
    if (t.skill < axion_dice(t.g, -5)) {
      /* quirk: shredding it costs nothing, and leaves it be */
      t.tell("@GYou end up shredding @g$p@G with your clumsy unskilled hands.@n", "@g$n@G grabs a hold of @g$p@G and shreds it in an attempt to pick it.@n");
      return;
    }
    t.tell("@GYou grab a hold of @g$p@G and carefully pick it out of the soil.@n", "@g$n@G grabs a hold of @g$p@G and carefully picks it out of the soil.@n");
    obj_from_room(t.g, t.plant);
    obj_to_char(t.g, t.plant, t.ch);
    t.tire();
    worked(t.g, t.ch);
  }

  struct garden_action {
    std::string_view word{};
    void (*tend)(tending &t);
  };

  constexpr auto GARDEN_ACTIONS = std::to_array<garden_action>({
      {"water",   water  },
      {"harvest", harvest},
      {"dig",     dig    },
      {"plant",   plant  },
      {"pick",    pick   },
  });
} // namespace

ACMD(do_garden)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (!knows_gardening(g, ch))
    return;

  if (str_eq_nocase(arg, "collect")) {
    collect_soil(g, ch);
    return;
  }
  if (arg.empty() || arg2.empty()) {
    garden_syntax(g, ch);
    return;
  }
  if (!in_garden(g, char_get_in_room(ch))) {
    send_to_char(g, ch, "You are not even in a garden!\r\n");
    return;
  }

  /* A plant to plant is carried; any other is in the room. */
  struct obj_data *obj;
  if (str_eq_nocase(arg2, "plant")) {
    if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "What are you trying to plant?\r\n");
      send_to_char(g, ch, "Syntax: garden (plant in inventory) plant\r\n");
      return;
    }
  } else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch))))) {
    send_to_char(g, ch, "That plant doesn't seem to be here.\r\n");
    return;
  }

  tending t{g, ch, obj, static_cast<int64_t>((getMaxST(g, ch) * 0.005) + rand_number(g, 50, 150)), char_get_skill(ch, SKILL_GARDENING)};
  if (getCurST(g, ch) < t.cost) {
    send_to_char(g, ch, "@WYou need at least @G{}@W stamina to garden.\r\n", add_commas(t.cost));
    return;
  }

  auto action = std::ranges::find_if(GARDEN_ACTIONS, [&](const garden_action &a) { return str_eq_nocase(arg2, a.word); });
  if (action == std::end(GARDEN_ACTIONS)) {
    garden_syntax(g, ch);
    return;
  }
  action->tend(t);
}

/* pack, and the counting of what a house holds */

namespace {
  /* The key to a house: the house before it, but for the first */
  obj_vnum house_key(GameWorld *g, struct obj_data *house)
  {
    return obj_get_vnum(g, house) == 18802 ? 18800 : obj_get_vnum(g, house) - 1;
  }

  bool has_housekey(GameWorld *g, struct char_data *ch, struct obj_data *house)
  {
    /* quirk: a copied house is no one's */
    if (obj_extra_flagged(house, extra_flag::duplicate))
      return false;
    return find_first(char_inventory(g, ch), [&](obj_data *key) { return obj_get_vnum(g, key) == house_key(g, house); }) != nullptr;
  }

  /* What houses sell for, by their vnums; each has four rooms, from its key's vnum */
  struct house_kind {
    obj_vnum lo, hi;
    int price;
  };

  constexpr auto HOUSES = std::to_array<house_kind>({
      {18800, 18899, 65000  },
      {18900, 18999, 150000 },
      {19100, 19199, 1000000},
  });

  void pack_furniture(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    room_vnum here = char_get_in_room(ch);
    tell_both(g, ch, obj, "@CYou push a hidden button on $p@C and a cloud of smoke erupts and covers it. As the smoke clears a small capsule can be seen on the ground.@n",
              "@c$n@C pushes a hidden button on $p@C and a cloud of smokes erupts and covers it. As the smoke clears a small capsule can be seen on the ground.@n");
    if (obj_get_vnum(g, obj) == 11) {
      extract_obj(g, obj);
      obj_to_room(g, read_object(g, 19085), here);
      return;
    }
    struct obj_data *packed = read_object(g, obj_get_vnum(g, obj) - 10);
    extract_obj(g, obj);
    obj_to_room(g, packed, here);
  }

  void sell_house(GameWorld *g, struct char_data *ch, struct obj_data *house)
  {
    tell_both(g, ch, house, "@CYou push a hidden button on $p@C and a cloud of smoke erupts and covers it. As the smoke clears a pile of money can be seen on the ground!@n",
              "@c$n@C pushes a hidden button on $p@C and a cloud of smokes erupts and covers it. As the smoke clears a pile of money can be seen on the ground!@n");
    obj_vnum vnum = obj_get_vnum(g, house), key = house_key(g, house);
    auto kind     = std::ranges::find_if(HOUSES, [&](const house_kind &k) { return vnum >= k.lo && vnum <= k.hi; });
    int money     = 0;
    /* quirk: a house of 19000 to 19099 sells for nothing, and keeps what it holds */
    if (kind != std::end(HOUSES)) {
      money = kind->price;
      for (room_vnum room = key; room < key + 4; room++)
        while (room_get_contents(g, room))
          extract_obj(g, first_obj(g, room_get_contents(g, room)));
    }
    for (obj_data *held : char_inventory(g, ch))
      if (obj_get_vnum(g, held) == key)
        extract_obj(g, held);
    obj_to_room(g, create_money(g, money), char_get_in_room(ch));
    extract_obj(g, house);
  }
} // namespace

ACMD(do_pack)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Pack up which type of house capsule?\nSyntax: pack (target)\r\n");
    return;
  }
  auto found = find_in_room(g, ch, arg, "That house item doesn't seem to be around.\r\n");
  if (reject(g, ch, found))
    return;
  struct obj_data *obj = *found;
  obj_vnum vnum        = obj_get_vnum(g, obj);

  if ((vnum >= 19090 && vnum <= 19099) || vnum == 11) {
    pack_furniture(g, ch, obj);
    return;
  }
  if (vnum < 18800 || vnum > 19199 || obj_get_type_flag(obj) != ITEM_VEHICLE) {
    send_to_char(g, ch, "That isn't something you can pack up!\r\n");
    return;
  }
  if (!str_eq_nocase(arg2, "yes")) {
    send_to_char(g, ch, "This will sell off your house and delete everything inside. Are you sure? If you are then enter the command again with a yes at the end.\nSyntax: pack (house) yes\r\n");
    return;
  }
  if (!has_housekey(g, ch, obj)) {
    send_to_char(g, ch, "You do not own this house.\r\n");
    return;
  }
  sell_house(g, ch, obj);
}

int check_insidebag(GameWorld *g, struct obj_data *cont, double mult)
{
  int count = 0, containers = 0;

  for (obj_data *inside : obj_contents(g, cont)) {
    count++;
    if (obj_get_type_flag(inside) == ITEM_CONTAINER) {
      count += check_insidebag(g, inside, mult);
      containers++;
    }
  }
  count = count * mult;
  return count + containers;
}

int check_saveroom_count(GameWorld *g, struct char_data *ch, struct obj_data *cont)
{
  room_vnum room = char_get_in_room(ch);
  if (room == NOWHERE || !room_flagged(g, room, room_flag::house))
    return 0;

  int count = 0;
  for (obj_data *obj : room_contents(g, room)) {
    count++;
    if (!obj_extra_flagged(obj, extra_flag::cardcase))
      count += check_insidebag(g, obj, 0.5);
  }
  if (cont) {
    if (!obj_extra_flagged(cont, extra_flag::cardcase))
      count += check_insidebag(g, cont, 0.5);
    count++;
  }
  return count;
}

/* deploy */

namespace {
  constexpr obj_vnum HOUSE_DOOR = 18801, HOUSE_FURNISHING = 18803;

  /* What each furniture capsule unpacks into */
  struct unpacking {
    obj_vnum capsule, furniture;
  };

  constexpr auto FURNITURE = std::to_array<unpacking>({
      {19080, 19090},
      {19081, 19091},
      {19082, 19092},
      {19083, 19093},
      {19085, 11   },
  });

  /* Where each house capsule's houses stand, and how many rooms apart */
  struct house_site {
    obj_vnum capsule;
    room_vnum first;
    int apart;
  };

  constexpr auto HOUSE_SITES = std::to_array<house_site>({
      {4, 18800, 4},
      {5, 18900, 5},
      {6, 19100, 5},
  });

  bool is_house_capsule(GameWorld *g, struct obj_data *obj)
  {
    return std::ranges::any_of(HOUSE_SITES, [&](const house_site &s) { return obj_get_vnum(g, obj) == s.capsule; });
  }

  void deploy_furniture(GameWorld *g, struct char_data *ch, struct obj_data *capsule)
  {
    auto unpacked = std::ranges::find_if(FURNITURE, [&](const unpacking &u) { return u.capsule == obj_get_vnum(g, capsule); });
    if (unpacked == std::end(FURNITURE)) {
      send_to_imm(g, "ERROR: Furniture failed to deploy at {:d}.", char_get_in_room(ch));
      return;
    }
    struct obj_data *furniture = read_object(g, unpacked->furniture);
    tell_both(g, ch, furniture, "@CYou click the capsule's button and toss it to the floor. A puff of smoke erupts immediately and quickly dissipates to reveal, $p@C.@n",
              "@c$n@C clicks a capsule's button and tosses it to the floor. A puff of smoke erupts immediately and quickly dissipates to reveal, $p@C.@n");
    obj_to_room(g, furniture, char_get_in_room(ch));
    extract_obj(g, capsule);
  }

  void deploy_house(GameWorld *g, struct char_data *ch, struct obj_data *capsule)
  {
    const house_site &site = *std::ranges::find_if(HOUSE_SITES, [&](const house_site &s) { return obj_get_vnum(g, capsule) == s.capsule; });
    auto is_door           = [&](obj_data *obj) { return obj_get_vnum(g, obj) == HOUSE_DOOR; };

    for (room_vnum room = site.first; room < site.first + 99; room += site.apart) {
      if (find_first(room_contents(g, room), is_door))
        continue;
      struct obj_data *door = read_object(g, HOUSE_DOOR);
      obj_set_value(door, 6, char_get_in_room(ch));
      obj_set_value(door, 0, room != 18800 ? room + 1 : 18802);
      obj_set_value(door, 2, room);
      obj_to_room(g, door, room);
      obj_to_char(g, read_object(g, room), ch); /* its key */
      act_triple(g, {"@WYou click the capsule and toss it to the ground. A large cloud of smoke erupts from the capsule and after it clears a house is visible in its place!@n", {},
                     "@C$n@W clicks a capsule and then tosses it to the ground. A large cloud of smoke erupts from the capsule and after it clears a house is visible in its place!@n", true},
                 ch, nullptr, nullptr);
      obj_to_room(g, read_object(g, HOUSE_FURNISHING), room + 1);
      extract_obj(g, capsule);
      return;
    }
    send_to_char(g, ch, "@ROOC@D: @wSorry for the inconvenience, but it appears there are no houses available. Please contact Iovan.@n\r\n");
  }
} // namespace

ACMD(do_deploy)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  room_vnum here        = char_get_in_room(ch);

  if (arg.empty()) {
    struct obj_data *capsule = last_carried(g, ch, [&](obj_data *obj) { return is_house_capsule(g, obj); });
    if (!capsule) {
      send_to_char(g, ch, "You do not have any house type capsules to deploy.@n\r\n");
      return;
    }
    if (char_get_rp(ch) < 10) {
      send_to_char(g, ch, "You are required to have (not spend) 10 RPP in order to place a house.\r\n");
      return;
    }
    int sect = room_sect(g, here);
    if (sect == SECT_INSIDE || sect == SECT_WATER_NOSWIM || sect == SECT_WATER_SWIM || sect == SECT_SPACE) {
      send_to_char(g, ch, "You can not deploy that in this kind of area. Try an area more suitable for a house.\r\n");
      return;
    }
    deploy_house(g, ch, capsule);
    return;
  }

  auto found = find_carried(g, ch, arg, "Syntax: deploy (no argument for houses)\nSyntax: deploy (target) <-- For furniture\r\n");
  if (reject(g, ch, found))
    return;
  if (obj_get_vnum(g, *found) < 19080 || obj_get_vnum(g, *found) > 19099) {
    send_to_char(g, ch, "That is not a furniture capsule!\r\n");
    return;
  }
  if (!room_flagged(g, here, room_flag::house) || room_flagged(g, here, room_flag::ship) || in_garden(g, here)) {
    send_to_char(g, ch, "You can't deploy house furniture capsules here.\r\n");
    return;
  }
  deploy_furniture(g, ch, *found);
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
  if (char_plr_flagged(ch, player_flag::thandw)) {
    send_to_char(g, ch, "You stop wielding your weapon with both hands.\r\n");
    act(g, "$n stops wielding $s weapon with both hands.", true, ch, nullptr, nullptr, act_to::room);
    char_plr_set(ch, player_flag::thandw, false);
    return;
  }
  if (!char_get_equipment(ch, WEAR_WIELD1)) {
    send_to_char(g, ch, "You need to wield a sword to use this.\r\n");
    return;
  }
  if (char_get_equipment(ch, WEAR_WIELD2)) {
    send_to_char(g, ch, "You have something in your offhand already and can't two hand wield your main weapon.\r\n");
    return;
  }
  if (char_get_limb_condition(ch, 0) <= 0 || char_get_limb_condition(ch, 1) <= 0) {
    send_to_char(g, ch, "Kind of hard with only one arm...\r\n");
    return;
  }
  send_to_char(g, ch, "You grab your weapon with both hands.\r\n");
  act(g, "$n starts wielding $s weapon with both hands.", true, ch, nullptr, nullptr, act_to::room);
  char_plr_set(ch, player_flag::thandw, true);
}

/* The auction, for which there's no command any more */

namespace {
  static constexpr std::array<std::string_view, AUC_BID + 1> auctioneer = {{
      "@D[@CAUCTION@c: @C$n@W puts $p@W up for sale at @Y{:d}@W zenni.@D]@n",
      "@D[@CAUCTION@c: @W$p@W at @Y{:d}@W zenni going once!@D]@n",
      "@D[@CAUCTION@c: @W$p@W at @Y{:d}@W zenni going twice!@D]@n",
      "@D[@CAUCTION@c: @WLast call: $p@W going for @Y{:d}@W zenni.@D]@n",
      "@D[@CAUCTION@c: @WUnfortunately $p@W is unsold, returning it to $n. @D]@n",
      "@D[@CAUCTION@c: @WSOLD! $p@W to @C$n@W for @Y{:d}@W zenni!@D]@n",
      "@D[@CAUCTION@c: @WSorry, @C$n@W has cancelled the auction.@D]@n",
      "@D[@CAUCTION@c: @WSorry, @C$n@W has left us, the auction can't go on.@D]@n",
      "@D[@CAUCTION@c: @WSorry, $p@W has been confiscated, shame on you $n.@D]@n",
      "@D[@CAUCTION@c: @C$n@W is selling $p@W for @Y{:d}@W zenni.@D]@n",
      "@D[@CAUCTION@c: @C$n@W bids @Y{:d}@W zenni on $p@W.@D]@n"}};

  /* auctioneer[i], with the bid where it has {:d}, capitalised if asked */
  std::string auction_message(int i, int bid, bool capitalised = false)
  {
    std::string message = std::vformat(auctioneer[i], std::make_format_args(bid));
    if (capitalised)
      CAP(message);
    return message;
  }

  /* To everyone playing but in the time chamber or the past, as the buyer or the seller says it */
  void auc_send_to_all(GameWorld *g, const std::string &messg, bool buyer)
  {
    for (descriptor_data *i : each_desc(g)) {
      if (desc_get_connected(i) != CON_PLAYING || room_flagged(g, char_get_in_room(i->character), room_flag::hbtc) ||
          room_flagged(g, char_get_in_room(i->character), room_flag::past))
        continue;
      act(g, messg, true, buyer ? auction_buyer(g) : auction_seller(g), auction_obj(g), i->character, act_to::vict | act_option::even_asleep);
    }
  }

  /* No one is selling, buying or bidding any more */
  void auction_over(GameWorld *g)
  {
    auction_set_seller(g, nullptr);
    auction_set_buyer(g, nullptr);
    g->curbid  = 0;
    g->aucstat = AUC_NULL_STATE;
  }

  /* ... as the thing has gone */
  void auction_lost(GameWorld *g)
  {
    auc_send_to_all(g, "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n", false);
    auction_over(g);
  }

  /* The thing on auction, back to ch */
  void auction_return(GameWorld *g, struct char_data *ch)
  {
    std::string buf = str_format("{} flies out the sky and into your hands.\r\n", obj_get_short_description(auction_obj(g)));
    CAP(buf);
    send_to_char(g, ch, buf);
    obj_to_char(g, auction_obj(g), ch);
  }

  void start_auction(GameWorld *g, struct char_data *ch, struct obj_data *obj, int bid)
  {
    obj_from_char(g, obj);
    auction_set_obj(g, obj);
    auction_set_seller(g, ch);
    auction_set_buyer(g, nullptr);
    g->curbid = bid;

    std::string buf = str_format("{} magicly flies away from your hands to be auctioned!\r\n", obj_get_short_description(auction_obj(g)));
    CAP(buf);
    send_to_char(g, auction_seller(g), buf);
    auc_send_to_all(g, auction_message(AUC_NULL_STATE, g->curbid), false);
    g->aucstat = AUC_OFFERING;
  }

  void auction_sold(GameWorld *g)
  {
    struct char_data *seller = auction_seller(g), *buyer = auction_buyer(g);

    auc_send_to_all(g, auction_message(AUC_SOLD, g->curbid), true);
    obj_to_char(g, auction_obj(g), buyer);
    std::string buf = str_format("{} flies out the sky and into your hands, what a steal!\r\n", obj_get_short_description(auction_obj(g)));
    CAP(buf);
    send_to_char(g, buyer, buf);
    send_to_char(g, seller, "Congrats! You have sold {} for @Y{:d}@W zenni!\r\n", obj_get_short_description(auction_obj(g)), g->curbid);

    if (char_get_gold(seller) + g->curbid > char_gold_carry(seller)) {
      /* quirk: the buyer is told of the seller's deposit */
      send_to_char(g, buyer, "You couldn't hold all the zenni, so some of it was deposited for you.\r\n");
      int diff = (char_get_gold(seller) + g->curbid) - char_gold_carry(seller);
      char_set_gold(seller, char_gold_carry(seller));
      char_set_bank_gold(seller, char_get_bank_gold(seller) + diff);
    } else {
      char_set_gold(seller, char_get_gold(seller) + g->curbid);
    }
    auction_set_obj(g, nullptr);
    auction_over(g);
  }

  void auc_stat(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    if (g->aucstat == AUC_NULL_STATE) {
      send_to_char(g, ch, "Nothing is being auctioned!\r\n");
      return;
    }
    if (ch == auction_seller(g)) {
      send_to_char(g, ch, "You should have found that out BEFORE auctioning it!\r\n");
      return;
    }
    if (char_get_gold(ch) < 500) {
      send_to_char(g, ch, "You can't afford to find the stats on that, it costs 500 zenni!\r\n");
      return;
    }
    act(g, auction_message(AUC_STAT, g->curbid), true, auction_seller(g), obj, ch, act_to::vict | act_option::even_asleep);
    char_set_gold(ch, char_get_gold(ch) - 500);
  }
} // namespace

void check_auction(GameWorld *g)
{
  if (g->aucstat == AUC_NULL_STATE)
    return;
  if (!auction_obj(g)) {
    auction_lost(g);
    return;
  }
  switch (g->aucstat) {
    case AUC_OFFERING:
    case AUC_GOING_ONCE:
    case AUC_GOING_TWICE:
      auc_send_to_all(g, auction_message(g->aucstat, g->curbid, true), false);
      g->aucstat++;
      return;
    case AUC_LAST_CALL:
      if (auction_buyer(g)) {
        auction_sold(g);
        return;
      }
      auc_send_to_all(g, auction_message(AUC_LAST_CALL, g->curbid, true), false);
      auction_return(g, auction_seller(g));
      auction_set_obj(g, nullptr);
      auction_over(g);
      return;
  }
}

void stop_auction(GameWorld *g, int type, struct char_data *ch)
{
  if (!auction_obj(g)) {
    auction_lost(g);
    return;
  }
  if (type != AUC_NORMAL_CANCEL && type != AUC_QUIT_CANCEL && type != AUC_WIZ_CANCEL) {
    send_to_char(g, ch, "Sorry, that is an unrecognised cancel command, please report.");
    return;
  }
  auc_send_to_all(g, auction_message(type, g->curbid), false);
  auction_return(g, type == AUC_WIZ_CANCEL ? ch : auction_seller(g));
  if (auction_buyer(g))
    char_set_gold(auction_buyer(g), char_get_gold(auction_buyer(g)) + g->curbid);
  auction_set_obj(g, nullptr);
  auction_over(g);
}

ACMD(do_auction)
{
  std::string_view rest = argument;
  std::string arg1      = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (room_flagged(g, char_get_in_room(ch), room_flag::hbtc)) {
    send_to_char(g, ch, "This is a different dimension!\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), room_flag::past)) {
    send_to_char(g, ch, "You are in the past!\r\n");
    return;
  }
  if (char_prf_flagged(g, ch, pref_flag::hide)) {
    send_to_char(g, ch, "The auctioneer will not accept items from hidden people.\r\n");
    return;
  }
  if (arg1.empty()) {
    send_to_char(g, ch, "Auction what?\r\n");
    send_to_char(g, ch, "[ Auction: <item> | <cancel> ]\r\n");
    return;
  }
  if (is_abbrev(arg1, "cancel") || is_abbrev(arg1, "stop")) {
    if ((ch != auction_seller(g) && char_get_admlevel(ch) <= ADMLVL_GRGOD) || g->aucstat == AUC_NULL_STATE)
      send_to_char(g, ch, "You're not even selling anything!\r\n");
    else if (ch == auction_seller(g))
      stop_auction(g, AUC_NORMAL_CANCEL, nullptr);
    else
      stop_auction(g, AUC_WIZ_CANCEL, ch);
    return;
  }
  if (is_abbrev(arg1, "stats") || is_abbrev(arg1, "identify")) {
    auc_stat(g, ch, auction_obj(g));
    return;
  }
  struct obj_data *obj = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch));
  if (!obj) {
    send_to_char(g, ch, "You don't seem to have {} {}.\r\n", AN(arg1), arg1);
    return;
  }
  if (arg2.empty()) {
    send_to_char(g, ch, "What should be the minimum bid?\r\n");
    return;
  }
  int bid = parse_int(arg2);
  if (bid <= 0) {
    send_to_char(g, ch, "Come on? One zenni at least?\r\n");
    return;
  }
  if (g->aucstat != AUC_NULL_STATE) {
    /* quirk: tells of the bid asked for, not the one standing */
    send_to_char(g, ch, "Sorry, but {} is already auctioning {} at @Y{:d}@W zenni!\r\n", char_get_name(auction_seller(g)), obj_get_short_description(auction_obj(g)), bid);
    return;
  }
  if (obj_extra_flagged(obj, extra_flag::nosell) || obj_get_value(obj, VAL_CONTAINER_CORPSE) == 1) {
    send_to_char(g, ch, "Sorry but you can't sell that!\r\n");
    return;
  }
  send_to_char(g, ch, "Ok.\r\n");
  start_auction(g, ch, obj, bid);
}

/* bid, at the auction house */

namespace {
  constexpr room_vnum AUCTION_HOUSE = 80;
  constexpr time_t BIDDING_TIME     = 86400;

  /* the player's name capitalised, leaving the player table's lowercase name alone, or "Nobody" */
  std::string name_or_nobody(GameWorld *g, long id)
  {
    std::string text(player_name_or(g, id, "Nobody"));
    return CAP(text);
  }

  bool on_auction(struct obj_data *obj) { return obj_get_aucter(obj) > 0; }

  /* The nth thing on auction, counting from 1 */
  struct obj_data *auction_lot(GameWorld *g, int n)
  {
    int at = 0;
    return find_first(room_contents(g, AUCTION_HOUSE), [&](obj_data *obj) { return on_auction(obj) && ++at == n; });
  }

  void bid_syntax(GameWorld *g, struct char_data *ch, std::string_view why)
  {
    send_to_char(g, ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
    send_to_char(g, ch, why);
  }

  void list_lots(GameWorld *g, struct char_data *ch)
  {
    send_to_char(g, ch, "@Y                                   Auction@n\r\n");
    send_to_char(g, ch, "@c------------------------------------------------------------------------------@n\r\n");
    int at = 0;
    for (obj_data *obj : room_contents(g, AUCTION_HOUSE)) {
      if (!on_auction(obj))
        continue;
      at++;
      std::string owner = name_or_nobody(g, obj_get_aucter(obj));
      std::string item  = printf_width(obj_get_short_description(obj), count_color_chars(obj_get_short_description(obj)) + 30, true);
      bool bid_on       = obj_get_curBidder(obj) > -1;
      std::string state;
      if (obj_get_aucTime(obj) + BIDDING_TIME > game_now(g))
        state = bid_on ? str_format("[@RTop Bid@W: {} @Y{}@D]", name_or_nobody(g, obj_get_curBidder(obj)), add_commas(obj_get_bid(obj)))
                       : str_format("[@GCost@W: @Y{}@D]", add_commas(obj_get_bid(obj)));
      else if (obj_get_aucTime(obj) + BIDDING_TIME < game_now(g) && bid_on)
        state = str_format("[@RBid Winner@W: {} @Y{}@D]", name_or_nobody(g, obj_get_curBidder(obj)), add_commas(obj_get_bid(obj)));
      else
        state = "[@RClosed@D]";
      send_to_char(g, ch, "@D[@R#@W{:>3d}@D][@mOwner@W: @w{:>10}@D][@GItem Name@W: @w{}@D]{}@n\r\n", at, owner, item, state);
    }
    if (!at)
      send_to_char(g, ch, "No items are currently being auctioned.\r\n");
    send_to_char(g, ch, "@c------------------------------------------------------------------------------@n\r\n");
  }

  std::string_view condition_colour(int health)
  {
    if (health >= 100)
      return "@C";
    if (health >= 50)
      return "@y";
    if (health >= 1)
      return "@r";
    return "@D";
  }

  struct weapon_grade {
    extra_flag flag;
    int bonus;
  };

  constexpr auto WEAPON_GRADES = std::to_array<weapon_grade>({
      {extra_flag::weaplvl1, 5 },
      {extra_flag::weaplvl2, 10},
      {extra_flag::weaplvl3, 20},
      {extra_flag::weaplvl4, 30},
      {extra_flag::weaplvl5, 50},
  });

  void appraisal(GameWorld *g, struct char_data *ch, struct obj_data *lot)
  {
    send_to_char(g, ch, "You look at images of the object on your scouter.\r\n");
    act(g, "@c$n@w looks at something on their scouter screen.@n", true, ch, nullptr, nullptr, act_to::room);
    send_to_char(g, ch, "@c------------------------------------------------------------------------\n");
    send_to_char(g, ch, "@GOwner       @W: @w{}@n\n", name_or_nobody(g, obj_get_aucter(lot)));
    send_to_char(g, ch, "@GItem Name   @W: @w{}@n\n", obj_get_short_description(lot));
    send_to_char(g, ch, "@GCurrent Bid @W: @Y{}@n\n", add_commas(obj_get_bid(lot)));
    send_to_char(g, ch, "@GStore Value @W: @Y{}@n\n", add_commas(obj_get_cost(lot)));
    send_to_char(g, ch, "@GItem Min LVL@W: @w{:d}@n\n", obj_get_level(lot));
    send_to_char(g, ch, "@GCondition   @W: {}{:d}%@n\n", condition_colour(obj_get_value(lot, VAL_ALL_HEALTH)), obj_get_value(lot, VAL_ALL_HEALTH));
    send_to_char(g, ch, "@GItem Weight @W: @w{}@n\n", add_commas(obj_get_weight(lot)));
    std::string bits = flag_names(obj_get_wear_flags(lot));
    search_replace(bits, "TAKE", "");
    send_to_char(g, ch, "@GWear Loc.   @W:@w{}\n", bits);
    if (obj_get_type_flag(lot) == ITEM_WEAPON) {
      auto grade = std::ranges::find_if(WEAPON_GRADES, [&](const weapon_grade &w) { return obj_extra_flagged(lot, w.flag); });
      /* quirk: every grade shows as level 1 */
      if (grade != std::end(WEAPON_GRADES))
        send_to_char(g, ch, "@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w{:d}%@D]@n\r\n", grade->bonus);
    }
    send_to_char(g, ch, "@GItem Size   @W:@w {}@n\r\n", size_names[obj_get_size(lot)]);
    send_to_char(g, ch, "@GItem Bonuses@W:@w");
    int found_bonus = false;
    for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
      const auto &aff = obj_get_affected(lot)[i];
      if (!aff.modifier)
        continue;
      send_to_char(g, ch, "{} {:+d} to {}", found_bonus++ ? "," : "", aff.modifier, type_name(apply_types, aff.location));
      if (aff.location == APPLY_FEAT)
        send_to_char(g, ch, " ({})", g->feat_list[aff.specific].name);
      else if (aff.location == APPLY_SKILL)
        send_to_char(g, ch, " ({})", g->spell_info[aff.specific].name);
    }
    send_to_char(g, ch, found_bonus ? "@n" : " None@n");
    send_to_char(g, ch, "\n@GSpecial     @W:@w {}\n", flag_names(obj_get_bitvector(lot)));
    send_to_char(g, ch, "@c------------------------------------------------------------------------\n");
  }

  void appraise(GameWorld *g, struct char_data *ch, struct obj_data *lot)
  {
    if (!char_get_skill(ch, SKILL_APPRAISE)) {
      send_to_char(g, ch, "You are unskilled at appraising.\r\n");
      return;
    }
    improve_skill(g, ch, SKILL_APPRAISE, 1);
    if (char_get_skill(ch, SKILL_APPRAISE) < rand_number(g, 1, 101)) {
      send_to_char(g, ch, "You look at the images for {} and fail to perceive its worth..\r\n", obj_get_short_description(lot));
      act(g, "@c$n@w looks stumped about something they viewed on their scouter screen.@n", true, ch, nullptr, nullptr, act_to::room);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
    appraisal(g, ch, lot);
  }

  void place_bid(GameWorld *g, struct char_data *ch, struct obj_data *lot, int amount)
  {
    if (obj_get_curBidder(lot) == char_get_id(ch)) {
      send_to_char(g, ch, "You already have the highest bid.\r\n");
      return;
    }
    if (obj_get_aucter(lot) == char_get_id(ch)) {
      send_to_char(g, ch, "You auctioned the item, go to the auction house and cancel if you can.\r\n");
      return;
    }
    if (obj_get_curBidder(lot) > 0 && amount <= (obj_get_bid(lot) + (obj_get_bid(lot) * .1))) {
      send_to_char(g, ch, "You have to bid at least 10 percent over the current bid.\r\n");
      return;
    }
    if (amount < obj_get_bid(lot) && obj_get_curBidder(lot) <= -1) {
      send_to_char(g, ch, "You have to bid at least the starting bid.\r\n");
      return;
    }
    if (amount > (((char_get_gold(ch) + char_get_bank_gold(ch)) / 100) * 50) + (char_get_gold(ch) + char_get_bank_gold(ch))) {
      send_to_char(g, ch, "You can not bid more than 150% of your total money (on hand and in the bank).\r\n");
      return;
    }
    if (obj_get_aucTime(lot) + BIDDING_TIME <= game_now(g)) {
      send_to_char(g, ch, "Bidding on that object has been closed.\r\n");
      return;
    }
    obj_set_bid(lot, amount);
    obj_set_curBidder(lot, char_get_id(ch));
    auc_save(g);
    /* quirk: the log has the thing and the amount the wrong way round */
    mud_log("AUCTION: {} has bid {} on {}", char_get_name(ch), obj_get_short_description(lot), add_commas(amount));
    for (descriptor_data *d : each_desc(g)) {
      if (desc_get_connected(d) != CON_PLAYING || char_is_npc(d->character) || !char_get_equipment(d->character, WEAR_EYE))
        continue;
      if (d->character == ch)
        send_to_char(g, ch, "@RScouter Auction News@D: @GYou have bid @Y{}@G on @w{}@G@n\r\n", add_commas(obj_get_bid(lot)), obj_get_short_description(lot));
      else
        send_to_char(g, d->character, "@RScouter Auction News@D: @GThe bid on, @w{}@G, has been raised to @Y{}@n\r\n", obj_get_short_description(lot), add_commas(obj_get_bid(lot)));
    }
  }
} // namespace

ACMD(do_bid)
{
  if (char_is_npc(ch))
    return;
  if (!char_get_equipment(ch, WEAR_EYE)) {
    send_to_char(g, ch, "You need a scouter to make an auction bid.\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), room_flag::hbtc)) {
    send_to_char(g, ch, "This is a different dimension!\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), room_flag::past)) {
    send_to_char(g, ch, "This is the past, nothing is being auctioned!\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
    return;
  }
  int things = count_members(room_contents(g, AUCTION_HOUSE));

  if (str_eq_nocase(arg, "list")) {
    list_lots(g, ch);
    return;
  }
  if (str_eq_nocase(arg, "appraise")) {
    if (arg2.empty()) {
      bid_syntax(g, ch, "What item number did you want to appraise?\r\n");
      return;
    }
    if (parse_int(arg2) < 0 || parse_int(arg2) > things) {
      bid_syntax(g, ch, "That item number doesn't exist.\r\n");
      return;
    }
    struct obj_data *lot = auction_lot(g, parse_int(arg2));
    if (!lot) {
      send_to_char(g, ch, "That item number is not found.\r\n");
      return;
    }
    appraise(g, ch, lot);
    return;
  }
  if (arg2.empty()) {
    bid_syntax(g, ch, "What amount did you want to bid?\r\n");
    return;
  }
  if (parse_int(arg) < 0 || parse_int(arg) > things) {
    bid_syntax(g, ch, "That item number is not found.\r\n");
    return;
  }
  struct obj_data *lot = auction_lot(g, parse_int(arg));
  if (!lot) {
    send_to_char(g, ch, "That item number is not found.\r\n");
    return;
  }
  place_bid(g, ch, lot, parse_int(arg2));
}

/* build, and the other ways of assembling things */

namespace {
  constexpr obj_vnum TOOLKIT = 386, CAMPFIRE = 1611;

  /* What ch is making, and how it's said */
  struct making {
    GameWorld *g;
    struct char_data *ch;
    std::string_view verb;
    const std::string &text;
    obj_vnum vnum;

    void cant() const { send_to_char(g, ch, "You can't {} {} {}.\r\n", verb, AN(text), text); }

    /* The thing made and unmade, as a try fails so; false if it can't be made at all */
    bool botch(std::string_view how) const
    {
      struct obj_data *made = read_object(g, vnum);
      if (!made) {
        cant();
        return false;
      }
      extract_obj(g, made);
      send_to_char(g, ch, "You start to {} {} {}, {}\r\n", verb, AN(text), text, how);
      return true;
    }
  };

  /* A truffle's knack: one of the made thing's bonuses grows, or, if none does by the 11th try, one more is added */
  void truffle_touch(GameWorld *g, struct char_data *ch, struct obj_data *made)
  {
    auto aff    = obj_get_affected(made);
    int hasstat = 0;
    for (int tries = 1; tries <= 11; tries++) {
      int i = (tries - 1) % 6;
      if (aff[i].location > 0) {
        if (rand_number(g, 1, 6) <= 2) {
          aff[i].modifier += rand_number(g, 1, 3);
          send_to_char(g, ch, "@YYour intuitive skill with building has made this item even better!@n\r\n");
          return;
        }
        hasstat++;
      }
      if (tries == 11) {
        send_to_char(g, ch, "@YYour intuitive skill with building has made this item even better!@n\r\n");
        aff[i + 1].location = rand_number(g, 1, 6);
        aff[i + 1].modifier += rand_number(g, 1, 3);
        return;
      }
      if (tries == 6 && !hasstat)
        return;
    }
  }

  /* The made thing's worth, by how well it was made */
  void quality(GameWorld *g, struct char_data *ch, struct obj_data *made)
  {
    if (!char_is_truffle(ch) && axion_dice(g, 8) > char_get_skill(ch, SKILL_BUILD)) {
      send_to_char(g, ch, "@yYou've made an inferior product. Its value will be somewhat less.@n\r\n");
      obj_set_cost(made, obj_get_cost(made) - (obj_get_cost(made) * 0.25));
    } else if (char_is_truffle(ch) && axion_dice(g, 18) > char_get_skill(ch, SKILL_BUILD)) {
      send_to_char(g, ch, "@yYou've made an inferior product. Its value will be somewhat less.@n\r\n");
      obj_set_cost(made, obj_get_cost(made) - (obj_get_cost(made) * 0.12));
    } else if (char_is_truffle(ch) < char_get_skill(ch, SKILL_BUILD)) { /* quirk: whether a truffle, against the skill */
      send_to_char(g, ch, "@YYou've made an excellent product. Its value will be somewhat more.@n\r\n");
      obj_set_cost(made, obj_get_cost(made) + (obj_get_cost(made) * 0.12));
    }
  }

  void truffle_bonus(GameWorld *g, struct char_data *ch)
  {
    if (char_get_level(ch) < 100 && level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0) {
      int64_t gain = level_exp(ch, char_get_level(ch) + 1) * 0.011;
      send_to_char(g, ch, "@RExp Bonus@D: @G{}@n\r\n", add_commas(gain));
      gain_exp(g, ch, gain);
    } else {
      gain_exp(g, ch, 1375000);
      send_to_char(g, ch, "@RExp Bonus@D: @G{}@n\r\n", add_commas(1375000));
    }
  }
} // namespace

ACMD(do_assemble)
{
  std::string text(skip_spaces(argument));
  struct obj_data *tool = nullptr;

  for (obj_data *kit : char_inventory(g, ch)) {
    if (obj_get_vnum(g, kit) != TOOLKIT || obj_get_value(kit, VAL_ALL_HEALTH) <= 0)
      continue;
    tool = kit;
    tell_both(g, ch, nullptr, "@WYou open up your toolkit and take out the necessary tools.@n", "@C$n@W opens up $s toolkit and takes out the necessary tools.@n");
  }

  if (text.empty()) {
    send_to_char(g, ch, "What would you like to {}?\r\n", CMD_NAME);
    return;
  }
  making m{g, ch, CMD_NAME, text, assemblyFindAssembly(g, text.c_str())};
  if (m.vnum < 0 || assemblyGetType(g, m.vnum) != subcmd) {
    m.cant();
    return;
  }
  if (!assemblyCheckComponents(g, m.vnum, ch, false)) {
    send_to_char(g, ch, "You haven't got all the things you need.\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), room_flag::space)) {
    send_to_char(g, ch, "You can't do that in space.");
    return;
  }
  bool campfire = str_eq_nocase(text, "campfire");
  if (!char_get_skill(ch, SKILL_SURVIVAL) && campfire) {
    send_to_char(g, ch, "You know nothing about building campfires.\r\n");
    return;
  }
  if ((text.contains("Signal") || text.contains("signal")) && char_get_skill(ch, SKILL_BUILD) < 70) {
    send_to_char(g, ch, "You need at least a build skill level of 70.\r\n");
    return;
  }

  int roll = 0;
  if (!tool) {
    send_to_char(g, ch, "You wish you had tools, but make the best out of what you do have anyway...\r\n");
    roll = 20;
  }
  if (!campfire) {
    /* quirk: everything but a campfire needs a place a fire could burn */
    if (room_flagged(g, char_get_in_room(ch), room_flag::space) || room_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM || room_sunken(g, char_get_in_room(ch))) {
      send_to_char(g, ch, "This area will not allow a fire to burn properly.\r\n");
      return;
    }
    roll += axion_dice(g, 0);
    improve_skill(g, ch, SKILL_BUILD, 1);
    if (char_get_skill(ch, SKILL_BUILD) <= roll) {
      if (m.botch("but mess up royally!"))
        assemblyCheckComponents(g, m.vnum, ch, true);
      return;
    }
  } else {
    roll += axion_dice(g, char_get_skill(ch, SKILL_SURVIVAL) >= 90 ? 0 : -10);
    improve_skill(g, ch, SKILL_BUILD, 1);
    if (char_get_skill(ch, SKILL_SURVIVAL) <= roll) {
      if (!m.botch("but mess up royally!"))
        return;
      if (tool && rand_number(g, 1, 3) == 3 && obj_get_value(tool, VAL_ALL_HEALTH) > 0) {
        obj_set_value(tool, VAL_ALL_HEALTH, obj_get_value(tool, VAL_ALL_HEALTH) - (rand_number(g, 1, 5)));
        act(g, "@RYour toolset is looking a bit more worn.@n", true, ch, nullptr, nullptr, act_to::actor);
        if (obj_get_value(tool, VAL_ALL_HEALTH) <= 0)
          obj_set_value(tool, VAL_ALL_HEALTH, 0);
      }
      assemblyCheckComponents(g, m.vnum, ch, true);
      return;
    }
  }

  if (axion_dice(g, 0) - (char_get_aff_abils_intel(ch) / 5) > 95) {
    if (m.botch("but forget a couple of steps. You take it apart and give up."))
      char_set_wait(ch, PULSE_6SEC);
    return;
  }
  if (rand_number(g, 1, 100) >= 92) {
    if (m.botch("but put it together wrong and have to stop. You take it apart and give up."))
      char_set_wait(ch, PULSE_4SEC);
    return;
  }

  struct obj_data *made = read_object(g, m.vnum);
  if (!made) {
    m.cant();
    return;
  }
  add_unique_id(g, made);
  if (obj_get_vnum(g, made) != CAMPFIRE) {
    obj_to_char(g, made, ch);
    if (char_is_truffle(ch))
      truffle_touch(g, ch, made);
  } else {
    obj_to_room(g, made, char_get_in_room(ch));
    obj_set_timer(made, (char_get_skill(ch, SKILL_SURVIVAL) * 0.12));
  }
  act(g, str_format("You {} $p.", m.verb), false, ch, made, nullptr, act_to::actor);
  act(g, str_format("$n {}s $p.", m.verb), false, ch, made, nullptr, act_to::room);
  assemblyCheckComponents(g, m.vnum, ch, true);
  quality(g, ch, made);
  if (char_is_truffle(ch) && rand_number(g, 1, 5) >= 4 && obj_get_cost(made) >= 500)
    truffle_bonus(g, ch);
}

/* put */

namespace {
  bool is_duel_table(GameWorld *g, struct obj_data *obj) { return obj_get_vnum(g, obj) >= 600 && obj_get_vnum(g, obj) <= 603; }

  bool is_card_case(GameWorld *g, struct obj_data *obj)
  {
    obj_vnum vnum = obj_get_vnum(g, obj);
    return vnum == 697 || vnum == 698 || vnum == 682 || vnum == 683 || vnum == 684 || obj_extra_flagged(obj, extra_flag::cardcase);
  }

  /* Whether a sheath has room for one more weapon, saying why not */
  bool sheath_fits(GameWorld *g, struct char_data *ch, struct obj_data *obj, struct obj_data *sheath)
  {
    if (obj_get_type_flag(obj) != ITEM_WEAPON) {
      send_to_char(g, ch, "That is made to only hold weapons.\r\n");
      return false;
    }
    int count = 0, minus = 0;
    for (obj_data *item : obj_contents(g, sheath)) {
      minus += obj_get_weight(item);
      count++;
    }
    int holds = obj_get_weight(sheath) - minus;
    if (count >= holds) {
      send_to_char(g, ch, "It can only hold {:d} weapon{} at a time.\r\n", holds, holds > 1 ? "s" : "");
      return false;
    }
    return true;
  }

  /* Whether obj may go in cont, saying why not */
  bool can_put(GameWorld *g, struct char_data *ch, struct obj_data *obj, struct obj_data *cont)
  {
    if (obj_extra_flagged(cont, extra_flag::forged)) {
      act(g, "$P is forged and won't hold anything.", false, ch, nullptr, cont, act_to::actor);
      return false;
    }
    if (obj_extra_flagged(cont, extra_flag::sheath) && !sheath_fits(g, ch, obj, cont))
      return false;
    bool container = obj_get_type_flag(cont) == ITEM_CONTAINER;
    int capacity   = obj_get_value(cont, VAL_CONTAINER_CAPACITY);
    if (container && capacity == 0) {
      act(g, "$p won't fit in $P.", false, ch, obj, cont, act_to::actor);
      return false;
    }
    if (is_duel_table(g, cont)) {
      send_to_char(g, ch, "You can't put cards on a duel table. You have to @Gplay@n them.\r\n");
      return false;
    }
    if (is_card_case(g, cont) && !obj_extra_flagged(obj, extra_flag::anti_hierophant)) {
      send_to_char(g, ch, "You can only put cards in a case.\r\n");
      return false;
    }
    if (container && capacity > 0 && obj_get_weight(cont) + obj_get_weight(obj) > capacity) {
      act(g, "$p won't fit in $P.", false, ch, obj, cont, act_to::actor);
      return false;
    }
    if (obj_extra_flagged(obj, extra_flag::nodrop) && obj_get_in_room(cont) != NOWHERE) {
      act(g, "You can't get $p out of your hand.", false, ch, obj, nullptr, act_to::actor);
      return false;
    }
    if (is_dragon_ball(g, obj)) {
      send_to_char(g, ch, "You can not bag dragon balls.\r\n");
      return false;
    }
    if (obj_extra_flagged(obj, extra_flag::norent)) {
      send_to_char(g, ch, "That isn't worth bagging. Better keep that close if you wanna keep it at all.\r\n");
      return false;
    }
    if (!obj_get_carried_by(cont) && check_saveroom_count(g, ch, obj) > 150) {
      send_to_char(g, ch, "The save room can not hold anymore items. (150 max, count of items in containers is halved)\r\n");
      return false;
    }
    return true;
  }

  void perform_put(GameWorld *g, struct char_data *ch, struct obj_data *obj, struct obj_data *cont)
  {
    if (!drop_otrigger(g, obj, ch) || !can_put(g, ch, obj, cont))
      return;

    obj_from_char(g, obj);
    obj_to_obj(g, obj, cont);
    if (!obj_extra_flagged(obj, extra_flag::anti_hierophant))
      act(g, "$n puts $p in $P.", true, ch, obj, cont, act_to::room);
    else
      act(g, "$n puts an @DA@wd@cv@Ce@Wnt @DD@wu@ce@Cl @mC@Ma@Wr@wd@n in $P.", true, ch, obj, cont, act_to::room);

    /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
    if (obj_extra_flagged(obj, extra_flag::nodrop) && !obj_extra_flagged(cont, extra_flag::nodrop)) {
      obj_extra_set(cont, extra_flag::nodrop, true);
      act(g, "You get a strange feeling as you put $p in $P.", false, ch, obj, cont, act_to::actor);
    } else {
      act(g, "You put $p in $P.", false, ch, obj, cont, act_to::actor);
    }
    /* What's put in a portal or a vehicle goes where it leads */
    if (obj_get_type_flag(cont) == ITEM_PORTAL || obj_get_type_flag(cont) == ITEM_VEHICLE) {
      obj_from_obj(g, obj);
      obj_to_room(g, obj, obj_get_value(cont, VAL_CONTAINER_CAPACITY));
      if (obj_get_type_flag(cont) == ITEM_PORTAL) {
        act(g, "What? $U$p disappears from $P in a puff of smoke!", true, ch, obj, cont, act_to::room);
        act(g, "What? $U$p disappears from $P in a puff of smoke!", false, ch, obj, cont, act_to::actor);
      }
    }
  }

  /* put <object> <container>, of so many */
  void put_some(GameWorld *g, struct char_data *ch, std::string &what, struct obj_data *cont, int howmany)
  {
    struct obj_data *obj = get_obj_in_list_vis(g, ch, what, nullptr, char_get_inventory(ch));
    if (!obj) {
      send_to_char(g, ch, "You aren't carrying {} {}.\r\n", AN(what), what);
      return;
    }
    if (obj == cont && howmany == 1) {
      send_to_char(g, ch, "You attempt to fold it into itself, but fail.\r\n");
      return;
    }
    for (obj_data *match : obj_list_from(g, char_get_inventory(ch), obj)) {
      if (!howmany)
        break;
      if (!obj_named_and_visible(g, ch, what, match) || match == cont)
        continue;
      howmany--;
      perform_put(g, ch, match, cont);
    }
  }

  /* put all <container>, or put all.<object> <container> */
  void put_all(GameWorld *g, struct char_data *ch, const std::string &what, find_dots dotmode, struct obj_data *cont)
  {
    bool found = false;
    for (obj_data *item : char_inventory(g, ch)) {
      if (item != cont && char_can_see_obj(g, ch, item) && (dotmode == find_dots::all || isname(what, obj_get_name(item)))) {
        found = true;
        perform_put(g, ch, item, cont);
      }
    }
    if (found)
      return;
    if (dotmode == find_dots::all)
      send_to_char(g, ch, "You don't seem to have anything to put in it.\r\n");
    else
      send_to_char(g, ch, "You don't seem to have any {}s.\r\n", what);
  }
} // namespace

/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
  std::string_view rest = argument;
  std::string arg1      = take_arg(rest);
  std::string arg2      = take_arg(rest);
  std::string arg3      = take_arg(rest);

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }

  bool counted         = !arg3.empty() && is_number(arg1); /* put <count> <thing> <container> */
  int howmany          = counted ? parse_int(arg1) : 1;
  std::string &theobj  = counted ? arg2 : arg1;
  std::string &thecont = counted ? arg3 : arg2;
  find_dots obj_dotmode      = find_all_dots(theobj);
  find_dots cont_dotmode     = find_all_dots(thecont);

  if (theobj.empty()) {
    send_to_char(g, ch, "Put what in what?\r\n");
    return;
  }
  if (cont_dotmode != find_dots::indiv) {
    send_to_char(g, ch, "You can only put things into one container at a time.\r\n");
    return;
  }
  if (thecont.empty()) {
    send_to_char(g, ch, "What do you want to put {} in?\r\n", obj_dotmode == find_dots::indiv ? "it" : "them");
    return;
  }
  struct char_data *tmp_char;
  struct obj_data *cont;
  generic_find(g, thecont, find_flag::obj_inv | find_flag::obj_equip | find_flag::obj_room, ch, &tmp_char, &cont);
  if (!cont) {
    send_to_char(g, ch, "You don't see {} {} here.\r\n", AN(thecont), thecont);
    return;
  }
  if (obj_get_type_flag(cont) != ITEM_CONTAINER && obj_get_type_flag(cont) != ITEM_PORTAL && obj_get_type_flag(cont) != ITEM_VEHICLE) {
    act(g, "$p is not a container.", false, ch, cont, nullptr, act_to::actor);
    return;
  }
  if (obj_val_flagged(cont, container_flag::closed)) {
    send_to_char(g, ch, "You'd better open it first!\r\n");
    return;
  }
  if (obj_dotmode == find_dots::indiv)
    put_some(g, ch, theobj, cont, howmany);
  else
    put_all(g, ch, theobj, obj_dotmode, cont);
}

/* get */

namespace {
  bool can_take_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    if (!obj_wear_flagged(obj, wear_flag::take)) {
      act(g, "$p: you can't take that!", false, ch, obj, nullptr, act_to::actor);
      return false;
    }
    if (char_get_carry_items(ch) >= char_can_carry_n(ch)) {
      act(g, "$p: your arms are full!", false, ch, obj, nullptr, act_to::actor);
      return false;
    }
    if (getCurCarriedWeight(ch) + obj_get_weight(obj) > getMaxCarryWeight(g, ch)) {
      act(g, "$p: you can't carry that much weight.", false, ch, obj, nullptr, act_to::actor);
      return false;
    }
    if ((obj_get_weight(obj) + room_get_gravity(g, char_get_in_room(ch))) + getCurCarriedWeight(ch) > getMaxCarryWeight(g, ch)) {
      act(g, "$p: you can't carry that much weight because of the gravity.", false, ch, obj, nullptr, act_to::actor);
      return false;
    }
    return true;
  }

  /* Zenni taken up is counted as zenni, as much as can be carried */
  void get_check_money(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    int value = obj_get_value(obj, VAL_MONEY_SIZE);
    if (obj_get_type_flag(obj) != ITEM_MONEY || value <= 0)
      return;

    if (char_get_gold(ch) + value > char_gold_carry(ch)) {
      send_to_char(g, ch, "You can only carry {} zenni at your current level, and leave the rest.\r\n", add_commas(char_gold_carry(ch)));
      act(g, "@w$n @wdrops some onto the ground.@n", false, ch, nullptr, nullptr, act_to::room);
      extract_obj(g, obj);
      obj_to_room(g, create_money(g, (char_get_gold(ch) + value) - char_gold_carry(ch)), char_get_in_room(ch));
      char_set_gold(ch, char_gold_carry(ch));
      return;
    }
    char_set_gold(ch, char_get_gold(ch) + value);
    extract_obj(g, obj);
    if (value == 1) {
      send_to_char(g, ch, "There was 1 zenni.\r\n");
      return;
    }
    send_to_char(g, ch, "There were {:d} zenni.\r\n", value);
    if (char_is_affected(ch, aff_flag::group) && char_prf_flagged(g, ch, pref_flag::autosplit))
      do_split(g, ch, str_format("{:d}", value), 0, 0);
  }

  /* A hot thing burns the one who takes it up, unless they're fireproof or a demon; `hurt` is who it hurts */
  void scorch(GameWorld *g, struct char_data *holder, struct char_data *hurt, struct obj_data *obj, int64_t floor)
  {
    if (!obj_extra_flagged(obj, extra_flag::hot) || char_get_bonuses(holder, BONUS_FIREPROOF) > 0 || char_is_demon(holder))
      return;
    decCurHealthPercent(g, hurt, .25, floor);
    if (char_get_bonuses(holder, BONUS_FIREPRONE) > 0)
      decCurHealthPercent(g, hurt, 1, 1);
    char_aff_set(holder, aff_flag::burned, true);
    tell_both(g, holder, nullptr, "@RYou are burned by it!@n", "@R$n@R is burned by it!@n");
  }

  /* What's been taken up, and is held now */
  void taken(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    if (char_is_npc(ch))
      item_check(g, obj, ch);
    get_check_money(g, ch, obj);
  }

  bool at_another_table(GameWorld *g, struct char_data *ch, struct obj_data *cont)
  {
    struct obj_data *seat = char_get_sits(g, ch);
    return seat && obj_get_vnum(g, seat) > 603 && obj_get_vnum(g, seat) < 608 && obj_get_vnum(g, seat) - 4 != obj_get_vnum(g, cont) &&
           obj_get_vnum(g, cont) > 599 && obj_get_vnum(g, cont) < 604;
  }

  void perform_get_from_container(GameWorld *g, struct char_data *ch, struct obj_data *obj, struct obj_data *cont, std::optional<find_flag> mode)
  {
    if (mode != find_flag::obj_inv && mode != find_flag::obj_equip && !can_take_obj(g, ch, obj))
      return;
    if (char_get_carry_items(ch) >= char_can_carry_n(ch)) {
      act(g, "$p: you can't hold any more items.", false, ch, obj, nullptr, act_to::actor);
      return;
    }
    if (at_another_table(g, ch, cont)) {
      send_to_char(g, ch, "You aren't playing at that table!\r\n");
      return;
    }
    if (!get_otrigger(g, obj, ch))
      return;
    obj_from_obj(g, obj);
    obj_to_char(g, obj, ch);
    bool drawn = obj_extra_flagged(cont, extra_flag::sheath);
    act(g, std::string_view(drawn ? "You draw $p from $P." : "You get $p from $P."), false, ch, obj, cont, act_to::actor);
    act(g, std::string_view(drawn ? "$n draws $p from $P." : "$n gets $p from $P."), true, ch, obj, cont, act_to::room);
    /* quirk: a hot thing taken from a container may burn the last of one's health, as taking it up doesn't */
    scorch(g, ch, ch, obj, 0);
    taken(g, ch, obj);
  }

  void get_from_container(GameWorld *g, struct char_data *ch, struct obj_data *cont, std::string arg, std::optional<find_flag> mode, int howmany)
  {
    find_dots dotmode = find_all_dots(arg);

    if (obj_val_flagged(cont, container_flag::closed)) {
      act(g, "$p is closed.", false, ch, cont, nullptr, act_to::actor);
      return;
    }
    if (dotmode == find_dots::indiv) {
      struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, obj_get_contains(cont));
      if (!obj) {
        act(g, str_format("There doesn't seem to be {} {} in $p.", AN(arg), arg), false, ch, cont, nullptr, act_to::actor);
        return;
      }
      for (obj_data *match : obj_list_from(g, obj_get_contains(cont), obj)) {
        if (!obj_named_and_visible(g, ch, arg, match))
          continue;
        if (!howmany--)
          break;
        perform_get_from_container(g, ch, match, cont, mode);
      }
      return;
    }
    if (dotmode == find_dots::alldot && arg.empty()) {
      send_to_char(g, ch, "Get all of what?\r\n");
      return;
    }
    bool found = false;
    for (obj_data *item : obj_contents(g, cont)) {
      if (char_can_see_obj(g, ch, item) && (dotmode == find_dots::all || isname(arg, obj_get_name(item)))) {
        found = true;
        perform_get_from_container(g, ch, item, cont, mode);
      }
    }
    if (found)
      return;
    if (dotmode == find_dots::all)
      act(g, "$p seems to be empty.", false, ch, cont, nullptr, act_to::actor);
    else
      act(g, str_format("You can't seem to find any {}s in $p.", arg), false, ch, cont, nullptr, act_to::actor);
  }

  std::string_view find_exdesc_keywords(std::string_view word, const ex_desc_list &list)
  {
    for (const extra_descr_data &i : list)
      if (isname(word, i.keyword.c_str()))
        return i.keyword.c_str();
    return {};
  }

  void get_from_room(GameWorld *g, struct char_data *ch, std::string arg, int howmany)
  {
    room_vnum here = char_get_in_room(ch);

    /* Are they trying to take something in a room extra description? */
    if (find_exdesc(arg, room_get_ex_description(g, here)) != nullptr) {
      send_to_char(g, ch, "You can't take {} {}.\r\n", AN(arg), arg);
      return;
    }
    find_dots dotmode = find_all_dots(arg);

    if (dotmode == find_dots::indiv) {
      if (std::string_view descword = find_exdesc_keywords(arg, room_get_ex_description(g, here)); !descword.empty()) {
        send_to_char(g, ch, "{}: you can't take that!\r\n", fname(descword));
        return;
      }
      struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, here));
      if (!obj) {
        send_to_char(g, ch, "You don't see {} {} here.\r\n", AN(arg), arg);
        return;
      }
      for (obj_data *match : obj_list_from(g, room_get_contents(g, here), obj)) {
        if (!obj_named_and_visible(g, ch, arg, match))
          continue;
        if (!howmany--)
          break;
        perform_get_from_room(g, ch, match);
      }
      return;
    }
    if (dotmode == find_dots::alldot && arg.empty()) {
      send_to_char(g, ch, "Get all of what?\r\n");
      return;
    }
    bool found = false;
    for (obj_data *item : room_contents(g, here)) {
      if (char_can_see_obj(g, ch, item) && (dotmode == find_dots::all || isname(arg, obj_get_name(item)))) {
        found = true;
        perform_get_from_room(g, ch, item);
      }
    }
    if (found)
      return;
    if (dotmode == find_dots::all)
      send_to_char(g, ch, "There doesn't seem to be anything here.\r\n");
    else
      send_to_char(g, ch, "You don't see any {}s here.\r\n", arg);
  }

  /* get <things> <container> */
  void get_from_one(GameWorld *g, struct char_data *ch, std::string &what, std::string &from, int amount)
  {
    struct char_data *tmp_char;
    struct obj_data *cont;
    std::optional<find_flag> mode = generic_find(g, from, find_flag::obj_inv | find_flag::obj_equip | find_flag::obj_room, ch, &tmp_char, &cont);
    if (!cont)
      send_to_char(g, ch, "You don't have {} {}.\r\n", AN(from), from);
    else if (obj_get_type_flag(cont) == ITEM_VEHICLE)
      send_to_char(g, ch, "You will need to enter it first.\r\n");
    else if (obj_get_type_flag(cont) != ITEM_CONTAINER && !(obj_get_type_flag(cont) == ITEM_PORTAL && obj_val_flagged(cont, container_flag::closeable)))
      act(g, "$p is not a container.", false, ch, cont, nullptr, act_to::actor);
    else
      get_from_container(g, ch, cont, what, mode, amount);
  }

  /* get <things> all.<containers>, or all of them: those carried, then those in the room */
  void get_from_all(GameWorld *g, struct char_data *ch, const std::string &what, const std::string &from, find_dots dotmode, int amount)
  {
    bool found = false;
    auto from_each = [&](obj_data *cont, std::optional<find_flag> mode) {
      if (!char_can_see_obj(g, ch, cont) || (dotmode != find_dots::all && !isname(from, obj_get_name(cont))))
        return;
      if (obj_get_type_flag(cont) == ITEM_CONTAINER) {
        found = true;
        get_from_container(g, ch, cont, what, mode, amount);
      } else if (dotmode == find_dots::alldot) {
        found = true;
        act(g, "$p is not a container.", false, ch, cont, nullptr, act_to::actor);
      }
    };
    for (obj_data *thing : char_inventory(g, ch))
      from_each(thing, find_flag::obj_inv);
    for (obj_data *item : room_contents(g, char_get_in_room(ch)))
      from_each(item, find_flag::obj_room);
    if (found)
      return;
    if (dotmode == find_dots::all)
      send_to_char(g, ch, "You can't seem to find any containers.\r\n");
    else
      send_to_char(g, ch, "You can't seem to find any {}s here.\r\n", from);
  }
} // namespace

bool perform_get_from_room(GameWorld *g, struct char_data *ch, struct obj_data *obj)
{
  if (obj_get_sitting(g, obj)) {
    send_to_char(g, ch, "Someone is on that!\r\n");
    return false;
  }
  if (obj_extra_flagged(obj, extra_flag::buried)) {
    send_to_char(g, ch, "Get what?\r\n");
    return false;
  }
  if (in_garden(g, char_get_in_room(ch))) {
    send_to_char(g, ch, "You can't get things from a garden. Help garden.\r\n");
    return false;
  }
  if (!can_take_obj(g, ch, obj) || !get_otrigger(g, obj, ch))
    return false;
  obj_from_room(g, obj);
  obj_to_char(g, obj, ch);
  tell_both(g, ch, obj, "You get $p.", "$n gets $p.");
  scorch(g, ch, ch, obj, 1);
  taken(g, ch, obj);
  return false;
}

ACMD(do_get)
{
  std::string_view rest = argument;
  std::string arg1      = take_arg(rest);
  std::string arg2      = take_arg(rest);
  std::string arg3      = take_arg(rest);

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }
  if (arg1.empty()) {
    send_to_char(g, ch, "Get what?\r\n");
    return;
  }
  if (arg2.empty()) {
    get_from_room(g, ch, arg1, 1);
    return;
  }
  if (is_number(arg1) && arg3.empty()) {
    get_from_room(g, ch, arg2, parse_int(arg1));
    return;
  }
  int amount = 1;
  if (is_number(arg1)) {
    amount = parse_int(arg1);
    arg1   = arg2;
    arg2   = arg3;
  }
  find_dots cont_dotmode = find_all_dots(arg2);
  if (cont_dotmode == find_dots::indiv) {
    get_from_one(g, ch, arg1, arg2, amount);
    return;
  }
  if (cont_dotmode == find_dots::alldot && arg2.empty()) {
    send_to_char(g, ch, "Get from all of what?\r\n");
    return;
  }
  get_from_all(g, ch, arg1, arg2, cont_dotmode, amount);
}

/* drop, junk and donate */

namespace {
  void log_imm_drop(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    if (char_get_admlevel(ch) <= 0)
      return;
    send_to_imm(g, "IMM DROP: {} dropped {} in room [{:d}]", char_get_name(ch), obj_get_short_description(obj), obj_get_in_room(obj));
    log_imm_action(g, "IMM DROP: {} dropped {} in room [{:d}]", char_get_name(ch), obj_get_short_description(obj), obj_get_in_room(obj));
  }

  void perform_drop_gold(GameWorld *g, struct char_data *ch, int amount, int8_t mode, room_vnum RDR)
  {
    if (amount <= 0) {
      send_to_char(g, ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
      return;
    }
    if (char_get_gold(ch) < amount) {
      send_to_char(g, ch, "You don't have that many zenni!\r\n");
      return;
    }
    if (mode == SCMD_JUNK) {
      act(g, str_format("$n drops {} which disappears in a puff of smoke!", money_desc(amount)), false, ch, nullptr, nullptr, act_to::room);
      send_to_char(g, ch, "You drop some zenni which disappears in a puff of smoke!\r\n");
    } else {
      char_set_wait(ch, PULSE_1SEC); /* to prevent zenni-bombing */
      struct obj_data *obj = create_money(g, amount);
      if (mode == SCMD_DONATE) {
        send_to_char(g, ch, "You throw some zenni into the air where it disappears in a puff of smoke!\r\n");
        act(g, "$n throws some gold into the air where it disappears in a puff of smoke!", false, ch, nullptr, nullptr, act_to::room);
        obj_to_room(g, obj, RDR);
        act(g, "$p suddenly appears in a puff of orange smoke!", 0, nullptr, obj, nullptr, act_to::room);
      } else {
        if (!drop_wtrigger(g, obj, ch)) {
          extract_obj(g, obj);
          return;
        }
        act(g, str_format("$n drops {}.", money_desc(amount)), true, ch, nullptr, nullptr, act_to::room);
        send_to_char(g, ch, "You drop some zenni.\r\n");
        obj_to_room(g, obj, char_get_in_room(ch));
        log_imm_drop(g, ch, obj);
      }
    }
    char_set_gold(ch, char_get_gold(ch) - amount);
  }

  /* Whether a dragon ball may be let go of here, saying why not */
  bool can_drop_dragon_ball(GameWorld *g, struct char_data *ch, struct obj_data *obj, int8_t mode, std::string_view sname)
  {
    room_vnum here = char_get_in_room(ch);
    auto refuse    = [&](std::string_view where) {
      act(g, str_format("You can't {} $p {}!", sname, where), false, ch, obj, nullptr, act_to::actor);
      return false;
    };
    if (room_flagged(g, here, room_flag::space))
      return refuse("in space");
    if (in_garden(g, here)) {
      act(g, str_format("You can't {} $p in here. Read help garden.", sname), false, ch, obj, nullptr, act_to::actor);
      return false;
    }
    if ((mode == SCMD_DROP) && obj_extra_flagged(obj, extra_flag::norent)) {
      act(g, "You drop $p but it gets lost on the ground!", false, ch, obj, nullptr, act_to::actor);
      obj_from_char(g, obj);
      extract_obj(g, obj);
      return false;
    }
    if (room_flagged(g, here, room_flag::noinstant))
      return refuse("in this protected area");
    if (room_flagged(g, here, room_flag::ship))
      return refuse("on a private ship");
    if (room_flagged(g, here, room_flag::house))
      return refuse("in a private house");
    return true;
  }

  void perform_drop(GameWorld *g, struct char_data *ch, struct obj_data *obj, int8_t mode, std::string_view sname, room_vnum RDR)
  {
    if (!drop_otrigger(g, obj, ch))
      return;
    if ((mode == SCMD_DROP) && !drop_wtrigger(g, obj, ch))
      return;
    auto refuse = [&](const std::string &why) { act(g, why, false, ch, obj, nullptr, act_to::actor); };

    if (obj_get_vnum(g, obj) == 17 || obj_get_vnum(g, obj) == 17998) {
      refuse(str_format("You can't {} $p, it is grafted into your soul :P", sname));
      return;
    }
    if (is_dragon_ball(g, obj) && !can_drop_dragon_ball(g, ch, obj, mode, sname))
      return;
    if (obj_extra_flagged(obj, extra_flag::nodrop) && char_get_admlevel(ch) < 1) {
      refuse(str_format("You can't {} $p, it must be CURSED!", sname));
      return;
    }
    if ((mode == SCMD_DONATE || mode == SCMD_JUNK) && obj_extra_flagged(obj, extra_flag::nodonate)) {
      refuse(str_format("You can't {} $p!", sname));
      return;
    }

    act(g, str_format("You {} $p.", sname), false, ch, obj, nullptr, act_to::actor);
    act(g, str_format("$n {}s $p.", sname), true, ch, obj, nullptr, act_to::room);
    obj_from_char(g, obj);

    if (mode == SCMD_DONATE) {
      obj_to_room(g, obj, RDR);
      act(g, "$p suddenly appears in a puff a smoke!", false, nullptr, obj, nullptr, act_to::room);
      return;
    }
    if (mode == SCMD_JUNK) {
      extract_obj(g, obj);
      return;
    }
    if (room_get_geffect(g, char_get_in_room(ch)) == 6) {
      if (!obj_extra_flagged(obj, extra_flag::unbreakable)) {
        act(g, "$p melts in the lava!", false, ch, obj, nullptr, act_to::actor);
        act(g, "$p melts in the lava!", false, ch, obj, nullptr, act_to::room);
        extract_obj(g, obj);
        return;
      }
      act(g, "$p plops down on some cooled lava!", false, ch, obj, nullptr, act_to::actor);
      act(g, "$p plops down on some cooled lava!", false, ch, obj, nullptr, act_to::room);
    }
    obj_to_room(g, obj, char_get_in_room(ch));
    log_imm_drop(g, ch, obj);
  }

  /* Where donations go: a donation room, or the junk, by a roll; false if there's nowhere */
  bool donating(GameWorld *g, struct char_data *ch, int8_t &mode, room_vnum &RDR)
  {
    /* fail + double chance for room 1 */
    int num_don_rooms = (CONFIG_DON_ROOM_1 != NOWHERE) * 2 + (CONFIG_DON_ROOM_2 != NOWHERE) + (CONFIG_DON_ROOM_3 != NOWHERE) + 1;
    /* quirk: the roll is of more rooms than there may be, and past them it leaves room 0 */
    switch (rand_number(g, 0, num_don_rooms)) {
      case 0: mode = SCMD_JUNK; break;
      case 1:
      case 2: RDR = CONFIG_DON_ROOM_1; break;
      case 3: RDR = CONFIG_DON_ROOM_2; break;
      case 4: RDR = CONFIG_DON_ROOM_3; break;
    }
    if (room_of(g, RDR))
      return true;
    send_to_char(g, ch, "Sorry, you can't donate anything right now.\r\n");
    return false;
  }

  constexpr std::string_view SAVE_ROOM_FULL = "Some of the items couldn't be dropped into this save room. It is too full. (150 max, containers half the count inside)\r\n";
} // namespace

ACMD(do_drop)
{
  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }
  if (in_garden(g, char_get_in_room(ch))) {
    send_to_char(g, ch, "You can not do that in a garden.\r\n");
    return;
  }

  room_vnum RDR     = 0;
  int8_t mode       = SCMD_DROP;
  std::string_view sname = "drop";
  if (subcmd == SCMD_JUNK) {
    sname = "junk";
    mode  = SCMD_JUNK;
  } else if (subcmd == SCMD_DONATE) {
    sname = "donate";
    mode  = SCMD_DONATE;
    if (!donating(g, ch, mode, RDR))
      return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "What do you want to {}?\r\n", sname);
    return;
  }
  if (is_number(arg)) {
    int multi = parse_int(arg);
    arg       = take_arg(rest);
    if (str_eq_nocase("zenni", arg) || str_eq_nocase("gold", arg)) {
      perform_drop_gold(g, ch, multi, mode, RDR);
      return;
    }
    if (multi <= 0) {
      send_to_char(g, ch, "Yeah, that makes sense.\r\n");
      return;
    }
    if (arg.empty()) {
      send_to_char(g, ch, "What do you want to {} {:d} of?\r\n", sname, multi);
      return;
    }
    struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
    if (!obj) {
      send_to_char(g, ch, "You don't seem to have any {}s.\r\n", arg);
      return;
    }
    if (check_saveroom_count(g, ch, obj) > 150) {
      send_to_char(g, ch, "The save room you are in can not hold anymore items! (150 max, count of items in containers is halved)\r\n");
      return;
    }
    for (obj_data *match : obj_list_from(g, char_get_inventory(ch), obj)) {
      if (!obj_named_and_visible(g, ch, arg, match))
        continue;
      perform_drop(g, ch, match, mode, sname, RDR);
      if (!--multi)
        break;
    }
    return;
  }

  find_dots dotmode = find_all_dots(arg);
  /* Can't junk or donate all */
  if (dotmode == find_dots::all && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
    send_to_char(g, ch, subcmd == SCMD_JUNK ? "Go to the dump if you want to junk EVERYTHING!\r\n" : "Go do the donation room if you want to donate EVERYTHING!\r\n");
    return;
  }

  /* Each thing dropped, unless the save room here is full */
  bool full         = false;
  auto drop_or_note = [&](obj_data *thing) {
    if (check_saveroom_count(g, ch, thing) > 150)
      full = true;
    else
      perform_drop(g, ch, thing, mode, sname, RDR);
  };

  if (dotmode == find_dots::all) {
    if (!char_get_inventory(ch)) {
      send_to_char(g, ch, "You don't seem to be carrying anything.\r\n");
      return;
    }
    for (obj_data *thing : char_inventory(g, ch))
      drop_or_note(thing);
    if (full)
      send_to_char(g, ch, SAVE_ROOM_FULL);
    return;
  }
  if (dotmode == find_dots::alldot) {
    if (arg.empty()) {
      send_to_char(g, ch, "What do you want to {} all of?\r\n", sname);
      return;
    }
    struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
    if (!obj) {
      send_to_char(g, ch, "You don't seem to have any {}s.\r\n", arg);
      return;
    }
    for (obj_data *match : obj_list_from(g, char_get_inventory(ch), obj))
      if (obj_named_and_visible(g, ch, arg, match))
        drop_or_note(match);
    if (full)
      send_to_char(g, ch, SAVE_ROOM_FULL);
    return;
  }

  struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
  if (!obj)
    send_to_char(g, ch, "You don't seem to have {} {}.\r\n", AN(arg), arg);
  else if (check_saveroom_count(g, ch, obj) > 150)
    send_to_char(g, ch, "The item couldn't be dropped into this save room. It is too full. (150 max, containers half the count inside)\r\n");
  else
    perform_drop(g, ch, obj, mode, sname, RDR);
}

/* give */

namespace {
  void log_imm_give(GameWorld *g, struct char_data *ch, struct char_data *vict, std::string_view what)
  {
    if (char_get_admlevel(ch) <= 0 || char_is_npc(vict))
      return;
    send_to_imm(g, "IMM GIVE: {} has given {} to {}.", char_get_name(ch), what, char_get_name(vict));
    log_imm_action(g, "IMM GIVE: {} has given {} to {}.", char_get_name(ch), what, char_get_name(vict));
  }

  /* vict can't take obj, ch is told why; a mob that tried drops it */
  void cant_take(GameWorld *g, struct char_data *ch, struct char_data *vict, struct obj_data *obj, std::string_view why)
  {
    act(g, why, false, ch, nullptr, vict, act_to::actor);
    if (!char_is_npc(ch))
      return;
    act_triple(g, {{}, "$n@n drops $p because you can't carry anymore.", "$n@n drops $p on the ground since $N's unable to carry it.", true}, ch, obj, vict);
    obj_from_char(g, obj);
    obj_to_room(g, obj, char_get_in_room(ch));
  }

  void perform_give(GameWorld *g, struct char_data *ch, struct char_data *vict, struct obj_data *obj)
  {
    if (!give_otrigger(g, obj, ch, vict) || !receive_mtrigger(g, vict, ch, obj))
      return;

    if (obj_extra_flagged(obj, extra_flag::nodrop)) {
      act(g, "You can't let go of $p!!  Yeech!", false, ch, obj, nullptr, act_to::actor);
      return;
    }
    if (char_get_carry_items(vict) >= char_can_carry_n(vict)) {
      cant_take(g, ch, vict, obj, "$N seems to have $S hands full.");
      return;
    }
    if (char_is_npc(vict) && obj_extra_flagged(obj, extra_flag::forged)) {
      act(g, "$n tries to hand $p to $N.", true, ch, obj, vict, act_to::notvict);
      do_say(g, vict, "I don't want that piece of junk.", 0, 0);
      return;
    }
    if (obj_get_weight(obj) + getCurCarriedWeight(vict) > getMaxCarryWeight(g, vict)) {
      cant_take(g, ch, vict, obj, "$E can't carry that much weight.");
      return;
    }
    if ((obj_get_weight(obj) + room_get_gravity(g, char_get_in_room(vict))) + getCurCarriedWeight(vict) > getMaxCarryWeight(g, vict)) {
      cant_take(g, ch, vict, obj, "$E can't carry that much weight because of the gravity.");
      return;
    }
    if (!char_is_npc(vict) && !char_is_npc(ch) && char_prf_flagged(g, vict, pref_flag::nogive)) {
      act_triple(g, {"$N refuses to accept $p at this time.", "$n tries to give you $p, but you are refusing to be handed things.", "$n tries to give $N, $p, but $E is refusing to be handed things right now."}, ch, obj, vict);
      return;
    }
    obj_from_char(g, obj);
    obj_to_char(g, obj, vict);
    act_triple(g, {"You give $p to $N.", "$n gives you $p.", "$n gives $p to $N.", true}, ch, obj, vict);
    /* quirk: the one burned isn't the one hurt, who is the giver */
    scorch(g, vict, ch, obj, 1);
  }

  void give_and_log(GameWorld *g, struct char_data *ch, struct char_data *vict, struct obj_data *obj)
  {
    perform_give(g, ch, vict, obj);
    log_imm_give(g, ch, vict, obj_get_short_description(obj));
  }

  /* Who ch means to give to, or null, having said why not */
  struct char_data *give_find_vict(GameWorld *g, struct char_data *ch, std::string_view arg)
  {
    std::string name(skip_spaces(arg));
    if (name.empty()) {
      send_to_char(g, ch, "To who?\r\n");
      return nullptr;
    }
    struct char_data *vict = get_char_vis(g, ch, name, nullptr, find_flag::char_room);
    if (!vict) {
      send_to_char(g, ch, CONFIG_NOPERSON);
      if (char_is_npc(ch))
        send_to_imm(g, "Mob Give: Victim, {}, doesn't exist.", name);
      return nullptr;
    }
    if (vict == ch) {
      send_to_char(g, ch, "What's the point of that?\r\n");
      return nullptr;
    }
    return vict;
  }

  void perform_give_gold(GameWorld *g, struct char_data *ch, struct char_data *vict, int amount)
  {
    bool pays = char_is_npc(ch) || !char_adm_flagged(ch, admin_flag::money);
    if (amount <= 0) {
      send_to_char(g, ch, "Heh heh heh ... we are jolly funny today, eh?\r\n");
      return;
    }
    if (char_get_gold(ch) < amount && pays) {
      send_to_char(g, ch, "You don't have that much zenni!\r\n");
      return;
    }
    if (char_get_gold(vict) + amount > char_gold_carry(vict)) {
      send_to_char(g, ch, "They can't carry that much zenni.\r\n");
      return;
    }
    send_to_char(g, ch, CONFIG_OK);
    act(g, str_format("$n gives you {:d} zenni.", amount), false, ch, nullptr, vict, act_to::vict);
    act(g, str_format("$n gives {} to $N.", money_desc(amount)), true, ch, nullptr, vict, act_to::notvict);
    if (pays)
      char_set_gold(ch, char_get_gold(ch) - amount);
    char_set_gold(vict, char_get_gold(vict) + amount);
    bribe_mtrigger(g, vict, ch, amount);
  }

  /* give <count> <things> <who>, or give <count> zenni <who> */
  void give_counted(GameWorld *g, struct char_data *ch, int amount, std::string_view rest)
  {
    std::string arg = take_arg(rest);
    struct char_data *vict;

    if (str_eq_nocase("zenni", arg) || str_eq_nocase("gold", arg)) {
      arg = take_arg(rest);
      if (!(vict = give_find_vict(g, ch, arg)))
        return;
      perform_give_gold(g, ch, vict, amount);
      log_imm_give(g, ch, vict, str_format("{} zenni", add_commas(amount)));
      return;
    }
    if (arg.empty()) {
      send_to_char(g, ch, "What do you want to give {:d} of?\r\n", amount);
      return;
    }
    if (!(vict = give_find_vict(g, ch, rest)))
      return;
    struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
    if (!obj) {
      send_to_char(g, ch, "You don't seem to have any {}s.\r\n", arg);
      return;
    }
    for (obj_data *match : obj_list_from(g, char_get_inventory(ch), obj)) {
      if (!obj_named_and_visible(g, ch, arg, match))
        continue;
      if (!amount--)
        break;
      give_and_log(g, ch, vict, match);
    }
  }
} // namespace

ACMD(do_give)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }
  reveal_hiding(g, ch, 0);
  if (arg.empty()) {
    send_to_char(g, ch, "Give what to who?\r\n");
    return;
  }
  if (is_number(arg)) {
    give_counted(g, ch, parse_int(arg), rest);
    return;
  }

  struct char_data *vict = give_find_vict(g, ch, take_arg(rest));
  if (!vict)
    return;
  find_dots dotmode = find_all_dots(arg);
  if (dotmode == find_dots::indiv) {
    struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
    if (!obj)
      send_to_char(g, ch, "You don't seem to have {} {}.\r\n", AN(arg), arg);
    else
      give_and_log(g, ch, vict, obj);
    return;
  }
  if (dotmode == find_dots::alldot && arg.empty()) {
    send_to_char(g, ch, "All of what?\r\n");
    return;
  }
  if (!char_get_inventory(ch)) {
    send_to_char(g, ch, "You don't seem to be holding anything.\r\n");
    return;
  }
  for (obj_data *item : char_inventory(g, ch))
    if (char_can_see_obj(g, ch, item) && (dotmode == find_dots::all || isname(arg, obj_get_name(item))))
      give_and_log(g, ch, vict, item);
}

/* drink, eat and pour, and the things they hold */

void weight_change_object(GameWorld *g, struct obj_data *obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (obj_get_in_room(obj) != NOWHERE) {
    obj_set_weight(obj, obj_get_weight(obj) + weight);
  } else if ((tmp_ch = obj_get_carried_by(obj))) {
    obj_from_char(g, obj);
    obj_set_weight(obj, obj_get_weight(obj) + weight);
    obj_to_char(g, obj, tmp_ch);
  } else if ((tmp_obj = obj_get_in_obj(obj))) {
    obj_from_obj(g, obj);
    obj_set_weight(obj, obj_get_weight(obj) + weight);
    obj_to_obj(g, obj, tmp_obj);
  } else {
    mud_log("SYSERR: Unknown attempt to subtract weight from an object.");
  }
}

void name_from_drinkcon(GameWorld *g, struct obj_data *obj)
{
  if (!obj || (obj_get_type_flag(obj) != ITEM_DRINKCON && obj_get_type_flag(obj) != ITEM_FOUNTAIN))
    return;

  if (obj_get_value(obj, VAL_DRINKCON_LIQUID) < 0 || obj_get_value(obj, VAL_DRINKCON_LIQUID) >= NUM_LIQ_TYPES)
    return;
  std::string_view liquid = drinknames[obj_get_value(obj, VAL_DRINKCON_LIQUID)];
  if (!isname(liquid, obj_get_name(obj)))
    return;

  /* The name without its words that start with the liquid's name (quirk: a longer word such as "waterskin" goes
     too), a space between those kept; a space at the very start is passed over */
  std::string_view name = obj_get_name(obj);
  std::string kept;
  for (auto word : name | std::views::split(' ')) {
    auto at = static_cast<size_t>(word.begin() - name.begin());
    if (at == 0 && name.starts_with(' '))
      continue;
    if (str_eq_nocase(name.substr(at, liquid.size()), liquid))
      continue;
    if (!kept.empty())
      kept += ' ';
    kept.append(word.begin(), word.end());
  }

  obj_set_name(obj, kept);
}

void name_to_drinkcon(GameWorld *g, struct obj_data *obj, int type)
{
  if (!obj || (obj_get_type_flag(obj) != ITEM_DRINKCON && obj_get_type_flag(obj) != ITEM_FOUNTAIN))
    return;
  if (type < 0 || type >= NUM_LIQ_TYPES)
    return;

  obj_set_name(obj, str_format("{} {}", obj_get_name(obj), drinknames[type]));
}

namespace {
  void poison(GameWorld *g, struct char_data *ch, int duration)
  {
    struct affected_type af = {};
    af.type                 = SPELL_POISON;
    af.duration             = duration;
    af.modifier             = 0;
    af.location             = APPLY_NONE;
    af.bitvector            = aff_flag::poison;
    affect_join(g, ch, &af, false, false, false, false);
  }

  /* A wellspring's drink restores ki; quirk: the water around always says it's full */
  void wellspring(GameWorld *g, struct char_data *ch, bool always_full)
  {
    if (incCurKI(g, ch, ((getMaxKI(g, ch) * 0.005) + (char_get_aff_abils_wis(ch) * rand_number(g, 80, 100))) * char_get_skill(ch, SKILL_WELLSPRING), true) == getMaxKI(g, ch) ||
        always_full)
      send_to_char(g, ch, "You feel your ki return to full strength.\r\n");
    else
      send_to_char(g, ch, "You feel your ki has rejuvenated.\r\n");
  }

  void quenched(GameWorld *g, struct char_data *ch)
  {
    if (char_get_conditions(g, ch, THIRST) >= 48)
      send_to_char(g, ch, "You don't feel thirsty anymore.\r\n");
  }

  /* A drink from the water around ch, if there is any */
  void drink_around(GameWorld *g, struct char_data *ch, int wasthirsty, int subcmd)
  {
    int sect   = room_sect(g, char_get_in_room(ch));
    bool water = sect == SECT_WATER_SWIM || sect == SECT_WATER_NOSWIM || sect == SECT_UNDERWATER;
    if (!water && !room_sunken(g, char_get_in_room(ch))) {
      send_to_char(g, ch, "Drink from what?\r\n");
      return;
    }
    act(g, "$n takes a refreshing drink from the surrounding water.", true, ch, nullptr, nullptr, act_to::room);
    send_to_char(g, ch, "You take a refreshing drink from the surrounding water.\r\n");
    gain_condition(g, ch, THIRST, 1);
    if (char_get_skill(ch, SKILL_WELLSPRING) && (water ? getCurKI(g, ch) < getMaxKI(g, ch) : !isFullKI(ch)) && wasthirsty <= 30 && subcmd != SCMD_SIP)
      wellspring(g, ch, water);
    quenched(g, ch);
  }

  /* Whether ch can drink from temp, saying why not */
  bool drinkable(GameWorld *g, struct char_data *ch, struct obj_data *temp, bool on_ground)
  {
    if (obj_get_type_flag(temp) != ITEM_DRINKCON && obj_get_type_flag(temp) != ITEM_FOUNTAIN) {
      if (obj_get_vnum(g, temp) == 86 && !on_ground) { /* a ki potion */
        act_triple(g, {"@wYou uncork the $p and tip it to your lips. Drinking it down you feel a warmth flow through your body and your ki returns to full strength .@n", {},
                       "@C$n@w uncorks the $p and tips it to $s lips. Drinking it down and then smiling.@n", true},
                   ch, temp, nullptr);
        obj_from_char(g, temp);
        extract_obj(g, temp);
        restoreKI(ch, true);
        gain_condition(g, ch, THIRST, 8);
      } else if (obj_get_vnum(g, temp) == 86) {
        send_to_char(g, ch, "You need to pick that up first.\r\n");
      } else {
        send_to_char(g, ch, "You can't drink from that!\r\n");
      }
      return false;
    }
    if (on_ground && obj_get_type_flag(temp) == ITEM_DRINKCON) {
      send_to_char(g, ch, "You have to be holding that to drink from it.\r\n");
      return false;
    }
    if (obj_extra_flagged(temp, extra_flag::broken)) {
      send_to_char(g, ch, "Seems like it's broken!\r\n");
      return false;
    }
    if (obj_extra_flagged(temp, extra_flag::forged)) {
      send_to_char(g, ch, "Seems like it doesn't work, maybe it is fake...\r\n");
      return false;
    }
    if (char_get_conditions(g, ch, DRUNK) > 10 && char_get_conditions(g, ch, THIRST) > 0) {
      /* The pig is drunk */
      send_to_char(g, ch, "You can't seem to get close enough to your mouth.\r\n");
      act(g, "$n tries to drink but misses $s mouth!", true, ch, nullptr, nullptr, act_to::room);
      return false;
    }
    if (obj_get_value(temp, VAL_DRINKCON_HOWFULL) <= 0 && obj_get_value(temp, VAL_DRINKCON_CAPACITY) >= 1) {
      send_to_char(g, ch, "It's empty.\r\n");
      return false;
    }
    return true;
  }
} // namespace

ACMD(do_drink)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch))
    return;
  if (char_is_android(ch) || char_get_conditions(g, ch, THIRST) < 0) {
    send_to_char(g, ch, "You need not drink!\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::healt)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_get_conditions(g, ch, HUNGER) <= 1 && char_get_conditions(g, ch, THIRST) >= 2 && !char_is_namek(ch) && char_get_genome(ch, 0) != 3 && char_get_genome(ch, 1) != 3) {
    send_to_char(g, ch, "You need to eat first!\r\n");
    return;
  }
  int wasthirsty = char_get_conditions(g, ch, THIRST);
  if (arg.empty()) {
    drink_around(g, ch, wasthirsty, subcmd);
    return;
  }

  bool on_ground       = false;
  struct obj_data *temp = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
  if (!temp) {
    if (!(temp = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "You can't find it!\r\n");
      return;
    }
    on_ground = true;
  }
  if (!drinkable(g, ch, temp, on_ground) || !consume_otrigger(g, temp, ch, consume_cmd::drink))
    return;

  int amount;
  if (subcmd == SCMD_DRINK) {
    act(g, str_format("$n drinks {} from $p.", drinks[obj_get_value(temp, VAL_DRINKCON_LIQUID)]), true, ch, temp, nullptr, act_to::room);
    send_to_char(g, ch, "You drink the {}.\r\n", drinks[obj_get_value(temp, VAL_DRINKCON_LIQUID)]);
    if (!obj_get_action_description(temp).empty())
      act(g, obj_get_action_description(temp), true, ch, temp, nullptr, act_to::actor);
    amount = 4;
  } else {
    act(g, "$n sips from $p.", true, ch, temp, nullptr, act_to::room);
    send_to_char(g, ch, "It tastes like {}.\r\n", drinks[obj_get_value(temp, VAL_DRINKCON_LIQUID)]);
    if (obj_get_value(temp, VAL_DRINKCON_POISON)) {
      send_to_char(g, ch, "It has a sickening taste! Better not eat it...");
      return;
    }
    amount = 1;
  }
  amount = MIN(amount, obj_get_value(temp, VAL_DRINKCON_HOWFULL));

  /* What's eternal (of no capacity) weighs no less */
  if (obj_get_value(temp, VAL_DRINKCON_CAPACITY) > 0) {
    int weight = MIN(amount, obj_get_weight(temp));
    weight_change_object(g, temp, -weight);
  }

  int liquid = obj_get_value(temp, VAL_DRINKCON_LIQUID);
  gain_condition(g, ch, DRUNK, drink_aff[liquid][DRUNK] * amount);
  gain_condition(g, ch, HUNGER, drink_aff[liquid][HUNGER] * amount);
  gain_condition(g, ch, THIRST, drink_aff[liquid][THIRST] * amount);
  if (char_get_foodr(ch) == 0 && subcmd != SCMD_SIP) {
    incCurST(g, ch, (getMaxST(g, ch) / 100) * amount, true);
    char_set_foodr(ch, 2);
    send_to_char(g, ch, "You feel rejuvinated by it.\r\n");
  }
  if (char_get_skill(ch, SKILL_WELLSPRING) && getCurKI(g, ch) < getMaxKI(g, ch) && wasthirsty <= 30 && subcmd != SCMD_SIP &&
      (liquid == LIQ_WATER || liquid == 14 || liquid == 15))
    wellspring(g, ch, false);

  if (char_get_conditions(g, ch, DRUNK) > 10)
    send_to_char(g, ch, "You feel drunk.\r\n");
  quenched(g, ch);

  if (obj_get_value(temp, VAL_DRINKCON_POISON) && (!char_is_mutant(ch) || (char_get_genome(ch, 0) != 7 && char_get_genome(ch, 1) != 7))) {
    send_to_char(g, ch, "Oops, it tasted rather strange!\r\n");
    act(g, "$n chokes and utters some strange sounds.", true, ch, nullptr, nullptr, act_to::room);
    poison(g, ch, amount * 3);
  }
  /* What's eternal doesn't empty, nor is it unpoisoned */
  if (obj_get_value(temp, VAL_DRINKCON_CAPACITY) > 0) {
    obj_set_value(temp, VAL_DRINKCON_HOWFULL, obj_get_value(temp, VAL_DRINKCON_HOWFULL) - amount);
    if (!obj_get_value(temp, VAL_DRINKCON_HOWFULL)) { /* The last bit */
      name_from_drinkcon(g, temp);
      obj_set_value(temp, VAL_DRINKCON_POISON, 0);
    }
  }
}

namespace {
  /* The candies a majin grows by: what's restored, and the most each of their powers can grow, at a share of it */
  struct candy {
    obj_vnum vnum;
    double stamina;
    int64_t most, share;
  };

  constexpr auto CANDIES = std::to_array<candy>({
      {53, 0.033, 300000,  1200},
      {93, 0.05,  500000,  1200},
      {94, 0.1,   1200000, 1000},
      {95, 0.1,   1500000, 900 },
  });

  void majin_gain(GameWorld *g, struct char_data *ch, const candy &c)
  {
    if (!char_is_majin(ch) || char_is_npc(ch))
      return;
    /* Rillao: transloc, add new transes here */
    if (is_soft_cap(ch, 0, 1.0)) {
      send_to_char(g, ch, "You can not gain anymore from candy consumption at your current level.\r\n");
      return;
    }
    auto gain  = [&](int64_t base) { return MIN(c.most, (base / c.share) + rand_number(g, char_get_level(ch), char_get_level(ch) * 2)); };
    int64_t st = gain(getBaseST(ch)), pl = gain(getBasePL(ch)), ki = gain(getBaseKI(ch));
    gainBasePL(ch, pl, true);
    gainBaseKI(ch, ki, true);
    gainBaseST(ch, st, true);
    send_to_char(g, ch, "@mYou feel stronger after consuming the candy @D[@RPL@W: @r{} @CKi@D: @c{} @GSt@D: @g{}@D]@m!@n\r\n", add_commas(pl), add_commas(ki), add_commas(st));
  }

  void full(GameWorld *g, struct char_data *ch)
  {
    send_to_char(g, ch, char_is_majin(ch) ? "You are full, but there's always room for candy!\r\n" : "You are full.\r\n");
  }

  /* The good of what was eaten, but for a taste */
  void nourished(GameWorld *g, struct char_data *ch, struct obj_data *food, double percent_eaten)
  {
    int maxfval  = MAX(1, obj_get_value(food, VAL_FOOD_MAXFOODVAL));
    int psbonus  = (int)((double)(maxfval + obj_get_value(food, VAL_FOOD_PSBONUS)) * percent_eaten);
    int expbonus = (int)((double)obj_get_value(food, VAL_FOOD_EXPBONUS) * ((char_get_level(ch) * 0.4) + 1) * percent_eaten);
    gain_exp(g, ch, expbonus);
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) + psbonus);
    send_to_char(g, ch, "That was exceptionally delicious! @D[@mPS@D: @C+{:d}@D] [@gEXP@D: @G+{}@D]@n\r\n", psbonus, add_commas(expbonus));
    if (obj_get_value(food, VAL_FOOD_POISON) || getCurPL(g, ch) >= getEffMaxPL(g, ch))
      return;
    incCurHealthPercent(g, ch, obj_get_weight(food) < 6 ? .05 : .1, true);
    if (obj_extra_flagged(food, extra_flag::only_wizard)) /* quirk: reads extra flag ONLY_WIZARD (33), meant item type YUM (33) */
      incCurHealthPercent(g, ch, .2, true);
    send_to_char(g, ch, "@MYou feel some of your strength return!@n\r\n");
  }
} // namespace

ACMD(do_eat)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch)) /* Cannot use GET_COND() on mobs. */
    return;
  if (char_is_android(ch) || char_get_conditions(g, ch, HUNGER) < 0) {
    send_to_char(g, ch, "You need not eat!\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::healt)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_aff_flagged(ch, aff_flag::poison)) {
    send_to_char(g, ch, "You feel too sick from the poison to eat!\r\n");
    return;
  }
  if (arg.empty()) {
    send_to_char(g, ch, "Eat what?\r\n");
    return;
  }
  struct obj_data *food = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
  if (!food) {
    send_to_char(g, ch, "You don't seem to have {} {}.\r\n", AN(arg), arg);
    return;
  }
  if (char_get_conditions(g, ch, THIRST) <= 1 && char_get_conditions(g, ch, HUNGER) >= 2) {
    send_to_char(g, ch, "You need to drink first!\r\n");
    return;
  }
  if (subcmd == SCMD_TASTE && (obj_get_type_flag(food) == ITEM_DRINKCON || obj_get_type_flag(food) == ITEM_FOUNTAIN)) {
    do_drink(g, ch, argument, 0, SCMD_SIP);
    return;
  }
  if (obj_get_type_flag(food) != ITEM_FOOD && obj_get_type_flag(food) != ITEM_YUM) {
    send_to_char(g, ch, "You can't eat THAT!\r\n");
    return;
  }
  if (!consume_otrigger(g, food, ch, consume_cmd::eat)) /* check trigger */
    return;

  auto sweet = std::ranges::find_if(CANDIES, [&](const candy &c) { return c.vnum == obj_get_vnum(g, food); });
  if (sweet != std::end(CANDIES)) {
    incCurLFPercent(g, ch, 0.01, true);
    incCurSTPercent(g, ch, sweet->stamina, true);
    if (obj_extra_flagged(food, extra_flag::forged)) {
      send_to_char(g, ch, "This is a forgery. You gain nothing!\r\n");
    } else {
      send_to_char(g, ch, "You feel more energetic!\r\n");
      majin_gain(g, ch, *sweet);
    }
    extract_obj(g, food);
    return;
  }

  if (char_get_conditions(g, ch, HUNGER) >= 48) {
    full(g, ch);
    return;
  }
  if (subcmd == SCMD_EAT) {
    act(g, "You eat some of $p.", false, ch, food, nullptr, act_to::actor);
    if (!obj_get_action_description(food).empty())
      act(g, obj_get_action_description(food), false, ch, food, nullptr, act_to::actor);
    act(g, "$n eats some of $p.", true, ch, food, nullptr, act_to::room);
  } else {
    tell_both(g, ch, food, "You nibble a little bit of $p.", "$n tastes a little bit of $p.");
    if (obj_get_value(food, VAL_FOOD_POISON)) {
      send_to_char(g, ch, "It has a sickening taste! Better not eat it...");
      return;
    }
  }

  /* Tasting takes 1 point of the food's nourishment, eating as much as there's room for */
  int foob             = 48 - char_get_conditions(g, ch, HUNGER);
  int maxfval          = MAX(1, obj_get_value(food, VAL_FOOD_MAXFOODVAL));
  int amount           = MAX(1, MIN(foob, (subcmd == SCMD_EAT ? obj_get_value(food, VAL_FOOD_FOODVAL) : 1)));
  double percent_eaten = (double)amount / (double)maxfval;

  gain_condition(g, ch, HUNGER, amount);
  if (char_get_foodr(ch) == 0 && subcmd != SCMD_TASTE) {
    incCurSTPercent(g, ch, 0.01 * (double)amount, true);
    char_set_foodr(ch, 2);
    if (obj_extra_flagged(food, extra_flag::only_wizard)) /* quirk: reads extra flag ONLY_WIZARD (33), meant item type YUM (33) */
      incCurSTPercent(g, ch, .25, true);
    send_to_char(g, ch, "You feel rejuvinated by it.\r\n");
  }
  if (subcmd != SCMD_TASTE)
    nourished(g, ch, food, percent_eaten);
  if (char_get_conditions(g, ch, HUNGER) >= 48)
    full(g, ch);

  if (obj_get_value(food, VAL_FOOD_POISON) && !char_adm_flagged(ch, admin_flag::nopoison)) {
    send_to_char(g, ch, "Oops, that tasted rather strange!\r\n");
    act(g, "$n coughs and utters some strange sounds.", false, ch, nullptr, nullptr, act_to::room);
    poison(g, ch, amount * 2);
  }
  obj_set_value(food, VAL_FOOD_FOODVAL, obj_get_value(food, VAL_FOOD_FOODVAL) - amount);
  if (obj_get_value(food, VAL_FOOD_FOODVAL) <= 0) {
    send_to_char(g, ch, "You finish the last bite.\r\n");
    extract_obj(g, food);
  }
}

namespace {
  /* A container's liquid gone, and its poison with it */
  void emptied(GameWorld *g, struct obj_data *obj)
  {
    name_from_drinkcon(g, obj);
    obj_set_value(obj, VAL_DRINKCON_HOWFULL, 0);
    obj_set_value(obj, VAL_DRINKCON_LIQUID, 0);
    obj_set_value(obj, VAL_DRINKCON_POISON, 0);
  }

  /* pour <from> out */
  void pour_out(GameWorld *g, struct char_data *ch, struct obj_data *from_obj)
  {
    if (obj_get_value(from_obj, VAL_DRINKCON_CAPACITY) <= 0) {
      send_to_char(g, ch, "You can't possibly pour that container out!\r\n");
      return;
    }
    act(g, "$n empties $p.", true, ch, from_obj, nullptr, act_to::room);
    act(g, "You empty $p.", false, ch, from_obj, nullptr, act_to::actor);
    weight_change_object(g, from_obj, -obj_get_value(from_obj, VAL_DRINKCON_HOWFULL)); /* Empty */
    emptied(g, from_obj);
  }

  /* Where to pour from, for pour: something carried */
  struct obj_data *pour_from(GameWorld *g, struct char_data *ch, std::string &arg1)
  {
    if (arg1.empty()) {
      send_to_char(g, ch, "From what do you want to pour?\r\n");
      return nullptr;
    }
    struct obj_data *from_obj = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch));
    if (!from_obj) {
      send_to_char(g, ch, "You can't find it!\r\n");
      return nullptr;
    }
    if (obj_get_type_flag(from_obj) != ITEM_DRINKCON) {
      send_to_char(g, ch, "You can't pour from that!\r\n");
      return nullptr;
    }
    return from_obj;
  }

  /* What to fill, and from what, for fill: something carried, from a fountain in the room */
  bool fill_from(GameWorld *g, struct char_data *ch, std::string &arg1, std::string &arg2, struct obj_data *&to_obj, struct obj_data *&from_obj)
  {
    if (arg1.empty()) {
      send_to_char(g, ch, "What do you want to fill?  And what are you filling it from?\r\n");
      return false;
    }
    if (!(to_obj = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You can't find it!\r\n");
      return false;
    }
    if (obj_get_type_flag(to_obj) != ITEM_DRINKCON) {
      act(g, "You can't fill $p!", false, ch, to_obj, nullptr, act_to::actor);
      return false;
    }
    if (arg2.empty()) {
      act(g, "What do you want to fill $p from?", false, ch, to_obj, nullptr, act_to::actor);
      return false;
    }
    if (!(from_obj = get_obj_in_list_vis(g, ch, arg2, nullptr, room_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "There doesn't seem to be {} {} here.\r\n", AN(arg2), arg2);
      return false;
    }
    /* quirk: a broken thing that isn't a fountain can be filled from */
    if (obj_get_type_flag(from_obj) != ITEM_FOUNTAIN && !obj_extra_flagged(from_obj, extra_flag::broken)) {
      act(g, "You can't fill something from $p.", false, ch, from_obj, nullptr, act_to::actor);
      return false;
    }
    if (obj_get_type_flag(from_obj) == ITEM_FOUNTAIN && obj_extra_flagged(from_obj, extra_flag::broken)) {
      act(g, "You can't fill something from a broken fountain.", false, ch, from_obj, nullptr, act_to::actor);
      return false;
    }
    return true;
  }
} // namespace

ACMD(do_pour)
{
  std::string_view rest = argument;
  std::string arg1      = take_arg(rest);
  std::string arg2      = take_arg(rest);
  struct obj_data *from_obj = nullptr, *to_obj = nullptr;

  if (subcmd == SCMD_POUR && !(from_obj = pour_from(g, ch, arg1)))
    return;
  if (subcmd == SCMD_FILL && !fill_from(g, ch, arg1, arg2, to_obj, from_obj))
    return;
  if (obj_get_value(from_obj, VAL_DRINKCON_HOWFULL) == 0) {
    act(g, "The $p is empty.", false, ch, from_obj, nullptr, act_to::actor);
    return;
  }
  if (subcmd == SCMD_POUR) {
    if (arg2.empty()) {
      send_to_char(g, ch, "Where do you want it?  Out or in what?\r\n");
      return;
    }
    if (str_eq_nocase(arg2, "out")) {
      pour_out(g, ch, from_obj);
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(g, ch, arg2, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You can't find it!\r\n");
      return;
    }
    if (obj_get_type_flag(to_obj) != ITEM_DRINKCON && obj_get_type_flag(to_obj) != ITEM_FOUNTAIN) {
      send_to_char(g, ch, "You can't pour anything into that.\r\n");
      return;
    }
  }
  if (to_obj == from_obj) {
    send_to_char(g, ch, "A most unproductive effort.\r\n");
    return;
  }
  if (obj_get_value(to_obj, VAL_DRINKCON_HOWFULL) != 0 && obj_get_value(to_obj, VAL_DRINKCON_LIQUID) != obj_get_value(from_obj, VAL_DRINKCON_LIQUID)) {
    send_to_char(g, ch, "There is already another liquid in it!\r\n");
    return;
  }
  if (obj_get_value(to_obj, VAL_DRINKCON_CAPACITY) < 0 || obj_get_value(to_obj, VAL_DRINKCON_HOWFULL) >= obj_get_value(to_obj, VAL_DRINKCON_CAPACITY)) {
    send_to_char(g, ch, "There is no room for more.\r\n");
    return;
  }
  if (subcmd == SCMD_POUR) /* quirk: with no newline */
    send_to_char(g, ch, "You pour the {} into the {}.", drinks[obj_get_value(from_obj, VAL_DRINKCON_LIQUID)], arg2);
  if (subcmd == SCMD_FILL) {
    act(g, "You gently fill $p from $P.", false, ch, to_obj, from_obj, act_to::actor);
    act(g, "$n gently fills $p from $P.", true, ch, to_obj, from_obj, act_to::room);
  }

  /* New alias, and the same liquid */
  if (obj_get_value(to_obj, VAL_DRINKCON_HOWFULL) == 0)
    name_to_drinkcon(g, to_obj, obj_get_value(from_obj, VAL_DRINKCON_LIQUID));
  obj_set_value(to_obj, VAL_DRINKCON_LIQUID, obj_get_value(from_obj, VAL_DRINKCON_LIQUID));

  /* Then how much to pour: as much as there's room for, or all there is */
  int amount = 0;
  if (obj_get_value(from_obj, VAL_DRINKCON_CAPACITY) > 0) {
    amount = obj_get_value(to_obj, VAL_DRINKCON_CAPACITY) - obj_get_value(to_obj, VAL_DRINKCON_HOWFULL);
    obj_set_value(from_obj, VAL_DRINKCON_HOWFULL, obj_get_value(from_obj, VAL_DRINKCON_HOWFULL) - amount);
    obj_set_value(to_obj, VAL_DRINKCON_HOWFULL, obj_get_value(to_obj, VAL_DRINKCON_CAPACITY));
    if (obj_get_value(from_obj, VAL_DRINKCON_HOWFULL) < 0) { /* There was too little */
      obj_set_value(to_obj, VAL_DRINKCON_HOWFULL, obj_get_value(to_obj, VAL_DRINKCON_HOWFULL) + obj_get_value(from_obj, VAL_DRINKCON_HOWFULL));
      amount += obj_get_value(from_obj, VAL_DRINKCON_HOWFULL);
      emptied(g, from_obj);
    }
  } else {
    obj_set_value(to_obj, VAL_DRINKCON_HOWFULL, obj_get_value(to_obj, VAL_DRINKCON_CAPACITY));
  }

  /* Then the poison boogie, and the weight boogie for what isn't eternal */
  obj_set_value(to_obj, VAL_DRINKCON_POISON, (obj_get_value(to_obj, VAL_DRINKCON_POISON) || obj_get_value(from_obj, VAL_DRINKCON_POISON)));
  if (obj_get_value(from_obj, VAL_DRINKCON_CAPACITY) > 0)
    weight_change_object(g, from_obj, -amount);
  weight_change_object(g, to_obj, amount); /* Add weight */
}

/* wear, wield, grab and remove */

namespace {
  /* By where it's worn: what's said to the room, and to the wearer */
  constexpr auto WEAR_MESSAGES = std::to_array<std::array<std::string_view, 2>>({
      {"$n lights $p and holds it.",                "You light $p and hold it."                 },
      {"$n slides $p on to $s right ring finger.",  "You slide $p on to your right ring finger."},
      {"$n slides $p on to $s left ring finger.",   "You slide $p on to your left ring finger." },
      {"$n wears $p around $s neck.",               "You wear $p around your neck."             },
      {"$n wears $p around $s neck.",               "You wear $p around your neck."             },
      {"$n wears $p on $s body.",                   "You wear $p on your body."                 },
      {"$n wears $p on $s head.",                   "You wear $p on your head."                 },
      {"$n puts $p on $s legs.",                    "You put $p on your legs."                  },
      {"$n wears $p on $s feet.",                   "You wear $p on your feet."                 },
      {"$n puts $p on $s hands.",                   "You put $p on your hands."                 },
      {"$n wears $p on $s arms.",                   "You wear $p on your arms."                 },
      {"$n straps $p around $s arm as a shield.",   "You start to use $p as a shield."          },
      {"$n wears $p about $s body.",                "You wear $p around your body."             },
      {"$n wears $p around $s waist.",              "You wear $p around your waist."            },
      {"$n puts $p on around $s right wrist.",      "You put $p on around your right wrist."    },
      {"$n puts $p on around $s left wrist.",       "You put $p on around your left wrist."     },
      {"$n wields $p.",                             "You wield $p."                             },
      {"$n grabs $p.",                              "You grab $p."                              },
      {"$n wears $p on $s back.",                   "You wear $p on your back."                 },
      {"$n puts $p in $s right ear.",               "You put $p in your right ear."             },
      {"$n puts $p in $s left ear.",                "You put $p in your left ear."              },
      {"$n wears $p as a cape.",                    "You wear $p as a cape."                    },
      {"$n covers $s left eye with $p.",            "You wear $p over your left eye."           },
  });

  /*
   * By where it's worn, what it takes to be worn there: wear_flag::take is used for objects that do not require
   * special bits to be put into that position (e.g. you can hold any object, not just an object with a HOLD bit.)
   */
  constexpr auto WEAR_BITVECTORS = std::to_array<wear_flag>({
      wear_flag::take, wear_flag::finger, wear_flag::finger, wear_flag::neck,
      wear_flag::neck, wear_flag::body, wear_flag::head, wear_flag::legs,
      wear_flag::feet, wear_flag::hands, wear_flag::arms, wear_flag::shield,
      wear_flag::about, wear_flag::waist, wear_flag::wrist, wear_flag::wrist,
      wear_flag::take, wear_flag::take, wear_flag::pack, wear_flag::ear,
      wear_flag::ear, wear_flag::sh, wear_flag::eye});

  static constexpr auto ALREADY_WEARING = std::to_array<std::string_view>({
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
      "You're already wearing something as a scouter.\r\n"});

  /* Where a thing goes when no place is named: the first of these its wear flags and the body allow */
  struct wear_place {
    wear_flag flag;
    int where;
  };

  constexpr auto NATURAL_PLACES = std::to_array<wear_place>({
      {wear_flag::finger, WEAR_FINGER_R},
      {wear_flag::neck,   WEAR_NECK_1  },
      {wear_flag::body,   WEAR_BODY    },
      {wear_flag::head,   WEAR_HEAD    },
      {wear_flag::legs,   WEAR_LEGS    },
      {wear_flag::feet,   WEAR_FEET    },
      {wear_flag::hands,  WEAR_HANDS   },
      {wear_flag::arms,   WEAR_ARMS    },
      {wear_flag::shield, WEAR_WIELD2  },
      {wear_flag::about,  WEAR_ABOUT   },
      {wear_flag::waist,  WEAR_WAIST   },
      {wear_flag::wrist,  WEAR_WRIST_R },
      {wear_flag::hold,   WEAR_WIELD2  },
      {wear_flag::pack,   WEAR_BACKPACK},
      {wear_flag::ear,    WEAR_EAR_R   },
      {wear_flag::sh,     WEAR_SH      },
      {wear_flag::eye,    WEAR_EYE     },
  });

  void wear_message(GameWorld *g, struct char_data *ch, struct obj_data *obj, int where)
  {
    act(g, WEAR_MESSAGES[where][0], true, ch, obj, nullptr, act_to::room);
    act(g, WEAR_MESSAGES[where][1], false, ch, obj, nullptr, act_to::actor);
  }

  bool two_handed(struct char_data *ch, struct obj_data *obj)
  {
    return obj_extra_flagged(obj, extra_flag::two_handed) || wield_type(get_size(ch), obj) == WIELD_TWOHAND;
  }

  /* How many hands ch's weapons take */
  int hands(struct char_data *ch)
  {
    int x = 0;
    for (int pos : {WEAR_WIELD1, WEAR_WIELD2})
      if (struct obj_data *held = char_get_equipment(ch, pos))
        x += two_handed(ch, held) ? 2 : 1;
    return x;
  }

  int64_t max_carry_weight(GameWorld *g, struct char_data *ch)
  {
    return (getEffMaxPL(g, ch) / 200) + (char_get_aff_abils_str(ch) * 50);
  }

  /* Worn where it goes, with a warning for armor ch isn't used to */
  void wear_warned(GameWorld *g, struct char_data *ch, struct obj_data *obj, int where)
  {
    if (!is_proficient_with_armor(ch, obj_get_value(obj, VAL_ARMOR_SKILL)) && obj_get_type_flag(obj) == ITEM_ARMOR)
      send_to_char(g, ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
    perform_wear(g, ch, obj, where);
  }

  /* Whether a thing can be worn, saying why not */
  bool unworn_why(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    if (obj_extra_flagged(obj, extra_flag::broken))
      send_to_char(g, ch, "But it seems to be broken!\r\n");
    else if (obj_extra_flagged(obj, extra_flag::forged))
      send_to_char(g, ch, "But it seems to be fake!\r\n");
    else if (char_get_level(ch) < obj_get_level(obj))
      send_to_char(g, ch, "You are not experienced enough to use that.\r\n");
    else
      return false;
    return true;
  }

  /* wear all */
  void wear_all(GameWorld *g, struct char_data *ch)
  {
    int where, items_worn = 0;
    for (obj_data *thing : char_inventory(g, ch)) {
      if (!char_can_see_obj(g, ch, thing) || (where = find_eq_pos(g, ch, thing)) < 0)
        continue;
      if (char_get_level(ch) < obj_get_level(thing)) {
        act(g, "$p: you are not experienced enough to use that.", false, ch, thing, nullptr, act_to::actor);
        send_to_char(g, ch, "You need to be at least {:d} level to use it.\r\n", obj_get_level(thing));
      } else if (obj_extra_flagged(thing, extra_flag::broken)) {
        act(g, "$p: it seems to be broken.", false, ch, thing, nullptr, act_to::actor);
      } else if (obj_extra_flagged(thing, extra_flag::forged)) {
        act(g, "$p: it seems to be fake...", false, ch, thing, nullptr, act_to::actor);
      } else {
        items_worn++;
        wear_warned(g, ch, thing, where);
      }
    }
    if (!items_worn)
      send_to_char(g, ch, "You don't seem to have anything wearable.\r\n");
  }

  /* wear all.<things> */
  void wear_all_of(GameWorld *g, struct char_data *ch, std::string &what)
  {
    struct obj_data *obj = get_obj_in_list_vis(g, ch, what, nullptr, char_get_inventory(ch));
    if (!obj) {
      send_to_char(g, ch, "You don't seem to have any {}s.\r\n", what);
      return;
    }
    if (char_get_level(ch) < obj_get_level(obj)) {
      send_to_char(g, ch, "You are not experienced enough to use that.\r\n");
      return;
    }
    for (obj_data *match : obj_list_from(g, char_get_inventory(ch), obj)) {
      if (!obj_named_and_visible(g, ch, what, match))
        continue;
      int where = find_eq_pos(g, ch, match);
      if (where >= 0)
        wear_warned(g, ch, match, where);
      else
        act(g, "You can't wear $p.", false, ch, match, nullptr, act_to::actor);
    }
  }
} // namespace

void perform_wear(GameWorld *g, struct char_data *ch, struct obj_data *obj, int where)
{
  /* first, make sure that the wear position is valid. */
  if (!obj_wear_flagged(obj, WEAR_BITVECTORS[where])) {
    act(g, "You can't wear $p there.", false, ch, obj, nullptr, act_to::actor);
    return;
  }
  /* do they even have that wear slot by race? */
  if (!char_body_flagged(ch, where)) {
    send_to_char(g, ch, "Seems like your body type doesn't really allow that.\r\n");
    return;
  }
  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ((where == WEAR_FINGER_R || where == WEAR_NECK_1 || where == WEAR_WRIST_R || where == WEAR_EAR_R || where == WEAR_WIELD1) && char_get_equipment(ch, where))
    where++;

  /* checks for 2H sanity */
  if ((two_handed(ch, obj) && hands(ch) > 0) || (where == WEAR_WIELD2 && char_plr_flagged(ch, player_flag::thandw)) ||
      ((where == WEAR_WIELD1 || where == WEAR_WIELD2) && hands(ch) > 1)) {
    send_to_char(g, ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }
  if (char_get_equipment(ch, where)) {
    send_to_char(g, ch, "{}", ALREADY_WEARING[where]);
    return;
  }
  /* See if a trigger disallows it */
  if (!wear_otrigger(g, obj, ch, where) || obj_get_carried_by(obj) != ch)
    return;
  if (obj_get_type_flag(obj) == ITEM_WEAPON && obj_extra_flagged(obj, extra_flag::custom) && char_get_level(ch) < 20) {
    send_to_char(g, ch, "You are not experienced enough to hold that.\r\n");
    return;
  }
  wear_message(g, ch, obj, where);
  obj_from_char(g, obj);
  equip_char(g, ch, obj, where);
}

int find_eq_pos(GameWorld *g, struct char_data *ch, struct obj_data *obj, std::string_view arg)
{
  static constexpr auto keywords_entries = std::to_array<std::string_view>({
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
      "scouter"});
  static constexpr name_table keywords(keywords_entries);

  if (arg.empty()) {
    auto place = std::ranges::find_if(NATURAL_PLACES, [&](const wear_place &p) { return obj_wear_flagged(obj, p.flag) && char_body_flagged(ch, p.where); });
    return place == std::end(NATURAL_PLACES) ? -1 : place->where;
  }
  int where = search_block(arg, keywords, false);
  if (where < 0) {
    /* lowercased, unless it starts with '!' (which search_block() refuses before lowercasing) */
    std::string said(arg);
    if (said[0] != '!')
      for (char &c : said)
        c = tolower((unsigned char)c);
    send_to_char(g, ch, "'{}'?  What part of your body is THAT?\r\n", said);
  }
  return where;
}

ACMD(do_wear)
{
  std::string_view rest = argument;
  std::string arg1      = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (arg1.empty()) {
    send_to_char(g, ch, "Wear what?\r\n");
    return;
  }
  find_dots dotmode = find_all_dots(arg1);
  if (!arg2.empty() && dotmode != find_dots::indiv) {
    send_to_char(g, ch, "You can't specify the same body location for more than one item!\r\n");
    return;
  }
  if (dotmode == find_dots::all) {
    wear_all(g, ch);
    return;
  }
  if (dotmode == find_dots::alldot) {
    if (arg1.empty())
      send_to_char(g, ch, "Wear all of what?\r\n");
    else
      wear_all_of(g, ch, arg1);
    return;
  }
  struct obj_data *obj = get_obj_in_list_vis(g, ch, arg1, nullptr, char_get_inventory(ch));
  if (!obj) {
    send_to_char(g, ch, "You don't seem to have {} {}.\r\n", AN(arg1), arg1);
    return;
  }
  if (unworn_why(g, ch, obj))
    return;
  int where = find_eq_pos(g, ch, obj, arg2);
  if (where >= 0)
    wear_warned(g, ch, obj, where);
  else if (arg2.empty())
    act(g, "You can't wear $p.", false, ch, obj, nullptr, act_to::actor);
}

ACMD(do_wield)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Wield what?\r\n");
    return;
  }
  struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
  if (!obj) {
    send_to_char(g, ch, "You don't seem to have {} {}.\r\n", AN(arg), arg);
    return;
  }
  if (!obj_wear_flagged(obj, wear_flag::wield)) {
    send_to_char(g, ch, "You can't wield that.\r\n");
    return;
  }
  if (obj_get_weight(obj) > max_carry_weight(g, ch)) {
    send_to_char(g, ch, "It's too heavy for you to use.\r\n");
    return;
  }
  if (unworn_why(g, ch, obj))
    return;
  if (char_plr_flagged(ch, player_flag::thandw)) {
    send_to_char(g, ch, "You are holding a weapon with two hands right now!\r\n");
    return;
  }
  /* quirk: warns of an unfamiliar weapon only when it's armor */
  if (!char_is_npc(ch) && !is_proficient_with_weapon(ch, obj_get_value(obj, VAL_WEAPON_SKILL)) && obj_get_type_flag(obj) == ITEM_ARMOR)
    send_to_char(g, ch, "You have no proficiency with this type of weapon.\r\nYour attack accuracy will be greatly reduced.\r\n");
  perform_wear(g, ch, obj, WEAR_WIELD1);
}

ACMD(do_grab)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Hold what?\r\n");
    return;
  }
  struct obj_data *obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch));
  if (!obj) {
    send_to_char(g, ch, "You don't seem to have {} {}.\r\n", AN(arg), arg);
    return;
  }
  if (char_get_level(ch) < obj_get_level(obj)) {
    send_to_char(g, ch, "You are not experienced enough to use that.\r\n");
    return;
  }
  if (char_plr_flagged(ch, player_flag::thandw)) {
    send_to_char(g, ch, "You are wielding a weapon with both hands currently.\r\n");
    return;
  }
  if (obj_get_type_flag(obj) == ITEM_LIGHT) {
    perform_wear(g, ch, obj, WEAR_WIELD2);
    /* quirk: lit, or not, though it couldn't be held */
    if (obj_get_value(obj, VAL_LIGHT_HOURS) != 0)
      tell_both(g, ch, obj, "@wYou light $p@w.", "@C$n@w lights $p@w.");
    else
      tell_both(g, ch, obj, "@wYou try to light $p@w but it is burnt out.", "@C$n@w tries to light $p@w but nothing happens.");
    return;
  }
  int type = obj_get_type_flag(obj);
  if (!obj_wear_flagged(obj, wear_flag::hold) && type != ITEM_WAND && type != ITEM_STAFF && type != ITEM_SCROLL && type != ITEM_POTION)
    send_to_char(g, ch, "You can't hold that.\r\n");
  else
    perform_wear(g, ch, obj, WEAR_WIELD2);
}

void perform_remove(GameWorld *g, struct char_data *ch, int pos)
{
  int64_t previous     = getCurPL(g, ch);
  struct obj_data *obj = char_get_equipment(ch, pos);

  if (!obj) {
    mud_log("SYSERR: perform_remove: bad pos {:d} passed.", pos);
    return;
  }
  if (obj_extra_flagged(obj, extra_flag::nodrop) && char_get_admlevel(ch) < 1) {
    act(g, "You can't remove $p, it must be CURSED!", false, ch, obj, nullptr, act_to::actor);
    return;
  }
  if (char_get_carry_items(ch) >= char_can_carry_n(ch)) {
    act(g, "$p: your arms are full!", false, ch, obj, nullptr, act_to::actor);
    return;
  }
  if (!remove_otrigger(g, obj, ch))
    return;
  if (pos == WEAR_WIELD1 && char_plr_flagged(ch, player_flag::thandw))
    char_plr_set(ch, player_flag::thandw, false);
  obj_to_char(g, unequip_char(g, ch, pos), ch);
  tell_both(g, ch, obj, "You stop using $p.", "$n stops using $p.");
  if (previous > getCurPL(g, ch))
    act(g, str_format("@RYour powerlevel has dropped from removing $p@R! @D[@r-{}@D]\r\n", add_commas(previous - getCurPL(g, ch))), false, ch, obj, nullptr, act_to::actor);
}

ACMD(do_remove)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Remove what?\r\n");
    return;
  }
  /* A board's message, by its number, first */
  auto is_board          = [&](obj_data *o) { return obj_get_type_flag(o) == ITEM_BOARD; };
  struct obj_data *board = find_first(char_inventory(g, ch), is_board);
  if (!board)
    board = find_first(room_contents(g, char_get_in_room(ch)), is_board);
  int msg;
  if (board && isdigit(arg[0]) && (msg = parse_int(arg))) {
    remove_board_msg(g, obj_get_vnum(g, board), ch, msg);
    return;
  }

  find_dots dotmode = find_all_dots(arg);
  if (dotmode == find_dots::alldot && arg.empty()) {
    send_to_char(g, ch, "Remove all of what?\r\n");
    return;
  }
  if (dotmode == find_dots::all || dotmode == find_dots::alldot) {
    bool found = false;
    for (int i = 0; i < NUM_WEARS; i++) {
      struct obj_data *worn = char_get_equipment(ch, i);
      if (worn && (dotmode == find_dots::all || (char_can_see_obj(g, ch, worn) && isname(arg, obj_get_name(worn))))) {
        perform_remove(g, ch, i);
        found = true;
      }
    }
    if (found)
      return;
    if (dotmode == find_dots::all)
      send_to_char(g, ch, "You're not using anything.\r\n");
    else
      send_to_char(g, ch, "You don't seem to be using any {}s.\r\n", arg);
    return;
  }
  int i = get_obj_pos_in_equip_vis(g, ch, arg, nullptr, char_get_equipment_array(ch));
  if (i < 0)
    send_to_char(g, ch, "You don't seem to be using {} {}.\r\n", AN(arg), arg);
  else
    perform_remove(g, ch, i);
}
