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
#include "dbat/game/Account.hpp"
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
    for (auto o : dbat::util::filter_raw(con))
        extract_obj(o);

    // delete their gear.
    for (auto &[slot, i] : ch->getEquipment())
        extract_obj(i);

    // unsubscribe from everything, just in case.
    characterSubscriptions.unsubscribeFromAll(ch);

    // Get a copy of player_data
    auto pdata = players.at(ch->player->id);

    // Erase the character from the players map.
    players.erase(pdata->id);

    for (auto &[id, pd] : players)
    {
        // cleanups....
        pd->sense_player.erase(ch->player->id);
        pd->dub_names.erase(ch->player->id);
    }

    // Now we'll deal with the account.
    auto acc = pdata->account;

    // Remove the character from the account's list. That means we'll need to remove the matching ch->id from the vector.
    acc->characters.erase(std::remove_if(acc->characters.begin(), acc->characters.end(), [ch](const auto &c)
                                         { return c == ch->player->id; }),
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
        auto ch = find->second->character;
        if (!ch)
            continue;
        auto shared = ch->shared_from_this();
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

    auto descs = acc->descriptors;
    for (auto d : descs)
        close_socket(d);

    auto cha = acc->characters;

    for (const auto &ref : cha)
    {
        auto found = players.find(ref);
        if (found == players.end())
            continue;
        if (auto ch = found->second->character; ch)
        {
            if (canDeleteCharacter(ch->shared_from_this()))
                return false;
        }
    }

    for (auto c : cha)
    {
        auto found = players.find(c);
        if (found == players.end())
            continue;
        if (auto ch = found->second->character; ch)
        {
            deletePlayerCharacter(ch->shared_from_this());
        }
    }

    accounts.erase(id);

    return true;
}
