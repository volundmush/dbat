/* ************************************************************************
 *   File: players.c                                     Part of CircleMUD *
 *  Usage: Player loading/saving and utility routines                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/players.hpp"
#include "dbat/game/utils.hpp"
//#include "dbat/game/db.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/dg_scripts.hpp"
#include "dbat/game/class.hpp"
#include "dbat/util/FilterWeak.hpp"

std::unordered_map<std::string, std::shared_ptr<PlayerData>> players;

/*************************************************************************
 *  stuff related to the save/load player system				 *
 *************************************************************************/

constexpr int NUM_OF_SAVE_THROWS = 3;

/*************************************************************************
 *  stuff related to the player file cleanup system			 *
 *************************************************************************/

Character *findPlayer(const std::string &name)
{
    for (auto &[id, player] : players)
    {
        if (boost::iequals(player->name, name))
        {
            return player->character;
        }
    }
    return nullptr;
}

Result<std::string> validate_pc_name(const std::string &name)
{
    auto n = name;
    boost::trim(n);
    // Cannot be empty.
    if (n.empty())
        return err("Player names cannot be empty.");

    if (n.size() > 15)
        return err("Name is too long. 15 characters or less please.");

    // No whitespace allowed...
    if (std::any_of(n.begin(), n.end(), [](auto c)
                    { return std::isspace(c); }))
        return err("Whitespace is not allowed in player names.");

    if (!is_all_alpha(n))
        return err("No special symbols or numbers in names, please.");

    return n;
}
