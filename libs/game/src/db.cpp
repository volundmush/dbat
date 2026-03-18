/*************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include <fstream>

#include "dbat/game/Log.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/CharacterPrototype.hpp"
#include "dbat/game/ObjectPrototype.hpp"
//#include "dbat/game/Area.hpp"
#include "dbat/game/db.hpp"
//#include "dbat/game/send.hpp"
//#include "dbat/game/feats.hpp"
#include "dbat/game/config.hpp"
#include "dbat/game/players.hpp"
//#include "dbat/game/spec_assign.hpp"
//#include "dbat/game/act.informative.hpp"
#include "dbat/game/act.other.hpp"
//#include "dbat/game/act.social.hpp"
//#include "dbat/game/assemblies.hpp"
//#include "dbat/game/reset.hpp"
#include "dbat/game/class.hpp"
//#include "dbat/game/comm.hpp"
#include "dbat/game/dg_scripts.hpp"
#include "dbat/game/interpreter.hpp"
#include "dbat/game/Shop.hpp"
//#include "dbat/game/Guild.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/mail.hpp"
//#include "dbat/game/boards.hpp"
#include "dbat/game/constants.hpp"
//#include "dbat/game/spells.hpp"
#include "dbat/game/races.hpp"
//#include "dbat/game/maputils.hpp"
#include "dbat/util/FilterWeak.hpp"
#include "dbat/game/utils.hpp"

#include "dbat/game/Create.hpp"

#include "dbat/game/Random.hpp"
#include "dbat/game/ID.hpp"
//#include "dbat/game/Help.hpp"


/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

bool gameIsLoading = true;
bool saveAll = false;
bool isMigrating = false;


struct config_data config_info; /* Game configuration list.    */

// The global database of entities.


Character *affect_list = nullptr; /* global linked list of chars with affects */
Character *affectv_list = nullptr; /* global linked list of chars with round-based affects */

std::vector<std::weak_ptr<Character>> getAllCharacters() {
    std::vector<std::weak_ptr<Character>> out;
    out.reserve(Character::registry.size());

    for(const auto&[id, ent] : Character::registry)
        out.emplace_back(ent);

    return out;
}

std::vector<std::weak_ptr<Object>> getAllObjects() {
    std::vector<std::weak_ptr<Object>> out;
    out.reserve(Object::registry.size());

    for(const auto&[id, ent] : Object::registry)
        out.emplace_back(ent);

    return out;
}

Room* get_room(room_vnum vn) {
    if(auto it = Room::registry.find(vn); it != Room::registry.end())
        return it->second.get();
    return nullptr;
}

int dg_owner_purged;            /* For control of scripts */

void destroy_db() {
    for(auto &[id, ent] : Character::registry) {
        if(ent) ent->deactivate();
    }
    Character::registry.clear();
    for(auto &[id, ent] : Object::registry) {
        if(ent) ent->deactivate();
    }
    Object::registry.clear();

    for(auto &[id, ent] : Room::registry) {
        if(ent) ent->deactivate();
    }
    Room::registry.clear();

}

int no_mail = 0;        /* mail disabled?		 */
int mini_mud = 0;        /* mini-mud mode?		 */
int no_rent_check = 0;        /* skip rent check on boot?	 */
time_t boot_time = 0;        /* time of mud boot		 */
int circle_restrict = 0;    /* level of game restriction	 */

room_rnum r_mortal_start_room;    /* rnum of mortal start room	 */
room_rnum r_immort_start_room;    /* rnum of immort start room	 */
room_rnum r_frozen_start_room;    /* rnum of frozen start room	 */
int converting = false;

char *credits = nullptr;        /* game credits			 */
char *news = nullptr;        /* mud news			 */
char *motd = nullptr;        /* message of the day - mortals  */
char *imotd = nullptr;        /* message of the day - immorts  */
char *GREETINGS = nullptr;        /* opening credits screen	 */
char *GREETANSI = nullptr;        /* ansi opening credits screen	 */
char *help = nullptr;        /* help screen			 */
char *info = nullptr;        /* info page			 */
char *wizlist = nullptr;        /* list of higher gods		 */
char *immlist = nullptr;        /* list of peon gods		 */
char *background = nullptr;    /* background story		 */
char *handbook = nullptr;        /* handbook for new immortals	 */
char *policies = nullptr;        /* policies page		 */
char *ihelp = nullptr;        /* help screen (immortals)	 */


/* local functions */
static void dragon_level(Character *ch);

static int count_alias_records(FILE *fl);

static void get_one_line(FILE *fl, char *buf);

static void log_zone_error(zone_rnum zone, int cmd_no, const char *message);



void create_command_list();

void memorize_add(Character *ch, int spellnum, int timer);

void free_feats();

void free_assemblies();


/* external vars */


static void dragon_level(Character *ch) {
    struct descriptor_data *d;
    int level = 0, count = 0;

    for (d = descriptor_list; d; d = d->next) {
        if (IS_PLAYING(d) && GET_ADMLEVEL(d->character) < 1) {
            level += GET_LEVEL(d->character);
            count += 1;
        }
    }

    if (level > 0 && count > 0) {
        level = level / count;
    } else {
        level = Random::get<int>(60, 110);
    }

    if (level < 50) {
        level = Random::get<int>(40, 60);
    }

    ch->setBaseStat<int>("level", level + Random::get<int>(5, 20));
}


/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists() {
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
}

/* Wipe out all the loaded text files, for shutting down. */
void free_text_files() {
    char **textfiles[] = {
            &wizlist, &immlist, &news, &credits, &motd, &imotd, &help, &info,
            &policies, &handbook, &background, &GREETINGS, &GREETANSI, &ihelp, nullptr
    };
    int rf;

    for (rf = 0; textfiles[rf]; rf++)
        if (*textfiles[rf]) {
            free(*textfiles[rf]);
            *textfiles[rf] = nullptr;
        }
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot) {
        ch->sendText("Not a thing anymore.\r\n");
}


/* Free the world, in a memory allocation sense. */

/* You can define this to anything you want; 1 would work but it would
   be very inefficient. I would recommend that it actually be close to
   your total number of in-game objects if not double or triple it just
   to minimize collisions. The only O(n) [n=NUM_OBJ_UNIQUE_POOLS]
   operation is initialization of the hash table, all other operations
   that have to traverse are O(n) [n=num elements in pool], so more
   pools are better.
     - Elie Rosenblum Dec. 12 2003 */
constexpr int NUM_OBJ_UNIQUE_POOLS = 5000;


time_t old_beginning_of_time;

bitvector_t asciiflag_conv(char *flag) {
    bitvector_t flags = 0;
    int is_num = true;
    char *p;

    for (p = flag; *p; p++) {
        if (islower(*p))
            flags |= 1 << (*p - 'a');
        else if (isupper(*p))
            flags |= 1 << (26 + (*p - 'A'));

        if (!(isdigit(*p) || (*p == '-')))
            is_num = false;
    }

    if (is_num)
        flags = atoi(flag);

    return (flags);
}

/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms() {
    if ((r_mortal_start_room = real_room(CONFIG_MORTAL_START)) == NOWHERE) {
        basic_mud_log("SYSERR:  Mortal start room does not exist.  Change mortal_start_room in lib/etc/config.");
        exit(1);
    }
    if ((r_immort_start_room = real_room(CONFIG_IMMORTAL_START)) == NOWHERE) {
        if (!mini_mud)
            basic_mud_log("SYSERR:  Warning: Immort start room does not exist.  Change immort_start_room in /lib/etc/config.");
        r_immort_start_room = r_mortal_start_room;
    }
    if ((r_frozen_start_room = real_room(CONFIG_FROZEN_START)) == NOWHERE) {
        if (!mini_mud)
            basic_mud_log("SYSERR:  Warning: Frozen start room does not exist.  Change frozen_start_room in /lib/etc/config.");
        r_frozen_start_room = r_mortal_start_room;
    }
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/

int vnum_mobile(char *searchname, Character *ch) {
    int found = 0;

    for (auto &[vn, m] : mob_proto)
        if (isname(searchname, m->name))
                        ch->send_to("%3d. [%5d] %-40s %s\r\n", ++found, vn, m->short_description, !m->proto_script.empty() ? m->scriptString().c_str() : "");

    return (found);
}


int vnum_object(char *searchname, Character *ch) {
    int found = 0;

    for (auto &o : obj_proto)
        if (isname(searchname, o.second->name))
                        ch->send_to("%3d. [%5d] %-40s %s\r\n", ++found, o.first, o.second->short_description, !o.second->proto_script.empty() ? o.second->scriptString().c_str() : "");

    return (found);
}


int vnum_material(char *searchname, Character *ch) {
    int found = 0;

    for (auto &o : obj_proto)
        if (isname(searchname, material_names[o.second->getBaseStat<int>(VAL_ALL_MATERIAL)])) {
                        ch->send_to("%3d. [%5d] %-40s %s\r\n", ++found, o.first, o.second->short_description, !o.second->proto_script.empty() ? o.second->scriptString().c_str() : "");
        }

    return (found);
}


int vnum_weapontype(char *searchname, Character *ch) {
    int found = 0;

    for (auto &o : obj_proto)
        if (o.second->type_flag == ItemType::weapon) {
            if (isname(searchname, weapon_type[o.second->getBaseStat<int>(VAL_WEAPON_SKILL)])) {
                                ch->send_to("%3d. [%5d] %-40s %s\r\n", ++found, o.first, o.second->short_description, !o.second->proto_script.empty() ? o.second->scriptString().c_str() : "");
            }
        }

    return (found);
}


int vnum_armortype(char *searchname, Character *ch) {
    int found = 0;

    for (auto &o : obj_proto)
        if (o.second->type_flag == ItemType::armor) {
            if (isname(searchname, armor_type[o.second->getBaseStat<int>(VAL_ARMOR_SKILL)])) {
                                ch->send_to("%3d. [%5d] %-40s %s\r\n", ++found, o.first, o.second->short_description, !o.second->proto_script.empty() ? o.second->scriptString().c_str() : "");
            }
        }

    return (found);
}


/* create a new mobile from a prototype */
Character *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
    auto proto = mob_proto.find(nr);

    if(proto == mob_proto.end()) {
        basic_mud_log("WARNING: Mobile vnum %d does not exist in database.", nr);
        return (nullptr);
    }
    auto sh = createEntity<Character>();
    auto mob = sh.get();

    *mob = *(proto->second);

    mob->activate();

    if (!(IS_HOSHIJIN(mob) && GET_SEX(mob) == SEX_MALE)) {
        //setNumsTo[CharAppearance::hair_length] = Random::get<int>(0, 4);
        //setNumsTo[CharAppearance::hair_color] = Random::get<int>(1, 13);
        //setNumsTo[CharAppearance::hair_style] = Random::get<int>(1, 11);
    }

    //setNumsTo[CharAppearance::eye_color] = Random::get<int>(0, 11);

    if (!IS_HUMAN(mob) && !IS_SAIYAN(mob) && !IS_HALFBREED(mob) && !IS_NAMEK(mob)) {
        //setNumsTo[CharAppearance::skin_color] = Random::get<int>(0, 11);
    }
    if (IS_NAMEK(mob)) {
        //setNumsTo[CharAppearance::skin_color] = 2;
    }
    if (IS_HUMAN(mob) || IS_SAIYAN(mob) || IS_HALFBREED(mob)) {
        /*
        if (Random::get<int>(1, 5) <= 2) {
            setNumsTo[CharAppearance::skin_color] = Random::get<int>(0, 1);
        } else if (Random::get<int>(1, 5) <= 4) {
            setNumsTo[CharAppearance::skin_color] = Random::get<int>(4, 5);
        } else if (Random::get<int>(1, 5) <= 5) {
            setNumsTo[CharAppearance::skin_color] = Random::get<int>(9, 10);
        }
            */
    }
    if (IS_SAIYAN(mob)) {
        //setNumsTo[CharAppearance::hair_color] = HAIRC_BLACK;
        //setNumsTo[CharAppearance::eye_color] = 1;
    }

    if (GET_MOB_VNUM(mob) >= 81 && GET_MOB_VNUM(mob) <= 87) {
        dragon_level(mob);
    }

    int64_t mult = 0;

    switch (GET_LEVEL(mob)) {
        case 1:
            mult = Random::get<int>(50, 80);
            break;
        case 2:
            mult = Random::get<int>(90, 120);
            break;
        case 3:
            mult = Random::get<int>(100, 140);
            break;
        case 4:
            mult = Random::get<int>(120, 180);
            break;
        case 5:
            mult = Random::get<int>(200, 250);
            break;
        case 6:
            mult = Random::get<int>(240, 300);
            break;
        case 7:
            mult = Random::get<int>(280, 350);
            break;
        case 8:
            mult = Random::get<int>(320, 400);
            break;
        case 9:
            mult = Random::get<int>(380, 480);
            break;
        case 10:
            mult = Random::get<int>(500, 600);
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            mult = Random::get<int>(1200, 1600);
            break;
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
            mult = Random::get<int>(2400, 3000);
            break;
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            mult = Random::get<int>(5500, 8000);
            break;
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
            mult = Random::get<int>(10000, 14000);
            break;
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
            mult = Random::get<int>(16000, 20000);
            break;
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
            mult = Random::get<int>(22000, 30000);
            break;
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
            mult = Random::get<int>(50000, 70000);
            break;
        case 46:
        case 47:
        case 48:
        case 49:
        case 50:
            mult = Random::get<int>(95000, 140000);
            break;
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
            mult = Random::get<int>(180000, 250000);
            break;
        case 56:
        case 57:
        case 58:
        case 59:
        case 60:
            mult = Random::get<int>(400000, 480000);
            break;
        case 61:
        case 62:
        case 63:
        case 64:
        case 65:
            mult = Random::get<int>(700000, 900000);
            break;
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
            mult = Random::get<int>(1400000, 1600000);
            break;
        case 71:
        case 72:
        case 73:
        case 74:
        case 75:
            mult = Random::get<int>(2200000, 2500000);
            break;
        case 76:
        case 77:
        case 78:
        case 79:
        case 80:
            mult = Random::get<int>(3000000, 3500000);
            break;
        case 81:
        case 82:
        case 83:
        case 84:
        case 85:
            mult = Random::get<int>(4250000, 4750000);
            break;
        case 86:
        case 87:
        case 88:
        case 89:
        case 90:
            mult = Random::get<int>(6500000, 8500000);
            break;
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
            mult = Random::get<int>(15000000, 18000000);
            break;
        case 96:
        case 97:
        case 98:
        case 99:
        case 100:
            mult = Random::get<int>(22000000, 30000000);
            break;
        case 101:
            mult = Random::get<int>(32000000, 40000000);
            break;
        case 102:
            mult = Random::get<int>(42000000, 55000000);
            break;
        case 103:
            mult = Random::get<int>(80000000, 95000000);
            break;
        case 104:
            mult = Random::get<int>(150000000, 200000000);
            break;
        case 105:
            mult = Random::get<int>(220000000, 250000000);
            break;
        case 106:
        case 107:
        case 108:
        case 109:
        case 110:
            mult = Random::get<int>(500000000, 750000000);
            break;
        case 111:
        case 112:
        case 113:
        case 114:
        case 115:
        case 116:
        case 117:
        case 118:
        case 119:
        case 120:
            mult = Random::get<int>(800000000, 900000000);
            break;
        default:
            if (GET_LEVEL(mob) >= 150) {
                mult = Random::get<int>(1500000000, 2000000000);
            } else {
                mult = Random::get<int>(1250000000, 1500000000);
            }
            break;
    }

    mob->setBaseStat("last_played", time(nullptr));
    bool autoset = mob->getBaseStat<int64_t>("health") <= 1;
    if(autoset) {
        for(auto c : {"health", "ki", "stamina"}) {
            int64_t base = GET_LEVEL(mob) * mult;
            if (GET_LEVEL(mob) > 140) {
                base *= 8;
            } else if (GET_LEVEL(mob) > 130) {
                base *= 6;
            } else if (GET_LEVEL(mob) > 120) {
                base *= 3;
            } else if (GET_LEVEL(mob) > 110) {
                base *= 2;
            }
            mob->setBaseStat(c, base);
        }
    }

    if (GET_MOB_VNUM(mob) == 2245) {
        for(auto c : {"health", "ki", "stamina"}) {
            mob->setBaseStat(c, Random::get<int>(1, 4));
        }
    }

    int base = 0;
    switch (GET_LEVEL(mob)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            base = Random::get<int>(80, 120);
            break;
        case 6:
            base = Random::get<int>(200, 280);
            break;
        case 7:
            base = Random::get<int>(250, 350);
            break;
        case 8:
            base = Random::get<int>(275, 375);
            break;
        case 9:
            base = Random::get<int>(300, 400);
            break;
        case 10:
            base = Random::get<int>(325, 450);
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            base = Random::get<int>(500, 700);
            break;
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
            base = Random::get<int>(700, 1000);
            break;
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            base = Random::get<int>(1000, 1200);
            break;
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
            base = Random::get<int>(1200, 1400);
            break;
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
            base = Random::get<int>(1400, 1600);
            break;
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
            base = Random::get<int>(1600, 1800);
            break;
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
            base = Random::get<int>(1800, 2000);
            break;
        case 46:
        case 47:
        case 48:
        case 49:
        case 50:
            base = Random::get<int>(2000, 2200);
            break;
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
            base = Random::get<int>(2200, 2500);
            break;
        case 56:
        case 57:
        case 58:
        case 59:
        case 60:
            base = Random::get<int>(2500, 2800);
            break;
        case 61:
        case 62:
        case 63:
        case 64:
        case 65:
            base = Random::get<int>(2800, 3000);
            break;
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
            base = Random::get<int>(3000, 3200);
            break;
        case 71:
        case 72:
        case 73:
        case 74:
        case 75:
            base = Random::get<int>(3200, 3500);
            break;
        case 76:
        case 77:
        case 78:
        case 79:
            base = Random::get<int>(3500, 3800);
            break;
        case 80:
        case 81:
        case 82:
        case 83:
        case 84:
        case 85:
            base = Random::get<int>(4000, 4500);
            break;
        case 86:
        case 87:
        case 88:
        case 89:
        case 90:
            base = Random::get<int>(4500, 5500);
            break;
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
            base = Random::get<int>(5500, 7000);
            break;
        case 96:
        case 97:
        case 98:
        case 99:
            base = Random::get<int>(8000, 10000);
            break;
        case 100:
            base = Random::get<int>(10000, 15000);
            break;
        case 101:
            base = Random::get<int>(15000, 25000);
            break;
        case 102:
            base = Random::get<int>(35000, 40000);
            break;
        case 103:
            base = Random::get<int>(40000, 50000);
            break;
        case 104:
            base = Random::get<int>(60000, 80000);
            break;
        case 105:
            base = Random::get<int>(80000, 100000);
            break;
        default:
            base = Random::get<int>(130000, 180000);
            break;
    }

    auto money = GET_GOLD(mob);
    if (money <= 0 && !MOB_FLAGGED(mob, MOB_DUMMY)) {
        if (GET_LEVEL(mob) < 4) {
            money = GET_LEVEL(mob) * Random::get<int>(1, 2);
        } else if (GET_LEVEL(mob) < 10) {
            money = (GET_LEVEL(mob) * Random::get<int>(1, 2)) - 1;
        } else if (GET_LEVEL(mob) < 20) {
            money = (GET_LEVEL(mob) * Random::get<int>(1, 3)) - 2;
        } else if (GET_LEVEL(mob) < 30) {
            money = (GET_LEVEL(mob) * Random::get<int>(1, 3)) - 4;
        } else if (GET_LEVEL(mob) < 40) {
            money = (GET_LEVEL(mob) * Random::get<int>(1, 3)) - 6;
        } else if (GET_LEVEL(mob) < 50) {
            money = (GET_LEVEL(mob) * Random::get<int>(2, 3)) - 25;
        } else if (GET_LEVEL(mob) < 60) {
            money = (GET_LEVEL(mob) * Random::get<int>(2, 3)) - 40;
        } else if (GET_LEVEL(mob) < 70) {
            money = (GET_LEVEL(mob) * Random::get<int>(2, 3)) - 50;
        } else if (GET_LEVEL(mob) < 80) {
            money = (GET_LEVEL(mob) * Random::get<int>(2, 4)) - 60;
        } else if (GET_LEVEL(mob) < 90) {
            money = (GET_LEVEL(mob) * Random::get<int>(2, 4)) - 70;
        } else {
            money = (GET_LEVEL(mob) * Random::get<int>(3, 4)) - 85;
        }
        if (!IS_HUMANOID(mob)) {
            money = GET_GOLD(mob) * 0.5;
            if (GET_GOLD(mob) <= 0)
                money = 1;
        }
        mob->setBaseStat("money_carried", money);
    }


    if (GET_EXP(mob) <= 0 && !MOB_FLAGGED(mob, MOB_DUMMY)) {
        int64_t mexp = GET_LEVEL(mob) * base;
        mexp = mexp * .9;
        mexp += GET_LEVEL(mob) / 2;
        mexp += GET_LEVEL(mob) / 3;
        if (IS_DRAGON(mob)) {
            mexp *= 1.4;
        } else if (IS_ANDROID(mob)) {
            mexp *= 1.25;
        } else if (IS_SAIYAN(mob)) {
            mexp *= 1.1;
        } else if (IS_BIO(mob)) {
            mexp *= 1.2;
        } else if (IS_MAJIN(mob)) {
            mexp *= 1.25;
        } else if (IS_DEMON(mob)) {
            mexp *= 1.1;
        }
        if (GET_CLASS(mob) == Sensei::commoner && IS_HUMANOID(mob) && !IS_DRAGON(mob)) {
            if (!IS_ANDROID(mob) && !IS_SAIYAN(mob) && !IS_BIO(mob) && !IS_MAJIN(mob)) {
                mexp *= 0.75;
            }
        }

        if (GET_LEVEL(mob) > 90) {
            mexp = mexp * .7;
        } else if (GET_LEVEL(mob) > 80) {
            mexp = mexp * .75;
        } else if (GET_LEVEL(mob) > 70) {
            mexp = mexp * .8;
        } else if (GET_LEVEL(mob) > 60) {
            mexp = mexp * .85;
        } else if (GET_LEVEL(mob) > 40) {
            mexp = mexp * .9;
        } else if (GET_LEVEL(mob) > 30) {
            mexp = mexp * .95;
        }

        if (GET_EXP(mob) > 20000000) {
            mexp = 20000000;
        }
        mob->setExperience(mexp);
    }

    mob->setAge(birth_age(mob));
    mob->time.created = mob->time.logon = time(nullptr); /* why not */
    mob->time.played = 0.0;
    mob->time.logon = time(nullptr);
    for(const auto& i : {0, 1, 2, 3}) mob->limb_condition[i] = 100;

    assign_triggers(mob, MOB_TRIGGER);
    racial_body_parts(mob);

    if (GET_MOB_VNUM(mob) >= 800 && GET_MOB_VNUM(mob) <= 805) {
        number_of_assassins += 1;
    }

    return mob;
}



/* create an object, and add it to the object list */
Object *create_obj() {
    auto sh = createEntity<Object>();;
    sh->activate();
    assign_triggers(sh.get(), OBJ_TRIGGER);
    return sh.get();
}


/* create a new object from a prototype */
Object *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
    auto i = nr;
    int j;

    auto proto = obj_proto.find(i);

    if (proto == obj_proto.end()) {
        basic_mud_log("Object (%c) %d does not exist in database.", type == VIRTUAL ? 'V' : 'R', nr);
        return (nullptr);
    }
    auto sh = createEntity<Object>();
    auto obj = sh.get();
    // the operator= will copy the prototype data into the new object.
    *obj = *(proto->second);

    if (nr == 65) {
        SET_OBJ_VAL(obj, VAL_BED_HTANK_CHARGE, 20);
    }
    if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
        if (GET_OBJ_VAL(obj, VAL_FOOD_MAXFOODVAL) == 0) {
            SET_OBJ_VAL(obj, VAL_FOOD_MAXFOODVAL, GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL));
        }
        obj->setBaseStat("foob", GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL));
    }

    obj->activate();

    return (obj);
}


constexpr int ZO_DEAD = 999;

static std::deque<zone_vnum> zonesToUpdate;

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(uint64_t heartPulse, double deltaTime) {

    for (auto &[vn, z] : zone_table) {
        z->age += deltaTime;
        auto secs = (z->lifespan * 60.0);
        if(z->age < secs) continue;

        bool doReset = false;
        switch(z->reset_mode) {
            case 0:
                // Never reset.
            break;
            case 1:
                // reset only if zone is empty.
                if(z->playersInZone.empty()) doReset = true;
            break;
            case 2:
                // Always reset.
                doReset = true;
            break;
            default:
                // This shouldn't happen.
                    break;
        }
        if(doReset) {
            zonesToUpdate.emplace_back(vn);

            z->age -= secs;

            break;
        }
    }

    // Stagger zone updates so they don't all happen in the exact same heartbeat.
    while(!zonesToUpdate.empty()) {
        auto vn = zonesToUpdate.front();
        auto& z = zone_table.at(vn);
        reset_zone(vn);
        mudlog(CMP, ADMLVL_GOD, false, "Auto zone reset: %s (Zone %d)",
               z->name.c_str(), vn);
        zonesToUpdate.pop_front();
        break;
    }

}

void reset_zone(zone_vnum vn) {
    auto& z = zone_table.at(vn);
    z->reset();
}


void repairRoomDamage(uint64_t heartPulse, double deltaTime) {
    auto subs = roomSubscriptions.all("repairRoomDamage");
    for(auto room : dbat::util::filter_raw(subs)) {

        if(auto dmg = room->getDamage(); dmg > 0) {
            int toRepair = 0;
            if(dmg >= 100) toRepair = Random::get<int>(5, 10);
            else if(dmg >= 10) toRepair = Random::get<int>(1, 10);
            else if(dmg > 1) toRepair = Random::get<int>(1, dmg);
            else toRepair = 1;
            room->modDamage(-toRepair);
            room->sendText("The area gets rebuilt a little.\r\n");
        }
    }
}


/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr) {
    return zone_table.at(zone_nr)->playersInZone.empty();
}


/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error) {
    char buf[MAX_STRING_LENGTH], tmp[520];
    char *point;
    int done = 0, length = 0, templength;

    *buf = *tmp = '\0';

    do {
        if (!fgets(tmp, 512, fl)) {
            basic_mud_log("SYSERR: fread_string: format error at string (pos %ld): %s at or near %s",
                ftell(fl), feof(fl) ? "EOF" : ferror(fl) ? "read error" : "unknown error", error);
            exit(1);
        }
        /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
        /* now only removes trailing ~'s -- Welcor */
        for (point = tmp; *point && *point != '\r' && *point != '\n'; point++);
        if (point > tmp && point[-1] == '~') {
            *(--point) = '\0';
            done = 1;
        } else {
            *point = '\r';
            *(++point) = '\n';
            *(++point) = '\0';
        }

        templength = point - tmp;

        if (length + templength >= MAX_STRING_LENGTH) {
            basic_mud_log("SYSERR: fread_string: string too large (db.c)");
            basic_mud_log("%s", error);
            exit(1);
        } else {
            strcat(buf + length, tmp);    /* strcat: OK (size checked above) */
            length += templength;
        }
    } while (!done);

    /* allocate space for the new string and copy it */
    return (strlen(buf) ? strdup(buf) : nullptr);
}


/*
 * Steps:
 *   1: Read contents of a text file.
 *   2: Make sure no one is using the pointer in paging.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * strdup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 *
 * If someone is reading a global copy we're trying to
 * replace, give everybody using it a different copy so
 * as to avoid special cases.

 clear some of the the working variables of a char */
void reset_char(Character *ch) {
    int i;

    ch->followers.clear();
    ch->master = nullptr;
    FIGHTING(ch) = nullptr;
    ch->position = Position::Standing;
    ch->time.logon = time(nullptr);

}


/*
 * Called during character creation after picking character class
 * (and then never again for that character).
 */
void init_char(Character *ch) {
    int i;

    ch->setBaseStat("money_carried", 1500);
    ch->setBaseStat("practices", 600);


    /* If this is our first player make him LVL_IMPL. */
    if (players.size() == 0) {
        admin_set(ch, ADMLVL_IMPL);
    }

    /*ch->time.birth = time(0) - birth_age(ch);*/
    ch->time.logon = ch->time.created = time(nullptr);
    ch->time.played = 0.0;

    set_height_and_weight_by_race(ch);

    for (i = 1; i < SKILL_TABLE_SIZE; i++) {
        if (GET_ADMLEVEL(ch) >= ADMLVL_IMPL)
            SET_SKILL(ch, i, 100);
    }

    for (i = 0; i < 3; i++)
        ch->limbs[i] = 100;

    for (i = 0; i < 3; i++)
        GET_COND(ch, i) = (GET_ADMLEVEL(ch) == ADMLVL_IMPL ? -1 : 24);

    ch->registeredLocations.erase("load_room");

    do_start(ch);
}

/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum) {
    return Room::registry.contains(vnum) ? vnum : NOWHERE;
}


/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum) {
    return mob_proto.contains(vnum) ? vnum : NOBODY;
}


/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum) {
    return obj_proto.contains(vnum) ? vnum : NOTHING;
}

/* returns the real number of the room with given virtual number */
zone_rnum real_zone(zone_vnum vnum) {
    return zone_table.contains(vnum) ? vnum : NOWHERE;
}


/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */


/* This procedure removes the '\r\n' from a string so that it may be
   saved to a file.  Use it only on buffers, not on the orginal
   strings. */

void strip_string(char *buffer) {
    char *ptr, *str;

    ptr = buffer;
    str = ptr;

    while ((*str = *ptr)) {
        str++;
        ptr++;
        if (*ptr == '\r')
            ptr++;
    }
}

/* External variables from config.c */

void load_default_config() {
    /****************************************************************************/
    /** This function is called only once, at boot-time.                       **/
    /** - We assume config_info is empty                          -- Welcor    **/
    /****************************************************************************/
    /****************************************************************************/
    /** Game play options.                                                     **/
    /****************************************************************************/
    CONFIG_PK_ALLOWED = pk_allowed;
    CONFIG_PT_ALLOWED = pt_allowed;
    CONFIG_LEVEL_CAN_SHOUT = level_can_shout;
    CONFIG_HOLLER_MOVE_COST = holler_move_cost;
    CONFIG_TUNNEL_SIZE = tunnel_size;
    CONFIG_MAX_EXP_GAIN = max_exp_gain;
    CONFIG_MAX_EXP_LOSS = max_exp_loss;
    CONFIG_MAX_NPC_CORPSE_TIME = max_npc_corpse_time;
    CONFIG_MAX_PC_CORPSE_TIME = max_pc_corpse_time;
    CONFIG_IDLE_VOID = idle_void;
    CONFIG_IDLE_RENT_TIME = idle_rent_time;
    CONFIG_IDLE_MAX_LEVEL = idle_max_level;
    CONFIG_DTS_ARE_DUMPS = dts_are_dumps;
    CONFIG_LOAD_INVENTORY = load_into_inventory;
    CONFIG_OK = strdup(OK);
    CONFIG_NOPERSON = strdup(NOPERSON);
    CONFIG_NOEFFECT = strdup(NOEFFECT);
    CONFIG_TRACK_T_DOORS = track_through_doors;
    CONFIG_LEVEL_CAP = level_cap;
    CONFIG_STACK_MOBS = show_mob_stacking;
    CONFIG_STACK_OBJS = show_obj_stacking;
    CONFIG_MOB_FIGHTING = mob_fighting;
    CONFIG_DISP_CLOSED_DOORS = disp_closed_doors;
    CONFIG_REROLL_PLAYER_CREATION = reroll_status;
    CONFIG_INITIAL_POINTS_POOL = initial_points;
    CONFIG_ENABLE_COMPRESSION = enable_compression;
    CONFIG_ENABLE_LANGUAGES = enable_languages;
    CONFIG_ALL_ITEMS_UNIQUE = all_items_unique;
    CONFIG_EXP_MULTIPLIER = exp_multiplier;
    /****************************************************************************/
    /** Rent / crashsave options.                                              **/
    /****************************************************************************/
    CONFIG_FREE_RENT = free_rent;
    CONFIG_MAX_OBJ_SAVE = max_obj_save;
    CONFIG_MIN_RENT_COST = min_rent_cost;
    CONFIG_AUTO_SAVE = auto_save;
    CONFIG_AUTOSAVE_TIME = autosave_time;
    CONFIG_CRASH_TIMEOUT = crash_file_timeout;
    CONFIG_RENT_TIMEOUT = rent_file_timeout;

    /****************************************************************************/
    /** Room numbers.                                                          **/
    /****************************************************************************/
    CONFIG_MORTAL_START = mortal_start_room;
    CONFIG_IMMORTAL_START = immort_start_room;
    CONFIG_FROZEN_START = frozen_start_room;
    CONFIG_DON_ROOM_1 = donation_room_1;
    CONFIG_DON_ROOM_2 = donation_room_2;
    CONFIG_DON_ROOM_3 = donation_room_3;

    /****************************************************************************/
    /** Game operation options.                                                **/
    /****************************************************************************/
    CONFIG_DFLT_PORT = DFLT_PORT;

    if (DFLT_IP)
        CONFIG_DFLT_IP = strdup(DFLT_IP);
    else
        CONFIG_DFLT_IP = nullptr;

    CONFIG_DFLT_DIR = strdup("data");

    if (LOGNAME)
        CONFIG_LOGNAME = strdup(LOGNAME);
    else
        CONFIG_LOGNAME = nullptr;

    CONFIG_MAX_PLAYING = max_playing;
    CONFIG_MAX_FILESIZE = max_filesize;
    CONFIG_MAX_BAD_PWS = max_bad_pws;
    CONFIG_SITEOK_ALL = siteok_everyone;
    CONFIG_NS_IS_SLOW = nameserver_is_slow;
    CONFIG_NEW_SOCIALS = use_new_socials;
    CONFIG_OLC_SAVE = auto_save_olc;
    CONFIG_MENU = strdup(MENU);
    CONFIG_WELC_MESSG = strdup(WELC_MESSG);
    CONFIG_START_MESSG = strdup(START_MESSG);
    CONFIG_IMC_ENABLED = imc_is_enabled;
    CONFIG_EXP_MULTIPLIER = 1.0;

    /****************************************************************************/
    /** Autowiz options.                                                       **/
    /****************************************************************************/
    CONFIG_USE_AUTOWIZ = use_autowiz;
    CONFIG_MIN_WIZLIST_LEV = min_wizlist_lev;

}

void load_config() {
    load_default_config();
}
