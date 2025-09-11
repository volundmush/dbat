#include <filesystem>
#include <memory>
#include <iostream>
#include <vector>
#include <tuple>
#include <fstream>
#include <thread>
#include <cstdlib>
#include <bsd/string.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <regex>

#include "dbat/Zone.h"
#include "dbat/CharacterUtils.h"
#include "dbat/CharacterPrototype.h"
#include "dbat/ObjectUtils.h"
#include "dbat/ObjectPrototype.h"
#include "dbat/DgScriptPrototype.h"
#include "dbat/RoomUtils.h"
#include "dbat/Destination.h"
#include "dbat/Area.h"
#include "dbat/comm.h"
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/constants.h"
#include "dbat/maputils.h"
#include "dbat/config.h"
#include "dbat/class.h"
#include "dbat/players.h"
#include "dbat/Account.h"
#include "dbat/act.item.h"
#include "dbat/pfdefaults.h"
#include "dbat/spell_parser.h"
#include "dbat/Shop.h"
#include "dbat/Guild.h"
#include "dbat/saveload.h"
#include "dbat/assemblies.h"
#include "dbat/vehicles.h"
#include "dbat/ansi.h"
#include "dbat/Help.h"
#include "dbat/utils.h"
#include "dbat/interpreter.h"
#include "dbat/Random.h"
#include "dbat/ID.h"
#include "dbat/Startup.h"

#include "dbat/const/Max.h"
#include "dbat/const/Filename.h"
#include "dbat/const/Condition.h"
#include "dbat/const/ContainerFlag.h"
#include "dbat/const/Environment.h"

#define Q_FIELD(x)  ((int) (x) / 32)
#define Q_BIT(x)    (1 << ((x) % 32))

#define IS_SET_AR(var, bit)       ((var)[Q_FIELD(bit)] & Q_BIT(bit))
#define SET_BIT_AR(var, bit)      ((var)[Q_FIELD(bit)] |= Q_BIT(bit))
#define REMOVE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] &= ~Q_BIT(bit))
#define TOGGLE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] = \
                                   (var)[Q_FIELD(bit)] ^ Q_BIT(bit))
#define IS_SET(flag, bit)  ((flag) & (bit))
#define SET_BIT(var, bit)  ((var) |= (bit))
#define REMOVE_BIT(var, bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit) ((var) ^= (bit))

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

#define TOROOM(room, dir) (get_room(room)->dir_option[dir] ? \
get_room(room)->dir_option[dir]->to_room : NOWHERE)

/* structure for the reset commands */
struct reset_com {
    char command{};   /* current command                      */

    bool if_flag{};    /* if TRUE: exe only if preceding exe'd */
    int arg1{};        /*                                      */
    int arg2{};        /* Arguments to the command             */
    int arg3{};        /*                                      */
    int arg4{};        /* room_max  default 0			*/
    int arg5{};           /* percentages variable                 */
    int line{};        /* line number this command appears on  */
    std::string sarg1;        /* string argument                      */
    std::string sarg2;        /* string argument                      */

    /*
     *  Commands:              *
     *  'M': Read a mobile     *
     *  'O': Read an object    *
     *  'G': Give obj to mob   *
     *  'P': Put obj in obj    *
     *  'G': Obj to char       *
     *  'E': Obj to char equip *
     *  'D': Set state of door *
     *  'T': Trigger command   *
     *  'V': Assign a variable *
    */
};

static std::unordered_map<zone_vnum, std::tuple<vnum, vnum>> zone_ranges;

static std::unordered_map<zone_vnum, std::vector<reset_com>> oldResetCommands;

/* real zone of room/mobile/object/shop given */
zone_vnum zone_for_vnum(vnum vznum) {
    for(auto &z : zone_ranges) if(vznum >= std::get<0>(z.second) && vznum <= std::get<1>(z.second)) return z.first;
    return NOWHERE;
}

constexpr int DB_BOOT_WLD = 0;
constexpr int DB_BOOT_MOB = 1;
constexpr int DB_BOOT_OBJ = 2;
constexpr int DB_BOOT_ZON = 3;
constexpr int DB_BOOT_SHP = 4;
constexpr int DB_BOOT_HLP = 5;
constexpr int DB_BOOT_TRG = 6;
constexpr int DB_BOOT_GLD = 7;

static bool converting = false;

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

static const std::unordered_map<int, WhereFlag> wheremap = {
    {20, WhereFlag::planet_earth},
    {21, WhereFlag::planet_vegeta},
    {22, WhereFlag::planet_frigid},
    {23, WhereFlag::planet_konack},
    {24, WhereFlag::planet_namek},
    {25, WhereFlag::neo_nirvana},
    {26, WhereFlag::afterlife},
    {27, WhereFlag::space},
    {30, WhereFlag::afterlife_hell},
    {32, WhereFlag::planet_aether},
    {33, WhereFlag::hyperbolic_time_chamber},
    {34, WhereFlag::pendulum_past},
    {37, WhereFlag::planet_yardrat},
    {38, WhereFlag::planet_kanassa},
    {39, WhereFlag::planet_arlia},
    {41, WhereFlag::earth_orbit},
    {42, WhereFlag::frigid_orbit},
    {43, WhereFlag::konack_orbit},
    {44, WhereFlag::namek_orbit},
    {45, WhereFlag::vegeta_orbit},
    {46, WhereFlag::aether_orbit},
    {47, WhereFlag::yardrat_orbit},
    {48, WhereFlag::kanassa_orbit},
    {49, WhereFlag::arlia_orbit},
    {50, WhereFlag::nebula},
    {51, WhereFlag::asteroid},
    {52, WhereFlag::wormhole},
    {53, WhereFlag::space_station},
    {54, WhereFlag::star},
    {55, WhereFlag::planet_cerria},
    {56, WhereFlag::cerria_orbit},
    {66, WhereFlag::moon_zenith},
    {68, WhereFlag::zenith_orbit}
};

static void convert_room(Room& r) {
    for(const auto& [oldflag, newflag] : wheremap) {
        if(r.room_flags[oldflag]) {
            r.where_flags.set(newflag);
            r.room_flags.set(oldflag, false);
        }
    }
}

static void convert_character(CharacterPrototype *c) {
    c->character_flags.set(CharacterFlag::tail, race::hasTail(c->race));

    if(c->mob_flags.get(31)) {
        c->model = AndroidModel::Absorb;
    }
    if(c->mob_flags.get(32)) {
        c->model = AndroidModel::Repair;
    }
}

static void convert_character(Character *c) {
    if(!c->isPC) {
        c->character_flags.set(CharacterFlag::tail, race::hasTail(c->race));

        if(c->mob_flags.get(31)) {
            c->model = AndroidModel::Absorb;
        }
        if(c->mob_flags.get(32)) {
            c->model = AndroidModel::Repair;
        }
        
    } else {
        c->character_flags.set(CharacterFlag::tail, c->player_flags.get(30));
        c->character_flags.set(CharacterFlag::cyber_right_arm, c->player_flags.get(46));
        c->character_flags.set(CharacterFlag::cyber_left_arm, c->player_flags.get(47));
        c->character_flags.set(CharacterFlag::cyber_right_leg, c->player_flags.get(48));
        c->character_flags.set(CharacterFlag::cyber_left_leg, c->player_flags.get(49));

        if(c->player_flags.get(41)) {
            c->model = AndroidModel::Absorb;
        }
        if(c->player_flags.get(42)) {
            c->model = AndroidModel::Repair;
        }
        if(c->player_flags.get(43)) {
            c->model = AndroidModel::Sense;
        }
    }
}

static void load_help(FILE *fl, char *name) {
    char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
    size_t entrylen;
    char line[READ_SIZE + 1], hname[READ_SIZE + 1], *scan;
    struct help_index_element el{};

    strlcpy(hname, name, sizeof(hname));

    get_one_line(fl, key);
    while (*key != '$') {
        strcat(key, "\r\n"); /* strcat: OK (READ_SIZE - "\n"  "\r\n" == READ_SIZE  1) */
        entrylen = strlcpy(entry, key, sizeof(entry));

        /* Read in the corresponding help entry. */
        get_one_line(fl, line);
        while (*line != '#' && entrylen < sizeof(entry) - 1) {
            entrylen += strlcpy(entry + entrylen, line, sizeof(entry) - entrylen);

            if (entrylen + 2 < sizeof(entry) - 1) {
                strcpy(entry + entrylen, "\r\n"); /* strcpy: OK (size checked above) */
                entrylen += 2;
            }
            get_one_line(fl, line);
        }

        if (entrylen >= sizeof(entry) - 1) {
            int keysize;
            const char *truncmsg = "\r\n*TRUNCATED*\r\n";

            strcpy(entry + sizeof(entry) - strlen(truncmsg) - 1,
                   truncmsg); /* strcpy: OK (assuming sane 'entry' size) */

            keysize = strlen(key) - 2;
            basic_mud_log("SYSERR: Help entry exceeded buffer space: %.*s", keysize, key);

            /* If we ran out of buffer space, eat the rest of the entry. */
            while (*line != '#')
                get_one_line(fl, line);
        }

        if (*line == '#') {
            if (sscanf(line, "#%d", &el.min_level) != 1) {
                basic_mud_log("SYSERR: Help entry does not have a min level. %s", key);
                el.min_level = 0;
            }
        }

        el.duplicate = 0;
        el.entry = entry;
        scan = one_word(key, next_key);

        while (*next_key) {
            el.keywords = next_key;
            help_table.emplace_back(el);
            el.duplicate++;
            scan = one_word(scan, next_key);
        }
        get_one_line(fl, key);
    }
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

/* Whom will we not trade with (bitvector for SHOP_TRADE_WITH()) */
constexpr int TRADE_NOGOOD = 0;
constexpr int TRADE_NOEVIL = 1;
constexpr int TRADE_NONEUTRAL = 2;

constexpr int TRADE_NOWIZARD = 3; // roshi
constexpr int TRADE_NOCLERIC = 4; // piccolo
constexpr int TRADE_NOROGUE = 5; // krane
constexpr int TRADE_NOFIGHTER = 6; // nail
constexpr int TRADE_NOHUMAN = 7; // human
constexpr int TRADE_NOICER = 8; // icer
constexpr int TRADE_NOSAIYAN = 9; // saiyan
constexpr int TRADE_NOKONATSU = 10; // konatsu
constexpr int TRADE_NONAMEK = 11; // namek
constexpr int TRADE_NOMUTANT = 12; // mutant
constexpr int TRADE_NOKANASSAN = 13; // kanassan
constexpr int TRADE_NOBIO = 14; // bio-android
constexpr int TRADE_NOANDROID = 15; // android
constexpr int TRADE_NODEMON = 16; // demon
constexpr int TRADE_NOMAJIN = 17; // majin
constexpr int TRADE_NOKAI = 18; // kai
constexpr int TRADE_NOTRUFFLE = 19; // truffle
constexpr int TRADE_NOMONK = 29; // bardock
constexpr int TRADE_NOPALADIN = 30; //ginyu
constexpr int TRADE_ONLYWIZARD = 32; // roshi
constexpr int TRADE_ONLYCLERIC = 33; // piccolo
constexpr int TRADE_ONLYROGUE = 34; // krane
constexpr int TRADE_ONLYFIGHTER = 35; // nail
constexpr int TRADE_ONLYMONK = 36; // bardock
constexpr int TRADE_ONLYPALADIN = 37; // ginyu
constexpr int TRADE_NOSORCERER = 38; // frieza
constexpr int TRADE_NODRUID = 39; //tapion
constexpr int TRADE_NOBARD = 40; //sixteen
constexpr int TRADE_NORANGER = 41; //dabura
constexpr int TRADE_NOBARBARIAN = 42; //kabito
constexpr int TRADE_ONLYSORCERER = 43; // sorcerer
constexpr int TRADE_ONLYDRUID = 44; // tapion
constexpr int TRADE_ONLYBARD = 45; //sixteen
constexpr int TRADE_ONLYRANGER = 46; //dabura
constexpr int TRADE_ONLYBARBARIAN = 47; //kabito
constexpr int TRADE_ONLYARCANE_ARCHER = 48; // jinto
constexpr int TRADE_ONLYARCANE_TRICKSTER = 49; // tsuna
constexpr int TRADE_ONLYARCHMAGE = 50; //kurzak

constexpr int TRADE_NOARCANE_ARCHER = 63; //jinto
constexpr int TRADE_NOARCANE_TRICKSTER = 64; // tsuna
constexpr int TRADE_NOARCHMAGE = 65; // kurzak

constexpr int TRADE_NOBROKEN = 78;

static void handle_org_who(org_data &g, bitvector_t with_who[]) {
    for(auto i = 0; i < 128; i++) {
        if(IS_SET_AR(with_who, i)) {
            switch(i) {
                case TRADE_NOGOOD:
                    g.not_alignment.set(MoralAlign::good);
                    break;
                case TRADE_NOEVIL:
                    g.not_alignment.set(MoralAlign::evil);
                    break;
                case TRADE_NONEUTRAL:
                    g.not_alignment.set(MoralAlign::neutral);
                    break;
                case TRADE_NOWIZARD:
                    g.not_sensei.set(Sensei::roshi);
                    break;
                case TRADE_ONLYWIZARD:
                    g.only_sensei.set(Sensei::roshi);
                    break;
                case TRADE_NOCLERIC:
                    g.not_sensei.set(Sensei::piccolo);
                    break;
                case TRADE_ONLYCLERIC:
                    g.only_sensei.set(Sensei::piccolo);
                    break;
                case TRADE_NOROGUE:
                    g.not_sensei.set(Sensei::crane);
                    break;
                case TRADE_ONLYROGUE:
                    g.only_sensei.set(Sensei::crane);
                    break;
                case TRADE_NOFIGHTER:
                    g.not_sensei.set(Sensei::nail);
                    break;
                case TRADE_ONLYFIGHTER:
                    g.only_sensei.set(Sensei::nail);
                    break;
                case TRADE_NOHUMAN:
                    g.not_race.set(Race::human);
                    break;
                case TRADE_NOICER:
                    g.not_race.set(Race::icer);
                    break;
                case TRADE_NOSAIYAN:
                    g.not_race.set(Race::saiyan);
                    break;
                case TRADE_NOKONATSU:
                    g.not_race.set(Race::konatsu);
                    break;
                case TRADE_NONAMEK:
                    g.not_race.set(Race::namekian);
                    break;
                case TRADE_NOMUTANT:
                    g.not_race.set(Race::mutant);
                    break;
                case TRADE_NOKANASSAN:
                    g.not_race.set(Race::kanassan);
                    break;
                case TRADE_NOBIO:
                    g.not_race.set(Race::bio_android);
                    break;
                case TRADE_NOANDROID:
                    g.not_race.set(Race::android);
                    break;
                case TRADE_NODEMON:
                    g.not_race.set(Race::demon);
                    break;
                case TRADE_NOMAJIN:
                    g.not_race.set(Race::majin);
                    break;
                case TRADE_NOKAI:
                    g.not_race.set(Race::kai);
                    break;
                case TRADE_NOTRUFFLE:
                    g.not_race.set(Race::tuffle);
                    break;
                case TRADE_NOMONK:
                    g.not_sensei.set(Sensei::bardock);
                    break;
                case TRADE_ONLYMONK:
                    g.only_sensei.set(Sensei::bardock);
                    break;
                case TRADE_NOPALADIN:
                    g.not_sensei.set(Sensei::ginyu);
                    break;
                case TRADE_ONLYPALADIN:
                    g.only_sensei.set(Sensei::ginyu);
                    break;
                case TRADE_NOSORCERER:
                    g.not_sensei.set(Sensei::frieza);
                    break;
                case TRADE_ONLYSORCERER:
                    g.only_sensei.set(Sensei::frieza);
                    break;
                case TRADE_NODRUID:
                    g.not_sensei.set(Sensei::tapion);
                    break;
                case TRADE_ONLYDRUID:
                    g.only_sensei.set(Sensei::tapion);
                    break;
                case TRADE_NOBARD:
                    g.not_sensei.set(Sensei::sixteen);
                    break;
                case TRADE_ONLYBARD:
                    g.only_sensei.set(Sensei::sixteen);
                    break;
                case TRADE_NORANGER:
                    g.not_sensei.set(Sensei::dabura);
                    break;
                case TRADE_ONLYRANGER:
                    g.only_sensei.set(Sensei::dabura);
                    break;
                case TRADE_NOBARBARIAN:
                    g.not_sensei.set(Sensei::kibito);
                    break;
                case TRADE_ONLYBARBARIAN:
                    g.only_sensei.set(Sensei::kibito);
                    break;
                case TRADE_ONLYARCANE_ARCHER:
                    g.only_sensei.set(Sensei::jinto);
                    break;
                case TRADE_NOARCANE_ARCHER:
                    g.not_sensei.set(Sensei::jinto);
                    break;
                case TRADE_ONLYARCANE_TRICKSTER:
                    g.only_sensei.set(Sensei::tsuna);
                    break;
                case TRADE_NOARCANE_TRICKSTER:
                    g.not_sensei.set(Sensei::tsuna);
                    break;
                case TRADE_ONLYARCHMAGE:
                    g.only_sensei.set(Sensei::kurzak);
                    break;
                case TRADE_NOARCHMAGE:
                    g.not_sensei.set(Sensei::kurzak);
                    break;
                
            }
        }
    }
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
            auto g = std::make_shared<Guild>();
            guild_index[temp] = g;

            GM_NUM(top_guild) = temp;

            get_line(gm_f, buf3);
            rv = sscanf(buf3, "%d %d", &t1, &t2);
            while (t1 > -1) {
                if (rv == 1) { /* old style guilds, only skills */
                    g->skills.set(t1);
                } else if (rv == 2) { /* new style guilds, skills and feats */
                    if (t2 == 1) {
                        g->skills.set(t1);
                    } else if (t2 == 2) {
                        g->feats.insert(t1);
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
            read_guild_line(gm_f, "%f", &g->charge, "GM_CHARGE");
            g->no_such_skill = fread_string(gm_f, buf2);
            g->not_enough_gold = fread_string(gm_f, buf2);

            read_guild_line(gm_f, "%d", &g->minlvl, "GM_MINLVL");
            read_guild_line(gm_f, "%d", &g->keeper, "GM_TRAINER");

            bitvector_t with_who[4];
            read_guild_line(gm_f, "%d", &with_who[0], "GM_WITH_WHO");

            read_guild_line(gm_f, "%d", &g->open, "GM_OPEN");
            read_guild_line(gm_f, "%d", &g->close, "GM_CLOSE");

            CREATE(buf, char, READ_SIZE);
            get_line(gm_f, buf);
            if (buf && *buf != '#' && *buf != '$') {
                p = buf;
                for (temp = 1; temp < 4; temp++) {
                    if (!p || !*p)
                        break;
                    if (sscanf(p, "%d", &val) != 1) {
                        basic_mud_log("SYSERR: Can't parse GM_WITH_WHO line in %s: '%s'", buf2, buf);
                        break;
                    }
                    with_who[temp] = val;
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
        handle_org_who(*g, with_who);
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
    bool done = false;

    snprintf(buf2, sizeof(buf2), "beginning of shop file %s", filename);

    while (!done) {
        buf = fread_string(shop_f, buf2);
        if (*buf == '#') {        /* New shop */
            sscanf(buf, "#%d\n", &temp);
            snprintf(buf2, sizeof(buf2)-1, "shop #%d in shop file %s", temp, filename);
            auto sh = std::make_shared<Shop>();
            shop_index[temp] = sh;
            free(buf);        /* Plug memory leak! */
            sh->vnum = temp;
            top_shop = temp;
            while(true) {
                read_line(shop_f, "%d", &shop_temp);
                if(shop_temp == -1) break;
                temp = (shop_vnum)shop_temp;
                if(obj_proto.count(temp)) sh->producing.push_back(temp);
            }

            read_line(shop_f, "%f", &SHOP_BUYPROFIT(top_shop));
            read_line(shop_f, "%f", &SHOP_SELLPROFIT(top_shop));

            while(true) {
                read_line(shop_f, "%d", &shop_temp);
                if(shop_temp == -1) break;
                auto &t = sh->type.emplace_back();
                t.type = shop_temp;
            }

            sh->no_such_item1 = read_shop_message(0, SHOP_NUM(top_shop), shop_f, buf2);
            sh->no_such_item2 = read_shop_message(1, SHOP_NUM(top_shop), shop_f, buf2);
            sh->do_not_buy = read_shop_message(2, SHOP_NUM(top_shop), shop_f, buf2);
            sh->missing_cash1 = read_shop_message(3, SHOP_NUM(top_shop), shop_f, buf2);
            sh->missing_cash2 = read_shop_message(4, SHOP_NUM(top_shop), shop_f, buf2);
            sh->message_buy = read_shop_message(5, SHOP_NUM(top_shop), shop_f, buf2);
            sh->message_sell = read_shop_message(6, SHOP_NUM(top_shop), shop_f, buf2);
            read_line(shop_f, "%d", &SHOP_BROKE_TEMPER(top_shop));
            
            bitvector_t bitvector;
            read_line(shop_f, "%d", &bitvector);
            for(auto i = 0; i < 2; i++) {
                if(IS_SET(bitvector, 1 << i)) {
                    sh->shop_flags.set(static_cast<ShopFlag>(i));
                }
            }

            read_line(shop_f, "%d", &SHOP_KEEPER(top_shop));

            bitvector_t with_who[4];

            CREATE(buf, char, READ_SIZE);
            get_line(shop_f, buf);
            p = buf;
            for (temp = 0; temp < 4; temp++) {
                if (!p || !*p)
                    break;
                if (sscanf(p, "%d", &count) != 1) {
                    basic_mud_log("SYSERR: Can't parse TRADE_WITH line in %s: '%s'", buf2, buf);
                    break;
                }
                with_who[temp] = count;
                while (isdigit(*p) || *p == '-') {
                    p++;
                }
                while (*p && !(isdigit(*p) || *p == '-')) {
                    p++;
                }
            }
            free(buf);
            handle_org_who(*sh, with_who);

            while(true) {
                read_line(shop_f, "%d", &shop_temp);
                if(shop_temp == -1) break;
                if(Room::registry.contains(shop_temp)) sh->in_room.insert(shop_temp);

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

static int check_object_spell_number(struct Object *obj, const char* val) {
    int error = false;
    const char *spellname;

    if (GET_OBJ_VAL(obj, val) == -1 ||
        GET_OBJ_VAL(obj, val) == 0)    /* i.e.: no spell */
        return (error);

    /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
    if (GET_OBJ_VAL(obj, val) < 0)
        error = true;
    if (GET_OBJ_VAL(obj, val) >= SKILL_TABLE_SIZE)
        error = true;
    if (skill_type(GET_OBJ_VAL(obj, val)) != SKTYPE_SPELL)
        error = true;
    if (error)
        basic_mud_log("SYSERR: Object #%d (%s) has out of range spell #%d.",
            GET_OBJ_VNUM(obj), obj->getShortDescription(), GET_OBJ_VAL(obj, val));

    /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
                                                                                                                            if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
    log("... '%s' (#%d) uses %s spell '%s'.",
	obj->short_description,	GET_OBJ_VNUM(obj),
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
	skill_name(GET_OBJ_VAL(obj, val)));
#endif

    if (scheck)        /* Spell names don't exist in syntax check mode. */
        return (error);

    /* Now check for unnamed spells. */
    spellname = skill_name(GET_OBJ_VAL(obj, val));

    if ((spellname == unused_spellname || !strcasecmp("UNDEFINED", spellname)) && (error = true))
        basic_mud_log("SYSERR: Object #%d (%s) uses '%s' spell #%d.",
            GET_OBJ_VNUM(obj), obj->getShortDescription(), spellname,
            GET_OBJ_VAL(obj, val));

    return (error);
}

static int check_object_level(struct Object *obj, const char* val) {
    int error = false;

    if ((GET_OBJ_VAL(obj, val) < 0) && (error = true))
        basic_mud_log("SYSERR: Object #%d (%s) has out of range level #%d.",
            GET_OBJ_VNUM(obj), obj->getShortDescription(), GET_OBJ_VAL(obj, val));

    return (error);
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

int load_inv_backup(struct Character *ch) {
    if (GET_LEVEL(ch) < 2)
        return (-1);

    char chx;
    FILE *source, *target;
    char source_file[20480], target_file[20480], buf2[20480];
    char alpha[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
    sprintf(name, "%s", GET_NAME(ch));

    if (name[0] == 'a' || name[0] == 'A' || name[0] == 'b' || name[0] == 'B' || name[0] == 'c' || name[0] == 'C' ||
        name[0] == 'd' || name[0] == 'D' || name[0] == 'e' || name[0] == 'E') {
        sprintf(alpha, "A-E");
    } else if (name[0] == 'f' || name[0] == 'F' || name[0] == 'g' || name[0] == 'G' || name[0] == 'h' ||
               name[0] == 'H' || name[0] == 'i' || name[0] == 'I' || name[0] == 'j' || name[0] == 'J') {
        sprintf(alpha, "F-J");
    } else if (name[0] == 'k' || name[0] == 'K' || name[0] == 'l' || name[0] == 'L' || name[0] == 'm' ||
               name[0] == 'M' || name[0] == 'n' || name[0] == 'N' || name[0] == 'o' || name[0] == 'O') {
        sprintf(alpha, "K-O");
    } else if (name[0] == 'p' || name[0] == 'P' || name[0] == 'q' || name[0] == 'Q' || name[0] == 'r' ||
               name[0] == 'R' || name[0] == 's' || name[0] == 'S' || name[0] == 't' || name[0] == 'T') {
        sprintf(alpha, "P-T");
    } else if (name[0] == 'u' || name[0] == 'U' || name[0] == 'v' || name[0] == 'V' || name[0] == 'w' ||
               name[0] == 'W' || name[0] == 'x' || name[0] == 'X' || name[0] == 'y' || name[0] == 'Y' ||
               name[0] == 'z' || name[0] == 'Z') {
        sprintf(alpha, "U-Z");
    }

    sprintf(source_file, "plrobjs" SLASH"%s" SLASH"%s.copy",
            alpha, ch->getName());
    if (!get_filename(buf2, sizeof(buf2), NEW_OBJ_FILES, GET_NAME(ch)))
        return -1;
    sprintf(target_file, "%s", buf2);

    if (!(source = fopen(source_file, "r"))) {
        basic_mud_log("Source in load_inv_backup failed to load.");
        basic_mud_log(source_file);
        return -1;
    }

    if (!(target = fopen(target_file, "w"))) {
        basic_mud_log("Target in load_inv_backup failed to load.");
        basic_mud_log(target_file);
        return -1;
    }

    while ((chx = fgetc(source)) != EOF)
        fputc(chx, target);

    basic_mud_log("Inventory backup restore successful.");

    fclose(source);
    fclose(target);

    return 1;
}

static int inv_backup(struct Character *ch) {
    FILE *backup;
    char buf[20480];

    char alpha[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
    sprintf(name, "%s", GET_NAME(ch));

    if (name[0] == 'a' || name[0] == 'A' || name[0] == 'b' || name[0] == 'B' || name[0] == 'c' || name[0] == 'C' ||
        name[0] == 'd' || name[0] == 'D' || name[0] == 'e' || name[0] == 'E') {
        sprintf(alpha, "A-E");
    } else if (name[0] == 'f' || name[0] == 'F' || name[0] == 'g' || name[0] == 'G' || name[0] == 'h' ||
               name[0] == 'H' || name[0] == 'i' || name[0] == 'I' || name[0] == 'j' || name[0] == 'J') {
        sprintf(alpha, "F-J");
    } else if (name[0] == 'k' || name[0] == 'K' || name[0] == 'l' || name[0] == 'L' || name[0] == 'm' ||
               name[0] == 'M' || name[0] == 'n' || name[0] == 'N' || name[0] == 'o' || name[0] == 'O') {
        sprintf(alpha, "K-O");
    } else if (name[0] == 'p' || name[0] == 'P' || name[0] == 'q' || name[0] == 'Q' || name[0] == 'r' ||
               name[0] == 'R' || name[0] == 's' || name[0] == 'S' || name[0] == 't' || name[0] == 'T') {
        sprintf(alpha, "P-T");
    } else if (name[0] == 'u' || name[0] == 'U' || name[0] == 'v' || name[0] == 'V' || name[0] == 'w' ||
               name[0] == 'W' || name[0] == 'x' || name[0] == 'X' || name[0] == 'y' || name[0] == 'Y' ||
               name[0] == 'z' || name[0] == 'Z') {
        sprintf(alpha, "U-Z");
    }

    sprintf(buf, "plrobjs" SLASH"%s" SLASH"%s.copy", alpha,
            ch->getName());

    if (!(backup = fopen(buf, "r")))
        return -1;

    fclose(backup);

    return 1;
}

static std::vector<std::tuple<Room*, Direction, room_vnum>> room_directions;




/* read direction data */
static void setup_dir(FILE *fl, Room *r, int dir) {
    int t[11] = {0}, retval = 0;
    char line[READ_SIZE], buf2[128];

    snprintf(buf2, sizeof(buf2), "room #%d, direction D%d", r->getVnum(), dir);

    auto cdir = static_cast<Direction>(dir);
    auto &de = r->exits[cdir];
    auto d = &de;

    d->dir = cdir;

    char *temp = fread_string(fl, buf2);
    if(temp) {
        d->general_description = temp;
        free(temp);
        temp = nullptr;
    }
    temp = fread_string(fl, buf2);
    if(temp) {
        d->keyword = temp;
        free(temp);
        temp = nullptr;
    }

    if (!get_line(fl, line)) {
        basic_mud_log("SYSERR: Format error, %s", buf2);
        exit(1);
    }
    if (((retval = sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7,
                          t + 8, t + 9, t + 10)) == 3) && (bitwarning == true)) {
        basic_mud_log("SYSERR: Format error, %s", buf2);
        exit(1);
    } else if (bitwarning == false) {

        if (t[0] == 1) {
            d->exit_flags.set(ExitFlag::isdoor);
        }
        else if (t[0] == 2) {
            d->exit_flags.set(ExitFlag::isdoor);
            d->exit_flags.set(ExitFlag::pickproof);
        }
        else if (t[0] == 3) {
            d->exit_flags.set(ExitFlag::isdoor);
            d->exit_flags.set(ExitFlag::secret);
        }
        else if (t[0] == 4) {
            d->exit_flags.set(ExitFlag::isdoor);
            d->exit_flags.set(ExitFlag::pickproof);
            d->exit_flags.set(ExitFlag::secret);
        }

        d->key = real_object(t[1]);
        
        // since the rooms don't exist yet, we can't set d->unit...
        room_directions.emplace_back(r, cdir, t[2]);
        //d->unit = get_room(t[2]);

        if (retval == 3) {
            basic_mud_log("Converting world files to include DC add ons.");
            d->dclock = 20;
            d->dchide = 20;
            if (bitsavetodisk) {
                converting = true;
            }
        } else if (retval == 5) {
            d->dclock = t[3];
            d->dchide = t[4];
            if (bitsavetodisk) {
                converting = true;
            }
        } else if (retval == 7) {
            d->dclock = t[3];
            d->dchide = t[4];
            if (bitsavetodisk) {
                converting = true;
            }
        } else if (retval == 11) {
            d->dclock = t[3];
            d->dchide = t[4];
        }
    }
}


/* load the rooms */
static void parse_room(FILE *fl, room_vnum virtual_nr) {
    int t[3] = {0}, i = 0, retval = 0;
    char line[READ_SIZE] = {0}, flags[128] = {0}, flags2[128] = {0}, flags3[128] = {0};
    char flags4[128] = {0}, buf2[MAX_STRING_LENGTH] = {0}, buf[128] = {0};
    bitvector_t roomFlagsHolder[4] = {0};
    struct extra_descr_data *new_descr = nullptr;
    char letter = 0;

    /* This really had better fit or there are other problems. */
    snprintf(buf2, sizeof(buf2), "room #%d", virtual_nr);

    auto zone = zone_for_vnum(virtual_nr);
    if (zone == NOWHERE) {
        basic_mud_log("SYSERR: Room #%d is outside any zone.", virtual_nr);
        exit(1);
    }

    if(Room::registry.count(virtual_nr)) {
        basic_mud_log("SYSERR: Room #%d already exists, cannot parse!", virtual_nr);
        exit(1);
    }
    auto &z = zone_table.at(zone);
    auto sh = std::make_shared<Room>();
    auto r = sh.get();
    Room::registry.emplace(virtual_nr, sh);
    z->rooms.add(sh);

    r->zone.reset(z.get());
    r->vn = virtual_nr;
    r->name = fread_string(fl, buf2);
    r->look_description = fread_string(fl, buf2);

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

        for(auto i = 0; i < NUM_ROOM_FLAGS; i++) if(IS_SET_AR(roomFlagsHolder, i)) r->room_flags.set(i);

        r->sector_type = static_cast<SectorType>(t[2]);
        sprintf(flags, "object #%d", virtual_nr);    /* sprintf: OK (until 399-bit integers) */
        //check_bitvector_names(r.room_flags, room_bits_count, flags, "room");
    } else {
        basic_mud_log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
        exit(1);
    }

    snprintf(buf, sizeof(buf), "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);
    convert_room(*r);
    while(true) {
        if (!get_line(fl, line)) {
            basic_mud_log("%s", buf);
            exit(1);
        }
        switch (*line) {
            case 'D':
                setup_dir(fl, r, atoi(line + 1));
                break;
            case 'E': {
                auto &ex = r->extra_descriptions.emplace_back();
                ex.first = fread_string(fl, buf2);
                ex.second = fread_string(fl, buf2);
                if(!ex.second.ends_with("\r\n")) {
                    ex.second += "\r\n"; /* ensure it ends with \r\n */
                }
            }
                break;
            case 'S':            /* end of room */
                /* DG triggers -- script is defined after the end of the room */
                letter = fread_letter(fl);
                ungetc(letter, fl);
                while (letter == 'T') {
                    dg_read_trigger(fl, (HasProtoScript*)get_room(virtual_nr), WLD_TRIGGER);
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

static void mob_autobalance(struct CharacterPrototype *ch) {

}

static int parse_simple_mob(FILE *mob_f, struct CharacterPrototype *ch, mob_vnum nr) {
    int j, t[10];
    char line[READ_SIZE];

    if (!get_line(mob_f, line)) {
        basic_mud_log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
        return 0;
    }

    if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
               t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
        basic_mud_log("SYSERR: Format error in mob #%d, first line after S flag\n"
            "...expecting line of form '# # # #d#+# #d#+#'", nr);
        return 0;
    }

    ch->setBaseStat("level", t[0]);

    /* max hit = 0 is a flag that H, M, V is xdy+z */
    ch->setBaseStat("health", t[3]);
    ch->setBaseStat("ki", t[4]);
    ch->setBaseStat("stamina", t[5]);

    ch->setBaseStat("damage_mod", t[8]);

    if (!get_line(mob_f, line)) {
        basic_mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
            "...expecting line of form '# #', but file ended!", nr);
        return 0;
    }

    if (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
        basic_mud_log("SYSERR: Format error in mob #%d, second line after S flag\n"
            "...expecting line of form '# # # #'", nr);
        return 0;
    }

    ch->setBaseStat("money_carried", t[0]);
    auto race = t[2]+1;
    if(race > 23) race = 0;
    ch->race = static_cast<Race>(race);
    auto sen = t[3]+1;
    if(sen > 14) sen = 0;
    ch->sensei = static_cast<Sensei>(sen);

    /* GET_CLASS_RANKS(ch, t[3]) = GET_LEVEL(ch); */

    if (!IS_HUMAN(ch))
        ch->affect_flags.set(AFF_INFRAVISION);

    if (!get_line(mob_f, line)) {
        basic_mud_log("SYSERR: Format error in last line of mob #%d\n"
            "...expecting line of form '# # #', but file ended!", nr);
        return 0;
    }

    if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
        basic_mud_log("SYSERR: Format error in last line of mob #%d\n"
            "...expecting line of form '# # #'", nr);
        return 0;
    }

    ch->position = static_cast<Position>(t[1]);
    ch->sex = static_cast<Sex>(t[2]);

    //set_height_and_weight_by_race(ch);

    if (MOB_FLAGGED(ch, MOB_AUTOBALANCE)) {
        mob_autobalance(ch);
    }

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
    if (value && !matched && !strcasecmp(keyword, test) && (matched = true))

#define BOOL_CASE(test)    \
    if (!value && !matched && !strcasecmp(keyword, test) && (matched = true))

#define RANGE(low, high)    \
    (num_arg = std::clamp(num_arg, low, high))

static void interpret_espec(const char *keyword, const char *value, struct CharacterPrototype *ch, mob_vnum nr) {
    int num_arg = 0, matched = false;
    int num, num2, num3, num4, num5, num6;
    struct affected_type af;

    /*
   * If there isn't a colon, there is no value.  While Boolean options are
   * possible, we don't actually have any.  Feel free to make some.
  */
    if (value)
        num_arg = atoi(value);

    CASE("Size") {
        RANGE(SIZE_UNDEFINED, NUM_SIZES - 1);
        ch->size = static_cast<Size>(num_arg);
    }

    CASE("Str") {
        RANGE(0, 200);
        ch->setBaseStat("strength", num_arg);
    }

    CASE("StrAdd") {
        basic_mud_log("mob #%d trying to set StrAdd, rebalance its strength.",
            ch->vn);
    }

    CASE("Int") {
        RANGE(0, 200);
        ch->setBaseStat("intelligence", num_arg);
    }

    CASE("Wis") {
        RANGE(0, 200);
        ch->setBaseStat("wisdom", num_arg);
    }

    CASE("Dex") {
        RANGE(0, 200);
        ch->setBaseStat("agility", num_arg);
    }

    CASE("Con") {
        RANGE(0, 200);
        ch->setBaseStat("constitution", num_arg);
    }

    CASE("Cha") {
        RANGE(0, 200);
        ch->setBaseStat("speed", num_arg);
    }

    CASE("Hit") {
        RANGE(0, 99999);
        //GET_HIT(ch) = num_arg;
    }

    CASE("Mana") {
        RANGE(0, 99999);
        //GET_MANA(ch) = num_arg;
    }

    CASE("Moves") {
        RANGE(0, 99999);
        //GET_MOVE(ch) = num_arg;
    }

    CASE("Feat") {
        sscanf(value, "%d %d", &num, &num2);
        //HAS_FEAT(ch, num) = num2;
    }

    CASE("Skill") {
        sscanf(value, "%d %d", &num, &num2);
        //SET_SKILL(ch, num, num2);
    }

    CASE("SkillMod") {
        sscanf(value, "%d %d", &num, &num2);
        //SET_SKILL_BONUS(ch, num, num2);
    }

    if (!matched) {
        basic_mud_log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
            keyword, nr);
    }
}

#undef CASE
#undef BOOL_CASE
#undef RANGE

static void parse_espec(char *buf, struct CharacterPrototype *ch, mob_vnum nr) {
    char *ptr;

    if ((ptr = strchr(buf, ':')) != nullptr) {
        *(ptr++) = '\0';
        while (isspace(*ptr))
            ptr++;
    }
    interpret_espec(buf, ptr, ch, nr);
}

static void mob_stats(struct CharacterPrototype *mob) {
    int start = GET_LEVEL(mob) * 0.5, finish = GET_LEVEL(mob);

    if (finish < 20)
        finish = 20;

    std::unordered_map<std::string, int> setTo;

    if (!IS_HUMANOID(mob)) {
        setTo["strength"] = Random::get<int>(start, finish);
        setTo["intelligence"] = Random::get<int>(start, finish) - 30;
        setTo["wisdom"] = Random::get<int>(start, finish) - 30;
        setTo["agility"] = Random::get<int>(start + 5, finish);
        setTo["constitution"] = Random::get<int>(start + 5, finish);
        setTo["speed"] = Random::get<int>(start, finish);
    } else {
        if (IS_SAIYAN(mob)) {
            setTo["strength"] = Random::get<int>(start + 10, finish);
            setTo["intelligence"] = Random::get<int>(start, finish - 10);
            setTo["wisdom"] = Random::get<int>(start, finish - 5);
            setTo["agility"] = Random::get<int>(start, finish);
            setTo["constitution"] = Random::get<int>(start + 5, finish);
            setTo["speed"] = Random::get<int>(start + 5, finish);
        } else if (IS_KONATSU(mob)) {
            setTo["strength"] = Random::get<int>(start, finish - 10);
            setTo["intelligence"] = Random::get<int>(start, finish);
            setTo["wisdom"] = Random::get<int>(start, finish);
            setTo["agility"] = Random::get<int>(start + 10, finish);
            setTo["constitution"] = Random::get<int>(start, finish);
            setTo["speed"] = Random::get<int>(start, finish);
        } else if (IS_ANDROID(mob)) {
            setTo["strength"] = Random::get<int>(start, finish);
            setTo["intelligence"] = Random::get<int>(start, finish);
            setTo["wisdom"] = Random::get<int>(start, finish - 10);
            setTo["agility"] = Random::get<int>(start, finish);
            setTo["constitution"] = Random::get<int>(start, finish);
            setTo["speed"] = Random::get<int>(start, finish);
        } else if (IS_MAJIN(mob)) {
            setTo["strength"] = Random::get<int>(start, finish);
            setTo["intelligence"] = Random::get<int>(start, finish - 10);
            setTo["wisdom"] = Random::get<int>(start, finish - 5);
            setTo["agility"] = Random::get<int>(start, finish);
            setTo["constitution"] = Random::get<int>(start + 15, finish);
            setTo["speed"] = Random::get<int>(start, finish);
        } else if (IS_TRUFFLE(mob)) {
            setTo["strength"] = Random::get<int>(start, finish - 10);
            setTo["intelligence"] = Random::get<int>(start + 15, finish);
            setTo["wisdom"] = Random::get<int>(start, finish);
            setTo["agility"] = Random::get<int>(start, finish);
            setTo["constitution"] = Random::get<int>(start, finish);
            setTo["speed"] = Random::get<int>(start, finish);
        } else if (IS_ICER(mob)) {
            setTo["strength"] = Random::get<int>(start + 5, finish);
            setTo["intelligence"] = Random::get<int>(start, finish);
            setTo["wisdom"] = Random::get<int>(start, finish);
            setTo["agility"] = Random::get<int>(start, finish);
            setTo["constitution"] = Random::get<int>(start, finish);
            setTo["speed"] = Random::get<int>(start + 10, finish);
        } else {
            setTo["strength"] = Random::get<int>(start, finish);
            setTo["intelligence"] = Random::get<int>(start, finish);
            setTo["wisdom"] = Random::get<int>(start, finish);
            setTo["agility"] = Random::get<int>(start, finish);
            setTo["constitution"] = Random::get<int>(start, finish);
            setTo["speed"] = Random::get<int>(start, finish);
        }
    }

    for(auto &[attr, val] : setTo) {
        if(val > 100) {
            val = 100;
        } else if(val < 5) {
            val = Random::get<int>(5, 8);
        }
        mob->setBaseStat(attr, val);
    }
}

static int parse_enhanced_mob(FILE *mob_f, struct CharacterPrototype *ch, mob_vnum nr) {
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

static int parse_mobile_from_file(FILE *mob_f, struct CharacterPrototype *ch, vnum nr) {
    int j, t[10], retval;
    char line[READ_SIZE], *tmpptr, letter;
    char f1[128], f2[128], f3[128], f4[128], f5[128], f6[128];
    char f7[128], f8[128], buf2[128];
    ch->vn = nr;

    /*
   * Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg 2/25/98
   */

    sprintf(buf2, "mob vnum %d", nr);   /* sprintf: OK (for 'buf2 >= 19') */

    /***** String data *****/
    ch->name = fread_string(mob_f, buf2);
    ch->short_description = fread_string(mob_f, buf2);
    tmpptr = (char*)ch->short_description.c_str();
    if (tmpptr && *tmpptr)
        if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
            !strcasecmp(fname(tmpptr), "the"))
            *tmpptr = tolower(*tmpptr);
    ch->room_description = fread_string(mob_f, buf2);
    ch->look_description = fread_string(mob_f, buf2);

    /* *** Numeric data *** */
    if (!get_line(mob_f, line)) {
        basic_mud_log("SYSERR: Format error after string section of mob #%d\n"
            "...expecting line of form '# # # {S | E}', but file ended!", nr);
        return 0;
    }

    if ((retval = sscanf(line, "%s %s %s %s %s %s %s %s %d %c", f1, f2, f3, f4, f5, f6, f7, f8, t + 2, &letter)) == 10) {
        int taeller;

        bitvector_t mf[4], aff[4];

        mf[0] = asciiflag_conv(f1);
        mf[1] = asciiflag_conv(f2);
        mf[2] = asciiflag_conv(f3);
        mf[3] = asciiflag_conv(f4);
        for(auto i = 0; i < NUM_MOB_FLAGS; i++) ch->mob_flags.set(i, IS_SET_AR(mf, i));

        aff[0] = asciiflag_conv(f5);
        aff[1] = asciiflag_conv(f6);
        aff[2] = asciiflag_conv(f7);
        aff[3] = asciiflag_conv(f8);
        for(auto i = 0; i < 128; i++) ch->affect_flags.set(i, IS_SET_AR(aff, i));

        ch->setBaseStat("good_evil", t[2]);

    } else {
        basic_mud_log("SYSERR: Format error after string section of mob #%d\n"
            "...expecting line of form '# # # {S | E}'", nr);
        exit(1);
    }

    if (MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
        /* Rather bad to load mobiles with this bit already set. */
        basic_mud_log("SYSERR: Mob #%d has reserved bit MOB_NOTDEADYET set.", nr);
        ch->mob_flags.set(MOB_NOTDEADYET, false);
    }

    /* AGGR_TO_ALIGN is ignored if the mob is AGGRESSIVE.
  if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL))
    log("SYSERR: Mob #%d both Aggressive and Aggressive_to_Alignment.", nr); */

    /* Convert mobs to use AUTOBALANCE. Uncomment and reboot to flag all mobs AUTOBALANCE.
   * if (!MOB_FLAGGED(ch, MOB_AUTOBALANCE)) {
   *   SET_BIT_AR(MOB_FLAGS(ch), MOB_AUTOBALANCE);
   * } */

    switch (toupper(letter)) {
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
        dg_read_trigger(mob_f, ch, MOB_TRIGGER);
        letter = fread_letter(mob_f);
        ungetc(letter, mob_f);
    }

    /* Uncomment to force all mob files to be rewritten. Good for initial AUTOBALANCE setup.
   * if (bitsavetodisk) {
   *   add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 0);
   *   converting = TRUE;
   * } */

    convert_character(ch);

    return 1;
}


static void parse_mobile(FILE *mob_f, mob_vnum nr) {

    auto sh = std::make_shared<CharacterPrototype>();
    mob_proto[nr] = sh;

    if (parse_mobile_from_file(mob_f, sh.get(), nr)) {

    } else { /* We used to exit in the file reading code, but now we do it here */
        exit(1);
    }
}
template<typename T>
static void obj_values(T* obj, int64_t old_value[]) {
    // the basic shared values...
    if(old_value[4]) obj->setBaseStat(VAL_ALL_HEALTH, old_value[4]);
    if(old_value[5]) obj->setBaseStat(VAL_ALL_MAXHEALTH, old_value[5]);
    if(old_value[7]) obj->setBaseStat(VAL_ALL_MATERIAL, old_value[7]);

    switch(obj->type_flag) {
        case ITEM_LIGHT:
            if(old_value[0]) obj->setBaseStat(VAL_LIGHT_TIME, old_value[0]);
            if(old_value[2]) obj->setBaseStat(VAL_LIGHT_HOURS, old_value[2]);
            break;
        case ITEM_SCROLL:
        case ITEM_WAND:
        case ITEM_POTION:
            if(old_value[0]) obj->setBaseStat(VAL_SCROLL_LEVEL, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_SCROLL_SPELL1, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_SCROLL_SPELL2, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_SCROLL_SPELL3, old_value[3]);
            break;
        case ITEM_STAFF:
            if(old_value[0]) obj->setBaseStat(VAL_STAFF_LEVEL, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_STAFF_MAXCHARGES, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_STAFF_CHARGES, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_STAFF_SPELL, old_value[3]);
            break;
        case ITEM_WEAPON:
            if(old_value[0]) obj->setBaseStat(VAL_WEAPON_SKILL, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_WEAPON_DAMDICE, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_WEAPON_DAMSIZE, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_WEAPON_DAMTYPE, old_value[3]);
            if(old_value[6]) obj->setBaseStat(VAL_WEAPON_CRITTYPE, old_value[6]);
            if(old_value[8]) obj->setBaseStat(VAL_WEAPON_CRITRANGE, old_value[8]);
            if(old_value[9]) obj->setBaseStat(VAL_WEAPON_LEVEL, old_value[9]);
            break;
        case ITEM_ARMOR:
            if(old_value[0]) obj->setBaseStat(VAL_ARMOR_APPLYAC, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_ARMOR_SKILL, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_ARMOR_MAXDEXMOD, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_ARMOR_CHECK, old_value[3]);
            if(old_value[6]) obj->setBaseStat(VAL_ARMOR_SPELLFAIL, old_value[6]);
            break;
        case ITEM_WORN:
            if(old_value[15]) obj->setBaseStat(VAL_WORN_SCOUTER, old_value[15]);
            break;
        case ITEM_OTHER:
            if(old_value[6]) obj->setBaseStat(VAL_OTHER_SERAF, old_value[6]);
            if(old_value[8]) obj->setBaseStat(VAL_OTHER_SOILQUALITY, old_value[8]);
            break;
        case ITEM_TRAP:
            if(old_value[0]) obj->setBaseStat(VAL_TRAP_SPELL, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_TRAP_HITPOINTS, old_value[1]);
            break;
        case ITEM_CONTAINER:
            if(old_value[0]) obj->setBaseStat(VAL_CONTAINER_CAPACITY, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_CONTAINER_FLAGS, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_CONTAINER_KEY, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_CONTAINER_CORPSE, old_value[3]);
            if(old_value[8]) obj->setBaseStat(VAL_CONTAINER_OWNER, old_value[8]);
            break;
        case ITEM_NOTE:
            if(old_value[0]) obj->setBaseStat(VAL_NOTE_LANGUAGE, old_value[0]);
            break;
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
            if(old_value[0]) obj->setBaseStat(VAL_DRINKCON_CAPACITY, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_DRINKCON_HOWFULL, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_DRINKCON_LIQUID, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_DRINKCON_POISON, old_value[3]);
            break;
        case ITEM_KEY:
            if(old_value[2]) obj->setBaseStat(VAL_KEY_KEYCODE, old_value[2]);
            break;
        case ITEM_FOOD:
            if(old_value[0]) obj->setBaseStat(VAL_FOOD_FOODVAL, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_FOOD_MAXFOODVAL, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_FOOD_PSBONUS, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_FOOD_POISON, old_value[3]);
            if(old_value[6]) obj->setBaseStat(VAL_FOOD_EXPBONUS, old_value[6]);
            if(old_value[8]) obj->setBaseStat(VAL_FOOD_CANDY_PL, old_value[8]);
            if(old_value[9]) obj->setBaseStat(VAL_FOOD_CANDY_KI, old_value[9]);
            if(old_value[10]) obj->setBaseStat(VAL_FOOD_CANDY_ST, old_value[10]);
            if(old_value[11]) obj->setBaseStat(VAL_FOOD_WHICHATTR, old_value[11]);
            if(old_value[12]) obj->setBaseStat(VAL_FOOD_ATTRCHANCE, old_value[12]);
            break;
        case ITEM_MONEY:
            if(old_value[0]) obj->setBaseStat(VAL_MONEY_SIZE, old_value[0]);
            break;
        case ITEM_VEHICLE:
            if(old_value[0]) obj->setBaseStat(VAL_VEHICLE_DEST, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_VEHICLE_FLAGS, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_VEHICLE_FUEL, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_VEHICLE_FUELCOUNT, old_value[3]);
            break;
        case ITEM_HATCH:
            if(old_value[0]) obj->setBaseStat(VAL_HATCH_DEST, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_HATCH_FLAGS, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_HATCH_DCSKILL, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_HATCH_EXTROOM, old_value[3]);
            if(old_value[6]) obj->setBaseStat(VAL_HATCH_LOCATION, old_value[6]);
            if(old_value[8]) obj->setBaseStat(VAL_HATCH_DCLOCK, old_value[8]);
            if(old_value[9]) obj->setBaseStat(VAL_HATCH_DCHIDE, old_value[9]);
            break;
        case ITEM_WINDOW:
            if(old_value[0]) obj->setBaseStat(VAL_WINDOW_VIEWPORT, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_WINDOW_FLAGS, old_value[1]);
            if(old_value[3]) obj->setBaseStat(VAL_WINDOW_DEFAULT_ROOM, old_value[3]);
            break;
        case ITEM_CONTROL:
            if(old_value[0]) obj->setBaseStat(VAL_CONTROL_VEHICLE_VNUM, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_CONTROL_SPEED, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_CONTROL_FUEL, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_CONTROL_FUELCOUNT, old_value[3]);
            break;
        case ITEM_PORTAL:
            if(old_value[0]) obj->setBaseStat(VAL_PORTAL_DEST, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_PORTAL_FLAGS, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_PORTAL_DCMOVE, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_PORTAL_APPEAR, old_value[3]);
            if(old_value[8]) obj->setBaseStat(VAL_PORTAL_DCLOCK, old_value[8]);
            if(old_value[9]) obj->setBaseStat(VAL_PORTAL_DCHIDE, old_value[9]);
            break;
        case ITEM_BOARD:
            if(old_value[0]) obj->setBaseStat(VAL_BOARD_READ, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_BOARD_WRITE, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_BOARD_ERASE, old_value[2]);
            break;
        case ITEM_BED:
            if(old_value[8]) obj->setBaseStat(VAL_BED_LEVEL, old_value[8]);
            if(old_value[9]) obj->setBaseStat(VAL_BED_HTANK_CHARGE, old_value[9]);
            break;
        case ITEM_PLANT:
            if(old_value[0]) obj->setBaseStat(VAL_PLANT_SOILQUALITY, old_value[0]);
            if(old_value[1]) obj->setBaseStat(VAL_PLANT_MATGOAL, old_value[1]);
            if(old_value[2]) obj->setBaseStat(VAL_PLANT_MATURITY, old_value[2]);
            if(old_value[3]) obj->setBaseStat(VAL_PLANT_MAXMATURE, old_value[3]);
            if(old_value[6]) obj->setBaseStat(VAL_PLANT_WATERLEVEL, old_value[6]);
            break;
        case ITEM_FISHPOLE:
            if(old_value[0]) obj->setBaseStat(VAL_FISHPOLE_BAIT, old_value[0]);
            break;
    }

}
constexpr int NUM_OBJ_VAL_POSITIONS = 16;

/* read all objects from obj file; generate index and prototypes */
static char *parse_object(FILE *obj_f, obj_vnum nr) {
    static char line[READ_SIZE];
    int64_t t[NUM_OBJ_VAL_POSITIONS + 2], j, retval;
    char *tmpptr, buf2[128];
    char f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];
    char f5[READ_SIZE], f6[READ_SIZE], f7[READ_SIZE], f8[READ_SIZE];
    char f9[READ_SIZE], f10[READ_SIZE], f11[READ_SIZE], f12[READ_SIZE];
    struct extra_descr_data *new_descr;

    auto o = std::make_shared<ObjectPrototype>();
    obj_proto[nr] = o;

    o->vn = nr;

    sprintf(buf2, "object #%d", nr);    /* sprintf: OK (for 'buf2 >= 19') */

    /* *** string data *** */
    tmpptr = fread_string(obj_f, buf2);
    if(tmpptr) o->name = std::string(tmpptr);
    tmpptr = fread_string(obj_f, buf2);
    if (tmpptr && *tmpptr)
        if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
            !strcasecmp(fname(tmpptr), "the"))
            *tmpptr = tolower(*tmpptr);
    if(tmpptr) o->short_description = std::string(tmpptr);

    tmpptr = fread_string(obj_f, buf2);
    if (tmpptr && *tmpptr)
        CAP(tmpptr);
    if(tmpptr) o->room_description = std::string(tmpptr);
    tmpptr = fread_string(obj_f, buf2);
    if(tmpptr) o->look_description = std::string(tmpptr);

    /* *** numeric data *** */
    if (!get_line(obj_f, line)) {
        basic_mud_log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
        exit(1);
    }
    if ((retval = sscanf(line, " %ld %s %s %s %s %s %s %s %s %s %s %s %s", t, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10,
                                f11, f12)) == 13) {
        bitvector_t extraFlags[4], wearFlags[4], permFlags[4];

        extraFlags[0] = asciiflag_conv(f1);
        extraFlags[1] = asciiflag_conv(f2);
        extraFlags[2] = asciiflag_conv(f3);
        extraFlags[3] = asciiflag_conv(f4);
        for(auto i = 0; i < 128; i++) if(IS_SET_AR(extraFlags, i)) o->item_flags.set(i);

        wearFlags[0] = asciiflag_conv(f5);
        wearFlags[1] = asciiflag_conv(f6);
        wearFlags[2] = asciiflag_conv(f7);
        wearFlags[3] = asciiflag_conv(f8);
        for(auto i = 0; i < NUM_ITEM_WEARS; i++) if(IS_SET_AR(wearFlags, i)) o->wear_flags.set(i);

        permFlags[0] = asciiflag_conv(f9);
        permFlags[1] = asciiflag_conv(f10);
        permFlags[2] = asciiflag_conv(f11);
        permFlags[3] = asciiflag_conv(f12);
        for(auto i = 0; i < 128; i++) if(IS_SET_AR(permFlags, i)) o->affect_flags.set(i);

    } else {
        basic_mud_log("SYSERR: Format error in first numeric line (expecting 13 args, got %d), %s", retval, buf2);
        exit(1);
    }

    /* Object flags checked in check_object(). */
    o->type_flag = static_cast<ItemType>(t[0]);

    if (!get_line(obj_f, line)) {
        basic_mud_log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
        exit(1);
    }

    for (j = 0; j < NUM_OBJ_VAL_POSITIONS; j++)
        t[j] = 0;

    if ((retval = sscanf(line, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", t, t + 1, t + 2, t + 3, t + 4, t + 5,
                         t + 6, t + 7, t + 8, t + 9, t + 10, t + 11, t + 12, t + 13, t + 14, t + 15)) >
        NUM_OBJ_VAL_POSITIONS) {
        basic_mud_log("SYSERR: Format error in second numeric line (expecting <=%d args, got %d), %s", NUM_OBJ_VAL_POSITIONS,
            retval, buf2);
        exit(1);
    }

    obj_values(o.get(), t);

    if ((GET_OBJ_TYPE(o) == ITEM_PORTAL || \
       GET_OBJ_TYPE(o) == ITEM_HATCH) && \
       (!o->getBaseStat(VAL_DOOR_DCLOCK) || \
        !o->getBaseStat(VAL_DOOR_DCHIDE))) {
        o->setBaseStat(VAL_DOOR_DCLOCK, 20);
        o->setBaseStat(VAL_DOOR_DCHIDE, 20);
        if (bitsavetodisk) {
            converting = true;
        }
    }

    /* Convert old CWG-SunTzu style armor values to CWG-Rasputin. Should no longer be needed I think.
   * if (GET_OBJ_TYPE(obj_proto + i) == ITEM_ARMOR) {
   *   if (suntzu_armor_convert(obj_proto + i)) {
   *     if(bitsavetodisk) {
   *       add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 1);
   *       converting = TRUE;
   *     }
   *   }
   * }*/

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
    o->setBaseStat<weight_t>("weight", t[0]);
    o->setBaseStat("cost", t[1]);
    o->setBaseStat("cost_per_day", t[2]);
    o->setBaseStat("level", t[3]);

    /* check to make sure that weight of containers exceeds curr. quantity */
    if (GET_OBJ_TYPE(o) == ITEM_DRINKCON ||
        GET_OBJ_TYPE(o) == ITEM_FOUNTAIN) {
        if (GET_OBJ_WEIGHT(o) < o->getBaseStat(VAL_CONTAINER_FLAGS))
            o->setBaseStat<weight_t>("weight", o->getBaseStat(VAL_CONTAINER_FLAGS) + 5);
    }
    /* *** make sure portal objects have their timer set correctly *** */
    if (GET_OBJ_TYPE(o) == ITEM_PORTAL) {
        o->setBaseStat("timer", -1);
    }


    strcat(buf2, ", after numeric constants\n"    /* strcat: OK (for 'buf2 >= 87') */
                 "...expecting 'E', 'A', '$', or next object number");
    j = 0;

    for (;;) {
        if (!get_line(obj_f, line)) {
            basic_mud_log("SYSERR: Format error in %s", buf2);
            exit(1);
        }
        switch (*line) {
            case 'E': {
                auto &exd = o->extra_descriptions.emplace_back();
                exd.first = fread_string(obj_f, buf2);
                exd.second = fread_string(obj_f, buf2);
            }
                break;
            case 'A': {
                if (!get_line(obj_f, line)) {
                    basic_mud_log("SYSERR: Format error in 'A' field, %s\n"
                        "...expecting 2 numeric constants but file ended!", buf2);
                    exit(1);
                }

                t[1] = 0;
                if ((retval = sscanf(line, " %ld %ld %ld ", t, t + 1, t + 2)) != 3) {
                    if (retval != 2) {
                        basic_mud_log("SYSERR: Format error in 'A' field, %s\n"
                            "...expecting 2 numeric arguments, got %d\n"
                            "...offending line: '%s'", buf2, retval, line);
                        exit(1);
                    }
                }
                auto &af = o->affected.emplace_back();
                af.location = t[0];
                af.modifier = t[1];
                af.specific = t[2];
            }
                j++;
                break;
            case 'S':  /* Spells for Spellbooks*/
                if (j >= SPELLBOOK_SIZE) {
                    basic_mud_log("SYSERR: Unknown spellbook slot in S field, %s", buf2);
                    exit(1);
                }
                if (!get_line(obj_f, line)) {
                    basic_mud_log("SYSERR: Format error in 'S' field, %s\n"
                        "...expecting 2 numeric constants but file ended!", buf2);
                    exit(1);
                }

                if ((retval = sscanf(line, " %ld %ld ", t, t + 1)) != 2) {
                    basic_mud_log("SYSERR: Format error in 'S' field, %s\n"
                        "...expecting 2 numeric arguments, got %d\n"
                        "...offending line: '%s'", buf2, retval, line);
                    exit(1);
                }

                j++;
                break;
            case 'T':  /* DG triggers */
                dg_obj_trigger(line, o.get());
                break;
            case 'Z':
                if (!get_line(obj_f, line)) {
                    basic_mud_log("SYSERR: Format error in 'Z' field, %s\n"
                        "...expecting numeric constant but file ended!", buf2);
                    exit(1);
                }
                if (sscanf(line, "%ld", t) != 1) {
                    basic_mud_log("SYSERR: Format error in 'Z' field, %s\n"
                        "...expecting numeric argument\n"
                        "...offending line: '%s'", buf2, line);
                    exit(1);
                }
                o->size = static_cast<Size>(t[0]);
                break;
            case '$':
            case '#':
                /* Objects that set CHARM on players are bad. */
                o->affect_flags.set(AFF_CHARM, false);
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
    int cmd_no = 0, num_of_cmds = 0, line_num = 0, tmp = 0, error = 0, arg_num = 0, version = 1;
    char *ptr = nullptr, buf[READ_SIZE] = {0}, zname[READ_SIZE] = {0}, buf2[MAX_STRING_LENGTH] = {0};
    int zone_fix = false;
    char t1[80] = {0}, t2[80] = {0}, line[MAX_STRING_LENGTH] = {0};

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

    if (sscanf(buf, "#%d", &v) != 1) {
        basic_mud_log("SYSERR: FFFFFF Format error in %s, line %d", zname, line_num);
        exit(1);
    }
    snprintf(buf2, sizeof(buf2)-1, "beginning of zone #%d", v);

    auto z = std::make_shared<Zone>();
    zone_table[v] = z;
    z->number = v;

    line_num += get_line(fl, buf);
    if ((ptr = strchr(buf, '~')) != nullptr)    /* take off the '~' if it's there */
        *ptr = '\0';
    z->builders = buf;

    line_num += get_line(fl, buf);
    if ((ptr = strchr(buf, '~')) != nullptr)    /* take off the '~' if it's there */
        *ptr = '\0';
    z->name = buf;

    vnum bot, top;

    line_num += get_line(fl, buf);
    bitvector_t zone_flags[4];
    if (version >= 2) {

        char zbuf1[MAX_STRING_LENGTH];
        char zbuf2[MAX_STRING_LENGTH];
        char zbuf3[MAX_STRING_LENGTH];
        char zbuf4[MAX_STRING_LENGTH];
        int min_level, max_level;
        if (sscanf(buf, " %d %d %d %d %s %s %s %s %d %d", &bot, &top, &z->lifespan,
                   &z->reset_mode, zbuf1, zbuf2, zbuf3, zbuf4, &min_level, &max_level) != 10) {
            basic_mud_log("SYSERR: Format error in 10-constant line of %s", zname);
            exit(1);
        }

        zone_flags[0] = asciiflag_conv(zbuf1);
        zone_flags[1] = asciiflag_conv(zbuf2);
        zone_flags[2] = asciiflag_conv(zbuf3);
        zone_flags[3] = asciiflag_conv(zbuf4);

    } else if (sscanf(buf, " %d %d %d %d ", &bot, &top, &z->lifespan, &z->reset_mode) != 4) {
        /*
     * This may be due to the fact that the zone has no builder.  So, we just attempt
     * to fix this by copying the previous 2 last reads into this variable and the
     * last one.
     */
        basic_mud_log("SYSERR: Format error in numeric constant line of %s, attempting to fix.", zname);
        char* zname = nullptr;
        if (sscanf(zname, " %d %d %d %d ", &bot, &top, &z->lifespan, &z->reset_mode) != 4) {
            basic_mud_log("SYSERR: Could not fix previous error, aborting game.");
            exit(1);
        } else {
            //if(zname) z.name = zname;
            z->builders = "None.";
            zone_fix = true;
        }
    }

    auto &zr = zone_ranges[v] = {bot, top};

    for(auto i = 0; i < 128; i++) if(IS_SET_AR(zone_flags, i)) z->zone_flags.set(i);

    auto &res = oldResetCommands[v];

    for (auto c = 0;true;c++) {
        get_line(fl, buf);
        if(buf[0] == '*') continue;

        if(strchr("$S", buf[0])) {
            break;
        }

        auto &zc = res.emplace_back();
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

struct cmdlist_element {
    char *cmd{};                /* one line of a trigger */
    struct cmdlist_element *original{};
    struct cmdlist_element *next{};
};

/* local functions */
static void parse_trigger(FILE *trig_f, trig_vnum nr) {
    int t[2], k, attach_type;
    char line[256], *cmds, *s, flags[256], errors[MAX_INPUT_LENGTH];
    struct cmdlist_element *cle;

    auto trig = std::make_shared<DgScriptPrototype>();
    trig_index[nr] = trig;

    trig->vn = nr;

    snprintf(errors, sizeof(errors), "trig vnum %d", nr);

    char *buf = fread_string(trig_f, errors);

    trig->name = buf;

    get_line(trig_f, line);
    k = sscanf(line, "%d %s %d", &attach_type, flags, t);
    trig->attach_type = static_cast<UnitType>(attach_type);
    trig->trigger_type = (long) asciiflag_conv(flags);
    trig->narg = (k == 3) ? t[0] : 0;

    buf = fread_string(trig_f, errors);

    trig->arglist = buf ? buf : "";

    free(buf);

    cmds = s = fread_string(trig_f, errors);

    std::vector<std::string> lines;
    boost::split_regex(lines, cmds, boost::regex("\r\n|\r|\n"));

    trig->lines = parse_script(lines);

    free(cmds);
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


static void load_affects(FILE *fl, struct Character *ch, int violence) {
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
            af.aff_flags.set(static_cast<AffectFlag>(num5));
            af.specific = num6;
            if (violence)
                affectv_to_char(ch, &af);
            else
                affect_to_char(ch, &af);
            i++;
        }
    } while (num != 0);
}


static void load_skills(FILE *fl, struct Character *ch, bool mods) {
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

static void load_bonuses(FILE *fl, struct Character *ch, bool mods) {
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
            //GET_BONUS(ch, i) = num[i];
        }
    }
}

#define LOAD_HIT    0
#define LOAD_MANA    1
#define LOAD_MOVE    2
#define LOAD_KI        3
#define LOAD_LIFE       4

static void load_HMVS(struct Character *ch, const char *line, int mode) {
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

static void load_BASE(struct Character *ch, const char *line, int mode) {
    int64_t num = 0;

    sscanf(line, "%" I64T "", &num);

    switch (mode) {
        case LOAD_HIT:
            ch->setBaseStat("health", num);
            break;

        case LOAD_MANA:
            ch->setBaseStat("ki", num);
            break;

        case LOAD_MOVE:
            ch->setBaseStat("stamina", num);
            break;

        case LOAD_LIFE:
            //GET_LIFEFORCE(ch) = num;
            break;
    }
}

static void load_majin(struct Character *ch, const char *line) {
    int64_t num = 0;

    sscanf(line, "%" I64T "", &num);
    ch->setBaseStat("majinizer", num);

}

static void load_molt(struct Character *ch, const char *line) {
    int64_t num = 0;

    sscanf(line, "%" I64T "", &num);
    ch->setBaseStat<int64_t>("molt_experience", num);

}

/* new load_char reads ASCII Player Files */
/* Load a char, TRUE if loaded, FALSE if not */
static int load_char(const char *name, struct Character *ch) {
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

        ch->setBaseStat("weight", PFDEF_WEIGHT);

        ch->setBaseStat("bless_level", PFDEF_HEIGHT);
        ch->setBaseStat("life_percent", PFDEF_WEIGHT);

        ch->setBaseStat<int>("olc_zone", PFDEF_OLC);

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
                        for(auto f = 0; f < 128; f++) {
                            if(IS_SET_AR(flags, f)) ch->player_flags.set(f);
                        }
                    } else if (!strcmp(tag, "Aff ")) {
                        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
                        flags[0] = asciiflag_conv(f1);
                        flags[1] = asciiflag_conv(f2);
                        flags[2] = asciiflag_conv(f3);
                        flags[3] = asciiflag_conv(f4);
                        for(auto f = 0; f < 128; f++) {
                            if(IS_SET_AR(flags, f)) ch->affect_flags.set(f);
                        }
                    } else if (!strcmp(tag, "Affs")) load_affects(fl, ch, 0);
                    else if (!strcmp(tag, "Affv")) load_affects(fl, ch, 1);
                    else if (!strcmp(tag, "AdmL")) ch->setBaseStat("admin_level", atoi(line));
                    else if (!strcmp(tag, "Abso")) ch->setBaseStat<int>("absorbs", atoi(line));
                    else if (!strcmp(tag, "AdmF")) {
                        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
                        flags[0] = asciiflag_conv(f1);
                        flags[1] = asciiflag_conv(f2);
                        flags[2] = asciiflag_conv(f3);
                        flags[3] = asciiflag_conv(f4);
                        for(auto f = 0; f < 128; f++) {
                            if(IS_SET_AR(flags, f)) ch->admin_flags.set(f);
                        }
                    } else if (!strcmp(tag, "Alin")) ch->setBaseStat("good_evil", atoi(line));
                    //else if (!strcmp(tag, "Aura")) ch->set(CharAppearance::aura, atoi(line));
                    break;

                case 'B':
                    if (!strcmp(tag, "Bank")) ch->setBaseStat("money_bank", atoi(line));
                    else if (!strcmp(tag, "Bki ")) load_BASE(ch, line, LOAD_MANA);
                    else if (!strcmp(tag, "Blss")) ch->setBaseStat("bless_level", atoi(line));
                    else if (!strcmp(tag, "Boam")) GET_BOARD(ch, 0) = atoi(line);
                    else if (!strcmp(tag, "Boai")) GET_BOARD(ch, 1) = atoi(line);
                    else if (!strcmp(tag, "Boac")) GET_BOARD(ch, 2) = atoi(line);
                    else if (!strcmp(tag, "Boad")) GET_BOARD(ch, 3) = atoi(line);
                    else if (!strcmp(tag, "Boab")) GET_BOARD(ch, 4) = atoi(line);
                    else if (!strcmp(tag, "Bonu")) load_bonuses(fl, ch, false);
                    else if (!strcmp(tag, "Boos")) ch->setBaseStat<int>("boosts", atoi(line));
                    else if (!strcmp(tag, "Bpl ")) load_BASE(ch, line, LOAD_HIT);
                    else if (!strcmp(tag, "Brth")) ch->time.birth = atol(line);
                    else if (!strcmp(tag, "Bst ")) load_BASE(ch, line, LOAD_MOVE);
                    break;

                case 'C':
                    if (!strcmp(tag, "Cha ")) ch->setBaseStat("speed", atoi(line));
                    else if (!strcmp(tag, "Clan")) ;
                    else if (!strcmp(tag, "Clar")) ;
                    else if (!strcmp(tag, "Clas")) {
                        auto sen = atoi(line)+1;
                        if(sen > 14) sen = 0;
                        ch->sensei = static_cast<Sensei>(sen);
                    }
                    else if (!strcmp(tag, "Colr")) {
                        sscanf(line, "%d %s", &num, buf2);
                    } else if (!strcmp(tag, "Con ")) ch->setBaseStat("constitution", atoi(line));
                    else if (!strcmp(tag, "Cool")) ch->setBaseStat("concentrate_cooldown", atoi(line));
                    else if (!strcmp(tag, "Crtd")) ch->time.created = atol(line);
                    break;

                case 'D':
                    if (!strcmp(tag, "Deat")) ch->setBaseStat("death_time", atoi(line));
                    else if (!strcmp(tag, "Deac")) ch->setBaseStat("death_count", atoi(line));
                    else if (!strcmp(tag, "Desc")) ch->look_description = fread_string(fl, buf2);
                    else if (!strcmp(tag, "Dex ")) ch->setBaseStat("agility", atoi(line));
                    else if (!strcmp(tag, "Drnk")) GET_COND(ch, DRUNK) = atoi(line);
                    else if (!strcmp(tag, "Damg")) ch->setBaseStat("damage_mod", atoi(line));
                    else if (!strcmp(tag, "Droo")) ;
                    break;

                case 'E':
                    if (!strcmp(tag, "Exp ")) ch->setExperience(atoi(line));
                    else if (!strcmp(tag, "Eali")) ch->setBaseStat("law_chaos", atoi(line));
                    else if (!strcmp(tag, "Ecls")) {

                    } //else if (!strcmp(tag, "Eye ")) ch->set(CharAppearance::eye_color, atoi(line));
                    break;

                case 'F':
                    if (!strcmp(tag, "Fisd")) ch->setBaseStat("fish_distance", atoi(line));
                    else if (!strcmp(tag, "Frez")) ch->setBaseStat("freeze_level", atoi(line));
                    else if (!strcmp(tag, "Forc")) ch->setBaseStat("forget_count", atoi(line));
                    else if (!strcmp(tag, "Forg")) ch->setBaseStat<int>("forgetting_skill", atoi(line));
                    else if (!strcmp(tag, "Fury")) ch->setBaseStat("fury", atoi(line));
                    break;

                case 'G':
                    if (!strcmp(tag, "Gold")) ch->setBaseStat("money_carried", atoi(line));
                    else if (!strcmp(tag, "Gaun")) ch->setBaseStat("gauntlet", atoi(line));
                    //else if (!strcmp(tag, "Geno")) ch->genome.insert(atoi(line));
                    //else if (!strcmp(tag, "Gen1")) ch->genome.insert(atoi(line));
                    break;

                case 'H':
                    if (!strcmp(tag, "Hit ")) load_HMVS(ch, line, LOAD_HIT);
                    else if (!strcmp(tag, "Hite")) ch->setBaseStat("height", atoi(line));
                    else if (!strcmp(tag, "Home")) ;
                    else if (!strcmp(tag, "Host")) {
                    }
                    //else if (!strcmp(tag, "Hrc ")) ch->set(CharAppearance::hair_color, atoi(line));
                    //else if (!strcmp(tag, "Hrl ")) ch->set(CharAppearance::hair_length, atoi(line));
                    //else if (!strcmp(tag, "Hrs ")) ch->set(CharAppearance::hair_style, atoi(line));
                    else if (!strcmp(tag, "Hung")) GET_COND(ch, HUNGER) = atoi(line);
                    break;

                case 'I':
                    if (!strcmp(tag, "Id  ")) GET_IDNUM(ch) = atol(line);
                    else if (!strcmp(tag, "INGl")) ch->setBaseStat("ingest_learned", atoi(line));
                    else if (!strcmp(tag, "Int ")) ch->setBaseStat("intelligence", atoi(line));
                    else if (!strcmp(tag, "Invs")) ch->setBaseStat("invis_level", atoi(line));
                    break;

                case 'K':
                    if (!strcmp(tag, "Ki  ")) load_HMVS(ch, line, LOAD_KI);
                    else if (!strcmp(tag, "Kaio")) ch->setBaseStat("kaioken", atoi(line));
                    break;

                case 'L':
                    if (!strcmp(tag, "Last")) ch->time.logon = atol(line);
                    else if (!strcmp(tag, "Lern")) ch->modPractices(atoi(line));
                    else if (!strcmp(tag, "Levl")) ch->setBaseStat("level", atoi(line));
                    else if (!strcmp(tag, "LF  ")) load_BASE(ch, line, LOAD_LIFE);
                    else if (!strcmp(tag, "LFPC")) ch->setBaseStat("life_percent", atoi(line));
                    else if (!strcmp(tag, "Lila")) GET_LIMBCOND(ch, 1) = atoi(line);
                    else if (!strcmp(tag, "Lill")) GET_LIMBCOND(ch, 3) = atoi(line);
                    else if (!strcmp(tag, "Lira")) GET_LIMBCOND(ch, 0) = atoi(line);
                    else if (!strcmp(tag, "Lirl")) GET_LIMBCOND(ch, 2) = atoi(line);
                    else if (!strcmp(tag, "Lint")) ch->setBaseStat("last_interest", atoi(line));
                    else if (!strcmp(tag, "Lpla")) ch->setBaseStat("last_played", atoi(line));

                    break;

                case 'M':
                    if (!strcmp(tag, "Mana")) load_HMVS(ch, line, LOAD_MANA);
                    else if (!strcmp(tag, "Mexp")) load_molt(ch, line);
                    else if (!strcmp(tag, "Mlvl")) ch->setBaseStat<int>("molt_level", atoi(line));
                    else if (!strcmp(tag, "Move")) load_HMVS(ch, line, LOAD_MOVE);
                    else if (!strcmp(tag, "Mcls")) {

                    } else if (!strcmp(tag, "Maji")) ch->setBaseStat("majinizer", atoi(line));
                    else if (!strcmp(tag, "Majm")) load_majin(ch, line);
                    else if (!strcmp(tag, "Mimi"))
                        ch->mimic = (Race)atoi(line);
                    else if (!strcmp(tag, "MxAg")) ch->time.maxage = atol(line);
                    break;

                case 'N':
                    if (!strcmp(tag, "Name")) ch->name = line;
                    break;

                case 'O':
                    if (!strcmp(tag, "Olc ")) ch->setBaseStat<int>("olc_zone", atoi(line));
                    break;

                case 'P':
                    if (!strcmp(tag, "Phas")) ;//ch->set(CharAppearance::distinguishing_feature, atoi(line));
                    else if (!strcmp(tag, "Phse")) ch->setBaseStat<int>("starphase", atoi(line));
                    else if (!strcmp(tag, "Plyd")) ch->time.played = atol(line);
#ifdef ASCII_SAVE_POOFS
                    else if (!strcmp(tag, "PfIn")) POOFIN(ch) = strdup(line);
                    else if (!strcmp(tag, "PfOt")) POOFOUT(ch) = strdup(line);
#endif
                    else if (!strcmp(tag, "Pole")) ch->setBaseStat("pole_bonus", atoi(line));
                    else if (!strcmp(tag, "Posi")) ch->position = static_cast<Position>(atoi(line));
                    else if (!strcmp(tag, "Pref")) {
                        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
                        flags[0] = asciiflag_conv(f1);
                        flags[1] = asciiflag_conv(f2);
                        flags[2] = asciiflag_conv(f3);
                        flags[3] = asciiflag_conv(f4);
                        for(auto f = 0; f < 128; f++) {
                            if(IS_SET_AR(flags, f)) ch->pref_flags.set(f, true);
                        }
                    } else if (!strcmp(tag, "Prff")) ch->setBaseStat("preference", atoi(line));
                    break;

                case 'R':
                    if (!strcmp(tag, "Race")) {
                        auto race = atoi(line)+1;
                        if(race > 23) race = 0;
                        ch->race = static_cast<Race>(race);
                    }
                    //else if (!strcmp(tag, "Raci")) ch->set(CharNum::racial_preference, atoi(line));
                    else if (!strcmp(tag, "rDis")) {
                        if(!boost::iequals(line, "Empty"))
                            GET_RDISPLAY(ch) = strdup(line);
                    }
                    else if (!strcmp(tag, "Rela")) ch->setBaseStat("relax_count", atoi(line));
                    else if (!strcmp(tag, "Rtim")) ch->setBaseStat("rewtime", atoi(line));
                    else if (!strcmp(tag, "Rad1")) ch->setBaseStat("radar1", atoi(line));
                    else if (!strcmp(tag, "Rad2")) ch->setBaseStat("radar2", atoi(line));
                    else if (!strcmp(tag, "Rad3")) ch->setBaseStat("radar3", atoi(line));
                    else if (!strcmp(tag, "Room")) ;
                    else if (!strcmp(tag, "RPfe")) GET_FEATURE(ch) = strdup(line);
                    break;

                case 'S':
                    if (!strcmp(tag, "Sex ")) ch->sex = static_cast<Sex>(atoi(line));
                    else if (!strcmp(tag, "Ship")) ;
                    else if (!strcmp(tag, "Scoo")) ch->setBaseStat("selfdestruct_cooldown", atoi(line));
                    else if (!strcmp(tag, "Shpr")) ;
                    else if (!strcmp(tag, "Skil")) load_skills(fl, ch, false);
                    //else if (!strcmp(tag, "Skn ")) ch->set(CharAppearance::skin_color, atoi(line));
                    else if (!strcmp(tag, "Size")) ch->setSize(atoi(line));
                    else if (!strcmp(tag, "SklB")) load_skills(fl, ch, true);
                    else if (!strcmp(tag, "SkRc")) ch->modPractices(atoi(line));
                    else if (!strcmp(tag, "SkCl")) {
                        sscanf(line, "%d %d", &num2, &num3);
                        ch->modPractices(num3);
                    } else if (!strcmp(tag, "Slot")) ch->setBaseStat<int>("skill_slots", atoi(line));
                    else if (!strcmp(tag, "Spek")) ;
                    else if (!strcmp(tag, "Str ")) ch->setBaseStat("strength", atoi(line));
                    else if (!strcmp(tag, "Stuk")) ;
                    else if (!strcmp(tag, "Supp")) ch->setBaseStat("suppression", atoi(line));
                    break;

                case 'T':
                    if (!strcmp(tag, "Tgro")) ch->setBaseStat<int>("tail_growth", atoi(line));
                    else if (!strcmp(tag, "Tcla")) {
                        switch(atoi(line)) {
                            case 1: // great requirements... range is 0.2 to 0.3.
                                ch->setBaseStat("transBonus", Random::get<double>(0.2, 0.3));
                            break;
                            case 2:
                                ch->setBaseStat("transBonus", Random::get<double>(-0.1, 0.1));
                            break;
                            case 3:
                                ch->setBaseStat("transBonus", Random::get<double>(-0.3, -0.2));
                            break;
                            default:
                                ch->setBaseStat("transBonus", Random::get<double>(-0.3, 0.3));
                        }

                    }
                    else if (!strcmp(tag, "Tcos")) {
                        sscanf(line, "%d %d", &num2, &num3);
                        //GET_TRANSCOST(ch, num2) = num3;
                    } else if (!strcmp(tag, "Thir")) GET_COND(ch, THIRST) = atoi(line);
                    else if (!strcmp(tag, "Thr1")) ;
                    else if (!strcmp(tag, "Thr2")) ;
                    else if (!strcmp(tag, "Thr3")) ;
                    else if (!strcmp(tag, "Thr4") || !strcmp(tag, "Thr5")); /* Discard extra saves */
                    else if (!strcmp(tag, "ThB1")) ;
                    else if (!strcmp(tag, "ThB2")) ;
                    else if (!strcmp(tag, "ThB3")) ;
                    else if (!strcmp(tag, "Trag")) ch->setBaseStat("train_agility",  atoi(line));
                    else if (!strcmp(tag, "Trco")) ch->setBaseStat("train_constitution",  atoi(line));
                    else if (!strcmp(tag, "Trin")) ch->setBaseStat("train_intelligence",  atoi(line));
                    else if (!strcmp(tag, "Trsp")) ch->setBaseStat("train_speed",  atoi(line));
                    else if (!strcmp(tag, "Trst")) ch->setBaseStat("train_strength",  atoi(line));
                    else if (!strcmp(tag, "Trwi")) ch->setBaseStat("train_wisdom",  atoi(line));
                    break;
                case 'U':
                    if (!strcmp(tag, "Upgr")) ch->setBaseStat<int>("upgrade_points", atoi(line));
                    else if (!strcmp(tag, "User")) ;
                    break;
                case 'V':
                    if (!strcmp(tag, "Voic")) GET_VOICE(ch) = strdup(line);
                    break;

                case 'W':
                    if (!strcmp(tag, "Wate")) ch->setBaseStat("weight", atoi(line));
                    else if (!strcmp(tag, "Wimp")) ch->setBaseStat("wimp_level", atoi(line));
                    else if (!strcmp(tag, "Wis ")) ch->setBaseStat("wisdom", atoi(line));
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

static void handle_ships(Object *object, Room *room) {
    if (GET_OBJ_TYPE(object) == ITEM_HATCH && GET_OBJ_VNUM(object) <= 19199) {
        if ((GET_OBJ_VNUM(object) <= 18999 && GET_OBJ_VNUM(object) >= 18800) ||
            (GET_OBJ_VNUM(object) <= 19199 && GET_OBJ_VNUM(object) >= 19100)) {
            int hnum = GET_OBJ_VAL(object, VAL_HATCH_DEST);
            Object *house = read_object(hnum, VIRTUAL);
            house->moveToLocation(GET_OBJ_VAL(object, VAL_HATCH_LOCATION));
            int newval = GET_OBJ_VAL(object, VAL_CONTAINER_FLAGS) | CONT_CLOSED | CONT_LOCKED;
            SET_OBJ_VAL(object, VAL_CONTAINER_FLAGS, newval);
        }
    }

    if (GET_OBJ_TYPE(object) == ITEM_HATCH && GET_OBJ_VAL(object, VAL_HATCH_DEST) > 1 && GET_OBJ_VNUM(object) > 19199) {
        Object *vehicle = nullptr;
        if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(object, VAL_HATCH_DEST)))) {
            if (real_room(GET_OBJ_VAL(object, VAL_HATCH_EXTROOM)) != NOWHERE) {
                vehicle = read_object(GET_OBJ_VAL(object, VAL_HATCH_DEST), VIRTUAL);
                if(!vehicle) {
                    basic_mud_log("SYSERR: Vehicle %d not found for hatch %d", GET_OBJ_VAL(object, VAL_HATCH_DEST), GET_OBJ_VNUM(object));
                } else {
                    vehicle->moveToLocation(GET_OBJ_VAL(object, VAL_HATCH_EXTROOM));
                    if (auto ld = object->getLookDescription(); ld) {
                        if (strlen(ld)) {
                            char nick[MAX_INPUT_LENGTH], nick2[MAX_INPUT_LENGTH], nick3[MAX_INPUT_LENGTH];
                            if (GET_OBJ_VNUM(vehicle) <= 46099 && GET_OBJ_VNUM(vehicle) >= 46000) {
                                snprintf(nick, sizeof(nick), "Saiyan Pod %s", ld);
                                snprintf(nick2, sizeof(nick2), "@wA @Ys@ya@Yi@yy@Ya@yn @Dp@Wo@Dd@w named @D(@C%s@D)@w",
                                        ld);
                            } else if (GET_OBJ_VNUM(vehicle) >= 46100 && GET_OBJ_VNUM(vehicle) <= 46199) {
                                snprintf(nick, sizeof(nick), "EDI Xenofighter MK. II %s", ld);
                                snprintf(nick2, sizeof(nick2), 
                                        "@wAn @YE@yD@YI @CX@ce@Wn@Do@Cf@ci@Wg@Dh@Wt@ce@Cr @RMK. II @wnamed @D(@C%s@D)@w",
                                        ld);
                            }
                            snprintf(nick3, sizeof(nick3), "%s is resting here@w", nick2);
                            vehicle->name = nick;
                            vehicle->short_description = nick2;
                            vehicle->room_description = nick3;
                        }
                    }
                }
                int newval = GET_OBJ_VAL(object, VAL_CONTAINER_FLAGS) | CONT_CLOSED | CONT_LOCKED;
                SET_OBJ_VAL(object, VAL_CONTAINER_FLAGS, newval);
            } else {
                basic_mud_log("Hatch load: Hatch with no vehicle load room: #%d!", GET_OBJ_VNUM(object));
            }
        }
    }
}

int House_load(room_vnum rvnum) {
    FILE *fl;
    char f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];
    char cmfname[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char line[256];
    int64_t t[21], danger, zwei = 0;
    struct Object *temp;
    int locate = 0, j, nr, k, num_objs = 0;
    struct Object *obj1;
    struct Object *cont_row[MAX_BAG_ROWS];
    struct extra_descr_data *new_descr;
    room_rnum rrnum = rvnum;

    if (!Room::registry.contains(rvnum))
        return 0;

    if (!House_get_filename(rvnum, cmfname, sizeof(cmfname)))
        return 0;

    if (!(fl = fopen(cmfname, "r+b"))) {
        if (errno != ENOENT) {  /* if it fails, NOT because of no file */
            snprintf(buf1, sizeof(buf1), "SYSERR: READING HOUSE FILE %s (5)", cmfname);
            perror(buf1);
        }
        return 0;
    }

    for (j = 0; j < MAX_BAG_ROWS; j++)
        cont_row[j] = nullptr; /* empty all cont lists (you never know ...) */

    if (!feof(fl))
        get_line(fl, line);
    while (!feof(fl)) {
        temp = nullptr;
        /* first, we get the number. Not too hard. */
        if (*line == '#') {
            if (sscanf(line, "#%d", &nr) != 1) {
                continue;
            }
            /* we have the number, check it, load obj. */
            if (nr == NOTHING) {   /* then it is unique */
                temp = create_obj();
            } else if (nr < 0) {
                continue;
            } else {
                if (nr >= 999999)
                    continue;
                temp = read_object(nr, VIRTUAL);
                if (!temp) {
                    get_line(fl, line);
                    continue;
                }
            }

            get_line(fl, line);
            sscanf(line, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %s %s %s %s %ld %ld %ld %ld %ld %ld %ld %ld", t, t + 1, t + 2, t + 3,
                   t + 4, t + 5, t + 6, t + 7, t + 8, f1, f2, f3, f4, t + 13, t + 14, t + 15, t + 16, t + 17, t + 18,
                   t + 19, t + 20);
            locate = t[0];
            int64_t old_values[NUM_OBJ_VAL_POSITIONS];
            for(auto n = 0; n < 7; n++) old_values[n] = t[n + 1];
            for(auto n = 8; n < 15; n++) old_values[n] = t[n + 5];
            obj_values(temp, old_values);

            bitvector_t ex[4];
            ex[0] = asciiflag_conv(f1);
            ex[1] = asciiflag_conv(f2);
            ex[2] = asciiflag_conv(f3);
            ex[3] = asciiflag_conv(f4);
            for(auto i = 0; i < 128; i++) temp->item_flags.set(i, IS_SET_AR(ex, i));

            get_line(fl, line);
            char *txt;
            /* read line check for xap. */
            if (!strcmp("XAP", line)) {  /* then this is a Xap Obj, requires
                                       special care */
                if ((txt = fread_string(fl, buf2))) {
                    temp->name = txt;
                    free(txt);
                } else {
                    temp->name = "undefined";
                }

                if ((txt = fread_string(fl, buf2))) {
                    temp->short_description = txt;
                    free(txt);
                } else {
                    temp->short_description = "undefined";
                }

                if ((txt = fread_string(fl, buf2))) {
                    temp->room_description = txt;
                    free(txt);
                } else {
                    temp->room_description = "undefined";
                }

                if ((txt = fread_string(fl, buf2))) {
                    temp->look_description = txt;
                    free(txt);
                } else {
                    temp->look_description = "undefined";
                }

                if (!get_line(fl, line) ||
                    (sscanf(line, "%ld %ld %ld %ld %ld %ld %ld %ld", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7) !=
                     8)) {
                    fprintf(stderr, "Format error in first numeric line (expecting _x_ args)");
                    return 0;
                }
                temp->type_flag = static_cast<ItemType>(t[0]);
                bitvector_t wear[4];
                wear[0] = t[1];
                wear[1] = t[2];
                wear[2] = t[3];
                wear[3] = t[4];
                for(auto i = 0; i < NUM_ITEM_WEARS; i++) temp->wear_flags.set(i, IS_SET_AR(wear, i));
                temp->setBaseStat("weight", t[5]);
                temp->setBaseStat("cost", t[6]);
                temp->setBaseStat("cost_per_day", t[7]);

                /* buf2 is error codes pretty much */
                //strcat(buf2, ", after numeric constants (expecting E/#xxx)");

                get_line(fl, line);
                int64_t fakeid;
                for (k = j = zwei = 0; !zwei && !feof(fl);) {
                    switch (*line) {
                        case 'E':
                            fread_string(fl, buf2);
                            fread_string(fl, buf2);
                            get_line(fl, line);
                            break;
                        case 'A':
                            if (j >= MAX_OBJ_AFFECT) {
                                basic_mud_log("SYSERR: Too many object affectations in loading house file");
                                danger = 1;
                            }
                            get_line(fl, line);
                            sscanf(line, "%ld %ld %ld", t, t + 1, t + 2);
                            temp->affected[j].location = t[0];
                            temp->affected[j].modifier = t[1];
                            temp->affected[j].specific = t[2];
                            j++;
                            get_line(fl, line);
                            break;

                        case 'G':
                            get_line(fl, line);
                            //sscanf(line, "%" TMT, &temp->generation);
                            get_line(fl, line);
                            break;
                        case 'U':
                            get_line(fl, line);
                            sscanf(line, "%" I64T, &fakeid);
                            get_line(fl, line);
                            break;
                        case 'S':
                            if (j >= SPELLBOOK_SIZE) {
                                basic_mud_log("SYSERR: Too many spells in spellbook loading rent file");
                                danger = 1;
                            }
                            get_line(fl, line);
                            sscanf(line, "%ld %ld", t, t + 1);

                            j++;
                            get_line(fl, line);
                            break;
                        case 'Z': {
                            get_line(fl, line);
                            int size;
                            sscanf(line, "%d", &size);
                            temp->size = static_cast<Size>(size);
                            get_line(fl, line);
                        }
                            break;
                        case '$':
                        case '#':
                            zwei = 1;
                            break;
                        default:
                            zwei = 1;
                            break;
                    }
                }      /* exit our for loop */
            }   /* exit our xap loop */
            if (temp != nullptr) {
                num_objs++;
                auto r = get_room(rrnum);
                temp->moveToLocation(r);
                handle_ships(temp, r);
            }

        } else {
            get_line(fl, line);
        }
    }


    fclose(fl);

    return 1;
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

/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House_boot() {
    int num_of_houses = 0;
    struct house_control_rec temp_house;
    FILE *fl;

    if (!(fl = fopen(HCONTROL_FILE, "rb"))) {
        if (errno == ENOENT)
            basic_mud_log("   No houses to load. File '%s' does not exist.", HCONTROL_FILE);
        else
            perror("SYSERR: " HCONTROL_FILE);
        return;
    }
    while (!feof(fl) && num_of_houses < MAX_HOUSES) {
        fread(&temp_house, sizeof(struct house_control_rec), 1, fl);

        if (feof(fl))
            break;

        if (Room::registry.contains(temp_house.vn))
            House_load(temp_house.vn);
    }

    fclose(fl);
}

static void link_exits() {
    for (const auto& [room, dir, target] : room_directions) {
        if(auto r = get_room(target))
            room->exits.at(dir) = r;
    }
}

using land_spots = std::vector<std::pair<std::string, room_vnum>>;


static const land_spots land_earth = {
    {"Nexus City", 300},
    {"South Ocean", 800},
    {"Nexus Field", 1150},
    {"Cherry Blossom Mountain", 1180},
    {"Sandy Desert", 1287},
    {"Northern Plains", 1428},
    {"Korin's Tower", 1456},
    {"Kami's Lookout", 1506},
    {"Shadow Forest", 1636},
    {"Decrepit Area", 1710},
    {"West City", 19510},
    {"Hercule Beach", 2141},
    {"Satan City", 13020},
};

static const land_spots land_frigid = {
    {"Ice Crown City", 4264},
    {"Ice Highway", 4300},
    {"Topica Snowfield", 4351},
    {"Glug's Volcano", 4400},
    {"Platonic Sea", 4600},
    {"Slave City", 4800},
    {"Acturian Woods", 5100},
    {"Desolate Demesne", 5150},
    {"Chateau Ishran", 5165},
    {"Wyrm Spine Mountain", 5200},
    {"Cloud Ruler Temple", 5500},
    {"Koltoan Mine", 4944},
};

static const land_spots land_konack = {
    {"Tiranoc City", 8006},
    {"Great Oroist Temple", 8300},
    {"Elzthuan Forest", 8400},
    {"Mazori Farm", 8447},
    {"Dres", 8500},
    {"Colvian Farm", 8600},
    {"St Alucia", 8700},
    {"Meridius Memorial", 8800},
    {"Desert of Illusion", 8900},
    {"Plains of Confusion", 8954},
    {"Turlon Fair", 9200},
    {"Wetlands", 9700},
    {"Kerberos", 9855},
    {"Shaeras Mansion", 9864},
    {"Slavinus Ravine", 9900},
    {"Furian Citadel", 9949},
};

static const land_spots land_vegeta = {
    {"Vegetos City", 2226},
    {"Blood Dunes", 2600},
    {"Ancestral Mountains", 2616},
    {"Destopa Swamp", 2709},
    {"Pride Forest", 2800},
    {"Pride Tower", 2899},
    {"Ruby Cave", 2615},
};

static const land_spots land_namek = {
    {"Senzu Village", 11600},
    {"Guru's House", 10182},
    {"Crystalline Cave", 10474},
    {"Elder Village", 13300},
    {"Frieza's Ship", 10203},
    {"Kakureta Village", 10922},
};

static const land_spots land_aether = {
    {"Haven City", 12010},
    {"Serenity Lake", 12103},
    {"Kaiju Forest", 12300},
    {"Ortusian Temple", 12400},
    {"Silent Glade", 12480},
};

static const land_spots land_yardrat = {
    {"Yardra City", 14008},
    {"Jade Forest", 14100},
    {"Jade Cliffs", 14200},
    {"Mount Valaria", 14300},
};

static const land_spots land_kanassa = {
    {"Aquis City", 14904},
    {"Yunkai Pirate Base", 15655},
};

static const land_spots land_cerria = {
    {"Cerria Colony", 17531},
    {"Crystalline Forest", 7950},
    {"Fistarl Volcano", 17420},
};

static const land_spots land_arlia = {
    {"Janacre", 16009},
    {"Arlian Wasteland", 16544},
    {"Arlia Mine", 16600},
};

static const land_spots land_zenith = {
    {"Utatlan City", 3412},
    {"Zenith Jungle", 3520},
    {"Ancient Castle", 19600},
};

static const land_spots dock_earth = {
    {"1", 409},
    {"2", 411},
    {"3", 412},
    {"4", 410},
    {"Nexus City", 300},
    {"South Ocean", 800},
    {"Nexus Field", 1150},
    {"Cherry Blossom Mountain", 1180},
    {"Sandy Desert", 1287},
    {"Northern Plains", 1428},
    {"Korin's Tower", 1456},
    {"Kami's Lookout", 1506},
    {"Shadow Forest", 1600},
    {"Decrepit Area", 1710},
    {"West City", 19510},
    {"Hercule Beach", 2141},
    {"Satan City", 13020},
};

static const land_spots dock_frigid = {
    {"1", 4264},
    {"2", 4263},
    {"3", 4261},
    {"4", 4262},
    {"Ice Crown City", 4264},
    {"Ice Highway", 4300},
    {"Topica Snowfield", 4351},
    {"Glug's Volcano", 4400},
    {"Platonic Sea", 4600},
    {"Slave City", 4800},
    {"Acturian Woods", 5100},
    {"Desolate Demesne", 5150},
    {"Chateau Ishran", 5165},
    {"Wyrm Spine Mountain", 5200},
    {"Cloud Ruler Temple", 5500},
    {"Koltoan Mine", 4944},
};

static const land_spots dock_konack = {
    {"1", 8195},
    {"2", 8196},
    {"3", 8197},
    {"4", 8198},
    {"Tiranoc City", 8006},
    {"Great Oroist Temple", 8300},
    {"Elzthuan Forest", 8400},
    {"Mazori Farm", 8447},
    {"Dres", 8500},
    {"Colvian Farm", 8600},
    {"St Alucia", 8700},
    {"Meridius Memorial", 8800},
    {"Desert of Illusion", 8900},
    {"Plains of Confusion", 8954},
    {"Turlon Fair", 9200},
    {"Wetlands", 9700},
    {"Kerberos", 9855},
    {"Shaeras Mansion", 9864},
    {"Slavinus Ravine", 9900},
    {"Furian Citadel", 9949},
};

static const land_spots dock_vegeta = {
    {"1", 2319},
    {"2", 2318},
    {"3", 2320},
    {"4", 2322},
    {"Vegetos City", 2226},
    {"Blood Dunes", 2600},
    {"Ancestral Mountains", 2616},
    {"Destopa Swamp", 2709},
    {"Pride Forest", 2800},
    {"Pride Tower", 2899},
    {"Ruby Cave", 2615},
};

static const land_spots dock_namek = {
    {"1", 11628},
    {"2", 11629},
    {"3", 11630},
    {"4", 11627},
    {"Senzu Village", 11600},
    {"Guru's House", 10182},
    {"Crystalline Cave", 10474},
    {"Elder Village", 13300},
    {"Frieza's Ship", 10203},
    {"Kakureta Village", 10922},
};

static const land_spots dock_aether = {
    {"1", 12003},
    {"2", 12004},
    {"3", 12006},
    {"4", 12005},
    {"Haven City", 12010},
    {"Serenity Lake", 12103},
    {"Kaiju Forest", 12300},
    {"Ortusian Temple", 12400},
    {"Silent Glade", 12480},
};

static const land_spots dock_yardrat = {
    {"1", 14003},
    {"2", 14004},
    {"3", 14005},
    {"4", 14006},
    {"Yardra City", 14008},
    {"Jade Forest", 14100},
    {"Jade Cliffs", 14200},
    {"Mount Valaria", 14300},
};

static const land_spots dock_kanassa = {
    {"Aquis City", 14904},
    {"Yunkai Pirate Base", 15655},
};

static const land_spots dock_cerria = {
    {"Cerria Colony", 17531},
    {"Crystalline Forest", 7950},
    {"Fistarl Volcano", 17420},
};

static const land_spots dock_arlia = {
    {"1", 16065},
    {"2", 16066},
    {"3", 16067},
    {"4", 16068},
    {"Janacre", 16009},
    {"Arlian Wasteland", 16544},
    {"Arlia Mine", 16600},
    {"Kemabra Wastes", 16816},
};

static const land_spots dock_zenith = {
    {"Utatlan City", 3412},
    {"Zenith Jungle", 3520},
    {"Ancient Castle", 19600},
};

struct ZoneHierarchy {
    std::string name{};
    std::unordered_set<zone_vnum> children{};
    std::string orbit{};
    std::unordered_set<ZoneFlag> flagsToAdd{};
    land_spots landSpots{};
    land_spots dockSpots{};
};

static const std::vector<ZoneHierarchy> zonesToLink = {
    {"@GEarth@n", {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 58, 67, 77, 130, 131, 134, 159, 162, 164, 195, 224}, "R:50", {ZoneFlag::ether_stream, ZoneFlag::has_moon}, land_earth, dock_earth},
    {"@YVegeta@n", {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33}, "R:53", {}, land_vegeta, dock_vegeta},
    {"@BZenith@n", {34, 35, 196, 215}, "R:57", {ZoneFlag::ether_stream}, land_zenith, dock_zenith},
    {"Majinton", {36, 37}, {}, {}, {}, {}},
    {"@CFrigid@n", {40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 170}, "R:51", {ZoneFlag::has_moon}, land_frigid, dock_frigid},
    {"@gNamek@n", {59, 96, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 128, 132, 133, 144, 154, 260}, "R:54", {ZoneFlag::ether_stream}, land_namek, dock_namek},
    {"Afterlife", {60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 74, 75, 217}, {}, {}, {}, {}},
    {"@BKanassa@n", {76, 149, 150, 151, 152, 153, 156}, "R:58", {}, land_kanassa, dock_kanassa},
    {"@RCerria@n", {78, 79, 174, 175, 176}, "R:198", {}, land_cerria, dock_cerria},
    {"@MKonack@n", {80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 92, 93, 97, 98, 99, 127, 192, 193, 194}, "R:52", {}, land_konack, dock_konack},
    {"@MAether@n", {120, 121, 122, 123, 124, 125, 155}, "R:55", {ZoneFlag::ether_stream, ZoneFlag::has_moon}, land_aether, dock_aether},
    {"Neo Nirvana", {135, 136, 137, 138, 139, 145, 146, 147, 148}, {}, {}, {}, {}},
    {"@GArlia@n", {160, 161, 165, 166, 167, 168, 169}, "R:59", {}, land_arlia, dock_arlia},
    {"Space", {200, 201, 202, 203, 204, 206, 207, 208, 205, 163, 171, 172, 173, 178, 212, 256}, {}, {}, {}, {}},
    {"@mYardrat@n", {140, 141, 142, 143}, "R:56", {}, land_yardrat, dock_yardrat},
};

static void link_zones() {
    int lastZoneID = 0;
    for (const auto& zt : zonesToLink) {
        auto newid = getNextID(lastZoneID, zone_table);
        auto z = std::make_shared<Zone>();
        zone_table[newid] = z;
        z->number = newid;
        z->name = zt.name;
        for(auto cvn : zt.children) {
            if(auto zf = zone_table.find(cvn); zf != zone_table.end()) {
                zf->second->parent = newid;
                z->children.insert(cvn);
            } else {
                basic_mud_log("Warning: zone %d listed as child of %s but does not exist.", cvn, zt.name.c_str());
            }
        }
        if(auto oloc = Location(zt.orbit)) {
            z->launchDestination = oloc.getLocID();
            if(auto r = dynamic_cast<Room*>(oloc.getLoc())) {
                auto oldz = r->getZone();
                oldz->rooms.remove(r->shared_from_this());
                r->zone.reset(z.get());
                z->rooms.add(r->shared_from_this());
            } else {
                basic_mud_log("Warning: Orbit location %s for zone %s is not a room.", zt.orbit.c_str(), zt.name.c_str());
            }
        }
        for(const auto &zf : zt.flagsToAdd) {
            z->zone_flags.set(zf);
        }
        for(const auto &[k, v] : zt.landSpots) {
            z->landingSpots[k] = std::format("R:{}", v);
        }
        for(const auto& [k, v] : zt.dockSpots) {
            z->dockingSpots[k] = std::format("R:{}", v);
        }
    }
}

static void convert_reset_commands() {
    for (const auto& [zone, commands] : oldResetCommands) {
        for (const auto& cmd : commands) {
            // Convert each command to the new format...
            Room* r = nullptr;
            switch(cmd.command) {
                case 'M':
                case 'O':
                case 'T':
                case 'V':
                    r = get_room(cmd.arg3);
                    break;
                case 'D':
                case 'R':
                    r = get_room(cmd.arg1);
                    break;
            }

            if(!r) continue;

            auto &c = r->resetCommands.emplace_back();
            c.if_flag = cmd.if_flag;

            switch(cmd.command) {
                case 'M':
                    c.type = ResetCommandType::MOB;
                    c.target = cmd.arg1;
                    c.max = cmd.arg2;
                    c.max_location = cmd.arg4;
                    c.chance = 100 - cmd.arg5;
                    break;
                case 'G':
                    c.type = ResetCommandType::GIVE;
                    c.target = cmd.arg1;
                    c.max = cmd.arg2;
                    c.chance = 100 - cmd.arg5;
                    break;
                case 'O':
                    c.type = ResetCommandType::OBJ;
                    c.target = cmd.arg1;
                    c.max = cmd.arg2;
                    c.max_location = cmd.arg4;
                    c.chance = 100 - cmd.arg5;
                    break;
                case 'E':
                    c.type = ResetCommandType::EQUIP;
                    c.target = cmd.arg1;
                    c.max = cmd.arg2;
                    c.ex = cmd.arg3;
                    c.chance = 100 - cmd.arg5;
                    break;
                case 'P':
                    c.type = ResetCommandType::PUT;
                    c.target = cmd.arg1;
                    c.ex = cmd.arg3;
                    c.max = cmd.arg2;
                    c.chance = 100 - cmd.arg5;
                    break;
                case 'R':
                    c.type = ResetCommandType::REMOVE;
                    c.target = cmd.arg1;
                    break;
                case 'D':
                    c.type = ResetCommandType::DOOR;
                    c.target = cmd.arg2;
                    c.ex = cmd.arg3;
                    break;
                case 'T':
                    c.type = ResetCommandType::TRIGGER;
                    c.target = cmd.arg2;
                    c.ex = cmd.arg1;
                    break;
                case 'V':
                    c.type = ResetCommandType::VARIABLE;
                    c.ex = cmd.arg1;
                    c.key = cmd.sarg1;
                    c.value = cmd.sarg2;
                    break;
            }

        }
    }
}

static const std::unordered_map<WhereFlag, std::string> spacetiles = {
    {WhereFlag::earth_orbit, "@GE@n"},
    {WhereFlag::cerria_orbit, "@MC@n"},
    {WhereFlag::frigid_orbit, "@CF@n"},
    {WhereFlag::namek_orbit, "@gN@n"},
    {WhereFlag::konack_orbit, "@mK@n"},
    {WhereFlag::aether_orbit, "@BA@n"},
    {WhereFlag::yardrat_orbit, "@MY@n"},
    {WhereFlag::kanassa_orbit, "@CK@n"},
    {WhereFlag::cerria_orbit, "@MC@n"},
    {WhereFlag::arlia_orbit, "@m@n"},
    {WhereFlag::zenith_orbit, "@CZ@n"},
    {WhereFlag::vegeta_orbit, "@YV@n"},
    {WhereFlag::nebula, "@m&@n"},
    {WhereFlag::asteroid, "@y:@n"},
    {WhereFlag::wormhole, "@b*@n"},
    {WhereFlag::space_station, "@DS@n"},
    {WhereFlag::star, "@yX@n"}
};

static std::string renderSpace() {
    std::string output;
    for (int row = 0; row < 200; ++row) {
        for (int col = 0; col < 200; ++col) {
            auto vn = mapnums[row][col];
            if(vn == NOTHING) {
                output += " ";
                continue;
            }
            auto r = get_room(vn);
            if(!r) {
                output += " ";
                continue;
            }
            bool found = false;
            for(const auto& [flag, tile] : spacetiles) {
                if(r->where_flags[flag]) {
                    output += tile;
                    found = true;
                    break;
                }
            }
            if(!found) {
                output += " ";
            }

        }
        output += "\r\n";
    }
    return output;
}

static void printSpace() {
    auto space = renderSpace();
    std::filesystem::path spaceFile = "space.txt";
    std::ofstream ofs(spaceFile);
    if (ofs) {
        ofs << processColors(space, true, nullptr);
    }
    ofs.close();
}


struct PlanetCenterPoint {
    std::string name{};
    room_vnum vn{NOTHING};
    std::string tilePrint;
    int radius{0};
};

static const std::vector<PlanetCenterPoint> centerPoints = {
    {"earth", 40979, "@GE@n", 2},
    {"vegeta", 32365, "@YV@n", 2},
    {"frigid", 30889, "@CF@n", 2},
    {"namek", 42880, "@gN@n", 2},
    {"konack", 27065, "@mK@n", 2},
    {"aether", 41959, "@BA@n", 2},
    {"yardrat", 34899, "@MY@n", 2},
    {"kanassa", 53859, "@CK@n", 2},
    {"cerria", 59071, "@MC@n", 2},
    {"arlia", 52434, "@mA@n", 2},
    {"zenith", 50772, "@cZ@n", 1},
};

static std::unordered_map<room_vnum, Coordinates> spaceCoordinates;

void calculateSpaceCoordinates()
{
    for (int row = 0; row < 200; ++row) {
        for (int col = 0; col < 200; ++col) {
            auto vn = mapnums[row][col];
            Coordinates out;
            // Cartesian mapping:
            // - origin at center (MAP_W/2, MAP_H/2)
            // - +x to the right, +y up (so rows are inverted)
            out.x = col - (200 / 2);   // range: [-100, 99]
            out.y = (200 / 2) - row;   // range: [100, -99]
            out.z = 0;                   // space plane is z = 0
            spaceCoordinates[vn] = out;
        }
    }
}

static const std::unordered_map<WhereFlag, std::string> extraTiles = {
    {WhereFlag::nebula, "@m&@n"},
    {WhereFlag::asteroid, "@y:@n"},
    {WhereFlag::wormhole, "@b@1*@n"},
    {WhereFlag::space_station, "@DS@n"},
    {WhereFlag::star, "@6 @n"}
};

static void migrate_space() {
    calculateSpaceCoordinates();
    auto a = std::make_shared<Area>();
    a->vn = 1;
    areas[a->vn] = a;
    a->name = "Space";
    // Assign zone.
    auto &z = zone_table.at(232);
    a->zone.reset(z.get());
    z->environment[ENV_GRAVITY] = 0.0;

    auto dim = BoxDim::fromCenter({}, 200, 200);
    auto s = std::make_unique<Shape>();
    s->geom = dim;
    s->type = ShapeType::Box;
    s->sectorType = SectorType::space;
    s->name = "@WDepths of Space@n";
    auto sdesc = " @DThis dark void has very little light.  The light it does have comes from\r\ndistant astral bodies such as stars or planets.  Occasional clouds of gas,\r\nasteroids, or comets can be seen.  Other than that it is empty blackness for\r\nthousands upon thousands of miles in every direction.  @n\r\n";
    s->description = sdesc;
    a->shapes["space"] = std::move(s);


    for(const auto& cp : centerPoints) {
        auto coor = spaceCoordinates.at(cp.vn);
        auto rd = RoundDim::disk(coor, cp.radius, 0, 0);
        auto s = std::make_unique<Shape>();
        s->type = ShapeType::Round;
        s->geom = rd;
        s->priority = 1;
        s->tileDisplay = cp.tilePrint;
        s->sectorType = SectorType::space;
        a->shapes[cp.name] = std::move(s);
    }
    a->rebuildShapeIndex();

    basic_mud_log("Re-linking Space exits...");
    for(auto &[vn, r] : Room::registry) {
        auto find = spaceCoordinates.find(vn);
        if(find != spaceCoordinates.end()) {
            // we're working on a room that exists in the old space map.
            // our goals: scan for exits that don't lead to anything in
            // spaceCoordinates. If we find such, we need to create mirroring
            // exits in the new Space Area at those coordinates.

            for(const auto& [dir, dest] : r->getDirections()) {
                if(auto r2 = dynamic_cast<Room*>(dest.getLoc()); r2) {
                    // it's a room (of course it is! but we had to be sure.)
                    if(auto find2 = spaceCoordinates.find(r2->getVnum()); find2 == spaceCoordinates.end()) {
                         // This exit that's in space leads to not-space.
                         // grab a tileOverride from new Space for these new-Space coordinates...
                         auto &t = a->tileOverrides[find->second];
                         t.exits[dir] = dest;
                         basic_mud_log("Created mirroring exit from A:1:%s to %s.", fmt::format("{}", find->second), fmt::format("{}", *r2));
                    }
                }
            }

            // copy reset commands if need be... now amoebas and grosnak and etc will spawn in new-space.
            if(!r->resetCommands.empty()) {
                // copy the reset commands to a tileOverride...
                auto &t = a->tileOverrides[find->second];
                t.resetCommands = r->resetCommands;
            }

            if(!boost::iequals(r->name, "@WDepths of Space@n")) {
                auto &t = a->tileOverrides[find->second];
                t.name = r->name;
            }
            if(!boost::iequals(r->look_description, sdesc)) {
                auto &t = a->tileOverrides[find->second];
                t.look_description = r->look_description;
            }

            for(const auto& [wf, over] : extraTiles) {
                if(r->where_flags[wf]) {
                    auto &t = a->tileOverrides[find->second];
                    t.tileDisplay = over;
                    break;
                }
            }

            // Still need to deal with the darned stars properly. A direct tile copy works fine for now.
            
        } else {
            // We're working on a room that doesn't exist in the old space map.
            // Our goals: scan for exits that lead to space and re-link them
            // to the new Space Area's corresponding coordinates.

            for(const auto& [dir, dest] : r->getDirections()) {
                if(auto spaceroom = dynamic_cast<Room*>(dest.getLoc()); spaceroom) {
                    auto svn = spaceroom->getVnum();
                    if(auto find2 = spaceCoordinates.find(svn); find2 != spaceCoordinates.end()) {
                        Destination newdest = dest;
                        newdest.al = a;
                        newdest.position = find2->second;
                        newdest.locationID = newdest.getLocID();
                        r->exits[dir] = newdest;
                        basic_mud_log("Re-Linked Legacy Room exit %s in %s to %s.", fmt::format("{}", dir), fmt::format("{}", *r), fmt::format("{}", newdest));
                    }
                }
            }
        }
    }

    auto replaceScriptLine = [&](trig_vnum vn, std::vector<std::pair<int, std::string>> patches) {
        if(auto found = trig_index.find(vn); found != trig_index.end()) {
            for(auto& [line, newText] : patches) {
                if(line < 0 || line >= found->second->lines.size()) {
                    basic_mud_log("Warning: Attempted to replace line %d in trigger %d but it does not exist.", line, vn);
                    continue;
                }
                // Replace the line with the new text.
                found->second->lines[line] = {ScriptLineType::COMMAND, newText};
            }
        }
    };

    auto makeID = [&](room_vnum vn) {
        return fmt::format("A:1:{}", spaceCoordinates.at(vn));
    };

    auto makeRoute = [&](const std::string& txt, room_vnum vn) {
        return fmt::format("set {} {}", txt, makeID(vn));
    };

    std::vector<std::pair<int, std::string>> falconPatches = {
        {30, makeRoute("thirdplanetorbit", 33385)},
        {37, makeRoute("route1s1", 35780)},
        {38, makeRoute("route1s2", 35751)},
        {39, makeRoute("route1s3", 35687)},
        {40, makeRoute("route1s4", 40475)},
        {41, makeRoute("route2s1", 42899)},
        {42, makeRoute("route2s2", 42923)},
        {43, makeRoute("route2s3", 42956)},
        {44, makeRoute("route2s4", 42975)},
        {45, makeRoute("route3s1", 37686)},
        {46, makeRoute("route3s2", 33286)},
        {47, makeRoute("route3s3", 33336)},
        {48, makeRoute("route3s4", 33365)},
        {49, makeRoute("route4s1", 35385)},
        {50, makeRoute("route4s2", 37385)},
        {51, makeRoute("route4s3", 39014)},
        {52, makeRoute("route4s4", 40182)}
    };
    replaceScriptLine(3900, falconPatches);

    std::vector<std::pair<int, std::string>> simurghPatches = {
        {30, makeRoute("thirdplanetorbit", 33385)},
        {37, makeRoute("route1s1", 35781)},
        {38, makeRoute("route1s2", 35752)},
        {39, makeRoute("route1s3", 35681)},
        {40, makeRoute("route1s4", 40471)},
        {41, makeRoute("route2s1", 32341)},
        {42, makeRoute("route2s2", 35141)},
        {43, makeRoute("route2s3", 37652)},
        {44, makeRoute("route2s4", 37671)},
        {45, makeRoute("route3s1", 29055)},
        {46, makeRoute("route3s2", 32144)},
        {47, makeRoute("route3s3", 33351)},
        {48, makeRoute("route3s4", 33370)},
        {49, makeRoute("route4s1", 33365)},
        {50, makeRoute("route4s2", 33339)},
        {51, makeRoute("route4s3", 33304)},
        {52, makeRoute("route4s4", 40344)}
    };
    replaceScriptLine(3905, simurghPatches);

    std::vector<std::pair<int, std::string>> zypherPatches = {
        {27, makeRoute("secondplanetorbit", 33385)},
        {30, makeRoute("thirdplanetorbit", 41490)},
        {37, makeRoute("route1s1", 32961)},
        {38, makeRoute("route1s2", 33333)},
        {39, makeRoute("route1s3", 33570)},
        {40, makeRoute("route1s4", 33370)},
        {41, makeRoute("route2s1", 32343)},
        {42, makeRoute("route2s2", 35143)},
        {43, makeRoute("route2s3", 37653)},
        {44, makeRoute("route2s4", 37673)},
        {45, makeRoute("route3s1", 42893)},
        {46, makeRoute("route3s2", 42923)},
        {47, makeRoute("route3s3", 42953)},
        {48, makeRoute("route3s4", 42973)}
    };
    replaceScriptLine(3906, zypherPatches);

    std::vector<std::pair<int, std::string>> valkyriePatches = {
        {30, makeRoute("thirdplanetorbit", 33385)},
        {37, makeRoute("route1s1", 42974)},
        {38, makeRoute("route1s2", 42954)},
        {39, makeRoute("route1s3", 42924)},
        {40, makeRoute("route1s4", 42894)},
        {41, makeRoute("route2s1", 40182)},
        {42, makeRoute("route2s2", 39014)},
        {43, makeRoute("route2s3", 37385)},
        {44, makeRoute("route2s4", 35385)},
        {45, makeRoute("route3s1", 33369)},
        {46, makeRoute("route3s2", 33350)},
        {47, makeRoute("route3s3", 33311)},
        {48, makeRoute("route3s4", 32355)}
    };
    replaceScriptLine(3911, valkyriePatches);

    std::vector<std::pair<int, std::string>> phoenixPatches = {
        {28, makeRoute("secondplanetorbit", 33385)},
        {35, makeRoute("route1s1", 32961)},
        {36, makeRoute("route1s2", 33333)},
        {37, makeRoute("route1s3", 33570)},
        {38, makeRoute("route1s4", 33370)},
        {39, makeRoute("route2s1", 32343)},
        {40, makeRoute("route2s2", 35143)},
        {41, makeRoute("route2s3", 37653)},
        {42, makeRoute("route2s4", 37673)}
    };
    replaceScriptLine(3916, phoenixPatches);

    std::vector<std::pair<int, std::string>> merganserPatches = {
        {28, makeRoute("secondplanetorbit", 33385)},
        {35, makeRoute("route1s1", 32355)},
        {36, makeRoute("route1s2", 33311)},
        {37, makeRoute("route1s3", 33350)},
        {38, makeRoute("route1s4", 43369)},
        {39, makeRoute("route2s1", 43369)},
        {40, makeRoute("route2s2", 33350)},
        {41, makeRoute("route2s3", 33311)},
        {42, makeRoute("route2s4", 32355)}
    };

    replaceScriptLine(3921, merganserPatches);

    std::vector<std::pair<int, std::string>> wraithPatches = {
        {28, makeRoute("secondplanetorbit", 33385)},
        {35, makeRoute("route1s1", 37686)},
        {36, makeRoute("route1s2", 33286)},
        {37, makeRoute("route1s3", 33336)},
        {38, makeRoute("route1s4", 33365)},
        {39, makeRoute("route2s1", 33365)},
        {40, makeRoute("route2s2", 33336)},
        {41, makeRoute("route2s3", 33286)},
        {42, makeRoute("route2s4", 37686)}
    };

    replaceScriptLine(3926, wraithPatches);

    std::vector<std::pair<int, std::string>> ghostPatches = {
        {28, makeRoute("secondplanetorbit", 33385)},
        {35, makeRoute("route1s1", 29055)},
        {36, makeRoute("route1s2", 32144)},
        {37, makeRoute("route1s3", 33351)},
        {38, makeRoute("route1s4", 33370)},
        {39, makeRoute("route2s1", 33370)},
        {40, makeRoute("route2s2", 33351)},
        {41, makeRoute("route2s3", 32144)},
        {42, makeRoute("route2s4", 29055)}
    };
    replaceScriptLine(3931, ghostPatches);

    std::vector<std::pair<int, std::string>> wispPatches = {
        {28, makeRoute("secondplanetorbit", 33385)},
        {35, makeRoute("route1s1", 40344)},
        {36, makeRoute("route1s2", 33304)},
        {37, makeRoute("route1s3", 33339)},
        {38, makeRoute("route1s4", 33365)},
        {39, makeRoute("route2s1", 33365)},
        {40, makeRoute("route2s2", 33339)},
        {41, makeRoute("route2s3", 33304)},
        {42, makeRoute("route2s4", 40344)}
    };
    replaceScriptLine(3936, wispPatches);

    std::vector<std::pair<int, std::string>> eaglePatches = {
        {28, makeRoute("secondplanetorbit", 33385)},
        {35, makeRoute("route1s1", 32961)},
        {36, makeRoute("route1s2", 33333)},
        {37, makeRoute("route1s3", 33570)},
        {38, makeRoute("route1s4", 33370)},
        {39, makeRoute("route2s1", 33370)},
        {40, makeRoute("route2s2", 33570)},
        {41, makeRoute("route2s3", 33333)},
        {42, makeRoute("route2s4", 32961)}
    };

    replaceScriptLine(3941, eaglePatches);

    auto p = findPlayer("Wayland");
    Location loc(a, spaceCoordinates.at(40979));
    p->moveToLocation(loc);


    // delete old space grid, let's see what happens...
    for(const auto&[vn, coor] : spaceCoordinates) Room::registry.erase(vn);
}

void boot_db_world_legacy() {

    basic_mud_log("Loading stat handlers...");
    init_stat_handlers();

    basic_mud_log("Loading legacy world data...");
    basic_mud_log("Loading zone table.");
    index_boot(DB_BOOT_ZON);

    basic_mud_log("Loading triggers and generating index.");
    index_boot(DB_BOOT_TRG);

    basic_mud_log("Loading rooms.");
    index_boot(DB_BOOT_WLD);

    // Generating new zones and linking parents/children...
    link_zones();

    link_exits();

    convert_reset_commands();

    basic_mud_log("Checking start rooms.");
    check_start_rooms();

    basic_mud_log("Loading mobs and generating index.");
    index_boot(DB_BOOT_MOB);

    basic_mud_log("Loading objs and generating index.");
    index_boot(DB_BOOT_OBJ);

    basic_mud_log("Loading shops.");
    index_boot(DB_BOOT_SHP);

    basic_mud_log("Loading guild masters.");
    index_boot(DB_BOOT_GLD);
    boot_db_shadow();


}

static void index_boot_help();
static void assemblyBootAssemblies();

static void boot_db_legacy() {
    boot_db_textfiles();
    boot_db_spellfeats();
    boot_db_world_legacy();
    index_boot_help();
    boot_db_mail();
    boot_db_socials();
    boot_db_commands();
    boot_db_specials();
    assemblyBootAssemblies();
    boot_db_sort();
    boot_db_boards();
    boot_db_spacemap();
    topLoad();
}

// ACTUAL MIGRATION STUFF BELOW...

static std::string stripAnsi(const std::string& str) {
    return processColors(str, false, nullptr);
}

static std::vector<std::pair<std::string, vnum>> characterToAccount;

void migrate_accounts() {
	// user files are stored in <cwd>/user/<folder>/<file>.usr so we'll need to do recursive iteration using
    // C++ std::filesystem...

    auto path = std::filesystem::current_path() / "data" / "user";
    if(!std::filesystem::exists(path)) {
        basic_mud_log("No user directory found, skipping account migration.");
        return;
    }

    std::unordered_map<Account*, std::string> pendingPasswords;

    for(auto &p : std::filesystem::recursive_directory_iterator(path)) {
        if(p.path().extension() != ".usr") continue;
        auto accFile = p.path().parent_path().filename().string();

        // Open file for reading...
        std::ifstream file(p.path());

        // Step 1: create an ID for this account...
        auto id = getNextID(lastAccountID, accounts);

        // Now let's get a new account_data...
        auto a = std::make_shared<Account>();
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
        a->password = pass;
        
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
        a->admin_level = std::stoi(adminLevel);

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
        a->id = id;
    }
}

void migrate_characters() {
    // Unlike accounts, player files are indexed. However, unless their name showed up in an account,
    // there's no point migrating them.

    // The procedure we will use is: iterate through characterToAccount and attempt to load the character.
    // if we can load them, we'll convert them and bind them to the appropriate account.

    for(auto &[cname, accID] : characterToAccount) {
        auto sh = std::make_shared<Character>();
        if(load_char(cname.c_str(), sh.get()) < 0) {
            basic_mud_log("Error loading %s for account migration.", cname.c_str());
            sh.reset();
            continue;
        }
        auto ch = sh.get();
        ch->isPC = true;
        convert_character(ch);
        auto id = getNextID(Character::lastID, Character::registry);
        auto p = std::make_shared<PlayerData>();
        players[id] = p;
        p->id = id;
        ch->id = id;
        p->character = ch;
        p->name = ch->getName();
        auto &a = accounts[accID];
        p->account = a.get();
        a->admin_level = std::max(a->admin_level, GET_ADMLEVEL(ch));
        a->characters.emplace_back(ch->id);
        //auto lroom = ch->getBaseStat<room_vnum>("load_room");
        //ch->setBaseStat<room_vnum>("was_in_room", lroom);
        Character::registry.emplace(id, sh);
    }

    // migrate sense files...
    auto path = std::filesystem::current_path() / "data" / "sense";
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
        auto pa = players.at(ch->id);
        // The file contains a sequence of lines, with each line containing a number.
		// The number is the vnum of a mobile the player's sensed.
        // We will read each line and insert the vnum into the player's sensed list.
        std::ifstream file(p.path());
        std::string line;
        while(std::getline(file, line)) {
            try {
                auto vnum = std::stoi(line);
                if(mob_proto.contains(vnum)) pa->sense_memory.insert(vnum);
            } catch(...) {
                basic_mud_log("Error parsing %s for sense migration.", line.c_str());
            }
        }
        file.close();
    }

    path = std::filesystem::current_path() / "data" / "intro";
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

        auto pa = players.at(ch->id);

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
            pa->dub_names[pc->id] = dub;
        }
    }

    path = std::filesystem::current_path() / "data" / "plrvars";
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
                auto ctx = std::stoi(context);
                ch->variables[varname] = data;
            } catch(...) {
                basic_mud_log("Error parsing %s for variable migration.", line.c_str());
            }
        }
    }

    path = std::filesystem::current_path() / "data" / "plralias";
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
        auto pa = players.find(ch->id);
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

    path = std::filesystem::current_path() / "data" / "plrobjs";

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

static void migrate_aff(affect_t *aff) {
    switch(aff->location) {
        // Old Attributes
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6: {
            auto newLoc = 1 << (aff->location - 1);
            aff->location = APPLY_CATTR_BASE;
            aff->specific = newLoc;
        }
            break;
        case 12: // old Ki
        case 37:
            aff->location = APPLY_CVIT_BASE;
            aff->specific = static_cast<int>(CharVital::ki);
            break;
        case 13: // old hit
            aff->location = APPLY_CVIT_BASE;
            aff->specific = static_cast<int>(CharVital::health);
            break;
        case 14: // old stamina
            aff->location = APPLY_CVIT_BASE;
            aff->specific = static_cast<int>(CharVital::stamina);
            break;
        case 25: // old apply pl mult
            aff->location = APPLY_CVIT_MULT;
            aff->specific = static_cast<int>(CharVital::health);
            break;
        case 26: // old apply ki mult
            aff->location = APPLY_CVIT_MULT;
            aff->specific = static_cast<int>(CharVital::ki);
            break;
        case 27: // old apply st mult
            aff->location = APPLY_CVIT_MULT;
            aff->specific = static_cast<int>(CharVital::stamina);
            break;
        case 28: // old apply lf mult
            aff->location = APPLY_CVIT_MULT;
            aff->specific = static_cast<int>(CharVital::lifeforce);
            break;
        case 29: // old apply vitals mult
            aff->location = APPLY_CVIT_MULT;
            aff->specific = ~0;
            break;
        case 46: // old apply all vitals
            aff->location = APPLY_CVIT_BASE;
            aff->specific = ~0;
            break;
        case 16: // old experience gain mult
            aff->location = APPLY_CSTAT_GAIN_MULT;
            aff->specific = static_cast<int>(CharStat::experience);
            break;
        case 21: // old skill stat gain mult
            aff->location = APPLY_CSTAT_BASE;
            aff->specific = static_cast<int>(CharStat::skill_train);
            break;
        case 22:
            aff->location = APPLY_CVIT_MULT;
            aff->specific = static_cast<int>(CharVital::lifeforce);
            aff->modifier /= 100.0;
            break;
        case 17: // old armor / ac
            aff->location = APPLY_COMBAT_BASE;
            aff->specific = static_cast<int>(ComStat::armor);
            break;
        case 18: // old accuracy
            aff->location = APPLY_COMBAT_MULT;
            aff->specific = static_cast<int>(ComStat::accuracy);
            break;
        case 32: // old phys dam perc
            aff->location = APPLY_DTYPE_BON;
            aff->specific = static_cast<int>(DamType::physical);
            break;
        case 33: // old ki dam perc
            aff->location = APPLY_DTYPE_BON;
            aff->specific = static_cast<int>(DamType::ki);
            break;
        case 34: // old phys res perc
            aff->location = APPLY_DTYPE_RES;
            aff->specific = static_cast<int>(DamType::physical);
            break;
        case 35: // old ki res perc
            aff->location = APPLY_DTYPE_RES;
            aff->specific = static_cast<int>(DamType::ki);
            break;
        case 41: // old skill
            aff->location = APPLY_SKILL;
            break;
        default:
            break;
    }
}

template<typename T>
static void migrate_char_data(T* c) {
    // convert affected
    for(auto aff = c->affected; aff; aff = aff->next) {
        migrate_aff(aff);
    }

    for(auto aff = c->affectedv; aff; aff = aff->next) {
        migrate_aff(aff);
    }


    // convert affectedv
}

void combine_and_trim(ObjectBase* o) {
    auto& aff = o->affected;
    if (aff.empty()) return;

    // Iterate from tail to head (i = size-1 ... 1)
    for (std::size_t i = aff.size(); i-- > 0; ) {
        if (aff[i].location == 0) continue; // skip empty slots

        for (std::size_t j = 0; j < i; ++j) {
            if (aff[j].location == aff[i].location &&
                aff[j].modifier == aff[i].modifier &&
                aff[j].isBitwise()) {
                // Combine the specifics with bitwise OR into the earlier slot
                aff[j].specific |= aff[i].specific;

                // Clear the tail-wards element (same effect as your original)
                aff[i].location = 0;
                aff[i].modifier = 0.0;
                aff[i].specific = 0;
                break;
            }
        }
    }

    std::erase_if(aff, [](const affected_type& a) { return a.location == 0; });
    o->affected.shrink_to_fit();
}

template<typename T>
static void migrate_obj_data(T* o) {
    // Convert obj_affects

    for(auto &aff : o->affected) {
        migrate_aff(&aff);
    }

    combine_and_trim(o);

    // First let's cconvert all relevant flags to new data structures.
    for(auto i = 0; i < 96; i++) {
        // Skip if it's not set.
        if(!o->item_flags.get(i)) continue;

        bool resetFlag = true;

        // Convert it.
        switch(i) {
            case 9: // ITEM_ANTI_GOOD
                o->not_alignment.set(MoralAlign::good);
                break;
            case 10: // ITEM_ANTI_EVIL
                o->not_alignment.set(MoralAlign::evil);
                break;
            case 11: // ITEM_ANTI_NEUTRAL
                o->not_alignment.set(MoralAlign::neutral);
                break;
            case 12: // ITEM_ANTI_ROSHI
                o->not_sensei.set(Sensei::roshi);
                break;
            case 13:
                o->not_sensei.set(Sensei::piccolo);
                break;
            case 14:
                o->not_sensei.set(Sensei::crane);
                break;
            case 15:
                o->not_sensei.set(Sensei::nail);
                break;
            case 17:
                o->not_sensei.set(Sensei::tapion);
                break;
            case 19:
                o->not_sensei.set(Sensei::sixteen);
                break;
            case 20:
                o->not_sensei.set(Sensei::dabura);
                break;
            case 21:
                o->not_sensei.set(Sensei::ginyu);
                break;
            case 22: // ITEM_ANTI_HUMAN
                o->not_race.set(Race::human);
                break;
            case 23: // ITEM_ANTI_ICER
                o->not_race.set(Race::icer);
                break;
            case 24: // ITEM_ANTI_SAIYAN
                o->not_race.set(Race::saiyan);
                break;
            case 25: // ITEM_ANTI_KONATSU
                o->not_race.set(Race::konatsu);
                break;
            case 29:
                o->not_sensei.set(Sensei::bardock);
                break;
            case 30:
                o->not_sensei.set(Sensei::kibito);
                break;
            case 31:
                o->not_sensei.set(Sensei::frieza);
                break;
            case 41: // ITEM_ONLY_HUMAN
                o->only_race.set(Race::human);
                break;
            case 42: // ITEM_ONLY_ICER
                o->only_race.set(Race::icer);
                break;
            case 43: // ITEM_ONLY_SAIYAN
                o->only_race.set(Race::saiyan);
                break;
            case 44: // ITEM_ONLY_KONATSU
                o->only_race.set(Race::konatsu);
                break;
            case 45:
                o->only_sensei.set(Sensei::bardock);
                break;
            case 46:
                o->only_sensei.set(Sensei::kibito);
                break;
            case 47:
                o->only_sensei.set(Sensei::frieza);
                break;
            case 50:
                o->not_sensei.set(Sensei::kurzak);
                break;
            case 51:
                o->only_sensei.set(Sensei::kurzak);
                break;
            case 75:
                o->only_sensei.set(Sensei::jinto);
                break;
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
            case 38:
            case 39:
            case 40:
            case 48:
            case 49:
            case 52:
            case 53:
            case 54:
            case 55:
            case 56:
            case 58:
            case 59:
            case 60:
            case 61:
            case 62:
                break;
            case 63: // ITEM_BSCOUTER
                o->setBaseStat(VAL_WORN_SCOUTER, 500000);
                break;
            case 64: // ITEM_MSCOUTER
                o->setBaseStat(VAL_WORN_SCOUTER, 10000000);
                break;
            case 65: // ITEM_ASCOUTER
                o->setBaseStat(VAL_WORN_SCOUTER, 150000000);
                break;
            case 66: // ITEM_USCOUTER
                o->setBaseStat(VAL_WORN_SCOUTER, std::numeric_limits<int64_t>::max());
                break;
            case 67: // ITEM_WEAPLVL1...
            case 68:
            case 69:
            case 70:
            case 71:
                o->setBaseStat(VAL_WEAPON_LEVEL, i - 66);
                break;
            default:
                resetFlag = false;
                break;
        }

        if(resetFlag) {
            o->item_flags.set(i, false);
        }

    }

    // now we'll compress the remaining flag space.
    for(auto i = 0; i < 96; i++) {
        if(!o->item_flags.get(i)) continue;

        switch(i) {
            case 16:
                o->item_flags.set(9, true);
                break;
            case 18:
                o->item_flags.set(10, true);
                break;
            case 26:
            case 27:
            case 28:
                o->item_flags.set(i - 15, true);
                break;
            case 32:
                o->item_flags.set(14, true);
                break;
            case 57:
                o->item_flags.set(15, true);
                break;
            case 72:
            case 73:
            case 74:
                o->item_flags.set(i - 56, true);
                break;
            default:
                if(i >= 76 && i <= 94) {
                    o->item_flags.set(i - 57, true);
                }

                break;
        }

    }


}


static void index_boot_help() {
    const char *index_filename, *prefix = nullptr;    /* nullptr or egcs 1.1 complains */
    FILE *db_index, *db_file;
    int rec_count = 0, size[2];
    char buf2[PATH_MAX], buf1[MAX_STRING_LENGTH];
    int mode = DB_BOOT_HLP;

    switch (mode) {
        case DB_BOOT_HLP:
            prefix = HLP_PREFIX;
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
            rec_count += count_alias_records(db_file);
        }

        fclose(db_file);
        fscanf(db_index, "%s\n", buf1);
    }

    /* Exit if 0 records, unless this is shops */
    if (!rec_count) {
        return;
    }

    /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
    switch (mode) {
        case DB_BOOT_HLP:
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
            case DB_BOOT_HLP:
                load_help(db_file, buf2);
                break;
        }

        fclose(db_file);
        fscanf(db_index, "%s\n", buf1);
    }
    fclose(db_index);
}

static void assemblyBootAssemblies() {
    char szLine[MAX_STRING_LENGTH] = {'\0'};
    char szTag[MAX_STRING_LENGTH] = {'\0'};
    char szType[MAX_STRING_LENGTH] = {'\0'};
    int iExtract = 0;
    int iInRoom = 0;
    int iType = 0;
    long lLineCount = 0;
    long lPartVnum = NOTHING;
    long lVnum = NOTHING;
    FILE *pFile = nullptr;

    if ((pFile = fopen(ASSEMBLIES_FILE, "rt")) == nullptr) {
        basic_mud_log("SYSERR: assemblyBootAssemblies(): Couldn't open file '%s' for "
            "reading.", ASSEMBLIES_FILE);
        return;
    }

    while (!feof(pFile)) {
        lLineCount += get_line(pFile, szLine);
        half_chop(szLine, szTag, szLine);

        if (*szTag == '\0')
            continue;

        if (strcasecmp(szTag, "Component") == 0) {
            if (sscanf(szLine, "#%ld %d %d", &lPartVnum, &iExtract, &iInRoom) != 3
                    ) {
                basic_mud_log("SYSERR: bootAssemblies(): Invalid format in file %s, line %ld: "
                    "szTag=%s, szLine=%s.", ASSEMBLIES_FILE, lLineCount, szTag, szLine);
            } else if (!assemblyAddComponent(lVnum, lPartVnum, iExtract, iInRoom)) {
                basic_mud_log("SYSERR: bootAssemblies(): Could not add component #%ld to "
                    "assembly #%ld.", lPartVnum, lVnum);
            }
        } else if (strcasecmp(szTag, "Vnum") == 0) {
            if (sscanf(szLine, "#%ld %s", &lVnum, szType) != 2) {
                basic_mud_log("SYSERR: bootAssemblies(): Invalid format in file %s, "
                    "line %ld.", ASSEMBLIES_FILE, lLineCount);
                lVnum = NOTHING;
            } else if ((iType = search_block(szType, AssemblyTypes, true)) < 0) {
                basic_mud_log("SYSERR: bootAssemblies(): Invalid type '%s' for assembly "
                    "vnum #%ld at line %ld.", szType, lVnum, lLineCount);
                lVnum = NOTHING;
            } else if (!assemblyCreate(lVnum, iType)) {
                basic_mud_log("SYSERR: bootAssemblies(): Could not create assembly for vnum "
                    "#%ld, type %s.", lVnum, szType);
                lVnum = NOTHING;
            }
        } else {
            basic_mud_log("SYSERR: Invalid tag '%s' in file %s, line #%ld.", szTag,
                ASSEMBLIES_FILE, lLineCount);
        }

        *szLine = '\0';
        *szTag = '\0';
    }

    fclose(pFile);
}

void migrate_data() {

    basic_mud_log("Converting Item Prototypes...");

    for(auto &[vn, o] : obj_proto) {

        migrate_obj_data(o.get());

    }

    basic_mud_log("Converting Item Instances...");

    for(auto &[id, ent] : Object::registry) {
        migrate_obj_data(ent.get());
    }

    for(auto &[id, ent] : mob_proto) {
        //migrate_char_data(&ent);
    }

    for(auto &[id, ent] : Character::registry) {
        migrate_char_data(ent.get());
    }

}

void migrate_db() {
    boot_db_legacy();
    House_boot();

    migrate_accounts();

    migrate_data();

    try {
        migrate_characters();
    } catch(std::exception &e) {
        basic_mud_log("Error migrating characters: %s", e.what());
    }
}

void run_migration() {
    isMigrating = true;
    load_config();
    game::init_locale();
    migrate_db();
    // let's experiment here...
    migrate_space();
    printSpace();
    runSave();
    destroy_db();
}

int main(int argc, char **argv) {
    run_migration();
    return 0;
}