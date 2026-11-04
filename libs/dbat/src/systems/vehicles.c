/*************************************************************************
*  File: vehicles.c                                    Part of CircleMUD *
*  Usage: Vechicle related code						 *
*									 *
*  All rights reserved.  See license.doc for complete information.	 *
*									 *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University*
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
*  Vehicle.c written by Chris Jacobson <fear@athenet.net>		 *
*************************************************************************/
#include "dbat/vehicles.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/act.informative.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

static room_vnum ship_land_location(GameWorld *g, struct char_data *ch, struct obj_data *vehicle, char *arg);
static void disp_ship_locations(GameWorld *g, struct char_data *ch, struct obj_data *vehicle);

/* This shows the player what locations the planet has to land at. */
static void disp_ship_locations(GameWorld *g, struct char_data *ch, struct obj_data *vehicle)
{
  if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 50) { // Above Earth
    send_to_char(g, ch, "@D------------------[ @GEarth@D ]------------------@c\n");
    send_to_char(g, ch, "Nexus City, South Ocean, Nexus field, Cherry Blossom Mountain,\n");
    send_to_char(g, ch, "Sandy Desert, Northern Plains, Korin's Tower, Kami's Lookout,\n");
    send_to_char(g, ch, "Shadow Forest, Decrepit Area, West City, Hercule Beach, Satan City.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 51) { // Above Frigid
    send_to_char(g, ch, "@D------------------[ @CFrigid@D ]------------------@c\n");
    send_to_char(g, ch, "Ice Crown City, Ice Highway, Topica Snowfield, Glug's Volcano,\n");
    send_to_char(g, ch, "Platonic Sea, Slave City, Acturian Woods, Desolate Demesne,\n");
    send_to_char(g, ch, "Chateau Ishran, Wyrm Spine Mountain, Cloud Ruler Temple, Koltoan mine.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 52) { // Above Konack
    send_to_char(g, ch, "@D------------------[ @MKonack@D ]------------------@c\n");
    send_to_char(g, ch, "Great Oroist Temple, Elzthuan Forest, Mazori Farm, Dres,\n");
    send_to_char(g, ch, "Colvian Farm, St Alucia, Meridius Memorial, Desert of Illusion,\n");
    send_to_char(g, ch, "Plains of Confusion, Turlon Fair, Wetlands, Kerberos,\n");
    send_to_char(g, ch, "Shaeras Mansion, Slavinus Ravine, Furian Citadel.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 53) { // Above Vegeta
    send_to_char(g, ch, "@D------------------[ @YVegeta@D ]------------------@c\n");
    send_to_char(g, ch, "Vegetos City, Blood Dunes, Ancestral Mountains, Destopa Swamp,\n");
    send_to_char(g, ch, "Pride Forest, Pride tower, Ruby Cave.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 198) { // Above Cerria
    send_to_char(g, ch, "@D------------------[ @MCerria@D ]------------------@c\n");
    send_to_char(g, ch, "Cerria Colony, Fistarl Volcano, Crystalline Forest.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 54) { // Above Namek
    send_to_char(g, ch, "@D------------------[ @gNamek@D ]------------------@c\n");
    send_to_char(g, ch, "Senzu Village, Guru's House, Crystalline Cave, Elder Village,\n");
    send_to_char(g, ch, "Frieza's Ship, Kakureta Village.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 55) { // Above Aether
    send_to_char(g, ch, "@D------------------[ @BAether@D ]-----------------@c\n");
    send_to_char(g, ch, "Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n");
    send_to_char(g, ch, "Silent Glade.\n");
    send_to_char(g, ch, "@D--------------------------------------------@n\n");
    send_to_char(g, ch, "@D------------------[ @BAether@D ]-----------------@c\n");
    send_to_char(g, ch, "Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n");
    send_to_char(g, ch, "Silent Glade.\n");
    send_to_char(g, ch, "@D--------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 56) { // Above Yardrat
    send_to_char(g, ch, "@D-----------------[ @mYardrat@D ]-----------------@c\n");
    send_to_char(g, ch, "Yardra City, Jade Forest, Jade Cliffs, Mount Valaria.\n");
    send_to_char(g, ch, "@D-------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 57) { // Above Zennith
    send_to_char(g, ch, "@D-----------------[ @CZennith@D ]-----------------@c\n");
    send_to_char(g, ch, "Utatlan City, Zenith Jungle, Ancient Castle.\n");
    send_to_char(g, ch, "@D-------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 58) { // Above Kanassa
    send_to_char(g, ch, "@D-----------------[ @CKanassa@D ]-----------------@c\n");
    send_to_char(g, ch, "Aquis City, Yunkai Pirate Base.\n");
    send_to_char(g, ch, "@D-------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 59) { // Above Arlia
    send_to_char(g, ch, "@D------------------[ @MArlia@D ]------------------@c\n");
    send_to_char(g, ch, "Janacre, Arlian Wasteland, Arlia Mine, Kemabra Wastes.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else {
    send_to_char(g, ch, "You are not above a planet!\r\n");
  }
}

static room_vnum ship_land_location(GameWorld *g, struct char_data *ch, struct obj_data *vehicle, char *arg)
{
  room_vnum landspot = 50;
  if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 50) { // Above Earth
    if (!strcasecmp(arg, "Nexus City")) {
      landspot = 300;
      landspot += rand_number(g, 0, 63);
      return (landspot);
    }
    if (!strcasecmp(arg, "South Ocean")) {
      landspot = 800;
      landspot += rand_number(g, 0, 99);
      return (landspot);
    }
    if (!strcasecmp(arg, "Nexus Field")) {
      landspot = 1150;
      landspot += rand_number(g, -16, 28);
      return (landspot);
    }
    if (!strcasecmp(arg, "Cherry Blossom Mountain")) {
      landspot = 1180;
      landspot += rand_number(g, 0, 19);
      return (landspot);
    }
    if (!strcasecmp(arg, "Sandy Desert")) {
      landspot = 1287;
      landspot += rand_number(g, 0, 64);
      return (landspot);
    }
    if (!strcasecmp(arg, "Northern Plains")) {
      landspot = 1428;
      landspot += rand_number(g, 0, 55);
      return (landspot);
    }
    if (!strcasecmp(arg, "Korin's Tower")) {
      return (1456);
    }
    if (!strcasecmp(arg, "Kami's Lookout")) {
      landspot = 1506;
      landspot += rand_number(g, 0, 30);
      return (landspot);
    }
    if (!strcasecmp(arg, "Shadow Forest")) {
      landspot = 1600;
      landspot += rand_number(g, 0, 66);
      return (landspot);
    }
    if (!strcasecmp(arg, "Decrepit Area")) {
      return (1710);
    }
    if (!strcasecmp(arg, "West City")) {
      return (19510);
    }
    if (!strcasecmp(arg, "Hercule Beach")) {
      landspot = 2141;
      landspot += rand_number(g, 0, 53);
      return (landspot);
    }
    if (!strcasecmp(arg, "Satan City")) {
      landspot = 1150;
      landspot += rand_number(g, -16, 28);
      return (landspot);
      return (13020);
    }
    send_to_char(g, ch, "You don't know where that made up place is, but decided to land anyway.");
    return (300);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 51) { // Above Frigid
    if (!strcasecmp(arg, "Ice Crown City")) {
      return (4264);
    }
    if (!strcasecmp(arg, "Ice Highway")) {
      return (4300);
    }
    if (!strcasecmp(arg, "Topica Snowfield")) {
      return (4351);
    }
    if (!strcasecmp(arg, "Glug's Volcano")) {
      return (4400);
    }
    if (!strcasecmp(arg, "Platonic Sea")) {
      return (4600);
    }
    if (!strcasecmp(arg, "Slave City")) {
      return (4800);
    }
    if (!strcasecmp(arg, "Acturian Woods")) {
      return (5100);
    }
    if (!strcasecmp(arg, "Desolate Demesne")) {
      return (5150);
    }
    if (!strcasecmp(arg, "Chateau Ishran")) {
      return (5165);
    }
    if (!strcasecmp(arg, "Wyrm Spine Mountain")) {
      return (5200);
    }
    if (!strcasecmp(arg, "Cloud Ruler Temple")) {
      return (5500);
    }
    if (!strcasecmp(arg, "Koltoan Mine")) {
      return (4944);
    }
    send_to_char(g, ch, "You don't know where that made up place is, but decided to land anyway.");
    return (4264);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 52) { // Above Konack
    if (!strcasecmp(arg, "Tiranoc City")) {
      return (8006);
    }
    if (!strcasecmp(arg, "Great Oroist Temple")) {
      return (8300);
    }
    if (!strcasecmp(arg, "Elzthuan Forest")) {
      return (8400);
    }
    if (!strcasecmp(arg, "Mazori Farm")) {
      return (8447);
    }
    if (!strcasecmp(arg, "Dres")) {
      return (8500);
    }
    if (!strcasecmp(arg, "Colvian Farm")) {
      return (8600);
    }
    if (!strcasecmp(arg, "St Alucia")) {
      return (8700);
    }
    if (!strcasecmp(arg, "Meridius Memorial")) {
      return (8800);
    }
    if (!strcasecmp(arg, "Desert of Illusion")) {
      return (8900);
    }
    if (!strcasecmp(arg, "Plains of Confusion")) {
      return (8954);
    }
    if (!strcasecmp(arg, "Turlon Fair")) {
      return (9200);
    }
    if (!strcasecmp(arg, "Wetlands")) {
      return (9700);
    }
    if (!strcasecmp(arg, "Kerberos")) {
      return (9855);
    }
    if (!strcasecmp(arg, "Shaeras Mansion")) {
      return (9864);
    }
    if (!strcasecmp(arg, "Slavinus Ravine")) {
      return (9900);
    }
    if (!strcasecmp(arg, "Furian Citadel")) {
      return (9949);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (8006);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 53) { // Above Vegeta
    if (!strcasecmp(arg, "Vegetos City")) {
      return (2226);
    }
    if (!strcasecmp(arg, "Blood Dunes")) {
      return (2600);
    }
    if (!strcasecmp(arg, "Ancestral Mountains")) {
      return (2616);
    }
    if (!strcasecmp(arg, "Destopa Swamp")) {
      return (2709);
    }
    if (!strcasecmp(arg, "Pride forest")) {
      return (2800);
    }
    if (!strcasecmp(arg, "Pride Tower")) {
      return (2899);
    }
    if (!strcasecmp(arg, "Ruby Cave")) {
      return (2615);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (2226);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 54) { // Above Namek
    if (!strcasecmp(arg, "Senzu Village")) {
      return (11600);
    }
    if (!strcasecmp(arg, "Guru's House")) {
      return (10182);
    }
    if (!strcasecmp(arg, "Crystalline Cave")) {
      return (10474);
    }
    if (!strcasecmp(arg, "Elder Village")) {
      return (13300);
    }
    if (!strcasecmp(arg, "Frieza's Ship")) {
      return (10203);
    }
    if (!strcasecmp(arg, "Kakureta Village")) {
      return (10922);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (11600);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 55) { // Above Aether
    if (!strcasecmp(arg, "Haven City")) {
      return (12010);
    }
    if (!strcasecmp(arg, "Serenity Lake")) {
      return (12103);
    }
    if (!strcasecmp(arg, "Kaiju Forest")) {
      return (12300);
    }
    if (!strcasecmp(arg, "Ortusian Temple")) {
      return (12400);
    }
    if (!strcasecmp(arg, "Silent Glade")) {
      return (12480);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (12010);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 56) { // Above Yardrat
    if (!strcasecmp(arg, "Yardra City")) {
      return (14008);
    }
    if (!strcasecmp(arg, "Jade Forest")) {
      return (14100);
    }
    if (!strcasecmp(arg, "Jade Cliffs")) {
      return (14200);
    }
    if (!strcasecmp(arg, "Mount Valaria")) {
      return (14300);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (14008);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 198) { // Above Cerria
    if (!strcasecmp(arg, "Cerria Colony")) {
      return (17531);
    }
    if (!strcasecmp(arg, "Crystalline Forest")) {
      return (7950);
    }
    if (!strcasecmp(arg, "Fistarl Volcano")) {
      return (17420);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (17531);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 57) { // Above Zennith
    if (!strcasecmp(arg, "Utatlan City")) {
      return (3412);
    }
    if (!strcasecmp(arg, "Zenith Jungle")) {
      return (3520);
    }
    if (!strcasecmp(arg, "Ancient Castle")) {
      return (19600);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (3412);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 58) { // Above Kanassa
    if (!strcasecmp(arg, "Aquis City")) {
      return (14904);
    }
    if (!strcasecmp(arg, "Yunkai Pirate Base")) {
      return (15655);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (14904);
  } else if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 59) { // Above Arlia
    if (!strcasecmp(arg, "Janacre")) {
      return (16009);
    }
    if (!strcasecmp(arg, "Arlian Wasteland")) {
      return (16544);
    }
    if (!strcasecmp(arg, "Arlia Mine")) {
      return (16600);
    }
    if (!strcasecmp(arg, "Kemabra Wastes")) {
      return (16816);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (16009);
  }
  send_to_char(g, ch, "You are not above a planet!\r\n");
  return (-1);
}

struct obj_data *find_vehicle_by_vnum(GameWorld *g, obj_vnum vnum)
{
  struct obj_data *i;

  for (i = g->objects; i; i = obj_get_next(i))
    if (obj_get_type_flag(i) == ITEM_VEHICLE && obj_get_in_room(i) != NOWHERE)
      if (obj_get_vnum(g, i) == vnum)
        return i;

  return 0;
}

/* Search the given list for an object type, and return a ptr to that obj*/
static struct obj_data *get_obj_in_list_type(int type, struct obj_data *list)
{
  struct obj_data *i;

  for (i = list; i; i = obj_get_next_content(i))
    if (obj_get_type_flag(i) == type)
      return i;

  return nullptr;
}

/* Search the player's room, inventory and equipment for a control */
struct obj_data *find_control(GameWorld *g, struct char_data *ch)
{
  struct obj_data *controls, *obj;
  int j;

  controls = get_obj_in_list_type(ITEM_CONTROL, rroom_get_contents(g, char_get_in_room(ch)));
  if (!controls)
    for (obj = char_get_inventory(ch); obj && !controls; obj = obj_get_next_content(obj))
      if (char_can_see_obj(g, ch, obj) && obj_get_type_flag(obj) == ITEM_CONTROL)
        controls = obj;
  if (!controls)
    for (j = 0; j < NUM_WEARS && !controls; j++)
      if (char_get_equipment(ch, j) && char_can_see_obj(g, ch, char_get_equipment(ch, j)) &&
          obj_get_type_flag(char_get_equipment(ch, j)) == ITEM_CONTROL)
        controls = char_get_equipment(ch, j);
  return controls;
}

/* Drive our vehicle into another vehicle */
static void drive_into_vehicle(GameWorld *g, struct char_data *ch, struct obj_data *vehicle, char *arg)
{
  struct obj_data *vehicle_in_out;
  room_rnum is_in, is_going_to;
  char buf[MAX_INPUT_LENGTH];

  if (!*arg) {
    send_to_char(g, ch, "@wDrive into what?\r\n");
  } else if (!(vehicle_in_out = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, obj_get_in_room(vehicle))))) {
    send_to_char(g, ch, "@wNothing here by that name!\r\n");
  } else if (obj_get_type_flag(vehicle_in_out) != ITEM_VEHICLE) {
    send_to_char(g, ch, "@wThat's not a ship.\r\n");
  } else if (vehicle == vehicle_in_out) {
    send_to_char(g, ch, "@wMy, we are in a clever mood today, aren't we.\r\n");
  } else {
    is_going_to = real_room(g, obj_get_value(vehicle_in_out, 0));
    if (!rroom_flagged(g, is_going_to, ROOM_VEHICLE)) {
      send_to_char(g, ch, "@wThat ship can't carry other ships.");
    } else {
      sprintf(buf, "%s @wenters %s.\n\r", obj_get_short_description(vehicle),
              obj_get_short_description(vehicle_in_out));
      send_to_room(g, obj_get_in_room(vehicle), "%s", buf);

      obj_from_room(g, vehicle);
      obj_to_room(g, vehicle, is_going_to);
      is_in = obj_get_in_room(vehicle);
      if (char_get_desc(ch) != nullptr)
        act(g, "", true, ch, 0, 0, TO_ROOM);
      send_to_char(g, ch, "@wThe ship flies onward:\r\n");
      look_at_room(g, obj_get_in_room(vehicle), ch, 0);
      sprintf(buf, "%s @wenters.\r\n", obj_get_short_description(vehicle));
      send_to_room(g, is_in, "%s", buf);
    }
  }
}

/* Drive our vehicle out of another vehicle */
static void drive_outof_vehicle(GameWorld *g, struct char_data *ch, struct obj_data *vehicle)
{
  struct obj_data *hatch, *vehicle_in_out;
  char buf[MAX_INPUT_LENGTH];

  if (!(hatch = get_obj_in_list_type(ITEM_HATCH, rroom_get_contents(g, obj_get_in_room(vehicle))))) {
    send_to_char(g, ch, "@wNowhere to pilot out of.\r\n");
  } else if (!(vehicle_in_out = find_vehicle_by_vnum(g, obj_get_value(hatch, 0)))) {
    send_to_char(g, ch, "@wYou can't pilot out anywhere!\r\n");
  } else {
    sprintf(buf, "%s @wexits %s.\r\n", obj_get_short_description(vehicle),
            obj_get_short_description(vehicle_in_out));
    send_to_room(g, obj_get_in_room(vehicle), "%s", buf);

    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, obj_get_in_room(vehicle_in_out));

    if (char_get_desc(ch) != nullptr)
      act(g, "@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.", true, ch, 0, 0, TO_ROOM);
    send_to_char(g, ch, "@wThe ship flies onward:\r\n");
    look_at_room(g, obj_get_in_room(vehicle), ch, 0);
    int door;
    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (char_can_go(g, ch, door)) {
        send_to_room(g, rroom_get_dir_option(g, char_get_in_room(ch), door)->to_room, "@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.\r\n");
      }
    }
    sprintf(buf, "%s @wflies out of %s.\r\n", obj_get_short_description(vehicle),
            obj_get_short_description(vehicle_in_out));
    send_to_room(g, obj_get_in_room(vehicle), "%s", buf);
  }
}

/* Drive out vehicle in a certain direction */
void drive_in_direction(GameWorld *g, struct char_data *ch, struct obj_data *vehicle, int dir)
{
  char buf[MAX_INPUT_LENGTH];

  if (!obj_get_exit(g, vehicle, dir) || obj_get_exit(g, vehicle, dir)->to_room == NOWHERE) {
    send_to_char(g, ch, "@wApparently %s doesn't exist there.\r\n", dirs[dir]);
  } else if (IS_SET(obj_get_exit(g, vehicle, dir)->exit_info, EX_CLOSED)) {
    /* But the door is closed */
    if (obj_get_exit(g, vehicle, dir)->keyword)
      send_to_char(g, ch, "@wThe %s seems to be closed.\r\n", fname(obj_get_exit(g, vehicle, dir)->keyword));
    else
      send_to_char(g, ch, "@wIt seems to be closed.\r\n");

  } else if (!rroom_flagged(g, obj_get_exit(g, vehicle, dir)->to_room, ROOM_VEHICLE) && !rroom_flagged(g, obj_get_exit(g, vehicle, dir)->to_room, ROOM_SPACE)) {
    /* But the vehicle can't go that way*/
    send_to_char(g, ch, "@wThe ship can't fit there!\r\n");
  } else {
    /* But nothing!  Let's go that way! */
    room_rnum was_in, is_in;

    sprintf(buf, "%s @wflies %s.\n\r", obj_get_short_description(vehicle), dirs[dir]);
    send_to_room(g, obj_get_in_room(vehicle), "%s", buf);

    was_in = obj_get_in_room(vehicle);
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, rroom_get_dir_option(g, was_in, dir)->to_room);
    struct obj_data *controls;
    if ((controls = find_control(g, ch))) {
      if (obj_get_value(controls, 3) < 5) {
        obj_set_value(controls, 3, obj_get_value(controls, 3) + (1));
      } else {
        obj_set_value(controls, 3, 0);
        obj_set_value(controls, 2, obj_get_value(controls, 2) - (1));
        if (obj_get_value(controls, 2) < 0) {
          obj_set_value(controls, 2, 0);
        }
      }
    }

    struct obj_data *hatch = nullptr;

    for (hatch = rroom_get_contents(g, real_room(g, obj_get_value(vehicle, 0))); hatch; hatch = obj_get_next_content(hatch)) {
      if (obj_get_type_flag(hatch) == ITEM_HATCH) {
        obj_set_value(hatch, 3, rroom_get_vnum(g, obj_get_in_room(vehicle)));
      }
    }

    is_in = obj_get_in_room(vehicle);

    if (char_get_desc(ch) != nullptr)
      act(g, "@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.", true, ch, 0, 0, TO_ROOM);
    send_to_char(g, ch, "@wThe ship flies onward:\r\n");
    look_at_room(g, is_in, ch, 0);
    if (controls) {
      send_to_char(g, ch, "@RFUEL@D: %s%s@n\r\n", obj_get_value(controls, 2) >= 200 ? "@G" : obj_get_value(controls, 2) >= 100 ? "@Y"
                                                                                                                            : "@r",
                   add_commas(obj_get_value(controls, 2)));
    }
    int door;
    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (char_can_go(g, ch, door)) {
        send_to_room(g, rroom_get_dir_option(g, char_get_in_room(ch), door)->to_room, "@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.\r\n");
      }
    }
    sprintf(buf, "%s @wflies in from the %s.\r\n",
            obj_get_short_description(vehicle), dirs[rev_dir[dir]]);

    send_to_room(g, is_in, "%s", buf);
  }
}

ACMD(do_warp)
{
  struct obj_data *vehicle, *controls;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_is_npc(ch))
    return;

  if (!char_has_arms(ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }
  if (!char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "@wYou need to be seated in the pilot's seat.\r\n[Enter: Pilot ready/unready]\r\n");
    return;
  }

  if (!(controls = find_control(g, ch))) {
    send_to_char(g, ch, "@wYou have nothing to control here!\r\n");
    return;
  }
  if (!(vehicle = find_vehicle_by_vnum(g, obj_get_value(controls, 0)))) {
    send_to_char(g, ch, "@wYou can't find anything to pilot.\r\n");
    return;
  }
  if (!*arg) {
    send_to_char(g, ch, "Syntax: shipwarp [ earth | vegeta | namek | konack | aether | frigid | buoy1 | buoy2 | buoy3 ]\r\n");
    return;
  }
  if (strcasecmp(arg, "earth") && strcasecmp(arg, "vegeta") && strcasecmp(arg, "namek") && strcasecmp(arg, "konack") && strcasecmp(arg, "frigid") && strcasecmp(arg, "aether") && strcasecmp(arg, "buoy1") && strcasecmp(arg, "buoy2") && strcasecmp(arg, "buoy3")) {
    send_to_char(g, ch, "Syntax: shipwarp [ earth | vegeta | namek | konack | aether | frigid | buoy1 | buoy2 | buoy3 ]\r\n");
    return;
  }
  if (rroom_flagged(g, !obj_get_in_room(vehicle), ROOM_SPACE)) {
    send_to_char(g, ch, "Your ship needs to be in space to utilize its Instant Travel Warp Accelerator.\r\n");
    return;
  }
  if (obj_get_vnum(g, vehicle) != 18400) {
    send_to_char(g, ch, "Your ship is not outfitted with an Instant Travel Warp Accelerator.\r\n");
    return;
  }
  if (!strcasecmp(arg, "earth")) {
    if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 40979 || rroom_get_vnum(g, obj_get_in_room(vehicle)) == 50) {
      send_to_char(g, ch, "Your ship is already there!\r\n");
      return;
    }
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n", true, ch, 0, 0, TO_ROOM);
    send_to_room(g, obj_get_in_room(vehicle), "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", obj_get_short_description(vehicle));
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, real_room(g, 40979));
    send_to_room(g, obj_get_in_room(vehicle), "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", obj_get_short_description(vehicle));
  } else if (!strcasecmp(arg, "namek")) {
    if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 42880 || rroom_get_vnum(g, obj_get_in_room(vehicle)) == 54) {
      send_to_char(g, ch, "Your ship is already there!\r\n");
      return;
    }
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n", true, ch, 0, 0, TO_ROOM);
    send_to_room(g, obj_get_in_room(vehicle), "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", obj_get_short_description(vehicle));
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, real_room(g, 42880));
    send_to_room(g, obj_get_in_room(vehicle), "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", obj_get_short_description(vehicle));
  } else if (!strcasecmp(arg, "frigid")) {
    if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 30889 || rroom_get_vnum(g, obj_get_in_room(vehicle)) == 51) {
      send_to_char(g, ch, "Your ship is already there!\r\n");
      return;
    }
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n", true, ch, 0, 0, TO_ROOM);
    send_to_room(g, obj_get_in_room(vehicle), "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", obj_get_short_description(vehicle));
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, real_room(g, 30889));
    send_to_room(g, obj_get_in_room(vehicle), "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", obj_get_short_description(vehicle));
  } else if (!strcasecmp(arg, "konack")) {
    if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 27065 || rroom_get_vnum(g, obj_get_in_room(vehicle)) == 52) {
      send_to_char(g, ch, "Your ship is already there!\r\n");
      return;
    }
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n", true, ch, 0, 0, TO_ROOM);
    send_to_room(g, obj_get_in_room(vehicle), "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", obj_get_short_description(vehicle));
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, real_room(g, 27065));
    send_to_room(g, obj_get_in_room(vehicle), "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", obj_get_short_description(vehicle));
  } else if (!strcasecmp(arg, "vegeta")) {
    if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 32365 || rroom_get_vnum(g, obj_get_in_room(vehicle)) == 53) {
      send_to_char(g, ch, "Your ship is already there!\r\n");
      return;
    }
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n", true, ch, 0, 0, TO_ROOM);
    send_to_room(g, obj_get_in_room(vehicle), "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", obj_get_short_description(vehicle));
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, real_room(g, 32365));
    send_to_room(g, obj_get_in_room(vehicle), "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", obj_get_short_description(vehicle));
  } else if (!strcasecmp(arg, "aether")) {
    if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == 41959 || rroom_get_vnum(g, obj_get_in_room(vehicle)) == 55) {
      send_to_char(g, ch, "Your ship is already there!\r\n");
      return;
    }
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n", true, ch, 0, 0, TO_ROOM);
    send_to_room(g, obj_get_in_room(vehicle), "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", obj_get_short_description(vehicle));
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, real_room(g, 41959));
    send_to_room(g, obj_get_in_room(vehicle), "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", obj_get_short_description(vehicle));
  } else if (!strcasecmp(arg, "buoy1")) {
    if (char_get_radar1(ch) <= 0) {
      send_to_char(g, ch, "You have not launched that buoy!\r\n");
      return;
    }
    if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == char_get_radar1(ch)) {
      send_to_char(g, ch, "Your ship is already there!\r\n");
      return;
    }
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n", true, ch, 0, 0, TO_ROOM);
    send_to_room(g, obj_get_in_room(vehicle), "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", obj_get_short_description(vehicle));
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, real_room(g, char_get_radar1(ch)));
    send_to_room(g, obj_get_in_room(vehicle), "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", obj_get_short_description(vehicle));
  } else if (!strcasecmp(arg, "buoy2")) {
    if (char_get_radar2(ch) <= 0) {
      send_to_char(g, ch, "You have not launched that buoy!\r\n");
      return;
    }
    if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == char_get_radar2(ch)) {
      send_to_char(g, ch, "Your ship is already there!\r\n");
      return;
    }
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n", true, ch, 0, 0, TO_ROOM);
    send_to_room(g, obj_get_in_room(vehicle), "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", obj_get_short_description(vehicle));
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, real_room(g, char_get_radar2(ch)));
    send_to_room(g, obj_get_in_room(vehicle), "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", obj_get_short_description(vehicle));
  } else if (!strcasecmp(arg, "buoy3")) {
    if (char_get_radar3(ch) <= 0) {
      send_to_char(g, ch, "You have not launched that buoy!\r\n");
      return;
    }
    if (rroom_get_vnum(g, obj_get_in_room(vehicle)) == char_get_radar3(ch)) {
      send_to_char(g, ch, "Your ship is already there!\r\n");
      return;
    }
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n", true, ch, 0, 0, TO_ROOM);
    send_to_room(g, obj_get_in_room(vehicle), "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n", obj_get_short_description(vehicle));
    obj_from_room(g, vehicle);
    obj_to_room(g, vehicle, real_room(g, char_get_radar3(ch)));
    send_to_room(g, obj_get_in_room(vehicle), "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n", obj_get_short_description(vehicle));
  } else {
    mud_log("ERROR: Ship Instant Warp Failure! Unknown argument!");
    send_to_char(g, ch, "ERROR\r\n");
    return;
  }
}

ACMD(do_drive)
{
  int dir, confirmed = false, count = 0;
  struct obj_data *vehicle, *controls;
  char arg3[MAX_INPUT_LENGTH];

  one_argument(argument, arg3);

  if (!char_has_arms(ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }

  if (!strcasecmp(arg3, "unready") && !char_is_npc(ch)) {
    if (!char_plr_flagged(ch, PLR_PILOTING)) {
      send_to_char(g, ch, "You are already not flying the ship!\r\n");
      return;
    }
    if (char_plr_flagged(ch, PLR_PILOTING)) {
      act(g, "@w$n stands up and stops piloting the ship.", true, ch, 0, 0, TO_ROOM);
      send_to_char(g, ch, "@wYou stand up from the pilot's seat.\r\n");
      char_set_position(ch, POS_STANDING);
      char_plr_set(ch, PLR_PILOTING, false);
      return;
    }
  }
  if (!strcasecmp(arg3, "ready") && !char_is_npc(ch)) {
    if (!(controls = find_control(g, ch))) {
      send_to_char(g, ch, "@wYou have nothing to control here!\r\n");
      return;
    }
    struct descriptor_data *d;
    if (char_plr_flagged(ch, PLR_PILOTING)) {
      send_to_char(g, ch, "@wYou are already piloting the ship, try [pilot unready].\r\n");
      return;
    }
    if (char_get_carrying(ch)) {
      send_to_char(g, ch, "@wYou are busy carrying someone.\r\n");
      return;
    }
    if (char_get_drag(ch)) {
      send_to_char(g, ch, "@wYou are busy dragging someone.\r\n");
      return;
    }
    for (d = g->descriptors; d; d = d->next) {
      if (!desc_is_playing(d)) {
        continue;
      }
      if (d->character == ch) {
        continue;
      }
      if (char_plr_flagged(d->character, PLR_PILOTING) && char_get_in_room(d->character) == char_get_in_room(ch)) {
        send_to_char(g, ch, "@w%s is already piloting the ship!\r\n", char_get_name(d->character));
        count = 1;
        return;
      }
    }
    if (count == 0) {
      confirmed = true;
    }
  }
  if (confirmed == true) {
    char_plr_set(ch, PLR_PILOTING, true);
    act(g, "@w$n sits down and begins piloting the ship.", true, ch, 0, 0, TO_ROOM);
    char_set_position(ch, POS_SITTING);
    send_to_char(g, ch, "@wYou take a seat in the pilot's chair.\r\n");
    return;
  }
  if (!char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "@wYou need to be seated in the pilot's seat.\r\n[Enter: Pilot ready/unready]\r\n");
  } else if (char_get_position(ch) < POS_SLEEPING) {
    send_to_char(g, ch, "@wYou can't see anything but stars!\r\n");
  } else if (char_aff_flagged(ch, AFF_BLIND)) {
    send_to_char(g, ch, "@wYou can't see a damned thing, you're blind!\r\n");
  } else if (room_is_dark(g, char_get_in_room(ch)) && !char_can_see_in_dark(g, ch)) {
    send_to_char(g, ch, "@wIt is pitch black...\r\n");
  } else if (!(controls = find_control(g, ch))) {
    send_to_char(g, ch, "@wYou have nothing to control here!\r\n");
  } else if (invalid_align(ch, controls) ||
             invalid_class(ch, controls) ||
             invalid_race(ch, controls)) {
    act(g, "@wYou are zapped by $p@w and instantly step away from it.", false, ch, controls, 0, TO_CHAR);
    act(g, "@w$n@w is zapped by $p@w and instantly steps away from it.", false, ch, controls, 0, TO_ROOM);
  } else if (!(vehicle = find_vehicle_by_vnum(g, obj_get_value(controls, 0)))) {
    send_to_char(g, ch, "@wYou can't find anything to pilot.\r\n");
  } else {

    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    /*argument = any_one_arg(argument, arg);*/
    half_chop(argument, arg, arg2);

    if (obj_get_value(controls, 2) <= 0) {
      send_to_char(g, ch, "Your ship doesn't have enough fuel to move.\r\n");
      return;
    }

    if (!*arg) {
      send_to_char(g, ch, "@wPilot, yes, but where?\r\n");
    } else if (is_abbrev(arg, "into") ||
               is_abbrev(arg, "onto")) {
      /* Driving into another vehicle */
      drive_into_vehicle(g, ch, vehicle, arg2);
    } else if (is_abbrev(arg, "out") && !obj_get_exit(g, vehicle, OUTDIR)) {
      drive_outof_vehicle(g, ch, vehicle);
    } else {
      if (!obj_val_flagged(vehicle, CONT_CLOSED)) {
        send_to_char(g, ch, "@wThe hatch is open, are you insane!?\r\n");
        return;
      }

      if (!strcasecmp(arg, "north") || !strcasecmp(arg, "n")) {
        dir = 0;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "east") || !strcasecmp(arg, "e")) {
        dir = 1;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "south") || !strcasecmp(arg, "s")) {
        dir = 2;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "west") || !strcasecmp(arg, "w")) {
        dir = 3;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "up") || !strcasecmp(arg, "u")) {
        dir = 4;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "down") || !strcasecmp(arg, "d")) {
        dir = 5;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "northwest") || !strcasecmp(arg, "nw") || !strcasecmp(arg, "northw")) {
        dir = 6;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "northeast") || !strcasecmp(arg, "ne") || !strcasecmp(arg, "northe")) {
        dir = 7;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "southeast") || !strcasecmp(arg, "se") || !strcasecmp(arg, "southe")) {
        dir = 8;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "southwest") || !strcasecmp(arg, "sw") || !strcasecmp(arg, "southw")) {
        dir = 9;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "inside")) {
        dir = 10;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      } else if (!strcasecmp(arg, "outside")) {
        dir = 11;
        drive_in_direction(g, ch, vehicle, dir);
        if (obj_get_value(controls, 1) == 1) {
          char_set_wait(ch, PULSE_2SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 2) {
          char_set_wait(ch, PULSE_2SEC);
        } else if (obj_get_value(controls, 1) == 3) {
          char_set_wait(ch, PULSE_1SEC * 1.5);
        } else if (obj_get_value(controls, 1) == 4) {
          char_set_wait(ch, PULSE_1SEC);
        } else if (obj_get_value(controls, 1) == 5) {
          char_set_wait(ch, PULSE_1SEC * 0.5);
        }
      }
      /*else if (dir = search_block(arg, dirs, FALSE) >= 0) {*/
      /* Drive in a direction... */
      /*}*/
      else if (!strcasecmp(arg, "land")) {

        if (!*arg2) {
          if (obj_get_vnum(g, vehicle) >= 46000 && obj_get_vnum(g, vehicle) <= 46099) {
            send_to_char(g, ch, "@wLand on which pad? 1, 2, 3 or 4?\r\n");
            send_to_char(g, ch, "@CSpecial Ship Ability@D: @wpilot land (area name)\n@GExample@D: @wpilot land Nexus City\r\n");
            disp_ship_locations(g, ch, vehicle);
          } else {
            send_to_char(g, ch, "@wLand on which pad? 1, 2, 3 or 4?\r\n");
          }
          return;
        }
        room_vnum land_location = 50;
        if (obj_get_vnum(g, vehicle) > 46099) {
          if (strcasecmp(arg2, "1") && strcasecmp(arg2, "2") && strcasecmp(arg2, "3") && strcasecmp(arg2, "4")) {
            send_to_char(g, ch, "@wLand on which pad? 1, 2, 3 or 4?\r\n");
            return;
          }
        } else if (strcasecmp(arg2, "1") && strcasecmp(arg2, "2") && strcasecmp(arg2, "3") && strcasecmp(arg2, "4")) {
          land_location = ship_land_location(g, ch, vehicle, arg2);
        }
        char buf3[MAX_INPUT_LENGTH];
        *buf3 = '\0';

        /* For Earth */
        act(g, "@wYou set the controls to descend.@n", false, ch, 0, 0, TO_CHAR);
        act(g, "@C$n @wmanipulates the ship controls.@n", false, ch, 0, 0, TO_ROOM);
        act(g, "@RThe ship rocks and shakes as it descends through the atmosphere!@n", false, ch, 0, 0, TO_CHAR);
        act(g, "@RThe ship rocks and shakes as it descends through the atmosphere!@n", false, ch, 0, 0, TO_ROOM);
        if (land_location <= 50) {
          act(g, "@wThe ship has landed.@n", false, ch, 0, 0, TO_CHAR);
          act(g, "@wThe ship has landed.@n", false, ch, 0, 0, TO_ROOM);
        }
        if (land_location > 50) {
          act(g, "@wThe ship slams into the ground and forms a small crater!@n", false, ch, 0, 0, TO_CHAR);
          act(g, "@wThe ship slams into the ground and forms a small crater!@n", false, ch, 0, 0, TO_ROOM);
          obj_from_room(g, vehicle);
          obj_to_room(g, vehicle, real_room(g, land_location));
        } else if (obj_get_in_room(vehicle) == real_room(g, 50)) {
          if (!strcasecmp(arg2, "1")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 409));
          } else if (!strcasecmp(arg2, "2")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 411));
          } else if (!strcasecmp(arg2, "3")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 412));
          } else if (!strcasecmp(arg2, "4")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 410));
          } else if (!strcasecmp(arg2, "4365")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 18904));
          } else if (!strcasecmp(arg2, "6329")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 18925));
          } else if (!strcasecmp(arg2, "1983")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 18995));
          } else {
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_CHAR);
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_ROOM);
          }
        }
        /* For Zenith */
        else if (obj_get_in_room(vehicle) == real_room(g, 57)) {
          obj_from_room(g, vehicle);
          obj_to_room(g, vehicle, real_room(g, 3508));
        }
        /* For Cerria*/
        else if (obj_get_in_room(vehicle) == real_room(g, 198)) {
          obj_from_room(g, vehicle);
          obj_to_room(g, vehicle, real_room(g, 17420));
        }
        /* For Kanassa */
        else if (obj_get_in_room(vehicle) == real_room(g, 58)) {
          obj_from_room(g, vehicle);
          obj_to_room(g, vehicle, real_room(g, 14904));
        }
        /* For Vegeta */
        else if (obj_get_in_room(vehicle) == real_room(g, 53)) {
          if (!strcasecmp(arg2, "1")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 2319));
          } else if (!strcasecmp(arg2, "2")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 2318));
          } else if (!strcasecmp(arg2, "3")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 2320));
          } else if (!strcasecmp(arg2, "4")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 2322));
          } else if (!strcasecmp(arg2, "4126")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 18212));
          } else {
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_CHAR);
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_ROOM);
          }
        }

        /* For Yardrat */
        else if (obj_get_in_room(vehicle) == real_room(g, 56)) {
          if (!strcasecmp(arg2, "1")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 14003));
          } else if (!strcasecmp(arg2, "2")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 14004));
          } else if (!strcasecmp(arg2, "3")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 14005));
          } else if (!strcasecmp(arg2, "4")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 14006));
          } else {
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_CHAR);
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_ROOM);
          }
        }

        /* For Aether */
        else if (obj_get_in_room(vehicle) == real_room(g, 55)) {
          if (!strcasecmp(arg2, "1")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 12003));
          } else if (!strcasecmp(arg2, "2")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 12004));
          } else if (!strcasecmp(arg2, "3")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 12006));
          } else if (!strcasecmp(arg2, "4")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 12005));
          } else {
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_CHAR);
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_ROOM);
          }
        }

        /* For Arlia */
        else if (obj_get_in_room(vehicle) == real_room(g, 59)) {
          if (!strcasecmp(arg2, "1")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 16065));
          } else if (!strcasecmp(arg2, "2")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 16066));
          } else if (!strcasecmp(arg2, "3")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 16067));
          } else if (!strcasecmp(arg2, "4")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 16068));
          } else {
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_CHAR);
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_ROOM);
          }
        }

        /* For Frigid */
        else if (obj_get_in_room(vehicle) == real_room(g, 51)) {
          if (!strcasecmp(arg2, "1")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 4264));
          } else if (!strcasecmp(arg2, "2")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 4263));
          } else if (!strcasecmp(arg2, "3")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 4261));
          } else if (!strcasecmp(arg2, "4")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 4262));
          } else if (!strcasecmp(arg2, "1337")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 18116));
          } else {
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_CHAR);
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_ROOM);
          }
        }
        /* For Namek */
        else if (obj_get_in_room(vehicle) == real_room(g, 54)) {
          if (!strcasecmp(arg2, "1")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 11628));
          } else if (!strcasecmp(arg2, "2")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 11629));
          } else if (!strcasecmp(arg2, "3")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 11630));
          } else if (!strcasecmp(arg2, "4")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 11627));
          } else {
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_CHAR);
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_ROOM);
          }
        }
        /* For Konack */
        else if (obj_get_in_room(vehicle) == real_room(g, 52)) {
          if (!strcasecmp(arg2, "1")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 8195));
          } else if (!strcasecmp(arg2, "2")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 8196));
          } else if (!strcasecmp(arg2, "3")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 8197));
          } else if (!strcasecmp(arg2, "4")) {
            obj_from_room(g, vehicle);
            obj_to_room(g, vehicle, real_room(g, 8198));
          } else {
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_CHAR);
            act(g, "@wLanding sequence aborted, improper coordinates.@n", false, ch, 0, 0, TO_ROOM);
          }
        } else {
          send_to_char(g, ch, "@wYou are not where you can land, you need to be in a planet's low orbit.@n\r\n");
        }
        if (land_location <= 50) {
          sprintf(buf3, "%s @wcomes in from above and slowly settles on the launch-pad.@n\r\n", obj_get_short_description(vehicle));
          look_at_room(g, obj_get_in_room(vehicle), ch, 0);
          send_to_room(g, obj_get_in_room(vehicle), "%s", buf3);
        } else {
          sprintf(buf3, "%s @wcomes in from above and slams into the ground!@n\r\n", obj_get_short_description(vehicle));
          rroom_set_dmg(g, obj_get_in_room(vehicle), rroom_get_dmg(g, obj_get_in_room(vehicle)) + (1));
          if (rroom_get_dmg(g, obj_get_in_room(vehicle)) >= 10) {
            rroom_set_dmg(g, obj_get_in_room(vehicle), 10);
          }
          look_at_room(g, obj_get_in_room(vehicle), ch, 0);
          send_to_room(g, obj_get_in_room(vehicle), "%s", buf3);
        }
      } else if (!strcasecmp(arg, "launch")) {
        int lnum       = 0;
        room_rnum rnum = 0;
        if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_EARTH)) {
          lnum = 1;
        } else if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_FRIGID)) {
          lnum = 2;
        } else if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_KONACK)) {
          lnum = 3;
        } else if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_VEGETA)) {
          lnum = 4;
        } else if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_NAMEK)) {
          lnum = 5;
        } else if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_AETHER)) {
          lnum = 6;
        } else if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_YARDRAT)) {
          lnum = 7;
        } else if (rroom_planet_zenith(g, obj_get_in_room(vehicle))) {
          lnum = 8;
        } else if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_CERRIA)) {
          lnum = 11;
        } else if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_KANASSA)) {
          lnum = 9;
        } else if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_ARLIA)) {
          lnum = 10;
        } else {
          send_to_char(g, ch, "@wYou are not on a planet.@n\r\n");
          return;
        }
        act(g, "@wYou set the controls to launch.@n", false, ch, 0, 0, TO_CHAR);
        act(g, "@C$n @wmanipulates the ship controls.@n", false, ch, 0, 0, TO_ROOM);
        act(g, "@RThe ship shudders as it launches up into the sky!@n", false, ch, 0, 0, TO_CHAR);
        act(g, "@RThe ship shudders as it launches up into the sky!@n", false, ch, 0, 0, TO_ROOM);
        act(g, "@wThe ship has reached low orbit.@n", false, ch, 0, 0, TO_CHAR);
        act(g, "@wThe ship has reached low orbit.@n", false, ch, 0, 0, TO_ROOM);
        send_to_room(g, obj_get_in_room(vehicle), "@R%s @Rshudders before blasting off into the sky!@n", obj_get_short_description(vehicle));
        if (lnum == 1) {
          rnum = real_room(g, 50);
        }
        if (lnum == 2) {
          rnum = real_room(g, 51);
        }
        if (lnum == 3) {
          rnum = real_room(g, 52);
        }
        if (lnum == 4) {
          rnum = real_room(g, 53);
        }
        if (lnum == 5) {
          rnum = real_room(g, 54);
        }
        if (lnum == 6) {
          rnum = real_room(g, 55);
        }
        if (lnum == 7) {
          rnum = real_room(g, 56);
        }
        if (lnum == 8) {
          rnum = real_room(g, 57);
        }
        if (lnum == 9) {
          rnum = real_room(g, 58);
        }
        if (lnum == 10) {
          rnum = real_room(g, 59);
        }
        if (lnum == 11) {
          rnum = real_room(g, 198);
        }
        if (obj_get_value(controls, 3) < 5) {
          obj_set_value(controls, 3, obj_get_value(controls, 3) + (1));
        } else {
          obj_set_value(controls, 3, 0);
          obj_set_value(controls, 2, obj_get_value(controls, 2) - (1));
          if (obj_get_value(controls, 2) < 0) {
            obj_set_value(controls, 2, 0);
          }
        }
        obj_from_room(g, vehicle);
        obj_to_room(g, vehicle, rnum);
        look_at_room(g, obj_get_in_room(vehicle), ch, 0);
        send_to_char(g, ch, "@RFUEL@D: %s%s@n\r\n", obj_get_value(controls, 2) >= 200 ? "@G" : obj_get_value(controls, 2) >= 100 ? "@Y"
                                                                                                                              : "@r",
                     add_commas(obj_get_value(controls, 2)));
      } else if (!strcasecmp(arg, "mark")) {
        if (!*arg2) {
          send_to_char(g, ch, "@wWhich marker are you wanting to launch? 1, 2, or 3?\r\n");
          return;
        }
        if (!!strcasecmp(arg2, "1") && !!strcasecmp(arg2, "2") && !!strcasecmp(arg2, "3")) {
          send_to_char(g, ch, "@wWhich marker are you wanting to launch? 1, 2, or 3?\r\n");
          return;
        }

        if (!rroom_flagged(g, obj_get_in_room(vehicle), ROOM_SPACE)) {
          send_to_char(g, ch, "@wYou need to be in space to launch a marker buoy.\r\n");
          return;
        }

        if (char_get_radar1(ch) > 0 && !strcasecmp(arg2, "1")) {
          send_to_char(g, ch, "@wYou need to 'deactivate' that marker.\r\n");
          return;
        }
        if (char_get_radar1(ch) <= 0 && !strcasecmp(arg2, "1")) {
          act(g, "@wYou enter a unique code and launch a marker buoy.@n\r\n", false, ch, 0, 0, TO_CHAR);
          act(g, "@C$n@w manipulates the ship controls.@n\r\n", false, ch, 0, 0, TO_ROOM);
          char_set_radar1(ch, rroom_get_vnum(g, obj_get_in_room(vehicle)));
        }
        if (char_get_radar2(ch) > 0 && !strcasecmp(arg2, "2")) {
          send_to_char(g, ch, "@wYou need to 'deactivate' that marker.\r\n");
          return;
        }
        if (char_get_radar2(ch) <= 0 && !strcasecmp(arg2, "2")) {
          act(g, "@wYou enter a unique code and launch a marker buoy.@n\r\n", false, ch, 0, 0, TO_CHAR);
          act(g, "@C$n@w manipulates the ship controls.@n\r\n", false, ch, 0, 0, TO_ROOM);
          char_set_radar2(ch, rroom_get_vnum(g, obj_get_in_room(vehicle)));
        }
        if (char_get_radar3(ch) > 0 && !strcasecmp(arg2, "3")) {
          send_to_char(g, ch, "@wYou need to 'deactivate' that marker.\r\n");
          return;
        }
        if (char_get_radar3(ch) <= 0 && !strcasecmp(arg2, "3")) {
          act(g, "@wYou enter a unique code and launch a marker buoy.@n\r\n", false, ch, 0, 0, TO_CHAR);
          act(g, "@C$n@w manipulates the ship controls.@n\r\n", false, ch, 0, 0, TO_ROOM);
          char_set_radar3(ch, rroom_get_vnum(g, obj_get_in_room(vehicle)));
        }
      } else if (!strcasecmp(arg, "deactivate")) {
        if (!*arg2) {
          send_to_char(g, ch, "@wWhich marker are you wanting to launch? 1, 2, or 3?\r\n");
          return;
        }
        if (!!strcasecmp(arg2, "1") && !!strcasecmp(arg2, "2") && !!strcasecmp(arg2, "3")) {
          send_to_char(g, ch, "@wWhich marker are you wanting to deactivate? 1, 2, or 3?\r\n");
          return;
        }

        if (char_get_radar1(ch) <= 0 && !strcasecmp(arg2, "1")) {
          send_to_char(g, ch, "@wYou haven't launched that buoy yet.\r\n");
          return;
        }
        if (char_get_radar1(ch) > 0 && !strcasecmp(arg2, "1")) {
          act(g, "@wYou enter buoy one's code and command it to deactivate.@n\r\n", false, ch, 0, 0, TO_CHAR);
          act(g, "@C$n@w manipulates the ship controls.@n\r\n", false, ch, 0, 0, TO_ROOM);
          char_set_radar1(ch, 0);
        }
        if (char_get_radar2(ch) <= 0 && !strcasecmp(arg2, "2")) {
          send_to_char(g, ch, "@wYou haven't launched that buoy yet.\r\n");
          return;
        }
        if (char_get_radar2(ch) > 0 && !strcasecmp(arg2, "2")) {
          act(g, "@wYou enter buoy two's code and command it to deactivate.@n\r\n", false, ch, 0, 0, TO_CHAR);
          act(g, "@C$n@w manipulates the ship controls.@n\r\n", false, ch, 0, 0, TO_ROOM);
          char_set_radar2(ch, 0);
        }
        if (char_get_radar3(ch) <= 0 && !strcasecmp(arg2, "3")) {
          send_to_char(g, ch, "@wYou haven't launched that buoy yet.\r\n");
          return;
        }
        if (char_get_radar3(ch) > 0 && !strcasecmp(arg2, "3")) {
          act(g, "@wYou enter buoy three's code and command it to deactivate.@n\r\n", false, ch, 0, 0, TO_CHAR);
          act(g, "@C$n@w manipulates the ship controls.@n\r\n", false, ch, 0, 0, TO_ROOM);
          char_set_radar3(ch, 0);
        }
      } else {
        send_to_char(g, ch, "@wThats not a valid direction.\r\n");
        send_to_char(g, ch, "Try one of these.\r\n");
        send_to_char(g, ch, "[ north/n  | south/s  | east/e  |  west/w  ]\r\n");
        send_to_char(g, ch, "[ up/u | down/d | northeast/ne/northe | northwest/nw/northw]\r\n");
        send_to_char(g, ch, "[  southeast/se/southe  |  southwest/sw/southw]\r\n");
        send_to_char(g, ch, "[  into  |  onto  |  inside  |  outside  ]@n\r\n");
        send_to_char(g, ch, "[ land | launch ]@n\r\n");
      }
    }
  }
}
