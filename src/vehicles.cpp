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
#include "dbat/area.h"

#ifndef EXITN
#  define EXITN(room, door)        (get_room(room)->dir_option[door])
#endif


struct obj_data *find_vehicle_by_vnum(int vnum) {
    auto o = get_last_inserted(objectVnumIndex, vnum);
    if(o && GET_OBJ_TYPE(o) == ITEM_VEHICLE) return o;
    return nullptr;
}

struct obj_data *find_hatch_by_vnum(int vnum) {
    auto o = get_last_inserted(objectVnumIndex, vnum);
    if(o && GET_OBJ_TYPE(o) == ITEM_HATCH) return o;
    return nullptr;
}


/* Search the player's room, inventory and equipment for a control */
struct obj_data *find_control(struct char_data *ch) {
    struct obj_data *controls, *obj;
    int j;
    auto iscontrol = [ch](obj_data *o) { return CAN_SEE_OBJ(ch, o) && GET_OBJ_TYPE(o) == ITEM_CONTROL; };

    controls = ch->getRoom()->findObject(iscontrol);
    if (!controls) controls = ch->findObject(iscontrol);

    if (!controls)
        for (j = 0; j < NUM_WEARS && !controls; j++)
            if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)) &&
                GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_CONTROL) {
                    controls = GET_EQ(ch, j);
                    break;
                }
    return controls;
}

/* Drive our vehicle into another vehicle */
static void drive_into_vehicle(struct char_data *ch, struct obj_data *vehicle, char *arg) {
    struct obj_data *vehicle_in_out;
    int was_in, is_in, is_going_to;
    char buf[MAX_INPUT_LENGTH];

    if (!*arg) {
        send_to_char(ch, "@wDrive into what?\r\n");
        return;
    }
    if (!(vehicle_in_out = get_obj_in_list_vis(ch, arg, nullptr, vehicle->getLocationObjects()))) {
        send_to_char(ch, "@wNothing here by that name!\r\n");
        return;
    }

    if (GET_OBJ_TYPE(vehicle_in_out) != ITEM_VEHICLE) {
        send_to_char(ch, "@wThat's not a ship.\r\n");
        return;
    }

    if (vehicle == vehicle_in_out) {
        send_to_char(ch, "@wMy, we are in a clever mood today, aren't we.\r\n");
        return;
    }

    is_going_to = real_room(GET_OBJ_VAL(vehicle_in_out, 0));
    if (!ROOM_FLAGGED(is_going_to, ROOM_VEHICLE)) {
        send_to_char(ch, "@wThat ship can't carry other ships.");
        return;
    }

    sprintf(buf, "%s @wenters %s.\r\n", vehicle->short_description,
            vehicle_in_out->short_description);
    send_to_room(IN_ROOM(vehicle), buf);

    was_in = IN_ROOM(vehicle);
    obj_from_room(vehicle);
    obj_to_room(vehicle, is_going_to);
    is_in = IN_ROOM(vehicle);
    if (ch->desc)
        act("", true, ch, nullptr, nullptr, TO_ROOM);
    send_to_char(ch, "@wThe ship flies onward:\r\n");
    look_at_room(IN_ROOM(vehicle), ch, 0);
    sprintf(buf, "%s @wenters.\r\n", vehicle->short_description);
    send_to_room(is_in, buf);

}

/* Drive our vehicle out of another vehicle */
static void drive_outof_vehicle(struct char_data *ch, struct obj_data *vehicle) {
    struct obj_data *vehicle_in_out;
    char buf[MAX_INPUT_LENGTH];

    auto room = vehicle->getRoom();
    auto hatch = room->findObject([&](obj_data *o) { return GET_OBJ_TYPE(o) == ITEM_HATCH; });

    if (!hatch) {
        send_to_char(ch, "@wNowhere to pilot out of.\r\n");
        return;
    }

    if (!(vehicle_in_out = find_vehicle_by_vnum(GET_OBJ_VAL(hatch, 0)))) {
        send_to_char(ch, "@wYou can't pilot out anywhere!\r\n");
        return;
    }

    sprintf(buf, "%s @wexits %s.\r\n", vehicle->short_description,
            vehicle_in_out->short_description);
    send_to_room(room->vn, buf);

    obj_from_room(vehicle);
    obj_to_room(vehicle, IN_ROOM(vehicle_in_out));

    room = vehicle->getRoom();

    if (ch->desc)
        act("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.", true, ch, nullptr, nullptr,
            TO_ROOM);
    send_to_char(ch, "@wThe ship flies onward:\r\n");
    look_at_room(IN_ROOM(vehicle), ch, 0);
    int door;
    for (door = 0; door < NUM_OF_DIRS; door++) {
        auto e = room->dir_option[door];
        if(!e) continue;
        auto dest = e->getDestination();
        if(!dest) continue;
        if(!IS_SET(e->exit_info, EX_CLOSED)) continue;

        send_to_room(dest->vn, "@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.\r\n");
    }
    sprintf(buf, "%s @wflies out of %s.\r\n", vehicle->short_description,
            vehicle_in_out->short_description);
    send_to_room(IN_ROOM(vehicle), buf);
}

/* Drive out vehicle in a certain direction */
void drive_in_direction(struct char_data *ch, struct obj_data *vehicle, int dir) {
    char buf[MAX_INPUT_LENGTH];
    auto room = vehicle->getRoom();

    auto d = room->dir_option[dir];
    if(!d) {
        send_to_char(ch, "@wApparently %s doesn't exist there.\r\n", dirs[dir]);
        return;
    }
    auto dest = d->getDestination();
    if(!dest) {
        send_to_char(ch, "@wApparently %s doesn't exist there.\r\n", dirs[dir]);
        return;
    }

    if(IS_SET(d->exit_info, EX_CLOSED)) {
        if (d->keyword)
            send_to_char(ch, "@wThe %s seems to be closed.\r\n", fname(d->keyword));
        else
            send_to_char(ch, "@wIt seems to be closed.\r\n");
        return;
    }

    if (!dest->room_flags.test(ROOM_VEHICLE) && !dest->room_flags.test(ROOM_SPACE)) {
        /* But the vehicle can't go that way*/
        send_to_char(ch, "@wThe ship can't fit there!\r\n");
        return;
    }

    int was_in, is_in;

    sprintf(buf, "%s @wflies %s.\r\n", vehicle->short_description, dirs[dir]);
    send_to_room(IN_ROOM(vehicle), buf);

    obj_from_room(vehicle);
    obj_to_room(vehicle, dest->vn);
    struct obj_data *controls;
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

    struct obj_data *hatch = nullptr;
    auto con = get_room(GET_OBJ_VAL(vehicle, 0))->getObjects();
    for (auto h : filter_raw(con)) {
        if (GET_OBJ_TYPE(hatch) == ITEM_HATCH) {
            GET_OBJ_VAL(hatch, 3) = vehicle->getRoomVnum();
            hatch = h;
            break;
        }
    }

    is_in = IN_ROOM(vehicle);

    if (ch->desc)
        act("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.", true, ch, nullptr, nullptr,
            TO_ROOM);
    send_to_char(ch, "@wThe ship flies onward:\r\n");
    look_at_room(is_in, ch, 0);
    if (controls) {
        send_to_char(ch, "@RFUEL@D: %s%s@n\r\n",
                     GET_FUEL(controls) >= 200 ? "@G" : GET_FUEL(controls) >= 100 ? "@Y" : "@r",
                     add_commas(GET_FUEL(controls)).c_str());
    }
    int door;
    room = vehicle->getRoom();
    for (door = 0; door < NUM_OF_DIRS; door++) {
        auto e = room->dir_option[door];
        if(!e) continue;
        dest = e->getDestination();
        if(!dest) continue;
        if(!IS_SET(e->exit_info, EX_CLOSED)) continue;

        send_to_room(dest->vn, "@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.\r\n");
    }
    sprintf(buf, "%s @wflies in from the %s.\r\n",
            vehicle->short_description, dirs[rev_dir[dir]]);

    send_to_room(is_in, buf);

}

static void warp_ship_to_location(struct char_data *ch, struct obj_data *vehicle, int room_vnum) {
    if (vehicle->getRoomVnum() == room_vnum) {
        send_to_char(ch, "Your ship is already there!\r\n");
        return;
    }

    act("@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n",
        true, ch, nullptr, nullptr, TO_CHAR);
    act("@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n",
        true, ch, nullptr, nullptr, TO_ROOM);
    send_to_room(IN_ROOM(vehicle),
                 "%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n",
                 vehicle->short_description);
    obj_from_room(vehicle);
    obj_to_room(vehicle, real_room(room_vnum));
    send_to_room(IN_ROOM(vehicle),
                 "@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n",
                 vehicle->short_description);
}

static bool validate_warp_conditions(struct char_data *ch, struct obj_data *vehicle, const char *arg) {
    if (IS_NPC(ch))
        return false;

    if (!HAS_ARMS(ch)) {
        send_to_char(ch, "You have no arms!\r\n");
        return false;
    }
    if (!PLR_FLAGGED(ch, PLR_PILOTING)) {
        send_to_char(ch, "@wYou need to be seated in the pilot's seat.\r\n[Enter: Pilot ready/unready]\r\n");
        return false;
    }

    struct obj_data *controls = find_control(ch);
    if (!controls) {
        send_to_char(ch, "@wYou have nothing to control here!\r\n");
        return false;
    }

    vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0));
    if (!vehicle) {
        send_to_char(ch, "@wYou can't find anything to pilot.\r\n");
        return false;
    }

    if (!vehicle->getRoomFlag(ROOM_SPACE)) {
        send_to_char(ch, "Your ship needs to be in space to utilize its Instant Travel Warp Accelerator.\r\n");
        return false;
    }

    if (GET_OBJ_VNUM(vehicle) != 18400) {
        send_to_char(ch, "Your ship is not outfitted with an Instant Travel Warp Accelerator.\r\n");
        return false;
    }

    if (!*arg) {
        send_to_char(ch,
                     "Syntax: shipwarp [ earth | vegeta | namek | konack | aether | frigid | buoy1 | buoy2 | buoy3 ]\r\n");
        return false;
    }

    return true;
}

static int get_warp_destination(const char *arg, struct char_data *ch) {
    if (!strcasecmp(arg, "earth"))
        return 40979;
    if (!strcasecmp(arg, "namek"))
        return 42880;
    if (!strcasecmp(arg, "frigid"))
        return 30889;
    if (!strcasecmp(arg, "konack"))
        return 27065;
    if (!strcasecmp(arg, "vegeta"))
        return 32365;
    if (!strcasecmp(arg, "aether"))
        return 41959;
    if (!strcasecmp(arg, "buoy1"))
        return GET_RADAR1(ch) > 0 ? GET_RADAR1(ch) : NOWHERE;
    if (!strcasecmp(arg, "buoy2"))
        return GET_RADAR2(ch) > 0 ? GET_RADAR2(ch) : NOWHERE;
    if (!strcasecmp(arg, "buoy3"))
        return GET_RADAR3(ch) > 0 ? GET_RADAR3(ch) : NOWHERE;

    return NOWHERE;
}

ACMD(do_warp) {
    char arg[MAX_INPUT_LENGTH];
    struct obj_data *vehicle = nullptr;

    one_argument(argument, arg);

    if (!validate_warp_conditions(ch, vehicle, arg))
        return;

    int destination = get_warp_destination(arg, ch);
    if (destination == NOWHERE) {
        send_to_char(ch, "Invalid warp destination or buoy not launched.\r\n");
        return;
    }

    warp_ship_to_location(ch, vehicle, destination);
}

static void handle_pilot_ready(struct char_data *ch);
static void handle_pilot_unready(struct char_data *ch);
static bool validate_drive_conditions(struct char_data *ch, struct obj_data *&vehicle, struct obj_data *&controls);
static void handle_drive_command(struct char_data *ch, struct obj_data *vehicle, struct obj_data *controls, const std::string& arg, const std::string& arg2);
static void handle_drive_direction(struct char_data *ch, struct obj_data *vehicle, int dir, int speed);
static void handle_drive_land(struct char_data *ch, struct obj_data *vehicle, const std::string& pad);
static void handle_drive_launch(struct char_data *ch, struct obj_data *vehicle, struct obj_data *controls);
static void handle_buoy_launch(struct char_data *ch, struct obj_data *vehicle, const std::string& marker);
static void handle_buoy_deactivate(struct char_data *ch, const std::string& marker);

static void handle_pilot_ready(struct char_data *ch) {
    struct obj_data *controls = find_control(ch);
    if (!controls) {
        send_to_char(ch, "@wYou have nothing to control here!\r\n");
        return;
    }

    for (auto *d = descriptor_list; d; d = d->next) {
        if (IS_PLAYING(d) && d->character != ch && PLR_FLAGGED(d->character, PLR_PILOTING) && IN_ROOM(d->character) == IN_ROOM(ch)) {
            send_to_char(ch, "@w%s is already piloting the ship!\r\n", GET_NAME(d->character));
            return;
        }
    }

    ch->playerFlags.set(PLR_PILOTING);
    act("@w$n sits down and begins piloting the ship.", true, ch, nullptr, nullptr, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    send_to_char(ch, "@wYou take a seat in the pilot's chair.\r\n");
}

static void handle_pilot_unready(struct char_data *ch) {
    if (PLR_FLAGGED(ch, PLR_PILOTING)) {
        act("@w$n stands up and stops piloting the ship.", true, ch, nullptr, nullptr, TO_ROOM);
        send_to_char(ch, "@wYou stand up from the pilot's seat.\r\n");
        GET_POS(ch) = POS_STANDING;
        ch->playerFlags.reset(PLR_PILOTING);
    } else {
        send_to_char(ch, "You are already not flying the ship!\r\n");
    }
}

static bool validate_drive_conditions(struct char_data *ch, struct obj_data *&vehicle, struct obj_data *&controls) {
    if (!HAS_ARMS(ch)) {
        send_to_char(ch, "You have no arms!\r\n");
        return false;
    }

    if (!PLR_FLAGGED(ch, PLR_PILOTING)) {
        send_to_char(ch, "@wYou need to be seated in the pilot's seat.\r\n[Enter: Pilot ready/unready]\r\n");
        return false;
    }

    if (GET_POS(ch) < POS_SLEEPING) {
        send_to_char(ch, "@wYou can't see anything but stars!\r\n");
        return false;
    }

    if (AFF_FLAGGED(ch, AFF_BLIND)) {
        send_to_char(ch, "@wYou can't see a damned thing, you're blind!\r\n");
        return false;
    }

    if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
        send_to_char(ch, "@wIt is pitch black...\r\n");
        return false;
    }

    controls = find_control(ch);
    if (!controls) {
        send_to_char(ch, "@wYou have nothing to control here!\r\n");
        return false;
    }

    if (invalid_align(ch, controls) || invalid_class(ch, controls) || invalid_race(ch, controls)) {
        act("@wYou are zapped by $p@w and instantly step away from it.", false, ch, controls, nullptr, TO_CHAR);
        act("@w$n@w is zapped by $p@w and instantly steps away from it.", false, ch, controls, nullptr, TO_ROOM);
        return false;
    }

    vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0));
    if (!vehicle) {
        send_to_char(ch, "@wYou can't find anything to pilot.\r\n");
        return false;
    }

    if (GET_FUEL(controls) <= 0) {
        send_to_char(ch, "Your ship doesn't have enough fuel to move.\r\n");
        return false;
    }

    return true;
}

static const std::map<std::string, int> directions = {
    {"north", 0}, {"n", 0},
    {"east", 1}, {"e", 1},
    {"south", 2}, {"s", 2},
    {"west", 3}, {"w", 3},
    {"up", 4}, {"u", 4},
    {"down", 5}, {"d", 5},
    {"northwest", 6}, {"nw", 6}, {"northw", 6},
    {"northeast", 7}, {"ne", 7}, {"northe", 7},
    {"southeast", 8}, {"se", 8}, {"southe", 8},
    {"southwest", 9}, {"sw", 9}, {"southw", 9},
    {"inside", 10},
    {"outside", 11}
};

static void handle_drive_command(struct char_data *ch, struct obj_data *vehicle, struct obj_data *controls, const std::string& arg, const std::string& arg2) {
    if (arg.empty()) {
        send_to_char(ch, "@wPilot, yes, but where?\r\n");
        return;
    }

    if (is_abbrev(arg.c_str(), "into") || is_abbrev(arg.c_str(), "onto")) {
        drive_into_vehicle(ch, vehicle, (char*)arg2.c_str());
    } else if (is_abbrev(arg.c_str(), "out") && !EXIT(vehicle, OUTDIR)) {
        drive_outof_vehicle(ch, vehicle);
    } else {

        auto it = directions.find(arg);
        if (it != directions.end()) {
            int dir = it->second;
            handle_drive_direction(ch, vehicle, dir, GET_OBJ_VAL(controls, 1));
        } else if (is_abbrev(arg.c_str(), "land")) {
            handle_drive_land(ch, vehicle, arg2);
        } else if (is_abbrev(arg.c_str(), "launch")) {
            handle_drive_launch(ch, vehicle, controls);
        } else if (is_abbrev(arg.c_str(), "mark")) {
            handle_buoy_launch(ch, vehicle, arg2);
        } else if (is_abbrev(arg.c_str(), "deactivate")) {
            handle_buoy_deactivate(ch, arg2);
        } else {
            send_to_char(ch, "@wThats not a valid direction.\r\n");
            send_to_char(ch, "Try one of these.\r\n");
            send_to_char(ch, "[ north/n  | south/s  | east/e  |  west/w  ]\r\n");
            send_to_char(ch, "[ up/u | down/d | northeast/ne/northe | northwest/nw/northw]\r\n");
            send_to_char(ch, "[  southeast/se/southe  |  southwest/sw/southw]\r\n");
            send_to_char(ch, "[  into  |  onto  |  inside  |  outside  ]@n\r\n");
            send_to_char(ch, "[ land | launch ]@n\r\n");
        }
    }
}

static void handle_drive_direction(struct char_data *ch, struct obj_data *vehicle, int dir, int speed) {
    drive_in_direction(ch, vehicle, dir);

    switch (speed) {
        case 1:
            WAIT_STATE(ch, PULSE_2SEC * 1.5);
            break;
        case 2:
            WAIT_STATE(ch, PULSE_2SEC);
            break;
        case 3:
            WAIT_STATE(ch, PULSE_1SEC * 1.5);
            break;
        case 4:
            WAIT_STATE(ch, PULSE_1SEC);
            break;
        case 5:
            WAIT_STATE(ch, PULSE_1SEC * 0.5);
            break;
    }
}

static void handle_drive_land(struct char_data *ch, struct obj_data *vehicle, const std::string& pad) {
    auto rvn = vehicle->getRoomVnum();
    auto planet = checkOrbit(rvn);

    if(!planet) {
        send_to_char(ch, "@wYou are not orbiting a planet.\r\n");
        return;
    }

    auto pads = getPlanetSpacepads(planet);

    if(pads.empty()) {
        send_to_char(ch, "@wThere are no landing destinations here.\r\n");
        return;
    }

    if(pad.empty()) {
        send_to_char(ch, "Land where?\r\n");
        displayLandSpots(ch, getPlanetColorName(planet), pads);
        return;
    }

    room_vnum landing = NOWHERE;
    std::string landName = "UNKNOWN";

    if(auto matched = partialMatch(pad, pads.begin(), pads.end(), false, [](const auto& p) {return p.first;}); matched != pads.end()) {
        landing = matched->second;
        landName = matched->first;
    }

    auto landroom = get_room(landing);

    if(!landroom) {
        send_to_char(ch, "You can't land there.\r\n");
        return;
    }

    act("@wYou set the controls to descend.@n", false, ch, 0, 0, TO_CHAR);
    act("@C$n @wmanipulates the ship controls.@n", false, ch, 0, 0, TO_ROOM);
    act("@RThe ship rocks and shakes as it descends through the atmosphere!@n", false, ch, 0, 0, TO_CHAR);
    act("@RThe ship rocks and shakes as it descends through the atmosphere!@n", false, ch, 0, 0, TO_ROOM);
    act("@wThe ship has landed.@n", false, ch, 0, 0, TO_CHAR);
    act("@wThe ship has landed.@n", false, ch, 0, 0, TO_ROOM);

    obj_from_room(vehicle);
    obj_to_room(vehicle, landroom);

    char buf3[MAX_INPUT_LENGTH];
    sprintf(buf3, "%s @wcomes in from above and slowly settles on the ground.@n\r\n", vehicle->short_description);
    look_at_room(landroom, ch, 0);
    send_to_room(landroom, buf3);

}

static void handle_drive_launch(struct char_data *ch, struct obj_data *vehicle, struct obj_data *controls) {
    auto room = vehicle->getRoom();
    auto planet = getPlanet(room->vn);
    if (!planet) {
        send_to_char(ch, "@wYou are not on a planet.@n\r\n");
        return;
    }
    auto dest = getPlanetOrbit(planet);
    if(dest == NOWHERE) {
        send_to_char(ch, "@wYou are not on a planet.@n\r\n");
        return;
    }

    act("@wYou set the controls to launch.@n", false, ch, nullptr, nullptr, TO_CHAR);
    act("@C$n @wmanipulates the ship controls.@n", false, ch, nullptr, nullptr, TO_ROOM);
    act("@RThe ship shudders as it launches up into the sky!@n", false, ch, nullptr, nullptr, TO_CHAR);
    act("@RThe ship shudders as it launches up into the sky!@n", false, ch, nullptr, nullptr, TO_ROOM);
    act("@wThe ship has reached low orbit.@n", false, ch, nullptr, nullptr, TO_CHAR);
    act("@wThe ship has reached low orbit.@n", false, ch, nullptr, nullptr, TO_ROOM);
    send_to_room(IN_ROOM(vehicle), "@R%s @Rshudders before blasting off into the sky!@n",
                 vehicle->short_description);

    if (GET_FUELCOUNT(controls) < 5) {
        GET_FUELCOUNT(controls) += 1;
    } else {
        GET_FUELCOUNT(controls) = 0;
        GET_FUEL(controls) -= 1;
        if (GET_FUEL(controls) < 0) {
            GET_FUEL(controls) = 0;
        }
    }

    obj_from_room(vehicle);
    obj_to_room(vehicle, dest);
    look_at_room(IN_ROOM(vehicle), ch, 0);
    send_to_char(ch, "@RFUEL@D: %s%s@n\r\n",
                 GET_FUEL(controls) >= 200 ? "@G" : GET_FUEL(controls) >= 100 ? "@Y" : "@r",
                 add_commas(GET_FUEL(controls)).c_str());
}

static void handle_buoy_launch(struct char_data *ch, struct obj_data *vehicle, const std::string& marker) {
    if (!vehicle->getRoomFlag(ROOM_SPACE)) {
        send_to_char(ch, "@wYou need to be in space to launch a marker buoy.\r\n");
        return;
    }

    int buoy_num = 0;
    if (marker == "1") buoy_num = GET_RADAR1(ch);
    if (marker == "2") buoy_num = GET_RADAR2(ch);
    if (marker == "3") buoy_num = GET_RADAR3(ch);

    if (buoy_num > 0) {
        send_to_char(ch, "@wYou need to 'deactivate' that marker.\r\n");
    } else {
        act("@wYou enter a unique code and launch a marker buoy.@n\r\n", false, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);

        if (marker == "1") GET_RADAR1(ch) = vehicle->getRoomVnum();
        if (marker == "2") GET_RADAR2(ch) = vehicle->getRoomVnum();
        if (marker == "3") GET_RADAR3(ch) = vehicle->getRoomVnum();
    }
}

static void handle_buoy_deactivate(struct char_data *ch, const std::string& marker) {
    int &buoy_num = (marker == "1") ? GET_RADAR1(ch) : (marker == "2") ? GET_RADAR2(ch) : GET_RADAR3(ch);

    if (buoy_num <= 0) {
        send_to_char(ch, "@wYou haven't launched that buoy yet.\r\n");
    } else {
        act("@wYou enter buoy's code and command it to deactivate.@n\r\n", false, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);
        buoy_num = 0;
    }
}

ACMD(do_drive) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct obj_data *vehicle = nullptr, *controls = nullptr;

    half_chop(argument, arg, arg2);

    if (!strcasecmp(arg, "ready")) {
        handle_pilot_ready(ch);
        return;
    }

    if (!strcasecmp(arg, "unready")) {
        handle_pilot_unready(ch);
        return;
    }

    if (!validate_drive_conditions(ch, vehicle, controls))
        return;

    handle_drive_command(ch, vehicle, controls, arg, arg2);
}


ACMD(do_ship_fire) {
    struct obj_data *vehicle, *controls;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!(controls = find_control(ch))) {
        send_to_char(ch, "@wYou must be near the comm station in the cockpit.\r\n");
        return;
    }

    if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0)))) {
        send_to_char(ch, "@wSomething cosmic is jamming your signal! Quick call Iovan to repair it!\r\n");
        return;
    }

    struct obj_data *obj = nullptr, *obj2 = nullptr, *next_obj = nullptr;
    int shot = false;
    auto loco = ch->getLocationObjects();
    for (auto obj : filter_raw(loco)) {
        if (shot == false) {
            if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE && obj != vehicle) {
                if (!strcasecmp(arg1, obj->name)) {
                    obj2 = obj;
                    shot = true;
                }
            }
        }
    }
}
