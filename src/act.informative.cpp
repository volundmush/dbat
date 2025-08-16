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
#include "dbat/send.h"
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
#include "dbat/clan.h"
#include "dbat/players.h"
#include "dbat/account.h"
#include "dbat/improved-edit.h"
#include "dbat/transformation.h"
#include "dbat/planet.h"
#include "dbat/random.h"

/* local functions */
static void gen_map(const Location& loc, Character *ch, int num);

static void see_plant(Object *obj, Character *ch);

static double terrain_bonus(Character *ch);

static void search_room(Character *ch);

static void bonus_status(Character *ch);

static int sort_commands_helper(const void *a, const void *b);

static void print_object_location(int num, Object *obj, Character *ch, int recur);

static void list_obj_to_char(const std::vector<std::weak_ptr<Object>> &list, Character *ch, int mode, int show);

static void trans_check(Character *ch, Character *vict);

static int show_obj_modifiers(Object *obj, Character *ch);

static void perform_mortal_where(Character *ch, char *arg);

static void perform_immort_where(Character *ch, char *arg);

static void diag_char_to_char(Character *i, Character *ch);

static void diag_obj_to_char(Object *obj, Character *ch);

static void look_at_char(Character *i, Character *ch);

static void list_char_to_char(const std::vector<std::weak_ptr<Character>> &list, Character *ch);

static void look_in_direction(Character *ch, int dir);

static void look_in_obj(Character *ch, char *arg);

static void look_out_window(Character *ch, const char *arg);

static void look_at_target(Character *ch, char *arg, int read);

static void search_in_direction(Character *ch, int dir);

static void display_spells(Character *ch, Object *obj);

static void display_scroll(Character *ch, Object *obj);

static void space_to_minus(char *str);

static int yesrace(int num);

static void map_draw_room(char map[9][10], int x, int y, room_rnum rnum,
                          Character *ch);
// definitions
ACMD(do_evolve)
{
    if (!IS_ARLIAN(ch) || IS_NPC(ch))
    {
        ch->sendText("You are not an arlian!\r\n");
        return;
    }

    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    struct EvolutionCost
    {
        int64_t cost;
        const char *name;
        int stat;
    };

    // Define evolution costs
    std::vector<EvolutionCost> evolutionCosts = {
        {(int64_t)(GET_LEVEL(ch) + (molt_threshold(ch) * 0.65) + (ch->getBaseStat<int64_t>("health") * 0.15)), "health", GET_CON(ch)},
        {(int64_t)(GET_LEVEL(ch) + (molt_threshold(ch) * 0.50) + (ch->getBaseStat<int64_t>("ki") * 0.22)), "ki", GET_WIS(ch)},
        {(int64_t)(GET_LEVEL(ch) + (molt_threshold(ch) * 0.50) + (ch->getBaseStat<int64_t>("stamina") * 0.15)), "stamina", GET_CON(ch)},
    };

    if (!*arg)
    {
        ch->sendText("@D-=@YConvert Evolution Points To What?@D=-@n\r\n");
        ch->sendText("@D-------------------------------------@n\r\n");
        for (const auto &cost : evolutionCosts)
        {
            ch->send_to("@C%s  @D: @Y%s @Wpts\r\n", cost.name, add_commas(cost.cost).c_str());
        }
        ch->send_to("@D[@Y%s @Wpts currently@D]@n\r\n", add_commas(GET_MOLT_EXP(ch)).c_str());
        return;
    }

    auto findCost = [&arg](const EvolutionCost &cost)
    {
        return !strcasecmp(arg, cost.name) || (!strcasecmp(arg, "hp") && !strcasecmp(cost.name, "health")) ||
               (!strcasecmp(arg, "st") && !strcasecmp(cost.name, "stamina"));
    };

    auto it = std::find_if(evolutionCosts.begin(), evolutionCosts.end(), findCost);

    if (it == evolutionCosts.end())
    {
        ch->sendText("Invalid evolution type. Try powerlevel, ki, or stamina.\r\n");
        return;
    }

    if (it->cost > molt_threshold(ch))
    {
        ch->send_to("You need a few more evolution levels before you can start upgrading %s.\r\n", it->name);
        return;
    }

    if (GET_MOLT_EXP(ch) < it->cost)
    {
        ch->sendText("You do not have enough evolution experience.\r\n");
        return;
    }

    double baseVal = ch->getBaseStat(it->name);
    double attrBonus = 1 + (it->stat / 20.0);
    double startBonus = Random::get<double>(0.8, 1.2) * attrBonus * ch->getPotential();
    double softCap = ch->calc_soft_cap();
    double diminishingReturns = std::max<double>((softCap - baseVal) / softCap, 0.05);
    int64_t bonusVal = static_cast<int64_t>(startBonus * diminishingReturns * 20);

    bonusVal = std::min<int64_t>(bonusVal, ch->getBaseStat<int64_t>("stamina") / 10);
    ch->gainBaseStat(it->name, bonusVal);
    ch->modBaseStat<int64_t>("molt_experience", -it->cost);
    ch->send_to("Your body evolves to make better use of the way it is now, and you feel that your %s has strengthened. @D[@C%s@D: @Y+%s@D]@n\r\n", it->name, it->name, add_commas(bonusVal).c_str());
}

static void see_plant(Object *obj, Character *ch)
{
    int water = GET_OBJ_VAL(obj, VAL_PLANT_WATERLEVEL);
    const char *description = obj->getShortDescription();

    if (water >= 0)
    {
        const char *maturity_stage;

        switch (GET_OBJ_VAL(obj, VAL_PLANT_MATURITY))
        {
        case 0:
            maturity_stage = "seed";
            break;
        case 1:
            maturity_stage = "very young";
            break;
        case 2:
            maturity_stage = "half grown";
            break;
        case 3:
            maturity_stage = "mature";
            break;
        case 4:
            maturity_stage = "flowering";
            break;
        case 5:
            maturity_stage = "close to harvestable";
            break;
        case 6:
            maturity_stage = "harvestable";
            break;
        default:
            return; // If the maturity level is unknown, do nothing.
        }

        ch->send_to("@wA %s @G%s@w is here. @D(@C%d Water Hours@D)@n\r\n", maturity_stage, description, water);
    }
    else
    {
        const char *dryness_level;

        if (water > -4)
        {
            dryness_level = "looking a bit @rdry";
        }
        else if (water > -10)
        {
            dryness_level = "looking extremely @rdry";
        }
        else
        {
            dryness_level = "@rdead@y and @rwithered";
        }

        ch->send_to("@yA @G%s@y that is %s, is here.@n\r\n", description, dryness_level);
    }
}

/* This is used to determine the terrain bonus for search_room - Iovan 12/16/2012*/
static double terrain_bonus(Character *ch)
{

    double bonus = 0.0;

    switch (ch->location.getTileType())
    {
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

    if (ch->location.getWhereFlag(WhereFlag::space))
    {
        bonus += -0.5;
    }

    return (bonus);
}

static bool match_exdesc(const char *word, const ExtraDescription &exdesc)
{
    if (exdesc.keyword.starts_with("."))
        return isname(word, exdesc.keyword.c_str() + 1);
    else
        return isname(word, exdesc.keyword.c_str());
}

char *find_exdesc(char *word, const std::vector<ExtraDescription> &list)
{
    for (const auto &i : list)
    {
        if (match_exdesc(word, i))
            return ((char *)i.description.c_str());
    }
    return nullptr;
}

/* This is used to find hidden people in a room with search - Iovan 12/16/2012 */
static void search_room(Character *ch)
{

    Character *vict, *next_v;
    int perc =
        (GET_INT(ch) * 0.6) + GET_SKILL(ch, SKILL_SPOT) + GET_SKILL(ch, SKILL_SEARCH) + GET_SKILL(ch, SKILL_LISTEN);
    int prob = 0, found = 0;
    double bonus = 1.0, terrain = 1.0;

    if ((ch->getCurVital(CharVital::stamina)) < GET_MAX_MOVE(ch) * 0.001)
    {
        ch->sendText("You do not have enough stamina.\r\n");
        return;
    }

    if (GET_SKILL(ch, SKILL_SENSE))
    {
        bonus += (GET_SKILL(ch, SKILL_SENSE) * 0.01);
    }

    reveal_hiding(ch, 0);
    act("@y$n@Y begins searching the room carefully.@n", true, ch, nullptr, nullptr, TO_ROOM);
    WAIT_STATE(ch, PULSE_1SEC);
    auto people = ch->location.getPeople();
    for (auto t : filter_raw(people))
    {
        vict = t;
        if (AFF_FLAGGED(vict, AFF_HIDE) && vict != ch)
        {
            if (GET_SUPPRESS(vict) >= 1)
            {
                perc *= (GET_SUPPRESS(vict) * 0.01);
            }
            prob = GET_DEX(vict) + (GET_INT(vict) * 0.6) + GET_SKILL(vict, SKILL_HIDE) +
                   GET_SKILL(vict, SKILL_MOVE_SILENTLY);

            if (AFF_FLAGGED(vict, AFF_LIQUEFIED))
            {
                prob *= 1.5;
            }
            if (ch->mutations.get(Mutation::infravision))
            {
                perc += 5;
            }
            if (vict->mutations.get(Mutation::natural_camouflage))
            {
                prob += 10;
            }
            terrain += terrain_bonus(vict);
            if (perc * bonus >= prob * terrain)
            { /* Found them. */
                act("@YYou find @y$N@Y hiding nearby!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@y$n@Y has found your hiding spot!@n", true, ch, nullptr, vict, TO_VICT);
                act("@y$n@Y has found @y$N's@Y hiding spot!@n", true, ch, nullptr, vict, TO_NOTVICT);
                reveal_hiding(vict, 4);
                found++;
            }
        }
    }
    auto loco = ch->location.getObjects();
    for (auto obj : filter_raw(loco))
    {
        if (OBJ_FLAGGED(obj, ITEM_BURIED) && perc * bonus > rand_number(50, 200))
        {
            act("@YYou uncover @y$p@Y, which had been burried here.@n", true, ch, obj, nullptr, TO_CHAR);
            act("@y$n@Y uncovers @y$p@Y, which had burried here.@n", true, ch, obj, nullptr, TO_ROOM);
            obj->item_flags.set(ITEM_BURIED, false);
            found++;
        }
    }
    ch->modCurVitalDam(CharVital::stamina, .001);

    if (found == 0)
    {
        ch->sendText("You find nothing hidden.\r\n");
        return;
    }
}

static const char *weapon_disp[6] = {
    "Sword",
    "Dagger",
    "Spear",
    "Club",
    "Gun",
    "Brawling"};

ACMD(do_mimic)
{

    if (IS_NPC(ch))
        return;

    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!GET_SKILL(ch, SKILL_MIMIC))
    {
        ch->sendText("You do not know how to mimic the appearance of other races.\r\n");
        return;
    }

    int count = 0, x = 0;

    // generate a list of mimic'able races.
    auto check = [&](Race id)
    { return race::isValidMimic(id); };
    auto races = race::filterRaces(check);
    if (!*arg)
    {
        ch->sendText("@CMimic Menu\n@c--------------------@W\r\n");
        for (const auto &r : races)
        {
            if (count == 2)
            {
                ch->send_to("%s\n", race::getName(r).c_str());
                count = 0;
            }
            else
            {
                ch->send_to("%s\n", race::getName(r).c_str());
                count++;
            }
        }
        ch->sendText("Stop@n\r\n");
        if (ch->mimic)
        {
            ch->send_to("You currently Mimic a %s", race::getName(ch->mimic.value()).c_str());
        }

        return;
    }

    if (!strcasecmp(arg, "stop"))
    {
        if (!ch->mimic)
        {
            ch->sendText("You are not imitating another race.\r\n");
            return;
        }
        act("@mYou concentrate for a moment and release the illusion that was mimicing another race.@n", true, ch,
            nullptr, nullptr, TO_CHAR);
        act("@M$n@m concentrates for a moment and SUDDENLY $s appearance changes some what!@n", true, ch, nullptr,
            nullptr, TO_ROOM);
        ch->mimic.reset();
    }

    auto choices = getEnumMap<Race>(check);
    auto chosen_race = partialMatch(arg, choices, false);
    if (!chosen_race)
    {
        ch->sendText(chosen_race.err);
        ch->sendText("That is not a race you can change into. Enter mimic without arugments for the mimic menu.\r\n");
        return;
    }
    auto race = chosen_race.value()->second;

    int prob = GET_SKILL(ch, SKILL_MIMIC), perc = axion_dice(0);
    double mult = 1 / prob;
    int64_t cost = GET_MAX_MANA(ch) * mult;

    if (race == ch->mimic)
    {
        ch->sendText("You are already mimicing that race. To stop enter 'mimic stop'\r\n");
        return;
    }
    else if (ch->getCurVital(CharVital::ki) < cost)
    {
        ch->sendText("You do not have enough ki to perform the technique.\r\n");
        return;
    }
    else if (prob < perc)
    {
        ch->modCurVital(CharVital::ki, -cost);
        act("@mYou concentrate and attempt to create an illusion to obscure your racial features. However you frown as you realize you have failed.@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@M$n@m concentrates and the light around them seems to shift and blur. It stops a moment later and $e frowns.@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        return;
    }
    else
    {
        char buf[MAX_STRING_LENGTH];
        ch->mimic = race;
        ch->modCurVital(CharVital::ki, -cost);
        sprintf(buf,
                "@M$n@m concentrates for a moment and $s features start to blur as light bends around $m. Now $e appears to be %s @M%s!@n",
                AN(RACE(ch)), LRACE(ch));
        ch->send_to("@mYou concentrate for a moment and your features start to blur as you use your ki to bend the light around your body. You now appear to be %s %s.@n\r\n", AN(RACE(ch)), LRACE(ch));
        act(buf, true, ch, nullptr, nullptr, TO_ROOM);
        return;
    }
}

ACMD(do_kyodaika)
{

    if (!IS_NAMEK(ch))
    {
        ch->sendText("You are not a namek!\r\n");
        return;
    }

    if (!AFF_FLAGGED(ch, AFF_KYODAIKA))
    {
        act("@GYou growl as your body grows to ten times its normal size!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n@G growls as $s body grows to ten times its normal size!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->sendText("@cStrength@D: @C+5\r\n@cSpeed@D: @c-2@n\r\n");
        assign_affect(ch, AFF_KYODAIKA, 0, -1, 5, 0, 0, 0, 0, -2);
    }
    else
    {
        act("@GYou growl as your body shrinks to its normal size!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n@G growls as $s body shrinks to its normal size!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->sendText("@cStrength@D: @C-5\r\n@cSpeed@D: @c+2@n\r\n");
        null_affect(ch, AFF_KYODAIKA);
    }
}

ACMD(do_table)
{
    Object *obj = nullptr, *obj2 = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        ch->sendText("Syntax: table (red | blue | green | yellow) (card name)");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->location.getObjects())))
    {
        ch->sendText("You don't see that table here.\r\n");
        return;
    }

    if (!(obj2 = get_obj_in_list_vis(ch, arg2, nullptr, obj->getInventory())))
    {
        ch->sendText("That card doesn't seem to be on that table.\r\n");
        return;
    }

    char buf[200];
    sprintf(buf, "$n looks at %s on %s.\r\n", obj2->getShortDescription(), obj->getShortDescription());
    act(buf, true, ch, nullptr, nullptr, TO_ROOM);
    ch->send_to("%s", obj2->getLookDescription());
}

ACMD(do_draw)
{
    if (!SITS(ch))
    {
        ch->sendText("You are not sitting at a duel table.\r\n");
        return;
    }

    if (GET_OBJ_VNUM(SITS(ch)) < 604 || GET_OBJ_VNUM(SITS(ch)) > 607)
    {
        ch->sendText("You need to be sitting at an official table to play.\r\n");
        return;
    }

    Object *obj = nullptr, *obj2 = nullptr, *obj3 = nullptr, *next_obj = nullptr;
    int drawn = false;

    if (!(obj = get_obj_in_list_vis(ch, "case", nullptr, ch->getInventory())))
    {
        ch->sendText("You don't have a case.\r\n");
        return;
    }
    auto con = obj->getInventory();
    for (auto obj2 : filter_raw(con))
    {
        obj2->clearLocation();
        ch->addToInventory(obj2);
        obj3 = obj2;
        drawn = true;
        break;
    }
    if (drawn == false)
    {
        ch->sendText("You don't have any cards in the case!\r\n");
        return;
    }
    else
    {
        act("$n draws a card from $s $p.\r\n", true, ch, obj, nullptr, TO_ROOM);
        ch->send_to("You draw a card.\r\n%s\r\n", obj3->getLookDescription());
        return;
    }
}

ACMD(do_shuffle)
{

    if (!SITS(ch))
    {
        ch->sendText("You are not sitting at a duel table.\r\n");
        return;
    }

    if (GET_OBJ_VNUM(SITS(ch)) < 604 || GET_OBJ_VNUM(SITS(ch)) > 607)
    {
        ch->sendText("You need to be sitting at an official table to play.\r\n");
        return;
    }

    Object *obj = nullptr, *obj2 = nullptr, *next_obj = nullptr;
    int count = 0;

    if (!(obj = get_obj_in_list_vis(ch, "case", nullptr, ch->getInventory())))
    {
        ch->sendText("You don't have a case.\r\n");
        return;
    }

    auto con = obj->getInventory();
    for (auto obj2 : filter_raw(con))
    {
        if (!OBJ_FLAGGED(obj2, ITEM_CARD))
        {
            continue;
        }
        count += 1;
    }
    if (count <= 0)
    {
        ch->sendText("You don't have any cards in the case!\r\n");
        return;
    }
    int total = count;
    auto con2 = obj->getInventory();
    for (auto obj2 : filter_raw(con2))
    {
        obj2->clearLocation();
        obj2->moveToLocation(48);
    }
    while (count > 0)
    {
        auto con = get_room(48)->getObjects();
        for (auto obj2 : filter_raw(con))
        {
            if (!OBJ_FLAGGED(obj2, ITEM_CARD))
            {
                continue;
            }
            if (count > 1 && rand_number(1, 4) == 3)
            {
                count -= 1;
                obj2->clearLocation();
                obj->addToInventory(obj2);
            }
            else if (count == 1)
            {
                count -= 1;
                obj2->clearLocation();
                obj2->addToInventory(obj);
            }
        }
    }
    ch->sendText("You shuffle the cards carefully.\r\n");
    act("$n shuffles their deck.", true, ch, nullptr, nullptr, TO_ROOM);
    ch->location.send_to("There were %d cards in the deck.\r\n", total);
}

ACMD(do_hand)
{

    Object *obj, *next_obj;
    char arg[MAX_INPUT_LENGTH];
    int count = 0;

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Syntax: hand (look | show)\r\n");
        return;
    }

    if (!strcasecmp("look", arg))
    {
        ch->sendText("@CYour hand contains:\r\n@D---------------------------@n\r\n");
        auto con = ch->getInventory();
        for (auto obj : filter_raw(con))
        {
            if (obj && !OBJ_FLAGGED(obj, ITEM_CARD))
            {
                continue;
            }
            if (obj)
            {
                count += 1;
                ch->send_to("%s\r\n", obj->getShortDescription());
            }
        }
        act("$n looks at $s hand.", true, ch, nullptr, nullptr, TO_ROOM);
        if (count == 0)
        {
            ch->sendText("No cards.");
            act("There were no cards.", true, ch, nullptr, nullptr, TO_ROOM);
        }
        else if (count > 7)
        {
            act("You have more than seven cards in your hand.", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n has more than seven cards in $s hand.", true, ch, nullptr, nullptr, TO_ROOM);
        }
        else
        {
            char buf[200];
            sprintf(buf, "There are %d cards in the hand.", count);
            act(buf, true, ch, nullptr, nullptr, TO_ROOM);
        }
    }
    else if (!strcasecmp("show", arg))
    {
        ch->sendText("You show off your hand to the room.\r\n");
        act("@C$n's hand contains:\r\n@D---------------------------@n", true, ch, nullptr, nullptr, TO_ROOM);
        auto con = ch->getInventory();
        for (auto obj : filter_raw(con))
        {
            if (obj && !OBJ_FLAGGED(obj, ITEM_CARD))
            {
                continue;
            }
            if (obj)
            {
                count += 1;
                act("$p", true, ch, obj, nullptr, TO_ROOM);
            }
        }
        if (count == 0)
        {
            act("No cards.", true, ch, nullptr, nullptr, TO_ROOM);
        }
        if (count > 7)
        {
            act("You have more than seven cards in your hand.", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n has more than seven cards in $s hand.", true, ch, nullptr, nullptr, TO_ROOM);
        }
    }
    else
    {
        ch->sendText("Syntax: hand (look | show)\r\n");
        return;
    }
}

ACMD(do_post)
{

    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Object *obj;
    Object *obj2;

    two_arguments(argument, arg, arg2);

    if (!*arg)
    {
        ch->sendText("Syntax: post (obj name)\n"
                     "        post (obj name) (target obj name)\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
    {
        ch->sendText("You don't seem to have that.\r\n");
        return;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_NOTE)
    {
        ch->sendText("You can only post notepaper.\r\n");
        return;
    }

    if (ch->location.getRoomFlag(ROOM_GARDEN1) || ch->location.getRoomFlag(ROOM_GARDEN2))
    {
        ch->sendText("You can not post on things in a garden.\r\n");
        return;
    }

    if (!*arg2)
    {
        const auto tile = ch->location.getTileType();
        if (tile != SECT_INSIDE && tile != SECT_CITY)
        {
            ch->sendText("You are not near any general structure you can post it on.\r\n");
            return;
        }
        act("@WYou post $p@W on a nearby structure.@n", true, ch, obj, nullptr, TO_CHAR);
        act("@C$n@W posts $p@W on a nearby structure.@n", true, ch, obj, nullptr, TO_ROOM);
        obj->clearLocation();
        obj->moveToLocation(ch);
        GET_OBJ_POSTTYPE(obj) = 1;
        return;
    }

    if (!(obj2 = get_obj_in_list_vis(ch, arg2, nullptr, ch->location.getObjects())))
    {
        ch->sendText("You can't seem to find the thing you want to post it on.\r\n");
        return;
    }

    if (GET_OBJ_POSTED(obj2))
    {
        ch->sendText("It already has something posted on it. Get that first if you want to post.\r\n");
        return;
    }

    if (GET_OBJ_TYPE(obj2) == ITEM_BOARD)
    {
        ch->sendText("Boards come with their own means of posting messages.\r\n");
        return;
    }

    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "@C$n@W posts %s@W on %s@W.@n", obj->getShortDescription(), obj2->getShortDescription());
    ch->send_to("@WYou post %s@W on %s@W.@n\r\n", obj->getShortDescription(), obj2->getShortDescription());
    act(buf, true, ch, nullptr, nullptr, TO_ROOM);
    obj->clearLocation();
    obj->moveToLocation(ch);
    GET_OBJ_POSTTYPE(obj) = 2;
    GET_OBJ_POSTED(obj) = obj2;
    GET_OBJ_POSTED(obj2) = obj;
}

ACMD(do_play)
{

    if (GET_POS(ch) != POS_SITTING)
    {
        ch->sendText("You need to be sitting at an official table to play.\r\n");
        return;
    }

    if (!SITS(ch))
    {
        ch->sendText("You need to be sitting at an official table to play.\r\n");
        return;
    }

    if (GET_OBJ_VNUM(SITS(ch)) < 604 || GET_OBJ_VNUM(SITS(ch)) > 607)
    {
        ch->sendText("You need to be sitting at an official table to play.\r\n");
        return;
    }

    Object *obj = nullptr, *obj2 = nullptr, *obj3 = nullptr, *next_obj = nullptr;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Syntax: play (card name)");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
    {
        ch->sendText("You don't have that card to play.\r\n");
        return;
    }

    auto sitting = [ch](const auto &o)
    { return GET_OBJ_VNUM(o) == GET_OBJ_VNUM(SITS(ch)) - 4; };
    obj2 = ch->location.searchObjects(sitting);

    if (obj2 == nullptr)
    {
        ch->sendText("Your table is missing. Inform an immortal of this problem.\r\n");
        return;
    }

    act("You play $p on your table.", true, ch, obj, nullptr, TO_CHAR);
    act("$n plays $p on $s table.", true, ch, obj, nullptr, TO_ROOM);
    obj->clearLocation();
    obj2->addToInventory(obj);
}

/* Nickname an object */
ACMD(do_nickname)
{
    Object *obj = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        ch->sendText("Syntax: nickname (object) (nickname)\n");
        ch->sendText("Syntax: nickname ship (nickname)\n");
        return;
    }

    if (strcasecmp(arg, "ship"))
    {
        if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
        {
            ch->sendText("You don't have that item to nickname.\r\n");
            return;
        }
    }
    if (strlen(arg2) > 20)
    {
        ch->sendText("You can't nickname items with any name longer than 20 characters.\r\n");
        return;
    }

    if (!strcasecmp(arg, "ship"))
    {
        Object *ship = nullptr, *next_obj = nullptr, *ship2 = nullptr;
        int found = false;
        auto is_ship = [](const auto &o)
        { return GET_OBJ_VNUM(o) >= 45000 && GET_OBJ_VNUM(o) <= 45999; };
        if (ship2 = ch->location.searchObjects(is_ship))
        {
            found = true;
        }
        if (found == true)
        {
            if (strstr(arg2, "@"))
            {
                ch->sendText("You can't nickname a ship and use color codes. Sorry.\r\n");
                return;
            }
            else
            {
                char nick[MAX_INPUT_LENGTH];
                sprintf(nick, "%s", CAP(arg2));
                ship2->strings["look_description"] = nick;
                auto objs = objectSubscriptions.all(fmt::format("vnum_{}", GET_OBJ_VNUM(ship2) + 1000));
                for (auto k : filter_raw(objs))
                {
                    extract_obj(k);
                    auto was_in = ship2->location;
                    ship2->clearLocation();
                    ship2->moveToLocation(was_in);
                }
            }
        }
        return;
    }

    auto sdesc = obj->getShortDescription();

    if (strstr(sdesc, "nicknamed"))
    {
        ch->send_to("%s@w has already been nicknamed.@n\r\n", sdesc);
        return;
    }

    if (strstr(obj->getName(), "corpse"))
    {
        ch->send_to("%s@w is a corpse!@n\r\n", sdesc);
        return;
    }

    ch->send_to("@wYou nickname %s@w as '@C%s@w'.@n\r\n", sdesc, arg2);
    obj->strings["short_description"] = fmt::format("{} @wnicknamed @D(@C{}@D)@n", sdesc, CAP(arg2));
    obj->strings["name"] = fmt::format("{} {}", obj->getName(), arg2);
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
    "\n"};

ACMD(do_showoff)
{
    Object *obj = nullptr;
    Character *vict = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    *arg = '\0';
    *arg2 = '\0';

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        ch->sendText("You want to show what item to what character?\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
    {
        ch->sendText("You don't seem to have that.\r\n");
        return;
    }

    if (!(vict = get_player_vis(ch, arg2, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("There is no such person around.\r\n");
        return;
    }

    /* Ok show that target the object! */
    act("@WYou hold up $p@W for @C$N@W to see:@n", true, ch, obj, vict, TO_CHAR);
    act("@C$n@W holds up $p@W for you to see:@n", true, ch, obj, vict, TO_VICT);
    act("@C$n@W holds up $p@W for @c$N@W to see.@n", true, ch, obj, vict, TO_NOTVICT);
    show_obj_to_char(obj, vict, SHOW_OBJ_ACTION);
}

int readIntro(Character *ch, Character *vict)
{
    if (vict == nullptr)
    {
        return 0;
    }

    if (IS_NPC(ch) || IS_NPC(vict))
    {
        return 1;
    }

    auto &p = players.at(ch->id);

    return p.dub_names.contains(vict->id);
}

void introWrite(Character *ch, Character *vict, char *name)
{
    std::string n(name);
    auto &p = players.at(ch->id);
    p.dub_names[vict->id] = n;
}

ACMD(do_intro)
{

    if (IS_NPC(ch))
        return;

    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Character *vict;

    two_arguments(argument, arg, arg2);

    if (!*arg)
    {
        ch->sendText("Syntax: dub (target) (name)\r\nWho do you want to dub and what do you want to name them?\r\n");
        return;
    }

    if (!*arg2)
    {
        ch->sendText("Syntax: dub (target) (name)\r\nWhat name do you wish to know them by?\r\n");
        return;
    }

    if (strlen(arg2) > 20)
    {
        ch->sendText("Limit the name to 20 characters.\r\n");
        return;
    }
    if (strlen(arg2) < 3)
    {
        ch->sendText("Limit the name to at least 3 characters.\r\n");
        return;
    }

    if (strstr(arg2, "$") || strstr(arg2, "@") || strstr(arg2, "%"))
    {
        ch->sendText("Illegal character. No symbols.\r\n");
        return;
    }

    if (!(vict = get_player_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("There is no such person.\r\n");
        return;
    }

    if (vict == ch)
    {
        ch->sendText("That seems rather odd.\r\n");
        return;
    }

    if (IS_NPC(vict))
    {
        ch->sendText("That seems rather unwise.\r\n");
        return;
    }

    if (readIntro(vict, ch) == 2)
    {
        ch->sendText("There seems to have been an error, report this to Iovan.\r\n");
        return;
    }

    if (readIntro(ch, vict) == 1 && strstr(RACE(vict), arg))
    {
        ch->sendText("You have already dubbed them a name. If you want to redub them target the name you know them by.\r\n");
        return;
    }

    introWrite(ch, vict, arg2);
    act("You decide to call $M, $N.", true, ch, nullptr, vict, TO_CHAR);
    act("$n seems to decide something about you.", true, ch, nullptr, vict, TO_VICT);
    act("$n seems to decide something about $N.", true, ch, nullptr, vict, TO_NOTVICT);
}

/* Used when checking status or looking at a character */
static void send_attribute_desc(Character *ch, const char *label, const char *value)
{
    ch->send_to("            @D[@c%-12s@D: @W%-12s@D]@n\r\n", label, value);
}

static void draw_closed_exit(char map[9][10], int x, int y, int door)
{
    switch (door)
    {
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
}

static char get_sector_char(int sect, double geffect, double waterEnv)
{
    if (waterEnv >= 100.0)
        return '=';
    if (geffect >= 1)
    {
        switch (sect)
        {
        case SECT_INSIDE:
            return '2';
        case SECT_FIELD:
            return '2';
        case SECT_DESERT:
            return '7';
        case SECT_CITY:
            return '1';
        case SECT_FOREST:
            return '6';
        case SECT_MOUNTAIN:
            return '5';
        case SECT_HILLS:
            return '3';
        }
    }
    else
    {
        switch (sect)
        {
        case SECT_INSIDE:
            return 'i';
        case SECT_FIELD:
            return 'p';
        case SECT_DESERT:
            return '!';
        case SECT_CITY:
            return '(';
        case SECT_FOREST:
            return 'f';
        case SECT_MOUNTAIN:
            return '^';
        case SECT_HILLS:
            return 'h';
        case SECT_FLYING:
            return 's';
        case SECT_WATER_NOSWIM:
            return '`';
        case SECT_WATER_SWIM:
            return '+';
        case SECT_SHOP:
            return '&';
        case SECT_IMPORTANT:
            return '*';
        }
    }
    return '-';
}

static void draw_open_exit(char map[9][10], int x, int y, int door, int sect, double geffect, double waterEnv)
{
    char sectorChar = get_sector_char(sect, geffect, waterEnv);
    switch (door)
    {
    case NORTH:
        map[y - 1][x] = sectorChar;
        break;
    case EAST:
        map[y][x + 1] = sectorChar;
        break;
    case SOUTH:
        map[y + 1][x] = sectorChar;
        break;
    case WEST:
        map[y][x - 1] = sectorChar;
        break;
    case NORTHEAST:
        map[y - 1][x + 1] = sectorChar;
        break;
    case NORTHWEST:
        map[y - 1][x - 1] = sectorChar;
        break;
    case SOUTHEAST:
        map[y + 1][x + 1] = sectorChar;
        break;
    case SOUTHWEST:
        map[y + 1][x - 1] = sectorChar;
        break;
    }
}

static void map_draw_room(char map[9][10], int x, int y, Destination &node, Character *ch)
{

    for (auto &[door, e] : node.getExits())
    {
        bool isClosed = IS_SET(e.exit_info, EX_CLOSED);
        bool isSecret = IS_SET(e.exit_info, EX_SECRET);

        if (isClosed && !isSecret)
        {
            draw_closed_exit(map, x, y, static_cast<int>(door));
        }
        else if (!isClosed)
        {
            auto sect = static_cast<int>(e.getSectorType());
            double geffect = e.getGroundEffect();
            double waterEnv = e.getEnvironment(ENV_WATER);
            draw_open_exit(map, x, y, static_cast<int>(door), sect, geffect, waterEnv);
        }
    }
}

ACMD(do_map)
{
    gen_map(ch->location, ch, 1);
}

static void print_map_key(Character *ch)
{
    ch->sendText("@W               @D-[@CArea Map@D]-\r\n");
    ch->sendText("@D-------------------------------------------@w\r\n");
    ch->sendText("@WC = City, @wI@W = Inside, @GP@W = Plain, @gF@W = Forest\r\n");
    ch->sendText("@DM@W = Mountain, @yH@W = Hills, @CS@W = Sky, @BW@W = Water\r\n");
    ch->sendText("@bU@W = Underwater, @m$@W = Shop, @m#@W = Important,\r\n");
    ch->sendText("@YD@W = Desert, @c~@W = Shallow Water, @4 @n@W = Lava,\r\n");
    ch->sendText("@WLastly @RX@W = You.\r\n");
    ch->sendText("@D-------------------------------------------\r\n");
    ch->sendText("@D                  @CNorth@w\r\n");
    ch->sendText("@D                    @c^@w\r\n");
    ch->sendText("@D             @CWest @c< O > @CEast@w\r\n");
    ch->sendText("@D                    @cv@w\r\n");
    ch->sendText("@D                  @CSouth@w\r\n");
    ch->sendText("@D                ---------@w\r\n");
}

static void initialize_map(char map[9][10])
{
    for (int i = 0; i < 9; i++)
    {
        strcpy(map[i], "         ");
    }
}

static void replace_map_symbols(char *buf)
{
    static const char *search_pairs[][2] = {
        {"x", "@RX"}, {"&", "@m$"}, {"*", "@m#"}, {"+", "@c~"}, {"s", "@CS"}, {"i", "@wI"}, {"(", "@WC"}, {"^", "@DM"}, {"h", "@yH"}, {"`", "@BW"}, {"=", "@bU"}, {"p", "@GP"}, {"f", "@gF"}, {"!", "@YD"}, {"-", "@w:"}, {"1", "@4@YC@n"}, {"2", "@4@YP@n"}, {"3", "@4@YH@n"}, {"7", "@4@YD@n"}, {"5", "@4@YM@n"}, {"6", "@4@YF@n"}, {"8", "@1 @n"}};

    for (const auto &pair : search_pairs)
    {
        search_replace(buf, pair[0], pair[1]);
    }
}

static void print_map_line(Character *ch, int key, const char *buf)
{
    const char *key_strings[] = {
        "    @WC: City, @wI@W: Inside, @GP@W: Plain@n\r\n",
        "    @gF@W: Forest, @DM@W: Mountain, @yH@W: Hills@n\r\n",
        "    @CS@W: Sky, @BW@W: Water, @bU@W: Underwater@n\r\n",
        "    @m$@W: Shop, @m#@W: Important, @YD@W: Desert@n\r\n",
        "    @c~@W: Shallow Water, @4 @n@W: Lava, @RX@W: You@n\r\n"};

    if (key < 5)
    {
        ch->send_to("%s%s", buf, key_strings[key]);
    }
    else
    {
        ch->sendText(buf);
    }
}

static void gen_map(const Location& loc, Character *ch, int num)
{
    char map[9][10] = {{'-'}, {'-'}};
    char buf2[MAX_INPUT_LENGTH];

    if (num == 1)
    {
        print_map_key(ch);
    }

    initialize_map(map);
    // Create a fake Destination for our start node.
    Destination start;
    start.al = loc.al;
    start.position = loc.position;

    map_draw_room(map, 4, 4, start, ch);

    auto exits = start.getExits();

    for (auto &[door, e] : exits)
    {

        switch (door)
        {
        case Direction::north:
            map_draw_room(map, 4, 3, e, ch);
            break;
        case Direction::east:
            map_draw_room(map, 5, 4, e, ch);
            break;
        case Direction::south:
            map_draw_room(map, 4, 5, e, ch);
            break;
        case Direction::west:
            map_draw_room(map, 3, 4, e, ch);
            break;
        case Direction::northeast:
            map_draw_room(map, 5, 3, e, ch);
            break;
        case Direction::northwest:
            map_draw_room(map, 3, 3, e, ch);
            break;
        case Direction::southeast:
            map_draw_room(map, 5, 5, e, ch);
            break;
        case Direction::southwest:
            map_draw_room(map, 3, 5, e, ch);
            break;
        }
    }

    map[4][4] = 'x';

    int key = 0;
    auto goodexit = [&exits](Direction dir)
    {
        return exits.contains(dir) && !EXIT_FLAGGED(&exits.at(dir), EX_SECRET);
    };
    auto isclosed = [&exits](Direction dir) {
        return EXIT_FLAGGED(&exits.at(dir), EX_CLOSED);
    };

    for (int i = 2; i < 9; i++)
    {
        if (i > 6)
            continue;

        switch (i)
        {
        case 2:
            sprintf(buf2, "@w       @w|%s@w|           %s",
                    goodexit(Direction::north) ? (isclosed(Direction::north) ? " @rN " : " @CN ") : "   ", map[i]);
            break;
        case 3:
            sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
                    goodexit(Direction::northwest) ? (isclosed(Direction::northwest) ? " @rNW" : " @CNW") : "   ",
                    goodexit(Direction::up) ? (isclosed(Direction::up) ? " @yU " : " @YU ") : "   ",
                    goodexit(Direction::northeast) ? (isclosed(Direction::northeast) ? "@rNE " : "@CNE ") : "   ", map[i]);
            break;
        case 4:
            sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
                    goodexit(Direction::west) ? (isclosed(Direction::west) ? "  @rW" : "  @CW") : "   ",
                    goodexit(Direction::inside) ? (isclosed(Direction::inside) ? " @rI " : " @mI ") : (goodexit(Direction::outside) ? (isclosed(Direction::outside) ? "@rOUT" : "@mOUT") : "@r{ }"),
                    goodexit(Direction::east) ? (isclosed(Direction::east) ? "@rE  " : "@CE  ") : "   ", map[i]);
            break;
        case 5:
            sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|     %s",
                    goodexit(Direction::southwest) ? (isclosed(Direction::southwest) ? " @rSW" : " @CSW") : "   ",
                    goodexit(Direction::down) ? (isclosed(Direction::down) ? " @yD " : " @YD ") : "   ",
                    goodexit(Direction::southeast) ? (isclosed(Direction::southeast) ? "@rSE " : "@CSE ") : "   ", map[i]);
            break;
        case 6:
            sprintf(buf2, "@w       @w|%s@w|           %s",
                    goodexit(Direction::south) ? (isclosed(Direction::south) ? " @rS " : " @CS ") : "   ", map[i]);
            break;
        }

        replace_map_symbols(buf2);

        if (num != 1)
        {
            print_map_line(ch, key, buf2);
            key++;
        }
        else
        {
            ch->sendText(buf2);
        }
    }

    if (num == 1)
    {
        ch->sendText("@D                ---------@w\r\n");
    }
}

static void display_spells(Character *ch, Object *obj)
{
    int i;

    ch->sendText("The spellbook contains the following spells:\r\n");
    ch->sendText("@c---@wSpell Name@c------------------------------------@w# of pages@c-----@n\r\n");

    return;
}

static void display_scroll(Character *ch, Object *obj)
{
    ch->sendText("The scroll contains the following spell:\r\n");
    ch->sendText("@c---@wSpell Name@c---------------------------------------------------@n\r\n");
    ch->send_to("@y%-20s@n\r\n", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1)));
    return;
}

void show_obj_to_char(Object *obj, Character *ch, int mode)
{
    if (!obj || !ch)
    {
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

    if (GET_SKILL(ch, SKILL_SPOT) > rand_number(20, 110))
    {
        spotted = true;
    }

    const auto tile = obj->location.getTileType();
    switch (mode)
    {
    case SHOW_OBJ_LONG:
        /*
         * hide objects starting with . from non-holylighted people
         * Idea from Elaseth of TBA
         */
        if (*obj->getRoomDescription() == '.' && (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
            return;
        if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE && ch->location.getVnum() == GET_OBJ_VAL(obj, VAL_VEHICLE_DEST))
        {
            return;
        }
        if (SITTING(obj) && GET_ADMLEVEL(ch) < 1)
        {
            return;
        }
        if (SITTING(obj) && GET_ADMLEVEL(ch) >= 1)
        {
            ch->sendText("@D(@YBeing Used@D)@w");
        }
        if (GET_OBJ_TYPE(obj) == ITEM_PLANT &&
            (obj->location.getRoomFlag(ROOM_GARDEN1) || obj->location.getRoomFlag(ROOM_GARDEN2)))
        {
            see_plant(obj, ch);
            return;
        }

        if (OBJ_FLAGGED(obj, ITEM_BURIED))
        {
            char bury[MAX_INPUT_LENGTH];
            if (!IS_CORPSE(obj))
            {
                if (GET_OBJ_WEIGHT(obj) < 10)
                {
                    sprintf(bury, "small mound of");
                }
                else if (GET_OBJ_WEIGHT(obj) < 50)
                {
                    sprintf(bury, "medium sized mound of");
                }
                else if (GET_OBJ_WEIGHT(obj) < 1000)
                {
                    sprintf(bury, "large mound of");
                }
                else
                {
                    sprintf(bury, "gigantic mound of");
                }
            }
            else
            {
                sprintf(bury, "recent grave covered by");
            }
            if (spotted == true && tile != SECT_DESERT)
            {
                ch->send_to("@yA %s soft dirt is here.@n\r\n", bury);
                return;
            }
            else if (spotted == true && tile == SECT_DESERT)
            {
                ch->send_to("@YA %s soft sand is here.@n\r\n", bury);
                return;
            }
            else
            {
                return;
            }
        }
        if (GET_OBJ_VNUM(obj) == 11)
        {
            if (auto g = obj->getBaseStat("gravity"); g > 0.0)
            {
                auto msg = fmt::format("@wA gravity generator, set to {}x gravity, is built here", g);
                ch->sendText(msg.c_str());
            }
            else
            {
                ch->sendText("@wA gravity generator, currently on standby, is built here");
            }
        }
        else if (GET_OBJ_VNUM(obj) == 79)
        {
            ch->send_to("@wA @cG@Cl@wa@cc@Ci@wa@cl @wW@ca@Cl@wl @D[@C%s@D]@w is blocking access to the @G%s@w direction", add_commas(GET_OBJ_WEIGHT(obj)).c_str(), dirs[GET_OBJ_COST(obj)]);
        }
        else
        {
            ch->sendText("@w");
            if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
            {
                if (GET_OBJ_POSTED(obj) == nullptr)
                {
                    ch->send_to("@D[@G%d@D]@w ", GET_OBJ_VNUM(obj));
                    if (!obj->getProtoScript().empty())
                        ch->send_to("%s ", obj->scriptString().c_str());
                }
                else
                {
                    if (GET_OBJ_POSTTYPE(obj) <= 0)
                    {
                        ch->send_to("@D[@G%d@D]@w ", GET_OBJ_VNUM(obj));
                        if (!obj->getProtoScript().empty())
                            ch->send_to("%s ", obj->scriptString().c_str());
                    }
                }
            }

            if (GET_OBJ_POSTTYPE(obj) > 0)
            {
                if (GET_OBJ_POSTED(obj))
                {
                    return;
                }
                else
                {
                    ch->send_to("%s@w, has been posted here.@n", obj->getShortDescription());
                }
            }
            else
            {
                if (!OBJ_FLAGGED(obj, ITEM_BURIED))
                {
                    ch->send_to("%s@n", obj->getRoomDescription());
                }
            }

            if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE)
            {
                if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && GET_OBJ_VNUM(obj) > 19199)
                    ch->sendText("\r\n@c...its outer hatch is open@n");
                else if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && GET_OBJ_VNUM(obj) <= 19199)
                    ch->sendText("\r\n@c...its door is open@n");
            }
            if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !IS_CORPSE(obj))
            {
                if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && !OBJ_FLAGGED(obj, ITEM_SHEATH))
                    ch->sendText(". @D[@G-open-@D]@n");
                else if (!OBJ_FLAGGED(obj, ITEM_SHEATH))
                    ch->sendText(". @D[@rclosed@D]@n");
            }
            if (GET_OBJ_TYPE(obj) == ITEM_HATCH)
            {
                if (!OBJVAL_FLAGGED(obj, CONT_CLOSED))
                    ch->sendText(", it is open");
                else if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
                    ch->sendText(", it is closed");
                if (OBJVAL_FLAGGED(obj, CONT_LOCKED))
                    ch->sendText(" and locked@n");
                else
                    ch->sendText("@n");
            }
            if (GET_OBJ_TYPE(obj) == ITEM_FOOD)
            {
                if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj))
                {
                    ch->sendText(", and it has been ate on@n");
                }
            }
        }
        break;

    case SHOW_OBJ_SHORT:
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
        {
            ch->send_to("[%d] ", GET_OBJ_VNUM(obj));
            if (!obj->getProtoScript().empty())
                ch->send_to("%s ", obj->scriptString().c_str());
        }

        if (PRF_FLAGGED(ch, PRF_IHEALTH))
        {
            ch->send_to("@D<@gH@D: @C%d@D>@w %s", GET_OBJ_VAL(obj, VAL_ALL_HEALTH), obj->getShortDescription());
        }
        else
        {
            ch->send_to("%s", obj->getShortDescription());
        }
        if (GET_OBJ_TYPE(obj) == ITEM_FOOD)
        {
            if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj))
            {
                ch->sendText(", and it has been ate on.@n");
            }
        }
        if (GET_OBJ_VNUM(obj) == 255)
        {
            switch (GET_OBJ_VAL(obj, VAL_OTHER_SOILQUALITY))
            {
            case 0:
            case 1:
                ch->sendText(" @D[@wQuality @RC@D]@n");
                break;
            case 2:
                ch->sendText(" @D[@wQuality @RC+@D]@n");
                break;
            case 3:
                ch->sendText(" @D[@wQuality @yC++@D]@n");
                break;
            case 4:
                ch->sendText(" @D[@wQuality @yB@D]@n");
                break;
            case 5:
                ch->sendText(" @D[@wQuality @CB+@D]@n");
                break;
            case 6:
                ch->sendText(" @D[@wQuality @CB++@D]@n");
                break;
            case 7:
                ch->sendText(" @D[@wQuality @CA@D]@n");
                break;
            case 8:
                ch->sendText(" @D[@wQuality @GA+@D]@n");
                break;
            }
        }

        if (GET_OBJ_VNUM(obj) == 3424)
        {
            ch->send_to(" @D[@bInk Remaining@D: @w%d@D]@n", GET_OBJ_VAL(obj, VAL_OTHER_SERAF));
        }
        if (GET_OBJ_VNUM(obj) == 3423)
        {
            ch->send_to(" @D[@B%d@D/@B24 Inks@D]@n", GET_OBJ_VAL(obj, VAL_OTHER_SERAF));
        }
        if (OBJ_FLAGGED(obj, ITEM_THROW))
        {
            ch->sendText(" @D[@RThrow Only@D]@n");
        }
        if (GET_OBJ_TYPE(obj) == ITEM_PLANT && !OBJ_FLAGGED(obj, ITEM_MATURE))
        {
            if (GET_OBJ_VAL(obj, VAL_PLANT_WATERLEVEL) < -9)
            {
                ch->sendText("@D[@RDead@D]@n");
            }
            else
            {
                switch (GET_OBJ_VAL(obj, VAL_PLANT_MATURITY))
                {
                case 0:
                    ch->sendText(" @D[@ySeed@D]@n");
                    break;
                case 1:
                    ch->sendText(" @D[@GSprout@D]@n");
                    break;
                case 2:
                    ch->sendText(" @D[@GYoung@D]@n");
                    break;
                case 3:
                    ch->sendText(" @D[@GMature@D]@n");
                    break;
                case 4:
                    ch->sendText(" @D[@GBudding@D]@n");
                    break;
                case 5:
                    ch->sendText("@D[@GClose Harvest@D]@n");
                    break;
                case 6:
                    ch->sendText("@D[@gHarvest@D]@n");
                    break;
                }
            }
        }
        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !IS_CORPSE(obj))
        {
            if (!OBJVAL_FLAGGED(obj, CONT_CLOSED) && !OBJ_FLAGGED(obj, ITEM_SHEATH))
                ch->sendText(" @D[@G-open-@D]@n");
            else if (!OBJ_FLAGGED(obj, ITEM_SHEATH))
                ch->sendText(" @D[@rclosed@D]@n");
        }
        if (OBJ_FLAGGED(obj, ITEM_DUPLICATE))
        {
            ch->sendText(" @D[@YDuplicate@D]@n");
        }
        break;

    case SHOW_OBJ_ACTION:
        switch (GET_OBJ_TYPE(obj))
        {
        case ITEM_NOTE:
            if (auto ld = obj->getLookDescription(); ld)
            {
                char notebuf[MAX_NOTE_LENGTH];

                snprintf(notebuf, sizeof(notebuf), "There is something written on it:\r\n\r\n%s",
                         ld);
                ch->desc->sendText(notebuf);
            }
            else
                ch->sendText("There appears to be nothing written on it.\r\n");
            return;

        case ITEM_BOARD:
            show_board(GET_OBJ_VNUM(obj), ch);
            break;

        case ITEM_CONTROL:
            ch->send_to("@RFUEL@D: %s%s@n\r\n", GET_FUEL(obj) >= 200 ? "@G" : GET_FUEL(obj) >= 100 ? "@Y"
                                                                                                   : "@r",
                        add_commas(GET_FUEL(obj)).c_str());
            break;

        case ITEM_DRINKCON:
            ch->sendText("It looks like a drink container.\r\n");
            break;

        case ITEM_LIGHT:
            if (GET_OBJ_VAL(obj, VAL_LIGHT_HOURS) == -1)
                ch->sendText("Light Cycles left: Infinite\r\n");
            else
                ch->send_to("Light Cycles left: [%d]\r\n", GET_OBJ_VAL(obj, VAL_LIGHT_HOURS));
            break;

        case ITEM_FOOD:
            if (FOOB(obj) >= 4)
            {
                if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj) / 4)
                {
                    ch->sendText("Condition of the food: Almost gone.\r\n");
                }
                else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj) / 2)
                {
                    ch->sendText("Condition of the food: Half Eaten.");
                }
                else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj))
                {
                    ch->sendText("Condition of the food: Partially Eaten.");
                }
                else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) == FOOB(obj))
                {
                    ch->sendText("Condition of the food: Whole.");
                }
            }
            else if (FOOB(obj) > 0)
            {
                if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) < FOOB(obj))
                {
                    ch->sendText("Condition of the food: Almost gone.");
                }
                else if (GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL) == FOOB(obj))
                {
                    ch->sendText("Condition of the food: Whole.");
                }
            }
            else
            {
                ch->sendText("Condition of the food: Insignificant.");
            }
            break;

        case ITEM_SPELLBOOK:
            ch->sendText("It looks like an arcane tome.\r\n");
            display_spells(ch, obj);
            break;

        case ITEM_SCROLL:
            ch->sendText("It looks like an arcane scroll.\r\n");
            display_scroll(ch, obj);
            break;

        case ITEM_VEHICLE:
            if (GET_OBJ_VNUM(obj) > 19199)
            {
                ch->sendText("@YSyntax@D: @CUnlock hatch\r\n");
                ch->sendText("@YSyntax@D: @COpen hatch\r\n");
                ch->sendText("@YSyntax@D: @CClose hatch\r\n");
                ch->sendText("@YSyntax@D: @CEnter hatch\r\n");
            }
            else
            {
                ch->sendText("@YSyntax@D: @CUnlock door\r\n");
                ch->sendText("@YSyntax@D: @COpen door\r\n");
                ch->sendText("@YSyntax@D: @CClose door\r\n");
                ch->sendText("@YSyntax@D: @CEnter door\r\n");
            }
            break;

        case ITEM_HATCH:
            if (GET_OBJ_VNUM(obj) > 19199)
            {
                ch->sendText("@YSyntax@D: @CUnlock hatch\r\n");
                ch->sendText("@YSyntax@D: @COpen hatch\r\n");
                ch->sendText("@YSyntax@D: @CClose hatch\r\n");
                ch->sendText("@YSyntax@D: @CLeave@n\r\n");
            }
            else
            {
                ch->sendText("@YSyntax@D: @CUnlock door\r\n");
                ch->sendText("@YSyntax@D: @COpen door\r\n");
                ch->sendText("@YSyntax@D: @CClose door\r\n");
                ch->sendText("@YSyntax@D: @CEnter door\r\n");
            }
            break;

        case ITEM_WINDOW:
            look_out_window(ch, obj->getName());
            return;
            break;

        default:
            if (!IS_CORPSE(obj))
            {
                ch->sendText("You see nothing special.\r\n");
            }
            else
            {
                ch->sendText("This corpse has ");
                bool mention = false;

                const struct
                {
                    const char *value;
                    const char *no_part;
                    const char *broken_part;
                } corpse_parts[] = {
                    {VAL_CORPSE_HEAD, "no head,", nullptr},
                    {VAL_CORPSE_RARM, "no right arm, ", "a broken right arm, "},
                    {VAL_CORPSE_LARM, "no left arm, ", "a broken left arm, "},
                    {VAL_CORPSE_RLEG, "no right leg, ", "a broken right leg, "},
                    {VAL_CORPSE_LLEG, "no left leg, ", "a broken left leg, "}};

                for (const auto &part : corpse_parts)
                {
                    int part_status = GET_OBJ_VAL(obj, part.value);
                    if (part_status == 0)
                    {
                        ch->sendText(part.no_part);
                        mention = true;
                    }
                    else if (part_status == 2 && part.broken_part)
                    {
                        ch->sendText(part.broken_part);
                        mention = true;
                    }
                }

                if (!mention)
                {
                    ch->sendText("nothing missing from it but life.");
                }
                else
                {
                    ch->sendText("and is dead.");
                }

                ch->sendText("\r\n");
            }
            break;
        }

        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        {
            int num = 0;
            if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT)
            {
                num = 1;
            }
            else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT)
            {
                num = 0;
            }
            else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT)
            {
                num = 3;
            }
            else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT)
            {
                num = 2;
            }
            else if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT)
            {
                num = 4;
            }
            else
            {
                num = 5;
            }
            ch->send_to("The weapon type of %s@n is '%s'.\r\n", GET_OBJ_SHORT(obj), weapon_disp[num]);
            ch->send_to("You could wield it %s.\r\n", wield_names[wield_type(get_size(ch), obj)]);
        }
        diag_obj_to_char(obj, ch);
        ch->send_to("It appears to be made of %s, and weighs %s", material_names[GET_OBJ_MATERIAL(obj)], add_commas(GET_OBJ_WEIGHT(obj)).c_str());
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
        ch->sendText("\r\n");
}

static int show_obj_modifiers(Object *obj, Character *ch)
{
    int found = false;

    if (OBJ_FLAGGED(obj, ITEM_INVISIBLE))
    {
        ch->sendText(" (invisible)");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
    {
        ch->sendText(" ..It glows blue!");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
    {
        ch->sendText(" ..It glows yellow!");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_GLOW))
    {
        ch->sendText(" @D(@GGlowing@D)@n");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_HOT))
    {
        ch->sendText(" @D(@RHOT@D)@n");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_HUM))
    {
        ch->sendText(" @D(@RHumming@D)@n");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_SLOT2))
    {
        if (OBJ_FLAGGED(obj, ITEM_SLOT_ONE) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
            ch->sendText(" @D[@m1/2 Tokens@D]@n");
        else if (OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
            ch->sendText(" @D[@m2/2 Tokens@D]@n");
        else
            ch->sendText(" @D[@m0/2 Tokens@D]@n");
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_SLOT1))
    {
        if (OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
            ch->sendText(" @D[@m1/1 Tokens@D]@n");
        else
            ch->sendText(" @D[@m0/1 Tokens@D]@n");
        found++;
    }
    if (KICHARGE(obj) > 0)
    {
        int num = (KIDIST(obj) * 20) + rand_number(1, 5);
        ch->send_to(" %d meters away", num);
        found++;
    }
    if (OBJ_FLAGGED(obj, ITEM_CUSTOM))
    {
        ch->sendText(" @D(@YCUSTOM@D)@n");
    }
    if (OBJ_FLAGGED(obj, ITEM_RESTRING))
    {
        ch->send_to(" @D(@R%s@D)@n", GET_ADMLEVEL(ch) > 0 ? obj->getName() : "*");
    }
    if (OBJ_FLAGGED(obj, ITEM_BROKEN))
    {
        if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STEEL ||
            GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL ||
            GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_METAL)
        {
            ch->sendText(", and appears to be twisted and broken.");
        }
        else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_WOOD)
        {
            ch->sendText(", and is broken into hundreds of splinters.");
        }
        else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_GLASS)
        {
            ch->sendText(", and is shattered on the ground.");
        }
        else if (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL) == MATERIAL_STONE)
        {
            ch->sendText(", and is a pile of rubble.");
        }
        else
        {
            ch->sendText(", and is broken.");
        }
        found++;
    }
    else
    {
        if (GET_OBJ_TYPE(obj) != ITEM_BOARD)
        {
            if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
            {
                ch->sendText(".");
            }
            if (!IS_NPC(ch) && GET_OBJ_POSTED(obj) && GET_OBJ_POSTTYPE(obj) <= 0)
            {
                Object *obj2 = GET_OBJ_POSTED(obj);
                char dvnum[200];
                *dvnum = '\0';
                sprintf(dvnum, "@D[@G%d@D] @w", GET_OBJ_VNUM(obj2));
                ch->send_to("\n...%s%s has been posted to it.", PRF_FLAGGED(ch, PRF_ROOMFLAGS) ? dvnum : "", obj2->getShortDescription());
            }
        }
        found++;
    }
    return (found);
}

static void list_obj_to_char(const std::vector<std::weak_ptr<Object>> &list, Character *ch, int mode, int show)
{
    Object *d;
    bool found = false;
    int num;

    for (auto i : filter_raw(list))
    {
        if (i->getRoomDescription() == nullptr || strcasecmp(i->getRoomDescription(), "undefined") == 0)
            continue;

        num = 0;
        d = i;

        if (CAN_SEE_OBJ(ch, d) &&
                ((*d->getRoomDescription() != '.' && *d->getShortDescription() != '.') || PRF_FLAGGED(ch, PRF_HOLYLIGHT)) ||
            (GET_OBJ_TYPE(d) == ITEM_LIGHT))
        {
            show_obj_to_char(d, ch, mode);
            found = true;
        }
    }

    if (!found && show)
        ch->sendText(" Nothing.\r\n");
}

static void diag_obj_to_char(Object *obj, Character *ch)
{
    struct
    {
        int percent;
        const char *text;
    } diagnosis[] = {
        {100, "is in excellent condition."},
        {90, "has a few scuffs."},
        {75, "has some small scuffs and scratches."},
        {50, "has quite a few scratches."},
        {30, "has some big nasty scrapes and scratches."},
        {15, "looks pretty damaged."},
        {0, "is in awful condition."},
        {-1, "is in need of repair."},
    };
    int percent, ar_index;
    const char *objs = OBJS(obj, ch);

    if (GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) > 0)
        percent = (100 * GET_OBJ_VAL(obj, VAL_ALL_HEALTH)) / GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH);
    else
        percent = 0; /* How could MAX_HIT be < 1?? */

    for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
        if (percent >= diagnosis[ar_index].percent)
            break;

    ch->send_to("\r\n%c%s %s\r\n", UPPER(*objs), objs + 1, diagnosis[ar_index].text);
}

static void diag_char_to_char(Character *i, Character *ch)
{
    struct
    {
        int percent;
        const char *text;
    } diagnosis[] = {
        {100, "@wis in @Gexcellent@w condition."},
        {90, "@whas a few @Rscratches@w."},
        {80, "@whas some small @Rwounds@w and @Rbruises@w."},
        {70, "@whas quite a few @Rwounds@w."},
        {60, "@whas some big @rnasty wounds@w and @Rscratches@w."},
        {50, "@wlooks pretty @rhurt@w."},
        {40, "@wis mainly @rinjured@w."},
        {30, "@wis a @rmess@w of @rinjuries@w."},
        {20, "@wis @rstruggling@w to @msurvive@w."},
        {10, "@wis in @mawful condition@w."},
        {0, "@Ris barely alive.@w"},
        {-1, "@ris nearly dead.@w"},
    };
    int percent, ar_index;

    int64_t hit = GET_HIT(i), max = (i->getEffectiveStat<int64_t>("health"));

    int64_t total = max;

    if (hit == total)
    {
        percent = 100;
    }
    else if (hit < total && hit >= (total * .9))
    {
        percent = 90;
    }
    else if (hit < total && hit >= (total * .8))
    {
        percent = 80;
    }
    else if (hit < total && hit >= (total * .7))
    {
        percent = 70;
    }
    else if (hit < total && hit >= (total * .6))
    {
        percent = 60;
    }
    else if (hit < total && hit >= (total * .5))
    {
        percent = 50;
    }
    else if (hit < total && hit >= (total * .4))
    {
        percent = 40;
    }
    else if (hit < total && hit >= (total * .3))
    {
        percent = 30;
    }
    else if (hit < total && hit >= (total * .2))
    {
        percent = 20;
    }
    else if (hit < total && hit >= (total * .1))
    {
        percent = 10;
    }
    else if (hit < total * .1)
    {
        percent = 0;
    }
    else
    {
        percent = -1; /* How could MAX_HIT be < 1?? */
    }

    for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
        if (percent >= diagnosis[ar_index].percent)
            break;

    ch->send_to("%s\r\n", diagnosis[ar_index].text);
}

void display_limb(Character *ch, Character *i, int limb_index, const char *limb_display, CharacterFlag cyber_flag)
{
    int cond = GET_LIMBCOND(i, limb_index);
    bool isCyber = i->character_flags.get(cyber_flag);
    if (cond >= 50 && !isCyber)
    {
        ch->send_to("            @D[@c%-12s@D: @G%2d%%@D/@g100%%        @D]@n\r\n", limb_display, cond);
    }
    else if (cond > 0 && !isCyber)
    {
        ch->send_to("            @D[@c%-12s@D: @rBroken @y%2d%%@D/@g100%% @D]@n\r\n", limb_display, cond);
    }
    else if (cond > 0 && isCyber)
    {
        ch->send_to("            @D[@c%-12s@D: @cCybernetic @G%2d%%@D/@G100%%@D]@n\r\n", limb_display, cond);
    }
    else
    { // cond <= 0
        ch->send_to("            @D[@c%-12s@D: @rMissing.            @D]@n\r\n", limb_display);
    }
}

static void look_at_char(Character *i, Character *ch)
{
    int j, found, clan = false;
    char buf[100];
    Object *tmp_obj;

    if (!ch->desc)
    {
        return;
    }
    if (i->form == Form::base || !i->transforms[i->form].description.empty())
    {
        if (auto ld = i->getLookDescription(); ld)
        {
            ch->send_to("%s", ld);
        }
    }
    else
    {
        ch->send_to("%s", i->transforms[i->form].description);
    }

    ch->sendText("\r\n");
    if (!IS_NPC(i))
    {
        display_limb(ch, i, 0, "Right Arm", CharacterFlag::cyber_right_arm);
        display_limb(ch, i, 1, "Left Arm", CharacterFlag::cyber_left_arm);
        display_limb(ch, i, 2, "Right Leg", CharacterFlag::cyber_right_leg);
        display_limb(ch, i, 3, "Left Leg", CharacterFlag::cyber_left_leg);
        if (race::hasTail(i->race) && !PLR_FLAGGED(i, PLR_TAILHIDE))
        {
            if (i->character_flags.get(CharacterFlag::tail))
                ch->sendText("            @D[@cTail        @D: @GHas.                 @D]@n\r\n");
            else
                ch->sendText("            @D[@cTail        @D: @rMissing.             @D]@n\r\n");
        }
    }
    ch->sendText("\r\n");
    if (GET_CLAN(i) && !strstr(GET_CLAN(i), "None"))
    {
        sprintf(buf, "%s", GET_CLAN(i));
        clan = true;
    }
    if (GET_CLAN(i) == nullptr)
    {
        clan = false;
    }
    if (!IS_NPC(i))
    {
        ch->send_to("            @D[@mClan        @D: @W%-20s@D]@n\r\n", clan ? buf : "None.");
    }
    if (!IS_NPC(i))
    {
        ch->sendText("\r\n         @D----------------------------------------@n\r\n");
        trans_check(ch, i);
        ch->sendText("         @D----------------------------------------@n\r\n");
    }
    ch->sendText("\r\n");

    if ((!PLR_FLAGGED(i, PLR_DISGUISED) && (readIntro(ch, i) == 1 && !IS_NPC(i))))
    {
        if (GET_SEX(i) == SEX_NEUTRAL)
            ch->send_to("%s appears to be %s %s, ", get_i_name(ch, i), AN(RACE(i)), LRACE(i));
        else
            ch->send_to("%s appears to be %s %s %s, ", get_i_name(ch, i), AN(MAFE(i)), MAFE(i), LRACE(i));
    }
    else if (ch == i || IS_NPC(i))
    {
        if (GET_SEX(i) == SEX_NEUTRAL)
            ch->send_to("%c%s appears to be %s %s, ", UPPER(*GET_NAME(i)), GET_NAME(i) + 1, AN(RACE(i)), LRACE(i));
        else
            ch->send_to("%c%s appears to be %s %s %s, ", UPPER(*GET_NAME(i)), GET_NAME(i) + 1, AN(MAFE(i)), MAFE(i), LRACE(i));
    }
    else
    {
        if (GET_SEX(i) == SEX_NEUTRAL)
            ch->send_to("Appears to be %s %s, ", AN(RACE(i)), LRACE(i));
        else
            ch->send_to("Appears to be %s %s %s, ", AN(MAFE(i)), MAFE(i), LRACE(i));
    }

    if (IS_NPC(i))
    {
        ch->send_to("is %s sized, and\r\n", size_names[get_size(i)]);
    }
    if (!IS_NPC(i))
    {
        auto w = i->getEffectiveStat("weight");
        int h = i->getEffectiveStat("height");
        auto wString = fmt::format("{}kg", w);
        ch->send_to("is %s sized, about %dcm tall,\r\nabout %s heavy,", size_names[get_size(i)], h, wString.c_str());

        if (i == ch)
        {
            ch->sendText(" and ");
        }
        else if (GET_AGE(ch) >= GET_AGE(i) + 30)
        {
            ch->sendText(" appears to be very much younger than you, and ");
        }
        else if (GET_AGE(ch) >= GET_AGE(i) + 25)
        {
            ch->sendText(" appears to be much younger than you, and ");
        }
        else if (GET_AGE(ch) >= GET_AGE(i) + 15)
        {
            ch->sendText(" appears to be a good amount younger than you, and ");
        }
        else if (GET_AGE(ch) >= GET_AGE(i) + 10)
        {
            ch->sendText(" appears to be about a decade younger than you, and ");
        }
        else if (GET_AGE(ch) >= GET_AGE(i) + 5)
        {
            ch->sendText(" appears to be several years younger than you, and ");
        }
        else if (GET_AGE(ch) >= GET_AGE(i) + 2)
        {
            ch->sendText(" appears to be a bit younger than you, and ");
        }
        else if (GET_AGE(ch) > GET_AGE(i))
        {
            ch->sendText(" appears to be slightly younger than you, and ");
        }
        else if (GET_AGE(ch) == GET_AGE(i))
        {
            ch->sendText(" appears to be the same age as you, and ");
        }
        if (GET_AGE(i) >= GET_AGE(ch) + 30)
        {
            ch->sendText(" appears to be very much older than you, and ");
        }
        else if (GET_AGE(i) >= GET_AGE(ch) + 25)
        {
            ch->sendText(" appears to be much older than you, and ");
        }
        else if (GET_AGE(i) >= GET_AGE(ch) + 15)
        {
            ch->sendText(" appears to be a good amount older than you, and ");
        }
        else if (GET_AGE(i) >= GET_AGE(ch) + 10)
        {
            ch->sendText(" appears to be about a decade older than you, and ");
        }
        else if (GET_AGE(i) >= GET_AGE(ch) + 5)
        {
            ch->sendText(" appears to be several years older than you, and ");
        }
        else if (GET_AGE(i) >= GET_AGE(ch) + 2)
        {
            ch->sendText(" appears to be a bit older than you, and ");
        }
        else if (GET_AGE(i) > GET_AGE(ch))
        {
            ch->sendText(" appears to be slightly older than you, and ");
        }
    }
    diag_char_to_char(i, ch);
    found = false;
    for (j = 0; !found && j < NUM_WEARS; j++)
        if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
            found = true;

    if (found && (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOEQSEE)))
    {
        ch->sendText("\r\n"); /* act() does capitalization. */
        if (!PLR_FLAGGED(i, PLR_DISGUISED))
        {
            act("$n is using:", false, i, nullptr, ch, TO_VICT);
        }
        else
        {
            act("The disguised person is using:", false, i, nullptr, ch, TO_VICT);
        }
        for (j = 0; j < NUM_WEARS; j++)
            if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)) && (j != WEAR_WIELD1 && j != WEAR_WIELD2))
            {
                ch->send_to("%s", wear_where[j]);
                show_obj_to_char(GET_EQ(i, j), ch, SHOW_OBJ_SHORT);
                if (OBJ_FLAGGED(GET_EQ(i, j), ITEM_SHEATH))
                {
                    auto sheath = GET_EQ(i, j);
                    auto con = sheath->getInventory();
                    for (auto obj2 : filter_raw(con))
                    {
                        ch->sendText("@D  ---- @YSheathed@D ----@c> @n");
                        show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
                    }
                }
            }
            else if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)) && (!PLR_FLAGGED(i, PLR_THANDW)))
            {
                ch->send_to("%s", wear_where[j]);
                show_obj_to_char(GET_EQ(i, j), ch, SHOW_OBJ_SHORT);
                if (OBJ_FLAGGED(GET_EQ(i, j), ITEM_SHEATH))
                {
                    auto sheath = GET_EQ(i, j);
                    auto con = sheath->getInventory();
                    for (auto obj2 : filter_raw(con))
                    {
                        ch->sendText("@D  ---- @YSheathed@D ----@c> @n");
                        show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
                    }
                }
            }
            else if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)) && (PLR_FLAGGED(i, PLR_THANDW)))
            {
                ch->sendText("@c<@CWielded by B. Hands@c>@n ");
                show_obj_to_char(GET_EQ(i, j), ch, SHOW_OBJ_SHORT);
            }
    }
    if (ch != i && ((GET_SKILL(ch, SKILL_KEEN) && AFF_FLAGGED(ch, AFF_SNEAK)) || GET_ADMLEVEL(ch)))
    {
        found = false;
        act("\r\nYou attempt to peek at $s inventory:", false, i, nullptr, ch, TO_VICT);
        if (CAN_SEE(i, ch))
            act("$n tries to evaluate what you have in your inventory.", true, ch, nullptr, i, TO_VICT);
        if (GET_SKILL(ch, SKILL_KEEN) > axion_dice(0) && (!IS_NPC(i) || GET_ADMLEVEL(ch) > 1))
        {
            auto con = i->getInventory();
            for (auto tmp_obj : filter_raw(con))
            {
                if (CAN_SEE_OBJ(ch, tmp_obj) && (ADM_FLAGGED(ch, ADM_SEEINV) || (rand_number(0, 20) < GET_WIS(ch))))
                {
                    show_obj_to_char(tmp_obj, ch, SHOW_OBJ_SHORT);
                    found = true;
                }
            }
            improve_skill(ch, SKILL_KEEN, 1);
        }
        else if (IS_NPC(i) && GET_ADMLEVEL(ch) < 2)
        {
            return;
        }
        else
        {
            act("You are unsure about $s inventory.", false, i, nullptr, ch, TO_VICT);
            if (CAN_SEE(i, ch))
                act("$n didn't seem to get a good enough look.", true, ch, nullptr, i, TO_VICT);
            improve_skill(ch, SKILL_KEEN, 1);
            return;
        }
        if (!found)
        {
            ch->sendText("You can't see anything.\r\n");
            improve_skill(ch, SKILL_KEEN, 1);
        }
    }
}

void list_one_char(Character *i, Character *ch)
{
    Object *chair = nullptr;
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
        " is standing here"};

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS) && IS_NPC(i))
    {
        auto sString = !i->getProtoScript().empty() ? fmt::format("{} ", i->scriptString()) : "";
        ch->send_to("@D[@G%d@D]@w %s", GET_MOB_VNUM(i), sString.c_str());
    }

    if (IS_NPC(i) && i->getRoomDescription() && GET_POS(i) == GET_DEFAULT_POS(i) && !FIGHTING(i))
    {
        ch->send_to("%s", i->getRoomDescription());

        if (IS_NPC(i) && GET_HIT(i) >= (i->getEffectiveStat<int64_t>("health")) * .9 && GET_HIT(i) != (i->getEffectiveStat<int64_t>("health")))
            act("@R...Some slight wounds on $s body.@w", true, i, nullptr, ch, TO_VICT);
        else if (IS_NPC(i) && GET_HIT(i) >= (i->getEffectiveStat<int64_t>("health")) * .8 && GET_HIT(i) < (i->getEffectiveStat<int64_t>("health")) * .9)
            act("@R...A few wounds on $s body.@w", true, i, nullptr, ch, TO_VICT);
        else if (IS_NPC(i) && GET_HIT(i) >= (i->getEffectiveStat<int64_t>("health")) * .7 && GET_HIT(i) < (i->getEffectiveStat<int64_t>("health")) * .8)
            act("@R...Many wounds on $s body.@w", true, i, nullptr, ch, TO_VICT);
        else if (IS_NPC(i) && GET_HIT(i) >= (i->getEffectiveStat<int64_t>("health")) * .6 && GET_HIT(i) < (i->getEffectiveStat<int64_t>("health")) * .7)
            act("@R...Quite a few wounds on $s body.@w", true, i, nullptr, ch, TO_VICT);
        else if (IS_NPC(i) && GET_HIT(i) >= (i->getEffectiveStat<int64_t>("health")) * .5 && GET_HIT(i) < (i->getEffectiveStat<int64_t>("health")) * .6)
            act("@R...Horrible wounds on $s body.@w", true, i, nullptr, ch, TO_VICT);
        else if (IS_NPC(i) && GET_HIT(i) >= (i->getEffectiveStat<int64_t>("health")) * .4 && GET_HIT(i) < (i->getEffectiveStat<int64_t>("health")) * .5)
            act("@R...Blood is seeping from the wounds on $s body.@w", true, i, nullptr, ch, TO_VICT);
        else if (IS_NPC(i) && GET_HIT(i) >= (i->getEffectiveStat<int64_t>("health")) * .3 && GET_HIT(i) < (i->getEffectiveStat<int64_t>("health")) * .4)
            act("@R...$s body is in terrible shape.@w", true, i, nullptr, ch, TO_VICT);
        else if (IS_NPC(i) && GET_HIT(i) >= (i->getEffectiveStat<int64_t>("health")) * .2 && GET_HIT(i) < (i->getEffectiveStat<int64_t>("health")) * .3)
            act("@R...Is absolutely covered in wounds.@w", true, i, nullptr, ch, TO_VICT);
        else if (IS_NPC(i) && GET_HIT(i) >= (i->getEffectiveStat<int64_t>("health")) * .1 && GET_HIT(i) < (i->getEffectiveStat<int64_t>("health")) * .2)
            act("@R...Is on $s last leg.@w", true, i, nullptr, ch, TO_VICT);
        else if (IS_NPC(i) && GET_HIT(i) < (i->getEffectiveStat<int64_t>("health")) * .1)
            act("@R...Should be DEAD soon.@w", true, i, nullptr, ch, TO_VICT);

        if (GET_EAVESDROP(i) > 0)
        {
            char eaves[300];
            sprintf(eaves, "@w...$e is spying on everything to the @c%s@w.", dirs[GET_EAVESDIR(i)]);
            act(eaves, true, i, nullptr, ch, TO_VICT);
        }
        if (AFF_FLAGGED(i, AFF_FLYING) && GET_ALT(i) == 1)
            act("...$e is in the air!", false, i, nullptr, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_FLYING) && GET_ALT(i) == 2)
            act("...$e is high in the air!", false, i, nullptr, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_SANCTUARY) && !GET_SKILL(i, SKILL_AQUA_BARRIER))
            act("...$e has a barrier around $s body!", false, i, nullptr, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_FIRESHIELD))
            act("...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", false, i, nullptr, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_SANCTUARY) && GET_SKILL(i, SKILL_AQUA_BARRIER))
            act("...$e has a @Gbarrier@w of @cwater@w and @Cki@w around $s body!", false, i, nullptr, ch, TO_VICT);
        if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_SPIRAL))
            act("...$e is spinning in a vortex!", false, i, nullptr, ch, TO_VICT);
        if (GET_CHARGE(i))
            act("...$e has a bright %s aura around $s body!", false, i, nullptr, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_METAMORPH))
            act("@w...$e has a dark, @rred@w aura and menacing presence.", false, i, nullptr, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_HAYASA))
            act("@w...$e has a soft @cblue@w glow around $s body!", false, i, nullptr, ch, TO_VICT);
        if (AFF_FLAGGED(i, AFF_BLIND))
            act("...$e is groping around blindly!", false, i, nullptr, ch, TO_VICT);
        if (affected_by_spell(i, SPELL_FAERIE_FIRE))
            act("@m...$e @mis outlined with purple fire!@m", false, i, nullptr, ch, TO_VICT);
        if (GET_FEATURE(i))
        {
            char woo[MAX_STRING_LENGTH];
            sprintf(woo, "@C%s@n", GET_FEATURE(i));
            act(woo, false, i, nullptr, ch, TO_VICT);
        }

        return;
    }

    if (IS_NPC(i) && !FIGHTING(i) && GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING)
        ch->send_to("@w%c%s", UPPER(*i->getShortDescription()), i->getShortDescription() + 1);
    else if (IS_NPC(i) && GRAPPLED(i) && GRAPPLED(i) == ch)
        ch->send_to("@w%c%s is being grappled with by YOU!", UPPER(*i->getShortDescription()), i->getShortDescription() + 1);
    else if (IS_NPC(i) && GRAPPLED(i) && GRAPPLED(i) != ch)
        ch->send_to("@w%c%s is being absorbed from by %s!", UPPER(*i->getShortDescription()), i->getShortDescription() + 1, readIntro(ch, GRAPPLED(i)) == 1 ? get_i_name(ch, GRAPPLED(i)) : AN(RACE(GRAPPLED(i))));
    else if (IS_NPC(i) && ABSORBBY(i) && ABSORBBY(i) == ch)
        ch->send_to("@w%c%s is being absorbed from by YOU!", UPPER(*i->getShortDescription()), i->getShortDescription() + 1);
    else if (IS_NPC(i) && ABSORBBY(i) && ABSORBBY(i) != ch)
        ch->send_to("@w%c%s is being absorbed from by %s!", UPPER(*i->getShortDescription()), i->getShortDescription() + 1, readIntro(ch, ABSORBBY(i)) == 1 ? get_i_name(ch, ABSORBBY(i)) : AN(RACE(ABSORBBY(i))));
    else if (IS_NPC(i) && FIGHTING(i) && FIGHTING(i) != ch && GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING &&
             i->isSparring())
        ch->send_to("@w%c%s is sparring with %s!", UPPER(*i->getShortDescription()), i->getShortDescription() + 1, GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i)) : (readIntro(ch, FIGHTING(i)) == 1 ? get_i_name(ch, FIGHTING(i)) : LRACE(FIGHTING(i))));
    else if (IS_NPC(i) && FIGHTING(i) && i->isSparring() && FIGHTING(i) == ch && GET_POS(i) != POS_SITTING &&
             GET_POS(i) != POS_SLEEPING)
        ch->send_to("@w%c%s is sparring with you!", UPPER(*i->getShortDescription()), i->getShortDescription() + 1);
    else if (IS_NPC(i) && FIGHTING(i) && FIGHTING(i) != ch && GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING)
        ch->send_to("@w%c%s is fighting %s!", UPPER(*i->getShortDescription()), i->getShortDescription() + 1, GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i)) : (readIntro(ch, FIGHTING(i)) == 1 ? get_i_name(ch, FIGHTING(i)) : LRACE(FIGHTING(i))));
    else if (IS_NPC(i) && FIGHTING(i) && FIGHTING(i) == ch && GET_POS(i) != POS_SITTING && GET_POS(i) != POS_SLEEPING)
        ch->send_to("@w%c%s is fighting YOU!", UPPER(*i->getShortDescription()), i->getShortDescription() + 1);
    else if (IS_NPC(i) && FIGHTING(i) && GET_POS(i) == POS_SITTING)
        ch->send_to("@w%c%s is sitting here.", UPPER(*i->getShortDescription()), i->getShortDescription() + 1);
    else if (IS_NPC(i) && FIGHTING(i) && GET_POS(i) == POS_SLEEPING)
        ch->send_to("@w%c%s is sleeping here.", UPPER(*i->getShortDescription()), i->getShortDescription() + 1);
    else if (IS_NPC(i))
        ch->send_to("@w%c%s", UPPER(*i->getShortDescription()), i->getShortDescription() + 1);
    else if (!IS_NPC(i))
    {
        if (IS_MAJIN(i) && AFF_FLAGGED(i, AFF_LIQUEFIED))
        {
            ch->send_to("@wSeveral blobs of %s colored goo spread out here.@n\n", GET_SKIN(i));
            return;
        }
        if ((GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(i) > 0) || IS_NPC(ch))
        {
            ch->send_to("@w%s", i->getName());
        }
        else if ((!PLR_FLAGGED(i, PLR_DISGUISED) && readIntro(ch, i) == 1))
        {
            ch->send_to("@w%s", get_i_name(ch, i));
        }
        else if (!PLR_FLAGGED(i, PLR_DISGUISED) && readIntro(ch, i) != 1)
        {
            ch->send_to("@wA %s %s", MAFE(i), LRACE(i));
        }
        else
        {
            ch->send_to("@wA disguised %s %s", MAFE(i), LRACE(i));
        }
    }

    if (!IS_NPC(i) || !FIGHTING(i))
    {
        if (AFF_FLAGGED(i, AFF_INVISIBLE))
        {
            ch->sendText(", is invisible");
            count = true;
        }
        if (AFF_FLAGGED(i, AFF_ETHEREAL))
        {
            ch->sendText(", has a halo");
            count = true;
        }
        if (AFF_FLAGGED(i, AFF_HIDE) && i != ch)
        {
            ch->sendText(", is hiding");
            if (GET_SKILL(i, SKILL_HIDE) && !IS_NPC(ch) && i != ch)
            {
                improve_skill(i, SKILL_HIDE, 1);
            }
            count = true;
        }
        if (!IS_NPC(i) && !i->desc)
        {
            ch->sendText(", has a blank stare");
            count = true;
        }
        if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
        {
            ch->sendText(", is writing");
            count = true;
        }
        if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK))
        {
            ch->sendText(", is buildwalking");
            count = true;
        }
        if (!IS_NPC(i) && ABSORBING(i) && ABSORBING(i) != ch)
        {
            ch->send_to(", is absorbing from %s", GET_NAME(ABSORBING(i)));
            count = true;
        }
        if (!IS_NPC(i) && GRAPPLING(i) && GRAPPLING(i) != ch)
        {
            ch->send_to(", is grappling with %s", readIntro(ch, GRAPPLING(i)) == 1 ? get_i_name(ch, GRAPPLING(i)) : introd_calc(GRAPPLING(i)));
            count = true;
        }
        if (!IS_NPC(i) && CARRYING(i) && CARRYING(i) != ch)
        {
            ch->send_to(", is carrying %s", readIntro(ch, CARRYING(i)) == 1 ? get_i_name(ch, CARRYING(i)) : introd_calc(CARRYING(i)));
            count = true;
        }
        if (!IS_NPC(i) && CARRIED_BY(i) && CARRIED_BY(i) != ch)
        {
            ch->send_to(", is being carried by %s", readIntro(ch, CARRIED_BY(i)) == 1 ? get_i_name(ch, CARRIED_BY(i)) : introd_calc(CARRIED_BY(i)));
            count = true;
        }
        if (!IS_NPC(i) && GRAPPLING(i) && GRAPPLING(i) == ch)
        {
            ch->sendText(", is grappling with YOU");
            count = true;
        }
        if (!IS_NPC(i) && ABSORBING(i) && ABSORBING(i) == ch)
        {
            ch->sendText(", is absorbing from YOU");
            count = true;
        }
        if (!IS_NPC(i) && ABSORBING(ch) && ABSORBING(ch) == i)
        {
            ch->sendText(", is being absorbed from by YOU");
            count = true;
        }
        if (!IS_NPC(i) && GRAPPLING(ch) && GRAPPLING(ch) == i)
        {
            ch->sendText(", is being grappled with by YOU");
            count = true;
        }
        if (!IS_NPC(i) && CARRYING(ch) && CARRYING(ch) == i)
        {
            ch->sendText(", is being carried by you");
            count = true;
        }
        if (!IS_NPC(ch) && !IS_NPC(i) && FIGHTING(i))
        {
            if (!i->character_flags.get(CharacterFlag::sparring) ||
                (i->character_flags.get(CharacterFlag::sparring) && (!FIGHTING(i)->character_flags.get(CharacterFlag::sparring) || IS_NPC(FIGHTING(i)))))
            {
                ch->sendText(", is here fighting ");
            }
            if (i->character_flags.get(CharacterFlag::sparring) && FIGHTING(i)->character_flags.get(CharacterFlag::sparring))
            {
                ch->sendText(", is here sparring ");
            }
            if (FIGHTING(i) == ch)
            {
                ch->sendText("@rYOU@w");
                count = true;
            }
            else
            {
                if (i->location == FIGHTING(i)->location)
                {
                    ch->send_to("%s", GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i)) : (readIntro(ch, FIGHTING(i)) == 1 ? get_i_name(ch, FIGHTING(i)) : LRACE(FIGHTING(i))));
                    count = true;
                }
                else
                {
                    ch->sendText("someone who has already left!");
                }
            }
        }
    }
    if (SITS(i))
    {
        chair = SITS(i);
        if (PLR_FLAGGED(i, PLR_HEALT))
        {
            ch->sendText("@w is floating inside a healing tank.");
        }
        else if (count == true)
        {
            ch->send_to(",@w and%s on %s.", positions[(int)GET_POS(i)], chair->getShortDescription());
        }
        else if (count == false)
        {
            ch->send_to("@w%s on %s.", positions[(int)GET_POS(i)], chair->getShortDescription());
        }
    }
    else if (!PLR_FLAGGED(i, PLR_PILOTING) && !SITS(i) && (!IS_NPC(i) || !FIGHTING(i)))
    {
        if (count == true)
        {
            ch->send_to("@w, and%s.", positions[(int)GET_POS(i)]);
        }
        if (count == false)
        {
            ch->send_to("@w%s.", positions[(int)GET_POS(i)]);
        }
    }
    else if (PLR_FLAGGED(i, PLR_PILOTING))
    {
        ch->sendText("@w, is sitting in the pilot's chair.\r\n");
    }
    else
    {

        if (FIGHTING(i) && !IS_NPC(ch) && !IS_NPC(i))
        {
            if (!i->character_flags.get(CharacterFlag::sparring))
            {
                ch->sendText(", is here fighting ");
            }
            if (i->character_flags.get(CharacterFlag::sparring))
            {
                ch->sendText(", is here sparring ");
            }
            if (FIGHTING(i) == ch)
                ch->sendText("@rYOU@w!");
            else
            {
                if (i->location == FIGHTING(i)->location)
                    ch->send_to("%s!", GET_ADMLEVEL(ch) ? GET_NAME(FIGHTING(i)) : (readIntro(ch, FIGHTING(i)) == 1 ? get_i_name(ch, FIGHTING(i)) : LRACE(FIGHTING(i))));
                else
                    ch->sendText("someone who has already left!");
            }
        }
        else if (!IS_NPC(i))
        { /* NIL fighting pointer */
            ch->sendText(" is here struggling with thin air.");
        }
    }

    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
    {
        if (IS_EVIL(i))
            ch->sendText(" (@rRed@[3] Aura)");
        else if (IS_GOOD(i))
            ch->sendText(" (@bBlue@[3] Aura)");
    }
    if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_AFK))
        ch->sendText(" @D(@RAFK@D)");
    else if (!IS_NPC(i) && i->timer > 3)
        ch->sendText(" @D(@RIDLE@D)");
    ch->sendText("@n\r\n");

    if (auto eave = GET_EAVESDROP(i); eave > 0)
    {
        char eaves[300];
        sprintf(eaves, "@w...$e is spying on everything to the @c%s@w.", dirs[eave]);
        act(eaves, true, i, nullptr, ch, TO_VICT);
    }
    if (!IS_NPC(i))
    {
        if (PLR_FLAGGED(i, PLR_FISHING))
        {
            act("@w...$e is @Cfishing@w.@n", true, i, nullptr, ch, TO_VICT);
        }
    }
    if (PLR_FLAGGED(i, PLR_AURALIGHT))
    {
        char bloom[MAX_INPUT_LENGTH];
        sprintf(bloom, "...is surrounded by a bright %s aura.@n", GET_AURA(i));
        act(bloom, true, i, nullptr, ch, TO_VICT);
    }

    auto is_oozaru = (i->form == Form::oozaru || i->form == Form::golden_oozaru);

    if (AFF_FLAGGED(i, AFF_SANCTUARY) && !GET_SKILL(i, SKILL_AQUA_BARRIER))
        act("@w...$e has a @bbarrier@w around $s body!", true, i, nullptr, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_FIRESHIELD))
        act("@w...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!", false, i, nullptr, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_HEALGLOW))
        act("@w...$e has a serene @Cblue@Y glow@w around $s body.", true, i, nullptr, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_EARMOR))
        act("@w...$e has ghostly @Ggreen@w ethereal armor around $s body.", true, i, nullptr, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_SANCTUARY) && GET_SKILL(i, SKILL_AQUA_BARRIER))
        act("@w...$e has a @bbarrier@w of @cwater@w and @CKi@w around $s body!", true, i, nullptr, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_FLYING) && GET_ALT(i) == 1)
        act("@w...$e is in the air!", true, i, nullptr, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_FLYING) && GET_ALT(i) == 2)
        act("@w...$e is high in the air!", true, i, nullptr, ch, TO_VICT);
    if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_SPIRAL))
        act("@w...$e is spinning in a vortex!", false, i, nullptr, ch, TO_VICT);
    if (IS_TRANSFORMED(i) && !IS_ANDROID(i) && !IS_SAIYAN(i) && !IS_HALFBREED(i))
        act("@w...$e has energy crackling around $s body!", true, i, nullptr, ch, TO_VICT);
    if (GET_CHARGE(i) && !IS_SAIYAN(i) && !IS_HALFBREED(i))
    {
        char aura[MAX_INPUT_LENGTH];
        sprintf(aura, "@w...$e has a @Ybright@w %s aura around $s body!", GET_AURA(i));
        act(aura, true, i, nullptr, ch, TO_VICT);
    }
    if (!is_oozaru && GET_CHARGE(i) && !IS_TRANSFORMED(i) && (IS_SAIYAN(i) || IS_HALFBREED(i)))
    {
        char aura[MAX_INPUT_LENGTH];
        sprintf(aura, "@w...$e has a @Ybright@w %s aura around $s body!", GET_AURA(i));
        act(aura, true, i, nullptr, ch, TO_VICT);
    }
    if (i->form != Form::oozaru && !GET_CHARGE(i) && IS_TRANSFORMED(i) && (IS_SAIYAN(i) || IS_HALFBREED(i)))
        act("@w...$e has energy crackling around $s body!", true, i, nullptr, ch, TO_VICT);
    if (AFF_FLAGGED(ch, AFF_KYODAIKA))
        act("@w...$e has expanded $s body size@w!", true, i, nullptr, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_HAYASA))
        act("@w...$e has a soft @cblue@w glow around $s body!", false, i, nullptr, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_LIMIT_BREAKING))
        act("@w...$e has an overflowing aura around $s body!", false, i, nullptr, ch, TO_VICT);

    if (i->form != Form::base)
        act(trans::getExtra(i, i->form).c_str(), true, i, nullptr, ch, TO_VICT);
    if (i->technique != Form::base)
        act(trans::getExtra(i, i->technique).c_str(), true, i, nullptr, ch, TO_VICT);
    if (GET_FEATURE(i))
    {
        char woo[MAX_STRING_LENGTH];
        sprintf(woo, "@C%s@n", GET_FEATURE(i));
        act(woo, false, i, nullptr, ch, TO_VICT);
    }

    if (GET_RDISPLAY(i))
    {
        char rdis[MAX_STRING_LENGTH];
        sprintf(rdis, "...%s", GET_RDISPLAY(i));
        act(rdis, false, i, nullptr, ch, TO_VICT);
    }
}

struct hide_node
{
    struct hide_node *next;
    Character *hidden;
};

static void add_hidden_char(struct hide_node **hideinfo, Character *ch)
{
    struct hide_node *new_node = new hide_node{nullptr, ch};
    if (!*hideinfo)
    {
        *hideinfo = new_node;
    }
    else
    {
        struct hide_node *current = *hideinfo;
        while (current->next)
        {
            current = current->next;
        }
        current->next = new_node;
    }
}

static bool is_hidden(struct hide_node *hideinfo, Character *ch)
{
    for (auto node = hideinfo; node; node = node->next)
    {
        if (node->hidden == ch)
        {
            return true;
        }
    }
    return false;
}

static void list_char_to_char(const std::vector<std::weak_ptr<Character>> &list, Character *ch)
{
    struct hide_node *hideinfo = nullptr;

    for (auto i : filter_raw(list))
    {
        if (AFF_FLAGGED(i, AFF_HIDE) && roll_resisted(i, SKILL_HIDE, ch, SKILL_SPOT))
        {
            if (GET_SKILL(i, SKILL_HIDE) && !IS_NPC(ch) && i != ch)
            {
                improve_skill(i, SKILL_HIDE, 1);
            }
            add_hidden_char(&hideinfo, i);
            continue;
        }
    }

    for (auto i : filter_raw(list))
    {
        if (ch == i || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) && IS_NPC(i) &&
                        i->getRoomDescription() && *i->getRoomDescription() == '.'))
        {
            continue;
        }

        if (is_hidden(hideinfo, i))
        {
            continue;
        }

        if (CAN_SEE(ch, i))
        {
            auto num = 0;

            ch->sendText("@w");
            if (num > 1)
            {
                ch->send_to("@D(@Rx@Y%2i@D)@n ", num);
            }
            list_one_char(i, ch);
            ch->sendText("@n");
        }
        else if (ch->location.getIsDark() && !CAN_SEE_IN_DARK(ch) &&
                 AFF_FLAGGED(i, AFF_INFRAVISION))
        {
            ch->sendText("@wYou see a pair of glowing red eyes looking your way.@n\r\n");
        }
    }

    // cleanup
    while (hideinfo)
    {
        struct hide_node *temp = hideinfo;
        hideinfo = hideinfo->next;
        delete temp;
    }
}

void do_auto_exits(const Location& loc, Character *ch, int exit_mode)
{
    const int MAX_DIRS = 12;
    const char *dirNames[MAX_DIRS] = {"Northwest", "North", "Northeast", "East", "Southeast", "South", "Southwest", "West", "Up", "Down", "Inside", "Outside"};
    const char *mapKey[MAX_DIRS] = {"dlist1", "dlist2", "dlist3", "dlist4", "dlist5", "dlist6", "dlist7", "dlist8", "dlist9", "dlist10", "dlist11", "dlist12"};
    std::map<int, std::string> exitStrings;

    bool space = (loc.getSectorType() == SectorType::space && loc.getVnum() >= 20000);
    bool has_light = ch->isProvidingLight();
    bool admVision = ADM_FLAGGED(ch, ADM_SEESECRET) || GET_ADMLEVEL(ch) > 4;

    if (exit_mode == EXIT_OFF)
    {
        ch->sendText("@D------------------------------------------------------------------------@n\r\n");
    }

    auto cloc = ch->location;

    if (exit_mode == EXIT_NORMAL && !space && cloc == loc)
    {
        ch->sendText("@D------------------------------------------------------------------------@n\r\n");
        ch->sendText("@w      Compass           Auto-Map            Map Key\r\n");
        ch->sendText("@R     ---------         ----------   -----------------------------\r\n");
        gen_map(loc, ch, 0);
        ch->sendText("@D------------------------------------------------------------------------@n\r\n");
    }

    if (exit_mode == EXIT_NORMAL && space)
    {
        ch->sendText("@D------------------------------[@CRadar@D]---------------------------------@n\r\n");
        printmap(loc.getVnum(), ch, 1, -1);
        ch->sendText("     @D[@wTurn autoexit complete on for directions instead of radar@D]@n\r\n");
        ch->sendText("@D------------------------------------------------------------------------@n\r\n");
    }

    if (exit_mode == EXIT_COMPLETE || (exit_mode == EXIT_NORMAL && !space && cloc != loc))
    {
        ch->sendText("@D----------------------------[@gObvious Exits@D]-----------------------------@n\r\n");

        if (IS_AFFECTED(ch, AFF_BLIND))
        {
            ch->sendText("You can't see a damned thing, you're blind!\r\n");
            return;
        }

        if (PLR_FLAGGED(ch, PLR_EYEC))
        {
            ch->sendText("You can't see a damned thing, your eyes are closed!\r\n");
            return;
        }

        for (auto &[d, e] : loc.getExits())
        {
            auto door = static_cast<int>(d);
            if (admVision || (!IS_SET(e.exit_info, EX_CLOSED)))
            {
                std::string exitStr;
                std::string direction = dirs[door];
                direction[0] = toupper(direction[0]);

                if (admVision)
                {
                    exitStr = fmt::format("@c{} @D- [@Y{}@D]@w {}.\r\n", direction, e.getVnum(), e.getName());
                }
                else
                {
                    exitStr = fmt::format("@c{} @D-@w {}.\r\n", direction, (e.getIsDark() && !CAN_SEE_IN_DARK(ch) && !has_light) ? "@bToo dark to tell.@w" : e.getName());
                }

                if (IS_SET(e.exit_info, EX_ISDOOR) || IS_SET(e.exit_info, EX_SECRET))
                {
                    if (fname(e.keyword.c_str()) == nullptr)
                    {
                        ch->send_to("@RREPORT THIS ERROR IMMEDIATELY FOR DIRECTION %s@n\r\n", direction.c_str());
                        basic_mud_log("ERROR: %s found error direction %s at room %d", direction.c_str(), GET_NAME(ch), ch->location.getVnum());
                        return;
                    }
                    exitStr += fmt::format("The {}{} {} {} {}{}.\r\n",
                                           IS_SET(e.exit_info, EX_SECRET) ? "@rsecret@w " : "",
                                           (!e.keyword.empty() && strcasecmp(fname(e.keyword.c_str()), "undefined")) ? fname(e.keyword.c_str()) : "opening",
                                           strstr(fname(e.keyword.c_str()), "s ") ? "are" : "is",
                                           IS_SET(e.exit_info, EX_CLOSED) ? "closed" : "open",
                                           IS_SET(e.exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                                           IS_SET(e.exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
                }

                exitStrings[door] = exitStr;
            }
            else if (CONFIG_DISP_CLOSED_DOORS && !IS_SET(e.exit_info, EX_SECRET))
            {
                std::string direction = dirs[door];
                direction[0] = toupper(direction[0]);
                exitStrings[door] = fmt::format("@c{} @D-@w The {} appears @rclosed.@n\r\n",
                                                direction,
                                                (!e.keyword.empty() && strcasecmp(fname(e.keyword.c_str()), "undefined")) ? fname(e.keyword.c_str()) : "opening");
            }
        }

        if (exitStrings.empty())
        {
            ch->sendText(" None.\r\n");
        }
        else
        {
            for (const auto &entry : exitStrings)
            {
                ch->send_to("%s", entry.second.c_str());
            }
        }

        ch->sendText("@D------------------------------------------------------------------------@n\r\n");

        if (loc.getRoomFlag(ROOM_HOUSE))
        {
            auto con = loc.getObjects().size();
            if (!loc.getRoomFlag(ROOM_GARDEN1) && !loc.getRoomFlag(ROOM_GARDEN2))
            {
                ch->send_to("@D[@GItems Stored@D: @g%d@D]@n\r\n", con);
            }
            else if (loc.getRoomFlag(ROOM_GARDEN1) && !loc.getRoomFlag(ROOM_GARDEN2))
            {
                ch->send_to("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", con);
            }
            else if (!loc.getRoomFlag(ROOM_GARDEN1) && loc.getRoomFlag(ROOM_GARDEN2))
            {
                ch->send_to("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n", con);
            }
        }

        if (GET_RADAR1(ch) == loc.getVnum() && GET_RADAR2(ch) == loc.getVnum() && GET_RADAR3(ch) != loc.getVnum())
        {
            ch->sendText("@CTwo of your buoys are floating here.@n\r\n");
        }
        else if ((GET_RADAR1(ch) == loc.getVnum() && GET_RADAR2(ch) != loc.getVnum() && GET_RADAR3(ch) == loc.getVnum()) ||
                 (GET_RADAR1(ch) != loc.getVnum() && GET_RADAR2(ch) == loc.getVnum() && GET_RADAR3(ch) == loc.getVnum()))
        {
            ch->sendText("@CTwo of your buoys are floating here.@n\r\n");
        }
        else if (GET_RADAR1(ch) == loc.getVnum() && GET_RADAR2(ch) == loc.getVnum() && GET_RADAR3(ch) == loc.getVnum())
        {
            ch->sendText("@CAll three of your buoys are floating here. Why?@n\r\n");
        }
        else if (GET_RADAR1(ch) == loc.getVnum())
        {
            ch->sendText("@CYour @cBuoy #1@C is floating here.@n\r\n");
        }
        else if (GET_RADAR2(ch) == loc.getVnum())
        {
            ch->sendText("@CYour @cBuoy #2@C is floating here.@n\r\n");
        }
        else if (GET_RADAR3(ch) == loc.getVnum())
        {
            ch->sendText("@CYour @cBuoy #3@C is floating here.@n\r\n");
        }
    }
}

void do_auto_exits2(const Location& loc, Character *ch)
{
    int door, slen = 0;

    ch->sendText("\nExits: ");

    for (auto &[d, e] : loc.getExits())
    {
        if (EXIT_FLAGGED(&e, EX_CLOSED))
            continue;

        door = static_cast<int>(d);

        ch->send_to("%s ", abbr_dirs[door]);
        slen++;
    }

    ch->send_to("%s\r\n", slen ? "" : "None!");
}

ACMD(do_exits)
{
    /* Why duplicate code? */
    if (!PRF_FLAGGED(ch, PRF_NODEC))
    {
        do_auto_exits(ch->location, ch, EXIT_COMPLETE);
    }
    else
    {
        do_auto_exits2(ch->location, ch);
    }
}

static const char *exitlevels[] = {
    "off", "normal", "n/a", "complete", "\n"};

ACMD(do_autoexit)
{
    char arg[MAX_INPUT_LENGTH];
    int tp;

    if (IS_NPC(ch))
        return;

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->send_to("Your current autoexit level is %s.\r\n", exitlevels[EXIT_LEV(ch)]);
        return;
    }
    if (((tp = search_block(arg, exitlevels, false)) == -1))
    {
        ch->sendText("Usage: Autoexit { Off | Normal | Complete }\r\n");
        return;
    }
    switch (tp)
    {
    case EXIT_OFF:
        for (auto f : {PRF_AUTOEXIT, PRF_FULL_EXIT})
            ch->pref_flags.set(f, false);
        break;
    case EXIT_NORMAL:
        ch->pref_flags.set(PRF_AUTOEXIT, true);
        ch->pref_flags.set(PRF_FULL_EXIT, false);
        break;
    case EXIT_COMPLETE:
        for (auto f : {PRF_AUTOEXIT, PRF_FULL_EXIT})
            ch->pref_flags.set(f, true);
        break;
    }
    ch->send_to("Your @rautoexit level@n is now %s.\r\n", exitlevels[EXIT_LEV(ch)]);
}

void look_at_room(room_rnum target_room, Character *ch, int ignore_brief)
{
    Room *rm = get_room(target_room);
    look_at_room(rm, ch, ignore_brief);
}

static void display_room_info(Room *rm, Character *ch);

static void display_dimension_info(Room *rm, Character *ch)
{
    if (rm->where_flags.get(WhereFlag::neo_nirvana))
    {
        ch->sendText("@wPlanet: @WNeo Nirvana@n\r\n");
    }
    else if (rm->where_flags.get(WhereFlag::afterlife))
    {
        ch->sendText("@wDimension: @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye@n\r\n");
    }
    else if (rm->room_flags.get(ROOM_HELL))
    {
        ch->sendText("@wDimension: @RPunishment Hell@n\r\n");
    }
    else if (rm->where_flags.get(WhereFlag::afterlife_hell))
    {
        ch->sendText("@wDimension: @RH@re@Dl@Rl@n\r\n");
    }
}

static void display_special_room_descriptions(Room *rm, Character *ch)
{
    if (rm->room_flags.get(ROOM_REGEN))
    {
        ch->sendText("@CA feeling of calm and relaxation fills this room.@n\r\n");
    }
    if (rm->room_flags.get(ROOM_AURA))
    {
        ch->sendText("@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
    }
    if (rm->where_flags.get(WhereFlag::hyperbolic_time_chamber))
    {
        ch->sendText("@rThis room feels like it operates in a different time frame.@n\r\n");
    }
}

static void display_room_info(Room *rm, Character *ch)
{
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("@wO----------------------------------------------------------------------O@n\r\n");
    }

    ch->send_to("@wLocation: %-70s@n\r\n", rm->getName());

    if (auto planet = getPlanet(ch->location.getVnum()); planet)
    {
        ch->send_to("@wPlanet: @G%s@n\r\n", getPlanetColorName(planet.value()).c_str());
    }
    else
    {
        display_dimension_info(rm, ch);
    }

    double grav = rm->getEnvironment(ENV_GRAVITY);
    if (grav <= 1.0)
    {
        ch->sendText("@wGravity: @WNormal@n\r\n");
    }
    else
    {
        auto g = fmt::format("{}", grav);
        ch->send_to("@wGravity: @W%sx@n\r\n", g.c_str());
    }

    display_special_room_descriptions(rm, ch);

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("@wO----------------------------------------------------------------------O@n\r\n");
    }
}

static void display_room_flags(Room *rm, Character *ch)
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];

    sprintf(buf, "%s", rm->room_flags.getFlagNames().c_str());
    sprinttype(static_cast<int>(rm->sector_type), sector_types, buf2, sizeof(buf2));

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("\r\n@wO----------------------------------------------------------------------O@n\r\n");
    }

    ch->send_to("@wLocation: @G%-70s@w\r\n", rm->getName());

    if (auto sc = rm->getScripts(); !sc.empty())
    {
        ch->sendText("@D[@GTriggers");
        for (auto t : filter_shared(sc))
            ch->send_to(" %d", GET_TRIG_VNUM(t));
        ch->sendText("@D] ");
    }

    double grav = rm->getEnvironment(ENV_GRAVITY);
    auto g = fmt::format("{}", grav);
    snprintf(buf3, sizeof(buf3), "@D[ @G%s@D] @wSector: @D[ @G%s @D] @wVnum: @D[@G%5d@D]@n Gravity: @D[@G%sx@D]@n", buf, buf2, rm->getVnum(), g.c_str());
    ch->send_to("@wFlags: %-70s@w\r\n", buf3);

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("@wO----------------------------------------------------------------------O@n\r\n");
    }
}

static void display_damage_description(Character *ch, int dmg, const char *surface)
{
    if (dmg <= 2)
    {
        ch->send_to("@wA small hole with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 4)
    {
        ch->send_to("@wA couple small holes with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 6)
    {
        ch->send_to("@wA few small holes with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 10)
    {
        ch->send_to("@wThere are several small holes with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 20)
    {
        ch->send_to("@wMany holes fill the %s of this area, many of which have burn marks.@n", surface);
    }
    else if (dmg <= 30)
    {
        ch->send_to("@wThe %s is severely damaged with many large holes.@n", surface);
    }
    else if (dmg <= 50)
    {
        ch->sendText("@wBattle damage covers the entire area. Displayed as a tribute to the battles that have been waged here.@n");
    }
    else if (dmg <= 75)
    {
        ch->sendText("@wThis entire area is falling apart, it has been damaged so badly.@n");
    }
    else if (dmg <= 99)
    {
        ch->sendText("@wThis area cannot withstand much more damage. Everything has been damaged so badly it is hard to recognize any particular details about their former quality.@n");
    }
    else if (dmg >= 100)
    {
        ch->sendText("@wThis area is completely destroyed. Nothing is recognizable. Chunks of debris litter the ground, filling up holes, and overflowing onto what is left of the ground. A haze of smoke is wafting through the air, creating a chilling atmosphere.@n");
    }
}

static void display_damage_description_forest(Character *ch, int dmg)
{
    if (dmg <= 2)
    {
        ch->sendText("@wA small tree sits in a little crater here.@n");
    }
    else if (dmg <= 4)
    {
        ch->sendText("@wTrees have been uprooted by craters in the ground.@n");
    }
    else if (dmg <= 6)
    {
        ch->sendText("@wSeveral trees have been reduced to chunks of debris and are laying in a few craters here.@n");
    }
    else if (dmg <= 10)
    {
        ch->sendText("@wA large patch of trees have been destroyed and are laying in craters here.@n");
    }
    else if (dmg <= 20)
    {
        ch->sendText("@wSeveral craters have merged into one large crater in one part of this forest.@n");
    }
    else if (dmg <= 30)
    {
        ch->sendText("@wThe open sky can easily be seen through a hole of trees destroyed and resting at the bottom of several craters here.@n");
    }
    else if (dmg <= 50)
    {
        ch->sendText("@wA good deal of burning tree pieces can be found strewn across the cratered ground here.@n");
    }
    else if (dmg <= 75)
    {
        ch->sendText("@wVery few trees are left standing in this area, replaced instead by large craters.@n");
    }
    else if (dmg <= 99)
    {
        ch->sendText("@wSingle solitary trees can be found still standing here or there in the area. The rest have been almost completely obliterated in recent conflicts.@n");
    }
    else if (dmg >= 100)
    {
        ch->sendText("@wOne massive crater fills this area. This desolate crater leaves no evidence of what used to be found in the area. Smoke slowly wafts into the sky from the central point of the crater, creating an oppressive atmosphere.@n");
    }
}

static void display_damage_description_mountain(Character *ch, int dmg)
{
    if (dmg <= 2)
    {
        ch->sendText("@wA small crater has been burned into the side of this mountain.@n");
    }
    else if (dmg <= 4)
    {
        ch->sendText("@wA couple craters have been burned into the side of this mountain.@n");
    }
    else if (dmg <= 6)
    {
        ch->sendText("@wBurned bits of boulders can be seen lying at the bottom of a few nearby craters.@n");
    }
    else if (dmg <= 10)
    {
        ch->sendText("@wSeveral bad craters can be seen in the side of the mountain here.@n");
    }
    else if (dmg <= 20)
    {
        ch->sendText("@wLarge boulders have rolled down the mountainside and collected in many nearby craters.@n");
    }
    else if (dmg <= 30)
    {
        ch->sendText("@wMany craters are covering the mountainside here.@n");
    }
    else if (dmg <= 50)
    {
        ch->sendText("@wThe mountain side has partially collapsed, shedding rubble down towards its base.@n");
    }
    else if (dmg <= 75)
    {
        ch->sendText("@wA peak of the mountain has been blown off, leaving behind a smoldering tip.@n");
    }
    else if (dmg <= 99)
    {
        ch->sendText("@wThe mountainside here has completely collapsed, shedding dangerous rubble down to its base.@n");
    }
    else if (dmg >= 100)
    {
        ch->sendText("@wHalf the mountain has been blown away, leaving a scarred and jagged rock in its place. Billowing smoke wafts up from several parts of the mountain, filling the nearby skies and blotting out the sun.@n");
    }
}

static void display_room_damage_description(Room *rm, Character *ch)
{
    auto dmg = rm->getDamage();
    auto sect = static_cast<int>(rm->sector_type);
    auto sunk = rm->getEnvironment(ENV_WATER) >= 100.0;

    if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || rm->room_flags.get(ROOM_DEATH))
    {
        if (dmg <= 99 || (dmg == 100 && (sect == SECT_WATER_SWIM || sunk || sect == SECT_FLYING || sect == SECT_SHOP || sect == SECT_IMPORTANT)))
        {
            ch->send_to("@w%s@n", rm->getLookDescription());
        }

        if (dmg > 0)
        {
            ch->sendText("\r\n");
            switch (sect)
            {
            case SECT_INSIDE:
                display_damage_description(ch, dmg, "floor");
                break;
            case SECT_CITY:
            case SECT_FIELD:
            case SECT_HILLS:
            case SECT_IMPORTANT:
                display_damage_description(ch, dmg, "ground");
                break;
            case SECT_FOREST:
                display_damage_description_forest(ch, dmg);
                break;
            case SECT_MOUNTAIN:
                display_damage_description_mountain(ch, dmg);
                break;
            default:
                break;
            }
            ch->sendText("\r\n");
        }

        if (rm->ground_effect >= 1 && rm->ground_effect <= 5)
        {
            ch->sendText("@rLava@w is pooling in some places here...@n\r\n");
        }
        else if (rm->ground_effect >= 6)
        {
            ch->sendText("@RLava@r covers pretty much the entire area!@n\r\n");
        }
        else if (rm->ground_effect < 0)
        {
            ch->sendText("@cThe entire area is flooded with a @Cmystical@c cube of @Bwater!@n\r\n");
        }
    }
}

static void display_garden_info(Room *rm, Character *ch)
{
    auto con = rm->getObjects();
    if (rm->room_flags.get(ROOM_GARDEN1))
    {
        ch->send_to("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", con.size());
    }
    else if (rm->room_flags.get(ROOM_GARDEN2))
    {
        ch->send_to("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n", con.size());
    }
    else if (rm->room_flags.get(ROOM_HOUSE))
    {
        ch->send_to("@D[@GItems Stored@D: @g%d@D]@n\r\n", con.size());
    }
}

void look_at_room(Room *rm, Character *ch, int ignore_brief)
{
    if (!ch->desc)
        return;

    /*
    if (IS_DARK(rm->getVnum()) && !CAN_SEE_IN_DARK(ch) && !PLR_FLAGGED(ch, PLR_AURALIGHT)) {
                ch->sendText("It's too dark to make out much detail...\r\n");
        return;
    }
    */

    if (AFF_FLAGGED(ch, AFF_BLIND))
    {
        ch->sendText("You see nothing but infinite darkness...\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_EYEC))
    {
        ch->sendText("You can't see a damned thing, your eyes are closed!\r\n");
        return;
    }

    if (PRF_FLAGGED(ch, PRF_ROOMFLAGS))
    {
        display_room_flags(rm, ch);
    }
    else
    {
        display_room_info(rm, ch);
    }

    display_room_damage_description(rm, ch);

    /* autoexits */
    if (!IS_NPC(ch))
    {
        if (PRF_FLAGGED(ch, PRF_NODEC))
        {
            do_auto_exits2(rm, ch);
        }
        else
        {
            do_auto_exits(rm, ch, EXIT_LEV(ch));
        }
    }

    display_garden_info(rm, ch);
    list_obj_to_char(rm->getObjects(), ch, SHOW_OBJ_LONG, false);
    list_char_to_char(rm->getPeople(), ch);
}

static void look_in_direction(Character *ch, int dir)
{
    auto ex = ch->location.getExit(static_cast<Direction>(dir));
    if (!ex)
    {
        ch->sendText("Nothing special there...\r\n");
        return;
    }

    if (ex->general_description.empty())
        ch->send_to("%s", ex->general_description.c_str());

    bool canSeeRoom = false;

    if (EXIT_FLAGGED(ex, EX_ISDOOR) && !ex->keyword.empty())
    {
        if (!EXIT_FLAGGED(ex, EX_SECRET) &&
            EXIT_FLAGGED(ex, EX_CLOSED))
            ch->send_to("The %s is closed.\r\n", fname(ex->keyword.c_str()));
        else if (!EXIT_FLAGGED(ex, EX_CLOSED))
        {
            ch->send_to("The %s is open.\r\n", fname(ex->keyword.c_str()));
            canSeeRoom = true;
        }
    }
    else
    {
        canSeeRoom = true;
    }

    if (canSeeRoom)
    {
        ch->sendText("You peek over and see:\r\n");
        ch->lookAtLocation(*ex);
    }
}

static void handle_portal(Character *ch, Object *obj)
{
    if (!OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))
    {
        int portal_appear = GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR);

        Destination d;
        d.al = get_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST))->shared_from_this();

        if (portal_appear < 0)
        {
            if (!d || (d.getIsDark() && !CAN_SEE_IN_DARK(ch) && !PLR_FLAGGED(ch, PLR_AURALIGHT)))
            {
                ch->sendText("You see nothing but infinite darkness...\r\n");
            }
            else
            {
                ch->send_to("After seconds of concentration you see the image of %s.\r\n", d.getName());
            }
        }
        else if (portal_appear < MAX_PORTAL_TYPES)
        {
            ch->send_to("%s\r\n", portal_appearance[portal_appear]);
        }
        else
        {
            ch->sendText("All you can see is the glow of the portal.\r\n");
        }
    }
}

static void handle_vehicle(Character *ch, Object *obj)
{
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
    {
        ch->sendText("It is closed.\r\n");
        return;
    }

    Destination d;
    d.al = get_room(GET_OBJ_VAL(obj, VAL_VEHICLE_DEST))->shared_from_this();

    if (!d)
    {
        ch->sendText("You cannot see inside that.\r\n");
    }
    else if (d.getIsDark() && !CAN_SEE_IN_DARK(ch) && !PLR_FLAGGED(ch, PLR_AURALIGHT))
    {
        ch->sendText("It is pitch black...\r\n");
    }
    else
    {
        ch->sendText("You look inside and see:\r\n");
        ch->lookAtLocation(d);
    }
}

static void handle_container(Character *ch, Object *obj, int bits)
{
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
    {
        ch->sendText("It is closed.\r\n");
    }
    else
    {
        ch->send_to("%s", obj->getShortDescription());
        switch (bits)
        {
        case FIND_OBJ_INV:
            ch->sendText(" (carried): \r\n");
            break;
        case FIND_OBJ_ROOM:
            ch->sendText(" (here): \r\n");
            break;
        case FIND_OBJ_EQUIP:
            ch->sendText(" (used): \r\n");
            break;
        }

        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER &&
            (GET_OBJ_VNUM(obj) == 697 || GET_OBJ_VNUM(obj) == 698 || GET_OBJ_VNUM(obj) == 682 ||
             GET_OBJ_VNUM(obj) == 683 || GET_OBJ_VNUM(obj) == 684))
        {
            act("$n looks in $p.", true, ch, obj, nullptr, TO_ROOM);
        }

        list_obj_to_char(obj->getInventory(), ch, SHOW_OBJ_SHORT, true);
    }
}

static void describe_drink_level(Character *ch, const char *liquid, int howfull, int capacity)
{
    if (howfull == capacity)
    {
        ch->send_to("It's full of a %s liquid.\r\n", liquid);
    }
    else if (howfull >= capacity * .8)
    {
        ch->send_to("It's almost full of a %s liquid.\r\n", liquid);
    }
    else if (howfull >= capacity * .5)
    {
        ch->send_to("It's about half full of a %s liquid.\r\n", liquid);
    }
    else if (howfull >= capacity * .2)
    {
        ch->send_to("It's less than half full of a %s liquid.\r\n", liquid);
    }
    else if (howfull > 0)
    {
        ch->send_to("It's barely filled with a %s liquid.\r\n", liquid);
    }
    else
    {
        ch->sendText("It's empty.\r\n");
    }
}

static void handle_drinkcon(Character *ch, Object *obj)
{
    int capacity = GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY);
    int howfull = GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL);

    if (howfull <= 0 && capacity == 1)
    {
        ch->sendText("It is empty.\r\n");
    }
    else if (capacity < 0)
    {
        char buf2[MAX_STRING_LENGTH];
        sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
        ch->send_to("It's full of a %s liquid.\r\n", buf2);
    }
    else if (howfull > capacity)
    {
        ch->sendText("Its contents seem somewhat murky.\r\n"); /* BUG */
    }
    else
    {
        char buf2[MAX_STRING_LENGTH];
        sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
        describe_drink_level(ch, buf2, howfull, capacity);
    }
}

static void look_in_obj(Character *ch, char *arg)
{
    Object *obj = nullptr;
    Character *dummy = nullptr;

    if (!*arg)
    {
        ch->sendText("Look in what?\r\n");
        return;
    }

    int bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj);
    if (!bits)
    {
        ch->send_to("There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
        return;
    }

    if (find_exdesc(arg, obj->getExtraDescription()) && !bits)
    {
        ch->sendText("There's nothing inside that!\r\n");
        return;
    }

    auto otype = GET_OBJ_TYPE(obj);
    auto ovn = GET_OBJ_VNUM(obj);

    switch (otype)
    {
    case ITEM_PORTAL:
        handle_portal(ch, obj);
        break;

    case ITEM_VEHICLE:
        handle_vehicle(ch, obj);
        break;

    case ITEM_WINDOW:
        look_out_window(ch, arg);
        break;

    case ITEM_CONTAINER:
        handle_container(ch, obj, bits);
        break;

    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        handle_drinkcon(ch, obj);
        break;

    default:
        ch->sendText("There's nothing inside that!\r\n");
        break;
    }
}

char *find_exdesc(char *word, struct extra_descr_data *list)
{
    struct extra_descr_data *i;

    for (i = list; i; i = i->next)
        /*if (isname(word, i->keyword))*/
        if (*i->keyword == '.' ? isname(word, i->keyword + 1) : isname(word, i->keyword))
            return (i->description);

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
static void examine_equipped_item(Character *ch, Object *obj, const char *arg)
{
    if (isname(arg, obj->getName()))
    {
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        {
            ch->send_to("The weapon type of %s is a %s.\r\n", GET_OBJ_SHORT(obj), weapon_type[(int)GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
        }
        if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
        {
            display_spells(ch, obj);
        }
        if (GET_OBJ_TYPE(obj) == ITEM_SCROLL)
        {
            display_scroll(ch, obj);
        }
        diag_obj_to_char(obj, ch);
        ch->send_to("It appears to be made of %s", material_names[GET_OBJ_MATERIAL(obj)]);
    }
}

static void examine_item(Character *ch, Object *obj, const char *arg)
{
    if (isname(arg, obj->getName()))
    {
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        {
            ch->send_to("The weapon type of %s is a %s.\r\n", GET_OBJ_SHORT(obj), weapon_type[(int)GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
        }
        if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
        {
            display_spells(ch, obj);
        }
        if (GET_OBJ_TYPE(obj) == ITEM_SCROLL)
        {
            display_scroll(ch, obj);
        }
        if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE)
        {
            ch->sendText("@YSyntax@D: @CUnlock hatch\r\n");
            ch->sendText("@YSyntax@D: @COpen hatch\r\n");
            ch->sendText("@YSyntax@D: @CClose hatch\r\n");
            ch->sendText("@YSyntax@D: @CUnlock hatch\r\n");
            ch->sendText("@YSyntax@D: @CEnter hatch\r\n");
        }
        else if (GET_OBJ_TYPE(obj) == ITEM_HATCH)
        {
            ch->sendText("@YSyntax@D: @CUnlock hatch\r\n");
            ch->sendText("@YSyntax@D: @COpen hatch\r\n");
            ch->sendText("@YSyntax@D: @CClose hatch\r\n");
            ch->sendText("@YSyntax@D: @CUnlock hatch\r\n");
            ch->sendText("@YSyntax@D: @CLeave@n\r\n");
        }
        else if (GET_OBJ_TYPE(obj) == ITEM_WINDOW)
        {
            look_out_window(ch, obj->getName());
        }
        if (GET_OBJ_TYPE(obj) == ITEM_CONTROL)
        {
            ch->send_to("@RFUEL@D: %s%s@n\r\n", GET_FUEL(obj) >= 200 ? "@G" : GET_FUEL(obj) >= 100 ? "@Y"
                                                                                                   : "@r",
                        add_commas(GET_FUEL(obj)).c_str());
        }
        diag_obj_to_char(obj, ch);
        ch->send_to("It appears to be made of %s, and weighs %s", material_names[GET_OBJ_MATERIAL(obj)], add_commas(GET_OBJ_WEIGHT(obj)).c_str());
    }
}

static void handle_board_read(Character *ch, char *arg)
{
    Object *obj = ch->searchInventory([](const auto &o)
                                      { return GET_OBJ_TYPE(o) == ITEM_BOARD; });
    if (!obj)
        obj = ch->location.searchObjects([](const auto &o)
                                         { return GET_OBJ_TYPE(o) == ITEM_BOARD; });

    if (!obj)
    {
        ch->sendText("Read what?\r\n");
        return;
    }

    char number[MAX_STRING_LENGTH];
    arg = one_argument(arg, number);

    if (!*number)
    {
        ch->sendText("Read what?\r\n");
    }
    else if (isname(number, obj->getName()))
    {
        show_board(GET_OBJ_VNUM(obj), ch);
    }
    else if (!isdigit(*number) || strchr(number, '.'))
    {
        char new_arg[MAX_STRING_LENGTH];
        snprintf(new_arg, sizeof(new_arg), "%s %s", number, arg);
        look_at_target(ch, new_arg, 0);
    }
    else
    {
        int msg = atoi(number);
        board_display_msg(GET_OBJ_VNUM(obj), ch, msg);
    }
}

static bool handle_exdesc_look(Character *ch, char *arg, const std::vector<ExtraDescription> &ex_desc_list, Object *obj)
{
    char *desc;
    int fnum = get_number(&arg);
    int i = 0;

    for (const auto &i : ex_desc_list)
    {
        if (match_exdesc(arg, i))
        {
            ch->desc->sendText(i.description.c_str());
            return true;
        }
    }
    return false;
}

static void handle_look_in_inventory(Character *ch, char *arg)
{

    for (int j = 0; j < NUM_WEARS; j++)
    {
        Object *eq = GET_EQ(ch, j);
        if (eq && CAN_SEE_OBJ(ch, eq) && handle_exdesc_look(ch, arg, eq->getExtraDescription(), eq))
        {
            examine_equipped_item(ch, eq, arg);
            return;
        }
    }
    auto con = ch->getInventory();
    for (auto obj : filter_raw(con))
    {
        if (CAN_SEE_OBJ(ch, obj) && handle_exdesc_look(ch, arg, obj->getExtraDescription(), obj))
        {
            examine_item(ch, obj, arg);
            return;
        }
    }
    auto loco = ch->location.getObjects();
    for (auto obj : filter_raw(loco))
    {
        if (CAN_SEE_OBJ(ch, obj) && handle_exdesc_look(ch, arg, obj->getExtraDescription(), obj))
        {
            examine_item(ch, obj, arg);
            return;
        }
    }
}

static void handle_look(Character *ch, char *arg)
{
    Character *found_char = nullptr;
    Object *found_obj = nullptr;
    int bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &found_char, &found_obj);

    if (found_char)
    {
        look_at_char(found_char, ch);
        if (ch != found_char && !AFF_FLAGGED(ch, AFF_HIDE))
        {
            act("$n looks at you.", true, ch, nullptr, found_char, TO_VICT);
            act("$n looks at $N.", true, ch, nullptr, found_char, TO_NOTVICT);
        }
        return;
    }

    if (!handle_exdesc_look(ch, arg, ch->location.getExtraDescription(), nullptr))
    {
        handle_look_in_inventory(ch, arg);
    }

    if (bits && !found_obj)
    {
        show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
        if (show_obj_modifiers(found_obj, ch))
        {
            ch->sendText("\r\n");
        }
    }
    else if (!found_obj)
    {
        ch->sendText("You do not see that here.\r\n");
    }
}

static void look_at_target(Character *ch, char *arg, int cmread)
{
    if (!ch->desc)
        return;

    if (!*arg)
    {
        ch->sendText("Look at what?\r\n");
        return;
    }

    if (cmread)
    {
        handle_board_read(ch, arg);
    }
    else
    {
        handle_look(ch, arg);
    }
}

static void look_out_window(Character *ch, const char *arg)
{
    Object *i, *viewport = nullptr, *vehicle = nullptr;
    Character *dummy = nullptr;
    room_rnum target_room = NOWHERE;
    int bits, door;

    auto r = ch->getRoom();

    /* First, lets find something to look out of or through. */
    if (*arg)
    {
        /* Find this object and see if it is a window */
        if (!(bits = generic_find(arg,
                                  FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP,
                                  ch, &dummy, &viewport)))
        {
            ch->sendText("You don't see that here.\r\n");
            return;
        }
        else if (GET_OBJ_TYPE(viewport) != ITEM_WINDOW)
        {
            ch->sendText("You can't look out that!\r\n");
            return;
        }
    }
    else if (OUTSIDE(ch))
    {
        /* yeah, sure stupid */
        ch->sendText("But you are already outside.\r\n");
        return;
    }
    /* Look for any old window in the room */
    viewport = ch->location.searchObjects([&](auto obj)
                                          { return GET_OBJ_TYPE(obj) == ITEM_WINDOW && isname("window", obj->getName()); });

    if (!viewport)
    {
        /* Nothing suitable to look through */
        ch->sendText("You don't seem to be able to see outside.\r\n");
        return;
    }
    if (OBJVAL_FLAGGED(viewport, CONT_CLOSEABLE) &&
        OBJVAL_FLAGGED(viewport, CONT_CLOSED))
    {
        /* The window is closed */
        ch->sendText("It is closed.\r\n");
        return;
    }
    if (GET_OBJ_VAL(viewport, VAL_WINDOW_VIEWPORT) < 0)
    {
        /* We are looking out of the room */
        if (GET_OBJ_VAL(viewport, VAL_WINDOW_DEFAULT_ROOM) < 0)
        {
            /* Look for the default "outside" room */
            for (auto &[d, e] : r->getDirections())
            {
                if (!e.getRoomFlag(ROOM_INDOORS))
                {
                    target_room = e.getVnum();
                    break;
                }
            }
        }
        else
        {
            target_room = real_room(GET_OBJ_VAL(viewport, VAL_WINDOW_DEFAULT_ROOM));
        }
    }
    else
    {
        /* We are looking out of a vehicle */
        if ((vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(viewport, VAL_WINDOW_VIEWPORT))))
            target_room = IN_ROOM(vehicle);
    }
    if (target_room == NOWHERE)
    {
        ch->sendText("You don't seem to be able to see outside.\r\n");
        return;
    }
    if (auto ld = viewport->getLookDescription(); ld)
        act(ld, true, ch, viewport, nullptr, TO_CHAR);
    else
        act("$n looks out the window.", true, ch, nullptr, nullptr, TO_ROOM);
    ch->sendText("You look outside and see:\r\n");
    Location loc(target_room);
    ch->lookAtLocation(loc);
}

ACMD(do_finger)
{

    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("What user are you wanting to look at?\r\n");
        return;
    }

    auto account = findAccount(arg);
    if (!account)
    {
        ch->sendText("That user does not exist\r\n");
        return;
    }
    fingerUser(ch, account);
}

ACMD(do_rptrans)
{
    Character *vict = nullptr;
    struct descriptor_data *k;
    int amt = 0;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        ch->sendText("Syntax: exchange (target) (amount)\r\n");
        return;
    }

    amt = atoi(arg2);

    if (amt <= 0)
    {
        ch->sendText("Are you being funny?\r\n");
        return;
    }

    auto haveRP = GET_RP(ch);

    if (amt > haveRP)
    {
        ch->send_to("@WYou only have @C%d@W RPP!@n\r\n", haveRP);
        return;
    }

    if (!readUserIndex(arg))
    {
        ch->sendText("That is not a recognised user file.\r\n");
        return;
    }

    for (k = descriptor_list; k; k = k->next)
    {
        if (IS_NPC(k->character))
            continue;
        if (STATE(k) != CON_PLAYING)
            continue;
        if (!strcasecmp(k->account->name.c_str(), arg))
            vict = k->character;
    }
    if (vict == nullptr)
    {
    }
    else
    {
        vict->modRPP(amt);
        vict->send_to("@W%s gives @C%d@W of their RPP to you. How nice!\r\n", GET_NAME(ch), amt);
    }
    ch->modRPP(-amt);
    ch->send_to("@WYou exchange @C%d@W RPP to user @c%s@W for a warm fuzzy feeling.\r\n", amt, CAP(arg));
    mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), true, "EXCHANGE: %s gave %d RPP to user %s", GET_NAME(ch), amt,
           arg);
}

ACMD(do_rdisplay)
{
    skip_spaces(&argument);

    if (IS_NPC(ch))
    {
        return;
    }

    if (!*argument)
    {
        ch->sendText("Clearing room display.\r\n");
        if (GET_RDISPLAY(ch))
            free(GET_RDISPLAY(ch));
        GET_RDISPLAY(ch) = nullptr;
    }
    else
    {
        char derp[MAX_STRING_LENGTH];

        strcpy(derp, argument);

        ch->send_to("You set your display to; %s\r\n", derp);
        if (GET_RDISPLAY(ch))
            free(GET_RDISPLAY(ch));
        GET_RDISPLAY(ch) = strdup(derp);
    }
}

int perf_skill(int skill)
{
    switch (skill)
    {
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

ACMD(do_perf)
{
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int i, skill = 1, found = false, type = 0;

    two_arguments(argument, arg, arg2);

    if (IS_NPC(ch) || GET_ADMLEVEL(ch) > 0)
    {
        ch->sendText("I don't think so.\r\n");
        return;
    }

    if (!*arg || !*arg2)
    {
        ch->sendText("@WType @G1@D: @wOver Charged@n\r\n");
        ch->sendText("@WType @G2@D: @wAccurate@n\r\n");
        ch->sendText("@WType @G3@D: @wEfficient@n\r\n");
        ch->sendText("Syntax: perfect (skillname) (type 1/2/or 3)\r\n");
        return;
    }
    if (strlen(arg) < 4)
    {
        ch->sendText("The skill name should be longer than 3 characters...\r\n");
        return;
    }
    for (i = 1; i < SKILL_TABLE_SIZE; i++)
    {
        if (spell_info[i].skilltype != SKTYPE_SKILL)
            continue;

        if (found == true)
            continue;

        if (strstr(spell_info[i].name, arg))
        {
            skill = i;
            found = true;
        }
    }
    if (found == false)
    {
        ch->send_to("The skill %s doesn't exist.\r\n", arg);
        return;
    }
    if (!GET_SKILL(ch, skill))
    {
        ch->send_to("You don't know %s.\r\n", arg);
        return;
    }
    if (GET_SKILL(ch, skill) < 100)
    {
        ch->send_to("You have not mastered the skill %s and thus can't perfect it.\r\n", arg);
        return;
    }
    if (GET_SKILL_PERF(ch, skill) > 0)
    {
        ch->send_to("You have already mastered the skill %s and chosen how to perfect it.\r\n", arg);
        return;
    }
    if (!perf_skill(skill))
    {
        ch->sendText("You can't perfect that type of skill.\r\n");
        return;
    }
    if (atoi(arg2) < 1 || atoi(arg2) > 3)
    {
        ch->sendText("@WType @G1@D: @wOver Charged@n\r\n");
        ch->sendText("@WType @G2@D: @wAccurate@n\r\n");
        ch->sendText("@WType @G3@D: @wEfficient@n\r\n");
        ch->sendText("@RType must be a number between 1 and 3.@n\r\n");
        return;
    }

    type = atoi(arg2);
    switch (type)
    {
    case 1:
        ch->send_to("You perfect the skill %s so that you can over charge it!\r\n", spell_info[skill].name);
        SET_SKILL_PERF(ch, skill, 1);
        break;
    case 2:
        ch->send_to("You perfect the skill %s so that you have supreme accuracy with it!\r\n", spell_info[skill].name);
        SET_SKILL_PERF(ch, skill, 2);
        break;
    case 3:
        ch->send_to("You perfect the skill %s so that you require a lower minimum charge for it!\r\n", spell_info[skill].name);
        SET_SKILL_PERF(ch, skill, 3);
        break;
    }
}

ACMD(do_look)
{
    int look_type;

    if (!ch->desc)
        return;

    if (GET_POS(ch) < POS_SLEEPING)
    {
        ch->sendText("You can't see anything but stars!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_BLIND))
    {
        ch->sendText("You can't see a damned thing, you're blind!\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_EYEC))
    {
        ch->sendText("You can't see a damned thing, your eyes are closed!\r\n");
        return;
    }

    if (ch->location.getIsDark() && !CAN_SEE_IN_DARK(ch))
    {
        ch->sendText("It is pitch black...\r\n");
        list_char_to_char(ch->location.getPeople(), ch); /* glowing red eyes */
        return;
    }

    char arg[MAX_INPUT_LENGTH], arg2[200];

    if (subcmd == SCMD_READ)
    {
        one_argument(argument, arg);
        if (!*arg)
            ch->sendText("Read what?\r\n");
        else
            look_at_target(ch, arg, 1);
        return;
    }

    argument = any_one_arg(argument, arg);
    one_argument(argument, arg2);

    if (!*arg)
    {
        if (subcmd == SCMD_SEARCH)
        {
            search_room(ch);
        }
        else
        {
            ch->lookAtLocation();
            if (GET_ADMLEVEL(ch) < 1 && !AFF_FLAGGED(ch, AFF_HIDE))
            {
                // act("@w$n@w looks around the room.@n", TRUE, ch, 0, 0, TO_ROOM);
            }
        }
        return;
    }

    if (is_abbrev(arg, "moon"))
    {
        auto moonlight = ch->location.getEnvironment(ENV_MOONLIGHT);
        if (moonlight < 0)
        {
            ch->sendText("It's kinda hard to see any moons from here.\r\n");
            return;
        }
        if (moonlight == 0)
        {
            ch->sendText("You gaze skyward, but there's no full moon in sight.\r\n");
            return;
        }
        if (moonlight >= 100.0)
        {
            ch->sendText("You gaze upon a wondrous full moon... it's an amazing sight.\r\n");
            ch->gazeAtMoon();
            return;
        }
        return;
    }

    std::optional<Destination> dir;

    if (is_abbrev(arg, "inside") && (dir = ch->location.getExit(Direction::inside)) && !*arg2)
    {
        if (subcmd == SCMD_SEARCH)
            search_in_direction(ch, INDIR);
        else
            look_in_direction(ch, INDIR);
        return;
    }

    if (is_abbrev(arg, "inside") && (subcmd == SCMD_SEARCH) && !*arg2)
    {
        search_in_direction(ch, INDIR);
        return;
    }

    if (is_abbrev(arg, "inside") || is_abbrev(arg, "into") || is_abbrev(arg, "onto"))
    {
        look_in_obj(ch, arg2);
        return;
    }
    if ((is_abbrev(arg, "outside") || is_abbrev(arg, "through") || is_abbrev(arg, "thru")) && (subcmd == SCMD_LOOK) && *arg2)
    {
        look_out_window(ch, arg2);
        return;
    }

    if (is_abbrev(arg, "outside") && (subcmd == SCMD_LOOK) && !(dir = ch->location.getExit(Direction::outside)))
    {
        look_out_window(ch, arg2);
        return;
    }

    if ((look_type = search_block(arg, dirs, false)) >= 0 || (look_type = search_block(arg, abbr_dirs, false)) >= 0)
    {
        if (subcmd == SCMD_SEARCH)
            search_in_direction(ch, look_type);
        else
            look_in_direction(ch, look_type);
        return;
    }

    if ((is_abbrev(arg, "towards")) && ((look_type = search_block(arg2, dirs, false)) >= 0 || (look_type = search_block(arg2, abbr_dirs, false)) >= 0))
    {
        if (subcmd == SCMD_SEARCH)
            search_in_direction(ch, look_type);
        else
            look_in_direction(ch, look_type);
        return;
    }

    if (is_abbrev(arg, "at"))
    {
        if (subcmd == SCMD_SEARCH)
            ch->sendText("That is not a direction!\r\n");
        else
            look_at_target(ch, arg2, 0);
        return;
    }

    if (is_abbrev(arg, "around"))
    {
        struct extra_descr_data *i;
        int found = 0;

        for (const auto &ex : ch->location.getExtraDescription())
        {
            if (!ex.keyword.starts_with("."))
            {
                ch->send_to("%s%s:\r\n%s", (found ? "\r\n" : ""), ex.keyword.c_str(), ex.description.c_str());
                found = 1;
            }
        }
        if (!found)
            ch->sendText("You couldn't find anything noticeable.\r\n");
        return;
    }

    if (find_exdesc(arg, ch->location.getExtraDescription()))
    {
        look_at_target(ch, arg, 0);
        return;
    }

    if (subcmd == SCMD_SEARCH)
        ch->sendText("That is not a direction!\r\n");
    else
        look_at_target(ch, arg, 0);
}

ACMD(do_examine)
{
    Character *tmp_char;
    Object *tmp_object;
    char tempsave[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Examine what?\r\n");
        return;
    }

    /* look_at_target() eats the number. */
    look_at_target(ch, strcpy(tempsave, arg), 0); /* strcpy: OK */

    generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

    if (tmp_object)
    {
        if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
            (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
            (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER))
        {
            ch->sendText("When you look inside, you see:\r\n");
            look_in_obj(ch, arg);
        }
    }
}

ACMD(do_gold)
{
    if (GET_GOLD(ch) == 0)
        ch->sendText("You're broke!\r\n");
    else if (GET_GOLD(ch) == 1)
        ch->sendText("You have one little zenni.\r\n");
    else
        ch->send_to("You have %d zenni.\r\n", GET_GOLD(ch));
}

ACMD(do_score)
{

    if (IS_NPC(ch))
        return;

    int view = 0, full = 5, personal = 1, health = 2, stats = 3, other = 4;

    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg)
    {
        view = full;
    }
    else if (strstr("personal", arg) || strstr("Personal", arg))
    {
        view = personal;
    }
    else if (strstr("health", arg) || strstr("Health", arg))
    {
        view = health;
    }
    else if (strstr("statistics", arg) || strstr("Statistics", arg))
    {
        view = stats;
    }
    else if (strstr("other", arg) || strstr("Other", arg))
    {
        view = other;
    }
    else
    {
        ch->sendText("Syntax: score, or... score (personal, health, statistics, other)\r\n");
        return;
    }

    if (view == full || view == personal)
    {
        ch->sendText("  @cO@D-----------------------------[  @cPersonal  @D]-----------------------------@cO@n\n");
        ch->send_to("  @D|  @CName@D: @W%15s@D,   @CTitle@D: @W%-38s@D|@n\n", GET_NAME(ch), GET_TITLE(ch));
        if (IS_ANDROID(ch))
        {
            char model[100], version[100];
            int absorb = 0;
            if (ch->subrace == SubRace::android_model_absorb)
            {
                sprintf(model, "@CAbsorption");
            }
            else if (ch->subrace == SubRace::android_model_repair)
            {
                sprintf(model, "@GSelf Repairing");
            }
            else if (ch->subrace == SubRace::android_model_sense)
            {
                sprintf(model, "@RSensor Equiped");
            }

            switch (ch->form)
            {
            case Form::base:
                sprintf(version, "Alpha 0.5");
                break;
            case Form::android_1:
                sprintf(version, "Beta 1.0");
                break;
            case Form::android_2:
                sprintf(version, "ANS 2.0");
                break;
            case Form::android_3:
                sprintf(version, "ANS 3.0");
                break;
            case Form::android_4:
                sprintf(version, "ANS 4.0");
                break;
            case Form::android_5:
                sprintf(version, "ANS 5.0");
                break;
            case Form::android_6:
                sprintf(version, "ANS 6.0");
                break;
            default:
                break;
            }

            ch->send_to("  @D| @CModel@D: %15s@D,    @CUGP@D: @G%15s@D,  @CVersion@D: @r%-12s@D|@n\n", model, absorb > 0 ? "@RN/A" : add_commas(GET_UP(ch)).c_str(), version);
        }
        if (GET_CLAN(ch))
        {
            ch->send_to("  @D|  @CClan@D: @W%-64s@D|@n\n", GET_CLAN(ch));
        }
        ch->send_to("  @D|  @CRace@D: @W%10s@D,  @CSensei@D: @W%15s@D,     @CArt@D: @W%-17s@D|@n\n", race::getName(ch->race), sensei::getName(ch->sensei).c_str(), sensei::getStyle(ch->sensei).c_str());
        ch->send_to("  @D|@CGender@D: @W%10s@D,  @C  Size@D: @W%15s@D,  @C Align@D: @W%-17s@D|@n\n", genders[static_cast<int>(GET_SEX(ch))], size_names[get_size(ch)], disp_align(ch));
    }
    if (view == full || view == health)
    {
        ch->sendText("  @cO@D-----------------------------@D[   @cHealth   @D]-----------------------------@cO@n\n");
        ch->send_to("                          @D<@rPowerlevel@D>     [@B%s@D]             @n\n\n", add_commas(ch->getPL()).c_str());
        ch->sendText("                 @D<@rHealth@D>              <@BKi@D>             <@GStamina@D>@n\n");
        ch->send_to("    @wCurrent   @D-[@R%-16s@D]-[@R%-16s@D]-[@R%-16s@D]@n\n", add_commas(ch->getCurVital(CharVital::health)).c_str(), add_commas(ch->getCurVital(CharVital::ki)).c_str(), add_commas(ch->getCurVital(CharVital::stamina)).c_str());
        ch->send_to("    @wMaximum   @D-[@r%-16s@D]-[@r%-16s@D]-[@r%-16s@D]@n\n", add_commas(ch->getEffectiveStat<int64_t>("health")).c_str(), add_commas(GET_MAX_MANA(ch)).c_str(), add_commas(GET_MAX_MOVE(ch)).c_str());
        ch->send_to("    @wBase      @D-[@m%-16s@D]-[@m%-16s@D]-[@m%-16s@D]@n\n", add_commas(ch->getBaseStat<int64_t>("health")).c_str(), add_commas(ch->getBaseStat<int64_t>("ki")).c_str(), add_commas(ch->getBaseStat<int64_t>("stamina")).c_str());
        if (!IS_ANDROID(ch) && (ch->getCurVital(CharVital::lifeforce)) > 0)
        {
            ch->send_to("    @wLife Force@D-[@C%16s@D%s@c%16s@D]- @wLife Percent@D-[@Y%3d%s@D]@n\n", add_commas(ch->getCurVital(CharVital::lifeforce)).c_str(), "/", add_commas(ch->getEffectiveStat("lifeforce")).c_str(), GET_LIFEPERC(ch), "%");
        }
        else if (!IS_ANDROID(ch))
        {
            ch->send_to("    @wLife Force@D-[@C%16s@D%s@c%16s@D]- @wLife Percent@D-[@Y%3d%s@D]@n\n", add_commas(0).c_str(), "/", add_commas(ch->getEffectiveStat("lifeforce")).c_str(), GET_LIFEPERC(ch), "%");
        }
    }
    std::string grav = "x1";
    if (ch->hasGravAcclim(5))
        grav = "x1000";
    else if (ch->hasGravAcclim(4))
        grav = "x100";
    else if (ch->hasGravAcclim(3))
        grav = "x50";
    else if (ch->hasGravAcclim(2))
        grav = "x10";
    else if (ch->hasGravAcclim(1))
        grav = "x5";
    else if (ch->hasGravAcclim(0))
        grav = "x2";

    if (view == full || view == stats)
    {
        ch->sendText("  @cO@D-----------------------------@D[ @cStatistics @D]-----------------------------@cO@n\n");
        ch->send_to("                   @D<@wGravity Acclim@D: @w" + grav + "@D> <@wRPP@D: @w%-3d@D>@n\n", GET_RP(ch));
        ch->send_to("               @D<@wSpeed Index@D: @w%-8s@D> <@wArmor Index@D: @w%-8s@D>@n\n", add_commas(GET_SPEEDI(ch)).c_str(), add_commas(GET_ARMOR(ch)).c_str());
        ch->send_to("  @D[@RStrength     @D|@G%2d (%3d)@D] [@YAgility      @D|@G%2d (%3d)@D] [@BSpeed        @D|@G%2d (%3d)@D]@n\n", ch->getBaseStat<int>("strength"), GET_STR(ch), ch->getBaseStat<int>("agility"), GET_DEX(ch), ch->getBaseStat<int>("speed"), GET_CHA(ch));
        ch->send_to("  @D[@gConstitution @D|@G%2d (%3d)@D] [@CIntelligence @D|@G%2d (%3d)@D] [@MWisdom       @D|@G%2d (%3d)@D]@n\n", ch->getBaseStat<int>("constitution"), GET_CON(ch), ch->getBaseStat<int>("intelligence"), GET_INT(ch), ch->getBaseStat<int>("wisdom"), GET_WIS(ch));
    }
    if (view == full || view == other)
    {
        ch->sendText("  @cO@D-----------------------------@D[   @cOther    @D]-----------------------------@cO@n\n");
        ch->sendText("                @D<@YZenni@D>                    <@rInventory Weight@D>@n\n");
        ch->send_to("      @D[   @CCarried@D| @W%-15s@D] [   @CCarried@D| @W%-15s@D]@n\n", add_commas(GET_GOLD(ch)).c_str(), add_commas((ch->getEffectiveStat("weight_carried"))).c_str());
        double gravity = 1.0;

        if (auto room = ch->getRoom(); room)
        {
            gravity = room->getEnvironment(ENV_GRAVITY);
        }
        std::string grav = gravity > 1.0 ? fmt::format("(Gravity:", gravity) : "";
        ch->send_to("      @D[      @CBank@D| @W%-15s@D] [ @CMax Carry@D| @W%-15s@D]@n %s\n", add_commas(GET_BANK_GOLD(ch)).c_str(), add_commas(CAN_CARRY_W(ch)).c_str(), grav.c_str());

        grav = gravity > 1.0 ? fmt::format("{}x)", add_commas(gravity)) : "";
        ch->send_to("      @D[ @CMax Carry@D| @W%-15s@D] [    @CBurden@D| @W%-15s@D]@n %s\n", add_commas(GOLD_CARRY(ch)).c_str(), add_commas(ch->getBaseStat("burden_current")).c_str(), grav.c_str());
        int numb = 0;
        if (GET_BANK_GOLD(ch) > 99)
        {
            numb = (GET_BANK_GOLD(ch) / 100) * 2;
        }
        else if (GET_BANK_GOLD(ch) > 0)
        {
            numb = 1;
        }
        else
        {
            numb = 0;
        }
        if (numb >= 7500)
        {
            numb = 7500;
        }
        auto ratio = std::to_string(ch->getBaseStat("burden_ratio") * 100.0) + "%";
        ch->send_to("      @D[  @CInterest@D| @W%-15s@D] [     @CRatio@D| @W%-15s@D]@n\n", add_commas(numb).c_str(), ratio.c_str());
        if (IS_ARLIAN(ch))
        {
            ch->sendText("                             @D<@GEvolution @D>@n\n");
            ch->send_to("      @D[ @CEvo Level@D| @W%-15d@D] [   @CEvo Exp@D| @W%-15s@D]\n", GET_MOLT_LEVEL(ch), add_commas(GET_MOLT_EXP(ch)).c_str());
            ch->send_to("      @D[ @CThreshold@D| @W%-15s@D]@n\n", add_commas(molt_threshold(ch)).c_str());
        }

        ch->send_to("\n     @D<@wPlayed@D: @yYears @D(@W%2d@D) @yWeeks @D(@W%2d@D) @yDays @D(@W%2d@D) @yHours @D(@W%2d@D) @yMinutes @D(@W%2d@D)>@n\n", (int64_t)ch->time.played / 31536000, (int)(((int64_t)ch->time.played % 31536000) / 604800), (int)(((int64_t)ch->time.played % 604800) / 86400), (int)(((int64_t)ch->time.played % 86400) / 3600), (int)(((int64_t)ch->time.played % 3600) / 60));
    }
    ch->sendText("  @cO@D------------------------------------------------------------------------@cO@n\n");
}

static void trans_check(Character *ch, Character *vict)
{
    /* Rillao: transloc, add new transes here */
    if (vict->form == Form::base || (vict->mimic && vict != ch))
    {
        ch->sendText("         @cCurrent Transformation@D: @wNone@n\r\n");
        return;
    }

    ch->send_to("         @cCurrent Transformation@D: %s\r\n", trans::getName(vict, vict->form));

} // End trans check

ACMD(do_status)
{
    char arg[MAX_INPUT_LENGTH];
    struct affected_type *aff;

    const char *forget_level[7] = {
        "@GRemembered Well@n",
        "@GRemembered Well Enough@n",
        "@RGetting Foggy@n",
        "@RHalf Forgotten@n",
        "@rAlmost Forgotten@n",
        "@rForgotten@n",
        "\n"};

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("@D<@b------------------------@D[@YYour Status@D]@b-------------------------@D>@n\r\n\r\n");
        ch->sendText("            @D---------------@RAppendages@D---------------\n");

        if (GET_LIMBCOND(ch, 0) >= 50 && !ch->character_flags.get(CharacterFlag::cyber_right_arm))
        {
            ch->send_to("            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 0), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 0) > 0 && !ch->character_flags.get(CharacterFlag::cyber_right_arm))
        {
            ch->send_to("            @D[@cRight Arm   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", GET_LIMBCOND(ch, 0), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 0) > 0 && ch->character_flags.get(CharacterFlag::cyber_right_arm))
        {
            ch->send_to("            @D[@cRight Arm   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", GET_LIMBCOND(ch, 0), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 0) <= 0)
        {
            ch->sendText("            @D[@cRight Arm   @D: @rMissing.         @D]@n\r\n");
        }
        if (GET_LIMBCOND(ch, 1) >= 50 && !ch->character_flags.get(CharacterFlag::cyber_left_arm))
        {
            ch->send_to("            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 1), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 1) > 0 && !ch->character_flags.get(CharacterFlag::cyber_left_arm))
        {
            ch->send_to("            @D[@cLeft Arm    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", GET_LIMBCOND(ch, 1), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 1) > 0 && ch->character_flags.get(CharacterFlag::cyber_left_arm))
        {
            ch->send_to("            @D[@cLeft Arm    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", GET_LIMBCOND(ch, 1), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 1) <= 0)
        {
            ch->sendText("            @D[@cLeft Arm    @D: @rMissing.         @D]@n\r\n");
        }
        if (GET_LIMBCOND(ch, 2) >= 50 && !ch->character_flags.get(CharacterFlag::cyber_left_arm))
        {
            ch->send_to("            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 2), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 2) > 0 && !ch->character_flags.get(CharacterFlag::cyber_right_leg))
        {
            ch->send_to("            @D[@cRight Leg   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", GET_LIMBCOND(ch, 2), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 2) > 0 && ch->character_flags.get(CharacterFlag::cyber_right_leg))
        {
            ch->send_to("            @D[@cRight Leg   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", GET_LIMBCOND(ch, 2), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 2) <= 0)
        {
            ch->sendText("            @D[@cRight Leg   @D: @rMissing.         @D]@n\r\n");
        }
        if (GET_LIMBCOND(ch, 3) >= 50 && !ch->character_flags.get(CharacterFlag::cyber_left_leg))
        {
            ch->send_to("            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", GET_LIMBCOND(ch, 3), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 3) > 0 && !ch->character_flags.get(CharacterFlag::cyber_left_leg))
        {
            ch->send_to("            @D[@cLeft Leg    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", GET_LIMBCOND(ch, 3), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 3) > 0 && ch->character_flags.get(CharacterFlag::cyber_left_leg))
        {
            ch->send_to("            @D[@cLeft Leg    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", GET_LIMBCOND(ch, 3), "%", "%");
        }
        else if (GET_LIMBCOND(ch, 3) <= 0)
        {
            ch->sendText("            @D[@cLeft Leg    @D: @rMissing.         @D]@n\r\n");
        }

        if (race::hasTail(ch->race) && !PLR_FLAGGED(ch, PLR_TAILHIDE))
        {
            if (ch->character_flags.get(CharacterFlag::tail))
                ch->sendText("            @D[@cTail        @D: @GHave.            @D]@n\r\n");
            else
                ch->sendText("            @D[@cTail        @D: @rMissing.         @D]@n\r\n");
        }

        ch->sendText("\r\n");

        ch->sendText("         @D-----------------@YHunger@D/@yThirst@D-----------------@n\r\n");
        auto hung = GET_COND(ch, HUNGER);
        if (hung >= 48)
        {
            ch->sendText("         You are full.\r\n");
        }
        else if (hung >= 40)
        {
            ch->sendText("         You are nearly full.\r\n");
        }
        else if (hung >= 30)
        {
            ch->sendText("         You are not hungry.\r\n");
        }
        else if (hung >= 21)
        {
            ch->sendText("         You wouldn't mind a snack.\r\n");
        }
        else if (hung >= 15)
        {
            ch->sendText("         You are slightly hungry.\r\n");
        }
        else if (hung >= 10)
        {
            ch->sendText("         You are partially hungry.\r\n");
        }
        else if (hung >= 5)
        {
            ch->sendText("         You are really hungry.\r\n");
        }
        else if (hung >= 2)
        {
            ch->sendText("         You are extremely hungry.\r\n");
        }
        else if (hung >= 0)
        {
            ch->sendText("         You are starving!\r\n");
        }
        else if (hung < 0)
        {
            ch->sendText("         You need not eat.\r\n");
        }

        auto thirst = GET_COND(ch, THIRST);
        if (thirst >= 48)
        {
            ch->sendText("         You are not thirsty.\r\n");
        }
        else if (thirst >= 40)
        {
            ch->sendText("         You are nearly quenched.\r\n");
        }
        else if (thirst >= 30)
        {
            ch->sendText("         You are not thirsty.\r\n");
        }
        else if (thirst >= 21)
        {
            ch->sendText("         You wouldn't mind a drink.\r\n");
        }
        else if (thirst >= 15)
        {
            ch->sendText("         You are slightly thirsty.\r\n");
        }
        else if (thirst >= 10)
        {
            ch->sendText("         You are partially thirsty.\r\n");
        }
        else if (thirst >= 5)
        {
            ch->sendText("         You are really thirsty.\r\n");
        }
        else if (thirst >= 2)
        {
            ch->sendText("         You are extremely thirsty.\r\n");
        }
        else if (thirst >= 0)
        {
            ch->sendText("         You are dehydrated!\r\n");
        }
        else if (thirst < 0)
        {
            ch->sendText("         You need not drink.\r\n");
        }
        ch->sendText("         @D--------------------@D[@GInfo@D]---------------------@n\r\n");
        trans_check(ch, ch);
        ch->send_to("         You have died %d times.\r\n", GET_DCOUNT(ch));
        if (PLR_FLAGGED(ch, PLR_NOSHOUT))
        {
            ch->sendText("         You have been @rmuted@n on public channels.\r\n");
        }
        if (ch->location == 9)
        {
            ch->sendText("         You are in punishment hell, so sad....\r\n");
        }
        if (!PRF_FLAGGED(ch, PRF_HINTS))
        {
            ch->sendText("         You have hints turned off.\r\n");
        }
        if (NEWSUPDATE > GET_LPLAY(ch))
        {
            ch->sendText("         Check the 'news', it has been updated recently.\r\n");
        }
        if (has_mail(GET_IDNUM(ch)))
        {
            ch->sendText("         Check your mail at the nearest postmaster.\r\n");
        }
        if (PRF_FLAGGED(ch, PRF_HIDE))
        {
            ch->sendText("         You are hidden from who and ooc.\r\n");
        }
        if (GET_VOICE(ch))
        {
            ch->send_to("         Your voice desc: '%s'\r\n", GET_VOICE(ch));
        }

        if (GET_PREFERENCE(ch) == 0)
        {
            ch->sendText("         You preferred a balanced form of fighting.\r\n");
        }
        else if (GET_PREFERENCE(ch) == PREFERENCE_KI)
        {
            ch->sendText("         You preferred a ki dominate form of fighting.\r\n");
        }
        else if (GET_PREFERENCE(ch) == PREFERENCE_WEAPON)
        {
            ch->sendText("         You preferred a weapon dominate form of fighting.\r\n");
        }
        else if (GET_PREFERENCE(ch) == PREFERENCE_H2H)
        {
            ch->sendText("         You preferred a body dominate form of fighting.\r\n");
        }
        else if (GET_PREFERENCE(ch) == PREFERENCE_THROWING)
        {
            ch->sendText("         You preferred a throwing dominate form of fighting.\r\n");
        }

        if (GET_EQ(ch, WEAR_EYE))
        {
            Object *obj = GET_EQ(ch, WEAR_EYE);
            if (SFREQ(obj) == 0)
            {
                obj->setBaseStat("scoutfreq", 1);
            }
            ch->send_to("         Your scouter is on frequency @G%d@n\r\n", SFREQ(obj));
            obj = nullptr;
        }
        if (GET_CHARGE(ch) > 0)
        {
            ch->send_to("         You have @C%s@n ki charged.\r\n", add_commas(GET_CHARGE(ch)).c_str());
        }
        if (GET_KAIOKEN(ch) > 0)
        {
            ch->send_to("         You are focusing Kaioken x %d.\r\n", GET_KAIOKEN(ch));
        }
        if (AFF_FLAGGED(ch, AFF_SANCTUARY))
        {
            ch->send_to("         You are surrounded by a barrier @D(@Y%s@D)@n\r\n", add_commas(GET_BARRIER(ch)).c_str());
        }
        if (AFF_FLAGGED(ch, AFF_FIRESHIELD))
        {
            ch->sendText("         You are surrounded by flames!@n\r\n");
        }
        if (GET_SUPPRESS(ch) > 0)
        {
            ch->send_to("         You are suppressing current PL to %" I64T ".\r\n", GET_SUPPRESS(ch));
        }
        if (IS_MAJIN(ch))
        {
            ch->send_to("         You have ingested %d people.\r\n", GET_ABSORBS(ch));
        }
        if (IS_BIO(ch))
        {
            ch->send_to("         You have %d absorbs left.\r\n", GET_ABSORBS(ch));
        }
        ch->send_to("         You have %s colored aura.\r\n", GET_AURA(ch));

        if (GET_LEVEL(ch) < 100)
        {
            if ((IS_ANDROID(ch) && ch->subrace == SubRace::android_model_absorb) || (!IS_ANDROID(ch) && !IS_BIO(ch) && !IS_MAJIN(ch)))
            {
                ch->send_to("         @R%s@n to SC a stat this level.\r\n", add_commas(ch->calc_soft_cap()).c_str());
            }
            else
            {
                ch->send_to("         @R%s@n in PL/KI/ST combined to SC this level.\r\n", add_commas(ch->calc_soft_cap()).c_str());
            }
        }
        else
        {
            ch->sendText("         Your strengths are potentially limitless.\r\n");
        }

        if (GET_FORGETING(ch) != 0)
        {
            ch->send_to("         @MForgetting @D[@m%s - %s@D]@n\r\n", spell_info[GET_FORGETING(ch)].name, forget_level[GET_FORGET_COUNT(ch)]);
        }
        else
        {
            ch->sendText("         @MForgetting @D[@mNothing.@D]@n\r\n");
        }

        if (GET_SKILL(ch, SKILL_DAGGER) > 0)
        {
            if (GET_BACKSTAB_COOL(ch) > 0)
            {
                ch->sendText("         @yYou can't preform a backstab yet.@n\r\n");
            }
            else
            {
                ch->sendText("         @YYou can backstab.@n\r\n");
            }
        }

        if (GET_FEATURE(ch))
        {
            ch->send_to("         Extra Feature: @C%s@n\r\n", GET_FEATURE(ch));
        }

        if (GET_RDISPLAY(ch))
        {
            ch->send_to("         Room Display: @C...%s@n\r\n", GET_RDISPLAY(ch));
        }

        ch->sendText("\r\n@D<@b-------------------------@D[@BCondition@D]@b--------------------------@D>@n\r\n");

        if (GET_BONUS(ch, BONUS_INSOMNIAC))
        {
            ch->sendText("You can not sleep.\r\n");
        }
        else
        {
            if (GET_SLEEPT(ch) > 6 && GET_POS(ch) != POS_SLEEPING)
            {
                ch->sendText("You are well rested.\r\n");
            }
            else if (GET_SLEEPT(ch) > 6 && GET_POS(ch) == POS_SLEEPING)
            {
                ch->sendText("You are getting the rest you need.\r\n");
            }
            else if (GET_SLEEPT(ch) > 4)
            {
                ch->sendText("You are rested.\r\n");
            }
            else if (GET_SLEEPT(ch) > 2)
            {
                ch->sendText("You are not sleepy.\r\n");
            }
            else if (GET_SLEEPT(ch) >= 1)
            {
                ch->sendText("You are getting a little sleepy.\r\n");
            }
            else if (GET_SLEEPT(ch) == 0)
            {
                ch->sendText("You could sleep at any time.\r\n");
            }
        }

        if (GET_RELAXCOUNT(ch) > 464)
        {
            ch->sendText("You are far too at ease to train hard like you should. Get out of the house more often.\r\n");
        }
        else if (GET_RELAXCOUNT(ch) > 232)
        {
            ch->sendText("You are too at ease to train hard like you should. Get out of the house more often.\r\n");
        }
        else if (GET_RELAXCOUNT(ch) > 116)
        {
            ch->sendText("You are a bit at ease and your training suffers. Get out of the house more often.\r\n");
        }

        if (ch->mimic)
        {
            ch->send_to("You are mimicing the general appearance of %s %s\r\n", AN(LRACE(ch)), LRACE(ch));
        }
        if (IS_MUTANT(ch))
        {
            auto mutations = fmt::format("Your Mutations: {}\r\n", fmt::join(ch->mutations.getAll(), ", "));
            // replace underscore with space
            std::replace(mutations.begin(), mutations.end(), '_', ' ');
            ch->send_to("%s", mutations.c_str());
        }
        if (IS_BIO(ch))
        {

            auto genes = fmt::format("Your genes carry: {} DNA.\r\n", fmt::join(ch->bio_genomes.getAll(), ","));
            ch->send_to("%s", genes.c_str());
        }
        if (AFF_FLAGGED(ch, AFF_KYODAIKA))
        {
            ch->sendText("You have used kyodaika.\r\n");
        }
        if (PRF_FLAGGED(ch, PRF_NOPARRY))
        {
            ch->sendText("You have decided not to parry attacks.\r\n");
        }
        switch (GET_POS(ch))
        {
        case POS_DEAD:
            ch->sendText("You are DEAD!\r\n");
            break;
        case POS_MORTALLYW:
            ch->sendText("You are mortally wounded! You should seek help!\r\n");
            break;
        case POS_INCAP:
            ch->sendText("You are incapacitated, slowly fading away...\r\n");
            break;
        case POS_STUNNED:
            ch->sendText("You are stunned! You can't move!\r\n");
            break;
        case POS_SLEEPING:
            ch->sendText("You are sleeping.\r\n");
            break;
        case POS_RESTING:
            ch->sendText("You are resting.\r\n");
            break;
        case POS_SITTING:
            ch->sendText("You are sitting.\r\n");
            break;
        case POS_FIGHTING:
            ch->send_to("You are fighting %s.\r\n", FIGHTING(ch) ? PERS(FIGHTING(ch), ch) : "thin air");
            break;
        case POS_STANDING:
            ch->sendText("You are standing.\r\n");
            break;
        default:
            ch->sendText("You are floating.\r\n");
            break;
        }

        if (has_group(ch))
        {
            ch->send_to("@GGroup Victories@D: @w%s@n\r\n", add_commas(GET_GROUPKILLS(ch)).c_str());
        }

        if (PLR_FLAGGED(ch, PLR_EYEC))
        {
            ch->sendText("Your eyes are closed.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_SNEAK))
        {
            ch->sendText("You are prepared to sneak where ever you go.\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_DISGUISED))
        {
            ch->sendText("You have disguised your facial features.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_FLYING))
        {
            ch->sendText("You are flying.\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_PILOTING))
        {
            ch->sendText("You are busy piloting a ship.\r\n");
        }
        if (GET_SONG(ch) > 0)
        {
            ch->send_to("You are playing @y'@Y%s@y'@n.\r\n", song_types[GET_SONG(ch)]);
        }

        if (AFF_FLAGGED(ch, AFF_ZANZOKEN))
        {
            ch->sendText("You are prepared to zanzoken.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_HASS))
        {
            ch->sendText("Your arms are moving fast.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_INFUSE))
        {
            ch->sendText("Your ki will be infused in your next physical attack.\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_TAILHIDE))
        {
            ch->sendText("Your tail is hidden!\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_NOGROW))
        {
            ch->sendText("Your tail is no longer regrowing!\r\n");
        }
        if (PLR_FLAGGED(ch, PLR_POSE))
        {
            ch->sendText("You are feeling confident from your pose earlier.\r\n");
        }
        if (AFF_FLAGGED(ch, AFF_HYDROZAP))
        {
            ch->sendText("You are effected by Kanso Suru.\r\n");
        }
        if (GET_COND(ch, DRUNK) > 15)
            ch->sendText("You are extremely drunk.\r\n");
        else if (GET_COND(ch, DRUNK) > 10)
            ch->sendText("You are pretty drunk.\r\n");
        else if (GET_COND(ch, DRUNK) > 4)
            ch->sendText("You are drunk.\r\n");
        else if (GET_COND(ch, DRUNK) > 0)
            ch->sendText("You have an alcoholic buzz.\r\n");

        if (ch->affected)
        {
            int lasttype = 0;
            for (aff = ch->affected; aff; aff = aff->next)
            {
                if (!strcasecmp(skill_name(aff->type), "runic") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Your Kenaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "punch") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Your Algiz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "knee") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Your Oagaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "slam") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Your Wunjo rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "heeldrop") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Your Purisaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "special beam cannon") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Your Laguz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "might") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Your muscles are pumped! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "flex") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You are more agile right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "bless") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You have been blessed! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "curse") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You have been cursed! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "healing glow") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You have a healing glow enveloping your body! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "genius") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You are smarter right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "enlighten") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You are wiser right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "yoikominminken") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You have been lulled to sleep! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "solar flare") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You have been blinded! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "spirit control") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You have full control of your spirit! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "!UNUSED!") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You feel poison burning through your blood! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "tough skin") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You have toughened skin right now! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "poison") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("You have been poisoned! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "warp pool") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Weakened State! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "dark metamorphosis") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Your Dark Metamorphosis is still in effect. (%2d Mud Hours)\r\n", aff->duration + 1);
                }
                if (!strcasecmp(skill_name(aff->type), "hayasa") && aff->type != lasttype)
                {
                    lasttype = aff->type;
                    ch->send_to("Your body has been infused to move faster! (%2d Mud Hours)\r\n", aff->duration + 1);
                }
            }
        }

        if (AFF_FLAGGED(ch, AFF_KNOCKED))
            ch->sendText("You have been knocked unconcious!\r\n");

        if (AFF_FLAGGED(ch, AFF_INVISIBLE))
            ch->sendText("You are invisible.\r\n");

        if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
            ch->sendText("You are sensitive to the presence of invisible things.\r\n");

        if (AFF_FLAGGED(ch, AFF_MBREAK))
            ch->sendText("Your mind has been broken!\r\n");

        if (AFF_FLAGGED(ch, AFF_WITHER))
            ch->sendText("You've been withered! You feel so weak...\r\n");

        if (AFF_FLAGGED(ch, AFF_SHOCKED))
            ch->sendText("Your mind has been shocked!\r\n");

        if (AFF_FLAGGED(ch, AFF_CHARM))
            ch->sendText("You have been charmed!\r\n");

        if (affected_by_spell(ch, SPELL_MAGE_ARMOR))
            ch->sendText("You feel protected.\r\n");

        if (AFF_FLAGGED(ch, AFF_INFRAVISION))
            ch->sendText("You can see in darkness with infravision.\r\n");

        if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
            ch->sendText("You are summonable by other players.\r\n");

        if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
            ch->sendText("You see into the hearts of others.\r\n");

        if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
            ch->sendText("You are sensitive to the magical nature of things.\r\n");

        if (AFF_FLAGGED(ch, AFF_SPIRIT))
            ch->sendText("You have died and are part of the SPIRIT world!\r\n");

        if (PRF_FLAGGED(ch, PRF_NOGIVE))
            ch->sendText("You are not accepting items being handed to you right now.\r\n");

        if (AFF_FLAGGED(ch, AFF_ETHEREAL))
            ch->sendText("You are ethereal and cannot interact with normal space!\r\n");

        if (auto reg = GET_REGEN(ch); reg > 0)
        {
            ch->send_to("Something is augmenting your regen rate by %s%d%s!\r\n", reg > 0 ? "+" : "-", reg, "%");
        }

        if (auto asb = GET_ASB(ch); asb > 0)
        {
            ch->send_to("Something is augmenting your auto-skill training rate by %s%d%s!\r\n", asb > 0 ? "+" : "-", asb, "%");
        }

        if (auto lb = ch->getBaseStat<int>("lifebonus"); lb > 0)
        {
            ch->send_to("Something is augmenting your Life Force Max by %s%d%s!\r\n", lb > 0 ? "+" : "-", lb, "%");
        }

        if (PLR_FLAGGED(ch, PLR_FISHING))
            ch->send_to("Current Fishing Pole Bonus @D[@C%d@D]@n\r\n", GET_POLE_BONUS(ch));

        if (PLR_FLAGGED(ch, PLR_AURALIGHT))
            ch->sendText("Aura Light is active.\r\n");
        ch->sendText("@D<@b--------------------------------------------------------------@D>@n\r\n");
        ch->sendText("To view your bonus/negative traits enter: status traits\r\n");
    }
    else if (!strcasecmp(arg, "traits"))
    {
        bonus_status(ch);
    }
    else
    {
        ch->sendText("The only argument status takes is 'traits'. If you just want your status do not use an argument.\r\n");
    }
}

const char *list_bonuses[] = {
    "Thrifty     - -10% Shop Buy Cost and +10% Shop Sell Cost          ",                                                                                                                                                              /* Bonus 0 */
    "Prodigy     - +25% Experience Gained Until Level 80               ",                                                                                                                                                              /* Bonus 1 */
    "Quick Study - Character auto-trains skills faster                 ",                                                                                                                                                              /* Bonus 2 */
    "Die Hard    - Life Force's PL regen doubled, but cost is the same ",                                                                                                                                                              /* Bonus 3 */
    "Brawler     - Physical attacks do 20% more damage                 ",                                                                                                                                                              /* Bonus 4 */
    "Destroyer   - Damaged Rooms act as regen rooms for you            ",                                                                                                                                                              /* Bonus 5 */
    "Hard Worker - Physical activity bonuses + drains less stamina     ",                                                                                                                                                              /* Bonus 6 */
    "Healer      - Heal/First-aid/Vigor/Repair restore +10%            ",                                                                                                                                                              /* Bonus 7 */
    "Loyal       - +20% Experience When Grouped As Follower            ",                                                                                                                                                              /* Bonus 8 */
    "Brawny      - Strength gains +2 every 10 levels, Train STR + 75%  ",                                                                                                                                                              /* Bonus 9 */
    "Scholarly   - Intelligence gains +2 every 10 levels, Train INT + 75%",                                                                                                                                                            /* Bonus 10 */
    "Sage        - Wisdom gains +2 every 10 levels, Train WIS + 75%    ",                                                                                                                                                              /* Bonus 11 */
    "Agile       - Agility gains +2 every 10 levels, Train AGL + 75%   ",                                                                                                                                                              /* Bonus 12 */
    "Quick       - Speed gains +2 every 10 levels, Train SPD + 75%     ",                                                                                                                                                              /* Bonus 13 */
    "Sturdy      - Constitution +2 every 10 levels, Train CON + 75%    ",                                                                                                                                                              /* Bonus 14 */
    "Thick Skin  - -20% Physical and -10% ki dmg received              ",                                                                                                                                                              /* Bonus 15 */
    "Recipe Int. - Food cooked by you lasts longer/heals better        ",                                                                                                                                                              /* Bonus 16 */
    "Fireproof   - -50% Fire Dmg taken, -10% ki, immunity to burn      ",                                                                                                                                                              /* Bonus 17 */
    "Powerhitter - 15% critical hits will be x4 instead of x2          ",                                                                                                                                                              /* Bonus 18 */
    "Healthy     - 40% chance to recover from ill effects when sleeping",                                                                                                                                                              /* Bonus  19 */
    "Insomniac   - Can't Sleep. Immune to yoikominminken and paralysis ",                                                                                                                                                              /* Bonus  20 */
    "Evasive     - +15% to dodge rolls                                 ",                                                                                                                                                              /* Bonus  21 */
    "The Wall    - +20% chance to block                                ",                                                                                                                                                              /* Bonus  22 */
    "Accurate    - +20% chance to hit physical, +10% to hit with ki     ",                                                                                                                                                             /* Bonus  23 */
    "Energy Leech- -2% ki damage received for every 5 character levels,\n                  @cas long as you can take that ki to your charge pool.@D        ",                                                                          /* Bonus  24*/
    "Good Memory - +2 Skill Slots initially, +1 every 20 levels after  ",                                                                                                                                                              /* Bonus 25 */
    "Soft Touch  - Half damage for all hit locations                   ",                                                                                                                                                              /* Neg 26 */
    "Late Sleeper- Can only wake automatically. 33% every hour if maxed",                                                                                                                                                              /* Neg 27 */
    "Impulse Shop- +25% shop costs                                     ",                                                                                                                                                              /* Neg 28 */
    "Sickly      - Suffer from harmful effects longer                  ",                                                                                                                                                              /* Neg 29 */
    "Punching Bag- -15% to dodge rolls                                 ",                                                                                                                                                              /* Neg 30 */
    "Pushover    - -20% block chance                                   ",                                                                                                                                                              /* Neg 31 */
    "Poor D. Perc- -20% chance to hit with physical, -10% with ki       ",                                                                                                                                                             /* Neg 32 */
    "Thin Skin   - +20% physical and +10% ki damage received           ",                                                                                                                                                              /* Neg 33 */
    "Fireprone   - +50% Fire Dmg taken, +10% ki, always burned         ",                                                                                                                                                              /* Neg 34 */
    "Energy Int. - +2% ki damage received for every 5 character levels,\n                  @rif you have ki charged you have 10% chance to lose   \n                  it and to take 1/4th damage equal to it.@D                    ", /* Neg 35 */
    "Coward      - Can't Attack Enemy With 150% Your Powerlevel        ",                                                                                                                                                              /* Neg 36 */
    "Arrogant    - Cannot Suppress                                     ",                                                                                                                                                              /* Neg 37 */
    "Unfocused   - Charge concentration randomly breaks                ",                                                                                                                                                              /* Neg 38 */
    "Slacker     - Physical activity drains more stamina               ",                                                                                                                                                              /* Neg 39 */
    "Slow Learner- Character auto-trains skills slower                 ",                                                                                                                                                              /* Neg 40 */
    "Masochistic - Defense Skills Cap At 75                            ",                                                                                                                                                              /* Neg 41 */
    "Mute        - Can't use IC speech related commands                ",                                                                                                                                                              /* Neg 42 */
    "Wimp        - Strength is capped at 45                            ",                                                                                                                                                              /* Neg 43 */
    "Dull        - Intelligence is capped at 45                        ",                                                                                                                                                              /* Neg 44 */
    "Foolish     - Wisdom is capped at 45                              ",                                                                                                                                                              /* Neg 45 */
    "Clumsy      - Agility is capped at 45                             ",                                                                                                                                                              /* Neg 46 */
    "Slow        - Speed is capped at 45                               ",                                                                                                                                                              /* Neg 47 */
    "Frail       - Constitution capped at 45                           ",                                                                                                                                                              /* Neg 48 */
    "Sadistic    - Half Experience Gained For Quick Kills              ",                                                                                                                                                              /* Neg 49 */
    "Loner       - Can't Group with anyone, +5% train and +10% Phys    ",                                                                                                                                                              /* Neg 50 */
    "Bad Memory  - -5 Skill Slots                                      "                                                                                                                                                               /* Neg 51 */
};

/* Display What Bonuses/Negatives Player Has */
static void bonus_status(Character *ch)
{
    int i, max = 52, count = 0;

    if (IS_NPC(ch))
        return;

    ch->sendText("@CYour Traits@n\n@D-----------------------------@w\n");
    for (i = 0; i < max; i++)
    {
        if (i < 26)
        {
            if (GET_BONUS(ch, i))
            {
                ch->send_to("@c%s@n\n", list_bonuses[i]);
                count++;
            }
        }
        else
        {
            if (i == 26)
            {
                ch->sendText("\r\n");
            }
            if (GET_BONUS(ch, i))
            {
                ch->send_to("@r%s@n\n", list_bonuses[i]);
                count++;
            }
        }
    }
    if (count <= 0)
    {
        ch->sendText("@wNone.\r\n");
    }
    ch->sendText("@D-----------------------------@n\r\n");
    return;
}

ACMD(do_inventory)
{
    ch->sendText("@w              @YInventory\r\n@D-------------------------------------@w\r\n");
    if (!IS_NPC(ch))
    {
        if (PLR_FLAGGED(ch, PLR_STOLEN))
        {
            ch->player_flags.set(PLR_STOLEN, false);
            ch->sendText("@r   --------------------------------------------------@n\n");
            ch->sendText("@R    You notice that you have been robbed sometime recently!\n");
            ch->sendText("@r   --------------------------------------------------@n\n");
            return;
        }
    }
    list_obj_to_char(ch->getInventory(), ch, SHOW_OBJ_SHORT, true);
    ch->sendText("\n");
}

static void show_equipment(Character *ch, Object *equipment, const char *wear_location, int wear_pos)
{
    ch->send_to("%s", wear_location);
    show_obj_to_char(equipment, ch, SHOW_OBJ_SHORT);

    if (OBJ_FLAGGED(equipment, ITEM_SHEATH))
    {
        auto con = equipment->getInventory();
        for (auto obj2 : filter_raw(con))
        {
            ch->sendText("@D  ---- @YSheathed@D ----@c> @n");
            show_obj_to_char(obj2, ch, SHOW_OBJ_SHORT);
        }
    }
}

ACMD(do_equipment)
{
    ch->sendText("        @YEquipment Being Worn\r\n@D-------------------------------------@w\r\n");

    for (int i = 1; i < NUM_WEARS; i++)
    {
        if (auto equipment = GET_EQ(ch, i); equipment)
        {
            if (CAN_SEE_OBJ(ch, equipment))
            {
                if (i != WEAR_WIELD1 && i != WEAR_WIELD2)
                {
                    show_equipment(ch, equipment, wear_where[i], i);
                }
                else if (PLR_FLAGGED(ch, PLR_THANDW))
                {
                    ch->sendText("@c<@CWielded by B. Hands@c>@n ");
                    show_obj_to_char(equipment, ch, SHOW_OBJ_SHORT);
                }
                else
                {
                    show_equipment(ch, equipment, wear_where[i], i);
                }
            }
            else
            {
                ch->send_to("%sSomething.\r\n", wear_where[i]);
            }
        }
        else
        {
            if (BODY_FLAGGED(ch, i) && (i != WEAR_WIELD2 || !PLR_FLAGGED(ch, PLR_THANDW)))
            {
                ch->send_to("%s@wNothing.@n\r\n", wear_where[i]);
            }
        }
    }
}

ACMD(do_time)
{
    const char *suf;
    int weekday, day;

    /* day in [1..30] */
    day = time_info.day + 1;

    /* 30 days in a month, 6 days a week */
    weekday = day % 6;

    ch->send_to("It is %02d:%02d:%02d o'clock %s, on %s.\r\n", (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12), time_info.minutes, time_info.seconds, time_info.hours >= 12 ? "PM" : "AM", weekdays[weekday]);

    /*
     * Peter Ajamian <peter@PAJAMIAN.DHS.ORG> supplied the following as a fix
     * for a bug introduced in the ordinal display that caused 11, 12, and 13
     * to be incorrectly displayed as 11st, 12nd, and 13rd.  Nate Winters
     * <wintersn@HOTMAIL.COM> had already submitted a fix, but it hard-coded a
     * limit on ordinal display which I want to avoid.	-dak
     */

    suf = "th";

    if (((day % 100) / 10) != 1)
    {
        switch (day % 10)
        {
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

    ch->send_to("The %d%s Day of the %s, Year %d.\r\n", day, suf, month_name[time_info.month], time_info.year);
}

ACMD(do_weather)
{
    const char *sky_look[] = {
        "cloudless",
        "cloudy",
        "rainy",
        "lit by flashes of lightning"};

    if (OUTSIDE(ch))
    {
        ch->send_to("The sky is %s and %s.\r\n", sky_look[weather_info.sky], weather_info.change >= 0 ? "you feel a warm wind from south" : "your foot tells you bad weather is due");
        if (ADM_FLAGGED(ch, ADM_KNOWWEATHER))
            ch->send_to("Pressure: %d (change: %d), Sky: %d (%s)\r\n", weather_info.pressure, weather_info.change, weather_info.sky, sky_look[weather_info.sky]);
    }
    else
        ch->sendText("You have no feeling about the weather at all.\r\n");
}

/* puts -'s instead of spaces */
static void space_to_minus(char *str)
{
    while ((str = strchr(str, ' ')))
        *str = '-';
}

int search_help(const char *argument, int level)
{
    int chk, bot, top, mid, minlen;

    bot = 0;
    top = top_of_helpt;
    minlen = strlen(argument);

    while (bot <= top)
    {
        mid = (bot + top) / 2;

        if (!(chk = strncasecmp(argument, help_table[mid].keywords, minlen)))
        {
            while ((mid > 0) && !strncasecmp(argument, help_table[mid - 1].keywords, minlen))
                mid--;

            while (level < help_table[mid].min_level && mid < (bot + top) / 2)
                mid++;

            if (strncasecmp(argument, help_table[mid].keywords, minlen))
                break;

            return mid;
        }
        else if (chk > 0)
            bot = mid + 1;
        else
            top = mid - 1;
    }
    return NOWHERE;
}

ACMD(do_help)
{
    char buf[MAX_STRING_LENGTH * 4];
    int mid = 0;

    if (!ch->desc)
        return;

    skip_spaces(&argument);

    if (!help_table)
    {
        ch->sendText("No help available.\r\n");
        return;
    }

    if (!*argument)
    {
        if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
        {
            ch->desc->sendText(help);
        }
        else
        {
            ch->desc->sendText(ihelp);
        }
        return;
    }

    space_to_minus(argument);

    if ((mid = search_help(argument, GET_ADMLEVEL(ch))) == NOWHERE)
    {
        int i, found = 0;
        ch->sendText("There is no help on that word.\r\n");
        if (GET_ADMLEVEL(ch) < 3)
        {
            mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), true, "%s tried to get help on %s", GET_NAME(ch),
                   argument);
        }
        for (i = 0; i <= top_of_helpt; i++)
        {
            if (help_table[i].min_level > GET_ADMLEVEL(ch))
                continue;
            /* To help narrow down results, if they don't start with the same letters, move on */
            if (*argument != *help_table[i].keywords)
                continue;
            if (levenshtein_distance(argument, help_table[i].keywords) <= 2)
            {
                if (!found)
                {
                    ch->sendText("\r\nDid you mean:\r\n");
                    found = 1;
                }
                ch->send_to("  %s\r\n", help_table[i].keywords);
            }
        }
        return;
    }
    if (help_table[mid].min_level > GET_ADMLEVEL(ch))
    {
        ch->sendText("There is no help on that word.\r\n");
        return;
    }
    sprintf(buf, "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
    sprintf(buf + strlen(buf), "%s", help_table[mid].entry);
    sprintf(buf + strlen(buf), "@b~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\n");
    if (GET_ADMLEVEL(ch) > 0)
    {
        sprintf(buf + strlen(buf), "@WHelp File Level@w: @D(@R%d@D)@n\n", help_table[mid].min_level);
    }
    ch->desc->send_to("%s", buf);
}

#define WHO_FORMAT \
    "Usage: who [minlev[-maxlev]] [-k] [-n name] [-q] [-r] [-s] [-z]\r\n"

/* Written by Rhade */
ACMD(do_who)
{
    struct descriptor_data *d;
    Character *tch;
    int i, num_can_see = 0;
    char name_search[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    int low = 0, high = CONFIG_LEVEL_CAP, localwho = 0, questwho = 0, hide = 0;
    int showclass = 0, short_list = 0, outlaws = 0;
    int who_room = 0, showgroup = 0, showleader = 0;
    const char *line_color = "@n";

    skip_spaces(&argument);
    strcpy(buf, argument); /* strcpy: OK (sizeof: argument == buf) */
    name_search[0] = '\0';

    struct
    {
        const char *disp;
        int min_level;
        int max_level;
        int count; /* must always start as 0 */
    } rank[] = {
        {"\r\n               @c------------  @D[    @gI@Gm@Wm@Do@Gr@Dt@Wa@Gl@gs   @D]  @c------------@n\r\n", ADMLVL_IMMORT, ADMLVL_IMPL, 0},
        {"\r\n@D[@wx@D]@yxxxxxxxxxx@W  [    @GImmortals   @W]  @yxxxxxxxxxx@D[@wx@D]@n\r\n", ADMLVL_IMMORT + 8, ADMLVL_GRGOD + 8, 0},
        {"\r\n               @c------------  @D[     @DM@ro@Rr@wt@Ra@rl@Ds    ]  @c------------@n\r\n", 0, ADMLVL_IMMORT - 1, 0}
        /*{ "\r\n@GAdministrators@n\r\n\r\n", ADMLVL_GRGOD, ADMLVL_IMPL, 0},
{ "\r\n@GImmortals@n\r\n\r\n"     , ADMLVL_IMMORT, ADMLVL_GRGOD - 1, 0},
{ "\r\n@GMortal@n\r\n\r\n"        , 0, ADMLVL_IMMORT - 1, 0 }*/
    };
    char *tmstr;
    tmstr = (char *)asctime(localtime(&PCOUNTDATE));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    int num_ranks = sizeof(rank) / sizeof(rank[0]);
    ch->sendText("\r\n      @r{@b===============  @D[  @DD@wr@ca@Cg@Y(@R*@Y)@Wn@cB@Da@cl@Cl @DA@wd@cv@Ce@Wnt @DT@wr@cu@Ct@Wh@n  @D]  @b===============@r}      @n\r\n");
    for (d = descriptor_list; d && !short_list; d = d->next)
    {
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

        if (CAN_SEE(ch, tch) && IS_PLAYING(d))
        {
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
            if (PRF_FLAGGED(tch, PRF_HIDE) && tch != ch && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
            {
                hide += 1;
                continue;
            }
            if (who_room && (tch->location != ch->location))
                continue;
            if (showgroup && (!tch->master || !AFF_FLAGGED(tch, AFF_GROUP)))
                continue;
            for (i = 0; i < num_ranks; i++)
                if (GET_ADMLEVEL(tch) >= rank[i].min_level && GET_ADMLEVEL(tch) <= rank[i].max_level)
                    rank[i].count++;
        }
    }

    for (i = 0; i < num_ranks; i++)
    {
        if (!rank[i].count && !short_list)
            continue;

        if (short_list)
            ch->sendText("Players\r\n-------\r\n");
        else
            ch->sendText(rank[i].disp);

        for (d = descriptor_list; d; d = d->next)
        {
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
            if (who_room && (tch->location != ch->location))
                continue;
            if (PRF_FLAGGED(tch, PRF_HIDE) && tch != ch && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
                continue;
            if (showgroup && (!tch->master || !AFF_FLAGGED(tch, AFF_GROUP)))
                continue;
            if (showleader && (!tch->followers || !AFF_FLAGGED(tch, AFF_GROUP)))
                continue;

            if (short_list)
            {
                ch->send_to("               @B[@W%3d @Y%s @C%s@B]@W %-12.12s@n%s@n", GET_LEVEL(tch), race::getAbbr(tch->race), sensei::getAbbr(tch->sensei).c_str(), GET_NAME(tch), ((!(++num_can_see % 4)) ? "\r\n" : ""));
            }
            else
            {
                num_can_see++;

                char usr[100];
                sprintf(usr, "@W(@R%s@W)%s", tch->desc->account->name.c_str(),
                        PLR_FLAGGED(tch, PLR_BIOGR) ? "" : (SPOILED(tch) ? " @R*@n" : ""));
                ch->send_to("%s               @D<@C%-12s@D> %s@w%s", line_color, GET_ADMLEVEL(ch) > 0 ? GET_NAME(tch) : (GET_ADMLEVEL(tch) > 0 ? GET_NAME(tch) : (GET_USER(tch) ? GET_USER(tch) : "nullptr")), GET_ADMLEVEL(ch) > 0 ? usr : "", line_color);

                if (GET_ADMLEVEL(tch))
                {
                    ch->send_to(" (%s)", admin_level_names[GET_ADMLEVEL(tch)]);
                }

                if (d->snooping && d->snooping->character != ch && GET_ADMLEVEL(ch) >= 3)
                    ch->send_to(" (Snoop: %s)", GET_NAME(d->snooping->character));
                if (GET_INVIS_LEV(tch))
                    ch->send_to(" (i%d)", GET_INVIS_LEV(tch));
                else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
                    ch->sendText(" (invis)");

                if (PLR_FLAGGED(tch, PLR_MAILING))
                    ch->sendText(" (mailing)");
                else if (d->olc)
                    ch->sendText(" (OLC)");
                else if (PLR_FLAGGED(tch, PLR_WRITING))
                    ch->sendText(" (writing)");

                if (d->original)
                    ch->sendText(" (out of body)");

                if (d->connected == CON_OEDIT)
                    ch->sendText(" (O Edit)");
                if (d->connected == CON_MEDIT)
                    ch->sendText(" (M Edit)");
                if (d->connected == CON_ZEDIT)
                    ch->sendText(" (Z Edit)");
                if (d->connected == CON_SEDIT)
                    ch->sendText(" (S Edit)");
                if (d->connected == CON_REDIT)
                    ch->sendText(" (R Edit)");
                if (d->connected == CON_TEDIT)
                    ch->sendText(" (T Edit)");
                if (d->connected == CON_TRIGEDIT)
                    ch->sendText(" (T Edit)");
                if (d->connected == CON_AEDIT)
                    ch->sendText(" (S Edit)");
                if (d->connected == CON_CEDIT)
                    ch->sendText(" (C Edit)");
                if (d->connected == CON_HEDIT)
                    ch->sendText(" (H Edit)");
                if (PRF_FLAGGED(tch, PRF_DEAF))
                    ch->sendText(" (DEAF)");
                if (PRF_FLAGGED(tch, PRF_NOTELL))
                    ch->sendText(" (NO TELL)");
                if (PRF_FLAGGED(tch, PRF_NOGOSS))
                    ch->sendText(" (NO OOC)");
                if (PLR_FLAGGED(tch, PLR_NOSHOUT))
                    ch->sendText(" (MUTED)");
                if (PRF_FLAGGED(tch, PRF_HIDE))
                    ch->sendText(" (WH)");
                if (PRF_FLAGGED(tch, PRF_BUILDWALK))
                    ch->sendText(" (Buildwalking)");
                if (PRF_FLAGGED(tch, PRF_AFK))
                    ch->sendText(" (AFK)");
                if (PLR_FLAGGED(tch, PLR_FISHING) && GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
                    ch->sendText(" (@BFISHING@n)");
                if (PRF_FLAGGED(tch, PRF_NOWIZ))
                    ch->sendText(" (NO WIZ)");
                ch->sendText("@n\r\n");
            }
        }
        ch->sendText("\r\n");
        if (short_list)
            break;
    }

    if (!num_can_see)
        ch->sendText("                            Nobody at all!\r\n");
    else if (num_can_see == 1)
        ch->sendText("                         One lonely character displayed.\r\n");
    else
    {
        ch->send_to("                           @Y%d@w characters displayed.\r\n", num_can_see);
        if (hide > 0)
        {
            int bam = false;
            if (hide > 1)
            {
                bam = true;
            }
            ch->send_to("                           and @Y%d@w character%s hidden.\r\n", hide, bam ? "s" : "");
        }
    }
    if (circle_restrict > 0 && circle_restrict <= 100)
    {
        ch->send_to("                      @rThe mud has been wizlocked to lvl %d@n\r\n", circle_restrict);
    }
    if (circle_restrict == 101)
    {
        ch->sendText("                      @rThe mud has been wizlocked to IMMs only.@n\r\n");
    }
    ch->sendText("      @r{@b=================================================================@r}@n\r\n");
    ch->send_to("           @cHighest Logon Count Ever@D: @Y%d@w, on %s\r\n", HIGHPCOUNT, tmstr);
    ch->send_to("                        @cHighest Logon Count Today@D: @Y%d@n\r\n", PCOUNT);
}

#define USERS_FORMAT \
    "format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p]\r\n"

/* BIG OL' FIXME: Rewrite it all. Similar to do_who(). */
ACMD(do_users)
{
    char line[200], line2[220], idletime[10];
    char state[30], *timeptr, mode;
    char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
    Character *tch;
    struct descriptor_data *d;
    int low = 0, high = CONFIG_LEVEL_CAP, num_can_see = 0;
    int showclass = 0, outlaws = 0, playing = 0, deadweight = 0, showrace = 0;
    char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

    host_search[0] = name_search[0] = '\0';

    strcpy(buf, argument); /* strcpy: OK (sizeof: argument == buf) */
    while (*buf)
    {
        char buf1[MAX_INPUT_LENGTH];

        half_chop(buf, arg, buf1);
        if (*arg == '-')
        {
            mode = *(arg + 1); /* just in case; we destroy arg in the switch */
            switch (mode)
            {
            case 'o':
            case 'k':
                outlaws = 1;
                playing = 1;
                strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
                break;
            case 'p':
                playing = 1;
                strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
                break;
            case 'd':
                deadweight = 1;
                strcpy(buf, buf1); /* strcpy: OK (sizeof: buf1 == buf) */
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
                ch->send_to("%s", USERS_FORMAT);
                return;
            } /* end of switch */
        }
        else
        { /* endif */
            ch->send_to("%s", USERS_FORMAT);
            return;
        }
    } /* end while (parser) */
    ch->sendText("Num Name                 User-name            State          Idl Login    C\r\n"
                 "--- -------------------- -------------------- -------------- --- -------- -\r\n");

    one_argument(argument, arg);

    for (d = descriptor_list; d; d = d->next)
    {
        if (STATE(d) != CON_PLAYING && playing)
            continue;
        if (STATE(d) == CON_PLAYING && deadweight)
            continue;
        if (IS_PLAYING(d))
        {
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
            if (PRF_FLAGGED(tch, PRF_HIDE) && tch != ch && GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
            {
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
            sprintf(idletime, "%3d", d->character->timer);
        else
            strcpy(idletime, "");

        sprintf(line, "%3d %-20s %-20s %-14s %-3s %-8s %1s ", -1,
                d->original && d->original->getName() ? d->original->getName() : d->character && d->character->getName() ? d->character->getName()
                                                                                                                         : "UNDEFINED",
                d->account ? d->account->name.c_str() : "UNKNOWN", state, idletime, timeptr,
                "N");

        if (d->host && *d->host)
            sprintf(line + strlen(line), "\n%3d [%s Site: %s]\r\n", -1, d->account ? d->account->name.c_str() : "UNKNOWN",
                    d->host);
        else
            sprintf(line + strlen(line), "\n%3d [%s Site: Hostname unknown]\r\n", -1,
                    d->account ? d->account->name.c_str() : "UNKNOWN");

        if (STATE(d) != CON_PLAYING)
        {
            sprintf(line2, "@g%s@n", line);
            strcpy(line, line2);
        }
        if (STATE(d) != CON_PLAYING ||
            (STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character)))
        {
            ch->send_to("%s", line);
            num_can_see++;
        }
    }

    ch->send_to("\r\n%d visible sockets connected.\r\n", num_can_see);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
    char arg[MAX_INPUT_LENGTH];
    char bum[10000];
    one_argument(argument, arg);

    switch (subcmd)
    {
    case SCMD_CREDITS:
    {
        ch->desc->sendText(credits);
    }
    break;
    case SCMD_NEWS:
    {
        ch->desc->sendText(news);
    }
        ch->setBaseStat("last_played", time(nullptr));
        break;
    case SCMD_INFO:
    {
        ch->desc->sendText(info);
    }
    break;
    case SCMD_WIZLIST:
    {
        ch->desc->sendText(wizlist);
    }
    break;
    case SCMD_IMMLIST:
    {
        ch->desc->sendText(immlist);
    }
    break;
    case SCMD_HANDBOOK:
    {
        ch->desc->sendText(handbook);
    }
    break;
    case SCMD_POLICIES:
        sprintf(bum, "--------------------\r\n%s\r\n--------------------\r\n", policies);
        ch->desc->sendText(bum);
        break;
    case SCMD_MOTD:
    {
        ch->desc->sendText(motd);
    }
    break;
    case SCMD_IMOTD:
    {
        ch->desc->sendText(imotd);
    }
    break;
    case SCMD_CLEAR:
        ch->sendText("\033[H\033[J");
        break;
    case SCMD_VERSION:
        ch->send_to("%s\r\n", circlemud_version);
        ch->send_to("%s\r\n", oasisolc_version);
        ch->send_to("%s\r\n", DG_SCRIPT_VERSION);
        ch->send_to("%s\r\n", CWG_VERSION);
        ch->send_to("%s\r\n", DBAT_VERSION);
        break;
    case SCMD_WHOAMI:
        ch->send_to("%s\r\n", GET_NAME(ch));
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

static void perform_mortal_where(Character *ch, char *arg)
{
    Character *i;
    struct descriptor_data *d;

    if (!*arg)
    {
        ch->sendText("Players in your Zone\r\n--------------------\r\n");
        for (d = descriptor_list; d; d = d->next)
        {
            if (STATE(d) != CON_PLAYING || d->character == ch)
                continue;
            if ((i = (d->original ? d->original : d->character)) == nullptr)
                continue;
            auto room = i->getRoom();
            if (!room || !CAN_SEE(ch, i))
                continue;
            if (ch->getRoom()->zone != room->zone)
                continue;
            ch->send_to("%-20s - %s\r\n", GET_NAME(i), room->getName());
        }
    }
    else
    { /* print only FIRST char, not all. */
        auto ac = characterSubscriptions.all("active");
        for (auto i : filter_raw(ac))
        {
            auto room = i->getRoom();
            if (!room || i == ch)
                continue;
            if (!CAN_SEE(ch, i) || room->zone != ch->getRoom()->zone)
                continue;
            if (!isname(arg, i->getName()))
                continue;
            ch->send_to("%-25s - %s\r\n", GET_NAME(i), room->getName());
            return;
        }
        ch->sendText("Nobody around by that name.\r\n");
    }
}

static void print_object_location(int num, Object *obj, Character *ch,
                                  int recur)
{
    if (num > 0)
        ch->send_to("O%3d. %-25s - ", num, obj->getShortDescription());
    else
        ch->send_to("%33s", " - ");

    if (!obj->getProtoScript().empty())
        ch->send_to("%s", obj->scriptString().c_str());

    if (auto r = obj->getRoom())
    {
        ch->send_to("[%5d] %s\r\n", r->getVnum(), r->getName());
    }
    else if (auto c = obj->getCarriedBy())
    {
        ch->send_to("carried by %s in room [%d]\r\n", PERS(c, ch), c->location.getVnum());
    }
    else if (auto c = obj->getWornBy())
    {
        ch->send_to("worn by %s in room [%d]\r\n", PERS(c, ch), c->location.getVnum());
    }
    else if (auto o = obj->getContainer())
    {
        ch->send_to("inside %s%s\r\n", o->getShortDescription(), (recur ? ", which is" : " "));
        if (recur)
            print_object_location(0, o, ch, recur);
    }
    else
    {
        ch->sendText("in an unknown location\r\n");
    }
}

static void perform_immort_where(Character *ch, char *arg)
{
    Character *i;
    Object *k;
    struct descriptor_data *d;
    int num = 0, num2 = 0, found = 0;
    std::optional<WhereFlag> planet;

    if (!*arg)
    {
        mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true,
               "GODCMD: %s has checked where to check player locations", GET_NAME(ch));
        ch->sendText("Players                  Vnum    Planet        Location\r\n-------                 ------   ----------    ----------------\r\n");
        for (d = descriptor_list; d; d = d->next)
            if (IS_PLAYING(d))
            {
                if (d->character->location)
                {
                    planet = getPlanet(d->character->location.getVnum());
                }
                else
                {
                    planet = {};
                }
                i = (d->original ? d->original : d->character);
                if (i && CAN_SEE(ch, i) && i->location)
                {
                    if (d->original)
                        ch->send_to("%-20s - [%5d]   %s (in %s)\r\n", GET_NAME(i), d->character->location.getVnum(), d->character->location.getName(), GET_NAME(d->character));
                    else
                    {
                        std::string locName = getPlanetName(planet.value());
                        ch->send_to("%-20s - [%5d]   %-14s %s\r\n", GET_NAME(i), i->location.getVnum(), locName.c_str(), i->location.getName());
                    }
                }
            }
    }
    else
    {
        mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true, "GODCMD: %s has checked where for the location of %s",
               GET_NAME(ch), arg);
        auto ac = characterSubscriptions.all("active");
        for (auto i : filter_raw(ac))
        {
            if (CAN_SEE(ch, i) && i->location && isname(arg, i->getName()))
            {
                found = 1;
                ch->send_to("M%3d. %-25s - [%5d] %-25s", ++num, GET_NAME(i), i->location.getVnum(), i->getRoom()->getName());
                if (IS_NPC(i) && !i->scripts.empty())
                {
                    auto t = i->scriptString();
                    ch->send_to("%s ", t.c_str());
                }
                ch->sendText("\r\n");
            }
        }
        auto ao = objectSubscriptions.all("active");
        for (auto k : filter_raw(ao))
        {
            if (CAN_SEE_OBJ(ch, k) && isname(arg, k->getName()))
            {
                found = 1;
                print_object_location(++num, k, ch, true);
            }
        }
        if (!found)
        {
            ch->sendText("Couldn't find any such thing.\r\n");
        }
        else
        {
            ch->send_to("\r\nFound %d matches.\r\n", num);
        }
    }
}

ACMD(do_where)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (ADM_FLAGGED(ch, ADM_FULLWHERE) || GET_ADMLEVEL(ch) > 4)
        perform_immort_where(ch, arg);
    else
        perform_mortal_where(ch, arg);
}

ACMD(do_levels)
{
    char buf[MAX_STRING_LENGTH];
    size_t i, len = 0, nlen;

    if (IS_NPC(ch))
    {
        ch->sendText("You ain't nothin' but a hound-dog.\r\n");
        return;
    }

    for (i = 1; i < 101; i++)
    {
        if (i == 100)
            nlen = snprintf(buf + len, sizeof(buf) - len, "[100] %8s          : \r\n", add_commas(level_exp(ch, 100)).c_str());
        else
            nlen = snprintf(buf + len, sizeof(buf) - len, "[%2" SZT "] %8s-%-8s : \r\n", i,
                            add_commas(level_exp(ch, i)).c_str(), add_commas(level_exp(ch, i + 1) - 1).c_str());
        if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
        len += nlen;
    }

    ch->desc->send_to("%s", buf);
}

ACMD(do_consider)
{
    char buf[MAX_INPUT_LENGTH];
    Character *victim;
    double diff;

    one_argument(argument, buf);

    if (!(victim = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("Consider killing who?\r\n");
        return;
    }
    if (victim == ch)
    {
        ch->sendText("Easy!  Very easy indeed!\r\n");
        return;
    }
    diff = (victim->getPL() / ch->getPL());

    if (diff <= 0.05)
        ch->sendText("Now where did that chicken go?\r\n");
    else if (diff <= 0.1)
        ch->sendText("You could do it with a needle!\r\n");
    else if (diff <= 0.4)
        ch->sendText("Easy.\r\n");
    else if (diff <= 0.8)
        ch->sendText("Fairly easy.\r\n");
    else if (diff == 1.1)
        ch->sendText("The perfect match!\r\n");
    else if (diff <= 1.3)
        ch->sendText("You could probably manage it.\r\n");
    else if (diff <= 1.5)
        ch->sendText("You might take a beating.\r\n");
    else if (diff <= 1.8)
        ch->sendText("You MIGHT win, maybe.\r\n");
    else if (diff <= 2)
        ch->sendText("Do you feel lucky? You better.\r\n");
    else if (diff <= 2.5)
        ch->sendText("Better bring some tough backup!\r\n");
    else if (diff <= 3)
        ch->sendText("Maybe if they are allergic to you, otherwise your last words will be 'Oh shit.'\r\n");
    else
        ch->sendText("No chance.\r\n");
}

ACMD(do_diagnose)
{
    char buf[MAX_INPUT_LENGTH];

    one_argument(argument, buf);

    Character *vict = FIGHTING(ch);

    if (*buf)
    {
        vict = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM);
        if (!vict)
        {
            ch->send_to("%s", CONFIG_NOPERSON);
            return;
        }
    }

    if (!vict)
    {
        ch->sendText("Diagnose who?\r\n");
        return;
    }

    ch->send_to("%s", HSSH(ch));
    diag_char_to_char(vict, ch);
}

static const char *ctypes[] = {
    "off", "on", "\n"};

char *cchoice_to_str(char *col)
{
    static char buf[READ_SIZE];
    int i = 0;
    bool fg = false, bold = false, needfg = false;

    if (!col)
    {
        buf[0] = '\0';
        return buf;
    }

    auto append_to_buf = [&](const char *text)
    {
        if (needfg && !fg)
        {
            i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
            fg = true;
        }
        if (i)
            i += snprintf(buf + i, sizeof(buf) - i, " ");
        if (bold)
        {
            i += snprintf(buf + i, sizeof(buf) - i, "bright ");
            bold = false;
        }
        i += snprintf(buf + i, sizeof(buf) - i, "%s", text);
    };

    while (*col)
    {
        if (strchr(ANSISTART, *col))
        {
            col++;
            continue;
        }

        switch (*col)
        {
        case ANSISEP:
        case ANSIEND:
        case '0':
            // Reset all states
            fg = bold = needfg = false;
            break;
        case '1':
            bold = true;
            break;
        case '5':
            append_to_buf("blinking");
            break;
        case '7':
            append_to_buf("reverse");
            break;
        case '8':
            append_to_buf("invisible");
            break;
        case '3':
            col++;
            switch (*col)
            {
            case '0':
                append_to_buf(bold ? "grey" : "black");
                break;
            case '1':
                append_to_buf("red");
                break;
            case '2':
                append_to_buf("green");
                break;
            case '3':
                append_to_buf("yellow");
                break;
            case '4':
                append_to_buf("blue");
                break;
            case '5':
                append_to_buf("magenta");
                break;
            case '6':
                append_to_buf("cyan");
                break;
            case '7':
                append_to_buf("white");
                break;
            default:
                break;
            }
            fg = true;
            break;
        case '4':
            col++;
            switch (*col)
            {
            case '0':
                append_to_buf("on black");
                break;
            case '1':
                append_to_buf("on red");
                break;
            case '2':
                append_to_buf("on green");
                break;
            case '3':
                append_to_buf("on yellow");
                break;
            case '4':
                append_to_buf("on blue");
                break;
            case '5':
                append_to_buf("on magenta");
                break;
            case '6':
                append_to_buf("on cyan");
                break;
            case '7':
                append_to_buf("on white");
                break;
            default:
                append_to_buf("underlined");
                break;
            }
            needfg = true;
            break;
        default:
            break;
        }
        col++;
    }

    if (!fg)
    {
        append_to_buf("normal");
    }

    return buf;
}

int str_to_cchoice(char *str, char *choice)
{
    char buf[MAX_STRING_LENGTH];
    int bold = 0, blink = 0, uline = 0, rev = 0, invis = 0, fg = 0, bg = 0, error = 0;
    int i, len = MAX_INPUT_LENGTH;
    struct
    {
        char *name;
        int *ptr;
    } attribs[] = {
        {"bright", &bold},
        {"bold", &bold},
        {"underlined", &uline},
        {"reverse", &rev},
        {"blinking", &blink},
        {"invisible", &invis},
        {nullptr, nullptr}};
    struct
    {
        char *name;
        int val;
        int bold;
    } colors[] = {
        {"default", -1, 0},
        {"normal", -1, 0},
        {"black", 0, 0},
        {"red", 1, 0},
        {"green", 2, 0},
        {"yellow", 3, 0},
        {"blue", 4, 0},
        {"magenta", 5, 0},
        {"cyan", 6, 0},
        {"white", 7, 0},
        {"grey", 0, 1},
        {"gray", 0, 1},
        {nullptr, 0, 0}};
    skip_spaces(&str);
    if (isdigit(*str))
    { /* Accept a raw code */
        strcpy(choice, str);
        for (i = 0; choice[i] && (isdigit(choice[i]) || choice[i] == ';'); i++)
            ;
        error = choice[i] != 0;
        choice[i] = 0;
        return error;
    }
    while (*str)
    {
        str = any_one_arg(str, buf);
        if (!strcmp(buf, "on"))
        {
            bg = 1;
            continue;
        }
        if (!fg)
        {
            for (i = 0; attribs[i].name; i++)
                if (!strncmp(attribs[i].name, buf, strlen(buf)))
                    break;
            if (attribs[i].name)
            {
                *(attribs[i].ptr) = 1;
                continue;
            }
        }
        for (i = 0; colors[i].name; i++)
            if (!strncmp(colors[i].name, buf, strlen(buf)))
                break;
        if (!colors[i].name)
        {
            error = 1;
            continue;
        }
        if (colors[i].val != -1)
        {
            if (bg == 1)
            {
                bg = 40 + colors[i].val;
            }
            else
            {
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
    /* COLOR_NORMAL */ AA_NORMAL,
    /* COLOR_ROOMNAME */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_ROOMOBJS */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_ROOMPEOPLE */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_HITYOU */ AA_NORMAL ANSISEPSTR AF_RED,
    /* COLOR_YOUHIT */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_OTHERHIT */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_CRITICAL */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_HOLLER */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_SHOUT */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_GOSSIP */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_AUCTION */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_CONGRAT */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_TELL */ AA_NORMAL ANSISEPSTR AF_RED,
    /* COLOR_YOUSAY */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_ROOMSAY */ AA_NORMAL ANSISEPSTR AF_WHITE,
    nullptr};

ACMD(do_color)
{
    char arg[MAX_INPUT_LENGTH];
    char *p;
    int tp;

    /*if (IS_NPC(ch))
    return;*/

    p = any_one_arg(argument, arg);

    if (!*arg)
    {
        ch->sendText("Usage: color [ off | on ]\r\n");
        return;
    }
    if (((tp = search_block(arg, ctypes, false)) == -1))
    {
        ch->sendText("Usage: color [ off | on ]\r\n");
        return;
    }
    switch (tp)
    {
    case C_OFF:
        ch->pref_flags.set(PRF_COLOR, false);
        break;
    case C_ON:
        ch->pref_flags.set(PRF_COLOR, true);
        break;
    }
    ch->send_to("Your color is now @o%s@n.\r\n", ctypes[tp]);
}

ACMD(do_toggle)
{
    char buf2[4];

    if (IS_NPC(ch))
        return;

    if (GET_WIMP_LEV(ch) == 0)
        strcpy(buf2, "OFF"); /* strcpy: OK */
    else
        sprintf(buf2, "%-3.3d", GET_WIMP_LEV(ch)); /* sprintf: OK */

    if (GET_ADMLEVEL(ch))
    {
        ch->send_to("      Buildwalk: %-3s    "
                    "Clear Screen in OLC: %-3s\r\n",
                    ONOFF(PRF_FLAGGED(ch, PRF_BUILDWALK)), ONOFF(PRF_FLAGGED(ch, PRF_CLS)));

        ch->send_to("      No Hassle: %-3s    "
                    "      Holylight: %-3s    "
                    "     Room Flags: %-3s\r\n",
                    ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)), ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)), ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)));
    }

    ch->send_to("Hit Pnt Display: %-3s    "
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
                ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)), ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)), ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)), ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)), ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)), YESNO(PRF_FLAGGED(ch, PRF_QUEST)), ONOFF(PRF_FLAGGED(ch, PRF_DISPEXP)), ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)), YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)), ONOFF(PRF_FLAGGED(ch, PRF_DISPKI)), YESNO(PRF_FLAGGED(ch, PRF_DEAF)), buf2, ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)), ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)), ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)), ctypes[COLOR_LEV(ch)], ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOMEM)), ONOFF(PRF_FLAGGED(ch, PRF_VIEWORDER)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)), ONOFF(PRF_FLAGGED(ch, PRF_DISPTNL)));
}

static int sort_commands_helper(const void *a, const void *b)
{
    return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                  complete_cmd_info[*(const int *)b].sort_as);
}

void sort_commands()
{
    int a, num_of_cmds = 0;

    while (complete_cmd_info[num_of_cmds].command[0] != '\n')
        num_of_cmds++;
    num_of_cmds++; /* \n */

    CREATE(cmd_sort_info, int, num_of_cmds);

    for (a = 0; a < num_of_cmds; a++)
        cmd_sort_info[a] = a;

    /* Don't sort the RESERVED or \n entries. */
    qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}

ACMD(do_commands)
{
    int no, i, cmd_num;
    int wizhelp = 0, socials = 0;
    Character *vict = ch;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (*arg)
    {
        if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)) || IS_NPC(vict))
        {
            ch->sendText("Who is that?\r\n");
            return;
        }
        if (GET_LEVEL(ch) < GET_LEVEL(vict))
        {
            ch->sendText("You can't see the commands of people above your level.\r\n");
            return;
        }
    }

    if (subcmd == SCMD_SOCIALS)
        socials = 1;
    else if (subcmd == SCMD_WIZHELP)
        wizhelp = 1;

    ch->send_to("The following %s%s are available to %s:\r\n", wizhelp ? "privileged " : "", socials ? "socials" : "commands", vict == ch ? "you" : GET_NAME(vict));

    /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
    for (no = 1, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++)
    {
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

        ch->send_to("%-11s%s", arg, no++ % 7 == 0 ? "\r\n" : "");
    }

    if (no % 7 != 1)
        ch->sendText("\r\n");
}

ACMD(do_history)
{
    char arg[MAX_INPUT_LENGTH];
    int type;

    one_argument(argument, arg);
    if (IS_NPC(ch))
        return;

    auto &p = players.at(ch->id);

    type = search_block(arg, history_types, false);
    if (!*arg || type < 0)
    {
        int i;

        ch->sendText("Usage: history <");
        for (i = 0; *history_types[i] != '\n'; i++)
        {
            if ((i != 3 && GET_ADMLEVEL(ch) <= 0) || GET_ADMLEVEL(ch) >= 1)
            {
                ch->send_to(" %s ", history_types[i]);
            }
            if (*history_types[i + 1] == '\n')
            {
                ch->sendText(">\r\n");
            }
            else
            {
                if ((i != 3 && GET_ADMLEVEL(ch) <= 0) || GET_ADMLEVEL(ch) >= 1)
                {
                    ch->sendText("|");
                }
            }
        }
        return;
    }

    if (p.comm_hist[type] && p.comm_hist[type]->text && *p.comm_hist[type]->text)
    {
        struct txt_block *tmp;
        for (tmp = p.comm_hist[type]; tmp; tmp = tmp->next)
            ch->send_to("%s", tmp->text);
    }
    else
        ch->sendText("You have no history in that channel.\r\n");
}

void add_history(Character *ch, char *str, int type)
{
    int i = 0;
    char time_str[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    struct txt_block *tmp;
    time_t ct;

    if (IS_NPC(ch))
        return;

    auto &p = players.at(ch->id);

    tmp = p.comm_hist[type];
    ct = time(nullptr);
    strftime(time_str, sizeof(time_str), "%H:%M ", localtime(&ct));

    sprintf(buf, "%s%s", time_str, str);

    if (!tmp)
    {
        CREATE(p.comm_hist[type], struct txt_block, 1);
        p.comm_hist[type]->text = strdup(buf);
    }
    else
    {
        while (tmp->next)
            tmp = tmp->next;
        CREATE(tmp->next, struct txt_block, 1);
        tmp->next->text = strdup(buf);

        for (tmp = p.comm_hist[type]; tmp; tmp = tmp->next, i++)
            ;

        for (; i > HIST_LENGTH && p.comm_hist[type]; i--)
        {
            tmp = p.comm_hist[type];
            p.comm_hist[type] = tmp->next;
            if (tmp->text)
                free(tmp->text);
            free(tmp);
        }
    }
    /* add this history message to ALL */
    if (type != HIST_ALL)
        add_history(ch, str, HIST_ALL);
}

ACMD(do_scan)
{
    int i;
    char *dirnames[] = {
        "North", "East", "South", "West", "Up", "Down", "Northwest", "Northeast", "Southeast", "Southwest",
        "Inside", "Outside"};

    if (GET_POS(ch) < POS_SLEEPING)
    {
        ch->sendText("You can't see anything but stars!\r\n");
        return;
    }
    if (!AWAKE(ch))
    {
        ch->sendText("You must be dreaming.\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_BLIND))
    {
        ch->sendText("You can't see a damn thing, you're blind!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_EYEC))
    {
        ch->sendText("You can't see a damned thing, your eyes are closed!\r\n");
        return;
    }

    if (!ch->location)
    {
        ch->sendText("You are nowhere.\r\n");
        return;
    }

    auto darkHere = ch->location.getIsDark();

    for (i = 0; i < 10; i++)
    {
        auto d = ch->location.getExit(static_cast<Direction>(i));
        if (!d)
            continue;
        auto &dest = d.value();

        if (darkHere && (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) &&
            (!AFF_FLAGGED(ch, AFF_INFRAVISION)))
        {
            ch->send_to("%s: DARK\r\n", dirnames[i]);
            continue;
        }

        if (!dest)
            continue;
        if (IS_SET(dest.exit_info, EX_CLOSED))
            continue;

        ch->sendText("@w-----------------------------------------@n\r\n");
        ch->send_to("          %s%s: %s %s\r\n", CCCYN(ch, C_NRM), dirnames[i], dest.getName() ? dest.getName() : "You don't think you saw what you just saw.", CCNRM(ch, C_NRM));
        ch->sendText("@W          -----------------          @n\r\n");

        list_obj_to_char(dest.getObjects(), ch, SHOW_OBJ_LONG, false);
        list_char_to_char(dest.getPeople(), ch);
        auto ge = dest.getGroundEffect();
        if (ge >= 1 && ge <= 5)
        {
            ch->sendText("@rLava@w is pooling in someplaces here...@n\r\n");
        }
        if (ge >= 6)
        {
            ch->sendText("@RLava@r covers pretty much the entire area!@n\r\n");
        }
        /* Check 2nd room away */

        auto d2 = dest.getExit(static_cast<Direction>(i));
        if (!d2)
            continue;
        auto &dest2 = d2.value();
        if (!dest2)
            continue;
        if (IS_SET(dest2.exit_info, EX_CLOSED))
            continue;

        if (!dest2.getIsDark())
        {
            ch->sendText("@w-----------------------------------------@n\r\n");
            ch->send_to("          %sFar %s: %s %s\r\n", CCCYN(ch, C_NRM), dirnames[i], dest2.getName() ? dest2.getName() : "You don't think you saw what you just saw.", CCNRM(ch, C_NRM));
            ch->sendText("@W          -----------------          @n\r\n");

            list_obj_to_char(dest2.getObjects(), ch, SHOW_OBJ_LONG, false);
            list_char_to_char(dest2.getPeople(), ch);
            auto ge2 = dest2.getGroundEffect();
            if (ge2 >= 1 && ge2 <= 5)
            {
                ch->sendText("@rLava@w is pooling in someplaces here...@n\r\n");
            }
            if (ge2 >= 6)
            {
                ch->sendText("@RLava@r covers pretty much the entire area!@n\r\n");
            }
        }
        else
        {
            ch->send_to("%s<-> %sFar %s: Too dark to tell! %s<->%s\r\n", QMAG, QCYN, dirnames[i], QMAG, QNRM);
        }
    }
    ch->sendText("@w-----------------------------------------@n\r\n");
}

ACMD(do_toplist)
{
    if (IS_NPC(ch))
        return;

    FILE *file;
    char fname[40], filler[50], line[256];
    int64_t points[25] = {0}, stats;
    char *title[25] = {""};
    int count = 0, x = 0;

    /* Read Introduction File */
    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist"))
    {
        ch->sendText("The toplist file does not exist.");
        return;
    }

    if (!(file = fopen(fname, "r")))
    {
        ch->sendText("The toplist file does not exist.");
        return;
    }

    while (!feof(file) || count < 25)
    {
        get_line(file, line);
        switch (count)
        {
        default:
            sscanf(line, "%s %" I64T "\n", filler, &stats);
            break;
        }
        title[count] = strdup(filler);
        points[count] = stats;
        count++;
        *filler = '\0';
    }

    ch->send_to("@D-=[@BDBAT Top Lists for @REra@C %d@D]=-@n\r\n", CURRENT_ERA);
    while (x <= count)
    {
        switch (x)
        {
        /* Powerlevel Area */
        case 0:
            ch->sendText("       @D-@RPowerlevel@D-@n\r\n");
            ch->send_to("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 1:
            ch->send_to("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 2:
            ch->send_to("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 3:
            ch->send_to("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 4:
            ch->send_to("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
            /* Ki Area */
        case 5:
            ch->sendText("       @D-@BKi        @D-@n\r\n");
            ch->send_to("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 6:
            ch->send_to("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 7:
            ch->send_to("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 8:
            ch->send_to("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 9:
            ch->send_to("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
            /* Stamina Area */
        case 10:
            ch->sendText("       @D-@GStamina   @D-@n\r\n");
            ch->send_to("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 11:
            ch->send_to("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 12:
            ch->send_to("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 13:
            ch->send_to("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 14:
            ch->send_to("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
            /* Stamina Area */
        case 15:
            ch->sendText("       @D-@gZenni     @D-@n\r\n");
            ch->send_to("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 16:
            ch->send_to("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 17:
            ch->send_to("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 18:
            ch->send_to("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
        case 19:
            ch->send_to("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);
            free(title[x]);
            break;
            /* Rpp Area */
        case 20:
            /*                ch->sendText("       @D-@mRPP       @D-@n\r\n");
      ch->send_to("    @D|@c1@W: @C%13s@D|@n\r\n", title[x]);*/
            free(title[x]);
            break;
        case 21:
            /*                ch->send_to("    @D|@c2@W: @C%13s@D|@n\r\n", title[x]);*/
            free(title[x]);
            break;
        case 22:
            /*                ch->send_to("    @D|@c3@W: @C%13s@D|@n\r\n", title[x]);*/
            free(title[x]);
            break;
        case 23:
            /*                ch->send_to("    @D|@c4@W: @C%13s@D|@n\r\n", title[x]);*/
            free(title[x]);
            break;
        case 24:
            /*                ch->send_to("    @D|@c5@W: @C%13s@D|@n\r\n", title[x]);*/
            free(title[x]);
            break;
        }
        x++;
    }
    fclose(file);
}

ACMD(do_whois)
{
    char buf[MAX_INPUT_LENGTH];
    int clan = false;
    const char *immlevels[ADMLVL_IMPL + 2] = {
        "[Mortal]",               /* lowest admin level */
        "[Enforcer]",             /* lowest admin level +1 */
        "[First Class Enforcer]", /* lowest admin level +2 */
        "[High Enforcer]",        /* lowest admin level +3 */
        "[Vice Admin]",           /* lowest admin level +4 */
        "[Administrator]",        /* lowest admin level +5 */
        "[Implementor]",
    };

    skip_spaces(&argument);

    if (!*argument)
    {
        ch->sendText("Who?\r\n");
        return;
    }

    auto victim = findPlayer(argument);
    if (!victim)
    {
        ch->sendText("There is no such player.\r\n");
        return;
    }

    if (GET_CLAN(victim))
    {
        if (!strstr(GET_CLAN(victim), "None"))
        {
            sprintf(buf, "%s", GET_CLAN(victim));
            clan = true;
        }
        if (strstr(GET_CLAN(victim), "Applying"))
        {
            sprintf(buf, "%s", GET_CLAN(victim));
            clan = true;
        }
    }
    if (GET_CLAN(victim) == nullptr || strstr(GET_CLAN(victim), "None"))
    {
        clan = false;
    }
    ch->sendText("@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
    if (GET_ADMLEVEL(victim) >= ADMLVL_IMMORT)
    {
        ch->send_to("@cName     @D: @G%s\r\n", GET_NAME(victim));
        ch->send_to("@cImm Level@D: @G%s\r\n", immlevels[GET_ADMLEVEL(victim)]);
        ch->send_to("@cTitle    @D: @G%s\r\n", GET_TITLE(victim));
    }
    else
    {
        ch->send_to("@cName  @D: @w%s\r\n@cSensei@D: @w%s\r\n@cRace  @D: @w%s\r\n@cTitle @D: @w%s@n\r\n@cClan  @D: @w%s@n\r\n", GET_NAME(victim), sensei::getName(victim->sensei), race::getName(victim->race), GET_TITLE(victim), clan ? buf : "None.");
        if (clan == true && !strstr(GET_CLAN(victim), "Applying"))
        {
            if (checkCLAN(victim) == true)
            {
                clanRANKD(GET_CLAN(victim), ch, victim);
            }
        }
    }
    ch->sendText("@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
}

static void search_in_direction(Character *ch, int dir)
{
    int check = false, skill_lvl, dchide = 20;

    ch->sendText("You search for secret doors.\r\n");
    act("$n searches the area intently.", true, ch, nullptr, nullptr, TO_ROOM);

    auto ex = ch->location.getExit(static_cast<Direction>(dir));

    if (!ex)
    {
        ch->sendText("There is no exit there.\r\n");
        return;
    }
    auto &dest = ex.value();
    if (!dest)
    {
        ch->sendText("That leads nowhere.\r\n");
        return;
    }

    /* SEARCHING is allowed untrained */
    skill_lvl = GET_SKILL(ch, SKILL_SEARCH);
    if (IS_TRUFFLE(ch) || IS_HUMAN(ch))
        skill_lvl = skill_lvl + 2;
    if (IS_HALFBREED(ch))
        skill_lvl = skill_lvl + 1;

    dchide = dest.dchide;

    if (skill_lvl > dchide)
        check = true;

    if (!dest.general_description.empty() &&
        !EXIT_FLAGGED(&dest, EX_SECRET))
        ch->sendText(dest.general_description.c_str());
    else if (!EXIT_FLAGGED(&dest, EX_SECRET))
        ch->sendText("There is a normal exit there.\r\n");
    else if (EXIT_FLAGGED(&dest, EX_ISDOOR) &&
             EXIT_FLAGGED(&dest, EX_SECRET) &&
             !dest.keyword.empty() && (check == true))
        ch->send_to("There is a hidden door keyword: '%s' %sthere.\r\n", fname(dest.keyword.c_str()), (EXIT_FLAGGED(&dest, EX_CLOSED)) ? "" : "open ");
    else
        ch->sendText("There is no exit there.\r\n");
}

ACMD(do_oaffects)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("You must specify a target location.\r\n");
        return;
    }

    int location = -1;
    int i = 0;
    while (strcasecmp(apply_types[i], "\n"))
    {
        if (!strcasecmp(apply_types[i], arg))
        {
            location = i;
            break;
        }
        i++;
    }

    if (location == -1)
    {
        ch->sendText("That is not a valid apply location.\r\n");
        return;
    }

    ch->send_to("Affects for %s:\r\n", apply_types[location]);
    int counter = 0;
    for (auto &[vn, o] : obj_proto)
    {
        bool found = false;
        for (auto &aff : o.affected)
        {
            if (aff.location == location)
            {
                found = true;
                break;
            }
        }
        if (!found)
            continue;
        ch->send_to("[%d] %s\r\n", vn, o.short_description);
        counter++;
    }
    if (!counter)
    {
        ch->sendText("None.\r\n");
    }
}

ACMD(do_desc)
{
    auto d = ch->desc;
    if (!d)
    {
        return;
    }
    if (ch->form == Form::base)
    {
        d->send_to("Current description:\r\n%s\r\n", ch->getLookDescription());
        d->sendText("Enter the new text you'd like others to see when they look at you.\r\n");
        string_write(d, &ch->strings["look_description"], EXDSCR_LENGTH, 0, ch->strings["look_description"]);
        STATE(d) = CON_EXDESC;
    }
    else
    {
        auto form = ch->form;

        d->send_to("Current description for %s:\r\n%s\r\n", trans::getName(ch, form), ch->transforms[form].description);
        d->sendText("Enter the new text you'd like others to see when they look at you in this form.\r\n");

        string_write(d, &ch->transforms[form].description, EXDSCR_LENGTH, 0, ch->transforms[form].description);
        STATE(d) = CON_EXDESC;
    }
}