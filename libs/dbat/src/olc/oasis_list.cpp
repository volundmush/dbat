/******************************************************************************/
/** OasisOLC - InGame OLC Listings                                     v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/

#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/db.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/shop.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/guild.hpp"
#include "dbat/races.hpp"
#include "dbat/class.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/olc/commands.hpp"

namespace {
  /* What a list command lists: the vnums of a zone, or the range typed (asked_min to asked_max) */
  struct listing {
    struct zone_data *zone{};
    IDXTYPE asked_min = NOWHERE, asked_max = NOWHERE;
    IDXTYPE bottom() const { return zone ? zone->bot : asked_min; }
    IDXTYPE top() const { return zone ? zone->top : asked_max; }
    bool holds(IDXTYPE vnum) const { return vnum >= bottom() && vnum <= top(); }
  };

  void list_rooms(GameWorld *g, struct char_data *ch, const listing &range);
  void list_mobiles(GameWorld *g, struct char_data *ch, const listing &range);
  void list_objects(GameWorld *g, struct char_data *ch, const listing &range);
  void list_shops(GameWorld *g, struct char_data *ch, const listing &range);
  void list_triggers(GameWorld *g, struct char_data *ch, const listing &range);
  void list_zones(GameWorld *g, struct char_data *ch);
  void list_guilds_in(GameWorld *g, struct char_data *ch, const listing &range) { list_guilds(g, ch, range.zone, range.asked_min, range.asked_max); }

  /* The list commands, by their subcommand */
  constexpr std::pair<int, void (*)(GameWorld *, struct char_data *, const listing &)> LISTS[] = {
      {SCMD_OASIS_RLIST, list_rooms    },
      {SCMD_OASIS_OLIST, list_objects  },
      {SCMD_OASIS_MLIST, list_mobiles  },
      {SCMD_OASIS_TLIST, list_triggers },
      {SCMD_OASIS_SLIST, list_shops    },
      {SCMD_OASIS_GLIST, list_guilds_in},
  };
} // namespace

ACMD(do_oasis_list)
{
  std::string_view rest = argument;
  std::string smin      = take_arg(rest);
  std::string smax      = take_arg(rest);

  if (subcmd == SCMD_OASIS_ZLIST) { /* special case */
    if (!smin.empty() && is_number(smin))
      print_zone(g, ch, parse_int(smin));
    else
      list_zones(g, ch);
    return;
  }

  listing range;
  if (smin.empty() || smin[0] == '.')
    range.zone = room_get_zone(g, char_get_in_room(ch));
  else if (smax.empty()) {
    if (!(range.zone = find_zone(g, parse_int(smin)))) {
      send_to_char(g, ch, "Sorry, there's no zone with that number\r\n");
      return;
    }
  } else {
    range.asked_min = parse_int(smin);
    range.asked_max = parse_int(smax);
    if (range.asked_min + 500 < range.asked_max) {
      send_to_char(g, ch, "Really? Over 500?! You need to view that many at once? Come on...\r\n");
      return;
    }
    if (range.asked_min > range.asked_max) {
      send_to_char(g, ch, "List from {:d} to {:d} - Aren't we funny today!\r\n", range.asked_min, range.asked_max);
      return;
    }
  }

  auto list = std::ranges::find(LISTS, subcmd, &std::pair<int, void (*)(GameWorld *, struct char_data *, const listing &)>::first);
  if (list == std::end(LISTS)) {
    send_to_char(g, ch, "You can't list that!\r\n");
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: do_oasis_list: Unknown list option: {:d}", subcmd);
    return;
  }
  list->second(g, ch, range);
}

ACMD(do_oasis_links)
{
  struct zone_data *zrnum;
  zone_vnum zvnum;
  room_vnum to_room;
  room_vnum first, last;
  int j;
  std::string arg;

  std::string_view rest = argument;
  arg                   = take_arg(rest);

  if (arg == "." || arg.empty()) {
    zrnum = room_get_zone(g, char_get_in_room(ch));
    zvnum = zrnum->number;
  } else {
    zvnum = parse_int(arg);
    zrnum = find_zone(g, zvnum);
  }

  if (zrnum == nullptr || zvnum == NOWHERE) {
    send_to_char(g, ch, "No zone was found with that number.\n\r");
    return;
  }

  last  = zrnum->top;
  first = zrnum->bot;

  send_to_char(g, ch, "Zone {:d} is linked to the following zones:\r\n", zvnum);
  for (room_vnum nr : g->rooms.vnums_in(first, last)) {
    for (j = 0; j < NUM_OF_DIRS; j++) {
      if (room_get_dir_option(g, nr, j)) {
        to_room = room_get_dir_option(g, nr, j)->to_room;
        if (to_room != NOWHERE && (zrnum != room_get_zone(g, to_room)))
          send_to_char(g, ch, "{:>3d} {:<30} at {:>5d} ({:<5}) ---> {:>5d}\r\n",
                       room_get_zone(g, to_room)->number,
                       room_get_zone(g, to_room)->name,
                       nr, dirs[j], to_room);
      }
    }
  }
}

namespace {
  void list_rooms(GameWorld *g, struct char_data *ch, const listing &range)
  {
    send_to_char(g, ch, "@nIndex VNum    Room Name                                Exits\r\n----- ------- ---------------------------------------- -----@n\r\n");
    int counter = 0;
    for (room_vnum i : g->rooms.vnums_in(range.bottom(), range.top())) {
      counter++;
      send_to_char(g, ch, "{:>4d}) [@g{:<5d}@n] @[1]{}@n {}", counter, i,
                   printf_width(room_get_name(g, i), count_color_chars(room_get_name(g, i)) + 44, true), !room_get_proto_script(g, i).empty() ? "[TRIG] " : "");
      for (int j = 0; j < NUM_OF_DIRS; j++) /* the exits out of the zone */
        if (const room_direction_data *exit = room_get_dir_option(g, i, j); exit && exit->to_room != NOWHERE && room_get_zone(g, exit->to_room) != room_get_zone(g, i))
          send_to_char(g, ch, "(@y{:d}@n)", exit->to_room);
      send_to_char(g, ch, "\r\n");
    }
    if (counter == 0)
      send_to_char(g, ch, "No rooms found for zone/range specified.\r\n");
  }

  void list_mobiles(GameWorld *g, struct char_data *ch, const listing &range)
  {
    send_to_char(g, ch, "@nIndex VNum    Mobile Name                    Race      Class     Level\r\n----- ------- -------------------------      --------- --------- -----\r\n");
    if (g->mob_proto.empty())
      return;
    int counter = 0;
    for (mob_vnum i : g->mob_proto.vnums()) {
      if (!range.holds(i))
        continue;
      struct char_data *mob = mob_proto_of(g, i);
      send_to_char(g, ch, "@g{:>4d}@n) [@g{:<5d}@n] @[3]{} @C{:<9} @c{:<9} @y[{:>4d}]@n {}\r\n", ++counter, i,
                   printf_width(char_get_sdesc(mob), count_color_chars(char_get_sdesc(mob)) + 30, true), char_true_race_name(mob), char_sensei_name(mob),
                   char_get_class_level(mob) + char_get_level_adj(mob) + char_get_race_level(mob), !char_get_proto_script(mob).empty() ? " [TRIG]" : "");
    }
    if (counter == 0)
      send_to_char(g, ch, "None found.\r\n");
  }

  void list_objects(GameWorld *g, struct char_data *ch, const listing &range)
  {
    send_to_char(g, ch, "@nIndex VNum    Object Name                                  Object Type\r\n----- ------- -------------------------------------------- ----------------\r\n");
    if (g->obj_proto.empty())
      return;
    int counter = 0;
    for (obj_vnum i : g->obj_proto.vnums()) {
      if (!range.holds(i))
        continue;
      struct obj_data *obj = obj_proto_of(g, i);
      send_to_char(g, ch, "@g{:>4d}@n) [@g{:<5d}@n] @[2]{} @y[{}]@n{}\r\n", ++counter, i,
                   printf_width(obj_get_short_description(obj), count_color_chars(obj_get_short_description(obj)) + 44, true),
                   item_types[obj_get_type_flag(obj)], !obj_get_proto_script(obj).empty() ? " [TRIG]" : "");
    }
    if (counter == 0)
      send_to_char(g, ch, "None found.\r\n");
  }

  void list_shops(GameWorld *g, struct char_data *ch, const listing &range)
  {
    send_to_char(g, ch, "Index VNum    Shop Room(s)\r\n----- ------- ---------------------------------------------\r\n");
    int counter = 0;
    for (struct shop_data *shop : g->shops.all()) {
      if (!range.holds(shop_get_vnum(shop)))
        continue;
      send_to_char(g, ch, "@g{:>4d}@n) [@g{:<5d}@n]", ++counter, shop_get_vnum(shop));
      const std::vector<room_vnum> &rooms = shop_rooms(shop);
      for (size_t j = 0; j < rooms.size(); j++) /* eight to a line */
        send_to_char(g, ch, "{}@c[@y{:d}@c]@n", (j > 0 && j % 8 == 0) ? "\r\n              " : " ", rooms[j]);
      if (rooms.empty())
        send_to_char(g, ch, "@cNone.@n");
      send_to_char(g, ch, "\r\n");
    }
    if (counter == 0)
      send_to_char(g, ch, "None found.\r\n");
  }

  /* Every zone in the world (sort of like 'show zones') */
  void list_zones(GameWorld *g, struct char_data *ch)
  {
    send_to_char(g, ch, "VNum  Zone Name                      Builder(s)\r\n----- ------------------------------ --------------------------------------\r\n");
    for (struct zone_data *zone : g->zones.all())
      send_to_char(g, ch, "[@g{:>3d}@n] @c{} @y{:<1}@n\r\n", zone->number, printf_width(zone->name.c_str(), count_color_chars(zone->name.c_str()) + 30, true),
                   zone->builders);
  }

  /* List code by Ronald Evers - dlanor@xs4all.nl */
  void list_triggers(GameWorld *g, struct char_data *ch, const listing &range)
  {
    send_to_char(g, ch, "Index VNum    Trigger Name                        Type\r\n----- ------- -------------------------------------------------------\r\n");
    int counter = 0;
    for (struct index_data *index : g->trig_index.all()) {
      if (!range.holds(index->vnum))
        continue;
      const trig_data *trig = index->proto;
      send_to_char(g, ch, "{:>4d}) [@g{:>5d}@n] @[1]{:<45.45} ", ++counter, index->vnum, trig->name);
      if (trig->attach_type == OBJ_TRIGGER)
        send_to_char(g, ch, "obj @y{}@n\r\n", sprintbit(trig->trigger_type, otrig_types));
      else if (trig->attach_type == WLD_TRIGGER)
        send_to_char(g, ch, "wld @y{}@n\r\n", sprintbit(trig->trigger_type, wtrig_types));
      else
        send_to_char(g, ch, "mob @y{}@n\r\n", sprintbit(trig->trigger_type, trig_types));
    }
    /* quirk: listing a zone, the range is -1 to -1 */
    if (counter == 0)
      send_to_char(g, ch, "No triggers found from {:d} to {:d}.\r\n", range.asked_min, range.asked_max);
  }
} // namespace

/* Prints all of the zone information for the selected zone */
void print_zone(GameWorld *g, struct char_data *ch, zone_vnum vnum)
{
  struct zone_data *zone = find_zone(g, vnum);
  if (!zone) {
    send_to_char(g, ch, "Zone #{:d} does not exist in the database.\r\n", vnum);
    return;
  }

  const room_vnum bottom = zone->bot, top = zone->top;
  auto in_zone           = [&](IDXTYPE v) { return v >= bottom && v <= top; };
  int rooms              = 0;
  g->rooms.visit([&](room_vnum, struct room_data *room) {
    if (room_get_zone(room) == zone)
      rooms++;
  });
  const auto mobiles  = std::ranges::count_if(g->mob_index.vnums(), in_zone);
  const auto objects  = std::ranges::count_if(g->obj_index.vnums(), in_zone);
  int triggers        = 0;
  for (struct index_data *trig : g->trig_index.all())
    triggers += in_zone(trig->vnum);

  send_to_char(g, ch,
               "@gVirtual Number = @c{:d}\r\n@gName of zone   = @c{}\r\n@gBuilders       = @c{}\r\n@gLifespan       = @c{:d}\r\n@gAge            = @c{:d}\r\n@gBottom of Zone = @c{:d}\r\n@gTop of Zone    = @c{:d}\r\n@gReset Mode     = @c{}\r\n@gMin Level      = @c{:d}\r\n@gMax Level      = @c{:d}\r\n@gZone Flags     = @c{}\r\n@gSize\r\n@g   Rooms       = @c{:d}\r\n@g   Objects     = @c{:d}\r\n@g   Mobiles     = @c{:d}\r\n@g   Shops       = @c{:d}\r\n@g   Triggers    = @c{:d}\r\n@g   Guilds      = @c{:d}@n\r\n",
               zone->number, zone->name, zone->builders, zone->lifespan, zone->age, zone->bot, zone->top,
               zone->reset_mode ? ((zone->reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset",
               zone->min_level, zone->max_level, sprintbitarray(zone->zone_flags, zone_bits, ZF_ARRAY_MAX), rooms, objects, mobiles,
               count_shops(g, bottom, top), triggers, count_guilds(g, bottom, top));
}
