#include "comm.h"
#include "utils.h"
#include "dg_scripts.h"
#include "constants.h"
#include "genolc.h"
#include "dg_event.h"
#include "maputils.h"
#include <filesystem>
#include <memory>
#include <iostream>
#include <vector>
#include <fmt/format.h>
#include <tuple>
#include "SQLiteCpp/SQLiteCpp.h"
#include <boost/algorithm/string.hpp>
#include <fstream>
#include "class.h"
#include "races.h"
#include "players.h"
#include "spells.h"
#include "kaizermud/utils.h"
#include "dbatk/Zone.h"
#include "dbatk/DgScripts.h"
#include "nlohmann/json.hpp"
#include "dbatk/Core.h"
#include "kaizermud/Api.h"
#include "kaizermud/Prototypes.h"
#include "kaizermud/Database.h"
#include "kaizermud/Components.h"
#include "spdlog/spdlog.h"


void dump_zones() {

    for(const auto& [id, zone] : zone_table) {
        auto &z = dbat::zones[id];
        z.id = id;
        z.name = kaizer::intern(std::string(zone.name));
        z.builders = kaizer::intern(std::string(zone.builders));
        z.lifespan = zone.lifespan;
        z.age = zone.age;
        z.reset_mode = zone.reset_mode;
        z.top = zone.top;
        z.bot = zone.bot;

        for(const auto &c : zone.cmd) {
            auto &nc = z.cmds.emplace_back();
            nc.command = c.command;
            nc.if_flag = c.if_flag;
            nc.arg1 = c.arg1;
            nc.arg2 = c.arg2;
            nc.arg3 = c.arg3;
            nc.arg4 = c.arg4;
            nc.arg5 = c.arg5;
            if(c.sarg1) nc.sarg1 = c.sarg1;
            if(c.sarg2) nc.sarg2 = c.sarg2;
        }

        for(auto i = 0; i < NUM_ZONE_FLAGS; i++) {
            if(IS_SET_AR(zone.zone_flags, i)) {
                z.flags[i] = true;
            }
        }
    }
}

void dump_scripts() {

    for(const auto& [id, trig] : trig_index) {
        auto t = std::make_shared<dbat::DgScriptPrototype>();
        auto p = trig.proto;
        t->id = id;
        dbat::dgScripts[id] = t;
        t->name = p->name;
        t->scriptType = p->attach_type;

        for(auto i = 0; i < 22; i++) {
            if(IS_SET(p->trigger_type, i)) {
                t->triggerType[i] = true;
            }
        }

        std::string script_body;
        for(auto cmd = p->cmdlist; cmd; cmd = cmd->next) {
            t->lines.emplace_back(cmd->cmd);
        }

        if(p->arglist) t->arglist = p->arglist;
    }
}


static bool include_room(room_data *r) {
    return true;
    if(IS_SET_AR(r->room_flags, ROOM_SPACE)) {
        return false;
    }
    if(IS_SET_AR(r->room_flags, ROOM_NEBULA)) {
        return false;
    }
    return true;
}

static bool include_room(room_vnum vn) {
    auto found = world.find(vn);
    if(found == world.end()) {
        return false;
    }
    return include_room(&found->second);
}

static std::string cleanString(const std::string &txt) {
    // Given a txt, we want to remove all trailing whitespace/newlines and also the sequence "@n".
    // Then, if the trimmed text contains a @ at all, we want to append a "@n" at the end.
    std::string str = txt;
    for(auto i = 0; i < 3; i++) {
        boost::trim_right(str);
        if(str.ends_with("@n")) {
            str = str.substr(0, str.size() - 2);
        }
    }

    // if '@' is still present in the text, append "@n"
    if (str.find('@') != std::string::npos) {
        str.append("@n");
    }

    return str;
}

static std::string sectorTypes[] = {
        "inside",
        "city",
        "plain",
        "forest",
        "hills",
        "mountains",
        "shallows",
        "water",
        "sky",
        "underwater",
        "shop",
        "important",
        "desert",
        "space",
        "lava",
};

void dump_rooms() {
    std::bitset<32> bits;
    bits[0] = true;
    bits[3] = true;

    for(auto& [id, room] : world) {
        if(!include_room(&room)) continue;

        auto [obj, err] = kaizer::createEntity(id);
        if(err) {
            std::cerr << "Error creating room object: " << err.value() << std::endl;
            exit(1);
        }
        nlohmann::json j, asp;
        j["Types"] = bits.to_ulong();
        j["Name"] = processColors(room.name, false, nullptr);
        j["ShortDescription"] = std::string(room.name);
        if(room.look_description && strlen(room.look_description)) {
            j["LookDescription"] = cleanString(room.look_description);
        }
        asp["terrain"] = sectorTypes[room.sector_type];
        j["aspects"] = asp;

        kaizer::deserializeObject(obj, j);

    }
    // TODO: Room Flags
}



void dump_exits() {
    std::bitset<32> bits;
    bits[0] = true;
    bits[4] = true;

    for(const auto& [id, room] : world) {
        for(auto i = 0; i < NUM_OF_DIRS; i++) {
            auto ex = room.dir_option[i];
            if(ex) {
                if(!include_room(ex->to_room)) continue;
                auto destIT = kaizer::entities.find(ex->to_room);
                if(destIT == kaizer::entities.end()) continue;
                auto exIT = kaizer::entities.find(id);
                if(exIT == kaizer::entities.end()) continue;
                auto [obj, err] = kaizer::createEntity();
                if(err) {
                    std::cerr << "Error creating exit object: " << err.value() << std::endl;
                    exit(1);
                }

                nlohmann::json j, exdata;
                j["Types"] = bits.to_ulong();
                j["Name"] = dirs[i];
                if(ex->general_description) {
                    j["LookDescription"] = cleanString(ex->general_description);
                }
                if(ex->keyword) {
                    j["ShortDescription"] = cleanString(ex->keyword);
                }
                exdata["destination"] = ex->to_room;
                exdata["location"] = id;
                j["Exit"] = exdata;
                if(ex->key != NOTHING) j["integers"]["key"] = ex->key;

                kaizer::deserializeObject(obj, j);
            }
        }
    }
}

nlohmann::json dump_item(obj_data *obj, bool asPrototype) {
    std::bitset<32> bits;
    bits[0] = true;
    bits[1] = true;

    nlohmann::json j;
    j["Types"] = bits.to_ulong();

    nlohmann::json strings;
    if (obj->name && strlen(obj->name)) j["Name"] = cleanString(obj->name);
    if (obj->short_description && strlen(obj->short_description)) j["ShortDescription"] = cleanString(obj->short_description);
    if (obj->look_description && strlen(obj->look_description)) j["LookDescription"] = cleanString(obj->look_description);
    if (obj->room_description && strlen(obj->room_description)) j["RoomDescription"] = cleanString(obj->room_description);

    nlohmann::json integers;
    integers["type_flag"] = obj->type_flag;
    integers["level"] = obj->level;
    integers["weight"] = obj->weight;
    integers["cost"] = obj->cost;
    integers["cost_per_day"] = obj->cost_per_day;
    integers["size"] = obj->size;
    j["integers"] = integers;

    std::vector<std::pair<std::string, std::string>> extraDescriptions;
    for (auto ex = obj->ex_description; ex; ex = ex->next) {
        if (!(ex->keyword && ex->description)) continue;
        extraDescriptions.emplace_back(cleanString(std::string(ex->keyword)), cleanString(std::string(ex->description)));
    }
    if (!extraDescriptions.empty()) j["extra_descriptions"] = extraDescriptions;

    for (auto t = obj->proto_script; t; t = t->next) {
        j["dgscripts"].push_back(t->vnum);
    }

    for (auto i = 0; i < NUM_ITEM_WEARS; i++) {
        if (!IS_SET_AR(obj->wear_flags, i)) {
            continue;
        }
    }

    for (auto i = 0; i < NUM_ITEM_FLAGS; i++) {
        if (!IS_SET_AR(obj->extra_flags, i)) {
            continue;
        }
    }

    for (auto i = 0; i < NUM_AFF_FLAGS; i++) {
        if (!IS_SET_AR(obj->bitvector, i)) {
            continue;
        }
    }

    for (auto i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
        if (obj->value[i] == 0) continue;
    }

    for (auto i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location == APPLY_NONE) {
            continue;
        }
    }
    return j;
}

void dump_items() {
    for(auto [vnum, obj] : obj_proto) {
        auto key = fmt::format("item:{}", vnum);
        auto dump = dump_item(&obj, true);
        kaizer::registerPrototype(key, dump);
    }
}


nlohmann::json dump_character(char_data *ch, bool asPrototype) {
    std::bitset<32> bits;
    bits[0] = true;
    bits[2] = true;
    auto is_npc = IS_NPC(ch);
    if(IS_NPC(ch)) {
        bits[16] = true;
    } else {
        bits[17] = true;
    }

    nlohmann::json j;
    j["Types"] = bits.to_ulong();

    if (ch->name && strlen(ch->name)) j["Name"] = cleanString(ch->name);
    if (ch->short_description && strlen(ch->short_description)) j["ShortDescription"] = cleanString(ch->short_description);
    if (ch->look_description && strlen(ch->look_description)) j["LookDescription"] = cleanString(ch->look_description);
    if (ch->room_description && strlen(ch->room_description)) j["RoomDescription"] = cleanString(ch->room_description);

    nlohmann::json character;
    switch(ch->race->getID()) {
        case race::spirit:
            character["race"] = 0;
            break;
        default:
            character["race"] = ch->race->getID() + 1;
            break;
    }

    character["sex"] = ch->sex;
    switch(ch->chclass->getID()) {
        case sensei::commoner:
            character["sensei"] = 0;
            break;
        default:
            character["sensei"] = ch->chclass->getID() + 1;
            break;
    }

    nlohmann::json integers;
    integers["level"] = ch->level;
    integers["alignment"] = ch->alignment;
    integers["armor"] = ch->armor;
    integers["gold"] = ch->gold;
    integers["damage_mod"] = ch->damage_mod;
    integers["size"] = ch->size;
    integers["race_level"] = ch->race_level;
    integers["level_adj"] = ch->level_adj;



    nlohmann::json stats;
    stats["strength"] = ch->real_abils.str;
    stats["intelligence"] = ch->real_abils.intel;
    stats["wisdom"] = ch->real_abils.wis;
    stats["dexterity"] = ch->real_abils.dex;
    stats["constitution"] = ch->real_abils.con;
    stats["speed"] = ch->real_abils.cha;
    stats["powerlevel"] = ch->basepl;
    stats["ki"] = ch->baseki;
    stats["stamina"] = ch->basest;

    j["stats"] = stats;
    j["integers"] = integers;
    j["Character"] = character;

    for (auto t = ch->proto_script; t; t = t->next) {
        j["dgscripts"].push_back(t->vnum);
    }

    if(is_npc) for(auto i = 0; i < NUM_MOB_FLAGS; i++) {
            if(!IS_SET_AR(ch->act, i)) {
                continue;
            }
        }

    for(auto i = 0; i < NUM_AFF_FLAGS; i++) {
        if(!IS_SET_AR(ch->affected_by, i)) {
            continue;
        }
    }

    return j;

}

void dump_npc() {
    for (auto &[vnum, npc]: mob_proto) {
        auto key = fmt::format("npc:{}", vnum);
        auto data = dump_character(&npc, true);
        kaizer::registerPrototype(key, data);
    }
}

static std::set<long> player_ids;

std::optional<nlohmann::json> dump_player(const char *name) {

    auto *ch = new char_data();
    ch->player_specials = new player_special_data();
    if(load_char(name, ch) < 0) {
        log("Error loading player %s", name);
        free_char(ch);
        return {};
    }
    if(player_ids.count(ch->idnum)) {
        log("Duplicate player %s", name);
        free_char(ch);
        return {};
    }
    player_ids.insert(ch->idnum);
    log("Dumping player %d: %s", ch->idnum, ch->name);

    auto j = dump_character(ch, false);

    for(auto i = 0; i < NUM_PLR_FLAGS; i++) {
        if(!IS_SET_AR(ch->act, i)) {
            continue;
        }
    }

    for(auto i = 0; i < SKILL_TABLE_SIZE; i++) {
        if(GET_SKILL(ch, i) || GET_SKILL_BONUS(ch, i) || GET_SKILL_PERF(ch, i)) {

        }
    }

    for (auto i = 0; i < 52; i++) {
        if(GET_BONUS(ch, i)) {

        };
    }

    free_char(ch);
    return j;

}

void dump_accounts() {

    auto ufolder = std::filesystem::path("user");
    for(auto &p : std::filesystem::recursive_directory_iterator(ufolder)) {
        if(!p.path().string().ends_with(".usr")) continue;
        std::ifstream f(p.path());
        auto [ent, err] = kaizer::createEntity();
        if(err) {
            log("Error creating entity for %s", p.path().string().c_str());
            exit(1);
        }
        auto &acc = kaizer::registry.get_or_emplace<kaizer::components::Account>(ent);


        std::string password;
        long rpp, supervisor_level, character_slots;

        std::getline(f, acc.username);
        std::getline(f, acc.email);
        std::getline(f, password);
        acc.setPassword(password);

        f >> character_slots;
        f >> rpp;

        auto &i = kaizer::registry.get_or_emplace<kaizer::components::Integers>(ent);
        i.data["rpp"] = rpp;
        i.data["character_slots"] = character_slots;

        std::vector<std::string> characters;
        for(int i = 0; i < 5; i++) {
            std::string line;
            std::getline(f, line);
            if(line == "Empty") continue;
            if(line.empty()) continue;
            characters.push_back(line);
        }

        f >> acc.level;
        int b;
        f >> b;
        f >> b;


        for(auto &c : characters) {
            auto player_data = dump_player(c.c_str());
            if(!player_data.has_value()) {
                log("Error dumping player %s", c.c_str());
                continue;
            }
            auto [pc, err] = kaizer::createEntity();
            if(err) {
                log("Error creating entity for %s", c.c_str());
                exit(1);
            }
            kaizer::deserializeObject(pc, player_data.value());
            acc.characters.insert(kaizer::getID(pc));
        }
    }
}

void dump_skills() {

    for(auto i = 0; i < SKILL_TABLE_SIZE; i++) {
        if(!spell_info[i].name) continue;
        if(!strcasecmp(spell_info[i].name, "!UNUSED!")) continue;

    }

}


void dump_db() {

    dbat::registerResources();

    // First, convert zones.
    dump_zones();

    // Then scripts..
    dump_scripts();

    // Now we can dump rooms..
    dump_rooms();

    dump_exits();

    // dump prototypes...
    dump_items();
    dump_npc();

    dump_accounts();

    kaizer::saveSnapShot();

}

int main(int argc, char **argv)
{
    int pos = 1;
    const char *dir;

#ifdef MEMORY_DEBUG
    zmalloc_init();
#endif

#if CIRCLE_GNU_LIBC_MEMORY_TRACK
    mtrace();	/* This must come before any use of malloc(). */
#endif

    /****************************************************************************/
    /** Load the game configuration.                                           **/
    /** We must load BEFORE we use any of the constants stored in constants.c. **/
    /** Otherwise, there will be no variables set to set the rest of the vars  **/
    /** to, which will mean trouble --> Mythran                                **/
    /****************************************************************************/
    CONFIG_CONFFILE = nullptr;
    while ((pos < argc) && (*(argv[pos]) == '-')) {
        if (*(argv[pos] + 1) == 'f') {
            if (*(argv[pos] + 2))
                CONFIG_CONFFILE = argv[pos] + 2;
            else if (++pos < argc)
                CONFIG_CONFFILE = argv[pos];
            else {
                puts("SYSERR: File name to read from expected after option -f.");
                exit(1);
            }
        }
        pos++;
    }
    pos = 1;

    if (!CONFIG_CONFFILE)
        CONFIG_CONFFILE = strdup(CONFIG_FILE);

    load_config();

    port = CONFIG_DFLT_PORT;
    dir = CONFIG_DFLT_DIR;

    while ((pos < argc) && (*(argv[pos]) == '-')) {
        switch (*(argv[pos] + 1)) {
            case 'f':
                if (! *(argv[pos] + 2))
                    ++pos;
                break;
            case 'o':
                if (*(argv[pos] + 2))
                    CONFIG_LOGNAME = argv[pos] + 2;
                else if (++pos < argc)
                    CONFIG_LOGNAME = argv[pos];
                else {
                    puts("SYSERR: File name to log to expected after option -o.");
                    exit(1);
                }
                break;
            case 'C': /* -C<socket number> - recover from copyover, this is the control socket */
                fCopyOver = true;
                mother_desc = atoi(argv[pos]+2);
                break;
            case 'd':
                if (*(argv[pos] + 2))
                    dir = argv[pos] + 2;
                else if (++pos < argc)
                    dir = argv[pos];
                else {
                    puts("SYSERR: Directory arg expected after option -d.");
                    exit(1);
                }
                break;
            case 'm':
                mini_mud = 1;
                no_rent_check = 1;
                puts("Running in minimized mode & with no rent check.");
                break;
            case 'c':
                scheck = 1;
                puts("Syntax check mode enabled.");
                break;
            case 'q':
                no_rent_check = 1;
                puts("Quick boot mode -- rent check supressed.");
                break;
            case 'r':
                circle_restrict = 1;
                puts("Restricting game -- no new players allowed.");
                break;
            case 's':
                no_specials = 1;
                puts("Suppressing assignment of special routines.");
                break;
            case 'x':
                xap_objs = 1;
                log("Loading player objects from secondary (ascii) files.");
                break;
            case 'h':
                /* From: Anil Mahajan <amahajan@proxicom.com> */
                printf("Usage: %s [-c] [-m] [-x] [-q] [-r] [-s] [-d pathname] [port #]\n"
                       "  -c             Enable syntax check mode.\n"
                       "  -d <directory> Specify library directory (defaults to 'lib').\n"
                       "  -f<file>       Use <file> for configuration.\n"
                       "  -h             Print this command line argument help.\n"
                       "  -m             Start in mini-MUD mode.\n"
                       "  -o <file>      Write log to <file> instead of stderr.\n"
                       "  -q             Quick boot (doesn't scan rent for object limits)\n"
                       "  -r             Restrict MUD -- no new players allowed.\n"
                       "  -s             Suppress special procedure assignments.\n"
                       " Note:         These arguments are 'CaSe SeNsItIvE!!!'\n"
                       "  -x             Load using secondary (ascii) files.\n",
                       argv[0]
                );
                exit(0);
            default:
                printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1));
                break;
        }
        pos++;
    }

    if (pos < argc) {
        if (!isdigit(*argv[pos])) {
            printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
            exit(1);
        } else if ((port = atoi(argv[pos])) <= 1024) {
            printf("SYSERR: Illegal port number %d.\n", port);
            exit(1);
        }
    }

    /* All arguments have been parsed, try to open log file. */
    setup_log(CONFIG_LOGNAME, STDERR_FILENO);

    /*
     * Moved here to distinguish command line options and to show up
     * in the log if stderr is redirected to a file.
     */
    log("Using %s for configuration.", CONFIG_CONFFILE);
    log("%s", circlemud_version);
    log("%s", oasisolc_version);
    log("%s", DG_SCRIPT_VERSION);
    log("%s", ascii_pfiles_version);
    log("%s", CWG_VERSION);
    xap_objs = 1;
    if (chdir(dir) < 0) {
        perror("SYSERR: Fatal error changing to data directory");
        exit(1);
    }
    log("Using %s as data directory.", dir);

    init_lookup_table();
    event_init();
    boot_db();
    log("Database fully booted!");

    FILE *mapfile;
    int rowcounter, colcounter;
    int vnum_read;
    log("Loading Space Map. ");
    //Load the map vnums from a file into an array
    mapfile = fopen("../lib/surface.map", "r");

    for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++) {
        for (colcounter = 0; colcounter <= MAP_COLS; colcounter++) {
            fscanf(mapfile, "%d", &vnum_read);
            mapnums[rowcounter][colcounter] = real_room(vnum_read);
        }
    }
    fclose(mapfile);

    /* Load the toplist */
    topLoad();

    // BEGIN DUMP SEQUENCE
    log("Dumping db!");
    try {
        dump_db();
    } catch (const std::exception &e) {
        log("Exception thrown: %s", e.what());
    }
    log("Dumping db complete!");


    return 0;
}
