/******************************************************************************/
/** OasisOLC - InGame OLC Listings                                     v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/

#include "dbat/game/utils.h"
#include "dbat/game/comm.h"
#include "dbat/game/interpreter.h"
#include "dbat/game/db.h"
#include "dbat/game/oasis.h"
#include "dbat/game/shop.h"
#include "dbat/game/dg_scripts.h"
#include "dbat/game/guild.h"
#include "dbat/game/races_plus.h"
#include "dbat/game/class.h"
#include "dbat/db/shops.h"

#include "dbat/db/iterate.hpp"


/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/
void list_rooms(struct char_data *ch  , struct zone_data *zone, room_vnum vmin, room_vnum vmax);
void list_mobiles(struct char_data *ch, struct zone_data *zone, mob_vnum vmin , mob_vnum vmax );
void list_objects(struct char_data *ch, struct zone_data *zone, obj_vnum vmin , obj_vnum vmax );
void list_shops(struct char_data *ch  , struct zone_data *zone, shop_vnum vmin, shop_vnum vmax);
void list_triggers(struct char_data *ch, struct zone_data *zone, trig_vnum vmin, trig_vnum vmax);
void list_zones(struct char_data *ch);
void print_zone(struct char_data *ch, struct zone_data *zone);
void list_guilds(struct char_data *ch  , struct zone_data *zone, guild_vnum vmin, guild_vnum vmax);


/******************************************************************************/
/** Ingame Commands                                                          **/
/******************************************************************************/
ACMD(do_oasis_list) 
{
  zone_rnum rzone = NOWHERE;
  room_rnum vmin = NOWHERE;
  room_rnum vmax = NOWHERE;
  char smin[MAX_INPUT_LENGTH];
  char smax[MAX_INPUT_LENGTH];
  struct zone_data *zone = NULL;
  
  two_arguments(argument, smin, smax);
  
  if (subcmd == SCMD_OASIS_ZLIST) { /* special case */
    if (smin != NULL && *smin && is_number(smin)) 
      print_zone(ch, zone_by_id(atoi(smin)));
    else
      list_zones(ch);  
      return;
    }
    
  if (!*smin || *smin == '.') {
    zone = char_zone_get(ch);
  } else if (!*smax) {
    zone = zone_by_id(atoi(smin));
    
    if (zone == NULL) {
      send_to_char(ch, "Sorry, there's no zone with that number\r\n");
      return;
    }
  } else {
    /** Listing by min vnum / max vnum.  Retrieve the numeric values.          **/
    vmin = atoi(smin);
    vmax = atoi(smax);
    
    if (vmin + 500 < vmax) {
      send_to_char(ch, "Really? Over 500?! You need to view that many at once? Come on...\r\n");
      return;
    }
    if (vmin > vmax) {
      send_to_char(ch, "List from %d to %d - Aren't we funny today!\r\n", vmin, vmax);
      return;
    }
    }
    
  switch (subcmd) {
    case SCMD_OASIS_RLIST: list_rooms(ch, zone, vmin, vmax); break;
    case SCMD_OASIS_OLIST: list_objects(ch, zone, vmin, vmax); break;
    case SCMD_OASIS_MLIST: list_mobiles(ch, zone, vmin, vmax); break;
    case SCMD_OASIS_TLIST: list_triggers(ch, zone, vmin, vmax); break;
    case SCMD_OASIS_SLIST: list_shops(ch, zone, vmin, vmax); break;
    case SCMD_OASIS_GLIST: list_guilds(ch, zone, vmin, vmax); break;
    default: 
      send_to_char(ch, "You can't list that!\r\n");
      mudlog(BRF, ADMLVL_IMMORT, TRUE, 
        "SYSERR: do_oasis_list: Unknown list option: %d", subcmd);
  }
}

ACMD(do_oasis_links)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  room_rnum nr, to_room;
  int first, last, j;
  char arg[MAX_INPUT_LENGTH];

  skip_spaces(&argument);
  one_argument(argument, arg);

  if (!strcmp(arg, ".") || (arg == NULL || !*arg)) {
    auto zone = char_zone_get(ch);
    zvnum = zone->number;
  } else {
    zvnum = atoi(arg);
  }

  auto zone = zone_by_id(zvnum);

  if (zone == NULL) {
    send_to_char(ch, "No zone was found with that number.\n\r");
    return;
  }

  last  = zone->top;
  first = zone->bot;

  send_to_char(ch, "Zone %d is linked to the following zones:\r\n", zvnum);
  room_iterate([&](auto room) {
    if (room_vnum_get(room) >= first) {
      for (j = 0; j < NUM_OF_DIRS; j++) {
      if (room->dir_option[j]) {
        struct room_data *trm = exit_dest_get(room->dir_option[j]);
        if(!trm) continue;
        auto tz = room_zone_get(trm);
        send_to_char(ch, "%3d %-30s at %5d (%-5s) ---> %5d\r\n",
          tz->number,
          tz->name,
          room_vnum_get(room), dirs[j], trm->number);
      }
      }
    }
    return true;
  });
}

/******************************************************************************/
/** Helper Functions                                                         **/
/******************************************************************************/


/*
 * List all rooms in a zone.                              
 */                                                                           
void list_rooms(struct char_data *ch, struct zone_data *zone, room_vnum vmin, room_vnum vmax)
{
  int i, j, bottom, top, counter = 0;
  /*
   * Expect a minimum / maximum number if the rnum for the zone is NOWHERE. 
   */
  if (zone != NULL) {
    bottom = zone->bot;
    top    = zone->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  send_to_char(ch,
  "@nIndex VNum    Room Name                                Exits\r\n"
  "----- ------- ---------------------------------------- -----@n\r\n");
  
  for (i = bottom; i <= top; i++) {
    struct room_data *rm = room_by_id(i);
    if(!rm) continue;
    
    counter++;

      send_to_char(ch, "%4d) [@g%-5d@n] @[1]%-*s@n %s",
                        counter, rm->number, count_color_chars(rm->name)+44, 
                        rm->name, rm->proto_script ? "[TRIG] " : "");

      for (j = 0; j < NUM_OF_DIRS; j++) {
        struct room_direction_data *exit = rm->dir_option[j];
        if (exit == NULL)
          continue;
        struct room_data *dest = exit_dest_get(exit);
        if (!dest)
          continue;
          
        if (dest->zone != rm->zone) 
          send_to_char(ch, "(@y%d@n)", dest->number);
     
      }
    
      send_to_char(ch, "\r\n");
  }
  
  if (counter == 0) {
    send_to_char(ch, "No rooms found for zone/range specified.\r\n"); 
  }
}

/*
 * List all mobiles in a zone.                              
 */                                                                           
void list_mobiles(struct char_data *ch, struct zone_data *zone, mob_vnum vmin, mob_vnum vmax)
{
  int i, bottom, top, counter = 0, admg;
  
  if (zone != NULL) {
    bottom = zone->bot;
    top    = zone->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  send_to_char(ch,
  "@nIndex VNum    Mobile Name                    Race      Class     Level\r\n"
  "----- ------- -------------------------      --------- --------- -----\r\n");
  
  for (i = bottom; i <= top; i++) {
    struct char_data *mob = mob_proto_by_id(i);
    if(!mob) continue;
    counter++;

    send_to_char(ch, "@g%4d@n) [@g%-5d@n] @[3]%-*s @C%-9s @c%-9s @y[%4d]@n %s\r\n",
                  counter, char_proto_id_get(mob), count_color_chars(mob->short_descr)+30, mob->short_descr, TRUE_RACE((mob)), SENSEI_NAME((mob)),
                  mob->level + mob->level_adj + mob->race_level, mob->proto_script ? " [TRIG]" : "");
  }
  
  if (counter == 0) {
    send_to_char(ch, "None found.\r\n");
  }
}

/*
 * List all objects in a zone.                              
 */                                                                           
void list_objects(struct char_data *ch, struct zone_data *zone, obj_vnum vmin, obj_vnum vmax)
{
  int i, bottom, top, counter = 0;
  
  if (zone != NULL) {
    bottom = zone->bot;
    top    = zone->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  send_to_char(ch,
  "@nIndex VNum    Object Name                                  Object Type\r\n"
  "----- ------- -------------------------------------------- ----------------\r\n");
  
  for (i = bottom; i <= top; i++) {
    struct obj_proto_data *obj = obj_proto_by_id(i);
    if(!obj) continue;
    counter++;

    send_to_char(ch, "@g%4d@n) [@g%-5d@n] @[2]%-*s @y[%s]@n%s\r\n",
                  counter, obj->vnum, count_color_chars(obj->short_description)+44,
                  obj->short_description, item_types[obj->type_flag],
                  obj->proto_script ? " [TRIG]" : "");
  }
  
  if (counter == 0) {
    send_to_char(ch, "None found.\r\n");
  }
}


/*
 * List all shops in a zone.                              
 */                                                                           
void list_shops(struct char_data *ch, struct zone_data *zone, shop_vnum vmin, shop_vnum vmax)
{
  int i, j, bottom, top, counter = 0;
  
  if (zone != NULL) {
    bottom = zone->bot;
    top    = zone->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  /****************************************************************************/
  /** Store the header for the shop listing.                                 **/
  /****************************************************************************/
  send_to_char (ch,
  "Index VNum    Shop Room(s)\r\n"
  "----- ------- ---------------------------------------------\r\n");
  
  for (i = bottom; i <= top; i++) {
    if (auto shop = shop_by_id(i); shop) {
      counter++;
      
      send_to_char(ch, "@g%4d@n) [@g%-5d@n]", counter, shop->vnum);

      /************************************************************************/
      /** Retrieve the list of rooms for this shop.                          **/
      /************************************************************************/
      
      for (j = 0; shop->in_room[j] != NOWHERE; j++)
        send_to_char(ch, "%s@c[@y%d@c]@n",
          ((j > 0) && (j % 8 == 0)) ? "\r\n              " : " ", shop->in_room[j]);

      if (j == 0)
        send_to_char(ch, "@cNone.@n");
      
      send_to_char(ch, "\r\n");
    }
  }
  
  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

/*
 * List all zones in the world (sort of like 'show zones').                              
 */                                                                           
void list_zones(struct char_data *ch)
{
  int i;
  
  send_to_char(ch,
  "VNum  Zone Name                      Builder(s)\r\n"
  "----- ------------------------------ --------------------------------------\r\n");
  
  zone_iterate([&](auto zone) {
    send_to_char(ch, "[@g%3d@n] @c%-*s @y%-1s@n\r\n",
              zone->number, count_color_chars(zone->name)+30, zone->name,
              zone->builders ? zone->builders : "None.");
              return true;
  });
}



/*
 * Prints all of the zone information for the selected zone.
 */
void print_zone(struct char_data *ch, struct zone_data *zone)
{
  zone_rnum rnum;
  int size_rooms, size_objects, size_mobiles, i;
  int size_guilds, size_triggers, size_shops;
  room_vnum top, bottom;
  int largest_table;
  char bits[MAX_STRING_LENGTH];
  
  if (!zone) {
    send_to_char(ch, "Zone not found.\r\n");
    return;
  }

  sprintbitarray(zone->zone_flags, zone_bits, ZF_ARRAY_MAX, bits, sizeof(bits));  
  
  /****************************************************************************/
  /** Initialize some of the variables.                                      **/
  /****************************************************************************/
  top          = zone->top;
  bottom       = zone->bot;
  size_objects = 0;
  size_mobiles = 0;
  size_rooms   = 0;
  size_shops   = 0;
  size_triggers= 0;
  size_guilds  = 0;
  size_shops  = 0;
  
  for (i = bottom; i <= top; i++)
    if (room_by_id(i))
      size_rooms++;
   
  for (i = bottom; i <= top; i++) {
    if (obj_proto_by_id(i))
      size_objects++;
  }
   
  for(i = bottom; i <= top; i++)
      if (mob_proto_by_id(i))
        size_mobiles++;

  size_shops = count_shops(bottom, top);

  for (i = bottom; i <= top; i++)
    if (trig_proto_by_id(i))
      size_triggers++;

  size_guilds = count_guilds(bottom, top);
  
  /****************************************************************************/
  /** Display all of the zone information at once.                           **/
  /****************************************************************************/
  send_to_char(ch,
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
    zone->number, zone->name,
    zone->builders, zone->lifespan,
    zone->age, zone->bot, zone->top,
    zone->reset_mode ? ((zone->reset_mode == 1) ?
    "Reset when no players are in zone." : "Normal reset.") : "Never reset",
    zone->min_level, zone->max_level, bits,
    size_rooms, size_objects, size_mobiles, size_shops, size_triggers, size_guilds);
}

/* List code by Ronald Evers - dlanor@xs4all.nl */
void list_triggers(struct char_data *ch, struct zone_data *zone, trig_vnum vmin, trig_vnum vmax)
{
  int i, bottom, top, counter = 0;
  char trgtypes[256];

  /** Expect a minimum / maximum number if the rnum for the zone is NOWHERE. **/
  if (zone != NULL) {
    bottom = zone->bot;
    top    = zone->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }


  /** Store the header for the room listing. **/
  send_to_char(ch,
  "Index VNum    Trigger Name                        Type\r\n"
  "----- ------- -------------------------------------------------------\r\n");


  /** Loop through the world and find each room. **/
  for (i = bottom; i <= top; i++) {
    auto trig = trig_proto_by_id(i);
    if(!trig) continue;
    counter++;

      send_to_char(ch, "%4d) [@g%5d@n] @[1]%-45.45s ",
        counter, trig->vnum, trig->name);

      if (trig->attach_type == OBJ_TRIGGER) {
        sprintbit(GET_TRIG_TYPE(trig), otrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "obj @y%s@n\r\n", trgtypes);
      } else if (trig->attach_type==WLD_TRIGGER) {
        sprintbit(GET_TRIG_TYPE(trig), wtrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "wld @y%s@n\r\n", trgtypes);
      } else {
        sprintbit(GET_TRIG_TYPE(trig), trig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "mob @y%s@n\r\n", trgtypes);
      }
  }

  if (counter == 0) {
    send_to_char(ch, "No triggers found from %d to %d.\r\n", vmin, vmax);
  }
}
