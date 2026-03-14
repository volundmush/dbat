/* ************************************************************************
 *   File: spec_procs.c                                  Part of CircleMUD *
 *  Usage: implementation of special procedures for mobiles/objects/rooms  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/Destination.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/Account.hpp"
#include "dbat/game/spec_procs.hpp"
//#include "dbat/game/send.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/game/interpreter.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/db.hpp"
//#include "dbat/game/spells.hpp"
#include "dbat/game/constants.hpp"
#include "dbat/game/mail.hpp"
//#include "dbat/game/act.movement.hpp"
//#include "dbat/game/act.item.hpp"
#include "dbat/game/act.social.hpp"
#include "dbat/game/Guild.hpp"
#include "dbat/game/races.hpp"
#include "dbat/game/act.comm.hpp"
#include "dbat/game/class.hpp"
#include "dbat/game/players.hpp"
//#include "dbat/game/act.informative.hpp"
#include "dbat/util/FilterWeak.hpp"
#include "dbat/game/Random.hpp"
#include "dbat/game/utils.hpp"

#include "dbat/game/const/Gauntlet.hpp"
#include "dbat/game/const/WearSlot.hpp"

/* local functions */

/* ********************************************************************
 *  Special procedures for mobiles                                     *
 ******************************************************************** */

SPECIAL(dump)
{
    int value = 0;

    auto con = ch->location.getObjects();
    for (auto k : dbat::util::filter_raw(con))
    {
        act("$p vanishes in a puff of smoke!", false, nullptr, k, nullptr, TO_ROOM);
        extract_obj(k);
    }

    if (!CMD_IS("drop"))
        return (false);

    do_drop(ch, argument, cmd, SCMD_DROP);

    con = ch->location.getObjects();
    for (auto k : dbat::util::filter_raw(con))
    {
        act("$p vanishes in a puff of smoke!", false, nullptr, k, nullptr, TO_ROOM);
        value += std::clamp<int>(GET_OBJ_COST(k) / 10, 1, 50);
        extract_obj(k);
    }

    if (value)
    {
        ch->sendText("You are awarded for outstanding performance.\r\n");
        act("$n has been awarded for being a good citizen.", true, ch, nullptr, nullptr, TO_ROOM);

        if (GET_LEVEL(ch) < 3)
        {
            ch->modExperience(value);
        }
        else
            ch->modBaseStat("money_carried", value);
    }
    return (true);
}

/* ********************************************************************
 *  General special procedures for mobiles                             *
 ******************************************************************** */

int num_players_in_room(room_vnum room)
{
    auto r = get_room(room);
    if (!r)
        return 0;

    int num_players = 0;
    auto p = r->getPeople().snapshot_weak();
    for (auto ch : dbat::util::filter_raw(p))
    {
        if (IS_NPC(ch))
            continue;
        if ((GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) && (PRF_FLAGGED(ch, PRF_NOHASSLE)))
            num_players++;
    }

    return num_players;
}

bool check_mob_in_room(mob_vnum mob, room_vnum room)
{
    if (auto r = get_room(room); r)
    {
        auto p = r->getPeople().snapshot_weak();
        for (auto ch : dbat::util::filter_raw(p))
            if (ch->getVnum() == mob)
                return true;
    }
    return false;
}

bool check_obj_in_room(obj_vnum obj, room_vnum room)
{
    if (auto r = Location(get_room(room)); r)
    {
        if (r.searchObjects(obj))
            return true;
    }
    return false;
}

static const int gauntlet_info[][3] = {/* --mystic 26 Oct 2005 */

                                       /* Gauntlet Room Scoring*/
                                       /* Num  Rm Num   Direction   */
                                       {0, 2403, SCMD_SOUTH},  /* Waiting Room     */
                                       {1, 2404, SCMD_SOUTH},  /* About level 5    */
                                       {2, 2405, SCMD_SOUTH},  /* About level 10   */
                                       {3, 2406, SCMD_SOUTH},  /* About level 15   */
                                       {4, 2407, SCMD_SOUTH},  /* About level 20   */
                                       {5, 2408, SCMD_SOUTH},  /* About level 25   */
                                       {6, 2409, SCMD_SOUTH},  /* About level 30   */
                                       {7, 2410, SCMD_SOUTH},  /* About level 35   */
                                       {8, 2411, SCMD_SOUTH},  /* About level 40   */
                                       {9, 2412, SCMD_SOUTH},  /* About level 45   */
                                       {10, 2413, SCMD_SOUTH}, /* About level 50   */
                                       {11, 2414, SCMD_SOUTH}, /* About level 55   */
                                       {12, 2415, SCMD_SOUTH}, /* About level 60   */
                                       {13, 2416, SCMD_SOUTH}, /* About level 65   */
                                       {14, 2417, SCMD_SOUTH}, /* About level 70   */
                                       {15, 2418, SCMD_SOUTH}, /* About level 75   */
                                       {16, 2420, SCMD_SOUTH}, /* About level 80   */
                                       {17, 2421, SCMD_SOUTH}, /* About level 85   */
                                       {18, 2422, SCMD_SOUTH}, /* About level 90   */
                                       {19, 2423, SCMD_SOUTH}, /* About level 95   */
                                       {20, 2424, SCMD_SOUTH}, /* About level 100  */
                                       {21, 2425, SCMD_SOUTH}, /* About level 5    */
                                       {22, 2426, SCMD_SOUTH}, /* About level 10   */
                                       {23, 2427, SCMD_SOUTH}, /* About level 15   */
                                       {24, 2428, SCMD_SOUTH}, /* About level 20   */
                                       {25, 2429, SCMD_SOUTH}, /* About level 25   */
                                       {26, 2430, SCMD_SOUTH}, /* About level 30   */
                                       {27, 2431, SCMD_SOUTH}, /* About level 35   */
                                       {28, 2432, SCMD_SOUTH}, /* About level 40   */
                                       {29, 2433, SCMD_SOUTH}, /* About level 45   */
                                       {30, 2434, SCMD_SOUTH}, /* About level 50   */
                                       {31, 2435, SCMD_SOUTH}, /* About level 55   */
                                       {32, 2436, SCMD_SOUTH}, /* About level 60   */
                                       {33, 2437, SCMD_SOUTH}, /* About level 65   */
                                       {34, 2438, SCMD_SOUTH}, /* About level 70   */
                                       {35, 2439, SCMD_SOUTH}, /* About level 75   */
                                       {36, 2440, SCMD_SOUTH}, /* About level 80   */
                                       {37, 2441, SCMD_SOUTH}, /* About level 85   */
                                       {38, 2442, SCMD_SOUTH}, /* About level 90   */
                                       {39, 2443, SCMD_SOUTH}, /* About level 95   */
                                       {40, 2444, SCMD_SOUTH}, /* About level 100  */
                                       {-1, -1, -1}};

SPECIAL(gauntlet_room) /* Jamdog - 13th Feb 2006 */
{
    int i = 0;
    int proceed = 1;
    Character *tch;
    char *buf2 = "$N tried to sneak past without a fight, and got nowhere.";
    char buf[MAX_STRING_LENGTH];
    bool nomob = true;

    /* give player credit for making it this far */
    for (i = 0; gauntlet_info[i][0] != -1; i++)
    {
        if ((!IS_NPC(ch)) && (ch->location == gauntlet_info[i][1]))
        {
            /* Check not overwriting gauntlet rank with lower value (Jamdog - 20th July 2006) */
            if (GET_GAUNTLET(ch) < (gauntlet_info[i][0]))
            {
                // set player's gauntlet rank
                ch->setBaseStat("gauntlet", gauntlet_info[i][0]);
            }
        }
    }

    if (!cmd) /* If no command, then nothing to do             */
        return false;

    if (CMD_IS("flee"))
    {
        ch->sendText("Fleeing is not allowed!  If you want to get out of here, type @Ysurrender@n while fighting to be returned to the start.");
        return true;
    }

    if (!IS_MOVE(cmd) && !CMD_IS("surrender")) /* Only movement commands need to be checked     */
        return false;

    if (IS_NPC(ch))   /* Mobs can move about - Jamdog 20th July 2006   */
        return false; /* This also allows following pets!              */

    if (CMD_IS("surrender"))
    {
        if (FIGHTING(ch))
        {
            /* OK, player has had enough - position is already stored, so throw them back to the start */
            ch->leaveLocation();
            ch->moveToLocation(gauntlet_info[0][1]);
            act("$n suddenly appears looking relieved after $s trial in the Gauntlet", false, ch, nullptr, ch,
                TO_NOTVICT);
            act("You are returned to the start of the Gauntlet", false, ch, nullptr, ch, TO_VICT);

            /* Hit point penalty for surrendering */
            ch->modCurVital(CharVital::health, -2000);

            ch->lookAtLocation();
            return true;
        }
        else
        {
            ch->sendText("You can only surrender while fighting, so at least TRY to make an effort");
            return true;
        }
    }

    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) /* Imms can walk through the gauntlet unhindered */
        return false;

    for (i = 0; gauntlet_info[i][0] != -1; i++)
    {
        if (ch->location == gauntlet_info[i][1])
        {
            if (cmd == gauntlet_info[i][2])
            {
                // don't let him proceed if mob is still alive
                auto loco = ch->location.getPeople();
                for (auto tch : dbat::util::filter_raw(loco))
                {
                    if (IS_NPC(tch) && i > 0) /* Ignore mobs in the waiting room */
                    {
                        proceed = 0;
                        sprintf(buf, "%s wants to teach you a lesson first.\r\n", GET_NAME(tch));
                    }
                }
                /* In the case of the phoenix room, don't progress if there is 1st or 2nd ashes */
                /* TODO: (add in here when ash is created) */
                if (proceed)
                {
                    nomob = true;

                    /* Check the next room for players and ensure mob is waiting */
                    auto pg = get_room(real_room(gauntlet_info[i + 1][1]))->getPeople().snapshot_weak();
                    for (auto tch : dbat::util::filter_raw(pg))
                    {
                        if (!IS_NPC(tch))
                        {
                            proceed = 0; /* There is a player there */
                            sprintf(buf, "%s is in the next room.  You must wait for them to finish.\r\n",
                                    GET_NAME(tch));
                        }
                        else
                        {
                            nomob = false;
                        }
                    }
                    if (nomob == true)
                    {
                        proceed = 0; /* There is no mob there in the next room */
                        sprintf(buf, "The next room is empty.  You must wait for your opponent to re-appear.\r\n");
                    }
                }

                if (proceed == 0)
                {
                    ch->sendText(buf);
                    act(buf2, false, ch, nullptr, ch, TO_ROOM);
                    return true;
                }
            }
        }
    }
    return false;
}

SPECIAL(gauntlet_end) /* Jamdog - 20th Feb 2007 */
{
    int i = 0;

    /* give player credit for making it this far */
    if (!IS_NPC(ch))
    {
        /* Check not overwriting gauntlet rank with lower value (Jamdog - 20th July 2006) */
        if (GET_GAUNTLET(ch) < GAUNTLET_END)
        {
            // set player's gauntlet rank
            ch->setBaseStat("gauntlet", GAUNTLET_END);
        }
    }

    if (!cmd) /* If no command, then nothing to do             */
        return false;

    if (CMD_IS("flee"))
    {
        if ((FIGHTING(ch)) && (GET_POS(ch) == POS_FIGHTING))
        {
            ch->sendText("You can't flee from this fight./r/nIt's your own fault for summoning creatures into the gauntlet!\r\n");
            return true;
        }
        else
        {
            ch->sendText("There is nothing here to flee from\r\n");
            return true;
        }
    }

    if (CMD_IS("surrender"))
    {
        ch->sendText("You have completed the gauntlet, why would you need to surrender?\r\n");
        return true;
    }

    if (!IS_MOVE(cmd)) /* Only movement commands need to be checked     */
        return false;

    if (IS_NPC(ch))   /* Mobs can move about - Jamdog 20th July 2006   */
        return false; /* This also allows following pets!              */

    auto ex = EXIT(ch, cmd - 1);
    if (!ex)
        return false;
    if (ex->exit_flags[EX_CLOSED])
        return false;

    for (i = 0; gauntlet_info[i][0] != -1; i++)
    {
        if (*EXIT(ch, (cmd - 1)) == gauntlet_info[i][1])
        {
            ch->sendText("You have completed the gauntlet, you cannot go backwards!\r\n");
            return true;
        }
    }
    return false;
}

SPECIAL(gauntlet_rest) /* Jamdog - 20th Feb 2007 */
{
    int i = 0;
    int proceed = 1, door;
    Character *tch;
    char *buf2 = "$N tried to return to the gauntlet, and got nowhere.";
    char buf[MAX_STRING_LENGTH];
    bool nomob = true;

    if (!cmd) /* If no command, then nothing to do             */
        return false;

    if (CMD_IS("flee"))
    {
        ch->sendText("Fleeing is not allowed!  If you want to get out of here, type @Ysurrender@n while fighting to be returned to the start.");
        return true;
    }

    if (CMD_IS("surrender"))
    {
        ch->sendText("You are in a rest-room.  Surrender is not an option.\r\nIf you want to leave the Gauntlet, you can surrender while fighting.\r\n");
        return true;
    }

    if (!IS_MOVE(cmd)) /* Only movement commands need to be checked     */
        return false;

    if (IS_NPC(ch))   /* Mobs can move about - Jamdog 20th July 2006   */
        return false; /* This also allows following pets!              */

    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) /* Imms can walk through the gauntlet unhindered */
        return false;

    for (i = 0; gauntlet_info[i][0] != -1; i++)
    {
        for (door = 0; door < NUM_OF_DIRS; door++)
        {
            auto ex = EXIT(ch, door);
            if (!ex)
                continue;
            if (ex->exit_flags[EX_CLOSED])
                continue;

            if ((ex == gauntlet_info[i][1]) && (door == (cmd - 1)))
            {
                nomob = true;

                /* Check the next room for players and ensure mob is waiting */
                auto pg = get_room(real_room(gauntlet_info[i][1]))->getPeople().snapshot_weak();
                for (auto tch : dbat::util::filter_raw(pg))
                {
                    if (!IS_NPC(tch))
                    {
                        proceed = 0; /* There is a player there */
                        sprintf(buf, "%s has moved into the next room.  You must wait for them to finish.\r\n",
                                GET_NAME(tch));
                    }
                    else
                    {
                        nomob = false;
                    }
                }
                /* Not needed - players can go back if the mob is dead
                        if (nomob == TRUE)
                        {
                          proceed=0;  // There is no mob there in the next room
                          sprintf(buf,"The next room is empty.  You must wait for your opponent to re-appear.\r\n");
                        }
                */
                if (proceed == 0)
                {
                    ch->sendText(buf);
                    act(buf2, false, ch, nullptr, ch, TO_ROOM);
                    return true;
                }
            }
        }
    }
    return false;
}

void npc_steal(Character *ch, Character *victim)
{
    int gold;

    if (IS_NPC(victim))
        return;
    if (IS_NPC(ch))
        return;
    if (ADM_FLAGGED(victim, ADM_NOSTEAL))
        return;
    if (!ch->canSee(victim))
        return;

    if (AWAKE(victim) && (Random::get<int>(0, GET_LEVEL(ch)) == 0))
    {
        act("You discover that $n has $s hands in your wallet.", false, ch, nullptr, victim, TO_VICT);
        act("$n tries to steal zenni from $N.", true, ch, nullptr, victim, TO_NOTVICT);
    }
    else
    {
        /* Steal some gold coins */
        gold = (GET_GOLD(victim) * Random::get<int>(1, 10)) / 100;
        if (gold > 0)
        {
            ch->modBaseStat("money_carried", gold);
            victim->modBaseStat("money_carried", -gold);
        }
    }
}

/* ********************************************************************
 *  Special procedures for mobiles                                      *
 ******************************************************************** */
#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
    char buf[MAX_STRING_LENGTH], pet_name[256];
    room_rnum pet_room;
    Character *pet;
    /* Gross. */
    pet_room = IN_ROOM(ch) + 1;

    if (CMD_IS("list"))
    {
        ch->sendText("Available pets are:\r\n");
        auto pcon = get_room(pet_room)->getPeople().snapshot_weak();
        for (auto pet : dbat::util::filter_raw(pcon))
        {
            /* No, you can't have the Implementor as a pet if he's in there. */
            if (!IS_NPC(pet))
                continue;
            ch->send_to("%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
        }
        return (true);
    }
    else if (CMD_IS("buy"))
    {

        two_arguments(argument, buf, pet_name);

        if (!(pet = get_char_room(buf, nullptr, pet_room)) || !IS_NPC(pet))
        {
            ch->sendText("There is no such pet!\r\n");
            return (true);
        }
        if (GET_GOLD(ch) < PET_PRICE(pet))
        {
            ch->sendText("You don't have enough zenni!\r\n");
            return (true);
        }
        ch->modBaseStat("money_carried", -PET_PRICE(pet));

        pet = read_mobile(GET_MOB_RNUM(pet), REAL);
        pet->affect_flags.set(AFF_CHARM, true);

        if (*pet_name)
        {
            snprintf(buf, sizeof(buf), "%s %s", pet->getName(), pet_name);
            /* free(pet->name); don't free the prototype! */
            pet->name = buf;

            snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
                     pet->getLookDescription(), pet_name);
            /* free(pet->description); don't free the prototype! */
            pet->look_description = buf;
        }
        pet->moveToLocation(ch);
        add_follower(pet, ch);
        pet->setBaseStat("master_id", GET_IDNUM(ch));

        ch->sendText("May you enjoy your pet.\r\n");
        act("$n buys $N as a pet.", false, ch, nullptr, pet, TO_ROOM);

        return (true);
    }

    /* All commands except list and buy */
    return (false);
}

SPECIAL(auction)
{
    room_rnum auct_room;
    Object *obj, *next_obj, *obj2 = nullptr;
    int found = false;

    /* Gross. */
    auct_room = real_room(80);

    if (CMD_IS("cancel"))
    {
        auto aroom = get_room(auct_room);
        Destination des(aroom);
        auto con = des.getObjects();
        for (auto obj : dbat::util::filter_raw(con))
        {
            if (GET_AUCTER(obj) == ((ch)->id))
            {
                obj2 = obj;
                found = true;

                if (GET_CURBID(obj2) != -1 && GET_AUCTIME(obj2) + 518400 > time(nullptr))
                {
                    ch->sendText("Unable to cancel. Someone has already bid on it and their bid license hasn't expired.\r\n");
                    time_t remain = (GET_AUCTIME(obj2) + 518400) - time(nullptr);
                    int day = (int)((remain % 604800) / 86400);
                    int hour = (int)((remain % 86400) / 3600);
                    int minu = (int)((remain % 3600) / 60);
                    ch->send_to("Time Till License Expiration: %d day%s, %d hour%s, %d minute%s.\r\n", day, day > 1 ? "s" : "", hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
                    continue;
                }

                ch->send_to("@wYou cancel the auction of %s@w and it is returned to you.@n\r\n", obj2->getShortDescription());
                struct descriptor_data *d;

                for (d = descriptor_list; d; d = d->next)
                {
                    if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                        continue;
                    if (d->character == ch)
                        continue;
                    if (GET_EQ(d->character, WEAR_EYE))
                    {
                        d->character->send_to("@RScouter Auction News@D: @GThe auction of @w%s@G has been canceled.\r\n", obj2->getShortDescription());
                    }
                }

                obj2->clearLocation();
                ch->addToInventory(obj2);
                auc_save();
            }
        }

        if (found == false)
        {
            ch->sendText("There are no items being auctioned by you.\r\n");
        }

        return (true);
    }
    else if (CMD_IS("pickup"))
    {
        struct descriptor_data *d;
        int founded = false;
        auto aroom = get_room(auct_room);
        Destination des(aroom);
        auto con = des.getObjects();
        for (auto obj : dbat::util::filter_raw(con))
        {
            if (GET_CURBID(obj) == ((ch)->id))
            {
                obj2 = obj;
                found = true;

                if (GET_AUCTER(obj) <= 0)
                {
                    continue;
                }

                if (GET_BID(obj2) > GET_GOLD(ch))
                {
                    ch->send_to("Unable to purchase %s, you don't have enough money on hand.\r\n", obj2->getShortDescription());
                    continue;
                }

                if (GET_AUCTIME(obj2) + 86400 > time(nullptr))
                {
                    time_t remain = (GET_AUCTIME(obj2) + 86400) - time(nullptr);
                    int hour = (int)((remain % 86400) / 3600);
                    int minu = (int)((remain % 3600) / 60);
                    ch->send_to("Unable to purchase %s, minimum time to bid is 24 hours. %d hour%s and %d minute%s remain.\r\n", obj2->getShortDescription(), hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
                    continue;
                }

                ch->modBaseStat("money_carried", -GET_BID(obj2));
                obj2->clearLocation();
                ch->addToInventory(obj2);
                ch->send_to("You pay %s zenni and receive the item.\r\n", add_commas(GET_BID(obj2)).c_str());
                auc_save();

                for (d = descriptor_list; d; d = d->next)
                {
                    if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                        continue;
                    if (d->character == ch)
                        continue;
                    if (GET_IDNUM(d->character) == GET_AUCTER(obj2))
                    {
                        founded = true;
                        d->character->modBaseStat("money_carried", GET_BID(obj2));
                        if (GET_EQ(d->character, WEAR_EYE))
                        {
                            d->character->send_to("@RScouter Auction News@D: @GSomeone has purchased your @w%s@G and you had the money put in your bank account.\r\n", obj2->getShortDescription());
                        }
                    }
                    else if (GET_EQ(d->character, WEAR_EYE))
                    {
                        d->character->send_to("@RScouter Auction News@D: @GSomeone has purchased the @w%s@G that was on auction.\r\n", obj2->getShortDescription());
                    }
                }

                if (founded == false)
                {
                    char blam[50];
                    sprintf(blam, "%s", GET_AUCTERN(obj2));
                    auto vict = findPlayer(blam);
                    if (!vict)
                        continue;

                    vict->modBaseStat("money_bank", GET_BID(obj2));
                }
            }
        }

        if (found == false)
        {
            ch->sendText("There are no items that you have bid on.\r\n");
        }
        return (true);
    }
    else if (CMD_IS("auction"))
    {
        char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
        struct descriptor_data *d;
        int value = 0;

        two_arguments(argument, arg, arg2);

        if (!*arg || !*arg2)
        {
            ch->sendText("Auction what item and for how much?\r\n");
            return (true);
        }

        value = atoi(arg2);

        if (!(obj2 = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
        {
            ch->sendText("You don't have that item to auction.\r\n");
            return (true);
        }
        if (value <= 999)
        {
            ch->sendText("Do not auction anything for less than 1,000 zenni.\r\n");
            return (true);
        }

        if (OBJ_FLAGGED(obj2, ITEM_BROKEN))
        {
            act("$P is broken and we will not accept it.", false, ch, nullptr, obj2, TO_CHAR);
            return (true);
        }

        if (OBJ_FLAGGED(obj2, ITEM_NODONATE))
        {
            act("$P is junk and we will not accept it.", false, ch, nullptr, obj2, TO_CHAR);
            return (true);
        }

        GET_BID(obj2) = value;
        GET_STARTBID(obj2) = 0;
        GET_AUCTER(obj2) = 0;
        if (GET_AUCTERN(obj2))
            free(GET_AUCTERN(obj2));
        GET_AUCTIME(obj2) = 0;

        GET_BID(obj2) = value;
        GET_STARTBID(obj2) = GET_BID(obj2);
        GET_AUCTER(obj2) = ((ch)->id);
        GET_AUCTERN(obj2) = strdup(GET_NAME(ch));
        GET_AUCTIME(obj2) = time(nullptr);
        GET_CURBID(obj2) = -1;
        obj2->clearLocation();
        obj2->moveToLocation(auct_room);
        auc_save();
        ch->send_to("You place %s on auction for %s zenni.\r\n", obj2->getShortDescription(), add_commas(GET_BID(obj2)).c_str());
        basic_mud_log("AUCTION: %s places %s on auction for %s", GET_NAME(ch), obj2->getShortDescription(),
                      add_commas(GET_BID(obj2)).c_str());

        for (d = descriptor_list; d; d = d->next)
        {
            if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                continue;
            if (d->character == ch)
                continue;
            if (GET_EQ(d->character, WEAR_EYE))
            {
                d->character->send_to("@RScouter Auction News@D: @GThe item, @w%s@G, has been placed on auction for @Y%s@G zenni.@n\r\n", obj2->getShortDescription(), add_commas(GET_BID(obj2)).c_str());
            }
        }
        return (true);
    }

    /* All commands except list and buy */
    return (false);
}

/* ********************************************************************
 *  Special procedures for objects                                     *
 ******************************************************************** */

SPECIAL(healtank)
{
    Object *i;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    auto htank = ch->location.searchObjects(65);

    if (CMD_IS("htank"))
    {
        if (!htank)
        {
            return (false);
        }

        if (!*arg)
        {
            ch->sendText("@WHealing Tank Commands:\r\n"
                         "htank [ enter | exit | check ]@n");
            return (true);
        }

        if (boost::iequals("enter", arg))
        {
            if (PLR_FLAGGED(ch, PLR_HEALT))
            {
                ch->sendText("You are already inside a healing tank!\r\n");
                return (true);
            }
            if (ch->master && ch->master != ch)
            {
                ch->sendText("You can't enter it while following someone!\r\n");
                return (true);
            }
            if (IS_ANDROID(ch))
            {
                ch->sendText("A healing tank will have no effect on you.\r\n");
                return (true);
            }
            if (htank->getBaseStat("energy") <= 0.0)
            {
                ch->sendText("That healing tank needs to recharge, wait a while.\r\n");
                return (true);
            }
            if (OBJ_FLAGGED(htank, ITEM_BROKEN))
            {
                ch->sendText("It is broken! You will need to fix it yourself or wait for someone else to fix it.\r\n");
                return (true);
            }
            if (SITS(ch))
            {
                ch->sendText("You are already on something.\r\n");
                return (true);
            }
            if (SITTING(htank))
            {
                ch->sendText("Someone else is already inside that healing tank!\r\n");
                return (true);
            }
            ch->setBaseStat<int64_t>("charge", 0);
            ch->player_flags.set(PLR_CHARGE, false);
            ch->setBaseStat<int64_t>("chargeto", 0);
            ch->setBaseStat<int64_t>("barrier", 0);
            act("@wYou step inside the healing tank and put on its breathing mask. A water like solution pours over your body until the tank is full.@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w steps inside the healing tank and puts on its breathing mask. A water like solution pours over $s body until the tank is full.@n",
                true, ch, nullptr, nullptr, TO_ROOM);
            ch->player_flags.set(PLR_HEALT, true);
            ch->sits = htank->shared_from_this();
            htank->sitting = ch->shared_from_this();
            objectSubscriptions.subscribe("healTankService", htank);
            return (true);

        } // End of Enter argument

        else if (boost::iequals("exit", arg))
        {
            if (!PLR_FLAGGED(ch, PLR_HEALT))
            {
                ch->sendText("You are not inside a healing tank.\r\n");
                return (true);
            }
            act("@wThe healing tank drains and you exit it shortly after.", true, ch, nullptr, nullptr, TO_CHAR);
            act("@C$n@w exits the healing tank after letting it drain.@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->player_flags.set(PLR_HEALT, false);
            htank->sitting.reset();
            ch->sits.reset();
            return (true);
        } // End of Exit argument

        else if (boost::iequals("check", arg))
        {
            int en = std::floor(htank->getBaseStat("energy"));
            if (en < 200 && en > 0)
            {
                ch->send_to("The healing tank has %d bars of energy displayed on its meter.\r\n", en);
            }
            else if (en <= 0)
            {
                ch->sendText("The healing tank has no energy displayed on its meter.\r\n");
            }
            else
            {
                ch->sendText("The healing tank has full energy shown on its meter.\r\n");
            }
            return (true);
        }
        else
        {
            ch->sendText("@WHealing Tank Commands:\r\n"
                         "htank [ enter | exit | check ]@n");
            return (true);
        }

    } // End of htank command
    else
    {
        return (false);
    }
}

/* This controls stat augmenter functions */
SPECIAL(augmenter)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (CMD_IS("augment"))
    {
        int strength = ch->getBaseStat("strength");
        int intel = ch->getBaseStat("intelligence");
        int wisdom = ch->getBaseStat("wisdom");
        int speed = ch->getBaseStat("speed");
        int consti = ch->getBaseStat("constitution");
        int agility = ch->getBaseStat("agility");

        int strcost = strength * 1200;
        int intcost = intel * 1200;
        int concost = consti * 1200;
        int wiscost = wisdom * 1200;
        int agicost = agility * 1200;
        int specost = speed * 1200;

        if (!*arg)
        {
            ch->sendText("@D                        -----@WBody Augmentations@D-----@n\r\n");
            ch->send_to("@RStrength    @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", strength, add_commas(strcost).c_str());
            ch->send_to("@BIntelligence@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", intel, add_commas(intcost).c_str());
            ch->send_to("@CWisdom      @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", wisdom, add_commas(wiscost).c_str());
            ch->send_to("@GConstitution@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", consti, add_commas(concost).c_str());
            ch->send_to("@mAgility     @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", agility, add_commas(agicost).c_str());
            ch->send_to("@YSpeed       @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", speed, add_commas(specost).c_str());
            ch->sendText("\r\n");
            return (true);
        }
        else if (boost::iequals("strength", arg) || boost::iequals("str", arg))
        {
            if (strength >= 100)
                ch->sendText("Your strength is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < strcost)
                ch->sendText("You can not afford the price!\r\n");
            else
            { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->modBaseStat("strength", 1);
                ch->modBaseStat("money_carried", -strcost);
            }
        }
        else if (boost::iequals("intelligence", arg) || boost::iequals("int", arg))
        {
            if (intel >= 100)
                ch->sendText("Your intelligence is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < intcost)
                ch->sendText("You can not afford the price!\r\n");
            else
            { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->modBaseStat("intelligence", 1);
                ch->modBaseStat("money_carried", -intcost);
            }
        }
        else if (boost::iequals("constitution", arg) || boost::iequals("con", arg))
        {
            if (consti >= 100)
                ch->sendText("Your constitution is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < concost)
                ch->sendText("You can not afford the price!\r\n");
            else
            { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->modBaseStat("constitution", 1);
                ch->modBaseStat("money_carried", -concost);
            }
        }
        else if (boost::iequals("speed", arg) || boost::iequals("spe", arg))
        {
            if (speed >= 100)
                ch->sendText("Your speed is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < specost)
                ch->sendText("You can not afford the price!\r\n");
            else
            { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->modBaseStat("speed", 1);
                ch->modBaseStat("money_carried", -specost);
            }
        }
        else if (boost::iequals("agility", arg) || boost::iequals("agi", arg))
        {
            if (agility >= 100)
                ch->sendText("Your agility is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < agicost)
                ch->sendText("You can not afford the price!\r\n");
            else
            { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->modBaseStat("agility", 1);
                ch->modBaseStat("money_carried", -agicost);
            }
        }
        else if (boost::iequals("wisdom", arg) || boost::iequals("wis", arg))
        {
            if (wisdom >= 100)
                ch->sendText("Your wisdom how somehow been measured is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < wiscost)
                ch->sendText("You can not afford the price!\r\n");
            else
            { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->modBaseStat("wisdom", 1);
                ch->modBaseStat("money_carried", -wiscost);
            }
        }
        else
        {
            ch->sendText("Syntax: augment [str | con | int | wis | agi | spe]\r\n");
        }
        return (true);
    }
    else
    { /* They are not using the right command, ignore them. */
        return (false);
    }
}

static std::map<std::string, double> gravMap = {
    {"0", 0.0},
    {"N", 0.0},
    {"n", 0.0},
    {"10", 10.0},
    {"20", 20.0},
    {"30", 30.0},
    {"40", 40.0},
    {"50", 50.0},
    {"100", 100.0},
    {"200", 200.0},
    {"300", 300.0},
    {"400", 400.0},
    {"500", 500.0},
    {"1000", 1000.0},
    {"5000", 5000.0},
    {"10000", 10000.0}};

/* This controls gravity generator functions */
SPECIAL(gravity)
{
    Object *i;
    char arg[MAX_INPUT_LENGTH];
    int match = false;

    one_argument(argument, arg);
    auto obj = ch->location.searchObjects(11);

    if (CMD_IS("gravity") || CMD_IS("generator"))
    {

        if (!*arg)
        {
            ch->sendText("@WGravity Commands:@n\r\n");
            ch->sendText("@Wgravity [ N | <num> ]\r\n");
            return (true);
        }

        if (OBJ_FLAGGED(obj, ITEM_BROKEN))
        {
            ch->sendText("It's broken!\r\n");
            return (true);
        }

        std::string a = arg;
        // remove all commas from a.
        a.erase(std::remove(a.begin(), a.end(), ','), a.end());
        double grav = 0.0;
        if (!boost::iequals(a, "N"))
        {
            try
            {
                grav = std::clamp<double>(std::stod(a), 0, 20000.0);
            }
            catch (std::exception &e)
            {
                ch->sendText("That is not an acceptable gravity setting.\r\n");
                return (true);
            }
        }

        bool doChange = false;

        if (obj->getEffectiveStat("gravity") > 0.0)
        {
            if (obj->getEffectiveStat("gravity") == grav)
            {
                ch->sendText("The gravity generator is already set to that.\r\n");
                return (true);
            }
            doChange = true;
        }
        else
        {
            if (grav > 0.0)
            {
                doChange = true;
            }
            else
            {
                ch->sendText("The gravity generator is already set to that.\r\n");
                return (true);
            }
        }

        if (doChange)
        {
            if (grav > 0.0)
            {
                auto msg = fmt::format("You punch in {} times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n", grav);
                ch->sendText(msg.c_str());
                obj->setBaseStat("gravity", grav);
                if (ch->location.getRoomFlag(ROOM_AURA))
                {
                    ch->location.sendText("The increased gravity forces the aura to disappear.\r\n");
                    ch->location.setRoomFlag(ROOM_AURA, false);
                }
            }
            else
            {
                ch->sendText("You punch in normal gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
                obj->setBaseStat("gravity", 0.0);
            }
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);
        }

        return (true);
    }
    else
    {
        return (false);
    }
}

SPECIAL(bank)
{
    int amount, num = 0;

    auto obj = ch->location.searchObjects(3034);

    if (CMD_IS("balance"))
    {
        if (OBJ_FLAGGED(obj, ITEM_BROKEN))
        {
            ch->sendText("The ATM is broken!\r\n");
            return (true);
        }

        if (GET_BANK_GOLD(ch) > 0)
            ch->send_to("Your current balance is %d zenni.\r\n", GET_BANK_GOLD(ch));
        else
            ch->sendText("You currently have no money deposited.\r\n");
        return (true);
    }
    else if (CMD_IS("wire"))
    {
        char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
        Character *vict = nullptr;

        two_arguments(argument, arg, arg2);

        if (OBJ_FLAGGED(obj, ITEM_BROKEN))
        {
            ch->sendText("The ATM is broken!\r\n");
            return (true);
        }

        if ((amount = atoi(arg)) <= 0)
        {
            ch->sendText("How much do you want to transfer?\r\n");
            return (true);
        }
        if (GET_BANK_GOLD(ch) < amount + (amount / 100))
        {
            ch->send_to("You don't have that much zenni in the bank (plus 1%s charge)!\r\n", "%");
            return (true);
        }
        if (!*arg2)
        {
            ch->sendText("You want to transfer it to who?!\r\n");
            return (true);
        }
        if (!(vict = get_player_vis(ch, arg2, nullptr, FIND_CHAR_WORLD)))
        {
            int is_file = false, player_i = 0;
            char name[MAX_INPUT_LENGTH];
            sprintf(name, "%s", rIntro(ch, arg2));

            vict = findPlayer(name);

            if (!vict)
            {
                ch->sendText("That person doesn't exist.\r\n");
                return (true);
            }

            if (ch->desc->account == nullptr)
            {
                ch->sendText("There is an error. Report to staff.");
                return (true);
            }
            auto id = vict->id;
            auto &p = players.at(id);
            auto &c = p->account->characters;
            auto found = std::find_if(c.begin(), c.end(), [&](auto i)
                                      { return i == id; });
            if (found != c.end())
            {
                ch->sendText("You can not transfer money to your own offline characters...");
                return (true);
            }
            vict->modBaseStat("money_bank", amount);
            ch->modBaseStat("money_bank", -(amount + (amount / 100)));
            mudlog(NRM, std::max(ADMLVL_IMPL, GET_INVIS_LEV(ch)), true, "EXCHANGE: %s gave %s zenni to user %s",
                   GET_NAME(ch), add_commas(amount).c_str(), GET_NAME(vict));
        }
        else
        {
            vict->modBaseStat("money_bank", amount);
            ch->modBaseStat("money_bank", -(amount + (amount / 100)));
            vict->send_to("@WYou have just had @Y%s@W zenni wired into your bank account.@n\r\n", add_commas(amount).c_str());
        }
        ch->send_to("You transfer %s zenni to them.\r\n", add_commas(amount).c_str());
        act("$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
        return (true);
    }
    else if (CMD_IS("deposit"))
    {

        if (OBJ_FLAGGED(obj, ITEM_BROKEN))
        {
            ch->sendText("The ATM is broken!\r\n");
            return (true);
        }

        if ((amount = atoi(argument)) <= 0)
        {
            ch->sendText("How much do you want to deposit?\r\n");
            return (true);
        }
        if (GET_GOLD(ch) < amount)
        {
            ch->sendText("You don't have that much zenni!\r\n");
            return (true);
        }
        ch->modBaseStat("money_carried", -amount);
        ch->modBaseStat("money_bank", amount);
        ch->send_to("You deposit %d zenni.\r\n", amount);
        act("$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
        return (true);
    }
    else if (CMD_IS("withdraw"))
    {

        if (OBJ_FLAGGED(obj, ITEM_BROKEN))
        {
            ch->sendText("The ATM is broken!\r\n");
            return (true);
        }

        if ((amount = atoi(argument)) <= 0)
        {
            ch->sendText("How much do you want to withdraw?\r\n");
            return (true);
        }
        if (GET_BANK_GOLD(ch) < amount)
        {
            ch->sendText("You don't have that much zenni!\r\n");
            return (true);
        }
        if (GET_BANK_GOLD(ch) - (amount + (1 + amount / 100)) < 0)
        {
            if (amount >= 100)
            {
                amount = amount + (amount / 100);
            }
            else if (amount < 100)
            {
                amount = amount + 1;
            }
            ch->send_to("You need at least %s in the bank with the 1 percent withdraw fee.\r\n", add_commas(amount).c_str());
            return (true);
        }
        if (GET_GOLD(ch) + amount > GOLD_CARRY(ch))
        {
            ch->send_to("You can only carry %s zenni, you left the rest.\r\n", add_commas(GOLD_CARRY(ch)).c_str());
            int diff = (GET_GOLD(ch) + amount) - GOLD_CARRY(ch);
            ch->setBaseStat("money_carried", GOLD_CARRY(ch));
            amount -= diff;
            if (amount >= 100)
            {
                num = amount / 100;
                ch->modBaseStat("money_bank", -(amount + num));
            }
            else if (amount < 100)
            {
                ch->modBaseStat("money_bank", -(amount + 1));
            }
            ch->send_to("You withdraw %s zenni,  and pay %s in withdraw fees.\r\n.\r\n", add_commas(amount).c_str(), add_commas(num).c_str());
            act("$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
            return (true);
        }
        ch->modBaseStat("money_carried", amount);
        if (amount >= 100)
        {
            num = amount / 100;
            ch->modBaseStat("money_bank", -(amount + num));
        }
        else if (amount < 100)
        {
            ch->modBaseStat("money_bank", -(amount + 1));
        }
        ch->send_to("You withdraw %s zenni, and pay %s in withdraw fees.\r\n", add_commas(amount).c_str(), add_commas(num).c_str());
        act("$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
        return (true);
    }
    else
        return (false);
}
