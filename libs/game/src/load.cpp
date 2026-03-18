#include "dbat/game/load.hpp"

#include <chrono>
#include <execution>

#include "dbat/game/json.hpp"

#include "dbat/util/FilterWeak.hpp"
#include "dbat/game/ObjectPrototype.hpp"
#include "dbat/game/CharacterPrototype.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/Area.hpp"
#include "dbat/game/Structure.hpp"
#include "dbat/game/affect.hpp"
#include "dbat/game/config.hpp"
#include "dbat/game/send.hpp"
#include "dbat/game/players.hpp"
#include "dbat/game/db.hpp"
#include "dbat/game/dg_scripts.hpp"
#include "dbat/game/Guild.hpp"
#include "dbat/game/Shop.hpp"
#include "dbat/game/constants.hpp"
#include "dbat/game/races.hpp"
#include "dbat/game/class.hpp"
#include "dbat/game/assemblies.hpp"
#include "dbat/game/ID.hpp"
#include "dbat/game/TimeInfo.hpp"
#include "dbat/game/weather.hpp"
#include "dbat/game/Help.hpp"
#include "dbat/game/utils.hpp"
#include "dbat/game/Database.hpp"


void load_zones()
{
    auto zrows = dbat::db::txn->exec("SELECT id, data FROM dbat.zones_blob");
    for (const auto& row : zrows)
    {
        auto id = row["id"].as<int64_t>();
        auto z = std::make_shared<Zone>();
        auto j = nlohmann::json::parse(row["data"].as<std::string>());
        from_json(j, *z);
        zone_table.emplace(id, z);
    }

    for (auto &[v, z] : zone_table)
    {
        if (z->parent != NOTHING)
        {
            auto it = zone_table.find(z->parent);
            if (it != zone_table.end())
            {
                it->second->children.insert(v);
            }
            else
            {
                basic_mud_log("Warning: zone %d has parent %d which does not exist.", z->number, z->parent);
            }
        }
    }
}


void load_dgscript_prototypes()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * from dbat.dgproto"))
    {

        auto id = row["id"].as<int64_t>();
        auto t = std::make_shared<DgScriptPrototype>();
        t->vn = id;
        t->name = row["name"].as<std::string>();
        t->attach_type = static_cast<UnitType>(row["attach_type"].as<int>());
        t->trigger_type = row["trigger_type"].as<int>();
        t->narg = row["narg"].as<int>();
        t->arglist = row["arglist"].as<std::string>();

        auto body = row["body"].as<std::string>();
        t->setBody(body);

        trig_index.emplace(id, t);
    }
}

// org_data...
// shops serialize/deserialize...
void load_shops()
{
    for (const auto& row : dbat::db::txn->exec("SELECT id,data FROM dbat.shops_blob"))
    {
        auto id = row["id"].as<int64_t>();
        auto s = std::make_shared<Shop>();
        auto j = nlohmann::json::parse(row["data"].as<std::string>());
        from_json(j, *s);
        shop_index.emplace(id, s);
    }
}

void load_guilds()
{
    for (const auto& row : dbat::db::txn->exec("SELECT id,data FROM dbat.guilds_blob"))
    {
        auto id = row["id"].as<int64_t>();
        auto g = std::make_shared<Guild>();
        auto j = nlohmann::json::parse(row["data"].as<std::string>());
        from_json(j, *g);
        guild_index.emplace(id, g);
    }
}

// globaldata serialize/deserialize...
void load_globaldata()
{
    for (const auto& row : dbat::db::txn->exec("SELECT key, data FROM dbat.globals")) {
        auto key = row["key"].as<std::string>();
        auto data = nlohmann::json::parse(row["data"].as<std::string>());

        if (key == "time") {
            data.get_to(time_info);
        } else if (key == "era_uptime") {
            data.get_to(era_uptime);
        } else if (key == "weather") {
            data.get_to(weather_info);
        } else if (key == "lastCharacterID") {
            data.get_to(Character::lastID);
        } else if (key == "lastObjectID") {
            data.get_to(Object::lastID);
        } else if (key == "lastStructureID") {
            data.get_to(lastStructureID);
        } else if (key == "lastGridTemplateID") {
            data.get_to(lastGridTemplateID);
        } else if (key == "lastAreaID") {
            data.get_to(lastAreaID);
        } else if (key == "lastRoomID") {
            data.get_to(Room::lastID);
        } else if (key == "lastShopID") {
            data.get_to(lastShopID);
        } else if (key == "lastGuildID") {
            data.get_to(lastGuildID);
        } else if (key == "lastScriptID") {
            data.get_to(lastScriptID);
        }
    }
}

void load_rooms()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.rooms_blob"))
    {
        auto vn = row["id"].as<int64_t>();
        auto r = std::make_shared<Room>();
        // Assuming the JSON data is in the second column (index 1)
        nlohmann::json j = nlohmann::json::parse(row["data"].as<std::string>());
        j.get_to(*r);
        Room::registry.emplace(vn, r);
        r->zone->rooms.add(r);
        r->activate();
    }
    for(auto &[id, z] : zone_table) {
        z->sortRooms();
    }
}

void load_areas_initial()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.areas_blob")) {
        auto id = row["id"].as<int64_t>();
        auto r = std::make_shared<Area>();
        // Assuming the JSON data is in the second column (index 1)
        nlohmann::json j = nlohmann::json::parse(row["data"].as<std::string>());
        j.get_to(*r);
        areas.emplace(id, r);
        r->rebuildShapeIndex();
        auto z = r->getZone();
        z->areas.add(r);
    }
}

void load_areas_finish()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.areas_blob")) {
        auto id = row["id"].as<int64_t>();
        // Assuming the JSON data is in the second column (index 1)
        nlohmann::json j = nlohmann::json::parse(row["data"].as<std::string>());

        if(!j.contains(+"tileOverrides")) continue;

        if(auto cf = areas.find(id); cf != areas.end())
        {
            auto &to = cf->second->tileOverrides;
            j.at(+"tileOverrides").get_to(to);
        }

    }
}

void load_grid_templates()
{

}

void load_structures_initial()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.structures_blob"))
    {
        auto id = row["id"].as<int64_t>();
        auto r = std::make_shared<Structure>();
        // Assuming the JSON data is in the second column (index 1)
        nlohmann::json j = nlohmann::json::parse(row["data"].as<std::string>());
        j.get_to(*r);
        Structure::registry.emplace(id, r);
        r->rebuildShapeIndex();
    }
}

void load_structures_finish()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.structures_blob"))
    {
        auto id = row["id"].as<int64_t>();
        nlohmann::json j = nlohmann::json::parse(row["data"].as<std::string>());

        auto cf = Structure::registry.find(id);
        if(cf == Structure::registry.end()) continue;

        if(j.contains(+"tileOverrides")) {
            auto &to = cf->second->tileOverrides;
            j.at(+"tileOverrides").get_to(to);
        }

        if (j.contains(+"HasLocation"))
        {
            from_json(j["HasLocation"], *cf->second);
            Location l = cf->second->location;
            if(l) {
                cf->second->moveToLocation(l);
            }
        }
    }
}

void load_exits()
{
    for (const auto& row : dbat::db::txn->exec("SELECT id,exits FROM dbat.rooms_blob"))
    {
        auto id = row["id"].as<room_vnum>();
        auto exits = nlohmann::json::parse(row["exits"].as<std::string>());
        auto r = get_room(id);
        exits.get_to(r->exits);
    }
}



// Object serialize/deserialize...

;

void load_item_prototypes()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.oproto_blob"))
    {
        auto id = row["id"].as<int64_t>();
        auto i = std::make_shared<ObjectPrototype>();
        // Assuming the JSON data is in the second column (index 1)
        nlohmann::json j = nlohmann::json::parse(row["data"].as<std::string>());
        j.get_to(*i);
        auto p = obj_proto.emplace(id, i);
    }
}

static void save_inventory(HasInventory &hi, json &j) {
    json inv = json::array();
    auto contents = hi.getInventory();
    for (const auto &item : dbat::util::filter_raw(contents)) {
        json ji;
        to_json(ji, *item);
        save_inventory(*item, ji);
        inv.push_back(ji);
    }
    if (!inv.empty()) {
        j["inventory"] = inv;
    }
}

static void load_inventory(HasInventory &hi, const json &j) {
    if (j.contains(+"inventory")) {
        for (const auto &ji : j["inventory"]) {
            auto item = std::make_shared<Object>();
            ji.get_to(*item);
            Object::registry.emplace(item->id, item);
            hi.addToInventory(item);
            load_inventory(*item, ji);
        }
    }
}

static void save_equipment(HasEquipment &he, json &j) {
    json inv = json::array();
    auto eq = he.getEquipment();
    for (const auto &[slot, item] : eq) {
        json ji, jio;
        ji["slot"] = slot;
        to_json(jio, *item);
        save_inventory(*item, jio);
        ji["item"] = jio;
        inv.push_back(ji);
    }
    if (!inv.empty()) {
        j["equipment"] = inv;
    }
}

static void load_equipment(HasEquipment &he, const json &j) {
    if (j.contains(+"equipment")) {
        for (const auto &ji : j["equipment"]) {
            auto slot = ji["slot"].get<int>();
            auto item = std::make_shared<Object>();
            ji["item"].get_to(*item);
            Object::registry.emplace(item->id, item);
            he.addToEquip(item, slot);
            load_inventory(*item, ji["item"]);
        }
    }
}

static void save_contents(Location& al, json &j) {
    json inv = json::array();
    auto contents = al.getObjects();
    for (const auto &item : dbat::util::filter_raw(contents)) {
        json ji;
        to_json(ji, *item);
        save_inventory(*item, ji);
        inv.push_back(ji);
    }
    if (!inv.empty()) {
        j["contents"] = inv;
    }
}

static void load_contents(Location& al, const json &j) {
    if (j.contains(+"contents")) {
        for (const auto &ji : j["contents"]) {
            auto item = std::make_shared<Object>();
            ji.get_to(*item);
            Object::registry.emplace(item->id, item);
            item->moveToLocation(al);
            load_inventory(*item, ji);
        }
    }
}


void load_npc_prototypes()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.nproto_blob"))
    {
        auto id = row["id"].as<int64_t>();
        auto n = std::make_shared<CharacterPrototype>();
        nlohmann::json j = nlohmann::json::parse(row["data"].as<std::string>());
        j.get_to(*n);
        auto p = mob_proto.emplace(id, n);
    }
}

void load_players()
{
    for(const auto& row : dbat::db::txn->exec("SELECT p.*,u.username,pd.data AS player_data FROM pcs AS p LEFT JOIN users AS u ON p.user_id=u.id LEFT JOIN pc_components AS pd ON pd.pc_id=p.id AND pd.component_type='dbat_player'")) {
        auto id = row["id"].as<std::string>();

        auto p = std::make_shared<PlayerData>();

        if(auto pdata = row["player_data"].get<std::string>(); pdata) {
            nlohmann::json j = nlohmann::json::parse(*pdata);
            from_json(j, *p);
        }

        p->id = id;
        p->name = row["name"].as<std::string>();
        p->user_id = row["user_id"].as<std::string>();
        p->username = row["username"].as<std::string>();
    }
}


void load_assemblies()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.assemblies_blob"))
    {
        auto vn = row["id"].as<int64_t>();
        nlohmann::json j = nlohmann::json::parse(row["data"].as<std::string>());
        
        assembly_data a;
        from_json(j, a);
        g_mAssemblyTable[vn] = std::move(a);
    }
}

