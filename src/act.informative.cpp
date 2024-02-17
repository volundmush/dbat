/* ************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/vehicles.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/maputils.h"
#include "dbat/config.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/races.h"
#include "dbat/handler.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"
#include "dbat/class.h"
#include "dbat/boards.h"
#include "dbat/screen.h"
#include "dbat/mail.h"
#include "dbat/guild.h"
#include "dbat/players.h"
#include "dbat/account.h"
#include "dbat/improved-edit.h"
#include "dbat/transformation.h"

/* local functions */
static void gen_map(struct char_data *ch, int num);

static void bringdesc(struct char_data *ch, struct char_data *tch);

static void see_plant(struct obj_data *obj, struct char_data *ch);

static double terrain_bonus(struct char_data *ch);

static void search_room(struct char_data *ch);

static void bonus_status(struct char_data *ch);

static int sort_commands_helper(const void *a, const void *b);

static void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur);

static void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode);

static void list_obj_to_char(std::vector<obj_data*> list, struct char_data *ch, int mode, int show);

static void trans_check(struct char_data *ch, struct char_data *vict);

static int show_obj_modifiers(struct obj_data *obj, struct char_data *ch);

static void perform_mortal_where(struct char_data *ch, char *arg);

static void perform_immort_where(struct char_data *ch, char *arg);

static void diag_char_to_char(struct char_data *i, struct char_data *ch);

static void diag_obj_to_char(struct obj_data *obj, struct char_data *ch);

static void look_at_char(struct char_data *i, struct char_data *ch);

static void list_one_char(struct char_data *i, struct char_data *ch);

static void list_char_to_char(std::vector<char_data*> people, struct char_data *ch);

static void look_in_direction(struct char_data *ch, int dir);

static void look_in_obj(struct char_data *ch, char *arg);

static void look_out_window(struct char_data *ch, char *arg);

static void look_at_target(struct char_data *ch, char *arg, int read);

static void search_in_direction(struct char_data *ch, int dir);

static void do_auto_exits(struct room_data *room, struct char_data *ch, int exit_mode);

static void do_auto_exits2(room_rnum target_room, struct char_data *ch);

static void display_spells(struct char_data *ch, struct obj_data *obj);

static void display_scroll(struct char_data *ch, struct obj_data *obj);

static void space_to_minus(char *str);

static int yesrace(int num);

static void map_draw_room(char map[9][10], int x, int y, room_rnum rnum,
                          struct char_data *ch);
// definitions
ACMD(do_evolve) {

    if (!IS_ARLIAN(ch) || IS_NPC(ch)) {
        ch->sendf("You are not an arlian!\r\n");
        return;
    }

    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    int64_t plcost = GET_LEVEL(ch), stcost = GET_LEVEL(ch), kicost = GET_LEVEL(ch);

    plcost += molt_threshold(ch) * 0.65 + (ch->getBasePL() * 0.15);
    kicost += (molt_threshold(ch) * 0.50) + (ch->getBaseKI() * 0.22);
    stcost += (molt_threshold(ch) * 0.50) + (ch->getBaseST() * 0.15);

    if (!*arg) {
        ch->sendf("@D-=@YConvert Evolution Points To What?@D=-@n\r\n");
        ch->sendf("@D-------------------------------------@n\r\n");
        ch->sendf("@CPowerlevel  @D: @Y%s @Wpts\r\n", add_commas(plcost).c_str());
        ch->sendf("@CKi          @D: @Y%s @Wpts\r\n", add_commas(kicost).c_str());
        ch->sendf("@CStamina     @D: @Y%s @Wpts\r\n", add_commas(stcost).c_str());
        ch->sendf("@D[@Y%s @Wpts currently@D]@n\r\n", add_commas(GET_MOLT_EXP(ch)).c_str());
        return;
    } else if (!strcasecmp(arg, "powerlevel") || !strcasecmp(arg, "pl")) {
        if (plcost > molt_threshold(ch)) {
            ch->sendf("You need a few more evolution levels before you can start upgrading powerlevel.\r\n");
            return;
        } else if (GET_MOLT_EXP(ch) < plcost) {
            ch->sendf("You do not have enough evolution experience.\r\n");
            return;
        } else {
            int64_t plgain = (ch->getBasePL()) * 0.01;

            if (plgain <= 0) {
                plgain = rand_number(1, 5);
            } else {
                plgain = rand_number(plgain, plgain * .5);
            }
            ch->gainBasePL(plgain);
            GET_MOLT_EXP(ch) -= plcost;
            ch->sendf("Your body evolves to make better use of the way it is now, and you feel that your body has strengthened. @D[@RPL@D: @Y+%s@D]@n\r\n",
                         add_commas(plgain).c_str());
        }
    } else if (!strcasecmp(arg, "ki")) {
        if (kicost > molt_threshold(ch)) {
            ch->sendf("You need a few more evolution levels before you can start upgrading ki.\r\n");
            return;
        } else if (GET_MOLT_EXP(ch) < kicost) {
            ch->sendf("You do not have enough evolution experience.\r\n");
            return;
        } else {
            int64_t kigain = (ch->getBaseKI()) * 0.01;

            if (kigain <= 0) {
                kigain = rand_number(1, 5);
            } else {
                kigain = rand_number(kigain, kigain * .5);
            }
            ch->gainBaseKI(kigain);
            GET_MOLT_EXP(ch) -= kicost;
            ch->sendf(
                         "Your body evolves to make better use of the way it is now, and you feel that your spirit has strengthened. @D[@CKi@D: @Y+%s@D]@n\r\n",
                         add_commas(kigain).c_str());
        }
    } else if (!strcasecmp(arg, "stamina") || !strcasecmp(arg, "st")) {
        if (stcost > molt_threshold(ch)) {
            ch->sendf("You need a few more evolution levels before you can start upgrading stamina.\r\n");
            return;
        } else if (GET_MOLT_EXP(ch) < stcost) {
            ch->sendf("You do not have enough evolution experience.\r\n");
            return;
        } else {
            int64_t stgain = (ch->getBaseST()) * 0.01;

            if (stgain <= 0) {
                stgain = rand_number(1, 5);
            } else {
                stgain = rand_number(stgain, stgain * .5);
            }
            ch->gainBaseST(stgain);
            GET_MOLT_EXP(ch) -= stcost;
            ch->sendf(
                         "Your body evolves to make better use of the way it is now, and you feel that your body has more stamina. @D[@GST@D: @Y+%s@D]@n\r\n",
                         add_commas(stgain).c_str());
        }
    }
}

static void see_plant(struct obj_data *obj, struct char_data *ch) {

    int water = GET_OBJ_VAL(obj, VAL_WATERLEVEL);
    auto sd = obj->getShortDesc();
    if (water >= 0) {
        
        switch (GET_OBJ_VAL(obj, VAL_MATURITY)) {
            case 0:
                ch->sendf("@wA @G%s@y seed@w has been planted here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 1:
                ch->sendf("@wA very young @G%s@w has sprouted from a planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 2:
                ch->sendf("@wA half grown @G%s@w is in a planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 3:
                ch->sendf("@wA mature @G%s@w is growing in a planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 4:
                ch->sendf("@wA mature @G%s@w is flowering in a planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 5:
                ch->sendf("@wA mature @G%s@w that is close to harvestable is here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 6:
                ch->sendf("@wA @Rharvestable @G%s@w is in the planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            default:
                break;
        }
    } else {
        if (water > -4) {
            ch->sendf("@yA @G%s@y that is looking a bit @rdry@y, is here.@n\r\n", sd);
        } else if (water > -10) {
            ch->sendf("@yA @G%s@y that is looking extremely @rdry@y, is here.@n\r\n", sd);
        } else if (water <= -10) {
            ch->sendf("@yA @G%s@y that is completely @rdead@y and @rwithered@y, is here.@n\r\n",
                         sd);
        }
    }

}

/* This is used to determine the terrain bonus for search_room - Iovan 12/16/2012*/
static double terrain_bonus(struct char_data *ch) {

    double bonus = 0.0;

    switch (SECT(IN_ROOM(ch))) {
        case SECT_FOREST:
            bonus += 0.5;
            break;
        case SECT_SPACE:
            bonus += -0.5;
            break;
        case SECT_WATER_NOSWIM:
            bonus += 0.25;
            break;
        case SECT_MOUNTAIN:
            bonus += 0.1;
            break;
        default:
            bonus += 0.0;
            break;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE)) {
        bonus += -0.5;
    }

    return (bonus);

}


/* This is used to find hidden people in a room with search - Iovan 12/16/2012 */
static void search_room(struct char_data *ch) {

    struct char_data *vict, *next_v;
    int perc =
            (GET_INT(ch) * 0.6) + GET_SKILL(ch, SKILL_SPOT) + GET_SKILL(ch, SKILL_SEARCH) + GET_SKILL(ch, SKILL_LISTEN);
    int prob = 0, found = 0;
    double bonus = 1.0, terrain = 1.0;

    if ((ch->getCurST()) < GET_MAX_MOVE(ch) * 0.001) {
        ch->sendf("You do not have enough stamina.\r\n");
        return;
    }

    if (GET_SKILL(ch, SKILL_SENSE)) {
        bonus += (GET_SKILL(ch, SKILL_SENSE) * 0.01);
    }

    reveal_hiding(ch, 0);
    act("@y$n@Y begins searching the room carefully.@n", true, ch, nullptr, nullptr, TO_ROOM);
    WAIT_STATE(ch, PULSE_1SEC);

    for (auto vict : ch->getRoom()->getPeople()) {
        if (AFF_FLAGGED(vict, AFF_HIDE) && vict != ch) {
            if (GET_SUPPRESS(vict) >= 1) {
                perc *= (GET_SUPPRESS(vict) * 0.01);
            }
            prob = GET_DEX(vict) + (GET_INT(vict) * 0.6) + GET_SKILL(vict, SKILL_HIDE) +
                   GET_SKILL(vict, SKILL_MOVE_SILENTLY);

            if (AFF_FLAGGED(vict, AFF_LIQUEFIED)) {
                prob *= 1.5;
            }
            if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 4 || GET_GENOME(ch, 1) == 4)) {
                perc += 5;
            }
            if (IS_MUTANT(vict) && (GET_GENOME(vict, 0) == 5 || GET_GENOME(vict, 1) == 5)) {
                prob += 10;
            }
            terrain += terrain_bonus(vict);
            if (perc * bonus >= prob * terrain) { /* Found them. */
                act("@YYou find @y$N@Y hiding nearby!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@y$n@Y has found your hiding spot!@n", true, ch, nullptr, vict, TO_VICT);
                act("@y$n@Y has found @y$N's@Y hiding spot!@n", true, ch, nullptr, vict, TO_NOTVICT);
                reveal_hiding(vict, 4);
                found++;
            }
        }
    }

    for (auto obj : ch->getRoom()->getInventory()) {
        if (OBJ_FLAGGED(obj, ITEM_BURIED) && perc * bonus > rand_number(50, 200)) {
            act("@YYou uncover @y$p@Y, which had been burried here.@n", true, ch, obj, nullptr, TO_CHAR);
            act("@y$n@Y uncovers @y$p@Y, which had burried here.@n", true, ch, obj, nullptr, TO_ROOM);
            obj->clearFlag(FlagType::Item, ITEM_BURIED);
            found++;
        }
    }
    ch->decCurSTPercent(.001);


    if (found == 0) {
        ch->sendf("You find nothing hidden.\r\n");
        return;
    }

}

static const char *weapon_disp[6] = {
        "Sword",
        "Dagger",
        "Spear",
        "Club",
        "Gun",
        "Brawling"
};

ACMD(do_mimic) {

    if (IS_NPC(ch))
        return;

    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!GET_SKILL(ch, SKILL_MIMIC)) {
        ch->sendf("You do not know how to mimic the appearance of other races.\r\n");
        return;
    }

    int count = 0, x = 0;

    // generate a list of mimic'able races.
    auto check = [&](RaceID id) {return race::getValidSexes(id).contains(GET_SEX(ch)) && race::isValidMimic(id);};
    auto races = race::filterRaces(check);
    if (!*arg) {
        ch->sendf("@CMimic Menu\n@c--------------------@W\r\n");
        for (const auto &r: races) {
            if (count == 2) {
                ch->sendf("%s\n", race::getName(r).c_str());
                count = 0;
            } else {
                ch->sendf("%s\n", race::getName(r).c_str());
                count++;
            }
        }
        ch->sendf("Stop@n\r\n");
        if (ch->mimic) {
            ch->sendf("You currently Mimic a %s", race::getName(ch->mimic.value()).c_str());
        }

        return;
    }

    if (!strcasecmp(arg, "stop")) {
        if (!ch->mimic) {
            ch->sendf("You are not imitating another race.\r\n");
            return;
        }
        act("@mYou concentrate for a moment and release the illusion that was mimicing another race.@n", true, ch,
            nullptr, nullptr, TO_CHAR);
        act("@M$n@m concentrates for a moment and SUDDENLY $s appearance changes some what!@n", true, ch, nullptr,
            nullptr, TO_ROOM);
        ch->mimic.reset();
    }

    auto chosen_race = race::findRace(arg, check);
    if (!chosen_race) {
        ch->sendf(
                     "That is not a race you can change into. Enter mimic without arugments for the mimic menu.\r\n");
        return;
    }
    auto race = chosen_race.value();

    int prob = GET_SKILL(ch, SKILL_MIMIC), perc = axion_dice(0);
    double mult = 1 / prob;
    int64_t cost = GET_MAX_MANA(ch) * mult;

    if (race == ch->mimic) {
        ch->sendf("You are already mimicing that race. To stop enter 'mimic stop'\r\n");
        return;
    } else if (ch->getCurKI() < cost) {
        ch->sendf("You do not have enough ki to perform the technique.\r\n");
        return;
    } else if (prob < perc) {
        ch->decCurKI(cost);
        act("@mYou concentrate and attempt to create an illusion to obscure your racial features. However you frown as you realize you have failed.@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@M$n@m concentrates and the light around them seems to shift and blur. It stops a moment later and $e frowns.@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        return;
    } else {
        char buf[MAX_STRING_LENGTH];
        ch->mimic = race;
        ch->decCurKI(cost);
        sprintf(buf,
                "@M$n@m concentrates for a moment and $s features start to blur as light bends around $m. Now $e appears to be %s @M%s!@n",
                AN(RACE(ch)), LRACE(ch));
        ch->sendf(
                     "@mYou concentrate for a moment and your features start to blur as you use your ki to bend the light around your body. You now appear to be %s %s.@n\r\n",
                     AN(RACE(ch)), LRACE(ch));
        act(buf, true, ch, nullptr, nullptr, TO_ROOM);
        return;
    }
}

ACMD(do_kyodaika) {

    if (!IS_NAMEK(ch)) {
        ch->sendf("You are not a namek!\r\n");
        return;
    }

    if (!AFF_FLAGGED(ch, AFF_KYODAIKA)) {
        act("@GYou growl as your body grows to ten times its normal size!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n@G growls as $s body grows to ten times its normal size!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->sendf("@cStrength@D: @C+5\r\n@cSpeed@D: @c-2@n\r\n");
        assign_affect(ch, AFF_KYODAIKA, 0, -1, 5, 0, 0, 0, 0, -2);
    } else {
        act("@GYou growl as your body shrinks to its normal size!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n@G growls as $s body shrinks to its normal size!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->sendf("@cStrength@D: @C-5\r\n@cSpeed@D: @c+2@n\r\n");
        null_affect(ch, AFF_KYODAIKA);
    }

}

ACMD(do_table) {
    struct obj_data *obj = nullptr, *obj2 = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
        ch->sendf("Syntax: table (red | blue | green | yellow) (card name)");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getRoom()->getInventory()))) {
        ch->sendf("You don't see that table here.\r\n");
        return;
    }

    if (!(obj2 = get_obj_in_list_vis(ch, arg2, nullptr, obj->getInventory()))) {
        ch->sendf("That card doesn't seem to be on that table.\r\n");
        return;
    }

    char buf[200];
    sprintf(buf, "$n looks at %s on %s.\r\n", obj2->getShortDesc(), obj->getShortDesc());
    act(buf, true, ch, nullptr, nullptr, TO_ROOM);
    ch->sendf("%s", obj2->getLookDesc());
}

ACMD(do_draw) {
    if (!SITS(ch)) {
        ch->sendf("You are not sitting at a duel table.\r\n");
        return;
    }

    if (GET_OBJ_VNUM(SITS(ch)) < 604 || GET_OBJ_VNUM(SITS(ch)) > 607) {
        ch->sendf("You need to be sitting at an official table to play.\r\n");
        return;
    }

    struct obj_data *obj = nullptr, *obj2 = nullptr, *obj3 = nullptr, *next_obj = nullptr;
    int drawn = false;

    if (!(obj = get_obj_in_list_vis(ch, "case", nullptr, ch->getInventory()))) {
        ch->sendf("You don't have a case.\r\n");
        return;
    }
    for (auto obj2 : obj->getInventory()) {
        if (drawn == false) {
            obj2->removeFromLocation();
            obj2->addToLocation(ch);
            obj3 = obj2;
            drawn = true;
        }
    }
    if (drawn == false) {
        ch->sendf("You don't have any cards in the case!\r\n");
        return;
    } else {
        act("$n draws a card from $s $p.\r\n", true, ch, obj, nullptr, TO_ROOM);
        ch->sendf("You draw a card.\r\n%s\r\n", obj3->getLookDesc());
        return;
    }

}

ACMD(do_shuffle) {

    if (!SITS(ch)) {
        ch->sendf("You are not sitting at a duel table.\r\n");
        return;
    }

    if (GET_OBJ_VNUM(SITS(ch)) < 604 || GET_OBJ_VNUM(SITS(ch)) > 607) {
        ch->sendf("You need to be sitting at an official table to play.\r\n");
        return;
    }

    struct obj_data *obj = nullptr, *obj2 = nullptr, *next_obj = nullptr;
    int count = 0;

    if (!(obj = get_obj_in_list_vis(ch, "case", nullptr, ch->getInventory()))) {
        ch->sendf("You don't have a case.\r\n");
        return;
    }

    for (auto obj2 : obj->getInventory()) {
        if (!OBJ_FLAGGED(obj2, ITEM_ANTI_HIEROPHANT)) {
            continue;
        }
        count += 1;
    }
    if (count <= 0) {
        ch->sendf("You don't have any cards in the case!\r\n");
        return;
    }
    int total = count;
    for (auto obj2 : obj->getInventory()) {
        obj2->removeFromLocation();
        obj2->addToLocation(world.at(48));
    }
    while (count > 0) {
        for (auto obj2 : world[48]->getInventory()) {
            if (!OBJ_FLAGGED(obj2, ITEM_ANTI_HIEROPHANT)) {
                continue;
            }
            if (obj2 && count > 1 && rand_number(1, 4) == 3) {
                count -= 1;
                obj2->removeFromLocation();
                obj2->addToLocation(obj);
            } else if (obj2 && count == 1) {
                count -= 1;
                obj2->removeFromLocation();
                obj2->addToLocation(obj);
            }
        }
    }
    ch->sendf("You shuffle the cards carefully.\r\n");
    act("$n shuffles their deck.", true, ch, nullptr, nullptr, TO_ROOM);
    send_to_room(IN_ROOM(ch), "There were %d cards in the deck.\r\n", total);
}

ACMD(do_hand) {

    struct obj_data *obj, *next_obj;
    char arg[MAX_INPUT_LENGTH];
    int count = 0;

    one_argument(argument, arg);

    if (!*arg) {
        ch->sendf("Syntax: hand (look | show)\r\n");
        return;
    }

    if (!strcasecmp("look", arg)) {
        ch->sendf("@CYour hand contains:\r\n@D---------------------------@n\r\n");
        for (auto obj : ch->getInventory()) {
            if (obj && !OBJ_FLAGGED(obj, ITEM_ANTI_HIEROPHANT)) {
                continue;
            }
            if (obj) {
                count += 1;
                ch->sendf("%s\r\n", obj->getShortDesc());
            }
        }
        act("$n looks at $s hand.", true, ch, nullptr, nullptr, TO_ROOM);
        if (count == 0) {
            ch->sendf("No cards.");
            act("There were no cards.", true, ch, nullptr, nullptr, TO_ROOM);
        } else if (count > 7) {
            act("You have more than seven cards in your hand.", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n has more than seven cards in $s hand.", true, ch, nullptr, nullptr, TO_ROOM);
        } else {
            char buf[200];
            sprintf(buf, "There are %d cards in the hand.", count);
            act(buf, true, ch, nullptr, nullptr, TO_ROOM);
        }
    } else if (!strcasecmp("show", arg)) {
        ch->sendf("You show off your hand to the room.\r\n");
        act("@C$n's hand contains:\r\n@D---------------------------@n", true, ch, nullptr, nullptr, TO_ROOM);
        for (auto obj : ch->getInventory()) {
            if (obj && !OBJ_FLAGGED(obj, ITEM_ANTI_HIEROPHANT)) {
                continue;
            }
            if (obj) {
                count += 1;
                act("$p", true, ch, obj, nullptr, TO_ROOM);
            }
        }
        if (count == 0) {
            act("No cards.", true, ch, nullptr, nullptr, TO_ROOM);
        }
        if (count > 7) {
            act("You have more than seven cards in your hand.", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n has more than seven cards in $s hand.", true, ch, nullptr, nullptr, TO_ROOM);
        }
    } else {
        ch->sendf("Syntax: hand (look | show)\r\n");
        return;
    }
}

ACMD(do_post) {

    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct obj_data *obj;
    struct obj_data *obj2;

    two_arguments(argument, arg, arg2);

    if (!*arg) {
        ch->sendf("Syntax: post (obj name)\n"
                         "        post (obj name) (target obj name)\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory()))) {
        ch->sendf("You don't seem to have that.\r\n");
        return;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_NOTE) {
        ch->sendf("You can only post notepaper.\r\n");
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GARDEN1) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_GARDEN2)) {
        ch->sendf("You can not post on things in a garden.\r\n");
        return;
    }

    if (!*arg2) {
        if (SECT(IN_ROOM(ch)) != SECT_INSIDE && SECT(IN_ROOM(ch)) != SECT_CITY) {
            ch->sendf("You are not near any general structure you can post it on.\r\n");
            return;
        }
        act("@WYou post $p@W on a nearby structure.@n", true, ch, obj, nullptr, TO_CHAR);
        act("@C$n@W posts $p@W on a nearby structure.@n", true, ch, obj, nullptr, TO_ROOM);
        obj->removeFromLocation();
        obj->addToLocation(ch->getRoom());
        GET_OBJ_POSTTYPE(obj) = 1;
        return;
    } else {
        if (!(obj2 = get_obj_in_list_vis(ch, arg2, nullptr, ch->getRoom()->getInventory()))) {
            ch->sendf("You can't seem to find the thing you want to post it on.\r\n");
            return;
        } else if (GET_OBJ_POSTED(obj2)) {
            ch->sendf("It already has something posted on it. Get that first if you want to post.\r\n");
            return;
        } else if (GET_OBJ_TYPE(obj2) == ITEM_BOARD) {
            ch->sendf("Boards come with their own means of posting messages.\r\n");
            return;
        } else {
            char buf[MAX_STRING_LENGTH];
            sprintf(buf, "@C$n@W posts %s@W on %s@W.@n", obj->getShortDesc(), obj2->getShortDesc());
            ch->sendf("@WYou post %s@W on %s@W.@n\r\n", obj->getShortDesc(), obj2->getShortDesc());
            act(buf, true, ch, nullptr, nullptr, TO_ROOM);
            obj->removeFromLocation();
            obj->addToLocation(ch->getRoom());
            GET_OBJ_POSTTYPE(obj) = 2;
            GET_OBJ_POSTED(obj) = obj2;
            GET_OBJ_POSTED(obj2) = obj;
            return;
        }
    }

}

ACMD(do_play) {

    if (GET_POS(ch) != POS_SITTING) {
        ch->sendf("You need to be sitting at an official table to play.\r\n");
        return;
    }

    if (!SITS(ch)) {
        ch->sendf("You need to be sitting at an official table to play.\r\n");
        return;
    }

    if (GET_OBJ_VNUM(SITS(ch)) < 604 || GET_OBJ_VNUM(SITS(ch)) > 607) {
        ch->sendf("You need to be sitting at an official table to play.\r\n");
        return;
    }

    struct obj_data *obj = nullptr, *obj2 = nullptr, *obj3 = nullptr, *next_obj = nullptr;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (!*arg) {
        ch->sendf("Syntax: play (card name)");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory()))) {
        ch->sendf("You don't have that card to play.\r\n");
        return;
    }

    for (auto obj3 : ch->getRoom()->getInventory()) {
        if (GET_OBJ_VNUM(obj3) == GET_OBJ_VNUM(SITS(ch)) - 4) {
            obj2 = obj3;
            break;
        }
    }

    if (obj2 == nullptr) {
        ch->sendf("Your table is missing. Inform an immortal of this problem.\r\n");
        return;
    }

    act("You play $p on your table.", true, ch, obj, nullptr, TO_CHAR);
    act("$n plays $p on $s table.", true, ch, obj, nullptr, TO_ROOM);
    obj->removeFromLocation();
    obj->addToLocation(obj2);
}

/* Nickname an object */
ACMD(do_nickname) {
    struct obj_data *obj = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
        ch->sendf("Syntax: nickname (object) (nickname)\n");
        ch->sendf("Syntax: nickname ship (nickname)\n");
        return;
    }

    if (strcasecmp(arg, "ship")) {
        if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory()))) {
            ch->sendf("You don't have that item to nickname.\r\n");
            return;
        }
    }
    if (strlen(arg2) > 20) {
        ch->sendf("You can't nickname items with any name longer than 20 characters.\r\n");
        return;
    }

    if (!strcasecmp(arg, "ship")) {
        struct obj_data *ship = nullptr, *next_obj = nullptr, *ship2 = nullptr;
        int found = false;
        for (auto ship : ch->getRoom()->getInventory()) {
            if (GET_OBJ_VNUM(ship) >= 45000 && GET_OBJ_VNUM(ship) <= 45999 && found == false) {
                found = true;
                ship2 = ship;
                break;
            }
        }
        if (found == true) {
            if (strstr(arg2, "@")) {
                ch->sendf("You can't nickname a ship and use color codes. Sorry.\r\n");
                return;
            } else {
                char nick[MAX_INPUT_LENGTH];
                sprintf(nick, "%s", CAP(arg2));
                ship2->setLookDesc(nick);
                struct obj_data *k;
                for (k = object_list; k; k = k->next) {
                    if (GET_OBJ_VNUM(k) == GET_OBJ_VNUM(ship2) + 1000) {
                        k->extractFromWorld();
                        int was_in = GET_ROOM_VNUM(IN_ROOM(ship2));
                        ship2->removeFromLocation();
                        ship2->addToLocation(world.at(was_in));
                    }
                }
            }
        }
        return;
    }

    if (strstr(obj->getShortDesc().c_str(), "nicknamed")) {
        ch->sendf("%s@w has already been nicknamed.@n\r\n", obj->getShortDesc());
        return;
    } else if (strstr(obj->getName().c_str(), "corpse")) {
        ch->sendf("%s@w is a corpse!@n\r\n", obj->getShortDesc());
        return;
    } else {
        ch->sendf("@wYou nickname %s@w as '@C%s@w'.@n\r\n", obj->getShortDesc(), arg2);
        char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH];
        sprintf(nick, "%s @wnicknamed @D(@C%s@D)@n", obj->getShortDesc(), CAP(arg2));
        sprintf(nick2, "%s %s", obj->getName().c_str(), arg2);
        obj->setShortDesc(nick);
        obj->setName(nick2);
        return;
    }
}


/* local globals */
int *cmd_sort_info;

/* Portal appearance types */
static const char *portal_appearance[] = {
        "All you can see is the glow of the portal.",
        "You see an image of yourself in the room - my, you are looking attractive today.",
        "All you can see is a swirling grey mist.",
        "The scene is of the surrounding countryside, but somehow blurry and lacking focus.",
        "The blackness appears to stretch on forever.",
        "Suddenly, out of the blackness a flaming red eye appears and fixes its gaze upon you.",
        "\n"
};


ACMD(do_showoff) {
    struct obj_data *obj = nullptr;
    struct char_data *vict = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    *arg = '\0';
    *arg2 = '\0';

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
        ch->sendf("You want to show what item to what character?\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory()))) {
        ch->sendf("You don't seem to have that.\r\n");
        return;
    } else if (!(vict = get_player_vis(ch, arg2, nullptr, FIND_CHAR_ROOM))) {
        ch->sendf("There is no such person around.\r\n");
        return;
    } else { /* Ok show that target the object! */
        act("@WYou hold up $p@W for @C$N@W to see:@n", true, ch, obj, vict, TO_CHAR);
        act("@C$n@W holds up $p@W for you to see:@n", true, ch, obj, vict, TO_VICT);
        act("@C$n@W holds up $p@W for @c$N@W to see.@n", true, ch, obj, vict, TO_NOTVICT);
        show_obj_to_char(obj, vict, SHOW_OBJ_ACTION);
        return;
    }
}

int readIntro(struct char_data *ch, struct char_data *vict) {
    if (vict == nullptr) {
        return 0;
    }

    if (IS_NPC(ch) || IS_NPC(vict)) {
        return 1;
    }

    auto p = players[ch->uid];

    return p->dubNames.contains(vict->uid);
}

void introWrite(struct char_data *ch, struct char_data *vict, char *name) {
    std::string n(name);
    auto p = players[ch->uid];
    p->dubNames[vict->uid] = n;
}

ACMD(do_intro) {

    if (IS_NPC(ch))
        return;

    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    struct char_data *vict;

    two_arguments(argument, arg, arg2);

    if (!*arg) {
        ch->sendf(
                     "Syntax: dub (target) (name)\r\nWho do you want to dub and what do you want to name them?\r\n");
        return;
    }

    if (!*arg2) {
        ch->sendf("Syntax: dub (target) (name)\r\nWhat name do you wish to know them by?\r\n");
        return;
    }

    if (strlen(arg2) > 20) {
        ch->sendf("Limit the name to 20 characters.\r\n");
        return;
    }
    if (strlen(arg2) < 3) {
        ch->sendf("Limit the name to at least 3 characters.\r\n");
        return;
    }

    if (strstr(arg2, "$") || strstr(arg2, "@") || strstr(arg2, "%")) {
        ch->sendf("Illegal character. No symbols.\r\n");
        return;
    }

    if (!(vict = get_player_vis(ch, arg, nullptr, FIND_CHAR_ROOM))) {
        ch->sendf("There is no such person.\r\n");
        return;
    }

    if (vict == ch) {
        ch->sendf("That seems rather odd.\r\n");
        return;
    }

    if (IS_NPC(vict)) {
        ch->sendf("That seems rather unwise.\r\n");
        return;
    }

    if (readIntro(vict, ch) == 2) {
        ch->sendf("There seems to have been an error, report this to Iovan.\r\n");
        return;
    } else if (readIntro(ch, vict) == 1 && strstr(RACE(vict), arg)) {
        ch->sendf(
                     "You have already dubbed them a name. If you want to redub them target the name you know them by.\r\n");
        return;
    } else {
        introWrite(ch, vict, arg2);
        act("You decide to call $M, $N.", true, ch, nullptr, vict, TO_CHAR);
        act("$n seems to decide something about you.", true, ch, nullptr, vict, TO_VICT);
        act("$n seems to decide something about $N.", true, ch, nullptr, vict, TO_NOTVICT);
        return;
    }
}

/* Used when checking status or looking at a character */
static void bringdesc(struct char_data *ch, struct char_data *tch) {

    if (ch != nullptr && tch != nullptr && IS_HUMANOID(tch)) {

        if (ch != tch && PLR_FLAGGED(tch, PLR_DISGUISED)) {
            ch->sendf("            @D[@cHair Length @D: @WHidden.         @D]@n\r\n");
            ch->sendf("            @D[@cHair Color  @D: @WHidden.         @D]@n\r\n");
            ch->sendf("            @D[@cHair Style  @D: @WHidden.         @D]@n\r\n");
            ch->sendf("            @D[@cEye Color   @D: @WHidden.         @D]@n\r\n");
            if (GET_SKIN(tch) == SKIN_WHITE) {
                ch->sendf("            @D[@cSkin Color  @D: @WWhite.        @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_TAN) {
                ch->sendf("            @D[@cSkin Color  @D: @WTan.          @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_BLACK) {
                ch->sendf("            @D[@cSkin Color  @D: @WBlack.        @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_GREEN) {
                ch->sendf("            @D[@cSkin Color  @D: @WGreen.        @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_ORANGE) {
                ch->sendf("            @D[@cSkin Color  @D: @WOrange.       @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_YELLOW) {
                ch->sendf("            @D[@cSkin Color  @D: @WYellow.       @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_RED) {
                ch->sendf("            @D[@cSkin Color  @D: @WRed.          @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_GREY) {
                ch->sendf("            @D[@cSkin Color  @D: @WGrey.         @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_BLUE) {
                ch->sendf("            @D[@cSkin Color  @D: @WBlue.         @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_AQUA) {
                ch->sendf("            @D[@cSkin Color  @D: @WAqua.         @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_PINK) {
                ch->sendf("            @D[@cSkin Color  @D: @WPink.         @D]@n\r\n");
            } else if (GET_SKIN(tch) == SKIN_PURPLE) {
                ch->sendf("            @D[@cSkin Color  @D: @WPurple.       @D]@n\r\n");
            }
            return;
        }

        if (IS_HUMAN(tch) || IS_SAIYAN(tch) || IS_KONATSU(tch) || IS_MUTANT(tch) || IS_ANDROID(tch) || IS_KAI(tch) ||
            IS_HALFBREED(tch) || IS_TRUFFLE(tch) || IS_HOSHIJIN(tch)) {
            if ((!IS_SAIYAN(tch) && !IS_HALFBREED(tch)) ||
                ((IS_SAIYAN(tch) || IS_HALFBREED(tch)) && !IS_TRANSFORMED(tch))) {
                if (GET_HAIRL(tch) == HAIRL_LONG) {
                    ch->sendf("            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
                } else if (GET_HAIRL(tch) == HAIRL_BALD) {
                    ch->sendf("            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
                } else if (GET_HAIRL(tch) == HAIRL_SHORT) {
                    ch->sendf("            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
                } else if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
                    ch->sendf("            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
                } else if (GET_HAIRL(tch) == HAIRL_RLONG) {
                    ch->sendf("            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
                }
                if (GET_HAIRS(tch) == HAIRS_PLAIN) {
                    ch->sendf("            @D[@cHair Style  @D: @WPlain.        @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_MOHAWK) {
                    ch->sendf("            @D[@cHair Style  @D: @WMohawk.       @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_SPIKY) {
                    ch->sendf("            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_CURLY) {
                    ch->sendf("            @D[@cHair Style  @D: @WCurly.        @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_UNEVEN) {
                    ch->sendf("            @D[@cHair Style  @D: @WUneven.       @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_PONYTAIL) {
                    ch->sendf("            @D[@cHair Style  @D: @WPony Tail.    @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_AFRO) {
                    ch->sendf("            @D[@cHair Style  @D: @WAfro.         @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_FADE) {
                    ch->sendf("            @D[@cHair Style  @D: @WFade.         @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_CREW) {
                    ch->sendf("            @D[@cHair Style  @D: @WCrew Cut.     @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_FEATHERED) {
                    ch->sendf("            @D[@cHair Style  @D: @WFeathered.    @D]@n\r\n");
                } else if (GET_HAIRS(tch) == HAIRS_DRED) {
                    ch->sendf("            @D[@cHair Style  @D: @WDread Locks.  @D]@n\r\n");
                }
                if (GET_HAIRC(tch) == HAIRC_BLACK) {
                    ch->sendf("            @D[@cHair Color  @D: @WBlack.        @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_BROWN) {
                    ch->sendf("            @D[@cHair Color  @D: @WBrown.        @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_BLONDE) {
                    ch->sendf("            @D[@cHair Color  @D: @WBlonde.       @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_GREY) {
                    ch->sendf("            @D[@cHair Color  @D: @WGrey.         @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_RED) {
                    ch->sendf("            @D[@cHair Color  @D: @WRed.          @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_ORANGE) {
                    ch->sendf("            @D[@cHair Color  @D: @WOrange.       @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_GREEN) {
                    ch->sendf("            @D[@cHair Color  @D: @WGreen.        @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_BLUE) {
                    ch->sendf("            @D[@cHair Color  @D: @WBlue.         @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_PINK) {
                    ch->sendf("            @D[@cHair Color  @D: @WPink.         @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_PURPLE) {
                    ch->sendf("            @D[@cHair Color  @D: @WPurple.       @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_SILVER) {
                    ch->sendf("            @D[@cHair Color  @D: @WSilver.       @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_CRIMSON) {
                    ch->sendf("            @D[@cHair Color  @D: @WCrimson.      @D]@n\r\n");
                } else if (GET_HAIRC(tch) == HAIRC_WHITE) {
                    ch->sendf("            @D[@cHair Color  @D: @WWhite.        @D]@n\r\n");
                }
            } else if (IS_SAIYAN(tch) || IS_HALFBREED(tch)) {
                if (tch->form == FormID::SuperSaiyan) {
                    if (GET_HAIRL(tch) == HAIRL_LONG) {
                        ch->sendf("            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
                    } else if (GET_HAIRL(tch) == HAIRL_BALD) {
                        ch->sendf("            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
                    } else if (GET_HAIRL(tch) == HAIRL_SHORT) {
                        ch->sendf("            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
                    } else if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
                        ch->sendf("            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
                    } else if (GET_HAIRL(tch) == HAIRL_RLONG) {
                        ch->sendf("            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
                    }
                    ch->sendf("            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
                    ch->sendf("            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
                    ch->sendf("            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
                } else if (tch->form == FormID::SuperSaiyan2) {
                    if (GET_HAIRL(tch) == HAIRL_LONG) {
                        ch->sendf("            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
                    } else if (GET_HAIRL(tch) == HAIRL_BALD) {
                        ch->sendf("            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
                    } else if (GET_HAIRL(tch) == HAIRL_SHORT) {
                        ch->sendf("            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
                    } else if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
                        ch->sendf("            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
                    } else if (GET_HAIRL(tch) == HAIRL_RLONG) {
                        ch->sendf("            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
                    }
                    ch->sendf("            @D[@cHair Style  @D: @WSharp Spikes. @D]@n\r\n");
                    ch->sendf("            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
                    ch->sendf("            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
                } else if (tch->form == FormID::SuperSaiyan3) {
                    ch->sendf("            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
                    ch->sendf("            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
                    ch->sendf("            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
                    ch->sendf("            @D[@cEye Color   @D: @WAqua Green.   @D]@n\r\n");
                } else if (tch->form == FormID::SuperSaiyan4) {
                    ch->sendf("            @D[@cHair Length @D: @WLong.        @D]@n\r\n");
                    ch->sendf("            @D[@cHair Style  @D: @WSoft Spikes. @D]@n\r\n");
                    ch->sendf("            @D[@cHair Color  @D: @WBlack.       @D]@n\r\n");
                    ch->sendf("            @D[@cEye Color   @D: @WAmber.       @D]@n\r\n");
                }
            }
        }
        if (IS_DEMON(tch) || IS_ICER(tch)) {
            if (GET_HAIRL(tch) == HAIRL_BALD) {
                ch->sendf("            @D[@cHorn Length @D: @WNone.         @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_SHORT) {
                ch->sendf("            @D[@cHorn Length @D: @WShort.        @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
                ch->sendf("            @D[@cHorn Length @D: @WMedium.       @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_LONG) {
                ch->sendf("            @D[@cHorn Length @D: @WLong.         @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_RLONG) {
                ch->sendf("            @D[@cHorn Length @D: @WReally Long.  @D]@n\r\n");
            }
        }
        if (IS_NAMEK(tch) || IS_ARLIAN(tch)) {
            if (GET_HAIRL(tch) == HAIRL_BALD) {
                ch->sendf("            @D[@cAnt. Length @D: @WTiny.        @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_SHORT) {
                ch->sendf("            @D[@cAnt. Length @D: @WShort.       @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
                ch->sendf("            @D[@cAnt. Length @D: @WMedium.      @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_LONG) {
                ch->sendf("            @D[@cAnt. Length @D: @WLong.        @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_RLONG) {
                ch->sendf("            @D[@cAnt. Length @D: @WR. Long.     @D]@n\r\n");
            }
        }
        if (IS_ARLIAN(tch) && IS_FEMALE(tch)) {
            if (GET_HAIRC(tch) == HAIRC_BLACK) {
                ch->sendf("            @D[@cWing Color  @D: @WBlack.        @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_BROWN) {
                ch->sendf("            @D[@cWing Color  @D: @WBrown.        @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_BLONDE) {
                ch->sendf("            @D[@cWing Color  @D: @WBlonde.       @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_GREY) {
                ch->sendf("            @D[@cWing Color  @D: @WGrey.         @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_RED) {
                ch->sendf("            @D[@cWing Color  @D: @WRed.          @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_ORANGE) {
                ch->sendf("            @D[@cWing Color  @D: @WOrange.       @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_GREEN) {
                ch->sendf("            @D[@cWing Color  @D: @WGreen.        @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_BLUE) {
                ch->sendf("            @D[@cWing Color  @D: @WBlue.         @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_PINK) {
                ch->sendf("            @D[@cWing Color  @D: @WPink.         @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_PURPLE) {
                ch->sendf("            @D[@cWing Color  @D: @WPurple.       @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_SILVER) {
                ch->sendf("            @D[@cWing Color  @D: @WSilver.       @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_CRIMSON) {
                ch->sendf("            @D[@cWing Color  @D: @WCrimson.      @D]@n\r\n");
            } else if (GET_HAIRC(tch) == HAIRC_WHITE) {
                ch->sendf("            @D[@cWing Color  @D: @WWhite.        @D]@n\r\n");
            }
        } else if (IS_ARLIAN(tch) && !IS_FEMALE(tch)) {
            ch->sendf("            @D[@cWing Color  @D: @WWhite.        @D]@n\r\n");
        }
        if (IS_MAJIN(tch)) {
            if (GET_HAIRL(tch) == HAIRL_BALD) {
                ch->sendf("            @D[@cFor. Length @D: @WTiny.         @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_SHORT) {
                ch->sendf("            @D[@cFor. Length @D: @WShort.        @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_MEDIUM) {
                ch->sendf("            @D[@cFor. Length @D: @WMedium.       @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_LONG) {
                ch->sendf("            @D[@cFor. Length @D: @WLong.         @D]@n\r\n");
            }
            if (GET_HAIRL(tch) == HAIRL_RLONG) {
                ch->sendf("            @D[@cFor. Length @D: @WR. Long.      @D]@n\r\n");
            }
        }
        if ((!IS_SAIYAN(tch) && !IS_HALFBREED(tch)) ||
            ((IS_SAIYAN(tch) || IS_HALFBREED(tch)) && !IS_TRANSFORMED(tch))) {
            if (GET_EYE(tch) == EYE_BLUE) {
                ch->sendf("            @D[@cEye Color   @D: @WBlue.         @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_BLACK) {
                ch->sendf("            @D[@cEye Color   @D: @WBlack.        @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_GREEN) {
                ch->sendf("            @D[@cEye Color   @D: @WGreen.        @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_BROWN) {
                ch->sendf("            @D[@cEye Color   @D: @WBrown.        @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_RED) {
                ch->sendf("            @D[@cEye Color   @D: @WRed.          @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_AQUA) {
                ch->sendf("            @D[@cEye Color   @D: @WAqua.         @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_PINK) {
                ch->sendf("            @D[@cEye Color   @D: @WPink.         @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_PURPLE) {
                ch->sendf("            @D[@cEye Color   @D: @WPurple.       @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_CRIMSON) {
                ch->sendf("            @D[@cEye Color   @D: @WCrimson.      @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_GOLD) {
                ch->sendf("            @D[@cEye Color   @D: @WGold.         @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_AMBER) {
                ch->sendf("            @D[@cEye Color   @D: @WAmber.        @D]@n\r\n");
            } else if (GET_EYE(tch) == EYE_EMERALD) {
                ch->sendf("            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
            }
        }
        if (GET_SKIN(tch) == SKIN_WHITE) {
            ch->sendf("            @D[@cSkin Color  @D: @WWhite.        @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_TAN) {
            ch->sendf("            @D[@cSkin Color  @D: @WTan.          @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_BLACK) {
            ch->sendf("            @D[@cSkin Color  @D: @WBlack.        @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_GREEN) {
            ch->sendf("            @D[@cSkin Color  @D: @WGreen.        @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_ORANGE) {
            ch->sendf("            @D[@cSkin Color  @D: @WOrange.       @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_YELLOW) {
            ch->sendf("            @D[@cSkin Color  @D: @WYellow.       @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_RED) {
            ch->sendf("            @D[@cSkin Color  @D: @WRed.          @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_GREY) {
            ch->sendf("            @D[@cSkin Color  @D: @WGrey.         @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_BLUE) {
            ch->sendf("            @D[@cSkin Color  @D: @WBlue.         @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_AQUA) {
            ch->sendf("            @D[@cSkin Color  @D: @WAqua.         @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_PINK) {
            ch->sendf("            @D[@cSkin Color  @D: @WPink.         @D]@n\r\n");
        } else if (GET_SKIN(tch) == SKIN_PURPLE) {
            ch->sendf("            @D[@cSkin Color  @D: @WPurple.       @D]@n\r\n");
        }
        if (MAJINIZED(tch) != 0 && MAJINIZED(tch) != 3) {
            ch->sendf("            @D[@cForehead    @D: @mMajin Symbol  @D]@n\r\n");
        }
    } else if (!IS_HUMANOID(tch)) {
        /* Display nothing */
        return;
    } else {
        ch->sendf("Error in bring-desc, please report.\r\n");
    }
}

static void map_draw_room(char map[9][10], int x, int y, room_rnum rnum,
                          struct char_data *ch) {

    auto room = dynamic_cast<room_data*>(world[rnum]);

    for (auto &[door, d] : room->getExits()) {
        auto dest = d->getDestination();
        if(!dest) continue;
        bool isClosed = d->checkFlag(FlagType::Exit, EX_CLOSED);
        bool isSecret = d->checkFlag(FlagType::Exit, EX_SECRET);

        if(isClosed && !isSecret)  {
            switch (door) {
                case NORTH:
                    map[y - 1][x] = '8';
                    break;
                case EAST:
                    map[y][x + 1] = '8';
                    break;
                case SOUTH:
                    map[y + 1][x] = '8';
                    break;
                case WEST:
                    map[y][x - 1] = '8';
                    break;
                case NORTHEAST:
                    map[y - 1][x + 1] = '8';
                    break;
                case NORTHWEST:
                    map[y - 1][x - 1] = '8';
                    break;
                case SOUTHEAST:
                    map[y + 1][x + 1] = '8';
                    break;
                case SOUTHWEST:
                    map[y + 1][x - 1] = '8';
                    break;
            }
        } else if (!isClosed) {
            auto sect = dest->sector_type;
            switch (door) {
                case NORTH:
                    if (dest->isSunken()) {
                        map[y - 1][x] = '=';
                    } else if (sect == SECT_INSIDE) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x] = '2';
                        } else {
                            map[y - 1][x] = 'i';
                        }
                    } else if (sect == SECT_FIELD) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x] = '2';
                        } else {
                            map[y - 1][x] = 'p';
                        }
                    } else if (sect == SECT_DESERT) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x] = '7';
                        } else {
                            map[y - 1][x] = '!';
                        }
                    } else if (sect == SECT_CITY) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x] = '1';
                        } else {
                            map[y - 1][x] = '(';
                        }
                    } else if (sect == SECT_FOREST) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x] = '6';
                        } else {
                            map[y - 1][x] = 'f';
                        }
                    } else if (sect == SECT_MOUNTAIN) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x] = '5';
                        } else {
                            map[y - 1][x] = '^';
                        }
                    } else if (sect == SECT_HILLS) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x] = '3';
                        } else {
                            map[y - 1][x] = 'h';
                        }
                    } else if (sect == SECT_FLYING) {
                        map[y - 1][x] = 's';
                    } else if (sect == SECT_WATER_NOSWIM) {
                        map[y - 1][x] = '`';
                    } else if (sect == SECT_WATER_SWIM) {
                        map[y - 1][x] = '+';
                    } else if (sect == SECT_SHOP) {
                        map[y - 1][x] = '&';
                    } else if (sect == SECT_IMPORTANT) {
                        map[y - 1][x] = '*';
                    } else {
                        map[y - 1][x] = '-';
                    }
                    break;
                case EAST:
                    if (dest->isSunken()) {
                        map[y][x + 1] = '=';
                    } else if (sect == SECT_INSIDE) {
                        if (dest->geffect >= 1) {
                            map[y][x + 1] = '2';
                        } else {
                            map[y][x + 1] = 'i';
                        }
                    } else if (sect == SECT_FIELD) {
                        if (dest->geffect >= 1) {
                            map[y][x + 1] = '2';
                        } else {
                            map[y][x + 1] = 'p';
                        }
                    } else if (sect == SECT_DESERT) {
                        if (dest->geffect >= 1) {
                            map[y][x + 1] = '7';
                        } else {
                            map[y][x + 1] = '!';
                        }
                    } else if (sect == SECT_CITY) {
                        if (dest->geffect >= 1) {
                            map[y][x + 1] = '1';
                        } else {
                            map[y][x + 1] = '(';
                        }
                    } else if (sect == SECT_FOREST) {
                        if (dest->geffect >= 1) {
                            map[y][x + 1] = '6';
                        } else {
                            map[y][x + 1] = 'f';
                        }
                    } else if (sect == SECT_MOUNTAIN) {
                        if (dest->geffect >= 1) {
                            map[y][x + 1] = '5';
                        } else {
                            map[y][x + 1] = '^';
                        }
                    } else if (sect == SECT_HILLS) {
                        if (dest->geffect >= 1) {
                            map[y][x + 1] = '3';
                        } else {
                            map[y][x + 1] = 'h';
                        }
                    } else if (sect == SECT_FLYING) {
                        map[y][x + 1] = 's';
                    } else if (sect == SECT_WATER_NOSWIM) {
                        map[y][x + 1] = '`';
                    } else if (sect == SECT_WATER_SWIM) {
                        map[y][x + 1] = '+';
                    } else if (sect == SECT_SHOP) {
                        map[y][x + 1] = '&';
                    } else if (sect == SECT_IMPORTANT) {
                        map[y][x + 1] = '*';
                    } else {
                        map[y][x + 1] = '-';
                    }
                    break;
                case SOUTH:
                    if (dest->isSunken()) {
                        map[y + 1][x] = '=';
                    } else if (sect == SECT_INSIDE) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x] = '2';
                        } else {
                            map[y + 1][x] = 'i';
                        }
                    } else if (sect == SECT_FIELD) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x] = '2';
                        } else {
                            map[y + 1][x] = 'p';
                        }
                    } else if (sect == SECT_DESERT) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x] = '7';
                        } else {
                            map[y + 1][x] = '!';
                        }
                    } else if (sect == SECT_CITY) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x] = '1';
                        } else {
                            map[y + 1][x] = '(';
                        }
                    } else if (sect == SECT_FOREST) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x] = '6';
                        } else {
                            map[y + 1][x] = 'f';
                        }
                    } else if (sect == SECT_MOUNTAIN) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x] = '5';
                        } else {
                            map[y + 1][x] = '^';
                        }
                    } else if (sect == SECT_HILLS) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x] = '3';
                        } else {
                            map[y + 1][x] = 'h';
                        }
                    } else if (sect == SECT_FLYING) {
                        map[y + 1][x] = 's';
                    } else if (sect == SECT_WATER_NOSWIM) {
                        map[y + 1][x] = '`';
                    } else if (sect == SECT_WATER_SWIM) {
                        map[y + 1][x] = '+';
                    } else if (sect == SECT_SHOP) {
                        map[y + 1][x] = '&';
                    } else if (sect == SECT_IMPORTANT) {
                        map[y + 1][x] = '*';
                    } else {
                        map[y + 1][x] = '-';
                    }
                    break;
                case WEST:
                    if (dest->isSunken()) {
                        map[y][x - 1] = '=';
                    } else if (sect == SECT_INSIDE) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x] = '2';
                        } else {
                            map[y][x - 1] = 'i';
                        }
                    } else if (sect == SECT_FIELD) {
                        if (dest->geffect >= 1) {
                            map[y][x - 1] = '2';
                        } else {
                            map[y][x - 1] = 'p';
                        }
                    } else if (sect == SECT_DESERT) {
                        if (dest->geffect >= 1) {
                            map[y][x - 1] = '7';
                        } else {
                            map[y][x - 1] = '!';
                        }
                    } else if (sect == SECT_CITY) {
                        if (dest->geffect >= 1) {
                            map[y][x - 1] = '1';
                        } else {
                            map[y][x - 1] = '(';
                        }
                    } else if (sect == SECT_FOREST) {
                        if (dest->geffect >= 1) {
                            map[y][x - 1] = '6';
                        } else {
                            map[y][x - 1] = 'f';
                        }
                    } else if (sect == SECT_MOUNTAIN) {
                        if (dest->geffect >= 1) {
                            map[y][x - 1] = '5';
                        } else {
                            map[y][x - 1] = '^';
                        }
                    } else if (sect == SECT_HILLS) {
                        if (dest->geffect >= 1) {
                            map[y][x - 1] = '3';
                        } else {
                            map[y][x - 1] = 'h';
                        }
                    } else if (sect == SECT_FLYING) {
                        map[y][x - 1] = 's';
                    } else if (sect == SECT_WATER_NOSWIM) {
                        map[y][x - 1] = '`';
                    } else if (sect == SECT_WATER_SWIM) {
                        map[y][x - 1] = '+';
                    } else if (sect == SECT_SHOP) {
                        map[y][x - 1] = '&';
                    } else if (sect == SECT_IMPORTANT) {
                        map[y][x - 1] = '*';
                    } else {
                        map[y][x - 1] = '-';
                    }
                    break;
                case NORTHEAST:
                    if (dest->isSunken()) {
                        map[y - 1][x + 1] = '=';
                    } else if (sect == SECT_INSIDE) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x + 1] = '2';
                        } else {
                            map[y - 1][x + 1] = 'i';
                        }
                    } else if (sect == SECT_FIELD) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x + 1] = '2';
                        } else {
                            map[y - 1][x + 1] = 'p';
                        }
                    } else if (sect == SECT_DESERT) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x + 1] = '7';
                        } else {
                            map[y - 1][x + 1] = '!';
                        }
                    } else if (sect == SECT_CITY) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x + 1] = '1';
                        } else {
                            map[y - 1][x + 1] = '(';
                        }
                    } else if (sect == SECT_FOREST) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x + 1] = '6';
                        } else {
                            map[y - 1][x + 1] = 'f';
                        }
                    } else if (sect == SECT_MOUNTAIN) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x + 1] = '5';
                        } else {
                            map[y - 1][x + 1] = '^';
                        }
                    } else if (sect == SECT_HILLS) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x + 1] = '3';
                        } else {
                            map[y - 1][x + 1] = 'h';
                        }
                    } else if (sect == SECT_FLYING) {
                        map[y - 1][x + 1] = 's';
                    } else if (sect == SECT_WATER_NOSWIM) {
                        map[y - 1][x + 1] = '`';
                    } else if (sect == SECT_WATER_SWIM) {
                        map[y - 1][x + 1] = '+';
                    } else if (sect == SECT_SHOP) {
                        map[y - 1][x + 1] = '&';
                    } else if (sect == SECT_IMPORTANT) {
                        map[y - 1][x + 1] = '*';
                    } else {
                        map[y - 1][x + 1] = '-';
                    }
                    break;
                case NORTHWEST:
                    if (dest->isSunken()) {
                        map[y - 1][x - 1] = '=';
                    } else if (sect == SECT_INSIDE) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x - 1] = '2';
                        } else {
                            map[y - 1][x - 1] = 'i';
                        }
                    } else if (sect == SECT_FIELD) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x - 1] = '2';
                        } else {
                            map[y - 1][x - 1] = 'p';
                        }
                    } else if (sect == SECT_DESERT) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x - 1] = '7';
                        } else {
                            map[y - 1][x - 1] = '!';
                        }
                    } else if (sect == SECT_CITY) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x - 1] = '1';
                        } else {
                            map[y - 1][x - 1] = '(';
                        }
                    } else if (sect == SECT_FOREST) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x - 1] = '6';
                        } else {
                            map[y - 1][x - 1] = 'f';
                        }
                    } else if (sect == SECT_MOUNTAIN) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x - 1] = '5';
                        } else {
                            map[y - 1][x - 1] = '^';
                        }
                    } else if (sect == SECT_HILLS) {
                        if (dest->geffect >= 1) {
                            map[y - 1][x - 1] = '3';
                        } else {
                            map[y - 1][x - 1] = 'h';
                        }
                    } else if (sect == SECT_FLYING) {
                        map[y - 1][x - 1] = 's';
                    } else if (sect == SECT_WATER_NOSWIM) {
                        map[y - 1][x - 1] = '`';
                    } else if (sect == SECT_WATER_SWIM) {
                        map[y - 1][x - 1] = '+';
                    } else if (sect == SECT_SHOP) {
                        map[y - 1][x - 1] = '&';
                    } else if (sect == SECT_IMPORTANT) {
                        map[y - 1][x - 1] = '*';
                    } else {
                        map[y - 1][x - 1] = '-';
                    }
                    break;
                case SOUTHEAST:
                    if (dest->isSunken()) {
                        map[y + 1][x + 1] = '=';
                    } else if (sect == SECT_INSIDE) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x + 1] = '2';
                        } else {
                            map[y + 1][x + 1] = 'i';
                        }
                    } else if (sect == SECT_FIELD) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x + 1] = '2';
                        } else {
                            map[y + 1][x + 1] = 'p';
                        }
                    } else if (sect == SECT_DESERT) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x + 1] = '7';
                        } else {
                            map[y + 1][x + 1] = '!';
                        }
                    } else if (sect == SECT_CITY) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x + 1] = '1';
                        } else {
                            map[y + 1][x + 1] = '(';
                        }
                    } else if (sect == SECT_FOREST) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x + 1] = '6';
                        } else {
                            map[y + 1][x + 1] = 'f';
                        }
                    } else if (sect == SECT_MOUNTAIN) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x + 1] = '5';
                        } else {
                            map[y + 1][x + 1] = '^';
                        }
                    } else if (sect == SECT_HILLS) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x + 1] = '3';
                        } else {
                            map[y + 1][x + 1] = 'h';
                        }
                    } else if (sect == SECT_FLYING) {
                        map[y + 1][x + 1] = 's';
                    } else if (sect == SECT_WATER_NOSWIM) {
                        map[y + 1][x + 1] = '`';
                    } else if (sect == SECT_WATER_SWIM) {
                        map[y + 1][x + 1] = '+';
                    } else if (sect == SECT_SHOP) {
                        map[y + 1][x + 1] = '&';
                    } else if (sect == SECT_IMPORTANT) {
                        map[y + 1][x + 1] = '*';
                    } else {
                        map[y + 1][x + 1] = '-';
                    }
                    break;
                case SOUTHWEST:
                    if (dest->isSunken()) {
                        map[y + 1][x - 1] = '=';
                    } else if (sect == SECT_INSIDE) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x - 1] = '2';
                        } else {
                            map[y + 1][x - 1] = 'i';
                        }
                    } else if (sect == SECT_FIELD) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x - 1] = '2';
                        } else {
                            map[y + 1][x - 1] = 'p';
                        }
                    } else if (sect == SECT_DESERT) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x - 1] = '7';
                        } else {
                            map[y + 1][x - 1] = '!';
                        }
                    } else if (sect == SECT_CITY) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x - 1] = '1';
                        } else {
                            map[y + 1][x - 1] = '(';
                        }
                    } else if (sect == SECT_FOREST) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x - 1] = '6';
                        } else {
                            map[y + 1][x - 1] = 'f';
                        }
                    } else if (sect == SECT_MOUNTAIN) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x - 1] = '5';
                        } else {
                            map[y + 1][x - 1] = '^';
                        }
                    } else if (sect == SECT_HILLS) {
                        if (dest->geffect >= 1) {
                            map[y + 1][x - 1] = '3';
                        } else {
                            map[y + 1][x - 1] = 'h';
                        }
                    } else if (sect == SECT_FLYING) {
                        map[y + 1][x - 1] = 's';
                    } else if (sect == SECT_WATER_NOSWIM) {
                        map[y + 1][x - 1] = '`';
                    } else if (sect == SECT_WATER_SWIM) {
                        map[y + 1][x - 1] = '+';
                    } else if (sect == SECT_SHOP) {
                        map[y + 1][x - 1] = '&';
                    } else if (sect == SECT_IMPORTANT) {
                        map[y + 1][x - 1] = '*';
                    } else {
                        map[y + 1][x - 1] = '-';
                    }
                    break;
            }
        }
    }
}

ACMD(do_map) {
    gen_map(ch, 1);
}

static void gen_map(struct char_data *ch, int num) {
    int door, i;
    char map[9][10] = {{'-'},
                       {'-'}};
    char buf2[MAX_INPUT_LENGTH];

    if (num == 1) {
        /* Map Key */
        ch->sendf("@W               @D-[@CArea Map@D]-\r\n");
        ch->sendf("@D-------------------------------------------@w\r\n");
        ch->sendf("@WC = City, @wI@W = Inside, @GP@W = Plain, @gF@W = Forest\r\n");
        ch->sendf("@DM@W = Mountain, @yH@W = Hills, @CS@W = Sky, @BW@W = Water\r\n");
        ch->sendf("@bU@W = Underwater, @m$@W = Shop, @m#@W = Important,\r\n");
        ch->sendf("@YD@W = Desert, @c~@W = Shallow Water, @4 @n@W = Lava,\r\n");
        ch->sendf("@WLastly @RX@W = You.\r\n");
        ch->sendf("@D-------------------------------------------\r\n");
        ch->sendf("@D                  @CNorth@w\r\n");
        ch->sendf("@D                    @c^@w\r\n");
        ch->sendf("@D             @CWest @c< O > @CEast@w\r\n");
        ch->sendf("@D                    @cv@w\r\n");
        ch->sendf("@D                  @CSouth@w\r\n");
        ch->sendf("@D                ---------@w\r\n");
    }

    /* blank the map */
    for (i = 0; i < 9; i++) {
        strcpy(map[i], "         ");
    }

    auto room = ch->getRoom();
    
    /* print out exits */
    map_draw_room(map, 4, 4, ch->getRoom()->vn, ch);
    auto exits = room->getExits();
    for (auto &[door, d] : exits) {
        if(d->checkFlag(FlagType::Exit, EX_CLOSED)) continue;
        auto dest = d->getDestination();
        if(!dest) continue;

        switch (door) {
            case NORTH:
                map_draw_room(map, 4, 3, dest->uid, ch);
                break;
            case EAST:
                map_draw_room(map, 5, 4, dest->uid, ch);
                break;
            case SOUTH:
                map_draw_room(map, 4, 5, dest->uid, ch);
                break;
            case WEST:
                map_draw_room(map, 3, 4, dest->uid, ch);
                break;
            case NORTHEAST:
                map_draw_room(map, 5, 3, dest->uid, ch);
                break;
            case NORTHWEST:
                map_draw_room(map, 3, 3, dest->uid, ch);
                break;
            case SOUTHEAST:
                map_draw_room(map, 5, 5, dest->uid, ch);
                break;
            case SOUTHWEST:
                map_draw_room(map, 3, 5, dest->uid, ch);
                break;
        }
    }

    /* make it obvious what room they are in */
    map[4][4] = 'x';

    /* print out the map */
    int key = 0;
    *buf2 = '\0';
    for (i = 2; i < 9; i++) {
        if (i > 6) {
            continue;
        }
        if (num == 1) {
            sprintf(buf2, "@w                %s\r\n", map[i]);
        } else {
            if (i == 2) {
                sprintf(buf2, "@w       @w|%s@w|           %s",
                        (exits[0] && !EXIT_FLAGGED(exits[0], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[0],
                                                                                                     EX_CLOSED)
                                                                                             ? " @rN " : " @CN ")
                                                                                          : "   ", map[i]);
            }
            if (i == 3) {
                sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
                        (exits[6] && !EXIT_FLAGGED(exits[6], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[6],
                                                                                                     EX_CLOSED)
                                                                                             ? " @rNW" : " @CNW")
                                                                                          : "   ",
                        (exits[4] && !EXIT_FLAGGED(exits[4], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[4],
                                                                                                     EX_CLOSED)
                                                                                             ? " @yU " : " @YU ")
                                                                                          : "   ",
                        (exits[7] && !EXIT_FLAGGED(exits[7], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[7],
                                                                                                     EX_SECRET)
                                                                                             ? "@rNE " : "@CNE ")
                                                                                          : "   ", map[i]);
            }
            if (i == 4) {
                sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
                        (exits[3] && !EXIT_FLAGGED(exits[3], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[3],
                                                                                                     EX_CLOSED)
                                                                                             ? "  @rW" : "  @CW")
                                                                                          : "   ",
                        (exits[10] && !EXIT_FLAGGED(exits[10], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                            exits[10],
                                                                                                       EX_CLOSED)
                                                                                               ? " @rI " : " @mI ")
                                                                                            : ((exits[11] &&
                                                                                                !EXIT_FLAGGED(
                                                                                                        exits[11],
                                                                                                        EX_SECRET))
                                                                                               ? (EXIT_FLAGGED(
                                                                                                          exits[11],
                                                                                                          EX_CLOSED)
                                                                                                  ? "@rOUT" : "@mOUT")
                                                                                               : "@r{ }"),
                        (exits[1] && !EXIT_FLAGGED(exits[1], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[1],
                                                                                                     EX_CLOSED)
                                                                                             ? "@rE  " : "@CE  ")
                                                                                          : "   ", map[i]);
            }
            if (i == 5) {
                sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
                        (exits[9] && !EXIT_FLAGGED(exits[9], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[9],
                                                                                                     EX_CLOSED)
                                                                                             ? " @rSW" : " @CSW")
                                                                                          : "   ",
                        (exits[5] && !EXIT_FLAGGED(exits[5], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[5],
                                                                                                     EX_CLOSED)
                                                                                             ? " @yD " : " @YD ")
                                                                                          : "   ",
                        (exits[8] && !EXIT_FLAGGED(exits[8], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[8],
                                                                                                     EX_SECRET)
                                                                                             ? "@rSE " : "@CSE ")
                                                                                          : "   ", map[i]);
            }
            if (i == 6) {
                sprintf(buf2, "@w       @w|%s@w|           %s",
                        (exits[2] && !EXIT_FLAGGED(exits[2], EX_SECRET)) ? (EXIT_FLAGGED(
                                                                                                          exits[2],
                                                                                                     EX_CLOSED)
                                                                                             ? " @rS " : " @CS ")
                                                                                          : "   ", map[i]);
            }
        }
        search_replace(buf2, "x", "@RX");
        search_replace(buf2, "&", "@m$");
        search_replace(buf2, "*", "@m#");
        search_replace(buf2, "+", "@c~");
        search_replace(buf2, "s", "@CS");
        search_replace(buf2, "i", "@wI");
        search_replace(buf2, "(", "@WC");
        search_replace(buf2, "^", "@DM");
        search_replace(buf2, "h", "@yH");
        search_replace(buf2, "`", "@BW");
        search_replace(buf2, "=", "@bU");
        search_replace(buf2, "p", "@GP");
        search_replace(buf2, "f", "@gF");
        search_replace(buf2, "!", "@YD");
        search_replace(buf2, "-", "@w:");
        /* ------- Do Lava Rooms ------- */
        search_replace(buf2, "1", "@4@YC@n");
        search_replace(buf2, "2", "@4@YP@n");
        search_replace(buf2, "3", "@4@YH@n");
        search_replace(buf2, "7", "@4@YD@n");
        search_replace(buf2, "5", "@4@YM@n");
        search_replace(buf2, "6", "@4@YF@n");
        /* ------- Do Closed Rooms------- */
        search_replace(buf2, "8", "@1 @n");

        if (num != 1) {
            if (key == 0) {
                ch->sendf("%s    @WC: City, @wI@W: Inside, @GP@W: Plain@n\r\n", buf2);
            }
            if (key == 1) {
                ch->sendf("%s    @gF@W: Forest, @DM@W: Mountain, @yH@W: Hills@n\r\n", buf2);
            }
            if (key == 2) {
                ch->sendf("%s    @CS@W: Sky, @BW@W: Water, @bU@W: Underwater@n\r\n", buf2);
            }
            if (key == 3) {
                ch->sendf("%s    @m$@W: Shop, @m#@W: Important, @YD@W: Desert@n\r\n", buf2);
            }
            if (key == 4) {
                ch->sendf("%s    @c~@W: Shallow Water, @4 @n@W: Lava, @RX@W: You@n\r\n", buf2);
            }
            key += 1;
        } else {
            ch->sendf(buf2);
        }
    }
    if (num == 1) {
        ch->sendf("@D                ---------@w\r\n");
    }
}


static void display_spells(struct char_data *ch, struct obj_data *obj) {
    int i;

    ch->sendf("The spellbook contains the following spells:\r\n");
    ch->sendf("@c---@wSpell Name@c------------------------------------@w# of pages@c-----@n\r\n");

    if (!obj->sbinfo) {
        return;
    }
    for (i = 0; i < SPELLBOOK_SIZE; i++) {
        if (obj->sbinfo[i].spellname != 0) {
            if (obj->sbinfo[i].spellname > MAX_SPELLS) {
                continue;
            }
            ch->sendf("@y%-20s@n					[@R%2d@n]\r\n", spell_info[obj->sbinfo[i].spellname].name,
                         obj->sbinfo[i].pages);
        }
    }
    return;
}

static void display_scroll(struct char_data *ch, struct obj_data *obj) {
    ch->sendf("The scroll contains the following spell:\r\n");
    ch->sendf("@c---@wSpell Name@c---------------------------------------------------@n\r\n");
    ch->sendf("@y%-20s@n\r\n", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1)));
    return;
}

static void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode) {
    if (!obj || !ch) {
        basic_mud_log("SYSERR: nullptr pointer in show_obj_to_char()");
        /*  SYSERR_DESC:
     *  Somehow a nullptr pointer was sent to show_obj_to_char() in either the
     *  'obj' or the 'ch' variable.  The error will indicate which was nullptr
     *  be listing both of the pointers passed to it.  This is often a
     *  difficult one to trace, and may require stepping through a debugger.
     */
        return;
    }

    int spotted = false;

    if (GET_SKILL(ch, SKILL_SPOT) > rand_number(20, 110)) {
        spotted = true;
    }

    switch (mode) {
        case SHOW_OBJ_LONG:
            /*
     * hide objects starting with . from non-holylighted people
     * Idea from Elaseth of TBA
     */
            if (*obj->room_description == '.' && (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
                return;
            if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE && GET_ROOM_VNUM(IN_ROOM(ch)) == GET_OBJ_VAL(obj, 0)) {
                return;
            }
            if (SITTING(obj) && GET_ADMLEVEL(ch) < 1) {
                return;
            }
            if (SITTING(obj) && GET_ADMLEVEL(ch) >= 1) {
                ch->sendf("@D(@YBeing Used@D)@w");
            }
            if (GET_OBJ_TYPE(obj) == ITEM_PLANT &&
                (ROOM_FLAGGED(IN_ROOM(obj), ROOM_GARDEN1) || ROOM_FLAGGED(IN_ROOM(obj), ROOM_GARDEN2))) {
                see_plant(obj, ch);
                return;
            }
            if (OBJ_FLAGGED(obj, ITEM_BURIED)) {
                char bury[MAX_INPUT_LENGTH];
                if (!IS_CORPSE(obj)) {
                    if (GET_OBJ_WEIGHT(obj) < 10) {
                        sprintf(bury, "small mound of");
                    } else if (GET_OBJ_WEIGHT(obj) < 50) {
                        sprintf(bury, "medium sized mound of");
                    } else if (GET_OBJ_WEIGHT(obj) < 1000) {
                        sprintf(bury, "large mound of");
                    } else {
                        sprintf(bury, "gigantic mound of");
                    }
                } else {
                    sprintf(bury, "recent grave covered by");
                }
                if (spotted == true && SECT(IN_ROOM(obj)) != SECT_DESERT) {
                    ch->sendf("@yA %s soft dirt is here.@n\r\n", bury);
                    return;
                } else if (spotted == true && SECT(IN_ROOM(obj)) == SECT_DESERT) {
                    ch->sendf("@YA %s soft sand is here.@n\r\n", bury);
                    return;
                } else {
                    return;
                }
            }
            if (GET_OBJ_VNUM(obj) == 11) {
                if(obj->gravity) {
                    auto msg = fmt::format("@wA gravity generator, set to {}x gravity, is built here", obj->gravity.value());
                    ch->sendf(msg.c_str());
                } else {
                    ch->sendf("@wA gravity generator, currently on standby, is built here");
                }

            } else if (GET_OBJ_VNUM(obj) == 79) {
                ch->sendf(
                             "@wA @cG@Cl@wa@cc@Ci@wa@cl @wW@ca@Cl@wl @D[@C%s@D]@w is blocking access to the @G%s@w direction",
                             add_commas(GET_OBJ_WEIGHT(obj)).c_str(), dirs[GET_OBJ_COST(obj)]);
            } else {
                ch->sendf("@w");
                if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
                    if (GET_OBJ_POSTED(obj) == nullptr) {
                        ch->sendf("@D[@G%d@D]@w ", GET_OBJ_VNUM(obj));
                            ch->sendf("%s ", obj->scriptString().c_str());
                    } else {
                        if (GET_OBJ_POSTTYPE(obj) <= 0) {
                            ch->sendf("@D[@G%d@D]@w ", GET_OBJ_VNUM(obj));
                                ch->sendf("%s ", obj->scriptString().c_str());
                        }
                    }
                }

                if (GET_OBJ_POSTTYPE(obj) > 0) {
                    if (GET_OBJ_POSTED(obj)) {
                        return;
                    } else {
                        ch->sendf("%s@w, has been posted here.@n", obj->getShortDesc());
                    }
                } else {
                    if (!OBJ_FLAGGED(obj, ITEM_BURIED)) {
                        ch->sendf("%s@n", obj->room_description);
                    }
                }

                if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
                    if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && GET_OBJ_VNUM(obj) > 19199)
                        ch->sendf("\r\n@c...its outer hatch is open@n");
                    else if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && GET_OBJ_VNUM(obj) <= 19199)
                        ch->sendf("\r\n@c...its door is open@n");
                }
                if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !IS_CORPSE(obj)) {
                    if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && !OBJ_FLAGGED(obj, ITEM_SHEATH))
                        ch->sendf(". @D[@G-open-@D]@n");
                    else if (!OBJ_FLAGGED(obj, ITEM_SHEATH))
                        ch->sendf(". @D[@rclosed@D]@n");
                }
                if (GET_OBJ_TYPE(obj) == ITEM_HATCH) {
                    if (!OBJVAL_FLAGGED(obj, CONT_CLOSED))
                        ch->sendf(", it is open");
                    else if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
                        ch->sendf(", it is closed");
                    if (OBJVAL_FLAGGED(obj, CONT_LOCKED))
                        ch->sendf(" and locked@n");
                    else
                        ch->sendf("@n");
                }
                if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
                    if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj)) {
                        ch->sendf(", and it has been ate on@n");
                    }
                }
            }
            break;

        case SHOW_OBJ_SHORT:
            if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
                ch->sendf("[%d] ", GET_OBJ_VNUM(obj));
                    ch->sendf("%s ", obj->scriptString().c_str());
            }

            if (PRF_FLAGGED(ch, PRF_IHEALTH)) {
                ch->sendf("@D<@gH@D: @C%d@D>@w %s", GET_OBJ_VAL(obj, VAL_ALL_HEALTH), obj->getShortDesc());
            } else {
                ch->sendf("%s", obj->getShortDesc());
            }
            if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
                if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj)) {
                    ch->sendf(", and it has been ate on.@n");
                }
            }
            if (GET_OBJ_VNUM(obj) == 255) {
                switch (GET_OBJ_VAL(obj, 0)) {
                    case 0:
                    case 1:
                        ch->sendf(" @D[@wQuality @RC@D]@n");
                        break;
                    case 2:
                        ch->sendf(" @D[@wQuality @RC+@D]@n");
                        break;
                    case 3:
                        ch->sendf(" @D[@wQuality @yC++@D]@n");
                        break;
                    case 4:
                        ch->sendf(" @D[@wQuality @yB@D]@n");
                        break;
                    case 5:
                        ch->sendf(" @D[@wQuality @CB+@D]@n");
                        break;
                    case 6:
                        ch->sendf(" @D[@wQuality @CB++@D]@n");
                        break;
                    case 7:
                        ch->sendf(" @D[@wQuality @CA@D]@n");
                        break;
                    case 8:
                        ch->sendf(" @D[@wQuality @GA+@D]@n");
                        break;
                }
            }

            if (GET_OBJ_VNUM(obj) == 3424) {
                ch->sendf(" @D[@bInk Remaining@D: @w%d@D]@n", GET_OBJ_VAL(obj, 6));
            }
            if (GET_OBJ_VNUM(obj) == 3423) {
                ch->sendf(" @D[@B%d@D/@B24 Inks@D]@n", GET_OBJ_VAL(obj, 6));
            }
            if (OBJ_FLAGGED(obj, ITEM_THROW)) {
                ch->sendf(" @D[@RThrow Only@D]@n");
            }
            if (GET_OBJ_TYPE(obj) == ITEM_PLANT && !OBJ_FLAGGED(obj, ITEM_MATURE)) {
                if (GET_OBJ_VAL(obj, VAL_WATERLEVEL) < -9) {
                    ch->sendf("@D[@RDead@D]@n");
                } else {
                    switch (GET_OBJ_VAL(obj, VAL_MATURITY)) {
                        case 0:
                            ch->sendf(" @D[@ySeed@D]@n");
                            break;
                        case 1:
                            ch->sendf(" @D[@GSprout@D]@n");
                            break;
                        case 2:
                            ch->sendf(" @D[@GYoung@D]@n");
                            break;
                        case 3:
                            ch->sendf(" @D[@GMature@D]@n");
                            break;
                        case 4:
                            ch->sendf(" @D[@GBudding@D]@n");
                            break;
                        case 5:
                            ch->sendf("@D[@GClose Harvest@D]@n");
                            break;
                        case 6:
                            ch->sendf("@D[@gHarvest@D]@n");
                            break;
                    }
                }
            }
            if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !IS_CORPSE(obj)) {
                if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && !OBJ_FLAGGED(obj, ITEM_SHEATH))
                    ch->sendf(" @D[@G-open-@D]@n");
                else if (!OBJ_FLAGGED(obj, ITEM_SHEATH))
                    ch->sendf(" @D[@rclosed@D]@n");
            }
            if (OBJ_FLAGGED(obj, ITEM_DUPLICATE)) {
                ch->sendf(" @D[@YDuplicate@D]@n");
            }
            break;

        case SHOW_OBJ_ACTION:
            switch (GET_OBJ_TYPE(obj)) {
                case ITEM_NOTE:
                    if (auto ld = obj->getLookDesc(); !ld.empty()) {
                        write_to_output(ch->desc, "There is something written on it:\r\n\r\n%s", ld);
                    } else
                        ch->sendf("There appears to be nothing written on it.\r\n");
                    return;

                case ITEM_BOARD:
                    show_board(GET_OBJ_VNUM(obj), ch);
                    break;

                case ITEM_CONTROL:
                    ch->sendf("@RFUEL@D: %s%s@n\r\n",
                                 GET_FUEL(obj) >= 200 ? "@G" : GET_FUEL(obj) >= 100 ? "@Y" : "@r",
                                 add_commas(GET_FUEL(obj)).c_str());
                    break;

                case ITEM_DRINKCON:
                    ch->sendf("It looks like a drink container.\r\n");
                    break;

                case ITEM_LIGHT:
                    if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS) == -1)
                        ch->sendf("Light Cycles left: Infinite\r\n");
                    else
                        ch->sendf("Light Cycles left: [%d]\r\n", GET_OBJ_VAL(obj, VAL_LIGHT_HOURS));
                    break;

                case ITEM_FOOD:
                    if (FOOB(obj) >= 4) {
                        if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj) / 4) {
                            ch->sendf("Condition of the food: Almost gone.\r\n");
                        } else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj) / 2) {
                            ch->sendf("Condition of the food: Half Eaten.");
                        } else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj)) {
                            ch->sendf("Condition of the food: Partially Eaten.");
                        } else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) == FOOB(obj)) {
                            ch->sendf("Condition of the food: Whole.");
                        }
                    } else if (FOOB(obj) > 0) {
                        if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj)) {
                            ch->sendf("Condition of the food: Almost gone.");
                        } else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) == FOOB(obj)) {
                            ch->sendf("Condition of the food: Whole.");
                        }
                    } else {
                        ch->sendf("Condition of the food: Insignificant.");
                    }
                    break;

                case ITEM_SPELLBOOK:
                    ch->sendf("It looks like an arcane tome.\r\n");
                    display_spells(ch, obj);
                    break;

                case ITEM_SCROLL:
                    ch->sendf("It looks like an arcane scroll.\r\n");
                    display_scroll(ch, obj);
                    break;

                case ITEM_VEHICLE:
                    if (GET_OBJ_VNUM(obj) > 19199) {
                        ch->sendf("@YSyntax@D: @CUnlock hatch\r\n");
                        ch->sendf("@YSyntax@D: @COpen hatch\r\n");
                        ch->sendf("@YSyntax@D: @CClose hatch\r\n");
                        ch->sendf("@YSyntax@D: @CEnter hatch\r\n");
                    } else {
                        ch->sendf("@YSyntax@D: @CUnlock door\r\n");
                        ch->sendf("@YSyntax@D: @COpen door\r\n");
                        ch->sendf("@YSyntax@D: @CClose door\r\n");
                        ch->sendf("@YSyntax@D: @CEnter door\r\n");
                    }
                    break;

                case ITEM_HATCH:
                    if (GET_OBJ_VNUM(obj) > 19199) {
                        ch->sendf("@YSyntax@D: @CUnlock hatch\r\n");
                        ch->sendf("@YSyntax@D: @COpen hatch\r\n");
                        ch->sendf("@YSyntax@D: @CClose hatch\r\n");
                        ch->sendf("@YSyntax@D: @CLeave@n\r\n");
                    } else {
                        ch->sendf("@YSyntax@D: @CUnlock door\r\n");
                        ch->sendf("@YSyntax@D: @COpen door\r\n");
                        ch->sendf("@YSyntax@D: @CClose door\r\n");
                        ch->sendf("@YSyntax@D: @CEnter door\r\n");
                    }
                    break;

                case ITEM_WINDOW:
                    look_out_window(ch, obj->name);
                    return;
                    break;

                default:
                    if (!IS_CORPSE(obj)) {
                        ch->sendf("You see nothing special..\r\n");
                    } else {
                        int mention = false;
                        ch->sendf("This corpse has ");

                        if (GET_OBJ_VAL(obj, VAL_CORPSE_HEAD) == 0) {
                            ch->sendf("no head,");
                            mention = true;
                        }

                        if (GET_OBJ_VAL(obj, VAL_CORPSE_RARM) == 0) {
                            ch->sendf("no right arm, ");
                            mention = true;
                        } else if (GET_OBJ_VAL(obj, VAL_CORPSE_RARM) == 2) {
                            ch->sendf("a broken right arm, ");
                            mention = true;
                        }

                        if (GET_OBJ_VAL(obj, VAL_CORPSE_LARM) == 0) {
                            ch->sendf("no left arm, ");
                            mention = true;
                        } else if (GET_OBJ_VAL(obj, VAL_CORPSE_LARM) == 2) {
                            ch->sendf("a broken left arm, ");
                            mention = true;
                        }

                        if (GET_OBJ_VAL(obj, VAL_CORPSE_RLEG) == 0) {
                            ch->sendf("no right leg, ");
                            mention = true;
                        } else if (GET_OBJ_VAL(obj, VAL_CORPSE_RLEG) == 2) {
                            ch->sendf("a broken right leg, ");
                            mention = true;
                        }

                        if (GET_OBJ_VAL(obj, VAL_CORPSE_LLEG) == 0) {
                            ch->sendf("no left leg, ");
                            mention = true;
                        } else if (GET_OBJ_VAL(obj, VAL_CORPSE_LLEG) == 2) {
                            ch->sendf("a broken left leg, ");
                            mention = true;
                        }

                        if (mention == false) {
                            ch->sendf("nothing missing from it but life.");
                        } else {
                            ch->sendf("and is dead.");
                        }

                        ch->sendf("\r\n");
                    }
                    break;
            }

            if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
                int num = 0;
                if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
                    num = 1;
                } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
                    num = 0;
                } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
                    num = 3;
                } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
                    num = 2;
                } else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
                    num = 4;
                } else {
                    num = 5;
                }
                ch->sendf("The weapon type of %s@n is '%s'.\r\n", GET_OBJ_SHORT(obj), weapon_disp[num]);
                ch->sendf("You could wield it %s.\r\n", wield_names[wield_type(get_size(ch), obj)]);
            }
            diag_obj_to_char(obj, ch);
            ch->sendf("It appears to be made of %s, and weighs %s", material_names[GET_OBJ_MATERIAL(obj)],
                         add_commas(GET_OBJ_WEIGHT(obj)).c_str());
            break;

        default:
            basic_mud_log("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
            /*  SYSERR_DESC:
     *  show_obj_to_char() has some predefined 'mode's (argument #3) to tell
     *  it what to display to the character when it is called.  If the mode
     *  is not one of these, it will output this error, and indicate what
     *  mode was passed to it.  To correct it, you will need to find the
     *  call with the incorrect mode and change it to an acceptable mode.
     */
            return;
    }

    if ((show_obj_modifiers(obj, ch) || (mode != SHOW_OBJ_ACTION)))
        ch->sendf("\r\n");
}

static int show_obj_modifiers(struct obj_data *obj, struct char_data *ch) {
    int found = false;

    if (OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
        ch->sendf(" (invisible)");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
        ch->sendf(" ..It glows blue!");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
        ch->sendf(" ..It glows yellow!");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_GLOW)) {
        ch->sendf(" @D(@GGlowing@D)@n");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_HOT)) {
        ch->sendf(" @D(@RHOT@D)@n");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_HUM)) {
        ch->sendf(" @D(@RHumming@D)@n");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_SLOT2)) {
        if (OBJ_FLAGGED(obj, ITEM_SLOT_ONE) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
            ch->sendf(" @D[@m1/2 Tokens@D]@n");
        else if (OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
            ch->sendf(" @D[@m2/2 Tokens@D]@n");
        else
            ch->sendf(" @D[@m0/2 Tokens@D]@n");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_SLOT1)) {
        if (OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
            ch->sendf(" @D[@m1/1 Tokens@D]@n");
        else
            ch->sendf(" @D[@m0/1 Tokens@D]@n");
        found++;
    }
    if (KICHARGE(obj) > 0) {
        int num = (KIDIST(obj) * 20) + rand_number(1, 5);
        ch->sendf(" %d meters away", num);
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_CUSTOM)) {
        ch->sendf(" @D(@YCUSTOM@D)@n");
    }
    if (OBJ_FLAGGED(obj, ITEM_RESTRING)) {
        ch->sendf(" @D(@R%s@D)@n", GET_ADMLEVEL(ch) > 0 ? obj->name : "*");
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
        if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STEEL ||
            GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL ||
            GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_METAL) {
            ch->sendf(", and appears to be twisted and broken.");
        } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_WOOD) {
            ch->sendf(", and is broken into hundreds of splinters.");
        } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_GLASS) {
            ch->sendf(", and is shattered on the ground.");
        } else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STONE) {
            ch->sendf(", and is a pile of rubble.");
        } else {
            ch->sendf(", and is broken.");
        }
        found++;
    } else {
        if (GET_OBJ_TYPE(obj) != ITEM_BOARD) {
            if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
                ch->sendf(".");
            }
            if (!IS_NPC(ch) && GET_OBJ_POSTED(obj) && GET_OBJ_POSTTYPE(obj) <= 0) {
                struct obj_data *obj2 = GET_OBJ_POSTED(obj);
                char dvnum[200];
                *dvnum = '\0';
                sprintf(dvnum, "@D[@G%d@D] @w", GET_OBJ_VNUM(obj2));
                ch->sendf("\n...%s%s has been posted to it.", PRF_FLAGGED(ch, PRF_ROOMFLAGS) ? dvnum : "",
                             obj2->getShortDesc());
            }
        }
        found++;
    }
    return (found);
}

static void list_obj_to_char(std::vector<obj_data*> list, struct char_data *ch, int mode, int show) {
    struct obj_data *i, *j, *d;
    bool found = false;
    int num;

    /* Loop through all objects in the list */
    for (auto i : list) {
        if (i->room_description == nullptr)
            continue;
        if (strcasecmp(i->room_description, "undefined") == 0)
            continue;
        num = 0;
        d = i;
        if ((CAN_SEE_OBJ(ch, d) &&
             ((*d->room_description != '.' && *d->getShortDesc().c_str() != '.') || PRF_FLAGGED(ch, PRF_HOLYLIGHT))) ||
            (GET_OBJ_TYPE(d) == ITEM_LIGHT)) {
            if (num > 1)
                ch->sendf("@D(@Rx@Y%2i@D)@n ", num);
            show_obj_to_char(d, ch, mode);
            found = true;
        }
    }
    if (!found && show)
        ch->sendf(" Nothing.\r\n");
}

static void diag_obj_to_char(struct obj_data *obj, struct char_data *ch) {
    struct {
        int percent;
        const char *text;
    } diagnosis[] = {
            {100, "is in excellent condition."},
            {90,  "has a few scuffs."},
            {75,  "has some small scuffs and scratches."},
            {50,  "has quite a few scratches."},
            {30,  "has some big nasty scrapes and scratches."},
            {15,  "looks pretty damaged."},
            {0,   "is in awful condition."},
            {-1,  "is in need of repair."},
    };
    int percent, ar_index;
    const char *objs = OBJS(obj, ch);

    if (GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) > 0)
        percent = (100 * GET_OBJ_VAL(obj, VAL_ALL_HEALTH)) / GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH);
    else
        percent = 0;               /* How could MAX_HIT be < 1?? */

    for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
        if (percent >= diagnosis[ar_index].percent)
            break;

    ch->sendf("\r\n%c%s %s\r\n", UPPER(*objs), objs + 1, diagnosis[ar_index].text);
}

static void diag_char_to_char(struct char_data *i, struct char_data *ch) {
    static struct {
        int percent;
        const char *text;
    } diagnosis[] = {
            {100, "@wis in @Gexcellent@w condition."},
            {90,  "@whas a few @Rscratches@w."},
            {80,  "@whas some small @Rwounds@w and @Rbruises@w."},
            {70,  "@whas quite a few @Rwounds@w."},
            {60,  "@whas some big @rnasty wounds@w and @Rscratches@w."},
            {50,  "@wlooks pretty @rhurt@w."},
            {40,  "@wis mainly @rinjured@w."},
            {30,  "@wis a @rmess@w of @rinjuries@w."},
            {20,  "@wis @rstruggling@w to @msurvive@w."},
            {10,  "@wis in @mawful condition@w."},
            {0,   "@Ris barely alive.@w"},
            {-1,  "@ris nearly dead.@w"},
    };
    int percent, ar_index;

    int64_t hit = GET_HIT(i), max = (i->getEffMaxPL());

    int64_t total = max;

    if (hit == total) {
        percent = 100;
    } else if (hit < total && hit >= (total * .9)) {
        percent = 90;
    } else if (hit < total && hit >= (total * .8)) {
        percent = 80;
    } else if (hit < total && hit >= (total * .7)) {
        percent = 70;
    } else if (hit < total && hit >= (total * .6)) {
        percent = 60;
    } else if (hit < total && hit >= (total * .5)) {
        percent = 50;
    } else if (hit < total && hit >= (total * .4)) {
        percent = 40;
    } else if (hit < total && hit >= (total * .3)) {
        percent = 30;
    } else if (hit < total && hit >= (total * .2)) {
        percent = 20;
    } else if (hit < total && hit >= (total * .1)) {
        percent = 10;
    } else if (hit < total * .1) {
        percent = 0;
    } else {
        percent = -1;        /* How could MAX_HIT be < 1?? */
    }

    for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
        if (percent >= diagnosis[ar_index].percent)
            break;

    ch->sendf("%s\r\n", diagnosis[ar_index].text);
}

static void look_at_char(struct char_data *i, struct char_data *ch) {
    int j, found, clan = false;
    char buf[100];
    struct obj_data *tmp_obj;

    if (!ch->desc) {
        return;
    }
    if (auto ld = i->getLookDesc(); !ld.empty()) {
        ch->sendf("%s", ld);
    }
    if (!MOB_FLAGGED(i, MOB_JUSTDESC)) {
        bringdesc(ch, i);
    }
    ch->sendf("\r\n");
    if (!IS_NPC(i)) {
        if (GET_LIMBCOND(i, 0) >= 50 && !PLR_FLAGGED(i, PLR_CRARM)) {
            ch->sendf("            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(i, 0),
                         "%", "%");
        } else if (GET_LIMBCOND(i, 0) > 0 && !PLR_FLAGGED(i, PLR_CRARM)) {
            ch->sendf("            @D[@cRight Arm   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                         GET_LIMBCOND(i, 0), "%", "%");
        } else if (GET_LIMBCOND(i, 0) > 0 && PLR_FLAGGED(i, PLR_CRARM)) {
            ch->sendf("            @D[@cRight Arm   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                         GET_LIMBCOND(i, 0), "%", "%");
        } else if (GET_LIMBCOND(i, 0) <= 0) {
            ch->sendf("            @D[@cRight Arm   @D: @rMissing.            @D]@n\r\n");
        }
        if (GET_LIMBCOND(i, 1) >= 50 && !PLR_FLAGGED(i, PLR_CLARM)) {
            ch->sendf("            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(i, 1),
                         "%", "%");
        } else if (GET_LIMBCOND(i, 1) > 0 && !PLR_FLAGGED(i, PLR_CLARM)) {
            ch->sendf("            @D[@cLeft Arm    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                         GET_LIMBCOND(i, 1), "%", "%");
        } else if (GET_LIMBCOND(i, 1) > 0 && PLR_FLAGGED(i, PLR_CLARM)) {
            ch->sendf("            @D[@cLeft Arm    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                         GET_LIMBCOND(i, 1), "%", "%");
        } else if (GET_LIMBCOND(i, 1) <= 0) {
            ch->sendf("            @D[@cLeft Arm    @D: @rMissing.            @D]@n\r\n");
        }
        if (GET_LIMBCOND(i, 2) >= 50 && !PLR_FLAGGED(i, PLR_CLARM)) {
            ch->sendf("            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(i, 2),
                         "%", "%");
        } else if (GET_LIMBCOND(i, 2) > 0 && !PLR_FLAGGED(i, PLR_CRLEG)) {
            ch->sendf("            @D[@cRight Leg   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                         GET_LIMBCOND(i, 2), "%", "%");
        } else if (GET_LIMBCOND(i, 2) > 0 && PLR_FLAGGED(i, PLR_CRLEG)) {
            ch->sendf("            @D[@cRight Leg   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                         GET_LIMBCOND(i, 2), "%", "%");
        } else if (GET_LIMBCOND(i, 2) <= 0) {
            ch->sendf("            @D[@cRight Leg   @D: @rMissing.            @D]@n\r\n");
        }
        if (GET_LIMBCOND(i, 3) >= 50 && !PLR_FLAGGED(i, PLR_CLLEG)) {
            ch->sendf("            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(i, 3),
                         "%", "%");
        } else if (GET_LIMBCOND(i, 3) > 0 && !PLR_FLAGGED(i, PLR_CLLEG)) {
            ch->sendf("            @D[@cLeft Leg    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                         GET_LIMBCOND(i, 3), "%", "%");
        } else if (GET_LIMBCOND(i, 3) > 0 && PLR_FLAGGED(i, PLR_CLLEG)) {
            ch->sendf("            @D[@cLeft Leg    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                         GET_LIMBCOND(i, 3), "%", "%");
        } else if (GET_LIMBCOND(i, 3) <= 0) {
            ch->sendf("            @D[@cLeft Leg    @D: @rMissing.             @D]@n\r\n");
        }
        if (PLR_FLAGGED(i, PLR_HEAD)) {
            ch->sendf("            @D[@cHead        @D: @GHas.                 @D]@n\r\n");
        }
        if (!PLR_FLAGGED(i, PLR_HEAD)) {
            ch->sendf("            @D[@cHead        @D: @rMissing.             @D]@n\r\n");
        }
        if (race::hasTail(i->race) && !PLR_FLAGGED(i, PLR_TAILHIDE)) {
            if(PLR_FLAGGED(i, PLR_TAIL))
                ch->sendf("            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
            else
                ch->sendf("            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
        }
    }
    ch->sendf("\r\n");
    if (GET_CLAN(i) != nullptr && strstr(GET_CLAN(i), "None") == false) {
        sprintf(buf, "%s", GET_CLAN(i));
        clan = true;
    }
    if (GET_CLAN(i) == nullptr) {
        clan = false;
    }
    if (!IS_NPC(i)) {
        ch->sendf("            @D[@mClan        @D: @W%-20s@D]@n\r\n", clan ? buf : "None.");
    }
    if (!IS_NPC(i)) {
        ch->sendf("\r\n         @D----------------------------------------@n\r\n");
        trans_check(ch, i);
        ch->sendf("         @D----------------------------------------@n\r\n");
    }
    ch->sendf("\r\n");

    if ((!PLR_FLAGGED(i, PLR_DISGUISED) && (readIntro(ch, i) == 1 && !IS_NPC(i)))) {
        if (GET_SEX(i) == SEX_NEUTRAL)
            ch->sendf("%s appears to be %s %s, ", get_i_name(ch, i), AN(RACE(i)), LRACE(i));
        else
            ch->sendf("%s appears to be %s %s %s, ", get_i_name(ch, i), AN(MAFE(i)), MAFE(i), LRACE(i));
    } else if (ch == i || IS_NPC(i)) {
        if (GET_SEX(i) == SEX_NEUTRAL)
            ch->sendf("%c%s appears to be %s %s, ", UPPER(*GET_NAME(i)), GET_NAME(i) + 1, AN(RACE(i)), LRACE(i));
        else
            ch->sendf("%c%s appears to be %s %s %s, ", UPPER(*GET_NAME(i)), GET_NAME(i) + 1, AN(MAFE(i)),
                         MAFE(i), LRACE(i));
    } else {
        if (GET_SEX(i) == SEX_NEUTRAL)
            ch->sendf("Appears to be %s %s, ", AN(RACE(i)), LRACE(i));
        else
            ch->sendf("Appears to be %s %s %s, ", AN(MAFE(i)), MAFE(i), LRACE(i));
    }

    if (IS_NPC(i)) {
        ch->sendf("is %s sized, and\r\n", size_names[get_size(i)]);
    }
    if (!IS_NPC(i)) {
        auto w = i->getWeight();
        int h = i->getHeight();
        auto wString = fmt::format("{}kg", w);
        ch->sendf("is %s sized, about %dcm tall,\r\nabout %s heavy,", size_names[get_size(i)],
                     h, wString.c_str());

        if (i == ch) {
            ch->sendf(" and ");
        } else if (GET_AGE(ch) >= GET_AGE(i) + 30) {
            ch->sendf(" appears to be very much younger than you, and ");
        } else if (GET_AGE(ch) >= GET_AGE(i) + 25) {
            ch->sendf(" appears to be much younger than you, and ");
        } else if (GET_AGE(ch) >= GET_AGE(i) + 15) {
            ch->sendf(" appears to be a good amount younger than you, and ");
        } else if (GET_AGE(ch) >= GET_AGE(i) + 10) {
            ch->sendf(" appears to be about a decade younger than you, and ");
        } else if (GET_AGE(ch) >= GET_AGE(i) + 5) {
            ch->sendf(" appears to be several years younger than you, and ");
        } else if (GET_AGE(ch) >= GET_AGE(i) + 2) {
            ch->sendf(" appears to be a bit younger than you, and ");
        } else if (GET_AGE(ch) > GET_AGE(i)) {
            ch->sendf(" appears to be slightly younger than you, and ");
        } else if (GET_AGE(ch) == GET_AGE(i)) {
            ch->sendf(" appears to be the same age as you, and ");
        }
        if (GET_AGE(i) >= GET_AGE(ch) + 30) {
            ch->sendf(" appears to be very much older than you, and ");
        } else if (GET_AGE(i) >= GET_AGE(ch) + 25) {
            ch->sendf(" appears to be much older than you, and ");
        } else if (GET_AGE(i) >= GET_AGE(ch) + 15) {
            ch->sendf(" appears to be a good amount older than you, and ");
        } else if (GET_AGE(i) >= GET_AGE(ch) + 10) {
            ch->sendf(" appears to be about a decade older than you, and ");
        } else if (GET_AGE(i) >= GET_AGE(ch) + 5) {
            ch->sendf(" appears to be several years older than you, and ");
        } else if (GET_AGE(i) >= GET_AGE(ch) + 2) {
            ch->sendf(" appears to be a bit older than you, and ");
        } else if (GET_AGE(i) > GET_AGE(ch)) {
            ch->sendf(" appears to be slightly older than you, and ");
        }
    }
    diag_char_to_char(i, ch);
    found = false;
    for (j = 0; !found && j < NUM_WEARS; j++)
        if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
            found = true;

    if (found && (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOEQSEE))) {
        ch->sendf("\r\n");    /* act() does capitalization. */
        if (!PLR_FLAGGED(i, PLR_DISGUISED)) {
            act("$n is using:", false, i, nullptr, ch, TO_VICT);
        } else {
            act("The disguised person is using:", false, i, nullptr, ch, TO_VICT);
        }

        for (j = 0; j < NUM_WEARS; j++) {
            auto eq = GET_EQ(i, j);
            if(!eq) continue;
            if(!CAN_SEE_OBJ(ch, eq)) continue;
                
            if ((j != WEAR_WIELD1 && j != WEAR_WIELD2) || (!PLR_FLAGGED(i, PLR_THANDW))) {
                ch->sendf("%s", wear_where[j]);
                show_obj_to_char(eq, ch, SHOW_OBJ_SHORT);
                if (OBJ_FLAGGED(eq, ITEM_SHEATH)) {
                    for (auto obj2 : eq->getInventory()) {
                        ch->sendf("@D  ---- @YSheathed@D ----@c> @n");
                        show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
                    }
                }
            } else if (PLR_FLAGGED(i, PLR_THANDW)) {
                ch->sendf("@c<@CWielded by B. Hands@c>@n ");
                show_obj_to_char(eq, ch, SHOW_OBJ_SHORT);
            }
        }
    }
    if (ch != i && ((GET_SKILL(ch, SKILL_KEEN) && AFF_FLAGGED(ch, AFF_SNEAK)) || GET_ADMLEVEL(ch))) {
        found = false;
        act("\r\nYou attempt to peek at $s inventory:", false, i, nullptr, ch, TO_VICT);
        if (CAN_SEE(i, ch))
            act("$n tries to evaluate what you have in your inventory.", true, ch, nullptr, i, TO_VICT);
        if (GET_SKILL(ch, SKILL_KEEN) > axion_dice(0) && (!IS_NPC(i) || GET_ADMLEVEL(ch) > 1)) {
            for (auto tmp_obj : i->getInventory()) {
                if (CAN_SEE_OBJ(ch, tmp_obj) &&
                    (ADM_FLAGGED(ch, ADM_SEEINV) || (rand_number(0, 20) < GET_LEVEL(ch)))) {
                    show_obj_to_char(tmp_obj, ch, SHOW_OBJ_SHORT);
                    found = true;
                }
            }
            improve_skill(ch, SKILL_KEEN, 1);
        } else if (IS_NPC(i) && GET_ADMLEVEL(ch) < 2) {
            return;
        } else {
            act("You are unsure about $s inventory.", false, i, nullptr, ch, TO_VICT);
            if (CAN_SEE(i, ch))
                act("$n didn't seem to get a good enough look.", true, ch, nullptr, i, TO_VICT);
            improve_skill(ch, SKILL_KEEN, 1);
            return;
        }
        if (!found) {
            ch->sendf("You can't see anything.\r\n");
            improve_skill(ch, SKILL_KEEN, 1);
        }
    }
}

static void list_one_char(struct char_data *i, struct char_data *ch) {
    struct obj_data *chair = nullptr;
    int count = false;
    const char *positions[] = {
            " is dead",
            " is mortally wounded",
            " is lying here, incapacitated",
            " is lying here, stunned",
            " is sleeping here",
            " is resting here",
            " is sitting here",
            "!FIGHTING!",
            " is standing here"
    };

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS) && IS_NPC(i)) {
        ch->sendf("@D[@G%d@D]@w %s", GET_MOB_VNUM(i), i->scriptString().c_str());
    }

    if (IS_NPC(i) && i->room_description && GET_POS(i) == GET_DEFAULT_POS(i) && !FIGHTING(i)) {
        ch->sendf("%s", i->room_description);

        auto icur = GET_HIT(i);
        auto imax = i->getEffMaxPL();

        std::vector<std::string> messages;

        if (icur >= (imax) * .9 && icur != (imax))
            messages.emplace_back("@R...Some slight wounds on $s body.@w");
        else if (icur >= (imax) * .8 && icur < (imax) * .9)
            messages.emplace_back("@R...A few wounds on $s body.@w");
        else if (icur >= (imax) * .7 && icur < (imax) * .8)
            messages.emplace_back("@R...Many wounds on $s body.@w");
        else if (icur >= (imax) * .6 && icur < (imax) * .7)
            messages.emplace_back("@R...Quite a few wounds on $s body.@w");
        else if (icur >= (imax) * .5 && icur < (imax) * .6)
            messages.emplace_back("@R...Horrible wounds on $s body.@w");
        else if (icur >= (imax) * .4 && icur < (imax) * .5)
            messages.emplace_back("@R...Blood is seeping from the wounds on $s body.@w");
        else if (icur >= (imax) * .3 && icur < (imax) * .4)
            messages.emplace_back("@R...$s body is in terrible shape.@w");
        else if (icur >= (imax) * .2 && icur < (imax) * .3)
            messages.emplace_back("@R...Is absolutely covered in wounds.@w");
        else if (icur >= (imax) * .1 && icur < (imax) * .2)
            messages.emplace_back("@R...Is on $s last leg.@w");
        else if (icur < (imax) * .1)
            messages.emplace_back("@R...Should be DEAD soon.@w");

        if (GET_EAVESDROP(i) > 0) {
            messages.emplace_back(fmt::format("@w...$e is spying on everything to the @c%s@w.", dirs[GET_EAVESDIR(i)]));
        }
        if (AFF_FLAGGED(i, AFF_FLYING) && GET_ALT(i) == 1)
            messages.emplace_back("...$e is in the air!");
        if (AFF_FLAGGED(i, AFF_FLYING) && GET_ALT(i) == 2)
            messages.emplace_back("...$e is high in the air!");
        if (AFF_FLAGGED(i, AFF_SANCTUARY) && !GET_SKILL(i, SKILL_AQUA_BARRIER))
            messages.emplace_back("...$e has a barrier around $s body!");
        if (AFF_FLAGGED(i, AFF_FIRESHIELD))
            messages.emplace_back("...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!");
        if (AFF_FLAGGED(i, AFF_SANCTUARY) && GET_SKILL(i, SKILL_AQUA_BARRIER))
            messages.emplace_back("...$e has a @Gbarrier@w of @cwater@w and @Cki@w around $s body!");
        if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_SPIRAL))
            messages.emplace_back("...$e is spinning in a vortex!");
        if (GET_CHARGE(i))
            messages.emplace_back("...$e has a bright %s aura around $s body!");
        if (AFF_FLAGGED(i, AFF_METAMORPH))
            messages.emplace_back("@w...$e has a dark, @rred@w aura and menacing presence.");
        if (AFF_FLAGGED(i, AFF_HAYASA))
            messages.emplace_back("@w...$e has a soft @cblue@w glow around $s body!");
        if (AFF_FLAGGED(i, AFF_BLIND))
            messages.emplace_back("...$e is groping around blindly!");
        if (affected_by_spell(i, SPELL_FAERIE_FIRE))
            messages.emplace_back("@m...$e @mis outlined with purple fire!@m");
        if (GET_FEATURE(i)) {
            messages.emplace_back(fmt::format("@C%s@n", GET_FEATURE(i)));
        }

        for(const auto& msg : messages) act(msg.c_str(), false, i, nullptr, ch, TO_VICT);

        return;
    }

    auto shd = i->getShortDesc();

    if (IS_NPC(i) && !FIGHTING(i) && GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING)
        ch->sendf("@w%s", shd);
    else if (IS_NPC(i) && GRAPPLED(i) && GRAPPLED(i) == ch)
        ch->sendf("@w%s is being grappled with by YOU!", shd);
    else if (IS_NPC(i) && GRAPPLED(i) && GRAPPLED(i) != ch)
        ch->sendf("@w%s is being absorbed from by %s!", shd,
                     readIntro(ch, GRAPPLED(i)) == 1 ? get_i_name(ch, GRAPPLED(i)) : AN(RACE(GRAPPLED(i))));
    else if (IS_NPC(i) && ABSORBBY(i) && ABSORBBY(i) == ch)
        ch->sendf("@w%s is being absorbed from by YOU!", shd);
    else if (IS_NPC(i) && ABSORBBY(i) && ABSORBBY(i) != ch)
        ch->sendf("@w%s is being absorbed from by %s!", shd,
                     readIntro(ch, ABSORBBY(i)) == 1 ? get_i_name(ch, ABSORBBY(i)) : AN(RACE(ABSORBBY(i))));
    else if (IS_NPC(i) && FIGHTING(i) && FIGHTING(i) != ch && GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING &&
             is_sparring(i))
        ch->sendf("@w%c%s is sparring with %s!", shd,
                     GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i)) : (readIntro(ch, FIGHTING(i)) == 1 ? get_i_name(ch,
                                                                                                              FIGHTING(
                                                                                                                      i))
                                                                                                 : LRACE(FIGHTING(i))));
    else if (IS_NPC(i) && FIGHTING(i) && is_sparring(i) && FIGHTING(i) == ch && GET_POS(i) != POS_SITTING &&
             GET_POS(i) != POS_SLEEPING)
        ch->sendf("@w%s is sparring with you!", shd);
    else if (IS_NPC(i) && FIGHTING(i) && FIGHTING(i) != ch && GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING)
        ch->sendf("@w%s is fighting %s!", shd,
                     GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i)) : (readIntro(ch, FIGHTING(i)) == 1 ? get_i_name(ch,
                                                                                                              FIGHTING(
                                                                                                                      i))
                                                                                                 : LRACE(FIGHTING(i))));
    else if (IS_NPC(i) && FIGHTING(i) && FIGHTING(i) == ch && GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING)
        ch->sendf("@w%s is fighting YOU!", shd);
    else if (IS_NPC(i) && FIGHTING(i) && GET_POS(i) == POS_SITTING)
        ch->sendf("@w%s is sitting here.", shd);
    else if (IS_NPC(i) && FIGHTING(i) && GET_POS(i) == POS_SLEEPING)
        ch->sendf("@w%s is sleeping here.", shd);
    else if (IS_NPC(i))
        ch->sendf("@w%s", shd);
    else if (!IS_NPC(i)) {
        if (IS_MAJIN(i) && AFF_FLAGGED(i, AFF_LIQUEFIED)) {
            ch->sendf("@wSeveral blobs of %s colored goo spread out here.@n\n", skin_types[(int) GET_SKIN(i)]);
            return;
        }
        if ((GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(i) > 0) || IS_NPC(ch)) {
            ch->sendf("@w%s", i->name);
        } else if ((!PLR_FLAGGED(i, PLR_DISGUISED) && readIntro(ch, i) == 1)) {
            ch->sendf("@w%s", get_i_name(ch, i));
        } else if (!PLR_FLAGGED(i, PLR_DISGUISED) && readIntro(ch, i) != 1) {
            if (GET_DISTFEA(i) == DISTFEA_EYE) {
                ch->sendf("@wA %s eyed %s %s", eye_types[(int) GET_EYE(i)], MAFE(i), LRACE(i));
            } else if (GET_DISTFEA(i) == DISTFEA_HAIR) {
                if (IS_MAJIN(i)) {
                    ch->sendf("@wA %s majin, with a %s forelock,", MAFE(i), FHA_types[(int) GET_HAIRL(i)]);
                } else if (IS_NAMEK(i)) {
                    ch->sendf("@wA namek, with %s antennae,", FHA_types[(int) GET_HAIRL(i)]);
                } else if (IS_ARLIAN(i)) {
                    ch->sendf("@wA arlian, with %s antennae,", FHA_types[(int) GET_HAIRL(i)]);
                } else if (IS_ICER(i) || IS_DEMON(i)) {
                    ch->sendf("@wA %s %s, with %s horns", MAFE(i), LRACE(i), FHA_types[(int) GET_HAIRL(i)]);
                } else {
                    char blarg[MAX_INPUT_LENGTH];
                    sprintf(blarg, "%s %s hair %s", hairl_types[(int) GET_HAIRL(i)], hairc_types[(int) GET_HAIRC(i)],
                            hairs_types[(int) GET_HAIRS(i)]);
                    ch->sendf("@wA %s %s, with %s", MAFE(i), LRACE(i),
                                 GET_HAIRL(i) == 0 ? "a bald head" : (blarg));
                }
            } else if (GET_DISTFEA(i) == DISTFEA_SKIN) {
                ch->sendf("@wA %s skinned %s %s", skin_types[(int) GET_SKIN(i)], MAFE(i), LRACE(i));
            } else if (GET_DISTFEA(i) == DISTFEA_HEIGHT) {
                char *height;
                if (IS_TRUFFLE(i)) {
                    if (GET_PC_HEIGHT(i) > 70) {
                        height = strdup("very tall");
                    } else if (GET_PC_HEIGHT(i) > 55) {
                        height = strdup("tall");
                    } else if (GET_PC_HEIGHT(i) > 35) {
                        height = strdup("average height");
                    } else {
                        height = strdup("short");
                    }
                } else {
                    if (GET_PC_HEIGHT(i) > 200) {
                        height = strdup("very tall");
                    } else if (GET_PC_HEIGHT(i) > 180) {
                        height = strdup("tall");
                    } else if (GET_PC_HEIGHT(i) > 150) {
                        height = strdup("average height");
                    } else if (GET_PC_HEIGHT(i) > 120) {
                        height = strdup("short");
                    } else {
                        height = strdup("very short");
                    }
                }
                ch->sendf("@wA %s %s %s", height, MAFE(i), LRACE(i));
                if (height) {
                    free(height);
                }
            } else if (GET_DISTFEA(i) == DISTFEA_WEIGHT) {
                char *height;
                auto w = i->getWeight();
                if (IS_TRUFFLE(i)) {
                    if (w > 35) {
                        height = strdup("very heavy");
                    } else if (w > 25) {
                        height = strdup("heavy");
                    } else if (w > 15) {
                        height = strdup("average weight");
                    } else {
                        height = strdup("welterweight");
                    }
                } else {
                    if (w > 120) {
                        height = strdup("very heavy");
                    } else if (w > 100) {
                        height = strdup("heavy");
                    } else if (w > 80) {
                        height = strdup("average weight");
                    } else if (w > 60) {
                        height = strdup("lightweight");
                    } else {
                        height = strdup("welterweight");
                    }
                }
                ch->sendf("@wA %s %s %s", height, MAFE(i), LRACE(i));
                if (height) {
                    free(height);
                }
            }
        } else {
            ch->sendf("@wA disguised %s %s", MAFE(i), LRACE(i));
        }
    }

    if (!IS_NPC(i) || !FIGHTING(i)) {
        if (AFF_FLAGGED(i, AFF_INVISIBLE)) {
            ch->sendf(", is invisible");
            count = true;
        }
        if (AFF_FLAGGED(i, AFF_ETHEREAL)) {
            ch->sendf(", has a halo");
            count = true;
        }
        if (AFF_FLAGGED(i, AFF_HIDE) && i != ch) {
            ch->sendf(", is hiding");
            if (GET_SKILL(i, SKILL_HIDE) && !IS_NPC(ch) && i != ch) {
                improve_skill(i, SKILL_HIDE, 1);
            }
            count = true;
        }
        if (!IS_NPC(i) && !i->desc) {
            ch->sendf(", has a blank stare");
            count = true;
        }
        if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING)) {
            ch->sendf(", is writing");
            count = true;
        }
        if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK)) {
            ch->sendf(", is buildwalking");
            count = true;
        }
        if (!IS_NPC(i) && ABSORBING(i) && ABSORBING(i) != ch) {
            ch->sendf(", is absorbing from %s", GET_NAME(ABSORBING(i)));
            count = true;
        }
        if (!IS_NPC(i) && GRAPPLING(i) && GRAPPLING(i) != ch) {
            ch->sendf(", is grappling with %s",
                         readIntro(ch, GRAPPLING(i)) == 1 ? get_i_name(ch, GRAPPLING(i)) : introd_calc(GRAPPLING(i)));
            count = true;
        }
        if (!IS_NPC(i) && CARRYING(i) && CARRYING(i) != ch) {
            ch->sendf(", is carrying %s",
                         readIntro(ch, CARRYING(i)) == 1 ? get_i_name(ch, CARRYING(i)) : introd_calc(CARRYING(i)));
            count = true;
        }
        if (!IS_NPC(i) && CARRIED_BY(i) && CARRIED_BY(i) != ch) {
            ch->sendf(", is being carried by %s",
                         readIntro(ch, CARRIED_BY(i)) == 1 ? get_i_name(ch, CARRIED_BY(i)) : introd_calc(
                                 CARRIED_BY(i)));
            count = true;
        }
        if (!IS_NPC(i) && GRAPPLING(i) && GRAPPLING(i) == ch) {
            ch->sendf(", is grappling with YOU");
            count = true;
        }
        if (!IS_NPC(i) && ABSORBING(i) && ABSORBING(i) == ch) {
            ch->sendf(", is absorbing from YOU");
            count = true;
        }
        if (!IS_NPC(i) && ABSORBING(ch) && ABSORBING(ch) == i) {
            ch->sendf(", is being absorbed from by YOU");
            count = true;
        }
        if (!IS_NPC(i) && GRAPPLING(ch) && GRAPPLING(ch) == i) {
            ch->sendf(", is being grappled with by YOU");
            count = true;
        }
        if (!IS_NPC(i) && CARRYING(ch) && CARRYING(ch) == i) {
            ch->sendf(", is being carried by you");
            count = true;
        }
        if (!IS_NPC(ch) && !IS_NPC(i) && FIGHTING(i)) {
            if (!PLR_FLAGGED(i, PLR_SPAR) ||
                (PLR_FLAGGED(i, PLR_SPAR) && (!PLR_FLAGGED(FIGHTING(i), PLR_SPAR) || IS_NPC(FIGHTING(i))))) {
                ch->sendf(", is here fighting ");
            }
            if (PLR_FLAGGED(i, PLR_SPAR) && PLR_FLAGGED(FIGHTING(i), PLR_SPAR)) {
                ch->sendf(", is here sparring ");
            }
            if (FIGHTING(i) == ch) {
                ch->sendf("@rYOU@w");
                count = true;
            } else {
                if (IN_ROOM(i) == IN_ROOM(FIGHTING(i))) {
                    ch->sendf("%s", GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i)) : (readIntro(ch, FIGHTING(i)) == 1
                                                                                       ? get_i_name(ch, FIGHTING(i))
                                                                                       : LRACE(FIGHTING(i))));
                    count = true;
                } else {
                    ch->sendf("someone who has already left!");
                }
            }
        }
    }
    if (SITS(i)) {
        chair = SITS(i);
        if (PLR_FLAGGED(i, PLR_HEALT)) {
            ch->sendf("@w is floating inside a healing tank.");
        } else if (count == true) {
            ch->sendf(",@w and%s on %s.", positions[(int) GET_POS(i)], chair->getShortDesc());
        } else if (count == false) {
            ch->sendf("@w%s on %s.", positions[(int) GET_POS(i)], chair->getShortDesc());
        }
    } else if (!PLR_FLAGGED(i, PLR_PILOTING) && !SITS(i) && (!IS_NPC(i) || !FIGHTING(i))) {
        if (count == true) {
            ch->sendf("@w, and%s.", positions[(int) GET_POS(i)]);
        }
        if (count == false) {
            ch->sendf("@w%s.", positions[(int) GET_POS(i)]);
        }
    } else if (PLR_FLAGGED(i, PLR_PILOTING)) {
        ch->sendf("@w, is sitting in the pilot's chair.\r\n");
    } else {

        if (FIGHTING(i) && !IS_NPC(ch) && !IS_NPC(i)) {
            if (!PLR_FLAGGED(i, PLR_SPAR)) {
                ch->sendf(", is here fighting ");
            }
            if (PLR_FLAGGED(i, PLR_SPAR)) {
                ch->sendf(", is here sparring ");
            }
            if (FIGHTING(i) == ch)
                ch->sendf("@rYOU@w!");
            else {
                if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
                    ch->sendf("%s!", GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i)) : (readIntro(ch, FIGHTING(i)) == 1
                                                                                        ? get_i_name(ch, FIGHTING(i))
                                                                                        : LRACE(FIGHTING(i))));
                else
                    ch->sendf("someone who has already left!");
            }
        } else if (!IS_NPC(i)) {            /* NIL fighting pointer */
            ch->sendf(" is here struggling with thin air.");
        }
    }


    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
        if (IS_EVIL(i))
            ch->sendf(" (@rRed@[3] Aura)");
        else if (IS_GOOD(i))
            ch->sendf(" (@bBlue@[3] Aura)");
    }
    if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_AFK))
        ch->sendf(" @D(@RAFK@D)");
    else if (!IS_NPC(i) && i->timer > 3)
        ch->sendf(" @D(@RIDLE@D)");
    ch->sendf("@n\r\n");

    std::vector<std::string> messages;

    if (GET_EAVESDROP(i) > 0) {
        messages.emplace_back(fmt::format("@w...$e is spying on everything to the @c%s@w.", dirs[GET_EAVESDIR(i)]));
    }
    if (PLR_FLAGGED(i, PLR_FISHING)) {
        messages.emplace_back("@w...$e is @Cfishing@w.@n");
    }
    if (PLR_FLAGGED(i, PLR_AURALIGHT)) {
        messages.emplace_back(fmt::format("...is surrounded by a bright %s aura.", aura_types[GET_AURA(i)]));
    }

    auto is_oozaru = (i->form == FormID::Oozaru || i->form == FormID::GoldenOozaru);

    if (AFF_FLAGGED(i, AFF_SANCTUARY) && !GET_SKILL(i, SKILL_AQUA_BARRIER))
        messages.emplace_back("@w...$e has a @bbarrier@w around $s body!");
    if (AFF_FLAGGED(i, AFF_FIRESHIELD))
        messages.emplace_back("@w...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!");
    if (AFF_FLAGGED(i, AFF_HEALGLOW))
        messages.emplace_back("@w...$e has a serene @Cblue@Y glow@w around $s body.");
    if (AFF_FLAGGED(i, AFF_EARMOR))
        messages.emplace_back("@w...$e has ghostly @Ggreen@w ethereal armor around $s body.");
    if (AFF_FLAGGED(i, AFF_SANCTUARY) && GET_SKILL(i, SKILL_AQUA_BARRIER))
        messages.emplace_back("@w...$e has a @bbarrier@w of @cwater@w and @CKi@w around $s body!");
    if (AFF_FLAGGED(i, AFF_FLYING) && GET_ALT(i) == 1)
        messages.emplace_back("@w...$e is in the air!");
    if (AFF_FLAGGED(i, AFF_FLYING) && GET_ALT(i) == 2)
        messages.emplace_back("@w...$e is high in the air!");
    if (GET_KAIOKEN(i) > 0)
        messages.emplace_back("@w...@r$e has a red aura around $s body!");
    if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_SPIRAL))
        messages.emplace_back("@w...$e is spinning in a vortex!");
    if (IS_TRANSFORMED(i) && !IS_ANDROID(i) && !IS_SAIYAN(i) && !IS_HALFBREED(i))
        messages.emplace_back("@w...$e has energy crackling around $s body!");
    if (GET_CHARGE(i) && !IS_SAIYAN(i) && !IS_HALFBREED(i)) {
        messages.emplace_back(fmt::format("@w...$e has a bright %s aura around $s body!", aura_types[GET_AURA(i)]));
    }
    if (!is_oozaru && GET_CHARGE(i) && IS_TRANSFORMED(i) && (IS_SAIYAN(i) || IS_HALFBREED(i)))
        messages.emplace_back("@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!");
    if (!is_oozaru && GET_CHARGE(i) && !IS_TRANSFORMED(i) && (IS_SAIYAN(i) || IS_HALFBREED(i))) {
        messages.emplace_back(fmt::format("@w...$e has a @Ybright@w %s aura around $s body!", aura_types[GET_AURA(i)]));
    }
    if (i->form != FormID::Oozaru && !GET_CHARGE(i) && IS_TRANSFORMED(i) && (IS_SAIYAN(i) || IS_HALFBREED(i)))
        messages.emplace_back("@w...$e has energy crackling around $s body!");
    if (i->form == FormID::Oozaru && GET_CHARGE(i) && (IS_SAIYAN(i) || IS_HALFBREED(i)))
        messages.emplace_back("@w...$e is in the form of a @rgreat ape@w!");
    if (AFF_FLAGGED(ch, AFF_KYODAIKA))
        messages.emplace_back("@w...$e has expanded $s body size@w!");
    if (AFF_FLAGGED(i, AFF_HAYASA))
        messages.emplace_back("@w...$e has a soft @cblue@w glow around $s body!");
    if (i->form == FormID::Oozaru && !GET_CHARGE(i) && (IS_SAIYAN(i) || IS_HALFBREED(i)))
        messages.emplace_back("@w...$e has energy crackling around $s @rgreat ape@w body!");
    if (GET_FEATURE(i)) {
        messages.emplace_back(fmt::format("@C%s@n", GET_FEATURE(i)));
    }

    if (GET_RDISPLAY(i)) {
        if (GET_RDISPLAY(i) != "Empty") {
            messages.emplace_back(fmt::format("...%s", GET_RDISPLAY(i)));
        }
    }
    for(const auto& msg : messages) act(msg.c_str(), false, i, nullptr, ch, TO_VICT);

}

static void list_char_to_char(std::vector<char_data*> people, struct char_data *ch) {
    struct char_data *i, *j;
    struct hide_node {
        struct hide_node *next;
        struct char_data *hidden;
    } *hideinfo, *lasthide, *tmphide;
    int num;

    hideinfo = lasthide = nullptr;

    for (auto i : people) {
        if (AFF_FLAGGED(i, AFF_HIDE) && roll_resisted(i, SKILL_HIDE, ch, SKILL_SPOT)) {
            if (GET_SKILL(i, SKILL_HIDE) && !IS_NPC(ch) && i != ch) {
                improve_skill(i, SKILL_HIDE, 1);
            }
            CREATE(tmphide, struct hide_node, 1);
            tmphide->next = nullptr;
            tmphide->hidden = i;
            if (!lasthide) {
                hideinfo = lasthide = tmphide;
            } else {
                lasthide->next = tmphide;
                lasthide = tmphide;
            }
            continue;
        }
    }

    for (auto i : people) {
        /* hide npcs whose description starts with a '.' from non-holylighted people
    - Idea from Elaseth of TBA */
        if ((ch == i) || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) && IS_NPC(i)
                          && i->room_description && *i->room_description == '.'))
            continue;

        for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
            if (tmphide->hidden == i)
                break;
        if (tmphide)
            continue;

        if (CAN_SEE(ch, i)) {
            num = 0;
            /* Now show this mob's name and other stuff */
            ch->sendf("@w");
            if (num > 1)
                ch->sendf("@D(@Rx@Y%2i@D)@n ", num);
            list_one_char(i, ch);
            ch->sendf("@n");
        } /* processed a character we can see */
        else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) &&
                 AFF_FLAGGED(i, AFF_INFRAVISION))
            ch->sendf("@wYou see a pair of glowing red eyes looking your way.@n\r\n");
    } /* loop through all characters in room */
}

static void do_auto_exits(struct room_data *room, struct char_data *ch, int exit_mode) {
    int door, door_found = 0, has_light = false, i;
    char dlist1[500];
    char dlist2[500];
    char dlist3[500];
    char dlist4[500];
    char dlist5[500];
    char dlist6[500];
    char dlist7[500];
    char dlist8[500];
    char dlist9[500];
    char dlist10[500];
    char dlist11[500];
    char dlist12[500];

    *dlist1 = '\0';
    *dlist2 = '\0';
    *dlist3 = '\0';
    *dlist4 = '\0';
    *dlist5 = '\0';
    *dlist6 = '\0';
    *dlist7 = '\0';
    *dlist8 = '\0';
    *dlist9 = '\0';
    *dlist10 = '\0';
    *dlist11 = '\0';
    *dlist12 = '\0';

    if (exit_mode == EXIT_OFF) {
        ch->sendf("@D------------------------------------------------------------------------@n\r\n");
    }
    int space = false;
    if (room->sector_type == SECT_SPACE && room->vn >= 20000) {
        space = true;
    }
    if (exit_mode == EXIT_NORMAL && space == false && IN_ROOM(ch) == room->vn) {
        /* Compass and Auto-map - Iovan 9-11-10 */
        ch->sendf("@D------------------------------------------------------------------------@n\r\n");
        ch->sendf("@w      Compass           Auto-Map            Map Key\r\n");
        ch->sendf("@R     ---------         ----------   -----------------------------\r\n");
        gen_map(ch, 0);
        ch->sendf("@D------------------------------------------------------------------------@n\r\n");
    }
    if (exit_mode == EXIT_NORMAL && space == true) {
        /* printmap */
        ch->sendf("@D------------------------------[@CRadar@D]---------------------------------@n\r\n");
        printmap(room->vn, ch, 1, -1);
        ch->sendf("     @D[@wTurn autoexit complete on for directions instead of radar@D]@n\r\n");
        ch->sendf("@D------------------------------------------------------------------------@n\r\n");
    }
    if (exit_mode == EXIT_COMPLETE || (exit_mode == EXIT_NORMAL && space == false && IN_ROOM(ch) != room->vn)) {
        ch->sendf("@D----------------------------[@gObvious Exits@D]-----------------------------@n\r\n");
        if (IS_AFFECTED(ch, AFF_BLIND)) {
            ch->sendf("You can't see a damned thing, you're blind!\r\n");
            return;
        }
        if (PLR_FLAGGED(ch, PLR_EYEC)) {
            ch->sendf("You can't see a damned thing, your eyes are closed!\r\n");
            return;
        }
        if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) && !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
            ch->sendf("It is pitch black...\r\n");
            return;
        }

        /* Is the character using a working light source? */
        has_light = ch->isProvidingLight();

        bool admVision = ADM_FLAGGED(ch, ADM_SEESECRET) || GET_ADMLEVEL(ch) > 4;

        std::map<int, char*> dlists = {
                {0, dlist2},
                {1, dlist4},
                {2, dlist6},
                {3, dlist8},
                {4, dlist9},
                {5, dlist10},
                {6, dlist1},
                {7, dlist3},
                {8, dlist5},
                {9, dlist7},
                {10, dlist11},
                {11, dlist12}
        };

        for (auto &[door, d] : room->getExits()) {
            auto dest = d->getDestination();
            if(!dest) continue;
            auto dl = dlists[door];

            auto al = d->getAlias();

            if (admVision) {
                /* Immortals see everything */
                door_found++;
                char blam[9];
                sprintf(blam, "%s", dirs[door]);
                *blam = toupper(*blam);

                auto dirname = dirs[door];
                auto rdirname = dirs[rev_dir[door]];


                sprintf(dl, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, dest->vn, dest->name);
                if (d->checkFlag(FlagType::Exit, EX_ISDOOR) || d->checkFlag(FlagType::Exit, EX_SECRET)) {
                    /* This exit has a door - tell all about it */
                    char argh[100];
                    sprintf(argh, "%s ",
                            strcasecmp(fname(al.c_str()), "undefined") ? fname(
                                    al.c_str()) : "opening");
                    sprintf(dl + strlen(dl), "                    The %s%s %s %s %s%s.\r\n",
                            d->checkFlag(FlagType::Exit, EX_SECRET) ?
                            "@rsecret@w " : "",
                            (al.c_str() && strcasecmp(fname(al.c_str()), "undefined")) ?
                            fname(al.c_str()) : "opening",
                            strstr(argh, "s ") != nullptr ? "are" : "is",
                            d->checkFlag(FlagType::Exit, EX_CLOSED) ?
                            "closed" : "open",
                            d->checkFlag(FlagType::Exit, EX_LOCKED) ?
                            "and locked" : "and unlocked",
                            d->checkFlag(FlagType::Exit, EX_PICKPROOF) ?
                            " (pickproof)" : "");
                }
            }
            else { /* This is what mortal characters see */
                if (!d->checkFlag(FlagType::Exit, EX_CLOSED)) {
                    /* And the door is open */
                    door_found++;
                    char blam[9];
                    sprintf(blam, "%s", dirs[door]);
                    *blam = toupper(*blam);

                    sprintf(dl, "@c%-9s @D-@w %s\r\n", blam,
                            IS_DARK(dest->vn) && !CAN_SEE_IN_DARK(ch) && !has_light
                            ? "@bToo dark to tell.@w" : dest->name);

                } else if (CONFIG_DISP_CLOSED_DOORS && !d->checkFlag(FlagType::Exit, EX_SECRET)) {
                    /* But we tell them the door is closed */
                    door_found++;
                    char blam[9];
                    sprintf(blam, "%s", dirs[door]);
                    *blam = toupper(*blam);
                    if (door == 6) {

                    }
                    sprintf(dl, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam,
                            (al.c_str()) ? fname(al.c_str())
                                         : "opening");
                }
            }
        }

        if (!door_found)
            ch->sendf(" None.\r\n");
        if (strstr(dlist1, "Northwest")) {
            ch->sendf("%s", dlist1);
            *dlist1 = '\0';
        }
        if (strstr(dlist2, "North")) {
            ch->sendf("%s", dlist2);
            *dlist2 = '\0';
        }
        if (strstr(dlist3, "Northeast")) {
            ch->sendf("%s", dlist3);
            *dlist3 = '\0';
        }
        if (strstr(dlist4, "East")) {
            ch->sendf("%s", dlist4);
            *dlist4 = '\0';
        }
        if (strstr(dlist5, "Southeast")) {
            ch->sendf("%s", dlist5);
            *dlist5 = '\0';
        }
        if (strstr(dlist6, "South")) {
            ch->sendf("%s", dlist6);
            *dlist6 = '\0';
        }
        if (strstr(dlist7, "Southwest")) {
            ch->sendf("%s", dlist7);
            *dlist7 = '\0';
        }
        if (strstr(dlist8, "West")) {
            ch->sendf("%s", dlist8);
            *dlist8 = '\0';
        }
        if (strstr(dlist9, "Up")) {
            ch->sendf("%s", dlist9);
            *dlist9 = '\0';
        }
        if (strstr(dlist10, "Down")) {
            ch->sendf("%s", dlist10);
            *dlist10 = '\0';
        }
        if (strstr(dlist11, "Inside")) {
            ch->sendf("%s", dlist11);
            *dlist11 = '\0';
        }
        if (strstr(dlist12, "Outside")) {
            ch->sendf("%s", dlist12);
            *dlist12 = '\0';
        }
        
        ch->sendf("@D------------------------------------------------------------------------@n\r\n");
        if (room->checkFlag(FlagType::Room, ROOM_HOUSE) && !room->checkFlag(FlagType::Room, ROOM_GARDEN1) &&
            !room->checkFlag(FlagType::Room, ROOM_GARDEN2)) {
            ch->sendf("@D[@GItems Stored@D: @g%d@D]@n\r\n", check_saveroom_count(ch, nullptr));
        }
        if (room->checkFlag(FlagType::Room, ROOM_HOUSE) && room->checkFlag(FlagType::Room, ROOM_GARDEN1) &&
            !room->checkFlag(FlagType::Room, ROOM_GARDEN2)) {
            ch->sendf("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", check_saveroom_count(ch, nullptr));
        }
        if (room->checkFlag(FlagType::Room, ROOM_HOUSE) && !room->checkFlag(FlagType::Room, ROOM_GARDEN1) &&
                room->checkFlag(FlagType::Room, ROOM_GARDEN2)) {
            ch->sendf("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n",
                         check_saveroom_count(ch, nullptr));
        }
        if (GET_RADAR1(ch) == room->vn && GET_RADAR2(ch) == room->vn &&
            GET_RADAR3(ch) != room->vn) {
            ch->sendf("@CTwo of your buoys are floating here.@n\r\n");
        } else if (GET_RADAR1(ch) == room->vn && GET_RADAR2(ch) != room->vn &&
                   GET_RADAR3(ch) == room->vn) {
            ch->sendf("@CTwo of your buoys are floating here.@n\r\n");
        } else if (GET_RADAR1(ch) != room->vn && GET_RADAR2(ch) == room->vn &&
                   GET_RADAR3(ch) == room->vn) {
            ch->sendf("@CTwo of your buoys are floating here.@n\r\n");
        } else if (GET_RADAR1(ch) == room->vn && GET_RADAR2(ch) == room->vn &&
                   GET_RADAR3(ch) == room->vn) {
            ch->sendf("@CAll three of your buoys are floating here. Why?@n\r\n");
        } else if (GET_RADAR1(ch) == room->vn) {
            ch->sendf("@CYour @cBuoy #1@C is floating here.@n\r\n");
        } else if (GET_RADAR2(ch) == room->vn) {
            ch->sendf("@CYour @cBuoy #2@C is floating here.@n\r\n");
        } else if (GET_RADAR3(ch) == room->vn) {
            ch->sendf("@CYour @cBuoy #3@C is floating here.@n\r\n");
        }
    }
}

static void do_auto_exits2(struct room_data *room, struct char_data *ch) {
    int door, slen = 0;

    ch->sendf("\nExits: ");

    for (auto &[door, d] : room->getExits()) {

        auto dest = d->getDestination();
        if(!dest) continue;
        if (d->checkFlag(FlagType::Exit, EX_CLOSED))
            continue;

        ch->sendf("%s ", abbr_dirs[door]);
        slen++;
    }

    ch->sendf("%s\r\n", slen ? "" : "None!");
}

ACMD(do_exits) {
    /* Why duplicate code? */
    if (!PRF_FLAGGED(ch, PRF_NODEC)) {
        do_auto_exits(ch->getRoom(), ch, EXIT_COMPLETE);
    } else {
        do_auto_exits2(ch->getRoom(), ch);
    }
}

static const char *exitlevels[] = {
        "off", "normal", "n/a", "complete", "\n"};

ACMD(do_autoexit) {
    char arg[MAX_INPUT_LENGTH];
    int tp;

    if (IS_NPC(ch))
        return;

    one_argument(argument, arg);


    if (!*arg) {
        ch->sendf("Your current autoexit level is %s.\r\n", exitlevels[EXIT_LEV(ch)]);
        return;
    }
    if (((tp = search_block(arg, exitlevels, false)) == -1)) {
        ch->sendf("Usage: Autoexit { Off | Normal | Complete }\r\n");
        return;
    }
    switch (tp) {
        case EXIT_OFF:
            for(auto f : {PRF_AUTOEXIT, PRF_FULL_EXIT}) ch->clearFlag(FlagType::Pref, f);
            break;
        case EXIT_NORMAL:
            ch->setFlag(FlagType::Pref, PRF_AUTOEXIT);
            ch->clearFlag(FlagType::Pref, PRF_FULL_EXIT);
            break;
        case EXIT_COMPLETE:
            for(auto f : {PRF_AUTOEXIT, PRF_FULL_EXIT}) ch->setFlag(FlagType::Pref, f);
            break;
    }
    ch->sendf("Your @rautoexit level@n is now %s.\r\n", exitlevels[EXIT_LEV(ch)]);
}

void look_at_room(room_rnum target_room, struct char_data *ch, int ignore_brief) {
    auto rm = dynamic_cast<room_data*>(world.at(target_room));
    look_at_room(rm, ch, ignore_brief);
}

void look_at_room(struct room_data *rm, struct char_data *ch, int ignore_brief) {
    trig_data *t;

    if (!ch->desc)
        return;
    
    auto sect = rm->sector_type;
    auto sunk = SUNKEN(rm->vn);

    if (IS_DARK(rm->vn) && !CAN_SEE_IN_DARK(ch) && !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
        ch->sendf("It's too dark to make out much detail...\r\n");
    } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
        ch->sendf("You see nothing but infinite darkness...\r\n");
        return;
    } else if (PLR_FLAGGED(ch, PLR_EYEC)) {
        ch->sendf("You can't see a damned thing, your eyes are closed!\r\n");
        return;
    }
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
        char buf[MAX_STRING_LENGTH];
        char buf2[MAX_STRING_LENGTH];
        char buf3[MAX_STRING_LENGTH];

        sprintf(buf, "%s", join(rm->getFlagNames(FlagType::Room), " ").c_str());
        sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
        if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC)) {
            ch->sendf("\r\n@wO----------------------------------------------------------------------O@n\r\n");
        }

        ch->sendf("@wLocation: @G%-70s@w\r\n", rm->name);
        if (!rm->script->dgScripts.empty()) {
            ch->sendf("@D[@GTriggers");
            for (auto t : rm->script->dgScripts)
                ch->sendf(" %d", GET_TRIG_VNUM(t));
            ch->sendf("@D] ");
        }
        if(rm->area) {
            std::vector<std::string> ancestors;
            auto parent = rm->getLocation();
            while(parent) {
                ancestors.emplace_back(fmt::format("[{}] {}@n", parent->uid, parent->getDisplayName(ch)));
                parent = parent->getLocation();
            }
            // Reverse areas.
            std::reverse(ancestors.begin(), ancestors.end());
            auto joined = join(ancestors, " -> ");
            ch->sendf("@wArea: @D[@n %s @D]@n\r\n", joined.c_str());
        }
        double grav = rm->getEnvVar(EnvVar::Gravity);
        auto g = fmt::format("{}", grav);
        sprintf(buf3, "@D[ @G%s@D] @wSector: @D[ @G%s @D] @wVnum: @D[@G%5d@D]@n Gravity: @D[@G%sx@D]@n", buf, buf2,
                rm->vn, g.c_str());
        ch->sendf("@wFlags: %-70s@w\r\n", buf3);
        if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC)) {
            ch->sendf("@wO----------------------------------------------------------------------O@n\r\n");
        }
    } else {
        if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC)) {
            ch->sendf("@wO----------------------------------------------------------------------O@n\r\n");
        }
        ch->sendf("@wLocation: %-70s@n\r\n", rm->name);
        if(auto planet = ch->getPlanet(); planet) {
            ch->sendf("@wPlanet: @G%s@n\r\n", planet->getDisplayName(ch));
        } else {
            if (rm->checkFlag(FlagType::Room, ROOM_NEO)) {
                ch->sendf("@wPlanet: @WNeo Nirvana@n\r\n");
            } else if (rm->checkFlag(FlagType::Room, ROOM_AL)) {
                ch->sendf("@wDimension: @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye@n\r\n");
            } else if (rm->checkFlag(FlagType::Room, ROOM_HELL)) {
                ch->sendf("@wDimension: @RPunishment Hell@n\r\n");
            } else if (rm->checkFlag(FlagType::Room, ROOM_RHELL)) {
                ch->sendf("@wDimension: @RH@re@Dl@Rl@n\r\n");
            }
        }

        double grav = rm->getEnvVar(EnvVar::Gravity);
        if(grav <= 1.0) {
            ch->sendf("@wGravity: @WNormal@n\r\n");
        } else {
            auto g = fmt::format("{}", grav);
            ch->sendf("@wGravity: @W%sx@n\r\n", g.c_str());
        }
        if (rm->checkFlag(FlagType::Room, ROOM_REGEN)) {
            ch->sendf("@CA feeling of calm and relaxation fills this room.@n\r\n");
        }
        if (rm->checkFlag(FlagType::Room, ROOM_AURA)) {
            ch->sendf("@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
        }
        if (rm->checkFlag(FlagType::Room, ROOM_HBTC)) {
            ch->sendf("@rThis room feels like it opperates in a different time frame.@n\r\n");
        }
        if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC)) {
            ch->sendf("@wO----------------------------------------------------------------------O@n\r\n");
        }
    }
    
    auto dmg = rm->getDamage();

    if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || rm->checkFlag(FlagType::Room, ROOM_DEATH)) {
        if (dmg <= 99) {
            ch->sendf("@w%s@n", rm->getLookDesc());
        }
        if (dmg == 100 &&
            (sect == SECT_WATER_SWIM || sunk || sect == SECT_FLYING ||
             sect == SECT_SHOP || sect == SECT_IMPORTANT)) {
            ch->sendf("@w%s@n", rm->getLookDesc());
        }
        if (sect == SECT_INSIDE && dmg > 0) {
            ch->sendf("\r\n");
            if (dmg <= 2) {
                ch->sendf("@wA small hole with chunks of debris that can be seen scarring the floor.@n");
            } else if (dmg <= 4) {
                ch->sendf("@wA couple small holes with chunks of debris that can be seen scarring the floor.@n");
            } else if (dmg <= 6) {
                ch->sendf("@wA few small holes with chunks of debris that can be seen scarring the floor.@n");
            } else if (dmg <= 10) {
                ch->sendf(
                             "@wThere are several small holes with chunks of debris that can be seen scarring the floor.@n");
            } else if (dmg <= 20) {
                ch->sendf("@wMany holes fill the floor of this area, many of which have burn marks.@n");
            } else if (dmg <= 30) {
                ch->sendf("@wThe floor is severely damaged with many large holes.@n");
            } else if (dmg <= 50) {
                ch->sendf(
                             "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n");
            } else if (dmg <= 75) {
                ch->sendf("@wThis entire area is falling apart, it has been damaged so badly.@n");
            } else if (dmg <= 99) {
                ch->sendf(
                             "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n");
            } else if (dmg >= 100) {
                ch->sendf(
                             "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up holes, and overflowing onto what is left of the\r\nfloor. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n");
            }
            ch->sendf("\r\n");
        } else if (
                (sect == SECT_CITY || sect == SECT_FIELD || sect == SECT_HILLS ||
                 sect == SECT_IMPORTANT) && dmg > 0) {
            ch->sendf("\r\n");
            if (dmg <= 2) {
                ch->sendf("@wA small hole with chunks of debris that can be seen scarring the ground.@n");
            } else if (dmg <= 4) {
                ch->sendf(
                             "@wA couple small craters with chunks of debris that can be seen scarring the ground.@n");
            } else if (dmg <= 6) {
                ch->sendf("@wA few small craters with chunks of debris that can be seen scarring the ground.@n");
            } else if (dmg <= 10) {
                ch->sendf(
                             "@wThere are several small craters with chunks of debris that can be seen scarring the ground.@n");
            } else if (dmg <= 20) {
                ch->sendf("@wMany craters fill the ground of this area, many of which have burn marks.@n");
            } else if (dmg <= 30) {
                ch->sendf("@wThe ground is severely damaged with many large craters.@n");
            } else if (dmg <= 50) {
                ch->sendf(
                             "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n");
            } else if (dmg <= 75) {
                ch->sendf("@wThis entire area is falling apart, it has been damaged so badly.@n");
            } else if (dmg <= 99) {
                ch->sendf(
                             "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n");
            } else if (dmg >= 100) {
                ch->sendf(
                             "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up craters, and overflowing onto what is left of the\r\nground. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n");
            }
            ch->sendf("\r\n");
        } else if (sect == SECT_FOREST && dmg > 0) {
            ch->sendf("\r\n");
            if (dmg <= 2) {
                ch->sendf("@wA small tree sits in a little crater here.@n");
            } else if (dmg <= 4) {
                ch->sendf("@wTrees have been uprooted by craters in the ground.@n");
            } else if (dmg <= 6) {
                ch->sendf(
                             "@wSeveral trees have been reduced to chunks of debris and are\r\nlaying in a few craters here. @n");
            } else if (dmg <= 10) {
                ch->sendf("@wA large patch of trees have been destroyed and are laying in craters here.@n");
            } else if (dmg <= 20) {
                ch->sendf("@wSeveral craters have merged into one large crater in one part of this forest.@n");
            } else if (dmg <= 30) {
                ch->sendf(
                             "@wThe open sky can easily be seen through a hole of trees destroyed\r\nand resting at the bottom of several craters here.@n");
            } else if (dmg <= 50) {
                ch->sendf(
                             "@wA good deal of burning tree pieces can be found strewn across the cratered ground here.@n");
            } else if (dmg <= 75) {
                ch->sendf(
                             "@wVery few trees are left standing in this area, replaced instead by large craters.@n");
            } else if (dmg <= 99) {
                ch->sendf(
                             "@wSingle solitary trees can be found still standing here or there in the area.\r\nThe rest have been almost completely obliterated in recent conflicts.@n");
            } else if (dmg >= 100) {
                ch->sendf(
                             "@w  One massive crater fills this area. This desolate crater leaves no\r\nevidence of what used to be found in the area. Smoke slowly wafts into\r\nthe sky from the central point of the crater, creating an oppressive\r\natmosphere.@n");
            }
            ch->sendf("\r\n");
        } else if (sect == SECT_MOUNTAIN && dmg > 0) {
            ch->sendf("\r\n");
            
            if (dmg <= 2) {
                ch->sendf("@wA small crater has been burned into the side of this mountain.@n");
            } else if (dmg <= 4) {
                ch->sendf("@wA couple craters have been burned into the side of this mountain.@n");
            } else if (dmg <= 6) {
                ch->sendf(
                             "@wBurned bits of boulders can be seen lying at the bottom of a few nearby craters.@n");
            } else if (dmg <= 10) {
                ch->sendf("@wSeveral bad craters can be seen in the side of the mountain here.@n");
            } else if (dmg <= 20) {
                ch->sendf(
                             "@wLarge boulders have rolled down the mountain side and collected in many nearby craters.@n");
            } else if (dmg <= 30) {
                ch->sendf("@wMany craters are covering the mountainside here.@n");
            } else if (dmg <= 50) {
                ch->sendf(
                             "@wThe mountain side has partially collapsed, shedding rubble down towards its base.@n");
            } else if (dmg <= 75) {
                ch->sendf("@wA peak of the mountain has been blown off, leaving behind a smoldering tip.@n");
            } else if (dmg <= 99) {
                ch->sendf(
                             "@wThe mountain side here has completely collapsed, shedding dangerous rubble down to its base.@n");
            } else if (dmg >= 100) {
                ch->sendf(
                             "@w  Half the mountain has been blown away, leaving a scarred and jagged\r\nrock in its place. Billowing smoke wafts up from several parts of the\r\nmountain, filling the nearby skies and blotting out the sun.@n");
            }
            ch->sendf("\r\n");
        }
        if (rm->geffect >= 1 && rm->geffect <= 5) {
            ch->sendf("@rLava@w is pooling in someplaces here...@n\r\n");
        }
        if (rm->geffect >= 6) {
            ch->sendf("@RLava@r covers pretty much the entire area!@n\r\n");
        }
        if (rm->geffect < 0) {
            ch->sendf("@cThe entire area is flooded with a @Cmystical@c cube of @Bwater!@n\r\n");
        }
    }
    /* autoexits */
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NODEC))
        do_auto_exits2(rm, ch);
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
        do_auto_exits(rm, ch, EXIT_LEV(ch));

    /* now list characters & objects */
    if (rm->checkFlag(FlagType::Room, ROOM_GARDEN1)) {
        ch->sendf("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", check_saveroom_count(ch, nullptr));
    } else {
        if (rm->checkFlag(FlagType::Room, ROOM_GARDEN2)) {
            ch->sendf("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n", check_saveroom_count(ch, nullptr));
        } else if (rm->checkFlag(FlagType::Room, ROOM_HOUSE)) {
            ch->sendf("@D[@GItems Stored@D: @g%d@D]@n\r\n", check_saveroom_count(ch, nullptr));
        }
    }
    list_obj_to_char(rm->getInventory(), ch, SHOW_OBJ_LONG, false);
    list_char_to_char(rm->getPeople(), ch);
}

static void look_in_direction(struct char_data *ch, int dir) {
    auto r = ch->getRoom();
    if(!r) {
        ch->sendf("Nothing special there...\r\n");
        return;
    }
    auto exits = r->getExits();
    auto d = exits[dir];
    if(!d) {
        ch->sendf("Nothing special there...\r\n");
        return;
    }
    auto dest = d->getDestination();
    if(!dest) {
        ch->sendf("Nothing special there...\r\n");
        return;
    }

    if (auto desc = d->getLookDesc(); !desc.empty()) {
        ch->sendText(desc);

        bool canSeeRoom = false;

        auto alias = d->getAlias();

        if (d->checkFlag(FlagType::Exit, EX_ISDOOR) && !alias.empty()) {
            if (!d->checkFlag(FlagType::Exit, EX_SECRET) &&
                d->checkFlag(FlagType::Exit, EX_CLOSED))
                ch->sendf("The %s is closed.\r\n", fname(alias.c_str()));
            else if (!d->checkFlag(FlagType::Exit, EX_CLOSED)) {
                ch->sendf("The %s is open.\r\n", fname(alias.c_str()));
                canSeeRoom = true;
            }
        } else {
            canSeeRoom = true;
        }

        if(canSeeRoom) {
            ch->sendf("You peek over and see:\r\n");
            ch->lookAtLocation();
        }
    }

}



static void look_in_obj(struct char_data *ch, char *arg) {
    struct obj_data *obj = nullptr;
    struct char_data *dummy = nullptr;
    int amt, bits;

    if (!*arg)
        ch->sendf("Look in what?\r\n");
    else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
        ch->sendf("There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    } else if (find_exdesc(arg, obj->ex_description) != nullptr && !bits)
        ch->sendf("There's nothing inside that!\r\n");
    else if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) && !OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) {
        if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < 0) {
            /* You can look through the portal to the destination */
            /* where does this lead to? */
            room_rnum portal_dest = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST));
            if (portal_dest == NOWHERE) {
                ch->sendf("You see nothing but infinite darkness...\r\n");
            } else if (IS_DARK(portal_dest) && !CAN_SEE_IN_DARK(ch) && !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
                ch->sendf("You see nothing but infinite darkness...\r\n");
            } else {
                ch->sendf("After seconds of concentration you see the image of %s.\r\n",
                             world[portal_dest]->name);
            }
        } else if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < MAX_PORTAL_TYPES) {
            /* display the appropriate description from the list of descriptions
*/
            ch->sendf("%s\r\n", portal_appearance[GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR)]);
        } else {
            /* We shouldn't really get here, so give a default message */
            ch->sendf("All you can see is the glow of the portal.\r\n");
        }
    } else if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
        if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
            ch->sendf("It is closed.\r\n");
        else if (GET_OBJ_VAL(obj, VAL_VEHICLE_APPEAR) < 0) {
            /* You can look inside the vehicle */
            /* where does this lead to? */
            room_rnum vehicle_inside = real_room(GET_OBJ_VAL(obj, VAL_VEHICLE_ROOM));
            if (vehicle_inside == NOWHERE) {
                ch->sendf("You cannot see inside that.\r\n");
            } else if (IS_DARK(vehicle_inside) && !CAN_SEE_IN_DARK(ch) && !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
                ch->sendf("It is pitch black...\r\n");
            } else {
                ch->sendf("You look inside and see:\r\n");
                look_at_room(vehicle_inside, ch, 0);
                //ch->sendEvent(world.at(vehicle_inside)->renderLocationFor(ch));
            }
        } else {
            ch->sendf("You cannot see inside that.\r\n");
        }
    } else if (GET_OBJ_TYPE(obj) == ITEM_WINDOW) {
        look_out_window(ch, arg);
    } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
               (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
               (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) &&
               (GET_OBJ_TYPE(obj) != ITEM_PORTAL)) {
        ch->sendf("There's nothing inside that!\r\n");
    } else if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) ||
               (GET_OBJ_TYPE(obj) == ITEM_PORTAL)) {
        if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
            ch->sendf("It is closed.\r\n");
        else {
            ch->sendf("%s", obj->getShortDesc());
            if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER &&
                (GET_OBJ_VNUM(obj) == 697 || GET_OBJ_VNUM(obj) == 698 || GET_OBJ_VNUM(obj) == 682 ||
                 GET_OBJ_VNUM(obj) == 683 || GET_OBJ_VNUM(obj) == 684)) {
                act("$n looks in $p.", true, ch, obj, nullptr, TO_ROOM);
            }
            switch (bits) {
                case FIND_OBJ_INV:
                    ch->sendf(" (carried): \r\n");
                    break;
                case FIND_OBJ_ROOM:
                    ch->sendf(" (here): \r\n");
                    break;
                case FIND_OBJ_EQUIP:
                    ch->sendf(" (used): \r\n");
                    break;
            }

            list_obj_to_char(obj->getInventory(), ch, SHOW_OBJ_SHORT, true);
        }
    } else {        /* item must be a fountain or drink container */
        if (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) <= 0 && (!GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY) == 1))
            ch->sendf("It is empty.\r\n");
        else {
            if (GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY) < 0) {
                char buf2[MAX_STRING_LENGTH];
                sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
                ch->sendf("It's full of a %s liquid.\r\n", buf2);
            } else if (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) > GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY)) {
                ch->sendf("Its contents seem somewhat murky.\r\n"); /* BUG */
            } else {
                char buf2[MAX_STRING_LENGTH];
                amt = GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY);
                int leftin = GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL);
                sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
                if (leftin == amt) {
                    ch->sendf("It's full of a %s liquid.\r\n", buf2);
                } else if (leftin >= amt * .8) {
                    ch->sendf("It's almost full of a %s liquid.\r\n", buf2);
                } else if (leftin >= amt * .5) {
                    ch->sendf("It's about half full of a %s liquid.\r\n", buf2);
                } else if (leftin >= amt * .2) {
                    ch->sendf("It's less than half full of a %s liquid.\r\n", buf2);
                } else if (leftin > 0) {
                    ch->sendf("It's barely filled with a %s liquid.\r\n", buf2);
                } else {
                    ch->sendf("It's empty.\r\n");
                }
            }
        }
    }
}

char *find_exdesc(char *word, const std::vector<extra_descr_data>& list) {

    for (auto i : list)
        /*if (isname(word, i->keyword))*/
        if (i.keyword.starts_with(".") ? isname(word, i.keyword.substr(1).c_str()) : isname(word, i.keyword.c_str()))
            return (char*)i.description.c_str();

    return nullptr;
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
static void look_at_target(struct char_data *ch, char *arg, int cmread) {
    int bits, found = false, j, fnum, i = 0, msg = 1;
    struct char_data *found_char = nullptr;
    struct obj_data *obj, *found_obj = nullptr;
    char *desc;
    char number[MAX_STRING_LENGTH];

    if (!ch->desc)
        return;

    if (!*arg) {
        ch->sendf("Look at what?\r\n");
        return;
    }

     auto isBoard = [](const auto &o) {
        return GET_OBJ_TYPE(o) == ITEM_BOARD;};

    if (cmread) {

        obj = ch->findObject(isBoard);
        if(!obj) obj = ch->getRoom()->findObject(isBoard);

        if (obj) {
            arg = one_argument(arg, number);
            if (!*number) {
                ch->sendf("Read what?\r\n");
                return;
            }

            /* Okay, here i'm faced with the fact that the person could be
	 entering in something like 'read 5' or 'read 4.mail' .. so, whats the
	 difference between the two?  Well, there's a period in the second,
	 so, we'll just stick with that basic difference */

            if (isname(number, obj->name)) {
                show_board(GET_OBJ_VNUM(obj), ch);
            } else if ((!isdigit(*number) || (!(msg = atoi(number)))) ||
                       (strchr(number, '.'))) {
                sprintf(arg, "%s %s", number, arg);
                look_at_target(ch, arg, 0);
            } else {
                board_display_msg(GET_OBJ_VNUM(obj), ch, msg);
            }
        }
    } else {
        bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
                                 FIND_CHAR_ROOM, ch, &found_char, &found_obj);

        /* Is the target a character? */
        if (found_char != nullptr) {
            look_at_char(found_char, ch);
            if (ch != found_char) {
                if (!AFF_FLAGGED(ch, AFF_HIDE)) {
                    act("$n looks at you.", true, ch, nullptr, found_char, TO_VICT);
                    act("$n looks at $N.", true, ch, nullptr, found_char, TO_NOTVICT);
                }
            }
            return;
        }

        /* Strip off "number." from 2.foo and friends. */
        if (!(fnum = get_number(&arg))) {
            ch->sendf("Look at what?\r\n");
            return;
        }

        /* Does the argument match an extra desc in the room? */
        if ((desc = find_exdesc(arg, ch->getRoom()->ex_description)) != nullptr && ++i == fnum) {
            write_to_output(ch->desc, desc);
            return;
        }

        /* Does the argument match an extra desc in the char's equipment? */
        for (j = 0; j < NUM_WEARS && !found; j++)
            if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
                if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != nullptr && ++i == fnum) {
                    ch->sendf("%s", desc);
                    if (isname(arg, GET_EQ(ch, j)->name)) {
                        if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_WEAPON) {
                            ch->sendf("The weapon type of %s is a %s.\r\n",
                                         GET_OBJ_SHORT(GET_EQ(ch, j)),
                                         weapon_type[(int) GET_OBJ_VAL(GET_EQ(ch, j), VAL_WEAPON_SKILL)]);
                        }
                        if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_SPELLBOOK) {
                            display_spells(ch, GET_EQ(ch, j));
                        }
                        if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_SCROLL) {
                            display_scroll(ch, GET_EQ(ch, j));
                        }
                        diag_obj_to_char(GET_EQ(ch, j), ch);
                        ch->sendf("It appears to be made of %s",
                                     material_names[GET_OBJ_MATERIAL(GET_EQ(ch, j))]);
                    }
                    found = true;
                }

        /* Does the argument match an extra desc in the char's inventory? */
        for (auto obj : ch->getInventory()) {
            if (CAN_SEE_OBJ(ch, obj))
                if ((desc = find_exdesc(arg, obj->ex_description)) != nullptr && ++i == fnum) {
                    if (isBoard(obj)) {
                        show_board(GET_OBJ_VNUM(obj), ch);
                    } else {
                        ch->sendf("%s", desc);
                        if (isname(arg, obj->name)) {
                            if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
                                ch->sendf("The weapon type of %s is a %s.\r\n",
                                             GET_OBJ_SHORT(obj), weapon_type[(int) GET_OBJ_VAL(obj,
                                                                                               VAL_WEAPON_SKILL)]);
                            }
                            if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
                                display_spells(ch, obj);
                            }
                            if (GET_OBJ_TYPE(obj) == ITEM_SCROLL) {
                                display_scroll(ch, obj);
                            }
                            diag_obj_to_char(obj, ch);
                            ch->sendf("It appears to be made of %s, and weights %s",
                                         material_names[GET_OBJ_MATERIAL(obj)], add_commas(GET_OBJ_WEIGHT(obj)).c_str());
                        }
                    }
                    found = true;
                }
        }

        /* Does the argument match an extra desc of an object in the room? */
        for (auto obj : ch->getRoom()->getInventory())
            if (CAN_SEE_OBJ(ch, obj))
                if ((desc = find_exdesc(arg, obj->ex_description)) != nullptr && ++i == fnum) {
                    if (isBoard(obj)) {
                        show_board(GET_OBJ_VNUM(obj), ch);
                    } else {
                        ch->sendf("%s", desc);
                        if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
                            ch->sendf("@YSyntax@D: @CUnlock hatch\r\n");
                            ch->sendf("@YSyntax@D: @COpen hatch\r\n");
                            ch->sendf("@YSyntax@D: @CClose hatch\r\n");
                            ch->sendf("@YSyntax@D: @CUnlock hatch\r\n");
                            ch->sendf("@YSyntax@D: @CEnter hatch\r\n");
                        } else if (GET_OBJ_TYPE(obj) == ITEM_HATCH) {
                            ch->sendf("@YSyntax@D: @CUnlock hatch\r\n");
                            ch->sendf("@YSyntax@D: @COpen hatch\r\n");
                            ch->sendf("@YSyntax@D: @CClose hatch\r\n");
                            ch->sendf("@YSyntax@D: @CUnlock hatch\r\n");
                            ch->sendf("@YSyntax@D: @CLeave@n\r\n");
                        } else if (GET_OBJ_TYPE(obj) == ITEM_WINDOW) {
                            look_out_window(ch, obj->name);
                        }

                        if (GET_OBJ_TYPE(obj) == ITEM_CONTROL) {
                            ch->sendf("@RFUEL@D: %s%s@n\r\n",
                                         GET_FUEL(obj) >= 200 ? "@G" : GET_FUEL(obj) >= 100 ? "@Y" : "@r",
                                         add_commas(GET_FUEL(obj)).c_str());
                        }
                        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
                            ch->sendf("The weapon type of %s is a %s.\r\n", GET_OBJ_SHORT(obj),
                                         weapon_type[(int) GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
                        }
                        diag_obj_to_char(obj, ch);
                        ch->sendf("It appears to be made of %s, and weights %s",
                                     material_names[GET_OBJ_MATERIAL(obj)], add_commas(GET_OBJ_WEIGHT(obj)).c_str());
                    }
                    found = true;
                }

        /* If an object was found back in generic_find */
        if (bits) {
            if (!found)
                show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
            else {
                if (show_obj_modifiers(found_obj, ch))
                    ch->sendf("\r\n");
            }
        } else if (!found)
            ch->sendf("You do not see that here.\r\n");
    }
}

static void look_out_window(struct char_data *ch, char *arg) {
    struct obj_data *i, *viewport = nullptr, *vehicle = nullptr;
    struct char_data *dummy = nullptr;
    room_rnum target_room = NOWHERE;
    int bits, door;

    auto r = ch->getRoom();

    /* First, lets find something to look out of or through. */
    if (*arg) {
        /* Find this object and see if it is a window */
        if (!(bits = generic_find(arg,
                                  FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP,
                                  ch, &dummy, &viewport))) {
            ch->sendf("You don't see that here.\r\n");
            return;
        } else if (GET_OBJ_TYPE(viewport) != ITEM_WINDOW) {
            ch->sendf("You can't look out that!\r\n");
            return;
        }
    } else if (OUTSIDE(ch)) {
        /* yeah, sure stupid */
        ch->sendf("But you are already outside.\r\n");
        return;
    } else {
        /* Look for any old window in the room */
        viewport = ch->getRoom()->findObject([&](auto obj) {return GET_OBJ_TYPE(obj) == ITEM_WINDOW && isname("window", obj->name);});
    }
    if (!viewport) {
        /* Nothing suitable to look through */
        ch->sendf("You don't seem to be able to see outside.\r\n");
    } else if (OBJVAL_FLAGGED(viewport, CONT_CLOSEABLE) &&
               OBJVAL_FLAGGED(viewport, CONT_CLOSED)) {
        /* The window is closed */
        ch->sendf("It is closed.\r\n");
    } else {
        if (GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED1) < 0) {
            /* We are looking out of the room */
            if (GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED4) < 0) {
                /* Look for the default "outside" room */
                for (auto &[door, e] : r->getExits()) {
                    auto dest = e->getDestination();
                    if(!dest) continue;
                    if(!dest->checkFlag(FlagType::Room, ROOM_INDOORS)) {
                        target_room = dest->vn;
                        break;
                    }
                }
            } else {
                target_room = real_room(GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED4));
            }
        } else {
            /* We are looking out of a vehicle */
            if ((vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED1))))
                target_room = IN_ROOM(vehicle);
        }
        if (target_room == NOWHERE) {
            ch->sendf("You don't seem to be able to see outside.\r\n");
        } else {
            if (auto vp = viewport->getLookDesc(); !vp.empty())
                act(vp.c_str(), true, ch, viewport, nullptr, TO_CHAR);
            else
                act("$n looks out the window.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->sendf("You look outside and see:\r\n");
            look_at_room(target_room, ch, 0);
            //ch->sendEvent(world.at(target_room)->renderLocationFor(ch));
        }
    }
}

ACMD(do_finger) {

    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg) {
        ch->sendf("What user are you wanting to look at?\r\n");
        return;
    }

    auto account = findAccount(arg);
    if (!account) {
        ch->sendf("That user does not exist\r\n");
        return;
    }
    fingerUser(ch, account);
}

ACMD(do_rptrans) {
    struct char_data *vict = nullptr;
    struct descriptor_data *k;
    int amt = 0;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
        ch->sendf("Syntax: exchange (target) (amount)\r\n");
        return;
    }

    amt = atoi(arg2);

    if (amt <= 0) {
        ch->sendf("Are you being funny?\r\n");
        return;
    }

    auto haveRP = GET_RP(ch);

    if (amt > haveRP) {
        ch->sendf("@WYou only have @C%d@W RPP!@n\r\n", haveRP);
        return;
    }

    if (!readUserIndex(arg)) {
        ch->sendf("That is not a recognised user file.\r\n");
        return;
    }

    for (k = descriptor_list; k; k = k->next) {
        if (IS_NPC(k->character))
            continue;
        if (STATE(k) != CON_PLAYING)
            continue;
        if (!strcasecmp(k->account->name.c_str(), arg))
            vict = k->character;
    }
    if (vict == nullptr) {

    } else {
        vict->modRPP(amt);
        vict->sendf("@W%s gives @C%d@W of their RPP to you. How nice!\r\n", GET_NAME(ch), amt);
    }
    ch->modRPP(-amt);
    ch->sendf("@WYou exchange @C%d@W RPP to user @c%s@W for a warm fuzzy feeling.\r\n", amt, CAP(arg));
    mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), true, "EXCHANGE: %s gave %d RPP to user %s", GET_NAME(ch), amt,
           arg);
}


ACMD(do_rdisplay) {
    skip_spaces(&argument);

    if (IS_NPC(ch)) {
        return;
    }

    if (!*argument) {
        ch->sendf("Clearing room display.\r\n");
        GET_RDISPLAY(ch) = "Empty";
    } else {
        char derp[MAX_STRING_LENGTH];

        strcpy(derp, argument);

        ch->sendf("You set your display to; %s\r\n", derp);
        GET_RDISPLAY(ch) = strdup(derp);
    }
}

int perf_skill(int skill) {
    switch (skill) {
        case 464:
        case 441:
        case 444:
        case 475:
        case 474:
        case 476:
        case 488:
        case 472:
        case 485:
        case 442:
        case 510:
        case 533:
            return 1;
        default:
            return 0;
    }
}

ACMD(do_perf) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int i, skill = 1, found = false, type = 0;

    two_arguments(argument, arg, arg2);

    if (IS_NPC(ch) || GET_ADMLEVEL(ch) > 0) {
        ch->sendf("I don't think so.\r\n");
        return;
    }

    if (!*arg || !*arg2) {
        ch->sendf("@WType @G1@D: @wOver Charged@n\r\n");
        ch->sendf("@WType @G2@D: @wAccurate@n\r\n");
        ch->sendf("@WType @G3@D: @wEfficient@n\r\n");
        ch->sendf("Syntax: perfect (skillname) (type 1/2/or 3)\r\n");
        return;
    }
    if (strlen(arg) < 4) {
        ch->sendf("The skill name should be longer than 3 characters...\r\n");
        return;
    }
    for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
        if (spell_info[i].skilltype != SKTYPE_SKILL)
            continue;

        if (found == true)
            continue;

        if (strstr(spell_info[i].name, arg)) {
            skill = i;
            found = true;
        }
    }
    if (found == false) {
        ch->sendf("The skill %s doesn't exist.\r\n", arg);
        return;
    }
    if (!GET_SKILL(ch, skill)) {
        ch->sendf("You don't know %s.\r\n", arg);
        return;
    }
    if (GET_SKILL(ch, skill) < 100) {
        ch->sendf("You have not mastered the skill %s and thus can't perfect it.\r\n", arg);
        return;
    }
    if (GET_SKILL_PERF(ch, skill) > 0) {
        ch->sendf("You have already mastered the skill %s and chosen how to perfect it.\r\n", arg);
        return;
    }
    if (!perf_skill(skill)) {
        ch->sendf("You can't perfect that type of skill.\r\n");
        return;
    }
    if (atoi(arg2) < 1 || atoi(arg2) > 3) {
        ch->sendf("@WType @G1@D: @wOver Charged@n\r\n");
        ch->sendf("@WType @G2@D: @wAccurate@n\r\n");
        ch->sendf("@WType @G3@D: @wEfficient@n\r\n");
        ch->sendf("@RType must be a number between 1 and 3.@n\r\n");
        return;
    } else {
        type = atoi(arg2);
        switch (type) {
            case 1:
                ch->sendf("You perfect the skill %s so that you can over charge it!\r\n",
                             spell_info[skill].name);
                SET_SKILL_PERF(ch, skill, 1);
                break;
            case 2:
                ch->sendf("You perfect the skill %s so that you have supreme accuracy with it!\r\n",
                             spell_info[skill].name);
                SET_SKILL_PERF(ch, skill, 2);
                break;
            case 3:
                ch->sendf("You perfect the skill %s so that you require a lower minimum charge for it!\r\n",
                             spell_info[skill].name);
                SET_SKILL_PERF(ch, skill, 3);
                break;
        }
    }
}

ACMD(do_look) {
    int look_type;

    if (!ch->desc)
        return;

    if (GET_POS(ch) < POS_SLEEPING)
    {
        ch->sendf("You can't see anything but stars!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_BLIND))
    {
        ch->sendf("You can't see a damned thing, you're blind!\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_EYEC)) {
        ch->sendf("You can't see a damned thing, your eyes are closed!\r\n");
        return;
    }

    auto room = ch->getRoom();
    auto exits = room->getExits();

    if(IS_DARK(room->vn) && !CAN_SEE_IN_DARK(ch)) {
        ch->sendf("It is pitch black...\r\n");
        list_char_to_char(room->getPeople(), ch);    /* glowing red eyes */
        return;
    }

    char arg[MAX_INPUT_LENGTH], arg2[200];

    if (subcmd == SCMD_READ) {
        one_argument(argument, arg);
        if (!*arg)
            ch->sendf("Read what?\r\n");
        else
            look_at_target(ch, arg, 1);
        return;
    }
    argument = any_one_arg(argument, arg);
    one_argument(argument, arg2);
    if (!*arg) {
        if (subcmd == SCMD_SEARCH) {
            search_room(ch);
        } else {
            ch->lookAtLocation();
            if (GET_ADMLEVEL(ch) < 1 && !AFF_FLAGGED(ch, AFF_HIDE)) {
                //act("@w$n@w looks around the room.@n", TRUE, ch, 0, 0, TO_ROOM);
            }
        }
    } else if(is_abbrev(arg, "moon")) {
        switch(room->checkMoon()) {
            case MoonCheck::NoMoon:
                ch->sendf("It's kinda hard to see any moons from here.\r\n");
                return;
            case MoonCheck::NotFull:
                ch->sendf("You gaze skyward, but there's no full moon in sight.\r\n");
                return;
            case MoonCheck::Full:
                ch->sendf("You gaze upon a wondrous full moon... it's an amazing sight.\r\n");
                ch->gazeAtMoon();
                return;
        }
    }
    else if (is_abbrev(arg, "inside") && exits[INDIR] && !*arg2) {
        if (subcmd == SCMD_SEARCH)
            search_in_direction(ch, INDIR);
        else
            look_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") && (subcmd == SCMD_SEARCH) && !*arg2) {
        search_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") ||
               is_abbrev(arg, "into") || is_abbrev(arg, "onto")) {
        look_in_obj(ch, arg2);
    } else if ((is_abbrev(arg, "outside") ||
                is_abbrev(arg, "through") ||
                is_abbrev(arg, "thru")) &&
               (subcmd == SCMD_LOOK) && *arg2) {
        look_out_window(ch, arg2);
    } else if (is_abbrev(arg, "outside") &&
               (subcmd == SCMD_LOOK) && !exits[OUTDIR]) {
        look_out_window(ch, arg2);
    } else if ((look_type = search_block(arg, dirs, false)) >= 0 ||
               (look_type = search_block(arg, abbr_dirs, false)) >= 0) {
        if (subcmd == SCMD_SEARCH)
            search_in_direction(ch, look_type);
        else
            look_in_direction(ch, look_type);
    } else if ((is_abbrev(arg, "towards")) &&
               ((look_type = search_block(arg2, dirs, false)) >= 0 ||
                (look_type = search_block(arg2, abbr_dirs, false)) >= 0)) {
        if (subcmd == SCMD_SEARCH)
            search_in_direction(ch, look_type);
        else
            look_in_direction(ch, look_type);
    } else if (is_abbrev(arg, "at")) {
        if (subcmd == SCMD_SEARCH)
            ch->sendf("That is not a direction!\r\n");
        else
            look_at_target(ch, arg2, 0);
    } else if (is_abbrev(arg, "around")) {
        struct extra_descr_data *i;
        int found = 0;

        for (auto i : ch->getRoom()->ex_description) {
            if (!i.keyword.starts_with(".")) {
                ch->sendf("%s%s:\r\n%s",
                             (found ? "\r\n" : ""), i.keyword, i.description);
                found = 1;
            }
        }
        if (!found)
            ch->sendf("You couldn't find anything noticeable.\r\n");
    } else if (find_exdesc(arg, ch->getRoom()->ex_description) != nullptr) {
        look_at_target(ch, arg, 0);
    } else {
        if (subcmd == SCMD_SEARCH)
            ch->sendf("That is not a direction!\r\n");
        else
            look_at_target(ch, arg, 0);
    }

}

ACMD(do_examine) {
    struct char_data *tmp_char;
    struct obj_data *tmp_object;
    char tempsave[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg) {
        ch->sendf("Examine what?\r\n");
        return;
    }

    /* look_at_target() eats the number. */
    look_at_target(ch, strcpy(tempsave, arg), 0);    /* strcpy: OK */

    generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
                      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

    if (tmp_object) {
        if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
            (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
            (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
            ch->sendf("When you look inside, you see:\r\n");
            look_in_obj(ch, arg);
        }
    }
}

ACMD(do_gold) {
    if (GET_GOLD(ch) == 0)
        ch->sendf("You're broke!\r\n");
    else if (GET_GOLD(ch) == 1)
        ch->sendf("You have one little zenni.\r\n");
    else
        ch->sendf("You have %d zenni.\r\n", GET_GOLD(ch));
}

ACMD(do_score) {

    if (IS_NPC(ch))
        return;

    int view = 0, full = 5, personal = 1, health = 2, stats = 3, other = 4;

    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg) {
        view = full;
    } else if (strstr("personal", arg) || strstr("Personal", arg)) {
        view = personal;
    } else if (strstr("health", arg) || strstr("Health", arg)) {
        view = health;
    } else if (strstr("statistics", arg) || strstr("Statistics", arg)) {
        view = stats;
    } else if (strstr("other", arg) || strstr("Other", arg)) {
        view = other;
    } else {
        ch->sendf("Syntax: score, or... score (personal, health, statistics, other)\r\n");
        return;
    }

    if (view == full || view == personal) {
        ch->sendf("  @cO@D-----------------------------[  @cPersonal  @D]-----------------------------@cO@n\n");
        ch->sendf("  @D|  @CName@D: @W%15s@D,   @CTitle@D: @W%-38s@D|@n\n", GET_NAME(ch), GET_TITLE(ch));
        if (IS_ANDROID(ch)) {
            char model[100], version[100];
            int absorb = 0;
            if (PLR_FLAGGED(ch, PLR_ABSORB)) {
                sprintf(model, "@CAbsorption");
            } else if (PLR_FLAGGED(ch, PLR_REPAIR)) {
                sprintf(model, "@GSelf Repairing");
            } else if (PLR_FLAGGED(ch, PLR_SENSEM)) {
                sprintf(model, "@RSensor Equiped");
            }

            switch(ch->form) {
                case FormID::Base:
                    sprintf(version, "Alpha 0.5");
                    break;
                case FormID::Android10:
                    sprintf(version, "Beta 1.0");
                    break;
                case FormID::Android20:
                    sprintf(version, "ANS 2.0");
                    break;
                case FormID::Android30:
                    sprintf(version, "ANS 3.0");
                    break;
                case FormID::Android40:
                    sprintf(version, "ANS 4.0");
                    break;
                case FormID::Android50:
                    sprintf(version, "ANS 5.0");
                    break;
                case FormID::Android60:
                    sprintf(version, "ANS 6.0");
                    break;
                default:
                    break;
            }

            ch->sendf("  @D| @CModel@D: %15s@D,    @CUGP@D: @G%15s@D,  @CVersion@D: @r%-12s@D|@n\n", model,
                         absorb > 0 ? "@RN/A" : add_commas(GET_UP(ch)).c_str(), version);
        }
        if (GET_CLAN(ch) != nullptr) {
            ch->sendf("  @D|  @CClan@D: @W%-64s@D|@n\n", GET_CLAN(ch));
        }
        ch->sendf("  @D|  @CRace@D: @W%10s@D,  @CSensei@D: @W%15s@D,     @CArt@D: @W%-17s@D|@n\n", race::getName(ch->race),
                     sensei::getName(ch->chclass).c_str(), sensei::getStyle(ch->chclass).c_str());
        char hei[300], wei[300];
        sprintf(hei, "%dcm", ch->getHeight());
        sprintf(wei, "%dkg", (int)ch->getWeight());
        ch->sendf("  @D|   @CAge@D: @W%10s@D,  @CHeight@D: @W%15s@D,  @CWeight@D: @W%-17s@D|@n\n",
                     add_commas(GET_AGE(ch)).c_str(), hei, wei);
        ch->sendf("  @D|@CGender@D: @W%10s@D,  @C  Size@D: @W%15s@D,  @C Align@D: @W%-17s@D|@n\n",
                     genders[(int) GET_SEX(ch)], size_names[get_size(ch)], disp_align(ch));
    }
    if (view == full || view == health) {
        ch->sendf(
                     "  @cO@D-----------------------------@D[   @cHealth   @D]-----------------------------@cO@n\n");
        ch->sendf("                 @D<@rPowerlevel@D>          <@BKi@D>             <@GStamina@D>@n\n");
        ch->sendf("    @wCurrent   @D-[@R%-16s@D]-[@R%-16s@D]-[@R%-16s@D]@n\n", add_commas(ch->getCurPL()).c_str(),
                     add_commas(
                             (ch->getCurKI())).c_str(), add_commas((ch->getCurST())).c_str());
        ch->sendf("    @wMaximum   @D-[@r%-16s@D]-[@r%-16s@D]-[@r%-16s@D]@n\n", add_commas(ch->getEffMaxPL()).c_str(),
                     add_commas(GET_MAX_MANA(ch)).c_str(), add_commas(GET_MAX_MOVE(ch)).c_str());
        ch->sendf("    @wBase      @D-[@m%-16s@D]-[@m%-16s@D]-[@m%-16s@D]@n\n", add_commas((ch->getEffBasePL())).c_str(),
                     add_commas(
                             (ch->getEffBaseKI())).c_str(), add_commas((ch->getEffBaseST())).c_str());
        if (!IS_ANDROID(ch) && (ch->getCurLF()) > 0) {
            ch->sendf("    @wLife Force@D-[@C%16s@D%s@c%16s@D]- @wLife Percent@D-[@Y%3d%s@D]@n\n", add_commas(
                    (ch->getCurLF())).c_str(), "/", add_commas((ch->getMaxLF())).c_str(), GET_LIFEPERC(ch), "%");
        } else if (!IS_ANDROID(ch)) {
            ch->sendf("    @wLife Force@D-[@C%16s@D%s@c%16s@D]- @wLife Percent@D-[@Y%3d%s@D]@n\n", add_commas(0).c_str(),
                         "/", add_commas(
                            (ch->getMaxLF())).c_str(), GET_LIFEPERC(ch), "%");
        }
    }
    if (view == full || view == stats) {
        ch->sendf(
                     "  @cO@D-----------------------------@D[ @cStatistics @D]-----------------------------@cO@n\n");
        ch->sendf("      @D<@wCharacter Level@D: @w%-3d@D> <@wRPP@D: @w%-3d@D>@n\n",
                     GET_LEVEL(ch), GET_RP(ch));
        ch->sendf("      @D<@wSpeed Index@D: @w%-15s@D> <@wArmor Index@D: @w%-15s@D>@n\n",
                     add_commas(GET_SPEEDI(ch)).c_str(), add_commas(GET_ARMOR(ch)).c_str());
        ch->sendf(
                     "    @D[    @RStrength@D|@G%2d (%3d)@D] [     @YAgility@D|@G%2d (%3d)@D] [      @BSpeed@D|@G%2d (%3d)@D]@n\n",
                     ch->get(CharAttribute::Strength, true), GET_STR(ch), ch->get(CharAttribute::Agility, true), GET_DEX(ch), ch->get(CharAttribute::Speed, true), GET_CHA(ch));
        ch->sendf(
                     "    @D[@gConstitution@D|@G%2d (%3d)@D] [@CIntelligence@D|@G%2d (%3d)@D] [     @MWisdom@D|@G%2d (%3d)@D]@n\n",
                     ch->get(CharAttribute::Constitution, true), GET_CON(ch), ch->get(CharAttribute::Intelligence, true), GET_INT(ch), ch->get(CharAttribute::Wisdom, true),
                     GET_WIS(ch));
    }
    if (view == full || view == other) {
        ch->sendf(
                     "  @cO@D-----------------------------@D[   @cOther    @D]-----------------------------@cO@n\n");
        ch->sendf("                @D<@YZenni@D>                 <@rInventory Weight@D>@n\n");
        ch->sendf("      @D[   @CCarried@D| @W%-15s@D] [   @CCarried@D| @W%-15s@D]@n\n",
                     add_commas(GET_GOLD(ch)).c_str(), add_commas(
                        (ch->getCarriedWeight())).c_str());
        double gravity = ch->myEnvVar(EnvVar::Gravity);
        std::string grav = gravity > 1.0 ? fmt::format("(Gravity:", gravity) : "";
        ch->sendf("      @D[      @CBank@D| @W%-15s@D] [ @CMax Carry@D| @W%-15s@D]@n %s\n",
                     add_commas(GET_BANK_GOLD(ch)).c_str(), add_commas(CAN_CARRY_W(ch)).c_str(), grav.c_str());

        grav = gravity > 1.0 ? fmt::format("{}x)", add_commas(gravity)) : "";
        ch->sendf("      @D[ @CMax Carry@D| @W%-15s@D] [    @CBurden@D| @W%-15s@D]@n %s\n", add_commas(GOLD_CARRY(ch)).c_str(), add_commas(ch->getCurrentBurden()).c_str(), grav.c_str());
        int numb = 0;
        if (GET_BANK_GOLD(ch) > 99) {
            numb = (GET_BANK_GOLD(ch) / 100) * 2;
        } else if (GET_BANK_GOLD(ch) > 0) {
            numb = 1;
        } else {
            numb = 0;
        }
        if (numb >= 7500) {
            numb = 7500;
        }
        auto ratio = std::to_string(ch->getBurdenRatio() * 100.0) + "%";
        ch->sendf("      @D[  @CInterest@D| @W%-15s@D] [     @CRatio@D| @W%-15s@D]@n\n", add_commas(numb).c_str(), ratio.c_str());
        if (IS_ARLIAN(ch)) {
            ch->sendf("                             @D<@GEvolution @D>@n\n");
            ch->sendf("      @D[ @CEvo Level@D| @W%-15d@D] [   @CEvo Exp@D| @W%-15s@D]\n", GET_MOLT_LEVEL(ch),
                         add_commas(GET_MOLT_EXP(ch)).c_str());
            ch->sendf("      @D[ @CThreshold@D| @W%-15s@D]@n\n", add_commas(molt_threshold(ch)).c_str());
        }
        if (GET_LEVEL(ch) < 100) {
            ch->sendf("                             @D<@gAdvancement@D>@n\n");
        }
        if (GET_LEVEL(ch) < 100) {
            ch->sendf("      @D[@CExperience@D| @W%-15s@D] [@CNext Level@D| @W%-15s@D]@n\n",
                         add_commas(GET_EXP(ch)).c_str(), add_commas(level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch)).c_str());
            ch->sendf("      @D[  @CRpp Cost@D| @W%-15d@D]@n\n", rpp_to_level(ch));
        }

        ch->sendf(
                     "\n     @D<@wPlayed@D: @yYears @D(@W%2d@D) @yWeeks @D(@W%2d@D) @yDays @D(@W%2d@D) @yHours @D(@W%2d@D) @yMinutes @D(@W%2d@D)>@n\n",
                     (int64_t) ch->time.played / 31536000, (int) (((int64_t)ch->time.played % 31536000) / 604800),
                     (int) (((int64_t)ch->time.played % 604800) / 86400), (int) (((int64_t)ch->time.played % 86400) / 3600),
                     (int) (((int64_t)ch->time.played % 3600) / 60));
    }
    ch->sendf("  @cO@D------------------------------------------------------------------------@cO@n\n");

}

static void trans_check(struct char_data *ch, struct char_data *vict) {
/* Rillao: transloc, add new transes here */
    if(vict->form == FormID::Base || (vict->mimic && vict != ch)) {
        ch->sendf("         @cCurrent Transformation@D: @wNone@n\r\n");
        return;
    }

    ch->sendf("         @cCurrent Transformation@D: %s\r\n", trans::getName(vict, vict->form));

} // End trans check

ACMD(do_status) {
    char arg[MAX_INPUT_LENGTH];
    struct affected_type *aff;

    const char *forget_level[7] = {
            "@GRemembered Well@n",
            "@GRemembered Well Enough@n",
            "@RGetting Foggy@n",
            "@RHalf Forgotten@n",
            "@rAlmost Forgotten@n",
            "@rForgotten@n",
            "\n"
    };

    one_argument(argument, arg);

    if (!*arg) {
        ch->sendf("@D<@b------------------------@D[@YYour Status@D]@b-------------------------@D>@n\r\n\r\n");
        ch->sendf("            @D---------------@CAppearance@D---------------\n");
        bringdesc(ch, ch);
        ch->sendf("            @D---------------@RAppendages@D---------------\n");

        if (PLR_FLAGGED(ch, PLR_HEAD)) {
            ch->sendf("            @D[@cHead        @D: @GHave.          @D]@n\r\n");
        }
        if (!PLR_FLAGGED(ch, PLR_HEAD)) {
            ch->sendf("            @D[@cHead        @D: @rMissing.         @D]@n\r\n");
        }
        if (GET_LIMBCOND(ch, 0) >= 50 && !PLR_FLAGGED(ch, PLR_CRARM)) {
            ch->sendf("            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 0),
                         "%", "%");
        } else if (GET_LIMBCOND(ch, 0) > 0 && !PLR_FLAGGED(ch, PLR_CRARM)) {
            ch->sendf("            @D[@cRight Arm   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                         GET_LIMBCOND(ch, 0), "%", "%");
        } else if (GET_LIMBCOND(ch, 0) > 0 && PLR_FLAGGED(ch, PLR_CRARM)) {
            ch->sendf("            @D[@cRight Arm   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                         GET_LIMBCOND(ch, 0), "%", "%");
        } else if (GET_LIMBCOND(ch, 0) <= 0) {
            ch->sendf("            @D[@cRight Arm   @D: @rMissing.         @D]@n\r\n");
        }
        if (GET_LIMBCOND(ch, 1) >= 50 && !PLR_FLAGGED(ch, PLR_CLARM)) {
            ch->sendf("            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 1),
                         "%", "%");
        } else if (GET_LIMBCOND(ch, 1) > 0 && !PLR_FLAGGED(ch, PLR_CLARM)) {
            ch->sendf("            @D[@cLeft Arm    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                         GET_LIMBCOND(ch, 1), "%", "%");
        } else if (GET_LIMBCOND(ch, 1) > 0 && PLR_FLAGGED(ch, PLR_CLARM)) {
            ch->sendf("            @D[@cLeft Arm    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                         GET_LIMBCOND(ch, 1), "%", "%");
        } else if (GET_LIMBCOND(ch, 1) <= 0) {
            ch->sendf("            @D[@cLeft Arm    @D: @rMissing.         @D]@n\r\n");
        }
        if (GET_LIMBCOND(ch, 2) >= 50 && !PLR_FLAGGED(ch, PLR_CLARM)) {
            ch->sendf("            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 2),
                         "%", "%");
        } else if (GET_LIMBCOND(ch, 2) > 0 && !PLR_FLAGGED(ch, PLR_CRLEG)) {
            ch->sendf("            @D[@cRight Leg   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                         GET_LIMBCOND(ch, 2), "%", "%");
        } else if (GET_LIMBCOND(ch, 2) > 0 && PLR_FLAGGED(ch, PLR_CRLEG)) {
            ch->sendf("            @D[@cRight Leg   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                         GET_LIMBCOND(ch, 2), "%", "%");
        } else if (GET_LIMBCOND(ch, 2) <= 0) {
            ch->sendf("            @D[@cRight Leg   @D: @rMissing.         @D]@n\r\n");
        }
        if (GET_LIMBCOND(ch, 3) >= 50 && !PLR_FLAGGED(ch, PLR_CLLEG)) {
            ch->sendf("            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 3),
                         "%", "%");
        } else if (GET_LIMBCOND(ch, 3) > 0 && !PLR_FLAGGED(ch, PLR_CLLEG)) {
            ch->sendf("            @D[@cLeft Leg    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n",
                         GET_LIMBCOND(ch, 3), "%", "%");
        } else if (GET_LIMBCOND(ch, 3) > 0 && PLR_FLAGGED(ch, PLR_CLLEG)) {
            ch->sendf("            @D[@cLeft Leg    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n",
                         GET_LIMBCOND(ch, 3), "%", "%");
        } else if (GET_LIMBCOND(ch, 3) <= 0) {
            ch->sendf("            @D[@cLeft Leg    @D: @rMissing.         @D]@n\r\n");
        }

        if(race::hasTail(ch->race) && !PLR_FLAGGED(ch, PLR_TAILHIDE)) {
            if(PLR_FLAGGED(ch, PLR_TAIL))
                ch->sendf("            @D[@cTail        @D: @GHave.            @D]@n\r\n");
            else
                ch->sendf("            @D[@cTail        @D: @rMissing.         @D]@n\r\n");
        }

        ch->sendf("\r\n");

        ch->sendf("         @D-----------------@YHunger@D/@yThirst@D-----------------@n\r\n");
        if (GET_COND(ch, HUNGER) >= 48) {
            ch->sendf("         You are full.\r\n");
        } else if (GET_COND(ch, HUNGER) >= 40) {
            ch->sendf("         You are nearly full.\r\n");
        } else if (GET_COND(ch, HUNGER) >= 30) {
            ch->sendf("         You are not hungry.\r\n");
        } else if (GET_COND(ch, HUNGER) >= 21) {
            ch->sendf("         You wouldn't mind a snack.\r\n");
        } else if (GET_COND(ch, HUNGER) >= 15) {
            ch->sendf("         You are slightly hungry.\r\n");
        } else if (GET_COND(ch, HUNGER) >= 10) {
            ch->sendf("         You are partially hungry.\r\n");
        } else if (GET_COND(ch, HUNGER) >= 5) {
            ch->sendf("         You are really hungry.\r\n");
        } else if (GET_COND(ch, HUNGER) >= 2) {
            ch->sendf("         You are extremely hungry.\r\n");
        } else if (GET_COND(ch, HUNGER) >= 0) {
            ch->sendf("         You are starving!\r\n");
        } else if (GET_COND(ch, HUNGER) < 0) {
            ch->sendf("         You need not eat.\r\n");
        }
        if (GET_COND(ch, THIRST) >= 48) {
            ch->sendf("         You are not thirsty.\r\n");
        } else if (GET_COND(ch, THIRST) >= 40) {
            ch->sendf("         You are nearly quenched.\r\n");
        } else if (GET_COND(ch, THIRST) >= 30) {
            ch->sendf("         You are not thirsty.\r\n");
        } else if (GET_COND(ch, THIRST) >= 21) {
            ch->sendf("         You wouldn't mind a drink.\r\n");
        } else if (GET_COND(ch, THIRST) >= 15) {
            ch->sendf("         You are slightly thirsty.\r\n");
        } else if (GET_COND(ch, THIRST) >= 10) {
            ch->sendf("         You are partially thirsty.\r\n");
        } else if (GET_COND(ch, THIRST) >= 5) {
            ch->sendf("         You are really thirsty.\r\n");
        } else if (GET_COND(ch, THIRST) >= 2) {
            ch->sendf("         You are extremely thirsty.\r\n");
        } else if (GET_COND(ch, THIRST) >= 0) {
            ch->sendf("         You are dehydrated!\r\n");
        } else if (GET_COND(ch, THIRST) < 0) {
            ch->sendf("         You need not drink.\r\n");
        }
        ch->sendf("         @D--------------------@D[@GInfo@D]---------------------@n\r\n");
        trans_check(ch, ch);
        ch->sendf("         You have died %d times.\r\n", GET_DCOUNT(ch));
        if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
            ch->sendf("         You have been @rmuted@n on public channels.\r\n");
        }
        if (IN_ROOM(ch) == 9) {
            ch->sendf("         You are in punishment hell, so sad....\r\n");
        }
        if (!PRF_FLAGGED(ch, PRF_HINTS)) {
            ch->sendf("         You have hints turned off.\r\n");
        }
        if (NEWSUPDATE > GET_LPLAY(ch)) {
            ch->sendf("         Check the 'news', it has been updated recently.\r\n");
        }
        if (has_mail(GET_IDNUM(ch))) {
            ch->sendf("         Check your mail at the nearest postmaster.\r\n");
        }
        if (PRF_FLAGGED(ch, PRF_HIDE)) {
            ch->sendf("         You are hidden from who and ooc.\r\n");
        }
        if (GET_VOICE(ch) != nullptr) {
            ch->sendf("         Your voice desc: '%s'\r\n", GET_VOICE(ch));
        }
        if (GET_DISTFEA(ch) == DISTFEA_EYE) {
            ch->sendf("         Your eyes are your most distinctive feature.\r\n");
        }

        if (GET_PREFERENCE(ch) == 0) {
            ch->sendf("         You preferred a balanced form of fighting.\r\n");
        } else if (GET_PREFERENCE(ch) == PREFERENCE_KI) {
            ch->sendf("         You preferred a ki dominate form of fighting.\r\n");
        } else if (GET_PREFERENCE(ch) == PREFERENCE_WEAPON) {
            ch->sendf("         You preferred a weapon dominate form of fighting.\r\n");
        } else if (GET_PREFERENCE(ch) == PREFERENCE_H2H) {
            ch->sendf("         You preferred a body dominate form of fighting.\r\n");
        } else if (GET_PREFERENCE(ch) == PREFERENCE_THROWING) {
            ch->sendf("         You preferred a throwing dominate form of fighting.\r\n");
        }

        if (GET_DISTFEA(ch) == DISTFEA_HAIR && !IS_DEMON(ch) && !IS_MAJIN(ch) && !IS_ICER(ch) && !IS_NAMEK(ch)) {
            ch->sendf("         Your hair is your most distinctive feature.\r\n");
        } else if (GET_DISTFEA(ch) == DISTFEA_HAIR && IS_DEMON(ch)) {
            ch->sendf("         Your horns are your most distinctive feature.\r\n");
        } else if (GET_DISTFEA(ch) == DISTFEA_HAIR && IS_MAJIN(ch)) {
            ch->sendf("         Your forelock is your most distinctive feature.\r\n");
        } else if (GET_DISTFEA(ch) == DISTFEA_HAIR && IS_ICER(ch)) {
            ch->sendf("         Your horns are your most distinctive feature.\r\n");
        } else if (GET_DISTFEA(ch) == DISTFEA_HAIR && IS_NAMEK(ch)) {
            ch->sendf("         Your antennae are your most distinctive feature.\r\n");
        }

        if (GET_DISTFEA(ch) == DISTFEA_SKIN) {
            ch->sendf("         Your skin is your most distinctive feature.\r\n");
        }
        if (GET_DISTFEA(ch) == DISTFEA_HEIGHT) {
            ch->sendf("         Your height is your most distinctive feature.\r\n");
        }
        if (GET_DISTFEA(ch) == DISTFEA_WEIGHT) {
            ch->sendf("         Your weight is your most distinctive feature.\r\n");
        }

        if (GET_EQ(ch, WEAR_EYE)) {
            struct obj_data *obj = GET_EQ(ch, WEAR_EYE);
            if (SFREQ(obj) == 0) {
                SFREQ(obj) = 1;
            }
            ch->sendf("         Your scouter is on frequency @G%d@n\r\n", SFREQ(obj));
            obj = nullptr;
        }
        if (GET_CHARGE(ch) > 0) {
            ch->sendf("         You have @C%s@n ki charged.\r\n", add_commas(GET_CHARGE(ch)).c_str());
        }
        if (GET_KAIOKEN(ch) > 0) {
            ch->sendf("         You are focusing Kaioken x %d.\r\n", GET_KAIOKEN(ch));
        }
        if (AFF_FLAGGED(ch, AFF_SANCTUARY)) {
            ch->sendf("         You are surrounded by a barrier @D(@Y%s@D)@n\r\n", add_commas(GET_BARRIER(ch)).c_str());
        }
        if (AFF_FLAGGED(ch, AFF_FIRESHIELD)) {
            ch->sendf("         You are surrounded by flames!@n\r\n");
        }
        if (GET_SUPPRESS(ch) > 0) {
            ch->sendf("         You are suppressing current PL to %" I64T ".\r\n", GET_SUPPRESS(ch));
        }
        if (IS_MAJIN(ch)) {
            ch->sendf("         You have ingested %d people.\r\n", GET_ABSORBS(ch));
        }
        if (IS_BIO(ch)) {
            ch->sendf("         You have %d absorbs left.\r\n", GET_ABSORBS(ch));
        }
        ch->sendf("         You have %s colored aura.\r\n", aura_types[GET_AURA(ch)]);

        if (GET_LEVEL(ch) < 100) {
            if ((IS_ANDROID(ch) && PLR_FLAGGED(ch, PLR_ABSORB)) || (!IS_ANDROID(ch) && !IS_BIO(ch) && !IS_MAJIN(ch))) {
                ch->sendf("         @R%s@n to SC a stat this level.\r\n", add_commas(ch->calc_soft_cap()).c_str());
            } else {
                ch->sendf("         @R%s@n in PL/KI/ST combined to SC this level.\r\n",
                             add_commas(ch->calc_soft_cap()).c_str());
            }
        } else {
            ch->sendf("         Your strengths are potentially limitless.\r\n");
        }

        if (GET_FORGETING(ch) != 0) {
            ch->sendf("         @MForgetting @D[@m%s - %s@D]@n\r\n", spell_info[GET_FORGETING(ch)].name,
                         forget_level[GET_FORGET_COUNT(ch)]);
        } else {
            ch->sendf("         @MForgetting @D[@mNothing.@D]@n\r\n");
        }

        if (GET_SKILL(ch, SKILL_DAGGER) > 0) {
            if (GET_BACKSTAB_COOL(ch) > 0) {
                ch->sendf("         @yYou can't preform a backstab yet.@n\r\n");
            } else {
                ch->sendf("         @YYou can backstab.@n\r\n");
            }
        }

        if (GET_FEATURE(ch)) {
            ch->sendf("         Extra Feature: @C%s@n\r\n", GET_FEATURE(ch));
        }

        if (GET_RDISPLAY(ch)) {
            if (GET_RDISPLAY(ch) != "Empty") {
                ch->sendf("         Room Display: @C...%s@n\r\n", GET_RDISPLAY(ch));
            }
        }

        ch->sendf("\r\n@D<@b-------------------------@D[@BCondition@D]@b--------------------------@D>@n\r\n");

        if (GET_BONUS(ch, BONUS_INSOMNIAC)) {
            ch->sendf("You can not sleep.\r\n");
        } else {
            if (GET_SLEEPT(ch) > 6 && GET_POS(ch) != POS_SLEEPING) {
                ch->sendf("You are well rested.\r\n");
            } else if (GET_SLEEPT(ch) > 6 && GET_POS(ch) == POS_SLEEPING) {
                ch->sendf("You are getting the rest you need.\r\n");
            } else if (GET_SLEEPT(ch) > 4) {
                ch->sendf("You are rested.\r\n");
            } else if (GET_SLEEPT(ch) > 2) {
                ch->sendf("You are not sleepy.\r\n");
            } else if (GET_SLEEPT(ch) >= 1) {
                ch->sendf("You are getting a little sleepy.\r\n");
            } else if (GET_SLEEPT(ch) == 0) {
                ch->sendf("You could sleep at any time.\r\n");
            }
        }


        if (GET_RELAXCOUNT(ch) > 464) {
            ch->sendf(
                         "You are far too at ease to train hard like you should. Get out of the house more often.\r\n");
        } else if (GET_RELAXCOUNT(ch) > 232) {
            ch->sendf("You are too at ease to train hard like you should. Get out of the house more often.\r\n");
        } else if (GET_RELAXCOUNT(ch) > 116) {
            ch->sendf("You are a bit at ease and your training suffers. Get out of the house more often.\r\n");
        }

        if (ch->mimic) {
            ch->sendf("You are mimicing the general appearance of %s %s\r\n", AN(LRACE(ch)), LRACE(ch));
        }
        if (IS_MUTANT(ch)) {
            ch->sendf("Your Mutations:\r\n");
            if (GET_GENOME(ch, 0) == 1) {
                ch->sendf("  Extreme Speed.\r\n");
            }
            if (GET_GENOME(ch, 0) == 2) {
                ch->sendf("  Increased Cell Regeneration.\r\n");
            }
            if (GET_GENOME(ch, 0) == 3) {
                ch->sendf("  Extreme Reflexes.\r\n");
            }
            if (GET_GENOME(ch, 0) == 4) {
                ch->sendf("  Infravision.\r\n");
            }
            if (GET_GENOME(ch, 0) == 5) {
                ch->sendf("  Natural Camo.\r\n");
            }
            if (GET_GENOME(ch, 0) == 6) {
                ch->sendf("  Limb Regen.\r\n");
            }
            if (GET_GENOME(ch, 0) == 7) {
                ch->sendf("  Poisonous (you can use the bite attack).\r\n");
            }
            if (GET_GENOME(ch, 0) == 8) {
                ch->sendf("  Rubbery Body.\r\n");
            }
            if (GET_GENOME(ch, 0) == 9) {
                ch->sendf("  Innate Telepathy.\r\n");
            }
            if (GET_GENOME(ch, 0) == 10) {
                ch->sendf("  Natural Energy.\r\n");
            }
            if (GET_GENOME(ch, 1) == 1) {
                ch->sendf("  Extreme Speed.\r\n");
            }
            if (GET_GENOME(ch, 1) == 2) {
                ch->sendf("  Increased Cell Regeneration.\r\n");
            }
            if (GET_GENOME(ch, 1) == 3) {
                ch->sendf("  Extreme Reflexes.\r\n");
            }
            if (GET_GENOME(ch, 1) == 4) {
                ch->sendf("  Infravision.\r\n");
            }
            if (GET_GENOME(ch, 1) == 5) {
                ch->sendf("  Natural Camo.\r\n");
            }
            if (GET_GENOME(ch, 1) == 6) {
                ch->sendf("  Limb Regen.\r\n");
            }
            if (GET_GENOME(ch, 1) == 7) {
                ch->sendf("  Poisonous (you can use the bite attack).\r\n");
            }
            if (GET_GENOME(ch, 1) == 8) {
                ch->sendf("  Rubbery Body.\r\n");
            }
            if (GET_GENOME(ch, 1) == 9) {
                ch->sendf("  Innate Telepathy.\r\n");
            }
            if (GET_GENOME(ch, 1) == 10) {
                ch->sendf("  Natural Energy.\r\n");
            }
        }
        if (IS_BIO(ch)) {
            ch->sendf("Your genes carry:\r\n");
            for(auto i = 0; i < 2; i++) {
                std::string race;
                switch(GET_GENOME(ch, i)) {
                    case 1:
                        race = "Human";
                        break;
                    case 2:
                        race = "Saiyan";
                        break;
                    case 3:
                        race = "Namek";
                        break;
                    case 4:
                        race = "Icer";
                        break;
                    case 5:
                        race = "Truffle";
                        break;
                    case 6:
                        race = "Arlian";
                        break;
                    case 7:
                        race = "Kai";
                        break;
                    case 8:
                        race = "Konatsu";
                        break;
                    default:
                        race = "???";
                        break;
                }
                ch->sendf("  %s DNA.\r\n", race.c_str());
            }
        }
        if (GET_GENOME(ch, 0) == 11) {
            ch->sendf("You have used kyodaika.\r\n");
        }
        if (PRF_FLAGGED(ch, PRF_NOPARRY)) {
            ch->sendf("You have decided not to parry attacks.\r\n");
        }
        switch (GET_POS(ch)) {
            case POS_DEAD:
                ch->sendf("You are DEAD!\r\n");
                break;
            case POS_MORTALLYW:
                ch->sendf("You are mortally wounded! You should seek help!\r\n");
                break;
            case POS_INCAP:
                ch->sendf("You are incapacitated, slowly fading away...\r\n");
                break;
            case POS_STUNNED:
                ch->sendf("You are stunned! You can't move!\r\n");
                break;
            case POS_SLEEPING:
                ch->sendf("You are sleeping.\r\n");
                break;
            case POS_RESTING:
                ch->sendf("You are resting.\r\n");
                break;
            case POS_SITTING:
                ch->sendf("You are sitting.\r\n");
                break;
            case POS_FIGHTING:
                ch->sendf("You are fighting %s.\r\n", FIGHTING(ch) ? PERS(FIGHTING(ch), ch) : "thin air");
                break;
            case POS_STANDING:
                ch->sendf("You are standing.\r\n");
                break;
            default:
                ch->sendf("You are floating.\r\n");
                break;
        }

        if (has_group(ch)) {
            ch->sendf("@GGroup Victories@D: @w%s@n\r\n", add_commas(GET_GROUPKILLS(ch)).c_str());
        }

        if (PLR_FLAGGED(ch, PLR_EYEC)) {
            ch->sendf("Your eyes are closed.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_SNEAK)) {
            ch->sendf("You are prepared to sneak where ever you go.\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_DISGUISED)) {
            ch->sendf("You have disguised your facial features.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_FLYING)) {
            ch->sendf("You are flying.\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_PILOTING)) {
            ch->sendf("You are busy piloting a ship.\r\n");
        }
        if (GET_SONG(ch) > 0) {
            ch->sendf("You are playing @y'@Y%s@y'@n.\r\n", song_types[GET_SONG(ch)]);
        }

        if (AFF_FLAGGED(ch, AFF_ZANZOKEN)) {
            ch->sendf("You are prepared to zanzoken.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_HASS)) {
            ch->sendf("Your arms are moving fast.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_INFUSE)) {
            ch->sendf("Your ki will be infused in your next physical attack.\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_TAILHIDE)) {
            ch->sendf("Your tail is hidden!\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_NOGROW)) {
            ch->sendf("Your tail is no longer regrowing!\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_POSE)) {
            ch->sendf("You are feeling confident from your pose earlier.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_HYDROZAP)) {
            ch->sendf("You are effected by Kanso Suru.\r\n");
        }
        if (GET_COND(ch, DRUNK) > 15)
            ch->sendf("You are extremely drunk.\r\n");
        else if (GET_COND(ch, DRUNK) > 10)
            ch->sendf("You are pretty drunk.\r\n");
        else if (GET_COND(ch, DRUNK) > 4)
            ch->sendf("You are drunk.\r\n");
        else if (GET_COND(ch, DRUNK) > 0)
            ch->sendf("You have an alcoholic buzz.\r\n");

        if (ch->affected) {
            int lasttype = 0;
            for (aff = ch->affected; aff; aff = aff->next) {
                if (!strcasecmp(skill_name(aff->type), "runic") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Your Kenaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "punch") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Your Algiz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "knee") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Your Oagaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "slam") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Your Wunjo rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "heeldrop") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Your Purisaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "special beam cannon") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Your Laguz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "might") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Your muscles are pumped! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "flex") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You are more agile right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "bless") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You have been blessed! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "curse") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You have been cursed! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "healing glow") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You have a healing glow enveloping your body! (%2d Mud Hours)\r\n",
                                 aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "genius") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You are smarter right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "enlighten") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You are wiser right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "yoikominminken") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You have been lulled to sleep! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "solar flare") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You have been blinded! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "spirit control") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You have full control of your spirit! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "!UNUSED!") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You feel poison burning through your blood! (%2d Mud Hours)\r\n",
                                 aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "tough skin") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You have toughened skin right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "poison") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("You have been poisoned! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "warp pool") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Weakened State! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "dark metamorphosis") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Your Dark Metamorphosis is still in effect. (%2d Mud Hours)\r\n",
                                 aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "hayasa") && aff->type != lasttype) {
                    lasttype = aff->type;
                    ch->sendf("Your body has been infused to move faster! (%2d Mud Hours)\r\n",
                                 aff->duration + 1);
                }
            }
        }

        if (AFF_FLAGGED(ch, AFF_KNOCKED))
            ch->sendf("You have been knocked unconcious!\r\n");

        if (AFF_FLAGGED(ch, AFF_INVISIBLE))
            ch->sendf("You are invisible.\r\n");

        if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
            ch->sendf("You are sensitive to the presence of invisible things.\r\n");

        if (AFF_FLAGGED(ch, AFF_MBREAK))
            ch->sendf("Your mind has been broken!\r\n");

        if (AFF_FLAGGED(ch, AFF_WITHER))
            ch->sendf("You've been withered! You feel so weak...\r\n");

        if (AFF_FLAGGED(ch, AFF_SHOCKED))
            ch->sendf("Your mind has been shocked!\r\n");

        if (AFF_FLAGGED(ch, AFF_CHARM))
            ch->sendf("You have been charmed!\r\n");

        if (affected_by_spell(ch, SPELL_MAGE_ARMOR))
            ch->sendf("You feel protected.\r\n");

        if (AFF_FLAGGED(ch, AFF_INFRAVISION))
            ch->sendf("You can see in darkness with infravision.\r\n");

        if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
            ch->sendf("You are summonable by other players.\r\n");

        if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
            ch->sendf("You see into the hearts of others.\r\n");

        if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
            ch->sendf("You are sensitive to the magical nature of things.\r\n");

        if (AFF_FLAGGED(ch, AFF_SPIRIT))
            ch->sendf("You have died and are part of the SPIRIT world!\r\n");

        if (PRF_FLAGGED(ch, PRF_NOGIVE))
            ch->sendf("You are not accepting items being handed to you right now.\r\n");

        if (AFF_FLAGGED(ch, AFF_ETHEREAL))
            ch->sendf("You are ethereal and cannot interact with normal space!\r\n");

        if (GET_REGEN(ch) > 0) {
            ch->sendf("Something is augmenting your regen rate by %s%d%s!\r\n", GET_REGEN(ch) > 0 ? "+" : "-",
                         GET_REGEN(ch), "%");
        }

        if (GET_ASB(ch) > 0) {
            ch->sendf("Something is augmenting your auto-skill training rate by %s%d%s!\r\n",
                         GET_ASB(ch) > 0 ? "+" : "-", GET_ASB(ch), "%");
        }

        if (ch->lifebonus > 0) {
            ch->sendf("Something is augmenting your Life Force Max by %s%d%s!\r\n",
                         ch->lifebonus > 0 ? "+" : "-", ch->lifebonus, "%");
        }

        if (PLR_FLAGGED(ch, PLR_FISHING))
            ch->sendf("Current Fishing Pole Bonus @D[@C%d@D]@n\r\n", GET_POLE_BONUS(ch));

        if (PLR_FLAGGED(ch, PLR_AURALIGHT))
            ch->sendf("Aura Light is active.\r\n");
        ch->sendf("@D<@b--------------------------------------------------------------@D>@n\r\n");
        ch->sendf("To view your bonus/negative traits enter: status traits\r\n");
    } else if (!strcasecmp(arg, "traits")) {
        bonus_status(ch);
    } else {
        ch->sendf(
                     "The only argument status takes is 'traits'. If you just want your status do not use an argument.\r\n");
    }
}

const char *list_bonuses[] = {
        "Thrifty     - -10% Shop Buy Cost and +10% Shop Sell Cost          ", /* Bonus 0 */
        "Prodigy     - +25% Experience Gained Until Level 80               ", /* Bonus 1 */
        "Quick Study - Character auto-trains skills faster                 ", /* Bonus 2 */
        "Die Hard    - Life Force's PL regen doubled, but cost is the same ", /* Bonus 3 */
        "Brawler     - Physical attacks do 20% more damage                 ", /* Bonus 4 */
        "Destroyer   - Damaged Rooms act as regen rooms for you            ", /* Bonus 5 */
        "Hard Worker - Physical activity bonuses + drains less stamina     ", /* Bonus 6 */
        "Healer      - Heal/First-aid/Vigor/Repair restore +10%            ", /* Bonus 7 */
        "Loyal       - +20% Experience When Grouped As Follower            ", /* Bonus 8 */
        "Brawny      - Strength gains +2 every 10 levels, Train STR + 75%  ", /* Bonus 9 */
        "Scholarly   - Intelligence gains +2 every 10 levels, Train INT + 75%", /* Bonus 10 */
        "Sage        - Wisdom gains +2 every 10 levels, Train WIS + 75%    ", /* Bonus 11 */
        "Agile       - Agility gains +2 every 10 levels, Train AGL + 75%   ", /* Bonus 12 */
        "Quick       - Speed gains +2 every 10 levels, Train SPD + 75%     ", /* Bonus 13 */
        "Sturdy      - Constitution +2 every 10 levels, Train CON + 75%    ", /* Bonus 14 */
        "Thick Skin  - -20% Physical and -10% ki dmg received              ", /* Bonus 15 */
        "Recipe Int. - Food cooked by you lasts longer/heals better        ", /* Bonus 16 */
        "Fireproof   - -50% Fire Dmg taken, -10% ki, immunity to burn      ", /* Bonus 17 */
        "Powerhitter - 15% critical hits will be x4 instead of x2          ", /* Bonus 18 */
        "Healthy     - 40% chance to recover from ill effects when sleeping", /* Bonus  19 */
        "Insomniac   - Can't Sleep. Immune to yoikominminken and paralysis ", /* Bonus  20 */
        "Evasive     - +15% to dodge rolls                                 ", /* Bonus  21 */
        "The Wall    - +20% chance to block                                ", /* Bonus  22 */
        "Accurate    - +20% chance to hit physical, +10% to hit with ki     ", /* Bonus  23 */
        "Energy Leech- -2% ki damage received for every 5 character levels,\n                  @cas long as you can take that ki to your charge pool.@D        ", /* Bonus  24*/
        "Good Memory - +2 Skill Slots initially, +1 every 20 levels after  ", /* Bonus 25 */
        "Soft Touch  - Half damage for all hit locations                   ", /* Neg 26 */
        "Late Sleeper- Can only wake automatically. 33% every hour if maxed", /* Neg 27 */
        "Impulse Shop- +25% shop costs                                     ", /* Neg 28 */
        "Sickly      - Suffer from harmful effects longer                  ", /* Neg 29 */
        "Punching Bag- -15% to dodge rolls                                 ", /* Neg 30 */
        "Pushover    - -20% block chance                                   ", /* Neg 31 */
        "Poor D. Perc- -20% chance to hit with physical, -10% with ki       ", /* Neg 32 */
        "Thin Skin   - +20% physical and +10% ki damage received           ", /* Neg 33 */
        "Fireprone   - +50% Fire Dmg taken, +10% ki, always burned         ", /* Neg 34 */
        "Energy Int. - +2% ki damage received for every 5 character levels,\n                  @rif you have ki charged you have 10% chance to lose   \n                  it and to take 1/4th damage equal to it.@D                    ", /* Neg 35 */
        "Coward      - Can't Attack Enemy With 150% Your Powerlevel        ", /* Neg 36 */
        "Arrogant    - Cannot Suppress                                     ", /* Neg 37 */
        "Unfocused   - Charge concentration randomly breaks                ", /* Neg 38 */
        "Slacker     - Physical activity drains more stamina               ", /* Neg 39 */
        "Slow Learner- Character auto-trains skills slower                 ", /* Neg 40 */
        "Masochistic - Defense Skills Cap At 75                            ", /* Neg 41 */
        "Mute        - Can't use IC speech related commands                ", /* Neg 42 */
        "Wimp        - Strength is capped at 45                            ", /* Neg 43 */
        "Dull        - Intelligence is capped at 45                        ", /* Neg 44 */
        "Foolish     - Wisdom is capped at 45                              ", /* Neg 45 */
        "Clumsy      - Agility is capped at 45                             ", /* Neg 46 */
        "Slow        - Speed is capped at 45                               ", /* Neg 47 */
        "Frail       - Constitution capped at 45                           ", /* Neg 48 */
        "Sadistic    - Half Experience Gained For Quick Kills              ", /* Neg 49 */
        "Loner       - Can't Group with anyone, +5% train and +10% Phys    ", /* Neg 50 */
        "Bad Memory  - -5 Skill Slots                                      "  /* Neg 51 */
};

/* Display What Bonuses/Negatives Player Has */
static void bonus_status(struct char_data *ch) {
    int i, max = 52, count = 0;

    if (IS_NPC(ch))
        return;

    ch->sendf("@CYour Traits@n\n@D-----------------------------@w\n");
    for (i = 0; i < max; i++) {
        if (i < 26) {
            if (GET_BONUS(ch, i)) {
                ch->sendf("@c%s@n\n", list_bonuses[i]);
                count++;
            }
        } else {
            if (i == 26) {
                ch->sendf("\r\n");
            }
            if (GET_BONUS(ch, i)) {
                ch->sendf("@r%s@n\n", list_bonuses[i]);
                count++;
            }
        }
    }
    if (count <= 0) {
        ch->sendf("@wNone.\r\n");
    }
    ch->sendf("@D-----------------------------@n\r\n");
    return;
}

ACMD(do_inventory) {
    ch->sendf("@w              @YInventory\r\n@D-------------------------------------@w\r\n");
    if (!IS_NPC(ch)) {
        if (PLR_FLAGGED(ch, PLR_STOLEN)) {
            ch->clearFlag(FlagType::PC, PLR_STOLEN);
            ch->sendf("@r   --------------------------------------------------@n\n");
            ch->sendf("@R    You notice that you have been robbed sometime recently!\n");
            ch->sendf("@r   --------------------------------------------------@n\n");
            return;
        }
    }
    list_obj_to_char(ch->getInventory(), ch, SHOW_OBJ_SHORT, true);
    ch->sendf("\n");
}

ACMD(do_equipment) {
    ch->sendf("        @YEquipment Being Worn\r\n@D-------------------------------------@w\r\n");
    for (auto i = 1; i < NUM_WEARS; i++) {
        if (auto eq = GET_EQ(ch, i); eq) {
            if(CAN_SEE_OBJ(ch, eq)) {
                if ((i != WEAR_WIELD1 && i != WEAR_WIELD2) || (!PLR_FLAGGED(ch, PLR_THANDW))) {
                    ch->sendf("%s", wear_where[i]);
                    show_obj_to_char(GET_EQ(ch, i), ch, SHOW_OBJ_SHORT);

                    if (OBJ_FLAGGED(GET_EQ(ch, i), ITEM_SHEATH)) {
                        for (auto obj2 : eq->getInventory()) {
                            ch->sendf("@D  ---- @YSheathed@D ----@c> @n");
                            show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
                        }
                    }
                } else if ((PLR_FLAGGED(ch, PLR_THANDW))) {
                    ch->sendf("@c<@CWielded by B. Hands@c>@n ");
                    show_obj_to_char(GET_EQ(ch, i), ch, SHOW_OBJ_SHORT);
                }
            }
            else {
                ch->sendf("%s", wear_where[i]);
                ch->sendf("Something.\r\n");
            }
        } else {
            if (BODY_FLAGGED(ch, i) && (i != WEAR_WIELD2)) {
                ch->sendf("%s@wNothing.@n\r\n", wear_where[i]);
            } else if (BODY_FLAGGED(ch, i) && (i == WEAR_WIELD2 && !PLR_FLAGGED(ch, PLR_THANDW))) {
                ch->sendf("%s@wNothing.@n\r\n", wear_where[i]);
            }
        }
    }
}

ACMD(do_time) {
    const char *suf;
    int weekday, day;

    /* day in [1..30] */
    day = time_info.day + 1;

    /* 30 days in a month, 6 days a week */
    weekday = day % 6;

    ch->sendf("It is %02d:%02d:%02d o'clock %s, on %s.\r\n",
                 (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12), time_info.minutes, time_info.seconds,
                 time_info.hours >= 12 ? "PM" : "AM",
                 weekdays[weekday]);

    /*
   * Peter Ajamian <peter@PAJAMIAN.DHS.ORG> supplied the following as a fix
   * for a bug introduced in the ordinal display that caused 11, 12, and 13
   * to be incorrectly displayed as 11st, 12nd, and 13rd.  Nate Winters
   * <wintersn@HOTMAIL.COM> had already submitted a fix, but it hard-coded a
   * limit on ordinal display which I want to avoid.	-dak
   */

    suf = "th";

    if (((day % 100) / 10) != 1) {
        switch (day % 10) {
            case 1:
                suf = "st";
                break;
            case 2:
                suf = "nd";
                break;
            case 3:
                suf = "rd";
                break;
        }
    }

    ch->sendf("The %d%s Day of the %s, Year %d.\r\n",
                 day, suf, month_name[time_info.month], time_info.year);
}

ACMD(do_weather) {
    const char *sky_look[] = {
            "cloudless",
            "cloudy",
            "rainy",
            "lit by flashes of lightning"
    };

    if (OUTSIDE(ch)) {
        ch->sendf("The sky is %s and %s.\r\n", sky_look[weather_info.sky],
                     weather_info.change >= 0 ? "you feel a warm wind from south" :
                     "your foot tells you bad weather is due");
        if (ADM_FLAGGED(ch, ADM_KNOWWEATHER))
            ch->sendf("Pressure: %d (change: %d), Sky: %d (%s)\r\n",
                         weather_info.pressure,
                         weather_info.change,
                         weather_info.sky,
                         sky_look[weather_info.sky]);
    } else
        ch->sendf("You have no feeling about the weather at all.\r\n");
}

/* puts -'s instead of spaces */
static void space_to_minus(char *str) {
    while ((str = strchr(str, ' ')) != nullptr)
        *str = '-';
}

int search_help(const char *argument, int level) {
    int chk, bot, top, mid, minlen;

    bot = 0;
    top = top_of_helpt;
    minlen = strlen(argument);

    while (bot <= top) {
        mid = (bot + top) / 2;

        if (!(chk = strncasecmp(argument, help_table[mid].keywords, minlen))) {
            while ((mid > 0) && !strncasecmp(argument, help_table[mid - 1].keywords, minlen))
                mid--;

            while (level < help_table[mid].min_level && mid < (bot + top) / 2)
                mid++;

            if (strncasecmp(argument, help_table[mid].keywords, minlen))
                break;

            return mid;
        } else if (chk > 0)
            bot = mid + 1;
        else
            top = mid - 1;
    }
    return NOWHERE;
}

ACMD(do_help) {
    char buf[MAX_STRING_LENGTH * 4];
    int mid = 0;

    if (!ch->desc)
        return;

    skip_spaces(&argument);

    if (!help_table) {
        ch->sendf("No help available.\r\n");
        return;
    }

    if (!*argument) {
        if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
            write_to_output(ch->desc, help);
        }
        else {
            write_to_output(ch->desc, ihelp);
        }
        return;
    }

    space_to_minus(argument);

    if ((mid = search_help(argument, GET_ADMLEVEL(ch))) == NOWHERE) {
        int i, found = 0;
        ch->sendf("There is no help on that word.\r\n");
        if (GET_ADMLEVEL(ch) < 3) {
            mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), true, "%s tried to get help on %s", GET_NAME(ch),
                   argument);
        }
        for (i = 0; i <= top_of_helpt; i++) {
            if (help_table[i].min_level > GET_ADMLEVEL(ch))
                continue;
            /* To help narrow down results, if they don't start with the same letters, move on */
            if (*argument != *help_table[i].keywords)
                continue;
            if (levenshtein_distance(argument, help_table[i].keywords) <= 2) {
                if (!found) {
                    ch->sendf("\r\nDid you mean:\r\n");
                    found = 1;
                }
                ch->sendf("  %s\r\n", help_table[i].keywords);
            }
        }
        return;
    }
    if (help_table[mid].min_level > GET_ADMLEVEL(ch)) {
        ch->sendf("There is no help on that word.\r\n");
        return;
    }
    sprintf(buf, "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
    sprintf(buf + strlen(buf), "%s", help_table[mid].entry);
    sprintf(buf + strlen(buf), "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
    if (GET_ADMLEVEL(ch) > 0) {
        sprintf(buf + strlen(buf), "@WHelp File Level@w: @D(@R%d@D)@n\n", help_table[mid].min_level);
    }
    write_to_output(ch->desc, "%s", buf);
}

#define WHO_FORMAT \
"Usage: who [minlev[-maxlev]] [-k] [-n name] [-q] [-r] [-s] [-z]\r\n"

/* Written by Rhade */
ACMD(do_who) {
    struct descriptor_data *d;
    struct char_data *tch;
    int i, num_can_see = 0;
    char name_search[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    int low = 0, high = CONFIG_LEVEL_CAP, localwho = 0, questwho = 0, hide = 0;
    int showclass = 0, short_list = 0, outlaws = 0;
    int who_room = 0, showgroup = 0, showleader = 0;
    char *line_color = "@n";

    skip_spaces(&argument);
    strcpy(buf, argument);    /* strcpy: OK (sizeof: argument == buf) */
    name_search[0] = '\0';

    struct {
        char *disp;
        int min_level;
        int max_level;
        int count; /* must always start as 0 */
    } rank[] = {
            {"\r\n               @c------------  @D[    @gI@Gm@Wm@Do@Gr@Dt@Wa@Gl@gs   @D]  @c------------@n\r\n", ADMLVL_IMMORT, ADMLVL_IMPL, 0},
            {"\r\n@D[@wx@D]@yxxxxxxxxxx@W  [    @GImmortals   @W]  @yxxxxxxxxxx@D[@wx@D]@n\r\n",                  ADMLVL_IMMORT +
                                                                                                                  8,                          ADMLVL_GRGOD +
                                                                                                                                              8, 0},
            {"\r\n               @c------------  @D[     @DM@ro@Rr@wt@Ra@rl@Ds    ]  @c------------@n\r\n", 0,                   ADMLVL_IMMORT -
                                                                                                                                 1, 0}
            /*{ "\r\n@GAdministrators@n\r\n\r\n", ADMLVL_GRGOD, ADMLVL_IMPL, 0},
    { "\r\n@GImmortals@n\r\n\r\n"     , ADMLVL_IMMORT, ADMLVL_GRGOD - 1, 0},
    { "\r\n@GMortal@n\r\n\r\n"        , 0, ADMLVL_IMMORT - 1, 0 }*/
    };
    char *tmstr;
    tmstr = (char *) asctime(localtime(&PCOUNTDATE));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    int num_ranks = sizeof(rank) / sizeof(rank[0]);
    ch->sendf(
                 "\r\n      @r{@b===============  @D[  @DD@wr@ca@Cg@Y(@R*@Y)@Wn@cB@Da@cl@Cl @DA@wd@cv@Ce@Wnt @DT@wr@cu@Ct@Wh@n  @D]  @b===============@r}      @n\r\n");
    for (d = descriptor_list; d && !short_list; d = d->next) {
        if (!IS_PLAYING(d))
            continue;
        if (d->original)
            tch = d->original;
        else if (!(tch = d->character))
            continue;

        if (GET_ADMLEVEL(tch) >= ADMLVL_IMMORT)
            line_color = "@w";
        else
            line_color = "@w";

        if (CAN_SEE(ch, tch) && IS_PLAYING(d)) {
            if (*name_search && strcasecmp(GET_NAME(tch), name_search) &&
                !strstr(GET_TITLE(tch), name_search))
                continue;
            if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
                continue;
            if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF))
                continue;
            if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
                continue;
            if (localwho && ch->getRoom()->zone != tch->getRoom()->zone)
                continue;
            if (PRF_FLAGGED(tch, PRF_HIDE) && tch != ch && GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
                hide += 1;
                continue;
            }
            if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
                continue;
            if (showgroup && (!tch->master || !AFF_FLAGGED(tch, AFF_GROUP)))
                continue;
            for (i = 0; i < num_ranks; i++)
                if (GET_ADMLEVEL(tch) >= rank[i].min_level && GET_ADMLEVEL(tch) <= rank[i].max_level)
                    rank[i].count++;
        }
    }

    for (i = 0; i < num_ranks; i++) {
        if (!rank[i].count && !short_list)
            continue;

        if (short_list)
            ch->sendf("Players\r\n-------\r\n");
        else
            ch->sendf(rank[i].disp);

        for (d = descriptor_list; d; d = d->next) {
            if (!IS_PLAYING(d))
                continue;
            if (d->original)
                tch = d->original;
            else if (!(tch = d->character))
                continue;

            if ((GET_ADMLEVEL(tch) < rank[i].min_level || GET_ADMLEVEL(tch) > rank[i].max_level) && !short_list)
                continue;
            if (!IS_PLAYING(d))
                continue;
            if (*name_search && strcasecmp(GET_NAME(tch), name_search) &&
                !strstr(GET_TITLE(tch), name_search))
                continue;
            if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
                continue;
            if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF))
                continue;
            if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
                continue;
            if (localwho && ch->getRoom()->zone != tch->getRoom()->zone)
                continue;
            if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
                continue;
            if (PRF_FLAGGED(tch, PRF_HIDE) && tch != ch && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
                continue;
            if (showgroup && (!tch->master || !AFF_FLAGGED(tch, AFF_GROUP)))
                continue;
            if (showleader && (!tch->followers || !AFF_FLAGGED(tch, AFF_GROUP)))
                continue;

            if (short_list) {
                ch->sendf("               @B[@W%3d @Y%s @C%s@B]@W %-12.12s@n%s@n",
                             GET_LEVEL(tch), race::getAbbr(tch->race), sensei::getAbbr(tch->chclass).c_str(), GET_NAME(tch),
                             ((!(++num_can_see % 4)) ? "\r\n" : ""));
            } else {
                num_can_see++;

                char usr[100];
                sprintf(usr, "@W(@R%s@W)%s", tch->desc->account->name.c_str(),
                        PLR_FLAGGED(tch, PLR_BIOGR) ? "" : (SPOILED(tch) ? " @R*@n" : ""));
                ch->sendf("%s               @D<@C%-12s@D> %s@w%s", line_color,
                             GET_ADMLEVEL(ch) > 0 ? GET_NAME(tch) : (GET_ADMLEVEL(tch) > 0 ? GET_NAME(tch) : (GET_USER(
                                                                                                                      tch)
                                                                                                              ? GET_USER(
                                                                                                                      tch)
                                                                                                              : "nullptr")),
                             GET_ADMLEVEL(ch) > 0 ? usr : "", line_color);

                if (GET_ADMLEVEL(tch)) {
                    ch->sendf(" (%s)", admin_level_names[GET_ADMLEVEL(tch)]);
                }

                if (d->snooping && d->snooping->character != ch && GET_ADMLEVEL(ch) >= 3)
                    ch->sendf(" (Snoop: %s)", GET_NAME(d->snooping->character));
                if (GET_INVIS_LEV(tch))
                    ch->sendf(" (i%d)", GET_INVIS_LEV(tch));
                else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
                    ch->sendf(" (invis)");

                if (PLR_FLAGGED(tch, PLR_MAILING))
                    ch->sendf(" (mailing)");
                else if (d->olc)
                    ch->sendf(" (OLC)");
                else if (PLR_FLAGGED(tch, PLR_WRITING))
                    ch->sendf(" (writing)");

                if (d->original)
                    ch->sendf(" (out of body)");

                if (d->connected == CON_OEDIT)
                    ch->sendf(" (O Edit)");
                if (d->connected == CON_MEDIT)
                    ch->sendf(" (M Edit)");
                if (d->connected == CON_ZEDIT)
                    ch->sendf(" (Z Edit)");
                if (d->connected == CON_SEDIT)
                    ch->sendf(" (S Edit)");
                if (d->connected == CON_REDIT)
                    ch->sendf(" (R Edit)");
                if (d->connected == CON_TEDIT)
                    ch->sendf(" (T Edit)");
                if (d->connected == CON_TRIGEDIT)
                    ch->sendf(" (T Edit)");
                if (d->connected == CON_AEDIT)
                    ch->sendf(" (S Edit)");
                if (d->connected == CON_CEDIT)
                    ch->sendf(" (C Edit)");
                if (d->connected == CON_HEDIT)
                    ch->sendf(" (H Edit)");
                if (PRF_FLAGGED(tch, PRF_DEAF))
                    ch->sendf(" (DEAF)");
                if (PRF_FLAGGED(tch, PRF_NOTELL))
                    ch->sendf(" (NO TELL)");
                if (PRF_FLAGGED(tch, PRF_NOGOSS))
                    ch->sendf(" (NO OOC)");
                if (PLR_FLAGGED(tch, PLR_NOSHOUT))
                    ch->sendf(" (MUTED)");
                if (PRF_FLAGGED(tch, PRF_HIDE))
                    ch->sendf(" (WH)");
                if (PRF_FLAGGED(tch, PRF_BUILDWALK))
                    ch->sendf(" (Buildwalking)");
                if (PRF_FLAGGED(tch, PRF_AFK))
                    ch->sendf(" (AFK)");
                if (PLR_FLAGGED(tch, PLR_FISHING) && GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
                    ch->sendf(" (@BFISHING@n)");
                if (PRF_FLAGGED(tch, PRF_NOWIZ))
                    ch->sendf(" (NO WIZ)");
                ch->sendf("@n\r\n");
            }
        }
        ch->sendf("\r\n");
        if (short_list)
            break;
    }

    if (!num_can_see)
        ch->sendf("                            Nobody at all!\r\n");
    else if (num_can_see == 1)
        ch->sendf("                         One lonely character displayed.\r\n");
    else {
        ch->sendf("                           @Y%d@w characters displayed.\r\n", num_can_see);
        if (hide > 0) {
            int bam = false;
            if (hide > 1) {
                bam = true;
            }
            ch->sendf("                           and @Y%d@w character%s hidden.\r\n", hide, bam ? "s" : "");
        }
    }
    if (circle_restrict > 0 && circle_restrict <= 100) {
        ch->sendf("                      @rThe mud has been wizlocked to lvl %d@n\r\n", circle_restrict);
    }
    if (circle_restrict == 101) {
        ch->sendf("                      @rThe mud has been wizlocked to IMMs only.@n\r\n");
    }
    ch->sendf("      @r{@b=================================================================@r}@n\r\n");
    ch->sendf("           @cHighest Logon Count Ever@D: @Y%d@w, on %s\r\n", HIGHPCOUNT, tmstr);
    ch->sendf("                        @cHighest Logon Count Today@D: @Y%d@n\r\n", PCOUNT);
}

#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p]\r\n"

/* BIG OL' FIXME: Rewrite it all. Similar to do_who(). */
ACMD(do_users) {
    char line[200], line2[220], idletime[10];
    char state[30], *timeptr, mode;
    char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
    struct char_data *tch;
    struct descriptor_data *d;
    int low = 0, high = CONFIG_LEVEL_CAP, num_can_see = 0;
    int showclass = 0, outlaws = 0, playing = 0, deadweight = 0, showrace = 0;
    char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

    host_search[0] = name_search[0] = '\0';

    strcpy(buf, argument);    /* strcpy: OK (sizeof: argument == buf) */
    while (*buf) {
        char buf1[MAX_INPUT_LENGTH];

        half_chop(buf, arg, buf1);
        if (*arg == '-') {
            mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
            switch (mode) {
                case 'o':
                case 'k':
                    outlaws = 1;
                    playing = 1;
                    strcpy(buf, buf1);    /* strcpy: OK (sizeof: buf1 == buf) */
                    break;
                case 'p':
                    playing = 1;
                    strcpy(buf, buf1);    /* strcpy: OK (sizeof: buf1 == buf) */
                    break;
                case 'd':
                    deadweight = 1;
                    strcpy(buf, buf1);    /* strcpy: OK (sizeof: buf1 == buf) */
                    break;
                case 'l':
                    playing = 1;
                    half_chop(buf1, arg, buf);
                    sscanf(arg, "%d-%d", &low, &high);
                    break;
                case 'n':
                    playing = 1;
                    half_chop(buf1, name_search, buf);
                    break;
                case 'h':
                    playing = 1;
                    half_chop(buf1, host_search, buf);
                    break;
                default:
                    ch->sendf("%s", USERS_FORMAT);
                    return;
            }                /* end of switch */

        } else {            /* endif */
            ch->sendf("%s", USERS_FORMAT);
            return;
        }
    }                /* end while (parser) */
    ch->sendf(
                 "Num Name                 User-name            State          Idl Login    C\r\n"
                 "--- -------------------- -------------------- -------------- --- -------- -\r\n");

    one_argument(argument, arg);

    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING && playing)
            continue;
        if (STATE(d) == CON_PLAYING && deadweight)
            continue;
        if (IS_PLAYING(d)) {
            if (d->original)
                tch = d->original;
            else if (!(tch = d->character))
                continue;

            if (*host_search && !strstr(d->host, host_search))
                continue;
            if (*name_search && strcasecmp(GET_NAME(tch), name_search))
                continue;
            if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
                continue;
            if (PRF_FLAGGED(tch, PRF_HIDE) && tch != ch && GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
                continue;
            }
            if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
                !PLR_FLAGGED(tch, PLR_THIEF))
                continue;
            if (GET_INVIS_LEV(tch) > GET_ADMLEVEL(ch))
                continue;
        }

        timeptr = asctime(localtime(&d->login_time));
        timeptr += 11;
        *(timeptr + 8) = '\0';

        if (STATE(d) == CON_PLAYING && d->original)
            strcpy(state, "Switched");
        else
            strcpy(state, connected_types[STATE(d)]);

        if (d->character && STATE(d) == CON_PLAYING && GET_ADMLEVEL(d->character) <= GET_ADMLEVEL(ch))
            sprintf(idletime, "%3d", d->character->timer *
                                     SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
        else
            strcpy(idletime, "");

        sprintf(line, "%3d %-20s %-20s %-14s %-3s %-8s %1s ", -1,
                d->original && d->original->name ? d->original->name :
                d->character && d->character->name ? d->character->name :
                "UNDEFINED", d->account ? d->account->name.c_str() : "UNKNOWN", state, idletime, timeptr,
                "N");

        if (d->host && *d->host)
            sprintf(line + strlen(line), "\n%3d [%s Site: %s]\r\n", -1, d->account ? d->account->name.c_str() : "UNKNOWN",
                    d->host);
        else
            sprintf(line + strlen(line), "\n%3d [%s Site: Hostname unknown]\r\n", -1,
                    d->account ? d->account->name.c_str() : "UNKNOWN");

        if (STATE(d) != CON_PLAYING) {
            sprintf(line2, "@g%s@n", line);
            strcpy(line, line2);
        }
        if (STATE(d) != CON_PLAYING ||
            (STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
            ch->sendf("%s", line);
            num_can_see++;
        }
    }

    ch->sendf("\r\n%d visible sockets connected.\r\n", num_can_see);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps) {
    char arg[MAX_INPUT_LENGTH];
    char bum[10000];
    one_argument(argument, arg);

    switch (subcmd) {
        case SCMD_CREDITS: {
            write_to_output(ch->desc, credits);
        }
            break;
        case SCMD_NEWS: {
            write_to_output(ch->desc, news);
        }
            GET_LPLAY(ch) = time(nullptr);
            break;
        case SCMD_INFO: {
            write_to_output(ch->desc, info);
        }
            break;
        case SCMD_WIZLIST: {
            write_to_output(ch->desc, wizlist);
        }
            break;
        case SCMD_IMMLIST: {
            write_to_output(ch->desc, immlist);
        }
            break;
        case SCMD_HANDBOOK: {
            write_to_output(ch->desc, handbook);
        }
            break;
        case SCMD_POLICIES:
            sprintf(bum, "--------------------\r\n%s\r\n--------------------\r\n", policies);
            write_to_output(ch->desc, bum);
            break;
        case SCMD_MOTD: {
            write_to_output(ch->desc, motd);
        }
            break;
        case SCMD_IMOTD: {
            write_to_output(ch->desc, imotd);
        }
            break;
        case SCMD_CLEAR:
            ch->sendf("\033[H\033[J");
            break;
        case SCMD_VERSION:
            ch->sendf("%s\r\n", circlemud_version);
            ch->sendf("%s\r\n", oasisolc_version);
            ch->sendf("%s\r\n", DG_SCRIPT_VERSION);
            ch->sendf("%s\r\n", CWG_VERSION);
            ch->sendf("%s\r\n", DBAT_VERSION);
            break;
        case SCMD_WHOAMI:
            ch->sendf("%s\r\n", GET_NAME(ch));
            break;
        default:
            basic_mud_log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
            /*  SYSERR_DESC:
     *  General page string function for such things as 'credits', 'news',
     *  'wizlist', 'clear', 'version'.  This occurs when a call is made to
     *  this routine that is not one of the predefined calls.  To correct
     *  it, either a case needs to be added into the function to account for
     *  the subcmd that is being passed to it, or the call to the function
     *  needs to have the correct subcmd put into place.
     */
            return;
    }
}

static void perform_mortal_where(struct char_data *ch, char *arg) {
    struct char_data *i;
    struct descriptor_data *d;

    if (!*arg) {
        ch->sendf("Players in your Zone\r\n--------------------\r\n");
        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) != CON_PLAYING || d->character == ch)
                continue;
            if ((i = (d->original ? d->original : d->character)) == nullptr)
                continue;
            if (IN_ROOM(i) == NOWHERE || !CAN_SEE(ch, i))
                continue;
            if (ch->getRoom()->zone != i->getRoom()->zone)
                continue;
            ch->sendf("%-20s - %s\r\n", GET_NAME(i), i->getRoom()->name);
        }
    } else {            /* print only FIRST char, not all. */
        for (i = character_list; i; i = i->next) {
            if (IN_ROOM(i) == NOWHERE || i == ch)
                continue;
            if (!CAN_SEE(ch, i) || i->getRoom()->zone != ch->getRoom()->zone)
                continue;
            if (!isname(arg, i->name))
                continue;
            ch->sendf("%-25s - %s\r\n", GET_NAME(i), i->getRoom()->name);
            return;
        }
        ch->sendf("Nobody around by that name.\r\n");
    }
}

static void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
                                  int recur) {
    if (num > 0)
        ch->sendf("O%3d. %-25s - ", num, obj->getShortDesc());
    else
        ch->sendf("%33s", " - ");

    ch->sendf("%s", obj->scriptString().c_str());

    if (IN_ROOM(obj) != NOWHERE)
        ch->sendf("[%5d] %s\r\n", GET_ROOM_VNUM(IN_ROOM(obj)), obj->getRoom()->name);
    else if (obj->carried_by)
        ch->sendf("carried by %s in room [%d]\r\n", PERS(obj->carried_by, ch),
                     GET_ROOM_VNUM(IN_ROOM(obj->carried_by)));
    else if (obj->worn_by)
        ch->sendf("worn by %s in room [%d]\r\n", PERS(obj->worn_by, ch), GET_ROOM_VNUM(IN_ROOM(obj->worn_by)));
    else if (obj->in_obj) {
        ch->sendf("inside %s%s\r\n", obj->in_obj->getShortDesc(), (recur ? ", which is" : " "));
        if (recur)
            print_object_location(0, obj->in_obj, ch, recur);
    } else
        ch->sendf("in an unknown location\r\n");
}

static void perform_immort_where(struct char_data *ch, char *arg) {
    struct char_data *i;
    struct obj_data *k;
    int num = 0, num2 = 0, found = 0;

    if (!*arg) {
        mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true,
               "GODCMD: %s has checked where to check player locations", GET_NAME(ch));
        ch->sendf(
                     "Players                  Vnum    Planet        Location\r\n-------                 ------   ----------    ----------------\r\n");
        for (auto d = descriptor_list; d; d = d->next) {
            if(!IS_PLAYING(d)) continue;
            auto loc = d->character->getLocation();
            if(!loc) continue;
            if(!CAN_SEE(ch, d->character)) continue;
            auto reg = loc->getRegion();

            ch->sendf("%-20s - [%5d]   %-14s %s\r\n", GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(i)),
                            reg ? reg->getDisplayName(ch) : "Unknown", loc->getDisplayName(ch));
            
        }
    } else {
        mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true, "GODCMD: %s has checked where for the location of %s",
               GET_NAME(ch), arg);
        for (i = character_list; i; i = i->next) {
            if (CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE && isname(arg, i->name)) {
                found = 1;
                ch->sendf("M%3d. %-25s - [%5d] %-25s", ++num, GET_NAME(i),
                             GET_ROOM_VNUM(IN_ROOM(i)), i->getRoom()->name);
                if (IS_NPC(i) && !i->script->dgScripts.empty()) {
                    auto t = i->scriptString();
                    ch->sendf("%s ", t.c_str());
                }
                ch->sendf("\r\n");
            }
        }
        for (k = object_list; k; k = k->next)
            if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
                found = 1;
                print_object_location(++num, k, ch, true);
            }
        if (!found) {
            ch->sendf("Couldn't find any such thing.\r\n");
        } else {
            ch->sendf("\r\nFound %d matches.\r\n", num);
        }
    }
}

ACMD(do_where) {
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (ADM_FLAGGED(ch, ADM_FULLWHERE) || GET_ADMLEVEL(ch) > 4)
        perform_immort_where(ch, arg);
    else
        perform_mortal_where(ch, arg);
}

ACMD(do_levels) {
    char buf[MAX_STRING_LENGTH];
    size_t i, len = 0, nlen;

    if (IS_NPC(ch)) {
        ch->sendf("You ain't nothin' but a hound-dog.\r\n");
        return;
    }

    for (i = 1; i < 101; i++) {
        if (i == 100)
            nlen = snprintf(buf + len, sizeof(buf) - len, "[100] %8s          : \r\n", add_commas(level_exp(ch, 100)).c_str());
        else
            nlen = snprintf(buf + len, sizeof(buf) - len, "[%2" SZT "] %8s-%-8s : \r\n", i,
                            add_commas(level_exp(ch, i)).c_str(), add_commas(level_exp(ch, i + 1) - 1).c_str());
        if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
        len += nlen;
    }

    write_to_output(ch->desc, buf);
}

ACMD(do_consider) {
    char buf[MAX_INPUT_LENGTH];
    struct char_data *victim;
    int diff;

    one_argument(argument, buf);

    if (!(victim = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM))) {
        ch->sendf("Consider killing who?\r\n");
        return;
    }
    if (victim == ch) {
        ch->sendf("Easy!  Very easy indeed!\r\n");
        return;
    }
    diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

    if (diff <= -10)
        ch->sendf("Now where did that chicken go?\r\n");
    else if (diff <= -5)
        ch->sendf("You could do it with a needle!\r\n");
    else if (diff <= -2)
        ch->sendf("Easy.\r\n");
    else if (diff <= -1)
        ch->sendf("Fairly easy.\r\n");
    else if (diff == 0)
        ch->sendf("The perfect match!\r\n");
    else if (diff <= 1)
        ch->sendf("You could probably manage it.\r\n");
    else if (diff <= 2)
        ch->sendf("You might take a beating.\r\n");
    else if (diff <= 3)
        ch->sendf("You MIGHT win, maybe.\r\n");
    else if (diff <= 5)
        ch->sendf("Do you feel lucky? You better.\r\n");
    else if (diff <= 10)
        ch->sendf("Better bring some tough backup!\r\n");
    else if (diff <= 25)
        ch->sendf("Maybe if they are allergic to you, otherwise your last words will be 'Oh shit.'\r\n");
    else
        ch->sendf("No chance.\r\n");
}

ACMD(do_diagnose) {
    char buf[MAX_INPUT_LENGTH];
    struct char_data *vict;

    one_argument(argument, buf);

    if (*buf) {
        if (!(vict = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM)))
            ch->sendf("%s", CONFIG_NOPERSON);
        else {
            ch->sendf("%s", GET_SEX(vict) == SEX_MALE ? "He " : (GET_SEX(vict) == SEX_FEMALE ? "She " : "It "));
            diag_char_to_char(vict, ch);
        }
    } else {
        if (FIGHTING(ch)) {
            ch->sendf("%s",
                         GET_SEX(FIGHTING(ch)) == SEX_MALE ? "He " : (GET_SEX(FIGHTING(ch)) == SEX_FEMALE ? "She "
                                                                                                          : "It "));
            diag_char_to_char(FIGHTING(ch), ch);
        } else {
            ch->sendf("Diagnose who?\r\n");
        }
    }
}

static const char *ctypes[] = {
        "off", "on", "\n"
};

char *cchoice_to_str(char *col) {
    static char buf[READ_SIZE];
    char *s = nullptr;
    int i = 0;
    int fg = 0;
    int needfg = 0;
    int bold = 0;

    if (!col) {
        buf[0] = 0;
        return buf;
    }
    while (*col) {
        if (strchr(ANSISTART, *col)) {
            col++;
        } else {
            switch (*col) {
                case ANSISEP:
                case ANSIEND:
                    s = nullptr;
                    break;
                case '0':
                    s = nullptr;
                    break;
                case '1':
                    bold = 1;
                    s = nullptr;
                    break;
                case '5':
                    s = "blinking";
                    break;
                case '7':
                    s = "reverse";
                    break;
                case '8':
                    s = "invisible";
                    break;
                case '3':
                    col++;
                    fg = 1;
                    switch (*col) {
                        case '0':
                            s = bold ? "grey" : "black";
                            bold = 0;
                            fg = 1;
                            break;
                        case '1':
                            s = "red";
                            fg = 1;
                            break;
                        case '2':
                            s = "green";
                            fg = 1;
                            break;
                        case '3':
                            s = "yellow";
                            fg = 1;
                            break;
                        case '4':
                            s = "blue";
                            fg = 1;
                            break;
                        case '5':
                            s = "magenta";
                            fg = 1;
                            break;
                        case '6':
                            s = "cyan";
                            fg = 1;
                            break;
                        case '7':
                            s = "white";
                            fg = 1;
                            break;
                        case 0:
                            s = nullptr;
                            break;
                    }
                    break;
                case '4':
                    col++;
                    switch (*col) {
                        case '0':
                            s = "on black";
                            needfg = 1;
                            bold = 0;
                        case '1':
                            s = "on red";
                            needfg = 1;
                            bold = 0;
                        case '2':
                            s = "on green";
                            needfg = 1;
                            bold = 0;
                        case '3':
                            s = "on yellow";
                            needfg = 1;
                            bold = 0;
                        case '4':
                            s = "on blue";
                            needfg = 1;
                            bold = 0;
                        case '5':
                            s = "on magenta";
                            needfg = 1;
                            bold = 0;
                        case '6':
                            s = "on cyan";
                            needfg = 1;
                            bold = 0;
                        case '7':
                            s = "on white";
                            needfg = 1;
                            bold = 0;
                        default:
                            s = "underlined";
                            break;
                    }
                    break;
                default:
                    s = nullptr;
                    break;
            }
            if (s) {
                if (needfg && !fg) {
                    i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
                    fg = 1;
                }
                if (i)
                    i += snprintf(buf + i, sizeof(buf) - i, " ");
                if (bold) {
                    i += snprintf(buf + i, sizeof(buf) - i, "bright ");
                    bold = 0;
                }
                i += snprintf(buf + i, sizeof(buf) - i, "%s", s ? s : "null 1");
                s = nullptr;
            }
            col++;
        }
    }
    if (!fg)
        i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
    return buf;
}

int str_to_cchoice(char *str, char *choice) {
    char buf[MAX_STRING_LENGTH];
    int bold = 0, blink = 0, uline = 0, rev = 0, invis = 0, fg = 0, bg = 0, error = 0;
    int i, len = MAX_INPUT_LENGTH;
    struct {
        char *name;
        int *ptr;
    } attribs[] = {
            {"bright",     &bold},
            {"bold",       &bold},
            {"underlined", &uline},
            {"reverse",    &rev},
            {"blinking",   &blink},
            {"invisible",  &invis},
            {nullptr,      nullptr}
    };
    struct {
        char *name;
        int val;
        int bold;
    } colors[] = {
            {"default", -1, 0},
            {"normal",  -1, 0},
            {"black",   0,  0},
            {"red",     1,  0},
            {"green",   2,  0},
            {"yellow",  3,  0},
            {"blue",    4,  0},
            {"magenta", 5,  0},
            {"cyan",    6,  0},
            {"white",   7,  0},
            {"grey",    0,  1},
            {"gray",    0,  1},
            {nullptr,   0,  0}
    };
    skip_spaces(&str);
    if (isdigit(*str)) { /* Accept a raw code */
        strcpy(choice, str);
        for (i = 0; choice[i] && (isdigit(choice[i]) || choice[i] == ';'); i++);
        error = choice[i] != 0;
        choice[i] = 0;
        return error;
    }
    while (*str) {
        str = any_one_arg(str, buf);
        if (!strcmp(buf, "on")) {
            bg = 1;
            continue;
        }
        if (!fg) {
            for (i = 0; attribs[i].name; i++)
                if (!strncmp(attribs[i].name, buf, strlen(buf)))
                    break;
            if (attribs[i].name) {
                *(attribs[i].ptr) = 1;
                continue;
            }
        }
        for (i = 0; colors[i].name; i++)
            if (!strncmp(colors[i].name, buf, strlen(buf)))
                break;
        if (!colors[i].name) {
            error = 1;
            continue;
        }
        if (colors[i].val != -1) {
            if (bg == 1) {
                bg = 40 + colors[i].val;
            } else {
                fg = 30 + colors[i].val;
                if (colors[i].bold)
                    bold = 1;
            }
        }
    }
    choice[0] = i = 0;
    if (bold)
        i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_BOLD);
    if (uline)
        i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_UNDERLINE);
    if (blink)
        i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_BLINK);
    if (rev)
        i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_REVERSE);
    if (invis)
        i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_INVIS);
    if (!i)
        i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "", AA_NORMAL);
    if (fg && fg != -1)
        i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "", fg);
    if (bg && bg != -1)
        i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "", bg);

    return error;
}

char *default_color_choices[NUM_COLOR + 1] = {
/* COLOR_NORMAL */    AA_NORMAL,
/* COLOR_ROOMNAME */    AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_ROOMOBJS */    AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_ROOMPEOPLE */    AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_HITYOU */    AA_NORMAL ANSISEPSTR AF_RED,
/* COLOR_YOUHIT */    AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_OTHERHIT */    AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_CRITICAL */    AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_HOLLER */    AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_SHOUT */    AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_GOSSIP */    AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_AUCTION */    AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_CONGRAT */    AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_TELL */    AA_NORMAL ANSISEPSTR AF_RED,
/* COLOR_YOUSAY */    AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_ROOMSAY */    AA_NORMAL ANSISEPSTR AF_WHITE,
                      nullptr
};

ACMD(do_color) {
    char arg[MAX_INPUT_LENGTH];
    char *p;
    int tp;

    /*if (IS_NPC(ch))
    return;*/

    p = any_one_arg(argument, arg);

    if (!*arg) {
        ch->sendf("Usage: color [ off | on ]\r\n");
        return;
    }
    if (((tp = search_block(arg, ctypes, false)) == -1)) {
        ch->sendf("Usage: color [ off | on ]\r\n");
        return;
    }
    switch (tp) {
        case C_OFF:
            ch->clearFlag(FlagType::Pref, PRF_COLOR);
            break;
        case C_ON:
            ch->setFlag(FlagType::Pref, PRF_COLOR);
            break;
    }
    ch->sendf("Your color is now @o%s@n.\r\n", ctypes[tp]);
}

ACMD(do_toggle) {
    char buf2[4];

    if (IS_NPC(ch))
        return;

    if (GET_WIMP_LEV(ch) == 0)
        strcpy(buf2, "OFF");    /* strcpy: OK */
    else
        sprintf(buf2, "%-3.3d", GET_WIMP_LEV(ch));    /* sprintf: OK */

    if (GET_ADMLEVEL(ch)) {
        ch->sendf(
                     "      Buildwalk: %-3s    "
                     "Clear Screen in OLC: %-3s\r\n",
                     ONOFF(PRF_FLAGGED(ch, PRF_BUILDWALK)),
                     ONOFF(PRF_FLAGGED(ch, PRF_CLS)));

        ch->sendf(
                     "      No Hassle: %-3s    "
                     "      Holylight: %-3s    "
                     "     Room Flags: %-3s\r\n",
                     ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
                     ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
                     ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS))
        );
    }

    ch->sendf(
                 "Hit Pnt Display: %-3s    "
                 "     Brief Mode: %-3s    "
                 " Summon Protect: %-3s\r\n"

                 "   Move Display: %-3s    "
                 "   Compact Mode: %-3s    "
                 "       On Quest: %-3s\r\n"

                 "    Exp Display: %-3s    "
                 "         NoTell: %-3s    "
                 "   Repeat Comm.: %-3s\r\n"

                 "     Ki Display: %-3s    "
                 "           Deaf: %-3s    "
                 "     Wimp Level: %-3s\r\n"

                 " Gossip Channel: %-3s    "
                 "Auction Channel: %-3s    "
                 "  Grats Channel: %-3s\r\n"

                 "      Auto Loot: %-3s    "
                 "      Auto Gold: %-3s    "
                 "    Color Level: %s\r\n"

                 "     Auto Split: %-3s    "
                 "       Auto Sac: %-3s    "
                 "       Auto Mem: %-3s\r\n"

                 "     View Order: %-3s    "
                 "    Auto Assist: %-3s    "
                 " Auto Show Exit: %-3s\r\n"

                 "    TNL Display: %-3s    ",

                 ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
                 ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
                 ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

                 ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
                 ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
                 YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

                 ONOFF(PRF_FLAGGED(ch, PRF_DISPEXP)),
                 ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
                 YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

                 ONOFF(PRF_FLAGGED(ch, PRF_DISPKI)),
                 YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
                 buf2,

                 ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
                 ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
                 ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),

                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
                 ctypes[COLOR_LEV(ch)],

                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOMEM)),

                 ONOFF(PRF_FLAGGED(ch, PRF_VIEWORDER)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),

                 ONOFF(PRF_FLAGGED(ch, PRF_DISPTNL)));

}

static int sort_commands_helper(const void *a, const void *b) {
    return strcmp(complete_cmd_info[*(const int *) a].sort_as,
                  complete_cmd_info[*(const int *) b].sort_as);
}

void sort_commands() {
    int a, num_of_cmds = 0;

    while (complete_cmd_info[num_of_cmds].command[0] != '\n')
        num_of_cmds++;
    num_of_cmds++;    /* \n */

    CREATE(cmd_sort_info, int, num_of_cmds);

    for (a = 0; a < num_of_cmds; a++)
        cmd_sort_info[a] = a;

    /* Don't sort the RESERVED or \n entries. */
    qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}

ACMD(do_commands) {
    int no, i, cmd_num;
    int wizhelp = 0, socials = 0;
    struct char_data *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (*arg) {
        if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)) || IS_NPC(vict)) {
            ch->sendf("Who is that?\r\n");
            return;
        }
        if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
            ch->sendf("You can't see the commands of people above your level.\r\n");
            return;
        }
    } else
        vict = ch;

    if (subcmd == SCMD_SOCIALS)
        socials = 1;
    else if (subcmd == SCMD_WIZHELP)
        wizhelp = 1;

    ch->sendf("The following %s%s are available to %s:\r\n",
                 wizhelp ? "privileged " : "",
                 socials ? "socials" : "commands",
                 vict == ch ? "you" : GET_NAME(vict));

    /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
    for (no = 1, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) {
        i = cmd_sort_info[cmd_num];

        if (complete_cmd_info[i].minimum_level < 0 ||
            GET_LEVEL(vict) < complete_cmd_info[i].minimum_level)
            continue;

        if (complete_cmd_info[i].minimum_admlevel < 0 ||
            GET_ADMLEVEL(vict) < complete_cmd_info[i].minimum_admlevel)
            continue;

        if ((complete_cmd_info[i].minimum_admlevel >= ADMLVL_IMMORT) != wizhelp)
            continue;

        if (!wizhelp && socials != (complete_cmd_info[i].command_pointer == do_action ||
                                    complete_cmd_info[i].command_pointer == do_insult))
            continue;

        if (check_disabled(&complete_cmd_info[i]))
            sprintf(arg, "(%s)", complete_cmd_info[i].command);
        else
            sprintf(arg, "%s", complete_cmd_info[i].command);

        ch->sendf("%-11s%s", arg, no++ % 7 == 0 ? "\r\n" : "");

    }

    if (no % 7 != 1)
        ch->sendf("\r\n");
}


ACMD(do_history) {
    char arg[MAX_INPUT_LENGTH];
    int type;

    one_argument(argument, arg);
    if(IS_NPC(ch)) return;

    auto p = players[ch->uid];

    type = search_block(arg, history_types, false);
    if (!*arg || type < 0) {
        int i;

        ch->sendf("Usage: history <");
        for (i = 0; *history_types[i] != '\n'; i++) {
            if ((i != 3 && GET_ADMLEVEL(ch) <= 0) || GET_ADMLEVEL(ch) >= 1) {
                ch->sendf(" %s ", history_types[i]);
            }
            if (*history_types[i + 1] == '\n') {
                ch->sendf(">\r\n");
            } else {
                if ((i != 3 && GET_ADMLEVEL(ch) <= 0) || GET_ADMLEVEL(ch) >= 1) {
                    ch->sendf("|");
                }
            }
        }
        return;
    }

    if (p->comm_hist[type] && p->comm_hist[type]->text && *p->comm_hist[type]->text) {
        struct txt_block *tmp;
        for (tmp = p->comm_hist[type]; tmp; tmp = tmp->next)
            ch->sendf("%s", tmp->text);
/* Make this a 1 if you want history to cear after viewing */
#if 0
        free_history(ch, type);
#endif
    } else
        ch->sendf("You have no history in that channel.\r\n");
}

void add_history(struct char_data *ch, char *str, int type) {
    int i = 0;
    char time_str[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    struct txt_block *tmp;
    time_t ct;

    if (IS_NPC(ch))
        return;

    auto p = players[ch->uid];

    tmp = p->comm_hist[type];
    ct = time(nullptr);
    strftime(time_str, sizeof(time_str), "%H:%M ", localtime(&ct));

    sprintf(buf, "%s%s", time_str, str);

    if (!tmp) {
        CREATE(p->comm_hist[type], struct txt_block, 1);
        p->comm_hist[type]->text = strdup(buf);
    } else {
        while (tmp->next)
            tmp = tmp->next;
        CREATE(tmp->next, struct txt_block, 1);
        tmp->next->text = strdup(buf);

        for (tmp = p->comm_hist[type]; tmp; tmp = tmp->next, i++);

        for (; i > HIST_LENGTH && p->comm_hist[type]; i--) {
            tmp = p->comm_hist[type];
            p->comm_hist[type] = tmp->next;
            if (tmp->text)
                free(tmp->text);
            free(tmp);
        }
    }
    /* add this history message to ALL */
    if (type != HIST_ALL)
        add_history(ch, str, HIST_ALL);
}

ACMD(do_scan) {
    int i;
    char *dirnames[] = {
            "North", "East", "South", "West", "Up", "Down", "Northwest", "Northeast", "Southeast", "Southwest",
            "Inside", "Outside"};

    if (GET_POS(ch) < POS_SLEEPING) {
        ch->sendf("You can't see anything but stars!\r\n");
        return;
    }
    if (!AWAKE(ch)) {
        ch->sendf("You must be dreaming.\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_BLIND)) {
        ch->sendf("You can't see a damn thing, you're blind!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_EYEC)) {
        ch->sendf("You can't see a damned thing, your eyes are closed!\r\n");
        return;
    }

    auto room = ch->getRoom();
    if(!room) {
        ch->sendf("You are nowhere.\r\n");
        return;
    }

    auto darkHere = room->isInsideDark();

    for (auto &[i, d] : room->getExits()) {
        if(i > 10) break;

        if (darkHere && (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) &&
            (!AFF_FLAGGED(ch, AFF_INFRAVISION))) {
            ch->sendf("%s: DARK\r\n", dirnames[i]);
            continue;
        }

        auto dest = d->getDestination();
        if(!dest) continue;
        if(d->checkFlag(FlagType::Exit, EX_CLOSED)) continue;

        ch->sendf("@w-----------------------------------------@n\r\n");
        ch->sendf("          %s%s: %s %s\r\n", CCCYN(ch, C_NRM), dirnames[i],
                     dest->name ? dest->name : "You don't think you saw what you just saw.",
                     CCNRM(ch, C_NRM));
        ch->sendf("@W          -----------------          @n\r\n");

        list_obj_to_char(dest->getInventory(), ch, SHOW_OBJ_LONG, false);
        auto people = dest->getPeople();
        list_char_to_char(people, ch);
        if (dest->geffect >= 1 && dest->geffect <= 5) {
            ch->sendf("@rLava@w is pooling in someplaces here...@n\r\n");
        }
        if (dest->geffect >= 6) {
            ch->sendf("@RLava@r covers pretty much the entire area!@n\r\n");
        }
        /* Check 2nd room away */

        auto dexits = dest->getExits();
        auto d2 = dexits[i];
        if(!d2) continue;
        auto dest2 = d2->getDestination();
        if(!dest2) continue;
        if(d2->checkFlag(FlagType::Exit, EX_CLOSED)) continue;

        if (!IS_DARK(dest2->vn)) {
            ch->sendf("@w-----------------------------------------@n\r\n");
            ch->sendf("          %sFar %s: %s %s\r\n", CCCYN(ch, C_NRM), dirnames[i],
                         dest2->name ? dest2->name
                                             : "You don't think you saw what you just saw.",
                         CCNRM(ch, C_NRM));
            ch->sendf("@W          -----------------          @n\r\n");

            list_obj_to_char(dest2->getInventory(), ch, SHOW_OBJ_LONG, false);
            list_char_to_char(dest2->getPeople(), ch);
            if (dest2->geffect >= 1 && dest2->geffect <= 5) {
                ch->sendf("@rLava@w is pooling in someplaces here...@n\r\n");
            }
            if (dest2->geffect >= 6) {
                ch->sendf("@RLava@r covers pretty much the entire area!@n\r\n");
            }
        } else {
            ch->sendf("%s<-> %sFar %s: Too dark to tell! %s<->%s\r\n", QMAG, QCYN, dirnames[i],
                         QMAG, QNRM);
        }

    }
    ch->sendf("@w-----------------------------------------@n\r\n");
}

ACMD(do_toplist) {
    if (IS_NPC(ch))
        return;

    FILE *file;
    char fname[40], filler[50], line[256];
    int64_t points[25] = {0}, stats;
    char *title[25] = {""};
    int count = 0, x = 0;

    /* Read Introduction File */
    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist")) {
        ch->sendf("The toplist file does not exist.");
        return;
    } else if (!(file = fopen(fname, "r"))) {
        ch->sendf("The toplist file does not exist.");
        return;
    }
    while (!feof(file) || count < 25) {
        get_line(file, line);
        switch (count) {
            default:
                sscanf(line, "%s %" I64T "\n", filler, &stats);
                break;

        }
        title[count] = strdup(filler);
        points[count] = stats;
        count++;
        *filler = '\0';
    }
    ch->sendf("@D-=[@BDBAT Top Lists for @REra@C %d@D]=-@n\r\n", CURRENT_ERA);
    while (x <= count) {
        switch (x) {
            /* Powerlevel Area */
            case 0:
                ch->sendf("       @D-@RPowerlevel@D-@n\r\n");
                ch->sendf("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 1:
                ch->sendf("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 2:
                ch->sendf("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 3:
                ch->sendf("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 4:
                ch->sendf("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
                /* Ki Area */
            case 5:
                ch->sendf("       @D-@BKi        @D-@n\r\n");
                ch->sendf("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 6:
                ch->sendf("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 7:
                ch->sendf("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 8:
                ch->sendf("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 9:
                ch->sendf("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
                /* Stamina Area */
            case 10:
                ch->sendf("       @D-@GStamina   @D-@n\r\n");
                ch->sendf("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 11:
                ch->sendf("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 12:
                ch->sendf("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 13:
                ch->sendf("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 14:
                ch->sendf("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
                /* Stamina Area */
            case 15:
                ch->sendf("       @D-@gZenni     @D-@n\r\n");
                ch->sendf("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 16:
                ch->sendf("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 17:
                ch->sendf("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 18:
                ch->sendf("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
            case 19:
                ch->sendf("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
                free(title[x]);
                break;
                /* Rpp Area */
            case 20:
                /*ch->sendf("       @D-@mRPP       @D-@n\r\n");
     ch->sendf("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);*/
                free(title[x]);
                break;
            case 21:
                /*ch->sendf("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);*/
                free(title[x]);
                break;
            case 22:
                /*ch->sendf("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);*/
                free(title[x]);
                break;
            case 23:
                /*ch->sendf("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);*/
                free(title[x]);
                break;
            case 24:
                /*ch->sendf("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);*/
                free(title[x]);
                break;
        }
        x++;
    }
    fclose(file);
}

ACMD(do_whois) {
    char buf[MAX_INPUT_LENGTH];
    int clan = false;
    const char *immlevels[ADMLVL_IMPL + 2] = {
            "[Mortal]",          /* lowest admin level */
            "[Enforcer]",        /* lowest admin level +1 */
            "[First Class Enforcer]",         /* lowest admin level +2 */
            "[High Enforcer]",             /* lowest admin level +3 */
            "[Vice Admin]",     /* lowest admin level +4 */
            "[Administrator]",     /* lowest admin level +5 */
            "[Implementor]",
    };

    skip_spaces(&argument);

    if (!*argument) {
        ch->sendf("Who?\r\n");
        return;
    }

    auto victim = findPlayer(argument);
    if(!victim) {
        ch->sendf("There is no such player.\r\n");
        return;
    }

    if (GET_CLAN(victim) != nullptr) {
        if (!strstr(GET_CLAN(victim), "None")) {
            sprintf(buf, "%s", GET_CLAN(victim));
            clan = true;
        }
        if (strstr(GET_CLAN(victim), "Applying")) {
            sprintf(buf, "%s", GET_CLAN(victim));
            clan = true;
        }
    }
    if (GET_CLAN(victim) == nullptr || strstr(GET_CLAN(victim), "None")) {
        clan = false;
    }
    ch->sendf("@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
    if (GET_ADMLEVEL(victim) >= ADMLVL_IMMORT) {
        ch->sendf("@cName     @D: @G%s\r\n", GET_NAME(victim));
        ch->sendf("@cImm Level@D: @G%s\r\n", immlevels[GET_ADMLEVEL(victim)]);
        ch->sendf("@cTitle    @D: @G%s\r\n", GET_TITLE(victim));
    } else {
        ch->sendf(
                     "@cName  @D: @w%s\r\n@cSensei@D: @w%s\r\n@cRace  @D: @w%s\r\n@cTitle @D: @w%s@n\r\n@cClan  @D: @w%s@n\r\n",
                     GET_NAME(victim), sensei::getName(victim->chclass), race::getName(victim->race),
                     GET_TITLE(victim), clan ? buf : "None.");

    }
    ch->sendf("@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
}

static void search_in_direction(struct char_data *ch, int dir) {
    int check = false, skill_lvl, dchide = 20;

    ch->sendf("You search for secret doors.\r\n");
    act("$n searches the area intently.", true, ch, nullptr, nullptr, TO_ROOM);

    auto r = ch->getRoom();
    auto exits = r->getExits();
    auto e = exits[dir];

    if(!e) {
        ch->sendf("There is no exit there.\r\n");
        return;
    }
    auto dest = e->getDestination();
    if(!dest) {
        ch->sendf("That leads nowhere.\r\n");
        return;
    }

    /* SEARCHING is allowed untrained */
    skill_lvl = GET_SKILL(ch, SKILL_SEARCH);
    if (IS_TRUFFLE(ch) || IS_HUMAN(ch))
        skill_lvl = skill_lvl + 2;
    if (IS_HALFBREED(ch))
        skill_lvl = skill_lvl + 1;

    dchide = e->dchide;

    if (skill_lvl > dchide)
        check = true;

    if (auto gen = e->getLookDesc(); !gen.empty() && !e->checkFlag(FlagType::Exit, EX_SECRET))
        ch->sendf(gen);
    else if (!e->checkFlag(FlagType::Exit, EX_SECRET))
        ch->sendf("There is a normal exit there.\r\n");
    else if (e->checkFlag(FlagType::Exit, EX_ISDOOR) &&
             e->checkFlag(FlagType::Exit, EX_SECRET) &&
             !e->getAlias().empty() && (check == true))
        ch->sendf("There is a hidden door keyword: '%s' %sthere.\r\n",
                     fname(e->getAlias().c_str()),
                     (e->checkFlag(FlagType::Exit, EX_CLOSED)) ? "" : "open ");
    else
        ch->sendf("There is no exit there.\r\n");

}

ACMD(do_oaffects) {
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    ch->sendf("Temporarily disabled.\r\n");
}

ACMD(do_desc) {
    auto d = ch->desc;
    if(!d) {
        return;
    }
    write_to_output(d, "Current description:\r\n%s", ch->getLookDesc());
    write_to_output(d, "Enter the new text you'd like others to see when they look at you.\r\n");
    string_write(d, &ch->look_description, EXDSCR_LENGTH, 0, nullptr);
    STATE(d) = CON_EXDESC;
}