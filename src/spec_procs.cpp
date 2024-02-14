/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/spec_procs.h"

#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/spells.h"
#include "dbat/constants.h"
#include "dbat/mail.h"
#include "dbat/act.movement.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/guild.h"
#include "dbat/races.h"
#include "dbat/act.comm.h"
#include "dbat/class.h"
#include "dbat/players.h"
#include "dbat/act.informative.h"

/* local functions */


/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

SPECIAL(dump) {
    int value = 0;

    for (auto k : ch->getRoom()->getInventory()) {
        act("$p vanishes in a puff of smoke!", false, nullptr, k, nullptr, TO_ROOM);
        extract_obj(k);
    }

    if (!CMD_IS("drop"))
        return (false);

    do_drop(ch, argument, cmd, SCMD_DROP);

    for (auto k : ch->getRoom()->getInventory()) {
        act("$p vanishes in a puff of smoke!", false, nullptr, k, nullptr, TO_ROOM);
        value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
        extract_obj(k);
    }

    if (value) {
        send_to_char(ch, "You are awarded for outstanding performance.\r\n");
        act("$n has been awarded for being a good citizen.", true, ch, nullptr, nullptr, TO_ROOM);

        if (GET_LEVEL(ch) < 3) {
            ch->modExperience(value);
        }
        else
            ch->mod(CharMoney::Carried, value);
    }
    return (true);
}



/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */

int num_players_in_room(room_vnum room) {
    struct descriptor_data *i;
    int num_players = 0;

    for (i = descriptor_list; i; i = i->next) {
        if (STATE(i) != CON_PLAYING)
            continue;
        if (!(i->character))
            continue;
        if (!world.count(IN_ROOM(i->character)))
            continue;
        if (i->character->getRoom()->vn != room)
            continue;
        if ((GET_ADMLEVEL(i->character) >= ADMLVL_IMMORT) &&
            (PRF_FLAGGED(i->character, PRF_NOHASSLE))) /* Ignore Imms */
            continue;

        num_players++;
    }

    return num_players;
}

bool check_mob_in_room(mob_vnum mob, room_vnum room) {
    if(auto u = world.find(room); u != world.end()) {
        auto r = dynamic_cast<room_data*>(u->second);
        for(auto i = r->people; i; i = i->next_in_room) {
            if(i->vn == mob) return true;
        }
    }
    return false;
}

bool check_obj_in_room(obj_vnum obj, room_vnum room) {

    bool found = false;
    room_rnum r_room;

    r_room = real_room(room);
    if(r_room == NOWHERE) return false;
    return world.at(r_room)->findObjectVnum(obj) != nullptr;
}

static const int gauntlet_info[][3] = {  /* --mystic 26 Oct 2005 */

/* Gauntlet Room Scoring*/
/* Num  Rm Num   Direction   */
        {0,  2403, SCMD_SOUTH},  /* Waiting Room     */
        {1,  2404, SCMD_SOUTH},  /* About level 5    */
        {2,  2405, SCMD_SOUTH},  /* About level 10   */
        {3,  2406, SCMD_SOUTH},  /* About level 15   */
        {4,  2407, SCMD_SOUTH},  /* About level 20   */
        {5,  2408, SCMD_SOUTH},  /* About level 25   */
        {6,  2409, SCMD_SOUTH},  /* About level 30   */
        {7,  2410, SCMD_SOUTH},  /* About level 35   */
        {8,  2411, SCMD_SOUTH},  /* About level 40   */
        {9,  2412, SCMD_SOUTH},  /* About level 45   */
        {10, 2413, SCMD_SOUTH},  /* About level 50   */
        {11, 2414, SCMD_SOUTH},  /* About level 55   */
        {12, 2415, SCMD_SOUTH},  /* About level 60   */
        {13, 2416, SCMD_SOUTH},  /* About level 65   */
        {14, 2417, SCMD_SOUTH},  /* About level 70   */
        {15, 2418, SCMD_SOUTH},  /* About level 75   */
        {16, 2420, SCMD_SOUTH},  /* About level 80   */
        {17, 2421, SCMD_SOUTH},  /* About level 85   */
        {18, 2422, SCMD_SOUTH},  /* About level 90   */
        {19, 2423, SCMD_SOUTH},  /* About level 95   */
        {20, 2424, SCMD_SOUTH},  /* About level 100  */
        {21, 2425, SCMD_SOUTH},  /* About level 5    */
        {22, 2426, SCMD_SOUTH},  /* About level 10   */
        {23, 2427, SCMD_SOUTH},  /* About level 15   */
        {24, 2428, SCMD_SOUTH},  /* About level 20   */
        {25, 2429, SCMD_SOUTH},  /* About level 25   */
        {26, 2430, SCMD_SOUTH},  /* About level 30   */
        {27, 2431, SCMD_SOUTH},  /* About level 35   */
        {28, 2432, SCMD_SOUTH},  /* About level 40   */
        {29, 2433, SCMD_SOUTH},  /* About level 45   */
        {30, 2434, SCMD_SOUTH},  /* About level 50   */
        {31, 2435, SCMD_SOUTH},  /* About level 55   */
        {32, 2436, SCMD_SOUTH},  /* About level 60   */
        {33, 2437, SCMD_SOUTH},  /* About level 65   */
        {34, 2438, SCMD_SOUTH},  /* About level 70   */
        {35, 2439, SCMD_SOUTH},  /* About level 75   */
        {36, 2440, SCMD_SOUTH},  /* About level 80   */
        {37, 2441, SCMD_SOUTH},  /* About level 85   */
        {38, 2442, SCMD_SOUTH},  /* About level 90   */
        {39, 2443, SCMD_SOUTH},  /* About level 95   */
        {40, 2444, SCMD_SOUTH},  /* About level 100  */
        {-1, -1, -1}
};

SPECIAL(gauntlet_room)  /* Jamdog - 13th Feb 2006 */
{
    int i = 0;
    int proceed = 1;
    struct char_data *tch;
    char *buf2 = "$N tried to sneak past without a fight, and got nowhere.";
    char buf[MAX_STRING_LENGTH];
    bool nomob = true;

    /* give player credit for making it this far */
    for (i = 0; gauntlet_info[i][0] != -1; i++) {
        if ((!IS_NPC(ch)) && (ch->getRoom()->vn == gauntlet_info[i][1])) {
            /* Check not overwriting gauntlet rank with lower value (Jamdog - 20th July 2006) */
            if (GET_GAUNTLET(ch) < (gauntlet_info[i][0])) {
                //set player's gauntlet rank
                GET_GAUNTLET(ch) = (gauntlet_info[i][0]);
            }
        }
    }

    if (!cmd)                        /* If no command, then nothing to do             */
        return false;

    if (CMD_IS("flee")) {
        send_to_char(ch,
                     "Fleeing is not allowed!  If you want to get out of here, type @Ysurrender@n while fighting to be returned to the start.");
        return true;
    }

    if (!IS_MOVE(cmd) && !CMD_IS("surrender")) /* Only movement commands need to be checked     */
        return false;

    if (IS_NPC(ch))                  /* Mobs can move about - Jamdog 20th July 2006   */
        return false;                  /* This also allows following pets!              */

    if (CMD_IS("surrender")) {
        if (FIGHTING(ch)) {
            /* OK, player has had enough - position is already stored, so throw them back to the start */
            ch->removeFromLocation();
            ch->addToLocation(world[real_room(gauntlet_info[0][1])]);
            act("$n suddenly appears looking relieved after $s trial in the Gauntlet", false, ch, nullptr, ch,
                TO_NOTVICT);
            act("You are returned to the start of the Gauntlet", false, ch, nullptr, ch, TO_VICT);

            /* Hit point penalty for surrendering */
            ch->decCurHealth(2000);

            look_at_room(IN_ROOM(ch), ch, 0);
            return true;
        } else {
            send_to_char(ch, "You can only surrender while fighting, so at least TRY to make an effort");
            return true;
        }
    }

    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) /* Imms can walk through the gauntlet unhindered */
        return false;

    /* Only Avatars may pass the 11th room
    if ((world[ch->in_room].number == gauntlet_info[GAUNTLET_AV][1] ) && (cmd == gauntlet_info[GAUNTLET_AV][2]))
    {
      if (GET_CLASS(ch) != CLASS_AVATAR)
      {
        send_to_char (ch, "Only Avatars may proceed further!\r\n");
        return TRUE;
      }
    } */
    for (i = 0; gauntlet_info[i][0] != -1; i++) {
        auto r = ch->getRoom();
        if (r->vn == gauntlet_info[i][1]) {
            if (cmd == gauntlet_info[i][2]) {
                //don't let him proceed if mob is still alive
                for (tch = r->people; tch; tch = tch->next_in_room) {
                    if (IS_NPC(tch) && i > 0)  /* Ignore mobs in the waiting room */
                    {
                        proceed = 0;
                        sprintf(buf, "%s wants to teach you a lesson first.\r\n", GET_NAME(tch));
                    }
                }
                /* In the case of the phoenix room, don't progress if there is 1st or 2nd ashes */
                /* TODO: (add in here when ash is created) */
                if (proceed) {
                    nomob = true;

                    /* Check the next room for players and ensure mob is waiting */
                    for (tch = dynamic_cast<room_data*>(world[real_room(gauntlet_info[i + 1][1])])->people; tch; tch = tch->next_in_room) {
                        if (!IS_NPC(tch)) {
                            proceed = 0;  /* There is a player there */
                            sprintf(buf, "%s is in the next room.  You must wait for them to finish.\r\n",
                                    GET_NAME(tch));
                        } else {
                            nomob = false;
                        }
                    }
                    if (nomob == true) {
                        proceed = 0;  /* There is no mob there in the next room */
                        sprintf(buf, "The next room is empty.  You must wait for your opponent to re-appear.\r\n");
                    }
                }

                if (proceed == 0) {
                    send_to_char(ch, buf);
                    act(buf2, false, ch, nullptr, ch, TO_ROOM);
                    return true;
                }
            }
        }
    }
    return false;
}

SPECIAL(gauntlet_end)  /* Jamdog - 20th Feb 2007 */
{
    int i = 0;

    /* give player credit for making it this far */
    if (!IS_NPC(ch)) {
        /* Check not overwriting gauntlet rank with lower value (Jamdog - 20th July 2006) */
        if (GET_GAUNTLET(ch) < GAUNTLET_END) {
            //set player's gauntlet rank
            GET_GAUNTLET(ch) = GAUNTLET_END;
        }
    }

    if (!cmd)                        /* If no command, then nothing to do             */
        return false;

    if (CMD_IS("flee")) {
        if ((FIGHTING(ch)) && (GET_POS(ch) == POS_FIGHTING)) {
            send_to_char(ch,
                         "You can't flee from this fight./r/nIt's your own fault for summoning creatures into the gauntlet!\r\n");
            return true;
        } else {
            send_to_char(ch, "There is nothing here to flee from\r\n");
            return true;
        }
    }

    if (CMD_IS("surrender")) {
        send_to_char(ch, "You have completed the gauntlet, why would you need to surrender?\r\n");
        return true;
    }

    if (!IS_MOVE(cmd)) /* Only movement commands need to be checked     */
        return false;

    if (IS_NPC(ch))                  /* Mobs can move about - Jamdog 20th July 2006   */
        return false;                  /* This also allows following pets!              */

    if (!EXIT(ch, cmd - 1) || EXIT(ch, cmd - 1)->to_room == NOWHERE)
        return false;
    if (EXIT_FLAGGED(EXIT(ch, cmd - 1), EX_CLOSED))
        return false;

    for (i = 0; gauntlet_info[i][0] != -1; i++) {
        if (world[EXIT(ch, (cmd - 1))->to_room]->vn == gauntlet_info[i][1]) {
            send_to_char(ch, "You have completed the gauntlet, you cannot go backwards!\r\n");
            return true;
        }
    }
    return false;
}

SPECIAL(gauntlet_rest)  /* Jamdog - 20th Feb 2007 */
{
    int i = 0;
    int proceed = 1, door;
    struct char_data *tch;
    char *buf2 = "$N tried to return to the gauntlet, and got nowhere.";
    char buf[MAX_STRING_LENGTH];
    bool nomob = true;

    if (!cmd)                        /* If no command, then nothing to do             */
        return false;

    if (CMD_IS("flee")) {
        send_to_char(ch,
                     "Fleeing is not allowed!  If you want to get out of here, type @Ysurrender@n while fighting to be returned to the start.");
        return true;
    }

    if (CMD_IS("surrender")) {
        send_to_char(ch,
                     "You are in a rest-room.  Surrender is not an option.\r\nIf you want to leave the Gauntlet, you can surrender while fighting.\r\n");
        return true;
    }

    if (!IS_MOVE(cmd)) /* Only movement commands need to be checked     */
        return false;

    if (IS_NPC(ch))                  /* Mobs can move about - Jamdog 20th July 2006   */
        return false;                  /* This also allows following pets!              */

    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) /* Imms can walk through the gauntlet unhindered */
        return false;

    for (i = 0; gauntlet_info[i][0] != -1; i++) {
        for (door = 0; door < NUM_OF_DIRS; door++) {
            if (!EXIT(ch, door) || EXIT(ch, door)->to_room == NOWHERE)
                continue;
            if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
                continue;

            if ((world[EXIT(ch, door)->to_room]->vn == gauntlet_info[i][1]) && (door == (cmd - 1))) {
                nomob = true;

                /* Check the next room for players and ensure mob is waiting */
                for (tch = dynamic_cast<room_data*>(world[real_room(gauntlet_info[i][1])])->people; tch; tch = tch->next_in_room) {
                    if (!IS_NPC(tch)) {
                        proceed = 0;  /* There is a player there */
                        sprintf(buf, "%s has moved into the next room.  You must wait for them to finish.\r\n",
                                GET_NAME(tch));
                    } else {
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
                if (proceed == 0) {
                    send_to_char(ch, buf);
                    act(buf2, false, ch, nullptr, ch, TO_ROOM);
                    return true;
                }
            }
        }
    }
    return false;
}

void npc_steal(struct char_data *ch, struct char_data *victim) {
    int gold;

    if (IS_NPC(victim))
        return;
    if (IS_NPC(ch))
        return;
    if (ADM_FLAGGED(victim, ADM_NOSTEAL))
        return;
    if (!CAN_SEE(ch, victim))
        return;

    if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0)) {
        act("You discover that $n has $s hands in your wallet.", false, ch, nullptr, victim, TO_VICT);
        act("$n tries to steal zenni from $N.", true, ch, nullptr, victim, TO_NOTVICT);
    } else {
        /* Steal some gold coins */
        gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
        if (gold > 0) {
            ch->mod(CharMoney::Carried, gold);
            victim->mod(CharMoney::Carried, -gold);
        }
    }
}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */
#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops) {
    char buf[MAX_STRING_LENGTH], pet_name[256];
    room_rnum pet_room;
    struct char_data *pet;

    /* Gross. */
    pet_room = IN_ROOM(ch) + 1;

    if (CMD_IS("list")) {
        send_to_char(ch, "Available pets are:\r\n");
        for (pet = dynamic_cast<room_data*>(world[pet_room])->people; pet; pet = pet->next_in_room) {
            /* No, you can't have the Implementor as a pet if he's in there. */
            if (!IS_NPC(pet))
                continue;
            send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
        }
        return (true);
    } else if (CMD_IS("buy")) {

        two_arguments(argument, buf, pet_name);

        if (!(pet = get_char_room(buf, nullptr, pet_room)) || !IS_NPC(pet)) {
            send_to_char(ch, "There is no such pet!\r\n");
            return (true);
        }
        if (GET_GOLD(ch) < PET_PRICE(pet)) {
            send_to_char(ch, "You don't have enough zenni!\r\n");
            return (true);
        }
        ch->mod(CharMoney::Carried, -PET_PRICE(pet));

        pet = read_mobile(GET_MOB_RNUM(pet), REAL);
        pet->affected_by.set(AFF_CHARM);

        if (*pet_name) {
            snprintf(buf, sizeof(buf), "%s %s", pet->name, pet_name);
            /* free(pet->name); don't free the prototype! */
            pet->name = strdup(buf);

            snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
                     pet->look_description, pet_name);
            /* free(pet->description); don't free the prototype! */
            pet->look_description = strdup(buf);
        }
        pet->addToLocation(ch->getRoom());
        add_follower(pet, ch);
        pet->master_id = GET_IDNUM(ch);

        send_to_char(ch, "May you enjoy your pet.\r\n");
        act("$n buys $N as a pet.", false, ch, nullptr, pet, TO_ROOM);

        return (true);
    }

    /* All commands except list and buy */
    return (false);
}

SPECIAL(auction) {
    room_rnum auct_room;
    struct obj_data *obj, *next_obj, *obj2 = nullptr;
    int found = false;

    /* Gross. */
    auct_room = real_room(80);

    if (CMD_IS("cancel")) {

        for (auto obj : world[auct_room]->getInventory()) {
            if (obj && GET_AUCTER(obj) == ((ch)->uid)) {
                obj2 = obj;
                found = true;

                if (GET_CURBID(obj2) != -1 && GET_AUCTIME(obj2) + 518400 > time(nullptr)) {
                    send_to_char(ch,
                                 "Unable to cancel. Someone has already bid on it and their bid license hasn't expired.\r\n");
                    time_t remain = (GET_AUCTIME(obj2) + 518400) - time(nullptr);
                    int day = (int) ((remain % 604800) / 86400);
                    int hour = (int) ((remain % 86400) / 3600);
                    int minu = (int) ((remain % 3600) / 60);
                    send_to_char(ch, "Time Till License Expiration: %d day%s, %d hour%s, %d minute%s.\r\n", day,
                                 day > 1 ? "s" : "", hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
                    continue;
                }

                send_to_char(ch, "@wYou cancel the auction of %s@w and it is returned to you.@n\r\n",
                             obj2->getShortDesc());
                struct descriptor_data *d;

                for (d = descriptor_list; d; d = d->next) {
                    if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                        continue;
                    if (d->character == ch)
                        continue;
                    if (GET_EQ(d->character, WEAR_EYE)) {
                        send_to_char(d->character,
                                     "@RScouter Auction News@D: @GThe auction of @w%s@G has been canceled.\r\n",
                                     obj2->getShortDesc());
                    }
                }

                obj2->removeFromLocation();
                obj2->addToLocation(ch);
                auc_save();
            }
        }

        if (found == false) {
            send_to_char(ch, "There are no items being auctioned by you.\r\n");
        }

        return (true);
    } else if (CMD_IS("pickup")) {
        struct descriptor_data *d;
        int founded = false;

        for (auto obj : world[auct_room]->getInventory()) {
            if (obj && GET_CURBID(obj) == ((ch)->uid)) {
                obj2 = obj;
                found = true;

                if (GET_AUCTER(obj) <= 0) {
                    continue;
                }

                if (GET_BID(obj2) > GET_GOLD(ch)) {
                    send_to_char(ch, "Unable to purchase %s, you don't have enough money on hand.\r\n",
                                 obj2->getShortDesc());
                    continue;
                }

                if (GET_AUCTIME(obj2) + 86400 > time(nullptr)) {
                    time_t remain = (GET_AUCTIME(obj2) + 86400) - time(nullptr);
                    int hour = (int) ((remain % 86400) / 3600);
                    int minu = (int) ((remain % 3600) / 60);
                    send_to_char(ch,
                                 "Unable to purchase %s, minimum time to bid is 24 hours. %d hour%s and %d minute%s remain.\r\n",
                                 obj2->getShortDesc(), hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
                    continue;
                }

                ch->mod(CharMoney::Carried, -GET_BID(obj2));
                obj2->removeFromLocation();
                obj2->addToLocation(ch);
                send_to_char(ch, "You pay %s zenni and receive the item.\r\n", add_commas(GET_BID(obj2)).c_str());
                auc_save();

                for (d = descriptor_list; d; d = d->next) {
                    if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                        continue;
                    if (d->character == ch)
                        continue;
                    if (GET_IDNUM(d->character) == GET_AUCTER(obj2)) {
                        founded = true;
                        d->character->mod(CharMoney::Carried, GET_BID(obj2));
                        if (GET_EQ(d->character, WEAR_EYE)) {
                            send_to_char(d->character,
                                         "@RScouter Auction News@D: @GSomeone has purchased your @w%s@G and you had the money put in your bank account.\r\n",
                                         obj2->getShortDesc());
                        }
                    } else if (GET_EQ(d->character, WEAR_EYE)) {
                        send_to_char(d->character,
                                     "@RScouter Auction News@D: @GSomeone has purchased the @w%s@G that was on auction.\r\n",
                                     obj2->getShortDesc());
                    }
                }

                if (founded == false) {
                    char blam[50];
                    sprintf(blam, "%s", GET_AUCTERN(obj2));
                    auto vict = findPlayer(blam);
                    if(!vict) continue;

                    vict->mod(CharMoney::Bank, GET_BID(obj2));
                }

            }
        }

        if (found == false) {
            send_to_char(ch, "There are no items that you have bid on.\r\n");
        }
        return (true);
    } else if (CMD_IS("auction")) {
        char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
        struct descriptor_data *d;
        int value = 0;

        two_arguments(argument, arg, arg2);

        if (!*arg || !*arg2) {
            send_to_char(ch, "Auction what item and for how much?\r\n");
            return (true);
        }

        value = atoi(arg2);

        if (!(obj2 = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory()))) {
            send_to_char(ch, "You don't have that item to auction.\r\n");
            return (true);
        }
        if (value <= 999) {
            send_to_char(ch, "Do not auction anything for less than 1,000 zenni.\r\n");
            return (true);
        }

        if (OBJ_FLAGGED(obj2, ITEM_BROKEN)) {
            act("$P is broken and we will not accept it.", false, ch, nullptr, obj2, TO_CHAR);
            return (true);
        }

        if (OBJ_FLAGGED(obj2, ITEM_NODONATE)) {
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
        GET_AUCTER(obj2) = ((ch)->uid);
        GET_AUCTERN(obj2) = strdup(GET_NAME(ch));
        GET_AUCTIME(obj2) = time(nullptr);
        GET_CURBID(obj2) = -1;
        obj2->removeFromLocation();
        obj2->addToLocation(world.at(auct_room));
        auc_save();
        send_to_char(ch, "You place %s on auction for %s zenni.\r\n", obj2->getShortDesc(),
                     add_commas(GET_BID(obj2)).c_str());
        basic_mud_log("AUCTION: %s places %s on auction for %s", GET_NAME(ch), obj2->getShortDesc(),
            add_commas(GET_BID(obj2)).c_str());

        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                continue;
            if (d->character == ch)
                continue;
            if (GET_EQ(d->character, WEAR_EYE)) {
                send_to_char(d->character,
                             "@RScouter Auction News@D: @GThe item, @w%s@G, has been placed on auction for @Y%s@G zenni.@n\r\n",
                             obj2->getShortDesc(), add_commas(GET_BID(obj2)).c_str());
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

SPECIAL(healtank) {
    struct obj_data *htank = nullptr, *i;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    htank = ch->getRoom()->findObjectVnum(65);

    if (CMD_IS("htank")) {
        if (!htank) {
            return (false);
        }

        if (!*arg) {
            send_to_char(ch, "@WHealing Tank Commands:\r\n"
                             "htank [ enter | exit | check ]@n");
            return (true);
        }

        if (!strcasecmp("enter", arg)) {
            if (PLR_FLAGGED(ch, PLR_HEALT)) {
                send_to_char(ch, "You are already inside a healing tank!\r\n");
                return (true);
            }
            if (ch->master && ch->master != ch) {
                send_to_char(ch, "You can't enter it while following someone!\r\n");
                return (true);
            } else if (IS_ANDROID(ch)) {
                send_to_char(ch, "A healing tank will have no effect on you.\r\n");
                return (true);
            } else if (HCHARGE(htank) <= 0) {
                send_to_char(ch, "That healing tank needs to recharge, wait a while.\r\n");
                return (true);
            } else if (OBJ_FLAGGED(htank, ITEM_BROKEN)) {
                send_to_char(ch,
                             "It is broken! You will need to fix it yourself or wait for someone else to fix it.\r\n");
                return (true);
            } else if (SITS(ch)) {
                send_to_char(ch, "You are already on something.\r\n");
                return (true);
            } else if (SITTING(htank)) {
                send_to_char(ch, "Someone else is already inside that healing tank!\r\n");
                return (true);
            } else {
                GET_CHARGE(ch) = 0;
                ch->playerFlags.reset(PLR_CHARGE);
                GET_CHARGETO(ch) = 0;
                GET_BARRIER(ch) = 0;
                act("@wYou step inside the healing tank and put on its breathing mask. A water like solution pours over your body until the tank is full.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@w steps inside the healing tank and puts on its breathing mask. A water like solution pours over $s body until the tank is full.@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                ch->playerFlags.set(PLR_HEALT);
                SITS(ch) = htank;
                SITTING(htank) = ch;
                return (true);
            }

        } // End of Enter argument

        else if (!strcasecmp("exit", arg)) {
            if (!PLR_FLAGGED(ch, PLR_HEALT)) {
                send_to_char(ch, "You are not inside a healing tank.\r\n");
                return (true);
            } else {
                act("@wThe healing tank drains and you exit it shortly after.", true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@w exits the healing tank after letting it drain.@n", true, ch, nullptr, nullptr, TO_ROOM);
                ch->playerFlags.reset(PLR_HEALT);
                SITTING(htank) = nullptr;
                SITS(ch) = nullptr;
                return (true);
            }
        } // End of Exit argument

        else if (!strcasecmp("check", arg)) {
            if (HCHARGE(htank) < 20 && HCHARGE(htank) > 0) {
                send_to_char(ch, "The healing tank has %d bars of energy displayed on its meter.\r\n", HCHARGE(htank));
            } else if (HCHARGE(htank) <= 0) {
                send_to_char(ch, "The healing tank has no energy displayed on its meter.\r\n");
            } else {
                send_to_char(ch, "The healing tank has full energy shown on its meter.\r\n");
            }
            return (true);
        } else {
            send_to_char(ch, "@WHealing Tank Commands:\r\n"
                             "htank [ enter | exit | check ]@n");
            return (true);
        }

    }// End of htank command
    else {
        return (false);
    }
}

/* This controls stat augmenter functions */
SPECIAL(augmenter) {
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (CMD_IS("augment")) {
        int strength = ch->get(CharAttribute::Strength, true);
        int intel = ch->get(CharAttribute::Intelligence, true);
        int wisdom = ch->get(CharAttribute::Wisdom, true);
        int speed = ch->get(CharAttribute::Speed, true);
        int consti = ch->get(CharAttribute::Constitution, true);
        int agility = ch->get(CharAttribute::Agility, true);

        int strcost = strength * 1200;
        int intcost = intel * 1200;
        int concost = consti * 1200;
        int wiscost = wisdom * 1200;
        int agicost = agility * 1200;
        int specost = speed * 1200;

        if (!*arg) {
            send_to_char(ch, "@D                        -----@WBody Augmentations@D-----@n\r\n");
            send_to_char(ch, "@RStrength    @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n",
                         strength, add_commas(strcost).c_str());
            send_to_char(ch, "@BIntelligence@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", intel,
                         add_commas(intcost).c_str());
            send_to_char(ch, "@CWisdom      @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", wisdom,
                         add_commas(wiscost).c_str());
            send_to_char(ch, "@GConstitution@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", consti,
                         add_commas(concost).c_str());
            send_to_char(ch, "@mAgility     @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", agility,
                         add_commas(agicost).c_str());
            send_to_char(ch, "@YSpeed       @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", speed,
                         add_commas(specost).c_str());
            send_to_char(ch, "\r\n");
            return (true);
        } else if (!strcasecmp("strength", arg) || !strcasecmp("str", arg)) {
            if (strength >= 100)
                send_to_char(ch, "Your strength is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < strcost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->mod(CharAttribute::Strength, 1);
                ch->mod(CharMoney::Carried, -strcost);
            }
        } else if (!strcasecmp("intelligence", arg) || !strcasecmp("int", arg)) {
            if (intel >= 100)
                send_to_char(ch, "Your intelligence is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < intcost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->mod(CharAttribute::Intelligence, 1);
                ch->mod(CharMoney::Carried, -intcost);
            }
        } else if (!strcasecmp("constitution", arg) || !strcasecmp("con", arg)) {
            if (consti >= 100)
                send_to_char(ch, "Your constitution is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < concost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->mod(CharAttribute::Constitution, 1);
                ch->mod(CharMoney::Carried, -concost);
            }
        } else if (!strcasecmp("speed", arg) || !strcasecmp("spe", arg)) {
            if (speed >= 100)
                send_to_char(ch, "Your speed is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < specost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->mod(CharAttribute::Speed, 1);
                ch->mod(CharMoney::Carried, -specost);
            }
        } else if (!strcasecmp("agility", arg) || !strcasecmp("agi", arg)) {
            if (agility >= 100)
                send_to_char(ch, "Your agility is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < agicost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->mod(CharAttribute::Agility, 1);
                ch->mod(CharMoney::Carried, -agicost);
            }
        } else if (!strcasecmp("wisdom", arg) || !strcasecmp("wis", arg)) {
            if (wisdom >= 100)
                send_to_char(ch, "Your wisdom how somehow been measured is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < wiscost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", true, ch,
                    nullptr, nullptr, TO_ROOM);
                ch->mod(CharAttribute::Wisdom, 1);
                ch->mod(CharMoney::Carried, -wiscost);
            }
        } else {
            send_to_char(ch, "Syntax: augment [str | con | int | wis | agi | spe]\r\n");
        }
        ch->save();
        return (true);
    } else { /* They are not using the right command, ignore them. */
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
        {"10000", 10000.0}
};

/* This controls gravity generator functions */
SPECIAL(gravity) {
    struct obj_data *obj = nullptr;
    char arg[MAX_INPUT_LENGTH];
    int match = false;

    one_argument(argument, arg);
    obj = ch->getRoom()->findObjectVnum(11);
    if (CMD_IS("gravity") || CMD_IS("generator")) {

        if (!*arg) {
            send_to_char(ch, "@WGravity Commands:@n\r\n");
            send_to_char(ch, "@Wgravity [ N | <num> ]\r\n");
            return (true);
        }

        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "It's broken!\r\n");
            return (true);
        }

        std::string a = arg;
        // remove all commas from a.
        a.erase(std::remove(a.begin(), a.end(), ','), a.end());
        double grav = 0.0;
        if(!iequals(a, "N")) {
            try {
                grav = std::clamp<double>(std::stod(a), 0, 20000.0);
            } catch (std::exception &e) {
                send_to_char(ch, "That is not an acceptable gravity setting.\r\n");
                return (true);
            }
        }

        bool doChange = false;

        if(obj->gravity) {
            if(obj->gravity.value() == grav) {
                send_to_char(ch, "The gravity generator is already set to that.\r\n");
                return (true);
            }
            doChange = true;
        } else {
            if(grav > 0.0) {
                doChange = true;
            } else {
                send_to_char(ch, "The gravity generator is already set to that.\r\n");
                return (true);
            }
        }

        if(doChange) {
            if(grav > 0.0) {
                auto msg = fmt::format("You punch in {} times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n", grav);
                send_to_char(ch, msg.c_str());
                obj->gravity = grav;
                auto room = ch->getRoom();
                if (room->checkFlag(FlagType::Room, ROOM_AURA)) {
                    room->clearFlag(FlagType::Room, ROOM_AURA);
                    send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
                }
            } else {
                send_to_char(ch,
                             "You punch in normal gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
                obj->gravity = std::nullopt;
            }
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n", true, ch, nullptr, nullptr, TO_ROOM);

        }

        return (true);
    } else {
        return (false);
    }
}

SPECIAL(bank) {
    int amount, num = 0;

    struct obj_data *obj = nullptr;
    obj = ch->getRoom()->findObjectVnum(3034);

    if (CMD_IS("balance")) {
        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "The ATM is broken!\r\n");
            return (true);
        }

        if (GET_BANK_GOLD(ch) > 0)
            send_to_char(ch, "Your current balance is %d zenni.\r\n", GET_BANK_GOLD(ch));
        else
            send_to_char(ch, "You currently have no money deposited.\r\n");
        return (true);
    } else if (CMD_IS("wire")) {
        char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
        struct char_data *vict = nullptr;

        two_arguments(argument, arg, arg2);

        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "The ATM is broken!\r\n");
            return (true);
        }

        if ((amount = atoi(arg)) <= 0) {
            send_to_char(ch, "How much do you want to transfer?\r\n");
            return (true);
        }
        if (GET_BANK_GOLD(ch) < amount + (amount / 100)) {
            send_to_char(ch, "You don't have that much zenni in the bank (plus 1%s charge)!\r\n", "%");
            return (true);
        }
        if (!*arg2) {
            send_to_char(ch, "You want to transfer it to who?!\r\n");
            return (true);
        }
        if (!(vict = get_player_vis(ch, arg2, nullptr, FIND_CHAR_WORLD))) {
            int is_file = false, player_i = 0;
            char name[MAX_INPUT_LENGTH];
            sprintf(name, "%s", rIntro(ch, arg2));

            vict = findPlayer(name);

            if(!vict) {
                send_to_char(ch, "That person doesn't exist.\r\n");
                return (true);
            }

            if (ch->desc->account == nullptr) {
                send_to_char(ch, "There is an error. Report to staff.");
                return (true);
            }
            auto id = vict->uid;
            auto p = players[id];
            auto &c = p->account->characters;
            auto found = std::find_if(c.begin(), c.end(), [&](auto i) {return i == id;});
            if(found != c.end()) {
                send_to_char(ch, "You can not transfer money to your own offline characters...");
                return (true);
            }
            vict->mod(CharMoney::Bank, amount);
            ch->mod(CharMoney::Bank, -(amount + (amount / 100)));
            mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), true, "EXCHANGE: %s gave %s zenni to user %s",
                   GET_NAME(ch), add_commas(amount).c_str(), GET_NAME(vict));
            vict->save();
        } else {
            vict->mod(CharMoney::Bank, amount);
            ch->mod(CharMoney::Bank, -(amount + (amount / 100)));
            send_to_char(vict, "@WYou have just had @Y%s@W zenni wired into your bank account.@n\r\n",
                         add_commas(amount).c_str());
        }
        send_to_char(ch, "You transfer %s zenni to them.\r\n", add_commas(amount).c_str());
        act("$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
        return (true);
    } else if (CMD_IS("deposit")) {

        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "The ATM is broken!\r\n");
            return (true);
        }

        if ((amount = atoi(argument)) <= 0) {
            send_to_char(ch, "How much do you want to deposit?\r\n");
            return (true);
        }
        if (GET_GOLD(ch) < amount) {
            send_to_char(ch, "You don't have that much zenni!\r\n");
            return (true);
        }
        ch->mod(CharMoney::Carried, -amount);
        ch->mod(CharMoney::Bank, amount);
        send_to_char(ch, "You deposit %d zenni.\r\n", amount);
        act("$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
        return (true);
    } else if (CMD_IS("withdraw")) {

        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "The ATM is broken!\r\n");
            return (true);
        }

        if ((amount = atoi(argument)) <= 0) {
            send_to_char(ch, "How much do you want to withdraw?\r\n");
            return (true);
        }
        if (GET_BANK_GOLD(ch) < amount) {
            send_to_char(ch, "You don't have that much zenni!\r\n");
            return (true);
        }
        if (GET_BANK_GOLD(ch) - (amount + (1 + amount / 100)) < 0) {
            if (amount >= 100) {
                amount = amount + (amount / 100);
            } else if (amount < 100) {
                amount = amount + 1;
            }
            send_to_char(ch, "You need at least %s in the bank with the 1 percent withdraw fee.\r\n",
                         add_commas(amount).c_str());
            return (true);
        }
        if (GET_GOLD(ch) + amount > GOLD_CARRY(ch)) {
            send_to_char(ch, "You can only carry %s zenni, you left the rest.\r\n", add_commas(GOLD_CARRY(ch)).c_str());
            int diff = (GET_GOLD(ch) + amount) - GOLD_CARRY(ch);
            ch->set(CharMoney::Carried, GOLD_CARRY(ch));
            amount -= diff;
            if (amount >= 100) {
                num = amount / 100;
                ch->mod(CharMoney::Bank, -(amount + num));
            } else if (amount < 100) {
                ch->mod(CharMoney::Bank, -(amount + 1));
            }
            send_to_char(ch, "You withdraw %s zenni,  and pay %s in withdraw fees.\r\n.\r\n", add_commas(amount).c_str(),
                         add_commas(num).c_str());
            act("$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
            return (true);
        }
        ch->mod(CharMoney::Carried, amount);
        if (amount >= 100) {
            num = amount / 100;
            ch->mod(CharMoney::Bank, -(amount + num));
        } else if (amount < 100) {
            ch->mod(CharMoney::Bank, -(amount + 1));
        }
        send_to_char(ch, "You withdraw %s zenni, and pay %s in withdraw fees.\r\n", add_commas(amount).c_str(),
                     add_commas(num).c_str());
        act("$n makes a bank transaction.", true, ch, nullptr, nullptr, TO_ROOM);
        return (true);
    } else
        return (false);
}

