/* ************************************************************************
 *   File: utils.c                                       Part of CircleMUD *
 *  Usage: various internal functions of a utility nature                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include <cstdarg>
#include <fstream>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cstddef>
#include "dbat/CharacterUtils.h"
#include "dbat/ObjectUtils.h"
#include "dbat/Zone.h"
#include "dbat/Descriptor.h"
#include "dbat/RoomUtils.h"
#include "dbat/ObjectPrototype.h"
#include "dbat/CharacterPrototype.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/spells.h"
#include "dbat/db.h"
#include "dbat/fight.h"
#include "dbat/class.h"
//#include "dbat/feats.h"
#include "dbat/constants.h"
#include "dbat/act.informative.h"
#include "dbat/screen.h"
#include "dbat/players.h"
#include "dbat/act.other.h"
//#include "dbat/planet.h"
//#include "dbat/send.h"
#include "dbat/ansi.h"
#include "dbat/filter.h"
#include "dbat/Random.h"
#include "dbat/weather.h"
#include "dbat/TimeInfo.h"
#include "dbat/Parse.h"

#include "dbat/const/Pulse.h"
#include "dbat/const/Environment.h"
#include "dbat/const/ColorCustom.h"
#include "dbat/const/Filename.h"

/* local functions */
char commastring[MAX_STRING_LENGTH];

void dispel_ash(Character *ch)
{

    auto ash = ch->location.searchObjects(1306);
    if (!ash)
        return;

    int roll = axion_dice(0);
    bool success = false;
    if (GET_OBJ_COST(ash) == 3)
        success = GET_INT(ch) > roll;
    else if (GET_OBJ_COST(ash) == 2)
        success = GET_INT(ch) + 10 > roll;
    else if (GET_OBJ_COST(ash) == 1)
        success = GET_INT(ch) + 20 > roll;
    if (!success)
        return;
    act("@GYou clear the air with the shockwaves of your power!@n", true, ch, ash, nullptr, TO_CHAR);
    act("@C$n@G clears the air with the shockwaves of $s power!@n", true, ch, ash, nullptr, TO_ROOM);
    extract_obj(ash);
}

int has_group(Character *ch)
{

    if (!AFF_FLAGGED(ch, AFF_GROUP))
        return (false);

    if (auto foll = ch->followers.snapshot_shared(); !foll.empty())
    {
        for (auto k : foll)
        {
            if (!k->affect_flags.get(AFF_GROUP))
            {
                continue;
            }
            else
            {
                return (true);
            }
        }
    }
    else if (ch->master)
    {
        return AFF_FLAGGED(ch->master, AFF_GROUP);
    }

    return (false);
}

const char *report_party_health(Character *ch)
{

    if (!AFF_FLAGGED(ch, AFF_GROUP))
        return ("");

    if (!ch->followers && !ch->master)
        return ("");


    int count = 0, stam1 = 8, stam2 = 8, stam3 = 8, stam4 = 8, plc1 = 4, plc2 = 4, plc3 = 4, plc4 = 4;
    Character *party1 = nullptr, *party2 = nullptr, *party3 = nullptr, *party4 = nullptr;
    int64_t plperc1 = 0, plperc2 = 0, plperc3 = 0, plperc4 = 0;
    int64_t kiperc1 = 0, kiperc2 = 0, kiperc3 = 0, kiperc4 = 0;
    char result_party_health[MAX_STRING_LENGTH], result1[MAX_STRING_LENGTH], result2[MAX_STRING_LENGTH], result3[MAX_STRING_LENGTH], result4[MAX_STRING_LENGTH], result5[MAX_STRING_LENGTH];

    const char *plcol[5] = {"@r",
                            "@y",
                            "@Y",
                            "@G",
                            ""};

    const char *exhaust[9] = {"Exhausted",   /* 0/7 */
                              "Strained",    /* 1/7 */
                              "Very Tired",  /* 2/7 */
                              "Tired",       /* 3/7 */
                              "Kinda Tired", /* 4/7 */
                              "Very Winded", /* 5/7 */
                              "Winded",      /* 6/7 */
                              "Energetic",   /* 7/7 */
                              "?????????"};

    const char *excol[9] = {"@r", /* 0/7 */
                            "@R", /* 1/7 */
                            "@R", /* 2/7 */
                            "@M", /* 3/7 */
                            "@M", /* 4/7 */
                            "@M", /* 5/7 */
                            "@G", /* 6/7 */
                            "@g", /* 7/7 */
                            "@w"};
    
    if (auto foll = ch->followers.snapshot_weak(); !foll.empty())
    {
        for (auto k : filter_raw(foll))
        {
            if (!AFF_FLAGGED(k, AFF_GROUP))
                continue;
            if (k != ch)
            {
                count += 1;
                if (count == 1)
                {
                    party1 = k;
                    plperc1 = (GET_HIT(party1) * 100) / GET_MAX_HIT(party1);
                    kiperc1 = (GET_CHARGE(party1) * 100) / GET_MAX_MANA(party1);

                    if (plperc1 >= 80)
                        plc1 = 3;
                    else if (plperc1 >= 50)
                        plc1 = 2;
                    else if (plperc1 >= 30)
                        plc1 = 1;
                    else
                        plc1 = 0;

                    if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1))
                    {
                        stam1 = 7;
                    }
                    else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .9)
                    {
                        stam1 = 6;
                    }
                    else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .8)
                    {
                        stam1 = 5;
                    }
                    else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .7)
                    {
                        stam1 = 4;
                    }
                    else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .5)
                    {
                        stam1 = 3;
                    }
                    else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .4)
                    {
                        stam1 = 2;
                    }
                    else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .2)
                    {
                        stam1 = 1;
                    }
                    else
                    {
                        stam1 = 0;
                    }
                }
                else if (count == 2)
                {
                    party2 = k;
                    plperc2 = (GET_HIT(party2) * 100) / GET_MAX_HIT(party2);
                    kiperc2 = (GET_CHARGE(party2) * 100) / GET_MAX_MANA(party2);

                    if (plperc2 >= 80)
                        plc2 = 3;
                    else if (plperc2 >= 50)
                        plc2 = 2;
                    else if (plperc2 >= 30)
                        plc2 = 1;
                    else
                        plc2 = 0;

                    if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2))
                    {
                        stam2 = 7;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .9)
                    {
                        stam2 = 6;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .8)
                    {
                        stam2 = 5;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .7)
                    {
                        stam2 = 4;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .5)
                    {
                        stam2 = 3;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .4)
                    {
                        stam2 = 2;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .2)
                    {
                        stam2 = 1;
                    }
                    else
                    {
                        stam2 = 0;
                    }
                }
                else if (count == 3)
                {
                    party3 = k;
                    plperc3 = (GET_HIT(party3) * 100) / GET_MAX_HIT(party3);
                    kiperc3 = (GET_CHARGE(party3) * 100) / GET_MAX_MANA(party3);

                    if (plperc3 >= 80)
                        plc3 = 3;
                    else if (plperc3 >= 50)
                        plc3 = 2;
                    else if (plperc3 >= 30)
                        plc3 = 1;
                    else
                        plc3 = 0;

                    if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3))
                    {
                        stam3 = 7;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .9)
                    {
                        stam3 = 6;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .8)
                    {
                        stam3 = 5;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .7)
                    {
                        stam3 = 4;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .5)
                    {
                        stam3 = 3;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .4)
                    {
                        stam3 = 2;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .2)
                    {
                        stam3 = 1;
                    }
                    else
                    {
                        stam3 = 0;
                    }
                }
                else if (count == 4)
                {
                    party4 = k;
                    plperc4 = (GET_HIT(party4) * 100) / GET_MAX_HIT(party4);
                    kiperc4 = (GET_CHARGE(party4) * 100) / GET_MAX_MANA(party4);

                    if (plperc4 >= 80)
                        plc4 = 3;
                    else if (plperc4 >= 50)
                        plc4 = 2;
                    else if (plperc4 >= 30)
                        plc4 = 1;
                    else
                        plc4 = 0;

                    if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4))
                    {
                        stam4 = 7;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .9)
                    {
                        stam4 = 6;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .8)
                    {
                        stam4 = 5;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .7)
                    {
                        stam4 = 4;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .5)
                    {
                        stam4 = 3;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .4)
                    {
                        stam4 = 2;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .2)
                    {
                        stam4 = 1;
                    }
                    else
                    {
                        stam4 = 0;
                    }
                }
            }
        }
        snprintf(result1, sizeof(result1),
                 "@D[@BG@D]-------@mF@D------- -------@mF@D------- -------@mF@D------- -------@mF@D-------[@BG@D] <@RV@Y%s@R>@n\n",
                 add_commas(GET_GROUPKILLS(ch)).c_str());
        snprintf(result2, sizeof(result2), "@D[@BR@D]@C%-15s %-15s %-15s %-15s@D[@BR@D]@n\n", party1 ? get_i_name(ch, party1) : "Empty",
                 party2 ? get_i_name(ch, party2) : "Empty", party3 ? get_i_name(ch, party3) : "Empty",
                 party4 ? get_i_name(ch, party4) : "Empty");
        snprintf(result3, sizeof(result3),
                 "@D[@BO@D]@RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T "@w%s@D[@BO@D]@n\n",
                 plcol[plc1], plperc1, "%", plcol[plc2], plperc2, "%", plcol[plc3], plperc3, "%", plcol[plc4], plperc4,
                 "%");
        snprintf(result4, sizeof(result4),
                 "@D[@BU@D]@cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T "@w%s@D[@BU@D]@n\n",
                 kiperc1, "%", kiperc2, "%", kiperc3, "%", kiperc4, "%");
        snprintf(result5, sizeof(result5), "@D[@BP@D]@gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s@D[@BP@D]@n", excol[stam1],
                 exhaust[stam1], excol[stam2], exhaust[stam2], excol[stam3], exhaust[stam3], excol[stam4],
                 exhaust[stam4]);
        snprintf(result_party_health, sizeof(result_party_health), "%s%s%s%s%s\n", result1, result2, result3, result4, result5);
        ch->temp_prompt = strdup(result_party_health);
        return (ch->temp_prompt);
    }
    else if (ch->master && AFF_FLAGGED(ch->master, AFF_GROUP))
    {
        party1 = ch->master;
        plperc1 = (GET_HIT(party1) * 100) / GET_MAX_HIT(party1);
        kiperc1 = (GET_CHARGE(party1) * 100) / GET_MAX_MANA(party1);

        if (plperc1 >= 80)
            plc1 = 3;
        else if (plperc1 >= 50)
            plc1 = 2;
        else if (plperc1 >= 30)
            plc1 = 1;
        else
            plc1 = 0;

        if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1))
        {
            stam1 = 7;
        }
        else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .9)
        {
            stam1 = 6;
        }
        else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .8)
        {
            stam1 = 5;
        }
        else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .7)
        {
            stam1 = 4;
        }
        else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .5)
        {
            stam1 = 3;
        }
        else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .4)
        {
            stam1 = 2;
        }
        else if ((party1->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party1) * .2)
        {
            stam1 = 1;
        }
        else
        {
            stam1 = 0;
        }
        count = 1;

        if(auto pfoll = party1->followers.snapshot_weak(); !pfoll.empty())
        for (auto k : filter_raw(pfoll))
        {
            if (!AFF_FLAGGED(k, AFF_GROUP))
                continue;
            if (k != ch)
            {
                count += 1;
                if (count == 2)
                {
                    party2 = k;
                    plperc2 = (GET_HIT(party2) * 100) / GET_MAX_HIT(party2);
                    kiperc2 = (GET_CHARGE(party2) * 100) / GET_MAX_MANA(party2);

                    if (plperc2 >= 80)
                        plc2 = 3;
                    else if (plperc2 >= 50)
                        plc2 = 2;
                    else if (plperc2 >= 30)
                        plc2 = 1;
                    else
                        plc2 = 0;

                    if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2))
                    {
                        stam2 = 7;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .9)
                    {
                        stam2 = 6;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .8)
                    {
                        stam2 = 5;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .7)
                    {
                        stam2 = 4;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .5)
                    {
                        stam2 = 3;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .4)
                    {
                        stam2 = 2;
                    }
                    else if ((party2->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party2) * .2)
                    {
                        stam2 = 1;
                    }
                    else
                    {
                        stam2 = 0;
                    }
                }
                else if (count == 3)
                {
                    party3 = k;
                    plperc3 = (GET_HIT(party3) * 100) / GET_MAX_HIT(party3);
                    kiperc3 = (GET_CHARGE(party3) * 100) / GET_MAX_MANA(party3);

                    if (plperc3 >= 80)
                        plc3 = 3;
                    else if (plperc3 >= 50)
                        plc3 = 2;
                    else if (plperc3 >= 30)
                        plc3 = 1;
                    else
                        plc3 = 0;

                    if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3))
                    {
                        stam3 = 7;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .9)
                    {
                        stam3 = 6;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .8)
                    {
                        stam3 = 5;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .7)
                    {
                        stam3 = 4;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .5)
                    {
                        stam3 = 3;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .4)
                    {
                        stam3 = 2;
                    }
                    else if ((party3->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party3) * .2)
                    {
                        stam3 = 1;
                    }
                    else
                    {
                        stam3 = 0;
                    }
                }
                else if (count == 4)
                {
                    party4 = k;
                    plperc4 = (GET_HIT(party4) * 100) / GET_MAX_HIT(party4);
                    kiperc4 = (GET_CHARGE(party4) * 100) / GET_MAX_MANA(party4);

                    if (plperc4 >= 80)
                        plc4 = 3;
                    else if (plperc4 >= 50)
                        plc4 = 2;
                    else if (plperc4 >= 30)
                        plc4 = 1;
                    else
                        plc4 = 0;

                    if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4))
                    {
                        stam4 = 7;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .9)
                    {
                        stam4 = 6;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .8)
                    {
                        stam4 = 5;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .7)
                    {
                        stam4 = 4;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .5)
                    {
                        stam4 = 3;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .4)
                    {
                        stam4 = 2;
                    }
                    else if ((party4->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(party4) * .2)
                    {
                        stam4 = 1;
                    }
                    else
                    {
                        stam4 = 0;
                    }
                }
            } /* Is follower */
        } /* End for */

        snprintf(result1, sizeof(result1),
                 "@D[@BG@D]-------@YL@D------- -------@mF@D------- -------@mF@D------- -------@mF@D-------[@BG@D]@n\n");
        snprintf(result2, sizeof(result2), "@D[@BR@D]@C%-15s %-15s %-15s %-15s@D[@BR@D]@n\n", party1 ? get_i_name(ch, party1) : "Empty",
                 party2 ? get_i_name(ch, party2) : "Empty", party3 ? get_i_name(ch, party3) : "Empty",
                 party4 ? get_i_name(ch, party4) : "Empty");
        snprintf(result3, sizeof(result3),
                 "@D[@BO@D]@RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T "@w%s @RPL@D:%s%11" I64T "@w%s@D[@BO@D]@n\n",
                 plcol[plc1], plperc1, "%", plcol[plc2], plperc2, "%", plcol[plc3], plperc3, "%", plcol[plc4], plperc4,
                 "%");
        snprintf(result4, sizeof(result4),
                 "@D[@BU@D]@cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T "@w%s @cCharge@D:@B%7" I64T "@w%s@D[@BU@D]@n\n",
                 kiperc1, "%", kiperc2, "%", kiperc3, "%", kiperc4, "%");
        snprintf(result5, sizeof(result5), "@D[@BP@D]@gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s@D[@BP@D]@n", excol[stam1],
                 exhaust[stam1], excol[stam2], exhaust[stam2], excol[stam3], exhaust[stam3], excol[stam4],
                 exhaust[stam4]);
        snprintf(result_party_health, sizeof(result_party_health), "%s%s%s%s%s\n", result1, result2, result3, result4, result5);
        ch->temp_prompt = strdup(result_party_health);
        return (ch->temp_prompt);
    }
    else
    {
        return ("");
    }
}

/* Check to see if anything interferes with their "knowing" the skill */
int know_skill(Character *ch, int skill)
{

    int know = 0;

    if (GET_SKILL(ch, skill) > 0)
        know = 1;
    if (GET_STUPIDKISS(ch) == skill)
        know = 2;

    if (know == 0)
    {
        ch->send_to("You do not know how to perform %s.\r\n", spell_info[skill].name);
        know = 0;
    }
    else if (know == 2)
    {
        ch->send_to("@WYou try to use @M%s@W but lingering thoughts of a certain kiss distracts you!@n\r\n", spell_info[skill].name);
        ch->sendText("You must sleep in order to cure this.\r\n");
        know = 0;
    }

    return (know);
}

/* Add should be set to the amount you want to add to whatever is rolled. */
int roll_aff_duration(int num, int add)
{
    int start = num / 20;
    int finish = num / 10;
    int outcome = add;

    outcome += Random::get<int>(start, finish);

    return (outcome);
}

void null_affect(Character *ch, AffectFlag aff_flag)
{
    struct affected_type *af, *next_af;

    for (af = ch->affected; af; af = next_af)
    {
        next_af = af->next;
        if (af->location == APPLY_NONE && af->aff_flags.get(aff_flag))
            affect_remove(ch, af);
    }

    if (aff_flag == AFF_POISON)
    {
        if (ch->poisonby)
        {
            auto shared = ch->shared_from_this();
            ch->poisonby->poisoned.remove(shared);
            ch->poisonby = nullptr;
        }
    }
}

void assign_affect(Character *ch, AffectFlag aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis,
                   int spd)
{
    struct affected_type af[6];
    int num = 0;

    if (dur <= 0)
        dur = 1;

    if (str == 0 && con == 0 && wis == 0 && intel == 0 && agl == 0 && spd == 0)
    {
        af[num].type = skill;
        af[num].duration = dur;
        af[num].modifier = 0;
        af[num].location = APPLY_NONE;
        af[num].aff_flags.set(aff_flag);
        affect_join(ch, &af[num], false, false, false, false);
        num += 1;
    }
    return;

    if (str != 0)
    {
        af[num].type = skill;
        af[num].duration = dur;
        af[num].modifier = str;
        af[num].location = APPLY_CATTR_BASE;
        af[num].specific = static_cast<int>(CharAttribute::strength);
        af[num].aff_flags.set(aff_flag);
        affect_join(ch, &af[num], false, false, false, false);
        num += 1;
    }
    if (con != 0)
    {
        af[num].type = skill;
        af[num].duration = dur;
        af[num].modifier = con;
        af[num].location = APPLY_CATTR_BASE;
        af[num].specific = static_cast<int>(CharAttribute::constitution);
        af[num].aff_flags.set(aff_flag);
        affect_join(ch, &af[num], false, false, false, false);
        num += 1;
    }
    if (intel != 0)
    {
        af[num].type = skill;
        af[num].duration = dur;
        af[num].modifier = intel;
        af[num].location = APPLY_CATTR_BASE;
        af[num].specific = static_cast<int>(CharAttribute::intelligence);
        af[num].aff_flags.set(aff_flag);
        affect_join(ch, &af[num], false, false, false, false);
        num += 1;
    }
    if (agl != 0)
    {
        af[num].type = skill;
        af[num].duration = dur;
        af[num].modifier = agl;
        af[num].location = APPLY_CATTR_BASE;
        af[num].specific = static_cast<int>(CharAttribute::agility);
        af[num].aff_flags.set(aff_flag);
        affect_join(ch, &af[num], false, false, false, false);
        num += 1;
    }
    if (spd != 0)
    {
        af[num].type = skill;
        af[num].duration = dur;
        af[num].modifier = spd;
        af[num].location = APPLY_CATTR_BASE;
        af[num].specific = static_cast<int>(CharAttribute::speed);
        af[num].aff_flags.set(aff_flag);
        affect_join(ch, &af[num], false, false, false, false);
        num += 1;
    }
    if (wis != 0)
    {
        af[num].type = skill;
        af[num].duration = dur;
        af[num].modifier = wis;
        af[num].location = APPLY_CATTR_BASE;
        af[num].specific = static_cast<int>(CharAttribute::wisdom);
        af[num].aff_flags.set(aff_flag);
        affect_join(ch, &af[num], false, false, false, false);
        num += 1;
    }
}

int sec_roll_check(Character *ch)
{

    int figure = 0, chance = 0, outcome = 0;

    figure = 10 + (GET_CHA(ch) * 1.6);

    chance = axion_dice(0) + axion_dice(0) + Random::get<int>(0, 20);

    if (figure >= chance)
        outcome = 1;

    return (outcome);
}

int64_t physical_cost(Character *ch, int skill)
{

    int64_t result = 0;

    if (skill == SKILL_PUNCH)
        result = GET_MAX_HIT(ch) / 500;
    else if (skill == SKILL_KICK)
        result = GET_MAX_HIT(ch) / 350;
    else if (skill == SKILL_ELBOW)
        result = GET_MAX_HIT(ch) / 400;
    else if (skill == SKILL_KNEE)
        result = GET_MAX_HIT(ch) / 300;
    else if (skill == SKILL_UPPERCUT)
        result = GET_MAX_HIT(ch) / 200;
    else if (skill == SKILL_ROUNDHOUSE)
        result = GET_MAX_HIT(ch) / 150;
    else if (skill == SKILL_HEELDROP)
        result = GET_MAX_HIT(ch) / 80;
    else if (skill == SKILL_SLAM)
        result = GET_MAX_HIT(ch) / 90;

    int cou1 = 1 + Random::get<int>(1, 20), cou2 = cou1 + Random::get<int>(1, 6);

    result += Random::get<int>(cou1, cou2);

    if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100)
    {
        result -= result * 0.4;
    }
    else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75)
    {
        if (IS_TSUNA(ch))
        {
            result -= result * 0.40;
        }
        else if (IS_TAPION(ch) && (skill == SKILL_PUNCH || skill == SKILL_KICK))
        {
            result -= result * 0.35;
        }
        else if (IS_JINTO(ch))
        {
            if (GET_SKILL_BASE(ch, skill) >= 100)
            {
                result -= result * 0.45;
            }
            else
            {
                result -= result * 0.25;
            }
        }
        else
        {
            result -= result * 0.25;
        }
    }
    else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 50)
    {
        result -= result * 0.25;
    }

    if (IS_ANDROID(ch))
    {
        result *= 0.25;
    }

    return (result);
}

const char *disp_align(Character *ch)
{
    int align;

    if (GET_ALIGNMENT(ch) < -800)
    { // Horrifically Evil
        align = 8;
    }
    else if (GET_ALIGNMENT(ch) < -600)
    { // Terrible
        align = 7;
    }
    else if (GET_ALIGNMENT(ch) < -300)
    { // Villain
        align = 6;
    }
    else if (GET_ALIGNMENT(ch) < -50)
    { // Crook
        align = 5;
    }
    else if (GET_ALIGNMENT(ch) < 51)
    { // Neutral
        align = 4;
    }
    else if (GET_ALIGNMENT(ch) < 300)
    { // Do-gooder
        align = 3;
    }
    else if (GET_ALIGNMENT(ch) < 600)
    { // Hero
        align = 2;
    }
    else if (GET_ALIGNMENT(ch) < 800)
    { // Valiant
        align = 1;
    }
    else
    { // Saintly
        align = 0;
    }

    return (alignments[align]);
}

int read_sense_memory(Character *ch, Character *vict)
{
    /* Read Sense File */
    if (!vict || vict == ch)
    {
        return 0;
    }

    if (IS_NPC(ch))
        return 0;

    auto &p = players.at(ch->id);

    if (IS_NPC(vict))
    {
        return p->sense_memory.contains(vict->getVnum());
    }
    else
    {
        return p->sense_player.contains(vict->id);
    }
}

/* This writes a player's sense memory to file. */
void sense_memory_write(Character *ch, Character *vict)
{
    if (!vict || vict == ch)
    {
        return;
    }

    if (IS_NPC(ch))
        return;
    auto &p = players.at(ch->id);

    if (IS_NPC(vict))
    {
        p->sense_memory.insert(vict->getVnum());
    }
    else
    {
        p->sense_player.insert(vict->id);
    }
}

/* Will they manage to pursue a fleeing enemy? */
int roll_pursue(Character *ch, Character *vict)
{

    int skill, perc = axion_dice(0);

    if (ch == nullptr || vict == nullptr)
        return (false);

    if (!IS_NPC(ch))
    {
        skill = GET_SKILL(ch, SKILL_PURSUIT);
    }
    else if (IS_NPC(ch) && !MOB_FLAGGED(ch, MOB_SENTINEL))
    {
        skill = GET_LEVEL(ch);
        if (vict->location.getRoomFlag(ROOM_NOMOB))
            skill = -1;
    }
    else
    {
        skill = -1;
    }

    if (!IS_NPC(vict))
    {
        if (IS_NPC(ch) && !(vict->desc))
        {
            skill = -1;
        }
    }

    if (skill > perc)
    {
        act("@C$n@R pursues after the fleeing @c$N@R!@n", true, ch, nullptr, vict, TO_NOTVICT);
        ch->leaveLocation();
        ch->moveToLocation(vict->location);
        act("@GYou pursue right after @c$N@G!@n", true, ch, nullptr, vict, TO_CHAR);
        act("@C$n@R pursues after you!@n", true, ch, nullptr, vict, TO_VICT);
        act("@C$n@R pursues after the fleeing @c$N@R!@n", true, ch, nullptr, vict, TO_NOTVICT);

        struct follow_type *k, *next;
        
        ch->followers.for_each([&](Character* k) {
            if ((k->location == ch->location) && (GET_POS(k) >= POS_STANDING) &&
                    (!AFF_FLAGGED(ch, AFF_ZANZOKEN) ||
                     (AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(k, AFF_GROUP))))
                {
                    act("You follow $N.", true, k, nullptr, ch, TO_CHAR);
                    act("$n follows after $N.", true, k, nullptr, ch, TO_NOTVICT);
                    act("$n follows after you.", true, k, nullptr, ch, TO_VICT);
                    k->leaveLocation();
                    k->moveToLocation(ch->location);
                }
        });
        vict->affect_flags.set(AFF_PURSUIT, false);
        return (true);
    }
    else
    {
        ch->sendText("@RYou fail to pursue after them!@n\r\n");
        for (auto c : {ch, vict})
        {
            if (FIGHTING(c))
            {
                stop_fighting(c);
            }
        }
        return (false);
    }
}

/* This updates the malfunctioning of certain objects that are damaged. */
void broken_update(uint64_t heartPulse, double deltaTime)
{
    Object *k, *money;

    int rand_gravity[14] = {0, 10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 5000, 10000};
    int dice = Random::get<int>(2, 12), grav_roll = 0, grav_change = false, health = 0;

    // Gravity generators
    auto gens = objectSubscriptions.all("vnum_11");
    for (auto k : filter_raw(gens))
    {
        if (k->getCarriedBy())
        {
            continue;
        }

        if (Random::get<int>(1, 2) == 2)
        {
            continue;
        }

        health = GET_OBJ_VAL(k, VAL_ALL_HEALTH); // Indicated the health of the object in question

        grav_roll = Random::get<int>(0, 13);
        if (health <= 10)
        {
            grav_change = true;
        }
        else if (health <= 40 && dice <= 8)
        {
            grav_change = true;
        }
        else if (health <= 80 && dice <= 5)
        {
            grav_change = true;
        }
        else if (health <= 99 && dice <= 3)
        {
            grav_change = true;
        }
        if (grav_change == true)
        {
            k->setBaseStat("gravity", rand_gravity[grav_roll]);
            k->setBaseStat<weight_t>("weight", rand_gravity[grav_roll]);
            k->location.sendText("@RThe gravity generator malfunctions! The gravity level has changed!@n\r\n");
        }
        dice = Random::get<int>(2, 12); // Reset the dice
    }

    // ATMS
    auto atms = objectSubscriptions.all("vnum_3034");
    for (auto k : filter_raw(atms))
    {
        if (k->getCarriedBy())
        {
            continue;
        }

        if (Random::get<int>(1, 2) == 2)
        {
            continue;
        }

        health = GET_OBJ_VAL(k, VAL_ALL_HEALTH); // Indicated the health of the object in question

        dice = Random::get<int>(2, 12); // Reset the dice
        if (health <= 10)
        {
            k->location.sendText("@RThe ATM machine shoots smoking bills from its money slot. The bills burn up as they float through the air!@n\r\n");
        }
        else if (health <= 40 && dice <= 8)
        {
            k->location.sendText("@RGibberish flashes across the cracked ATM info screen.@n\r\n");
        }
        else if (health <= 80 && dice == 4)
        {
            k->location.sendText("@GThe damaged ATM spits out some money while flashing ERROR on its screen!@n\r\n");
            money = create_money(Random::get<int>(1, 30));
            money->moveToLocation(k->location);
        }
        else if (health <= 99 && dice < 4)
        {
            k->location.sendText("@RThe ATM machine emits a loud grinding sound from inside.@n\r\n");
        }

        dice = Random::get<int>(2, 12); // Reset the dice
    } /* End For */
}

bool wearable_obj(Object *obj)
{
    for (auto i = 1; i < NUM_ITEM_WEARS; i++)
    {
        if (CAN_WEAR(obj, i))
        {
            return true;
        }
    }
    return false;
}

void randomize_eq(Object *obj)
{
    if (!wearable_obj(obj) || OBJ_FLAGGED(obj, ITEM_NORANDOM))
        return;

    for (auto &aff : obj->affected)
    {
        if (aff.location != APPLY_CATTR_BASE)
            continue;
        int value = aff.modifier;
        int roll = Random::get<int>(2, 12);
        if (roll == 12)
        {
            value += 3;
        }
        else if (roll >= 9)
        {
            value += 2;
        }
        else if (roll >= 6)
        {
            value += 1;
        }
        else if (roll == 3)
        {
            value -= 1;
        }
        else if (roll <= 2)
        {
            value -= 2;
        }
        if (obj->getBaseStat("level") >= 80)
        {
            if (value <= 0)
            {
                value = 1;
            }
        }
        else if (obj->getBaseStat("level") >= 60)
        {
            if (value < 0)
            {
                value = 0;
            }
        }
        if (value == 0)
        {
            aff.location = 0;
            aff.specific = 0;
            aff.modifier = 0;
        }
        else
        {
            aff.modifier = value;
        }
    }

    int dice = Random::get<int>(2, 12);
    if (dice >= 10)
    {
        obj->item_flags.set(ITEM_SLOT2, true);
    }
    else if (dice >= 7)
    {
        obj->item_flags.set(ITEM_SLOT1, true);
    }
}

const char *sense_location_name(room_vnum roomnum)
{
    auto r = get_room(roomnum);
    auto num = r->getZone()->number;

    switch (num)
    {
    case 2:
        return "East of Nexus City";
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        if (roomnum < 795)
            return "Nexus City";
        else
            return "South Ocean";
    case 8:
    case 9:
    case 10:
    case 11:
        if (roomnum < 1133)
            return "South Ocean";
        else if (roomnum < 1179)
            return "Nexus Field";
        else
            return "Cherry Blossom Mountain";
        break;
    case 12:
    case 13:
        if (roomnum < 1287)
            return "Cherry Blossom Mountain";
        else
            return "Sandy Desert";
        break;
    case 14:
        if (roomnum < 1428)
            return "Sandy Desert";
        else if (roomnum < 1484)
            return "Northern Plains";
        else if (roomnum < 1496)
            return "Korin's Tower";
        else
            return "Kami's Lookout";
        break;
    case 15:
        if (roomnum < 1577)
            return "Kami's Lookout";
        else if (roomnum < 1580)
            return "Northern Plains";
        else if (roomnum < 1589)
            return "Kami's Lookout";
        else
            return "Shadow Forest";
        break;
    case 16:
        return "Shadow Forest";
        break;
    case 17:
    case 18:
        if (roomnum < 1715)
            return "Decrepit Area";
        else
            return "Inside Cherry Blossom Mountain";
        break;
    case 19:
        return "West City";
        break;
    case 20:
        if (roomnum < 2012)
            return "West City";
        else if (roomnum > 2070)
            return "West City";
        else
            return "Silver Mine";
        break;
    case 21:
        if (roomnum < 2141)
            return "West City";
        else
            return "Hercule Beach";
        break;
    case 22:
        return "Vegetos City";
        break;
    case 23:
    case 24:
        if (roomnum < 2334)
            return "Vegetos City";
        else if (roomnum > 2462)
            return "Vegetos City";
        else
            return "Vegetos Palace";
        break;
    case 25:
    case 26:
        if (roomnum < 2616)
            return "Blood Dunes";
        else
            return "Ancestral Mountains";
        break;
    case 27:
        if (roomnum < 2709)
            return "Ancestral Mountains";
        else if (roomnum < 2736)
            return "Destopa Swamp";
        else
            return "Swamp Base";
        break;
    case 28:
        return "Pride Forest";
        break;
    case 29:
    case 30:
    case 31:
        return "Pride Tower";
        break;
    case 32:
        return "Ruby Cave";
        break;
    case 34:
        return "Utatlan City";
        break;
    case 35:
        return "Zenith Jungle";
        break;
    case 40:
    case 41:
    case 42:
        return "Ice Crown City";
        break;
    case 43:
        if (roomnum < 4351)
            return "Ice Highway";
        else
            return "Topica Snowfield";
        break;
    case 44:
    case 45:
        return "Glug's Volcano";
        break;
    case 46:
    case 47:
        return "Platonic Sea";
        break;
    case 48:
        return "Slave City";
        break;
    case 49:
        if (roomnum < 4915)
            return "Descent Down Icecrown";
        else if (roomnum != 4915 && roomnum < 4994)
            return "Topica Snowfield";
        else
            return "Ice Highway";
        break;
    case 50:
        return "Mirror Shard Maze";
        break;
    case 51:
        if (roomnum < 5150)
            return "Acturian Woods";
        else if (roomnum < 5165)
            return "Desolate Demesne";
        else
            return "Chateau Ishran";
        break;
    case 52:
        return "Wyrm Spine Mountain";
        break;
    case 53:
    case 54:
        return "Aromina Hunting Preserve";
        break;
    case 55:
        return "Cloud Ruler Temple";
        break;
    case 56:
        return "Koltoan Mine";
        break;
    case 78:
        return "Orium Cave";
        break;
    case 79:
        return "Crystalline Forest";
        break;
    case 80:
    case 81:
    case 82:
        return "Tiranoc City";
        break;
    case 83:
        return "Great Oroist Temple";
        break;
    case 84:
        if (roomnum < 8447)
            return "Elsthuan Forest";
        else
            return "Mazori Farm";
        break;
    case 85:
        return "Dres";
        break;
    case 86:
        return "Colvian Farm";
        break;
    case 87:
        return "Saint Alucia";
        break;
    case 88:
        if (roomnum < 8847)
            return "Meridius Memorial";
        else
            return "Battlefields";
        break;
    case 89:
        if (roomnum < 8954)
            return "Desert of Illusion";
        else
            return "Plains of Confusion";
        break;
    case 90:
        return "Shadowlas Temple";
        break;
    case 92:
        return "Turlon Fair";
        break;
    case 97:
        return "Wetlands";
        break;
    case 98:
        if (roomnum < 9855)
            return "Wetlands";
        else if (roomnum < 9866)
            return "Kerberos";
        else
            return "Shaeras Mansion";
        break;
    case 99:
        if (roomnum < 9907)
            return "Slavinos Ravine";
        else if (roomnum < 9960)
            return "Kerberos";
        else
            return "Furian Citadel";
        break;
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
    case 111:
    case 112:
    case 113:
    case 114:
    case 115:
        return "Namekian Wilderness";
        break;
    case 116:
        if (roomnum < 11672)
            return "Senzu Village";
        else if (roomnum > 11672 && roomnum < 11698)
            return "Senzu Village";
        else
            return "Guru's House";
        break;
    case 117:
    case 118:
    case 119:
        return "Crystalline Cave";
        break;
    case 120:
        return "Haven City";
        break;
    case 121:
        if (roomnum < 12103)
            return "Haven City";
        else
            return "Serenity Lake";
        break;
    case 122:
        return "Serenity Lake";
        break;
    case 123:
        return "Kaiju Forest";
        break;
    case 124:
        if (roomnum < 12480)
            return "Ortusian Temple";
        else
            return "Silent Glade";
        break;
    case 125:
        return "Near Serenity Lake";
        break;
    case 130:
    case 131:
        if (roomnum < 13153)
            return "Satan City";
        else if (roomnum == 13153)
            return "West City";
        else if (roomnum == 13154)
            return "Nexus City";
        else
            return "South Ocean";
        break;
    case 132:
        if (roomnum < 13232)
            return "Frieza's Ship";
        else
            return "Namekian Wilderness";
        break;
    case 133:
        return "Elder Village";
        break;
    case 134:
        return "Satan City";
        break;
    case 140:
        return "Yardra City";
        break;
    case 141:
        return "Jade Forest";
        break;
    case 142:
        return "Jade Cliff";
        break;
    case 143:
        return "Mount Valaria";
        break;
    case 149:
    case 150:
        return "Aquis City";
        break;
    case 151:
    case 152:
    case 153:
        return "Kanassan Ocean";
        break;
    case 154:
        return "Kakureta Village";
        break;
    case 155:
        return "Captured Aether City";
        break;
    case 156:
        return "Yunkai Pirate Base";
        break;
    case 160:
    case 161:
        return "Janacre";
        break;
    case 165:
        return "Arlian Wasteland";
        break;
    case 166:
        return "Arlian Mine";
        break;
    case 167:
        return "Kilnak Caverns";
        break;
    case 168:
        return "Kemabra Wastes";
        break;
    case 169:
        return "Dark of Arlia";
        break;
    case 174:
        return "Fistarl Volcano";
        break;
    case 175:
    case 176:
        return "Cerria Colony";
        break;
    case 182:
        return "Below Tiranoc";
        break;
    case 196:
        return "Ancient Castle";
        break;
    default:
        return "Unknown.";
        break;
    }
}

const char *sense_location(Character *ch)
{

    return sense_location_name(ch->location.getVnum());
}

void reveal_hiding(Character *ch, int type)
{
    if (IS_NPC(ch) || !AFF_FLAGGED(ch, AFF_HIDE))
        return;

    int rand1 = Random::get<int>(-5, 5), rand2 = Random::get<int>(-5, 5), bonus = 0;

    if (AFF_FLAGGED(ch, AFF_LIQUEFIED))
    {
        bonus = 10;
    }

    if (type == 0)
    { /* Automatic reveal. */
        act("@MYou feel as though what you just did may have revealed your hiding spot...@n", true, ch, nullptr,
            nullptr, TO_CHAR);
        act("@M$n moves a little and you notice them!@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->affect_flags.set(AFF_HIDE, false);
    }
    else if (type == 1)
    { /* Their skill at hiding failed, reveal */
        if (GET_SKILL(ch, SKILL_HIDE) + bonus < axion_dice(0))
        {
            act("@MYou feel as though what you just did may have revealed your hiding spot...@n", true, ch, nullptr,
                nullptr, TO_CHAR);
            act("@M$n moves a little and you notice them!@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->affect_flags.set(AFF_HIDE, false);
        }
    }
    else if (type == 2)
    { /* They were spotted */
        struct descriptor_data *d;
        Character *tch = nullptr;
        for (d = descriptor_list; d; d = d->next)
        {
            if (STATE(d) != CON_PLAYING)
                continue;
            tch = d->character;

            if (tch == ch)
                continue;

            if (tch->location != ch->location)
                continue;

            if (GET_SKILL(tch, SKILL_SPOT) + rand1 >= GET_SKILL(ch, SKILL_HIDE) + rand2)
            {
                ch->affect_flags.set(AFF_HIDE, false);
                act("@M$N seems to notice you!@n", true, ch, nullptr, tch, TO_CHAR);
                act("@MYou notice $n's movements reveal $s hiding spot!@n", true, ch, nullptr, tch, TO_VICT);
                act("@MYou notice $N look keenly somewhere nearby. At that spot you see $n hiding!@n", true, ch,
                    nullptr, tch, TO_NOTVICT);
                return;
            }
        }
    }
    else if (type == 3)
    { /* They were heard, reveal with different messages. */
        struct descriptor_data *d;
        Character *tch = nullptr;

        act("@MThe scouter makes some beeping sounds as you tinker with its buttons.@n", true, ch, nullptr, nullptr,
            TO_CHAR);
        for (d = descriptor_list; d; d = d->next)
        {
            if (STATE(d) != CON_PLAYING)
                continue;
            tch = d->character;

            if (tch == ch)
                continue;

            if (tch->location != ch->location)
                continue;

            if (GET_SKILL(tch, SKILL_LISTEN) > axion_dice(0))
            {
                switch (type)
                {
                case 3:
                    act("@MYou notice some beeping sounds that sound really close by.@n", true, ch, nullptr, tch,
                        TO_VICT);
                    break;
                default:
                    act("@MYou notice some sounds coming from this room but can't seem to locate the source...@n",
                        true, ch, nullptr, tch, TO_VICT);
                    break;
                }
            }
        }
    }
    else if (type == 4)
    { /* You were found from search! */
        ch->affect_flags.set(AFF_HIDE, false);
    }
}

int block_calc(Character *ch)
{
    Character *blocker = nullptr;

    if (BLOCKED(ch))
    {
        blocker = BLOCKED(ch);
    }
    else
    {
        return (1);
    }

    if (GET_SPEEDI(ch) < GET_SPEEDI(blocker) && GET_POS(blocker) > POS_SITTING)
    {
        if (!AFF_FLAGGED(blocker, AFF_BLIND) && !PLR_FLAGGED(blocker, PLR_EYEC))
        {
            int minimum = GET_CHA(blocker) + Random::get<int>(5, 20);
            if (minimum > 100)
            {
                minimum = 100;
            }
            if (!GET_SKILL(ch, SKILL_ESCAPE_ARTIST) || (GET_SKILL(ch, SKILL_ESCAPE_ARTIST) &&
                                                        GET_SKILL(ch, SKILL_ESCAPE_ARTIST) <
                                                            Random::get<int>(minimum, 120)))
            {
                act("$n tries to leave, but can't outrun $N!", true, ch, nullptr, blocker, TO_NOTVICT);
                act("$n tries to leave, but can't outrun you!", true, ch, nullptr, blocker, TO_VICT);
                act("You try to leave, but can't outrun $N!", true, ch, nullptr, blocker, TO_CHAR);
                if (AFF_FLAGGED(ch, AFF_FLYING) && !AFF_FLAGGED(blocker, AFF_FLYING) && GET_ALT(ch) == 1)
                {
                    blocker->sendText("You're now floating in the air.\r\n");

                    blocker->affect_flags.set(AFF_FLYING, true);
                    blocker->setBaseStat<int>("altitude", GET_ALT(ch));
                }
                else if (AFF_FLAGGED(ch, AFF_FLYING) && !AFF_FLAGGED(blocker, AFF_FLYING) && GET_ALT(ch) == 2)
                {
                    blocker->sendText("You're now floating high in the sky.\r\n");
                    blocker->affect_flags.set(AFF_FLYING, true);
                    blocker->setBaseStat<int>("altitude", GET_ALT(ch));
                }
                return (0);
            }
            else
            {
                act("$n proves $s great skill and escapes from $N's attempted block!", true, ch, nullptr, blocker,
                    TO_NOTVICT);
                act("$n proves $s great skill and escapes from your attempted block!", true, ch, nullptr, blocker,
                    TO_VICT);
                act("Using your great skill you manage to escape from $N's attempted block!", true, ch, nullptr,
                    blocker, TO_CHAR);
                BLOCKED(ch) = nullptr;
                BLOCKS(blocker) = nullptr;
            }
        }
        else
        {
            act("$n proves $s great skill and escapes from $N's attempted block!", true, ch, nullptr, blocker,
                TO_NOTVICT);
            act("$n proves $s great skill and escapes from your attempted block!", true, ch, nullptr, blocker, TO_VICT);
            act("Using your great skill you manage to escape from $N's attempted block!", true, ch, nullptr, blocker,
                TO_CHAR);
            BLOCKED(ch) = nullptr;
            BLOCKS(blocker) = nullptr;
        }
    }
    else if (GET_POS(blocker) <= POS_SITTING)
    {
        act("$n proves $s great skill and escapes from $N!", true, ch, nullptr, blocker, TO_NOTVICT);
        act("$n proves $s great skill and escapes from you!", true, ch, nullptr, blocker, TO_VICT);
        act("Using your great skill you manage to escape from $N!", true, ch, nullptr, blocker, TO_CHAR);
        BLOCKED(ch) = nullptr;
        BLOCKS(blocker) = nullptr;
    }
    else if (GET_POS(blocker) > POS_SITTING)
    {
        act("$n proves $s great skill and escapes from $N's attempted block!", true, ch, nullptr, blocker, TO_NOTVICT);
        act("$n proves $s great skill and escapes from your attempted block!", true, ch, nullptr, blocker, TO_VICT);
        act("Using your great skill you manage to escape from $N's attempted block!", true, ch, nullptr, blocker,
            TO_CHAR);
        BLOCKED(ch) = nullptr;
        BLOCKS(blocker) = nullptr;
    }

    return (1);
}

int64_t molt_threshold(Character *ch)
{

    int64_t threshold = 0, max = 2000000000;

    max *= 250;

    if (!IS_ARLIAN(ch))
        return (0);
    else if (GET_MOLT_LEVEL(ch) < 100)
    {
        threshold = (((GET_MOLT_LEVEL(ch) + 1) * (ch->getBaseStat<int64_t>("health") * 0.02)) * GET_CON(ch)) / 4;
        threshold = threshold * 0.25;
    }
    else if (GET_MOLT_LEVEL(ch) < 200)
    {
        threshold = (((GET_MOLT_LEVEL(ch) + 1) * (ch->getBaseStat<int64_t>("health") * 0.02)) * GET_CON(ch)) / 2;
        threshold = threshold * 0.20;
    }
    else if (GET_MOLT_LEVEL(ch) < 400)
    {
        threshold = (((GET_MOLT_LEVEL(ch) + 1) * (ch->getBaseStat<int64_t>("health") * 0.02)) * GET_CON(ch));
        threshold = threshold * 0.17;
    }
    else if (GET_MOLT_LEVEL(ch) < 800)
    {
        threshold = (((GET_MOLT_LEVEL(ch) + 1) * (ch->getBaseStat<int64_t>("health") * 0.02)) * GET_CON(ch)) * 2;
        threshold = threshold * 0.15;
    }
    else
    {
        threshold = (((GET_MOLT_LEVEL(ch) + 1) * (ch->getBaseStat<int64_t>("health") * 0.02)) * GET_CON(ch)) * 4;
        threshold = threshold * 0.12;
    }

    return std::min(threshold, max);
}

int armor_evolve(Character *ch)
{
    int value = 0;

    if (GET_MOLT_LEVEL(ch) <= 5)
    {
        value = 8;
    }
    else if (GET_MOLT_LEVEL(ch) <= 10)
    {
        value = 12;
    }
    else if (GET_MOLT_LEVEL(ch) <= 20)
    {
        value = 15;
    }
    else if (GET_MOLT_LEVEL(ch) <= 30)
    {
        value = 20;
    }
    else if (GET_MOLT_LEVEL(ch) <= 40)
    {
        value = 30;
    }
    else if (GET_MOLT_LEVEL(ch) <= 50)
    {
        value = 50;
    }
    else if (GET_MOLT_LEVEL(ch) <= 75)
    {
        value = 100;
    }
    else if (GET_MOLT_LEVEL(ch) <= 100)
    {
        value = 150;
    }
    else if (GET_MOLT_LEVEL(ch) <= 500)
    {
        value = 200;
    }
    else
    {
        value = 220;
    }

    return (value);
}

/* This handles arlian exoskeleton molting */
void handle_evolution(Character *ch, int64_t dmg)
{

    /* Reject NPCs and non-arlians */
    if (IS_NPC(ch) || !IS_ARLIAN(ch))
    {
        return;
    }

    int64_t moltgain = 0;
    auto chCon = GET_CON(ch);
    moltgain = dmg * 0.5;
    if (chCon == 100)
        moltgain += 100000;
    else if (chCon >= 90)
        moltgain += chCon * 1000;
    else if (chCon >= 75)
        moltgain += chCon * 500;
    else if (chCon >= 50)
        moltgain += chCon * 250;
    else if (chCon >= 10)
        moltgain += chCon * 50;

    ch->modBaseStat<int64_t>("molt_experience", moltgain);

    if (AFF_FLAGGED(ch, AFF_SPIRIT))
    {
        ch->sendText("You are dead and all evolution experience is reduced. Gains are divided by 100 or reduced to a minimum of 1.\r\n");
        moltgain /= 100;
    }

    if (GET_MOLT_EXP(ch) > molt_threshold(ch))
    {
        if (GET_MOLT_LEVEL(ch) <= chCon * 2 || chCon >= 100)
        {
            ch->setBaseStat<int64_t>("molt_experience", 0);
            ch->modBaseStat<int>("molt_level", 1);
            int armorgain = armor_evolve(ch);

            if (ch->getBaseStat("armor_innate") + armorgain < 500000)
                ch->modBaseStat("armor_innate", armorgain);

            double baseHl = ch->getBaseStat<int64_t>("health");
            double baseSt = ch->getBaseStat<int64_t>("stamina");
            double attrBonus = (1 + (GET_CON(ch) / 20));

            int64_t bonusHl = 0;
            int64_t bonusSt = 0;
            double start_bonusHl = Random::get<double>(0.8, 1.2) * attrBonus * ch->getPotential();
            double start_bonusSt = Random::get<double>(0.8, 1.2) * attrBonus * ch->getPotential();
            double soft_cap = (double)ch->calc_soft_cap();
            double diminishing_returnsHl = (soft_cap - baseHl) / soft_cap;
            double diminishing_returnsSt = (soft_cap - baseSt) / soft_cap;
            if (diminishing_returnsHl > 0.0)
                diminishing_returnsHl = std::max<double>(diminishing_returnsHl, 0.05);
            else
                diminishing_returnsHl = 0;

            if (diminishing_returnsSt > 0.0)
                diminishing_returnsSt = std::max<double>(diminishing_returnsSt, 0.05);
            else
                diminishing_returnsSt = 0;

            bonusHl = start_bonusHl * diminishing_returnsHl * 16;
            bonusSt = start_bonusSt * diminishing_returnsSt * 16;

            if (bonusHl > (ch->getBaseStat<int64_t>("health") / 10))
                bonusHl = ch->getBaseStat<int64_t>("health") / 10;
            if (bonusSt > (ch->getBaseStat<int64_t>("stamina") / 10))
                bonusSt = ch->getBaseStat<int64_t>("stamina") / 10;

            bonusHl *= (1 + ch->getAffectModifier(APPLY_CVIT_GAIN_MULT, static_cast<int>(CharVital::health)));
            bonusSt *= (1 + ch->getAffectModifier(APPLY_CVIT_GAIN_MULT, static_cast<int>(CharVital::stamina)));

            act("@gYour @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn@g begins to crack. You quickly shed it and reveal a stronger version that was growing beneath it! At the same time you feel your adrenal sacs to be more efficient@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@G$n's@g @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn@g begins to crack. Suddenly $e sheds the damaged @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn and reveals a stronger version that had been growing underneath!@n",
                true, ch, nullptr, nullptr, TO_ROOM);
            ch->send_to("@D[@RHL@W: @G+%s@D] [@gStamina@W: @G+%s@D] [@wArmor Index@W: @G+%s@D]@n\r\n", add_commas(bonusHl).c_str(), add_commas(bonusSt).c_str(), GET_ARMOR(ch) >= 500000 ? "500k CAP" : add_commas(armorgain).c_str());
        }
        else
        {
            ch->sendText("@gYou are unable to evolve while your evolution level is higher than twice your character Constitution.@n\r\n");
        }
    }
}

void demon_refill_lf(Character *ch, int64_t num)
{
    auto pe = ch->location.getPeople();
    for (auto tch : filter_raw(pe))
    {
        if (!IS_DEMON(tch))
            continue;
        if ((tch->getCurVital(CharVital::lifeforce)) >= (tch->getEffectiveStat("lifeforce")))
            continue;
        tch->modCurVital(CharVital::lifeforce, num);
        act("@CYou feel the life energy from @c$N@C's cursed body flow out and you draw it into yourself!@n", true,
            tch, nullptr, ch, TO_CHAR);
    }
}

void mob_talk(Character *ch, const char *speech)
{

    int stop = 1;

    if (IS_NPC(ch))
    {
        return;
    }
    auto pe = ch->location.getPeople();
    for (auto tch : filter_raw(pe))
    {
        if (!IS_NPC(tch))
            continue;
        if (!IS_HUMANOID(tch))
            continue;
        if (stop == 0)
            continue;
        else
        {
            auto vict = tch;
            stop = mob_respond(ch, vict, speech);
            if (Random::get<int>(1, 2) == 2)
            {
                stop = 0;
            }
        }
    } /* End for loop */
} /* End Mob Talk */

bool spar_friendly(Character *ch, Character *npc)
{
    if (!IS_NPC(npc))
        return false;

    if (!IS_HUMANOID(npc))
        return false;

    if (npc->original == ch)
    {
        return true;
    }

    auto shared = ch->shared_from_this();
    auto find = std::find_if(npc->agg_memory.begin(), npc->agg_memory.end(), [shared](const auto &mem)
                             { return mem.lock() == shared; });

    if (find != npc->agg_memory.end())
        return false;

    for (auto f : {MOB_AGGRESSIVE, MOB_DUMMY})
        if (npc->mob_flags.get(f))
            return false;

    return true;
}

int mob_respond(Character *ch, Character *vict, const char *speech)
{
    if (ch && vict)
    {
        if (!IS_NPC(ch) && IS_NPC(vict))
        {
            if ((strstr(speech, "hello") || strstr(speech, "greet") || strstr(speech, "Hello") ||
                 strstr(speech, "Greet")) &&
                !FIGHTING(vict))
            {
                vict->location.sendText("\r\n");
                if (IS_HUMAN(vict) || IS_HALFBREED(vict))
                {
                    switch (Random::get<int>(1, 4))
                    {
                    case 1:
                        act("@w$n@W says, '@CYes, hello to you as well $N.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 2:
                        act("@w$n@W says, '@CHello!@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 3:
                        act("@w$n@W says, '@CHi, $N, how are you doing?@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 4:
                        act("@w$n@W says, '@CGreetings, $N. What are you up to?@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                        break;
                    }
                } /* End Human Section */
                else if (IS_SAIYAN(vict))
                {
                    switch (Random::get<int>(1, 4))
                    {
                    case 1:
                        act("@w$n@W says, '@CHmph, hi.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 2:
                        act("@w$n@W says, '@CHello weakling.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 3:
                        act("@w$n@W says, '@C$N do all weaklings like you waste time in idle talk?@W'@n", true,
                            vict, nullptr, ch, TO_ROOM);
                        break;
                    case 4:
                        act("@w$n@W says, '@C$N, you are not welcome around me.@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                        break;
                    }
                } /* End Saiyan Section */
                else if (IS_ICER(vict))
                {
                    switch (Random::get<int>(1, 4))
                    {
                    case 1:
                        act("@w$n@W says, '@CHa ha... Yes, hello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 2:
                        act("@w$n@W says, '@CAh a polite greeting. It's good to know your kind isn't totally worthless.@W'@n",
                            true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 3:
                        act("@w$n@W says, '@C$N, hello. Now leave me be.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 4:
                        act("@w$n@W says, '@C$N, you are below me. Now begone.@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                        break;
                    }
                } /* End Icer Section */
                else if (IS_KONATSU(vict))
                {
                    switch (Random::get<int>(1, 4))
                    {
                    case 1:
                        act("@w$n@W says, '@CGreetings, $N, may your travels be well.@W'@n", true, vict, nullptr,
                            ch, TO_ROOM);
                        break;
                    case 2:
                        act("@w$n@W says, '@CHello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 3:
                        act("@w$n@W says, '@C$N, hello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 4:
                        act("@w$n@W says, '@C$N, it is nice to meet you.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    }
                } /* End Konatsu Section */
                else if (IS_NAMEK(vict))
                {
                    switch (Random::get<int>(1, 4))
                    {
                    case 1:
                        act("@w$n@W says, '@CHello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 2:
                        act("@w$n@W says, '@CA peaceful greeting to you, $N.@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                        break;
                    case 3:
                        act("@w$n@W says, '@C$N, hello. What is your business here?@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                        break;
                    case 4:
                        act("@w$n@W says, '@C$N, greetings.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    }
                } /* End Namek Section */
                else if (IS_ARLIAN(vict))
                {
                    switch (Random::get<int>(1, 4))
                    {
                    case 1:
                        act("@w$n@W says, '@CPeace, stranger.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 2:
                        act("@w$n@W says, '@CStay out of my way.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 3:
                        act("@w$n@W says, '@C$N, what is your business here?@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                        break;
                    case 4:
                        act("@w$n@W says, '@C...Hello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    }
                } /* End Arlian Section */
                else if (IS_ANDROID(vict))
                {
                    act("@w$n@W says, '@C...@W'@n", true, vict, nullptr, ch, TO_ROOM);
                } /* End Android Section */
                else if (IS_MAJIN(vict))
                {
                    switch (Random::get<int>(1, 2))
                    {
                    case 1:
                        act("@w$n@W says, '@CHa ha...@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        break;
                    case 2:
                        act("@w$n@W says, '@CHello. What candy you want to be?@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                        break;
                    }
                } /* End MAJIN Section */
                else if (IS_TRUFFLE(vict))
                {
                    switch (Random::get<int>(1, 3))
                    {
                    case 1:
                        if (IS_SAIYAN(ch))
                        {
                            act("@w$n@W says, '@CEwww, dirty monkey...@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                        else
                        {
                            act("@w$n@W says, '@CHello.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                        break;
                    case 2:
                        if (IS_SAIYAN(ch))
                        {
                            act("@w$n@W says, '@CEwww, dirty monkey...@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                        else
                        {
                            act("@w$n@W says, '@C$N, hello. You are a curious individual.@W'@n", true, vict,
                                nullptr, ch, TO_ROOM);
                        }
                        break;
                    case 3:
                        if (IS_SAIYAN(ch))
                        {
                            act("@w$n@W says, '@CEwww, dirty monkey...@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                        else
                        {
                            act("@w$n@W says, '@C$N, hello. What's your IQ?@W'@n", true, vict, nullptr, ch,
                                TO_ROOM);
                        }
                        break;
                    }
                } /* End Truffle Section */
                else
                {
                    act("Hmph, yeah hi.", true, vict, nullptr, ch, TO_ROOM);
                }
            } /* End Hello Section */

            if (strstr(speech, "goodbye") || strstr(speech, "Goodbye") || strstr(speech, "bye") ||
                strstr(speech, "Bye"))
            {
                vict->location.sendText("\r\n");
                if (GET_ALIGNMENT(vict) >= 0)
                {
                    if (GET_SEX(vict) == SEX_MALE)
                    {
                        if (GET_SEX(ch) == SEX_FEMALE)
                        {
                            act("@w$n@W says, '@C$N, goodbye babe.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                        else
                        {
                            act("@w$n@W says, '@C$N, goodbye.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                    }
                    else if (GET_SEX(vict) == SEX_FEMALE)
                    {
                        if (GET_SEX(ch) == SEX_MALE)
                        {
                            act("@w$n@W says, '@C$N, goodbye...@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                        else
                        {
                            act("@w$n@W says, '@C$N, bye.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                    }
                    else
                    {
                        act("@w$n@W says, '@C$N, goodbye.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                    }
                }
                if (GET_ALIGNMENT(vict) < 0)
                {
                    if (GET_SEX(vict) == SEX_MALE)
                    {
                        if (GET_SEX(ch) == SEX_FEMALE)
                        {
                            act("@w$n@W says, '@CGoodbye. Eh heh heh.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                        else
                        {
                            act("@w$n@W says, '@CSo long and good ridance.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                    }
                    else if (GET_SEX(vict) == SEX_FEMALE)
                    {
                        if (GET_SEX(ch) == SEX_MALE)
                        {
                            act("@w$n@W says, '@CGoodbye then...@W'@n", true, vict, nullptr, ch, TO_ROOM);
                        }
                        else
                        {
                            act("@w$n@W says, '@C$N, no one wanted you around anyway.@W'@n", true, vict, nullptr, ch,
                                TO_ROOM);
                        }
                    }
                    else
                    {
                        act("@w$n@W says, '@CFine get lost.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                    }
                }
            } /* End goodbye If */
            if (strstr(speech, "train") || strstr(speech, "Train") || strstr(speech, "exercise") ||
                strstr(speech, "Exercise"))
            {
                vict->location.sendText("\r\n");
                if (GET_ALIGNMENT(vict) >= 0 && !MOB_FLAGGED(vict, MOB_NOKILL))
                {
                    if (GET_LEVEL(vict) > 4 && GET_LEVEL(vict) < 10)
                    {
                        act("@w$n@W says, '@CTraining is good for the body. I think I may need to go workout myself.@W'@n",
                            true, vict, nullptr, ch, TO_ROOM);
                    }
                    if (GET_LEVEL(vict) >= 10 && GET_LEVEL(vict) < 30)
                    {
                        act("@w$n@W says, '@CI think I might need a little more training...@W'@n", true, vict, nullptr,
                            ch, TO_ROOM);
                    }
                    if (GET_LEVEL(vict) >= 30 && GET_LEVEL(vict) < 60)
                    {
                        act("@w$n@W says, '@CI'm pretty tough already. Though I should probably work on my skills.@W'@n",
                            true, vict, nullptr, ch, TO_ROOM);
                    }
                    if (GET_LEVEL(vict) >= 60)
                    {
                        act("@w$n@W says, '@CI'm on top of my game.@W'@n", true, vict, nullptr, ch, TO_ROOM);
                    }
                    if (GET_LEVEL(vict) < 5)
                    {
                        act("@w$n@W says, '@CI really need to bust ass and train.@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                    }
                }
                if (GET_ALIGNMENT(vict) < 0 && !MOB_FLAGGED(vict, MOB_NOKILL))
                {
                    if (GET_LEVEL(vict) > 4 && GET_LEVEL(vict) < 10)
                    {
                        act("@w$n@W says, '@CWell maybe I could use some more training.@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                    }
                    if (GET_LEVEL(vict) >= 10 && GET_LEVEL(vict) < 30)
                    {
                        act("@w$n@W says, '@CTrain? Yeah it has become harder to take what I want....@W'@n", true, vict,
                            nullptr, ch, TO_ROOM);
                    }
                    if (GET_LEVEL(vict) >= 30 && GET_LEVEL(vict) < 60)
                    {
                        act("@w$n@W says, '@CTrain? I don't need to train to take you!@W'@n", true, vict, nullptr, ch,
                            TO_ROOM);
                    }
                    if (GET_LEVEL(vict) >= 60)
                    {
                        act("@w$n@W says, '@CTraining won't save you when I tire of your continued life.@W'@n", true,
                            vict, nullptr, ch, TO_ROOM);
                    }
                    if (GET_LEVEL(vict) < 5)
                    {
                        act("@w$n@W says, '@CYes. I need to train so I can reach the top. Then everyone will have to listen to me!@W'@n",
                            true, vict, nullptr, ch, TO_ROOM);
                    }
                }
            }
            return 1;
        }
    } /* End Valid targets Loop. */
    return 1;
}

char *introd_calc(Character *ch)
{
    static char intro[100];

    *intro = '\0';

    if (IS_NPC(ch))
    { /* How the hell... */
        return ("IAMERROR");
    }

    auto race = strdup(ch->getAppearanceStr(Appearance::seeming));
    auto sex = strdup(MAFE(ch));

    sprintf(intro, "%s %s %s", AN(sex), sex, race);
    if (sex)
    {
        free(sex);
    }
    if (race)
    {
        free(race);
    }

    return (intro);
}

int64_t gear_exp(Character *ch, int64_t exp)
{

    if (IS_NPC(ch))
    {
        return exp;
    }

    auto ratio = ch->getBaseStat("burden_ratio");
    exp += exp * ratio;
    return exp;
}

int planet_check(Character *ch, Character *vict)
{
    if (!ch)
    {
        basic_mud_log("ERROR: planet_check called without ch!");
        return false;
    }
    else if (!vict)
    {
        basic_mud_log("ERROR: planet_check called without vict!");
        return false;
    }

    return ch->location.getZone()->getRoot() == vict->location.getZone()->getRoot();
}

void purge_homing(Character *ch)
{

    auto isHoming = [&](const auto &o)
    { return (o->getVnum() == 80 || o->getVnum() == 81) && (TARGET(o) == ch || USER(o) == ch); };
    auto gather = ch->location.gatherFromObjects(isHoming);
    for (auto obj : gather)
    {
        act("$p @wloses its target and flies off into the distance.@n", true, nullptr, obj, nullptr, TO_ROOM);
        extract_obj(obj);
    }
}

static std::unordered_set<uint16_t> masoSkills = {
    SKILL_PARRY,
    SKILL_DODGE,
    SKILL_BARRIER,
    SKILL_BLOCK,
    SKILL_ZANZOKEN,
    SKILL_TSKIN};

void improve_skill(Character *ch, int skill, int num)
{
    if (IS_NPC(ch))
        return;

    if (AFF_FLAGGED(ch, AFF_SHOCKED))
        return;

    if (GET_FORGETING(ch) == skill)
        return;

    int percent = GET_SKILL(ch, skill);
    int newpercent, roll = 1200;
    char skillbuf[MAX_STRING_LENGTH];

    if (!num)
    {
        num = 2;
    }

    auto base = GET_SKILL_BASE(ch, skill);

    if (base >= 90)
    {
        roll += 800;
    }
    else if (base >= 75)
    {
        roll += 600;
    }
    else if (base >= 50)
    {
        roll += 400;
    }

    if (GET_BONUS(ch, BONUS_MASOCHISTIC) > 0 && masoSkills.contains(skill))
    {
        return;
    }

    if (!SPAR_TRAIN(ch))
    {
        if (num == 0)
        {
            roll -= 400;
        }
        else if (num == 1)
        {
            roll -= 200;
        }
    }
    else
    {
        if (num == 0)
        {
            roll -= 500;
        }
        else if (num == 1)
        {
            roll -= 400;
        }
        else
        {
            roll -= 200;
        }
    }

    if (FIGHTING(ch) && IS_NPC(FIGHTING(ch)) && MOB_FLAGGED(FIGHTING(ch), MOB_DUMMY))
    {
        roll -= 100;
    }

    if (IS_TRUFFLE(ch) || ch->bio_genomes.get(Race::tuffle))
    {
        roll *= 0.5;
    }
    else if (IS_MAJIN(ch))
    {
        roll += roll * .3;
    }
    else if (IS_BIO(ch) && skill == SKILL_ABSORB)
    {
        roll -= roll * .15;
    }
    else if (IS_HOSHIJIN(ch) &&
             (skill == SKILL_PUNCH || skill == SKILL_KICK || skill == SKILL_KNEE || skill == SKILL_ELBOW ||
              skill == SKILL_UPPERCUT || skill == SKILL_ROUNDHOUSE || skill == SKILL_SLAM ||
              skill == SKILL_HEELDROP || skill == SKILL_DAGGER || skill == SKILL_SWORD || skill == SKILL_CLUB ||
              skill == SKILL_GUN || skill == SKILL_SPEAR || skill == SKILL_BRAWL))
    {
        roll = roll * 0.30;
    }

    if (FIGHTING(ch) && IS_NPC(FIGHTING(ch)) && MOB_FLAGGED(FIGHTING(ch), MOB_DUMMY))
    {
        roll -= 100;
    }

    if (GET_BONUS(ch, BONUS_QUICK_STUDY) > 0)
    {
        roll -= roll * .25;
    }
    else if (GET_BONUS(ch, BONUS_SLOW_LEARNER) > 0)
    {
        roll += roll * .25;
    }

    if (auto asb = GET_ASB(ch); asb > 0)
    {
        roll -= (roll * 0.01) * asb;
    }

    roll = std::max(roll, 300);

    if (Random::get<int>(1, roll) > ((GET_INT(ch) * 2) + GET_WIS(ch)))
    {
        return;
    }
    if ((percent > 99 || percent <= 0))
    {
        return;
    }
    if (IS_MAJIN(ch) && GET_SKILL(ch, skill) >= 75)
    {
        switch (skill)
        {
        case 407:
        case 408:
        case 409:
        case 425:
        case 431:
        case 449:
        case 450:
        case 451:
        case 452:
        case 453:
        case 454:
        case 455:
        case 456:
        case 465:
        case 466:
        case 467:
        case 468:
        case 469:
        case 470:
        case 499:
        case 501:
        case 530:
        case 531:
        case 538:
            /* Do nothing. */
            break;
        default:
            return;
        }
    }
    else if (IS_MAJIN(ch) && skill == 425)
    {
        roll += 250;
    }

    /*
  if ((IS_JINTO(ch) || IS_TSUNA(ch) || IS_DABURA(ch) || IS_TAPION(ch) && GET_SKILL(ch, SKILL_SENSE) >= 75) && skill == SKILL_SENSE) {
    return;
  }

  if ((IS_BARDOCK(ch) || IS_KURZAK(ch) || IS_FRIEZA(ch) || IS_GINYU(ch) || IS_ANDSIX(ch) && GET_SKILL(ch, SKILL_SENSE) >= 50) && skill == SKILL_SENSE) {
    return;
  }
   */

    newpercent = 1;
    percent += newpercent;
    SET_SKILL(ch, skill, percent);
    if (newpercent >= 1)
    {
        sprintf(skillbuf, "@WYou feel you have learned something new about @G%s@W.@n\r\n", spell_info[skill].name);
        ch->sendText(skillbuf);
        if (GET_SKILL_BASE(ch, skill) >= 100)
        {
            ch->sendText("You learned a lot by mastering that skill.\r\n");
            if (perf_skill(skill))
            {
                ch->sendText("You can now choose a perfection for this skill (help perfection).\r\n");
            }
            if (IS_KONATSU(ch) && skill == SKILL_PARRY)
            {
                SET_SKILL(ch, skill, GET_SKILL_BASE(ch, skill) + 5);
            }
            if (GET_INT(ch) < 100)
            {
                ch->modExperience(level_exp(ch, GET_INT(ch) + 1) / 20);
            }
            else
            {
                ch->modExperience(5000000);
            }
        }
    }
}


/* Axion engine dice function */
int axion_dice(int adjust)
{

    int die1 = 0, die2 = 0, roll = 0;

    die1 = Random::get<int>(1, 60);
    die2 = Random::get<int>(1, 60);

    roll = (die1 + die2) + adjust;

    if (roll < 2)
        roll = 2;

    return (roll);
}

/* simulates dice roll */
int dice(int num, int size)
{
    int sum = 0;

    if (size <= 0 || num <= 0)
        return (0);

    while (num-- > 0)
        sum += Random::get<int>(1, size);

    return (sum);
}


char *CAP(char *txt)
{
    int i;
    for (i = 0; txt[i] != '\0' && (txt[i] == '@' && IS_COLOR_CHAR(txt[i + 1])); i += 2)
        ;

    txt[i] = toupper(txt[i]);
    return (txt);
}

char *strlwr(char *s)
{
    if (s)
    {
        char *p;

        for (p = s; *p; ++p)
            *p = tolower(*p);
    }
    return s;
}

/* New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.  */
void basic_mud_vlog(const char *format, va_list args)
{
    time_t ct = time(nullptr);
    char *time_s = asctime(localtime(&ct));

    // do a copy of the args to find out their total formatted byte size...
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (format == nullptr)
        format = "SYSERR: log() received a nullptr format.";

    char *buf = new char[size + 1];
    vsnprintf(buf, size + 1, format, args);
    basic_mud_log("%s :: %s", time_s, buf);
    delete[] buf;
}

/* the "touch" command, essentially. */
int touch(const char *path)
{
    FILE *fl;

    if (!(fl = fopen(path, "a")))
    {
        basic_mud_log("SYSERR: %s: %s", path, strerror(errno));
        return (-1);
    }
    else
    {
        fclose(fl);
        return (0);
    }
}

/* mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992 */
void mudlog(int type, int level, int file, const char *str, ...)
{
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *i;
    va_list args;

    if (str == nullptr)
        return; /* eh, oh well. */

    if (file)
    {
        va_start(args, str);
        basic_mud_vlog(str, args);
        va_end(args);
    }

    if (level < ADMLVL_IMMORT)
        level = ADMLVL_IMMORT;

    strcpy(buf, "[ "); /* strcpy: OK */
    va_start(args, str);
    vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
    va_end(args);
    strcat(buf, " ]\r\n"); /* strcat: OK */

    for (i = descriptor_list; i; i = i->next)
    {
        if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
            continue;
        if (GET_ADMLEVEL(i->character) < level)
            continue;
        if (PLR_FLAGGED(i->character, PLR_WRITING))
            continue;
        if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
            continue;

        i->character->send_to("@g%s@n", buf);
    }
}

/* If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.  */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
    size_t len = 0;
    int nlen;
    long nr;

    *result = '\0';

    for (nr = 0; bitvector && len < reslen; bitvector >>= 1)
    {
        if (IS_SET(bitvector, 1))
        {
            nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
            if (len + nlen >= reslen || nlen < 0)
                break;
            len += nlen;
        }

        if (*names[nr] != '\n')
            nr++;
    }

    if (!*result)
        len = strlcpy(result, "None ", reslen);

    return (len);
}

size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
    int nr = 0;

    while (type && *names[nr] != '\n')
    {
        type--;
        nr++;
    }

    return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}

time_t mud_time_to_secs(struct time_info_data *now)
{
    time_t when = 0;

    when += now->year * MUD_TIME_ACCELERATION * SECS_PER_YEAR;
    when += now->month * MUD_TIME_ACCELERATION * SECS_PER_MONTH;
    when += now->day * MUD_TIME_ACCELERATION * SECS_PER_DAY;
    when += now->hours * MUD_TIME_ACCELERATION * SECS_PER_HOUR;

    return (time(nullptr) - when);
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(Character *ch, Character *victim)
{
    Character *k;

    for (k = victim; k; k = k->master)
    {
        if (k == ch)
            return (true);
    }

    return (false);
}

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(Character *ch)
{
    struct follow_type *j, *k;

    if (ch->master == nullptr)
    {
        core_dump();
        return;
    }

    act("You stop following $N.", false, ch, nullptr, ch->master, TO_CHAR);
    act("$n stops following $N.", true, ch, nullptr, ch->master, TO_NOTVICT);
    if (!(DEAD(ch->master) || (ch->master->desc && STATE(ch->master->desc) == CON_MENU)))
        act("$n stops following you.", true, ch, nullptr, ch->master, TO_VICT);

    if (has_group(ch))
        ch->setBaseStat<int>("group_kills", 0);
    
    ch->master->followers.remove(ch->shared_from_this());
    ch->master = nullptr;
}

int num_followers_charmed(Character *ch)
{
    struct follow_type *lackey;
    int total = 0;

    /* Summoned creatures don't count against total */
    ch->followers.for_each([&](auto lackey) {
        if (AFF_FLAGGED(lackey, AFF_CHARM) &&
            !AFF_FLAGGED(lackey, AFF_SUMMONED) &&
            lackey->master == ch)
            total++;
    });

    return (total);
}

void switch_leader(Character *old, Character *new_leader)
{
    auto grabFollowers = old->followers;
    grabFollowers.remove(new_leader->shared_from_this());
    grabFollowers.add(old->shared_from_this());
    grabFollowers.for_each([&](auto f) {
        f->master = new_leader;
    });
    old->followers.clear();
    new_leader->followers = std::move(grabFollowers);
}

/* Called when a character that follows/is followed dies */
void die_follower(Character *ch)
{

    if (ch->master)
        stop_follower(ch);
    ch->followers.for_each_safe([&](Character* k) {stop_follower(k);});
}

/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(Character *ch, Character *leader)
{
    if (ch->master)
    {
        core_dump();
        return;
    }

    ch->master = leader;

    leader->followers.add(ch->shared_from_this());

    act("You now follow $N.", false, ch, nullptr, leader, TO_CHAR);
    if (IN_ROOM(ch) != NOWHERE && IN_ROOM(leader) != NOWHERE && leader->canSee(ch))
    {
        act("$n starts following you.", true, ch, nullptr, leader, TO_VICT);
        act("\r\n$n starts to follow $N.", true, ch, nullptr, leader, TO_NOTVICT);
    }
}

/* get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file. Buffer given must
 * be at least READ_SIZE (256) characters large.  */
int get_line(FILE *fl, char *buf)
{
    char temp[READ_SIZE];
    int lines = 0;
    int sl;

    do
    {
        if (!fgets(temp, READ_SIZE, fl))
            return (0);
        lines++;
    } while (*temp == '*' || *temp == '\n' || *temp == '\r');

    /* Last line of file doesn't always have a \n, but it should. */
    sl = strlen(temp);
    while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
        temp[--sl] = '\0';

    strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
    return (lines);
}

int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
    const char *prefix, *middle, *suffix;
    char name[PATH_MAX], *ptr;

    if (orig_name == nullptr || *orig_name == '\0' || filename == nullptr)
    {
        basic_mud_log("SYSERR: nullptr pointer or empty string passed to get_filename(), %p or %p.",
                      orig_name, filename);
        return (0);
    }

    switch (mode)
    {
    case CRASH_FILE:
        prefix = LIB_PLROBJS;
        suffix = SUF_OBJS;
        break;
    case ALIAS_FILE:
        prefix = LIB_PLRALIAS;
        suffix = SUF_ALIAS;
        break;
    case ETEXT_FILE:
        prefix = LIB_PLRTEXT;
        suffix = SUF_TEXT;
        break;
    case SCRIPT_VARS_FILE:
        prefix = LIB_PLRVARS;
        suffix = SUF_MEM;
        break;
    case NEW_OBJ_FILES:
        prefix = LIB_PLROBJS;
        suffix = SUF_OBJS;
        break;
    case PLR_FILE:
        prefix = LIB_PLRFILES;
        suffix = SUF_PLR;
        break;
    case IMC_FILE:
        prefix = LIB_PLRIMC;
        suffix = SUF_IMC;
        break;
    case PET_FILE:
        prefix = LIB_PLRFILES;
        suffix = SUF_PET;
        break;
    case USER_FILE:
        prefix = LIB_USER;
        suffix = SUF_USER;
        break;
    case INTRO_FILE:
        prefix = LIB_INTRO;
        suffix = SUF_INTRO;
        break;
    case SENSE_FILE:
        prefix = LIB_SENSE;
        suffix = SUF_SENSE;
        break;
    case CUSTOME_FILE:
        prefix = LIB_USER;
        suffix = SUF_CUSTOM;
        break;
    default:
        return (0);
    }

    strlcpy(name, orig_name, sizeof(name));
    for (ptr = name; *ptr; ptr++)
        *ptr = tolower(*ptr);

    switch (tolower(*name))
    {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
        middle = "A-E";
        break;
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
        middle = "F-J";
        break;
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
        middle = "K-O";
        break;
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
        middle = "P-T";
        break;
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
        middle = "U-Z";
        break;
    default:
        middle = "ZZZ";
        break;
    }

    snprintf(filename, fbufsize, "%s%s" SLASH "%s.%s", prefix, middle, name, suffix);
    return (1);
}


void core_dump_real(const char *who, int line)
{
    throw std::runtime_error(
        fmt::format("Core dump requested by {} at line {} in file {}",
                    who, line, __FILE__));
}

// A C++ version of proc_color from comm.c. it returns the colored string.
std::string processColors(const std::string &txt, int parse, char **choices)
{
    char *dest_char, *source_char, *color_char, *save_pos, *replacement = nullptr;
    int i, temp_color;
    size_t wanted;

    if (!txt.size() || !strchr(txt.c_str(), '@')) /* skip out if no color codes     */
        return txt;

    std::string out;
    source_char = (char *)txt.c_str();

    save_pos = dest_char;
    for (; *source_char;)
    {
        /* no color code - just copy */
        if (*source_char != '@')
        {
            out.push_back(*source_char++);
            continue;
        }

        /* if we get here we have a color code */

        source_char++; /* source_char now points to the code */

        /* look for a random color code picks a random number between 1 and 14 */
        if (*source_char == 'x')
        {
            temp_color = (rand() % 14);
            *source_char = RANDOM_COLORS[temp_color];
        }

        if (*source_char == '\0')
        { /* string was terminated with color code - just put it in */
            out.push_back('@');
            /* source_char will now point to '\0' in the for() check */
            continue;
        }

        if (!parse)
        { /* not parsing, just skip the code, unless it's @@ */
            if (*source_char == '@')
            {
                out.push_back('@');
            }
            if (*source_char == '[')
            { /* Multi-character code */
                source_char++;
                while (*source_char && isdigit(*source_char))
                    source_char++;
                if (!*source_char)
                    source_char--;
            }
            source_char++; /* skip to next (non-colorcode) char */
            continue;
        }

        /* parse the color code */
        if (*source_char == '[')
        { /* User configurable color */
            source_char++;
            if (*source_char)
            {
                i = atoi(source_char);
                if (i < 0 || i >= NUM_COLOR)
                    i = COLOR_NORMAL;
                replacement = default_color_choices[i];
                if (choices && choices[i])
                    replacement = choices[i];
                while (*source_char && isdigit(*source_char))
                    source_char++;
                if (!*source_char)
                    source_char--;
            }
        }
        else if (*source_char == 'n')
        {
            replacement = default_color_choices[COLOR_NORMAL];
            if (choices && choices[COLOR_NORMAL])
                replacement = choices[COLOR_NORMAL];
        }
        else
        {
            for (i = 0; CCODE[i] != '!'; i++)
            { /* do we find it ? */
                if ((*source_char) == CCODE[i])
                { /* if so :*/
                    replacement = ANSI[i];
                    break;
                }
            }
        }
        if (replacement)
        {
            if (isdigit(replacement[0]))
                for (color_char = ANSISTART; *color_char;)
                    out.push_back(*color_char++);
            for (color_char = replacement; *color_char;)
                out.push_back(*color_char++);
            if (isdigit(replacement[0]))
                out.push_back(ANSIEND);
            replacement = nullptr;
        }
        /* If we couldn't find any correct color code, or we found it and
         * substituted above, let's just process the next character.
         * - Welcor
         */
        source_char++;

    } /* for loop */

    return out;
}

size_t countColors(const std::string &txt)
{
    auto stripped = processColors(txt, false, nullptr);
    return txt.size() - stripped.size();
}

std::vector<AdminFlag> default_admin_flags_mortal = {};

std::vector<AdminFlag> default_admin_flags_immortal =
    {AdminFlag::see_invisible, AdminFlag::see_secret, AdminFlag::full_where, AdminFlag::no_poison, AdminFlag::walk_anywhere,
     AdminFlag::no_damage, AdminFlag::no_steal};

std::vector<AdminFlag> default_admin_flags_builder = {};

std::vector<AdminFlag> default_admin_flags_god =
    {AdminFlag::all_shops, AdminFlag::tell_all, AdminFlag::know_weather, AdminFlag::money, AdminFlag::eat_anything,
     AdminFlag::no_keys};
std::vector<AdminFlag> default_admin_flags_grgod =
    {AdminFlag::trans_all, AdminFlag::force_mass, AdminFlag::all_houses};
std::vector<AdminFlag> default_admin_flags_impl =
    {AdminFlag::switch_mortal, AdminFlag::instant_kill, AdminFlag::cedit};

std::vector<AdminFlag>* default_admin_flags[ADMLVL_IMPL + 1] = {
    &default_admin_flags_mortal,
    &default_admin_flags_immortal,
    &default_admin_flags_builder,
    &default_admin_flags_god,
    &default_admin_flags_grgod,
    &default_admin_flags_impl};

void admin_set(Character *ch, int value)
{
    int i;
    int orig = GET_ADMLEVEL(ch);

    value = std::clamp(value, 0, ADMLVL_IMPL - 1);

    if (GET_ADMLEVEL(ch) == value)
        return;
    if (GET_ADMLEVEL(ch) < value)
    { /* Promotion */
        mudlog(BRF, std::max(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true,
               "%s promoted from %s to %s", GET_NAME(ch), admin_level_names[GET_ADMLEVEL(ch)],
               admin_level_names[value]);
        while (GET_ADMLEVEL(ch) < value)
        {
            ch->modBaseStat<int>("admin_level", 1);
            for (auto f : *default_admin_flags[GET_ADMLEVEL(ch)])
                ch->admin_flags.set(f, true);
        }

        if (orig < ADMLVL_IMMORT && value >= ADMLVL_IMMORT)
        {
            for (auto f : {PRF_LOG2, PRF_HOLYLIGHT, PRF_ROOMFLAGS, PRF_AUTOEXIT})
                ch->pref_flags.set(f, true);
        }
        if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
        {
            for (i = 0; i < 3; i++)
                GET_COND(ch, i) = (char)-1;
            ch->pref_flags.set(PRF_HOLYLIGHT, true);
        }
        return;
    }
    if (GET_ADMLEVEL(ch) > value)
    { /* Demotion */
        mudlog(BRF, std::max(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true,
               "%s demoted from %s to %s", GET_NAME(ch), admin_level_names[GET_ADMLEVEL(ch)],
               admin_level_names[value]);
        while (GET_ADMLEVEL(ch) > value)
        {
            for (auto f : *default_admin_flags[GET_ADMLEVEL(ch)])
                ch->admin_flags.set(f, false);
            ch->modBaseStat<int>("admin_level", -1);
        }

        if (orig >= ADMLVL_IMMORT && value < ADMLVL_IMMORT)
        {
            for (auto f : {PRF_LOG1, PRF_LOG2, PRF_NOHASSLE, PRF_HOLYLIGHT, PRF_ROOMFLAGS})
                ch->pref_flags.set(f, false);
        }
        return;
    }
}

int levenshtein_distance(std::string_view s1, std::string_view s2) {
    const std::size_t s1_len = s1.size();
    const std::size_t s2_len = s2.size();

    // 2D matrix stored as a single vector for cache locality
    std::vector<int> d((s1_len + 1) * (s2_len + 1));

    auto at = [&](std::size_t i, std::size_t j) -> int& {
        return d[i * (s2_len + 1) + j];
    };

    for (std::size_t i = 0; i <= s1_len; ++i)
        at(i, 0) = static_cast<int>(i);

    for (std::size_t j = 0; j <= s2_len; ++j)
        at(0, j) = static_cast<int>(j);

    for (std::size_t i = 1; i <= s1_len; ++i) {
        for (std::size_t j = 1; j <= s2_len; ++j) {
            at(i, j) = std::min({
                at(i - 1, j) + 1,                  // deletion
                at(i, j - 1) + 1,                  // insertion
                at(i - 1, j - 1) + (s1[i - 1] == s2[j - 1] ? 0 : 1) // substitution
            });
        }
    }

    return at(s1_len, s2_len);
}

int count_color_chars(std::string_view string)
{
    int num = 0;

    for (size_t i = 0; i < string.size(); i++)
    {
        while (string[i] == '@')
        {
            if (string[i + 1] == '@')
            {
                num++;
            }
            else if (string[i + 1] == '[')
            {
                num += 4;
            }
            else
            {
                num += 2;
            }
            i += 2;
        }
    }
    return num;
}

/* Turns number into string and adds commas to it. */
std::string add_commas(double X)
{
    return fmt::format("{:L}", X);
}

int get_flag_by_name(const char *flag_list[], char *flag_name)
{
    int i = 0;
    for (; flag_list[i] && *flag_list[i] && strcmp(flag_list[i], "\n") != 0; i++)
        if (!strcmp(flag_list[i], flag_name))
            return (i);
    return (NOFLAG);
}

int16_t GET_SKILL_BONUS(Character *ch, uint16_t skill)
{
    return ch->getAffectModifier(APPLY_SKILL, skill);
}

int16_t GET_SKILL_PERF(Character *ch, uint16_t skill)
{
    auto found = ch->skill.find(static_cast<Skill>(skill));
    if (found != ch->skill.end())
    {
        return found->second.perfs;
    }
    return 0;
}

int16_t GET_SKILL_BASE(Character *ch, uint16_t skill)
{
    auto found = ch->skill.find(static_cast<Skill>(skill));
    if (found != ch->skill.end())
    {
        return found->second.level;
    }
    return 0;
}

int16_t GET_SKILL(Character *ch, uint16_t skill)
{
    return GET_SKILL_BASE(ch, skill) + GET_SKILL_BONUS(ch, skill);
}

void SET_SKILL(Character *ch, uint16_t skill, int16_t val)
{
    auto &s = ch->skill[static_cast<Skill>(skill)];
    s.level = val;
}

void SET_SKILL_BONUS(Character *ch, uint16_t skill, int16_t val)
{
}

void SET_SKILL_PERF(Character *ch, uint16_t skill, int16_t val)
{
    auto &s = ch->skill[static_cast<Skill>(skill)];
    s.perfs = val;
}

bool OBJ_FLAGGED(Object *obj, ItemFlag flag)
{
    return obj->item_flags.get(flag);
}

bool OBJAFF_FLAGGED(Object *obj, AffectFlag flag)
{
    return obj->affect_flags.get(flag);
}

bool ROOM_FLAGGED(Room *loc, int flag)
{
    if (!loc)
        return false;
    return loc->room_flags.get(flag);
}

bool ROOM_FLAGGED(room_vnum loc, int flag)
{
    return ROOM_FLAGGED(get_room(loc), flag);
}

bool WHERE_FLAGGED(Room *loc, WhereFlag flag)
{
    if (!loc)
        return false;
    Location l(loc);
    return l.getWhereFlag(flag);
}

bool WHERE_FLAGGED(room_vnum loc, WhereFlag flag)
{
    return WHERE_FLAGGED(get_room(loc), flag);
}



bool ADM_FLAGGED(Character *ch, AdminFlag flag)
{
    return ch->admin_flags.get(flag);
}

bool PRF_FLAGGED(Character *ch, int flag)
{
    return ch->pref_flags.get(flag);
}

bool PLR_FLAGGED(Character *ch, int flag)
{
    return ch->player_flags.get(flag);
}

bool AFF_FLAGGED(CharacterPrototype *ch, int flag)
{
    return ch->affect_flags.get(flag);
}

bool AFF_FLAGGED(Character *ch, AffectFlag flag) {
if (ch->affect_flags.get(flag))
        return true;
    for (auto i = 0; i < NUM_WEARS; i++)
        if (auto eq = GET_EQ(ch, i); eq)
            if (eq->affect_flags.get(flag))
                return true;
    return false;
}

bool AFF_FLAGGED(Character *ch, int flag)
{
    return ch->affect_flags.get(static_cast<AffectFlag>(flag));
}

bool ETHER_STREAM(Character *ch)
{
    return ch->location.getEnvironment(ENV_ETHER_STREAM) > 0;
}

int GET_SPEEDI(Character *ch)
{
    return (GET_SPEEDCALC(ch) + GET_SPEEDBONUS(ch) + GET_SPEEDBOOST(ch) + GET_MUTBOOST(ch));
}

bool MOON_TIMECHECK()
{
    switch (time_info.day)
    {
    case 20:
        return time_info.hours >= 21;
    case 21:
        return time_info.hours <= 4 || time_info.hours >= 21;
    case 22:
        return time_info.hours <= 4;
    }
    return false;
}

bool is_numeric(const std::string &str)
{
    return boost::algorithm::all(str, [](unsigned char c)
                                 { return std::isdigit(c); });
}

bool is_all_alpha(const std::string &str)
{
    return boost::algorithm::all(str, [](unsigned char c)
                                 { return std::isalpha(c); });
}

void craftProgress(Character *ch)
{
    bool continueCraft = true;
    continueCraft = ch->craftingDeck.playTopCard(ch);

    if (!continueCraft)
    {
        ch->sendText("You finish your project!\r\n");
        act("$n finally finishes their project!", true, ch, nullptr, nullptr, TO_ROOM);
        ch->addToInventory(ch->craftingTask.pObject);
        ch->craftingTask.pObject = nullptr;
        ch->craftingTask.improvementRounds = 0;

        ch->setTask(Task::nothing);
    }
    else
    {
        improve_skill(ch, SKILL_BUILD, 1);
        WAIT_STATE(ch, PULSE_5SEC * 4);
    }
}

void doContinuedTask(Character *ch)
{
    auto task = ch->task;

    if (task == Task::meditate)
    {
        meditateProgress(ch);
    }

    if (task == Task::situps)
    {
        situpProgress(ch);
    }

    if (task == Task::pushups)
    {
        pushupProgress(ch);
    }

    if (task == Task::trainStr || task == Task::trainAgl || task == Task::trainCon || task == Task::trainSpd || task == Task::trainInt || task == Task::trainWis)
    {
        trainProgress(ch);
    }

    if (task == Task::crafting)
    {
        craftProgress(ch);
    }
}

void WAIT_STATE(Character *ch, double timeToWait)
{
    ch->setBaseStat("waitTime", timeToWait);
    characterSubscriptions.subscribe("commandWaitQueue", ch);
}

std::string format_double(double value)
{
    if (std::floor(value) == value)
    {
        // It's a whole number
        return fmt::format("{:.0f}", value); // No decimal places
    }
    else
    {
        // Format with up to 2 decimal places
        return fmt::format("{:.2f}", value);
    }
}

Result<Zone*> getZone(std::string_view arg, Character* ch) {
    if(boost::iequals(arg, "here")) {
        if(auto z = ch->location.getZone()) {
            return z;
        }
        return err("You are not in a Zone, somehow. How the hell did that happen?\r\n");
    }
    auto res = parseNumber<zone_vnum>(arg, "Zone ID");
    if (!res) return err(res.error());

    if (auto zone = zone_table.find(res.value()); zone != zone_table.end()) {
        return zone->second.get();
    }
    return err("Zone {} does not exist.\r\n", res.value());
}

Result<std::string> validateZoneName(std::string_view arg, bool checkExists, zone_vnum exclude) {
    auto trimmed = boost::trim_copy(std::string(arg));
    if (trimmed.empty()) {
        return err("No zone name provided.\r\n");
    }
    if (checkExists) {
        for(const auto& [vn, z] : zone_table) {
            if(vn == exclude) continue;
            if(boost::iequals(trimmed, z->name)) {
                return err("Zone '{}' already exists.\r\n", trimmed);
            }
        }
    }
    return trimmed;
}


Result<Room*> getRoom(std::string_view arg, Character* ch) {
    if(boost::iequals(arg, "here")) {
        if(auto r = dynamic_cast<Room*>(ch->location.getLoc())) {
            return r;
        }
        return err("You are not in a Room.\r\n");
    }
    auto res = parseNumber(arg, "Room ID");
    if (!res) return err(res.error());

    if (auto room = Room::registry.find(res.value()); room != Room::registry.end()) {
        auto r = room->second.get();
        return r;
    }
    return err("Room {} does not exist.\r\n", res.value());

}

Result<Location> getLocation(std::string_view arg, Character* ch) {
    if(boost::iequals(arg, "here")) {
        if(auto loc = ch->location) {
            return loc;
        }
        return err("You are not in a Location, somehow. How the hell did that happen?\r\n");
    }
    if(auto res = Location(std::string(arg))) {
        return res;
    }
    return err("Location {} does not exist.\r\n", arg);
}

std::string ansiSafeString(std::string_view arg) {
    std::string out(arg);
    boost::trim(out);
    if(out.contains("@") && !out.ends_with("@n")) out += "@n";
    return out;
}

std::string replaceStringLine(std::string_view arg, bool enforceNewLine) {
    std::string out;
    // @- = \t
    // @/ = \r\n

    bool escaped = false;

    for (char c : arg) {
        if (c == '@' && !escaped) {
            escaped = true;
        } else {
            if (escaped) {
                if (c == '-') {
                    out += "\t";
                } else if (c == '/') {
                    out += "\r\n";
                } else {
                    out += '@';
                    out += c;
                }
                escaped = false;
            } else {
                out += c;
            }
        }
    }

    // ensure that ANSI strings don't bleed.
    boost::trim_right(out); // Trim trailing whitespace
    if(out.contains("@") && !out.ends_with("@n")) out += "@n";
    if(enforceNewLine && !out.ends_with("\r\n")) out += "\r\n";

    return out;
}

int file_to_string(const char *name, char *buf) {
    try {
        std::ifstream f(name);
        std::string out;
        // read all of f to out.
        std::getline(f, out, '\0');
        boost::trim_right(out);
        strcpy(buf, out.c_str());
        return 0;
    } catch (std::exception &e) {
        return -1;
    }
}

int file_to_string_alloc(const char *name, char **buf) {
    int temppage;
    char temp[MAX_STRING_LENGTH];

    /* Lets not free() what used to be there unless we succeeded. */
    if (file_to_string(name, temp) < 0)
        return (-1);

    if (*buf)
        free(*buf);

    *buf = strdup(temp);
    return (0);
}