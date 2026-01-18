#pragma once
#include "serde/json.h"

#include "dbat/Character.h"
#include "dbat/Object.h"
#include "dbat/Zone.h"
#include "dbat/AbstractGridArea.h"
#include "dbat/Shop.h"
#include "dbat/Guild.h"
#include "dbat/TimeInfo.h"
#include "dbat/Help.h"

extern PlayerData* create_player_character(int account_id, const json &j);

void runSave();

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

void to_json(json& j, const help_index_element& a);
void from_json(const json& j, help_index_element& a);

void to_json(json& j, const Account& a);
void from_json(const json& j, Account& a);

void to_json(json& j, const DgScript& t);
void from_json(const json& j, DgScript& t);

void to_json(json&j, const Shop& s);
void from_json(const json& j, Shop& s);

void to_json(json& j, const Guild& g);
void from_json(const json& j, Guild& g);

void to_json(json& j, const Destination &e);
void from_json(const json& j, Destination &e);

void to_json(json& j, const Room& r);
void from_json(const json& j, Room& r);

void to_json(json& j, const Object& o);
void from_json(const json& j, Object& o);

void to_json(json& j, const ObjectPrototype& o);
void from_json(const json& j, ObjectPrototype& o);

void to_json(json& j, const CharacterPrototype& o);
void from_json(const json& j, CharacterPrototype& o);

void to_json(json& j, const Character& c);
void from_json(const json& j, Character& c);

void from_json(const json& j, Character& c);
void to_json(json& j, const Character& c);

void to_json(json& j, const PlayerData& p);
void from_json(const json& j, PlayerData& p);

void to_json(json& j, const TileOverride& p);
void from_json(const json& j, TileOverride& p);

void to_json(json& j, const GridTemplate& p);
void from_json(const json& j, GridTemplate& p);

void to_json(json& j, const Area& p);
void from_json(const json& j, Area& p);

void to_json(json& j, const Structure& p);
void from_json(const json& j, Structure& p);

std::vector<std::filesystem::path> getDumpFiles(const std::filesystem::path &dir, std::string_view pattern);

namespace dbat::save {

    struct SaveTask {
        std::string filename;
        std::function<json()> task;
    };

    std::string generateSaveLocation();

    const std::vector<SaveTask>& getSaveAssetTasks();
    const std::vector<SaveTask>& getSaveUserTasks();

    void runSaveSyncHelper(const std::vector<SaveTask>& tasks, std::string_view folder, std::string_view prefix);
    void runSaveSync();
}