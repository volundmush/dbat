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
#include "dbat/dg_comm.h"
#include "dbat/vehicles.h"
#include "dbat/oasis_copy.h"
#include "dbat/handler.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/fight.h"
#include "dbat/spells.h"
#include "dbat/oasis.h"
#include "dbat/guild.h"
#include "dbat/dg_scripts.h"
#include "dbat/local_limits.h"
#include "dbat/constants.h"
#include "dbat/act.informative.h"

/* local functions */
static void handle_fall(BaseCharacter *ch);

static int check_swim(BaseCharacter *ch);

static void disp_locations(BaseCharacter *ch, vnum areaVnum, std::set<room_vnum>& rooms);

static int has_boat(BaseCharacter *ch);

static int has_key(BaseCharacter *ch, obj_vnum key);

static int ok_pick(BaseCharacter *ch, obj_vnum keynum, int pickproof, int dclock, int scmd, Object *obj);

static int has_flight(BaseCharacter *ch);

static int do_simple_enter(BaseCharacter *ch, Object *obj, int need_specials_check);

static int perform_enter_obj(BaseCharacter *ch, Object *obj, int need_specials_check);

static int do_simple_leave(BaseCharacter *ch, Object *obj, int need_specials_check);

static int perform_leave_obj(BaseCharacter *ch, Object *obj, int need_specials_check);

static int64_t calcNeedMovementGravity(BaseCharacter *ch) {
    if(IS_NPC(ch)) return 0.0;
    auto gravity = ch->currentGravity();
    return (gravity * gravity) * ch->getBurdenRatio();
}

/* This handles teleporting players with instant transmission or skills like it. */
void handle_teleport(BaseCharacter *ch, BaseCharacter *tar, int location) {
    int success = false;

    if (location != 0) { /* Teleport to a particular room */
        ch->removeFromLocation();
        ch->addToLocation(getWorld(location));
        success = true;
    } else if (tar != nullptr) { /* Teleport to a particular character */
        ch->removeFromLocation();
        ch->addToLocation(tar->getRoom());
        success = true;
    }

    if (success == true) { /* We have made it. */
        auto r = ch->getRoom();
        act("@w$n@w appears in an instant out of nowhere!@n", true, ch, nullptr, nullptr, TO_ROOM);
        if (auto drag = DRAGGING(ch); drag && !IS_NPC(drag)) {
            drag->removeFromLocation();
            drag->addToLocation(r);
            act("@w$n@w appears in an instant out of nowhere being dragged by $N!@n", true, drag, nullptr, ch,
                TO_NOTVICT);
        }
        if (auto grap = GRAPPLING(ch); grap && !IS_NPC(grap)) {
            grap->removeFromLocation();
            grap->addToLocation(r);
            act("@w$n@w appears in an instant out of nowhere being grappled by $N!@n", true, grap, nullptr, ch,
                TO_NOTVICT);
        }
        if (auto car = CARRYING(ch); car) {
            car->removeFromLocation();
            car->addToLocation(r);
            act("@w$n@w appears in an instant out of nowhere being carried by $N!@n", true, car, nullptr, ch,
                TO_NOTVICT);
        }
        if (auto grap = GRAPPLED(ch); grap && !IS_NPC(grap)) {
            grap->removeFromLocation();
            grap->addToLocation(r);
            act("@w$n@w appears in an instant out of nowhere being grappled by $N!@n", true, grap, nullptr, ch,
                TO_NOTVICT);
        }
        if (auto drag = DRAGGING(ch); drag && IS_NPC(drag)) {
            act("@WYou stop dragging @C$N@W!@n", true, ch, nullptr, drag, TO_CHAR);
            act("@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, drag, TO_ROOM);
            DRAGGED(drag) = nullptr;
            DRAGGING(ch) = nullptr;
        }
        if (auto grap = GRAPPLING(ch); grap && IS_NPC(grap)) {
            GRAPTYPE(grap) = -1;
            GRAPPLED(grap) = nullptr;
            GRAPPLING(ch) = nullptr;
            GRAPTYPE(ch) = -1;
        }
        if (auto grap = GRAPPLED(ch); grap && IS_NPC(grap)) {
            GRAPTYPE(grap) = -1;
            GRAPPLING(grap) = nullptr;
            GRAPPLED(ch) = nullptr;
            GRAPTYPE(ch) = -1;
        }
    } else { /* Wut... */
        basic_mud_log("ERROR: handle_teleport called without a destination.");
        return;
    }

}

/* Let's carry someone! Why not? - Iovan */
ACMD(do_carry) {

    if (IS_NPC(ch))
        return;

    BaseCharacter *vict = nullptr;
    char arg[MAX_INPUT_LENGTH];

    if (DRAGGING(ch)) {
        ch->sendf("You are busy dragging someone at the moment.\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_PILOTING)) {
        ch->sendf("You are busy piloting a ship!\r\n");
        return;
    }

    if (CARRYING(ch)) { /* Already carrying someone. Put them down. Simple and clean. */
        if (GET_ALIGNMENT(ch) > 50) {
            carry_drop(ch, 0);
        } else {
            carry_drop(ch, 1);
        }
        return;
    } else { /* So not carrying already. */
        one_argument(argument, arg);

        if (!*arg) {
            ch->sendf("You want to carry who?\r\n");
            return;
        }

        if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM))) {
            ch->sendf("That person isn't here.\r\n");
            return;
        }

        if (IS_NPC(vict)) {
            ch->sendf("There's no point in carrying them.\r\n");
            return;
        }

        if (CARRIED_BY(vict) != nullptr) {
            ch->sendf("Someone is already carrying them!\r\n");
            return;
        }

        if (GET_POS(vict) > POS_SLEEPING) {
            ch->sendf("They are not unconcious.\r\n");
            return;
        }

        if (vict->getTotalWeight() > CAN_CARRY_W(ch)) {
            act("@WYou try to pick up @C$N@W but have to put them down. They are too heavy for you at the moment.@n",
                true, ch, nullptr, vict, TO_CHAR);
            act("@C$n@W tries to pick up @c$N@W. After struggling for a moment $e has to put $M down.@n", true, ch,
                nullptr, vict, TO_NOTVICT);
            WAIT_STATE(ch, PULSE_1SEC);
            return;
        } else { /* Let's carry that mofo! */
            act("@WYou pick up @C$N@W and put $M over your shoulder.@n", true, ch, nullptr, vict, TO_CHAR);
            act("@C$n@W picks up $c$N@W and puts $M over $s shoulder.@n", true, ch, nullptr, vict, TO_NOTVICT);
            if (SITS(vict)) {
                Object *chair = SITS(vict);
                SITTING(chair) = nullptr;
                SITS(vict) = nullptr;
            }
            CARRYING(ch) = vict;
            CARRIED_BY(vict) = ch;
            WAIT_STATE(ch, PULSE_1SEC);
            return;
        }

    } /* End new carry target. */
}

/* Handles dropping someone you are carrying. */
void carry_drop(BaseCharacter *ch, int type) {


    BaseCharacter *vict = nullptr;

    vict = CARRYING(ch);

    switch (type) {
        case 0: /* Awww we were gentle >.> */
            act("@WYou gently set @C$N@W down on the ground.@n", true, ch, nullptr, vict, TO_CHAR);
            act("@C$n @Wgently sets you down on the ground.@n", true, ch, nullptr, vict, TO_VICT);
            act("@C$n @Wgently sets @c$N@W down on the ground.@n", true, ch, nullptr, vict, TO_NOTVICT);
            break;
        case 1: /* We're not super nice. */
            act("@WYou set @C$N@W hastily onto the ground.@n", true, ch, nullptr, vict, TO_CHAR);
            act("@C$n @Wsets you hastily onto the ground.@n", true, ch, nullptr, vict, TO_VICT);
            act("@C$n @Wsets @c$N@W hastily onto the ground.@n", true, ch, nullptr, vict, TO_NOTVICT);
            break;
        case 2: /* Uh oh we dropped them from being hit! */
            act("@WYou have @C$N@W knocked out of your arms and onto the ground!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@WYou are knocked out of @C$n's@W arms and onto the ground!@n", true, ch, nullptr, vict, TO_VICT);
            act("@C$n @Whas @c$N@W knocked out of $s arms and onto the ground!@n", true, ch, nullptr, vict, TO_NOTVICT);
            break;
        case 3: /* Uh oh they are being extracted! */
            act("@WYou stop carrying @C$N@W for some reason.@n", true, ch, nullptr, vict, TO_CHAR);
            act("@C$n @Wstops carrying you for some reason.@n", true, ch, nullptr, vict, TO_VICT);
            act("@C$n @Wstops carrying @c$N@W for some reason.@n", true, ch, nullptr, vict, TO_NOTVICT);
            break;
    }
    CARRYING(ch) = nullptr;
    CARRIED_BY(vict) = nullptr;
}

std::optional<room_vnum> land_location(char *arg, std::set<room_vnum>& rooms) {
    std::vector<std::pair<room_vnum, std::string>> names;
    for(auto r : rooms) {
        if(auto room = getWorld<Room>(r); room)
        names.emplace_back(r, processColors(room->getDisplayName(nullptr), false, nullptr));
    }

    std::sort(names.begin(), names.end(), [](const std::pair<room_vnum, std::string>& a, const std::pair<room_vnum, std::string>& b) {
        return a.second < b.second;
    });

    for(auto& name : names) {
        if(istarts_with(name.second, arg)) {
            return name.first;
        }
    }
    return std::nullopt;

}

static std::set<vnum> _areaRecurseGuard;


/* This shows the player what locations the planet has to land at. */
static void disp_locations(BaseCharacter *ch, vnum areaVnum, std::set<room_vnum>& rooms) {
	/*
    auto &a = areas[areaVnum];
    if(rooms.empty()) {
        ch->sendf("There are no landing locations on this planet.\r\n");
        return;
    }

    std::vector<std::string> names;
    for(auto r : rooms) {
        auto room = world.find(r);
        if(room == world.end()) continue;
        names.emplace_back(room->second->name);
    }
    // Sort the names vector...
    std::sort(names.begin(), names.end());
    ch->sendf("@D------------------[ %s ]@D------------------\n", a.name.c_str());
    for(auto &name : names) {
        ch->sendf("%s\n", name.c_str());
    }
    */
}

ACMD(do_land) {

    /*
    int above_planet = true, inroom = GET_ROOM_VNUM(IN_ROOM(ch));
    skip_spaces(&argument);
    std::function<bool(area_data&)> governingCelestial = [&](area_data& area) {
        return area.type == AreaType::CelestialBody;
    };
    auto onPlanet = governingAreaTypeFor(ch, governingCelestial);

    std::set<room_vnum> rooms;
    std::function<bool(room_data&)> scan = [&](room_data &r) {
        return r.checkFlag(FlagType::Room, ROOM_LANDING);
    };
    std::size_t count = 0;

    if(onPlanet) {
        auto &a = areas[onPlanet.value()];
        count = recurseScanRooms(a, rooms, scan);
        _areaRecurseGuard.clear();
        above_planet = (a.extraVn && inroom == a.extraVn.value());
    } else {
        above_planet = false;
    }

    if (!*argument) {
        if (above_planet == true) {
            ch->sendf("Land where?\n");
            disp_locations(ch, onPlanet.value(), rooms);
            return;
        } else {
            ch->sendf("You are not even in the lower atmosphere of a planet!\r\n");
            return;
        }
    }

    auto checkLanding = land_location(argument, rooms);
    if(!checkLanding) {
        ch->sendf("You can't land there.\r\n");
        return;
    }
    auto landing = checkLanding.value();

    if (landing != NOWHERE) {
        auto was_in = GET_ROOM_VNUM(IN_ROOM(ch));
        auto r = dynamic_cast<room_data*>(world[landing]);
        ch->sendf(
                     "You descend through the upper atmosphere, and coming down through the clouds you land quickly on the ground below.\r\n");
        std::string landName = "UNKNOWN";
        if(r->area) {
            auto &a = areas[r->area.value()];
            landName = a.name;
        }
        char sendback[MAX_INPUT_LENGTH];
        sprintf(sendback, "@C$n@Y flies down through the atmosphere toward @G%s@Y!@n", landName.c_str());
        act(sendback, true, ch, nullptr, nullptr, TO_ROOM);
        ch->removeFromLocation();
        ch->addToLocation(getWorld(landing));
        fly_planet(landing, "can be seen landing from space nearby!@n\r\n", ch);
        send_to_sense(1, "landing on the planet", ch);
        send_to_scouter("A powerlevel signal has been detected landing on the planet", ch, 0, 1);
        act("$n comes down from high above in the sky and quickly lands on the ground.", true, ch, nullptr, nullptr,
            TO_ROOM);
        return;
    }
    */
}


/* simple function to determine if char can walk on water */
static int has_boat(BaseCharacter *ch) {
    Object *obj;
    int i;

/*
  if (ROOM_IDENTITY(IN_ROOM(ch)) == DEAD_SEA)
    return (1);
*/

    if (ADM_FLAGGED(ch, ADM_WALKANYWHERE) || GET_ADMLEVEL(ch) > 4)
        return (1);

    if (AFF_FLAGGED(ch, AFF_WATERWALK))
        return (1);

    /* non-wearable boats in inventory will do it */
    auto isBoat = [](const auto& o) {return GET_OBJ_TYPE(o) == ITEM_BOAT;};
    if(ch->findObject(isBoat)) return true;

    return false;
}

/* simple function to determine if char can fly */
static int has_flight(BaseCharacter *ch) {
    Object *obj;

    if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
        return (1);

    if (AFF_FLAGGED(ch, AFF_FLYING) &&
        (ch->getCurKI()) >= (GET_LEVEL(ch) + (GET_MAX_MANA(ch) / (GET_LEVEL(ch) * 30))) && !IS_ANDROID(ch) &&
        !IS_NPC(ch)) {
        return (1);
    }
    if (AFF_FLAGGED(ch, AFF_FLYING) && (ch->getCurKI()) < (GET_LEVEL(ch) + (GET_MAX_MANA(ch) / (GET_LEVEL(ch) * 30))) &&
        !IS_ANDROID(ch) && !IS_NPC(ch)) {
        act("@WYou crash to the ground, too tired to fly anymore!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n@W crashes to the ground!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->clearFlag(FlagType::Affect,AFF_FLYING);
        handle_fall(ch);
        return (0);
    }
    if (AFF_FLAGGED(ch, AFF_FLYING) && IS_ANDROID(ch)) {
        return (1);
    }
    if (AFF_FLAGGED(ch, AFF_FLYING) && IS_NPC(ch)) {
        return (1);
    }

    /* flying items in inventory will do it */
    auto givesFlight = [](const auto&o) {return OBJAFF_FLAGGED(o, AFF_FLYING);};
    return ch->findObject(givesFlight) != nullptr;
}

/* simple function to determine if char can breathe non-o2 */
int has_o2(BaseCharacter *ch) {
    if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
        return (1);

    if (AFF_FLAGGED(ch, AFF_WATERBREATH))
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



ACMD(do_move) {
    if (IS_NPC(ch)) {
        ch->moveInDirection(subcmd - 1, 0);
        return;
    }
    if (PLR_FLAGGED(ch, PLR_SELFD)) {
        ch->sendf("You are preparing to blow up!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_LIQUEFIED)) {
        ch->sendf("You are liquefied right now!\r\n");
        return;
    }
    if (GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .51) {
        ch->sendf(
                     "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    } else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .5 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .51) &&
               GET_SKILL(ch, SKILL_CONCENTRATION) < 100) {
        ch->sendf(
                     "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    } else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .4 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .5) &&
               GET_SKILL(ch, SKILL_CONCENTRATION) < 80) {
        ch->sendf(
                     "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    } else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .3 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .4) &&
               GET_SKILL(ch, SKILL_CONCENTRATION) < 70) {
        ch->sendf(
                     "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    } else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .2 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .3) &&
               GET_SKILL(ch, SKILL_CONCENTRATION) < 60) {
        ch->sendf(
                     "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    }

    if (GET_COND(ch, DRUNK) > 4 && (rand_number(1, 9) + GET_COND(ch, DRUNK)) >= rand_number(14, 20)) {
        ch->sendf("You wobble around and then fall on your ass.\r\n");
        act("@C$n@W wobbles around before falling on $s ass@n.", true, ch, nullptr, nullptr, TO_ROOM);
        GET_POS(ch) = POS_SITTING;
        return;
    }

    if (FIGHTING(ch) && !IS_NPC(ch)) {
        char blah[MAX_INPUT_LENGTH];
        sprintf(blah, "%s", dirs[subcmd - 1]);
        do_flee(ch, blah, 0, 0);
        return;
    }

    /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
    if (PLR_FLAGGED(ch, PLR_PILOTING)) {
        Object *vehicle = nullptr, *controls = nullptr;
        int noship = false;
        if (!(controls = find_control(ch)) && GET_ADMLEVEL(ch) < 1) {
            noship = true;
        } else if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0)))) {
            noship = true;
        }
        if (noship == true) {
            ch->sendf("Your ship controls are not here or your ship was not found, report to Iovan!\r\n");
            return;
        } else if (controls != nullptr && vehicle != nullptr) {
            if (GET_FUEL(controls) <= 0) {
                ch->sendf("The ship is out of fuel!\r\n");
                return;
            }
            drive_in_direction(ch, vehicle, subcmd - 1);
            if (GET_OBJ_VAL(controls, 1) == 1) {
                WAIT_STATE(ch, PULSE_2SEC);
            } else if (GET_OBJ_VAL(controls, 1) == 2) {
                WAIT_STATE(ch, PULSE_1SEC);
            }
            controls = nullptr;
            vehicle = nullptr;
            return;
        }
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT)) {
        ch->sendf("You are inside a healing tank!\r\n");
        return;
    }
    if (!IS_NPC(ch)) {
        int fail = false;
        for (auto obj : ch->getRoom()->getInventory()) {
            if (KICHARGE(obj) > 0 && USER(obj) == ch) {
                fail = true;
            }
        }
        if (fail == true) {
            ch->sendf("You are too busy controlling your attack!\r\n");
            return;
        }
    }

    if (!IS_NPC(ch) && GET_LIMBCOND(ch, 0) <= 0 && GET_LIMBCOND(ch, 1) <= 0 && GET_LIMBCOND(ch, 2) <= 0 &&
        GET_LIMBCOND(ch, 3) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING)) {
        ch->sendf("Unless you fly, you can't get far with no limbs.\r\n");
        return;
    }
    if (GRAPPLING(ch) || GRAPPLED(ch)) {
        ch->sendf("You are grappling with someone!\r\n");
        return;
    }
    if (ABSORBING(ch)) {
        ch->sendf("You are busy absorbing from %s!\r\n", GET_NAME(ABSORBING(ch)));
        return;
    }
    if (auto ab = ABSORBBY(ch) ; ab) {
        if (axion_dice(0) < GET_SKILL(ab, SKILL_ABSORB)) {
            ch->sendf("You are being held by %s, they are absorbing you!\r\n", GET_NAME(ab));
            ab->sendf("%s struggles in your grasp!\r\n", GET_NAME(ch));
            WAIT_STATE(ch, PULSE_2SEC);
            return;
        } else {
            act("@c$N@W manages to break loose of @C$n's@W hold!@n", true, ab, nullptr, ch, TO_NOTVICT);
            act("@WYou manage to break loose of @C$n's@W hold!@n", true, ab, nullptr, ch, TO_VICT);
            act("@c$N@W manages to break loose of your hold!@n", true, ab, nullptr, ch, TO_CHAR);
            ABSORBING(ab) = nullptr;
            ABSORBBY(ch) = nullptr;
        }
    }
    if (!block_calc(ch)) {
        return;
    }
    if (GET_EAVESDROP(ch) > 0) {
        ch->sendf("You stop eavesdropping.\r\n");
        GET_EAVESDROP(ch) = real_room(0);
    }
    if (!IS_NPC(ch)) {
        if (PRF_FLAGGED(ch, PRF_ARENAWATCH)) {
            ch->clearFlag(FlagType::Pref, PRF_ARENAWATCH);
            ARENA_IDNUM(ch) = -1;
        }
        if (GET_ROOM_VNUM(IN_ROOM(ch)) != NOWHERE && GET_ROOM_VNUM(IN_ROOM(ch)) != 0 &&
            GET_ROOM_VNUM(IN_ROOM(ch)) != 1) {
            GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
        }

        auto ratio = ch->getBurdenRatio();
        if(ratio >= 1.0) {
            ch->sendf("Your immense burden hinders your progress.\r\n");
            WAIT_STATE(ch, std::min<int>(PULSE_3SEC * ratio, PULSE_5SEC));
        }

        if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE) && GET_ADMLEVEL(ch) < 1) {
            ch->sendf("You struggle to cross the vast distance.\r\n");
            WAIT_STATE(ch, PULSE_6SEC);
        } else if ((GET_LIMBCOND(ch, 2) <= 0 && GET_LIMBCOND(ch, 3) <= 0) && GET_LIMBCOND(ch, 0) <= 0 &&
                   !AFF_FLAGGED(ch, AFF_FLYING)) {
            act("@wYou slowly pull yourself along with your arm...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w slowly pulls $mself along with one arm...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 1) < 50) {
                ch->sendf("@RYour left arm is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 1) -= rand_number(1, 5);
                if (GET_LIMBCOND(ch, 0) <= 0) {
                    act("@RYour left arm falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R left arm falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_5SEC);
        } else if ((GET_LIMBCOND(ch, 2) <= 0 && GET_LIMBCOND(ch, 3) <= 0) && GET_LIMBCOND(ch, 1) <= 0 &&
                   !AFF_FLAGGED(ch, AFF_FLYING)) {
            act("@wYou slowly pull yourself along with your arm...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w slowly pulls $mself along with one arm...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 0) < 50) {
                ch->sendf("@RYour right arm is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 0) -= rand_number(1, 5);
                if (GET_LIMBCOND(ch, 0) <= 0) {
                    act("@RYour right arm falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R right arm falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_5SEC);
        } else if ((GET_LIMBCOND(ch, 2) <= 0 && GET_LIMBCOND(ch, 3) <= 0) && !AFF_FLAGGED(ch, AFF_FLYING)) {
            act("@wYou slowly pull yourself along with your arms...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w slowly pulls $mself along with one arms...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 1) < 50) {
                ch->sendf("@RYour left arm is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 1) -= rand_number(1, 5);
                if (GET_LIMBCOND(ch, 1) <= 0) {
                    act("@RYour left arm falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R left arm falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            if (GET_LIMBCOND(ch, 0) < 50) {
                ch->sendf("@RYour right arm is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 0) -= rand_number(1, 5);
                if (GET_LIMBCOND(ch, 0) <= 0) {
                    act("@RYour right arm falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R right arm falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_3SEC);
        } else if (GET_LIMBCOND(ch, 2) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING)) {
            act("@wYou hop on one leg...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w hops on one leg...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 3) < 50) {
                ch->sendf("@RYour left leg is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 3) -= rand_number(1, 5);
                if (GET_LIMBCOND(ch, 3) <= 0) {
                    act("@RYour left leg falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R left leg falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_2SEC);
        } else if (GET_LIMBCOND(ch, 3) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING)) {
            act("@wYou hop on one leg...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w hops on one leg...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 2) < 50) {
                ch->sendf("@RYour right leg is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 2) -= rand_number(1, 5);
                if (GET_LIMBCOND(ch, 2) <= 0) {
                    act("@RYour right leg falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R right leg falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_2SEC);
        } else if (GET_POS(ch) == POS_RESTING) {
            act("@wYou crawl on your hands and knees.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w crawls on $s hands and knees.@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (SITS(ch)) {
                Object *chair = SITS(ch);
                SITTING(chair) = nullptr;
                SITS(ch) = nullptr;
            }
            WAIT_STATE(ch, PULSE_3SEC);
        } else if (GET_POS(ch) == POS_SITTING) {
            act("@wYou shuffle on your hands and knees.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w shuffles on $s hands and knees.@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (SITS(ch)) {
                Object *chair = SITS(ch);
                SITTING(chair) = nullptr;
                SITS(ch) = nullptr;
            }
            WAIT_STATE(ch, PULSE_2SEC);
        } else if (GET_POS(ch) < POS_RESTING) {
            ch->sendf("You are in no condition to move! Try standing...\r\n");
            return;
        }
    }
    if(ch->moveInDirection(subcmd - 1, 0)) {
        ch->lookAtLocation();
    }
    if (GET_RDISPLAY(ch)) {
        if (GET_RDISPLAY(ch) != "Empty") {
            GET_RDISPLAY(ch) = "Empty";
        }
    }
}


static int has_key(BaseCharacter *ch, obj_vnum key) {
    return ch->findObjectVnum(key) != nullptr;
}

#define NEED_OPEN    (1 << 0)
#define NEED_CLOSED    (1 << 1)
#define NEED_UNLOCKED    (1 << 2)
#define NEED_LOCKED    (1 << 3)

const char *cmd_door[NUM_DOOR_CMD] =
        {
                "open",
                "close",
                "unlock",
                "lock",
                "pick"
        };

static const int flags_door[] =
        {
                NEED_CLOSED | NEED_UNLOCKED,
                NEED_OPEN,
                NEED_CLOSED | NEED_LOCKED,
                NEED_CLOSED | NEED_UNLOCKED,
                NEED_CLOSED | NEED_LOCKED
        };


static int ok_pick(BaseCharacter *ch, obj_vnum keynum, int pickproof, int dclock, int scmd, Object *hatch) {
    int skill_lvl, found = false;
    Object *obj, *next_obj;
    obj = ch->findObjectVnum(18);

    if (scmd != SCMD_PICK)
        return (1);

    /* PICKING_LOCKS is not an untrained skill */
    if (!GET_SKILL(ch, SKILL_OPEN_LOCK)) {
        ch->sendf("You have no idea how!\r\n");
        return (0);
    }
    if (found == false) {
        ch->sendf("You need a lock picking kit.\r\n");
        return (0);
    }
    if (hatch != nullptr && (GET_OBJ_TYPE(hatch) == ITEM_HATCH || GET_OBJ_TYPE(hatch) == ITEM_VEHICLE)) {
        ch->sendf("No picking ship hatches.\r\n");
        hatch = nullptr;
        return (0);
    }
    skill_lvl = roll_skill(ch, SKILL_OPEN_LOCK);
    if (dclock == 0) {
        dclock = rand_number(1, 101);
    }

    if (keynum == NOTHING) {
        ch->sendf("Odd - you can't seem to find a keyhole.\r\n");
    } else if (pickproof) {
        ch->sendf("It resists your attempts to pick it.\r\n");
        act("@c$n@w puts a set of lockpick tools away.@n", true, ch, nullptr, nullptr, TO_ROOM);
        /* The -2 is here because that is a penality for not having a set of
   * thieves' tools. If the player has them, that modifier will be accounted
   * for in roll_skill, and negate (or surpass) this.
   */
    } else if ((ch->getCurST()) < GET_MAX_MOVE(ch) / 30) {
        ch->sendf("You don't have the stamina to try, it takes percision to pick locks."
                         "Not shaking tired hands.\r\n");
    } else if (dclock > (skill_lvl - 2)) {
        ch->sendf("You failed to pick the lock...\r\n");
        act("@c$n@w puts a set of lockpick tools away.@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->decCurST(ch->getCurST() / 30);
    } else {
        ch->decCurST(ch->getCurST() / 30);
        return (1);
    }

    return (0);
}


ACMD(do_gen_door) {

}

static int do_simple_enter(BaseCharacter *ch, Object *obj, int need_specials_check) {
    room_rnum dest_room = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST));
    room_rnum was_in = IN_ROOM(ch);
    int need_movement = 0;

    /* charmed? */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
        IN_ROOM(ch) == IN_ROOM(ch->master)) {
        ch->sendf("The thought of leaving your master makes you weep.\r\n");
        act("$n bursts into tears.", false, ch, nullptr, nullptr, TO_ROOM);
        return (0);
    }

    /* move points needed is avg. move loss for src and destination sect type */
    need_movement = calcNeedMovementGravity(ch);

    if (GET_LEVEL(ch) <= 1) {
        need_movement = 0;
    }
    if ((ch->getCurST()) < need_movement && !AFF_FLAGGED(ch, AFF_FLYING) && !IS_NPC(ch)) {
        if (need_specials_check && ch->master)
            ch->sendf("You are too exhausted to follow.\r\n");
        else
            ch->sendf("You are too exhausted.\r\n");

        return (0);
    }

    if (ROOM_FLAGGED(dest_room, ROOM_TUNNEL) &&
        num_pc_in_room(getWorld<Room>(dest_room)) >= CONFIG_TUNNEL_SIZE) {
        if (CONFIG_TUNNEL_SIZE > 1)
            ch->sendf("There isn't enough room for you to go there!\r\n");
        else
            ch->sendf("There isn't enough room there for more than one person!\r\n");
        return (0);
    }
    /* Mortals and low level gods cannot enter greater god rooms. */
    if (ROOM_FLAGGED(dest_room, ROOM_GODROOM) &&
        GET_ADMLEVEL(ch) < ADMLVL_GRGOD) {
        ch->sendf("You aren't godly enough to use that room!\r\n");
        return (0);
    }
    /* Now we know we're allowed to go into the room. */
    if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)) && !AFF_FLAGGED(ch, AFF_FLYING))
        ch->decCurST(need_movement);

    act("$n enters $p.", true, ch, obj, nullptr, TO_ROOM | TO_SNEAKRESIST);

    if (DRAGGING(ch)) {
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, DRAGGING(ch), TO_ROOM);
    }
    if (CARRYING(ch)) {
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, CARRYING(ch), TO_ROOM);
    }
    ch->removeFromLocation();
    ch->addToLocation(getWorld(dest_room));

    /* move them first, then move them back if they aren't allowed to go. */
    /* see if an entry trigger disallows the move */
    if (!entry_mtrigger(ch)) {
        ch->removeFromLocation();
        ch->addToLocation(getWorld(was_in));
        return 0;
    }

    if (GET_OBJ_TYPE(obj) == ITEM_PORTAL)
        act("$n arrives from $p.", false, ch, obj, nullptr, TO_ROOM | TO_SNEAKRESIST);
    else
        act("$n arrives from outside.", false, ch, nullptr, nullptr, TO_ROOM | TO_SNEAKRESIST);
    
    auto r = ch->getRoom();
    if (auto drag = DRAGGING(ch); drag) {
        act("@wYou drag @C$N@w with you.@n", true, ch, nullptr, drag, TO_CHAR);
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, drag, TO_ROOM);
        if (!AFF_FLAGGED(drag, AFF_KNOCKED) && !AFF_FLAGGED(drag, AFF_SLEEP) && rand_number(1, 3)) {
            drag->sendf("You feel your sleeping body being moved.\r\n");
            if (IS_NPC(drag) && !FIGHTING(drag)) {
                set_fighting(drag, ch);
            }
        }
        
        drag->removeFromLocation();
        drag->addToLocation(r);
        if (auto s = SITS(drag); s) {
            s->removeFromLocation();
            s->addToLocation(r);
        }
    }
    if (auto carry = CARRYING(ch); carry) {
        act("@wYou carry @C$N@w with you.@n", true, ch, nullptr, carry, TO_CHAR);
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, carry, TO_ROOM);
        if (!AFF_FLAGGED(carry, AFF_KNOCKED) && !AFF_FLAGGED(carry, AFF_SLEEP) && rand_number(1, 3)) {
            carry->sendf("You feel your sleeping body being moved.\r\n");
        }
        carry->removeFromLocation();
        carry->addToLocation(ch->getRoom());
        if (auto s = SITS(carry); s) {
            s->removeFromLocation();
            s->addToLocation(r);
        }
    }

    if (ch->desc != nullptr)
        ch->lookAtLocation();

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
        log_death_trap(ch);
        death_cry(ch);
        extract_char(ch);
        return 0;
    }

    entry_memory_mtrigger(ch);
    greet_memory_mtrigger(ch);

    return 1;
}

static int perform_enter_obj(BaseCharacter *ch, Object *obj, int need_specials_check) {
    room_rnum was_in = IN_ROOM(ch);
    int could_move = false;
    struct follow_type *k;

    if (GRAPPLING(ch) || GRAPPLED(ch)) {
        ch->sendf("You are grappling with someone!\r\n");
        return (0);
    }

    if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE ||
        GET_OBJ_TYPE(obj) == ITEM_PORTAL) {
        if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
            ch->sendf("But it's closed!\r\n");
        } else if ((GET_OBJ_VAL(obj, VAL_PORTAL_DEST) != NOWHERE) &&
                   (real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)) != NOWHERE)) {
            if (GET_OBJ_VAL(obj, VAL_PORTAL_DEST) >= 45000 && GET_OBJ_VAL(obj, VAL_PORTAL_DEST) <= 45099) {
                BaseCharacter *tch, *next_v;
                int filled = false;
                for (auto tch : getWorld<Room>(GET_OBJ_VAL(obj, VAL_PORTAL_DEST))->getPeople()) {
                    if (tch) {
                        filled = true;
                    }
                }
                if (filled == true) {
                    ch->sendf("Only one person can fit in there at a time.\r\n");
                    return (0);
                }
            }
            if ((could_move = do_simple_enter(ch, obj, need_specials_check)))
                for (k = ch->followers; k; k = k->next)
                    if ((IN_ROOM(k->follower) == was_in) &&
                        (GET_POS(k->follower) >= POS_STANDING)) {
                        act("You follow $N.\r\n", false, k->follower, nullptr, ch, TO_CHAR);
                        perform_enter_obj(k->follower, obj, 1);
                    }
        } else {
            ch->sendf(
                         "It doesn't look like you can enter it at the moment.\r\n");
        }
    } else {
        ch->sendf("You can't enter that!\r\n");
    }
    return could_move;
}

ACMD(do_enter) {
    /*
    struct obj_data *obj = nullptr;
    char buf[MAX_INPUT_LENGTH];
    int door, move_dir = -1;

    one_argument(argument, buf);
    auto r = ch->getRoom();

    if (*buf) {

        obj = get_obj_in_list_vis(ch, buf, nullptr, ch->getRoom()->getInventory());

        if (!obj)
            obj = get_obj_in_list_vis(ch, buf, nullptr, ch->getInventory());

        if (!obj)
            obj = get_obj_in_equip_vis(ch, buf, nullptr, ch->getEquipment());

        if (obj)
            perform_enter_obj(ch, obj, 0);

        else {
            for (auto &[door, e] : r->getExits())
                if (auto kw = e->getKeywords(ch);r->dir_option[door]->keyword)
                    if (isname(buf, r->dir_option[door]->keyword))
                        move_dir = door;

            if (move_dir > -1)
                perform_move(ch, move_dir, 1);
            else
                ch->sendf("There is no %s here.\r\n", buf);
        }
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS)) {
        ch->sendf("You are already indoors.\r\n");
    } else {

        for (door = 0; door < NUM_OF_DIRS; door++)
            if (r->dir_option[door])
                if (r->dir_option[door]->to_room != NOWHERE)
                    if (!EXIT_FLAGGED(r->dir_option[door], EX_CLOSED) &&
                        ROOM_FLAGGED(r->dir_option[door]->to_room, ROOM_INDOORS))
                        move_dir = door;
        if (move_dir > -1)
            perform_move(ch, move_dir, 1);
        else
            ch->sendf("You can't seem to find anything to enter.\r\n");
    }
    */
}

static int do_simple_leave(BaseCharacter *ch, Object *obj, int need_specials_check) {
    room_rnum was_in = IN_ROOM(ch), dest_room = NOWHERE;
    int need_movement = 0;
    Object *vehicle = nullptr;

    if (GET_OBJ_TYPE(obj) != ITEM_PORTAL) {
        vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(obj, VAL_HATCH_DEST));
    }

    if (vehicle == nullptr && GET_OBJ_TYPE(obj) != ITEM_PORTAL) {
        ch->sendf("That doesn't appear to lead anywhere.\r\n");
        return 0;
    }

    if (GET_OBJ_TYPE(obj) == ITEM_PORTAL && OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
        ch->sendf("But it's closed!\r\n");
        return 0;
    }

    if (vehicle != nullptr) {
        if ((dest_room = IN_ROOM(vehicle)) == NOWHERE) {
            ch->sendf("That doesn't appear to lead anywhere.\r\n");
            return 0;
        }
    }
    if (vehicle == nullptr) {
        if ((dest_room = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST))) == NOWHERE) {
            ch->sendf("That doesn't appear to lead anywhere.\r\n");
            return 0;
        }
    }

    /* charmed? */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
        IN_ROOM(ch) == IN_ROOM(ch->master)) {
        ch->sendf("The thought of leaving your master makes you weep.\r\n");
        act("$n bursts into tears.", false, ch, nullptr, nullptr, TO_ROOM);
        return (0);
    }

    /* move points needed is avg. move loss for src and destination sect type */
    need_movement = calcNeedMovementGravity(ch);
    if (GET_LEVEL(ch) <= 1) {
        need_movement = 0;
    }
    if ((ch->getCurST()) < need_movement && !AFF_FLAGGED(ch, AFF_FLYING) && !IS_NPC(ch)) {
        if (need_specials_check && ch->master)
            ch->sendf("You are too exhausted to follow.\r\n");
        else
            ch->sendf("You are too exhausted.\r\n");

        return (0);
    }

    if (ROOM_FLAGGED(dest_room, ROOM_TUNNEL) &&
        num_pc_in_room(getWorld<Room>(dest_room)) >= CONFIG_TUNNEL_SIZE) {
        if (CONFIG_TUNNEL_SIZE > 1)
            ch->sendf("There isn't enough room for you to go there!\r\n");
        else
            ch->sendf("There isn't enough room there for more than one person!\r\n");
        return (0);
    }
    /* Now we know we're allowed to go into the room. */
    if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)) && !AFF_FLAGGED(ch, AFF_FLYING))
        ch->decCurST(need_movement);

    act("$n leaves $p.", true, ch, vehicle, nullptr, TO_ROOM | TO_SNEAKRESIST);

    if (DRAGGING(ch)) {
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, DRAGGING(ch), TO_ROOM);
    }
    if (CARRYING(ch)) {
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, CARRYING(ch), TO_ROOM);
    }
    ch->removeFromLocation();
    ch->addToLocation(getWorld(dest_room));

    /* move them first, then move them back if they aren't allowed to go. */
    /* see if an entry trigger disallows the move */
    if (!entry_mtrigger(ch)) {
        ch->removeFromLocation();
        ch->addToLocation(getWorld(was_in));
        return 0;
    }

    if (vehicle) {
        act("$n arrives from inside $p.", true, ch, vehicle, nullptr, TO_ROOM | TO_SNEAKRESIST);
    } else {
        act("$n arrives from inside", true, ch, nullptr, nullptr, TO_ROOM | TO_SNEAKRESIST);
    }
    auto r = ch->getRoom();
    if (auto drag = DRAGGING(ch); drag) {
        act("@wYou drag @C$N@w with you.@n", true, ch, nullptr, drag, TO_CHAR);
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, drag, TO_ROOM);
        drag->removeFromLocation();
        drag->addToLocation(r);
        if (auto s = SITS(drag); s) {
            s->removeFromLocation();
            s->addToLocation(r);
        }
        if (!AFF_FLAGGED(drag, AFF_KNOCKED) && !AFF_FLAGGED(drag, AFF_SLEEP) && rand_number(1, 3)) {
            drag->sendf("You feel your sleeping body being moved.\r\n");
            if (IS_NPC(drag) && !FIGHTING(drag)) {
                set_fighting(drag, ch);
            }
        }
    }
    if (auto carry = CARRYING(ch); carry) {
        act("@wYou carry @C$N@w with you.@n", true, ch, nullptr, carry, TO_CHAR);
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, carry, TO_ROOM);
        carry->removeFromLocation();
        carry->addToLocation(r);
        if (auto s = SITS(carry); s) {
            s->removeFromLocation();
            s->addToLocation(r);
        }
        if (!AFF_FLAGGED(carry, AFF_KNOCKED) && !AFF_FLAGGED(carry, AFF_SLEEP) && rand_number(1, 3)) {
            carry->sendf("You feel your sleeping body being moved.\r\n");
        }
    }

    char buf3[MAX_STRING_LENGTH];
    send_to_sense(0, "You sense someone ", ch);
    sprintf(buf3, "@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection range.@n",
            add_commas(GET_HIT(ch)).c_str());
    send_to_scouter(buf3, ch, 0, 0);

    if (ch->desc != nullptr) {
        act(obj->getLookDesc().c_str(), true, ch, obj, nullptr, TO_CHAR);
        ch->lookAtLocation();
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
        log_death_trap(ch);
        death_cry(ch);
        extract_char(ch);
        return 0;
    }

    entry_memory_mtrigger(ch);
    greet_memory_mtrigger(ch);

    return 1;
}

static int perform_leave_obj(BaseCharacter *ch, Object *obj, int need_specials_check) {
    room_rnum was_in = IN_ROOM(ch);
    int could_move = false;
    struct follow_type *k;

    if (GRAPPLING(ch) || GRAPPLED(ch)) {
        ch->sendf("You are grappling with someone!\r\n");
        return (0);
    }

    if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
        ch->sendf("But the way out is closed.\r\n");
    } else {
        if (GET_OBJ_VAL(obj, VAL_HATCH_DEST) != NOWHERE)
            if ((could_move = do_simple_leave(ch, obj, need_specials_check)))
                for (k = ch->followers; k; k = k->next)
                    if ((IN_ROOM(k->follower) == was_in) &&
                        (GET_POS(k->follower) >= POS_STANDING)) {
                        act("You follow $N.\r\n", false, k->follower, nullptr, ch, TO_CHAR);
                        perform_leave_obj(k->follower, obj, 1);
                    }
    }
    return could_move;
}


ACMD(do_leave) {
    int door;

    if (PLR_FLAGGED(ch, PLR_HEALT)) {
        ch->sendf("You are inside a healing tank!\r\n");
        return;
    }

    auto r = ch->getRoom();

    for (auto obj : r->getInventory())
        if (CAN_SEE_OBJ(ch, obj))
            if (GET_OBJ_TYPE(obj) == ITEM_HATCH || GET_OBJ_TYPE(obj) == ITEM_PORTAL) {
                perform_leave_obj(ch, obj, 0);
                return;
            }

    if (OUTSIDE(ch)) {
        ch->sendf("You are outside.. where do you want to go?\r\n");
        return;
    }

    for (auto &[door, e] : r->getExits()) {
        auto dest = e->getDestination();
        if(!dest) continue;
        if (!e->checkFlag(FlagType::Exit, EX_CLOSED) && !dest->checkFlag(FlagType::Room, ROOM_INDOORS)) {
            ch->moveInDirection(door, 1);
            return;
        }
    }

    ch->sendf("I see no obvious exits to the outside.\r\n");
}

static void handle_fall(BaseCharacter *ch) {
    int room = -1;
    while (EXIT(ch, 5) && SECT(IN_ROOM(ch)) == SECT_FLYING) {
        room = EXIT(ch, 5)->getDestination()->getUID();
        ch->removeFromLocation();
        ch->addToLocation(getWorld(room));
        if (auto carry = CARRYING(ch); carry) {
            carry->removeFromLocation();
            carry->addToLocation(getWorld(room));
        }
        if (!EXIT(ch, 5) || SECT(IN_ROOM(ch)) != SECT_FLYING) {
            act("@r$n slams into the ground!@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->decCurHealth(ch->getEffMaxPL() / 20, 1);

            act("@rYou slam into the ground!@n", true, ch, nullptr, nullptr, TO_CHAR);
            ch->lookAtLocation();
        } else {
            act("@r$n pummets down toward the ground below!@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
    }
    if (SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM && !CARRIED_BY(ch) && !IS_KANASSAN(ch)) {
        if ((ch->getCurST()) >= (ch->getCarriedWeight())) {
            act("@bYou swim in place.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@b swims in place.@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->decCurST(ch->getCarriedWeight());
            act("@RYou are drowning!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@b gulps water as $e struggles to stay above the water line.@n", true, ch, nullptr, nullptr,
                TO_ROOM);
            if (GET_HIT(ch) - ((ch->getEffMaxPL()) / 3) <= 0) {
                act("@rYou drown!@n", true, ch, nullptr, nullptr, TO_CHAR);
                act("@R$n@r drowns!@n", true, ch, nullptr, nullptr, TO_ROOM);
                die(ch, nullptr);
                ch->decCurHealthPercent(1, 1);
            } else {
                ch->decCurHealthPercent(.33);
            }
        }
    }

}

static int check_swim(BaseCharacter *ch) {
    auto can = false;

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE)) {
        auto space_cost = (GET_MAX_MANA(ch) / 1000) + ((ch->getCarriedWeight()) / 2);
        if (ch->getCurKI() >= space_cost)
            can = true;
        ch->decCurKI(space_cost);
        if (!can) ch->sendf("You do not have enough ki to fly through space. You are drifting helplessly.\r\n");
        return can;
    } else {
        auto swim_cost = (ch->getCarriedWeight()) - 1;
        if (ch->getCurST() >= swim_cost)
            can = true;
        ch->decCurST(swim_cost);
        if (!can) ch->sendf("You are too tired to swim!\r\n");
        return can;
    }
}


ACMD(do_fly) {
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (ABSORBING(ch) || ABSORBBY(ch)) {
        ch->sendf("You can't fly, you are struggling with someone right now!");
        return;
    }
    if (GRAPPLING(ch) || GRAPPLED(ch)) {
        ch->sendf("You can't fly, you are struggling with someone right now!");
        return;
    }
    if (!IS_NPC(ch)) {
        if (PLR_FLAGGED(ch, PLR_HEALT)) {
            ch->sendf("You are inside a healing tank!\r\n");
            return;
        }
        if (PLR_FLAGGED(ch, PLR_PILOTING)) {
            ch->sendf("You are busy piloting a ship!\r\n");
            return;
        }
    }

    if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_FOCUS) < 30 && !IS_ANDROID(ch)) {
        ch->sendf("You do not have enough focus to hold yourself aloft.\r\n");
        ch->sendf("@wOOC@D: @WYou need the skill Focus at @m30@W.@n\r\n");
        return;
    }

    if (!*arg) {
        if (AFF_FLAGGED(ch, AFF_FLYING) && SECT(IN_ROOM(ch)) != SECT_FLYING && SECT(IN_ROOM(ch)) != SECT_SPACE) {
            act("@WYou slowly settle down to the ground.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n slowly settles down to the ground.@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->clearFlag(FlagType::Affect,AFF_FLYING);
            GET_ALT(ch) = 0;
            return;
        }

        if (AFF_FLAGGED(ch, AFF_FLYING) && SECT(IN_ROOM(ch)) == SECT_FLYING) {
            act("@WYou begin to plummet to the ground!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n starts to pummet to the ground below!@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->clearFlag(FlagType::Affect,AFF_FLYING);
            GET_ALT(ch) = 0;
            handle_fall(ch);
            return;
        }
        if (AFF_FLAGGED(ch, AFF_FLYING) && SECT(IN_ROOM(ch)) == SECT_SPACE) {
            act("@WYou let yourself drift aimlessly through space.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n starts to drift slowly.!@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->clearFlag(FlagType::Affect,AFF_FLYING);
            GET_ALT(ch) = 0;
            return;
        }
        if ((ch->getCurKI()) < GET_MAX_MANA(ch) / 100 && !IS_ANDROID(ch)) {
            ch->sendf("You do not have the ki to fly.");
            return;
        } else {
            reveal_hiding(ch, 0);
            act("@WYou slowly take off into the sky.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n slowly takes off into the sky.@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (SITS(ch)) {
                SITTING(SITS(ch)) = nullptr;
                SITS(ch) = nullptr;
            }
            if (GET_POS(ch) < POS_STANDING) {
                GET_POS(ch) = POS_STANDING;
            }
            ch->setFlag(FlagType::Affect, AFF_FLYING);
            GET_ALT(ch) = 1;
            ch->decCurKI(ch->getMaxKI() / 100);
        }
    }
    if (!strcasecmp("high", arg)) {
        if ((ch->getCurKI()) < GET_MAX_MANA(ch) / 100 && !IS_ANDROID(ch)) {
            ch->sendf("You do not have the ki to fly.");
            return;
        } else {
            reveal_hiding(ch, 0);
            act("@WYou rocket high into the sky.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n rockets high into the sky.@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (SITS(ch)) {
                SITTING(SITS(ch)) = nullptr;
                SITS(ch) = nullptr;
            }
            if (GET_POS(ch) < POS_STANDING) {
                GET_POS(ch) = POS_STANDING;
            }
            ch->setFlag(FlagType::Affect, AFF_FLYING);
            GET_ALT(ch) = 2;
            ch->decCurKI(ch->getMaxKI() / 100);
        }
    }
    if (!strcasecmp("space", arg)) {
        /*
        if (!OUTSIDE(ch)) {
            ch->sendf("You are not outside!");
            return;
        }
        if ((ch->getCurKI()) < GET_MAX_MANA(ch) / 10 && !IS_ANDROID(ch)) {
            ch->sendf("You do not have the ki to fly to space.");
            return;
        }
        if (FIGHTING(ch)) {
            ch->sendf("You are too busy fighting!");
            return;
        }

        auto r = ch->getRoom();
        auto dest = r->getLaunchDestination();

        auto planet = ch->getMatchingArea(area_data::isPlanet);
        if(!dest) {
            ch->sendf("You can't fly to space from here!");
            return;
        }

        reveal_hiding(ch, 0);
        GET_ALT(ch) = 2;
        ch->setFlag(FlagType::Affect, AFF_FLYING);
        if (!block_calc(ch)) {
            return;
        }
        GET_ALT(ch) = 0;
        ch->clearFlag(FlagType::Affect,AFF_FLYING);

        if(planet) {
            fly_planet(IN_ROOM(ch), "can be seen blasting off into space!@n\r\n", ch);
            send_to_sense(1, "leaving the planet", ch);
            send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
        }


        act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        ch->removeFromLocation();
        ch->addToLocation(getWorld(dest.value()));
        if(planet) {
            act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", true, ch, nullptr, nullptr,
            TO_ROOM);
            ch->sendf("@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
        }

        if (!IS_ANDROID(ch)) {
            ch->decCurKI(ch->getMaxKI() / 10);
        }
        WAIT_STATE(ch, PULSE_3SEC);
        return;
        */

    }
}

static void autochair(BaseCharacter *ch, Object *chair) {
    // TODO: Make this configurable.
    SITTING(chair) = nullptr;
    SITS(ch) = nullptr;
    if (CAN_WEAR(chair, ITEM_WEAR_TAKE) && GET_OBJ_TYPE(chair) != ITEM_CHAIR && ch->canCarryWeight(chair)) {
        chair->removeFromLocation();
        chair->addToLocation(ch);
        act("You pick up $p.", true, ch, chair, nullptr, TO_CHAR);
        act("$n picks up $p.", true, ch, chair, nullptr, TO_ROOM);
    }
}

ACMD(do_stand) {
    auto chair = SITS(ch);
    if (AFF_FLAGGED(ch, AFF_KNOCKED)) {
        ch->sendf("You are knocked out cold for right now!\r\n");
        return;
    }
    if (!IS_NPC(ch) && GET_LIMBCOND(ch, 2) <= 0 && GET_LIMBCOND(ch, 3) <= 0) {
        ch->sendf("With what legs will you be standing up on?\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_PILOTING)) {
        ch->sendf("You are busy piloting a ship!\r\n");
        return;
    }

    switch (GET_POS(ch)) {
        case POS_STANDING:
            ch->sendf("You are already standing.\r\n");
            break;
        case POS_SITTING:
            reveal_hiding(ch, 0);
            ch->sendf("You stand up.\r\n");
            act("$n clambers to $s feet.", true, ch, nullptr, nullptr, TO_ROOM);
            if (chair) autochair(ch, chair);
            /* May be sitting for some reason and may still be fighting. */
            GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
            break;
        case POS_RESTING:
            ch->sendf("You stop resting, and stand up.\r\n");
            act("$n stops resting, and clambers to $s feet.", true, ch, nullptr, nullptr, TO_ROOM);
            if (chair) autochair(ch, chair);
            GET_POS(ch) = POS_STANDING;
            break;
        case POS_SLEEPING:
            ch->sendf("You have to wake up first!\r\n");
            break;
        default:
            ch->sendf("You stop floating around, and put your feet on the ground.\r\n");
            act("$n stops floating around, and puts $s feet on the ground.",
                true, ch, nullptr, nullptr, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;
    }
}

ACMD(do_sit) {
    Object *chair = nullptr;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (PLR_FLAGGED(ch, PLR_PILOTING)) {
        ch->sendf("You are busy piloting a ship!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT)) {
        ch->sendf("You are inside a healing tank!\r\n");
        return;
    }

    if (auto drag = DRAGGING(ch); drag) {
        act("@WYou stop dragging @C$N@W!@n", true, ch, nullptr, drag, TO_CHAR);
        act("@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, drag, TO_ROOM);
        DRAGGED(drag) = nullptr;
        DRAGGING(ch) = nullptr;
    }
    if (CARRYING(ch)) {
        ch->sendf("You are busy carrying someone!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_FLYING)) {
        do_fly(ch, nullptr, 0, 0);
    }

    if (!*arg) {
        switch (GET_POS(ch)) {
            case POS_STANDING:
                reveal_hiding(ch, 0);
                ch->sendf("You sit down.\r\n");
                act("$n sits down.", false, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_SITTING;
                break;
            case POS_SITTING:
                ch->sendf("You're sitting already.\r\n");
                break;
            case POS_RESTING:
                ch->sendf("You stop resting, and sit up.\r\n");
                act("$n stops resting.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_SITTING;
                break;
            case POS_SLEEPING:
                ch->sendf("You have to wake up first.\r\n");
                break;
            case POS_FIGHTING:
                ch->sendf("Sit down while fighting? Are you MAD?\r\n");
                break;
            default:
                ch->sendf("You stop floating around, and sit down.\r\n");
                act("$n stops floating around, and sits down.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_SITTING;
                break;
        }
    } else {
        if (SITS(ch)) {
            ch->sendf("You are already on something!\r\n");
            return;
        }
        if (!(chair = get_obj_in_list_vis(ch, arg, nullptr, ch->getRoom()->getInventory()))) {
            ch->sendf("That isn't here.\r\n");
            return;
        }
        if (GET_OBJ_VNUM(chair) == 65) {
            ch->sendf("You can't get on that!\r\n");
            return;
        }
        if (SITTING(chair)) {
            ch->sendf("Someone is already on that one!\r\n");
            return;
        }
        if (GET_OBJ_TYPE(chair) != ITEM_CHAIR && GET_OBJ_TYPE(chair) != ITEM_BED) {
            ch->sendf("You can't sit on that!\r\n");
            return;
        }
        if (GET_OBJ_SIZE(chair) + 1 < get_size(ch)) {
            ch->sendf("You are too large for it!\r\n");
            return;
        }
        switch (GET_POS(ch)) {
            case POS_STANDING:
                reveal_hiding(ch, 0);
                act("You sit down on $p.", false, ch, chair, nullptr, TO_CHAR);
                act("$n sits down on $p.", false, ch, chair, nullptr, TO_ROOM);
                GET_POS(ch) = POS_SITTING;
                SITS(ch) = chair;
                SITTING(chair) = ch;
                break;
            case POS_SITTING:
                ch->sendf("You should stand up first.\r\n");
                break;
            case POS_RESTING:
                ch->sendf("You should stand up first.\r\n");
                break;
            case POS_SLEEPING:
                ch->sendf("You have to wake up first.\r\n");
                break;
            case POS_FIGHTING:
                ch->sendf("Sit down while fighting? Are you MAD?\r\n");
                break;
            default:
                ch->sendf("You stop floating around, and sit down.\r\n");
                act("$n stops floating around, and sits down.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_SITTING;
                break;
        }
    }
}

ACMD(do_rest) {
    Object *chair = nullptr;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (PLR_FLAGGED(ch, PLR_PILOTING)) {
        ch->sendf("You are busy piloting a ship!\r\n");
        return;
    }
    if (SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) {
        ch->sendf("You can't rest here!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT)) {
        ch->sendf("You are inside a healing tank!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_SANCTUARY)) {
        if (GET_SKILL(ch, SKILL_BARRIER)) {
            ch->sendf("You have a barrier around you and can't rest.\r\n");
            return;
        } else {
            GET_BARRIER(ch) = 0;
            ch->clearFlag(FlagType::Affect,AFF_SANCTUARY);
        }
    }
    if (FIGHTING(ch)) {
        ch->sendf("You are a bit busy at the moment!\r\n");
        return;
    }
    if (GET_KAIOKEN(ch) > 0) {
        ch->sendf("You are utilizing kaioken and can't rest!\r\n");
        return;
    }

    if (DRAGGING(ch)) {
        act("@WYou stop dragging @C$N@W!@n", true, ch, nullptr, DRAGGING(ch), TO_CHAR);
        act("@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, DRAGGING(ch), TO_ROOM);
        DRAGGED(DRAGGING(ch)) = nullptr;
        DRAGGING(ch) = nullptr;
    }

    if (CARRYING(ch)) {
        ch->sendf("You are carrying someone!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_FLYING)) {
        do_fly(ch, nullptr, 0, 0);
    }

    if (!*arg) {
        if (SITS(ch)) {
            chair = SITS(ch);
            if (GET_OBJ_TYPE(chair) != ITEM_BED) {
                ch->sendf("You can't lay on that!\r\n");
                return;
            }
        }
        switch (GET_POS(ch)) {
            case POS_STANDING:
                reveal_hiding(ch, 0);
                ch->sendf("You lay down and rest your tired bones.\r\n");
                act("$n lays down and rests.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_RESTING;
                break;
            case POS_SITTING:
                ch->sendf("You rest your tired bones.\r\n");
                act("$n rests.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_RESTING;
                break;
            case POS_RESTING:
                ch->sendf("You are already resting.\r\n");
                break;
            case POS_SLEEPING:
                ch->sendf("You have to wake up first.\r\n");
                break;
            case POS_FIGHTING:
                ch->sendf("Rest while fighting?  Are you MAD?\r\n");
                break;
            default:
                ch->sendf("You stop floating around, and stop to rest your tired bones.\r\n");
                act("$n stops floating around, and rests.", false, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_RESTING;
                break;
        }
    } else {
        if (SITS(ch)) {
            ch->sendf("You are already on something!\r\n");
            return;
        }
        if (!(chair = get_obj_in_list_vis(ch, arg, nullptr, ch->getRoom()->getInventory()))) {
            ch->sendf("That isn't here.\r\n");
            return;
        }
        if (GET_OBJ_VNUM(chair) == 65) {
            ch->sendf("You can't get on that!\r\n");
            return;
        }
        if (SITTING(chair)) {
            ch->sendf("Someone is already on that one!\r\n");
            return;
        }
        if (GET_OBJ_TYPE(chair) != ITEM_BED) {
            ch->sendf("You can't lay on that!\r\n");
            return;
        }
        if (GET_OBJ_SIZE(chair) + 1 < get_size(ch)) {
            ch->sendf("You are too large for it!\r\n");
            return;
        }
        switch (GET_POS(ch)) {
            case POS_STANDING:
                reveal_hiding(ch, 0);
                act("You lay down and rest on $p.", true, ch, chair, nullptr, TO_CHAR);
                act("$n lays down and rests on $p.", true, ch, chair, nullptr, TO_ROOM);
                SITS(ch) = chair;
                SITTING(chair) = ch;
                GET_POS(ch) = POS_RESTING;
                break;
            case POS_SITTING:
                ch->sendf("You should get up first.\r\n");
                break;
            case POS_RESTING:
                ch->sendf("You are already resting.\r\n");
                break;
            case POS_SLEEPING:
                ch->sendf("You have to wake up first.\r\n");
                break;
            case POS_FIGHTING:
                ch->sendf("Rest while fighting?  Are you MAD?\r\n");
                break;
            default:
                ch->sendf("You stop floating around, and stop to rest your tired bones.\r\n");
                act("$n stops floating around, and rests.", false, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_RESTING;
                break;
        }
    }
}

ACMD(do_sleep) {
    Object *chair = nullptr;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (!IS_NPC(ch)) {
        if (PRF_FLAGGED(ch, PRF_ARENAWATCH)) {
            ch->clearFlag(FlagType::Pref, PRF_ARENAWATCH);
            ARENA_IDNUM(ch) = -1;
            ch->sendf("You stop watching the arena action.\r\n");
        }
    }

    if (GET_BONUS(ch, BONUS_INSOMNIAC)) {
        ch->sendf("You don't feel the least bit tired.\r\n");
        return;
    }

    if (SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) {
        ch->sendf("You can't rest here!\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_PILOTING)) {
        ch->sendf("You are busy piloting a ship!\r\n");
        return;
    }
    if (FIGHTING(ch)) {
        ch->sendf("You are a bit busy at the moment!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT)) {
        ch->sendf("You are inside a healing tank!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_POWERUP)) {
        ch->sendf("You are busy powering up!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_SANCTUARY)) {
        if (GET_SKILL(ch, SKILL_BARRIER) > 0) {
            ch->sendf("You have a barrier around you and can't sleep.\r\n");
            return;
        } else {
            GET_BARRIER(ch) = 0;
            ch->clearFlag(FlagType::Affect,AFF_SANCTUARY);
        }
    }
    if (GET_KAIOKEN(ch) > 0) {
        ch->sendf("You are utilizing kaioken and can't sleep!\r\n");
        return;
    }
    if (GET_SLEEPT(ch) > 0) {
        ch->sendf("You aren't sleepy enough.\r\n");
        return;
    }
    if (DRAGGING(ch)) {
        act("@WYou stop dragging @C$N@W!@n", true, ch, nullptr, DRAGGING(ch), TO_CHAR);
        act("@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, DRAGGING(ch), TO_ROOM);
        DRAGGED(DRAGGING(ch)) = nullptr;
        DRAGGING(ch) = nullptr;
    }
    if (CARRYING(ch)) {
        ch->sendf("You are carrying someone!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_FLYING)) {
        do_fly(ch, nullptr, 0, 0);
    }

    if (!*arg) {
        if (SITS(ch)) {
            chair = SITS(ch);
            if (GET_OBJ_TYPE(chair) != ITEM_BED) {
                ch->sendf("You can't sleep on %s.\r\n", chair->getShortDesc());
                return;
            }
        }
        switch (GET_POS(ch)) {
            case POS_STANDING:
            case POS_SITTING:
            case POS_RESTING:
                reveal_hiding(ch, 0);
                ch->sendf("You go to sleep.\r\n");
                act("$n lies down and falls asleep.", true, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_SLEEPING;
                /* Fury Mode Loss for halfbreeds */

                if (PLR_FLAGGED(ch, PLR_FURY)) {
                    ch->sendf(
                                 "Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
                    ch->clearFlag(FlagType::PC, PLR_FURY);
                }

                /* Fury Mode Loss for halfbreeds */

                if (GET_STUPIDKISS(ch) > 0) {
                    GET_STUPIDKISS(ch) = 0;
                    ch->sendf("You forget about that stupid kiss.\r\n");
                }
                break;
            case POS_SLEEPING:
                ch->sendf("You are already sound asleep.\r\n");
                break;
            case POS_FIGHTING:
                ch->sendf("Sleep while fighting?  Are you MAD?\r\n");
                break;
            default:
                ch->sendf("You stop floating around, and lie down to sleep.\r\n");
                act("$n stops floating around, and lie down to sleep.",
                    true, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_SLEEPING;
                break;
        }
    } else {
        if (SITS(ch)) {
            ch->sendf("You are already on something!\r\n");
            return;
        }
        if (!(chair = get_obj_in_list_vis(ch, arg, nullptr, ch->getRoom()->getInventory()))) {
            ch->sendf("That isn't here.\r\n");
            return;
        }
        if (GET_OBJ_VNUM(chair) == 65) {
            ch->sendf("You can't get on that!\r\n");
            return;
        }
        if (SITTING(chair)) {
            ch->sendf("Someone is already on that one!\r\n");
            return;
        }
        if (GET_OBJ_TYPE(chair) != ITEM_BED) {
            ch->sendf("You can't sleep on that!\r\n");
            return;
        }
        if (GET_OBJ_SIZE(chair) + 1 < get_size(ch)) {
            ch->sendf("You are too large for it!\r\n");
            return;
        }
        switch (GET_POS(ch)) {
            case POS_RESTING:
            case POS_SITTING:
                ch->sendf("You need to get up first!\r\n");
                break;
            case POS_STANDING:
                reveal_hiding(ch, 0);
                act("You lay down on $p and sleep.", false, ch, chair, nullptr, TO_CHAR);
                act("$n lays down on $p and sleeps.", false, ch, chair, nullptr, TO_ROOM);
                /* Fury Mode Loss for halfbreeds */

                if (PLR_FLAGGED(ch, PLR_FURY)) {
                    ch->sendf(
                                 "Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
                    ch->clearFlag(FlagType::PC, PLR_FURY);
                }

                /* Fury Mode Loss for halfbreeds */

                if (GET_STUPIDKISS(ch) > 0) {
                    GET_STUPIDKISS(ch) = 0;
                    ch->sendf("You forget about that stupid kiss.\r\n");
                }
                SITS(ch) = chair;
                SITTING(chair) = ch;
                GET_POS(ch) = POS_SLEEPING;
                break;
            case POS_SLEEPING:
                ch->sendf("You are already sound asleep.\r\n");
                break;
            case POS_FIGHTING:
                ch->sendf("Sleep while fighting?  Are you MAD?\r\n");
                break;
            default:
                ch->sendf("You stop floating around, and lie down to sleep.\r\n");
                act("$n stops floating around, and lie down to sleep.",
                    true, ch, nullptr, nullptr, TO_ROOM);
                GET_POS(ch) = POS_SLEEPING;
                break;
        }
    }
}

ACMD(do_wake) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *vict;
    int self = 0;

    one_argument(argument, arg);

    if (AFF_FLAGGED(ch, AFF_KNOCKED)) {
        ch->sendf("You are knocked out cold for right now!\r\n");
        return;
    }

    if (GET_BONUS(ch, BONUS_LATE) && GET_POS(ch) == POS_SLEEPING) {
        ch->sendf("Nah you're enjoying sleeping too much.\r\n");
        return;
    }

    if (*arg) {
        if (GET_POS(ch) == POS_SLEEPING)
            ch->sendf("Maybe you should wake yourself up first.\r\n");
        else if ((vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)) == nullptr)
            ch->sendf("%s", CONFIG_NOPERSON);
        else if (vict == ch)
            self = 1;
        else if (AWAKE(vict))
            act("$E is already awake.", false, ch, nullptr, vict, TO_CHAR);
        else if (AFF_FLAGGED(vict, AFF_SLEEP))
            act("You can't wake $M up!", false, ch, nullptr, vict, TO_CHAR);
        else if (GET_POS(vict) < POS_SLEEPING)
            act("$E's in pretty bad shape!", false, ch, nullptr, vict, TO_CHAR);
        else if (AFF_FLAGGED(vict, AFF_KNOCKED))
            ch->sendf("They are knocked out cold for right now!\r\n");
        else if (GET_BONUS(ch, BONUS_LATE))
            ch->sendf("They say 'Yeah yeah...' and then roll back over.\r\n");
        else {
            act("You wake $M up.", false, ch, nullptr, vict, TO_CHAR);
            act("You are awakened by $n.", false, ch, nullptr, vict, TO_VICT | TO_SLEEP);
            GET_POS(vict) = POS_SITTING;
            if (DRAGGED(vict)) {
                act("@WYou stop dragging @C$N@W!@n", true, DRAGGED(vict), nullptr, vict, TO_CHAR);
                act("@C$n@W stops dragging @c$N@W!@n", true, DRAGGED(vict), nullptr, vict, TO_ROOM);
                DRAGGING(DRAGGED(vict)) = nullptr;
                DRAGGED(vict) = nullptr;
            }
            if (CARRIED_BY(vict)) {
                if (GET_ALIGNMENT(CARRIED_BY(vict)) > 50) {
                    carry_drop(CARRIED_BY(vict), 0);
                } else {
                    carry_drop(CARRIED_BY(vict), 1);
                }
            }
        }
        if (!self)
            return;
    }
    if (AFF_FLAGGED(ch, AFF_SLEEP))
        ch->sendf("You can't wake up!\r\n");
    else if (GET_POS(ch) > POS_SLEEPING)
        ch->sendf("You are already awake...\r\n");
    else {
        ch->sendf("You awaken, and sit up.\r\n");
        act("$n awakens.", true, ch, nullptr, nullptr, TO_ROOM);
        if (DRAGGED(ch)) {
            act("@WYou stop dragging @C$N@W!@n", true, DRAGGED(ch), nullptr, ch, TO_CHAR);
            act("@C$n@W stops dragging you!@n", true, DRAGGED(ch), nullptr, ch, TO_VICT);
            act("@C$n@W stops dragging @c$N@W!@n", true, DRAGGED(ch), nullptr, ch, TO_NOTVICT);
            DRAGGING(DRAGGED(ch)) = nullptr;
            DRAGGED(ch) = nullptr;
        }
        if (CARRIED_BY(ch)) {
            if (GET_ALIGNMENT(CARRIED_BY(ch)) > 50) {
                carry_drop(CARRIED_BY(ch), 0);
            } else {
                carry_drop(CARRIED_BY(ch), 1);
            }
        }
        GET_POS(ch) = POS_SITTING;
    }
}

ACMD(do_follow) {
    char buf[MAX_INPUT_LENGTH];
    BaseCharacter *leader;

    one_argument(argument, buf);

    if (PLR_FLAGGED(ch, PLR_HEALT)) {
        ch->sendf("You are inside a healing tank!\r\n");
        return;
    }

    if (*buf) {
        if (!(leader = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM))) {
            ch->sendf("%s", CONFIG_NOPERSON);
            return;
        }
    } else {
        ch->sendf("Whom do you wish to follow?\r\n");
        return;
    }

    if (ch->master == leader) {
        act("You are already following $M.", false, ch, nullptr, leader, TO_CHAR);
        return;
    }
    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
        act("But you only feel like following $N!", false, ch, nullptr, ch->master, TO_CHAR);
    } else {            /* Not Charmed follow person */
        if (leader == ch) {
            if (!ch->master) {
                ch->sendf("You are already following yourself.\r\n");
                return;
            }
            stop_follower(ch);
        } else {
            if (circle_follow(ch, leader)) {
                ch->sendf("Sorry, but following in loops is not allowed.\r\n");
                return;
            }
            if (ch->master)
                stop_follower(ch);
            ch->clearFlag(FlagType::Affect,AFF_GROUP);
            reveal_hiding(ch, 0);
            add_follower(ch, leader);
        }
    }
}
