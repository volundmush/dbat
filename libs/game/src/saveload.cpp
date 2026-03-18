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
#include "dbat/game/Account.hpp"
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

// dump and load routines...
static void dump_to_file(const std::filesystem::path &loc, const std::string &name, const json &data)
{
    if (data.empty())
        return;
    // auto startTime = std::chrono::high_resolution_clock::now();
    std::ofstream file(loc / (name + ".gz"));
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(boost::iostreams::gzip_compressor());
    out.push(file);
    std::ostream outStream(&out);
    outStream << jdumps_pretty(data);

    // auto endTime = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration<double>(endTime - startTime).count();
    // basic_mud_log("Dumping %s to disk took %f seconds.", name, duration);
}

static json load_from_file(const std::filesystem::path &loc, const std::string &name)
{
    // We'll automatically append ".gz"
    auto path = loc / (name + ".gz");
    if (!std::filesystem::exists(path))
    {
        basic_mud_log("File %s does not exist", path.c_str());
        return {};
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        basic_mud_log("Error opening file %s", path.c_str());
        return {};
    }

    // Setup a Boost filtering stream for GZIP *de*compression
    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
    inbuf.push(boost::iostreams::gzip_decompressor());
    inbuf.push(file);

    // Construct an istream on top of that filtering streambuf
    std::istream instream(&inbuf);

    // Parse the JSON
    json j;
    try
    {
        instream >> j;
    }
    catch (const std::exception &e)
    {
        basic_mud_log("JSON parse error reading %s: %s", path.c_str(), e.what());
        return {};
    }
    return j;
}


void load_zones(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "zones.json"))
    {
        auto id = j.at(+"number").get<int64_t>();
        auto z = std::make_shared<Zone>();
        j.get_to(*z);
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

static json dump_zones()
{
    json j;

    for (auto &[v, z] : zone_table)
    {
        j.push_back(*z);
    }
    return j;
}


// account_data serialize/deserialize...
static json dump_accounts()
{
    json j;

    for (auto &[v, r] : accounts)
    {
        j.push_back(*r);
    }

    return j;
}

void load_accounts(const std::filesystem::path &loc)
{
    for (auto acc : load_from_file(loc, "accounts.json"))
    {
        auto vn = acc.at("id").get<int64_t>();
        auto a = std::make_shared<Account>();
        acc.get_to(*a);
        accounts.emplace(vn, a);
    }
}

void load_dgscript_prototypes(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "dgscript_prototypes.json"))
    {
        auto id = j["vn"].get<int64_t>();
        auto t = std::make_shared<DgScriptPrototype>();
        j.get_to(*t);
        trig_index.emplace(id, t);
    }
}

static json dump_dgscript_prototypes()
{
    json j;
    for (auto &[v, t] : trig_index)
    {
        j.push_back(*t);
    }
    return j;
}

void load_dgscripts(const std::filesystem::path &loc)
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

static json dump_dgscripts_characters()
{
    json j;

    for (auto &[id, u] : Character::registry)
    {
        if (u->scripts.empty())
            continue; // Skip units without scripts
        json j2;
        j2["id"] = id;
        for (auto &[vn, r] : u->scripts)
        {
            json j3;
            j3["vn"] = vn;
            j3["data"] = *r;
            j2["scripts"].push_back(j3);
        }
        j.push_back(j2);
    }
    return j;
}

static json dump_dgscripts_rooms()
{
    json j;

    for (auto &[id, u] : Room::registry)
    {
        if (u->scripts.empty())
            continue; // Skip units without scripts
        json j2;
        j2["vn"] = id;
        for (auto &[vn, r] : u->scripts)
        {
            json j3;
            j3["vn"] = vn;
            j3["data"] = *r;
            j2["scripts"].push_back(j3);
        }
        j.push_back(j2);
    }
    return j;
}

static json dump_dgscripts_objects()
{
    json j;

    for (auto &[id, u] : Object::registry)
    {
        if (u->scripts.empty())
            continue; // Skip units without scripts
        json j2;
        j2["id"] = id;
        for (auto &[vn, r] : u->scripts)
        {
            json j3;
            j3["vn"] = vn;
            j3["data"] = *r;
            j2["scripts"].push_back(j3);
        }
        j.push_back(j2);
    }
    return j;
}

// org_data...
// shops serialize/deserialize...
void load_shops(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "shops.json"))
    {
        auto id = j["vnum"].get<int64_t>();
        auto s = std::make_shared<Shop>();
        j.get_to(*s);
        shop_index.emplace(id, s);
    }
}

static json dump_shops()
{
    json j;
    for (auto &[v, s] : shop_index)
    {
        j.push_back(*s);
    }
    return j;
}

// guilds serialize/deserialize...
static json dump_guilds()
{
    json j;
    for (auto &[v, g] : guild_index)
    {
        j.push_back(*g);
    }
    return j;
}

void load_guilds(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "guilds.json"))
    {
        auto id = j["vnum"].get<int64_t>();
        auto g = std::make_shared<Guild>();
        j.get_to(*g);
        guild_index.emplace(id, g);
    }
}

// globaldata serialize/deserialize...
void load_globaldata(const std::filesystem::path &loc)
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
    if (j.contains(+"lastCharacterID"))
    {
        j["lastCharacterID"].get_to(Character::lastID);
    }
    if (j.contains(+"lastAccountID"))
    {
        j["lastAccountID"].get_to(lastAccountID);
    }
    if (j.contains(+"lastObjectID"))
    {
        j["lastObjectID"].get_to(Object::lastID);
    }
    if (j.contains(+"lastStructureID"))
    {
        j["lastStructureID"].get_to(lastStructureID);
    }
    if (j.contains(+"lastGridTemplateID"))
    {
        j["lastGridTemplateID"].get_to(lastGridTemplateID);
    }
    if (j.contains(+"lastAreaID"))
    {
        j["lastAreaID"].get_to(lastAreaID);
    }
    if (j.contains(+"lastRoomID"))
    {
        j["lastRoomID"].get_to(Room::lastID);
    }
    if (j.contains(+"lastShopID"))
    {
        j["lastShopID"].get_to(lastShopID);
    }
    if (j.contains(+"lastGuildID"))
    {
        j["lastGuildID"].get_to(lastGuildID);
    }

    if (j.contains(+"lastScriptID"))
    {
        j["lastScriptID"].get_to(lastScriptID);
    }

}

json dump_globaldata()
{
    json j;

    j["time"] = time_info;
    j["era_uptime"] = era_uptime;
    j["weather"] = weather_info;
    j["lastCharacterID"] = Character::lastID;
    j["lastAccountID"] = lastAccountID;
    j["lastObjectID"] = Object::lastID;
    j["lastStructureID"] = lastStructureID;
    j["lastAreaID"] = lastAreaID;
    j["lastGridTemplateID"] = lastGridTemplateID;
    j["lastRoomID"] = Room::lastID;
    j["lastZoneID"] = lastZoneID;
    j["lastShopID"] = lastShopID;
    j["lastGuildID"] = lastGuildID;
    j["lastScriptID"] = lastScriptID;

    return j;
}

void load_rooms(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "rooms.json"))
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

void load_areas_initial(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "areas.json"))
    {
        auto vn = j["vn"].get<int>();
        auto r = std::make_shared<Area>();
        j.get_to(*r);
        areas.emplace(vn, r);
        r->rebuildShapeIndex();
        auto z = r->getZone();
        z->areas.add(r);
    }
}

void load_areas_finish(const std::filesystem::path &loc)
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

void load_grid_templates(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "grid_templates.json"))
    {
        auto vn = j["vn"].get<int>();
        auto t = std::make_shared<GridTemplate>();
        j.get_to(*t);
        auto p = gridTemplates.emplace(vn, t);
    }
}

void load_structures_initial(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "structures.json"))
    {
        auto id = j["id"].get<int64_t>();
        auto r = std::make_shared<Structure>();
        j.get_to(*r);
        Structure::registry.emplace(id, r);
        r->rebuildShapeIndex();
    }
}

void load_structures_finish(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "structures.json"))
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

void load_exits(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "exits.json"))
    {
        auto id = j["room"].get<int64_t>();
        auto dir = j["direction"].get<Direction>();
        auto r = get_room(id);
        auto &ex = r->exits[dir];
        j["data"].get_to(ex);
    }
}

static json dump_exits()
{
    json exits;

    for (auto &[v, r] : Room::registry)
    {

        for (auto &[d, e] : r->getDirections())
        {
            json j2;
            j2["room"] = v;
            j2["direction"] = d;
            j2["data"] = e;
            exits.push_back(j2);
        }
    }
    return exits;
}

static json dump_rooms()
{
    json rooms;

    for (auto &[v, r] : Room::registry)
    {
        rooms.push_back(*r);
    }
    return rooms;
}

static json dump_grid_templates()
{
    json jdata;

    for (auto &[v, r] : gridTemplates)
    {
        jdata.push_back(*r);
    }
    return jdata;
}

static json dump_areas()
{
    json jdata;

    for (auto &[v, r] : areas)
    {
        jdata.push_back(*r);
    }
    return jdata;
}

static json dump_structures()
{
    json jdata;

    for (auto &[v, r] : Structure::registry)
    {
        auto j = json::object();
        auto j3 = json::object();
        j["id"] = r->id;
        j["data"] = *r;
        to_json(j3, static_cast<const HasLocation&>(*r));
        j["HasLocation"] = j3;
        jdata.push_back(j);
    }
    return jdata;
}

// Object serialize/deserialize...

;

void load_item_prototypes(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "item_prototypes.json"))
    {
        auto id = j["vn"].get<int64_t>();
        auto i = std::make_shared<ObjectPrototype>();
        j.get_to(*i);
        auto p = obj_proto.emplace(id, i);
    }
}

void load_items_initial(const std::filesystem::path &loc)
{
    for (const auto j : load_from_file(loc, "items.json"))
    {
        auto id = j["id"].get<int64_t>();
        auto data = j["data"];
        auto sh = std::make_shared<Object>();
        data.get_to(*sh);
        Object::registry.emplace(id, sh);
    }
    basic_mud_log("Loaded %d items", Object::registry.size());
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

static json serialize_obj_relations(const Object *o)
{
    auto j = json::object();

    if (o->posted_to)
        j["posted_to"] = o->posted_to->id;
    if (o->fellow_wall)
        j["fellow_wall"] = o->fellow_wall->id;
    if (auto c = o->getContainer())
    {
        j["container"] = c->id;
    }
    else if (auto c = o->getCarriedBy())
    {
        j["carried_by"] = c->id;
    }
    else if (auto c = o->getWornBy())
    {
        j["worn_by"] = c->id;
        j["worn_on"] = o->worn_on;
    }

    return j;
}

static void deserialize_obj_relations(Object *o, const json &j)
{
    if (j.contains(+"posted_to"))
    {
        auto check = Object::registry.find(j["posted_to"].get<int64_t>());
        if (check != Object::registry.end())
            o->posted_to = check->second.get();
    }
    if (j.contains(+"fellow_wall"))
    {
        auto check = Object::registry.find(j["fellow_wall"].get<int>());
        if (check != Object::registry.end())
            o->fellow_wall = check->second.get();
    }

    if (j.contains(+"container"))
    {
        auto check = Character::registry.find(j["container"].get<int>());
        if (check != Character::registry.end())
            check->second->addToInventory(o);
    }
    else if (j.contains(+"carried_by"))
    {
        auto check = Character::registry.find(j["carried_by"].get<int>());
        if (check != Character::registry.end())
            check->second->addToInventory(o);
    }
    else if (j.contains(+"worn_by"))
    {
        auto check = Character::registry.find(j["worn_by"].get<int>());
        if (check != Character::registry.end())
        {
            check->second->addToEquip(o, j["worn_on"].get<int>());
        }
    }
}

void load_items_finish(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "items.json"))
    {
        auto id = j["id"].get<int>();
        if (auto cf = Object::registry.find(id); cf != Object::registry.end())
        {
            if (auto i = cf->second)
            {
                if(j.contains(+"relations"))
                    deserialize_obj_relations(i.get(), j["relations"]);
                if(j.contains(+"HasLocation")) {
                    from_json(j["HasLocation"], *i);
                    Location l = i->location;
                    if(l) {
                        cf->second->moveToLocation(l);
                    }
                }
            }
        }
    }
}

static json dump_items()
{
    json j;

    for (auto &[v, r] : Object::registry)
    {
        json j2;
        auto j3 = json::object();
        j2["id"] = v;
        j2["data"] = *r;
        j2["relations"] = serialize_obj_relations(r.get());
        to_json(j3, static_cast<const HasLocation&>(*r));
        j2["HasLocation"] = j3;
        j.push_back(j2);
    }
    return j;
}

static json dump_item_prototypes()
{
    json j;
    for (auto &[v, o] : obj_proto)
    {
        j.push_back(*o);
    }
    return j;
}

static json dump_characters()
{
    json j = json::array();

    for (auto &[v, r] : Character::registry)
    {
        json j2 = json::object();
        auto j3 = json::object();
        j2["id"] = v;
        j2["data"] = *r;

        to_json(j3, static_cast<const HasLocation&>(*r));
        j2["HasLocation"] = j3;
        j.push_back(j2);
    }
    return j;
}

static json dump_npc_prototypes()
{
    json j;

    for (auto &[v, n] : mob_proto)
    {
        j.push_back(*n);
    }
    return j;
}

void load_characters_initial(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "characters.json"))
    {
        auto id = j["id"].get<int64_t>();
        auto data = j["data"];
        auto c = std::make_shared<Character>();
        j["data"].get_to(*c);
        if (auto isPlayer = players.find(id); isPlayer != players.end())
        {
            isPlayer->second->character = c.get();
        }
        Character::registry.emplace(id, c);
    }
}

void load_characters_finish(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "characters.json"))
    {
        auto id = j["id"].get<int64_t>();
        if(!j.contains(+"HasLocation")) continue;

        // basic_mud_log("Finishing Character %d", id);
        if (auto cf = Character::registry.find(id); cf != Character::registry.end())
        {
            auto ch = cf->second;
            auto hl = j["HasLocation"];
            hl.get_to(static_cast<HasLocation&>(*ch));
        }
    }
}

void load_npc_prototypes(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "npc_prototypes.json"))
    {
        auto id = j["vn"].get<int64_t>();
        if (id <= 0)
        {
            throw std::runtime_error("Invalid NPC prototype vnum: " + std::to_string(id));
        }
        auto n = std::make_shared<CharacterPrototype>();
        j.get_to(*n);
        auto p = mob_proto.emplace(id, n);
    }
}

// players data serialize/deserialize...
static json dump_players()
{
    json j = json::array();

    for (auto &[v, r] : players)
    {
        auto j2 = json::object();
        to_json(j2, *r);

        auto j3 = json::object();
        to_json(j3, *r->character);
        save_inventory(*r->character, j3);
        save_equipment(*r->character, j3);

        j2["character"] = j3;

        j.push_back(j2);
    }
    return j;
}

void load_players(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "players.json"))
    {
        auto id = j["id"].get<int64_t>();
        auto p = std::make_shared<PlayerData>();
        j.get_to(*p);
        players.emplace(id, p);

        auto ch = std::make_shared<Character>();
        j["character"].get_to(*ch);
        Character::registry.emplace(ch->id, ch);
        p->character = ch.get();
        load_inventory(*ch, j["character"]);
        load_equipment(*ch, j["character"]);
    }
}

std::vector<std::filesystem::path> getDumpFiles(const std::filesystem::path &dir, std::string_view pattern)
{
    std::vector<std::filesystem::path> directories;

    for (const auto &entry : std::filesystem::directory_iterator(dir))
    {
        if (entry.is_directory() && entry.path().filename().string().starts_with(pattern))
        {
            directories.push_back(entry.path());
        }
    }

    std::sort(directories.begin(), directories.end(), std::greater<>());
    return directories;
}

static void cleanup_state(const std::filesystem::path &dir, std::string_view pattern)
{
    auto vecFiles = getDumpFiles(dir, pattern);
    std::list<std::filesystem::path> files(vecFiles.begin(), vecFiles.end());

    // If we have more than x state files, we want to purge the oldest one(s) until we have just x.
    while (files.size() > 200)
    {
        std::filesystem::remove_all(files.back());
        files.pop_back();
    }
}


static json dump_help()
{
    auto j = json::array();
    to_json(j, help_table);
    return j;
}

void load_help(const std::filesystem::path &loc)
{
    auto data = load_from_file(loc, "help.json");
    data.get_to(help_table);
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

void load_assemblies(const std::filesystem::path &loc)
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

static json dump_save_rooms() {
    json j = json::array();

    for (auto &[v, r] : Room::registry) {
        //j.push_back(*r);
    }
    return j;
}

namespace dbat::save {

    struct SaveTask {
        std::string filename;
        std::function<json()> task;
    };

    static const std::vector<SaveTask>& getSaveUserTasks() {
        static const std::vector<SaveTask> tasks = {
            {"accounts", dump_accounts},
            {"players", dump_players},
            // TODO: Add structures to replace saverooms.
            {"saverooms", dump_save_rooms}
        };
        return tasks;
    }

    static const std::vector<SaveTask>& getSaveAssetTasks() {
        static const std::vector<SaveTask> tasks = {
            {"help", dump_help},
            {"assemblies", dump_assemblies},
            {"globaldata", dump_globaldata},
            {"grid_templates", dump_grid_templates},
            {"rooms", dump_rooms},
            {"exits", dump_exits},
            {"areas", dump_areas},
            {"item_prototypes", dump_item_prototypes},
            {"npc_prototypes", dump_npc_prototypes},
            {"shops", dump_shops},
            {"guilds", dump_guilds},
            {"zones", dump_zones},
            {"structures", dump_structures},
            {"dgscript_prototypes", dump_dgscript_prototypes}
        };
        return tasks;
    }

    static std::string generateSaveLocation(const std::chrono::_V2::system_clock::time_point now, std::string_view prefix) {

        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&time_t_now);
        return fmt::format("{}{:04}{:02}{:02}{:02}{:02}{:02}",
                           prefix,
                           tm_now.tm_year + 1900,
                           tm_now.tm_mon + 1,
                           tm_now.tm_mday,
                           tm_now.tm_hour,
                           tm_now.tm_min,
                           tm_now.tm_sec);
    }

    static void runSaveSyncHelper(const std::chrono::_V2::system_clock::time_point now, const std::vector<SaveTask>& tasks, std::string_view folder, std::string_view prefix)
    {
        LINFO("Beginning dump of {} to disk.", folder);
        // Open up a new database file as <cwd>/state/<timestamp>.sqlite3 and dump the state into it.
        auto path = std::filesystem::current_path() / "data" / "dumps" / folder;
        auto newPath = path / dbat::save::generateSaveLocation(now, prefix);
        std::filesystem::create_directories(path);

        auto tempPath = path / "temp";
        std::filesystem::remove_all(tempPath);
        std::filesystem::create_directories(tempPath);

        double duration{};
        bool failed = false;
        try
        {
            auto startTime = std::chrono::high_resolution_clock::now();

            std::exception_ptr firstException;
            std::mutex exceptionMutex;
            std::atomic<bool> failedTask{false};

            std::for_each(std::execution::par, tasks.begin(), tasks.end(), [&](const SaveTask &entry)
            {
                if (failedTask.load(std::memory_order_relaxed))
                {
                    return;
                }

                try
                {
                    auto data = entry.task();
                    dump_to_file(tempPath, entry.filename + ".json", data);
                }
                catch (const std::exception &e)
                {
                    LERROR("Error during dump: %s", e.what());
                    failedTask.store(true, std::memory_order_relaxed);
                    std::lock_guard<std::mutex> lock(exceptionMutex);
                    if (!firstException)
                    {
                        firstException = std::current_exception();
                    }
                }
            });

            if (firstException)
            {
                std::rethrow_exception(firstException);
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration<double>(endTime - startTime).count();
        }
        catch (std::exception &e)
        {
            LERROR("(GAME HAS NOT BEEN SAVED!) Exception in dump_state(): %s", e.what());
            send_to_all("Warning, a critical error occurred during save! Please alert staff!\r\n");
            failed = true;
        }
        if (failed) {
            std::filesystem::remove_all(tempPath);
            return;
        }

        std::filesystem::rename(tempPath, newPath);
        LINFO("Finished dumping state to {} in {} seconds.", newPath.string(), duration);
        cleanup_state(path, prefix);
    }

    void runSaveSync() {
        auto now = std::chrono::system_clock::now();
        runSaveSyncHelper(now, getSaveUserTasks(), "user", "user-");
        runSaveSyncHelper(now, getSaveAssetTasks(), "assets", "assets-");
    }

}


void rest_post_script(int accountID, int scriptID, const std::string &data)
{
    auto &acc = accounts.at(accountID);
    json j;
    try
    {
        j = json::parse(data);
    }
    catch (const json::parse_error &e)
    {
        basic_mud_log("Error parsing JSON data for script %d: %s", scriptID, e.what());
        return;
    }
    if (trig_index.contains(scriptID))
    {
        auto &trig = trig_index.at(scriptID);
        // TODO: Post the data to the trigger

        basic_mud_log("%s updated DgScript %d: '%s'", acc->name.c_str(), scriptID, trig->name);
    }
    else
    {
        auto t = std::make_shared<DgScriptPrototype>();
        j.get_to(*t);
        trig_index.emplace(scriptID, t);
        basic_mud_log("%s created DgScript %d: '%s'", acc->name.c_str(), scriptID, t->name);
    }
}
