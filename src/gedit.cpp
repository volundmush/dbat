/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  gedit.c:  Olc written for shoplike guildmasters, code by             *
 *             Jason Goodwin                                             *
 *    Made for Circle3.0 bpl11, its copyright applies                    *
 *                                                                       *
 *  Made for Oasis OLC                                                   *
 *  Copyright 1996 Harvey Gilpin.                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "dbat/Descriptor.h"
#include "dbat/Character.h"
#include "dbat/Zone.h"
#include "dbat/CharacterPrototype.h"
#include "dbat/gedit.h"
#include "dbat/genzon.h"
#include "dbat/act.informative.h"
#include "dbat/oasis.h"
#include "dbat/gengld.h"
#include "dbat/send.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/interpreter.h"
#include "dbat/spells.h"
#include "dbat/feats.h"
#include "dbat/genolc.h"
#include "dbat/Shop.h"

/*
 * Should check more things.
 */
void gedit_save_internally(struct descriptor_data *d)
{
    OLC_GUILD(d)->vnum = OLC_NUM(d);
    add_guild(OLC_GUILD(d));
}

void gedit_save_to_disk(int num)
{
    save_guilds(num);
}

/*-------------------------------------------------------------------*\
  utility functions
 \*-------------------------------------------------------------------*/

ACMD(do_oasis_gedit)
{
    int number = NOWHERE, save = 0;
    guild_rnum real_num;
    struct descriptor_data *d;
    char *buf3;
    char buf1[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];

    /****************************************************************************/
    /** Parse any arguments.                                                   **/
    /****************************************************************************/
    buf3 = two_arguments(argument, buf1, buf2);

    if (!*buf1)
    {
        ch->sendText("Specify a guild VNUM to edit.\r\n");
        return;
    }
    else if (!isdigit(*buf1))
    {
        if (!boost::iequals("save", buf1) != 0)
        {
            ch->sendText("Yikes!  Stop that, someone will get hurt!\r\n");
            return;
        }

        save = true;

        if (is_number(buf2))
            number = atoi(buf2);
        else if (GET_OLC_ZONE(ch) > 0)
        {
            zone_rnum zlok;

            if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
                number = NOWHERE;
        }

        if (number == NOWHERE)
        {
            ch->sendText("Save which zone?\r\n");
            return;
        }
    }

    /****************************************************************************/
    /** If a numeric argument was given, get it.                               **/
    /****************************************************************************/
    if (number == NOWHERE)
        number = atoi(buf1);

    /****************************************************************************/
    /** Check that the guild isn't already being edited.                        **/
    /****************************************************************************/
    for (d = descriptor_list; d; d = d->next)
    {
        if (STATE(d) == CON_GEDIT)
        {
            if (d->olc && OLC_NUM(d) == number)
            {
                ch->send_to("That guild is currently being edited by %s.\r\n", d->character->displayNameFor(ch));
                return;
            }
        }
    }

    /****************************************************************************/
    /** Point d to the builder's descriptor.                                   **/
    /****************************************************************************/
    d = ch->desc;

    /****************************************************************************/
    /** Give the descriptor an OLC structure.                                  **/
    /****************************************************************************/
    if (d->olc)
    {
        mudlog(BRF, ADMLVL_IMMORT, true,
               "SYSERR: do_oasis_gedit: Player already had olc structure.");
        delete d->olc;
    }

    d->olc = new oasis_olc_data();

    /****************************************************************************/
    /** Everyone but IMPLs can only edit zones they have been assigned.        **/
    /****************************************************************************/
    if (!can_edit_zone(ch, NOWHERE))
    {
        send_cannot_edit(ch, NOWHERE);

        /**************************************************************************/
        /** Free the OLC structure.                                              **/
        /**************************************************************************/
        delete d->olc;
        d->olc = nullptr;
        return;
    }

    if (save)
    {

        /**************************************************************************/
        /** Free the OLC structure.                                              **/
        /**************************************************************************/
        delete d->olc;
        d->olc = nullptr;
        return;
    }

    OLC_NUM(d) = number;

    if ((real_num = real_guild(number)) != NOTHING)
        gedit_setup_existing(d, real_num);
    else
        gedit_setup_new(d);

    STATE(d) = CON_GEDIT;

    act("$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
    ch->player_flags.set(PLR_WRITING, true);

    mudlog(BRF, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
           GET_NAME(ch), zone_table.at(OLC_ZNUM(d)).number, GET_OLC_ZONE(ch));
}

void gedit_setup_new(struct descriptor_data *d)
{
    auto guilddata = new guild_data();

    /*. Some default strings . */
    G_NO_SKILL(guilddata) = strdup("%s Sorry, but I don't know that one.");
    G_NO_GOLD(guilddata) = strdup("%s Sorry, but I'm gonna need more zenni first.");

    OLC_GUILD(d) = guilddata;
    gedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void gedit_setup_existing(struct descriptor_data *d, int rgm_num)
{
    /*. Alloc some guild shaped space . */
    OLC_GUILD(d) = new guild_data();
    copy_guild(OLC_GUILD(d), &guild_index.at(rgm_num));
    gedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/**************************************************************************
 Menu functions
**************************************************************************/

/*-------------------------------------------------------------------*/

void gedit_select_skills_menu(struct descriptor_data *d)
{
    int i, j = 0, found = 0;
    auto guilddata = OLC_GUILD(d);
    clear_screen(d);

    d->sendText("Skills known:\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE; i++)
    {
        if (spell_info[i].skilltype == SKTYPE_SKILL &&
            strcmp(spell_info[i].name, "!UNUSED!"))
        {
            d->send_to("@n[@c%-3s@n] %-3d %-20.20s  ",
                       YESNO(guilddata->skills.get(i)), i, spell_info[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3))
        {
            found = 0;
            d->sendText("\r\n");
        }
    }
    d->sendText("\r\nEnter skill num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_SKILLS;
}

/*-------------------------------------------------------------------*/

void gedit_select_spells_menu(struct descriptor_data *d)
{
    int i, j = 0, found = 0;
    struct guild_data *guilddata;

    guilddata = OLC_GUILD(d);
    clear_screen(d);

    d->sendText("Spells known:\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE; i++)
    {
        if (IS_SET(spell_info[i].skilltype, SKTYPE_SPELL) &&
            strcmp(spell_info[i].name, "!UNUSED!"))
        {
            d->send_to("@n[@c%-3s@n] %-3d %-20.20s  ",
                       YESNO(guilddata->skills.get(i)), i, spell_info[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3))
        {
            found = 0;
            d->sendText("\r\n");
        }
    }
    d->sendText("\r\nEnter spell num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_SPELLS;
}

/*-------------------------------------------------------------------*/

void gedit_select_feats_menu(struct descriptor_data *d)
{
    int i, j = 0, found = 0;
    auto guilddata = OLC_GUILD(d);
    clear_screen(d);

    d->sendText("Feats known:\r\n");

    for (i = 0; i <= NUM_FEATS_DEFINED; i++)
    {
        if (feat_list[i].in_game)
        {
            d->send_to("@n[@c%-3s@n] %-3d %-20.20s  ",
                       YESNO(guilddata->feats.count(i)), i, feat_list[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3))
        {
            found = 0;
            d->sendText("\r\n");
        }
    }
    d->sendText("\r\nEnter feat num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_FEATS;
}

/*-------------------------------------------------------------------*/

void gedit_select_lang_menu(struct descriptor_data *d)
{
    int i, j = 0, found = 0;
    auto guilddata = OLC_GUILD(d);
    clear_screen(d);

    d->sendText("Skills known:\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE; i++)
    {
        if (IS_SET(spell_info[i].skilltype, SKTYPE_LANG) &&
            strcmp(spell_info[i].name, "!UNUSED!"))
        {
            d->send_to("@n[@c%-3s@n] %-3d %-20.20s  ",
                       YESNO(guilddata->skills.get(i)), i, spell_info[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3))
        {
            found = 0;
            d->sendText("\r\n");
        }
    }
    d->sendText("\r\nEnter skill num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_LANGS;
}

/*-------------------------------------------------------------------*/

void gedit_select_wp_menu(struct descriptor_data *d)
{
    int i, j = 0, found = 0;
    auto guilddata = OLC_GUILD(d);
    clear_screen(d);

    d->sendText("Skills known:\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE; i++)
    {
        if (IS_SET(spell_info[i].skilltype, SKTYPE_WEAPON) &&
            strcmp(spell_info[i].name, "!UNUSED!"))
        {
            d->send_to("@n[@c%-3s@n] %-3d %-20.20s  ",
                       YESNO(guilddata->skills.get(i)), i, spell_info[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3))
        {
            found = 0;
            d->sendText("\r\n");
        }
    }
    d->sendText("\r\nEnter skill num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_WPS;
}

/*-------------------------------------------------------------------*/

void gedit_no_train_menu(struct descriptor_data *d)
{
    char bits[MAX_STRING_LENGTH];
    int i, count = 0;
    struct guild_data *guilddata;

    guilddata = OLC_GUILD(d);
    clear_screen(d);

    for (i = 0; i < NUM_TRADERS; i++)
    {
        d->send_to("@g%2d@n) %-20.20s   %s", i + 1, trade_letters[i],
                   !(++count % 2) ? "\r\n" : "");
    }

    // sprintbitarray(G_WITH_WHO(guilddata), trade_letters, sizeof(bits), bits);
    d->send_to("\r\nCurrent train flags: @c%s@n\r\nEnter choice, 0 to quit : ", bits);
    OLC_MODE(d) = GEDIT_NO_TRAIN;
}

/*-------------------------------------------------------------------*/
/*. Display main menu . */

void gedit_disp_menu(struct descriptor_data *d)
{
    struct guild_data *guilddata;
    char buf1[MAX_STRING_LENGTH];

    guilddata = OLC_GUILD(d);

    clear_screen(d);

    // sprintbitarray(G_WITH_WHO(guilddata), trade_letters, sizeof(buf1), buf1);

    d->send_to(
        "-- Guild Number: [@c%d@n]\r\n"
        "@g 0@n) Guild Master : [@c%d@n] @y%s\r\n"
        "@g 1@n) Doesn't know skill:\r\n @y%s\r\n"
        "@g 2@n) Player no gold:\r\n @y%s\r\n"
        "@g 3@n) Open   :  [@c%d@n]\r\n"
        "@g 4@n) Close  :  [@c%d@n]\r\n"
        "@g 5@n) Charge :  [@c%3.1f@n]\r\n"
        "@g 6@n) Minlvl :  [@c%d@n]\r\n"
        "@g 7@n) Who to Train:  @c%s\r\n"
        "@g 8@n) Feats Menu\r\n"
        "@g 9@n) Skills Menu\r\n"
        "@g B@n) Languages Menu\r\n"
        "@g Q@n) Quit\r\n"
        "Enter Choice : ",

        OLC_NUM(d),
        G_TRAINER(guilddata) == NOBODY ? -1 : mob_index.at(G_TRAINER(guilddata)).vn,
        G_TRAINER(guilddata) == NOBODY ? "None" : mob_proto.at(G_TRAINER(guilddata)).short_description,
        G_NO_SKILL(guilddata).c_str(),
        G_NO_GOLD(guilddata).c_str(),
        G_OPEN(guilddata),
        G_CLOSE(guilddata),
        G_CHARGE(guilddata),
        G_MINLVL(guilddata),
        buf1);

    OLC_MODE(d) = GEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTUAN event handler
**************************************************************************/

void gedit_parse(struct descriptor_data *d, char *arg)
{
    int i;

    if (OLC_MODE(d) > GEDIT_NUMERICAL_RESPONSE)
    {
        if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1]))))
        {
            d->sendText("Field must be numerical, try again : ");
            return;
        }
    }
    switch (OLC_MODE(d))
    {
        /*-------------------------------------------------------------------*/
    case GEDIT_CONFIRM_SAVESTRING:
        switch (*arg)
        {
        case 'y':
        case 'Y':
            d->character->sendText("Saving Guild to memory.\r\n");
            gedit_save_internally(d);
            mudlog(CMP, std::max(ADMLVL_BUILDER, GET_INVIS_LEV(d->character)), true,
                   "OLC: %s edits guild %d", GET_NAME(d->character), OLC_NUM(d));
            if (CONFIG_OLC_SAVE)
            {
                d->send_to("Guild %d saved to disk.\r\n", OLC_NUM(d));
            }
            else
                d->send_to("Guild %d saved to memory.\r\n", OLC_NUM(d));
            cleanup_olc(d, CLEANUP_STRUCTS);
            return;
        case 'n':
        case 'N':
            cleanup_olc(d, CLEANUP_ALL);
            return;
        default:
            d->sendText("Invalid choice!\r\nDo you wish to save the guild? : ");
            return;
        }
        break;

        /*-------------------------------------------------------------------*/
    case GEDIT_MAIN_MENU:
        i = 0;
        switch (*arg)
        {
        case 'q':
        case 'Q':
            if (OLC_VAL(d))
            { /*. Anything been changed? . */
                d->sendText("Do you wish to save the changes to the Guild? (y/n) : ");
                OLC_MODE(d) = GEDIT_CONFIRM_SAVESTRING;
            }
            else
                cleanup_olc(d, CLEANUP_ALL);
            return;
        case '0':
            OLC_MODE(d) = GEDIT_TRAINER;
            d->sendText("Enter vnum of guild master : ");
            return;
        case '1':
            OLC_MODE(d) = GEDIT_NO_SKILL;
            i--;
            break;
        case '2':
            OLC_MODE(d) = GEDIT_NO_CASH;
            i--;
            break;
        case '3':
            OLC_MODE(d) = GEDIT_OPEN;
            d->sendText("When does this shop open (a day has 28 hours) ? ");
            i++;
            break;
        case '4':
            OLC_MODE(d) = GEDIT_CLOSE;
            d->sendText("When does this shop close (a day has 28 hours) ? ");
            i++;
            break;
        case '5':
            OLC_MODE(d) = GEDIT_CHARGE;
            i++;
            break;
        case '6':
            OLC_MODE(d) = GEDIT_MINLVL;
            d->sendText("Minumum Level will Train: ");
            i++;
            return;
        case '7':
            OLC_MODE(d) = GEDIT_NO_TRAIN;
            gedit_no_train_menu(d);
            return;
        case '8':
            OLC_MODE(d) = GEDIT_SELECT_FEATS;
            gedit_select_feats_menu(d);
            return;
        case '9':
            OLC_MODE(d) = GEDIT_SELECT_SKILLS;
            gedit_select_skills_menu(d);
            return;
            /*case 'a':
            case 'A':
                OLC_MODE(d) = GEDIT_SELECT_WPS;
                gedit_select_wp_menu(d);
                return; */
        case 'b':
        case 'B':
            OLC_MODE(d) = GEDIT_SELECT_LANGS;
            gedit_select_lang_menu(d);
            return;
        default:
            gedit_disp_menu(d);
            return;
        }

        if (i == 0)
            break;
        else if (i == 1)
            d->sendText("\r\nEnter new value : ");
        else if (i == -1)
            d->sendText("\r\nEnter new text :\r\n] ");
        else
            d->sendText("Oops...\r\n");
        return;
        /*-------------------------------------------------------------------*/
        /*. String edits . */
    case GEDIT_NO_SKILL:
        gedit_modify_string(G_NO_SKILL(OLC_GUILD(d)), arg);
        break;
    case GEDIT_NO_CASH:
        gedit_modify_string(G_NO_GOLD(OLC_GUILD(d)), arg);
        break;

        /*-------------------------------------------------------------------*/
        /*. Numerical responses . */

    case GEDIT_TRAINER:
        if (isdigit(*arg))
        {
            i = atoi(arg);
            if ((i = atoi(arg)) != -1)
                if ((i = real_mobile(i)) == NOBODY)
                {
                    d->sendText("That mobile does not exist, try again : ");
                    return;
                }
            G_TRAINER(OLC_GUILD(d)) = i;
            if (i == -1)
                break;
            /*. Fiddle with special procs . */
            G_FUNC(OLC_GUILD(d)) = mob_index.at(i).func != guild ? mob_index.at(i).func : nullptr;
            mob_index.at(i).func = guild;
            break;
        }
        else
        {
            d->sendText("Invalid response.\r\n");
            gedit_disp_menu(d);
            return;
        }
    case GEDIT_OPEN:
        G_OPEN(OLC_GUILD(d)) = LIMIT(atoi(arg), 0, 28);
        break;
    case GEDIT_CLOSE:
        G_CLOSE(OLC_GUILD(d)) = LIMIT(atoi(arg), 0, 28);
        ;
        break;
    case GEDIT_CHARGE:
        sscanf(arg, "%f", &G_CHARGE(OLC_GUILD(d)));
        break;
    case GEDIT_NO_TRAIN:
        if ((i = LIMIT(atoi(arg), 0, NUM_TRADERS - 1)) > 0)
        {
            gedit_no_train_menu(d);
            return;
        }
        break;

    case GEDIT_MINLVL:
        G_MINLVL(OLC_GUILD(d)) = std::max(atoi(arg), 0);
        break;

    case GEDIT_SELECT_SPELLS:
        i = atoi(arg);
        if (i == 0)
            break;
        i = std::clamp(i, 1, SKILL_TABLE_SIZE);
        OLC_GUILD(d)->toggle_skill(i);
        gedit_select_spells_menu(d);
        return;

    case GEDIT_SELECT_FEATS:
        i = atoi(arg);
        if (i == 0)
            break;
        i = std::clamp(i, 1, NUM_FEATS_DEFINED);
        OLC_GUILD(d)->toggle_feat(i);
        gedit_select_feats_menu(d);
        return;

    case GEDIT_SELECT_SKILLS:
        i = atoi(arg);
        if (i == 0)
            break;
        i = std::clamp(i, 1, SKILL_TABLE_SIZE);
        OLC_GUILD(d)->toggle_skill(i);
        gedit_select_skills_menu(d);
        return;

    case GEDIT_SELECT_WPS:
        i = atoi(arg);
        if (i == 0)
            break;
        i = std::clamp(i, 1, SKILL_TABLE_SIZE);
        OLC_GUILD(d)->toggle_feat(i);
        gedit_select_wp_menu(d);
        return;

    case GEDIT_SELECT_LANGS:
        i = atoi(arg);
        if (i == 0)
            break;
        i = std::clamp(i, 1, SKILL_TABLE_SIZE);
        OLC_GUILD(d)->toggle_skill(i);
        gedit_select_lang_menu(d);
        return;

        /*-------------------------------------------------------------------*/
    default:
        /*. We should never get here . */
        cleanup_olc(d, CLEANUP_ALL);
        mudlog(BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: gedit_parse(): "
                                          "Reached default case!");
        d->sendText("Oops...\r\n");
        break;
    }
    /*-------------------------------------------------------------------*/
    /*. END OF CASE
      If we get here, we have probably changed something, and now want to
      return to main menu.  Use OLC_VAL as a 'has changed' flag . */

    OLC_VAL(d) = 1;
    gedit_disp_menu(d);
}
