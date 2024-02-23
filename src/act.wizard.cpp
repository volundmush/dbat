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
#include "dbat/utils.h"
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
static void print_lockout(BaseCharacter *ch);

static void execute_copyover();

static int perform_set(BaseCharacter *ch, BaseCharacter *vict, int mode, char *val_arg);

static void perform_immort_invis(BaseCharacter *ch, int level);

static void do_stat_room(BaseCharacter *ch);

static void do_stat_object(BaseCharacter *ch, Object *j);

static void do_stat_character(BaseCharacter *ch, BaseCharacter *k);

static void stop_snooping(BaseCharacter *ch);

static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall);

static void mob_checkload(BaseCharacter *ch, mob_vnum mvnum);

static void obj_checkload(BaseCharacter *ch, obj_vnum ovnum);

static void trg_checkload(BaseCharacter *ch, trig_vnum tvnum);

static void lockWrite(BaseCharacter *ch, char *name);

// definitions
ACMD(do_lag) {

    struct descriptor_data *d;
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
        ch->sendf("Syntax: lag (target) (number of seconds)\r\n");
        return;
    }

    int found = false, num = atoi(arg2);

    if (num <= 0 || num > 5) {
        ch->sendf("Keep it between 1 to 5 seconds please.\r\n");
        return;
    }

    for (d = descriptor_list; d; d = d->next) {
        if (!strcasecmp(CAP((char*)GET_NAME(d->character)), CAP(arg))) {
            if (GET_ADMLEVEL(d->character) > GET_ADMLEVEL(ch)) {
                ch->sendf("Sorry, you've been outranked.\r\n");
                return;
            }
            switch (num) {
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

    if (found == false) {
        ch->sendf("That player isn't around.\r\n");
        return;
    }

}

/*Update things that need it. Just space for the moment - Iovan */
void update_space() {
    FILE *mapfile;
    int rowcounter, colcounter;
    int vnum_read;

    basic_mud_log("Updated Space Map. ");

    //Load the map vnums from a file into an array
    mapfile = fopen("../lib/surface.map", "r");

    for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++) {
        for (colcounter = 0; colcounter <= MAP_COLS; colcounter++) {
            fscanf(mapfile, "%d", &vnum_read);
            mapnums[rowcounter][colcounter] = real_room(vnum_read);
        }
    }

    fclose(mapfile);
}

/* Read the news entry */
ACMD(do_news) {

    if (IS_NPC(ch)) {
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

    if (!*arg) {
        ch->sendf("Syntax: news (number | list)\r\n");
        return;
    }

    lookup = atoi(arg);

    if (!(fl = fopen(filename, "r"))) {
        basic_mud_log("SYSERR: opening news file for reading");
        return;
    }

    if (lookup > 0) {
        while (!feof(fl) && exit == false) {
            get_line(fl, line);
            if (*line == '#') { /* Count the entries */
                if (sscanf(line, "#%d", &nr) != 1) { /* Check the entry */
                    continue;
                } else if (nr != lookup) {
                    entries++;
                    continue;
                } else { /* One we want to read */
                    sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
                    sprintf(buf,
                            "@w--------------------------------------------------------------\n@cNum@W: @D(@G%3d@D)                @cTitle@W: @g%-50s@n\n",
                            nr, title);
                    found = true;
                    while (!feof(fl) && exit == false) {
                        get_line(fl, line);
                        if (*line != '#') { /* As long as it isn't a new entry*/
                            if (first == true) {
                                first = false;
                                sprintf(buf + strlen(buf),
                                        "%s\n@w--------------------------------------------------------------\n", line);
                                sprintf(lastline, "%s", line);
                            } else if (!strcasecmp(line, lastline)) {
                                continue;
                            } else {
                                sprintf(buf + strlen(buf), "%s\n", line);
                                sprintf(lastline, "%s", line);
                            }
                        } else {
                            exit = true;
                        }
                    } /* End write buffer with entry text */
                } /* End One We wanted to read */
            } /* End Check the entry */
        } /* End of main read while */
        fclose(fl);
    } else if (!strcasecmp(arg, "list")) {
        while (!feof(fl)) {
            get_line(fl, line);
            if (*line == '#') { /* Count the entries */
                entries++;
                if (sscanf(line, "#%d", &nr) != 1) { /* Check the entry */
                    continue;
                } else { /* One we want to read */
                    if (first == true) {
                        sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
                        sprintf(buf,
                                "@wNews Entries (Newest at the bottom, to read an entry use 'news (number)')\n@D[@cNum@W: @D(@G%3d@D) @cTitle@W: @g%-50s@D]@n\n",
                                nr, title);
                        first = false;
                    } else {
                        sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
                        sprintf(buf + strlen(buf), "@D[@cNum@W: @D(@G%3d@D) @cTitle@W: @g%-50s@D]@n\n", nr, title);
                    }
                }
            } /* End Check the entry */
        } /* End of main listwhile */
        fclose(fl);

        if (entries > 0) {
            GET_LPLAY(ch) = time(nullptr);
            WAIT_STATE(ch, PULSE_1SEC);
            write_to_output(ch->desc, buf);
        } else {
            ch->sendf("The news file is empty right now.\r\n");
        }
        *buf = '\0';
        *title = '\0';
        *lastline = '\0';
        return;
    } else {
        fclose(fl);
        ch->sendf("Syntax: news (number | list)\r\n");
        return;
    }

    if (found == true) {
        ch->sendf("%s\r\n", buf);
        GET_LPLAY(ch) = time(nullptr);
        *buf = '\0';
        *title = '\0';
        *lastline = '\0';
        WAIT_STATE(ch, PULSE_1SEC);
        return;
    } else {
        ch->sendf("That news entry does not exist.\r\n");
        return;
    }

}

/* Write the news entry */

ACMD(do_newsedit) {

    if (GET_ADMLEVEL(ch) < 1 || IS_NPC(ch)) {
        return;
    }

    FILE *fl;
    const char *filename;
    char line[256];
    int entries = 0, lookup = 0, lastentry = 0, nr;

    filename = NEWS_FILE;

    if (!*argument) {
        ch->sendf("Syntax: newsedit (title)\r\n");
        return;
    } else if (strlen(argument) > 50) {
        ch->sendf("Limit of 50 characters for title.\r\n");
        return;
    } else if (strstr(argument, "#")) {
        ch->sendf("# is a forbidden character for news entries as it is used by the file system.\r\n");
        return;
    }

    /* Check the file for the entry so we may edit it if need be*/

    if (!(fl = fopen(filename, "r"))) {
        basic_mud_log("SYSERR: Couldn't open news file for reading");
        return;
    }

    while (!feof(fl)) {
        get_line(fl, line);
        if (*line == '#') { /* Count the entries */
            entries++;
            if (sscanf(line, "#%d", &nr) != 1) {
                continue;
            } else {
                lastentry = nr;
            }
        }
    } /* End of main read while */
    fclose(fl);

    /* Time to write the entry */
    struct {
        char *cmd;
        char level;
        char **buffer;
        int size;
        char *filename;
    } fields[] = {
            /* edit the lvls to your own needs */
            {"news", ADMLVL_IMMORT, &immlist, 2000, IMMLIST_FILE},
            {"\n", 0,               nullptr,  0, nullptr}
    };

    char *tmstr;
    time_t mytime = time(nullptr);
    tmstr = (char *) asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    if (lastentry == 0) {
        if (entries == 0) {
            lookup = 1;
        } else { /* Uh oh */
            send_to_imm("ERROR: News file entries are disorganized. Report to Iovan for analysis.\r\n");
            return;
        }
    } else {
        lookup = lastentry + 1;
    }
    char *backstr = nullptr;
    act("$n begins to edit the news.", true, ch, nullptr, nullptr, TO_ROOM);
    ch->sendf("@D----------------------=[@GNews Edit@D]=----------------------@n\n");
    ch->sendf(" @RRemember that using # in newsedit is not possible. That\n");
    ch->sendf("character will be eaten because it is required for the news\n");
    ch->sendf("file as a delimiter. Also if you want to create an empty line\n");
    ch->sendf("between paragraphs you will need to enter a single space and\n");
    ch->sendf("not just push enter. Happy editing!@n\n");
    ch->sendf("@D---------------------------------------------------------@n\n");
    send_editor_help(ch->desc);
    skip_spaces(&argument);
    ch->desc->newsbuf = strdup(argument);
    TOP_OF_NEWS = lookup;
    LASTNEWS = lookup;
    string_write(ch->desc, fields[0].buffer, 2000, 0, backstr);
    STATE(ch->desc) = CON_NEWSEDIT;
}

static void print_lockout(BaseCharacter *ch) {
    if (IS_NPC(ch))
        return;

    FILE *file;
    char fname[40], filler[50], line[256], buf[MAX_STRING_LENGTH * 4];
    int count = 0, first = true;

    /* Read Introduction File */
    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
        ch->sendf("The lockout file does not exist.");
        return;
    } else if (!(file = fopen(fname, "r"))) {
        ch->sendf("The lockout file does not exist.");
        return;
    }
    sprintf(buf, "@b------------------[ @RLOCKOUT @b]------------------@n\n");
    while (!feof(file)) {
        get_line(file, line);
        sscanf(line, "%s\n", filler);
        if (first != true && strstr(buf, filler) == nullptr) {
            if (count == 0) {
                sprintf(buf + strlen(buf), "%-23s@D|@n", filler);
                count = 1;
            } else {
                sprintf(buf + strlen(buf), "%-23s\n", filler);
                count = 0;
            }
        } else if (first == true) {
            sprintf(buf + strlen(buf), "%-23s@D|@n", filler);
            first = false;
            count = 1;
        }
        *filler = '\0';
    }
    if (count == 1) {
        sprintf(buf + strlen(buf), "\n");
    }
    sprintf(buf + strlen(buf), "@b------------------[ @RLOCKOUT @b]------------------@n\n");

    write_to_output(ch->desc, buf);

    fclose(file);
}

ACMD(do_approve) {

    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *vict = nullptr;

    one_argument(argument, arg);

    if (!*arg) {
        ch->sendf("What player do you want to approve as having an acceptable bio?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD))) {
        ch->sendf("That player is not in the game.\r\n");
        return;
    }

    if (PLR_FLAGGED(vict, PLR_BIOGR)) {
        ch->sendf("They have already been approved. If this was made in error inform Iovan.\r\n");
        return;
    } else {
        vict->setFlag(FlagType::PC, PLR_BIOGR);
        ch->sendf("They have now been approved.\r\n");
        return;
    }
}

static void lockWrite(BaseCharacter *ch, char *name) {
    FILE *file;
    char fname[40], filler[50], line[256];
    char *names[500] = {""};
    FILE *fl;
    int count = 0, x = 0, found = false;

    /* Read Introduction File */
    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
        ch->sendf("The lockout file does not exist.");
        return;
    } else if (!(file = fopen(fname, "r"))) {
        ch->sendf("The lockout file does not exist.");
        return;
    }
    while (!feof(file) || count < 498) {
        get_line(file, line);
        sscanf(line, "%s\n", filler);
        names[count] = strdup(filler);
        count++;
        *filler = '\0';
    }
    fclose(file);

    /* Write Introduction File */

    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout"))
        return;

    if (!(fl = fopen(fname, "w"))) {
        basic_mud_log("ERROR: could not save Lockout File, %s.", fname);
        return;
    }

    while (x < count) {
        if (x == 0 || strcasecmp(names[x - 1], names[x])) {
            if (strcasecmp(names[x], CAP(name))) {
                fprintf(fl, "%s\n", CAP(names[x]));
            } else {
                found = true;
            }
        }
        x++;
    }
    if (found == false) {
        fprintf(fl, "%s\n", CAP(name));
        send_to_all("@rLOCKOUT@D: @WThe character, @C%s@W, was locked out of the MUD by @c%s@W.@n\r\n", CAP(name),
                    GET_NAME(ch));
        basic_mud_log("LOCKOUT: %s sentenced by %s.", CAP(name), GET_NAME(ch));
        log_imm_action("LOCKOUT: %s sentenced by %s.", CAP(name), GET_NAME(ch));
    } else {
        send_to_all("@rLOCKOUT@D: @WThe character, @C%s@W, has had lockout removed by @c%s@W.@n\r\n", CAP(name),
                    GET_NAME(ch));
        basic_mud_log("LOCKOUT: %s sentenced by %s.", CAP(name), GET_NAME(ch));
        log_imm_action("LOCKOUT: %s sentenced by %s.", CAP(name), GET_NAME(ch));
    }

    fclose(fl);
    return;
}

ACMD(do_reward) {
    int amt = 0;

    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg || !*arg2) {
        ch->sendf("Syntax: reward (target) (amount)\r\nThis is either a positive number or a negative.\r\n");
        return;
    }

    auto vict = findPlayer(arg);

    if (!vict) {
        ch->sendf("That is not a recognised player character.\r\n");
        return;
    }

    amt = atoi(arg2);

    if (amt == 0) {
        ch->sendf("That is pointless don't you think? Try an amount higher than 0.\r\n");
        return;
    }

    if (amt > 0) {
        ch->sendf("@WYou award @C%s @D(@G%d@D)@W RP points.@n\r\n", GET_NAME(vict), amt);
        vict->sendf("@D[@YROLEPLAY@D] @WYou have been awarded @D(@G%d@D)@W RP points by @C%s@W.@n\r\n", amt,
                     GET_NAME(ch));
        send_to_imm("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, GET_NAME(ch));
        log_imm_action("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, GET_NAME(ch));
        vict->modRPP(amt);
    } else {
        ch->sendf("@WYou deduct @D(@G%d@D)@W RP points from @C%s@W.@n\r\n", amt, GET_NAME(vict));
        vict->sendf("@D[@YROLEPLAY@D] @C%s@W deducts @D(@G%d@D)@W RP points from you.@n\r\n", GET_NAME(ch),
                     amt);
        send_to_imm("ROLEPLAY: %s has had %d RP points deducted by %s.", GET_NAME(vict), amt, GET_NAME(ch));
        log_imm_action("ROLEPLAY: %s has had %d RP points deducted by %s.", GET_NAME(vict), amt, GET_NAME(ch));
        vict->modRPP(amt);
    }

}

ACMD(do_permission) {
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!*arg) {
        ch->sendf("You want to @Grestrict@n or @Gunrestrict@n?\r\n");
        return;
    }

    if (!*arg2 && !strcasecmp("unrestrict", arg)) {
        ch->sendf("You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
        return;
    }
    if (!strcasecmp("unrestrict", arg)) {
        if (!strcasecmp("saiyan", arg2)) {
            ch->sendf("You have unrestricted saiyans for the very next character creation.\r\n");
            send_to_imm("PERMISSION: %s unrestricted saiyans.", GET_NAME(ch));
            SAIYAN_ALLOWED = true;
        } else if (!strcasecmp("majin", arg2)) {
            ch->sendf("You have unrestricted majins for the very next character creation.\r\n");
            send_to_imm("PERMISSION: %s unrestricted majins.", GET_NAME(ch));
            MAJIN_ALLOWED = true;
        } else {
            ch->sendf("You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
            return;
        }
    } else if (!strcasecmp("restrict", arg)) {
        ch->sendf("You have restricted character creation to standard race slection.\r\n");
        send_to_imm("PERMISSION: %s restricted races again.", GET_NAME(ch));
        MAJIN_ALLOWED = false;
    } else {
        ch->sendf("You want to @Grestrict@n or @Gunrestrict@n?\r\n");
        return;
    }
}

ACMD(do_transobj) {

    Object *obj;
    BaseCharacter *vict;
    struct descriptor_data *d;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg, arg2);

    if (!IS_NPC(ch) && GET_ADMLEVEL(ch) < 1) {
        ch->sendf("Huh!?");
        return;
    }

    if (!*arg || !*arg2) {
        ch->sendf("Syntax: transo (object) (target)\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, nullptr, ch->getInventory()))) {
        ch->sendf("You want to send what?\r\n");
        return;
    } else if (!strcasecmp("all", arg2)) {
        int num = GET_OBJ_VNUM(obj);
        Object *obj2 = nullptr;

        act("You send $p to everyone in the game.", true, ch, obj, nullptr, TO_CHAR);

        for (d = descriptor_list; d; d = d->next) {
            if (IS_NPC(d->character))
                continue;
            else if (!IS_PLAYING(d))
                continue;
            else if (d->character == ch)
                continue;
            else {
                act("$N sends $p across the universe to you.", true, d->character, obj, ch, TO_CHAR);
                obj2 = read_object(num, VIRTUAL);
                obj2->addToLocation(d->character);
            }
        }
    } else if (!(vict = get_char_vis(ch, arg2, nullptr, FIND_CHAR_WORLD))) {
        ch->sendf("That player is not in the game.\r\n");
        return;
    } else {
        act("You send $p to $N.", true, ch, obj, vict, TO_CHAR);
        act("$n sends $p across the universe to you.", true, ch, obj, vict, TO_VICT);
        obj->removeFromLocation();
        obj->addToLocation(vict);
        return;
    }
}

void search_replace(char *string, const char *find, const char *replace) {
    char final[MAX_STRING_LENGTH], temp[2];
    size_t start, end, i;

    while (strstr(string, find) != nullptr) {

        final[0] = '\0';
        start = strstr(string, find) - string;
        end = start + strlen(find);

        temp[1] = '\0';

        strncat(final, string, start);

        strcat(final, replace);

        for (i = end; string[i] != '\0'; i++) {
            temp[0] = string[i];
            strcat(final, temp);
        }

        sprintf(string, final);

    }
    return;
}

ACMD(do_interest) {
    if (GET_ADMLEVEL(ch) < 5) {
        ch->sendf("Huh!?\r\n");
        return;
    } else {
        if (INTERESTTIME > 0) {
            char *tmstr;
            tmstr = (char *) asctime(localtime(&INTERESTTIME));
            *(tmstr + strlen(tmstr) - 1) = '\0';
            ch->sendf("INTEREST TIME: [%s]\r\n", tmstr);
            return;
        }
        ch->sendf("Interest time has been initiated!\r\n");
        INTERESTTIME = time(nullptr) + 86400;
        LASTINTEREST = time(nullptr) + 86400;
        return;
    }
}

/* do_finddoor, finds the door(s) that a key goes to */
ACMD(do_finddoor) {
    int d, vnum = NOTHING, num = 0;
    size_t len, nlen;
    room_rnum i;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH] = {0};
    BaseCharacter *tmp_char;
    Object *obj;

    one_argument(argument, arg);

    if (!*arg) {
        ch->sendf("Format: finddoor <obj/vnum>\r\n");
    } else if (is_number(arg)) {
        ch->sendf("Temporarily disabled...");
        return;
    } else {
        generic_find(arg,
                     FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_WORLD | FIND_OBJ_EQUIP,
                     ch, &tmp_char, &obj);
        if (!obj)
            ch->sendf("What key do you want to find a door for?\r\n");
        else
            vnum = GET_OBJ_VNUM(obj);
    }
    if (vnum != NOTHING) {
        len = snprintf(buf, sizeof(buf), "Doors unlocked by key [%d] %s are:\r\n",
                       vnum, GET_OBJ_SHORT(obj));
        for (auto &[vn, u] : entities) {
            auto r = reg.try_get<Room>(u);
            if (!r)
                continue;
            for (auto &[d, e] : r->getExits()) {
                if (e && e->key &&
                    e->key == vnum) {
                    nlen = snprintf(buf + len, sizeof(buf) - len,
                                    "[%3d] Room %d, %s (%s)\r\n",
                                    ++num, vn,
                                    dirs[d], e->getName().c_str());
                    if (len + nlen >= sizeof(buf) || nlen < 0)
                        break;
                    len += nlen;
                }
            } /* for all directions */
        } /* for all rooms */
        if (num > 0) {
            write_to_output(ch->desc, buf);
        }
        else
            ch->sendf("No doors were found for key [%d] %s.\r\n",
                         vnum, GET_OBJ_SHORT(obj));
    }
}

ACMD(do_recall) {
    if (GET_ADMLEVEL(ch) < 1) {
        ch->sendf("You are not an immortal!\r\n");
    } else {
        ch->sendf("You disappear in a burst of light!\r\n");
        act("$n disappears in a burst of light!", false, ch, nullptr, nullptr, TO_ROOM);
        if (real_room(2) != NOWHERE) {
            ch->removeFromLocation();
            ch->addToLocation(getEntity(2));
            ch->lookAtLocation();
            GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
        }
    }

}

ACMD(do_hell) {
    BaseCharacter *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg) {
        ch->sendf("Syntax: lockout (character)\n"
                         "        lockout list\r\n");
        return;
    }

    if (!strcasecmp(arg, "Iovan") || !strcasecmp(arg, "iovan") || !strcasecmp(arg, "Fahl") ||
        !strcasecmp(arg, "fahl") || !strcasecmp(arg, "Xyron") || !strcasecmp(arg, "xyron") ||
        !strcasecmp(arg, "Samael") || !strcasecmp(arg, "samael")) {
        ch->sendf("What are you smoking? You can't lockout senior imms.\r\n");
        return;
    }

    if (!strcasecmp(arg, "list")) {
        print_lockout(ch);
        return;
    }

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD))) {
        lockWrite(ch, arg);
        return;
    } else {
        struct descriptor_data *d = vict->desc;
        extract_char(vict);
        lockWrite(ch, (char*)GET_NAME(vict));
        if (d && STATE(d) != CON_PLAYING) {
            STATE(d) = CON_CLOSE;
            vict->desc->character = nullptr;
            vict->desc = nullptr;
        }
        return;
    }
    return;
}

ACMD(do_echo) {
    skip_spaces(&argument);
    bool NoName = false;

    if (!*argument)
        ch->sendf("Yes.. but what?\r\n");
    else {
        char buf[8096];
        char name[128];
        int found = false, trunc = 0;
        BaseCharacter *vict = nullptr, *next_v = nullptr, *tch = nullptr;

        if (strlen(argument) > 7000) {
            trunc = strlen(argument) - 7000;
            argument[strlen(argument) - trunc] = '\0';
            sprintf(argument, "%s\n@D(@gMessage truncated to 7000 characters@D)@n\n", argument);
        }

        for (auto vict : ch->getRoom()->getPeople()) {
            if (vict == ch)
                continue;
            if (found == false) {
                sprintf(name, "*%s", GET_NAME(vict));
                if (strstr(argument, CAP(name))) {
                    found = true;
                    tch = vict;
                }
                if (found == false && !IS_NPC(vict)) {
                    if (readIntro(ch, vict) == 1) {
                        sprintf(name, "*%s", get_i_name(ch, vict));
                        if (strstr(argument, CAP(name))) {
                            found = true;
                            tch = vict;
                        }
                    }
                }
            }
        }

        if (subcmd == SCMD_SMOTE) {
            if (!strstr(argument, "#")) {
                NoName = true;
            }
            strlcpy(buf, argument, sizeof(buf));
            search_replace(buf, "#", "$n");
            search_replace(buf, "&1", "'@C");
            search_replace(buf, "&2", "@w'");
            if (found == true) {
                search_replace(buf, name, "$N");
            } else if (strstr(buf, "*")) {
                search_replace(buf, "*", "");
            }
        } else if (subcmd == SCMD_EMOTE) {
            snprintf(buf, sizeof(buf), "$n %s", argument);
            search_replace(buf, "#", "$n");
            search_replace(buf, "&1", "'@C");
            search_replace(buf, "&2", "@w'");
            if (found == true) {
                search_replace(buf, name, "$N");
            }
        } else {
            snprintf(buf, sizeof(buf), "%s", argument);
        }
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT)) {
            ch->sendf("%s", CONFIG_OK);
        }
        if (NoName == true) {
            char blom[MAX_INPUT_LENGTH];
            sprintf(blom, "@D(@GOOC@W: @gSmote by user %s@D)@n",
                    IS_NPC(ch) ? GET_NAME(ch) : (ch->desc->account == nullptr ? "ERROR REPORT" : ch->desc->account->name.c_str()));
            act(blom, false, ch, nullptr, nullptr, TO_ROOM);
        }
        if (found == false) {
            act(buf, false, ch, nullptr, nullptr, TO_CHAR);
            act(buf, false, ch, nullptr, nullptr, TO_ROOM);
        } else {

            act(buf, false, ch, nullptr, tch, TO_CHAR);
            act(buf, false, ch, nullptr, tch, TO_NOTVICT);
            search_replace(buf, "$N", "you");
            act(buf, false, ch, nullptr, tch, TO_VICT);
        }
    }
}

ACMD(do_send) {
    char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    BaseCharacter *vict;

    half_chop(argument, arg, buf);

    if (!*arg) {
        ch->sendf("Send what to who?\r\n");
        return;
    }
    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD))) {
        ch->sendf("%s", CONFIG_NOPERSON);
        return;
    }
    vict->sendf("%s\r\n", buf);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        ch->sendf("Sent.\r\n");
    else
        ch->sendf("You send '%s' to %s.\r\n", buf, GET_NAME(vict));
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(BaseCharacter *ch, char *rawroomstr) {
    room_rnum location = NOWHERE;
    char roomstr[MAX_INPUT_LENGTH];
    Room *rm;

    one_argument(rawroomstr, roomstr);

    if (!*roomstr) {
        ch->sendf("You must supply a room number or name.\r\n");
        return (NOWHERE);
    }

    if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
        if ((location = real_room((room_vnum) atoi(roomstr))) == NOWHERE) {
            ch->sendf("No room exists with that number.\r\n");
            return (NOWHERE);
        }
    } else {
        BaseCharacter *target_mob;
        Object *target_obj;
        char *mobobjstr = roomstr;
        int num;

        num = get_number(&mobobjstr);
        if ((target_mob = get_char_vis(ch, mobobjstr, &num, FIND_CHAR_WORLD)) != nullptr) {
            if ((location = IN_ROOM(target_mob)) == NOWHERE) {
                ch->sendf("That character is currently lost.\r\n");
                return (NOWHERE);
            }
        } else if ((target_obj = get_obj_vis(ch, mobobjstr, &num)) != nullptr) {
            if (IN_ROOM(target_obj) != NOWHERE)
                location = IN_ROOM(target_obj);
            else if (target_obj->carried_by && IN_ROOM(target_obj->carried_by) != NOWHERE)
                location = IN_ROOM(target_obj->carried_by);
            else if (target_obj->worn_by && IN_ROOM(target_obj->worn_by) != NOWHERE)
                location = IN_ROOM(target_obj->worn_by);

            if (location == NOWHERE) {
                ch->sendf("That object is currently not in a room.\r\n");
                return (NOWHERE);
            }
        }

        if (location == NOWHERE) {
            ch->sendf("Nothing exists by that name.\r\n");
            return (NOWHERE);
        }
    }

    /* a location has been found -- if you're >= GRGOD, no restrictions. */
    if (GET_ADMLEVEL(ch) >= ADMLVL_VICE)
        return (location);

    rm = getEntity<Room>(location);

    if ((!can_edit_zone(ch, rm->zone) && GET_ADMLEVEL(ch) < ADMLVL_GOD)
        && ZONE_FLAGGED(rm->zone, ZONE_QUEST)) {
        ch->sendf("This target is in a quest zone.\r\n");
        return (NOWHERE);
    }

    if ((GET_ADMLEVEL(ch) < ADMLVL_VICE) && ZONE_FLAGGED(rm->zone, ZONE_NOIMMORT)) {
        ch->sendf("This target is in a zone closed to all.\r\n");
        return (NOWHERE);
    }

    if (ROOM_FLAGGED(location, ROOM_GODROOM))
        ch->sendf("You are not godly enough to use that room!\r\n");
    else
        return (location);

    return (NOWHERE);
}

ACMD(do_at) {
    char command[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    room_rnum location;

    half_chop(argument, buf, command);
    if (!*buf) {
        ch->sendf("You must supply a room number or a name.\r\n");
        return;
    }

    if (!*command) {
        ch->sendf("What do you want to do there?\r\n");
        return;
    }

    if ((location = find_target_room(ch, buf)) == NOWHERE)
        return;
    auto r = getEntity(location);

    /* a location has been found. */
    auto original = ch->getRoom();
    ch->removeFromLocation();
    ch->addToLocation(r);
    ch->executeCommand(command);

    /* check if the char is still there */
    if (ch->getRoom()->getUID() == location) {
        ch->removeFromLocation();
        ch->addToLocation(original);
    }
}

ACMD(do_goto) {
    char buf[MAX_STRING_LENGTH];
    room_rnum location;

    if ((location = find_target_room(ch, argument)) == NOWHERE)
        return;
    if (PLR_FLAGGED(ch, PLR_HEALT)) {
        ch->sendf("They are inside a healing tank!\r\n");
        return;
    }

    auto r = getEntity<Room>(location);
    if(!r) {
        ch->sendf("That room does not exist.\r\n");
        return;
    }

    snprintf(buf, sizeof(buf), "$n %s", POOFOUT(ch) ? POOFOUT(ch) : "disappears in a puff of smoke.");
    act(buf, true, ch, nullptr, nullptr, TO_ROOM);

    ch->removeFromLocation();
    ch->addToLocation(r);

    snprintf(buf, sizeof(buf), "$n %s", POOFIN(ch) ? POOFIN(ch) : "appears with an ear-splitting bang.");
    act(buf, true, ch, nullptr, nullptr, TO_ROOM);

    ch->lookAtLocation();
    enter_wtrigger(r, ch, -1);
}

ACMD(do_trans) {
    char buf[MAX_INPUT_LENGTH];
    struct descriptor_data *i;
    BaseCharacter *victim;

    one_argument(argument, buf);
    if (!*buf)
        ch->sendf("Whom do you wish to transfer?\r\n");
    else if (strcasecmp("all", buf)) {
        if (!(victim = get_char_vis(ch, buf, nullptr, FIND_CHAR_WORLD)))
            ch->sendf("%s", CONFIG_NOPERSON);
        else if (victim == ch)
            ch->sendf("That doesn't make much sense, does it?\r\n");
        else {
            if ((GET_ADMLEVEL(ch) < GET_ADMLEVEL(victim)) && !IS_NPC(victim)) {
                ch->sendf("Go transfer someone your own size.\r\n");
                return;
            }
            if (PLR_FLAGGED(victim, PLR_HEALT)) {
                ch->sendf("They are inside a healing tank!\r\n");
                return;
            }
            act("$n disappears in a mushroom cloud.", false, victim, nullptr, nullptr, TO_ROOM);
            auto r = ch->getRoom();
            victim->removeFromLocation();
            victim->addToLocation(r);
            act("$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
            act("$n has transferred you!", false, ch, nullptr, victim, TO_VICT);
            victim->lookAtLocation();
            enter_wtrigger(r, victim, -1);
        }
    } else {            /* Trans All */
        if (!ADM_FLAGGED(ch, ADM_TRANSALL)) {
            ch->sendf("I think not.\r\n");
            return;
        }
        auto r = ch->getRoom();

        for (i = descriptor_list; i; i = i->next)
            if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
                victim = i->character;
                if (GET_ADMLEVEL(victim) >= GET_ADMLEVEL(ch))
                    continue;
                act("$n disappears in a mushroom cloud.", false, victim, nullptr, nullptr, TO_ROOM);
                victim->removeFromLocation();
                victim->addToLocation(r);
                act("$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
                act("$n has transferred you!", false, ch, nullptr, victim, TO_VICT);
                victim->lookAtLocation();
                enter_wtrigger(r, victim, -1);
            }
        ch->sendf("%s", CONFIG_OK);
    }
}

ACMD(do_teleport) {
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
    BaseCharacter *victim;
    room_rnum target;

    two_arguments(argument, buf, buf2);

    if (!*buf)
        ch->sendf("Whom do you wish to teleport?\r\n");
    else if (!(victim = get_char_vis(ch, buf, nullptr, FIND_CHAR_WORLD)))
        ch->sendf("%s", CONFIG_NOPERSON);
    else if (victim == ch)
        ch->sendf("Use 'goto' to teleport yourself.\r\n");
    else if (GET_ADMLEVEL(victim) >= GET_ADMLEVEL(ch))
        ch->sendf("Maybe you shouldn't do that.\r\n");
    else if (!*buf2)
        ch->sendf("Where do you wish to send this person?\r\n");
    else if ((target = find_target_room(ch, buf2)) != NOWHERE) {
        if (PLR_FLAGGED(victim, PLR_HEALT)) {
            ch->sendf("They are inside a healing tank!\r\n");
            return;
        }
        auto r = getEntity<Room>(target);
        ch->sendf("%s", CONFIG_OK);
        act("$n disappears in a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
        victim->removeFromLocation();
        victim->addToLocation(r);
        act("$n arrives from a puff of smoke.", false, victim, nullptr, nullptr, TO_ROOM);
        act("$n has teleported you!", false, ch, nullptr, (char *) victim, TO_VICT);
        victim->lookAtLocation();
        enter_wtrigger(r, victim, -1);
    }
}

ACMD(do_vnum) {
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

    half_chop(argument, buf, buf2);

    if (!*buf || !*buf2 ||
        (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj") && !is_abbrev(buf, "mat") && !is_abbrev(buf, "wtype") &&
         !is_abbrev(buf, "atype"))) {
        ch->sendf("Usage: vnum { atype | material | mob | obj | wtype } <name>\r\n");
        return;
    }
    if (is_abbrev(buf, "mob"))
        if (!vnum_mobile(buf2, ch))
            ch->sendf("No mobiles by that name.\r\n");

    if (is_abbrev(buf, "obj"))
        if (!vnum_object(buf2, ch))
            ch->sendf("No objects by that name.\r\n");

    if (is_abbrev(buf, "mat"))
        if (!vnum_material(buf2, ch))
            ch->sendf("No materials by that name.\r\n");

    if (is_abbrev(buf, "wtype"))
        if (!vnum_weapontype(buf2, ch))
            ch->sendf("No weapon types by that name.\r\n");

    if (is_abbrev(buf, "atype"))
        if (!vnum_armortype(buf2, ch))
            ch->sendf("No armor types by that name.\r\n");

}

#define ZOCMD zone_table[zrnum].cmd[subcmd]

void list_zone_commands_room(BaseCharacter *ch, room_vnum rvnum) {
    zone_rnum zrnum = real_zone_by_thing(rvnum);
    room_rnum rrnum = real_room(rvnum), cmd_room = NOWHERE;
    int subcmd = 0, count = 0;

    if (zrnum == NOWHERE || rrnum == NOWHERE) {
        ch->sendf("No zone information available.\r\n");
        return;
    }

    ch->sendf("Zone commands in this room:@y\r\n");
    while (ZOCMD.command != 'S') {
        switch (ZOCMD.command) {
            case 'M':
            case 'O':
            case 'T':
            case 'V':
                cmd_room = ZOCMD.arg3;
                break;
            case 'D':
            case 'R':
                cmd_room = ZOCMD.arg1;
                break;
            default:
                break;
        }
        if (cmd_room == rrnum) {
            count++;
            /* start listing */
            switch (ZOCMD.command) {
                case 'M':
                    ch->sendf("%sLoad %s@y [@c%d@y], MaxMud : %d, MaxR : %d, Chance : %d\r\n",
                                 ZOCMD.if_flag ? " then " : "",
                                 mob_proto[ZOCMD.arg1]["short_description"].get<std::string>().c_str(),
                                 mob_index[ZOCMD.arg1].vn, ZOCMD.arg2,
                                 ZOCMD.arg4, ZOCMD.arg5
                    );
                    break;
                case 'G':
                    ch->sendf("%sGive it %s@y [@c%d@y], Max : %d, Chance : %d\r\n",
                                 ZOCMD.if_flag ? " then " : "",
                                 obj_proto[ZOCMD.arg1]["short_description"].get<std::string>().c_str(),
                                 obj_index[ZOCMD.arg1].vn,
                                 ZOCMD.arg2, ZOCMD.arg5
                    );
                    break;
                case 'O':
                    ch->sendf("%sLoad %s@y [@c%d@y], Max : %d, MaxR : %d, Chance : %d\r\n",
                                 ZOCMD.if_flag ? " then " : "",
                                 obj_proto[ZOCMD.arg1]["short_description"].get<std::string>().c_str(),
                                 obj_index[ZOCMD.arg1].vn,
                                 ZOCMD.arg2, ZOCMD.arg4, ZOCMD.arg5
                    );
                    break;
                case 'E':
                    ch->sendf("%sEquip with %s@y [@c%d@y], %s, Max : %d, Chance : %d\r\n",
                                 ZOCMD.if_flag ? " then " : "",
                                 obj_proto[ZOCMD.arg1]["short_description"].get<std::string>().c_str(),
                                 obj_index[ZOCMD.arg1].vn,
                                 equipment_types[ZOCMD.arg3],
                                 ZOCMD.arg2, ZOCMD.arg5
                    );
                    break;
                case 'P':
                    ch->sendf("%sPut %s@y [@c%d@y] in %s@y [@c%d@y], Max : %d, Chance : %d\r\n",
                                 ZOCMD.if_flag ? " then " : "",
                                 obj_proto[ZOCMD.arg1]["short_description"].get<std::string>().c_str(),
                                 obj_index[ZOCMD.arg1].vn,
                                 obj_proto[ZOCMD.arg3]["short_description"].get<std::string>().c_str(),
                                 obj_index[ZOCMD.arg3].vn,
                                 ZOCMD.arg2, ZOCMD.arg5
                    );
                    break;
                case 'R':
                    ch->sendf("%sRemove %s@y [@c%d@y] from room.\r\n",
                                 ZOCMD.if_flag ? " then " : "",
                                 obj_proto[ZOCMD.arg2]["short_description"].get<std::string>().c_str(),
                                 obj_index[ZOCMD.arg2].vn
                    );
                    break;
                case 'D':
                    ch->sendf("%sSet door %s as %s.\r\n",
                                 ZOCMD.if_flag ? " then " : "",
                                 dirs[ZOCMD.arg2],
                                 ZOCMD.arg3 ? ((ZOCMD.arg3 == 1) ? "closed" : "locked") : "open"
                    );
                    break;
                case 'T':
                    ch->sendf("%sAttach trigger @c%s@y [@c%d@y] to %s\r\n",
                                 ZOCMD.if_flag ? " then " : "",
                                 trig_index[ZOCMD.arg2]->name,
                                 trig_index[ZOCMD.arg2]->vn,
                                 ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
                                  ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
                                   ((ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))));
                    break;
                case 'V':
                    ch->sendf("%sAssign global %s:%d to %s = %s\r\n",
                                 ZOCMD.if_flag ? " then " : "",
                                 ZOCMD.sarg1.c_str(), ZOCMD.arg2,
                                 ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
                                  ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
                                   ((ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))),
                                 ZOCMD.sarg2.c_str());
                    break;
                default:
                    ch->sendf("<Unknown Command>\r\n");
                    break;
            }
        }
        subcmd++;
    }
    ch->sendf("@n");
    if (!count)
        ch->sendf("None!\r\n");

}

#undef ZOCMD

static void do_stat_room(BaseCharacter *ch) {
    char buf2[MAX_STRING_LENGTH];
    struct extra_descr_data *desc;
    Room *rm = ch->getRoom();
    int i, found, column;
    Object *j;
    BaseCharacter *k;

    ch->sendf("Room name: @c%s@n\r\n", rm->getDisplayName(ch));

    sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
    ch->sendf("Zone: [%3d], VNum: [@g%5d@n], RNum: [%5d], IDNum: [%5ld], Type: %s\r\n",
                 zone_table[rm->zone].number, rm->getVN(), IN_ROOM(ch),
                 (long) rm->getVN(), buf2);

    ch->sendf("Room Damage: %d, Room Effect: %d\r\n", rm->getDamage(), rm->geffect);
    ch->sendf("SpecProc: %s, Flags: %s\r\n", rm->func == nullptr ? "None" : "Exists", buf2);

    ch->sendf("Description:\r\n%s", withPlaceholder(rm->getLookDesc(), "  None.\r\n"));

    if (!rm->ex_description.empty()) {
        ch->sendf("Extra descs:");
        for (auto desc : rm->ex_description)
            ch->sendf(" [@c%s@n]", desc.keyword);
        ch->sendf("\r\n");
    }

    ch->sendf("Chars present:");
    column = 14;    /* ^^^ strlen ^^^ */
    found = false;
    for (auto k : rm->getPeople()) {
        if (!CAN_SEE(ch, k))
            continue;

        ch->sendf("%s @y%s@n(%s)", found++ ? "," : "", GET_NAME(k),
                               !IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB"));
    }

    if (auto inv = rm->getInventory(); !inv.empty()) {
        std::vector<std::string> names;
        for (auto obj : inv) {
            if (CAN_SEE_OBJ(ch, obj)) {
                names.push_back(obj->getShortDesc());
            }
        }
        ch->sendf("Contents:@g %s@n\r\n", join(names, ", "));
    }

    for (auto &[i, e] : rm->getExits()) {
        char buf1[128];


        if (!e)
            continue;

        if (auto dest = e->getDestination(); dest)
            snprintf(buf1, sizeof(buf1), " @cNONE@n");
        else
            snprintf(buf1, sizeof(buf1), "@c%5d@n", dest->getUID());
        
        snprintf(buf2, sizeof(buf2), "%s", join(e->getFlagNames(FlagType::Exit), ", ").c_str());

        ch->sendf(
                     "Exit @c%-5s@n:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n  DC Lock: [%2d], DC Hide: [%2d], DC Skill: [%4s], DC Move: [%2d]\r\n%s\r\n",
                     dirs[i], buf1,
                     e->key == NOTHING ? -1 : e->key,
                     withPlaceholder(e->getAlias(), "None").c_str(),
                     e->dclock, e->dchide,
                     e->dcskill == 0 ? "None" : spell_info[e->dcskill].name,
                     e->dcmove,
                     withPlaceholder(e->getLookDesc(), "None").c_str());
    }

    /* check the room for a script */
    do_sstat(ch, rm);

    list_zone_commands_room(ch, rm->getVN());
}

static void do_stat_object(BaseCharacter *ch, Object *j) {
    int i, found;
    obj_vnum vnum;
    Object *j2;
    BaseCharacter *sitter;
    struct extra_descr_data *desc;
    char buf[MAX_STRING_LENGTH];

    vnum = GET_OBJ_VNUM(j);
    if (GET_LAST_LOAD(j) > 0) {
        char *tmstr;
        tmstr = (char *) asctime(localtime(&GET_LAST_LOAD(j)));
        *(tmstr + strlen(tmstr) - 1) = '\0';
        ch->sendf("LOADED DROPPED: [%s]\r\n", tmstr);
    }
    if (GET_OBJ_VNUM(j) == 65) {
        ch->sendf("Healing Tank Charge Level: [%d]\r\n", HCHARGE(j));
    }

    ch->sendf("Name: '%s', Keywords: %s, Size: %s\r\n",
                 withPlaceholder(j->getShortDesc(), "<None>"), j->getDisplayName(ch),
                 size_names[GET_OBJ_SIZE(j)]);

    sprinttype(GET_OBJ_TYPE(j), item_types, buf, sizeof(buf));
    ch->sendf("VNum: [@g%5d@n], RNum: [%5d], Idnum: [%5d], Type: %s, SpecProc: %s\r\n",
                 vnum, GET_OBJ_RNUM(j), ((j)->getUID()), buf, GET_OBJ_SPEC(j) ? "Exists" : "None");

    ch->sendf("@nUnique ID: @g%" I64T "@n\r\n", j->getUID());

    ch->sendf("Object Hit Points: [ @g%3d@n/@g%3d@n]\r\n",
                 GET_OBJ_VAL(j, VAL_ALL_HEALTH), GET_OBJ_VAL(j, VAL_ALL_MAXHEALTH));

    ch->sendf("Object loaded in room: @y%d@n\r\n",
                 OBJ_LOADROOM(j));

    ch->sendf("Object Material: @y%s@n\r\n",
                 material_names[GET_OBJ_MATERIAL(j)]);

    if (SITTING(j)) {
        sitter = SITTING(j);
        ch->sendf("HOLDING: %s\r\n", GET_NAME(sitter));
    }

    if (!j->ex_description.empty()) {
        ch->sendf("Extra descs:");
        for (auto desc : j->ex_description)
            ch->sendf(" [@c%s@n]", desc.keyword);
        ch->sendf("\r\n");
    }

    // TODO: sprintbitarray(GET_OBJ_WEAR(j), wear_bits, TW_ARRAY_MAX, buf);
    ch->sendf("Can be worn on: %s\r\n", buf);

    // TODO: sprintbitarray(GET_OBJ_PERM(j), affected_bits, AF_ARRAY_MAX, buf);
    ch->sendf("Set char bits : %s\r\n", buf);

    // TODO: sprintbitarray(GET_OBJ_EXTRA(j), extra_bits, EF_ARRAY_MAX, buf);
    ch->sendf("Extra flags   : %s\r\n", buf);

    auto wString = fmt::format("{}", GET_OBJ_WEIGHT(j));
    ch->sendf("Weight: %s, Value: %d, Cost/day: %d, Timer: %d, Min Level: %d\r\n",
                 wString.c_str(), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j), GET_OBJ_LEVEL(j));

    ch->sendf("In room: %d (%s), ", GET_ROOM_VNUM(IN_ROOM(j)),
                 IN_ROOM(j) == NOWHERE ? "Nowhere" : j->getRoom()->getDisplayName(ch));

    /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
    ch->sendf("In object: %s, ", j->in_obj ? j->in_obj->getShortDesc() : "None");
    ch->sendf("Carried by: %s, ", j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
    ch->sendf("Worn by: %s\r\n", j->worn_by ? GET_NAME(j->worn_by) : "Nobody");

    switch (GET_OBJ_TYPE(j)) {
        case ITEM_LIGHT:
            if (GET_OBJ_VAL(j, VAL_LIGHT_HOURS) == -1)
                ch->sendf("Hours left: Infinite\r\n");
            else
                ch->sendf("Hours left: [%d]\r\n", GET_OBJ_VAL(j, VAL_LIGHT_HOURS));
            break;
        case ITEM_SCROLL:
            ch->sendf("Spell: (Level %d) %s\r\n", GET_OBJ_VAL(j, VAL_SCROLL_LEVEL),
                         skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL1)));
            break;
        case ITEM_POTION:
            ch->sendf("Spells: (Level %d) %s, %s, %s\r\n", GET_OBJ_VAL(j, VAL_POTION_LEVEL),
                         skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL1)),
                         skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL2)),
                         skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL3)));
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            ch->sendf("Spell: %s at level %d, %d (of %d) charges remaining\r\n",
                         skill_name(GET_OBJ_VAL(j, VAL_STAFF_SPELL)), GET_OBJ_VAL(j, VAL_STAFF_LEVEL),
                         GET_OBJ_VAL(j, VAL_STAFF_CHARGES), GET_OBJ_VAL(j, VAL_STAFF_MAXCHARGES));
            break;
        case ITEM_WEAPON:
            ch->sendf("Weapon Type: %s, Todam: %dd%d, Message type: %d\r\n",
                         weapon_type[GET_OBJ_VAL(j, VAL_WEAPON_SKILL)],
                         GET_OBJ_VAL(j, VAL_WEAPON_DAMDICE),
                         GET_OBJ_VAL(j, VAL_WEAPON_DAMSIZE),
                         GET_OBJ_VAL(j, VAL_WEAPON_DAMTYPE));
            ch->sendf("Average damage per round %.1f\r\n",
                         ((GET_OBJ_VAL(j, VAL_WEAPON_DAMSIZE) + 1) / 2.0) * GET_OBJ_VAL (j, VAL_WEAPON_DAMDICE));
            ch->sendf("Crit type: %s, Crit range: %d-20\r\n",
                         crit_type[GET_OBJ_VAL(j, 6)], 20 - GET_OBJ_VAL(j, 8));
            break;
        case ITEM_ARMOR:
            ch->sendf("Armor Type: %s, AC-apply: [%d]\r\n", armor_type[GET_OBJ_VAL(j, VAL_ARMOR_SKILL)],
                         GET_OBJ_VAL(j, VAL_ARMOR_APPLYAC));
            ch->sendf("Max dex bonus: %d, Armor penalty: %d, Spell failure: %d\r\n",
                         GET_OBJ_VAL(j, VAL_ARMOR_MAXDEXMOD), GET_OBJ_VAL(j, VAL_ARMOR_CHECK),
                         GET_OBJ_VAL(j, VAL_ARMOR_SPELLFAIL));
            break;
        case ITEM_TRAP:
            ch->sendf("Spell: %d, - Hitpoints: %d\r\n", GET_OBJ_VAL(j, VAL_TRAP_SPELL),
                         GET_OBJ_VAL(j, VAL_TRAP_HITPOINTS));
            break;
        case ITEM_CONTAINER:
            sprintbit(GET_OBJ_VAL(j, VAL_CONTAINER_FLAGS), container_bits, buf, sizeof(buf));
            ch->sendf("Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s\r\n",
                         GET_OBJ_VAL(j, VAL_CONTAINER_CAPACITY), buf, GET_OBJ_VAL(j, VAL_CONTAINER_KEY),
                         YESNO(GET_OBJ_VAL(j, VAL_CONTAINER_CORPSE)));
            break;
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
            sprinttype(GET_OBJ_VAL(j, VAL_DRINKCON_LIQUID), drinks, buf, sizeof(buf));
            ch->sendf("Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s\r\n",
                         GET_OBJ_VAL(j, VAL_DRINKCON_CAPACITY), GET_OBJ_VAL(j, VAL_DRINKCON_HOWFULL),
                         YESNO(GET_OBJ_VAL(j, VAL_DRINKCON_POISON)), buf);
            break;
        case ITEM_NOTE:
            ch->sendf("Tongue: %d\r\n", GET_OBJ_VAL(j, VAL_NOTE_LANGUAGE));
            break;
        case ITEM_KEY:
            /* Nothing */
            break;
        case ITEM_FOOD:
            ch->sendf("Makes full: %d, Poisoned: %s\r\n", GET_OBJ_VAL(j, VAL_FOOD_FOODVAL),
                         YESNO(GET_OBJ_VAL(j, VAL_FOOD_POISON)));
            break;
        case ITEM_MONEY:
            ch->sendf("Coins: %d\r\n", GET_OBJ_VAL(j, VAL_MONEY_SIZE));
            break;
        default:
            ch->sendf("Values 0-12: [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d]\r\n",
                         GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
                         GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3),
                         GET_OBJ_VAL(j, 4), GET_OBJ_VAL(j, 5),
                         GET_OBJ_VAL(j, 6), GET_OBJ_VAL(j, 7),
                         GET_OBJ_VAL(j, 8), GET_OBJ_VAL(j, 9),
                         GET_OBJ_VAL(j, 10), GET_OBJ_VAL(j, 11));
            break;
    }

    /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

    if (auto inv = j->getInventory(); !inv.empty()) {
        std::vector<std::string> names;
        for (auto j2 : inv) {
            names.emplace_back(j2->getShortDesc());
        }

        ch->sendf("\r\nContents:@g %s@n", join(names, ", "));
    }

    found = false;
    ch->sendf("Affections:");
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
        if (j->affected[i].location != APPLY_NONE) {
            sprinttype(j->affected[i].location, apply_types, buf, sizeof(buf));
            auto m = fmt::format("{}", j->affected[i].modifier);
            ch->sendf("%s %s to %s", found++ ? "," : "", m.c_str(), buf);
            switch (j->affected[i].location) {
                case APPLY_FEAT:
                    ch->sendf(" (%s)", feat_list[j->affected[i].specific].name);
                    break;
                case APPLY_SKILL:
                    ch->sendf(" (%s)", spell_info[j->affected[i].specific].name);
                    break;
            }
        }
    if (!found)
        ch->sendf(" None");

    ch->sendf("\r\n");

    /* check the object for a script */
    do_sstat(ch, j);
}

static void do_stat_character(BaseCharacter *ch, BaseCharacter *k) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int i, i2, column, found = false;
    Object *j;
    Object *chair;
    struct follow_type *fol;
    struct affected_type *aff;


    if (IS_NPC(k)) {
        char *tmstr;
        tmstr = (char *) asctime(localtime(&GET_LPLAY(k)));
        *(tmstr + strlen(tmstr) - 1) = '\0';
        ch->sendf("LOADED AT: [%s]\r\n", tmstr);
    }
    sprinttype(GET_SEX(k), genders, buf, sizeof(buf));
    ch->sendf("%s %s '%s'  IDNum: [%5d], In room [%5d], Loadroom : [%5d]\r\n",
                 buf, (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
                 GET_NAME(k), IS_NPC(k) ? ((k)->getUID()) : GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)),
                 IS_NPC(k) ? MOB_LOADROOM(k) : GET_LOADROOM(k));

    ch->sendf("DROOM: [%5d]\r\n", GET_DROOM(k));
    if (IS_MOB(k)) {
        if (k->master_id > -1)
            sprintf(buf, ", Master: %s", get_name_by_id(k->master_id));
        else
            buf[0] = 0;
        ch->sendf("Keyword: %s, VNum: [%5d], RNum: [%5d]%s\r\n", k->getName(),
                     GET_MOB_VNUM(k), GET_MOB_RNUM(k), buf);
    } else

        ch->sendf("Title: %s\r\n", k->title ? k->title : "<None>");

    ch->sendf("L-Des: %s@n", withPlaceholder(k->getRoomDesc(), "<None>\r\n"));
    snprintf(buf, sizeof(buf), "%s", sensei::getName(k->chclass).c_str());
    snprintf(buf2, sizeof(buf2), "%s", race::getName(k->race).c_str());
    ch->sendf("Class: %s, Race: %s, Lev: [@y%2d@n], XP: [@y%" I64T "@n]\r\n",
                 buf, buf2, GET_LEVEL(k), GET_EXP(k));

    if (!IS_NPC(k)) {
        char buf1[64], cmbuf2[64];

        strlcpy(buf1, asctime(localtime(&(k->time.created))), sizeof(buf1));
        strlcpy(cmbuf2, asctime(localtime(&(k->time.logon))), sizeof(cmbuf2));
        buf1[10] = cmbuf2[10] = '\0';

        ch->sendf("Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
                     buf1, cmbuf2, (int) k->time.played / 3600,
                     (int) (((int64_t)k->time.played % 3600) / 60), 0);

        if (k->desc != nullptr) {
            ch->sendf("@YOwned by User@D: [@C%s@D]@n\r\n", GET_USER(k));
        } else {
            ch->sendf("@YOwned by User@D: [@C%s@D]@n\r\n", GET_LOG_USER(k));
        }
        if (!IS_NPC(k)) {
            ch->sendf("@RCharacter Deaths@D: @r%d@n\r\n", GET_DCOUNT(k));
        }

        ch->sendf("Hometown: [%d], Align: [%4d], Ethic: [%4d]", GET_HOME(k),
                     GET_ALIGNMENT(k), GET_ETHIC_ALIGNMENT(k));

        /*. Display OLC zone for immorts .*/
        if (GET_ADMLEVEL(k) >= ADMLVL_BUILDER) {
            if (GET_OLC_ZONE(k) == AEDIT_PERMISSION)
                ch->sendf(", OLC[@cActions@n]");
            else if (GET_OLC_ZONE(k) == HEDIT_PERMISSION)
                ch->sendf(", OLC[@cHedit@n]");
            else if (GET_OLC_ZONE(k) == NOWHERE)
                ch->sendf(", OLC[@cOFF@n]");
            else
                ch->sendf(", OLC: [@c%d@n]", GET_OLC_ZONE(k));
        }
        ch->sendf("\r\n");
    }
    ch->sendf("Str: [@c%d@n]  Int: [@c%d@n]  Wis: [@c%d@n]  "
                     "Dex: [@c%d@n]  Con: [@c%d@n]  Cha: [@c%d@n]\r\n",
                 GET_STR(k), GET_INT(k), GET_WIS(k), GET_DEX(k), GET_CON(k), GET_CHA(k));

    ch->sendf("PL :[@g%12s@n]  KI :[@g%12s@n]  ST :[@g%12s@n]\r\n",
                 add_commas(GET_HIT(k)).c_str(), add_commas((k->getCurKI())).c_str(), add_commas((k->getCurST())).c_str());
    ch->sendf("MPL:[@g%12s@n]  MKI:[@g%12s@n]  MST:[@g%12s@n]\r\n",
                 add_commas(GET_MAX_HIT(k)).c_str(), add_commas(GET_MAX_MANA(k)).c_str(), add_commas(GET_MAX_MOVE(k)).c_str());
    ch->sendf("BPL:[@g%12s@n]  BKI:[@g%12s@n]  BST:[@g%12s@n]\r\n",
                 add_commas((k->getBasePL())).c_str(), add_commas((k->getBaseKI())).c_str(), add_commas((k->getBaseST())).c_str());
    ch->sendf("LF :[@g%12s@n]  MLF:[@g%12s@n]  LFP:[@g%3d@n]\r\n",
                 add_commas((k->getCurLF())).c_str(), add_commas((k->getMaxLF())).c_str(), GET_LIFEPERC(k));

    if (GET_ADMLEVEL(k))
        ch->sendf("Admin Level: [@y%d - %s@n]\r\n", GET_ADMLEVEL(k), admin_level_names[GET_ADMLEVEL(k)]);

    ch->sendf("Coins: [%9d], Bank: [%9d] (Total: %d)\r\n",
                 GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));

    ch->sendf("Armor: [%d ], Damage: [%2d]\r\n",
                 GET_ARMOR(k), GET_DAMAGE_MOD(k));

    sprinttype(GET_POS(k), position_types, buf, sizeof(buf));
    ch->sendf("Pos: %s, Fighting: %s", buf, FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody");


    if (k->desc) {
        sprinttype(STATE(k->desc), connected_types, buf, sizeof(buf));
        ch->sendf(", Connected: %s", buf);
    }

    sprinttype(k->mob_specials.default_pos, position_types, buf, sizeof(buf));
    ch->sendf(", Default position: %s", buf);
    ch->sendf(", Idle Timer (in tics) [%d]\r\n", k->timer);
    sprintf(buf, "%s", join(k->getFlagNames(FlagType::NPC), ", ").c_str());
    ch->sendf("NPC flags: @c%s@n\r\n", buf);
    sprintf(buf, "%s", join(k->getFlagNames(FlagType::PC), ", ").c_str());
    ch->sendf("PLR flags: @c%s@n\r\n", buf);
    sprintf(buf, "%s", join(k->getFlagNames(FlagType::Pref), ", ").c_str());
    ch->sendf("PRF flags: @g%s@n\r\n", buf);

    ch->sendf("Form: %s\r\n", trans::getName(k, k->form));

    if (IS_MOB(k)) {
        ch->sendf("Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n",
                     (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"),
                     k->mob_specials.damnodice, k->mob_specials.damsizedice);
        ch->sendf("Average damage per round %.1f (%.1f [BHD] + %d [STR MOD] + %d [DMG MOD])\r\n",
                     (((k->mob_specials.damsizedice) + 1) / 2.0) * (k->mob_specials.damnodice) +
                     ability_mod_value(GET_STR(k)) + GET_DAMAGE_MOD(k),
                     (((k->mob_specials.damsizedice) + 1) / 2.0) * (k->mob_specials.damnodice),
                     ability_mod_value(GET_STR(k)), GET_DAMAGE_MOD(k));
    }

    int counts = 0, total = 0;
    i = 0;
    for (auto j : k->getInventory()) {
        counts += check_insidebag(j, 0.5);
        counts++;
        i++;
    }
    total = counts;
    total += i;
    i = 0;
    for (auto &[pos, o] : k->getEquipment())
        {
            i2++;
            total += check_insidebag(o, 0.5) + 1;
        }
    ch->sendf("Carried: weight: %d, Total Items (includes bagged items): %d, EQ: %d\r\n", (int64_t)IS_CARRYING_W(k),
                 total, i2);


    if (!IS_NPC(k))
        ch->sendf("Hunger: %d, Thirst: %d, Drunk: %d\r\n", GET_COND(k, HUNGER), GET_COND(k, THIRST),
                     GET_COND(k, DRUNK));

    ch->sendf("Master is: %s, Followers are:", k->master ? GET_NAME(k->master) : "<none>");
    if (!k->followers)
        ch->sendf(" <none>\r\n");
    else {
        for (fol = k->followers; fol; fol = fol->next) {
            ch->sendf("%s %s", found++ ? "," : "", PERS(fol->follower, ch));
            if (column >= 62) {
                ch->sendf("%s\r\n", fol->next ? "," : "");
                found = false;
                column = 0;
            }
        }
        if (column != 0)
            ch->sendf("\r\n");
    }

    if (SITS(k)) {
        chair = SITS(k);
        ch->sendf("Is on: %s@n\r\n", chair->getShortDesc());
    }

    /* Showing the bitvector */
    snprintf(buf, sizeof(buf), "%s", join(k->getFlagNames(FlagType::Affect), ", ").c_str());
    ch->sendf("AFF: @y%s@n\r\n", buf);

    /* Routine to show what spells a char is affected by */
    if (k->affected) {
        for (aff = k->affected; aff; aff = aff->next) {
            ch->sendf("SPL: (%3dhr) @c%-21s@n ", aff->duration + 1, skill_name(aff->type));

            if (aff->modifier)
                ch->sendf("%+d to %s", aff->modifier, apply_types[(int) aff->location]);

            if (aff->bitvector) {
                if (aff->modifier)
                    ch->sendf(", ");

                strcpy(buf, affected_bits[aff->bitvector]);
                ch->sendf("sets %s", buf);
            }
            ch->sendf("\r\n");
        }
    }

    /* Routine to show what spells a char is affectedv by */
    if (k->affectedv) {
        for (aff = k->affectedv; aff; aff = aff->next) {
            ch->sendf("SPL: (%3d rounds) @c%-21s@n ", aff->duration + 1, skill_name(aff->type));

            if (aff->modifier)
                ch->sendf("%+d to %s", aff->modifier, apply_types[(int) aff->location]);

            if (aff->bitvector) {
                if (aff->modifier)
                    ch->sendf(", ");

                strcpy(buf, affected_bits[aff->bitvector]);
                ch->sendf("sets %s", buf);
            }
            ch->sendf("\r\n");
        }
    }

    /* check mobiles for a script */
    if (IS_NPC(k)) {
        do_sstat(ch, k);
        if (SCRIPT_MEM(k)) {
            struct script_memory *mem = SCRIPT_MEM(k);
            ch->sendf("Script memory:\r\n  Remember             Command\r\n");
            while (mem) {
                auto find = uniqueCharacters.find(mem->id);
                if(find == uniqueCharacters.end()) {
                    ch->sendf("  ** Corrupted!\r\n");
                } else {
                    ch->sendf("  %-20.20s <default>\r\n", GET_NAME(find->second.second));
                }
                mem = mem->next;
            }
        }
    } else {
        int x, track = 0;

        ch->sendf("Bonuses/Negatives:\r\n");

        for (x = 0; x < 30; x++) {
            if (x < 15) {
                if (GET_BONUS(k, x) > 0) {
                    ch->sendf("@c%s@n\n", list_bonus[x]);
                    track += 1;
                }
            } else {
                if (GET_BONUS(k, x) > 0) {
                    ch->sendf("@r%s@n\n", list_bonus[x]);
                    track += 1;
                }
            }
        }
        if (track <= 0) {
            ch->sendf("@wNone.@n\r\n");
        }
        ch->sendf("To see player variables use varstat now.\r\n");
    }
}

ACMD(do_varstat) {
    BaseCharacter *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD))) {
        ch->sendf("That player is not in the game.\r\n");
        return;
    } else if (IS_NPC(vict)) {
        ch->sendf("Just use stat for an NPC\r\n");
        return;
    } else {
        /* Display their global variables */
        if (!vict->script->vars.empty()) {
            struct trig_var_data *tv;
            char uname[MAX_INPUT_LENGTH];
            void find_uid_name(char *uid, char *name, size_t nlen);

            ch->sendf("%s's Global Variables:\r\n", GET_NAME(vict));

            /* currently, variable context for players is always 0, so it is */
            /* not displayed here. in the future, this might change */
            for (auto &[name, v] : vict->script->vars) {
                if (auto u = resolveUID(v); u) {
                    ch->sendf("    %10s:  [UID]: %s\r\n", name, u->getName().c_str());
                } else {
                    ch->sendf("    %10s:  %s\r\n", name, v);
                }
            }
        }
    }

}

ACMD(do_stat) {
    char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
    BaseCharacter *victim;
    Object *object;

    half_chop(argument, buf1, buf2);

    if (!*buf1) {
        ch->sendf("Stats on who or what or where?\r\n");
        return;
    } else if (is_abbrev(buf1, "room")) {
        do_stat_room(ch);
    } else if (is_abbrev(buf1, "mob")) {
        if (!*buf2)
            ch->sendf("Stats on which mobile?\r\n");
        else {
            if ((victim = get_char_vis(ch, buf2, nullptr, FIND_CHAR_WORLD)) != nullptr)
                do_stat_character(ch, victim);
            else
                ch->sendf("No such mobile around.\r\n");
        }
    } else if (is_abbrev(buf1, "player")) {
        if (!*buf2) {
            ch->sendf("Stats on which player?\r\n");
        } else {
            if ((victim = findPlayer(buf2)) != nullptr)
                do_stat_character(ch, victim);
            else
                ch->sendf("No such player around.\r\n");
        }
    } else if (is_abbrev(buf1, "file")) {
        if (!*buf2)
            ch->sendf("Stats on which player?\r\n");
        else if ((victim = get_player_vis(ch, buf2, nullptr, FIND_CHAR_WORLD)) != nullptr)
            do_stat_character(ch, victim);
        else {
            victim = findPlayer(buf2);
            if(!victim) {
                ch->sendf("There is no such player.\r\n");
                return;
            }
            if (GET_ADMLEVEL(victim) > GET_ADMLEVEL(ch))
                ch->sendf("Sorry, you can't do that.\r\n");
            else
                do_stat_character(ch, victim);
        }
    } else if (is_abbrev(buf1, "object")) {
        if (!*buf2)
            ch->sendf("Stats on which object?\r\n");
        else {
            if ((object = get_obj_vis(ch, buf2, nullptr)) != nullptr)
                do_stat_object(ch, object);
            else
                ch->sendf("No such object around.\r\n");
        }
    } else if (is_abbrev(buf1, "zone")) {
        if (!*buf2) {
            ch->sendf("Stats on which zone?\r\n");
            return;
        } else {
            //print_zone(ch, atoi(buf2));
            return;
        }
    } else {
        char *name = buf1;
        int number = get_number(&name);

        if ((object = get_obj_in_equip_vis(ch, name, &number, ch->getEquipment())) != nullptr)
            do_stat_object(ch, object);
        else if ((object = get_obj_in_list_vis(ch, name, &number, ch->getInventory())) != nullptr)
            do_stat_object(ch, object);
        else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_ROOM)) != nullptr)
            do_stat_character(ch, victim);
        else if ((object = get_obj_in_list_vis(ch, name, &number, ch->getRoom()->getInventory())) != nullptr)
            do_stat_object(ch, object);
        else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_WORLD)) != nullptr)
            do_stat_character(ch, victim);
        else if ((object = get_obj_vis(ch, name, &number)) != nullptr)
            do_stat_object(ch, object);
        else
            ch->sendf("Nothing around by that name.\r\n");
    }
}

ACMD(do_shutdown) {
    char arg[MAX_INPUT_LENGTH];

    if (subcmd != SCMD_SHUTDOWN) {
        ch->sendf("If you want to shut something down, say so!\r\n");
        return;
    }
    one_argument(argument, arg);

    if (!*arg) {
        basic_mud_log("(GC) Shutdown by %s.", GET_NAME(ch));
        send_to_all("Shutting down.\r\n");
        circle_shutdown = 1;
    } else if (!strcasecmp(arg, "reboot")) {
        basic_mud_log("(GC) Reboot by %s.", GET_NAME(ch));
        send_to_all("Rebooting.. come back in a minute or two.\r\n");
        touch(FASTBOOT_FILE);
        circle_shutdown = circle_reboot = 1;
    } else if (!strcasecmp(arg, "die")) {
        basic_mud_log("(GC) Shutdown by %s.", GET_NAME(ch));
        send_to_all("Shutting down for maintenance.\r\n");
        touch(KILLSCRIPT_FILE);
        circle_shutdown = 1;
    } else if (!strcasecmp(arg, "now")) {
        basic_mud_log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
        send_to_all("Rebooting.. come back in a minute or two.\r\n");
        circle_shutdown = 1;
        circle_reboot = 2; /* do not autosave olc */
    } else if (!strcasecmp(arg, "pause")) {
        basic_mud_log("(GC) Shutdown by %s.", GET_NAME(ch));
        send_to_all("Shutting down for maintenance.\r\n");
        touch(PAUSE_FILE);
        circle_shutdown = 1;
    } else
        ch->sendf("Unknown shutdown option.\r\n");
}

void snoop_check(BaseCharacter *ch) {
    /*  This short routine is to ensure that characters that happen
   *  to be snooping (or snooped) and get advanced/demoted will
   *  not be snooping/snooped someone of a higher/lower level (and
   *  thus, not entitled to be snooping.
   */
    if (!ch || !ch->desc)
        return;
    if (ch->desc->snooping &&
        (GET_ADMLEVEL(ch->desc->snooping->character) >= GET_ADMLEVEL(ch))) {
        ch->desc->snooping->snoop_by = nullptr;
        ch->desc->snooping = nullptr;
    }

    if (ch->desc->snoop_by &&
        (GET_ADMLEVEL(ch) >= GET_ADMLEVEL(ch->desc->snoop_by->character))) {
        ch->desc->snoop_by->snooping = nullptr;
        ch->desc->snoop_by = nullptr;
    }
}

static void stop_snooping(BaseCharacter *ch) {
    if (!ch->desc->snooping)
        ch->sendf("You aren't snooping anyone.\r\n");
    else {
        ch->sendf("You stop snooping.\r\n");
        ch->desc->snooping->snoop_by = nullptr;
        ch->desc->snooping = nullptr;
    }
}

ACMD(do_snoop) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *victim, *tch;

    if (!ch->desc)
        return;

    one_argument(argument, arg);

    if (!*arg)
        stop_snooping(ch);
    else if (!(victim = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
        ch->sendf("No such person around.\r\n");
    else if (!victim->desc)
        ch->sendf("There's no link.. nothing to snoop.\r\n");
    else if (victim == ch)
        stop_snooping(ch);
    else if (victim->desc->snoop_by)
        ch->sendf("Busy already. \r\n");
    else if (victim->desc->snooping == ch->desc)
        ch->sendf("Don't be stupid.\r\n");
    else {
        if (victim->desc->original)
            tch = victim->desc->original;
        else
            tch = victim;

        if (GET_ADMLEVEL(tch) >= GET_ADMLEVEL(ch)) {
            ch->sendf("You can't.\r\n");
            return;
        }
        ch->sendf("%s", CONFIG_OK);

        if (ch->desc->snooping)
            ch->desc->snooping->snoop_by = nullptr;

        ch->desc->snooping = victim->desc;
        victim->desc->snoop_by = ch->desc;
    }
}

ACMD(do_switch) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *victim;

    one_argument(argument, arg);

    if (ch->desc->original)
        ch->sendf("You're already switched.\r\n");
    else if (!*arg)
        ch->sendf("Switch with who?\r\n");
    else if (!(victim = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
        ch->sendf("No such character.\r\n");
    else if (ch == victim)
        ch->sendf("Hee hee... we are jolly funny today, eh?\r\n");
    else if (victim->desc)
        ch->sendf("You can't do that, the body is already in use!\r\n");
    else if (!(IS_NPC(victim) || ADM_FLAGGED(ch, ADM_SWITCHMORTAL)))
        ch->sendf("You aren't holy enough to use a mortal's body.\r\n");
    else if (GET_ADMLEVEL(ch) < ADMLVL_VICE && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
        ch->sendf("You are not godly enough to use that room!\r\n");
    else {
        ch->sendf("%s", CONFIG_OK);

        ch->desc->character = victim;
        ch->desc->original = ch;

        victim->desc = ch->desc;
        ch->desc = nullptr;
    }
}

ACMD(do_return) {
    if (ch->desc && ch->desc->original) {
        ch->sendf("You return to your original body.\r\n");

        /*
     * If someone switched into your original body, disconnect them.
     *   - JE 2/22/95
     *
     * Zmey: here we put someone switched in our body to disconnect state
     * but we must also nullptr his pointer to our character, otherwise
     * close() will damage our character's pointer to our descriptor
     * (which is assigned below in this function). 12/17/99
     */
        if (ch->desc->original->desc) {
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

ACMD(do_load) {
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
    int i = 0, n = 1;

    one_argument(two_arguments(argument, buf, buf2), buf3);

    if (!*buf || !*buf2 || !isdigit(*buf2)) {
        ch->sendf("Usage: load { obj | mob } <vnum> (amt)\r\n");
        return;
    }
    if (!is_number(buf2) || !is_number(buf3)) {
        ch->sendf("That is not a number.\r\n");
        return;
    }

    if (atoi(buf3) > 0) {
        if (atoi(buf3) >= 100) {
            n = 100;
        } else if (atoi(buf3) < 100) {
            n = atoi(buf3);
        }
    } else {
        n = 1;
    }

    if (is_abbrev(buf, "mob")) {
        BaseCharacter *mob = nullptr;
        mob_rnum r_num;

        if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
            ch->sendf("There is no monster with that number.\r\n");
            return;
        }
        for (i = 0; i < n; i++) {
            mob = read_mobile(r_num, REAL);
            mob->addToLocation(ch->getRoom());

            act("$n makes a quaint, magical gesture with one hand.", true, ch, nullptr, nullptr, TO_ROOM);
            act("$n has created $N!", false, ch, nullptr, mob, TO_ROOM);
            act("You create $N.", false, ch, nullptr, mob, TO_CHAR);
            load_mtrigger(mob);
        }
    } else if (is_abbrev(buf, "obj")) {
        Object *obj;
        obj_rnum r_num;

        if ((r_num = real_object(atoi(buf2))) == NOTHING) {
            ch->sendf("There is no object with that number.\r\n");
            return;
        }
        for (i = 0; i < n; i++) {
            obj = read_object(r_num, REAL);
            if (GET_ADMLEVEL(ch) > 0) {
                send_to_imm("LOAD: %s has loaded a %s", GET_NAME(ch), obj->getShortDesc());
                log_imm_action("LOAD: %s has loaded a %s", GET_NAME(ch), obj->getShortDesc());
            }
            if (CONFIG_LOAD_INVENTORY)
                obj->addToLocation(ch);
            else
                obj->addToLocation(ch->getRoom());
            act("$n makes a strange magical gesture.", true, ch, nullptr, nullptr, TO_ROOM);
            act("$n has created $p!", false, ch, obj, nullptr, TO_ROOM);
            act("You create $p.", false, ch, obj, nullptr, TO_CHAR);
            load_otrigger(obj);
        }
    } else
        ch->sendf("That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_vstat) {
    char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

    two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2 || !isdigit(*buf2)) {
        ch->sendf("Usage: vstat { obj | mob } <number>\r\n");
        return;
    }
    if (!is_number(buf2)) {
        ch->sendf("That's not a valid number.\r\n");
        return;
    }

    if (is_abbrev(buf, "mob")) {
        BaseCharacter *mob;
        mob_rnum r_num;

        if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
            ch->sendf("There is no monster with that number.\r\n");
            return;
        }
        mob = read_mobile(r_num, REAL);
        mob->addToLocation(getEntity(0));
        do_stat_character(ch, mob);
        extract_char(mob);
    } else if (is_abbrev(buf, "obj")) {
        Object *obj;
        obj_rnum r_num;

        if ((r_num = real_object(atoi(buf2))) == NOTHING) {
            ch->sendf("There is no object with that number.\r\n");
            return;
        }
        obj = read_object(r_num, REAL);
        do_stat_object(ch, obj);
        obj->extractFromWorld();
    } else
        ch->sendf("That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_pgrant) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    BaseCharacter *vict;

    two_arguments(argument, arg1, arg2);

    std::string strForm;
    if(!*arg2) {
        vict = ch;
        strForm = arg1;
    } else {
        if (!(vict = get_char_vis(ch, arg1, nullptr, FIND_CHAR_WORLD))) {
            ch->sendf("No such character.\r\nUsage: pgrant <char> <target>\r\n");
            return;
        }
        strForm = arg2;
    }


    auto foundForm = trans::findForm(vict, strForm);

    if (!foundForm.has_value()) {
        ch->sendf("Form %s not found.\r\n", strForm);
        return;
    }

    if(!trans::getFormsFor(vict).contains(*foundForm)) {
        vict->addTransform(*foundForm);
        ch->sendf("Form %s added!\r\n", strForm);
        log_imm_action("Form Added: %s added %s to %s!", ch, strForm, vict);
    } else {
        if(vict->transforms.find(*foundForm)->second.visible) {
            vict->hideTransform(*foundForm, true);
            ch->sendf("Form %s hidden!\r\n", strForm);
            log_imm_action("Form Hidden: %s hidden %s from %s!", ch, strForm, vict);
        } else {
            vict->hideTransform(*foundForm, false);
            ch->sendf("Form %s unhidden!\r\n", strForm);
            log_imm_action("Form Unhidden: %s unhidden %s from %s!", ch, strForm, vict);
        }

        
    }

}

/* clean a room of all mobiles and objects */
ACMD(do_purge) {
    char buf[MAX_INPUT_LENGTH];
    BaseCharacter *vict;
    Object *obj;

    one_argument(argument, buf);

    /* argument supplied. destroy single object or char */
    if (*buf) {
        if ((vict = get_char_vis(ch, buf, nullptr, FIND_CHAR_WORLD)) != nullptr) {
            if (!IS_NPC(vict) && (GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict))) {
                ch->sendf("Fuuuuuuuuu!\r\n");
                return;
            }
            act("$n disintegrates $N.", false, ch, nullptr, vict, TO_NOTVICT);
            if (!IS_NPC(vict)) {
                send_to_all("@R%s@r purges @R%s's@r sorry ass right off the MUD!@n\r\n", GET_NAME(ch), GET_NAME(vict));
            }


            if (!IS_NPC(vict)) {
                mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s has purged %s.", GET_NAME(ch),
                       GET_NAME(vict));
                log_imm_action("PURGED: %s burned %s's sorry ass off the MUD!", GET_NAME(ch), GET_NAME(vict));
                if (vict->desc) {
                    STATE(vict->desc) = CON_CLOSE;
                    vict->desc->character = nullptr;
                    vict->desc = nullptr;
                }
            }
            extract_char(vict);
        } else if ((obj = get_obj_in_list_vis(ch, buf, nullptr, ch->getRoom()->getInventory())) != nullptr) {
            act("$n destroys $p.", false, ch, obj, nullptr, TO_ROOM);
            obj->extractFromWorld();
        } else {
            ch->sendf("Nothing here by that name.\r\n");
            return;
        }

        ch->sendf("%s", CONFIG_OK);
    } else {            /* no argument. clean out the room */
        int i;

        act("$n gestures... You are surrounded by scorching flames!",
            false, ch, nullptr, nullptr, TO_ROOM);
        send_to_room(IN_ROOM(ch), "The world seems a little cleaner.\r\n");

        for (auto vict : ch->getRoom()->getPeople()) {
            if (!IS_NPC(vict))
                continue;

            /* Dump inventory. */
            for (auto o : vict->getInventory())
                o->extractFromWorld();

            /* Dump equipment. */
            for (auto &[pos, o] : vict->getEquipment())
                    o->extractFromWorld();

            /* Dump character. */
            extract_char(vict);
        }

        /* Clear the ground. */
        for (auto o : ch->getRoom()->getInventory())
            o->extractFromWorld();
    }
}

static const char *logtypes[] = {
        "off", "brief", "normal", "complete", "\n"
};

ACMD(do_syslog) {
    char arg[MAX_INPUT_LENGTH];
    int tp;

    one_argument(argument, arg);
    if (!*arg) {
        ch->sendf("Your syslog is currently %s.\r\n",
                     logtypes[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)]);
        return;
    }
    if (((tp = search_block(arg, logtypes, false)) == -1)) {
        ch->sendf("Usage: syslog { Off | Brief | Normal | Complete }\r\n");
        return;
    }
    for(auto f : {PRF_LOG1, PRF_LOG2}) ch->clearFlag(FlagType::Pref, f);
    if (tp & 1) ch->setFlag(FlagType::Pref, PRF_LOG1);
    if (tp & 2) ch->setFlag(FlagType::Pref, PRF_LOG2);

    ch->sendf("Your syslog is now %s.\r\n", logtypes[tp]);
}


/* (c) 1996-97 Erwin S. Andreasen <erwin@pip.dknet.dk> */
ACMD(do_copyover) {
#ifdef CIRCLE_WINDOWS
    ch->sendf("Copyover is not available for Windows.\r\n");
#else
    char arg[MAX_INPUT_LENGTH];
    int secs;

    one_argument(argument, arg);
    if (!*arg) {
        execute_copyover();
    } else if (is_abbrev(arg, "cancel") || is_abbrev(arg, "stop")) {
        if (!copyover_timer) {
            ch->sendf("A timed copyover has not been started!\r\n");
        } else {
            copyover_timer = 0;
            game_info("Copyover cancelled");
        }
    } else if (is_abbrev(arg, "help")) {
        ch->sendf("COPYOVER\r\n\r\n");
        ch->sendf("Usage: @ycopyover@n           - Perform an immediate copyover\r\n"
                         "       @ycopyover <seconds>@n - Start a timed copyover\r\n"
                         "       @ycopyover cancel@n    - Stop a timed copyover\r\n\r\n");
        ch->sendf("A timed copyover will produce an automatic warning when it starts, and then\r\n");
        ch->sendf("every minute.  During the last minute, there will be a warning every 15 seconds.\r\n");
    } else {
        secs = atoi(arg);
        if (!secs || secs < 0) {
            ch->sendf("Type @ycopyover help@n for usage info.");
        } else {
            copyover_timer = secs;
            basic_mud_log("-- Timed Copyover started by %s - %d seconds until copyover --", GET_NAME(ch), secs);
            if (secs >= 60) {
                if (secs % 60) {
                    game_info("A copyover will be performed in %d minutes and %d seconds.", (copyover_timer / 60),
                              (copyover_timer % 60));
                } else {
                    game_info("A copyover will be performed in %d minute%s.", (copyover_timer / 60),
                              (copyover_timer / 60) > 1 ? "s" : "");
                }
            } else {
                game_info("A copyover will be performed in %d seconds.", copyover_timer);
            }
        }
    }
#endif
}

static void execute_copyover() {
    circle_shutdown = 1;
    circle_reboot = 1;
}

void copyover_check(uint64_t heartPulse, double deltaTime) {
    if (!copyover_timer) return;
    copyover_timer--;
    if (!copyover_timer) {
        execute_copyover();
    }
    if (copyover_timer > 59) {
        if (copyover_timer % 60 == 0) {
            game_info("A copyover will be performed in %d minute%s.", (copyover_timer / 60),
                      (copyover_timer / 60) > 1 ? "s" : "");
        }
    } else {
        if (copyover_timer % 10 == 0 && copyover_timer > 29) {
            game_info("A copyover will be performed in %d seconds.", (copyover_timer));
        }
        if (copyover_timer % 5 == 0 && copyover_timer <= 29) {
            game_info("A copyover will be performed in %d seconds.", (copyover_timer));
        }
    }
}

ACMD(do_advance) {
    BaseCharacter *victim;
    char name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
    int newlevel, oldlevel;

    two_arguments(argument, name, level);

    if (*name) {
        if (!(victim = get_char_vis(ch, name, nullptr, FIND_CHAR_WORLD))) {
            ch->sendf("That player is not here.\r\n");
            return;
        }
    } else {
        ch->sendf("Advance who?\r\n");
        return;
    }

    if (IS_NPC(victim)) {
        ch->sendf("NO!  Not on NPC's.\r\n");
        return;
    }
    if (!*level) {
        ch->sendf("[ 1 - 100 | demote ]\r\n");
        return;
    } else if ((newlevel = atoi(level)) <= 0) {
        if (!strcasecmp("demote", level)) {
            victim->set(CharNum::Level, 1);
            victim->set(CharStat::PowerLevel, 150);
            victim->set(CharStat::Ki, 150);
            victim->set(CharStat::Stamina, 150);
            ch->sendf("They have now been demoted!\r\n");
            victim->sendf("You were demoted to level 1!\r\n");
            return;
        } else {
            ch->sendf("That's not a level!\r\n");
            return;
        }
    }
    if (newlevel > 100) {
        ch->sendf("100 is the highest possible level.\r\n");
        return;
    }
    if (newlevel == GET_LEVEL(victim)) {
        ch->sendf("They are already at that level.\r\n");
        return;
    }
    oldlevel = GET_LEVEL(victim);
    if (newlevel < GET_LEVEL(victim)) {
        ch->sendf("You cannot demote a player.\r\n");
    } else {
        act("$n makes some strange gestures.\r\n"
            "A strange feeling comes upon you, like a giant hand, light comes down\r\n"
            "from above, grabbing your body, which begins to pulse with colored\r\n"
            "lights from inside.\r\n\r\n"
            "Your head seems to be filled with demons from another plane as your\r\n"
            "body dissolves to the elements of time and space itself.\r\n\r\n"
            "Suddenly a silent explosion of light snaps you back to reality.\r\n\r\n"
            "You feel slightly different.", false, ch, nullptr, victim, TO_VICT);
    }

    ch->sendf("%s", CONFIG_OK);

    if (newlevel < oldlevel)
        basic_mud_log("(GC) %s demoted %s from level %d to %d.",
            GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
    else
        basic_mud_log("(GC) %s has advanced %s to level %d (from %d)",
            GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

    int gain = level_exp(victim, newlevel) - GET_EXP(victim);
    victim->modExperience(gain);
}

ACMD(do_handout) {
    struct descriptor_data *j;

    if (GET_ADMLEVEL(ch) < 3) {
        ch->sendf("You can't do that.\r\n");
        return;
    }

    for (j = descriptor_list; j; j = j->next) {
        if (!IS_PLAYING(j) || ch == j->character || GET_ADMLEVEL(j->character) > 0)
            continue;
        if (IS_NPC(j->character))
            continue;
        else {
            j->character->modPractices(10);
        }
    }

    send_to_all("@g%s@G hands out 10 practice sessions to everyone!@n\r\n", GET_NAME(ch));
    basic_mud_log("%s gave a handout of 10 PS to everyone.", GET_NAME(ch));
    log_imm_action("HANDOUT: %s has handed out 10 PS to everyone.", GET_NAME(ch));
}

ACMD(do_restore) {
    char buf[MAX_INPUT_LENGTH];
    BaseCharacter *vict;
    struct descriptor_data *j;
    int i;

    one_argument(argument, buf);
    if (!*buf)
        ch->sendf("Whom do you wish to restore?\r\n");
    else if (is_abbrev(buf, "all")) {
        send_to_imm("[Log: %s restored all.]", GET_NAME(ch));
        log_imm_action("RESTORE: %s has restored all players.", GET_NAME(ch));
        for (j = descriptor_list; j; j = j->next) {
            if (!IS_PLAYING(j) || !(vict = j->character))
                continue;
            vict->restore_by(ch);
        }
        ch->sendf("Okay.\r\n");
    } else if (!(vict = get_char_vis(ch, buf, nullptr, FIND_CHAR_WORLD)))
        ch->sendf("%s", CONFIG_NOPERSON);
    else if (!IS_NPC(vict) && ch != vict && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch))
        ch->sendf("They don't need your help.\r\n");
    else {
        vict->restore_by(ch);
        ch->sendf("%s", CONFIG_OK);
        send_to_imm("[Log: %s restored %s.]", GET_NAME(ch), GET_NAME(vict));
        log_imm_action("RESTORE: %s has restored %s.", GET_NAME(ch), GET_NAME(vict));
    }
}

void perform_immort_vis(BaseCharacter *ch) {
    GET_INVIS_LEV(ch) = 0;
}

static void perform_immort_invis(BaseCharacter *ch, int level) {
    BaseCharacter *tch;

    for (auto tch : ch->getRoom()->getPeople()) {
        if (tch == ch)
            continue;
        if (GET_ADMLEVEL(tch) >= GET_INVIS_LEV(ch) && GET_ADMLEVEL(tch) < level)
            act("You blink and suddenly realize that $n is gone.", false, ch, nullptr,
                tch, TO_VICT);
        if (GET_ADMLEVEL(tch) < GET_INVIS_LEV(ch) && GET_ADMLEVEL(tch) >= level)
            act("You suddenly realize that $n is standing beside you.", false, ch, nullptr,
                tch, TO_VICT);
    }

    GET_INVIS_LEV(ch) = level;
    ch->sendf("Your invisibility level is %d.\r\n", level);
}

ACMD(do_invis) {
    char arg[MAX_INPUT_LENGTH];
    int level;

    if (IS_NPC(ch)) {
        ch->sendf("You can't do that!\r\n");
        return;
    }

    one_argument(argument, arg);
    if (!*arg) {
        if (GET_INVIS_LEV(ch) > 0)
            perform_immort_vis(ch);
        else
            perform_immort_invis(ch, GET_ADMLEVEL(ch));
    } else {
        level = atoi(arg);
        if (level > GET_ADMLEVEL(ch))
            ch->sendf("You can't go invisible above your own level.\r\n");
        else if (level < 1)
            perform_immort_vis(ch);
        else
            perform_immort_invis(ch, level);
    }
}

ACMD(do_gecho) {
    struct descriptor_data *pt;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument)
        ch->sendf("That must be a mistake...\r\n");
    else {
        for (pt = descriptor_list; pt; pt = pt->next)
            if (IS_PLAYING(pt) && pt->character && pt->character != ch)
                pt->character->sendf("%s\r\n", argument);

        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
            ch->sendf("%s", CONFIG_OK);
        else
            ch->sendf("%s\r\n", argument);
    }
}

ACMD(do_ginfo) {
    struct descriptor_data *pt;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument)
        ch->sendf("That must be a mistake...\r\n");
    else {
        for (pt = descriptor_list; pt; pt = pt->next)
            if (IS_PLAYING(pt) && pt->character && pt->character != ch)
                pt->character->sendf("@D[@GINFO@D] @g%s@n\r\n", argument);

        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
            ch->sendf("%s", CONFIG_OK);
        else
            ch->sendf("@D[@GINFO@D] @g%s@n\r\n", argument);
    }
}

ACMD(do_poofset) {
    char **msg;

    switch (subcmd) {
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

    ch->sendf("%s", CONFIG_OK);
}

ACMD(do_dc) {
    ch->sendf("temporarily disabled.");
    return;
}

ACMD(do_wizlock) {
    char arg[MAX_INPUT_LENGTH];
    int value;
    const char *when;

    one_argument(argument, arg);
    if (*arg) {
        value = atoi(arg);
        if (value < 0 || value > 101) {
            ch->sendf("Invalid wizlock value.\r\n");
            return;
        }
        circle_restrict = value;
        when = "now";
    } else
        when = "currently";

    if (*arg) {
        switch (circle_restrict) {
            case 0:
                ch->sendf("The game is %s completely open.\r\n", when);
                send_to_all("@RWIZLOCK@D: @WThe game has been completely opened by @C%s@W.@n", GET_NAME(ch));
                basic_mud_log("WIZLOCK: The game has been completely opened by %s.", GET_NAME(ch));
                break;
            case 1:
                ch->sendf("The game is %s closed to new players.\r\n", when);
                send_to_all("@RWIZLOCK@D: @WThe game is %s closed to new players by @C%s@W.@n", when, GET_NAME(ch));
                basic_mud_log("WIZLOCK: The game is %s closed to new players by %s.", when, GET_NAME(ch));
                break;
            case 101:
                ch->sendf("The game is %s closed to non-imms.\r\n", when);
                send_to_all("@RWIZLOCK@D: @WThe game is %s closed to non-imms by @C%s@W.@n", when, GET_NAME(ch));
                basic_mud_log("WIZLOCK: The game is %s closed to non-imms by %s.", when, GET_NAME(ch));
                break;
            default:
                ch->sendf("Only level %d+ may enter the game %s.\r\n", circle_restrict, when);
                send_to_all("@RWIZLOCK@D: @WLevel %d+ only can enter the game %s, thanks to @C%s@W.@n", circle_restrict,
                            when, GET_NAME(ch));
                basic_mud_log("WIZLOCK: Level %d+ only can enter the game %s, thanks to %s.", circle_restrict, when,
                    GET_NAME(ch));
                break;
        }
    }
    if (!*arg) {
        switch (circle_restrict) {
            case 0:
                ch->sendf("The game is %s completely open.\r\n", when);
                break;
            case 1:
                ch->sendf("The game is %s closed to new players.\r\n", when);
                break;
            default:
                ch->sendf("Only level %d and above may enter the game %s.\r\n", circle_restrict, when);
                break;
        }
    }
}

ACMD(do_date) {
    char *tmstr;
    time_t mytime;
    int d, h, m;

    if (subcmd == SCMD_DATE)
        mytime = time(nullptr);
    else
        mytime = boot_time;

    tmstr = (char *) asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    if (subcmd == SCMD_DATE)
        ch->sendf("Current machine time: %s\r\n", tmstr);
    else {
        mytime = time(nullptr) - boot_time;
        d = mytime / 86400;
        h = (mytime / 3600) % 24;
        m = (mytime / 60) % 60;

        ch->sendf("Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, d == 1 ? "" : "s", h, m);
    }
}

ACMD(do_last) {
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (!*arg) {
        ch->sendf("For whom do you wish to search?\r\n");
        return;
    }
    auto vict = findPlayer(arg);

    if (!vict) {
        ch->sendf("There is no such player.\r\n");
        return;
    }
    if ((GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch)) && (GET_ADMLEVEL(ch) < ADMLVL_IMPL)) {
        ch->sendf("You are not sufficiently godly for that!\r\n");
        return;
    }

    ch->sendf("[%5d] [%2d %s %s] %-12s : %-18s : %-20s\r\n",
                 GET_IDNUM(vict), (int) GET_LEVEL(vict),
                 race::getAbbr(vict->race).c_str(), CLASS_ABBR(vict),
                 GET_NAME(vict), "(FIXHOSTPLZ)",
                 ctime(&vict->time.logon));
}

ACMD(do_force) {
    struct descriptor_data *i, *next_desc;
    BaseCharacter *vict, *next_force;
    char arg[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH + 32];

    half_chop(argument, arg, to_force);

    snprintf(buf1, sizeof(buf1), "$n has forced you to '%s'.", to_force);

    if (!*arg || !*to_force)
        ch->sendf("Whom do you wish to force do what?\r\n");
    else if (!ADM_FLAGGED(ch, ADM_FORCEMASS) || (strcasecmp("all", arg) && strcasecmp("room", arg))) {
        if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
            ch->sendf("%s", CONFIG_NOPERSON);
        else if (!IS_NPC(vict) && GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict))
            ch->sendf("No, no, no!\r\n");
        else {
            ch->sendf("%s", CONFIG_OK);
            act(buf1, true, ch, nullptr, vict, TO_VICT);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s forced %s to %s", GET_NAME(ch),
                   GET_NAME(vict), to_force);
            vict->executeCommand(to_force);
        }
    } else if (!strcasecmp("room", arg)) {
        ch->sendf("%s", CONFIG_OK);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s forced room %d to %s",
               GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);

        for (auto vict : ch->getRoom()->getPeople()) {
            if (!IS_NPC(vict) && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch))
                continue;
            act(buf1, true, ch, nullptr, vict, TO_VICT);
            vict->executeCommand(to_force);
        }
    } else { /* force all */
        ch->sendf("%s", CONFIG_OK);
        mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s forced all to %s", GET_NAME(ch), to_force);

        for (i = descriptor_list; i; i = next_desc) {
            next_desc = i->next;

            if (STATE(i) != CON_PLAYING || !(vict = i->character) ||
                (!IS_NPC(vict) && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch)))
                continue;
            act(buf1, true, ch, nullptr, vict, TO_VICT);
            vict->executeCommand(to_force);
        }
    }
}

ACMD(do_wiznet) {
    char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
            buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32], *msg;
    struct descriptor_data *d;
    char emote = false;
    char any = false;
    int level = ADMLVL_IMMORT;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument) {
        ch->sendf(
                     "Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n        wiznet @<level> *<emotetext> | wiz @\r\n");
        return;
    }
    switch (*argument) {
        case '*':
            emote = true;
        case '#':
            one_argument(argument + 1, buf1);
            if (is_number(buf1)) {
                half_chop(argument + 1, buf1, argument);
                level = MAX(atoi(buf1), ADMLVL_IMMORT);
                if (level > GET_ADMLEVEL(ch)) {
                    ch->sendf("You can't wizline above your own level.\r\n");
                    return;
                }
            } else if (emote)
                argument++;
            break;

        case '@':
            ch->sendf("God channel status:\r\n");
            for (any = 0, d = descriptor_list; d; d = d->next) {
                if (STATE(d) != CON_PLAYING || GET_ADMLEVEL(d->character) < ADMLVL_IMMORT)
                    continue;
                if (!CAN_SEE(ch, d->character))
                    continue;

                ch->sendf("  %-*s%s%s%s\r\n", MAX_NAME_LENGTH, GET_NAME(d->character),
                             PLR_FLAGGED(d->character, PLR_WRITING) ? " (Writing)" : "",
                             PLR_FLAGGED(d->character, PLR_MAILING) ? " (Writing mail)" : "",
                             PRF_FLAGGED(d->character, PRF_NOWIZ) ? " (Offline)" : "");
            }
            return;

        case '\\':
            ++argument;
            break;
        default:
            break;
    }
    if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
        ch->sendf("You are offline!\r\n");
        return;
    }
    skip_spaces(&argument);

    if (!*argument) {
        ch->sendf("Don't bother the gods like that!\r\n");
        return;
    }
    if (level > ADMLVL_IMMORT) {
        snprintf(buf1, sizeof(buf1), "@c%s@D: <@C%d@D> @G%s%s@n\r\n", GET_NAME(ch), level, emote ? "<--- " : "",
                 argument);
        snprintf(buf2, sizeof(buf1), "@cSomeone@D: <@C%d@D> @G%s%s@n\r\n", level, emote ? "<--- " : "", argument);
    } else {
        snprintf(buf1, sizeof(buf1), "@c%s@D: @G%s%s@n\r\n", GET_NAME(ch), emote ? "<--- " : "", argument);
        snprintf(buf2, sizeof(buf1), "@cSomeone@D: @G%s%s@n\r\n", emote ? "<--- " : "", argument);
    }

    for (d = descriptor_list; d; d = d->next) {
        if (IS_PLAYING(d) && (GET_ADMLEVEL(d->character) >= level) &&
            (!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
            (!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING))
            && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
            if (CAN_SEE(d->character, ch)) {
                msg = strdup(buf1);
                d->character->sendf("%s", buf1);
            } else {
                msg = strdup(buf2);
                d->character->sendf("%s", buf2);
            }
            add_history(d->character, msg, HIST_WIZNET);
        }
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        ch->sendf("%s", CONFIG_OK);
}

ACMD(do_zreset) {
    char arg[MAX_INPUT_LENGTH];
    zone_rnum i;
    zone_vnum j;

    one_argument(argument, arg);

    if (*arg == '*') {
        if (GET_ADMLEVEL(ch) < ADMLVL_VICE) {
            ch->sendf("You do not have permission to reset the entire world.\r\n");
            return;
        } else {
            for (auto &z : zone_table) {
                if (z.first < 200) {
                    reset_zone(z.first);
                }
            }
            ch->sendf("Reset world.\r\n");
            mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true, "(GC) %s reset all MUD zones.", GET_NAME(ch));
            log_imm_action("RESET: %s has reset all MUD zones.", GET_NAME(ch));
            return;
        }
    } else if (*arg == '.' || !*arg)
        i = real_zone_by_thing(ch->getRoom()->zone);
    else {
        i = atol(arg);
    }
    if (!zone_table.count(i) || !(can_edit_zone(ch, i) || GET_ADMLEVEL(ch) > ADMLVL_IMMORT)) {
        ch->sendf("You do not have permission to reset this zone. Try %d.\r\n", GET_OLC_ZONE(ch));
        return;
    }
    auto &z = zone_table[i];
    reset_zone(z.number);
    ch->sendf("Reset zone #%d: %s.\r\n", z.number, z.name);
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), true, "(GC) %s reset zone %d (%s)", GET_NAME(ch),
           z.number, z.name);
    log_imm_action("RESET: %s has reset zone #%d: %s.", GET_NAME(ch), z.number, z.name);
}

/*
 *  General fn for wizcommands of the sort: cmd <player>
 */
ACMD(do_wizutil) {
    char arg[MAX_INPUT_LENGTH];
    BaseCharacter *vict;
    int taeller;
    long result;

    one_argument(argument, arg);

    if (!*arg)
        ch->sendf("Yes, but for whom?!?\r\n");
    else if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
        ch->sendf("There is no such player.\r\n");
    else if (IS_NPC(vict))
        ch->sendf("You can't do that to a mob!\r\n");
    else if (GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch))
        ch->sendf("Hmmm...you'd better not.\r\n");
    else {
        switch (subcmd) {
            case SCMD_REROLL:
                ch->sendf("Rerolling is not possible at this time, bug Iovan about it...\r\n");
                basic_mud_log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
                ch->sendf("New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
                             GET_STR(vict), GET_INT(vict), GET_WIS(vict),
                             GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
                break;
            case SCMD_PARDON:
                if (!PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(vict, PLR_KILLER)) {
                    ch->sendf("Your victim is not flagged.\r\n");
                    return;
                }
            for(auto f : {PLR_THIEF, PLR_KILLER}) vict->clearFlag(FlagType::PC, f);
                ch->sendf("Pardoned.\r\n");
                vict->sendf("You have been pardoned by the Gods!\r\n");
                mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s pardoned by %s", GET_NAME(vict),
                       GET_NAME(ch));
                break;
            case SCMD_NOTITLE:
                result = vict->flipFlag(FlagType::PC, PLR_NOTITLE);
                mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) Notitle %s for %s by %s.",
                       ONOFF(result), GET_NAME(vict), GET_NAME(ch));
                ch->sendf("(GC) Notitle %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
                break;
            case SCMD_SQUELCH:
                result = vict->flipFlag(FlagType::PC, PLR_NOSHOUT);
                mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) Squelch %s for %s by %s.",
                       ONOFF(result), GET_NAME(vict), GET_NAME(ch));
                ch->sendf("(GC) Mute turned %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
                send_to_all("@D[@RMUTE@D] @C%s@W has had mute turned @r%s@W by @C%s@W.\r\n", GET_NAME(vict),
                            ONOFF(result), GET_NAME(ch));
                break;
            case SCMD_FREEZE:
                if (ch == vict) {
                    ch->sendf("Oh, yeah, THAT'S real smart...\r\n");
                    return;
                }
                if (GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict)) {
                    ch->sendf("Pfft...\r\n");
                    return;
                }
                if (PLR_FLAGGED(vict, PLR_FROZEN)) {
                    ch->sendf("Your victim is already pretty cold.\r\n");
                    return;
                }
                vict->setFlag(FlagType::PC, PLR_FROZEN);
                GET_FREEZE_LEV(vict) = GET_ADMLEVEL(ch);
                vict->sendf(
                             "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
                ch->sendf("Frozen.\r\n");
                act("A sudden cold wind conjured from nowhere freezes $n!", false, vict, nullptr, nullptr, TO_ROOM);
                mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s frozen by %s.", GET_NAME(vict),
                       GET_NAME(ch));
                break;
            case SCMD_THAW:
                if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
                    ch->sendf("Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
                    return;
                }
                if (GET_FREEZE_LEV(vict) > GET_ADMLEVEL(ch)) {
                    ch->sendf("Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
                                 GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
                    return;
                }
                mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s un-frozen by %s.", GET_NAME(vict),
                       GET_NAME(ch));
                vict->clearFlag(FlagType::PC, PLR_FROZEN);
                vict->sendf(
                             "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
                ch->sendf("Thawed.\r\n");
                act("A sudden fireball conjured from nowhere thaws $n!", false, vict, nullptr, nullptr, TO_ROOM);
                break;
            case SCMD_UNAFFECT:
                ch->sendf("Disabled.\r\n");
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
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall) {
    size_t tmp;
    auto &z = zone_table[zone];
    if (listall) {

        tmp = snprintf(bufptr, left,
                       "%3d %-30.30s By: %-10.10s Age: %3d; Reset: %3d (%1d); Range: %5d-%5d\r\n",
                       z.number, z.name, z.builders,
                       z.age, z.lifespan,
                       z.reset_mode,
                       z.bot, z.top);

        auto j = z.rooms.size();
        auto k = z.objects.size();
        auto l = z.mobiles.size();
        auto m = z.shops.size();
        auto n = z.triggers.size();
        auto o = z.guilds.size();

        tmp += snprintf(bufptr + tmp, left - tmp,
                        "       Zone stats:\r\n"
                        "       ---------------\r\n"
                        "         Rooms:    %2d\r\n"
                        "         Objects:  %2d\r\n"
                        "         Mobiles:  %2d\r\n"
                        "         Shops:    %2d\r\n"
                        "         Triggers: %2d\r\n"
                        "         Guilds:   %2d\r\n",
                        j, k, l, m, n, o);

        return tmp;
    }

    return snprintf(bufptr, left,
                    "%3d %-*s By: %-10.10s Range: %5d-%5d\r\n", z.number,
                    count_color_chars(z.name) + 30, z.name,
                    z.builders, z.bot, z.top);
}

ACMD(do_show) {
    int i, j, k, l, con;                /* i, j, k to specifics? */
    size_t len, nlen;
    zone_rnum zrn;
    zone_vnum zvn;
    int low, high;
    int8_t self = false;
    BaseCharacter *vict = nullptr;
    Object *obj;
    struct descriptor_data *d;
    struct affected_type *aff;
    char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], *strp,
            arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];

    struct show_struct {
        const char *cmd;
        const char level;
    } fields[] = {
            {"nothing", 0},                /* 0 */
            {"zones",      ADMLVL_IMMORT},        /* 1 */
            {"player",     ADMLVL_GOD},
            {"rent",       ADMLVL_GRGOD},
            {"stats",      ADMLVL_IMMORT},
            {"errors",     ADMLVL_IMPL},            /* 5 */
            {"death",      ADMLVL_GOD},
            {"godrooms",   ADMLVL_IMMORT},
            {"shops",      ADMLVL_IMMORT},
            {"houses",     ADMLVL_GOD},
            {"snoop",      ADMLVL_GRGOD},            /* 10 */
            {"assemblies", ADMLVL_IMMORT},
            {"guilds",     ADMLVL_GOD},
            {"levels",     ADMLVL_GRGOD},
            {"uniques",    ADMLVL_GRGOD},
            {"affect",     ADMLVL_GRGOD},            /* 15 */
            {"affectv",    ADMLVL_GRGOD},
            {"\n",      0}
    };

    skip_spaces(&argument);

    if (!*argument) {
        ch->sendf("Game Info options:\r\n");
        for (j = 0, i = 1; fields[i].level; i++)
            if (fields[i].level <= GET_ADMLEVEL(ch))
                ch->sendf("%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
        ch->sendf("\r\n");
        return;
    }

    strcpy(arg, two_arguments(argument, field, value));    /* strcpy: OK (argument <= MAX_INPUT_LENGTH == arg) */

    for (l = 0; *(fields[l].cmd) != '\n'; l++)
        if (!strncmp(field, fields[l].cmd, strlen(field)))
            break;

    if (GET_ADMLEVEL(ch) < fields[l].level) {
        ch->sendf("You are not godly enough for that!\r\n");
        return;
    }
    if (!strcmp(value, "."))
        self = true;
    buf[0] = '\0';

    switch (l) {
        /* show zone */
        case 1:
            /* tightened up by JE 4/6/93 */
            if (self)
                print_zone_to_buf(buf, sizeof(buf), ch->getRoom()->zone, 1);
            else if (*value && is_number(value)) {
                zvn = real_zone(atoi(value));
                if (zvn == NOBODY) {
                    ch->sendf("That is not a valid zone.\r\n");
                    return;
                }
                print_zone_to_buf(buf, sizeof(buf), zrn, 1);
            } else {
                len = 0;
                for (auto &z : zone_table) {
                    nlen = print_zone_to_buf(buf + len, sizeof(buf) - len, z.first, 0);
                    if (len + nlen >= sizeof(buf) || nlen < 0)
                        break;
                    len += nlen;
                }
            }
            write_to_output(ch->desc, buf);
            break;

            /* show player */
        case 2:
            if (!*value) {
                ch->sendf("A name would help.\r\n");
                return;
            }
            vict = findPlayer(value);
            if (!vict) {
                ch->sendf("There is no such player.\r\n");
                free_char(vict);
                return;
            }
            ch->sendf("Player: %-12s (%s) [%2d %s %s]\r\n", GET_NAME(vict),
                         genders[(int) GET_SEX(vict)], GET_LEVEL(vict), CLASS_ABBR(vict),
                         race::getAbbr(vict->race).c_str());
            ch->sendf("Au: %-8d  Bal: %-8d  Exp: %" I64T "  Align: %-5d  Ethic: %-5d\r\n",
                         GET_GOLD(vict), GET_BANK_GOLD(vict), GET_EXP(vict),
                         GET_ALIGNMENT(vict), GET_ETHIC_ALIGNMENT(vict));

            /* ctime() uses static buffer: do not combine. */
            ch->sendf("Started: %-20.16s  ", ctime(&vict->time.created));
            ch->sendf("Last: %-20.16s  Played: %3dh %2dm\r\n",
                         ctime(&vict->time.logon),
                         (int) ((int64_t)vict->time.played / 3600),
                         (int) ((int64_t)vict->time.played / 60 % 60));
            break;

            /* show rent */
        case 3:
            if (!*value) {
                ch->sendf("A name would help.\r\n");
                return;
            }
            break;

            /* show stats */
        case 4:
            i = 0;
            j = 0;
            k = 0;
            con = 0;
            for (vict = character_list; vict; vict = vict->next) {
                if (IS_NPC(vict))
                    j++;
                else if (CAN_SEE(ch, vict)) {
                    i++;
                    if (vict->desc)
                        con++;
                }
            }
            for (obj = object_list; obj; obj = obj->next)
                k++;
            ch->sendf(
                         "             @D---   @CCore Stats   @D---\r\n"
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
                         i, con,
                         players.size(),
                         j, mob_proto.size(),
                         k, obj_proto.size(),
                         entities.size(), zone_table.size(),
                         trig_index.size(),
                         buf_largecount,
                         buf_switches, buf_overflows,
                         add_commas(mob_specials_used).c_str(),
                         add_commas(number_of_assassins).c_str(),
                         SELFISHMETER
            );
            break;

            /* show errors */
        case 5:
            len = strlcpy(buf, "Errant Rooms\r\n------------\r\n", sizeof(buf));
            for (auto &[vn, u] : entities) {
                auto r = reg.try_get<Room>(u);
                if(!r) continue;
                for (auto &[j, e] : r->getExits()) {
                    auto dest = e->getDestination();
                    if(!dest) {
                        if(auto gen = e->getLookDesc(); !gen.empty()) {
                            nlen = snprintf(buf + len, sizeof(buf) - len, "[%5d] %s: %s\r\n", vn, r->getDisplayName(ch), gen.c_str());
                            if (len + nlen >= sizeof(buf) || nlen < 0)
                                break;
                            len += nlen;
                        }
                    }
                    else {
                        if(dest->getUID() == 0) {
                            nlen = snprintf(buf + len, sizeof(buf) - len, "[%5d] %s: %s\r\n", vn, r->getDisplayName(ch), e->getLookDesc().c_str());
                            if (len + nlen >= sizeof(buf) || nlen < 0)
                                break;
                            len += nlen;
                        }
                    }
                    if (dest && dest->getUID() == 0) {
                        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (void   ) [%5d] %-*s%s (%s)\r\n", ++k,
                                        vn, count_color_chars((char*)r->getDisplayName(ch).c_str()) + 40, r->getDisplayName(ch).c_str(), QNRM,
                                        dirs[j]);
                        if (len + nlen >= sizeof(buf) || nlen < 0)
                            break;
                        len += nlen;
                    }
                }
            }

            write_to_output(ch->desc, buf);
            break;

            /* show death */
        case 6:
            j = 0;
            len = strlcpy(buf, "Death Traps\r\n-----------\r\n", sizeof(buf));
            for (auto &[vn, u] : entities) {
                auto r = reg.try_get<Room>(u);
                if(!r) continue;
                if (ROOM_FLAGGED(vn, ROOM_DEATH)) {
                    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, vn,
                                    r->getDisplayName(ch).c_str());
                    if (len + nlen >= sizeof(buf) || nlen < 0)
                        break;
                    len += nlen;
                }
            }
            write_to_output(ch->desc, buf);
            break;

            /* show godrooms */
        case 7:
            j = 0;
            len = strlcpy(buf, "Godrooms\r\n--------------------------\r\n", sizeof(buf));
            for (auto &[vn, u] : entities) {
                auto r = reg.try_get<Room>(u);
                if(!r) continue;
                if (ROOM_FLAGGED(r, ROOM_GODROOM)) {
                    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, vn,
                                    r->getDisplayName(ch).c_str());
                    if (len + nlen >= sizeof(buf) || nlen < 0)
                        break;
                    len += nlen;
                }
            }
                
            write_to_output(ch->desc, buf);
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
            ch->sendf("People currently snooping:\r\n--------------------------\r\n");
            for (d = descriptor_list; d; d = d->next) {
                if (d->snooping == nullptr || d->character == nullptr)
                    continue;
                if (STATE(d) != CON_PLAYING || GET_ADMLEVEL(ch) < GET_ADMLEVEL(d->character))
                    continue;
                if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
                    continue;
                i++;
                ch->sendf("%-10s - snooped by %s.\r\n", GET_NAME(d->snooping->character),
                             GET_NAME(d->character));
            }
            if (i == 0)
                ch->sendf("No one is currently snooping.\r\n");
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
            ch->sendf("This is not used currently.\r\n");
            break;

        case 14:
            if (value != nullptr && *value) {
                if (sscanf(value, "%d-%d", &low, &high) != 2) {
                    if (sscanf(value, "%d", &low) != 1) {
                        ch->sendf("Usage: show uniques, show uniques [vnum], or show uniques [low-high]\r\n");
                        return;
                    } else {
                        high = low;
                    }
                }
            } else {
                low = -1;
                high = 9999999;
            }
            strp = sprintuniques(low, high);
            write_to_output(ch->desc, strp);
            free(strp);
            break;

        case 15:
        case 16:
            if (!*value) {
                low = 1;
                vict = (l == 15) ? affect_list : affectv_list;
            } else {
                low = 0;
                if (!(vict = get_char_world_vis(ch, value, nullptr))) {
                    ch->sendf("Cannot find that character.\r\n");
                    return;
                }
            }
            k = MAX_STRING_LENGTH;
            CREATE(strp, char, k);
            strp[0] = j = 0;
            if (!vict) {
                ch->sendf("None.\r\n");
                return;
            }
            do {
                if ((k - j) < (MAX_INPUT_LENGTH * 8)) {
                    k *= 2;
                    RECREATE(strp, char, k);
                }
                j += snprintf(strp + j, k - j, "Name: %s\r\n", GET_NAME(vict));
                if (l == 15)
                    aff = vict->affected;
                else
                    aff = vict->affectedv;
                for (; aff; aff = aff->next) {
                    j += snprintf(strp + j, k - j, "SPL: (%3d%s) @c%-21s@n ", aff->duration + 1,
                                  (l == 15) ? "hr" : "rd", skill_name(aff->type));

                    if (aff->modifier)
                        j += snprintf(strp + j, k - j, "%+d to %s", aff->modifier,
                                      apply_types[(int) aff->location]);

                    if (aff->bitvector) {
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
            write_to_output(ch->desc, strp);
            free(strp);
            break;

            /* show what? */
        default:
            ch->sendf("Sorry, I don't understand that.\r\n");
            break;
    }
}

/***************** The do_set function ***********************************/

constexpr int PC = 1;
constexpr int NPC = 2;
constexpr int BOTH = 3;

#define MISC    0
#define BINARY    1
#define NUMBER    2

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))


/* The set options available */
static const struct set_struct {
    const char *cmd;
    const char level;
    const char pcnpc;
    const char type;
} set_fields[] = {
        {"brief",      ADMLVL_GOD,     PC,   BINARY},  /* 0 */
        {"invstart",   ADMLVL_GOD,     PC,   BINARY},  /* 1 */
        {"title",      ADMLVL_GOD,     PC,   MISC},
        {"nosummon",   ADMLVL_GRGOD,   PC,   BINARY},
        {"maxpl",      ADMLVL_BUILDER, BOTH, NUMBER},
        {"maxki",      ADMLVL_BUILDER, BOTH, NUMBER},  /* 5 */
        {"maxst",      ADMLVL_BUILDER, BOTH, NUMBER},
        {"pl",         ADMLVL_BUILDER, BOTH, NUMBER},
        {"ki",         ADMLVL_BUILDER, BOTH, NUMBER},
        {"sta",        ADMLVL_BUILDER, BOTH, NUMBER},
        {"align",      ADMLVL_BUILDER, BOTH, NUMBER},  /* 10 */
        {"str",        ADMLVL_BUILDER, BOTH, NUMBER},
        {"stradd",     ADMLVL_IMPL,    BOTH, NUMBER},
        {"int",        ADMLVL_BUILDER, BOTH, NUMBER},
        {"wis",        ADMLVL_BUILDER, BOTH, NUMBER},
        {"dex",        ADMLVL_BUILDER, BOTH, NUMBER},  /* 15 */
        {"con",        ADMLVL_BUILDER, BOTH, NUMBER},
        {"cha",        ADMLVL_BUILDER, BOTH, NUMBER},
        {"ac",         ADMLVL_IMPL,    BOTH, NUMBER},
        {"zenni",      ADMLVL_BUILDER, BOTH, NUMBER},
        {"bank",       ADMLVL_BUILDER, PC,   NUMBER},  /* 20 */
        {"exp",        ADMLVL_BUILDER, BOTH, NUMBER},
        {"accuracy",   ADMLVL_VICE,    BOTH, NUMBER},
        {"damage",     ADMLVL_VICE,    BOTH, NUMBER},
        {"invis",      ADMLVL_IMPL,    PC,   NUMBER},
        {"nohassle",   ADMLVL_VICE,    PC,   BINARY},  /* 25 */
        {"frozen",     ADMLVL_FREEZE,  PC,   BINARY},
        {"practices",  ADMLVL_BUILDER, PC,   NUMBER},
        {"lessons",    ADMLVL_IMPL,    PC,   NUMBER},
        {"drunk",      ADMLVL_GOD,     BOTH, MISC},
        {"hunger",     ADMLVL_BUILDER, BOTH, MISC},    /* 30 */
        {"thirst",     ADMLVL_BUILDER, BOTH, MISC},
        {"killer",     ADMLVL_GOD,     PC,   BINARY},
        {"thief",      ADMLVL_GOD,     PC,   BINARY},
        {"level",      ADMLVL_GRGOD,   BOTH, NUMBER},
        {"room",       ADMLVL_IMPL,    BOTH, NUMBER},  /* 35 */
        {"roomflag",   ADMLVL_VICE,    PC,   BINARY},
        {"siteok",     ADMLVL_VICE,    PC,   BINARY},
        {"deleted",    ADMLVL_IMPL,    PC,   BINARY},
        {"class",      ADMLVL_VICE,    BOTH, MISC},
        {"nowizlist",  ADMLVL_GOD,     PC,   BINARY},  /* 40 */
        {"quest",      ADMLVL_GOD,     PC,   BINARY},
        {"loadroom",   ADMLVL_VICE,    PC,   MISC},
        {"color",      ADMLVL_GOD,     PC,   BINARY},
        {"idnum",      ADMLVL_IMPL,    PC,   NUMBER},
        {"passwd",     ADMLVL_IMPL,    PC,   MISC},    /* 45 */
        {"nodelete",   ADMLVL_GOD,     PC,   BINARY},
        {"sex",        ADMLVL_VICE,    BOTH, MISC},
        {"age",        ADMLVL_VICE,    BOTH, NUMBER},
        {"height",     ADMLVL_GOD,     BOTH, NUMBER},
        {"weight",     ADMLVL_GOD,     BOTH, NUMBER},  /* 50 */
        {"olc",        ADMLVL_GRGOD,   PC,   MISC},
        {"race",       ADMLVL_VICE,    PC,   MISC},
        {"trains",     ADMLVL_VICE,    PC,   NUMBER},
        {"feats",      ADMLVL_VICE,    PC,   NUMBER},
        {"ethic",      ADMLVL_GOD,     BOTH, NUMBER},  /* 55 */
        {"unused1",    ADMLVL_GRGOD,   BOTH, NUMBER},
        {"unused2",    ADMLVL_GRGOD,   BOTH, NUMBER},
        {"adminlevel", ADMLVL_GRGOD,   PC,   NUMBER},
        {"hairl",      ADMLVL_VICE,    PC,   NUMBER},
        {"hairs",      ADMLVL_VICE,    PC,   NUMBER},
        {"hairc",      ADMLVL_VICE,    PC,   NUMBER},
        {"skin",       ADMLVL_VICE,    PC,   NUMBER},
        {"eye",        ADMLVL_VICE,    PC,   NUMBER},
        {"basepl",     ADMLVL_BUILDER, BOTH, NUMBER},
        {"baseki",     ADMLVL_BUILDER, BOTH, NUMBER},
        {"basest",     ADMLVL_BUILDER, BOTH, NUMBER},
        {"droom",      ADMLVL_GRGOD,   BOTH, NUMBER},
        {"absorbs",    ADMLVL_GRGOD,   BOTH, NUMBER},
        {"ugp",        ADMLVL_GRGOD,   BOTH, NUMBER},
        {"aura",       ADMLVL_IMMORT,  BOTH, NUMBER},
        {"trp",        ADMLVL_GRGOD,   BOTH, NUMBER},
        {"boost",      ADMLVL_GRGOD,   BOTH, NUMBER},
        {"multi",      ADMLVL_IMPL,    PC,   BINARY},
        {"deaths",     ADMLVL_BUILDER, BOTH, NUMBER},
        {"user",       ADMLVL_IMPL,    PC,   MISC},
        {"phase",      ADMLVL_IMMORT,  PC,   NUMBER},
        {"racial",     ADMLVL_IMMORT,  PC,   NUMBER},
        {"slots",      ADMLVL_IMMORT,  PC,   NUMBER},
        {"feature",    ADMLVL_IMMORT,  PC,   BINARY},
        {"tclass",     ADMLVL_VICE,    PC,   NUMBER},
        {"clones",     ADMLVL_IMPL,    PC,   NUMBER},
        {"armor",      ADMLVL_IMPL,    PC,   NUMBER},
        {"\n", 0,                      BOTH, MISC}
};

static int perform_set(BaseCharacter *ch, BaseCharacter *vict, int mode,
                       char *val_arg) {
    int i, on = 0, off = 0;
    int64_t value = 0;
    room_rnum rnum;
    room_vnum rvnum;

    /* Check to make sure all the levels are correct */
    if (GET_ADMLEVEL(ch) != ADMLVL_IMPL) {
        if (!IS_NPC(vict) && GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict) && vict != ch) {
            ch->sendf("Maybe that's not such a great idea...\r\n");
            return (0);
        }
    }
    if (GET_ADMLEVEL(ch) < set_fields[mode].level) {
        ch->sendf("You are not godly enough for that!\r\n");
        return (0);
    }

    /* Make sure the PC/NPC is correct */
    if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
        ch->sendf("You can't do that to a beast!\r\n");
        return (0);
    } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
        ch->sendf("That can only be done to a beast!\r\n");
        return (0);
    }

    /* Find the value of the argument */
    if (set_fields[mode].type == BINARY) {
        if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
            on = 1;
        else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
            off = 1;
        if (!(on || off)) {
            ch->sendf("Value must be 'on' or 'off'.\r\n");
            return (0);
        }
        ch->sendf("%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), GET_NAME(vict));
    } else if (set_fields[mode].type == NUMBER) {
        char *ptr;

        value = strtoll(val_arg, &ptr, 10);
        /*    value = atoi(val_arg); */
        ch->sendf("%s's %s set to %" I64T ".\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
    } else
        ch->sendf("%s", CONFIG_OK);

    switch (mode) {
        case 0: vict->flipFlag(FlagType::Pref, PRF_BRIEF);
            break;
        case 1: vict->flipFlag(FlagType::PC, PLR_INVSTART);
            break;
        case 2:
            set_title(vict, val_arg);
            ch->sendf("%s's title is now: %s\r\n", GET_NAME(vict), GET_TITLE(vict));
            break;
        case 3: vict->flipFlag(FlagType::Pref, PRF_SUMMONABLE);
            ch->sendf("Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
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
            vict->set(CharAlign::GoodEvil, RANGE(-1000, 1000));
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set align for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set align for %s.", GET_NAME(ch), GET_NAME(vict));
            affect_total(vict);
            break;
        case 11:
            RANGE(0, 100);
            vict->set(CharAttribute::Strength, value);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set str for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set str for %s.", GET_NAME(ch), GET_NAME(vict));
            affect_total(vict);
            break;
        case 12:
            ch->sendf("Setting str_add does nothing now.\r\n");
            /* vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
       break; */
        case 13:
            RANGE(0, 100);
            vict->set(CharAttribute::Intelligence, value);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set intel for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set intel for %s.", GET_NAME(ch), GET_NAME(vict));
            affect_total(vict);
            break;
        case 14:
            RANGE(0, 100);
            vict->set(CharAttribute::Wisdom, value);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set wis for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set wis for %s.", GET_NAME(ch), GET_NAME(vict));
            affect_total(vict);
            break;
        case 15:
            RANGE(0, 100);
            vict->set(CharAttribute::Agility, value);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set dex for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set dex for %s.", GET_NAME(ch), GET_NAME(vict));
            affect_total(vict);
            break;
        case 16:
            RANGE(0, 100);
            vict->set(CharAttribute::Constitution, value);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set con for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set con for %s.", GET_NAME(ch), GET_NAME(vict));
            affect_total(vict);
            break;
        case 17:
            RANGE(0, 100);
            vict->set(CharAttribute::Speed, value);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set speed for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set speed for %s.", GET_NAME(ch), GET_NAME(vict));
            affect_total(vict);
            break;
        case 18:
            vict->armor = RANGE(-100, 500);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set armor index for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set armor index for %s.", GET_NAME(ch), GET_NAME(vict));
            affect_total(vict);
            break;
        case 19:
            vict->set(CharMoney::Carried, RANGE(0, 100000000));
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set zenni for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set zenni for %s.", GET_NAME(ch), GET_NAME(vict));
            break;
        case 20:
            vict->set(CharMoney::Bank, RANGE(0, 100000000));
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set bank for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set bank for %s.", GET_NAME(ch), GET_NAME(vict));
            break;
        case 21:
            vict->exp = RANGE(0, 50000000);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set exp for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set exp for %s.", GET_NAME(ch), GET_NAME(vict));
            break;
        case 22:
            ch->sendf("This does nothing at the moment.\r\n");
            break;
        case 23:
            vict->damage_mod = RANGE(-20, 20);
            affect_total(vict);
            break;
        case 24:
            if (GET_ADMLEVEL(ch) < ADMLVL_IMPL && ch != vict) {
                ch->sendf("You aren't godly enough for that!\r\n");
                return (0);
            }
            GET_INVIS_LEV(vict) = RANGE(0, GET_ADMLEVEL(vict));
            break;
        case 25:
            if (GET_ADMLEVEL(ch) < ADMLVL_IMPL && ch != vict) {
                ch->sendf("You aren't godly enough for that!\r\n");
                return (0);
            }
            vict->flipFlag(FlagType::Pref, PRF_NOHASSLE);
            break;
        case 26:
            if (ch == vict && on) {
                ch->sendf("Better not -- could be a long winter!\r\n");
                return (0);
            }
            vict->flipFlag(FlagType::PC,  PLR_FROZEN);
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
            if (!strcasecmp(val_arg, "off")) {
                GET_COND(vict, (mode - 29)) = -1; /* warning: magic number here */
                ch->sendf("%s's %s now off.\r\n", GET_NAME(vict), set_fields[mode].cmd);
            } else if (is_number(val_arg)) {
                value = atoi(val_arg);
                RANGE(0, 24);
                GET_COND(vict, (mode - 29)) = value; /* and here too */
                ch->sendf("%s's %s set to %" I64T ".\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
            } else {
                ch->sendf("Must be 'off' or a value from 0 to 24.\r\n");
                return (0);
            }
            break;
        case 32: vict->flipFlag(FlagType::PC,  PLR_KILLER);
            break;
        case 33: vict->flipFlag(FlagType::PC,  PLR_THIEF);
            break;
        case 34:
            if (!IS_NPC(vict) && value > 100) {
                ch->sendf("You can't do that.\r\n");
                return (0);
            }
            value = MAX(0, value);
            vict->set(CharNum::Level, value);
            break;
        case 35:
            if ((rnum = real_room(value)) == NOWHERE) {
                ch->sendf("No room exists with that number.\r\n");
                return (0);
            }
            if (IN_ROOM(vict) != NOWHERE)    /* Another Eric Green special. */
                vict->removeFromLocation();
            vict->addToLocation(getEntity(rnum));
            break;
        case 36: vict->flipFlag(FlagType::Pref, PRF_ROOMFLAGS);
            break;
        case 37: vict->flipFlag(FlagType::PC,  PLR_SITEOK);
            break;
        case 38: vict->flipFlag(FlagType::PC,  PLR_DELETED);
            break;
        case 39: {
            auto check = [&](SenseiID id) {return sensei::isPlayable(id) && sensei::isValidSenseiForRace(id, vict->race);};
            auto chosen_sensei = sensei::findSensei(val_arg, check);
            if (!chosen_sensei) {
                ch->sendf("That is not a sensei. Or, it is invalid for the target's race.\r\n");
                return (0);
            }
            vict->chclass = chosen_sensei.value();
        }
            break;
        case 40: vict->flipFlag(FlagType::PC,  PLR_NOWIZLIST);
            break;
        case 41: vict->flipFlag(FlagType::Pref, PRF_QUEST);
            break;
        case 42:
            if (!strcasecmp(val_arg, "off")) {
                vict->clearFlag(FlagType::PC, PLR_LOADROOM);
                GET_LOADROOM(vict) = NOWHERE;
            } else if (is_number(val_arg)) {
                rvnum = atoi(val_arg);
                if (real_room(rvnum) != NOWHERE) {
                    vict->setFlag(FlagType::PC, PLR_LOADROOM);
                    GET_LOADROOM(vict) = rvnum;
                    ch->sendf("%s will enter at room #%d.\r\n", GET_NAME(vict), GET_LOADROOM(vict));
                } else {
                    ch->sendf("That room does not exist!\r\n");
                    return (0);
                }
            } else {
                ch->sendf("Must be 'off' or a room's virtual number.\r\n");
                return (0);
            }
            break;
        case 43: vict->flipFlag(FlagType::Pref, PRF_COLOR);
            break;
        case 44:
            if (GET_IDNUM(ch) == 0 || IS_NPC(vict))
                return (0);
            GET_IDNUM(vict) = value;
            break;
        case 45:
            if (GET_IDNUM(ch) > 1) {
                ch->sendf("Please don't use this command, yet.\r\n");
                return (0);
            }
            if (GET_ADMLEVEL(ch) < 10) {
                ch->sendf("NO.\r\n");
                return (0);
            }
            break;
        case 46: vict->flipFlag(FlagType::PC,  PLR_NODELETE);
            break;
        case 47:
            if ((i = search_block(val_arg, genders, false)) < 0) {
                ch->sendf("Must be 'male', 'female', or 'neutral'.\r\n");
                return (0);
            }
            vict->set(CharAppearance::Sex, i);
            break;
        case 48:    /* set age */
            if (value <= 0) {    /* Arbitrary limits. */
                ch->sendf("Ages must be positive.\r\n");
                return (0);
            }
            /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
            vict->setAge(value);
            break;

        case 49:    /* Blame/Thank Rick Glover. :) */
            vict->setHeight(value);
            affect_total(vict);
            break;

        case 50:
            GET_WEIGHT(vict) = value;
            affect_total(vict);
            break;

        case 51:
            if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions"))
                GET_OLC_ZONE(vict) = AEDIT_PERMISSION;
            else if (is_abbrev(val_arg, "hedit"))
                GET_OLC_ZONE(vict) = HEDIT_PERMISSION;
            else if (is_abbrev(val_arg, "off"))
                GET_OLC_ZONE(vict) = NOWHERE;
            else if (!is_number(val_arg)) {
                ch->sendf("Value must be either 'socials', 'actions', 'hedit', 'off' or a zone number.\r\n");
                return (0);
            } else
                GET_OLC_ZONE(vict) = atoi(val_arg);
            break;

        case 52: {
            std::optional<RaceID> chosen_race;

            if (IS_NPC(vict)) {
                auto check = [ch](RaceID id) {return race::getValidSexes(id).contains(GET_SEX(ch));};
                chosen_race = race::findRace(val_arg, check);
            } else {
                auto check = [ch](RaceID id) {return race::getValidSexes(id).contains(GET_SEX(ch)) && race::isPlayable(id);};
                chosen_race = chosen_race = race::findRace(val_arg, check);
            }
            if (!chosen_race) {
                ch->sendf("That is not a valid race for them. Try changing sex first.\r\n");
                return (0);
            }
            vict->race = chosen_race.value();
            racial_body_parts(vict);
        }
            break;

        case 53:
            break;

        case 54:
            break;

        case 55:
            vict->set(CharAlign::LawChaos, RANGE(-1000, 1000));
            affect_total(vict);
            break;

        case 56:
            affect_total(vict);
            break;

        case 57:
            affect_total(vict);
            break;

        case 58:
            if (GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch) && vict != ch) {
                ch->sendf("Permission denied.\r\n");
                return (0);
            }
            if (value < ADMLVL_NONE || value > GET_ADMLEVEL(ch)) {
                ch->sendf("You can't set it to that.\r\n");
                return (0);
            }
            if (GET_ADMLEVEL(vict) == value)
                return (1);
            admin_set(vict, value);
            break;

        case 59:
            if (value < 0 || value >= 6) {
                ch->sendf("You can't set it to that.\r\n");
                return (0);
            }
            vict->set(CharAppearance::HairLength, value);
            return (1);
            break;

        case 60:
            if (value < 0 || value >= 13) {
                ch->sendf("You can't set it to that.\r\n");
                return (0);
            }
            vict->set(CharAppearance::HairStyle, value);
            return (1);
            break;

        case 61:
            if (value < 0 || value >= 15) {
                ch->sendf("You can't set it to that.\r\n");
                return (0);
            }
            vict->set(CharAppearance::HairColor, value);
            return (1);
            break;

        case 62:
            if (value < 0 || value >= 12) {
                ch->sendf("You can't set it to that.\r\n");
                return (0);
            }
            vict->set(CharAppearance::SkinColor, value);
            return (1);
            break;

        case 63:
            if (value < 0 || value >= 13) {
                ch->sendf("You can't set it to that.\r\n");
                return (0);
            }
            vict->set(CharAppearance::EyeColor, value);
            return (1);
            break;

        case 64:
            vict->set(CharStat::PowerLevel, value);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set basepl for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set basepl for %s.", GET_NAME(ch), GET_NAME(vict));
            break;

        case 65:
            vict->set(CharStat::Ki, value);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set baseki for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set baseki for %s.", GET_NAME(ch), GET_NAME(vict));
            break;

        case 66:
            vict->set(CharStat::Stamina, value);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set basest for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set basest for %s.", GET_NAME(ch), GET_NAME(vict));
            break;

        case 67:
            GET_DROOM(vict) = RANGE(0, 20000);
            break;

        case 68:
            GET_ABSORBS(vict) = RANGE(0, 3);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set absorbs for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set absorbs for %s.", GET_NAME(ch), GET_NAME(vict));
            break;

        case 69:
            GET_UP(vict) += RANGE(1, 1000);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set upgrade points for %s.",
                   GET_NAME(ch), GET_NAME(vict));
            log_imm_action("SET: %s has set upgrade points for %s.", GET_NAME(ch), GET_NAME(vict));
            break;

        case 70:
            vict->set(CharAppearance::Aura, RANGE(0, 8));
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set aura for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set aura for %s.", GET_NAME(ch), GET_NAME(vict));
            break;
        case 71:
            ch->sendf("Use the reward command.\r\n");
            break;
        case 72:
            GET_BOOSTS(vict) = RANGE(-1000, 1000);
            break;
        case 73: vict->flipFlag(FlagType::PC,  PLR_MULTP);
            break;
        case 74:
            GET_DCOUNT(vict) = RANGE(-1000, 1000);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set death count for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set death count for %s.", GET_NAME(ch), GET_NAME(vict));
            break;
        case 75:
            ch->sendf("No.");
            break;
        case 76:
            if (vict->desc != nullptr) {
                star_phase(vict, RANGE(0, 2));
            } else {
                ch->sendf("They aren't even in the game!\r\n");
            }
            break;
        case 77:
            vict->set(CharNum::RacialPref, RANGE(1, 3));
            break;

        case 78:
            GET_SLOTS(vict) = RANGE(1, 1000);
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set skill slots for %s.", GET_NAME(ch),
                   GET_NAME(vict));
            log_imm_action("SET: %s has set skill slots for %s.", GET_NAME(ch), GET_NAME(vict));
            break;

        case 79:
            GET_FEATURE(vict) = '\0';
            break;

        case 80:
            mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "SET: %s has set transformation class for %s.",
                   GET_NAME(ch), GET_NAME(vict));
            log_imm_action("SET: %s has set transformation class for %s.", GET_NAME(ch), GET_NAME(vict));
            break;

        case 81:
            ch->sendf("Done.\r\n");
            break;

        case 82:
            ch->sendf("Can't set that!\r\n");
            break;

        default:
            ch->sendf("Can't set that!\r\n");
            return (0);
    }

    return (1);
}


ACMD(do_set) {
    BaseCharacter *vict = nullptr, *cbuf = nullptr;
    char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    int mode, len, player_i = 0, retval;
    char is_file = 0, is_player = 0;

    half_chop(argument, name, buf);

    if (!strcasecmp(name, "player")) {
        is_player = 1;
        half_chop(buf, name, buf);
    } else if (!strcasecmp(name, "mob"))
        half_chop(buf, name, buf);

    half_chop(buf, field, buf);

    if (!*name || !*field) {
        ch->sendf("Usage: set <victim> <field> <value>\r\n");
        return;
    }

    /* find the target */
    if (is_player) {
        vict = findPlayer(name);
        if (!vict) {
            ch->sendf("There is no such player.\r\n");
            return;
        }
    } else { /* is_mob */
        if (!(vict = get_char_vis(ch, name, nullptr, FIND_CHAR_WORLD))) {
            ch->sendf("There is no such creature.\r\n");
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

ACMD(do_saveall) {
    if (GET_ADMLEVEL(ch) < ADMLVL_BUILDER)
        ch->sendf("You are not holy enough to use this privelege.\r\n");
    else {
        save_all();
        ch->sendf("World and house files saved.\r\n");
    }
}

#define PLIST_FORMAT \
  "players [minlev[-maxlev]] [-n name] [-d days] [-h hours] [-m]"

ACMD(do_plist) {

}

ACMD(do_peace) {
    send_to_room(IN_ROOM(ch), "Everything is quite peaceful now.\r\n");

    for (auto vict : ch->getRoom()->getPeople()) {
        if (GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch))
            continue;
        stop_fighting(vict);
        GET_POS(vict) = POS_SITTING;
    }
    stop_fighting(ch);
    GET_POS(ch) = POS_STANDING;
}

ACMD(do_wizupdate) {
    run_autowiz();
    ch->sendf("Wizlists updated.\r\n");
}

ACMD(do_raise) {
    BaseCharacter *vict = nullptr;
    char name[MAX_INPUT_LENGTH];

    one_argument(argument, name);

    if (GET_ADMLEVEL(ch) < ADMLVL_BUILDER && !IS_NPC(ch)) {
        return;
    }

    if (!(vict = get_player_vis(ch, name, nullptr, FIND_CHAR_WORLD))) {
        ch->sendf("There is no such player.\r\n");
        return;
    }

    if (IS_NPC(vict)) {
        ch->sendf("Sorry, only players get spirits.\r\n");
        return;
    }

    if (!AFF_FLAGGED(vict, AFF_SPIRIT)) {
        ch->sendf("But they aren't even dead!\r\n");
        return;
    }

    if (GET_ADMLEVEL(ch) <= 0) {
        vict->resurrect(Basic);
    } else {
        vict->resurrect(Costless);
    }

    ch->sendf("@wYou return %s from the @Bspirit@w world, to the world of the living!@n\r\n", GET_NAME(vict));
    vict->sendf("@wYour @Bspirit@w has been returned to the world of the living by %s!@n\r\n", GET_NAME(ch));

    send_to_imm("Log: %s has raised %s from the dead.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("RAISE: %s has raised %s from the dead.", GET_NAME(ch), GET_NAME(vict));


}

ACMD(do_chown) {
    BaseCharacter *victim;
    Object *obj;
    char buf2[80];
    char buf3[80];
    int i, k = 0;

    two_arguments(argument, buf2, buf3);

    if (!*buf2)
        ch->sendf("Syntax: chown <object> <character>.\r\n");
    else if (!(victim = get_char_vis(ch, buf3, nullptr, FIND_CHAR_WORLD)))
        ch->sendf("No one by that name here.\r\n");
    else if (victim == ch)
        ch->sendf("Are you sure you're feeling ok?\r\n");
    else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
        ch->sendf("That's really not such a good idea.\r\n");
    else if (!*buf3)
        ch->sendf("Syntax: chown <object> <character>.\r\n");
    else {
        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(victim, i) && CAN_SEE_OBJ(ch, GET_EQ(victim, i)) &&
                isname(buf2, GET_EQ(victim, i)->getName().c_str())) {
                    unequip_char(victim, i)->addToLocation(victim);
                k = 1;
            }
        }

        if (!(obj = get_obj_in_list_vis(victim, buf2, nullptr, victim->getInventory()))) {
            if (!k && !(obj = get_obj_in_list_vis(victim, buf2, nullptr, victim->getInventory()))) {
                ch->sendf("%s does not appear to have the %s.\r\n", GET_NAME(victim), buf2);
                return;
            }
        }

        act("@n$n makes a magical gesture and $p@n flies from $N to $m.", false, ch, obj, victim, TO_NOTVICT);
        act("@n$n makes a magical gesture and $p@n flies away from you to $m.", false, ch, obj, victim, TO_VICT);
        act("@nYou make a magical gesture and $p@n flies away from $N to you.", false, ch, obj, victim, TO_CHAR);

        obj->removeFromLocation();
        obj->addToLocation(ch);
    }
}

ACMD(do_zpurge) {
    BaseCharacter *mob, *next_mob;
    vnum i, stored = -1, zone;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    zone = !*arg ? zone_table[ch->getRoom()->zone].number : atol(arg);

    if (!zone_table.count(zone) || !can_edit_zone(ch, zone)) {
        ch->sendf("You cannot purge that zone. Try %d.\r\n", GET_OLC_ZONE(ch));
        return;
    }

    auto &z = zone_table[zone];

    for (auto rvn : z.rooms) {
        auto r = getEntity<Room>(rvn);
        if(!r) continue;
        for (auto mob : r->getPeople()) {
            if (IS_NPC(mob)) {
                extract_char(mob);
            }
        }

        for (auto obj : r->getInventory()) {
            obj->extractFromWorld();
        }
    }

    ch->sendf("All mobiles and objects in zone %d purged.\r\n", zone);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), true, "(GC) %s has purged zone %d.", GET_NAME(ch), zone);
}

/******************************************************************************/
/*                         Zone Checker Code below                            */
/******************************************************************************/

/*mob limits*/
#define MAX_DAMAGE_MOD_ALLOWED   MAX(GET_LEVEL(mob)/5, 2)
#define MAX_GOLD_ALLOWED         GET_LEVEL(mob)*3000
#define MAX_EXP_ALLOWED          GET_LEVEL(mob)*GET_LEVEL(mob) * 120
#define MAX_LEVEL_ALLOWED        (100)
#define GET_OBJ_AVG_DAM(obj)     (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1))
/* arbitrary limit for per round dam */
#define MAX_MOB_DAM_ALLOWED      500

#define ZCMD2 zone_table[zone].cmd[cmd_no]  /*from DB.C*/

/*item limits*/
#define MAX_DAM_ALLOWED            50    /* for weapons  - avg. dam*/
#define MAX_AFFECTS_ALLOWED        3

/* Armor class limits*/
#define TOTAL_WEAR_CHECKS  (NUM_ITEM_WEARS-2)  /*minus Wield and Take*/
static const struct zcheck_armor {
    bitvector_t bitvector;          /* from Structs.h                       */
    int ac_allowed;                 /* Max. AC allowed for this body part  */
    char *message;                  /* phrase for error message            */
} zarmor[] = {
        {ITEM_WEAR_FINGER, 10, "Ring"},
        {ITEM_WEAR_NECK,   10, "Necklace"},
        {ITEM_WEAR_BODY,   10, "Body armor"},
        {ITEM_WEAR_HEAD,   10, "Head gear"},
        {ITEM_WEAR_LEGS,   10, "Legwear"},
        {ITEM_WEAR_FEET,   10, "Footwear"},
        {ITEM_WEAR_HANDS,  10, "Glove"},
        {ITEM_WEAR_ARMS,   10, "Armwear"},
        {ITEM_WEAR_SHIELD, 10, "Shield"},
        {ITEM_WEAR_ABOUT,  10, "Cloak"},
        {ITEM_WEAR_WAIST,  10, "Belt"},
        {ITEM_WEAR_WRIST,  10, "Wristwear"},
        {ITEM_WEAR_HOLD,   10, "Held item"},
        {ITEM_WEAR_PACK,   10, "Backpack item"},
        {ITEM_WEAR_EAR,    10, "Earring item"},
        {ITEM_WEAR_SH,     10, "Shoulder item"},
        {ITEM_WEAR_EYE,    10, "Eye item"}
};

/*These are strictly boolean*/
#define CAN_WEAR_WEAPONS         0     /* toggle - can weapon also be armor? */
#define MAX_APPLIES_LIMIT        1     /* toggle - is there a limit at all?  */
#define CHECK_ITEM_RENT          0     /* do we check for rent cost == 0 ?   */
#define CHECK_ITEM_COST          0     /* do we check for item cost == 0 ?   */

/* These are ABS() values. */
#define MAX_APPLY_ACCURCY_MOD_TOTAL    5
#define MAX_APPLY_DAMAGE_MOD_TOTAL    5

/*room limits*/
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
static const int offlimit_zones[] = {0, 12, 13, 14, -1};  /*what zones can no room connect to (virtual num) */
#define MIN_ROOM_DESC_LENGTH   80       /* at least one line - set to 0 to not care. */
#define MAX_COLOUMN_WIDTH      80       /* at most 80 chars per line */


ACMD (do_zcheck) {
    zone_rnum zrnum;
    Object *obj;
    room_vnum exroom = 0;
    int ac = 0;
    int affs = 0, tohit, todam, value;
    int i = 0, j = 0, k = 0, l = 0, m = 0, found = 0; /* found is used as a 'send now' flag*/
    char buf[MAX_STRING_LENGTH];
    float avg_dam;
    size_t len = 0;
    struct extra_descr_data *ext, *ext2;
    one_argument(argument, buf);

    ch->sendf("Temporarily disabled.\r\n");

}

/**********************************************************************************/

ACMD(do_checkloadstatus) {
    ch->sendf("Disabled.\r\n");
}
/**************************************************************************************/
/*                         Zone Checker code above                                    */
/**************************************************************************************/

/* do_findkey, finds where the key to a door loads to using do_checkloadstatus() */
ACMD(do_findkey) {
    int dir, key;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    any_one_arg(argument, arg); /* Because "in" is a valid direction */

    if (!*arg) {
        ch->sendf("Format: findkey <dir>\r\n");
    } else if ((dir = search_block(arg, dirs, false)) >= 0 ||
               (dir = search_block(arg, abbr_dirs, false)) >= 0) {
        if (!EXIT(ch, dir)) {
            ch->sendf("There's no exit in that direction!\r\n");
        } else if ((key = EXIT(ch, dir)->key) == NOTHING || key == 0) {
            ch->sendf("There's no key for that exit.\r\n");
        } else {
            sprintf(buf, "obj %d", key);
            do_checkloadstatus(ch, buf, 0, 0);
        }
    } else {
        ch->sendf("What direction is that?!?\r\n");
    }
}

ACMD(do_spells) {
    int i, qend;

    ch->sendf("The following spells are in the game:\r\n");

    for (qend = 0, i = 0; i < MAX_SPELLS; i++) {
        if (spell_info[i].name == unused_spellname)       /* This is valid. */
            continue;
        ch->sendf("%18s", spell_info[i].name);
        if (qend++ % 4 == 3)
            ch->sendf("\r\n");
    }
    if (qend % 4 != 0)
        ch->sendf("\r\n");
    return;

}

ACMD(do_boom) {
    /* Only IDNUM = 1 can cause a boom! Ideally for testing something that might
   * cause a crash. Currently left over from testing changes to send_to_outdoor. */
    if (GET_IDNUM(ch) != 1) {
        ch->sendf("Sorry, only the Founder may use the boom command.\r\n");
        return;
    }

    send_to_outdoor("%s shakes the world with a mighty boom!\r\n", GET_NAME(ch));
}
