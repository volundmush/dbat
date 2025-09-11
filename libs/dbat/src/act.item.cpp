/* ************************************************************************
 *   File: act.item.c                                    Part of CircleMUD *
 *  Usage: object handling routines -- get/drop and container handling     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "dbat/CharacterUtils.h"
#include "dbat/ObjectUtils.h"
#include "dbat/RoomUtils.h"
#include "dbat/Destination.h"
#include "dbat/Descriptor.h"
#include "dbat/act.item.h"
#include "dbat/vehicles.h"
#include "dbat/dg_comm.h"
#include "dbat/act.wizard.h"
#include "dbat/act.other.h"
#include "dbat/act.comm.h"
#include "dbat/act.informative.h"
#include "dbat/config.h"
#include "dbat/assemblies.h"
#include "dbat/send.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/class.h"
#include "dbat/feats.h"
#include "dbat/Guild.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"
#include "dbat/boards.h"
#include "dbat/ansi.h"
#include "dbat/utils.h"
#include "dbat/filter.h"

#include "dbat/players.h"

#include "dbat/Random.h"

#include "dbat/DragonBall.h"

#include "dbat/CharacterPrototype.h"

#include "dbat/HasExtraDescriptions.h"

#include "dbat/const/AuctionState.h"
#include "dbat/const/Pulse.h"
#include "dbat/const/WearSlot.h"
#include "dbat/const/ItemValues.h"
#include "dbat/const/ContainerFlag.h"
#include "dbat/const/Condition.h"
#include "dbat/const/Recipe.h"
#include "dbat/const/Environment.h"


/* global variables */
Object *obj_selling = nullptr;   /* current object for sale */
Character *ch_selling = nullptr; /* current character selling obj */
Character *ch_buying = nullptr;  /* current character buying the object */

/* local vvariables  */
static int curbid = 0;               /* current bid on item being auctioned */
static int aucstat = AUC_NULL_STATE; /* state of auction.. first_bid etc.. */

static const char *auctioneer[AUC_BID + 1] = {

    "@D[@CAUCTION@c: @C$n@W puts $p@W up for sale at @Y%d@W zenni.@D]@n",
    "@D[@CAUCTION@c: @W$p@W at @Y%d@W zenni going once!@D]@n",
    "@D[@CAUCTION@c: @W$p@W at @Y%d@W zenni going twice!@D]@n",
    "@D[@CAUCTION@c: @WLast call: $p@W going for @Y%d@W zenni.@D]@n",
    "@D[@CAUCTION@c: @WUnfortunately $p@W is unsold, returning it to $n. @D]@n",
    "@D[@CAUCTION@c: @WSOLD! $p@W to @C$n@W for @Y%d@W zenni!@D]@n",
    "@D[@CAUCTION@c: @WSorry, @C$n@W has cancelled the auction.@D]@n",
    "@D[@CAUCTION@c: @WSorry, @C$n@W has left us, the auction can't go on.@D]@n",
    "@D[@CAUCTION@c: @WSorry, $p@W has been confiscated, shame on you $n.@D]@n",
    "@D[@CAUCTION@c: @C$n@W is selling $p@W for @Y%d@W zenni.@D]@n",
    "@D[@CAUCTION@c: @C$n@W bids @Y%d@W zenni on $p@W.@D]@n"};

/* local functions */
static void majin_gain(Character *ch, Object *food, int foob);

static int can_take_obj(Character *ch, Object *obj);

static void get_check_money(Character *ch, Object *obj);

static void get_from_room(Character *ch, char *arg, int howmany);

static void perform_give_gold(Character *ch, Character *vict, int amount);

static void perform_give(Character *ch, Character *vict, Object *obj);

static int perform_drop(Character *ch, Object *obj, int8_t mode, const char *sname, room_rnum RDR);

static void perform_drop_gold(Character *ch, int amount, int8_t mode, room_rnum RDR);

static Character *give_find_vict(Character *ch, char *arg);

static void perform_put(Character *ch, Object *obj, Object *cont);

static void get_from_container(Character *ch, Object *cont, char *arg, int mode, int howmany);

static void wear_message(Character *ch, Object *obj, int where);

static void perform_get_from_container(Character *ch, Object *obj, Object *cont, int mode);

static int hands(Character *ch);

static void start_auction(Character *ch, Object *obj, int bid);

static void auc_stat(Character *ch, Object *obj);

static void auc_send_to_all(const char *messg, bool buyer);

static bool has_housekey(Character *ch, Object *obj);

static void harvest_plant(Character *ch, Object *plant);

static int can_harvest(Object *plant);

/* local variables */
static char buf[MAX_STRING_LENGTH];

// definitions

ACMD(do_refuel)
{

    Object *controls;

    if (!(controls = find_control(ch)))
    {
        ch->sendText("@wYou need to be in the cockpit to place a new fuel canister into the ship.\r\n");
        return;
    }

    Object *rep = nullptr, *next_obj = nullptr, *fuel = nullptr;
    fuel = ch->searchInventory(17290);

    if (!fuel)
    {
        ch->sendText("You do not have any fuel canisters on you.\r\n");
        return;
    }

    int max = 0;

    if (GET_OBJ_VNUM(controls) >= 44000 && GET_OBJ_VNUM(controls) <= 44199)
    {
        max = 300;
    }
    else if (GET_OBJ_VNUM(controls) >= 44200 && GET_OBJ_VNUM(controls) <= 44499)
    {
        max = 500;
    }
    else if (GET_OBJ_VNUM(controls) >= 44200 && GET_OBJ_VNUM(controls) <= 44999)
    {
        max = 1000;
    }

    if (GET_FUEL(controls) == max)
    {
        ch->sendText("The ship is full on fuel!\r\n");
        return;
    }
    else
    {

        if (GET_FUEL(controls) + (GET_OBJ_WEIGHT(fuel) * 4) > max)
        {
            MOD_OBJ_VAL(controls, VAL_VEHICLE_FUEL, max);
        }
        else
        {
            MOD_OBJ_VAL(controls, VAL_VEHICLE_FUEL, (GET_OBJ_WEIGHT(fuel) * 4));
        }

        extract_obj(fuel);

        ch->sendText("You place the fuel canister into the ship. Within seconds the fuel has been extracted from the canister into the ships' internal tanks.\r\n");
    }
}

static int can_harvest(Object *plant)
{

    switch (GET_OBJ_VNUM(plant))
    {
    case 250:
    case 1129:
    case 17210:
    case 17211:
    case 17214:
    case 17216:
    case 17218:
    case 17220:
    case 17222:
    case 17224:
    case 17226:
    case 3702:
        return true;
    }

    return false;
}

static void harvest_plant(Character *ch, Object *plant)
{
    int extract = false, reward = Random::get<int>(5, 15), count = reward;
    Object *fruit = nullptr;

    if (GET_OBJ_VAL(plant, VAL_PLANT_SOILQUALITY) > 7)
    {
        reward += 10;
        ch->sendText("@GThe soil seems to have made the plant exteremely bountiful");
    }
    else if (GET_OBJ_VAL(plant, VAL_PLANT_SOILQUALITY) >= 5)
    {
        reward += 6;
        ch->sendText("@GThe soil seems to have made the plant very bountiful");
    }
    else if (GET_OBJ_VAL(plant, VAL_PLANT_SOILQUALITY) >= 3)
    {
        reward += 4;
        ch->sendText("@GThe soil seems to have made the plant bountiful");
    }
    else if (GET_OBJ_VAL(plant, VAL_PLANT_SOILQUALITY) > 0)
    {
        reward += 2;
        ch->sendText("@GThe soil seems to have made the plant a bit more bountiful");
    }

    int skill = GET_SKILL(ch, SKILL_GARDENING);

    if (skill >= 100)
    {
        reward += 10;
        ch->sendText(" and your outstanding skill has helped the plant be more bountiful yet!@n\r\n");
    }
    else if (skill >= 90)
    {
        reward += 8;
        ch->sendText(" and your great skill has also helped the plant be more bountiful yet!@n\r\n");
    }
    else if (skill >= 80)
    {
        reward += 5;
        ch->sendText(" and your good skill has also helped the plant be more bountiful yet!@n\r\n");
    }
    else if (skill >= 50)
    {
        reward += 3;
        ch->sendText(" and your decent skill has also helped the plant be more bountiful yet!@n\r\n");
    }
    else if (skill >= 40)
    {
        reward += 2;
        ch->sendText(" and your mastery of the basics of gardening has also helped the plant be more bountiful yet!@n\r\n");
    }
    else if (skill >= 30)
    {
        reward += 1;
        ch->sendText(" and you somehow managed to make the plant slightly more bountiful with what little you know!@n\r\n");
    }
    else
    {
        ch->sendText(".@n\r\n");
    }

    count = reward;

    switch (GET_OBJ_VNUM(plant))
    {
    case 250:
        if (reward > 2)
        {
            reward = 2;
            count = 2;
        }
        while (count > 0)
        {
            fruit = read_object(1, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = false;
        break;
    case 1129:
        while (count > 0)
        {
            fruit = read_object(1131, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 17210:
        reward += 2;
        count += 2;
        while (count > 0)
        {
            fruit = read_object(17212, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 17211:
        reward += 2;
        count += 2;
        while (count > 0)
        {
            fruit = read_object(17213, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 17214:
        reward += 1;
        count += 1;
        while (count > 0)
        {
            fruit = read_object(17215, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 17216:
        while (count > 0)
        {
            fruit = read_object(17217, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 17218:
        reward += 14;
        count += 14;
        while (count > 0)
        {
            fruit = read_object(17219, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 17220:
        reward -= reward * 0.75;
        count = reward;
        while (count > 0)
        {
            fruit = read_object(17221, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 17222:
        reward += Random::get<int>(1, 3);
        count = reward;
        while (count > 0)
        {
            fruit = read_object(17223, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 17224:
        reward += 10;
        count = reward;
        while (count > 0)
        {
            fruit = read_object(17225, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 17226:
        reward -= 8;
        if (reward < 0)
            reward = 1;
        count = reward;
        while (count > 0)
        {
            fruit = read_object(17227, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    case 3702:
        reward -= 2;
        count = reward;
        while (count > 0)
        {
            fruit = read_object(3703, VIRTUAL);
            ch->addToInventory(fruit);
            count -= 1;
        }
        ch->send_to("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n\r\n", reward, fruit->getShortDescription());
        extract = true;
        break;
    default:
        send_to_imm("ERROR: Harvest plant called for illegitimate plant, VNUM %d.", GET_OBJ_VNUM(plant));
        break;
    }

    if (extract == true)
    {
        ch->sendText("@wThe harvesting process has killed the plant. Do not worry, this is normal for that type.@n\r\n");
        extract_obj(plant);
    }
    else
    {
        SET_OBJ_VAL(plant, VAL_PLANT_MATURITY, 3);
        SET_OBJ_VAL(plant, VAL_PLANT_SOILQUALITY, 0);
    }
}

ACMD(do_garden)
{

    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Object *obj;

    two_arguments(argument, arg, arg2);

    if (!GET_SKILL(ch, SKILL_GARDENING) && slot_count(ch) + 1 <= GET_SLOTS(ch))
    {
        int numb = Random::get<int>(8, 16);
        SET_SKILL(ch, SKILL_GARDENING, numb);
        ch->sendText("@GYou learn the very basics of gardening.\r\n");
    }
    else if (!GET_SKILL(ch, SKILL_GARDENING) && slot_count(ch) + 1 > GET_SLOTS(ch))
    {
        ch->sendText("You need additional skill slots to pick up the skill linked with this.\r\n");
        return;
    }

    if (*arg)
    {
        if (boost::iequals(arg, "collect"))
        {
            Object *obj2, *shovel = nullptr, *next_obj;
            int found = false;
            shovel = ch->searchInventory(254);
            if (!shovel)
            {
                ch->sendText("You need a shovel in order to collect soil.\r\n");
                return;
            }
            const auto tile = ch->location.getTileType();
            if (tile != SECT_FOREST && tile != SECT_FIELD &&
                tile != SECT_MOUNTAIN && tile != SECT_HILLS)
            {
                ch->sendText("You can not collect soil from this area.\r\n");
                return;
            }
            if (ch->location.getRoomFlag(ROOM_FERTILE1))
            {
                Object *soil = read_object(255, VIRTUAL);
                ch->addToInventory(soil);
                act("@yYou sink your shovel into the soft ground and manage to dig up a pile of fertile soil!@n", true,
                    ch, nullptr, nullptr, TO_CHAR);
                act("@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of fertile soil!@n", true,
                    ch, nullptr, nullptr, TO_ROOM);
                SET_OBJ_VAL(soil, VAL_OTHER_SOILQUALITY, 8);
                WAIT_STATE(ch, PULSE_4SEC);
                return;
            }
            else if (ch->location.getRoomFlag(ROOM_FERTILE2))
            {
                Object *soil = read_object(255, VIRTUAL);
                ch->addToInventory(soil);
                act("@yYou sink your shovel into the soft ground and manage to dig up a pile of good soil!@n", true, ch,
                    nullptr, nullptr, TO_CHAR);
                act("@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of good soil!@n", true,
                    ch, nullptr, nullptr, TO_ROOM);
                SET_OBJ_VAL(soil, VAL_OTHER_SOILQUALITY, Random::get<int>(5, 7));
                WAIT_STATE(ch, PULSE_4SEC);
                return;
            }
            else
            {
                Object *soil = read_object(255, VIRTUAL);
                ch->addToInventory(soil);
                act("@yYou sink your shovel into the soft ground and manage to dig up a pile of soil!@n", true, ch,
                    nullptr, nullptr, TO_CHAR);
                act("@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of soil!@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                SET_OBJ_VAL(soil, VAL_OTHER_SOILQUALITY, Random::get<int>(0, 4));
                WAIT_STATE(ch, PULSE_4SEC);
                return;
            }
        }
    }

    if (!*arg || !*arg2)
    {
        ch->sendText("Syntax: garden (plant) ( water | harvest | dig | plant | pick )\r\n");
        ch->sendText("Syntax: garden collect [Will collect soil from a room with soil.\r\n");
        return;
    }

    if (!ch->location.getRoomFlag(ROOM_GARDEN1) && !ch->location.getRoomFlag(ROOM_GARDEN2))
    {
        ch->sendText("You are not even in a garden!\r\n");
        return;
    }

    if (boost::iequals(arg2, "plant"))
    {
        if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
        {
            ch->sendText("What are you trying to plant?\r\n");
            ch->sendText("Syntax: garden (plant in inventory) plant\r\n");
            return;
        }
    }
    else
    {
        if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->location.getObjects())))
        {
            ch->sendText("That plant doesn't seem to be here.\r\n");
            return;
        }
    }

    if (!obj)
    {
        ch->sendText("What plant are you gardening?\r\n");
        return;
    }

    int64_t cost = (GET_MAX_MOVE(ch) * 0.005) + Random::get<int>(50, 150);
    int skill = GET_SKILL(ch, SKILL_GARDENING);

    if ((ch->getCurVital(CharVital::stamina)) < cost)
    {
        ch->send_to("@WYou need at least @G%s@W stamina to garden.\r\n", add_commas(cost).c_str());
        return;
    }
    else
    {
        if (boost::iequals(arg2, "water"))
        {
            Object *obj2, *water = nullptr, *next_obj;
            int found = false;
            water = ch->searchInventory(251);
            if (!water)
            {
                ch->sendText("You do not have any grow water!\r\n");
                return;
            }
            else if (GET_OBJ_VAL(obj, VAL_PLANT_WATERLEVEL) >= 500)
            {
                ch->sendText("You stop as you realize that the plant already has enough water.\r\n");
                return;
            }
            else if (GET_OBJ_VAL(obj, VAL_PLANT_WATERLEVEL) <= -10)
            {
                ch->sendText("The plant is dead!\r\n");
                return;
            }
            else if (skill < axion_dice(0))
            {
                act("@GAs you go to water @g$p@G you end up sloppily wasting about half of it on the ground.@n", true,
                    ch, obj, nullptr, TO_CHAR);
                act("@g$n@G takes a bottle of grow water and sloshes some of it on @g$p@G.@n", true, ch, obj, nullptr,
                    TO_ROOM);

                ch->modCurVital(CharVital::stamina, -cost);
                if (MOD_OBJ_VAL(obj, VAL_PLANT_WATERLEVEL, 40) > 500)
                {
                    SET_OBJ_VAL(obj, VAL_PLANT_WATERLEVEL, 500);
                    ch->sendText("@YThe plant is now at full water level.@n\r\n");
                }
                extract_obj(water);
                WAIT_STATE(ch, PULSE_3SEC);
                improve_skill(ch, SKILL_GARDENING, 0);
                return;
            }
            else
            {
                act("@GYou calmly and expertly pour the grow water on @g$p@G.@n", true, ch, obj, nullptr, TO_CHAR);
                act("@g$n@G calmly and expertly pours some grow water on @g$p@G.@n", true, ch, obj, nullptr, TO_ROOM);
                ch->modCurVital(CharVital::stamina, -cost);
                if (MOD_OBJ_VAL(obj, VAL_PLANT_WATERLEVEL, 225) >= 500)
                {
                    SET_OBJ_VAL(obj, VAL_PLANT_WATERLEVEL, 500);
                    ch->sendText("@YThe plant is now at full water level.@n\r\n");
                }
                extract_obj(water);
                WAIT_STATE(ch, PULSE_3SEC);
                improve_skill(ch, SKILL_GARDENING, 0);
                return;
            }
        }
        else if (boost::iequals(arg2, "harvest"))
        {
            Object *obj2, *clippers = nullptr, *next_obj;
            int found = false;
            clippers = ch->searchInventory(253);
            if (!clippers)
            {
                ch->sendText("You do not have any working gardening clippers!\r\n");
                return;
            }
            else if (can_harvest(obj) == false)
            {
                ch->sendText("You can not harvest that plant. Instead, Syntax: garden (plant) (pick)\r\n");
                return;
            }
            else if (GET_OBJ_VAL(obj, VAL_PLANT_WATERLEVEL) <= -10)
            {
                ch->sendText("That plant is dead!\r\n");
                return;
            }
            else if (GET_OBJ_VAL(obj, VAL_PLANT_MATURITY) < GET_OBJ_VAL(obj, VAL_PLANT_MAXMATURE))
            {
                ch->sendText("You stop as you realize that the plant isn't mature enough to harvest.\r\n");
                return;
            }
            else if (skill < axion_dice(-5))
            {
                act("@GAs you go to harvest @g$p@G you end up cutting it in half instead!@n", true, ch, obj, nullptr,
                    TO_CHAR);
                act("@g$n@G attempts to harvest @g$p@G with $s clippers, but accidently cuts the plant in half!@n",
                    true, ch, obj, nullptr, TO_ROOM);
                ch->modCurVital(CharVital::stamina, -cost);
                extract_obj(obj);
                WAIT_STATE(ch, PULSE_3SEC);
                improve_skill(ch, SKILL_GARDENING, 0);
                return;
            }
            else
            {
                act("@GYou calmly and expertly harvest @g$p@G.@n", true, ch, obj, nullptr, TO_CHAR);
                act("@g$n@G calmly and expertly harvests @g$p@G.@n", true, ch, obj, nullptr, TO_ROOM);
                ch->modCurVital(CharVital::stamina, -cost);
                if (MOD_OBJ_VAL(clippers, VAL_ALL_HEALTH, -1) <= 0)
                {
                    ch->sendText("The clippers are now too dull to use.\r\n");
                    return;
                }
                harvest_plant(ch, obj);
                WAIT_STATE(ch, PULSE_3SEC);
                improve_skill(ch, SKILL_GARDENING, 0);
                return;
            }
        }
        else if (boost::iequals(arg2, "dig"))
        {
            Object *obj2, *shovel = nullptr, *next_obj;
            int found = false;
            shovel = ch->searchInventory(254);
            if (!shovel)
            {
                ch->sendText("You do not have any working gardening shovels!\r\n");
                return;
            }
            else
            {
                act("@GYou calmly dig up @g$p@G.@n", true, ch, obj, nullptr, TO_CHAR);
                act("@g$n@G calmly digs up @g$p@G.@n", true, ch, obj, nullptr, TO_ROOM);
                ch->modCurVital(CharVital::stamina, -cost);
                obj->clearLocation();
                ch->addToInventory(obj);
                SET_OBJ_VAL(obj, VAL_OTHER_SOILQUALITY, 0);
                WAIT_STATE(ch, PULSE_3SEC);
                improve_skill(ch, SKILL_GARDENING, 0);
                return;
            }
        }
        else if (boost::iequals(arg2, "plant"))
        {
            Object *obj2, *shovel, *next_obj;
            int found = false;
            shovel = ch->searchInventory(254);

            if (found == false)
            {
                ch->sendText("You do not have any working gardening shovels!\r\n");
                return;
            }
            found = false;
            Object *soil = nullptr;
            soil = ch->searchInventory(255);

            auto con = ch->location.getObjects();

            if (found == false)
            {
                ch->sendText("You don't have any real soil.\r\n");
            }
            else if (con.size() > 7 && ch->location.getRoomFlag(ROOM_GARDEN1))
            {
                ch->sendText("This room already has all its planters full. Try digging up some plants.\r\n");
                return;
            }
            else if (con.size() > 19 && ch->location.getRoomFlag(ROOM_GARDEN2))
            {
                ch->sendText("This room already has all its planters full. Try digging up some plants.\r\n");
                return;
            }
            else if (skill < axion_dice(-5))
            {
                act("@GYou end up digging a hole too shallow to hold @g$p@G. Better try again.@n", true, ch, obj,
                    nullptr, TO_CHAR);
                act("@g$n@G digs a very shallow hole in one of the planters and then realizes @g$p@G won't fit in it.@n",
                    true, ch, obj, nullptr, TO_ROOM);
                ch->modCurVital(CharVital::stamina, -cost);
                WAIT_STATE(ch, PULSE_3SEC);
                improve_skill(ch, SKILL_GARDENING, 0);
                return;
            }
            else
            {
                act("@GYou dig a proper sized hole and plant @g$p@G in it.@n", true, ch, obj, nullptr, TO_CHAR);
                act("@g$n@G digs a proper sized hole in a planter and plants @g$p@G in it.@n", true, ch, obj, nullptr,
                    TO_ROOM);
                obj->clearLocation();
                obj->moveToLocation(ch);
                ch->modCurVital(CharVital::stamina, -cost);
                SET_OBJ_VAL(obj, VAL_PLANT_MAXMATURE, 6);
                SET_OBJ_VAL(obj, VAL_PLANT_MATGOAL, 200);
                SET_OBJ_VAL(obj, VAL_OTHER_SOILQUALITY, GET_OBJ_VAL(soil, VAL_PLANT_SOILQUALITY));
                auto toreduce = 0;
                switch (GET_OBJ_VAL(obj, VAL_OTHER_SOILQUALITY))
                {
                case 1:
                    toreduce -= 10;
                    break;
                case 2:
                    toreduce -= 15;
                    break;
                case 3:
                    toreduce -= 20;
                    break;
                case 4:
                    toreduce -= 25;
                    break;
                case 5:
                    toreduce -= 50;
                    break;
                case 6:
                    toreduce -= 60;
                    break;
                case 7:
                    toreduce -= 70;
                    break;
                default:
                    toreduce -= 80;
                    break;
                }
                MOD_OBJ_VAL(obj, VAL_PLANT_MATGOAL, toreduce);
                extract_obj(soil);
                WAIT_STATE(ch, PULSE_3SEC);
                improve_skill(ch, SKILL_GARDENING, 0);
            }
        }
        else if (boost::iequals(arg2, "pick"))
        {
            if (!OBJ_FLAGGED(obj, ITEM_MATURE))
            {
                ch->sendText("You can't pick that type of plant. Syntax: garden (plant) harvest\r\n");
                return;
            }
            else if (GET_OBJ_VAL(obj, VAL_PLANT_MATURITY) < GET_OBJ_VAL(obj, VAL_PLANT_MAXMATURE))
            {
                ch->sendText("That plant is not mature enough yet.\r\n");
                return;
            }
            else if (skill < axion_dice(-5))
            {
                act("@GYou end up shredding @g$p@G with your clumsy unskilled hands.@n", true, ch, obj, nullptr,
                    TO_CHAR);
                act("@g$n@G grabs a hold of @g$p@G and shreds it in an attempt to pick it.@n", true, ch, obj, nullptr,
                    TO_ROOM);
                return;
            }
            else
            {
                act("@GYou grab a hold of @g$p@G and carefully pick it out of the soil.@n", true, ch, obj, nullptr,
                    TO_CHAR);
                act("@g$n@G grabs a hold of @g$p@G and carefully picks it out of the soil.@n", true, ch, obj, nullptr,
                    TO_ROOM);
                obj->clearLocation();
                ch->addToInventory(obj);
                ch->modCurVital(CharVital::stamina, -cost);
                WAIT_STATE(ch, PULSE_3SEC);
                improve_skill(ch, SKILL_GARDENING, 0);
                return;
            }
        }
        else
        {
            ch->sendText("Syntax: garden (plant) ( water | harvest | dig | plant | pick )\r\n");
            ch->sendText("Syntax: garden collect [Will collect soil from a room with soil.\r\n");
            return;
        }
    }
}

static bool has_housekey(Character *ch, Object *obj)
{

    if (OBJ_FLAGGED(obj, ITEM_DUPLICATE))
        return false;

    auto isHouseKey = [&](Object *obj2) -> bool
    {
        if (obj2->getVnum() == 18800 && obj->getVnum() == 18802)
            return true;
        if (obj2->getVnum() == obj->getVnum() - 1)
            return true;
        return false;
    };

    return ch->searchInventory(isHouseKey);
}

ACMD(do_pack)
{

    Object *obj;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg)
    {
        ch->sendText("Pack up which type of house capsule?\nSyntax: pack (target)\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->location.getObjects())))
    {
        ch->sendText("That house item doesn't seem to be around.\r\n");
        return;
    }
    else
    {
        Object *packed = nullptr;
        if ((GET_OBJ_VNUM(obj) >= 19090 && GET_OBJ_VNUM(obj) <= 19099) || GET_OBJ_VNUM(obj) == 11)
        {
            act("@CYou push a hidden button on $p@C and a cloud of smoke erupts and covers it. As the smoke clears a small capsule can be seen on the ground.@n",
                true, ch, obj, nullptr, TO_CHAR);
            act("@c$n@C pushes a hidden button on $p@C and a cloud of smokes erupts and covers it. As the smoke clears a small capsule can be seen on the ground.@n",
                true, ch, obj, nullptr, TO_ROOM);
            if (GET_OBJ_VNUM(obj) == 11)
            {
                extract_obj(obj);
                packed = read_object(19085, VIRTUAL);
                packed->moveToLocation(ch);
            }
            else
            {
                int fnum = GET_OBJ_VNUM(obj) - 10;
                packed = read_object(fnum, VIRTUAL);
                extract_obj(obj);
                packed->moveToLocation(ch);
            }
            return;
        }
        else if (GET_OBJ_VNUM(obj) >= 18800 && GET_OBJ_VNUM(obj) <= 19199 && GET_OBJ_TYPE(obj) == ITEM_VEHICLE)
        {
            if (!*arg2)
            {
                ch->sendText("This will sell off your house and delete everything inside. Are you sure? If you are then enter the command again with a yes at the end.\nSyntax: pack (house) yes\r\n");
                return;
            }
            else if (!boost::iequals(arg2, "yes"))
            {
                ch->sendText("This will sell off your house and delete everything inside. Are you sure? If you are then enter the command again with a yes at the end.\nSyntax: pack (house) yes\r\n");
                return;
            }
            else if (has_housekey(ch, obj) == 0)
            {
                ch->sendText("You do not own this house.\r\n");
                return;
            }
            else
            {
                Object *cont = nullptr;
                act("@CYou push a hidden button on $p@C and a cloud of smoke erupts and covers it. As the smoke clears a pile of money can be seen on the ground!@n",
                    true, ch, obj, nullptr, TO_CHAR);
                act("@c$n@C pushes a hidden button on $p@C and a cloud of smokes erupts and covers it. As the smoke clears a pile of money can be seen on the ground!@n",
                    true, ch, obj, nullptr, TO_ROOM);
                int money = 0, count = 0, rnum = GET_OBJ_VNUM(obj);
                if (GET_OBJ_VNUM(obj) >= 18800 && GET_OBJ_VNUM(obj) <= 18899)
                {
                    if (rnum == 18802)
                    {
                        rnum = 18800;
                    }
                    else
                    {
                        rnum = rnum - 1;
                    }
                    money = 65000;
                    while (count < 4)
                    {
                        auto r = get_room(rnum);
                        auto con = r->getObjects().snapshot_weak();
                        for (auto o : filter_raw(con))
                            extract_obj(o);
                        count++;
                        rnum++;
                    }
                }
                else if (GET_OBJ_VNUM(obj) >= 18900 && GET_OBJ_VNUM(obj) <= 18999)
                {
                    rnum = rnum - 1;
                    money = 150000;
                    while (count < 4)
                    {
                        auto r = get_room(rnum);
                        auto con = r->getObjects().snapshot_weak();
                        for (auto o : filter_raw(con))
                            extract_obj(o);
                        count++;
                        rnum++;
                    }
                }
                else if (GET_OBJ_VNUM(obj) >= 19100 && GET_OBJ_VNUM(obj) <= 19199)
                {
                    rnum = rnum - 1;
                    money = 1000000;
                    while (count < 4)
                    {
                        auto r = get_room(rnum);
                        auto con = r->getObjects().snapshot_weak();
                        for (auto o : filter_raw(con))
                            extract_obj(o);
                        count++;
                        rnum++;
                    }
                }
                auto con = ch->getInventory();
                for (auto obj2 : filter_raw(con))
                {
                    if (GET_OBJ_VNUM(obj) == 18802)
                    {
                        if (GET_OBJ_VNUM(obj2) == 18800)
                        {
                            extract_obj(obj2);
                        }
                    }
                    else
                    {
                        if (GET_OBJ_VNUM(obj2) == GET_OBJ_VNUM(obj) - 1)
                        {
                            extract_obj(obj2);
                        }
                    }
                }
                auto money_obj = create_money(money);
                money_obj->moveToLocation(ch);
                extract_obj(obj);
                return;
            }
        }
        else
        {
            ch->sendText("That isn't something you can pack up!\r\n");
            return;
        }
    }
}

int check_insidebag(Object *cont, double mult)
{
    int count = 0, containers = 0;

    auto con = cont->getInventory();
    for (auto inside : filter_raw(con))
    {
        if (GET_OBJ_TYPE(inside) == ITEM_CONTAINER)
        {
            count++;
            count += check_insidebag(inside, mult);
            containers++;
        }
        else
        {
            count++;
        }
    }

    count = count * mult;
    count += containers;

    return (count);
}

ACMD(do_deploy)
{

    Object *obj3, *next_obj, *obj4, *obj = nullptr;
    int capsule = false, furniture = false;

    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg)
    {
        auto iscapsule = [](const auto &o)
        { return o->getVnum() == 4 || o->getVnum() == 5 || o->getVnum() == 6; };
        if (obj = ch->searchInventory(iscapsule))
        {
            capsule = true;
        }
    }
    else if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
    {
        ch->sendText("Syntax: deploy (no argument for houses)\nSyntax: deploy (target) <-- For furniture\r\n");
        return;
    }

    if (capsule == false && obj)
    {
        if (GET_OBJ_VNUM(obj) >= 19080 && GET_OBJ_VNUM(obj) <= 19099)
        {
            capsule = true;
            furniture = true;
        }
        else
        {
            ch->sendText("That is not a furniture capsule!\r\n");
            return;
        }
    }

    if (capsule == false)
    {
        ch->sendText("You do not have any house type capsules to deploy.@n\r\n");
        return;
    }
    else if (GET_RP(ch) < 10 && furniture == false)
    {
        ch->sendText("You are required to have (not spend) 10 RPP in order to place a house.\r\n");
        return;
    }
    else if (furniture == true && (!ch->location.getRoomFlag(ROOM_HOUSE) || ch->location.getRoomFlag(ROOM_SHIP)))
    {
        ch->sendText("You can't deploy house furniture capsules here.\r\n");
        return;
    }
    else if (furniture == true &&
             (ch->location.getRoomFlag(ROOM_GARDEN1) || ch->location.getRoomFlag(ROOM_GARDEN2)))
    {
        ch->sendText("You can't deploy house furniture capsules here.\r\n");
        return;
    }
    else if (const auto tile = ch->location.getTileType(); furniture == false && (tile == SECT_INSIDE || tile == SECT_WATER_NOSWIM ||
                                                                                  tile == SECT_WATER_SWIM || tile == SECT_SPACE))
    {
        ch->sendText("You can not deploy that in this kind of area. Try an area more suitable for a house.\r\n");
        return;
    }

    if (furniture == true)
    {
        int fnum = 0;
        if (GET_OBJ_VNUM(obj) == 19080)
        {
            fnum = 19090;
        }
        else if (GET_OBJ_VNUM(obj) == 19081)
        {
            fnum = 19091;
        }
        else if (GET_OBJ_VNUM(obj) == 19082)
        {
            fnum = 19092;
        }
        else if (GET_OBJ_VNUM(obj) == 19083)
        {
            fnum = 19093;
        }
        else if (GET_OBJ_VNUM(obj) == 19085)
        {
            fnum = 11;
        }
        if (fnum != 0)
        {
            Object *furn = read_object(fnum, VIRTUAL);
            act("@CYou click the capsule's button and toss it to the floor. A puff of smoke erupts immediately and quickly dissipates to reveal, $p@C.@n",
                true, ch, furn, nullptr, TO_CHAR);
            act("@c$n@C clicks a capsule's button and tosses it to the floor. A puff of smoke erupts immediately and quickly dissipates to reveal, $p@C.@n",
                true, ch, furn, nullptr, TO_ROOM);
            furn->moveToLocation(ch);
            extract_obj(obj);
            return;
        }
        else
        {
            send_to_imm("ERROR: Furniture failed to deploy at %d.", ch->location.getVnum());
            return;
        }
    }

    int rnum = 18800, giveup = false, cont = false, found = false, type = 0;

    if (GET_OBJ_VNUM(obj) == 4)
    {
        type = 0;
    }
    else if (GET_OBJ_VNUM(obj) == 5)
    {
        rnum = 18900;
        type = 1;
    }
    else if (GET_OBJ_VNUM(obj) == 6)
    {
        rnum = 19100;
        type = 2;
    }

    int final = rnum + 99;

    while (giveup == false && cont == false)
    {
        if (obj3 = Location(get_room(rnum)).searchObjects(18801))
        {
            found = true;
        }
        if (found == true && rnum < final)
        {
            if (type == 0)
            {
                rnum += 4;
            }
            else
            {
                rnum += 5;
            }
            found = false;
        }
        else if (rnum >= final)
        {
            giveup = true;
        }
        else
        {
            cont = true;
        }
    } /* End while */

    if (cont == true)
    {
        int hnum = ch->location.getVnum();
        Object *door = read_object(18801, VIRTUAL);

        SET_OBJ_VAL(door, VAL_HATCH_LOCATION, ch->location.getVnum());
        if (rnum != 18800)
            SET_OBJ_VAL(door, VAL_HATCH_DEST, rnum + 1);
        else
            SET_OBJ_VAL(door, VAL_HATCH_DEST, 18802);
        SET_OBJ_VAL(door, VAL_HATCH_DCSKILL, rnum);
        door->moveToLocation(rnum);
        Object *key = read_object(rnum, VIRTUAL);
        ch->addToInventory(key);
        act("@WYou click the capsule and toss it to the ground. A large cloud of smoke erupts from the capsule and after it clears a house is visible in its place!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W clicks a capsule and then tosses it to the ground. A large cloud of smoke erupts from the capsule and after it clears a house is visible in its place!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        Object *foun = read_object(18803, VIRTUAL);
        foun->moveToLocation(rnum + 1);
        extract_obj(obj);
    }
    else
    {
        ch->sendText("@ROOC@D: @wSorry for the inconvenience, but it appears there are no houses available. Please contact Iovan.@n\r\n");
        return;
    }
}

ACMD(do_twohand)
{

    if (GRAPPLING(ch) || GRAPPLED(ch))
    {
        ch->sendText("You are busy grappling with someone!\r\n");
        return;
    }

    if (ABSORBING(ch) || ABSORBBY(ch))
    {
        ch->sendText("You are busy struggling with someone!\r\n");
        return;
    }

    if (!GET_EQ(ch, WEAR_WIELD1) && !PLR_FLAGGED(ch, PLR_THANDW))
    {
        ch->sendText("You need to wield a sword to use this.\r\n");
        return;
    }
    else if (GET_EQ(ch, WEAR_WIELD2) && !PLR_FLAGGED(ch, PLR_THANDW))
    {
        ch->sendText("You have something in your offhand already and can't two hand wield your main weapon.\r\n");
        return;
    }
    else if ((GET_LIMBCOND(ch, 0) <= 0 || GET_LIMBCOND(ch, 1) <= 0) && !PLR_FLAGGED(ch, PLR_THANDW))
    {
        ch->sendText("Kind of hard with only one arm...\r\n");
        return;
    }
    else if (PLR_FLAGGED(ch, PLR_THANDW))
    {
        ch->sendText("You stop wielding your weapon with both hands.\r\n");
        act("$n stops wielding $s weapon with both hands.", true, ch, nullptr, nullptr, TO_ROOM);
        ch->player_flags.set(PLR_THANDW, false);
        return;
    }
    else
    {
        ch->sendText("You grab your weapon with both hands.\r\n");
        act("$n starts wielding $s weapon with both hands.", true, ch, nullptr, nullptr, TO_ROOM);
        ch->player_flags.set(PLR_THANDW, true);
        return;
    }
}

static void start_auction(Character *ch, Object *obj, int bid)
{
    /* Take object from character and set variables */

    obj->clearLocation();
    obj_selling = obj;
    ch_selling = ch;
    ch_buying = nullptr;
    curbid = bid;

    /* Tell th character where his item went */
    sprintf(buf, "%s magicly flies away from your hands to be auctioned!\r\n", obj_selling->getShortDescription());
    CAP(buf);
    ch_selling->sendText(buf);

    /* Anounce the item is being sold */
    sprintf(buf, auctioneer[AUC_NULL_STATE], curbid);
    auc_send_to_all(buf, false);

    aucstat = AUC_OFFERING;
}

void check_auction(uint64_t heartPulse, double deltaTime)
{
    switch (aucstat)
    {
    case AUC_NULL_STATE:
        return;
    case AUC_OFFERING:
    {
        if (obj_selling == nullptr)
        {
            auc_send_to_all(
                "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n",
                false);
            curbid = 0;
            ch_selling = nullptr;
            ch_buying = nullptr;
            aucstat = AUC_NULL_STATE;
            return;
        }
        sprintf(buf, auctioneer[AUC_OFFERING], curbid);
        CAP(buf);
        auc_send_to_all(buf, false);
        aucstat = AUC_GOING_ONCE;
        return;
    }
    case AUC_GOING_ONCE:
    {
        if (obj_selling == nullptr)
        {
            auc_send_to_all(
                "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n",
                false);
            curbid = 0;
            ch_selling = nullptr;
            ch_buying = nullptr;
            aucstat = AUC_NULL_STATE;
            return;
        }

        sprintf(buf, auctioneer[AUC_GOING_ONCE], curbid);
        CAP(buf);
        auc_send_to_all(buf, false);
        aucstat = AUC_GOING_TWICE;
        return;
    }
    case AUC_GOING_TWICE:
    {
        if (obj_selling == nullptr)
        {
            auc_send_to_all(
                "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n",
                false);
            curbid = 0;
            ch_selling = nullptr;
            ch_buying = nullptr;
            aucstat = AUC_NULL_STATE;
            return;
        }

        sprintf(buf, auctioneer[AUC_GOING_TWICE], curbid);
        CAP(buf);
        auc_send_to_all(buf, false);
        aucstat = AUC_LAST_CALL;
        return;
    }
    case AUC_LAST_CALL:
    {
        if (obj_selling == nullptr)
        {
            auc_send_to_all(
                "@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n",
                false);
            curbid = 0;
            ch_selling = nullptr;
            ch_buying = nullptr;
            aucstat = AUC_NULL_STATE;
            return;
        }
        if (ch_buying == nullptr)
        {

            sprintf(buf, auctioneer[AUC_LAST_CALL], curbid);

            CAP(buf);
            auc_send_to_all(buf, false);

            sprintf(buf, "%s flies out the sky and into your hands.\r\n", obj_selling->getShortDescription());
            CAP(buf);
            ch_selling->sendText(buf);
            ch_selling->addToInventory(obj_selling);

            /* Reset auctioning values */
            obj_selling = nullptr;
            ch_selling = nullptr;
            ch_buying = nullptr;
            curbid = 0;
            aucstat = AUC_NULL_STATE;
            return;
        }
        else
        {

            sprintf(buf, auctioneer[AUC_SOLD], curbid);
            auc_send_to_all(buf, true);

            /* Give the object to the buyer */
            ch_buying->addToInventory(obj_selling);
            sprintf(buf, "%s flies out the sky and into your hands, what a steal!\r\n",
                    obj_selling->getShortDescription());
            CAP(buf);
            ch_buying->sendText(buf);

            sprintf(buf, "Congrats! You have sold %s for @Y%d@W zenni!\r\n", obj_selling->getShortDescription(),
                    curbid);
            ch_selling->sendText(buf);

            /* Give selling char the money for his stuff */
            if (GET_GOLD(ch_selling) + curbid > GOLD_CARRY(ch_selling))
            {
                ch_buying->sendText("You couldn't hold all the zenni, so some of it was deposited for you.\r\n");
                int diff = 0;
                diff = (GET_GOLD(ch_selling) + curbid) - GOLD_CARRY(ch_selling);
                ch_selling->setBaseStat("money_carried", GOLD_CARRY(ch_selling));
                ch_selling->modBaseStat("money_bank", diff);
            }
            else if (GET_GOLD(ch_selling) + curbid <= GOLD_CARRY(ch_selling))
            {
                ch_selling->modBaseStat("money_carried", curbid);
            }
            /* Reset auctioning values */
            obj_selling = nullptr;
            ch_selling = nullptr;
            ch_buying = nullptr;
            curbid = 0;
            aucstat = AUC_NULL_STATE;
            return;
        }
    }
    }
}

void loadDragonball(int vnum, int &foundFlag, bool &hunter1, bool &hunter2)
{
    if (foundFlag == false)
    {
        bool load = false;
        int room = 0;
        int num = Random::get<int>(200, 20000);
        mob_rnum r_num;
        Character *hunter = nullptr;
        Object *k = nullptr;

        while (!load)
        {
            if (real_room(num) != NOWHERE)
            {
                if (WHERE_FLAGGED(real_room(num), WhereFlag::planet_earth) || WHERE_FLAGGED(real_room(num), WhereFlag::planet_vegeta) ||
                    WHERE_FLAGGED(real_room(num), WhereFlag::planet_frigid) || WHERE_FLAGGED(real_room(num), WhereFlag::planet_aether) ||
                    WHERE_FLAGGED(real_room(num), WhereFlag::planet_namek) || WHERE_FLAGGED(real_room(num), WhereFlag::planet_konack) ||
                    WHERE_FLAGGED(real_room(num), WhereFlag::planet_yardrat))
                {
                    room = num;
                    load = true;
                }
            }
            num = Random::get<int>(200, 20000);
        }

        if (Random::get<int>(1, 10) > 8)
        {
            if (!hunter1)
            {
                if ((r_num = real_mobile(DBALL_HUNTER1_VNUM)) == NOBODY)
                    return;
                hunter = read_mobile(r_num, REAL);
                hunter->moveToLocation(room);
                hunter1 = true;
                DBALL_HUNTER1 = room;
            }
            else if (!hunter2)
            {
                if ((r_num = real_mobile(DBALL_HUNTER2_VNUM)) == NOBODY)
                    return;
                hunter = read_mobile(r_num, REAL);
                hunter->moveToLocation(room);
                hunter2 = true;
                DBALL_HUNTER2 = room;
            }
            k = read_object(vnum, VIRTUAL);
            hunter->addToInventory(k);
        }
        else
        {
            k = read_object(vnum, VIRTUAL);
            k->moveToLocation(room);
        }
    }
}

void dball_load(uint64_t heartPulse, double deltaTime)
{
    if (SELFISHMETER >= 10)
        return;

    int foundFlags[7] = {false, false, false, false, false, false, false};
    bool hunter1 = false, hunter2 = false;

    if (dballtime == 0)
    {
        Object *k = nullptr;

        WISHTIME = 0;
        auto ao = objectSubscriptions.all("active");
        for (auto k : filter_raw(ao))
        {
            if (OBJ_FLAGGED(k, ITEM_FORGED))
                continue;

            int vnum = GET_OBJ_VNUM(k);
            if (vnum >= 20 && vnum <= 26)
            {
                foundFlags[vnum - 20] = true;
            }
            else if (k->location.getGroundEffect() == 6 && !OBJ_FLAGGED(k, ITEM_UNBREAKABLE))
            {
                k->location.send_to("@R%s@r melts in the lava!@n\r\n", k->getShortDescription());
                extract_obj(k);
            }
        }

        for (int i = 0; i < 7; ++i)
        {
            loadDragonball(20 + i, foundFlags[i], hunter1, hunter2);
        }

        dballtime = 604800;
    }
    else if (dballtime == 518400 || dballtime == 432000 || dballtime == 345600 ||
             dballtime == 259200 || dballtime == 172800 || dballtime == 86400)
    {
        dballtime -= 1;
    }
    else
    {
        if (WISHTIME == 0)
        {
            WISHTIME = dballtime - 1;
        }
        else if (WISHTIME > 0 && dballtime != WISHTIME)
        {
            dballtime = WISHTIME;
        }
        WISHTIME -= 1;
        dballtime -= 1;
    }
}

ACMD(do_auction)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Object *obj;
    int bid = 0;

    two_arguments(argument, arg1, arg2);

    if (ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
    {
        ch->sendText("This is a different dimension!\r\n");
        return;
    }
    if (ch->location.getWhereFlag(WhereFlag::pendulum_past))
    {
        ch->sendText("You are in the past!\r\n");
        return;
    }
    if (PRF_FLAGGED(ch, PRF_HIDE))
    {
        ch->sendText("The auctioneer will not accept items from hidden people.\r\n");
        return;
    }

    if (!*arg1)
    {
        ch->sendText("Auction what?\r\n");
        ch->sendText("[ Auction: <item> | <cancel> ]\r\n");
        return;
    }
    else if (is_abbrev(arg1, "cancel") || is_abbrev(arg1, "stop"))
    {
        if ((ch != ch_selling && GET_ADMLEVEL(ch) <= ADMLVL_GRGOD) || aucstat == AUC_NULL_STATE)
        {
            ch->sendText("You're not even selling anything!\r\n");
            return;
        }
        else if (ch == ch_selling)
        {
            stop_auction(AUC_NORMAL_CANCEL, nullptr);
            return;
        }
        else
        {
            stop_auction(AUC_WIZ_CANCEL, ch);
        }
    }
    else if (is_abbrev(arg1, "stats") || is_abbrev(arg1, "identify"))
    {
        auc_stat(ch, obj_selling);
        return;
    }
    else if (!(obj = get_obj_in_list_vis(ch, arg1, nullptr, ch->getInventory())))
    {
        sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
        ch->sendText(buf);
        return;
    }
    else if (!*arg2)
    {
        sprintf(buf, "What should be the minimum bid?\r\n");
        ch->sendText(buf);
        return;
    }
    else if (*arg2 && (bid = atoi(arg2)) <= 0)
    {
        ch->sendText("Come on? One zenni at least?\r\n");
        return;
    }
    else if (aucstat != AUC_NULL_STATE)
    {
        sprintf(buf, "Sorry, but %s is already auctioning %s at @Y%d@W zenni!\r\n", GET_NAME(ch_selling),
                obj_selling->getShortDescription(), bid);
        ch->sendText(buf);
        return;
    }
    else if (OBJ_FLAGGED(obj, ITEM_NOSELL))
    {
        ch->sendText("Sorry but you can't sell that!\r\n");
        return;
    }
    else if (GET_OBJ_VAL(obj, VAL_CONTAINER_CORPSE) == 1)
    {
        ch->sendText("Sorry but you can't sell that!\r\n");
        return;
    }
    else
    {
        ch->sendText("Ok.\r\n");
        start_auction(ch, obj, bid);
        return;
    }
}

ACMD(do_bid)
{
    Object *obj, *next_obj, *obj2 = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int found = false, list = 0, masterList = 0;
    room_vnum auct_room;

    auct_room = real_room(80);

    if (IS_NPC(ch))
        return;

    if (!GET_EQ(ch, WEAR_EYE))
    {
        ch->sendText("You need a scouter to make an auction bid.\r\n");
        return;
    }

    if (ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
    {
        ch->sendText("This is a different dimension!\r\n");
        return;
    }
    if (ch->location.getWhereFlag(WhereFlag::pendulum_past))
    {
        ch->sendText("This is the past, nothing is being auctioned!\r\n");
        return;
    }

    two_arguments(argument, arg, arg2);

    if (!*arg)
    {
        ch->sendText("Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
        return;
    }
    auto con = get_room(auct_room)->getObjects().snapshot_weak();
    for (auto obj : filter_raw(con))
    {
        list++;
    }
    masterList = list;
    list = 0;

    if (boost::iequals(arg, "list"))
    {
        ch->sendText("@Y                                   Auction@n\r\n");
        ch->sendText("@c------------------------------------------------------------------------------@n\r\n");
        auto con = get_room(auct_room)->getObjects().snapshot_weak();
        for (auto obj : filter_raw(con))
        {
            if (GET_AUCTER(obj) <= 0)
            {
                continue;
            }
            list++;
            if (GET_AUCTIME(obj) + 86400 > time(nullptr) && GET_CURBID(obj) <= -1)
            {
                ch->send_to("@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: @w%-*s@D][@GCost@W: @Y%s@D]@n\r\n", list, get_name_by_id(GET_AUCTER(obj)) ? CAP(get_name_by_id(GET_AUCTER(obj))) : "Nobody", count_color_chars(obj->getShortDescription()) + 30, obj->getShortDescription(), add_commas(GET_BID(obj)).c_str());
            }
            else if (GET_AUCTIME(obj) + 86400 > time(nullptr) && GET_CURBID(obj) > -1)
            {
                ch->send_to("@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: @w%-*s@D][@RTop Bid@W: %s @Y%s@D]@n\r\n", list, get_name_by_id(GET_AUCTER(obj)) ? CAP(get_name_by_id(GET_AUCTER(obj))) : "Nobody", count_color_chars(obj->getShortDescription()) + 30, obj->getShortDescription(), get_name_by_id(GET_CURBID(obj)) ? CAP(get_name_by_id(GET_CURBID(obj))) : "Nobody", add_commas(GET_BID(obj)).c_str());
            }
            else if (GET_AUCTIME(obj) + 86400 < time(nullptr) && GET_CURBID(obj) > -1)
            {
                ch->send_to("@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: @w%-*s@D][@RBid Winner@W: %s @Y%s@D]@n\r\n", list, get_name_by_id(GET_AUCTER(obj)) ? CAP(get_name_by_id(GET_AUCTER(obj))) : "Nobody", count_color_chars(obj->getShortDescription()) + 30, obj->getShortDescription(), get_name_by_id(GET_CURBID(obj)) ? CAP(get_name_by_id(GET_CURBID(obj))) : "Nobody", add_commas(GET_BID(obj)).c_str());
            }
            else
            {
                ch->send_to("@D[@R#@W%3d@D][@mOwner@W: @w%10s@D][@GItem Name@W: @w%-*s@D][@RClosed@D]@n\r\n", list, get_name_by_id(GET_AUCTER(obj)) ? CAP(get_name_by_id(GET_AUCTER(obj))) : "Nobody", count_color_chars(obj->getShortDescription()) + 30, obj->getShortDescription());
            }
            found = true;
        }
        if (found == false)
        {
            ch->sendText("No items are currently being auctioned.\r\n");
        }
        ch->sendText("@c------------------------------------------------------------------------------@n\r\n");
    }
    else if (boost::iequals(arg, "appraise"))
    {
        if (!*arg2)
        {
            ch->sendText("Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
            ch->sendText("What item number did you want to appraise?\r\n");
            return;
        }
        else if (atoi(arg2) < 0 || atoi(arg2) > masterList)
        {
            ch->sendText("Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
            ch->sendText("That item number doesn't exist.\r\n");
            return;
        }

        auto con = get_room(auct_room)->getObjects().snapshot_weak();
        for (auto obj : filter_raw(con))
        {
            if (GET_AUCTER(obj) <= 0)
            {
                continue;
            }
            list++;
            if (atoi(arg2) == list)
            {
                obj2 = obj;
            }
        }
        if (!obj2)
        {
            ch->sendText("That item number is not found.\r\n");
            return;
        }
        else
        {
            if (!GET_SKILL(ch, SKILL_APPRAISE))
            {
                ch->sendText("You are unskilled at appraising.\r\n");
                return;
            }
            improve_skill(ch, SKILL_APPRAISE, 1);
            if (GET_SKILL(ch, SKILL_APPRAISE) < Random::get<int>(1, 101))
            {
                ch->send_to("You look at the images for %s and fail to perceive its worth..\r\n", obj2->getShortDescription());
                act("@c$n@w looks stumped about something they viewed on their scouter screen.@n", true, ch, nullptr,
                    nullptr, TO_ROOM);
                WAIT_STATE(ch, PULSE_2SEC);
                return;
            }
            else
            {
                ch->sendText("You look at images of the object on your scouter.\r\n");
                act("@c$n@w looks at something on their scouter screen.@n", true, ch, nullptr, nullptr, TO_ROOM);
                ch->sendText("@c------------------------------------------------------------------------\n");
                ch->send_to("@GOwner       @W: @w%s@n\n", get_name_by_id(GET_AUCTER(obj2)) ? CAP(get_name_by_id(GET_AUCTER(obj2)))
                                                                                           : "Nobody");
                ch->send_to("@GItem Name   @W: @w%s@n\n", obj2->getShortDescription());
                ch->send_to("@GCurrent Bid @W: @Y%s@n\n", add_commas(GET_BID(obj2)).c_str());
                ch->send_to("@GStore Value @W: @Y%s@n\n", add_commas(GET_OBJ_COST(obj2)).c_str());
                ch->send_to("@GItem Min LVL@W: @w%d@n\n", GET_OBJ_LEVEL(obj2));
                if (GET_OBJ_VAL(obj2, VAL_ALL_HEALTH) >= 100)
                {
                    ch->send_to("@GCondition   @W: @C%d%s@n\n", GET_OBJ_VAL(obj2, VAL_ALL_HEALTH), "%");
                }
                else if (GET_OBJ_VAL(obj2, VAL_ALL_HEALTH) >= 50)
                {
                    ch->send_to("@GCondition   @W: @y%d%s@n\n", GET_OBJ_VAL(obj2, VAL_ALL_HEALTH), "%");
                }
                else if (GET_OBJ_VAL(obj2, VAL_ALL_HEALTH) >= 1)
                {
                    ch->send_to("@GCondition   @W: @r%d%s@n\n", GET_OBJ_VAL(obj2, VAL_ALL_HEALTH), "%");
                }
                else
                {
                    ch->send_to("@GCondition   @W: @D%d%s@n\n", GET_OBJ_VAL(obj2, VAL_ALL_HEALTH), "%");
                }
                ch->send_to("@GItem Weight @W: @w%s@n\n", add_commas(GET_OBJ_WEIGHT(obj2)).c_str());
                char bits[MAX_STRING_LENGTH];
                sprintf(bits, "%s", GET_OBJ_WEAR(obj2).getFlagNames().c_str());
                search_replace(bits, "TAKE", "");
                ch->send_to("@GWear Loc.   @W:@w%s\n", bits);
                if (GET_OBJ_TYPE(obj2) == ITEM_WEAPON)
                {
                    auto wlvl = obj2->getBaseStat<int64_t>(VAL_WEAPON_LEVEL);
                    if (wlvl == 1)
                    {
                        ch->send_to("@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w5%s@D]@n\r\n", "%");
                    }
                    else if (wlvl == 2)
                    {
                        ch->send_to("@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w10%s@D]@n\r\n", "%");
                    }
                    else if (wlvl == 3)
                    {
                        ch->send_to("@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w20%s@D]@n\r\n", "%");
                    }
                    else if (wlvl == 4)
                    {
                        ch->send_to("@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w30%s@D]@n\r\n", "%");
                    }
                    else if (wlvl == 5)
                    {
                        ch->send_to("@GWeapon Level@W: @D[@C1@D]\n@GDamage Bonus@W: @D[@w50%s@D]@n\r\n", "%");
                    }
                }
                int i, found = false;
                ch->send_to("@GItem Size   @W:@w %s@n\r\n", size_names[GET_OBJ_SIZE(obj2)]);
                ch->sendText("@GItem Bonuses@W:@w");
                for (i = 0; i < MAX_OBJ_AFFECT; i++)
                {
                    if (obj2->affected[i].modifier)
                    {
                        sprinttype(obj2->affected[i].location, apply_types, buf, sizeof(buf));
                        ch->send_to("%s %+d to %s", found++ ? "," : "", obj2->affected[i].modifier, buf);
                        switch (obj2->affected[i].location)
                        {
                        case APPLY_SKILL:
                            ch->send_to(" (%s)", spell_info[obj2->affected[i].specific].name);
                            break;
                        }
                    }
                }
                if (!found)
                    ch->sendText(" None@n");
                else
                    ch->sendText("@n");
                char buf2[MAX_STRING_LENGTH];
                sprintf(buf2, "%s", GET_OBJ_PERM(obj2).getFlagNames().c_str());
                ch->send_to("\n@GSpecial     @W:@w %s\n", buf2);
                ch->sendText("@c------------------------------------------------------------------------\n");
                return;
            }
        }
    }
    else
    {
        if (!*arg2)
        {
            ch->sendText("Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
            ch->sendText("What amount did you want to bid?\r\n");
            return;
        }
        else if (atoi(arg) < 0 || atoi(arg) > masterList)
        {
            ch->sendText("Syntax: bid [ list | # ] (amt)\r\nOr...\r\nSyntax: bid appraise (list number)\r\n");
            ch->sendText("That item number is not found.\r\n");
            return;
        }
        auto con = Location(auct_room).getObjects();
        for (auto obj : filter_raw(con))
        {
            if (GET_AUCTER(obj) <= 0)
            {
                continue;
            }
            list++;
            if (atoi(arg) == list)
            {
                obj2 = obj;
            }
        }

        if (!obj2)
        {
            ch->sendText("That item number is not found.\r\n");
            return;
        }
        else if (GET_CURBID(obj2) == ((ch)->id))
        {
            ch->sendText("You already have the highest bid.\r\n");
            return;
        }
        else if (GET_AUCTER(obj2) == ((ch)->id))
        {
            ch->sendText("You auctioned the item, go to the auction house and cancel if you can.\r\n");
            return;
        }
        else if (GET_CURBID(obj2) > 0 && atoi(arg2) <= (GET_BID(obj2) + (GET_BID(obj2) * .1)) &&
                 GET_CURBID(obj2) > -1)
        {
            ch->sendText("You have to bid at least 10 percent over the current bid.\r\n");
            return;
        }
        else if (atoi(arg2) < GET_BID(obj2) && GET_CURBID(obj2) <= -1)
        {
            ch->sendText("You have to bid at least the starting bid.\r\n");
            return;
        }
        else if (atoi(arg2) >
                 (((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / 100) * 50) + (GET_GOLD(ch) + GET_BANK_GOLD(ch)))
        {
            ch->send_to("You can not bid more than 150%s of your total money (on hand and in the bank).\r\n", "%");
            return;
        }
        else if (GET_AUCTIME(obj2) + 86400 <= time(nullptr))
        {
            ch->sendText("Bidding on that object has been closed.\r\n");
            return;
        }
        else
        {
            GET_BID(obj2) = atoi(arg2);
            GET_CURBID(obj2) = ((ch)->id);
            auc_save();
            struct descriptor_data *d;
            int bid = atoi(arg2);
            basic_mud_log("AUCTION: %s has bid %s on %s", GET_NAME(ch), obj2->getShortDescription(), add_commas(bid).c_str());
            for (d = descriptor_list; d; d = d->next)
            {
                if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                    continue;
                if (d->character == ch)
                {
                    if (GET_EQ(d->character, WEAR_EYE))
                    {
                        d->character->send_to("@RScouter Auction News@D: @GYou have bid @Y%s@G on @w%s@G@n\r\n", add_commas(GET_BID(obj2)).c_str(), obj2->getShortDescription());
                    }
                    continue;
                }
                if (GET_EQ(d->character, WEAR_EYE))
                {
                    d->character->send_to("@RScouter Auction News@D: @GThe bid on, @w%s@G, has been raised to @Y%s@n\r\n", obj2->getShortDescription(), add_commas(GET_BID(obj2)).c_str());
                }
            }
        }
    }
}

void stop_auction(int type, Character *ch)
{
    if (obj_selling == nullptr)
    {
        auc_send_to_all("@RThe auction has stopped because someone has made off with the auctioned object!@n\r\n",
                        false);
        curbid = 0;
        ch_selling = nullptr;
        ch_buying = nullptr;
        aucstat = AUC_NULL_STATE;
        return;
    }
    switch (type)
    {

    case AUC_NORMAL_CANCEL:
    {

        snprintf(buf, sizeof(buf), "%s", auctioneer[AUC_NORMAL_CANCEL]);
        auc_send_to_all(buf, false);
        break;
    }
    case AUC_QUIT_CANCEL:
    {

        snprintf(buf, sizeof(buf), "%s", auctioneer[AUC_QUIT_CANCEL]);
        auc_send_to_all(buf, false);
        break;
    }
    case AUC_WIZ_CANCEL:
    {

        snprintf(buf, sizeof(buf), "%s", auctioneer[AUC_WIZ_CANCEL]);
        auc_send_to_all(buf, false);
        break;
    }
    default:
    {
        ch->sendText("Sorry, that is an unrecognised cancel command, please report.");
        return;
    }
    }

    if (type != AUC_WIZ_CANCEL)
    {
        sprintf(buf, "%s flies out the sky and into your hands.\r\n", obj_selling->getShortDescription());
        CAP(buf);
        ch_selling->sendText(buf);
        ch_selling->addToInventory(obj_selling);
    }
    else
    {
        sprintf(buf, "%s flies out the sky and into your hands.\r\n", obj_selling->getShortDescription());
        CAP(buf);
        ch->sendText(buf);
        ch->addToInventory(obj_selling);
    }

    if (!(ch_buying == nullptr))
        ch_buying->modBaseStat("money_carried", curbid);

    obj_selling = nullptr;
    ch_selling = nullptr;
    ch_buying = nullptr;
    curbid = 0;

    aucstat = AUC_NULL_STATE;
}

static void auc_stat(Character *ch, Object *obj)
{

    if (aucstat == AUC_NULL_STATE)
    {
        ch->sendText("Nothing is being auctioned!\r\n");
        return;
    }
    else if (ch == ch_selling)
    {
        ch->sendText("You should have found that out BEFORE auctioning it!\r\n");
        return;
    }
    else if (GET_GOLD(ch) < 500)
    {
        ch->sendText("You can't afford to find the stats on that, it costs 500 zenni!\r\n");
        return;
    }
    else
    {
        /* auctioneer tells the character the auction details */
        sprintf(buf, auctioneer[AUC_STAT], curbid);
        act(buf, true, ch_selling, obj, ch, TO_VICT | TO_SLEEP);
        ch->modBaseStat("money_carried", -500);

        /*call_magic(ch, nullptr, obj_selling, SPELL_IDENTIFY, 30, CAST_SPELL);*/
    }
}

static void auc_send_to_all(const char *messg, bool buyer)
{
    struct descriptor_data *i;

    if (messg == nullptr)
        return;

    for (i = descriptor_list; i; i = i->next)
    {
        if (STATE(i) != CON_PLAYING)
            continue;
        if (i->character->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
            continue;
        if (i->character->location.getWhereFlag(WhereFlag::pendulum_past))
            continue;
        if (buyer)
            act(messg, true, ch_buying, obj_selling, i->character, TO_VICT | TO_SLEEP);
        else
            act(messg, true, ch_selling, obj_selling, i->character, TO_VICT | TO_SLEEP);
    }
}

ACMD(do_assemble)
{
    long lVnum = NOTHING;
    Object *pObject = nullptr;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    int roll = 0;

    // skip_spaces(&argument);
    two_arguments(argument, arg, arg2);

    Object *tool = nullptr, *next_obj;

    if (tool = ch->searchInventory(386))
    {
        act("@WYou open up your toolkit and take out the necessary tools.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W opens up $s toolkit and takes out the necessary tools.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }

    int menu = 0;

    for (int i = 0; i < NUM_ITEM_TYPES; i++)
    {
        if (strcasestr(arg, item_types[i]))
            menu = i;
    }

    if (*argument == '\0' || menu > 0)
    {
        ch->send_to("What would you like to %s?\r\n", CMD_NAME);
        assemblyListToChar(ch, menu);
        return;
    }

    if (!strcasestr(arg, "create"))
    {
        ch->sendText("If you wish to make something, the syntax is 'build create <item>'\r\n");
        ch->sendText("Categories available to show via 'build <category>':\r\n");
        ch->sendText("[WEAPON]\r\n");
        ch->sendText("[ARMOR]\r\n");
        ch->sendText("[LIGHT]\r\n");
        ch->sendText("[WORN]\r\n");
        ch->sendText("[LIQCONTAINER]\r\n");
        ch->sendText("[CAMPFIRE]\r\n");
        ch->sendText("[OTHER]\r\n");

        return;
    }

    if ((lVnum = assemblyFindAssembly(arg2)) < 0 && (lVnum = assemblyGetAssemblyIndex(atoi(arg2))) < 0)
    {
        ch->send_to("You can't %s %s %s.\r\n", CMD_NAME, AN(arg2), arg2);
        return;
    }
    else if (!assemblyCheckComponents(lVnum, ch, false))
    {
        ch->sendText("You haven't got all the things you need.\r\n");
        return;
    }
    else if (ch->location.getWhereFlag(WhereFlag::space))
    {
        ch->sendText("You can't do that in space.");
        return;
    }
    else if (!GET_SKILL(ch, SKILL_SURVIVAL) && boost::iequals(arg2, "campfire"))
    {
        ch->sendText("You know nothing about building campfires.\r\n");
        return;
    }

    if (strstr(arg2, "Signal") || strstr(arg2, "signal"))
    {
        if (GET_SKILL(ch, SKILL_BUILD) < 70)
        {
            ch->sendText("You need at least a build skill level of 70.\r\n");
            return;
        }
    }

    if (!boost::iequals(arg2, "campfire"))
    {
        if (ch->location.getWhereFlag(WhereFlag::space) || ch->location.getTileType() == SECT_WATER_NOSWIM || ch->location.getEnvironment(ENV_WATER) >= 100.0)
        {
            ch->sendText("This area will not allow a fire to burn properly.\r\n");
            return;
        }

        roll += axion_dice(0);

        improve_skill(ch, SKILL_SURVIVAL, 1);
    }
    improve_skill(ch, SKILL_BUILD, 1);

    /* Create the assembled object. */
    if ((pObject = read_object(lVnum, VIRTUAL)) == nullptr)
    {
        ch->send_to("You can't %s %s %s.\r\n", CMD_NAME, AN(arg2), arg2);
        return;
    }

    assemblyCheckComponents(lVnum, ch, true);

    /* Now give the object to the character. */
    if (GET_OBJ_VNUM(pObject) != 1611)
    {
        // ch->addToInventory(pObject);
        ch->craftingTask.improvementRounds = 0;
        ch->craftingTask.pObject = pObject;

        ch->craftingDeck.initDeck(ch);
        ch->task = Task::crafting;

        ch->send_to("You start crafting a %s\r\n", pObject->getName());
        act("$n starts to work on creating something.\r\n", true, ch, nullptr, nullptr, TO_ROOM);

        WAIT_STATE(ch, PULSE_5SEC * 4);
    }
    else
    {
        pObject->moveToLocation(ch);
        pObject->setBaseStat<int>("timer", (GET_SKILL(ch, SKILL_SURVIVAL) * 0.12));
    }
}

static void perform_put(Character *ch, Object *obj,
                        Object *cont)
{

    static std::unordered_set<int> dball = {20, 21, 22, 23, 24, 25, 26};

    if (!drop_otrigger(obj, ch))
        return;

    if (!obj) /* object might be extracted by drop_otrigger */
        return;
    if (OBJ_FLAGGED(cont, ITEM_FORGED))
    {
        act("$P is forged and won't hold anything.", false, ch, nullptr, cont, TO_CHAR);
        return;
    }
    if (OBJ_FLAGGED(cont, ITEM_SHEATH) && GET_OBJ_TYPE(obj) != ITEM_WEAPON)
    {
        ch->sendText("That is made to only hold weapons.\r\n");
        return;
    }
    if (OBJ_FLAGGED(cont, ITEM_SHEATH))
    {
        Object *obj2 = nullptr, *next_obj = nullptr;
        int count = 0, minus = 0;
        auto con = cont->getInventory();
        for (auto obj2 : filter_raw(con))
        {
            minus += GET_OBJ_WEIGHT(obj2);
            count++;
        }
        int holds = GET_OBJ_WEIGHT(cont) - minus;
        if (count >= holds)
        {
            ch->send_to("It can only hold %d weapon%s at a time.\r\n", holds, holds > 1 ? "s" : "");
            return;
        }
    }

    auto ovn = cont->getVnum();

    if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) && (GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY) == 0))
        act("$p won't fit in $P.", false, ch, obj, cont, TO_CHAR);
    else if (ovn >= 600 && ovn <= 603)
        ch->sendText("You can't put cards on a duel table. You have to @Gplay@n them.\r\n");
    else if ((ovn == 697 || ovn == 698 || ovn == 682 ||
              ovn == 683 || ovn == 684 || OBJ_FLAGGED(cont, ITEM_CARDCASE)) &&
             !OBJ_FLAGGED(obj, ITEM_CARD))
        ch->sendText("You can only put cards in a case.\r\n");
    else if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) &&
             (GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY) > 0) &&
             (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY)))
        act("$p won't fit in $P.", false, ch, obj, cont, TO_CHAR);
    else if (OBJ_FLAGGED(obj, ITEM_NODROP) && cont->location)
        act("You can't get $p out of your hand.", false, ch, obj, nullptr, TO_CHAR);
    else if (dball.contains(ovn))
        ch->sendText("You can not bag dragon balls.\r\n");
    else if (OBJ_FLAGGED(obj, ITEM_NORENT))
        ch->sendText("That isn't worth bagging. Better keep that close if you wanna keep it at all.\r\n");
    else
    {
        obj->clearLocation();
        cont->addToInventory(obj);

        if (!OBJ_FLAGGED(obj, ITEM_CARD))
        {
            act("$n puts $p in $P.", true, ch, obj, cont, TO_ROOM);
        }
        else
        {
            act("$n puts an @DA@wd@cv@Ce@Wnt @DD@wu@ce@Cl @mC@Ma@Wr@wd@n in $P.", true, ch, obj, cont, TO_ROOM);
        }

        /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
        if (OBJ_FLAGGED(obj, ITEM_NODROP) && !OBJ_FLAGGED(cont, ITEM_NODROP))
        {
            cont->item_flags.set(ITEM_NODROP, true);
            act("You get a strange feeling as you put $p in $P.", false,
                ch, obj, cont, TO_CHAR);
        }
        else
            act("You put $p in $P.", false, ch, obj, cont, TO_CHAR);
        /* If object placed in portal or vehicle, move it to the portal destination */
        if ((GET_OBJ_TYPE(cont) == ITEM_PORTAL) ||
            (GET_OBJ_TYPE(cont) == ITEM_VEHICLE))
        {
            obj->clearLocation();
            obj->moveToLocation(GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY));
            if (GET_OBJ_TYPE(cont) == ITEM_PORTAL)
            {
                act("What? $U$p disappears from $P in a puff of smoke!",
                    true, ch, obj, cont, TO_ROOM);
                act("What? $U$p disappears from $P in a puff of smoke!",
                    false, ch, obj, cont, TO_CHAR);
            }
        }
    }
}

/* The following put modes are supported by the code below:

    1) put <object> <container>
    2) put all.<object> <container>
    3) put all <container>

    <container> must be in inventory or on ground.
    all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    Object *obj, *next_obj, *cont;
    Character *tmp_char;
    int obj_dotmode, cont_dotmode, found = 0, howmany = 1;
    char *theobj, *thecont;

    one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments */

    if (!HAS_ARMS(ch))
    {
        ch->sendText("You have no arms!\r\n");
        return;
    }

    if (*arg3 && is_number(arg1))
    {
        howmany = atoi(arg1);
        theobj = arg2;
        thecont = arg3;
    }
    else
    {
        theobj = arg1;
        thecont = arg2;
    }
    obj_dotmode = find_all_dots(theobj);
    cont_dotmode = find_all_dots(thecont);

    if (!*theobj)
        ch->sendText("Put what in what?\r\n");
    else if (cont_dotmode != FIND_INDIV)
        ch->sendText("You can only put things into one container at a time.\r\n");
    else if (!*thecont)
    {
        ch->send_to("What do you want to put %s in?\r\n", obj_dotmode == FIND_INDIV ? "it" : "them");
    }
    else
    {
        generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
        if (!cont)
            ch->send_to("You don't see %s %s here.\r\n", AN(thecont), thecont);
        else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) &&
                 (GET_OBJ_TYPE(cont) != ITEM_PORTAL) &&
                 (GET_OBJ_TYPE(cont) != ITEM_VEHICLE))
            act("$p is not a container.", false, ch, cont, nullptr, TO_CHAR);
        else if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
            ch->sendText("You'd better open it first!\r\n");
        else
        {
            if (obj_dotmode == FIND_INDIV)
            { /* put <obj> <container> */
                auto con = ch->getInventory();
                int transferred = 0;
                for (auto obj : filter_raw(con))
                {
                    if ((ch->canSee(obj) || GET_OBJ_TYPE(obj) == ITEM_LIGHT) && isname(theobj, obj->getName()))
                    {
                        if (obj == cont)
                        {
                            ch->sendText("You attempt to fold it into itself, but fail.\r\n");
                            continue;
                        }
                        if (transferred >= howmany)
                            break;
                        transferred++;
                        perform_put(ch, obj, cont);
                    }
                }
                if (!transferred)
                {
                    ch->send_to("You aren't carrying %s %s.\r\n", AN(theobj), theobj);
                }
            }
            else
            {
                auto con = ch->getInventory();
                for (auto obj : filter_raw(con))
                {
                    if (obj != cont && ch->canSee(obj) && (obj_dotmode == FIND_ALL || isname(theobj, obj->getName())))
                    {
                        found = 1;
                        perform_put(ch, obj, cont);
                    }
                }
                if (!found)
                {
                    if (obj_dotmode == FIND_ALL)
                        ch->sendText("You don't seem to have anything to put in it.\r\n");
                    else
                        ch->send_to("You don't seem to have any %ss.\r\n", theobj);
                }
            }
        }
    }
}

static int can_take_obj(Character *ch, Object *obj)
{
    if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)))
    {
        act("$p: you can't take that!", false, ch, obj, nullptr, TO_CHAR);
        return (0);
    }
    else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    {
        act("$p: your arms are full!", false, ch, obj, nullptr, TO_CHAR);
        return (0);
    }
    else if (!ch->canCarryWeight(obj))
    {
        act("$p: you can't carry that much weight.", false, ch, obj, nullptr, TO_CHAR);
        return (0);
    }
    return (1);
}

static void get_check_money(Character *ch, Object *obj)
{
    int value = GET_OBJ_VAL(obj, VAL_MONEY_SIZE);

    if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
        return;

    if (GET_GOLD(ch) + value > GOLD_CARRY(ch))
    {
        ch->send_to("You can only carry %s zenni at your current level, and leave the rest.\r\n", add_commas(GOLD_CARRY(ch)).c_str());
        act("@w$n @wdrops some onto the ground.@n", false, ch, nullptr, nullptr, TO_ROOM);
        extract_obj(obj);
        int diff = 0;
        diff = (GET_GOLD(ch) + value) - GOLD_CARRY(ch);
        obj = create_money(diff);
        obj->moveToLocation(ch);
        ch->setBaseStat("money_carried", GOLD_CARRY(ch));
        return;
    }

    ch->modBaseStat("money_carried", value);
    extract_obj(obj);

    if (value == 1)
    {
        ch->sendText("There was 1 zenni.\r\n");
    }
    else
    {
        ch->send_to("There were %d zenni.\r\n", value);
        if (IS_AFFECTED(ch, AFF_GROUP) && PRF_FLAGGED(ch, PRF_AUTOSPLIT))
        {
            char split[MAX_INPUT_LENGTH];
            sprintf(split, "%d", value);
            do_split(ch, split, 0, 0);
        }
    }
}

static void perform_get_from_container(Character *ch, Object *obj,
                                       Object *cont, int mode)
{
    if (mode == FIND_OBJ_INV || mode == FIND_OBJ_EQUIP || can_take_obj(ch, obj))
    {
        if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
        {
            act("$p: you can't hold any more items.", false, ch, obj, nullptr, TO_CHAR);
            return;
        }
        if (SITS(ch) && GET_OBJ_VNUM(SITS(ch)) > 603 && GET_OBJ_VNUM(SITS(ch)) < 608 &&
            GET_OBJ_VNUM(SITS(ch)) - 4 != GET_OBJ_VNUM(cont) && GET_OBJ_VNUM(cont) > 599 && GET_OBJ_VNUM(cont) < 604)
        {
            ch->sendText("You aren't playing at that table!\r\n");
            return;
        }
        else if (get_otrigger(obj, ch))
        {
            obj->clearLocation();
            ch->addToInventory(obj);
            if (OBJ_FLAGGED(cont, ITEM_SHEATH))
            {
                act("You draw $p from $P.", false, ch, obj, cont, TO_CHAR);
                act("$n draws $p from $P.", true, ch, obj, cont, TO_ROOM);
            }
            else
            {
                act("You get $p from $P.", false, ch, obj, cont, TO_CHAR);
                act("$n gets $p from $P.", true, ch, obj, cont, TO_ROOM);
            }
            if (OBJ_FLAGGED(obj, ITEM_HOT))
            {
                if (GET_BONUS(ch, BONUS_FIREPROOF) <= 0 && !IS_DEMON(ch))
                {
                    ch->modCurVitalDam(CharVital::health, .25);
                    if (GET_BONUS(ch, BONUS_FIREPRONE) > 0)
                        ch->modCurVitalDam(CharVital::health, 1);

                    ch->affect_flags.set(AFF_BURNED, true);
                    act("@RYou are burned by it!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    act("@R$n@R is burned by it!@n", true, ch, nullptr, nullptr, TO_ROOM);
                }
            }
            if (IS_NPC(ch))
            {
                item_check(obj, ch);
            }
            get_check_money(ch, obj);
        }
    }
}

static void get_from_container(Character *ch, Object *cont,
                               char *arg, int mode, int howmany)
{
    Object *obj, *next_obj;
    int obj_dotmode, found = 0;

    obj_dotmode = find_all_dots(arg);

    if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
        act("$p is closed.", false, ch, cont, nullptr, TO_CHAR);
    else if (obj_dotmode == FIND_INDIV)
    {
        auto con = cont->getInventory();
        int transferred = 0;
        for (auto obj : filter_raw(con))
        {
            if (ch->canSee(obj) && isname(arg, obj->getName()))
            {
                if (transferred >= howmany)
                    break;
                transferred++;
                perform_get_from_container(ch, obj, cont, mode);
            }
        }
        if (!transferred)
        {
            char buf[MAX_STRING_LENGTH];
            snprintf(buf, sizeof(buf), "There doesn't seem to be %s %s in $p.", AN(arg), arg);
            act(buf, false, ch, cont, nullptr, TO_CHAR);
        }
    }
    else
    {
        if (obj_dotmode == FIND_ALLDOT && !*arg)
        {
            ch->sendText("Get all of what?\r\n");
            return;
        }
        auto con = cont->getInventory();
        for (auto obj : filter_raw(con))
        {
            if (ch->canSee(obj) && (obj_dotmode == FIND_ALL || isname(arg, obj->getName())))
            {
                found = 1;
                perform_get_from_container(ch, obj, cont, mode);
            }
        }
        if (!found)
        {
            if (obj_dotmode == FIND_ALL)
                act("$p seems to be empty.", false, ch, cont, nullptr, TO_CHAR);
            else
            {
                char buf[MAX_STRING_LENGTH];

                snprintf(buf, sizeof(buf), "You can't seem to find any %ss in $p.", arg);
                act(buf, false, ch, cont, nullptr, TO_CHAR);
            }
        }
    }
}

int perform_get_from_room(Character *ch, Object *obj)
{

    if (SITTING(obj))
    {
        ch->sendText("Someone is on that!\r\n");
        return (0);
    }

    if (OBJ_FLAGGED(obj, ITEM_BURIED))
    {
        ch->sendText("Get what?\r\n");
        return (0);
    }

    if (ch->location.getRoomFlag(ROOM_GARDEN1) || ch->location.getRoomFlag(ROOM_GARDEN2))
    {
        ch->sendText("You can't get things from a garden. Help garden.\r\n");
        return (0);
    }

    if (can_take_obj(ch, obj) && get_otrigger(obj, ch))
    {
        obj->clearLocation();
        ch->addToInventory(obj);
        act("You get $p.", false, ch, obj, nullptr, TO_CHAR);
        act("$n gets $p.", true, ch, obj, nullptr, TO_ROOM);

        if (OBJ_FLAGGED(obj, ITEM_HOT))
        {
            if (GET_BONUS(ch, BONUS_FIREPROOF) <= 0 && !IS_DEMON(ch))
            {
                ch->modCurVitalDam(CharVital::health, .25);
                if (GET_BONUS(ch, BONUS_FIREPRONE) > 0)
                    ch->modCurVitalDam(CharVital::health, 1);

                ch->affect_flags.set(AFF_BURNED, true);
                act("@RYou are burned by it!@n", true, ch, nullptr, nullptr, TO_CHAR);
                act("@R$n@R is burned by it!@n", true, ch, nullptr, nullptr, TO_ROOM);
            }
        }

        if (IS_NPC(ch))
            item_check(obj, ch);
        get_check_money(ch, obj);
    }
    return (0);
}

static void get_from_room(Character *ch, char *arg, int howmany)
{
    Object *obj, *next_obj;
    int dotmode, found = 0;
    char *descword;
    auto exd = ch->location.getExtraDescription();

    /* Are they trying to take something in a room extra description? */
    if (find_exdesc(arg, exd))
    {
        ch->send_to("You can't take %s %s.\r\n", AN(arg), arg);
        return;
    }

    dotmode = find_all_dots(arg);

    if (dotmode == FIND_INDIV)
    {
        if (auto dw = find_exdesc(arg, exd))
        {
            ch->send_to("%s: you can't take that!\r\n", dw->first);
            return;
        }
        auto con = ch->location.getObjects();
        int transferred = 0;
        for (auto obj : filter_raw(con))
        {
            if (ch->canSee(obj) && isname(arg, obj->getName()))
            {
                if (transferred >= howmany)
                    break;
                transferred++;
                perform_get_from_room(ch, obj);
            }
        }
        if (!transferred)
        {
            ch->send_to("You don't see %s %s here.\r\n", AN(arg), arg);
        }
    }
    else
    {
        if (dotmode == FIND_ALLDOT && !*arg)
        {
            ch->sendText("Get all of what?\r\n");
            return;
        }
        auto loco = ch->location.getObjects();
        for (auto obj : filter_raw(loco))
        {
            if (ch->canSee(obj) && (dotmode == FIND_ALL || isname(arg, obj->getName())))
            {
                found = 1;
                perform_get_from_room(ch, obj);
            }
        }
        if (!found)
        {
            if (dotmode == FIND_ALL)
                ch->sendText("There doesn't seem to be anything here.\r\n");
            else
                ch->send_to("You don't see any %ss here.\r\n", arg);
        }
    }
}

ACMD(do_get)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];

    int cont_dotmode, found = 0, mode;
    Object *cont;
    Character *tmp_char;

    one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments */

    if (!HAS_ARMS(ch))
    {
        ch->sendText("You have no arms!\r\n");
        return;
    }

    if (!*arg1)
        ch->sendText("Get what?\r\n");
    else if (!*arg2)
        get_from_room(ch, arg1, 1);
    else if (is_number(arg1) && !*arg3)
        get_from_room(ch, arg2, atoi(arg1));
    else
    {
        int amount = 1;
        if (is_number(arg1))
        {
            amount = atoi(arg1);
            strcpy(arg1, arg2); /* strcpy: OK (sizeof: arg1 == arg2) */
            strcpy(arg2, arg3); /* strcpy: OK (sizeof: arg2 == arg3) */
        }
        cont_dotmode = find_all_dots(arg2);
        if (cont_dotmode == FIND_INDIV)
        {
            mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
            if (!cont)
                ch->send_to("You don't have %s %s.\r\n", AN(arg2), arg2);
            else if (GET_OBJ_TYPE(cont) == ITEM_VEHICLE)
                ch->sendText("You will need to enter it first.\r\n");
            else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) &&
                     !((GET_OBJ_TYPE(cont) == ITEM_PORTAL) && (OBJVAL_FLAGGED(cont, CONT_CLOSEABLE))))
                act("$p is not a container.", false, ch, cont, nullptr, TO_CHAR);
            else
                get_from_container(ch, cont, arg1, mode, amount);
        }
        else
        {
            if (cont_dotmode == FIND_ALLDOT && !*arg2)
            {
                ch->sendText("Get from all of what?\r\n");
                return;
            }
            auto con = ch->getInventory();
            for (auto cont : filter_raw(con))
                if (ch->canSee(cont) &&
                    (cont_dotmode == FIND_ALL || isname(arg2, cont->getName())))
                {
                    if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
                    {
                        found = 1;
                        get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount);
                    }
                    else if (cont_dotmode == FIND_ALLDOT)
                    {
                        found = 1;
                        act("$p is not a container.", false, ch, cont, nullptr, TO_CHAR);
                    }
                }
            auto loco = ch->location.getObjects();
            for (auto cont : filter_raw(loco))
                if (ch->canSee(cont) &&
                    (cont_dotmode == FIND_ALL || isname(arg2, cont->getName())))
                {
                    if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
                    {
                        get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount);
                        found = 1;
                    }
                    else if (cont_dotmode == FIND_ALLDOT)
                    {
                        act("$p is not a container.", false, ch, cont, nullptr, TO_CHAR);
                        found = 1;
                    }
                }
            if (!found)
            {
                if (cont_dotmode == FIND_ALL)
                    ch->sendText("You can't seem to find any containers.\r\n");
                else
                    ch->send_to("You can't seem to find any %ss here.\r\n", arg2);
            }
        }
    }
}

static void perform_drop_gold(Character *ch, int amount,
                              int8_t mode, room_rnum RDR)
{
    Object *obj;

    if (amount <= 0)
        ch->sendText("Heh heh heh.. we are jolly funny today, eh?\r\n");
    else if (GET_GOLD(ch) < amount)
        ch->sendText("You don't have that many zenni!\r\n");
    else
    {
        if (mode != SCMD_JUNK)
        {
            WAIT_STATE(ch, PULSE_1SEC); /* to prevent zenni-bombing */
            obj = create_money(amount);
            if (mode == SCMD_DONATE)
            {
                ch->sendText("You throw some zenni into the air where it disappears in a puff of smoke!\r\n");
                act("$n throws some gold into the air where it disappears in a puff of smoke!",
                    false, ch, nullptr, nullptr, TO_ROOM);
                obj->moveToLocation(RDR);
                act("$p suddenly appears in a puff of orange smoke!", 0, nullptr, obj, nullptr, TO_ROOM);
            }
            else
            {
                char buf[MAX_STRING_LENGTH];

                if (!drop_wtrigger(obj, ch))
                {
                    extract_obj(obj);
                    return;
                }

                if (!drop_wtrigger(obj, ch) && (obj))
                { /* obj may be purged */
                    extract_obj(obj);
                    return;
                }

                snprintf(buf, sizeof(buf), "$n drops %s.", money_desc(amount));
                act(buf, true, ch, nullptr, nullptr, TO_ROOM);

                ch->sendText("You drop some zenni.\r\n");
                obj->moveToLocation(ch);
                if (GET_ADMLEVEL(ch) > 0)
                {
                    send_to_imm("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch), obj->getShortDescription(),
                                obj->location.getVnum());
                    log_imm_action("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch), obj->getShortDescription(),
                                   obj->location.getVnum());
                    if (check_insidebag(obj, 0.0) > 1)
                    {
                        send_to_imm("IMM DROP: Object contains %d other items.", check_insidebag(obj, 0.0));
                        log_imm_action("IMM DROP: Object contains %d other items.", check_insidebag(obj, 0.0));
                    }
                }
            }
        }
        else
        {
            char buf[MAX_STRING_LENGTH];

            snprintf(buf, sizeof(buf), "$n drops %s which disappears in a puff of smoke!", money_desc(amount));
            act(buf, false, ch, nullptr, nullptr, TO_ROOM);

            ch->sendText("You drop some zenni which disappears in a puff of smoke!\r\n");
        }
        ch->modBaseStat("money_carried", -amount);
    }
}

#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? "  It vanishes in a puff of smoke!" : "")

static int perform_drop(Character *ch, Object *obj,
                        int8_t mode, const char *sname, room_rnum RDR)
{
    char buf[MAX_STRING_LENGTH];
    int value;

    if (!drop_otrigger(obj, ch))
        return 0;

    if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
        return 0;

    if (GET_OBJ_VNUM(obj) == 17 || GET_OBJ_VNUM(obj) == 17998)
    {
        snprintf(buf, sizeof(buf), "You can't %s $p, it is grafted into your soul :P", sname);
        act(buf, false, ch, obj, nullptr, TO_CHAR);
        return (0);
    }
    if (GET_OBJ_VNUM(obj) == 20 || GET_OBJ_VNUM(obj) == 21 || GET_OBJ_VNUM(obj) == 22 || GET_OBJ_VNUM(obj) == 23 ||
        GET_OBJ_VNUM(obj) == 24 || GET_OBJ_VNUM(obj) == 25 || GET_OBJ_VNUM(obj) == 26)
    {
        if (ch->location.getWhereFlag(WhereFlag::space))
        {
            snprintf(buf, sizeof(buf), "You can't %s $p in space!", sname);
            act(buf, false, ch, obj, nullptr, TO_CHAR);
            return (0);
        }
        if (ch->location.getRoomFlag(ROOM_GARDEN1) || ch->location.getRoomFlag(ROOM_GARDEN2))
        {
            snprintf(buf, sizeof(buf), "You can't %s $p in here. Read help garden.", sname);
            act(buf, false, ch, obj, nullptr, TO_CHAR);
            return (0);
        }
        if ((mode == SCMD_DROP) && OBJ_FLAGGED(obj, ITEM_NORENT))
        {
            snprintf(buf, sizeof(buf), "You drop $p but it gets lost on the ground!");
            act(buf, false, ch, obj, nullptr, TO_CHAR);
            obj->clearLocation();
            extract_obj(obj);
            return (0);
        }
        if (ch->location.getRoomFlag(ROOM_NOINSTANT))
        {
            snprintf(buf, sizeof(buf), "You can't %s $p in this protected area!", sname);
            act(buf, false, ch, obj, nullptr, TO_CHAR);
            return (0);
        }
        if (ch->location.getRoomFlag(ROOM_SHIP))
        {
            snprintf(buf, sizeof(buf), "You can't %s $p on a private ship!", sname);
            act(buf, false, ch, obj, nullptr, TO_CHAR);
            return (0);
        }
        if (ch->location.getRoomFlag(ROOM_HOUSE))
        {
            snprintf(buf, sizeof(buf), "You can't %s $p in a private house!", sname);
            act(buf, false, ch, obj, nullptr, TO_CHAR);
            return (0);
        }
    }
    if (OBJ_FLAGGED(obj, ITEM_NODROP) && GET_ADMLEVEL(ch) < 1)
    {
        snprintf(buf, sizeof(buf), "You can't %s $p, it must be CURSED!", sname);
        act(buf, false, ch, obj, nullptr, TO_CHAR);
        return (0);
    }
    if ((mode == SCMD_DONATE || mode == SCMD_JUNK) && OBJ_FLAGGED(obj, ITEM_NODONATE))
    {
        snprintf(buf, sizeof(buf), "You can't %s $p!", sname);
        act(buf, false, ch, obj, nullptr, TO_CHAR);
        return (0);
    }

    snprintf(buf, sizeof(buf), "You %s $p.", sname);
    act(buf, false, ch, obj, nullptr, TO_CHAR);

    snprintf(buf, sizeof(buf), "$n %ss $p.", sname);
    act(buf, true, ch, obj, nullptr, TO_ROOM);

    obj->clearLocation();

    switch (mode)
    {
    case SCMD_DROP:
        if (!OBJ_FLAGGED(obj, ITEM_UNBREAKABLE) && ch->location.getGroundEffect() == 6)
        {
            act("$p melts in the lava!", false, ch, obj, nullptr, TO_CHAR);
            act("$p melts in the lava!", false, ch, obj, nullptr, TO_ROOM);
            extract_obj(obj);
        }
        else if (ch->location.getGroundEffect() == 6)
        {
            act("$p plops down on some cooled lava!", false, ch, obj, nullptr, TO_CHAR);
            act("$p plops down on some cooled lava!", false, ch, obj, nullptr, TO_ROOM);
            obj->moveToLocation(ch);
            if (GET_ADMLEVEL(ch) > 0)
            {
                send_to_imm("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch), obj->getShortDescription(),
                            obj->location.getVnum());
                log_imm_action("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch), obj->getShortDescription(),
                               obj->location.getVnum());
            }
        }
        else
        {
            obj->moveToLocation(ch);
            if (GET_ADMLEVEL(ch) > 0)
            {
                send_to_imm("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch), obj->getShortDescription(),
                            obj->location.getVnum());
                log_imm_action("IMM DROP: %s dropped %s in room [%d]", GET_NAME(ch), obj->getShortDescription(),
                               obj->location.getVnum());
            }
        }
        return (0);
    case SCMD_DONATE:
        obj->moveToLocation(RDR);
        act("$p suddenly appears in a puff a smoke!", false, nullptr, obj, nullptr, TO_ROOM);
        return (0);
    case SCMD_JUNK:
        value = std::clamp(GET_OBJ_COST(obj) / 16, 1, 200);
        extract_obj(obj);
        return (value);
    default:
        basic_mud_log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
        /*  SYSERR_DESC:
         *  This error comes from perform_drop() and is output when perform_drop()
         *  is called with an illegal 'mode' argument.
         */
        break;
    }

    return (0);
}

ACMD(do_drop)
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj, *next_obj;
    room_rnum RDR = 0;
    int8_t mode = SCMD_DROP;
    int dotmode, amount = 0, multi, num_don_rooms;
    const char *sname;

    if (!HAS_ARMS(ch))
    {
        ch->sendText("You have no arms!\r\n");
        return;
    }

    if (ch->location.getRoomFlag(ROOM_GARDEN1) || ch->location.getRoomFlag(ROOM_GARDEN2))
    {
        ch->sendText("You can not do that in a garden.\r\n");
        return;
    }

    switch (subcmd)
    {
    case SCMD_JUNK:
        sname = "junk";
        mode = SCMD_JUNK;
        break;
    case SCMD_DONATE:
        sname = "donate";
        mode = SCMD_DONATE;
        /* fail + double chance for room 1   */
        num_don_rooms = (CONFIG_DON_ROOM_1 != NOWHERE) * 2 +
                        (CONFIG_DON_ROOM_2 != NOWHERE) +
                        (CONFIG_DON_ROOM_3 != NOWHERE) + 1;
        switch (Random::get<int>(0, num_don_rooms))
        {
        case 0:
            mode = SCMD_JUNK;
            break;
        case 1:
        case 2:
            RDR = real_room(CONFIG_DON_ROOM_1);
            break;
        case 3:
            RDR = real_room(CONFIG_DON_ROOM_2);
            break;
        case 4:
            RDR = real_room(CONFIG_DON_ROOM_3);
            break;
        }
        if (RDR == NOWHERE)
        {
            ch->sendText("Sorry, you can't donate anything right now.\r\n");
            return;
        }
        break;
    default:
        sname = "drop";
        break;
    }

    argument = one_argument(argument, arg);

    if (!*arg)
    {
        ch->send_to("What do you want to %s?\r\n", sname);
        return;
    }
    else if (is_number(arg))
    {
        multi = atoi(arg);
        one_argument(argument, arg);
        if (boost::iequals("zenni", arg) || boost::iequals("gold", arg))
            perform_drop_gold(ch, multi, mode, RDR);
        else if (multi <= 0)
            ch->sendText("Yeah, that makes sense.\r\n");
        else if (!*arg)
            ch->send_to("What do you want to %s %d of?\r\n", sname, multi);
        else
        {
            auto con = ch->getInventory();
            for (auto obj : filter_raw(con))
            {
                if (ch->canSee(obj) && isname(arg, obj->getName()))
                {
                    amount += perform_drop(ch, obj, mode, sname, RDR);
                    if (--multi <= 0)
                        break;
                }
            }
            if (!amount)
            {
                ch->send_to("You don't seem to have any %ss.\r\n", arg);
            }
        }
    }
    else
    {
        dotmode = find_all_dots(arg);

        /* Can't junk or donate all */
        if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE))
        {
            if (subcmd == SCMD_JUNK)
                ch->sendText("Go to the dump if you want to junk EVERYTHING!\r\n");
            else
                ch->sendText("Go do the donation room if you want to donate EVERYTHING!\r\n");
            return;
        }
        if (dotmode == FIND_ALL)
        {
            int fail = false;
            auto con = ch->getInventory();
            if (con.empty())
                ch->sendText("You don't seem to be carrying anything.\r\n");
            else
            {
                for (auto obj : filter_raw(con))
                {
                    amount += perform_drop(ch, obj, mode, sname, RDR);
                }
            }
        }
        else if (dotmode == FIND_ALLDOT)
        {
            if (!*arg)
            {
                ch->send_to("What do you want to %s all of?\r\n", sname);
                return;
            }
            auto con = ch->getInventory();
            for (auto obj : filter_raw(con))
            {
                if (isname(arg, obj->getName()))
                    if (ch->canSee(obj) || (GET_OBJ_TYPE(obj) == ITEM_LIGHT))
                    {
                        amount += perform_drop(ch, obj, mode, sname, RDR);
                    }
            }
            if (!amount)
            {
                ch->send_to("You don't seem to have any %ss.\r\n", arg);
            }
        }
        else
        {
            if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
            {
                ch->send_to("You don't seem to have %s %s.\r\n", AN(arg), arg);
            }
            else
            {
                amount += perform_drop(ch, obj, mode, sname, RDR);
            }
        }
    }
}

static void perform_give(Character *ch, Character *vict,
                         Object *obj)
{
    if (!give_otrigger(obj, ch, vict))
        return;
    if (!receive_mtrigger(vict, ch, obj))
        return;

    if (OBJ_FLAGGED(obj, ITEM_NODROP))
    {
        act("You can't let go of $p!!  Yeech!", false, ch, obj, nullptr, TO_CHAR);
        return;
    }
    if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict))
    {
        act("$N seems to have $S hands full.", false, ch, nullptr, vict, TO_CHAR);
        if (IS_NPC(ch))
        {
            act("$n@n drops $p because you can't carry anymore.", true, ch, obj, vict, TO_VICT);
            act("$n@n drops $p on the ground since $N's unable to carry it.", true, ch, obj, vict, TO_NOTVICT);
            obj->clearLocation();
            obj->moveToLocation(ch);
        }
        return;
    }
    if (IS_NPC(vict) && (OBJ_FLAGGED(obj, ITEM_FORGED) || OBJ_FLAGGED(obj, ITEM_FORGED)))
    {
        act("$n tries to hand $p to $N.", true, ch, obj, vict, TO_NOTVICT);
        do_say(vict, "I don't want that piece of junk.", 0, 0);
        return;
    }
    if (!vict->canCarryWeight(obj))
    {
        act("$E can't carry that much weight.", false, ch, nullptr, vict, TO_CHAR);
        if (IS_NPC(ch))
        {
            act("$n@n drops $p because you can't carry anymore.", true, ch, obj, vict, TO_VICT);
            act("$n@n drops $p on the ground since $N's unable to carry it.", true, ch, obj, vict, TO_NOTVICT);
            obj->clearLocation();
            obj->moveToLocation(ch);
        }
        return;
    }
    if (!vict->canCarryWeight(obj))
    {
        act("$E can't carry that much weight because of the gravity.", false, ch, nullptr, vict, TO_CHAR);
        if (IS_NPC(ch))
        {
            act("$n@n drops $p because you can't carry anymore.", true, ch, obj, vict, TO_VICT);
            act("$n@n drops $p on the ground since $N's unable to carry it.", true, ch, obj, vict, TO_NOTVICT);
            obj->clearLocation();
            obj->moveToLocation(ch);
        }
        return;
    }
    if (!IS_NPC(vict) && !IS_NPC(ch))
    {
        if (PRF_FLAGGED(vict, PRF_NOGIVE))
        {
            act("$N refuses to accept $p at this time.", false, ch, obj, vict, TO_CHAR);
            act("$n tries to give you $p, but you are refusing to be handed things.", false, ch, obj, vict, TO_VICT);
            act("$n tries to give $N, $p, but $E is refusing to be handed things right now.", false, ch, obj, vict,
                TO_NOTVICT);
            return;
        }
    }
    obj->clearLocation();
    vict->addToInventory(obj);
    act("You give $p to $N.", false, ch, obj, vict, TO_CHAR);
    act("$n gives you $p.", false, ch, obj, vict, TO_VICT);
    act("$n gives $p to $N.", true, ch, obj, vict, TO_NOTVICT);

    if (OBJ_FLAGGED(obj, ITEM_HOT))
    {
        if (GET_BONUS(vict, BONUS_FIREPROOF) <= 0 && !IS_DEMON(vict))
        {
            ch->modCurVitalDam(CharVital::health, .25);
            if (GET_BONUS(vict, BONUS_FIREPRONE) > 0)
                ch->modCurVitalDam(CharVital::health, 1);

            vict->affect_flags.set(AFF_BURNED, true);
            act("@RYou are burned by it!@n", true, vict, nullptr, nullptr, TO_CHAR);
            act("@R$n@R is burned by it!@n", true, vict, nullptr, nullptr, TO_ROOM);
        }
    }
}

/* utility function for give */
static Character *give_find_vict(Character *ch, char *arg)
{
    Character *vict;

    skip_spaces(&arg);
    if (!*arg)
        ch->sendText("To who?\r\n");
    else if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->send_to("%s", CONFIG_NOPERSON);
        if (IS_NPC(ch))
            send_to_imm("Mob Give: Victim, %s, doesn't exist.", arg);
    }
    else if (vict == ch)
        ch->sendText("What's the point of that?\r\n");
    else
        return (vict);

    return (nullptr);
}

static void perform_give_gold(Character *ch, Character *vict,
                              int amount)
{
    char buf[MAX_STRING_LENGTH];

    if (amount <= 0)
    {
        ch->sendText("Heh heh heh ... we are jolly funny today, eh?\r\n");
        return;
    }
    if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_MONEY)))
    {
        ch->sendText("You don't have that much zenni!\r\n");
        return;
    }
    if (GET_GOLD(vict) + amount > GOLD_CARRY(vict))
    {
        ch->sendText("They can't carry that much zenni.\r\n");
        return;
    }
    ch->send_to("%s", CONFIG_OK);

    snprintf(buf, sizeof(buf), "$n gives you %d zenni.", amount);
    act(buf, false, ch, nullptr, vict, TO_VICT);

    snprintf(buf, sizeof(buf), "$n gives %s to $N.", money_desc(amount));
    act(buf, true, ch, nullptr, vict, TO_NOTVICT);

    if (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_MONEY))
        ch->modBaseStat("money_carried", -amount);
    vict->modBaseStat("money_carried", amount);

    bribe_mtrigger(vict, ch, amount);
}

ACMD(do_give)
{
    char arg[MAX_STRING_LENGTH];
    int amount, dotmode;
    Character *vict;
    Object *obj, *next_obj;

    argument = one_argument(argument, arg);

    if (!HAS_ARMS(ch))
    {
        ch->sendText("You have no arms!\r\n");
        return;
    }

    reveal_hiding(ch, 0);
    if (!*arg)
        ch->sendText("Give what to who?\r\n");
    else if (is_number(arg))
    {
        amount = atoi(arg);
        argument = one_argument(argument, arg);
        if (boost::iequals("zenni", arg) || boost::iequals("gold", arg))
        {
            one_argument(argument, arg);
            if ((vict = give_find_vict(ch, arg)))
            {
                perform_give_gold(ch, vict, amount);
                if (GET_ADMLEVEL(ch) > 0 && !IS_NPC(vict))
                {
                    send_to_imm("IMM GIVE: %s has given %s zenni to %s.", GET_NAME(ch), add_commas(amount).c_str(),
                                GET_NAME(vict));
                    log_imm_action("IMM GIVE: %s has given %s zenni to %s.", GET_NAME(ch), add_commas(amount).c_str(),
                                   GET_NAME(vict));
                }
            }
            return;
        }
        else if (!*arg) /* Give multiple code. */
            ch->send_to("What do you want to give %d of?\r\n", amount);
        else if (!(vict = give_find_vict(ch, argument)))
            return;

        auto con = ch->getInventory();
        int given = 0;
        for (auto obj : filter_raw(con))
        {
            if (ch->canSee(obj) || GET_OBJ_TYPE(obj) == ITEM_LIGHT && isname(arg, obj->getName()))
            {
                perform_give(ch, vict, obj);
                if (GET_ADMLEVEL(ch) > 0 && !IS_NPC(vict))
                {
                    send_to_imm("IMM GIVE: %s has given %s to %s.", GET_NAME(ch), obj->getShortDescription(),
                                GET_NAME(vict));
                    log_imm_action("IMM GIVE: %s has given %s to %s.", GET_NAME(ch), obj->getShortDescription(),
                                   GET_NAME(vict));
                }
                given++;
                if (given >= amount)
                    break;
            }
        }
        if (!given)
        {
            ch->send_to("You don't seem to have any %ss.\r\n", arg);
        }
    }
    else
    {
        char buf1[MAX_INPUT_LENGTH];

        one_argument(argument, buf1);
        if (!(vict = give_find_vict(ch, buf1)))
            return;
        dotmode = find_all_dots(arg);
        if (dotmode == FIND_INDIV)
        {
            if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
                ch->send_to("You don't seem to have %s %s.\r\n", AN(arg), arg);
            else
            {
                perform_give(ch, vict, obj);
                if (GET_ADMLEVEL(ch) > 0 && !IS_NPC(vict))
                {
                    send_to_imm("IMM GIVE: %s has given %s to %s.", GET_NAME(ch), obj->getShortDescription(),
                                GET_NAME(vict));
                    log_imm_action("IMM GIVE: %s has given %s to %s.", GET_NAME(ch), obj->getShortDescription(),
                                   GET_NAME(vict));
                }
            }
        }
        else
        {
            if (dotmode == FIND_ALLDOT && !*arg)
            {
                ch->sendText("All of what?\r\n");
                return;
            }
            auto con = ch->getInventory();
            if (con.empty())
                ch->sendText("You don't seem to be holding anything.\r\n");
            else
            {
                for (auto obj : filter_raw(con))
                {
                    if (ch->canSee(obj) &&
                        ((dotmode == FIND_ALL || isname(arg, obj->getName()))))
                    {
                        perform_give(ch, vict, obj);
                        if (GET_ADMLEVEL(ch) > 0 && !IS_NPC(vict))
                        {
                            send_to_imm("IMM GIVE: %s has given %s to %s.", GET_NAME(ch), obj->getShortDescription(),
                                        GET_NAME(vict));
                            log_imm_action("IMM GIVE: %s has given %s to %s.", GET_NAME(ch), obj->getShortDescription(),
                                           GET_NAME(vict));
                        }
                    }
                }
            }
        }
    }
}

void weight_change_object(Object *obj, int weight)
{
    obj->modBaseStat<weight_t>("weight", weight);
}

void name_from_drinkcon(Object *obj)
{
    if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
        return;

    std::string name = obj->getName(); // copy the name
    std::string liqname = drinknames[GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)];

    std::istringstream iss(name);
    std::ostringstream oss;
    std::string word;
    bool first = true;

    while (iss >> word)
    {
        if (!std::equal(word.begin(), word.end(),
                        liqname.begin(), liqname.end(),
                        [](char a, char b)
                        { return std::tolower(a) == std::tolower(b); }))
        {
            if (!first)
                oss << " ";
            oss << word;
            first = false;
        }
    }

    obj->name = oss.str();
}

void name_to_drinkcon(Object *obj, int type)
{
    char *new_name;

    if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
        return;

    CREATE(new_name, char, strlen(obj->getName()) + strlen(drinknames[type]) + 2);
    sprintf(new_name, "%s %s", obj->getName(), drinknames[type]); /* sprintf: OK */

    obj->name = new_name;
    free(new_name);
}

ACMD(do_drink)
{
    char arg[MAX_INPUT_LENGTH];
    Object *temp;
    struct affected_type af;
    int amount, weight, wasthirsty = 0;
    int on_ground = 0;

    one_argument(argument, arg);
    if (IS_NPC(ch))
    {
        return;
    }
    if (IS_ANDROID(ch) || GET_COND(ch, THIRST) < 0)
    {
        ch->sendText("You need not drink!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }
    if (GET_COND(ch, HUNGER) <= 1 && GET_COND(ch, THIRST) >= 2 && !IS_NAMEK(ch) && !ch->bio_genomes.get(Race::namekian))
    {
        ch->sendText("You need to eat first!\r\n");
        return;
    }
    wasthirsty = GET_COND(ch, THIRST);
    if (!*arg && !IS_NPC(ch))
    {
        char buf[MAX_STRING_LENGTH];
        switch (ch->location.getTileType())
        {
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_UNDERWATER:
            snprintf(buf, sizeof(buf), "$n takes a refreshing drink from the surrounding water.");
            act(buf, true, ch, nullptr, nullptr, TO_ROOM);
            ch->sendText("You take a refreshing drink from the surrounding water.\r\n");
            gain_condition(ch, THIRST, 1);
            if (GET_SKILL(ch, SKILL_WELLSPRING) && (ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) && wasthirsty <= 30 &&
                subcmd != SCMD_SIP)
            {

                ch->modCurVital(CharVital::ki, ((GET_MAX_MANA(ch) * 0.005) + (GET_WIS(ch) * Random::get<int>(80, 100))) *
                                                   GET_SKILL(ch, SKILL_WELLSPRING));

                ch->sendText("You feel your ki return to full strength.\r\n");
            }
            if (GET_COND(ch, THIRST) >= 48)
                ch->sendText("You don't feel thirsty anymore.\r\n");
            return;
        default:
            if (ch->location.getEnvironment(ENV_WATER) < 100.0)
            {
                ch->sendText("Drink from what?\r\n");
                return;
            }
            else
            {
                snprintf(buf, sizeof(buf), "$n takes a refreshing drink from the surrounding water.");
                act(buf, true, ch, nullptr, nullptr, TO_ROOM);
                ch->sendText("You take a refreshing drink from the surrounding water.\r\n");
                gain_condition(ch, THIRST, 1);
                if (GET_SKILL(ch, SKILL_WELLSPRING) && !ch->isFullVital(CharVital::ki) && wasthirsty <= 30 && subcmd != SCMD_SIP)
                {
                    if (ch->modCurVital(CharVital::ki, ((GET_MAX_MANA(ch) * 0.005) + (GET_WIS(ch) * Random::get<int>(80, 100))) *
                                                           GET_SKILL(ch, SKILL_WELLSPRING)) == ch->getEffectiveStat<int64_t>("ki"))
                    {
                        ch->sendText("You feel your ki return to full strength.\r\n");
                    }
                    else
                    {
                        ch->sendText("You feel your ki has rejuvenated.\r\n");
                    }
                }
                if (GET_COND(ch, THIRST) >= 48)
                    ch->sendText("You don't feel thirsty anymore.\r\n");
            }
            return;
            ;
        }
    }
    if (!(temp = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
    {
        if (!(temp = get_obj_in_list_vis(ch, arg, nullptr, ch->location.getObjects())))
        {
            ch->sendText("You can't find it!\r\n");
            return;
        }
        else
            on_ground = 1;
    }
    if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
        (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN))
    {
        if (GET_OBJ_VNUM(temp) == 86 && on_ground != 1)
        {
            act("@wYou uncork the $p and tip it to your lips. Drinking it down you feel a warmth flow through your body and your ki returns to full strength .@n",
                true, ch, temp, nullptr, TO_CHAR);
            act("@C$n@w uncorks the $p and tips it to $s lips. Drinking it down and then smiling.@n", true, ch, temp,
                nullptr, TO_ROOM);
            temp->clearLocation();
            extract_obj(temp);
            ch->restoreVital(CharVital::ki);
            GET_COND(ch, THIRST) += 8;
            return;
        }
        else if (GET_OBJ_VNUM(temp) == 86 && on_ground == 1)
        {
            ch->sendText("You need to pick that up first.\r\n");
            return;
        }
        else
        {
            ch->sendText("You can't drink from that!\r\n");
            return;
        }
    }
    if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON))
    {
        ch->sendText("You have to be holding that to drink from it.\r\n");
        return;
    }
    if (OBJ_FLAGGED(temp, ITEM_BROKEN))
    {
        ch->sendText("Seems like it's broken!\r\n");
        return;
    }
    if (OBJ_FLAGGED(temp, ITEM_FORGED))
    {
        ch->sendText("Seems like it doesn't work, maybe it is fake...\r\n");
        return;
    }
    if (IS_NPC(ch))
    { /* Cannot use GET_COND() on mobs. */
        act("$n@w drinks from $p.", true, ch, temp, nullptr, TO_ROOM);
        temp->clearLocation();
        extract_obj(temp);
        return;
    }
    if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0))
    {
        /* The pig is drunk */
        ch->sendText("You can't seem to get close enough to your mouth.\r\n");
        act("$n tries to drink but misses $s mouth!", true, ch, nullptr, nullptr, TO_ROOM);
        return;
    }
    if (GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL) <= 0 && (GET_OBJ_VAL(temp, VAL_DRINKCON_CAPACITY) >= 1))
    {
        ch->sendText("It's empty.\r\n");
        return;
    }

    if (!consume_otrigger(temp, ch, OCMD_DRINK)) /* check trigger */
        return;

    if (subcmd == SCMD_DRINK)
    {
        char buf[MAX_STRING_LENGTH];

        snprintf(buf, sizeof(buf), "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)]);
        act(buf, true, ch, temp, nullptr, TO_ROOM);

        ch->send_to("You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)]);
        if (auto ld = temp->getLookDescription(); ld)
            act(ld, true, ch, temp, nullptr, TO_CHAR);

        /* if (drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK] > 0)
      amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK];
    else
      amount = Random::get<int>(3, 10);*/

        amount = 4;
    }
    else
    {
        act("$n sips from $p.", true, ch, temp, nullptr, TO_ROOM);
        ch->send_to("It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)]);
        if (GET_OBJ_VAL(temp, VAL_DRINKCON_POISON))
        {
            ch->sendText("It has a sickening taste! Better not eat it...");
            return;
        }
        amount = 1;
    }

    amount = std::min<int>(amount, GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL));

    /* You can't subtract more than the object weighs
     Objects that are eternal (max capacity -1) don't get a
     weight subtracted */
    if (GET_OBJ_VAL(temp, VAL_DRINKCON_CAPACITY) > 0)
    {
        weight = std::min<int>(amount, GET_OBJ_WEIGHT(temp));
        weight_change_object(temp, -weight); /* Subtract amount */
    }

    gain_condition(ch, DRUNK, drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK] * amount);
    gain_condition(ch, HUNGER, drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][HUNGER] * amount);
    gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][THIRST] * amount);
    if (GET_FOODR(ch) == 0 && subcmd != SCMD_SIP)
    {
        ch->modCurVital(CharVital::stamina, (ch->getEffectiveStat<int64_t>("stamina") / 100) * amount);
        ch->setBaseStat("food_rejuvenation", 2);
        ch->sendText("You feel rejuvinated by it.\r\n");
    }
    if (GET_SKILL(ch, SKILL_WELLSPRING) && (ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) && wasthirsty <= 30 &&
        subcmd != SCMD_SIP)
    {
        if (GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID) == 0 || GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID) == 14 ||
            GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID) == 15)
        {

            if (ch->modCurVital(CharVital::ki, ((GET_MAX_MANA(ch) * 0.005) + (GET_WIS(ch) * Random::get<int>(80, 100))) *
                                                   GET_SKILL(ch, SKILL_WELLSPRING)) == ch->getEffectiveStat<int64_t>("ki"))
            {
                ch->sendText("You feel your ki return to full strength.\r\n");
            }
            else
            {
                ch->sendText("You feel your ki has rejuvenated.\r\n");
            }
        }
    }

    if (GET_COND(ch, DRUNK) > 10)
        ch->sendText("You feel drunk.\r\n");

    if (GET_COND(ch, THIRST) >= 48)
        ch->sendText("You don't feel thirsty anymore.\r\n");

    if (GET_OBJ_VAL(temp, VAL_DRINKCON_POISON) &&
        !ch->mutations.get(Mutation::venomous))
    { /* The crap was poisoned
! */
        ch->sendText("Oops, it tasted rather strange!\r\n");
        act("$n chokes and utters some strange sounds.", true, ch, nullptr, nullptr, TO_ROOM);

        af.type = SPELL_POISON;
        af.duration = amount * 3;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.aff_flags.set(AFF_POISON);
        affect_join(ch, &af, false, false, false, false);
    }
    /* empty the container, and no longer poison.
     Only remove if it's max capacity > 0, not eternal */
    if (GET_OBJ_VAL(temp, VAL_DRINKCON_CAPACITY) > 0)
    {
        if (MOD_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL, -amount) <= 0)
        { /* The last bit */
            name_from_drinkcon(temp);
            SET_OBJ_VAL(temp, VAL_DRINKCON_POISON, 0);
        }
    }
    return;
}

ACMD(do_eat)
{
    char arg[MAX_INPUT_LENGTH];
    Object *food;
    struct affected_type af;
    int amount;

    one_argument(argument, arg);

    // Set up the conditions in which you cannot eat
    if (IS_NPC(ch)) /* Cannot use GET_COND() on mobs. */
        return;

    if ((IS_ANDROID(ch) || GET_COND(ch, HUNGER)) < 0 && GET_ADMLEVEL(ch) < 1)
    {
        ch->sendText("You need not eat!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_POISON))
    {
        ch->sendText("You feel too sick from the poison to eat!\r\n");
        return;
    }
    if (!*arg)
    {
        ch->sendText("Eat what?\r\n");
        return;
    }
    if (!(food = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
    {
        ch->send_to("You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
    }
    if (GET_COND(ch, THIRST) <= 1 && GET_COND(ch, HUNGER) >= 2)
    {
        ch->sendText("You need to drink first!\r\n");
        return;
    }
    if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
                                 (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN)))
    {
        do_drink(ch, argument, 0, SCMD_SIP);
        return;
    }
    if ((GET_OBJ_TYPE(food) != ITEM_FOOD))
    {
        ch->sendText("You can't eat THAT!\r\n");
        return;
    }

    // Base how much a player can eat on their con
    float maxCapacity = 48 * ((GET_CON(ch) / 50) + 1);

    // If they're over the limit and not majin they can no longer eat
    if (GET_COND(ch, HUNGER) >= maxCapacity && !IS_MAJIN(ch))
    {
        ch->sendText("You can't stomach to eat any more!\r\n");
        return;
    }

    if (!consume_otrigger(food, ch, OCMD_EAT)) /* check trigger */
        return;

    if (subcmd == SCMD_EAT)
    {
        act("You eat some of $p.", false, ch, food, nullptr, TO_CHAR);
        if (auto ld = food->getLookDescription(); ld)
            act(ld, false, ch, food, nullptr, TO_CHAR);
        act("$n eats some of $p.", true, ch, food, nullptr, TO_ROOM);
    }
    else
    {
        act("You nibble a little bit of $p.", false, ch, food, nullptr, TO_CHAR);
        act("$n tastes a little bit of $p.", true, ch, food, nullptr, TO_ROOM);
        if (GET_OBJ_VAL(food, VAL_FOOD_POISON))
        {
            ch->sendText("It has a sickening taste! Better not eat it...");
            return;
        }
    }

    // Calculate how much hunger the character has left
    int foob = maxCapacity - GET_COND(ch, HUNGER);
    amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, VAL_FOOD_FOODVAL) : 1);

    gain_condition(ch, HUNGER, amount);
    if (GET_FOODR(ch) == 0 && subcmd != SCMD_TASTE)
    {
        ch->modCurVital(CharVital::stamina, (ch->getEffectiveStat<int64_t>("stamina") / 100) * amount);
        ch->setBaseStat("food_rejuvenation", 2);
        ch->sendText("You feel rejuvinated by it.\r\n");
    }

    // Logic for food that will give PS or Exp
    if (GET_OBJ_VNUM(food) >= MEAL_START && GET_OBJ_VNUM(food) <= MEAL_LAST &&
        (!ch->location.getWhereFlag(WhereFlag::afterlife) && !ch->location.getWhereFlag(WhereFlag::afterlife_hell)))
    {
        if (subcmd != SCMD_TASTE)
        {
            int psbonus = GET_OBJ_VAL(food, VAL_FOOD_PSBONUS);
            int expbonus = GET_OBJ_VAL(food, VAL_FOOD_EXPBONUS) * ((GET_LEVEL(ch) * 0.4) + 1);
            int attr = GET_OBJ_VAL(food, VAL_FOOD_WHICHATTR);
            int attrChance = GET_OBJ_VAL(food, VAL_FOOD_ATTRCHANCE);
            int pscapped = false;
            if (level_exp(ch, GET_LEVEL(ch) + 1) - (GET_EXP(ch)) <= 0 && GET_LEVEL(ch) < 100)
            {
                expbonus = 1;
            }

            if (GET_PRACTICES(ch) >= 1000)
            {
                psbonus = 0;
                pscapped = true;
            }

            ch->modExperience(expbonus);
            ch->modPractices(psbonus);

            if (axion_dice(0) < attrChance)
            {
                switch (attr)
                {
                case 1:
                    ch->modBaseStat("strength", 1);
                    ch->sendText("@mThat was a hearty meal!@n\r\n");
                    break;
                case 2:
                    ch->modBaseStat("agility", 1);
                    ch->sendText("@mDiced to perfection.@n\r\n");
                    break;
                case 3:
                    ch->modBaseStat("constitution", 1);
                    ch->sendText("@mWhat a fortifying meal!@n\r\n");
                    break;
                case 4:
                    ch->modBaseStat("intelligence", 1);
                    ch->sendText("@mA splendid dish!@n\r\n");
                    break;
                case 5:
                    ch->modBaseStat("speed", 1);
                    ch->sendText("@mWhere did it all go?@n\r\n");
                    break;
                case 6:
                    ch->modBaseStat("wisdom", 1);
                    ch->sendText("@mYou feel sated. Content.@n\r\n");
                    break;
                }
            }

            ch->send_to("That was exceptionally delicious! @D[@mPS@D: @C+%d@D] [@gEXP@D: @G+%s@D]@n\r\n", psbonus, add_commas(expbonus).c_str());
            if (pscapped == true)
                ch->sendText("Practice Sessions capped for food at 1000 PS.\r\n");
        }
        // Good food can heal you
        if (!GET_OBJ_VAL(food, VAL_FOOD_POISON) && GET_HIT(ch) < (ch->getEffectiveStat<int64_t>("health")) && subcmd != SCMD_TASTE)
        {
            int64_t suppress = ((ch->getEffectiveStat<int64_t>("health")) * 0.01) * GET_SUPPRESS(ch);
            if (food->getEffectiveStat("weight") < 6)
            {
                ch->modCurVitalDam(CharVital::health, -.05);
            }
            else
            {
                ch->modCurVitalDam(CharVital::health, -.1);
            }

            ch->sendText("@MYou feel some of your strength return!@n\r\n");
        }
    }

    if (GET_OBJ_VAL(food, VAL_FOOD_POISON) && !ADM_FLAGGED(ch, ADM_NOPOISON))
    {
        /* The crap was poisoned ! */
        ch->sendText("Oops, that tasted rather strange!\r\n");
        act("$n coughs and utters some strange sounds.", false, ch, nullptr, nullptr, TO_ROOM);

        af.type = SPELL_POISON;
        af.duration = amount * 2;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.aff_flags.set(AFF_POISON);
        affect_join(ch, &af, false, false, false, false);
    }

    std::unordered_set<obj_vnum> candies = {53, 93, 94, 95};

    if (candies.contains(food->getVnum()))
    {
        if (IS_MAJIN(ch))
            foob = GET_OBJ_VAL(food, VAL_FOOD_FOODVAL);
        majin_gain(ch, food, foob);
    }

    if (subcmd == SCMD_EAT)
    {
        if (foob >= GET_OBJ_VAL(food, VAL_FOOD_FOODVAL))
        {
            ch->sendText("You finish the last bite.\r\n");
            extract_obj(food);
        }
        else
        {
            MOD_OBJ_VAL(food, VAL_FOOD_FOODVAL, -foob);
        }
    }
    else
    {
        if (!(MOD_OBJ_VAL(food, VAL_FOOD_FOODVAL, -1)))
        {
            ch->sendText("There's nothing left now.\r\n");
            extract_obj(food);
        }
    }
}

static void majin_gain(Character *ch, Object *food, int foob)
{
    if (!IS_MAJIN(ch) || IS_NPC(ch))
    {
        return;
    }

    auto soft_cap = ch->calc_soft_cap();
    auto current = (ch->getBaseStat<int64_t>("health") + ch->getBaseStat<int64_t>("ki") + ch->getBaseStat<int64_t>("stamina"));

    auto available = soft_cap - current;

    if (available <= 0.0)
    {
        ch->sendText("You can not gain anymore from candy consumption at your current level.\r\n");
        return;
    }

    double max = 0;
    double start_bonus = Random::get<double>(0.8, 1.2) * (1 + (GET_CON(ch) / 20)) * ch->getPotential();
    double diminishing_returns = (soft_cap - current) / soft_cap;
    if (diminishing_returns > 0.0)
        diminishing_returns = std::max<double>(diminishing_returns, 0.05);
    else
        diminishing_returns = 0;
    max = start_bonus * diminishing_returns * 8;

    int64_t st = food->getBaseStat<int64_t>(VAL_FOOD_CANDY_ST), ki = food->getBaseStat<int64_t>(VAL_FOOD_CANDY_KI), pl = food->getBaseStat<int64_t>(VAL_FOOD_CANDY_PL);

    st *= 0.05;
    ki *= 0.05;
    pl *= 0.05;

    std::vector<int> toAdd = {0, 1, 2};
    Random::shuffle(toAdd);

    // 0: pl, 1: ki, 2: stamina.

    int64_t addPL = 0;
    int64_t addKI = 0;
    int64_t addST = 0;

    for (auto &t : toAdd)
    {
        switch (t)
        {
        case 0:
            addPL = std::min<int64_t>(pl, available);
            addPL = std::min<int64_t>(addPL, (int64_t)max);
            available -= addPL;
            if (addPL <= 0)
                addPL = 1;
            break;
        case 1:
            addKI = std::min<int64_t>(ki, available);
            addKI = std::min<int64_t>(addKI, (int64_t)max);
            available -= addKI;
            if (addKI <= 0)
                addKI = 1;
            break;
        case 2:
            addST = std::min<int64_t>(st, available);
            addST = std::min<int64_t>(addST, (int64_t)max);
            available -= addST;
            if (addST <= 0)
                addST = 1;
            break;
        }
    }

    ch->gainBaseStat("health", addPL);
    ch->gainBaseStat("ki", addKI);
    ch->gainBaseStat("stamina", addST);

    ch->send_to("@mYou feel stronger after consuming the candy @D[@RPL@W: @r%s @CKi@D: @c%s @GSt@D: @g%s@D]@m!@n\r\n", add_commas(addPL).c_str(), add_commas(addKI).c_str(), add_commas(addST).c_str());
}

ACMD(do_pour)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Object *from_obj = nullptr, *to_obj = nullptr;
    int amount = 0;

    two_arguments(argument, arg1, arg2);

    if (subcmd == SCMD_POUR)
    {
        if (!*arg1)
        { /* No arguments */
            ch->sendText("From what do you want to pour?\r\n");
            return;
        }
        if (!(from_obj = get_obj_in_list_vis(ch, arg1, nullptr, ch->getInventory())))
        {
            ch->sendText("You can't find it!\r\n");
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON)
        {
            ch->sendText("You can't pour from that!\r\n");
            return;
        }
    }
    if (subcmd == SCMD_FILL)
    {
        if (!*arg1)
        { /* no arguments */
            ch->sendText("What do you want to fill?  And what are you filling it from?\r\n");
            return;
        }
        if (!(to_obj = get_obj_in_list_vis(ch, arg1, nullptr, ch->getInventory())))
        {
            ch->sendText("You can't find it!\r\n");
            return;
        }
        if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON)
        {
            act("You can't fill $p!", false, ch, to_obj, nullptr, TO_CHAR);
            return;
        }
        if (!*arg2)
        { /* no 2nd argument */
            act("What do you want to fill $p from?", false, ch, to_obj, nullptr, TO_CHAR);
            return;
        }
        if (!(from_obj = get_obj_in_list_vis(ch, arg2, nullptr, ch->location.getObjects())))
        {
            ch->send_to("There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
            return;
        }
        if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN && !OBJ_FLAGGED(from_obj, ITEM_BROKEN))
        {
            act("You can't fill something from $p.", false, ch, from_obj, nullptr, TO_CHAR);
            return;
        }
        else if (GET_OBJ_TYPE(from_obj) == ITEM_FOUNTAIN && OBJ_FLAGGED(from_obj, ITEM_BROKEN))
        {
            act("You can't fill something from a broken fountain.", false, ch, from_obj, nullptr, TO_CHAR);
            return;
        }
    }
    if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) == 0)
    {
        act("The $p is empty.", false, ch, from_obj, nullptr, TO_CHAR);
        return;
    }
    if (subcmd == SCMD_POUR)
    { /* pour */
        if (!*arg2)
        {
            ch->sendText("Where do you want it?  Out or in what?\r\n");
            return;
        }
        if (boost::iequals(arg2, "out"))
        {
            if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0)
            {
                act("$n empties $p.", true, ch, from_obj, nullptr, TO_ROOM);
                act("You empty $p.", false, ch, from_obj, nullptr, TO_CHAR);

                weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL)); /* Empty */

                name_from_drinkcon(from_obj);
                for (const auto &val : {VAL_DRINKCON_HOWFULL, VAL_DRINKCON_LIQUID, VAL_DRINKCON_POISON})
                {
                    SET_OBJ_VAL(from_obj, val, 0);
                }
            }
            else
            {
                ch->sendText("You can't possibly pour that container out!\r\n");
            }

            return;
        }
        if (!(to_obj = get_obj_in_list_vis(ch, arg2, nullptr, ch->getInventory())))
        {
            ch->sendText("You can't find it!\r\n");
            return;
        }
        if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
            (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN))
        {
            ch->sendText("You can't pour anything into that.\r\n");
            return;
        }
    }
    if (to_obj == from_obj)
    {
        ch->sendText("A most unproductive effort.\r\n");
        return;
    }
    if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) != 0) &&
        (GET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID) != GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID)))
    {
        ch->sendText("There is already another liquid in it!\r\n");
        return;
    }
    if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) < 0) ||
        (!(GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) < GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY))))
    {
        ch->sendText("There is no room for more.\r\n");
        return;
    }
    if (subcmd == SCMD_POUR)
        ch->send_to("You pour the %s into the %s.", drinks[GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID)], arg2);

    if (subcmd == SCMD_FILL)
    {
        act("You gently fill $p from $P.", false, ch, to_obj, from_obj, TO_CHAR);
        act("$n gently fills $p from $P.", true, ch, to_obj, from_obj, TO_ROOM);
    }
    /* New alias */
    if (GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) == 0)
        name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID));

    /* First same type liq. */
    SET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID, GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID));

    /* Then how much to pour */
    if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0)
    {
        MOD_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL, -(amount = (GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) - GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL))));

        SET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL, GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY));

        if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) < 0)
        { /* There was too little */
            MOD_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL, GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL));
            amount += GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL);
            name_from_drinkcon(from_obj);
            for (const auto &val : {VAL_DRINKCON_HOWFULL, VAL_DRINKCON_LIQUID, VAL_DRINKCON_POISON})
            {
                SET_OBJ_VAL(from_obj, val, 0);
            }
        }
    }
    else
    {
        SET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL, GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY));
    }

    /* Then the poison boogie */
    SET_OBJ_VAL(to_obj, VAL_DRINKCON_POISON, (GET_OBJ_VAL(to_obj, VAL_DRINKCON_POISON) || GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON)));

    /* And the weight boogie for non-eternal from_objects */
    if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0)
    {
        weight_change_object(from_obj, -amount);
    }
    weight_change_object(to_obj, amount); /* Add weight */
}

static void wear_message(Character *ch, Object *obj, int where)
{
    const char *wear_messages[][2] = {
        {"$n lights $p and holds it.",
         "You light $p and hold it."},

        {"$n slides $p on to $s right ring finger.",
         "You slide $p on to your right ring finger."},

        {"$n slides $p on to $s left ring finger.",
         "You slide $p on to your left ring finger."},

        {"$n wears $p around $s neck.",
         "You wear $p around your neck."},

        {"$n wears $p around $s neck.",
         "You wear $p around your neck."},

        {"$n wears $p on $s body.",
         "You wear $p on your body."},

        {"$n wears $p on $s head.",
         "You wear $p on your head."},

        {"$n puts $p on $s legs.",
         "You put $p on your legs."},

        {"$n wears $p on $s feet.",
         "You wear $p on your feet."},

        {"$n puts $p on $s hands.",
         "You put $p on your hands."},

        {"$n wears $p on $s arms.",
         "You wear $p on your arms."},

        {"$n straps $p around $s arm as a shield.",
         "You start to use $p as a shield."},

        {"$n wears $p about $s body.",
         "You wear $p around your body."},

        {"$n wears $p around $s waist.",
         "You wear $p around your waist."},

        {"$n puts $p on around $s right wrist.",
         "You put $p on around your right wrist."},

        {"$n puts $p on around $s left wrist.",
         "You put $p on around your left wrist."},

        {"$n wields $p.",
         "You wield $p."},

        {"$n grabs $p.",
         "You grab $p."},

        {"$n wears $p on $s back.",
         "You wear $p on your back."},

        {"$n puts $p in $s right ear.",
         "You put $p in your right ear."},

        {"$n puts $p in $s left ear.",
         "You put $p in your left ear."},

        {"$n wears $p as a cape.",
         "You wear $p as a cape."},

        {"$n covers $s left eye with $p.",
         "You wear $p over your left eye."}

    };

    act(wear_messages[where][0], true, ch, obj, nullptr, TO_ROOM);
    act(wear_messages[where][1], false, ch, obj, nullptr, TO_CHAR);
}

static int hands(Character *ch)
{
    int x;

    if (GET_EQ(ch, WEAR_WIELD1))
    {
        if (OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD1), ITEM_2H) ||
            wield_type(get_size(ch), GET_EQ(ch, WEAR_WIELD1)) == WIELD_TWOHAND)
        {
            x = 2;
        }
        else
            x = 1;
    }
    else
        x = 0;

    if (GET_EQ(ch, WEAR_WIELD2))
    {
        if (OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD2), ITEM_2H) ||
            wield_type(get_size(ch), GET_EQ(ch, WEAR_WIELD2)) == WIELD_TWOHAND)
        {
            x += 2;
        }
        else
            x += 1;
    }

    return x;
}

static int wear_bitvectors[] = {
    ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
    ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS,
    ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
    ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,
    ITEM_WEAR_TAKE, ITEM_WEAR_TAKE, ITEM_WEAR_PACK, ITEM_WEAR_EAR,
    ITEM_WEAR_EAR, ITEM_WEAR_SH, ITEM_WEAR_EYE};

static const char *already_wearing[] = {
    "You're already using a light.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already wearing something on both of your ring fingers.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You can't wear anything else around your neck.\r\n",
    "You're already wearing something on your body.\r\n",
    "You're already wearing something on your head.\r\n",
    "You're already wearing something on your legs.\r\n",
    "You're already wearing something on your feet.\r\n",
    "You're already wearing something on your hands.\r\n",
    "You're already wearing something on your arms.\r\n",
    "You're already using a shield.\r\n",
    "You're already wearing something about your body.\r\n",
    "You already have something around your waist.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already wearing something around both of your wrists.\r\n",
    "You're already wielding a weapon.\r\n",
    "You're already holding something.\r\n",
    "You're already wearing something on your back.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already wearing something in both ears.\r\n",
    "You're already wearing something on your shoulders.\r\n",
    "You're already wearing something as a scouter.\r\n"};

void perform_wear(Character *ch, Object *obj, int where)
{
    /*
     * ITEM_WEAR_TAKE is used for objects that do not require special bits
     * to be put into that position (e.g. you can hold any object, not just
     * an object with a HOLD bit.)
     */

    /* first, make sure that the wear position is valid. */
    if (!CAN_WEAR(obj, wear_bitvectors[where]))
    {
        act("You can't wear $p there.", false, ch, obj, nullptr, TO_CHAR);
        return;
    }
    /* do they even have that wear slot by race? */
    if (!BODY_FLAGGED(ch, where))
    {
        ch->sendText("Seems like your body type doesn't really allow that.\r\n");
        return;
    }
    /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
    if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R) ||
        (where == WEAR_EAR_R) || (where == WEAR_WIELD1))
        if (GET_EQ(ch, where))
            where++;

    /* checks for 2H sanity */
    if ((OBJ_FLAGGED(obj, ITEM_2H) || (wield_type(get_size(ch), obj) == WIELD_TWOHAND)) && hands(ch) > 0)
    {
        ch->sendText("Seems like you might not have enough free hands.\r\n");
        return;
    }
    if (where == WEAR_WIELD2 && PLR_FLAGGED(ch, PLR_THANDW))
    {
        ch->sendText("Seems like you might not have enough free hands.\r\n");
        return;
    }

    if (((where == WEAR_WIELD1) ||
         (where == WEAR_WIELD2)) &&
        (hands(ch) > 1))
    {
        ch->sendText("Seems like you might not have enough free hands.\r\n");
        return;
    }

    if (GET_EQ(ch, where))
    {
        ch->send_to("%s", already_wearing[where]);
        return;
    }

    /* See if a trigger disallows it */
    if (!wear_otrigger(obj, ch, where) || (obj->getCarriedBy() != ch))
        return;

    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && OBJ_FLAGGED(obj, ITEM_CUSTOM))
    {
        if (GET_STR(ch) < 20)
        {
            ch->sendText("You are not experienced enough to hold that.\r\n");
            return;
        }
    }

    // if (GET_OBJ_TYPE(obj) != ITEM_LIGHT && GET_OBJ_SIZE(obj) > get_size(ch)) {
    //         ch->sendText("Seems like it is too big for you.\r\n");
    //     return;
    // }
    // if (GET_OBJ_SIZE(obj) < get_size(ch) && GET_OBJ_TYPE(obj) != ITEM_LIGHT) {
    //         ch->sendText("Seems like it is too small for you.\r\n");
    //     return;
    // }

    wear_message(ch, obj, where);
    obj->clearLocation();
    equip_char(ch, obj, where);
}

int find_eq_pos(Character *ch, Object *obj, char *arg)
{
    int where = -1;

    const char *keywords[] = {
        "!RESERVED!",
        "finger",
        "!RESERVED!",
        "neck",
        "!RESERVED!",
        "body",
        "head",
        "legs",
        "feet",
        "hands",
        "arms",
        "shield",
        "about",
        "waist",
        "wrist",
        "!RESERVED!",
        "!RESERVED!",
        "!RESERVED!",
        "back",
        "ear",
        "\r!RESERVED!",
        "shoulders",
        "scouter",
        "\n"};

    if (!arg || !*arg)
    {
        if (CAN_WEAR(obj, ITEM_WEAR_FINGER) && BODY_FLAGGED(ch, WEAR_FINGER_R))
            return WEAR_FINGER_R;
        if (CAN_WEAR(obj, ITEM_WEAR_NECK) && BODY_FLAGGED(ch, WEAR_NECK_1))
            return WEAR_NECK_1;
        if (CAN_WEAR(obj, ITEM_WEAR_BODY) && BODY_FLAGGED(ch, WEAR_BODY))
            return WEAR_BODY;
        if (CAN_WEAR(obj, ITEM_WEAR_HEAD) && BODY_FLAGGED(ch, WEAR_HEAD))
            return WEAR_HEAD;
        if (CAN_WEAR(obj, ITEM_WEAR_LEGS) && BODY_FLAGGED(ch, WEAR_LEGS))
            return WEAR_LEGS;
        if (CAN_WEAR(obj, ITEM_WEAR_FEET) && BODY_FLAGGED(ch, WEAR_FEET))
            return WEAR_FEET;
        if (CAN_WEAR(obj, ITEM_WEAR_HANDS) && BODY_FLAGGED(ch, WEAR_HANDS))
            return WEAR_HANDS;
        if (CAN_WEAR(obj, ITEM_WEAR_ARMS) && BODY_FLAGGED(ch, WEAR_ARMS))
            return WEAR_ARMS;
        if (CAN_WEAR(obj, ITEM_WEAR_SHIELD) && BODY_FLAGGED(ch, WEAR_WIELD2))
            return WEAR_WIELD2;
        if (CAN_WEAR(obj, ITEM_WEAR_ABOUT) && BODY_FLAGGED(ch, WEAR_ABOUT))
            return WEAR_ABOUT;
        if (CAN_WEAR(obj, ITEM_WEAR_WAIST) && BODY_FLAGGED(ch, WEAR_WAIST))
            return WEAR_WAIST;
        if (CAN_WEAR(obj, ITEM_WEAR_WRIST) && BODY_FLAGGED(ch, WEAR_WRIST_R))
            return WEAR_WRIST_R;
        if (CAN_WEAR(obj, ITEM_WEAR_HOLD) && BODY_FLAGGED(ch, WEAR_WIELD2))
            return WEAR_WIELD2;
        if (CAN_WEAR(obj, ITEM_WEAR_PACK) && BODY_FLAGGED(ch, WEAR_BACKPACK))
            return WEAR_BACKPACK;
        if (CAN_WEAR(obj, ITEM_WEAR_EAR) && BODY_FLAGGED(ch, WEAR_EAR_R))
            return WEAR_EAR_R;
        if (CAN_WEAR(obj, ITEM_WEAR_SH) && BODY_FLAGGED(ch, WEAR_SH))
            return WEAR_SH;
        if (CAN_WEAR(obj, ITEM_WEAR_EYE) && BODY_FLAGGED(ch, WEAR_EYE))
            return WEAR_EYE;
    }
    else if (((where = search_block(arg, keywords, false)) < 0) || (*arg == '!'))
    {
        ch->send_to("'%s'?  What part of your body is THAT?\r\n", arg);
        return -1;
    }

    return (where);
}

ACMD(do_wear)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Object *obj, *next_obj;
    int where, dotmode, items_worn = 0;

    two_arguments(argument, arg1, arg2);

    if (!*arg1)
    {
        ch->sendText("Wear what?\r\n");
        return;
    }
    dotmode = find_all_dots(arg1);

    if (*arg2 && (dotmode != FIND_INDIV))
    {
        ch->sendText("You can't specify the same body location for more than one item!\r\n");
        return;
    }
    if (dotmode == FIND_ALL)
    {
        auto con = ch->getInventory();
        for (auto obj : filter_raw(con))
        {
            if (ch->canSee(obj) && (where = find_eq_pos(ch, obj, nullptr)) >= 0)
            {
                if (GET_WIS(ch) < GET_OBJ_LEVEL(obj))
                {
                    act("$p: you are not experienced enough to use that.",
                        false, ch, obj, nullptr, TO_CHAR);
                    ch->send_to("You need to be at least %d wisdom to use it.\r\n", GET_OBJ_LEVEL(obj));
                }
                else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
                {
                    act("$p: it seems to be broken.", false, ch, obj, nullptr, TO_CHAR);
                }
                else if (OBJ_FLAGGED(obj, ITEM_FORGED))
                {
                    act("$p: it seems to be fake...", false, ch, obj, nullptr, TO_CHAR);
                }
                else
                {
                    items_worn++;
                    if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, VAL_ARMOR_SKILL)) && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
                        ch->sendText("You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
                    perform_wear(ch, obj, where);
                }
            }
        }
        if (!items_worn)
            ch->sendText("You don't seem to have anything wearable.\r\n");
    }
    else if (dotmode == FIND_ALLDOT)
    {
        if (!*arg1)
        {
            ch->sendText("Wear all of what?\r\n");
            return;
        }
        auto con = ch->getInventory();
        int found = 0;
        for (auto obj : filter_raw(con))
        {
            if (!(ch->canSee(obj) && isname(arg1, obj->getName())))
                continue;
            found++;
            if (GET_WIS(ch) < GET_OBJ_LEVEL(obj))
            {
                ch->sendText("You are not experienced enough to use that.\r\n");
                continue;
            }
            if ((where = find_eq_pos(ch, obj, nullptr)) >= 0)
            {
                if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, VAL_ARMOR_SKILL)) && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
                    ch->sendText("You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
                perform_wear(ch, obj, where);
            }
            else
                act("You can't wear $p.", false, ch, obj, nullptr, TO_CHAR);
        }
        if (!found)
        {
            ch->send_to("You don't seem to have any %ss.\r\n", arg1);
        }
    }
    else
    {
        if (!(obj = get_obj_in_list_vis(ch, arg1, nullptr, ch->getInventory())))
            ch->send_to("You don't seem to have %s %s.\r\n", AN(arg1), arg1);
        else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
            ch->sendText("But it seems to be broken!\r\n");
        else if (OBJ_FLAGGED(obj, ITEM_FORGED))
            ch->sendText("But it seems to be fake!\r\n");
        else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
            ch->sendText("You are not experienced enough to use that.\r\n");
        else
        {
            if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
            {
                if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, VAL_ARMOR_SKILL)) && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
                    ch->sendText("You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
                perform_wear(ch, obj, where);
            }
            else if (!*arg2)
                act("You can't wear $p.", false, ch, obj, nullptr, TO_CHAR);
        }
    }
}

ACMD(do_wield)
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument(argument, arg);

    if (!*arg)
        ch->sendText("Wield what?\r\n");
    else if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
        ch->send_to("You don't seem to have %s %s.\r\n", AN(arg), arg);
    else
    {
        if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
            ch->sendText("You can't wield that.\r\n");
        else if (GET_OBJ_WEIGHT(obj) > max_carry_weight(ch))
            ch->sendText("It's too heavy for you to use.\r\n");
        else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
            ch->sendText("But it seems to be broken!\r\n");
        else if (OBJ_FLAGGED(obj, ITEM_FORGED))
            ch->sendText("But it seems to be fake!\r\n");
        else if (GET_WIS(ch) < GET_OBJ_LEVEL(obj))
            ch->sendText("You are not experienced enough to use that.\r\n");
        else if (PLR_FLAGGED(ch, PLR_THANDW))
            ch->sendText("You are holding a weapon with two hands right now!\r\n");
        else
        {
            if (!IS_NPC(ch) && !is_proficient_with_weapon(ch, GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)) && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
                ch->sendText("You have no proficiency with this type of weapon.\r\nYour attack accuracy will be greatly reduced.\r\n");
            perform_wear(ch, obj, WEAR_WIELD1);
        }
    }
}

ACMD(do_grab)
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;

    one_argument(argument, arg);

    if (!*arg)
        ch->sendText("Hold what?\r\n");
    else if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
        ch->send_to("You don't seem to have %s %s.\r\n", AN(arg), arg);
    else if (GET_WIS(ch) < GET_OBJ_LEVEL(obj))
        ch->sendText("You are not experienced enough to use that.\r\n");
    else if (PLR_FLAGGED(ch, PLR_THANDW))
        ch->sendText("You are wielding a weapon with both hands currently.\r\n");
    else
    {
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
        {
            perform_wear(ch, obj, WEAR_WIELD2);
            if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS) > 0 || GET_OBJ_VAL(obj, VAL_LIGHT_HOURS) < 0)
            {
                act("@wYou light $p@w.", true, ch, obj, nullptr, TO_CHAR);
                act("@C$n@w lights $p@w.", true, ch, obj, nullptr, TO_ROOM);
            }
            if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS) == 0)
            {
                act("@wYou try to light $p@w but it is burnt out.", true, ch, obj, nullptr, TO_CHAR);
                act("@C$n@w tries to light $p@w but nothing happens.", true, ch, obj, nullptr, TO_ROOM);
            }
        }
        else
        {
            if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND &&
                GET_OBJ_TYPE(obj) != ITEM_STAFF && GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
                GET_OBJ_TYPE(obj) != ITEM_POTION)
                ch->sendText("You can't hold that.\r\n");
            else
                perform_wear(ch, obj, WEAR_WIELD2);
        }
    }
}

void perform_remove(Character *ch, int pos)
{
    Object *obj;

    int64_t previous = GET_HIT(ch);

    if (!(obj = GET_EQ(ch, pos)))
        basic_mud_log("SYSERR: perform_remove: bad pos %d passed.", pos);
    /*  SYSERR_DESC:
     *  This error occurs when perform_remove() is passed a bad 'pos'
     *  (location) to remove an object from.
     */
    else if (OBJ_FLAGGED(obj, ITEM_NODROP) && GET_ADMLEVEL(ch) < 1)
        act("You can't remove $p, it must be CURSED!", false, ch, obj, nullptr, TO_CHAR);
    else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
        act("$p: your arms are full!", false, ch, obj, nullptr, TO_CHAR);
    else
    {
        if (!remove_otrigger(obj, ch))
            return;

        if (pos == WEAR_WIELD1 && PLR_FLAGGED(ch, PLR_THANDW))
        {
            ch->player_flags.set(PLR_THANDW, false);
        }
        auto un = unequip_char(ch, pos);
        ch->addToInventory(un);
        act("You stop using $p.", false, ch, obj, nullptr, TO_CHAR);
        act("$n stops using $p.", true, ch, obj, nullptr, TO_ROOM);
        if (previous > GET_HIT(ch))
        {
            char drop[MAX_INPUT_LENGTH];
            sprintf(drop, "@RYour powerlevel has dropped from removing $p@R! @D[@r-%s@D]\r\n",
                    add_commas(previous - GET_HIT(ch)).c_str());
            act(drop, false, ch, obj, nullptr, TO_CHAR);
        }
    }
}

ACMD(do_remove)
{
    Object *obj;
    char arg[MAX_INPUT_LENGTH];
    int i, dotmode, found = 0, msg;

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Remove what?\r\n");
        return;
    }

    auto isBoard = [&](const auto &o)
    { return GET_OBJ_TYPE(o) == ITEM_BOARD; };

    obj = ch->searchInventory(isBoard);
    /* lemme check for a board FIRST */
    if (!obj)
        ch->location.searchObjects(isBoard);

    if (obj)
    {
        if (!isdigit(*arg) || (!(msg = atoi(arg))))
        {
            found = 0;
        }
        else
        {
            remove_board_msg(GET_OBJ_VNUM(obj), ch, msg);
        }
    }
    if (found)
        return;

    dotmode = find_all_dots(arg);

    if (dotmode == FIND_ALL)
    {
        found = 0;
        for (i = 0; i < NUM_WEARS; i++)
        {
            if (GET_EQ(ch, i))
            {
                perform_remove(ch, i);
                found = 1;
            }
        }
        if (!found)
        {
            ch->sendText("You're not using anything.\r\n");
        }
    }
    else if (dotmode == FIND_ALLDOT)
    {
        if (!*arg)
        {
            ch->sendText("Remove all of what?\r\n");
        }
        else
        {
            found = 0;
            for (i = 0; i < NUM_WEARS; i++)
            {
                if (GET_EQ(ch, i) && ch->canSee(GET_EQ(ch, i)) &&
                    isname(arg, GET_EQ(ch, i)->getName()))
                {
                    perform_remove(ch, i);
                    found = 1;
                }
            }
            if (!found)
            {
                ch->send_to("You don't seem to be using any %ss.\r\n", arg);
            }
        }
    }
    else
    {
        if ((i = get_obj_pos_in_equip_vis(ch, arg, nullptr, ch->getEquipment())) < 0)
        {
            ch->send_to("You don't seem to be using %s %s.\r\n", AN(arg), arg);
        }
        else
        {
            perform_remove(ch, i);
        }
    }
}

ACMD(do_sac)
{
    char arg[MAX_INPUT_LENGTH];
    Object *j;

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Sacrifice what?\r\n");
        return;
    }

    if (!(j = get_obj_in_list_vis(ch, arg, nullptr, ch->location.getObjects())) &&
        (!(j = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory()))))
    {
        ch->sendText("It doesn't seem to be here.\r\n");
        return;
    }

    if (!CAN_WEAR(j, ITEM_WEAR_TAKE))
    {
        ch->sendText("You can't sacrifice that!\r\n");
        return;
    }

    act("$n sacrifices $p.", false, ch, j, nullptr, TO_ROOM);

    if (!GET_OBJ_COST(j) && !IS_CORPSE(j))
    {
        ch->sendText("Zizazat mocks your sacrifice. Try again, try harder.\r\n");
        return;
    }

    if (!IS_CORPSE(j))
    {
        switch (Random::get<int>(0, 5))
        {
        case 0:
            ch->send_to("You sacrifice %s to the Gods.\r\nYou receive one zenni for your humility.\r\n", GET_OBJ_SHORT(j));
            ch->modBaseStat("money_carried", 1);
            break;
        case 1:
            ch->send_to("You sacrifice %s to the Gods.\r\nThe Gods ignore your sacrifice.\r\n", GET_OBJ_SHORT(j));
            break;
        case 2:
            ch->send_to("You sacrifice %s to the Gods.\r\nZizazat gives you %d experience points.\r\n", GET_OBJ_SHORT(j), (2 * GET_OBJ_COST(j)));
            ch->modExperience(2 * GET_OBJ_COST(j));
            break;
        case 3:
            ch->send_to("You sacrifice %s to the Gods.\r\nYou receive %d experience points.\r\n", GET_OBJ_SHORT(j), GET_OBJ_COST(j));
            ch->modExperience(GET_OBJ_COST(j));
            break;
        case 4:
            ch->send_to("Your sacrifice to the Gods is rewarded with %d zenni.\r\n", GET_OBJ_COST(j));
            ch->modBaseStat("money_carried", GET_OBJ_COST(j));
            break;
        case 5:
            ch->send_to("Your sacrifice to the Gods is rewarded with %d zenni\r\n", (2 * GET_OBJ_COST(j)));
            ch->modBaseStat("money_carried", (2 * GET_OBJ_COST(j)));
            break;
        default:
            ch->send_to("You sacrifice %s to the Gods.\r\nYou receive one zenni for your humility.\r\n", GET_OBJ_SHORT(j));
            ch->modBaseStat("money_carried", 1);
            break;
        }
    }
    else
    {
        /* No longer transfer corpse contents to room. Sac it, sac it all. */
        ch->sendText("You send the corpse on to the next life!\r\n");
    }
    extract_obj(j);
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const int max_carry_load[] = {
    0,
    10,
    20,
    30,
    40,
    50,
    60,
    70,
    80,
    90,
    100,
    115,
    130,
    150,
    175,
    200,
    230,
    260,
    300,
    350,
    400,
    460,
    520,
    600,
    700,
    800,
    920,
    1040,
    1200,
    1400,
    1640,
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int64_t max_carry_weight(Character *ch)
{
    int64_t abil;
    int total;
    abil = (GET_MAX_HIT(ch) / 200) + (GET_STR(ch) * 50);
    total = 1;
    return (total * abil);
}
