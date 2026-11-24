/* Scenes for test_techniques: see scene_kit.hpp. These are act.item.cpp's: refuel to remove. */
#include "scene_kit.hpp"

#include "dbat/act.item.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/auction.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/auction.hpp"
#include "dbat/interpreter.hpp"

namespace {
  /* item70 made into something else */
  struct made {
    const char *name                                  = "pebble";
    obj_vnum vnum                                     = 70;
    int type                                          = -1;
    int64_t weight                                    = 1;
    std::initializer_list<std::pair<int, int>> values = {};
    std::initializer_list<flag_type_of<ITEM_GLOW>> flags                  = {}; /* extra flags */
    std::initializer_list<flag_type_of<ITEM_WEAR_TAKE>> wear = {}; /* wear flags */
  };

  struct obj_data *make(GameWorld *g, const made &m)
  {
    struct obj_data *obj = read_object(g, 70);
    if (m.vnum != 70)
      renumber(g, obj, m.vnum);
    obj_set_name(obj, m.name);
    obj_set_weight(obj, m.weight);
    if (m.type >= 0)
      obj_set_type_flag(obj, m.type);
    for (auto [i, v] : m.values)
      obj_set_value(obj, i, v);
    for (flag_type_of<ITEM_GLOW> flag : m.flags)
      obj_extra_set(obj, flag, true);
    for (flag_type_of<ITEM_WEAR_TAKE> flag : m.wear)
      obj_wear_set(obj, flag, true);
    return obj;
  }

  /* ... carrying it */
  struct obj_data *carry(GameWorld *g, struct char_data *ch, const made &m)
  {
    struct obj_data *obj = make(g, m);
    obj_to_char(g, obj, ch);
    return obj;
  }

  /* ... by it, in the arena or the room asked */
  struct obj_data *here(GameWorld *g, const made &m, room_vnum room = ARENA)
  {
    struct obj_data *obj = make(g, m);
    obj_to_room(g, obj, room);
    return obj;
  }

  /* ... it in that container */
  struct obj_data *inside(GameWorld *g, struct obj_data *cont, const made &m)
  {
    struct obj_data *obj = make(g, m);
    obj_to_obj(g, obj, cont);
    return obj;
  }

  /* ... wearing it there */
  struct obj_data *worn(GameWorld *g, struct char_data *ch, const made &m, int where)
  {
    struct obj_data *obj = make(g, m);
    equip_char(g, ch, obj, where);
    return obj;
  }

  void pebbles(GameWorld *g, struct char_data *ch, int count, const char *name = "pebble")
  {
    for (int i = 0; i < count; i++)
      carry(g, ch, {.name = name});
  }

  /* ... in a room of so many more things, in a house */
  void stuffed(GameWorld *g, int count)
  {
    room_set_flag(g, ARENA, ROOM_HOUSE, true);
    for (int i = 0; i < count; i++)
      here(g, {.name = "clutter"});
  }

  /* ... carrying nothing, not even the souls players start with */
  void bare(GameWorld *g, struct char_data *ch)
  {
    for (obj_data *obj : char_inventory(g, ch))
      extract_obj(g, obj);
  }

  /* ... the arena's floor swept of everything */
  void swept(GameWorld *g)
  {
    for (obj_data *obj : room_contents(g, ARENA))
      extract_obj(g, obj);
  }

  /* ... the training dummy, with arms to handle things, which the setup makes do something */
  struct char_data *dummy(GameWorld *g)
  {
    for (char_data *c : room_people(g, ARENA))
      if (char_is_npc(c) && isname("dummy", char_get_alias(c))) {
        char_mob_set(c, MOB_LARM, true);
        char_mob_set(c, MOB_RARM, true);
        return c;
      }
    return nullptr;
  }

  void zenni_here(GameWorld *g, int amount) { obj_to_room(g, create_money(g, amount), ARENA); }

  void armless(struct char_data *ch)
  {
    char_set_limb_condition(ch, 0, 0);
    char_set_limb_condition(ch, 1, 0);
  }

  void unarmed(GameWorld *g, struct char_data *ch)
  {
    if (char_get_equipment(ch, WEAR_WIELD1))
      extract_obj(g, unequip_char(g, ch, WEAR_WIELD1));
  }

  void conditions(GameWorld *g, struct char_data *ch, int hunger, int thirst, int drunk = 0)
  {
    char_set_conditions(g, ch, HUNGER, hunger);
    char_set_conditions(g, ch, THIRST, thirst);
    char_set_conditions(g, ch, DRUNK, drunk);
    char_set_foodr(ch, 0);
  }

  /* garden: a ripe plant of this vnum in soil of this quality, by a gardener of this skill with clippers */
  void ripe(GameWorld *g, struct char_data *att, int skill, obj_vnum plant, int soil, bool stocked = true)
  {
    struct obj_data *p = gardening(g, att, skill, plant);
    obj_set_value(p, VAL_MATURITY, 6);
    obj_set_value(p, VAL_MAXMATURE, 6);
    obj_set_value(p, VAL_SOILQ, soil);
    obj_set_value(carries_as(g, att, 253), VAL_ALL_HEALTH, 5);
    if (stocked)
      stock(g, {1131, 17212, 17213, 17215, 17217, 17219, 17221, 17223, 17225, 17227, 3703});
  }

  /* refuel: a ship's controls of this vnum, with this much fuel, and canisters of these weights */
  void ship(GameWorld *g, struct char_data *att, obj_vnum controls, int fuel, std::initializer_list<int> canisters = {})
  {
    here(g, {.name = "controls", .vnum = controls, .type = ITEM_CONTROL, .values = {{2, fuel}}});
    for (int weight : canisters)
      carry(g, att, {.name = "canister", .vnum = 17290, .weight = weight});
  }

  /* pack: a house of this vnum, its rooms with something in each, and its key if asked */
  struct obj_data *house(GameWorld *g, struct char_data *att, obj_vnum vnum, bool keyed, int type = ITEM_VEHICLE)
  {
    struct obj_data *home = here(g, {.name = "house", .vnum = vnum, .type = type});
    obj_vnum key          = vnum == 18802 ? 18800 : vnum - 1;
    for (room_vnum room = key; room < key + 4; room++) {
      room_stock(g, room);
      here(g, {.name = "furnishing"}, room);
    }
    if (keyed)
      carry(g, att, {.name = "key", .vnum = key});
    return home;
  }

  /* deploy: a capsule of this vnum, a place for a house, and the house sites from `first`, `taken` of them taken */
  void capsule(GameWorld *g, struct char_data *att, obj_vnum vnum, int rpp = 10, int sect = SECT_FIELD)
  {
    carry(g, att, {.name = "capsule", .vnum = vnum});
    char_set_rp(att, rpp);
    room_set_sector_type(g, ARENA, sect);
  }

  void sites(GameWorld *g, room_vnum first, int step, int taken)
  {
    for (room_vnum room = first; room < first + 100; room += step) {
      room_stock(g, room);
      room_stock(g, room + 1);
      stock(g, {room});
      if (taken-- > 0)
        here(g, {.name = "door", .vnum = 18801}, room);
    }
  }

  /* ... a furniture capsule of this vnum, in a house unless `where` says otherwise */
  void furniture(GameWorld *g, struct char_data *att, obj_vnum vnum, std::initializer_list<flag_type_of<ROOM_DARK>> flags = {ROOM_HOUSE})
  {
    carry(g, att, {.name = "capsule", .vnum = vnum});
    for (flag_type_of<ROOM_DARK> flag : flags)
      room_set_flag(g, ARENA, flag, true);
    stock(g, {19090, 19091, 19092, 19093, 11});
  }

  /* auction: selling a rock for this much (said before the line, and the pulses of it played `calls` times) */
  struct obj_data *rock(GameWorld *g, struct char_data *ch, std::initializer_list<flag_type_of<ITEM_GLOW>> flags = {})
  {
    return carry(g, ch, {.name = "rock", .flags = flags});
  }

  void auction(GameWorld *g, struct char_data *ch, const char *what) { do_auction(g, ch, what, 0, 0); }

  void calls(GameWorld *g, int times)
  {
    for (int i = 0; i < times; i++)
      check_auction(g);
  }

  /* bid: something on auction in the auction house, owned and bid on so, put up `age` seconds ago */
  struct obj_data *lot(GameWorld *g, int32_t owner, int32_t bidder, int bid, time_t age, const made &m = {.name = "lot"})
  {
    struct obj_data *obj = here(g, m, 80);
    obj_set_aucter(obj, owner);
    obj_set_curBidder(obj, bidder);
    obj_set_bid(obj, bid);
    obj_set_aucTime(obj, game_now(g) - age);
    return obj;
  }

  /* ... one of every kind the list shows */
  void lots(GameWorld *g, struct char_data *att, struct char_data *victim)
  {
    lot(g, char_get_id(victim), -1, 100, 60);
    lot(g, char_get_id(victim), char_get_id(att), 200, 60);
    lot(g, 99999, char_get_id(victim), 300, 90000);
    lot(g, char_get_id(victim), -1, 400, 90000);
    here(g, {.name = "lot"}, 80); /* not auctioned */
    lot(g, char_get_id(victim), char_get_id(att), 500, 86400);
  }

  void bidder(GameWorld *g, struct char_data *att, int gold = 1000)
  {
    scouter_on(g, att);
    char_set_gold(att, gold);
  }

  void appraiser(GameWorld *g, struct char_data *att, int skill = 100)
  {
    scouter_on(g, att);
    char_set_skills_base(att, SKILL_APPRAISE, skill);
  }

  /* build: a toolkit of this health; the chest's parts; a campfire, and a beacon, to make */
  void toolkit(GameWorld *g, struct char_data *att, int health = 100)
  {
    carry(g, att, {.name = "toolkit", .vnum = 386, .values = {{VAL_ALL_HEALTH, health}}});
  }

  void parts(GameWorld *g, struct char_data *att, int build = 100)
  {
    obj_to_char(g, read_object(g, 301), att);
    obj_to_char(g, read_object(g, 302), att);
    char_set_skills_base(att, SKILL_BUILD, build);
  }

  void named_proto(GameWorld *g, obj_vnum vnum, const char *name)
  {
    stock(g, {vnum});
    obj_set_name(obj_proto_of(g, vnum), name);
    obj_set_short_description(obj_proto_of(g, vnum), name);
  }

  void campfire(GameWorld *g, struct char_data *att, int survival = 50)
  {
    named_proto(g, 1611, "campfire");
    assemblyCreate(g, 1611, SCMD_BREW);
    assemblyAddComponent(g, 1611, 302, true, false);
    obj_to_char(g, read_object(g, 302), att);
    char_set_skills_base(att, SKILL_SURVIVAL, survival);
  }

  void beacon(GameWorld *g, struct char_data *att, int build)
  {
    named_proto(g, 5001, "signal beacon");
    assemblyCreate(g, 5001, SCMD_BREW);
    assemblyAddComponent(g, 5001, 301, true, false);
    obj_to_char(g, read_object(g, 301), att);
    char_set_skills_base(att, SKILL_BUILD, build);
  }

  /* ... a truffle, the chest worth this much, with bonuses at so many of its places */
  void truffle(GameWorld *g, struct char_data *att, int cost, int bonuses)
  {
    char_set_race(att, RACE_TRUFFLE);
    struct obj_data *proto = obj_proto_of(g, 300);
    obj_set_cost(proto, cost);
    for (int i = 0; i < bonuses; i++) {
      obj_get_affected(proto)[i].location = APPLY_STR;
      obj_get_affected(proto)[i].modifier = 1;
    }
  }

  /* put and get: a bag, of this capacity, flagged so */
  struct obj_data *bag(GameWorld *g, struct char_data *att, int capacity = 100, std::initializer_list<flag_type_of<ITEM_GLOW>> flags = {}, int closed = 0)
  {
    return carry(g, att, {.name = "bag", .type = ITEM_CONTAINER, .values = {{0, capacity}, {1, closed}}, .flags = flags});
  }

  struct obj_data *sheath(GameWorld *g, struct char_data *att, int holds, int swords)
  {
    struct obj_data *obj = make(g, {.name = "sheath", .type = ITEM_CONTAINER, .weight = holds, .values = {{0, 100}}, .flags = {ITEM_SHEATH}});
    for (int i = 0; i < swords; i++)
      obj_to_obj(g, read_object(g, 301), obj);
    obj_to_char(g, obj, att);
    return obj;
  }

  /* drink: a flask of this liquid, so full of so much */
  struct obj_data *flask(GameWorld *g, struct char_data *att, int liquid = LIQ_WATER, int full = 10, int capacity = 10,
                         std::initializer_list<flag_type_of<ITEM_GLOW>> flags = {}, int poison = 0, const char *name = "flask")
  {
    return carry(g, att, {.name = name, .type = ITEM_DRINKCON, .weight = 12, .values = {{0, capacity}, {1, full}, {2, liquid}, {3, poison}}, .flags = flags});
  }

  void thirsty(GameWorld *g, struct char_data *att, int thirst = 10, int hunger = 20, int drunk = 0)
  {
    conditions(g, att, hunger, thirst, drunk);
  }

  /* ... a wellspring, with a deep well of ki if asked, spent or nearly so */
  void wellspring(GameWorld *g, struct char_data *att, bool drained_all = true, bool deep = false)
  {
    char_set_skills_base(att, SKILL_WELLSPRING, 50);
    if (deep) {
      char_set_baseki(att, 1000000000);
      restoreVitals(att, false);
    }
    if (drained_all)
      drained(g, att);
    else
      decCurKI(g, att, 1, 0);
  }

  /* eat: food, so nourishing */
  struct obj_data *bread(GameWorld *g, struct char_data *att, int food = 10, std::initializer_list<flag_type_of<ITEM_GLOW>> flags = {}, int poison = 0,
                         int type = ITEM_FOOD, int64_t weight = 2)
  {
    return carry(g, att, {.name = "bread", .type = type, .weight = weight, .values = {{0, food}, {1, 10}, {2, 5}, {3, poison}, {6, 10}}, .flags = flags});
  }

  /* wear: something that goes there */
  struct obj_data *wearable(GameWorld *g, struct char_data *att, const char *name, flag_type_of<ITEM_WEAR_TAKE> wear, std::initializer_list<flag_type_of<ITEM_GLOW>> flags = {},
                            int level = 0, int type = -1)
  {
    struct obj_data *obj = make(g, {.name = name, .type = type, .flags = flags, .wear = {wear}});
    obj_set_level(obj, level);
    obj_to_char(g, obj, att);
    return obj;
  }

  /* ... one of everything */
  void wardrobe(GameWorld *g, struct char_data *att)
  {
    for (flag_type_of<ITEM_WEAR_TAKE> wear : {ITEM_WEAR_FINGER, ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS, ITEM_WEAR_FEET,
                     ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD, ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST,
                     ITEM_WEAR_HOLD, ITEM_WEAR_PACK, ITEM_WEAR_EAR, ITEM_WEAR_SH, ITEM_WEAR_EYE})
      wearable(g, att, "garment", wear);
  }

  const scene list[] = {
    /* garden: what harvest_plant yields */
    {"a master in rich soil, by ripe herbs", "garden seedling harvest", false, SETUP(ripe(g, att, 100, 250, 8);), false, 1},
    {"a great gardener in good soil, by ripe herbs", "garden seedling harvest", false, SETUP(ripe(g, att, 90, 250, 5);), false, 2},
    {"a good gardener in fair soil, by ripe herbs", "garden seedling harvest", false, SETUP(ripe(g, att, 80, 250, 3);), false, 2},
    {"a decent gardener in poor soil, by ripe herbs", "garden seedling harvest", false, SETUP(ripe(g, att, 50, 250, 1);), false, 4},
    {"a beginner in bare soil, by ripe herbs", "garden seedling harvest", false, SETUP(ripe(g, att, 40, 250, 0);), false, 6},
    {"a novice, by ripe herbs", "garden seedling harvest", false, SETUP(ripe(g, att, 30, 250, 0);), false, 6},
    {"a dabbler, by ripe herbs", "garden seedling harvest", false, SETUP(ripe(g, att, 20, 250, 0);), false, 8},
    {"a master, by a ripe bush", "garden seedling harvest", false, SETUP(ripe(g, att, 100, 1129, 0);), false, 1},
    {"a master, by a ripe vine", "garden seedling harvest", false, SETUP(ripe(g, att, 100, 17220, 0);), false, 2},
    {"a master, by a ripe tree", "garden seedling harvest", false, SETUP(ripe(g, att, 100, 17222, 0);), false, 2},
    {"a master, by a ripe weed", "garden seedling harvest", false, SETUP(ripe(g, att, 100, 17226, 0);), false, 1},
    {"a dabbler, by a ripe weed", "garden seedling harvest", false, SETUP(ripe(g, att, 29, 17226, 0);), false, 8},
    {"a master, by a ripe fern that bears nothing", "garden seedling harvest", false, SETUP(ripe(g, att, 100, 3702, 0, false);), false, 1},

    /* refuel */
    {"", "refuel", false, SETUP(), false, 1},
    {"at the controls, without fuel", "refuel", false, SETUP(ship(g, att, 44000, 0);), false, 1},
    {"at a small ship's full controls", "refuel", false, SETUP(ship(g, att, 44000, 300, {10});), false, 1},
    {"at a small ship's controls", "refuel", false, SETUP(ship(g, att, 44100, 0, {10});), false, 1},
    {"at a small ship's nearly full controls", "refuel", false, SETUP(ship(g, att, 44100, 290, {10});), false, 1},
    {"at a middling ship's controls", "refuel", false, SETUP(ship(g, att, 44300, 0, {10});), false, 1},
    {"at a big ship's controls", "refuel", false, SETUP(ship(g, att, 44600, 0, {10});), false, 1},
    {"at an odd ship's controls", "refuel", false, SETUP(ship(g, att, 45000, 0, {10});), false, 1},
    {"with two canisters", "refuel", false, SETUP(ship(g, att, 44000, 0, {1, 2});), false, 1},

    /* pack */
    {"", "pack", false, SETUP(), false, 1},
    {"", "pack table", false, SETUP(), false, 1},
    {"by a table", "pack table", false, SETUP(here(g, {.name = "table", .vnum = 19094}); stock(g, {19084});), false, 1},
    {"by a table that packs into nothing", "pack table", false, SETUP(here(g, {.name = "table", .vnum = 19094});), false, 1},
    {"by a cushion", "pack cushion", false, SETUP(here(g, {.name = "cushion", .vnum = 11});), false, 1},
    {"by a house", "pack house", false, SETUP(house(g, att, 18850, true);), false, 1},
    {"by a house", "pack house no", false, SETUP(house(g, att, 18850, true);), false, 1},
    {"by a house, keyless", "pack house yes", false, SETUP(house(g, att, 18850, false); pebbles(g, att, 1);), false, 1},
    {"by a house, with its key", "pack house yes", false, SETUP(house(g, att, 18850, true);), false, 1},
    {"by the first house, with its key", "pack house yes", false, SETUP(house(g, att, 18802, true);), false, 1},
    {"by a copied house, with its key", "pack house yes", false, SETUP(obj_extra_set(house(g, att, 18850, true), ITEM_DUPLICATE, true);), false, 1},
    {"by a big house, with its key", "pack house yes", false, SETUP(house(g, att, 18950, true);), false, 1},
    {"by a grand house, with its key", "pack house yes", false, SETUP(house(g, att, 19150, true);), false, 1},
    {"by an odd house, with its key", "pack house yes", false, SETUP(house(g, att, 19050, true);), false, 1},
    {"by a house that isn't a vehicle", "pack house yes", false, SETUP(house(g, att, 18850, true, ITEM_OTHER);), false, 1},
    {"by a rock", "pack rock", false, SETUP(here(g, {.name = "rock"});), false, 1},

    /* deploy */
    {"", "deploy", false, SETUP(), false, 1},
    {"with a capsule, short of rpp", "deploy", false, SETUP(capsule(g, att, 4, 9);), false, 1},
    {"with a capsule, indoors", "deploy", false, SETUP(capsule(g, att, 4, 10, SECT_INSIDE);), false, 1},
    {"with a capsule, at sea", "deploy", false, SETUP(capsule(g, att, 4, 10, SECT_WATER_NOSWIM);), false, 1},
    {"with a capsule, on a lake", "deploy", false, SETUP(capsule(g, att, 4, 10, SECT_WATER_SWIM);), false, 1},
    {"with a capsule, in space", "deploy", false, SETUP(capsule(g, att, 4, 10, SECT_SPACE);), false, 1},
    {"with a capsule", "deploy", false, SETUP(capsule(g, att, 4); sites(g, 18800, 4, 0);), false, 1},
    {"with a capsule, the first site taken", "deploy", false, SETUP(capsule(g, att, 4); sites(g, 18800, 4, 1);), false, 1},
    {"with a capsule, every site taken", "deploy", false, SETUP(capsule(g, att, 4); sites(g, 18800, 4, 25);), false, 1},
    {"with a big capsule", "deploy", false, SETUP(capsule(g, att, 5); sites(g, 18900, 5, 1);), false, 1},
    {"with a grand capsule", "deploy", false, SETUP(capsule(g, att, 6); sites(g, 19100, 5, 0);), false, 1},
    {"with two capsules", "deploy", false, SETUP(capsule(g, att, 6); capsule(g, att, 4); sites(g, 18800, 4, 0); sites(g, 19100, 5, 0);), false, 1},
    {"", "deploy capsule", false, SETUP(), false, 1},
    {"with a rock", "deploy rock", false, SETUP(carry(g, att, {.name = "rock"});), false, 1},
    {"with a furniture capsule, outdoors", "deploy capsule", false, SETUP(furniture(g, att, 19080, {});), false, 1},
    {"with a furniture capsule, on a ship", "deploy capsule", false, SETUP(furniture(g, att, 19080, {ROOM_HOUSE, ROOM_SHIP});), false, 1},
    {"with a furniture capsule, in a garden", "deploy capsule", false, SETUP(furniture(g, att, 19080, {ROOM_HOUSE, ROOM_GARDEN1});), false, 1},
    {"with a furniture capsule, in a big garden", "deploy capsule", false, SETUP(furniture(g, att, 19080, {ROOM_HOUSE, ROOM_GARDEN2});), false, 1},
    {"with a bed capsule", "deploy capsule", false, SETUP(furniture(g, att, 19080);), false, 1},
    {"with a chair capsule", "deploy capsule", false, SETUP(furniture(g, att, 19081);), false, 1},
    {"with a table capsule", "deploy capsule", false, SETUP(furniture(g, att, 19082);), false, 1},
    {"with a stove capsule", "deploy capsule", false, SETUP(furniture(g, att, 19083);), false, 1},
    {"with a cushion capsule", "deploy capsule", false, SETUP(furniture(g, att, 19085);), false, 1},
    {"with a broken furniture capsule", "deploy capsule", false, SETUP(furniture(g, att, 19084);), false, 1},

    /* twohand */
    {"absorbing", "twohand", false, SETUP(absorbed_by(victim, att);), false, 1},
    {"absorbed", "twohand", false, SETUP(absorbed_by(att, victim);), false, 1},
    {"unarmed", "twohand", false, SETUP(unarmed(g, att);), false, 1},
    {"armed, and holding something", "twohand", false, SETUP(dual(g, att, TYPE_SLASH - TYPE_HIT);), false, 1},
    {"armed, with an arm lost", "twohand", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT); char_set_limb_condition(att, 0, 0);), false, 1},
    {"armed, with the other arm lost", "twohand", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT); char_set_limb_condition(att, 1, 0);), false, 1},
    {"armed", "twohand", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT);), false, 1},
    {"two-handing", "twohand", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT); two_handed(att, 50);), false, 1},
    {"two-handing nothing, armless", "twohand", false, SETUP(unarmed(g, att); two_handed(att, 50); armless(att);), false, 1},

    /* auction: there's no such command, so the setup calls it */
    {"in the time chamber", "auction", false, SETUP(room_set_flag(g, ARENA, ROOM_HBTC, true); rock(g, att); auction(g, att, "rock 10");), false, 1},
    {"in the past", "auction", false, SETUP(room_set_flag(g, ARENA, ROOM_PAST, true); rock(g, att); auction(g, att, "rock 10");), false, 1},
    {"hidden", "auction", false, SETUP(char_prf_set(g, att, PRF_HIDE, true); rock(g, att); auction(g, att, "rock 10");), false, 1},
    {"asking for nothing", "auction", false, SETUP(auction(g, att, "");), false, 1},
    {"cancelling nothing", "auction", false, SETUP(auction(g, att, "cancel");), false, 1},
    {"stopping nothing", "auction", false, SETUP(auction(g, att, "stop");), false, 1},
    {"asking about nothing", "auction", false, SETUP(auction(g, att, "stats");), false, 1},
    {"identifying nothing", "auction", false, SETUP(auction(g, att, "identify");), false, 1},
    {"selling what isn't there", "auction", false, SETUP(auction(g, att, "rock 10");), false, 1},
    {"selling an item", "auction", false, SETUP(rock(g, att); auction(g, att, "item 10");), false, 1},
    {"selling a rock, at no price", "auction", false, SETUP(rock(g, att); auction(g, att, "rock");), false, 1},
    {"selling a rock, for nothing", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 0");), false, 1},
    {"selling an unsellable rock", "auction", false, SETUP(rock(g, att, {ITEM_NOSELL}); auction(g, att, "rock 10");), false, 1},
    {"selling a corpse", "auction", false, SETUP(obj_set_value(rock(g, att), VAL_CONTAINER_CORPSE, 1); auction(g, att, "rock 10");), false, 1},
    {"selling a rock, unbought", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); calls(g, 5);), false, 1},
    {"selling a rock, bought", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); auction_set_buyer(g, victim); g->curbid = 20; calls(g, 5);), false, 1},
    {"selling a rock, bought, the seller rich", "auction", false, SETUP(rock(g, att); char_set_gold(att, char_gold_carry(att) - 5); auction(g, att, "rock 10"); auction_set_buyer(g, victim); calls(g, 5);), false, 1},
    {"selling a rock, lost on offer", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); extract_obj(g, auction_obj(g)); calls(g, 1);), false, 1},
    {"selling a rock, lost going once", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); calls(g, 1); extract_obj(g, auction_obj(g)); calls(g, 1);), false, 1},
    {"selling a rock, lost going twice", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); calls(g, 2); extract_obj(g, auction_obj(g)); calls(g, 1);), false, 1},
    {"selling a rock, lost at the last call", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); calls(g, 3); extract_obj(g, auction_obj(g)); calls(g, 1);), false, 1},
    {"selling a rock, lost, then cancelling", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); extract_obj(g, auction_obj(g)); auction(g, att, "cancel");), false, 1},
    {"selling a rock while another sells", "auction", false, SETUP(rock(g, victim); auction(g, victim, "rock 10"); rock(g, att); auction(g, att, "rock 15"); calls(g, 5);), false, 1},
    {"selling a rock, then cancelling", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); auction(g, att, "cancel");), false, 1},
    {"selling a rock, bid on, then cancelling", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); auction_set_buyer(g, victim); g->curbid = 20; auction(g, att, "stop");), false, 1},
    {"selling a rock, cancelled by another", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); auction(g, victim, "cancel"); calls(g, 5);), false, 1},
    {"selling a rock, cancelled by a god", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); admin_set(g, victim, ADMLVL_IMPL); auction(g, victim, "cancel");), false, 1},
    {"selling a rock, then quitting", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); stop_auction(g, AUC_QUIT_CANCEL, nullptr);), false, 1},
    {"selling a rock, cancelled oddly", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); stop_auction(g, AUC_SOLD, att); auction(g, att, "cancel");), false, 1},
    {"selling a rock, asking about it", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); auction(g, att, "stats"); auction(g, att, "cancel");), false, 1},
    {"selling a rock, asked about by the poor", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); char_set_gold(victim, 499); auction(g, victim, "stats"); auction(g, att, "cancel");), false, 1},
    {"selling a rock, asked about", "auction", false, SETUP(rock(g, att); auction(g, att, "rock 10"); char_set_gold(victim, 500); auction(g, victim, "identify"); auction(g, att, "cancel");), false, 1},
    {"selling a rock, heard in the past", "auction", false, SETUP(room_set_flag(g, 300, ROOM_PAST, true); away(g, victim); rock(g, att); auction(g, att, "rock 10"); auction(g, att, "cancel");), false, 1},
    {"selling a rock, heard in the time chamber", "auction", false, SETUP(room_set_flag(g, 300, ROOM_HBTC, true); away(g, victim); rock(g, att); auction(g, att, "rock 10"); auction(g, att, "cancel");), false, 1},

    /* bid */
    {"", "bid", false, SETUP(), false, 1},
    {"with a scouter, in the time chamber", "bid", false, SETUP(bidder(g, att); room_set_flag(g, ARENA, ROOM_HBTC, true);), false, 1},
    {"with a scouter, in the past", "bid", false, SETUP(bidder(g, att); room_set_flag(g, ARENA, ROOM_PAST, true);), false, 1},
    {"with a scouter", "bid", false, SETUP(bidder(g, att);), false, 1},
    {"with a scouter", "bid list", false, SETUP(bidder(g, att);), false, 1},
    {"with a scouter, lots up", "bid list", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, lots up", "bid appraise", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, lots up", "bid appraise 99", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, lots up", "bid appraise -1", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, lots up", "bid appraise 0", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, lots up", "bid appraise 6", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"unskilled, lots up", "bid appraise 1", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"a poor appraiser, lots up", "bid appraise 1", false, SETUP(appraiser(g, att, 1); lots(g, att, victim);), false, 1},
    {"an appraiser, lots up", "bid appraise 3", false, SETUP(appraiser(g, att); lots(g, att, victim);), false, 1},
    {"an appraiser, a worn lot", "bid appraise 1", false, SETUP(appraiser(g, att); obj_set_value(lot(g, 99999, -1, 10, 60), VAL_ALL_HEALTH, 60);), false, 1},
    {"an appraiser, a battered lot", "bid appraise 1", false, SETUP(appraiser(g, att); obj_set_value(lot(g, 99999, -1, 10, 60), VAL_ALL_HEALTH, 10);), false, 1},
    {"an appraiser, a ruined lot", "bid appraise 1", false, SETUP(appraiser(g, att); obj_set_value(lot(g, 99999, -1, 10, 60), VAL_ALL_HEALTH, 0);), false, 1},
    {"an appraiser, an enchanted lot", "bid appraise 1", false, SETUP(appraiser(g, att); enchanted(lot(g, 99999, -1, 10, 60, {.name = "lot", .wear = {ITEM_WEAR_FINGER, ITEM_WEAR_NECK}}));), false, 1},
    {"an appraiser, a plain weapon", "bid appraise 1", false, SETUP(appraiser(g, att); lot(g, 99999, -1, 10, 60, {.name = "lot", .type = ITEM_WEAPON});), false, 1},
    {"an appraiser, a weapon of the first grade", "bid appraise 1", false, SETUP(appraiser(g, att); lot(g, 99999, -1, 10, 60, {.name = "lot", .type = ITEM_WEAPON, .flags = {ITEM_WEAPLVL1}});), false, 1},
    {"an appraiser, a weapon of the second grade", "bid appraise 1", false, SETUP(appraiser(g, att); lot(g, 99999, -1, 10, 60, {.name = "lot", .type = ITEM_WEAPON, .flags = {ITEM_WEAPLVL2}});), false, 1},
    {"an appraiser, a weapon of the third grade", "bid appraise 1", false, SETUP(appraiser(g, att); lot(g, 99999, -1, 10, 60, {.name = "lot", .type = ITEM_WEAPON, .flags = {ITEM_WEAPLVL3}});), false, 1},
    {"an appraiser, a weapon of the fourth grade", "bid appraise 1", false, SETUP(appraiser(g, att); lot(g, 99999, -1, 10, 60, {.name = "lot", .type = ITEM_WEAPON, .flags = {ITEM_WEAPLVL4}});), false, 1},
    {"an appraiser, a weapon of the fifth grade", "bid appraise 1", false, SETUP(appraiser(g, att); lot(g, 99999, -1, 10, 60, {.name = "lot", .type = ITEM_WEAPON, .flags = {ITEM_WEAPLVL5}});), false, 1},
    {"a middling appraiser, lots up", "bid appraise 2", false, SETUP(appraiser(g, att, 50); lots(g, att, victim);), false, 4},
    {"with a scouter, lots up", "bid 1", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, lots up", "bid 99 10", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, lots up", "bid -1 10", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, lots up", "bid 0 10", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, lots up", "bid 2 1000", false, SETUP(bidder(g, att); lots(g, att, victim);), false, 1},
    {"with a scouter, by your own lot", "bid 1 1000", false, SETUP(bidder(g, att); lot(g, char_get_id(att), -1, 10, 60);), false, 1},
    {"with a scouter, by your top bid", "bid 1 1000", false, SETUP(bidder(g, att); lot(g, 99999, char_get_id(att), 100, 60);), false, 1},
    {"watching the dummy bid", "bid", false, SETUP(command_interpreter(g, dummy(g), "bid list");), false, 1},
    {"with a scouter, by a lot bid on", "bid 1 105", false, SETUP(bidder(g, att); lot(g, 99999, char_get_id(victim), 100, 60);), false, 1},
    {"with a scouter, by a lot bid on", "bid 1 111", false, SETUP(bidder(g, att); lot(g, 99999, char_get_id(victim), 100, 60);), false, 1},
    {"with a scouter, by a lot", "bid 1 50", false, SETUP(bidder(g, att); lot(g, 99999, -1, 100, 60);), false, 1},
    {"with a scouter, by a lot", "bid 1 100", false, SETUP(bidder(g, att); lot(g, 99999, -1, 100, 60);), false, 1},
    {"poor, with a scouter, by a lot", "bid 1 100", false, SETUP(bidder(g, att, 10); lot(g, 99999, -1, 100, 60);), false, 1},
    {"with a scouter, by a closed lot", "bid 1 100", false, SETUP(bidder(g, att); lot(g, 99999, -1, 100, 90000);), false, 1},
    {"with a scouter, among bidders", "bid 1 100", false, SETUP(bidder(g, att); scouter_on(g, victim); lot(g, 99999, -1, 100, 60);), false, 1},
    {"with a scouter, among bidders, lots up", "bid 4 1000", false, SETUP(bidder(g, att); scouter_on(g, victim); lots(g, att, victim);), false, 1},

    /* build */
    {"", "build", false, SETUP(), false, 1},
    {"", "build nothing", false, SETUP(), false, 1},
    {"", "build chest", false, SETUP(), false, 1},
    {"with the parts of a lamp", "build lamp", false, SETUP(named_proto(g, 5000, "lamp"); assemblyCreate(g, 5000, SCMD_BAKE); assemblyAddComponent(g, 5000, 301, true, false); parts(g, att);), false, 1},
    {"with the parts, in space", "build chest", false, SETUP(parts(g, att); room_set_flag(g, ARENA, ROOM_SPACE, true);), false, 1},
    {"with the parts, unskilled", "build chest", false, SETUP(parts(g, att, 0);), false, 2},
    {"with the parts, middling", "build chest", false, SETUP(parts(g, att, 50); toolkit(g, att);), false, 4},
    {"with the parts and a toolkit", "build chest", false, SETUP(parts(g, att); toolkit(g, att);), false, 8},
    {"with the parts and two toolkits", "build chest", false, SETUP(parts(g, att); toolkit(g, att); toolkit(g, att);), false, 1},
    {"a survivor with the parts", "build chest", false, SETUP(parts(g, att); toolkit(g, att); char_set_skills_base(att, SKILL_SURVIVAL, 95);), false, 4},
    {"with the parts and a broken toolkit", "build chest", false, SETUP(parts(g, att); toolkit(g, att, 0);), false, 2},
    {"with the parts, dim", "build chest", false, SETUP(parts(g, att); toolkit(g, att); char_set_real_abils_intel(att, 1); affect_total(g, att);), false, 8},
    {"with the parts, at sea", "build chest", false, SETUP(parts(g, att); room_set_sector_type(g, ARENA, SECT_WATER_NOSWIM);), false, 1},
    {"with the parts, underwater", "build chest", false, SETUP(parts(g, att); room_set_geffect(g, ARENA, -1);), false, 1},
    {"a truffle with the parts", "build chest", false, SETUP(parts(g, att); toolkit(g, att); truffle(g, att, 100, 0);), false, 8},
    {"a truffle with the parts of something good", "build chest", false, SETUP(parts(g, att); toolkit(g, att); truffle(g, att, 1000, 1);), false, 8},
    {"a truffle with the parts of something fine", "build chest", false, SETUP(parts(g, att); toolkit(g, att); truffle(g, att, 1000, 6);), false, 8},
    {"a truffle at level 100", "build chest", false, SETUP(parts(g, att); toolkit(g, att); truffle(g, att, 1000, 1); char_set_class_level(att, 100);), false, 8},
    {"a novice truffle", "build chest", false, SETUP(parts(g, att, 50); toolkit(g, att); truffle(g, att, 1000, 1);), false, 8},
    {"with a beacon's parts, unskilled", "build signal", false, SETUP(beacon(g, att, 60);), false, 1},
    {"with a beacon's parts", "build signal", false, SETUP(beacon(g, att, 100); toolkit(g, att);), false, 2},
    {"without survival, by wood", "build campfire", false, SETUP(campfire(g, att, 0);), false, 1},
    {"by wood", "build campfire", false, SETUP(campfire(g, att); toolkit(g, att);), false, 8},
    {"by wood, without tools", "build campfire", false, SETUP(campfire(g, att);), false, 4},
    {"a survivor, by wood", "build campfire", false, SETUP(campfire(g, att, 95); toolkit(g, att);), false, 8},
    {"by wood, with a worn toolkit", "build campfire", false, SETUP(campfire(g, att, 5); toolkit(g, att, 1);), false, 8},
    {"by wood, at sea", "build campfire", false, SETUP(campfire(g, att, 95); room_set_sector_type(g, ARENA, SECT_WATER_NOSWIM);), false, 2},

    /* put */
    {"armless", "put pebble bag", false, SETUP(armless(att); pebbles(g, att, 1); bag(g, att);), false, 1},
    {"", "put", false, SETUP(), false, 1},
    {"", "put pebble all.bag", false, SETUP(), false, 1},
    {"", "put pebble", false, SETUP(), false, 1},
    {"", "put all", false, SETUP(), false, 1},
    {"", "put pebble bag", false, SETUP(), false, 1},
    {"with a pebble", "put pebble pebble", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with a closed bag", "put pebble bag", false, SETUP(pebbles(g, att, 1); bag(g, att, 100, {}, cont_value(CONT_CLOSED));), false, 1},
    {"with a bag", "put pebble bag", false, SETUP(bag(g, att);), false, 1},
    {"with a bag", "put bag bag", false, SETUP(bag(g, att);), false, 1},
    {"with two bags", "put 2.bag bag", false, SETUP(bag(g, att); bag(g, att);), false, 1},
    {"with two bags", "put 2 bag bag", false, SETUP(bag(g, att); bag(g, att);), false, 1},
    {"with pebbles and a bag", "put pebble bag", false, SETUP(bag(g, att); pebbles(g, att, 3);), false, 1},
    {"with pebbles and a bag", "put 2 pebble bag", false, SETUP(bag(g, att); pebbles(g, att, 3);), false, 1},
    {"with pebbles and a bag", "put all bag", false, SETUP(bag(g, att); pebbles(g, att, 3);), false, 1},
    {"with pebbles and a bag", "put all.pebble bag", false, SETUP(bag(g, att); pebbles(g, att, 3);), false, 1},
    {"with pebbles and a bag", "put all.rock bag", false, SETUP(bag(g, att); pebbles(g, att, 3);), false, 1},
    {"with a bag", "put all bag", false, SETUP(bag(g, att);), false, 1},
    {"bare, with a bag", "put all bag", false, SETUP(bare(g, att); bag(g, att);), false, 1},
    {"with a pebble", "put pebble chest", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with a pebble and a forged bag", "put pebble bag", false, SETUP(pebbles(g, att, 1); bag(g, att, 100, {ITEM_FORGED});), false, 1},
    {"with a pebble and a sheath", "put pebble sheath", false, SETUP(pebbles(g, att, 1); sheath(g, att, 1, 0);), false, 1},
    {"with a sword and a sheath", "put sword sheath", false, SETUP(missile(g, att); sheath(g, att, 1, 0);), false, 1},
    {"with a sword and a full sheath", "put sword sheath", false, SETUP(missile(g, att); sheath(g, att, 1, 1);), false, 1},
    {"with a sword and a full double sheath", "put sword sheath", false, SETUP(missile(g, att); sheath(g, att, 2, 2);), false, 1},
    {"with a pebble and a bag that holds nothing", "put pebble bag", false, SETUP(pebbles(g, att, 1); bag(g, att, 0);), false, 1},
    {"with a pebble, by a duel table", "put pebble table", false, SETUP(pebbles(g, att, 1); here(g, {.name = "table", .vnum = 601, .type = ITEM_CONTAINER, .values = {{0, 100}}});), false, 1},
    {"with a pebble and a card case", "put pebble case", false, SETUP(pebbles(g, att, 1); carry(g, att, {.name = "case", .type = ITEM_CONTAINER, .values = {{0, 100}}, .flags = {ITEM_CARDCASE}});), false, 1},
    {"with a pebble and a card binder", "put pebble binder", false, SETUP(pebbles(g, att, 1); carry(g, att, {.name = "binder", .vnum = 683, .type = ITEM_CONTAINER, .values = {{0, 100}}});), false, 1},
    {"with a card and a card case", "put card case", false, SETUP(carry(g, att, {.name = "card", .flags = {ITEM_ANTI_HIEROPHANT}}); carry(g, att, {.name = "case", .vnum = 697, .type = ITEM_CONTAINER, .values = {{0, 100}}});), false, 1},
    {"with a pebble and a small bag", "put pebble bag", false, SETUP(pebbles(g, att, 1); bag(g, att, 1);), false, 1},
    {"with a cursed pebble", "put pebble chest", false, SETUP(carry(g, att, {.flags = {ITEM_NODROP}});), false, 1},
    {"with a cursed pebble and a bag", "put pebble bag", false, SETUP(carry(g, att, {.flags = {ITEM_NODROP}}); bag(g, att);), false, 1},
    {"with a cursed pebble and a cursed bag", "put pebble bag", false, SETUP(carry(g, att, {.flags = {ITEM_NODROP}}); bag(g, att, 100, {ITEM_NODROP});), false, 1},
    {"with a dragon ball and a bag", "put ball bag", false, SETUP(dragon_balls(g, att, {3}); bag(g, att);), false, 1},
    {"with a fleeting pebble and a bag", "put pebble bag", false, SETUP(carry(g, att, {.flags = {ITEM_NORENT}}); bag(g, att);), false, 1},
    {"with a pebble, in a full house", "put pebble chest", false, SETUP(pebbles(g, att, 1); stuffed(g, 150);), false, 1},
    {"with a pebble and a bag, in a full house", "put pebble bag", false, SETUP(pebbles(g, att, 1); bag(g, att); stuffed(g, 150);), false, 1},
    {"with a pebble, by a portal", "put pebble portal", false, SETUP(pebbles(g, att, 1); here(g, {.name = "portal", .type = ITEM_PORTAL, .values = {{0, 300}}});), false, 1},
    {"with a pebble, by a closed portal", "put pebble portal", false, SETUP(pebbles(g, att, 1); here(g, {.name = "portal", .type = ITEM_PORTAL, .values = {{0, 300}, {1, cont_value(CONT_CLOSED)}}});), false, 1},
    {"with a pebble, by a vehicle", "put pebble car", false, SETUP(pebbles(g, att, 1); here(g, {.name = "car", .type = ITEM_VEHICLE, .values = {{0, 300}}});), false, 1},

    /* get */
    {"armless", "get pebble", false, SETUP(armless(att); here(g, {});), false, 1},
    {"", "get", false, SETUP(), false, 1},
    {"", "get pebble", false, SETUP(), false, 1},
    {"by a pebble", "get pebble", false, SETUP(here(g, {});), false, 1},
    {"by a pebble", "take pebble", false, SETUP(here(g, {});), false, 1},
    {"by pebbles", "get 2 pebble", false, SETUP(here(g, {}); here(g, {}); here(g, {});), false, 1},
    {"by pebbles", "get all", false, SETUP(here(g, {}); here(g, {}); here(g, {});), false, 1},
    {"by pebbles", "get all.pebble", false, SETUP(here(g, {}); here(g, {}); here(g, {});), false, 1},
    {"by pebbles", "get all.", false, SETUP(here(g, {});), false, 1},
    {"by pebbles", "get all.rock", false, SETUP(here(g, {});), false, 1},
    {"in an empty room", "get all", false, SETUP(swept(g);), false, 1},
    {"by a sign", "get sign", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "sign");), false, 1},
    {"by a sign and a signpost", "get sig", false, SETUP(described(room_ex_description_slot(room_of(g, ARENA)), "signpost"); here(g, {.name = "sig"});), false, 1},
    {"by a fixed pebble", "get pebble", false, SETUP(obj_wear_set(here(g, {}), ITEM_WEAR_TAKE, false);), false, 1},
    {"crowded, by a stone", "get stone", false, SETUP(crowded(g, att); here(g, {.name = "stone"});), false, 1},
    {"laden, by a pebble", "get stone", false, SETUP(laden(g, att); here(g, {.name = "stone"});), false, 1},
    {"in heavy gravity, by a pebble", "get pebble", false, SETUP(room_set_gravity(g, ARENA, (int)getMaxCarryWeight(g, att)); here(g, {});), false, 1},
    {"by an occupied seat", "get seat", false, SETUP({ obj_data *seat = here(g, {.name = "seat"}); char_set_sits(victim, seat); obj_set_sitting(seat, victim); }), false, 1},
    {"by a buried pebble", "get pebble", false, SETUP(here(g, {.flags = {ITEM_BURIED}});), false, 1},
    {"in a garden, by a sprout", "get sprout", false, SETUP(room_set_flag(g, ARENA, ROOM_GARDEN1, true); here(g, {.name = "sprout", .type = ITEM_PLANT});), false, 1},
    {"in a big garden, by a sprout", "get sprout", false, SETUP(room_set_flag(g, ARENA, ROOM_GARDEN2, true); here(g, {.name = "sprout", .type = ITEM_PLANT});), false, 1},
    {"by a hot pebble", "get pebble", false, SETUP(here(g, {.flags = {ITEM_HOT}});), false, 1},
    {"fireproof, by a hot pebble", "get pebble", false, SETUP(char_set_bonuses(att, BONUS_FIREPROOF, 1); here(g, {.flags = {ITEM_HOT}});), false, 1},
    {"fireprone, by a hot pebble", "get pebble", false, SETUP(char_set_bonuses(att, BONUS_FIREPRONE, 1); here(g, {.flags = {ITEM_HOT}});), false, 1},
    {"a demon, by a hot pebble", "get pebble", false, SETUP(char_set_race(att, RACE_DEMON); here(g, {.flags = {ITEM_HOT}});), false, 1},
    {"by a zenni", "get zenni", false, SETUP(zenni_here(g, 1);), false, 1},
    {"by zenni", "get zenni", false, SETUP(zenni_here(g, 100);), false, 1},
    {"rich, by zenni", "get zenni", false, SETUP(char_set_gold(att, char_gold_carry(att) - 10); zenni_here(g, 100);), false, 1},
    {"grouped, splitting, by zenni", "get zenni", false, SETUP(group(g, att, cast.helper); char_prf_set(g, att, PRF_AUTOSPLIT, true); zenni_here(g, 100);), false, 1},
    {"grouped, not splitting, by zenni", "get zenni", false, SETUP(group(g, att, cast.helper); zenni_here(g, 100);), false, 1},
    {"with a bag", "get pebble bag", false, SETUP(inside(g, bag(g, att), {});), false, 1},
    {"with a bag", "get nothing bag", false, SETUP(inside(g, bag(g, att), {});), false, 1},
    {"with a bag", "get pebble nothing", false, SETUP(inside(g, bag(g, att), {});), false, 1},
    {"with a pebble", "get pebble pebble", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"by a car", "get pebble car", false, SETUP(here(g, {.name = "car", .type = ITEM_VEHICLE});), false, 1},
    {"by a portal", "get pebble portal", false, SETUP(here(g, {.name = "portal", .type = ITEM_PORTAL});), false, 1},
    {"by a closeable portal", "get pebble portal", false, SETUP(here(g, {.name = "portal", .type = ITEM_PORTAL, .values = {{1, cont_value(CONT_CLOSEABLE)}}});), false, 1},
    {"with a closed bag", "get pebble bag", false, SETUP(inside(g, bag(g, att, 100, {}, cont_value(CONT_CLOSED)), {});), false, 1},
    {"by a pebble in the chest", "get pebble chest", false, SETUP(inside(g, cast.chest, {});), false, 1},
    {"by a fixed pebble in the chest", "get pebble chest", false, SETUP(obj_wear_set(inside(g, cast.chest, {}), ITEM_WEAR_TAKE, false);), false, 1},
    {"with a bag of pebbles", "get 2 pebble bag", false, SETUP({ obj_data *b = bag(g, att); for (int i = 0; i < 3; i++) inside(g, b, {}); }), false, 1},
    {"with a bag of pebbles", "get all bag", false, SETUP({ obj_data *b = bag(g, att); for (int i = 0; i < 3; i++) inside(g, b, {}); }), false, 1},
    {"with a bag of pebbles", "get all.pebble bag", false, SETUP({ obj_data *b = bag(g, att); for (int i = 0; i < 3; i++) inside(g, b, {}); }), false, 1},
    {"with a bag of pebbles", "get all. bag", false, SETUP(inside(g, bag(g, att), {});), false, 1},
    {"with a bag of pebbles", "get all.rock bag", false, SETUP(inside(g, bag(g, att), {});), false, 1},
    {"with an empty bag", "get all bag", false, SETUP(bag(g, att);), false, 1},
    {"crowded, with a bag", "get pebble bag", false, SETUP(inside(g, bag(g, att), {}); crowded(g, att);), false, 1},
    {"with a hot pebble in a bag", "get pebble bag", false, SETUP(inside(g, bag(g, att), {.flags = {ITEM_HOT}});), false, 1},
    {"fireprone, with a hot pebble in a bag", "get pebble bag", false, SETUP(char_set_bonuses(att, BONUS_FIREPRONE, 1); inside(g, bag(g, att), {.flags = {ITEM_HOT}});), false, 1},
    {"with zenni in a bag", "get zenni bag", false, SETUP(obj_to_obj(g, create_money(g, 50), bag(g, att));), false, 1},
    {"with a sword in a sheath", "get sword sheath", false, SETUP(sheath(g, att, 1, 1);), false, 1},
    {"at a duel table", "get card case", false, SETUP(at_duel_table(g, att, 1);), false, 1},
    {"at a duel table", "get card table", false, SETUP({ obj_data *t = at_duel_table(g, att, 1); obj_set_type_flag(t, ITEM_CONTAINER); obj_set_value(t, 0, 100); inside(g, t, {.name = "card"}); }), false, 1},
    {"by another duel table", "get card other", false, SETUP(at_duel_table(g, att, 1); inside(g, here(g, {.name = "other", .vnum = 601, .type = ITEM_CONTAINER, .values = {{0, 100}}}), {.name = "card"});), false, 1},
    {"with bags of pebbles", "get pebble all.bag", false, SETUP(inside(g, bag(g, att), {}); inside(g, bag(g, att), {}); here(g, {.name = "bag"});), false, 1},
    {"with bags of pebbles", "get all all", false, SETUP(inside(g, bag(g, att), {}); pebbles(g, att, 1); inside(g, cast.chest, {});), false, 1},
    {"with bags", "get pebble all.", false, SETUP(bag(g, att);), false, 1},
    {"", "get pebble all.bag", false, SETUP(), false, 1},
    {"in an empty room", "get pebble all", false, SETUP(swept(g);), false, 1},
    {"with a bag that holds nothing", "get pebble all.bag", false, SETUP(carry(g, att, {.name = "bag"});), false, 1},
    {"with a bag of this and that", "get 2 pebble bag", false, SETUP({ obj_data *b = bag(g, att); inside(g, b, {}); inside(g, b, {.name = "rock"}); inside(g, b, {}); }), false, 1},
    {"watching the dummy take a pebble", "get", false, SETUP(here(g, {}); command_interpreter(g, dummy(g), "get pebble");), false, 1},
    {"watching the dummy take a pebble from its bag", "get", false, SETUP({ char_data *d = dummy(g); inside(g, bag(g, d), {}); command_interpreter(g, d, "get pebble bag"); }), false, 1},

    /* drop, junk and donate */
    {"armless", "drop pebble", false, SETUP(armless(att); pebbles(g, att, 1);), false, 1},
    {"in a garden", "drop pebble", false, SETUP(room_set_flag(g, ARENA, ROOM_GARDEN1, true); pebbles(g, att, 1);), false, 1},
    {"in a big garden", "drop pebble", false, SETUP(room_set_flag(g, ARENA, ROOM_GARDEN2, true); pebbles(g, att, 1);), false, 1},
    {"", "drop", false, SETUP(), false, 1},
    {"", "junk", false, SETUP(), false, 1},
    {"", "donate", false, SETUP(), false, 4},
    {"with the donation rooms there", "donate", false, SETUP(CONFIG_DON_ROOM_1 = 300; CONFIG_DON_ROOM_2 = 300; CONFIG_DON_ROOM_3 = 300;), false, 8},
    {"", "drop 0 zenni", false, SETUP(), false, 1},
    {"", "drop 100 zenni", false, SETUP(), false, 1},
    {"with zenni", "drop 100 zenni", false, SETUP(char_set_gold(att, 1000);), false, 1},
    {"with zenni", "drop 100 gold", false, SETUP(char_set_gold(att, 1000);), false, 1},
    {"an immortal with zenni", "drop 100 zenni", false, SETUP(char_set_gold(att, 1000); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"with zenni", "junk 100 zenni", false, SETUP(char_set_gold(att, 1000);), false, 1},
    {"with zenni, by the donation rooms", "donate 100 zenni", false, SETUP(char_set_gold(att, 1000); CONFIG_DON_ROOM_1 = 300;), false, 4},
    {"with pebbles", "drop 0 pebble", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles", "drop 2", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles", "drop 2 rock", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles", "drop 2 pebble", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles, in a full house", "drop 2 pebble", false, SETUP(pebbles(g, att, 3); stuffed(g, 150);), false, 1},
    {"with pebbles", "junk all", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles", "donate all", false, SETUP(pebbles(g, att, 3); CONFIG_DON_ROOM_1 = 300;), false, 1},
    {"empty-handed", "drop all", false, SETUP(bare(g, att);), false, 1},
    {"with this and that", "drop 2 pebble", false, SETUP(pebbles(g, att, 1); carry(g, att, {.name = "rock"}); pebbles(g, att, 1);), false, 1},
    {"with a bag of bags, at home", "drop bag", false, SETUP(stuffed(g, 0); inside(g, inside(g, bag(g, att), {.name = "pouch", .type = ITEM_CONTAINER, .values = {{0, 100}}}), {});), false, 1},
    {"with pebbles", "drop all", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles, in a nearly full house", "drop all", false, SETUP(pebbles(g, att, 3); stuffed(g, 149);), false, 1},
    {"with pebbles", "drop all.pebble", false, SETUP(pebbles(g, att, 3); carry(g, att, {.name = "rock"});), false, 1},
    {"with pebbles, in a nearly full house", "drop all.pebble", false, SETUP(pebbles(g, att, 3); stuffed(g, 149);), false, 1},
    {"with pebbles", "drop all.", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles", "drop all.rock", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"", "drop pebble", false, SETUP(), false, 1},
    {"with a pebble", "drop pebble", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with a pebble, in a full house", "drop pebble", false, SETUP(pebbles(g, att, 1); stuffed(g, 150);), false, 1},
    {"an immortal with a pebble", "drop pebble", false, SETUP(pebbles(g, att, 1); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"with a soul", "drop soul", false, SETUP(carry(g, att, {.name = "soul", .vnum = 17});), false, 1},
    {"with another soul", "junk soul", false, SETUP(carry(g, att, {.name = "soul", .vnum = 17998});), false, 1},
    {"with a dragon ball, in space", "drop ball", false, SETUP(dragon_balls(g, att, {1}); room_set_flag(g, ARENA, ROOM_SPACE, true);), false, 1},
    {"with a fleeting dragon ball", "drop ball", false, SETUP(obj_extra_set(holding_as(g, att, "ball", 22), ITEM_NORENT, true);), false, 1},
    {"with a fleeting dragon ball", "junk ball", false, SETUP(obj_extra_set(holding_as(g, att, "ball", 22), ITEM_NORENT, true); room_set_flag(g, ARENA, ROOM_NOINSTANT, true);), false, 1},
    {"with a dragon ball, in a protected place", "drop ball", false, SETUP(dragon_balls(g, att, {2}); room_set_flag(g, ARENA, ROOM_NOINSTANT, true);), false, 1},
    {"with a dragon ball, on a ship", "drop ball", false, SETUP(dragon_balls(g, att, {4}); room_set_flag(g, ARENA, ROOM_SHIP, true);), false, 1},
    {"with a dragon ball, at home", "drop ball", false, SETUP(dragon_balls(g, att, {5}); room_set_flag(g, ARENA, ROOM_HOUSE, true);), false, 1},
    {"with a dragon ball", "drop ball", false, SETUP(dragon_balls(g, att, {7});), false, 1},
    {"with a cursed pebble", "drop pebble", false, SETUP(carry(g, att, {.flags = {ITEM_NODROP}});), false, 1},
    {"an immortal with a cursed pebble", "drop pebble", false, SETUP(carry(g, att, {.flags = {ITEM_NODROP}}); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"with a keepsake", "junk pebble", false, SETUP(carry(g, att, {.flags = {ITEM_NODONATE}});), false, 1},
    {"with a keepsake, by the donation rooms", "donate pebble", false, SETUP(carry(g, att, {.flags = {ITEM_NODONATE}}); CONFIG_DON_ROOM_1 = 300;), false, 4},
    {"with a keepsake", "drop pebble", false, SETUP(carry(g, att, {.flags = {ITEM_NODONATE}});), false, 1},
    {"with a pebble, by lava", "drop pebble", false, SETUP(pebbles(g, att, 1); room_set_geffect(g, ARENA, 6);), false, 1},
    {"with a hard pebble, by lava", "drop pebble", false, SETUP(carry(g, att, {.flags = {ITEM_UNBREAKABLE}}); room_set_geffect(g, ARENA, 6);), false, 1},
    {"an immortal with a hard pebble, by lava", "drop pebble", false, SETUP(carry(g, att, {.flags = {ITEM_UNBREAKABLE}}); room_set_geffect(g, ARENA, 6); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"with a pebble", "junk pebble", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with a pebble, by the donation rooms", "donate pebble", false, SETUP(pebbles(g, att, 1); CONFIG_DON_ROOM_1 = 300;), false, 4},

    /* give */
    {"armless", "give pebble namek", false, SETUP(armless(att); pebbles(g, att, 1);), false, 1},
    {"", "give", false, SETUP(), false, 1},
    {"", "give 10 zenni", false, SETUP(), false, 1},
    {"", "give 10 zenni nobody", false, SETUP(), false, 1},
    {"", "give 10 zenni self", false, SETUP(), false, 1},
    {"", "give 0 zenni namek", false, SETUP(), false, 1},
    {"poor", "give 10 zenni namek", false, SETUP(char_set_gold(att, 0);), false, 1},
    {"with zenni", "give 10 zenni namek", false, SETUP(char_set_gold(att, 1000);), false, 1},
    {"with zenni", "give 10 gold namek", false, SETUP(char_set_gold(att, 1000);), false, 1},
    {"with zenni, to the rich", "give 10 zenni namek", false, SETUP(char_set_gold(att, 1000); char_set_gold(victim, char_gold_carry(victim));), false, 1},
    {"an immortal with zenni", "give 10 zenni namek", false, SETUP(char_set_gold(att, 1000); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"an implementor", "give 10 zenni namek", false, SETUP(admin_set(g, att, ADMLVL_IMPL);), false, 1},
    {"an immortal with zenni", "give 10 zenni dummy", false, SETUP(char_set_gold(att, 1000); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"with pebbles", "give 2", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles", "give 2 pebble", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles", "give 2 rock namek", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with pebbles", "give 2 pebble namek", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"an immortal with pebbles", "give 2 pebble namek", false, SETUP(pebbles(g, att, 3); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"", "give pebble", false, SETUP(), false, 1},
    {"", "give pebble namek", false, SETUP(), false, 1},
    {"with a pebble", "give pebble namek", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"an immortal with a pebble", "give pebble namek", false, SETUP(pebbles(g, att, 1); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"empty-handed", "give all namek", false, SETUP(bare(g, att);), false, 1},
    {"with this and that", "give 2 pebble namek", false, SETUP(pebbles(g, att, 1); carry(g, att, {.name = "rock"}); pebbles(g, att, 1);), false, 1},
    {"watching the dummy give to the crowded", "give", false, SETUP({ char_data *d = dummy(g); carry(g, d, {}); crowded(g, victim); command_interpreter(g, d, "give pebble namek"); }), false, 1},
    {"watching the dummy give to the laden", "give", false, SETUP({ char_data *d = dummy(g); carry(g, d, {}); laden(g, victim); command_interpreter(g, d, "give pebble namek"); }), false, 1},
    {"watching the dummy give in heavy gravity", "give", false, SETUP({ char_data *d = dummy(g); carry(g, d, {}); room_set_gravity(g, ARENA, (int)getMaxCarryWeight(g, victim)); command_interpreter(g, d, "give pebble namek"); }), false, 1},
    {"watching the dummy give to no one", "give", false, SETUP({ char_data *d = dummy(g); carry(g, d, {}); command_interpreter(g, d, "give pebble nobody"); }), false, 1},
    {"with pebbles", "give all namek", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"an immortal with pebbles", "give all.pebble namek", false, SETUP(pebbles(g, att, 3); carry(g, att, {.name = "rock"}); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"with pebbles", "give all. namek", false, SETUP(pebbles(g, att, 3);), false, 1},
    {"with a cursed pebble", "give pebble namek", false, SETUP(carry(g, att, {.flags = {ITEM_NODROP}});), false, 1},
    {"with a pebble, to the crowded", "give pebble namek", false, SETUP(pebbles(g, att, 1); crowded(g, victim);), false, 1},
    {"with a forged pebble", "give pebble dummy", false, SETUP(carry(g, att, {.flags = {ITEM_FORGED}});), false, 1},
    {"with a forged pebble", "give pebble namek", false, SETUP(carry(g, att, {.flags = {ITEM_FORGED}});), false, 1},
    {"with a pebble, to the laden", "give pebble namek", false, SETUP(pebbles(g, att, 1); laden(g, victim);), false, 1},
    {"with a pebble, in heavy gravity", "give pebble namek", false, SETUP(pebbles(g, att, 1); room_set_gravity(g, ARENA, (int)getMaxCarryWeight(g, victim));), false, 1},
    {"with a pebble, to the unwilling", "give pebble namek", false, SETUP(pebbles(g, att, 1); char_prf_set(g, victim, PRF_NOGIVE, true);), false, 1},
    {"with a pebble, to an unwilling dummy", "give pebble dummy", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with a hot pebble", "give pebble namek", false, SETUP(carry(g, att, {.flags = {ITEM_HOT}});), false, 1},
    {"with a hot pebble, to the fireproof", "give pebble namek", false, SETUP(carry(g, att, {.flags = {ITEM_HOT}}); char_set_bonuses(victim, BONUS_FIREPROOF, 1);), false, 1},
    {"with a hot pebble, to the fireprone", "give pebble namek", false, SETUP(carry(g, att, {.flags = {ITEM_HOT}}); char_set_bonuses(victim, BONUS_FIREPRONE, 1);), false, 1},
    {"with a hot pebble, to a demon", "give pebble namek", false, SETUP(carry(g, att, {.flags = {ITEM_HOT}}); char_set_race(victim, RACE_DEMON);), false, 1},

    /* drink and sip */
    {"an android", "drink", false, SETUP(char_set_race(att, RACE_ANDROID);), false, 1},
    {"never thirsty", "drink", false, SETUP(thirsty(g, att, -1);), false, 1},
    {"in a healing tank", "drink", false, SETUP(thirsty(g, att); char_plr_set(att, PLR_HEALT, true);), false, 1},
    {"starving", "drink", false, SETUP(thirsty(g, att, 10, 0);), false, 1},
    {"a starving namek", "drink", false, SETUP(thirsty(g, att, 10, 0); char_set_race(att, RACE_NAMEK);), false, 1},
    {"a starving mutant", "drink", false, SETUP(thirsty(g, att, 10, 0); char_set_genome(att, 0, 3);), false, 1},
    {"a starving mutant of the other kind", "drink", false, SETUP(thirsty(g, att, 10, 0); char_set_genome(att, 1, 3);), false, 1},
    {"starving and parched", "drink", false, SETUP(thirsty(g, att, 1, 0);), false, 1},
    {"", "drink", false, SETUP(thirsty(g, att);), false, 1},
    {"at a lake", "drink", false, SETUP(thirsty(g, att); room_set_sector_type(g, ARENA, SECT_WATER_SWIM);), false, 1},
    {"at sea", "drink", false, SETUP(thirsty(g, att); room_set_sector_type(g, ARENA, SECT_WATER_NOSWIM);), false, 1},
    {"underwater", "drink", false, SETUP(thirsty(g, att); room_set_sector_type(g, ARENA, SECT_UNDERWATER);), false, 1},
    {"sunken", "drink", false, SETUP(thirsty(g, att); room_set_geffect(g, ARENA, -1);), false, 1},
    {"quenched, at a lake", "drink", false, SETUP(thirsty(g, att, 47); room_set_sector_type(g, ARENA, SECT_WATER_SWIM);), false, 1},
    {"quenched, sunken", "drink", false, SETUP(thirsty(g, att, 47); room_set_geffect(g, ARENA, -1);), false, 1},
    {"a spent wellspring, at a lake", "drink", false, SETUP(thirsty(g, att); wellspring(g, att); room_set_sector_type(g, ARENA, SECT_WATER_SWIM);), false, 1},
    {"a spent wellspring, at a lake", "sip", false, SETUP(thirsty(g, att); wellspring(g, att); room_set_sector_type(g, ARENA, SECT_WATER_SWIM);), false, 1},
    {"a spent wellspring, quenched, at a lake", "drink", false, SETUP(thirsty(g, att, 31); wellspring(g, att); room_set_sector_type(g, ARENA, SECT_WATER_SWIM);), false, 1},
    {"a wellspring, at a lake", "drink", false, SETUP(thirsty(g, att); char_set_skills_base(att, SKILL_WELLSPRING, 50); room_set_sector_type(g, ARENA, SECT_WATER_SWIM);), false, 1},
    {"a spent wellspring, sunken", "drink", false, SETUP(thirsty(g, att); wellspring(g, att); room_set_geffect(g, ARENA, -1);), false, 1},
    {"a nearly full wellspring, sunken", "drink", false, SETUP(thirsty(g, att); wellspring(g, att, false); room_set_geffect(g, ARENA, -1);), false, 1},
    {"a deep spent wellspring, sunken", "drink", false, SETUP(thirsty(g, att); wellspring(g, att, true, true); room_set_geffect(g, ARENA, -1);), false, 1},
    {"watching the dummy drink", "drink", false, SETUP(command_interpreter(g, dummy(g), "drink");), false, 1},
    {"a wellspring, sunken", "drink", false, SETUP(thirsty(g, att); char_set_skills_base(att, SKILL_WELLSPRING, 50); room_set_geffect(g, ARENA, -1);), false, 1},
    {"", "drink flask", false, SETUP(thirsty(g, att);), false, 1},
    {"with a potion", "drink potion", false, SETUP(thirsty(g, att); drained(g, att); carry(g, att, {.name = "potion", .vnum = 86});), false, 1},
    {"by a potion", "drink potion", false, SETUP(thirsty(g, att); here(g, {.name = "potion", .vnum = 86});), false, 1},
    {"with a pebble", "drink pebble", false, SETUP(thirsty(g, att); pebbles(g, att, 1);), false, 1},
    {"by a flask", "drink flask", false, SETUP(thirsty(g, att); here(g, {.name = "flask", .type = ITEM_DRINKCON, .values = {{0, 10}, {1, 10}}});), false, 1},
    {"by a fountain", "drink fountain", false, SETUP(thirsty(g, att); here(g, {.name = "fountain", .type = ITEM_FOUNTAIN, .values = {{0, -1}, {1, 10}}});), false, 1},
    {"with a broken flask", "drink flask", false, SETUP(thirsty(g, att); flask(g, att, LIQ_WATER, 10, 10, {ITEM_BROKEN});), false, 1},
    {"with a fake flask", "drink flask", false, SETUP(thirsty(g, att); flask(g, att, LIQ_WATER, 10, 10, {ITEM_FORGED});), false, 1},
    {"drunk, with a flask", "drink flask", false, SETUP(thirsty(g, att, 10, 20, 11); flask(g, att);), false, 1},
    {"drunk and parched, with a flask", "drink flask", false, SETUP(thirsty(g, att, 0, 20, 11); flask(g, att);), false, 1},
    {"with an empty flask", "drink flask", false, SETUP(thirsty(g, att); flask(g, att, LIQ_WATER, 0);), false, 1},
    {"with a flask", "drink flask", false, SETUP(thirsty(g, att); flask(g, att);), false, 1},
    {"refreshed, with a flask", "drink flask", false, SETUP(thirsty(g, att); char_set_foodr(att, 2); flask(g, att);), false, 1},
    {"with a flask", "sip flask", false, SETUP(thirsty(g, att); flask(g, att);), false, 1},
    {"with a poisoned flask", "sip flask", false, SETUP(thirsty(g, att); flask(g, att, LIQ_WATER, 10, 10, {}, 1);), false, 1},
    {"with a poisoned flask", "drink flask", false, SETUP(thirsty(g, att); flask(g, att, LIQ_WATER, 10, 10, {}, 1);), false, 1},
    {"a mutant with a poisoned flask", "drink flask", false, SETUP(thirsty(g, att); char_set_race(att, RACE_MUTANT); char_set_genome(att, 0, 7); flask(g, att, LIQ_WATER, 10, 10, {}, 1);), false, 1},
    {"a mutant of the other kind with a poisoned flask", "drink flask", false, SETUP(thirsty(g, att); char_set_race(att, RACE_MUTANT); char_set_genome(att, 1, 7); flask(g, att, LIQ_WATER, 10, 10, {}, 1);), false, 1},
    {"a plain mutant with a poisoned flask", "drink flask", false, SETUP(thirsty(g, att); char_set_race(att, RACE_MUTANT); flask(g, att, LIQ_WATER, 10, 10, {}, 1);), false, 1},
    {"with a nearly empty flask", "drink flask", false, SETUP(thirsty(g, att); flask(g, att, LIQ_WATER, 2);), false, 1},
    {"with a nearly empty poisoned flask", "drink flask", false, SETUP(thirsty(g, att); flask(g, att, LIQ_WATER, 3, 10, {}, 1);), false, 1},
    {"with a bottomless flask", "drink flask", false, SETUP(thirsty(g, att); flask(g, att, LIQ_WATER, 10, -1);), false, 1},
    {"with a light flask", "drink flask", false, SETUP(thirsty(g, att); carry(g, att, {.name = "flask", .type = ITEM_DRINKCON, .weight = 2, .values = {{0, 10}, {1, 10}}});), false, 1},
    {"with a described flask", "drink flask", false, SETUP(thirsty(g, att); obj_set_action_description(flask(g, att), "The water is cold.");), false, 1},
    {"with a flask of beer", "drink flask", false, SETUP(thirsty(g, att, 10, 20, 9); flask(g, att, LIQ_BEER);), false, 1},
    {"nearly quenched, with a flask", "drink flask", false, SETUP(thirsty(g, att, 46); flask(g, att);), false, 1},
    {"a spent wellspring, with a flask", "drink flask", false, SETUP(thirsty(g, att); wellspring(g, att); flask(g, att);), false, 1},
    {"a deep spent wellspring, with a flask", "drink flask", false, SETUP(thirsty(g, att); wellspring(g, att, true, true); flask(g, att);), false, 1},
    {"with the last of a flask of water", "drink flask", false, SETUP(thirsty(g, att); flask(g, att, LIQ_WATER, 2, 10, {}, 0, "flask water");), false, 1},
    {"a nearly full wellspring, with a flask", "drink flask", false, SETUP(thirsty(g, att); wellspring(g, att, false); flask(g, att);), false, 1},
    {"a spent wellspring, with a flask of beer", "drink flask", false, SETUP(thirsty(g, att); wellspring(g, att); flask(g, att, LIQ_BEER);), false, 1},
    {"a spent wellspring, with a flask of the 14th", "drink flask", false, SETUP(thirsty(g, att); wellspring(g, att); flask(g, att, 14);), false, 1},
    {"a spent wellspring, with a flask of the 15th", "drink flask", false, SETUP(thirsty(g, att); wellspring(g, att); flask(g, att, 15);), false, 1},
    {"a spent wellspring, with a flask", "sip flask", false, SETUP(thirsty(g, att); wellspring(g, att); flask(g, att);), false, 1},
    {"a quenched spent wellspring, with a flask", "drink flask", false, SETUP(thirsty(g, att, 31); wellspring(g, att); flask(g, att);), false, 1},

    /* eat and taste */
    {"an android", "eat bread", false, SETUP(char_set_race(att, RACE_ANDROID);), false, 1},
    {"never hungry", "eat bread", false, SETUP(conditions(g, att, -1, 20);), false, 1},
    {"in a healing tank", "eat bread", false, SETUP(conditions(g, att, 10, 20); char_plr_set(att, PLR_HEALT, true);), false, 1},
    {"poisoned", "eat bread", false, SETUP(conditions(g, att, 10, 20); char_aff_set(att, AFF_POISON, true);), false, 1},
    {"", "eat", false, SETUP(conditions(g, att, 10, 20);), false, 1},
    {"", "eat bread", false, SETUP(conditions(g, att, 10, 20);), false, 1},
    {"parched, with bread", "eat bread", false, SETUP(conditions(g, att, 10, 1); bread(g, att);), false, 1},
    {"parched and starving, with bread", "eat bread", false, SETUP(conditions(g, att, 1, 1); bread(g, att);), false, 1},
    {"with a flask", "taste flask", false, SETUP(conditions(g, att, 10, 20); flask(g, att);), false, 1},
    {"with a fountain", "taste fountain", false, SETUP(conditions(g, att, 10, 20); carry(g, att, {.name = "fountain", .type = ITEM_FOUNTAIN, .values = {{0, -1}, {1, 10}}});), false, 1},
    {"with a flask", "eat flask", false, SETUP(conditions(g, att, 10, 20); flask(g, att);), false, 1},
    {"with a pebble", "eat pebble", false, SETUP(conditions(g, att, 10, 20); pebbles(g, att, 1);), false, 1},
    {"a majin with a candy", "eat candy", false, SETUP(conditions(g, att, 10, 20); char_set_race(att, RACE_MAJIN); carry(g, att, {.name = "candy", .vnum = 53, .type = ITEM_FOOD});), false, 1},
    {"a majin with a good candy", "eat candy", false, SETUP(conditions(g, att, 10, 20); char_set_race(att, RACE_MAJIN); carry(g, att, {.name = "candy", .vnum = 93, .type = ITEM_FOOD});), false, 1},
    {"a majin with a better candy", "eat candy", false, SETUP(conditions(g, att, 10, 20); char_set_race(att, RACE_MAJIN); carry(g, att, {.name = "candy", .vnum = 94, .type = ITEM_FOOD});), false, 1},
    {"a majin with the best candy", "eat candy", false, SETUP(conditions(g, att, 10, 20); char_set_race(att, RACE_MAJIN); carry(g, att, {.name = "candy", .vnum = 95, .type = ITEM_FOOD});), false, 1},
    {"a majin with a fake candy", "eat candy", false, SETUP(conditions(g, att, 10, 20); char_set_race(att, RACE_MAJIN); carry(g, att, {.name = "candy", .vnum = 94, .type = ITEM_FOOD, .flags = {ITEM_FORGED}});), false, 1},
    {"with a candy", "eat candy", false, SETUP(conditions(g, att, 10, 20); carry(g, att, {.name = "candy", .vnum = 93, .type = ITEM_FOOD});), false, 1},
    {"a majin at level 100 with a candy", "eat candy", false, SETUP(conditions(g, att, 10, 20); char_set_race(att, RACE_MAJIN); char_set_class_level(att, 100); carry(g, att, {.name = "candy", .vnum = 93, .type = ITEM_FOOD});), false, 1},
    {"full, with bread", "eat bread", false, SETUP(conditions(g, att, 48, 20); bread(g, att);), false, 1},
    {"a full majin, with bread", "eat bread", false, SETUP(conditions(g, att, 48, 20); char_set_race(att, RACE_MAJIN); bread(g, att);), false, 1},
    {"with bread", "eat bread", false, SETUP(conditions(g, att, 10, 20); bread(g, att);), false, 1},
    {"hurt, with bread", "eat bread", false, SETUP(conditions(g, att, 10, 20); decCurHealthPercent(g, att, 0.5, 1); bread(g, att);), false, 1},
    {"hurt, with a big loaf", "eat bread", false, SETUP(conditions(g, att, 10, 20); decCurHealthPercent(g, att, 0.5, 1); bread(g, att, 10, {}, 0, ITEM_FOOD, 6);), false, 1},
    {"hurt, with a treat", "eat bread", false, SETUP(conditions(g, att, 10, 20); decCurHealthPercent(g, att, 0.5, 1); bread(g, att, 10, {}, 0, ITEM_YUM);), false, 1},
    {"hurt, with a sweet loaf", "eat bread", false, SETUP(conditions(g, att, 10, 20); decCurHealthPercent(g, att, 0.5, 1); bread(g, att, 10, {ITEM_ONLY_WIZARD}); /* as eating reads it: extra flag 33, meant as type YUM */), false, 1},
    {"watching the dummy eat", "eat", false, SETUP(command_interpreter(g, dummy(g), "eat bread");), false, 1},
    {"refreshed, with bread", "eat bread", false, SETUP(conditions(g, att, 10, 20); char_set_foodr(att, 2); bread(g, att);), false, 1},
    {"with a described loaf", "eat bread", false, SETUP(conditions(g, att, 10, 20); obj_set_action_description(bread(g, att), "It is stale.");), false, 1},
    {"nearly full, with bread", "eat bread", false, SETUP(conditions(g, att, 40, 20); bread(g, att);), false, 1},
    {"a nearly full majin, with bread", "eat bread", false, SETUP(conditions(g, att, 40, 20); char_set_race(att, RACE_MAJIN); bread(g, att);), false, 1},
    {"with the last bite", "eat bread", false, SETUP(conditions(g, att, 10, 20); bread(g, att, 1);), false, 1},
    {"with spent bread", "eat bread", false, SETUP(conditions(g, att, 10, 20); bread(g, att, 0);), false, 1},
    {"with poisoned bread", "eat bread", false, SETUP(conditions(g, att, 10, 20); bread(g, att, 10, {}, 1);), false, 1},
    {"an immortal with poisoned bread", "eat bread", false, SETUP(conditions(g, att, 10, 20); admin_set(g, att, ADMLVL_IMPL); bread(g, att, 10, {}, 1);), false, 1},
    {"with bread", "taste bread", false, SETUP(conditions(g, att, 10, 20); bread(g, att);), false, 1},
    {"with poisoned bread", "taste bread", false, SETUP(conditions(g, att, 10, 20); bread(g, att, 10, {}, 1);), false, 1},
    {"with the last bite", "taste bread", false, SETUP(conditions(g, att, 10, 20); bread(g, att, 1);), false, 1},

    /* pour and fill */
    {"", "pour", false, SETUP(), false, 1},
    {"", "pour flask", false, SETUP(), false, 1},
    {"with a pebble", "pour pebble out", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with an empty flask", "pour flask out", false, SETUP(flask(g, att, LIQ_WATER, 0);), false, 1},
    {"with a flask", "pour flask", false, SETUP(flask(g, att);), false, 1},
    {"with a flask", "pour flask out", false, SETUP(flask(g, att);), false, 1},
    {"with a flask of water", "pour flask out", false, SETUP(flask(g, att, LIQ_WATER, 10, 10, {}, 0, "flask water");), false, 1},
    {"with a flask of something unknown", "pour flask out", false, SETUP(flask(g, att, 99);), false, 1},
    {"with a bottomless flask", "pour flask out", false, SETUP(flask(g, att, LIQ_WATER, 10, -1);), false, 1},
    {"with a flask", "pour flask jug", false, SETUP(flask(g, att);), false, 1},
    {"with a flask and a pebble", "pour flask pebble", false, SETUP(flask(g, att); pebbles(g, att, 1);), false, 1},
    {"with a flask", "pour flask flask", false, SETUP(flask(g, att);), false, 1},
    {"with a flask and a jug of beer", "pour flask jug", false, SETUP(flask(g, att); flask(g, att, LIQ_BEER, 5, 20, {}, 0, "jug");), false, 1},
    {"with a flask and a full jug", "pour flask jug", false, SETUP(flask(g, att); flask(g, att, LIQ_WATER, 20, 20, {}, 0, "jug");), false, 1},
    {"with a flask and a bottomless jug", "pour flask jug", false, SETUP(flask(g, att); flask(g, att, LIQ_WATER, 0, -1, {}, 0, "jug");), false, 1},
    {"with a flask and an empty jug", "pour flask jug", false, SETUP(flask(g, att); flask(g, att, LIQ_BEER, 0, 20, {}, 0, "jug");), false, 1},
    {"with a flask and a half-full jug", "pour flask jug", false, SETUP(flask(g, att); flask(g, att, LIQ_WATER, 15, 20, {}, 0, "jug");), false, 1},
    {"with a bottomless flask and a jug", "pour flask jug", false, SETUP(flask(g, att, LIQ_WATER, 10, -1); flask(g, att, LIQ_WATER, 5, 20, {}, 0, "jug");), false, 1},
    {"with a poisoned flask and a jug", "pour flask jug", false, SETUP(flask(g, att, LIQ_WATER, 10, 10, {}, 1); flask(g, att, LIQ_WATER, 0, 20, {}, 0, "jug");), false, 1},
    {"with a flask and a fountain", "pour flask fountain", false, SETUP(flask(g, att); carry(g, att, {.name = "fountain", .type = ITEM_FOUNTAIN, .values = {{0, 50}, {1, 0}}});), false, 1},
    {"", "fill", false, SETUP(), false, 1},
    {"", "fill jug", false, SETUP(), false, 1},
    {"with a pebble", "fill pebble", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with a jug", "fill jug", false, SETUP(flask(g, att, LIQ_WATER, 0, 20, {}, 0, "jug");), false, 1},
    {"with a jug", "fill jug fountain", false, SETUP(flask(g, att, LIQ_WATER, 0, 20, {}, 0, "jug");), false, 1},
    {"with a jug, by a rock", "fill jug rock", false, SETUP(flask(g, att, LIQ_WATER, 0, 20, {}, 0, "jug"); here(g, {.name = "rock"});), false, 1},
    {"with a jug, by a broken fountain", "fill jug fountain", false, SETUP(flask(g, att, LIQ_WATER, 0, 20, {}, 0, "jug"); here(g, {.name = "fountain", .type = ITEM_FOUNTAIN, .values = {{0, -1}, {1, 10}}, .flags = {ITEM_BROKEN}});), false, 1},
    {"with a jug, by a broken rock", "fill jug rock", false, SETUP(flask(g, att, LIQ_WATER, 0, 20, {}, 0, "jug"); here(g, {.name = "rock", .values = {{1, 5}}, .flags = {ITEM_BROKEN}});), false, 1},
    {"with a jug, by a dry fountain", "fill jug fountain", false, SETUP(flask(g, att, LIQ_WATER, 0, 20, {}, 0, "jug"); here(g, {.name = "fountain", .type = ITEM_FOUNTAIN, .values = {{0, -1}, {1, 0}}});), false, 1},
    {"with a jug, by a fountain", "fill jug fountain", false, SETUP(flask(g, att, LIQ_WATER, 0, 20, {}, 0, "jug"); here(g, {.name = "fountain", .type = ITEM_FOUNTAIN, .values = {{0, -1}, {1, 10}}});), false, 1},
    {"with a jug, by a small fountain", "fill jug fountain", false, SETUP(flask(g, att, LIQ_WATER, 0, 20, {}, 0, "jug"); here(g, {.name = "fountain", .type = ITEM_FOUNTAIN, .values = {{0, 50}, {1, 10}}});), false, 1},

    /* wear */
    {"", "wear", false, SETUP(), false, 1},
    {"", "wear all.ring finger", false, SETUP(), false, 1},
    {"empty-handed", "wear all", false, SETUP(unarmed(g, att);), false, 1},
    {"with a wardrobe", "wear all", false, SETUP(wardrobe(g, att);), false, 1},
    {"with rings beyond them", "wear all", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER, {}, 50); wearable(g, att, "ring", ITEM_WEAR_FINGER, {ITEM_BROKEN}); wearable(g, att, "ring", ITEM_WEAR_FINGER, {ITEM_FORGED});), false, 1},
    {"with armor beyond them", "wear all", false, SETUP(wearable(g, att, "plate", ITEM_WEAR_BODY, {}, 0, ITEM_ARMOR); obj_set_value(first_obj(g, char_get_inventory(att)), VAL_ARMOR_SKILL, 0);), false, 1},
    {"with rings", "wear all.ring", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER); wearable(g, att, "ring", ITEM_WEAR_FINGER); wearable(g, att, "ring", ITEM_WEAR_FINGER); pebbles(g, att, 1);), false, 1},
    {"with rings", "wear all.", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"with rings", "wear all.band", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"with a ring beyond them", "wear all.ring", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER, {}, 50);), false, 1},
    {"with pebbles", "wear all.pebble", false, SETUP(pebbles(g, att, 2);), false, 1},
    {"with plates of armor", "wear all.plate", false, SETUP(wearable(g, att, "plate", ITEM_WEAR_BODY, {}, 0, ITEM_ARMOR); obj_set_value(first_obj(g, char_get_inventory(att)), VAL_ARMOR_SKILL, 0);), false, 1},
    {"", "wear ring", false, SETUP(), false, 1},
    {"with a broken ring", "wear ring", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER, {ITEM_BROKEN});), false, 1},
    {"with a fake ring", "wear ring", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER, {ITEM_FORGED});), false, 1},
    {"with a ring beyond them", "wear ring", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER, {}, 50);), false, 1},
    {"with a ring", "wear ring", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"with a ring", "wear ring finger", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"with a ring", "wear ring elbow", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"with a ring", "wear ring ELBOW", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"with a ring", "wear ring !elbow", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"with a ring", "wear ring body", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"with a pebble", "wear pebble", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with a plate of armor", "wear plate", false, SETUP(wearable(g, att, "plate", ITEM_WEAR_BODY, {}, 0, ITEM_ARMOR); obj_set_value(first_obj(g, char_get_inventory(att)), VAL_ARMOR_SKILL, 0);), false, 1},
    {"with a ring, without fingers", "wear ring", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER); char_body_set(att, WEAR_FINGER_R, false);), false, 1},
    {"with a ring, without fingers", "wear ring finger", false, SETUP(wearable(g, att, "ring", ITEM_WEAR_FINGER); char_body_set(att, WEAR_FINGER_R, false);), false, 1},
    {"wearing a ring, with another", "wear ring", false, SETUP(worn(g, att, {.name = "band", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R); wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"wearing two rings, with another", "wear ring", false, SETUP(worn(g, att, {.name = "band", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R); worn(g, att, {.name = "band", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_L); wearable(g, att, "ring", ITEM_WEAR_FINGER);), false, 1},
    {"wearing a hat, with another", "wear hat", false, SETUP(worn(g, att, {.name = "cap", .wear = {ITEM_WEAR_HEAD}}, WEAR_HEAD); wearable(g, att, "hat", ITEM_WEAR_HEAD);), false, 1},
    {"armed, with a great sword", "wield sword", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT); wearable(g, att, "sword", ITEM_WEAR_WIELD, {ITEM_2H}, 0, ITEM_WEAPON);), false, 1},
    {"with a great sword", "wield sword", false, SETUP(unarmed(g, att); wearable(g, att, "sword", ITEM_WEAR_WIELD, {ITEM_2H}, 0, ITEM_WEAPON);), false, 1},
    {"two-handing, with a shield", "wear shield", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT); two_handed(att, 50); wearable(g, att, "shield", ITEM_WEAR_SHIELD);), false, 1},
    {"holding a great sword, with a shield", "wear shield", false, SETUP(unarmed(g, att); equip_char(g, att, make(g, {.name = "claymore", .type = ITEM_WEAPON, .flags = {ITEM_2H}, .wear = {ITEM_WEAR_WIELD}}), WEAR_WIELD1); wearable(g, att, "shield", ITEM_WEAR_SHIELD);), false, 1},
    {"armed, with a custom sword", "wield blade", false, SETUP(unarmed(g, att); wearable(g, att, "blade", ITEM_WEAR_WIELD, {ITEM_CUSTOM}, 0, ITEM_WEAPON);), false, 1},
    {"a novice, with a custom sword", "wield blade", false, SETUP(unarmed(g, att); char_set_class_level(att, 19); wearable(g, att, "blade", ITEM_WEAR_WIELD, {ITEM_CUSTOM}, 0, ITEM_WEAPON);), false, 1},

    /* wield, and grab or hold */
    {"", "wield", false, SETUP(), false, 1},
    {"", "wield blade", false, SETUP(), false, 1},
    {"with a pebble", "wield pebble", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with a heavy blade", "wield blade", false, SETUP(unarmed(g, att); carry(g, att, {.name = "blade", .type = ITEM_WEAPON, .weight = getEffMaxPL(g, att) / 200 + char_get_aff_abils_str(att) * 50 + 1, .wear = {ITEM_WEAR_WIELD}});), false, 1},
    {"holding a great sword in the off hand, with a blade", "wield blade", false, SETUP(unarmed(g, att); equip_char(g, att, make(g, {.name = "claymore", .type = ITEM_WEAPON, .flags = {ITEM_2H}, .wear = {ITEM_WEAR_WIELD}}), WEAR_WIELD2); wearable(g, att, "blade", ITEM_WEAR_WIELD, {}, 0, ITEM_WEAPON);), false, 1},
    {"with a broken blade", "wield blade", false, SETUP(unarmed(g, att); wearable(g, att, "blade", ITEM_WEAR_WIELD, {ITEM_BROKEN}, 0, ITEM_WEAPON);), false, 1},
    {"with a fake blade", "wield blade", false, SETUP(unarmed(g, att); wearable(g, att, "blade", ITEM_WEAR_WIELD, {ITEM_FORGED}, 0, ITEM_WEAPON);), false, 1},
    {"with a blade beyond them", "wield blade", false, SETUP(unarmed(g, att); wearable(g, att, "blade", ITEM_WEAR_WIELD, {}, 50, ITEM_WEAPON);), false, 1},
    {"two-handing, with a blade", "wield blade", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT); two_handed(att, 50); wearable(g, att, "blade", ITEM_WEAR_WIELD, {}, 0, ITEM_WEAPON);), false, 1},
    {"with a blade", "wield blade", false, SETUP(unarmed(g, att); wearable(g, att, "blade", ITEM_WEAR_WIELD, {}, 0, ITEM_WEAPON);), false, 1},
    {"armed, with a blade", "wield blade", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT); wearable(g, att, "blade", ITEM_WEAR_WIELD, {}, 0, ITEM_WEAPON);), false, 1},
    {"dual wielding, with a blade", "wield blade", false, SETUP(dual(g, att, TYPE_SLASH - TYPE_HIT); wearable(g, att, "blade", ITEM_WEAR_WIELD, {}, 0, ITEM_WEAPON);), false, 1},
    {"with wieldable armor", "wield plate", false, SETUP(unarmed(g, att); wearable(g, att, "plate", ITEM_WEAR_WIELD, {}, 0, ITEM_ARMOR); obj_set_value(first_obj(g, char_get_inventory(att)), VAL_WEAPON_SKILL, SKILL_SWORD);), false, 1},
    {"", "hold", false, SETUP(), false, 1},
    {"", "grab torch", false, SETUP(), false, 1},
    {"with a torch beyond them", "hold torch", false, SETUP(wearable(g, att, "torch", ITEM_WEAR_HOLD, {}, 50, ITEM_LIGHT);), false, 1},
    {"two-handing, with a torch", "hold torch", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT); two_handed(att, 50); wearable(g, att, "torch", ITEM_WEAR_HOLD, {}, 0, ITEM_LIGHT);), false, 1},
    {"with a torch", "hold torch", false, SETUP(obj_set_value(wearable(g, att, "torch", ITEM_WEAR_HOLD, {}, 0, ITEM_LIGHT), VAL_LIGHT_HOURS, 5);), false, 1},
    {"with an everlasting torch", "hold torch", false, SETUP(obj_set_value(wearable(g, att, "torch", ITEM_WEAR_HOLD, {}, 0, ITEM_LIGHT), VAL_LIGHT_HOURS, -1);), false, 1},
    {"with a burnt torch", "hold torch", false, SETUP(wearable(g, att, "torch", ITEM_WEAR_HOLD, {}, 0, ITEM_LIGHT);), false, 1},
    {"holding something, with a torch", "hold torch", false, SETUP(dual(g, att, TYPE_SLASH - TYPE_HIT); obj_set_value(wearable(g, att, "torch", ITEM_WEAR_HOLD, {}, 0, ITEM_LIGHT), VAL_LIGHT_HOURS, 5);), false, 1},
    {"with a pebble", "hold pebble", false, SETUP(pebbles(g, att, 1);), false, 1},
    {"with a charm", "hold charm", false, SETUP(wearable(g, att, "charm", ITEM_WEAR_HOLD);), false, 1},
    {"with a wand", "hold wand", false, SETUP(carry(g, att, {.name = "wand", .type = ITEM_WAND});), false, 1},
    {"with a staff", "hold staff", false, SETUP(carry(g, att, {.name = "staff", .type = ITEM_STAFF});), false, 1},
    {"with a scroll", "hold scroll", false, SETUP(carry(g, att, {.name = "scroll", .type = ITEM_SCROLL});), false, 1},
    {"with a potion", "hold potion", false, SETUP(carry(g, att, {.name = "potion", .type = ITEM_POTION});), false, 1},

    /* remove */
    {"", "remove", false, SETUP(), false, 1},
    {"by a board", "remove 1", false, SETUP(here(g, {.name = "board", .type = ITEM_BOARD});), false, 1},
    {"with a board", "remove 1", false, SETUP(carry(g, att, {.name = "board", .type = ITEM_BOARD});), false, 1},
    {"by a board", "remove 0", false, SETUP(here(g, {.name = "board", .type = ITEM_BOARD});), false, 1},
    {"by a board", "remove ring", false, SETUP(here(g, {.name = "board", .type = ITEM_BOARD}); worn(g, att, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R);), false, 1},
    {"naked", "remove all", false, SETUP(unarmed(g, att);), false, 1},
    {"wearing rings", "remove all", false, SETUP(worn(g, att, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R); worn(g, att, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_L);), false, 1},
    {"wearing rings", "remove all.ring", false, SETUP(worn(g, att, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R); worn(g, att, {.name = "hat", .wear = {ITEM_WEAR_HEAD}}, WEAR_HEAD);), false, 1},
    {"wearing rings", "remove all.", false, SETUP(worn(g, att, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R);), false, 1},
    {"wearing rings", "remove all.band", false, SETUP(worn(g, att, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R);), false, 1},
    {"", "remove ring", false, SETUP(), false, 1},
    {"wearing a ring", "remove ring", false, SETUP(worn(g, att, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R);), false, 1},
    {"wearing a cursed ring", "remove ring", false, SETUP(worn(g, att, {.name = "ring", .flags = {ITEM_NODROP}, .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R);), false, 1},
    {"an immortal wearing a cursed ring", "remove ring", false, SETUP(admin_set(g, att, ADMLVL_IMMORT); worn(g, att, {.name = "ring", .flags = {ITEM_NODROP}, .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R);), false, 1},
    {"crowded, wearing a ring", "remove ring", false, SETUP(worn(g, att, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}, WEAR_FINGER_R); crowded(g, att);), false, 1},
    {"wearing a ring of strength", "remove ring", false, SETUP({ obj_data *r = make(g, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}); obj_get_affected(r)[0].location = APPLY_STR; obj_get_affected(r)[0].modifier = 20; equip_char(g, att, r, WEAR_FINGER_R); restoreVitals(att, false); }), false, 1},
    {"wearing a ring of might", "remove ring", false, SETUP({ obj_data *r = make(g, {.name = "ring", .wear = {ITEM_WEAR_FINGER}}); obj_get_affected(r)[0].location = APPLY_HIT; obj_get_affected(r)[0].modifier = 50000; equip_char(g, att, r, WEAR_FINGER_R); restoreVitals(att, false); }), false, 1},
    {"two-handing", "remove sword", false, SETUP(wield(g, att, TYPE_SLASH - TYPE_HIT); two_handed(att, 50);), false, 1},
  };
} // namespace

const std::span<const scene> SCENES_ITEMS = list;
