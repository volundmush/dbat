/* ************************************************************************
 *   File: players.c                                     Part of CircleMUD *
 *  Usage: Player loading/saving and utility routines                      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include <boost/algorithm/string.hpp>

#include "dbat/players.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/dg_scripts.h"
#include "dbat/class.h"
#include "dbat/ban.h"
#include "dbat/account.h"
#include "dbat/filter.h"

long get_id_by_name(const char *name)
{
    auto find = findPlayer(name);
    if (!find)
        return -1;
    return find->id;
}

char *get_name_by_id(long id)
{
    static char buf[128];
    auto find = players.find(id);
    if (find == players.end())
        return nullptr;
    sprintf(buf, "%s", find->second.name.c_str());
    return buf;
}

/*************************************************************************
 *  stuff related to the save/load player system				 *
 *************************************************************************/

constexpr int NUM_OF_SAVE_THROWS = 3;

/*************************************************************************
 *  stuff related to the player file cleanup system			 *
 *************************************************************************/

Character *findPlayer(const std::string &name)
{
    for (auto &player : players)
    {
        if (boost::iequals(player.second.name, name))
        {
            return player.second.character;
        }
    }
    return nullptr;
}

OpResult<> validate_pc_name(const std::string &name)
{
    auto n = name;
    boost::trim(n);
    // Cannot be empty.
    if (n.empty())
        return {false, "Player names cannot be empty."};

    if (n.size() > 15)
        return {false, "Name is too long. 15 characters or less please."};

    // No whitespace allowed...
    if (std::any_of(n.begin(), n.end(), [](auto c)
                    { return std::isspace(c); }))
        return {false, "Whitespace is not allowed in player names."};

    if (!is_all_alpha(n))
        return {false, "No special symbols or numbers in names, please."};
    // And nothing from our badnames list...
    for (auto &badname : invalid_list)
    {
        if (boost::iequals(n, badname))
        {
            return {false, "That name is disallowed. Nothing profane, lame, or conflicting with an official character please."};
        }
    }

    return {true, n};
}

bool canDeleteCharacter(std::weak_ptr<Character> ref)
{
    auto ch = ref.lock();
    if (!ch)
        return false;

    // We don't want to delete NPCs...
    if (IS_NPC(ch.get()))
        return false;

    // The character must not be logged in!
    if (ch->desc)
        return false;
    if (ch->isActive())
        return false;

    return true;
}

void deletePlayerCharacter(std::weak_ptr<Character> ref)
{
    if (!canDeleteCharacter(ref))
        return;

    auto ch = ref.lock();
    if (!ch)
        return;

    // Okay the coast is clear.

    // erase their inventory.
    auto con = ch->getInventory();
    for (auto o : filter_raw(con))
        extract_obj(o);

    // delete their gear.
    for (auto &[slot, i] : ch->getEquipment())
        extract_obj(i);

    // unsubscribe from everything, just in case.
    characterSubscriptions.unsubscribeFromAll(ch);

    // Get a copy of player_data
    PlayerData pdata = players.at(ch->id);

    // Erase the character from the players map.
    players.erase(ch->id);

    for (auto &[id, pd] : players)
    {
        // cleanups....
        pd.sense_player.erase(ch->id);
        pd.dub_names.erase(ch->id);
    }

    // Now we'll deal with the account.
    auto acc = pdata.account;

    // Remove the character from the account's list. That means we'll need to remove the matching ch->id from the vector.
    acc->characters.erase(std::remove_if(acc->characters.begin(), acc->characters.end(), [ch](const auto &c)
                                         { return c == ch->id; }),
                          acc->characters.end());

    // Let the destructor take it from here, and pray.
    Character::registry.erase(ch->id);
}

bool Account::canBeDeleted()
{
    if (!descriptors.empty())
        return false;
    for (auto ref : characters)
    {
        auto find = players.find(ref);
        if (find == players.end())
            continue;
        auto ch = find->second.character;
        if (!ch)
            continue;
        auto shared = ch->shared();
        if (!canDeleteCharacter(shared))
            return false;
    }
    return true;
}

bool deleteUserAccount(vnum id)
{
    if (!accounts.contains(id))
        return false;
    auto &acc = accounts.at(id);

    auto descs = acc.descriptors;
    for (auto d : descs)
        close_socket(d);

    auto cha = acc.characters;

    for (const auto &ref : cha)
    {
        auto found = players.find(ref);
        if (found == players.end())
            continue;
        if (auto ch = found->second.character; ch)
        {
            if (canDeleteCharacter(ch->shared()))
                return false;
        }
    }

    for (auto c : cha)
    {
        auto found = players.find(c);
        if (found == players.end())
            continue;
        if (auto ch = found->second.character; ch)
        {
            deletePlayerCharacter(ch->shared());
        }
    }

    accounts.erase(id);

    return true;
}