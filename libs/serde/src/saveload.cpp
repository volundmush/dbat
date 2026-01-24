#include <fstream>
#include <thread>
#include <chrono>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "dbat/serde/json.hpp"
#include "dbat/serde/templates.hpp"
#include "dbat/serde/saveload.hpp"

#include "volcano/util/FilterWeak.hpp"
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

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(time_data, birth, created, maxage, logon, played, seconds_aged)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(time_info_data, remainder, seconds, minutes, hours, day, month, year)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(weather_data, pressure, change, sky, sunlight)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(shop_buy_data, type, keywords)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Coordinates, x, y, z)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HasStats, stats)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HasID, id)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HasExtraDescriptions, extra_descriptions)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HasMudStrings, name, look_description, short_description, room_description)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HasVnum, vn)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HasVariables, variables)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Location, locationID, position)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HasLocation, location, registeredLocations)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ResetCommand, type, if_flag, target, max, max_location, ex, chance, key, value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HasResetCommands, resetCommands)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Zone, number, parent, name, builders, lifespan, age, reset_mode, zone_flags, launchDestination, landingSpots, dockingSpots)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(affect_t, location, modifier, specific)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RoundDim, center, radius, zMin, zMax, r2)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AABB, min, max)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BoxDim, box)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(skill_data, level, perfs)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(alias_data, name, replacement, type)


void to_json(json& j, const HasDgScripts& p);
void from_json(const json& j, HasDgScripts& p);

void to_json(json& j, const HasZone& p);
void from_json(const json& j, HasZone& p);

void to_json(json& j, const trans_data& t);
void from_json(const json& j, trans_data& t);

void to_json(json& j, const affected_type& a);
void from_json(const json& j, affected_type& a);

void to_json(json& j, const ShapeBase& p);
void from_json(const json& j, ShapeBase& p);

void to_json(json& j, const Shape& p);
void from_json(const json& j, Shape& p);

// helper type save/load...
template <std::size_t N>
void to_json(json &j, const std::bitset<N> &bs)
{
    std::vector<std::size_t> set_indexes;
    for (std::size_t i = 0; i < N; ++i)
    {
        if (bs.test(i))
        {
            set_indexes.push_back(i);
        }
    }
    j = set_indexes;
}

template <std::size_t N>
void from_json(const json &j, std::bitset<N> &bs)
{
    bs.reset();
    // Extract the vector of indexes from JSON
    auto indexes = j.get<std::vector<std::size_t>>();
    for (auto idx : indexes)
    {
        if (idx < N)
        { // ensure the index is within range
            bs.set(idx, true);
        }
    }
}

/*
void to_json(json &j, const Zone &z)
{
    j["number"] = z.number;
    if (z.parent)
        j["parent"] = z.parent.value();
    if (!z.name.empty())
        j["name"] = z.name;
    if (!z.builders.empty())
        j["builders"] = z.builders;
    if (z.lifespan)
        j["lifespan"] = z.lifespan;

    if (z.reset_mode)
        j["reset_mode"] = z.reset_mode;

    // Serialize zone flags
    if (z.zone_flags)
        j["zone_flags"] = z.zone_flags;
}

void from_json(const json &j, Zone &z)
{
    if (j.contains("number"))
        z.number = j["number"];
    if (j.contains("parent"))
        z.parent = j["parent"].get<zone_vnum>();
    if (j.contains("name"))
        z.name = j["name"].get<std::string>();
    if (j.contains("builders"))
        z.builders = j["builders"].get<std::string>();
    if (j.contains("lifespan"))
        z.lifespan = j["lifespan"];
    if (j.contains("reset_mode"))
        z.reset_mode = j["reset_mode"];

    if (j.contains("zone_flags"))
        z.zone_flags = j["zone_flags"].get<FlagHandler<ZoneFlag>>();
}
*/
void load_zones(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "zones.json"))
    {
        auto id = j.at("number").get<int64_t>();
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
void to_json(json &j, const Account &a)
{
    if (a.id != NOTHING)
        j["id"] = a.id;
    if (!a.name.empty())
        j["name"] = a.name;
    if (!a.password.empty())
        j["password"] = a.password;
    if (!a.email.empty())
        j["email"] = a.email;
    if (a.created)
        j["created"] = a.created;
    if (a.last_login)
        j["last_login"] = a.last_login;
    if (a.last_logout)
        j["last_logout"] = a.last_logout;
    if (a.last_change_password)
        j["last_change_password"] = a.last_change_password;
    if (a.playtime != 0.0)
        j["playtime"] = a.playtime;
    if (!a.disabled_reason.empty())
        j["disabled_reason"] = a.disabled_reason;
    if (a.disabled_until)
        j["disabled_until"] = a.disabled_until;
    if (a.rpp)
        j["rpp"] = a.rpp;
    if (a.slots != 3)
        j["slots"] = a.slots;
    if (a.admin_level)
        j["admin_level"] = a.admin_level;

    // For the characters field (vector<int>), assign it directly.
    j["characters"] = a.characters;
}

void from_json(const json &j, Account &a)
{
    if (j.contains("id"))
        a.id = j["id"];
    if (j.contains("name"))
        a.name = j["name"];
    if (j.contains("password"))
        a.password = j["password"];
    if (j.contains("email"))
        a.email = j["email"];
    if (j.contains("created"))
        a.created = j["created"];
    if (j.contains("lastLogin"))
        a.last_login = j["lastLogin"];
    if (j.contains("lastLogout"))
        a.last_logout = j["lastLogout"];
    if (j.contains("last_change_password"))
        a.last_change_password = j["last_change_password"];
    if (j.contains("playtime"))
        a.playtime = j["playtime"];
    if (j.contains("disabled_reason"))
        a.disabled_reason = j["disabled_reason"];
    if (j.contains("disabled_until"))
        a.disabled_until = j["disabled_until"];
    if (j.contains("rpp"))
        a.rpp = j["rpp"];
    if (j.contains("slots"))
        a.slots = j["slots"];
    if (j.contains("admin_level"))
        a.admin_level = j["admin_level"];

    // Directly deserialize the vector<int> field.
    if (j.contains("characters"))
        a.characters = j["characters"].get<std::vector<int>>();
}

void to_json(json &j, const help_index_element &a)
{
    j["index"] = a.index;
    if (!a.keywords.empty())
        j["keywords"] = a.keywords;
    if (!a.entry.empty())
        j["entry"] = a.entry;
    if (a.duplicate != NOTHING)
        j["duplicate"] = a.duplicate;
    if (a.min_level != 0)
        j["min_level"] = a.min_level;
}

void from_json(const json &j, help_index_element &a)
{
    if (j.contains("index")) j.at("index").get_to(a.index);
    if (j.contains("keywords")) j.at("keywords").get_to(a.keywords);
    if (j.contains("entry")) j.at("entry").get_to(a.entry);
    if (j.contains("duplicate")) j.at("duplicate").get_to(a.duplicate);
    if (j.contains("min_level")) j.at("min_level").get_to(a.min_level);
}

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

void to_json(json &j, const DgScriptPrototype &t)
{
    j["vn"] = t.vn;
    j["name"] = t.name;
    j["attach_type"] = t.attach_type;
    j["trigger_type"] = t.trigger_type;
    j["narg"] = t.narg;
    j["arglist"] = t.arglist;

    j["body"] = t.scriptString();
}

void from_json(const json &j, DgScriptPrototype &t)
{
    if (j.contains("vn"))
        t.vn = j["vn"].get<int>();
    if (j.contains("name"))
        t.name = strdup(j["name"].get<std::string>().c_str());
    if (j.contains("attach_type"))
        t.attach_type = j["attach_type"].get<UnitType>();
    if (j.contains("trigger_type"))
        t.trigger_type = j["trigger_type"].get<int>();
    if (j.contains("narg"))
        t.narg = j["narg"].get<int>();
    if (j.contains("arglist"))
        t.arglist = strdup(j["arglist"].get<std::string>().c_str());

    if (j.contains("body"))
    {
        t.setBody(j["body"].get<std::string>());
    }
}

void to_json(json &j, const DgScript &t)
{
    j["vn"] = t.getVnum();
    j["current_line"] = t.current_line;
    j["state"] = t.state;
    j["depth_stack"] = t.depth_stack;
    if (t.waiting != 0.0)
        j["waiting"] = t.waiting;
    if (!t.variables.empty())
        j["variables"] = t.variables;
}

void from_json(const json &j, DgScript &t)
{
    auto vn = j["vn"].get<int>();
    t.proto = trig_index.at(vn).get();

    if (j.contains("state"))
        t.state = j["state"].get<DgScriptState>();
    if (j.contains("waiting"))
        t.waiting = j["waiting"].get<double>();
    if (j.contains("depth_stack"))
        t.depth_stack = j["depth_stack"].get<std::vector<DepthType>>();

    if (j.contains("variables"))
    {
        t.variables = j["variables"].get<std::unordered_map<std::string, std::string>>();
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
void to_json(json &j, const picky_data &p)
{
    if (p.not_alignment) j["not_alignment"] = p.not_alignment;
    if (p.not_race) j["not_race"] = p.not_race;
    if (p.only_race) j["only_race"] = p.only_race;
    if (p.not_sensei) j["not_sensei"] = p.not_sensei;
    if (p.only_sensei) j["only_sensei"] = p.only_sensei;
}

void from_json(const json &j, picky_data &p)
{
    if (j.contains("not_alignment")) j.at("not_alignment").get_to(p.not_alignment);
    if (j.contains("not_race")) j.at("not_race").get_to(p.not_race);
    if (j.contains("only_race")) j.at("only_race").get_to(p.only_race);
    if (j.contains("not_sensei")) j.at("not_sensei").get_to(p.not_sensei);
    if (j.contains("only_sensei")) j.at("only_sensei").get_to(p.only_sensei);
}

void to_json(json &j, const org_data &o)
{
    to_json(j, static_cast<picky_data>(o));
    j["vnum"] = o.vnum;

    if (o.keeper != NOTHING)
        j["keeper"] = o.keeper;
}

void from_json(const json &j, org_data &o)
{
    from_json(j, static_cast<picky_data &>(o));
    if (j.contains("vnum"))
        o.vnum = j["vnum"];

    if (j.contains("keeper"))
        o.keeper = j["keeper"];
}

// shops serialize/deserialize...
void to_json(json &j, const Shop &s)
{
    to_json(j, static_cast<org_data>(s));

    if (!s.producing.empty())
        j["producing"] = s.producing;
    if (s.profit_buy)
        j["profit_buy"] = s.profit_buy;
    if (s.profit_sell)
        j["profit_sell"] = s.profit_sell;
    if (!s.type.empty())
        j["type"] = s.type;
    if (!s.no_such_item1.empty())
        j["no_such_item1"] = s.no_such_item1;
    if (!s.no_such_item2.empty())
        j["no_such_item2"] = s.no_such_item2;
    if (!s.missing_cash1.empty())
        j["missing_cash1"] = s.missing_cash1;
    if (!s.missing_cash2.empty())
        j["missing_cash2"] = s.missing_cash2;
    if (!s.do_not_buy.empty())
        j["do_not_buy"] = s.do_not_buy;
    if (!s.message_buy.empty())
        j["message_buy"] = s.message_buy;
    if (!s.message_sell.empty())
        j["message_sell"] = s.message_sell;
    if (s.temper1)
        j["temper1"] = s.temper1;
    j["shop_flags"] = s.shop_flags;
    for (auto r : s.in_room)
        j["in_room"].push_back(r);
    if (s.open1)
        j["open1"] = s.open1;
    if (s.close1)
        j["close1"] = s.close1;
    if (s.open2)
        j["open2"] = s.open2;
    if (s.close2)
        j["close2"] = s.close2;
    if (s.bankAccount)
        j["bankAccount"] = s.bankAccount;
    if (s.lastsort)
        j["lastsort"] = s.lastsort;
}

void from_json(const json &j, Shop &s)
{
    from_json(j, static_cast<org_data &>(s));
    if (j.contains("producing"))
        s.producing = j["producing"].get<std::vector<int>>();
    if (j.contains("profit_buy"))
        s.profit_buy = j["profit_buy"];
    if (j.contains("profit_sell"))
        s.profit_sell = j["profit_sell"];
    if (j.contains("type")) j.at("type").get_to(s.type);
    if (j.contains("no_such_item1"))
        j.at("no_such_item1").get_to(s.no_such_item1);
    if (j.contains("no_such_item2"))
        j.at("no_such_item2").get_to(s.no_such_item2);
    if (j.contains("missing_cash1"))
        j.at("missing_cash1").get_to(s.missing_cash1);
    if (j.contains("missing_cash2"))
        j.at("missing_cash2").get_to(s.missing_cash2);
    if (j.contains("do_not_buy"))
        j.at("do_not_buy").get_to(s.do_not_buy);
    if (j.contains("message_buy"))
        j.at("message_buy").get_to(s.message_buy);
    if (j.contains("message_sell"))
        j.at("message_sell").get_to(s.message_sell);
    if (j.contains("temper1"))
        s.temper1 = j["temper1"];

    if (j.contains("shop_flags"))
        s.shop_flags = j["shop_flags"].get<FlagHandler<ShopFlag>>();

    if (j.contains("in_room"))
    {
        for (auto &r : j["in_room"])
        {
            s.in_room.insert(r.get<int>());
        }
    }
    if (j.contains("open1"))
        s.open1 = j["open1"];
    if (j.contains("close1"))
        s.close1 = j["close1"];
    if (j.contains("open2"))
        s.open2 = j["open2"];
    if (j.contains("close2"))
        s.close2 = j["close2"];
    if (j.contains("bankAccount"))
        s.bankAccount = j["bankAccount"];
    if (j.contains("lastsort"))
        s.lastsort = j["lastsort"];
}

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
void to_json(json &j, const Guild &g)
{
    to_json(j, static_cast<org_data>(g));
    if (g.skills)
        j["skills"] = g.skills;
    if (!g.feats.empty())
        j["feats"] = g.feats;
    if (g.charge != 1.0)
        j["charge"] = g.charge;
    if (!g.no_such_skill.empty())
        j["no_such_skill"] = g.no_such_skill;
    if (!g.not_enough_gold.empty())
        j["not_enough_gold"] = g.not_enough_gold;
    if (g.minlvl)
        j["minlvl"] = g.minlvl;
    if (g.open)
        j["open"] = g.open;
    if (g.close)
        j["close"] = g.close;
}

void from_json(const json &j, Guild &g)
{
    from_json(j, static_cast<org_data &>(g));
    if (j.contains("skills"))
        g.skills = j["skills"].get<FlagHandler<Skill>>();
    if (j.contains("feats"))
        g.feats = j["feats"].get<std::unordered_set<uint8_t>>();
    if (j.contains("charge"))
        g.charge = j["charge"];
    if (j.contains("no_such_skill"))
        g.no_such_skill = j["no_such_skill"];
    if (j.contains("not_enough_gold"))
        g.not_enough_gold = j["not_enough_gold"];
    if (j.contains("minlvl"))
        g.minlvl = j["minlvl"];
    if (j.contains("open"))
        g.open = j["open"];
    if (j.contains("close"))
        g.close = j["close"];
}

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

    if (j.contains("time"))
    {
        j["time"].get_to(time_info);
    }
    if (j.contains("era_uptime"))
        j["era_uptime"].get_to(era_uptime);
    if (j.contains("weather"))
    {
        j["weather"].get_to(weather_info);
    }
    if (j.contains("lastCharacterID"))
    {
        j["lastCharacterID"].get_to(Character::lastID);
    }
    if (j.contains("lastAccountID"))
    {
        j["lastAccountID"].get_to(lastAccountID);
    }
    if (j.contains("lastObjectID"))
    {
        j["lastObjectID"].get_to(Object::lastID);
    }
    if (j.contains("lastStructureID"))
    {
        j["lastStructureID"].get_to(lastStructureID);
    }
    if (j.contains("lastGridTemplateID"))
    {
        j["lastGridTemplateID"].get_to(lastGridTemplateID);
    }
    if (j.contains("lastAreaID"))
    {
        j["lastAreaID"].get_to(lastAreaID);
    }
    if (j.contains("lastRoomID"))
    {
        j["lastRoomID"].get_to(Room::lastID);
    }
    if (j.contains("lastShopID"))
    {
        j["lastShopID"].get_to(lastShopID);
    }
    if (j.contains("lastGuildID"))
    {
        j["lastGuildID"].get_to(lastGuildID);
    }
    
    if (j.contains("lastScriptID"))
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

void to_json(json &j, const HasProtoScript &s) {
    if(!s.proto_script.empty()) {
        j["proto_script"] = s.proto_script;
    }
}

void from_json(const json &j, HasProtoScript &s) {
    if(j.contains("proto_script")) {
        j.at("proto_script").get_to(s.proto_script);
    }
}


void to_json(json &j, const HasLocation &hl) {
    if(hl.location) {
        j["location"] = hl.location;
    }
    if(!hl.registeredLocations.empty()) {
        j["registeredLocations"] = hl.registeredLocations;
    }
}

void from_json(const json &j, HasLocation &hl) {
    if(j.contains("location")) {
        hl.location = j["location"].get<Location>();
    }
    if(j.contains("registeredLocations")) {
        j.at("registeredLocations").get_to(hl.registeredLocations);
    }
}

void to_json(json &j, const Destination &e)
{
    to_json(j, static_cast<const Location &>(e));
    j["dir"] = e.dir;
    if (!e.general_description.empty())
        j["general_description"] = e.general_description;
    if (!e.keyword.empty())
        j["keyword"] = e.keyword;
    // legacy exit_info omitted in new serialization; exit_flags used instead
    if(e.exit_flags) j["exit_flags"] = e.exit_flags;
    if (e.key > 0)
        j["key"] = e.key;
    if (e.dclock)
        j["dclock"] = e.dclock;
    if (e.dchide)
        j["dchide"] = e.dchide;
}

void from_json(const json &j, Destination &e)
{
    from_json(j, static_cast<Location &>(e));
    if (j.contains("dir"))
        e.dir = j["dir"].get<Direction>();
    if (j.contains("general_description"))
        e.general_description = j["general_description"].get<std::string>();
    if (j.contains("keyword"))
        e.keyword = j["keyword"].get<std::string>();
    if (j.contains("exit_flags"))
        e.exit_flags = j["exit_flags"].get<FlagHandler<ExitFlag>>();
    if (j.contains("key"))
        e.key = j["key"];
    if (j.contains("dclock"))
        e.dclock = j["dclock"];
    if (j.contains("dchide"))
        e.dchide = j["dchide"];
}

void to_json(json &j, const Room &r)
{
    // we need to call the to_json for unit_data...
    to_json(j, static_cast<const HasVnum &>(r));
    to_json(j, static_cast<const HasDgScripts &>(r));
    to_json(j, static_cast<const HasMudStrings &>(r));
    to_json(j, static_cast<const HasExtraDescriptions &>(r));
    to_json(j, static_cast<const HasResetCommands &>(r));
    to_json(j, static_cast<const HasZone &>(r));
    // to_json(j, static_cast<const HasStats&>(r));
    // to_json(j, static_cast<const HasAffectFlags&>(r));

    j["sector_type"] = r.sector_type;

    if (r.room_flags)
        j["room_flags"] = r.room_flags;

    for (auto p : r.proto_script)
    {
        if (trig_index.contains(p))
            j["proto_script"].push_back(p);
    }
}

void from_json(const json &j, Room &r)
{
    // call the from_json of unit_data...
    from_json(j, static_cast<HasVnum &>(r));
    from_json(j, static_cast<HasDgScripts &>(r));
    from_json(j, static_cast<HasMudStrings &>(r));
    from_json(j, static_cast<HasExtraDescriptions &>(r));
    from_json(j, static_cast<HasResetCommands &>(r));
    from_json(j, static_cast<HasZone &>(r));
    // from_json(j, static_cast<HasStats&>(r));
    // from_json(j, static_cast<HasAffectFlags&>(r));

    if (j.contains("sector_type"))
        r.sector_type = j["sector_type"];

    if (j.contains("room_flags"))
        r.room_flags = j["room_flags"].get<FlagHandler<RoomFlag>>();

    if (j.contains("proto_script"))
    {
        for (auto p : j["proto_script"])
            r.proto_script.emplace_back(p.get<trig_vnum>());
    }
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
        if(!j.contains("tileOverrides"))
            continue;

        auto vn = j["vn"].get<int>();
        if(auto cf = areas.find(vn); cf != areas.end())
        {
            auto &to = cf->second->tileOverrides;
            j.at("tileOverrides").get_to(to);
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
        structures.emplace(id, r);
        r->rebuildShapeIndex();
    }
}

void load_structures_finish(const std::filesystem::path &loc)
{
    for (auto j : load_from_file(loc, "structures.json"))
    {
        auto id = j["id"].get<int>();

        auto cf = structures.find(id);
        if(cf == structures.end()) continue;

        if(j.contains("tileOverrides")) {
            auto &to = cf->second->tileOverrides;
            j.at("tileOverrides").get_to(to);
        }

        if (j.contains("HasLocation"))
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

    for (auto &[v, r] : structures)
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

void to_json(json &j, const ObjectBase &o) {
    to_json(j, static_cast<const HasVnum &>(o));
    to_json(j, static_cast<const HasMudStrings &>(o));
    to_json(j, static_cast<const HasExtraDescriptions &>(o));
    to_json(j, static_cast<const HasStats &>(o));
    to_json(j, static_cast<const picky_data &>(o));
    j["type_flag"] = o.type_flag;
    if (o.wear_flags)
        j["wear_flags"] = o.wear_flags;
    if (o.item_flags)
        j["item_flags"] = o.item_flags;
    j["size"] = o.size;
    if(o.affect_flags) j["affect_flags"] = o.affect_flags;
    j["affected"] = o.affected;
}

void from_json(const json& j, ObjectBase &o) {
    from_json(j, static_cast<HasVnum &>(o));
    from_json(j, static_cast<HasMudStrings &>(o));
    from_json(j, static_cast<HasExtraDescriptions &>(o));
    from_json(j, static_cast<HasStats &>(o));
    from_json(j, static_cast<picky_data &>(o));

    if (j.contains("type_flag")) o.type_flag = j["type_flag"];
    if (j.contains("wear_flags")) j.at("wear_flags").get_to(o.wear_flags);
    if (j.contains("item_flags")) j.at("item_flags").get_to(o.item_flags);
    if (j.contains("size")) j.at("size").get_to(o.size);
    if (j.contains("affect_flags")) j.at("affect_flags").get_to(o.affect_flags);

    if (j.contains("affected")) j.at("affected").get_to(o.affected);
}

void to_json(json &j, const ObjectPrototype &o)
{
    to_json(j, static_cast<const ObjectBase &>(o));
    to_json(j, static_cast<const HasProtoScript &>(o));
};

void to_json(json &j, const Object &o)
{
    to_json(j, static_cast<const HasID &>(o));
    to_json(j, static_cast<const HasDgScripts &>(o));
    to_json(j, static_cast<const picky_data &>(o));
    to_json(j, static_cast<const HasMudStrings &>(o));
    to_json(j, static_cast<const HasExtraDescriptions &>(o));
    to_json(j, static_cast<const HasStats &>(o));

    //if (o.running_scripts) j["running_scripts"] = o.running_scripts.value();

    j["type_flag"] = o.type_flag;
    if (o.wear_flags)
        j["wear_flags"] = o.wear_flags;
    if (o.item_flags)
        j["item_flags"] = o.item_flags;

    for (auto &i : o.affected)
    {
        if (i.location == APPLY_NONE)
            continue;
        j["affected"].push_back(i);
    }
}

void from_json(const json &j, ObjectPrototype &o)
{
    from_json(j, static_cast<ObjectBase &>(o));
    from_json(j, static_cast<HasProtoScript &>(o));

    auto otype = o.type_flag;
    if ((otype == ITEM_PORTAL || otype == ITEM_HATCH) &&
        (!o.getBaseStat<int>(VAL_DOOR_DCLOCK) ||
         !o.getBaseStat<int>(VAL_DOOR_DCHIDE)))
    {
        for (const auto v : {VAL_DOOR_DCLOCK, VAL_DOOR_DCHIDE})
        {
            o.setBaseStat(v, 20);
        }
    }

    /* check to make sure that weight of containers exceeds curr. quantity */
    if (otype == ITEM_DRINKCON ||
        otype == ITEM_FOUNTAIN)
    {
        if (o.getBaseStat("weight") < o.getBaseStat(VAL_FOUNTAIN_HOWFULL))
            o.setBaseStat<weight_t>("weight", o.getBaseStat(VAL_FOUNTAIN_HOWFULL) + 5);
    }
    /* *** make sure portal objects have their timer set correctly *** */
    if (otype == ITEM_PORTAL)
    {
        o.setBaseStat<int>("timer", -1);
    }
}

void from_json(const json &j, Object &o)
{
    from_json(j, static_cast<ObjectBase &>(o));
    from_json(j, static_cast<HasID &>(o));
    from_json(j, static_cast<HasDgScripts &>(o));

    /*
    if (j.contains("running_scripts"))
    {
        o.running_scripts.emplace();
        j["running_scripts"].get_to(o.running_scripts.value());
    }
    */
}

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
    for (const auto &item : volcano::util::filter_raw(contents)) {
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
    if (j.contains("inventory")) {
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
    if (j.contains("equipment")) {
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
    for (const auto &item : volcano::util::filter_raw(contents)) {
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
    if (j.contains("contents")) {
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
    if (j.contains("posted_to"))
    {
        auto check = Object::registry.find(j["posted_to"].get<int64_t>());
        if (check != Object::registry.end())
            o->posted_to = check->second.get();
    }
    if (j.contains("fellow_wall"))
    {
        auto check = Object::registry.find(j["fellow_wall"].get<int>());
        if (check != Object::registry.end())
            o->fellow_wall = check->second.get();
    }

    if (j.contains("container"))
    {
        auto check = Character::registry.find(j["container"].get<int>());
        if (check != Character::registry.end())
            check->second->addToInventory(o);
    }
    else if (j.contains("carried_by"))
    {
        auto check = Character::registry.find(j["carried_by"].get<int>());
        if (check != Character::registry.end())
            check->second->addToInventory(o);
    }
    else if (j.contains("worn_by"))
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
                if(j.contains("relations"))
                    deserialize_obj_relations(i.get(), j["relations"]);
                if(j.contains("HasLocation")) {
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

// Character serialize/deserialize...
void to_json(json &j, const trans_data &t)
{
    if (t.time_spent_in_form != 0.0)
        j["time_spent_in_form"] = t.time_spent_in_form;
    j["visible"] = t.visible;
    j["limit_broken"] = t.limit_broken;
    j["unlocked"] = t.unlocked;
    j["grade"] = t.grade;
    if (!t.vars.empty())
        j["vars"] = t.vars;
    if (!t.description.empty())
        j["description"] = t.description;
    if (!t.appearances.empty())
        j["appearances"] = t.appearances;
}

void from_json(const json &j, trans_data &t)
{
    if (j.contains("time_spent_in_form"))
        t.time_spent_in_form = j["time_spent_in_form"];
    if (j.contains("visible"))
        t.visible = j["visible"];
    if (j.contains("limit_broken"))
        t.limit_broken = j["limit_broken"];
    if (j.contains("unlocked"))
        t.unlocked = j["unlocked"];
    if (j.contains("grade"))
        t.grade = j["grade"];
    if (j.contains("vars"))
        t.vars = j["vars"];
    if (j.contains("description"))
        t.description = j["description"].get<std::string>();
    if (j.contains("appearances"))
        t.appearances = j["appearances"];
}

void to_json(json &j, const affected_type &a)
{
    to_json(j, static_cast<const affect_t &>(a));
    if (a.type)
        j["type"] = a.type;
    if (a.duration)
        j["duration"] = a.duration;
    if (a.aff_flags)
        j["aff_flags"] = a.aff_flags;
}

void from_json(const json &j, affected_type &a)
{
    from_json(j, static_cast<affect_t &>(a));
    if (j.contains("type"))
        a.type = j["type"];
    if (j.contains("duration"))
        a.duration = j["duration"];
    if (j.contains("aff_flags")) j.at("aff_flags").get_to(a.aff_flags);
}

void to_json(json &j, const CharacterBase &c) {
    to_json(j, static_cast<const HasVnum&>(c));
    to_json(j, static_cast<const HasMudStrings&>(c));
    to_json(j, static_cast<const HasExtraDescriptions&>(c));
    to_json(j, static_cast<const HasStats&>(c));
    j["race"] = c.race;
    j["sensei"] = c.sensei;
    if(c.model) j["model"] = c.model;
    j["sex"] = c.sex;
    j["size"] = c.size;
    j["position"] = c.position;
    if(c.character_flags) j["character_flags"] = c.character_flags;
    if(c.mob_flags) j["mob_flags"] = c.mob_flags;
    if(c.affect_flags) j["affect_flags"] = c.affect_flags;
    if(c.bio_genomes) j["bio_genomes"] = c.bio_genomes;
    if(c.mutations) j["mutations"] = c.mutations;
}

void from_json(const json &j, CharacterBase &c) {
    from_json(j, static_cast<HasVnum&>(c));
    from_json(j, static_cast<HasMudStrings&>(c));
    from_json(j, static_cast<HasExtraDescriptions&>(c));
    from_json(j, static_cast<HasStats&>(c));
    if (j.contains("race")) c.race = j["race"];
    if(j.contains("sensei")) c.sensei = j["sensei"];
    if (j.contains("model")) c.model = j["model"];
    if(j.contains("position")) j.at("position").get_to(c.position);
    if (j.contains("sex")) c.sex = j["sex"];
    if (j.contains("size")) c.size = j["size"];
    if (j.contains("character_flags")) j.at("character_flags").get_to(c.character_flags);
    if (j.contains("mob_flags")) j.at("mob_flags").get_to(c.mob_flags);
    if (j.contains("affect_flags")) j.at("affect_flags").get_to(c.affect_flags);
    if (j.contains("bio_genomes")) j.at("bio_genomes").get_to(c.bio_genomes);
    if (j.contains("mutations")) j.at("mutations").get_to(c.mutations);
}

void to_json(json &j, const CharacterPrototype &c)
{
    to_json(j, static_cast<const CharacterBase &>(c));
    to_json(j, static_cast<const HasProtoScript&>(c));
}

void to_json(json& j, const HasZone& p) {
    if(p.zone) j["zone"] = p.zone->number;
}

void from_json(const json& j, HasZone& p) {
    if (j.contains("zone"))
        p.zone.reset(zone_table.at(j["zone"].get<zone_vnum>()).get());
}

void to_json(json& j, const HasDgScripts& p) {
    to_json(j, static_cast<const HasVariables&>(p));
    j["type"] = p.type;
    if(p.running_scripts) j["running_scripts"] = *p.running_scripts;
}

void from_json(const json& j, HasDgScripts& p) {
    from_json(j, static_cast<HasVariables&>(p));
    if (j.contains("type"))
        p.type = j["type"];
    if (j.contains("running_scripts"))
        p.running_scripts = j["running_scripts"].get<std::vector<vnum>>();
}

void to_json(json& j, const TileOverride& p) {
    to_json(j, static_cast<const HasResetCommands&>(p));
    if(!p.name.empty()) j["name"] = p.name;
    if(!p.look_description.empty()) j["look_description"] = p.look_description;
    j["roomFlags"] = p.roomFlags;
    j["whereFlags"] = p.whereFlags;
    j["damage"] = p.damage;
    j["groundEffect"] = p.groundEffect;
    j["exits"] = p.exits;
    if(!p.tileDisplay.empty()) j["tileDisplay"] = p.tileDisplay;

}

void from_json(const json& j, TileOverride& p) {
    from_json(j, static_cast<HasResetCommands&>(p));
    if(j.contains("name")) j.at("name").get_to(p.name);
    if(j.contains("look_description")) j.at("look_description").get_to(p.look_description);
    if (j.contains("roomFlags")) j.at("roomFlags").get_to(p.roomFlags);
    if (j.contains("whereFlags")) j.at("whereFlags").get_to(p.whereFlags);
    if (j.contains("damage")) j.at("damage").get_to(p.damage);
    if (j.contains("groundEffect")) j.at("groundEffect").get_to(p.groundEffect);
    if (j.contains("exits")) j.at("exits").get_to(p.exits);
    if(j.contains("tileDisplay")) j.at("tileDisplay").get_to(p.tileDisplay);
}


void to_json(json& j, const GridTemplate& p) {
    to_json(j, static_cast<const HasMudStrings&>(p));
    to_json(j, static_cast<const HasVnum&>(p));
    if(!p.shapes.empty()) j["shapes"] = p.shapes;
    if(!p.tileOverrides.empty()) j["tileOverrides"] = p.tileOverrides;

}

void from_json(const json& j, GridTemplate& p) {
    from_json(j, static_cast<HasMudStrings&>(p));
    from_json(j, static_cast<HasVnum&>(p));
    if(j.contains("shapes")) j.at("shapes").get_to(p.shapes);
    if(j.contains("tileOverrides")) j.at("tileOverrides").get_to(p.tileOverrides);
}

void to_json(json& j, const AbstractGridArea& p) {
    to_json(j, static_cast<const HasMudStrings&>(p));
    if(!p.shapes.empty()) {
        auto j2 = json::object();
        for(auto& [name, ptr] : p.shapes) {
            j2[name] = *ptr;
        }
        j["shapes"] = j2;
    }
    if(!p.tileOverrides.empty()) j["tileOverrides"] = p.tileOverrides;
}

void from_json(const json& j, AbstractGridArea& p) {
    from_json(j, static_cast<HasMudStrings&>(p));
    if(j.contains("shapes")) {
        // we have an object of key->data and need to use make_unique on data...
        for (auto& [key, value] : j.at("shapes").get<json::object_t>()) {
            Shape s{};
            value.get_to(s);
            p.shapes[key] = std::make_unique<Shape>(std::move(s));
        }
    }
    if(j.contains("tileOverrides")) j.at("tileOverrides").get_to(p.tileOverrides);
}

void to_json(json& j, const Area& p) {
    to_json(j, static_cast<const AbstractGridArea&>(p));
    to_json(j, static_cast<const HasVnum&>(p));
    to_json(j, static_cast<const HasZone&>(p));

}

void from_json(const json& j, Area& p) {
    from_json(j, static_cast<AbstractGridArea&>(p));
    from_json(j, static_cast<HasVnum&>(p));
    from_json(j, static_cast<HasZone&>(p));
}

void to_json(json& j, const Structure& p) {
    to_json(j, static_cast<const AbstractGridArea&>(p));
    to_json(j, static_cast<const HasID&>(p));

}

void from_json(const json& j, Structure& p) {
    from_json(j, static_cast<AbstractGridArea&>(p));
    from_json(j, static_cast<HasID&>(p));
}

void from_json(const json &j, CharacterPrototype &c)
{
    from_json(j, static_cast<CharacterBase &>(c));
    from_json(j, static_cast<HasProtoScript&>(c));

    if (c.race != Race::human)
        c.affect_flags.set(AFF_INFRAVISION, true);

    c.mob_flags.set(MOB_NOTDEADYET, false);
}

void to_json(json &j, const Character &c)
{
    to_json(j, static_cast<const CharacterBase &>(c));
    to_json(j, static_cast<const HasID &>(c));
    to_json(j, static_cast<const HasDgScripts &>(c));

    if(c.isPC) j["isPC"] = c.isPC;

    //if (c.running_scripts) j["running_scripts"] = c.running_scripts.value();

    if (!c.appearances.empty())
        j["appearances"] = c.appearances;

    if (c.character_flags)
        j["character_flags"] = c.character_flags;

    if (c.player_flags)
        j["player_flags"] = c.player_flags;

    if (c.pref_flags)
        j["pref_flags"] = c.pref_flags;

    for (auto i = 0; i < c.bodyparts.size(); i++)
        if (c.bodyparts.test(i))
        {
            j["bodyparts"].push_back(i);
        }

    if (c.admin_flags)
        j["admin_flags"] = c.admin_flags;

    json td;
    to_json(td, c.time);
    if (!td.empty())
        j["time"] = td;

    for (auto i = 0; i < 4; i++)
    {
        if (c.limb_condition[i])
        {
            j["limb_condition"].push_back(std::make_pair(i, c.limb_condition[i]));
        }
    }

    for (auto i = 0; i < NUM_CONDITIONS; i++)
    {
        if (c.conditions[i])
            j["conditions"].push_back(std::make_pair(i, c.conditions[i]));
    }

    for (auto i = 0; i < c.gravAcclim.size(); i++)
    {
        if (c.gravAcclim[i])
            j["gravAcclim"].push_back(std::make_pair(i, c.gravAcclim[i]));
    }

    auto ch = (Character *)&c;

    std::erase_if(ch->skill, [](const auto &s)
                  { return s.second.level == 0 && s.second.perfs == 0; });
    if (!c.skill.empty())
        j["skill"] = c.skill;

    for (auto a = c.affected; a; a = a->next)
    {
        if (a->type)
        {
            j["affected"].push_back(*a);
        }
    }

    for (auto a = c.affectedv; a; a = a->next)
    {
        if (a->type)
            j["affectedv"].push_back(*a);
    }

    for (auto i = 0; i < 5; i++)
    {
        if (c.lboard[i])
            j["lboard"].push_back(std::make_pair(i, c.lboard[i]));
    }

    if (c.mimic)
        j["mimic"] = c.mimic.value();
    j["form"] = c.form;

    if (c.rdisplay)
        j["rdisplay"] = c.rdisplay;
    if (c.feature)
        j["feature"] = c.feature;

    if (c.voice && strlen(c.voice))
        j["voice"] = c.voice;

    if (c.poofin && strlen(c.poofin))
        j["poofin"] = c.poofin;
    if (c.poofout && strlen(c.poofout))
        j["poofout"] = c.poofout;

    if (!c.transforms.empty())
        j["transforms"] = c.transforms;

    if (!c.permForms.empty())
        j["permForms"] = c.permForms;
}

void from_json(const json &j, Character &c)
{
    from_json(j, static_cast<CharacterBase &>(c));
    from_json(j, static_cast<HasID &>(c));
    from_json(j, static_cast<HasDgScripts &>(c));

    if(j.contains("isPC")) {
        c.isPC = j["isPC"];
    }

    /*
    if (j.contains("running_scripts"))
    {
        c.running_scripts.emplace();
        j["running_scripts"].get_to(c.running_scripts.value());
    }
    */

    if (j.contains("appearances"))
        c.appearances = j["appearances"];

    if (j.contains("player_flags"))
        c.player_flags = j["player_flags"].get<FlagHandler<PlayerFlag>>();

    if (j.contains("pref_flags"))
        c.pref_flags = j["pref_flags"].get<FlagHandler<PrefFlag>>();
    if (j.contains("bodyparts"))
        for (auto &i : j["bodyparts"])
            c.bodyparts.set(i.get<int>());

    if (j.contains("admin_flags"))
        c.admin_flags = j["admin_flags"].get<FlagHandler<AdminFlag>>();
    ;

    if (j.contains("time"))
    {
        j["time"].get_to(c.time);
    }

    if (j.contains("limb_condition"))
    {
        for (auto &i : j["limb_condition"])
        {
            c.limb_condition[i[0].get<int>()] = i[1];
        }
    }

    if (j.contains("skill"))
        c.skill = j["skill"].get<std::map<Skill, skill_data>>();

    if (j.contains("affected"))
    {
        auto ja = j["affected"];
        // reverse iterate using .rbegin() and .rend() while filling out
        // the linked list.
        for (auto it = ja.rbegin(); it != ja.rend(); ++it)
        {
            auto a = new affected_type(*it);
            a->next = c.affected;
            c.affected = a;
        }
    }

    if (j.contains("affectedv"))
    {
        auto ja = j["affectedv"];
        // reverse iterate using .rbegin() and .rend() while filling out
        // the linked list.
        for (auto it = ja.rbegin(); it != ja.rend(); ++it)
        {
            auto a = new affected_type(*it);
            a->next = c.affectedv;
            c.affectedv = a;
        }
    }

    if (j.contains("lboard"))
    {
        for (auto &i : j["lboard"])
        {
            c.lboard[i[0].get<int>()] = i[1];
        }
    }

    if (j.contains("conditions"))
    {
        for (auto &i : j["conditions"])
        {
            c.conditions[i[0].get<int>()] = i[1];
        }
    }

    if (j.contains("gravAcclim"))
    {
        for (auto &i : j["gravAcclim"])
        {
            c.gravAcclim[i[0].get<int>()] = i[1];
        }
    }

    if (j.contains("mimic"))
        c.mimic = j["mimic"].get<Race>();
    if (j.contains("rdisplay"))
        c.rdisplay = strdup(j["rdisplay"].get<std::string>().c_str());
    if (j.contains("feature"))
        c.feature = strdup(j["feature"].get<std::string>().c_str());

    if (j.contains("voice"))
        c.voice = strdup(j["voice"].get<std::string>().c_str());

    if (j.contains("form"))
        c.form = j["form"];
    if (j.contains("transforms"))
        c.transforms = j["transforms"];
    if (j.contains("permForms"))
        c.permForms = j["permForms"].get<std::unordered_set<Form>>();
}

void to_json(json &j, const ChargenData &c)
{
    if(c.name) j["name"] = *c.name;
    if(c.race) j["race"] = *c.race;
    if(c.model) j["model"] = *c.model;
    if(c.sex) j["sex"] = *c.sex;
    if(c.sensei) j["sensei"] = *c.sensei;
    if(c.bio_genomes) j["bio_genomes"] = c.bio_genomes;
    if(c.mutations) j["mutations"] = c.mutations;
    j["keep_skills"] = c.keep_skills;
    j["alignment"] = c.alignment;
}

void from_json(const json &j, ChargenData &c)
{
    if (j.contains("name")) c.name = j["name"].get<std::string>();
    if (j.contains("race")) c.race = j["race"].get<Race>();
    if (j.contains("model")) c.model = j["model"].get<AndroidModel>();
    if (j.contains("sex")) c.sex = j["sex"].get<Sex>();
    if (j.contains("sensei")) c.sensei = j["sensei"].get<Sensei>();
    if (j.contains("bio_genomes")) j.at("bio_genomes").get_to(c.bio_genomes);
    if (j.contains("mutations")) j.at("mutations").get_to(c.mutations);
    if (j.contains("keep_skills")) c.keep_skills = j["keep_skills"];
    if (j.contains("alignment")) c.alignment = j["alignment"];
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
        if(!j.contains("HasLocation")) continue;

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
void to_json(json &j, const PlayerData &p)
{
    j["id"] = p.id;
    j["name"] = p.name;
    if (p.account)
        j["account"] = p.account->id;

    for (auto &a : p.aliases)
    {
        j["aliases"].push_back(a);
    }

    for (auto &i : p.sense_player)
    {
        j["sensePlayer"].push_back(i);
    }

    for (auto &i : p.sense_memory)
    {
        j["senseMemory"].push_back(i);
    }

    for (auto &i : p.dub_names)
    {
        j["dubNames"].push_back(i);
    }

    for (auto i = 0; i < NUM_COLOR; i++)
    {
        if (p.color_choices[i] && strlen(p.color_choices[i]))
            j["color_choices"].push_back(std::make_pair(i, p.color_choices[i]));
    }
}

void from_json(const json &j, PlayerData &p)
{
    p.id = j["id"];
    p.name = j["name"].get<std::string>();
    if (j.contains("account"))
    {
        auto accID = j["account"].get<vnum>();
        auto accFind = accounts.find(accID);
        if (accFind != accounts.end())
            p.account = accFind->second.get();
    }

    if (j.contains("aliases"))
    {
        for (auto ja : j["aliases"])
        {
            p.aliases.emplace_back(ja);
        }
    }

    if (j.contains("sensePlayer"))
    {
        for (auto &i : j["sensePlayer"])
        {
            p.sense_player.insert(i.get<int64_t>());
        }
    }

    if (j.contains("senseMemory"))
    {
        for (auto &i : j["senseMemory"])
        {
            p.sense_memory.insert(i.get<vnum>());
        }
    }

    if (j.contains("dubNames"))
    {
        for (auto &i : j["dubNames"])
        {
            p.dub_names.emplace(i[0].get<int64_t>(), i[1].get<std::string>());
        }
    }

    if (j.contains("color_choices"))
    {
        for (auto &i : j["color_choices"])
        {
            p.color_choices[i[0].get<int>()] = strdup(i[1].get<std::string>().c_str());
        }
    }
}

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


void to_json(json &j, const ShapeBase &p)
{
    j["type"] = p.type;
    j["priority"] = p.priority;
    j["sectorType"] = p.sectorType;
    if(!p.name.empty()) j["name"] = p.name;
    if(!p.description.empty()) j["description"] = p.description;
    j["geom"] = std::visit([](auto const& g) {
        return json(g); // relies on to_json for BoxDim / RoundDim
    }, p.geom);
    if(!p.tileDisplay.empty()) j["tileDisplay"] = p.tileDisplay;
}

void from_json(const json &j, ShapeBase &r) {
    if(j.contains("type")) r.type = j["type"].get<ShapeType>();
    if(j.contains("priority")) r.priority = j["priority"].get<int>();
    if(j.contains("sectorType")) r.sectorType = j["sectorType"].get<SectorType>();
    if(j.contains("name")) r.name = j["name"].get<std::string>();
    if(j.contains("description")) r.description = j["description"].get<std::string>();
    if (j.contains("geom")) {
        const auto& g = j.at("geom");
        switch (r.type) { // <- r.type (not p.type)
            case ShapeType::Box:
                r.geom = g.get<BoxDim>();
                break;
            case ShapeType::Round:
                r.geom = g.get<RoundDim>();
                break;
        }
    } else {
        // optional: default the variant based on type
        switch (r.type) {
            case ShapeType::Box:   r.geom = BoxDim{};   break;
            case ShapeType::Round: r.geom = RoundDim{}; break;
        }
    }
    if(j.contains("tileDisplay")) r.tileDisplay = j["tileDisplay"];
}

void to_json(json &j, const Shape &p) {
    to_json(j, static_cast<const ShapeBase&>(p));
}

void from_json(const json &j, Shape &r) {
    from_json(j, static_cast<ShapeBase&>(r));
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
        auto vn = j.at("vnum").get<vnum>();
        auto atype = j.at("assembly_type").get<int>();
        assemblyCreate(vn, atype);
        if (j.contains("components"))
        {
            // components are an array containing bExtract, bInRoom, and vnum lVnum...
            for (auto comp : j.at("components"))
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
            {"areas", dump_areas},
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
            std::vector<std::thread> threads;
            for (const auto &[name, task] : tasks)
            {
                threads.emplace_back([task, name, &tempPath]()
                                    {
                    try {
                        auto data = task();
                        dump_to_file(tempPath, name + ".json", data);
                    } catch (const std::exception& e) {
                        LERROR("Error during dump: %s", e.what());
                        throw;
                    } });
            }

            for (auto &thread : threads)
            {
                thread.join();
            }
            threads.clear();

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

