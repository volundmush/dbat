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
#include "dbat/ObjectUtils.h"
#include "dbat/CharacterUtils.h"
#include "dbat/Destination.h"
#include "dbat/Descriptor.h"
#include "dbat/RoomUtils.h"
#include "dbat/Zone.h"
#include "dbat/vehicles.h"
//#include "dbat/send.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/constants.h"
#include "dbat/class.h"
#include "dbat/races.h"
//#include "dbat/act.informative.h"
#include "dbat/planet.h"
#include "dbat/utils.h"
#include "dbat/filter.h"

#include "dbat/const/Pulse.h"

#ifndef EXITN
#define EXITN(room, door) (get_room(room)->dir_option[door])
#endif

Object *find_vehicle_by_vnum(int vnum)
{
    auto o = objectSubscriptions.first(fmt::format("vnum_{}", vnum));
    if (o && GET_OBJ_TYPE(o) == ITEM_VEHICLE)
        return o;
    return nullptr;
}

Object *find_hatch_by_vnum(int vnum)
{
    auto o = objectSubscriptions.first(fmt::format("vnum_{}", vnum));
    if (o && GET_OBJ_TYPE(o) == ITEM_HATCH)
        return o;
    return nullptr;
}

/* Search the player's room, inventory and equipment for a control */
Object *find_control(Character *ch)
{
    auto iscontrol = [ch](Object *o)
    { return ch->canSee(o) && GET_OBJ_TYPE(o) == ITEM_CONTROL; };

    auto controls = ch->location.searchObjects(iscontrol);
    if (!controls)
        controls = ch->searchInventory(iscontrol);

    if (!controls)
        for (auto &[slot, eq] : ch->getEquipment())
        {
            if (iscontrol(eq))
            {
                controls = eq;
                break;
            }
        }
    return controls;
}

/* Drive our vehicle into another vehicle */
static void drive_into_vehicle(Character *ch, Object *vehicle, char *arg)
{
    Object *vehicle_in_out;
    int is_going_to;
    char buf[MAX_INPUT_LENGTH];

    if (!*arg)
    {
        ch->sendText("@wDrive into what?\r\n");
        return;
    }
    if (!(vehicle_in_out = get_obj_in_list_vis(ch, arg, nullptr, vehicle->location.getObjects())))
    {
        ch->sendText("@wNothing here by that name!\r\n");
        return;
    }

    if (GET_OBJ_TYPE(vehicle_in_out) != ITEM_VEHICLE)
    {
        ch->sendText("@wThat's not a ship.\r\n");
        return;
    }

    if (vehicle == vehicle_in_out)
    {
        ch->sendText("@wMy, we are in a clever mood today, aren't we.\r\n");
        return;
    }

    is_going_to = real_room(GET_OBJ_VAL(vehicle_in_out, VAL_VEHICLE_DEST));
    if (!ROOM_FLAGGED(is_going_to, ROOM_VEHICLE))
    {
        ch->sendText("@wThat ship can't carry other ships.");
        return;
    }

    vehicle->location.send_to("%s @wenters %s.\r\n", vehicle->getShortDescription(),
                              vehicle_in_out->getShortDescription());

    auto was_in = vehicle->location;
    vehicle->leaveLocation();
    vehicle->moveToLocation(is_going_to);
    auto is_in = vehicle->location;
    if (ch->desc)
        act("", true, ch, nullptr, nullptr, TO_ROOM);
    ch->sendText("@wThe ship flies onward:\r\n");
    ch->lookAtLocation(vehicle->location);
    vehicle->location.send_to("%s @wenters.\r\n", vehicle->getShortDescription());
}

/* Drive our vehicle out of another vehicle */
static void drive_outof_vehicle(Character *ch, Object *vehicle)
{
    Object *vehicle_in_out;

    auto hatch = ch->location.searchObjects([&](Object *o)
                                            { return GET_OBJ_TYPE(o) == ITEM_HATCH; });

    if (!hatch)
    {
        ch->sendText("@wNowhere to pilot out of.\r\n");
        return;
    }

    if (!(vehicle_in_out = find_vehicle_by_vnum(GET_OBJ_VAL(hatch, VAL_HATCH_DEST))))
    {
        ch->sendText("@wYou can't pilot out anywhere!\r\n");
        return;
    }

    vehicle->location.send_to("%s @wexits %s.\r\n", vehicle->getShortDescription(),
                              vehicle_in_out->getShortDescription());

    vehicle->leaveLocation();
    vehicle->moveToLocation(vehicle_in_out->location);

    if (ch->desc)
        act("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.", true, ch, nullptr, nullptr,
            TO_ROOM);
    ch->sendText("@wThe ship flies onward:\r\n");
    ch->lookAtLocation(vehicle->location);
    for (auto &[door, e] : vehicle->location.getExits())
    {
        if (e.exit_flags[EX_CLOSED])
            continue;
        e.sendText("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.\r\n");
    }
    vehicle->location.send_to("%s @wflies out of %s.\r\n", vehicle->getShortDescription(),
                              vehicle_in_out->getShortDescription());
}

/* Drive out vehicle in a certain direction */
void drive_in_direction(Character *ch, Object *vehicle, int dir)
{

    auto d = vehicle->location.getExit(static_cast<Direction>(dir));
    if (!d)
    {
        ch->send_to("@wApparently %s doesn't exist there.\r\n", dirs[dir]);
        return;
    }
    auto &dest = d.value();
    if (!dest)
    {
        ch->send_to("@wApparently %s doesn't exist there.\r\n", dirs[dir]);
        return;
    }

    if (dest.exit_flags[EX_CLOSED])
    {
        if (!dest.keyword.empty())
            ch->send_to("@wThe %s seems to be closed.\r\n", fname(dest.keyword.c_str()));
        else
            ch->sendText("@wIt seems to be closed.\r\n");
        return;
    }

    if (!dest.getRoomFlag(RoomFlag::vehicle) && !dest.getWhereFlag(WhereFlag::space))
    {
        /* But the vehicle can't go that way*/
        ch->sendText("@wThe ship can't fit there!\r\n");
        return;
    }

    vehicle->location.send_to("%s @wflies %s.\r\n", vehicle->getShortDescription(), dirs[dir]);

    vehicle->leaveLocation();
    vehicle->moveToLocation(dest);

    Object *controls;
    if ((controls = find_control(ch)))
    {
        if (GET_FUELCOUNT(controls) < 5)
        {
            MOD_OBJ_VAL(controls, VAL_CONTROL_FUEL, 1);
        }
        else
        {
            SET_OBJ_VAL(controls, VAL_CONTROL_FUEL, 0);
            MOD_OBJ_VAL(controls, VAL_CONTROL_FUEL, -1);
            if (GET_FUEL(controls) < 0)
            {
                SET_OBJ_VAL(controls, VAL_CONTROL_FUEL, 0);
            }
        }
    }

    Object *hatch = nullptr;
    auto desroom = get_room(GET_OBJ_VAL(vehicle, VAL_VEHICLE_DEST));
    Destination des(desroom);
    auto con = des.getObjects();

    for (auto h : filter_raw(con))
    {
        if (GET_OBJ_TYPE(hatch) == ITEM_HATCH)
        {
            SET_OBJ_VAL(hatch, VAL_HATCH_EXTROOM, vehicle->location.getVnum());
            hatch = h;
            break;
        }
    }

    if (ch->desc)
        act("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.", true, ch, nullptr, nullptr,
            TO_ROOM);
    ch->sendText("@wThe ship flies onward:\r\n");
    ch->lookAtLocation(vehicle->location);
    if (controls)
    {
        ch->send_to("@RFUEL@D: %s%s@n\r\n", GET_FUEL(controls) >= 200 ? "@G" : GET_FUEL(controls) >= 100 ? "@Y"
                                                                                                         : "@r",
                    add_commas(GET_FUEL(controls)).c_str());
    }

    for (auto &[door, e] : vehicle->location.getExits())
    {
        if (e.exit_flags[EX_CLOSED])
            continue;
        e.sendText("@wThe @De@Wn@wg@Di@wn@We@Ds@w of the ship @rr@Ro@ra@Rr@w as it moves.\r\n");
    }

    vehicle->location.send_to("%s @wflies in from the %s.\r\n",
                              vehicle->getShortDescription(), dirs[rev_dir[dir]]);
}

static void warp_ship_to_location(Character *ch, Object *vehicle, int room_vnum)
{
    if (vehicle->location == room_vnum)
    {
        ch->sendText("Your ship is already there!\r\n");
        return;
    }

    act("@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find your ship in a new location!@n",
        true, ch, nullptr, nullptr, TO_CHAR);
    act("@BA glow of blue light floods in through the window for an instant. You feel a strange shift as the light disappears and you find the ship in a new location!@n",
        true, ch, nullptr, nullptr, TO_ROOM);
    vehicle->location.send_to("%s @Bbegins to glow bright blue before disappearing in a flash of light!@n\r\n",
                              vehicle->getShortDescription());
    vehicle->leaveLocation();
    vehicle->moveToLocation(room_vnum);
    vehicle->location.send_to("@BSuddenly in a flash of blue light @n%s @B appears instantly!@n\r\n",
                              vehicle->getShortDescription());
}

static bool validate_warp_conditions(Character *ch, Object *vehicle, const char *arg)
{
    if (IS_NPC(ch))
        return false;

    if (!HAS_ARMS(ch))
    {
        ch->sendText("You have no arms!\r\n");
        return false;
    }
    if (!PLR_FLAGGED(ch, PLR_PILOTING))
    {
        ch->sendText("@wYou need to be seated in the pilot's seat.\r\n[Enter: Pilot ready/unready]\r\n");
        return false;
    }

    Object *controls = find_control(ch);
    if (!controls)
    {
        ch->sendText("@wYou have nothing to control here!\r\n");
        return false;
    }

    vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, VAL_CONTROL_VEHICLE_VNUM));
    if (!vehicle)
    {
        ch->sendText("@wYou can't find anything to pilot.\r\n");
        return false;
    }

    if (!vehicle->location.getWhereFlag(WhereFlag::space))
    {
        ch->sendText("Your ship needs to be in space to utilize its Instant Travel Warp Accelerator.\r\n");
        return false;
    }

    if (GET_OBJ_VNUM(vehicle) != 18400)
    {
        ch->sendText("Your ship is not outfitted with an Instant Travel Warp Accelerator.\r\n");
        return false;
    }

    if (!*arg)
    {
        ch->sendText("Syntax: shipwarp [ earth | vegeta | namek | konack | aether | frigid | buoy1 | buoy2 | buoy3 ]\r\n");
        return false;
    }

    return true;
}

static int get_warp_destination(const char *arg, Character *ch)
{
    if (boost::iequals(arg, "earth"))
        return 40979;
    if (boost::iequals(arg, "namek"))
        return 42880;
    if (boost::iequals(arg, "frigid"))
        return 30889;
    if (boost::iequals(arg, "konack"))
        return 27065;
    if (boost::iequals(arg, "vegeta"))
        return 32365;
    if (boost::iequals(arg, "aether"))
        return 41959;
    if (boost::iequals(arg, "buoy1"))
        return GET_RADAR1(ch) > 0 ? GET_RADAR1(ch) : NOWHERE;
    if (boost::iequals(arg, "buoy2"))
        return GET_RADAR2(ch) > 0 ? GET_RADAR2(ch) : NOWHERE;
    if (boost::iequals(arg, "buoy3"))
        return GET_RADAR3(ch) > 0 ? GET_RADAR3(ch) : NOWHERE;

    return NOWHERE;
}

ACMD(do_warp)
{
    return;
    char arg[MAX_INPUT_LENGTH];
    Object *vehicle = nullptr;

    one_argument(argument, arg);

    if (!validate_warp_conditions(ch, vehicle, arg))
        return;

    int destination = get_warp_destination(arg, ch);
    if (destination == NOWHERE)
    {
        ch->sendText("Invalid warp destination or buoy not launched.\r\n");
        return;
    }

    warp_ship_to_location(ch, vehicle, destination);
}

static void handle_pilot_ready(Character *ch);
static void handle_pilot_unready(Character *ch);
static bool validate_drive_conditions(Character *ch, Object *&vehicle, Object *&controls);
static void handle_drive_command(Character *ch, Object *vehicle, Object *controls, const std::string &arg, const std::string &arg2);
static void handle_drive_direction(Character *ch, Object *vehicle, int dir, int speed);
static void handle_drive_land(Character *ch, Object *vehicle, const std::string &pad);
static void handle_drive_launch(Character *ch, Object *vehicle, Object *controls);
static void handle_buoy_launch(Character *ch, Object *vehicle, const std::string &marker);
static void handle_buoy_deactivate(Character *ch, const std::string &marker);

static void handle_pilot_ready(Character *ch)
{
    Object *controls = find_control(ch);
    if (!controls)
    {
        ch->sendText("@wYou have nothing to control here!\r\n");
        return;
    }

    for (auto *d = descriptor_list; d; d = d->next)
    {
        if (IS_PLAYING(d) && d->character != ch && PLR_FLAGGED(d->character, PLR_PILOTING) && d->character->location == ch->location)
        {
            ch->send_to("@w%s is already piloting the ship!\r\n", GET_NAME(d->character));
            return;
        }
    }

    ch->player_flags.set(PLR_PILOTING, true);
    act("@w$n sits down and begins piloting the ship.", true, ch, nullptr, nullptr, TO_ROOM);
    ch->position = POS_SITTING;
    ch->sendText("@wYou take a seat in the pilot's chair.\r\n");
}

static void handle_pilot_unready(Character *ch)
{
    if (PLR_FLAGGED(ch, PLR_PILOTING))
    {
        act("@w$n stands up and stops piloting the ship.", true, ch, nullptr, nullptr, TO_ROOM);
        ch->sendText("@wYou stand up from the pilot's seat.\r\n");
        ch->position = POS_STANDING;
        ch->player_flags.set(PLR_PILOTING, false);
    }
    else
    {
        ch->sendText("You are already not flying the ship!\r\n");
    }
}

static bool validate_drive_conditions(Character *ch, Object *&vehicle, Object *&controls)
{
    if (!HAS_ARMS(ch))
    {
        ch->sendText("You have no arms!\r\n");
        return false;
    }

    if (!PLR_FLAGGED(ch, PLR_PILOTING))
    {
        ch->sendText("@wYou need to be seated in the pilot's seat.\r\n[Enter: Pilot ready/unready]\r\n");
        return false;
    }

    if (GET_POS(ch) < POS_SLEEPING)
    {
        ch->sendText("@wYou can't see anything but stars!\r\n");
        return false;
    }

    if (AFF_FLAGGED(ch, AFF_BLIND))
    {
        ch->sendText("@wYou can't see a damned thing, you're blind!\r\n");
        return false;
    }

    if (ch->location.getIsDark() && !CAN_SEE_IN_DARK(ch))
    {
        ch->sendText("@wIt is pitch black...\r\n");
        return false;
    }

    controls = find_control(ch);
    if (!controls)
    {
        ch->sendText("@wYou have nothing to control here!\r\n");
        return false;
    }

    if (invalid_align(ch, controls) || invalid_class(ch, controls) || invalid_race(ch, controls))
    {
        act("@wYou are zapped by $p@w and instantly step away from it.", false, ch, controls, nullptr, TO_CHAR);
        act("@w$n@w is zapped by $p@w and instantly steps away from it.", false, ch, controls, nullptr, TO_ROOM);
        return false;
    }

    vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, VAL_CONTROL_VEHICLE_VNUM));
    if (!vehicle)
    {
        ch->sendText("@wYou can't find anything to pilot.\r\n");
        return false;
    }

    if (GET_FUEL(controls) <= 0)
    {
        ch->sendText("Your ship doesn't have enough fuel to move.\r\n");
        return false;
    }

    return true;
}

static const std::map<std::string, int> directions = {
    {"north", 0}, {"n", 0}, {"east", 1}, {"e", 1}, {"south", 2}, {"s", 2}, {"west", 3}, {"w", 3}, {"up", 4}, {"u", 4}, {"down", 5}, {"d", 5}, {"northwest", 6}, {"nw", 6}, {"northw", 6}, {"northeast", 7}, {"ne", 7}, {"northe", 7}, {"southeast", 8}, {"se", 8}, {"southe", 8}, {"southwest", 9}, {"sw", 9}, {"southw", 9}, {"inside", 10}, {"outside", 11}};

static void handle_drive_command(Character *ch, Object *vehicle, Object *controls, const std::string &arg, const std::string &arg2)
{
    if (arg.empty())
    {
        ch->sendText("@wPilot, yes, but where?\r\n");
        return;
    }

    if (is_abbrev(arg.c_str(), "into") || is_abbrev(arg.c_str(), "onto"))
    {
        drive_into_vehicle(ch, vehicle, (char *)arg2.c_str());
    }
    else if (is_abbrev(arg.c_str(), "out") && !EXIT(vehicle, OUTDIR))
    {
        drive_outof_vehicle(ch, vehicle);
    }
    else
    {

        auto it = directions.find(arg);
        if (it != directions.end())
        {
            int dir = it->second;
            handle_drive_direction(ch, vehicle, dir, GET_OBJ_VAL(controls, VAL_CONTROL_SPEED));
        }
        else if (is_abbrev(arg.c_str(), "land"))
        {
            handle_drive_land(ch, vehicle, arg2);
        }
        else if (is_abbrev(arg.c_str(), "launch"))
        {
            handle_drive_launch(ch, vehicle, controls);
        }
        else if (is_abbrev(arg.c_str(), "mark"))
        {
            handle_buoy_launch(ch, vehicle, arg2);
        }
        else if (is_abbrev(arg.c_str(), "deactivate"))
        {
            handle_buoy_deactivate(ch, arg2);
        }
        else
        {
            ch->sendText("@wThats not a valid direction.\r\n");
            ch->sendText("Try one of these.\r\n");
            ch->sendText("[ north/n  | south/s  | east/e  |  west/w  ]\r\n");
            ch->sendText("[ up/u | down/d | northeast/ne/northe | northwest/nw/northw]\r\n");
            ch->sendText("[  southeast/se/southe  |  southwest/sw/southw]\r\n");
            ch->sendText("[  into  |  onto  |  inside  |  outside  ]@n\r\n");
            ch->sendText("[ land | launch ]@n\r\n");
        }
    }
}

static void handle_drive_direction(Character *ch, Object *vehicle, int dir, int speed)
{
    drive_in_direction(ch, vehicle, dir);

    switch (speed)
    {
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

static void handle_drive_land(Character *ch, Object *vehicle, const std::string &pad)
{
    auto lz = vehicle->location.getLandZone();

    if (!lz)
    {
        ch->sendText("@wYou are not orbiting a planet.\r\n");
        return;
    }

    auto pads = lz->getDockingSpots();

    if (pads.empty())
    {
        ch->sendText("@wThere are no landing destinations here.\r\n");
        return;
    }

    if (pad.empty())
    {
        ch->sendText("Land where?\r\n");
        displayLandSpots(ch, lz->name, pads);
        return;
    }

    Location landing;
    std::string landName = "UNKNOWN";

    if (auto matched = partialMatch(pad, pads, false))
    {
        landing = matched.value()->second;
        landName = matched.value()->first;
    }

    if (!landing)
    {
        ch->sendText("You can't land there.\r\n");
        return;
    }

    act("@wYou set the controls to descend.@n", false, ch, 0, 0, TO_CHAR);
    act("@C$n @wmanipulates the ship controls.@n", false, ch, 0, 0, TO_ROOM);
    act("@RThe ship rocks and shakes as it descends through the atmosphere!@n", false, ch, 0, 0, TO_CHAR);
    act("@RThe ship rocks and shakes as it descends through the atmosphere!@n", false, ch, 0, 0, TO_ROOM);
    act("@wThe ship has landed.@n", false, ch, 0, 0, TO_CHAR);
    act("@wThe ship has landed.@n", false, ch, 0, 0, TO_ROOM);

    vehicle->leaveLocation();
    vehicle->moveToLocation(landing);

    char buf3[MAX_INPUT_LENGTH];
    sprintf(buf3, "%s @wcomes in from above and slowly settles on the ground.@n\r\n", vehicle->getShortDescription());
    ch->lookAtLocation(vehicle->location);
    landing.sendText(buf3);
}

static void handle_drive_launch(Character *ch, Object *vehicle, Object *controls)
{
    auto lz = vehicle->location.getLandZone();
    if (!lz)
    {
        ch->sendText("@wYou are not on a planet.@n\r\n");
        return;
    }
    auto dest = vehicle->location.getLaunchDestination();
    if (!dest)
    {
        ch->sendText("@wYou are not on a planet.@n\r\n");
        return;
    }

    act("@wYou set the controls to launch.@n", false, ch, nullptr, nullptr, TO_CHAR);
    act("@C$n @wmanipulates the ship controls.@n", false, ch, nullptr, nullptr, TO_ROOM);
    act("@RThe ship shudders as it launches up into the sky!@n", false, ch, nullptr, nullptr, TO_CHAR);
    act("@RThe ship shudders as it launches up into the sky!@n", false, ch, nullptr, nullptr, TO_ROOM);
    act("@wThe ship has reached low orbit.@n", false, ch, nullptr, nullptr, TO_CHAR);
    act("@wThe ship has reached low orbit.@n", false, ch, nullptr, nullptr, TO_ROOM);
    vehicle->location.send_to("@R%s @Rshudders before blasting off into the sky!@n",
                              vehicle->getShortDescription());

    if (GET_FUELCOUNT(controls) < 5)
    {
        MOD_OBJ_VAL(controls, VAL_CONTROL_FUELCOUNT, 1);
    }
    else
    {
        SET_OBJ_VAL(controls, VAL_CONTROL_FUELCOUNT, 0);
        MOD_OBJ_VAL(controls, VAL_CONTROL_FUEL, -1);
        if (GET_FUEL(controls) < 0)
        {
            SET_OBJ_VAL(controls, VAL_CONTROL_FUEL, 0);
        }
    }

    vehicle->leaveLocation();
    vehicle->moveToLocation(dest);
    ch->lookAtLocation(vehicle->location);
    ch->send_to("@RFUEL@D: %s%s@n\r\n", GET_FUEL(controls) >= 200 ? "@G" : GET_FUEL(controls) >= 100 ? "@Y"
                                                                                                     : "@r",
                add_commas(GET_FUEL(controls)).c_str());
}

static void handle_buoy_launch(Character *ch, Object *vehicle, const std::string &marker)
{
    if (!vehicle->location.getWhereFlag(WhereFlag::space))
    {
        ch->sendText("@wYou need to be in space to launch a marker buoy.\r\n");
        return;
    }

    int buoy_num = ch->getBaseStat(fmt::format("radar{}", marker));

    if (buoy_num > 0)
    {
        ch->sendText("@wYou need to 'deactivate' that marker.\r\n");
    }
    else
    {
        act("@wYou enter a unique code and launch a marker buoy.@n\r\n", false, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);

        ch->setBaseStat(fmt::format("radar{}", marker), vehicle->location.getVnum());
    }
}

static void handle_buoy_deactivate(Character *ch, const std::string &marker)
{
    auto buoy = ch->getBaseStat(fmt::format("radar{}", marker));

    if (buoy <= 0)
    {
        ch->sendText("@wYou haven't launched that buoy yet.\r\n");
    }
    else
    {
        act("@wYou enter buoy's code and command it to deactivate.@n\r\n", false, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@w manipulates the ship controls.@n\r\n", false, ch, nullptr, nullptr, TO_ROOM);
        ch->setBaseStat(fmt::format("radar{}", marker), 0);
    }
}

ACMD(do_drive)
{
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Object *vehicle = nullptr, *controls = nullptr;

    half_chop(argument, arg, arg2);

    if (boost::iequals(arg, "ready"))
    {
        handle_pilot_ready(ch);
        return;
    }

    if (boost::iequals(arg, "unready"))
    {
        handle_pilot_unready(ch);
        return;
    }

    if (!validate_drive_conditions(ch, vehicle, controls))
        return;

    handle_drive_command(ch, vehicle, controls, arg, arg2);
}

ACMD(do_ship_fire)
{
    Object *vehicle, *controls;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!(controls = find_control(ch)))
    {
        ch->sendText("@wYou must be near the comm station in the cockpit.\r\n");
        return;
    }

    if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, VAL_CONTROL_VEHICLE_VNUM))))
    {
        ch->sendText("@wSomething cosmic is jamming your signal! Quick call Iovan to repair it!\r\n");
        return;
    }

    Object *obj = nullptr, *obj2 = nullptr, *next_obj = nullptr;
    int shot = false;
    auto loco = ch->location.getObjects();
    for (auto obj : filter_raw(loco))
    {
        if (shot == false)
        {
            if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE && obj != vehicle)
            {
                if (boost::iequals(arg1, obj->getName()))
                {
                    obj2 = obj;
                    shot = true;
                }
            }
        }
    }
}
