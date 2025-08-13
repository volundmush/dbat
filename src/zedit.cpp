/************************************************************************
 * OasisOLC - Zones / zedit.c					v2.0	*
 * Copyright 1996 Harvey Gilpin						*
 * Copyright 1997-2001 George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/structs.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/send.h"
#include "dbat/db.h"
#include "dbat/constants.h"
#include "dbat/genolc.h"
#include "dbat/genzon.h"
#include "dbat/oasis.h"
#include "dbat/dg_scripts.h"
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/handler.h"

/*-------------------------------------------------------------------*/

ACMD(do_oasis_zedit)
{
    int number = NOWHERE, save = 0, real_num;
    struct descriptor_data *d;
    char *buf3;
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    /****************************************************************************/
    /** Parse any arguments.                                                   **/
    /****************************************************************************/
    buf3 = two_arguments(argument, buf1, buf2);

    /****************************************************************************/
    /** If no argument was given, use the zone the builder is standing in.     **/
    /****************************************************************************/
    if (!*buf1)
        number = ch->location.getVnum();
    else if (!isdigit(*buf1))
    {
        if (strcasecmp("save", buf1) == 0)
        {
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
        else if (GET_ADMLEVEL(ch) >= ADMLVL_IMPL)
        {
            if (strcasecmp("new", buf1) || !buf3 || !*buf3)
                ch->sendText("Format: zedit new <zone number> <bottom-room> "
                             "<upper-room>\r\n");
            else
            {

                number = atoi(buf2);
                if (number < 0)
                    number = NOWHERE;

                /**********************************************************************/
                /** Setup the new zone (displays the menu to the builder).           **/
                /**********************************************************************/
                zedit_new_zone(ch, number);
            }

            /************************************************************************/
            /** Done now, exit the function.                                       **/
            /************************************************************************/
            return;
        }
        else
        {
            ch->sendText("Yikes!  Stop that, someone will get hurt!\r\n");
            return;
        }
    }

    /****************************************************************************/
    /** If a numeric argument was given, retrieve it.                           **/
    /****************************************************************************/
    if (number == NOWHERE)
        number = atoi(buf1);

    /****************************************************************************/
    /** Check that nobody is currently editing this zone.                      **/
    /****************************************************************************/
    for (d = descriptor_list; d; d = d->next)
    {
        if (STATE(d) == CON_ZEDIT)
        {
            if (d->olc && OLC_NUM(d) == number)
            {
                ch->send_to("That zone is currently being edited by %s.\r\n", PERS(d->character, ch));
                return;
            }
        }
    }

    /****************************************************************************/
    /** Store the builder's descriptor in d.                                   **/
    /****************************************************************************/
    d = ch->desc;

    /****************************************************************************/
    /** Give the builder's descriptor an OLC structure.                        **/
    /****************************************************************************/
    if (d->olc)
    {
        mudlog(BRF, ADMLVL_IMMORT, true, "SYSERR: do_oasis_zedit: Player already "
                                         "had olc structure.");
        free(d->olc);
    }

    d->olc = new oasis_olc_data();

    /****************************************************************************/
    /** Find the zone.                                                         **/
    /****************************************************************************/
    OLC_ZNUM(d) = save ? number : NOWHERE;
    if (OLC_ZNUM(d) == NOWHERE)
    {
        ch->sendText("Sorry, there is no zone for that number!\r\n");

        /**************************************************************************/
        /** Free the descriptor's OLC structure.                                 **/
        /**************************************************************************/
        delete d->olc;
        d->olc = nullptr;
        return;
    }

    auto &z = zone_table.at(OLC_ZNUM(d));

    /****************************************************************************/
    /** Everyone but IMPLs can only edit zones they have been assigned.        **/
    /****************************************************************************/
    if (!can_edit_zone(ch, OLC_ZNUM(d)))
    {
        send_cannot_edit(ch, z.number);
        free(d->olc);
        d->olc = nullptr;
        return;
    }

    /****************************************************************************/
    /** If we need to save, then save the zone.                                **/
    /****************************************************************************/
    if (save)
    {
        ch->send_to("Saving all zone information for zone %d.\r\n", z.number);
        mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), true,
               "OLC: %s saves zone information for zone %d.", GET_NAME(ch),
               z.number);

        /**************************************************************************/
        /** Save the zone information to the zone file.                          **/
        /**************************************************************************/

        /**************************************************************************/
        /** Free the descriptor's OLC structure.                                 **/
        /**************************************************************************/
        delete d->olc;
        d->olc = nullptr;
        return;
    }

    OLC_NUM(d) = number;

    if ((real_num = real_room(number)) == NOWHERE)
    {
        d->sendText("That room does not exist.\r\n");

        /**************************************************************************/
        /** Free the descriptor's OLC structure.                                 **/
        /**************************************************************************/
        free(d->olc);
        d->olc = nullptr;
        return;
    }

    zedit_setup(d, real_num);
    STATE(d) = CON_ZEDIT;

    act("$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
    ch->player_flags.set(PLR_WRITING, true);

    mudlog(CMP, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
           GET_NAME(ch), z.number, GET_OLC_ZONE(ch));
}

void zedit_setup(struct descriptor_data *d, int room_num)
{
    int subcmd = 0, count = 0, cmd_room = NOWHERE;

    /*
     * Allocate one scratch zone structure.
     */

    auto &z = zone_table.at(OLC_ZNUM(d));
    auto zd = OLC_ZONE(d) = new Zone();
    /*
     * Copy all the zone header information over.
     */
    zd->name = z.name;
    zd->builders = z.builders;
    zd->lifespan = z.lifespan;
    zd->reset_mode = z.reset_mode;
    zd->zone_flags = z.zone_flags;
    /*
     * The remaining fields are used as a 'has been modified' flag
     */
    zd->number = 0; /* Header information has changed.	*/
    zd->age = 0;    /* The commands have changed.		*/

    /*
     * Add all entries in zone_table that relate to this room.
     */

    /*
     * Display main menu.
     */
    zedit_disp_menu(d);
}

/*------------------------------------------------------------------*/
void zedit_disp_flag_menu(struct descriptor_data *d)
{
    int counter, columns = 0;
    char bits[MAX_STRING_LENGTH];

    clear_screen(d);
    for (counter = 0; counter < NUM_ZONE_FLAGS; counter++)
    {
        d->send_to("@g%2d@n) %-20.20s %s", counter + 1,
                   zone_bits[counter], !(++columns % 2) ? "\r\n" : "");
    }

    sprintf(bits, "%s", OLC_ZONE(d)->zone_flags.getFlagNames().c_str());
    d->send_to("\r\nZone flags: @c%s@n\r\n"
               "Enter Zone flags, 0 to quit : ",
               bits);
    OLC_MODE(d) = ZEDIT_ZONE_FLAGS;
}

/*-------------------------------------------------------------------*/

/*
 * Create a new zone.
 */
void zedit_new_zone(Character *ch, zone_vnum vzone_num)
{
    int result;
    const char *error;
    struct descriptor_data *dsc;

    if ((result = create_new_zone(vzone_num, &error)) == NOWHERE)
    {
        ch->desc->sendText(error);
        return;
    }

    mudlog(BRF, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), true, "OLC: %s creates new zone #%d", GET_NAME(ch), vzone_num);
    ch->desc->sendText("Zone created successfully.\r\n");
}

/*-------------------------------------------------------------------*/

/*
 * Save all the information in the player's temporary buffer back into
 * the current zone table.
 */
void zedit_save_internally(struct descriptor_data *d)
{
    int mobloaded = false,
        objloaded = false,
        subcmd;
    room_rnum room_num = real_room(OLC_NUM(d));

    if (room_num == NOWHERE)
    {
        basic_mud_log("SYSERR: zedit_save_internally: OLC_NUM(d) room %d not found.", OLC_NUM(d));
        return;
    }

    auto zd = OLC_ZONE(d);

    auto &z = zone_table.at(OLC_ZNUM(d));

    /*
     * Finally, if zone headers have been changed, copy over
     */
    if (zd->number)
    {
        z.name = zd->name;
        z.builders = zd->builders;
        z.reset_mode = zd->reset_mode;
        z.lifespan = zd->lifespan;
        z.zone_flags = zd->zone_flags;
    }
}

/*-------------------------------------------------------------------*/


/*-------------------------------------------------------------------*/


/**************************************************************************
 Menu functions
 **************************************************************************/

/*
 * the main menu
 */
void zedit_disp_menu(struct descriptor_data *d)
{
    int subcmd = 0, room, counter = 0;
    char buf1[MAX_STRING_LENGTH];

    clear_screen(d);
    room = real_room(OLC_NUM(d));
    sprintf(buf1, "%s", OLC_ZONE(d)->zone_flags.getFlagNames().c_str());
    auto &z = zone_table.at(OLC_ZNUM(d));
    /*
     * Menu header
     */
    d->character->send_to("Room number: [@c%d@n]\t\tRoom zone: @c%d\r\n"
                          "@g1@n);Builders       : @y%s\r\n"
                          "@gA@n) Zone name      : @y%s\r\n"
                          "@gL@n) Lifespan       : @y%d minutes\r\n"
                          "@gR@n) Reset Mode     : @y%s@n\r\n"
                          "@gF@n) Zone Flags     : @y%s@n\r\n"
                          "@gZ@n) Wiznet         :\r\n",

                          OLC_NUM(d),
                          z.number,
                          !OLC_ZONE(d)->builders.empty() ? OLC_ZONE(d)->builders : "None.",
                          !OLC_ZONE(d)->name.empty() ? OLC_ZONE(d)->name : "<NONE!>",
                          OLC_ZONE(d)->lifespan,
                          OLC_ZONE(d)->reset_mode ? ((OLC_ZONE(d)->reset_mode == 1) ? "Reset when no players are in zone."
                                                                                    : "Normal reset.")
                                                  : "Never reset",
                          buf1);


    /*
     * Finish off menu
     */
    d->send_to("@n%d - <END OF LIST>\r\n"
               "@gN@n) Insert new command.\r\n"
               "@gE@n) Edit a command.\r\n"
               "@gD@n) Delete a command.\r\n"
               "@gQ@n) Quit\r\nEnter your choice : ",
               counter);

    OLC_MODE(d) = ZEDIT_MAIN_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Print the command type menu and setup response catch.
 */
void zedit_disp_comtype(struct descriptor_data *d)
{
    clear_screen(d);
    d->send_to("\r\n"
               "@gM@n) Load Mobile to room             @gO@n) Load Object to room\r\n"
               "@gE@n) Equip mobile with object        @gG@n) Give an object to a mobile\r\n"
               "@gP@n) Put object in another object    @gD@n) Open/Close/Lock a Door\r\n"
               "@gR@n) Remove an object from the room\r\n"
               "@gT@n) Assign a trigger                @gV@n) Set a global variable\r\n"
               "\r\n"
               "What sort of command will this be? : ");
    OLC_MODE(d) = ZEDIT_COMMAND_TYPE;
}


/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void zedit_parse(struct descriptor_data *d, char *arg)
{
    int pos, i = 0;
    int number;

    switch (OLC_MODE(d))
    {
        /*-------------------------------------------------------------------*/
    case ZEDIT_CONFIRM_SAVESTRING:
        switch (*arg)
        {
        case 'y':
        case 'Y':
            /*
             * Save the zone in memory, hiding invisible people.
             */
            zedit_save_internally(d);
            if (CONFIG_OLC_SAVE)
            {
                d->sendText("Saving zone info to disk.\r\n");
            }
            else
                d->sendText("Saving zone info in memory.\r\n");

            mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(d->character)), true,
                   "OLC: %s edits zone info for room %d.", GET_NAME(d->character), OLC_NUM(d));
            /* FALL THROUGH */
        case 'n':
        case 'N':
            cleanup_olc(d, CLEANUP_ALL);
            break;
        default:
            d->sendText("Invalid choice!\r\n");
            d->sendText("Do you wish to save your changes? : ");
            break;
        }
        break;
        /* End of ZEDIT_CONFIRM_SAVESTRING */

        /*-------------------------------------------------------------------*/
    case ZEDIT_MAIN_MENU:
        switch (*arg)
        {
        case 'q':
        case 'Q':
            if (OLC_ZONE(d)->age || OLC_ZONE(d)->number)
            {
                d->sendText("Do you wish to save your changes? : ");
                OLC_MODE(d) = ZEDIT_CONFIRM_SAVESTRING;
            }
            else
            {
                d->sendText("No changes made.\r\n");
                cleanup_olc(d, CLEANUP_ALL);
            }
            break;
        case 'a':
        case 'A':
            /*
             * Edit zone name.
             */
            d->sendText("Enter new zone name : ");
            OLC_MODE(d) = ZEDIT_ZONE_NAME;
            break;
        case '1':
            /*
             * Edit zone builders.
             * We do not want to allow baby builders to tweak this.
             */
            if (GET_ADMLEVEL(d->character) <= ADMLVL_BUILDER)
            {
                OLC_MODE(d) = ZEDIT_MAIN_MENU;
                d->sendText("Access Denied.\r\n");
            }
            else
            {
                d->sendText("Enter new builders list : ");
                OLC_MODE(d) = ZEDIT_ZONE_BUILDERS;
            }
            break;
        case 'l':
        case 'L':
            /*
             * Edit zone lifespan.
             */
            d->sendText("Enter new zone lifespan : ");
            OLC_MODE(d) = ZEDIT_ZONE_LIFE;
            break;
        case 'r':
        case 'R':
            /*
             * Edit zone reset mode.
             */
            d->sendText("\r\n"
                        "0) Never reset\r\n"
                        "1) Reset only when no players in zone\r\n"
                        "2) Normal reset\r\n"
                        "Enter new zone reset type : ");
            OLC_MODE(d) = ZEDIT_ZONE_RESET;
            break;
        case 'f':
        case 'F':
            zedit_disp_flag_menu(d);
            break;
        case 'Z':
        case 'z':
            search_replace(arg, "z ", "");
            do_wiznet(d->character, arg, 0, 0);
            break;
        default:
            zedit_disp_menu(d);
            break;
        }
        break;
        /* End of ZEDIT_MAIN_MENU */
        /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_NAME:
        /*
         * Add new name and return to main menu.
         */
        if (genolc_checkstring(d, arg))
        {
            OLC_ZONE(d)->name = arg;
            OLC_ZONE(d)->number = 1;
        }
        zedit_disp_menu(d);
        break;

        /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_BUILDERS:
        /*
         * Add new builders list and return to main menu.
         */
        if (genolc_checkstring(d, arg))
        {
            OLC_ZONE(d)->builders = arg;
            OLC_ZONE(d)->number = 1;
        }
        zedit_disp_menu(d);
        break;

        /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_RESET:
        /*
         * Parse and add new reset_mode and return to main menu.
         */
        pos = atoi(arg);
        if (!isdigit(*arg) || pos < 0 || pos > 2)
            d->sendText("Try again (0-2) : ");
        else
        {
            OLC_ZONE(d)->reset_mode = pos;
            OLC_ZONE(d)->number = 1;
            zedit_disp_menu(d);
        }
        break;

        /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_LIFE:
        /*
         * Parse and add new lifespan and return to main menu.
         */
        pos = atoi(arg);
        if (!isdigit(*arg) || pos < 0 || pos > 240)
            d->sendText("Try again (0-240) : ");
        else
        {
            OLC_ZONE(d)->lifespan = pos;
            OLC_ZONE(d)->number = 1;
            zedit_disp_menu(d);
        }
        break;

        /*-------------------------------------------------------------------*/

        /*-------------------------------------------------------------------*/
    case ZEDIT_ZONE_FLAGS:
        number = atoi(arg);
        if (number < 0 || number > NUM_ZONE_FLAGS)
        {
            d->sendText("That is not a valid choice!\r\n");
            zedit_disp_flag_menu(d);
        }
        else if (number == 0)
        {
            zedit_disp_menu(d);
            break;
        }
        else
        {
            /*
             * Toggle the bit.
             */
            OLC_ZONE(d)->zone_flags.toggle(number - 1);
            OLC_ZONE(d)->number = 1;
            zedit_disp_flag_menu(d);
        }
        return;

        /*-------------------------------------------------------------------*/

    default:
        /*
         * We should never get here, but just in case...
         */
        cleanup_olc(d, CLEANUP_ALL);
        mudlog(BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: zedit_parse(): Reached default case!");
        d->sendText("Oops...\r\n");
        break;
    }
}

/******************************************************************************/
/** End of parse_zedit()                                                     **/
/******************************************************************************/
