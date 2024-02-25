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
#include "dbat/entity.h"


std::unordered_map<int64_t, std::shared_ptr<player_data>> players;


long get_id_by_name(const char *name) {
    auto find = findPlayer(name);
    if(!find) return -1;
    return find->getUID();
}


char *get_name_by_id(long id) {
    static char buf[128];
    auto find = getEntity<BaseCharacter>(id);
    if(!find) return nullptr;
    if(find->isNPC()) return nullptr;
    sprintf(buf, "%s", find->getName().c_str());
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
    auto view = reg.view<BaseCharacter, PlayerCharacter>(entt::exclude<Deleted>);
    
    for (auto ent : view) {
        if (iequals(text::get(ent, "name"), name)) {
            return reg.try_get<BaseCharacter>(ent);
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