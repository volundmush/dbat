/* ************************************************************************
 *   File: act.wizard.c                                  Part of CircleMUD *
 *  Usage: Player-level god commands and other goodies                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <fstream>
#include "dbat/act.wizard.h"
#include "dbat/interpreter.h"
#include "dbat/send.h"
#include "dbat/config.h"
#include "dbat/act.other.h"
#include "dbat/maputils.h"
#include "dbat/dg_comm.h"
#include "dbat/handler.h"
#include "dbat/act.item.h"
#include "dbat/act.informative.h"
#include "dbat/players.h"
#include "dbat/weather.h"
#include "dbat/assemblies.h"
#include "dbat/comm.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"
#include "dbat/races.h"
#include "dbat/class.h"
#include "dbat/spells.h"
#include "dbat/improved-edit.h"
#include "dbat/feats.h"
#include "dbat/fight.h"
#include "dbat/genolc.h"
#include "dbat/screen.h"
#include "dbat/local_limits.h"
#include "dbat/shop.h"
#include "dbat/guild.h"
#include "dbat/spell_parser.h"
#include "dbat/transformation.h"

/* local variables */
static int copyover_timer = 0; /* for timed copyovers */

/* local functions */
static void print_lockout(Character *ch);

static void execute_copyover();

static int perform_set(Character *ch, Character *vict, int mode, char *val_arg);

static void perform_immort_invis(Character *ch, int level);

static void do_stat_room(Character *ch);

static void do_stat_object(Character *ch, Object *j);

static void do_stat_character(Character *ch, Character *k);

static void stop_snooping(Character *ch);

static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall);

static void mob_checkload(Character *ch, mob_vnum mvnum);

static void obj_checkload(Character *ch, obj_vnum ovnum);

static void trg_checkload(Character *ch, trig_vnum tvnum);

// definitions
ACMD(do_lag)
{

    struct descriptor_data *d;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        ch->sendText("Syntax: lag (target) (number of seconds)\r\n");
        return;
    }

    int found = false, num = atoi(arg2);

    if (num <= 0 || num > 5)
    {
        ch->sendText("Keep it between 1 to 5 seconds please.\r\n");
        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (!strcasecmp(GET_NAME(d->character), arg))
        {
            if (GET_ADMLEVEL(d->character) > GET_ADMLEVEL(ch))
            {
                ch->sendText("Sorry, you've been outranked.\r\n");
                return;
            }
            switch (num)
            {
            case 1:
                WAIT_STATE(d->character, PULSE_1SEC);
                break;
            case 2:
                WAIT_STATE(d->character, PULSE_2SEC);
                break;
            case 3:
                WAIT_STATE(d->character, PULSE_3SEC);
                break;
            case 4:
                WAIT_STATE(d->character, PULSE_4SEC);
                break;
            case 5:
                WAIT_STATE(d->character, PULSE_5SEC);
                break;
            }
            found = true;
        }
    }

    if (found == false)
    {
        ch->sendText("That player isn't around.\r\n");
        return;
    }
}

/*Update things that need it. Just space for the moment - Iovan */
void update_space()
{
    FILE *mapfile;
    int rowcounter, colcounter;
    int vnum_read;

    basic_mud_log("Updated Space Map. ");

    // Load the map vnums from a file into an array
    mapfile = fopen(MAP_FILE, "r");

    for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++)
    {
        for (colcounter = 0; colcounter <= MAP_COLS; colcounter++)
        {
            auto res = fscanf(mapfile, "%d", &vnum_read);
            mapnums[rowcounter][colcounter] = real_room(vnum_read);
        }
    }

    fclose(mapfile);
}

/* Read the news entry */
ACMD(do_news)
{

    if (IS_NPC(ch))
    {
        return;
    }

    FILE *fl;
    const char *filename;
    char line[256];
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], title[256], lastline[256];
    int entries = 0, lookup = 0, nr, found = false, first = true, exit = false;

    one_argument(argument, arg);
    filename = NEWS_FILE;

    /* Check the file for the entry so we may edit it if need be*/

    if (!*arg)
    {
        ch->sendText("Syntax: news (number | list)\r\n");
        return;
    }

    lookup = atoi(arg);

    if (!(fl = fopen(filename, "r")))
    {
        basic_mud_log("SYSERR: opening news file for reading");
        return;
    }

    if (lookup > 0)
    {
        while (!feof(fl) && exit == false)
        {
            get_line(fl, line);
            if (*line == '#')
            { /* Count the entries */
                if (sscanf(line, "#%d", &nr) != 1)
                { /* Check the entry */
                    continue;
                }
                else if (nr != lookup)
                {
                    entries++;
                    continue;
                }
                else
                { /* One we want to read */
                    sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
                    sprintf(buf,
                            "@w--------------------------------------------------------------\n@cNum@W: @D(@G%3d@D)                @cTitle@W: @g%-50s@n\n",
                            nr, title);
                    found = true;
                    while (!feof(fl) && exit == false)
                    {
                        get_line(fl, line);
                        if (*line != '#')
                        { /* As long as it isn't a new entry*/
                            if (first == true)
                            {
                                first = false;
                                sprintf(buf + strlen(buf),
                                        "%s\n@w--------------------------------------------------------------\n", line);
                                sprintf(lastline, "%s", line);
                            }
                            else if (!strcasecmp(line, lastline))
                            {
                                continue;
                            }
                            else
                            {
                                sprintf(buf + strlen(buf), "%s\n", line);
                                sprintf(lastline, "%s", line);
                            }
                        }
                        else
                        {
                            exit = true;
                        }
                    } /* End write buffer with entry text */
                } /* End One We wanted to read */
            } /* End Check the entry */
        } /* End of main read while */
        fclose(fl);
    }
    else if (!strcasecmp(arg, "list"))
    {
        while (!feof(fl))
        {
            get_line(fl, line);
            if (*line == '#')
            { /* Count the entries */
                entries++;
                if (sscanf(line, "#%d", &nr) != 1)
                { /* Check the entry */
                    continue;
                }
                else
                { /* One we want to read */
                    if (first == true)
                    {
                        sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
                        sprintf(buf,
                                "@wNews Entries (Newest at the bottom, to read an entry use 'news (number)')\n@D[@cNum@W: @D(@G%3d@D) @cTitle@W: @g%-50s@D]@n\n",
                                nr, title);
                        first = false;
                    }
                    else
                    {
                        sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
                        sprintf(buf + strlen(buf), "@D[@cNum@W: @D(@G%3d@D) @cTitle@W: @g%-50s@D]@n\n", nr, title);
                    }
                }
            } /* End Check the entry */
        } /* End of main listwhile */
        fclose(fl);

        if (entries > 0)
        {
            ch->setBaseStat("last_played", time(nullptr));
            WAIT_STATE(ch, PULSE_1SEC);
            if (ch->desc)
                ch->desc->send_to("%s", buf);
        }
        else
        {
            ch->sendText("The news file is empty right now.\r\n");
        }
        *buf = '\0';
        *title = '\0';
        *lastline = '\0';
        return;
    }
    else
    {
        fclose(fl);
        ch->sendText("Syntax: news (number | list)\r\n");
        return;
    }

    if (found == true)
    {
        ch->send_to("%s\r\n", buf);
        ch->setBaseStat("last_played", time(nullptr));
        *buf = '\0';
        *title = '\0';
        *lastline = '\0';
        WAIT_STATE(ch, PULSE_1SEC);
        return;
    }
    else
    {
        ch->sendText("That news entry does not exist.\r\n");
        return;
    }
}

/* Write the news entry */

ACMD(do_newsedit)
{

    if (GET_ADMLEVEL(ch) < 1 || IS_NPC(ch))
    {
        return;
    }

    FILE *fl;
    const char *filename;
    char line[256];
    int entries = 0, lookup = 0, lastentry = 0, nr;

    filename = NEWS_FILE;

    if (!*argument)
    {
        ch->sendText("Syntax: newsedit (title)\r\n");
        return;
    }
    else if (strlen(argument) > 50)
    {
        ch->sendText("Limit of 50 characters for title.\r\n");
        return;
    }
    else if (strstr(argument, "#"))
    {
        ch->sendText("# is a forbidden character for news entries as it is used by the file system.\r\n");
        return;
    }

    /* Check the file for the entry so we may edit it if need be*/

    if (!(fl = fopen(filename, "r")))
    {
        basic_mud_log("SYSERR: Couldn't open news file for reading");
        return;
    }

    while (!feof(fl))
    {
        get_line(fl, line);
        if (*line == '#')
        { /* Count the entries */
            entries++;
            if (sscanf(line, "#%d", &nr) != 1)
            {
                continue;
            }
            else
            {
                lastentry = nr;
            }
        }
    } /* End of main read while */
    fclose(fl);

    /* Time to write the entry */
    struct
    {
        const char *cmd;
        char level;
        char **buffer;
        int size;
        char *filename;
    } fields[] = {
        /* edit the lvls to your own needs */
        {"news", ADMLVL_IMMORT, &immlist, 2000, IMMLIST_FILE},
        {"\n", 0, nullptr, 0, nullptr}};

    char *tmstr;
    time_t mytime = time(nullptr);
    tmstr = (char *)asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    if (lastentry == 0)
    {
        if (entries == 0)
        {
            lookup = 1;
        }
        else
        { /* Uh oh */
            send_to_imm("ERROR: News file entries are disorganized. Report to Iovan for analysis.\r\n");
            return;
        }
    }
    else
    {
        lookup = lastentry + 1;
    }
    char *backstr = nullptr;
    act("$n begins to edit the news.", true, ch, nullptr, nullptr, TO_ROOM);
    ch->sendText("@D----------------------=[@GNews Edit@D]=----------------------@n\n");
    ch->sendText(" @RRemember that using # in newsedit is not possible. That\n");
    ch->sendText("character will be eaten because it is required for the news\n");
    ch->sendText("file as a delimiter. Also if you want to create an empty line\n");
    ch->sendText("between paragraphs you will need to enter a single space and\n");
    ch->sendText("not just push enter. Happy editing!@n\n");
    ch->sendText("@D---------------------------------------------------------@n\n");
    send_editor_help(ch->desc);
    skip_spaces(&argument);
    ch->desc->newsbuf = strdup(argument);
    TOP_OF_NEWS = lookup;
    LASTNEWS = lookup;
    string_write(ch->desc, fields[0].buffer, 2000, 0, backstr);
    STATE(ch->desc) = CON_NEWSEDIT;
}

static void print_lockout(Character *ch)
{
    if (IS_NPC(ch))
        return;

    FILE *file;
    char fname[40], filler[50], line[256], buf[MAX_STRING_LENGTH * 4];
    int count = 0, first = true;

    /* Read Introduction File */
    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout"))
    {
        ch->sendText("The lockout file does not exist.");
        return;
    }
    else if (!(file = fopen(fname, "r")))
    {
        ch->sendText("The lockout file does not exist.");
        return;
    }
    sprintf(buf, "@b------------------[ @RLOCKOUT @b]------------------@n\n");
    while (!feof(file))
    {
        get_line(file, line);
        sscanf(line, "%s\n", filler);
        if (first != true && strstr(buf, filler) == nullptr)
        {
            if (count == 0)
            {
                sprintf(buf + strlen(buf), "%-23s@D|@n", filler);
                count = 1;
            }
            else
            {
                sprintf(buf + strlen(buf), "%-23s\n", filler);
                count = 0;
            }
        }
        else if (first == true)
        {
            sprintf(buf + strlen(buf), "%-23s@D|@n", filler);
            first = false;
            count = 1;
        }
        *filler = '\0';
    }
    if (count == 1)
    {
        sprintf(buf + strlen(buf), "\n");
    }
    sprintf(buf + strlen(buf), "@b------------------[ @RLOCKOUT @b]------------------@n\n");

    if (ch->desc)
        ch->desc->send_to("%s", buf);

    fclose(file);
}

ACMD(do_approve)
{

    char arg[MAX_INPUT_LENGTH];
    Character *vict = nullptr;

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("What player do you want to approve as having an acceptable bio?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
    {
        ch->sendText("That player is not in the game.\r\n");
        return;
    }

    if (PLR_FLAGGED(vict, PLR_BIOGR))
    {
        ch->sendText("They have already been approved. If this was made in error inform Iovan.\r\n");
        return;
    }
    else
    {
        vict->player_flags.set(PLR_BIOGR, true);
        ch->sendText("They have now been approved.\r\n");
        return;
    }
}

ACMD(do_reward)
{
    int amt = 0;

    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2)
    {
        ch->sendText("Syntax: reward (target) (amount)\r\nThis is either a positive number or a negative.\r\n");
        return;
    }

    auto vict = findPlayer(arg);

    if (!vict)
    {
        ch->sendText("That is not a recognised player character.\r\n");
        return;
    }

    amt = atoi(arg2);

    if (amt == 0)
    {
        ch->sendText("That is pointless don't you think? Try an amount higher than 0.\r\n");
        return;
    }

    if (amt > 0)
    {
        ch->send_to("@WYou award @C%s @D(@G%d@D)@W RP points.@n\r\n", GET_NAME(vict), amt);
        vict->send_to("@D[@YROLEPLAY@D] @WYou have been awarded @D(@G%d@D)@W RP points by @C%s@W.@n\r\n", amt, GET_NAME(ch));
        send_to_imm("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, GET_NAME(ch));
        log_imm_action("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, GET_NAME(ch));
        vict->modRPP(amt);
    }
    else
    {
        ch->send_to("@WYou deduct @D(@G%d@D)@W RP points from @C%s@W.@n\r\n", amt, GET_NAME(vict));
        vict->send_to("@D[@YROLEPLAY@D] @C%s@W deducts @D(@G%d@D)@W RP points from you.@n\r\n", GET_NAME(ch), amt);
        send_to_imm("ROLEPLAY: %s has had %d RP points deducted by %s.", GET_NAME(vict), amt, GET_NAME(ch));
        log_imm_action("ROLEPLAY: %s has had %d RP points deducted by %s.", GET_NAME(vict), amt, GET_NAME(ch));
        vict->modRPP(amt);
    }
}

ACMD(do_permission)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg)
    {
        ch->sendText("You want to @Grestrict@n or @Gunrestrict@n?\r\n");
        return;
    }

    if (!*arg2 && !strcasecmp("unrestrict", arg))
    {
        ch->sendText("You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
        return;
    }
    if (!strcasecmp("unrestrict", arg))
    {
        if (!strcasecmp("saiyan", arg2))
        {
            ch->sendText("You have unrestricted saiyans for the very next character creation.\r\n");
            send_to_imm("PERMISSION: %s unrestricted saiyans.", GET_NAME(ch));
            SAIYAN_ALLOWED = true;
        }
        else if (!strcasecmp("majin", arg2))
        {
            ch->sendText("You have unrestricted majins for the very next character creation.\r\n");
            send_to_imm("PERMISSION: %s unrestricted majins.", GET_NAME(ch));
            MAJIN_ALLOWED = true;
        }
        else
        {
            ch->sendText("You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
            return;
        }
    }
    else if (!strcasecmp("restrict", arg))
    {
        ch->sendText("You have restricted character creation to standard race slection.\r\n");
        send_to_imm("PERMISSION: %s restricted races again.", GET_NAME(ch));
        MAJIN_ALLOWED = false;
    }
    else
    {
        ch->sendText("You want to @Grestrict@n or @Gunrestrict@n?\r\n");
        return;
    }
}

ACMD(do_transobj)
{

    Object *obj;
    Character *vict;
    struct descriptor_data *d;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!IS_NPC(ch) && GET_ADMLEVEL(ch) < 1)
    {
        ch->sendText("Huh!?");
        return;
    }

    if (!*arg || !*arg2)
    {
        ch->sendText("Syntax: transo (object) (target)\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory())))
    {
        ch->sendText("You want to send what?\r\n");
        return;
    }
    else if (!strcasecmp("all", arg2))
    {
        int num = GET_OBJ_VNUM(obj);
        Object *obj2 = nullptr;

        act("You send $p to everyone in the game.", true, ch, obj, nullptr, TO_CHAR);

        for (d = descriptor_list; d; d = d->next)
        {
            if (IS_NPC(d->character))
                continue;
            else if (!IS_PLAYING(d))
                continue;
            else if (d->character == ch)
                continue;
            else
            {
                act("$N sends $p across the universe to you.", true, d->character, obj, ch, TO_CHAR);
                obj2 = read_object(num, VIRTUAL);
                d->character->addToInventory(obj2);
            }
        }
    }
    else if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_WORLD)))
    {
        ch->sendText("That player is not in the game.\r\n");
        return;
    }
    else
    {
        act("You send $p to $N.", true, ch, obj, vict, TO_CHAR);
        act("$n sends $p across the universe to you.", true, ch, obj, vict, TO_VICT);
        obj->clearLocation();
        vict->addToInventory(obj);
        return;
    }
}

void search_replace(char *string, const char *find, const char *replace)
{
    char final[MAX_STRING_LENGTH], temp[2];
    size_t start, end, i;

    while (strstr(string, find))
    {

        final[0] = '\0';
        start = strstr(string, find) - string;
        end = start + strlen(find);

        temp[1] = '\0';

        strncat(final, string, start);

        strcat(final, replace);

        for (i = end; string[i] != '\0'; i++)
        {
            temp[0] = string[i];
            strcat(final, temp);
        }

        sprintf(string, "%s", final);
    }
    return;
}

ACMD(do_interest)
{
    if (GET_ADMLEVEL(ch) < 5)
    {
        ch->sendText("Huh!?\r\n");
        return;
    }
    else
    {
        if (INTERESTTIME > 0)
        {
            char *tmstr;
            tmstr = (char *)asctime(localtime(&INTERESTTIME));
            *(tmstr + strlen(tmstr) - 1) = '\0';
            ch->send_to("INTEREST TIME: [%s]\r\n", tmstr);
            return;
        }
        ch->sendText("Interest time has been initiated!\r\n");
        INTERESTTIME = time(nullptr) + 86400;
        LASTINTEREST = time(nullptr) + 86400;
        return;
    }
}

/* do_finddoor, finds the door(s) that a key goes to */
ACMD(do_finddoor)
{
    int d, vnum = NOTHING, num = 0;
    size_t len, nlen;
    room_rnum i;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH] = {0};
    Character *tmp_char;
    Object *obj;
    std::string sdesc;

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Format: finddoor <obj/vnum>\r\n");
        return;
    }

    if (is_number(arg))
    {
        vnum = atoi(arg);
        if (!obj_proto.contains(vnum))
        {
            ch->sendText("There is no object with that vnum.\r\n");
            return;
        }
        auto &o = obj_proto.at(vnum);
        sdesc = o.short_description;
    }
    else
    {
        generic_find(arg,
                     FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_WORLD | FIND_OBJ_EQUIP,
                     ch, &tmp_char, &obj);
        if (!obj)
            ch->sendText("What key do you want to find a door for?\r\n");
        else
            vnum = GET_OBJ_VNUM(obj);
        sdesc = obj->getShortDescription();
    }

    if (vnum != NOTHING)
    {
        len = snprintf(buf, sizeof(buf), "Doors unlocked by key [%d] %s are:\r\n",
                       vnum, sdesc.c_str());
        for (auto &[vn, r] : world)
        {
            for (auto &[d, e] : r->getDirections())
            {
                if (e.key == vnum)
                {
                    nlen = snprintf(buf + len, sizeof(buf) - len,
                                    "[%3d] Room %d, %s (%s)\r\n",
                                    ++num, vn,
                                    dirs[static_cast<int>(d)], e.keyword.c_str());
                    if (len + nlen >= sizeof(buf) || nlen < 0)
                        break;
                    len += nlen;
                }
            } /* for all directions */
        } /* for all rooms */
        if (num > 0)
        {
            if (ch->desc)
                ch->desc->send_to("%s", buf);
        }
        else
            ch->send_to("No doors were found for key [%d] %s.\r\n", vnum, sdesc);
    }
}

ACMD(do_recall)
{
    if (GET_ADMLEVEL(ch) < 1)
    {
        ch->sendText("You are not an immortal!\r\n");
    }
    else
    {
        ch->sendText("You disappear in a burst of light!\r\n");
        act("$n disappears in a burst of light!", false, ch, nullptr, nullptr, TO_ROOM);
        if (real_room(2) != NOWHERE)
        {
            ch->leaveLocation();
            ch->moveToLocation(2);
            ch->lookAtLocation();
            ch->setBaseStat("load_room", ch->location.getVnum());
        }
    }
}

ACMD(do_hell)
{
    Character *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg)
    {
        ch->sendText("Syntax: lockout (character)\n"
                     "        lockout list\r\n");
        return;
    }

    if (!strcasecmp(arg, "Iovan") || !strcasecmp(arg, "iovan") || !strcasecmp(arg, "Fahl") ||
        !strcasecmp(arg, "fahl") || !strcasecmp(arg, "Xyron") || !strcasecmp(arg, "xyron") ||
        !strcasecmp(arg, "Samael") || !strcasecmp(arg, "samael"))
    {
        ch->sendText("What are you smoking? You can't lockout senior imms.\r\n");
        return;
    }

    if (!strcasecmp(arg, "list"))
    {
        print_lockout(ch);
        return;
    }

    // TODO: Fix this command.

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
    {
        // lockWrite(ch, arg);
        return;
    }
    else
    {
        struct descriptor_data *d = vict->desc;
        extract_char(vict);
        // lockWrite(ch, GET_NAME(vict));
        if (d && STATE(d) != CON_PLAYING)
        {
            STATE(d) = CON_CLOSE;
            vict->desc->character = nullptr;
            vict->desc = nullptr;
        }
        return;
    }
    return;
}

ACMD(do_echo)
{
    skip_spaces(&argument);
    bool NoName = false;

    if (!*argument)
        ch->sendText("Yes.. but what?\r\n");
    else
    {
        char buf[8096 * 4];
        char name[128];
        int found = false, trunc = 0;
        Character *vict = nullptr, *next_v = nullptr, *tch = nullptr;
        auto truncAt = (8096 * 4 - 1000);

        if (strlen(argument) > truncAt)
        {
            trunc = strlen(argument) - truncAt;
            argument[strlen(argument) - trunc] = '\0';
            sprintf(argument, "%s\n@D(@gMessage truncated to %d characters@D)@n\n", argument, truncAt);
        }
        auto people = ch->location.getPeople();
        for (auto vict : filter_raw(people))
        {
            if (vict == ch)
                continue;
            if (found == false)
            {
                sprintf(name, "*%s", GET_NAME(vict));
                if (strstr(argument, CAP(name)))
                {
                    found = true;
                    tch = vict;
                }
                if (found == false && !IS_NPC(vict))
                {
                    if (readIntro(ch, vict) == 1)
                    {
                        sprintf(name, "*%s", get_i_name(ch, vict));
                        if (strstr(argument, CAP(name)))
                        {
                            found = true;
                            tch = vict;
                        }
                    }
                }
            }
        }

        if (subcmd == SCMD_SMOTE)
        {
            if (!strstr(argument, "#"))
            {
                NoName = true;
            }
            strlcpy(buf, argument, sizeof(buf));
            search_replace(buf, "#", "$n");
            search_replace(buf, "&1", "'@C");
            search_replace(buf, "&2", "@w'");
            search_replace(buf, "&n", "\n");
            search_replace(buf, "&r", "\r");
            if (found == true)
            {
                search_replace(buf, name, "$N");
            }
            else if (strstr(buf, "*"))
            {
                search_replace(buf, "*", "");
            }
        }
        else if (subcmd == SCMD_EMOTE)
        {
            snprintf(buf, sizeof(buf), "$n %s", argument);
            search_replace(buf, "#", "$n");
            search_replace(buf, "&1", "'@C");
            search_replace(buf, "&2", "@w'");
            search_replace(buf, "&n", "\n");
            search_replace(buf, "&r", "\r");
            if (found == true)
            {
                search_replace(buf, name, "$N");
            }
        }
        else
        {
            snprintf(buf, sizeof(buf), "%s", argument);
        }
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
        {
            ch->send_to("%s", CONFIG_OK);
        }
        if (NoName == true)
        {
            char blom[MAX_INPUT_LENGTH];
            sprintf(blom, "@D(@GOOC@W: @gSmote by user %s@D)@n",
                    IS_NPC(ch) ? GET_NAME(ch) : (ch->desc->account == nullptr ? "ERROR REPORT" : ch->desc->account->name.c_str()));
            act(blom, false, ch, nullptr, nullptr, TO_ROOM);
        }
        act("\n\n", false, ch, nullptr, nullptr, TO_CHAR);
        act("\n\n", false, ch, nullptr, nullptr, TO_ROOM);
        if (found == false)
        {
            act(buf, false, ch, nullptr, nullptr, TO_CHAR);
            act(buf, false, ch, nullptr, nullptr, TO_ROOM);
        }
        else
        {

            act(buf, false, ch, nullptr, tch, TO_CHAR);
            act(buf, false, ch, nullptr, tch, TO_NOTVICT);
            search_replace(buf, "$N", "you");
            act(buf, false, ch, nullptr, tch, TO_VICT);
        }
        act("\n\n", false, ch, nullptr, nullptr, TO_CHAR);
        act("\n\n", false, ch, nullptr, nullptr, TO_ROOM);
    }
}

ACMD(do_send)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    Character *vict;

    half_chop(argument, arg, buf);

    if (!*arg)
    {
        ch->sendText("Send what to who?\r\n");
        return;
    }
    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
    {
        ch->send_to("%s", CONFIG_NOPERSON);
        return;
    }
    vict->send_to("%s\r\n", buf);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        ch->sendText("Sent.\r\n");
    else
        ch->send_to("You send '%s' to %s.\r\n", buf, GET_NAME(vict));
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(Character *ch, char *rawroomstr)
{
    room_rnum location = NOWHERE;
    char roomstr[MAX_INPUT_LENGTH];
    Room *rm;

    one_argument(rawroomstr, roomstr);

    if (!*roomstr)
    {
        ch->sendText("You must supply a room number or name.\r\n");
        return (NOWHERE);
    }

    if (isdigit(*roomstr) && !strchr(roomstr, '.'))
    {
        if ((location = real_room((room_vnum)atoi(roomstr))) == NOWHERE)
        {
            ch->sendText("No room exists with that number.\r\n");
            return (NOWHERE);
        }
    }
    else
    {
        Character *target_mob;
        Object *target_obj;
        char *mobobjstr = roomstr;

        auto num = get_number(&mobobjstr);
        if ((target_mob = get_char_vis(ch, mobobjstr, &num, FIND_CHAR_WORLD)))
        {
            if (!target_mob->location)
            {
                ch->sendText("That character is currently lost.\r\n");
                return (NOWHERE);
            }
            location = target_mob->location.getVnum();
        }
        else if ((target_obj = get_obj_vis(ch, mobobjstr, &num)))
        {
            auto loc = target_obj->getAbsoluteRoom();

            if (!loc)
            {
                ch->sendText("That object is currently not in a room.\r\n");
                return (NOWHERE);
            }
            location = loc->getVnum();
        }

        if (location == NOWHERE)
        {
            ch->sendText("Nothing exists by that name.\r\n");
            return (NOWHERE);
        }
    }

    /* a location has been found -- if you're >= GRGOD, no restrictions. */
    if (GET_ADMLEVEL(ch) >= ADMLVL_VICE)
        return (location);

    rm = get_room(location);

    if ((!can_edit_zone(ch, rm->zone->number) && GET_ADMLEVEL(ch) < ADMLVL_GOD) && rm->zone->zone_flags.get(ZONE_QUEST))
    {
        ch->sendText("This target is in a quest zone.\r\n");
        return (NOWHERE);
    }

    if ((GET_ADMLEVEL(ch) < ADMLVL_VICE) && rm->zone->zone_flags.get(ZONE_NOIMMORT))
    {
        ch->sendText("This target is in a zone closed to all.\r\n");
        return (NOWHERE);
    }

    if (ROOM_FLAGGED(location, ROOM_GODROOM))
        ch->sendText("You are not godly enough to use that room!\r\n");
    else
        return (location);

    return (NOWHERE);
}

ACMD(do_at)
{
    room_vnum location;
    char command[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];

    half_chop(argument, buf, command);
    if (!*buf)
    {
        ch->sendText("You must supply a room number or a name.\r\n");
        return;
    }

    if (!*command)
    {
        ch->sendText("What do you want to do there?\r\n");
        return;
    }

    if ((location = find_target_room(ch, buf)) == NOWHERE)
        return;

    /* a location has been found. */
    auto original_loc = ch->location;
    ch->leaveLocation();
    ch->moveToLocation(location);
    command_interpreter(ch, command);

    /* check if the char is still there */
    if (ch->location == location)
    {
        ch->leaveLocation();
        ch->moveToLocation(original_loc);
    }
}

ACMD(do_goto)
{
    char buf[MAX_STRING_LENGTH];

    Location loc;

    if(argument && boost::icontains(argument, ":")) {
        loc = resolveLocID(argument);
        if(!loc) {
            ch->sendText("No location exists with that LocID.\r\n");
            return;
        }
    } else if (auto location = find_target_room(ch, argument); location != NOWHERE) {
        loc = Location(location);
    } else
        return;
    
    if (PLR_FLAGGED(ch, PLR_HEALT))
    {
        ch->sendText("You are inside a healing tank!\r\n");
        return;
    }

    snprintf(buf, sizeof(buf), "$n %s", POOFOUT(ch) ? POOFOUT(ch) : "disappears in a puff of smoke.");
    act(buf, true, ch, nullptr, nullptr, TO_ROOM);

    ch->leaveLocation();
    ch->moveToLocation(loc);

    snprintf(buf, sizeof(buf), "$n %s", POOFIN(ch) ? POOFIN(ch) : "appears with an ear-splitting bang.");
    act(buf, true, ch, nullptr, nullptr, TO_ROOM);

    ch->lookAtLocation();
    enter_wtrigger(ch->getRoom(), ch, -1);
}

ACMD(do_trans)
{
    char buf[MAX_INPUT_LENGTH];
    struct descriptor_data *i;
    Character *victim;

    one_argument(argument, buf);
    if (!*buf)
        ch->sendText("Whom do you wish to transfer?\r\n");
    else if (strcasecmp("all", buf))
    {
        if (!(victim = get_char_vis(ch, buf, nullptr, FIND_CHAR_WORLD)))
            ch->send_to("%s", CONFIG_NOPERSON);
        else if (victim == ch)
            ch->sendText("That doesn't make much sense, does it?\r\n");
        else
        {
            if ((GET_ADMLEVEL(ch) < GET_ADMLEVEL(victim)) && !IS_NPC(victim))
            {
                ch->sendText("Go transfer someone your own size.\r\n");
                return;
            }
            if (PLR_FLAGGED(victim, PLR_HEALT))
            {
                ch->sendText("They are inside a healing tank!\r\n");
                return;
            }
            act("$n disappears in a mushroom cloud.", false, victim, nullptr, nullptr, TO_ROOM);
            victim->leaveLocation();
            victim->moveToLocation(ch);
            act("$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
            act("$n has transferred you!", false, ch, nullptr, victim, TO_VICT);
            victim->lookAtLocation();
            enter_wtrigger(victim->getRoom(), victim, -1);
        }
    }
    else
    { /* Trans All */
        if (!ADM_FLAGGED(ch, ADM_TRANSALL))
        {
            ch->sendText("I think not.\r\n");
            return;
        }

        for (i = descriptor_list; i; i = i->next)
            if (STATE(i) == CON_PLAYING && i->character && i->character != ch)
            {
                victim = i->character;
                if (GET_ADMLEVEL(victim) >= GET_ADMLEVEL(ch))
                    continue;
                act("$n disappears in a mushroom cloud.", false, victim, nullptr, nullptr, TO_ROOM);
                victim->leaveLocation();
                victim->moveToLocation(ch);
                act("$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
                act("$n has transferred you!", false, ch, nullptr, victim, TO_VICT);
                victim->lookAtLocation();
                enter_wtrigger(victim->getRoom(), victim, -1);
            }
        ch->send_to("%s", CONFIG_OK);
    }
}

ACMD(do_teleport)
{
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
    Character *victim;
    room_rnum target;

    two_arguments(argument, buf, buf2);

    if (!*buf)
        ch->sendText("Whom do you wish to teleport?\r\n");
    else if (!(victim = get_char_vis(ch, buf, nullptr, FIND_CHAR_WORLD)))
        ch->send_to("%s", CONFIG_NOPERSON);
    else if (victim == ch)
        ch->sendText("Use 'goto' to teleport yourself.\r\n");
    else if (GET_ADMLEVEL(victim) >= GET_ADMLEVEL(ch))
        ch->sendText("Maybe you shouldn't do that.\r\n");
    else if (!*buf2)
        ch->sendText("Where do you wish to send this person?\r\n");
    else if ((target = find_target_room(ch, buf2)) != NOWHERE)
    {
        if (PLR_FLAGGED(victim, PLR_HEALT))
        {
            ch->sendText("They are inside a healing tank!\r\n");
            return;
        }
        ch->send_to("%s", CONFIG_OK);
        act("$n disappears in a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
        victim->leaveLocation();
        victim->moveToLocation(target);
        act("$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
        act("$n has teleported you!", false, ch, nullptr, (char *)victim, TO_VICT);
        victim->lookAtLocation();
        enter_wtrigger(victim->getRoom(), victim, -1);
    }
}

ACMD(do_vnum)
{
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

    half_chop(argument, buf, buf2);

    if (!*buf || !*buf2 ||
        (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj") && !is_abbrev(buf, "mat") && !is_abbrev(buf, "wtype") &&
         !is_abbrev(buf, "atype")))
    {
        ch->sendText("Usage: vnum { atype | material | mob | obj | wtype } <name>\r\n");
        return;
    }
    if (is_abbrev(buf, "mob"))
        if (!vnum_mobile(buf2, ch))
            ch->sendText("No mobiles by that name.\r\n");

    if (is_abbrev(buf, "obj"))
        if (!vnum_object(buf2, ch))
            ch->sendText("No objects by that name.\r\n");

    if (is_abbrev(buf, "mat"))
        if (!vnum_material(buf2, ch))
            ch->sendText("No materials by that name.\r\n");

    if (is_abbrev(buf, "wtype"))
        if (!vnum_weapontype(buf2, ch))
            ch->sendText("No weapon types by that name.\r\n");

    if (is_abbrev(buf, "atype"))
        if (!vnum_armortype(buf2, ch))
            ch->sendText("No armor types by that name.\r\n");
}

static void list_zone_commands_room(Character *ch, Room *room)
{
    auto out = room->printResetCommands();
    if (out.empty())
    {
        ch->sendText("None!\r\n");
        return;
    }
    ch->sendText(out);
}

static void do_stat_room(Character *ch, Room *rm)
{
    char buf2[MAX_STRING_LENGTH];
    struct extra_descr_data *desc;
    int i, found, column;
    Object *j;
    Character *k;

    Location rl(rm);

    ch->send_to("Room name: @c%s@n\r\n", rm->getName());

    sprinttype(static_cast<int>(rm->sector_type), sector_types, buf2, sizeof(buf2));
    ch->send_to("Zone: [%3d], VNum: [@g%5d@n], Type: %s\r\n", rm->zone->number, rm->getVnum(), buf2);

    sprintf(buf2, "%s", rm->room_flags.getFlagNames().c_str());
    ch->send_to("Room Damage: %d, Room Effect: %d\r\n", rm->getDamage(), rm->ground_effect);
    ch->send_to("SpecProc: %s, Flags: %s\r\n", rm->func == nullptr ? "None" : "Exists", buf2);

    ch->send_to("Description:\r\n%s", rm->getLookDescription() ? rm->getLookDescription() : "  None.\r\n");

    if (auto exd = rl.getExtraDescription(); !exd.empty())
    {
        ch->sendText("Extra descs:");
        for (const auto &ex : exd)
        {
            ch->send_to(" [@c%s@n]", ex.keyword.c_str());
        }
        ch->sendText("\r\n");
    }

    ch->sendText("Chars present:");
    column = 14; /* ^^^ strlen ^^^ */
    auto people = rm->getPeople();
    auto sz = people.size();
    int i2 = 0;
    found = false;
    for (auto k : filter_raw(people))
    {
        i2++;
        if (!CAN_SEE(ch, k))
            continue;

        column += ch->send_to("%s @y%s@n(%s)", found++ ? "," : "", GET_NAME(k), !IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB"));
        if (column >= 62)
        {
            ch->send_to("%s\r\n", i2 < sz ? "," : "");
            found = false;
            column = 0;
        }
    }

    auto con = rm->getObjects();
    sz = con.size();

    if (sz)
    {
        ch->sendText("Contents:@g");
        column = 9; /* ^^^ strlen ^^^ */
        i2 = 0;
        found = false;
        for (auto j : filter_raw(con))
        {
            i2++;
            if (!CAN_SEE_OBJ(ch, j))
                continue;

            column += ch->send_to("%s %s", found++ ? "," : "", j->getShortDescription());
            if (column >= 62)
            {
                ch->send_to("%s\r\n", i2 < sz ? "," : "");
                found = false;
                column = 0;
            }
        }
        ch->sendText("@n");
    }

    for (auto &[d, ex] : rm->getDirections())
    {
        char buf1[128];
        i = static_cast<int>(d);
        if (!ex)
            continue;

        snprintf(buf1, sizeof(buf1), "@c%5d@n", ex.getVnum());

        sprintbit(ex.exit_info, exit_bits, buf2, sizeof(buf2));

        ch->send_to("Exit @c%-5s@n:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n%s", dirs[i], buf1, ex.key == NOTHING ? -1 : ex.key, !ex.keyword.empty() ? ex.keyword : "None", buf2, !ex.general_description.empty() ? ex.general_description : "  No exit description.\r\n");
    }

    /* check the room for a script */
    do_sstat(ch, rm);

    list_zone_commands_room(ch, rm);
}

static void do_stat_room(Character *ch)
{
    auto rm = ch->getRoom();
    do_stat_room(ch, rm);
}

static void do_stat_object(Character *ch, Object *j)
{
    int i, found;
    obj_vnum vnum;
    Object *j2;
    Character *sitter;
    struct extra_descr_data *desc;
    char buf[MAX_STRING_LENGTH];

    vnum = GET_OBJ_VNUM(j);
    if (GET_LAST_LOAD(j) > 0)
    {
        char *tmstr;
        time_t t = GET_LAST_LOAD(j);
        tmstr = (char *)asctime(localtime(&t));
        *(tmstr + strlen(tmstr) - 1) = '\0';
        ch->send_to("LOADED DROPPED: [%s]\r\n", tmstr);
    }
    if (GET_OBJ_VNUM(j) == 65)
    {
        ch->send_to("Healing Tank Charge Level: [%d]\r\n", HCHARGE(j));
    }
    ch->send_to("Name: '%s', Keywords: %s, Size: %s\r\n", j->getShortDescription() ? j->getShortDescription() : "<None>", j->getName(), size_names[static_cast<int>(GET_OBJ_SIZE(j))]);

    sprinttype(GET_OBJ_TYPE(j), item_types, buf, sizeof(buf));
    ch->send_to("VNum: [@g%5d@n], RNum: [%5d], Idnum: [%5d], Type: %s, SpecProc: %s\r\n", vnum, GET_OBJ_RNUM(j), ((j)->id), buf, GET_OBJ_SPEC(j) ? "Exists" : "None");

    ch->send_to("Unique ID: @g%" I64T "@n\r\n", j->id);

    ch->send_to("Object Hit Points: [ @g%3d@n/@g%3d@n]\r\n", GET_OBJ_VAL(j, VAL_ALL_HEALTH), GET_OBJ_VAL(j, VAL_ALL_MAXHEALTH));

    ch->send_to("Object loaded in room: @y%d@n\r\n", OBJ_LOADROOM(j));

    ch->send_to("Object Material: @y%s@n\r\n", material_names[GET_OBJ_MATERIAL(j)]);

    if (auto sitter = SITTING(j))
    {
        ch->send_to("HOLDING: %s\r\n", GET_NAME(sitter));
    }

    if (!j->getExtraDescription().empty())
    {
        ch->sendText("Extra descs:");
        for (const auto &ex : j->getExtraDescription())
        {
            ch->send_to(" [@g%s@n]", ex.keyword.c_str());
        }
        ch->sendText("\r\n");
    }

    sprintf(buf, "%s", GET_OBJ_WEAR(j).getFlagNames().c_str());
    ch->send_to("Can be worn on: %s\r\n", buf);

    sprintf(buf, "%s", GET_OBJ_PERM(j).getFlagNames().c_str());
    ch->send_to("Set char bits : %s\r\n", buf);

    sprintf(buf, "%s", GET_OBJ_EXTRA(j).getFlagNames().c_str());
    ch->send_to("Extra flags   : %s\r\n", buf);

    auto wString = fmt::format("{}", GET_OBJ_WEIGHT(j));
    ch->send_to("Weight: %s, Value: %d, Cost/day: %d, Timer: %d, Min Level: %d\r\n", wString.c_str(), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j), GET_OBJ_LEVEL(j));

    /*
     * NOTE: In order to make it this far, we must already be able to see the
     *       character holding the object. Therefore, we do not need CAN_SEE().
     */
    if (auto r = j->getRoom())
    {
        ch->send_to("In room: %d (%s), ", r->getVnum(), r->getName());
    }
    else if (auto j2 = j->getContainer())
    {
        ch->send_to("In object: %s, ", j2->getShortDescription());
    }
    else if (auto c = j->getCarriedBy())
    {
        ch->send_to("Carried by: %s, ", GET_NAME(c));
    }
    else if (auto c = j->getWornBy())
    {
        ch->send_to("Worn by: %s, ", GET_NAME(c));
    }
    else
    {
        ch->send_to("In unknown location: %s, ", j->getName());
    }

    switch (GET_OBJ_TYPE(j))
    {
    case ITEM_LIGHT:
        if (GET_OBJ_VAL(j, VAL_LIGHT_HOURS) == -1)
            ch->sendText("Hours left: Infinite\r\n");
        else
            ch->send_to("Hours left: [%d]\r\n", GET_OBJ_VAL(j, VAL_LIGHT_HOURS));
        break;
    case ITEM_SCROLL:
        ch->send_to("Spell: (Level %d) %s\r\n", GET_OBJ_VAL(j, VAL_SCROLL_LEVEL), skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL1)));
        break;
    case ITEM_POTION:
        ch->send_to("Spells: (Level %d) %s, %s, %s\r\n", GET_OBJ_VAL(j, VAL_POTION_LEVEL), skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL1)), skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL2)), skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL3)));
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        ch->send_to("Spell: %s at level %d, %d (of %d) charges remaining\r\n", skill_name(GET_OBJ_VAL(j, VAL_STAFF_SPELL)), GET_OBJ_VAL(j, VAL_STAFF_LEVEL), GET_OBJ_VAL(j, VAL_STAFF_CHARGES), GET_OBJ_VAL(j, VAL_STAFF_MAXCHARGES));
        break;
    case ITEM_WEAPON:
        ch->send_to("Weapon Type: %s, Todam: %dd%d, Message type: %d\r\n", weapon_type[GET_OBJ_VAL(j, VAL_WEAPON_SKILL)], GET_OBJ_VAL(j, VAL_WEAPON_DAMDICE), GET_OBJ_VAL(j, VAL_WEAPON_DAMSIZE), GET_OBJ_VAL(j, VAL_WEAPON_DAMTYPE));
        ch->send_to("Average damage per round %.1f\r\n", ((GET_OBJ_VAL(j, VAL_WEAPON_DAMSIZE) + 1) / 2.0) * GET_OBJ_VAL(j, VAL_WEAPON_DAMDICE));
        ch->send_to("Crit type: %s, Crit range: %d-20\r\n", crit_type[GET_OBJ_VAL(j, VAL_WEAPON_CRITTYPE)], 20 - GET_OBJ_VAL(j, VAL_WEAPON_CRITRANGE));
        break;
    case ITEM_ARMOR:
        ch->send_to("Armor Type: %s, AC-apply: [%d]\r\n", armor_type[GET_OBJ_VAL(j, VAL_ARMOR_SKILL)], GET_OBJ_VAL(j, VAL_ARMOR_APPLYAC));
        ch->send_to("Max dex bonus: %d, Armor penalty: %d, Spell failure: %d\r\n", GET_OBJ_VAL(j, VAL_ARMOR_MAXDEXMOD), GET_OBJ_VAL(j, VAL_ARMOR_CHECK), GET_OBJ_VAL(j, VAL_ARMOR_SPELLFAIL));
        break;
    case ITEM_TRAP:
        ch->send_to("Spell: %d, - Hitpoints: %d\r\n", GET_OBJ_VAL(j, VAL_TRAP_SPELL), GET_OBJ_VAL(j, VAL_TRAP_HITPOINTS));
        break;
    case ITEM_CONTAINER:
        sprintbit(GET_OBJ_VAL(j, VAL_CONTAINER_FLAGS), container_bits, buf, sizeof(buf));
        ch->send_to("Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s\r\n", GET_OBJ_VAL(j, VAL_CONTAINER_CAPACITY), buf, GET_OBJ_VAL(j, VAL_CONTAINER_KEY), YESNO(GET_OBJ_VAL(j, VAL_CONTAINER_CORPSE)));
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        sprinttype(GET_OBJ_VAL(j, VAL_DRINKCON_LIQUID), drinks, buf, sizeof(buf));
        ch->send_to("Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s\r\n", GET_OBJ_VAL(j, VAL_DRINKCON_CAPACITY), GET_OBJ_VAL(j, VAL_DRINKCON_HOWFULL), YESNO(GET_OBJ_VAL(j, VAL_DRINKCON_POISON)), buf);
        break;
    case ITEM_NOTE:
        ch->send_to("Tongue: %d\r\n", GET_OBJ_VAL(j, VAL_NOTE_LANGUAGE));
        break;
    case ITEM_KEY:
        /* Nothing */
        break;
    case ITEM_FOOD:
        ch->send_to("Makes full: %d, Poisoned: %s\r\n", GET_OBJ_VAL(j, VAL_FOOD_FOODVAL), YESNO(GET_OBJ_VAL(j, VAL_FOOD_POISON)));
        break;
    case ITEM_MONEY:
        ch->send_to("Coins: %d\r\n", GET_OBJ_VAL(j, VAL_MONEY_SIZE));
        break;
    default:
    {
        std::vector<std::string> value(j->stats.size());
        for (auto &[nm, val] : j->stats)
            value.emplace_back(fmt::format("[{}:{}]", nm, val));
        ch->send_to("%s", fmt::format("Values 0-{}:\r\n{}\r\n", j->stats.size() - 1, fmt::join(value, " ")));
    }
    break;
    }

    /*
     * I deleted the "equipment status" code from here because it seemed
     * more or less useless and just takes up valuable screen space.
     */

    if (auto con = j->getInventory(); !con.empty())
    {
        int column;

        ch->sendText("\r\nContents:@g");
        column = 9; /* ^^^ strlen ^^^ */
        found = false;
        auto sz = con.size();
        auto count = 0;
        for (auto j2 : filter_raw(con))
        {
            column += ch->send_to("%s %s", found++ ? "," : "", j2->getShortDescription());
            count++;
            if (column >= 62)
            {
                ch->send_to("%s\r\n", count < sz ? "," : "");
                found = false;
                column = 0;
            }
        }
        ch->sendText("@n");
    }

    ch->sendText("Affections: ");
    std::vector<std::string> affs;
    for (auto &aff : j->affected)
    {
        if (aff.location == APPLY_NONE)
            continue;
        found = true;
        std::string bon = (aff.modifier >= 0.0 ? "+" : "") + (aff.isPercent() ? fmt::format("{:.2f}%", aff.modifier * 100.0) : format_double(aff.modifier));
        affs.emplace_back(fmt::format("{} to {} ({})", bon, aff.locName(), fmt::join(aff.specificNames(), ", ")));
    }

    if (affs.empty())
        ch->sendText("None");
    else
        ch->send_to("%s", fmt::format("{}", fmt::join(affs, ", ")));

    ch->sendText("\r\n");

    /* check the object for a script */
    do_sstat(ch, j);
}

static void do_stat_character(Character *ch, Character *k)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int i, i2, column, found = false;
    Object *j;
    Object *chair;
    struct follow_type *fol;
    struct affected_type *aff;

    if (IS_NPC(k))
    {
        char *tmstr;
        auto tm = GET_LPLAY(k);
        tmstr = (char *)asctime(localtime(&tm));
        *(tmstr + strlen(tmstr) - 1) = '\0';
        ch->send_to("LOADED AT: [%s]\r\n", tmstr);
    }
    auto sex = fmt::format("{}", magic_enum::enum_name(k->sex));

    snprintf(buf, sizeof(buf), "%s", sex.c_str());
    ch->send_to("%s %s '%s'  IDNum: [%5d], In room [%5d], Loadroom : [%5d]\r\n", buf, (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")), GET_NAME(k), IS_NPC(k) ? ((k)->id) : GET_IDNUM(k), k->location.getVnum(), IS_NPC(k) ? MOB_LOADROOM(k) : GET_LOADROOM(k));

    ch->send_to("DROOM: [%5d]\r\n", GET_DROOM(k));
    if (IS_MOB(k))
    {
        if (auto mas = k->getBaseStat<int>("master_id"); mas != NOTHING)
            sprintf(buf, ", Master: %s", get_name_by_id(mas));
        else
            buf[0] = 0;
        ch->send_to("Keyword: %s, VNum: [%5d], RNum: [%5d]%s\r\n", k->getName(), GET_MOB_VNUM(k), GET_MOB_RNUM(k), buf);
    }
    else

        ch->send_to("Title: %s\r\n", k->title ? k->title : "<None>");

    ch->send_to("L-Des: %s@n", k->getRoomDescription() ? k->getRoomDescription() : "<None>\r\n");
    snprintf(buf, sizeof(buf), "%s", sensei::getName(k->sensei).c_str());
    snprintf(buf2, sizeof(buf2), "%s", race::getName(k->race).c_str());
    ch->send_to("Class: %s, Race: %s, Lev: [@y%2d@n], XP: [@y%" I64T "@n]\r\n", buf, buf2, GET_LEVEL(k), GET_EXP(k));

    if (!IS_NPC(k))
    {
        char buf1[64], cmbuf2[64];

        strlcpy(buf1, asctime(localtime(&(k->time.created))), sizeof(buf1));
        strlcpy(cmbuf2, asctime(localtime(&(k->time.logon))), sizeof(cmbuf2));
        buf1[10] = cmbuf2[10] = '\0';

        ch->send_to("Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n", buf1, cmbuf2, (int)k->time.played / 3600, (int)(((int64_t)k->time.played % 3600) / 60), 0);

        if (auto find = players.find(k->id); find != players.end())
        {
            ch->send_to("@YOwned by User@D: [@C%s@D]@n\r\n", find->second.account->name.c_str());
        }
        if (!IS_NPC(k))
        {
            ch->send_to("@RCharacter Deaths@D: @r%d@n\r\n", GET_DCOUNT(k));
        }

        ch->send_to("Hometown: [%d], Align: [%4d], Ethic: [%4d]", GET_HOME(k), GET_ALIGNMENT(k), GET_ETHIC_ALIGNMENT(k));

        /*. Display OLC zone for immorts .*/
        if (GET_ADMLEVEL(k) >= ADMLVL_BUILDER)
        {
            if (GET_OLC_ZONE(k) == AEDIT_PERMISSION)
                ch->sendText(", OLC[@cActions@n]");
            else if (GET_OLC_ZONE(k) == HEDIT_PERMISSION)
                ch->sendText(", OLC[@cHedit@n]");
            else if (GET_OLC_ZONE(k) == NOWHERE)
                ch->sendText(", OLC[@cOFF@n]");
            else
                ch->send_to(", OLC: [@c%d@n]", GET_OLC_ZONE(k));
        }
        ch->sendText("\r\n");
    }
    ch->send_to("Str: [@c%d@n]  Int: [@c%d@n]  Wis: [@c%d@n]  "
                "Dex: [@c%d@n]  Con: [@c%d@n]  Cha: [@c%d@n]\r\n",
                GET_STR(k), GET_INT(k), GET_WIS(k), GET_DEX(k), GET_CON(k), GET_CHA(k));

    ch->send_to("PL :[@g%12s@n]  KI :[@g%12s@n]  ST :[@g%12s@n]\r\n", add_commas(GET_HIT(k)).c_str(), add_commas((k->getCurVital(CharVital::ki))).c_str(), add_commas((k->getCurVital(CharVital::stamina))).c_str());
    ch->send_to("MPL:[@g%12s@n]  MKI:[@g%12s@n]  MST:[@g%12s@n]\r\n", add_commas(GET_MAX_HIT(k)).c_str(), add_commas(GET_MAX_MANA(k)).c_str(), add_commas(GET_MAX_MOVE(k)).c_str());
    ch->send_to("BPL:[@g%12s@n]  BKI:[@g%12s@n]  BST:[@g%12s@n]\r\n", add_commas((k->getBaseStat<int64_t>("health"))).c_str(), add_commas((k->getBaseStat<int64_t>("ki"))).c_str(), add_commas((k->getBaseStat<int64_t>("stamina"))).c_str());
    ch->send_to("LF :[@g%12s@n]  MLF:[@g%12s@n]  LFP:[@g%3d@n]\r\n", add_commas((k->getCurVital(CharVital::lifeforce))).c_str(), add_commas((k->getEffectiveStat("lifeforce"))).c_str(), GET_LIFEPERC(k));

    if (GET_ADMLEVEL(k))
        ch->send_to("Admin Level: [@y%d - %s@n]\r\n", GET_ADMLEVEL(k), admin_level_names[GET_ADMLEVEL(k)]);

    ch->send_to("Coins: [%9d], Bank: [%9d] (Total: %d)\r\n", GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));

    ch->send_to("Armor: [%d ], Damage: [%2d]\r\n", GET_ARMOR(k), GET_DAMAGE_MOD(k));

    sprinttype(GET_POS(k), position_types, buf, sizeof(buf));
    ch->send_to("Pos: %s, Fighting: %s", buf, FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody");

    if (k->desc)
    {
        sprinttype(STATE(k->desc), connected_types, buf, sizeof(buf));
        ch->send_to(", Connected: %s", buf);
    }

    sprinttype(k->mob_specials.default_pos, position_types, buf, sizeof(buf));
    ch->send_to(", Default position: %s", buf);
    ch->send_to(", Idle Timer (in tics) [%d]\r\n", k->timer);
    sprintf(buf, "%s", k->character_flags.getFlagNames().c_str());
    ch->send_to("Character flags: @c%s@n\r\n", buf);
    sprintf(buf, "%s", k->mob_flags.getFlagNames().c_str());
    ch->send_to("NPC flags: @c%s@n\r\n", buf);
    sprintf(buf, "%s", k->player_flags.getFlagNames().c_str());
    ch->send_to("PLR: @c%s@n\r\n", buf);
    sprintf(buf, "%s", k->pref_flags.getFlagNames().c_str());
    ch->send_to("PRF: @g%s@n\r\n", buf);

    ch->send_to("Form: %s\r\n", trans::getName(k, k->form));

    if (IS_MOB(k))
    {
        ch->send_to("Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n", (mob_index.at(GET_MOB_RNUM(k)).func ? "Exists" : "None"), k->mob_specials.damnodice, k->mob_specials.damsizedice);
        ch->send_to("Average damage per round %.1f (%.1f [BHD] + %d [STR MOD] + %d [DMG MOD])\r\n", (((k->mob_specials.damsizedice) + 1) / 2.0) * (k->mob_specials.damnodice) + ability_mod_value(GET_STR(k)) + GET_DAMAGE_MOD(k), (((k->mob_specials.damsizedice) + 1) / 2.0) * (k->mob_specials.damnodice), ability_mod_value(GET_STR(k)), GET_DAMAGE_MOD(k));
    }

    int counts = 0, total = 0;
    i = 0;
    auto con = k->getInventory();
    for (auto j : filter_raw(con))
    {
        counts += check_insidebag(j, 0.5);
        counts++;
        i++;
    }
    total = counts;
    total += i;
    for (i = 0, i2 = 0; i < NUM_WEARS; i++)
        if (GET_EQ(k, i))
        {
            i2++;
            total += check_insidebag(GET_EQ(k, i), 0.5) + 1;
        }
    ch->send_to("Carried: weight: %d, Total Items (includes bagged items): %d, EQ: %d\r\n", (int64_t)IS_CARRYING_W(k), total, i2);

    if (!IS_NPC(k))
        ch->send_to("Hunger: %d, Thirst: %d, Drunk: %d\r\n", GET_COND(k, HUNGER), GET_COND(k, THIRST), GET_COND(k, DRUNK));

    column = ch->send_to("Master is: %s, Followers are:", k->master ? GET_NAME(k->master) : "<none>");
    if (!k->followers)
        ch->sendText(" <none>\r\n");
    else
    {
        for (fol = k->followers; fol; fol = fol->next)
        {
            column += ch->send_to("%s %s", found++ ? "," : "", PERS(fol->follower, ch));
            if (column >= 62)
            {
                ch->send_to("%s\r\n", fol->next ? "," : "");
                found = false;
                column = 0;
            }
        }
        if (column != 0)
            ch->sendText("\r\n");
    }

    if (SITS(k))
    {
        chair = SITS(k);
        ch->send_to("Is on: %s@n\r\n", chair->getShortDescription());
    }

    /* Showing the bitvector */
    sprintf(buf, "%s", AFF_FLAGS(k).getFlagNames().c_str());
    ch->send_to("AFF: @y%s@n\r\n", buf);

    /* Routine to show what spells a char is affected by */
    if (k->affected)
    {
        for (aff = k->affected; aff; aff = aff->next)
        {
            ch->send_to("SPL: (%3dhr) @c%-21s@n ", aff->duration + 1, skill_name(aff->type));

            if (aff->modifier)
                ch->send_to("%+d to %s", aff->modifier, apply_types[(int)aff->location]);

            if (aff->bitvector)
            {
                if (aff->modifier)
                    ch->sendText(", ");

                strcpy(buf, affected_bits[aff->bitvector]);
                ch->send_to("sets %s", buf);
            }
            ch->sendText("\r\n");
        }
    }

    /* Routine to show what spells a char is affectedv by */
    if (k->affectedv)
    {
        for (aff = k->affectedv; aff; aff = aff->next)
        {
            ch->send_to("SPL: (%3d rounds) @c%-21s@n ", aff->duration + 1, skill_name(aff->type));

            if (aff->modifier)
                ch->send_to("%+d to %s", aff->modifier, apply_types[(int)aff->location]);

            if (aff->bitvector)
            {
                if (aff->modifier)
                    ch->sendText(", ");

                strcpy(buf, affected_bits[aff->bitvector]);
                ch->send_to("sets %s", buf);
            }
            ch->sendText("\r\n");
        }
    }

    /* check mobiles for a script */
    if (IS_NPC(k))
    {
        do_sstat(ch, k);
        if (SCRIPT_MEM(k))
        {
            struct script_memory *mem = SCRIPT_MEM(k);
            ch->sendText("Script memory:\r\n  Remember             Command\r\n");
            while (mem)
            {
                auto find = uniqueCharacters.find(mem->id);
                if (find == uniqueCharacters.end())
                {
                    ch->sendText("  ** Corrupted!\r\n");
                }
                else
                {
                    ch->send_to("  %-20.20s <default>\r\n", GET_NAME(find->second.get()));
                }
                mem = mem->next;
            }
        }
    }
    else
    {
        int x, track = 0;

        ch->sendText("Bonuses/Negatives:\r\n");

        for (x = 0; x < 30; x++)
        {
            if (x < 15)
            {
                if (GET_BONUS(k, x) > 0)
                {
                    ch->send_to("@c%s@n\n", list_bonus[x]);
                    track += 1;
                }
            }
            else
            {
                if (GET_BONUS(k, x) > 0)
                {
                    ch->send_to("@r%s@n\n", list_bonus[x]);
                    track += 1;
                }
            }
        }
        if (track <= 0)
        {
            ch->sendText("@wNone.@n\r\n");
        }
        ch->sendText("To see player variables use varstat now.\r\n");
    }
}

ACMD(do_varstat)
{
    Character *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
    {
        ch->sendText("That player is not in the game.\r\n");
        return;
    }
    else if (IS_NPC(vict))
    {
        ch->sendText("Just use stat for an NPC\r\n");
        return;
    }
    else
    {
        /* Display their global variables */
        if (!vict->variables.empty())
        {
            struct trig_var_data *tv;
            char uname[MAX_INPUT_LENGTH];
            void find_uid_name(char *uid, char *name, size_t nlen);

            ch->send_to("%s's Global Variables:\r\n", GET_NAME(vict));

            /* currently, variable context for players is always 0, so it is */
            /* not displayed here. in the future, this might change */
            for (const auto &[name, value] : vict->variables)
            {
                if (value.starts_with(UID_CHAR))
                {
                    auto uidResult = resolveUID(value);
                    if (uidResult)
                    {
                        ch->send_to("    %10s:  [UID]: %s\r\n", name, uidResult->getDgName());
                    }
                    else
                    {
                        ch->send_to("   -BAD UID: %s", value);
                    }
                }
                else
                {
                    ch->send_to("    %10s:  %s\r\n", name, value);
                }
            }
        }
    }
}

ACMD(do_stat)
{
    char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
    Character *victim;
    Object *object;

    half_chop(argument, buf1, buf2);

    if (!*buf1)
    {
        ch->sendText("Stats on who or what or where?\r\n");
        return;
    }
    else if (is_abbrev(buf1, "room"))
    {
        do_stat_room(ch);
    }
    else if (is_abbrev(buf1, "mob"))
    {
        if (!*buf2)
            ch->sendText("Stats on which mobile?\r\n");
        else
        {
            if ((victim = get_char_vis(ch, buf2, nullptr, FIND_CHAR_WORLD)))
                do_stat_character(ch, victim);
            else
                ch->sendText("No such mobile around.\r\n");
        }
    }
    else if (is_abbrev(buf1, "player"))
    {
        if (!*buf2)
        {
            ch->sendText("Stats on which player?\r\n");
        }
        else
        {
            if ((victim = findPlayer(buf2)))
                do_stat_character(ch, victim);
            else
                ch->sendText("No such player around.\r\n");
        }
    }
    else if (is_abbrev(buf1, "file"))
    {
        if (!*buf2)
            ch->sendText("Stats on which player?\r\n");
        else if ((victim = get_player_vis(ch, buf2, nullptr, FIND_CHAR_WORLD)))
            do_stat_character(ch, victim);
        else
        {
            victim = findPlayer(buf2);
            if (!victim)
            {
                ch->sendText("There is no such player.\r\n");
                return;
            }
            if (GET_ADMLEVEL(victim) > GET_ADMLEVEL(ch))
                ch->sendText("Sorry, you can't do that.\r\n");
            else
                do_stat_character(ch, victim);
        }
    }
    else if (is_abbrev(buf1, "object"))
    {
        if (!*buf2)
            ch->sendText("Stats on which object?\r\n");
        else
        {
            if ((object = get_obj_vis(ch, buf2, nullptr)))
                do_stat_object(ch, object);
            else
                ch->sendText("No such object around.\r\n");
        }
    }
    else if (is_abbrev(buf1, "zone"))
    {
        if (!*buf2)
        {
            ch->sendText("Stats on which zone?\r\n");
            return;
        }
        else
        {
            print_zone(ch, atoi(buf2));
            return;
        }
    }
    else
    {
        char *name = buf1;
        int number = get_number(&name);

        if ((object = get_obj_in_equip_vis(ch, name, &number, ch->getEquipment())))
            do_stat_object(ch, object);
        else if ((object = get_obj_in_list_vis(ch, name, &number, ch->getInventory())))
            do_stat_object(ch, object);
        else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_ROOM)))
            do_stat_character(ch, victim);
        else if ((object = get_obj_in_list_vis(ch, name, &number, ch->location.getObjects())))
            do_stat_object(ch, object);
        else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_WORLD)))
            do_stat_character(ch, victim);
        else if ((object = get_obj_vis(ch, name, &number)))
            do_stat_object(ch, object);
        else
            ch->sendText("Nothing around by that name.\r\n");
    }
}

ACMD(do_shutdown)
{
    char arg[MAX_INPUT_LENGTH];

    if (subcmd != SCMD_SHUTDOWN)
    {
        ch->sendText("If you want to shut something down, say so!\r\n");
        return;
    }
    one_argument(argument, arg);

    if (!*arg)
    {
        basic_mud_log("(GC) Shutdown by %s.", GET_NAME(ch));
        send_to_all("Shutting down.\r\n");
        circle_shutdown = 1;
    }
    else if (!strcasecmp(arg, "now"))
    {
        basic_mud_log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
        send_to_all("Rebooting.. come back in a minute or two.\r\n");
        circle_shutdown = 1;
        circle_reboot = 2; /* do not autosave olc */
    }
    else
        ch->sendText("Unknown shutdown option.\r\n");
}

void snoop_check(Character *ch)
{
    /*  This short routine is to ensure that characters that happen
     *  to be snooping (or snooped) and get advanced/demoted will
     *  not be snooping/snooped someone of a higher/lower level (and
     *  thus, not entitled to be snooping.
     */
    if (!ch || !ch->desc)
        return;
    if (ch->desc->snooping &&
        (GET_ADMLEVEL(ch->desc->snooping->character) >= GET_ADMLEVEL(ch)))
    {
        ch->desc->snooping->snoop_by = nullptr;
        ch->desc->snooping = nullptr;
    }

    if (ch->desc->snoop_by &&
        (GET_ADMLEVEL(ch) >= GET_ADMLEVEL(ch->desc->snoop_by->character)))
    {
        ch->desc->snoop_by->snooping = nullptr;
        ch->desc->snoop_by = nullptr;
    }
}

static void stop_snooping(Character *ch)
{
    if (!ch->desc->snooping)
        ch->sendText("You aren't snooping anyone.\r\n");
    else
    {
        ch->sendText("You stop snooping.\r\n");
        ch->desc->snooping->snoop_by = nullptr;
        ch->desc->snooping = nullptr;
    }
}

ACMD(do_snoop)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim, *tch;

    if (!ch->desc)
        return;

    one_argument(argument, arg);

    if (!*arg)
        stop_snooping(ch);
    else if (!(victim = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
        ch->sendText("No such person around.\r\n");
    else if (!victim->desc)
        ch->sendText("There's no link.. nothing to snoop.\r\n");
    else if (victim == ch)
        stop_snooping(ch);
    else if (victim->desc->snoop_by)
        ch->sendText("Busy already. \r\n");
    else if (victim->desc->snooping == ch->desc)
        ch->sendText("Don't be stupid.\r\n");
    else
    {
        if (victim->desc->original)
            tch = victim->desc->original;
        else
            tch = victim;

        if (GET_ADMLEVEL(tch) >= GET_ADMLEVEL(ch))
        {
            ch->sendText("You can't.\r\n");
            return;
        }
        ch->send_to("%s", CONFIG_OK);

        if (ch->desc->snooping)
            ch->desc->snooping->snoop_by = nullptr;

        ch->desc->snooping = victim->desc;
        victim->desc->snoop_by = ch->desc;
    }
}

ACMD(do_switch)
{
    char arg[MAX_INPUT_LENGTH];
    Character *victim;

    one_argument(argument, arg);

    if (ch->desc->original)
        ch->sendText("You're already switched.\r\n");
    else if (!*arg)
        ch->sendText("Switch with who?\r\n");
    else if (!(victim = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
        ch->sendText("No such character.\r\n");
    else if (ch == victim)
        ch->sendText("Hee hee... we are jolly funny today, eh?\r\n");
    else if (victim->desc)
        ch->sendText("You can't do that, the body is already in use!\r\n");
    else if (!(IS_NPC(victim) || ADM_FLAGGED(ch, ADM_SWITCHMORTAL)))
        ch->sendText("You aren't holy enough to use a mortal's body.\r\n");
    else if (GET_ADMLEVEL(ch) < ADMLVL_VICE && victim->location.getRoomFlag(ROOM_GODROOM))
        ch->sendText("You are not godly enough to use that room!\r\n");
    else
    {
        ch->send_to("%s", CONFIG_OK);

        ch->desc->character = victim;
        ch->desc->original = ch;

        victim->desc = ch->desc;
        ch->desc = nullptr;
    }
}

ACMD(do_return)
{
    if (ch->desc && ch->desc->original)
    {
        ch->sendText("You return to your original body.\r\n");

        /*
         * If someone switched into your original body, disconnect them.
         *   - JE 2/22/95
         *
         * Zmey: here we put someone switched in our body to disconnect state
         * but we must also nullptr his pointer to our character, otherwise
         * close() will damage our character's pointer to our descriptor
         * (which is assigned below in this function). 12/17/99
         */
        if (ch->desc->original->desc)
        {
            ch->desc->original->desc->character = nullptr;
            STATE(ch->desc->original->desc) = CON_DISCONNECT;
        }

        /* Now our descriptor points to our original body. */
        ch->desc->character = ch->desc->original;
        ch->desc->original = nullptr;

        /* And our body's pointer to descriptor now points to our descriptor. */
        ch->desc->character->desc = ch->desc;
        ch->desc = nullptr;
    }
}

ACMD(do_load)
{
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
    int i = 0, n = 1;

    one_argument(two_arguments(argument, buf, buf2), buf3);

    if (!*buf || !*buf2 || !isdigit(*buf2))
    {
        ch->sendText("Usage: load { obj | mob } <vnum> (amt)\r\n");
        return;
    }
    if (!is_number(buf2) || !is_number(buf3))
    {
        ch->sendText("That is not a number.\r\n");
        return;
    }

    if (atoi(buf3) > 0)
    {
        if (atoi(buf3) >= 100)
        {
            n = 100;
        }
        else if (atoi(buf3) < 100)
        {
            n = atoi(buf3);
        }
    }
    else
    {
        n = 1;
    }

    if (is_abbrev(buf, "mob"))
    {
        Character *mob = nullptr;
        mob_rnum r_num;

        if ((r_num = real_mobile(atoi(buf2))) == NOBODY)
        {
            ch->sendText("There is no monster with that number.\r\n");
            return;
        }
        for (i = 0; i < n; i++)
        {
            mob = read_mobile(r_num, REAL);
            mob->moveToLocation(ch);

            act("$n makes a quaint, magical gesture with one hand.", true, ch, nullptr, nullptr, TO_ROOM);
            act("$n has created $N!", false, ch, nullptr, mob, TO_ROOM);
            act("You create $N.", false, ch, nullptr, mob, TO_CHAR);
            load_mtrigger(mob);
        }
    }
    else if (is_abbrev(buf, "obj"))
    {
        Object *obj;
        obj_rnum r_num;

        if ((r_num = real_object(atoi(buf2))) == NOTHING)
        {
            ch->sendText("There is no object with that number.\r\n");
            return;
        }
        for (i = 0; i < n; i++)
        {
            obj = read_object(r_num, REAL);
            if (GET_ADMLEVEL(ch) > 0)
            {
                send_to_imm("LOAD: %s has loaded a %s", GET_NAME(ch), obj->getShortDescription());
                log_imm_action("LOAD: %s has loaded a %s", GET_NAME(ch), obj->getShortDescription());
            }
            if (CONFIG_LOAD_INVENTORY)
                ch->addToInventory(obj);
            else
                obj->moveToLocation(ch);
            act("$n makes a strange magical gesture.", true, ch, nullptr, nullptr, TO_ROOM);
            act("$n has created $p!", false, ch, obj, nullptr, TO_ROOM);
            act("You create $p.", false, ch, obj, nullptr, TO_CHAR);
            load_otrigger(obj);
        }
    }
    else
        ch->sendText("That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_vstat)
{
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

    two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2 || !isdigit(*buf2))
    {
        ch->sendText("Usage: vstat { obj | mob } <number>\r\n");
        return;
    }
    if (!is_number(buf2))
    {
        ch->sendText("That's not a valid number.\r\n");
        return;
    }

    if (is_abbrev(buf, "mob"))
    {
        Character *mob;
        mob_rnum r_num;

        if ((r_num = real_mobile(atoi(buf2))) == NOBODY)
        {
            ch->sendText("There is no monster with that number.\r\n");
            return;
        }
        mob = read_mobile(r_num, REAL);
        mob->moveToLocation(0);
        do_stat_character(ch, mob);
        extract_char(mob);
    }
    else if (is_abbrev(buf, "obj"))
    {
        Object *obj;
        obj_rnum r_num;

        if ((r_num = real_object(atoi(buf2))) == NOTHING)
        {
            ch->sendText("There is no object with that number.\r\n");
            return;
        }
        obj = read_object(r_num, REAL);
        do_stat_object(ch, obj);
        extract_obj(obj);
    }
    else
        ch->sendText("That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_pgrant)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Character *vict;

    two_arguments(argument, arg1, arg2);

    std::string strForm;
    if (!*arg1)
    {
        ch->sendText("Usage: pgrant <char> <form>||growth\r\n");
        return;
    }

    if (!*arg2)
    {
        vict = ch;
        strForm = arg1;
    }
    else
    {
        if (!(vict = get_char_vis(ch, arg1, nullptr, FIND_CHAR_WORLD)))
        {
            ch->sendText("No such character.\r\nUsage: pgrant <char> <form>||growth\r\n");
            return;
        }
        strForm = arg2;
    }

    if (strForm == "growth")
    {
        if (vict)
        {
            vict->modBaseStat("internalGrowth", 500);
            ch->send_to("Given 500 growth to %s\r\n", vict->getName());
        }
        else
        {
            ch->modBaseStat("internalGrowth", 500);
            ch->sendText("500 growth has been given to you.\r\n");
        }
        return;
    }

    auto foundForm = trans::findForm(vict, strForm);

    if (!foundForm.has_value())
    {
        ch->send_to("Form %s not found.\r\n", strForm);
        return;
    }

    if (!vict->transforms.contains(*foundForm))
    {
        vict->addTransform(*foundForm);
        ch->send_to("Form %s added!\r\n", strForm);
        log_imm_action("Form Added: %s added %s to %s!", ch, strForm, vict);
    }
    else
    {
        if (vict->transforms.find(*foundForm)->second.visible)
        {
            vict->hideTransform(*foundForm, true);
            ch->send_to("Form %s hidden!\r\n", strForm);
            log_imm_action("Form Hidden: %s hidden %s from %s!", ch, strForm, vict);
        }
        else
        {
            vict->hideTransform(*foundForm, false);
            ch->send_to("Form %s unhidden!\r\n", strForm);
            log_imm_action("Form Unhidden: %s unhidden %s from %s!", ch, strForm, vict);
        }
    }
}

ACMD(do_rpreward)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Character *vict;

    two_arguments(argument, arg1, arg2);

    if (!(vict = get_char_vis(ch, arg1, nullptr, FIND_CHAR_WORLD)))
    {
        ch->sendText("No such character.\r\nUsage: rpreward <char> <low|medium|high>\r\n");
        return;
    }
    if (!(is_abbrev(arg2, "low") || is_abbrev(arg2, "medium") || is_abbrev(arg2, "high")))
    {
        ch->sendText("Usage: rpreward <char> <low|medium|high>\r\n");
        return;
    }

    int rppGain = 3;
    double vitalsGain = 0.3;
    int growthGain = 2;

    if (is_abbrev(arg2, "medium"))
    {
        int rppGain = 5;
        int vitalsGain = 0.6;
        int growthGain = 3;
    }
    else if (is_abbrev(arg2, "high"))
    {
        int rppGain = 7;
        int vitalsGain = 0.9;
        int growthGain = 4;
    }

    double boundPL = log(ch->getBaseStat<int64_t>("health"));
    double boundKI = log(ch->getBaseStat<int64_t>("ki"));
    double boundST = log(ch->getBaseStat<int64_t>("stamina"));

    vict->modRPP(rppGain);
    vict->gainGrowth(growthGain);

    vict->gainBaseStatPercent("health", vitalsGain * (1.0 / boundPL));
    vict->gainBaseStatPercent("ki", vitalsGain * (1.0 / boundKI));
    vict->gainBaseStatPercent("stamina", vitalsGain * (1.0 / boundST));

    ch->send_to("Granted RP rewards to %s", vict->getName());
    log_imm_action("RP Reward: %s granted %s an RP reward!", ch, vict);
}

ACMD(do_eratime)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!*arg1)
    {
        ch->send_to("The server has been up for: %s Years, %s Months, %s Days and %s Hours.\r\n", std::to_string(era_uptime.year), std::to_string(era_uptime.month), std::to_string(era_uptime.day), std::to_string(era_uptime.hours));
    }

    int days = 0;
    int months = 0;
    int years = 0;

    if (*arg1 && !is_abbrev(arg1, "add") && !is_abbrev(arg1, "remove"))
    {
        ch->sendText("Syntax: eratime <add / remove> <days>\r\n");
    }

    if (is_abbrev(arg1, "add"))
    {
        if (!*arg2)
        {
            ch->sendText("Please add a number of days to advance time by.");
            return;
        }

        days = atoi(arg2);

        years = days / (int)DAYS_PER_YEAR;
        days = days % (int)DAYS_PER_YEAR;

        months = days / (int)DAYS_PER_MONTH;
        days = days % (int)DAYS_PER_MONTH;

        if (days + era_uptime.day >= DAYS_PER_MONTH)
        {
            months += 1;
            days -= DAYS_PER_MONTH;
        }
        if (months + era_uptime.month >= MONTHS_PER_YEAR)
        {
            years += 1;
            months -= MONTHS_PER_YEAR;
        }

        era_uptime.day += days;
        era_uptime.month += months;
        era_uptime.year += years;
        ch->send_to("Time advanced by %s days.\r\n", arg2);
    }

    if (is_abbrev(arg1, "remove"))
    {
        if (!*arg2)
        {
            ch->sendText("Please add a number of days to advance time by.");
            return;
        }

        days = atoi(arg2);

        years = days / (int)DAYS_PER_YEAR;
        days = days % (int)DAYS_PER_YEAR;

        months = days / (int)DAYS_PER_MONTH;
        days = days % (int)DAYS_PER_MONTH;

        if (era_uptime.year - years < 0)
        {
            years = era_uptime.year;
            months = era_uptime.month;
            days = era_uptime.day;
        }

        if (era_uptime.month - months < 0)
        {
            months = era_uptime.month;
            days = era_uptime.day;
        }

        if (era_uptime.day - days < 0)
        {
            days = era_uptime.day;
        }

        era_uptime.day -= days;
        era_uptime.month -= months;
        era_uptime.year -= years;

        ch->send_to("Time reduced by %s days.\r\n", arg2);
    }
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
    char buf[MAX_INPUT_LENGTH];
    Character *vict;
    Object *obj;

    one_argument(argument, buf);

    /* argument supplied. destroy single object or char */
    if (*buf)
    {
        if ((vict = get_char_vis(ch, buf, nullptr, FIND_CHAR_WORLD)))
        {
            if (!IS_NPC(vict) && (GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict)))
            {
                ch->sendText("Fuuuuuuuuu!\r\n");
                return;
            }
            act("$n disintegrates $N.", false, ch, nullptr, vict, TO_NOTVICT);
            if (!IS_NPC(vict))
            {
                send_to_all("@R%s@r purges @R%s's@r sorry ass right off the MUD!@n\r\n", GET_NAME(ch), GET_NAME(vict));
            }

            if (!IS_NPC(vict))
            {
                mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s has purged %s.", GET_NAME(ch),
                       GET_NAME(vict));
                log_imm_action("PURGED: %s burned %s's sorry ass off the MUD!", GET_NAME(ch), GET_NAME(vict));
                if (vict->desc)
                {
                    STATE(vict->desc) = CON_CLOSE;
                    vict->desc->character = nullptr;
                    vict->desc = nullptr;
                }
            }
            extract_char(vict);
        }
        else if ((obj = get_obj_in_list_vis(ch, buf, nullptr, ch->location.getObjects())))
        {
            act("$n destroys $p.", false, ch, obj, nullptr, TO_ROOM);
            extract_obj(obj);
        }
        else
        {
            ch->sendText("Nothing here by that name.\r\n");
            return;
        }

        ch->send_to("%s", CONFIG_OK);
    }
    else
    { /* no argument. clean out the room */
        int i;

        act("$n gestures... You are surrounded by scorching flames!",
            false, ch, nullptr, nullptr, TO_ROOM);
        ch->location.sendText("The world seems a little cleaner.\r\n");

        auto people = ch->location.getPeople();
        for (auto it : filter_raw(people))
        {
            vict = it;
            if (!IS_NPC(vict))
                continue;

            /* Dump inventory. */
            auto con = vict->getInventory();
            for (auto o : filter_raw(con))
                extract_obj(o);

            /* Dump equipment. */
            for (i = 0; i < NUM_WEARS; i++)
                if (GET_EQ(vict, i))
                    extract_obj(GET_EQ(vict, i));

            /* Dump character. */
            extract_char(vict);
        }

        /* Clear the ground. */
        auto con = ch->location.getObjects();
        for (auto o : filter_raw(con))
            extract_obj(o);
    }
}

static const char *logtypes[] = {
    "off", "brief", "normal", "complete", "\n"};

ACMD(do_syslog)
{
    char arg[MAX_INPUT_LENGTH];
    int tp;

    one_argument(argument, arg);
    if (!*arg)
    {
        ch->send_to("Your syslog is currently %s.\r\n", logtypes[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)]);
        return;
    }
    if (((tp = search_block(arg, logtypes, false)) == -1))
    {
        ch->sendText("Usage: syslog { Off | Brief | Normal | Complete }\r\n");
        return;
    }
    for (auto f : {PRF_LOG1, PRF_LOG2})
        ch->pref_flags.set(f, false);
    if (tp & 1)
        ch->pref_flags.set(PRF_LOG1, true);
    if (tp & 2)
        ch->pref_flags.set(PRF_LOG2, true);

    ch->send_to("Your syslog is now %s.\r\n", logtypes[tp]);
}

/* (c) 1996-97 Erwin S. Andreasen <erwin@pip.dknet.dk> */
ACMD(do_copyover)
{
#ifdef CIRCLE_WINDOWS
    ch->sendText("Copyover is not available for Windows.\r\n");
#else
    char arg[MAX_INPUT_LENGTH];
    int secs;

    one_argument(argument, arg);
    if (!*arg)
    {
        execute_copyover();
    }
    else if (is_abbrev(arg, "cancel") || is_abbrev(arg, "stop"))
    {
        if (!copyover_timer)
        {
            ch->sendText("A timed copyover has not been started!\r\n");
        }
        else
        {
            copyover_timer = 0;
            game_info("Copyover cancelled");
        }
    }
    else if (is_abbrev(arg, "help"))
    {
        ch->sendText("COPYOVER\r\n\r\n");
        ch->sendText("Usage: @ycopyover@n           - Perform an immediate copyover\r\n"
                     "       @ycopyover <seconds>@n - Start a timed copyover\r\n"
                     "       @ycopyover cancel@n    - Stop a timed copyover\r\n\r\n");
        ch->sendText("A timed copyover will produce an automatic warning when it starts, and then\r\n");
        ch->sendText("every minute.  During the last minute, there will be a warning every 15 seconds.\r\n");
    }
    else
    {
        secs = atoi(arg);
        if (!secs || secs < 0)
        {
            ch->sendText("Type @ycopyover help@n for usage info.");
        }
        else
        {
            copyover_timer = secs;
            basic_mud_log("-- Timed Copyover started by %s - %d seconds until copyover --", GET_NAME(ch), secs);
            if (secs >= 60)
            {
                if (secs % 60)
                {
                    game_info("A copyover will be performed in %d minutes and %d seconds.", (copyover_timer / 60),
                              (copyover_timer % 60));
                }
                else
                {
                    game_info("A copyover will be performed in %d minute%s.", (copyover_timer / 60),
                              (copyover_timer / 60) > 1 ? "s" : "");
                }
            }
            else
            {
                game_info("A copyover will be performed in %d seconds.", copyover_timer);
            }
        }
    }
#endif
}

static void execute_copyover()
{
    circle_shutdown = 1;
    circle_reboot = 1;
}

void copyover_check(uint64_t heartPulse, double deltaTime)
{
    if (!copyover_timer)
        return;
    copyover_timer--;
    if (!copyover_timer)
    {
        execute_copyover();
    }
    if (copyover_timer > 59)
    {
        if (copyover_timer % 60 == 0)
        {
            game_info("A copyover will be performed in %d minute%s.", (copyover_timer / 60),
                      (copyover_timer / 60) > 1 ? "s" : "");
        }
    }
    else
    {
        if (copyover_timer % 10 == 0 && copyover_timer > 29)
        {
            game_info("A copyover will be performed in %d seconds.", (copyover_timer));
        }
        if (copyover_timer % 5 == 0 && copyover_timer <= 29)
        {
            game_info("A copyover will be performed in %d seconds.", (copyover_timer));
        }
    }
}

ACMD(do_advance)
{
    Character *victim;
    char name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
    int newlevel, oldlevel;

    two_arguments(argument, name, level);

    if (*name)
    {
        if (!(victim = get_char_vis(ch, name, nullptr, FIND_CHAR_WORLD)))
        {
            ch->sendText("That player is not here.\r\n");
            return;
        }
    }
    else
    {
        ch->sendText("Advance who?\r\n");
        return;
    }

    if (IS_NPC(victim))
    {
        ch->sendText("NO!  Not on NPC's.\r\n");
        return;
    }
    if (!*level)
    {
        ch->sendText("[ 1 - 100 | demote ]\r\n");
        return;
    }
    else if ((newlevel = atoi(level)) <= 0)
    {
        if (!strcasecmp("demote", level))
        {
            victim->setBaseStat<int>("level", 1);
            victim->setBaseStat("health", 150);
            victim->setBaseStat("ki", 150);
            victim->setBaseStat("stamina", 150);
            ch->sendText("They have now been demoted!\r\n");
            victim->sendText("You were demoted to level 1!\r\n");
            return;
        }
        else
        {
            ch->sendText("That's not a level!\r\n");
            return;
        }
    }
    if (newlevel > 100)
    {
        ch->sendText("100 is the highest possible level.\r\n");
        return;
    }
    if (newlevel == GET_LEVEL(victim))
    {
        ch->sendText("They are already at that level.\r\n");
        return;
    }
    oldlevel = GET_LEVEL(victim);
    if (newlevel < GET_LEVEL(victim))
    {
        ch->sendText("You cannot demote a player.\r\n");
    }
    else
    {
        act("$n makes some strange gestures.\r\n"
            "A strange feeling comes upon you, like a giant hand, light comes down\r\n"
            "from above, grabbing your body, which begins to pulse with colored\r\n"
            "lights from inside.\r\n\r\n"
            "Your head seems to be filled with demons from another plane as your\r\n"
            "body dissolves to the elements of time and space itself.\r\n\r\n"
            "Suddenly a silent explosion of light snaps you back to reality.\r\n\r\n"
            "You feel slightly different.",
            false, ch, nullptr, victim, TO_VICT);
    }

    ch->send_to("%s", CONFIG_OK);

    if (newlevel < oldlevel)
        basic_mud_log("(GC) %s demoted %s from level %d to %d.",
                      GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
    else
        basic_mud_log("(GC) %s has advanced %s to level %d (from %d)",
                      GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

    int gain = level_exp(victim, newlevel) - GET_EXP(victim);
    victim->modExperience(gain);
}

ACMD(do_handout)
{
    struct descriptor_data *j;

    if (GET_ADMLEVEL(ch) < 3)
    {
        ch->sendText("You can't do that.\r\n");
        return;
    }

    for (j = descriptor_list; j; j = j->next)
    {
        if (!IS_PLAYING(j) || ch == j->character || GET_ADMLEVEL(j->character) > 0)
            continue;
        if (IS_NPC(j->character))
            continue;
        else
        {
            j->character->modPractices(10);
        }
    }

    send_to_all("@g%s@G hands out 10 practice sessions to everyone!@n\r\n", GET_NAME(ch));
    basic_mud_log("%s gave a handout of 10 PS to everyone.", GET_NAME(ch));
    log_imm_action("HANDOUT: %s has handed out 10 PS to everyone.", GET_NAME(ch));
}

ACMD(do_restore)
{
    char buf[MAX_INPUT_LENGTH];
    Character *vict;
    struct descriptor_data *j;
    int i;

    one_argument(argument, buf);
    if (!*buf)
        ch->sendText("Whom do you wish to restore?\r\n");
    else if (is_abbrev(buf, "all"))
    {
        send_to_imm("[Log: %s restored all.]", GET_NAME(ch));
        log_imm_action("RESTORE: %s has restored all players.", GET_NAME(ch));
        for (j = descriptor_list; j; j = j->next)
        {
            if (!IS_PLAYING(j) || !(vict = j->character))
                continue;
            vict->restore_by(ch);
        }
        ch->sendText("Okay.\r\n");
    }
    else if (!(vict = get_char_vis(ch, buf, nullptr, FIND_CHAR_WORLD)))
        ch->send_to("%s", CONFIG_NOPERSON);
    else if (!IS_NPC(vict) && ch != vict && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch))
        ch->sendText("They don't need your help.\r\n");
    else
    {
        vict->restore_by(ch);
        ch->send_to("%s", CONFIG_OK);
        send_to_imm("[Log: %s restored %s.]", GET_NAME(ch), GET_NAME(vict));
        log_imm_action("RESTORE: %s has restored %s.", GET_NAME(ch), GET_NAME(vict));
    }
}

void perform_immort_vis(Character *ch)
{
    ch->setBaseStat("invis_level", 0);
}

static void perform_immort_invis(Character *ch, int level)
{
    auto people = ch->location.getPeople();
    for (auto tch : filter_raw(people))
    {
        if (tch == ch)
            continue;
        if (GET_ADMLEVEL(tch) >= GET_INVIS_LEV(ch) && GET_ADMLEVEL(tch) < level)
            act("You blink and suddenly realize that $n is gone.", false, ch, nullptr,
                tch, TO_VICT);
        if (GET_ADMLEVEL(tch) < GET_INVIS_LEV(ch) && GET_ADMLEVEL(tch) >= level)
            act("You suddenly realize that $n is standing beside you.", false, ch, nullptr,
                tch, TO_VICT);
    }

    ch->setBaseStat("invis_level", level);
    ch->send_to("Your invisibility level is %d.\r\n", level);
}

ACMD(do_invis)
{
    char arg[MAX_INPUT_LENGTH];
    int level;

    if (IS_NPC(ch))
    {
        ch->sendText("You can't do that!\r\n");
        return;
    }

    one_argument(argument, arg);
    if (!*arg)
    {
        if (GET_INVIS_LEV(ch) > 0)
            perform_immort_vis(ch);
        else
            perform_immort_invis(ch, GET_ADMLEVEL(ch));
    }
    else
    {
        level = atoi(arg);
        if (level > GET_ADMLEVEL(ch))
            ch->sendText("You can't go invisible above your own level.\r\n");
        else if (level < 1)
            perform_immort_vis(ch);
        else
            perform_immort_invis(ch, level);
    }
}

ACMD(do_gecho)
{
    struct descriptor_data *pt;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument)
        ch->sendText("That must be a mistake...\r\n");
    else
    {
        for (pt = descriptor_list; pt; pt = pt->next)
            if (IS_PLAYING(pt) && pt->character && pt->character != ch)
                pt->character->send_to("%s\r\n", argument);

        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
            ch->send_to("%s", CONFIG_OK);
        else
            ch->send_to("%s\r\n", argument);
    }
}

ACMD(do_ginfo)
{
    struct descriptor_data *pt;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument)
        ch->sendText("That must be a mistake...\r\n");
    else
    {
        for (pt = descriptor_list; pt; pt = pt->next)
            if (IS_PLAYING(pt) && pt->character && pt->character != ch)
                pt->character->send_to("@D[@GINFO@D] @g%s@n\r\n", argument);

        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
            ch->send_to("%s", CONFIG_OK);
        else
            ch->send_to("@D[@GINFO@D] @g%s@n\r\n", argument);
    }
}

ACMD(do_poofset)
{
    char **msg;

    switch (subcmd)
    {
    case SCMD_POOFIN:
        msg = &(POOFIN(ch));
        break;
    case SCMD_POOFOUT:
        msg = &(POOFOUT(ch));
        break;
    default:
        return;
    }

    skip_spaces(&argument);

    if (*msg)
        free(*msg);

    if (!*argument)
        *msg = nullptr;
    else
        *msg = strdup(argument);

    ch->send_to("%s", CONFIG_OK);
}

ACMD(do_dc)
{
    ch->sendText("temporarily disabled.");
    return;
}

ACMD(do_wizlock)
{
    char arg[MAX_INPUT_LENGTH];
    int value;
    const char *when;

    one_argument(argument, arg);
    if (*arg)
    {
        value = atoi(arg);
        if (value < 0 || value > 101)
        {
            ch->sendText("Invalid wizlock value.\r\n");
            return;
        }
        circle_restrict = value;
        when = "now";
    }
    else
        when = "currently";

    if (*arg)
    {
        switch (circle_restrict)
        {
        case 0:
            ch->send_to("The game is %s completely open.\r\n", when);
            send_to_all("@RWIZLOCK@D: @WThe game has been completely opened by @C%s@W.@n", GET_NAME(ch));
            basic_mud_log("WIZLOCK: The game has been completely opened by %s.", GET_NAME(ch));
            break;
        case 1:
            ch->send_to("The game is %s closed to new players.\r\n", when);
            send_to_all("@RWIZLOCK@D: @WThe game is %s closed to new players by @C%s@W.@n", when, GET_NAME(ch));
            basic_mud_log("WIZLOCK: The game is %s closed to new players by %s.", when, GET_NAME(ch));
            break;
        case 101:
            ch->send_to("The game is %s closed to non-imms.\r\n", when);
            send_to_all("@RWIZLOCK@D: @WThe game is %s closed to non-imms by @C%s@W.@n", when, GET_NAME(ch));
            basic_mud_log("WIZLOCK: The game is %s closed to non-imms by %s.", when, GET_NAME(ch));
            break;
        default:
            ch->send_to("Only level %d+ may enter the game %s.\r\n", circle_restrict, when);
            send_to_all("@RWIZLOCK@D: @WLevel %d+ only can enter the game %s, thanks to @C%s@W.@n", circle_restrict,
                        when, GET_NAME(ch));
            basic_mud_log("WIZLOCK: Level %d+ only can enter the game %s, thanks to %s.", circle_restrict, when,
                          GET_NAME(ch));
            break;
        }
    }
    if (!*arg)
    {
        switch (circle_restrict)
        {
        case 0:
            ch->send_to("The game is %s completely open.\r\n", when);
            break;
        case 1:
            ch->send_to("The game is %s closed to new players.\r\n", when);
            break;
        default:
            ch->send_to("Only level %d and above may enter the game %s.\r\n", circle_restrict, when);
            break;
        }
    }
}

ACMD(do_date)
{
    char *tmstr;
    time_t mytime;
    int d, h, m;

    if (subcmd == SCMD_DATE)
        mytime = time(nullptr);
    else
        mytime = boot_time;

    tmstr = (char *)asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    if (subcmd == SCMD_DATE)
        ch->send_to("Current machine time: %s\r\n", tmstr);
    else
    {
        mytime = time(nullptr) - boot_time;
        d = mytime / 86400;
        h = (mytime / 3600) % 24;
        m = (mytime / 60) % 60;

        ch->send_to("Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, d == 1 ? "" : "s", h, m);
    }
}

ACMD(do_last)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (!*arg)
    {
        ch->sendText("For whom do you wish to search?\r\n");
        return;
    }
    auto vict = findPlayer(arg);

    if (!vict)
    {
        ch->sendText("There is no such player.\r\n");
        return;
    }
    if ((GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch)) && (GET_ADMLEVEL(ch) < ADMLVL_IMPL))
    {
        ch->sendText("You are not sufficiently godly for that!\r\n");
        return;
    }

    ch->send_to("[%5d] [%2d %s %s] %-12s : %-18s : %-20s\r\n", GET_IDNUM(vict), (int)GET_LEVEL(vict), race::getAbbr(vict->race).c_str(), CLASS_ABBR(vict), GET_NAME(vict), "(FIXHOSTPLZ)", ctime(&vict->time.logon));
}

ACMD(do_force)
{
    struct descriptor_data *i, *next_desc;
    Character *vict, *next_force;
    char arg[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH + 32];

    half_chop(argument, arg, to_force);

    snprintf(buf1, sizeof(buf1), "$n has forced you to '%s'.", to_force);

    if (!*arg || !*to_force)
        ch->sendText("Whom do you wish to force do what?\r\n");
    else if (!ADM_FLAGGED(ch, ADM_FORCEMASS) || (strcasecmp("all", arg) && strcasecmp("room", arg)))
    {
        if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
            ch->send_to("%s", CONFIG_NOPERSON);
        else if (!IS_NPC(vict) && GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict))
            ch->sendText("No, no, no!\r\n");
        else
        {
            ch->send_to("%s", CONFIG_OK);
            act(buf1, true, ch, nullptr, vict, TO_VICT);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s forced %s to %s", GET_NAME(ch),
                   GET_NAME(vict), to_force);
            command_interpreter(vict, to_force);
        }
    }
    else if (!strcasecmp("room", arg))
    {
        ch->send_to("%s", CONFIG_OK);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s forced room %d to %s",
               GET_NAME(ch), ch->location.getVnum(), to_force);
        auto people = ch->location.getPeople();
        for (auto target : filter_raw(people))
        {
            vict = target;
            if (!IS_NPC(vict) && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch))
                continue;
            act(buf1, true, ch, nullptr, vict, TO_VICT);
            command_interpreter(vict, to_force);
        }
    }
    else
    { /* force all */
        ch->send_to("%s", CONFIG_OK);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s forced all to %s", GET_NAME(ch), to_force);

        for (i = descriptor_list; i; i = next_desc)
        {
            next_desc = i->next;

            if (STATE(i) != CON_PLAYING || !(vict = i->character) ||
                (!IS_NPC(vict) && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch)))
                continue;
            act(buf1, true, ch, nullptr, vict, TO_VICT);
            command_interpreter(vict, to_force);
        }
    }
}

ACMD(do_wiznet)
{
    char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
        buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32], *msg;
    struct descriptor_data *d;
    char emote = false;
    char any = false;
    int level = ADMLVL_IMMORT;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument)
    {
        ch->sendText("Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n        wiznet @<level> *<emotetext> | wiz @\r\n");
        return;
    }
    switch (*argument)
    {
    case '*':
        emote = true;
    case '#':
        one_argument(argument + 1, buf1);
        if (is_number(buf1))
        {
            half_chop(argument + 1, buf1, argument);
            level = MAX(atoi(buf1), ADMLVL_IMMORT);
            if (level > GET_ADMLEVEL(ch))
            {
                ch->sendText("You can't wizline above your own level.\r\n");
                return;
            }
        }
        else if (emote)
            argument++;
        break;

    case '@':
        ch->sendText("God channel status:\r\n");
        for (any = 0, d = descriptor_list; d; d = d->next)
        {
            if (STATE(d) != CON_PLAYING || GET_ADMLEVEL(d->character) < ADMLVL_IMMORT)
                continue;
            if (!CAN_SEE(ch, d->character))
                continue;

            ch->send_to("  %-*s%s%s%s\r\n", MAX_NAME_LENGTH, GET_NAME(d->character), PLR_FLAGGED(d->character, PLR_WRITING) ? " (Writing)" : "", PLR_FLAGGED(d->character, PLR_MAILING) ? " (Writing mail)" : "", PRF_FLAGGED(d->character, PRF_NOWIZ) ? " (Offline)" : "");
        }
        return;

    case '\\':
        ++argument;
        break;
    default:
        break;
    }
    if (PRF_FLAGGED(ch, PRF_NOWIZ))
    {
        ch->sendText("You are offline!\r\n");
        return;
    }
    skip_spaces(&argument);

    if (!*argument)
    {
        ch->sendText("Don't bother the gods like that!\r\n");
        return;
    }
    if (level > ADMLVL_IMMORT)
    {
        snprintf(buf1, sizeof(buf1), "@c%s@D: <@C%d@D> @G%s%s@n\r\n", GET_NAME(ch), level, emote ? "<--- " : "",
                 argument);
        snprintf(buf2, sizeof(buf1), "@cSomeone@D: <@C%d@D> @G%s%s@n\r\n", level, emote ? "<--- " : "", argument);
    }
    else
    {
        snprintf(buf1, sizeof(buf1), "@c%s@D: @G%s%s@n\r\n", GET_NAME(ch), emote ? "<--- " : "", argument);
        snprintf(buf2, sizeof(buf1), "@cSomeone@D: @G%s%s@n\r\n", emote ? "<--- " : "", argument);
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (IS_PLAYING(d) && (GET_ADMLEVEL(d->character) >= level) &&
            (!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
            (!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING)) && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT))))
        {
            if (CAN_SEE(d->character, ch))
            {
                msg = strdup(buf1);
                d->character->send_to("%s", buf1);
            }
            else
            {
                msg = strdup(buf2);
                d->character->send_to("%s", buf2);
            }
            add_history(d->character, msg, HIST_WIZNET);
        }
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        ch->send_to("%s", CONFIG_OK);
}

ACMD(do_zreset)
{
    char arg[MAX_INPUT_LENGTH];
    zone_rnum i;
    zone_vnum j;

    one_argument(argument, arg);

    if (*arg == '*')
    {
        if (GET_ADMLEVEL(ch) < ADMLVL_VICE)
        {
            ch->sendText("You do not have permission to reset the entire world.\r\n");
            return;
        }
        else
        {
            for (auto &z : zone_table)
            {
                if (z.first < 200)
                {
                    reset_zone(z.first);
                }
            }
            ch->sendText("Reset world.\r\n");
            mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true, "(GC) %s reset all MUD zones.", GET_NAME(ch));
            log_imm_action("RESET: %s has reset all MUD zones.", GET_NAME(ch));
            return;
        }
    }
    else if (*arg == '.' || !*arg)
        i = ch->location.getZone()->number;
    else
    {
        i = atoi(arg);
    }
    if (!zone_table.count(i) || !(can_edit_zone(ch, i) || GET_ADMLEVEL(ch) > ADMLVL_IMMORT))
    {
        ch->send_to("You do not have permission to reset this zone. Try %d.\r\n", GET_OLC_ZONE(ch));
        return;
    }
    auto &z = zone_table.at(i);
    reset_zone(z.number);
    ch->send_to("Reset zone #%d: %s.\r\n", z.number, z.name.c_str());
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true, "(GC) %s reset zone %d (%s)", GET_NAME(ch),
           z.number, z.name.c_str());
    log_imm_action("RESET: %s has reset zone #%d: %s.", GET_NAME(ch), z.number, z.name.c_str());
}

/*
 *  General fn for wizcommands of the sort: cmd <player>
 */
ACMD(do_wizutil)
{
    char arg[MAX_INPUT_LENGTH];
    Character *vict;
    int taeller;
    long result;

    one_argument(argument, arg);

    if (!*arg)
        ch->sendText("Yes, but for whom?!?\r\n");
    else if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
        ch->sendText("There is no such player.\r\n");
    else if (IS_NPC(vict))
        ch->sendText("You can't do that to a mob!\r\n");
    else if (GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch))
        ch->sendText("Hmmm...you'd better not.\r\n");
    else
    {
        switch (subcmd)
        {
        case SCMD_REROLL:
            ch->sendText("Rerolling is not possible at this time, bug Iovan about it...\r\n");
            basic_mud_log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
            ch->send_to("New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n", GET_STR(vict), GET_INT(vict), GET_WIS(vict), GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
            break;
        case SCMD_PARDON:
            if (!PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(vict, PLR_KILLER))
            {
                ch->sendText("Your victim is not flagged.\r\n");
                return;
            }
            for (auto f : {PLR_THIEF, PLR_KILLER})
                vict->player_flags.set(f, false);
            ch->sendText("Pardoned.\r\n");
            vict->sendText("You have been pardoned by the Gods!\r\n");
            mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s pardoned by %s", GET_NAME(vict),
                   GET_NAME(ch));
            break;
        case SCMD_NOTITLE:
            result = vict->player_flags.toggle(PLR_NOTITLE);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) Notitle %s for %s by %s.",
                   ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            ch->send_to("(GC) Notitle %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            break;
        case SCMD_SQUELCH:
            result = vict->player_flags.toggle(PLR_NOSHOUT);
            mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) Squelch %s for %s by %s.",
                   ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            ch->send_to("(GC) Mute turned %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            send_to_all("@D[@RMUTE@D] @C%s@W has had mute turned @r%s@W by @C%s@W.\r\n", GET_NAME(vict),
                        ONOFF(result), GET_NAME(ch));
            break;
        case SCMD_FREEZE:
            if (ch == vict)
            {
                ch->sendText("Oh, yeah, THAT'S real smart...\r\n");
                return;
            }
            if (GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict))
            {
                ch->sendText("Pfft...\r\n");
                return;
            }
            if (PLR_FLAGGED(vict, PLR_FROZEN))
            {
                ch->sendText("Your victim is already pretty cold.\r\n");
                return;
            }
            vict->player_flags.set(PLR_FROZEN, true);
            vict->setBaseStat("freeze_level", GET_ADMLEVEL(ch));
            vict->sendText("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
            ch->sendText("Frozen.\r\n");
            act("A sudden cold wind conjured from nowhere freezes $n!", false, vict, nullptr, nullptr, TO_ROOM);
            mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s frozen by %s.", GET_NAME(vict),
                   GET_NAME(ch));
            break;
        case SCMD_THAW:
            if (!PLR_FLAGGED(vict, PLR_FROZEN))
            {
                ch->sendText("Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
                return;
            }
            if (GET_FREEZE_LEV(vict) > GET_ADMLEVEL(ch))
            {
                ch->send_to("Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n", GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
                return;
            }
            mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s un-frozen by %s.", GET_NAME(vict),
                   GET_NAME(ch));
            vict->player_flags.set(PLR_FROZEN, false);
            vict->sendText("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
            ch->sendText("Thawed.\r\n");
            act("A sudden fireball conjured from nowhere thaws $n!", false, vict, nullptr, nullptr, TO_ROOM);
            break;
        case SCMD_UNAFFECT:
            ch->sendText("Disabled.\r\n");
            break;
        default:
            basic_mud_log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
            /*  SYSERR_DESC:
             *  This is the same as the unhandled case in do_gen_ps(), but this
             *  function handles 'reroll', 'pardon', 'freeze', etc.
             */
            break;
        }
    }
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

/* FIXME: overflow possible */
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall)
{
    size_t tmp;
    auto &z = zone_table.at(zone);
    if (listall)
    {

        tmp = snprintf(bufptr, left,
                       "%3d %-30.30s By: %-10.10s Age: %3f; Reset: %3d (%1d)\r\n",
                       z.number, z.name.c_str(), z.builders.c_str(),
                       z.age, z.lifespan,
                       z.reset_mode);

        auto j = z.rooms.live_count();

        tmp += snprintf(bufptr + tmp, left - tmp,
                        "       Zone stats:\r\n"
                        "       ---------------\r\n"
                        "         Rooms:    %2lu\r\n",
                        j);

        return tmp;
    }

    return snprintf(bufptr, left,
                    "%3d %-*s By: %-10.10s\r\n", z.number,
                    count_color_chars(z.name.c_str()) + 30, z.name.c_str(),
                    z.builders.c_str());
}

ACMD(do_show)
{
    int i, j, k, l, con; /* i, j, k to specifics? */
    size_t len, nlen;
    zone_rnum zrn;
    zone_vnum zvn;
    int low, high;
    int8_t self = false;
    Character *vict = nullptr;
    Object *obj;
    struct descriptor_data *d;
    struct affected_type *aff;
    char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], *strp,
        arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];

    struct show_struct
    {
        const char *cmd;
        const char level;
    } fields[] = {
        {"nothing", 0},           /* 0 */
        {"zones", ADMLVL_IMMORT}, /* 1 */
        {"player", ADMLVL_GOD},
        {"rent", ADMLVL_GRGOD},
        {"stats", ADMLVL_IMMORT},
        {"errors", ADMLVL_IMPL}, /* 5 */
        {"death", ADMLVL_GOD},
        {"godrooms", ADMLVL_IMMORT},
        {"shops", ADMLVL_IMMORT},
        {"houses", ADMLVL_GOD},
        {"snoop", ADMLVL_GRGOD}, /* 10 */
        {"assemblies", ADMLVL_IMMORT},
        {"guilds", ADMLVL_GOD},
        {"levels", ADMLVL_GRGOD},
        {"uniques", ADMLVL_GRGOD},
        {"affect", ADMLVL_GRGOD}, /* 15 */
        {"affectv", ADMLVL_GRGOD},
        {"\n", 0}};

    skip_spaces(&argument);

    if (!*argument)
    {
        ch->sendText("Game Info options:\r\n");
        for (j = 0, i = 1; fields[i].level; i++)
            if (fields[i].level <= GET_ADMLEVEL(ch))
                ch->send_to("%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
        ch->sendText("\r\n");
        return;
    }

    strcpy(arg, two_arguments(argument, field, value)); /* strcpy: OK (argument <= MAX_INPUT_LENGTH == arg) */

    for (l = 0; *(fields[l].cmd) != '\n'; l++)
        if (!strncmp(field, fields[l].cmd, strlen(field)))
            break;

    if (GET_ADMLEVEL(ch) < fields[l].level)
    {
        ch->sendText("You are not godly enough for that!\r\n");
        return;
    }
    if (!strcmp(value, "."))
        self = true;
    buf[0] = '\0';

    switch (l)
    {
    /* show zone */
    case 1:
        /* tightened up by JE 4/6/93 */
        if (self)
            print_zone_to_buf(buf, sizeof(buf), ch->location.getZone()->number, 1);
        else if (*value && is_number(value))
        {
            zvn = real_zone(atoi(value));
            if (zvn == NOBODY)
            {
                ch->sendText("That is not a valid zone.\r\n");
                return;
            }
            print_zone_to_buf(buf, sizeof(buf), zrn, 1);
        }
        else
        {
            len = 0;
            for (auto &z : zone_table)
            {
                nlen = print_zone_to_buf(buf + len, sizeof(buf) - len, z.first, 0);
                if (len + nlen >= sizeof(buf) || nlen < 0)
                    break;
                len += nlen;
            }
        }
        if (ch->desc)
            ch->desc->send_to("%s", buf);
        break;

        /* show player */
    case 2:
        if (!*value)
        {
            ch->sendText("A name would help.\r\n");
            return;
        }
        vict = findPlayer(value);
        if (!vict)
        {
            ch->sendText("There is no such player.\r\n");
            return;
        }
        ch->send_to("Player: %-12s (%s) [%2d %s %s]\r\n", GET_NAME(vict), genders[(int)GET_SEX(vict)], GET_LEVEL(vict), CLASS_ABBR(vict), race::getAbbr(vict->race).c_str());
        ch->send_to("Au: %-8d  Bal: %-8d  Exp: %" I64T "  Align: %-5d  Ethic: %-5d\r\n", GET_GOLD(vict), GET_BANK_GOLD(vict), GET_EXP(vict), GET_ALIGNMENT(vict), GET_ETHIC_ALIGNMENT(vict));

        /* ctime() uses static buffer: do not combine. */
        ch->send_to("Started: %-20.16s  ", ctime(&vict->time.created));
        ch->send_to("Last: %-20.16s  Played: %3dh %2dm\r\n", ctime(&vict->time.logon), (int)((int64_t)vict->time.played / 3600), (int)((int64_t)vict->time.played / 60 % 60));
        break;

        /* show rent */
    case 3:
        if (!*value)
        {
            ch->sendText("A name would help.\r\n");
            return;
        }
        break;

        /* show stats */
    case 4:
        i = 0;
        j = 0;
        k = uniqueObjects.size();
        con = sessions.size();
        for (auto &[id, ent] : uniqueCharacters)
        {
            vict = ent.get();
            if (IS_NPC(vict))
                j++;
            else if (CAN_SEE(ch, vict))
            {
                i++;
                if (vict->desc)
                    con++;
            }
        }
        ch->send_to("             @D---   @CCore Stats   @D---\r\n"
                    "  @Y%5d@W players in game  @y%5d@W connected\r\n"
                    "  @Y%5d@W registered\r\n"
                    "  @Y%5d@W mobiles          @y%5d@W prototypes\r\n"
                    "  @Y%5d@W objects          @y%5d@W prototypes\r\n"
                    "  @Y%5d@W rooms            @y%5d@W zones\r\n"
                    "  @Y%5d@W triggers\r\n"
                    "  @Y%5d@W large bufs\r\n"
                    "  @Y%5d@W buf switches     @y%5d@W overflows\r\n"
                    "             @D--- @CMiscellaneous  @D---\r\n"
                    "  @Y%5s@W Mob ki attacks this boot\r\n"
                    "  @Y%5s@W Asssassins Generated@n\r\n"
                    "  @Y%5d@W Wish Selfishness Meter@n\r\n",
                    i, con, players.size(), j, mob_proto.size(), k, obj_proto.size(), world.size(), zone_table.size(), trig_index.size(), buf_largecount, buf_switches, buf_overflows, add_commas(mob_specials_used).c_str(), add_commas(number_of_assassins).c_str(), SELFISHMETER);
        break;

        /* show errors */
    case 5:
        len = strlcpy(buf, "Errant Rooms\r\n------------\r\n", sizeof(buf));
        for (auto &[vn, r] : world)
        {
            for (auto &[d, e] : r->getDirections())
            {
                if (e == 0)
                {
                    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (void   ) [%5d] %-*s%s (%s)\r\n", ++k,
                                    vn, count_color_chars(r->getName()) + 40, r->getName(), QNRM,
                                    dirs[j]);
                    if (len + nlen >= sizeof(buf) || nlen < 0)
                        break;
                    len += nlen;
                }
                if (e == NOWHERE && !e.general_description.empty())
                {
                    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (Nowhere) [%5d] %-*s%s (%s)\r\n", ++k,
                                    vn, count_color_chars(r->getName()) + 40, r->getName(), QNRM,
                                    dirs[j]);
                    if (len + nlen >= sizeof(buf) || nlen < 0)
                        break;
                    len += nlen;
                }
            }
        }

        if (ch->desc)
            ch->desc->send_to("%s", buf);
        break;

        /* show godrooms */
    case 7:
        j = 0;
        len = strlcpy(buf, "Godrooms\r\n--------------------------\r\n", sizeof(buf));
        for (auto &[vn, r] : world)
            if (ROOM_FLAGGED(r.get(), ROOM_GODROOM))
            {
                nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, vn,
                                r->getName());
                if (len + nlen >= sizeof(buf) || nlen < 0)
                    break;
                len += nlen;
            }
        if (ch->desc)
            ch->desc->send_to("%s", buf);
        break;

        /* show shops */
    case 8:
        show_shops(ch, value);
        break;

        /* show houses */
    case 9:
        break;

        /* show snoop */
    case 10:
        i = 0;
        ch->sendText("People currently snooping:\r\n--------------------------\r\n");
        for (d = descriptor_list; d; d = d->next)
        {
            if (d->snooping == nullptr || d->character == nullptr)
                continue;
            if (STATE(d) != CON_PLAYING || GET_ADMLEVEL(ch) < GET_ADMLEVEL(d->character))
                continue;
            if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
                continue;
            i++;
            ch->send_to("%-10s - snooped by %s.\r\n", GET_NAME(d->snooping->character), GET_NAME(d->character));
        }
        if (i == 0)
            ch->sendText("No one is currently snooping.\r\n");
        break;
        /* show assembly */
    case 11:
        assemblyListToChar(ch);
        break;
        /* show guilds */
    case 12:
        show_guild(ch, value);
        break;

        /* show level tables */
    case 13:
        ch->sendText("This is not used currently.\r\n");
        break;

    case 14:
        if (value && *value)
        {
            if (sscanf(value, "%d-%d", &low, &high) != 2)
            {
                if (sscanf(value, "%d", &low) != 1)
                {
                    ch->sendText("Usage: show uniques, show uniques [vnum], or show uniques [low-high]\r\n");
                    return;
                }
                else
                {
                    high = low;
                }
            }
        }
        else
        {
            low = -1;
            high = 9999999;
        }
        strp = sprintuniques(low, high);
        if (ch->desc)
            ch->desc->send_to("%s", strp);
        free(strp);
        break;

    case 15:
    case 16:
        if (!*value)
        {
            low = 1;
            vict = (l == 15) ? affect_list : affectv_list;
        }
        else
        {
            low = 0;
            if (!(vict = get_char_world_vis(ch, value, nullptr)))
            {
                ch->sendText("Cannot find that character.\r\n");
                return;
            }
        }
        k = MAX_STRING_LENGTH;
        CREATE(strp, char, k);
        strp[0] = j = 0;
        if (!vict)
        {
            ch->sendText("None.\r\n");
            return;
        }
        do
        {
            if ((k - j) < (MAX_INPUT_LENGTH * 8))
            {
                k *= 2;
                RECREATE(strp, char, k);
            }
            j += snprintf(strp + j, k - j, "Name: %s\r\n", GET_NAME(vict));
            if (l == 15)
                aff = vict->affected;
            else
                aff = vict->affectedv;
            for (; aff; aff = aff->next)
            {
                j += snprintf(strp + j, k - j, "SPL: (%3d%s) @c%-21s@n ", aff->duration + 1,
                              (l == 15) ? "hr" : "rd", skill_name(aff->type));

                if (aff->modifier)
                    j += snprintf(strp + j, k - j, "%+.2f to %s", aff->modifier,
                                  apply_types[(int)aff->location]);

                if (aff->bitvector)
                {
                    if (aff->modifier)
                        j += snprintf(strp + j, k - j, ", ");

                    strcpy(field, affected_bits[aff->bitvector]);
                    j += snprintf(strp + j, k - j, "sets %s", field);
                }
                j += snprintf(strp + j, k - j, "\r\n");
            }
            if (l == 15)
                vict = vict->next_affect;
            else
                vict = vict->next_affectv;
        } while (low && vict);
        if (ch->desc)
            ch->desc->send_to("%s", strp);
        free(strp);
        break;

        /* show what? */
    default:
        ch->sendText("Sorry, I don't understand that.\r\n");
        break;
    }
}

/***************** The do_set function ***********************************/

constexpr int PC = 1;
constexpr int NPC = 2;
constexpr int BOTH = 3;

constexpr int MISC = 0;
constexpr int BINARY = 1;
constexpr int NUMBER = 2;

#define SET_OR_REMOVE(flagset, flags) \
    {                                 \
        if (on)                       \
            flagset.set(flags);       \
        else if (off)                 \
            flagset.reset(flags);     \
    }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

/* The set options available */
static const struct set_struct
{
    const char *cmd;
    const char level;
    const char pcnpc;
    const char type;
} set_fields[] = {
    {"brief", ADMLVL_GOD, PC, BINARY},    /* 0 */
    {"invstart", ADMLVL_GOD, PC, BINARY}, /* 1 */
    {"title", ADMLVL_GOD, PC, MISC},
    {"nosummon", ADMLVL_GRGOD, PC, BINARY},
    {"maxpl", ADMLVL_BUILDER, BOTH, NUMBER},
    {"maxki", ADMLVL_BUILDER, BOTH, NUMBER}, /* 5 */
    {"maxst", ADMLVL_BUILDER, BOTH, NUMBER},
    {"pl", ADMLVL_BUILDER, BOTH, NUMBER},
    {"ki", ADMLVL_BUILDER, BOTH, NUMBER},
    {"sta", ADMLVL_BUILDER, BOTH, NUMBER},
    {"align", ADMLVL_BUILDER, BOTH, NUMBER}, /* 10 */
    {"str", ADMLVL_BUILDER, BOTH, NUMBER},
    {"stradd", ADMLVL_IMPL, BOTH, NUMBER},
    {"int", ADMLVL_BUILDER, BOTH, NUMBER},
    {"wis", ADMLVL_BUILDER, BOTH, NUMBER},
    {"dex", ADMLVL_BUILDER, BOTH, NUMBER}, /* 15 */
    {"con", ADMLVL_BUILDER, BOTH, NUMBER},
    {"cha", ADMLVL_BUILDER, BOTH, NUMBER},
    {"ac", ADMLVL_IMPL, BOTH, NUMBER},
    {"zenni", ADMLVL_BUILDER, BOTH, NUMBER},
    {"bank", ADMLVL_BUILDER, PC, NUMBER}, /* 20 */
    {"exp", ADMLVL_BUILDER, BOTH, NUMBER},
    {"accuracy", ADMLVL_VICE, BOTH, NUMBER},
    {"damage", ADMLVL_VICE, BOTH, NUMBER},
    {"invis", ADMLVL_IMPL, PC, NUMBER},
    {"nohassle", ADMLVL_VICE, PC, BINARY}, /* 25 */
    {"frozen", ADMLVL_FREEZE, PC, BINARY},
    {"practices", ADMLVL_BUILDER, PC, NUMBER},
    {"lessons", ADMLVL_IMPL, PC, NUMBER},
    {"drunk", ADMLVL_GOD, BOTH, MISC},
    {"hunger", ADMLVL_BUILDER, BOTH, MISC}, /* 30 */
    {"thirst", ADMLVL_BUILDER, BOTH, MISC},
    {"killer", ADMLVL_GOD, PC, BINARY},
    {"thief", ADMLVL_GOD, PC, BINARY},
    {"level", ADMLVL_GRGOD, BOTH, NUMBER},
    {"room", ADMLVL_IMPL, BOTH, NUMBER}, /* 35 */
    {"roomflag", ADMLVL_VICE, PC, BINARY},
    {"siteok", ADMLVL_VICE, PC, BINARY},
    {"deleted", ADMLVL_IMPL, PC, BINARY},
    {"class", ADMLVL_VICE, BOTH, MISC},
    {"nowizlist", ADMLVL_GOD, PC, BINARY}, /* 40 */
    {"quest", ADMLVL_GOD, PC, BINARY},
    {"loadroom", ADMLVL_VICE, PC, MISC},
    {"color", ADMLVL_GOD, PC, BINARY},
    {"idnum", ADMLVL_IMPL, PC, NUMBER},
    {"passwd", ADMLVL_IMPL, PC, MISC}, /* 45 */
    {"nodelete", ADMLVL_GOD, PC, BINARY},
    {"sex", ADMLVL_VICE, BOTH, MISC},
    {"age", ADMLVL_VICE, BOTH, NUMBER},
    {"height", ADMLVL_GOD, BOTH, NUMBER},
    {"weight", ADMLVL_GOD, BOTH, NUMBER}, /* 50 */
    {"olc", ADMLVL_GRGOD, PC, MISC},
    {"race", ADMLVL_VICE, PC, MISC},
    {"trains", ADMLVL_VICE, PC, NUMBER},
    {"feats", ADMLVL_VICE, PC, NUMBER},
    {"ethic", ADMLVL_GOD, BOTH, NUMBER}, /* 55 */
    {"unused1", ADMLVL_GRGOD, BOTH, NUMBER},
    {"unused2", ADMLVL_GRGOD, BOTH, NUMBER},
    {"adminlevel", ADMLVL_GRGOD, PC, NUMBER},
    {"hairl", ADMLVL_VICE, PC, NUMBER},
    {"hairs", ADMLVL_VICE, PC, NUMBER},
    {"hairc", ADMLVL_VICE, PC, NUMBER},
    {"skin", ADMLVL_VICE, PC, NUMBER},
    {"eye", ADMLVL_VICE, PC, NUMBER},
    {"basepl", ADMLVL_BUILDER, BOTH, NUMBER},
    {"baseki", ADMLVL_BUILDER, BOTH, NUMBER},
    {"basest", ADMLVL_BUILDER, BOTH, NUMBER},
    {"droom", ADMLVL_GRGOD, BOTH, NUMBER},
    {"absorbs", ADMLVL_GRGOD, BOTH, NUMBER},
    {"ugp", ADMLVL_GRGOD, BOTH, NUMBER},
    {"aura", ADMLVL_IMMORT, BOTH, NUMBER},
    {"trp", ADMLVL_GRGOD, BOTH, NUMBER},
    {"boost", ADMLVL_GRGOD, BOTH, NUMBER},
    {"multi", ADMLVL_IMPL, PC, BINARY},
    {"deaths", ADMLVL_BUILDER, BOTH, NUMBER},
    {"user", ADMLVL_IMPL, PC, MISC},
    {"phase", ADMLVL_IMMORT, PC, NUMBER},
    {"racial", ADMLVL_IMMORT, PC, NUMBER},
    {"slots", ADMLVL_IMMORT, PC, NUMBER},
    {"feature", ADMLVL_IMMORT, PC, BINARY},
    {"tclass", ADMLVL_VICE, PC, NUMBER},
    {"clones", ADMLVL_IMPL, PC, NUMBER},
    {"armor", ADMLVL_IMPL, PC, NUMBER},
    {"\n", 0, BOTH, MISC}};

static int perform_set(Character *ch, Character *vict, int mode,
                       char *val_arg)
{
    int i, on = 0, off = 0;
    int64_t value = 0;
    room_rnum rnum;
    room_vnum rvnum;

    /* Check to make sure all the levels are correct */
    if (GET_ADMLEVEL(ch) != ADMLVL_IMPL)
    {
        if (!IS_NPC(vict) && GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict) && vict != ch)
        {
            ch->sendText("Maybe that's not such a great idea...\r\n");
            return (0);
        }
    }
    if (GET_ADMLEVEL(ch) < set_fields[mode].level)
    {
        ch->sendText("You are not godly enough for that!\r\n");
        return (0);
    }

    /* Make sure the PC/NPC is correct */
    if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC))
    {
        ch->sendText("You can't do that to a beast!\r\n");
        return (0);
    }
    else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC))
    {
        ch->sendText("That can only be done to a beast!\r\n");
        return (0);
    }

    /* Find the value of the argument */
    if (set_fields[mode].type == BINARY)
    {
        if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
            on = 1;
        else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
            off = 1;
        if (!(on || off))
        {
            ch->sendText("Value must be 'on' or 'off'.\r\n");
            return (0);
        }
        ch->send_to("%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), GET_NAME(vict));
    }
    else if (set_fields[mode].type == NUMBER)
    {
        char *ptr;

        value = strtoll(val_arg, &ptr, 10);
        /*    value = atoi(val_arg); */
        ch->send_to("%s's %s set to %" I64T ".\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
    }
    else
        ch->send_to("%s", CONFIG_OK);

    switch (mode)
    {
    case 0:
        vict->pref_flags.toggle(PRF_BRIEF);
        break;
    case 1:
        vict->player_flags.toggle(PLR_INVSTART);
        break;
    case 2:
        set_title(vict, val_arg);
        ch->send_to("%s's title is now: %s\r\n", GET_NAME(vict), GET_TITLE(vict));
        break;
    case 3:
        vict->pref_flags.toggle(PRF_SUMMONABLE);
        ch->send_to("Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
        break;
    case 4:
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set maxpl for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set maxpl for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 5:
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set maxki for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set maxki for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 6:
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set maxsta for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set maxsta for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 7:
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set pl for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set pl for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 8:
        affect_total(vict);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set ki for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set ki for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 9:
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set st for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set st for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 10:
        vict->setBaseStat("good_evil", RANGE(-1000, 1000));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set align for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set align for %s.", GET_NAME(ch), GET_NAME(vict));
        affect_total(vict);
        break;
    case 11:
        RANGE(0, 100);
        vict->setBaseStat("strength", value);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set str for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set str for %s.", GET_NAME(ch), GET_NAME(vict));
        affect_total(vict);
        break;
    case 12:
        ch->sendText("Setting str_add does nothing now.\r\n");
        /* vict->real_abils.str_add = RANGE(0, 100);
if (value > 0)
  vict->real_abils.str = 18;
affect_total(vict);
   break; */
    case 13:
        RANGE(0, 100);
        vict->setBaseStat("intelligence", value);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set intel for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set intel for %s.", GET_NAME(ch), GET_NAME(vict));
        affect_total(vict);
        break;
    case 14:
        RANGE(0, 100);
        vict->setBaseStat("wisdom", value);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set wis for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set wis for %s.", GET_NAME(ch), GET_NAME(vict));
        affect_total(vict);
        break;
    case 15:
        RANGE(0, 100);
        vict->setBaseStat("agility", value);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set dex for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set dex for %s.", GET_NAME(ch), GET_NAME(vict));
        affect_total(vict);
        break;
    case 16:
        RANGE(0, 100);
        vict->setBaseStat("constitution", value);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set con for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set con for %s.", GET_NAME(ch), GET_NAME(vict));
        affect_total(vict);
        break;
    case 17:
        RANGE(0, 100);
        vict->setBaseStat("speed", value);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set speed for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set speed for %s.", GET_NAME(ch), GET_NAME(vict));
        affect_total(vict);
        break;
    case 18:
        vict->setBaseStat("armor_innate", RANGE(-100, 500));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set armor index for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set armor index for %s.", GET_NAME(ch), GET_NAME(vict));
        affect_total(vict);
        break;
    case 19:
        vict->setBaseStat("money_carried", RANGE(0, 100000000));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set zenni for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set zenni for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 20:
        vict->setBaseStat("money_bank", RANGE(0, 100000000));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set bank for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set bank for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 21:
        vict->setExperience(RANGE(0, 50000000));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set exp for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set exp for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 22:
        ch->sendText("This does nothing at the moment.\r\n");
        break;
    case 23:
        vict->setBaseStat("damage_mod", RANGE(-20, 20));
        affect_total(vict);
        break;
    case 24:
        if (GET_ADMLEVEL(ch) < ADMLVL_IMPL && ch != vict)
        {
            ch->sendText("You aren't godly enough for that!\r\n");
            return (0);
        }
        vict->setBaseStat("invis_level", RANGE(0, GET_ADMLEVEL(vict)));
        break;
    case 25:
        if (GET_ADMLEVEL(ch) < ADMLVL_IMPL && ch != vict)
        {
            ch->sendText("You aren't godly enough for that!\r\n");
            return (0);
        }
        vict->pref_flags.toggle(PRF_NOHASSLE);
        break;
    case 26:
        if (ch == vict && on)
        {
            ch->sendText("Better not -- could be a long winter!\r\n");
            return (0);
        }
        vict->player_flags.toggle(PLR_FROZEN);
        break;
    case 27:
    case 28:
        vict->modPractices(RANGE(0, 10000));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set PS for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set PS for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 29:
    case 30:
    case 31:
        if (!strcasecmp(val_arg, "off"))
        {
            GET_COND(vict, (mode - 29)) = -1; /* warning: magic number here */
            ch->send_to("%s's %s now off.\r\n", GET_NAME(vict), set_fields[mode].cmd);
        }
        else if (is_number(val_arg))
        {
            value = atoi(val_arg);
            RANGE(0, 24);
            GET_COND(vict, (mode - 29)) = value; /* and here too */
            ch->send_to("%s's %s set to %" I64T ".\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
        }
        else
        {
            ch->sendText("Must be 'off' or a value from 0 to 24.\r\n");
            return (0);
        }
        break;
    case 32:
        vict->player_flags.toggle(PLR_KILLER);
        break;
    case 33:
        vict->player_flags.toggle(PLR_THIEF);
        break;
    case 34:
        if (!IS_NPC(vict) && value > 100)
        {
            ch->sendText("You can't do that.\r\n");
            return (0);
        }
        value = MAX(0, value);
        vict->setBaseStat<int>("level", value);
        break;
    case 35:
        if ((rnum = real_room(value)) == NOWHERE)
        {
            ch->sendText("No room exists with that number.\r\n");
            return (0);
        }
        vict->leaveLocation();
        vict->moveToLocation(rnum);
        break;
    case 36:
        vict->pref_flags.toggle(PRF_ROOMFLAGS);
        break;
    case 38:
        vict->player_flags.toggle(PLR_DELETED);
        break;
    case 39:
    {
        auto check = [&](Sensei id)
        { return sensei::isPlayable(id) && sensei::isValidSenseiForRace(id, vict->race); };
        auto chosen_sensei = sensei::findSensei(val_arg, check);
        if (!chosen_sensei)
        {
            ch->sendText("That is not a sensei. Or, it is invalid for the target's race.\r\n");
            return (0);
        }
        vict->sensei = chosen_sensei.value();
    }
    break;
    case 40:
        vict->player_flags.toggle(PLR_NOWIZLIST);
        break;
    case 41:
        vict->pref_flags.toggle(PRF_QUEST);
        break;
    case 42:
        if (!strcasecmp(val_arg, "off"))
        {
            vict->player_flags.set(PLR_LOADROOM, false);
            vict->setBaseStat("load_room", NOWHERE);
        }
        else if (is_number(val_arg))
        {
            rvnum = atoi(val_arg);
            if (real_room(rvnum) != NOWHERE)
            {
                vict->player_flags.set(PLR_LOADROOM, true);
                vict->setBaseStat("load_room", rvnum);

                ch->send_to("%s will enter at room #%d.\r\n", GET_NAME(vict), GET_LOADROOM(vict));
            }
            else
            {
                ch->sendText("That room does not exist!\r\n");
                return (0);
            }
        }
        else
        {
            ch->sendText("Must be 'off' or a room's virtual number.\r\n");
            return (0);
        }
        break;
    case 43:
        vict->pref_flags.toggle(PRF_COLOR);
        break;
    case 44:
        if (GET_IDNUM(ch) == 0 || IS_NPC(vict))
            return (0);
        GET_IDNUM(vict) = value;
        break;
    case 45:
        if (GET_IDNUM(ch) > 1)
        {
            ch->sendText("Please don't use this command, yet.\r\n");
            return (0);
        }
        if (GET_ADMLEVEL(ch) < 10)
        {
            ch->sendText("NO.\r\n");
            return (0);
        }
        break;
    case 46:
        vict->player_flags.toggle(PLR_NODELETE);
        break;
    case 47:
        if ((i = search_block(val_arg, genders, false)) < 0)
        {
            ch->sendText("Must be 'male', 'female', or 'neutral'.\r\n");
            return (0);
        }
        vict->sex = static_cast<Sex>(i);
        break;
    case 48: /* set age */
        if (value <= 0)
        { /* Arbitrary limits. */
            ch->sendText("Ages must be positive.\r\n");
            return (0);
        }
        /*
         * NOTE: May not display the exact age specified due to the integer
         * division used elsewhere in the code.  Seems to only happen for
         * some values below the starting age (17) anyway. -gg 5/27/98
         */
        vict->setAge(value);
        break;

    case 49: /* Blame/Thank Rick Glover. :) */
        vict->setBaseStat("height", value);
        affect_total(vict);
        break;

    case 50:
        vict->setBaseStat("weight", value);
        affect_total(vict);
        break;

    case 51:
        if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions"))
            vict->setBaseStat<int>("olc_zone", AEDIT_PERMISSION);
        else if (is_abbrev(val_arg, "hedit"))
            vict->setBaseStat<int>("olc_zone", HEDIT_PERMISSION);
        else if (is_abbrev(val_arg, "off"))
            vict->setBaseStat<int>("olc_zone", NOWHERE);
        else if (!is_number(val_arg))
        {
            ch->sendText("Value must be either 'socials', 'actions', 'hedit', 'off' or a zone number.\r\n");
            return (0);
        }
        else
            vict->setBaseStat<int>("olc_zone", atoi(val_arg));
        break;

    case 52:
    {
        std::function<bool(Race)> check;

        if (IS_NPC(vict))
        {
            check = [ch](Race id)
            { return race::getValidSexes(id).contains(GET_SEX(ch)); };
        }
        else
        {
            check = [ch](Race id)
            { return race::getValidSexes(id).contains(GET_SEX(ch)) && race::isPlayable(id); };
        }
        auto choices = getEnumMap<Race>(check);
        auto res = partialMatch(val_arg, choices, false);
        if(!res) {
            ch->sendText(res.err);
            return 0;
        }
        vict->race = res.value()->second;
        racial_body_parts(vict);
    }
    break;

    case 53:
        break;

    case 54:
        break;

    case 55:
        vict->setBaseStat("law_chaos", RANGE(-1000, 1000));
        affect_total(vict);
        break;

    case 56:
        affect_total(vict);
        break;

    case 57:
        affect_total(vict);
        break;

    case 58:
        if (GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch) && vict != ch)
        {
            ch->sendText("Permission denied.\r\n");
            return (0);
        }
        if (value < ADMLVL_NONE || value > GET_ADMLEVEL(ch))
        {
            ch->sendText("You can't set it to that.\r\n");
            return (0);
        }
        if (GET_ADMLEVEL(vict) == value)
            return (1);
        admin_set(vict, value);
        break;

    case 64:
        vict->setBaseStat("health", value);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set basepl for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set basepl for %s.", GET_NAME(ch), GET_NAME(vict));
        break;

    case 65:
        vict->setBaseStat("ki", value);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set baseki for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set baseki for %s.", GET_NAME(ch), GET_NAME(vict));
        break;

    case 66:
        vict->setBaseStat("stamina", value);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set basest for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set basest for %s.", GET_NAME(ch), GET_NAME(vict));
        break;

    case 67:
        if (!world.contains(value))
        {
            ch->sendText("There is no such room.\r\n");
            return (0);
        }
        vict->setBaseStat("death_room", value);
        break;

    case 68:
        vict->setBaseStat<int>("absorbs", RANGE(0, 3));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set absorbs for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set absorbs for %s.", GET_NAME(ch), GET_NAME(vict));
        break;

    case 69:
        vict->modBaseStat<int>("upgrade_points", RANGE(1, 1000));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set upgrade points for %s.",
               GET_NAME(ch), GET_NAME(vict));
        log_imm_action("SET: %s has set upgrade points for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 71:
        ch->sendText("Use the reward command.\r\n");
        break;
    case 72:
        vict->setBaseStat<int>("boosts", RANGE(-1000, 1000));
        break;
    case 74:
        vict->setBaseStat<int>("death_count", RANGE(-1000, 1000));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set death count for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set death count for %s.", GET_NAME(ch), GET_NAME(vict));
        break;
    case 75:
        ch->sendText("No.");
        break;
    case 76:
        if (vict->desc)
        {
            star_phase(vict, RANGE(0, 2));
        }
        else
        {
            ch->sendText("They aren't even in the game!\r\n");
        }
        break;
    case 78:
        vict->setBaseStat<int>("skill_slots", RANGE(1, 1000));
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set skill slots for %s.", GET_NAME(ch),
               GET_NAME(vict));
        log_imm_action("SET: %s has set skill slots for %s.", GET_NAME(ch), GET_NAME(vict));
        break;

    case 79:
        GET_FEATURE(vict) = nullptr;
        break;

    case 80:
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set transformation class for %s.",
               GET_NAME(ch), GET_NAME(vict));
        log_imm_action("SET: %s has set transformation class for %s.", GET_NAME(ch), GET_NAME(vict));
        break;

    case 81:
        ch->sendText("Done.\r\n");
        break;

    case 82:
        ch->sendText("Can't set that!\r\n");
        break;

    default:
        ch->sendText("Can't set that!\r\n");
        return (0);
    }

    return (1);
}

ACMD(do_set)
{
    Character *vict = nullptr, *cbuf = nullptr;
    char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    int mode, len, player_i = 0, retval;
    char is_file = 0, is_player = 0;

    half_chop(argument, name, buf);

    if (!strcasecmp(name, "player"))
    {
        is_player = 1;
        half_chop(buf, name, buf);
    }
    else if (!strcasecmp(name, "mob"))
        half_chop(buf, name, buf);

    half_chop(buf, field, buf);

    if (!*name || !*field)
    {
        ch->sendText("Usage: set <victim> <field> <value>\r\n");
        return;
    }

    /* find the target */
    if (is_player)
    {
        vict = findPlayer(name);
        if (!vict)
        {
            ch->sendText("There is no such player.\r\n");
            return;
        }
    }
    else
    { /* is_mob */
        if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_WORLD)))
        {
            ch->sendText("There is no such creature.\r\n");
            return;
        }
    }

    /* find the command in the list */
    len = strlen(field);
    for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
        if (!strcmp(field, set_fields[mode].cmd))
            break;
    if (*(set_fields[mode].cmd) == '\n')
        for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
            if (!strncmp(field, set_fields[mode].cmd, len))
                break;

    /* perform the set */
    retval = perform_set(ch, vict, mode, buf);
}

ACMD(do_saveall)
{
    if (GET_ADMLEVEL(ch) < ADMLVL_BUILDER)
        ch->sendText("You are not holy enough to use this privelege.\r\n");
    else
    {
        save_all();
        ch->sendText("World and house files saved.\r\n");
    }
}

#define PLIST_FORMAT \
    "players [minlev[-maxlev]] [-n name] [-d days] [-h hours] [-m]"

ACMD(do_plist)
{
}

ACMD(do_peace)
{
    ch->location.sendText("Everything is quite peaceful now.\r\n");
    auto people = ch->location.getPeople();
    for (auto vict : filter_raw(people))
    {
        if (GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch))
            continue;
        stop_fighting(vict);
        vict->setBaseStat("combo", POS_SITTING);
    }
    stop_fighting(ch);
    ch->setBaseStat("combo", POS_STANDING);
}

ACMD(do_wizupdate)
{
    ch->sendText("Wizlists updated.\r\n");
}

ACMD(do_raise)
{
    Character *vict = nullptr;
    char name[MAX_INPUT_LENGTH];

    one_argument(argument, name);

    if (GET_ADMLEVEL(ch) < ADMLVL_BUILDER && !IS_NPC(ch))
    {
        return;
    }

    if (!(vict = get_player_vis(ch, name, nullptr, FIND_CHAR_WORLD)))
    {
        ch->sendText("There is no such player.\r\n");
        return;
    }

    if (IS_NPC(vict))
    {
        ch->sendText("Sorry, only players get spirits.\r\n");
        return;
    }

    if (!AFF_FLAGGED(vict, AFF_SPIRIT))
    {
        ch->sendText("But they aren't even dead!\r\n");
        return;
    }

    if (GET_ADMLEVEL(ch) <= 0)
    {
        vict->resurrect(Basic);
    }
    else
    {
        vict->resurrect(Costless);
    }

    ch->send_to("@wYou return %s from the @Bspirit@w world, to the world of the living!@n\r\n", GET_NAME(vict));
    vict->send_to("@wYour @Bspirit@w has been returned to the world of the living by %s!@n\r\n", GET_NAME(ch));

    send_to_imm("Log: %s has raised %s from the dead.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("RAISE: %s has raised %s from the dead.", GET_NAME(ch), GET_NAME(vict));
}

ACMD(do_chown)
{
    Character *victim;
    Object *obj;
    char buf2[80];
    char buf3[80];
    int i, k = 0;

    two_arguments(argument, buf2, buf3);

    if (!*buf2)
        ch->sendText("Syntax: chown <object> <character>.\r\n");
    else if (!(victim = get_char_vis(ch, buf3, nullptr, FIND_CHAR_WORLD)))
        ch->sendText("No one by that name here.\r\n");
    else if (victim == ch)
        ch->sendText("Are you sure you're feeling ok?\r\n");
    else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
        ch->sendText("That's really not such a good idea.\r\n");
    else if (!*buf3)
        ch->sendText("Syntax: chown <object> <character>.\r\n");
    else
    {
        for (i = 0; i < NUM_WEARS; i++)
        {
            if (GET_EQ(victim, i) && CAN_SEE_OBJ(ch, GET_EQ(victim, i)) &&
                isname(buf2, GET_EQ(victim, i)->getName()))
            {
                auto un = unequip_char(victim, i);
                victim->addToInventory(un);
                k = 1;
            }
        }

        if (!(obj = get_obj_in_list_vis(victim, buf2, nullptr, victim->getInventory())))
        {
            if (!k && !(obj = get_obj_in_list_vis(victim, buf2, nullptr, victim->getInventory())))
            {
                ch->send_to("%s does not appear to have the %s.\r\n", GET_NAME(victim), buf2);
                return;
            }
        }

        act("@n$n makes a magical gesture and $p@n flies from $N to $m.", false, ch, obj, victim, TO_NOTVICT);
        act("@n$n makes a magical gesture and $p@n flies away from you to $m.", false, ch, obj, victim, TO_VICT);
        act("@nYou make a magical gesture and $p@n flies away from $N to you.", false, ch, obj, victim, TO_CHAR);

        obj->clearLocation();
        ch->addToInventory(obj);
    }
}

ACMD(do_zpurge)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    auto zone = !*arg ? ch->location.getZone()->number : atoi(arg);

    if (!zone_table.count(zone) || !can_edit_zone(ch, zone))
    {
        ch->send_to("You cannot purge that zone. Try %d.\r\n", GET_OLC_ZONE(ch));
        return;
    }

    auto &z = zone_table.at(zone);
    auto zr = z.rooms.snapshot_weak();

    for (auto r : filter_raw(zr))
    {
        auto people = r->getPeople();
        for (auto mob : filter_raw(people))
        {
            if (!IS_NPC(mob))
                continue;
            extract_char(mob);
        }
        auto con = r->getObjects();
        for (auto obj : filter_raw(con))
        {
            extract_obj(obj);
        }
    }

    ch->send_to("All mobiles and objects in zone %d purged.\r\n", zone);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s has purged zone %d.", GET_NAME(ch), zone);
}

/******************************************************************************/
/*                         Zone Checker Code below                            */
/******************************************************************************/

/*mob limits*/
#define MAX_DAMAGE_MOD_ALLOWED MAX(GET_LEVEL(mob) / 5, 2)
#define MAX_GOLD_ALLOWED GET_LEVEL(mob) * 3000
#define MAX_EXP_ALLOWED GET_LEVEL(mob) * GET_LEVEL(mob) * 120
constexpr int MAX_LEVEL_ALLOWED = 100;
#define GET_OBJ_AVG_DAM(obj) (((GET_OBJ_VAL(obj, VAL_WEAPON_DAMSIZE) + 1) / 2.0) * GET_OBJ_VAL(obj, VAL_WEAPON_DAMDICE))
/* arbitrary limit for per round dam */
constexpr int MAX_MOB_DAM_ALLOWED = 500;

#define ZCMD2 zone_table.at(zone).cmd[cmd_no] /*from DB.C*/

/*item limits*/
constexpr int MAX_DAM_ALLOWED = 50; /* for weapons  - avg. dam*/
constexpr int MAX_AFFECTS_ALLOWED = 3;

/* Armor class limits*/
#define TOTAL_WEAR_CHECKS (NUM_ITEM_WEARS - 2) /*minus Wield and Take*/
static const struct zcheck_armor
{
    bitvector_t bitvector; /* from Structs.h                       */
    int ac_allowed;        /* Max. AC allowed for this body part  */
    char *message;         /* phrase for error message            */
} zarmor[] = {
    {ITEM_WEAR_FINGER, 10, "Ring"},
    {ITEM_WEAR_NECK, 10, "Necklace"},
    {ITEM_WEAR_BODY, 10, "Body armor"},
    {ITEM_WEAR_HEAD, 10, "Head gear"},
    {ITEM_WEAR_LEGS, 10, "Legwear"},
    {ITEM_WEAR_FEET, 10, "Footwear"},
    {ITEM_WEAR_HANDS, 10, "Glove"},
    {ITEM_WEAR_ARMS, 10, "Armwear"},
    {ITEM_WEAR_SHIELD, 10, "Shield"},
    {ITEM_WEAR_ABOUT, 10, "Cloak"},
    {ITEM_WEAR_WAIST, 10, "Belt"},
    {ITEM_WEAR_WRIST, 10, "Wristwear"},
    {ITEM_WEAR_HOLD, 10, "Held item"},
    {ITEM_WEAR_PACK, 10, "Backpack item"},
    {ITEM_WEAR_EAR, 10, "Earring item"},
    {ITEM_WEAR_SH, 10, "Shoulder item"},
    {ITEM_WEAR_EYE, 10, "Eye item"}};

/*These are strictly boolean*/
constexpr int CAN_WEAR_WEAPONS = 0;  /* toggle - can weapon also be armor? */
constexpr int MAX_APPLIES_LIMIT = 1; /* toggle - is there a limit at all?  */
constexpr int CHECK_ITEM_RENT = 0;   /* do we check for rent cost == 0 ?   */
constexpr int CHECK_ITEM_COST = 0;   /* do we check for item cost == 0 ?   */

/* These are ABS() values. */
constexpr int MAX_APPLY_ACCURCY_MOD_TOTAL = 5;
constexpr int MAX_APPLY_DAMAGE_MOD_TOTAL = 5;

/*room limits*/
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
static const int offlimit_zones[] = {0, 12, 13, 14, -1}; /*what zones can no room connect to (virtual num) */
constexpr int MIN_ROOM_DESC_LENGTH = 80;                 /* at least one line - set to 0 to not care. */
constexpr int MAX_COLOUMN_WIDTH = 80;                    /* at most 80 chars per line */

ACMD(do_zcheck)
{
    zone_rnum zrnum;
    room_vnum exroom = 0;
    int ac = 0;
    int affs = 0, tohit, todam, value;
    int i = 0, j = 0, k = 0, l = 0, m = 0, found = 0; /* found is used as a 'send now' flag*/
    char buf[MAX_STRING_LENGTH];
    float avg_dam;
    size_t len = 0;
    struct extra_descr_data *ext, *ext2;
    one_argument(argument, buf);

    if (buf == nullptr || !*buf || !strcmp(buf, "."))
        zrnum = ch->location.getZone()->number;
    else
        zrnum = real_zone(atoi(buf));

    auto &z = zone_table.at(zrnum);

    if (zrnum == NOWHERE)
    {
        ch->sendText("Check what zone ?\r\n");
        return;
    }
    else
        ch->send_to("Checking zone %d!\r\n", z.number);

    /*
    ch->sendText("Checking Mobs for limits...\r\n");
    for (auto &[vn, mo] : mob_proto)
    {
        if (real_zone_by_thing(vn) == zrnum)
        {
            auto mob = &mo;
            if (!strcmp(mob->name, "mob unfinished") && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Alias hasn't been set.\r\n");

            if (!strcmp(mob->short_description, "the unfinished mob") && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Short description hasn't been set.\r\n");

            if (!strncmp(mob->room_description, "An unfinished mob stands here.", 30) && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Long description hasn't been set.\r\n");

            if (mob->look_description && *mob->look_description)
            {
                if (!strncmp(mob->look_description, "It looks unfinished.", 20) && (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "- Description hasn't been set.\r\n");
                else if (strncmp(mob->look_description, "   ", 3) && (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "- Description hasn't been formatted. (/fi)\r\n");
            }

            if (GET_LEVEL(mob) > MAX_LEVEL_ALLOWED && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Is level %d (limit: 1-%d)\r\n",
                                GET_LEVEL(mob), MAX_LEVEL_ALLOWED);

            if (GET_DAMAGE_MOD(mob) > MAX_DAMAGE_MOD_ALLOWED && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Damage mod of %d is too high (limit: %ld)\r\n",
                                GET_DAMAGE_MOD(mob), MAX_DAMAGE_MOD_ALLOWED);

            avg_dam = (((mob->mob_specials.damsizedice / 2.0) * mob->mob_specials.damnodice) + GET_DAMAGE_MOD(mob));
            if (avg_dam > MAX_MOB_DAM_ALLOWED && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- average damage of %4.1f is too high (limit: %d)\r\n",
                                avg_dam, MAX_MOB_DAM_ALLOWED);

            if (mob->mob_specials.damsizedice == 1 &&
                mob->mob_specials.damnodice == 1 &&
                GET_LEVEL(mob) == 0 &&
                (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Needs to be fixed - %sAutogenerate!%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));

            if (MOB_FLAGGED(mob, MOB_AGGRESSIVE) &&
                MOB_FLAGGED(mob, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL) && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Both aggresive and agressive to align.\r\n");

            if ((GET_GOLD(mob) > MAX_GOLD_ALLOWED) && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Set to %lu Gold (limit : %d).\r\n",
                                GET_GOLD(mob),
                                MAX_GOLD_ALLOWED);

            if (GET_EXP(mob) > MAX_EXP_ALLOWED && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Has %" I64T " experience (limit: %d)\r\n",
                                GET_EXP(mob), MAX_EXP_ALLOWED);
            if (AFF_FLAGGED(mob, AFF_GROUP | AFF_CHARM | AFF_POISON) && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- Has illegal affection bits set (%s %s %s)\r\n",
                                AFF_FLAGGED(mob, AFF_GROUP) ? "GROUP" : "",
                                AFF_FLAGGED(mob, AFF_CHARM) ? "CHARM" : "",
                                AFF_FLAGGED(mob, AFF_POISON) ? "POISON" : "");

            if (MOB_FLAGGED(mob, MOB_SPEC) && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- SPEC flag needs to be removed.\r\n");

            if (found)
            {
                ch->send_to("%s[%5d]%s %-30s: %s\r\n%s", CCCYN(ch, C_NRM), vn, CCYEL(ch, C_NRM), mob->short_description, CCNRM(ch, C_NRM), buf);
            }
            strcpy(buf, "");
            found = 0;
            len = 0;
        }
    }*/

    /************** Check objects *****************/
    /*
    ch->sendText("\r\nChecking Objects for limits...\r\n");
    for (auto &[vn, ob] : obj_proto)
    {
        if (real_zone_by_thing(vn) == zrnum)
        {
            auto obj = &ob;
            switch (GET_OBJ_TYPE(obj))
            {
            case ITEM_WEAPON:
                if (GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE) >= NUM_ATTACK_TYPES && (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "- has out of range attack type %ld.\r\n",
                                    GET_OBJ_VAL(obj, VAL_WEAPON_DAMTYPE));

                if (GET_OBJ_AVG_DAM(obj) > MAX_DAM_ALLOWED && (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "- Damroll is %2.1f (limit %d)\r\n",
                                    GET_OBJ_AVG_DAM(obj), MAX_DAM_ALLOWED);
                break;
            case ITEM_ARMOR:
                ac = GET_OBJ_VAL(obj, VAL_ARMOR_APPLYAC);
                for (j = 0; j < TOTAL_WEAR_CHECKS; j++)
                {
                    if (CAN_WEAR(obj, zarmor[j].bitvector) && (ac > zarmor[j].ac_allowed) && (found = 1))
                        len += snprintf(buf + len, sizeof(buf) - len,
                                        "- Has AC %d (%s limit is %d)\r\n",
                                        ac, zarmor[j].message, zarmor[j].ac_allowed);
                }
                break;

            }

            if (!CAN_WEAR(obj, ITEM_WEAR_TAKE))
            {
                if ((GET_OBJ_COST(obj) || (GET_OBJ_WEIGHT(obj) && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) ||
                     GET_OBJ_RENT(obj)) &&
                    (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "- is NO_TAKE, but has cost (%d) weight (%.2f) or rent (%d) set.\r\n",
                                    GET_OBJ_COST(obj), GET_OBJ_WEIGHT(obj), GET_OBJ_RENT(obj));
            }
            else
            {
                if (GET_OBJ_COST(obj) == 0 && (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "- has 0 cost (min. 1).\r\n");

                if (GET_OBJ_WEIGHT(obj) == 0 && (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "- has 0 weight (min. 1).\r\n");

                if (GET_OBJ_WEIGHT(obj) > MAX_OBJ_WEIGHT && (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "  Weight is too high: %.2f (limit  %ld).\r\n",
                                    GET_OBJ_WEIGHT(obj), MAX_OBJ_WEIGHT);

                if (GET_OBJ_COST(obj) > MAX_OBJ_COST && (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "- has %d cost (max %d).\r\n",
                                    GET_OBJ_COST(obj), MAX_OBJ_COST);
            }

            if (GET_OBJ_LEVEL(obj) > ADMLVL_IMMORT - 1 && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- has min level set to %d (max %d).\r\n",
                                GET_OBJ_LEVEL(obj), ADMLVL_IMMORT - 1);

            if (obj->look_description && *obj->look_description &&
                GET_OBJ_TYPE(obj) != ITEM_STAFF &&
                GET_OBJ_TYPE(obj) != ITEM_WAND &&
                GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
                GET_OBJ_TYPE(obj) != ITEM_NOTE && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- has action_description set, but is inappropriate type.\r\n");

            for (affs = 0, j = 0; j < MAX_OBJ_AFFECT; j++)
                if (obj->affected[j].modifier)
                    affs++;

            if (affs > MAX_AFFECTS_ALLOWED && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- has %d affects (limit %d).\r\n",
                                affs, MAX_AFFECTS_ALLOWED);

            for (todam = 0, tohit = 0, j = 0; j < MAX_OBJ_AFFECT; j++)
            {
                if (obj->affected[j].location == APPLY_COMBAT_BASE && obj->affected[j].specific | static_cast<int>(ComStat::damage))
                    todam += obj->affected[j].modifier;
            }
            if (abs(todam) > MAX_APPLY_DAMAGE_MOD_TOTAL && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- total damage mod %d out of range (limit +/-%d.\r\n",
                                todam, MAX_APPLY_DAMAGE_MOD_TOTAL);
            if (abs(tohit) > MAX_APPLY_ACCURCY_MOD_TOTAL && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- total accurcy mod %d out of range (limit +/-%d).\r\n",
                                tohit, MAX_APPLY_ACCURCY_MOD_TOTAL);

            for (ext2 = nullptr, ext = obj->ex_description; ext; ext = ext->next)
                if (strncmp(ext->description, "   ", 3))
                    ext2 = ext;

            if (ext2 && (found = 1))
                len += snprintf(buf + len, sizeof(buf) - len,
                                "- has unformatted extra description\r\n");

            if (found)
            {
                ch->send_to("[%5d] %-30s: \r\n%s", vn, obj->short_description, buf);
            }
            strcpy(buf, "");
            len = 0;
            found = 0;
        }
    }*/

    /************** Check rooms *****************/
    ch->sendText("\r\nChecking Rooms for limits...\r\n");

    auto zr = z.rooms.snapshot_weak();
    for (auto r : filter_raw(zr))
    {
        for (auto &[d, e] : r->getDirections())
        {
            /*check for exit, but ignore off limits if you're in an offlimit zone*/
            auto j = static_cast<int>(d);
            auto ez = e.getZone();
            if (ez == r->getZone())
                continue;

            for (k = 0; offlimit_zones[k] != -1; k++)
            {
                if (ez->number == real_zone(offlimit_zones[k]) && (found = 1))
                    len += snprintf(buf + len, sizeof(buf) - len,
                                    "- Exit %s cannot connect to %d (zone off limits).\r\n",
                                    dirs[j], e.getVnum());
            } /* for (k.. */
        } /* cycle directions */

        if (ROOM_FLAGGED(i, ROOM_ATRIUM | ROOM_HOUSE | ROOM_OLC))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Has illegal affection bits set (%s %s %s)\r\n",
                            ROOM_FLAGGED(i, ROOM_ATRIUM) ? "ATRIUM" : "",
                            ROOM_FLAGGED(i, ROOM_HOUSE) ? "HOUSE" : "",
                            ROOM_FLAGGED(i, ROOM_OLC) ? "OLC" : "");

        if ((MIN_ROOM_DESC_LENGTH) && strlen(r->getLookDescription()) < MIN_ROOM_DESC_LENGTH && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Room description is too short. (%4.4" SZT " of min. %d characters).\r\n",
                            strlen(r->getLookDescription()), MIN_ROOM_DESC_LENGTH);

        if (strncmp(r->getLookDescription(), "   ", 3) && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Room description not formatted with indent (/fi in the editor).\r\n");

        /* strcspan = size of text in first arg before any character in second arg */
        if ((strcspn(r->getLookDescription(), "\r\n") > MAX_COLOUMN_WIDTH) && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Room description not wrapped at %d chars (/fi in the editor).\r\n",
                            MAX_COLOUMN_WIDTH);

        if (found)
        {
            ch->send_to("[%5d] %-30s: \r\n%s", i, r->getName() ? r->getName() : "An unnamed room", buf);
            strcpy(buf, "");
            len = 0;
            found = 0;
        }
    } /*checking rooms*/

    auto zro = z.rooms.snapshot_weak();
    for (auto i : filter_raw(zro))
    {
        m++;
        if (i->exits.empty())
            l++;
    }
    if (l * 3 > m)
        ch->sendText("More than 1/3 of the rooms are not linked.\r\n");
}

/**********************************************************************************/

static void mob_checkload(Character *ch, mob_vnum mvnum)
{
    int count = 0;

    auto find = mob_proto.find(mvnum);
    if (find == mob_proto.end())
    {
        ch->sendText("That mob does not exist.\r\n");
        return;
    }

    ch->send_to("Checking load info for the mob [%d] %s...\r\n", mvnum, find->second.short_description);

    for (auto &[zvn, z] : zone_table)
    {
        auto zro = z.rooms.snapshot_weak();
        for(auto r : filter_raw(zro)) {
            for (auto c : r->resetCommands)
            {
                if (c.type != ResetCommandType::MOB)
                    continue;
                if (c.target != mvnum)
                    continue;

                /* read a mobile */
                else
                {
                    ch->send_to("  [%5d] %s (%d MaxW, %d MaxL)\r\n", r->getVnum(), r->getName(), c.max, c.max_location);
                }
                count += 1;
            }
        }

    }
    if (count > 0)
        ch->send_to("@D[@nTotal counted: %s.@D]@n\r\n", add_commas(count).c_str());
}

static void obj_checkload(Character *ch, obj_vnum ovnum)
{
    int count = 0;

    mob_vnum lastmob_v = NOTHING;
    auto lastmob = mob_proto.end();

    obj_vnum lastobj_v = NOTHING;
    auto lastobj = obj_proto.end();

    room_vnum lastroom_v = NOWHERE;
    Room *lastroom = nullptr;

    auto obj = obj_proto.find(ovnum);
    if (obj == obj_proto.end())
    {
        ch->sendText("That object does not exist.\r\n");
        return;
    }

    ch->send_to("Checking load info for the obj [%d] %s...\r\n", ovnum, obj->second.short_description);

    for (auto &[vn, r] : world)
    {
        lastroom = r.get();
        lastroom_v = r->getVnum();
        for (auto &c : r->resetCommands)
        {
            
            switch (c.type)
            {
            case ResetCommandType::MOB:
                lastmob_v = c.target;
                lastmob = mob_proto.find(lastmob_v);
                break;
            case ResetCommandType::OBJ: /* read an object */
                lastobj_v = c.target;
                lastobj = obj_proto.find(lastobj_v);
                if (c.target == ovnum)
                {
                    ch->send_to("  [%5d] %s (%d MaxR, %d MaxW)\r\n", lastroom->getVnum(), lastroom->getName(), c.max_location, c.max);
                    count += 1;
                }
                break;
            case ResetCommandType::PUT: /* object to object */
                if (c.target == ovnum)
                {
                    if (lastobj != obj_proto.end())
                    {
                        ch->send_to("  [%5d] %s (Put in another object [%d Max])\r\n", lastroom->getVnum(), lastroom->getName(), c.max);
                    }
                    else if (lastobj == obj_proto.end())
                    {
                        ch->send_to("  [%5d] %s (object does not exist)\r\n", c.target, "ERROR");
                    }
                    count += 1;
                }
                break;
            case ResetCommandType::GIVE: /* obj_to_char */
                if (c.target == ovnum)
                {
                    if (lastmob != mob_proto.end())
                    {
                        ch->send_to("  [%5d] %s (Given to %s [%d][%d Max])\r\n", lastroom_v, lastroom->getName(), lastmob->second.short_description, lastmob->first, c.max);
                    }
                    else if (lastmob == mob_proto.end())
                    {
                        ch->send_to("  [%5d] %s (mob does not exist)\r\n", c.target, "ERROR");
                    }
                    count += 1;
                }
                break;
            case ResetCommandType::EQUIP: /* object to equipment list */
                if (c.target == ovnum)
                {
                    if (lastmob != mob_proto.end())
                    {
                        ch->send_to("  [%5d] %s (Equipped to %s [%d] at %s [%d Max])\r\n", lastroom_v, lastroom->getName(), lastmob->second.short_description, lastmob->first, equipment_types[c.ex], c.max);
                    }
                    else
                    {
                        ch->send_to("  [%5d] %s (Equipped to ??? [%d] at %s [%d Max])\r\n", lastroom_v, lastroom->getName(), c.target, equipment_types[c.ex], c.max);
                    }

                    count += 1;
                }
                break;
            case ResetCommandType::REMOVE: /* rem obj from room */
                
                if (c.target == ovnum)
                {
                    if (lastroom)
                    {
                        ch->send_to("  [%5d] %s (Removed from room)\r\n", lastroom_v, lastroom->getName());
                    }
                    count += 1;
                }
                break;
            } /* switch */
        } /*for cmd_no......*/
    } /*for zone...*/

    if (count > 0)
        ch->send_to("@D[@nTotal counted: %s.@D]@n\r\n", add_commas(count).c_str());
}

static void trg_checkload(Character *ch, trig_vnum tvnum)
{
    int found = 0;
    room_vnum lastroom_v = NOWHERE;
    mob_rnum lastmob_v = NOTHING;
    obj_rnum lastobj_v = NOTHING;

    auto lastmob = mob_proto.end();
    auto lastobj = obj_proto.end();
    Room *lastroom = nullptr;

    struct trig_proto_list *tpl = nullptr;

    auto trg = trig_index.find(tvnum);
    if (trg == trig_index.end())
    {
        ch->sendText("That trigger does not exist.\r\n");
        return;
    }

    ch->send_to("Checking load info for the %s trigger [%d] '%s':\r\n", trg->second.attach_type == MOB_TRIGGER ? "mobile" : (trg->second.attach_type == OBJ_TRIGGER ? "object" : "room"), tvnum, trg->second.name);

    for (auto &[zvn, r] : world)
    {
        lastroom_v = r->getVnum();
        for (auto &c : r->resetCommands)
        {
            switch (c.type)
            {
            case ResetCommandType::MOB:
                lastmob_v = c.target;
                break;
            case ResetCommandType::OBJ: /* read an object */
                lastobj_v = c.target;
                break;
            case ResetCommandType::PUT: /* object to object */
                lastobj_v = c.target;
                break;
            case ResetCommandType::GET: /* obj_to_char */
                lastobj_v = c.target;
                break;
            case ResetCommandType::EQUIP: /* object to equipment list */
                lastobj_v = c.target;
                break;
            case ResetCommandType::REMOVE: /* rem obj from room */
                lastroom_v = 0;
                lastobj_v = 0;
                lastmob_v = 0;
            case ResetCommandType::TRIGGER: /* trigger to something */
                if (c.target != tvnum)
                    break;
                if (c.ex == static_cast<int>(MOB_TRIGGER))
                {
                    lastmob = mob_proto.find(lastmob_v);
                    if (lastmob != mob_proto.end())
                    {
                        ch->send_to("mob [%5d] %-60s (zedit room %5d)\r\n", lastmob->first, lastmob->second.short_description, lastroom_v);
                    }
                    else
                    {
                        ch->send_to("mob [%5d] %-60s (zedit room %5d)\r\n", lastmob_v, "ERROR NOTEXIST", lastroom_v);
                    }

                    found = 1;
                }
                else if (c.ex == static_cast<int>(OBJ_TRIGGER))
                {
                    lastobj = obj_proto.find(lastobj_v);
                    if (lastobj != obj_proto.end())
                    {
                        ch->send_to("obj [%5d] %-60s  (zedit room %d)\r\n", lastobj_v, lastobj->second.short_description, lastroom_v);
                    }
                    else
                    {
                        ch->send_to("obj [%5d] %-60s  (zedit room %d)\r\n", lastobj_v, "ERROR NOTEXIST", lastroom_v);
                    }
                    found = 1;
                }
                else if (c.ex == static_cast<int>(WLD_TRIGGER))
                {
                    ch->send_to("room [%5d] %-60s (zedit)\r\n", lastroom_v, lastroom->getName());
                    found = 1;
                }
                break;
            } /* switch */
        } /*for cmd_no......*/
    } /*for zone...*/

    for (auto &[vn, m] : mob_proto)
    {
        auto find = std::find(m.proto_script.begin(), m.proto_script.end(), tvnum);
        if (find == m.proto_script.end())
            continue;
        ch->send_to("mob [%5d] %s\r\n", vn, m.short_description);
        found = 1;
    }

    for (auto &[vn, o] : obj_proto)
    {
        auto find = std::find(o.proto_script.begin(), o.proto_script.end(), tvnum);
        if (find == o.proto_script.end())
            continue;
        ch->send_to("obj [%5d] %s\r\n", vn, o.short_description);
        found = 1;
    }

    for (auto &[vn, r] : world)
    {
        auto find = std::find(r->proto_script.begin(), r->proto_script.end(), tvnum);
        if (find == r->proto_script.end())
            continue;
        ch->send_to("room[%5d] %s\r\n", vn, r->getName());
        found = 1;
    }

    if (!found)
        ch->sendText("This trigger is not attached to anything.\r\n");
}

ACMD(do_checkloadstatus)
{
    char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

    two_arguments(argument, buf1, buf2);

    if ((!*buf1) || (!*buf2) || (!isdigit(*buf2)))
    {
        ch->sendText("Checkload <M | O | T> <vnum>\r\n");
        return;
    }

    if (LOWER(*buf1) == 'm')
    {
        mob_checkload(ch, atoi(buf2));
        return;
    }

    if (LOWER(*buf1) == 'o')
    {
        obj_checkload(ch, atoi(buf2));
        return;
    }

    if (LOWER(*buf1) == 't')
    {
        trg_checkload(ch, atoi(buf2));
        return;
    }
}
/**************************************************************************************/
/*                         Zone Checker code above                                    */
/**************************************************************************************/

/* do_findkey, finds where the key to a door loads to using do_checkloadstatus() */
ACMD(do_findkey)
{
    int dir, key;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    any_one_arg(argument, arg); /* Because "in" is a valid direction */

    if (!*arg)
    {
        ch->sendText("Format: findkey <dir>\r\n");
    }
    else if ((dir = search_block(arg, dirs, false)) >= 0 ||
             (dir = search_block(arg, abbr_dirs, false)) >= 0)
    {
        if (!EXIT(ch, dir))
        {
            ch->sendText("There's no exit in that direction!\r\n");
        }
        else if ((key = EXIT(ch, dir)->key) == NOTHING || key == 0)
        {
            ch->sendText("There's no key for that exit.\r\n");
        }
        else
        {
            sprintf(buf, "obj %d", key);
            do_checkloadstatus(ch, buf, 0, 0);
        }
    }
    else
    {
        ch->sendText("What direction is that?!?\r\n");
    }
}

ACMD(do_spells)
{
    int i, qend;

    ch->sendText("The following spells are in the game:\r\n");

    for (qend = 0, i = 0; i < MAX_SPELLS; i++)
    {
        if (spell_info[i].name == unused_spellname) /* This is valid. */
            continue;
        ch->send_to("%18s", spell_info[i].name);
        if (qend++ % 4 == 3)
            ch->sendText("\r\n");
    }
    if (qend % 4 != 0)
        ch->sendText("\r\n");
    return;
}

ACMD(do_boom)
{
    /* Only IDNUM = 1 can cause a boom! Ideally for testing something that might
     * cause a crash. Currently left over from testing changes to send_to_outdoor. */
    if (GET_IDNUM(ch) != 1)
    {
        ch->sendText("Sorry, only the Founder may use the boom command.\r\n");
        return;
    }

    send_to_outdoor("%s shakes the world with a mighty boom!\r\n", GET_NAME(ch));
}

enum class ZoneOp
{
    Create,
    Delete,
    Rename,
    Desc,
    Reset,
    Flags,
    Parent,
    List,
    Help,
    AddRooms,
    Examine
};

static const std::unordered_map<std::string, ZoneOp> kOps{
    {"create", ZoneOp::Create},
    {"delete", ZoneOp::Delete},
    {"rename", ZoneOp::Rename},
    {"reset",  ZoneOp::Reset},
    {"flags",  ZoneOp::Flags},
    {"parent", ZoneOp::Parent},
    {"list",   ZoneOp::List},
    {"help",   ZoneOp::Help},
    {"addrooms", ZoneOp::AddRooms},
    {"examine", ZoneOp::Examine}
};

static const std::string mushZoneHelp = R"(
MUSH-style Zone Editor Commands:
  Alias: .z

  .zone <id>
       Examine a zone.

  .zone/create <name>[=<parent ID>] 
       Create a new zone with optional parent.
  
  .zone/delete <id>=YES
       Delete a zone. It must be totally empty.

  .zone/rename <id>=<new>
       Rename a zone.

  .zone/reset <id>
       Reset a zone.

  .zone/flags <id>=[[+|-]flag...]
       Set zone flags. Example: .zone/flags 50=+dark -cave

  .zone/parent <id>=<new parent id> | NONE
       Set or clear zone parent.

  .zone/list
       List all zones.

  .zone/help
       Show this help.

  .zone/addrooms <id>=[<number>|<from>-<to>]...
       Add rooms to a zone. example: .zone/addrooms 50=20 21 99-120

)";

ACMD(do_mush_zone) {

    auto op = cdata.switch_type;
    if(op.empty()) op = "examine";

    auto oper = partialMatch(op, kOps);

    if(!oper) {
        ch->sendFmt(oper.err);
        return;
    }

    auto operation = oper.value()->second;
    switch(operation) {
        case ZoneOp::Create: {
            auto res = validateZoneName(cdata.lsargs);
            if(!res) {
                ch->sendText(res.err);
                return;
            }
            Zone* parent = nullptr;
            if(!cdata.rsargs.empty()) {
                auto parentRes = getZone(cdata.rsargs, ch);
                if(!parentRes) {
                    ch->sendText(parentRes.err);
                    return;
                }
                parent = parentRes.value();
            }
            auto newid = getNextID(lastZoneID, zone_table);
            auto &z = zone_table.emplace(newid, Zone{}).first->second;
            z.name = res.value();
            z.number = newid;
            if(parent) {
                z.parent = parent->number;
                parent->children.insert(newid);
                ch->sendFmt("{} created. Its parent is {}'\r\n", z, *parent);
            } else {
                ch->sendFmt("{} created. It has no parent.\r\n", z);
            }
            return;
        }
        case ZoneOp::Delete: {
            auto zRes = getZone(cdata.lsargs, ch);
            if(!zRes) {
                ch->sendText(zRes.err);
                return;
            }
            auto z = zRes.value();
            auto zCan = z->canBeDeletedBy(ch);
            if(!zCan) {
                ch->sendText(zCan.err);
                return;
            }
            // TODO: finish sanitizing zone deletion.
            //zone_table.erase(z->number);
            //ch->sendFmt("Zone {} '{}' deleted.\r\n", z->number, z->name);
            return;
        }
        case ZoneOp::Rename: {
            auto res = getZone(cdata.lsargs, ch);
            if(!res) {
                ch->sendText(res.err);
                return;
            }
            auto z = res.value();
            auto oldname = z->name;
            auto nameRes = validateZoneName(cdata.rsargs);
            if(!nameRes) {
                ch->sendText(nameRes.err);
                return;
            }
            z->name = nameRes.value();
            ch->sendFmt("Renamed, now {}. Old name was: {}\r\n", *z, oldname);
            return;
        }
        case ZoneOp::Reset: {
            auto res = getZone(cdata.lsargs, ch);
            if(!res) {
                ch->sendText(res.err);
                return;
            }
            auto z = res.value();
            z->reset();
            ch->sendFmt("{} reset.\r\n", *z);
            return;
        }
        case ZoneOp::Flags: {
            auto res = getZone(cdata.lsargs, ch);
            if(!res) {
                ch->sendText(res.err);
                return;
            }
            auto z = res.value();
            if(cdata.rsargs.empty()) {
                ch->sendFmt("Current flags: {}\r\n", z->zone_flags.getFlagNames());
                return;
            }
            auto results = z->zone_flags.applyChanges(cdata.rsargs);
            ch->sendText(results.printResults());
            return;
        }
        case ZoneOp::List:
            list_zones(ch);
            return;
        case ZoneOp::Examine: {
            auto res = getZone(cdata.lsargs, ch);
            if(!res) {
                ch->sendText(res.err);
                return;
            }
            auto z = res.value();
            print_zone(ch, z->number);
            return;
        }
        case ZoneOp::Parent: {
            auto res = getZone(cdata.lsargs, ch);
            if(!res) {
                ch->sendText(res.err);
                return;
            }
            auto z = res.value();
            if(cdata.rsargs.empty()) {
                ch->sendText("Set parent to what? Use NONE to clear.");
                return;
            }
            if(boost::iequals(cdata.rsargs, "NONE")) {
                auto par = z->getParent();
                if(!par) {
                    ch->sendText("This zone has no parent.\r\n");
                    return;
                }
                par->children.erase(z->number);
                z->parent = NOTHING;
                ch->sendFmt("{} parent cleared.", *z);
                return;
            }
            return;
        }
        case ZoneOp::AddRooms: {
            auto res = getZone(cdata.lsargs, ch);
            if(!res) {
                ch->sendText(res.err);
                return;
            }
            auto z = res.value();
            // the lsargs should be a space-delimited sequence of numbers >0...
            auto ranges = parseRanges<room_vnum>(cdata.rsargs);
            if(!ranges) {
                ch->sendText(ranges.err);
                return;
            }
            for(auto i : ranges.value()) {
                auto r = get_room(i);
                if(!r) {
                    ch->sendFmt("Room {} does not exist.\r\n", i);
                    continue;
                }
                auto sh = r->shared_from_this();
                if(auto already_zone = sh->getZone(); already_zone) {
                    already_zone->rooms.remove(sh);
                }
                z->rooms.add(sh);
                sh->zone.reset(z);
                ch->sendFmt("Added {} to {}\r\n", *r, *z);
            }
        }
        case ZoneOp::Help: {
            ch->sendText(mushExitsHelp);
            return;
        }
        default:
            ch->sendText("Oops?!\r\n");
            break;
    }

}

static const std::string mushExitsHelp = R"(
MUSH-style Exits Editor
=============================================================================
The following exit directions are available:
- north, east, south, west, up, down, northeast, southeast, southwest, 
- northwest, inside, outside

This command always targets the exits in your current location.

A LocationID is a string representing the unique identifier for a location.
The first letter is the type (R = Room, A = Area, S = Structure)
It's followed by a : then the ID of that thing.
Non-Rooms use a coordinate system.
So, a Room might be R:50 and an Area location could be A:2:3:9:-2

It is usually more efficient to use buildwalk to create lots of connected
rooms. This command is for performing specific edits like setting keys.

It is not recommended to override auto-generated exits in grid areas that
simply connect coordinates. It could become very confusing.

Alias: .ex

.exit
    Display exits in current location.
    Automatically generated exits in grid areas will be marked out.

.exit/destination <direction>=<LocationID>
    Create/open or re-link an exit.
    In a grid area this will create an exit override that can lead anywhere, 
    but this is best used only on edges. The automapper will become very
    confused otherwise if default bounds are in play.

.exit/key <direction>=<key vnum>
   The object vnum that'll be used as a key. 
   Set to NONE or -1 to clear.

.exit/dclock <direction>=<difficulty>
    The difficulty number for picking the lock. 0 by default.

.exit/dchide <direction>=<difficulty>
    How easy it is to search for the exit if hidden. 0 By default.

.exit/flags <direction>=<flagset>...
    Choices: isdoor, closed, locked, pickproof, secret
    Flagset can look like: +isdoor +closed -secret

.exit/clear <direction>
    Delete/close/wipe an exit.
    This won't do anything in a grid area where you're in default 
    bounds without overrides.

.exit/help
    Display this help text.

)";

// TODO: Replace ExitInfo with ExitFlags.

enum class ExitOp : uint8_t
{
    List,
    Destination,
    Key,
    DCLock,
    DCHide,
    Flags,
    Clear,
    Help
};

static class std::unordered_map<std::string, ExitOp> kExitOps{
    {"list", ExitOp::List},
    {"destination", ExitOp::Destination},
    {"key", ExitOp::Key},
    {"dclock", ExitOp::DCLock},
    {"dchide", ExitOp::DCHide},
    {"flags", ExitOp::Flags},
    {"clear", ExitOp::Clear},
    {"help", ExitOp::Help}
};

ACMD(do_mush_exits) {
    auto op = cdata.switch_type;
    if(op.empty()) op = "examine";

    auto oper = partialMatch(op, kExitOps);

    if(!oper) {
        ch->sendFmt(oper.err);
        return;
    }

    auto operation = oper.value()->second;

    switch(operation) {
        case ExitOp::Help:
            ch->sendText(mushExitsHelp);
            return;
        case ExitOp::List: {
            ch->sendText("Exits:\r\n");
            for(auto &[d, e] : ch->location.getExits()) {
                ch->sendFmt("{}\r\n", e);
            }
            return;
        }
        case ExitOp::Clear: {
            auto emap = getEnumMap<Direction>();
            auto dirRes = partialMatch(cdata.lsargs, emap);
            if(!dirRes) {
                ch->sendFmt(dirRes.err);
                return;
            }
            auto dir = dirRes.value()->second;
            auto ex = ch->location.getExit(dir);
            if(!ex) {
                ch->sendFmt("There is no {} exit.\r\n", dir);
                return;
            }
            if(ex->generated) {
                ch->sendFmt("You cannot clear the generated {} exit.\r\n", dir);
                return;
            }
            auto e = ex.value();
            ch->sendFmt("Clearing exit {}:\r\n", dir);
            ch->location.deleteExit(dir);
            return;
        }
    }

}