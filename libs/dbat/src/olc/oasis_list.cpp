/******************************************************************************/
/** OasisOLC - InGame OLC Listings                                     v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/

#include <cstdlib>

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
static void list_rooms(GameWorld *g, struct char_data *ch, zone_rnum rnum, room_vnum vmin, room_vnum vmax);
static void list_mobiles(GameWorld *g, struct char_data *ch, zone_rnum rnum, mob_vnum vmin, mob_vnum vmax);
static void list_objects(GameWorld *g, struct char_data *ch, zone_rnum rnum, obj_vnum vmin, obj_vnum vmax);
static void list_shops(GameWorld *g, struct char_data *ch, zone_rnum rnum, shop_vnum vmin, shop_vnum vmax);
static void list_triggers(GameWorld *g, struct char_data *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax);
static void list_zones(GameWorld *g, struct char_data *ch);

/******************************************************************************/
/** Ingame Commands                                                          **/
/******************************************************************************/
ACMD(do_oasis_list)
{
  zone_rnum rzone = NOWHERE;
  room_rnum vmin  = NOWHERE;
  room_rnum vmax  = NOWHERE;
  char smin[MAX_INPUT_LENGTH];
  char smax[MAX_INPUT_LENGTH];

  two_arguments(argument, smin, smax);

  if (subcmd == SCMD_OASIS_ZLIST) { /* special case */
    if (*smin && is_number(smin))
      print_zone(g, ch, atoi(smin));
    else
      list_zones(g, ch);
    return;
  }

  if (!*smin || *smin == '.') {
    rzone = rroom_get_zone(g, char_get_in_room(ch));
  } else if (!*smax) {
    rzone = real_zone(g, atoi(smin));

    if (rzone == NOWHERE) {
      send_to_char(g, ch, "Sorry, there's no zone with that number\r\n");
      return;
    }
  } else {
    /** Listing by min vnum / max vnum.  Retrieve the numeric values.          **/
    vmin = atoi(smin);
    vmax = atoi(smax);

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
  zone_rnum zrnum;
  zone_vnum zvnum;
  room_rnum nr, to_room;
  room_vnum first, last;
  int j;
  char arg[MAX_INPUT_LENGTH];

  skip_spaces(&argument);
  one_argument(argument, arg);

  if (!strcmp(arg, ".") || !*arg) {
    zrnum = rroom_get_zone(g, char_get_in_room(ch));
    zvnum = g->zones[zrnum].number;
  } else {
    zvnum = atoi(arg);
    zrnum = real_zone(g, zvnum);
  }

  if (zrnum == NOWHERE || zvnum == NOWHERE) {
    send_to_char(g, ch, "No zone was found with that number.\n\r");
    return;
  }

  last  = g->zones[zrnum].top;
  first = g->zones[zrnum].bot;

  send_to_char(g, ch, "Zone %d is linked to the following zones:\r\n", zvnum);
  for (nr = 0; nr <= g->top_room && (rroom_get_vnum(g, nr) <= last); nr++) {
    if (rroom_get_vnum(g, nr) >= first) {
      for (j = 0; j < NUM_OF_DIRS; j++) {
        if (rroom_get_dir_option(g, nr, j)) {
          to_room = rroom_get_dir_option(g, nr, j)->to_room;
          if (to_room != NOWHERE && (zrnum != rroom_get_zone(g, to_room)))
            send_to_char(g, ch, "%3d %-30s at %5d (%-5s) ---> %5d\r\n",
                         g->zones[rroom_get_zone(g, to_room)].number,
                         g->zones[rroom_get_zone(g, to_room)].name,
                         rroom_get_vnum(g, nr), dirs[j], rroom_get_vnum(g, to_room));
        }
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
static void list_rooms(GameWorld *g, struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax)
{
  room_rnum i;
  int j, counter = 0;
  IDXTYPE bottom, top;
  /*
   * Expect a minimum / maximum number if the rnum for the zone is NOWHERE. 
   */
  if (rnum != NOWHERE) {
    bottom = g->zones[rnum].bot;
    top    = g->zones[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char(g, ch,
               "@nIndex VNum    Room Name                                Exits\r\n"
               "----- ------- ---------------------------------------- -----@n\r\n");

  if (!g->top_room)
    return;

  for (i = 0; i <= g->top_room; i++) {

    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((rroom_get_vnum(g, i) >= bottom) && (rroom_get_vnum(g, i) <= top)) {
      counter++;

      send_to_char(g, ch, "%4d) [@g%-5d@n] @[1]%-*s@n %s",
                   counter, rroom_get_vnum(g, i), count_color_chars(rroom_get_name(g, i)) + 44,
                   rroom_get_name(g, i), rroom_get_proto_script(g, i) ? "[TRIG] " : "");

      for (j = 0; j < NUM_OF_DIRS; j++) {
        if (rroom_get_dir_option(g, i, j) == nullptr)
          continue;
        if (rroom_get_dir_option(g, i, j)->to_room == NOWHERE)
          continue;

        if (rroom_get_zone(g, rroom_get_dir_option(g, i, j)->to_room) != rroom_get_zone(g, i))
          send_to_char(g, ch, "(@y%d@n)", rroom_get_vnum(g, rroom_get_dir_option(g, i, j)->to_room));
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
static void list_mobiles(GameWorld *g, struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax)
{
  mob_rnum i;
  IDXTYPE bottom, top;
  int counter = 0;

  if (rnum != NOWHERE) {
    bottom = g->zones[rnum].bot;
    top    = g->zones[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char(g, ch,
               "@nIndex VNum    Mobile Name                    Race      Class     Level\r\n"
               "----- ------- -------------------------      --------- --------- -----\r\n");

  if (!g->top_mob)
    return;

  for (i = 0; i <= g->top_mob; i++) {
    if (g->mob_index[i].vnum >= bottom && g->mob_index[i].vnum <= top) {
      counter++;

      send_to_char(g, ch, "@g%4d@n) [@g%-5d@n] @[3]%-*s @C%-9s @c%-9s @y[%4d]@n %s\r\n",
                   counter, g->mob_index[i].vnum, count_color_chars(char_get_sdesc(&g->mob_proto[i])) + 30, char_get_sdesc(&g->mob_proto[i]), char_true_race_name((&g->mob_proto[i])), char_sensei_name((&g->mob_proto[i])),
                   char_get_class_level(&g->mob_proto[i]) + char_get_level_adj(&g->mob_proto[i]) + char_get_race_level(&g->mob_proto[i]), char_get_proto_script(&g->mob_proto[i]) ? " [TRIG]" : "");
    }
  }

  if (counter == 0) {
    send_to_char(g, ch, "None found.\r\n");
  }
}

/*
 * List all objects in a zone.                              
 */
static void list_objects(GameWorld *g, struct char_data *ch, zone_rnum rnum, room_vnum vmin, room_vnum vmax)
{
  obj_rnum i;
  IDXTYPE bottom, top;
  int counter = 0;

  if (rnum != NOWHERE) {
    bottom = g->zones[rnum].bot;
    top    = g->zones[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  send_to_char(g, ch,
               "@nIndex VNum    Object Name                                  Object Type\r\n"
               "----- ------- -------------------------------------------- ----------------\r\n");

  if (!g->top_obj)
    return;

  for (i = 0; i <= g->top_obj; i++) {
    if (g->obj_index[i].vnum >= bottom && g->obj_index[i].vnum <= top) {
      counter++;

      send_to_char(g, ch, "@g%4d@n) [@g%-5d@n] @[2]%-*s @y[%s]@n%s\r\n",
                   counter, g->obj_index[i].vnum, count_color_chars(obj_get_short_description(&g->obj_proto[i])) + 44,
                   obj_get_short_description(&g->obj_proto[i]), item_types[obj_get_type_flag(&g->obj_proto[i])],
                   obj_get_proto_script(&g->obj_proto[i]) ? " [TRIG]" : "");
    }
  }

  if (counter == 0) {
    send_to_char(g, ch, "None found.\r\n");
  }
}

/*
 * List all shops in a zone.                              
 */
static void list_shops(GameWorld *g, struct char_data *ch, zone_rnum rnum, shop_vnum vmin, shop_vnum vmax)
{
  shop_rnum i;
  int j, counter = 0;
  IDXTYPE bottom, top;

  if (rnum != NOWHERE) {
    bottom = g->zones[rnum].bot;
    top    = g->zones[rnum].top;
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

  for (i = 0; i <= g->top_shop; i++) {
    if (rshop_get_vnum(g, i) >= bottom && rshop_get_vnum(g, i) <= top) {
      counter++;

      send_to_char(g, ch, "@g%4d@n) [@g%-5d@n]", counter, rshop_get_vnum(g, i));

      /************************************************************************/
      /** Retrieve the list of rooms for this shop.                          **/
      /************************************************************************/

      for (j = 0; rshop_get_room(g, i, j) != NOWHERE; j++)
        send_to_char(g, ch, "%s@c[@y%d@c]@n",
                     ((j > 0) && (j % 8 == 0)) ? "\r\n              " : " ", rshop_get_room(g, i, j));

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
  zone_rnum i;

  send_to_char(g, ch,
               "VNum  Zone Name                      Builder(s)\r\n"
               "----- ------------------------------ --------------------------------------\r\n");

  if (!g->top_zone)
    return;

  for (i = 0; i <= g->top_zone; i++)
    send_to_char(g, ch, "[@g%3d@n] @c%-*s @y%-1s@n\r\n",
                 g->zones[i].number, count_color_chars(g->zones[i].name) + 30, g->zones[i].name,
                 g->zones[i].builders ? g->zones[i].builders : "None.");
}

/*
 * Prints all of the zone information for the selected zone.
 */
void print_zone(GameWorld *g, struct char_data *ch, zone_vnum vnum)
{
  zone_rnum rnum;
  int size_rooms, size_objects, size_mobiles;
  IDXTYPE i;
  int size_guilds, size_triggers, size_shops;
  room_vnum top, bottom;
  IDXTYPE largest_table;
  char bits[MAX_STRING_LENGTH];

  if ((rnum = real_zone(g, vnum)) == NOWHERE) {
    send_to_char(g, ch, "Zone #%d does not exist in the database.\r\n", vnum);
    return;
  }

  sprintbitarray(g->zones[rnum].zone_flags, zone_bits, ZF_ARRAY_MAX, bits, sizeof(bits));
  /****************************************************************************/
  /** Locate the largest of the three, g->top_room, g->top_mob, or           **/
  /** g->top_obj.                                                            **/
  /****************************************************************************/
  if (g->top_room >= g->top_obj && g->top_room >= g->top_mob)
    largest_table = g->top_room;
  else if (g->top_obj >= g->top_mob && g->top_obj >= g->top_room)
    largest_table = g->top_obj;
  else
    largest_table = g->top_mob;

  /****************************************************************************/
  /** Initialize some of the variables.                                      **/
  /****************************************************************************/
  size_rooms    = 0;
  size_objects  = 0;
  size_mobiles  = 0;
  top           = g->zones[rnum].top;
  bottom        = g->zones[rnum].bot;
  size_shops    = 0;
  size_triggers = 0;
  size_guilds   = 0;
  size_shops    = 0;

  for (i = 0; i <= largest_table; i++) {
    if (i <= g->top_room)
      if (rroom_get_zone(g, i) == rnum)
        size_rooms++;

    if (i <= g->top_obj)
      if (g->obj_index[i].vnum >= bottom && g->obj_index[i].vnum <= top)
        size_objects++;

    if (i <= g->top_mob)
      if (g->mob_index[i].vnum >= bottom && g->mob_index[i].vnum <= top)
        size_mobiles++;

    size_shops = count_shops(g, bottom, top);

    if (i < g->top_of_trigt)
      if (g->trig_index[i]->vnum >= bottom && g->trig_index[i]->vnum <= top)
        size_triggers++;

    size_guilds = count_guilds(g, bottom, top);
  }

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
               g->zones[rnum].number, g->zones[rnum].name,
               g->zones[rnum].builders, g->zones[rnum].lifespan,
               g->zones[rnum].age, g->zones[rnum].bot, g->zones[rnum].top,
               g->zones[rnum].reset_mode ? ((g->zones[rnum].reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset",
               g->zones[rnum].min_level, g->zones[rnum].max_level, bits,
               size_rooms, size_objects, size_mobiles, size_shops, size_triggers, size_guilds);
}

/* List code by Ronald Evers - dlanor@xs4all.nl */
static void list_triggers(GameWorld *g, struct char_data *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax)
{
  trig_rnum i;
  IDXTYPE bottom, top;
  int counter = 0;
  char trgtypes[256];

  /** Expect a minimum / maximum number if the rnum for the zone is NOWHERE. **/
  if (rnum != NOWHERE) {
    bottom = g->zones[rnum].bot;
    top    = g->zones[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  /** Store the header for the room listing. **/
  send_to_char(g, ch,
               "Index VNum    Trigger Name                        Type\r\n"
               "----- ------- -------------------------------------------------------\r\n");

  /** Loop through the world and find each room. **/
  for (i = 0; i < g->top_of_trigt; i++) {
    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((g->trig_index[i]->vnum >= bottom) && (g->trig_index[i]->vnum <= top)) {
      counter++;

      send_to_char(g, ch, "%4d) [@g%5d@n] @[1]%-45.45s ",
                   counter, g->trig_index[i]->vnum, g->trig_index[i]->proto->name);

      if (g->trig_index[i]->proto->attach_type == OBJ_TRIGGER) {
        sprintbit(g->trig_index[i]->proto->trigger_type, otrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(g, ch, "obj @y%s@n\r\n", trgtypes);
      } else if (g->trig_index[i]->proto->attach_type == WLD_TRIGGER) {
        sprintbit(g->trig_index[i]->proto->trigger_type, wtrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(g, ch, "wld @y%s@n\r\n", trgtypes);
      } else {
        sprintbit(g->trig_index[i]->proto->trigger_type, trig_types, trgtypes, sizeof(trgtypes));
        send_to_char(g, ch, "mob @y%s@n\r\n", trgtypes);
      }
    }
  }

  if (counter == 0) {
    send_to_char(g, ch, "No triggers found from %d to %d.\r\n", vmin, vmax);
  }
}
