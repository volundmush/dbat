#include <fstream>
#include <thread>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "fmt/core.h"

#include "magic_enum/magic_enum_all.hpp"

#include "dbat/saveload.h"
#include "dbat/config.h"
#include "dbat/send.h"
#include "dbat/players.h"
#include "dbat/db.h"
#include "dbat/account.h"
#include "dbat/dg_scripts.h"
#include "dbat/guild.h"
#include "dbat/shop.h"
#include "dbat/constants.h"
#include "dbat/races.h"
#include "dbat/class.h"
#include "dbat/json.h"
#include "dbat/assemblies.h"
#include "dbat/assedit.h"

// dump and load routines...
static void dump_to_file(const std::filesystem::path &loc, const std::string &name, const json &data) {
    if(data.empty()) return;
    //auto startTime = std::chrono::high_resolution_clock::now();
    std::ofstream file(loc / (name + ".gz"));
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(boost::iostreams::gzip_compressor());
    out.push(file);
    std::ostream outStream(&out);
    outStream << jdumps(data);

    //auto endTime = std::chrono::high_resolution_clock::now();
    //auto duration = std::chrono::duration<double>(endTime - startTime).count();
    //basic_mud_log("Dumping %s to disk took %f seconds.", name, duration);
}

static json load_from_file(const std::filesystem::path& loc, const std::string& name)
{
    // We'll automatically append ".gz"
    auto path = loc / (name + ".gz");
    if (!std::filesystem::exists(path)) {
        basic_mud_log("File %s does not exist", path.c_str());
        return {};
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
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
    try {
        instream >> j;
    } catch (const std::exception &e) {
        basic_mud_log("JSON parse error reading %s: %s", path.c_str(), e.what());
        return {};
    }
    return j;
}


// helper type save/load...
template <std::size_t N>
void to_json(json& j, const std::bitset<N>& bs) {
    std::vector<std::size_t> set_indexes;
    for (std::size_t i = 0; i < N; ++i) {
        if (bs.test(i)) {
            set_indexes.push_back(i);
        }
    }
    j = set_indexes;
}

template <std::size_t N>
void from_json(const json& j, std::bitset<N>& bs) {
    bs.reset();
    // Extract the vector of indexes from JSON
    auto indexes = j.get<std::vector<std::size_t>>();
    for (auto idx : indexes) {
        if (idx < N) {  // ensure the index is within range
            bs.set(idx, true);
        }
    }
}

// zone_data and reset_com serialize/deserialize...
void to_json(json& j, const reset_com& r) {
    std::string cmd;
    cmd.push_back(r.command);
    j["command"] = cmd;
    
    if(r.if_flag)
        j["if_flag"] = r.if_flag;
    if(r.arg1)
        j["arg1"] = r.arg1;
    if(r.arg2)
        j["arg2"] = r.arg2;
    if(r.arg3)
        j["arg3"] = r.arg3;
    if(r.arg4)
        j["arg4"] = r.arg4;
    if(r.arg5)
        j["arg5"] = r.arg5;
    if(!r.sarg1.empty())
        j["sarg1"] = r.sarg1;
    if(!r.sarg2.empty())
        j["sarg2"] = r.sarg2;
}

void from_json(const json& j, reset_com& r) {
    if(j.contains("command"))
        r.command = j["command"].get<std::string>()[0];
    if(j.contains("if_flag"))
        r.if_flag = j["if_flag"];
    if(j.contains("arg1"))
        r.arg1 = j["arg1"];
    if(j.contains("arg2"))
        r.arg2 = j["arg2"];
    if(j.contains("arg3"))
        r.arg3 = j["arg3"];
    if(j.contains("arg4"))
        r.arg4 = j["arg4"];
    if(j.contains("arg5"))
        r.arg5 = j["arg5"];
    if(j.contains("sarg1"))
        r.sarg1 = j["sarg1"];
    if(j.contains("sarg2"))
        r.sarg2 = j["sarg2"];
}

void to_json(json& j, const Zone& z) {
    j["number"] = z.number;
    if(!z.name.empty())
        j["name"] = z.name;
    if(!z.builders.empty())
        j["builders"] = z.builders;
    if(z.lifespan)
        j["lifespan"] = z.lifespan;
    j["bot"] = z.bot;
    j["top"] = z.top;
    if(z.reset_mode)
        j["reset_mode"] = z.reset_mode;
    if(z.min_level)
        j["min_level"] = z.min_level;
    if(z.max_level)
        j["max_level"] = z.max_level;

    // Serialize zone flags
    if(z.zone_flags) j["zone_flags"] = z.zone_flags;

    // Serialize command vector (assuming each element in 'cmd' is of type reset_com)
    for(const auto &c : z.cmd) {
        j["cmd"].push_back(c);  // This will call to_json(reset_com) automatically.
    }
}

void from_json(const json& j, Zone& z) {
    if(j.contains("number"))
        z.number = j["number"];
    if(j.contains("name"))
        z.name = j["name"].get<std::string>();
    if(j.contains("builders"))
        z.builders = j["builders"].get<std::string>();
    if(j.contains("lifespan"))
        z.lifespan = j["lifespan"];
    if(j.contains("bot"))
        z.bot = j["bot"];
    if(j.contains("top"))
        z.top = j["top"];
    if(j.contains("reset_mode"))
        z.reset_mode = j["reset_mode"];
    if(j.contains("min_level"))
        z.min_level = j["min_level"];
    if(j.contains("max_level"))
        z.max_level = j["max_level"];

    if(j.contains("zone_flags")) z.zone_flags = j["zone_flags"].get<FlagHandler<ZoneFlag>>();

    if(j.contains("cmd")) {
        int line = 1;
        z.cmd.reserve(j["cmd"].size());
        for(auto &c : j["cmd"]) {
            // Here we rely on reset_com's from_json to convert each element.
            auto &cm = z.cmd.emplace_back(c.get<reset_com>());
            cm.line = line++;
        }
    }
}

void load_zones(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "zones.json")) {
        auto id = j.at("number").get<int64_t>();
        zone_table.emplace(id, j);
    }
}

static void dump_zones(const std::filesystem::path &loc) {
    json j;

    for(auto &[v, z] : zone_table) {
        j.push_back(z);
    }
    dump_to_file(loc, "zones.json", j);
}

// affect_t serialize/deserialize...

void to_json(json& j, const affect_t& a) {
    if(a.location) j["location"] = a.location;
    if(a.modifier != 0.0) j["modifier"] = a.modifier;
    if(a.specific) j["specific"] = a.specific;
}

void from_json(const json& j, affect_t& a) {
    if(j.contains("location")) a.location = j.at("location");
    if(j.contains("modifier")) a.modifier = j.at("modifier");
    if(j.contains("specific")) a.specific = j.at("specific");
}


// account_data serialize/deserialize...
void to_json(json& j, const Account& a) {
    if(a.id != NOTHING)
        j["id"] = a.id;
    if(!a.name.empty())
        j["name"] = a.name;
    if(!a.password.empty())
        j["password"] = a.password;
    if(!a.email.empty())
        j["email"] = a.email;
    if(a.created)
        j["created"] = a.created;
    if(a.last_login)
        j["last_login"] = a.last_login;
    if(a.last_logout)
        j["last_logout"] = a.last_logout;
    if(a.last_change_password)
        j["last_change_password"] = a.last_change_password;
    if(a.playtime != 0.0)
        j["playtime"] = a.playtime;
    if(!a.disabled_reason.empty())
        j["disabled_reason"] = a.disabled_reason;
    if(a.disabled_until)
        j["disabled_until"] = a.disabled_until;
    if(a.rpp)
        j["rpp"] = a.rpp;
    if(a.slots != 3)
        j["slots"] = a.slots;
    if(a.admin_level)
        j["admin_level"] = a.admin_level;

    // For the characters field (vector<int>), assign it directly.
    j["characters"] = a.characters;
}

void from_json(const json& j, Account& a) {
    if(j.contains("id"))
        a.id = j["id"];
    if(j.contains("name"))
        a.name = j["name"];
    if(j.contains("password"))
        a.password = j["password"];
    if(j.contains("email"))
        a.email = j["email"];
    if(j.contains("created"))
        a.created = j["created"];
    if(j.contains("lastLogin"))
        a.last_login = j["lastLogin"];
    if(j.contains("lastLogout"))
        a.last_logout = j["lastLogout"];
    if(j.contains("last_change_password"))
        a.last_change_password = j["last_change_password"];
    if(j.contains("playtime"))
        a.playtime = j["playtime"];
    if(j.contains("disabled_reason"))
        a.disabled_reason = j["disabled_reason"];
    if(j.contains("disabled_until"))
        a.disabled_until = j["disabled_until"];
    if(j.contains("rpp"))
        a.rpp = j["rpp"];
    if(j.contains("slots"))
        a.slots = j["slots"];
    if(j.contains("admin_level"))
        a.admin_level = j["admin_level"];

    // Directly deserialize the vector<int> field.
    if(j.contains("characters"))
        a.characters = j["characters"].get<std::vector<int>>();
}


static void dump_accounts(const std::filesystem::path &loc) {
    json j;

    for(auto &[v, r] : accounts) {
        j.push_back(r);
    }

    dump_to_file(loc, "accounts.json", j);
}

void load_accounts(const std::filesystem::path& loc) {
    for(auto acc : load_from_file(loc, "accounts.json")) {
        auto vn = acc.at("id").get<int64_t>();
        accounts.emplace(vn, acc);
    }

}

void to_json(json& j, const DgScriptPrototype& t) {
    j["vn"] = t.vn;
    j["name"] = t.name;
    j["attach_type"] = t.attach_type;
    j["trigger_type"] = t.trigger_type;
    j["narg"] = t.narg;
    j["arglist"] = t.arglist;

    j["body"] = t.scriptString();
}


void from_json(const json& j, DgScriptPrototype& t) {
    if(j.contains("vn")) t.vn = j["vn"].get<int>();
    if(j.contains("name")) t.name = strdup(j["name"].get<std::string>().c_str());
    if(j.contains("attach_type")) t.attach_type = j["attach_type"].get<UnitType>();
    if(j.contains("trigger_type")) t.trigger_type = j["trigger_type"].get<int>();
    if(j.contains("narg")) t.narg = j["narg"].get<int>();
    if(j.contains("arglist")) t.arglist = strdup(j["arglist"].get<std::string>().c_str());

    if(j.contains("body")) {
        t.setBody(j["body"].get<std::string>());
    }
}


void to_json(json& j, const DgScript& t) {
    j["vn"] = t.getVnum();
    j["current_line"] = t.current_line;
    j["state"] = t.state;
    j["depth_stack"] = t.depth_stack;
    if(t.waiting != 0.0) j["waiting"] = t.waiting;
    if(!t.variables.empty()) j["variables"] = t.variables;
}

void from_json(const json& j, DgScript& t) {
    auto vn = j["vn"].get<int>();
    t.proto = &trig_index.at(vn);

    if(j.contains("state")) t.state = j["state"].get<DgScriptState>();
    if(j.contains("waiting")) t.waiting = j["waiting"].get<double>();
    if(j.contains("depth_stack")) t.depth_stack = j["depth_stack"].get<std::vector<DepthType>>();

    if(j.contains("variables")) {
        t.variables = j["variables"].get<std::unordered_map<std::string, std::string>>();
    }
}

void load_dgscript_prototypes(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "dgScriptPrototypes.json")) {
        auto id = j["vn"].get<int64_t>();
        auto &t = trig_index[id];
        j.get_to(t);
        auto zone = real_zone_by_thing(id);
        auto& z = zone_table.at(zone);
        z.triggers.insert(id);
    }
}

static void dump_dgscript_prototypes(const std::filesystem::path &loc) {
    json j;
    for(auto &[v, t] : trig_index) {
        j.push_back(t);
    }
    dump_to_file(loc, "dgScriptPrototypes.json", j);
}

void load_dgscripts(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "dgscripts.json")) {
        auto id = j["id"].get<int64_t>();

        auto u = units.at(id);

        for(auto d : j["scripts"]) {
            auto vn = d["vn"].get<int>();
            auto r = std::make_shared<DgScript>();
            d["data"].get_to(*r);
            u->scripts.emplace(vn, r);
            r->owner = u.get();
            u->trigger_types |= GET_TRIG_TYPE(r);
        }
    }
}

static void dump_dgscripts(const std::filesystem::path &loc) {
    json j;

    for(auto &[id, u] : units) {
        if(u->scripts.empty()) continue; // Skip units without scripts
        json j2;
        j2["id"] = id;
        for(auto &[vn, r] : u->scripts) {
            json j3;
            j3["vn"] = vn;
            j3["data"] = *r;
            j2["scripts"].push_back(j3);
        }
        j.push_back(j2);
    }
    dump_to_file(loc, "dgscripts.json", j);
}

// org_data...
void to_json(json& j, const picky_data& p) {
    if(!p.not_alignment.empty()) j["not_alignment"] = p.not_alignment;
    if(!p.not_race.empty()) j["not_race"] = p.not_race;
    if(!p.only_race.empty()) j["only_race"] = p.only_race;
    if(!p.not_sensei.empty()) j["not_sensei"] = p.not_sensei;
    if(!p.only_sensei.empty()) j["only_sensei"] = p.only_sensei;
}

void from_json(const json& j, picky_data& p) {
    if(j.contains("not_alignment")) p.not_alignment = j["not_alignment"].get<std::unordered_set<MoralAlign>>();
    if(j.contains("not_race")) p.not_race = j["not_race"].get<std::unordered_set<Race>>();
    if(j.contains("only_race")) p.only_race = j["only_race"].get<std::unordered_set<Race>>();
    if(j.contains("not_sensei")) p.not_sensei = j["not_sensei"].get<std::unordered_set<Sensei>>();
    if(j.contains("only_sensei")) p.only_sensei = j["only_sensei"].get<std::unordered_set<Sensei>>();
}

void to_json(json& j, const org_data& o) {
    to_json(j, static_cast<picky_data>(o));
    j["vnum"] = o.vnum;

    if(o.keeper != NOTHING) j["keeper"] = o.keeper;
}

void from_json(const json& j, org_data& o) {
    from_json(j, static_cast<picky_data&>(o));
    if(j.contains("vnum")) o.vnum = j["vnum"];
    
    if(j.contains("keeper")) o.keeper = j["keeper"];
}

// shops serialize/deserialize...
void to_json(json&j, const shop_data& s) {
    to_json(j, static_cast<org_data>(s));
    
    if(!s.producing.empty()) j["producing"] = s.producing;
    if(s.profit_buy) j["profit_buy"] = s.profit_buy;
    if(s.profit_sell) j["profit_sell"] = s.profit_sell;
    if(!s.type.empty()) j["type"] = s.type;
    if(s.no_such_item1 && strlen(s.no_such_item1)) j["no_such_item1"] = s.no_such_item1;
    if(s.no_such_item2 && strlen(s.no_such_item2)) j["no_such_item2"] = s.no_such_item2;
    if(s.missing_cash1 && strlen(s.missing_cash1)) j["missing_cash1"] = s.missing_cash1;
    if(s.missing_cash2 && strlen(s.missing_cash2)) j["missing_cash2"] = s.missing_cash2;
    if(s.do_not_buy && strlen(s.do_not_buy)) j["do_not_buy"] = s.do_not_buy;
    if(s.message_buy && strlen(s.message_buy)) j["message_buy"] = s.message_buy;
    if(s.message_sell && strlen(s.message_sell)) j["message_sell"] = s.message_sell;
    if(s.temper1) j["temper1"] = s.temper1;
    j["shop_flags"] = s.shop_flags;
    for(auto r : s.in_room) j["in_room"].push_back(r);
    if(s.open1) j["open1"] = s.open1;
    if(s.close1) j["close1"] = s.close1;
    if(s.open2) j["open2"] = s.open2;
    if(s.close2) j["close2"] = s.close2;
    if(s.bankAccount) j["bankAccount"] = s.bankAccount;
    if(s.lastsort) j["lastsort"] = s.lastsort;
}

void from_json(const json& j, shop_data& s) {
    from_json(j, static_cast<org_data&>(s));
    if(j.contains("producing")) s.producing = j["producing"].get<std::vector<int>>();
    if(j.contains("profit_buy")) s.profit_buy = j["profit_buy"];
    if(j.contains("profit_sell")) s.profit_sell = j["profit_sell"];
    if(j.contains("type")) s.type = j["type"];
    if(j.contains("no_such_item1")) s.no_such_item1 = strdup(j["no_such_item1"].get<std::string>().c_str());
    if(j.contains("no_such_item2")) s.no_such_item2 = strdup(j["no_such_item2"].get<std::string>().c_str());
    if(j.contains("missing_cash1")) s.missing_cash1 = strdup(j["missing_cash1"].get<std::string>().c_str());
    if(j.contains("missing_cash2")) s.missing_cash2 = strdup(j["missing_cash2"].get<std::string>().c_str());
    if(j.contains("do_not_buy")) s.do_not_buy = strdup(j["do_not_buy"].get<std::string>().c_str());
    if(j.contains("message_buy")) s.message_buy = strdup(j["message_buy"].get<std::string>().c_str());
    if(j.contains("message_sell")) s.message_sell = strdup(j["message_sell"].get<std::string>().c_str());
    if(j.contains("temper1")) s.temper1 = j["temper1"];
    
    if(j.contains("shop_flags")) s.shop_flags = j["shop_flags"].get<FlagHandler<ShopFlag>>();

    if(j.contains("in_room")) {
        for(auto &r : j["in_room"]) {
            s.in_room.insert(r.get<int>());
        }
    }
    if(j.contains("open1")) s.open1 = j["open1"];
    if(j.contains("close1")) s.close1 = j["close1"];
    if(j.contains("open2")) s.open2 = j["open2"];
    if(j.contains("close2")) s.close2;
    if(j.contains("bankAccount")) s.bankAccount = j["bankAccount"];
    if(j.contains("lastsort")) s.lastsort = j["lastsort"];
}

void load_shops(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "shops.json")) {
        auto id = j["vnum"].get<int64_t>();
        shop_index.emplace(id, j);
        auto zone = real_zone_by_thing(id);
        auto& z = zone_table.at(zone);
        z.shops.insert(id);
    }
}

static void dump_shops(const std::filesystem::path &loc) {
    json j;
    for(auto &[v, s] : shop_index) {
        j.push_back(s);
    }
    dump_to_file(loc, "shops.json", j);
}

// guilds serialize/deserialize...
void to_json(json& j, const guild_data& g) {
    to_json(j, static_cast<org_data>(g));
    if(g.skills) j["skills"] = g.skills;
    if(!g.feats.empty()) j["feats"] = g.feats;
    if(g.charge != 1.0) j["charge"] = g.charge;
    if(!g.no_such_skill.empty()) j["no_such_skill"] = g.no_such_skill;
    if(!g.not_enough_gold.empty()) j["not_enough_gold"] = g.not_enough_gold;
    if(g.minlvl) j["minlvl"] = g.minlvl;
    if(g.open) j["open"] = g.open;
    if(g.close) j["close"] = g.close;
}

void from_json(const json& j, guild_data& g) {
    from_json(j, static_cast<org_data&>(g));
    if(j.contains("skills")) g.skills = j["skills"].get<FlagHandler<Skill>>();
    if(j.contains("feats")) g.feats = j["feats"].get<std::unordered_set<uint8_t>>();
    if(j.contains("charge")) g.charge = j["charge"];
    if(j.contains("no_such_skill")) g.no_such_skill = j["no_such_skill"];
    if(j.contains("not_enough_gold")) g.not_enough_gold = j["not_enough_gold"];
    if(j.contains("minlvl")) g.minlvl = j["minlvl"];
    if(j.contains("open")) g.open = j["open"];
    if(j.contains("close")) g.close = j["close"];
}

static void dump_guilds(const std::filesystem::path &loc) {
    json j;
    for(auto &[v, g] : guild_index) {
        j.push_back(g);
    }
    dump_to_file(loc, "guilds.json", j);
}

void load_guilds(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "guilds.json")) {
        auto id = j["vnum"].get<int64_t>();
        guild_index.emplace(id, j);
        auto zone = real_zone_by_thing(id);
        auto& z = zone_table.at(zone);
        z.guilds.insert(id);
    }
}

// globaldata serialize/deserialize...
void load_globaldata(const std::filesystem::path& loc) {
    auto j = load_from_file(loc, "globaldata.json");

    if(j.contains("time")) {
        j["time"].get_to(time_info);
    }
    if(j.contains("era_uptime"))
        j["era_uptime"].get_to(era_uptime);
    if(j.contains("weather")) {
        j["weather"].get_to(weather_info);
    }
}

void dump_globaldata(const std::filesystem::path &loc) {
    json j;

    j["time"] = time_info;
    j["era_uptime"] = era_uptime;
    j["weather"] = weather_info;

    dump_to_file(loc, "globaldata.json", j);
}

void to_json(json& j, const struct extra_descr_data& e) {
    if(e.keyword && strlen(e.keyword)) j["keyword"] = e.keyword;
    if(e.description && strlen(e.description)) j["description"] = e.description;
}

void from_json(const json& j, struct extra_descr_data& e) {
    if(j.contains("keyword")) e.keyword = strdup(j["keyword"].get<std::string>().c_str());
    if(j.contains("description")) e.description = strdup(j["description"].get<std::string>().c_str());
}

// unit_data serialize/deserialize...
void to_json(json& j, const Entity& u) {
    j["vn"] = u.vn;
    j["id"] = u.id;
    j["generation"] = u.generation;
    j["strings"] = u.strings;
    j["extra_descriptions"] = u.extra_descriptions;

    if(u.running_scripts.has_value()) {
        j["running_scripts"] = u.running_scripts.value();
    }

    if(u.variables.empty()) {
        j["variables"] = u.variables;
    }
}

void from_json(const json& j, Entity& u) {
    if(j.contains("vn")) u.vn = j["vn"].get<int>();
    if(j.contains("id")) u.id = j["id"];
    if(j.contains("generation")) u.generation = j["generation"];
    if(j.contains("strings")) u.strings = j["strings"].get<std::unordered_map<std::string, std::string>>();
    if(j.contains("extra_descriptions")) u.extra_descriptions = j["extra_descriptions"].get<std::vector<ExtraDescription>>();

    if(j.contains("running_scripts")) {
        u.running_scripts = j["running_scripts"].get<std::vector<trig_vnum>>();
    }

    if(j.contains("variables")) u.variables = j["variables"].get<std::unordered_map<std::string, std::string>>();
}


void to_json(json& j, const ThingPrototype& u) {
    j["vn"] = u.vn;
    if(u.name && strlen(u.name)) j["name"] = u.name;
    if(u.room_description && strlen(u.room_description)) j["room_description"] = u.room_description;
    if(u.look_description && strlen(u.look_description)) j["look_description"] = u.look_description;
    if(u.short_description && strlen(u.short_description)) j["short_description"] = u.short_description;
    for(auto ex = u.ex_description; ex; ex = ex->next) {
        if(ex->keyword && strlen(ex->keyword) && ex->description && strlen(ex->description)) {
            j["ex_description"].push_back(*ex);
        }
    }
    if(!u.proto_script.empty()) j["proto_script"] = u.proto_script;
    if(!u.stats.empty()) j["stats"] = u.stats;
    if(u.affect_flags) j["affect_flags"] = u.affect_flags;
}

void from_json(const json& j, ThingPrototype& u) {
    u.vn = j["vn"].get<int>();
    if(j.contains("name")) u.name = strdup(j["name"].get<std::string>().c_str());
    if(j.contains("room_description")) u.room_description = strdup(j["room_description"].get<std::string>().c_str());
    if(j.contains("look_description")) u.look_description = strdup(j["look_description"].get<std::string>().c_str());
    if(j.contains("short_description")) u.short_description = strdup(j["short_description"].get<std::string>().c_str());

    if(j.contains("ex_description")) {
        auto &e = j["ex_description"];
        for(auto ex = e.rbegin(); ex != e.rend(); ex++) {
            auto new_ex = new extra_descr_data();
            ex->get_to(*new_ex);
            new_ex->next = u.ex_description;
            u.ex_description = new_ex;
        }
    }

    if(j.contains("proto_script")) {
        for(auto p : j["proto_script"]) {
            u.proto_script.emplace_back(p.get<trig_vnum>());
        }
    }

    if(j.contains("stats")) u.stats = j["stats"];

    if(j.contains("affect_flags")) {
        u.affect_flags = j["affect_flags"].get<FlagHandler<AffectFlag>>();
    }
}

void to_json(json& j, const Location& loc) {
    if(loc.unit) j["uid"] = loc.unit->getUID();
    else {
        throw std::runtime_error("Location has no unit.");
    }
    j["position"] = loc.position;
}

void from_json(const json& j, Location& loc) {
    try {
        if(j.contains("uid")) {
        auto u = j.at("uid").get<std::string>();
        auto uid = resolveUID(u);
        if(uid) loc.unit = uid.get();
        else {
            throw std::runtime_error(fmt::format("Location has invalid unit: {}", u));
        }
        } else {
            throw std::runtime_error("Location JSON does not contain 'uid'.");
        }
        loc.position = j["position"];
    } catch (const std::exception &e) {
        throw std::runtime_error(fmt::format("Error parsing Location JSON: {}\r\nJSON DATA: {}", e.what(), jdumps(j)));
    }
    
}

void to_json(json& j, const Destination &e) {
    to_json(j, static_cast<const Location&>(e));
    j["dir"] = e.dir;
    if(!e.general_description.empty()) j["general_description"] = e.general_description;
    if(!e.keyword.empty()) j["keyword"] = e.keyword;
    if(e.exit_info) {
        j["exit_info"] = e.exit_info;
        for(auto i = 0; i < NUM_EXIT_FLAGS; i++) {
            if(IS_SET(e.exit_info, 1 << i)) {
                auto key = std::string(exit_bits[i]);
                boost::to_lower(key);
                j["exit_flags"].push_back(key);
            }
        }
    }
    if(e.key > 0) j["key"] = e.key;
    if(e.dclock) j["dclock"] = e.dclock;
    if(e.dchide) j["dchide"] = e.dchide;
}

void from_json(const json& j, Destination &e) {
    from_json(j, static_cast<Location&>(e));
    if(j.contains("dir")) e.dir = j["dir"].get<Direction>();
    if(j.contains("general_description")) e.general_description = j["general_description"].get<std::string>();
    if(j.contains("keyword")) e.keyword = j["keyword"].get<std::string>();
    if(j.contains("exit_info")) e.exit_info = j["exit_info"].get<int16_t>();
    if(j.contains("key")) e.key = j["key"];
    if(j.contains("dclock")) e.dclock = j["dclock"];
    if(j.contains("dchide")) e.dchide = j["dchide"];
}

void to_json(json& j, const Room& r) {
    // we need to call the to_json for unit_data...
    to_json(j, static_cast<const Entity&>(r));
    j["zone"] = r.zone->number;
    
    j["sector_type"] = r.sector_type;

    if(r.room_flags) j["room_flags"] = r.room_flags;

    for(auto p : r.proto_script) {
        if(trig_index.contains(p)) j["proto_script"].push_back(p);
    }
}

void from_json(const json& j, Room& r) {
    // call the from_json of unit_data...
    from_json(j, static_cast<Entity&>(r));

    if(j.contains("zone")) r.zone = &(zone_table.at(j["zone"].get<zone_vnum>()));

    if(j.contains("sector_type")) r.sector_type = j["sector_type"];

    if(j.contains("room_flags")) r.room_flags = j["room_flags"].get<FlagHandler<RoomFlag>>();

    if(j.contains("proto_script")) {
        for(auto p : j["proto_script"]) r.proto_script.emplace_back(p.get<trig_vnum>());
    }
}

void load_rooms(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "rooms.json")) {
        auto id = j["id"].get<int64_t>();
        auto r = std::make_shared<Room>();
        j.get_to(*r);
        r->id = id;
        r->vn = id;
        units.emplace(id, r);
        world.emplace(id, r);
        r->zone->rooms.push_back(r);
        r->activate();
    }
}

void load_exits(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "exits.json")) {
        auto id = j["room"].get<int64_t>();
        auto dir = j["direction"].get<Direction>();
        auto r = get_room(id);
        auto &ex = r->exits[dir];
        j["data"].get_to(ex);
    }
}

static void dump_exits(const std::filesystem::path &loc) {
    json exits;

    for(auto &[v, r] : world) {

        for(auto& [d, e] : r->getDirections()) {
            json j2;
            j2["room"] = v;
            j2["direction"] = d;
            j2["data"] = e;
            exits.push_back(j2);
        }

    }
    dump_to_file(loc, "exits.json", exits);
}

static void dump_rooms(const std::filesystem::path &loc) {
    json rooms;

    for(auto &[v, r] : world) {
        rooms.push_back(*r);
    }
    dump_to_file(loc, "rooms.json", rooms);
}

// thing_data serialize/deserialize...
void to_json(json& j, const AbstractThing& t) {
    to_json(j, static_cast<const Entity&>(t));

    if(!t.stats.empty()) j["stats"] = t.stats;
    if(t.affect_flags) j["affect_flags"] = t.affect_flags;
}

void from_json(const json& j, AbstractThing& t) {
    from_json(j, static_cast<Entity&>(t));

    if(j.contains("stats")) t.stats = j["stats"];
    if(j.contains("affect_flags")) t.affect_flags = j["affect_flags"].get<FlagHandler<AffectFlag>>();
}

// Object serialize/deserialize...

void to_json(json& j, const ObjectPrototype& o) {
    to_json(j, static_cast<const ThingPrototype&>(o));
    to_json(j, static_cast<const picky_data&>(o));

    j["type_flag"] = o.type_flag;
    if(o.wear_flags) j["wear_flags"] = o.wear_flags;
    if(o.item_flags) j["item_flags"] = o.item_flags;

    for(auto & i : o.affected) {
        if(i.location == APPLY_NONE) continue;
        j["affected"].push_back(i);
    }

};

void to_json(json& j, const Object& o) {
    to_json(j, static_cast<const picky_data&>(o));
    to_json(j, static_cast<const AbstractThing&>(o));

    j["type_flag"] = o.type_flag;
    if(o.wear_flags) j["wear_flags"] = o.wear_flags;
    if(o.item_flags) j["item_flags"] = o.item_flags;

    for(auto & i : o.affected) {
        if(i.location == APPLY_NONE) continue;
        j["affected"].push_back(i);
    }

    if(get_room(o.room_loaded)) j["room_loaded"] = o.room_loaded;
}

void from_json(const json& j, ObjectPrototype& o) {
    from_json(j, static_cast<picky_data&>(o));
    from_json(j, static_cast<ThingPrototype&>(o));

    if(j.contains("type_flag")) o.type_flag = j["type_flag"];
    if(j.contains("wear_flags")) o.wear_flags = j["wear_flags"].get<FlagHandler<WearFlag>>();
    if(j.contains("item_flags")) o.item_flags = j["item_flags"].get<FlagHandler<ItemFlag>>();

    if(j.contains("affected")) {
        int counter = 0;
        for(auto & i : j["affected"]) {
            i.get_to(o.affected[counter]);
            counter++;
        }
    }

    if ((GET_OBJ_TYPE(&o) == ITEM_PORTAL || \
        GET_OBJ_TYPE(&o) == ITEM_HATCH) && \
        (!GET_OBJ_VAL(&o, VAL_DOOR_DCLOCK) || \
            !GET_OBJ_VAL(&o, VAL_DOOR_DCHIDE))) {
                for(const auto v : {VAL_DOOR_DCLOCK, VAL_DOOR_DCHIDE}) {
                    SET_OBJ_VAL(&o, v, 20);
                }
        }

    /* check to make sure that weight of containers exceeds curr. quantity */
        if (GET_OBJ_TYPE(&o) == ITEM_DRINKCON ||
            GET_OBJ_TYPE(&o) == ITEM_FOUNTAIN) {
            if (GET_OBJ_WEIGHT(&o) < GET_OBJ_VAL(&o, VAL_FOUNTAIN_HOWFULL))
                o.setBaseStat<weight_t>("weight", GET_OBJ_VAL(&o, VAL_FOUNTAIN_HOWFULL) + 5);
        }
        /* *** make sure portal objects have their timer set correctly *** */
        if (GET_OBJ_TYPE(&o) == ITEM_PORTAL) {
            o.setBaseStat<int>("timer", -1);
        }
}

void from_json(const json& j, Object& o) {
    from_json(j, static_cast<picky_data&>(o));
    from_json(j, static_cast<AbstractThing&>(o));

    if(j.contains("type_flag")) o.type_flag = j["type_flag"];

    if(j.contains("wear_flags")) o.wear_flags = j["wear_flags"].get<FlagHandler<WearFlag>>();

    if(j.contains("item_flags")) o.item_flags = j["item_flags"].get<FlagHandler<ItemFlag>>();

    if(j.contains("affected")) {
        int counter = 0;
        for(auto & i : j["affected"]) {
            i.get_to(o.affected[counter]);
            counter++;
        }
    }

    // this is an instance.
    if(j.contains("generation")) o.generation = j["generation"];

    if(j.contains("room_loaded")) o.room_loaded = j["room_loaded"];

}

void load_item_prototypes(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "itemPrototypes.json")) {
        auto id = j["vn"].get<int64_t>();
        if(id <= 0) {
            throw std::runtime_error("Invalid item prototype vnum: " + std::to_string(id));
        }
        auto p = obj_proto.emplace(id, j);
        auto zone = real_zone_by_thing(id);
        auto &i = obj_index[id];
        i.vn = id;
        
        auto& z = zone_table.at(zone);
        z.objects.insert(id);
    }
}

void load_items_initial(const std::filesystem::path& loc) {
    for(const auto j : load_from_file(loc, "items.json")) {
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<int>();
        auto data = j["data"];
        auto sh = std::make_shared<Object>();
        data.get_to(*sh);
        uniqueObjects.emplace(id, sh);
        units.emplace(id, sh);
    }
    basic_mud_log("Loaded %d items", uniqueObjects.size());
}

static void deserialize_obj_relations(Object* o, const json& j) {
    if(j.contains("posted_to")) {
        auto check = resolveUID(j["posted_to"]);
        if(check) o->posted_to = std::dynamic_pointer_cast<Object>(check).get();
    }
    if(j.contains("fellow_wall")) {
        auto check = resolveUID(j["fellow_wall"]);
        if(check) o->fellow_wall = std::dynamic_pointer_cast<Object>(check).get();
    }
}

void load_items_finish(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "items.json")) {
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<int>();
        if(auto cf = uniqueObjects.find(id); cf != uniqueObjects.end()) {
            if(auto i = cf->second) {
                deserialize_obj_relations(i.get(), j["relations"]);
                auto jloc = j["location"];
                Location loc;
                jloc.get_to(loc);
                i->setLocation(loc);
            }
        }
    }
}

static json serialize_obj_relations(const Object* o) {
    auto j = json::object();

    if(o->posted_to) j["posted_to"] = o->posted_to->getUID();
    if(o->fellow_wall) j["fellow_wall"] = o->fellow_wall->getUID();

    return j;
}


static void dump_items(const std::filesystem::path &loc) {
    json j;

    for(auto &[v, r] : uniqueObjects) {
        json j2;
        j2["id"] = v;
        j2["generation"] = static_cast<int32_t>(r->generation);
        j2["data"] = *r;
        j2["location"] = r->location;
        j2["relations"] = serialize_obj_relations(r.get());
        j.push_back(j2);
    }
    dump_to_file(loc, "items.json", j);
}


static void dump_item_prototypes(const std::filesystem::path &loc) {
    json j;
    for(auto &[v, o] : obj_proto) {
        if(v <= 0) throw std::runtime_error("Invalid item prototype vnum: " + std::to_string(v));
        if(o.vn <= 0) throw std::runtime_error("Invalid item prototype vn: " + std::to_string(o.vn));
        j.push_back(o);
    }
    dump_to_file(loc, "itemPrototypes.json", j);
}

// Character serialize/deserialize...
void to_json(json& j, const trans_data& t) {
    if(t.time_spent_in_form != 0.0) j["time_spent_in_form"] = t.time_spent_in_form;
    j["visible"] = t.visible;
    j["limit_broken"] = t.limit_broken;
    j["unlocked"] = t.unlocked;
    j["grade"] = t.grade;
    if(!t.vars.empty()) j["vars"] = t.vars;
    if(!t.description.empty()) j["description"] = t.description;
    if(!t.appearances.empty()) j["appearances"] = t.appearances;
}

void from_json(const json& j, trans_data& t) {
    if(j.contains("time_spent_in_form")) t.time_spent_in_form = j["time_spent_in_form"];
    if(j.contains("visible")) t.visible = j["visible"];
    if(j.contains("limit_broken")) t.limit_broken = j["limit_broken"];
    if(j.contains("unlocked")) t.unlocked = j["unlocked"];
    if(j.contains("grade")) t.grade = j["grade"];
    if(j.contains("vars")) t.vars = j["vars"];
    if(j.contains("description")) t.description = j["description"].get<std::string>();
    if(j.contains("appearances")) t.appearances = j["appearances"];
}

void to_json(json& j, const affected_type& a) {
    to_json(j, static_cast<const affect_t&>(a));
    if(a.type) j["type"] = a.type;
    if(a.duration) j["duration"] = a.duration;
    if(a.bitvector) j["bitvector"] = a.bitvector;
}

void from_json(const json& j, affected_type& a) {
    from_json(j, static_cast<affect_t&>(a));
    if(j.contains("type")) a.type = j["type"];
    if(j.contains("duration")) a.duration = j["duration"];
    if(j.contains("bitvector")) a.bitvector = j["bitvector"];
}

void to_json(json& j, const CharacterPrototype& c) {
    to_json(j, static_cast<const ThingPrototype&>(c));
    j["sex"] = c.sex;
    if(c.character_flags) j["character_flags"] = c.character_flags;
    if(c.mob_flags) j["mob_flags"] = c.mob_flags;
    j["race"] = c.race;
    j["sensei"] = c.sensei;
    json ms;
    to_json(ms, c.mob_specials);
    if(!ms.empty()) j["mob_specials"] = ms;
}

void from_json(const json& j, CharacterPrototype& c) {
    from_json(j, static_cast<ThingPrototype&>(c));
    if(j.contains("sex")) c.sex = j["sex"];
    if(j.contains("character_flags")) c.character_flags = j["character_flags"];
    if(j.contains("mob_flags")) c.mob_flags = j["mob_flags"];
    if(j.contains("race")) c.race = j["race"];
    if(j.contains("sensei")) c.sensei = j["sensei"];
    if(j.contains("mob_specials")) from_json(j["mob_specials"], c.mob_specials);

    if(j.contains("proto_script")) c.proto_script = j["proto_script"].get<std::vector<trig_vnum>>();

    if (c.race != Race::human)
        c.affect_flags.set(AFF_INFRAVISION, true);

    c.mob_flags.set(MOB_NOTDEADYET, false);

}

void to_json(json& j, const Character& c) {
    to_json(j, static_cast<const AbstractThing&>(c));

    j["sex"] = c.sex;
    if(!c.appearances.empty()) j["appearances"] = c.appearances;

    if(c.character_flags) j["character_flags"] = c.character_flags;
    if(c.mob_flags) j["mob_flags"] = c.mob_flags;
    if(c.player_flags) j["player_flags"] = c.player_flags;

    if(c.pref_flags) j["pref_flags"] = c.pref_flags;

    for(auto i = 0; i < c.bodyparts.size(); i++)
        if(c.bodyparts.test(i)) {
            j["bodyparts"].push_back(i);
        }

    if(c.title && strlen(c.title)) j["title"] = c.title;
    j["race"] = c.race;
    j["sensei"] = c.sensei;

    if(c.admin_flags) j["admin_flags"] = c.admin_flags;

    json td;
    to_json(td, c.time);
    if(!td.empty()) j["time"] = td;

    for(auto i = 0; i < 4; i++) {
        if(c.limb_condition[i]) {
            j["limb_condition"].push_back(std::make_pair(i, c.limb_condition[i]));
        }
    }

    for(auto i = 0; i < NUM_CONDITIONS; i++) {
        if(c.conditions[i]) j["conditions"].push_back(std::make_pair(i, c.conditions[i]));
    }

    for(auto i = 0; i < c.gravAcclim.size() ; i++) {
        if(c.gravAcclim[i]) j["gravAcclim"].push_back(std::make_pair(i, c.gravAcclim[i]));
    }

    auto ch = (Character*)&c;
    
    std::erase_if(ch->skill, [](const auto &s) { return s.second.level == 0 && s.second.perfs == 0; });
    if(!c.skill.empty()) j["skill"] = c.skill;

    for(auto a = c.affected; a; a = a->next) {
        if(a->type) {
            j["affected"].push_back(*a);
        }
    }

    for(auto a = c.affectedv; a; a = a->next) {
        if(a->type) j["affectedv"].push_back(*a);
    }

    for(auto i = 0; i < 5; i++) {
        if(c.lboard[i]) j["lboard"].push_back(std::make_pair(i, c.lboard[i]));
    }

    if(c.clan && strlen(c.clan)) j["clan"] = c.clan;
    if(c.mutations) j["mutations"] = c.mutations;
    if(c.bio_genomes) j["bio_genomes"] = c.bio_genomes;

    if(c.mimic) j["mimic"] = c.mimic.value();
    j["form"] = c.form;

    if(c.rdisplay) j["rdisplay"] = c.rdisplay;
    if(c.feature) j["feature"] = c.feature;

    if(c.voice && strlen(c.voice)) j["voice"] = c.voice;

    if (c.poofin && strlen(c.poofin)) j["poofin"] = c.poofin;
    if (c.poofout && strlen(c.poofout)) j["poofout"] = c.poofout;

    if(!c.transforms.empty()) j["transforms"] = c.transforms;

    if(!c.permForms.empty()) j["permForms"] = c.permForms;

}

void from_json(const json& j, Character& c) {
    from_json(j, static_cast<AbstractThing&>(c));

    if(j.contains("sex")) c.sex = j["sex"];
    if(j.contains("appearances")) c.appearances = j["appearances"];

    if(j.contains("title")) c.title = strdup(j["title"].get<std::string>().c_str());
    if(j.contains("race")) c.race = j["race"];

    if(j.contains("sensei")) c.sensei = j["sensei"];

    if(j.contains("mob_specials")) j["mob_specials"].get_to(c.mob_specials);

    if(j.contains("character_flags")) c.character_flags = j["character_flags"].get<FlagHandler<CharacterFlag>>();
    if(j.contains("mob_flags")) c.mob_flags = j["mob_flags"].get<FlagHandler<MobFlag>>();
    if(j.contains("player_flags")) c.player_flags = j["player_flags"].get<FlagHandler<PlayerFlag>>();

    if(j.contains("pref_flags")) c.pref_flags = j["pref_flags"].get<FlagHandler<PrefFlag>>();
    if(j.contains("bodyparts")) for(auto &i : j["bodyparts"]) c.bodyparts.set(i.get<int>());

    if(j.contains("admin_flags")) c.admin_flags = j["admin_flags"].get<FlagHandler<AdminFlag>>();;

    if(j.contains("time")) {
        j["time"].get_to(c.time);
    }

    if(j.contains("limb_condition")) {
        for(auto &i : j["limb_condition"]) {
            c.limb_condition[i[0].get<int>()] = i[1];
        }
    }

    if(j.contains("skill")) c.skill = j["skill"].get<std::map<Skill, skill_data>>();

    if(j.contains("affected")) {
        auto ja = j["affected"];
        // reverse iterate using .rbegin() and .rend() while filling out
        // the linked list.
        for(auto it = ja.rbegin(); it != ja.rend(); ++it) {
            auto a = new affected_type(*it);
            a->next = c.affected;
            c.affected = a;
        }
    }

    if(j.contains("affectedv")) {
        auto ja = j["affectedv"];
        // reverse iterate using .rbegin() and .rend() while filling out
        // the linked list.
        for(auto it = ja.rbegin(); it != ja.rend(); ++it) {
            auto a = new affected_type(*it);
            a->next = c.affectedv;
            c.affectedv = a;
        }
    }


    if(j.contains("lboard")) {
        for(auto &i : j["lboard"]) {
            c.lboard[i[0].get<int>()] = i[1];
        }
    }

    if(j.contains("clan")) c.clan = strdup(j["clan"].get<std::string>().c_str());
    if(j.contains("crank")) c.crank = j["crank"];

    if(j.contains("conditions")) {
        for(auto &i : j["conditions"]) {
            c.conditions[i[0].get<int>()] = i[1];
        }
    }

    if(j.contains("gravAcclim")) {
        for(auto &i : j["gravAcclim"]) {
            c.gravAcclim[i[0].get<int>()] = i[1];
        }
    }

    if(j.contains("bio_genomes")) c.bio_genomes = j["bio_genomes"];
    if(j.contains("mutations")) c.mutations = j["mutations"];

    if(j.contains("mimic")) c.mimic = j["mimic"].get<Race>();
    if(j.contains("rdisplay")) c.rdisplay = strdup(j["rdisplay"].get<std::string>().c_str());
    if(j.contains("feature")) c.feature = strdup(j["feature"].get<std::string>().c_str());

    if(j.contains("voice")) c.voice = strdup(j["voice"].get<std::string>().c_str());

    if(j.contains("form")) c.form = j["form"];
    if(j.contains("transforms")) c.transforms = j["transforms"];
    if(j.contains("permForms")) c.permForms = j["permForms"].get<std::unordered_set<Form>>();

}

static json serialize_char_location(Character* ch) {
    auto j = json::object();

    // PCs have special handling. NPCs just use the normal approach.
    if(!IS_NPC(ch)) {
        auto r = ch->location.getVnum() != NOWHERE ? ch->location.getVnum() : ch->getBaseStat<room_vnum>("was_in_room");
        if(!ch->desc) {
            r = ch->getBaseStat<room_vnum>("load_room");
        }
        j["load_room"] = ch->normalizeLoadRoom(r);
    }

    return j;
}

static void dump_characters(const std::filesystem::path &loc) {
    json j;

    for(auto &[v, r] : uniqueCharacters) {
        json j2;
        j2["id"] = v;
        j2["generation"] = r->generation;
        j2["data"] = *r;
        if(r->location) {
            // PCs who aren't logged in won't have a valid location.
            // So we only care about those who do.
            j2["location"] = r->location;
        }
        //j2["relations"] = r->serializeRelations();
        j.push_back(j2);
    }
    dump_to_file(loc, "characters.json", j);

}

static void dump_npc_prototypes(const std::filesystem::path &loc) {
    json j;

    for(auto &[v, n] : mob_proto) {
        if(v <= 0) throw std::runtime_error("Invalid NPC prototype vnum: " + std::to_string(v));
        if(n.vn <= 0) throw std::runtime_error("Invalid NPC prototype vn: " + std::to_string(n.vn));
        j.push_back(n);
    }
    dump_to_file(loc, "npcPrototypes.json", j);
}

void load_characters_initial(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "characters.json")) {
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<time_t>();
        auto data = j["data"];
        auto c = std::make_shared<Character>();
        j["data"].get_to(*c);
        if(auto isPlayer = players.find(id); isPlayer != players.end()) {
            isPlayer->second.character = c.get();
        }
        uniqueCharacters.emplace(id, c);
        units.emplace(id, c);
    }
}

void load_characters_finish(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "characters.json")) {
        if(!j.contains("location")) {
            // this can happen with PCs.
            continue;
        }
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<time_t>();
        //basic_mud_log("Finishing Character %d", id);
        if(auto cf = uniqueCharacters.find(id); cf != uniqueCharacters.end()) {
            Location loc;
            j.at("location").get_to(loc);
            cf->second->setLocation(loc);
        }
    }
}

void load_npc_prototypes(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "npcPrototypes.json")) {
        auto id = j["vn"].get<int64_t>();
        if(id <= 0) {
            throw std::runtime_error("Invalid NPC prototype vnum: " + std::to_string(id));
        }
        auto p = mob_proto.emplace(id, j);
        auto zone = real_zone_by_thing(id);
        auto &i = mob_index[id];
        i.vn = id;
        auto& z = zone_table.at(zone);
        z.mobiles.insert(id);
    }
}

// players data serialize/deserialize...
void to_json(json& j, const PlayerData& p) {
    j["id"] = p.id;
    j["name"] = p.name;
    if(p.account) j["account"] = p.account->id;

    for(auto &a : p.aliases) {
        j["aliases"].push_back(a);
    }

    for(auto &i : p.sense_player) {
        j["sensePlayer"].push_back(i);
    }

    for(auto &i : p.sense_memory) {
        j["senseMemory"].push_back(i);
    }

    for(auto &i : p.dub_names) {
        j["dubNames"].push_back(i);
    }

    for(auto i = 0; i < NUM_COLOR; i++) {
        if(p.color_choices[i] && strlen(p.color_choices[i])) j["color_choices"].push_back(std::make_pair(i, p.color_choices[i]));
    }
}

void from_json(const json& j, PlayerData& p) {
    p.id = j["id"];
    p.name = j["name"].get<std::string>();
    if(j.contains("account")) {
        auto accID = j["account"].get<vnum>();
        auto accFind = accounts.find(accID);
        if(accFind != accounts.end()) p.account = &accFind->second;
    }

    if(j.contains("aliases")) {
        for(auto ja : j["aliases"]) {
            p.aliases.emplace_back(ja);
        }
    }

    if(j.contains("sensePlayer")) {
        for(auto &i : j["sensePlayer"]) {
            p.sense_player.insert(i.get<int64_t>());
        }
    }

    if(j.contains("senseMemory")) {
        for(auto &i : j["senseMemory"]) {
            p.sense_memory.insert(i.get<vnum>());
        }
    }

    if(j.contains("dubNames")) {
        for(auto &i : j["dubNames"]) {
            p.dub_names.emplace(i[0].get<int64_t>(), i[1].get<std::string>());
        }
    }

    if(j.contains("color_choices")) {
        for(auto &i : j["color_choices"]) {
            p.color_choices[i[0].get<int>()] = strdup(i[1].get<std::string>().c_str());
        }
    }
}

static void dump_players(const std::filesystem::path &loc) {
    json j;

    for(auto &[v, r] : players) {
        j.push_back(r);
    }
    dump_to_file(loc, "players.json", j);
}

void load_players(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "players.json")) {
        auto id = j["id"].get<int64_t>();
        players.emplace(id, j);
    }
}

static std::vector<std::filesystem::path> getDumpFiles() {
    std::filesystem::path dir = "data/dumps"; // Change to your directory
    std::vector<std::filesystem::path> directories;

    auto pattern = "dump-";
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_directory() && entry.path().filename().string().starts_with(pattern)) {
            directories.push_back(entry.path());
        }
    }

    std::sort(directories.begin(), directories.end(), std::greater<>());
    return directories;
}

static void cleanup_state() {
    auto vecFiles = getDumpFiles();
    std::list<std::filesystem::path> files(vecFiles.begin(), vecFiles.end());

    // If we have more than x state files, we want to purge the oldest one(s) until we have just x.
    while(files.size() > 200) {
        std::filesystem::remove_all(files.back());
        files.pop_back();
    }
}

void to_json(json& j, const struct help_index_element& r) {
    if(r.index && strlen(r.index)) j["index"] = r.index;
    if(r.keywords && strlen(r.keywords)) j["keywords"] = r.keywords;
    if(r.entry && strlen(r.entry)) j["entry"] = r.entry;
    if(r.duplicate) j["duplicate"] = r.duplicate;
    if(r.min_level) j["min_level"] = r.min_level;
}

void from_json(const json& j, struct help_index_element& r) {
    if(j.contains("index")) {
        auto s = j["index"].get<std::string>();
        if(r.index) free(r.index);
        r.index = strdup(s.c_str());
    }
    if(j.contains("keywords")) {
        auto s = j["keywords"].get<std::string>();
        if(r.keywords) free(r.keywords);
        r.keywords = strdup(s.c_str());
    }
    if(j.contains("entry")) {
        auto s = j["entry"].get<std::string>();
        if(r.entry) free(r.entry);
        r.entry = strdup(s.c_str());
    }
    if(j.contains("duplicate")) r.duplicate = j["duplicate"].get<int>();
    if(j.contains("min_level")) r.min_level = j["min_level"].get<int>();
}

static void dump_help(const std::filesystem::path& loc) {
    auto j = json::array();
    for(auto i = 0; i < top_of_helpt; i++) {
        j.push_back(help_table[i]);
    }
    dump_to_file(loc, "help.json", j);
}

void load_help(const std::filesystem::path& loc) {
    auto data = load_from_file(loc, "help.json");
    top_of_helpt = data.size();

    // allocate the help_table with the size of top_of_helpt
    CREATE(help_table, struct help_index_element, top_of_helpt);
    auto i = 0;
    for(auto j : data) {
        from_json(j, help_table[i++]);
    }
}

static void dump_assemblies(const std::filesystem::path &loc) {
    auto j = json::array();

    if(g_pAssemblyTable) {
        for(auto i = 0; i < g_lNumAssemblies; i++) {
            auto a = json::object();
            a["vnum"] = g_pAssemblyTable[i].lVnum;
            a["assembly_type"] = g_pAssemblyTable[i].uchAssemblyType;

            for(auto k = 0; k < g_pAssemblyTable[i].lNumComponents; k++) {
                auto comp = json::object();
                comp["bExtract"] = g_pAssemblyTable[i].pComponents[k].bExtract;
                comp["bInRoom"] = g_pAssemblyTable[i].pComponents[k].bInRoom;
                comp["lVnum"] = g_pAssemblyTable[i].pComponents[k].lVnum;
                a["components"].push_back(comp);
            }
            j.push_back(a);
        }
    }


    dump_to_file(loc, "assemblies.json", j);
}

void load_assemblies(const std::filesystem::path &loc) {
    auto data = load_from_file(loc, "assemblies.json");

    for(auto j : data) {
        auto vn = j.at("vnum").get<vnum>();
        auto atype = j.at("assembly_type").get<int>();
        assemblyCreate(vn, atype);
        if(j.contains("components")) {
            // components are an array containing bExtract, bInRoom, and vnum lVnum...
            for(auto comp : j.at("components")) {
                auto bExtract = comp.at("bExtract").get<bool>();
                auto bInRoom = comp.at("bInRoom").get<bool>();
                auto lVnum = comp.at("lVnum").get<vnum>();
                assemblyAddComponent(vn, lVnum, bExtract, bInRoom);
            }
        }
    }

}

// This is called by the Cython code to create a new player character.
// Everything should be already validated several times over.
PlayerData* create_player_character(int account_id, const json& j) {
    auto &acc = accounts[account_id];
    auto ch = std::make_shared<Character>();
    ch->id = getNextUnitID();
    ch->generation = time(nullptr);
    auto &p = players[ch->id];
    p.id = ch->id;
    p.account = &acc;
    p.character = ch.get();
    p.name = j.at("name").get<std::string>();
    ch->strings["name"] = p.name;

    acc.characters.push_back(ch->id);

    if(j.contains("sex")) ch->sex = j["sex"];
    if(j.contains("race")) ch->race = j["race"];
    if(j.contains("sensei")) ch->sensei = j["sensei"];
    if(j.contains("bio_genomes")) ch->bio_genomes = j["bio_genomes"];
    if(j.contains("mutations")) ch->mutations = j["mutations"];
    if(j.contains("align")) ch->setBaseStat("good_evil", j["align"].get<int>());

    if(j.contains("keep_skills") && !j.at("keep_skills").get<bool>()) {
        ch->modBaseStat("practices", 200);
        ch->player_flags.set(PlayerFlag::forgetting_skill);
    }

    uniqueCharacters.emplace(ch->id, ch);
    units.emplace(ch->id, ch);

    init_char(ch.get());

    send_to_imm("New character created, %s, by user, %s.", GET_NAME(ch), acc.name.c_str());

    return &p;
}

void runSave() {
    basic_mud_log("Beginning dump of state to disk.");
    // Open up a new database file as <cwd>/state/<timestamp>.sqlite3 and dump the state into it.
    auto path = std::filesystem::current_path() / "data" / "dumps";
    std::filesystem::create_directories(path);
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = *std::localtime(&time_t_now);

    auto tempPath = path / "temp";
    std::filesystem::remove_all(tempPath);
    std::filesystem::create_directories(tempPath);

    auto newPath = path / fmt::format("dump-{:04}{:02}{:02}{:02}{:02}{:02}",
                                      tm_now.tm_year + 1900,
                                      tm_now.tm_mon + 1,
                                      tm_now.tm_mday,
                                      tm_now.tm_hour,
                                      tm_now.tm_min,
                                      tm_now.tm_sec);

    double duration{};
    bool failed = false;
    try {
        auto startTime = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        for(const auto func : {dump_accounts, dump_characters,
              dump_players, dump_dgscripts, dump_help, dump_assemblies,
                          dump_items, dump_globaldata,
                          dump_rooms, dump_exits, dump_item_prototypes,
                          dump_npc_prototypes, dump_shops,
                          dump_guilds, dump_zones,
                          dump_dgscript_prototypes}) {
            threads.emplace_back([func, &tempPath]() {
                try {
                    func(tempPath);
                } catch (const std::exception& e) {
                    basic_mud_log("Error during dump: %s", e.what());
                    throw;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();

        auto endTime = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration<double>(endTime - startTime).count();

    } catch (std::exception &e) {
        basic_mud_log("(GAME HAS NOT BEEN SAVED!) Exception in dump_state(): %s", e.what());
        send_to_all("Warning, a critical error occurred during save! Please alert staff!\r\n");
        failed = true;
    }
    if(failed) return;

    std::filesystem::rename(tempPath, newPath);
    basic_mud_log("Finished dumping state to %s in %f seconds.", newPath.string(), duration);
    cleanup_state();
}

void rest_post_script(int accountID, int scriptID, const std::string& data) {
    auto &acc = accounts.at(accountID);
    json j;
    try {
        j = json::parse(data);
    } catch (const json::parse_error& e) {
        basic_mud_log("Error parsing JSON data for script %d: %s", scriptID, e.what());
        return;
    }
    if(trig_index.contains(scriptID)) {
        auto &trig = trig_index.at(scriptID);
        // TODO: Post the data to the trigger

        basic_mud_log("%s updated DgScript %d: '%s'", acc.name.c_str(), scriptID, trig.name);
        
    } else {
        auto &trig = trig_index[scriptID];
        j.get_to(trig);
        basic_mud_log("%s created DgScript %d: '%s'", acc.name.c_str(), scriptID, trig.name);
    }
}