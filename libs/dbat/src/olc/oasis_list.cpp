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
#include "dbat/oasis.hpp"
#include "dbat/shop.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/guild.hpp"
#include "dbat/races.hpp"
#include "dbat/class.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/
static void list_rooms(GameWorld *g, struct char_data *ch, struct zone_data *rnum, room_vnum vmin, room_vnum vmax);
static void list_mobiles(GameWorld *g, struct char_data *ch, struct zone_data *rnum, mob_vnum vmin, mob_vnum vmax);
static void list_objects(GameWorld *g, struct char_data *ch, struct zone_data *rnum, obj_vnum vmin, obj_vnum vmax);
static void list_shops(GameWorld *g, struct char_data *ch, struct zone_data *rnum, shop_vnum vmin, shop_vnum vmax);
static void list_triggers(GameWorld *g, struct char_data *ch, struct zone_data *rnum, trig_vnum vmin, trig_vnum vmax);
static void list_zones(GameWorld *g, struct char_data *ch);

/******************************************************************************/
/** Ingame Commands                                                          **/
/******************************************************************************/
ACMD(do_oasis_list)
{
  struct zone_data *rzone = nullptr;
  room_vnum vmin          = NOWHERE;
  room_vnum vmax          = NOWHERE;
  std::string smin;
  std::string smax;

  std::string_view rest = argument;
  smin                  = take_arg(rest);
  smax                  = take_arg(rest);

  if (subcmd == SCMD_OASIS_ZLIST) { /* special case */
    if (!smin.empty() && is_number(smin))
      print_zone(g, ch, parse_int(smin));
    else
      list_zones(g, ch);
    return;
  }

  if (smin.empty() || smin[0] == '.') {
    rzone = room_get_zone(g, char_get_in_room(ch));
  } else if (smax.empty()) {
    rzone = find_zone(g, parse_int(smin));

    if (rzone == nullptr) {
      send_to_char(g, ch, "Sorry, there's no zone with that number\r\n");
      return;
    }
  } else {
    /** Listing by min vnum / max vnum.  Retrieve the numeric values.          **/
    vmin = parse_int(smin);
    vmax = parse_int(smax);

    if (vmin + 500 < vmax) {
      send_to_char(g, ch, "Really? Over 500?! You need to view that many at once? Come on...\r\n");
      return;
    }
    if (vmin > vmax) {
      send_to_char(g, ch, "List from %d to %d - Aren't we funny today!\r\n", vmin, vmax);
      return;
    }
  }

  switch (subcmd) {
    case SCMD_OASIS_RLIST: list_rooms(g, ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_OLIST: list_objects(g, ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_MLIST: list_mobiles(g, ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_TLIST: list_triggers(g, ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_SLIST: list_shops(g, ch, rzone, vmin, vmax); break;
    case SCMD_OASIS_GLIST: list_guilds(g, ch, rzone, vmin, vmax); break;
    default:
      send_to_char(g, ch, "You can't list that!\r\n");
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true,
             "SYSERR: do_oasis_list: Unknown list option: %d", subcmd);
  }
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

  send_to_char(g, ch, "Zone %d is linked to the following zones:\r\n", zvnum);
  for (room_vnum nr : g->rooms.vnums_in(first, last)) {
    for (j = 0; j < NUM_OF_DIRS; j++) {
      if (room_get_dir_option(g, nr, j)) {
        to_room = room_get_dir_option(g, nr, j)->to_room;
        if (to_room != NOWHERE && (zrnum != room_get_zone(g, to_room)))
          send_to_char(g, ch, "%3d %-30s at %5d (%-5s) ---> %5d\r\n",
                       room_get_zone(g, to_room)->number,
                       room_get_zone(g, to_room)->name.c_str(),
                       nr, dirs[j], to_room);
      }
    }
  }
}

/******************************************************************************/
/** Helper Functions                                                         **/
/******************************************************************************/

/*
 * List all rooms in a zone.                              
 */
static void list_rooms(GameWorld *g, struct char_data *ch, struct zone_data *rnum, zone_vnum vmin, zone_vnum vmax)
{
  int j, counter = 0;
  IDXTYPE bottom, top;
  /*
   * Expect a minimum / maximum number if the rnum for the zone is NOWHERE. 
   */
  if (rnum != nullptr) {
    bottom = rnum->bot;
    top    = rnum->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char(g, ch,
               "@nIndex VNum    Room Name                                Exits\r\n"
               "----- ------- ---------------------------------------- -----@n\r\n");

  for (room_vnum i : g->rooms.vnums_in(bottom, top)) {

    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((i >= bottom) && (i <= top)) {
      counter++;

      send_to_char(g, ch, "%4d) [@g%-5d@n] @[1]%-*s@n %s",
                   counter, i, count_color_chars(room_get_name(g, i)) + 44,
                   room_get_name(g, i), room_get_proto_script(g, i) ? "[TRIG] " : "");

      for (j = 0; j < NUM_OF_DIRS; j++) {
        if (room_get_dir_option(g, i, j) == nullptr)
          continue;
        if (room_get_dir_option(g, i, j)->to_room == NOWHERE)
          continue;

        if (room_get_zone(g, room_get_dir_option(g, i, j)->to_room) != room_get_zone(g, i))
          send_to_char(g, ch, "(@y%d@n)", room_get_dir_option(g, i, j)->to_room);
      }

      send_to_char(g, ch, "\r\n");
    }
  }

  if (counter == 0) {
    send_to_char(g, ch, "No rooms found for zone/range specified.\r\n");
  }
}

/*
 * List all mobiles in a zone.                              
 */
static void list_mobiles(GameWorld *g, struct char_data *ch, struct zone_data *rnum, zone_vnum vmin, zone_vnum vmax)
{
  IDXTYPE bottom, top;
  int counter = 0;

  if (rnum != nullptr) {
    bottom = rnum->bot;
    top    = rnum->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char(g, ch,
               "@nIndex VNum    Mobile Name                    Race      Class     Level\r\n"
               "----- ------- -------------------------      --------- --------- -----\r\n");

  if (g->mob_proto.empty())
    return;

  for (mob_vnum i : g->mob_proto.vnums()) {
    if (mob_index_of(g, i)->vnum >= bottom && mob_index_of(g, i)->vnum <= top) {
      counter++;

      send_to_char(g, ch, "@g%4d@n) [@g%-5d@n] @[3]%-*s @C%-9s @c%-9s @y[%4d]@n %s\r\n",
                   counter, mob_index_of(g, i)->vnum, count_color_chars(char_get_sdesc(mob_proto_of(g, i))) + 30, char_get_sdesc(mob_proto_of(g, i)), char_true_race_name((mob_proto_of(g, i))), char_sensei_name((mob_proto_of(g, i))),
                   char_get_class_level(mob_proto_of(g, i)) + char_get_level_adj(mob_proto_of(g, i)) + char_get_race_level(mob_proto_of(g, i)), char_get_proto_script(mob_proto_of(g, i)) ? " [TRIG]" : "");
    }
  }

  if (counter == 0) {
    send_to_char(g, ch, "None found.\r\n");
  }
}

/*
 * List all objects in a zone.                              
 */
static void list_objects(GameWorld *g, struct char_data *ch, struct zone_data *rnum, room_vnum vmin, room_vnum vmax)
{
  IDXTYPE bottom, top;
  int counter = 0;

  if (rnum != nullptr) {
    bottom = rnum->bot;
    top    = rnum->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char(g, ch,
               "@nIndex VNum    Object Name                                  Object Type\r\n"
               "----- ------- -------------------------------------------- ----------------\r\n");

  if (g->obj_proto.empty())
    return;

  for (obj_vnum i : g->obj_proto.vnums()) {
    if (obj_index_of(g, i)->vnum >= bottom && obj_index_of(g, i)->vnum <= top) {
      counter++;

      send_to_char(g, ch, "@g%4d@n) [@g%-5d@n] @[2]%-*s @y[%s]@n%s\r\n",
                   counter, obj_index_of(g, i)->vnum, count_color_chars(obj_get_short_description(obj_proto_of(g, i))) + 44,
                   obj_get_short_description(obj_proto_of(g, i)), item_types[obj_get_type_flag(obj_proto_of(g, i))],
                   obj_get_proto_script(obj_proto_of(g, i)) ? " [TRIG]" : "");
    }
  }

  if (counter == 0) {
    send_to_char(g, ch, "None found.\r\n");
  }
}

/*
 * List all shops in a zone.                              
 */
static void list_shops(GameWorld *g, struct char_data *ch, struct zone_data *rnum, shop_vnum vmin, shop_vnum vmax)
{
  int j, counter = 0;
  IDXTYPE bottom, top;

  if (rnum != nullptr) {
    bottom = rnum->bot;
    top    = rnum->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  /****************************************************************************/
  /** Store the header for the shop listing.                                 **/
  /****************************************************************************/
  send_to_char(g, ch,
               "Index VNum    Shop Room(s)\r\n"
               "----- ------- ---------------------------------------------\r\n");

  for (struct shop_data *i : g->shops.all()) {
    if (shop_get_vnum(i) >= bottom && shop_get_vnum(i) <= top) {
      counter++;

      send_to_char(g, ch, "@g%4d@n) [@g%-5d@n]", counter, shop_get_vnum(i));

      /************************************************************************/
      /** Retrieve the list of rooms for this shop.                          **/
      /************************************************************************/

      for (j = 0; j < (int)shop_rooms(i).size(); j++)
        send_to_char(g, ch, "%s@c[@y%d@c]@n",
                     ((j > 0) && (j % 8 == 0)) ? "\r\n              " : " ", shop_rooms(i)[j]);

      if (j == 0)
        send_to_char(g, ch, "@cNone.@n");

      send_to_char(g, ch, "\r\n");
    }
  }

  if (counter == 0)
    send_to_char(g, ch, "None found.\r\n");
}

/*
 * List all zones in the world (sort of like 'show zones').                              
 */
static void list_zones(GameWorld *g, struct char_data *ch)
{

  send_to_char(g, ch,
               "VNum  Zone Name                      Builder(s)\r\n"
               "----- ------------------------------ --------------------------------------\r\n");

  if (g->zones.empty())
    return;

  for (struct zone_data *i : g->zones.all())
    send_to_char(g, ch, "[@g%3d@n] @c%-*s @y%-1s@n\r\n",
                 i->number, count_color_chars(i->name.c_str()) + 30, i->name.c_str(),
                 i->builders.c_str());
}

/*
 * Prints all of the zone information for the selected zone.
 */
void print_zone(GameWorld *g, struct char_data *ch, zone_vnum vnum)
{
  struct zone_data *rnum;
  int size_rooms, size_objects, size_mobiles;
  int size_guilds, size_triggers, size_shops;
  room_vnum top, bottom;
  char bits[MAX_STRING_LENGTH];

  if ((rnum = find_zone(g, vnum)) == nullptr) {
    send_to_char(g, ch, "Zone #%d does not exist in the database.\r\n", vnum);
    return;
  }

  sprintbitarray(rnum->zone_flags, zone_bits, ZF_ARRAY_MAX, bits, sizeof(bits));

  /****************************************************************************/
  /** Initialize some of the variables.                                      **/
  /****************************************************************************/
  size_rooms    = 0;
  size_objects  = 0;
  size_mobiles  = 0;
  top           = rnum->top;
  bottom        = rnum->bot;
  size_shops    = 0;
  size_triggers = 0;
  size_guilds   = 0;
  size_shops    = 0;

  g->rooms.visit([&](room_vnum, struct room_data *room) {
    if (room_get_zone(room) == rnum)
      size_rooms++;
  });
  size_shops  = count_shops(g, bottom, top);
  size_guilds = count_guilds(g, bottom, top);

  for (mob_vnum mvnum : g->mob_index.vnums())
    if (mvnum >= bottom && mvnum <= top)
      size_mobiles++;
  for (obj_vnum ovnum : g->obj_index.vnums())
    if (ovnum >= bottom && ovnum <= top)
      size_objects++;
  for (struct index_data *trig : g->trig_index.all())
    if (trig->vnum >= bottom && trig->vnum <= top)
      size_triggers++;

  /****************************************************************************/
  /** Display all of the zone information at once.                           **/
  /****************************************************************************/
  send_to_char(g, ch,
               "@gVirtual Number = @c%d\r\n"
               "@gName of zone   = @c%s\r\n"
               "@gBuilders       = @c%s\r\n"
               "@gLifespan       = @c%d\r\n"
               "@gAge            = @c%d\r\n"
               "@gBottom of Zone = @c%d\r\n"
               "@gTop of Zone    = @c%d\r\n"
               "@gReset Mode     = @c%s\r\n"
               "@gMin Level      = @c%d\r\n"
               "@gMax Level      = @c%d\r\n"
               "@gZone Flags     = @c%s\r\n"
               "@gSize\r\n"
               "@g   Rooms       = @c%d\r\n"
               "@g   Objects     = @c%d\r\n"
               "@g   Mobiles     = @c%d\r\n"
               "@g   Shops       = @c%d\r\n"
               "@g   Triggers    = @c%d\r\n"
               "@g   Guilds      = @c%d@n\r\n",
               rnum->number, rnum->name.c_str(),
               rnum->builders.c_str(), rnum->lifespan,
               rnum->age, rnum->bot, rnum->top,
               rnum->reset_mode ? ((rnum->reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset",
               rnum->min_level, rnum->max_level, bits,
               size_rooms, size_objects, size_mobiles, size_shops, size_triggers, size_guilds);
}

/* List code by Ronald Evers - dlanor@xs4all.nl */
static void list_triggers(GameWorld *g, struct char_data *ch, struct zone_data *rnum, trig_vnum vmin, trig_vnum vmax)
{
  IDXTYPE bottom, top;
  int counter = 0;
  char trgtypes[256];

  /** Expect a minimum / maximum number if no zone was given.               **/
  if (rnum != nullptr) {
    bottom = rnum->bot;
    top    = rnum->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  /** Store the header for the room listing. **/
  send_to_char(g, ch,
               "Index VNum    Trigger Name                        Type\r\n"
               "----- ------- -------------------------------------------------------\r\n");

  /** Loop through the world and find each room. **/
  for (struct index_data *i : g->trig_index.all()) {
    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((i->vnum >= bottom) && (i->vnum <= top)) {
      counter++;

      send_to_char(g, ch, "%4d) [@g%5d@n] @[1]%-45.45s ",
                   counter, i->vnum, i->proto->name.c_str());

      if (i->proto->attach_type == OBJ_TRIGGER) {
        sprintbit(i->proto->trigger_type, otrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(g, ch, "obj @y%s@n\r\n", trgtypes);
      } else if (i->proto->attach_type == WLD_TRIGGER) {
        sprintbit(i->proto->trigger_type, wtrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(g, ch, "wld @y%s@n\r\n", trgtypes);
      } else {
        sprintbit(i->proto->trigger_type, trig_types, trgtypes, sizeof(trgtypes));
        send_to_char(g, ch, "mob @y%s@n\r\n", trgtypes);
      }
    }
  }

  if (counter == 0) {
    send_to_char(g, ch, "No triggers found from %d to %d.\r\n", vmin, vmax);
  }
}
