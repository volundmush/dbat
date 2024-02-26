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
#include "dbat/constants.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/act.informative.h"
#include "dbat/entity.h"

#ifndef EXITN
#  define EXITN(room, door)        (world[room].dir_option[door])
#endif

static int ship_land_location(Character *ch, Object *vehicle, char *arg);

static void disp_ship_locations(Character *ch, Object *vehicle);

/* This shows the player what locations the planet has to land at. */
static void disp_ship_locations(Character *ch, Object *vehicle) {
    auto r = vehicle->getRoom();
    if (r->getUID() == 50) { // Above Earth
        ch->sendf("@D------------------[ @GEarth@D ]------------------@c\n");
        ch->sendf("Nexus City, South Ocean, Nexus field, Cherry Blossom Mountain,\n");
        ch->sendf("Sandy Desert, Northern Plains, Korin's Tower, Kami's Lookout,\n");
        ch->sendf("Shadow Forest, Decrepit Area, West City, Hercule Beach, Satan City.\n");
        ch->sendf("@D---------------------------------------------@n\n");
    } else if (r->getUID() == 51) { // Above Frigid
        ch->sendf("@D------------------[ @CFrigid@D ]------------------@c\n");
        ch->sendf("Ice Crown City, Ice Highway, Topica Snowfield, Glug's Volcano,\n");
        ch->sendf("Platonic Sea, Slave City, Acturian Woods, Desolate Demesne,\n");
        ch->sendf("Chateau Ishran, Wyrm Spine Mountain, Cloud Ruler Temple, Koltoan mine.\n");
        ch->sendf("@D---------------------------------------------@n\n");
    } else if (r->getUID() == 52) { // Above Konack
        ch->sendf("@D------------------[ @MKonack@D ]------------------@c\n");
        ch->sendf("Great Oroist Temple, Elzthuan Forest, Mazori Farm, Dres,\n");
        ch->sendf("Colvian Farm, St Alucia, Meridius Memorial, Desert of Illusion,\n");
        ch->sendf("Plains of Confusion, Turlon Fair, Wetlands, Kerberos,\n");
        ch->sendf("Shaeras Mansion, Slavinus Ravine, Furian Citadel.\n");
        ch->sendf("@D---------------------------------------------@n\n");
    } else if (r->getUID() == 53) { // Above Vegeta
        ch->sendf("@D------------------[ @YVegeta@D ]------------------@c\n");
        ch->sendf("Vegetos City, Blood Dunes, Ancestral Mountains, Destopa Swamp,\n");
        ch->sendf("Pride Forest, Pride tower, Ruby Cave.\n");
        ch->sendf("@D---------------------------------------------@n\n");
    } else if (r->getUID() == 198) { // Above Cerria
        ch->sendf("@D------------------[ @MCerria@D ]------------------@c\n");
        ch->sendf("Cerria Colony, Fistarl Volcano, Crystalline Forest.\n");
        ch->sendf("@D---------------------------------------------@n\n");
    } else if (r->getUID() == 54) { // Above Namek
        ch->sendf("@D------------------[ @gNamek@D ]------------------@c\n");
        ch->sendf("Senzu Village, Guru's House, Crystalline Cave, Elder Village,\n");
        ch->sendf("Frieza's Ship, Kakureta Village.\n");
        ch->sendf("@D---------------------------------------------@n\n");
    } else if (r->getUID() == 55) { // Above Aether
        ch->sendf("@D------------------[ @BAether@D ]-----------------@c\n");
        ch->sendf("Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n");
        ch->sendf("Silent Glade.\n");
        ch->sendf("@D--------------------------------------------@n\n");
        ch->sendf("@D------------------[ @BAether@D ]-----------------@c\n");
        ch->sendf("Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n");
        ch->sendf("Silent Glade.\n");
        ch->sendf("@D--------------------------------------------@n\n");
    } else if (r->getUID() == 56) { // Above Yardrat
        ch->sendf("@D-----------------[ @mYardrat@D ]-----------------@c\n");
        ch->sendf("Yardra City, Jade Forest, Jade Cliffs, Mount Valaria.\n");
        ch->sendf("@D-------------------------------------------@n\n");
    } else if (r->getUID() == 57) { // Above Zennith
        ch->sendf("@D-----------------[ @CZennith@D ]-----------------@c\n");
        ch->sendf("Utatlan City, Zenith Jungle, Ancient Castle.\n");
        ch->sendf("@D-------------------------------------------@n\n");
    } else if (r->getUID() == 58) { // Above Kanassa
        ch->sendf("@D-----------------[ @CKanassa@D ]-----------------@c\n");
        ch->sendf("Aquis City, Yunkai Pirate Base.\n");
        ch->sendf("@D-------------------------------------------@n\n");
    } else if (r->getUID() == 59) { // Above Arlia
        ch->sendf("@D------------------[ @MArlia@D ]------------------@c\n");
        ch->sendf("Janacre, Arlian Wasteland, Arlia Mine, Kemabra Wastes.\n");
        ch->sendf("@D---------------------------------------------@n\n");
    } else {
        ch->sendf("You are not above a planet!\r\n");
    }
}

static int ship_land_location(Character *ch, Object *vehicle, char *arg) {
    int landspot = 50;
    auto r = vehicle->getRoom();
    if (r->getUID() == 50) { // Above Earth
        if (!strcasecmp(arg, "Nexus City")) {
            landspot = 300;
            landspot += rand_number(0, 63);
            return (landspot);
        } else if (!strcasecmp(arg, "South Ocean")) {
            landspot = 800;
            landspot += rand_number(0, 99);
            return (landspot);
        } else if (!strcasecmp(arg, "Nexus Field")) {
            landspot = 1150;
            landspot += rand_number(-16, 28);
            return (landspot);
        } else if (!strcasecmp(arg, "Cherry Blossom Mountain")) {
            landspot = 1180;
            landspot += rand_number(0, 19);
            return (landspot);
        } else if (!strcasecmp(arg, "Sandy Desert")) {
            landspot = 1287;
            landspot += rand_number(0, 64);
            return (landspot);
        } else if (!strcasecmp(arg, "Northern Plains")) {
            landspot = 1428;
            landspot += rand_number(0, 55);
            return (landspot);
        } else if (!strcasecmp(arg, "Korin's Tower")) {
            return (1456);
        } else if (!strcasecmp(arg, "Kami's Lookout")) {
            landspot = 1506;
            landspot += rand_number(0, 30);
            return (landspot);
        } else if (!strcasecmp(arg, "Shadow Forest")) {
            landspot = 1600;
            landspot += rand_number(0, 66);
            return (landspot);
        } else if (!strcasecmp(arg, "Decrepit Area")) {
            return (1710);
        } else if (!strcasecmp(arg, "West City")) {
            return (19510);
        } else if (!strcasecmp(arg, "Hercule Beach")) {
            landspot = 2141;
            landspot += rand_number(0, 53);
            return (landspot);
        } else if (!strcasecmp(arg, "Satan City")) {
            landspot = 1150;
            landspot += rand_number(-16, 28);
            return (landspot);
            return (13020);
        } else {
            ch->sendf("You don't know where that made up place is, but decided to land anyway.");
            return (300);
        }
    } else if (r->getUID() == 51) { // Above Frigid
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
            ch->sendf("You don't know where that made up place is, but decided to land anyway.");
            return (4264);
        }
    } else if (r->getUID() == 52) { // Above Konack
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
            ch->sendf("you don't know where that made up place is, but decided to land anyway.");
            return (8006);
        }
    } else if (r->getUID() == 53) { // Above Vegeta
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
            ch->sendf("you don't know where that made up place is, but decided to land anyway.");
            return (2226);
        }
    } else if (r->getUID() == 54) { // Above Namek
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
            ch->sendf("you don't know where that made up place is, but decided to land anyway.");
            return (11600);
        }
    } else if (r->getUID() == 55) { // Above Aether
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
            ch->sendf("you don't know where that made up place is, but decided to land anyway.");
            return (12010);
        }
    } else if (r->getUID() == 56) { // Above Yardrat
        if (!strcasecmp(arg, "Yardra City")) {
            return (14008);
        } else if (!strcasecmp(arg, "Jade Forest")) {
            return (14100);
        } else if (!strcasecmp(arg, "Jade Cliffs")) {
            return (14200);
        } else if (!strcasecmp(arg, "Mount Valaria")) {
            return (14300);
        } else {
            ch->sendf("you don't know where that made up place is, but decided to land anyway.");
            return (14008);
        }
    } else if (r->getUID() == 198) { // Above Cerria
        if (!strcasecmp(arg, "Cerria Colony")) {
            return (17531);
        } else if (!strcasecmp(arg, "Crystalline Forest")) {
            return (7950);
        } else if (!strcasecmp(arg, "Fistarl Volcano")) {
            return (17420);
        } else {
            ch->sendf("you don't know where that made up place is, but decided to land anyway.");
            return (17531);
        }
    } else if (r->getUID() == 57) { // Above Zennith
        if (!strcasecmp(arg, "Utatlan City")) {
            return (3412);
        } else if (!strcasecmp(arg, "Zenith Jungle")) {
            return (3520);
        } else if (!strcasecmp(arg, "Ancient Castle")) {
            return (19600);
        } else {
            ch->sendf("you don't know where that made up place is, but decided to land anyway.");
            return (3412);
        }
    } else if (r->getUID() == 58) { // Above Kanassa
        if (!strcasecmp(arg, "Aquis City")) {
            return (14904);
        } else if (!strcasecmp(arg, "Yunkai Pirate Base")) {
            return (15655);
        } else {
            ch->sendf("you don't know where that made up place is, but decided to land anyway.");
            return (14904);
        }
    } else if (r->getUID() == 59) { // Above Arlia
        if (!strcasecmp(arg, "Janacre")) {
            return (16009);
        } else if (!strcasecmp(arg, "Arlian Wasteland")) {
            return (16544);
        } else if (!strcasecmp(arg, "Arlia Mine")) {
            return (16600);
        } else if (!strcasecmp(arg, "Kemabra Wastes")) {
            return (16816);
        } else {
            ch->sendf("you don't know where that made up place is, but decided to land anyway.");
            return (16009);
        }
    } else {
        ch->sendf("You are not above a planet!\r\n");
        return (-1);
    }
}

Object *find_vehicle_by_vnum(int vnum) {
    auto o = get_last_inserted(objectVnumIndex, vnum);
    if(o && GET_OBJ_TYPE(o) == ITEM_VEHICLE) return o;
    return nullptr;
}

Object *find_hatch_by_vnum(int vnum) {
    auto o = get_last_inserted(objectVnumIndex, vnum);
    if(o && GET_OBJ_TYPE(o) == ITEM_HATCH) return o;
    return nullptr;
}

/* Search the player's room, inventory and equipment for a control */
Object *find_control(Character *ch) {
    return nullptr;
}

/* Drive our vehicle into another vehicle */
static void drive_into_vehicle(Character *ch, Object *vehicle, char *arg) {
    return;

}

/* Drive our vehicle out of another vehicle */
static void drive_outof_vehicle(Character *ch, Object *vehicle) {
    return;

}

/* Drive out vehicle in a certain direction */
void drive_in_direction(Character *ch, Object *vehicle, int dir) {
    char buf[MAX_INPUT_LENGTH];
    auto room = vehicle->getRoom();

    auto exits = room->getExits();

    auto d = exits[dir];
    if(!d) {
        ch->sendf("@wApparently %s doesn't exist there.\r\n", dirs[dir]);
        return;
    }
    auto dest = reg.try_get<Destination>(d->ent);
    if(!dest) {
        ch->sendf("@wApparently %s doesn't exist there.\r\n", dirs[dir]);
        return;
    }

    if(d->checkFlag(FlagType::Exit, EX_CLOSED)) {
        if (!d->getAlias().empty())
            ch->sendf("@wThe %s seems to be closed.\r\n", fname(d->getAlias().c_str()));
        else
            ch->sendf("@wIt seems to be closed.\r\n");
        return;
    }

    if (!flags::check(dest->target, FlagType::Room, ROOM_VEHICLE) && !flags::check(dest->target, FlagType::Room, ROOM_SPACE)) {
        /* But the vehicle can't go that way*/
        ch->sendf("@wThe ship can't fit there!\r\n");
        return;
    }

    int was_in, is_in;

    sprintf(buf, "%s @wflies %s.\r\n", vehicle->getShortDesc().c_str(), dirs[dir]);
    send_to_room(IN_ROOM(vehicle), buf);

    vehicle->removeFromLocation();
    vehicle->addToLocation(*dest);
    Object *controls;
    if ((controls = find_control(ch))) {
        if (GET_FUELCOUNT(controls) < 5) {
            GET_FUELCOUNT(controls) += 1;
        } else {
            GET_FUELCOUNT(controls) = 0;
            GET_FUEL(controls) -= 1;
            if (GET_FUEL(controls) < 0) {
                GET_FUEL(controls) = 0;
            }
        }
    }

    Object *hatch = nullptr;

    for (auto hatch : getEntity(real_room(GET_OBJ_VAL(vehicle, 0)))->getInventory()) {
        if (GET_OBJ_TYPE(hatch) == ITEM_HATCH) {
            GET_OBJ_VAL(hatch, 3) = GET_ROOM_VNUM(IN_ROOM(vehicle));
        }
    }

    is_in = IN_ROOM(vehicle);

    if (ch->desc != nullptr)
        act("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.", true, ch, nullptr, nullptr,
            TO_ROOM);
    ch->sendf("@wThe ship flies onward:\r\n");
    ch->sendText(vehicle->getRoom()->renderLocationFor(ch));
    if (controls) {
        ch->sendf("@RFUEL@D: %s%s@n\r\n",
                     GET_FUEL(controls) >= 200 ? "@G" : GET_FUEL(controls) >= 100 ? "@Y" : "@r",
                     add_commas(GET_FUEL(controls)).c_str());
    }
    int door;
    room = vehicle->getRoom();
    for (auto &[door, e] : room->getExits()) {
        dest = reg.try_get<Destination>(e->ent);
        if(!dest) continue;
        if(!e->checkFlag(FlagType::Exit, EX_CLOSED)) continue;

        send::printfContents(dest->target, "@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.\r\n");
    }
    sprintf(buf, "%s @wflies in from the %s.\r\n",
            vehicle->getShortDesc().c_str(), dirs[rev_dir[dir]]);

    send_to_room(is_in, buf);

}


ACMD(do_drive) {
    int dir, confirmed = false, count = 0;
    Object *vehicle, *controls;
    char arg3[MAX_INPUT_LENGTH];

    one_argument(argument, arg3);

    if (!HAS_ARMS(ch)) {
        ch->sendf("You have no arms!\r\n");
        return;
    }

    if (!strcasecmp(arg3, "unready") && !IS_NPC(ch)) {
        if (!PLR_FLAGGED(ch, PLR_PILOTING)) {
            ch->sendf("You are already not flying the ship!\r\n");
            return;
        } else if (PLR_FLAGGED(ch, PLR_PILOTING)) {
            act("@w$n stands up and stops piloting the ship.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->sendf("@wYou stand up from the pilot's seat.\r\n");
            GET_POS(ch) = POS_STANDING;
            ch->clearFlag(FlagType::PC, PLR_PILOTING);
            return;
        }
    }
    if (!strcasecmp(arg3, "ready") && !IS_NPC(ch)) {
        if (!(controls = find_control(ch))) {
            ch->sendf("@wYou have nothing to control here!\r\n");
            return;
        }
        struct descriptor_data *d;
        if (PLR_FLAGGED(ch, PLR_PILOTING)) {
            ch->sendf("@wYou are already piloting the ship, try [pilot unready].\r\n");
            return;
        }
        if (CARRYING(ch)) {
            ch->sendf("@wYou are busy carrying someone.\r\n");
            return;
        }
        if (DRAGGING(ch)) {
            ch->sendf("@wYou are busy dragging someone.\r\n");
            return;
        }
        for (d = descriptor_list; d; d = d->next) {
            if (!IS_PLAYING(d)) {
                continue;
            }
            if (d->character == ch) {
                continue;
            }
            if (PLR_FLAGGED(d->character, PLR_PILOTING) && IN_ROOM(d->character) == IN_ROOM(ch)) {
                ch->sendf("@w%s is already piloting the ship!\r\n", GET_NAME(d->character));
                count = 1;
                return;
            }
        }
        if (count == 0) {
            confirmed = true;
        }
    }
    if (confirmed == true) {
        ch->setFlag(FlagType::PC, PLR_PILOTING);
        act("@w$n sits down and begins piloting the ship.", true, ch, nullptr, nullptr, TO_ROOM);
        GET_POS(ch) = POS_SITTING;
        ch->sendf("@wYou take a seat in the pilot's chair.\r\n");
        return;
    } else if (!PLR_FLAGGED(ch, PLR_PILOTING)) {
        ch->sendf("@wYou need to be seated in the pilot's seat.\r\n[Enter: Pilot ready/unready]\r\n");
    } else if (GET_POS(ch) < POS_SLEEPING) {
        ch->sendf("@wYou can't see anything but stars!\r\n");
    } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
        ch->sendf("@wYou can't see a damned thing, you're blind!\r\n");
    } else if (ch->getRoom()->isInsideDark(ch) && !CAN_SEE_IN_DARK(ch)) {
        ch->sendf("@wIt is pitch black...\r\n");
    } else if (!(controls = find_control(ch))) {
        ch->sendf("@wYou have nothing to control here!\r\n");
    } else if (invalid_align(ch, controls) ||
               invalid_class(ch, controls) ||
               invalid_race(ch, controls)) {
        act("@wYou are zapped by $p@w and instantly step away from it.", false, ch, controls, nullptr, TO_CHAR);
        act("@w$n@w is zapped by $p@w and instantly steps away from it.", false, ch, controls, nullptr, TO_ROOM);
    } else if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0)))) {
        ch->sendf("@wYou can't find anything to pilot.\r\n");
    } else {

        char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

        /*argument = any_one_arg(argument, arg);*/
        half_chop(argument, arg, arg2);

        if (GET_FUEL(controls) <= 0) {
            ch->sendf("Your ship doesn't have enough fuel to move.\r\n");
            return;
        }

        if (!*arg) {
            ch->sendf("@wPilot, yes, but where?\r\n");
        } else if (is_abbrev(arg, "into") ||
                   is_abbrev(arg, "onto")) {
            /* Driving into another vehicle */
            drive_into_vehicle(ch, vehicle, arg2);
        } else if (is_abbrev(arg, "out") && !EXIT(vehicle, OUTDIR)) {
            drive_outof_vehicle(ch, vehicle);
        } else {
            if (!OBJVAL_FLAGGED(vehicle, CONT_CLOSED)) {
                ch->sendf("@wThe hatch is open, are you insane!?\r\n");
                return;
            }

            if (!strcasecmp(arg, "north") || !strcasecmp(arg, "n")) {
                dir = 0;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "east") || !strcasecmp(arg, "e")) {
                dir = 1;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "south") || !strcasecmp(arg, "s")) {
                dir = 2;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "west") || !strcasecmp(arg, "w")) {
                dir = 3;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "up") || !strcasecmp(arg, "u")) {
                dir = 4;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "down") || !strcasecmp(arg, "d")) {
                dir = 5;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "northwest") || !strcasecmp(arg, "nw") || !strcasecmp(arg, "northw")) {
                dir = 6;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "northeast") || !strcasecmp(arg, "ne") || !strcasecmp(arg, "northe")) {
                dir = 7;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "southeast") || !strcasecmp(arg, "se") || !strcasecmp(arg, "southe")) {
                dir = 8;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "southwest") || !strcasecmp(arg, "sw") || !strcasecmp(arg, "southw")) {
                dir = 9;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "inside")) {
                dir = 10;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            } else if (!strcasecmp(arg, "outside")) {
                dir = 11;
                drive_in_direction(ch, vehicle, dir);
                if (GET_OBJ_VAL(controls, 1) == 1) {
                    WAIT_STATE(ch, PULSE_2SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 2) {
                    WAIT_STATE(ch, PULSE_2SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 3) {
                    WAIT_STATE(ch, PULSE_1SEC * 1.5);
                } else if (GET_OBJ_VAL(controls, 1) == 4) {
                    WAIT_STATE(ch, PULSE_1SEC);
                } else if (GET_OBJ_VAL(controls, 1) == 5) {
                    WAIT_STATE(ch, PULSE_1SEC * 0.5);
                }
            }
                /*else if (dir = search_block(arg, dirs, FALSE) >= 0) {*/
                /* Drive in a direction... */
                /*}*/
            else if (!strcasecmp(arg, "land")) {

                if (!*arg2) {
                    if (GET_OBJ_VNUM(vehicle) >= 46000 && GET_OBJ_VNUM(vehicle) <= 46099) {
                        ch->sendf("@wLand on which pad? 1, 2, 3 or 4?\r\n");
                        ch->sendf(
                                     "@CSpecial Ship Ability@D: @wpilot land (area name)\n@GExample@D: @wpilot land Nexus City\r\n");
                        disp_ship_locations(ch, vehicle);
                    } else {
                        ch->sendf("@wLand on which pad? 1, 2, 3 or 4?\r\n");
                    }
                    return;
                }
                char blah[MAX_INPUT_LENGTH];
                int land_location = 50;
                if (GET_OBJ_VNUM(vehicle) > 46099) {
                    if (strcasecmp(arg2, "1") && strcasecmp(arg2, "2") && strcasecmp(arg2, "3") &&
                        strcasecmp(arg2, "4")) {
                        ch->sendf("@wLand on which pad? 1, 2, 3 or 4?\r\n");
                        return;
                    }
                } else if (strcasecmp(arg2, "1") && strcasecmp(arg2, "2") && strcasecmp(arg2, "3") &&
                           strcasecmp(arg2, "4")) {
                    land_location = ship_land_location(ch, vehicle, arg2);
                }
                char buf3[MAX_INPUT_LENGTH];
                *buf3 = '\0';

                /* For Earth */
                act("@wYou set the controls to descend.@n", false, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n @wmanipulates the ship controls.@n", false, ch, nullptr, nullptr, TO_ROOM);
                act("@RThe ship rocks and shakes as it descends through the atmosphere!@n", false, ch, nullptr, nullptr,
                    TO_CHAR);
                act("@RThe ship rocks and shakes as it descends through the atmosphere!@n", false, ch, nullptr, nullptr,
                    TO_ROOM);
                if (land_location <= 50) {
                    act("@wThe ship has landed.@n", false, ch, nullptr, nullptr, TO_CHAR);
                    act("@wThe ship has landed.@n", false, ch, nullptr, nullptr, TO_ROOM);
                }
                if (land_location > 50) {
                    act("@wThe ship slams into the ground and forms a small crater!@n", false, ch, nullptr, nullptr,
                        TO_CHAR);
                    act("@wThe ship slams into the ground and forms a small crater!@n", false, ch, nullptr, nullptr,
                        TO_ROOM);
                    vehicle->removeFromLocation();
                    vehicle->addToLocation(getEntity(land_location));
                } else if (IN_ROOM(vehicle) == real_room(50)) {
                    if (!strcasecmp(arg2, "1")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(409));
                    } else if (!strcasecmp(arg2, "2")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(411));
                    } else if (!strcasecmp(arg2, "3")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(412));
                    } else if (!strcasecmp(arg2, "4")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(410));
                    } else if (!strcasecmp(arg2, "4365")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(18904));
                    } else if (!strcasecmp(arg2, "6329")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(18925));
                    } else if (!strcasecmp(arg2, "1983")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(18995));
                    } else {
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_ROOM);
                    }
                }
                    /* For Zenith */
                else if (IN_ROOM(vehicle) == real_room(57)) {
                    vehicle->removeFromLocation();
                    vehicle->addToLocation(getEntity(3508));
                }
                    /* For Cerria*/
                else if (IN_ROOM(vehicle) == real_room(198)) {
                    vehicle->removeFromLocation();
                    vehicle->addToLocation(getEntity(17420));
                }
                    /* For Kanassa */
                else if (IN_ROOM(vehicle) == real_room(58)) {
                    vehicle->removeFromLocation();
                    vehicle->addToLocation(getEntity(14904));
                }
                    /* For Vegeta */
                else if (IN_ROOM(vehicle) == real_room(53)) {
                    if (!strcasecmp(arg2, "1")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(2319));
                    } else if (!strcasecmp(arg2, "2")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(2318));
                    } else if (!strcasecmp(arg2, "3")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(2320));
                    } else if (!strcasecmp(arg2, "4")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(2322));
                    } else if (!strcasecmp(arg2, "4126")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(18212));
                    } else {
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_ROOM);
                    }
                }

                    /* For Yardrat */
                else if (IN_ROOM(vehicle) == real_room(56)) {
                    if (!strcasecmp(arg2, "1")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(14003));
                    } else if (!strcasecmp(arg2, "2")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(14004));
                    } else if (!strcasecmp(arg2, "3")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(14005));
                    } else if (!strcasecmp(arg2, "4")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(14006));
                    } else {
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_ROOM);
                    }
                }

                    /* For Aether */
                else if (IN_ROOM(vehicle) == real_room(55)) {
                    if (!strcasecmp(arg2, "1")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(12003));
                    } else if (!strcasecmp(arg2, "2")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(12004));
                    } else if (!strcasecmp(arg2, "3")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(12006));
                    } else if (!strcasecmp(arg2, "4")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(12005));
                    } else {
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_ROOM);
                    }
                }

                    /* For Arlia */
                else if (IN_ROOM(vehicle) == real_room(59)) {
                    if (!strcasecmp(arg2, "1")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(16065));
                    } else if (!strcasecmp(arg2, "2")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(16066));
                    } else if (!strcasecmp(arg2, "3")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(16067));
                    } else if (!strcasecmp(arg2, "4")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(16068));
                    } else {
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_ROOM);
                    }
                }


                    /* For Frigid */
                else if (IN_ROOM(vehicle) == real_room(51)) {
                    if (!strcasecmp(arg2, "1")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(4264));
                    } else if (!strcasecmp(arg2, "2")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(4263));
                    } else if (!strcasecmp(arg2, "3")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(4261));
                    } else if (!strcasecmp(arg2, "4")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(4262));
                    } else if (!strcasecmp(arg2, "1337")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(18116));
                    } else {
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_ROOM);
                    }
                }
                    /* For Namek */
                else if (IN_ROOM(vehicle) == real_room(54)) {
                    if (!strcasecmp(arg2, "1")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(11628));
                    } else if (!strcasecmp(arg2, "2")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(11629));
                    } else if (!strcasecmp(arg2, "3")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(11630));
                    } else if (!strcasecmp(arg2, "4")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(11627));
                    } else {
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_ROOM);
                    }
                }
                    /* For Konack */
                else if (IN_ROOM(vehicle) == real_room(52)) {
                    if (!strcasecmp(arg2, "1")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(8195));
                    } else if (!strcasecmp(arg2, "2")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(8196));
                    } else if (!strcasecmp(arg2, "3")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(8197));
                    } else if (!strcasecmp(arg2, "4")) {
                        vehicle->removeFromLocation();
                        vehicle->addToLocation(getEntity(8198));
                    } else {
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_CHAR);
                        act("@wLanding sequence aborted, improper coordinates.@n", false, ch, nullptr, nullptr,
                            TO_ROOM);
                    }
                } else {
                    ch->sendf("@wYou are not where you can land, you need to be in a planet's low orbit.@n\r\n");
                }
                auto room = vehicle->getRoom();
                if (land_location <= 50) {
                    sprintf(buf3, "%s @wcomes in from above and slowly settles on the launch-pad.@n\r\n",
                            vehicle->getShortDesc().c_str());
                    ch->sendText(room->renderLocationFor(ch));
                    send_to_room(IN_ROOM(vehicle), buf3);
                } else {
                    sprintf(buf3, "%s @wcomes in from above and slams into the ground!@n\r\n",
                            vehicle->getShortDesc().c_str());
                    
                    room->dmg += 1;
                    if (room->dmg >= 10) {
                        room->dmg = 10;
                    }
                    ch->lookAtLocation();
                    send_to_room(IN_ROOM(vehicle), buf3);
                }
            } else if (!strcasecmp(arg, "launch")) {
                int lnum = 0;
                int rnum = 0;
                auto room = vehicle->getRoom();
                auto dest = movement::getLaunchDestinationFor(room->ent, vehicle->ent);
                if(!dest) {
                    ch->sendf("@wYou are not on a planet.@n\r\n");
                    return;
                }
                act("@wYou set the controls to launch.@n", false, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n @wmanipulates the ship controls.@n", false, ch, nullptr, nullptr, TO_ROOM);
                act("@RThe ship shudders as it launches up into the sky!@n", false, ch, nullptr, nullptr, TO_CHAR);
                act("@RThe ship shudders as it launches up into the sky!@n", false, ch, nullptr, nullptr, TO_ROOM);
                act("@wThe ship has reached low orbit.@n", false, ch, nullptr, nullptr, TO_CHAR);
                act("@wThe ship has reached low orbit.@n", false, ch, nullptr, nullptr, TO_ROOM);
                vehicle->getRoom()->sendfContents("@R%s @Rshudders before blasting off into the sky!@n",
                             vehicle->getShortDesc().c_str());
                if (GET_FUELCOUNT(controls) < 5) {
                    GET_FUELCOUNT(controls) += 1;
                } else {
                    GET_FUELCOUNT(controls) = 0;
                    GET_FUEL(controls) -= 1;
                    if (GET_FUEL(controls) < 0) {
                        GET_FUEL(controls) = 0;
                    }
                }
                vehicle->removeFromLocation();
                vehicle->addToLocation(dest.value());
                //vehicle->lookAtLocation();
                ch->sendText(vehicle->getRoom()->renderLocationFor(ch));
                ch->sendf("@RFUEL@D: %s%s@n\r\n",
                             GET_FUEL(controls) >= 200 ? "@G" : GET_FUEL(controls) >= 100 ? "@Y" : "@r",
                             add_commas(GET_FUEL(controls)).c_str());
            } else if (!strcasecmp(arg, "mark")) {
                int rnum = 0;
                if (!*arg2) {
                    ch->sendf("@wWhich marker are you wanting to launch? 1, 2, or 3?\r\n");
                    return;
                }
                if (!!strcasecmp(arg2, "1") && !!strcasecmp(arg2, "2") && !!strcasecmp(arg2, "3")) {
                    ch->sendf("@wWhich marker are you wanting to launch? 1, 2, or 3?\r\n");
                    return;
                }

                if (!ROOM_FLAGGED(IN_ROOM(vehicle), ROOM_SPACE)) {
                    ch->sendf("@wYou need to be in space to launch a marker buoy.\r\n");
                    return;
                }

                rnum = IN_ROOM(vehicle);

                if (GET_RADAR1(ch) > 0 && !strcasecmp(arg2, "1")) {
                    ch->sendf("@wYou need to 'deactivate' that marker.\r\n");
                    return;
                } else if (GET_RADAR1(ch) <= 0 && !strcasecmp(arg2, "1")) {
                    act("@wYou enter a unique code and launch a marker buoy.@n\r\n", false, ch, nullptr, nullptr,
                        TO_CHAR);
                    act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);
                    GET_RADAR1(ch) = GET_ROOM_VNUM(IN_ROOM(vehicle));
                }
                if (GET_RADAR2(ch) > 0 && !strcasecmp(arg2, "2")) {
                    ch->sendf("@wYou need to 'deactivate' that marker.\r\n");
                    return;
                } else if (GET_RADAR2(ch) <= 0 && !strcasecmp(arg2, "2")) {
                    act("@wYou enter a unique code and launch a marker buoy.@n\r\n", false, ch, nullptr, nullptr,
                        TO_CHAR);
                    act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);
                    GET_RADAR2(ch) = GET_ROOM_VNUM(IN_ROOM(vehicle));
                }
                if (GET_RADAR3(ch) > 0 && !strcasecmp(arg2, "3")) {
                    ch->sendf("@wYou need to 'deactivate' that marker.\r\n");
                    return;
                } else if (GET_RADAR3(ch) <= 0 && !strcasecmp(arg2, "3")) {
                    act("@wYou enter a unique code and launch a marker buoy.@n\r\n", false, ch, nullptr, nullptr,
                        TO_CHAR);
                    act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);
                    GET_RADAR3(ch) = GET_ROOM_VNUM(IN_ROOM(vehicle));
                }
            } else if (!strcasecmp(arg, "deactivate")) {
                if (!*arg2) {
                    ch->sendf("@wWhich marker are you wanting to launch? 1, 2, or 3?\r\n");
                    return;
                }
                if (!!strcasecmp(arg2, "1") && !!strcasecmp(arg2, "2") && !!strcasecmp(arg2, "3")) {
                    ch->sendf("@wWhich marker are you wanting to deactivate? 1, 2, or 3?\r\n");
                    return;
                }

                if (GET_RADAR1(ch) <= 0 && !strcasecmp(arg2, "1")) {
                    ch->sendf("@wYou haven't launched that buoy yet.\r\n");
                    return;
                } else if (GET_RADAR1(ch) > 0 && !strcasecmp(arg2, "1")) {
                    act("@wYou enter buoy one's code and command it to deactivate.@n\r\n", false, ch, nullptr, nullptr,
                        TO_CHAR);
                    act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);
                    GET_RADAR1(ch) = 0;
                }
                if (GET_RADAR2(ch) <= 0 && !strcasecmp(arg2, "2")) {
                    ch->sendf("@wYou haven't launched that buoy yet.\r\n");
                    return;
                } else if (GET_RADAR2(ch) > 0 && !strcasecmp(arg2, "2")) {
                    act("@wYou enter buoy two's code and command it to deactivate.@n\r\n", false, ch, nullptr, nullptr,
                        TO_CHAR);
                    act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);
                    GET_RADAR2(ch) = 0;
                }
                if (GET_RADAR3(ch) <= 0 && !strcasecmp(arg2, "3")) {
                    ch->sendf("@wYou haven't launched that buoy yet.\r\n");
                    return;
                } else if (GET_RADAR3(ch) > 0 && !strcasecmp(arg2, "3")) {
                    act("@wYou enter buoy three's code and command it to deactivate.@n\r\n", false, ch, nullptr,
                        nullptr, TO_CHAR);
                    act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);
                    GET_RADAR3(ch) = 0;
                }
            } else {
                ch->sendf("@wThats not a valid direction.\r\n");
                ch->sendf("Try one of these.\r\n");
                ch->sendf("[ north/n  | south/s  | east/e  |  west/w  ]\r\n");
                ch->sendf("[ up/u | down/d | northeast/ne/northe | northwest/nw/northw]\r\n");
                ch->sendf("[  southeast/se/southe  |  southwest/sw/southw]\r\n");
                ch->sendf("[  into  |  onto  |  inside  |  outside  ]@n\r\n");
                ch->sendf("[ land | launch ]@n\r\n");
            }
        }
    }
}

ACMD(do_ship_fire) {
    Object *vehicle, *controls;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!(controls = find_control(ch))) {
        ch->sendf("@wYou must be near the comm station in the cockpit.\r\n");
        return;
    }

    if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0)))) {
        ch->sendf("@wSomething cosmic is jamming your signal! Quick call Iovan to repair it!\r\n");
        return;
    }

    Object *obj = nullptr, *obj2 = nullptr, *next_obj = nullptr;
    int shot = false;

    for (auto obj : ch->getRoom()->getInventory()) {
        if (shot == false) {
            if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE && obj != vehicle) {
                if (!strcasecmp(arg1, obj->getName().c_str())) {
                    obj2 = obj;
                    shot = true;
                }
            }
        }
    }
}
