#pragma once
#include <filesystem>

void runSave();
void load_zones(const std::filesystem::path& loc);
void load_accounts(const std::filesystem::path& loc);
void load_dgscript_prototypes(const std::filesystem::path& loc);

void load_dgscripts_initial(const std::filesystem::path& loc);
void load_dgscripts_finish(const std::filesystem::path& loc);

void load_globaldata(const std::filesystem::path& loc);

void load_shops(const std::filesystem::path& loc);
void load_guilds(const std::filesystem::path& loc);
void load_rooms(const std::filesystem::path& loc);
void load_exits(const std::filesystem::path& loc);

void load_item_prototypes(const std::filesystem::path& loc);
void load_items_initial(const std::filesystem::path& loc);
void load_items_finish(const std::filesystem::path& loc);
void load_npc_prototypes(const std::filesystem::path& loc);
void load_characters_finish(const std::filesystem::path& loc);
void load_characters_initial(const std::filesystem::path& loc);

void load_players(const std::filesystem::path& loc);