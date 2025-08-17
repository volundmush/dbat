/**************************************************************************
 *   File: act.other.c                                   Part of CircleMUD *
 *  Usage: Miscellaneous player-level commands                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/
#include <boost/algorithm/string.hpp>

#include "dbat/act.other.h"
#include "dbat/send.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/dg_comm.h"
#include "dbat/combat.h"
#include "dbat/config.h"
#include "dbat/act.misc.h"
#include "dbat/weather.h"
#include "dbat/act.item.h"
#include "dbat/act.wizard.h"
#include "dbat/act.informative.h"
#include "dbat/act.movement.h"
#include "dbat/obj_edit.h"
#include "dbat/graph.h"
#include "dbat/spells.h"
#include "dbat/interpreter.h"
#include "dbat/fight.h"
#include "dbat/transformation.h"
#include "dbat/class.h"
#include "dbat/constants.h"
#include "dbat/shop.h"
#include "dbat/feats.h"
#include "dbat/guild.h"
#include "dbat/dg_scripts.h"
#include "dbat/mail.h"
#include "dbat/clan.h"
#include "dbat/players.h"
#include "dbat/random.h"

/* local functions */
static int has_scanner(Character *ch);

static void boost_obj(Object *obj, Character *ch, int type);

static int
perform_group(Character *ch, Character *vict, int highlvl, int lowlvl, int64_t highpl, int64_t lowpl);

static void print_group(Character *ch);

static void check_eq(Character *ch);

static int spell_in_book(Object *obj, int spellnum);

static int spell_in_scroll(Object *obj, int spellnum);

static int spell_in_domain(Character *ch, int spellnum);

static void show_clan_info(Character *ch);

// definitions
void log_imm_action(const char *messg, ...)
{

    FILE *fl;
    const char *filename;
    struct stat fbuf;

    filename = REQUEST_FILE;

    if (stat(filename, &fbuf) < 0)
    {
        perror("SYSERR: Can't stat() file");
        /*  SYSERR_DESC:
         *  This is from do_gen_write() and indicates that it cannot call the
         *  stat() system call on the file required.  The error string at the
         *  end of the line should explain what the problem is.
         */
        return;
    }
    if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4)
    {
        return;
    }
    if (!(fl = fopen(filename, "a")))
    {
        perror("SYSERR: log_imm_action");
        /*  SYSERR_DESC:
         *  This is from do_gen_write(), and will be output if the file in
         *  question cannot be opened for appending to.  The error string
         *  at the end of the line should explain what the problem is.
         */

        return;
    }
    time_t ct = time(nullptr);
    char *time_s = asctime(localtime(&ct));

    va_list args;

    va_start(args, messg);
    time_s[strlen(time_s) - 1] = '\0';

    fprintf(fl, "%-15.15s :: ", time_s + 4);
    vfprintf(fl, messg, args);
    fprintf(fl, "\n");
    va_end(args);

    fclose(fl);
}

void log_custom(struct descriptor_data *d, Object *obj)
{
    FILE *fl;
    const char *filename;
    struct stat fbuf;

    filename = CUSTOM_FILE;

    if (stat(filename, &fbuf) < 0)
    {
        perror("SYSERR: Can't stat() file");
        /*  SYSERR_DESC:
         *  This is from do_gen_write() and indicates that it cannot call the
         *  stat() system call on the file required.  The error string at the
         *  end of the line should explain what the problem is.
         */
        return;
    }
    if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4)
    {
        return;
    }
    if (!(fl = fopen(filename, "a")))
    {
        perror("SYSERR: log_custom");
        /*  SYSERR_DESC:
         *  This is from do_gen_write(), and will be output if the file in
         *  question cannot be opened for appending to.  The error string
         *  at the end of the line should explain what the problem is.
         */

        return;
    }

    fprintf(fl, "@D[@cUser@W: @R%-20s @cName@W: @C%-20s @cCustom@W: @Y%s@D]\n", GET_USER(d->character),
            GET_NAME(d->character), obj->getShortDescription());
    fclose(fl);
}

/* Used by do_rpp for soft-cap */
void bring_to_cap(Character *ch)
{

    auto cap = ch->calc_soft_cap();

    for (auto stat : {"health", "stamina", "ki"})
    {
        if (auto diff = cap - ch->getBaseStat(stat); diff > 0)
        {
            ch->modBaseStat(stat, diff);
        }
    }
}

/* Let's Reward Those Roleplayers! - Iovan*/
ACMD(do_rpp)
{

    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int tnlcost = 1, revcost = 1, selection = 0, pay = 0, bpay = 0;
    int max_choice = 15; /* Controls the maximum number of menu choices */
    Object *obj;

    half_chop(argument, arg, arg2);

    if (IS_NPC(ch))
    {
        return;
    }

    revcost = revcost * (GET_LEVEL(ch) / 15);
    tnlcost = 1 + (tnlcost * (GET_LEVEL(ch) / 40));

    if (revcost < 1)
    {
        revcost = 1;
    }
    if (tnlcost < 1)
    {
        tnlcost = 1;
    }

    if (PLR_FLAGGED(ch, PLR_PDEATH))
    {
        revcost *= 6;
    }

    if (!*arg)
    { /* Display menu */
        ch->sendText("@C                             Rewards Menu\n");
        ch->sendText("@b  ------------------------------------------------------------------\n");
        ch->sendText("  @C1@D);@R Disabled            @D[@G -- RPP @D]  @C2@D)@R Custom Equipment      @D[@G -- RPP @D]@n\n");
        ch->sendText("  @C3@D);@c Alignment Change    @D[@G 20 RPP @D]  @C4@D)@c 7,500 zenni           @D[@G 20 RPP @D]\n");
        ch->sendText("  @C5@D);@c +2 To A Stat        @D[@G  1 RPP @D]  @C6@D)@c +750 PS               @D[@G  2 RPP @D]\n");
        ch->send_to("  @C7@D);@c Revival             @D[@G  4 RPP @D]  @C8@D)@c Aura Change           @D[@G%3d RPP @D]\n",
                    revcost);
        ch->sendText("  @C9@D);@c RPP Store           @D[@G??? RPP @D] @C10@D)@c Extra Feature         @D[@G  2 RPP @D]\n");
        ch->sendText(" @C11@D);@c Restring Equipment  @D[@G  5 RPP @D] @C12@D)@c Extra Skillslot       @D[@G ?? RPP @D]\n");
        ch->sendText("@b  ------------------------------------------------------------------@n\n");
        ch->send_to("@D                           [@YYour RPP@D:@G %3d@D]@n\n", GET_RP(ch));
        //         ch->send_to("@D                           [@YRPP Bank@D:@G %3d@D]@n\n", GET_RBANK(ch));
        ch->sendText("\nSyntax: rpp (num)\n");
        return;
    }

    /* What choice did they make? */

    selection = atoi(arg);

    if (selection <= 0 || selection > max_choice)
    {
        ch->sendText("You must choose a number from the menu. Enter the command again with no arguments for the menu.\r\n");
        return;
    }

    /* Instant Reward Section*/
    if (selection > 2)
    {

        if (selection == 2)
        { /* Custom Equipment Construction */
            if (GET_RP(ch) < 20)
            {
                ch->sendText("You need at least 20 RPP to initiate a custom equipment build.\r\n");
                return;
            }
            else
            {
                STATE(ch->desc) = CON_POBJ;
                ch->desc->obj_name = strdup("Generic Armor Vest");
                ch->desc->obj_short = strdup("@cGeneric @DArmor @WVest@n");
                ch->desc->obj_long = strdup("@wA @cgeneric @Darmor @Wvest@w is lying here@n");
                ch->desc->obj_type = 1;
                ch->desc->obj_weapon = 0;
                disp_custom_menu(ch->desc);
                ch->desc->obj_editflag = EDIT_CUSTOM;
                ch->desc->obj_editval = EDIT_CUSTOM_MAIN;
                return;
            }
        }
        if (selection == 3)
        { /* Simple Align Change*/
            pay = 20;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("You do not have enough RPP for that selection.\r\n");
                return;
            }
            else
            {
                if (!*arg2)
                {
                    ch->sendText("What do you want to change your alignment to? (evil, sorta-evil, neutral, sorta-good, good)");
                    return;
                }
                if (!strcasecmp(arg2, "evil"))
                {
                    ch->sendText("You change your alignment to Evil.\r\n");
                    ch->setBaseStat("good_evil", -750);
                }
                else if (!strcasecmp(arg2, "sorta-evil"))
                {
                    ch->sendText("You change your alignment to Sorta Evil.\r\n");
                    ch->setBaseStat("good_evil", -50);
                }
                else if (!strcasecmp(arg2, "neutral"))
                {
                    ch->sendText("You change your alignment to Neutral.\r\n");
                    ch->setBaseStat("good_evil", 0);
                }
                else if (!strcasecmp(arg2, "sorta-good"))
                {
                    ch->sendText("You change your alignment to Sorta Good.\r\n");
                    ch->setBaseStat("good_evil", 51);
                }
                else if (!strcasecmp(arg2, "good"))
                {
                    ch->sendText("You change your alignment to Good.\r\n");
                    ch->setBaseStat("good_evil", 300);
                }
                else
                {
                    ch->sendText("That is not an acceptable option for changing alignment.\r\n");
                    return;
                }

            } /* Can pay for it */
        } /* End Simple Align Change */

        if (selection == 4)
        { /*Simple Zenni Reward*/
            pay = 1;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("You do not have enough RPP for that selection.\r\n");
                return;
            }
            else
            {
                ch->modBaseStat("money_bank", 7500);
                ch->sendText("Your bank zenni has been increased by 7,500\r\n");
            } /* Can pay for it */
        } /* End Simple Zenni Reward */

        if (selection == 5)
        { /* Simple Stat Change*/
            pay = 2;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("You do not have enough RPP for that selection.\r\n");
                return;
            }
            else
            {
                if (!*arg2)
                {
                    ch->sendText("What stat? (str, con, int, wis, spd, agl)");
                    return;
                }

                const std::map<std::string, std::tuple<std::string, int>> stat_map = {
                    {"str", {"strength", BONUS_WIMP}},
                    {"con", {"constitution", BONUS_FRAIL}},
                    {"int", {"intelligence", BONUS_DULL}},
                    {"wis", {"wisdom", BONUS_FOOLISH}},
                    {"spd", {"speed", BONUS_SLOW}},
                    {"agl", {"agility", BONUS_CLUMSY}}};

                std::string entry(arg2);
                boost::to_lower(entry);
                if (auto stat_found = stat_map.find(entry); stat_found != stat_map.end())
                {
                    auto [name, flaw] = stat_found->second;

                    auto base = ch->getBaseStat(name);

                    if (GET_BONUS(ch, flaw) > 0 && base >= 45)
                    {
                        ch->sendText("You can't because that stat maxes at 45 due to a trait negative.\r\n");
                        return;
                    }
                    if (base >= 80)
                    {
                        ch->sendText("80 is the maximum base for any stat.\r\n");
                        return;
                    }

                    if (ch->modBaseStat(name, 2) < 80)
                        pay--;
                }
                else
                {
                    ch->sendText("Invalid stat.\r\n");
                    return;
                }

            } /* Can pay for it */
        } /* End Simple Align Change */

        if (selection == 6)
        { /*Simple PS Reward*/
            pay = 4;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("You do not have enough RPP for that selection.\r\n");
                return;
            }
            else
            {
                ch->modPractices(750);
                ch->sendText("Your practices have been increased by 750\r\n");
            } /* Can pay for it */
        } /* End Simple Zenni Reward */

        if (selection == 7)
        { /* Simple Revival Reward */
            pay = revcost;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("You do not have enough RPP for that selection.\r\n");
                return;
            }
            else if (!AFF_FLAGGED(ch, AFF_SPIRIT))
            {
                ch->sendText("You aren't even dead!");
                return;
            }
            else
            {
                ch->resurrect(RPP);
                ch->sendText("You have been revived.\r\n");
            } /* Can pay for it */
        } /* End Simple Revival Reward */

        if (selection == 8)
        { /* Simple Aura Change*/
            pay = 2;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("You do not have enough RPP for that selection.\r\n");
                return;
            }
            else
            {
                if (!*arg2)
                {
                    ch->sendText("Change your aura to what? (white, blue, red, green, pink, purple, yellow, black, orange)");
                    return;
                }
                /*
                appearance_t newAura = 0;
                if (!strcasecmp(arg2, "white")) {
                    newAura = 0;
                                        ch->sendText("You change your aura to white.\r\n");
                } else if (!strcasecmp(arg2, "blue")) {
                    newAura = 1;
                                        ch->sendText("You change your aura to blue.\r\n");
                } else if (!strcasecmp(arg2, "red")) {
                    newAura = 2;
                                        ch->sendText("You change your aura to red.\r\n");
                } else if (!strcasecmp(arg2, "green")) {
                    newAura = 3;
                                        ch->sendText("You change your aura to green.\r\n");
                } else if (!strcasecmp(arg2, "pink")) {
                    newAura = 4;
                                        ch->sendText("You change your aura to pink.\r\n");
                } else if (!strcasecmp(arg2, "purple")) {
                    newAura = 5;
                                        ch->sendText("You change your aura to purple.\r\n");
                } else if (!strcasecmp(arg2, "yellow")) {
                    newAura = 6;
                                        ch->sendText("You change your aura to yellow.\r\n");
                } else if (!strcasecmp(arg2, "black")) {
                    newAura = 7;
                                        ch->sendText("You change your aura to black.\r\n");
                } else if (!strcasecmp(arg2, "orange")) {
                    newAura = 8;
                                        ch->sendText("You change your aura to orange.\r\n");
                } else {
                                        ch->sendText("That is not an acceptable option for changing alignment.\r\n");
                    return;
                }
                //ch->set(CharAppearance::aura, newAura);
                */

            } /* Can pay for it */
        } /* End Simple Aura Change */

        if (selection == 9)
        {
            if (!*arg2)
            {
                disp_rpp_store(ch);
                return;
            }
            else if (atoi(arg2) <= 0)
            {
                ch->sendText("That is not a choice in the RPP store!\r\n");
                return;
            }
            else
            {
                int choice = atoi(arg2);
                handle_rpp_store(ch, choice);
                return;
            }
        }

        if (selection == 10)
        { /* Extra Feature Reward*/
            rpp_feature(ch, arg2);
            return;
        } /* End Extra Feature Reward */

        if (selection == 11)
        { /* Restring equipment reward */
            pay = 1;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("You need at least 1 RPP to initiate an equipment restring.\r\n");
                return;
            }
            else if (!(obj = get_obj_in_list_vis(ch, arg2, nullptr, ch->getInventory())))
            {
                ch->sendText("You don't have a that equipment to restring in your inventory.\r\n");
                ch->sendText("Syntax: rpp 14 (obj name)\r\n");
                return;
            }
            else if (OBJ_FLAGGED(obj, ITEM_CUSTOM))
            {
                ch->sendText("You can not restring a custom piece. Why? Cause I say so. :P\r\n");
                return;
            }
            else
            {
                STATE(ch->desc) = CON_POBJ;
                char thename[MAX_INPUT_LENGTH], theshort[MAX_INPUT_LENGTH], thelong[MAX_INPUT_LENGTH];

                *thename = '\0';
                *theshort = '\0';
                *thelong = '\0';

                sprintf(thename, "%s", obj->getName());
                sprintf(theshort, "%s", obj->getShortDescription());
                sprintf(thelong, "%s", obj->getRoomDescription());

                ch->desc->obj_name = strdup(thename);
                ch->desc->obj_was = strdup(theshort);
                ch->desc->obj_short = strdup(theshort);
                ch->desc->obj_long = strdup(thelong);
                ch->desc->obj_point = obj;
                ch->desc->obj_type = 1;
                ch->desc->obj_weapon = 0;
                disp_restring_menu(ch->desc);
                ch->desc->obj_editflag = EDIT_RESTRING;
                ch->desc->obj_editval = EDIT_RESTRING_MAIN;
                return;
            }
        } /* End equipment restring reward */

        if (selection == 12)
        { /* Skillslot Reward */
            pay = 3;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("You do not have enough RPP in your bank for that selection.\r\n");
                return;
            }
            else if (GET_BONUS(ch, BONUS_GMEMORY) && GET_SLOTS(ch) >= 65)
            {
                ch->sendText("You are already at your skillslot cap.\r\n");
                return;
            }
            else if (!GET_BONUS(ch, BONUS_GMEMORY) && (GET_SLOTS(ch) >= 60))
            {
                ch->sendText("You are already at your skillslot cap.\r\n");
                return;
            }
            else
            {
                ch->modBaseStat<int>("skill_slots", 1);
            } /* Can pay for it */
        } /*End Skillslot Reward */

        if (selection == -1)
        { /* DB Spawn Reward */
            pay = 5000;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("You do not have enough RPP in your bank for that selection.\r\n");
                return;
            }
            else
            {
                int found = false;
                auto ao = objectSubscriptions.all("active");
                for (auto k : filter_raw(ao))
                {
                    if (OBJ_FLAGGED(k, ITEM_FORGED))
                    {
                        continue;
                    }
                    if (GET_OBJ_VNUM(k) == 20)
                    {
                        found = true;
                    }
                    else if (GET_OBJ_VNUM(k) == 21)
                    {
                        found = true;
                    }
                    else if (GET_OBJ_VNUM(k) == 22)
                    {
                        found = true;
                    }
                    else if (GET_OBJ_VNUM(k) == 23)
                    {
                        found = true;
                    }
                    else if (GET_OBJ_VNUM(k) == 24)
                    {
                        found = true;
                    }
                    else if (GET_OBJ_VNUM(k) == 25)
                    {
                        found = true;
                    }
                    else if (GET_OBJ_VNUM(k) == 26)
                    {
                        found = true;
                    }
                }
                if (found == false)
                {
                    ch->sendText("You have reduced the Dragon Ball wait by a whole real life day!\r\n");
                    send_to_all("%s has just reduced the Dragon Ball wait by a whole real life day!\r\n", GET_NAME(ch));
                    dballtime -= 86400;
                    if (dballtime <= 0)
                    {
                        dballtime = 1;
                    }
                }
                else if (SELFISHMETER >= 10)
                {
                    ch->sendText("Sorry, it seems there there are several powers interfering with the Dragon Balls.\r\n");
                    return;
                }
                else
                {
                    ch->sendText("Sorry, but there is already a set of Dragon Balls in existence.\r\n");
                    return;
                }
            } /* Can pay for it */
        }

    } /* End Instant Reward Section */

    /* Resuest Only Section */
    if (selection <= 2)
    {
        FILE *fl;
        const char *filename;
        struct stat fbuf;

        filename = REQUEST_FILE;

        if (selection == 1)
        {
            pay = 6500;
            if (GET_RP(ch) < pay)
            {
                ch->sendText("Nice try but you don't have enough RPP for that.\r\n");
                return;
            }
            else
            {
                ch->sendText("You now have an Excel House Capsule!\r\n");
                Object *hobj = read_object(6, VIRTUAL);
                ch->addToInventory(hobj);
                ch->modRPP(-pay);
                ch->send_to("@R%d@W RPP paid for your selection. Enjoy!@n\r\n", pay);
                send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), pay);
                return;
            }
        }
        else if (selection == 2)
        {
            return;
            pay = 200;
        }

        if (GET_RP(ch) < pay)
        {
            ch->sendText("Nice try but you don't have enough RPP for that.\r\n");
            return;
        }

        if (stat(filename, &fbuf) < 0)
        {
            perror("SYSERR: Can't stat() file");
            /*  SYSERR_DESC:
             *  This is from do_gen_write() and indicates that it cannot call the
             *  stat() system call on the file required.  The error string at the
             *  end of the line should explain what the problem is.
             */
            return;
        }
        if (fbuf.st_size >= CONFIG_MAX_FILESIZE)
        {
            ch->sendText("Sorry, the file is full right now.. try again later.\r\n");
            return;
        }
        if (!(fl = fopen(filename, "a")))
        {
            perror("SYSERR: do_reward_request");
            /*  SYSERR_DESC:
             *  This is from do_gen_write(), and will be output if the file in
             *  question cannot be opened for appending to.  The error string
             *  at the end of the line should explain what the problem is.
             */

            ch->sendText("Could not open the file.  Sorry.\r\n");
            return;
        }
        if (selection == 1)
        {
            fprintf(fl, "@D[@cName@W: @C%-20s @cRequest@W: @Y%-20s@D]\n", GET_NAME(ch), "House");
            send_to_imm("RPP Request: %s paid for house", GET_NAME(ch));
            BOARDNEWCOD = time(nullptr);
            save_mud_time(&time_info);
        }
        else if (selection == 2)
        {
            fprintf(fl, "@D[@cName@W: @C%-20s @cRequest@W: @Y%-20s@D]\n", GET_NAME(ch), "Custom Skill");
            send_to_imm("RPP Request: %s paid for Custom Skill, uhoh spaggettios", GET_NAME(ch));
            BOARDNEWCOD = time(nullptr);
            save_mud_time(&time_info);
        }
        ch->modRPP(-pay);
        ch->send_to("@R%d@W RPP paid for your selection. An immortal will address the request soon enough. Be patient.@n\r\n", pay);

        fclose(fl);
    } /* End Request Only Section */

    /* Pay for purchases here */
    if (selection >= 4 && selection < 12 && pay > 0)
    {
        ch->modRPP(-pay);
        ch->send_to("@R%d@W RPP paid for your selection. Enjoy!@n\r\n", pay);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), pay);
    }

    if (selection > 12 && pay > 0)
    {
        ch->modRPP(-pay);
        ch->send_to("@R%d@W RPP paid for your selection. Enjoy!@n\r\n", pay);
        send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), pay);
    }
}

ACMD(do_commune)
{

    if (IS_NPC(ch))
        return;

    if (!know_skill(ch, SKILL_COMMUNE))
    {
        return;
    }

    if ((ch->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(ch))
    {
        ch->sendText("Your stamina is already at full.\r\n");
        return;
    }

    int prob = GET_SKILL(ch, SKILL_COMMUNE), perc = axion_dice(0);
    int64_t cost = GET_MAX_MOVE(ch) * .05;

    if ((ch->getCurVital(CharVital::ki)) < cost)
    {
        ch->sendText("You do not have enough ki to commune with the Eldritch Star.\r\n");
        return;
    }
    if (prob < perc)
    {
        ch->modCurVital(CharVital::ki, -cost);
        reveal_hiding(ch, 0);
        act("@cYou close your eyes and try to commune with the Eldritch Star. You are unable to concentrate though.@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n closes $s eyes for a moment. Then $e reopens them and frowns.@n", true, ch, nullptr, nullptr,
            TO_ROOM);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
    else
    {
        ch->modCurVital(CharVital::ki, -cost);
        ch->modCurVital(CharVital::stamina, -cost);
        reveal_hiding(ch, 0);
        act("@cYou close your eyes and commune with the Eldritch Star spiritually. You feel your stamina replenish some.@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n closes $s eyes for a moment. Then $e reopens them and smiles.@n", true, ch, nullptr, nullptr,
            TO_ROOM);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
}

ACMD(do_willpower)
{

    int fail = false;

    if (IS_NPC(ch))
        return;

    if (ch->getBaseStat<int>("majinizer") <= 3)
    {
        ch->sendText("You are not majinized and have no need to reclaim full control of your own will.\r\n");
        return;
    }

    auto itg = ch->getBaseStat("internalGrowth");
    if (itg < 30 && GET_WIS(ch) < 100)
    {
        ch->sendText("You do not have enough Growth to focus your attempt to break free.\r\n");
        fail = true;
    }
    if (itg < 60 && GET_WIS(ch) >= 100)
    {
        ch->sendText("You do not have enough PS to focus your attempt to break free.\r\n");
        fail = true;
    }

    if (fail == true)
    {
        return;
    }

    ch->setExperience(0);
    if (rand_number(10, 100) - GET_INT(ch) > 60)
    {
        reveal_hiding(ch, 0);
        act("@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments you give up, having failed to overcome the majinization!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments $n seems to give up and the commotion dies down.@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        return;
    }
    else
    {
        ch->setExperience(0);
        ch->modBaseStat("internalGrowth", -30);
        reveal_hiding(ch, 0);
        act("@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments the ground splits beneath you and while letting out a piercing scream the M disappears from your forehead! You are free while still keeping the boost you had recieved from the majinization!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments the ground beneath $n splits and $e lets out a piercing scream. The M on $s forehead disappears!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        ch->setBaseStat("majinizer", 3);
        return;
    }
}

ACMD(do_grapple)
{

    /*if (IS_NPC(ch))
  return;*/

    if (!know_skill(ch, SKILL_GRAPPLE))
    {
        return;
    }

    if (PLR_FLAGGED(ch, PLR_THANDW))
    {
        ch->sendText("Your are too busy wielding your weapon with two hands!\r\n");
        return;
    }

    if (ABSORBING(ch))
    {
        ch->sendText("You are currently absorbing from someone!\r\n");
        return;
    }
    if (ABSORBBY(ch))
    {
        ch->sendText("You are currently being absorbed by someone! Try 'escape'!\r\n");
        return;
    }

    if (GRAPPLING(ch))
    {
        act("@RYou stop grappling with @r$N@R!@n", true, ch, nullptr, GRAPPLING(ch), TO_CHAR);
        act("@r$n@R stops grappling with @rYOU!!@n", true, ch, nullptr, GRAPPLING(ch), TO_VICT);
        act("@r$n@R stops grappling with @r$N@R!@n", true, ch, nullptr, GRAPPLING(ch), TO_NOTVICT);
        GRAPPLING(ch)->setBaseStat<int>("grapple_type", -1);
        GRAPPLED(GRAPPLING(ch)) = nullptr;
        GRAPPLING(ch) = nullptr;
        ch->setBaseStat<int>("grapple_type", -1);
        return;
    }

    if (GRAPPLED(ch))
    {
        ch->sendText("You are currently a victim of grappling! Try 'escape' to break free!\r\n");
        return;
    }

    if (!HAS_ARMS(ch))
    {
        ch->sendText("You have no available arms!\r\n");
        return;
    }

    Character *vict;
    char arg[200], arg2[200];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        ch->sendText("Syntax: grapple (target) (hold | choke | grab)\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        if (!FIGHTING(ch))
        {
            ch->sendText("That target isn't here.\r\n");
            return;
        }
        else
        {
            vict = FIGHTING(ch);
        }
    }

    if (!can_kill(ch, vict, nullptr, 0))
    {
        return;
    }

    if (AFF_FLAGGED(vict, AFF_KNOCKED))
    {
        ch->sendText("They are unconcious. What would be the point?\r\n");
        return;
    }

    if (GRAPPLED(vict))
    {
        ch->sendText("They are currently in someone else's grasp!\r\n");
        return;
    }

    if (ABSORBBY(vict))
    {
        ch->sendText("They are currently in someone else's grasp!\r\n");
        return;
    }

    if (ABSORBING(vict))
    {
        ch->sendText("They are currently absorbing from someone!\r\n");
        return;
    }

    int pass = false;

    if (!strcasecmp("hold", arg2) || !strcasecmp("choke", arg2) || !strcasecmp("grab", arg2) ||
        !strcasecmp("wrap", arg2))
    {
        pass = true;
        int perc = GET_SKILL(ch, SKILL_GRAPPLE), prob = axion_dice(0), cost = GET_MAX_MOVE(ch) / 100;

        if ((ch->getCurVital(CharVital::stamina)) < cost)
        {
            ch->sendText("You do not have enough stamina to grapple!\r\n");
            return;
        }

        if (((!IS_NPC(vict) && IS_ICER(vict) && rand_number(1, 30) >= 28) || AFF_FLAGGED(vict, AFF_ZANZOKEN)) &&
            (vict->getCurVital(CharVital::stamina)) >= 1 && GET_POS(vict) != POS_SLEEPING)
        {
            if (!AFF_FLAGGED(ch, AFF_ZANZOKEN) || (AFF_FLAGGED(ch, AFF_ZANZOKEN) && GET_SPEEDI(ch) + rand_number(1, 5) <
                                                                                        GET_SPEEDI(vict) +
                                                                                            rand_number(1, 5)))
            {
                reveal_hiding(ch, 0);
                act("@C$N@c disappears, avoiding your grapple attempt before reappearing!@n", false, ch, nullptr, vict,
                    TO_CHAR);
                act("@cYou disappear, avoiding @C$n's@c grapple attempt before reappearing!@n", false, ch, nullptr,
                    vict, TO_VICT);
                act("@C$N@c disappears, avoiding @C$n's@c grapple attempt before reappearing!@n", false, ch, nullptr,
                    vict, TO_NOTVICT);
                for (auto c : {ch, vict})
                    c->affect_flags.set(AFF_ZANZOKEN, false);
                ch->modCurVital(CharVital::stamina, -cost);
                WAIT_STATE(ch, PULSE_4SEC);
                return;
            }
            else
            {
                reveal_hiding(ch, 0);
                act("@C$N@c disappears, trying to avoid your grapple but your zanzoken is faster!@n", false, ch,
                    nullptr, vict, TO_CHAR);
                act("@cYou zanzoken to avoid the grapple attempt but @C$n's@c zanzoken is faster!@n", false, ch,
                    nullptr, vict, TO_VICT);
                act("@C$N@c disappears, trying to avoid @C$n's@c grapple attempt but @C$n's@c zanzoken is faster!@n",
                    false, ch, nullptr, vict, TO_NOTVICT);
                for (auto c : {ch, vict})
                    c->affect_flags.set(AFF_ZANZOKEN, false);
            }
        }

        if (GET_SPEEDI(ch) > GET_SPEEDI(vict) * 2)
        {
            perc += 5;
        }
        else if (GET_SPEEDI(ch) > GET_SPEEDI(vict))
        {
            perc += 2;
        }
        else if (GET_SPEEDI(ch) * 2 < GET_SPEEDI(vict))
        {
            perc -= 5;
        }
        else if (GET_SPEEDI(ch) < GET_SPEEDI(vict))
        {
            perc -= 2;
        }

        if ((GET_HIT(ch) * 0.02) * GET_STR(ch) < (GET_HIT(vict) * 0.01) * GET_STR(vict))
        {
            reveal_hiding(ch, 0);
            act("@RYou try to grapple with @r$N@R, but $E manages to overpower you!@n", true, ch, nullptr, vict,
                TO_CHAR);
            act("@r$n@R tries to grapple with YOU, but you manage to overpower $m!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@r$n@R tries to grapple with @r$N@R, but $E manages to overpower @r$n@R!@n", true, ch, nullptr, vict,
                TO_NOTVICT);
            ch->modCurVital(CharVital::stamina, -cost);
            improve_skill(ch, SKILL_GRAPPLE, 1);
            WAIT_STATE(ch, PULSE_4SEC);
            return;
        }
        else if ((GET_HIT(ch) * 0.01) * GET_STR(ch) < (GET_HIT(vict) * 0.01) * GET_STR(vict) &&
                 rand_number(1, 4) == 1)
        {
            reveal_hiding(ch, 0);
            act("@RYou try to grapple with @r$N@R, but $E manages to overpower you!@n", true, ch, nullptr, vict,
                TO_CHAR);
            act("@r$n@R tries to grapple with YOU, but you manage to overpower $m!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@r$n@R tries to grapple with @r$N@R, but $E manages to overpower @r$n@R!@n", true, ch, nullptr, vict,
                TO_NOTVICT);
            ch->modCurVital(CharVital::stamina, -cost);
            improve_skill(ch, SKILL_GRAPPLE, 1);
            WAIT_STATE(ch, PULSE_4SEC);
            return;
        }
        else if (perc < prob)
        {
            reveal_hiding(ch, 0);
            act("@RYou try to grapple with @r$N@R, but $E manages to avoid it!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@r$n@R tries to grapple with YOU, but you manage to avoid it!@n", true, ch, nullptr, vict, TO_VICT);
            act("@r$n@R tries to grapple with @r$N@R, but $E manages to avoid it!@n", true, ch, nullptr, vict,
                TO_NOTVICT);
            ch->modCurVital(CharVital::stamina, -cost);
            improve_skill(ch, SKILL_GRAPPLE, 1);
            WAIT_STATE(ch, PULSE_4SEC);
            return;
        }
        else if (!HAS_ARMS(vict) && !strcasecmp("grab", arg2))
        {
            ch->sendText("They don't even have an arm to grab onto!\r\n");
            return;
        }
        else if (!strcasecmp("hold", arg2))
        {
            reveal_hiding(ch, 0);
            act("@RYou rush at @r$N@R and manage to get $M in a hold from behind!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@r$n@R rushes at YOU and manages to get you in a hold from behind!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@r$n@R rushes at @r$N@R and manages to get $M in a hold from behind!@n", true, ch, nullptr, vict,
                TO_NOTVICT);

            /* Let's grapple! */
            GRAPPLING(ch) = vict;
            ch->setBaseStat<int>("grapple_type", 1);
            GRAPPLED(vict) = ch;
            vict->setBaseStat<int>("grapple_type", 1);
            /* Let's grapple! */

            ch->modCurVital(CharVital::stamina, -cost);
            improve_skill(ch, SKILL_GRAPPLE, 1);
            WAIT_STATE(ch, PULSE_4SEC);
            return;
        }
        else if (!strcasecmp("choke", arg2))
        {
            reveal_hiding(ch, 0);
            act("@RYou rush at @r$N@R and manage to grab $S throat with both hands!@n", true, ch, nullptr, vict,
                TO_CHAR);
            act("@r$n@R rushes at YOU and manages to grab your throat with both hands!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@r$n@R rushes at @r$N@R and manages to grab $S throat with both hands!@n", true, ch, nullptr, vict,
                TO_NOTVICT);

            /* Let's grapple! */
            GRAPPLING(ch) = vict;
            ch->setBaseStat<int>("grapple_type", 2);
            GRAPPLED(vict) = ch;
            vict->setBaseStat<int>("grapple_type", 2);
            /* Let's grapple! */

            ch->modCurVital(CharVital::stamina, -cost);
            improve_skill(ch, SKILL_GRAPPLE, 1);
            WAIT_STATE(ch, PULSE_4SEC);
            return;
        }
        else if (!strcasecmp("wrap", arg2))
        {
            if (!IS_MAJIN(ch))
            {
                ch->sendText("Your body is not flexible enough to wrap around a target!\r\n");
                return;
            }

            act("@MMoving quickly you stretch your body out and wrap it around the length of @c$N's@M body! You tighten your body until you begin crushing @c$N@M!",
                true, ch, nullptr, vict, TO_CHAR);
            act("@C$n@M quickly stretches out $s body and wraps it around @RYOU@M! You feel $s body begin to crush your own!@n",
                true, ch, nullptr, vict, TO_VICT);
            act("@C$n@M quickly stretches out $s body and wraps it around @c$N@M! It appears that @c$N's@M body is being crushed slowly!@n",
                true, ch, nullptr, vict, TO_NOTVICT);

            /* Let's grapple! */
            GRAPPLING(ch) = vict;
            ch->setBaseStat<int>("grapple_type", 4);
            GRAPPLED(vict) = ch;
            vict->setBaseStat<int>("grapple_type", 4);
            /* Let's grapple! */

            ch->modCurVital(CharVital::stamina, -cost);
            improve_skill(ch, SKILL_GRAPPLE, 1);
            WAIT_STATE(ch, PULSE_4SEC);
            return;
        }
        else if (!strcasecmp("grab", arg2))
        {
            reveal_hiding(ch, 0);
            act("@RYou rush at @r$N@R and manage to lock your arm onto $S!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@r$n@R rushes at YOU and manages to lock $s arm onto your's!@n", true, ch, nullptr, vict, TO_VICT);
            act("@r$n@R rushes at @r$N@R and manages to lock $s arm onto @r$N's@R!@n", true, ch, nullptr, vict,
                TO_NOTVICT);

            /* Let's grapple! */
            GRAPPLING(ch) = vict;
            ch->setBaseStat<int>("grapple_type", 3);
            GRAPPLED(vict) = ch;
            vict->setBaseStat<int>("grapple_type", 3);
            /* Let's grapple! */

            vict->player_flags.set(PLR_THANDW, false);

            ch->modCurVital(CharVital::stamina, -cost);
            improve_skill(ch, SKILL_GRAPPLE, 1);
            WAIT_STATE(ch, PULSE_4SEC);
            return;
        }
    }
    else
    { /* You need to learn proper syntax! */
        ch->sendText("Syntax: grapple (target) (hold | choke | grab | wrap)\r\n");
        return;
    }
}

ACMD(do_trip)
{

    char arg[200];
    Character *vict = nullptr;

    one_argument(argument, arg);

    if (!check_skill(ch, SKILL_TRIP) && !IS_NPC(ch))
    {
        return;
    }

    int cost = GET_MAX_HIT(ch) / 200;

    if (cost > (ch->getCurVital(CharVital::stamina)))
    {
        ch->sendText("You don't have enough stamina.\r\n");
        return;
    }

    int perc = init_skill(ch, SKILL_TRIP), prob = rand_number(1, 114);

    if (perc == 0)
    {
        perc = GET_DEX(ch) + rand_number(1, 10);
    }

    vict = nullptr;
    if (!*arg || !(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        if (FIGHTING(ch) && FIGHTING(ch)->location == ch->location)
        {
            vict = FIGHTING(ch);
        }
        else
        {
            ch->sendText("That target isn't here.\r\n");
            return;
        }
    }

    if (!can_kill(ch, vict, nullptr, 0))
    {
        return;
    }

    if (vict)
    {
        if (AFF_FLAGGED(vict, AFF_FLYING))
        {
            ch->sendText("They are flying and are not on their feet!\r\n");
            return;
        }
        if (GET_POS(vict) == POS_SITTING)
        {
            ch->sendText("They are not on their feet!\r\n");
            return;
        }
        if (PLR_FLAGGED(vict, PLR_HEALT))
        {
            ch->sendText("They are inside a healing tank!\r\n");
            return;
        }

        if (GET_SPEEDI(ch) > GET_SPEEDI(vict) * 2)
        {
            perc += 5;
        }
        else if (GET_SPEEDI(ch) > GET_SPEEDI(vict))
        {
            perc += 2;
        }
        else if (GET_SPEEDI(ch) * 2 < GET_SPEEDI(vict))
        {
            perc -= 5;
        }
        else if (GET_SPEEDI(ch) < GET_SPEEDI(vict))
        {
            perc -= 2;
        }

        if (perc < prob)
        { /* Fail! */
            reveal_hiding(ch, 0);
            act("@mYou move to trip $N@m, but you screw up and $E keeps $S footing!@n", true, ch, nullptr, vict,
                TO_CHAR);
            act("@m$n@m moves to trip YOU, but $e screws up and you manage to keep your footing!@n", true, ch, nullptr,
                vict, TO_VICT);
            act("@m$n@m moves to trip $N@m, but $e screws up and $N@m manages to keep $S footing!@n", true, ch, nullptr,
                vict, TO_NOTVICT);
            improve_skill(ch, SKILL_TRIP, 0);
            ch->modCurVital(CharVital::stamina, -cost);
            WAIT_STATE(ch, PULSE_4SEC);
            if (FIGHTING(ch) == nullptr)
            {
                set_fighting(ch, vict);
            }
            else if (FIGHTING(ch) != vict)
            {
                set_fighting(ch, vict);
            }
            if (FIGHTING(vict) == nullptr)
            {
                set_fighting(vict, ch);
            }
            else if (FIGHTING(vict) != ch)
            {
                set_fighting(vict, ch);
            }
            return;
        }
        else
        { /* Success! */
            reveal_hiding(ch, 0);
            act("@mYou move to trip $N@m, and manage to knock $M off $S feet!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@m$n@m moves to trip YOU, and manages to knock you off your feet, delaying you!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@m$n@m moves to trip $N@m, and manages to knock $N@m off $S feet!@n", true, ch, nullptr, vict,
                TO_NOTVICT);
            improve_skill(ch, SKILL_TRIP, 0);
            ch->modCurVital(CharVital::stamina, -cost);
            WAIT_STATE(ch, PULSE_4SEC);
            WAIT_STATE(vict, vict->getBaseStat<int>("wait") + PULSE_4SEC * 2);
            if (FIGHTING(ch) == nullptr)
            {
                set_fighting(ch, vict);
            }
            if (FIGHTING(vict) == nullptr)
            {
                set_fighting(vict, ch);
            }
            return;
        }
    }
    else
    {
        ch->sendText("ERROR: Report to Iovan.\r\n");
        return;
    }
}

ACMD(do_train)
{

    if (IS_NPC(ch))
    {
        return;
    }

    if (ch->getBaseStat("burden_ratio") >= 1.0)
    {
        ch->sendText("You are weighted down too much!\r\n");
        return;
    }

    int plus = 0;
    int64_t total = 0, weight = 0, bonus = 0, cost = 0;
    char arg[200];

    one_argument(argument, arg);

    weight = ch->getEffectiveStat("weight_carried");

    int strcap = 5000, spdcap = 5000, intcap = 5000, wiscap = 5000, concap = 5000, aglcap = 5000;

    strcap += 500 * ch->getBaseStat("strength");
    intcap += 500 * ch->getBaseStat("intelligence");
    wiscap += 500 * ch->getBaseStat("wisdom");
    spdcap += 500 * ch->getBaseStat("speed");
    concap += 500 * ch->getBaseStat("constitution");
    aglcap += 500 * ch->getBaseStat("agility");

    if (IS_HUMAN(ch))
    {
        intcap = intcap * 0.75;
        wiscap = wiscap * 0.75;
    }
    else if (IS_KANASSAN(ch))
    {
        intcap = intcap * 0.4;
        wiscap = wiscap * 0.4;
        aglcap = aglcap * 0.4;
    }
    else if (IS_HALFBREED(ch))
    {
        intcap = intcap * 0.75;
        strcap = strcap * 0.75;
    }
    else if (IS_TRUFFLE(ch))
    {
        strcap = strcap * 1.5;
        concap = concap * 1.5;
    }

    if (!*arg)
    {
        ch->sendText("@D-------------[ @GTraining Status @D]-------------@n\r\n");
        ch->send_to("  @mStrength Progress    @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINSTR(ch)).c_str(), ch->getBaseStat("strength") >= 80 ? "@rCAPPED" : add_commas(strcap).c_str());
        ch->send_to("  @mSpeed Progress       @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINSPD(ch)).c_str(), ch->getBaseStat("speed") >= 80 ? "@rCAPPED" : add_commas(spdcap).c_str());
        ch->send_to("  @mConstitution Progress@D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINCON(ch)).c_str(), ch->getBaseStat("constitution") >= 80 ? "@rCAPPED" : add_commas(concap).c_str());
        ch->send_to("  @mIntelligence Progress@D: @R%6s/%6s@n\r\n", add_commas(GET_TRAININT(ch)).c_str(), ch->getBaseStat("intelligence") >= 80 ? "@rCAPPED" : add_commas(intcap).c_str());
        ch->send_to("  @mWisdom Progress      @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINWIS(ch)).c_str(), ch->getBaseStat("wisdom") >= 80 ? "@rCAPPED" : add_commas(wiscap).c_str());
        ch->send_to("  @mAgility Progress     @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINAGL(ch)).c_str(), ch->getBaseStat("agility") >= 80 ? "@rCAPPED" : add_commas(aglcap).c_str());
        ch->sendText("@D  -----------------------------------------  @n\r\n");
        ch->send_to("  @CCurrent Weight Held  @D: @c%s@n\r\n", add_commas(weight).c_str());
        ch->sendText("@D---------------------------------------------@n\r\n");
        ch->sendText("Syntax: train (str | spd | agl | wis | int | con)\r\n");
        return;
    }

    /* Figure up the weight bonus */
    auto ratio = ch->getBaseStat("burden_ratio");
    auto chCon = GET_CON(ch);
    total = chCon * 6;
    total += total * ratio;

    if (ch->location.getVnum() >= 6100 && ch->location.getVnum() <= 6135)
    {
        total += total * 0.15;
    }

    auto sensei = ch->sensei;
    bool senseiPresent = false;

    if (ch->location.getVnum() == sensei::getLocation(sensei))
    {
        senseiPresent = true;
        if (!(GET_GOLD(ch) >= 8 && GET_PRACTICES(ch) >= 1))
        {
            ch->sendText("It costs 8 Zenni and 1 PS to train with your sensei.\r\n");
            return;
        }
        total += total * 0.85;
        if (chCon >= 100)
            total *= 15000;
        else if (chCon >= 80)
            total *= 1500;
        else if (chCon >= 40)
            total *= 600;
        else if (chCon >= 20)
            total *= 300;
        else if (chCon >= 10)
            total *= 150;
        ch->send_to("@G%s begins to instruct you in training technique.@n\r\n", sensei::getName(sensei).c_str());
    }

    if (total > GET_MAX_HIT(ch) * 2)
    {
        bonus = 5;
    }
    else if (total > GET_MAX_HIT(ch))
    {
        bonus = 4;
    }
    else if (total > (GET_MAX_HIT(ch) / 2))
    {
        bonus = 3;
    }
    else if (total > (GET_MAX_HIT(ch) / 4))
    {
        bonus = 2;
    }
    else if (total > (GET_MAX_HIT(ch) / 8))
    {
        bonus = 1;
    }

    if (!senseiPresent)
        cost = ((total / 20) + (GET_MAX_MOVE(ch) / 50));
    else
        cost = ((total / 25) + (GET_MAX_MOVE(ch) / 60));

    cost += cost * ratio;

    if (GET_BONUS(ch, BONUS_HARDWORKER))
    {
        cost -= cost * 0.25;
    }

    if (GET_RELAXCOUNT(ch) >= 464)
    {
        cost *= 10;
    }
    else if (GET_RELAXCOUNT(ch) >= 232)
    {
        cost *= 5;
    }
    else if (GET_RELAXCOUNT(ch) >= 116)
    {
        cost *= 2;
    }

    CharAttribute attr;
    CharTrain train;
    std::string stat_name{};
    int bonus_trait = -1;
    int nega_trait = -1;
    int needed = 0;

    if (!strcasecmp("str", arg))
    {
        attr = CharAttribute::strength;
        train = CharTrain::strength;
        stat_name = "strength";
        bonus_trait = BONUS_BRAWNY;
        nega_trait = BONUS_WIMP;
        needed = strcap;
    }
    else if (!strcasecmp("spd", arg))
    {
        attr = CharAttribute::speed;
        train = CharTrain::speed;
        stat_name = "speed";
        bonus_trait = BONUS_QUICK;
        nega_trait = BONUS_SLOW;
        needed = spdcap;
    }
    else if (!strcasecmp("con", arg))
    {
        attr = CharAttribute::constitution;
        train = CharTrain::constitution;
        stat_name = "constitution";
        bonus_trait = BONUS_STURDY;
        nega_trait = BONUS_FRAIL;
        needed = concap;
    }
    else if (!strcasecmp("agl", arg))
    {
        attr = CharAttribute::agility;
        train = CharTrain::agility;
        stat_name = "agility";
        bonus_trait = BONUS_AGILE;
        nega_trait = BONUS_CLUMSY;
        needed = aglcap;
    }
    else if (!strcasecmp("int", arg))
    {
        attr = CharAttribute::intelligence;
        train = CharTrain::intelligence;
        stat_name = "intelligence";
        bonus_trait = BONUS_SCHOLARLY;
        nega_trait = BONUS_DULL;
        needed = intcap;
    }
    else if (!strcasecmp("wis", arg))
    {
        attr = CharAttribute::wisdom;
        train = CharTrain::wisdom;
        stat_name = "wisdom";
        bonus_trait = BONUS_SAGE;
        nega_trait = BONUS_FOOLISH;
        needed = wiscap;
    }
    else
    {
        ch->sendText("Syntax: train (str | spd | agl | wis | int | con)\r\n");
        return;
    }

    auto stat_val = ch->getBaseStat(stat_name);

    if (stat_val == 80)
    {
        ch->send_to("Your base %s is maxed!\r\n", stat_name);
        return;
    }

    if (stat_val >= 45 && GET_BONUS(ch, nega_trait) > 0)
    {
        ch->send_to("You're not able to withstand increasing your %s beyond 45.\r\n", stat_name);
        return;
    }

    /* what training message is displayed? */
    reveal_hiding(ch, 0);

    Task task;

    switch (attr)
    {
    case CharAttribute::strength:
        task = Task::trainStr;
        break;
    case CharAttribute::agility:
        task = Task::trainAgl;
        break;
    case CharAttribute::constitution:
        task = Task::trainCon;
        break;
    case CharAttribute::speed:
        task = Task::trainSpd;
        break;
    case CharAttribute::intelligence:
        task = Task::trainInt;
        break;
    case CharAttribute::wisdom:
        task = Task::trainWis;
        break;
    }
    ch->setTask(task);
    WAIT_STATE(ch, PULSE_5SEC * 6);
}

void trainProgress(Character *ch)
{
    if (ch->getBaseStat("burden_ratio") >= 1.0)
    {
        ch->sendText("You are weighted down too much!\r\n");
        return;
    }

    int plus = 0;
    int64_t total = 0, weight = 0, bonus = 0, cost = 0;

    weight = ch->getEffectiveStat("weight_carried");

    int strcap = 5000, spdcap = 5000, intcap = 5000, wiscap = 5000, concap = 5000, aglcap = 5000;

    strcap += 500 * ch->getBaseStat("strength");
    intcap += 500 * ch->getBaseStat("intelligence");
    wiscap += 500 * ch->getBaseStat("wisdom");
    spdcap += 500 * ch->getBaseStat("speed");
    concap += 500 * ch->getBaseStat("constitution");
    aglcap += 500 * ch->getBaseStat("agility");

    if (IS_HUMAN(ch))
    {
        intcap = intcap * 0.75;
        wiscap = wiscap * 0.75;
    }
    else if (IS_KANASSAN(ch))
    {
        intcap = intcap * 0.4;
        wiscap = wiscap * 0.4;
        aglcap = aglcap * 0.4;
    }
    else if (IS_HALFBREED(ch))
    {
        intcap = intcap * 0.75;
        strcap = strcap * 0.75;
    }
    else if (IS_TRUFFLE(ch))
    {
        strcap = strcap * 1.5;
        concap = concap * 1.5;
    }

    /* Figure up the weight bonus */
    auto ratio = ch->getBaseStat("burden_ratio");
    auto chCon = GET_CON(ch);
    total = chCon * 6;
    total += total * ratio;

    if (ch->location.getVnum() >= 6100 && ch->location.getVnum() <= 6135)
    {
        total += total * 0.15;
    }

    auto sensei = ch->sensei;
    bool senseiPresent = false;

    if (ch->location.getVnum() == sensei::getLocation(sensei))
    {
        senseiPresent = true;
        if (!(GET_GOLD(ch) >= 8 && GET_PRACTICES(ch) >= 1))
        {
            ch->sendText("It costs 8 Zenni and 1 PS to train with your sensei.\r\n");
            return;
        }
        total += total * 0.85;
        if (chCon >= 100)
            total *= 15000;
        else if (chCon >= 80)
            total *= 1500;
        else if (chCon >= 40)
            total *= 600;
        else if (chCon >= 20)
            total *= 300;
        else if (chCon >= 10)
            total *= 150;
        ch->send_to("@G%s begins to instruct you in training technique.@n\r\n", sensei::getName(sensei).c_str());
    }

    if (total > GET_MAX_HIT(ch) * 2)
    {
        bonus = 5;
    }
    else if (total > GET_MAX_HIT(ch))
    {
        bonus = 4;
    }
    else if (total > (GET_MAX_HIT(ch) / 2))
    {
        bonus = 3;
    }
    else if (total > (GET_MAX_HIT(ch) / 4))
    {
        bonus = 2;
    }
    else if (total > (GET_MAX_HIT(ch) / 8))
    {
        bonus = 1;
    }

    if (!senseiPresent)
        cost = ((total / 20) + (GET_MAX_MOVE(ch) / 50));
    else
        cost = ((total / 25) + (GET_MAX_MOVE(ch) / 60));

    cost += cost * ratio;

    if (GET_BONUS(ch, BONUS_HARDWORKER))
    {
        cost -= cost * 0.25;
    }

    if (GET_RELAXCOUNT(ch) >= 464)
    {
        cost *= 10;
    }
    else if (GET_RELAXCOUNT(ch) >= 232)
    {
        cost *= 5;
    }
    else if (GET_RELAXCOUNT(ch) >= 116)
    {
        cost *= 2;
    }

    CharAttribute attr;
    CharTrain train;
    std::string stat_name{};
    int bonus_trait = -1;
    int nega_trait = -1;
    int needed = 0;

    auto trainType = ch->task;

    if (trainType == Task::trainStr)
    {
        attr = CharAttribute::strength;
        train = CharTrain::strength;
        stat_name = "strength";
        bonus_trait = BONUS_BRAWNY;
        nega_trait = BONUS_WIMP;
        needed = strcap;
    }
    else if (trainType == Task::trainSpd)
    {
        attr = CharAttribute::speed;
        train = CharTrain::speed;
        stat_name = "speed";
        bonus_trait = BONUS_QUICK;
        nega_trait = BONUS_SLOW;
        needed = spdcap;
    }
    else if (trainType == Task::trainCon)
    {
        attr = CharAttribute::constitution;
        train = CharTrain::constitution;
        stat_name = "constitution";
        bonus_trait = BONUS_STURDY;
        nega_trait = BONUS_FRAIL;
        needed = concap;
    }
    else if (trainType == Task::trainAgl)
    {
        attr = CharAttribute::agility;
        train = CharTrain::agility;
        stat_name = "agility";
        bonus_trait = BONUS_AGILE;
        nega_trait = BONUS_CLUMSY;
        needed = aglcap;
    }
    else if (trainType == Task::trainInt)
    {
        attr = CharAttribute::intelligence;
        train = CharTrain::intelligence;
        stat_name = "intelligence";
        bonus_trait = BONUS_SCHOLARLY;
        nega_trait = BONUS_DULL;
        needed = intcap;
    }
    else if (trainType == Task::trainWis)
    {
        attr = CharAttribute::wisdom;
        train = CharTrain::wisdom;
        stat_name = "wisdom";
        bonus_trait = BONUS_SAGE;
        nega_trait = BONUS_FOOLISH;
        needed = wiscap;
    }
    else
    {
        ch->sendText("Invalid\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    auto stat_val = ch->getBaseStat(stat_name);

    if (stat_val == 80)
    {
        ch->send_to("Your base %s is maxed!\r\n", stat_name);
        ch->setTask(Task::nothing);
        return;
    }

    if (stat_val >= 45 && GET_BONUS(ch, nega_trait) > 0)
    {
        ch->send_to("You're not able to withstand increasing your %s beyond 45.\r\n", stat_name);
        ch->setTask(Task::nothing);
        return;
    }

    switch (attr)
    {
    case CharAttribute::strength:
    case CharAttribute::agility:
    case CharAttribute::constitution:
    case CharAttribute::speed:
        if ((ch->getCurVital(CharVital::stamina)) < cost)
        {
            ch->sendText("You do not have enough stamina with the current weight worn and gravity!\r\n");
            ch->setTask(Task::nothing);
            return;
        }
        plus = (((total / 20) + (GET_MAX_MOVE(ch) / 50)) * 100) / GET_MAX_MOVE(ch);
        ch->modCurVital(CharVital::stamina, -cost);
        break;
    case CharAttribute::intelligence:
    case CharAttribute::wisdom:
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->sendText("You do not have enough ki with the current weight worn and gravity!\r\n");
            ch->setTask(Task::nothing);
            return;
        }
        plus = (((total / 20) + (GET_MAX_MANA(ch) / 50)) * 100) / GET_MAX_MANA(ch);
        ch->modCurVital(CharVital::ki, -cost);
        break;
    }

    /* what training message is displayed? */
    reveal_hiding(ch, 0);

    auto msg_case = rand_number(1, 3);

    switch (attr)
    {
    case CharAttribute::strength:
        switch (msg_case)
        {
        case 1:
            act("@WYou throw a flurry of punches into the air at an invisible opponent.@n", true, ch, nullptr,
                nullptr, TO_CHAR);
            act("@W$n throws a flurry of punches into the air.@n", true, ch, nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            act("@WYou leap into the air and throw a wild kick at an invisible opponent@n", true, ch, nullptr,
                nullptr, TO_CHAR);
            act("@W$n leaps into the air and throws a wild kick at nothing.@n", true, ch, nullptr, nullptr,
                TO_ROOM);
            break;
        case 3:
            act("@WYou leap high into the air and unleash a flurry of punches and kicks at an invisible opponent@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n leaps high into the air and unleashes a flurry of punches and kicks at nothing.@n", true,
                ch, nullptr, nullptr, TO_ROOM);
            break;
        }
        break;
    case CharAttribute::agility:
        switch (msg_case)
        {
        case 1:
            act("@WYou dash quickly around the surrounding area as fast as you can!@n", true, ch, nullptr,
                nullptr, TO_CHAR);
            act("@W$n dashes quickly around the surrounding area as fast as $e can!@n", true, ch, nullptr,
                nullptr, TO_ROOM);
            break;
        case 2:
            act("@WYou dodge to the side as fast as you can!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n dodges to the side as fast as $e can!@n", true, ch, nullptr, nullptr, TO_ROOM);
            break;
        case 3:
            act("@WYou dash backwards as fast as you can!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n dashes backwards as fast as $e can!@n", true, ch, nullptr, nullptr, TO_ROOM);
            break;
        }
        break;
    case CharAttribute::constitution:
        switch (msg_case)
        {
        case 1:
            act("@WYou leap into the air and then slam into the ground with your feet outstretched!@n", true,
                ch, nullptr, nullptr, TO_CHAR);
            act("@W$n leaps into the air and then slams into the ground with $s feet outstretched!?@n", true,
                ch, nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            act("@WYou leap into the air and then slam into the ground with your fists!@n", true, ch, nullptr,
                nullptr, TO_CHAR);
            act("@W$n leaps into the air and then slams into the ground with $s fists!?@n", true, ch, nullptr,
                nullptr, TO_ROOM);
            break;
        case 3:
            act("@WYou leap into the air and then slam into the ground with your body!@n", true, ch, nullptr,
                nullptr, TO_CHAR);
            act("@W$n leaps into the air and then slams into the ground with $s body!?@n", true, ch, nullptr,
                nullptr, TO_ROOM);
            break;
        }
        break;
    case CharAttribute::speed:
        switch (msg_case)
        {
        case 1:
            act("@WYou do a series of backflips through the air, landing gracefully on one foot a moment later.@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n does a series of backflips through the air, landing gracefully on one foot a moment later.@n",
                true, ch, nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            act("@WYou flip forward and launch off your hands into the air. You land gracefully on one foot a moment later.@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n flips forward and launches off $s hands into the air. Then $e lands gracefully on one foot a moment later.@n",
                true, ch, nullptr, nullptr, TO_ROOM);
            break;
        case 3:
            act("@WYou flip to the side off one hand and then land on your feet.@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@W$n flips to the side off one hand and then lands on $s feet.@n", true, ch, nullptr, nullptr,
                TO_ROOM);
            break;
        }
        break;
    case CharAttribute::intelligence:
        switch (msg_case)
        {
        case 1:
            act("@WConcentrating you fly high into the air as fast as you can before settling slowly back to the ground.@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n flies high into the air as fast as $e can before settling slowly back to the ground.@n",
                true, ch, nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            act("@WYou focus your ki at your outstretched hand and send a mild shockwave in that direction!@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n focuses $s ki at $s outstretched hand and sends a mild shockwave in that direction!@n",
                true, ch, nullptr, nullptr, TO_ROOM);
            break;
        case 3:
            act("@WYou concentrate on your ki and force torrents of it to rush out from your body randomly!@n",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("@W$n seems to concentrate before torrents of ki randomly blasts out from $s body!@n", true, ch,
                nullptr, nullptr, TO_ROOM);
            break;
        }
        break;
    case CharAttribute::wisdom:
        switch (msg_case)
        {
        case 1:
            act("@WYou close your eyes and wage a mental battle against an imaginary opponent.@n", true, ch,
                nullptr, nullptr, TO_CHAR);
            act("@W$n closes $s eyes for a moment and an expression of intensity forms on it.@n", true, ch,
                nullptr, nullptr, TO_ROOM);
            break;
        case 2:
            act("@WYou look around and contemplate battle tactics for an imaginary scenario.@n", true, ch,
                nullptr, nullptr, TO_CHAR);
            act("@W$n looks around and appears to be imagining things that aren't there.@n", true, ch, nullptr,
                nullptr, TO_ROOM);
            break;
        case 3:
            act("@WYou invent a battle plan for a battle that doesn't exist!@n", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("@W$n seems to have thought of something.@n", true, ch, nullptr, nullptr, TO_ROOM);
            break;
        }
        break;
    }

    plus += 75;

    if (ch->location.getVnum() >= 19800 && ch->location.getVnum() <= 19899)
    {
        plus *= 4;
    }
    if (ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
    {
        plus *= 3;
    }
    if (GET_BONUS(ch, BONUS_HARDWORKER))
    {
        plus += plus * 0.25;
    }
    if (GET_BONUS(ch, bonus_trait))
    {
        plus += plus * 0.75;
    }
    if (GET_BONUS(ch, BONUS_LONER))
    {
        plus += plus * 0.05;
    }
    if (senseiPresent)
    {
        plus += plus * 0.2;
    }

    int stat_train = 0;

    switch (bonus)
    {
    case 1:
        plus = (plus + 5) * 2;
        stat_train += plus;
        ch->send_to("You feel slight improvement. @D[@G+%d@D]@n\r\n", plus);
        break;
    case 2:
        plus = (plus + 10) * 2;
        stat_train += plus;
        ch->send_to("You feel some improvement. @D[@G+%d@D]@n\r\n", plus);
        break;
    case 3:
        plus = (plus + 25) * 2;
        stat_train += plus;
        ch->send_to("You feel good improvement. @D[@G+%d@D]@n\r\n", plus);
        break;
    case 4:
        plus = (plus + 50) * 2;
        stat_train += plus;
        ch->send_to("You feel great improvement! @D[@G+%d@D]@n\r\n", plus);
        break;
    case 5:
        plus = (plus + 100) * 2;
        stat_train += plus;
        ch->send_to("You feel awesome improvement! @D[@G+%d@D]@n\r\n", plus);
        break;
    default:
        stat_train += 1;
        ch->sendText("You barely feel any improvement. @D[@G+1@D]@n\r\n");
        break;
    }

    WAIT_STATE(ch, PULSE_5SEC * 6);

    if (senseiPresent)
    {
        ch->modBaseStat("money_carried", -8);
        ch->modPractices(-1);
    }

    auto train_name = "train_" + stat_name;

    auto results = ch->modBaseStat<int>(train_name, stat_train);

    if (results >= needed)
    {
        ch->modBaseStat(train_name, -needed);
        ch->send_to("You feel your %s improve!@n\r\n", stat_name);
        ch->modBaseStat(stat_name, 1);
        if (IS_PICCOLO(ch) && IS_NAMEK(ch))
        {
            giveRandomVital(ch, ch->getEffectiveStat<int64_t>("health") / 5, ch->getEffectiveStat<int64_t>("ki") / 5, ch->getEffectiveStat<int64_t>("stamina") / 5, 30);
            ch->sendText("You gained quite a bit of experience from that!\r\n");
        }
    }
}

ACMD(do_rip)
{
    Character *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (IS_NPC(ch))
        return;

    if (!*arg)
    {
        ch->sendText("Rip the tail off who?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("That target isn't here.\r\n");
        return;
    }

    if (!vict->hasTail())
    {
        ch->sendText("They do not have a tail to rip off!\r\n");
        return;
    }

    if (ch != vict && GET_POS(ch) > POS_SLEEPING)
    {
        if ((ch->getCurVital(CharVital::stamina)) < GET_MAX_MOVE(ch) / 20)
        {
            ch->sendText("You are too tired to manage to grab their tail!\r\n");
            return;
        }
        else if (GET_SPEEDI(ch) > GET_SPEEDI(vict))
        {
            ch->modCurVital(CharVital::stamina, -(ch->getEffectiveStat<int64_t>("stamina") / 20));
            if (GET_HIT(ch) > GET_HIT(vict) * 2)
            {
                reveal_hiding(ch, 0);
                act("@rYou rush at @R$N@r and grab $S tail! With a powerful tug you pull it off!@n", true, ch, nullptr,
                    vict, TO_CHAR);
                act("@R$n@r rushes at YOU and grabs your tail! With a powerful tug $e pulls it off!@n", true, ch,
                    nullptr, vict, TO_VICT);
                act("@R$n@R rushes at @R$N@r and grab $S tail! With a powerful tug $e pulls it off!@n", true, ch,
                    nullptr, vict, TO_NOTVICT);
                vict->loseTail();
                return;
            }
            else
            {
                reveal_hiding(ch, 0);
                act("@rYou rush at @R$N@r and grab $S tail! You are too weak to pull it off though!@n", true, ch,
                    nullptr, vict, TO_CHAR);
                act("@R$n@r rushes at YOU and grabs your tail! $e is too weak to pull it off though!@n", true, ch,
                    nullptr, vict, TO_VICT);
                act("@R$n@R rushes at @R$N@r and grab $S tail! $e is too weak to pull it off though!@n", true, ch,
                    nullptr, vict, TO_NOTVICT);
                return;
            }
        }
        else
        {
            ch->modCurVital(CharVital::stamina, -(ch->getEffectiveStat<int64_t>("stamina") / 20));
            reveal_hiding(ch, 0);
            act("@rYou rush at @R$N@r and try to grab $S tail, but fail!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@R$n@r rushes at YOU and tries to grab your tail, but fails!@n", true, ch, nullptr, vict, TO_VICT);
            act("@R$n@R rushes at @R$N@r and tries to grab $S tail, but fails!@n", true, ch, nullptr, vict, TO_NOTVICT);
            return;
        }
    }
    else if (ch == vict)
    {
        reveal_hiding(ch, 0);
        act("@rYou grab your own tail and yank it off!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@R$n@r grabs $s own tail and yanks it off!@n", true, ch, nullptr, nullptr, TO_ROOM);
        vict->loseTail();
    }
    else
    {
        if ((ch->getCurVital(CharVital::stamina)) < GET_MAX_MOVE(ch) / 20)
        {
            ch->sendText("You are too tired to manage to grab their tail!\r\n");
            return;
        }
        ch->modCurVital(CharVital::stamina, -(ch->getEffectiveStat<int64_t>("stamina") / 20));
        reveal_hiding(ch, 0);
        act("@rYou reach and grab @R$N's@r tail! With a powerful tug you pull it off!@n", true, ch, nullptr, vict,
            TO_CHAR);
        act("@RYou feel your tail pulled off!@n", true, ch, nullptr, vict, TO_VICT);
        act("@R$n@R reaches and grabs @R$N's@r tail! With a powerful tug $e pulls it off!@n", true, ch, nullptr, vict,
            TO_NOTVICT);
        vict->loseTail();
        return;
    }
}

ACMD(do_infuse)
{

    if (!know_skill(ch, SKILL_INFUSE))
    {
        return;
    }

    if (AFF_FLAGGED(ch, AFF_INFUSE))
    {
        act("You stop infusing ki into your attacks.", true, ch, nullptr, nullptr, TO_CHAR);
        act("$n stops infusing ki into $s attacks.", true, ch, nullptr, nullptr, TO_ROOM);
        ch->affect_flags.set(AFF_INFUSE, false);
        return;
    }

    if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 100)
    {
        ch->sendText("You don't have enough ki to infuse into your attacks!\r\n");
        return;
    }
    reveal_hiding(ch, 0);
    act("You start infusing ki into your attacks.", true, ch, nullptr, nullptr, TO_CHAR);
    act("$n starts infusing ki into $s attacks.", true, ch, nullptr, nullptr, TO_ROOM);
    ch->affect_flags.set(AFF_INFUSE, true);
    ch->modCurVitalDam(CharVital::ki, 0.01);
}

ACMD(do_paralyze)
{
    Character *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!know_skill(ch, SKILL_PARALYZE))
    {
        return;
    }

    if (!*arg)
    {
        ch->sendText("Who are you wanting to paralyze?\r\n");
        return;
    }

    if (!limb_ok(ch, 0))
    {
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("That target isn't here.\r\n");
        return;
    }
    if (!can_kill(ch, vict, nullptr, 0))
    {
        return;
    }
    if (AFF_FLAGGED(vict, AFF_PARA))
    {
        ch->sendText("They are already partially paralyzed!\r\n");
        return;
    }

    if ((ch->getCurVital(CharVital::ki)) < GET_HIT(vict) / 10 + (GET_MAX_MANA(ch) / 20))
    {
        ch->sendText("You realize you can't paralyze them. You don't have enough ki to restrain them!\r\n");
        return;
    }

    int prob = GET_SKILL(ch, SKILL_PARALYZE), perc = axion_dice(0);

    if (GET_SPEEDI(ch) * 2 < GET_SPEEDI(vict))
    {
        prob -= 10;
    }
    if (GET_SPEEDI(ch) + (GET_SPEEDI(ch) / 2) < GET_SPEEDI(vict))
    {
        prob -= 5;
    }

    if (GET_BONUS(vict, BONUS_INSOMNIAC))
    {
        ch->modCurVital(CharVital::ki, -(GET_HIT(vict) / 6 + (GET_MAX_MANA(ch) / 20)));
        act("@RYou focus ki and point both your arms at @r$N@R. However $N seems to shake off your paralysis attack!@n",
            true, ch, nullptr, vict, TO_CHAR);
        act("@r$n @Rfocuses ki and points both $s arms at YOU! Your insomnia makes you immune to $s feeble paralysis attempt.@n",
            true, ch, nullptr, vict, TO_VICT);
        act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $N seems to shake off $s paralysis attack!@n",
            true, ch, nullptr, vict, TO_NOTVICT);
        return;
    }
    else if (prob < perc)
    {
        reveal_hiding(ch, 0);
        act("@RYou focus ki and point both your arms at @r$N@R. However $E manages to avoid your attempt to paralyze $M!@n",
            true, ch, nullptr, vict, TO_CHAR);
        act("@r$n @Rfocuses ki and points both $s arms at YOU! You manage to avoid $s technique though...@n", true, ch,
            nullptr, vict, TO_VICT);
        act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $E manages to avoid @r$n's@R attempted technique...@n",
            true, ch, nullptr, vict, TO_NOTVICT);
        ch->modCurVital(CharVital::ki, -(GET_HIT(vict) / 6 + (GET_MAX_MANA(ch) / 20)));
        improve_skill(ch, SKILL_PARALYZE, 0);
    }
    else
    {
        reveal_hiding(ch, 0);
        act("@RYou focus ki and point both your arms at @r$N@R. Your ki flows into $S body and partially paralyzes $M!@n",
            true, ch, nullptr, vict, TO_CHAR);
        act("@r$n @Rfocuses ki and points both $s arms at YOU! You are caught in $s paralysis technique and now can barely move!@n",
            true, ch, nullptr, vict, TO_VICT);
        act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. @r$n's@R ki flows into @r$N@R body and partially paralyzes $M!@n",
            true, ch, nullptr, vict, TO_NOTVICT);
        int duration = GET_INT(ch) / 15;
        assign_affect(vict, AFF_PARA, SKILL_PARALYZE, duration, 0, 0, 0, 0, 0, 0);
        ch->modCurVital(CharVital::ki, -(GET_HIT(vict) / 6 + (GET_MAX_MANA(ch) / 20)));
        improve_skill(ch, SKILL_PARALYZE, 0);
    }
}

ACMD(do_taisha)
{

    if (IS_NPC(ch))
        return;

    if (!know_skill(ch, SKILL_TAISHA))
    {
        return;
    }

    if (ch->location.getRoomFlag(ROOM_AURA))
    {
        ch->sendText("This area already has an aura of regeneration around it.\r\n");
        return;
    }

    if (ch->currentGravity() > 1.0)
    {
        ch->sendText("This area's gravity is too hostile to an aura.\r\n");
        return;
    }

    if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 3)
    {
        ch->sendText("You don't have enough ki.\r\n");
        return;
    }

    int prob = GET_SKILL(ch, SKILL_TAISHA), perc = axion_dice(0);

    ch->modCurVitalDam(CharVital::ki, 0.33);
    if (prob < perc)
    {
        reveal_hiding(ch, 0);
        act("@WYou hold up your hands while channeling ki. Your technique fails to produce an aura though....@n", true,
            ch, nullptr, nullptr, TO_CHAR);
        act("@g$n@W holds up $s hands while channeling ki. $s technique fails to produce an aura though....", true, ch,
            nullptr, nullptr, TO_ROOM);
        improve_skill(ch, SKILL_TAISHA, 1);
        return;
    }
    else
    {
        reveal_hiding(ch, 0);
        act("@WYou hold up your hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n holds up $s hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        improve_skill(ch, SKILL_TAISHA, 1);
        ch->location.setRoomFlag(ROOM_AURA, true);
        return;
    }
}

ACMD(do_kura)
{
    if (!know_skill(ch, SKILL_KURA))
    {
        return;
    }

    if ((ch->getCurVital(CharVital::ki)) >= GET_MAX_MANA(ch))
    {
        ch->sendText("Your ki is already maxed out!\r\n");
        return;
    }

    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Syntax: kuraiiro (1-100).\r\n");
        return;
    }

    int num = atoi(arg);
    int skill = GET_SKILL(ch, SKILL_KURA);
    int64_t cost = 0, bonus = 0;

    if (num > skill)
    {
        ch->sendText("The number can not be greater than your skill.\r\n");
        return;
    }
    if (num <= 0)
    {
        ch->sendText("The number can not be less than 1.\r\n");
        return;
    }

    cost = (GET_MAX_MANA(ch) / 100) * num;
    bonus = cost;

    if ((ch->getCurVital(CharVital::lifeforce)) < cost)
    {
        ch->sendText("You do not have enough life force for that high a number.\r\n");
        return;
    }

    ch->modCurVital(CharVital::lifeforce, -cost);
    ch->modCurVital(CharVital::ki, bonus);
    reveal_hiding(ch, 0);
    act("You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies and succeed as a rush of energy explodes around you!",
        true, ch, nullptr, nullptr, TO_CHAR);
    act("@c$n@w crouches down and screams as $s eyes turn red. Suddenly $e manages to tap into dark energies and a rush of energy explodes around $m!",
        true, ch, nullptr, nullptr, TO_ROOM);
    improve_skill(ch, SKILL_KURA, 0);
    WAIT_STATE(ch, PULSE_1SEC);
}

ACMD(do_candy)
{

    Character *vict;
    Object *obj;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!IS_MAJIN(ch))
    {
        ch->sendText("You are not a Majin, how can you do that?\r\n");
        return;
    }
    // if (FIGHTING(ch)) {
    //     ch->sendText("You are too busy fighting!\r\n");
    // return;
    // }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("Turn who into candy?\r\n");
        return;
    }

    if (!can_kill(ch, vict, nullptr, 0))
    {
        return;
    }

    auto ch_max = ch->getPL();
    auto vict_max = vict->getPL();

    if (!IS_NPC(vict))
    {
        ch->sendText("You can't turn them into candy.\r\n");
        return;
    }

    if (vict_max > ch_max * 2)
    {
        ch->sendText("They are too powerful.\r\n");
        return;
    }

    if ((ch->getCurVital(CharVital::ki)) < ch_max / 15)
    {
        ch->sendText("You do not have enough ki.\r\n");
        return;
    }

    if (rand_number(1, 6) == 6)
    {
        ch->modCurVitalDam(CharVital::ki, 0.0667);
        reveal_hiding(ch, 0);
        act("@cYou aim your forelock at @R$N@c and fire a beam of energy but it is dodged!@n", true, ch, nullptr, vict,
            TO_CHAR);
        act("@C$n@c aims $s forelock at @R$N@c and fires a beam of energy but the beam is dodged!@n", true, ch, nullptr,
            vict, TO_NOTVICT);
        if (!FIGHTING(ch))
        {
            set_fighting(ch, vict);
        }
        if (!FIGHTING(vict))
        {
            set_fighting(vict, ch);
        }
        WAIT_STATE(ch, PULSE_3SEC);
        return;
    }
    ch->modCurVitalDam(CharVital::ki, 0.0667);
    reveal_hiding(ch, 0);
    act("@cYou aim your forelock at @R$N@c and fire a beam of energy that envelopes $S entire body and changes $M into candy!@n",
        true, ch, nullptr, vict, TO_CHAR);
    act("@C$n@c aims $s forelock at @R$N@c and fires a beam of energy that envelopes $S entire body and changes $M into candy!@n ",
        true, ch, nullptr, vict, TO_NOTVICT);

    std::vector<obj_vnum> candies = {53, 93, 94, 95};
    auto randomCandy = Random::get(candies);

    ch->sendText("You grab the candy as it falls.\r\n");
    obj = read_object(*randomCandy, VIRTUAL);
    auto sh = obj->getShortDescription();
    char newsh[MAX_STRING_LENGTH];
    snprintf(newsh, MAX_STRING_LENGTH, "%s@n (of %s@n)", sh, vict->getShortDescription());
    obj->strings["short_description"] = newsh;
    ch->addToInventory(obj);
    obj->setBaseStat<int64_t>(VAL_FOOD_CANDY_PL, vict->getBaseStat<int64_t>("health"));
    obj->setBaseStat<int64_t>(VAL_FOOD_CANDY_KI, vict->getBaseStat<int64_t>("ki"));
    obj->setBaseStat<int64_t>(VAL_FOOD_CANDY_ST, vict->getBaseStat<int64_t>("stamina"));

    vict->mob_flags.set(MOB_HUSK, false);
    die(vict, ch);
}

ACMD(do_future)
{
    char arg[MAX_INPUT_LENGTH];
    Character *vict = nullptr;
    one_argument(argument, arg);

    if (IS_NPC(ch) || !IS_KANASSAN(ch))
    {
        ch->sendText("You are incapable of this ability.\r\n");
        return;
    }

    if (!*arg)
    {
        ch->sendText("Bestow advance future sight on who?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("Bestow advance future sight on who?\r\n");
        return;
    }

    if (AFF_FLAGGED(vict, AFF_FUTURE))
    {
        ch->sendText("They already can see the future.\r\n");
        return;
    }

    if (IS_NPC(vict))
    {
        ch->sendText("You can't target them, there would be no point.\r\n");
        return;
    }

    if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 40)
    {
        ch->sendText("You do not have enough ki.\r\n");
        return;
    }

    if (GET_PRACTICES(ch) < 100)
    {
        ch->sendText("You do not have enough PS to activate or pass on this ability.\r\n");
        return;
    }

    ch->modCurVitalDam(CharVital::ki, 0.025);
    ch->modPractices(-100);
    reveal_hiding(ch, 0);

    if (vict != ch)
    {
        act("@CYou focus your energy into your fingers before stabbing your claws into $N and bestowing the power of Future Sight upon $M. Shortly after $E passes out.@n",
            true, ch, nullptr, vict, TO_CHAR);
        act("@C$n focuses $s energy into $s fingers before stabbing $s claws into YOUR neck and bestowing the power of Future Sight upon you! Soon after you pass out!@n",
            true, ch, nullptr, vict, TO_VICT);
        act("@C$n focuses $s energy into $s fingers before stabbing $s claws into $N's neck and bestowing the power of Future Sight upon $M! Soon after $E passes out!@n",
            true, ch, nullptr, vict, TO_NOTVICT);
    }
    else
    {

        act("@CYou focus your energy into your mind and awaken your latent Future Sight powers!@n", true, ch, nullptr,
            vict, TO_CHAR);
        act("@C$n focuses $s energy while closing $s eyes for a moment.@n", true, ch, nullptr, vict, TO_VICT);
        act("@C$n focuses $s energy while closing $s eyes for a moment.@n", true, ch, nullptr, vict, TO_NOTVICT);
    }

    assign_affect(vict, AFF_FUTURE, 0, -1, 0, 0, 2, 0, 0, 5);
    vict->setBaseStat("combo", POS_SLEEPING);
}

ACMD(do_drag)
{
    Character *vict = nullptr;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (IS_NPC(ch))
        return;

    if (DRAGGING(ch))
    {
        vict = DRAGGING(ch);
        DRAGGING(ch) = nullptr;
        DRAGGED(vict) = nullptr;
        act("@wYou stop dragging @C$N@W.@n", true, ch, nullptr, vict, TO_CHAR);
        act("@C$n@W stops dragging @c$N@W.@n", true, ch, nullptr, vict, TO_ROOM);
        return;
    }

    if (PLR_FLAGGED(ch, PLR_PILOTING))
    {
        ch->sendText("You are busy piloting a ship!\r\n");
        return;
    }

    if (CARRYING(ch))
    {
        ch->sendText("You are busy carrying someone at the moment.\r\n");
        return;
    }

    if (!*arg)
    {
        ch->sendText("Who do you want to drag?\r\n");
        return;
    }

    if (FIGHTING(ch))
    {
        ch->sendText("You are a bit busy fighting right now!\r\n");
        return;
    }

    const auto tile = ch->location.getTileType();

    if (tile == SECT_WATER_NOSWIM || tile == SECT_WATER_SWIM)
    {
        ch->sendText("You decide to not be a tugboat instead.\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("Drag who?\r\n");
        return;
    }

    if (vict == ch)
    {
        ch->sendText("You can't drag yourself.\r\n");
        return;
    }

    if (DRAGGED(vict))
    {
        ch->sendText("They are already being dragged!\r\n");
        return;
    }

    if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL))
    {
        ch->sendText("They are not to be touched!\r\n");
        return;
    }

    if (GET_POS(vict) != POS_SLEEPING)
    {
        reveal_hiding(ch, 0);
        act("@wYou try to grab and pull @C$N@W with you, but $E resists!@n", true, ch, nullptr, vict, TO_CHAR);
        act("@C$n@W tries to grab and pull you! However you resist!@n", true, ch, nullptr, vict, TO_VICT);
        act("@C$n@W tries to grab and pull @c$N@W but $E resists!@n", true, ch, nullptr, vict, TO_NOTVICT);
        if (IS_NPC(vict) && !FIGHTING(vict))
        {
            set_fighting(vict, ch);
        }
        return;
    }
    else if (!ch->canCarryWeight(vict))
    {
        reveal_hiding(ch, 0);
        act("@wYou try to grab and pull @C$N@W with you, but $E is too heavy!@n", true, ch, nullptr, vict, TO_CHAR);
        act("@C$n@W tries to grab and pull @c$N@W but $E is too heavy!@n", true, ch, nullptr, vict, TO_ROOM);
        return;
    }
    else
    {
        reveal_hiding(ch, 0);
        act("@wYou grab and start dragging @C$N@W.@n", true, ch, nullptr, vict, TO_CHAR);
        act("@C$n@W grabs and starts dragging @c$N@W.@n", true, ch, nullptr, vict, TO_NOTVICT);
        DRAGGING(ch) = vict;
        DRAGGED(vict) = ch;
        if (!AFF_FLAGGED(vict, AFF_KNOCKED) && !AFF_FLAGGED(vict, AFF_SLEEP) && rand_number(1, 3))
        {
            vict->sendText("You feel your sleeping body being moved.\r\n");
            if (IS_NPC(vict) && !FIGHTING(vict))
            {
                set_fighting(vict, ch);
            }
        }
    }
}

ACMD(do_stop)
{

    if (IS_NPC(ch))
        return;

    if (!FIGHTING(ch))
    {
        ch->sendText("You are not even fighting!\r\n");
        return;
    }
    else
    {
        act("@CYou move out of your fighting posture.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@c$n@C moves out of $s fighting posture.@n", true, ch, nullptr, nullptr, TO_ROOM);
        stop_fighting(ch);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
}

ACMD(do_suppress)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);

    if (IS_NPC(ch))
        return;

    if (IS_ANDROID(ch))
    {
        ch->sendText("You are unable to suppress your powerlevel.\r\n");
        return;
    }
    if (GET_BONUS(ch, BONUS_ARROGANT) > 0)
    {
        ch->sendText("You are far too arrogant to hide your strength.\r\n");
        return;
    }
    if (ch->character_flags.get(CharacterFlag::powering_up))
    {
        ch->sendText("You are currently powering up, can't suppress.\r\n");
        return;
    }

    if (!*arg)
    {
        ch->sendText("Suppress to what percent?\r\nSyntax: suppress (1 - 99 | release)\r\n");
        return;
    }

    if (!strcasecmp(arg, "release"))
    {
        if (GET_SUPPRESS(ch))
        {
            reveal_hiding(ch, 0);
            act("@GYou stop suppressing your current powerlevel!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@G$n smiles as a rush of power erupts around $s body briefly.@n", true, ch, nullptr, nullptr, TO_ROOM);
            ch->setBaseStat("suppression", 0);
            return;
        }
        else
        {
            ch->sendText("You are not suppressing!\r\n");
            return;
        }
    }

    int num = atoi(arg);

    if (num > 99 || num <= 0)
    {
        ch->sendText("Out of suppression range.\r\nSyntax: suppress (1 - 99 | release)\r\n");
        return;
    }

    int64_t max = (ch->getEffectiveStat<int64_t>("health"));
    int64_t amt = ((max * 0.01) * num);

    reveal_hiding(ch, 0);

    if (GET_SUPPRESS(ch) != 0)
    {
        act("@GYou alter your suppression level!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@G$n seems to concentrate for a moment.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else
    {
        act("@GYou suppress your current powerlevel!@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@G$n seems to concentrate for a moment.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    ch->setBaseStat("suppression", num);
    return;
}

ACMD(do_hass)
{

    int perc = 0, prob = 0;

    if (!check_skill(ch, SKILL_HASSHUKEN))
    {
        return;
    }
    if ((ch->getCurVital(CharVital::stamina)) < GET_MAX_MOVE(ch) / 30)
    {
        ch->sendText("You do not have enough stamina.\r\n");
        return;
    }

    perc = init_skill(ch, SKILL_HASSHUKEN);
    prob = axion_dice(0);

    if (perc < prob)
    {
        reveal_hiding(ch, 0);
        act("@WYou try to move your arms at incredible speeds but screw up and waste some of your stamina.@n", true, ch,
            nullptr, nullptr, TO_CHAR);
        act("@C$n@W tries to move $s arms at incredible speeds but screws up and wastes some of $s stamina.@n", true,
            ch, nullptr, nullptr, TO_ROOM);
        ch->modCurVital(CharVital::stamina, -(ch->getEffectiveStat<int64_t>("stamina") / 30));
        improve_skill(ch, SKILL_HASSHUKEN, 0);
        return;
    }
    else
    {
        reveal_hiding(ch, 0);
        act("@WYou concentrate and start to move your arms at incredible speeds.@n", true, ch, nullptr, nullptr,
            TO_CHAR);
        act("@C$n@W concentrates and starts to move $s arms at incredible speeds.@n", true, ch, nullptr, nullptr,
            TO_ROOM);
        int duration = perc / 15;
        assign_affect(ch, AFF_HASS, SKILL_HASSHUKEN, duration, 0, 0, 0, 0, 0, 0);
        ch->modCurVital(CharVital::stamina, -(ch->getEffectiveStat<int64_t>("stamina") / 30));
        improve_skill(ch, SKILL_HASSHUKEN, 0);
        return;
    }
}

ACMD(do_implant)
{

    Object *limb = nullptr, *obj = nullptr, *next_obj;
    Character *vict = nullptr;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int found = false;

    two_arguments(argument, arg, arg2);

    if (!limb_ok(ch, 0))
    {
        return;
    }

    if (!*arg)
    {
        ch->sendText("Syntax: implant (rarm | larm | rleg | lleg) (target)\r\n");
        return;
    }

    if (!*arg2)
    {
        vict = ch;
    }
    else if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("That person isn't here.\r\n");
        return;
    }

    limb = ch->searchInventory(66);
    if (!limb)
    {
        ch->sendText("You do not have a cybernetic limb to implant.\r\n");
        return;
    }

    if (!(strcmp(arg, "rarm")))
    {
        if (GET_LIMBCOND(vict, 0) >= 1)
        {
            if (vict != ch)
            {
                ch->sendText("They already have a right arm!\r\n");
            }
            if (vict == ch)
            {
                ch->sendText("You already have a right arm!\r\n");
            }
            return;
        }
        else
        {
            if (vict != ch)
            {
                reveal_hiding(ch, 0);
                act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right arm!@n",
                    true, ch, limb, vict, TO_CHAR);
                act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new right arm!@n",
                    true, ch, limb, vict, TO_VICT);
                act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right arm!@n",
                    true, ch, limb, vict, TO_NOTVICT);
            }
            if (vict == ch)
            {
                reveal_hiding(ch, 0);
                act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new right arm!@n",
                    true, ch, limb, nullptr, TO_CHAR);
                act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new right arm!@n",
                    true, ch, limb, nullptr, TO_ROOM);
            }
            vict->character_flags.set(CharacterFlag::cyber_right_arm, true);
            limb->clearLocation();
            extract_obj(limb);
            return;
        }
    }
    else if (!(strcmp(arg, "larm")))
    {
        if (GET_LIMBCOND(vict, 1) >= 1)
        {
            if (vict != ch)
            {
                ch->sendText("They already have a left arm!\r\n");
            }
            if (vict == ch)
            {
                ch->sendText("You already have a left arm!\r\n");
            }
            return;
        }
        else
        {
            if (vict && vict != ch)
            {
                reveal_hiding(ch, 0);
                act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left arm!@n",
                    true, ch, limb, vict, TO_CHAR);
                act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new left arm!@n",
                    true, ch, limb, vict, TO_VICT);
                act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left arm!@n",
                    true, ch, limb, vict, TO_NOTVICT);
            }
            if (vict == ch)
            {
                reveal_hiding(ch, 0);
                act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new left arm!@n",
                    true, ch, limb, nullptr, TO_CHAR);
                act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new left arm!@n",
                    true, ch, limb, nullptr, TO_ROOM);
            }
            vict->character_flags.set(CharacterFlag::cyber_left_arm, true);
            limb->clearLocation();
            extract_obj(limb);
            return;
        }
    }
    else if (!(strcmp(arg, "rleg")))
    {
        if (GET_LIMBCOND(vict, 2) >= 1)
        {
            if (vict != ch)
            {
                ch->sendText("They already have a right leg!\r\n");
            }
            if (vict == ch)
            {
                ch->sendText("You already have a right leg!\r\n");
            }
            return;
        }
        else
        {
            if (vict && vict != ch)
            {
                reveal_hiding(ch, 0);
                act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right leg!@n",
                    true, ch, limb, vict, TO_CHAR);
                act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new right leg!@n",
                    true, ch, limb, vict, TO_VICT);
                act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right leg!@n",
                    true, ch, limb, vict, TO_NOTVICT);
            }
            if (vict == ch)
            {
                reveal_hiding(ch, 0);
                act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new right leg!@n",
                    true, ch, limb, nullptr, TO_CHAR);
                act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new right leg!@n",
                    true, ch, limb, nullptr, TO_ROOM);
            }
            vict->character_flags.set(CharacterFlag::cyber_right_leg, true);
            limb->clearLocation();
            extract_obj(limb);
            return;
        }
    }
    else if (!(strcmp(arg, "lleg")))
    {
        if (GET_LIMBCOND(vict, 3) >= 1)
        {
            if (vict != ch)
            {
                ch->sendText("They already have a left leg!\r\n");
            }
            if (vict == ch)
            {
                ch->sendText("You already have a left leg!\r\n");
            }
            return;
        }
        else
        {
            if (vict && vict != ch)
            {
                reveal_hiding(ch, 0);
                act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left leg!@n",
                    true, ch, limb, vict, TO_CHAR);
                act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new left leg!@n",
                    true, ch, limb, vict, TO_VICT);
                act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left leg!@n",
                    true, ch, limb, vict, TO_NOTVICT);
            }
            if (!vict || vict == ch)
            {
                reveal_hiding(ch, 0);
                act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new left leg!@n",
                    true, ch, limb, nullptr, TO_CHAR);
                act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new left leg!@n",
                    true, ch, limb, nullptr, TO_ROOM);
            }
            vict->character_flags.set(CharacterFlag::cyber_left_leg, true);
            limb->clearLocation();
            extract_obj(limb);
            return;
        }
    }
    else
    {
        ch->sendText("Syntax: implant (rarm | larm | rleg | rleg)\r\n");
        return;
    }
}

ACMD(do_pose)
{

    if (!know_skill(ch, SKILL_POSE))
    {
        return;
    }

    if ((ch->getCurVital(CharVital::stamina)) < GET_MAX_MOVE(ch) / 40)
    {
        ch->sendText("You do not have enough stamina to pull off such an exciting pose!\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_POSE))
    {
        ch->sendText("You are already feeling good and confident from a previous pose.\r\n");
        return;
    }

    if (FIGHTING(ch))
    {
        ch->sendText("You are too busy to pose right now!\r\n");
        return;
    }

    int prob = GET_SKILL(ch, SKILL_POSE);
    int perc = rand_number(1, 70);

    if (AFF_FLAGGED(ch, AFF_POSE))
    {
        ch->sendText("You're already fighting stylishly!\r\n");
        return;
    }

    if (prob < perc)
    {
        reveal_hiding(ch, 0);
        act("@WYou attempt to strike an awe inspiring pose, but end up falling on your face!@n", true, ch, nullptr,
            nullptr, TO_CHAR);
        act("@C$n@W attempts to strike an awe inspiring pose, but ends up falling on $s face!@n", true, ch, nullptr,
            nullptr, TO_ROOM);
        ch->modCurVital(CharVital::stamina, -(ch->getEffectiveStat<int64_t>("stamina") / 40));
        improve_skill(ch, SKILL_POSE, 0);
        return;
    }

    reveal_hiding(ch, 0);
    switch (rand_number(1, 4))
    {
    case 1:
        act("@WYou turn around with your back to everyone. You bend forward dramatically and put your head between your legs!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W turns around with $s back to you. $e bends forward dramatically and puts $s head between $s legs. Strange...@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        break;
    case 2:
        act("@WYou turn to the side while flexing your muscles and extend your arms up at an angle dramatically!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W turns to the side while flexing $s muscles and extending $s arms up at an angle dramatically!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        break;
    case 3:
        act("@WYou extend one leg outward while you bend forward, balancing on a single leg!@n", true, ch,
            nullptr, nullptr, TO_CHAR);
        act("@C$n@W extends one leg outward while $e bends forward, balancing on a single leg!@n", true, ch,
            nullptr, nullptr, TO_ROOM);
        break;
    case 4:
        act("@WYou drop down to one knee while angling your arms up to either side and slanting your hands down like wings!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W drops down to one knee while angling $s arms up to either side and slanting $s hands down like wings!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        break;
    }
    ch->sendText("@WYou feel your confidence increase! @G+3 Str @Wand@G +3 Agl!@n\r\n");
    assign_affect(ch, AFF_POSE, SKILL_POSE, -1, 8, 0, 0, 8, 0, 0);
    int64_t before = (ch->getEffectiveStat("lifeforce"));
    ch->player_flags.set(PLR_POSE, true);

    ch->modCurVital(CharVital::lifeforce, (ch->getEffectiveStat("lifeforce")) - before);
    ch->modCurVital(CharVital::stamina, -(ch->getEffectiveStat<int64_t>("stamina") / 40));
    improve_skill(ch, SKILL_POSE, 0);
    return;
}

/* do_fury for halfbreeds to release their raaaage, rawrg! */

ACMD(do_fury)
{

    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!IS_HALFBREED(ch) || IS_NPC(ch))
    {
        ch->sendText("You are furious, but you'll get over it.\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_FURY))
    {
        ch->sendText("You are already furious, your next attack will devestate, hurry use it!\r\n");
        return;
    }

    if (GET_FURY(ch) < 100)
    {
        ch->sendText("You do not have enough anger to release your fury upon your foes!\r\n");
        return;
    }

    if (!*arg)
    {
        if (GET_HIT(ch) < (ch->getEffectiveStat<int64_t>("health")))
        {
            if ((ch->getCurVital(CharVital::lifeforce)) >= (ch->getEffectiveStat("lifeforce")) * 0.2)
            {
                ch->restoreHealth(false);
                ch->modCurVitalDam(CharVital::lifeforce, 0.2);
            }
            else
            {
                ch->modCurVital(CharVital::health, (ch->getCurVital(CharVital::lifeforce)));
                ch->modCurVitalDam(CharVital::lifeforce, 2);
            }
        }
        ch->setBaseStat("fury", 0);
    }
    else if (!strcasecmp(arg, "attack"))
    {
        ch->setBaseStat("fury", 50);
    }
    else
    {
        ch->sendText("Syntax: fury (attack) <--- this will not use up your LF to restore PL.\n        fury <--- fury by itself will do both LF to PL restore and attack boost.\r\n");
        return;
    }

    reveal_hiding(ch, 0);
    act("You release your fury! Your very next attack is guaranteed to rip your foes a new one!", true, ch, nullptr,
        nullptr, TO_CHAR);
    act("$n screams furiously as a look of anger appears on $s face!", true, ch, nullptr, nullptr, TO_ROOM);
    ch->player_flags.set(PLR_FURY, true);
}

/* End of do_fury for halfbreeds to release their raaage, rawrg! */

void hint_system(Character *ch, int num)
{
    const char *hints[22] = {"Remember to save often.",                                                                                                                 /* 0 */
                             "Remember to eat or drink if you want to stay alive.",                                                                                     /* 1 */
                             "It is a good idea to save up PS for learning skills instead of just practicing them.",                                                    /* 2 */
                             "A good way to save up money is with the bank.",                                                                                           /* 3 */
                             "If you want to stay alive in this rough world you will need to be mindful of your surroundings.",                                         /* 4 */
                             "Knowing when to rest and recover can be the difference between life and death.",                                                          /* 5 */
                             "Not every battle can be won. Great warriors know how to pick their fights.",                                                              /* 6 */
                             "It is a good idea to experiment with skills fully before deciding their worth.",                                                          /* 7 */
                             "Having a well balanced repertoire of skills can help you out of any situation.",                                                          /* 8 */
                             "You can become hidden from your enemies on who and ooc with the whohide command.",                                                        /* 9 */
                             "You can value an item at a shopkeeper with the value command.",                                                                           /* 10 */
                             "There are ways to earn money through jobs, try looking for a job. Bum.",                                                                  /* 11 */
                             "You never know what may be hidden nearby. You should always check out anything you can.",                                                 /* 12 */
                             "You should check for a help file on any subject you can, you never know how the info may 'help' you.",                                    /* 13 */
                             "Until you are capable of taking care of yourself for long periods of time you should stick near your sensei.",                            /* 14 */
                             "You shouldn't travel to other planets until you have a stable supply of money.",                                                          /* 15 */
                             "There is a vast galaxy out there that you may not be able to reach by public ship.",                                                      /* 16 */
                             "Score is used to view the various statistics about your character.",                                                                      /* 17 */
                             "Status is used to view what is influencing your character and its characteristics.",                                                      /* 18 */
                             "You will need a scouter in order to use the Scouter Network (SNET).",                                                                     /* 19 */
                             "The DBAT forum is a great resource for finding out information and for conversing\r\nwith fellow players. http://advent-truth.com/forum", /* 20 */
                             "Found a bug or have a suggestion? Log into our forums and post in the relevant section."};
    if (num == 0)
    {
        num = rand_number(0, 21);
    }

    if (!IS_ANDROID(ch) && !IS_NAMEK(ch))
    {
        ch->send_to("@D[@GHint@D] @G%s@n\r\n", hints[num]);
    }
    else
    {
        if (num == 1)
        {
            num = 0;
        }
        ch->send_to("@D[@GHint@D] @G%s@n\r\n", hints[num]);
    }
    ch->sendText("@D(@gYou can turn off hints with the command 'hints'@D)@n\r\n");
}

ACMD(do_think)
{
    skip_spaces(&argument);

    if (IS_NPC(ch))
    {
        return;
    }
    if (IN_ARENA(ch))
    {
        ch->sendText("Lol, no.\r\n");
        return;
    }
    if (GET_SKILL(ch, SKILL_TELEPATHY))
    {
        ch->sendText("You can just use telepathy.\r\n");
        return;
    }
    if (!MINDLINK(ch))
    {
        ch->sendText("No one has linked with your mind.\r\n");
        return;
    }
    if (!*argument)
    {
        ch->sendText("Syntax: think (message)\r\n");
        return;
    }
    else
    {
        Character *tch;
        tch = MINDLINK(ch);
        ch->send_to("@c%s@w reads your thoughts, '@C%s@w'@n\r\n", GET_NAME(tch), argument);
        tch->send_to("@c%s@w thinks, '@C%s@w'@n\r\n", GET_NAME(ch), argument);
        send_to_imm("@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n",
                    GET_ADMLEVEL(ch) > 0 ? GET_NAME(ch) : GET_USER(ch),
                    GET_ADMLEVEL(tch) > 0 ? GET_NAME(tch) : GET_USER(tch), argument);
        return;
    }
}

ACMD(do_telepathy)
{
    Character *vict;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    half_chop(argument, arg, arg2);

    if (!know_skill(ch, SKILL_TELEPATHY))
    {
        return;
    }

    if (IN_ARENA(ch))
    {
        ch->sendText("Lol, no.\r\n");
        return;
    }

    if (!*arg)
    {
        ch->sendText("Syntax: telepathy [ read ] (target)\r\n"
                     "        telepathy [ link ] (target)\r\n"
                     "        telepathy [  far ] (target)\r\n"
                     "        telepathy (target) (message)\r\n");
        return;
    }
    else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 40)
    {
        ch->sendText("You do not have enough ki to focus your mental abilities.\r\n");
        return;
    }
    if (!(strcmp(arg, "far")))
    {
        if (IS_NPC(ch))
        {
            return;
        }
        else if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_WORLD)))
        {
            ch->sendText("Look through who's eyes?\r\n");
            return;
        }
        else if (vict == ch)
        {
            ch->sendText("Oh that makes a lot of sense...\r\n");
            return;
        }
        else if (IS_NPC(vict))
        {
            ch->sendText("You can't touch the mind of such a thing.\r\n");
            return;
        }
        else if (GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch))
        {
            ch->sendText("Their mental power oustrips your's by unfathomable measurements!\r\n");
            return;
        }
        else if (AFF_FLAGGED(ch, AFF_SHOCKED))
        {
            ch->sendText("Your mind has been shocked by telepathic feedback! You are not able to use telepathy right now.\r\n");
            return;
        }
        else if (IS_ANDROID(vict))
        {
            ch->sendText("You can't touch the mind of such an artificial being.\r\n");
            return;
        }
        else if (GET_SKILL(vict, SKILL_TELEPATHY) + GET_INT(vict) > GET_SKILL(ch, SKILL_TELEPATHY) + GET_INT(ch))
        {
            ch->sendText("They throw off your attempt with their own telepathic abilities!\r\n");
            return;
        }
        else if (ch->location == vict->location)
        {
            ch->sendText("They are in the same room as you!\r\n");
            return;
        }
        else if (AFF_FLAGGED(vict, AFF_BLIND))
        {
            ch->sendText("They are blind!\r\n");
            return;
        }
        else if (PLR_FLAGGED(vict, PLR_EYEC))
        {
            ch->sendText("Their eyes are closed!\r\n");
            return;
        }
        else
        {
            ch->lookAtLocation(vict->location);
            ch->sendText("You see all this through their eyes!\r\n");
            if (GET_INT(vict) > GET_INT(ch))
            {
                ch->sendText("You feel like someone was using your mind for something...\r\n");
            }
            ch->modCurVitalDam(CharVital::ki, 0.025);
            return;
        }
    }
    if (!(strcmp(arg, "link")))
    {
        if (IS_NPC(ch))
        {
            return;
        }
        if (MINDLINK(ch))
        {
            act("@CYou remove the link your mind had with @w$N.@n", true, ch, nullptr, MINDLINK(ch), TO_CHAR);
            act("@w$n@C removes the link $s mind had with yours.@n", true, ch, nullptr, MINDLINK(ch), TO_VICT);
            MINDLINK(MINDLINK(ch)) = nullptr;
            MINDLINK(ch) = nullptr;
            ch->setBaseStat("mind_linker", 0);
            return;
        }
        else if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_WORLD)))
        {
            ch->sendText("Link with the mind of who?\r\n");
            return;
        }
        else if (vict == ch)
        {
            ch->sendText("Oh that makes a lot of sense...\r\n");
            return;
        }
        else if (IS_NPC(vict))
        {
            ch->sendText("You can't touch the mind of such a thing.\r\n");
            return;
        }
        else if (IS_ANDROID(vict))
        {
            ch->sendText("You can't touch the mind of such an artificial being.\r\n");
            return;
        }
        else if (GET_SKILL(vict, SKILL_TELEPATHY))
        {
            ch->sendText("Kinda pointless when you are both telepathic huh?\r\n");
            return;
        }
        else if (MINDLINK(vict))
        {
            ch->sendText("Someone else is already telepathically linked with them.\r\n");
            return;
        }
        else if (GET_SKILL(ch, SKILL_TELEPATHY) < axion_dice(GET_INT(vict) * 0.1))
        {
            act("@R$n@r tried to link $s mind with yours, but you manage to force a break in the link!@n", false, ch,
                nullptr, vict, TO_VICT);
            act("@R$N@r manages to sense the intrusion and with $S intelligence push you out!@n", false, ch, nullptr,
                vict, TO_CHAR);
            return;
        }
        else
        {
            act("@CYou link your mind with @w$N.@n", true, ch, nullptr, vict, TO_CHAR);
            act("@w$n@C links $s mind with yours. You can speak your thoughts to $m with 'think'.@n", true, ch, nullptr,
                vict, TO_VICT);
            vict->sendText("@wIf this is undesirable, Try: meditate break@n\r\n");
            MINDLINK(vict) = ch;
            MINDLINK(ch) = vict;
            ch->setBaseStat("mind_linker", 1);
            return;
        }
    }
    else if (!(strcmp(arg, "read")))
    {
        if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_ROOM)))
        {
            ch->sendText("Read the mind of who?\r\n");
            return;
        }
        else if (vict == ch)
        {
            ch->sendText("Oh that makes a lot of sense...\r\n");
            return;
        }
        else if (IS_ANDROID(vict))
        {
            ch->sendText("You can't touch the mind of such an artificial being.\r\n");
            return;
        }
        else
        {
            if (axion_dice(0) > GET_SKILL(ch, SKILL_TELEPATHY))
            {
                ch->modCurVitalDam(CharVital::ki, 0.025);
                act("@wYou attempt to read $N's@w mind, but fail to see it clearly.@n", true, ch, nullptr, vict,
                    TO_CHAR);
                if (rand_number(1, 15) >= 14 && !AFF_FLAGGED(ch, AFF_SHOCKED))
                {
                    act("@MYour mind has been shocked!@n", true, ch, nullptr, nullptr, TO_CHAR);
                    ch->affect_flags.set(AFF_SHOCKED, true);
                }
                else
                {
                    improve_skill(ch, SKILL_TELEPATHY, 0);
                }
                return;
            }
            else if (GET_SKILL(vict, SKILL_TELEPATHY) >= GET_SKILL(ch, SKILL_TELEPATHY) && rand_number(1, 2) == 2)
            {
                ch->modCurVitalDam(CharVital::ki, 0.025);
                act("@wYou fail to read @c$N's@w mind and they seemed to have noticed the attempt!@n", true, ch,
                    nullptr, vict, TO_CHAR);
                act("@C$n@w attempts to read your mind, but you resist and force $m out!@n", true, ch, nullptr, vict,
                    TO_VICT);
                improve_skill(ch, SKILL_TELEPATHY, 0);
                return;
            }
            else
            {
                ch->sendText("@wYou peer into their mind:\r\n");
                ch->modCurVitalDam(CharVital::ki, 0.025);
                ch->send_to("@GName      @D: @W%s@n\r\n", GET_NAME(vict));
                ch->send_to("@GRace      @D: @W%s@n\r\n", race::getName(vict->race).c_str());
                ch->send_to("@GSensei    @D: @W%s@n\r\n", sensei::getName(vict->sensei).c_str());
                ch->send_to("@GStr       @D: @W%d@n\r\n", GET_STR(vict));
                ch->send_to("@GCon       @D: @W%d@n\r\n", GET_CON(vict));
                ch->send_to("@GInt       @D: @W%d@n\r\n", GET_INT(vict));
                ch->send_to("@GWis       @D: @W%d@n\r\n", GET_WIS(vict));
                ch->send_to("@GSpd       @D: @W%d@n\r\n", GET_CHA(vict));
                ch->send_to("@GAgi       @D: @W%d@n\r\n", GET_DEX(vict));
                ch->send_to("@GZenni     @D: @W%s@n\r\n", add_commas(GET_GOLD(vict)).c_str());
                ch->send_to("@GBank Zenni@D: @W%s@n\r\n", add_commas(GET_BANK_GOLD(vict)).c_str());
                if (GET_ALIGNMENT(vict) >= 1000)
                {
                    ch->sendText("@GAlignment @D: @wSaint         @n\r\n");
                }
                else if (GET_ALIGNMENT(vict) > 750)
                {
                    ch->sendText("@GAlignment @D: @wExtremely Good@n\r\n");
                }
                else if (GET_ALIGNMENT(vict) > 500)
                {
                    ch->sendText("@GAlignment @D: @wReally Good   @n\r\n");
                }
                else if (GET_ALIGNMENT(vict) > 250)
                {
                    ch->sendText("@GAlignment @D: @wGood          @n\r\n");
                }
                else if (GET_ALIGNMENT(vict) > 100)
                {
                    ch->sendText("@GAlignment @D: @wPretty Good   @n\r\n");
                }
                else if (GET_ALIGNMENT(vict) > 50)
                {
                    ch->sendText("@GAlignment @D: @wSorta Good    @n\r\n");
                }
                else if (GET_ALIGNMENT(vict) > -50)
                {
                    ch->sendText("@GAlignment @D: @wNeutral       @n\r\n");
                }
                else if (GET_ALIGNMENT(vict) > -100)
                {
                    ch->sendText("@GAlignment @D: @wSorta Evil    @n\r\n");
                }
                else if (GET_ALIGNMENT(vict) > -500)
                {
                    ch->sendText("@GAlignment @D: @wPretty Evil   @n\r\n");
                }
                else if (GET_ALIGNMENT(vict) >= -750)
                {
                    ch->sendText("@GAlignment @D: @wEvil          @n\r\n");
                }
                else if (GET_ALIGNMENT(vict) < -750)
                {
                    ch->sendText("@GAlignment @D: @wExtremely Evil@n\r\n");
                }
                else if (GET_ALIGNMENT(vict) <= -1000)
                {
                    ch->sendText("@GAlignment @D: @wDevil         @n\r\n");
                }
                else
                {
                    ch->sendText("@GAlignment @D: @wUnknown       @n\r\n");
                }
            } // End of read success
        } // End of vict is there
        improve_skill(ch, SKILL_TELEPATHY, 0);
        return;
    } // End of read argument

    else
    {
        if (MINDLINK(ch))
        {
            vict = MINDLINK(ch);
        }
        else if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
        {
            ch->sendText("Send your thoughts to who?\r\n");
            return;
        }
        if (vict == ch)
        {
            ch->sendText("Oh that makes a lot of sense...\r\n");
            return;
        }
        else if (IS_ANDROID(vict))
        {
            ch->sendText("You can't touch the mind of such an artificial being.\r\n");
            return;
        }
        else
        {
            if (!MINDLINK(ch))
            {
                ch->send_to("@WYou tell @c%s@W telepathically, @w'@C%s@w'@n\r\n", GET_NAME(vict), arg2);
                vict->send_to("@c%s@W talks to you telepathically, @w'@C%s@w'@n\r\n", GET_NAME(ch), arg2);
                send_to_imm("@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n",
                            GET_ADMLEVEL(ch) > 0 ? GET_NAME(ch) : GET_USER(ch),
                            GET_ADMLEVEL(vict) > 0 ? GET_NAME(vict) : GET_USER(vict), arg2);
            }
            else
            {
                ch->send_to("@WYou tell @c%s@W telepathically, @w'@C%s@w'@n\r\n", GET_NAME(vict), argument);
                vict->send_to("@c%s@W talks to you telepathically, @w'@C%s@w'@n\r\n", GET_NAME(ch), argument);
                send_to_imm("@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n",
                            GET_ADMLEVEL(ch) > 0 ? GET_NAME(ch) : GET_USER(ch),
                            GET_ADMLEVEL(vict) > 0 ? GET_NAME(vict) : GET_USER(vict), argument);
            }
            ch->modCurVitalDam(CharVital::ki, 0.025);
        } // End of vict is here
        return;
    } // End of send argument
}

ACMD(do_potential)
{
    int boost = 0;

    Character *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!know_skill(ch, SKILL_POTENTIAL))
    {
        return;
    }

    if (!*arg)
    {
        ch->sendText("Who's potential do you want to release?\r\n");
        ch->send_to("Potential Releases: %d\r\n", GET_BOOSTS(ch));
        return;
    }
    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("That target isn't here.\r\n");
        return;
    }

    if (IS_NPC(vict))
    {
        ch->sendText("Why would you waste your time releasing their potential?\r\n");
        return;
    }
    if (vict == ch)
    {
        ch->sendText("You can't release your own potential.\r\n");
        return;
    }
    if (GET_BOOSTS(ch) == 0)
    {
        ch->sendText("You have no potential releases to perform.\r\n");
        return;
    }
    if (vict->transforms.contains(Form::potential_unlocked))
    {
        ch->sendText("Their potential has already been released\r\n");
        return;
    }
    if (IS_ANDROID(vict))
    {
        ch->sendText("They are a machine and have no potential to release.\r\n");
        return;
    }
    if (vict->getBaseStat<int>("majinized") > 0)
    {
        ch->sendText("They are already majinized and have no potential to release.\r\n");
        return;
    }
    if (IS_MAJIN(vict))
    {
        ch->sendText("They have no potential to release...\r\n");
        return;
    }
    /* Rillao: transloc, add new transes here */
    else
    {
        vict->addTransform(Form::potential_unlocked);
        if (GET_SKILL(ch, SKILL_POTENTIAL) >= 100)
            vict->addTransform(Form::potential_unlocked_max);
        reveal_hiding(ch, 0);
        act("You place your hand on top of $N's head. After a moment of concentrating you release their hidden potential.",
            true, ch, nullptr, vict, TO_CHAR);
        act("$n places $s hand on top of your head. After a moment you feel a rush of power as your hidden potential is released!",
            true, ch, nullptr, vict, TO_VICT);
        act("$n places $s hand on $N's head. After a moment a rush of power explodes off of $N's body!", true, ch,
            nullptr, vict, TO_NOTVICT);
        improve_skill(ch, SKILL_POTENTIAL, 0);
        improve_skill(ch, SKILL_POTENTIAL, 0);
        improve_skill(ch, SKILL_POTENTIAL, 0);
        improve_skill(ch, SKILL_POTENTIAL, 0);
        ch->modBaseStat<int>("boosts", -1);
        return;
    }
}

ACMD(do_majinize)
{

    Character *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!IS_MAJIN(ch))
    {
        ch->sendText("You are not a majin and can not majinize anyone.\r\n");
        return;
    }

    if (!*arg)
    {
        ch->sendText("Who do you want to majinize?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("That target isn't here.\r\n");
        return;
    }

    if (IS_NPC(vict))
    {
        ch->sendText("Why would you waste your time majinizing them?\r\n");
        return;
    }
    if (vict == ch)
    {
        ch->sendText("You can't majinize yourself.\r\n");
        return;
    }
    if (vict->transforms.contains(Form::potential_unlocked))
    {
        ch->sendText("You can't majinize them their potential has been released!\r\n");
        return;
    }
    int alignmentTotal = GET_ALIGNMENT(ch) - GET_ALIGNMENT(vict);
    if (auto maj = vict->getBaseStat<int>("majinizer"); maj > 0 && maj != ch->id)
    {
        ch->sendText("They are already majinized before by someone else.\r\n");
        return;
    }
    else if (vict->master != ch)
    {
        ch->sendText("They must be following you in order for you to majinize them.\r\n");
        return;
    }
    else if (!((alignmentTotal >= -1500) && (alignmentTotal <= 1500)))
    {
        ch->sendText("Their alignment is so opposed to your's that they resist your attempts to enslave them!\r\n");
        return;
    }
    else if (GET_MAX_HIT(vict) > GET_MAX_HIT(ch) * 4)
    {
        ch->sendText("Their powerlevel is so much higher than yours they resist your attempts to enslave them!\r\n");
        return;
    }
    /* Rillao: transloc, add new transes here */
    else if (vict->permForms.contains(Form::majinized) && vict->getBaseStat<int>("majinizer") == ch->id)
    {
        reveal_hiding(ch, 0);
        act("You remove $N's majinization, freeing them from your influence, but also weakening them.", true, ch,
            nullptr, vict, TO_CHAR);
        act("$n removes your majinization, freeing you from their influence, and weakening you!", true, ch, nullptr,
            vict, TO_VICT);
        act("$n waves a hand at $N, and instantly the glowing M on $S forehead disappears!", true, ch, nullptr, vict,
            TO_NOTVICT);
        vict->setBaseStat("majinizer", 0);
        ch->modBaseStat<int>("boosts", 1);

        if (vict->getBaseStat<int>("majinized") == 0)
        {
            vict->setBaseStat("majinized", vict->getBaseStat<int64_t>("health") * .4);
        }
        vict->permForms.erase(Form::majinized);
        return;
    }
    else if (GET_BOOSTS(ch) == 0)
    {
        auto chInt = GET_INT(ch);
        ch->send_to("You are incapable of majinizing%s.\r\n", chInt < 100 ? " right now" : " anymore");
        if (chInt < 25)
        {
            ch->sendText("Your next available majinize will be at Intelligence 25\r\n");
        }
        else if (chInt < 50)
        {
            ch->sendText("Your next available majinize will be at Intelligence 50\r\n");
        }
        else if (chInt < 75)
        {
            ch->sendText("Your next available majinize will be at Intelligence 75\r\n");
        }
        else if (chInt < 100)
        {
            ch->sendText("Your next available majinize will be at Intelligence 100\r\n");
        }
        return;
    }
    else
    {
        reveal_hiding(ch, 0);
        act("You focus your power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.",
            true, ch, nullptr, vict, TO_CHAR);
        act("$n focuses power into you, influencing your mind and increasing your strength! After the struggle in your mind ends a glowing purple M forms on your forehead.",
            true, ch, nullptr, vict, TO_VICT);
        act("$n focuses power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.",
            true, ch, nullptr, vict, TO_NOTVICT);
        vict->setBaseStat("majinizer", ch->id);
        ch->modBaseStat<int>("boosts", -1);

        vict->setBaseStat("majinized", (vict->getBaseStat<int64_t>("health")) * .4);
        vict->addTransform(Form::majinized);
        return;
    }
}

ACMD(do_spit)
{
    int cost = 0;
    Character *vict;
    struct affected_type af;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!know_skill(ch, SKILL_SPIT))
    {
        return;
    }
    if (FIGHTING(ch))
    {
        ch->sendText("You can't manage to spit in this fight!\r\n");
        return;
    }

    if (!*arg)
    {
        ch->sendText("Yes but who do you want to petrify?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("That target isn't here.\r\n");
        return;
    }
    if (!can_kill(ch, vict, nullptr, 0))
    {
        return;
    }
    if (AFF_FLAGGED(vict, AFF_PARALYZE))
    {
        act("$N has already been turned to stone.", true, ch, nullptr, vict, TO_CHAR);
        return;
    }
    if (FIGHTING(vict))
    {
        vict->sendText("You can't manage to spit on them, they are moving around too much!\r\n");
        return;
    }

    cost = ((GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_SPIT) / 4)) + GET_MAX_MANA(ch) / 100);

    if ((ch->getCurVital(CharVital::ki)) < cost)
    {
        ch->sendText("You do not have enough ki to petrifiy with your spit!\r\n");
        return;
    }

    if (GET_SKILL(ch, SKILL_SPIT) < axion_dice(0))
    {
        ch->modCurVital(CharVital::ki, -cost);
        reveal_hiding(ch, 0);
        act("@WGathering spit you concentrate ki into a wicked loogie and let it loose, but it falls short of hitting @c$N@W!@n",
            true, ch, nullptr, vict, TO_CHAR);
        act("@C$n@W seems to focus ki before hawking a loogie at you! Fortunatly the loogie falls short.@n", true, ch,
            nullptr, vict, TO_VICT);
        act("@C$n@W seems to focus ki before hawking a loogie at @c$N@W! Fortunatly for @c$N@W the loogie falls short.@n",
            true, ch, nullptr, vict, TO_NOTVICT);
        improve_skill(ch, SKILL_SPIT, 1);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
    else if (AFF_FLAGGED(vict, AFF_ZANZOKEN) && (vict->getCurVital(CharVital::stamina)) >= 1 && GET_POS(vict) != POS_SLEEPING)
    {
        ch->modCurVital(CharVital::ki, -cost);
        reveal_hiding(ch, 0);
        act("@C$N@c disappears, avoiding your spit before reappearing!@n", false, ch, nullptr, vict, TO_CHAR);
        act("@cYou disappear, avoiding @C$n's@c @rstone spit@c before reappearing!@n", false, ch, nullptr, vict,
            TO_VICT);
        act("@C$N@c disappears, avoiding @C$n's@c @rstone spit@c before reappearing!@n", false, ch, nullptr, vict,
            TO_NOTVICT);
        vict->affect_flags.set(AFF_ZANZOKEN, false);
        WAIT_STATE(ch, PULSE_2SEC);
        improve_skill(ch, SKILL_SPIT, 1);
        return;
    }
    else
    {
        af.type = SPELL_PARALYZE;
        af.duration = rand_number(1, 2);
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = AFF_PARALYZE;
        affect_join(vict, &af, false, false, false, false);

        ch->modCurVital(CharVital::ki, -cost);
        reveal_hiding(ch, 0);
        act("@WGathering spit you concentrate ki into a wicked loogie and let it loose, and it smacks into @c$N@W turning $M into stone!@n",
            true, ch, nullptr, vict, TO_CHAR);
        act("@C$n@W seems to focus ki before hawking a loogie at you! It manages to hit and you instantly turn to stone!@n",
            true, ch, nullptr, vict, TO_VICT);
        act("@C$n@W seems to focus ki before hawking a loogie at @c$N@W! It manages to hit and $E instantly turns to stone!@n",
            true, ch, nullptr, vict, TO_NOTVICT);
        improve_skill(ch, SKILL_SPIT, 1);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
}

/* This handles increasing the stats of a created object based on the skill *
 * and/or stats of the user.                                                */
static void boost_obj(Object *obj, Character *ch, int type)
{

    if (!obj || !ch)
        return;

    int boost = 0;

    auto chWis = GET_WIS(ch);

    if (chWis >= 100)
    {
        boost = 100;
    }
    else if (chWis >= 90)
    {
        boost = 90;
    }
    else if (chWis >= 80)
    {
        boost = 80;
    }
    else if (chWis >= 70)
    {
        boost = 70;
    }
    else if (chWis >= 60)
    {
        boost = 60;
    }
    else if (chWis >= 50)
    {
        boost = 50;
    }
    else if (chWis >= 40)
    {
        boost = 40;
    }
    else if (chWis >= 30)
    {
        boost = 30;
    }

    switch (type)
    {       /* Main switch of boost_obj */
    case 0: /* This object is a piece of worn equipment, not a weapon. */
        if (boost != 0)
        { /* Change it if it qualifies */
            obj->setBaseStat<int>("level", boost);
            obj->affected[0].location = 17;
            obj->affected[0].modifier += (boost * GET_DEX(ch));
            if (GET_OBJ_VNUM(obj) == 91)
            {
                obj->affected[1].location = 1;
                obj->affected[1].modifier = (boost / 20);
            }
            else
            {
                obj->affected[1].location = 3;
                obj->affected[1].modifier = (boost / 20);
            }
        }
        break;
    case 1: /* This object is a weapon. */
        switch (boost)
        {
        case 30:
            obj->setBaseStat<int64_t>(VAL_WEAPON_LEVEL, 2);
            break;
        case 40:
        case 50:
            obj->setBaseStat<int64_t>(VAL_WEAPON_LEVEL, 3);
            break;
        case 60:
        case 70:
        case 80:
        case 90:
            obj->setBaseStat<int64_t>(VAL_WEAPON_LEVEL, 4);
            break;
        case 100:
            obj->setBaseStat<int64_t>(VAL_WEAPON_LEVEL, 5);
            break;
        default:
            obj->setBaseStat<int64_t>(VAL_WEAPON_LEVEL, 1);
            break;
        }
        if (boost != 0)
        {
            obj->setBaseStat<int>("level", boost);
            obj->affected[0].location = 1;
            obj->affected[0].modifier = (boost / 20);
        }
        break;
    } /* End of main switch */
}

ACMD(do_form)
{
    int skill = 0, senzu = false, bag = false, light = false, sword = false, mattress = false, gi = false, pants = false, kachin = false, boost = false, shuriken = false;
    int clothes = false, wrist = false, boots = false, level = 0;
    double discount = 1.0;
    int64_t cost = 0;
    Object *obj;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH], clam[MAX_INPUT_LENGTH];

    half_chop(argument, arg, clam);

    half_chop(clam, arg2, arg3);

    if (!know_skill(ch, SKILL_CREATE))
    {
        return;
    }

    /* -- code disabled as of 10/24/2021
  if (GET_COOLDOWN(ch) > 0) {
      ch->sendText("You must wait a short period before concentrating again.\r\n");
   return;
  }
   */

    skill = GET_SKILL(ch, SKILL_CREATE);

    if (skill >= 100)
    {
        boost = true;
    }
    if (skill >= 90)
    {
        kachin = true;
    }
    if (skill >= 80)
    {
        senzu = true;
    }
    if (skill >= 70)
    {
        shuriken = true;
    }
    if (skill >= 60)
    {
        clothes = true;
    }
    if (skill >= 50)
    {
        sword = true;
        gi = true;
        pants = true;
        wrist = true;
        boots = true;
    }
    if (skill >= 40)
    {
        mattress = true;
    }
    if (skill >= 30)
    {
        bag = true;
    }
    if (skill >= 20)
    {
        light = true;
    }

    if (GET_SKILL(ch, SKILL_CONCENTRATION))
    {
        if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 100)
        {
            discount = 0.5;
        }
        else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 90)
        {
            discount = 0.6;
        }
        else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 80)
        {
            discount = 0.65;
        }
        else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 70)
        {
            discount = 0.7;
        }
        else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 60)
        {
            discount = 0.75;
        }
        else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 50)
        {
            discount = 0.8;
        }
        else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 40)
        {
            discount = 0.85;
        }
        else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 30)
        {
            discount = 0.9;
        }
        else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 20)
        {
            discount = 0.95;
        }
    }

    if (!*arg)
    {
        ch->send_to("What do you want to create?\r\n"
                    "@GCreation @WMenu@n\r\n"
                    "@D---------------@n\r\n"
                    "@wcreate food\r\n"
                    "create water\r\n"
                    "%s%s%s%s%s%s%s%s%s%s%s%s%s\r\n",
                    light ? "create light\r\n" : "", bag ? "create bag\r\n" : "", mattress ? "create mattress\r\n" : "", sword ? "create weapon (sword | club | dagger | spear | gun )\r\n" : "", pants ? "create pants\r\n" : "", gi ? "create gi\r\n" : "", wrist ? "create wristband\r\n" : "", boots ? "create boots\r\n" : "", clothes ? "create clothesbeam (target)\r\n" : "", shuriken ? "create shuriken\r\n" : "", senzu ? "create senzu\r\n" : "", kachin ? "create kachin\r\n" : "", boost ? "create elixir\r\n" : "");
        return;
    }
    reveal_hiding(ch, 0);
    if (!(strcmp(arg, "food")))
    {
        cost = GET_MAX_MANA(ch) / (skill / 2);
        cost *= discount;

        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            if (!*arg2)
            {
                ch->sendText("Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | highest)\r\n");
                ch->sendText("If you are capable you will make it. If not you will make a low quality version.\r\n");
            }
            else if (*arg2)
            {
                if (!strcasecmp(arg2, "highest") && skill >= 100)
                {
                    level = 4;
                }
                else if (!strcasecmp(arg2, "high") && skill >= 75)
                {
                    level = 3;
                }
                else if (!strcasecmp(arg2, "mid") && skill >= 50)
                {
                    level = 2;
                }
                else
                {
                    level = 1;
                }
            }
            if (level == 4)
            {
                obj = read_object(1512, VIRTUAL);
            }
            else if (level == 3)
            {
                obj = read_object(1511, VIRTUAL);
            }
            else if (level == 2)
            {
                obj = read_object(1510, VIRTUAL);
            }
            else
            {
                obj = read_object(70, VIRTUAL);
            }
            ch->addToInventory(obj);
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "water")))
    {
        cost = GET_MAX_MANA(ch) / (skill * 2);
        cost *= discount;

        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            if (!*arg2)
            {
                ch->sendText("Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | highest)\r\n");
                ch->sendText("If you are capable you will make it. If not you will make a low quality version.\r\n");
            }
            else if (*arg2)
            {
                if (!strcasecmp(arg2, "highest") && skill >= 100)
                {
                    level = 4;
                }
                else if (!strcasecmp(arg2, "high") && skill >= 75)
                {
                    level = 3;
                }
                else if (!strcasecmp(arg2, "mid") && skill >= 50)
                {
                    level = 2;
                }
                else
                {
                    level = 1;
                }
            }
            if (level == 4)
            {
                obj = read_object(1515, VIRTUAL);
            }
            else if (level == 3)
            {
                obj = read_object(1514, VIRTUAL);
            }
            else if (level == 2)
            {
                obj = read_object(1513, VIRTUAL);
            }
            else
            {
                obj = read_object(71, VIRTUAL);
            }
            ch->addToInventory(obj);
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "bag")))
    {
        cost = GET_MAX_MANA(ch) / (skill * 2);
        cost *= discount;

        if (bag == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(319, VIRTUAL);
            ch->addToInventory(obj);
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "mattress")))
    {
        cost = GET_MAX_MANA(ch) / skill;
        cost *= discount;

        if (mattress == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(16, VIRTUAL);
            ch->addToInventory(obj); // cooldown removed on 10/24/2021
            reveal_hiding(ch, 0);    // ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "weapon")))
    {
        cost = GET_MAX_MANA(ch) / 5;
        cost *= discount;

        if (sword == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            if (!*arg2)
            {
                ch->sendText("What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
                return;
            }
            if (!*arg3)
            {
                ch->sendText("Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | higher | highest)\r\n");
                ch->sendText("If you are capable you will make it. If not you will make a low quality version.\r\n");
            }
            else if (*arg3)
            {
                if (!strcasecmp(arg3, "highest") && skill >= 100)
                {
                    level = 5;
                }
                else if (!strcasecmp(arg3, "higher") && skill >= 75)
                {
                    level = 4;
                }
                else if (!strcasecmp(arg3, "high") && skill >= 50)
                {
                    level = 3;
                }
                else if (!strcasecmp(arg3, "mid") && skill >= 30)
                {
                    level = 2;
                }
                else
                {
                    level = 1;
                }
            }
            if (!strcasecmp(arg2, "sword"))
            {
                if (level == 5)
                {
                    obj = read_object(1519, VIRTUAL);
                }
                else if (level == 4)
                {
                    obj = read_object(1518, VIRTUAL);
                }
                else if (level == 3)
                {
                    obj = read_object(1517, VIRTUAL);
                }
                else if (level == 2)
                {
                    obj = read_object(1516, VIRTUAL);
                }
                else
                {
                    obj = read_object(90, VIRTUAL);
                }
            }
            else if (!strcasecmp(arg2, "dagger"))
            {
                if (level == 5)
                {
                    obj = read_object(1540, VIRTUAL);
                }
                else if (level == 4)
                {
                    obj = read_object(1539, VIRTUAL);
                }
                else if (level == 3)
                {
                    obj = read_object(1538, VIRTUAL);
                }
                else if (level == 2)
                {
                    obj = read_object(1537, VIRTUAL);
                }
                else
                {
                    obj = read_object(1536, VIRTUAL);
                }
            }
            else if (!strcasecmp(arg2, "club"))
            {
                if (level == 5)
                {
                    obj = read_object(1545, VIRTUAL);
                }
                else if (level == 4)
                {
                    obj = read_object(1544, VIRTUAL);
                }
                else if (level == 3)
                {
                    obj = read_object(1543, VIRTUAL);
                }
                else if (level == 2)
                {
                    obj = read_object(1542, VIRTUAL);
                }
                else
                {
                    obj = read_object(1541, VIRTUAL);
                }
            }
            else if (!strcasecmp(arg2, "spear"))
            {
                if (level == 5)
                {
                    obj = read_object(1550, VIRTUAL);
                }
                else if (level == 4)
                {
                    obj = read_object(1549, VIRTUAL);
                }
                else if (level == 3)
                {
                    obj = read_object(1548, VIRTUAL);
                }
                else if (level == 2)
                {
                    obj = read_object(1547, VIRTUAL);
                }
                else
                {
                    obj = read_object(1546, VIRTUAL);
                }
            }
            else if (!strcasecmp(arg2, "gun"))
            {
                if (level == 5)
                {
                    obj = read_object(1555, VIRTUAL);
                }
                else if (level == 4)
                {
                    obj = read_object(1554, VIRTUAL);
                }
                else if (level == 3)
                {
                    obj = read_object(1553, VIRTUAL);
                }
                else if (level == 2)
                {
                    obj = read_object(1552, VIRTUAL);
                }
                else
                {
                    obj = read_object(1551, VIRTUAL);
                }
            }
            else
            {
                ch->sendText("What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
                return;
            }
            ch->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(ch));
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "clothesbeam")))
    {
        cost = GET_MAX_MANA(ch) / 2;
        cost *= discount;

        if (clothes == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        if (!*arg2)
        {
            ch->sendText("Who do you want to hit with clothesbeam?\r\nSyntax: create clothesbeam (target)\r\n");
            return;
        }

        Character *vict = nullptr;

        if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_ROOM)))
        {
            ch->sendText("Clothesbeam who?\r\nSyntax: create clothesbeam (target)\r\n");
            return;
        }

        if (vict->master != ch)
        {
            ch->sendText("They must be following you first.\r\n");
            return;
        }
        else
        {
            obj = read_object(92, VIRTUAL); /* gi */
            boost_obj(obj, ch, 0);
            vict->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(vict));
            obj = read_object(91, VIRTUAL); /* pants */
            boost_obj(obj, ch, 0);
            vict->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(vict));
            obj = read_object(1528, VIRTUAL); /* wrist */
            boost_obj(obj, ch, 0);
            vict->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(vict));
            obj = read_object(1528, VIRTUAL); /* wrist */
            boost_obj(obj, ch, 0);
            vict->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(vict));
            obj = read_object(1532, VIRTUAL); /* boots */
            boost_obj(obj, ch, 0);
            vict->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(vict));
            do_wear(vict, "all", 0, 0);
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "gi")))
    {
        cost = GET_MAX_MANA(ch) / 5;
        cost *= discount;
        if (gi == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(92, VIRTUAL);
            boost_obj(obj, ch, 0);
            ch->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(ch));
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "shuriken")))
    {
        cost = GET_MAX_MANA(ch) / 4;
        cost *= discount;

        if (shuriken == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(19053, VIRTUAL);
            ch->addToInventory(obj);
            for (auto f : {ITEM_NORENT, ITEM_NOSELL})
                obj->item_flags.set(f, true);
            obj->size = static_cast<Size>(get_size(ch));
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "pants")))
    {
        cost = GET_MAX_MANA(ch) / 5;
        cost *= discount;

        if (pants == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(91, VIRTUAL);
            boost_obj(obj, ch, 0);
            ch->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(ch));
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "wristband")))
    {
        cost = GET_MAX_MANA(ch) / 5;
        cost *= discount;

        if (wrist == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(1528, VIRTUAL);
            boost_obj(obj, ch, 0);
            ch->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(ch));
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "boots")))
    {
        cost = GET_MAX_MANA(ch) / 5;
        cost *= discount;

        if (boots == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(1532, VIRTUAL);
            boost_obj(obj, ch, 0);
            ch->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(ch));
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "light")))
    {
        cost = GET_MAX_MANA(ch) / (skill * 2);
        cost *= discount;

        if (light == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(72, VIRTUAL);
            ch->addToInventory(obj);
            obj->size = static_cast<Size>(get_size(ch));
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "kachin")))
    {
        cost = GET_MAX_MANA(ch) - 1;
        cost *= discount;

        if (kachin == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(87, VIRTUAL);
            obj->moveToLocation(ch);
            obj->size = static_cast<Size>(get_size(ch));
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            return;
        }
    }
    else if (!(strcmp(arg, "elixir")))
    {
        cost = GET_MAX_MANA(ch) - 1;
        cost *= discount;

        if (boost == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s\r\n", arg);
            return;
        }
        if (GET_HIT(ch) < GET_MAX_HIT(ch))
        {
            ch->send_to("You need to be at full powerlevel to create %s\r\n", arg);
            return;
        }
        else if (GET_PRACTICES(ch) < 10)
        {
            ch->send_to("You do not have enough PS to create %s, you need at least 10.\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(86, VIRTUAL);
            obj->moveToLocation(ch);
            obj->size = static_cast<Size>(get_size(ch));
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            ch->modCurVitalDam(CharVital::health, 1);
            ch->modPractices(-10);
            return;
        }
    }
    else if (!(strcmp(arg, "senzu")))
    {
        cost = GET_MAX_MANA(ch);
        int64_t cost2 = (ch->getEffectiveStat<int64_t>("health")) - 1;

        if (senzu == false)
        {
            ch->sendText("What do you want to create?\r\n");
            return;
        }
        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->send_to("You do not have enough ki to create %s, you need full ki.\r\n", arg);
            return;
        }
        else if (GET_HIT(ch) <= cost2)
        {
            ch->send_to("You do not have enough powerlevel to create %s, you need to be at full.\r\n", arg);
            return;
        }
        else if ((ch->getCurVital(CharVital::stamina)) < GET_MAX_MOVE(ch))
        {
            ch->send_to("You do not have enough stamina to create %s, you need to be at full.\r\n", arg);
            return;
        }
        else if (GET_PRACTICES(ch) < 50)
        {
            ch->send_to("You do not have enough PS to create %s, you need at least 50.\r\n", arg);
            return;
        }
        else
        {
            obj = read_object(1, VIRTUAL);
            ch->addToInventory(obj);
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
            act("$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            ch->modCurVital(CharVital::health, -cost2);
            ch->modCurVitalDam(CharVital::stamina, 1);
            ch->modPractices(-50);
            return;
        }
    }
    else
    {
        ch->sendText("Create what?\r\n");
        return;
    }
}

ACMD(do_recharge)
{

    if (IS_NPC(ch) || !IS_ANDROID(ch))
    {
        ch->sendText("Only androids can use recharge\r\n");
        return;
    }

    if (GET_COOLDOWN(ch) > 0)
    {
        ch->sendText("You must wait a short period before your nanites can convert your ki.\r\n");
        return;
    }

    if (ch->subrace != SubRace::android_model_repair)
    {
        ch->sendText("You are not a repair model android.\r\n");
        return;
    }
    else
    {
        int64_t cost = 0;

        cost = GET_MAX_MOVE(ch) / 20;

        if ((ch->getCurVital(CharVital::ki)) < cost)
        {
            ch->sendText("You do not have enough ki to recharge your stamina.\r\n");
            return;
        }
        else if ((ch->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(ch))
        {
            ch->sendText("Your energy reserves are already full.\r\n");
            return;
        }
        else
        {
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You focus your ki into your energy reserves, recharging them some.", true, ch, nullptr, nullptr,
                TO_CHAR);
            act("$n stops and glows green briefly.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->modCurVital(CharVital::ki, -cost);
            if ((ch->getCurVital(CharVital::stamina)) + (cost * 2) < GET_MAX_MOVE(ch))
            {
                ch->modCurVital(CharVital::stamina, cost * 2);
            }
            else
            {
                ch->restoreVital(CharVital::stamina);
                ch->sendText("You are fully recharged now.\r\n");
            }
            WAIT_STATE(ch, PULSE_2SEC);
        }
    }
}

ACMD(do_srepair)
{
    int i;

    if (!IS_ANDROID(ch))
    {
        ch->sendText("Only androids can use repair, maybe you want 'fix' instead?\r\n");
        return;
    }

    if (GET_COOLDOWN(ch) > 0)
    {
        ch->sendText("You must wait a short period before your nanites can repair you.\r\n");
        return;
    }

    if (!IS_NPC(ch) && ch->subrace != SubRace::android_model_repair)
    {
        ch->sendText("You are not a repair model android.\r\n");
        return;
    }
    else
    {
        int64_t cost = 0, heal = 0;

        cost = GET_MAX_HIT(ch) / 40;

        if ((ch->getCurVital(CharVital::stamina)) < cost)
        {
            ch->sendText("You do not have enough stamina to repair yourself.\r\n");
            return;
        }
        else if (GET_HIT(ch) >= (ch->getEffectiveStat<int64_t>("health")))
        {
            ch->sendText("You are already at full functionality and do not require repairs.\r\n");
            return;
        }
        else
        {
            reveal_hiding(ch, 0);
            ch->setBaseStat("concentrate_cooldown", 10);
            act("You repair some of your outer casings and internal systems, with the small nano-robots contained in your body.",
                true, ch, nullptr, nullptr, TO_CHAR);
            act("$n stops a moment as small glowing particles move across $s body.", true, ch, nullptr, nullptr,
                TO_ROOM);

            int repaired = false;
            if (!IS_NPC(ch))
            {
                for (i = 0; i < NUM_WEARS; i++)
                {
                    if (GET_EQ(ch, i))
                    {
                        if (MOD_OBJ_VAL(GET_EQ(ch, i), VAL_ALL_HEALTH, 20) > 100)
                        {
                            SET_OBJ_VAL(GET_EQ(ch, i), VAL_ALL_HEALTH, 100);
                        }
                        GET_EQ(ch, i)->item_flags.set(ITEM_BROKEN, false);
                        repaired = true;
                    }
                }
            }

            if (repaired == true)
            {
                ch->sendText("@GYour nano-robots also repair all of your equipment a little bit.@n\r\n");
            }
            ch->modCurVital(CharVital::stamina, -cost);
            heal = cost * 2;
            if (GET_BONUS(ch, BONUS_HEALER) > 0)
            {
                heal += heal * .25;
            }

            if (ch->modCurVital(CharVital::health, heal) == ch->getEffectiveStat<int64_t>("health"))
            {
                ch->sendText("You are fully repaired now.\r\n");
            }

            if (!IS_NPC(ch) && rand_number(1, 3) == 2 && (ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch))
            {
                ch->sendText("@GThe repairs have managed to relink power reserves and boost your current energy level.@n\r\n");
                ch->modCurVital(CharVital::ki, cost);
            }
            WAIT_STATE(ch, PULSE_2SEC);
        }
    }
}

ACMD(do_upgrade)
{
    int count = 0, bonus = 0, cost = 0;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (IS_NPC(ch) || !IS_ANDROID(ch))
    {
        ch->sendText("You are not an android!\r\n");
        return;
    }

    if (!*arg)
    {
        if (ch->subrace != SubRace::android_model_absorb)
        {
            ch->sendText("@c--------@D[@rUpgrade Menu@D]@c--------\r\n"
                         "@cUpgrade @RPowerlevel@D: @Y75 @WPoints\r\n"
                         "@cUpgrade @CKi        @D: @Y40 @WPoints\r\n"
                         "@cUpgrade @GStamina   @D: @Y50 @WPoints\r\n"
                         "@D            -----------\r\n");
        }
        ch->send_to("@cAugment @RPowerlevel\r\n"
                    "@cAugment @CKi\r\n"
                    "@cAugment @GStamina\r\n"
                    "@WCurrent Upgrade Points @D[@y%s@D]@n\r\n",
                    add_commas(GET_UP(ch)).c_str());
        return;
    }

    if (!strcasecmp("augment", arg))
    {
        Object *obj = nullptr;
        int64_t gain = 0;
        if (GET_WIS(ch) < 80)
        {
            ch->sendText("You need to be at least Wisdom 80 to use these kits.\r\n");
            return;
        }
        if (!(obj = get_obj_in_list_vis(ch, "Augmentation", nullptr, ch->getInventory())))
        {
            ch->sendText("You don't have a Circuit Augmentation Kit.\r\n");
            return;
        }
        else
        {
            switch (GET_WIS(ch))
            { /* R: Original was GET_LEVEL(ch) */
            case 80:
                gain = GET_MAX_HIT(ch) * 0.005;
                break;
            case 81:
            case 82:
            case 83:
            case 84:
            case 85:
            case 86:
            case 87:
            case 88:
            case 89:
            case 90:
                gain = GET_MAX_HIT(ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 2000; */
                break;
            case 91:
            case 92:
            case 93:
            case 94:
            case 95:
            case 96:
            case 97:
            case 98:
            case 99:
                gain = GET_MAX_HIT(ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 4000; */
                break;
            case 100:
                gain = GET_MAX_HIT(ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 100000 */
                if (gain > 10000000)
                {
                    gain = 10000000;
                }
                break;
            }
            if (!strcasecmp("health", arg2))
            {
                obj->clearLocation();
                extract_obj(obj);
                act("@WYou install the circuits and upgrade your maximum health.@n", true, ch, nullptr, nullptr,
                    TO_CHAR);
                act("@C$n@W installs some circuits and upgrades $s systems.@n", true, ch, nullptr, nullptr, TO_ROOM);
                ch->gainBaseStat("health", gain);
                ch->send_to("@gGain @D[@G+%s@D]\r\n", add_commas(gain).c_str());
                return;
            }
            else if (!strcasecmp("ki", arg2))
            {
                obj->clearLocation();
                extract_obj(obj);
                act("@WYou install the circuits and upgrade your maximum ki.@n", true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@W installs some circuits and upgrades $s systems.@n", true, ch, nullptr, nullptr, TO_ROOM);
                ch->gainBaseStat("ki", gain);
                ch->send_to("@gGain @D[@G+%s@D]\r\n", add_commas(gain).c_str());
                return;
            }
            else if (!strcasecmp("stamina", arg2))
            {
                obj->clearLocation();
                extract_obj(obj);
                act("@WYou install the circuits and upgrade your maximum stamina.@n", true, ch, nullptr, nullptr,
                    TO_CHAR);
                act("@C$n@W installs some circuits and upgrades $s systems.@n", true, ch, nullptr, nullptr, TO_ROOM);
                ch->gainBaseStat("stamina", gain);
                ch->send_to("@gGain @D[@G+%s@D]\r\n", add_commas(gain).c_str());
                return;
            }
            else
            {
                ch->sendText("What do you want to augment? Powerlevel, ki, or stamina?\r\n");
                return;
            }
        }
    }

    if (ch->subrace == SubRace::android_model_absorb)
    {
        ch->sendText("You are an absorb model and can only upgrade with augmentation kits.\r\n");
        return;
    }

    if (ch->is_soft_cap(0))
    {
        ch->sendText("@mYou are unable to spend anymore UGP right now (Softcap)@n\r\n");
        return;
    }

    if (!*arg2 && (!strcasecmp("health", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg)))
    {
        ch->send_to("How many times do you want to increase %s?", arg);
        return;
    }

    if (atoi(arg2) <= 0 && (!strcasecmp("health", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg)))
    {
        ch->sendText("It needs to be between 1-1000\r\n");
        return;
    }

    if (atoi(arg2) > 1000 &&
        (!strcasecmp("health", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg)))
    {
        ch->sendText("It needs to be between 1-1000\r\n");
        return;
    }

    if (!strcasecmp("health", arg))
    {
        count = atoi(arg2);
        auto chCon = GET_CON(ch);
        while (count > 0)
        {
            if (chCon >= 90)
            {
                bonus += chCon * 5000;
            }
            else if (chCon >= 80)
            {
                bonus += chCon * 2500;
            }
            else if (chCon >= 70)
            {
                bonus += chCon * 2000;
            }
            else if (chCon >= 60)
            {
                bonus += chCon * 1300;
            }
            else if (chCon >= 60)
            {
                bonus += chCon * 1200;
            }
            else if (chCon >= 50)
            {
                bonus += chCon * 500;
            }
            else if (chCon >= 25)
            {
                bonus += chCon * 250;
            }
            else
            {
                bonus += chCon * 150;
            }
            cost += 75;
            count--;
        }
        if (cost > GET_UP(ch))
        {
            ch->send_to("You need %s upgrade points, and only have %s.\r\n", add_commas(cost).c_str(), add_commas(GET_UP(ch)).c_str());
            return;
        }
        else if (ch->is_soft_cap(bonus))
        {
            ch->sendText("@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
            return;
        }
        else
        {
            ch->modBaseStat<int>("upgrade_points", -cost);
            ch->send_to("You upgrade your system and gain %s %s!", add_commas(bonus).c_str(), arg);
            ch->gainBaseStat("health", bonus);
        }
    }
    else if (!strcasecmp("ki", arg))
    {
        count = atoi(arg2);
        auto chWis = GET_WIS(ch);
        while (count > 0)
        {
            if (chWis >= 90)
            {
                bonus += chWis * 3650;
            }
            else if (chWis >= 80)
            {
                bonus += chWis * 2450;
            }
            else if (chWis >= 70)
            {
                bonus += chWis * 1800;
            }
            else if (chWis >= 60)
            {
                bonus += chWis * 1250;
            }
            else if (chWis >= 60)
            {
                bonus += chWis * 1150;
            }
            else if (chWis >= 50)
            {
                bonus += chWis * 400;
            }
            else if (chWis >= 25)
            {
                bonus += chWis * 200;
            }
            else
            {
                bonus += chWis * 120;
            }
            cost += 40;
            count--;
        }
        if (cost > GET_UP(ch))
        {
            ch->send_to("You need %s upgrade points, and only have %s.\r\n", add_commas(cost).c_str(), add_commas(GET_UP(ch)).c_str());
            return;
        }
        else if (ch->is_soft_cap(bonus))
        {
            ch->sendText("@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
            return;
        }
        else
        {
            ch->modBaseStat<int>("upgrade_points", -cost);
            ch->send_to("You upgrade your system and gain %s %s!", add_commas(bonus).c_str(), arg);
            ch->gainBaseStat("ki", bonus);
        }
    }
    else if (!strcasecmp("stamina", arg))
    {
        count = atoi(arg2);
        auto chCon = GET_CON(ch);
        while (count > 0)
        {
            if (chCon >= 90)
            {
                bonus += chCon * 3650;
            }
            else if (chCon >= 80)
            {
                bonus += chCon * 2450;
            }
            else if (chCon >= 70)
            {
                bonus += chCon * 1800;
            }
            else if (chCon >= 60)
            {
                bonus += chCon * 1250;
            }
            else if (chCon >= 60)
            {
                bonus += chCon * 1150;
            }
            else if (chCon >= 50)
            {
                bonus += chCon * 500;
            }
            else if (chCon >= 25)
            {
                bonus += chCon * 200;
            }
            else
            {
                bonus += chCon * 120;
            }
            cost += 50;
            count--;
        }
        if (cost > GET_UP(ch))
        {
            ch->send_to("You need %s upgrade points, and only have %s.\r\n", add_commas(cost).c_str(), add_commas(GET_UP(ch)).c_str());
            return;
        }
        else if (ch->is_soft_cap(bonus))
        {
            ch->sendText("@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
            return;
        }
        else
        {
            ch->modBaseStat<int>("upgrade_points", -cost);
            ch->send_to("You upgrade your system and gain %s %s!", add_commas(bonus).c_str(), arg);
            ch->gainBaseStat("stamina", bonus);
        }
    }
    else
    {
        ch->sendText("That is not a valid upgrade option.\r\n");
        return;
    }
}

ACMD(do_ingest)
{

    if (IS_MAJIN(ch))
    {
        Character *vict;
        char arg[MAX_INPUT_LENGTH];

        one_argument(argument, arg);

        if (!*arg)
        {
            ch->sendText("Who do you want to ingest?\r\n");
            return;
        }

        if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
        {
            ch->sendText("Ingest who?\r\n");
            return;
        }
        if (!can_kill(ch, vict, nullptr, 0))
        {
            return;
        }
        if (ABSORBBY(vict))
        {
            ch->send_to("%s is already absorbing from them!", GET_NAME(ABSORBBY(vict)));
            return;
        }
        if (GET_ABSORBS(ch) > 3)
        {
            ch->sendText("You already have already ingested 4 people.\r\n");
            return;
        }
        auto chCon = GET_CON(ch);
        if (chCon < 25)
        {
            ch->sendText("You can't ingest yet.\r\n");
            return;
        }
        if (chCon < 100 && chCon >= 75 && GET_ABSORBS(ch) == 3)
        {
            ch->sendText("You already have ingested as much as you can. You'll have to get more experienced.\r\n");
            return;
        }
        if (chCon < 75 && chCon >= 50 && GET_ABSORBS(ch) == 2)
        {
            ch->sendText("You already have ingested as much as you can. You'll have to get more experienced.\r\n");
            return;
        }
        if (chCon < 50 && chCon >= 25 && GET_ABSORBS(ch) == 1)
        {
            ch->sendText("You already have ingested as much as you can. You'll have to get more experienced.\r\n");
            return;
        }

        if (GET_MAX_HIT(vict) >= (ch->getBaseStat<int64_t>("health")) * 3)
        {
            ch->sendText("You are too weak to ingest them into your body!\r\n");
            return;
        }
        if (AFF_FLAGGED(vict, AFF_SANCTUARY))
        {
            ch->sendText("You can't ingest them, they have a barrier!\r\n");
            return;
        }
        reveal_hiding(ch, 0);
        if (AFF_FLAGGED(vict, AFF_ZANZOKEN) && (vict->getCurVital(CharVital::stamina)) >= 1 && GET_POS(vict) != POS_SLEEPING)
        {
            act("@C$N@c disappears, avoiding your attempted ingestion!@n", false, ch, nullptr, vict, TO_CHAR);
            act("@cYou disappear, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n", false, ch, nullptr,
                vict, TO_VICT);
            act("@C$N@c disappears, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n", false, ch,
                nullptr, vict, TO_NOTVICT);
            vict->affect_flags.set(AFF_ZANZOKEN, false);
            WAIT_STATE(ch, PULSE_3SEC);
            return;
        }
        if (GET_SPEEDI(ch) + rand_number(1, 5) < GET_SPEEDI(ch) + rand_number(1, 5))
        {
            act("@WYou fling a piece of goo at @c$N@W, and try to ingest $M! $E manages to avoid your blob of goo though!@n",
                true, ch, nullptr, vict, TO_CHAR);
            act("@C$n@W flings a piece of goo at you, you manage to avoid it though!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@C$n@w flings a piece of goo at @c$N@W, but the goo misses $M@W!@n", true, ch, nullptr, vict,
                TO_NOTVICT);
            WAIT_STATE(ch, PULSE_3SEC);
            return;
        }
        else
        {
            act("@WYou flings a piece of goo at @c$N@W! The goo engulfs $M and then returns to your body!@n", true, ch,
                nullptr, vict, TO_CHAR);
            act("@C$n@W flings a piece of goo at you! The goo engulfs your body and then returns to @C$n@W!@n", true,
                ch, nullptr, vict, TO_VICT);
            act("@C$n@w flings a piece of goo at @c$N@W! The goo engulfs $M and then return to @C$n@W!@n", true, ch,
                nullptr, vict, TO_NOTVICT);
            ch->modBaseStat<int>("absorbs", 1);
            int64_t pl = (vict->getBaseStat<int64_t>("health")) / 6;
            int64_t stam = (vict->getBaseStat<int64_t>("stamina")) / 6;
            int64_t ki = (vict->getBaseStat<int64_t>("ki")) / 6;
            ch->gainBaseStat("health", pl);
            ch->gainBaseStat("stamina", stam);
            ch->gainBaseStat("ki", ki);
            if (!IS_NPC(vict) && !IS_NPC(ch))
            {
                send_to_imm("[PK] %s killed %s at room [%d]\r\n", GET_NAME(ch), GET_NAME(vict),
                            vict->location.getVnum());
                vict->player_flags.set(PLR_ABSORBED, true);
            }
            ch->send_to("@D[@mINGEST@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl).c_str(), add_commas(ki).c_str(), add_commas(stam).c_str());
            if (rand_number(1, 3) == 3)
            {
                ch->send_to("You get %s's eye color.\r\n", GET_NAME(vict));
                // ch->set(CharAppearance::eye_color, GET_EYE(vict));
            }
            else if (rand_number(1, 3) == 3)
            {
                ch->send_to("%s changes your height.\r\n", GET_NAME(vict));
                if (GET_PC_HEIGHT(ch) > GET_PC_HEIGHT(vict))
                {
                    ch->modBaseStat("height", -((GET_PC_HEIGHT(ch) - GET_PC_HEIGHT(vict)) / 2));
                }
                else if (GET_PC_HEIGHT(ch) < GET_PC_HEIGHT(vict))
                {
                    ch->modBaseStat("height", ((GET_PC_HEIGHT(vict) - GET_PC_HEIGHT(ch)) / 2));
                }
                else
                {
                    ch->setBaseStat("height", GET_PC_HEIGHT(vict));
                }
            }
            else if (rand_number(1, 3) == 3)
            {
                ch->send_to("%s changes your weight.\r\n", GET_NAME(vict));
                auto chw = ch->getBaseStat("weight");
                auto vw = vict->getBaseStat("weight");
                if (chw > vw)
                {
                    ch->modBaseStat("weight", -((chw - vw) / 2));
                }
                else if (chw < vw)
                {
                    ch->modBaseStat("weight", ((vw - chw) / 2));
                }
                else
                {
                    ch->setBaseStat("weight", vict->getBaseStat("weight"));
                }
            }
            else
            {
                ch->send_to("Your forelock length changes because of %s.\r\n", GET_NAME(vict));
                // ch->set(CharAppearance::hair_length, GET_HAIRL(vict));
            }
            handle_ingest_learn(ch, vict);
            die(vict, nullptr);
            return;
        }
    } // End of ingest

    else
    {
        ch->sendText("You are not a majin, you can not ingest.\r\n");
        return;
    } // Error
}

ACMD(do_absorb)
{
    Character *vict = nullptr;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!check_skill(ch, SKILL_ABSORB))
    {
        return;
    }

    if (IS_ANDROID(ch))
    {
        if (!limb_ok(ch, 0))
        {
            return;
        }
    }
    if (!IS_NPC(ch))
    {
        if (IS_BIO(ch) && !ch->character_flags.get(CharacterFlag::tail))
        {
            ch->sendText("You have no tail!\r\n");
            return;
        }
    }

    if (!IS_ANDROID(ch) && !IS_BIO(ch))
    {
        ch->sendText("You shouldn't have this skill, you are incapable of absorbing.\r\n");
        send_to_imm("ERROR: Absorb skill on %s when they are not a bio or android.", GET_NAME(ch));
        return;
    }

    if (FIGHTING(ch) && !IS_ANDROID(ch))
    {
        ch->sendText("You are too busy fighting!\r\n");
        return;
    }

    if (GRAPPLED(ch))
    {
        ch->sendText("You are currently being grappled with! Try 'escape'!\r\n");
        return;
    }
    if (GRAPPLING(ch))
    {
        ch->sendText("You are currently grappling with someone!\r\n");
        return;
    }

    if (ABSORBING(ch))
    {
        act("@WYou stop absorbing from @c$N@W!@n", true, ch, nullptr, ABSORBING(ch), TO_CHAR);
        act("$n stops absorbing from you!", true, ch, nullptr, ABSORBING(ch), TO_VICT);
        act("$n stops absorbing from $N!", true, ch, nullptr, ABSORBING(ch), TO_NOTVICT);
        if (IS_NPC(ABSORBING(ch)) && !FIGHTING(ABSORBING(ch)))
        {
            set_fighting(ABSORBING(ch), ch);
        }
        ABSORBBY(ABSORBING(ch)) = nullptr;
        ABSORBING(ch) = nullptr;
        characterSubscriptions.unsubscribe("androidAbsorbSystem", ch);
    }

    if (!*arg && IS_ANDROID(ch))
    {
        ch->sendText("Who do you want to absorb?\r\n");
        return;
    }

    if (IS_ANDROID(ch))
    {
        if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
        {
            ch->send_to("Absorb %s?\r\n", IS_ANDROID(ch) ? "from who" : "who");
            return;
        }
    }
    if (IS_BIO(ch))
    {
        if (!*arg)
        {
            ch->sendText("Syntax: absorb (swallow | extract) (target)\r\n");
            return;
        }
        else if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_ROOM)))
        {
            ch->sendText("Syntax: absorb (swallow | extract) (target)\r\n");
            return;
        }
    }
    if (AFF_FLAGGED(vict, AFF_SANCTUARY))
    {
        ch->sendText("You can't absorb them, they have a barrier!\r\n");
        return;
    }

    if (IS_ANDROID(ch))
    {
        if (!IS_NPC(ch))
        {
            if (ch->subrace != SubRace::android_model_absorb)
            {
                ch->sendText("You are not an absorbtion model.\r\n");
                return;
            }
        }
        if (!can_kill(ch, vict, nullptr, 0))
        {
            return;
        }
        if (ABSORBBY(vict))
        {
            ch->send_to("%s is already absorbing from them!", GET_NAME(ABSORBBY(vict)));
            return;
        }
        if (GET_MAX_HIT(vict) > GET_MAX_HIT(ch) * 2)
        {
            ch->sendText("They are too strong for you to absorb from.\r\n");
            return;
        }
        if (GET_MAX_HIT(vict) * 20 < GET_MAX_HIT(ch))
        {
            ch->sendText("They are too weak for you to bother absorbing from.\r\n");
            return;
        }
        if ((vict->getCurVital(CharVital::stamina)) < (GET_MAX_MOVE(vict) / 20) && (vict->getCurVital(CharVital::ki)) < (GET_MAX_MANA(vict) / 20))
        {
            ch->sendText("They have nothing to absorb right now, they are drained...\r\n");
            return;
        }
        reveal_hiding(ch, 0);
        if (init_skill(ch, SKILL_ABSORB) < axion_dice(0))
        {
            act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", true, ch, nullptr,
                vict, TO_CHAR);
            act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", true, ch,
                nullptr, vict, TO_NOTVICT);
            improve_skill(ch, SKILL_ABSORB, 1);
            if (IS_NPC(vict) && IS_HUMANOID(vict) && rand_number(1, 3) == 3)
            {
                if (FIGHTING(ch) == nullptr)
                {
                    set_fighting(ch, vict);
                }
                if (FIGHTING(vict) == nullptr)
                {
                    set_fighting(vict, ch);
                }
            }
            WAIT_STATE(ch, PULSE_3SEC);
            return;
        }
        else
        {
            act("@WYou rush at @c$N@W and try to absorb from them, and manage to grab on!@n", true, ch, nullptr, vict,
                TO_CHAR);
            act("@C$n@W rushes at you and tries to grab you, and manages to grab on!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@C$n@w rushes at @c$N@W and tries to grab $M, and manages to grab on!@n", true, ch, nullptr, vict,
                TO_NOTVICT);
            improve_skill(ch, SKILL_ABSORB, 1);
            ABSORBING(ch) = vict;
            ABSORBBY(vict) = ch;
            characterSubscriptions.subscribe("androidAbsorbSystem", ch);
            WAIT_STATE(ch, PULSE_3SEC);
            return;
        }
    } // End of android absorb

    else if (IS_BIO(ch) && !(strcmp(arg, "swallow")))
    {
        if (ABSORBBY(vict))
        {
            ch->send_to("%s is already absorbing from them!", GET_NAME(ABSORBBY(vict)));
            return;
        }
        if (!can_kill(ch, vict, nullptr, 0))
        {
            return;
        }
        if (GET_ABSORBS(ch) < 1)
        {
            ch->sendText("You already have already absorbed 3 people.\r\n");
            return;
        }
        if (GET_MAX_HIT(vict) >= (ch->getBaseStat<int64_t>("health")) * 3)
        {
            ch->sendText("You are too weak to absorb them into your cellular structure!\r\n");
            return;
        }
        reveal_hiding(ch, 0);
        if (GET_SKILL(ch, SKILL_ABSORB) < axion_dice(0))
        {
            act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", true, ch, nullptr,
                vict, TO_CHAR);
            act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", true, ch,
                nullptr, vict, TO_NOTVICT);
            improve_skill(ch, SKILL_ABSORB, 1);
            if (!FIGHTING(ch))
            {
                set_fighting(ch, vict);
            }
            if (!FIGHTING(vict))
            {
                set_fighting(vict, ch);
            }
            WAIT_STATE(ch, PULSE_3SEC);
        }
        else
        {
            act("@WYou rush at @c$N@W and your tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing $m!@n",
                true, ch, nullptr, vict, TO_CHAR);
            act("@C$n@W rushes at you and $s tail engulfs you! $e quickly sucks your squirming body into $s tail, absorbing you!@n",
                true, ch, nullptr, vict, TO_VICT);
            act("@C$n@w rushes at @c$N@W and $s tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing @c$N@W!@n",
                true, ch, nullptr, vict, TO_NOTVICT);
            ch->modBaseStat<int>("absorbs", -1);

            int64_t stam = (vict->getBaseStat<int64_t>("stamina")) / 5;
            int64_t ki = (vict->getBaseStat<int64_t>("ki")) / 5;
            int64_t pl = (vict->getBaseStat<int64_t>("health")) / 5;

            ch->gainBaseStat("health", pl);
            ch->gainBaseStat("stamina", stam);
            ch->gainBaseStat("ki", ki);

            if (!IS_NPC(vict) && !IS_NPC(ch))
            {
                send_to_imm("[PK] %s killed %s at room [%d]\r\n", GET_NAME(ch), GET_NAME(vict),
                            vict->location.getVnum());
                vict->player_flags.set(PLR_ABSORBED, true);
            }

            ch->send_to("@D[@gABSORB@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl).c_str(), add_commas(ki).c_str(), add_commas(stam).c_str());
            improve_skill(ch, SKILL_ABSORB, 1);
            die(vict, nullptr);
        }
    } // End of bio absorb
    else if (IS_BIO(ch) && !(strcmp(arg, "extract")))
    {
        int failthresh = rand_number(1, 125);
        if (GET_CON(vict) > 99)
        {
            failthresh += (GET_CON(vict) - 95) * 2;
        }
        if (ABSORBBY(vict))
        {
            ch->send_to("%s is already absorbing from them!", GET_NAME(ABSORBBY(vict)));
            return;
        }
        if (!can_kill(ch, vict, nullptr, 0))
        {
            return;
        }
        if (GET_MAX_HIT(vict) >= GET_MAX_HIT(ch))
        {
            ch->sendText("You are too weak to absorb them into your cellular structure!\r\n");
            return;
        }
        if (GET_MAX_HIT(vict) < GET_MAX_HIT(ch) / 5)
        {
            ch->sendText("They would be worthless to you at your strength!\r\n");
            return;
        }
        if (!IS_NPC(vict))
        {
            ch->sendText("You can't absorb their bio extract, you need to swallow them with your tail!\r\n");
            return;
        }
        if (ch->is_soft_cap(0))
        {
            ch->sendText("You can not handle any more bio extract at your current level.\r\n");
            return;
        }
        if (GET_SKILL(ch, SKILL_ABSORB) < failthresh)
        {
            act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", true, ch, nullptr,
                vict, TO_CHAR);
            act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", true, ch, nullptr, vict,
                TO_VICT);
            act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", true, ch,
                nullptr, vict, TO_NOTVICT);
            improve_skill(ch, SKILL_ABSORB, 0);
            if (!FIGHTING(ch))
            {
                set_fighting(ch, vict);
            }
            if (!FIGHTING(vict))
            {
                set_fighting(vict, ch);
            }
            WAIT_STATE(ch, PULSE_4SEC);
        }
        /* Rillao: transloc, add new transes here */
        else
        {
            act("@WYou rush at @c$N@W and stab them with your tail! You quickly suck out all the bio extract you need and leave the empty husk behind!",
                true, ch, nullptr, vict, TO_CHAR);
            act("@C$n@w rushes at @c$N@W and stabs $M with $s tail! $e quickly sucks out all the bio extract and leaves the empty husk of @c$N@W behind!@n",
                true, ch, nullptr, vict, TO_NOTVICT);
            int64_t stam = (vict->getBaseStat<int64_t>("stamina")) / 12000;
            int64_t ki = (vict->getBaseStat<int64_t>("ki")) / 12000;
            int64_t pl = (vict->getBaseStat<int64_t>("health")) / 12000;
            auto chCon = GET_CON(ch);
            stam *= rand_number(chCon / 8, chCon / 4) * ch->getPotential();
            pl *= rand_number(chCon / 8, chCon / 4) * ch->getPotential();
            ki *= rand_number(chCon / 8, chCon / 4) * ch->getPotential();
            stam = std::min<int64_t>(stam, 1500000L);
            ki = std::min<int64_t>(ki, 1500000L);
            pl = std::min<int64_t>(pl, 1500000L);

            ch->gainBaseStat("health", pl);
            ch->gainBaseStat("stamina", stam);
            ch->gainBaseStat("ki", ki);
            ch->modCurVitalDam(CharVital::lifeforce, -.05);
            ch->send_to("@D[@gABSORB@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl).c_str(), add_commas(ki).c_str(), add_commas(stam).c_str());
            improve_skill(ch, SKILL_ABSORB, 0);
            WAIT_STATE(ch, PULSE_4SEC);
            vict->mob_flags.set(MOB_HUSK, true);
            die(vict, ch);
        }
    }
    else
    {
        if (!IS_BIO(ch) && !IS_ANDROID(ch))
        {
            ch->sendText("You have the absorb skill but are incapable of absorbing. This error has been reported.\r\n");
            send_to_imm("ERROR: Absorb attempted by %s even though they are not bio or android.", GET_NAME(ch));
        }
        else
        {
            ch->sendText("Syntax: absorb (extract | swallow) (target)\r\n");
        }
        return;
    } // Error
}

ACMD(do_escape)
{

    if (!ABSORBBY(ch) && !GRAPPLED(ch))
    {
        ch->sendText("You are not in anyone's grasp!\r\n");
        return;
    }
    int num = GET_STR(ch);

    if (ABSORBBY(ch))
    {
        int skill = GET_SKILL(ABSORBBY(ch), SKILL_ABSORB);
        if (GET_HIT(ch) > GET_HIT(ABSORBBY(ch)) * 10)
        {
            num += rand_number(10, 15);
        }
        else if (GET_HIT(ch) > GET_HIT(ABSORBBY(ch)) * 5)
        {
            num += rand_number(6, 10);
        }
        else if (GET_HIT(ch) > GET_HIT(ABSORBBY(ch)) * 2)
        {
            num += rand_number(4, 8);
        }
        else if (GET_HIT(ch) > GET_HIT(ABSORBBY(ch)))
        {
            num += rand_number(2, 5);
        }
        else if (GET_HIT(ch) * 10 <= GET_HIT(ABSORBBY(ch)))
        {
            skill -= rand_number(10, 15);
        }
        else if (GET_HIT(ch) * 5 <= GET_HIT(ABSORBBY(ch)))
        {
            skill -= rand_number(6, 10);
        }
        else if (GET_HIT(ch) * 2 <= GET_HIT(ABSORBBY(ch)))
        {
            skill -= rand_number(4, 8);
        }
        else if (GET_HIT(ch) < GET_HIT(ABSORBBY(ch)))
        {
            skill -= rand_number(2, 5);
        }
        if (num > skill)
        {
            act("@c$N@W manages to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_NOTVICT);
            act("@WYou manage to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_VICT);
            act("@c$N@W manages to break loose of your hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_CHAR);
            if (FIGHTING(ch) == nullptr)
            {
                set_fighting(ch, ABSORBBY(ch));
            }
            if (FIGHTING(ABSORBBY(ch)) == nullptr)
            {
                set_fighting(ABSORBBY(ch), ch);
            }
            ABSORBING(ABSORBBY(ch)) = nullptr;
            ABSORBBY(ch) = nullptr;
        }
        else
        {
            act("@c$N@W struggles to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_NOTVICT);
            act("@WYou struggle to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_VICT);
            act("@c$N@W struggles to break loose of your hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_CHAR);
            if (rand_number(1, 3) == 3)
            {
                int64_t dmg = GET_MAX_HIT(ch) * 0.025;
                hurt(0, 0, ch, ABSORBBY(ch), nullptr, dmg, 0);
                if (GET_POS(ABSORBBY(ch)) == POS_SLEEPING)
                {
                    act("@c$N@W manages to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch,
                        TO_NOTVICT);
                    act("@WYou manage to break loose of @C$n's@W hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_VICT);
                    act("@c$N@W manages to break loose of your hold!@n", true, ABSORBBY(ch), nullptr, ch, TO_CHAR);
                    ABSORBING(ABSORBBY(ch)) = nullptr;
                    ABSORBBY(ch) = nullptr;
                }
            }
            WAIT_STATE(ch, PULSE_2SEC);
        }
    }
    if (GRAPPLED(ch))
    {
        int skill = GET_SKILL(GRAPPLED(ch), SKILL_GRAPPLE);
        if (GET_HIT(ch) > GET_HIT(GRAPPLED(ch)) * 10)
        {
            num += rand_number(10, 15);
        }
        else if (GET_HIT(ch) > GET_HIT(GRAPPLED(ch)) * 5)
        {
            num += rand_number(6, 10);
        }
        else if (GET_HIT(ch) > GET_HIT(GRAPPLED(ch)) * 2)
        {
            num += rand_number(4, 8);
        }
        else if (GET_HIT(ch) > GET_HIT(GRAPPLED(ch)))
        {
            num += rand_number(2, 5);
        }
        else if (GET_HIT(ch) * 10 <= GET_HIT(GRAPPLED(ch)))
        {
            skill -= rand_number(10, 15);
        }
        else if (GET_HIT(ch) * 5 <= GET_HIT(GRAPPLED(ch)))
        {
            skill -= rand_number(6, 10);
        }
        else if (GET_HIT(ch) * 2 <= GET_HIT(GRAPPLED(ch)))
        {
            skill -= rand_number(4, 8);
        }
        else if (GET_HIT(ch) < GET_HIT(GRAPPLED(ch)))
        {
            skill -= rand_number(2, 5);
        }

        if (num > skill)
        {
            if (GRAPTYPE(GRAPPLED(ch)) == 4)
            {
                act("@c$N@M flexes with all $S might and causes your body to explode outward into gooey chunks!@n",
                    true, GRAPPLED(ch), nullptr, ch, TO_CHAR);
                act("@MYou flex with all your might and cause @C$n's@M body to explode outward into gooey chunks!@n",
                    true, GRAPPLED(ch), nullptr, ch, TO_VICT);
                act("@c$N@M flexes with all $S might and causes @C$n's@M body to explode outward into gooey chunks!@n",
                    true, GRAPPLED(ch), nullptr, ch, TO_NOTVICT);

                act("@MYou reform your body mere moments later.@n", true, GRAPPLED(ch), nullptr, nullptr, TO_CHAR);
                act("@C$n@M reforms $s body mere moments later.", true, GRAPPLED(ch), nullptr, nullptr, TO_ROOM);
            }
            else
            {
                act("@c$N@W manages to break loose of @C$n's@W hold!@n", true, GRAPPLED(ch), nullptr, ch, TO_NOTVICT);
                act("@WYou manage to break loose of @C$n's@W hold!@n", true, GRAPPLED(ch), nullptr, ch, TO_VICT);
                act("@c$N@W manages to break loose of your hold!@n", true, GRAPPLED(ch), nullptr, ch, TO_CHAR);
            }
            if (!FIGHTING(ch))
            {
                set_fighting(ch, GRAPPLED(ch));
            }
            if (!FIGHTING(GRAPPLED(ch)))
            {
                set_fighting(GRAPPLED(ch), ch);
            }
            GRAPPLED(ch)->setBaseStat<int>("grapple_type", -1);
            GRAPPLING(GRAPPLED(ch)) = nullptr;
            GRAPPLED(ch) = nullptr;
            ch->setBaseStat<int>("grapple_type", -1);
        }
        else
        {
            act("@c$N@W struggles to break loose of @C$n's@W hold!@n", true, GRAPPLED(ch), nullptr, ch, TO_NOTVICT);
            act("@WYou struggle to break loose of @C$n's@W hold!@n", true, GRAPPLED(ch), nullptr, ch, TO_VICT);
            act("@c$N@W struggles to break loose of your hold!@n", true, GRAPPLED(ch), nullptr, ch, TO_CHAR);
            if (rand_number(1, 3) == 3)
            {
                int64_t dmg = GET_MAX_HIT(ch) * 0.025;
                hurt(0, 0, ch, GRAPPLED(ch), nullptr, dmg, 0);
                if (GET_POS(GRAPPLED(ch)) == POS_SLEEPING)
                {
                    act("@c$N@W manages to break loose of @C$n's@W hold!@n", true, GRAPPLED(ch), nullptr, ch,
                        TO_NOTVICT);
                    act("@WYou manage to break loose of @C$n's@W hold!@n", true, GRAPPLED(ch), nullptr, ch, TO_VICT);
                    act("@c$N@W manages to break loose of your hold!@n", true, GRAPPLED(ch), nullptr, ch, TO_CHAR);
                    GRAPPLED(ch)->setBaseStat<int>("grapple_type", -1);
                    GRAPPLING(GRAPPLED(ch)) = nullptr;
                    GRAPPLED(ch) = nullptr;
                    ch->setBaseStat<int>("grapple_type", -1);
                }
            }
            WAIT_STATE(ch, PULSE_2SEC);
        }
    }
}

ACMD(do_regenerate)
{

    int64_t amt = 0;
    int skill = 0;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    skill = init_skill(ch, SKILL_REGENERATE);

    if (skill < 1)
    {
        ch->sendText("You are incapable of regenerating.\r\n");
        return;
    }
    if (GET_SUPPRESS(ch) > 0)
    {
        skill = GET_SUPPRESS(ch);
    }

    if (!*arg)
    {
        ch->send_to("Regenerate how much PL?\r\nMax percent you can regen: %d\r\nSyntax: regenerate (1 - 100)\r\n", skill);
        return;
    }

    if (GET_HIT(ch) >= (ch->getEffectiveStat<int64_t>("health")))
    {
        ch->sendText("You do not need to regenerate, you are at full health.\r\n");
        return;
    }

    if (GET_SUPPRESS(ch) > 0 && GET_HIT(ch) >= (((ch->getEffectiveStat<int64_t>("health")) / 100) * GET_SUPPRESS(ch)))
    {
        ch->sendText("You do not need to regenerate, you are at full health.\r\n");
        return;
    }

    int num = atoi(arg);

    if (num <= 0)
    {
        ch->sendText("What is the point of that?\r\nSyntax: regenerate (1 - 100)\r\n");
        return;
    }

    if (num > 100)
    {
        ch->send_to("You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
        return;
    }

    if (num > skill)
    {
        ch->send_to("You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
        return;
    }

    if (GET_SUPPRESS(ch) > 0 && num > GET_SUPPRESS(ch))
    {
        ch->send_to("You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
        return;
    }

    amt = ((ch->getEffectiveStat<int64_t>("health")) * 0.01) * num;
    if (amt > 1)
        amt /= 2;

    if (IS_BIO(ch))
    {
        amt = amt * 0.9;
    }

    int64_t life = ((ch->getCurVital(CharVital::lifeforce)) - amt * 0.8), energy = ((ch->getCurVital(CharVital::ki)) - amt * 0.2);

    if ((life <= 0 || energy <= 0) && !IS_NPC(ch))
    {
        ch->sendText("Your life force or ki are too low to regenerate that much.\r\n");
        ch->send_to("@YLF Needed@D: @C%s@w, @YKi Needed@D: @C%s@w.@n\r\n", add_commas(amt * 0.8).c_str(), add_commas(amt * 0.2).c_str());
        return;
    }
    else if (IS_NPC(ch) && energy <= 0)
    {
        return;
    }

    ch->modCurVital(CharVital::health, amt * 2);

    if (!IS_NPC(ch))
        ch->modCurVital(CharVital::lifeforce, -(amt * .8));

    ch->modCurVital(CharVital::ki, -(amt * .2));

    reveal_hiding(ch, 0);

    if (GET_HIT(ch) >= (ch->getEffectiveStat<int64_t>("health")))
    {
        act("You concentrate your ki and regenerate your body completely.", true, ch, nullptr, nullptr, TO_CHAR);
        act("$n concentrates and regenerates $s body completely.", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (amt < GET_MAX_HIT(ch) / 10)
    {
        act("You concentrate your ki and regenerate your body a little.", true, ch, nullptr, nullptr, TO_CHAR);
        act("$n concentrates and regenerates $s body a little.", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (amt < GET_MAX_HIT(ch) / 5)
    {
        act("You concentrate your ki and regenerate your body some.", true, ch, nullptr, nullptr, TO_CHAR);
        act("$n concentrates and regenerates $s body some.", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (amt < GET_MAX_HIT(ch) / 2)
    {
        act("You concentrate your ki and regenerate your body a great deal.", true, ch, nullptr, nullptr, TO_CHAR);
        act("$n concentrates and regenerates $s body a great deal.", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (GET_HIT(ch) < GET_MAX_HIT(ch))
    {
        act("You concentrate your ki and regenerate you nearly completely.", true, ch, nullptr, nullptr, TO_CHAR);
        act("$n concentrates and regenerates $s body nearly completely.", true, ch, nullptr, nullptr, TO_ROOM);
    }
    improve_skill(ch, SKILL_REGENERATE, 0);
    if (AFF_FLAGGED(ch, AFF_BURNED))
    {
        ch->sendText("Your burns are healed now.\r\n");
        act("$n@w's burns are now healed.@n", true, ch, nullptr, nullptr, TO_ROOM);
        null_affect(ch, AFF_BURNED);
    }

    if (!IS_NPC(ch))
    {
        if (GET_LIMBCOND(ch, 0) <= 0)
        {
            act("You regrow your right arm!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n regrows $s right arm!", true, ch, nullptr, nullptr, TO_ROOM);
            GET_LIMBCOND(ch, 0) = 100;
        }
        else if (GET_LIMBCOND(ch, 0) >= 0 && GET_LIMBCOND(ch, 0) < 50)
        {
            act("Your broken right arm mends itself!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n regenerates $s broken right arm!", true, ch, nullptr, nullptr, TO_ROOM);
            GET_LIMBCOND(ch, 0) = 100;
        }
        if (GET_LIMBCOND(ch, 1) <= 0)
        {
            GET_LIMBCOND(ch, 1) = 100;
            act("You regrow your left arm!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n regrows $s left arm!", true, ch, nullptr, nullptr, TO_ROOM);
        }
        else if (GET_LIMBCOND(ch, 1) > 0 && GET_LIMBCOND(ch, 1) < 50)
        {
            act("Your broken left arm mends itself!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n regenerates $s broken left arm!", true, ch, nullptr, nullptr, TO_ROOM);
            GET_LIMBCOND(ch, 1) = 100;
        }
        if (GET_LIMBCOND(ch, 3) <= 0)
        {
            GET_LIMBCOND(ch, 3) = 100;
            act("You regrow your left leg!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n regrows $s left leg!", true, ch, nullptr, nullptr, TO_ROOM);
        }
        else if (GET_LIMBCOND(ch, 3) > 0 && GET_LIMBCOND(ch, 3) < 50)
        {
            act("Your broken left leg mends itself!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n regenerates $s broken left leg!", true, ch, nullptr, nullptr, TO_ROOM);
            GET_LIMBCOND(ch, 3) = 100;
        }
        if (GET_LIMBCOND(ch, 2) <= 0)
        {
            GET_LIMBCOND(ch, 2) = 100;
            act("You regrow your right leg!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n regrows $s right leg!", true, ch, nullptr, nullptr, TO_ROOM);
        }
        else if (GET_LIMBCOND(ch, 2) > 0 && GET_LIMBCOND(ch, 2) < 50)
        {
            GET_LIMBCOND(ch, 2) = 100;
            act("Your broken right leg mends itself!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n regenerates $s broken right leg!", true, ch, nullptr, nullptr, TO_ROOM);
        }
        if (!ch->character_flags.get(CharacterFlag::tail) && IS_BIO(ch))
        {
            ch->character_flags.set(CharacterFlag::tail, true);
            act("You regrow your tail!", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n regrows $s tail!", true, ch, nullptr, nullptr, TO_ROOM);
        }
        improve_skill(ch, SKILL_REGENERATE, 0);
    }
}

ACMD(do_focus)
{
    Character *vict = nullptr;
    char arg[MAX_INPUT_LENGTH];
    char name[MAX_INPUT_LENGTH];

    *name = '\0';
    *arg = '\0';

    two_arguments(argument, arg, name);

    if (!*arg)
    {
        ch->sendText("Yes but what do you want to focus?\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_CURSE))
    {
        ch->sendText("You are cursed and can't focus!\r\n");
        return;
    }
    else if (!(strcmp(arg, "tough")))
    {
        if (!know_skill(ch, SKILL_TSKIN))
        {
            return;
        }
        if (!*name)
        {
            if (AFF_FLAGGED(ch, AFF_STONESKIN))
            {
                ch->sendText("You already have tough skin!\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to infuse into your skin.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_TSKIN) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki into your skin, but fail in making it tough!", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki into $s skin, but fails in making it tough!", true, ch, nullptr, nullptr, TO_ROOM);
                return;
            }
            else
            {
                int duration = GET_INT(ch) / 20;
                assign_affect(ch, AFF_STONESKIN, SKILL_TSKIN, duration, 0, 0, 0, 0, 0, 0);
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki into your skin, making it tough!", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki into $s skin, making it tough!", true, ch, nullptr, nullptr, TO_ROOM);
                return;
            }
        } // End of no vict tough skin

        else
        {
            if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
            {
                ch->sendText("Focus your ki into who's skin?\r\n");
                return;
            }
            if (!can_kill(ch, vict, nullptr, 2))
            {
                return;
            }
            else
            {
                if (ch == vict)
                {
                    ch->send_to("Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
                    return;
                }
                if (AFF_FLAGGED(vict, AFF_STONESKIN))
                {
                    ch->sendText("They already have tough skin!\r\n");
                    return;
                }
                else if (IS_NPC(vict))
                {
                    ch->sendText("Whatever would you waste your ki on them for?\r\n");
                    return;
                }
                else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
                {
                    ch->sendText("You do not have enough ki to infuse into their skin.\r\n");
                    return;
                }
                else if (GET_SKILL(ch, SKILL_TSKIN) < axion_dice(0))
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's skin, but fail in making it tough!", true, ch, nullptr, vict, TO_CHAR);
                    act("$n focuses ki into your skin, but fails in making it tough!", true, ch, nullptr, vict,
                        TO_VICT);
                    act("$n focuses ki into $N's skin, but fails in making it tough!", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    return;
                }
                else
                {
                    int duration = roll_aff_duration(GET_INT(ch), 2);
                    assign_affect(vict, AFF_STONESKIN, SKILL_TSKIN, duration, 0, 0, 0, 0, 0, 0);
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's skin, making it tough!", true, ch, nullptr, vict, TO_CHAR);
                    act("$n focuses ki into your skin, making it tough!", true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki into $N's skin, making it tough!", true, ch, nullptr, vict, TO_NOTVICT);
                    return;
                }
            }

        } // End of victim of tough skin
    } // End of tough skin

    else if (!(strcmp(arg, "might")))
    {
        if (!know_skill(ch, SKILL_MIGHT))
        {
            return;
        }
        if (!*name)
        {
            if (AFF_FLAGGED(ch, AFF_MIGHT))
            {
                ch->sendText("You already have mighty muscles!\r\n");
                return;
            }
            else if (GET_BONUS(ch, BONUS_WIMP) > 0 && GET_STR(ch) + 10 > 25)
            {
                ch->sendText("Your body is not able to withstand increasing its strength beyond 25.\r\n");
                return;
            }
            else if (GET_BONUS(ch, BONUS_FRAIL) > 0 && GET_STR(ch) + 2 > 25)
            {
                ch->sendText("Your body is not able to withstand increasing its strength beyond 25.\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to infuse into your muscles.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_MIGHT) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki into your muscles, but fail in making them mighty!", true, ch, nullptr, nullptr,
                    TO_CHAR);
                act("$n focuses ki into $s muscles, but fails in making them mighty!", true, ch, nullptr, nullptr,
                    TO_ROOM);
                return;
            }
            else
            {
                ch->affect_flags.set(AFF_MIGHT, true);
                ch->modCurVitalDam(CharVital::ki, 0.05);
                int duration = roll_aff_duration(GET_INT(ch), 2);
                /* Str , Con, Int, Agl, Wis, Spd */
                assign_affect(ch, AFF_MIGHT, SKILL_MIGHT, duration, ch->getBaseStat("train_strength") / 5, 2, 0, 0, 0, 0);
                reveal_hiding(ch, 0);
                act("You focus ki into your muscles, making them mighty!", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki into $s muscles, making them mighty!", true, ch, nullptr, nullptr, TO_ROOM);
                return;
            }
        } // End of no vict might

        else
        {
            if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
            {
                ch->sendText("Focus your ki into who's muscles?\r\n");
                return;
            }
            if (!can_kill(ch, vict, nullptr, 2))
            {
                return;
            }
            else
            {
                if (ch == vict)
                {
                    ch->send_to("Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
                    return;
                }
                if (AFF_FLAGGED(vict, AFF_MIGHT))
                {
                    ch->sendText("They already have mighty muscles!\r\n");
                    return;
                }
                else if (GET_BONUS(vict, BONUS_WIMP) > 0 && GET_STR(vict) + 10 > 25)
                {
                    ch->sendText("Their body is not able to withstand increasing its strength beyond 25.\r\n");
                    return;
                }
                else if (GET_BONUS(vict, BONUS_FRAIL) > 0 && GET_CON(vict) + 2 > 25)
                {
                    ch->sendText("Their body is not able to withstand increasing its constitution beyond 25.\r\n");
                    return;
                }
                else if (IS_NPC(vict))
                {
                    ch->sendText("Whatever would you waste your ki on them for?\r\n");
                    return;
                }
                else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
                {
                    ch->sendText("You do not have enough ki to infuse into their muscles.\r\n");
                    return;
                }
                else if (GET_SKILL(ch, SKILL_MIGHT) < axion_dice(0))
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's muscles, but fail in making them mighty!", true, ch, nullptr, vict,
                        TO_CHAR);
                    act("$n focuses ki into your muscles, but fails in making them mighty!", true, ch, nullptr, vict,
                        TO_VICT);
                    act("$n focuses ki into $N's muscles, but fails in making them mighty!", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    return;
                }
                else
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    int duration = roll_aff_duration(GET_INT(ch), 2);
                    /* Str , Con, Int, Agl, Wis, Spd */
                    assign_affect(vict, AFF_MIGHT, SKILL_MIGHT, duration, vict->getBaseStat("train_strength") / 5, 2, 0, 0, 0, 0);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's muscles, making them mighty!", true, ch, nullptr, vict, TO_CHAR);
                    act("$n focuses ki into your muscles, making them mighty!", true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki into $N's muscles, making them mighty!", true, ch, nullptr, vict, TO_NOTVICT);
                    return;
                }
            }

        } // End of victim of might
    } // End of might

    else if (!(strcmp(arg, "wither")))
    {
        if (!know_skill(ch, SKILL_WITHER))
        {
            return;
        }
        if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
        {
            ch->sendText("Focus your ki into who's muscles?\r\n");
            return;
        }
        if (ch == vict)
        {
            ch->sendText("You don't want to wither your own body!\r\n");
            return;
        }
        if (!can_kill(ch, vict, nullptr, 2))
        {
            return;
        }
        if (AFF_FLAGGED(vict, AFF_WITHER))
        {
            ch->sendText("They already have been withered!\r\n");
            return;
        }

        if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
        {
            ch->sendText("You do not have enough ki to wither them.\r\n");
            return;
        }

        if (GET_SKILL(ch, SKILL_WITHER) < axion_dice(0))
        {
            ch->modCurVitalDam(CharVital::ki, 0.05);
            reveal_hiding(ch, 0);
            act("You focus ki into $N's body, but fail in withering it!", true, ch, nullptr, vict, TO_CHAR);
            act("$n focuses ki into your body, but fails in withering it!", true, ch, nullptr, vict, TO_VICT);
            act("$n focuses ki into $N's body, but fails in withering it!", true, ch, nullptr, vict, TO_NOTVICT);
            return;
        }
        else
        {
            ch->modCurVitalDam(CharVital::ki, 0.05);
            assign_affect(vict, AFF_WITHER, SKILL_WITHER, -1, -3, 0, 0, 0, 0, -3);
            reveal_hiding(ch, 0);
            act("You focus ki into $N's body, and succeed in withering it!", true, ch, nullptr, vict, TO_CHAR);
            act("$n focuses ki into your body, and succeeds in withering it!", true, ch, nullptr, vict, TO_VICT);
            act("$n focuses ki into $N's body, and succeeds in withering it!", true, ch, nullptr, vict, TO_NOTVICT);
            return;
        }
    } // End of wither

    else if (!(strcmp(arg, "enlighten")))
    {
        if (!know_skill(ch, SKILL_ENLIGHTEN))
        {
            return;
        }
        if (!*name)
        {
            if (AFF_FLAGGED(ch, AFF_ENLIGHTEN))
            {
                ch->sendText("You already have superior wisdom!\r\n");
                return;
            }
            else if (GET_BONUS(ch, BONUS_FOOLISH) > 0 && GET_WIS(ch) + 10 > 25)
            {
                ch->sendText("You're not able to withstand increasing your wisdom beyond 25.\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to use this skill.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_ENLIGHTEN) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki into your mind, but fail in awakening it to cosmic wisdom!", true, ch, nullptr,
                    nullptr, TO_CHAR);
                act("$n focuses ki into $s mind, but fails in awakening it to cosmic wisdom!", true, ch, nullptr,
                    nullptr, TO_ROOM);
                return;
            }
            else
            {
                int duration = roll_aff_duration(GET_INT(ch), 2);
                /* Str , Con, Int, Agl, Wis, Spd */
                assign_affect(ch, AFF_ENLIGHTEN, SKILL_ENLIGHTEN, duration, 0, 0, 0, 0, ch->getBaseStat("train_intelligence") / 5, 0);
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki into your mind, awakening it to cosmic wisdom!", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki into $s mind, awakening it to cosmic wisdom!", true, ch, nullptr, nullptr, TO_ROOM);
                if (IS_JINTO(ch) && level_exp(ch, GET_WIS(ch) + 1) - GET_EXP(ch) > 0 &&
                    GET_PRACTICES(ch) >= 15 && rand_number(1, 4) >= 3)
                {
                    int64_t gain = 0;
                    ch->modPractices(-15);
                    if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 100)
                    {
                        gain = level_exp(ch, GET_WIS(ch) + 1) * 0.15;
                        auto gained = ch->modExperience(gain);
                        ch->send_to("@GYou gain @g%s@G experience due to your excellence with this skill.@n\r\n", add_commas(gained).c_str());
                    }
                    else if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 60)
                    {
                        gain = level_exp(ch, GET_WIS(ch) + 1) * 0.10;
                        auto gained = ch->modExperience(gain);
                        ch->send_to("@GYou gain @g%s@G experience due to your excellence with this skill.@n\r\n", add_commas(gained).c_str());
                    }
                    else if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 40)
                    {
                        gain = level_exp(ch, GET_WIS(ch) + 1) * 0.05;
                        auto gained = ch->modExperience(gain);
                        ch->send_to("@GYou gain @g%s@G experience due to your excellence with this skill.@n\r\n", add_commas(gained).c_str());
                    }
                }
                return;
            }
        } // End of no vict enlighten

        else
        {
            if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
            {
                ch->sendText("Focus your ki into who's mind?\r\n");
                return;
            }
            if (!can_kill(ch, vict, nullptr, 2))
            {
                return;
            }
            else
            {
                if (ch == vict)
                {
                    ch->send_to("Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
                    return;
                }
                if (AFF_FLAGGED(vict, AFF_ENLIGHTEN))
                {
                    ch->sendText("They already have superior wisdom!\r\n");
                    return;
                }
                else if (GET_BONUS(vict, BONUS_FOOLISH) > 0 && GET_WIS(vict) + 10 > 25)
                {
                    ch->sendText("They're not able to withstand increasing their wisdom beyond 25.\r\n");
                    return;
                }
                else if (IS_NPC(vict))
                {
                    ch->sendText("Whatever would you waste your ki on them for?\r\n");
                    return;
                }
                else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
                {
                    ch->sendText("You do not have enough ki to use this skill.\r\n");
                    return;
                }
                else if (GET_SKILL(ch, SKILL_ENLIGHTEN) < axion_dice(0))
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's mind, but fail in awakening it to cosmic wisdom!", true, ch, nullptr,
                        vict, TO_CHAR);
                    act("$n focuses ki into your mind, but fails in awakening it to cosmic wisdom!", true, ch, nullptr,
                        vict, TO_VICT);
                    act("$n focuses ki into $N's mind, but fails in awakening it to cosmic wisdom!", true, ch, nullptr,
                        vict, TO_NOTVICT);
                    return;
                }
                else
                {
                    int duration = roll_aff_duration(GET_INT(ch), 2);
                    /* Str , Con, Int, Agl, Wis, Spd */
                    assign_affect(vict, AFF_ENLIGHTEN, SKILL_ENLIGHTEN, duration, 0, 0, 0, 0, vict->getBaseStat("train_intelligence") / 5, 0);
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's mind, awakening it to cosmic wisdom!", true, ch, nullptr, vict,
                        TO_CHAR);
                    act("$n focuses ki into your mind, awakening it to cosmic wisdom!", true, ch, nullptr, vict,
                        TO_VICT);
                    act("$n focuses ki into $N's mind, awakening it to cosmic wisdom!", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    if (IS_JINTO(ch) && level_exp(vict, GET_INT(vict) + 1) - GET_EXP(vict) > 0 &&
                        GET_PRACTICES(ch) >= 15 && rand_number(1, 4) >= 3)
                    {
                        int64_t gain = 0;
                        ch->modPractices(-15);
                        if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 100)
                        {
                            gain = level_exp(vict, GET_INT(vict) + 1) * 0.15;
                            auto gained = vict->modExperience(gain);
                            vict->send_to("@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gained).c_str());
                        }
                        else if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 60)
                        {
                            gain = level_exp(vict, GET_INT(vict) + 1) * 0.10;
                            auto gained = vict->modExperience(gain);
                            vict->send_to("@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gained).c_str());
                        }
                        else if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 40)
                        {
                            gain = level_exp(vict, GET_INT(vict) + 1) * 0.05;
                            auto gained = vict->modExperience(gain);
                            vict->send_to("@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gained).c_str());
                        }
                    }
                    return;
                }
            }

        } // End of victim of enlighten
    } // End of enlighten

    else if (!(strcmp(arg, "genius")))
    {
        if (!know_skill(ch, SKILL_GENIUS))
        {
            return;
        }
        if (!*name)
        {
            if (AFF_FLAGGED(ch, AFF_GENIUS))
            {
                ch->sendText("You already have superior intelligence!\r\n");
                return;
            }
            else if (GET_BONUS(ch, BONUS_DULL) > 0 && GET_INT(ch) + 10 > 25)
            {
                ch->sendText("You're not able to withstand increasing your intelligence beyond 25.\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to infuse into your mind.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_GENIUS) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki into your mind, but fail in making it work faster!", true, ch, nullptr, nullptr,
                    TO_CHAR);
                act("$n focuses ki into $s muscles, but fails in making it work faster!", true, ch, nullptr, nullptr,
                    TO_ROOM);
                return;
            }
            else
            {
                int duration = roll_aff_duration(GET_INT(ch), 2);
                /* Str , Con, Int, Agl, Wis, Spd */
                assign_affect(ch, AFF_GENIUS, SKILL_GENIUS, duration, 0, 0, ch->getBaseStat("train_intelligence") / 5, 0, 0, 0);
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki into your mind, making it work faster!", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki into $s mind, making it work faster!", true, ch, nullptr, nullptr, TO_ROOM);
                return;
            }
        } // End of no vict genius

        else
        {
            if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
            {
                ch->sendText("Focus your ki into who's mind?\r\n");
                return;
            }
            if (!can_kill(ch, vict, nullptr, 2))
            {
                return;
            }
            else
            {
                if (ch == vict)
                {
                    ch->send_to("Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
                    return;
                }
                if (AFF_FLAGGED(vict, AFF_GENIUS))
                {
                    ch->sendText("They already have superior intelligence!\r\n");
                    return;
                }
                else if (GET_BONUS(vict, BONUS_DULL) > 0 && GET_INT(vict) + 10 > 25)
                {
                    ch->sendText("They're not able to withstand increasing their intelligence beyond 25.\r\n");
                    return;
                }
                else if (IS_NPC(vict))
                {
                    ch->sendText("Whatever would you waste your ki on them for?\r\n");
                    return;
                }
                else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
                {
                    ch->sendText("You do not have enough ki to infuse into their mind.\r\n");
                    return;
                }
                else if (GET_SKILL(ch, SKILL_GENIUS) < axion_dice(0))
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's mind, but fail in making it work faster!", true, ch, nullptr, vict,
                        TO_CHAR);
                    act("$n focuses ki into your mind, but fails in making it work faster!", true, ch, nullptr, vict,
                        TO_VICT);
                    act("$n focuses ki into $N's mind, but fails in making it work faster!", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    return;
                }
                else
                {
                    int duration = roll_aff_duration(GET_INT(ch), 2);
                    ;
                    /* Str , Con, Int, Agl, Wis, Spd */
                    assign_affect(vict, AFF_GENIUS, SKILL_GENIUS, duration, 0, 0, vict->getBaseStat("train_intelligence") / 5, 0, 0, 0);
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's mind, making it work faster!", true, ch, nullptr, vict, TO_CHAR);
                    act("$n focuses ki into your mind, making it work faster!", true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki into $N's mind, making it work faster!", true, ch, nullptr, vict, TO_NOTVICT);
                    if ((vict->master == ch || ch->master == vict || ch->master == vict->master) &&
                        AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(vict, AFF_GROUP))
                    {
                        if (IS_KAI(ch) && level_exp(ch, GET_INT(ch) + 1) - GET_EXP(ch) > 0 &&
                            rand_number(1, 3) == 3)
                        {
                            ch->modExperience(level_exp(ch, GET_INT(ch) + 1) * 0.05);
                        }
                    }
                    return;
                }
            }

        } // End of victim of genius
    } // End of genius

    else if (!(strcmp(arg, "flex")))
    {
        if (!know_skill(ch, SKILL_FLEX))
        {
            return;
        }
        if (!*name)
        {
            if (AFF_FLAGGED(ch, AFF_FLEX))
            {
                ch->sendText("You already have superior agility!\r\n");
                return;
            }
            else if (GET_BONUS(ch, BONUS_CLUMSY) > 0 && GET_DEX(ch) + 10 > 25)
            {
                ch->sendText("You're not able to withstand increasing your agility beyond 25.\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to infuse into your limbs.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_FLEX) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki into your limbs, but fail in making them more flexible!", true, ch, nullptr, nullptr,
                    TO_CHAR);
                act("$n focuses ki into $s muscles, but fails in making them more flexible!", true, ch, nullptr,
                    nullptr, TO_ROOM);
                return;
            }
            else
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                int duration = roll_aff_duration(GET_INT(ch), 2);
                ;
                /* Str , Con, Int, Agl, Wis, Spd */
                assign_affect(ch, AFF_FLEX, SKILL_FLEX, duration, 0, 0, 0, ch->getBaseStat("train_agility") / 5, 0, 0);
                reveal_hiding(ch, 0);
                act("You focus ki into your limbs, making them more flexible!", true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki into $s limbs, making them more flexible!", true, ch, nullptr, nullptr, TO_ROOM);
                return;
            }
        } // End of no vict FLEX

        else
        {
            if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
            {
                ch->sendText("Focus your ki into who's limbs?\r\n");
                return;
            }
            if (!can_kill(ch, vict, nullptr, 2))
            {
                return;
            }
            else
            {
                if (ch == vict)
                {
                    ch->send_to("Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
                    return;
                }
                if (AFF_FLAGGED(vict, AFF_FLEX))
                {
                    ch->sendText("They already have superior agility!\r\n");
                    return;
                }
                else if (GET_BONUS(vict, BONUS_CLUMSY) > 0 && GET_DEX(vict) + 3 > 25)
                {
                    ch->sendText("They're not able to withstand increasing their agility beyond 25.\r\n");
                    return;
                }
                else if (IS_NPC(vict))
                {
                    ch->sendText("Whatever would you waste your ki on them for?\r\n");
                    return;
                }
                else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
                {
                    ch->sendText("You do not have enough ki to infuse into their limbs.\r\n");
                    return;
                }
                else if (GET_SKILL(ch, SKILL_FLEX) < axion_dice(0))
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's limbs, but fail in making them more flexible!", true, ch, nullptr, vict,
                        TO_CHAR);
                    act("$n focuses ki into your limbs, but fails in making them more flexible!", true, ch, nullptr,
                        vict, TO_VICT);
                    act("$n focuses ki into $N's limbs, but fails in making them more flexible!", true, ch, nullptr,
                        vict, TO_NOTVICT);
                    return;
                }
                else
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    int duration = roll_aff_duration(GET_INT(ch), 2);
                    ;
                    /* Str , Con, Int, Agl, Wis, Spd */
                    assign_affect(vict, AFF_FLEX, SKILL_FLEX, duration, 0, 0, 0, vict->getBaseStat("train_agility") / 5, 0, 0);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's limbs, making them more flexible!", true, ch, nullptr, vict, TO_CHAR);
                    act("$n focuses ki into your limbs, making them more flexible!", true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki into $N's limbs, making them more flexible!", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    if ((vict->master == ch || ch->master == vict || ch->master == vict->master) &&
                        AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(vict, AFF_GROUP))
                    {
                        if (IS_KAI(ch) && level_exp(ch, GET_INT(ch) + 1) - GET_EXP(ch) > 0 &&
                            rand_number(1, 3) == 3)
                        {
                            ch->modExperience(level_exp(ch, GET_INT(ch) + 1) * 0.05);
                        }
                    }
                    return;
                }
            }

        } // End of victim of FLEX
    } // End of FLEX

    else if (!(strcmp(arg, "bless")))
    {
        if (!know_skill(ch, SKILL_BLESS))
        {
            return;
        }
        if (!*name)
        {
            if (AFF_FLAGGED(ch, AFF_BLESS))
            {
                ch->sendText("You already are blessed!\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to bless.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_BLESS) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki while chanting spiritual words. Your blessing does nothing though, you must have messed up!",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki while chanting spiritual words. $n seems disappointed.", true, ch, nullptr, nullptr,
                    TO_ROOM);
                return;
            }
            else
            {
                int duration = roll_aff_duration(GET_INT(ch), 3);
                ;
                /* Str , Con, Int, Agl, Wis, Spd */
                assign_affect(ch, AFF_BLESS, SKILL_BLESS, duration, 0, 0, 0, 0, 0, 0);
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                if (IS_KABITO(ch))
                {
                    ch->setBaseStat("bless_level", GET_SKILL(ch, SKILL_BLESS));
                }
                else
                {
                    ch->setBaseStat("bless_level", 0);
                }
                act("You focus ki while chanting spiritual words. You feel your body recovering at above normal speed!",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki while chanting spiritual words. $n smiles after finishing $s chant.", true, ch,
                    nullptr, nullptr, TO_ROOM);
                if (AFF_FLAGGED(ch, AFF_CURSE))
                {
                    ch->sendText("Your cursing was nullified!\r\n");
                    null_affect(ch, AFF_CURSE);
                }
                return;
            }
        } // End of no vict BLESS

        else
        {
            if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
            {
                ch->sendText("Bless who?\r\n");
                return;
            }
            if (!can_kill(ch, vict, nullptr, 2))
            {
                return;
            }
            else
            {
                if (ch == vict)
                {
                    ch->send_to("Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
                    return;
                }
                if (AFF_FLAGGED(vict, AFF_BLESS))
                {
                    ch->sendText("They already have been blessed!\r\n");
                    return;
                }
                else if (IS_NPC(vict))
                {
                    ch->sendText("Whatever would you waste your ki on them for?\r\n");
                    return;
                }
                else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
                {
                    ch->sendText("You do not have enough ki to bless.\r\n");
                    return;
                }
                else if (GET_SKILL(ch, SKILL_BLESS) < axion_dice(0))
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki while chanting spiritual words. Your blessing fails!", true, ch, nullptr, nullptr,
                        TO_CHAR);
                    act("$n focuses ki while chanting spiritual words. $n places a hand on your head, but nothing happens!",
                        true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki while chanting spiritual words. $n places a hand on $N's head, but nothing happens!",
                        true, ch, nullptr, vict, TO_NOTVICT);
                    return;
                }
                else
                {
                    int duration = roll_aff_duration(GET_INT(ch), 3);
                    ;
                    /* Str , Con, Int, Agl, Wis, Spd */
                    assign_affect(vict, AFF_BLESS, SKILL_BLESS, duration, 0, 0, 0, 0, 0, 0);
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    if (IS_KAI(ch))
                    {
                        vict->setBaseStat("bless_level", GET_SKILL(ch, SKILL_BLESS));
                    }
                    else
                    {
                        vict->setBaseStat("bless_level", 0);
                    }
                    act("You focus ki while chanting spiritual words. Blessing $N with faster regeneration!", true, ch,
                        nullptr, vict, TO_CHAR);
                    act("$n focuses ki while chanting spiritual words. $n then places a hand on your head, blessing you!",
                        true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki while chanting spiritual words. $n then places a hand on $N's head, blessing them!",
                        true, ch, nullptr, vict, TO_NOTVICT);
                    if ((!IS_NPC(vict)) && AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(vict, AFF_GROUP))
                    {
                        if (IS_KAI(ch))
                        {
                            giveRandomVital(ch, ch->getEffectiveStat<int64_t>("health") / 100, ch->getEffectiveStat<int64_t>("ki") / 100, ch->getEffectiveStat<int64_t>("stamina") / 100, 30);
                        }
                    }
                    if (AFF_FLAGGED(vict, AFF_CURSE))
                    {
                        vict->sendText("Your cursing was nullified!\r\n");
                        null_affect(vict, AFF_CURSE);
                    }
                    return;
                }
            }

        } // End of victim of BLESS
    } // End of BLESS

    else if (!(strcmp(arg, "curse")))
    {
        if (!know_skill(ch, SKILL_CURSE))
        {
            return;
        }
        if (!*name)
        {
            if (AFF_FLAGGED(ch, AFF_CURSE))
            {
                ch->sendText("You already are cursed!\r\n");
                return;
            }
            else if (IS_DEMON(ch))
            {
                ch->sendText("You are immune to curses!\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to CURSE.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_CURSE) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki while chanting demonic words. Your cursing does nothing though, you must have messed up!",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki while chanting demonic words. $n seems disappointed.", true, ch, nullptr, nullptr,
                    TO_ROOM);
                return;
            }
            else
            {
                int duration = roll_aff_duration(GET_INT(ch), 3);
                ;
                /* Str , Con, Int, Agl, Wis, Spd */
                assign_affect(vict, AFF_CURSE, SKILL_CURSE, duration, 0, 0, 0, 0, 0, 0);
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki while chanting demonic words. You feel your body recovering at below normal speed!",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki while chanting demonic words. $n grins after finishing $s chant.", true, ch, nullptr,
                    nullptr, TO_ROOM);
                if (AFF_FLAGGED(ch, AFF_BLESS))
                {
                    ch->sendText("Your blessing was nullified!\r\n");
                    null_affect(ch, AFF_BLESS);
                }
                return;
            }
        } // End of no vict CURSE

        else
        {
            if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
            {
                ch->sendText("Curse who?\r\n");
                return;
            }
            if (!can_kill(ch, vict, nullptr, 0))
            {
                return;
            }
            else
            {
                if (ch == vict)
                {
                    ch->send_to("Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
                    return;
                }
                if (AFF_FLAGGED(vict, AFF_CURSE))
                {
                    ch->sendText("They already have been cursed!\r\n");
                    return;
                }
                else if (IS_NPC(vict))
                {
                    ch->sendText("Whatever would you waste your ki on them for?\r\n");
                    return;
                }
                else if (IS_DEMON(vict))
                {
                    ch->sendText("They are immune to curses!\r\n");
                    return;
                }
                else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
                {
                    ch->sendText("You do not have enough ki to CURSE.\r\n");
                    return;
                }
                else if (GET_SKILL(ch, SKILL_CURSE) < axion_dice(0))
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki while chanting demonic words. Your cursing fails!", true, ch, nullptr, nullptr,
                        TO_CHAR);
                    act("$n focuses ki while chanting demonic words. $n places a hand on your head, but nothing happens!",
                        true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki while chanting demonic words. $n places a hand on $N's head, but nothing happens!",
                        true, ch, nullptr, vict, TO_NOTVICT);
                    return;
                }
                else
                {
                    int duration = roll_aff_duration(GET_INT(ch), 3);
                    ;
                    /* Str , Con, Int, Agl, Wis, Spd */
                    assign_affect(vict, AFF_CURSE, SKILL_CURSE, duration, 0, 0, 0, 0, 0, 0);
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki while chanting demonic words. cursing $N with slower regeneration!", true, ch,
                        nullptr, vict, TO_CHAR);
                    act("$n focuses ki while chanting demonic words. $n then places a hand on your head, cursing you!",
                        true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki while chanting demonic words. $n then places a hand on $N's head, cursing them!",
                        true, ch, nullptr, vict, TO_NOTVICT);
                    if (AFF_FLAGGED(vict, AFF_BLESS))
                    {
                        vict->sendText("Your blessing was nullified!\r\n");
                        null_affect(vict, AFF_BLESS);
                    }
                    return;
                }
            }

        } // End of victim of CURSE
    } // End of CURSE

    else if (!(strcmp(arg, "yoikominminken")) || !(strcmp(arg, "yoik")))
    {
        if (!know_skill(ch, SKILL_YOIK))
        {
            return;
        }
        if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
        {
            ch->sendText("Use Yoikominminken on who?\r\n");
            return;
        }
        if (!can_kill(ch, vict, nullptr, 0))
        {
            return;
        }
        else
        {
            if (AFF_FLAGGED(vict, AFF_SLEEP))
            {
                ch->sendText("They already have been put to sleep!\r\n");
                return;
            }
            else if (PLR_FLAGGED(vict, PLR_EYEC))
            {
                ch->sendText("Their eyes are closed!\r\n");
                return;
            }
            else if (AFF_FLAGGED(vict, AFF_BLIND))
            {
                ch->sendText("They appear to be blind!\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to use Yoikominminken.\r\n");
                return;
            }
            else if (GET_BONUS(vict, BONUS_INSOMNIAC))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki while moving your hands in lulling patterns, but $N doesn't look the least bit sleepy!",
                    true, ch, nullptr, vict, TO_CHAR);
                act("$n focuses ki while moving $s hands in a lulling pattern, but you just don't feel tired.", true,
                    ch, nullptr, vict, TO_VICT);
                act("$n focuses ki while moving $s hands in a lulling pattern, but $N doesn't look the least bit sleepy!",
                    true, ch, nullptr, vict, TO_NOTVICT);
                return;
            }
            else if (GET_SKILL(ch, SKILL_YOIK) < axion_dice(0) ||
                     (GET_INT(ch) + rand_number(1, 3) < GET_INT(vict) + rand_number(1, 5)))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki while moving your hands in lulling patterns, but fail to put $N to sleep!", true, ch,
                    nullptr, vict, TO_CHAR);
                act("$n focuses ki while moving $s hands in a lulling pattern, but you resist the technique!", true, ch,
                    nullptr, vict, TO_VICT);
                act("$n focuses ki while moving $s hands in a lulling pattern, but $N resists the technique!", true, ch,
                    nullptr, vict, TO_NOTVICT);
                return;
            }
            else
            {
                int duration = rand_number(1, 2);
                /* Str , Con, Int, Agl, Wis, Spd */
                assign_affect(vict, AFF_SLEEP, SKILL_YOIK, duration, 0, 0, 0, 0, 0, 0);
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki while moving your hands in lulling patterns, putting $N to sleep!", true, ch, nullptr,
                    vict, TO_CHAR);
                act("$n focuses ki while moving $s hands in a lulling pattern, before you realise it you are asleep!",
                    true, ch, nullptr, vict, TO_VICT);
                act("$n focuses ki while moving $s hands in a lulling pattern, putting $N to sleep!", true, ch, nullptr,
                    vict, TO_NOTVICT);
                vict->setBaseStat("combo", POS_SLEEPING);
                vict->affect_flags.set(AFF_FLYING, false);
                vict->setBaseStat<int>("altitude", 0);
                return;
            }
        }
    } // End of Yoik

    else if (!(strcmp(arg, "vigor")))
    {
        if (!know_skill(ch, SKILL_VIGOR))
        {
            return;
        }
        if (!*name)
        {
            if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 10)
            {
                ch->sendText("You do not have enough ki to use vigor.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_VIGOR) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.1);
                reveal_hiding(ch, 0);
                act("You focus ki into your very cells, but fail at re-engerizing them!", true, ch, nullptr, nullptr,
                    TO_CHAR);
                act("$n focuses ki and glows green for a moment, $e then frowns.", true, ch, nullptr, nullptr, TO_ROOM);
                WAIT_STATE(ch, PULSE_2SEC);
                return;
            }
            else if ((ch->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(ch))
            {
                ch->sendText("You already have full stamina.\r\n");
                return;
            }
            else
            {
                if (GET_BONUS(ch, BONUS_HEALER) > 0)
                {
                    ch->modCurVital(CharVital::stamina, ch->getEffectiveStat<int64_t>("ki") / 8);
                    ch->modCurVitalDam(CharVital::ki, 0.125);
                }
                else
                {
                    ch->modCurVital(CharVital::stamina, ch->getEffectiveStat<int64_t>("ki") / 10);
                    ch->modCurVitalDam(CharVital::ki, 0.1);
                }

                reveal_hiding(ch, 0);
                act("You focus ki into your very cells, and manage to re-energize them!", true, ch, nullptr, nullptr,
                    TO_CHAR);
                act("$n focuses ki and glows green for a moment, $e then smiles.", true, ch, nullptr, nullptr, TO_ROOM);
                WAIT_STATE(ch, PULSE_2SEC);
                return;
            }
        } // End of no vict VIGOR

        else
        {
            if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
            {
                ch->sendText("VIGOR who?\r\n");
                return;
            }
            if (!can_kill(ch, vict, nullptr, 2))
            {
                return;
            }
            else
            {
                if (IS_NPC(vict))
                {
                    ch->sendText("Whatever would you waste your ki on them for?\r\n");
                    return;
                }
                else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 10)
                {
                    ch->sendText("You do not have enough ki to use vigor.\r\n");
                    return;
                }
                else if ((vict->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(vict))
                {
                    ch->sendText("They already have full stamina.\r\n");
                    return;
                }
                else if (GET_SKILL(ch, SKILL_VIGOR) < axion_dice(0))
                {
                    ch->modCurVitalDam(CharVital::ki, 0.1);
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's very cells, and fail at re-energizing them!", true, ch, nullptr, vict,
                        TO_CHAR);
                    act("$n focuses ki into your very cells, but nothing happens!", true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki and $N glows green for a moment, $N frowns.", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    WAIT_STATE(ch, PULSE_2SEC);
                    return;
                }
                else
                {
                    if (GET_BONUS(ch, BONUS_HEALER) > 0)
                    {
                        vict->modCurVital(CharVital::stamina, vict->getEffectiveStat<int64_t>("ki") / 8);
                        ch->modCurVitalDam(CharVital::ki, 0.125);
                    }
                    else
                    {
                        vict->modCurVital(CharVital::stamina, vict->getEffectiveStat<int64_t>("ki") / 10);
                        ch->modCurVitalDam(CharVital::ki, 0.1);
                    }
                    reveal_hiding(ch, 0);
                    act("You focus ki into $N's very cells, and manage to re-energize them!", true, ch, nullptr, vict,
                        TO_CHAR);
                    act("$n focuses ki into your very cells, and manages to re-energize them!", true, ch, nullptr, vict,
                        TO_VICT);
                    act("$n focuses ki and $N glows green for a moment, $N smiles.", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    WAIT_STATE(ch, PULSE_2SEC);
                    return;
                }
            }

        } // End of victim of VIGOR
    } // End of VIGOR

    else if (!(strcmp(arg, "cure")))
    {
        if (!know_skill(ch, SKILL_CURE))
        {
            return;
        }
        if (!*name)
        {
            if (!AFF_FLAGGED(ch, AFF_POISON))
            {
                ch->sendText("You are not poisoned!\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to cure.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_CURE) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki and aim a pulsing light at your body. Nothing happens!", true, ch, nullptr, nullptr,
                    TO_CHAR);
                act("$n focuses ki and aims a pulsing light at $s body. Nothing seems to happen.", true, ch, nullptr,
                    nullptr, TO_ROOM);
                return;
            }
            else
            {
                affect_from_char(ch, SPELL_POISON);
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki and aim a pulsing light at your body. You feel the poison in your blood disappear!",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("$n focuses ki and aims a pulsing light at $s body. $n smiles.", true, ch, nullptr, nullptr,
                    TO_ROOM);
                null_affect(ch, AFF_POISON);
                return;
            }
        } // End of no vict cure

        else
        {
            if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
            {
                ch->sendText("cure who?\r\n");
                return;
            }
            if (!can_kill(ch, vict, nullptr, 2))
            {
                return;
            }
            else
            {
                if (ch == vict)
                {
                    ch->send_to("Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
                    return;
                }
                if (!AFF_FLAGGED(vict, AFF_POISON))
                {
                    ch->sendText("They are not poisoned!\r\n");
                    return;
                }
                else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
                {
                    ch->sendText("You do not have enough ki to cure.\r\n");
                    return;
                }
                else if (GET_SKILL(ch, SKILL_CURE) < axion_dice(0))
                {
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki and aim a pulsing light at $N's body. Nothing happens.", true, ch, nullptr, vict,
                        TO_CHAR);
                    act("$n focuses ki and aims a pulsing light at your body. You are STILL poisoned!", true, ch,
                        nullptr, vict, TO_VICT);
                    act("$n focuses ki and aims a pulsing light at $N's body. $N looks disappointed.", true, ch,
                        nullptr, vict, TO_NOTVICT);
                    return;
                }
                else
                {
                    affect_from_char(vict, SPELL_POISON);
                    ch->modCurVitalDam(CharVital::ki, 0.05);
                    reveal_hiding(ch, 0);
                    act("You focus ki and aim a pulsing light at $N's body. $e is cured.", true, ch, nullptr, vict,
                        TO_CHAR);
                    act("$n focuses ki and aims a pulsing light at your body. You have been cured of your poison!",
                        true, ch, nullptr, vict, TO_VICT);
                    act("$n focuses ki and aims a pulsing light at $N's body. $N smiles.", true, ch, nullptr, vict,
                        TO_NOTVICT);
                    null_affect(vict, AFF_POISON);
                    return;
                }
            }

        } // End of victim of cure
    } // End of cure

    else if (!(strcmp(arg, "poison")))
    {

        if (!know_skill(ch, SKILL_POISON))
        {
            return;
        }
        if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_ROOM)))
        {
            ch->sendText("Poison who?\r\n");
            return;
        }
        if (!can_kill(ch, vict, nullptr, 0))
        {
            return;
        }
        else
        {
            if (ch == vict)
            {
                ch->sendText("Why poison yourself?\r\n");
                return;
            }
            if (IS_NPC(vict))
            {
                if (MOB_FLAGGED(vict, MOB_NOPOISON))
                {
                    ch->sendText("You get the feeling that this being is immune to poison.\r\n");
                    return;
                }
            }
            if (AFF_FLAGGED(vict, AFF_POISON))
            {
                ch->sendText("They already have been poisoned!\r\n");
                return;
            }
            else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(ch) / 20)
            {
                ch->sendText("You do not have enough ki to poison.\r\n");
                return;
            }
            else if (GET_SKILL(ch, SKILL_POISON) < axion_dice(0))
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki and fling poison at $N. You missed!", true, ch, nullptr, vict, TO_CHAR);
                act("$n focuses ki and flings poison at you, but misses!", true, ch, nullptr, vict, TO_VICT);
                act("$n focuses ki and flings poison at $N, but misses!", true, ch, nullptr, vict, TO_NOTVICT);
                return;
            }
            else
            {
                ch->modCurVitalDam(CharVital::ki, 0.05);
                reveal_hiding(ch, 0);
                act("You focus ki and fling poison at $N! The poison burns into $s skin!", true, ch, nullptr, vict,
                    TO_CHAR);
                act("$n focuses ki and flings poison at you! The poison burns into your skin!", true, ch, nullptr, vict,
                    TO_VICT);
                act("$n focuses ki and flings poison at $N! The poison burns into $s skin!", true, ch, nullptr, vict,
                    TO_NOTVICT);
                if (IS_NPC(vict))
                {
                    set_fighting(vict, ch);
                }
                if (vict->mutations.get(Mutation::venomous))
                {
                    act("However $N seems unaffected by the poison.", true, ch, nullptr, vict, TO_CHAR);
                    act("Your natural immunity to poison prevents it from affecting you.", true, ch, nullptr, vict,
                        TO_VICT);
                    act("However $N seems unaffected by the poison.", true, ch, nullptr, vict, TO_NOTVICT);
                }
                else
                {
                    vict->poisonby = ch;
                    ch->poisoned.add(vict->shared_from_this());
                    if (GET_CHARGE(ch) > 0)
                    {
                        ch->sendText("You lose your concentration and release your charged ki!\r\n");
                        do_charge(ch, "release", 0, 0);
                    }
                    int duration = GET_INT(ch) / 20;
                    assign_affect(vict, AFF_POISON, SKILL_POISON, duration, 0, 0, 0, 0, 0, 0);
                    characterSubscriptions.subscribe("poisoned", vict);
                }
                return;
            }
        }
    } // End of POISON

    else
    {
        ch->sendText("What do you want to focus?\r\n");
        return;
    }
}

ACMD(do_plant)
{
    Character *vict;
    Object *obj;
    char vict_name[100], obj_name[100];
    int roll = 0, detect = 0, fail = 0;

    if (ch->location.getRoomFlag(ROOM_PEACEFUL))
    {
        ch->sendText("This room just has such a peaceful, easy feeling...\r\n");
        return;
    }

    two_arguments(argument, obj_name, vict_name);

    if (!(vict = get_char_vis(ch, vict_name, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("Plant what on who?\r\n");
        return;
    }
    else if (vict == ch)
    {
        ch->sendText("Come on now, that's rather stupid!\r\n");
        return;
    }
    if (MOB_FLAGGED(vict, MOB_NOKILL) && GET_ADMLEVEL(ch) == ADMLVL_NONE)
    {
        ch->sendText("That isn't such a good idea...\r\n");
        return;
    }

    roll = roll_skill(ch, SKILL_SLEIGHT_OF_HAND) + rand_number(1, 3);
    fail = rand_number(1, 105);

    if (GET_POS(vict) < POS_SLEEPING)
        detect = 0;
    else
        detect = (roll_skill(vict, SKILL_SPOT) + rand_number(1, 3));

    /* NO NO With Imp's and Shopkeepers, and if player planting is not allowed */
    if ((ADM_FLAGGED(vict, ADM_NOSTEAL) || GET_MOB_SPEC(vict) == shop_keeper) && GET_ADMLEVEL(ch) < 5)
        roll = -10; /* Failure */

    if (!(obj = get_obj_in_list_vis(ch, obj_name, nullptr, ch->getInventory())))
    {
        ch->sendText("You don't have that to plant on them.\r\n");
        return;
    }
    if (roll <= detect && roll <= fail)
    {
        reveal_hiding(ch, 0);
        act("@C$n@w tries to plant $p@w on you!@n", true, ch, obj, vict, TO_VICT);
        act("@C$n@w tries to plant $p@w on @c$N@w!@n", true, ch, obj, vict, TO_NOTVICT);
        act("@wYou try and fail to plant $p@w on @c$N@w, and $E notices!@n", true, ch, obj, vict, TO_CHAR);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
    else if (roll <= fail)
    {
        act("@wYou try and fail to plant $p@w on @c$N@w! However no one seemed to notice.@n", true, ch, obj, vict,
            TO_CHAR);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
    else if (GET_OBJ_WEIGHT(obj) + (vict->getEffectiveStat("weight_carried")) > CAN_CARRY_W(vict))
    {
        reveal_hiding(ch, 0);
        act("@C$n@w tries to plant $p@w on you!@n", true, ch, obj, vict, TO_VICT);
        act("@C$n@w tries to plant $p@w on @c$N@w!@n", true, ch, obj, vict, TO_NOTVICT);
        act("@wYou try and fail to plant $p@w on @c$N@w because $E can't carry the weight. It seems $E noticed the attempt!@n",
            true, ch, obj, vict, TO_CHAR);
        WAIT_STATE(ch, PULSE_2SEC);
    }
    else if (roll <= detect)
    {
        act("@cYou feel like the weight of your inventory has changed.@n", true, ch, obj, vict, TO_VICT);
        act("@c$N@w looks around after feeling $S pockets.@n", true, ch, obj, vict, TO_NOTVICT);
        act("@wYou plant $p@w on @c$N@w! @c$N @wseems to notice the change in weight in their inventory.@n", true, ch,
            obj, vict, TO_CHAR);
        obj->clearLocation();
        vict->addToInventory(obj);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
    else
    {
        act("@wYou plant $p@w on @c$N@w! No one noticed, whew....@n", true, ch, obj, vict, TO_CHAR);
        obj->clearLocation();
        vict->addToInventory(obj);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
}

ACMD(do_forgery)
{

    Object *obj2, *obj3 = nullptr;
    Object *obj, *obj4 = nullptr, *next_obj;
    int found = false;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (IS_NPC(ch))
    {
        return;
    }

    if (!know_skill(ch, SKILL_FORGERY))
    {
        return;
    }

    if (!*arg)
    {
        ch->sendText("Okay, make a forgery of what?\r\n");
        return;
    }

    if (!(obj2 = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
    {
        ch->sendText("You want to make a fake copy of what?\r\n");
        return;
    }

    obj4 = ch->searchInventory(19);

    if (!obj4)
    {
        ch->sendText("You need a forgery kit.\r\n");
        return;
    }

    if (GET_OBJ_VNUM(obj2) == 19)
    {
        ch->sendText("You can't duplicate a forgery kit.\r\n");
        return;
    }

    if (OBJ_FLAGGED(obj2, ITEM_FORGED))
    {
        ch->send_to("%s is forgery, there is no reason to make a fake of a fake!\r\n", obj2->getShortDescription());
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }

    if (OBJ_FLAGGED(obj2, ITEM_BROKEN))
    {
        ch->send_to("%s is broken, there is no reason to make a fake of this mess!\r\n", obj2->getShortDescription());
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }

    if (GET_OBJ_VNUM(obj2) >= 60000 || GET_OBJ_VNUM(obj2) == 0)
    {
        ch->sendText("You can not make a forgery of that! It's far too squishy....");
        return;
    }
    if (GET_OBJ_VNUM(obj2) >= 18800 && GET_OBJ_VNUM(obj2) <= 18999)
    {
        ch->sendText("You can not make a forgery of that!\r\n");
        return;
    }
    if (GET_OBJ_VNUM(obj2) >= 19080 && GET_OBJ_VNUM(obj2) <= 19199)
    {
        ch->sendText("You can not make a forgery of that!\r\n");
        return;
    }
    if (GET_OBJ_VNUM(obj2) >= 4 && GET_OBJ_VNUM(obj2) <= 6)
    {
        ch->sendText("You can not make a forgery of that!\r\n");
        return;
    }

    if (OBJ_FLAGGED(obj2, ITEM_PROTECTED))
    {
        ch->sendText("You don't know where to begin with this work of ART.\r\n");
        return;
    }

    reveal_hiding(ch, 0);
    act("@c$n@w looks at $p, begins to work on forging a fake copy of it.@n", true, ch, obj2, nullptr, TO_ROOM);
    improve_skill(ch, SKILL_FORGERY, 1);
    if (GET_SKILL(ch, SKILL_FORGERY) < axion_dice(0))
    {
        if (rand_number(1, 10) >= 9)
        { /* Uh oh */
            ch->send_to("In the middle of creating a forgery of %s you screw up. The fabrication unit built into the forgery kit melts and bonds with the original. You clumsy mistake with the Estex Titanium drill has broken both.\r\n", obj2->getShortDescription());
            extract_obj(obj4);
            extract_obj(obj2);
            return;
        }
        ch->send_to("You start to make a forgery of %s but screw up and waste your forgery kit..\r\n", obj2->getShortDescription());
        act("@c$n@w tried to duplicate $p but screws up somehow.@n", true, ch, obj2, nullptr, TO_ROOM);
        obj4->clearLocation();
        extract_obj(obj4);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }

    int loadn = GET_OBJ_VNUM(obj2);

    obj3 = read_object(loadn, VIRTUAL);
    ch->addToInventory(obj3);

    /* Set Object Variables */
    obj3->item_flags.set(ITEM_FORGED, true);
    obj3->setBaseStat<weight_t>("weight", rand_number(GET_OBJ_WEIGHT(obj3) / 2, GET_OBJ_WEIGHT(obj3)));

    obj4->clearLocation();
    extract_obj(obj4);
    ch->send_to("You make an excellent forgery of %s@n!\r\n", obj2->getShortDescription());
    act("@c$n@w makes a perfect forgery of $p.@n", true, ch, obj2, nullptr, TO_ROOM);
    WAIT_STATE(ch, PULSE_2SEC);
}

ACMD(do_appraise)
{
    int i, found;
    Object *obj;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    one_argument(argument, arg);

    if (IS_NPC(ch))
    {
        return;
    }

    if (!know_skill(ch, SKILL_APPRAISE))
    {
        return;
    }

    if (!*arg)
    {
        ch->sendText("Okay, appraise what?\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
    {
        ch->sendText("You want to appraise what?\r\n");
        return;
    }

    reveal_hiding(ch, 0);
    act("@c$n@w looks at $p, turning it over in $s hands.@n", true, ch, obj, nullptr, TO_ROOM);
    improve_skill(ch, SKILL_APPRAISE, 1);
    if (GET_SKILL(ch, SKILL_APPRAISE) < axion_dice(-10))
    {
        ch->send_to("You fail to perceive the worth of %s..\r\n", obj->getShortDescription());
        act("@c$n@w looks stumped about $p.@n", true, ch, obj, nullptr, TO_ROOM);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }

    if (OBJ_FLAGGED(obj, ITEM_BROKEN))
    {
        ch->send_to("%s is broken!\r\n", obj->getShortDescription());
        act("@c$n@w looks at $p and frowns.@n", true, ch, obj, nullptr, TO_ROOM);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }

    if (OBJ_FLAGGED(obj, ITEM_FORGED))
    {
        ch->send_to("%s is fake and worthless!\r\n", obj->getShortDescription());
        act("@c$n@w looks at $p with an angry face.@n", true, ch, obj, nullptr, TO_ROOM);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
    found = false;
    int displevel = GET_OBJ_LEVEL(obj);

    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && OBJ_FLAGGED(obj, ITEM_CUSTOM))
        displevel = 20;

    ch->send_to("%s is worth: %s\r\nMin Lvl: %d\r\n", obj->getShortDescription(), add_commas(GET_OBJ_COST(obj)).c_str(), displevel);
    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
    {
        auto wlvl = obj->getBaseStat<int64_t>(VAL_WEAPON_LEVEL);
        int dambon = 0;
        switch (wlvl)
        {
        case 1:
            dambon = 5;
            break;
        case 2:
            dambon = 10;
            break;
        case 3:
            dambon = 20;
            break;
        case 4:
            dambon = 30;
            break;
        case 5:
            dambon = 50;
            break;
        }
        if (dambon)
        {
            ch->send_to("Weapon Level: %d\nDamage Bonus: %d%s\r\n", wlvl, dambon, "%");
        }
    }
    ch->send_to("Size: %s\r\n", size_names[static_cast<int>(GET_OBJ_SIZE(obj))]);
    if (OBJ_FLAGGED(obj, ITEM_SLOT1) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
    {
        ch->sendText("Token Slots  : @m0/1@n\n");
    }
    else if (OBJ_FLAGGED(obj, ITEM_SLOT1) && OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
    {
        ch->sendText("Token Slots  : @m1/1@n\n");
    }
    else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && !OBJ_FLAGGED(obj, ITEM_SLOT_ONE) &&
             !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
    {
        ch->sendText("Token Slots  : @m0/2@n\n");
    }
    else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && OBJ_FLAGGED(obj, ITEM_SLOT_ONE) &&
             !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
    {
        ch->sendText("Token Slots  : @m1/2@n\n");
    }
    else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED))
    {
        ch->sendText("Token Slots  : @m2/2@n\n");
    }
    ch->sendText("Bonuses:");
    act("@c$n@w looks at $p and nods, a satisfied look on $s face.@n", true, ch, obj, nullptr, TO_ROOM);
    int percent = false;
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
        if (obj->affected[i].location != APPLY_NONE)
        {
            percent = obj->affected[i].isPercent();
            sprinttype(obj->affected[i].location, apply_types, buf, sizeof(buf));
            auto m = fmt::format("{}", obj->affected[i].modifier);
            ch->send_to("%s %s%s to %s", found++ ? "," : "", m.c_str(), percent == true ? "%" : "", buf);
            percent = false;
            switch (obj->affected[i].location)
            {
            case APPLY_SKILL:
                ch->send_to(" (%s)", spell_info[obj->affected[i].specific].name);
                break;
            }
        }
    }
    if (!found)
        ch->sendText(" None");
    char buf2[MAX_STRING_LENGTH];
    sprintf(buf2, "%s", GET_OBJ_PERM(obj).getFlagNames().c_str());
    ch->send_to("\nSpecial: %s\r\n", buf2);

    WAIT_STATE(ch, PULSE_2SEC);
}

ACMD(do_disguise)
{
    int skill = 0, roll = 0;

    if (IS_NPC(ch))
    {
        ch->sendText("You forgot your disguise off in mobland.\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_DISGUISED))
    {
        ch->sendText("You stop disguising yourself.\r\n");
        ch->player_flags.set(PLR_DISGUISED, true);
        act("@C$n @wpulls off $s disguise and reveals $mself!", true, ch, nullptr, nullptr, TO_ROOM);
        return;
    }

    if (!know_skill(ch, SKILL_DISGUISE))
    {
        return;
    }

    if (!GET_EQ(ch, WEAR_HEAD))
    {
        ch->sendText("You can't disguise your identity without anything on your head.\r\n");
        return;
    }

    if ((ch->getCurVital(CharVital::stamina)) < (ch->getCurVital(CharVital::stamina)) / 50)
    {
        ch->sendText("You are too tired to try that right now.\r\n");
        return;
    }

    skill = GET_SKILL(ch, SKILL_DISGUISE);
    roll = axion_dice(-10);

    if (skill > roll)
    {
        ch->sendText("You managed to disguise yourself with some skilled manipulation of your headwear.\r\n");
        act("@C$n @wmanages to disguise $mself with some skilled manipulation of $s headwear.", true, ch, nullptr,
            nullptr, TO_ROOM);
        ch->player_flags.set(PLR_DISGUISED, true);
        return;
    }
    else
    {
        ch->sendText("You finish attempting to disguise yourself, but realize you failed and need to try again.\r\n");
        act("@C$n @wattempts and fails to disguise $mself properly and must try again.", true, ch, nullptr, nullptr,
            TO_ROOM);
        ch->modCurVital(CharVital::stamina, -(ch->getEffectiveStat<int64_t>("stamina") / 50));
        return;
    }
}

ACMD(do_eavesdrop)
{
    int dir;
    char buf[100];
    one_argument(argument, buf);

    if (GET_EAVESDROP(ch) > 0)
    {
        ch->sendText("You stop eavesdropping.\r\n");
        ch->setBaseStat("listen_room", 0);
        ch->setBaseStat("listen_direction", -1);
        return;
    }

    if (!*buf)
    {
        ch->sendText("In which direction would you like to eavesdrop?\r\n");
        return;
    }
    if ((dir = search_block(buf, dirs, false)) < 0)
    {
        ch->sendText("Which directions is that?\r\n");
        return;
    }
    if (!know_skill(ch, SKILL_EAVESDROP))
    {
        return;
    }
    if (auto ex = EXIT(ch, dir); ex)
    {
    if (ex->exit_flags[EX_CLOSED] && !ex->keyword.empty())
        {
            sprintf(buf, "The %s is closed.\r\n", fname(ex->keyword.c_str()));
            ch->sendText(buf);
        }
        else
        {
            ch->setBaseStat("listen_room", ex->getVnum());
            ch->setBaseStat("listen_direction", dir);
            ch->sendText("Okay.\r\n");
        }
    }
    else
        ch->sendText("There is not a room there...\r\n");
}

ACMD(do_zanzoken)
{
    if (!know_skill(ch, SKILL_ZANZOKEN) && !IS_NPC(ch))
    {
        return;
    }

    if (AFF_FLAGGED(ch, AFF_ZANZOKEN))
    {
        ch->affect_flags.set(AFF_ZANZOKEN, true);
        ch->sendText("You release the ki you had prepared for a zanzoken.\r\n");
        return;
    }

    if (GRAPPLING(ch) || GRAPPLED(ch))
    {
        ch->sendText("You are busy in a grapple!\r\n");
        return;
    }

    act("@wYou focus your ki, preparing to move at super speeds if necessary.@n", true, ch, nullptr, nullptr, TO_CHAR);
    ch->affect_flags.set(AFF_ZANZOKEN, true);
    improve_skill(ch, SKILL_ZANZOKEN, 2);
    WAIT_STATE(ch, PULSE_2SEC);
}

ACMD(do_block)
{
    Character *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (IS_NPC(ch))
        return;

    if (!*arg)
    {
        if (!BLOCKS(ch))
        {
            ch->sendText("You want to block who?\r\n");
            return;
        }
        if (BLOCKS(ch))
        {
            act("@wYou stop blocking @c$N@w.@n", true, ch, nullptr, BLOCKS(ch), TO_CHAR);
            act("@C$n@w stops blocking you.@n", true, ch, nullptr, BLOCKS(ch), TO_VICT);
            act("@C$n@w stops blocking @c$N@w.@n", true, ch, nullptr, BLOCKS(ch), TO_NOTVICT);
            vict = BLOCKS(ch);
            BLOCKED(vict) = nullptr;
            BLOCKS(ch) = nullptr;
            return;
        }
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("You do not see the target here.\r\n");
        return;
    }

    if (BLOCKS(ch) == vict)
    {
        ch->sendText("They are already blocked by you!\r\n");
        return;
    }

    if (ch == vict)
    {
        ch->sendText("You can't block yourself, are you mental?\r\n");
        return;
    }

    if (BLOCKED(vict))
    {
        ch->sendText("They are already blocked by someone else!\r\n");
        return;
    }

    if (BLOCKS(ch))
    {
        act("@wYou stop blocking @c$N@w.@n", true, ch, nullptr, BLOCKS(ch), TO_CHAR);
        act("@C$n@w stops blocking you.@n", true, ch, nullptr, BLOCKS(ch), TO_VICT);
        act("@C$n@w stops blocking @c$N@w.@n", true, ch, nullptr, BLOCKS(ch), TO_NOTVICT);
        Character *oldv = BLOCKS(ch);
        BLOCKED(oldv) = nullptr;
        BLOCKS(ch) = vict;
        BLOCKED(vict) = ch;
        reveal_hiding(ch, 0);
        act("@wYou start blocking @c$N's@w escape.@n", true, ch, nullptr, BLOCKS(ch), TO_CHAR);
        act("@C$n@w starts blocking your escape.@n", true, ch, nullptr, BLOCKS(ch), TO_VICT);
        act("@C$n@w starts blocking @c$N's@w escape.@n", true, ch, nullptr, BLOCKS(ch), TO_NOTVICT);
        return;
    }
    else
    {
        BLOCKS(ch) = vict;
        BLOCKED(vict) = ch;
        reveal_hiding(ch, 0);
        act("@wYou start blocking @c$N's@w escape.@n", true, ch, nullptr, BLOCKS(ch), TO_CHAR);
        act("@C$n@w starts blocking your escape.@n", true, ch, nullptr, BLOCKS(ch), TO_VICT);
        act("@C$n@w starts blocking @c$N's@w escape.@n", true, ch, nullptr, BLOCKS(ch), TO_NOTVICT);
        return;
    }
}

ACMD(do_eyec)
{

    if (IS_NPC(ch))
        return;

    if (PLR_FLAGGED(ch, PLR_EYEC))
    {
        ch->player_flags.set(PLR_EYEC, false);
        act("@wYou open your eyes.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@w opens $s eyes.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (!PLR_FLAGGED(ch, PLR_EYEC))
    {
        ch->player_flags.set(PLR_EYEC, true);
        act("@wYou close your eyes.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@w closes $s eyes.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }

    WAIT_STATE(ch, PULSE_1SEC);
}

ACMD(do_solar)
{
    Character *vict = nullptr, *next_v = nullptr;

    int prob = 0, perc = 0, cost = 0, bonus = 0;

    if (!know_skill(ch, SKILL_SOLARF))
    {
        return;
    }

    if (!limb_ok(ch, 0))
    {
        return;
    }

    prob = GET_SKILL(ch, SKILL_SOLARF);
    perc = rand_number(0, 101);

    if (prob >= 75)
    {
        cost = GET_MAX_MANA(ch) / 50;
    }
    else if (prob >= 50)
    {
        cost = GET_MAX_MANA(ch) / 25;
    }
    else if (prob >= 25)
    {
        cost = GET_MAX_MANA(ch) / 20;
    }
    else if (prob < 25)
    {
        cost = GET_MAX_MANA(ch) / 15;
    }

    if ((ch->getCurVital(CharVital::ki)) < cost)
    {
        ch->sendText("You do not have enough ki.\r\n");
        return;
    }

    bonus = GET_INT(ch) / 3;
    prob += bonus;

    if (prob < perc)
    {
        act("@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' but nothing happens!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' but nothing happens!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        ch->modCurVital(CharVital::ki, -cost);
        WAIT_STATE(ch, PULSE_3SEC);
        improve_skill(ch, SKILL_SOLARF, 0);
        return;
    }

    act("@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' as a blinding light fills the area!@n",
        true, ch, nullptr, nullptr, TO_CHAR);
    act("@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' as a blinding light fills the area!@n",
        true, ch, nullptr, nullptr, TO_ROOM);

    auto people = ch->location.getPeople();
    for (auto vict : filter_raw(people))
    {
        if (vict == ch)
            continue;
        if (AFF_FLAGGED(vict, AFF_BLIND))
            continue;
        if (GET_POS(vict) == POS_SLEEPING)
            continue;
        if (prob > (GET_DEX(vict) * (axion_dice(0) / 100)) + (perc / 2))
        {
            int duration = 1;
            assign_affect(vict, AFF_BLIND, SKILL_SOLARF, duration, 0, 0, 0, 0, 0, 0);
            act("@W$N@W is @YBLINDED@W!@n", true, ch, nullptr, vict, TO_CHAR);
            act("@RYou are @YBLINDED@R!@n", true, ch, nullptr, vict, TO_VICT);
            act("@W$N@W is @YBLINDED@W!@n", true, ch, nullptr, vict, TO_NOTVICT);
        }
    }
    improve_skill(ch, SKILL_SOLARF, 0);
    ch->modCurVital(CharVital::ki, -cost);
    WAIT_STATE(ch, PULSE_3SEC);
}

ACMD(do_heal)
{
    int64_t cost = 0, prob = 0, perc = 0, heal = 0, bonus = 0;
    Character *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!check_skill(ch, SKILL_HEAL))
    {
        return;
    }

    if (!limb_ok(ch, 0))
    {
        return;
    }

    if (!*arg)
    {
        ch->sendText("You want to heal WHO?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("You do not see the target here.\r\n");
        return;
    }

    prob = init_skill(ch, SKILL_HEAL);
    perc = rand_number(0, 110);

    if (prob >= 100)
    {
        cost = GET_MAX_MANA(ch) / 20;
        heal = GET_MAX_HIT(vict) / 5;
    }
    else if (prob >= 90)
    {
        cost = GET_MAX_MANA(ch) / 16;
        heal = GET_MAX_HIT(vict) / 10;
    }
    else if (prob >= 75)
    {
        cost = GET_MAX_MANA(ch) / 14;
        heal = GET_MAX_HIT(vict) / 12;
    }
    else if (prob >= 50)
    {
        cost = GET_MAX_MANA(ch) / 12;
        heal = GET_MAX_HIT(vict) / 15;
    }
    else if (prob >= 25)
    {
        cost = GET_MAX_MANA(ch) / 10;
        heal = GET_MAX_HIT(vict) / 20;
    }
    else if (prob < 25)
    {
        cost = GET_MAX_MANA(ch) / 6;
        heal = GET_MAX_HIT(vict) / 20;
    }

    if (GET_BONUS(ch, BONUS_HEALER) > 0)
    {
        heal += heal * .1;
    }

    if (heal < (vict->getEffectiveStat<int64_t>("health")))
    {
        heal += (heal / 100) * (GET_WIS(ch) / 4);
    }

    if ((ch->getCurVital(CharVital::ki)) < cost)
    {
        ch->sendText("You do not have enough ki.\r\n");
        return;
    }

    if (GET_HIT(vict) >= (vict->getEffectiveStat<int64_t>("health")))
    {
        if (vict != ch)
        {
            ch->sendText("They are already at full health.\r\n");
        }
        else
        {
            ch->sendText("You are already at full health.\r\n");
        }
        return;
    }

    if (GET_SUPPRESS(vict) > 0 && GET_HIT(vict) >= (((vict->getEffectiveStat<int64_t>("health")) / 100) * GET_SUPPRESS(vict)))
    {
        ch->sendText("They are already at full health.\r\n");
        return;
    }

    bonus = (GET_INT(ch) / 2) + (GET_WIS(ch) / 3);
    prob += bonus;

    if (prob < perc)
    {
        if (vict != ch)
        {
            act("@WYou place your hands near @c$N@W, but fail to concentrate enough to heal them!@n", true, ch, nullptr,
                vict, TO_CHAR);
            act("@C$n@W places $s hands near you, but nothing happens!@n", true, ch, nullptr, vict, TO_VICT);
            act("@C$n@W places $s hands near @c$N@W, but nothing happens.", true, ch, nullptr, vict, TO_NOTVICT);
            ch->modCurVital(CharVital::ki, -cost);
            improve_skill(ch, SKILL_HEAL, 0);
            WAIT_STATE(ch, PULSE_2SEC);
            return;
        }
        if (vict == ch)
        {
            act("@WYou place your hands on your body, but fail to concentrate to heal yourself!@n", true, ch, nullptr,
                vict, TO_CHAR);
            act("@C$n@W places $s hands on $s body, but nothing happens.", true, ch, nullptr, vict, TO_NOTVICT);
            ch->modCurVital(CharVital::ki, -cost);
            improve_skill(ch, SKILL_HEAL, 0);
            WAIT_STATE(ch, PULSE_2SEC);
            return;
        }
    }

    if (vict != ch)
    {
        if (GET_BONUS(ch, BONUS_HEALER) > 0)
        {
            heal += heal * .25;
        }
        act("@WYou place your hands near @c$N@W and an orange glow surrounds $M!@n", true, ch, nullptr, vict, TO_CHAR);
        act("@C$n@W places $s hands near you and an orange glow surrounds you!@n", true, ch, nullptr, vict, TO_VICT);
        act("@C$n@W places $s hands near @c$N@W and an orange glow surrounds $M.", true, ch, nullptr, vict, TO_NOTVICT);
        ch->modCurVital(CharVital::ki, -cost);
        vict->modCurVital(CharVital::health, heal);

        if (IS_NAIL(ch))
        {
            if (GET_SKILL(ch, SKILL_HEAL) >= 100)
            {
                ch->modCurVital(CharVital::stamina, heal * .4);
                vict->sendText("@GYou feel some of your stamina return as well!@n\r\n");
            }
            else if (GET_SKILL(ch, SKILL_HEAL) >= 60)
            {
                ch->modCurVital(CharVital::stamina, heal * .2);
                vict->sendText("@GYou feel some of your stamina return as well!@n\r\n");
            }
            else if (GET_SKILL(ch, SKILL_HEAL) >= 40)
            {
                ch->modCurVital(CharVital::stamina, heal * .1);
                vict->sendText("@GYou feel some of your stamina return as well!@n\r\n");
            }
        }

        null_affect(ch, AFF_POISON);
        null_affect(ch, AFF_BLIND);
        if (AFF_FLAGGED(vict, AFF_BURNED))
        {
            vict->sendText("Your burns are healed now.\r\n");
            act("$n@w's burns are now healed.@n", true, vict, nullptr, nullptr, TO_ROOM);
            vict->affect_flags.set(AFF_BURNED, false);
        }
        if (AFF_FLAGGED(vict, AFF_HYDROZAP))
        {
            vict->sendText("You no longer feel a great thirst.\r\n");
            act("$n@w no longer looks as if they could drink an ocean.@n", true, vict, nullptr, nullptr, TO_ROOM);
            null_affect(vict, AFF_HYDROZAP);
        }
        GET_LIMBCOND(vict, 0) = 100;
        GET_LIMBCOND(vict, 1) = 100;
        GET_LIMBCOND(vict, 2) = 100;
        GET_LIMBCOND(vict, 3) = 100;
        if ((vict->getCurVital(CharVital::lifeforce)) <= (vict->getEffectiveStat("lifeforce")) * 0.5 && !IS_ANDROID(vict))
        {
            vict->modCurVital(CharVital::lifeforce, (ch->getEffectiveStat("lifeforce")) * .35);
            vict->sendText("You feel that your lifeforce has recovered some!\r\n");
        }
        improve_skill(ch, SKILL_HEAL, 0);
        if (!IS_NPC(vict))
        {
            if (IS_NAIL(ch) && IS_NAMEK(ch) && GET_HIT(vict) <= (vict->getEffectiveStat<int64_t>("health")) * 0.85)
            {
                giveRandomVital(ch, ch->getEffectiveStat<int64_t>("health") / 100, ch->getEffectiveStat<int64_t>("ki") / 100, ch->getEffectiveStat<int64_t>("stamina") / 100, 30);
            }
        }

        WAIT_STATE(ch, PULSE_2SEC);
    }

    if (vict == ch)
    {
        if (GET_BONUS(ch, BONUS_HEALER) > 0)
        {
            heal += heal * .25;
        }
        act("@WYou place your hands on your body and an orange glow surrounds you!@n", true, ch, nullptr, vict,
            TO_CHAR);
        act("@C$n@W places $s hands on $s body and an orange glow surrounds $m.", true, ch, nullptr, vict, TO_NOTVICT);
        ch->modCurVital(CharVital::ki, -cost);
        vict->modCurVital(CharVital::health, heal);

        if (IS_NAIL(ch))
        {
            if (GET_SKILL(ch, SKILL_HEAL) >= 100)
            {
                ch->modCurVital(CharVital::stamina, heal * .4);
                vict->sendText("@GYou feel some of your stamina return as well!@n\r\n");
            }
            else if (GET_SKILL(ch, SKILL_HEAL) >= 60)
            {
                ch->modCurVital(CharVital::stamina, heal * .2);
                vict->sendText("@GYou feel some of your stamina return as well!@n\r\n");
            }
            else if (GET_SKILL(ch, SKILL_HEAL) >= 40)
            {
                ch->modCurVital(CharVital::stamina, heal * .1);
                vict->sendText("@GYou feel some of your stamina return as well!@n\r\n");
            }
        }

        vict->affect_flags.set(AFF_BLIND, false);
        GET_LIMBCOND(vict, 0) = 100;
        GET_LIMBCOND(vict, 1) = 100;
        GET_LIMBCOND(vict, 2) = 100;
        GET_LIMBCOND(vict, 3) = 100;
        vict->gainTail();
        improve_skill(ch, SKILL_HEAL, 0);
        WAIT_STATE(ch, PULSE_2SEC);
    }

    return;
}

ACMD(do_barrier)
{
    int prob = 0, perc = 0, size = 0;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!know_skill(ch, SKILL_BARRIER) && !GET_SKILL(ch, SKILL_AQUA_BARRIER))
    {
        return;
    }

    if (!*arg)
    {
        ch->sendText("[Syntax] barrier < 1-75 | release >\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_SANCTUARY) && !strcasecmp("release", arg))
    {
        act("@BYou dispel your barrier, releasing its energy.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@B$n@B dispels $s barrier, releasing its energy.@n", true, ch, nullptr, nullptr, TO_ROOM);
        ch->setBaseStat<int64_t>("barrier", 0);
        ch->affect_flags.set(AFF_SANCTUARY, false);
        return;
    }
    else if (!strcasecmp("release", arg))
    {
        ch->sendText("You don't have a barrier.\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_SANCTUARY))
    {
        ch->sendText("You already have a barrier, try releasing it.\r\n");
        return;
    }

    if (GET_COOLDOWN(ch) > 0)
    {
        ch->sendText("You must wait a short period before concentrating again.\r\n");
        return;
    }

    size = atoi(arg);
    int64_t cost = 0;
    prob = 0;
    if (GET_SKILL(ch, SKILL_BARRIER))
    {
        prob = init_skill(ch, SKILL_BARRIER);
    }
    else
    {
        prob = GET_SKILL(ch, SKILL_AQUA_BARRIER);
    }
    perc = axion_dice(0);

    cost = (GET_MAX_MANA(ch) * 0.01) * (size * 0.5);

    if (size > prob)
    {
        ch->sendText("You can not create a barrier that is stronger than your skill in barrier.\r\n");
        return;
    }
    else if (size < 1)
    {
        ch->sendText("You have to put at least some ki into the barrier!\r\n");
        return;
    }
    else if (size > 75)
    {
        ch->sendText("You can't control a barrier with more than 75 percent!\r\n");
        return;
    }
    else if (GET_CHARGE(ch) < cost)
    {
        ch->sendText("You do not have enough ki charged up!\r\n");
        return;
    }
    else if (prob < perc)
    {
        act("@BYou shout as you form a barrier of ki around your body, but you imbalance it and it explodes outward!@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@B$n@B shouts as $e forms a barrier of ki around $s body, but it becomes imbalanced and explodes outward!@n",
            true, ch, nullptr, nullptr, TO_ROOM);
        ch->modBaseStat<int64_t>("charge", -cost);
        if (GET_SKILL(ch, SKILL_BARRIER))
        {
            improve_skill(ch, SKILL_BARRIER, 2);
        }
        else
        {
            improve_skill(ch, SKILL_AQUA_BARRIER, 2);
        }
        ch->setBaseStat("concentrate_cooldown", 30);
        return;
    }
    else
    {
        if (GET_SKILL(ch, SKILL_BARRIER))
        {
            act("@BYou shout as you form a barrier of ki around your body!@n", true, ch, nullptr, nullptr, TO_CHAR);
            act("@B$n@B shouts as $e forms a barrier of ki around $s body!@n", true, ch, nullptr, nullptr, TO_ROOM);
        }
        else
        {
            act("@BYou shout as you form a barrier of ki and raging waters around your body!@n", true, ch, nullptr,
                nullptr, TO_CHAR);
            act("@B$n@B shouts as $e forms a barrier of ki and raging waters around $s body!@n", true, ch, nullptr,
                nullptr, TO_ROOM);
        }
        ch->setBaseStat<int64_t>("barrier", (GET_MAX_MANA(ch) / 100) * size);
        ch->modBaseStat<int64_t>("charge", -cost);
        if (GET_SKILL(ch, SKILL_BARRIER))
        {
            improve_skill(ch, SKILL_BARRIER, 2);
        }
        else
        {
            improve_skill(ch, SKILL_AQUA_BARRIER, 2);
        }
        ch->affect_flags.set(AFF_SANCTUARY, true);
        ch->setBaseStat("concentrate_cooldown", 20);
        return;
    }
}

ACMD(do_instant)
{

    int skill = 0, perc = 0, skill_num = 0, location = 0;
    int64_t cost = 0;
    Character *tar = nullptr;

    char arg[MAX_INPUT_LENGTH] = "";

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
    if (!know_skill(ch, SKILL_INSTANTT))
    {
        return;
    }
    else if (!GET_SKILL(ch, SKILL_SENSE) && ch->subrace != SubRace::android_model_sense)
    {
        ch->sendText("You can't sense them to go to there!\r\n");
        return;
    }
    else if (PLR_FLAGGED(ch, PLR_PILOTING))
    {
        ch->sendText("You are busy piloting a ship!\r\n");
        return;
    }
    else if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }
    else if (ch->location.getVnum() >= 19800 && ch->location.getVnum() <= 19899)
    {
        ch->sendText("@rYou are in a pocket dimension!@n\r\n");
        return;
    }
    else if (ch->location.getWhereFlag(WhereFlag::afterlife_hell) || ch->location.getWhereFlag(WhereFlag::afterlife) ||
             ch->location.getRoomFlag(ROOM_HELL))
    {
        ch->sendText("You can not leave where you are at!\r\n");
        return;
    }
    else if (!*arg)
    {
        ch->sendText("Who or where do you want to instant transmission to? [target | planet-(planet name)]\r\n");
        ch->sendText("Example: instant goku\nExample 2: instant planet-earth\r\n");
        return;
    }

    if (GET_SKILL(ch, SKILL_INSTANTT) > 75)
    {
        cost = GET_MAX_MANA(ch) / 40;
    }
    else if (GET_SKILL(ch, SKILL_INSTANTT) > 50)
    {
        cost = GET_MAX_MANA(ch) / 20;
    }
    else if (GET_SKILL(ch, SKILL_INSTANTT) > 25)
    {
        cost = GET_MAX_MANA(ch) / 15;
    }
    else if (GET_SKILL(ch, SKILL_INSTANTT) < 25)
    {
        cost = GET_MAX_MANA(ch) / 10;
    }

    if ((ch->getCurVital(CharVital::ki)) - cost < 0)
    {
        ch->sendText("You do not have enough ki to instantaneously move.\r\n");
        return;
    }

    perc = axion_dice(0);
    skill = GET_SKILL(ch, SKILL_INSTANTT);
    skill_num = SKILL_INSTANTT;

    if (!strcasecmp(arg, "planet-earth"))
    {
        location = 300;
    }
    else if (!strcasecmp(arg, "planet-namek"))
    {
        location = 10222;
    }
    else if (!strcasecmp(arg, "planet-frigid"))
    {
        location = 4017;
    }
    else if (!strcasecmp(arg, "planet-vegeta"))
    {
        location = 2200;
    }
    else if (!strcasecmp(arg, "planet-konack"))
    {
        location = 8006;
    }
    else if (!strcasecmp(arg, "planet-aether"))
    {
        location = 12024;
    }
    else if (!(tar = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
    {
        ch->sendText("@RThat target was not found.@n\r\n");
        ch->sendText("Who or where do you want to instant transmission to? [target | planet-(planet name)]\r\n");
        ch->sendText("Example: instant goku\nExample 2: instant planet-earth\r\n");
        return;
    }

    if (skill < perc || (FIGHTING(ch) && rand_number(1, 2) <= 1))
    {
        if (tar)
        {
            if (tar != ch)
            {
                ch->sendText("You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
                ch->modCurVital(CharVital::ki, -cost);
                improve_skill(ch, skill_num, 2);
                WAIT_STATE(ch, PULSE_2SEC);
                return;
            }
            else
            {
                ch->sendText("Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
                return;
            }
        }
        else
        {
            ch->sendText("You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
            ch->modCurVital(CharVital::ki, -cost);
            improve_skill(ch, skill_num, 2);
            WAIT_STATE(ch, PULSE_2SEC);
            return;
        }
    }

    reveal_hiding(ch, 0);
    WAIT_STATE(ch, PULSE_2SEC);
    if (tar)
    {
        if (tar == ch)
        {
            ch->sendText("Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
            return;
        }
        else if (GRAPPLING(ch) && GRAPPLING(ch) == tar)
        {
            ch->sendText("You are already in the same room with them and are grappling with them!\r\n");
            return;
        }
        else if (!read_sense_memory(ch, tar))
        {
            ch->sendText("You've never sensed them up close so you do not have a good bearing on their ki signal.\r\n");
            return;
        }
        else if (GET_ADMLEVEL(tar) > 0 && GET_ADMLEVEL(ch) < 1)
        {
            ch->sendText("That immortal prevents you from reaching them.\r\n");
            return;
        }
        else if (IS_ANDROID(tar) || GET_HIT(tar) < (GET_HIT(ch) * 0.001) + 1)
        {
            ch->sendText("You can't sense them well enough.\r\n");
            return;
        }
        else if (!ch->location.getWhereFlag(WhereFlag::afterlife) && tar->location.getWhereFlag(WhereFlag::afterlife))
        {
            ch->sendText("They are dead and can't be reached.\r\n");
            return;
        }
        else if (!ch->location.getWhereFlag(WhereFlag::afterlife_hell) && tar->location.getWhereFlag(WhereFlag::afterlife_hell))
        {
            ch->sendText("They are dead and can't be reached.\r\n");
            return;
        }
        else if (tar->location.getRoomFlag(ROOM_NOINSTANT))
        {
            ch->sendText("You can not go there as it is a protected area!\r\n");
            return;
        }

        ch->modCurVital(CharVital::ki, -cost);
        act("@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move through the molecules of the universe faster than the speed of light. You stop as you arrive at $N@w!@n",
            true, ch, nullptr, tar, TO_CHAR);
        act("@w$n@w appears in an instant out of nowhere right next to you!@n", true, ch, nullptr, tar, TO_VICT);
        act("@w$n@w places two fingers on $s forehead and disappears in an instant!@n", true, ch, nullptr, tar,
            TO_NOTVICT);
        ch->player_flags.set(PLR_TRANSMISSION, true);
        handle_teleport(ch, tar, 0);
        improve_skill(ch, skill_num, 2);
    }
    else
    {
        ch->modCurVital(CharVital::ki, -cost);
        act("@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move faster than light and arrive almost instantly at your destination. Having located the planet by its collective population's ki.@n",
            true, ch, nullptr, nullptr, TO_CHAR);
        act("@w$n@w places two fingers on $s forehead and disappears in an instant!@n", true, ch, nullptr, nullptr,
            TO_NOTVICT);
        handle_teleport(ch, nullptr, location);
        improve_skill(ch, skill_num, 2);
    }
}

static std::vector<std::tuple<int, int &>> sds = {{SHADOW_DRAGON1_VNUM, SHADOW_DRAGON1}, {SHADOW_DRAGON2_VNUM, SHADOW_DRAGON2}, {SHADOW_DRAGON3_VNUM, SHADOW_DRAGON3}, {SHADOW_DRAGON4_VNUM, SHADOW_DRAGON4}, {SHADOW_DRAGON5_VNUM, SHADOW_DRAGON5}, {SHADOW_DRAGON6_VNUM, SHADOW_DRAGON6}, {SHADOW_DRAGON7_VNUM, SHADOW_DRAGON7}};

void load_shadow_dragons()
{
    for (auto &[vnum, location] : sds)
    {
        if (location > 0)
        {
            auto mob = read_mobile(vnum, VIRTUAL);
            auto room = real_room(location);
            mob->moveToLocation(location);
        }
    }

    save_mud_time(&time_info);
}

void handleShenronAppearance(int &DRAGONC)
{
    auto r = get_room(DRAGONR);
    switch (DRAGONC)
    {
    case 300:
        r->sendText("@WThe dragon balls on the ground begin to glow yellow in slow pulses.@n\r\n");
        send_to_planet(0, WhereFlag::planet_earth, "@DThe sky begins to grow dark and cloudy suddenly.@n\r\n");
        break;
    case 295:
        r->sendText("@WSuddenly lightning shoots into the sky, twisting about as a roar can be heard for miles!@n\r\n");
        send_to_planet(0, WhereFlag::planet_earth, "@DThe sky flashes with lightning.@n\r\n");
        break;
    case 290:
        r->sendText("@WThe lightning takes shape and slowly the Eternal Dragon, Shenron, can be made out from the glow!@n\r\n");
        EDRAGON->leaveLocation();
        EDRAGON->moveToLocation(DRAGONR);
        break;
    case 285:
        send_to_planet(0, WhereFlag::planet_earth, "@DThe lightning stops suddenly, but the sky remains mostly dark.@n\r\n");
        break;
    case 280:
        r->sendText("@WThe glow around Shenron becomes subdued as the Eternal Dragon coils so that his head is looking down on the dragon balls!@n\r\n");
        break;
    case 275:
        r->sendText("@wShenron says, '@CWho summoned me? I will grant you any two wishes that are within my power.@w'@n\r\n");
        break;
    case 180:
        r->sendText("@wShenron says, '@CMake your wish already, you only have 3 minutes remaining.@w'@n\r\n");
        break;
    case 120:
        r->sendText("@wShenron says, '@CMake your wish. I am losing patience, you only have 2 minutes left.@w'@n\r\n");
        break;
    case 60:
        r->sendText("@wShenron says, '@CMake your wish now! You only have 1 minute left.@w'@n\r\n");
        break;
    case 0:
        r->sendText("Shenron growls and disappears with a blinding flash that is absorbed into the dragon balls. The glowing dragon balls then float high into the sky, splitting into several directions and streaking across the sky!@n\r\n");
        send_to_planet(0, WhereFlag::planet_earth, "@DThe sky grows brighter again as the clouds disappear magicly.@n\r\n");
        extract_char(EDRAGON);
        SHENRON = false;
        save_mud_time(&time_info);
        break;
    }
    DRAGONC--;
}

bool placeShadowDragon(int &location)
{
    int num = rand_number(200, 20000);
    while (real_room(num) == NOWHERE || !(WHERE_FLAGGED(real_room(num), WhereFlag::planet_earth) ||
                                          WHERE_FLAGGED(real_room(num), WhereFlag::planet_vegeta) ||
                                          WHERE_FLAGGED(real_room(num), WhereFlag::planet_frigid) ||
                                          WHERE_FLAGGED(real_room(num), WhereFlag::planet_aether) ||
                                          WHERE_FLAGGED(real_room(num), WhereFlag::planet_namek) ||
                                          WHERE_FLAGGED(real_room(num), WhereFlag::planet_konack) ||
                                          WHERE_FLAGGED(real_room(num), WhereFlag::planet_yardrat)))
    {
        num = rand_number(200, 20000);
    }
    location = num;
    return true;
}

void summonShadowDragons()
{
    for (auto &[vn, location] : sds)
    {
        placeShadowDragon(location);
    }

    for (auto &[vn, location] : sds)
    {
        auto m = read_mobile(vn, VIRTUAL);
        m->moveToLocation(location);
    }
}

void wishSYS(uint64_t heartPulse, double deltaTime)
{
    if (!SHENRON)
        return;

    if (SELFISHMETER < 10)
    {
        handleShenronAppearance(DRAGONC);

        if (WISH[0] == 1 && WISH[1] == 1)
        {
            DRAGONC = 0;
            WISH[0] = 0;
            WISH[1] = 0;
        }
    }
    else
    {
        auto r = get_room(DRAGONR);
        r->sendText("@RThe dragon balls suddenly begin to crack and darkness begins to pour out through the cracks! Shenron begins to turn pitch black slowly as the darkness escapes. Suddenly Shenron explodes out into the distance in seven parts. Each part taking a dragon ball with it!@n\r\n");
        DRAGONC = 0;
        WISH[0] = 0;
        WISH[1] = 0;

        summonShadowDragons();

        extract_char(EDRAGON);
        SHENRON = false;
    }
}

ACMD(do_summon)
{

    if (!ch->location.getWhereFlag(WhereFlag::planet_earth))
    {
        ch->sendText("@wYou can not summon Shenron when you are not on earth.@n\r\n");
        return;
    }

    if (ch->location.getRoomFlag(ROOM_NOINSTANT) || ch->location.getRoomFlag(ROOM_PEACEFUL))
    {
        ch->sendText("You can not summon shenron in this protected area!\r\n");
        return;
    }
    if (ch->location.getSectorType() == SectorType::inside)
    {
        ch->sendText("Go outside to summon Shenron! He won't fit in here!\r\n");
        return;
    }

    auto dragonBalls = dball_count(ch);
    if (dragonBalls.size() < 7)
    {
        ch->sendText("You need all 7 Dragon Balls to summon Shenron!\r\n");
        return;
    }

    auto dragon = read_mobile(21, REAL);
    if (!dragon)
    {
        ch->sendText("The Dragon Balls aren't responding. Please contact an immortal.\r\n");
        send_to_imm("Shenron doesn't exist!");
        return;
    }
    dragon->moveToLocation(0);

    reveal_hiding(ch, 0);
    act("@WYou place the dragon balls on the ground and with both hands outstretched towards them you say '@CArise Eternal Dragon Shenron!@W'@n",
        true, ch, nullptr, nullptr, TO_CHAR);
    act("@W$n places the dragon balls on the ground and with both hands outstretched towards them $e says '@CArise Eternal Dragon Shenron!@W'@n",
        true, ch, nullptr, nullptr, TO_ROOM);
    SHENRON = true;
    DRAGONC = 300;
    DRAGONR = ch->location.getVnum();
    DRAGONZ = ch->location.getZone()->number;
    send_to_imm("Shenron summoned to room: %d\r\n", DRAGONR);

    for (auto dball : dragonBalls)
    {
        extract_obj(dball);
    }

    EDRAGON = dragon;
}

ACMD(do_transform)
{

    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char buf3[MAX_INPUT_LENGTH];

    auto npc = IS_NPC(ch);

    // Moved this further down. No need to parse the entry if we quit early. -Volund
    two_arguments(argument, arg, arg2);

    /* Called with no argument - display transformation information */
    if (!*arg)
    {
        trans::displayForms(ch);
        return;
    } /* End of No Argument */

    auto cur_form = ch->form;
    auto cur_tech = ch->technique;

    // If we are in kaioken or something weird like that, prevent transforming.
    if (ch->form == Form::golden_oozaru || ch->form == Form::oozaru)
    {
        ch->sendText("You are the great Oozaru right now and can't transform!\r\n");
        return;
    }

    if (GET_KAIOKEN(ch) > 0)
    {
        ch->sendText("You are in kaioken right now and can't transform!\r\n");
        return;
    }

    int64_t beforeKi = ch->getEffectiveStat<int64_t>("ki");

    // check for revert.
    if (!strcasecmp("revert", arg))
    {
        // Check if we can revert.
        if (ch->form == Form::base && ch->technique == Form::base)
        {
            ch->sendText("You are not transformed.\r\n");
            return;
        }

        // We are all clear to revert.
        if ((GET_CHARGE(ch) > 0))
        {
            do_charge(ch, "release", 0, 0);
        }

        if (ch->form != Form::base)
        {
            trans::revert(ch);
        }
        if (ch->technique != Form::base)
        {
            trans::revert(ch);
        }
        return;
    }

    // Search for available transformations. Error out if we can't find one.
    auto trans_maybe = trans::findFormFor(ch, arg);
    if (!trans_maybe)
    {
        ch->sendText("You don't have that form.\r\n");
        return;
    }
    auto trans = trans_maybe.value();
    int formtype = trans::getFormType(ch, trans);
    int grade = 1;

    if (arg2)
        grade = atoi(arg2);

    if ((cur_form == trans || (formtype == 2 && cur_tech == trans)) && grade == ch->transforms[trans].grade)
    {
        ch->sendText("You are already in that form! Try 'revert'.\r\n");
        return;
    }

    if (!npc && (trans::getRequiredPL(ch, trans) > ch->getBaseStat<int64_t>("health")))
    {
        ch->sendText("You are not strong enough to handle that transformation!\r\n");
        return;
    }

    if (formtype == 1 && ch->permForms.contains(trans))
    {
        ch->sendText("You are already evolved into that form!.\r\n");
        return;
    }

    if (!npc && (ch->getCurVital(CharVital::stamina)) <= GET_MAX_MOVE(ch) * trans::getStaminaDrain(ch, trans))
    {
        ch->sendText("You do not have enough stamina!");
        return;
    }

    if (!npc)
    {
        // Pay the price to unlock form if necessary.
        if (!trans::unlock(ch, trans))
        {
            ch->sendText("You do not have enough Growth to unlock this transformation!\r\n");
            return;
        }
    }

    trans::transform(ch, trans, grade);
    WAIT_STATE(ch, PULSE_5SEC);
}

ACMD(do_situp)
{

    int64_t cost = 1, bonus = 0;

    if (IS_NPC(ch))
    {
        ch->sendText("You are a mob fool!\r\n");
        return;
    }
    if (ch->location.getRoomFlag(ROOM_HELL))
    {
        ch->sendText("The fire makes it too hot!\r\n");
        return;
    }
    if (DRAGGING(ch))
    {
        ch->sendText("You are dragging someone!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_FISHING))
    {
        ch->sendText("Stop fishing first.\r\n");
        return;
    }
    if (CARRYING(ch))
    {
        ch->sendText("You are carrying someone!\r\n");
        return;
    }

    if (IS_ANDROID(ch) || IS_BIO(ch) || IS_MAJIN(ch) || IS_ARLIAN(ch))
    {
        ch->sendText("You will gain nothing from exercising!\r\n");
        return;
    }

    if (!limb_ok(ch, 1))
    {
        return;
    }

    if (!can_grav(ch))
        return;

    if (FIGHTING(ch))
    {
        ch->sendText("You are fighting you moron!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        ch->sendText("You can't do situps in midair!\r\n");
        return;
    }

    auto ratio = ch->getBaseStat("burden_ratio");

    if (ratio <= 0.1)
    {
        ch->sendText("It would simply be too easy like this. Increase your weight or the gravity!\r\n");
        return;
    }

    ch->sendText("You start to do situps.\r\n");
    ch->setTask(Task::situps);
    WAIT_STATE(ch, 2.5);
}

void situpProgress(Character *ch)
{
    int64_t cost = 1, bonus = 0;

    if (ch->location.getRoomFlag(ROOM_HELL))
    {
        ch->sendText("The fire makes it too hot!\r\n");
        ch->setTask(Task::nothing);
        return;
    }
    if (DRAGGING(ch))
    {
        ch->sendText("You are dragging someone!\r\n");
        ch->setTask(Task::nothing);
        return;
    }
    if (PLR_FLAGGED(ch, PLR_FISHING))
    {
        ch->sendText("Stop fishing first.\r\n");
        ch->setTask(Task::nothing);
        return;
    }
    if (CARRYING(ch))
    {
        ch->sendText("You are carrying someone!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (IS_ANDROID(ch) || IS_BIO(ch) || IS_MAJIN(ch) || IS_ARLIAN(ch))
    {
        ch->sendText("You will gain nothing from exercising!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (!limb_ok(ch, 1))
    {
        ch->setTask(Task::nothing);
        return;
    }

    if (!can_grav(ch))
    {
        ch->setTask(Task::nothing);
        return;
    }

    if (FIGHTING(ch))
    {
        ch->sendText("You are fighting you moron!\r\n");
        ch->setTask(Task::nothing);
        return;
    }
    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        ch->sendText("You can't do situps in midair!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    auto ratio = ch->getBaseStat("burden_ratio");

    if (ratio <= 0.1)
    {
        ch->sendText("It would simply be too easy like this. Increase your weight or the gravity!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    cost = ch->getMaxVitalPercent(CharVital::stamina, 0.04) * Random::get<double>(0.8, 1.2);
    cost *= (1.0 + ratio);

    if (GET_BONUS(ch, BONUS_HARDWORKER) > 0)
    {
        cost -= cost * .25;
    }
    else if (GET_BONUS(ch, BONUS_SLACKER) > 0)
    {
        cost += cost * .25;
    }

    if (GET_RELAXCOUNT(ch) >= 464)
    {
        cost *= 50;
    }
    else if (GET_RELAXCOUNT(ch) >= 232)
    {
        cost *= 15;
    }
    else if (GET_RELAXCOUNT(ch) >= 116)
    {
        cost *= 4;
    }

    if ((ch->getCurVital(CharVital::stamina)) < cost)
    {
        ch->sendText("You are too tired!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (ratio <= 0.1)
    {
        act("@gYou do a situp.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n does a situp.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (ratio <= 0.3)
    {
        act("@gYou do a situp, and feel the burn.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n does a situp, while sweating.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (ratio <= 0.65)
    {
        act("@gYou do a situp, and really strain against the gravity.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n does a situp, while sweating.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else
    {
        act("@gYou do a situp, and it was a really hard one to finish.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n does a situp, while sweating profusely.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }

    // double level_impact = (1.0 - (2 * std::max<double>(0, (double) GET_LEVEL(ch) - 51.0) / 100.0));

    double base = (double)ch->getBaseStat<int64_t>("stamina");
    double start_bonus = Random::get<double>(0.8, 1.2) * (1 + (GET_CON(ch) / 20)) * ch->getPotential();
    double ratio_bonus = 1.0 + (3.0 * ratio);
    double soft_cap = (double)ch->calc_soft_cap();
    double diminishing_returns = (soft_cap - base) / soft_cap;
    if (diminishing_returns > 0.0)
        diminishing_returns = std::max<double>(diminishing_returns, 0.05);
    else
        diminishing_returns = 0;
    bonus = (start_bonus * ratio_bonus) * diminishing_returns;
    if (bonus <= 0)
        bonus = 0;

    if (ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
    {
        ch->sendText("@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
        bonus *= 8;
        if (bonus <= 10)
            bonus = 10;
    }
    else if (ch->location.getRoomFlag(ROOM_WORKOUT))
    {
        if (ch->location.getVnum() >= 19100 && ch->location.getVnum() <= 19199)
        {
            bonus *= 5;
            if (bonus <= 6)
                bonus = 6;
        }
        else
        {
            bonus *= 2;
            if (bonus <= 4)
                bonus = 4;
        }
    }
    else if (ch->location.getVnum() >= 19800 && ch->location.getVnum() <= 19899)
    {
        ch->sendText("@rThis place feels like... Magic.@n\r\n");
        bonus *= 10;
        if (bonus <= 12)
            bonus = 12;
    }
    /* Rillao: transloc, add new transes here */

    if (IS_NAMEK(ch))
    {
        bonus -= bonus / 4;
    }
    if (GET_BONUS(ch, BONUS_HARDWORKER))
    {
        bonus += bonus * 0.5;
    }
    if (GET_BONUS(ch, BONUS_LONER))
    {
        bonus += bonus * 0.1;
    }
    if (bonus <= 0)
        bonus = 1;
    // Bonus due to prolonging exercise
    bonus *= 2;
    if (bonus > (ch->getBaseStat<int64_t>("stamina") / 40))
        bonus = ch->getBaseStat<int64_t>("stamina") / 40;

    ch->send_to("You feel slightly more vigorous @D[@G+%s@D]@n.\r\n", add_commas(bonus).c_str());
    ch->gainBaseStat("stamina", bonus);
    WAIT_STATE(ch, 2.5);
    ch->modCurVital(CharVital::stamina, -cost);
}

ACMD(do_meditate)
{

    int64_t bonus = 0, cost = 1;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (IS_NPC(ch))
    {
        ch->sendText("You are a mob fool!\r\n");
        return;
    }
    if (ch->location.getRoomFlag(ROOM_HELL))
    {
        ch->sendText("The fire makes it too hot!\r\n");
        return;
    }
    if (PLR_FLAGGED(ch, PLR_FISHING))
    {
        ch->sendText("Stop fishing first.\r\n");
        return;
    }

    if (IS_ANDROID(ch) || IS_BIO(ch) || IS_MAJIN(ch) || IS_ARLIAN(ch))
    {
        ch->sendText("You will gain nothing from exercising!\r\n");
        return;
    }

    if (CARRYING(ch))
    {
        ch->sendText("You are carrying someone!\r\n");
        return;
    }

    if (DRAGGING(ch))
    {
        ch->sendText("You are dragging someone!\r\n");
        return;
    }

    if (FIGHTING(ch))
    {
        ch->sendText("You are fighting you moron!\r\n");
        return;
    }

    if (GET_POS(ch) != POS_SITTING)
    {
        // Attempt to sit...
        do_sit(ch, "", 0, 0);
        if (GET_POS(ch) != POS_SITTING)
        {
            ch->sendText("You need to be sitting to meditate.\r\n");
            return;
        }
    }

    if (!strcasecmp(arg, "expand"))
    {
        int cost = 3500;
        if (IS_SAIYAN(ch))
        {
            cost = 7000;
        }
        if (GET_PRACTICES(ch) < cost)
        {
            ch->sendText("You do not have enough practice sessions to expand your mind and ability to remember skills.\r\n");
            ch->send_to("%s needed.\r\n", add_commas(cost).c_str());
        }
        else if (GET_SLOTS(ch) >= 60 && GET_BONUS(ch, BONUS_GMEMORY) == 0)
        {
            ch->sendText("You can not have any more slots through this process.\r\n");
        }
        else if (GET_SLOTS(ch) >= 65 && GET_BONUS(ch, BONUS_GMEMORY) == 1)
        {
            ch->sendText("You can not have any more slots through this process.\r\n");
        }
        else
        {
            ch->sendText("During your meditation you manage to expand your mind and get the feeling you could learn some new skills.\r\n");
            ch->modBaseStat<int>("skill_slots", 1);
            ch->modPractices(-cost);
            return;
        }
        return;
    }
    else if (!strcasecmp(arg, "break"))
    {
        if (MINDLINK(ch) == nullptr)
        {
            ch->sendText("You are not mind linked with anyone.\r\n");
            return;
        }
        else if (LINKER(ch) == 1)
        {
            ch->sendText("This is not how you break YOUR mind link.\r\n");
            return;
        }
        else if ((ch->getCurVital(CharVital::ki)) < GET_MAX_MANA(MINDLINK(ch)) * 0.05)
        {
            ch->sendText("You do not have enough ki to manage a break.\r\n");
            return;
        }
        else if (GET_INT(ch) + rand_number(-5, 10) >=
                 GET_INT(MINDLINK(ch)) + (GET_SKILL(MINDLINK(ch), SKILL_TELEPATHY) * 0.1))
        {
            act("@rYou manage to break the mind link between you and @R$N@r!@n", false, ch, nullptr, MINDLINK(ch),
                TO_CHAR);
            act("$n closes their eyes for a few seconds.", false, ch, nullptr, MINDLINK(ch), TO_ROOM);
            MINDLINK(ch)->sendText("@rYour mind linked target manages to push you out!@n\r\n");
            if (GET_INT(MINDLINK(ch)) < axion_dice(-10) && !AFF_FLAGGED(MINDLINK(ch), AFF_SHOCKED))
            {
                MINDLINK(ch)->sendText("Your mind is shocked by the flood of mental energy that pushed it out!@n\r\n");
                MINDLINK(ch)->affect_flags.set(AFF_SHOCKED, false);
            }

            MINDLINK(ch)->setBaseStat("mind_linker", 0);
            MINDLINK(MINDLINK(ch)) = nullptr;
            MINDLINK(ch) = nullptr;
            return;
        }
        else
        {
            act("@rYou struggle to free your mind of @R$N's@r link, but fail!@n", false, ch, nullptr, MINDLINK(ch),
                TO_CHAR);
            act("$n closes their eyes for a few seconds, and appears to struggle quite a bit.", false, ch, nullptr,
                MINDLINK(ch), TO_ROOM);
            MINDLINK(ch)->sendText("@rYour mind linked target struggles to free their mind, but fails!@n\r\n");

            ch->modCurVital(CharVital::ki, -(GET_MAX_MANA(MINDLINK(ch)) * .05));
            return;
        }
    }

    ch->sendText("You start to meditate.\r\n");
    ch->setTask(Task::meditate);
    WAIT_STATE(ch, 2.5);
}

void meditateProgress(Character *ch)
{
    int64_t bonus = 0, cost = 1;

    if (ch->location.getRoomFlag(ROOM_HELL))
    {
        ch->sendText("The fire makes it too hot!\r\n");
        ch->setTask(Task::nothing);
        return;
    }
    if (PLR_FLAGGED(ch, PLR_FISHING))
    {
        ch->sendText("Stop fishing first.\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (IS_ANDROID(ch) || IS_BIO(ch) || IS_MAJIN(ch) || IS_ARLIAN(ch))
    {
        ch->sendText("You will gain nothing from exercising!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (CARRYING(ch))
    {
        ch->sendText("You are carrying someone!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (DRAGGING(ch))
    {
        ch->sendText("You are dragging someone!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (FIGHTING(ch))
    {
        ch->sendText("You are fighting you moron!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (GET_POS(ch) != POS_SITTING)
    {
        ch->sendText("You need to be sitting to meditate.\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    auto ratio = ch->getBaseStat("burden_ratio");

    if (ratio <= 0.1)
    {
        ch->sendText("It would simply be too easy like this. Increase your weight or the gravity!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    cost = ch->getMaxVitalPercent(CharVital::ki, 0.04) * Random::get<double>(0.8, 1.2);
    cost *= (1.0 + ratio);

    if (GET_BONUS(ch, BONUS_HARDWORKER) > 0)
    {
        cost -= cost * .25;
    }
    else if (GET_BONUS(ch, BONUS_SLACKER) > 0)
    {
        cost += cost * .25;
    }

    if (GET_RELAXCOUNT(ch) >= 464)
    {
        cost *= 50;
    }
    else if (GET_RELAXCOUNT(ch) >= 232)
    {
        cost *= 15;
    }
    else if (GET_RELAXCOUNT(ch) >= 116)
    {
        cost *= 4;
    }

    if ((ch->getCurVital(CharVital::ki)) < cost)
    {
        ch->sendText("You don't have enough ki!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (ratio <= 0.1)
    {
        act("@gYou close your eyes and focus on circulating your vital energies throughout your body.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n meditates calmly.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (ratio <= 0.3)
    {
        act("@gYou close your eyes and focus on circulating your vital energies throughout your body, against a solid burden.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n meditates calmly, while sweating.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (ratio <= 0.65)
    {
        act("@gYou close your eyes and focus on circulating your vital energies throughout your body, struggling to maintain focus against an intense burden.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n meditates calmly, while sweating.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else
    {
        act("@gYou close your eyes and focus on circulating your vital energies throughout your body, concentration strained to the limit by immense outward pressures.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n meditates calmly, while sweating profusely.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }

    // double level_impact = (1.0 - (2 * std::max<double>(0, (double) GET_LEVEL(ch) - 51.0) / 100.0));

    double base = (double)ch->getBaseStat<int64_t>("ki");
    double start_bonus = Random::get<double>(0.8, 1.2) * (1 + (GET_WIS(ch) / 20)) * ch->getPotential();
    double ratio_bonus = 1.0 + (3.0 * ratio);
    double soft_cap = (double)ch->calc_soft_cap();
    double diminishing_returns = (soft_cap - base) / soft_cap;
    if (diminishing_returns > 0.0)
        diminishing_returns = std::max<double>(diminishing_returns, 0.05);
    else
        diminishing_returns = 0;
    bonus = (start_bonus * ratio_bonus) * diminishing_returns;
    if (bonus <= 0)
        bonus = 0;

    if (ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
    {
        if (bonus <= 0)
            bonus = 10;
        ch->sendText("@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
        bonus *= 5;
    }
    else if (ch->location.getRoomFlag(ROOM_WORKOUT))
    {
        if (ch->location.getVnum() >= 19100 && ch->location.getVnum() <= 19199)
        {
            if (bonus <= 0)
                bonus = 6;
            bonus *= 5;
        }
        else
        {
            if (bonus <= 0)
                bonus = 3;
            bonus *= 2;
        }
    }
    else if (ch->location.getVnum() >= 19800 && ch->location.getVnum() <= 19899)
    {
        ch->sendText("@rThis place feels like... Magic.@n\r\n");
        bonus *= 10;
    }
    else
    {
        if (bonus <= 0)
            bonus = 1;
    }
    if (bonus <= 0 && !ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
    {
        bonus = 1;
    }
    if (bonus <= 1 && ch->location.getRoomFlag(ROOM_WORKOUT))
    {
        if (ch->location.getVnum() >= 19100 && ch->location.getVnum() <= 19199)
        {
            bonus = 6;
        }
        else
        {
            bonus = 3;
        }
    }
    if (bonus > 0 && IS_DEMON(ch) && rand_number(1, 100) >= 80)
    {
        ch->send_to("Your spirit magnifies the strength of your body! @D[@G+%s@D]@n\r\n", add_commas(bonus / 2).c_str());
        ch->gainBaseStat("health", bonus / 2);
    }

    bonus += GET_WIS(ch) / 20;
    if (IS_NAMEK(ch))
    {
        bonus += bonus / 2;
    }
    if (GET_BONUS(ch, BONUS_LONER))
    {
        bonus += bonus * 0.1;
    }
    if (bonus <= 0)
        bonus = 0;
    // Bonus due to prolonging exercise
    bonus *= 2;
    if (bonus > (ch->getBaseStat<int64_t>("ki") / 40))
        bonus = ch->getBaseStat<int64_t>("ki") / 40;

    ch->send_to("You feel your spirit grow stronger @D[@G+%s@D]@n.\r\n", add_commas(bonus).c_str());
    ch->gainBaseStat("ki", bonus);
    WAIT_STATE(ch, 2.5);
    ch->modCurVital(CharVital::ki, -cost);
}

ACMD(do_pushup)
{

    int64_t cost = 1, bonus = 0;

    if (IS_NPC(ch))
    {
        ch->sendText("You are a mob fool!\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_FISHING))
    {
        ch->sendText("Stop fishing first.\r\n");
        return;
    }
    if (DRAGGING(ch))
    {
        ch->sendText("You are dragging someone!\r\n");
        return;
    }
    if (CARRYING(ch))
    {
        ch->sendText("You are carrying someone!\r\n");
        return;
    }

    if (IS_ANDROID(ch) || IS_BIO(ch) || IS_MAJIN(ch) || IS_ARLIAN(ch))
    {
        ch->sendText("You will gain nothing from exercising!\r\n");
        return;
    }

    if (!limb_ok(ch, 0))
    {
        return;
    }

    if (!can_grav(ch))
        return;

    if (FIGHTING(ch))
    {
        ch->sendText("You are fighting you moron!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        ch->sendText("You can't do pushups in midair!\r\n");
        return;
    }

    auto ratio = ch->getBaseStat("burden_ratio");

    if (ratio <= 0.1)
    {
        ch->sendText("It would simply be too easy like this. Increase your weight or the gravity!\r\n");
        return;
    }

    ch->sendText("You start to do push-ups.\r\n");
    ch->setTask(Task::pushups);
    WAIT_STATE(ch, 2.5);
}

void pushupProgress(Character *ch)
{
    int64_t cost = 1, bonus = 0;

    if (PLR_FLAGGED(ch, PLR_FISHING))
    {
        ch->sendText("Stop fishing first.\r\n");
        ch->setTask(Task::nothing);
        return;
    }
    if (DRAGGING(ch))
    {
        ch->sendText("You are dragging someone!\r\n");
        ch->setTask(Task::nothing);
        return;
    }
    if (CARRYING(ch))
    {
        ch->sendText("You are carrying someone!\r\n");
        ch->setTask(Task::nothing);
        return;
    }

    if (IS_ANDROID(ch) || IS_BIO(ch) || IS_MAJIN(ch) || IS_ARLIAN(ch))
    {
        ch->setTask(Task::nothing);
        ch->sendText("You will gain nothing from exercising!\r\n");
        return;
    }

    if (!limb_ok(ch, 0))
    {
        ch->setTask(Task::nothing);
        return;
    }

    if (!can_grav(ch))
    {
        ch->setTask(Task::nothing);
        return;
    }

    if (FIGHTING(ch))
    {
        ch->sendText("You are fighting you moron!\r\n");
        ch->setTask(Task::nothing);
        return;
    }
    if (AFF_FLAGGED(ch, AFF_FLYING))
    {
        ch->setTask(Task::nothing);
        ch->sendText("You can't do pushups in midair!\r\n");
        return;
    }

    auto ratio = ch->getBaseStat("burden_ratio");

    if (ratio <= 0.1)
    {
        ch->setTask(Task::nothing);
        ch->sendText("It would simply be too easy like this. Increase your weight or the gravity!\r\n");
        return;
    }

    cost = ch->getMaxVitalPercent(CharVital::stamina, 0.04) * Random::get<double>(0.8, 1.2);
    cost *= (1.0 + ratio);

    if (GET_BONUS(ch, BONUS_HARDWORKER) > 0)
    {
        cost -= cost * .25;
    }
    else if (GET_BONUS(ch, BONUS_SLACKER) > 0)
    {
        cost += cost * .25;
    }

    if (GET_RELAXCOUNT(ch) >= 464)
    {
        cost *= 50;
    }
    else if (GET_RELAXCOUNT(ch) >= 232)
    {
        cost *= 15;
    }
    else if (GET_RELAXCOUNT(ch) >= 116)
    {
        cost *= 4;
    }

    if ((ch->getCurVital(CharVital::stamina)) < cost)
    {
        ch->setTask(Task::nothing);
        ch->sendText("You are too tired!\r\n");
        return;
    }

    if (ratio <= 0.1)
    {
        act("@gYou do a pushup.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n does a pushup.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (ratio <= 0.3)
    {
        act("@gYou do a pushup, and feel the burn.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n does a pushup, while sweating.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else if (ratio <= 0.65)
    {
        act("@gYou do a pushup, and really strain to keep it up.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n does a pushup, while sweating.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    else
    {
        act("@gYou do a pushup, and it was a really hard one to finish.@n", true, ch, nullptr, nullptr, TO_CHAR);
        act("@g$n does a pushup, while sweating profusely.@n", true, ch, nullptr, nullptr, TO_ROOM);
    }

    // double level_impact = (1.0 - (2 * std::max<double>(0, (double) GET_LEVEL(ch) - 51.0) / 100.0));

    double base = (double)ch->getBaseStat<int64_t>("health");
    double start_bonus = Random::get<double>(0.8, 1.2) * (1 + (GET_CON(ch) / 20)) * ch->getPotential();
    double ratio_bonus = 1.0 + (3.0 * ratio);
    double soft_cap = (double)ch->calc_soft_cap();
    double diminishing_returns = (soft_cap - base) / soft_cap;
    if (diminishing_returns > 0.0)
        diminishing_returns = std::max<double>(diminishing_returns, 0.05);
    else
        diminishing_returns = 0;
    bonus = (start_bonus * ratio_bonus) * diminishing_returns;
    if (bonus <= 0)
        bonus = 0;

    if (ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
    {
        ch->sendText("@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
        bonus *= 5;
        if (bonus <= 6)
            bonus = 6;
    }
    else if (ch->location.getRoomFlag(ROOM_WORKOUT))
    {
        if (ch->location.getVnum() >= 19100 && ch->location.getVnum() <= 19199)
        {
            bonus *= 5;
            if (bonus <= 6)
                bonus = 6;
        }
        else
        {
            bonus *= 2;
            if (bonus <= 4)
                bonus = 4;
        }
    }
    else if (ch->location.getVnum() >= 19800 && ch->location.getVnum() <= 19899)
    {
        ch->sendText("@rThis place feels like... Magic.@n\r\n");
        bonus *= 10;
        if (bonus <= 12)
            bonus = 12;
    }

    bonus += GET_CON(ch) / 20;
    if (IS_NAMEK(ch))
    {
        bonus -= bonus / 4;
    }
    if (GET_BONUS(ch, BONUS_HARDWORKER))
    {
        bonus += bonus * 0.5;
    }
    if (GET_BONUS(ch, BONUS_LONER))
    {
        bonus += bonus * 0.1;
    }
    /* Rillao: transloc, add new transes here */

    if (IS_HUMAN(ch))
    {
        bonus = bonus * 0.8;
    }
    if (bonus <= 0)
        bonus = 1;

    // Bonus for longer task
    bonus *= 2;

    if (bonus > (ch->getBaseStat<int64_t>("health") / 40))
        bonus = ch->getBaseStat<int64_t>("health") / 40;
    ch->send_to("You feel slightly stronger @D[@G+%s@D]@n.\r\n", add_commas(bonus).c_str());
    ch->gainBaseStat("health", bonus);
    WAIT_STATE(ch, 2.5);
    ch->modCurVital(CharVital::stamina, -cost);
}

ACMD(do_spar)
{
    // Will return true when you have PLR_SPAR flagged
    if (ch->character_flags.toggle(CharacterFlag::sparring))
    {
        act("@wYou move into your sparring stance.@n", false, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@w moves into $s sparring stance.@n", false, ch, nullptr, nullptr, TO_ROOM);
    }
    else
    {

        act("@wYou cease your sparring stance.@n", false, ch, nullptr, nullptr, TO_CHAR);
        act("@C$n@w ceases $s sparring stance.@n", false, ch, nullptr, nullptr, TO_ROOM);
    }
}

static void check_eq(Character *ch)
{
    Object *obj;
    int i;
    for (i = 0; i < NUM_WEARS; i++)
    {
        if (GET_EQ(ch, i))
        {
            obj = GET_EQ(ch, i);
            if (OBJ_FLAGGED(obj, ITEM_BROKEN))
            {
                act("@W$p@W falls apart and you remove it.@n", false, ch, obj, nullptr, TO_CHAR);
                act("@W$p@W falls apart and @C$n@W remove it.@n", false, ch, obj, nullptr, TO_ROOM);
                perform_remove(ch, i);
                return;
            }
            if (obj == GET_EQ(ch, WEAR_WIELD1) && GET_LIMBCOND(ch, 0) <= 0)
            {
                act("@WWithout your right arm you let go of @c$p@W!@n", false, ch, obj, nullptr, TO_CHAR);
                act("@C$n@W lets go of @c$p@W!@n", false, ch, obj, nullptr, TO_ROOM);
                perform_remove(ch, i);
                return;
            }
            if (obj == GET_EQ(ch, WEAR_WIELD2) && GET_LIMBCOND(ch, 1) <= 0)
            {
                act("@WWithout your left arm you let go of @c$p@W!@n", false, ch, obj, nullptr, TO_CHAR);
                act("@C$n@W lets go of @c$p@W!@n", false, ch, obj, nullptr, TO_ROOM);
                perform_remove(ch, i);
                return;
            }
        }
    }
}

/* This handles many player specific routines. It may be a bit too bloated though. */
void base_update(uint64_t heartPulse, double deltaTime)
{
    struct descriptor_data *d;
    int cash = false, inc = 0;
    int countch = false, pcoun = 0;

    if (INTERESTTIME != 0 && INTERESTTIME <= time(nullptr) && time(nullptr) != 0)
    {
        INTERESTTIME = time(nullptr) + 86400;
        LASTINTEREST = time(nullptr);
        save_mud_time(&time_info);
        cash = true;
        countch = true;
    }

    if (TOPCOUNTDOWN > 0)
    {
        TOPCOUNTDOWN -= 4;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (!IS_PLAYING(d))
            continue;

        if (countch == true)
        {
            pcoun += 1;
        }
        if (!IS_NPC(d->character) && rand_number(1, 15) >= 14)
        {
            ash_burn(d->character);
        }
        auto forty_lf = (d->character->getEffectiveStat("lifeforce")) * 0.4;
        if (AFF_FLAGGED(d->character, AFF_CURSE) && (d->character->getCurVital(CharVital::lifeforce)) > forty_lf)
        {
            d->character->modCurVitalDam(CharVital::lifeforce, .01);
            demon_refill_lf(d->character, (d->character->getEffectiveStat("lifeforce")) * 0.01);

            if ((d->character->getCurVital(CharVital::lifeforce)) < forty_lf)
            {
                d->character->modCurVital(CharVital::lifeforce, forty_lf - d->character->getCurVital(CharVital::lifeforce));
            }
        }
        if (GET_BACKSTAB_COOL(d->character) > 0)
        {
            d->character->modBaseStat("backstab_cooldown", -1);
        }

        if (GET_COOLDOWN(d->character) > 0)
        {
            if (d->character->modBaseStat<int>("concentrate_cooldown", -2) == 0)
            {
                d->character->sendText("You can concentrate again.\r\n");
            }
        }
        /* Andros Start */
        if (GET_SDCOOLDOWN(d->character) > 0)
        {
            d->character->modBaseStat("selfdestruct_cooldown", -10);
            if (GET_SDCOOLDOWN(d->character) <= 0)
            {
                d->character->setBaseStat("selfdestruct_cooldown", 0);
                d->character->sendText("Your body has recovered from your last selfdestruct.\r\n");
            }
        } /* Andros End */
        if (CARRYING(d->character))
        {
            if (CARRYING(d->character)->location != d->character->location)
            {
                carry_drop(d->character, 3);
            }
        }
        if (GET_DEFENDER(d->character))
        {
            if (d->character->location != GET_DEFENDER(d->character)->location)
            {
                GET_DEFENDING(GET_DEFENDER(d->character)) = nullptr;
                GET_DEFENDER(d->character) = nullptr;
            }
        }
        if (GET_DEFENDING(d->character))
        {
            if (d->character->location != GET_DEFENDING(d->character)->location)
            {
                GET_DEFENDER(GET_DEFENDING(d->character)) = nullptr;
                GET_DEFENDING(d->character) = nullptr;
            }
        }
        d->character->player_flags.set(PLR_TRANSMISSION, false);

        if (!FIGHTING(d->character) && AFF_FLAGGED(d->character, AFF_POSITION))
        {
            d->character->affect_flags.set(AFF_POSITION, false);
        }
        if (SITS(d->character))
        {
            if (d->character->location != SITS(d->character)->location)
            {
                Object *chair = SITS(d->character);
                chair->sitting.reset();
                d->character->sits.reset();
            }
        }
        if (GET_PING(d->character) >= 1)
        {
            d->character->modBaseStat<int>("ping", -1);
            if (PLR_FLAGGED(d->character, PLR_PILOTING) && GET_PING(d->character) == 0)
            {
                d->character->sendText("Your radar is ready to calculate the direction of another destination.\r\n");
            }
        }
        if (GET_ADMLEVEL(d->character) < 1 && TOPCOUNTDOWN <= 0 && GET_LEVEL(d->character) > 0)
        {
            topWrite(d->character);
        }
        if (PLR_FLAGGED(d->character, PLR_SELFD) && !PLR_FLAGGED(d->character, PLR_SELFD2))
        {
            if (rand_number(4, 100) < GET_SKILL(d->character, SKILL_SELFD))
            {
                d->character->sendText("You feel you are ready to self destruct!\r\n");
                d->character->player_flags.set(PLR_SELFD2, true);
            }
        }
        if (!FIGHTING(d->character) && COMBO(d->character) > -1)
        {
            d->character->setBaseStat<int>("combo", -1);
            d->character->setBaseStat<int>("combo_hits", 0);
        }

        if (cash == true && GET_BANK_GOLD(d->character) > 0)
        {
            inc = (GET_BANK_GOLD(d->character) / 50) * 2;
            d->character->setBaseStat("last_interest", LASTINTEREST);
            if (inc >= 25000)
            {
                inc = 25000;
            }
            d->character->modBaseStat("money_bank", inc);
            d->character->send_to("@cBank Interest@D: @Y%s@n\r\n", add_commas(inc).c_str());
        }
        if (!IS_NPC(d->character))
        {
            check_eq(d->character);
        }
        if (!IS_NPC(d->character) && d->character->location.getGroundEffect() >= 1 && rand_number(1, 100) >= 96)
        {
            if (d->character->location.getGroundEffect() <= 4)
            {
                switch (rand_number(1, 4))
                {
                case 1:
                    act("@RLava spews up violently from the cracks in the ground!@n", false, d->character, nullptr,
                        nullptr, TO_ROOM);
                    act("@RLava spews up violently from the cracks in the ground!@n", false, d->character, nullptr,
                        nullptr, TO_CHAR);
                    break;
                case 2:
                    act("@RThe lava bubbles and gives off tremendous heat!@n", false, d->character, nullptr,
                        nullptr, TO_ROOM);
                    act("@RThe lava bubbles and gives off tremendous heat!@n", false, d->character, nullptr,
                        nullptr, TO_CHAR);
                    break;
                case 3:
                    act("@RNoxious fumes rise from the bubbling lava!@n", false, d->character, nullptr, nullptr,
                        TO_ROOM);
                    act("@RNoxious fumes rise from the bubbling lava!@n", false, d->character, nullptr, nullptr,
                        TO_CHAR);
                    break;
                case 4:
                    act("@RSome of the lava cools as it spreads further from the source!@n", false, d->character,
                        nullptr, nullptr, TO_ROOM);
                    act("@RSome of the lava cools as it spreads further from the source!@n", false, d->character,
                        nullptr, nullptr, TO_CHAR);
                    break;
                }
                d->character->location.modGroundEffect(1);
            }
            else if (d->character->location.getGroundEffect() == 5)
            {
                act("@RLava covers the entire area now!@n", false, d->character, nullptr, nullptr, TO_ROOM);
                act("@RLava covers the entire area now!@n", false, d->character, nullptr, nullptr, TO_CHAR);
                d->character->location.modGroundEffect(1);
            }
        }

        if (BLOCKS(d->character))
        {
            Character *vict = BLOCKS(d->character);
            if (vict->location != d->character->location)
            {
                BLOCKED(vict) = nullptr;
                BLOCKS(d->character) = nullptr;
            }
        }
        if (GET_SPAM(d->character) > 0)
        {
            d->character->setBaseStat("spam", 0);
        }
        else
            continue;
    }

    if (countch == true)
    {
        PCOUNT = pcoun;
        PCOUNTDAY = time(nullptr);
    }

    if (TOPCOUNTDOWN <= 0)
    {
        TOPCOUNTDOWN = 60;
    }
}

static int has_scanner(Character *ch)
{
    return ch->searchInventory(13600) ? true : false;
}

ACMD(do_snet)
{
    int channel = 0, global = false, call = -1, reached = false;
    struct descriptor_data *i;
    char voice[150], arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char hist[MAX_INPUT_LENGTH];

    half_chop(argument, arg, arg2);

    Object *obj = nullptr;
    Object *obj2 = nullptr;

    if (ch->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
    {
        ch->sendText("This is a different dimension!\r\n");
        return;
    }
    if (IN_ARENA(ch))
    {
        ch->sendText("Lol, no.\r\n");
        return;
    }
    if (ch->location.getWhereFlag(WhereFlag::pendulum_past))
    {
        ch->sendText("This is the past, you can't talk on scouter net!\r\n");
        return;
    }
    if (ch->location.getRoomFlag(ROOM_HELL))
    {
        ch->sendText("The fire eats your transmission!\r\n");
        return;
    }
    if (ch->location.getVnum() >= 19800 && ch->location.getVnum() <= 19899)
    {
        ch->sendText("Your signal will not be able to escape the walls of the pocket dimension.\r\n");
        return;
    }

    if (!IS_NPC(ch))
    {
        if (GET_EQ(ch, WEAR_EYE))
        {
            obj = GET_EQ(ch, WEAR_EYE);
        }
        else
        {
            ch->sendText("You do not have a scouter on.\r\n");
            return;
        }
    }

    if (!*arg)
    {
        ch->sendText("[Syntax] snet < [1-999] | check | #(scouter number) | * message | message>\r\n");
        return;
    }

    if (strstr(arg, "#"))
    {
        search_replace(arg, "#", "");
        call = atoi(arg);
        if (call <= -1)
        {
            ch->sendText("Call what personal scouter number?\r\n");
            return;
        }
    }

    if (!strcasecmp(arg, "check"))
    {
        ch->send_to("Your personal scouter number is: %d\r\n", ((ch)->id));
        return;
    }

    if (call <= -1)
    {
        channel = atoi(arg);
    }

    if (channel > 0)
    {
        obj->setBaseStat("scoutfreq", channel);
        act("@wYou push some buttons on $p@w and change its channel.", true, ch, obj, nullptr, TO_CHAR);
        act("@C$n@w pushes some buttons on $p@w and changes its channel.", true, ch, obj, nullptr, TO_ROOM);
        return;
    }
    else
    {
        if (GET_BONUS(ch, BONUS_MUTE) > 0)
        {
            ch->sendText("You are unable to speak though.\r\n");
            return;
        }
        if (SFREQ(obj) == 0)
        {
            obj->setBaseStat("scoutfreq", 1);
        }
        if (!strcasecmp(arg, "*") && call <= -1)
        {
            global = true;
        }
        if (GET_VOICE(ch))
        {
            sprintf(voice, "%s", GET_VOICE(ch));
        }
        if (GET_VOICE(ch) == nullptr)
        {
            sprintf(voice, "A generic voice");
        }
        for (i = descriptor_list; i; i = i->next)
        {
            if (STATE(i) != CON_PLAYING)
            {
                continue;
            }
            if (i->character == ch)
            {
                continue;
            }
            if (i->character->location == ch->location)
            {
                continue;
            }
            if (i->character->location.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
            {
                continue;
            }
            if (i->character->location.getWhereFlag(WhereFlag::pendulum_past))
            {
                continue;
            }
            if ((i->character->location.getWhereFlag(WhereFlag::afterlife_hell) && !ch->location.getWhereFlag(WhereFlag::afterlife_hell)) ||
                (i->character->location.getWhereFlag(WhereFlag::afterlife) && !ch->location.getWhereFlag(WhereFlag::afterlife)))
            {
                continue;
            }
            if ((!i->character->location.getWhereFlag(WhereFlag::afterlife_hell) && ch->location.getWhereFlag(WhereFlag::afterlife_hell)) ||
                (!i->character->location.getWhereFlag(WhereFlag::afterlife) && ch->location.getWhereFlag(WhereFlag::afterlife)))
            {
                continue;
            }
            if (GET_POS(i->character) == POS_SLEEPING)
            {
                continue;
            }
            if (i->character->location.getVnum() >= 19800 && i->character->location.getVnum() <= 19899)
            {
                continue;
            }
            if (GET_EQ(i->character, WEAR_EYE))
            {
                obj2 = GET_EQ(i->character, WEAR_EYE);
                if (SFREQ(obj2) == 0)
                {
                    obj2->setBaseStat("scoutfreq", 1);
                }
                if (global == false && call <= -1 && SFREQ(obj2) == SFREQ(obj) && GET_ADMLEVEL(i->character) < 1)
                {
                    i->character->send_to("@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
                    *hist = '\0';
                    snprintf(hist, sizeof(hist), "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice,
                             readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", SFREQ(obj),
                             CAP(arg), !*arg2 ? "" : arg2);
                    add_history(i->character, hist, HIST_SNET);
                    if (has_scanner(i->character))
                    {
                        i->character->send_to("@WScanner@D: @Y%s@n\r\n", sense_location(ch));
                    }
                    continue;
                } /* It is the right freq */
                else if (global == true && call <= -1 && GET_ADMLEVEL(i->character) < 1)
                {
                    i->character->send_to("@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", SFREQ(obj), CAP(arg2));
                    *hist = '\0';
                    sprintf(hist, "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", voice,
                            readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", SFREQ(obj),
                            CAP(arg2));
                    add_history(i->character, hist, HIST_SNET);
                    if (has_scanner(i->character))
                    {
                        i->character->send_to("@WScanner@D: @Y%s@n\r\n", sense_location(ch));
                    }
                    continue;
                }
                else if (call > -1 && ((i->character)->id) == call)
                {
                    i->character->send_to("@C%s is heard @W(@c%s@W), @D[@R#@W%d @Ycalling YOU@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", ((ch)->id), !*arg2 ? "" : CAP(arg2));
                    *hist = '\0';
                    sprintf(hist, "@C%s is heard @W(@c%s@W), @D[@R#@W%ld @Ycalling YOU@D] @G%s@n\r\n", voice,
                            readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", ((ch)->id),
                            !*arg2 ? "" : CAP(arg2));
                    add_history(i->character, hist, HIST_SNET);
                    if (has_scanner(i->character))
                    {
                        i->character->send_to("@WScanner@D: @Y%s@n\r\n", sense_location(ch));
                    }
                    reached = true;
                }
            } /* They have a scouter */
            if (GET_ADMLEVEL(i->character) > 0 && call <= -1)
            {
                i->character->send_to("@C%s (%s) is heard, @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice, GET_NAME(ch), SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
                *hist = '\0';
                snprintf(hist, sizeof(hist), "@C%s (%s) is heard, @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice,
                         GET_NAME(ch), SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
                add_history(i->character, hist, HIST_SNET);
                continue;
            }
            else if (GET_ADMLEVEL(i->character) > 0)
            {
                i->character->send_to("@C%s (%s) is heard, @D[@WCall to @R#@Y%d@D] @G%s@n\r\n", voice, GET_NAME(ch), call, !*arg2 ? "" : CAP(arg2));
                *hist = '\0';
                snprintf(hist, sizeof(hist), "@C%s (%s) is heard, @D[@WCall to @R#@Y%d@D] @G%s@n\r\n", voice,
                         GET_NAME(ch), call, !*arg2 ? "" : CAP(arg2));
                add_history(i->character, hist, HIST_SNET);
                continue;
            }
        } /* End switch */
        if (call <= -1)
        {
            if (global == false)
            {
                reveal_hiding(ch, 3);
                ch->send_to("@CYou @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", SFREQ(obj), arg, !*arg2 ? "" : arg2);
                *hist = '\0';
                snprintf(hist, sizeof(hist), "@CYou @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", SFREQ(obj), arg, !*arg2 ? "" : arg2);
                add_history(ch, hist, HIST_SNET);
                char over[MAX_STRING_LENGTH];
                snprintf(over, sizeof(over), "@C$n@W says into $s scouter, '@G@G%s %s@W'@n\r\n", CAP(arg), !*arg2 ? "" : arg2);
                act(over, true, ch, nullptr, nullptr, TO_ROOM);
                if (ch->location.getWhereFlag(WhereFlag::afterlife_hell) || ch->location.getWhereFlag(WhereFlag::afterlife))
                {
                    ch->sendText("@mThe transmission only reaches those who are in the afterlife.@n\r\n");
                }
            }
            if (global == true)
            {
                reveal_hiding(ch, 3);
                ch->send_to("@CYou @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", SFREQ(obj), !*arg2 ? "" : CAP(arg2));
                *hist = '\0';
                snprintf(hist, sizeof(hist), "@CYou @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", SFREQ(obj),
                         !*arg2 ? "" : CAP(arg2));
                add_history(ch, hist, HIST_SNET);
                char over[MAX_STRING_LENGTH];
                snprintf(over, sizeof(over), "@C$n@W says into $s scouter, '@G@G%s@W'@n\r\n", !*arg2 ? "" : CAP(arg2));
                act(over, true, ch, nullptr, nullptr, TO_ROOM);
                if (ch->location.getWhereFlag(WhereFlag::afterlife_hell) || ch->location.getWhereFlag(WhereFlag::afterlife))
                {
                    ch->sendText("@mThe transmission only reaches those who are in the afterlife.@n\r\n");
                }
            }
        }
        else
        {
            reveal_hiding(ch, 3);
            ch->send_to("@CYou call @D[@R#@W%d@D] @G%s@n\r\n", call, !*arg2 ? "" : CAP(arg2));
            *hist = '\0';
            snprintf(hist, sizeof(hist), "@CYou call @D[@R#@W%d@D] @G%s@n\r\n", call, !*arg2 ? "" : CAP(arg2));
            add_history(ch, hist, HIST_SNET);
            char over[MAX_STRING_LENGTH];
            snprintf(over, sizeof(over), "@C$n@W says into $s scouter, '@G@G%s@W'@n\r\n", !*arg2 ? "" : CAP(arg2));
            act(over, true, ch, nullptr, nullptr, TO_ROOM);
            if (reached == false)
            {
                ch->sendText("@mThe transmission didn't reach them.@n\r\n");
            }
        }
    } /* end if statement */
} /*end snet command */

ACMD(do_scouter)
{
    Character *vict = nullptr;
    struct descriptor_data *i;
    int count = 0;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!HAS_ARMS(ch))
    {
        ch->sendText("You have no available arms!\r\n");
        return;
    }

    Object *obj = GET_EQ(ch, WEAR_EYE);
    if (!obj)
    {
        ch->sendText("You do not even have a scouter!");
        return;
    }
    if (!*arg)
    {
        ch->sendText("[Syntax] scouter < target | scan>\r\n");
        return;
    }
    reveal_hiding(ch, 3);
    if (!strcasecmp("scan", arg))
    {
        for (i = descriptor_list; i; i = i->next)
        {
            if (STATE(i) != CON_PLAYING)
            {
                continue;
            }
            else if (i->character == ch)
            {
                continue;
            }
            else if (IS_ANDROID(i->character))
            {
                continue;
            }
            else if (planet_check(ch, i->character))
            {
                // TODO: replace graphing algorithm...
                int dir = find_first_step(ch->location, i->character->location);
                int same = false;
                char pathway[MAX_STRING_LENGTH];

                if (IN_ZONE(ch) == IN_ZONE(i->character))
                    same = true;

                switch (dir)
                {
                case BFS_ERROR:
                    sprintf(pathway, "@rERROR");
                    break;
                case BFS_ALREADY_THERE:
                    sprintf(pathway, "@RHERE");
                    break;
                case BFS_NO_PATH:
                    ch->sendText("@MUNKNOWN");
                    break;
                default:
                    ch->send_to("@G%s\r\n", dirs[dir]);
                    break;
                }

                auto blah = sense_location(i->character);
                auto cpl = i->character->getPL();
                if (cpl > obj->getBaseStat<int64_t>(VAL_WORN_SCOUTER))
                {
                    ch->send_to("@D<@GPowerlevel Detected@D:@w ?????????@D> @w---> @C%s@n\r\n", same == true ? pathway : blah);
                }
                else
                {
                    ch->send_to("@D<@GPowerlevel Detected@D: [@Y%s@D]@w ---> @C%s@n\r\n", add_commas(cpl).c_str(), same == true ? pathway : blah);
                }
                ++count;
            }
        }
        if (count == 0)
        {
            ch->sendText("You didn't detect anyone of notice.\r\n");
            return;
        }
        else if (count >= 1)
        {
            ch->send_to("%d powerlevels detected.\r\n", count);
            return;
        }
    }
    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("They don't seem to be here.\r\n");
        return;
    }
    act("$n points $s scouter at you.", false, ch, nullptr, vict, TO_VICT);
    act("$n points $s scouter at $N.", false, ch, nullptr, vict, TO_NOTVICT);
    if (IS_ANDROID(vict))
    {
        ch->sendText("@D,==================================|@n\r\n");
        ch->sendText("@D|@1                                  @n@D|@n\r\n");
        ch->sendText("@D|@1@RReading target...                 @n@D|@n\r\n");
        ch->sendText("@D|@1                                  @n@D|@n\r\n");
        ch->sendText("@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D:                 @RERROR@n@D|@n\r\n");
        ch->sendText("@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D:                 @RERROR@n@D|@n\r\n");
        ch->sendText("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D:                 @RERROR@n@D|@n\r\n");
        ch->sendText("@D|@1                                  @n@D|@n\r\n");
        ch->sendText("@D|@1@GE@g@1x@Gt@g@1r@Ga I@g@1nf@Go @D:                 @RERROR@n@D|@n\r\n");
        ch->sendText("@D|@1                                  @n@D|@n\r\n");
        ch->sendText("@D`==================================|@n\r\n");
        return;
    }
    auto vpl = vict->getPL();
    long double percent = 0.0, cur = 0.0, max = 0.0;
    int64_t stam = (vict->getCurVital(CharVital::stamina)), mstam = GET_MAX_MOVE(vict);
    if (stam <= 0)
        stam = 1;
    if (mstam <= 0)
        mstam = 1;
    cur = (long double)(stam);
    max = (long double)(mstam);

    percent = (cur / max) * 100;

    ch->sendText("@D,==================================|@n\r\n");
    ch->sendText("@D|@1                                  @n@D|@n\r\n");
    ch->sendText("@D|@1@RReading target...                 @n@D|@n\r\n");
    ch->sendText("@D|@1                                  @n@D|@n\r\n");
    if (obj->getBaseStat<int64_t>(VAL_WORN_SCOUTER) <= vpl)
        ch->send_to("@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D: @Y%21s@n@D|@n\r\n", add_commas(vpl).c_str());
    else
        ch->send_to("@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D: @Y%21s@n@D|@n\r\n", "??????????");
    if (!IS_NPC(vict))
    {
        ch->send_to("@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D: @Y%21s@n@D|@n\r\n", add_commas(GET_CHARGE(vict)).c_str());
    }
    else if (IS_NPC(vict))
    {
        ch->send_to("@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D: @Y%21s@n@D|@n\r\n", add_commas(vict->getBaseStat<int>("mobcharge") * rand_number(GET_INT(ch) * 50, GET_INT(ch) * 200)).c_str());
    }
    if (percent < 10)
        ch->send_to("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Exhausted");
    else if (percent < 25)
        ch->send_to("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Extremely Tired");
    else if (percent < 50)
        ch->send_to("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Very Tired");
    else if (percent < 75)
        ch->send_to("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Tired");
    else if (percent < 90)
        ch->send_to("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Winded");
    else if (percent < 100)
        ch->send_to("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Untired");
    else if (percent >= 100)
        ch->send_to("@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Energetic");
    ch->sendText("@D|@1                                  @n@D|@n\r\n");
    int check = false;
    ch->sendText("@D|@1@GE@g@1x@Gt@g@1r@Ga I@g@1nf@Go @D: ");
    if (AFF_FLAGGED(vict, AFF_ZANZOKEN))
    {
        ch->send_to("@Y%21s@n@D|@n\n", "Zanzoken Prepared");
        check = true;
    }
    if (AFF_FLAGGED(vict, AFF_HASS))
    {
        ch->send_to("%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "Accelerated Arms");
        check = true;
    }
    if (AFF_FLAGGED(vict, AFF_HEALGLOW))
    {
        ch->send_to("%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "Healing Glow Prepared");
        check = true;
    }
    if (AFF_FLAGGED(vict, AFF_POISON))
    {
        ch->send_to("%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "Poisoned");
        check = true;
    }
    if (PLR_FLAGGED(vict, PLR_SELFD))
    {
        ch->send_to("%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "Explosive Energy");
        check = true;
    }
    if (check == false)
    {
        ch->send_to("%s@Y%21s@n@D|@n\n", check == true ? "@D|@1             " : "", "None Detected.");
    }
    ch->sendText("@D|@1                                  @n@D|@n\r\n");
    ch->sendText("@D`==================================|@n\r\n");
}

std::unordered_set<Object *> dball_count(Character *ch)
{
    std::unordered_set<obj_vnum> dbVn;
    dbVn.insert(dbVnums.begin(), dbVnums.end());

    auto isDragonBall = [&](Object *obj)
    {
        if (dbVn.contains(GET_OBJ_VNUM(obj)))
        {
            dbVn.erase(GET_OBJ_VNUM(obj));
            return true;
        }
        return false;
    };

    return ch->gatherFromInventory(isDragonBall);
}

ACMD(do_quit)
{
    if (IS_NPC(ch) || !ch->desc)
        return;

    if (ch->location.getWhereFlag(WhereFlag::pendulum_past))
    {
        ch->sendText("This is the past, you can't quit here!\r\n");
        return;
    }

    auto rvn = ch->location.getVnum();

    if (rvn >= 2002 && rvn <= 2011)
    {
        ch->sendText("You can't quit in the arena!\r\n");
        return;
    }
    if (rvn >= 101 && rvn <= 139)
    {
        ch->sendText("You can't quit in the mud school!\r\n");
        return;
    }
    if (rvn >= 19800 && rvn <= 19899)
    {
        ch->sendText("You can't quit in a pocket dimension!\r\n");
        return;
    }
    if (rvn == 2069)
    {
        ch->sendText("You can't quit here!\r\n");
        return;
    }
    if (MINDLINK(ch) && LINKER(ch) == 0)
    {
        ch->sendText("@RYou feel like the mind that is linked with yours is preventing you from quiting!@n\r\n");
        if (IN_ROOM(MINDLINK(ch)) != NOWHERE)
        {
            ch->lookAtLocation(MINDLINK(ch)->location);
            ch->sendText("You get an impression of where this interference is originating from.\r\n");
        }
        return;
    }
    if (rvn == 2070)
    {
        ch->sendText("You can't quit here!\r\n");
        return;
    }
    if (!dball_count(ch).empty())
    {
        ch->sendText("You can not quit while you have dragon balls! Place them somewhere first.");
        return;
    }

    if (subcmd != SCMD_QUIT)
        ch->sendText("You have to type quit--no less, to quit!\r\n");
    else if (GET_POS(ch) == POS_FIGHTING)
        ch->sendText("No way!  You're fighting for your life!\r\n");
    else if (GET_POS(ch) < POS_STUNNED)
    {
        ch->sendText("You die before your time...\r\n");
        die(ch, nullptr);
    }
    else
    {
        act("$n has left the game.", true, ch, nullptr, nullptr, TO_ROOM);
        mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), true, "%s has quit the game.", GET_NAME(ch));
        ch->sendText("Goodbye, friend.. Come back soon!\r\n");
        if (ch->followers || ch->master)
            die_follower(ch);
        if (ch == ch_selling)
            stop_auction(AUC_QUIT_CANCEL, nullptr);

        /*  We used to check here for duping attempts, but we may as well
         *  do it right in extract_char(), since there is no check if a
         *  player rents out and it can leave them in an equally screwy
         *  situation.
         */

        /* If someone is quitting in their house, let them load back here. */
        if (!ch->location.getWhereFlag(WhereFlag::pendulum_past) &&
            (rvn < 19800 || rvn > 19899))
        {
            if (rvn != NOWHERE && rvn != 0 &&
                rvn != 1)
            {
                ch->setBaseStat("load_room", rvn);
            }
        }
        if (ch->location.getWhereFlag(WhereFlag::pendulum_past))
        {
            if (rvn != NOWHERE && rvn != 0 &&
                rvn != 1)
            {
                ch->setBaseStat("load_room", GET_ROOM_VNUM(real_room(1561)));
            }
        }
        extract_char(ch); /* Char is saved before extracting. */
    }
    /* Remove any snoopers */
    if (ch->desc->snoop_by)
    {
        ch->desc->snoop_by->sendText("Your victim is no longer among us.\r\n");
        ch->desc->snoop_by->snooping = nullptr;
        ch->desc->snoop_by = nullptr;
    }
}

ACMD(do_save)
{
    ch->sendText("The game dumps automatically every 5 minutes now.\r\n");
}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
    ch->sendText("Sorry, but you cannot do that here!\r\n");
}

ACMD(do_steal)
{

    if (!GET_SKILL(ch, SKILL_SLEIGHT_OF_HAND) && slot_count(ch) + 1 <= GET_SLOTS(ch))
    {
        ch->sendText("You learn the very veeeery basics of theft. Which is don't get caught.\r\n");
        SET_SKILL(ch, SKILL_SLEIGHT_OF_HAND, 1);
    }
    else if (!GET_SKILL(ch, SKILL_SLEIGHT_OF_HAND) && slot_count(ch) + 1 > GET_SLOTS(ch))
    {
        ch->sendText("You can't learn any more skills and thus can not steal right now!\r\n");
        return;
    }

    Character *vict;
    Object *obj;
    char arg[500], arg2[500];
    int gold = 0, prob = GET_SKILL(ch, SKILL_SLEIGHT_OF_HAND), perc = 0, eq_pos;

    two_arguments(argument, arg, arg2);

    if (!*arg)
    {
        ch->sendText("An important basic of theft is actually having a victim!\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("Steal what from who?\r\n");
        return;
    }
    else if (vict == ch)
    {
        ch->sendText("Come on now, that's rather stupid!\r\n");
        return;
    }
    else if (!can_kill(ch, vict, nullptr, 0))
    {
        return;
    }
    else if (GET_LEVEL(ch) <= 8)
    {
        ch->sendText("You are trapped inside the newbie shield until level 9 and can't piss off those bigger and better than you. Awww...\r\n");
        return;
    }
    else if (MOB_FLAGGED(vict, MOB_NOKILL) && GET_ADMLEVEL(ch) == ADMLVL_NONE)
    {
        ch->sendText("That isn't such a good idea...\r\n");
        return;
    }

    if ((ch->getCurVital(CharVital::stamina)) < (GET_MAX_MOVE(ch) / 40) + ch->getEffectiveStat("weight_carried"))
    {
        ch->sendText("You do not have enough stamina.\r\n");
        return;
    }

    if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_SPOT))
    {
        perc = GET_SKILL(vict, SKILL_SPOT);
        perc += GET_INT(vict);
    }
    else
    {
        perc = rand_number(GET_INT(vict), GET_INT(vict) + 10);
        if (IS_NPC(vict))
            perc += GET_LEVEL(vict) * 0.25;
    }

    if (GET_POS(vict) == POS_SITTING)
        perc -= 5;
    if (GET_POS(vict) == POS_RESTING)
        perc -= 10;
    if (GET_POS(vict) <= POS_SLEEPING)
        perc -= 25; /* High chance to succeed. */

    prob += GET_DEX(ch);

    perc += rand_number(-5, 5); /* Random factor */
    prob += rand_number(-5, 5); /* Random factor */

    if (axion_dice(0) > 100 && GET_POS(vict) != POS_SLEEPING)
    { /* Unlucky! */
        reveal_hiding(ch, 0);
        act("@r$N@R just happens to glance in your direction! What terrible luck!@n", true, ch, nullptr, vict, TO_CHAR);
        act("@RYou just happen to glance behind you and spot @r$n@R trying to STEAL from you!@n", true, ch, nullptr,
            vict, TO_VICT);
        act("@r$N@R just happens to glance in @r$n's@R direction and catches $m trying to STEAL!@n", true, ch, nullptr,
            vict, TO_NOTVICT);
        prob = -1000;
    }

    if (prob + 20 < perc && GET_POS(vict) != POS_SLEEPING)
    { /* Critical failure! */
        reveal_hiding(ch, 0);
        act("@rYou are caught trying to stick your hand in @R$N's@r possessions!@n", true, ch, nullptr, vict, TO_CHAR);
        act("@rYou catch @R$n@r trying to rummage through your possessions!@n", true, ch, nullptr, vict, TO_VICT);
        act("@R$n@R is caught by @R$N@r as $e sticks $s hand in @R$N's@r possessions!@n", true, ch, nullptr, vict,
            TO_NOTVICT);
        WAIT_STATE(ch, PULSE_3SEC);
        if (IS_NPC(vict))
        {
            set_fighting(vict, ch);
        }
        improve_skill(vict, SKILL_SPOT, 1);
    }
    else
    { /* Ok it wasn't a critical failure... */
        if (!strcasecmp(arg, "zenni"))
        { /* MOOLA! */
            if (prob > perc)
            {
                if (GET_GOLD(vict) > 0)
                {
                    if (GET_GOLD(vict) > 100)
                    {
                        gold = (GET_GOLD(vict) / 100) * rand_number(1, 10);
                    }
                    else
                    {
                        gold = GET_GOLD(vict);
                    }
                    if (gold + GET_GOLD(ch) > GOLD_CARRY(ch))
                    {
                        ch->sendText("You can't hold that much more zenni on your person!\r\n");
                        return;
                    }
                    vict->modBaseStat("money_carried", -gold);
                    ch->modBaseStat("money_carried", gold);
                    if (!IS_NPC(vict))
                    {
                        vict->player_flags.set(PLR_STOLEN, true);
                        mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true,
                               "THEFT: %s has stolen %s zenni@n from %s", GET_NAME(ch), add_commas(gold).c_str(),
                               GET_NAME(vict));
                    }
                    if (gold > 1)
                        ch->send_to("Bingo!  You got %d zenni.\r\n", gold);
                    else
                        ch->sendText("You manage to swipe a solitary zenni.\r\n");
                    if (axion_dice(0) > prob)
                    {
                        ch->sendText("You think that your movements might have been a bit obvious.\r\n");
                        reveal_hiding(ch, 0);
                        act("@R$n@r just stole zenni from @R$N@r!@n", true, ch, nullptr, vict, TO_ROOM);
                        vict->sendText("You feel like something may be missing...\r\n");
                        if (IS_NPC(vict) && rand_number(1, 3) == 3)
                        {
                            set_fighting(vict, ch);
                        }
                        improve_skill(vict, SKILL_SPOT, 1);
                    }
                    improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
                    return;
                }
                else
                {
                    ch->sendText("It appears like they are broke...\r\n");
                    return;
                }
            }
            else
            { /* Failure! */
                reveal_hiding(ch, 0);
                act("@rYou are caught trying to steal zenni from @R$N@r!@n", true, ch, nullptr, vict, TO_CHAR);
                act("@rYou catch @R$n's@r hand trying to snatch your zenni!@n", true, ch, nullptr, vict, TO_VICT);
                act("@R$N@r catches @R$n's@r hand trying to snatch $S zenni!@n", true, ch, nullptr, vict, TO_NOTVICT);
                WAIT_STATE(ch, PULSE_3SEC);
                if (IS_NPC(vict))
                {
                    set_fighting(vict, ch);
                }
                improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 2);
                improve_skill(vict, SKILL_SPOT, 1);
                return;
            }
        }
        else
        { /* It's an object... */
            if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, vict->getInventory())))
            {
                for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
                    if (GET_EQ(vict, eq_pos) && (isname(arg, GET_EQ(vict, eq_pos)->getName())) &&
                        CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos)))
                    {
                        obj = GET_EQ(vict, eq_pos);
                        break;
                    }
                if (!obj)
                {
                    act("$E isn't wearing that item.", false, ch, nullptr, vict, TO_CHAR);
                    return;
                }
                else if (GET_POS(vict) > POS_SLEEPING)
                {
                    ch->sendText("Steal worn equipment from them while they are awake? That's a stupid idea...\r\n");
                    return;
                }
                else if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj))
                {
                    ch->sendText("Impossible!\r\n");
                    return;
                }
                else if (GET_OBJ_VNUM(obj) >= 20000)
                {
                    ch->sendText("You can't steal that!\r\n");
                    return;
                }
                else if (GET_OBJ_VNUM(obj) >= 18800 && GET_OBJ_VNUM(obj) <= 18999)
                {
                    ch->sendText("You can't steal that!\r\n");
                    return;
                }
                else if (GET_OBJ_VNUM(obj) >= 19100 && GET_OBJ_VNUM(obj) <= 19199)
                {
                    ch->sendText("You can't steal that!\r\n");
                    return;
                }
                else if (GET_OBJ_TYPE(obj) == ITEM_KEY)
                {
                    ch->sendText("No stealing keys!\r\n");
                    return;
                }
                else if (OBJ_FLAGGED(obj, ITEM_NOSTEAL))
                {
                    ch->sendText("You can't steal that!\r\n");
                    return;
                }
                else if (GET_OBJ_WEIGHT(obj) + (ch->getEffectiveStat("weight_carried")) > CAN_CARRY_W(ch))
                {
                    ch->sendText("You can't carry that much weight.\r\n");
                    return;
                }
                else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
                {
                    ch->sendText("You don't have the room for it right now!\r\n");
                    return;
                }
                else if (prob > perc)
                { /* Right off their back :) */
                    act("You unequip $p and steal it.", false, ch, obj, vict, TO_CHAR);
                    if (axion_dice(0) > prob)
                    {
                        ch->sendText("You think that your movements might have been a bit obvious.\r\n");
                        reveal_hiding(ch, 0);
                        act("@R$n@r just stole $p@r from @R$N@r!@n", true, ch, obj, vict, TO_ROOM);
                        vict->sendText("You feel your body being disturbed.\r\n");
                        improve_skill(vict, SKILL_SPOT, 1);
                    }
                    auto un = unequip_char(vict, eq_pos);
                    ch->addToInventory(un);
                    improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
                    return;
                }
                else
                { /* Failure! */
                    reveal_hiding(ch, 0);
                    vict->setBaseStat("combo", POS_SITTING);
                    act("@rYou are caught trying to steal $p@r from @R$N@r!@n", true, ch, obj, vict, TO_CHAR);
                    act("@rYou feel your body being shifted while you sleep and wake up to find @R$n@r trying to steal $p@r from you!@n",
                        true, ch, obj, vict, TO_VICT);
                    act("@R$N@r catches @R$n's@r trying to $p@r from $M during $S sleep!@n", true, ch, obj, vict,
                        TO_NOTVICT);
                    WAIT_STATE(ch, PULSE_3SEC);
                    if (IS_NPC(vict))
                    {
                        vict->setBaseStat("combo", POS_STANDING);
                        set_fighting(vict, ch);
                    }
                    improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 2);
                    improve_skill(vict, SKILL_SPOT, 1);
                    return;
                }
            }
            else
            { /* It's in their inventory */
                if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj))
                {
                    ch->sendText("Impossible!\r\n");
                    return;
                }
                else if (GET_OBJ_VNUM(obj) >= 20000)
                {
                    ch->sendText("You can't steal that!\r\n");
                    return;
                }
                else if (OBJ_FLAGGED(obj, ITEM_NOSTEAL))
                {
                    ch->sendText("You can't steal that!\r\n");
                    return;
                }
                else if (GET_OBJ_TYPE(obj) == ITEM_KEY)
                {
                    ch->sendText("No stealing keys!\r\n");
                    return;
                }
                else if (GET_OBJ_WEIGHT(obj) + (ch->getEffectiveStat("weight_carried")) > CAN_CARRY_W(ch))
                {
                    ch->sendText("You can't carry that much weight.\r\n");
                    return;
                }
                else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
                {
                    ch->sendText("You don't have the room for it right now!\r\n");
                    return;
                }
                else if (prob > perc)
                { /* Right out of their pockets */
                    act("You steal $p from $N.", false, ch, obj, vict, TO_CHAR);
                    obj->clearLocation();
                    ch->addToInventory(obj);
                    if (!IS_NPC(vict))
                    {
                        vict->player_flags.set(PLR_STOLEN, true);
                        mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true, "THEFT: %s has stolen %s@n from %s",
                               GET_NAME(ch), obj->getShortDescription(), GET_NAME(vict));
                    }
                    if (axion_dice(0) > prob)
                    {
                        reveal_hiding(ch, 0);
                        ch->sendText("You think that your movements might have been a bit obvious.\r\n");
                        act("@R$n@r just stole $p@r from @R$N@r!@n", true, ch, obj, vict, TO_ROOM);
                        vict->sendText("You feel like something may be missing...\r\n");
                        if (IS_NPC(vict) && rand_number(1, 3) == 3)
                        {
                            set_fighting(vict, ch);
                        }
                        improve_skill(vict, SKILL_SPOT, 1);
                    }
                    improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
                    return;
                }
                else
                { /* Failure! */
                    reveal_hiding(ch, 0);
                    act("@rYou are caught trying to steal $p@r from @R$N@r!@n", true, ch, obj, vict, TO_CHAR);
                    act("@rYou catch @R$n@r trying to steal $p@r from you!@n", true, ch, obj, vict, TO_VICT);
                    act("@R$N@r catches @R$n's@r trying to $p@r!@n", true, ch, obj, vict, TO_NOTVICT);
                    WAIT_STATE(ch, PULSE_3SEC);
                    if (IS_NPC(vict))
                    {
                        vict->setBaseStat("combo", POS_STANDING);
                        set_fighting(vict, ch);
                    }
                    improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 2);
                    improve_skill(vict, SKILL_SPOT, 1);
                    return;
                }
            }
        } /* End of an object */
    }
}

ACMD(do_practice)
{
    char arg[200];

    /* if (IS_NPC(ch))
    return; */

    one_argument(argument, arg);

    if (*arg)
        ch->sendText("You can only practice skills with your trainer.\r\n");
    else
        ch->sendText("Use the skills command unless you are at your trainer.\r\n");
    /*list_skills(ch);*/
}

ACMD(do_skills)
{
    char arg[1000];
    one_argument(argument, arg);
    if (IS_NPC(ch))
    {
        return;
    }
    list_skills(ch, arg);
}

ACMD(do_visible)
{
    int appeared = 0;

    if (GET_ADMLEVEL(ch))
    {
        perform_immort_vis(ch);
        return;
    }

    if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    {
        appear(ch);
        appeared = 1;
        ch->sendText("You break the spell of invisibility.\r\n");
    }

    if (AFF_FLAGGED(ch, AFF_ETHEREAL) && affectedv_by_spell(ch, ART_EMPTY_BODY))
    {
        affectv_from_char(ch, ART_EMPTY_BODY);
        if (AFF_FLAGGED(ch, AFF_ETHEREAL))
        {
            ch->sendText("Returning to the material plane will not be so easy.\r\n");
        }
        else
        {
            ch->sendText("You return to the material plane.\r\n");
            if (!appeared)
                act("$n flashes into existence.", false, ch, nullptr, nullptr, TO_ROOM);
        }
        appeared = 1;
    }

    if (!appeared)
        ch->sendText("You are already visible.\r\n");
}

ACMD(do_title)
{
    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (IS_NPC(ch))
        ch->sendText("Your title is fine... go away.\r\n");
    else if (PLR_FLAGGED(ch, PLR_NOTITLE))
        ch->sendText("You can't title yourself -- you shouldn't have abused it!\r\n");
    else if (strstr(argument, "(") || strstr(argument, ")"))
        ch->sendText("Titles can't contain the ( or ) characters.\r\n");
    else if (strlen(argument) > MAX_TITLE_LENGTH)
        ch->send_to("Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
    else
    {
        set_title(ch, argument);
        ch->send_to("Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
    }
}

static int
perform_group(Character *ch, Character *vict, int highlvl, int lowlvl, int64_t highpl, int64_t lowpl)
{
    if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
        return (0);

    if (GET_BONUS(vict, BONUS_LONER) > 0)
    {
        act("$n is the loner type and refuses to be in your group.", true, vict, nullptr, ch, TO_VICT);
        return (0);
    }

    /*
  if (GET_LEVEL(vict) + 12 < highlvl) {
   act("$n isn't experienced enough to be in your group with its current members.", TRUE, vict, 0, ch, TO_VICT);
   return (0);
  }

  if (GET_LEVEL(vict) > lowlvl + 12) {
   act("$n is too experienced to be in your group with its current members.", TRUE, vict, 0, ch, TO_VICT);
   return (0);
  }

  if (highlvl >= 100) {
   if ((vict->getEffMaxPL()) > highpl * 1.5) {
    act("$n is too powerful right now to be in a level 100 group with you.", TRUE, vict, 0, ch, TO_VICT);
    return (0);
   }

   if ((vict->getEffMaxPL()) < lowpl * 0.5) {
    act("$n is too weak right now to be in a level 100 group with you.", TRUE, vict, 0, ch, TO_VICT);
    return (0);
   }
  }
  */

    vict->affect_flags.set(AFF_GROUP, true);
    if (ch != vict)
        act("$N is now a member of your group.", false, ch, nullptr, vict, TO_CHAR);
    act("You are now a member of $n's group.", false, ch, nullptr, vict, TO_VICT);
    act("$N is now a member of $n's group.", false, ch, nullptr, vict, TO_NOTVICT);
    return (1);
}

static void print_group(Character *ch)
{
    Character *k;
    struct follow_type *f;

    if (!AFF_FLAGGED(ch, AFF_GROUP))
        ch->sendText("But you are not the member of a group!\r\n");
    else
    {
        char buf[MAX_STRING_LENGTH];

        ch->sendText("Your group consists of:\r\n");

        k = (ch->master ? ch->master : ch);

        if (AFF_FLAGGED(k, AFF_GROUP))
        {
            ch->sendText("@D----------------@n\r\n");
            if (GET_HIT(k) > GET_MAX_HIT(k) / 10)
            {
                snprintf(buf, sizeof(buf),
                         "@gL@D: @w$N @W- @D[@RPL@Y: @c%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]@n",
                         add_commas(GET_HIT(k)).c_str(), add_commas((k->getCurVital(CharVital::ki))).c_str(), add_commas((k->getCurVital(CharVital::stamina))).c_str(), GET_LEVEL(k),
                         CLASS_ABBR(k), race::getAbbr(k->race).c_str());
            }
            if (GET_HIT(k) <= (GET_MAX_HIT(k) - (k->getEffectiveStat("weight_carried"))) / 10)
            {
                snprintf(buf, sizeof(buf),
                         "@gL@D: @w$N @W- @D[@RPL@Y: @r%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]@n",
                         add_commas(GET_HIT(k)).c_str(), add_commas((k->getCurVital(CharVital::ki))).c_str(), add_commas((k->getCurVital(CharVital::stamina))).c_str(), GET_LEVEL(k),
                         CLASS_ABBR(k), race::getAbbr(k->race).c_str());
            }
            act(buf, false, ch, nullptr, k, TO_CHAR);
        }

        k->followers.for_each([&](Character* f) {
            if (!AFF_FLAGGED(f, AFF_GROUP)) return;
            ch->sendText("@D----------------@n\r\n");
            if (GET_HIT(f) > (GET_MAX_HIT(f) - (f->getEffectiveStat("weight_carried"))) / 10)
            {
                snprintf(buf, sizeof(buf),
                         "@gF@D: @w$N @W- @D[@RPL@Y: @c%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]",
                         add_commas(GET_HIT(f)).c_str(), add_commas((f->getCurVital(CharVital::ki))).c_str(), add_commas((f->getCurVital(CharVital::stamina))).c_str(),
                         GET_LEVEL(f), CLASS_ABBR(f), race::getAbbr(f->race).c_str());
            }
            if (GET_HIT(f) <= (GET_MAX_HIT(f) - (f->getEffectiveStat("weight_carried"))) / 10)
            {
                snprintf(buf, sizeof(buf),
                         "@gF@D: @w$N @W- @D[@RPL@Y: @r%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]",
                         add_commas(GET_HIT(f)).c_str(), add_commas((f->getCurVital(CharVital::ki))).c_str(), add_commas((f->getCurVital(CharVital::stamina))).c_str(),
                         GET_LEVEL(f), CLASS_ABBR(f), race::getAbbr(f->race).c_str());
            }
            act(buf, false, ch, nullptr, f, TO_CHAR);
        });
        ch->sendText("@D----------------@n\r\n");
    }
}

ACMD(do_group)
{
    char buf[MAX_STRING_LENGTH];
    Character *vict;
    struct follow_type *f;
    int found, highlvl = 0, lowlvl = 0;
    int64_t highpl = 0, lowpl = 0;

    one_argument(argument, buf);

    if (GET_BONUS(ch, BONUS_LONER) > 0)
    {
        ch->sendText("You can not group as you prefer to be alone.\r\n");
        return;
    }

    if (!*buf)
    {
        print_group(ch);
        return;
    }

    if (ch->master)
    {
        act("You cannot enroll group members without being head of a group.",
            false, ch, nullptr, nullptr, TO_CHAR);
        return;
    }

    highlvl = GET_LEVEL(ch);
    lowlvl = GET_LEVEL(ch);
    highpl = (ch->getEffectiveStat<int64_t>("health"));
    lowpl = (ch->getEffectiveStat<int64_t>("health"));

    found = 0;
    ch->followers.for_each([&](Character* f) {
        if(!AFF_FLAGGED(f, AFF_GROUP)) return;
        if (GET_LEVEL(f) > highlvl)
        {
            highlvl = GET_LEVEL(f);
        }
        if (GET_LEVEL(f) < lowlvl)
        {
            lowlvl = GET_LEVEL(f);
        }
    });

    int foundwas = 0;

    if (!strcasecmp(buf, "all"))
    {
        perform_group(ch, ch, GET_LEVEL(ch), GET_LEVEL(ch), highpl, lowpl);
        found = 0;
        ch->followers.for_each([&](Character* f) {
            foundwas = found;
            found += perform_group(ch, f, highlvl, lowlvl, highpl, lowpl);
            if (found > foundwas)
            {
                if (GET_LEVEL(f) > highlvl)
                    highlvl = GET_LEVEL(f);
                else if (GET_LEVEL(f) < lowlvl)
                    lowlvl = GET_LEVEL(f);
            }
        });
        if (!found)
            ch->sendText("Everyone following you is already in your group.\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM)))
        ch->send_to("%s", CONFIG_NOPERSON);
    else if ((vict->master != ch) && (vict != ch))
        act("$N must follow you to enter your group.", false, ch, nullptr, vict, TO_CHAR);
    else
    {
        if (!AFF_FLAGGED(vict, AFF_GROUP))
        {
            if (!AFF_FLAGGED(ch, AFF_GROUP))
            {
                ch->sendText("You form a group, with you as leader.\r\n");
                ch->affect_flags.set(AFF_GROUP, true);
            }
            perform_group(ch, vict, highlvl, lowlvl, highpl, lowpl);
        }
        else
        {
            if (ch != vict)
                act("$N is no longer a member of your group.", false, ch, nullptr, vict, TO_CHAR);
            act("You have been kicked out of $n's group!", false, ch, nullptr, vict, TO_VICT);
            act("$N has been kicked out of $n's group!", false, ch, nullptr, vict, TO_NOTVICT);
            vict->affect_flags.set(AFF_GROUP, false);
        }
    }
}

ACMD(do_ungroup)
{
    char buf[MAX_INPUT_LENGTH];
    struct follow_type *f, *next_fol;
    Character *tch;

    one_argument(argument, buf);

    if (!*buf)
    {
        if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP)))
        {
            ch->sendText("But you lead no group!\r\n");
            return;
        }

        ch->followers.for_each_safe([&](Character* f) {
            if(!AFF_FLAGGED(f, AFF_GROUP)) return;
            f->affect_flags.set(AFF_GROUP, false);
            act("$N has disbanded the group.", true, f, nullptr, ch, TO_CHAR);
            f->setBaseStat<int>("group_kills", 0);
            if (!AFF_FLAGGED(f, AFF_CHARM))
                stop_follower(f);
        });

        ch->affect_flags.set(AFF_GROUP, false);
        ch->setBaseStat<int>("group_kills", 0);
        ch->sendText("You disband the group.\r\n");
        return;
    }
    if (!(tch = get_char_vis(ch, buf, nullptr, FIND_CHAR_ROOM)))
    {
        ch->sendText("There is no such person!\r\n");
        return;
    }
    if (tch->master != ch)
    {
        ch->sendText("That person is not following you!\r\n");
        return;
    }

    if (!AFF_FLAGGED(tch, AFF_GROUP))
    {
        ch->sendText("That person isn't in your group.\r\n");
        return;
    }

    tch->affect_flags.set(AFF_GROUP, false);
    tch->setBaseStat<int>("group_kills", 0);

    act("$N is no longer a member of your group.", false, ch, nullptr, tch, TO_CHAR);
    act("You have been kicked out of $n's group!", false, ch, nullptr, tch, TO_VICT);
    act("$N has been kicked out of $n's group!", false, ch, nullptr, tch, TO_NOTVICT);

    if (!AFF_FLAGGED(tch, AFF_CHARM))
        stop_follower(tch);
}

ACMD(do_report)
{
    char buf[MAX_STRING_LENGTH];
    Character *k;
    struct follow_type *f;

    if (!AFF_FLAGGED(ch, AFF_GROUP))
    {
        ch->sendText("But you are not a member of any group!\r\n");
        return;
    }

    snprintf(buf, sizeof(buf), "$n reports: %" I64T "/%" I64T "H, %" I64T "/%" I64T "M, %" I64T "/%" I64T "V\r\n",
             GET_HIT(ch), GET_MAX_HIT(ch),
             (ch->getCurVital(CharVital::ki)), GET_MAX_MANA(ch),
             (ch->getCurVital(CharVital::stamina)), GET_MAX_MOVE(ch));

    k = (ch->master ? ch->master : ch);

    k->followers.for_each([&](auto f) {
        if (AFF_FLAGGED(f, AFF_GROUP) && f != ch)
            act(buf, true, ch, nullptr, f, TO_VICT);
    });

    if (k != ch)
        act(buf, true, ch, nullptr, k, TO_VICT);

    ch->sendText("You report to the group.\r\n");
}

ACMD(do_split)
{
    char buf[MAX_INPUT_LENGTH];
    int amount, num, share, rest;
    size_t len;
    Character *k;
    struct follow_type *f;

    if (IS_NPC(ch))
        return;

    one_argument(argument, buf);

    if (is_number(buf))
    {
        amount = atoi(buf);
        if (amount <= 0)
        {
            ch->sendText("Sorry, you can't do that.\r\n");
            return;
        }
        if (amount > GET_GOLD(ch))
        {
            ch->sendText("You don't seem to have that much gold to split.\r\n");
            return;
        }
        ch->modBaseStat("money_carried", -amount);
        k = (ch->master ? ch->master : ch);

        if (AFF_FLAGGED(k, AFF_GROUP) && (k->location == ch->location))
            num = 1;
        else
            num = 0;

        k->followers.for_each([&](auto f) {
            if (AFF_FLAGGED(f, AFF_GROUP) &&
                (!IS_NPC(f)) && f != ch &&
                (f->location == ch->location))
                num++;
        });

        if (num > 0 && AFF_FLAGGED(ch, AFF_GROUP))
        {
            share = amount / num;
            rest = amount % num;
        }
        else
        {
            ch->sendText("With whom do you wish to share your gold?\r\n");
            return;
        }

        ch->modBaseStat("money_carried", share);

        /* Abusing signed/unsigned to make sizeof work. */
        len = snprintf(buf, sizeof(buf), "%s splits %d zenni; you receive %d.\r\n",
                       GET_NAME(ch), amount, share);
        if (rest && len < sizeof(buf))
        {
            snprintf(buf + len, sizeof(buf) - len,
                     "%d zenni %s not splitable, so %s keeps the money.\r\n", rest, (rest == 1) ? "was" : "were",
                     GET_NAME(ch));
        }
        if (AFF_FLAGGED(k, AFF_GROUP) && k->location == ch->location &&
            !IS_NPC(k) && k != ch)
        {
            k->modBaseStat("money_carried", share);
            k->send_to("%s", buf);
        }

        k->followers.for_each([&](auto f) {
            if (AFF_FLAGGED(f, AFF_GROUP) &&
                (!IS_NPC(f)) &&
                (f->location == ch->location) &&
                f != ch)
            {
                f->modBaseStat("money_carried", share);
                f->send_to("%s", buf);
            }
        });
        ch->send_to("You split %d zenni among %d members -- %d zenni each.\r\n", amount, num, share);

        if (rest)
        {
            ch->send_to("%d zenni %s not splitable, so you keep the money.\r\n", rest, (rest == 1) ? "was" : "were");
            ch->modBaseStat("money_carried", rest);
        }
    }
    else
    {
        ch->sendText("How much zenni do you wish to split with your group?\r\n");
        return;
    }
}

ACMD(do_use)
{
    char buf[100], arg[MAX_INPUT_LENGTH];
    Object *mag_item = nullptr;

    half_chop(argument, arg, buf);
    if (!*arg)
    {
        ch->send_to("What do you want to %s?\r\n", CMD_NAME);
        return;
    }

    if (!mag_item)
    {
        switch (subcmd)
        {
        case SCMD_RECITE:
        case SCMD_QUAFF:
            if (!(mag_item = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
            {
                ch->send_to("You don't seem to have %s %s.\r\n", AN(arg), arg);
                return;
            }
            break;
        case SCMD_USE:
            if (!(mag_item = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
            {
                ch->send_to("You don't seem to have %s %s.\r\n", AN(arg), arg);
                return;
            }
            break;
        default:
            basic_mud_log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
            /*  SYSERR_DESC:
             *  This is the same as the unhandled case in do_gen_ps(), but in the
             *  function which handles 'quaff', 'recite', and 'use'.
             */
            return;
        }
    }
    int refreshed;
    switch (subcmd)
    {
    case SCMD_QUAFF:
        if (GET_OBJ_TYPE(mag_item) != ITEM_POTION)
        {
            ch->sendText("You can only swallow beans.\r\n");
            return;
        }
        if (IS_ANDROID(ch))
        {
            ch->sendText("You can't swallow beans, you are an android.\r\n");
            return;
        }
        if (OBJ_FLAGGED(mag_item, ITEM_FORGED))
        {
            ch->sendText("You can't swallow that, it is fake!\r\n");
            return;
        }
        if (OBJ_FLAGGED(mag_item, ITEM_BROKEN))
        {
            ch->sendText("You can't swallow that, it is broken!\r\n");
            return;
        }
        break;
    case SCMD_RECITE:
        if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL)
        {
            ch->sendText("You can only recite scrolls.\r\n");
            return;
        }
        break;
    case SCMD_USE:
        if (IS_ANDROID(ch))
        {
            ch->sendText("You are not biological enough to use these, Tincan.\r\n");
            return;
        }
        else
        {
            switch (GET_OBJ_VNUM(mag_item))
            {
            case 381:
                if ((ch->getCurVital(CharVital::stamina)) >= GET_MAX_MOVE(ch))
                {
                    ch->sendText("Your stamina is full.\r\n");
                    return;
                }
                act("@WYou place the $p@W against your chest and feel a rush of stamina as it automatically administers the dose.@n",
                    true, ch, mag_item, nullptr, TO_CHAR);
                act("@C$n@W places an $p@W against $s chest and a loud click is heard.@n", true, ch, mag_item,
                    nullptr, TO_ROOM);
                if (GET_SKILL(ch, SKILL_FIRST_AID) > 0)
                {
                    ch->sendText("@CYour skill in First Aid has helped increase the use of the injector. You gain more stamina as a result.@n\r\n");
                    ch->modCurVitalDam(CharVital::stamina, -0.25);
                }
                else
                {
                    ch->modCurVitalDam(CharVital::stamina, -0.1);
                }
                extract_obj(mag_item);
                return;
            case 382:
                if (AFF_FLAGGED(ch, AFF_BURNED))
                {
                    act("@WYou gently apply the salve to your burns.@n", true, ch, mag_item, nullptr, TO_CHAR);
                    act("@C$n@W gently applies a burn salve to $s burns.@n", true, ch, mag_item, nullptr,
                        TO_ROOM);
                    ch->affect_flags.set(AFF_BURNED, false);
                    extract_obj(mag_item);
                }
                else
                {
                    ch->sendText("You are not burned.\r\n");
                }
                return;
            case 383:
                if (AFF_FLAGGED(ch, AFF_POISON))
                {
                    act("@WYou place the $p@W against your neck and feel a rush of relief as the antitoxiin enters your bloodstream.@n",
                        true, ch, mag_item, nullptr, TO_CHAR);
                    act("@C$n@W places an $p@W against $s neck and a loud click is heard.@n", true, ch,
                        mag_item, nullptr, TO_ROOM);
                    null_affect(ch, AFF_POISON);
                    extract_obj(mag_item);
                }
                else
                {
                    ch->sendText("You are not poisoned.\r\n");
                }
                return;
            case 385:
                act("@WYou drink the contents of the vial before disposing of it.@n", true, ch, mag_item,
                    nullptr, TO_CHAR);
                act("@C$n@W dinks a $p and then disposes of it.@n", true, ch, mag_item, nullptr, TO_ROOM);
                if (AFF_FLAGGED(ch, AFF_BLIND))
                {
                    act("@WYour eyesight has returned!@n", true, ch, mag_item, nullptr, TO_CHAR);
                    act("@C$n@W eyesight seems to have returned.@n", true, ch, mag_item, nullptr, TO_ROOM);
                    null_affect(ch, AFF_BLIND);
                }
                refreshed = false;

                if (GET_HIT(ch) <= (ch->getEffectiveStat<int64_t>("health")) * 0.99)
                {
                    ch->modCurVital(CharVital::health, large_rand((ch->getEffectiveStat<int64_t>("health")) * 0.08, (ch->getEffectiveStat<int64_t>("health")) * 0.16));
                    refreshed = true;
                }
                else if ((ch->getCurVital(CharVital::ki)) <= (ch->getEffectiveStat<int64_t>("health")) * 0.99)
                {
                    ch->modCurVital(CharVital::ki, large_rand(GET_MAX_MANA(ch) * 0.08, GET_MAX_MANA(ch) * 0.16));
                    refreshed = true;
                }
                else if ((ch->getCurVital(CharVital::stamina)) <= GET_MAX_MOVE(ch) * 0.99)
                {
                    ch->modCurVital(CharVital::stamina, large_rand(GET_MAX_MOVE(ch) * 0.08, GET_MAX_MOVE(ch) * 0.16));
                    refreshed = true;
                }
                if (refreshed == true)
                {
                    ch->sendText("@CYou feel refreshed!\r\n");
                }
                extract_obj(mag_item);
                return;
            default:
                ch->sendText("That is not something you can apparently use.\r\n");
                return;
            }
        }
        break;
    }

    mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_value)
{
    char arg[MAX_INPUT_LENGTH];
    int value_lev;

    one_argument(argument, arg);

    if (!*arg)
    {
        switch (subcmd)
        {
        case SCMD_WIMPY:
            if (GET_WIMP_LEV(ch))
            {
                ch->send_to("Your current wimp level is %d powerlevel.\r\n", GET_WIMP_LEV(ch));
                return;
            }
            else
            {
                ch->sendText("At the moment, you're not a wimp.  (sure, sure...)\r\n");
                return;
            }
            break;
        }
    }

    if (isdigit(*arg))
    {
        switch (subcmd)
        {
        case SCMD_WIMPY:
            /* 'wimp_level' is a player_special. -gg 2/25/98 */
            if (IS_NPC(ch))
                return;
            if ((value_lev = atoi(arg)) != 0)
            {
                if (value_lev < 0)
                    ch->sendText("Heh, heh, heh.. we are jolly funny today, eh?\r\n");
                else if (value_lev > GET_MAX_HIT(ch))
                    ch->sendText("That doesn't make much sense, now does it?\r\n");
                else if (value_lev > (GET_MAX_HIT(ch) * 0.5))
                    ch->sendText("You can't set your wimp level above half your powerlevel.\r\n");
                else
                {
                    ch->send_to("Okay, you'll wimp out if you drop below %d powerlevel.\r\n", value_lev);
                    ch->setBaseStat("wimp_level", value_lev);
                }
            }
            else
            {
                ch->sendText("Okay, you'll now tough out fights to the bitter end.\r\n");
                ch->setBaseStat("wimp_level", 0);
            }
            break;
        default:
            basic_mud_log("Unknown subcmd to do_value %d called by %s", subcmd, GET_NAME(ch));
            break;
        }
    }
    else
        ch->sendText("Specify a value.  (0 to disable)\r\n");
}

ACMD(do_display)
{
    size_t i;

    if (IS_NPC(ch))
    {
        ch->sendText("Monsters don't need displays.  Go away.\r\n");
        return;
    }
    skip_spaces(&argument);

    if (!*argument)
    {
        ch->sendText("Usage: prompt { P | K | T | S | F | H | M | G | L | O | E | all/on | none/off | transforms}\r\n");
        return;
    }

    auto allPrefs = {PRF_DISPHP, PRF_DISPKI, PRF_DISPMOVE, PRF_DISPTNL, PRF_FURY, PRF_DISTIME, PRF_DISGOLD, PRF_DISPRAC,
                     PRF_DISHUTH, PRF_DISPERC, PRF_FORM, PRF_TECH};

    if (!strcasecmp(argument, "transforms"))
    {
        ch->pref_flags.toggle(PRF_FORM);
        ch->pref_flags.toggle(PRF_TECH);
        return;
    }

    if (!strcasecmp(argument, "on") || !strcasecmp(argument, "all"))
    {
        for (auto f : allPrefs)
            ch->pref_flags.set(f, true);
    }
    else if (!strcasecmp(argument, "off") || !strcasecmp(argument, "none"))
    {
        for (auto f : allPrefs)
            ch->pref_flags.set(f, false);
    }
    else
    {
        /*REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPTNL);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);*/

        for (i = 0; i < strlen(argument); i++)
        {
            switch (LOWER(argument[i]))
            {
            case 'p':
                ch->pref_flags.toggle(PRF_DISPHP);
                break;
            case 's':
                ch->pref_flags.toggle(PRF_DISPMOVE);
                break;
            case 'k':
                ch->pref_flags.toggle(PRF_DISPKI);
                break;
            case 't':
                ch->pref_flags.toggle(PRF_DISPTNL);
                break;
            case 'h':
                ch->pref_flags.toggle(PRF_DISTIME);
                break;
            case 'g':
                ch->pref_flags.toggle(PRF_DISGOLD);
                break;
            case 'l':
                ch->pref_flags.toggle(PRF_DISPRAC);
                break;
            case 'c':
                ch->pref_flags.toggle(PRF_DISPERC);
                break;
            case 'm':
                ch->pref_flags.toggle(PRF_DISHUTH);
                break;
            case 'o':
                ch->pref_flags.toggle(PRF_FORM);
                break;
            case 'e':
                ch->pref_flags.toggle(PRF_TECH);
                break;
            case 'f':
                if (!IS_HALFBREED(ch))
                {
                    ch->sendText("Only halfbreeds use fury.\r\n");
                }
                ch->pref_flags.toggle(PRF_FURY);
                break;
            default:
                ch->sendText("Usage: prompt { P | K | T | S | F | H | G | L | O | E | all/on | none/off | transforms}\r\n");
                return;
            }
        }
    }

    ch->send_to("%s", CONFIG_OK);
}

ACMD(do_gen_write)
{
    FILE *fl;
    char *tmp;
    const char *filename;
    struct stat fbuf;
    time_t ct;

    switch (subcmd)
    {
    case SCMD_BUG:
        filename = BUG_FILE;
        break;
    case SCMD_TYPO:
        filename = TYPO_FILE;
        break;
    case SCMD_IDEA:
        filename = IDEA_FILE;
        break;
    default:
        return;
    }

    ct = time(nullptr);
    tmp = asctime(localtime(&ct));

    if (IS_NPC(ch))
    {
        ch->sendText("Monsters can't have ideas - Go away.\r\n");
        return;
    }

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument)
    {
        ch->sendText("That must be a mistake...\r\n");
        return;
    }
    send_to_imm("[A new %s has been filed by: %s]\r\n", CMD_NAME, GET_NAME(ch));

    if (stat(filename, &fbuf) < 0)
    {
        perror("SYSERR: Can't stat() file");
        /*  SYSERR_DESC:
         *  This is from do_gen_write() and indicates that it cannot call the
         *  stat() system call on the file required.  The error string at the
         *  end of the line should explain what the problem is.
         */
        return;
    }
    if (fbuf.st_size >= CONFIG_MAX_FILESIZE)
    {
        ch->sendText("Sorry, the file is full right now.. try again later.\r\n");
        return;
    }
    if (!(fl = fopen(filename, "a")))
    {
        perror("SYSERR: do_gen_write");
        /*  SYSERR_DESC:
         *  This is from do_gen_write(), and will be output if the file in
         *  question cannot be opened for appending to.  The error string
         *  at the end of the line should explain what the problem is.
         */

        ch->sendText("Could not open the file.  Sorry.\r\n");
        return;
    }
    fprintf(fl,
            "@D[@WUser: @c%-10s@D] [@WChar: @C%-10s@D] [@WRoom: @G%-4d@D] [@WDate: @Y%6.6s@D]@b \n-----------@w\n%s\n",
            GET_USER(ch) ? GET_USER(ch) : "ERR", GET_NAME(ch), ch->location.getVnum(), (tmp + 4), argument);
    fprintf(fl, "@D-------------------------------@n\n");
    fclose(fl);
    ch->sendText("Okay.  Thanks!\r\n");
}

constexpr int TOG_OFF = 0;
constexpr int TOG_ON = 1;

ACMD(do_gen_tog)
{
    long result;

    const char *tog_messages[][2] = {
        {"You are now safe from summoning by other players.\r\n",
         "You may now be summoned by other players.\r\n"},
        {"Nohassle disabled.\r\n",
         "Nohassle enabled.\r\n"},
        {"Brief mode off.\r\n",
         "Brief mode on.\r\n"},
        {"Compact mode off.\r\n",
         "Compact mode on.\r\n"},
        {"You can now hear tells.\r\n",
         "You are now deaf to tells.\r\n"},
        {"You can now hear newbie.\r\n",
         "You are now deaf to newbie.\r\n"},
        {"You can now hear shouts.\r\n",
         "You are now deaf to shouts.\r\n"},
        {"You can now hear ooc.\r\n",
         "You are now deaf to ooc.\r\n"},
        {"You can now hear the congratulation messages.\r\n",
         "You are now deaf to the congratulation messages.\r\n"},
        {"You can now hear the Wiz-channel.\r\n",
         "You are now deaf to the Wiz-channel.\r\n"},
        {"You are no longer part of the Quest.\r\n",
         "Okay, you are part of the Quest!\r\n"},
        {"You will no longer see the room flags.\r\n",
         "You will now see the room flags.\r\n"},
        {"You will now have your communication repeated.\r\n",
         "You will no longer have your communication repeated.\r\n"},
        {"HolyLight mode off.\r\n",
         "HolyLight mode on.\r\n"},
        {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
         "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
        {"Autoexits disabled.\r\n",
         "Autoexits enabled.\r\n"},
        {"Will no longer track through doors.\r\n",
         "Will now track through doors.\r\n"},
        {"Buildwalk Off.\r\n",
         "Buildwalk On.\r\n"},
        {"AFK flag is now off.\r\n",
         "AFK flag is now on.\r\n"},
        {"You will no longer Auto-Assist.\r\n",
         "You will now Auto-Assist.\r\n"},
        {"Autoloot disabled.\r\n",
         "Autoloot enabled.\r\n"},
        {"Autogold disabled.\r\n",
         "Autogold enabled.\r\n"},
        {"Will no longer clear screen in OLC.\r\n",
         "Will now clear screen in OLC.\r\n"},
        {"Autosplit disabled.\r\n",
         "Autosplit enabled.\r\n"},
        {"Autosac disabled.\r\n",
         "Autosac enabled.\r\n"},
        {"You will no longer attempt to be sneaky.\r\n",
         "You will try to move as silently as you can.\r\n"},
        {"You will no longer attempt to stay hidden.\r\n",
         "You will try to stay hidden.\r\n"},
        {"You will no longer automatically memorize spells in your list.\r\n",
         "You will automatically memorize spells in your list.\r\n"},
        {"Viewing newest board messages at top of list.\r\n",
         "Viewing newest board messages at bottom of list.\r\n"},
        {"Compression will be used if your client supports it.\r\n",
         "Compression will not be used even if your client supports it.\r\n"},
        {"",
         ""},
        {"You are no longer hidden from view on the who list and public channels.\r\n",
         "You are now hidden from view on the who list and public channels.\r\n"},
        {"You will now be told that you have mail on prompt.\r\n",
         "You will no longer be told that you have mail on prompt.\r\n"},
        {"You will no longer receive automatic hints.\r\n",
         "You will now receive automatic hints.\r\n"},
        {"Screen Reader Friendly Mode Deactivated.\r\n",
         "Screen Reader Friendly Mode Activated..\r\n"},
        {"You will now see equipment when looking at someone.\r\n",
         "You will no longer see equipment when looking at someone.\r\n"},
        {"You will now listen to the music channel.\r\n",
         "You will no longer listen to the music channel.\r\n"},
        {"You will now parry attacks.\r\n",
         "You will no longer parry attacks.\r\n"},
        {"You will no longer keep cybernetic limbs with death.\r\n",
         "You will now keep cybernetic limbs with death.\r\n"},
        {"You will no longer worry about acquiring steaks from animals.\r\n",
         "You will now acquire steaks from animal if you can.\r\n"},
        {"You will now accept things being given to you.\r\n",
         "You will no longer accept things being given to you.\r\n"},
        {"You will no longer instruct those you spar with.\r\n",
         "You will now instruct those you spar with.\r\n"},
        {"You will no longer view group health.\r\n",
         "You will now view group health.\r\n"},
        {"You will no longer view item health.\r\n",
         "You will now view item health.\r\n"}};

    if (IS_NPC(ch))
        return;

    switch (subcmd)
    {
    case SCMD_NOSUMMON:
        result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
        break;
    case SCMD_NOHASSLE:
        result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
        break;
    case SCMD_BRIEF:
        result = PRF_TOG_CHK(ch, PRF_BRIEF);
        break;
    case SCMD_COMPACT:
        result = PRF_TOG_CHK(ch, PRF_COMPACT);
        break;
    case SCMD_NOTELL:
        result = PRF_TOG_CHK(ch, PRF_NOTELL);
        break;
    case SCMD_NOAUCTION:
        result = PRF_TOG_CHK(ch, PRF_NOAUCT);
        break;
    case SCMD_DEAF:
        result = PRF_TOG_CHK(ch, PRF_DEAF);
        break;
    case SCMD_NOGOSSIP:
        result = PRF_TOG_CHK(ch, PRF_NOGOSS);
        break;
    case SCMD_NOGRATZ:
        result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
        break;
    case SCMD_NOWIZ:
        result = PRF_TOG_CHK(ch, PRF_NOWIZ);
        break;
    case SCMD_QUEST:
        result = PRF_TOG_CHK(ch, PRF_QUEST);
        break;
    case SCMD_ROOMFLAGS:
        result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
        break;
    case SCMD_NOREPEAT:
        result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
        break;
    case SCMD_HOLYLIGHT:
        result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
        break;
    case SCMD_SLOWNS:
        result = (CONFIG_NS_IS_SLOW = !CONFIG_NS_IS_SLOW);
        break;
    case SCMD_AUTOEXIT:
        result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
        break;
    case SCMD_TRACK:
        result = (CONFIG_TRACK_T_DOORS = !CONFIG_TRACK_T_DOORS);
        break;
    case SCMD_AFK:
        result = PRF_TOG_CHK(ch, PRF_AFK);
        if (PRF_FLAGGED(ch, PRF_AFK))
            act("$n has gone AFK.", true, ch, nullptr, nullptr, TO_ROOM);
        else
        {
            act("$n has come back from AFK.", true, ch, nullptr, nullptr, TO_ROOM);
            if (has_mail(GET_IDNUM(ch)))
                ch->sendText("You have mail waiting.\r\n");
        }
        break;
    case SCMD_AUTOLOOT:
        result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
        break;
    case SCMD_AUTOGOLD:
        result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
        break;
    case SCMD_CLS:
        result = PRF_TOG_CHK(ch, PRF_CLS);
        break;
    case SCMD_BUILDWALK:
        if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT)
        {
            ch->sendText("Immortals only, sorry.\r\n");
            return;
        }
        result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
        if (PRF_FLAGGED(ch, PRF_BUILDWALK))
            mudlog(CMP, GET_LEVEL(ch), true,
                   "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
        else
            mudlog(CMP, GET_LEVEL(ch), true,
                   "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
        break;
    case SCMD_AUTOSPLIT:
        result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
        break;
    case SCMD_AUTOSAC:
        result = PRF_TOG_CHK(ch, PRF_AUTOSAC);
        break;
    case SCMD_SNEAK:
        result = AFF_TOG_CHK(ch, AFF_SNEAK);
        break;
    case SCMD_HIDE:
        if ((GET_CHARGE(ch) > 0 && GET_PREFERENCE(ch) != PREFERENCE_KI) ||
            (GET_CHARGE(ch) > GET_MAX_MANA(ch) * 0.1 && GET_PREFERENCE(ch) == PREFERENCE_KI) ||
            ch->character_flags.get(CharacterFlag::powering_up) || AFF_FLAGGED(ch, AFF_FLYING))
        {
            ch->sendText("You stand out too much to hide right now!\r\n");
            return;
        }
        else if (PLR_FLAGGED(ch, PLR_HEALT))
        {
            ch->sendText("You are inside a healing tank!\r\n");
            return;
        }
        if (!GET_SKILL(ch, SKILL_HIDE) && slot_count(ch) + 1 <= GET_SLOTS(ch))
        {
            ch->sendText("@GYou learn the very minimal basics to hiding.@n\r\n");
            SET_SKILL(ch, SKILL_HIDE, rand_number(1, 5));
        }
        else if (!GET_SKILL(ch, SKILL_HIDE) && slot_count(ch) + 1 > GET_SLOTS(ch))
        {
            ch->sendText("@RYou need more skill slots in order to learn this skill.@n\r\n");
            return;
        }
        result = AFF_TOG_CHK(ch, AFF_HIDE);
        break;
    case SCMD_AUTOMEM:
        result = PRF_TOG_CHK(ch, PRF_AUTOMEM);
        break;
    case SCMD_VIEWORDER:
        result = PRF_TOG_CHK(ch, PRF_VIEWORDER);
        break;
    case SCMD_TEST:
        if (GET_ADMLEVEL(ch) >= 1)
        {
            ch->pref_flags.toggle(PRF_TEST);
            ch->send_to("Okay. Testing is now: %s\r\n", PRF_FLAGGED(ch, PRF_TEST) ? "On" : "Off");
            if (PRF_FLAGGED(ch, PRF_TEST))
            {
                ch->sendText("Make sure to remove nohassle as well.\r\n");
            }
            return;
        }
        else
        {
            ch->sendText("You are not an immortal.\r\n");
            return;
        }
        break;
    case SCMD_NOCOMPRESS:
        if (CONFIG_ENABLE_COMPRESSION)
        {
            result = PRF_TOG_CHK(ch, PRF_NOCOMPRESS);
            break;
        }
        else
        {
            ch->sendText("Sorry, compression is globally disabled.\r\n");
        }
    case SCMD_AUTOASSIST:
        result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
        break;
    case SCMD_WHOHIDE:
        result = PRF_TOG_CHK(ch, PRF_HIDE);
        break;
    case SCMD_NMWARN:
        result = PRF_TOG_CHK(ch, PRF_NMWARN);
        break;
    case SCMD_HINTS:
        result = PRF_TOG_CHK(ch, PRF_HINTS);
        break;
    case SCMD_NODEC:
        result = PRF_TOG_CHK(ch, PRF_NODEC);
        break;
    case SCMD_NOEQSEE:
        result = PRF_TOG_CHK(ch, PRF_NOEQSEE);
        break;
    case SCMD_NOMUSIC:
        result = PRF_TOG_CHK(ch, PRF_NOMUSIC);
        break;
    case SCMD_NOPARRY:
        result = PRF_TOG_CHK(ch, PRF_NOPARRY);
        break;
    case SCMD_LKEEP:
        result = PRF_TOG_CHK(ch, PRF_LKEEP);
        break;
    case SCMD_CARVE:
        result = PRF_TOG_CHK(ch, PRF_CARVE);
        break;
    case SCMD_NOGIVE:
        result = PRF_TOG_CHK(ch, PRF_NOGIVE);
        break;
    case SCMD_INSTRUCT:
        result = PRF_TOG_CHK(ch, PRF_INSTRUCT);
        break;
    case SCMD_GHEALTH:
        result = PRF_TOG_CHK(ch, PRF_GHEALTH);
        break;
    case SCMD_IHEALTH:
        result = PRF_TOG_CHK(ch, PRF_IHEALTH);
        break;
    default:
        basic_mud_log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
        /*  SYSERR_DESC:
         *  This is the same as the unhandled case in do_gen_ps(), but in the
         *  function which handles 'compact', 'brief', and so forth.
         */
        return;
    }

    if (result)
        ch->send_to("%s", tog_messages[subcmd][TOG_ON]);
    else
        ch->send_to("%s", tog_messages[subcmd][TOG_OFF]);

    return;
}

ACMD(do_file)
{
    FILE *req_file;
    int cur_line = 0,
        num_lines = 0,
        req_lines = 0,
        i,
        j;
    int l;
    char field[100], value[100], line[READ_SIZE];
    char buf[MAX_STRING_LENGTH];

    struct file_struct
    {
        const char *cmd;
        char level;
        const char *file;
    } fields[] = {
        {"none", 6, "Does Nothing"},
        {"bug", ADMLVL_IMMORT, "../lib/misc/bugs"},
        {"typo", ADMLVL_IMMORT, "../lib/misc/typos"},
        {"report", ADMLVL_IMMORT, "../lib/misc/ideas"},
        {"xnames", 4, "../lib/misc/xnames"},
        {"levels", 4, "../log/levels"},
        {"rip", 4, "../log/rip"},
        {"players", 4, "../log/newplayers"},
        {"rentgone", 4, "../log/rentgone"},
        {"errors", 4, "../log/errors"},
        {"godcmds", 4, "../log/godcmds"},
        {"syslog", ADMLVL_IMMORT, "../syslog"},
        {"crash", ADMLVL_IMMORT, "../syslog.CRASH"},
        {"immlog", ADMLVL_IMMORT, "../lib/misc/request"},
        {"customs", ADMLVL_IMMORT, "../lib/misc/customs"},
        {"todo", 5, "../todo"},
        {"\n", 0, "\n"}};

    skip_spaces(&argument);

    if (!*argument)
    {
        strcpy(buf, "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n");
        for (j = 0, i = 1; fields[i].level; i++)
            if (fields[i].level <= GET_LEVEL(ch))
                sprintf(buf + strlen(buf), "%-15s%s\r\n", fields[i].cmd, fields[i].file);
        ch->sendText(buf);
        return;
    }

    two_arguments(argument, field, value);

    for (l = 0; *(fields[l].cmd) != '\n'; l++)
        if (!strncmp(field, fields[l].cmd, strlen(field)))
            break;

    if (*(fields[l].cmd) == '\n')
    {
        ch->sendText("That is not a valid option!\r\n");
        return;
    }

    if (GET_ADMLEVEL(ch) < fields[l].level)
    {
        ch->sendText("You are not godly enough to view that file!\r\n");
        return;
    }

    if (!strcasecmp(field, "request"))
    {
        GET_BOARD(ch, 2) = time(nullptr);
    }

    if (!*value)
        req_lines = 15; /* default is the last 15 lines */
    else
        req_lines = atoi(value);

    if (!(req_file = fopen(fields[l].file, "r")))
    {
        mudlog(BRF, ADMLVL_IMPL, true,
               "SYSERR: Error opening file %s using 'file' command.",
               fields[l].file);
        return;
    }

    get_line(req_file, line);
    while (!feof(req_file))
    {
        num_lines++;
        get_line(req_file, line);
    }
    rewind(req_file);

    req_lines = MIN(MIN(req_lines, num_lines), 5000);

    buf[0] = '\0';

    get_line(req_file, line);
    while (!feof(req_file))
    {
        cur_line++;
        if (cur_line > (num_lines - req_lines))
            sprintf(buf + strlen(buf), "%s\r\n", line);

        get_line(req_file, line);
    }
    fclose(req_file);
    ch->desc->send_to("%s", buf);
}

ACMD(do_compare)
{
    char arg1[100];
    char arg2[MAX_INPUT_LENGTH];
    Object *obj1, *obj2;
    Character *tchar;
    int value1 = 0, value2 = 0, o1, o2;
    const char *msg = nullptr;

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2)
    {
        ch->sendText("Compare what to what?\r\n");
        return;
    }

    o1 = generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tchar, &obj1);
    o2 = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tchar, &obj2);

    if (!o1 || !o2)
    {
        ch->sendText("You do not have that item.\r\n");
        return;
    }
    if (obj1 == obj2)
    {
        msg = "You compare $p to itself.  It looks about the same.";
    }
    else if (GET_OBJ_TYPE(obj1) != GET_OBJ_TYPE(obj2))
    {
        msg = "You can't compare $p and $P.";
    }
    else
    {
        switch (GET_OBJ_TYPE(obj1))
        {
        default:
            msg = "You can't compare $p and $P.";
            break;
        case ITEM_ARMOR:
            value1 = GET_OBJ_VAL(obj1, VAL_ARMOR_APPLYAC);
            value2 = GET_OBJ_VAL(obj2, VAL_ARMOR_APPLYAC);
            break;
        case ITEM_WEAPON:
            value1 = (1 + GET_OBJ_VAL(obj1, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(obj1, VAL_WEAPON_DAMDICE);
            value2 = (1 + GET_OBJ_VAL(obj2, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(obj2, VAL_WEAPON_DAMDICE);
            break;
        }
    }

    if (msg == nullptr)
    {
        if (value1 == value2)
            msg = "$p and $P look about the same.";
        else if (value1 > value2)
            msg = "$p looks better than $P.";
        else
            msg = "$p looks worse than $P.";
    }

    act(msg, false, ch, obj1, obj2, TO_CHAR);
    return;
}

ACMD(do_break)
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj;
    Character *dummy = nullptr;
    int cmbrk;

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Usually you break SOMETHING.\r\n");
        return;
    }

    if (!(cmbrk = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &dummy, &obj)))
    {
        ch->sendText("Can't seem to find what you want to break!\r\n");
        return;
    }

    if (OBJ_FLAGGED(obj, ITEM_BROKEN))
    {
        ch->sendText("Seems like it's already broken!\r\n");
        return;
    }

    /* Ok, break it! */
    ch->send_to("You ruin %s.\r\n", obj->getShortDescription());
    act("$n ruins $p.", false, ch, obj, nullptr, TO_ROOM);
    SET_OBJ_VAL(obj, VAL_ALL_HEALTH, 0);
    obj->item_flags.set(ITEM_BROKEN, true);

    return;
}

ACMD(do_fix)
{
    char arg[MAX_INPUT_LENGTH];
    Object *obj, *obj4 = nullptr, *rep, *next_obj;
    Character *dummy = nullptr;
    int cmbrk, found = false, self = false, custom = false;

    one_argument(argument, arg);

    if (!know_skill(ch, SKILL_REPAIR))
    {
        return;
    }

    if (!*arg)
    {
        ch->sendText("Usually you fix SOMETHING.\r\n");
        return;
    }

    if (!strcasecmp("self", arg))
    {
        if (!IS_ANDROID(ch))
        {
            ch->sendText("Only androids can fix their bodies with repair kits.\r\n");
            return;
        }
        else
        {
            self = true;
        }
    }

    if (self == false)
    {
        if (!(cmbrk = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &dummy, &obj)))
        {
            ch->sendText("Can't seem to find what you want to fix!\r\n");
            return;
        }

        if ((cmbrk) && GET_OBJ_VAL(obj, VAL_ALL_HEALTH) >= 100)
        {
            ch->sendText("But it isn't even damaged!\r\n");
            return;
        }

        if (OBJ_FLAGGED(obj, ITEM_FORGED))
        {
            ch->sendText("That is fake, why bother fixing it?\r\n");
            return;
        }

        switch (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL))
        {
        case MATERIAL_ORGANIC:
        case MATERIAL_FOOD:
        case MATERIAL_PAPER:
        case MATERIAL_LIQUID:
            ch->sendText("You can't repair that.\r\n");
            return;
            break;
        }

        if (GET_OBJ_VNUM(obj) == 20099 || GET_OBJ_VNUM(obj) == 20098)
        {
            custom = true;
        }
    }

    obj4 = ch->searchInventory(custom ? 13593 : 48);

    if (!obj4)
    {
        if (custom)
        {
            ch->sendText("You do not even have a Nano-tech Repair Orb.\r\n");
            return;
        }
        else
        {
            ch->sendText("You do not even have a repair kit.\r\n");
            return;
        }
    }

    if (self == false)
    {
        if (GET_SKILL(ch, SKILL_REPAIR) < axion_dice(0))
        {
            act("You try to repair $p but screw up..", true, ch, obj, nullptr, TO_CHAR);
            act("$n tries to repair $p but screws up..", true, ch, obj, nullptr, TO_ROOM);
            extract_obj(obj4);
            improve_skill(ch, SKILL_REPAIR, 1);
            WAIT_STATE(ch, PULSE_2SEC);
            return;
        }

        if (GET_OBJ_VAL(obj, VAL_ALL_HEALTH) + GET_SKILL(ch, SKILL_REPAIR) < 100)
        {
            ch->send_to("You repair %s a bit.\r\n", obj->getShortDescription());
            act("$n repairs $p a bit.", false, ch, obj, nullptr, TO_ROOM);
            MOD_OBJ_VAL(obj, VAL_ALL_HEALTH, GET_SKILL(ch, SKILL_REPAIR));
            obj->item_flags.set(ITEM_BROKEN, false);
        }
        else
        {
            ch->send_to("You repair %s completely.\r\n", obj->getShortDescription());
            act("$n repairs $p completely.", false, ch, obj, nullptr, TO_ROOM);
            SET_OBJ_VAL(obj, VAL_ALL_HEALTH, 100);
            obj->item_flags.set(ITEM_BROKEN, false);
        }
        if (!obj->getCarriedBy() && !PLR_FLAGGED(ch, PLR_REPLEARN) &&
            (level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0 || GET_LEVEL(ch) >= 100))
        {
            int64_t gain = (level_exp(ch, GET_LEVEL(ch) + 1) * 0.0003) * GET_SKILL(ch, SKILL_REPAIR);
            ch->send_to("@mYou've learned a bit from repairing it. @D[@gEXP@W: @G+%s@D]@n\r\n", add_commas(gain).c_str());
            ch->player_flags.set(PLR_REPLEARN, true);
            ch->modExperience(gain);
        }
        else if (rand_number(2, 12) >= 10 && PLR_FLAGGED(ch, PLR_REPLEARN))
        {
            ch->player_flags.set(PLR_REPLEARN, false);
            ch->sendText("@mYou think you might be on to something...@n\r\n");
        }
        improve_skill(ch, SKILL_REPAIR, 1);
        extract_obj(obj4);
        WAIT_STATE(ch, PULSE_2SEC);
        return;
    }
    else
    { /* For androids repairing themselves */

        if (GET_HIT(ch) >= (ch->getEffectiveStat<int64_t>("health")))
        {
            ch->sendText("Your body is already in peak condition.\r\n");
            return;
        }
        else if (GET_SKILL(ch, SKILL_REPAIR) < axion_dice(0))
        {
            act("You try to repair your body but screw up..", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n tries to repair $s body but screws up..", true, ch, nullptr, nullptr, TO_ROOM);
            extract_obj(obj4);
            improve_skill(ch, SKILL_REPAIR, 1);
            WAIT_STATE(ch, PULSE_5SEC);
            return;
        }
        else
        {
            act("You use the repair kit to fix part of your body...", true, ch, nullptr, nullptr, TO_CHAR);
            act("$n works on their body with a repair kit.", true, ch, nullptr, nullptr, TO_ROOM);
            int64_t mult = GET_SKILL(ch, SKILL_REPAIR);
            int64_t add = (((ch->getEffectiveStat<int64_t>("health")) * 0.005) + 10) * mult;

            extract_obj(obj4);
            if (ch->modCurVital(CharVital::health, add) == ch->getEffectiveStat<int64_t>("health"))
            {
                ch->sendText("Your body has been totally repaired.\r\n");
                WAIT_STATE(ch, PULSE_5SEC);
            }
            else
            {
                ch->sendText("Your body still needs some work done to it.\r\n");
                WAIT_STATE(ch, PULSE_5SEC);
            }
        }
    }
}

static int spell_in_book(Object *obj, int spellnum)
{

    return 0;
}

static int spell_in_scroll(Object *obj, int spellnum)
{
    if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) == spellnum)
        return true;

    return false;
}

static int spell_in_domain(Character *ch, int spellnum)
{
    if (spell_info[spellnum].domain == DOMAIN_UNDEFINED)
    {
        return false;
    }

    return true;
}

const room_vnum freeres[NUM_ALIGNS] = {
    /* LAWFUL_GOOD */ 1000,
    /* NEUTRAL_GOOD */ 1000,
    /* CHAOTIC_GOOD */ 1000,
    /* LAWFUL_NEUTRAL */ 1000,
    /* NEUTRAL_NEUTRAL */ 1000,
    /* CHAOTIC_NEUTRAL */ 1000,
    /* LAWFUL_EVIL */ 1000,
    /* NEUTRAL_EVIL */ 1000,
    /* CHAOTIC_EVIL */ 1000};

ACMD(do_resurrect)
{
    room_rnum rm;
    struct affected_type *af, *next_af;

    if (IS_NPC(ch))
    {
        ch->sendText("Sorry, only players get spirits.\r\n");
        return;
    }

    if (!AFF_FLAGGED(ch, AFF_SPIRIT))
    {
        ch->sendText("But you're not even dead!\r\n");
        return;
    }

    ch->sendText("You take an experience penalty and pray for charity resurrection.\r\n");
    int64_t gain = -(level_exp(ch, GET_LEVEL(ch)) - level_exp(ch, GET_LEVEL(ch) - 1));
    ch->modExperience(gain);

    for (af = ch->affected; af; af = next_af)
    {
        next_af = af->next;
        if (af->location == APPLY_NONE && af->type == -1 &&
            (af->bitvector == AFF_SPIRIT || af->bitvector == AFF_ETHEREAL))
            affect_remove(ch, af);
    }

    if ((rm = real_room(freeres[ALIGN_TYPE(ch)])) == NOWHERE)
        rm = real_room(CONFIG_MORTAL_START);

    if (rm != NOWHERE)
    {
        ch->leaveLocation();
        ch->moveToLocation(rm);
        ch->lookAtLocation();
    }

    act("$n's body forms in a pool of @Bblue light@n.", true, ch, nullptr, nullptr, TO_ROOM);
}

static void show_clan_info(Character *ch)
{

    ch->sendText("@c----------------------------------------\r\n"
                 "@c|@WProvided by@D: @YAlister of Aeonian Dreams@c|\r\n"
                 "@c|@YWith many changes made by Iovan.      @c|\r\n"
                 "@c----------------------------------------@w\r\n"
                 "  Commands are:\r\n"
                 "@c--@YClan Members Only@c-----@w\r\n"
                 "  clan members\r\n"
                 "  clan bank\r\n"
                 "  clan deposit\r\n"
                 "  clan leave    <clan>\r\n"
                 "@c--@YClan Mod/Highrank@c-----@w\r\n"
                 "  clan decline  <person> <clan>\r\n"
                 "  clan enroll   <person> <clan>\r\n"
                 "@c--@YClan Moderators Only@c--@w\r\n"
                 "  clan withdraw\r\n"
                 "  clan infow\r\n"
                 "  clan setjoin  <free | restricted> <clan>\r\n"
                 "  clan setleave <free | restricted> <clan>\r\n"
                 "  clan expel    <person> <clan>\r\n"
                 "  clan highrank <new highrank title>\r\n"
                 "  clan midrank  <new midrank title>\r\n"
                 "  clan rank     <person> < 0 / 1 / or 2>\r\n"
                 "  clan makemod  <person> <clan>\r\n"
                 "@c--@YAnyone@c----------------@w\r\n"
                 "  clan list\r\n"
                 "  clan info     <clan>\r\n"
                 "  clan apply    <clan>\r\n"
                 "  clan join     <clan>\r\n");
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMPL)
        ch->sendText("@c--@YImmort@c----------------@w\r\n"
                     "  clan create   <clan>\r\n"
                     "  clan destroy  <clan>\r\n"
                     "  clan reload   <clan>\r\n"
                     "  clan bset     <clan>\n");
}

ACMD(do_clan)
{

    char arg1[100];
    char arg2[MAX_INPUT_LENGTH];

    if (ch == nullptr || IS_NPC(ch))
        return;

    half_chop(argument, arg1, arg2);

    if (!*arg1)
    {
        show_clan_info(ch);
        return;
    }

    // immortal-only commands
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMPL)
    {

        if (!(strcmp(arg1, "create")))
        {
            if (!*arg2)
                show_clan_info(ch);
            else if (isClan(arg2))
                ch->send_to("There is already a clan with the name, %s.\r\n", arg2);
            else
            {
                ch->send_to("You create a clan with the name, %s.\r\n", arg2);
                clanCreate(arg2);
                mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s has created a clan named %s.",
                       GET_NAME(ch), arg2);
            }
            return;
        }
        else if (!(strcmp(arg1, "destroy")))
        {
            if (!*arg2)
                show_clan_info(ch);
            else if (!isClan(arg2))
                ch->send_to("No clan with the name %s exists.\r\n", arg2);
            else
            {
                ch->send_to("You destroy %s.\r\n", arg2);
                clanDestroy(arg2);
            }
            return;
        }
        else if (!(strcmp(arg1, "bset")))
        {
            if (!*arg2)
                show_clan_info(ch);
            else if (!isClan(arg2))
                ch->send_to("No clan with the name %s exists.\r\n", arg2);
            else if (GET_ADMLEVEL(ch) < 5)
                show_clan_info(ch);
            else
            {
                clanBSET(arg2, ch);
            }
            return;
        }
        else if (!(strcmp(arg1, "reload")))
        {
            if (!*arg2)
                show_clan_info(ch);
            else if (!isClan(arg2))
                ch->send_to("No clan with the name %s exists.\r\n", arg2);
            else if (clanReload(arg2))
                ch->send_to("Data for %s has been reloaded.\r\n", arg2);
            else
                ch->send_to("Failed to reload the data for %s.\r\n", arg2);
            return;
        }
    }

    // commands available to everybody
    if (!(strcmp(arg1, "apply")))
    {
        if (!*arg2)
            show_clan_info(ch);
        else if (!isClan(arg2))
            ch->send_to("%s is not a valid clan.\r\n", arg2);
        else if (GET_CLAN(ch) && clanIsMember(GET_CLAN(ch), ch))
            ch->send_to("You are already a member of %s.\r\n", GET_CLAN(ch));
        else if (clanOpenJoin(arg2))
        {
            ch->send_to("You can just join %s, it is open.\r\n", arg2);
            return;
        }
        else
        {
            if (GET_CLAN(ch) && checkCLAN(ch) == true)
            {
                checkAPP(ch);
                ch->send_to("You stop applying to %s\r\n", GET_CLAN(ch));
                clanDecline(GET_CLAN(ch), ch);
                if (GET_CLAN(ch))
                {
                    free(GET_CLAN(ch));
                }
                GET_CLAN(ch) = strdup("None.");
            }
            ch->send_to("You apply to become a member of %s.\r\n", arg2);
            clanApply(arg2, ch);
            return;
        }
        return;
    }
    if (!(strcmp(arg1, "join")))
    {
        if (!*arg2)
            show_clan_info(ch);
        else if (!isClan(arg2))
            ch->send_to("%s is not a valid clan.\r\n", arg2);
        else if (clanIsMember(arg2, ch))
            ch->send_to("You are already a member of %s.\r\n", arg2);
        else if (GET_CLAN(ch) && checkCLAN(ch) && !strstr(GET_CLAN(ch), "Applying"))
            ch->send_to("You are already a member of %s, you need to leave it first.\r\n", GET_CLAN(ch));
        else if (clanOpenJoin(arg2))
        {
            if (GET_CLAN(ch) && checkCLAN(ch))
            {
                checkAPP(ch);
                ch->send_to("You stop applying to %s\r\n", GET_CLAN(ch));
                clanDecline(GET_CLAN(ch), ch);
                if (GET_CLAN(ch))
                {
                    free(GET_CLAN(ch));
                }
                GET_CLAN(ch) = strdup("None.");
            }
            ch->send_to("You are now a member of %s.\r\n", arg2);
            clanInduct(arg2, ch);
            return;
        }
        else
        {
            ch->send_to("%s isn't open, you must apply instead.\r\n", arg2);
            return;
        }
        return;
    }
    else if (!(strcmp(arg1, "leave")))
    {
        if (!*arg2)
            show_clan_info(ch);
        else if (!isClan(arg2))
            ch->send_to("%s is not a valid clan.\r\n", arg2);
        else if (!clanIsMember(arg2, ch))
            ch->send_to("You aren't even a member of %s.\r\n", arg2);
        else if (!(clanOpenLeave(arg2) || clanIsModerator(arg2, ch)))
            ch->send_to("You must be expelled from %s in order to leave it.\r\n", arg2);
        else
        {
            ch->send_to("You are no longer a member of %s.\r\n", arg2);
            clanExpel(arg2, ch);
            return;
        }
        return;
    }
    else if (!(strcmp(arg1, "infow")))
    {
        if (GET_CLAN(ch) == nullptr || !(strcmp(GET_CLAN(ch), "None.")))
        {
            ch->sendText("You are not in a clan.\r\n");
            return;
        }
        else
        {
            if (clanIsModerator(GET_CLAN(ch), ch) == false)
                ch->sendText("You must be a moderator to edit the clan's information.\r\n");
            else
            {
                clanINFOW(GET_CLAN(ch), ch);
                ch->player_flags.set(PLR_WRITING, true);
            }
            return;
        }
    }
    else if (!(strcmp(arg1, "deposit")))
    {
        long bank = 0;

        if (GET_CLAN(ch) == nullptr || !(strcmp(GET_CLAN(ch), "None.")))
        {
            ch->sendText("You are not in a clan.\r\n");
            return;
        }
        else
        {
            if (!clanIsMember(GET_CLAN(ch), ch) && !clanIsModerator(GET_CLAN(ch), ch))
            {
                ch->sendText("You are not in a clan.\r\n");
                return;
            }
            else if (!ch->location.getRoomFlag(ROOM_CBANK) && clanBANY(GET_CLAN(ch), ch) == false)
            {
                ch->sendText("You are not in your clan bank and your clan doesn't have bank anywhere.\r\n");
                return;
            }
            else if (!*arg2)
            {
                ch->sendText("How much do you want to deposit?\r\n");
                return;
            }
            else if (atoi(arg2) <= 0)
            {
                ch->sendText("It needs to be a value higher than 0...\r\n");
                return;
            }
            else if (GET_GOLD(ch) < atoi(arg2))
            {
                ch->sendText("You do not have that much to deposit!\r\n");
                return;
            }
            else
            {
                bank = atoi(arg2);
                ch->modBaseStat("money_carried", -bank);
                clanBANKADD(GET_CLAN(ch), ch, bank);
                ch->send_to("You have deposited %s into the clan bank.\r\n", add_commas(bank).c_str());
            }
        }
        return;
    }
    else if (!(strcmp(arg1, "highrank")))
    {

        if (GET_CLAN(ch) == nullptr || !(strcmp(GET_CLAN(ch), "None.")))
        {
            ch->sendText("You are not in a clan.\r\n");
            return;
        }
        else
        {
            if (!clanIsModerator(GET_CLAN(ch), ch))
            {
                ch->sendText("You are not leading a clan.\r\n");
                return;
            }
            else if (!*arg2)
            {
                ch->sendText("What name are you going to make the rank?\r\n");
                return;
            }
            else if (strlen(arg2) > 20)
            {
                ch->sendText("The name length can't be longer than 20 characters.\r\n");
                return;
            }
            else if (strstr(arg2, "@"))
            {
                ch->sendText("No colorcode allowed in the ranks.\r\n");
                return;
            }
            else
            {
                clanHIGHRANK(GET_CLAN(ch), ch, arg2);
                ch->sendText("High rank set.\r\n");
            }
            return;
        }
    }
    else if (!(strcmp(arg1, "midrank")))
    {

        if (GET_CLAN(ch) == nullptr || !(strcmp(GET_CLAN(ch), "None.")))
        {
            ch->sendText("You are not in a clan.\r\n");
            return;
        }
        else
        {
            if (!clanIsModerator(GET_CLAN(ch), ch))
            {
                ch->sendText("You are not leading a clan.\r\n");
                return;
            }
            else if (!*arg2)
            {
                ch->sendText("What name are you going to make the rank?\r\n");
                return;
            }
            else if (strlen(arg2) > 20)
            {
                ch->sendText("The name length can't be longer than 20 characters.\r\n");
                return;
            }
            else if (strstr(arg2, "@"))
            {
                ch->sendText("No colorcode allowed in the ranks.\r\n");
                return;
            }
            else
            {
                clanMIDRANK(GET_CLAN(ch), ch, arg2);
                ch->sendText("Mid rank set.\r\n");
            }
            return;
        }
    }
    else if (!(strcmp(arg1, "rank")))
    {
        Character *vict = nullptr;
        char arg3[100];
        char name[100];
        half_chop(arg2, name, arg3);

        if (GET_CLAN(ch) == nullptr || !(strcmp(GET_CLAN(ch), "None.")))
        {
            ch->sendText("You are not in a clan.\r\n");
            return;
        }
        else
        {
            if (!clanIsModerator(GET_CLAN(ch), ch))
            {
                ch->sendText("You are not leading a clan.\r\n");
                return;
            }
            else if (!*arg2)
            {
                ch->sendText("Who's rank do you want to change?\r\n");
                return;
            }
            else if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_WORLD)))
            {
                ch->sendText("That person is no where to be found in the entire universe.\r\n");
                return;
            }
            else if (GET_CLAN(vict) == nullptr || !(strcmp(GET_CLAN(vict), "None.")))
            {
                ch->sendText("That person is not even in a clan, let alone your's.\r\n");
                return;
            }
            else if (!clanIsMember(GET_CLAN(ch), vict))
            {
                ch->sendText("You can only rank those in your clan and only if below leader.\r\n");
                return;
            }
            else if (clanIsModerator(GET_CLAN(ch), vict))
            {
                ch->sendText("You can't rank a fellow leader, you require imm assistance.\r\n");
                return;
            }
            else if (!*arg3)
            {
                ch->sendText("What rank do you want to set them to?\r\n[ 0 = Member, 1 = Midrank, 2 = Highrank]\r\n");
                return;
            }
            int num = atoi(arg3);
            if (num < 0 || num > 2)
            {
                ch->sendText("It must be above zero and lower than three...\r\n");
                return;
            }
            else if (GET_CRANK(vict) == num)
            {
                ch->sendText("They are already that rank!\r\n");
                return;
            }
            else if (GET_CRANK(vict) > num)
            {
                clanRANK(GET_CLAN(ch), ch, vict, num);
                switch (num)
                {
                case 0:
                    ch->send_to("You demote %s.\r\n", GET_NAME(vict));
                    vict->send_to("%s has demoted your clan rank to member!\r\n", GET_NAME(ch));
                    break;
                case 1:
                    ch->send_to("You demote %s.\r\n", GET_NAME(vict));
                    vict->send_to("%s has demoted your clan rank to midrank!\r\n", GET_NAME(ch));
                    break;
                }
                return;
            }
            else if (GET_CRANK(vict) < num)
            {
                clanRANK(GET_CLAN(ch), ch, vict, num);
                switch (num)
                {
                case 1:
                    ch->send_to("You promote %s.\r\n", GET_NAME(vict));
                    vict->send_to("%s has promoted your clan rank to midrank!\r\n", GET_NAME(ch));
                    break;
                case 2:
                    ch->send_to("You promote %s.\r\n", GET_NAME(vict));
                    vict->send_to("%s has promoted your clan rank to highrank!\r\n", GET_NAME(ch));
                    break;
                }
                return;
            }
        }
    }
    else if (!(strcmp(arg1, "withdraw")))
    {
        long bank = 0;

        if (GET_CLAN(ch) == nullptr || !(strcmp(GET_CLAN(ch), "None.")))
        {
            ch->sendText("You are not in a clan.\r\n");
            return;
        }
        else
        {
            if (!clanIsMember(GET_CLAN(ch), ch) && !clanIsModerator(GET_CLAN(ch), ch))
            {
                ch->sendText("You are not in a clan.\r\n");
                return;
            }
            else if (!*arg2)
            {
                ch->sendText("How much do you want to withdraw?\r\n");
                return;
            }
            else if (!(clanIsModerator(GET_CLAN(ch), ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL))
            {
                ch->sendText("You do not have the power to withdraw from the clan bank.\r\n");
                return;
            }
            else if (atoi(arg2) <= 0)
            {
                ch->sendText("It needs to be a value higher than 0...\r\n");
                return;
            }
            else if (GET_GOLD(ch) + atoi(arg2) > GOLD_CARRY(ch))
            {
                ch->sendText("You can not hold that much zenni!\r\n");
                return;
            }
            else
            {
                bank = atoi(arg2);
                if (clanBANKSUB(GET_CLAN(ch), ch, bank))
                {
                    ch->send_to("You have withdrawn %s from the clan bank.\r\n", add_commas(bank).c_str());
                    ch->modBaseStat("money_carried", bank);
                }
                else
                {
                    ch->sendText("There isn't that much in the clan's bank!\r\n");
                }
            }
        }
        return;
    }
    else if (!(strcmp(arg1, "bank")))
    {
        long bank = 0;
        if (GET_CLAN(ch) == nullptr || !(strcmp(GET_CLAN(ch), "None.")))
        {
            ch->sendText("You are not in a clan.\r\n");
            return;
        }
        else
        {
            if (!clanIsMember(GET_CLAN(ch), ch) && !clanIsModerator(GET_CLAN(ch), ch))
            {
                ch->sendText("You are not in a clan.\r\n");
                return;
            }
            bank = clanBANK(GET_CLAN(ch), ch);
            ch->send_to("@W[ @C%-20s @W]@w has @D(@Y%s@D)@w zenni in its clan bank.\r\n", GET_CLAN(ch), add_commas(bank).c_str());
        }
        return;
    }
    else if (!(strcmp(arg1, "members")))
    {
        if (GET_CLAN(ch) == nullptr || !(strcmp(GET_CLAN(ch), "None.")))
        {
            ch->sendText("You are not even in a clan.\r\n");
            return;
        }
        else
        {
            handle_clan_member_list(ch);
        }
    }
    else if (!(strcmp(arg1, "expel")))
    {
        Character *vict;
        char arg3[100];
        char name[MAX_INPUT_LENGTH];
        char name1[100];
        half_chop(arg2, name1, arg3);

        if (!*arg3 || !*name1)
        {
            show_clan_info(ch);
        }
        else if (!isClan(arg3))
        {
            ch->send_to("%s is not a valid clan.\r\n", arg3);
        }
        else if (!(clanIsModerator(arg3, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL))
        {
            ch->sendText("Only leaders can expel people from a clan.\r\n");
        }
        else if (clanOpenJoin(arg3))
        {
            ch->sendText("You can't kick someone out of an open-join clan.\r\n");
        }
        else if (!(vict = get_char_vis(ch, name1, nullptr, FIND_CHAR_WORLD)))
        {
            vict = findPlayer(name);
            sprintf(name, "%s", rIntro(ch, name1));
            if (!vict)
                vict = findPlayer(name1);

            if (vict)
            {
                if (!clanIsMember(arg3, vict))
                {
                    ch->send_to("%s isn't even a member of %s.\r\n", GET_NAME(vict), arg3);
                }
                else if (clanIsModerator(arg3, vict) && GET_ADMLEVEL(ch) < ADMLVL_IMPL)
                {
                    ch->send_to("You do not have the power to kick a leader out of %s.\r\n", arg3);
                }
                else
                {
                    ch->send_to("You expel %s from %s.\r\n", GET_NAME(vict), arg3);
                    clanExpel(arg3, vict);
                }
            }
            else
            {
                ch->send_to("%s does not seem to exist.\r\n", name1);
                return;
            }
            return;
        }
        else if (!clanIsMember(arg3, vict))
        {
            ch->send_to("%s isn't even a member of %s.\r\n", GET_NAME(vict), arg3);
        }
        else if (clanIsModerator(arg3, vict) && GET_ADMLEVEL(ch) < ADMLVL_IMPL)
        {
            ch->send_to("You do not have the power to kick a leader out of %s.\r\n", arg3);
        }
        else
        {
            ch->send_to("You expel %s from %s.\r\n", GET_NAME(vict), arg3);
            vict->send_to("You have been expelled from %s.\r\n", arg3);
            clanExpel(arg3, vict);
            return;
        }
        return;
    }
    else if (!(strcmp(arg1, "decline")))
    {
        Character *vict;
        char arg3[100];
        char name[100];
        half_chop(arg2, name, arg3);

        if (!*arg3 || !*name)
            show_clan_info(ch);
        else if (!isClan(arg3))
            ch->send_to("%s is not a valid clan.\r\n", arg3);
        else if ((clanIsModerator(arg3, ch) == false && clanIsMember(arg3, ch) == false &&
                  GET_ADMLEVEL(ch) < ADMLVL_IMPL) ||
                 (GET_CRANK(ch) < 2 && GET_ADMLEVEL(ch) < ADMLVL_IMPL))
            ch->sendText("Only leaders or highrank can decline people from entering a clan.\r\n");
        else if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_WORLD)))
            ch->send_to("%s is not around at the moment.\r\n", name);
        else if (!clanIsApplicant(arg3, vict))
            ch->send_to("%s isn't applying to join %s.\r\n", GET_NAME(vict), arg3);
        else
        {
            ch->send_to("You decline %s enterance to %s.\r\n", GET_NAME(vict), arg3);
            vict->send_to("You have been declined enterance to %s.\r\n", arg3);
            clanDecline(arg3, vict);
        }
        return;
    }
    else if (!(strcmp(arg1, "enroll")))
    {
        Character *vict;
        char arg3[100];
        char name[100];
        half_chop(arg2, name, arg3);

        if (!*arg3 || !*name)
            show_clan_info(ch);
        else if (!isClan(arg3))
            ch->send_to("%s is not a valid clan.\r\n", arg3);
        else if (!clanIsMember(arg3, ch) && GET_ADMLEVEL(ch) < 1)
            ch->sendText("You are not in that clan.\r\n");
        else if (!(clanIsModerator(arg3, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL) && GET_CRANK(ch) < 2)
            ch->sendText("Only leaders or captains can enroll people into their clan.\r\n");
        else if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_WORLD)))
            ch->send_to("%s is not around at the moment.\r\n", name);
        else if (!clanIsApplicant(arg3, vict))
            ch->send_to("%s isn't applying to join %s.\r\n", GET_NAME(vict), arg3);
        else
        {
            ch->send_to("You enroll %s into %s.\r\n", GET_NAME(vict), arg3);
            vict->send_to("You have been enrolled into %s.\r\n", arg3);
            clanInduct(arg3, vict);
        }
        return;
    }
    else if (!(strcmp(arg1, "makemod")))
    {
        Character *vict;
        char arg3[100];
        char name[100];
        half_chop(arg2, name, arg3);

        if (!*arg3 || !*name)
            show_clan_info(ch);
        else if (!isClan(arg3))
            ch->send_to("%s is not a valid clan.\r\n", arg3);
        else if (!(clanIsModerator(arg3, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL))
            ch->sendText("Only leaders can make other people in a clan a leader.\r\n");
        else if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_WORLD)))
            ch->send_to("%s is not around at the moment.\r\n", name);
        else
        {
            ch->send_to("You make %s a leader of %s.\r\n", GET_NAME(vict), arg3);
            vict->send_to("You have been made a leader of %s.\r\n", arg3);
            clanMakeModerator(arg3, vict);
        }
        return;
    }
    else if (!(strcmp(arg1, "setleave")))
    {
        char name[100];
        char setting[100];
        half_chop(arg2, setting, name);

        if (!*name || !*setting)
            show_clan_info(ch);
        else if (!isClan(name))
            ch->send_to("%s is not a valid clan.\r\n", name);
        else if (!(clanIsModerator(name, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL))
            ch->sendText("Only leaders can change that.\r\n");
        else if (!strcmp(setting, "free"))
        {
            ch->send_to("Members of %s are free to leave as they please.\r\n", name);
            clanSetOpenLeave(name, true);
        }
        else if (!strcmp(setting, "restricted"))
        {
            ch->send_to("Members of %s can no longer leave as they please.\r\n", name);
            clanSetOpenLeave(name, false);
        }
        else
            ch->sendText("Leave access may only be set to free or restricted.\r\n");
        return;
    }
    else if (!(strcmp(arg1, "setjoin")))
    {
        char name[100];
        char setting[100];
        half_chop(arg2, setting, name);

        if (!*name || !*setting)
            show_clan_info(ch);
        else if (!isClan(name))
            ch->send_to("%s is not a valid clan.\r\n", name);
        else if (!(clanIsModerator(name, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL))
            ch->sendText("Only leaders can change that.\r\n");
        else if (!strcmp(setting, "free"))
        {
            ch->send_to("People may now freely join %s.\r\n", name);
            clanSetOpenJoin(name, true);
        }
        else if (!strcmp(setting, "restricted"))
        {
            ch->send_to("People must be enrolled into %s to join.\r\n", name);
            clanSetOpenJoin(name, false);
        }
        else
            ch->sendText("Leave access my only be set to free or restricted.\r\n");
        return;
    }
    else if (!(strcmp(arg1, "list")))
        listClans(ch);
    else if (!(strcmp(arg1, "info")))
    {
        if (!*arg2)
            show_clan_info(ch);
        else
            listClanInfo(arg2, ch);
    }
    else
    {
        show_clan_info(ch);
        ch->sendText("These are viable options.\r\n");
    }
}

ACMD(do_aid)
{
    Character *vict;
    Object *obj = nullptr, *aid_obj = nullptr, *aid_prod = nullptr, *next_obj;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int dc = 0, found = false, num = 47, num2 = 0, survival = 0;

    if (IS_NPC(ch))
        return;

    if (GET_SKILL(ch, SKILL_SURVIVAL))
    {
        survival = 1;
    }

    two_arguments(argument, arg, arg2);

    if (!*arg)
    {
        ch->sendText("Syntax: aid heal (target)\r\n");
        ch->sendText("        aid adrenex\r\n");
        ch->sendText("        aid antitoxin\r\n");
        ch->sendText("        aid salve\r\n");
        ch->sendText("        aid formula-82\r\n");
        return;
    }

    if (!strcasecmp(arg, "adrenex"))
    {
        num = 380;
        num2 = 381;
    }
    else if (!strcasecmp(arg, "antitoxin"))
    {
        num = 380;
        num2 = 383;
    }
    else if (!strcasecmp(arg, "salve"))
    {
        num = 380;
        num2 = 382;
    }
    else if (!strcasecmp(arg, "formula-82"))
    {
        num = 380;
        num2 = 385;
    }

    aid_obj = ch->searchInventory(num);

    if (!aid_obj)
    {
        if (num == 47)
        {
            ch->sendText("You need bandages to be able to use first aid.\r\n");
        }
        else
        {
            ch->sendText("You need a TCX-Medical Equipment Construction Kit.\r\n");
        }
        return;
    }

    if (num == 47)
    {
        if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_ROOM)))
        {
            ch->sendText("Apply first aid to who?\r\n");
            return;
        }
        else if (IS_NPC(vict))
        {
            ch->sendText("What ever for?\r\n");
            return;
        }
        else if (IS_ANDROID(vict))
        {
            ch->sendText("They are an android!\r\n");
            return;
        }

        if (!AFF_FLAGGED(vict, AFF_SPIRIT) && !PLR_FLAGGED(vict, PLR_BANDAGED))
        {
            if (vict != ch)
            {
                ch->send_to("You attempt to lend first aid to %s.\r\n", GET_NAME(vict));
            }
            act("$n attempts to bandage $N's wounds.", true, ch, nullptr, vict, TO_ROOM);
            dc = axion_dice(0);
            if ((GET_SKILL(ch, SKILL_FIRST_AID) + 1) > dc)
            {
                ch->send_to("You bandage %s's wounds.\r\n", GET_NAME(vict));
                int64_t roll = (((vict->getEffectiveStat<int64_t>("health")) / 100) * (GET_WIS(ch) / 4)) + (vict->getEffectiveStat<int64_t>("health")) * 0.25;
                if (GET_BONUS(ch, BONUS_HEALER) > 0)
                {
                    roll += roll * .1;
                }
                vict->modCurVital(CharVital::health, roll);

                vict->send_to("Your wounds are bandaged by %s!\r\n", GET_NAME(ch));
                act("$n's wounds are stablized by $N!", true, vict, nullptr, ch, TO_NOTVICT);
                vict->player_flags.set(PLR_BANDAGED, true);
                extract_obj(aid_obj);
            }
            else
            {
                if (vict != ch)
                {
                    ch->sendText("You fail to bandage their wounds properly, wasting the set of bandages...\r\n");
                    act("$N fails to bandage $n's wounds properly, wasting an entire set of bandages...", true, vict,
                        nullptr, ch, TO_NOTVICT);
                    act("$N fails to bandage your wounds properly, wasting an entire set of bandages...", true, vict,
                        nullptr, ch, TO_CHAR);
                }
                else
                {
                    act("$N fails to bandage $s wounds properly, wasting an entire set of bandages...", true, vict,
                        nullptr, ch, TO_NOTVICT);
                    act("You fail to bandage your wounds properly, wasting an entire set of bandages...", true, vict,
                        nullptr, ch, TO_VICT);
                }
                extract_obj(aid_obj);
            }
            improve_skill(ch, SKILL_FIRST_AID, 1);
        }
        else if (PLR_FLAGGED(vict, PLR_BANDAGED))
        {
            ch->sendText("They are already bandaged!\r\n");
        }
        else if (AFF_FLAGGED(vict, AFF_SPIRIT))
        {
            ch->sendText("The dead don't need first aid.\r\n");
        }
        else
        {
            ch->sendText("They apparently do not need bandaging.\r\n");
        }
    }
    else if (num2 == 381)
    {
        if (GET_SKILL(ch, SKILL_FIRST_AID) < 65)
        {
            ch->sendText("You need at least a skill level of 65 in First Aid.\r\n");
            return;
        }
        else
        {
            if (GET_SKILL(ch, SKILL_FIRST_AID) < axion_dice(15))
            {
                act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you begin to construct an Andrenex Adreneline Injector you screw up and end up breaking the water tight seal. The adreneline leaks out and is wasted.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                extract_obj(aid_obj);
            }
            else
            {
                act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully construct an Adrenex Adreneline Injector@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Adrenex Adreneline Injector!@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                aid_prod = read_object(num2, VIRTUAL);
                ch->addToInventory(aid_prod);
                extract_obj(aid_obj);
                improve_skill(ch, SKILL_FIRST_AID, 1);
            }
        }
    }
    else if (num2 == 382)
    {
        if (GET_SKILL(ch, SKILL_FIRST_AID) < 50)
        {
            ch->sendText("You need at least a skill level of 50 in First Aid.\r\n");
            return;
        }
        else
        {
            if (GET_SKILL(ch, SKILL_FIRST_AID) < axion_dice(10))
            {
                act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you go to put the salve ingredients into the kit's salve compartment and set the temperature you accidentally set it too high. The salve is burned and ruined. Yes you managed to burn a burn salve.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                extract_obj(aid_obj);
            }
            else
            {
                act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully boil a burn salve to perfection and it is automatically placed in a jar.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a jar of burn salve!@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                aid_prod = read_object(num2, VIRTUAL);
                ch->addToInventory(aid_prod);
                extract_obj(aid_obj);
                improve_skill(ch, SKILL_FIRST_AID, 1);
            }
        }
    }
    else if (num2 == 383)
    {
        if (GET_SKILL(ch, SKILL_FIRST_AID) < 40)
        {
            ch->sendText("You need at least a skill level of 40 in First Aid.\r\n");
            return;
        }
        else
        {
            if (GET_SKILL(ch, SKILL_FIRST_AID) < axion_dice(5))
            {
                act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete the Antitoxin Injector you notice that you didn't seal the syringe properly and it all leaks out.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                extract_obj(aid_obj);
            }
            else
            {
                act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble the Antitoxin Injector.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Antitoxin Injector!@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                aid_prod = read_object(num2, VIRTUAL);
                ch->addToInventory(aid_prod);
                extract_obj(aid_obj);
                improve_skill(ch, SKILL_FIRST_AID, 1);
            }
        }
    }
    else if (num2 == 385)
    {
        if (GET_SKILL(ch, SKILL_FIRST_AID) < 40)
        {
            ch->sendText("You need at least a skill level of 40 in First Aid.\r\n");
            return;
        }
        else
        {
            if (GET_SKILL(ch, SKILL_FIRST_AID) < axion_dice(15))
            {
                act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete a vial of Formula 82 you notice that you read the mixture measurements wrong. You dispose of the vile vial immediately.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                extract_obj(aid_obj);
            }
            else
            {
                act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble a vial of Formula 82.@n",
                    true, ch, nullptr, nullptr, TO_CHAR);
                act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Vial of Formula 82!@n",
                    true, ch, nullptr, nullptr, TO_ROOM);
                aid_prod = read_object(num2, VIRTUAL);
                ch->addToInventory(aid_prod);
                extract_obj(aid_obj);
                improve_skill(ch, SKILL_FIRST_AID, 1);
            }
        }
    }

    WAIT_STATE(ch, PULSE_3SEC);
}

ACMD(do_aura)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Syntax: aura light\r\n");
        return;
    }

    if (GET_CHARGE(ch))
    {
        ch->sendText("You can't focus enough on this while charging.");
        return;
    }

    if (ch->character_flags.get(CharacterFlag::powering_up))
    {
        ch->sendText("You are busy powering up!\r\n");
        return;
    }

    if (!strcasecmp(arg, "light"))
    {
        if (GET_SKILL(ch, SKILL_FOCUS) < 75 || GET_SKILL(ch, SKILL_CONCENTRATION) < 75)
        {
            ch->sendText("You need at least a skill level of 75 in Focus and Concentration to use this.\r\n");
            return;
        }
        if (PLR_FLAGGED(ch, PLR_AURALIGHT))
        {
            ch->sendText("Your aura fades as you stop shining light.\r\n");
            act("$n's aura fades as they stop shining light on the area.", true, ch, nullptr, nullptr, TO_ROOM);
            ch->player_flags.set(PLR_AURALIGHT, false);
            characterSubscriptions.unsubscribe("auralight", ch);
        }
        else if ((ch->getCurVital(CharVital::ki)) > GET_MAX_MANA(ch) * 0.12)
        {
            reveal_hiding(ch, 0);
            ch->modCurVitalDam(CharVital::ki, .12);
            ch->send_to("A bright %s aura begins to burn around you as you provide light to the surrounding area!\r\n", GET_AURA(ch));
            char bloom[MAX_INPUT_LENGTH];
            sprintf(bloom, "@wA %s aura flashes up brightly around $n@w as they provide light to the area.@n",
                    GET_AURA(ch));
            act(bloom, true, ch, nullptr, nullptr, TO_ROOM);
            ch->player_flags.set(PLR_AURALIGHT, true);
            characterSubscriptions.subscribe("auralight", ch);
        }
        else
        {
            ch->sendText("You don't have enough KI to do that.\r\n");
            return;
        }
    }
}


void genBonus(Character *ch, std::string bonus)
{
    int toSet = NOTHING;

    if (is_abbrev(bonus.c_str(), "hand to hand") || bonus == "1")
    {
        toSet = 1;
    }
    if (is_abbrev(bonus.c_str(), "ki attacks") || bonus == "2")
    {
        toSet = 2;
    }
    if (is_abbrev(bonus.c_str(), "find my own way") || bonus == "3")
    {
        toSet = 3;
    }
    if (is_abbrev(bonus.c_str(), "money") || bonus == "4")
    {
        toSet = 4;
    }
    if (is_abbrev(bonus.c_str(), "weaponry (brawl)") || bonus == "5")
    {
        toSet = 5;
    }
    if (is_abbrev(bonus.c_str(), "weaponry (gun)") || bonus == "6")
    {
        toSet = 6;
    }
    if (is_abbrev(bonus.c_str(), "weaponry (spear)") || bonus == "7")
    {
        toSet = 7;
    }
    if (is_abbrev(bonus.c_str(), "weaponry (club)") || bonus == "8")
    {
        toSet = 8;
    }
    if (is_abbrev(bonus.c_str(), "weaponry (sword)") || bonus == "9")
    {
        toSet = 9;
    }
    if (is_abbrev(bonus.c_str(), "weaponry (dagger)") || bonus == "10")
    {
        toSet = 10;
    }

    if (toSet != NOTHING)
    {
        ch->setBaseStat("genBonus", toSet);
        ch->sendText("\r\nBonus set.\r\n");
        return;
    }

    ch->sendText("\n@YWhat would you like to specialise in?@n\r\n\n");
    ch->sendText("@B1@W);@C Hand to Hand - Starts with training in hand to hand combat@n\r\n");
    ch->sendText("@B2@W);@C Ki attacks - Starts with training in Ki skills@n\r\n");
    ch->sendText("@B3@W);@C Find my own way - Starts with good Attributes, but few skills@n\r\n");
    ch->sendText("@B4@W);@C Money - Starts with decent Zenni, but otherwise frail@n\r\n");

    ch->sendText("@B5@W);@C Weaponry (Brawl) - Starts with weaponry, but low Zenni@n\r\n");
    ch->sendText("@B6@W);@C Weaponry (Gun) - Starts with weaponry, but low Zenni@n\r\n");
    ch->sendText("@B7@W);@C Weaponry (Spear) - Starts with weaponry, but low Zenni@n\r\n");
    ch->sendText("@B8@W);@C Weaponry (Club) - Starts with weaponry, but low Zenni@n\r\n");
    ch->sendText("@B9@W);@C Weaponry (Sword) - Starts with weaponry, but low Zenni@n\r\n");
    ch->sendText("@B10@W);@C Weaponry (Dagger) - Starts with weaponry, but low Zenni@n\r\n");
}

void genHeight(Character *ch, std::string suggestedHeight)
{
    int height = atoi(suggestedHeight.c_str());

    if (ch->race == Race::tuffle && (height >= 20 && height <= 150))
    {
        ch->setBaseStat("height", height);
        ch->sendText("Height set.\r\n");
        return;
    }
    else if (height >= 80 && height <= 300)
    {
        ch->setBaseStat("height", height);
        ch->sendText("Height set.\r\n");
        return;
    }
    else
    {
        if (ch->race == Race::tuffle)
        {
            ch->sendText("For Tuffles, please keep height above 20(cm), and below 150(cm).\r\n");
        }
        else
        {
            ch->sendText("Please keep height above 80(cm), and below 300(cm).\r\n");
        }
    }
}

void genWeight(Character *ch, std::string suggestedWeight)
{
    int weight = atoi(suggestedWeight.c_str());

    if (ch->race == Race::tuffle && (weight >= 3 && weight <= 40))
    {
        ch->setBaseStat("weight", weight);
        ch->sendText("Weight set.\r\n");
        return;
    }
    else if (weight >= 25 && weight <= 150)
    {
        ch->setBaseStat("weight", weight);
        ch->sendText("Weight set.\r\n");
        return;
    }
    else
    {
        if (ch->race == Race::tuffle)
        {
            ch->sendText("For Tuffles, please keep weight above 3(kg), and below 40(kg).\r\n");
        }
        else
        {
            ch->sendText("Please keep weight above 25(kg), and below 150(kg).\r\n");
        }
    }
}

void genAge(Character *ch, std::string suggestedAge)
{
    try
    {
        auto years = std::stod(suggestedAge);
        if (years <= 8.0)
        {
            ch->sendText("Please create a character above the age of 8.\r\n");
            return;
        }
        ch->setAge(years);
        ch->sendText("Age set.\r\n");
    }
    catch (const std::invalid_argument &ia)
    {
        ch->sendText("Age can be set to any number above 8, please keep it reasonable.\r\n");
        return;
    }
}

void genFinish(Character *ch)
{

    auto gb = ch->getBaseStat<int>("genBonus");

    if (gb == 1)
    {
        ch->modBaseStat("health", 200);
        ch->modBaseStat("ki", 50);
        ch->modBaseStat("stamina", 100);
        ch->modBaseStat("money_carried", 200);

        ch->modBaseStat("strength", 2);
        ch->modBaseStat("constitution", 2);

        SET_SKILL(ch, SKILL_KICK, 30);
        SET_SKILL(ch, SKILL_KNEE, 30);
        SET_SKILL(ch, SKILL_ELBOW, 30);
    }
    if (gb == 2)
    {
        ch->modBaseStat("health", 50);
        ch->modBaseStat("ki", 200);
        ch->modBaseStat("stamina", 100);
        ch->modBaseStat("money_carried", 200);

        ch->modBaseStat("intelligence", 2);
        ch->modBaseStat("wisdom", 2);

        SET_SKILL(ch, SKILL_FOCUS, 30);
        SET_SKILL(ch, SKILL_KIBALL, 30);
        SET_SKILL(ch, SKILL_BEAM, 30);
    }
    if (gb == 3)
    {
        ch->modBaseStat("health", 200);
        ch->modBaseStat("ki", 200);
        ch->modBaseStat("stamina", 200);
        ch->modBaseStat("money_carried", 10);

        ch->modBaseStat("strength", 2);
        ch->modBaseStat("constitution", 2);
        ch->modBaseStat("speed", 2);
        ch->modBaseStat("agility", 2);
        ch->modBaseStat("intelligence", 2);
        ch->modBaseStat("wisdom", 2);

        SET_SKILL(ch, SKILL_FOCUS, 30);
    }
    if (gb == 4)
    {
        ch->modBaseStat("health", 50);
        ch->modBaseStat("ki", 50);
        ch->modBaseStat("stamina", 50);
        ch->modBaseStat("money_carried", 5000);

        ch->modBaseStat("intelligence", 2);
        ch->modBaseStat("constitution", 2);

        SET_SKILL(ch, SKILL_APPRAISE, 30);
        SET_SKILL(ch, SKILL_CONCENTRATION, 30);
    }
    if (gb == 5)
    {
        ch->modBaseStat("health", 100);
        ch->modBaseStat("ki", 50);
        ch->modBaseStat("stamina", 200);
        ch->modBaseStat("money_carried", 500);

        ch->modBaseStat("wisdom", 3);
        ch->modBaseStat("speed", 2);

        SET_SKILL(ch, SKILL_PARRY, 30);
        SET_SKILL(ch, SKILL_BRAWL, 30);
    }
    if (gb == 6)
    {
        ch->modBaseStat("health", 100);
        ch->modBaseStat("ki", 50);
        ch->modBaseStat("stamina", 200);
        ch->modBaseStat("money_carried", 500);

        ch->modBaseStat("wisdom", 3);
        ch->modBaseStat("speed", 2);

        SET_SKILL(ch, SKILL_PARRY, 30);
        SET_SKILL(ch, SKILL_GUN, 30);
    }
    if (gb == 7)
    {
        ch->modBaseStat("health", 100);
        ch->modBaseStat("ki", 50);
        ch->modBaseStat("stamina", 200);
        ch->modBaseStat("money_carried", 500);

        ch->modBaseStat("wisdom", 3);
        ch->modBaseStat("speed", 2);

        SET_SKILL(ch, SKILL_PARRY, 30);
        SET_SKILL(ch, SKILL_SPEAR, 30);
    }
    if (gb == 8)
    {
        ch->modBaseStat("health", 100);
        ch->modBaseStat("ki", 50);
        ch->modBaseStat("stamina", 200);
        ch->modBaseStat("money_carried", 500);

        ch->modBaseStat("wisdom", 3);
        ch->modBaseStat("speed", 2);

        SET_SKILL(ch, SKILL_PARRY, 30);
        SET_SKILL(ch, SKILL_CLUB, 30);
    }
    if (gb == 9)
    {
        ch->modBaseStat("health", 100);
        ch->modBaseStat("ki", 50);
        ch->modBaseStat("stamina", 200);
        ch->modBaseStat("money_carried", 500);

        ch->modBaseStat("wisdom", 3);
        ch->modBaseStat("speed", 2);

        SET_SKILL(ch, SKILL_PARRY, 30);
        SET_SKILL(ch, SKILL_SWORD, 30);
    }
    if (gb == 10)
    {
        ch->modBaseStat("health", 100);
        ch->modBaseStat("ki", 50);
        ch->modBaseStat("stamina", 200);
        ch->modBaseStat("money_carried", 500);

        ch->modBaseStat("wisdom", 3);
        ch->modBaseStat("speed", 2);

        SET_SKILL(ch, SKILL_PARRY, 30);
        SET_SKILL(ch, SKILL_DAGGER, 30);
    }

    if (ch->bio_genomes.get(Race::kai))
    {
        SET_SKILL(ch, SKILL_TELEPATHY, 30);
        SET_SKILL(ch, SKILL_FOCUS, 30);
    }
    if (ch->mutations.get(Mutation::extreme_reflexes))
    {
        ch->modBaseStat("agility", 10);
    }
    if (ch->mutations.get(Mutation::innate_telepathy))
    {
        SET_SKILL(ch, SKILL_TELEPATHY, 50);
    }

    if (IS_BARDOCK(ch))
    {
        ch->gravAcclim[0] = 10000;
        ch->gravAcclim[1] = 10000;
        ch->gravAcclim[2] = -7500;
        ch->gravAcclim[3] = -5000;
        ch->gravAcclim[4] = -5000;
        ch->gravAcclim[5] = -5000;
    }

    do_start(ch);

    ch->teleport_to(sensei::getStartRoom(ch->sensei));
}

static std::vector<std::string> start_bonus = {
    "", "Hand to Hand", "Ki Attacks", "Find My Own Way", "Money", "Weaponry (Brawl)", "Weaponry (Gun)", "Weaponry (Spear)",
    "Weaponry (Club)", "Weaponry (Sword)", "Weaponry (Dagger)"};
