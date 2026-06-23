/*************************************************************************
 *   File: act.movement.c                                Part of CircleMUD *
 *  Usage: movement commands, door handling, & sleep/rest/etc state        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "act.movement.h"
#include "consts/affflags.h"
#include "consts/applies.h"
#include "consts/itemdata.h"
#include "consts/maximums.h"
#include "consts/mobflags.h"
#include "consts/playerflags.h"
#include "consts/positions.h"
#include "consts/pulse.h"
#include "consts/races.h"
#include "consts/search.h"
#include "consts/sectortypes.h"
#include "search.hpp"

#include <strings.h>

#include "config.h"
#include "extract.h"
#include "random.h"
#include "search.h"

#include "act.informative.h"

#include "character_api.h"
#include "character_macros.h"
#include "character_utils.h"
#include "comm.h"
#include "consts/applies.h"
#include "consts/mobflags.h"
#include "consts/positions.h"
#include "consts/races.h"
#include "dg_comm.h"
#include "fight.h"
#include "flags.h"
#include "interpreter.h"
#include "log.h"
#include "oasis.h"
#include "oasis_copy.h"
#include "object_impl.h"
#include "object_macros.h"
#include "races.h"
#include "relocate.h"
#include "room_api.h"
#include "room_db.h"
#include "room_macros.h"
#include "room_utils.h"
#include "skills.h"
#include "spells.h"
#include "util_macros.h"
#include "vehicles.h"

#include "class.h"
#include "config_db.h"
#include "consts/admlevel.h"
#include "consts/exitflags.h"
#include "dg_scripts.h"
#include "guild.h"
#include "house.h"
#include "local_limits.h"
#include "races_plus.h"
#include "stringutils.h"
#include "zone_api.h"
#include "zone_impl.h"

#include "iterate.hpp"

/* local functions */
void handle_fall(struct char_data *ch);
static int check_swim(struct char_data *ch);
void disp_locations(struct char_data *ch);
static int has_boat(struct char_data *ch);
static int find_door(struct char_data *ch, const char *type, char *dir,
                     const char *cmdname);
static int has_key(struct char_data *ch, obj_vnum key);
static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door,
                       int scmd);
static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof,
                   int dclock, int scmd, struct obj_data *obj);
static int has_flight(struct char_data *ch);
int do_simple_enter(struct char_data *ch, struct obj_data *obj,
                    int need_specials_check);
static int perform_enter_obj(struct char_data *ch, struct obj_data *obj,
                             int need_specials_check);
int do_simple_leave(struct char_data *ch, struct obj_data *obj,
                    int need_specials_check);
static int perform_leave_obj(struct char_data *ch, struct obj_data *obj,
                             int need_specials_check);

/* This handles teleporting players with instant transmission or skills like it.
 */
void handle_teleport(struct char_data *ch, struct char_data *tar,
                     int location) {
  int success = FALSE;

  if (location != 0) { /* Teleport to a particular room */
    char_from_room(ch);
    char_to_room(ch, room_by_id(location));
    success = TRUE;
  } else if (tar != NULL) { /* Teleport to a particular character */
    char_from_room(ch);
    char_to_room(ch, char_room_get(tar));
    success = TRUE;
  }

  if (success == TRUE) { /* We have made it. */
    act("@w$n@w appears in an instant out of nowhere!@n", TRUE, ch, 0, 0,
        TO_ROOM);
    if (DRAGGING(ch) && !IS_NPC(DRAGGING(ch))) {
      char_from_room(DRAGGING(ch));
      char_to_room(DRAGGING(ch), char_room_get(ch));
      act("@w$n@w appears in an instant out of nowhere being dragged by $N!@n",
          TRUE, DRAGGING(ch), 0, ch, TO_NOTVICT);
    }
    if (GRAPPLING(ch) && !IS_NPC(GRAPPLING(ch))) {
      char_from_room(GRAPPLING(ch));
      char_to_room(GRAPPLING(ch), char_room_get(ch));
      act("@w$n@w appears in an instant out of nowhere being grappled by $N!@n",
          TRUE, GRAPPLING(ch), 0, ch, TO_NOTVICT);
    }
    if (CARRYING(ch)) {
      char_from_room(CARRYING(ch));
      char_to_room(CARRYING(ch), char_room_get(ch));
      act("@w$n@w appears in an instant out of nowhere being carried by $N!@n",
          TRUE, CARRYING(ch), 0, ch, TO_NOTVICT);
    }
    if (GRAPPLED(ch) && !IS_NPC(GRAPPLED(ch))) {
      char_from_room(GRAPPLED(ch));
      char_to_room(GRAPPLED(ch), char_room_get(ch));
      act("@w$n@w appears in an instant out of nowhere being grappled by $N!@n",
          TRUE, GRAPPLED(ch), 0, ch, TO_NOTVICT);
    }
    if (DRAGGING(ch) && IS_NPC(DRAGGING(ch))) {
      act("@WYou stop dragging @C$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
      act("@C$n@W stops dragging @c$N@W!@n", TRUE, ch, 0, DRAGGING(ch),
          TO_ROOM);
      char_being_dragged_set(DRAGGING(ch), NULL);
      char_dragging_set(ch, NULL);
    }
    if (GRAPPLING(ch) && IS_NPC(GRAPPLING(ch))) {
      struct char_data *other = GRAPPLING(ch);
      char_grappling_set(ch, NULL, 0);
      char_grappled_set(other, NULL, 0);
    }
    if (GRAPPLED(ch) && IS_NPC(GRAPPLED(ch))) {
      struct char_data *other = GRAPPLED(ch);
      char_grappling_set(other, NULL, 0);
      char_grappled_set(ch, NULL, 0);
    }
  } else { /* Wut... */
    mud_log("ERROR: handle_teleport called without a destination.");
    return;
  }
}

/* Let's carry someone! Why not? - Iovan */
ACMD(do_carry) {

  if (IS_NPC(ch))
    return;

  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH];

  if (DRAGGING(ch)) {
    send_to_char(ch, "You are busy dragging someone at the moment.\r\n");
    return;
  }

  if (PLR_FLAGGED(ch, PLR_PILOTING)) {
    send_to_char(ch, "You are busy piloting a ship!\r\n");
    return;
  }

  if (CARRYING(ch)) { /* Already carrying someone. Put them down. Simple and
                         clean. */
    if (GET_ALIGNMENT(ch) > 50) {
      carry_drop(ch, 0);
    } else {
      carry_drop(ch, 1);
    }
    return;
  } else { /* So not carrying already. */
    one_argument(argument, arg);

    if (!*arg) {
      send_to_char(ch, "You want to carry who?\r\n");
      return;
    }

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "That person isn't here.\r\n");
      return;
    }

    if (IS_NPC(vict)) {
      send_to_char(ch, "There's no point in carrying them.\r\n");
      return;
    }

    if (CARRIED_BY(vict) != NULL) {
      send_to_char(ch, "Someone is already carrying them!\r\n");
      return;
    }

    if (GET_POS(vict) > POS_SLEEPING) {
      send_to_char(ch, "They are not unconcious.\r\n");
      return;
    }

    if (GET_PC_WEIGHT(vict) + IS_CARRYING_W(vict) > CAN_CARRY_W(ch)) {
      act("@WYou try to pick up @C$N@W but have to put them down. They are too "
          "heavy for you at the moment.@n",
          TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W tries to pick up @c$N@W. After struggling for a moment $e "
          "has to put $M down.@n",
          TRUE, ch, 0, vict, TO_NOTVICT);
      WAIT_STATE(ch, PULSE_1SEC);
      return;
    } else { /* Let's carry that mofo! */
      act("@WYou pick up @C$N@W and put $M over your shoulder.@n", TRUE, ch, 0,
          vict, TO_CHAR);
      act("@C$n@W picks up $c$N@W and puts $M over $s shoulder.@n", TRUE, ch, 0,
          vict, TO_NOTVICT);
      if (SITS(vict)) {
        struct obj_data *chair = SITS(vict);
        SITTING(chair) = NULL;
        SITS(vict) = NULL;
      }
      char_carrying_char_set(ch, vict);
      char_carried_by_char_set(vict, ch);
      WAIT_STATE(ch, PULSE_1SEC);
      return;
    }

  } /* End new carry target. */
}

/* Handles dropping someone you are carrying. */
void carry_drop(struct char_data *ch, int type) {

  struct char_data *vict = NULL;

  vict = CARRYING(ch);

  switch (type) {
  case 0: /* Awww we were gentle >.> */
    act("@WYou gently set @C$N@W down on the ground.@n", TRUE, ch, 0, vict,
        TO_CHAR);
    act("@C$n @Wgently sets you down on the ground.@n", TRUE, ch, 0, vict,
        TO_VICT);
    act("@C$n @Wgently sets @c$N@W down on the ground.@n", TRUE, ch, 0, vict,
        TO_NOTVICT);
    break;
  case 1: /* We're not super nice. */
    act("@WYou set @C$N@W hastily onto the ground.@n", TRUE, ch, 0, vict,
        TO_CHAR);
    act("@C$n @Wsets you hastily onto the ground.@n", TRUE, ch, 0, vict,
        TO_VICT);
    act("@C$n @Wsets @c$N@W hastily onto the ground.@n", TRUE, ch, 0, vict,
        TO_NOTVICT);
    break;
  case 2: /* Uh oh we dropped them from being hit! */
    act("@WYou have @C$N@W knocked out of your arms and onto the ground!@n",
        TRUE, ch, 0, vict, TO_CHAR);
    act("@WYou are knocked out of @C$n's@W arms and onto the ground!@n", TRUE,
        ch, 0, vict, TO_VICT);
    act("@C$n @Whas @c$N@W knocked out of $s arms and onto the ground!@n", TRUE,
        ch, 0, vict, TO_NOTVICT);
    break;
  case 3: /* Uh oh they are being extracted! */
    act("@WYou stop carrying @C$N@W for some reason.@n", TRUE, ch, 0, vict,
        TO_CHAR);
    act("@C$n @Wstops carrying you for some reason.@n", TRUE, ch, 0, vict,
        TO_VICT);
    act("@C$n @Wstops carrying @c$N@W for some reason.@n", TRUE, ch, 0, vict,
        TO_NOTVICT);
    break;
  }
  char_carrying_char_set(ch, NULL);
  char_carried_by_char_set(vict, NULL);
}

int land_location(struct char_data *ch, char *arg) {

  if (char_room_vnum_get(ch) == 50) { // Above Earth
    if (!strcasecmp(arg, "Nexus City")) {
      return (300);
    } else if (!strcasecmp(arg, "South Ocean")) {
      return (800);
    } else if (!strcasecmp(arg, "Nexus Field")) {
      return (1150);
    } else if (!strcasecmp(arg, "Cherry Blossom Mountain")) {
      return (1180);
    } else if (!strcasecmp(arg, "Sandy Desert")) {
      return (1287);
    } else if (!strcasecmp(arg, "Northern Plains")) {
      return (1428);
    } else if (!strcasecmp(arg, "Korin's Tower")) {
      return (1456);
    } else if (!strcasecmp(arg, "Kami's Lookout")) {
      return (1506);
    } else if (!strcasecmp(arg, "Shadow Forest")) {
      return (1636);
    } else if (!strcasecmp(arg, "Decrepit Area")) {
      return (1710);
    } else if (!strcasecmp(arg, "West City")) {
      return (19510);
    } else if (!strcasecmp(arg, "Hercule Beach")) {
      return (2141);
    } else if (!strcasecmp(arg, "Satan City")) {
      return (13020);
    } else {
      send_to_char(ch, "You don't know where that made up place is, but "
                       "decided to land anyway.");
      return (300);
    }
  } else if (char_room_vnum_get(ch) == 51) { // Above Frigid
    if (!strcasecmp(arg, "Ice Crown City")) {
      return (4264);
    } else if (!strcasecmp(arg, "Ice Highway")) {
      return (4300);
    } else if (!strcasecmp(arg, "Topica Snowfield")) {
      return (4351);
    } else if (!strcasecmp(arg, "Glug's Volcano")) {
      return (4400);
    } else if (!strcasecmp(arg, "Platonic Sea")) {
      return (4600);
    } else if (!strcasecmp(arg, "Slave City")) {
      return (4800);
    } else if (!strcasecmp(arg, "Acturian Woods")) {
      return (5100);
    } else if (!strcasecmp(arg, "Desolate Demesne")) {
      return (5150);
    } else if (!strcasecmp(arg, "Chateau Ishran")) {
      return (5165);
    } else if (!strcasecmp(arg, "Wyrm Spine Mountain")) {
      return (5200);
    } else if (!strcasecmp(arg, "Cloud Ruler Temple")) {
      return (5500);
    } else if (!strcasecmp(arg, "Koltoan Mine")) {
      return (4944);
    } else {
      send_to_char(ch, "You don't know where that made up place is, but "
                       "decided to land anyway.");
      return (4264);
    }
  } else if (char_room_vnum_get(ch) == 52) { // Above Konack
    if (!strcasecmp(arg, "Tiranoc City")) {
      return (8006);
    } else if (!strcasecmp(arg, "Great Oroist Temple")) {
      return (8300);
    } else if (!strcasecmp(arg, "Elzthuan Forest")) {
      return (8400);
    } else if (!strcasecmp(arg, "Mazori Farm")) {
      return (8447);
    } else if (!strcasecmp(arg, "Dres")) {
      return (8500);
    } else if (!strcasecmp(arg, "Colvian Farm")) {
      return (8600);
    } else if (!strcasecmp(arg, "St Alucia")) {
      return (8700);
    } else if (!strcasecmp(arg, "Meridius Memorial")) {
      return (8800);
    } else if (!strcasecmp(arg, "Desert of Illusion")) {
      return (8900);
    } else if (!strcasecmp(arg, "Plains of Confusion")) {
      return (8954);
    } else if (!strcasecmp(arg, "Turlon Fair")) {
      return (9200);
    } else if (!strcasecmp(arg, "Wetlands")) {
      return (9700);
    } else if (!strcasecmp(arg, "Kerberos")) {
      return (9855);
    } else if (!strcasecmp(arg, "Shaeras Mansion")) {
      return (9864);
    } else if (!strcasecmp(arg, "Slavinus Ravine")) {
      return (9900);
    } else if (!strcasecmp(arg, "Furian Citadel")) {
      return (9949);
    } else {
      send_to_char(ch, "you don't know where that made up place is, but "
                       "decided to land anyway.");
      return (8006);
    }
  } else if (char_room_vnum_get(ch) == 53) { // Above Vegeta
    if (!strcasecmp(arg, "Vegetos City")) {
      return (2226);
    } else if (!strcasecmp(arg, "Blood Dunes")) {
      return (2600);
    } else if (!strcasecmp(arg, "Ancestral Mountains")) {
      return (2616);
    } else if (!strcasecmp(arg, "Destopa Swamp")) {
      return (2709);
    } else if (!strcasecmp(arg, "Pride forest")) {
      return (2800);
    } else if (!strcasecmp(arg, "Pride Tower")) {
      return (2899);
    } else if (!strcasecmp(arg, "Ruby Cave")) {
      return (2615);
    } else {
      send_to_char(ch, "you don't know where that made up place is, but "
                       "decided to land anyway.");
      return (2226);
    }
  } else if (char_room_vnum_get(ch) == 54) { // Above Namek
    if (!strcasecmp(arg, "Senzu Village")) {
      return (11600);
    } else if (!strcasecmp(arg, "Guru's House")) {
      return (10182);
    } else if (!strcasecmp(arg, "Crystalline Cave")) {
      return (10474);
    } else if (!strcasecmp(arg, "Elder Village")) {
      return (13300);
    } else if (!strcasecmp(arg, "Frieza's Ship")) {
      return (10203);
    } else if (!strcasecmp(arg, "Kakureta Village")) {
      return (10922);
    } else {
      send_to_char(ch, "you don't know where that made up place is, but "
                       "decided to land anyway.");
      return (11600);
    }
  } else if (char_room_vnum_get(ch) == 55) { // Above Aether
    if (!strcasecmp(arg, "Haven City")) {
      return (12010);
    } else if (!strcasecmp(arg, "Serenity Lake")) {
      return (12103);
    } else if (!strcasecmp(arg, "Kaiju Forest")) {
      return (12300);
    } else if (!strcasecmp(arg, "Ortusian Temple")) {
      return (12400);
    } else if (!strcasecmp(arg, "Silent Glade")) {
      return (12480);
    } else {
      send_to_char(ch, "you don't know where that made up place is, but "
                       "decided to land anyway.");
      return (12010);
    }
  } else if (char_room_vnum_get(ch) == 56) { // Above Yardrat
    if (!strcasecmp(arg, "Yardra City")) {
      return (14008);
    } else if (!strcasecmp(arg, "Jade Forest")) {
      return (14100);
    } else if (!strcasecmp(arg, "Jade Cliffs")) {
      return (14200);
    } else if (!strcasecmp(arg, "Mount Valaria")) {
      return (14300);
    } else {
      send_to_char(ch, "you don't know where that made up place is, but "
                       "decided to land anyway.");
      return (14008);
    }
  } else if (char_room_vnum_get(ch) == 198) { // Above Cerria
    if (!strcasecmp(arg, "Cerria Colony")) {
      return (17531);
    } else if (!strcasecmp(arg, "Crystalline Forest")) {
      return (7950);
    } else if (!strcasecmp(arg, "Fistarl Volcano")) {
      return (17420);
    } else {
      send_to_char(ch, "you don't know where that made up place is, but "
                       "decided to land anyway.");
      return (17531);
    }
  } else if (char_room_vnum_get(ch) == 57) { // Above Zennith
    if (!strcasecmp(arg, "Utatlan City")) {
      return (3412);
    } else if (!strcasecmp(arg, "Zenith Jungle")) {
      return (3520);
    } else if (!strcasecmp(arg, "Ancient Castle")) {
      return (19600);
    } else {
      send_to_char(ch, "you don't know where that made up place is, but "
                       "decided to land anyway.");
      return (3412);
    }
  } else if (char_room_vnum_get(ch) == 58) { // Above Kanassa
    if (!strcasecmp(arg, "Aquis City")) {
      return (14904);
    } else if (!strcasecmp(arg, "Yunkai Pirate Base")) {
      return (15655);
    } else {
      send_to_char(ch, "you don't know where that made up place is, but "
                       "decided to land anyway.");
      return (14904);
    }
  } else if (char_room_vnum_get(ch) == 59) { // Above Arlia
    if (!strcasecmp(arg, "Janacre")) {
      return (16009);
    } else if (!strcasecmp(arg, "Arlian Wasteland")) {
      return (16544);
    } else if (!strcasecmp(arg, "Arlia Mine")) {
      return (16600);
    } else {
      send_to_char(ch, "you don't know where that made up place is, but "
                       "decided to land anyway.");
      return (16009);
    }
  } else {
    send_to_char(ch, "You are not above a planet!\r\n");
    return (-1);
  }
}

/* This shows the player what locations the planet has to land at. */
void disp_locations(struct char_data *ch) {
  if (char_room_vnum_get(ch) == 50) { // Above Earth
    send_to_char(ch, "@D------------------[ @GEarth@D ]------------------@c\n");
    send_to_char(
        ch, "Nexus City, South Ocean, Nexus field, Cherry Blossom Mountain,\n");
    send_to_char(
        ch, "Sandy Desert, Northern Plains, Korin's Tower, Kami's Lookout,\n");
    send_to_char(ch, "Shadow Forest, Decrepit Area, West City, Hercule Beach, "
                     "Satan City.\n");
    send_to_char(ch, "@D---------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 51) { // Above Frigid
    send_to_char(ch,
                 "@D------------------[ @CFrigid@D ]------------------@c\n");
    send_to_char(
        ch, "Ice Crown City, Ice Highway, Topica Snowfield, Glug's Volcano,\n");
    send_to_char(
        ch, "Platonic Sea, Slave City, Acturian Woods, Desolate Demesne,\n");
    send_to_char(ch, "Chateau Ishran, Wyrm Spine Mountain, Cloud Ruler Temple, "
                     "Koltoan mine.\n");
    send_to_char(ch, "@D---------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 52) { // Above Konack
    send_to_char(ch,
                 "@D------------------[ @MKonack@D ]------------------@c\n");
    send_to_char(ch,
                 "Great Oroist Temple, Elzthuan Forest, Mazori Farm, Dres,\n");
    send_to_char(
        ch,
        "Colvian Farm, St Alucia, Meridius Memorial, Desert of Illusion,\n");
    send_to_char(ch, "Plains of Confusion, Turlon Fair, Wetlands, Kerberos,\n");
    send_to_char(ch, "Shaeras Mansion, Slavinus Ravine, Furian Citadel.\n");
    send_to_char(ch, "@D---------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 53) { // Above Vegeta
    send_to_char(ch,
                 "@D------------------[ @YVegeta@D ]------------------@c\n");
    send_to_char(
        ch, "Vegetos City, Blood Dunes, Ancestral Mountains, Destopa Swamp,\n");
    send_to_char(ch, "Pride Forest, Pride tower, Ruby Cave.\n");
    send_to_char(ch, "@D---------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 198) { // Above Cerria
    send_to_char(ch,
                 "@D------------------[ @MCerria@D ]------------------@c\n");
    send_to_char(ch, "Cerria Colony, Fistarl Volcano, Crystalline Forest.\n");
    send_to_char(ch, "@D---------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 54) { // Above Namek
    send_to_char(ch, "@D------------------[ @gNamek@D ]------------------@c\n");
    send_to_char(
        ch, "Senzu Village, Guru's House, Crystalline Cave, Elder Village,\n");
    send_to_char(ch, "Frieza's Ship, Kakureta Village.\n");
    send_to_char(ch, "@D---------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 55) { // Above Aether
    send_to_char(ch, "@D------------------[ @BAether@D ]-----------------@c\n");
    send_to_char(ch,
                 "Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n");
    send_to_char(ch, "Silent Glade.\n");
    send_to_char(ch, "@D--------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 56) { // Above Yardrat
    send_to_char(ch, "@D-----------------[ @mYardrat@D ]-----------------@c\n");
    send_to_char(ch, "Yardra City, Jade Forest, Jade Cliffs, Mount Valaria.\n");
    send_to_char(ch, "@D-------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 57) { // Above Zennith
    send_to_char(ch, "@D-----------------[ @CZennith@D ]-----------------@c\n");
    send_to_char(ch, "Utatlan City, Zenith Jungle, Ancient Castle.\n");
    send_to_char(ch, "@D-------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 58) { // Above Kanassa
    send_to_char(ch, "@D-----------------[ @CKanassa@D ]-----------------@c\n");
    send_to_char(ch, "Aquis City, Yunkai Pirate Base.\n");
    send_to_char(ch, "@D-------------------------------------------@n\n");
  } else if (char_room_vnum_get(ch) == 59) { // Above Arlia
    send_to_char(ch, "@D------------------[ @MArlia@D ]------------------@c\n");
    send_to_char(ch, "Janacre, Arlian Wasteland, Arlia Mine.\n");
    send_to_char(ch, "@D---------------------------------------------@n\n");
  } else {
    send_to_char(ch, "You are not above a planet!\r\n");
  }
}

ACMD(do_land) {}

/* simple function to determine if char can walk on water */
static int has_boat(struct char_data *ch) {
  struct obj_data *obj;

  if (ADM_FLAGGED(ch, ADM_WALKANYWHERE) || GET_ADMLEVEL(ch) > 4)
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERWALK))
    return (1);

  if ((obj = dbat::game::search::character_inventory_find(
           ch, FALSE, [&](auto it) {
             return GET_OBJ_TYPE(it) == ITEM_BOAT &&
                    (find_eq_pos(ch, it, NULL) < 0);
           })) != NULL)
    return (1);

  /* and any boat you're wearing will do it too */
  {
    bool found_boat = false;
    char_equipment_iterate(ch, [&](auto i, auto eq) {
      if (GET_OBJ_TYPE(eq) == ITEM_BOAT) {
        found_boat = true;
        return false;
      }
      return true;
    });
    if (found_boat) return (1);
  }

  return (0);
}

/* simple function to determine if char can fly */
static int has_flight(struct char_data *ch) {
  if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
    return (1);

  if (char_condition_has(ch, "flying") &&
      (getCurKI(ch)) >=
          (GET_LEVEL(ch) + (GET_MAX_MANA(ch) / (GET_LEVEL(ch) * 30))) &&
      !IS_ANDROID(ch) && !IS_NPC(ch)) {
    return (1);
  }
  if (char_condition_has(ch, "flying") &&
      (getCurKI(ch)) <
          (GET_LEVEL(ch) + (GET_MAX_MANA(ch) / (GET_LEVEL(ch) * 30))) &&
      !IS_ANDROID(ch) && !IS_NPC(ch)) {
    act("@WYou crash to the ground, too tired to fly anymore!@n", TRUE, ch, 0,
        0, TO_CHAR);
    act("@W$n@W crashes to the ground!@n", TRUE, ch, 0, 0, TO_ROOM);
    char_condition_remove(ch, "flying", "stop_flying");
    handle_fall(ch);
    return (0);
  }
  if (char_condition_has(ch, "flying") && IS_ANDROID(ch)) {
    return (1);
  }
  if (char_condition_has(ch, "flying") && IS_NPC(ch)) {
    return (1);
  }

  /* non-wearable flying items in inventory will do it */
  {
    bool found = false;
    char_inventory_iterate(ch, [&](auto obj) {
      if (obj_aff_flagged(obj, AFF_FLYING) && (find_eq_pos(ch, obj, NULL) < 0)) {
        found = true;
        return false;
      }
      return true;
    });
    if (found) return (1);
  }

  /* anything worn as wings will do */
  return (0);
}

/* simple function to determine if char can breathe non-o2 */
int has_o2(struct char_data *ch) {
  if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
    return (1);

  if (char_condition_has(ch, "rune_laguz"))
    return (1);

  if (IS_KANASSAN(ch) || IS_ANDROID(ch) || IS_ICER(ch) || IS_MAJIN(ch))
    return (1);

  return (0);
}

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check) {
  char throwaway[MAX_INPUT_LENGTH] = ""; /* Functions assume writable. */
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  struct room_data *was_in_room = char_room_get(ch);
  int need_movement;
  struct room_data *rm;

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, throwaway))
    return (0);

  /* blocked by a leave trigger ? */
  if (!leave_mtrigger(ch, dir) ||
      char_room_get(ch) != was_in_room) /* prevent teleport crashes */
    return 0;
  if (!leave_wtrigger(char_room_get(ch), ch, dir) ||
      char_room_get(ch) != was_in_room) /* prevent teleport crashes */
    return 0;
  if (!leave_otrigger(char_room_get(ch), ch, dir) ||
      char_room_get(ch) != was_in_room) /* prevent teleport crashes */
    return 0;
  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && MASTER(ch) &&
      char_room_get(ch) == char_room_get(MASTER(ch))) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  int willfall = FALSE;
  int sect = room_sector_type_get(char_room_get(ch));
  /* if this room or the one we're going to needs flight, check for it */
  if ((sect == SECT_FLYING) ||
      (room_sector_type_get(exit_dest_get(EXIT(ch, dir))) == SECT_FLYING)) {
    if (!has_flight(ch)) {
      if (dir != 4) {
        willfall = TRUE;
      } else {
        send_to_char(ch, "You need to fly to go there!\r\n");
        return (0);
      }
    }
  }

  if (((sect == SECT_WATER_NOSWIM) || (room_sector_type_get(exit_dest_get(EXIT(
                                           ch, dir))) == SECT_WATER_NOSWIM)) &&
      IS_HUMANOID(ch)) {
    if (IS_KANASSAN(ch) && !has_flight(ch)) {
      act("@CYou swim swiftly.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C swims swiftly.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (IS_ICER(ch) && !has_flight(ch)) {
      act("@CYou swim swiftly.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C swims swiftly.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (!IS_KANASSAN(ch) && !IS_ICER(ch) && !has_flight(ch)) {
      if (!check_swim(ch)) {
        return (0);
      } else {
        act("@CYou swim through the cold water.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@c$n@C swim through the cold water.@n", TRUE, ch, 0, 0, TO_ROOM);
        WAIT_STATE(ch, PULSE_1SEC);
      }
    }
  }

  if (room_flagged(char_room_get(ch), ROOM_SPACE)) {
    if (!IS_ANDROID(ch)) {
      if (!check_swim(ch)) {
        return (0);
      }
    }
  }

  if (room_geffect_get(exit_dest_get(EXIT(ch, dir))) == 6 && !IS_HUMANOID(ch) &&
      IS_NPC(ch)) {
    return (0);
  }

  if (IS_NPC(ch) && room_flagged(exit_dest_get(EXIT(ch, dir)), ROOM_NOMOB) &&
      !MASTER(ch)) {
    return (0);
  }

  if (room_is_sunken(char_room_get(ch)) ||
      room_is_sunken(exit_dest_get(EXIT(ch, dir)))) {
    if (!has_o2(ch) && ((group_bonus(ch, 2) != 10 &&
                         (getCurKI(ch)) < GET_MAX_MANA(ch) / 200) ||
                        (group_bonus(ch, 2) == 10 &&
                         (getCurKI(ch)) < GET_MAX_MANA(ch) / 800))) {
      if (GET_HIT(ch) >= GET_MAX_HIT(ch) / 20) {
        send_to_char(ch, "@RYou struggle to breath!@n\r\n");
        decCurHealth(ch, getMaxPL(ch) / 20);
      }
      if (GET_HIT(ch) < GET_MAX_HIT(ch) / 20) {
        send_to_char(ch, "@rYou drown!@n\r\n");
        die(ch, NULL);
        return (0);
      }
    }
    if (!has_o2(ch) && ((group_bonus(ch, 2) != 10 &&
                         (getCurKI(ch)) >= GET_MAX_MANA(ch) / 200) ||
                        (group_bonus(ch, 2) == 10 &&
                         (getCurKI(ch)) >= GET_MAX_MANA(ch) / 800))) {
      send_to_char(ch, "@CYou hold your breath!@n\r\n");
      if (group_bonus(ch, 2) == 10) {
        decCurKI(ch, getMaxKI(ch) / 800);
      } else {
        decCurKI(ch, getMaxKI(ch) / 200);
      }
    }
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = 1;
  int gravity = room_gravity_get(char_room_get(ch));
  if (gravity > 10) {
    need_movement = (need_movement + gravity) * gravity;
  } else if (gravity == 10 && !IS_BARDOCK(ch) && !IS_NPC(ch)) {
    need_movement = (need_movement + gravity) * gravity;
  }
  if (GET_LEVEL(ch) <= 1) {
    need_movement = 0;
  }
  /* Stealth increases your move cost, less if you are good at it */
  if (AFF_FLAGGED(ch, AFF_HIDE))
    need_movement *= ((roll_skill(ch, SKILL_HIDE) > 15) ? 2 : 4);

  if (AFF_FLAGGED(ch, AFF_SNEAK))
    need_movement *= ((roll_skill(ch, SKILL_MOVE_SILENTLY) > 15) ? 1.2 : 2);

  int flight_cost = 0;

  if (char_condition_has(ch, "flying") && !IS_ANDROID(ch)) {
    if (!GET_SKILL(ch, SKILL_CONCENTRATION) && !GET_SKILL(ch, SKILL_FOCUS)) {
      flight_cost = GET_MAX_MANA(ch) / 100;
    } else if (GET_SKILL(ch, SKILL_CONCENTRATION) &&
               !GET_SKILL(ch, SKILL_FOCUS)) {
      flight_cost = GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_CONCENTRATION) * 2);
    } else if (!GET_SKILL(ch, SKILL_CONCENTRATION) &&
               GET_SKILL(ch, SKILL_FOCUS)) {
      flight_cost = GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_FOCUS) * 3);
    } else {
      flight_cost =
          GET_MAX_MANA(ch) / ((GET_SKILL(ch, SKILL_CONCENTRATION) * 2) +
                              (GET_SKILL(ch, SKILL_FOCUS) * 3));
    }
  }

  if (char_condition_has(ch, "flying") && ((getCurKI(ch)) < flight_cost) &&
      !IS_ANDROID(ch)) {
    decCurKI(ch, flight_cost);
    act("@WYou crash to the ground, too tired to fly anymore!@n", TRUE, ch, 0,
        0, TO_CHAR);
    act("@W$n@W crashes to the ground!@n", TRUE, ch, 0, 0, TO_ROOM);
    char_condition_remove(ch, "flying", "stop_flying");
  } else if (char_condition_has(ch, "flying") && !IS_ANDROID(ch)) {
    decCurKI(ch, flight_cost);
  }

  if ((getCurST(ch)) < need_movement && !char_condition_has(ch, "flying") &&
      !IS_NPC(ch)) {
    if (need_specials_check && MASTER(ch)) {
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    } else {
      send_to_char(ch, "You are too exhausted.\r\n");
    }

    return (0);
  }

  struct room_direction_data *ex = EXIT(ch, dir);

  /* Check if the character needs a skill check to go that way. */
  if (exit_dcskill_get(ex) != 0) {
    if (exit_dcmove_get(ex) > roll_skill(ch, exit_dcskill_get(ex))) {
      send_to_char(ch, "Your skill in %s isn't enough to move that way!\r\n",
                   spell_info[exit_dcskill_get(ex)].name);
      /* A failed skill check still spends the movement points! */
      if (!ADM_FLAGGED(ch, ADM_WALKANYWHERE) && !IS_NPC(ch) &&
          !char_condition_has(ch, "flying"))
        decCurST(ch, need_movement);
      return (0);
    } else {
      send_to_char(ch, "Your skill in %s aids in your movement.\r\n",
                   spell_info[exit_dcskill_get(ex)].name);
    }
  }

  struct room_data *dest = exit_dest_get(ex);

  if ((char_room_get(ch) && room_flagged(char_room_get(ch), ROOM_ATRIUM))) {
    if (!House_can_enter(ch, room_vnum_get(dest))) {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }
  if (room_flagged(dest, ROOM_TUNNEL) &&
      (num_pc_in_room(dest) >= CONFIG_TUNNEL_SIZE)) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(
          ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (room_flagged(dest, ROOM_GODROOM) && GET_ADMLEVEL(ch) < ADMLVL_GRGOD) {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }

  /******* Zone flag checks *******/

  rm = dest;
  auto zone = room_zone_get(rm);
  auto zone_min = zone->min_level;
  auto zone_max = zone->max_level;

  if (!IS_NPC(ch) && (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) &&
      (GET_LEVEL(ch) < zone_min) && (zone_min > 0)) {
    send_to_char(ch, "Sorry, you are too low a level to enter this zone.\r\n");
    return (0);
  }

  if ((GET_ADMLEVEL(ch) < ADMLVL_IMMORT) && (GET_LEVEL(ch) > zone_max) &&
      (zone_max > 0)) {
    send_to_char(ch, "Sorry, you are too high a level to enter this zone.\r\n");
    return (0);
  }

  if ((GET_ADMLEVEL(ch) < ADMLVL_IMMORT) && zone_flagged(zone, ZONE_CLOSED)) {
    send_to_char(ch, "This zone is currently closed to mortals.\r\n");
    return (0);
  }

  if ((GET_ADMLEVEL(ch) >= ADMLVL_IMMORT && GET_ADMLEVEL(ch) < ADMLVL_GRGOD) &&
      zone_flagged(zone, ZONE_NOIMMORT)) {
    send_to_char(ch, "This zone is closed to all.\r\n");
    return (0);
  }

  /* No low level immortal scouting */
  if ((GET_ADMLEVEL(ch) >= ADMLVL_IMMORT && GET_ADMLEVEL(ch) < ADMLVL_GOD) &&
      !can_edit_zone(ch, room_zone_get(rm)) && zone_flagged(zone, ZONE_QUEST)) {
    send_to_char(ch, "This is a Quest zone.\r\n");
    return (0);
  }

  /* Now we know we're allowed to go into the room. */
  if (!ADM_FLAGGED(ch, ADM_WALKANYWHERE) && !IS_NPC(ch) &&
      !char_condition_has(ch, "flying")) {
    decCurST(ch, need_movement);
  }

  if (AFF_FLAGGED(ch, AFF_SNEAK) && !IS_NPC(ch)) {
    sprintf(buf2, "$n sneaks %s.", dirs[dir]);
    if (GET_SKILL(ch, SKILL_MOVE_SILENTLY)) {
      improve_skill(ch, SKILL_MOVE_SILENTLY, 0);
    } else if (slot_count(ch) + 1 > GET_SLOTS(ch)) {
      send_to_char(
          ch,
          "@RYour skill slots are full. You can not learn Move Silently.\r\n");
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
    } else {
      send_to_char(ch, "@GYou learn the very basics of moving silently.@n\r\n");
      SET_SKILL(ch, SKILL_MOVE_SILENTLY, rand_number(5, 10));
      act(buf2, TRUE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
      if (GET_DEX(ch) < rand_number(1, 30)) {
        WAIT_STATE(ch, PULSE_1SEC);
      }
    }
  }

  if (!AFF_FLAGGED(ch, AFF_SNEAK) && !char_condition_has(ch, "flying")) {
    sprintf(buf2, "$n leaves %s.", dirs[dir]);
    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }
  if (!AFF_FLAGGED(ch, AFF_SNEAK) && char_condition_has(ch, "flying")) {
    sprintf(buf2, "$n flies %s.", dirs[dir]);
    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }

  was_in_room = char_room_get(ch);
  if (DRAGGING(ch)) {
    act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
  }
  if (CARRYING(ch)) {
    act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
  }
  SET_BIT_AR(AFF_FLAGS(ch), AFF_PURSUIT);
  char_from_room(ch);
  char_to_room(ch, exit_dest_get(room_dir_option_get(was_in_room, dir)));
  if ((room_zone_vnum_get(char_room_get(ch)) != room_zone_vnum_get(was_in_room)) && !IS_NPC(ch) &&
      !IS_ANDROID(ch)) {
    send_to_sense(0, "You sense someone", ch);
    sprintf(buf3,
            "@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection "
            "range@n.",
            add_commas(GET_HIT(ch)));
    send_to_scouter(buf3, ch, 0, 0);
  }
  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch) || !enter_wtrigger(char_room_get(ch), ch, dir)) {
    char_from_room(ch);
    char_to_room(ch, was_in_room);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PURSUIT);
    return 0;
  }

  snprintf(buf2, sizeof(buf2), "%s%s",
           ((dir == UP) || (dir == DOWN) ? "" : "the "),
           (dir == UP       ? "below"
            : (dir == DOWN) ? "above"
                            : dirs[rev_dir[dir]]));
  act("$n arrives from $T.", TRUE, ch, 0, buf2, TO_ROOM | TO_SNEAKRESIST);
  if (FIGHTING(ch)) {
    struct room_data *dest = exit_dest_get(room_dir_option_get(was_in_room, dir));
    int sect = room_sector_type_get(dest);
    if (sect != SECT_FLYING && sect != SECT_WATER_NOSWIM &&
        room_geffect_get(dest) == 0) {
      roll_pursue(FIGHTING(ch), ch);
    }
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PURSUIT);
  }
  if (DRAGGING(ch)) {
    act("@wYou drag @C$N@w with you.@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
    act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
    char_from_room(DRAGGING(ch));
    char_to_room(DRAGGING(ch), char_room_get(ch));
    if (SITS(DRAGGING(ch))) {
      obj_from_room(SITS(DRAGGING(ch)));
      obj_to_room(SITS(DRAGGING(ch)), char_room_get(ch));
    }
    if (!AFF_FLAGGED(DRAGGING(ch), AFF_KNOCKED) &&
        !is_affected(DRAGGING(ch), AFF_SLEEP) && rand_number(1, 3)) {
      send_to_char(DRAGGING(ch),
                   "You feel your sleeping body being moved.\r\n");
      if (IS_NPC(DRAGGING(ch)) && !FIGHTING(DRAGGING(ch))) {
        set_fighting(DRAGGING(ch), ch);
      }
    }
  }
  if (CARRYING(ch)) {
    act("@wYou carry @C$N@w with you.@n", TRUE, ch, 0, CARRYING(ch), TO_CHAR);
    act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
    char_from_room(CARRYING(ch));
    char_to_room(CARRYING(ch), char_room_get(ch));
    if (!AFF_FLAGGED(CARRYING(ch), AFF_KNOCKED) &&
        !is_affected(CARRYING(ch), AFF_SLEEP) && rand_number(1, 3)) {
      send_to_char(CARRYING(ch),
                   "You feel your sleeping body being moved.\r\n");
    }
  }

  if (ch->desc != NULL) {
    look_at_room(char_room_get(ch), ch, 0);
    if (AFF_FLAGGED(ch, AFF_SNEAK) && !IS_NPC(ch) &&
        GET_SKILL(ch, SKILL_MOVE_SILENTLY) &&
        GET_SKILL(ch, SKILL_MOVE_SILENTLY) < rand_number(1, 101)) {
      send_to_char(
          ch,
          "@wYou make a noise as you arrive and are no longer sneaking!@n\r\n");
      act("@c$n@w makes a noise revealing $s sneaking!@n", TRUE, ch, 0, 0,
          TO_ROOM | TO_SNEAKRESIST);
      reveal_hiding(ch, 0);
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
    }
  }

  if (room_geffect_get(char_room_get(ch)) == 6 ||
      room_geffect_get(was_in_room) == 6) {
    if (!IS_DEMON(ch) && !char_condition_has(ch, "flying") &&
        group_bonus(ch, 2) != 14) {
      act("@rYour legs are burned by the lava!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@R$n@r's legs are burned by the lava!@n", TRUE, ch, 0, 0, TO_ROOM);
      if (IS_NPC(ch) && IS_HUMANOID(ch) && rand_number(1, 2) == 2) {
        do_fly(ch, 0, 0, 0);
      }
      decCurHealth(ch, getMaxPL(ch) / 20);
      if (GET_HIT(ch) <= 0) {
        act("@rYou have burned to death!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@R$n@r has burned to death!@n", TRUE, ch, 0, 0, TO_ROOM);
        die(ch, NULL);
      }
    }
    if (DRAGGING(ch) && !IS_DEMON(DRAGGING(ch))) {
      act("@R$N@r gets burned!@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
      act("@R$N@r gets burned!@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
      decCurHealth(DRAGGING(ch), getMaxPL(DRAGGING(ch)) / 20);
      if (GET_HIT(DRAGGING(ch)) < 0) {
        act("@rYou have burned to death!@n", TRUE, DRAGGING(ch), 0, 0, TO_CHAR);
        act("@R$n@r has burned to death!@n", TRUE, DRAGGING(ch), 0, 0, TO_ROOM);
        die(DRAGGING(ch), NULL);
      }
    }
  }

  entry_memory_mtrigger(ch);
  if (!greet_mtrigger(ch, dir)) {
    char_from_room(ch);
    char_to_room(ch, was_in_room);
    look_at_room(char_room_get(ch), ch, 0);
  } else
    greet_memory_mtrigger(ch);
  if (willfall == TRUE) {
    handle_fall(ch);
    if (DRAGGING(ch)) {
      handle_fall(DRAGGING(ch));
    }
  }
  return (1);
}

int perform_move(struct char_data *ch, int dir, int need_specials_check) {
  struct room_data *was_in_room = NULL;
  if (GRAPPLING(ch) || GRAPPLED(ch)) {
    send_to_char(ch, "You are grappling with someone!\r\n");
    return (0);
  }

  if (ABSORBING(ch) || ABSORBBY(ch)) {
    send_to_char(ch, "You are struggling with someone!\r\n");
    return (0);
  }

  if (!AFF_FLAGGED(ch, AFF_SNEAK) ||
      (AFF_FLAGGED(ch, AFF_SNEAK) &&
       GET_SKILL(ch, SKILL_MOVE_SILENTLY) < axion_dice(0))) {
    reveal_hiding(ch, 0);
  }

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS)
    return (0);
  else if ((!EXIT(ch, dir) && !buildwalk(ch, dir)) ||
           exit_to_room_vnum_get(EXIT(ch, dir)) == NOWHERE ||
           (exit_flagged(EXIT(ch, dir), EX_SECRET) &&
            (exit_flagged(EXIT(ch, dir), EX_CLOSED))))
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if (exit_flagged(EXIT(ch, dir), EX_CLOSED)) {
    if (exit_keyword_get(EXIT(ch, dir)))
      send_to_char(ch, "The %s seems to be closed.\r\n",
                   fname(exit_keyword_get(EXIT(ch, dir))));
    else
      send_to_char(ch, "It seems to be closed.\r\n");
  } else if (exit_to_room_vnum_get(EXIT(ch, dir)) == 0 || exit_to_room_vnum_get(EXIT(ch, dir)) == 1) {
    send_to_char(ch, "Report this direction, it is illegal.\r\n");
  } else {

    {
      int wall_found = 0;
      room_contents_iterate(char_room_get(ch), [&](auto wall) {
        if (GET_OBJ_VNUM(wall) == 79) {
          if (GET_OBJ_COST(wall) == dir) {
            send_to_char(ch,
                         "That direction has a glacial wall blocking it.\r\n");
            wall_found = 1;
            return false;
          }
        }
        return true;
      });
      if (wall_found)
        return (0);
    }

    if (!char_follower_count(ch))
      return (do_simple_move(ch, dir, need_specials_check));

    was_in_room = char_room_get(ch);
    if (!do_simple_move(ch, dir, need_specials_check))
      return (0);

    char_followers_iterate(ch, [&](struct char_data *k) {
      if ((char_room_get(k) == was_in_room) &&
          (GET_POS(k) >= POS_STANDING) &&
          (!char_condition_has(ch, "zanzoken") ||
           (char_condition_has(ch, "group") && char_condition_has(k, "group")))) {
        act("You follow $N.\r\n", FALSE, k, 0, ch, TO_CHAR);
        perform_move(k, dir, 1);
      } else if ((char_room_get(k) == was_in_room) &&
                 (GET_POS(k) >= POS_STANDING) &&
                 (char_condition_has(ch, "zanzoken") && char_condition_has(k, "zanzoken")) &&
                 (!char_condition_has(ch, "group") || !char_condition_has(k, "group"))) {
        act("$N tries to zanzoken and escape, but your zanzoken matches $S!\r\n",
            FALSE, k, 0, ch, TO_CHAR);
        act("$N tries to zanzoken and escape, but $n's zanzoken matches $S!\r\n",
            FALSE, k, 0, ch, TO_NOTVICT);
        act("You zanzoken to try and escape, but $n's zanzoken matches yours!\r\n",
            FALSE, k, 0, ch, TO_VICT);
        char_condition_remove(ch, "zanzoken", "zanzoken_over");
        char_condition_remove(k, "zanzoken", "zanzoken_over");
        perform_move(k, dir, 1);
      } else if ((char_room_get(k) == was_in_room) &&
                 (GET_POS(k) >= POS_STANDING) &&
                 (char_condition_has(ch, "zanzoken") && !char_condition_has(k, "zanzoken"))) {
        act("You try to follow $N, but $E disappears in a flash of movement!\r\n",
            FALSE, k, 0, ch, TO_CHAR);
        act("$n tries to follow $N, but $E disappears in a flash of movement!\r\n",
            FALSE, k, 0, ch, TO_NOTVICT);
        act("$n tries to follow you, but you manage to zanzoken away!\r\n",
            FALSE, k, 0, ch, TO_VICT);
        char_condition_remove(ch, "zanzoken", "zanzoken_over");
      }
      return true;
    });
    return (1);
  }
  return (0);
}

ACMD(do_move) {}

static int find_door(struct char_data *ch, const char *type, char *dir,
                     const char *cmdname) {
  int door = NOTHING;

  if (*dir) { /* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) < 0 &&
        (door = search_block(dir, abbr_dirs, FALSE)) < 0) { /* Partial Match */
      send_to_char(ch, "That's not a direction.\r\n");
      return (-1);
    }
    if (EXIT(ch, door)) { /* Braces added according to indent. -gg */
      if (exit_keyword_get(EXIT(ch, door))) {
        if (is_name(type, exit_keyword_get(EXIT(ch, door))))
          return (door);
        else {
          send_to_char(ch, "I see no %s there.\r\n", type);
          return (-1);
        }
      } else
        return (door);
    } else {
      send_to_char(ch, "I really don't see how you can %s anything there.\r\n",
                   cmdname);
      return (-1);
    }
  } else { /* try to locate the keyword */
    if (!*type) {
      send_to_char(ch, "What is it you want to %s?\r\n", cmdname);
      return (-1);
    }
    room_exits_iterate(char_room_get(ch), [&](auto dir, auto exit) {
      if(!exit_keyword_get(exit)) return true;
      if(is_name(type, exit_keyword_get(exit))) {
        door = dir;
        return false;
      }
      return true;
    });
    if(door != NOTHING) {
      return door;
    }

    send_to_char(ch,
                 "There doesn't seem to be %s %s that could be manipulated in "
                 "that way here.\r\n",
                 AN(type), type);
    return (-1);
  }
}

static int has_key(struct char_data *ch, obj_vnum key) {
  if (key == 1) {
    return (1);
  }

  {
    bool found = false;
    char_inventory_iterate(ch, [&](auto o) {
      if (GET_OBJ_VNUM(o) == key) {
        found = true;
        return false;
      }
      return true;
    });
    if (found) return (1);
  }

  {
    bool found_key = false;
    char_equipment_iterate(ch, [&](auto i, auto eq) {
      if (GET_OBJ_VNUM(eq) == key) {
        found_key = true;
        return false;
      }
      return true;
    });
    if (found_key) return (1);
  }

  return (0);
}

#define NEED_OPEN (1 << 0)
#define NEED_CLOSED (1 << 1)
#define NEED_UNLOCKED (1 << 2)
#define NEED_LOCKED (1 << 3)

const char *cmd_door[NUM_DOOR_CMD] = {"open", "close", "unlock", "lock",
                                      "pick"};

static const int flags_door[] = {
    NEED_CLOSED | NEED_UNLOCKED, NEED_OPEN, NEED_CLOSED | NEED_LOCKED,
    NEED_CLOSED | NEED_UNLOCKED, NEED_CLOSED | NEED_LOCKED};

#define EXITN(room, door) room_dir_option_get(room, door)
#define OPEN_DOOR(room, obj, door)                                             \
  ((obj) ? (REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_CLOSED))    \
         : (exit_flag_set(EXITN(room, door), EX_CLOSED, false), 0))
#define CLOSE_DOOR(room, obj, door)                                            \
  ((obj) ? (SET_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_CLOSED))       \
         : (exit_flag_set(EXITN(room, door), EX_CLOSED, true), 0))
#define LOCK_DOOR(room, obj, door)                                             \
  ((obj) ? (SET_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_LOCKED))       \
         : (exit_flag_set(EXITN(room, door), EX_LOCKED, true), 0))
#define UNLOCK_DOOR(room, obj, door)                                           \
  ((obj) ? (REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_LOCKED))    \
         : (exit_flag_set(EXITN(room, door), EX_LOCKED, false), 0))
#define TOGGLE_LOCK(room, obj, door)                                           \
  ((obj) ? (TOGGLE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_LOCKED))    \
         : (exit_flag_toggle(EXITN(room, door), EX_LOCKED), 0))

static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door,
                       int scmd) {
  char buf[MAX_STRING_LENGTH];
  size_t len;
  struct room_data *num = NULL;
  struct room_direction_data *back = NULL;
  struct obj_data *hatch = NULL, *obj2 = NULL, *next_obj, *vehicle = NULL;

  if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH) {
    vehicle = hatch_get_vehicle(obj);
  } else if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
    if (room_by_id(GET_OBJ_VAL(obj, VAL_PORTAL_DEST))) {
      num = char_room_get(ch);
      char_from_room(ch);
      char_to_room(ch, room_by_id(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)));
    }
    room_contents_iterate(char_room_get(ch), [&](auto obj2) {
      if (GET_OBJ_TYPE(obj2) == ITEM_HATCH) {
        hatch = obj2;
      }
      return true;
    });
    obj2 = NULL;
  }

  if (!door_mtrigger(ch, scmd, door))
    return;

  if (!door_wtrigger(ch, scmd, door))
    return;

  len = snprintf(buf, sizeof(buf), "$n %ss ", cmd_door[scmd]);
  struct room_data *other_room_ptr = exit_dest_get(EXIT(ch, door));

  if (!obj && other_room_ptr) {
    if ((back = room_dir_option_get(other_room_ptr, rev_dir[door])) != NULL)
      if (exit_dest_get(back) != char_room_get(ch))
        back = NULL;
  }

  switch (scmd) {
  case SCMD_OPEN:
    if (obj) {
      if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle)) {
        OPEN_DOOR(char_room_get(ch), vehicle, door);
        if (GET_OBJ_VNUM(obj) > 19199) {
          send_to_room(char_room_get(ch),
                       "@wThe ship hatch opens slowly and settles onto the "
                       "ground outside.\r\n");
          send_to_room(
              obj_room_get(vehicle),
              "@wThe ship hatch opens slowly and settles onto the ground.\r\n");
          if (room_flagged(obj_room_get(vehicle), ROOM_SPACE)) {
            send_to_room(char_room_get(ch),
                         "@wA great vortex forms as air begins to get sucked "
                         "out into the void!\r\n");
          }
        } else {
          act("@wYou open @c$p@w.", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@w opens @c$p@w.", TRUE, ch, obj, 0, TO_ROOM);
          send_to_room(obj_room_get(vehicle),
                       "@wThe door to %s@w is opened from the other side.\r\n",
                       vehicle->short_description);
        }
        vehicle = NULL;
      }
      if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch)) {
        OPEN_DOOR(char_room_get(ch), hatch, door);
        char_from_room(ch);
        char_to_room(ch, num);
        if (GET_OBJ_VNUM(obj) > 19199) {
          send_to_room(
              char_room_get(ch),
              "@wThe ship hatch opens slowly and settles onto the ground.\r\n");
          send_to_room(obj_room_get(hatch),
                       "@wThe ship hatch opens slowly.\r\n");
          if (room_flagged(obj_room_get(obj), ROOM_SPACE)) {
            send_to_room(char_room_get(ch),
                         "@wThe air starts getting sucked out into space as "
                         "the hatch opens!\r\n");
          }
        } else {
          act("@wYou open @c$p@w.", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@w opens @c$p@w.", TRUE, ch, obj, 0, TO_ROOM);
          send_to_room(obj_room_get(hatch),
                       "@wThe door is opened from the other side.\r\n");
        }
        hatch = NULL;
      }
    }
    OPEN_DOOR(char_room_get(ch), obj, door);
    if (back) {
      OPEN_DOOR(other_room_ptr, obj, rev_dir[door]);
    }
    if (!obj) {
      send_to_char(ch, "You open the %s that leads %s.\r\n",
                   exit_keyword_get(EXIT(ch, door)) ? exit_keyword_get(EXIT(ch, door)) : "door",
                   dirs[door]);
    } else if (GET_OBJ_TYPE(obj) != ITEM_VEHICLE &&
               GET_OBJ_TYPE(obj) != ITEM_HATCH) {
      send_to_char(ch, "You open %s.\r\n", obj->short_description);
    }
    break;

  case SCMD_CLOSE:
    if (obj) {
      if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle)) {
        CLOSE_DOOR(char_room_get(ch), vehicle, door);
        if (GET_OBJ_VNUM(obj) > 19199) {
          send_to_room(char_room_get(ch),
                       "@wThe ship hatch slowly closes, sealing the ship from "
                       "the outside.\r\n");
          send_to_room(obj_room_get(vehicle),
                       "@wThe ship hatch slowly closes, sealing the ship.\r\n");
          if (room_flagged(obj_room_get(vehicle), ROOM_SPACE)) {
            send_to_room(char_room_get(ch),
                         "@wThe air stops getting sucked out into space as the "
                         "hatch seals!\r\n");
          }
        } else {
          act("@wYou close @c$p@w.", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@w closes @c$p@w.", TRUE, ch, obj, 0, TO_ROOM);
          send_to_room(obj_room_get(vehicle),
                       "@wThe door to %s@w is closed from the other side.\r\n",
                       vehicle->short_description);
        }
        vehicle = NULL;
      }
      if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch)) {
        CLOSE_DOOR(char_room_get(ch), hatch, door);
        char_from_room(ch);
        char_to_room(ch, num);
        if (GET_OBJ_VNUM(obj) > 19199) {
          send_to_room(char_room_get(ch),
                       "@wThe ship hatch slowly closes, sealing the ship.\r\n");
          send_to_room(obj_room_get(hatch),
                       "@wThe ship hatch slowly closes, sealing the ship from "
                       "the outside.\r\n");
          if (room_flagged(obj_room_get(obj), ROOM_SPACE)) {
            send_to_room(char_room_get(ch),
                         "@wAir stops getting sucked out into space as the "
                         "hatch seals!\r\n");
          }
        } else {
          act("@wYou close @c$p@w.", TRUE, ch, obj, 0, TO_CHAR);
          act("@C$n@w closes @c$p@w.", TRUE, ch, obj, 0, TO_ROOM);
          send_to_room(obj_room_get(hatch),
                       "@wThe door to %s@w is closed from the other side.\r\n",
                       hatch->short_description);
        }
        hatch = NULL;
      }
    }
    CLOSE_DOOR(char_room_get(ch), obj, door);
    if (back) {
      CLOSE_DOOR(other_room_ptr, obj, rev_dir[door]);
    }
    if (!obj) {
      send_to_char(ch, "You close the %s that leads %s.\r\n",
                   exit_keyword_get(EXIT(ch, door)) ? exit_keyword_get(EXIT(ch, door)) : "door",
                   dirs[door]);
    } else if (GET_OBJ_TYPE(obj) != ITEM_VEHICLE &&
               GET_OBJ_TYPE(obj) != ITEM_HATCH) {
      send_to_char(ch, "You close %s.\r\n", obj->short_description);
    }
    break;

  case SCMD_LOCK:
    if (obj) {
      if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle)) {
        LOCK_DOOR(char_room_get(ch), vehicle, door);
        vehicle = NULL;
      }
      if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch)) {
        LOCK_DOOR(char_room_get(ch), hatch, door);
        char_from_room(ch);
        char_to_room(ch, num);
        hatch = NULL;
      }
    }
    LOCK_DOOR(char_room_get(ch), obj, door);
    if (back) {
      LOCK_DOOR(other_room_ptr, obj, rev_dir[door]);
    }
    if (!obj) {
      send_to_char(ch, "You lock the %s that leads %s.\r\n",
                   exit_keyword_get(EXIT(ch, door)) ? exit_keyword_get(EXIT(ch, door)) : "door",
                   dirs[door]);
    } else {
      send_to_char(ch, "You lock %s.\r\n", obj->short_description);
    }
    break;

  case SCMD_UNLOCK:
    if (obj) {
      if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle)) {
        UNLOCK_DOOR(char_room_get(ch), vehicle, door);
        vehicle = NULL;
      }
      if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch)) {
        UNLOCK_DOOR(char_room_get(ch), hatch, door);
        char_from_room(ch);
        char_to_room(ch, num);
        hatch = NULL;
      }
    }
    UNLOCK_DOOR(char_room_get(ch), obj, door);
    if (back) {
      UNLOCK_DOOR(other_room_ptr, obj, rev_dir[door]);
    }
    if (!obj) {
      send_to_char(ch, "You unlock the %s that leads %s.\r\n",
                   exit_keyword_get(EXIT(ch, door)) ? exit_keyword_get(EXIT(ch, door)) : "door",
                   dirs[door]);
    } else {
      send_to_char(ch, "You unlock %s.\r\n", obj->short_description);
    }
    break;

  case SCMD_PICK:
    TOGGLE_LOCK(char_room_get(ch), obj, door);
    if (back)
      TOGGLE_LOCK(other_room_ptr, obj, rev_dir[door]);
    send_to_char(ch, "The lock quickly yields to your skills.\r\n");
    len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
    break;
  }

  /* Notify the room. */
  char dbuf[100];
  if (!obj) {
    sprintf(dbuf, "%s", dirs[door]);
  }
  if (len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "%s%s%s%s.", obj ? "" : "the ",
             obj                       ? "$p"
             : exit_keyword_get(EXIT(ch, door)) ? "$F"
                                       : "door",
             obj ? "" : " that leads ", obj ? "" : dbuf);
  if (!obj || obj_room_get(obj) != NULL)
    act(buf, FALSE, ch, obj, obj ? 0 : exit_keyword_get(EXIT(ch, door)), TO_ROOM);

  /* Notify the other room */
  if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE)) {
    send_to_room(exit_dest_get(EXIT(ch, door)),
                 "The %s that leads %s is %s%s from the other side.\r\n",
                 exit_keyword_get(back) ? fname(exit_keyword_get(back)) : "door", dbuf,
                 cmd_door[scmd], scmd == SCMD_CLOSE ? "d" : "ed");
  } else if (back && (scmd == SCMD_LOCK || scmd == SCMD_UNLOCK)) {
    send_to_room(exit_dest_get(EXIT(ch, door)),
                 "The %s that leads %s is %sed from the other side.\r\n",
                 exit_keyword_get(back) ? fname(exit_keyword_get(back)) : "door", dbuf,
                 cmd_door[scmd]);
  }
  *dbuf = '\0';
}

static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof,
                   int dclock, int scmd, struct obj_data *hatch) {
  int skill_lvl, found = FALSE;
  struct obj_data *obj = char_inventory_search_vnum(
      ch, 18, FALSE, SEARCH_GENUINE | SEARCH_WORKING);

  if (scmd != SCMD_PICK)
    return (1);

  /* PICKING_LOCKS is not an untrained skill */
  if (!GET_SKILL(ch, SKILL_OPEN_LOCK)) {
    send_to_char(ch, "You have no idea how!\r\n");
    return (0);
  }
  if (!obj) {
    send_to_char(ch, "You need a lock picking kit.\r\n");
    return (0);
  }
  if (hatch != NULL && (GET_OBJ_TYPE(hatch) == ITEM_HATCH ||
                        GET_OBJ_TYPE(hatch) == ITEM_VEHICLE)) {
    send_to_char(ch, "No picking ship hatches.\r\n");
    hatch = NULL;
    return (0);
  }
  skill_lvl = roll_skill(ch, SKILL_OPEN_LOCK);
  if (dclock == 0) {
    dclock = rand_number(1, 101);
  }

  if (keynum == NOTHING) {
    send_to_char(ch, "Odd - you can't seem to find a keyhole.\r\n");
  } else if (pickproof) {
    send_to_char(ch, "It resists your attempts to pick it.\r\n");
    act("@c$n@w puts a set of lockpick tools away.@n", TRUE, ch, 0, 0, TO_ROOM);
    /* The -2 is here because that is a penality for not having a set of
     * thieves' tools. If the player has them, that modifier will be accounted
     * for in roll_skill, and negate (or surpass) this.
     */
  } else if ((getCurST(ch)) < GET_MAX_MOVE(ch) / 30) {
    send_to_char(
        ch,
        "You don't have the stamina to try, it takes percision to pick locks."
        "Not shaking tired hands.\r\n");
  } else if (dclock > (skill_lvl - 2)) {
    send_to_char(ch, "You failed to pick the lock...\r\n");
    act("@c$n@w puts a set of lockpick tools away.@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurST(ch, getCurST(ch) / 30);
  } else {
    decCurST(ch, getCurST(ch) / 30);
    return (1);
  }

  return (0);
}

#define DOOR_IS_OPENABLE(ch, obj, door)                                        \
  ((obj) ? ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) &&                           \
            OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) ||                            \
               ((GET_OBJ_TYPE(obj) == ITEM_VEHICLE) &&                         \
                OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) ||                        \
               ((GET_OBJ_TYPE(obj) == ITEM_HATCH) &&                           \
                OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) ||                        \
               ((GET_OBJ_TYPE(obj) == ITEM_WINDOW) &&                          \
                OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) ||                        \
               ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) &&                          \
                OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))                           \
         : (exit_flagged(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door)                                            \
  ((obj) ? (!OBJVAL_FLAGGED(obj, CONT_CLOSED))                                 \
         : (!exit_flagged(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)                                        \
  ((obj) ? (!OBJVAL_FLAGGED(obj, CONT_LOCKED))                                 \
         : (!exit_flagged(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door)                                       \
  ((obj) ? (OBJVAL_FLAGGED(obj, CONT_PICKPROOF))                               \
         : (exit_flagged(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_SECRET(ch, obj, door)                                          \
  ((obj) ? (OBJVAL_FLAGGED(obj, CONT_SECRET))                                  \
         : (exit_flagged(EXIT(ch, door), EX_SECRET)))

#define DOOR_IS_CLOSED(ch, obj, door) (!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door) (!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)                                                \
  ((obj) ? (GET_OBJ_VAL(obj, VAL_KEY_KEYCODE)) : exit_key_get(EXIT(ch, door)))
#define DOOR_DCLOCK(ch, obj, door)                                             \
  ((obj) ? (GET_OBJ_VAL(obj, VAL_DOOR_DCLOCK)) : exit_dclock_get(EXIT(ch, door)))

ACMD(do_gen_door) {
  int door = -1;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;

  skip_spaces(&argument);
  if (!*argument) {
    send_to_char(ch, "%c%s what?\r\n", UPPER(*cmd_door[subcmd]),
                 cmd_door[subcmd] + 1);
    return;
  }
  two_arguments(argument, type, dir);
  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(ch, type, dir, cmd_door[subcmd]);

  if ((obj) &&
      (GET_OBJ_TYPE(obj) != ITEM_CONTAINER &&
       GET_OBJ_TYPE(obj) != ITEM_VEHICLE && GET_OBJ_TYPE(obj) != ITEM_HATCH)) {
    obj = NULL;
    door = find_door(ch, type, dir, cmd_door[subcmd]);
  }

  if ((obj) || (door >= 0)) {
    keynum = DOOR_KEY(ch, obj, door);
    if (!DOOR_DCLOCK(ch, obj, door)) {
      if (obj) {
        GET_OBJ_VAL(obj, VAL_DOOR_DCLOCK) = 20;
      } else {
        exit_dclock_set(EXIT(ch, door), 20);
      }
    }
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
    else if (!DOOR_IS_OPEN(ch, obj, door) &&
             IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char(ch, "But it's already closed!\r\n");
    else if (!DOOR_IS_CLOSED(ch, obj, door) &&
             IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char(ch, "But it's currently open!\r\n");
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
             IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char(ch, "Oh.. it wasn't locked, after all..\r\n");
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
             IS_SET(flags_door[subcmd], NEED_UNLOCKED))
      send_to_char(ch, "It seems to be locked.\r\n");
    else if (!has_key(ch, keynum) && !ADM_FLAGGED(ch, ADM_NOKEYS) &&
             ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char(ch, "You don't seem to have the proper key.\r\n");
    else if (!obj && ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door),
                             DOOR_DCLOCK(ch, obj, door), subcmd, NULL))
      do_doorcmd(ch, obj, door, subcmd);
    else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door),
                     DOOR_DCLOCK(ch, obj, door), subcmd, obj) &&
             obj)
      do_doorcmd(ch, obj, door, subcmd);
  }
  return;
}

int do_simple_enter(struct char_data *ch, struct obj_data *obj,
                    int need_specials_check) {
  struct room_data *dest_room = room_by_id(GET_OBJ_VAL(obj, VAL_PORTAL_DEST));
  struct room_data *was_in = char_room_get(ch);
  int need_movement = 0;

  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && MASTER(ch) &&
      char_room_get(ch) == char_room_get(MASTER(ch))) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = 1;
  int gravity = room_gravity_get(char_room_get(ch));
  if (gravity > 10) {
    need_movement = (need_movement + gravity) * gravity;
  } else if (gravity == 10 && !IS_BARDOCK(ch) && !IS_NPC(ch)) {
    need_movement = (need_movement + gravity) * gravity;
  }
  if (GET_LEVEL(ch) <= 1) {
    need_movement = 0;
  }
  if ((getCurST(ch)) < need_movement && !char_condition_has(ch, "flying") &&
      !IS_NPC(ch)) {
    if (need_specials_check && MASTER(ch))
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");

    return (0);
  }
  if ((char_room_get(ch) && room_flagged(char_room_get(ch), ROOM_ATRIUM))) {
    if (!House_can_enter(ch, room_vnum_get(dest_room))) {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }
  if (room_flagged(dest_room, ROOM_TUNNEL) &&
      num_pc_in_room(dest_room) >= CONFIG_TUNNEL_SIZE) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(
          ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (room_flagged(dest_room, ROOM_GODROOM) &&
      GET_ADMLEVEL(ch) < ADMLVL_GRGOD) {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }
  /* Now we know we're allowed to go into the room. */
  if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)) &&
      !char_condition_has(ch, "flying"))
    decCurST(ch, need_movement);

  act("$n enters $p.", TRUE, ch, obj, 0, TO_ROOM | TO_SNEAKRESIST);

  if (DRAGGING(ch)) {
    act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
  }
  if (CARRYING(ch)) {
    act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
  }
  char_from_room(ch);
  char_to_room(ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch)) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_PORTAL)
    act("$n arrives from $p.", FALSE, ch, obj, 0, TO_ROOM | TO_SNEAKRESIST);
  else
    act("$n arrives from outside.", FALSE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
  if (DRAGGING(ch)) {
    act("@wYou drag @C$N@w with you.@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
    act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
    if (!AFF_FLAGGED(DRAGGING(ch), AFF_KNOCKED) &&
        !is_affected(DRAGGING(ch), AFF_SLEEP) && rand_number(1, 3)) {
      send_to_char(DRAGGING(ch),
                   "You feel your sleeping body being moved.\r\n");
      if (IS_NPC(DRAGGING(ch)) && !FIGHTING(DRAGGING(ch))) {
        set_fighting(DRAGGING(ch), ch);
      }
    }
    char_from_room(DRAGGING(ch));
    char_to_room(DRAGGING(ch), char_room_get(ch));
    if (SITS(DRAGGING(ch))) {
      obj_from_room(SITS(DRAGGING(ch)));
      obj_to_room(SITS(DRAGGING(ch)), char_room_get(ch));
    }
  }
  if (CARRYING(ch)) {
    act("@wYou carry @C$N@w with you.@n", TRUE, ch, 0, CARRYING(ch), TO_CHAR);
    act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
    if (!AFF_FLAGGED(CARRYING(ch), AFF_KNOCKED) &&
        !is_affected(CARRYING(ch), AFF_SLEEP) && rand_number(1, 3)) {
      send_to_char(CARRYING(ch),
                   "You feel your sleeping body being moved.\r\n");
    }
    char_from_room(CARRYING(ch));
    char_to_room(CARRYING(ch), char_room_get(ch));
    if (SITS(CARRYING(ch))) {
      obj_from_room(SITS(CARRYING(ch)));
      obj_to_room(SITS(CARRYING(ch)), char_room_get(ch));
    }
  }

  if (ch->desc != NULL)
    look_at_room(char_room_get(ch), ch, 0);

  if ((char_room_get(ch) && room_flagged(char_room_get(ch), ROOM_DEATH)) &&
      !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch);
    return 0;
  }

  entry_memory_mtrigger(ch);
  greet_memory_mtrigger(ch);

  return 1;
}

static int perform_enter_obj(struct char_data *ch, struct obj_data *obj,
                             int need_specials_check) {
  struct room_data *was_in_room = char_room_get(ch);
  int could_move = FALSE;

  if (GRAPPLING(ch) || GRAPPLED(ch)) {
    send_to_char(ch, "You are grappling with someone!\r\n");
    return (0);
  }

  if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE || GET_OBJ_TYPE(obj) == ITEM_PORTAL) {
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
      send_to_char(ch, "But it's closed!\r\n");
    } else if ((GET_OBJ_VAL(obj, VAL_PORTAL_DEST) != NOWHERE) &&
               (room_by_id(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)))) {
      if (GET_OBJ_VAL(obj, VAL_PORTAL_DEST) >= 45000 &&
          GET_OBJ_VAL(obj, VAL_PORTAL_DEST) <= 45099) {
        struct char_data *tch, *next_v;
        int filled = FALSE;
        room_people_iterate(room_by_id(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)), [&](auto tch) {
          if (tch) {
            filled = TRUE;
          }
          return true;
        });
        if (filled == TRUE) {
          send_to_char(ch, "Only one person can fit in there at a time.\r\n");
          return (0);
        }
      }
      if ((could_move = do_simple_enter(ch, obj, need_specials_check)))
        char_followers_iterate(ch, [&](struct char_data *k) {
          if ((char_room_get(k) == was_in_room) && (GET_POS(k) >= POS_STANDING)) {
            act("You follow $N.\r\n", FALSE, k, 0, ch, TO_CHAR);
            perform_enter_obj(k, obj, 1);
          }
          return true;
        });
    } else {
      send_to_char(ch,
                   "It doesn't look like you can enter it at the moment.\r\n");
    }
  } else {
    send_to_char(ch, "You can't enter that!\r\n");
  }
  return could_move;
}

ACMD(do_enter) {}

int do_simple_leave(struct char_data *ch, struct obj_data *obj,
                           int need_specials_check)

{
  struct room_data *was_in = char_room_get(ch), *dest_room = NULL;
  int need_movement = 0;
  struct obj_data *vehicle = NULL;

  if (GET_OBJ_TYPE(obj) != ITEM_PORTAL) {
    vehicle = hatch_get_vehicle(obj);
  }

  if (vehicle == NULL && GET_OBJ_TYPE(obj) != ITEM_PORTAL) {
    send_to_char(ch, "That doesn't appear to lead anywhere.\r\n");
    return 0;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_PORTAL && OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
    send_to_char(ch, "But it's closed!\r\n");
    return 0;
  }

  if (vehicle != NULL) {
    if (!(dest_room = obj_room_get(vehicle))) {
      send_to_char(ch, "That doesn't appear to lead anywhere.\r\n");
      return 0;
    }
  }
  if (vehicle == NULL) {
    if (!(dest_room = room_by_id(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)))) {
      send_to_char(ch, "That doesn't appear to lead anywhere.\r\n");
      return 0;
    }
  }

  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && MASTER(ch) &&
      char_room_get(ch) == char_room_get(MASTER(ch))) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = 1;
  int gravity = room_gravity_get(char_room_get(ch));
  if (gravity > 10) {
    need_movement = (need_movement + gravity) * gravity;
  } else if (gravity == 10 && !IS_BARDOCK(ch) && !IS_NPC(ch)) {
    need_movement = (need_movement + gravity) * gravity;
  }
  if (GET_LEVEL(ch) <= 1) {
    need_movement = 0;
  }
  if ((getCurST(ch)) < need_movement && !char_condition_has(ch, "flying") &&
      !IS_NPC(ch)) {
    if (need_specials_check && MASTER(ch))
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");

    return (0);
  }
  if ((char_room_get(ch) && room_flagged(char_room_get(ch), ROOM_ATRIUM))) {
    if (!House_can_enter(ch, room_vnum_get(dest_room))) {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }
  if (room_flagged(dest_room, ROOM_TUNNEL) &&
      num_pc_in_room(dest_room) >= CONFIG_TUNNEL_SIZE) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(
          ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
  /* Now we know we're allowed to go into the room. */
  if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)) &&
      !char_condition_has(ch, "flying"))
    decCurST(ch, need_movement);

  act("$n leaves $p.", TRUE, ch, vehicle, 0, TO_ROOM | TO_SNEAKRESIST);

  if (DRAGGING(ch)) {
    act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
  }
  if (CARRYING(ch)) {
    act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
  }
  char_from_room(ch);
  char_to_room(ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch)) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }

  if (vehicle) {
    act("$n arrives from inside $p.", TRUE, ch, vehicle, 0,
        TO_ROOM | TO_SNEAKRESIST);
  } else {
    act("$n arrives from inside", TRUE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
  }
  if (DRAGGING(ch)) {
    act("@wYou drag @C$N@w with you.@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
    act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
    char_from_room(DRAGGING(ch));
    char_to_room(DRAGGING(ch), char_room_get(ch));
    if (SITS(DRAGGING(ch))) {
      obj_from_room(SITS(DRAGGING(ch)));
      obj_to_room(SITS(DRAGGING(ch)), char_room_get(ch));
    }
    if (!AFF_FLAGGED(DRAGGING(ch), AFF_KNOCKED) &&
        !is_affected(DRAGGING(ch), AFF_SLEEP) && rand_number(1, 3)) {
      send_to_char(DRAGGING(ch),
                   "You feel your sleeping body being moved.\r\n");
      if (IS_NPC(DRAGGING(ch)) && !FIGHTING(DRAGGING(ch))) {
        set_fighting(DRAGGING(ch), ch);
      }
    }
  }
  if (CARRYING(ch)) {
    act("@wYou carry @C$N@w with you.@n", TRUE, ch, 0, CARRYING(ch), TO_CHAR);
    act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
    char_from_room(CARRYING(ch));
    char_to_room(CARRYING(ch), char_room_get(ch));
    if (SITS(CARRYING(ch))) {
      obj_from_room(SITS(CARRYING(ch)));
      obj_to_room(SITS(CARRYING(ch)), char_room_get(ch));
    }
    if (!AFF_FLAGGED(CARRYING(ch), AFF_KNOCKED) &&
        !is_affected(CARRYING(ch), AFF_SLEEP) && rand_number(1, 3)) {
      send_to_char(CARRYING(ch),
                   "You feel your sleeping body being moved.\r\n");
    }
  }

  char buf3[MAX_STRING_LENGTH];
  send_to_sense(0, "You sense someone ", ch);
  sprintf(buf3,
          "@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection "
          "range.@n",
          add_commas(GET_HIT(ch)));
  send_to_scouter(buf3, ch, 0, 0);

  if (ch->desc != NULL) {
    act(obj->action_description, TRUE, ch, obj, 0, TO_CHAR);
    look_at_room(char_room_get(ch), ch, 0);
  }

  if ((char_room_get(ch) && room_flagged(char_room_get(ch), ROOM_DEATH)) &&
      !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch);
    return 0;
  }

  entry_memory_mtrigger(ch);
  greet_memory_mtrigger(ch);

  return 1;
}

static int perform_leave_obj(struct char_data *ch, struct obj_data *obj,
                             int need_specials_check) {
  struct room_data *was_in_room = char_room_get(ch);
  int could_move = FALSE;

  if (GRAPPLING(ch) || GRAPPLED(ch)) {
    send_to_char(ch, "You are grappling with someone!\r\n");
    return (0);
  }

  if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
    send_to_char(ch, "But the way out is closed.\r\n");
  } else {
    if (GET_OBJ_VAL(obj, VAL_HATCH_DEST) != NOWHERE)
      if ((could_move = do_simple_leave(ch, obj, need_specials_check)))
        char_followers_iterate(ch, [&](struct char_data *k) {
          if ((char_room_get(k) == was_in_room) && (GET_POS(k) >= POS_STANDING)) {
            act("You follow $N.\r\n", FALSE, k, 0, ch, TO_CHAR);
            perform_leave_obj(k, obj, 1);
          }
          return true;
        });
  }
  return could_move;
}

ACMD(do_leave) {}

void handle_fall(struct char_data *ch) {
  int room = -1;
  while (EXIT(ch, 5) &&
         room_sector_type_get(char_room_get(ch)) == SECT_FLYING) {
    room = exit_to_room_vnum_get(EXIT(ch, 5));
    char_from_room(ch);
    char_to_room(ch, room_by_id(room));
    if (CARRYING(ch)) {
      char_from_room(CARRYING(ch));
      char_to_room(CARRYING(ch), room_by_id(room));
    }
    if (!EXIT(ch, 5) ||
        room_sector_type_get(char_room_get(ch)) != SECT_FLYING) {
      act("@r$n slams into the ground!@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurHealthFloored(ch, getMaxPL(ch) / 20, 1);

      act("@rYou slam into the ground!@n", TRUE, ch, 0, 0, TO_CHAR);
      look_at_room(char_room_get(ch), ch, 0);
    } else {
      act("@r$n pummets down toward the ground below!@n", TRUE, ch, 0, 0,
          TO_ROOM);
    }
  }
  if (room_sector_type_get(char_room_get(ch)) == SECT_WATER_NOSWIM &&
      !CARRIED_BY(ch) && !IS_KANASSAN(ch)) {
    if ((getCurST(ch)) >= (getCurCarriedWeight(ch))) {
      act("@bYou swim in place.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@b swims in place.@n", TRUE, ch, 0, 0, TO_ROOM);
      decCurST(ch, getCurCarriedWeight(ch));
      act("@RYou are drowning!@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@b gulps water as $e struggles to stay above the water line.@n",
          TRUE, ch, 0, 0, TO_ROOM);
      if (GET_HIT(ch) - ((getMaxPL(ch)) / 3) <= 0) {
        act("@rYou drown!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@R$n@r drowns!@n", TRUE, ch, 0, 0, TO_ROOM);
        die(ch, NULL);
        decCurHealthPercentFloored(ch, 1, 1);
      } else {
        decCurHealthPercent(ch, .33);
      }
    }
  }
}

static int check_swim(struct char_data *ch) {
  bool can = false;

  if ((char_room_get(ch) && room_flagged(char_room_get(ch), ROOM_SPACE))) {
    int64_t space_cost =
        (GET_MAX_MANA(ch) / 1000) + ((getCurCarriedWeight(ch)) / 2);
    if (getCurKI(ch) >= space_cost)
      can = true;
    decCurKI(ch, space_cost);
    if (!can)
      send_to_char(ch, "You do not have enough ki to fly through space. You "
                       "are drifting helplessly.\r\n");
    return can;
  } else {
    int64_t swim_cost = (getCurCarriedWeight(ch)) - 1;
    if (getCurST(ch) >= swim_cost)
      can = true;
    decCurST(ch, swim_cost);
    if (!can)
      send_to_char(ch, "You are too tired to swim!\r\n");
    return can;
  }
}

ACMD(do_fly) {}

ACMD(do_follow) {}
