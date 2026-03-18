#include "dbat/game/saveload.hpp"

#include <fstream>
#include <chrono>
#include <execution>
#include <mutex>
#include <atomic>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>

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
        auto id = row.get<int64_t>(0);
        auto z = std::make_shared<Zone>();
        json::from_json(row.get<std::string>(1), *z);
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
        auto id = j["vn"].get<int64_t>();
        auto t = std::make_shared<DgScriptPrototype>();
        j.get_to(*t);
        trig_index.emplace(id, t);
    }
}

void load_dgscripts()
{
    for (auto j : load_from_file(loc, "dgscripts_characters.json"))
    {
        auto id = j["id"].get<int64_t>();

        auto u = Character::registry.at(id);

        for (auto d : j["scripts"])
        {
            auto vn = d["vn"].get<int>();
            auto r = std::make_shared<DgScript>();
            d["data"].get_to(*r);
            u->scripts.emplace(vn, r);
            r->owner.reset(u.get());
            u->trigger_types |= GET_TRIG_TYPE(r);
        }
    }

    for (auto j : load_from_file(loc, "dgscripts_objects.json"))
    {
        auto id = j["id"].get<int64_t>();

        auto u = Object::registry.at(id);

        for (auto d : j["scripts"])
        {
            auto vn = d["vn"].get<int>();
            auto r = std::make_shared<DgScript>();
            d["data"].get_to(*r);
            u->scripts.emplace(vn, r);
            r->owner.reset(u.get());
            u->trigger_types |= GET_TRIG_TYPE(r);
        }
    }

    for (auto j : load_from_file(loc, "dgscripts_rooms.json"))
    {
        auto id = j["vn"].get<int64_t>();

        auto u = Room::registry.at(id).get();

        for (auto d : j["scripts"])
        {
            auto vn = d["vn"].get<int>();
            auto r = std::make_shared<DgScript>();
            d["data"].get_to(*r);
            u->scripts.emplace(vn, r);
            r->owner.reset(u);
            u->trigger_types |= GET_TRIG_TYPE(r);
        }
    }
}




// org_data...
// shops serialize/deserialize...
void load_shops()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.shops_blob"))
    {
        auto id = j["vnum"].get<int64_t>();
        auto s = std::make_shared<Shop>();
        j.get_to(*s);
        shop_index.emplace(id, s);
    }
}

void load_guilds()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.guilds_blob"))
    {
        auto id = j["vnum"].get<int64_t>();
        auto g = std::make_shared<Guild>();
        j.get_to(*g);
        guild_index.emplace(id, g);
    }
}

// globaldata serialize/deserialize...
void load_globaldata()
{
    auto j = load_from_file(loc, "globaldata.json");

    if (j.contains(+"time"))
    {
        j["time"].get_to(time_info);
    }
    if (j.contains(+"era_uptime"))
        j["era_uptime"].get_to(era_uptime);
    if (j.contains(+"weather"))
    {
        j["weather"].get_to(weather_info);
    }

}

json dump_globaldata()
{
    json j;

    j["time"] = time_info;
    j["era_uptime"] = era_uptime;
    j["weather"] = weather_info;

    return j;
}

void load_rooms()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.rooms_blob"))
    {
        auto vn = j["vn"].get<int64_t>();
        auto r = std::make_shared<Room>();
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
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.areas_blob"))
        auto vn = j["vn"].get<int>();
        auto r = std::make_shared<Area>();
        j.get_to(*r);
        areas.emplace(vn, r);
        r->rebuildShapeIndex();
        auto z = r->getZone();
        z->areas.add(r);
    }
}

void load_areas_finish()
{
    for (auto j : load_from_file(loc, "areas.json"))
    {
        if(!j.contains(+"tileOverrides"))
            continue;

        auto vn = j["vn"].get<int>();
        if(auto cf = areas.find(vn); cf != areas.end())
        {
            auto &to = cf->second->tileOverrides;
            j.at(+"tileOverrides").get_to(to);
        }

    }
}

void load_grid_templates()
{
    for (auto j : load_from_file(loc, "grid_templates.json"))
    {
        auto vn = j["vn"].get<int>();
        auto t = std::make_shared<GridTemplate>();
        j.get_to(*t);
        auto p = gridTemplates.emplace(vn, t);
    }
}

void load_structures_initial()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.structures_blob"))
    {
        auto id = j["id"].get<int64_t>();
        auto r = std::make_shared<Structure>();
        j.get_to(*r);
        Structure::registry.emplace(id, r);
        r->rebuildShapeIndex();
    }
}

void load_structures_finish()
{
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.structures_blob"))
    {
        auto id = j["id"].get<int>();

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
    for (const auto& row : dbat::db::txn->exec("SELECT * FROM dbat.rooms_blob"))
    {
        auto id = j["room"].get<int64_t>();
        auto dir = j["direction"].get<Direction>();
        auto r = get_room(id);
        auto &ex = r->exits[dir];
        j["data"].get_to(ex);
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
        p->id = id;
        p->name = row["name"].as<std::string>();
        p->user_id = row["user_id"].as<std::string>();
        p->username = row["username"].as<std::string>();

        if(row["player_data"]) {
            nlohmann::json j = nlohmann::json::parse(row["player_data"].as<std::string>());
            j.get_to(*p);
        }
    }
}

static json dump_assemblies()
{
    auto j = json::array();

    if (g_pAssemblyTable)
    {
        for (auto i = 0; i < g_lNumAssemblies; i++)
        {
            auto a = json::object();
            a["vnum"] = g_pAssemblyTable[i].lVnum;
            a["assembly_type"] = g_pAssemblyTable[i].uchAssemblyType;

            for (auto k = 0; k < g_pAssemblyTable[i].lNumComponents; k++)
            {
                auto comp = json::object();
                comp["bExtract"] = g_pAssemblyTable[i].pComponents[k].bExtract;
                comp["bInRoom"] = g_pAssemblyTable[i].pComponents[k].bInRoom;
                comp["lVnum"] = g_pAssemblyTable[i].pComponents[k].lVnum;
                a["components"].push_back(comp);
            }
            j.push_back(a);
        }
    }

    return j;
}

void load_assemblies()
{
    auto data = load_from_file(loc, "assemblies.json");

    for (auto j : data)
    {
        auto vn = j.at(+"vnum").get<vnum>();
        auto atype = j.at(+"assembly_type").get<int>();
        assemblyCreate(vn, atype);
        if (j.contains(+"components"))
        {
            // components are an array containing bExtract, bInRoom, and vnum lVnum...
            for (auto comp : j.at(+"components"))
            {
                auto bExtract = comp.at("bExtract").get<bool>();
                auto bInRoom = comp.at("bInRoom").get<bool>();
                auto lVnum = comp.at("lVnum").get<vnum>();
                assemblyAddComponent(vn, lVnum, bExtract, bInRoom);
            }
        }
    }
}

