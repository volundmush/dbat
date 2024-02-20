/* ************************************************************************
*   File: players.c                                     Part of CircleMUD *
*  Usage: Player loading/saving and utility routines                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/players.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/dg_scripts.h"
#include "dbat/class.h"
#include "dbat/ban.h"
#include "dbat/account.h"


std::unordered_map<int64_t, std::shared_ptr<player_data>> players;


long get_id_by_name(const char *name) {
    auto find = findPlayer(name);
    if(!find) return -1;
    return find->getUID();
}


char *get_name_by_id(long id) {
    static char buf[128];
    auto find = players.find(id);
    if(find == players.end()) return nullptr;
    sprintf(buf, "%s", find->second->name.c_str());
    return buf;
}


/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/


#define NUM_OF_SAVE_THROWS    3


/*************************************************************************
*  stuff related to the player file cleanup system			 *
*************************************************************************/


BaseCharacter *findPlayer(const std::string& name) {
    for (auto& player : players) {
        if (iequals(player.second->name, name)) {
            return player.second->character;
        }
    }
    return nullptr;
}

OpResult<> validate_pc_name(const std::string& name) {
    auto n = name;
    trim(n);
    // Cannot be empty.
    if(n.empty()) return {false, "Player names cannot be empty."};

    if(n.size() > 15) return {false, "Name is too long. 15 characters or less please."};

    // No whitespace allowed...
    if(std::any_of(n.begin(), n.end(), [](auto c) { return std::isspace(c); }))
        return {false, "Whitespace is not allowed in player names."};

    if(!is_all_alpha(n)) return {false, "No special symbols or numbers in names, please."};
    // And nothing from our badnames list...
    for(auto &badname : invalid_list) {
        if(iequals(n, badname)) {
            return {false, "That name is disallowed. Nothing profane, lame, or conflicting with an official character please."};
        }
    }

    return {true, n};
}

nlohmann::json alias_data::serialize() {
    auto j = nlohmann::json::object();

    if(!name.empty()) j["name"] = name;
    if(!replacement.empty()) j["replacement"] = replacement;
    if(type) j["type"] = type;

    return j;
}


alias_data::alias_data(const nlohmann::json &j) : alias_data() {
    if(j.contains("name")) name = j["name"].get<std::string>();
    if(j.contains("replacement")) replacement = j["replacement"].get<std::string>();
    if(j.contains("type")) type = j["type"];
}

nlohmann::json player_data::serialize() {
    auto j = nlohmann::json::object();
    j["id"] = id;
    j["name"] = name;
    if(account) j["account"] = account->vn;

    for(auto &a : aliases) {
        j["aliases"].push_back(a.serialize());
    }

    for(auto &i : sensePlayer) {
        j["sensePlayer"].push_back(i);
    }

    for(auto &i : senseMemory) {
        j["senseMemory"].push_back(i);
    }

    for(auto &i : dubNames) {
        j["dubNames"].push_back(i);
    }

    for(auto i = 0; i < NUM_COLOR; i++) {
        if(color_choices[i] && strlen(color_choices[i])) j["color_choices"].push_back(std::make_pair(i, color_choices[i]));
    }

    return j;
}


player_data::player_data(const nlohmann::json &j) {
    id = j["id"];
    name = j["name"].get<std::string>();
    if(j.contains("account")) {
        auto accID = j["account"].get<vnum>();
        if(auto accFind = accounts.find(accID); accFind != accounts.end()) account = accFind->second;
        else {
            basic_mud_log("Player data found with invalid account ID.");
        }
    }

    if(j.contains("aliases")) {
        for(auto ja : j["aliases"]) {
            aliases.emplace_back(ja);
        }
    }

    if(j.contains("sensePlayer")) {
        for(auto &i : j["sensePlayer"]) {
            sensePlayer.insert(i.get<int64_t>());
        }
    }

    if(j.contains("senseMemory")) {
        for(auto &i : j["senseMemory"]) {
            senseMemory.insert(i.get<vnum>());
        }
    }

    if(j.contains("dubNames")) {
        for(auto &i : j["dubNames"]) {
            dubNames.emplace(i[0].get<int64_t>(), i[1].get<std::string>());
        }
    }

    if(j.contains("color_choices")) {
        for(auto &i : j["color_choices"]) {
            color_choices[i[0].get<int>()] = strdup(i[1].get<std::string>().c_str());
        }
    }

}