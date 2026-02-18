/*************************************************************************
 *   File: act.movement.c                                Part of CircleMUD *
 *  Usage: movement commands, door handling, & sleep/rest/etc state        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include <bsd/string.h>
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/Destination.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/Structure.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/act.movement.hpp"
#include "dbat/game/dg_comm.hpp"
#include "dbat/game/vehicles.hpp"
#include "dbat/game/handler.hpp"
//#include "dbat/game/send.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/game/interpreter.hpp"
#include "dbat/game/fight.hpp"
#include "dbat/game/spells.hpp"
#include "dbat/game/Guild.hpp"
#include "dbat/game/dg_scripts.hpp"
//#include "dbat/game/local_limits.hpp"
#include "dbat/game/constants.hpp"
//#include "dbat/game/act.informative.hpp"
#include "dbat/game/planet.hpp"
#include "dbat/game/config.hpp"
#include "dbat/game/utils.hpp"
#include "volcano/util/FilterWeak.hpp"

#include "dbat/game/const/Pulse.hpp"
#include "dbat/game/const/Environment.hpp"
#include "dbat/game/const/ContainerFlag.hpp"
#include "dbat/game/const/Condition.hpp"

#include "dbat/game/Random.hpp"

/* local functions */
static void handle_fall(Character *ch);

static int check_swim(Character *ch);

static void disp_locations(Character *ch, vnum areaVnum, std::unordered_set<room_vnum> &rooms);

static int has_boat(Character *ch);

static int find_door(Character *ch, const char *type, char *dir, const char *cmdname);

static int has_key(Character *ch, obj_vnum key);

static void do_doorcmd(Character *ch, Object *obj, int door, int scmd);

static int ok_pick(Character *ch, obj_vnum keynum, int pickproof, int dclock, int scmd, Object *obj);

static int has_flight(Character *ch);

static int do_simple_enter(Character *ch, Object *obj, int need_specials_check);

static int perform_enter_obj(Character *ch, Object *obj, int need_specials_check);

static int do_simple_leave(Character *ch, Object *obj, int need_specials_check);

static int perform_leave_obj(Character *ch, Object *obj, int need_specials_check);

static int64_t calcNeedMovementGravity(Character *ch)
{
    if (IS_NPC(ch))
        return 0.0;
    auto gravity = ch->currentGravity();
    return (gravity * gravity) * ch->getBaseStat("burden_ratio");
}

/* This handles teleporting players with instant transmission or skills like it. */
void handle_teleport(Character *ch, Character *tar, int location)
{
    int success = false;

    if (location != 0)
    { /* Teleport to a particular room */
        ch->leaveLocation();
        ch->moveToLocation(location);
        success = true;
    }
    else if (tar)
    { /* Teleport to a particular character */
        ch->leaveLocation();
        ch->moveToLocation(tar);
        success = true;
    }

    if (success == true)
    { /* We have made it. */
        act("@w$n@w appears in an instant out of nowhere!@n", true, ch, nullptr, nullptr, TO_ROOM);
        if (auto drg = DRAGGING(ch); drg && !IS_NPC(drg))
        {
            drg->leaveLocation();
            drg->moveToLocation(ch);
            act("@w$n@w appears in an instant out of nowhere being dragged by $N!@n", true, drg, nullptr, ch,
                TO_NOTVICT);
        }
        if (auto grap = GRAPPLING(ch); grap && !IS_NPC(grap))
        {
            grap->leaveLocation();
            grap->moveToLocation(ch);
            act("@w$n@w appears in an instant out of nowhere being grappled by $N!@n", true, grap, nullptr, ch,
                TO_NOTVICT);
        }
        if (auto carry = CARRYING(ch))
        {
            carry->leaveLocation();
            carry->moveToLocation(ch);
            act("@w$n@w appears in an instant out of nowhere being carried by $N!@n", true, carry, nullptr, ch,
                TO_NOTVICT);
        }
        if (auto grap = GRAPPLED(ch); grap && !IS_NPC(grap))
        {
            grap->leaveLocation();
            grap->moveToLocation(ch);
            act("@w$n@w appears in an instant out of nowhere being grappled by $N!@n", true, grap, nullptr, ch,
                TO_NOTVICT);
        }
        if (auto drg = DRAGGING(ch); drg && IS_NPC(drg))
        {
            act("@WYou stop dragging @C$N@W!@n", true, ch, nullptr, drg, TO_CHAR);
            act("@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, drg, TO_ROOM);
            DRAGGED(drg) = nullptr;
            DRAGGING(ch) = nullptr;
        }
        if (auto grap = GRAPPLING(ch); grap && IS_NPC(grap))
        {
            grap->setBaseStat<int>("grapple_type", -1);
            GRAPPLED(grap) = nullptr;
            GRAPPLING(ch) = nullptr;
            ch->setBaseStat<int>("grapple_type", -1);
        }
        if (auto grap = GRAPPLED(ch); grap && IS_NPC(grap))
        {
            grap->setBaseStat<int>("grapple_type", -1);
            GRAPPLING(grap) = nullptr;
            GRAPPLED(ch) = nullptr;
            ch->setBaseStat<int>("grapple_type", -1);
        }
    }
    else
    { /* Wut... */
        basic_mud_log("ERROR: handle_teleport called without a destination.");
        return;
    }
}

/* Let's carry someone! Why not? - Iovan */
ACMD(do_carry)
{

    if (IS_NPC(ch))
        return;

    Character *vict = nullptr;
    char arg[MAX_INPUT_LENGTH];

    if (DRAGGING(ch))
    {
        ch->sendText("You are busy dragging someone at the moment.\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_PILOTING))
    {
        ch->sendText("You are busy piloting a ship!\r\n");
        return;
    }

    if (CARRYING(ch))
    { /* Already carrying someone. Put them down. Simple and clean. */
        if (GET_ALIGNMENT(ch) > 50)
        {
            carry_drop(ch, 0);
        }
        else
        {
            carry_drop(ch, 1);
        }
        return;
    }

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("You want to carry who?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("That person isn't here.\r\n");
        return;
    }

    if (IS_NPC(vict))
    {
        ch->sendText("There's no point in carrying them.\r\n");
        return;
    }

    if (CARRIED_BY(vict))
    {
        ch->sendText("Someone is already carrying them!\r\n");
        return;
    }

    if (GET_POS(vict) > POS_SLEEPING)
    {
        ch->sendText("They are not unconcious.\r\n");
        return;
    }

    if (vict->getBaseStat("weight_total") > CAN_CARRY_W(ch))
    {
        act("@WYou try to pick up @C$N@W but have to put them down. They are too heavy for you at the moment.@n",
            true, ch, nullptr, vict, TO_CHAR);
        act("@C$n@W tries to pick up @c$N@W. After struggling for a moment $e has to put $M down.@n", true, ch,
            nullptr, vict, TO_NOTVICT);
        WAIT_STATE(ch, PULSE_1SEC);
        return;
    }

    /* Let's carry that mofo! */
    act("@WYou pick up @C$N@W and put $M over your shoulder.@n", true, ch, nullptr, vict, TO_CHAR);
    act("@C$n@W picks up $c$N@W and puts $M over $s shoulder.@n", true, ch, nullptr, vict, TO_NOTVICT);
    if (SITS(vict))
    {
        Object *chair = SITS(vict);
        chair->sitting.reset();
        vict->sits.reset();
    }
    CARRYING(ch) = vict;
    CARRIED_BY(vict) = ch;
    WAIT_STATE(ch, PULSE_1SEC);
}

/* Handles dropping someone you are carrying. */
void carry_drop(Character *ch, int type)
{

    Character *vict = nullptr;

    vict = CARRYING(ch);

    switch (type)
    {
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

ACMD(do_land)
{

    auto lz = ch->location.getLandZone();

    skip_spaces(&argument);

    if (!lz && !*argument)
    {
        if (ch->affect_flags.get(AFF_FLYING))
        {
            act("@WYou land.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n@W lands nearby.@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->affect_flags.set(AFF_FLYING, false);
            return;
        }
        ch->sendText("You are not even in the lower atmosphere of a planet!\r\n");
        return;
    }

    if(!lz) {
        ch->sendText("You are not in the lower atmosphere of a planet!\r\n");
        return;
    }

    auto landLocations = lz->getLandingSpots();

    if (!*argument)
    {
        ch->sendText("Land where?\n");
        displayLandSpots(ch, lz->name, landLocations);
        return;
    }

    Location landing;
    std::string landName = "UNKNOWN";

    if (auto matched = volcano::util::partialMatch(argument, landLocations, false))
    {
        landing = matched.value()->second;
        landName = matched.value()->first;
    }

    if (!landing)
    {
        ch->sendText("You can't land there.\r\n");
        return;
    }

    ch->sendText("You descend through the upper atmosphere, and coming down through the clouds you land quickly on the ground below.\r\n");

    char sendback[MAX_INPUT_LENGTH];
    sprintf(sendback, "@C$n@Y flies down through the atmosphere toward @G%s@Y!@n", landName.c_str());
    act(sendback, true, ch, nullptr, nullptr, TO_ROOM);
    ch->leaveLocation();
    ch->moveToLocation(landing);

    lz->for_each_listening([&](Character *c) {
        if(c->location == landing) return; // Don't message people who are at the landing place.
        if(!OUTSIDE(c)) return; // can't see it.
        if(!AWAKE(c)) return; // not conscious
        act("$n can be seen landing from space nearby!", true, ch, nullptr, c, TO_VICT);
    });

    lz->sendToSense(ch, "landing on the planet");
    send_to_scouter("A powerlevel signal has been detected landing on the planet", ch, 0, 1);
    act("$n comes down from high above in the sky and quickly lands on the ground.", true, ch, nullptr, nullptr,
        TO_ROOM);
    
    ch->lookAtLocation();
}

/* simple function to determine if char can walk on water */
static int has_boat(Character *ch)
{
    Object *obj;
    int i;

    if (ADM_FLAGGED(ch, ADM_WALKANYWHERE) || GET_ADMLEVEL(ch) > 4)
        return (1);

    if (AFF_FLAGGED(ch, AFF_WATERWALK))
        return (1);

    /* non-wearable boats in inventory will do it */
    auto isBoat = [](const auto &o)
    { return GET_OBJ_TYPE(o) == ITEM_BOAT; };
    if (ch->searchInventory(isBoat))
        return true;

    return false;
}

/* simple function to determine if char can fly */
static int has_flight(Character *ch)
{
    if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
        return 1;

    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        // Separate check for NPCs and Androids
        if (IS_NPC(ch) || IS_ANDROID(ch))
            return 1;

        // Check if the character has enough KI to keep flying
        if (ch->getCurVital(CharVital::ki) >= (GET_MAX_MANA(ch) / (GET_WIS(ch) * 30)))
            return 1;

        // If out of KI, crash to the ground
        act("@WYou crash to the ground, too tired to fly anymore!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n@W crashes to the ground!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->affect_flags.set(AFF_FLYING, false);
        handle_fall(ch);
        return 0;
    }

    // Check for flying items in inventory
    auto givesFlight = [](const auto &o)
    { return OBJAFF_FLAGGED(o, AFF_FLYING); };
    return ch->searchInventory(givesFlight) ? true : false;
}

/* simple function to determine if char can breathe non-o2 */
int has_o2(Character *ch)
{
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

int do_simple_move(Character *ch, int dir, int need_specials_check)
{
    char throwaway[MAX_INPUT_LENGTH] = ""; /* Functions assume writable. */
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];
    int need_movement;
    Room *rm;
    auto cdir = static_cast<Direction>(dir);

    /*
     * Check for special routines (North is 1 in command list, but 0 here) Note
     * -- only check if following; this avoids 'double spec-proc' bug
     */
    if (need_specials_check && special(ch, dir + 1, throwaway))
        return (0);

    auto was_in = ch->location;

    /* blocked by a leave trigger ? */
    if (!leave_mtrigger(ch, dir) || ch->location != was_in) /* prevent teleport crashes */
        return 0;
    if (!leave_wtrigger(ch->getRoom(), ch, dir) || ch->location != was_in) /* prevent teleport crashes */
        return 0;
    if (!leave_otrigger(ch->getRoom(), ch, dir) || ch->location != was_in) /* prevent teleport crashes */
        return 0;
    /* charmed? */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && ch->location == ch->master->location)
    {
        ch->sendText("The thought of leaving your master makes you weep.\r\n");
        act("$n bursts into tears.", false, ch, nullptr, nullptr, TO_ROOM);
        return (0);
    }

    auto ex = ch->location.getExit(static_cast<Direction>(dir));
    if (!ex)
    {
        basic_mud_log("No exit found");
        return 0;
    }

    auto &e = ex.value();

    int willfall = false;
    /* if this room or the one we're going to needs flight, check for it */
    if ((ch->location.getSectorType() == SectorType::flying) || (e.getSectorType() == SectorType::flying))
    {
        if (!has_flight(ch))
        {
            if (dir != 4)
            {
                willfall = true;
            }
            else
            {
                ch->sendText("You need to fly to go there!\r\n");
                return (0);
            }
        }
    }

    if (((ch->location.getSectorType() == SectorType::water_noswim) || (e.getSectorType() == SectorType::water_noswim)) &&
        IS_HUMANOID(ch))
    {
        if (IS_KANASSAN(ch) && !has_flight(ch))
        {
            act("@CYou swim swiftly.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@c$n@C swims swiftly.@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
        else if (IS_ICER(ch) && !has_flight(ch))
        {
            act("@CYou swim swiftly.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@c$n@C swims swiftly.@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
        else if (!IS_KANASSAN(ch) && !IS_ICER(ch) && !has_flight(ch))
        {
            if (!check_swim(ch))
            {
                return (0);
            }
            else
            {
                act("@CYou swim through the cold water.@n", true, ch, nullptr, nullptr, TO_CHAR);
                act("@c$n@C swim through the cold water.@n", true, ch, nullptr, nullptr, TO_ROOM);
                WAIT_STATE(ch, PULSE_1SEC);
            }
        }
    }

    if (e.getWhereFlag(WhereFlag::space))
    {
        if (!IS_ANDROID(ch))
        {
            if (!check_swim(ch))
            {
                return (0);
            }
        }
    }

    if (e.getGroundEffect() == 6 && !IS_HUMANOID(ch) && IS_NPC(ch))
    {
        return (0);
    }

    if (IS_NPC(ch) && e.getRoomFlag(ROOM_NOMOB) && !ch->master)
    {
        return (0);
    }

    if (ch->location.getEnvironment(ENV_WATER) >= 100.0 || e.getEnvironment(ENV_WATER) >= 100.0)
    {
        if (!has_o2(ch) &&
            ((group_bonus(ch, 2) != 10 && (ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 200) || (group_bonus(ch, 2) == 10 &&
                                                                                                         (ch->getCurVital(CharVital::ki)) <
                                                                                                             GET_MAX_MANA(ch) / 800)))
        {
            if (ch->modCurVitalDam(CharVital::health, 0.05) > 0)
            {
                ch->sendText("@RYou struggle to breath!@n\r\n");
            }
            else
            {
                ch->sendText("@rYou drown!@n\r\n");
                die(ch, nullptr);
                return (0);
            }
        }
        if (!has_o2(ch) &&
            ((group_bonus(ch, 2) != 10 && (ch->getCurVital(CharVital::ki)) >= GET_MAX_MANA(ch) / 200) || (group_bonus(ch, 2) == 10 &&
                                                                                                          (ch->getCurVital(CharVital::ki)) >=
                                                                                                              GET_MAX_MANA(ch) / 800)))
        {
            ch->sendText("@CYou hold your breath!@n\r\n");
            if (group_bonus(ch, 2) == 10)
            {
                ch->modCurVital(CharVital::ki, -(ch->getEffectiveStat<int64_t>("ki") / 800));
            }
            else
            {
                ch->modCurVital(CharVital::ki, -(ch->getEffectiveStat<int64_t>("ki") / 200));
            }
        }
    }

    /* move points needed is avg. move loss for src and destination sect type */
    if (!IS_NPC(ch))
    {
        auto gravity = ch->location.getEnvironment(ENV_GRAVITY);
        need_movement = (gravity * gravity) * ch->getBaseStat("burden_ratio");
    }

    if (GET_LEVEL(ch) <= 1)
    {
        need_movement = 0;
    }
    /* Stealth increases your move cost, less if you are good at it */
    if (AFF_FLAGGED(ch, AFF_HIDE))
        need_movement *= ((roll_skill(ch, SKILL_HIDE) > 15) ? 2 : 4);

    if (AFF_FLAGGED(ch, AFF_SNEAK))
        need_movement *= ((roll_skill(ch, SKILL_MOVE_SILENTLY) > 15) ? 1.2 : 2);

    int flight_cost = 0;

    if (AFF_FLAGGED(ch, AFF_FLYING) && !IS_ANDROID(ch))
    {
        if (!GET_SKILL(ch, SKILL_CONCENTRATION) && !GET_SKILL(ch, SKILL_FOCUS))
        {
            flight_cost = GET_MAX_MANA(ch) / 100;
        }
        else if (GET_SKILL(ch, SKILL_CONCENTRATION) && !GET_SKILL(ch, SKILL_FOCUS))
        {
            flight_cost = GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_CONCENTRATION) * 2);
        }
        else if (!GET_SKILL(ch, SKILL_CONCENTRATION) && GET_SKILL(ch, SKILL_FOCUS))
        {
            flight_cost = GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_FOCUS) * 3);
        }
        else
        {
            flight_cost =
                GET_MAX_MANA(ch) / ((GET_SKILL(ch, SKILL_CONCENTRATION) * 2) + (GET_SKILL(ch, SKILL_FOCUS) * 3));
        }
    }

    if (AFF_FLAGGED(ch, AFF_FLYING) && ((ch->getCurVital(CharVital::ki)) < flight_cost) && !IS_ANDROID(ch))
    {
        ch->modCurVital(CharVital::ki, -flight_cost);
        act("@WYou crash to the ground, too tired to fly anymore!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n@W crashes to the ground!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->affect_flags.set(AFF_FLYING, false);
    }
    else if (AFF_FLAGGED(ch, AFF_FLYING) && !IS_ANDROID(ch))
    {
        ch->modCurVital(CharVital::ki, -flight_cost);
    }

    if ((ch->getCurVital(CharVital::stamina)) < need_movement && !AFF_FLAGGED(ch, AFF_FLYING) && !IS_NPC(ch))
    {
        if (need_specials_check && ch->master)
        {
            ch->sendText("You are too exhausted to follow.\r\n");
        }
        else
        {
            ch->sendText("You are too exhausted.\r\n");
        }

        return (0);
    }

    if (ch->location.getRoomFlag(ROOM_TUNNEL) && (e.countPlayers() >= CONFIG_TUNNEL_SIZE))
    {
        if (CONFIG_TUNNEL_SIZE > 1)
            ch->sendText("There isn't enough room for you to go there!\r\n");
        else
            ch->sendText("There isn't enough room there for more than one person!\r\n");
        return (0);
    }
    /* Mortals and low level gods cannot enter greater god rooms. */
    if (e.getRoomFlag(ROOM_GODROOM) && GET_ADMLEVEL(ch) < ADMLVL_GRGOD)
    {
        ch->sendText("You aren't godly enough to use that room!\r\n");
        return (0);
    }

    /******* Zone flag checks *******/

    auto z = e.getZone();

    if ((GET_ADMLEVEL(ch) < ADMLVL_IMMORT) && z->zone_flags.get(ZONE_CLOSED))
    {
        ch->sendText("This zone is currently closed to mortals.\r\n");
        return (0);
    }

    if ((GET_ADMLEVEL(ch) >= ADMLVL_IMMORT && GET_ADMLEVEL(ch) < ADMLVL_GRGOD) && z->zone_flags.get(ZONE_NOIMMORT))
    {
        ch->sendText("This zone is closed to all.\r\n");
        return (0);
    }

    /* Now we know we're allowed to go into the room. */
    if (!ADM_FLAGGED(ch, ADM_WALKANYWHERE) && !IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_FLYING))
    {
        ch->modCurVital(CharVital::stamina, -need_movement);
    }

    if (AFF_FLAGGED(ch, AFF_SNEAK) && !IS_NPC(ch))
    {
        sprintf(buf2, "$n sneaks %s.", dirs[dir]);
        if (GET_SKILL(ch, SKILL_MOVE_SILENTLY))
        {
            improve_skill(ch, SKILL_MOVE_SILENTLY, 0);
        }
        else if (slot_count(ch) + 1 > GET_SLOTS(ch))
        {
            ch->sendText("@RYour skill slots are full. You can not learn Move Silently.\r\n");
            ch->affect_flags.set(AFF_SNEAK, false);
        }
        else
        {
            ch->sendText("@GYou learn the very basics of moving silently.@n\r\n");
            SET_SKILL(ch, SKILL_MOVE_SILENTLY, Random::get<int>(5, 10));
            act(buf2, true, ch, nullptr, nullptr, TO_ROOM | TO_SNEAKRESIST);
            if (GET_DEX(ch) < Random::get<int>(1, 30))
            {
                WAIT_STATE(ch, PULSE_1SEC);
            }
        }
    }

    if (!AFF_FLAGGED(ch, AFF_SNEAK) && !AFF_FLAGGED(ch, AFF_FLYING))
    {
        sprintf(buf2, "$n leaves %s.", dirs[dir]);
        act(buf2, true, ch, nullptr, nullptr, TO_ROOM);
    }
    if (!AFF_FLAGGED(ch, AFF_SNEAK) && AFF_FLAGGED(ch, AFF_FLYING))
    {
        sprintf(buf2, "$n flies %s.", dirs[dir]);
        act(buf2, true, ch, nullptr, nullptr, TO_ROOM);
    }

    was_in = ch->location;
    if (DRAGGING(ch))
    {
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, DRAGGING(ch), TO_ROOM);
    }
    if (CARRYING(ch))
    {
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, CARRYING(ch), TO_ROOM);
    }
    ch->affect_flags.set(AFF_PURSUIT, true);
    ch->leaveLocation();
    ch->moveToLocation(e);
    auto z2 = ch->location.getZone();
    if ((z2 != was_in.getZone()) && !IS_NPC(ch) && !IS_ANDROID(ch))
    {
        z2->sendToSense(ch, "nearby");
        sprintf(buf3, "@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection range@n.",
                add_commas(ch->getPL()).c_str());
        send_to_scouter(buf3, ch, 0, 0);
    }
    /* move them first, then move them back if they aren't allowed to go. */
    /* see if an entry trigger disallows the move */
    if (!entry_mtrigger(ch) || !enter_wtrigger(ch->getRoom(), ch, dir))
    {
        ch->leaveLocation();
        ch->moveToLocation(was_in);
        ch->affect_flags.set(AFF_PURSUIT, false);
        return 0;
    }

    snprintf(buf2, sizeof(buf2), "%s%s",
             ((cdir == UP) || (cdir == DOWN) ? "" : "the "),
             (cdir == UP ? "below" : (cdir == DOWN) ? "above"
                                                  : dirs[static_cast<int>(rev_dir.at(cdir))]));
    act("$n arrives from $T.", true, ch, nullptr, buf2, TO_ROOM | TO_SNEAKRESIST);

    if (FIGHTING(ch))
    {
        if (was_in.getSectorType() != SectorType::flying && was_in.getSectorType() != SectorType::water_noswim && was_in.getGroundEffect() == 0)
        {
            roll_pursue(FIGHTING(ch), ch);
        }
        ch->affect_flags.set(AFF_PURSUIT, false);
    }

    if (auto dragging = DRAGGING(ch); dragging)
    {
        act("@wYou drag @C$N@w with you.@n", true, ch, nullptr, dragging, TO_CHAR);
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, dragging, TO_ROOM);
        dragging->leaveLocation();
        dragging->moveToLocation(ch);
        if (auto sits = SITS(dragging); sits)
        {
            sits->clearLocation();
            sits->moveToLocation(ch);
        }
        if (!AFF_FLAGGED(dragging, AFF_KNOCKED) && !AFF_FLAGGED(dragging, AFF_SLEEP) && Random::get<int>(1, 3))
        {
            dragging->sendText("You feel your sleeping body being moved.\r\n");
            if (IS_NPC(dragging) && !FIGHTING(dragging))
            {
                set_fighting(dragging, ch);
            }
        }
    }
    if (auto carrying = CARRYING(ch); carrying)
    {
        act("@wYou carry @C$N@w with you.@n", true, ch, nullptr, carrying, TO_CHAR);
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, carrying, TO_ROOM);
        carrying->leaveLocation();
        carrying->moveToLocation(ch);
        if (!AFF_FLAGGED(carrying, AFF_KNOCKED) && !AFF_FLAGGED(carrying, AFF_SLEEP) && Random::get<int>(1, 3))
        {
            carrying->sendText("You feel your sleeping body being moved.\r\n");
        }
    }

    if (ch->desc)
    {
        ch->lookAtLocation();
        if (AFF_FLAGGED(ch, AFF_SNEAK) && !IS_NPC(ch) && GET_SKILL(ch, SKILL_MOVE_SILENTLY) &&
            GET_SKILL(ch, SKILL_MOVE_SILENTLY) < Random::get<int>(1, 101))
        {
            ch->sendText("@wYou make a noise as you arrive and are no longer sneaking!@n\r\n");
            act("@c$n@w makes a noise revealing $s sneaking!@n", true, ch, nullptr, nullptr, TO_ROOM | TO_SNEAKRESIST);
            reveal_hiding(ch, 0);
            ch->affect_flags.set(AFF_SNEAK, false);
        }
    }

    if (ch->location.getGroundEffect() == 6 || was_in.getGroundEffect() == 6)
    {
        if (!IS_DEMON(ch) && !AFF_FLAGGED(ch, AFF_FLYING) && group_bonus(ch, 2) != 14)
        {
            act("@rYour legs are burned by the lava!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@R$n@r's legs are burned by the lava!@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (IS_NPC(ch) && IS_HUMANOID(ch) && Random::get<int>(1, 2) == 2)
            {
                do_fly(ch, nullptr, 0, 0);
            }
            ch->modCurVital(CharVital::health, -(ch->getEffectiveStat<int64_t>("health") / 20));
            if (GET_HIT(ch) <= 0)
            {
                act("@rYou have burned to death!@n", true, ch, nullptr, nullptr, TO_CHAR);
                act("@R$n@r has burned to death!@n", true, ch, nullptr, nullptr, TO_ROOM);
                die(ch, nullptr);
            }
        }
        if (auto dragging = DRAGGING(ch); dragging && !IS_DEMON(dragging))
        {
            act("@R$N@r gets burned!@n", true, ch, nullptr, dragging, TO_CHAR);
            act("@R$N@r gets burned!@n", true, ch, nullptr, dragging, TO_ROOM);
            dragging->modCurVital(CharVital::health, -(dragging->getEffectiveStat<int64_t>("health") / 20));
            if (GET_HIT(dragging) < 0)
            {
                act("@rYou have burned to death!@n", true, dragging, nullptr, nullptr, TO_CHAR);
                act("@R$n@r has burned to death!@n", true, dragging, nullptr, nullptr, TO_ROOM);
                die(dragging, nullptr);
            }
        }
    }

    entry_memory_mtrigger(ch);
    if (!greet_mtrigger(ch, dir))
    {
        ch->leaveLocation();
        ch->moveToLocation(was_in);
        ch->lookAtLocation();
    }
    else
        greet_memory_mtrigger(ch);

    if (willfall == true)
    {
        handle_fall(ch);
        if (auto dragging = DRAGGING(ch); dragging)
        {
            handle_fall(dragging);
        }
    }
    return (1);
}

int perform_move(Character *ch, int dir, int need_specials_check)
{
    room_rnum was_in;

    if (GRAPPLING(ch) || GRAPPLED(ch))
    {
        ch->sendText("You are grappling with someone!\r\n");
        return (0);
    }

    if (ABSORBING(ch) || ABSORBBY(ch))
    {
        ch->sendText("You are struggling with someone!\r\n");
        return (0);
    }

    if (!AFF_FLAGGED(ch, AFF_SNEAK) ||
        (AFF_FLAGGED(ch, AFF_SNEAK) && GET_SKILL(ch, SKILL_MOVE_SILENTLY) < axion_dice(0)))
    {
        reveal_hiding(ch, 0);
    }

    if (ch == nullptr || dir < 0 || dir >= NUM_OF_DIRS)
        return 0;

    auto ex = EXIT(ch, dir);
    if(!ex) {
        if(ch->location.buildwalk(ch, static_cast<Direction>(dir)))
        {
            ex = EXIT(ch, dir);
        } else {
            ch->sendText("Alas, you cannot go that way...\r\n");
            return 0;
        }
    }

    if(!ex) {
        ch->sendText("Alas, you cannot go that way...\r\n");
        return 0;
    }

    if ((ex->exit_flags[EX_SECRET] && (ex->exit_flags[EX_CLOSED])))
    {
        ch->sendText("Alas, you cannot go that way...\r\n");
        return 0;
    }

    if (ex->exit_flags[EX_CLOSED])
    {
        if (!ex->keyword.empty())
            ch->send_to("The %s seems to be closed.\r\n", fname(ex->keyword.c_str()));
        else
            ch->sendText("It seems to be closed.\r\n");
        return 0;
    }

    if (!ex)
    {
        ch->sendText("Report this direction, it is illegal.\r\n");
        return 0;
    }

    if (auto wall = ch->location.searchObjects(79); wall && (GET_OBJ_COST(wall) == dir))
    {
        ch->sendText("That direction has a glacial wall blocking it.\r\n");
        return 0;
    }

    if (!ch->followers)
        return do_simple_move(ch, dir, need_specials_check);

    was_in = IN_ROOM(ch);
    if (!do_simple_move(ch, dir, need_specials_check))
        return 0;

    ch->followers.for_each([&](Character *k) {
                if ((IN_ROOM(k) == was_in) &&
            (GET_POS(k) >= POS_STANDING) &&
            (!AFF_FLAGGED(ch, AFF_ZANZOKEN) ||
             (AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(k, AFF_GROUP))))
        {
            act("You follow $N.\r\n", false, k, nullptr, ch, TO_CHAR);
            perform_move(k, dir, 1);
        }
        else if ((IN_ROOM(k) == was_in) &&
                 (GET_POS(k) >= POS_STANDING) &&
                 (AFF_FLAGGED(ch, AFF_ZANZOKEN) && AFF_FLAGGED(k, AFF_ZANZOKEN)) &&
                 (!AFF_FLAGGED(ch, AFF_GROUP) || !AFF_FLAGGED(k, AFF_GROUP)))
        {
            act("$N tries to zanzoken and escape, but your zanzoken matches $S!\r\n", false, k, nullptr,
                ch, TO_CHAR);
            act("$N tries to zanzoken and escape, but $n's zanzoken matches $S!\r\n", false, k, nullptr,
                ch, TO_NOTVICT);
            act("You zanzoken to try and escape, but $n's zanzoken matches yours!\r\n", false, k, nullptr,
                ch, TO_VICT);
            for (auto c : {ch, k})
                c->affect_flags.set(AFF_ZANZOKEN, false);
            perform_move(k, dir, 1);
        }
        else if ((IN_ROOM(k) == was_in) &&
                 (GET_POS(k) >= POS_STANDING) &&
                 (AFF_FLAGGED(ch, AFF_ZANZOKEN) && !AFF_FLAGGED(k, AFF_ZANZOKEN)))
        {
            act("You try to follow $N, but $E disappears in a flash of movement!\r\n", false, k, nullptr,
                ch, TO_CHAR);
            act("$n tries to follow $N, but $E disappears in a flash of movement!\r\n", false, k, nullptr,
                ch, TO_NOTVICT);
            act("$n tries to follow you, but you manage to zanzoken away!\r\n", false, k, nullptr, ch,
                TO_VICT);
            ch->affect_flags.set(AFF_ZANZOKEN, false);
        }

    });
    return 1;
}

ACMD(do_move)
{
    if (IS_NPC(ch))
    {
        perform_move(ch, subcmd - 1, 0);
        return;
    }
    if (PLR_FLAGGED(ch, PLR_SELFD))
    {
        ch->sendText("You are preparing to blow up!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_LIQUEFIED))
    {
        ch->sendText("You are liquefied right now!\r\n");
        return;
    }
    if (GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .51)
    {
        ch->sendText("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    }
    else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .5 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .51) &&
             GET_SKILL(ch, SKILL_CONCENTRATION) < 100)
    {
        ch->sendText("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    }
    else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .4 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .5) &&
             GET_SKILL(ch, SKILL_CONCENTRATION) < 80)
    {
        ch->sendText("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    }
    else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .3 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .4) &&
             GET_SKILL(ch, SKILL_CONCENTRATION) < 70)
    {
        ch->sendText("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    }
    else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .2 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .3) &&
             GET_SKILL(ch, SKILL_CONCENTRATION) < 60)
    {
        ch->sendText("You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
        return;
    }

    if (GET_COND(ch, DRUNK) > 4 && (Random::get<int>(1, 9) + GET_COND(ch, DRUNK)) >= Random::get<int>(14, 20))
    {
        ch->sendText("You wobble around and then fall on your ass.\r\n");
        act("@C$n@W wobbles around before falling on $s ass@n.", true, ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_SITTING;
        return;
    }

    if (FIGHTING(ch) && !IS_NPC(ch))
    {
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
    if (PLR_FLAGGED(ch, PLR_PILOTING))
    {
        auto vehicle = std::dynamic_pointer_cast<Structure>(ch->location.al.lock()).get();
        if(!vehicle)
        {
            ch->sendText("You are not in a vehicle!\r\n");
            return;
        }
        
        handle_drive_direction(ch, vehicle, subcmd - 1, 0);

        return;
    }

    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }
    if (!IS_NPC(ch))
    {
        int fail = false;
        auto shouldfail = [ch](const auto &o)
        {
            return KICHARGE(o) > 0 && USER(o) == ch;
        };
        if (auto obj = ch->location.searchObjects(shouldfail))
        {
            fail = true;
        }
        if (fail)
        {
            ch->sendText("You are too busy controlling your attack!\r\n");
            return;
        }
    }

    if (!IS_NPC(ch) && GET_LIMBCOND(ch, 0) <= 0 && GET_LIMBCOND(ch, 1) <= 0 && GET_LIMBCOND(ch, 2) <= 0 &&
        GET_LIMBCOND(ch, 3) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING))
    {
        ch->sendText("Unless you fly, you can't get far with no limbs.\r\n");
        return;
    }
    if (GRAPPLING(ch) || GRAPPLED(ch))
    {
        ch->sendText("You are grappling with someone!\r\n");
        return;
    }
    if (ABSORBING(ch))
    {
        ch->send_to("You are busy absorbing from %s!\r\n", GET_NAME(ABSORBING(ch)));
        return;
    }
    if (ABSORBBY(ch))
    {
        if (axion_dice(0) < GET_SKILL(ABSORBBY(ch), SKILL_ABSORB))
        {
            ch->send_to("You are being held by %s, they are absorbing you!\r\n", GET_NAME(ABSORBBY(ch)));
            ABSORBBY(ch)->send_to("%s struggles in your grasp!\r\n", GET_NAME(ch));
            WAIT_STATE(ch, PULSE_2SEC);
            return;
        }
        else
        {
            act("@c$N@W manages to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_NOTVICT);
            act("@WYou manage to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_VICT);
            act("@c$N@W manages to break loose of your hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_CHAR);
            ABSORBING(ABSORBBY(ch)) = nullptr;
            ABSORBBY(ch) = nullptr;
        }
    }
    if (!block_calc(ch))
    {
        return;
    }
    if (GET_EAVESDROP(ch) > 0)
    {
        ch->sendText("You stop eavesdropping.\r\n");
        ch->setBaseStat("listen_room", 0);
    }
    if (!IS_NPC(ch))
    {
        if (PRF_FLAGGED(ch, PRF_ARENAWATCH))
        {
            ch->pref_flags.set(PRF_ARENAWATCH, false);
            ch->setBaseStat<room_vnum>("arena_watch", -1);
        }
        if (auto rvn = ch->location.getVnum(); rvn != NOWHERE && rvn != 0 &&
                                               rvn != 1)
        {
            ch->registeredLocations["load_room"] = rvn;
        }

        auto ratio = ch->getBaseStat("burden_ratio");
        if (ratio >= 1.0)
        {
            ch->sendText("Your immense burden hinders your progress.\r\n");
            WAIT_STATE(ch, std::min<int>(PULSE_3SEC * ratio, PULSE_5SEC));
        }

        if (ch->location.getWhereFlag(WhereFlag::space) && GET_ADMLEVEL(ch) < 1)
        {
            ch->sendText("You struggle to cross the vast distance.\r\n");
            WAIT_STATE(ch, PULSE_6SEC);
        }
        else if ((GET_LIMBCOND(ch, 2) <= 0 && GET_LIMBCOND(ch, 3) <= 0) && GET_LIMBCOND(ch, 0) <= 0 &&
                 !AFF_FLAGGED(ch, AFF_FLYING))
        {
            act("@wYou slowly pull yourself along with your arm...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w slowly pulls $mself along with one arm...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 1) < 50)
            {
                ch->sendText("@RYour left arm is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 1) -= Random::get<int>(1, 5);
                if (GET_LIMBCOND(ch, 0) <= 0)
                {
                    act("@RYour left arm falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R left arm falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_5SEC);
        }
        else if ((GET_LIMBCOND(ch, 2) <= 0 && GET_LIMBCOND(ch, 3) <= 0) && GET_LIMBCOND(ch, 1) <= 0 &&
                 !AFF_FLAGGED(ch, AFF_FLYING))
        {
            act("@wYou slowly pull yourself along with your arm...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w slowly pulls $mself along with one arm...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 0) < 50)
            {
                ch->sendText("@RYour right arm is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 0) -= Random::get<int>(1, 5);
                if (GET_LIMBCOND(ch, 0) <= 0)
                {
                    act("@RYour right arm falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R right arm falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_5SEC);
        }
        else if ((GET_LIMBCOND(ch, 2) <= 0 && GET_LIMBCOND(ch, 3) <= 0) && !AFF_FLAGGED(ch, AFF_FLYING))
        {
            act("@wYou slowly pull yourself along with your arms...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w slowly pulls $mself along with one arms...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 1) < 50)
            {
                ch->sendText("@RYour left arm is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 1) -= Random::get<int>(1, 5);
                if (GET_LIMBCOND(ch, 1) <= 0)
                {
                    act("@RYour left arm falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R left arm falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            if (GET_LIMBCOND(ch, 0) < 50)
            {
                ch->sendText("@RYour right arm is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 0) -= Random::get<int>(1, 5);
                if (GET_LIMBCOND(ch, 0) <= 0)
                {
                    act("@RYour right arm falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R right arm falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_3SEC);
        }
        else if (GET_LIMBCOND(ch, 2) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING))
        {
            act("@wYou hop on one leg...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w hops on one leg...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 3) < 50)
            {
                ch->sendText("@RYour left leg is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 3) -= Random::get<int>(1, 5);
                if (GET_LIMBCOND(ch, 3) <= 0)
                {
                    act("@RYour left leg falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R left leg falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_2SEC);
        }
        else if (GET_LIMBCOND(ch, 3) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING))
        {
            act("@wYou hop on one leg...@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w hops on one leg...@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (GET_LIMBCOND(ch, 2) < 50)
            {
                ch->sendText("@RYour right leg is damaged by the forced use!@n\r\n");
                GET_LIMBCOND(ch, 2) -= Random::get<int>(1, 5);
                if (GET_LIMBCOND(ch, 2) <= 0)
                {
                    act("@RYour right leg falls apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@r$n's@R right leg falls apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            WAIT_STATE(ch, PULSE_2SEC);
        }
        else if (GET_POS(ch) == POS_RESTING)
        {
            act("@wYou crawl on your hands and knees.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w crawls on $s hands and knees.@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (SITS(ch))
            {
                Object *chair = SITS(ch);
                chair->sitting.reset();
                ch->sits.reset();
            }
            WAIT_STATE(ch, PULSE_3SEC);
        }
        else if (GET_POS(ch) == POS_SITTING)
        {
            act("@wYou shuffle on your hands and knees.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w shuffles on $s hands and knees.@n", true, ch, nullptr, nullptr, TO_ROOM);
            if (SITS(ch))
            {
                Object *chair = SITS(ch);
                chair->sitting.reset();
                ch->sits.reset();
            }
            WAIT_STATE(ch, PULSE_2SEC);
        }
        else if (GET_POS(ch) < POS_RESTING)
        {
            ch->sendText("You are in no condition to move! Try standing...\r\n");
            return;
        }
    }
    perform_move(ch, subcmd - 1, 0);
}

static int find_door(Character *ch, const char *type, char *dir, const char *cmdname)
{
    int door;

    if (*dir)
    { /* a direction was specified */
        if ((door = search_block(dir, dirs, false)) < 0 &&
            (door = search_block(dir, abbr_dirs, false)) < 0)
        { /* Partial Match */
            ch->sendText("That's not a direction.\r\n");
            return (-1);
        }
        if (auto ex = EXIT(ch, door); ex)
        { /* Braces added according to indent. -gg */
            if (!ex->keyword.empty())
            {
                if (is_name(type, ex->keyword.c_str()))
                    return (door);
                else
                {
                    ch->send_to("I see no %s there.\r\n", type);
                    return (-1);
                }
            }
            else
                return (door);
        }
        else
        {
            ch->send_to("I really don't see how you can %s anything there.\r\n", cmdname);
            return (-1);
        }
    }
    else
    { /* try to locate the keyword */
        if (!*type)
        {
            ch->send_to("What is it you want to %s?\r\n", cmdname);
            return (-1);
        }
        for (door = 0; door < NUM_OF_DIRS; door++)
            if (auto ex = EXIT(ch, door); ex)
                if (!ex->keyword.empty())
                    if (is_name(type, ex->keyword.c_str()))
                        return (door);

        ch->send_to("There doesn't seem to be %s %s that could be manipulated in that way here.\r\n", AN(type), type);
        return (-1);
    }
}

static int has_key(Character *ch, obj_vnum key)
{
    return ch->searchInventory(key) ? true : false;
}

#define NEED_OPEN (1 << 0)
#define NEED_CLOSED (1 << 1)
#define NEED_UNLOCKED (1 << 2)
#define NEED_LOCKED (1 << 3)

const char *cmd_door[NUM_DOOR_CMD] =
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

static void OPEN_DOOR(room_vnum room, Object *obj, int door)
{
    if (obj)
    {
        int val = GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS);
        val &= ~CONT_CLOSED;
        SET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS, val);
    }
    else
    {
        auto r = get_room(room);
        if (!r)
            return;
        auto cdir = static_cast<Direction>(door);
        if (!r->exits.contains(cdir))
            return;
        r->exits.at(cdir).exit_flags.set(EX_CLOSED, false);
    }
}

static void CLOSE_DOOR(room_vnum room, Object *obj, int door)
{
    if (obj)
    {
        int val = GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS);
        val |= CONT_CLOSED;
        SET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS, val);
    }
    else
    {
        auto r = get_room(room);
        if (!r)
            return;
        auto cdir = static_cast<Direction>(door);
        if (!r->exits.contains(cdir))
            return;
        r->exits.at(cdir).exit_flags.set(EX_CLOSED);
    }
}

static void LOCK_DOOR(room_vnum room, Object *obj, int door)
{
    if (obj)
    {
        int val = GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS);
        val |= CONT_LOCKED;
        SET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS, val);
    }
    else
    {
        auto r = get_room(room);
        if (!r)
            return;
        auto cdir = static_cast<Direction>(door);
        if (!r->exits.contains(cdir))
            return;
        r->exits.at(cdir).exit_flags.set(EX_LOCKED);
    }
}

static void UNLOCK_DOOR(room_vnum room, Object *obj, int door)
{
    if (obj)
    {
        int val = GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS);
        val &= ~CONT_LOCKED;
        SET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS, val);
    }
    else
    {
        auto r = get_room(room);
        if (!r)
            return;
        auto cdir = static_cast<Direction>(door);
        if (!r->exits.contains(cdir))
            return;
        r->exits.at(cdir).exit_flags.set(EX_LOCKED, false);
    }
}

static void TOGGLE_LOCK(room_vnum room, Object *obj, int door)
{
    if (obj)
    {
        int val = GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS);
        val ^= CONT_LOCKED;
        SET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS, val);
    }
    else
    {
        auto r = get_room(room);
        if (!r)
            return;
        auto cdir = static_cast<Direction>(door);
        if (!r->exits.contains(cdir))
            return;
        r->exits.at(cdir).exit_flags.toggle(EX_LOCKED);
    }
}

static void do_doorcmd(Character *ch, Object *obj, int door, int scmd)
{
    char buf[MAX_STRING_LENGTH];
    size_t len;
    int num = 0;
    room_rnum other_room = NOWHERE;
    Object *hatch = nullptr, *obj2 = nullptr, *next_obj;

    std::optional<Destination> ex;
    std::optional<Destination> back;
    auto cdoor = static_cast<Direction>(door);

    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE)
    {
        if (auto pdest = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)) != NOWHERE)
        {
            num = IN_ROOM(ch);
            ch->leaveLocation();
            ch->moveToLocation(pdest);
        }
        auto ishatch = [](const auto &o)
        {
            return GET_OBJ_TYPE(o) == ITEM_HATCH;
        };
        hatch = ch->location.searchObjects(ishatch);
        obj2 = nullptr;
    }

    if (!door_mtrigger(ch, scmd, door))
        return;

    if (!door_wtrigger(ch, scmd, door))
        return;

    if (!obj)
    {
        ex = ch->location.getExit(static_cast<Direction>(door));
        std::optional<Destination> back;
        if (ex)
        {
            back = ex->getReverse();
        }
    }

    len = snprintf(buf, sizeof(buf), "$n %ss ", cmd_door[scmd]);

    switch (scmd)
    {
    case SCMD_OPEN:
        if (obj)
        {
            if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle))
            {
                OPEN_DOOR(IN_ROOM(ch), vehicle, door);
                if (GET_OBJ_VNUM(obj) > 19199)
                {
                    ch->location.sendText("@wThe ship hatch opens slowly and settles onto the ground outside.\r\n");
                    vehicle->location.sendText("@wThe ship hatch opens slowly and settles onto the ground.\r\n");
                    if (vehicle->location.getWhereFlag(WhereFlag::space))
                    {
                        ch->location.sendText("@wA great vortex forms as air begins to get sucked out into the void!\r\n");
                    }
                }
                else
                {
                    act("@wYou open @c$p@w.", true, ch, obj, 0, TO_CHAR);
                    act("@C$n@w opens @c$p@w.", true, ch, obj, 0, TO_ROOM);
                    vehicle->location.send_to("@wThe door to %s@w is opened from the other side.\r\n",
                                              vehicle->getShortDescription());
                }
                vehicle = nullptr;
            }
            if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch))
            {
                OPEN_DOOR(IN_ROOM(ch), hatch, door);
                ch->leaveLocation();
                ch->moveToLocation(num);
                if (GET_OBJ_VNUM(obj) > 19199)
                {
                    ch->location.sendText("@wThe ship hatch opens slowly and settles onto the ground.\r\n");
                    hatch->location.sendText("@wThe ship hatch opens slowly.\r\n");
                    if (obj->location.getWhereFlag(WhereFlag::space))
                    {
                        ch->location.sendText("@wThe air starts getting sucked out into space as the hatch opens!\r\n");
                    }
                }
                else
                {
                    act("@wYou open @c$p@w.", true, ch, obj, 0, TO_CHAR);
                    act("@C$n@w opens @c$p@w.", true, ch, obj, 0, TO_ROOM);
                    hatch->location.sendText("@wThe door is opened from the other side.\r\n");
                }
                hatch = nullptr;
            }
        }
        OPEN_DOOR(IN_ROOM(ch), obj, door);
        if (back)
        {
            OPEN_DOOR(other_room, obj, static_cast<int>(rev_dir.at(cdoor)));
        }
        if (!obj)
        {
            ch->send_to("You open the %s that leads %s.\r\n", !ex->keyword.empty() ? ex->keyword : "door", dirs[door]);
        }
        else if (GET_OBJ_TYPE(obj) != ITEM_VEHICLE && GET_OBJ_TYPE(obj) != ITEM_HATCH)
        {
            ch->send_to("You open %s.\r\n", obj->getShortDescription());
        }
        break;

    case SCMD_CLOSE:
        if (obj)
        {
            if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle))
            {
                CLOSE_DOOR(IN_ROOM(ch), vehicle, door);
                if (GET_OBJ_VNUM(obj) > 19199)
                {
                    ch->location.sendText("@wThe ship hatch slowly closes, sealing the ship from the outside.\r\n");
                    vehicle->location.sendText("@wThe ship hatch slowly closes, sealing the ship.\r\n");
                    if (vehicle->location.getWhereFlag(WhereFlag::space))
                    {
                        ch->location.sendText("@wThe air stops getting sucked out into space as the hatch seals!\r\n");
                    }
                }
                else
                {
                    act("@wYou close @c$p@w.", true, ch, obj, 0, TO_CHAR);
                    act("@C$n@w closes @c$p@w.", true, ch, obj, 0, TO_ROOM);
                    vehicle->location.send_to("@wThe door to %s@w is closed from the other side.\r\n",
                                              vehicle->getShortDescription());
                }
                vehicle = NULL;
            }
            if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch))
            {
                CLOSE_DOOR(IN_ROOM(ch), hatch, door);
                ch->leaveLocation();
                ch->moveToLocation(num);
                if (GET_OBJ_VNUM(obj) > 19199)
                {
                    ch->location.sendText("@wThe ship hatch slowly closes, sealing the ship.\r\n");
                    hatch->location.sendText("@wThe ship hatch slowly closes, sealing the ship from the outside.\r\n");
                    if (obj->location.getWhereFlag(WhereFlag::space))
                    {
                        ch->location.sendText("@wAir stops getting sucked out into space as the hatch seals!\r\n");
                    }
                }
                else
                {
                    act("@wYou close @c$p@w.", true, ch, obj, 0, TO_CHAR);
                    act("@C$n@w closes @c$p@w.", true, ch, obj, 0, TO_ROOM);
                    hatch->location.send_to("@wThe door to %s@w is closed from the other side.\r\n",
                                            hatch->getShortDescription());
                }
                hatch = NULL;
            }
        }
        CLOSE_DOOR(IN_ROOM(ch), obj, door);
        if (back)
        {
            CLOSE_DOOR(other_room, obj, static_cast<int>(rev_dir.at(cdoor)));
        }
        if (!obj)
        {
            ch->send_to("You close the %s that leads %s.\r\n",
                        !EXIT(ch, door)->keyword.empty() ? EXIT(ch, door)->keyword.c_str() : "door", dirs[door]);
        }
        else if (GET_OBJ_TYPE(obj) != ITEM_VEHICLE && GET_OBJ_TYPE(obj) != ITEM_HATCH)
        {
            ch->send_to("You close %s.\r\n", obj->getShortDescription());
        }
        break;

    case SCMD_LOCK:
        if (obj)
        {
            if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle))
            {
                LOCK_DOOR(IN_ROOM(ch), vehicle, door);
                vehicle = NULL;
            }
            if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch))
            {
                LOCK_DOOR(IN_ROOM(ch), hatch, door);
                ch->leaveLocation();
                ch->moveToLocation(num);
                hatch = NULL;
            }
        }
        LOCK_DOOR(IN_ROOM(ch), obj, door);
        if (back)
        {
            LOCK_DOOR(other_room, obj, static_cast<int>(rev_dir.at(cdoor)));
        }
        if (!obj)
        {
            ch->send_to("You lock the %s that leads %s.\r\n",
                        !EXIT(ch, door)->keyword.empty() ? EXIT(ch, door)->keyword.c_str() : "door", dirs[door]);
        }
        else
        {
            ch->send_to("You lock %s.\r\n", obj->getShortDescription());
        }
        break;

    case SCMD_UNLOCK:
        if (obj)
        {
            if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle))
            {
                UNLOCK_DOOR(IN_ROOM(ch), vehicle, door);
                vehicle = NULL;
            }
            if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch))
            {
                UNLOCK_DOOR(IN_ROOM(ch), hatch, door);
                ch->leaveLocation();
                ch->moveToLocation(num);
                hatch = NULL;
            }
        }
        UNLOCK_DOOR(IN_ROOM(ch), obj, door);
        if (back)
        {
            UNLOCK_DOOR(other_room, obj, static_cast<int>(rev_dir.at(cdoor)));
        }
        if (!obj)
        {
            ch->send_to("You unlock the %s that leads %s.\r\n",
                        !EXIT(ch, door)->keyword.empty() ? EXIT(ch, door)->keyword.c_str() : "door", dirs[door]);
        }
        else
        {
            ch->send_to("You unlock %s.\r\n", obj->getShortDescription());
        }
        break;

    case SCMD_PICK:
        TOGGLE_LOCK(IN_ROOM(ch), obj, door);
        if (back)
            TOGGLE_LOCK(other_room, obj, static_cast<int>(rev_dir.at(cdoor)));
        ch->sendText("The lock quickly yields to your skills.\r\n");
        len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
        break;
    }

    /* Notify the room. */
    char dbuf[100];
    if (!obj)
    {
        sprintf(dbuf, "%s", dirs[door]);
    }
    if (len < sizeof(buf))
        snprintf(buf + len, sizeof(buf) - len, "%s%s%s%s.",
                 obj ? "" : "the ", obj ? "$p" : !EXIT(ch, door)->keyword.empty() ? "$F"
                                                                                  : "door",
                 obj ? "" : " that leads ",
                 obj ? "" : dbuf);
    if (!obj || IN_ROOM(obj) != NOWHERE)
        act(buf, false, ch, obj, obj ? 0 : EXIT(ch, door)->keyword.c_str(), TO_ROOM);

    /* Notify the other room */
    if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE))
    {
        ex->send_to("The %s that leads %s is %s%s from the other side.\r\n",
                    !back->keyword.empty() ? fname(back->keyword.c_str()) : "door", dbuf, cmd_door[scmd],
                    scmd == SCMD_CLOSE ? "d" : "ed");
    }
    else if (back && (scmd == SCMD_LOCK || scmd == SCMD_UNLOCK))
    {
        ex->send_to("The %s that leads %s is %sed from the other side.\r\n",
                    !back->keyword.empty() ? fname(back->keyword.c_str()) : "door", dbuf, cmd_door[scmd]);
    }
    *dbuf = '\0';
}

static int ok_pick(Character *ch, obj_vnum keynum, int pickproof, int dclock, int scmd, Object *hatch)
{
    int skill_lvl, found = false;
    Object *obj, *next_obj;
    obj = ch->searchInventory(18);

    if (scmd != SCMD_PICK)
        return (1);

    /* PICKING_LOCKS is not an untrained skill */
    if (!GET_SKILL(ch, SKILL_OPEN_LOCK))
    {
        ch->sendText("You have no idea how!\r\n");
        return (0);
    }
    if (found == false)
    {
        ch->sendText("You need a lock picking kit.\r\n");
        return (0);
    }
    if (hatch && (GET_OBJ_TYPE(hatch) == ITEM_HATCH || GET_OBJ_TYPE(hatch) == ITEM_VEHICLE))
    {
        ch->sendText("No picking ship hatches.\r\n");
        hatch = nullptr;
        return (0);
    }
    skill_lvl = roll_skill(ch, SKILL_OPEN_LOCK);
    if (dclock == 0)
    {
        dclock = Random::get<int>(1, 101);
    }

    if (keynum == NOTHING)
    {
        ch->sendText("Odd - you can't seem to find a keyhole.\r\n");
    }
    else if (pickproof)
    {
        ch->sendText("It resists your attempts to pick it.\r\n");
        act("@c$n@w puts a set of lockpick tools away.@n", true, ch, nullptr, nullptr, TO_ROOM);
        /* The -2 is here because that is a penality for not having a set of
         * thieves' tools. If the player has them, that modifier will be accounted
         * for in roll_skill, and negate (or surpass) this.
         */
    }
    else if ((ch->getCurVital(CharVital::stamina)) < GET_MAX_MOVE(ch) / 30)
    {
        ch->sendText("You don't have the stamina to try, it takes percision to pick locks."
                     "Not shaking tired hands.\r\n");
    }
    else if (dclock > (skill_lvl - 2))
    {
        ch->sendText("You failed to pick the lock...\r\n");
        act("@c$n@w puts a set of lockpick tools away.@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->modCurVital(CharVital::stamina, -(ch->getCurVital(CharVital::stamina) / 30));
    }
    else
    {
        ch->modCurVital(CharVital::stamina, -(ch->getCurVital(CharVital::stamina) / 30));
        return (1);
    }

    return (0);
}

#define DOOR_IS_OPENABLE(ch, obj, door) ((obj) ? ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) &&    \
                                                  OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) ||     \
                                                     ((GET_OBJ_TYPE(obj) == ITEM_VEHICLE) &&  \
                                                      OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) || \
                                                     ((GET_OBJ_TYPE(obj) == ITEM_HATCH) &&    \
                                                      OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) || \
                                                     ((GET_OBJ_TYPE(obj) == ITEM_WINDOW) &&   \
                                                      OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) || \
                                                     ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) &&   \
                                                      OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))    \
                                               : (EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door) ((obj) ? (!OBJVAL_FLAGGED(obj, CONT_CLOSED)) : (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door) ((obj) ? (!OBJVAL_FLAGGED(obj, CONT_LOCKED)) : (!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? (OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) : (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_SECRET(ch, obj, door) ((obj) ? (OBJVAL_FLAGGED(obj, CONT_SECRET)) : (EXIT_FLAGGED(EXIT(ch, door), EX_SECRET)))

#define DOOR_IS_CLOSED(ch, obj, door) (!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door) (!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door) ((obj) ? (GET_OBJ_VAL(obj, VAL_KEY_KEYCODE)) : (EXIT(ch, door)->key))
#define DOOR_DCLOCK(ch, obj, door) ((obj) ? (GET_OBJ_VAL(obj, VAL_DOOR_DCLOCK)) : EXIT(ch, door)->dclock)

ACMD(do_gen_door)
{
    int door = -1;
    obj_vnum keynum;
    char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
    Object *obj = nullptr;
    Character *victim = nullptr;

    skip_spaces(&argument);
    if (!*argument)
    {
        ch->send_to("%c%s what?\r\n", toupper(*cmd_door[subcmd]), cmd_door[subcmd] + 1);
        return;
    }
    two_arguments(argument, type, dir);
    if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
        door = find_door(ch, type, dir, cmd_door[subcmd]);

    if ((obj) &&
        (GET_OBJ_TYPE(obj) != ITEM_CONTAINER && GET_OBJ_TYPE(obj) != ITEM_VEHICLE && GET_OBJ_TYPE(obj) != ITEM_HATCH))
    {
        obj = nullptr;
        door = find_door(ch, type, dir, cmd_door[subcmd]);
    }

    if ((obj) || (door >= 0))
    {
        keynum = DOOR_KEY(ch, obj, door);
        if (!DOOR_DCLOCK(ch, obj, door))
        {
            if (obj)
            {
                SET_OBJ_VAL(obj, VAL_DOOR_DCLOCK, 20);
            }
            else
            {
                auto cdir = static_cast<Direction>(door);
                if (auto ex = ch->location.getExit(cdir); ex)
                {
                    ex->dclock = 20;
                    ch->location.replaceExit(*ex);
                }
            }
        }
        if (!(DOOR_IS_OPENABLE(ch, obj, door)))
            act("You can't $F that!", false, ch, nullptr, cmd_door[subcmd], TO_CHAR);
        else if (!DOOR_IS_OPEN(ch, obj, door) &&
                 IS_SET(flags_door[subcmd], NEED_OPEN))
            ch->sendText("But it's already closed!\r\n");
        else if (!DOOR_IS_CLOSED(ch, obj, door) &&
                 IS_SET(flags_door[subcmd], NEED_CLOSED))
            ch->sendText("But it's currently open!\r\n");
        else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
                 IS_SET(flags_door[subcmd], NEED_LOCKED))
            ch->sendText("Oh.. it wasn't locked, after all..\r\n");
        else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
                 IS_SET(flags_door[subcmd], NEED_UNLOCKED))
            ch->sendText("It seems to be locked.\r\n");
        else if (!has_key(ch, keynum) && !ADM_FLAGGED(ch, ADM_NOKEYS) &&
                 ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
            ch->sendText("You don't seem to have the proper key.\r\n");
        else if (!obj &&
                 ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), DOOR_DCLOCK(ch, obj, door), subcmd, nullptr))
            do_doorcmd(ch, obj, door, subcmd);
        else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), DOOR_DCLOCK(ch, obj, door), subcmd, obj) && obj)
            do_doorcmd(ch, obj, door, subcmd);
    }
}

static int do_simple_enter(Character *ch, Object *obj, int need_specials_check)
{
    room_rnum dest_room = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST));
    auto was_in = ch->location;
    int need_movement = 0;

    Destination d;
    d.al = get_room(dest_room)->shared_from_this();

    /* charmed? */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
        ch->location == ch->master->location)
    {
        ch->sendText("The thought of leaving your master makes you weep.\r\n");
        act("$n bursts into tears.", false, ch, nullptr, nullptr, TO_ROOM);
        return (0);
    }

    /* move points needed is avg. move loss for src and destination sect type */
    need_movement = calcNeedMovementGravity(ch);

    if (GET_LEVEL(ch) <= 1)
    {
        need_movement = 0;
    }
    if ((ch->getCurVital(CharVital::stamina)) < need_movement && !AFF_FLAGGED(ch, AFF_FLYING) && !IS_NPC(ch))
    {
        if (need_specials_check && ch->master)
            ch->sendText("You are too exhausted to follow.\r\n");
        else
            ch->sendText("You are too exhausted.\r\n");

        return (0);
    }

    if (d.getRoomFlag(ROOM_TUNNEL) &&
        d.countPlayers() >= CONFIG_TUNNEL_SIZE)
    {
        if (CONFIG_TUNNEL_SIZE > 1)
            ch->sendText("There isn't enough room for you to go there!\r\n");
        else
            ch->sendText("There isn't enough room there for more than one person!\r\n");
        return (0);
    }
    /* Mortals and low level gods cannot enter greater god rooms. */
    if (d.getRoomFlag(ROOM_GODROOM) &&
        GET_ADMLEVEL(ch) < ADMLVL_GRGOD)
    {
        ch->sendText("You aren't godly enough to use that room!\r\n");
        return (0);
    }
    /* Now we know we're allowed to go into the room. */
    if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)) && !AFF_FLAGGED(ch, AFF_FLYING))
        ch->modCurVital(CharVital::stamina, -need_movement);

    act("$n enters $p.", true, ch, obj, nullptr, TO_ROOM | TO_SNEAKRESIST);

    if (DRAGGING(ch))
    {
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, DRAGGING(ch), TO_ROOM);
    }
    if (CARRYING(ch))
    {
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, CARRYING(ch), TO_ROOM);
    }
    ch->leaveLocation();
    ch->moveToLocation(d);

    /* move them first, then move them back if they aren't allowed to go. */
    /* see if an entry trigger disallows the move */
    if (!entry_mtrigger(ch))
    {
        ch->leaveLocation();
        ch->moveToLocation(was_in);
        return 0;
    }

    if (GET_OBJ_TYPE(obj) == ITEM_PORTAL)
        act("$n arrives from $p.", false, ch, obj, nullptr, TO_ROOM | TO_SNEAKRESIST);
    else
        act("$n arrives from outside.", false, ch, nullptr, nullptr, TO_ROOM | TO_SNEAKRESIST);
    if (auto drg = DRAGGING(ch))
    {
        act("@wYou drag @C$N@w with you.@n", true, ch, nullptr, drg, TO_CHAR);
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, drg, TO_ROOM);
        if (!AFF_FLAGGED(drg, AFF_KNOCKED) && !AFF_FLAGGED(drg, AFF_SLEEP) && Random::get<int>(1, 3))
        {
            drg->sendText("You feel your sleeping body being moved.\r\n");
            if (IS_NPC(drg) && !FIGHTING(drg))
            {
                set_fighting(drg, ch);
            }
        }
        drg->leaveLocation();
        drg->moveToLocation(ch);
        if (auto s = SITS(drg))
        {
            s->clearLocation();
            s->moveToLocation(ch);
        }
    }
    if (auto carry = CARRYING(ch); carry)
    {
        act("@wYou carry @C$N@w with you.@n", true, ch, nullptr, carry, TO_CHAR);
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, carry, TO_ROOM);
        if (!AFF_FLAGGED(carry, AFF_KNOCKED) && !AFF_FLAGGED(carry, AFF_SLEEP) && Random::get<int>(1, 3))
        {
            carry->sendText("You feel your sleeping body being moved.\r\n");
        }
        carry->leaveLocation();
        carry->moveToLocation(ch);
        if (auto s = SITS(carry); s)
        {
            s->clearLocation();
            s->moveToLocation(ch);
        }
    }

    if (ch->desc)
        ch->lookAtLocation();

    entry_memory_mtrigger(ch);
    greet_memory_mtrigger(ch);

    return 1;
}

static int perform_enter_obj(Character *ch, Object *obj, int need_specials_check)
{
    room_rnum was_in = IN_ROOM(ch);
    int could_move = false;
    struct follow_type *k;

    if (GRAPPLING(ch) || GRAPPLED(ch))
    {
        ch->sendText("You are grappling with someone!\r\n");
        return (0);
    }

    if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE || GET_OBJ_TYPE(obj) == ITEM_PORTAL)
    {
        if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
        {
            ch->sendText("But it's closed!\r\n");
        }
        else if ((GET_OBJ_VAL(obj, VAL_PORTAL_DEST) != NOWHERE) &&
                 (real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)) != NOWHERE))
        {
            if (GET_OBJ_VAL(obj, VAL_PORTAL_DEST) >= 45000 && GET_OBJ_VAL(obj, VAL_PORTAL_DEST) <= 45099)
            {
                int filled = false;
                auto dest = get_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST));
                auto people = dest->getPeople().snapshot_weak();
                for (auto tch : volcano::util::filter_raw(people))
                {
                    filled = true;
                    break;
                }
                if (filled == true)
                {
                    ch->sendText("Only one person can fit in there at a time.\r\n");
                    return (0);
                }
            }
            if ((could_move = do_simple_enter(ch, obj, need_specials_check)))
                ch->followers.for_each([&](Character* k) {
                    if ((IN_ROOM(k) == was_in) && (GET_POS(k) >= POS_STANDING))
                    {
                        act("You follow $N.\r\n", false, k, nullptr, ch, TO_CHAR);
                        perform_enter_obj(k, obj, 1);
                    }
                });        
        }
        else
        {
            ch->sendText("It doesn't look like you can enter it at the moment.\r\n");
        }
    }
    else
    {
        ch->sendText("You can't enter that!\r\n");
    }
    return could_move;
}

ACMD(do_enter)
{
    Object *obj = nullptr;
    char buf[MAX_INPUT_LENGTH];
    int door, move_dir = -1;

    one_argument(argument, buf);

    if (*buf)
    { /* an argument was supplied, search for door keyword */
        /* Is the object in the room? */
        obj = get_obj_in_list_vis(ch, buf, nullptr, ch->location.getObjects());
        /* Is the object in the character's inventory? */
        if (!obj)
            obj = get_obj_in_list_vis(ch, buf, nullptr, ch->getInventory());
        /* Is the character carrying the object? */
        if (!obj)
            obj = get_obj_in_equip_vis(ch, buf, nullptr, ch->getEquipment());
        /* We have an object to enter */
        if (obj)
            perform_enter_obj(ch, obj, 0);
        /* Is there a door to enter? */
        else
        {
            for (auto &[door, e] : ch->location.getExits())
            {
                if (!e.keyword.empty() && isname(buf, e.keyword.c_str()))
                {
                    move_dir = static_cast<int>(door);
                    break;
                }
            }
            /* Did we find what they wanted to enter. */
            if (move_dir > -1)
                perform_move(ch, move_dir, 1);
            else
                ch->send_to("There is no %s here.\r\n", buf);
        }
    }
    else if (ch->location.getRoomFlag(ROOM_INDOORS))
    {
        ch->sendText("You are already indoors.\r\n");
    }
    else
    {
        /* try to locate an entrance */
        for (auto &[door, e] : ch->location.getExits())
        {
            if (!e.exit_flags[EX_CLOSED] && e.getRoomFlag(ROOM_INDOORS))
            {
                move_dir = static_cast<int>(door);
                break;
            }
        }
        if (move_dir > -1)
            perform_move(ch, move_dir, 1);
        else
            ch->sendText("You can't seem to find anything to enter.\r\n");
    }
}

static int do_simple_leave(Character *ch, Object *obj, int need_specials_check)
{
    room_rnum dest_room = NOWHERE;
    int need_movement = 0;
    Object *vehicle = nullptr;

    auto was_in = ch->location;

    if (GET_OBJ_TYPE(obj) != ITEM_PORTAL)
    {
        vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(obj, VAL_HATCH_DEST));
    }

    if (vehicle == nullptr && GET_OBJ_TYPE(obj) != ITEM_PORTAL)
    {
        ch->sendText("That doesn't appear to lead anywhere.\r\n");
        return 0;
    }

    if (GET_OBJ_TYPE(obj) == ITEM_PORTAL && OBJVAL_FLAGGED(obj, CONT_CLOSED))
    {
        ch->sendText("But it's closed!\r\n");
        return 0;
    }

    if (vehicle)
    {
        if ((dest_room = IN_ROOM(vehicle)) == NOWHERE)
        {
            ch->sendText("That doesn't appear to lead anywhere.\r\n");
            return 0;
        }
    }
    if (vehicle == nullptr)
    {
        if ((dest_room = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST))) == NOWHERE)
        {
            ch->sendText("That doesn't appear to lead anywhere.\r\n");
            return 0;
        }
    }

    Destination d(dest_room);

    /* charmed? */
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
        ch->location == ch->master->location)
    {
        ch->sendText("The thought of leaving your master makes you weep.\r\n");
        act("$n bursts into tears.", false, ch, nullptr, nullptr, TO_ROOM);
        return (0);
    }

    /* move points needed is avg. move loss for src and destination sect type */
    need_movement = calcNeedMovementGravity(ch);

    if (ch->getCurVital(CharVital::stamina) < need_movement && !AFF_FLAGGED(ch, AFF_FLYING) && !IS_NPC(ch))
    {
        if (need_specials_check && ch->master)
            ch->sendText("You are too exhausted to follow.\r\n");
        else
            ch->sendText("You are too exhausted.\r\n");

        return (0);
    }

    if (d.getRoomFlag(ROOM_TUNNEL) &&
        d.countPlayers() >= CONFIG_TUNNEL_SIZE)
    {
        if (CONFIG_TUNNEL_SIZE > 1)
            ch->sendText("There isn't enough room for you to go there!\r\n");
        else
            ch->sendText("There isn't enough room there for more than one person!\r\n");
        return (0);
    }
    /* Now we know we're allowed to go into the room. */
    if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)) && !AFF_FLAGGED(ch, AFF_FLYING))
        ch->modCurVital(CharVital::stamina, -need_movement);

    act("$n leaves $p.", true, ch, vehicle, nullptr, TO_ROOM | TO_SNEAKRESIST);

    if (auto dr = DRAGGING(ch))
    {
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, dr, TO_ROOM);
    }
    if (auto car = CARRYING(ch))
    {
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, car, TO_ROOM);
    }
    auto oldloc = ch->location;
    ch->leaveLocation();
    ch->moveToLocation(d);

    /* move them first, then move them back if they aren't allowed to go. */
    /* see if an entry trigger disallows the move */
    if (!entry_mtrigger(ch))
    {
        ch->leaveLocation();
        ch->moveToLocation(was_in);
        return 0;
    }

    if (vehicle)
    {
        act("$n arrives from inside $p.", true, ch, vehicle, nullptr, TO_ROOM | TO_SNEAKRESIST);
    }
    else
    {
        act("$n arrives from inside", true, ch, nullptr, nullptr, TO_ROOM | TO_SNEAKRESIST);
    }
    if (auto drg = DRAGGING(ch))
    {
        act("@wYou drag @C$N@w with you.@n", true, ch, nullptr, drg, TO_CHAR);
        act("@C$n@w drags @c$N@w with $m.@n", true, ch, nullptr, drg, TO_ROOM);
        drg->leaveLocation();
        drg->moveToLocation(ch);
        if (auto si = SITS(drg))
        {
            si->clearLocation();
            si->moveToLocation(ch);
        }
        if (!AFF_FLAGGED(drg, AFF_KNOCKED) && !AFF_FLAGGED(drg, AFF_SLEEP) && Random::get<int>(1, 3))
        {
            drg->sendText("You feel your sleeping body being moved.\r\n");
            if (IS_NPC(drg) && !FIGHTING(drg))
            {
                set_fighting(drg, ch);
            }
        }
    }
    if (auto car = CARRYING(ch))
    {
        act("@wYou carry @C$N@w with you.@n", true, ch, nullptr, car, TO_CHAR);
        act("@C$n@w carries @c$N@w with $m.@n", true, ch, nullptr, car, TO_ROOM);
        car->leaveLocation();
        car->moveToLocation(ch);
        if (auto si = SITS(car))
        {
            si->clearLocation();
            si->moveToLocation(ch);
        }
        if (!AFF_FLAGGED(car, AFF_KNOCKED) && !AFF_FLAGGED(car, AFF_SLEEP) && Random::get<int>(1, 3))
        {
            car->sendText("You feel your sleeping body being moved.\r\n");
        }
    }

    char buf3[MAX_STRING_LENGTH];
    oldloc.getZone()->sendToSense(ch, "approaching nearby", ch);
    sprintf(buf3, "@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection range.@n",
            add_commas(ch->getPL()).c_str());
    send_to_scouter(buf3, ch, 0, 0);

    if (ch->desc)
    {
        act(obj->getLookDescription(), true, ch, obj, nullptr, TO_CHAR);
        ch->lookAtLocation();
    }

    entry_memory_mtrigger(ch);
    greet_memory_mtrigger(ch);

    return 1;
}

static int perform_leave_obj(Character *ch, Object *obj, int need_specials_check)
{
    room_rnum was_in = IN_ROOM(ch);
    int could_move = false;


    if (GRAPPLING(ch) || GRAPPLED(ch))
    {
        ch->sendText("You are grappling with someone!\r\n");
        return (0);
    }

    if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
    {
        ch->sendText("But the way out is closed.\r\n");
    }
    else
    {
        if (GET_OBJ_VAL(obj, VAL_HATCH_DEST) != NOWHERE)
            if ((could_move = do_simple_leave(ch, obj, need_specials_check)))
                ch->followers.for_each([&](Character* k) {
                    if ((IN_ROOM(k) == was_in) &&
                        (GET_POS(k) >= POS_STANDING))
                    {
                        act("You follow $N.\r\n", false, k, nullptr, ch, TO_CHAR);
                        perform_leave_obj(k, obj, 1);
                    }
                });
                    
    }
    return could_move;
}

ACMD(do_leave)
{
    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }

    auto leave_obj = [ch](const auto &o)
    {
    return ch->canSee(o) && GET_OBJ_TYPE(o) == ITEM_HATCH || GET_OBJ_TYPE(o) == ITEM_PORTAL;
    };

    if (auto obj = ch->location.searchObjects(leave_obj))
    {
        perform_leave_obj(ch, obj, 0);
        return;
    }

    if (OUTSIDE(ch))
    {
        ch->sendText("You are outside.. where do you want to go?\r\n");
        return;
    }

    for (auto &[door, ex] : ch->location.getExits())
    {
        if (!ex.exit_flags[EX_CLOSED] && !ex.getRoomFlag(ROOM_INDOORS))
        {
            perform_move(ch, static_cast<int>(door), 1);
            return;
        }
    }

    ch->sendText("I see no obvious exits to the outside.\r\n");
}

static void handle_fall(Character *ch)
{
    int room = -1;
    std::optional<Destination> ex;
    ex = EXIT(ch, 5);
    while (ex && ch->location.getTileType() == SECT_FLYING)
    {
        ch->leaveLocation();
        ch->moveToLocation(*ex);
        if (auto carrying = CARRYING(ch))
        {
            carrying->leaveLocation();
            carrying->moveToLocation(*ex);
        }
        if (!EXIT(ch, 5) || ch->location.getTileType() != SECT_FLYING)
        {
            act("@r$n slams into the ground!@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::health, -(ch->getEffectiveStat<int64_t>("health") / 20));

            act("@rYou slam into the ground!@n", true, ch, nullptr, nullptr, TO_CHAR);
            ch->lookAtLocation();
        }
        else
        {
            act("@r$n pummets down toward the ground below!@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
    }
    if (ch->location.getTileType() == SECT_WATER_NOSWIM && !CARRIED_BY(ch) && !IS_KANASSAN(ch))
    {
        if ((ch->getCurVital(CharVital::stamina)) >= (ch->getEffectiveStat("weight_carried")))
        {
            act("@bYou swim in place.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@b swims in place.@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::stamina, -ch->getEffectiveStat("weight_carried"));
            act("@RYou are drowning!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@b gulps water as $e struggles to stay above the water line.@n", true, ch, nullptr, nullptr,
                TO_ROOM);
            if (GET_HIT(ch) - ((ch->getEffectiveStat<int64_t>("health")) / 3) <= 0)
            {
                act("@rYou drown!@n", true, ch, nullptr, nullptr, TO_CHAR);
                act("@R$n@r drowns!@n", true, ch, nullptr, nullptr, TO_ROOM);
                die(ch, nullptr);
                ch->modCurVitalDam(CharVital::health, 1);
            }
            else
            {
                ch->modCurVitalDam(CharVital::health, .33);
            }
        }
    }
}

static int check_swim(Character *ch)
{
    auto can = false;

    if (ch->location.getWhereFlag(WhereFlag::space))
    {
        auto space_cost = (GET_MAX_MANA(ch) / 1000) + ((ch->getEffectiveStat("weight_carried")) / 2);
        can = ch->getCurVital(CharVital::ki) >= space_cost;
        ch->modCurVitalDam(CharVital::ki, -space_cost);
        if (!can)
            ch->sendText("You do not have enough ki to fly through space. You are drifting helplessly.\r\n");
        return can;
    }

    auto swim_cost = (ch->getEffectiveStat("weight_carried")) - 1;
    can = ch->getCurVital(CharVital::stamina) >= swim_cost;
    ch->modCurVital(CharVital::stamina, -swim_cost);
    if (!can)
        ch->sendText("You are too tired to swim!\r\n");
    return can;
}

static void handle_fly_space(Character *ch)
{
    if (!OUTSIDE(ch))
    {
        ch->sendText("You are not outside!");
        return;
    }

    if (ch->getCurVitalDam(CharVital::ki) > 0.9 && !IS_ANDROID(ch))
    {
        ch->sendText("You do not have the ki to fly to space.");
        return;
    }

    if (FIGHTING(ch))
    {
        ch->sendText("You are too busy fighting!");
        return;
    }

    auto dest = ch->location.getLaunchDestination();
    if (!dest)
    {
        ch->sendText("You can't fly to space from here!");
        return;
    }
    auto z = dest.getZone();

    reveal_hiding(ch, 0);
    ch->setBaseStat<int>("altitude", 2);
    ch->affect_flags.set(AFF_FLYING, true);
    if (!block_calc(ch))
    {
        return;
    }
    ch->setBaseStat<int>("altitude", 0);
    ch->affect_flags.set(AFF_FLYING, false);

    z->actToOutside(ch, "@C$n can be seen blasting off into space!@n", true);
    z->sendToSense(ch, "leaving the planet", true);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);

    act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n",
        true, ch, nullptr, nullptr, TO_CHAR);
    act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n",
        true, ch, nullptr, nullptr, TO_ROOM);
    ch->leaveLocation();
    ch->moveToLocation(dest);
    act("@C$n blasts up from below and then comes to a stop.@n", true, ch, nullptr, nullptr,
            TO_ROOM);

    auto landing = z->getLandingSpots();
    if(!landing.empty()) 
        ch->sendText("@mOOC: Use the command 'land' to see where you can land from here.@n\r\n");

    if (!IS_ANDROID(ch))
    {
        ch->modCurVitalDam(CharVital::ki, 0.1);
    }
    WAIT_STATE(ch, PULSE_3SEC);
    return;
}

ACMD(do_fly)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    // Common checks for flying restrictions
    auto cannot_fly = [](Character *ch)
    {
        if (ABSORBING(ch) || ABSORBBY(ch))
        {
            ch->sendText("You can't fly, you are struggling with someone right now!\r\n");
            return true;
        }
        if (GRAPPLING(ch) || GRAPPLED(ch))
        {
            ch->sendText("You can't fly, you are struggling with someone right now!\r\n");
            return true;
        }
        if (!IS_NPC(ch))
        {
            if (PLR_FLAGGED(ch, PLR_HEALT))
            {
                ch->sendText("You are inside a healing tank!\r\n");
                return true;
            }
            if (PLR_FLAGGED(ch, PLR_PILOTING))
            {
                ch->sendText("You are busy piloting a ship!\r\n");
                return true;
            }
            if (GET_SKILL(ch, SKILL_FOCUS) < 30 && !IS_ANDROID(ch))
            {
                ch->sendText("You do not have enough focus to hold yourself aloft.\r\n");
                ch->sendText("@wOOC@D: @WYou need the skill Focus at @m30@W.@n\r\n");
                return true;
            }
        }

        return false;
    };

    if (cannot_fly(ch))
        return;

    // Handle specific flight commands
    if (boost::iequals("space", arg))
    {
        handle_fly_space(ch);
        return;
    }

    auto set_flying = [&](int alt)
    {
        reveal_hiding(ch, 0);
        ch->setBaseStat<int>("altitude", alt);
        ch->affect_flags.set(AFF_FLYING, true);
        if (auto chair = SITS(ch); chair)
        {
            chair->sitting.reset();
            ch->sits.reset();
        }
        ch->position = POS_STANDING;
        if (!IS_ANDROID(ch))
            ch->modCurVitalDam(CharVital::ki, 0.01);
    };

    auto ki_check = [ch](const char *msg)
    {
        if (!IS_ANDROID(ch) && ch->getCurVitalDam(CharVital::ki) < 0.01)
        {
            ch->send_to("You do not have the ki to %s.\r\n", msg);
            return false;
        }
        return true;
    };

    if (boost::iequals("high", arg))
    {
        if (!ki_check("fly high"))
            return;
        reveal_hiding(ch, 0);
        act("@WYou rocket high into the sky.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n rockets high into the sky.@n", true, ch, nullptr, nullptr, TO_ROOM);
        set_flying(2);
        return;
    }

    if (*arg)
    {
        ch->sendText("Fly where?\r\n");
        return;
    }

    // Handle landing and taking off based on current status and environment
    auto tile = ch->location.getTileType();
    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        if (tile != SECT_FLYING && tile != SECT_SPACE)
        {
            act("@WYou slowly settle down to the ground.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n slowly settles down to the ground.@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
        else if (tile == SECT_FLYING)
        {
            act("@WYou begin to plummet to the ground!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n starts to plummet to the ground below!@n", true, ch, nullptr, nullptr, TO_ROOM);
            handle_fall(ch);
        }
        else if (tile == SECT_SPACE)
        {
            act("@WYou let yourself drift aimlessly through space.@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n starts to drift slowly!@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
        ch->affect_flags.set(AFF_FLYING, false);
        ch->setBaseStat<int>("altitude", 0);
        return;
    }

    if (!ki_check("fly"))
        return;

    // Taking off if not already flying
    reveal_hiding(ch, 0);
    act("@WYou slowly take off into the sky.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act("@W$n slowly takes off into the sky.@n", true, ch, nullptr, nullptr, TO_ROOM);
    set_flying(1);
}

static void autochair(Character *ch, Object *chair)
{
    // TODO: Make this configurable.
    chair->sitting.reset();
    ch->sits.reset();
    if (CAN_WEAR(chair, ITEM_WEAR_TAKE) && GET_OBJ_TYPE(chair) != ITEM_CHAIR && ch->canCarryWeight(chair))
    {
        chair->clearLocation();
        ch->addToInventory(chair);
        act("You pick up $p.", true, ch, chair, nullptr, TO_CHAR);
        act("$n picks up $p.", true, ch, chair, nullptr, TO_ROOM);
    }
}

ACMD(do_stand)
{
    auto chair = SITS(ch);
    if (AFF_FLAGGED(ch, AFF_KNOCKED))
    {
        ch->sendText("You are knocked out cold for right now!\r\n");
        return;
    }
    if (!IS_NPC(ch) && GET_LIMBCOND(ch, 2) <= 0 && GET_LIMBCOND(ch, 3) <= 0)
    {
        ch->sendText("With what legs will you be standing up on?\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_PILOTING))
    {
        ch->sendText("You are busy piloting a ship!\r\n");
        return;
    }

    switch (GET_POS(ch))
    {
    case POS_STANDING:
        ch->sendText("You are already standing.\r\n");
        break;
    case POS_SITTING:
        reveal_hiding(ch, 0);
        ch->sendText("You stand up.\r\n");
        act("$n clambers to $s feet.", true, ch, nullptr, nullptr, TO_ROOM);
        if (chair)
            autochair(ch, chair);
        /* May be sitting for some reason and may still be fighting. */
        ch->position = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
        break;
    case POS_RESTING:
        ch->sendText("You stop resting, and stand up.\r\n");
        act("$n stops resting, and clambers to $s feet.", true, ch, nullptr, nullptr, TO_ROOM);
        if (chair)
            autochair(ch, chair);
        ch->position = POS_STANDING;
        break;
    case POS_SLEEPING:
        ch->sendText("You have to wake up first!\r\n");
        break;
    default:
        ch->sendText("You stop floating around, and put your feet on the ground.\r\n");
        act("$n stops floating around, and puts $s feet on the ground.",
            true, ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_STANDING;
        break;
    }
}

ACMD(do_sit)
{
    Object *chair = nullptr;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (PLR_FLAGGED(ch, PLR_PILOTING))
    {
        ch->sendText("You are busy piloting a ship!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }

    if (DRAGGING(ch))
    {
        act("@WYou stop dragging @C$N@W!@n", true, ch, nullptr, DRAGGING(ch), TO_CHAR);
        act("@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, DRAGGING(ch), TO_ROOM);
        DRAGGED(DRAGGING(ch)) = nullptr;
        DRAGGING(ch) = nullptr;
    }
    if (CARRYING(ch))
    {
        ch->sendText("You are busy carrying someone!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        do_fly(ch, nullptr, 0, 0);
    }

    if (!*arg)
    {
        switch (GET_POS(ch))
        {
        case POS_STANDING:
            reveal_hiding(ch, 0);
            ch->sendText("You sit down.\r\n");
            act("$n sits down.", false, ch, nullptr, nullptr, TO_ROOM);
            ch->position = POS_SITTING;
            break;
        case POS_SITTING:
            ch->sendText("You're sitting already.\r\n");
            break;
        case POS_RESTING:
            ch->sendText("You stop resting, and sit up.\r\n");
            act("$n stops resting.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->position = POS_SITTING;
            break;
        case POS_SLEEPING:
            ch->sendText("You have to wake up first.\r\n");
            break;
        case POS_FIGHTING:
            ch->sendText("Sit down while fighting? Are you MAD?\r\n");
            break;
        default:
            ch->sendText("You stop floating around, and sit down.\r\n");
            act("$n stops floating around, and sits down.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->position = POS_SITTING;
            break;
        }
        return;
    }

    if (SITS(ch))
    {
        ch->sendText("You are already on something!\r\n");
        return;
    }
    if (!(chair = get_obj_in_list_vis(ch, arg, nullptr, ch->location.getObjects())))
    {
        ch->sendText("That isn't here.\r\n");
        return;
    }
    if (GET_OBJ_VNUM(chair) == 65)
    {
        ch->sendText("You can't get on that!\r\n");
        return;
    }
    if (SITTING(chair))
    {
        ch->sendText("Someone is already on that one!\r\n");
        return;
    }
    if (GET_OBJ_TYPE(chair) != ITEM_CHAIR && GET_OBJ_TYPE(chair) != ITEM_BED)
    {
        ch->sendText("You can't sit on that!\r\n");
        return;
    }
    if (GET_OBJ_SIZE(chair) + 1 < get_size(ch))
    {
        ch->sendText("You are too large for it!\r\n");
        return;
    }
    switch (GET_POS(ch))
    {
    case POS_STANDING:
        reveal_hiding(ch, 0);
        act("You sit down on $p.", false, ch, chair, nullptr, TO_CHAR);
        act("$n sits down on $p.", false, ch, chair, nullptr, TO_ROOM);
        ch->position = POS_SITTING;
        ch->sits = chair->shared_from_this();
        chair->sitting = ch->shared_from_this();
        break;
    case POS_SITTING:
        ch->sendText("You should stand up first.\r\n");
        break;
    case POS_RESTING:
        ch->sendText("You should stand up first.\r\n");
        break;
    case POS_SLEEPING:
        ch->sendText("You have to wake up first.\r\n");
        break;
    case POS_FIGHTING:
        ch->sendText("Sit down while fighting? Are you MAD?\r\n");
        break;
    default:
        ch->sendText("You stop floating around, and sit down.\r\n");
        act("$n stops floating around, and sits down.", true, ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_SITTING;
        break;
    }
}

ACMD(do_rest)
{
    Object *chair = nullptr;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (PLR_FLAGGED(ch, PLR_PILOTING))
    {
        ch->sendText("You are busy piloting a ship!\r\n");
        return;
    }
    if (ch->location.getTileType() == SECT_WATER_NOSWIM)
    {
        ch->sendText("You can't rest here!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_SANCTUARY))
    {
        if (GET_SKILL(ch, SKILL_BARRIER))
        {
            ch->sendText("You have a barrier around you and can't rest.\r\n");
            return;
        }
        else
        {
            ch->setBaseStat<int64_t>("barrier", 0);
            ch->affect_flags.set(AFF_SANCTUARY, false);
        }
    }
    if (FIGHTING(ch))
    {
        ch->sendText("You are a bit busy at the moment!\r\n");
        return;
    }
    if (GET_KAIOKEN(ch) > 0)
    {
        ch->sendText("You are utilizing kaioken and can't rest!\r\n");
        return;
    }

    if (auto drg = DRAGGING(ch))
    {
        act("@WYou stop dragging @C$N@W!@n", true, ch, nullptr, drg, TO_CHAR);
        act("@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, drg, TO_ROOM);
        DRAGGED(drg) = nullptr;
        DRAGGING(ch) = nullptr;
    }

    if (CARRYING(ch))
    {
        ch->sendText("You are carrying someone!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        do_fly(ch, nullptr, 0, 0);
    }

    if (!*arg)
    {
        if (auto chair = SITS(ch))
        {
            if (GET_OBJ_TYPE(chair) != ITEM_BED)
            {
                ch->sendText("You can't lay on that!\r\n");
                return;
            }
        }
        switch (GET_POS(ch))
        {
        case POS_STANDING:
            reveal_hiding(ch, 0);
            ch->sendText("You lay down and rest your tired bones.\r\n");
            act("$n lays down and rests.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->position = POS_RESTING;
            break;
        case POS_SITTING:
            ch->sendText("You rest your tired bones.\r\n");
            act("$n rests.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->position = POS_RESTING;
            break;
        case POS_RESTING:
            ch->sendText("You are already resting.\r\n");
            break;
        case POS_SLEEPING:
            ch->sendText("You have to wake up first.\r\n");
            break;
        case POS_FIGHTING:
            ch->sendText("Rest while fighting?  Are you MAD?\r\n");
            break;
        default:
            ch->sendText("You stop floating around, and stop to rest your tired bones.\r\n");
            act("$n stops floating around, and rests.", false, ch, nullptr, nullptr, TO_ROOM);
            ch->position = POS_RESTING;
            break;
        }
        return;
    }
    if (SITS(ch))
    {
        ch->sendText("You are already on something!\r\n");
        return;
    }
    if (!(chair = get_obj_in_list_vis(ch, arg, nullptr, ch->location.getObjects())))
    {
        ch->sendText("That isn't here.\r\n");
        return;
    }
    if (GET_OBJ_VNUM(chair) == 65)
    {
        ch->sendText("You can't get on that!\r\n");
        return;
    }
    if (SITTING(chair))
    {
        ch->sendText("Someone is already on that one!\r\n");
        return;
    }
    if (GET_OBJ_TYPE(chair) != ITEM_BED)
    {
        ch->sendText("You can't lay on that!\r\n");
        return;
    }
    if (GET_OBJ_SIZE(chair) + 1 < get_size(ch))
    {
        ch->sendText("You are too large for it!\r\n");
        return;
    }
    switch (GET_POS(ch))
    {
    case POS_STANDING:
        reveal_hiding(ch, 0);
        act("You lay down and rest on $p.", true, ch, chair, nullptr, TO_CHAR);
        act("$n lays down and rests on $p.", true, ch, chair, nullptr, TO_ROOM);
        ch->sits = chair->shared_from_this();
        chair->sitting = ch->shared_from_this();
        ch->position = POS_RESTING;
        ch->removeLimitBreak();
        break;
    case POS_SITTING:
        ch->sendText("You should get up first.\r\n");
        break;
    case POS_RESTING:
        ch->sendText("You are already resting.\r\n");
        break;
    case POS_SLEEPING:
        ch->sendText("You have to wake up first.\r\n");
        break;
    case POS_FIGHTING:
        ch->sendText("Rest while fighting?  Are you MAD?\r\n");
        break;
    default:
        ch->sendText("You stop floating around, and stop to rest your tired bones.\r\n");
        act("$n stops floating around, and rests.", false, ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_RESTING;
        ch->removeLimitBreak();
        break;
    }
}

ACMD(do_sleep)
{
    Object *chair = nullptr;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (!IS_NPC(ch))
    {
        if (PRF_FLAGGED(ch, PRF_ARENAWATCH))
        {
            ch->pref_flags.set(PRF_ARENAWATCH, false);
            ch->setBaseStat<room_vnum>("arena_watch", -1);
            ch->sendText("You stop watching the arena action.\r\n");
        }
    }

    if (GET_BONUS(ch, BONUS_INSOMNIAC))
    {
        ch->sendText("You don't feel the least bit tired.\r\n");
        return;
    }

    if (ch->location.getTileType() == SECT_WATER_NOSWIM)
    {
        ch->sendText("You can't rest here!\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_PILOTING))
    {
        ch->sendText("You are busy piloting a ship!\r\n");
        return;
    }
    if (FIGHTING(ch))
    {
        ch->sendText("You are a bit busy at the moment!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }
    if (ch->character_flags.get(CharacterFlag::powering_up))
    {
        ch->sendText("You are busy powering up!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_SANCTUARY))
    {
        if (GET_SKILL(ch, SKILL_BARRIER) > 0)
        {
            ch->sendText("You have a barrier around you and can't sleep.\r\n");
            return;
        }
        else
        {
            ch->setBaseStat<int64_t>("barrier", 0);
            ch->affect_flags.set(AFF_SANCTUARY, false);
        }
    }
    if (GET_KAIOKEN(ch) > 0)
    {
        ch->sendText("You are utilizing kaioken and can't sleep!\r\n");
        return;
    }
    if (GET_SLEEPT(ch) > 0)
    {
        ch->sendText("You aren't sleepy enough.\r\n");
        return;
    }
    if (auto drg = DRAGGING(ch))
    {
        act("@WYou stop dragging @C$N@W!@n", true, ch, nullptr, drg, TO_CHAR);
        act("@C$n@W stops dragging @c$N@W!@n", true, ch, nullptr, drg, TO_ROOM);
        DRAGGED(drg) = nullptr;
        DRAGGING(ch) = nullptr;
    }
    if (CARRYING(ch))
    {
        ch->sendText("You are carrying someone!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        do_fly(ch, nullptr, 0, 0);
    }

    if (!*arg)
    {
        if (SITS(ch))
        {
            chair = SITS(ch);
            if (GET_OBJ_TYPE(chair) != ITEM_BED)
            {
                ch->send_to("You can't sleep on %s.\r\n", chair->getShortDescription());
                return;
            }
        }
        switch (GET_POS(ch))
        {
        case POS_STANDING:
        case POS_SITTING:
        case POS_RESTING:
            reveal_hiding(ch, 0);
            ch->sendText("You go to sleep.\r\n");
            act("$n lies down and falls asleep.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->position = POS_SLEEPING;
            ch->removeLimitBreak();
            /* Fury Mode Loss for halfbreeds */

            if (PLR_FLAGGED(ch, PLR_FURY))
            {
                ch->sendText("Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
                ch->player_flags.set(PLR_FURY, false);
            }

            /* Fury Mode Loss for halfbreeds */

            if (GET_STUPIDKISS(ch) > 0)
            {
                ch->setBaseStat("stupidkiss", 0);
                ch->sendText("You forget about that stupid kiss.\r\n");
            }
            break;
        case POS_SLEEPING:
            ch->sendText("You are already sound asleep.\r\n");
            break;
        case POS_FIGHTING:
            ch->sendText("Sleep while fighting?  Are you MAD?\r\n");
            break;
        default:
            ch->sendText("You stop floating around, and lie down to sleep.\r\n");
            act("$n stops floating around, and lie down to sleep.",
                true, ch, nullptr, nullptr, TO_ROOM);
            ch->position = POS_SLEEPING;
            ch->removeLimitBreak();
            break;
        }
        return;
    }
    if (SITS(ch))
    {
        ch->sendText("You are already on something!\r\n");
        return;
    }
    if (!(chair = get_obj_in_list_vis(ch, arg, nullptr, ch->location.getObjects())))
    {
        ch->sendText("That isn't here.\r\n");
        return;
    }
    if (GET_OBJ_VNUM(chair) == 65)
    {
        ch->sendText("You can't get on that!\r\n");
        return;
    }
    if (SITTING(chair))
    {
        ch->sendText("Someone is already on that one!\r\n");
        return;
    }
    if (GET_OBJ_TYPE(chair) != ITEM_BED)
    {
        ch->sendText("You can't sleep on that!\r\n");
        return;
    }
    if (GET_OBJ_SIZE(chair) + 1 < get_size(ch))
    {
        ch->sendText("You are too large for it!\r\n");
        return;
    }
    switch (GET_POS(ch))
    {
    case POS_RESTING:
    case POS_SITTING:
        ch->sendText("You need to get up first!\r\n");
        break;
    case POS_STANDING:
        reveal_hiding(ch, 0);
        act("You lay down on $p and sleep.", false, ch, chair, nullptr, TO_CHAR);
        act("$n lays down on $p and sleeps.", false, ch, chair, nullptr, TO_ROOM);
        ch->removeLimitBreak();
        /* Fury Mode Loss for halfbreeds */

        if (PLR_FLAGGED(ch, PLR_FURY))
        {
            ch->sendText("Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
            ch->player_flags.set(PLR_FURY, false);
        }

        /* Fury Mode Loss for halfbreeds */

        if (GET_STUPIDKISS(ch) > 0)
        {
            ch->setBaseStat("stupidkiss", 0);
            ch->sendText("You forget about that stupid kiss.\r\n");
        }
        ch->sits = chair->shared_from_this();
        chair->sitting = ch->shared_from_this();
        ch->position = POS_SLEEPING;
        break;
    case POS_SLEEPING:
        ch->sendText("You are already sound asleep.\r\n");
        break;
    case POS_FIGHTING:
        ch->sendText("Sleep while fighting?  Are you MAD?\r\n");
        break;
    default:
        ch->sendText("You stop floating around, and lie down to sleep.\r\n");
        act("$n stops floating around, and lie down to sleep.",
            true, ch, nullptr, nullptr, TO_ROOM);
        ch->position = POS_SLEEPING;
        ch->removeLimitBreak();
        break;
    }
}

ACMD(do_wake)
{
    char arg[MAX_INPUT_LENGTH];
    Character *vict;
    int self = 0;

    one_argument(argument, arg);

    if (AFF_FLAGGED(ch, AFF_KNOCKED))
    {
        ch->sendText("You are knocked out cold for right now!\r\n");
        return;
    }

    if (GET_BONUS(ch, BONUS_LATE) && GET_POS(ch) == POS_SLEEPING)
    {
        ch->sendText("Nah you're enjoying sleeping too much.\r\n");
        return;
    }

    if (*arg)
    {
        if (GET_POS(ch) == POS_SLEEPING)
            ch->sendText("Maybe you should wake yourself up first.\r\n");
        else if ((vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)) == nullptr)
            ch->send_to("%s", CONFIG_NOPERSON);
        else if (vict == ch)
            self = 1;
        else if (AWAKE(vict))
            act("$E is already awake.", false, ch, nullptr, vict, TO_CHAR);
        else if (AFF_FLAGGED(vict, AFF_SLEEP))
            act("You can't wake $M up!", false, ch, nullptr, vict, TO_CHAR);
        else if (GET_POS(vict) < POS_SLEEPING)
            act("$E's in pretty bad shape!", false, ch, nullptr, vict, TO_CHAR);
        else if (AFF_FLAGGED(vict, AFF_KNOCKED))
            ch->sendText("They are knocked out cold for right now!\r\n");
        else if (GET_BONUS(ch, BONUS_LATE))
            ch->sendText("They say 'Yeah yeah...' and then roll back over.\r\n");
        else
        {
            act("You wake $M up.", false, ch, nullptr, vict, TO_CHAR);
            act("You are awakened by $n.", false, ch, nullptr, vict, TO_VICT | TO_SLEEP);
            vict->position = POS_SITTING;
            if (auto drg = DRAGGED(vict))
            {
                act("@WYou stop dragging @C$N@W!@n", true, drg, nullptr, vict, TO_CHAR);
                act("@C$n@W stops dragging @c$N@W!@n", true, drg, nullptr, vict, TO_ROOM);
                DRAGGING(drg) = nullptr;
                DRAGGED(vict) = nullptr;
            }
            if (auto carry = CARRIED_BY(vict))
            {
                if (GET_ALIGNMENT(carry) > 50)
                {
                    carry_drop(carry, 0);
                }
                else
                {
                    carry_drop(carry, 1);
                }
            }
        }
        if (!self)
            return;
    }
    if (AFF_FLAGGED(ch, AFF_SLEEP))
        ch->sendText("You can't wake up!\r\n");
    else if (GET_POS(ch) > POS_SLEEPING)
        ch->sendText("You are already awake...\r\n");
    else
    {
        ch->sendText("You awaken, and sit up.\r\n");
        act("$n awakens.", true, ch, nullptr, nullptr, TO_ROOM);
        if (auto drg = DRAGGED(ch))
        {
            act("@WYou stop dragging @C$N@W!@n", true, drg, nullptr, ch, TO_CHAR);
            act("@C$n@W stops dragging you!@n", true, drg, nullptr, ch, TO_VICT);
            act("@C$n@W stops dragging @c$N@W!@n", true, drg, nullptr, ch, TO_NOTVICT);
            DRAGGING(drg) = nullptr;
            DRAGGED(ch) = nullptr;
        }
        if (auto carry = CARRIED_BY(ch))
        {
            if (GET_ALIGNMENT(carry) > 50)
            {
                carry_drop(carry, 0);
            }
            else
            {
                carry_drop(carry, 1);
            }
        }
        ch->position = POS_SITTING;
    }
}

ACMD(do_follow)
{
    char buf[MAX_INPUT_LENGTH];
    Character *leader;

    one_argument(argument, buf);

    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }

    if (*buf)
    {
        if (!(leader = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM)))
        {
            ch->send_to("%s", CONFIG_NOPERSON);
            return;
        }
    }
    else
    {
        ch->sendText("Whom do you wish to follow?\r\n");
        return;
    }

    if (ch->master == leader)
    {
        act("You are already following $M.", false, ch, nullptr, leader, TO_CHAR);
        return;
    }
    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master))
    {
        act("But you only feel like following $N!", false, ch, nullptr, ch->master, TO_CHAR);
        return;
    }

    /* Not Charmed follow person */
    if (leader == ch)
    {
        if (!ch->master)
        {
            ch->sendText("You are already following yourself.\r\n");
            return;
        }
        stop_follower(ch);
        return;
    }
    if (circle_follow(ch, leader))
    {
        ch->sendText("Sorry, but following in loops is not allowed.\r\n");
        return;
    }
    if (ch->master)
        stop_follower(ch);
    ch->affect_flags.set(AFF_GROUP, false);
    reveal_hiding(ch, 0);
    add_follower(ch, leader);
}
