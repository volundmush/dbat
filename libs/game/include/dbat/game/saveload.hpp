#pragma once
#include <filesystem>
#include "json.hpp"


#include "dbat/game/Character.hpp"
#include "dbat/game/Object.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/AbstractGridArea.hpp"
#include "dbat/game/Shop.hpp"
#include "dbat/game/Guild.hpp"
#include "dbat/game/TimeInfo.hpp"
#include "dbat/game/Help.hpp"

#include <boost/asio/awaitable.hpp>

extern PlayerData* create_player_character(int account_id, const json &j);

void load_zones(const std::filesystem::path& loc);
void load_accounts(const std::filesystem::path& loc);
void load_dgscript_prototypes(const std::filesystem::path& loc);
void load_dgscripts(const std::filesystem::path& loc);
void load_globaldata(const std::filesystem::path& loc);

void load_shops(const std::filesystem::path& loc);
void load_guilds(const std::filesystem::path& loc);
void load_rooms(const std::filesystem::path& loc);
void load_exits(const std::filesystem::path& loc);
void load_areas_initial(const std::filesystem::path& loc);
void load_areas_finish(const std::filesystem::path& loc);
void load_grid_templates(const std::filesystem::path& loc);
void load_structures_initial(const std::filesystem::path& loc);
void load_structures_finish(const std::filesystem::path& loc);

void load_item_prototypes(const std::filesystem::path& loc);
void load_items_initial(const std::filesystem::path& loc);
void load_items_finish(const std::filesystem::path& loc);
void load_npc_prototypes(const std::filesystem::path& loc);
void load_characters_finish(const std::filesystem::path& loc);
void load_characters_initial(const std::filesystem::path& loc);

void load_players(const std::filesystem::path& loc);
void load_help(const std::filesystem::path& loc);

void load_assemblies(const std::filesystem::path& loc);

std::vector<std::filesystem::path> getDumpFiles(const std::filesystem::path &dir, std::string_view pattern);

namespace dbat::save {

    void runSaveSync();
}
