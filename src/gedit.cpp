/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  gedit.c:  Olc written for shoplike guildmasters, code by             *
 *             Jason Goodwin                                             *
 *    Made for Circle3.0 bpl11, its copyright applies                    *
 *                                                                       *
 *  Made for Oasis OLC                                                   *
 *  Copyright 1996 Harvey Gilpin.                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "dbat/gedit.h"
#include "dbat/genzon.h"
#include "dbat/act.informative.h"
#include "dbat/oasis.h"
#include "dbat/gengld.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/interpreter.h"
#include "dbat/spells.h"
#include "dbat/feats.h"
#include "dbat/genolc.h"
#include "dbat/shop.h"

/*
 * Should check more things.
 */
void gedit_save_internally(struct descriptor_data *d) {
    OLC_GUILD(d)->vnum = OLC_NUM(d);
    add_guild(OLC_GUILD(d));
}

void gedit_save_to_disk(int num) {
    save_guilds(num);
}


/*-------------------------------------------------------------------*\
  utility functions 
 \*-------------------------------------------------------------------*/

ACMD(do_oasis_gedit) {
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

    if (!*buf1) {
        send_to_char(ch, "Specify a guild VNUM to edit.\r\n");
        return;
    } else if (!isdigit(*buf1)) {
        if (strcasecmp("save", buf1) != 0) {
            send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
            return;
        }

        save = true;

        if (is_number(buf2))
            number = atoi(buf2);
        else if (GET_OLC_ZONE(ch) > 0) {
            zone_rnum zlok;

            if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
                number = NOWHERE;
            else
                number = genolc_zone_bottom(zlok);
        }

        if (number == NOWHERE) {
            send_to_char(ch, "Save which zone?\r\n");
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
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) == CON_GEDIT) {
            if (d->olc && OLC_NUM(d) == number) {
                send_to_char(ch, "That guild is currently being edited by %s.\r\n",
                             PERS(d->character, ch));
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
    if (d->olc) {
        mudlog(BRF, ADMLVL_IMMORT, true,
               "SYSERR: do_oasis_gedit: Player already had olc structure.");
        free(d->olc);
    }

    CREATE(d->olc, struct oasis_olc_data, 1);

    /****************************************************************************/
    /** Find the zone.                                                         **/
    /****************************************************************************/
    if ((OLC_ZNUM(d) = real_zone_by_thing(number)) == NOWHERE) {
        send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
        free(d->olc);
        d->olc = nullptr;
        return;
    }

    /****************************************************************************/
    /** Everyone but IMPLs can only edit zones they have been assigned.        **/
    /****************************************************************************/
    if (!can_edit_zone(ch, OLC_ZNUM(d))) {
        send_cannot_edit(ch, zone_table[OLC_ZNUM(d)].number);

        /**************************************************************************/
        /** Free the OLC structure.                                              **/
        /**************************************************************************/
        free(d->olc);
        d->olc = nullptr;
        return;
    }

    if (save) {
        send_to_char(ch, "Saving all guilds in zone %d.\r\n",
                     zone_table[OLC_ZNUM(d)].number);
        mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), true,
               "OLC: %s saves guild info for zone %d.",
               GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);

        /**************************************************************************/
        /** Save the guild to the guild file.                                    **/
        /**************************************************************************/
        gedit_save_to_disk(OLC_ZNUM(d));

        /**************************************************************************/
        /** Free the OLC structure.                                              **/
        /**************************************************************************/
        free(d->olc);
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
    SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

    mudlog(BRF, ADMLVL_IMMORT, true, "OLC: %s starts editing zone %d allowed zone %d",
           GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

void gedit_setup_new(struct descriptor_data *d) {
    auto guilddata = new guild_data();

    /*. Some default strings . */
    G_NO_SKILL(guilddata) = strdup("%s Sorry, but I don't know that one.");
    G_NO_GOLD(guilddata) = strdup("%s Sorry, but I'm gonna need more zenni first.");

    OLC_GUILD(d) = guilddata;
    gedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void gedit_setup_existing(struct descriptor_data *d, int rgm_num) {
    /*. Alloc some guild shaped space . */
    OLC_GUILD(d) = new guild_data();
    copy_guild(OLC_GUILD(d), &guild_index[rgm_num]);
    gedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/**************************************************************************
 Menu functions 
**************************************************************************/

/*-------------------------------------------------------------------*/

void gedit_select_skills_menu(struct descriptor_data *d) {
    int i, j = 0, found = 0;
    auto guilddata = OLC_GUILD(d);
    clear_screen(d);

    write_to_output(d, "Skills known:\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE; i++) {
        if (spell_info[i].skilltype == SKTYPE_SKILL &&
            strcmp(spell_info[i].name, "!UNUSED!")) {
            write_to_output(d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                            YESNO(guilddata->skills.count(i)), i, spell_info[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3)) {
            found = 0;
            write_to_output(d, "\r\n");
        }
    }
    write_to_output(d, "\r\nEnter skill num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_SKILLS;
}

/*-------------------------------------------------------------------*/

void gedit_select_spells_menu(struct descriptor_data *d) {
    int i, j = 0, found = 0;
    struct guild_data *guilddata;

    guilddata = OLC_GUILD(d);
    clear_screen(d);

    write_to_output(d, "Spells known:\r\n");

    for (i = 0; i <= SKILL_TABLE_SIZE; i++) {
        if (IS_SET(spell_info[i].skilltype, SKTYPE_SPELL) &&
            strcmp(spell_info[i].name, "!UNUSED!")) {
            write_to_output(d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                            YESNO(guilddata->skills.count(i)), i, spell_info[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3)) {
            found = 0;
            write_to_output(d, "\r\n");
        }
    }
    write_to_output(d, "\r\nEnter spell num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_SPELLS;
}

/*-------------------------------------------------------------------*/

void gedit_select_feats_menu(struct descriptor_data *d) {
    int i, j = 0, found = 0;
    auto guilddata = OLC_GUILD(d);
    clear_screen(d);

    write_to_output(d, "Feats known:\r\n");

    for (i = 0; i <= NUM_FEATS_DEFINED; i++) {
        if (feat_list[i].in_game) {
            write_to_output(d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                            YESNO(guilddata->feats.count(i)), i, feat_list[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3)) {
            found = 0;
            write_to_output(d, "\r\n");
        }
    }
    write_to_output(d, "\r\nEnter feat num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_FEATS;
}

/*-------------------------------------------------------------------*/

void gedit_select_lang_menu(struct descriptor_data *d) {
    int i, j = 0, found = 0;
    auto guilddata = OLC_GUILD(d);
    clear_screen(d);

    write_to_output(d, "Skills known:\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE; i++) {
        if (IS_SET(spell_info[i].skilltype, SKTYPE_LANG) &&
            strcmp(spell_info[i].name, "!UNUSED!")) {
            write_to_output(d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                            YESNO(guilddata->skills.count(i)), i, spell_info[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3)) {
            found = 0;
            write_to_output(d, "\r\n");
        }
    }
    write_to_output(d, "\r\nEnter skill num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_LANGS;
}

/*-------------------------------------------------------------------*/

void gedit_select_wp_menu(struct descriptor_data *d) {
    int i, j = 0, found = 0;
    auto guilddata = OLC_GUILD(d);
    clear_screen(d);

    write_to_output(d, "Skills known:\r\n");

    for (i = 0; i < SKILL_TABLE_SIZE; i++) {
        if (IS_SET(spell_info[i].skilltype, SKTYPE_WEAPON) &&
            strcmp(spell_info[i].name, "!UNUSED!")) {
            write_to_output(d, "@n[@c%-3s@n] %-3d %-20.20s  ",
                            YESNO(guilddata->skills.count(i)), i, spell_info[i].name);
            j++;
            found = 1;
        }
        if (found && !(j % 3)) {
            found = 0;
            write_to_output(d, "\r\n");
        }
    }
    write_to_output(d, "\r\nEnter skill num, 0 to quit:  ");

    OLC_MODE(d) = GEDIT_SELECT_WPS;
}

/*-------------------------------------------------------------------*/

void gedit_no_train_menu(struct descriptor_data *d) {
    char bits[MAX_STRING_LENGTH];
    int i, count = 0;
    struct guild_data *guilddata;

    guilddata = OLC_GUILD(d);
    clear_screen(d);

    for (i = 0; i < NUM_TRADERS; i++) {
        write_to_output(d, "@g%2d@n) %-20.20s   %s", i + 1, trade_letters[i],
                        !(++count % 2) ? "\r\n" : "");
    }

    sprintbitarray(G_WITH_WHO(guilddata), trade_letters, sizeof(bits), bits);
    write_to_output(d, "\r\nCurrent train flags: @c%s@n\r\n"
                       "Enter choice, 0 to quit : ", bits);
    OLC_MODE(d) = GEDIT_NO_TRAIN;
}

/*-------------------------------------------------------------------*/
/*. Display main menu . */

void gedit_disp_menu(struct descriptor_data *d) {
    struct guild_data *guilddata;
    char buf1[MAX_STRING_LENGTH];

    guilddata = OLC_GUILD(d);

    clear_screen(d);

    sprintbitarray(G_WITH_WHO(guilddata), trade_letters, sizeof(buf1), buf1);

    write_to_output(d,
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
                    G_TRAINER(guilddata) == NOBODY ? -1 : mob_index[G_TRAINER(guilddata)].vn,
                    G_TRAINER(guilddata) == NOBODY ? "None" : mob_proto[G_TRAINER(guilddata)].short_description,
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

void gedit_parse(struct descriptor_data *d, char *arg) {
    int i;

    if (OLC_MODE(d) > GEDIT_NUMERICAL_RESPONSE) {
        if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
            write_to_output(d, "Field must be numerical, try again : ");
            return;
        }
    }
    switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
        case GEDIT_CONFIRM_SAVESTRING:
            switch (*arg) {
                case 'y':
                case 'Y':
                    send_to_char(d->character, "Saving Guild to memory.\r\n");
                    gedit_save_internally(d);
                    mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(d->character)), true,
                           "OLC: %s edits guild %d", GET_NAME(d->character), OLC_NUM(d));
                    if (CONFIG_OLC_SAVE) {
                        gedit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
                        write_to_output(d, "Guild %d saved to disk.\r\n", OLC_NUM(d));
                    } else
                        write_to_output(d, "Guild %d saved to memory.\r\n", OLC_NUM(d));
                    cleanup_olc(d, CLEANUP_STRUCTS);
                    return;
                case 'n':
                case 'N':
                    cleanup_olc(d, CLEANUP_ALL);
                    return;
                default:
                    write_to_output(d, "Invalid choice!\r\nDo you wish to save the guild? : ");
                    return;
            }
            break;

/*-------------------------------------------------------------------*/
        case GEDIT_MAIN_MENU:
            i = 0;
            switch (*arg) {
                case 'q':
                case 'Q':
                    if (OLC_VAL(d)) {        /*. Anything been changed? . */
                        write_to_output(d, "Do you wish to save the changes to the Guild? (y/n) : ");
                        OLC_MODE(d) = GEDIT_CONFIRM_SAVESTRING;
                    } else
                        cleanup_olc(d, CLEANUP_ALL);
                    return;
                case '0':
                    OLC_MODE(d) = GEDIT_TRAINER;
                    write_to_output(d, "Enter vnum of guild master : ");
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
                    write_to_output(d, "When does this shop open (a day has 28 hours) ? ");
                    i++;
                    break;
                case '4':
                    OLC_MODE(d) = GEDIT_CLOSE;
                    write_to_output(d, "When does this shop close (a day has 28 hours) ? ");
                    i++;
                    break;
                case '5':
                    OLC_MODE(d) = GEDIT_CHARGE;
                    i++;
                    break;
                case '6':
                    OLC_MODE(d) = GEDIT_MINLVL;
                    write_to_output(d, "Minumum Level will Train: ");
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
                write_to_output(d, "\r\nEnter new value : ");
            else if (i == -1)
                write_to_output(d, "\r\nEnter new text :\r\n] ");
            else
                write_to_output(d, "Oops...\r\n");
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
            if (isdigit(*arg)) {
                i = atoi(arg);
                if ((i = atoi(arg)) != -1)
                    if ((i = real_mobile(i)) == NOBODY) {
                        write_to_output(d, "That mobile does not exist, try again : ");
                        return;
                    }
                G_TRAINER(OLC_GUILD(d)) = i;
                if (i == -1)
                    break;
                /*. Fiddle with special procs . */
                G_FUNC(OLC_GUILD(d)) = mob_index[i].func != guild ? mob_index[i].func : nullptr;
                mob_index[i].func = guild;
                break;
            } else {
                write_to_output(d, "Invalid response.\r\n");
                gedit_disp_menu(d);
                return;
            }
        case GEDIT_OPEN:
            G_OPEN(OLC_GUILD(d)) = LIMIT(atoi(arg), 0, 28);
            break;
        case GEDIT_CLOSE:
            G_CLOSE(OLC_GUILD(d)) = LIMIT(atoi(arg), 0, 28);;
            break;
        case GEDIT_CHARGE:
            sscanf(arg, "%f", &G_CHARGE(OLC_GUILD(d)));
            break;
        case GEDIT_NO_TRAIN:
            if ((i = LIMIT(atoi(arg), 0, NUM_TRADERS - 1)) > 0) {
                TOGGLE_BIT_AR(G_WITH_WHO(OLC_GUILD(d)), i - 1);
                gedit_no_train_menu(d);
                return;
            }
            break;

        case GEDIT_MINLVL:
            G_MINLVL(OLC_GUILD(d)) = MAX(atoi(arg), 0);
            break;

        case GEDIT_SELECT_SPELLS:
            i = atoi(arg);
            if (i == 0)
                break;
            i = MAX(1, MIN(i, SKILL_TABLE_SIZE));
            OLC_GUILD(d)->toggle_skill(i);
            gedit_select_spells_menu(d);
            return;

        case GEDIT_SELECT_FEATS:
            i = atoi(arg);
            if (i == 0)
                break;
            i = MAX(1, MIN(i, NUM_FEATS_DEFINED));
            OLC_GUILD(d)->toggle_feat(i);
            gedit_select_feats_menu(d);
            return;

        case GEDIT_SELECT_SKILLS:
            i = atoi(arg);
            if (i == 0)
                break;
            i = MAX(1, MIN(i, SKILL_TABLE_SIZE));
            OLC_GUILD(d)->toggle_skill(i);
            gedit_select_skills_menu(d);
            return;

        case GEDIT_SELECT_WPS:
            i = atoi(arg);
            if (i == 0)
                break;
            i = MAX(1, MIN(i, SKILL_TABLE_SIZE));
            OLC_GUILD(d)->toggle_feat(i);
            gedit_select_wp_menu(d);
            return;

        case GEDIT_SELECT_LANGS:
            i = atoi(arg);
            if (i == 0)
                break;
            i = MAX(1, MIN(i, SKILL_TABLE_SIZE));
            OLC_GUILD(d)->toggle_skill(i);
            gedit_select_lang_menu(d);
            return;

/*-------------------------------------------------------------------*/
        default:
            /*. We should never get here . */
            cleanup_olc(d, CLEANUP_ALL);
            mudlog(BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: gedit_parse(): "
                                              "Reached default case!");
            write_to_output(d, "Oops...\r\n");
            break;
    }
/*-------------------------------------------------------------------*/
/*. END OF CASE 
  If we get here, we have probably changed something, and now want to
  return to main menu.  Use OLC_VAL as a 'has changed' flag . */

    OLC_VAL(d) = 1;
    gedit_disp_menu(d);
}

