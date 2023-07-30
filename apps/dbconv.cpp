#include "dbat/comm.h"
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/constants.h"
#include "dbat/genolc.h"
#include "dbat/dg_event.h"
#include "dbat/maputils.h"
#include <filesystem>
#include <memory>
#include <iostream>
#include <vector>
#include <fmt/format.h>
#include <tuple>
#include "SQLiteCpp/SQLiteCpp.h"
#include <boost/algorithm/string.hpp>
#include <fstream>
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/players.h"
#include "dbat/spells.h"
#include "nlohmann/json.hpp"

std::unique_ptr<SQLite::Database> db;

void run_query(const std::string &q, bool display = false) {
    try {
        if(display) std::cout << "Running query: " << q << std::endl;
        db->exec(q);
    }
    catch (const std::exception& e) {
        std::cerr << "Error executing schema query: " << e.what() << std::endl;
        std::cerr << "Query was: " << q << std::endl;
        exit(1);
    }
}

void populate_char_array(const std::string &table_name, int size, const char* data[]) {

    SQLite::Statement q1(*db, "INSERT INTO " + table_name + " (id, name) VALUES (?,?);");

    for(int i = 0; i < size; ++i) {
        q1.bind(1, i);
        std::string name(data[i]);
        if(name.empty()) name = "!UNDEFINED!";
        q1.bind(2, name);
        q1.exec();
        q1.reset();
    }

}

std::vector<std::tuple<std::string,int,const char**>> basic_tables = {
        {"attack_names_comp", 14, attack_names_comp},
        {"attack_names", 14, attack_names},
        {"npc_personality", MAX_PERSONALITIES, npc_personality},
        {"alignments", NUM_ALIGNS, alignments},
        {"aura_types", 9, aura_types},
        {"armor_types", MAX_ARMOR_TYPES, armor_type},
        {"weapon_types", MAX_WEAPON_TYPES+1, weapon_type},
        {"crit_type", NUM_CRIT_TYPES, crit_type},
        {"dirs", NUM_OF_DIRS, dirs},
        {"eye_types", 12, eye_types},
        {"hairl_types", 5, hairl_types},
        {"FHA_types", 5, FHA_types},
        {"hairc_types", 14, hairc_types},
        {"hairs_types", 12, hairs_types},
        {"skin_types", 12, skin_types},
        {"zone_bits", NUM_ZONE_FLAGS, zone_bits},
        {"room_bits", NUM_ROOM_FLAGS, room_bits},
        {"exit_bits", NUM_EXIT_FLAGS, exit_bits},
        {"sector_types", NUM_ROOM_SECTORS, sector_types},
        {"genders", NUM_SEX, genders},
        {"position_types", NUM_POSITIONS, position_types},
        {"player_bits", NUM_PLR_FLAGS, player_bits},
        {"action_bits", NUM_MOB_FLAGS, action_bits},
        {"preference_bits", NUM_PRF_FLAGS, preference_bits},
        {"affected_bits", NUM_AFF_FLAGS, affected_bits},
        {"connected_types", NUM_CON_TYPES, connected_types},
        {"wear_where", NUM_WEARS, wear_where},
        {"equipment_types", NUM_WEARS, equipment_types},
        {"item_types", NUM_ITEM_TYPES, item_types},
        {"wear_bits", NUM_ITEM_WEARS, wear_bits},
        {"extra_bits", NUM_ITEM_FLAGS, extra_bits},
        {"apply_types", NUM_APPLIES, apply_types},
        {"container_bits", NUM_CONT_FLAGS, container_bits},
        {"drinks", NUM_LIQ_TYPES, drinks},
        {"material_names", NUM_MATERIALS, material_names},
        {"domains", 22, domains},
        {"schools", NUM_SCHOOLS, schools},
        {"history_types", NUM_HIST, history_types},
        {"drinknames", NUM_LIQ_TYPES, drinknames},
        {"color_liquid", NUM_LIQ_TYPES, color_liquid},
        {"fullness", NUM_FULLNESS, fullness},
        {"trig_types", NUM_MTRIG_TYPES, trig_types},
        {"otrig_types", NUM_OTRIG_TYPES, otrig_types},
        {"wtrig_types", NUM_WTRIG_TYPES, wtrig_types},
        {"size_names", NUM_SIZES, size_names},
        {"wield_names", NUM_WIELD_NAMES, wield_names},
        {"weekdays", NUM_WEEK_DAYS, weekdays},
        {"month_name", NUM_MONTHS, month_name},
        {"admin_level_names", ADMLVL_IMPL+1, admin_level_names},
        {"admin_flag_names", 19, admin_flag_names},
        {"spell_schools", NUM_SCHOOLS, spell_schools},
        {"cchoice_names", NUM_COLOR, cchoice_names},
        {"admin_flags", NUM_ADMFLAGS, admin_flags},
        {"list_bonus", 51, list_bonus}
};

void create_basic_tables() {

    for(const auto& [table_name, num, data] : basic_tables) {
        auto q = fmt::format("CREATE TABLE {} ("
                             "    id INTEGER PRIMARY KEY,"
                             "    name TEXT"
                             ");", table_name);
        run_query(q);
        populate_char_array(table_name, num, data);
    }
}


static std::vector<std::string> schema = {
        "CREATE TABLE senseis ("
        "    id INTEGER PRIMARY KEY,"
        "    name TEXT UNIQUE COLLATE NOCASE,"
        "    abbreviation TEXT,"
        "    style_name TEXT"
        ");",

        "CREATE TABLE races ("
        "    id INTEGER PRIMARY KEY,"
        "    name TEXT UNIQUE COLLATE NOCASE,"
        "    abbreviation TEXT,"
        "    size INTEGER,"
        "    playable INTEGER"
        ");",

        "CREATE TABLE zones ("
        "    id INTEGER PRIMARY KEY,"
        "    name TEXT,"
        "    builders TEXT,"
        "    lifespan INTEGER,"
        "    age INTEGER,"
        "    bot INTEGER,"
        "    top INTEGER,"
        "    reset_mode INTEGER,"
        "    min_level INTEGER,"
        "    max_level INTEGER"
        ");",

        "CREATE TABLE reset_commands ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    zone_id INTEGER,"
        "    command CHAR,"
        "    if_flag BOOLEAN,"
        "    arg1 INTEGER,"
        "    arg2 INTEGER,"
        "    arg3 INTEGER,"
        "    arg4 INTEGER,"
        "    arg5 INTEGER,"
        "    line INTEGER,"
        "    sarg1 TEXT,"
        "    sarg2 TEXT,"
        "    FOREIGN KEY(zone_id) REFERENCES zones(id)"
        ");",

        "CREATE TABLE zones_zone_flags ("
        "    zone_id INTEGER,"
        "    flag_id INTEGER,"
        "    PRIMARY KEY(zone_id, flag_id),"
        "    FOREIGN KEY(zone_id) REFERENCES zones(id),"
        "    FOREIGN KEY(flag_id) REFERENCES zone_bits(id)"
        ");",

        "CREATE TABLE scripts ("
        "    id INTEGER PRIMARY KEY,"
        "    name TEXT,"
        "    attach_type INTEGER,"
        "    trigger_type INTEGER,"
        "    script_body TEXT,"
        "    arglist TEXT,"
        "    zone_id INTEGER,"
        "    FOREIGN KEY(zone_id) REFERENCES zones(id)"
        ");",

        "CREATE TABLE rooms ("
        "    id INTEGER PRIMARY KEY,"
        "    name TEXT,"
        "    clean_name TEXT,"
        "    look_description TEXT,"
        "    sector_type INTEGER,"
        "    zone_id INTEGER,"
        "    sense_location TEXT,"
        "    FOREIGN KEY(zone_id) REFERENCES zones(id)"
        ");",

        "CREATE TABLE room_flags ("
        "    id INTEGER PRIMARY KEY,"
        "    room_id INTEGER,"
        "    flag_id INTEGER,"
        "    FOREIGN KEY (room_id) REFERENCES rooms (id)"
        ");",

        "CREATE TABLE room_extra_descriptions ("
        "    id INTEGER PRIMARY KEY,"
        "    room_id INTEGER,"
        "    keyword TEXT,"
        "    description TEXT,"
        "    FOREIGN KEY (room_id) REFERENCES rooms (id)"
        ");",

        "CREATE TABLE exits ("
        "    id INTEGER PRIMARY KEY,"
        "    room_id INTEGER,"
        "    direction INTEGER,"
        "    general_description TEXT,"
        "    keyword TEXT,"
        "    key INTEGER,"
        "    to_room INTEGER,"
        "    dclock INTEGER,"
        "    dchide INTEGER,"
        "    dcskill INTEGER,"
        "    dcmove INTEGER,"
        "    failsavetype INTEGER,"
        "    dcfailsave INTEGER,"
        "    failroom INTEGER,"
        "    totalfailroom INTEGER,"
        "    FOREIGN KEY (room_id) REFERENCES rooms (id)"
        ");",

        "CREATE TABLE IF NOT EXISTS room_scripts (\n"
        "    room_id INTEGER,\n"
        "    script_id INTEGER,\n"
        "    PRIMARY KEY (room_id, script_id),\n"
        "    FOREIGN KEY (room_id) REFERENCES rooms (id),\n"
        "    FOREIGN KEY (script_id) REFERENCES scripts (id)\n"
        ");",

        "CREATE TABLE exits_info ("
        "    id INTEGER PRIMARY KEY,"
        "    exit_id INTEGER,"
        "    flag_id INTEGER,"
        "    FOREIGN KEY (exit_id) REFERENCES exits (id)"
        ");",

        "CREATE TABLE objects (\n"
        "    id INTEGER PRIMARY KEY,\n"
        "    name TEXT,\n"
        "    room_description TEXT,\n"
        "    look_description TEXT,\n"
        "    short_description TEXT,\n"
        "    type_flag INTEGER,\n"
        "    level INTEGER,\n"
        "    weight INTEGER,\n"
        "    cost INTEGER,\n"
        "    cost_per_day INTEGER,\n"
        "    size INTEGER\n"
        ");",

        "CREATE TABLE object_extra_desc (\n"
        "    id INTEGER PRIMARY KEY,\n"
        "    object_id INTEGER,\n"
        "    keyword TEXT,\n"
        "    description TEXT,\n"
        "    FOREIGN KEY (object_id) REFERENCES objects (id)\n"
        ");",

        "CREATE TABLE object_wear_flags (\n"
        "    object_id INTEGER,\n"
        "    wear_flag_id INTEGER,\n"
        "    FOREIGN KEY (object_id) REFERENCES objects (id),\n"
        "    FOREIGN KEY (wear_flag_id) REFERENCES wear_bits (id)\n"
        ");",

        "CREATE TABLE object_extra_flags (\n"
        "    object_id INTEGER,\n"
        "    extra_flag_id INTEGER,\n"
        "    FOREIGN KEY (object_id) REFERENCES objects (id),\n"
        "    FOREIGN KEY (extra_flag_id) REFERENCES extra_bits (id)\n"
        ");",

        "CREATE TABLE object_affected_flags (\n"
        "    object_id INTEGER,\n"
        "    affected_flag_id INTEGER,\n"
        "    FOREIGN KEY (object_id) REFERENCES objects (id),\n"
        "    FOREIGN KEY (affected_flag_id) REFERENCES affected_bits (id)\n"
        ");",

        "CREATE TABLE object_values (\n"
        "    object_id INTEGER,\n"
        "    position INTEGER,\n"
        "    value INTEGER,\n"
        "    FOREIGN KEY (object_id) REFERENCES objects (id)\n"
        ");",

        "CREATE TABLE object_affects (\n"
        "    object_id INTEGER,\n"
        "    idx INTEGER,\n"
        "    location INTEGER,\n"
        "    specific INTEGER,\n"
        "    modifier INTEGER,\n"
        "    FOREIGN KEY (object_id) REFERENCES objects (id)\n"
        ");",

        "CREATE TABLE IF NOT EXISTS object_scripts (\n"
        "    object_id INTEGER,\n"
        "    script_id INTEGER,\n"
        "    PRIMARY KEY (object_id, script_id),\n"
        "    FOREIGN KEY (object_id) REFERENCES objects (id),\n"
        "    FOREIGN KEY (script_id) REFERENCES scripts (id)\n"
        ");",

        "CREATE TABLE IF NOT EXISTS characters (\n"
        "    id INTEGER PRIMARY KEY,\n"
        "    name TEXT,\n"
        "    room_description TEXT,\n"
        "    look_description TEXT,\n"
        "    short_description TEXT,\n"
        "    size INTEGER,\n"
        "    sex INTEGER,\n"
        "    race_id INTEGER,\n"
        "    race_level INTEGER,\n"
        "    level_adj INTEGER,\n"
        "    sensei_id INTEGER,\n"
        "    level INTEGER,\n"
        "    alignment INTEGER,\n"
        "    armor INTEGER,\n"
        "    gold INTEGER,\n"
        "    damage_mod INTEGER,\n"
        "    basepl INTEGER,\n"
        "    baseki INTEGER,\n"
        "    basest INTEGER,\n"
        "    str INTEGER,\n"
        "    intel INTEGER,\n"
        "    wis INTEGER,\n"
        "    dex INTEGER,\n"
        "    con INTEGER,\n"
        "    cha INTEGER\n"
        ");",

        "CREATE TABLE IF NOT EXISTS character_scripts (\n"
        "    character_id INTEGER,\n"
        "    script_id INTEGER,\n"
        "    PRIMARY KEY (character_id, script_id),\n"
        "    FOREIGN KEY (character_id) REFERENCES characters (id),\n"
        "    FOREIGN KEY (script_id) REFERENCES scripts (id)\n"
        ");",

        "CREATE TABLE IF NOT EXISTS character_act_bits (\n"
        "    character_id INTEGER,\n"
        "    act_bit INTEGER,\n"
        "    PRIMARY KEY (character_id, act_bit),\n"
        "    FOREIGN KEY (character_id) REFERENCES characters (id)\n"
        ");",

        "CREATE TABLE IF NOT EXISTS character_affected_bits (\n"
        "    character_id INTEGER,\n"
        "    affected_bit INTEGER,\n"
        "    PRIMARY KEY (character_id, affected_bit),\n"
        "    FOREIGN KEY (character_id) REFERENCES characters (id)\n"
        ");",

        "CREATE TABLE IF NOT EXISTS npc_prototypes ("
        "   id INTEGER PRIMARY KEY,\n"
        "   character_id INTEGER,"
        "    attack_type INTEGER,\n"
        "    default_pos INTEGER,\n"
        "    damnodice INTEGER,\n"
        "    damsizedice INTEGER,\n"
        "    zone_id INTEGER,"
        "    FOREIGN KEY(zone_id) REFERENCES zones(id),"
        "    FOREIGN KEY(character_id) REFERENCES characters(id)\n"
        ");",

        "CREATE TABLE IF NOT EXISTS item_prototypes ("
        "   id INTEGER PRIMARY KEY,\n"
        "   object_id INTEGER,"
        "    zone_id INTEGER,"
        "    FOREIGN KEY(zone_id) REFERENCES zones(id),"
        "   FOREIGN KEY(object_id) REFERENCES objects(id)\n"
        ");",

        "CREATE VIEW npc_prototypes_view AS SELECT n.id as vnum,c.*,n.attack_type,n.default_pos,n.damnodice,n.damsizedice,n.zone_id FROM npc_prototypes n LEFT JOIN characters AS c ON n.character_id = c.id;",

        "CREATE VIEW item_prototypes_view AS SELECT i.id as vnum,o.*,i.zone_id FROM item_prototypes i LEFT JOIN objects AS o ON i.object_id = o.id;",

        "CREATE TABLE IF NOT EXISTS accounts ("
        "   id INTEGER PRIMARY KEY,\n"
        "   name TEXT UNIQUE,\n"
        "   password TEXT,\n"
        "   email TEXT,\n"
        "   rpp INTEGER,\n"
        "   supervisor_level INTEGER,\n"
        "   character_slots INTEGER"
        ");",

        "CREATE TABLE IF NOT EXISTS character_player_bits (\n"
        "    character_id INTEGER,\n"
        "    player_bit INTEGER,\n"
        "    PRIMARY KEY (character_id, player_bit),\n"
        "    FOREIGN KEY (character_id) REFERENCES characters (id)\n"
        ");",

        "CREATE TABLE IF NOT EXISTS player_characters ("
        "   id INTEGER PRIMARY KEY,"
        "   character_id INTEGER,"
        "   name TEXT UNIQUE,"
        "   level INTEGER,"
        "   admlevel INTEGER,"
        "   last INTEGER,"
        "   ship INTEGER,"
        "   shiproom INTEGER,"
        "   played INTEGER,"
        "   clan TEXT,"
        "   FOREIGN KEY(character_id) REFERENCES characters(id)\n"
        ");",

        "CREATE TABLE IF NOT EXISTS player_skills ("
        "   character_id INTEGER,"
        "   skill_id INTEGER,"
        "   level INTEGER,"
        "   bonus INTEGER,"
        "   perfection INTEGER,"
        "   PRIMARY KEY(character_id, skill_id)"
        ");",

        "CREATE TABLE IF NOT EXISTS player_bonus ("
        "   character_id INTEGER,"
        "   bonus_id INTEGER,"
        "   PRIMARY KEY(character_id, bonus_id)"
        ");",

        "CREATE TABLE IF NOT EXISTS accounts_characters ("
        "   account_id INTEGER,\n"
        "   player_character_id INTEGER,\n"
        "   PRIMARY KEY(account_id, player_character_id),\n"
        "   FOREIGN KEY(player_character_id) REFERENCES player_characters(id),\n"
        "   FOREIGN KEY(account_id) REFERENCES accounts(id)"
        ");",

        "CREATE TABLE IF NOT EXISTS skill_names ("
        "   id INTEGER PRIMARY KEY,"
        "   name TEXT"
        ");",

        "CREATE TABLE IF NOT EXISTS space ("
        "   id INTEGER PRIMARY KEY,"
        "   x INTEGER NOT NULL,"
        "   y INTEGER NOT NULL,"
        "   z INTEGER NOT NULL,"
        "   tile TEXT NOT NULL,"
        "   name TEXT NOT NULL,"
        "   description TEXT NOT NULL,"
        "   exits TEXT NOT NULL,"
        "   doors TEXT NOT NULL,"
        "   UNIQUE(x,y,z)"
        ");"

};

void dump_assets() {
    for(const auto& q : schema) {
        run_query(q);
    }

    SQLite::Statement q1(*db, "INSERT INTO senseis (id, name, abbreviation, style_name) VALUES (?, ?, ?, ?);");
    for(const auto& [s_id, sen] : sensei::sensei_map) {
        q1.bind(1, sen->getID());
        q1.bind(2, sen->getName());
        q1.bind(3, sen->getAbbr());
        q1.bind(4, sen->getStyleName());
        q1.exec();
        q1.reset();
    }

    SQLite::Statement q2(*db, "INSERT INTO races (id, name, abbreviation, size, playable) VALUES (?, ?, ?, ?, ?);");

    for(const auto& [r_id, r] : race::race_map) {
        q2.bind(1, r->getID());
        q2.bind(2, r->getName());
        q2.bind(3, r->getAbbr());
        q2.bind(4, r->getSize());
        q2.bind(5, r->isPcOk());
        q2.exec();
        q2.reset();
    }

}

void dump_zones() {

    SQLite::Statement q1(*db, "INSERT INTO zones (id, name, builders, lifespan, age, bot, top, reset_mode, min_level, max_level) VALUES "
                              "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    SQLite::Statement q2(*db, "INSERT INTO reset_commands (zone_id, command, if_flag, arg1, arg2, arg3, arg4, arg5, line, sarg1, sarg2) VALUES"
                              "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    SQLite::Statement q3(*db, "INSERT INTO zones_zone_flags (zone_id, flag_id) VALUES (?, ?);");

    for(const auto& [id, zone] : zone_table) {
        q1.bind(1, id);
        q1.bind(2, zone.name);
        q1.bind(3, zone.builders);
        q1.bind(4, zone.lifespan);
        q1.bind(5, zone.age);
        q1.bind(6, zone.bot);
        q1.bind(7, zone.top);
        q1.bind(8, zone.reset_mode);
        q1.bind(9, zone.min_level);
        q1.bind(10, zone.max_level);
        q1.exec();
        q1.reset();

        int line = 0;
        for(const auto& cmd : zone.cmd) {
            std::string command;
            command.push_back(cmd.command);
            q2.bind(1, id);
            q2.bind(2, command);
            q2.bind(3, cmd.if_flag);
            q2.bind(4, cmd.arg1);
            q2.bind(5, cmd.arg2);
            q2.bind(6, cmd.arg3);
            q2.bind(7, cmd.arg4);
            q2.bind(8, cmd.arg5);
            q2.bind(9, ++line);
            q2.bind(10, cmd.sarg1);
            q2.bind(11, cmd.sarg2);
            q2.exec();
            q2.reset();
        }

        for(auto i = 0; i < NUM_ZONE_FLAGS; i++) {
            if(!IS_SET_AR(zone.zone_flags, i)) {
                continue;
            }
            q3.bind(1, id);
            q3.bind(2, i);
            q3.exec();
            q3.reset();
        }
    }
}

void dump_scripts() {
    SQLite::Statement query(*db, "INSERT INTO scripts (id, name, attach_type, trigger_type, script_body, arglist, zone_id) VALUES (?,?,?,?,?,?,?);");

    for(const auto& [id, trig] : trig_index) {
        query.bind(1, id);
        query.bind(2, trig.proto->name);
        query.bind(3, trig.proto->attach_type);
        query.bind(4, trig.proto->trigger_type);

        std::string script_body;
        for(auto cmd = trig.proto->cmdlist; cmd; cmd = cmd->next) {
            script_body += cmd->cmd;
            script_body += "\n";
        }

        query.bind(5, script_body);
        query.bind(6, trig.proto->arglist);
        query.bind(7, real_zone_by_thing(id));
        query.exec();
        query.reset();
    }
}

void dump_rooms() {
    SQLite::Statement query(*db, "INSERT INTO rooms (id, name, clean_name, look_description, sector_type, sense_location, zone_id) VALUES (?,?,?,?,?,?,?);");

    SQLite::Statement q2(*db, "INSERT INTO room_extra_descriptions (room_id, keyword, description) VALUES (?,?,?);");

    SQLite::Statement q3(*db, "INSERT INTO room_flags (room_id, flag_id) VALUES (?,?);");

    SQLite::Statement q4(*db, "INSERT INTO exits (room_id, direction, general_description, keyword, key, to_room, dclock, dchide, dcskill, dcmove, failsavetype, dcfailsave, failroom, totalfailroom) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?);");

    SQLite::Statement q5(*db, "INSERT INTO exits_info (exit_id, flag_id) VALUES (?,?);");

    SQLite::Statement q6(*db, "INSERT INTO room_scripts (room_id, script_id) VALUES (?,?)");

    for(const auto& [id, room] : world) {
        query.bind(1, id);
        query.bind(2, room.name);
        query.bind(3, processColors(room.name,false,nullptr));
        query.bind(4, room.look_description);
        query.bind(5, room.sector_type);
        query.bind(6, sense_location_name(id));
        query.bind(7, real_zone_by_thing(id));
        query.exec();
        query.reset();

        for(auto ex = room.ex_description; ex; ex = ex->next) {
            q2.bind(1, id);
            q2.bind(2, ex->keyword);
            q2.bind(3, ex->description);
            q2.exec();
            q2.reset();
        }

        std::set<trig_vnum> trig_ids;
        for(auto t = room.proto_script; t; t = t->next) {
            if(trig_ids.find(t->vnum) != trig_ids.end()) {
                continue;
            }
            trig_ids.insert(t->vnum);
            q6.bind(1, id);
            q6.bind(2, t->vnum);
            q6.exec();
            q6.reset();
        }

        for(auto i = 0; i < NUM_ROOM_FLAGS; i++) {
            if(!IS_SET_AR(room.room_flags, i)) {
                continue;
            }
            q3.bind(1, id);
            q3.bind(2, i);
            q3.exec();
            q3.reset();
        }


        for(auto i = 0; i < NUM_OF_DIRS; i++) {
            if(!room.dir_option[i]) {
                continue;
            }
            auto &ex = room.dir_option[i];
            q4.bind(1, id);
            q4.bind(2, i);
            q4.bind(3, ex->general_description);
            q4.bind(4, ex->keyword);
            q4.bind(5, ex->key);
            q4.bind(6, ex->to_room);
            q4.bind(7, ex->dclock);
            q4.bind(8, ex->dchide);
            q4.bind(9, ex->dcskill);
            q4.bind(10, ex->dcmove);
            q4.bind(11, ex->failsavetype);
            q4.bind(12, ex->dcfailsave);
            q4.bind(13, ex->failroom);
            q4.bind(14, ex->totalfailroom);
            q4.exec();
            q4.reset();

            auto eid = db->getLastInsertRowid();
            for(auto i = 0; i < NUM_EXIT_FLAGS; i++) {
                if(!IS_SET(ex->exit_info, i)) {
                    continue;
                }
                q5.bind(1, eid);
                q5.bind(2, i);
                q5.exec();
                q5.reset();
            }
        }
    }
}

long dump_object(obj_data *obj) {
    SQLite::Statement q1(*db, "INSERT INTO objects (name, room_description, look_description, short_description, type_flag, level, weight, cost, cost_per_day, size) VALUES (?,?,?,?,?,?,?,?,?,?);");
    SQLite::Statement q2(*db, "INSERT INTO object_extra_desc (object_id, keyword, description) VALUES (?,?,?);");
    SQLite::Statement q3(*db, "INSERT INTO object_wear_flags (object_id, wear_flag_id) VALUES (?,?);");
    SQLite::Statement q4(*db, "INSERT INTO object_extra_flags (object_id, extra_flag_id) VALUES (?,?);");
    SQLite::Statement q5(*db, "INSERT INTO object_affected_flags (object_id, affected_flag_id) VALUES (?,?);");
    SQLite::Statement q6(*db, "INSERT INTO object_values (object_id, position, value) VALUES (?,?,?);");
    SQLite::Statement q7(*db, "INSERT INTO object_affects (object_id, idx, location, specific, modifier) VALUES (?,?,?,?,?);");

    SQLite::Statement q8(*db, "INSERT INTO object_scripts (object_id, script_id) VALUES (?,?)");

    q1.bind(1, obj->name);
    q1.bind(2, obj->room_description);
    q1.bind(3, obj->look_description);
    q1.bind(4, obj->short_description);
    q1.bind(5, obj->type_flag);
    q1.bind(6, obj->level);
    q1.bind(7, obj->weight);
    q1.bind(8, obj->cost);
    q1.bind(9, obj->cost_per_day);
    q1.bind(10, obj->size);
    q1.exec();

    auto id = db->getLastInsertRowid();

    for (auto ex = obj->ex_description; ex; ex = ex->next) {
        if (!(ex->keyword && ex->description)) continue;
        q2.bind(1, id);
        q2.bind(2, ex->keyword);
        q2.bind(3, ex->description);
        q2.exec();
        q2.reset();
    }

    for(auto t = obj->proto_script; t; t = t->next) {
        q8.bind(1, id);
        q8.bind(2, t->vnum);
        q8.exec();
        q8.reset();
    }

    for (auto i = 0; i < NUM_ITEM_WEARS; i++) {
        if (!IS_SET_AR(obj->wear_flags, i)) {
            continue;
        }
        q3.bind(1, id);
        q3.bind(2, i);
        q3.exec();
        q3.reset();
    }

    for (auto i = 0; i < NUM_ITEM_FLAGS; i++) {
        if (!IS_SET_AR(obj->extra_flags, i)) {
            continue;
        }
        q4.bind(1, id);
        q4.bind(2, i);
        q4.exec();
        q4.reset();
    }

    for (auto i = 0; i < NUM_AFF_FLAGS; i++) {
        if (!IS_SET_AR(obj->bitvector, i)) {
            continue;
        }
        q5.bind(1, id);
        q5.bind(2, i);
        q5.exec();
        q5.reset();
    }

    for (auto i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
        if (obj->value[i] == 0) continue;
        q6.bind(1, id);
        q6.bind(2, i);
        q6.bind(3, obj->value[i]);
        q6.exec();
        q6.reset();
    }

    for (auto i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location == APPLY_NONE) {
            continue;
        }
        auto &aff = obj->affected[i];
        q7.bind(1, id);
        q7.bind(2, i);
        q7.bind(3, aff.location);
        q7.bind(4, aff.specific);
        q7.bind(5, aff.modifier);
        q7.exec();
        q7.reset();

    }
    return id;
    
}

void dump_objects() {
    SQLite::Statement q1(*db, "INSERT INTO item_prototypes (id, object_id,zone_id) VALUES (?,?,?);");

    for(auto& [id, obj] : obj_proto) {
        auto oid = dump_object(&obj);
        q1.bind(1, id);
        q1.bind(2, oid);
        q1.bind(3, real_zone_by_thing(id));
        q1.exec();
        q1.reset();
    }

}

long dump_character(char_data *ch, bool is_npc = true) {
    SQLite::Statement q1(*db,
                         "INSERT INTO characters (name, room_description, look_description, short_description,"
                         "size, sex, race_id, race_level, level_adj, sensei_id, level, alignment, armor, gold, "
                         "damage_mod, basepl, baseki, basest, str, intel, wis, dex, con, cha) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);");
    // Thank you Copilot. I love you.

    SQLite::Statement q2(*db, "INSERT INTO character_scripts (character_id, script_id) VALUES (?,?)");

    SQLite::Statement q3(*db, "INSERT INTO character_act_bits (character_id, act_bit) VALUES (?,?)");

    SQLite::Statement q4(*db, "INSERT INTO character_affected_bits (character_id, affected_bit) VALUES (?,?)");
    
    q1.bind(1, ch->name);
    q1.bind(2, ch->room_description);
    q1.bind(3, ch->look_description);
    q1.bind(4, ch->short_description);
    q1.bind(5, ch->size);
    q1.bind(6, ch->sex);
    q1.bind(7, ch->race->getID());
    q1.bind(8, ch->race_level);
    q1.bind(9, ch->level_adj);
    q1.bind(10, ch->chclass->getID());
    q1.bind(11, ch->level);
    q1.bind(12, ch->alignment);
    q1.bind(13, ch->armor);
    q1.bind(14, ch->gold);
    q1.bind(15, ch->damage_mod);
    q1.bind(16, ch->basepl);
    q1.bind(17, ch->baseki);
    q1.bind(18, ch->basest);
    q1.bind(19, ch->real_abils.str);
    q1.bind(20, ch->real_abils.intel);
    q1.bind(21, ch->real_abils.wis);
    q1.bind(22, ch->real_abils.dex);
    q1.bind(23, ch->real_abils.con);
    q1.bind(24, ch->real_abils.cha);
    q1.exec();

    auto id = db->getLastInsertRowid();

    for(auto t = ch->proto_script; t; t = t->next) {
        q2.bind(1, id);
        q2.bind(2, t->vnum);
        q2.exec();
        q2.reset();
    }

    if(is_npc) for(auto i = 0; i < NUM_MOB_FLAGS; i++) {
        if(!IS_SET_AR(ch->act, i)) {
            continue;
        }
        q3.bind(1, id);
        q3.bind(2, i);
        q3.exec();
        q3.reset();
    }

    for(auto i = 0; i < NUM_AFF_FLAGS; i++) {
        if(!IS_SET_AR(ch->affected_by, i)) {
            continue;
        }
        q4.bind(1, id);
        q4.bind(2, i);
        q4.exec();
        q4.reset();
    }

    return id;
    
}

void dump_npc() {
    SQLite::Statement q1(*db,
                         "INSERT INTO npc_prototypes (id, character_id, attack_type, default_pos,"
                         "damnodice, damsizedice) VALUES (?,?,?,?,?,?);");
    // Thank you Copilot. I love you.

    for (auto &[id, npc]: mob_proto) {
        auto cid = dump_character(&npc);
        q1.bind(1, id);
        q1.bind(2, cid);
        q1.bind(3, npc.mob_specials.attack_type);
        q1.bind(4, npc.mob_specials.default_pos);
        q1.bind(5, npc.mob_specials.damnodice);
        q1.bind(6, npc.mob_specials.damsizedice);
        q1.exec();
        q1.reset();
    }


}

static std::set<long> player_ids;

std::optional<long> dump_player(const char *name) {
    SQLite::Statement q1(*db,
                         "INSERT INTO player_characters (id, character_id, name, level, admlevel, last, ship,"
                         "shiproom, played, clan) VALUES (?,?,?,?,?,?,?,?,?,?);");
    SQLite::Statement q2(*db, "INSERT INTO character_player_bits (character_id, player_bit) VALUES (?,?)");

    SQLite::Statement q3(*db, "INSERT INTO player_skills (character_id, skill_id, level, bonus, perfection) VALUES (?,?,?,?,?)");

    SQLite::Statement q4(*db, "INSERT INTO player_bonus (character_id, bonus_id) VALUES (?,?)");

    char_data *ch = new char_data();
    if(load_char(name, ch) < 0) {
        basic_mud_log("Error loading player %s", name);
        free_char(ch);
        return {};
    }
    if(player_ids.count(ch->id)) {
        basic_mud_log("Duplicate player %s", name);
        free_char(ch);
        return {};
    }
    player_ids.insert(ch->id);
    basic_mud_log("Dumping player %d: %s", ch->id, ch->name);
    auto id = ch->id;
    auto cid = dump_character(ch, false);
    q1.bind(1, ch->id);
    q1.bind(2, cid);
    q1.bind(3, ch->name);
    q1.bind(4, ch->level);
    q1.bind(5, ch->admlevel);
    q1.bind(6, ch->lastpl);
    q1.bind(7, ch->ship);
    q1.bind(8, ch->shipr);
    q1.bind(9, 0);
    q1.bind(10, ch->clan);

    q1.exec();

    for(auto i = 0; i < NUM_PLR_FLAGS; i++) {
        if(!IS_SET_AR(ch->act, i)) {
            continue;
        }
        q2.bind(1, cid);
        q2.bind(2, i);
        q2.exec();
        q2.reset();
    }

    for(auto i = 0; i < SKILL_TABLE_SIZE; i++) {
        if(GET_SKILL(ch, i) || GET_SKILL_BONUS(ch, i) || GET_SKILL_PERF(ch, i)) {
            q3.bind(1, cid);
            q3.bind(2, i);
            q3.bind(3, GET_SKILL(ch, i));
            q3.bind(4, GET_SKILL_BONUS(ch, i));
            q3.bind(5, GET_SKILL_PERF(ch, i));
            q3.exec();
            q3.reset();
        }
    }

    for (auto i = 0; i < 52; i++) {
        if(GET_BONUS(ch, i)) {
            q4.bind(1, cid);
            q4.bind(2, i);
            q4.exec();
            q4.reset();
        };
    }

    free_char(ch);
    return id;

}

void dump_accounts() {

    SQLite::Statement q1(*db, "INSERT INTO accounts (name, password, email, rpp, supervisor_level, character_slots) VALUES (?,?,?,?,?,?)");
    SQLite::Statement q2(*db, "INSERT INTO accounts_characters (account_id, player_character_id) VALUES (?,?)");

    auto ufolder = std::filesystem::path("user");
    for(auto &p : std::filesystem::recursive_directory_iterator(ufolder)) {
        if(!p.path().string().ends_with(".usr")) continue;
        std::ifstream f(p.path());

        std::string name, email, password;
        long rpp, supervisor_level, character_slots;

        std::getline(f, name);
        std::getline(f, email);
        std::getline(f, password);
        q1.bind(1, name);
        q1.bind(2, password);
        q1.bind(3, email);
        f >> character_slots;
        f >> rpp;
        q1.bind(4,rpp);
        q1.bind(6, character_slots);

        std::vector<std::string> characters;
        for(int i = 0; i < 5; i++) {
            std::string line;
            std::getline(f, line);
            if(line == "Empty") continue;
            characters.push_back(line);
        }

        f >> supervisor_level;
        q1.bind(5, supervisor_level);
        int b;
        f >> b;
        f >> b;
        q1.exec();
        q1.reset();

        auto id = db->getLastInsertRowid();

        for(auto &c : characters) {
            auto player_character_id = dump_player(c.c_str());
            if(!player_character_id.has_value()) {
                basic_mud_log("Error dumping player %s", c.c_str());
                continue;
            }
            q2.bind(1, id);
            q2.bind(2, player_character_id.value());
            q2.exec();
            q2.reset();
        }
    }
}

void dump_skills() {
    SQLite::Statement q1(*db, "INSERT INTO skill_names (id, name) VALUES (?,?)");

    for(auto i = 0; i < SKILL_TABLE_SIZE; i++) {
        if(!spell_info[i].name) continue;
        if(!strcasecmp(spell_info[i].name, "!UNUSED!")) continue;
        q1.bind(1, i);
        q1.bind(2, spell_info[i].name);
        q1.exec();
        q1.reset();
    }

}

void dump_space() {
    // for this we need to scan mapnums, both rows and columns, and check the old "space" rooms at those ids.
    // Only if it's something particularly cool will we add it to the grid.
    int curX = -100, curY = 100;
    SQLite::Statement q1(*db, "INSERT INTO space (id, x, y, z, tile, name, description, exits, doors) VALUES (?,?,?,?,?,?,?,?,?)");
    auto &base = world[20199];
    for(auto &row : mapnums) {
        for(auto &col : row) {
            q1.bind(1, col);
            q1.bind(2, curX);
            q1.bind(3, curY);
            q1.bind(4, 0);

            auto &room = world[col];

            std::string tile = "";
            if(IS_SET_AR(room.room_flags, ROOM_EORBIT)) {
                    tile = "@GE@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_NORBIT)) {
                    tile = "@gN@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_KORBIT)) {
                    tile = "@mK@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_VORBIT)) {
                    tile = "@YV@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_AORBIT)) {
                    tile = "@BA@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_YORBIT)) {
                    tile = "@MY@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_KANORB)) {
                    tile = "@CK@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_ARLORB)) {
                    tile = "@mA@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_NEBULA)) {
                    tile = "@m&@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_ASTERO)) {
                    tile = "@yQ@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_WORMHO)) {
                    tile = "@1 @n";
                } else if(IS_SET_AR(room.room_flags, ROOM_STATION)) {
                    tile = "@DS@n";
                } else if(IS_SET_AR(room.room_flags, ROOM_STAR)) {
                    tile = "@6 @n";
                } else if(IS_SET_AR(room.room_flags, ROOM_CORBIT)) {
                    tile = "@MC@n";
                }
            q1.bind(5, tile);
            if(!strcasecmp(base.name, room.name)) {
                q1.bind(6, "");
            } else {
                q1.bind(6, room.name);
            }
            if(!strcasecmp(base.look_description, room.look_description)) {
                q1.bind(7, "");
            } else {
                q1.bind(7, room.look_description);
            }

            nlohmann::json exits, doors;
            for(auto &dir : {UP, DOWN, INDIR, OUTDIR}) {
                    if(room.dir_option[dir]) {
                        auto &exit = room.dir_option[dir];
                        nlohmann::json destination, door;
                        destination["destination"] = exit->to_room;
                        exits.push_back(std::make_pair(dir, destination));

                        if(exit->keyword && strlen(exit->keyword)) door["keyword"] = exit->keyword;
                        if(exit->general_description && strlen(exit->general_description)) door["description"] = exit->general_description;
                        if(exit->dclock) door["dclock"] = exit->dclock;
                        if(exit->key) door["legacyKey"] = exit->key;
                        if(!door.empty()) doors.push_back(std::make_pair(dir, door));
                    }
                }
            if(!exits.empty()) {
                q1.bind(8, exits.dump(4, ' ', false, nlohmann::json::error_handler_t::ignore));
            } else {
                q1.bind(8, "");
            }
            if(!doors.empty()) {
                q1.bind(9, doors.dump(4, ' ', false, nlohmann::json::error_handler_t::ignore));
            } else {
                q1.bind(9, "");
            }
            q1.exec();
            q1.reset();
            curX++;
        }
        curX = -100;
        curY--;
    }
}


void dump_db() {
    // Define the database file name
    const std::string dbName = "dbat.sqlite3";

    // Check if dbat.sqlite3 exists in the current working directory. If it does, delete it.
    if (std::filesystem::exists(dbName)) {
        std::filesystem::remove(dbName);
    }

    // Afterwards, open a new database into the above pointer named dbat.sqlite3
    try {
        db = std::make_unique<SQLite::Database>(dbName, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        exit(1);
    }

    SQLite::Transaction transaction(*db);
    basic_mud_log("Dumping basic tables...");
    create_basic_tables();

    basic_mud_log("Dumping senseis and races...");
    dump_assets();
    basic_mud_log("Dumping zones...");
    dump_zones();

    basic_mud_log("Dumping scripts...");
    dump_scripts();

    basic_mud_log("Dumping rooms and exits...");
    dump_rooms();

    basic_mud_log("Dumping space...");
    dump_space();

    basic_mud_log("Dumping item prototypes...");
    dump_objects();

    basic_mud_log("Dumping npc prototypes...");
    dump_npc();

    basic_mud_log("Dumping accounts...");
    dump_accounts();

    basic_mud_log("Dumping skill names...");
    dump_skills();

    // Commit transaction
    transaction.commit();
    basic_mud_log("Dumping complete!");

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
                basic_mud_log("Loading player objects from secondary (ascii) files.");
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

    try {
        /* All arguments have been parsed, try to open log file. */
        setup_log();
    }
    catch(std::exception& e) {
        std::cerr << "Cannot start logger: " << e.what() << std::endl;
        exit(1);
    }

    /*
     * Moved here to distinguish command line options and to show up
     * in the log if stderr is redirected to a file.
     */
    basic_mud_log("Using %s for configuration.", CONFIG_CONFFILE);
    basic_mud_log("%s", circlemud_version);
    basic_mud_log("%s", oasisolc_version);
    basic_mud_log("%s", DG_SCRIPT_VERSION);
    basic_mud_log("%s", ascii_pfiles_version);
    basic_mud_log("%s", CWG_VERSION);
    xap_objs = 1;
    if (chdir(dir) < 0) {
        perror("SYSERR: Fatal error changing to data directory");
        exit(1);
    }
    basic_mud_log("Using %s as data directory.", dir);

    init_lookup_table();
    boot_db();
    basic_mud_log("Database fully booted!");

    FILE *mapfile;
    int rowcounter, colcounter;
    int vnum_read;
    basic_mud_log("Loading Space Map. ");
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
    basic_mud_log("Dumping db!");
    dump_db();


    return 0;
}
