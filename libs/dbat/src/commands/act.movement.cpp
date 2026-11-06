/*************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/act.movement.h"
#include "dbat/consts/maximums.h"
#include "dbat/utils.h"

#include "dbat/character_utils.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/zone_utils.h"
#include "dbat/stringutils.h"
#include "dbat/search.h"
#include "dbat/config.h"
#include "dbat/random.h"
#include "dbat/extract.h"

#include "dbat/act.informative.h"

#include "dbat/broadcast.h"
#include "dbat/vehicles.h"
#include "dbat/oasis_copy.h"
#include "dbat/relocate.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/fight.h"
#include "dbat/spells.h"
#include "dbat/oasis.h"

#include "dbat/guild.h"
#include "dbat/dg_scripts.h"
#include "dbat/local_limits.h"
#include "dbat/house.h"
#include "dbat/class.h"
#include "dbat/act.offensive.h"
#include "dbat/game_world.h"

/* local functions */
static void handle_fall(GameWorld *g, struct char_data *ch);
static int check_swim(GameWorld *g, struct char_data *ch);
static void disp_locations(GameWorld *g, struct char_data *ch);
static int find_door(GameWorld *g, struct char_data *ch, const char *type, char *dir, const char *cmdname);
static bool has_key(GameWorld *g, struct char_data *ch, obj_vnum key);
static void do_doorcmd(GameWorld *g, struct char_data *ch, struct obj_data *obj, int door, int scmd);
static bool ok_pick(GameWorld *g, struct char_data *ch, obj_vnum keynum, int pickproof, int dclock, int scmd, struct obj_data *obj);
static bool has_flight(GameWorld *g, struct char_data *ch);
static bool do_simple_enter(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check);
static int perform_enter_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check);
static bool do_simple_leave(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check);
static int perform_leave_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check);

/* This handles teleporting players with instant transmission or skills like it. */
void handle_teleport(GameWorld *g, struct char_data *ch, struct char_data *tar, room_vnum location)
{
  int success = false;

  if (location != 0) { /* Teleport to a particular room */
    char_from_room(g, ch);
    char_to_room(g, ch, real_room(g, location));
    success = true;
  } else if (tar != nullptr) { /* Teleport to a particular character */
    char_from_room(g, ch);
    char_to_room(g, ch, char_get_in_room(tar));
    success = true;
  }

  if (success == true) { /* We have made it. */
    act(g, "@w$n@w appears in an instant out of nowhere!@n", true, ch, 0, 0, TO_ROOM);
    if (char_get_drag(ch) && !char_is_npc(char_get_drag(ch))) {
      char_from_room(g, char_get_drag(ch));
      char_to_room(g, char_get_drag(ch), char_get_in_room(ch));
      act(g, "@w$n@w appears in an instant out of nowhere being dragged by $N!@n", true, char_get_drag(ch), 0, ch, TO_NOTVICT);
    }
    if (char_get_grappling(ch) && !char_is_npc(char_get_grappling(ch))) {
      char_from_room(g, char_get_grappling(ch));
      char_to_room(g, char_get_grappling(ch), char_get_in_room(ch));
      act(g, "@w$n@w appears in an instant out of nowhere being grappled by $N!@n", true, char_get_grappling(ch), 0, ch, TO_NOTVICT);
    }
    if (char_get_carrying(ch)) {
      char_from_room(g, char_get_carrying(ch));
      char_to_room(g, char_get_carrying(ch), char_get_in_room(ch));
      act(g, "@w$n@w appears in an instant out of nowhere being carried by $N!@n", true, char_get_carrying(ch), 0, ch, TO_NOTVICT);
    }
    if (char_get_grappled(ch) && !char_is_npc(char_get_grappled(ch))) {
      char_from_room(g, char_get_grappled(ch));
      char_to_room(g, char_get_grappled(ch), char_get_in_room(ch));
      act(g, "@w$n@w appears in an instant out of nowhere being grappled by $N!@n", true, char_get_grappled(ch), 0, ch, TO_NOTVICT);
    }
    if (char_get_drag(ch) && char_is_npc(char_get_drag(ch))) {
      act(g, "@WYou stop dragging @C$N@W!@n", true, ch, 0, char_get_drag(ch), TO_CHAR);
      act(g, "@C$n@W stops dragging @c$N@W!@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
      char_set_dragged(char_get_drag(ch), nullptr);
      char_set_drag(ch, nullptr);
    }
    if (char_get_grappling(ch) && char_is_npc(char_get_grappling(ch))) {
      char_set_grap(char_get_grappling(ch), -1);
      char_set_grappled(char_get_grappling(ch), nullptr);
      char_set_grappling(ch, nullptr);
      char_set_grap(ch, -1);
    }
    if (char_get_grappled(ch) && char_is_npc(char_get_grappled(ch))) {
      char_set_grap(char_get_grappled(ch), -1);
      char_set_grappling(char_get_grappled(ch), nullptr);
      char_set_grappled(ch, nullptr);
      char_set_grap(ch, -1);
    }
  } else { /* Wut... */
    mud_log("ERROR: handle_teleport called without a destination.");
    return;
  }
}

/* Let's carry someone! Why not? - Iovan */
ACMD(do_carry)
{

  if (char_is_npc(ch))
    return;

  struct char_data *vict = nullptr;
  char arg[MAX_INPUT_LENGTH];

  if (char_get_drag(ch)) {
    send_to_char(g, ch, "You are busy dragging someone at the moment.\r\n");
    return;
  }

  if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }

  if (char_get_carrying(ch)) { /* Already carrying someone. Put them down. Simple and clean. */
    if (char_get_alignment(ch) > 50) {
      carry_drop(g, ch, 0);
    } else {
      carry_drop(g, ch, 1);
    }
    return;
  } else { /* So not carrying already. */
    one_argument(argument, arg);

    if (!*arg) {
      send_to_char(g, ch, "You want to carry who?\r\n");
      return;
    }

    if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "That person isn't here.\r\n");
      return;
    }

    if (char_is_npc(vict)) {
      send_to_char(g, ch, "There's no point in carrying them.\r\n");
      return;
    }

    if (char_get_carried_by(vict) != nullptr) {
      send_to_char(g, ch, "Someone is already carrying them!\r\n");
      return;
    }

    if (char_get_position(vict) > POS_SLEEPING) {
      send_to_char(g, ch, "They are not unconcious.\r\n");
      return;
    }

    if (char_get_pc_weight(g, vict) + char_get_carry_weight(vict) > getMaxCarryWeight(g, ch)) {
      act(g, "@WYou try to pick up @C$N@W but have to put them down. They are too heavy for you at the moment.@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W tries to pick up @c$N@W. After struggling for a moment $e has to put $M down.@n", true, ch, 0, vict, TO_NOTVICT);
      char_set_wait(ch, PULSE_1SEC);
      return;
    } else { /* Let's carry that mofo! */
      act(g, "@WYou pick up @C$N@W and put $M over your shoulder.@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@W picks up $c$N@W and puts $M over $s shoulder.@n", true, ch, 0, vict, TO_NOTVICT);
      if (char_get_sits(vict)) {
        struct obj_data *chair = char_get_sits(vict);
        obj_set_sitting(chair, nullptr);
        char_set_sits(vict, nullptr);
      }
      char_set_carrying(g, ch, vict);
      char_set_carried_by(g, vict, ch);
      char_set_wait(ch, PULSE_1SEC);
      return;
    }

  } /* End new carry target. */
}

/* Handles dropping someone you are carrying. */
void carry_drop(GameWorld *g, struct char_data *ch, int type)
{

  struct char_data *vict = nullptr;

  vict = char_get_carrying(ch);

  switch (type) {
    case 0: /* Awww we were gentle >.> */
      act(g, "@WYou gently set @C$N@W down on the ground.@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n @Wgently sets you down on the ground.@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n @Wgently sets @c$N@W down on the ground.@n", true, ch, 0, vict, TO_NOTVICT);
      break;
    case 1: /* We're not super nice. */
      act(g, "@WYou set @C$N@W hastily onto the ground.@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n @Wsets you hastily onto the ground.@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n @Wsets @c$N@W hastily onto the ground.@n", true, ch, 0, vict, TO_NOTVICT);
      break;
    case 2: /* Uh oh we dropped them from being hit! */
      act(g, "@WYou have @C$N@W knocked out of your arms and onto the ground!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@WYou are knocked out of @C$n's@W arms and onto the ground!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n @Whas @c$N@W knocked out of $s arms and onto the ground!@n", true, ch, 0, vict, TO_NOTVICT);
      break;
    case 3: /* Uh oh they are being extracted! */
      act(g, "@WYou stop carrying @C$N@W for some reason.@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n @Wstops carrying you for some reason.@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n @Wstops carrying @c$N@W for some reason.@n", true, ch, 0, vict, TO_NOTVICT);
      break;
  }
  char_set_carrying(g, ch, nullptr);
  char_set_carried_by(g, vict, nullptr);
}

static room_vnum land_location(GameWorld *g, struct char_data *ch, char *arg)
{

  if (rroom_get_vnum(g, char_get_in_room(ch)) == 50) { // Above Earth
    if (!strcasecmp(arg, "Nexus City")) {
      return (300);
    }
    if (!strcasecmp(arg, "South Ocean")) {
      return (800);
    }
    if (!strcasecmp(arg, "Nexus Field")) {
      return (1150);
    }
    if (!strcasecmp(arg, "Cherry Blossom Mountain")) {
      return (1180);
    }
    if (!strcasecmp(arg, "Sandy Desert")) {
      return (1287);
    }
    if (!strcasecmp(arg, "Northern Plains")) {
      return (1428);
    }
    if (!strcasecmp(arg, "Korin's Tower")) {
      return (1456);
    }
    if (!strcasecmp(arg, "Kami's Lookout")) {
      return (1506);
    }
    if (!strcasecmp(arg, "Shadow Forest")) {
      return (1636);
    }
    if (!strcasecmp(arg, "Decrepit Area")) {
      return (1710);
    }
    if (!strcasecmp(arg, "West City")) {
      return (19510);
    }
    if (!strcasecmp(arg, "Hercule Beach")) {
      return (2141);
    }
    if (!strcasecmp(arg, "Satan City")) {
      return (13020);
    }
    send_to_char(g, ch, "You don't know where that made up place is, but decided to land anyway.");
    return (300);
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 51) { // Above Frigid
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
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 52) { // Above Konack
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
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 53) { // Above Vegeta
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
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 54) { // Above Namek
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
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 55) { // Above Aether
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
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 56) { // Above Yardrat
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
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 198) { // Above Cerria
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
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 57) { // Above Zennith
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
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 58) { // Above Kanassa
    if (!strcasecmp(arg, "Aquis City")) {
      return (14904);
    }
    if (!strcasecmp(arg, "Yunkai Pirate Base")) {
      return (15655);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (14904);
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 59) { // Above Arlia
    if (!strcasecmp(arg, "Janacre")) {
      return (16009);
    }
    if (!strcasecmp(arg, "Arlian Wasteland")) {
      return (16544);
    }
    if (!strcasecmp(arg, "Arlia Mine")) {
      return (16600);
    }
    send_to_char(g, ch, "you don't know where that made up place is, but decided to land anyway.");
    return (16009);
  }
  send_to_char(g, ch, "You are not above a planet!\r\n");
  return (-1);
}

/* This shows the player what locations the planet has to land at. */
static void disp_locations(GameWorld *g, struct char_data *ch)
{
  if (rroom_get_vnum(g, char_get_in_room(ch)) == 50) { // Above Earth
    send_to_char(g, ch, "@D------------------[ @GEarth@D ]------------------@c\n");
    send_to_char(g, ch, "Nexus City, South Ocean, Nexus field, Cherry Blossom Mountain,\n");
    send_to_char(g, ch, "Sandy Desert, Northern Plains, Korin's Tower, Kami's Lookout,\n");
    send_to_char(g, ch, "Shadow Forest, Decrepit Area, West City, Hercule Beach, Satan City.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 51) { // Above Frigid
    send_to_char(g, ch, "@D------------------[ @CFrigid@D ]------------------@c\n");
    send_to_char(g, ch, "Ice Crown City, Ice Highway, Topica Snowfield, Glug's Volcano,\n");
    send_to_char(g, ch, "Platonic Sea, Slave City, Acturian Woods, Desolate Demesne,\n");
    send_to_char(g, ch, "Chateau Ishran, Wyrm Spine Mountain, Cloud Ruler Temple, Koltoan mine.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 52) { // Above Konack
    send_to_char(g, ch, "@D------------------[ @MKonack@D ]------------------@c\n");
    send_to_char(g, ch, "Great Oroist Temple, Elzthuan Forest, Mazori Farm, Dres,\n");
    send_to_char(g, ch, "Colvian Farm, St Alucia, Meridius Memorial, Desert of Illusion,\n");
    send_to_char(g, ch, "Plains of Confusion, Turlon Fair, Wetlands, Kerberos,\n");
    send_to_char(g, ch, "Shaeras Mansion, Slavinus Ravine, Furian Citadel.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 53) { // Above Vegeta
    send_to_char(g, ch, "@D------------------[ @YVegeta@D ]------------------@c\n");
    send_to_char(g, ch, "Vegetos City, Blood Dunes, Ancestral Mountains, Destopa Swamp,\n");
    send_to_char(g, ch, "Pride Forest, Pride tower, Ruby Cave.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 198) { // Above Cerria
    send_to_char(g, ch, "@D------------------[ @MCerria@D ]------------------@c\n");
    send_to_char(g, ch, "Cerria Colony, Fistarl Volcano, Crystalline Forest.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 54) { // Above Namek
    send_to_char(g, ch, "@D------------------[ @gNamek@D ]------------------@c\n");
    send_to_char(g, ch, "Senzu Village, Guru's House, Crystalline Cave, Elder Village,\n");
    send_to_char(g, ch, "Frieza's Ship, Kakureta Village.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 55) { // Above Aether
    send_to_char(g, ch, "@D------------------[ @BAether@D ]-----------------@c\n");
    send_to_char(g, ch, "Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n");
    send_to_char(g, ch, "Silent Glade.\n");
    send_to_char(g, ch, "@D--------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 56) { // Above Yardrat
    send_to_char(g, ch, "@D-----------------[ @mYardrat@D ]-----------------@c\n");
    send_to_char(g, ch, "Yardra City, Jade Forest, Jade Cliffs, Mount Valaria.\n");
    send_to_char(g, ch, "@D-------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 57) { // Above Zennith
    send_to_char(g, ch, "@D-----------------[ @CZennith@D ]-----------------@c\n");
    send_to_char(g, ch, "Utatlan City, Zenith Jungle, Ancient Castle.\n");
    send_to_char(g, ch, "@D-------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 58) { // Above Kanassa
    send_to_char(g, ch, "@D-----------------[ @CKanassa@D ]-----------------@c\n");
    send_to_char(g, ch, "Aquis City, Yunkai Pirate Base.\n");
    send_to_char(g, ch, "@D-------------------------------------------@n\n");
  } else if (rroom_get_vnum(g, char_get_in_room(ch)) == 59) { // Above Arlia
    send_to_char(g, ch, "@D------------------[ @MArlia@D ]------------------@c\n");
    send_to_char(g, ch, "Janacre, Arlian Wasteland, Arlia Mine.\n");
    send_to_char(g, ch, "@D---------------------------------------------@n\n");
  } else {
    send_to_char(g, ch, "You are not above a planet!\r\n");
  }
}

ACMD(do_land)
{

  int above_planet = true;
  room_vnum inroom = rroom_get_vnum(g, char_get_in_room(ch));
  skip_spaces(&argument);

  if (inroom != 50 && inroom != 198 && inroom != 51 && inroom != 52 && inroom != 53 && inroom != 54 && inroom != 55 && inroom != 56 && inroom != 57 && inroom != 58 && inroom != 59) {
    above_planet = false;
  }

  if (!*argument) {
    if (above_planet == true) {
      send_to_char(g, ch, "Land where?\n");
      disp_locations(g, ch);
      return;
    }
    send_to_char(g, ch, "You are not even in the lower atmosphere of a planet!\r\n");
    return;
  }

  room_vnum landing = land_location(g, ch, argument);

  if (landing != -1) {
    room_vnum was_in = rroom_get_vnum(g, char_get_in_room(ch));
    send_to_char(g, ch, "You descend through the upper atmosphere, and coming down through the clouds you land quickly on the ground below.\r\n");
    char_from_room(g, ch);
    char_to_room(g, ch, real_room(g, landing));
    char *blah = sense_location(g, ch);
    char sendback[MAX_INPUT_LENGTH];
    char_from_room(g, ch);
    char_to_room(g, ch, real_room(g, was_in));
    snprintf(sendback, sizeof(sendback), "@C$n@Y flies down through the atmosphere toward @G%s@Y!@n", blah);
    act(g, sendback, true, ch, 0, 0, TO_ROOM);
    char_from_room(g, ch);
    char_to_room(g, ch, real_room(g, landing));
    zone_rnum zone = 0;
    if ((zone = real_zone_by_thing(g, rroom_get_vnum(g, char_get_in_room(ch)))) != NOWHERE) {
      fly_zone(g, zone, "can be seen landing from space nearby!@n\r\n", ch);
    }
    send_to_sense(g, 1, "landing on the planet", ch);
    send_to_scouter(g, "A powerlevel signal has been detected landing on the planet", ch, 0, 1);
    act(g, "$n comes down from high above in the sky and quickly lands on the ground.", true, ch, 0, 0, TO_ROOM);
    return;
  }
}

/* simple function to determine if char can walk on water */
/* simple function to determine if char can fly */
static bool has_flight(GameWorld *g, struct char_data *ch)
{
  struct obj_data *obj;

  if (char_adm_flagged(ch, ADM_WALKANYWHERE))
    return true;

  if (char_aff_flagged(ch, AFF_FLYING) && (getCurKI(g, ch)) >= (char_get_level(ch) + (getMaxKI(g, ch) / (char_get_level(ch) * 30))) && !char_is_android(ch) && !char_is_npc(ch)) {
    return true;
  }
  if (char_aff_flagged(ch, AFF_FLYING) && (getCurKI(g, ch)) < (char_get_level(ch) + (getMaxKI(g, ch) / (char_get_level(ch) * 30))) && !char_is_android(ch) && !char_is_npc(ch)) {
    act(g, "@WYou crash to the ground, too tired to fly anymore!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@W$n@W crashes to the ground!@n", true, ch, 0, 0, TO_ROOM);
    char_aff_set(ch, AFF_FLYING, false);
    handle_fall(g, ch);
    return false;
  }
  if (char_aff_flagged(ch, AFF_FLYING) && char_is_android(ch)) {
    return true;
  }
  if (char_aff_flagged(ch, AFF_FLYING) && char_is_npc(ch)) {
    return true;
  }

  /* non-wearable flying items in inventory will do it */
  for (obj = char_get_inventory(ch); obj; obj = obj_get_next_content(obj))
    if (obj_aff_flagged(obj, AFF_FLYING) && (find_eq_pos(g, ch, obj, nullptr) < 0))
      return true;

  /* anything worn as wings will do */
  return false;
}

/* simple function to determine if char can breathe non-o2 */
bool has_o2(struct char_data *ch)
{
  if (char_adm_flagged(ch, ADM_WALKANYWHERE))
    return true;

  if (char_aff_flagged(ch, AFF_WATERBREATH))
    return true;

  if (char_is_kanassan(ch) || char_is_android(ch) || char_is_icer(ch) || char_is_majin(ch))
    return true;

  return false;
}

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
bool do_simple_move(GameWorld *g, struct char_data *ch, int dir, int need_specials_check)
{
  char throwaway[MAX_INPUT_LENGTH] = ""; /* Functions assume writable. */
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  room_rnum was_in = char_get_in_room(ch);
  int need_movement;
  struct room_data *rm;

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(g, ch, dir + 1, throwaway))
    return false;

  /* blocked by a leave trigger ? */
  if (!leave_mtrigger(g, ch, dir) || char_get_in_room(ch) != was_in) /* prevent teleport crashes */
    return false;
  if (!leave_wtrigger(g, &g->rooms[char_get_in_room(ch)], ch, dir) || char_get_in_room(ch) != was_in) /* prevent teleport crashes */
    return false;
  if (!leave_otrigger(g, &g->rooms[char_get_in_room(ch)], ch, dir) || char_get_in_room(ch) != was_in) /* prevent teleport crashes */
    return false;
  /* charmed? */
  if (char_aff_flagged(ch, AFF_CHARM) && char_get_master(ch) && char_get_in_room(ch) == char_get_in_room(char_get_master(ch))) {
    send_to_char(g, ch, "The thought of leaving your master makes you weep.\r\n");
    act(g, "$n bursts into tears.", false, ch, 0, 0, TO_ROOM);
    return false;
  }

  int willfall = false;
  /* if this room or the one we're going to needs flight, check for it */
  if ((rroom_sect(g, char_get_in_room(ch)) == SECT_FLYING) || (rroom_sect(g, char_get_exit(g, ch, dir)->to_room) == SECT_FLYING)) {
    if (!has_flight(g, ch)) {
      if (dir != 4) {
        willfall = true;
      } else {
        send_to_char(g, ch, "You need to fly to go there!\r\n");
        return false;
      }
    }
  }

  if (((rroom_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM) || (rroom_sect(g, char_get_exit(g, ch, dir)->to_room) == SECT_WATER_NOSWIM)) && char_is_humanoid(ch)) {
    if (char_is_kanassan(ch) && !has_flight(g, ch)) {
      act(g, "@CYou swim swiftly.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@c$n@C swims swiftly.@n", true, ch, 0, 0, TO_ROOM);
    } else if (char_is_icer(ch) && !has_flight(g, ch)) {
      act(g, "@CYou swim swiftly.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@c$n@C swims swiftly.@n", true, ch, 0, 0, TO_ROOM);
    } else if (!char_is_kanassan(ch) && !char_is_icer(ch) && !has_flight(g, ch)) {
      if (!check_swim(g, ch)) {
        return false;
      }
      act(g, "@CYou swim through the cold water.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@c$n@C swim through the cold water.@n", true, ch, 0, 0, TO_ROOM);
      char_set_wait(ch, PULSE_1SEC);
    }
  }

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_SPACE)) {
    if (!char_is_android(ch)) {
      if (!check_swim(g, ch)) {
        return false;
      }
    }
  }

  if (rroom_get_geffect(g, char_get_exit(g, ch, dir)->to_room) == 6 && !char_is_humanoid(ch) && char_is_npc(ch)) {
    return false;
  }

  if (char_is_npc(ch) && rroom_flagged(g, char_get_exit(g, ch, dir)->to_room, ROOM_NOMOB) && !char_get_master(ch)) {
    return false;
  }

  if (rroom_sunken(g, char_get_in_room(ch)) ||
      rroom_sunken(g, char_get_exit(g, ch, dir)->to_room)) {
    if (!has_o2(ch) && ((group_bonus(g, ch, 2) != 10 && (getCurKI(g, ch)) < getMaxKI(g, ch) / 200) || (group_bonus(g, ch, 2) == 10 &&
                                                                                              (getCurKI(g, ch)) < getMaxKI(g, ch) / 800))) {
      if (getCurPL(g, ch) >= getEffMaxPL(g, ch) / 20) {
        send_to_char(g, ch, "@RYou struggle to breath!@n\r\n");
        decCurHealth(g, ch, getMaxPL(g, ch) / 20, 0);
      }
      if (getCurPL(g, ch) < getEffMaxPL(g, ch) / 20) {
        send_to_char(g, ch, "@rYou drown!@n\r\n");
        die(g, ch, nullptr);
        return false;
      }
    }
    if (!has_o2(ch) && ((group_bonus(g, ch, 2) != 10 && (getCurKI(g, ch)) >= getMaxKI(g, ch) / 200) || (group_bonus(g, ch, 2) == 10 &&
                                                                                               (getCurKI(g, ch)) >= getMaxKI(g, ch) / 800))) {
      send_to_char(g, ch, "@CYou hold your breath!@n\r\n");
      if (group_bonus(g, ch, 2) == 10) {
        decCurKI(g, ch, getMaxKI(g, ch) / 800, 0);
      } else {
        decCurKI(g, ch, getMaxKI(g, ch) / 200, 0);
      }
    }
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = 1;
  if (rroom_get_gravity(g, char_get_in_room(ch)) > 10) {
    need_movement = (need_movement + rroom_get_gravity(g, char_get_in_room(ch))) * rroom_get_gravity(g, char_get_in_room(ch));
  } else if (rroom_get_gravity(g, char_get_in_room(ch)) == 10 && !char_is_bardock(ch) && !char_is_npc(ch)) {
    need_movement = (need_movement + rroom_get_gravity(g, char_get_in_room(ch))) * rroom_get_gravity(g, char_get_in_room(ch));
  }
  if (char_get_level(ch) <= 1) {
    need_movement = 0;
  }
  /* Stealth increases your move cost, less if you are good at it */
  if (char_aff_flagged(ch, AFF_HIDE))
    need_movement *= ((roll_skill(g, ch, SKILL_HIDE) > 15) ? 2 : 4);

  if (char_aff_flagged(ch, AFF_SNEAK))
    need_movement *= ((roll_skill(g, ch, SKILL_MOVE_SILENTLY) > 15) ? 1.2 : 2);

  int flight_cost = 0;

  if (char_aff_flagged(ch, AFF_FLYING) && !char_is_android(ch)) {
    if (!char_get_skill(ch, SKILL_CONCENTRATION) && !char_get_skill(ch, SKILL_FOCUS)) {
      flight_cost = getMaxKI(g, ch) / 100;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) && !char_get_skill(ch, SKILL_FOCUS)) {
      flight_cost = getMaxKI(g, ch) / (char_get_skill(ch, SKILL_CONCENTRATION) * 2);
    } else if (!char_get_skill(ch, SKILL_CONCENTRATION) && char_get_skill(ch, SKILL_FOCUS)) {
      flight_cost = getMaxKI(g, ch) / (char_get_skill(ch, SKILL_FOCUS) * 3);
    } else {
      flight_cost = getMaxKI(g, ch) / ((char_get_skill(ch, SKILL_CONCENTRATION) * 2) + (char_get_skill(ch, SKILL_FOCUS) * 3));
    }
  }

  if (char_aff_flagged(ch, AFF_FLYING) && ((getCurKI(g, ch)) < flight_cost) && !char_is_android(ch)) {
    decCurKI(g, ch, flight_cost, 0);
    act(g, "@WYou crash to the ground, too tired to fly anymore!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@W$n@W crashes to the ground!@n", true, ch, 0, 0, TO_ROOM);
    char_aff_set(ch, AFF_FLYING, false);
  } else if (char_aff_flagged(ch, AFF_FLYING) && !char_is_android(ch)) {
    decCurKI(g, ch, flight_cost, 0);
  }

  if ((getCurST(g, ch)) < need_movement && !char_aff_flagged(ch, AFF_FLYING) && !char_is_npc(ch)) {
    if (need_specials_check && char_get_master(ch)) {
      send_to_char(g, ch, "You are too exhausted to follow.\r\n");
    } else {
      send_to_char(g, ch, "You are too exhausted.\r\n");
    }

    return false;
  }

  /* Check if the character needs a skill check to go that way. */
  if (char_get_exit(g, ch, dir)->dcskill != 0) {
    if (char_get_exit(g, ch, dir)->dcmove > roll_skill(g, ch, char_get_exit(g, ch, dir)->dcskill)) {
      send_to_char(g, ch, "Your skill in %s isn't enough to move that way!\r\n", g->spell_info[char_get_exit(g, ch, dir)->dcskill].name);
      /* A failed skill check still spends the movement points! */
      if (!char_adm_flagged(ch, ADM_WALKANYWHERE) && !char_is_npc(ch) && !char_aff_flagged(ch, AFF_FLYING))
        decCurST(g, ch, need_movement, 0);
      return false;
    }
    send_to_char(g, ch, "Your skill in %s aids in your movement.\r\n", g->spell_info[char_get_exit(g, ch, dir)->dcskill].name);
  }

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_ATRIUM)) {
    if (!House_can_enter(g, ch, rroom_get_vnum(g, char_get_exit(g, ch, dir)->to_room))) {
      send_to_char(g, ch, "That's private property -- no trespassing!\r\n");
      return false;
    }
  }
  if (rroom_flagged(g, char_get_exit(g, ch, dir)->to_room, ROOM_TUNNEL) &&
      (num_pc_in_room(&(g->rooms[char_get_exit(g, ch, dir)->to_room])) >= CONFIG_TUNNEL_SIZE)) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(g, ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(g, ch, "There isn't enough room there for more than one person!\r\n");
    return false;
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (rroom_flagged(g, char_get_exit(g, ch, dir)->to_room, ROOM_GODROOM) &&
      char_get_admlevel(ch) < ADMLVL_GRGOD) {
    send_to_char(g, ch, "You aren't godly enough to use that room!\r\n");
    return false;
  }

  /******* Zone flag checks *******/

  rm = &g->rooms[char_get_exit(g, ch, dir)->to_room];

  if (!char_is_npc(ch) && (char_get_admlevel(ch) < ADMLVL_IMMORT) &&
      (char_get_level(ch) < zone_get_min_level(g, room_get_zone(rm))) && (zone_get_min_level(g, room_get_zone(rm)) > 0)) {
    send_to_char(g, ch, "Sorry, you are too low a level to enter this zone.\r\n");
    return false;
  }

  if ((char_get_admlevel(ch) < ADMLVL_IMMORT) && (char_get_level(ch) > zone_get_max_level(g, room_get_zone(rm))) &&
      (zone_get_max_level(g, room_get_zone(rm)) > 0)) {
    send_to_char(g, ch, "Sorry, you are too high a level to enter this zone.\r\n");
    return false;
  }

  if ((char_get_admlevel(ch) < ADMLVL_IMMORT) && zone_flagged(g, room_get_zone(rm), ZONE_CLOSED)) {
    send_to_char(g, ch, "This zone is currently closed to mortals.\r\n");
    return false;
  }

  if ((char_get_admlevel(ch) >= ADMLVL_IMMORT && char_get_admlevel(ch) < ADMLVL_GRGOD) && zone_flagged(g, room_get_zone(rm), ZONE_NOIMMORT)) {
    send_to_char(g, ch, "This zone is closed to all.\r\n");
    return false;
  }

  /* No low level immortal scouting */
  if ((char_get_admlevel(ch) >= ADMLVL_IMMORT && char_get_admlevel(ch) < ADMLVL_GOD) &&
      !can_edit_zone(g, ch, room_get_zone(rm)) && zone_flagged(g, room_get_zone(rm), ZONE_QUEST)) {
    send_to_char(g, ch, "This is a Quest zone.\r\n");
    return false;
  }

  /* Now we know we're allowed to go into the room. */
  if (!char_adm_flagged(ch, ADM_WALKANYWHERE) && !char_is_npc(ch) && !char_aff_flagged(ch, AFF_FLYING)) {
    decCurST(g, ch, need_movement, 0);
  }

  if (char_aff_flagged(ch, AFF_SNEAK) && !char_is_npc(ch)) {
    snprintf(buf2, sizeof(buf2), "$n sneaks %s.", dirs[dir]);
    if (char_get_skill(ch, SKILL_MOVE_SILENTLY)) {
      improve_skill(g, ch, SKILL_MOVE_SILENTLY, 0);
    } else if (slot_count(ch) + 1 > char_get_skill_slots(ch)) {
      send_to_char(g, ch, "@RYour skill slots are full. You can not learn Move Silently.\r\n");
      char_aff_set(ch, AFF_SNEAK, false);
    } else {
      send_to_char(g, ch, "@GYou learn the very basics of moving silently.@n\r\n");
      char_set_skills_base(ch, SKILL_MOVE_SILENTLY, rand_number(g, 5, 10));
      act(g, buf2, true, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
      if (char_get_aff_abils_dex(ch) < rand_number(g, 1, 30)) {
        char_set_wait(ch, PULSE_1SEC);
      }
    }
  }

  if (!char_aff_flagged(ch, AFF_SNEAK) && !char_aff_flagged(ch, AFF_FLYING)) {
    snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
    act(g, buf2, true, ch, 0, 0, TO_ROOM);
  }
  if (!char_aff_flagged(ch, AFF_SNEAK) && char_aff_flagged(ch, AFF_FLYING)) {
    snprintf(buf2, sizeof(buf2), "$n flies %s.", dirs[dir]);
    act(g, buf2, true, ch, 0, 0, TO_ROOM);
  }

  was_in = char_get_in_room(ch);
  if (char_get_drag(ch)) {
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
  }
  if (char_get_carrying(ch)) {
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, 0, char_get_carrying(ch), TO_ROOM);
  }
  char_aff_set(ch, AFF_PURSUIT, true);
  char_from_room(g, ch);
  char_to_room(g, ch, rroom_get_dir_option(g, was_in, dir)->to_room);
  if ((rroom_get_zone(g, char_get_in_room(ch)) != rroom_get_zone(g, was_in)) && !char_is_npc(ch) && !char_is_android(ch)) {
    send_to_sense(g, 0, "You sense someone", ch);
    snprintf(buf3, sizeof(buf3), "@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection range@n.", add_commas(getCurPL(g, ch)));
    send_to_scouter(g, buf3, ch, 0, 0);
  }
  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(g, ch) || !enter_wtrigger(g, &g->rooms[char_get_in_room(ch)], ch, dir)) {
    char_from_room(g, ch);
    char_to_room(g, ch, was_in);
    char_aff_set(ch, AFF_PURSUIT, false);
    return false;
  }

  snprintf(buf2, sizeof(buf2), "%s%s",
           ((dir == UP) || (dir == DOWN) ? "" : "the "),
           (dir == UP ? "below" : (dir == DOWN) ? "above"
                                                : dirs[rev_dir[dir]]));
  act(g, "$n arrives from $T.", true, ch, 0, buf2, TO_ROOM | TO_SNEAKRESIST);
  if (char_get_fighting(ch)) {
    if (rroom_sect(g, rroom_get_dir_option(g, was_in, dir)->to_room) != SECT_FLYING && rroom_sect(g, rroom_get_dir_option(g, was_in, dir)->to_room) != SECT_WATER_NOSWIM && rroom_get_geffect(g, rroom_get_dir_option(g, was_in, dir)->to_room) == 0) {
      roll_pursue(g, char_get_fighting(ch), ch);
    }
    char_aff_set(ch, AFF_PURSUIT, false);
  }
  if (char_get_drag(ch)) {
    act(g, "@wYou drag @C$N@w with you.@n", true, ch, 0, char_get_drag(ch), TO_CHAR);
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
    char_from_room(g, char_get_drag(ch));
    char_to_room(g, char_get_drag(ch), char_get_in_room(ch));
    if (char_get_sits(char_get_drag(ch))) {
      obj_from_room(g, char_get_sits(char_get_drag(ch)));
      obj_to_room(g, char_get_sits(char_get_drag(ch)), char_get_in_room(ch));
    }
    if (!char_aff_flagged(char_get_drag(ch), AFF_KNOCKED) && !char_aff_flagged(char_get_drag(ch), AFF_SLEEP) && rand_number(g, 1, 3)) {
      send_to_char(g, char_get_drag(ch), "You feel your sleeping body being moved.\r\n");
      if (char_is_npc(char_get_drag(ch)) && !char_get_fighting(char_get_drag(ch))) {
        set_fighting(g, char_get_drag(ch), ch);
      }
    }
  }
  if (char_get_carrying(ch)) {
    act(g, "@wYou carry @C$N@w with you.@n", true, ch, 0, char_get_carrying(ch), TO_CHAR);
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, 0, char_get_carrying(ch), TO_ROOM);
    char_from_room(g, char_get_carrying(ch));
    char_to_room(g, char_get_carrying(ch), char_get_in_room(ch));
    if (!char_aff_flagged(char_get_carrying(ch), AFF_KNOCKED) && !char_aff_flagged(char_get_carrying(ch), AFF_SLEEP) && rand_number(g, 1, 3)) {
      send_to_char(g, char_get_carrying(ch), "You feel your sleeping body being moved.\r\n");
    }
  }

  if (char_get_desc(ch) != nullptr) {
    look_at_room(g, char_get_in_room(ch), ch, 0);
    if (char_aff_flagged(ch, AFF_SNEAK) && !char_is_npc(ch) && char_get_skill(ch, SKILL_MOVE_SILENTLY) && char_get_skill(ch, SKILL_MOVE_SILENTLY) < rand_number(g, 1, 101)) {
      send_to_char(g, ch, "@wYou make a noise as you arrive and are no longer sneaking!@n\r\n");
      act(g, "@c$n@w makes a noise revealing $s sneaking!@n", true, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
      reveal_hiding(g, ch, 0);
      char_aff_set(ch, AFF_SNEAK, false);
    }
  }

  if (rroom_get_geffect(g, char_get_in_room(ch)) == 6 || rroom_get_geffect(g, was_in) == 6) {
    if (!char_is_demon(ch) && !char_aff_flagged(ch, AFF_FLYING) && group_bonus(g, ch, 2) != 14) {
      act(g, "@rYour legs are burned by the lava!@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@R$n@r's legs are burned by the lava!@n", true, ch, 0, 0, TO_ROOM);
      if (char_is_npc(ch) && char_is_humanoid(ch) && rand_number(g, 1, 2) == 2) {
        do_fly(g, ch, 0, 0, 0);
      }
      decCurHealth(g, ch, getEffMaxPL(g, ch) / 20, 0);
      if (getCurPL(g, ch) <= 0) {
        act(g, "@rYou have burned to death!@n", true, ch, 0, 0, TO_CHAR);
        act(g, "@R$n@r has burned to death!@n", true, ch, 0, 0, TO_ROOM);
        die(g, ch, nullptr);
      }
    }
    if (char_get_drag(ch) && !char_is_demon(char_get_drag(ch))) {
      act(g, "@R$N@r gets burned!@n", true, ch, 0, char_get_drag(ch), TO_CHAR);
      act(g, "@R$N@r gets burned!@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
      decCurHealth(g, char_get_drag(ch), getEffMaxPL(g, char_get_drag(ch)) / 20, 0);
      if (getCurPL(g, char_get_drag(ch)) < 0) {
        act(g, "@rYou have burned to death!@n", true, char_get_drag(ch), 0, 0, TO_CHAR);
        act(g, "@R$n@r has burned to death!@n", true, char_get_drag(ch), 0, 0, TO_ROOM);
        die(g, char_get_drag(ch), nullptr);
      }
    }
  }

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_TIMED_DT) && !char_adm_flagged(ch, ADM_WALKANYWHERE))
    timed_dt(g, nullptr);

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_DEATH) && !char_adm_flagged(ch, ADM_WALKANYWHERE)) {
    log_death_trap(g, ch);
    death_cry(g, ch);
    extract_char(g, ch);
    return false;
  }

  entry_memory_mtrigger(g, ch);
  if (!greet_mtrigger(g, ch, dir)) {
    char_from_room(g, ch);
    char_to_room(g, ch, was_in);
    look_at_room(g, char_get_in_room(ch), ch, 0);
  } else greet_memory_mtrigger(g, ch);
  if (willfall == true) {
    handle_fall(g, ch);
    if (char_get_drag(ch)) {
      handle_fall(g, char_get_drag(ch));
    }
  }
  return true;
}

bool perform_move(GameWorld *g, struct char_data *ch, int dir, int need_specials_check)
{
  room_rnum was_in;
  struct follow_type *k, *next;

  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return false;
  }

  if (char_get_absorbing(ch) || char_get_absorbby(ch)) {
    send_to_char(g, ch, "You are struggling with someone!\r\n");
    return false;
  }

  if (!char_aff_flagged(ch, AFF_SNEAK) || (char_aff_flagged(ch, AFF_SNEAK) && char_get_skill(ch, SKILL_MOVE_SILENTLY) < axion_dice(g, 0))) {
    reveal_hiding(g, ch, 0);
  }

  if (ch == nullptr || dir < 0 || dir >= NUM_OF_DIRS)
    return false;
  else if ((!char_get_exit(g, ch, dir) && !buildwalk(g, ch, dir)) || char_get_exit(g, ch, dir)->to_room == NOWHERE || (room_exit_flagged(char_get_exit(g, ch, dir), EX_SECRET) && (room_exit_flagged(char_get_exit(g, ch, dir), EX_CLOSED))))
    send_to_char(g, ch, "Alas, you cannot go that way...\r\n");
  else if (room_exit_flagged(char_get_exit(g, ch, dir), EX_CLOSED)) {
    if (char_get_exit(g, ch, dir)->keyword)
      send_to_char(g, ch, "The %s seems to be closed.\r\n", fname(char_get_exit(g, ch, dir)->keyword));
    else
      send_to_char(g, ch, "It seems to be closed.\r\n");
  } else if (rroom_get_vnum(g, char_get_exit(g, ch, dir)->to_room) == 0 || rroom_get_vnum(g, char_get_exit(g, ch, dir)->to_room) == 1) {
    send_to_char(g, ch, "Report this direction, it is illegal.\r\n");
  } else {

    struct obj_data *wall;
    for (wall = rroom_get_contents(g, char_get_in_room(ch)); wall; wall = obj_get_next_content(wall)) {
      if (obj_get_vnum(g, wall) == 79) {
        if (obj_get_cost(wall) == dir) {
          send_to_char(g, ch, "That direction has a glacial wall blocking it.\r\n");
          return false;
        }
      }
    }

    if (!char_get_followers(ch))
      return (do_simple_move(g, ch, dir, need_specials_check));

    was_in = char_get_in_room(ch);
    if (!do_simple_move(g, ch, dir, need_specials_check))
      return false;

    for (k = char_get_followers(ch); k; k = next) {
      next = k->next;
      if ((char_get_in_room(k->follower) == was_in) &&
          (char_get_position(k->follower) >= POS_STANDING) &&
          (!char_aff_flagged(ch, AFF_ZANZOKEN) ||
           (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(k->follower, AFF_GROUP)))) {
        act(g, "You follow $N.\r\n", false, k->follower, 0, ch, TO_CHAR);
        perform_move(g, k->follower, dir, 1);
      } else if ((char_get_in_room(k->follower) == was_in) &&
                 (char_get_position(k->follower) >= POS_STANDING) &&
                 (char_aff_flagged(ch, AFF_ZANZOKEN) && char_aff_flagged(k->follower, AFF_ZANZOKEN)) &&
                 (!char_aff_flagged(ch, AFF_GROUP) || !char_aff_flagged(k->follower, AFF_GROUP))) {
        act(g, "$N tries to zanzoken and escape, but your zanzoken matches $S!\r\n", false, k->follower, 0, ch, TO_CHAR);
        act(g, "$N tries to zanzoken and escape, but $n's zanzoken matches $S!\r\n", false, k->follower, 0, ch, TO_NOTVICT);
        act(g, "You zanzoken to try and escape, but $n's zanzoken matches yours!\r\n", false, k->follower, 0, ch, TO_VICT);
        char_aff_set(ch, AFF_ZANZOKEN, false);
        char_aff_set(k->follower, AFF_ZANZOKEN, false);
        perform_move(g, k->follower, dir, 1);
      } else if ((char_get_in_room(k->follower) == was_in) &&
                 (char_get_position(k->follower) >= POS_STANDING) &&
                 (char_aff_flagged(ch, AFF_ZANZOKEN) && !char_aff_flagged(k->follower, AFF_ZANZOKEN))) {
        act(g, "You try to follow $N, but $E disappears in a flash of movement!\r\n", false, k->follower, 0, ch, TO_CHAR);
        act(g, "$n tries to follow $N, but $E disappears in a flash of movement!\r\n", false, k->follower, 0, ch, TO_NOTVICT);
        act(g, "$n tries to follow you, but you manage to zanzoken away!\r\n", false, k->follower, 0, ch, TO_VICT);
        char_aff_set(ch, AFF_ZANZOKEN, false);
      }
    }
    return true;
  }
  return false;
}

ACMD(do_move)
{
  if (char_is_npc(ch)) {
    perform_move(g, ch, subcmd - 1, 0);
    return;
  }
  if (char_plr_flagged(ch, PLR_SELFD)) {
    send_to_char(g, ch, "You are preparing to blow up!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_LIQUEFIED)) {
    send_to_char(g, ch, "You are liquefied right now!\r\n");
    return;
  }
  if (char_get_charge(ch) >= getMaxKI(g, ch) * .51) {
    send_to_char(g, ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
    return;
  }
  if ((char_get_charge(ch) >= getMaxKI(g, ch) * .5 && char_get_charge(ch) < getMaxKI(g, ch) * .51) && char_get_skill(ch, SKILL_CONCENTRATION) < 100) {
    send_to_char(g, ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
    return;
  }
  if ((char_get_charge(ch) >= getMaxKI(g, ch) * .4 && char_get_charge(ch) < getMaxKI(g, ch) * .5) && char_get_skill(ch, SKILL_CONCENTRATION) < 80) {
    send_to_char(g, ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
    return;
  }
  if ((char_get_charge(ch) >= getMaxKI(g, ch) * .3 && char_get_charge(ch) < getMaxKI(g, ch) * .4) && char_get_skill(ch, SKILL_CONCENTRATION) < 70) {
    send_to_char(g, ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
    return;
  }
  if ((char_get_charge(ch) >= getMaxKI(g, ch) * .2 && char_get_charge(ch) < getMaxKI(g, ch) * .3) && char_get_skill(ch, SKILL_CONCENTRATION) < 60) {
    send_to_char(g, ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
    return;
  }

  if (char_get_conditions(g, ch, DRUNK) > 4 && (rand_number(g, 1, 9) + char_get_conditions(g, ch, DRUNK)) >= rand_number(g, 14, 20)) {
    send_to_char(g, ch, "You wobble around and then fall on your ass.\r\n");
    act(g, "@C$n@W wobbles around before falling on $s ass@n.", true, ch, 0, 0, TO_ROOM);
    char_set_position(ch, POS_SITTING);
    return;
  }

  if (char_get_fighting(ch) && !char_is_npc(ch)) {
    char blah[MAX_INPUT_LENGTH];
    snprintf(blah, sizeof(blah), "%s", dirs[subcmd - 1]);
    do_flee(g, ch, blah, 0, 0);
    return;
  }

  /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  if (char_plr_flagged(ch, PLR_PILOTING)) {
    struct obj_data *vehicle = nullptr, *controls = nullptr;
    int noship = false;
    if (!(controls = find_control(g, ch)) && char_get_admlevel(ch) < 1) {
      noship = true;
    } else if (!(vehicle = find_vehicle_by_vnum(g, obj_get_value(controls, 0)))) {
      noship = true;
    }
    if (noship == true) {
      send_to_char(g, ch, "Your ship controls are not here or your ship was not found, report to Iovan!\r\n");
      return;
    }
    if (controls != nullptr && vehicle != nullptr) {
      if (obj_get_value(controls, 2) <= 0) {
        send_to_char(g, ch, "The ship is out of fuel!\r\n");
        return;
      }
      drive_in_direction(g, ch, vehicle, subcmd - 1);
      if (obj_get_value(controls, 1) == 1) {
        char_set_wait(ch, PULSE_2SEC);
      } else if (obj_get_value(controls, 1) == 2) {
        char_set_wait(ch, PULSE_1SEC);
      }
      controls = nullptr;
      vehicle  = nullptr;
      return;
    }
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (!char_is_npc(ch)) {
    int fail = false;
    struct obj_data *obj, *next_obj;
    for (obj = rroom_get_contents(g, char_get_in_room(ch)); obj; obj = next_obj) {
      next_obj = obj_get_next_content(obj);
      if (obj_get_kicharge(obj) > 0 && obj_get_user(obj) == ch) {
        fail = true;
      }
    }
    if (fail == true) {
      send_to_char(g, ch, "You are too busy controlling your attack!\r\n");
      return;
    }
  }

  if (!char_is_npc(ch) && char_get_limb_condition(ch, (1) - 1) <= 0 && char_get_limb_condition(ch, (2) - 1) <= 0 && char_get_limb_condition(ch, (3) - 1) <= 0 && char_get_limb_condition(ch, (4) - 1) <= 0 && !char_aff_flagged(ch, AFF_FLYING)) {
    send_to_char(g, ch, "Unless you fly, you can't get far with no limbs.\r\n");
    return;
  }
  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return;
  }
  if (char_get_absorbing(ch)) {
    send_to_char(g, ch, "You are busy absorbing from %s!\r\n", char_get_name(char_get_absorbing(ch)));
    return;
  }
  if (char_get_absorbby(ch)) {
    if (axion_dice(g, 0) < char_get_skill(char_get_absorbby(ch), SKILL_ABSORB)) {
      send_to_char(g, ch, "You are being held by %s, they are absorbing you!\r\n", char_get_name(char_get_absorbby(ch)));
      send_to_char(g, char_get_absorbby(ch), "%s struggles in your grasp!\r\n", char_get_name(ch));
      char_set_wait(ch, PULSE_2SEC);
      return;
    }
    act(g, "@c$N@W manages to break loose of @C$n's@W hold!@n", true, char_get_absorbby(ch), 0, ch, TO_NOTVICT);
    act(g, "@WYou manage to break loose of @C$n's@W hold!@n", true, char_get_absorbby(ch), 0, ch, TO_VICT);
    act(g, "@c$N@W manages to break loose of your hold!@n", true, char_get_absorbby(ch), 0, ch, TO_CHAR);
    char_set_absorbing(char_get_absorbby(ch), nullptr);
    char_set_absorbby(ch, nullptr);
  }
  if (!block_calc(g, ch)) {
    return;
  }
  if (char_get_listenroom(ch) > 0) {
    send_to_char(g, ch, "You stop eavesdropping.\r\n");
    char_set_listenroom(ch, real_room(g, 0));
  }
  if (!char_is_npc(ch)) {
    if (char_prf_flagged(g, ch, PRF_ARENAWATCH)) {
      char_prf_set(g, ch, PRF_ARENAWATCH, false);
      char_set_arenawatch(ch, -1);
    }
    if (rroom_get_vnum(g, char_get_in_room(ch)) != NOWHERE && rroom_get_vnum(g, char_get_in_room(ch)) != 0 && rroom_get_vnum(g, char_get_in_room(ch)) != 1) {
      char_set_load_room(g, ch, rroom_get_vnum(g, char_get_in_room(ch)));
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 10 && getEffMaxPL(g, ch) <= 10000 && !char_is_bardock(ch) && !char_is_npc(ch)) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_1SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 20 && getEffMaxPL(g, ch) <= 30000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_2SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 30 && getEffMaxPL(g, ch) <= 100000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_3SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 40 && getEffMaxPL(g, ch) <= 200000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_3SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 50 && getEffMaxPL(g, ch) <= 300000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_3SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 100 && getEffMaxPL(g, ch) <= 500000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_3SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 200 && getEffMaxPL(g, ch) <= 1000000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_3SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 300 && getEffMaxPL(g, ch) <= 8000000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_3SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 400 && getEffMaxPL(g, ch) <= 15000000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_3SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 500 && getEffMaxPL(g, ch) <= 25000000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_4SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 1000 && getEffMaxPL(g, ch) <= 35000000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_5SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 5000 && getEffMaxPL(g, ch) <= 100000000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_5SEC);
    }
    if (rroom_get_gravity(g, char_get_in_room(ch)) == 10000 && getEffMaxPL(g, ch) <= 200000000) {
      send_to_char(g, ch, "The gravity slows you down some.\r\n");
      char_set_wait(ch, PULSE_5SEC);
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_SPACE) && char_get_admlevel(ch) < 1) {
      send_to_char(g, ch, "You struggle to cross the vast distance.\r\n");
      char_set_wait(ch, PULSE_6SEC);
    } else if ((char_get_limb_condition(ch, (3) - 1) <= 0 && char_get_limb_condition(ch, (4) - 1) <= 0) && char_get_limb_condition(ch, (1) - 1) <= 0 && !char_aff_flagged(ch, AFF_FLYING)) {
      act(g, "@wYou slowly pull yourself along with your arm...@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@w slowly pulls $mself along with one arm...@n", true, ch, 0, 0, TO_ROOM);
      if (char_get_limb_condition(ch, (2) - 1) < 50) {
        send_to_char(g, ch, "@RYour left arm is damaged by the forced use!@n\r\n");
        char_set_limb_condition(ch, (2) - 1, char_get_limb_condition(ch, (2) - 1) - (rand_number(g, 1, 5)));
        if (char_get_limb_condition(ch, (1) - 1) <= 0) {
          act(g, "@RYour left arm falls apart!@n", true, ch, 0, 0, TO_CHAR);
          act(g, "@r$n's@R left arm falls apart!@n", true, ch, 0, 0, TO_ROOM);
        }
      }
      char_set_wait(ch, PULSE_5SEC);
    } else if ((char_get_limb_condition(ch, (3) - 1) <= 0 && char_get_limb_condition(ch, (4) - 1) <= 0) && char_get_limb_condition(ch, (2) - 1) <= 0 && !char_aff_flagged(ch, AFF_FLYING)) {
      act(g, "@wYou slowly pull yourself along with your arm...@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@w slowly pulls $mself along with one arm...@n", true, ch, 0, 0, TO_ROOM);
      if (char_get_limb_condition(ch, (1) - 1) < 50) {
        send_to_char(g, ch, "@RYour right arm is damaged by the forced use!@n\r\n");
        char_set_limb_condition(ch, (1) - 1, char_get_limb_condition(ch, (1) - 1) - (rand_number(g, 1, 5)));
        if (char_get_limb_condition(ch, (1) - 1) <= 0) {
          act(g, "@RYour right arm falls apart!@n", true, ch, 0, 0, TO_CHAR);
          act(g, "@r$n's@R right arm falls apart!@n", true, ch, 0, 0, TO_ROOM);
        }
      }
      char_set_wait(ch, PULSE_5SEC);
    } else if ((char_get_limb_condition(ch, (3) - 1) <= 0 && char_get_limb_condition(ch, (4) - 1) <= 0) && !char_aff_flagged(ch, AFF_FLYING)) {
      act(g, "@wYou slowly pull yourself along with your arms...@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@w slowly pulls $mself along with one arms...@n", true, ch, 0, 0, TO_ROOM);
      if (char_get_limb_condition(ch, (2) - 1) < 50) {
        send_to_char(g, ch, "@RYour left arm is damaged by the forced use!@n\r\n");
        char_set_limb_condition(ch, (2) - 1, char_get_limb_condition(ch, (2) - 1) - (rand_number(g, 1, 5)));
        if (char_get_limb_condition(ch, (2) - 1) <= 0) {
          act(g, "@RYour left arm falls apart!@n", true, ch, 0, 0, TO_CHAR);
          act(g, "@r$n's@R left arm falls apart!@n", true, ch, 0, 0, TO_ROOM);
        }
      }
      if (char_get_limb_condition(ch, (1) - 1) < 50) {
        send_to_char(g, ch, "@RYour right arm is damaged by the forced use!@n\r\n");
        char_set_limb_condition(ch, (1) - 1, char_get_limb_condition(ch, (1) - 1) - (rand_number(g, 1, 5)));
        if (char_get_limb_condition(ch, (1) - 1) <= 0) {
          act(g, "@RYour right arm falls apart!@n", true, ch, 0, 0, TO_CHAR);
          act(g, "@r$n's@R right arm falls apart!@n", true, ch, 0, 0, TO_ROOM);
        }
      }
      char_set_wait(ch, PULSE_3SEC);
    } else if (char_get_limb_condition(ch, (3) - 1) <= 0 && !char_aff_flagged(ch, AFF_FLYING)) {
      act(g, "@wYou hop on one leg...@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@w hops on one leg...@n", true, ch, 0, 0, TO_ROOM);
      if (char_get_limb_condition(ch, (4) - 1) < 50) {
        send_to_char(g, ch, "@RYour left leg is damaged by the forced use!@n\r\n");
        char_set_limb_condition(ch, (4) - 1, char_get_limb_condition(ch, (4) - 1) - (rand_number(g, 1, 5)));
        if (char_get_limb_condition(ch, (4) - 1) <= 0) {
          act(g, "@RYour left leg falls apart!@n", true, ch, 0, 0, TO_CHAR);
          act(g, "@r$n's@R left leg falls apart!@n", true, ch, 0, 0, TO_ROOM);
        }
      }
      char_set_wait(ch, PULSE_2SEC);
    } else if (char_get_limb_condition(ch, (4) - 1) <= 0 && !char_aff_flagged(ch, AFF_FLYING)) {
      act(g, "@wYou hop on one leg...@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@w hops on one leg...@n", true, ch, 0, 0, TO_ROOM);
      if (char_get_limb_condition(ch, (3) - 1) < 50) {
        send_to_char(g, ch, "@RYour right leg is damaged by the forced use!@n\r\n");
        char_set_limb_condition(ch, (3) - 1, char_get_limb_condition(ch, (3) - 1) - (rand_number(g, 1, 5)));
        if (char_get_limb_condition(ch, (3) - 1) <= 0) {
          act(g, "@RYour right leg falls apart!@n", true, ch, 0, 0, TO_CHAR);
          act(g, "@r$n's@R right leg falls apart!@n", true, ch, 0, 0, TO_ROOM);
        }
      }
      char_set_wait(ch, PULSE_2SEC);
    } else if (char_get_position(ch) == POS_RESTING) {
      act(g, "@wYou crawl on your hands and knees.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@w crawls on $s hands and knees.@n", true, ch, 0, 0, TO_ROOM);
      if (char_get_sits(ch)) {
        struct obj_data *chair = char_get_sits(ch);
        obj_set_sitting(chair, nullptr);
        char_set_sits(ch, nullptr);
      }
      char_set_wait(ch, PULSE_3SEC);
    } else if (char_get_position(ch) == POS_SITTING) {
      act(g, "@wYou shuffle on your hands and knees.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@w shuffles on $s hands and knees.@n", true, ch, 0, 0, TO_ROOM);
      if (char_get_sits(ch)) {
        struct obj_data *chair = char_get_sits(ch);
        obj_set_sitting(chair, nullptr);
        char_set_sits(ch, nullptr);
      }
      char_set_wait(ch, PULSE_2SEC);
    } else if (char_get_position(ch) < POS_RESTING) {
      send_to_char(g, ch, "You are in no condition to move! Try standing...\r\n");
      return;
    }
  }
  perform_move(g, ch, subcmd - 1, 0);
  if (char_get_rdisplay(ch)) {
    if (strcmp(char_get_rdisplay(ch), "Empty")) {
      char_set_rdisplay(ch, "Empty");
    }
  }
}

static int find_door(GameWorld *g, struct char_data *ch, const char *type, char *dir, const char *cmdname)
{
  int door;

  if (*dir) { /* a direction was specified */
    if ((door = search_block(dir, dirs, false)) < 0 &&
        (door = search_block(dir, abbr_dirs, false)) < 0) { /* Partial Match */
      send_to_char(g, ch, "That's not a direction.\r\n");
      return (-1);
    }
    if (char_get_exit(g, ch, door)) { /* Braces added according to indent. -gg */
      if (char_get_exit(g, ch, door)->keyword) {
        if (is_name(type, char_get_exit(g, ch, door)->keyword))
          return (door);
        else {
          send_to_char(g, ch, "I see no %s there.\r\n", type);
          return (-1);
        }
      } else
        return (door);
    } else {
      send_to_char(g, ch, "I really don't see how you can %s anything there.\r\n", cmdname);
      return (-1);
    }
  } else { /* try to locate the keyword */
    if (!*type) {
      send_to_char(g, ch, "What is it you want to %s?\r\n", cmdname);
      return (-1);
    }
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (char_get_exit(g, ch, door))
        if (char_get_exit(g, ch, door)->keyword)
          if (is_name(type, char_get_exit(g, ch, door)->keyword))
            return (door);

    send_to_char(g, ch, "There doesn't seem to be %s %s that could be manipulated in that way here.\r\n", AN(type), type);
    return (-1);
  }
}

static bool has_key(GameWorld *g, struct char_data *ch, obj_vnum key)
{
  struct obj_data *o;
  int i;

  if (key == 1) {
    return true;
  }

  for (o = char_get_inventory(ch); o; o = obj_get_next_content(o))
    if (obj_get_vnum(g, o) == key)
      return true;

  for (i = 0; i < NUM_WEARS; i++)
    if (char_get_equipment(ch, i))
      if (obj_get_vnum(g, char_get_equipment(ch, i)) == key)
        return true;

  return false;
}

constexpr int NEED_OPEN     = (1 << 0);
constexpr int NEED_CLOSED   = (1 << 1);
constexpr int NEED_UNLOCKED = (1 << 2);
constexpr int NEED_LOCKED   = (1 << 3);

const char *const cmd_door[NUM_DOOR_CMD] =
    {
        "open",
        "close",
        "unlock",
        "lock",
        "pick"};

static const int flags_door[] =
    {
        NEED_CLOSED | NEED_UNLOCKED,
        NEED_OPEN,
        NEED_CLOSED | NEED_LOCKED,
        NEED_CLOSED | NEED_UNLOCKED,
        NEED_CLOSED | NEED_LOCKED};

#define OPEN_DOOR(g, room, obj, door)   ((obj) ? (obj_set_value(obj, VAL_CONTAINER_FLAGS, obj_get_value(obj, VAL_CONTAINER_FLAGS) & ~(CONT_CLOSED))) : (void)(REMOVE_BIT(rroom_get_dir_option(g, room, door)->exit_info, EX_CLOSED)))
#define CLOSE_DOOR(g, room, obj, door)  ((obj) ? (obj_set_value(obj, VAL_CONTAINER_FLAGS, obj_get_value(obj, VAL_CONTAINER_FLAGS) | (CONT_CLOSED))) : (void)(SET_BIT(rroom_get_dir_option(g, room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(g, room, obj, door)   ((obj) ? (obj_set_value(obj, VAL_CONTAINER_FLAGS, obj_get_value(obj, VAL_CONTAINER_FLAGS) | (CONT_LOCKED))) : (void)(SET_BIT(rroom_get_dir_option(g, room, door)->exit_info, EX_LOCKED)))
#define UNLOCK_DOOR(g, room, obj, door) ((obj) ? (obj_set_value(obj, VAL_CONTAINER_FLAGS, obj_get_value(obj, VAL_CONTAINER_FLAGS) & ~(CONT_LOCKED))) : (void)(REMOVE_BIT(rroom_get_dir_option(g, room, door)->exit_info, EX_LOCKED)))
#define TOGGLE_LOCK(g, room, obj, door) ((obj) ? (obj_set_value(obj, VAL_CONTAINER_FLAGS, obj_get_value(obj, VAL_CONTAINER_FLAGS) ^ (CONT_LOCKED))) : (void)(TOGGLE_BIT(rroom_get_dir_option(g, room, door)->exit_info, EX_LOCKED)))

static void do_doorcmd(GameWorld *g, struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  char buf[MAX_STRING_LENGTH];
  size_t len;
  room_rnum num                    = 0;
  room_rnum other_room             = NOWHERE;
  struct room_direction_data *back = nullptr;
  struct obj_data *hatch = nullptr, *obj2 = nullptr, *next_obj, *vehicle = nullptr;

  if ((obj) && obj_get_type_flag(obj) == ITEM_HATCH) {
    vehicle = find_vehicle_by_vnum(g, obj_get_value(obj, VAL_HATCH_DEST));
  } else if ((obj) && obj_get_type_flag(obj) == ITEM_VEHICLE) {
    if (real_room(g, obj_get_value(obj, VAL_PORTAL_DEST)) != NOWHERE) {
      num = char_get_in_room(ch);
      char_from_room(g, ch);
      char_to_room(g, ch, real_room(g, obj_get_value(obj, VAL_PORTAL_DEST)));
    }
    for (obj2 = rroom_get_contents(g, char_get_in_room(ch)); obj2; obj2 = next_obj) {
      next_obj = obj_get_next_content(obj2);
      if (obj_get_type_flag(obj2) == ITEM_HATCH) {
        hatch = obj2;
      }
    }
    obj2 = nullptr;
  }

  if (!door_mtrigger(g, ch, scmd, door))
    return;

  if (!door_wtrigger(g, ch, scmd, door))
    return;

  len = snprintf(buf, sizeof(buf), "$n %ss ", cmd_door[scmd]);
  if (!obj && ((other_room = char_get_exit(g, ch, door)->to_room) != NOWHERE)) {
    if ((back = rroom_get_dir_option(g, other_room, rev_dir[door])) != nullptr)
      if (back->to_room != char_get_in_room(ch))
        back = nullptr;
  }
  switch (scmd) {
    case SCMD_OPEN:
      if (obj) {
        if ((obj) && obj_get_type_flag(obj) == ITEM_HATCH && (vehicle)) {
          OPEN_DOOR(g, char_get_in_room(ch), vehicle, door);
          if (obj_get_vnum(g, obj) > 19199) {
            send_to_room(g, char_get_in_room(ch), "@wThe ship hatch opens slowly and settles onto the ground outside.\r\n");
            send_to_room(g, obj_get_in_room(vehicle), "@wThe ship hatch opens slowly and settles onto the ground.\r\n");
            if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_SPACE)) {
              send_to_room(g, char_get_in_room(ch), "@wA great vortex forms as air begins to get sucked out into the void!\r\n");
            }
          } else {
            act(g, "@wYou open @c$p@w.", true, ch, obj, 0, TO_CHAR);
            act(g, "@C$n@w opens @c$p@w.", true, ch, obj, 0, TO_ROOM);
            send_to_room(g, obj_get_in_room(vehicle), "@wThe door to %s@w is opened from the other side.\r\n", obj_get_short_description(vehicle));
          }
          vehicle = nullptr;
        }
        if ((obj) && obj_get_type_flag(obj) == ITEM_VEHICLE && (hatch)) {
          OPEN_DOOR(g, char_get_in_room(ch), hatch, door);
          char_from_room(g, ch);
          char_to_room(g, ch, num);
          if (obj_get_vnum(g, obj) > 19199) {
            send_to_room(g, num, "@wThe ship hatch opens slowly and settles onto the ground.\r\n");
            send_to_room(g, obj_get_in_room(hatch), "@wThe ship hatch opens slowly.\r\n");
            if (obj_get_in_room(obj) != NOWHERE && rroom_flagged(g, obj_get_in_room(obj), ROOM_SPACE)) {
              send_to_room(g, num, "@wThe air starts getting sucked out into space as the hatch opens!\r\n");
            }
          } else {
            act(g, "@wYou open @c$p@w.", true, ch, obj, 0, TO_CHAR);
            act(g, "@C$n@w opens @c$p@w.", true, ch, obj, 0, TO_ROOM);
            send_to_room(g, obj_get_in_room(hatch), "@wThe door is opened from the other side.\r\n");
          }
          hatch = nullptr;
        }
      }
      OPEN_DOOR(g, char_get_in_room(ch), obj, door);
      if (back) {
        OPEN_DOOR(g, other_room, obj, rev_dir[door]);
      }
      if (!obj) {
        send_to_char(g, ch, "You open the %s that leads %s.\r\n", char_get_exit(g, ch, door)->keyword ? char_get_exit(g, ch, door)->keyword : "door", dirs[door]);
      } else if (obj_get_type_flag(obj) != ITEM_VEHICLE && obj_get_type_flag(obj) != ITEM_HATCH) {
        send_to_char(g, ch, "You open %s.\r\n", obj_get_short_description(obj));
      }
      break;

    case SCMD_CLOSE:
      if (obj) {
        if ((obj) && obj_get_type_flag(obj) == ITEM_HATCH && (vehicle)) {
          CLOSE_DOOR(g, char_get_in_room(ch), vehicle, door);
          if (obj_get_vnum(g, obj) > 19199) {
            send_to_room(g, char_get_in_room(ch), "@wThe ship hatch slowly closes, sealing the ship from the outside.\r\n");
            send_to_room(g, obj_get_in_room(vehicle), "@wThe ship hatch slowly closes, sealing the ship.\r\n");
            if (rroom_flagged(g, obj_get_in_room(vehicle), ROOM_SPACE)) {
              send_to_room(g, char_get_in_room(ch), "@wThe air stops getting sucked out into space as the hatch seals!\r\n");
            }
          } else {
            act(g, "@wYou close @c$p@w.", true, ch, obj, 0, TO_CHAR);
            act(g, "@C$n@w closes @c$p@w.", true, ch, obj, 0, TO_ROOM);
            send_to_room(g, obj_get_in_room(vehicle), "@wThe door to %s@w is closed from the other side.\r\n", obj_get_short_description(vehicle));
          }
          vehicle = nullptr;
        }
        if ((obj) && obj_get_type_flag(obj) == ITEM_VEHICLE && (hatch)) {
          CLOSE_DOOR(g, char_get_in_room(ch), hatch, door);
          char_from_room(g, ch);
          char_to_room(g, ch, num);
          if (obj_get_vnum(g, obj) > 19199) {
            send_to_room(g, num, "@wThe ship hatch slowly closes, sealing the ship.\r\n");
            send_to_room(g, obj_get_in_room(hatch), "@wThe ship hatch slowly closes, sealing the ship from the outside.\r\n");
            if (obj_get_in_room(obj) != NOWHERE && rroom_flagged(g, obj_get_in_room(obj), ROOM_SPACE)) {
              send_to_room(g, num, "@wAir stops getting sucked out into space as the hatch seals!\r\n");
            }
          } else {
            act(g, "@wYou close @c$p@w.", true, ch, obj, 0, TO_CHAR);
            act(g, "@C$n@w closes @c$p@w.", true, ch, obj, 0, TO_ROOM);
            send_to_room(g, obj_get_in_room(hatch), "@wThe door to %s@w is closed from the other side.\r\n", obj_get_short_description(hatch));
          }
          hatch = nullptr;
        }
      }
      CLOSE_DOOR(g, char_get_in_room(ch), obj, door);
      if (back) {
        CLOSE_DOOR(g, other_room, obj, rev_dir[door]);
      }
      if (!obj) {
        send_to_char(g, ch, "You close the %s that leads %s.\r\n", char_get_exit(g, ch, door)->keyword ? char_get_exit(g, ch, door)->keyword : "door", dirs[door]);
      } else if (obj_get_type_flag(obj) != ITEM_VEHICLE && obj_get_type_flag(obj) != ITEM_HATCH) {
        send_to_char(g, ch, "You close %s.\r\n", obj_get_short_description(obj));
      }
      break;

    case SCMD_LOCK:
      if (obj) {
        if ((obj) && obj_get_type_flag(obj) == ITEM_HATCH && (vehicle)) {
          LOCK_DOOR(g, char_get_in_room(ch), vehicle, door);
          vehicle = nullptr;
        }
        if ((obj) && obj_get_type_flag(obj) == ITEM_VEHICLE && (hatch)) {
          LOCK_DOOR(g, char_get_in_room(ch), hatch, door);
          char_from_room(g, ch);
          char_to_room(g, ch, num);
          hatch = nullptr;
        }
      }
      LOCK_DOOR(g, char_get_in_room(ch), obj, door);
      if (back) {
        LOCK_DOOR(g, other_room, obj, rev_dir[door]);
      }
      if (!obj) {
        send_to_char(g, ch, "You lock the %s that leads %s.\r\n", char_get_exit(g, ch, door)->keyword ? char_get_exit(g, ch, door)->keyword : "door", dirs[door]);
      } else {
        send_to_char(g, ch, "You lock %s.\r\n", obj_get_short_description(obj));
      }
      break;

    case SCMD_UNLOCK:
      if (obj) {
        if ((obj) && obj_get_type_flag(obj) == ITEM_HATCH && (vehicle)) {
          UNLOCK_DOOR(g, char_get_in_room(ch), vehicle, door);
          vehicle = nullptr;
        }
        if ((obj) && obj_get_type_flag(obj) == ITEM_VEHICLE && (hatch)) {
          UNLOCK_DOOR(g, char_get_in_room(ch), hatch, door);
          char_from_room(g, ch);
          char_to_room(g, ch, num);
          hatch = nullptr;
        }
      }
      UNLOCK_DOOR(g, char_get_in_room(ch), obj, door);
      if (back) {
        UNLOCK_DOOR(g, other_room, obj, rev_dir[door]);
      }
      if (!obj) {
        send_to_char(g, ch, "You unlock the %s that leads %s.\r\n", char_get_exit(g, ch, door)->keyword ? char_get_exit(g, ch, door)->keyword : "door", dirs[door]);
      } else {
        send_to_char(g, ch, "You unlock %s.\r\n", obj_get_short_description(obj));
      }
      break;

    case SCMD_PICK:
      TOGGLE_LOCK(g, char_get_in_room(ch), obj, door);
      if (back)
        TOGGLE_LOCK(g, other_room, obj, rev_dir[door]);
      send_to_char(g, ch, "The lock quickly yields to your skills.\r\n");
      len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
      break;
  }

  /* Notify the room. */
  char dbuf[100];
  if (!obj) {
    snprintf(dbuf, sizeof(dbuf), "%s", dirs[door]);
  }
  if (len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "%s%s%s%s.",
             obj ? "" : "the ", obj ? "$p" : char_get_exit(g, ch, door)->keyword ? "$F"
                                                                              : "door",
             obj ? "" : " that leads ", obj ? "" : dbuf);
  if (!obj || obj_get_in_room(obj) != NOWHERE)
    act(g, buf, false, ch, obj, obj ? 0 : char_get_exit(g, ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE)) {
    send_to_room(g, char_get_exit(g, ch, door)->to_room, "The %s that leads %s is %s%s from the other side.\r\n",
                 back->keyword ? fname(back->keyword) : "door", dbuf, cmd_door[scmd],
                 scmd == SCMD_CLOSE ? "d" : "ed");
  } else if (back && (scmd == SCMD_LOCK || scmd == SCMD_UNLOCK)) {
    send_to_room(g, char_get_exit(g, ch, door)->to_room, "The %s that leads %s is %sed from the other side.\r\n",
                 back->keyword ? fname(back->keyword) : "door", dbuf, cmd_door[scmd]);
  }
  *dbuf = '\0';
}

static bool ok_pick(GameWorld *g, struct char_data *ch, obj_vnum keynum, int pickproof, int dclock, int scmd, struct obj_data *hatch)
{
  int skill_lvl, found = false;
  struct obj_data *obj, *next_obj;

  for (obj = char_get_inventory(ch); obj; obj = next_obj) {
    next_obj = obj_get_next_content(obj);
    if (obj_get_vnum(g, obj) == 18 && (!obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED))) {
      found = true;
    }
  }

  if (scmd != SCMD_PICK)
    return true;

  /* PICKING_LOCKS is not an untrained skill */
  if (!char_get_skill(ch, SKILL_OPEN_LOCK)) {
    send_to_char(g, ch, "You have no idea how!\r\n");
    return false;
  }
  if (found == false) {
    send_to_char(g, ch, "You need a lock picking kit.\r\n");
    return false;
  }
  if (hatch != nullptr && (obj_get_type_flag(hatch) == ITEM_HATCH || obj_get_type_flag(hatch) == ITEM_VEHICLE)) {
    send_to_char(g, ch, "No picking ship hatches.\r\n");
    hatch = nullptr;
    return false;
  }
  skill_lvl = roll_skill(g, ch, SKILL_OPEN_LOCK);
  if (dclock == 0) {
    dclock = rand_number(g, 1, 101);
  }

  if (keynum == NOTHING) {
    send_to_char(g, ch, "Odd - you can't seem to find a keyhole.\r\n");
  } else if (pickproof) {
    send_to_char(g, ch, "It resists your attempts to pick it.\r\n");
    act(g, "@c$n@w puts a set of lockpick tools away.@n", true, ch, 0, 0, TO_ROOM);
    /* The -2 is here because that is a penality for not having a set of
   * thieves' tools. If the player has them, that modifier will be accounted
   * for in roll_skill, and negate (or surpass) this. 
   */
  } else if ((getCurST(g, ch)) < getMaxST(g, ch) / 30) {
    send_to_char(g, ch, "You don't have the stamina to try, it takes percision to pick locks."
                     "Not shaking tired hands.\r\n");
  } else if (dclock > (skill_lvl - 2)) {
    send_to_char(g, ch, "You failed to pick the lock...\r\n");
    act(g, "@c$n@w puts a set of lockpick tools away.@n", true, ch, 0, 0, TO_ROOM);
    decCurST(g, ch, getCurST(g, ch) / 30, 0);
  } else {
    decCurST(g, ch, getCurST(g, ch) / 30, 0);
    return true;
  }

  return false;
}

#define DOOR_IS_OPENABLE(g, ch, obj, door) ((obj) ? ((obj_get_type_flag(obj) == ITEM_CONTAINER) &&   \
                                                  obj_val_flagged(obj, CONT_CLOSEABLE)) ||        \
                                                     ((obj_get_type_flag(obj) == ITEM_VEHICLE) && \
                                                      obj_val_flagged(obj, CONT_CLOSEABLE)) ||    \
                                                     ((obj_get_type_flag(obj) == ITEM_HATCH) &&   \
                                                      obj_val_flagged(obj, CONT_CLOSEABLE)) ||    \
                                                     ((obj_get_type_flag(obj) == ITEM_WINDOW) &&  \
                                                      obj_val_flagged(obj, CONT_CLOSEABLE)) ||    \
                                                     ((obj_get_type_flag(obj) == ITEM_PORTAL) &&  \
                                                      obj_val_flagged(obj, CONT_CLOSEABLE))       \
                                               : (room_exit_flagged(char_get_exit(g, ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(g, ch, obj, door)      ((obj) ? (!obj_val_flagged(obj, CONT_CLOSED)) : (!room_exit_flagged(char_get_exit(g, ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(g, ch, obj, door)  ((obj) ? (!obj_val_flagged(obj, CONT_LOCKED)) : (!room_exit_flagged(char_get_exit(g, ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(g, ch, obj, door) ((obj) ? (obj_val_flagged(obj, CONT_PICKPROOF)) : (room_exit_flagged(char_get_exit(g, ch, door), EX_PICKPROOF)))
#define DOOR_IS_SECRET(g, ch, obj, door)    ((obj) ? (obj_val_flagged(obj, CONT_SECRET)) : (room_exit_flagged(char_get_exit(g, ch, door), EX_SECRET)))

#define DOOR_IS_CLOSED(g, ch, obj, door) (!(DOOR_IS_OPEN(g, ch, obj, door)))
#define DOOR_IS_LOCKED(g, ch, obj, door) (!(DOOR_IS_UNLOCKED(g, ch, obj, door)))
#define DOOR_KEY(g, ch, obj, door)       ((obj) ? (obj_get_value(obj, VAL_KEY_KEYCODE)) : (char_get_exit(g, ch, door)->key))
#define DOOR_DCLOCK(g, ch, obj, door)    ((obj) ? (obj_get_value(obj, VAL_DOOR_DCLOCK)) : char_get_exit(g, ch, door)->dclock)

ACMD(do_gen_door)
{
  int door = -1;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj     = nullptr;
  struct char_data *victim = nullptr;

  skip_spaces(&argument);
  if (!*argument) {
    send_to_char(g, ch, "%c%s what?\r\n", toupper(*cmd_door[subcmd]), cmd_door[subcmd] + 1);
    return;
  }
  two_arguments(argument, type, dir);
  if (!generic_find(g, type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(g, ch, type, dir, cmd_door[subcmd]);

  if ((obj) && (obj_get_type_flag(obj) != ITEM_CONTAINER && obj_get_type_flag(obj) != ITEM_VEHICLE && obj_get_type_flag(obj) != ITEM_HATCH)) {
    obj  = nullptr;
    door = find_door(g, ch, type, dir, cmd_door[subcmd]);
  }

  if ((obj) || (door >= 0)) {
    keynum = DOOR_KEY(g, ch, obj, door);
    if (!DOOR_DCLOCK(g, ch, obj, door)) {
      if (obj) {
        obj_set_value(obj, VAL_DOOR_DCLOCK, 20);
      } else {
        char_get_exit(g, ch, door)->dclock = 20;
      }
    }
    if (!(DOOR_IS_OPENABLE(g, ch, obj, door)))
      act(g, "You can't $F that!", false, ch, 0, cmd_door[subcmd], TO_CHAR);
    else if (!DOOR_IS_OPEN(g, ch, obj, door) &&
             IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char(g, ch, "But it's already closed!\r\n");
    else if (!DOOR_IS_CLOSED(g, ch, obj, door) &&
             IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char(g, ch, "But it's currently open!\r\n");
    else if (!(DOOR_IS_LOCKED(g, ch, obj, door)) &&
             IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char(g, ch, "Oh.. it wasn't locked, after all..\r\n");
    else if (!(DOOR_IS_UNLOCKED(g, ch, obj, door)) &&
             IS_SET(flags_door[subcmd], NEED_UNLOCKED))
      send_to_char(g, ch, "It seems to be locked.\r\n");
    else if (!has_key(g, ch, keynum) && !char_adm_flagged(ch, ADM_NOKEYS) &&
             ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char(g, ch, "You don't seem to have the proper key.\r\n");
    else if (!obj && ok_pick(g, ch, keynum, DOOR_IS_PICKPROOF(g, ch, obj, door), DOOR_DCLOCK(g, ch, obj, door), subcmd, nullptr))
      do_doorcmd(g, ch, obj, door, subcmd);
    else if (ok_pick(g, ch, keynum, DOOR_IS_PICKPROOF(g, ch, obj, door), DOOR_DCLOCK(g, ch, obj, door), subcmd, obj) && obj)
      do_doorcmd(g, ch, obj, door, subcmd);
  }
  return;
}

/*
 * The checks entering and leaving an object share. Returns the movement points it costs, or -1
 * (after telling ch why) when ch can't go.
 */
static int portal_move_cost(GameWorld *g, struct char_data *ch, room_rnum dest_room, int need_specials_check)
{
  int need_movement;

  /* charmed? */
  if (char_aff_flagged(ch, AFF_CHARM) && char_get_master(ch) &&
      char_get_in_room(ch) == char_get_in_room(char_get_master(ch))) {
    send_to_char(g, ch, "The thought of leaving your master makes you weep.\r\n");
    act(g, "$n bursts into tears.", false, ch, 0, 0, TO_ROOM);
    return -1;
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = 1;
  if (rroom_get_gravity(g, char_get_in_room(ch)) > 10) {
    need_movement = (need_movement + rroom_get_gravity(g, char_get_in_room(ch))) * rroom_get_gravity(g, char_get_in_room(ch));
  } else if (rroom_get_gravity(g, char_get_in_room(ch)) == 10 && !char_is_bardock(ch) && !char_is_npc(ch)) {
    need_movement = (need_movement + rroom_get_gravity(g, char_get_in_room(ch))) * rroom_get_gravity(g, char_get_in_room(ch));
  }
  if (char_get_level(ch) <= 1) {
    need_movement = 0;
  }
  if ((getCurST(g, ch)) < need_movement && !char_aff_flagged(ch, AFF_FLYING) && !char_is_npc(ch)) {
    if (need_specials_check && char_get_master(ch))
      send_to_char(g, ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(g, ch, "You are too exhausted.\r\n");

    return -1;
  }
  if (rroom_flagged(g, char_get_in_room(ch), ROOM_ATRIUM)) {
    if (!House_can_enter(g, ch, rroom_get_vnum(g, dest_room))) {
      send_to_char(g, ch, "That's private property -- no trespassing!\r\n");
      return -1;
    }
  }
  if (rroom_flagged(g, dest_room, ROOM_TUNNEL) &&
      num_pc_in_room(&(g->rooms[dest_room])) >= CONFIG_TUNNEL_SIZE) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(g, ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(g, ch, "There isn't enough room there for more than one person!\r\n");
    return -1;
  }
  return need_movement;
}

static bool do_simple_enter(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_rnum dest_room = real_room(g, obj_get_value(obj, VAL_PORTAL_DEST));
  room_rnum was_in    = char_get_in_room(ch);
  int need_movement   = 0;

  if ((need_movement = portal_move_cost(g, ch, dest_room, need_specials_check)) < 0)
    return false;
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (rroom_flagged(g, dest_room, ROOM_GODROOM) &&
      char_get_admlevel(ch) < ADMLVL_GRGOD) {
    send_to_char(g, ch, "You aren't godly enough to use that room!\r\n");
    return false;
  }
  /* Now we know we're allowed to go into the room. */
  if (!(char_is_npc(ch) || char_adm_flagged(ch, ADM_WALKANYWHERE)) && !char_aff_flagged(ch, AFF_FLYING))
    decCurST(g, ch, need_movement, 0);

  act(g, "$n enters $p.", true, ch, obj, 0, TO_ROOM | TO_SNEAKRESIST);

  if (char_get_drag(ch)) {
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
  }
  if (char_get_carrying(ch)) {
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, 0, char_get_carrying(ch), TO_ROOM);
  }
  char_from_room(g, ch);
  char_to_room(g, ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(g, ch)) {
    char_from_room(g, ch);
    char_to_room(g, ch, was_in);
    return false;
  }

  if (obj_get_type_flag(obj) == ITEM_PORTAL)
    act(g, "$n arrives from $p.", false, ch, obj, 0, TO_ROOM | TO_SNEAKRESIST);
  else
    act(g, "$n arrives from outside.", false, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
  if (char_get_drag(ch)) {
    act(g, "@wYou drag @C$N@w with you.@n", true, ch, 0, char_get_drag(ch), TO_CHAR);
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
    if (!char_aff_flagged(char_get_drag(ch), AFF_KNOCKED) && !char_aff_flagged(char_get_drag(ch), AFF_SLEEP) && rand_number(g, 1, 3)) {
      send_to_char(g, char_get_drag(ch), "You feel your sleeping body being moved.\r\n");
      if (char_is_npc(char_get_drag(ch)) && !char_get_fighting(char_get_drag(ch))) {
        set_fighting(g, char_get_drag(ch), ch);
      }
    }
    char_from_room(g, char_get_drag(ch));
    char_to_room(g, char_get_drag(ch), char_get_in_room(ch));
    if (char_get_sits(char_get_drag(ch))) {
      obj_from_room(g, char_get_sits(char_get_drag(ch)));
      obj_to_room(g, char_get_sits(char_get_drag(ch)), char_get_in_room(ch));
    }
  }
  if (char_get_carrying(ch)) {
    act(g, "@wYou carry @C$N@w with you.@n", true, ch, 0, char_get_carrying(ch), TO_CHAR);
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, 0, char_get_carrying(ch), TO_ROOM);
    if (!char_aff_flagged(char_get_carrying(ch), AFF_KNOCKED) && !char_aff_flagged(char_get_carrying(ch), AFF_SLEEP) && rand_number(g, 1, 3)) {
      send_to_char(g, char_get_carrying(ch), "You feel your sleeping body being moved.\r\n");
    }
    char_from_room(g, char_get_carrying(ch));
    char_to_room(g, char_get_carrying(ch), char_get_in_room(ch));
    if (char_get_sits(char_get_carrying(ch))) {
      obj_from_room(g, char_get_sits(char_get_carrying(ch)));
      obj_to_room(g, char_get_sits(char_get_carrying(ch)), char_get_in_room(ch));
    }
  }

  if (char_get_desc(ch) != nullptr)
    look_at_room(g, char_get_in_room(ch), ch, 0);

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_DEATH) && !char_adm_flagged(ch, ADM_WALKANYWHERE)) {
    log_death_trap(g, ch);
    death_cry(g, ch);
    extract_char(g, ch);
    return false;
  }

  entry_memory_mtrigger(g, ch);
  greet_memory_mtrigger(g, ch);

  return true;
}

static int perform_enter_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_rnum was_in = char_get_in_room(ch);
  int could_move   = false;
  struct follow_type *k;

  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return (0);
  }

  if (obj_get_type_flag(obj) == ITEM_VEHICLE ||
      obj_get_type_flag(obj) == ITEM_PORTAL) {
    if (obj_val_flagged(obj, CONT_CLOSED)) {
      send_to_char(g, ch, "But it's closed!\r\n");
    } else if ((obj_get_value(obj, VAL_PORTAL_DEST) != NOWHERE) &&
               (real_room(g, obj_get_value(obj, VAL_PORTAL_DEST)) != NOWHERE)) {
      if (obj_get_value(obj, VAL_PORTAL_DEST) >= 45000 && obj_get_value(obj, VAL_PORTAL_DEST) <= 45099) {
        struct char_data *tch, *next_v;
        int filled = false;
        for (tch = rroom_get_people(g, real_room(g, obj_get_value(obj, VAL_PORTAL_DEST))); tch; tch = next_v) {
          next_v = char_get_next_in_room(tch);
          if (tch) {
            filled = true;
          }
        }
        if (filled == true) {
          send_to_char(g, ch, "Only one person can fit in there at a time.\r\n");
          return (0);
        }
      }
      if ((could_move = do_simple_enter(g, ch, obj, need_specials_check)))
        for (k = char_get_followers(ch); k; k = k->next)
          if ((char_get_in_room(k->follower) == was_in) &&
              (char_get_position(k->follower) >= POS_STANDING)) {
            act(g, "You follow $N.\r\n", false, k->follower, 0, ch, TO_CHAR);
            perform_enter_obj(g, k->follower, obj, 1);
          }
    } else {
      send_to_char(g, ch,
                   "It doesn't look like you can enter it at the moment.\r\n");
    }
  } else {
    send_to_char(g, ch, "You can't enter that!\r\n");
  }
  return could_move;
}

ACMD(do_enter)
{
  struct obj_data *obj = nullptr;
  char buf[MAX_INPUT_LENGTH];
  int door, move_dir = -1;

  one_argument(argument, buf);

  if (*buf) { /* an argument was supplied, search for door keyword */
    /* Is the object in the room? */
    obj = get_obj_in_list_vis(g, ch, buf, nullptr, rroom_get_contents(g, char_get_in_room(ch)));
    /* Is the object in the character's inventory? */
    if (!obj)
      obj = get_obj_in_list_vis(g, ch, buf, nullptr, char_get_inventory(ch));
    /* Is the character carrying the object? */
    if (!obj)
      obj = get_obj_in_equip_vis(g, ch, buf, nullptr, char_get_equipment_array(ch));
    /* We have an object to enter */
    if (obj)
      perform_enter_obj(g, ch, obj, 0);
    /* Is there a door to enter? */
    else {
      for (door = 0; door < NUM_OF_DIRS; door++)
        if (char_get_exit(g, ch, door))
          if (char_get_exit(g, ch, door)->keyword)
            if (isname(buf, char_get_exit(g, ch, door)->keyword))
              move_dir = door;
      /* Did we find what they wanted to enter. */
      if (move_dir > -1)
        perform_move(g, ch, move_dir, 1);
      else
        send_to_char(g, ch, "There is no %s here.\r\n", buf);
    }
  } else if (rroom_flagged(g, char_get_in_room(ch), ROOM_INDOORS)) {
    send_to_char(g, ch, "You are already indoors.\r\n");
  } else {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (char_get_exit(g, ch, door))
        if (char_get_exit(g, ch, door)->to_room != NOWHERE)
          if (!room_exit_flagged(char_get_exit(g, ch, door), EX_CLOSED) &&
              rroom_flagged(g, char_get_exit(g, ch, door)->to_room, ROOM_INDOORS))
            move_dir = door;
    if (move_dir > -1)
      perform_move(g, ch, move_dir, 1);
    else
      send_to_char(g, ch, "You can't seem to find anything to enter.\r\n");
  }
}

static bool do_simple_leave(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check)

{
  room_rnum was_in = char_get_in_room(ch), dest_room = NOWHERE;
  int need_movement        = 0;
  struct obj_data *vehicle = nullptr;

  if (obj_get_type_flag(obj) != ITEM_PORTAL) {
    vehicle = find_vehicle_by_vnum(g, obj_get_value(obj, VAL_HATCH_DEST));
  }

  if (vehicle == nullptr && obj_get_type_flag(obj) != ITEM_PORTAL) {
    send_to_char(g, ch, "That doesn't appear to lead anywhere.\r\n");
    return false;
  }

  if (obj_get_type_flag(obj) == ITEM_PORTAL && obj_val_flagged(obj, CONT_CLOSED)) {
    send_to_char(g, ch, "But it's closed!\r\n");
    return false;
  }

  if (vehicle != nullptr) {
    if ((dest_room = obj_get_in_room(vehicle)) == NOWHERE) {
      send_to_char(g, ch, "That doesn't appear to lead anywhere.\r\n");
      return false;
    }
  }
  if (vehicle == nullptr) {
    if ((dest_room = real_room(g, obj_get_value(obj, VAL_PORTAL_DEST))) == NOWHERE) {
      send_to_char(g, ch, "That doesn't appear to lead anywhere.\r\n");
      return false;
    }
  }

  if ((need_movement = portal_move_cost(g, ch, dest_room, need_specials_check)) < 0)
    return false;
  /* Now we know we're allowed to go into the room. */
  if (!(char_is_npc(ch) || char_adm_flagged(ch, ADM_WALKANYWHERE)) && !char_aff_flagged(ch, AFF_FLYING))
    decCurST(g, ch, need_movement, 0);

  act(g, "$n leaves $p.", true, ch, vehicle, 0, TO_ROOM | TO_SNEAKRESIST);

  if (char_get_drag(ch)) {
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
  }
  if (char_get_carrying(ch)) {
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, 0, char_get_carrying(ch), TO_ROOM);
  }
  char_from_room(g, ch);
  char_to_room(g, ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(g, ch)) {
    char_from_room(g, ch);
    char_to_room(g, ch, was_in);
    return false;
  }

  if (vehicle) {
    act(g, "$n arrives from inside $p.", true, ch, vehicle, 0, TO_ROOM | TO_SNEAKRESIST);
  } else {
    act(g, "$n arrives from inside", true, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
  }
  if (char_get_drag(ch)) {
    act(g, "@wYou drag @C$N@w with you.@n", true, ch, 0, char_get_drag(ch), TO_CHAR);
    act(g, "@C$n@w drags @c$N@w with $m.@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
    char_from_room(g, char_get_drag(ch));
    char_to_room(g, char_get_drag(ch), char_get_in_room(ch));
    if (char_get_sits(char_get_drag(ch))) {
      obj_from_room(g, char_get_sits(char_get_drag(ch)));
      obj_to_room(g, char_get_sits(char_get_drag(ch)), char_get_in_room(ch));
    }
    if (!char_aff_flagged(char_get_drag(ch), AFF_KNOCKED) && !char_aff_flagged(char_get_drag(ch), AFF_SLEEP) && rand_number(g, 1, 3)) {
      send_to_char(g, char_get_drag(ch), "You feel your sleeping body being moved.\r\n");
      if (char_is_npc(char_get_drag(ch)) && !char_get_fighting(char_get_drag(ch))) {
        set_fighting(g, char_get_drag(ch), ch);
      }
    }
  }
  if (char_get_carrying(ch)) {
    act(g, "@wYou carry @C$N@w with you.@n", true, ch, 0, char_get_carrying(ch), TO_CHAR);
    act(g, "@C$n@w carries @c$N@w with $m.@n", true, ch, 0, char_get_carrying(ch), TO_ROOM);
    char_from_room(g, char_get_carrying(ch));
    char_to_room(g, char_get_carrying(ch), char_get_in_room(ch));
    if (char_get_sits(char_get_carrying(ch))) {
      obj_from_room(g, char_get_sits(char_get_carrying(ch)));
      obj_to_room(g, char_get_sits(char_get_carrying(ch)), char_get_in_room(ch));
    }
    if (!char_aff_flagged(char_get_carrying(ch), AFF_KNOCKED) && !char_aff_flagged(char_get_carrying(ch), AFF_SLEEP) && rand_number(g, 1, 3)) {
      send_to_char(g, char_get_carrying(ch), "You feel your sleeping body being moved.\r\n");
    }
  }

  char buf3[MAX_STRING_LENGTH];
  send_to_sense(g, 0, "You sense someone ", ch);
  snprintf(buf3, sizeof(buf3), "@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection range.@n", add_commas(getCurPL(g, ch)));
  send_to_scouter(g, buf3, ch, 0, 0);

  if (char_get_desc(ch) != nullptr) {
    act(g, obj_get_action_description(obj), true, ch, obj, 0, TO_CHAR);
    look_at_room(g, char_get_in_room(ch), ch, 0);
  }

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_DEATH) && !char_adm_flagged(ch, ADM_WALKANYWHERE)) {
    log_death_trap(g, ch);
    death_cry(g, ch);
    extract_char(g, ch);
    return false;
  }

  entry_memory_mtrigger(g, ch);
  greet_memory_mtrigger(g, ch);

  return true;
}

static int perform_leave_obj(GameWorld *g, struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_rnum was_in = char_get_in_room(ch);
  int could_move   = false;
  struct follow_type *k;

  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return (0);
  }

  if (obj_val_flagged(obj, CONT_CLOSED)) {
    send_to_char(g, ch, "But the way out is closed.\r\n");
  } else {
    if (obj_get_value(obj, VAL_HATCH_DEST) != NOWHERE)
      if ((could_move = do_simple_leave(g, ch, obj, need_specials_check)))
        for (k = char_get_followers(ch); k; k = k->next)
          if ((char_get_in_room(k->follower) == was_in) &&
              (char_get_position(k->follower) >= POS_STANDING)) {
            act(g, "You follow $N.\r\n", false, k->follower, 0, ch, TO_CHAR);
            perform_leave_obj(g, k->follower, obj, 1);
          }
  }
  return could_move;
}

ACMD(do_leave)
{
  int door;
  struct obj_data *obj = nullptr;

  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }

  for (obj = rroom_get_contents(g, char_get_in_room(ch)); obj; obj = obj_get_next_content(obj))
    if (char_can_see_obj(g, ch, obj))
      if (obj_get_type_flag(obj) == ITEM_HATCH || obj_get_type_flag(obj) == ITEM_PORTAL) {
        perform_leave_obj(g, ch, obj, 0);
        return;
      }

  if (char_is_outside(g, ch))
    send_to_char(g, ch, "You are outside.. where do you want to go?\r\n");
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (char_get_exit(g, ch, door))
        if (char_get_exit(g, ch, door)->to_room != NOWHERE)
          if (!room_exit_flagged(char_get_exit(g, ch, door), EX_CLOSED) &&
              !rroom_flagged(g, char_get_exit(g, ch, door)->to_room, ROOM_INDOORS)) {
            perform_move(g, ch, door, 1);
            return;
          }
    send_to_char(g, ch, "I see no obvious exits to the outside.\r\n");
  }
}

static void handle_fall(GameWorld *g, struct char_data *ch)
{
  room_vnum room = -1;
  while (char_get_exit(g, ch, 5) && rroom_sect(g, char_get_in_room(ch)) == SECT_FLYING) {
    room = rroom_get_vnum(g, char_get_exit(g, ch, 5)->to_room);
    char_from_room(g, ch);
    char_to_room(g, ch, real_room(g, room));
    if (char_get_carrying(ch)) {
      char_from_room(g, char_get_carrying(ch));
      char_to_room(g, char_get_carrying(ch), real_room(g, room));
    }
    if (!char_get_exit(g, ch, 5) || rroom_sect(g, char_get_in_room(ch)) != SECT_FLYING) {
      act(g, "@r$n slams into the ground!@n", true, ch, 0, 0, TO_ROOM);
      decCurHealth(g, ch, getEffMaxPL(g, ch) / 20, 1);

      act(g, "@rYou slam into the ground!@n", true, ch, 0, 0, TO_CHAR);
      look_at_room(g, char_get_in_room(ch), ch, 0);
    } else {
      act(g, "@r$n pummets down toward the ground below!@n", true, ch, 0, 0, TO_ROOM);
    }
  }
  if (rroom_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM && !char_get_carried_by(ch) && !char_is_kanassan(ch)) {
    if ((getCurST(g, ch)) >= (getCurCarriedWeight(ch))) {
      act(g, "@bYou swim in place.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@b swims in place.@n", true, ch, 0, 0, TO_ROOM);
      decCurST(g, ch, getCurCarriedWeight(ch), 0);
      act(g, "@RYou are drowning!@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@C$n@b gulps water as $e struggles to stay above the water line.@n", true, ch, 0, 0, TO_ROOM);
      if (getCurPL(g, ch) - ((getEffMaxPL(g, ch)) / 3) <= 0) {
        act(g, "@rYou drown!@n", true, ch, 0, 0, TO_CHAR);
        act(g, "@R$n@r drowns!@n", true, ch, 0, 0, TO_ROOM);
        die(g, ch, nullptr);
        decCurHealthPercent(g, ch, 1, 1);
      } else {
        decCurHealthPercent(g, ch, .33, 0);
      }
    }
  }
}

static int check_swim(GameWorld *g, struct char_data *ch)
{
  bool can = false;

  if (rroom_flagged(g, char_get_in_room(ch), ROOM_SPACE)) {
    int64_t space_cost = (getMaxKI(g, ch) / 1000) + ((getCurCarriedWeight(ch)) / 2);
    if (getCurKI(g, ch) >= space_cost)
      can = true;
    decCurKI(g, ch, space_cost, 0);
    if (!can) send_to_char(g, ch, "You do not have enough ki to fly through space. You are drifting helplessly.\r\n");
    return can;
  }
  int64_t swim_cost = (getCurCarriedWeight(ch)) - 1;
  if (getCurST(g, ch) >= swim_cost)
    can = true;
  decCurST(g, ch, swim_cost, 0);
  if (!can) send_to_char(g, ch, "You are too tired to swim!\r\n");
  return can;
}

/* Takes off from the planet into the space room. */
static void fly_to_space(GameWorld *g, struct char_data *ch, room_vnum space)
{
  reveal_hiding(g, ch, 0);
  char_set_altitude(ch, 2);
  char_aff_set(ch, AFF_FLYING, true);
  if (!block_calc(g, ch)) {
    return;
  }
  char_set_altitude(ch, 0);
  char_aff_set(ch, AFF_FLYING, false);
  zone_rnum zone = 0;
  if ((zone = real_zone_by_thing(g, rroom_get_vnum(g, char_get_in_room(ch)))) != NOWHERE) {
    fly_zone(g, zone, "can be seen blasting off into space!@n\r\n", ch);
  }
  send_to_sense(g, 1, "leaving the planet", ch);
  send_to_scouter(g, "A powerlevel signal has left the planet", ch, 0, 2);
  act(g, "@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", true, ch, 0, 0, TO_CHAR);
  act(g, "@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", true, ch, 0, 0, TO_ROOM);
  char_from_room(g, ch);
  char_to_room(g, ch, real_room(g, space));
  act(g, "@C$n blasts up from the atmosphere below and then comes to a stop.@n", true, ch, 0, 0, TO_ROOM);
  send_to_char(g, ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
  if (!char_is_android(ch)) {
    decCurKI(g, ch, getMaxKI(g, ch) / 10, 0);
  }
  char_set_wait(ch, PULSE_3SEC);
}

ACMD(do_fly)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_get_absorbing(ch) || char_get_absorbby(ch)) {
    send_to_char(g, ch, "You can't fly, you are struggling with someone right now!");
    return;
  }
  if (char_get_grappling(ch) || char_get_grappled(ch)) {
    send_to_char(g, ch, "You can't fly, you are struggling with someone right now!");
    return;
  }
  if (!char_is_npc(ch)) {
    if (char_plr_flagged(ch, PLR_HEALT)) {
      send_to_char(g, ch, "You are inside a healing tank!\r\n");
      return;
    }
    if (char_plr_flagged(ch, PLR_PILOTING)) {
      send_to_char(g, ch, "You are busy piloting a ship!\r\n");
      return;
    }
  }

  if (!char_is_npc(ch) && char_get_skill(ch, SKILL_FOCUS) < 30 && !char_is_android(ch)) {
    send_to_char(g, ch, "You do not have enough focus to hold yourself aloft.\r\n");
    send_to_char(g, ch, "@wOOC@D: @WYou need the skill Focus at @m30@W.@n\r\n");
    return;
  }

  if (!*arg) {
    if (char_aff_flagged(ch, AFF_FLYING) && rroom_sect(g, char_get_in_room(ch)) != SECT_FLYING && rroom_sect(g, char_get_in_room(ch)) != SECT_SPACE) {
      act(g, "@WYou slowly settle down to the ground.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@W$n slowly settles down to the ground.@n", true, ch, 0, 0, TO_ROOM);
      char_aff_set(ch, AFF_FLYING, false);
      char_set_altitude(ch, 0);
      return;
    }

    if (char_aff_flagged(ch, AFF_FLYING) && rroom_sect(g, char_get_in_room(ch)) == SECT_FLYING) {
      act(g, "@WYou begin to plummet to the ground!@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@W$n starts to pummet to the ground below!@n", true, ch, 0, 0, TO_ROOM);
      char_aff_set(ch, AFF_FLYING, false);
      char_set_altitude(ch, 0);
      handle_fall(g, ch);
      return;
    }
    if (char_aff_flagged(ch, AFF_FLYING) && rroom_sect(g, char_get_in_room(ch)) == SECT_SPACE) {
      act(g, "@WYou let yourself drift aimlessly through space.@n", true, ch, 0, 0, TO_CHAR);
      act(g, "@W$n starts to drift slowly.!@n", true, ch, 0, 0, TO_ROOM);
      char_aff_set(ch, AFF_FLYING, false);
      char_set_altitude(ch, 0);
      return;
    }
    if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 100 && !char_is_android(ch)) {
      send_to_char(g, ch, "You do not have the ki to fly.");
      return;
    }
    reveal_hiding(g, ch, 0);
    act(g, "@WYou slowly take off into the sky.@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@W$n slowly takes off into the sky.@n", true, ch, 0, 0, TO_ROOM);
    if (char_get_sits(ch)) {
      obj_set_sitting(char_get_sits(ch), nullptr);
      char_set_sits(ch, nullptr);
    }
    if (char_get_position(ch) < POS_STANDING) {
      char_set_position(ch, POS_STANDING);
    }
    char_aff_set(ch, AFF_FLYING, true);
    char_set_altitude(ch, 1);
    decCurKI(g, ch, getMaxKI(g, ch) / 100, 0);
  }
  if (!strcasecmp("high", arg)) {
    if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 100 && !char_is_android(ch)) {
      send_to_char(g, ch, "You do not have the ki to fly.");
      return;
    }
    reveal_hiding(g, ch, 0);
    act(g, "@WYou rocket high into the sky.@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@W$n rockets high into the sky.@n", true, ch, 0, 0, TO_ROOM);
    if (char_get_sits(ch)) {
      obj_set_sitting(char_get_sits(ch), nullptr);
      char_set_sits(ch, nullptr);
    }
    if (char_get_position(ch) < POS_STANDING) {
      char_set_position(ch, POS_STANDING);
    }
    char_aff_set(ch, AFF_FLYING, true);
    char_set_altitude(ch, 2);
    decCurKI(g, ch, getMaxKI(g, ch) / 100, 0);
  }
  if (!strcasecmp("space", arg)) {
    if (!char_is_outside(g, ch)) {
      send_to_char(g, ch, "You are not outside!");
      return;
    }
    if ((getCurKI(g, ch)) < getMaxKI(g, ch) / 10 && !char_is_android(ch)) {
      send_to_char(g, ch, "You do not have the ki to fly to space.");
      return;
    }
    if (char_get_fighting(ch)) {
      send_to_char(g, ch, "You are too busy fighting!");
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_EARTH)) {
      fly_to_space(g, ch, 50);
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_CERRIA)) {
      fly_to_space(g, ch, 198);
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_VEGETA)) {
      fly_to_space(g, ch, 53);
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_KANASSA)) {
      if (rroom_get_vnum(g, char_get_in_room(ch)) == 14904) {
        fly_to_space(g, ch, 58);
      } else {
        send_to_char(g, ch, "You can only fly off the planet from the launchpad of Aquis.\r\n");
      }
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_FRIGID)) {
      fly_to_space(g, ch, 51);
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_KONACK)) {
      fly_to_space(g, ch, 52);
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_NAMEK)) {
      fly_to_space(g, ch, 54);
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_AETHER)) {
      fly_to_space(g, ch, 55);
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_YARDRAT)) {
      fly_to_space(g, ch, 56);
      return;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_ARLIA)) {
      fly_to_space(g, ch, 59);
      return;
    }
    if (rroom_planet_zenith(g, char_get_in_room(ch))) {
      fly_to_space(g, ch, 57);
      return;
    }
    send_to_char(g, ch, "You are not on a planet.\r\n");
    return;
  }
}

ACMD(do_stand)
{
  struct obj_data *chair;
  if (char_aff_flagged(ch, AFF_KNOCKED)) {
    send_to_char(g, ch, "You are knocked out cold for right now!\r\n");
    return;
  }
  if (!char_is_npc(ch) && char_get_limb_condition(ch, (3) - 1) <= 0 && char_get_limb_condition(ch, (4) - 1) <= 0) {
    send_to_char(g, ch, "With what legs will you be standing up on?\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  switch (char_get_position(ch)) {
    case POS_STANDING:
      send_to_char(g, ch, "You are already standing.\r\n");
      break;
    case POS_SITTING:
      reveal_hiding(g, ch, 0);
      send_to_char(g, ch, "You stand up.\r\n");
      act(g, "$n clambers to $s feet.", true, ch, 0, 0, TO_ROOM);
      if (char_get_sits(ch)) {
        if (obj_wear_flagged(char_get_sits(ch), ITEM_WEAR_TAKE) && obj_get_type_flag(char_get_sits(ch)) != ITEM_CHAIR && char_get_carry_weight(ch) + obj_get_weight(char_get_sits(ch)) <= getMaxCarryWeight(g, ch)) {
          obj_from_room(g, char_get_sits(ch));
          obj_to_char(g, char_get_sits(ch), ch);
          act(g, "You pick up $p.", true, ch, char_get_sits(ch), 0, TO_CHAR);
          act(g, "$n picks up $p.", true, ch, char_get_sits(ch), 0, TO_ROOM);
        }
        chair = char_get_sits(ch);
        obj_set_sitting(chair, nullptr);
        char_set_sits(ch, nullptr);
      }
      /* May be sitting for some reason and may still be fighting. */
      char_set_position(ch, char_get_fighting(ch) ? POS_FIGHTING : POS_STANDING);
      break;
    case POS_RESTING:
      send_to_char(g, ch, "You stop resting, and stand up.\r\n");
      act(g, "$n stops resting, and clambers to $s feet.", true, ch, 0, 0, TO_ROOM);
      if (char_get_sits(ch)) {
        if (obj_wear_flagged(char_get_sits(ch), ITEM_WEAR_TAKE) && char_get_carry_weight(ch) + obj_get_weight(char_get_sits(ch)) <= getMaxCarryWeight(g, ch)) {
          obj_from_room(g, char_get_sits(ch));
          obj_to_char(g, char_get_sits(ch), ch);
          act(g, "You pick up $p.", true, ch, char_get_sits(ch), 0, TO_CHAR);
          act(g, "$n picks up $p.", true, ch, char_get_sits(ch), 0, TO_ROOM);
        }
        chair = char_get_sits(ch);
        obj_set_sitting(chair, nullptr);
        char_set_sits(ch, nullptr);
      }
      char_set_position(ch, POS_STANDING);
      break;
    case POS_SLEEPING:
      send_to_char(g, ch, "You have to wake up first!\r\n");
      break;
    default:
      send_to_char(g, ch, "You stop floating around, and put your feet on the ground.\r\n");
      act(g, "$n stops floating around, and puts $s feet on the ground.",
          true, ch, 0, 0, TO_ROOM);
      char_set_position(ch, POS_STANDING);
      break;
  }
}

ACMD(do_sit)
{
  struct obj_data *chair = nullptr;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }

  if (char_get_drag(ch)) {
    act(g, "@WYou stop dragging @C$N@W!@n", true, ch, 0, char_get_drag(ch), TO_CHAR);
    act(g, "@C$n@W stops dragging @c$N@W!@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
    char_set_dragged(char_get_drag(ch), nullptr);
    char_set_drag(ch, nullptr);
  }
  if (char_get_carrying(ch)) {
    send_to_char(g, ch, "You are busy carrying someone!\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_FLYING)) {
    do_fly(g, ch, 0, 0, 0);
  }

  if (!*arg) {
    switch (char_get_position(ch)) {
      case POS_STANDING:
        reveal_hiding(g, ch, 0);
        send_to_char(g, ch, "You sit down.\r\n");
        act(g, "$n sits down.", false, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_SITTING);
        break;
      case POS_SITTING:
        send_to_char(g, ch, "You're sitting already.\r\n");
        break;
      case POS_RESTING:
        send_to_char(g, ch, "You stop resting, and sit up.\r\n");
        act(g, "$n stops resting.", true, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_SITTING);
        break;
      case POS_SLEEPING:
        send_to_char(g, ch, "You have to wake up first.\r\n");
        break;
      case POS_FIGHTING:
        send_to_char(g, ch, "Sit down while fighting? Are you MAD?\r\n");
        break;
      default:
        send_to_char(g, ch, "You stop floating around, and sit down.\r\n");
        act(g, "$n stops floating around, and sits down.", true, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_SITTING);
        break;
    }
  } else {
    if (char_get_sits(ch)) {
      send_to_char(g, ch, "You are already on something!\r\n");
      return;
    }
    if (!(chair = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "That isn't here.\r\n");
      return;
    }
    if (obj_get_vnum(g, chair) == 65) {
      send_to_char(g, ch, "You can't get on that!\r\n");
      return;
    }
    if (obj_get_sitting(chair)) {
      send_to_char(g, ch, "Someone is already on that one!\r\n");
      return;
    }
    if (obj_get_type_flag(chair) != ITEM_CHAIR && obj_get_type_flag(chair) != ITEM_BED) {
      send_to_char(g, ch, "You can't sit on that!\r\n");
      return;
    }
    if (obj_get_size(chair) + 1 < get_size(ch)) {
      send_to_char(g, ch, "You are too large for it!\r\n");
      return;
    }
    switch (char_get_position(ch)) {
      case POS_STANDING:
        reveal_hiding(g, ch, 0);
        act(g, "You sit down on $p.", false, ch, chair, 0, TO_CHAR);
        act(g, "$n sits down on $p.", false, ch, chair, 0, TO_ROOM);
        char_set_position(ch, POS_SITTING);
        char_set_sits(ch, chair);
        obj_set_sitting(chair, ch);
        break;
      case POS_SITTING:
        send_to_char(g, ch, "You should stand up first.\r\n");
        break;
      case POS_RESTING:
        send_to_char(g, ch, "You should stand up first.\r\n");
        break;
      case POS_SLEEPING:
        send_to_char(g, ch, "You have to wake up first.\r\n");
        break;
      case POS_FIGHTING:
        send_to_char(g, ch, "Sit down while fighting? Are you MAD?\r\n");
        break;
      default:
        send_to_char(g, ch, "You stop floating around, and sit down.\r\n");
        act(g, "$n stops floating around, and sits down.", true, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_SITTING);
        break;
    }
  }
}

ACMD(do_rest)
{
  struct obj_data *chair = nullptr;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (rroom_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM) {
    send_to_char(g, ch, "You can't rest here!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_SANCTUARY)) {
    if (char_get_skill(ch, SKILL_BARRIER)) {
      send_to_char(g, ch, "You have a barrier around you and can't rest.\r\n");
      return;
    }
    char_set_barrier(ch, 0);
    char_aff_set(ch, AFF_SANCTUARY, false);
  }
  if (char_get_fighting(ch)) {
    send_to_char(g, ch, "You are a bit busy at the moment!\r\n");
    return;
  }
  if (char_get_kaioken(ch) > 0) {
    send_to_char(g, ch, "You are utilizing kaioken and can't rest!\r\n");
    return;
  }

  if (char_get_drag(ch)) {
    act(g, "@WYou stop dragging @C$N@W!@n", true, ch, 0, char_get_drag(ch), TO_CHAR);
    act(g, "@C$n@W stops dragging @c$N@W!@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
    char_set_dragged(char_get_drag(ch), nullptr);
    char_set_drag(ch, nullptr);
  }

  if (char_get_carrying(ch)) {
    send_to_char(g, ch, "You are carrying someone!\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_FLYING)) {
    do_fly(g, ch, 0, 0, 0);
  }

  if (!*arg) {
    if (char_get_sits(ch)) {
      chair = char_get_sits(ch);
      if (obj_get_type_flag(chair) != ITEM_BED) {
        send_to_char(g, ch, "You can't lay on that!\r\n");
        return;
      }
    }
    switch (char_get_position(ch)) {
      case POS_STANDING:
        reveal_hiding(g, ch, 0);
        send_to_char(g, ch, "You lay down and rest your tired bones.\r\n");
        act(g, "$n lays down and rests.", true, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_RESTING);
        break;
      case POS_SITTING:
        send_to_char(g, ch, "You rest your tired bones.\r\n");
        act(g, "$n rests.", true, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_RESTING);
        break;
      case POS_RESTING:
        send_to_char(g, ch, "You are already resting.\r\n");
        break;
      case POS_SLEEPING:
        send_to_char(g, ch, "You have to wake up first.\r\n");
        break;
      case POS_FIGHTING:
        send_to_char(g, ch, "Rest while fighting?  Are you MAD?\r\n");
        break;
      default:
        send_to_char(g, ch, "You stop floating around, and stop to rest your tired bones.\r\n");
        act(g, "$n stops floating around, and rests.", false, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_RESTING);
        break;
    }
  } else {
    if (char_get_sits(ch)) {
      send_to_char(g, ch, "You are already on something!\r\n");
      return;
    }
    if (!(chair = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "That isn't here.\r\n");
      return;
    }
    if (obj_get_vnum(g, chair) == 65) {
      send_to_char(g, ch, "You can't get on that!\r\n");
      return;
    }
    if (obj_get_sitting(chair)) {
      send_to_char(g, ch, "Someone is already on that one!\r\n");
      return;
    }
    if (obj_get_type_flag(chair) != ITEM_BED) {
      send_to_char(g, ch, "You can't lay on that!\r\n");
      return;
    }
    if (obj_get_size(chair) + 1 < get_size(ch)) {
      send_to_char(g, ch, "You are too large for it!\r\n");
      return;
    }
    switch (char_get_position(ch)) {
      case POS_STANDING:
        reveal_hiding(g, ch, 0);
        act(g, "You lay down and rest on $p.", true, ch, chair, 0, TO_CHAR);
        act(g, "$n lays down and rests on $p.", true, ch, chair, 0, TO_ROOM);
        char_set_sits(ch, chair);
        obj_set_sitting(chair, ch);
        char_set_position(ch, POS_RESTING);
        break;
      case POS_SITTING:
        send_to_char(g, ch, "You should get up first.\r\n");
        break;
      case POS_RESTING:
        send_to_char(g, ch, "You are already resting.\r\n");
        break;
      case POS_SLEEPING:
        send_to_char(g, ch, "You have to wake up first.\r\n");
        break;
      case POS_FIGHTING:
        send_to_char(g, ch, "Rest while fighting?  Are you MAD?\r\n");
        break;
      default:
        send_to_char(g, ch, "You stop floating around, and stop to rest your tired bones.\r\n");
        act(g, "$n stops floating around, and rests.", false, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_RESTING);
        break;
    }
  }
}

ACMD(do_sleep)
{
  struct obj_data *chair = nullptr;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!char_is_npc(ch)) {
    if (char_prf_flagged(g, ch, PRF_ARENAWATCH)) {
      char_prf_set(g, ch, PRF_ARENAWATCH, false);
      char_set_arenawatch(ch, -1);
      send_to_char(g, ch, "You stop watching the arena action.\r\n");
    }
  }

  if (char_get_bonuses(ch, BONUS_INSOMNIAC)) {
    send_to_char(g, ch, "You don't feel the least bit tired.\r\n");
    return;
  }

  if (rroom_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM) {
    send_to_char(g, ch, "You can't rest here!\r\n");
    return;
  }

  if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (char_get_fighting(ch)) {
    send_to_char(g, ch, "You are a bit busy at the moment!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_POWERUP)) {
    send_to_char(g, ch, "You are busy powering up!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_SANCTUARY)) {
    if (char_get_skill(ch, SKILL_BARRIER) > 0) {
      send_to_char(g, ch, "You have a barrier around you and can't sleep.\r\n");
      return;
    }
    char_set_barrier(ch, 0);
    char_aff_set(ch, AFF_SANCTUARY, false);
  }
  if (char_get_kaioken(ch) > 0) {
    send_to_char(g, ch, "You are utilizing kaioken and can't sleep!\r\n");
    return;
  }
  if (char_get_sleeptime(ch) > 0) {
    send_to_char(g, ch, "You aren't sleepy enough.\r\n");
    return;
  }
  if (char_get_drag(ch)) {
    act(g, "@WYou stop dragging @C$N@W!@n", true, ch, 0, char_get_drag(ch), TO_CHAR);
    act(g, "@C$n@W stops dragging @c$N@W!@n", true, ch, 0, char_get_drag(ch), TO_ROOM);
    char_set_dragged(char_get_drag(ch), nullptr);
    char_set_drag(ch, nullptr);
  }
  if (char_get_carrying(ch)) {
    send_to_char(g, ch, "You are carrying someone!\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_FLYING)) {
    do_fly(g, ch, 0, 0, 0);
  }

  if (!*arg) {
    if (char_get_sits(ch)) {
      chair = char_get_sits(ch);
      if (obj_get_type_flag(chair) != ITEM_BED) {
        send_to_char(g, ch, "You can't sleep on %s.\r\n", obj_get_short_description(chair));
        return;
      }
    }
    switch (char_get_position(ch)) {
      case POS_STANDING:
      case POS_SITTING:
      case POS_RESTING:
        reveal_hiding(g, ch, 0);
        send_to_char(g, ch, "You go to sleep.\r\n");
        act(g, "$n lies down and falls asleep.", true, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_SLEEPING);
        /* Fury Mode Loss for halfbreeds */

        if (char_plr_flagged(ch, PLR_FURY)) {
          send_to_char(g, ch, "Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
          char_plr_set(ch, PLR_FURY, false);
        }

        /* Fury Mode Loss for halfbreeds */
        break;
      case POS_SLEEPING:
        send_to_char(g, ch, "You are already sound asleep.\r\n");
        break;
      case POS_FIGHTING:
        send_to_char(g, ch, "Sleep while fighting?  Are you MAD?\r\n");
        break;
      default:
        send_to_char(g, ch, "You stop floating around, and lie down to sleep.\r\n");
        act(g, "$n stops floating around, and lie down to sleep.",
            true, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_SLEEPING);
        break;
    }
  } else {
    if (char_get_sits(ch)) {
      send_to_char(g, ch, "You are already on something!\r\n");
      return;
    }
    if (!(chair = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "That isn't here.\r\n");
      return;
    }
    if (obj_get_vnum(g, chair) == 65) {
      send_to_char(g, ch, "You can't get on that!\r\n");
      return;
    }
    if (obj_get_sitting(chair)) {
      send_to_char(g, ch, "Someone is already on that one!\r\n");
      return;
    }
    if (obj_get_type_flag(chair) != ITEM_BED) {
      send_to_char(g, ch, "You can't sleep on that!\r\n");
      return;
    }
    if (obj_get_size(chair) + 1 < get_size(ch)) {
      send_to_char(g, ch, "You are too large for it!\r\n");
      return;
    }
    switch (char_get_position(ch)) {
      case POS_RESTING:
      case POS_SITTING:
        send_to_char(g, ch, "You need to get up first!\r\n");
        break;
      case POS_STANDING:
        reveal_hiding(g, ch, 0);
        act(g, "You lay down on $p and sleep.", false, ch, chair, 0, TO_CHAR);
        act(g, "$n lays down on $p and sleeps.", false, ch, chair, 0, TO_ROOM);
        /* Fury Mode Loss for halfbreeds */

        if (char_plr_flagged(ch, PLR_FURY)) {
          send_to_char(g, ch, "Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
          char_plr_set(ch, PLR_FURY, false);
        }

        /* Fury Mode Loss for halfbreeds */
        char_set_sits(ch, chair);
        obj_set_sitting(chair, ch);
        char_set_position(ch, POS_SLEEPING);
        break;
      case POS_SLEEPING:
        send_to_char(g, ch, "You are already sound asleep.\r\n");
        break;
      case POS_FIGHTING:
        send_to_char(g, ch, "Sleep while fighting?  Are you MAD?\r\n");
        break;
      default:
        send_to_char(g, ch, "You stop floating around, and lie down to sleep.\r\n");
        act(g, "$n stops floating around, and lie down to sleep.",
            true, ch, 0, 0, TO_ROOM);
        char_set_position(ch, POS_SLEEPING);
        break;
    }
  }
}

ACMD(do_wake)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);

  if (char_aff_flagged(ch, AFF_KNOCKED)) {
    send_to_char(g, ch, "You are knocked out cold for right now!\r\n");
    return;
  }

  if (char_get_bonuses(ch, BONUS_LATE) && char_get_position(ch) == POS_SLEEPING) {
    send_to_char(g, ch, "Nah you're enjoying sleeping too much.\r\n");
    return;
  }

  if (*arg) {
    if (char_get_position(ch) == POS_SLEEPING)
      send_to_char(g, ch, "Maybe you should wake yourself up first.\r\n");
    else if ((vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM)) == nullptr)
      send_to_char(g, ch, "%s", CONFIG_NOPERSON);
    else if (vict == ch)
      self = 1;
    else if (char_is_awake(vict))
      act(g, "$E is already awake.", false, ch, 0, vict, TO_CHAR);
    else if (char_aff_flagged(vict, AFF_SLEEP))
      act(g, "You can't wake $M up!", false, ch, 0, vict, TO_CHAR);
    else if (char_get_position(vict) < POS_SLEEPING)
      act(g, "$E's in pretty bad shape!", false, ch, 0, vict, TO_CHAR);
    else if (char_aff_flagged(vict, AFF_KNOCKED))
      send_to_char(g, ch, "They are knocked out cold for right now!\r\n");
    else if (char_get_bonuses(ch, BONUS_LATE))
      send_to_char(g, ch, "They say 'Yeah yeah...' and then roll back over.\r\n");
    else {
      act(g, "You wake $M up.", false, ch, 0, vict, TO_CHAR);
      act(g, "You are awakened by $n.", false, ch, 0, vict, TO_VICT | TO_SLEEP);
      char_set_position(vict, POS_SITTING);
      if (char_get_dragged(vict)) {
        act(g, "@WYou stop dragging @C$N@W!@n", true, char_get_dragged(vict), 0, vict, TO_CHAR);
        act(g, "@C$n@W stops dragging @c$N@W!@n", true, char_get_dragged(vict), 0, vict, TO_ROOM);
        char_set_drag(char_get_dragged(vict), nullptr);
        char_set_dragged(vict, nullptr);
      }
      if (char_get_carried_by(vict)) {
        if (char_get_alignment(char_get_carried_by(vict)) > 50) {
          carry_drop(g, char_get_carried_by(vict), 0);
        } else {
          carry_drop(g, char_get_carried_by(vict), 1);
        }
      }
    }
    if (!self)
      return;
  }
  if (char_aff_flagged(ch, AFF_SLEEP))
    send_to_char(g, ch, "You can't wake up!\r\n");
  else if (char_get_position(ch) > POS_SLEEPING)
    send_to_char(g, ch, "You are already awake...\r\n");
  else {
    send_to_char(g, ch, "You awaken, and sit up.\r\n");
    act(g, "$n awakens.", true, ch, 0, 0, TO_ROOM);
    if (char_get_dragged(ch)) {
      act(g, "@WYou stop dragging @C$N@W!@n", true, char_get_dragged(ch), 0, ch, TO_CHAR);
      act(g, "@C$n@W stops dragging you!@n", true, char_get_dragged(ch), 0, ch, TO_VICT);
      act(g, "@C$n@W stops dragging @c$N@W!@n", true, char_get_dragged(ch), 0, ch, TO_NOTVICT);
      char_set_drag(char_get_dragged(ch), nullptr);
      char_set_dragged(ch, nullptr);
    }
    if (char_get_carried_by(ch)) {
      if (char_get_alignment(char_get_carried_by(ch)) > 50) {
        carry_drop(g, char_get_carried_by(ch), 0);
      } else {
        carry_drop(g, char_get_carried_by(ch), 1);
      }
    }
    char_set_position(ch, POS_SITTING);
  }
}

ACMD(do_follow)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *leader;

  one_argument(argument, buf);

  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }

  if (*buf) {
    if (!(leader = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "%s", CONFIG_NOPERSON);
      return;
    }
  } else {
    send_to_char(g, ch, "Whom do you wish to follow?\r\n");
    return;
  }

  if (char_get_master(ch) == leader) {
    act(g, "You are already following $M.", false, ch, 0, leader, TO_CHAR);
    return;
  }
  if (char_aff_flagged(ch, AFF_CHARM) && (char_get_master(ch))) {
    act(g, "But you only feel like following $N!", false, ch, 0, char_get_master(ch), TO_CHAR);
  } else { /* Not Charmed follow person */
    if (leader == ch) {
      if (!char_get_master(ch)) {
        send_to_char(g, ch, "You are already following yourself.\r\n");
        return;
      }
      stop_follower(g, ch);
    } else {
      if (circle_follow(ch, leader)) {
        send_to_char(g, ch, "Sorry, but following in loops is not allowed.\r\n");
        return;
      }
      if (char_get_master(ch))
        stop_follower(g, ch);
      char_aff_set(ch, AFF_GROUP, false);
      reveal_hiding(g, ch, 0);
      add_follower(g, ch, leader);
    }
  }
}
