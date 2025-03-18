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

void to_json(json& j, const zone_data& z) {
    j["number"] = z.number;
    if(z.name && std::strlen(z.name))
        j["name"] = z.name;
    if(z.builders && std::strlen(z.builders))
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
    for(auto i = 0; i < NUM_ZONE_FLAGS; i++) {
        if(IS_SET_AR(z.zone_flags, i)) {
            j["zone_flags"].push_back(i);
            std::string key(zone_bits[i]);
            boost::to_lower(key);
            j["zone_flags_name"].push_back(key);
        }
    }

    // Serialize command vector (assuming each element in 'cmd' is of type reset_com)
    for(const auto &c : z.cmd) {
        j["cmd"].push_back(c);  // This will call to_json(reset_com) automatically.
    }
}

void from_json(const json& j, zone_data& z) {
    if(j.contains("number"))
        z.number = j["number"];
    if(j.contains("name"))
        z.name = strdup(j["name"].get<std::string>().c_str());
    if(j.contains("builders"))
        z.builders = strdup(j["builders"].get<std::string>().c_str());
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

    if(j.contains("zone_flags")) {
        for(auto &f : j["zone_flags"]) {
            SET_BIT_AR(z.zone_flags, f.get<int>());
        }
    }

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
void to_json(json& j, const account_data& a) {
    if(a.vn != NOTHING)
        j["vn"] = a.vn;
    if(!a.name.empty())
        j["name"] = a.name;
    if(!a.passHash.empty())
        j["passHash"] = a.passHash;
    if(!a.email.empty())
        j["email"] = a.email;
    if(a.created)
        j["created"] = a.created;
    if(a.lastLogin)
        j["lastLogin"] = a.lastLogin;
    if(a.lastLogout)
        j["lastLogout"] = a.lastLogout;
    if(a.lastPasswordChanged)
        j["lastPasswordChanged"] = a.lastPasswordChanged;
    if(a.totalPlayTime != 0.0)
        j["totalPlayTime"] = a.totalPlayTime;
    if(!a.disabledReason.empty())
        j["disabledReason"] = a.disabledReason;
    if(a.disabledUntil)
        j["disabledUntil"] = a.disabledUntil;
    if(a.rpp)
        j["rpp"] = a.rpp;
    if(a.slots != 3)
        j["slots"] = a.slots;
    if(a.adminLevel)
        j["adminLevel"] = a.adminLevel;

    // For the characters field (vector<int>), assign it directly.
    j["characters"] = a.characters;
}

void from_json(const json& j, account_data& a) {
    if(j.contains("vn"))
        a.vn = j["vn"];
    if(j.contains("name"))
        a.name = j["name"];
    if(j.contains("passHash"))
        a.passHash = j["passHash"];
    if(j.contains("email"))
        a.email = j["email"];
    if(j.contains("created"))
        a.created = j["created"];
    if(j.contains("lastLogin"))
        a.lastLogin = j["lastLogin"];
    if(j.contains("lastLogout"))
        a.lastLogout = j["lastLogout"];
    if(j.contains("lastPasswordChanged"))
        a.lastPasswordChanged = j["lastPasswordChanged"];
    if(j.contains("totalPlayTime"))
        a.totalPlayTime = j["totalPlayTime"];
    if(j.contains("disabledReason"))
        a.disabledReason = j["disabledReason"];
    if(j.contains("disabledUntil"))
        a.disabledUntil = j["disabledUntil"];
    if(j.contains("rpp"))
        a.rpp = j["rpp"];
    if(j.contains("slots"))
        a.slots = j["slots"];
    if(j.contains("adminLevel"))
        a.adminLevel = j["adminLevel"];

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
        auto vn = acc.at("vn").get<int64_t>();
        accounts.emplace(vn, acc);
    }

}

// Triggers serialize/deserialize...
void to_json(json& j, const trig_var_data& t) {
    if(t.name && std::strlen(t.name))
        j["name"] = t.name;
    if(t.value && std::strlen(t.value))
        j["value"] = t.value;
    if(t.context)
        j["context"] = t.context;
}

void from_json(const json& j, trig_var_data& t) {
    if(j.contains("name"))
        t.name = strdup(j["name"].get<std::string>().c_str());
    if(j.contains("value"))
        t.value = strdup(j["value"].get<std::string>().c_str());
    if(j.contains("context"))
        t.context = j["context"].get<long>();
}

json serializeVars(struct trig_var_data *vd) {
    auto j = json::array();;
    for(auto v = vd; v; v = v->next) {
        j.push_back(*v);
    }
    return j;
}


void deserializeVars(struct trig_var_data **vd, const json &j) {
    for(auto it = j.rbegin(); it != j.rend(); ++it) {
        auto v = new trig_var_data();
        it->get_to(*v);
        v->next = *vd;
        *vd = v;
    }
}

void to_json(json& j, const trig_data& t) {
    if(t.id != NOTHING) {
        // we're serializing an instance.
        j["vn"] = t.vn;

        j["id"] = t.id;
        j["generation"] = t.generation;
        j["order"] = t.order;

        if(t.depth) j["depth"] = t.depth;
        if(t.loops) j["loops"] = t.loops;
        if(t.waiting != 0.0) j["waiting"] = t.waiting;

        if(!(t.curr_state == t.cmdlist || !t.curr_state)) {
            j["curr_state"] = t.countLine(t.curr_state);
            if(t.curr_state->original) j["curr_state_original"] = t.countLine(t.curr_state->original);
        }
        if(t.var_list) j["var_list"] = serializeVars(t.var_list);
    } else {
        // we're serializing a prototype.
        if(t.vn != NOTHING) j["vn"] = t.vn;
        if(t.name && strlen(t.name)) j["name"] = t.name;
        if(t.attach_type) j["attach_type"] = t.attach_type;
        if(t.data_type) j["data_type"] = t.data_type;
        if(t.trigger_type) j["trigger_type"] = t.trigger_type;
        if(t.narg) j["narg"] = t.narg;
        if(t.arglist && strlen(t.arglist)) j["arglist"] = t.arglist;

        for(auto c = t.cmdlist; c; c = c->next) {
            j["cmdlist"].push_back(c->cmd);
        }

    }
}

void from_json(const json& j, trig_data& t) {
    if(j.contains("id")) {
        // we're deserializing an instance.
        t.vn = j["vn"].get<int>();
        auto &tp = trig_index[t.vn];
        auto p = tp.proto;
        if(p->name && strlen(p->name)) t.name = strdup(p->name);
        t.attach_type = p->attach_type;
        t.data_type = p->data_type;
        t.trigger_type = p->trigger_type;
        t.narg = p->narg;
        if(p->arglist && strlen(p->arglist)) t.arglist = strdup(p->arglist);

        t.cmdlist = p->cmdlist;
        t.curr_state = p->cmdlist;

        if(j.contains("id")) t.id = j["id"].get<long>();
        if(j.contains("generation")) t.generation = j["generation"].get<time_t>();
        if(j.contains("order")) t.order = j["order"].get<long>();

        if(j.contains("waiting")) t.waiting = j["waiting"].get<double>();
        if(j.contains("depth")) t.depth = j["depth"].get<int>();
        if(j.contains("loops")) t.loops = j["loops"].get<int>();

        if(j.contains("curr_state")) {
            int curr_state_num = j["curr_state"].get<int>();
            if(curr_state_num > 0) {
                for(int i = 0; i < curr_state_num; i++) {
                    t.curr_state = t.curr_state->next;
                }
            }
        }

        if(j.contains("curr_state_original")) {
            int curr_state_num = j["curr_state_original"].get<int>();
            t.curr_state->original = t.cmdlist;
            if(curr_state_num > 0) {
                for(int i = 0; i < curr_state_num; i++) {
                    t.curr_state->original = t.curr_state->original->next;
                }
            }
        }

        if(j.contains("var_list")) {
            deserializeVars(&t.var_list, j["var_list"]);
        }
    } else {
        // we're deserializing a prototype.
        if(j.contains("vn")) t.vn = j["vn"].get<int>();
        if(j.contains("name")) t.name = strdup(j["name"].get<std::string>().c_str());
        if(j.contains("attach_type")) t.attach_type = j["attach_type"].get<int>();
        if(j.contains("data_type")) t.data_type = j["data_type"].get<int>();
        if(j.contains("trigger_type")) t.trigger_type = j["trigger_type"].get<int>();
        if(j.contains("narg")) t.narg = j["narg"].get<int>();
        if(j.contains("arglist")) t.arglist = strdup(j["arglist"].get<std::string>().c_str());

        if(j.contains("cmdlist")) {
            auto &cl = j["cmdlist"];
            for(auto c = cl.rbegin(); c != cl.rend(); c++) {
                auto cle = new cmdlist_element();
                cle->cmd = strdup(c->get<std::string>().c_str());
                cle->next = t.cmdlist;
                t.cmdlist = cle;
            }
        }
    }
}

void load_dgscript_prototypes(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "dgScriptPrototypes.json")) {
        auto id = j["vn"].get<int64_t>();
        auto &t = trig_index[id];
        t.vn = id;
        t.proto = new trig_data();
        j.get_to(*t.proto);
        auto zone = real_zone_by_thing(id);
        auto &z = zone_table[zone];
        z.triggers.insert(id);
    }
}

static void dump_dgscript_prototypes(const std::filesystem::path &loc) {
    json j;
    for(auto &[v, t] : trig_index) {
        const auto &tp = *t.proto;
        j.push_back(tp);
    }
    dump_to_file(loc, "dgScriptPrototypes.json", j);
}

void load_dgscripts_initial(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "dgscripts.json")) {
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<int>();
        auto t = std::make_shared<trig_data>();
        j["data"].get_to(*t);
        uniqueScripts.emplace(id, t);
    }
}

void load_dgscripts_finish(const std::filesystem::path& loc) {
    // Iterating through DgSCripts in reverse to ensure they're added in proper order.
    auto json_array = load_from_file(loc, "dgscripts.json");

    for (auto it = json_array.rbegin(); it != json_array.rend(); ++it) {
        auto& j = *it;
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<int>();
        auto location = j["location"].get<std::string>();

        if (auto cf = uniqueScripts.find(id); cf != uniqueScripts.end()) {
            if (auto t = cf->second) {
                auto o = t->owner = resolveUID(location);
                t->next = o->trig_list;
                o->trig_list = t.get();
                t->owner->trigger_types |= GET_TRIG_TYPE(t);
            }
        }
    }
}

static void dump_dgscripts(const std::filesystem::path &loc) {
    json j;

    for(auto &[v, r] : uniqueScripts) {
        json j2;
        j2["id"] = v;
        j2["generation"] = r->generation;
        j2["data"] = *r;
        j2["location"] = r->owner->getUID();
        j2["order"] = r->order;
        j.push_back(j2);
    }
    dump_to_file(loc, "dgscripts.json", j);
}

// shops serialize/deserialize...
void to_json(json&j, const shop_data& s) {
    j["vnum"] = s.vnum;
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
    if(s.bitvector) j["bitvector"] = s.bitvector;
    if(s.keeper != NOBODY) j["keeper"] = s.keeper;
    for(auto i = 0; i < 79; i++) if(IS_SET_AR(s.with_who, i)) j["with_who"].push_back(i);
    for(auto r : s.in_room) j["in_room"].push_back(r);
    if(s.open1) j["open1"] = s.open1;
    if(s.close1) j["close1"] = s.close1;
    if(s.open2) j["open2"] = s.open2;
    if(s.close2) j["close2"] = s.close2;
    if(s.bankAccount) j["bankAccount"] = s.bankAccount;
    if(s.lastsort) j["lastsort"] = s.lastsort;
}

void from_json(const json& j, shop_data& s) {
    if(j.contains("vnum")) s.vnum = j["vnum"];
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
    if(j.contains("bitvector")) s.bitvector = j["bitvector"];
    if(j.contains("keeper")) s.keeper = j["keeper"];
    if(j.contains("with_who")) {
        for(auto &w : j["with_who"]) {
            SET_BIT_AR(s.with_who, w.get<int>());
        }
    }
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
        auto &z = zone_table[zone];
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
    j["vnum"] = g.vnum;
    if(!g.skills.empty()) j["skills"] = g.skills;
    if(!g.feats.empty()) j["feats"] = g.feats;
    if(g.charge != 1.0) j["charge"] = g.charge;
    if(!g.no_such_skill.empty()) j["no_such_skill"] = g.no_such_skill;
    if(!g.not_enough_gold.empty()) j["not_enough_gold"] = g.not_enough_gold;
    if(g.minlvl) j["minlvl"] = g.minlvl;
    if(g.gm != NOBODY) j["gm"] = g.gm;
    for(auto i = 0; i < 79; i++) if(IS_SET_AR(g.with_who, i)) j["with_who"].push_back(i);
    if(g.open) j["open"] = g.open;
    if(g.close) j["close"] = g.close;
}

void from_json(const json& j, guild_data& g) {
    if(j.contains("vnum")) g.vnum = j["vnum"];
    if(j.contains("skills")) {
        auto skills = j["skills"].get<std::vector<int>>();
        for(auto s : skills) {
            g.skills.insert(s);
        }
    }
    if(j.contains("feats")) {
        auto feats = j["feats"].get<std::vector<int>>();
        for(auto f : feats) {
            g.feats.insert(f);
        }
    }
    if(j.contains("charge")) g.charge = j["charge"];
    if(j.contains("no_such_skill")) g.no_such_skill = j["no_such_skill"];
    if(j.contains("not_enough_gold")) g.not_enough_gold = j["not_enough_gold"];
    if(j.contains("minlvl")) g.minlvl = j["minlvl"];
    if(j.contains("gm")) g.gm = j["gm"];
    if(j.contains("with_who")) {
        for(auto &w : j["with_who"]) {
            SET_BIT_AR(g.with_who, w.get<int>());
        }
    }
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
        auto &z = zone_table[zone];
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
    if(j.contains("dgGlobals")) {
        if(auto room = get_room(0); room) {
            deserializeVars(&(room->global_vars), j["dgGlobals"]);
        }
    }
}

void dump_globaldata(const std::filesystem::path &loc) {
    json j;

    j["time"] = time_info;
    j["era_uptime"] = era_uptime;
    j["weather"] = weather_info;
    if(auto gRoom = get_room(0); gRoom) {
        if(gRoom->global_vars) {
            j["dgGlobals"] = serializeVars(gRoom->global_vars);
        }
    }

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
void to_json(json& j, const unit_data& u) {
    if(u.vn != NOTHING) j["vn"] = u.vn;
    if(u.id != NOTHING) {
        // an instance or room.
        j["id"] = u.id;
        j["generation"] = u.generation;
    }
    if(u.zone != NOTHING) j["zone"] = u.zone;

    if(u.proto) {
        if(u.name && u.name != u.proto->name) j["name"] = u.name;
        if(u.room_description && u.room_description != u.proto->room_description) j["room_description"] = u.room_description;
        if(u.look_description && u.look_description != u.proto->look_description) j["look_description"] = u.look_description;
        if(u.short_description && u.short_description != u.proto->short_description) j["short_description"] = u.short_description;
    } else {
        if(u.name && strlen(u.name)) j["name"] = u.name;
        if(u.room_description && strlen(u.room_description)) j["room_description"] = u.room_description;
        if(u.look_description && strlen(u.look_description)) j["look_description"] = u.look_description;
        if(u.short_description && strlen(u.short_description)) j["short_description"] = u.short_description;
        for(auto ex = u.ex_description; ex; ex = ex->next) {
            if(ex->keyword && strlen(ex->keyword) && ex->description && strlen(ex->description)) {
                j["ex_description"].push_back(*ex);
            }
        }
    }
}

void from_json(const json& j, unit_data& u) {
    if(j.contains("vn")) u.vn = j["vn"];
    if(j.contains("id")) u.id = j["id"];
    if(j.contains("generation")) u.generation = j["generation"];
    if(j.contains("zone")) u.zone = j["zone"];

    if(u.vn != NOTHING && u.id != NOTHING)
        switch(u.getType()) {
            case 0:
                break;
            case 1:
                u.proto = &(obj_proto.at(u.vn));
                break;
            case 2:
                u.proto = &(mob_proto.at(u.vn));
                break;
        }

    if(u.proto) {
        u.name = u.proto->name;
        u.room_description = u.proto->room_description;
        u.look_description = u.proto->look_description;
        u.short_description = u.proto->short_description;
        u.ex_description = u.proto->ex_description;
    }

    if(j.contains("name")) {
        u.name = strdup(j["name"].get<std::string>().c_str());
    }
    if(j.contains("room_description")) {
        u.room_description = strdup(j["room_description"].get<std::string>().c_str());
    }
    if(j.contains("look_description")) {
        u.look_description = strdup(j["look_description"].get<std::string>().c_str());
    }
    if(j.contains("short_description")) {
        u.short_description = strdup(j["short_description"].get<std::string>().c_str());
    }

    if(j.contains("ex_description")) {
        auto &e = j["ex_description"];
        for(auto ex = e.rbegin(); ex != e.rend(); ex++) {
            auto new_ex = new extra_descr_data();
            ex->get_to(*new_ex);
            new_ex->next = u.ex_description;
            u.ex_description = new_ex;
        }
    }
}

void to_json(json& j, const room_direction_data &e) {
    if(e.general_description && strlen(e.general_description)) j["general_description"] = e.general_description;
    if(e.keyword && strlen(e.keyword)) j["keyword"] = e.keyword;
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
	if(e.to_room != NOWHERE) j["to_room"] = e.to_room;
    if(e.dclock) j["dclock"] = e.dclock;
    if(e.dchide) j["dchide"] = e.dchide;
    if(e.dcskill) j["dcskill"] = e.dcskill;
    if(e.dcmove) j["dcmove"] = e.dcmove;
    if(e.failsavetype) j["failsavetype"] = e.failsavetype;
    if(e.dcfailsave) j["dcfailsave"] = e.dcfailsave;
    if(e.failroom > 0) j["failroom"] = e.failroom;
    if(e.totalfailroom > 0) j["totalfailroom"] = e.totalfailroom;
}

void from_json(const json& j, room_direction_data &e) {
    if(j.contains("general_description")) e.general_description = strdup(j["general_description"].get<std::string>().c_str());
    if(j.contains("keyword")) e.keyword = strdup(j["keyword"].get<std::string>().c_str());
    if(j.contains("exit_info")) e.exit_info = j["exit_info"].get<int16_t>();
    if(j.contains("key")) e.key = j["key"];
    if(j.contains("to_room")) e.to_room = j["to_room"];
    if(j.contains("dclock")) e.dclock = j["dclock"];
    if(j.contains("dchide")) e.dchide = j["dchide"];
    if(j.contains("dcskill")) e.dcskill = j["dcskill"];
    if(j.contains("dcmove")) e.dcmove = j["dcmove"];
    if(j.contains("failsavetype")) e.failsavetype = j["failsavetype"];
    if(j.contains("dcfailsave")) e.dcfailsave = j["dcfailsave"];
    if(j.contains("failroom")) e.failroom = j["failroom"];
    if(j.contains("totalfailroom")) e.totalfailroom = j["totalfailroom"];
}

void to_json(json& j, const room_data& r) {
    // we need to call the to_json for unit_data...
    to_json(j, static_cast<const unit_data&>(r));
    
    if(r.sector_type) j["sector_type"] = r.sector_type;
    auto sect_key = std::string(sector_types[r.sector_type]);
    boost::to_lower(sect_key);
    j["sector_type_name"] = sect_key;

    if(!r.room_flags.empty()) j["room_flags"] = r.room_flags;

    for(auto p : r.proto_script) {
        if(trig_index.contains(p)) j["proto_script"].push_back(p);
    }
}

void from_json(const json& j, room_data& r) {
    // call the from_json of unit_data...
    from_json(j, static_cast<unit_data&>(r));

    if(j.contains("sector_type")) r.sector_type = j["sector_type"];

    if(j.contains("dir_option")) {
        // this is an array of (<number>, <json>) pairs, with number matching the dir_option array index.
        // Thankfully we can pass the json straight into the room_direction_data constructor...
        for(auto &d : j["dir_option"]) {
            auto ex = new room_direction_data();
            d[1].get_to(*ex);
            r.dir_option[d[0]] = ex;
        }
    }

    if(j.contains("room_flags")) r.room_flags = j["room_flags"].get<std::unordered_set<RoomFlag>>();

    if(j.contains("proto_script")) {
        for(auto p : j["proto_script"]) r.proto_script.emplace_back(p.get<trig_vnum>());
    }
}

void load_rooms(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "rooms.json")) {
        auto id = j["vn"].get<int64_t>();
        auto r = std::make_shared<room_data>();
        j.get_to(*r);
        r->id = id;
        units.emplace(id, r);
        world.emplace(id, r);
        auto zone = real_zone_by_thing(id);
        auto &z = zone_table[zone];
        z.rooms.insert(id);
        r->zone = zone;
        r->activate();
    }
}

void load_exits(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "exits.json")) {
        auto id = j["room"].get<int64_t>();
        auto dir = j["direction"].get<int64_t>();
        auto r = get_room(id);
        auto ex = new room_direction_data();
        j["data"].get_to(*ex);
        r->dir_option[dir] = ex;
    }
}

static void dump_exits(const std::filesystem::path &loc) {
    json exits;

    for(auto &[v, r] : world) {

        for(auto i = 0; i < NUM_OF_DIRS; i++) {
            if(auto ex = r->dir_option[i]; ex) {
                json j2;
                j2["room"] = v;
                j2["direction"] = i;
                j2["direction_name"] = dirs[i];
                j2["data"] = *ex;
                exits.push_back(j2);
            }
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


// obj_data serialize/deserialize...

void to_json(json& j, const obj_data& o) {
    to_json(j, static_cast<const unit_data&>(o));

    if(!o.value.empty()) j["value"] = o.value;

    if(o.type_flag) j["type_flag"] = o.type_flag;
    if(o.level) j["level"] = o.level;

    for(auto i = 0; i < o.wear_flags.size(); i++)
        if(o.wear_flags.test(i)) j["wear_flags"].push_back(i);

    for(auto i = 0; i < o.extra_flags.size(); i++)
        if(o.extra_flags.test(i)) j["extra_flags"].push_back(i);

    for(auto i = 0; i < o.onlyAlignLawChaos.size(); i++)
        if(o.onlyAlignLawChaos.test(i)) j["onlyAlignLawChaos"].push_back(i);
    for(auto i = 0; i < o.antiAlignLawChaos.size(); i++)
        if(o.antiAlignLawChaos.test(i)) j["antiAlignLawChaos"].push_back(i);
    for(auto i = 0; i < o.onlyAlignGoodEvil.size(); i++)
        if(o.onlyAlignGoodEvil.test(i)) j["onlyAlignGoodEvil"].push_back(i);
    for(auto i = 0; i < o.antiAlignGoodEvil.size(); i++)
        if(o.antiAlignGoodEvil.test(i)) j["antiAlignGoodEvil"].push_back(i);
    for(auto i = 0; i < o.onlyClass.size(); i++)
        if(o.onlyClass.test(i)) j["onlyClass"].push_back(i);
    for(auto i = 0; i < o.antiClass.size(); i++)
        if(o.antiClass.test(i)) j["antiClass"].push_back(i);
    for(auto i = 0; i < o.onlyRace.size(); i++)
        if(o.onlyRace.test(i)) j["onlyRace"].push_back(i);
    for(auto i = 0; i < o.antiRace.size(); i++)
        if(o.antiRace.test(i)) j["antiRace"].push_back(i);

    if(o.weight != 0.0) j["weight"] = o.weight;
    if(o.cost) j["cost"] = o.cost;
    if(o.cost_per_day) j["cost_per_day"] = o.cost_per_day;

    for(auto i = 0; i < o.bitvector.size(); i++)
        if(o.bitvector.test(i)) j["bitvector"].push_back(i);

    for(auto & i : o.affected) {
        if(i.location == APPLY_NONE) continue;
        j["affected"].push_back(i);
    }

    if(o.id != NOTHING) {
        // this is an instance.
        if(o.global_vars) {
            j["dgvariables"] = serializeVars(o.global_vars);
        }
    
        if(get_room(o.room_loaded)) j["room_loaded"] = o.room_loaded;

    } else {
        // this is a prototype.
        for(auto p : o.proto_script) {
            if(trig_index.contains(p)) j["proto_script"].push_back(p);
        }
    }
}

void from_json(const json& j, obj_data& o) {
    from_json(j, static_cast<unit_data&>(o));

    if(j.contains("value")) o.value = j["value"].get<std::unordered_map<std::string, int64_t>>();

    if(j.contains("type_flag")) o.type_flag = j["type_flag"];
    if(j.contains("level")) o.level = j["level"];

    if(j.contains("wear_flags")) {
        for(auto & i : j["wear_flags"]) {
            o.wear_flags.set(i.get<int>());
        }
    }

    if(j.contains("extra_flags")) for(auto & i : j["extra_flags"]) o.extra_flags.set(i.get<int>());

    if(j.contains("onlyAlignLawChaos")) for(auto & i : j["onlyAlignLawChaos"]) o.onlyAlignLawChaos.set(i.get<int>());
    if(j.contains("antiAlignLawChaos")) for(auto & i : j["antiAlignLawChaos"]) o.antiAlignLawChaos.set(i.get<int>());
    if(j.contains("onlyAlignGoodEvil")) for(auto & i : j["onlyAlignGoodEvil"]) o.onlyAlignGoodEvil.set(i.get<int>());
    if(j.contains("antiAlignGoodEvil")) for(auto & i : j["antiAlignGoodEvil"]) o.antiAlignGoodEvil.set(i.get<int>());

    if(j.contains("onlyClass")) for(auto & i : j["onlyClass"]) o.onlyClass.set(i.get<int>());
    if(j.contains("antiClass")) for(auto & i : j["antiClass"]) o.antiClass.set(i.get<int>());
    if(j.contains("onlyRace")) for(auto & i : j["onlyRace"]) o.onlyRace.set(i.get<int>());
    if(j.contains("tiRace")) for(auto & i : j["tiRace"]) o.antiAlignGoodEvil.set(i.get<int>());

    if(j.contains("weight")) o.weight = j["weight"];
    if(j.contains("cost")) o.cost = j["cost"];
    if(j.contains("cost_per_day")) o.cost_per_day = j["cost_per_day"];

    if(j.contains("bitvector")) {
        for(auto & i : j["bitvector"]) {
            o.bitvector.set(i.get<int>());
        }
    }

    if(j.contains("affected")) {
        int counter = 0;
        for(auto & i : j["affected"]) {
            i.get_to(o.affected[counter]);
            counter++;
        }
    }

    if(o.id != NOTHING) {
        // this is an instance.
        if(j.contains("generation")) o.generation = j["generation"];

        if(j.contains("dgvariables")) {
            deserializeVars(&o.global_vars, j["dgvariables"]);
        }

        if(j.contains("room_loaded")) o.room_loaded = j["room_loaded"];

        auto proto = obj_proto.find(o.vn);
        if(proto != obj_proto.end()) {
            o.proto_script = proto->second.proto_script;
        }
    } else {
        // this is a prototype.
        if(j.contains("proto_script")) {
            for(auto p : j["proto_script"]) o.proto_script.emplace_back(p.get<trig_vnum>());
        }

            if ((GET_OBJ_TYPE(&o) == ITEM_PORTAL || \
        GET_OBJ_TYPE(&o) == ITEM_HATCH) && \
        (!GET_OBJ_VAL(&o, VAL_DOOR_DCLOCK) || \
            !GET_OBJ_VAL(&o, VAL_DOOR_DCHIDE))) {
                for(const auto v : {VAL_DOOR_DCLOCK, VAL_DOOR_DCHIDE}) {
                    SET_OBJ_VAL(&o, v, 20);
                }
        }

        GET_OBJ_SIZE(&o) = SIZE_MEDIUM;

    /* check to make sure that weight of containers exceeds curr. quantity */
        if (GET_OBJ_TYPE(&o) == ITEM_DRINKCON ||
            GET_OBJ_TYPE(&o) == ITEM_FOUNTAIN) {
            if (GET_OBJ_WEIGHT(&o) < GET_OBJ_VAL(&o, VAL_FOUNTAIN_HOWFULL))
                GET_OBJ_WEIGHT(&o) = GET_OBJ_VAL(&o, VAL_FOUNTAIN_HOWFULL) + 5;
        }
        /* *** make sure portal objects have their timer set correctly *** */
        if (GET_OBJ_TYPE(&o) == ITEM_PORTAL) {
            GET_OBJ_TIMER(&o) = -1;
        }
    }

}

void load_item_prototypes(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "itemPrototypes.json")) {
        auto id = j["vn"].get<int64_t>();
        auto p = obj_proto.emplace(id, j);
        auto zone = real_zone_by_thing(id);
        p.first->second.zone = zone;
        auto &i = obj_index[id];
        i.vn = id;

        auto &z = zone_table[zone];
        z.objects.insert(id);
    }
}

void load_items_initial(const std::filesystem::path& loc) {
    for(const auto j : load_from_file(loc, "items.json")) {
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<int>();
        auto data = j["data"];
        auto sh = std::make_shared<obj_data>();
        data.get_to(*sh);
        uniqueObjects.emplace(id, sh);
        units.emplace(id, sh);
    }
    basic_mud_log("Loaded %d items", uniqueObjects.size());
}

static void deserialize_obj_relations(obj_data* o, const json& j) {
    if(j.contains("posted_to")) {
        auto check = resolveUID(j["posted_to"]);
        if(check) o->posted_to = std::dynamic_pointer_cast<obj_data>(check).get();
    }
    if(j.contains("fellow_wall")) {
        auto check = resolveUID(j["fellow_wall"]);
        if(check) o->fellow_wall = std::dynamic_pointer_cast<obj_data>(check).get();
    }
}

void load_items_finish(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "items.json")) {
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<int>();
        if(auto cf = uniqueObjects.find(id); cf != uniqueObjects.end()) {
            if(auto i = cf->second) {
                deserialize_obj_relations(i.get(), j["relations"]);
                i->deserializeLocation(j["location"], j["slot"].get<int>());
            }
        }
    }
}

static json serialize_obj_relations(const obj_data* o) {
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
        j2["generation"] = static_cast<int32_t>(v);
        j2["data"] = *r;
        j2["location"] = r->serializeLocation();
        j2["slot"] = r->worn_on;
        j2["relations"] = serialize_obj_relations(r.get());
        j.push_back(j2);
    }
    dump_to_file(loc, "items.json", j);
}


static void dump_item_prototypes(const std::filesystem::path &loc) {
    json j;
    for(auto &[v, o] : obj_proto) {
        j.push_back(o);
    }
    dump_to_file(loc, "itemPrototypes.json", j);
}

// char_data serialize/deserialize...
void to_json(json& j, const trans_data& t) {
    if(t.timeSpentInForm != 0.0) j["timeSpentInForm"] = t.timeSpentInForm;
    j["visible"] = t.visible;
    j["limitBroken"] = t.limitBroken;
    j["unlocked"] = t.unlocked;
    j["grade"] = t.grade;
    if(t.blutz != 0.0) j["blutz"] = t.blutz;
    if(t.description && strlen(t.description)) j["description"] = t.description;
}

void from_json(const json& j, trans_data& t) {
    if(j.contains("timeSpentInForm")) t.timeSpentInForm = j["timeSpentInForm"];
    if(j.contains("visible")) t.visible = j["visible"];
    if(j.contains("limitBroken")) t.limitBroken = j["limitBroken"];
    if(j.contains("unlocked")) t.unlocked = j["unlocked"];
    if(j.contains("grade")) t.grade = j["grade"];
    if(j.contains("blutz")) t.blutz = j["blutz"];
    if(j.contains("description")) {
        if(t.description) free(t.description);
        t.description = strdup(j["description"].get<std::string>().c_str());
    }
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

void to_json(json& j, const char_data& c) {
    to_json(j, static_cast<const unit_data&>(c));

    if(!c.trains.empty()) j["trains"] = c.trains;
    if(!c.attributes.empty()) j["attributes"] = c.attributes;
    if(!c.moneys.empty()) j["moneys"] = c.moneys;
    if(!c.aligns.empty()) j["aligns"] = c.aligns;
    if(!c.appearances.empty()) j["appearances"] = c.appearances;
    if(!c.vitals.empty()) j["vitals"] = c.vitals;
    if(!c.nums.empty()) j["nums"] = c.nums;
    if(!c.stats.empty()) j["stats"] = c.stats;
    if(!c.dims.empty()) j["dims"] = c.dims;

    for(auto i = 0; i < c.mobFlags.size(); i++)
        if(c.mobFlags.test(i)) {
            j["mobFlags"].push_back(i);
            auto key = std::string(action_bits[i]);
            boost::algorithm::to_lower(key);
            j["mobFlags_name"].push_back(key);
        }

    for(auto i = 0; i < c.playerFlags.size(); i++)
        if(c.playerFlags.test(i)) {
            j["playerFlags"].push_back(i);
            auto key = std::string(player_bits[i]);
            boost::algorithm::to_lower(key);
            j["playerFlags_name"].push_back(key);
        }

    for(auto i = 0; i < c.pref.size(); i++)
        if(c.pref.test(i)) {
            j["pref"].push_back(i);
            auto key = std::string(preference_bits[i]);
            boost::algorithm::to_lower(key);
            j["pref_name"].push_back(key);
        }

    for(auto i = 0; i < c.bodyparts.size(); i++)
        if(c.bodyparts.test(i)) {
            j["bodyparts"].push_back(i);
            auto key = std::string(equipment_types_simple[i]);
            boost::algorithm::to_lower(key);
            j["bodyparts_name"].push_back(key);
        }

    if(c.title && strlen(c.title)) j["title"] = c.title;
    j["race"] = c.race;
    j["chclass"] = c.chclass;

    for(auto i = 0; i < c.affected_by.size(); i++)
        if(c.affected_by.test(i)) {
            j["affected_by"].push_back(i);
            auto key = std::string(affected_bits[i]);
            boost::algorithm::to_lower(key);
            j["affected_by_name"].push_back(key);
        }


    if(c.armor) j["armor"] = c.armor;
    if(c.damage_mod) j["damage_mod"] = c.damage_mod;

    if(c.id != NOTHING) {
        // this is an instance...
        for(auto i = 0; i < NUM_ADMFLAGS; i++)
        if(c.admflags.test(i)) {
            j["admflags"].push_back(i);
            auto key = std::string(admin_flag_names[i]);
            boost::algorithm::to_lower(key);
            j["admflags_name"].push_back(key);
        }


        if(c.was_in_room != NOWHERE) j["was_in_room"] = c.was_in_room;
        json td;
        to_json(td, c.time);
        if(!td.empty()) j["time"] = td;

        for(auto i = 0; i < 4; i++) {
            if(c.limb_condition[i]) {
                j["limb_condition"].push_back(std::make_pair(i, c.limb_condition[i]));
                auto key = std::string(limb_names[i]);
                boost::algorithm::to_lower(key);
                j["limb_condition_name"][key] = c.limb_condition[i];
            }
        }

        if(!c.damages.empty()) j["damages"] = c.damages;

        for(auto i = 0; i < NUM_CONDITIONS; i++) {
            if(c.conditions[i]) j["conditions"].push_back(std::make_pair(i, c.conditions[i]));
        }

        for(auto i = 0; i < c.gravAcclim.size() ; i++) {
            if(c.gravAcclim[i]) j["gravAcclim"].push_back(std::make_pair(i, c.gravAcclim[i]));
        }

        if(c.internalGrowth) j["internalGrowth"] = c.internalGrowth;
        if(c.lifetimeGrowth) j["lifetimeGrowth"] = c.lifetimeGrowth;
        if(c.freeze_level) j["freeze_level"] = c.freeze_level;
        if(c.invis_level) j["invis_level"] = c.invis_level;
        if(c.wimp_level) j["wimp_level"] = c.wimp_level;
        if(get_room(c.load_room)) j["load_room"] = c.load_room;
        if(get_room(c.hometown)) j["hometown"] = c.hometown;

        if(!c.skill.empty()) j["skill"] = c.skill;

        if(c.speaking) j["speaking"] = c.speaking;
        if(c.preference) j["preference"] = c.preference;

        if(c.practice_points) j["practice_points"] = c.practice_points;

        for(auto a = c.affected; a; a = a->next) {
            if(a->type) {
                j["affected"].push_back(*a);
            }
        }

        for(auto a = c.affectedv; a; a = a->next) {
            if(a->type) j["affectedv"].push_back(*a);
        }

        if(c.absorbs) j["absorbs"] = c.absorbs;
        if(c.blesslvl) j["blesslvl"] = c.blesslvl;
        for(auto i = 0; i < 5; i++) {
            if(c.lboard[i]) j["lboard"].push_back(std::make_pair(i, c.lboard[i]));
        }

        for(auto i = 0; i < MAX_BONUSES; i++) {
            if(c.bonuses[i]) j["bonuses"].push_back(i);
        }

        if(c.boosts) j["boosts"] = c.boosts;

        if(c.clan && strlen(c.clan)) j["clan"] = c.clan;
        if(c.crank) j["crank"] = c.crank;
        if(c.con_cooldown) j["con_cooldown"] = c.con_cooldown;
        if(c.deathtime) j["deathtime"] = c.deathtime;
        if(c.dcount) j["dcount"] = c.dcount;
        if(c.death_type) j["death_type"] = c.death_type;
        if(c.damage_mod) j["damage_mod"] = c.damage_mod;
        if(c.droom) j["droom"] = c.droom;
        if(c.accuracy_mod) j["accuracy_mod"] = c.accuracy_mod;
        for(auto i : c.genome) j["genome"].push_back(i);
        if(c.gauntlet) j["gauntlet"] = c.gauntlet;
        if(c.ingestLearned) j["ingestLearned"] = c.ingestLearned;
        if(c.kaioken) j["kaioken"] = c.kaioken;
        if(c.lifeperc) j["lifeperc"] = c.lifeperc;
        if(c.lastint) j["lastint"] = c.lastint;
        if(c.lastpl) j["lastpl"] = c.lastpl;
        if(c.moltexp) j["moltexp"] = c.moltexp;
        if(c.moltlevel) j["moltlevel"] = c.moltlevel;
        if(c.majinize) j["majinize"] = c.majinize;
        if(c.majinizer) j["majinizer"] = c.majinizer;
        if(c.mimic) j["mimic"] = c.mimic.value();
        j["form"] = c.form;
        if(c.olc_zone) j["olc_zone"] = c.olc_zone;
        if(c.starphase) j["starphase"] = c.starphase;
        if(c.accuracy) j["accuracy"] = c.accuracy;
        if(c.position) j["position"] = c.position;

        if(c.rdisplay) j["rdisplay"] = c.rdisplay;
        if(c.relax_count) j["relax_count"] = c.relax_count;
        if(c.radar1) j["radar1"] = c.radar1;
        if(c.radar2) j["radar2"] = c.radar2;
        if(c.radar3) j["radar3"] = c.radar3;
        if(c.feature) j["feature"] = c.feature;
        if(c.ship) j["ship"] = c.ship;
        if(c.con_sdcooldown) j["con_sdcooldown"] = c.con_sdcooldown;
        if(c.shipr) j["shipr"] = c.shipr;
        if(c.skill_slots) j["skill_slots"] = c.skill_slots;
        if(c.stupidkiss) j["stupidkiss"] = c.stupidkiss;
        if(c.suppression) j["suppression"] = c.suppression;
        if(c.tail_growth) j["tail_growth"] = c.tail_growth;

        for(auto i = 0; i < 3; i++) {
            if(c.saving_throw[i]) j["saving_throw"].push_back(std::make_pair(i, c.saving_throw[i]));
        }
        for(auto i = 0; i < 3; i++) {
            if(c.apply_saving_throw[i]) j["apply_saving_throw"].push_back(std::make_pair(i, c.apply_saving_throw[i]));
        }
        if(c.upgrade) j["upgrade"] = c.upgrade;
        if(c.voice && strlen(c.voice)) j["voice"] = c.voice;

        if(c.global_vars) {
            j["dgvariables"] = serializeVars(c.global_vars);
        }

        if(c.relax_count) j["relax_count"] = c.relax_count;
        if(c.ingestLearned) j["ingestLearned"] = c.ingestLearned;

        if (c.poofin && strlen(c.poofin)) j["poofin"] = c.poofin;
        if (c.poofout && strlen(c.poofout)) j["poofout"] = c.poofout;
        if(players.contains(c.last_tell)) j["last_tell"] = c.last_tell;

        j["transBonus"] = c.transBonus;
        if(!c.transforms.empty()) j["transforms"] = c.transforms;

        if(!c.permForms.empty()) j["permForms"] = c.permForms;

    } else {
        // this is a prototype...
        json ms;
        to_json(ms, c.mob_specials);
        if(!ms.empty()) j["mob_specials"] = ms;

        if(!c.proto_script.empty()) j["proto_script"] = c.proto_script;
    }

}

void from_json(const json& j, char_data& c) {
    from_json(j, static_cast<unit_data&>(c));

    if(j.contains("trains")) c.trains = j["trains"];

    if(j.contains("attributes")) c.attributes = j["attributes"];

    if(j.contains("moneys")) c.moneys = j["moneys"];

    if(j.contains("aligns")) c.aligns = j["aligns"];

    if(j.contains("appearances")) c.appearances = j["appearances"];

    if(j.contains("vitals")) c.vitals = j["vitals"];

    if(j.contains("nums")) c.nums = j["nums"];

    if(j.contains("dims")) c.dims = j["dims"];

    if(j.contains("stats")) c.stats = j["stats"];

    if(j.contains("title")) c.title = strdup(j["title"].get<std::string>().c_str());
    if(j.contains("race")) c.race = j["race"];

    if(j.contains("chclass")) c.chclass = j["chclass"];

    if(j.contains("affected_by"))
        for(auto &i : j["affected_by"])
        c.affected_by.set(i.get<int>());

    if(j.contains("armor")) c.armor = j["armor"];
    if(j.contains("damage_mod")) c.damage_mod = j["damage_mod"];
    if(j.contains("mob_specials")) j["mob_specials"].get_to(c.mob_specials);
    if(j.contains("mobFlags")) {
        for(auto &i : j["mobFlags"])
        c.mobFlags.set(i.get<int>());
    }
    if(j.contains("playerFlags")) for(auto &i : j["playerFlags"]) c.playerFlags.set(i.get<int>());
    if(j.contains("pref")) for(auto &i : j["pref"]) c.pref.set(i.get<int>());
    if(j.contains("bodyparts")) for(auto &i : j["bodyparts"]) c.bodyparts.set(i.get<int>());

    if(c.id != NOTHING) {
        // this is an instance.
        if(j.contains("admflags"))
            for(auto &i : j["admflags"])
                c.admflags.set(i.get<int>());

        if(j.contains("hometown")) c.hometown = j["hometown"];

        if(j.contains("time")) {
            j["time"].get_to(c.time);
        }

        if(j.contains("limb_condition")) {
            for(auto &i : j["limb_condition"]) {
                c.limb_condition[i[0].get<int>()] = i[1];
            }
        }

        if(j.contains("damages")) c.damages = j["damages"];

        if(j.contains("was_in_room")) c.was_in_room = j["was_in_room"];

        if(j.contains("skill")) c.skill = j["skill"].get<std::map<SkillID, skill_data>>();

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

        if(j.contains("absorbs")) c.absorbs = j["absorbs"];
        if(j.contains("blesslvl")) c.blesslvl = j["blesslvl"];
        if(j.contains("lboard")) {
            for(auto &i : j["lboard"]) {
                c.lboard[i[0].get<int>()] = i[1];
            }
        }

        if(j.contains("bonuses")) {
            for(auto &i : j["bonuses"]) {
                c.bonuses[i.get<int>()] = true;
            }
        }

        if(j.contains("boosts")) c.boosts = j["boosts"];

        if(j.contains("clan")) c.clan = strdup(j["clan"].get<std::string>().c_str());
        if(j.contains("crank")) c.crank = j["crank"];
        if(j.contains("con_cooldown")) c.con_cooldown = j["con_cooldown"];
        if(j.contains("deathtime")) c.deathtime = j["deathtime"];
        if(j.contains("dcount")) c.dcount = j["dcount"];
        if(j.contains("death_type")) c.death_type = j["death_type"];

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

        if(j.contains("internalGrowth")) c.internalGrowth = j["internalGrowth"];
        if(j.contains("lifetimeGrowth")) c.lifetimeGrowth = j["lifetimeGrowth"];
        if(j.contains("damage_mod")) c.damage_mod = j["damage_mod"];
        if(j.contains("droom")) c.droom = j["droom"];
        if(j.contains("accuracy_mod")) c.accuracy_mod = j["accuracy_mod"];
        if(j.contains("genome")) {
            for(auto &i : j["genome"]) {
                c.genome.insert(i.get<int>());
            }
        }
        if(j.contains("gauntlet")) c.gauntlet = j["gauntlet"];
        if(j.contains("ingestLearned")) c.ingestLearned = j["ingestLearned"];
        if(j.contains("kaioken")) c.kaioken = j["kaioken"];
        if(j.contains("lifeperc")) c.lifeperc = j["lifeperc"];
        if(j.contains("lastint")) c.lastint = j["lastint"];
        if(j.contains("lastpl")) c.lastpl = j["lastpl"];
        if(j.contains("moltexp")) c.moltexp = j["moltexp"];
        if(j.contains("moltlevel")) c.moltlevel = j["moltlevel"];
        if(j.contains("majinize")) c.majinize = j["majinize"];
        if(j.contains("majinizer")) c.majinizer = j["majinizer"];
        if(j.contains("mimic")) c.mimic = j["mimic"].get<RaceID>();
        if(j.contains("olc_zone")) c.olc_zone = j["olc_zone"];
        if(j.contains("starphase")) c.starphase = j["starphase"];
        if(j.contains("accuracy")) c.accuracy = j["accuracy"];
        if(j.contains("position")) c.position = j["position"];

        if(j.contains("rdisplay")) c.rdisplay = strdup(j["rdisplay"].get<std::string>().c_str());
        if(j.contains("relax_count")) c.relax_count = j["relax_count"];
        if(j.contains("radar1")) c.radar1 = j["radar1"];
        if(j.contains("radar2")) c.radar2 = j["radar2"];
        if(j.contains("radar3")) c.radar3 = j["radar3"];
        if(j.contains("feature")) c.feature = strdup(j["feature"].get<std::string>().c_str());
        if(j.contains("ship")) c.ship = j["ship"];
        if(j.contains("con_sdcooldown")) c.con_sdcooldown = j["con_sdcooldown"];
        if(j.contains("shipr")) c.shipr = j["shipr"];
        if(j.contains("skill_slots")) c.skill_slots = j["skill_slots"];
        if(j.contains("stupidkiss")) c.stupidkiss = j["stupidkiss"];
        if(j.contains("suppression")) c.suppression = j["suppression"];
        if(j.contains("tail_growth")) c.tail_growth = j["tail_growth"];

        if(j.contains("saving_throw")) {
            for(auto t : j["saving_throw"]) {
                c.saving_throw[t[0].get<int>()] = t[1];
            }
        }
        if(j.contains("apply_saving_throw")) {
            for(auto t : j["apply_saving_throw"]) {
                c.apply_saving_throw[t[0].get<int>()] = t[1];
            }
        }
        if(j.contains("upgrade")) c.upgrade = j["upgrade"];
        if(j.contains("voice")) c.voice = strdup(j["voice"].get<std::string>().c_str());
        if(j.contains("wimp_level")) c.wimp_level = j["wimp_level"];

        if(!c.proto_script.empty()) {
            assign_triggers(&c, OBJ_TRIGGER);
        }

        if(j.contains("dgvariables")) {
            deserializeVars(&c.global_vars, j["dgvariables"]);
        }

        auto proto = mob_proto.find(c.vn);
        if(proto != mob_proto.end()) {
            c.proto_script = proto->second.proto_script;
        }

        if(j.contains("load_room")) c.load_room = j["load_room"];

        if(j.contains("transBonus")) c.transBonus = j["transBonus"];
        if(j.contains("form")) c.form = j["form"];
        if(j.contains("transforms")) c.transforms = j["transforms"];
        if(j.contains("permForms")) c.permForms = j["permForms"].get<std::unordered_set<FormID>>();
        

        if(j.contains("preference")) c.preference = j["preference"];
        if(j.contains("freeze_level")) c.freeze_level = j["freeze_level"];
        if(j.contains("practice_points")) c.practice_points = j["practice_points"];
        if(j.contains("speaking")) c.speaking = j["speaking"];
    } else {
        // this is a prototype.
        if(j.contains("proto_script")) c.proto_script = j["proto_script"].get<std::vector<trig_vnum>>();

        if (!IS_HUMAN(&c))
            c.affected_by.set(AFF_INFRAVISION);

        SPEAKING(&c) = SKILL_LANG_COMMON;
        set_height_and_weight_by_race(&c);

        c.mobFlags.set(MOB_ISNPC);
        c.mobFlags.reset(MOB_NOTDEADYET);

        c.playerFlags.reset(PLR_NOTDEADYET);
    }
}

static json serialize_char_location(char_data* ch) {
    auto j = json::object();

    if(IS_NPC(ch)) {
        j["in_room"] = ch->in_room;
    } else {
        auto room = ch->in_room != NOWHERE ? ch->in_room : ch->was_in_room;
        if(!ch->desc) {
            room = ch->load_room;
        }
        j["load_room"] = ch->normalizeLoadRoom(room);
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
        j2["location"] = serialize_char_location(r.get());
        //j2["relations"] = r->serializeRelations();
        j.push_back(j2);
    }
    dump_to_file(loc, "characters.json", j);

}

static void dump_npc_prototypes(const std::filesystem::path &loc) {
    json j;

    for(auto &[v, n] : mob_proto) {
        j.push_back(n);
    }
    dump_to_file(loc, "npcPrototypes.json", j);
}

void load_characters_initial(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "characters.json")) {
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<int>();
        auto data = j["data"];
        auto c = std::make_shared<char_data>();
        j["data"].get_to(*c);
        if(auto isPlayer = players.find(id); isPlayer != players.end()) {
            isPlayer->second.character = c.get();
        }
        uniqueCharacters.emplace(id, c);
        units.emplace(id, c);
    }
}

static void deserialize_character_location(json& j, char_data* ch) {
    if(j.contains("in_room")) {
        auto vn = j["in_room"].get<room_vnum>();
        char_to_room(ch, vn);
    } else if(j.contains("load_room")) {
        ch->load_room = j["load_room"].get<room_vnum>();
    }
}

void load_characters_finish(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "characters.json")) {
        auto id = j["id"].get<int64_t>();
        auto generation = j["generation"].get<int>();
        if(auto cf = uniqueCharacters.find(id); cf != uniqueCharacters.end()) {
            if(auto c = cf->second) {
                deserialize_character_location(j["location"], c.get());
            }
        }
    }
}

void load_npc_prototypes(const std::filesystem::path& loc) {
    for(auto j : load_from_file(loc, "npcPrototypes.json")) {
        auto id = j["vn"].get<int64_t>();
        auto p = mob_proto.emplace(id, j);
        auto zone = real_zone_by_thing(id);
        p.first->second.zone = zone;
        auto &i = mob_index[id];
        i.vn = id;
        auto &z = zone_table[zone];
        z.mobiles.insert(id);
    }
}

// players data serialize/deserialize...
void to_json(json& j, const player_data& p) {
    j["id"] = p.id;
    j["name"] = p.name;
    if(p.account) j["account"] = p.account->vn;

    for(auto &a : p.aliases) {
        j["aliases"].push_back(a);
    }

    for(auto &i : p.sensePlayer) {
        j["sensePlayer"].push_back(i);
    }

    for(auto &i : p.senseMemory) {
        j["senseMemory"].push_back(i);
    }

    for(auto &i : p.dubNames) {
        j["dubNames"].push_back(i);
    }

    for(auto i = 0; i < NUM_COLOR; i++) {
        if(p.color_choices[i] && strlen(p.color_choices[i])) j["color_choices"].push_back(std::make_pair(i, p.color_choices[i]));
    }
}

void from_json(const json& j, player_data& p) {
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
            p.sensePlayer.insert(i.get<int64_t>());
        }
    }

    if(j.contains("senseMemory")) {
        for(auto &i : j["senseMemory"]) {
            p.senseMemory.insert(i.get<vnum>());
        }
    }

    if(j.contains("dubNames")) {
        for(auto &i : j["dubNames"]) {
            p.dubNames.emplace(i[0].get<int64_t>(), i[1].get<std::string>());
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
    std::filesystem::path dir = "dumps"; // Change to your directory
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

void runSave() {
    basic_mud_log("Beginning dump of state to disk.");
    // Open up a new database file as <cwd>/state/<timestamp>.sqlite3 and dump the state into it.
    auto path = std::filesystem::current_path() / "dumps";
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
              dump_players, dump_dgscripts,
                          dump_items, dump_globaldata,
                          dump_rooms, dump_exits, dump_item_prototypes,
                          dump_npc_prototypes, dump_shops,
                          dump_guilds, dump_zones,
                          dump_dgscript_prototypes}) {
            threads.emplace_back([func, &tempPath]() {
                func(tempPath);
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