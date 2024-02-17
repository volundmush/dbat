#include "dbat/migrate.h"
#include "dbat/comm.h"
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/constants.h"
#include "dbat/genolc.h"
#include "dbat/maputils.h"
#include "dbat/config.h"
#include <filesystem>
#include <memory>
#include <iostream>
#include <vector>
#include <tuple>
#include <fstream>
#include "dbat/class.h"
#include "dbat/players.h"
#include "dbat/account.h"
#include "dbat/act.item.h"
#include "dbat/pfdefaults.h"
#include "dbat/spell_parser.h"
#include "dbat/shop.h"
#include "dbat/guild.h"
#include "dbat/genobj.h"
#include "dbat/saveload.h"
#include "dbat/random.h"

#define RENT_FACTOR    1
#define CRYO_FACTOR    4

#define LOC_INVENTORY    0
#define MAX_BAG_ROWS    5

#define MAX_HOUSES    1000
#define MAX_GUESTS    10

#define HOUSE_PRIVATE    0
#define HOUSE_GOD       1  /* Imm owned house             */
#define HOUSE_CLAN      2  /* Clan crash-save room        */
#define HOUSE_UNOWNED   3

#define NUM_HOUSE_TYPES 4


#define HOUSE_FLAGS(house) (house).bitvector

/* House can have up to 31 bitvectors - don't go higher */
#define HOUSE_NOGUESTS   (1 << 0)   /* Owner cannot add guests     */
#define HOUSE_FREE       (1 << 1)   /* House does not require payments */
#define HOUSE_NOIMMS     (1 << 2)   /* Imms below level 999 cannot enter */
#define HOUSE_IMPONLY    (1 << 3)   /* Imms below level 1000 cannot enter */
#define HOUSE_RENTFREE   (1 << 4)   /* No rent is charged on items left here */
#define HOUSE_SAVENORENT (1 << 5)   /* NORENT items are crashsaved too */
#define HOUSE_NOSAVE     (1 << 6)   /* Do not crash save this room - private only */

#define HOUSE_NUM_FLAGS 7

#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
world[room].dir_option[dir]->to_room : NOWHERE)


static bool converting = false;

static void parse_trigger(FILE *trig_f, trig_vnum nr) {
    int t[2], k, attach_type;
    char line[256], *cmds, *s, flags[256], errors[MAX_INPUT_LENGTH];
    auto trig = std::make_shared<trig_proto>();
    trig_index[nr] = trig;
    trig->vn = nr;

    auto &z = zone_table[real_zone_by_thing(nr)];
    z.triggers.insert(nr);

    snprintf(errors, sizeof(errors), "trig vnum %d", nr);

    trig->name = fread_string(trig_f, errors);

    get_line(trig_f, line);
    k = sscanf(line, "%d %s %d", &attach_type, flags, t);
    trig->attach_type = (int8_t) attach_type;
    trig->trigger_type = (long) asciiflag_conv(flags);
    trig->narg = (k == 3) ? t[0] : 0;
    auto args = fread_string(trig_f, errors);
    if(args) trig->arglist = args;

    cmds = s = fread_string(trig_f, errors);

    trig->lines.emplace_back(strtok(s, "\r\n"));

    while ((s = strtok(nullptr, "\r\n"))) {
        trig->lines.emplace_back(s);
    }

    free(cmds);
}

static void read_line(FILE *shop_f, const char *string, void *data) {
    char buf[READ_SIZE];

    if (!get_line(shop_f, buf) || !sscanf(buf, string, data)) {
        basic_mud_log("SYSERR: Error in shop #%d, near '%s' with '%s'", SHOP_NUM(top_shop), buf, string);
        exit(1);
    }
}

static void read_guild_line(FILE *gm_f, char *string, void *data, char *type) {
    char buf[MAX_STRING_LENGTH];

    if (!get_line(gm_f, buf) || !sscanf(buf, string, data)) {
        fprintf(stderr, "Error in guild #%d, Could not get %s\n", GM_NUM(top_guild), type);
        exit(1);
    }
}

static char *read_shop_message(int mnum, room_vnum shr, FILE *shop_f, const char *why) {
    int cht, ss = 0, ds = 0, err = 0;
    char *tbuf;

    if (!(tbuf = fread_string(shop_f, why)))
        return (nullptr);

    for (cht = 0; tbuf[cht]; cht++) {
        if (tbuf[cht] != '%')
            continue;

        if (tbuf[cht + 1] == 's')
            ss++;
        else if (tbuf[cht + 1] == 'd' && (mnum == 5 || mnum == 6)) {
            if (ss == 0) {
                basic_mud_log("SYSERR: Shop #%d has %%d before %%s, message #%d.", shr, mnum);
                err++;
            }
            ds++;
        } else if (tbuf[cht + 1] != '%') {
            basic_mud_log("SYSERR: Shop #%d has invalid format '%%%c' in message #%d.", shr, tbuf[cht + 1], mnum);
            err++;
        }
    }

    if (ss > 1 || ds > 1) {
        basic_mud_log("SYSERR: Shop #%d has too many specifiers for message #%d. %%s=%d %%d=%d", shr, mnum, ss, ds);
        err++;
    }

    if (err) {
        free(tbuf);
        return (nullptr);
    }
    return (tbuf);
}

static void boot_the_guilds(FILE *gm_f, char *filename, int rec_count) {
    char *buf, buf2[256], *p, buf3[READ_SIZE];
    int temp, val, t1, t2, rv;
    int done = false;

    snprintf(buf2, sizeof(buf2), "beginning of GM file %s", filename);

    buf = fread_string(gm_f, buf2);
    while (!done) {
        if (*buf == '#') {        /* New Trainer */
            sscanf(buf, "#%d\n", &temp);
            snprintf(buf2, sizeof(buf2), "GM #%d in GM file %s", temp, filename);
            free(buf);        /* Plug memory leak! */
            top_guild = temp;
            auto &g = guild_index[temp];
            auto &z = zone_table[real_zone_by_thing(temp)];
            z.guilds.insert(temp);

            GM_NUM(top_guild) = temp;

            get_line(gm_f, buf3);
            rv = sscanf(buf3, "%d %d", &t1, &t2);
            while (t1 > -1) {
                if (rv == 1) { /* old style guilds, only skills */
                    g.skills.insert(t1);
                } else if (rv == 2) { /* new style guilds, skills and feats */
                    if (t2 == 1) {
                        g.skills.insert(t1);
                    } else if (t2 == 2) {
                        g.feats.insert(t1);
                    } else {
                        basic_mud_log("SYSERR: Invalid 2nd arg in guild file!");
                        exit(1);
                    }
                } else {
                    basic_mud_log("SYSERR: Invalid format in guild file. Expecting 2 args but got %d!", rv);
                    exit(1);
                }
                get_line(gm_f, buf3);
                rv = sscanf(buf3, "%d %d", &t1, &t2);
            }
            read_guild_line(gm_f, "%f", &g.charge, "GM_CHARGE");
            g.no_such_skill = fread_string(gm_f, buf2);
            g.not_enough_gold = fread_string(gm_f, buf2);

            read_guild_line(gm_f, "%d", &g.minlvl, "GM_MINLVL");
            read_guild_line(gm_f, "%d", &g.gm, "GM_TRAINER");

            g.gm = real_mobile(g.gm);
            read_guild_line(gm_f, "%d", &g.with_who[0], "GM_WITH_WHO");

            read_guild_line(gm_f, "%d", &g.open, "GM_OPEN");
            read_guild_line(gm_f, "%d", &g.close, "GM_CLOSE");

            CREATE(buf, char, READ_SIZE);
            get_line(gm_f, buf);
            if (buf && *buf != '#' && *buf != '$') {
                p = buf;
                for (temp = 1; temp < GW_ARRAY_MAX; temp++) {
                    if (!p || !*p)
                        break;
                    if (sscanf(p, "%d", &val) != 1) {
                        basic_mud_log("SYSERR: Can't parse GM_WITH_WHO line in %s: '%s'", buf2, buf);
                        break;
                    }
                    g.with_who[temp] = val;
                    while (isdigit(*p) || *p == '-') {
                        p++;
                    }
                    while (*p && !(isdigit(*p) || *p == '-')) {
                        p++;
                    }
                }
                free(buf);
                buf = fread_string(gm_f, buf2);
            }
        } else {
            if (*buf == '$')        /* EOF */
                done = true;
            free(buf);        /* Plug memory leak! */
        }
    }
}

static void boot_the_shops(FILE *shop_f, char *filename, int rec_count) {
    char *buf, buf2[256], *p;
    shop_vnum temp, count, new_format = false;
    int shop_temp;
    struct shop_buy_data list[MAX_SHOP_OBJ + 1]{};
    int done = false;

    snprintf(buf2, sizeof(buf2), "beginning of shop file %s", filename);

    while (!done) {
        buf = fread_string(shop_f, buf2);
        if (*buf == '#') {        /* New shop */
            sscanf(buf, "#%ld\n", &temp);
            snprintf(buf2, sizeof(buf2)-1, "shop #%ld in shop file %s", temp, filename);
            auto &sh = shop_index[temp];
            free(buf);        /* Plug memory leak! */
            sh.vnum = temp;
            auto &z = zone_table[real_zone_by_thing(sh.vnum)];
            z.shops.insert(sh.vnum);
            top_shop = temp;
            while(true) {
                read_line(shop_f, "%ld", &shop_temp);
                if(shop_temp == -1) break;
                temp = (shop_vnum)shop_temp;
                if(obj_index.count(temp)) sh.producing.push_back(temp);
            }

            read_line(shop_f, "%f", &SHOP_BUYPROFIT(top_shop));
            read_line(shop_f, "%f", &SHOP_SELLPROFIT(top_shop));

            while(true) {
                read_line(shop_f, "%ld", &shop_temp);
                if(shop_temp == -1) break;
                auto &t = sh.type.emplace_back();
                t.type = shop_temp;
            }

            sh.no_such_item1 = read_shop_message(0, SHOP_NUM(top_shop), shop_f, buf2);
            sh.no_such_item2 = read_shop_message(1, SHOP_NUM(top_shop), shop_f, buf2);
            sh.do_not_buy = read_shop_message(2, SHOP_NUM(top_shop), shop_f, buf2);
            sh.missing_cash1 = read_shop_message(3, SHOP_NUM(top_shop), shop_f, buf2);
            sh.missing_cash2 = read_shop_message(4, SHOP_NUM(top_shop), shop_f, buf2);
            sh.message_buy = read_shop_message(5, SHOP_NUM(top_shop), shop_f, buf2);
            sh.message_sell = read_shop_message(6, SHOP_NUM(top_shop), shop_f, buf2);
            read_line(shop_f, "%d", &SHOP_BROKE_TEMPER(top_shop));
            read_line(shop_f, "%ld", &SHOP_BITVECTOR(top_shop));
            read_line(shop_f, "%hd", &SHOP_KEEPER(top_shop));

            SHOP_KEEPER(top_shop) = real_mobile(SHOP_KEEPER(top_shop));
            CREATE(buf, char, READ_SIZE);
            get_line(shop_f, buf);
            p = buf;
            for (temp = 0; temp < SW_ARRAY_MAX; temp++) {
                if (!p || !*p)
                    break;
                if (sscanf(p, "%d", &count) != 1) {
                    basic_mud_log("SYSERR: Can't parse TRADE_WITH line in %s: '%s'", buf2, buf);
                    break;
                }
                SHOP_TRADE_WITH(top_shop)[temp] = count;
                while (isdigit(*p) || *p == '-') {
                    p++;
                }
                while (*p && !(isdigit(*p) || *p == '-')) {
                    p++;
                }
            }
            free(buf);
            while (temp < SW_ARRAY_MAX)
                SHOP_TRADE_WITH(top_shop)[temp++] = 0;

            while(true) {
                read_line(shop_f, "%ld", &shop_temp);
                if(shop_temp == -1) break;
                if(world.contains(shop_temp)) sh.in_room.insert(shop_temp);

            }


            read_line(shop_f, "%d", &SHOP_OPEN1(top_shop));
            read_line(shop_f, "%d", &SHOP_CLOSE1(top_shop));
            read_line(shop_f, "%d", &SHOP_OPEN2(top_shop));
            read_line(shop_f, "%d", &SHOP_CLOSE2(top_shop));

            SHOP_BANK(top_shop) = 0;
            SHOP_SORT(top_shop) = 0;
            SHOP_FUNC(top_shop) = nullptr;
        } else {
            if (*buf == '$')        /* EOF */
                done = true;
            else if (strstr(buf, VERSION3_TAG))    /* New format marker */
                new_format = true;
            free(buf);        /* Plug memory leak! */
        }
    }
}

static int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits) {
    unsigned int flagnum;
    bool error = false;

    /* See if any bits are set above the ones we know about. */
    if (bits <= (~(bitvector_t) 0 >> (sizeof(bitvector_t) * 8 - namecount)))
        return (false);

    for (flagnum = namecount; flagnum < sizeof(bitvector_t) * 8; flagnum++)
        if ((1 << flagnum) & bits) {
            basic_mud_log("SYSERR: %s has unknown %s flag, bit %d (0 through %" SZT " known).", whatami, whatbits, flagnum,
                namecount - 1);
            error = true;
        }

    return (error);
}

static char fread_letter(FILE *fp) {
    char c;
    do {
        c = getc(fp);
    } while (isspace(c));
    return c;
}

static void get_one_line(FILE *fl, char *buf) {
    if (fgets(buf, READ_SIZE, fl) == nullptr) {
        basic_mud_log("SYSERR: error reading help file: not terminated with $?");
        exit(1);
    }

    buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}

/* read direction data */
static void setup_dir(FILE *fl, room_vnum room, int dir) {
    int t[11], retval;
    char line[READ_SIZE], buf2[128];

    snprintf(buf2, sizeof(buf2), "room #%d, direction D%d", room, dir);

    auto r = world.at(room);
    
    auto d = new Exit();
    d->script = std::make_shared<script_data>(d);
    d->uid = getNextUID();
    

    if(auto b = fread_string(fl, buf2); b) {
        d->setLookDesc(b);
        free(b);
    }

    if(auto b = fread_string(fl, buf2); b) {
        d->setAlias(b);
        free(b);
    }

    d->addToLocation(r, dir);

    if (!get_line(fl, line)) {
        basic_mud_log("SYSERR: Format error, %s", buf2);
        exit(1);
    }
    if (((retval = sscanf(line, " %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7,
                          t + 8, t + 9, t + 10)) == 3) && (bitwarning == true)) {
        basic_mud_log("SYSERR: Format error, %s", buf2);
        exit(1);
    } else if (bitwarning == false) {

        bitvector_t flags = 0;
        if (t[0] == 1)
            flags = EX_ISDOOR;
        else if (t[0] == 2)
            flags = EX_ISDOOR | EX_PICKPROOF;
        else if (t[0] == 3)
            flags = EX_ISDOOR | EX_SECRET;
        else if (t[0] == 4)
            flags = EX_ISDOOR | EX_PICKPROOF | EX_SECRET;

        for(auto i = 0; i < NUM_EXIT_FLAGS; i++) if(IS_SET(flags, i)) d->setFlag(FlagType::Exit, i);
        d->key = world.contains(t[1]) ? t[1] : NOTHING;
        d->destination = dynamic_cast<Room*>(world.contains(t[2]) ? world.at(t[2]) : nullptr);

        if (retval == 3) {
            basic_mud_log("Converting world files to include DC add ons.");
            d->dclock = 20;
            d->dchide = 20;
            d->dcskill = 0;
            d->dcmove = 0;
            d->failsavetype = 0;
            d->dcfailsave = 0;
            d->failroom = nullptr;
            d->totalfailroom = nullptr;
            if (bitsavetodisk) {
                converting = true;
            }
        } else if (retval == 5) {
            d->dclock = t[3];
            d->dchide = t[4];
            d->dcskill = 0;
            d->dcmove = 0;
            d->failsavetype = 0;
            d->dcfailsave = 0;
            if (bitsavetodisk) {
                converting = true;
            }
        } else if (retval == 7) {
            d->dclock = t[3];
            d->dchide = t[4];
            d->dcskill = t[5];
            d->dcmove = t[6];
            d->failsavetype = 0;
            d->dcfailsave = 0;
            if (bitsavetodisk) {
                converting = true;
            }
        } else if (retval == 11) {
            d->dclock = t[3];
            d->dchide = t[4];
            d->dcskill = t[5];
            d->dcmove = t[6];
            d->failsavetype = t[7];
            d->dcfailsave = t[8];
            d->failroom = dynamic_cast<Room*>(world.contains(t[9]) ? world.at(t[9]) : nullptr);
            d->totalfailroom = dynamic_cast<Room*>(world.contains(t[10]) ? world.at(t[10]) : nullptr);
        }
    }
}


/* load the rooms */
static void parse_room(FILE *fl, room_vnum virtual_nr) {
    int t[10], i, retval;
    char line[READ_SIZE], flags[128], flags2[128], flags3[128];
    char flags4[128], buf2[MAX_STRING_LENGTH], buf[128];
    bitvector_t roomFlagsHolder[4];
    struct extra_descr_data *new_descr;
    char letter;

    /* This really had better fit or there are other problems. */
    snprintf(buf2, sizeof(buf2), "room #%d", virtual_nr);

    auto zone = real_zone_by_thing(virtual_nr);
    if (zone == NOWHERE) {
        basic_mud_log("SYSERR: Room #%d is outside any zone.", virtual_nr);
        exit(1);
    }

    if(world.count(virtual_nr)) {
        basic_mud_log("SYSERR: Room #%d already exists, cannot parse!", virtual_nr);
        exit(1);
    }
    auto &z = zone_table[zone];
    auto r = new Room();
    world[virtual_nr] = r;
    z.rooms.insert(virtual_nr);
    r->script = std::make_shared<script_data>(r);
    r->zone = zone;
    r->vn = virtual_nr;
    r->uid = virtual_nr;
    r->setName(fread_string(fl, buf2));
    r->setLookDesc(fread_string(fl, buf2));

    if (!get_line(fl, line)) {
        basic_mud_log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
            virtual_nr);
        exit(1);
    }

    if ((retval = sscanf(line, " %d %s %s %s %s %d ", t, flags, flags2, flags3, flags4, t + 2)) == 6) {
        int taeller;
        roomFlagsHolder[0] = asciiflag_conv(flags);
        roomFlagsHolder[1] = asciiflag_conv(flags2);
        roomFlagsHolder[2] = asciiflag_conv(flags3);
        roomFlagsHolder[3] = asciiflag_conv(flags4);

        for(auto i = 0; i < NUM_ROOM_FLAGS; i++) if(IS_SET_AR(roomFlagsHolder, i)) r->setFlag(FlagType::Room, i);

        r->sector_type = t[2];
        sprintf(flags, "object #%d", virtual_nr);    /* sprintf: OK (until 399-bit integers) */
        //check_bitvector_names(r.room_flags, room_bits_count, flags, "room");
    } else {
        basic_mud_log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
        exit(1);
    }

    r->timed = -1;

    snprintf(buf, sizeof(buf), "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

    while(true) {
        if (!get_line(fl, line)) {
            basic_mud_log("%s", buf);
            exit(1);
        }
        switch (*line) {
            case 'D':
                setup_dir(fl, virtual_nr, atoi(line + 1));
                break;
            case 'E':
                {
                    auto &e = r->ex_description.emplace_back();
                    e.keyword = fread_string(fl, buf2);
                    e.description = fread_string(fl, buf2);
                }
                break;
            case 'S':            /* end of room */
                /* DG triggers -- script is defined after the end of the room */
                letter = fread_letter(fl);
                ungetc(letter, fl);
                while (letter == 'T') {
                    char junk[248];
                    char tline[248];
                    trig_vnum tvn;
                    get_line(fl, tline);
                    auto count = sscanf(line, "%7s %d", junk, &tvn);
                    if(trig_index.contains(tvn)) r->proto_script.push_back(tvn);
                    letter = fread_letter(fl);
                    ungetc(letter, fl);
                }
                return;
            default:
                basic_mud_log("%s", buf);
                exit(1);
        }
    }
}

/*
 * "resulve vnums into rnums in the zone reset tables"
 *
 * Or in English: Once all of the zone reset tables have been loaded, we
 * resolve the virtual numbers into real numbers all at once so we don't have
 * to do it repeatedly while the game is running.  This does make adding any
 * room, mobile, or object a little more difficult while the game is running.
 *
 * NOTE 1: Assumes NOWHERE == NOBODY == NOTHING.
 * NOTE 2: Assumes sizeof(room_rnum) >= (sizeof(mob_rnum) and sizeof(obj_rnum))
 */

static int parse_simple_mob(FILE *mob_f, nlohmann::json& ch, mob_vnum nr) {
    int j, t[10];
    char line[READ_SIZE];

    if (!get_line(mob_f, line)) {
        basic_mud_log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
        return 0;
    }

    if (sscanf(line, " %ld %ld %ld %ldd%ld+%ld %ldd%ld+%ld ",
               t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
        basic_mud_log("SYSERR: Format error in mob #%d, first line after S flag\n"
            "...expecting line of form '# # # #d#+# #d#+#'", nr);
        return 0;
    }
    auto level = t[0];
    ch["level"] = t[0];
    ch["nums"].push_back(std::make_pair(CharNum::Level, level));

    /* max hit = 0 is a flag that H, M, V is xdy+z */
    ch["stats"].push_back(std::make_pair(CharStat::PowerLevel, t[3]));
    ch["stats"].push_back(std::make_pair(CharStat::Ki, t[4]));
    ch["stats"].push_back(std::make_pair(CharStat::Stamina, t[5]));

    ch["damage_mod"] = t[8];

    if (!get_line(mob_f, line)) {
        basic_mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
            "...expecting line of form '# #', but file ended!", nr);
        return 0;
    }

    if (sscanf(line, " %ld %ld %ld %ld", t, t + 1, t + 2, t + 3) != 4) {
        basic_mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
            "...expecting line of form '# # # #'", nr);
        return 0;
    }
    ch["moneys"].push_back(std::make_pair(CharMoney::Carried, t[0]));
    ch["race"] = t[2];
    ch["chclass"] = t[3];


    /* GET_CLASS_RANKS(ch, t[3]) = GET_LEVEL(ch); */
    if (!get_line(mob_f, line)) {
        basic_mud_log("SYSERR: Format error in last line of mob #%d\n"
            "...expecting line of form '# # #', but file ended!", nr);
        return 0;
    }

    if (sscanf(line, " %ld %ld %ld ", t, t + 1, t + 2) != 3) {
        basic_mud_log("SYSERR: Format error in last line of mob #%d\n"
            "...expecting line of form '# # #'", nr);
        return 0;
    }

    auto sex = t[2];
    ch["appearances"].push_back(std::make_pair(CharAppearance::Sex, t[2]));

    set_height_and_weight_by_race(ch, sex);

    return 1;
}


/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 *
 * CASE		: Requires a parameter through 'value'.
 * BOOL_CASE	: Being specified at all is its value.
 */

#define CASE(test)    \
    if (value && !matched && !strcasecmp(keyword, test) && (matched = false))

#define BOOL_CASE(test)    \
    if (!value && !matched && !strcasecmp(keyword, test) && (matched = true))

#define RANGE(low, high)    \
    (num_arg = MAX((low), MIN((high), (num_arg))))

static void interpret_espec(const char *keyword, const char *value, nlohmann::json& ch, mob_vnum nr) {

}

#undef CASE
#undef BOOL_CASE
#undef RANGE

static void parse_espec(char *buf, nlohmann::json& ch, mob_vnum nr) {
    char *ptr;

    if ((ptr = strchr(buf, ':')) != nullptr) {
        *(ptr++) = '\0';
        while (isspace(*ptr))
            ptr++;
    }
    interpret_espec(buf, ptr, ch, nr);
}

static void mob_stats(nlohmann::json& mob) {
    auto level = mob["level"].get<int>();
    int start = level * 0.5, finish = level;

    if (finish < 20)
        finish = 20;

    std::unordered_map<CharAttribute, int> setTo;

    auto race = mob["race"].get<RaceID>();

    if ((race == RaceID::Serpent || race == RaceID::Animal)) {
        setTo[CharAttribute::Strength] = rand_number(start, finish);
        setTo[CharAttribute::Intelligence] = rand_number(start, finish) - 30;
        setTo[CharAttribute::Wisdom] = rand_number(start, finish) - 30;
        setTo[CharAttribute::Agility] = rand_number(start + 5, finish);
        setTo[CharAttribute::Constitution] = rand_number(start + 5, finish);
        setTo[CharAttribute::Speed] = rand_number(start, finish);
    } else {
        if (race == RaceID::Saiyan) {
            setTo[CharAttribute::Strength] = rand_number(start + 10, finish);
            setTo[CharAttribute::Intelligence] = rand_number(start, finish - 10);
            setTo[CharAttribute::Wisdom] = rand_number(start, finish - 5);
            setTo[CharAttribute::Agility] = rand_number(start, finish);
            setTo[CharAttribute::Constitution] = rand_number(start + 5, finish);
            setTo[CharAttribute::Speed] = rand_number(start + 5, finish);
        } else if (race == RaceID::Konatsu) {
            setTo[CharAttribute::Strength] = rand_number(start, finish - 10);
            setTo[CharAttribute::Intelligence] = rand_number(start, finish);
            setTo[CharAttribute::Wisdom] = rand_number(start, finish);
            setTo[CharAttribute::Agility] = rand_number(start + 10, finish);
            setTo[CharAttribute::Constitution] = rand_number(start, finish);
            setTo[CharAttribute::Speed] = rand_number(start, finish);
        } else if (race == RaceID::Android) {
            setTo[CharAttribute::Strength] = rand_number(start, finish);
            setTo[CharAttribute::Intelligence] = rand_number(start, finish);
            setTo[CharAttribute::Wisdom] = rand_number(start, finish - 10);
            setTo[CharAttribute::Agility] = rand_number(start, finish);
            setTo[CharAttribute::Constitution] = rand_number(start, finish);
            setTo[CharAttribute::Speed] = rand_number(start, finish);
        } else if (race == RaceID::Majin) {
            setTo[CharAttribute::Strength] = rand_number(start, finish);
            setTo[CharAttribute::Intelligence] = rand_number(start, finish - 10);
            setTo[CharAttribute::Wisdom] = rand_number(start, finish - 5);
            setTo[CharAttribute::Agility] = rand_number(start, finish);
            setTo[CharAttribute::Constitution] = rand_number(start + 15, finish);
            setTo[CharAttribute::Speed] = rand_number(start, finish);
        } else if (race == RaceID::Tuffle) {
            setTo[CharAttribute::Strength] = rand_number(start, finish - 10);
            setTo[CharAttribute::Intelligence] = rand_number(start + 15, finish);
            setTo[CharAttribute::Wisdom] = rand_number(start, finish);
            setTo[CharAttribute::Agility] = rand_number(start, finish);
            setTo[CharAttribute::Constitution] = rand_number(start, finish);
            setTo[CharAttribute::Speed] = rand_number(start, finish);
        } else if (race == RaceID::Icer) {
            setTo[CharAttribute::Strength] = rand_number(start + 5, finish);
            setTo[CharAttribute::Intelligence] = rand_number(start, finish);
            setTo[CharAttribute::Wisdom] = rand_number(start, finish);
            setTo[CharAttribute::Agility] = rand_number(start, finish);
            setTo[CharAttribute::Constitution] = rand_number(start, finish);
            setTo[CharAttribute::Speed] = rand_number(start + 10, finish);
        } else {
            setTo[CharAttribute::Strength] = rand_number(start, finish);
            setTo[CharAttribute::Intelligence] = rand_number(start, finish);
            setTo[CharAttribute::Wisdom] = rand_number(start, finish);
            setTo[CharAttribute::Agility] = rand_number(start, finish);
            setTo[CharAttribute::Constitution] = rand_number(start, finish);
            setTo[CharAttribute::Speed] = rand_number(start, finish);
        }
    }

    for(auto &[attr, val] : setTo) {
        if(val > 100) {
            val = 100;
        } else if(val < 5) {
            val = rand_number(5, 8);
        }
        mob["attributes"].push_back(std::make_pair(attr, val));
    }
}

static int parse_enhanced_mob(FILE *mob_f, nlohmann::json& ch, mob_vnum nr) {
    char line[READ_SIZE];

    parse_simple_mob(mob_f, ch, nr);

    while (get_line(mob_f, line)) {
        if (!strcmp(line, "E"))    /* end of the enhanced section */
            return 1;
        else if (*line == '#') {    /* we've hit the next mob, maybe? */
            basic_mud_log("SYSERR: Unterminated E section in mob #%d", nr);
            return 0;
        } else
            parse_espec(line, ch, nr);
    }

    basic_mud_log("SYSERR: Unexpected end of file reached after mob #%d", nr);
    return 0;
}

static int parse_mobile_from_file(FILE *mob_f, nlohmann::json& ch) {
    int j, t[10], retval;
    char line[READ_SIZE], *tmpptr, letter;
    char f1[128], f2[128], f3[128], f4[128], f5[128], f6[128];
    char f7[128], f8[128], buf2[128];
    mob_vnum nr = ch["vn"];
    auto &z = zone_table[real_zone_by_thing(nr)];
    z.mobiles.insert(nr);

    /*
   * Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg 2/25/98
   */

    sprintf(buf2, "mob vnum %d", nr);   /* sprintf: OK (for 'buf2 >= 19') */

    /***** String data *****/
    ch["strings"]["name"] = fread_string(mob_f, buf2);
    tmpptr = fread_string(mob_f, buf2);
    if (tmpptr && *tmpptr) {
        if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
            !strcasecmp(fname(tmpptr), "the"))
            *tmpptr = LOWER(*tmpptr);
            ch["strings"]["short_description"] = tmpptr;
            free(tmpptr);
    }
    ch["strings"]["room_description"] = fread_string(mob_f, buf2);
    ch["strings"]["look_description"] = fread_string(mob_f, buf2);

    /* *** Numeric data *** */
    if (!get_line(mob_f, line)) {
        basic_mud_log("SYSERR: Format error after string section of mob #%d\n"
            "...expecting line of form '# # # {S | E}', but file ended!", nr);
        return 0;
    }

    if ((retval = sscanf(line, "%s %s %s %s %s %s %s %s %d %c", f1, f2, f3, f4, f5, f6, f7, f8, t + 2, &letter)) == 10) {
        int taeller;

        bitvector_t mf[4], aff[4];

        std::set<int> flags;

        mf[0] = asciiflag_conv(f1);
        mf[1] = asciiflag_conv(f2);
        mf[2] = asciiflag_conv(f3);
        mf[3] = asciiflag_conv(f4);

        for(auto i = 0; i < NUM_MOB_FLAGS; i++) if(IS_SET_AR(mf, i)) flags.insert(i);
        flags.insert(MOB_ISNPC);
        flags.erase(MOB_NOTDEADYET);
        if(!flags.empty()) {
            ch["flags"].push_back(std::make_pair(FlagType::NPC, std::vector<int>(flags.begin(), flags.end())));
            flags.clear();
        }

        aff[0] = asciiflag_conv(f5);
        aff[1] = asciiflag_conv(f6);
        aff[2] = asciiflag_conv(f7);
        aff[3] = asciiflag_conv(f8);
        for(auto i = 0; i < NUM_AFF_FLAGS; i++) if(IS_SET_AR(aff, i)) flags.insert(i);
        if(!flags.empty()) {
            ch["flags"].push_back(std::make_pair(FlagType::Affect, std::vector<int>(flags.begin(), flags.end())));
            flags.clear();
        }
        ch["aligns"].push_back(std::make_pair(CharAlign::GoodEvil, t[2]));

    } else {
        basic_mud_log("SYSERR: Format error after string section of mob #%d\n"
            "...expecting line of form '# # # {S | E}'", nr);
        exit(1);
    }

    switch (UPPER(letter)) {
        case 'S':    /* Simple monsters */
            parse_simple_mob(mob_f, ch, nr);
            break;
        case 'E':    /* Circle3 Enhanced monsters */
            parse_enhanced_mob(mob_f, ch, nr);
            mob_stats(ch);
            break;
            /* add new mob types here.. */
        default:
            basic_mud_log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
            exit(1);
    }

    /* DG triggers -- script info follows mob S/E section */
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
    while (letter == 'T') {
        char junk[248];
        char tline[248];
        trig_vnum tvn;
        get_line(mob_f, tline);
        auto count = sscanf(line, "%7s %d", junk, &tvn);
        if(trig_index.contains(tvn)) ch["proto_script"].push_back(tvn);
        letter = fread_letter(mob_f);
        ungetc(letter, mob_f);
    }

    /* Uncomment to force all mob files to be rewritten. Good for initial AUTOBALANCE setup.
   * if (bitsavetodisk) {
   *   add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 0);
   *   converting = TRUE;
   * } */

    return 1;
}


static void parse_mobile(FILE *mob_f, mob_vnum nr) {
    auto &idx = mob_index[nr];
    idx.vn = nr;

    auto mj = mob_proto.emplace(nr, nlohmann::json::object());
    auto &j = mj.first->second;
    j["vn"] = nr;

    if (parse_mobile_from_file(mob_f, j)) {

    } else { /* We used to exit in the file reading code, but now we do it here */
        exit(1);
    }
}


/* read all objects from obj file; generate index and prototypes */
static char *parse_object(FILE *obj_f, obj_vnum nr) {
    static char line[READ_SIZE];
    int64_t t[NUM_OBJ_VAL_POSITIONS + 2], retval;
    char *tmpptr, buf2[128];
    char f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];
    char f5[READ_SIZE], f6[READ_SIZE], f7[READ_SIZE], f8[READ_SIZE];
    char f9[READ_SIZE], f10[READ_SIZE], f11[READ_SIZE], f12[READ_SIZE];

    auto &idx = obj_index[nr];
    auto oi = obj_proto.emplace(nr, nlohmann::json::object());
    auto &obj = oi.first->second;
    
    idx.vn = nr;
    obj["vn"] = nr;

    sprintf(buf2, "object #%d", nr);    /* sprintf: OK (for 'buf2 >= 19') */
    char* tempStr = fread_string(obj_f, buf2);
    if(tempStr != nullptr) {
        obj["strings"]["name"] = std::string(tempStr);
    }

    auto &z = zone_table[real_zone_by_thing(nr)];
    z.objects.insert(nr);

    tmpptr = fread_string(obj_f, buf2);
    if (tmpptr && *tmpptr) {
        if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
            !strcasecmp(fname(tmpptr), "the"))
            *tmpptr = LOWER(*tmpptr);
            obj["strings"]["short_description"] = tmpptr;
            free(tmpptr);
    }
    tmpptr = fread_string(obj_f, buf2);
    if (tmpptr && *tmpptr) {
        CAP(tmpptr);
        obj["strings"]["room_description"] = tmpptr;
        free(tmpptr);
    }
    tmpptr = fread_string(obj_f, buf2);
    if(tmpptr && *tmpptr) {
        obj["strings"]["look_description"] = tmpptr;
        free(tmpptr);
    }

    /* *** numeric data *** */
    if (!get_line(obj_f, line)) {
        basic_mud_log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
        exit(1);
    }
    if ((retval = sscanf(line, " %d %s %s %s %s %s %s %s %s %s %s %s %s", t, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10,
                                f11, f12)) == 13) {
        bitvector_t extraFlags[4], wearFlags[4], permFlags[4];

        std::set<int> flags;

        extraFlags[0] = asciiflag_conv(f1);
        extraFlags[1] = asciiflag_conv(f2);
        extraFlags[2] = asciiflag_conv(f3);
        extraFlags[3] = asciiflag_conv(f4);
        
        for(auto i = 0; i < NUM_ITEM_FLAGS; i++) {
            if(IS_SET_AR(extraFlags, i)) flags.insert(i);
        };
        if(!flags.empty()) {
            obj["flags"].push_back(std::make_pair(FlagType::Item, std::vector<int>(flags.begin(), flags.end())));
            flags.clear();
        }

        wearFlags[0] = asciiflag_conv(f5);
        wearFlags[1] = asciiflag_conv(f6);
        wearFlags[2] = asciiflag_conv(f7);
        wearFlags[3] = asciiflag_conv(f8);
        for(auto i = 0; i < NUM_ITEM_WEARS; i++) if(IS_SET_AR(wearFlags, i)) flags.insert(i);
        if(!flags.empty()) {
            obj["flags"].push_back(std::make_pair(FlagType::Wear, std::vector<int>(flags.begin(), flags.end())));
            flags.clear();
        }

        permFlags[0] = asciiflag_conv(f9);
        permFlags[1] = asciiflag_conv(f10);
        permFlags[2] = asciiflag_conv(f11);
        permFlags[3] = asciiflag_conv(f12);
        for(auto i = 0; i < NUM_AFF_FLAGS; i++) if(IS_SET_AR(permFlags, i)) flags.insert(i);
        if(!flags.empty()) {
            obj["flags"].push_back(std::make_pair(FlagType::Affect, std::vector<int>(flags.begin(), flags.end())));
            flags.clear();
        }

    } else {
        basic_mud_log("SYSERR: Format error in first numeric line (expecting 13 args, got %d), %s", retval, buf2);
        exit(1);
    }

    /* Object flags checked in check_object(). */
    auto obj_type = t[0];
    obj["type_flag"] = obj_type;

    if (!get_line(obj_f, line)) {
        basic_mud_log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
        exit(1);
    }

    for (auto i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
        t[i] = 0;

    if ((retval = sscanf(line, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", t, t + 1, t + 2, t + 3, t + 4, t + 5,
                         t + 6, t + 7, t + 8, t + 9, t + 10, t + 11, t + 12, t + 13, t + 14, t + 15)) >
        NUM_OBJ_VAL_POSITIONS) {
        basic_mud_log("SYSERR: Format error in second numeric line (expecting <=%d args, got %d), %s", NUM_OBJ_VAL_POSITIONS,
            retval, buf2);
        exit(1);
    }

    std::array<int64_t, NUM_OBJ_VAL_POSITIONS> values{};

    for (auto i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) if(t[i] != 0) 
        values[i] = t[i];

    if ((obj_type == ITEM_PORTAL || \
       obj_type == ITEM_HATCH) && \
       (!values[VAL_DOOR_DCLOCK] || \
        !values[VAL_DOOR_DCHIDE])) {
        values[VAL_DOOR_DCLOCK] = 20;
        values[VAL_DOOR_DCHIDE] = 20;

    }

    if (!get_line(obj_f, line)) {
        basic_mud_log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
        exit(1);
    }
    if ((retval = sscanf(line, "%ld %ld %ld %ld", t, t + 1, t + 2, t + 3)) != 4) {
        if (retval == 3)
            t[3] = 0;
        else {
            basic_mud_log("SYSERR: Format error in third numeric line (expecting 4 args, got %d), %s", retval, buf2);
            exit(1);
        }
    }
    auto weight = t[0];
    obj["weight"] = weight;
    obj["cost"] = t[1];
    obj["cost_per_day"] = t[2];
    obj["level"] = t[3];
    obj["size"] = SIZE_MEDIUM;

    /* check to make sure that weight of containers exceeds curr. quantity */
    if (obj_type == ITEM_DRINKCON ||
        obj_type == ITEM_FOUNTAIN) {
        if (weight < values[1])
            obj["weight"] = values[1] + 5;
    }
    /* *** make sure portal objects have their timer set correctly *** */
    if (obj_type == ITEM_PORTAL) {
        obj["timer"] = -1;
    }

    /* *** extra descriptions and affect fields *** */
    for(auto i = 0; i < values.size(); i++) {
        if(values[i] != 0) {
            obj["values"].push_back(std::make_pair(i, values[i]));
        }
    }

    strcat(buf2, ", after numeric constants\n"    /* strcat: OK (for 'buf2 >= 87') */
                 "...expecting 'E', 'A', '$', or next object number");

    for (;;) {
        if (!get_line(obj_f, line)) {
            basic_mud_log("SYSERR: Format error in %s", buf2);
            exit(1);
        }
        switch (*line) {
            case 'E': {
                nlohmann::json ex;
                ex["keyword"] = fread_string(obj_f, buf2);
                ex["description"] = fread_string(obj_f, buf2);
                obj["ex_description"].push_back(ex);
            }
                break;
            case 'A': {
                t[1] = 0;
                if (!get_line(obj_f, line)) {
                    basic_mud_log("SYSERR: Format error in 'A' field, %s\n"
                        "...expecting 2 numeric constants but file ended!", buf2);
                    exit(1);
                }
                if ((retval = sscanf(line, " %ld %ld %ld ", t, t + 1, t + 2)) != 3) {
                    if (retval != 2) {
                        basic_mud_log("SYSERR: Format error in 'A' field, %s\n"
                            "...expecting 2 numeric arguments, got %d\n"
                            "...offending line: '%s'", buf2, retval, line);
                        exit(1);
                    }
                }

                obj_affected_type af;
                
                af.location = t[0];
                af.modifier = t[1];
                af.specific = t[2];
                obj["affected"].push_back(af.serialize());
            }
                break;
            case 'S':  /* Spells for Spellbooks*/
                if (!get_line(obj_f, line)) {
                    basic_mud_log("SYSERR: Format error in 'S' field, %s\n"
                        "...expecting 2 numeric constants but file ended!", buf2);
                    exit(1);
                }
                if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
                    basic_mud_log("SYSERR: Format error in 'S' field, %s\n"
                        "...expecting 2 numeric arguments, got %d\n"
                        "...offending line: '%s'", buf2, retval, line);
                    exit(1);
                }
                break;
            case 'T':  /* DG triggers */
                {
                    char junk[8];
                    int vnum, count;
                    count = sscanf(line, "%s %d", junk, &vnum);
                    obj["proto_script"].push_back(vnum);
                }
                break;
            case 'Z':
                if (!get_line(obj_f, line)) {
                    basic_mud_log("SYSERR: Format error in 'Z' field, %s\n"
                        "...expecting numeric constant but file ended!", buf2);
                    exit(1);
                }
                if (sscanf(line, "%d", t) != 1) {
                    basic_mud_log("SYSERR: Format error in 'Z' field, %s\n"
                        "...expecting numeric argument\n"
                        "...offending line: '%s'", buf2, line);
                    exit(1);
                }
                obj["size"] = t[0];
                break;
            case '$':
            case '#':
                return (line);
            default:
                basic_mud_log("SYSERR: Format error in (%c): %s", *line, buf2);
                exit(1);
        }
    }
    return line;
}

static int count_alias_records(FILE *fl) {
    char key[READ_SIZE], next_key[READ_SIZE];
    char line[READ_SIZE], *scan;
    int total_keywords = 0;

    /* get the first keyword line */
    get_one_line(fl, key);

    while (*key != '$') {
        /* skip the text */
        do {
            get_one_line(fl, line);
            if (feof(fl))
                goto ackeof;
        } while (*line != '#');

        /* now count keywords */
        scan = key;
        do {
            scan = one_word(scan, next_key);
            if (*next_key)
                ++total_keywords;
        } while (*next_key);

        /* get next keyword line (or $) */
        get_one_line(fl, key);

        if (feof(fl))
            goto ackeof;
    }

    return (total_keywords);

    /* No, they are not evil. -gg 6/24/98 */
    ackeof:
    basic_mud_log("SYSERR: Unexpected end of help file.");
    exit(1);    /* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
static int count_hash_records(FILE *fl) {
    char buf[128];
    int count = 0;

    while (fgets(buf, 128, fl))
        if (*buf == '#')
            count++;

    return (count);
}

/* load the zone table and command tables */
static void load_zones(FILE *fl, char *zonename) {
    int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error, arg_num, version = 1;
    char *ptr, buf[READ_SIZE], zname[READ_SIZE], buf2[MAX_STRING_LENGTH];
    int zone_fix = false;
    char t1[80], t2[80], line[MAX_STRING_LENGTH];

    strlcpy(zname, zonename, sizeof(zname));

    line_num += get_line(fl, buf);

    if (*buf == '@') {
        if (sscanf(buf, "@Version: %d", &version) != 1) {
            basic_mud_log("SYSERR: Format error in %s (version)", zname);
            basic_mud_log("SYSERR: ...Line: %s", line);
            exit(1);
        }
        line_num += get_line(fl, buf);
    }
    zone_vnum v;

    if (sscanf(buf, "#%hd", &v) != 1) {
        basic_mud_log("SYSERR: FFFFFF Format error in %s, line %d", zname, line_num);
        exit(1);
    }
    snprintf(buf2, sizeof(buf2)-1, "beginning of zone #%d", v);

    auto &z = zone_table[v];
    z.number = v;

    line_num += get_line(fl, buf);
    if ((ptr = strchr(buf, '~')) != nullptr)    /* take off the '~' if it's there */
        *ptr = '\0';
    z.builders = strdup(buf);

    line_num += get_line(fl, buf);
    if ((ptr = strchr(buf, '~')) != nullptr)    /* take off the '~' if it's there */
        *ptr = '\0';
    z.name = strdup(buf);

    line_num += get_line(fl, buf);
    if (version >= 2) {

        char zbuf1[MAX_STRING_LENGTH];
        char zbuf2[MAX_STRING_LENGTH];
        char zbuf3[MAX_STRING_LENGTH];
        char zbuf4[MAX_STRING_LENGTH];

        if (sscanf(buf, " %hd %hd %d %d %s %s %s %s %d %d", &z.bot, &z.top, &z.lifespan,
                   &z.reset_mode, zbuf1, zbuf2, zbuf3, zbuf4, &z.min_level, &z.max_level) != 10) {
            basic_mud_log("SYSERR: Format error in 10-constant line of %s", zname);
            exit(1);
        }

        z.zone_flags[0] = asciiflag_conv(zbuf1);
        z.zone_flags[1] = asciiflag_conv(zbuf2);
        z.zone_flags[2] = asciiflag_conv(zbuf3);
        z.zone_flags[3] = asciiflag_conv(zbuf4);

    } else if (sscanf(buf, " %hd %hd %d %d ", &z.bot, &z.top, &z.lifespan, &z.reset_mode) != 4) {
        /*
     * This may be due to the fact that the zone has no builder.  So, we just attempt
     * to fix this by copying the previous 2 last reads into this variable and the
     * last one.
     */
        basic_mud_log("SYSERR: Format error in numeric constant line of %s, attempting to fix.", zname);
        if (sscanf(z.name, " %hd %hd %d %d ", &z.bot, &z.top, &z.lifespan, &z.reset_mode) != 4) {
            basic_mud_log("SYSERR: Could not fix previous error, aborting game.");
            exit(1);
        } else {
            free(z.name);
            z.name = strdup(z.builders);
            free(z.builders);
            z.builders = strdup("None.");
            zone_fix = true;
        }
    }
    if (z.bot > z.top) {
        basic_mud_log("SYSERR: Zone %d bottom (%d) > top (%d).", z.number, z.bot, z.top);
        exit(1);
    }

    for (auto c = 0;true;c++) {
        get_line(fl, buf);
        if(buf[0] == '*') continue;

        if(strchr("$S", buf[0])) {
            break;
        }

        auto &zc = z.cmd.emplace_back();
        zc.command = buf[0];

        if(zc.command == 'V') { /* a string-arg command */
            if (sscanf(&buf[1], " %d %d %d %d %d %d %79s %79[^\f\r\n\t\v]", &tmp,&zc.arg1, &zc.arg2, &zc.arg3,
                       &zc.arg4, &zc.arg5, t1, t2) != 8)
                error = 1;
            else {
                zc.sarg1 = t1;
                zc.sarg2 = t2;
            }
        } else {
            if ((arg_num = sscanf(&buf[1], " %d %d %d %d %d %d ", &tmp, &zc.arg1, &zc.arg2, &zc.arg3, &zc.arg4,
                                  &zc.arg5)) != 6) {
                if (arg_num != 5) {
                    error = 1;
                } else {
                    zc.arg5 = 0;
                }
            }
        }

        zc.if_flag = tmp;

        if (error) {
            basic_mud_log("SYSERR: Format error in %s, line %d: '%s'", zname, c, buf);
            exit(1);
        }
        zc.line = c;
    }
}

static void discrete_load(FILE *fl, int mode, char *filename) {
    int nr = -1, last;
    char line[READ_SIZE];

    const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg"};
    /* modes positions correspond to DB_BOOT_xxx in db.h */

    for (;;) {
        /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
        if (mode != DB_BOOT_OBJ || nr < 0)
            if (!get_line(fl, line)) {
                if (nr == -1) {
                    basic_mud_log("SYSERR: %s file %s is empty!", modes[mode], filename);
                } else {
                    basic_mud_log("SYSERR: Format error in %s after %s #%d\n"
                        "...expecting a new %s, but file ended!\n"
                        "(maybe the file is not terminated with '$'?)", filename,
                        modes[mode], nr, modes[mode]);
                }
                exit(1);
            }
        if (*line == '$')
            return;

        if (*line == '#') {
            last = nr;
            if (sscanf(line, "#%d", &nr) != 1) {
                basic_mud_log("SYSERR: Format error after %s #%d", modes[mode], last);
                exit(1);
            }
            if (nr >= 99999)
                return;
            else
                switch (mode) {
                    case DB_BOOT_WLD:
                        parse_room(fl, nr);
                        break;
                    case DB_BOOT_MOB:
                        parse_mobile(fl, nr);
                        break;
                    case DB_BOOT_TRG:
                        parse_trigger(fl, nr);
                        break;
                    case DB_BOOT_OBJ:
                        strlcpy(line, parse_object(fl, nr), sizeof(line));
                        break;
                }
        } else {
            basic_mud_log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
                filename, modes[mode], nr);
            basic_mud_log("SYSERR: ... offending line: '%s'", line);
            exit(1);
        }
    }
}


static void index_boot(int mode) {
    const char *index_filename, *prefix = nullptr;    /* nullptr or egcs 1.1 complains */
    FILE *db_index, *db_file;
    int rec_count = 0, size[2];
    char buf2[PATH_MAX], buf1[MAX_STRING_LENGTH];

    switch (mode) {
        case DB_BOOT_WLD:
            prefix = WLD_PREFIX;
            break;
        case DB_BOOT_MOB:
            prefix = MOB_PREFIX;
            break;
        case DB_BOOT_OBJ:
            prefix = OBJ_PREFIX;
            break;
        case DB_BOOT_ZON:
            prefix = ZON_PREFIX;
            break;
        case DB_BOOT_SHP:
            prefix = SHP_PREFIX;
            break;
        case DB_BOOT_HLP:
            prefix = HLP_PREFIX;
            break;
        case DB_BOOT_TRG:
            prefix = TRG_PREFIX;
            break;
        case DB_BOOT_GLD:
            prefix = GLD_PREFIX;
            break;
        default:
            basic_mud_log("SYSERR: Unknown subcommand %d to index_boot!", mode);
            exit(1);
    }

    if (mini_mud)
        index_filename = MINDEX_FILE;
    else
        index_filename = INDEX_FILE;

    snprintf(buf2, sizeof(buf2), "%s%s", prefix, index_filename);
    if (!(db_index = fopen(buf2, "r"))) {
        basic_mud_log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
        exit(1);
    }

    /* first, count the number of records in the file so we can malloc */
    fscanf(db_index, "%s\n", buf1);
    while (*buf1 != '$') {
        snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
        if (!(db_file = fopen(buf2, "r"))) {
            basic_mud_log("SYSERR: File '%s' listed in '%s%s': %s", buf2, prefix,
                index_filename, strerror(errno));
            fscanf(db_index, "%s\n", buf1);
            continue;
        } else {
            if (mode == DB_BOOT_ZON)
                rec_count++;
            else if (mode == DB_BOOT_HLP)
                rec_count += count_alias_records(db_file);
            else
                rec_count += count_hash_records(db_file);
        }

        fclose(db_file);
        fscanf(db_index, "%s\n", buf1);
    }

    /* Exit if 0 records, unless this is shops */
    if (!rec_count) {
        if (mode == DB_BOOT_SHP || mode == DB_BOOT_GLD)
            return;
        basic_mud_log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
            index_filename);
        exit(1);
    }

    /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
    switch (mode) {
        case DB_BOOT_TRG:
            break;
        case DB_BOOT_WLD:
            size[0] = sizeof(Room) * rec_count;
            basic_mud_log("   %d rooms, %d bytes.", rec_count, size[0]);
            break;
        case DB_BOOT_MOB:
            size[0] = sizeof(struct index_data) * rec_count;
            size[1] = sizeof(BaseCharacter) * rec_count;
            basic_mud_log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
            break;
        case DB_BOOT_OBJ:
            size[0] = sizeof(struct index_data) * rec_count;
            size[1] = sizeof(Object) * rec_count;
            basic_mud_log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
            break;
        case DB_BOOT_ZON:
            size[0] = sizeof(struct zone_data) * rec_count;
            basic_mud_log("   %d zones, %d bytes.", rec_count, size[0]);
            break;
        case DB_BOOT_HLP:
            CREATE(help_table, struct help_index_element, rec_count);
            size[0] = sizeof(struct help_index_element) * rec_count;
            basic_mud_log("   %d entries, %d bytes.", rec_count, size[0]);
            break;
    }

    rewind(db_index);
    fscanf(db_index, "%s\n", buf1);
    while (*buf1 != '$') {
        snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
        if (!(db_file = fopen(buf2, "r"))) {
            basic_mud_log("SYSERR: %s: %s", buf2, strerror(errno));
            exit(1);
        }
        switch (mode) {
            case DB_BOOT_WLD:
            case DB_BOOT_OBJ:
            case DB_BOOT_MOB:
            case DB_BOOT_TRG:
                discrete_load(db_file, mode, buf2);
                break;
            case DB_BOOT_ZON:
                load_zones(db_file, buf2);
                break;
            case DB_BOOT_HLP:
                load_help(db_file, buf2);
                break;
            case DB_BOOT_SHP:
                boot_the_shops(db_file, buf2, rec_count);
                break;
            case DB_BOOT_GLD:
                boot_the_guilds(db_file, buf2, rec_count);
                break;
        }

        fclose(db_file);
        fscanf(db_index, "%s\n", buf1);
    }
    fclose(db_index);
}


/* remove ^M's from file output */
/* There may be a similar function in Oasis (and I'm sure
   it's part of obuild).  Remove this if you get a
   multiple definition error or if it you want to use a
   substitute
*/
static void kill_ems(char *str) {
    char *ptr1, *ptr2, *tmp;

    tmp = str;
    ptr1 = str;
    ptr2 = str;

    while (*ptr1) {
        if ((*(ptr2++) = *(ptr1++)) == '\r')
            if (*ptr1 == '\r')
                ptr1++;
    }
    *ptr2 = '\0';
}


/* Separate a 4-character id tag from the data it precedes */
static void tag_argument(char *argument, char *tag) {
    char *tmp = argument, *ttag = tag, *wrt = argument;
    int i;

    for (i = 0; i < 4; i++)
        *(ttag++) = *(tmp++);
    *ttag = '\0';

    while (*tmp == ':' || *tmp == ' ')
        tmp++;

    while (*tmp)
        *(wrt++) = *(tmp++);
    *wrt = '\0';
}


static void load_affects(FILE *fl, BaseCharacter *ch, int violence) {
    int num, num2, num3, num4, num5, num6, i;
    char line[MAX_INPUT_LENGTH + 1];
    struct affected_type af;

    i = 0;
    do {
        get_line(fl, line);
        num = num2 = num3 = num4 = num5 = num6 = 0;
        sscanf(line, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
        if (num != 0) {
            af.type = num;
            af.duration = num2;
            af.modifier = num3;
            af.location = num4;
            af.bitvector = num5;
            af.specific = num6;
            if (violence)
                affectv_to_char(ch, &af);
            else
                affect_to_char(ch, &af);
            i++;
        }
    } while (num != 0);
}


static void load_skills(FILE *fl, BaseCharacter *ch, bool mods) {
    int num = 0, num2 = 0, num3 = 0;
    char line[MAX_INPUT_LENGTH + 1];

    do {
        get_line(fl, line);
        sscanf(line, "%d %d %d", &num, &num2, &num3);
        if (num != 0) {
            if (mods) {
                SET_SKILL_BONUS(ch, num, num2);
            } else {
                SET_SKILL(ch, num, num2);
            }
            SET_SKILL_PERF(ch, num, num3);
        }
    } while (num != 0);
}

static void load_bonuses(FILE *fl, BaseCharacter *ch, bool mods) {
    int num[52] = {0}, i;
    char line[MAX_INPUT_LENGTH + 1];

    get_line(fl, line);
    sscanf(line,
           "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
           &num[0], &num[1], &num[2], &num[3], &num[4], &num[5], &num[6], &num[7], &num[8], &num[9], &num[10], &num[11],
           &num[12], &num[13], &num[14], &num[15], &num[16], &num[17], &num[18], &num[19], &num[20], &num[21], &num[22],
           &num[23], &num[24], &num[25], &num[26], &num[27], &num[28], &num[29], &num[30], &num[31], &num[32], &num[33],
           &num[34], &num[35], &num[36], &num[37], &num[38], &num[39], &num[40], &num[41], &num[42], &num[43], &num[44],
           &num[45], &num[46], &num[47], &num[48], &num[49], &num[50], &num[51]);
    for (i = 0; i < 52; i++) {
        if (num[i] > 0) {
            GET_BONUS(ch, i) = num[i];
        }
    }
}

#define LOAD_HIT    0
#define LOAD_MANA    1
#define LOAD_MOVE    2
#define LOAD_KI        3
#define LOAD_LIFE       4

static void load_HMVS(BaseCharacter *ch, const char *line, int mode) {
    int64_t num = 0, num2 = 0;

    sscanf(line, "%" I64T "/%" I64T "", &num, &num2);

    switch (mode) {
        case LOAD_HIT:
            //GET_HIT(ch) = num;
            break;

        case LOAD_MANA:
            //GET_MANA(ch) = num;
            break;

        case LOAD_MOVE:
            //GET_MOVE(ch) = num;
            break;

        case LOAD_KI:
            //GET_KI(ch) = num;
            break;
    }
}

static void load_BASE(BaseCharacter *ch, const char *line, int mode) {
    int64_t num = 0;

    sscanf(line, "%" I64T "", &num);

    switch (mode) {
        case LOAD_HIT:
            ch->set(CharStat::PowerLevel, num);
            break;

        case LOAD_MANA:
            ch->set(CharStat::Ki, num);
            break;

        case LOAD_MOVE:
            ch->set(CharStat::Stamina, num);
            break;

        case LOAD_LIFE:
            //GET_LIFEFORCE(ch) = num;
            break;
    }
}

static void load_majin(BaseCharacter *ch, const char *line) {
    int64_t num = 0;

    sscanf(line, "%" I64T "", &num);
    GET_MAJINIZED(ch) = num;

}

static void load_molt(BaseCharacter *ch, const char *line) {
    int64_t num = 0;

    sscanf(line, "%" I64T "", &num);
    GET_MOLT_EXP(ch) = num;

}

/* new load_char reads ASCII Player Files */
/* Load a char, TRUE if loaded, FALSE if not */
static int load_char(const char *name, BaseCharacter *ch) {
    int id = 0, i, num = 0, num2 = 0, num3 = 0;
    FILE *fl = nullptr;
    char fname[READ_SIZE];
    char buf[128], buf2[128], line[MAX_INPUT_LENGTH], tag[6];
    char f1[128], f2[128], f3[128], f4[128];

    if (!get_filename(fname, sizeof(fname), PLR_FILE, name))
        return (-1);
    else {
        if (!(fl = fopen(fname, "r"))) {
            mudlog(NRM, ADMLVL_GOD, true, "SYSERR: Couldn't open player file %s", fname);
            return (-1);
        }

        /* character initializations */
        /* initializations necessary to keep some things straight */
        ch->affected = nullptr;
        ch->affectedv = nullptr;
        for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
            SET_SKILL(ch, i, 0);
            SET_SKILL_BONUS(ch, i, 0);
            SET_SKILL_PERF(ch, i, 0);
        }

        GET_LOG_USER(ch) = strdup("NOUSER");
        GET_SUPPRESS(ch) = PFDEF_SKIN;
        GET_FURY(ch) = PFDEF_HAIRL;
        GET_CLAN(ch) = strdup("None.");
        GET_HOME(ch) = PFDEF_HOMETOWN;
        GET_WEIGHT(ch) = PFDEF_WEIGHT;

        ch->health = 1.0;
        GET_RELAXCOUNT(ch) = PFDEF_EYE;
        GET_BLESSLVL(ch) = PFDEF_HEIGHT;
        ch->life = 1.0;
        // GET_LIFEFORCE(ch) = PFDEF_BASEPL;
        GET_LIFEPERC(ch) = PFDEF_WEIGHT;
        GET_STUPIDKISS(ch) = 0;
        GET_POS(ch) = POS_STANDING;
        GET_MAJINIZED(ch) = PFDEF_BASEPL;
        GET_GAUNTLET(ch) = PFDEF_GAUNTLET;
        ch->energy = 1.0;
        ch->stamina = 1.0;


        GET_LOADROOM(ch) = PFDEF_LOADROOM;
        GET_INVIS_LEV(ch) = PFDEF_INVISLEV;
        GET_FREEZE_LEV(ch) = PFDEF_FREEZELEV;
        GET_WIMP_LEV(ch) = PFDEF_WIMPLEV;

        GET_OLC_ZONE(ch) = PFDEF_OLC;

        ch->time.birth = ch->time.created = ch->time.maxage = 0;
        ch->followers = nullptr;

        while (get_line(fl, line)) {
            bitvector_t flags[4];
            tag_argument(line, tag);

            switch (*tag) {
                case 'A':
                    if (!strcmp(tag, "Ac  ")) ;
                    else if (!strcmp(tag, "Act ")) {
                        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
                        flags[0] = asciiflag_conv(f1);
                        flags[1] = asciiflag_conv(f2);
                        flags[2] = asciiflag_conv(f3);
                        flags[3] = asciiflag_conv(f4);
                        for(auto f = 0; f < NUM_PLR_FLAGS; f++) {
                            if(IS_SET_AR(flags, f)) ch->setFlag(FlagType::PC, f);
                        }
                    } else if (!strcmp(tag, "Aff ")) {
                        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
                        flags[0] = asciiflag_conv(f1);
                        flags[1] = asciiflag_conv(f2);
                        flags[2] = asciiflag_conv(f3);
                        flags[3] = asciiflag_conv(f4);
                        for(auto f = 0; f < NUM_AFF_FLAGS; f++) {
                            if(IS_SET_AR(flags, f)) ch->setFlag(FlagType::Affect, f);
                        }
                    } else if (!strcmp(tag, "Affs")) load_affects(fl, ch, 0);
                    else if (!strcmp(tag, "Affv")) load_affects(fl, ch, 1);
                    else if (!strcmp(tag, "AdmL")) ch->set(CharNum::AdmLevel, atoi(line));
                    else if (!strcmp(tag, "Abso")) GET_ABSORBS(ch) = atoi(line);
                    else if (!strcmp(tag, "AdmF")) {
                        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
                        flags[0] = asciiflag_conv(f1);
                        flags[1] = asciiflag_conv(f2);
                        flags[2] = asciiflag_conv(f3);
                        flags[3] = asciiflag_conv(f4);
                        for(auto f = 0; f < NUM_ADMFLAGS; f++) {
                            if(IS_SET_AR(flags, f)) ch->setFlag(FlagType::Admin, f);
                        }
                    } else if (!strcmp(tag, "Alin")) ch->set(CharAlign::GoodEvil, atoi(line));
                    else if (!strcmp(tag, "Aura")) ch->set(CharAppearance::Aura, atoi(line));
                    break;

                case 'B':
                    if (!strcmp(tag, "Bank")) ch->set(CharMoney::Bank, atoi(line));
                    else if (!strcmp(tag, "Bki ")) load_BASE(ch, line, LOAD_MANA);
                    else if (!strcmp(tag, "Blss")) GET_BLESSLVL(ch) = atoi(line);
                    else if (!strcmp(tag, "Boam")) GET_BOARD(ch, 0) = atoi(line);
                    else if (!strcmp(tag, "Boai")) GET_BOARD(ch, 1) = atoi(line);
                    else if (!strcmp(tag, "Boac")) GET_BOARD(ch, 2) = atoi(line);
                    else if (!strcmp(tag, "Boad")) GET_BOARD(ch, 3) = atoi(line);
                    else if (!strcmp(tag, "Boab")) GET_BOARD(ch, 4) = atoi(line);
                    else if (!strcmp(tag, "Bonu")) load_bonuses(fl, ch, false);
                    else if (!strcmp(tag, "Boos")) GET_BOOSTS(ch) = atoi(line);
                    else if (!strcmp(tag, "Bpl ")) load_BASE(ch, line, LOAD_HIT);
                    else if (!strcmp(tag, "Brth")) ch->time.birth = atol(line);
                    else if (!strcmp(tag, "Bst ")) load_BASE(ch, line, LOAD_MOVE);
                    break;

                case 'C':
                    if (!strcmp(tag, "Cha ")) ch->set(CharAttribute::Speed, atoi(line));
                    else if (!strcmp(tag, "Clan")) GET_CLAN(ch) = strdup(line);
                    else if (!strcmp(tag, "Clar")) GET_CRANK(ch) = atoi(line);
                    else if (!strcmp(tag, "Clas"))
                        ch->chclass = static_cast<SenseiID>(std::min(14, atoi(line)));
                    else if (!strcmp(tag, "Colr")) {
                        sscanf(line, "%d %s", &num, buf2);
                    } else if (!strcmp(tag, "Con ")) ch->set(CharAttribute::Constitution, atoi(line));
                    else if (!strcmp(tag, "Cool")) GET_COOLDOWN(ch) = atoi(line);
                    else if (!strcmp(tag, "Crtd")) ch->time.created = atol(line);
                    break;

                case 'D':
                    if (!strcmp(tag, "Deat")) GET_DTIME(ch) = atoi(line);
                    else if (!strcmp(tag, "Deac")) GET_DCOUNT(ch) = atoi(line);
                    else if (!strcmp(tag, "Desc")) ch->setLookDesc(fread_string(fl, buf2));
                    else if (!strcmp(tag, "Dex ")) ch->set(CharAttribute::Agility, atoi(line));
                    else if (!strcmp(tag, "Drnk")) GET_COND(ch, DRUNK) = atoi(line);
                    else if (!strcmp(tag, "Damg")) GET_DAMAGE_MOD(ch) = atoi(line);
                    else if (!strcmp(tag, "Droo")) GET_DROOM(ch) = atoi(line);
                    break;

                case 'E':
                    if (!strcmp(tag, "Exp ")) ch->exp = atoi(line);
                    else if (!strcmp(tag, "Eali")) ch->set(CharAlign::LawChaos, atoi(line));
                    else if (!strcmp(tag, "Ecls")) {

                    } else if (!strcmp(tag, "Eye ")) ch->set(CharAppearance::EyeColor, atoi(line));
                    break;

                case 'F':
                    if (!strcmp(tag, "Fisd")) GET_FISHD(ch) = atoi(line);
                    else if (!strcmp(tag, "Frez")) GET_FREEZE_LEV(ch) = atoi(line);
                    else if (!strcmp(tag, "Forc")) GET_FORGET_COUNT(ch) = atoi(line);
                    else if (!strcmp(tag, "Forg")) GET_FORGETING(ch) = atoi(line);
                    else if (!strcmp(tag, "Fury")) GET_FURY(ch) = atoi(line);
                    break;

                case 'G':
                    if (!strcmp(tag, "Gold")) ch->set(CharMoney::Carried, atoi(line));
                    else if (!strcmp(tag, "Gaun")) GET_GAUNTLET(ch) = atoi(line);
                    else if (!strcmp(tag, "Geno")) GET_GENOME(ch, 0) = atoi(line);
                    else if (!strcmp(tag, "Gen1")) GET_GENOME(ch, 1) = atoi(line);
                    break;

                case 'H':
                    if (!strcmp(tag, "Hit ")) load_HMVS(ch, line, LOAD_HIT);
                    else if (!strcmp(tag, "Hite")) ch->setHeight(atoi(line));
                    else if (!strcmp(tag, "Home")) GET_HOME(ch) = atoi(line);
                    else if (!strcmp(tag, "Host")) {}
                    else if (!strcmp(tag, "Hrc ")) ch->set(CharAppearance::HairColor, atoi(line));
                    else if (!strcmp(tag, "Hrl ")) ch->set(CharAppearance::HairLength, atoi(line));
                    else if (!strcmp(tag, "Hrs ")) ch->set(CharAppearance::HairStyle, atoi(line));
                    else if (!strcmp(tag, "Hung")) GET_COND(ch, HUNGER) = atoi(line);
                    break;

                case 'I':
                    if (!strcmp(tag, "Id  ")) GET_IDNUM(ch) = atol(line);
                    else if (!strcmp(tag, "INGl")) GET_INGESTLEARNED(ch) = atoi(line);
                    else if (!strcmp(tag, "Int ")) ch->set(CharAttribute::Intelligence, atoi(line));
                    else if (!strcmp(tag, "Invs")) GET_INVIS_LEV(ch) = atoi(line);
                    break;

                case 'K':
                    if (!strcmp(tag, "Ki  ")) load_HMVS(ch, line, LOAD_KI);
                    else if (!strcmp(tag, "Kaio")) GET_KAIOKEN(ch) = atoi(line);
                    break;

                case 'L':
                    if (!strcmp(tag, "Last")) ch->time.logon = atol(line);
                    else if (!strcmp(tag, "Lern")) ch->modPractices(atoi(line));
                    else if (!strcmp(tag, "Levl")) ch->set(CharNum::Level, atoi(line));
                    else if (!strcmp(tag, "LF  ")) load_BASE(ch, line, LOAD_LIFE);
                    else if (!strcmp(tag, "LFPC")) GET_LIFEPERC(ch) = atoi(line);
                    else if (!strcmp(tag, "Lila")) GET_LIMBCOND(ch, 1) = atoi(line);
                    else if (!strcmp(tag, "Lill")) GET_LIMBCOND(ch, 3) = atoi(line);
                    else if (!strcmp(tag, "Lira")) GET_LIMBCOND(ch, 0) = atoi(line);
                    else if (!strcmp(tag, "Lirl")) GET_LIMBCOND(ch, 2) = atoi(line);
                    else if (!strcmp(tag, "Lint")) GET_LINTEREST(ch) = atoi(line);
                    else if (!strcmp(tag, "Lpla")) GET_LPLAY(ch) = atoi(line);

                    break;

                case 'M':
                    if (!strcmp(tag, "Mana")) load_HMVS(ch, line, LOAD_MANA);
                    else if (!strcmp(tag, "Mexp")) load_molt(ch, line);
                    else if (!strcmp(tag, "Mlvl")) GET_MOLT_LEVEL(ch) = atoi(line);
                    else if (!strcmp(tag, "Move")) load_HMVS(ch, line, LOAD_MOVE);
                    else if (!strcmp(tag, "Mcls")) {

                    } else if (!strcmp(tag, "Maji")) MAJINIZED(ch) = atoi(line);
                    else if (!strcmp(tag, "Majm")) load_majin(ch, line);
                    else if (!strcmp(tag, "Mimi"))
                        ch->mimic = (RaceID)atoi(line);
                    else if (!strcmp(tag, "MxAg")) ch->time.maxage = atol(line);
                    break;

                case 'N':
                    if (!strcmp(tag, "Name")) ch->setName(line);
                    break;

                case 'O':
                    if (!strcmp(tag, "Olc ")) GET_OLC_ZONE(ch) = atoi(line);
                    break;

                case 'P':
                    if (!strcmp(tag, "Phas")) ch->set(CharAppearance::DistinguishingFeature, atoi(line));
                    else if (!strcmp(tag, "Phse")) GET_PHASE(ch) = atoi(line);
                    else if (!strcmp(tag, "Plyd")) ch->time.played = atol(line);
                    else if (!strcmp(tag, "Pole")) GET_POLE_BONUS(ch) = atoi(line);
                    else if (!strcmp(tag, "Posi")) GET_POS(ch) = atoi(line);
                    else if (!strcmp(tag, "Pref")) {
                        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
                        flags[0] = asciiflag_conv(f1);
                        flags[1] = asciiflag_conv(f2);
                        flags[2] = asciiflag_conv(f3);
                        flags[3] = asciiflag_conv(f4);
                        for(auto f = 0; f < NUM_PRF_FLAGS; f++) {
                            if(IS_SET_AR(flags, f)) ch->setFlag(FlagType::Pref, f);
                        }
                    } else if (!strcmp(tag, "Prff")) GET_PREFERENCE(ch) = atoi(line);
                    break;

                case 'R':
                    if (!strcmp(tag, "Race")) ch->race = (RaceID)atoi(line);
                    else if (!strcmp(tag, "Raci")) ch->set(CharNum::RacialPref, atoi(line));
                    else if (!strcmp(tag, "rDis")) GET_RDISPLAY(ch) = strdup(line);
                    else if (!strcmp(tag, "Rela")) GET_RELAXCOUNT(ch) = atoi(line);
                    else if (!strcmp(tag, "Rtim")) GET_RTIME(ch) = atoi(line);
                    else if (!strcmp(tag, "Rad1")) GET_RADAR1(ch) = atoi(line);
                    else if (!strcmp(tag, "Rad2")) GET_RADAR2(ch) = atoi(line);
                    else if (!strcmp(tag, "Rad3")) GET_RADAR3(ch) = atoi(line);
                    else if (!strcmp(tag, "Room")) GET_LOADROOM(ch) = atoi(line);
                    else if (!strcmp(tag, "RPfe")) GET_FEATURE(ch) = strdup(line);
                    break;

                case 'S':
                    if (!strcmp(tag, "Sex ")) ch->set(CharAppearance::Sex, atoi(line));
                    else if (!strcmp(tag, "Ship")) GET_SHIP(ch) = atoi(line);
                    else if (!strcmp(tag, "Scoo")) GET_SDCOOLDOWN(ch) = atoi(line);
                    else if (!strcmp(tag, "Shpr")) GET_SHIPROOM(ch) = atoi(line);
                    else if (!strcmp(tag, "Skil")) load_skills(fl, ch, false);
                    else if (!strcmp(tag, "Skn ")) ch->set(CharAppearance::SkinColor, atoi(line));
                    else if (!strcmp(tag, "Size")) ch->setSize(atoi(line));
                    else if (!strcmp(tag, "SklB")) load_skills(fl, ch, true);
                    else if (!strcmp(tag, "SkRc")) ch->modPractices(atoi(line));
                    else if (!strcmp(tag, "SkCl")) {
                        sscanf(line, "%d %d", &num2, &num3);
                        ch->modPractices(num3);
                    } else if (!strcmp(tag, "Slot")) ch->skill_slots = atoi(line);
                    else if (!strcmp(tag, "Spek")) SPEAKING(ch) = atoi(line);
                    else if (!strcmp(tag, "Str ")) ch->set(CharAttribute::Strength, atoi(line));
                    else if (!strcmp(tag, "Stuk")) ch->stupidkiss = atoi(line);
                    else if (!strcmp(tag, "Supp")) GET_SUPPRESS(ch) = atoi(line);
                    break;

                case 'T':
                    if (!strcmp(tag, "Tgro")) GET_TGROWTH(ch) = atoi(line);
                    else if (!strcmp(tag, "Tcla")) {
                        switch(atoi(line)) {
                            case 1: // great requirements... range is 0.2 to 0.3.
                                ch->transBonus = Random::get<double>(0.2, 0.3);
                            break;
                            case 2:
                                ch->transBonus = Random::get<double>(-0.1, 0.1);
                            break;
                            case 3:
                                ch->transBonus = Random::get<double>(-0.3, -0.2);
                            break;
                            default:
                                ch->transBonus = Random::get<double>(-0.3, 0.3);
                        }

                    }
                    else if (!strcmp(tag, "Tcos")) {
                        sscanf(line, "%d %d", &num2, &num3);
                        //GET_TRANSCOST(ch, num2) = num3;
                    } else if (!strcmp(tag, "Thir")) GET_COND(ch, THIRST) = atoi(line);
                    else if (!strcmp(tag, "Thr1")) GET_SAVE_MOD(ch, 0) = atoi(line);
                    else if (!strcmp(tag, "Thr2")) GET_SAVE_MOD(ch, 1) = atoi(line);
                    else if (!strcmp(tag, "Thr3")) GET_SAVE_MOD(ch, 2) = atoi(line);
                    else if (!strcmp(tag, "Thr4") || !strcmp(tag, "Thr5")); /* Discard extra saves */
                    else if (!strcmp(tag, "ThB1")) GET_SAVE_BASE(ch, 0) = atoi(line);
                    else if (!strcmp(tag, "ThB2")) GET_SAVE_BASE(ch, 1) = atoi(line);
                    else if (!strcmp(tag, "ThB3")) GET_SAVE_BASE(ch, 2) = atoi(line);
                    else if (!strcmp(tag, "Trag")) ch->set(CharTrain::Agility, atoi(line));
                    else if (!strcmp(tag, "Trco")) ch->set(CharTrain::Constitution, atoi(line));
                    else if (!strcmp(tag, "Trin")) ch->set(CharTrain::Intelligence, atoi(line));
                    else if (!strcmp(tag, "Trsp")) ch->set(CharTrain::Speed, atoi(line));
                    else if (!strcmp(tag, "Trst")) ch->set(CharTrain::Strength, atoi(line));
                    else if (!strcmp(tag, "Trwi")) ch->set(CharTrain::Wisdom, atoi(line));
                    break;
                case 'U':
                    if (!strcmp(tag, "Upgr")) GET_UP(ch) = atoi(line);
                    else if (!strcmp(tag, "User")) {
                        if (GET_LOG_USER(ch)) {
                            free(GET_LOG_USER(ch));
                        }
                        GET_LOG_USER(ch) = strdup(line);
                    }
                    break;
                case 'V':
                    if (!strcmp(tag, "Voic")) GET_VOICE(ch) = strdup(line);
                    break;

                case 'W':
                    if (!strcmp(tag, "Wate")) GET_WEIGHT(ch) = atoi(line);
                    else if (!strcmp(tag, "Wimp")) GET_WIMP_LEV(ch) = atoi(line);
                    else if (!strcmp(tag, "Wis ")) ch->set(CharAttribute::Wisdom, atoi(line));
                    break;

                default:
                    sprintf(buf, "SYSERR: Unknown tag %s in pfile %s", tag, name);
            }
        }
    }

    if (!ch->time.created) {
        basic_mud_log("No creation timestamp for user %s, using current time", GET_NAME(ch));
        ch->time.created = time(nullptr);
    }

    if (!ch->time.birth) {
        basic_mud_log("No birthday for user %s, using standard starting age determination", GET_NAME(ch));
        ch->time.birth = time(nullptr) - birth_age(ch);
    }

    /* initialization for imms */
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
        for (i = 1; i <= SKILL_TABLE_SIZE; i++)
            SET_SKILL(ch, i, 100);
        GET_COND(ch, HUNGER) = -1;
        GET_COND(ch, THIRST) = -1;
        GET_COND(ch, DRUNK) = -1;
    }


    if (IS_ANDROID(ch)) {
        GET_COND(ch, HUNGER) = -1;
        GET_COND(ch, THIRST) = -1;
        GET_COND(ch, DRUNK) = -1;
    }
    fclose(fl);
    return (id);
}

/* Return a filename given a house vnum */
int House_get_filename(room_vnum vnum, char *filename, size_t maxlen) {
    if (vnum == NOWHERE)
        return (0);

    snprintf(filename, maxlen, LIB_HOUSE"%d.house", vnum);
    return (1);
}


struct house_control_rec {
    room_vnum vn;        /* vnum of this house		*/
    room_vnum atrium;        /* vnum of atrium		*/
    int16_t exit_num;        /* direction of house's exit	*/
    time_t built_on;        /* date this house was built	*/
    int mode;            /* mode of ownership		*/
    long owner;            /* idnum of house's owner	*/
    int num_of_guests;        /* how many guests for house	*/
    long guests[MAX_GUESTS];    /* idnums of house's guests	*/
    time_t last_payment;        /* date of last house payment   */
    long bitvector;
    long builtby;
    long spare2;
    long spare3;
    long spare4;
    long spare5;
    long spare6;
    long spare7;
};

void boot_db_world_legacy() {

    basic_mud_log("Loading legacy world data...");
    basic_mud_log("Loading zone table.");
    index_boot(DB_BOOT_ZON);

    basic_mud_log("Loading triggers and generating index.");
    index_boot(DB_BOOT_TRG);

    basic_mud_log("Loading rooms.");
    index_boot(DB_BOOT_WLD);

    basic_mud_log("Checking start rooms.");
    check_start_rooms();

    basic_mud_log("Loading mobs and generating index.");
    index_boot(DB_BOOT_MOB);

    basic_mud_log("Loading objs and generating index.");
    index_boot(DB_BOOT_OBJ);

    basic_mud_log("Loading disabled commands list...");
    load_disabled();

    basic_mud_log("Loading shops.");
    index_boot(DB_BOOT_SHP);

    basic_mud_log("Loading guild masters.");
    index_boot(DB_BOOT_GLD);

    //boot_db_help();

    boot_db_shadow();
}

static void boot_db_legacy() {
    boot_db_time();
    boot_db_textfiles();
    boot_db_spellfeats();
    boot_db_world_legacy();
    boot_db_mail();
    boot_db_socials();
    boot_db_commands();
    boot_db_specials();
    boot_db_assemblies();
    boot_db_sort();
    boot_db_boards();
    boot_db_banned();
    boot_db_spacemap();
    //boot_db_help();
    topLoad();
}

// ACTUAL MIGRATION STUFF BELOW...

static std::string stripAnsi(const std::string& str) {
    return processColors(str, false, nullptr);
}

struct old_ship_data {
    std::string name;
    std::set<vnum> vnums;
    std::optional<vnum> hatch_room{};
    std::optional<vnum> ship_obj{};
    std::optional<vnum> location{};
};

static struct old_ship_data gships[] = {
        {"Falcon", {3900, 3901, 3902, 3903, 3904}, 3900, 3900, 408},
        {"Simurgh", {3905, 3996, 3997, 3998, 3999}, 3905, 3905, 12002},
        {"Zypher", {3906, 3907, 3908, 3909, 3910}, 3906, 3906, 4250},
        {"Valkyrie", {3911, 3912, 3913, 3914, 3915}, 3911, 3911, 2323},
        {"Phoenix", {3916, 3917, 3918, 3919, 3920}, 3916, 3916, 408},
        {"Merganser", {3921, 3922, 3923, 3924, 3925}, 3921, 3921, 2323},
        {"Wraith", {3926, 3927, 3928, 3929, 3930}, 3930, 3930, 11626},
        {"Ghost", {3931, 3932, 3933, 3934, 3935}, 3935, 3935, 8194},
        {"Wisp", {3936, 3937, 3938, 3939, 3940}, 3940, 3940, 12002},
        {"Eagle", {3941, 3942, 3943, 3944, 3945}, 3945, 3945, 4250},

        {"Spectral", {3946, 3947, 3948, 3949, 3950}, 3950, {}, {}},
        {"Raven", {3951, 3952, 3953, 3954, 3955, 3961}, 3955, {}, {}},
        {"Marauder", {3956, 3957, 3958, 3959, 3960}, 3960, {}, {}},
        {"Vanir", {3962, 3963, 3964, 3965}, 3965, {}, {}},
        {"Aesir", {3966, 3967, 3968, 3969, 3970}, 3970, {}, {}},
        {"Undine", {3971, 3972, 3973, 3974, 3975}, 3975, {}, {}},
        {"Banshee", {3976, 3977, 3978, 3979, 3980}, 3980, {}, {}},
        {"Hydra", {3981, 3982, 3983, 3984, 3985}, 3985, {}, {}},
        {"Medusa", {3986, 3987, 3988, 3989, 3990}, 3990, {}, {}},
        {"Pegasus", {3991, 3992, 3993, 3994, 3995}, 3995, {}, {}},
        {"Zel 1", {5824}, 5824, {}, {}},
        {"Zel 2", {5825}, 5825, {}, {}}
};

static struct old_ship_data customs[] = {
        {"Yun-Yammka", {19900, 19901, 19902}, 19901, 19900, {}},
        {"The Dark Archon", {19903, 19912, 19913, 19914}, 19912, 19916, {}},
        {"The Zafir Krakken", {19904, 19905, 19906, 19907}, 19905, 19905, {}},
        {"Crimson Talon", {19908, 19909, 19910, 19911}, 19908, 19910, {}},
        {"Rust Bucket", {19915, 19916, 19918, 19930}, 19915, 19921, {}},
        {"The Adamant", {19917, 19949, 19955, 19956}, 19949, 19966, {}},
        {"Vanguard", {19919, 19920, 19921, 19922}, 19920, 19926, {}},
        {"The Glacial Shadow", {19925, 19923, 19924, 19926}, 19923, 19931, {}},
        {"The Molecular Dynasty", {19927, 19928, 19929, 19954}, 19927, 19936, {}},
        {"Poseidon", {19931, 19933, 19932, 19934}, 19931, 19941, {}},
        {"Sakana Mirai", {19935, 19936, 19937, 19938}, 19935, 19946, {}},
        {"Earth Far Freighter Enterjection", {19939, 19940, 19941, 19942}, 19939, 19951, {}},
        {"Soaring Angel", {19943, 19944, 19945, 19946}, 19943, 19956, {}},
        {"The Grey Wolf", {19947, 19948, 19978, 19979}, 19947, 19961, {}},
        {"The Event Horizon", {19950, 19951, 19980, 19981}, 19950, 19971, {}},
        {"Fleeting Star", {19952, 19953, 19957, 19958}, 19952, 19976, {}},
        {"Makenkosappo", {19959, 19960, 19961, 19962}, 19959, 19981, {}},
        {"The Nightingale", {19963, 19964, 19965, 19982}, 19963, 19986, {}},
        {"The Honey Bee", {19966, 19967, 19968, 19969}, 19966, 19991, {}},
        {"The Bloodrose", {19970, 19971, 19972, 19973}, 19970, 19996, {}},
        {"The Stimato", {19974, 19975, 19976, 19977}, 19974, {}, {}},
        {"The Tatsumaki", {15805, 15806, 15807, 15808}, 15805, 15805, {}},
        {"Shattered Soul", {15800, 15801, 15802, 15803}, 15800, {}, {}}
};

struct AreaDef {
    std::string name;
    int type{ITEM_REGION};
    GameEntity *location{};
    std::optional<vnum> parent;
    std::set<std::size_t> roomFlags{};
    std::vector<std::pair<std::size_t, std::size_t>> roomRanges;
    std::set<vnum> roomIDs{}, roomSkips{};
    std::unordered_set<int> flags;
    std::unordered_map<EnvVar, double> envVars;
};

static Structure* assembleArea(const AreaDef &def) {
    auto vn = getNextUID();
    auto a = new Structure();
    a->script = std::make_shared<script_data>(a);
    a->uid = vn;
    world[vn] = a;
    a->setName(def.name);
    a->type_flag= def.type;
    a->flags[FlagType::Structure] = def.flags;

    a->envVars = def.envVars;

    if(def.location) {
        a->addToLocation(def.location);
    }

    std::set<vnum> rooms = def.roomIDs;

    for(auto &[start, end] : def.roomRanges) {
        for(auto i = start; i <= end; i++) {
            auto found = world.find(i);
            if(found == world.end()) continue;
            rooms.insert(i);
        }
    }

    if(!def.roomFlags.empty()) {
        for(auto &[vn, u] : world) {
            if(auto room = dynamic_cast<Room*>(u); room) {
                for(auto &f : def.roomFlags) {
                    if(room->checkFlag(FlagType::Room, f)) {
                        rooms.insert(vn);
                        break;
                    }
                }
            }
        }
    }

    for(auto r: def.roomSkips) rooms.erase(r);

    basic_mud_log("Assembling Area: %s, Rooms: %d", def.name.c_str(), rooms.size());

    for(auto r : rooms) {
        auto found = world.find(r);
        if(found == world.end()) continue;
        auto room = dynamic_cast<Room*>(found->second);
        if(!room) continue;
        if(room->location) continue;
        room->addToLocation(a);
    }

    return a;

}


void migrate_grid() {
    AreaDef adef;
    adef.name = "Admin Land";
    adef.roomRanges.emplace_back(0, 16);
    adef.roomIDs = {16694, 16698};
    adef.type = ITEM_DIMENSION;
    auto admin_land = assembleArea(adef);

    AreaDef mudschooldef;
    mudschooldef.name = "MUD School";
    mudschooldef.roomRanges.emplace_back(100, 154);
    mudschooldef.type = ITEM_DIMENSION;
    auto mud_school = assembleArea(mudschooldef);

    AreaDef mvdef;
    mvdef.name = "Multiverse";
    mvdef.type = ITEM_DIMENSION;
    auto multiverse = assembleArea(mvdef);

    AreaDef xvdef;
    xvdef.name = "Xenoverse";
    xvdef.type = ITEM_DIMENSION;
    auto xenoverse = assembleArea(xvdef);

    AreaDef u7def;
    u7def.name = "Universe 7";
    u7def.type = ITEM_DIMENSION;
    u7def.location = multiverse;
    auto universe7 = assembleArea(u7def);

    AreaDef mplane;
    mplane.name = "Mortal Plane";
    mplane.type = ITEM_DIMENSION;
    mplane.location = universe7;
    auto mortal_plane = assembleArea(mplane);

    AreaDef cplane;
    cplane.name = "Celestial Plane";
    cplane.type = ITEM_DIMENSION;
    cplane.location = universe7;
    auto celestial_plane = assembleArea(cplane);

    AreaDef spacedef;
    spacedef.name = "Depths of Space";
    spacedef.type = ITEM_REGION;
    spacedef.location = mortal_plane;
    // Insert every room id from mapnums (the 2d array) into spacedef.roomIDs...
    for(auto &row : mapnums) {
        for(auto &col : row) {
            spacedef.roomIDs.insert(col);
        }
    }
    auto space = assembleArea(spacedef);

    std::unordered_map<std::string, AreaDef> areaDefs;

    { // Earth miscellaneous rooms...
        auto &w = areaDefs["West City"];
        w.roomRanges.emplace_back(19500, 19558);
        w.roomIDs.insert(19576);
        w.roomIDs.insert(19599);
        w.roomIDs.insert(178);
        auto &s = areaDefs["Silver Mine"];
        s.roomIDs.insert(19577);
        auto &n = areaDefs["Nexus City"];
        n.roomIDs.insert(5827);
        n.roomIDs.insert(199);
        n.roomIDs.insert(19);
        n.roomIDs.insert(20);
        n.roomIDs.insert(25);
        n.roomIDs.insert(29);
        n.roomIDs.insert(81);
        n.roomIDs.insert(97);
        n.roomIDs.insert(98);
        n.roomIDs.insert(99);
        n.roomIDs.insert(19001);
        n.roomIDs.insert(19007);
        n.roomIDs.insert(23);
    }

    {// Vegeta misc..
        auto &v = areaDefs["Vegetos City"];
        v.roomIDs.insert(15700);
        v.roomIDs.insert(82);
        v.roomIDs.insert(19003);
        v.roomIDs.insert(179);
        auto &b = areaDefs["Blood Dunes"];
        b.roomIDs.insert(155);
        b.roomIDs.insert(156);
    }

    {
        // Frigid misc...
        auto &i = areaDefs["Ice Crown City"];
        i.roomIDs.insert(83);
        i.roomIDs.insert(19002);
        i.roomIDs.insert(180);
    }

    {
        // Aether misc...
        auto &h = areaDefs["Haven City"];
        h.roomIDs.insert(85);
        h.roomIDs.insert(183);
        h.roomIDs.insert(19005);
        h.roomIDs.insert(19008);
    }

    {
        // yardrat...
        auto &y = areaDefs["Yardra City"];
        y.roomIDs.insert(26);
    }
    {
        // Konack
        auto &t = areaDefs["Tiranoc City"];
        t.roomIDs.insert(86);
        t.roomIDs.insert(181);
        t.roomIDs.insert(19004);
    }

    {
        // Namek stuff..
        auto &k = areaDefs["Kakureta Village"];
        k.roomRanges.emplace_back(14400, 14499);
        auto &s = areaDefs["Senzu Village"];
        s.roomIDs.insert(84);
        s.roomIDs.insert(182);
        auto &e = areaDefs["Elder Village"];
        e.roomIDs.insert(19006);
    }

    {
        // kanassa misc...
        auto &a = areaDefs["Aquis City"];
        a.roomIDs.insert(177);
    }

    for(auto &[rv, u] : world) {
        auto room = dynamic_cast<Room*>(u);
        if(!room) continue;
        if(room->area) continue;
        auto sense = sense_location_name(rv);
        if(sense != "Unknown.") {
            auto &area = areaDefs[sense];
            area.roomIDs.insert(rv);
        }
    }

    std::unordered_map<std::string, Structure*> areaObjects;

    for(auto &[name, def] : areaDefs) {
        def.name = name;
        def.type = ITEM_REGION;
        auto aent = assembleArea(def);
        areaObjects[name] = aent;
    }

    AreaDef pearth;
    pearth.name = "@GEarth@n";
    pearth.type = ITEM_CELESTIAL_BODY;
    pearth.location = world.at(50);
    auto planet_earth = assembleArea(pearth);

    AreaDef pvegeta;
    pvegeta.name = "@YVegeta@n";
    pvegeta.type = ITEM_CELESTIAL_BODY;
    pvegeta.location = world.at(53);
    pvegeta.envVars[EnvVar::Gravity] = 10.0;
    auto planet_vegeta = assembleArea(pvegeta);

    AreaDef pfrigid;
    pfrigid.name = "@CFrigid@n";
    pfrigid.type = ITEM_CELESTIAL_BODY;
    pfrigid.location = world.at(51);
    auto planet_frigid = assembleArea(pfrigid);

    AreaDef pnamek;
    pnamek.name = "@gNamek@n";
    pnamek.type = ITEM_CELESTIAL_BODY;
    pnamek.location = world.at(54);
    auto planet_namek = assembleArea(pnamek);

    AreaDef pkonack;
    pkonack.name = "@MKonack@n";
    pkonack.type = ITEM_CELESTIAL_BODY;
    pkonack.location = world.at(52);
    auto planet_konack = assembleArea(pkonack);

    AreaDef paether;
    paether.name = "@MAether@n";
    paether.type = ITEM_CELESTIAL_BODY;
    paether.location = world.at(55);
    auto planet_aether = assembleArea(paether);

    AreaDef pyardrat;
    pyardrat.name = "@mYardrat@n";
    pyardrat.type = ITEM_CELESTIAL_BODY;
    pyardrat.location = world.at(56);
    auto planet_yardrat = assembleArea(pyardrat);

    AreaDef pkanassa;
    pkanassa.name = "@BKanassa@n";
    pkanassa.type = ITEM_CELESTIAL_BODY;
    pkanassa.location = world.at(58);
    auto planet_kanassa = assembleArea(pkanassa);

    AreaDef pcerria;
    pcerria.name = "@RCerria@n";
    pcerria.type = ITEM_CELESTIAL_BODY;
    pcerria.location = world.at(198);
    auto planet_cerria = assembleArea(pcerria);

    AreaDef parlia;
    parlia.name = "@GArlia@n";
    parlia.type = ITEM_CELESTIAL_BODY;
    parlia.location = world.at(59);
    auto planet_arlia = assembleArea(parlia);

    AreaDef pzenith;
    pzenith.name = "@BZenith@n";
    pzenith.type = ITEM_CELESTIAL_BODY;
    pzenith.location = world.at(57);
    auto moon_zenith = assembleArea(pzenith);
    for(const auto& name : {"Ancient Castle", "Utatlan City", "Zenith Jungle"}) {
        auto a = areaObjects[name];
        a->addToLocation(moon_zenith);
    }


    AreaDef ucdef;
    ucdef.name = "Underground Cavern";
    ucdef.location = moon_zenith;
    ucdef.roomRanges.emplace_back(62900, 63000);
    auto underground_cavern = assembleArea(ucdef);

    for(auto &p : {planet_earth, planet_aether, planet_namek, moon_zenith}) {
        p->setFlag(FlagType::Structure, STRUCTURE_ETHER);
    }

    for(auto &p : {planet_earth, planet_aether, planet_vegeta, planet_frigid}) {
        p->setFlag(FlagType::Structure, STRUCTURE_MOON);
    }

    AreaDef zelakinfarm;
    zelakinfarm.name = "Zelakin's Farm";
    zelakinfarm.location = xenoverse;
    zelakinfarm.roomRanges.emplace_back(5896, 5899);
    auto zelakin_farm = assembleArea(zelakinfarm);

    AreaDef hbtcdef;
    hbtcdef.name = "Hyperbolic Time Chamber";
    hbtcdef.location = universe7;
    hbtcdef.roomRanges.emplace_back(64000, 64097);
    hbtcdef.type = ITEM_DIMENSION;
    auto hbtc = assembleArea(hbtcdef);

    AreaDef bodef;
    bodef.name = "The Black Omen";
    bodef.location = space;
    bodef.roomIDs.insert(19053);
    bodef.roomIDs.insert(19039);
    for(auto &[r, room] : world) {
        if(icontains(stripAnsi(room->getName()), "Black Omen")) bodef.roomIDs.insert(r);
    }
    bodef.roomIDs.insert(19050);
    bodef.type = ITEM_VEHICLE;
    auto black_omen = assembleArea(bodef);

    AreaDef earthduel;
    earthduel.name = "Duel Dome";
    earthduel.location = planet_earth;
    earthduel.roomRanges.emplace_back(160, 176);
    auto earth_duel_dome = assembleArea(earthduel);

    AreaDef earthwmat;
    earthwmat.name = "World Martial Arts Building";
    earthwmat.location = planet_earth;
    earthwmat.roomRanges.emplace_back(3800, 3834);
    earthwmat.roomRanges.emplace_back(19578, 19598);
    earthwmat.roomRanges.emplace_back(19570, 19573);
    earthwmat.roomIDs = {19574, 19575};
    auto earth_wmat = assembleArea(earthwmat);

    AreaDef capsulecorp;
    capsulecorp.name = "Capsule Corporation";
    capsulecorp.location = areaObjects["West City"];
    capsulecorp.roomRanges.emplace_back(19559, 19569);
    auto capsule_corp = assembleArea(capsulecorp);

    AreaDef threestarelem;
    threestarelem.name = "Three Star Elementary";
    threestarelem.location = planet_earth;
    threestarelem.roomRanges.emplace_back(5800, 5823);
    threestarelem.roomIDs.insert(5826);
    auto three_star_elem = assembleArea(threestarelem);

    AreaDef gerol;
    gerol.name = "Gero's Lab";
    gerol.location = planet_earth;
    gerol.roomRanges.emplace_back(7701, 7753);
    auto gero_lab = assembleArea(gerol);

    AreaDef shadowrain;
    shadowrain.name = "Shadowrain City";
    shadowrain.location = planet_earth;
    shadowrain.roomRanges.emplace_back(9111, 9199);
    auto shadowrain_city = assembleArea(shadowrain);

    AreaDef kingcastle;
    kingcastle.name = "King Castle";
    kingcastle.location = planet_earth;
    kingcastle.roomRanges.emplace_back(12600, 12627);
    auto king_castle = assembleArea(kingcastle);

    AreaDef orangestar;
    orangestar.name = "Orange Star Highschool";
    orangestar.location = planet_earth;
    orangestar.roomRanges.emplace_back(16400, 16499);
    auto orange_star = assembleArea(orangestar);

    AreaDef ath;
    ath.name = "Athletic Field";
    ath.location = orange_star;
    ath.roomRanges.emplace_back(15900, 15937);
    auto athletic_field = assembleArea(ath);

    AreaDef oak;
    oak.name = "Inside an Oak Tree";
    oak.location = areaObjects["Northern Plains"];
    oak.roomRanges.emplace_back(16200, 16210);
    oak.roomIDs = {19199};
    auto oak_tree = assembleArea(oak);

    AreaDef edfhq;
    edfhq.name = "EDF Headquarters";
    edfhq.location = planet_earth;
    edfhq.type = ITEM_BUILDING;
    edfhq.roomRanges.emplace_back(9101, 9110);
    auto edf_hq = assembleArea(edfhq);

    AreaDef bar;
    bar.name = "Bar";
    bar.location = planet_earth;
    bar.type = ITEM_BUILDING;
    bar.roomRanges.emplace_back(18100, 18114);
    auto bar_ = assembleArea(bar);

    AreaDef themoon;
    themoon.name = "The Moon";
    themoon.location = space;
    themoon.type = ITEM_CELESTIAL_BODY;
    themoon.envVars[EnvVar::Gravity] = 10.0;
    auto moon = assembleArea(themoon);

    AreaDef luncrat;
    luncrat.name = "Lunar Crater";
    luncrat.location = moon;
    luncrat.roomRanges.emplace_back(63300, 63311);
    auto lunar_crater = assembleArea(luncrat);

    AreaDef cratpass;
    cratpass.name = "Crater Passage";
    cratpass.location = moon;
    cratpass.roomRanges.emplace_back(63312, 63336);
    auto crater_passage = assembleArea(cratpass);

    AreaDef darkside;
    darkside.name = "Darkside Crater";
    darkside.location = moon;
    darkside.roomRanges.emplace_back(63337, 63362);
    auto darkside_crater = assembleArea(darkside);

    AreaDef moonstone;
    moonstone.name = "Moonstone Quarry";
    moonstone.location = moon;
    moonstone.roomRanges.emplace_back(63381, 63392);
    auto moonstone_quarry = assembleArea(moonstone);

    AreaDef intrepidbase;
    intrepidbase.name = "Intrepid Base";
    intrepidbase.location = moon;
    intrepidbase.roomRanges.emplace_back(63363, 63380);
    intrepidbase.roomRanges.emplace_back(63393, 63457);
    auto intrepid_base = assembleArea(intrepidbase);

    AreaDef fortemple;
    fortemple.name = "Forgotten Temple";
    fortemple.location = moon;
    fortemple.roomRanges.emplace_back(63458, 63499);
    auto forgotten_temple = assembleArea(fortemple);

    for(auto child : moon->getContents()) {
        for(auto r : child->getRooms()) {
            r->clearFlag(FlagType::Room, ROOM_EARTH);
        }
    }

    AreaDef prideplains;
    prideplains.name = "Pride Plains";
    prideplains.location = planet_vegeta;
    prideplains.roomRanges.emplace_back(19700, 19711);
    auto pride_plains = assembleArea(prideplains);

    AreaDef pridesomething;
    pridesomething.name = "Pride Something";
    pridesomething.location = planet_vegeta;
    pridesomething.roomRanges.emplace_back(19740, 19752);
    auto pride_something = assembleArea(pridesomething);

    AreaDef pridejungle;
    pridejungle.name = "Pride Jungle";
    pridejungle.location = planet_vegeta;
    pridejungle.roomRanges.emplace_back(19712, 19718);
    pridejungle.roomRanges.emplace_back(19753, 19789);
    auto pride_jungle = assembleArea(pridejungle);

    AreaDef pridecave;
    pridecave.name = "Pride Cave";
    pridecave.location = planet_vegeta;
    pridecave.roomRanges.emplace_back(9400, 9499);
    auto pride_cave = assembleArea(pridecave);

    AreaDef pridedesert;
    pridedesert.name = "Pride Desert";
    pridedesert.location = planet_vegeta;
    pridedesert.roomRanges.emplace_back(19719, 19739);
    pridedesert.roomIDs.insert(19790);
    auto pride_desert = assembleArea(pridedesert);

    AreaDef rocktail;
    rocktail.name = "Rocktail Camp";
    rocktail.location = planet_vegeta;
    rocktail.roomRanges.emplace_back(61030, 61044);
    rocktail.roomIDs.insert(19198);
    auto rocktail_camp = assembleArea(rocktail);

    AreaDef lavaarena;
    lavaarena.name = "Lava Arena";
    lavaarena.location = planet_frigid;
    lavaarena.roomRanges.emplace_back(12900, 12918);
    auto lava_arena = assembleArea(lavaarena);

    AreaDef strangecliff;
    strangecliff.name = "Strange Cliff";
    strangecliff.location = planet_namek;
    strangecliff.roomRanges.emplace_back(12800, 12813);
    auto strange_cliff = assembleArea(strangecliff);

    AreaDef stonehallway;
    stonehallway.name = "Stone Hallway";
    stonehallway.location = planet_namek;
    stonehallway.roomRanges.emplace_back(12814, 12831);
    stonehallway.roomSkips.insert(12825);
    auto stone_hallway = assembleArea(stonehallway);

    AreaDef tranquilpalm;
    tranquilpalm.name = "Tranquil Palm Dojo";
    tranquilpalm.location = planet_namek;
    tranquilpalm.roomRanges.emplace_back(12832, 12868);
    auto tranquil_palm_dojo = assembleArea(tranquilpalm);

    AreaDef namekunder;
    namekunder.name = "Namekian Underground";
    namekunder.location = planet_namek;
    namekunder.roomRanges.emplace_back(64700, 65009);
    auto namek_underground = assembleArea(namekunder);

    AreaDef advkindojo;
    advkindojo.name = "Advanced Kinetic Dojo";
    advkindojo.location = planet_aether;
    advkindojo.roomRanges.emplace_back(17743, 17751);
    auto advanced_kinetic_dojo = assembleArea(advkindojo);

    AreaDef lostcity;
    lostcity.name = "Lost City";
    lostcity.location = planet_kanassa;
    lostcity.roomRanges.emplace_back(7600, 7686);
    auto lost_city = assembleArea(lostcity);

    AreaDef aqtower;
    aqtower.name = "Aquis Tower";
    aqtower.location = areaObjects["Aquis City"];
    aqtower.roomRanges.emplace_back(12628, 12666);
    auto aquis_tower = assembleArea(aqtower);

    AreaDef moaipalace;
    moaipalace.name = "Moai's Palace";
    moaipalace.location = planet_arlia;
    moaipalace.roomRanges.emplace_back(12667, 12699);
    auto moai_palace = assembleArea(moaipalace);

    AreaDef darkthorne;
    darkthorne.name = "DarkThorne Compound";
    darkthorne.location = planet_arlia;
    darkthorne.roomRanges.emplace_back(18150, 18169);
    auto darkthorne_compound = assembleArea(darkthorne);


    std::unordered_map<int, Structure*> planetMap = {
            {ROOM_EARTH, planet_earth},
            {ROOM_VEGETA, planet_vegeta},
            {ROOM_FRIGID, planet_frigid},
            {ROOM_NAMEK, planet_namek},
            {ROOM_YARDRAT, planet_yardrat},
            {ROOM_KONACK, planet_konack},
            {ROOM_AETHER, planet_aether},
            {ROOM_KANASSA, planet_kanassa},
            {ROOM_ARLIA, planet_arlia},
            {ROOM_CERRIA, planet_cerria},
    };

    basic_mud_log("Attempting to deduce Areas to Planets...");
    for(auto &[vnum, u] : world) {
        // check for planetMap flags and, if found, bind the area this room belongs to, to the respective planet.
        auto room = dynamic_cast<Room*>(u);
        if(!room) continue;

        for(auto &[rflag, a] : planetMap) {
            if(!room->area) continue;
            if(room->checkFlag(FlagType::Room, rflag)) {
                auto avn = room->getLocation();
                avn->addToLocation(a);
                break;
            }
        }
    }
    basic_mud_log("Done deducing Areas to Planets.");


    AreaDef nodef;
    nodef.name = "Northran";
    nodef.location = xenoverse;
    nodef.type = ITEM_DIMENSION;
    nodef.roomRanges.emplace_back(17900, 17999);
    auto northran = assembleArea(nodef);

    AreaDef celdef;
    celdef.name = "Celestial Corp";
    celdef.location = space;
    celdef.type = ITEM_SPACE_STATION;
    celdef.roomRanges.emplace_back(16305, 16399);
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room->getName()), "Celestial Corp")) celdef.roomIDs.insert(rv);
    }
    auto celestial_corp = assembleArea(celdef);

    AreaDef gneb;
    gneb.name = "Green Nebula Mall";
    gneb.location = space;
    gneb.type = ITEM_SPACE_STATION;
    gneb.roomRanges.emplace_back(17200, 17276);
    gneb.roomIDs.insert(184);
    auto green_nebula = assembleArea(gneb);

    AreaDef cooler;
    cooler.name = "Cooler's Ship";
    cooler.location = space;
    cooler.type = ITEM_VEHICLE;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room->getName()), "Cooler's Ship")) {
            cooler.roomIDs.insert(rv);
        }
    }
    auto cooler_ship = assembleArea(cooler);

    AreaDef alph;
    alph.name = "Alpharis";
    alph.type = ITEM_SPACE_STATION;
    alph.location = space;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room->getName()), "Alpharis")) alph.roomIDs.insert(rv);
    }
    auto alpharis = assembleArea(alph);

    AreaDef dzone;
    dzone.name = "Dead Zone";
    dzone.location = universe7;
    dzone.type = ITEM_DIMENSION;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room->getName()), "Dead Zone")) dzone.roomIDs.insert(rv);
    }
    auto dead_zone = assembleArea(dzone);

    AreaDef bast;
    bast.name = "Blasted Asteroid";
    bast.location = space;
    bast.type = ITEM_CELESTIAL_BODY;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room->getName()), "Blasted Asteroid")) bast.roomIDs.insert(rv);
    }
    auto blasted_asteroid = assembleArea(bast);


    AreaDef listres;
    listres.name = "Lister's Restaurant";
    listres.location = xenoverse;
    listres.type = ITEM_BUILDING;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room->getName()), "Lister's Restaurant")) listres.roomIDs.insert(rv);
    }
    listres.roomIDs = {18640};
    auto listers_restaurant = assembleArea(listres);

    AreaDef scasino;
    scasino.name = "Shooting Star Casino";
    scasino.type = ITEM_BUILDING;
    scasino.location = xenoverse;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room->getName()), "Shooting Star Casino")) scasino.roomIDs.insert(rv);
    }
    auto shooting_star_casino = assembleArea(scasino);

    AreaDef outdef;
    outdef.name = "The Outpost";
    outdef.location = celestial_plane;
	outdef.type = ITEM_BUILDING;
    for(auto &[rv, room] : world) {
        if(icontains(stripAnsi(room->getName()), "The Outpost")) outdef.roomIDs.insert(rv);
    }
    auto outpost = assembleArea(outdef);

    AreaDef kyem;
    kyem.name = "King Yemma's Domain";
    kyem.location = celestial_plane;
    kyem.roomRanges.emplace_back(6000, 6030);
    kyem.roomSkips.insert(6017);
    kyem.roomIDs.insert(6295);
    auto king_yemma = assembleArea(kyem);

    AreaDef snway;
    snway.name = "Snake Way";
    snway.location = celestial_plane;
    snway.roomRanges.emplace_back(6031, 6099);
    snway.roomIDs.insert(6017);
    auto snake_way = assembleArea(snway);

    AreaDef nkai;
    nkai.name = "North Kai's Planet";
    nkai.location = celestial_plane;
    nkai.envVars[EnvVar::Gravity] = 10.0;
    nkai.type = ITEM_CELESTIAL_BODY;
    nkai.roomRanges.emplace_back(6100, 6138);
    auto north_kai = assembleArea(nkai);

    AreaDef serp;
    serp.name = "Serpent's Castle";
    serp.location = snake_way;
    serp.type = ITEM_BUILDING;
    serp.roomRanges.emplace_back(6139, 6166);
    auto serpents_castle = assembleArea(serp);

    AreaDef gkai;
    gkai.name = "Grand Kai's Planet";
    gkai.location = celestial_plane;
    gkai.type = ITEM_CELESTIAL_BODY;
    gkai.roomRanges.emplace_back(6800, 6960);
    auto grand_kai = assembleArea(gkai);

    AreaDef gkaipalace;
    gkaipalace.name = "Grand Kai's Palace";
    gkaipalace.location = grand_kai;
    gkaipalace.type = ITEM_BUILDING;
    gkaipalace.roomRanges.emplace_back(6961, 7076);
    auto grand_kais_palace = assembleArea(gkaipalace);

    AreaDef maze;
    maze.name = "Maze of Echoes";
    maze.location = celestial_plane;
    maze.roomRanges.emplace_back(7100, 7199);
    auto maze_of_echoes = assembleArea(maze);

    AreaDef cat;
    cat.name = "Dark Catacomb";
    cat.location = maze_of_echoes;
    cat.roomRanges.emplace_back(7200, 7245);
    auto dark_catacomb = assembleArea(cat);

    AreaDef twi;
    twi.name = "Twilight Cavern";
    twi.location = celestial_plane;
    twi.roomRanges.emplace_back(7300, 7499);
    auto twilight_cavern = assembleArea(twi);

    AreaDef helldef;
    helldef.name = "Hell";
    helldef.location = celestial_plane;
    helldef.roomRanges.emplace_back(6200, 6298);
    helldef.roomSkips.insert(6295);
    auto hell = assembleArea(helldef);

    AreaDef hellhouse;
    hellhouse.name = "Hell - Old House";
    hellhouse.location = hell;
    hellhouse.roomRanges.emplace_back(61000, 61007);
    auto hell_old_house = assembleArea(hellhouse);

    AreaDef gyukihouse;
    gyukihouse.name = "Gyuki's House";
    gyukihouse.location = planet_earth;
    gyukihouse.roomRanges.emplace_back(61015, 61026);
    auto gyukis_house = assembleArea(gyukihouse);

    AreaDef hfields;
    hfields.name = "Hell Fields";
    hfields.location = hell;
    hfields.roomRanges.emplace_back(6200, 6300);
    auto hell_fields = assembleArea(hfields);

    AreaDef hsands;
    hsands.name = "Sands of Time";
    hsands.location = hell;
    hsands.roomRanges.emplace_back(6300, 6348);
    auto sands_of_time = assembleArea(hsands);

    AreaDef hchaotic;
    hchaotic.name = "Chaotic Spiral";
    hchaotic.location = hell;
    hchaotic.roomRanges.emplace_back(6349, 6399);
    auto chaotic_spiral = assembleArea(hchaotic);

    AreaDef hfirecity;
    hfirecity.name = "Hellfire City";
    hfirecity.location = hell;
    hfirecity.roomRanges.emplace_back(6400, 6529);
    hfirecity.roomIDs = {6568, 6569, 6600, 6699};
    auto hellfire_city = assembleArea(hfirecity);

    AreaDef fbagdojo;
    fbagdojo.name = "Flaming Bag Dojo";
    fbagdojo.type = ITEM_BUILDING;
    fbagdojo.location = hellfire_city;
    fbagdojo.roomRanges.emplace_back(6530, 6568);
    auto flaming_bag_dojo = assembleArea(fbagdojo);

    AreaDef etrailgrave;
    etrailgrave.name = "Entrail Graveyard";
    etrailgrave.location = hellfire_city;
    etrailgrave.roomRanges.emplace_back(6601, 6689);
    auto entrail_graveyard = assembleArea(etrailgrave);

    AreaDef psihnon;
    psihnon.name = "Sihnon";
    psihnon.location = space;
    psihnon.type = ITEM_CELESTIAL_BODY;
    psihnon.roomRanges.emplace_back(3600, 3699);
    auto planet_sihnon = assembleArea(psihnon);

    AreaDef majdef;
    majdef.name = "Majinton";
    majdef.location = planet_sihnon;
    majdef.type = ITEM_DIMENSION;
    majdef.roomRanges.emplace_back(3700, 3797);
    auto majinton = assembleArea(majdef);

    AreaDef wistower;
    wistower.name = "Wisdom Tower";
    wistower.location = planet_namek;
    wistower.type = ITEM_BUILDING;
    wistower.roomRanges.emplace_back(9600, 9666);
    auto wisdom_tower = assembleArea(wistower);

    AreaDef veld;
    veld.name = "Veldryth Mountains";
    veld.location = planet_konack;
    veld.roomRanges.emplace_back(9300, 9355);
    auto veldryth_mountains = assembleArea(veld);

    AreaDef machia;
    machia.name = "Machiavilla";
    machia.location = planet_konack;
    machia.type = ITEM_BUILDING;
    machia.roomRanges.emplace_back(12743, 12798);
    machia.roomRanges.emplace_back(12700, 12761);
    machia.roomIDs.insert(9356);
    auto machiavilla = assembleArea(machia);

    AreaDef laron;
    laron.name = "Laron Forest";
    laron.location = planet_konack;
    laron.roomRanges.emplace_back(19200, 19299);
    auto laron_forest = assembleArea(laron);

    AreaDef nazr;
    nazr.name = "Nazrin Village";
    nazr.location = planet_konack;
    nazr.roomRanges.emplace_back(19300, 19347);
    nazr.roomIDs = {19398};
    auto nazrin_village = assembleArea(nazr);

    AreaDef nazchief;
    nazchief.name = "Chieftain's House";
    nazchief.type = ITEM_BUILDING;
    nazchief.location = nazrin_village;
    nazchief.roomRanges.emplace_back(19348, 19397);
    auto chieftains_house = assembleArea(nazchief);

    AreaDef shmaze;
    shmaze.name = "Shadow Maze";
    shmaze.type = ITEM_REGION;
    shmaze.location = chieftains_house;
    shmaze.roomRanges.emplace_back(19400, 19499);
    auto shadow_maze = assembleArea(shmaze);

    AreaDef monbal;
    monbal.name = "Monastery of Balance";
    monbal.type = ITEM_BUILDING;
    monbal.location = planet_konack;
    monbal.roomRanges.emplace_back(9500, 9599);
    monbal.roomRanges.emplace_back(9357, 9364);
    monbal.roomIDs.insert(9365);
    auto monastery_of_balance = assembleArea(monbal);

    AreaDef futschool;
    futschool.name = "Future School";
    futschool.location = xenoverse;
    futschool.type = ITEM_DIMENSION;
    futschool.roomRanges.emplace_back(15938, 15999);
    auto future_school = assembleArea(futschool);

    AreaDef udfhq;
    udfhq.name = "UDF Headquarters";
    udfhq.location = space;
    udfhq.type = ITEM_VEHICLE;
    udfhq.roomRanges.emplace_back(18000, 18059);
    auto udf_headquarters = assembleArea(udfhq);

    AreaDef hspire;
    hspire.name = "The Haven Spire";
    hspire.location = space;
    hspire.type = ITEM_VEHICLE;
    hspire.roomRanges.emplace_back(18300, 18341);
    auto haven_spire = assembleArea(hspire);

    AreaDef knoit;
    knoit.name = "Kame no Itto";
    knoit.location = space;
    knoit.type = ITEM_VEHICLE;
    knoit.roomRanges.emplace_back(18400, 18460);
    auto kame_no_itto = assembleArea(knoit);

    AreaDef neonirvana;
    neonirvana.name = "Neo Nirvana";
    neonirvana.location = space;
    neonirvana.type = ITEM_VEHICLE;
    neonirvana.roomRanges.emplace_back(13500, 13552);
    neonirvana.roomRanges.emplace_back(14782, 14790);
    auto neo_nirvana = assembleArea(neonirvana);

    AreaDef neohologram;
    neohologram.name = "Hologram Combat";
    neohologram.location = neo_nirvana;
    neohologram.roomRanges.emplace_back(13553, 13567);
    auto neo_hologram_combat = assembleArea(neohologram);

    AreaDef neonexusfield;
    neonexusfield.name = "Nexus Field";
    neonexusfield.location = neo_hologram_combat;
    neonexusfield.roomRanges.emplace_back(13568, 13612);
    auto neo_nexus_field = assembleArea(neonexusfield);

    AreaDef neonamekgrassyisland;
    neonamekgrassyisland.name = "Namek: Grassy Island";
    neonamekgrassyisland.location = neo_hologram_combat;
    neonamekgrassyisland.roomRanges.emplace_back(13613, 13657);
    auto neo_namek_grassy_island = assembleArea(neonamekgrassyisland);

    AreaDef neoslavemarket;
    neoslavemarket.name = "Slave Market";
    neoslavemarket.location = neo_hologram_combat;
    neoslavemarket.roomRanges.emplace_back(13658, 13702);
    auto neo_slave_market = assembleArea(neoslavemarket);

    AreaDef neokanassa;
    neokanassa.name = "Kanassa: Blasted Battlefield";
    neokanassa.location = neo_hologram_combat;
    neokanassa.roomRanges.emplace_back(13703, 13747);
    auto neo_kanassa_blasted_battlefield = assembleArea(neokanassa);

    AreaDef neosilentglade;
    neosilentglade.name = "Silent Glade";
    neosilentglade.location = neo_hologram_combat;
    neosilentglade.roomRanges.emplace_back(13748, 13792);
    auto neo_silent_glade = assembleArea(neosilentglade);

    AreaDef neohell;
    neohell.name = "Hell - Flat Plains";
    neohell.location = neo_hologram_combat;
    neohell.roomRanges.emplace_back(13793, 13837);
    auto neo_hell_flat_plains = assembleArea(neohell);

    AreaDef neosandydesert;
    neosandydesert.name = "Sandy Desert";
    neosandydesert.location = neo_hologram_combat;
    neosandydesert.roomRanges.emplace_back(13838, 13882);
    auto neo_sandy_desert = assembleArea(neosandydesert);

    AreaDef neotopicasnowfield;
    neotopicasnowfield.name = "Topica Snowfield";
    neotopicasnowfield.location = neo_hologram_combat;
    neotopicasnowfield.roomRanges.emplace_back(13883, 13927);
    auto neo_topica_snow_field = assembleArea(neotopicasnowfield);

    AreaDef neogerolab;
    neogerolab.name = "Gero's Lab";
    neogerolab.location = neo_hologram_combat;
    neogerolab.roomRanges.emplace_back(13928, 14517);
    auto neo_geros_lab = assembleArea(neogerolab);

    AreaDef neocandyland;
    neocandyland.name = "Candy Land";
    neocandyland.location = neo_hologram_combat;
    neocandyland.roomRanges.emplace_back(14518, 14562);
    auto neo_candy_land = assembleArea(neocandyland);

    AreaDef neoancestralmountains;
    neoancestralmountains.name = "Ancestral Mountains";
    neoancestralmountains.location = neo_hologram_combat;
    neoancestralmountains.roomRanges.emplace_back(14563, 14607);
    auto neo_ancestral_mountains = assembleArea(neoancestralmountains);

    AreaDef neoelzthuanforest;
    neoelzthuanforest.name = "Elzthuan Forest";
    neoelzthuanforest.location = neo_hologram_combat;
    neoelzthuanforest.roomRanges.emplace_back(14608, 14652);
    auto neo_elzthuan_forest = assembleArea(neoelzthuanforest);

    AreaDef neoyardracity;
    neoyardracity.name = "Yardra City";
    neoyardracity.location = neo_hologram_combat;
    neoyardracity.roomRanges.emplace_back(14653, 14697);
    auto neo_yardra_city = assembleArea(neoyardracity);

    AreaDef neoancientcoliseum;
    neoancientcoliseum.name = "Ancient Coliseum";
    neoancientcoliseum.location = neo_hologram_combat;
    neoancientcoliseum.roomRanges.emplace_back(14698, 14742);
    auto neo_ancient_coliseum = assembleArea(neoancientcoliseum);

    AreaDef fortrancomplex;
    fortrancomplex.name = "Fortran Complex";
    fortrancomplex.location = neo_nirvana;
    fortrancomplex.roomRanges.emplace_back(14743, 14772);
    auto fortran_complex = assembleArea(fortrancomplex);

    AreaDef revolutionpark;
    revolutionpark.name = "Revolution Park";
    revolutionpark.location = neo_nirvana;
    revolutionpark.roomRanges.emplace_back(14773, 14802);
    auto revolution_park = assembleArea(revolutionpark);

    AreaDef akatsukilabs;
    akatsukilabs.name = "Akatsuki Labs";
    akatsukilabs.location = neo_nirvana;
    akatsukilabs.roomRanges.emplace_back(14800, 14893);
    auto akatsuki_labs = assembleArea(akatsukilabs);

    AreaDef southgal;
    southgal.name = "South Galaxy";
    southgal.location = mortal_plane;
    southgal.roomIDs = {64300, 64399};
    auto south_galaxy = assembleArea(southgal);

    AreaDef undergroundpassage;
    undergroundpassage.name = "Underground Passage";
    undergroundpassage.location = planet_namek;
    undergroundpassage.roomRanges.emplace_back(12869, 12899);
    auto underground_passage = assembleArea(undergroundpassage);

    AreaDef shatplan;
    shatplan.name = "Shattered Planet";
    shatplan.location = south_galaxy;
    shatplan.type = ITEM_CELESTIAL_BODY;
    shatplan.roomRanges.emplace_back(64301, 64399);
    auto shattered_planet = assembleArea(shatplan);

    AreaDef wzdef;
    wzdef.name = "War Zone";
    wzdef.location = xenoverse;
    wzdef.type = ITEM_BUILDING;
    wzdef.roomRanges.emplace_back(17700, 17702);
    auto war_zone = assembleArea(wzdef);

    AreaDef corlight;
    corlight.name = "Corridor of Light";
    corlight.location = war_zone;
    corlight.roomRanges.emplace_back(17703, 17722);
    auto corridor_of_light = assembleArea(corlight);

    AreaDef cordark;
    cordark.name = "Corridor of Darkness";
    cordark.location = war_zone;
    cordark.roomRanges.emplace_back(17723, 17743);
    auto corridor_of_darkness = assembleArea(cordark);

    AreaDef soisland;
    soisland.name = "South Ocean Island";
    soisland.location = planet_earth;
    soisland.roomRanges.emplace_back(6700, 6758);
    auto south_ocean_island = assembleArea(soisland);

    AreaDef hhouse;
    hhouse.name = "Haunted House";
    hhouse.location = xenoverse;
    hhouse.type = ITEM_DIMENSION;
    hhouse.roomRanges.emplace_back(18600, 18693);
    auto haunted_house = assembleArea(hhouse);

    AreaDef roc;
    roc.name = "Random Occurences, WTF?";
    roc.location = xenoverse;
    roc.type = ITEM_DIMENSION;
    roc.roomRanges.emplace_back(18700, 18776);
    auto random_occurences = assembleArea(roc);

    AreaDef galstrong;
    galstrong.name = "Galaxy's Strongest Tournament";
    galstrong.location = space;
    galstrong.type = ITEM_SPACE_STATION;
    galstrong.roomRanges.emplace_back(17875, 17894);
    auto galaxy_strongest_tournament = assembleArea(galstrong);

    AreaDef arwater;
    arwater.name = "Arena - Water";
    arwater.location = galaxy_strongest_tournament;
    arwater.roomRanges.emplace_back(17800, 17824);
    auto arena_water = assembleArea(arwater);

    AreaDef arring;
    arring.name = "Arena - The Ring";
    arring.location = galaxy_strongest_tournament;
    arring.roomRanges.emplace_back(17825, 17849);
    auto arena_ring = assembleArea(arring);

    AreaDef arsky;
    arsky.name = "Arena - In the Sky";
    arsky.location = galaxy_strongest_tournament;
    arsky.roomRanges.emplace_back(17850, 17875);
    auto arena_sky = assembleArea(arsky);

    AreaDef stdef;
    stdef.name = "Structures";
    auto structures = assembleArea(stdef);

    AreaDef spdef;
    spdef.name = "Spaceships";
    auto spaceships = assembleArea(spdef);

    auto crunch_ship = [&](old_ship_data &data) {

        AreaDef sdata;
        sdata.name = data.name;
        sdata.roomIDs = data.vnums;
        sdata.type = ITEM_VEHICLE;
        sdata.location = world.at(data.location ? data.location.value() : 16694);
        return assembleArea(sdata);
    };

    for(auto &sd : gships) {
        crunch_ship(sd);
    }

    for(auto &sd : customs) {
        crunch_ship(sd);
    }

    // A very luxurious player custom home
    AreaDef dunnoHouse;
    dunnoHouse.name = "Dunno's House";
    dunnoHouse.location = xenoverse;
    dunnoHouse.roomIDs = {19009, 19010, 19011, 19012, 19013, 19014, 19015, 19016, 19017, 19018,
                          19019, 19020, 19021, 19022, 19023};
    auto dunno_house = assembleArea(dunnoHouse);

    // This looks like an unused old player home, seems like it's attached to Cherry Blossom Mountain?
    AreaDef mountainFortress;
    mountainFortress.name = "Mountaintop Fortress";
    mountainFortress.location = xenoverse;
    mountainFortress.roomIDs = {19025, 19026, 19027, 19028, 19029, 19030, 19031, 19032,
                                19033, 19034, 19035, 19036, 19037, 19038, 19024};
    auto mountain_fortress = assembleArea(mountainFortress);

    // Personal Ships / Pods...
    for(auto vn = 45000; vn <= 45199; vn++) {
        auto ovn = vn + 1000;
        auto o = obj_proto.find(ovn);
        if(o == obj_proto.end()) continue;
        old_ship_data shipData;
        shipData.name = o->second["strings"]["name"];
        shipData.ship_obj = ovn;
        shipData.vnums.insert(vn);
        shipData.hatch_room = vn;
        auto v = crunch_ship(shipData);
    }

    AreaDef sphouses;
    sphouses.name = "Small Player Houses";
    sphouses.location = structures;
    auto small_player_houses = assembleArea(sphouses);

    int count = 1;
    for(auto i = 18800; i != 18896; i += 4) {
        AreaDef house;
        house.name = fmt::format("Small Player House {}", count++);
        house.roomRanges.emplace_back(i, i+3);
        house.location = small_player_houses;
        assembleArea(house);
    }

    AreaDef mdhouses;
    mdhouses.name = "Deluxe Player Houses";
    mdhouses.location = structures;
    auto medium_player_houses = assembleArea(mdhouses);

    count = 1;
    for(auto i = 18900; i != 18995; i += 5) {
        AreaDef house;
        house.name = fmt::format("Deluxe Player House {}", count++);
        house.roomRanges.emplace_back(i, i+4);
        house.location = medium_player_houses;
        assembleArea(house);
    }

    AreaDef lphouses;
    lphouses.name = "Excellent Player Houses";
    lphouses.location = structures;
    auto large_player_houses = assembleArea(lphouses);

    count = 1;
    for(auto i = 19100; i != 19195; i += 5) {
        AreaDef house;
        house.name = fmt::format("Excellent Player House {}", count++);
        house.roomRanges.emplace_back(i, i+4);
        house.location = large_player_houses;
        assembleArea(house);
    }

    AreaDef pdimen;
    pdimen.name = "Personal Pocket Dimensions";
    auto personal_dimensions = assembleArea(pdimen);

    int counter = 1;
    for(auto vn = 19800; vn <= 19899; vn++) {
        AreaDef pdim;
        pdim.name = "Personal Pocket Dimension " + std::to_string(counter++);
        pdim.location = personal_dimensions;
        pdim.roomIDs.insert(vn);
        pdim.type = ITEM_DIMENSION;
        pdim.envVars[EnvVar::Gravity] = 1000.0;
        auto pd = assembleArea(pdim);
    }

    AreaDef misc;
    misc.name = "Miscellaneous";
    for(auto &[rv, u] : world) {
        if(auto room = dynamic_cast<Room*>(u); room && !room->area) {
            misc.roomIDs.insert(rv);
        }
    }
    auto misc_area = assembleArea(misc);

    for(auto r : {
        300, 800, 1150, 1180, 1287, 1428, 1456, 1506, 1636, 1710, 19510, 2141, 13020, // earth vnums
    	4264, 4300, 4351, 4400, 4600, 4800, 5100, 5150, 5165, 5200, 5500, 4944, // frigid vnums
        8006, 8300, 8400, 8447, 8500, 8600, 8700, 8800, 8900, 8954, 9200, 9700, 9855, 9864, 9900, 9949, // konack
        2226, 2600, 2616, 2709, 2800, 2899, 2615, // vegeta
        11600, 10182, 10474, 13300, 10203, 10922, 11600, // namek
        12010, 12103, 12300, 12400, 12480, 12010, // Aether
        14008, 14100, 14200, 14300, // yardrat
        17531, 7950, 17420, // cerria
        3412, 3520, 19600, // zenith
        14904, 15655, // kanassa
        16009, 16544, 16600 // Arlia
    }) {
        if(auto u = world.find(r); u != world.end()) {
            auto room = dynamic_cast<Room*>(u->second);
            room->setFlag(FlagType::Room, ROOM_LANDING);
        }
    }
}

static std::vector<std::pair<std::string, vnum>> characterToAccount;

void migrate_accounts() {
	// user files are stored in <cwd>/user/<folder>/<file>.usr so we'll need to do recursive iteration using
    // C++ std::filesystem...

    auto path = std::filesystem::current_path() / "user";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No user directory found, skipping account migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".usr") continue;
        auto accFile = p.path().parent_path().filename().string();

        // Open file for reading...
        std::ifstream file(p.path());

        // Step 1: create an ID for this account...
        auto id = account_data::getNextID();

        // Now let's get a new account_data...
        auto a = std::make_shared<account_data>();
        accounts[id] = a;

        // Moving forward, we assume that every account file is using the above structure and is valid.
        // Don't second-guess it, just process.

        // Line 1: Name (string)
        std::getline(file, a->name);

        // Line 2: Email Address (string)
        std::getline(file, a->email);

        // Line 3: password (clear text, will hash...)
        std::string pass;
        std::getline(file, pass);
        if(!a->setPassword(pass)) basic_mud_log("Error hashing %s's password: %s", a->name.c_str(), pass.c_str());

        // Line 4: slots (int)
        std::string slots;
        std::getline(file, slots);
        a->slots = std::stoi(slots);

        // Line 5: current RPP (int)
        std::string rpp;
        std::getline(file, rpp);
        a->rpp = std::stoi(rpp);

        // Now for the Character lines, they either contain a name, or they contain "Empty".
        // "Empty" is not a character. It's a placeholder for an empty slot.
        // For now, we will move through all five lines, and for any that do not contain "Empty",
        // we insert the character name into the characterToAccount map.

        for(int i = 0;i < 5; i++) {
            std::string charName;
            std::getline(file, charName);
            if(charName != "Empty") {
                characterToAccount.emplace_back(charName, id);
            }
        }

        // Line 11: adminLevel (int)
        std::string adminLevel;
        std::getline(file, adminLevel);
        a->adminLevel = std::stoi(adminLevel);

        // Line 12: customFile present (bool)
        std::string customFile;
        std::getline(file, customFile);
        bool custom = std::stoi(customFile);

        // if custom, then we want to open a sister file that ends in .cus in the same directory and read every line
        // beyond the first into a.customs.
        if(custom) {
            auto customPath = p.path();
            customPath.replace_extension(".cus");
            std::ifstream customFile(customPath);
            std::string line;
            std::getline(customFile, line); // skip the first line
            while(std::getline(customFile, line)) {
                a->customs.emplace_back(line);
            }
            customFile.close();
        }

        // Line 13: RPP bank (unused)
        std::string rppBank;
        std::getline(file, rppBank);
        auto bank = std::stoi(rppBank);
        file.close();
        a->vn = id;

    }
}

void migrate_characters() {
    // Unlike accounts, player files are indexed. However, unless their name showed up in an account,
    // there's no point migrating them.

    // The procedure we will use is: iterate through characterToAccount and attempt to load the character.
    // if we can load them, we'll convert them and bind them to the appropriate account.

    for(auto &[cname, accID] : characterToAccount) {
        auto ch = new PlayerCharacter();
        ch->script = std::make_shared<script_data>(ch);
        if(load_char(cname.c_str(), ch) < 0) {
            basic_mud_log("Error loading %s for account migration.", cname.c_str());
            delete ch;
            continue;
        }
        auto id = ch->getUID();
        auto p = std::make_shared<player_data>();
        p->id = id;
        players[id] = p;
        p->id = id;
        p->character = ch;
        p->name = ch->getName();
        auto a = accounts[accID];
        accounts[accID] = a;
        p->account = a;
        a->adminLevel = std::max(a->adminLevel, GET_ADMLEVEL(ch));
        a->characters.emplace_back(id);
        ch->location = world[ch->load_room];
        ch->was_in_room = ch->load_room;
        world[id] = ch;
    }

    // migrate sense files...
    auto path = std::filesystem::current_path() / "sense";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No sense directory found, skipping account migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".sen") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for sense migration.", name.c_str());
            continue;
        }
        auto pa = players[ch->getUID()];
        // The file contains a sequence of lines, with each line containing a number.
		// The number is the vnum of a mobile the player's sensed.
        // We will read each line and insert the vnum into the player's sensed list.
        std::ifstream file(p.path());
        std::string line;
        while(std::getline(file, line)) {
            try {
                auto vnum = std::stoi(line);
                if(mob_proto.contains(vnum)) pa->senseMemory.insert(vnum);
            } catch(...) {
                basic_mud_log("Error parsing %s for sense migration.", line.c_str());
            }
        }
        file.close();
    }

    path = std::filesystem::current_path() / "intro";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No intro directory found, skipping intro migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".itr") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for dub migration.", name.c_str());
            continue;
        }

        auto pa = players[ch->getUID()];

		// The file contains a series of lines.
        // Each line looks like: <name> <dub>
        // We will need to use findPlayer on name, and then save id->dub to ch->player_specials->dubNames.
        // ignore if <name> == "Gibbles"

        std::ifstream file(p.path());
        std::string line;
        while(std::getline(file, line)) {
            auto pos = line.find(' ');
            if(pos == std::string::npos) continue;
            auto name = line.substr(0, pos);
            auto dub = line.substr(pos + 1);
            if(name == "Gibbles") continue;
            auto pc = findPlayer(name);
            if(!pc) continue;
            pa->dubNames[pc->getUID()] = dub;
        }
    }

    path = std::filesystem::current_path() / "plrvars";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No intro directory found, skipping intro migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".mem") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for variable migration.", name.c_str());
            continue;
        }

        // The file contains a series of lines.
        // each line looks like this: <varname> <context> <data>
        // where varname is a single-word string, context is an integer, and data is a string - although it might be an
        // empty string.
        std::ifstream file(p.path());
        std::string line;
        while(std::getline(file, line)) {
            auto pos = line.find(' ');
            if(pos == std::string::npos) continue;
            auto varname = line.substr(0, pos);
            auto pos2 = line.find(' ', pos + 1);
            if(pos2 == std::string::npos) continue;
            auto context = line.substr(pos + 1, pos2 - pos - 1);
            auto data = line.substr(pos2 + 1);

            try {
                ch->script->addVar(varname, data);
            } catch(...) {
                basic_mud_log("Error parsing %s for variable migration.", line.c_str());
            }
        }
    }

    path = std::filesystem::current_path() / "plralias";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No plralias directory found, skipping alias migration.");
        return;
    }

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".alias") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for alias migration.", name.c_str());
            continue;
        }
        auto pa = players.find(ch->getUID());
        if(pa == players.end()) {
            basic_mud_log("Error loading %s for alias migration.", name.c_str());
            continue;
        }


        std::ifstream file(p.path());
        std::string line;

        // Alias files are stored as sequences of this:
        // "%d\n%s\n%d\n%s\n%d\n", in order: alias name string length (size_t), alias name string,
        // replacement string length  (size_t), replacement string, alias type (a bool)

        while(std::getline(file, line)) {
            auto &a = pa->second->aliases.emplace_back();
            std::getline(file, a.name);
            std::getline(file, line);
            std::getline(file, a.replacement);
            std::getline(file, line);
            a.type = atoi(line.c_str());
        }
    }

    path = std::filesystem::current_path() / "plrobjs";

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".new") continue;

        // use the file stem against findPlayer...
        auto name = p.path().stem().string();
        auto ch = findPlayer(name);
        if(!ch) {
            basic_mud_log("Error loading %s for object migration.", name.c_str());
            continue;
        }

    }
}

void migrate_db() {
    boot_db_legacy();

    migrate_grid();

    migrate_accounts();

    try {
        migrate_characters();
    } catch(std::exception &e) {
        basic_mud_log("Error migrating characters: %s", e.what());
    }
}



void run_migration() {
    game::init_locale();
    load_config();
    chdir("lib");
    migrate_db();

    runSave();
}