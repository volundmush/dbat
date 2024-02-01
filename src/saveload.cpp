#include "dbat/saveload.h"
#include "dbat/config.h"

#include "dbat/net.h"
#include "dbat/utils.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "dbat/players.h"
#include "dbat/db.h"
#include "dbat/account.h"
#include "dbat/dg_scripts.h"
#include "dbat/guild.h"
#include "dbat/shop.h"

#include <fstream>

static void dump_to_file(const std::filesystem::path &loc, const std::string &name, const nlohmann::json &data) {
    std::ofstream out(loc / name);
    out << jdump_pretty(data);
    out.close();
}

static void dump_state_accounts(const std::filesystem::path &loc) {
    nlohmann::json j;

    for(auto &[v, r] : accounts) {
        j.push_back(std::make_pair(v, r.serialize()));
    }

    dump_to_file(loc, "accounts.json", j);

}

static void dump_state_players(const std::filesystem::path &loc) {
    nlohmann::json j;

    for(auto &[v, r] : players) {
        j.push_back(std::make_pair(v, r.serialize()));
    }
    dump_to_file(loc, "players.json", j);
}

static void dump_state_characters(const std::filesystem::path &loc) {
    nlohmann::json j;

    for(auto &[v, r] : uniqueCharacters) {
        if(v != r.second->id) r.second->id = v;
        nlohmann::json j2;
        j2["id"] = v;
        j2["generation"] = static_cast<int32_t>(r.first);
        j2["vnum"] = r.second->vn;
        j2["name"] = r.second->name;
        j2["shortDesc"] = r.second->short_description;
        j2["data"] = r.second->serializeInstance();
        j2["location"] = r.second->serializeLocation();
        j2["relations"] = r.second->serializeRelations();
        j.push_back(std::make_pair(v, j2));
    }
    dump_to_file(loc, "characters.json", j);

}

static void dump_state_items(const std::filesystem::path &loc) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO items (id, generation, vnum, name, shortDesc, data, location, slot, relations) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

    for(auto &[v, r] : uniqueObjects) {
        if(v != r.second->id) r.second->id = v;
        q.bind(1, v);
        q.bind(2, static_cast<int32_t>(r.first));
        q.bind(3, r.second->vn);
        q.bind(4, r.second->name);
        q.bind(5, r.second->short_description);
        q.bind(6, jdump(r.second->serializeInstance()));
        q.bind(7, r.second->serializeLocation());
        q.bind(8, r.second->worn_on);
        q.bind(9, jdump(r.second->serializeRelations()));
        q.exec();
        q.reset();
    }
}

static void dump_state_dgscripts(const std::filesystem::path &loc) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO dgscripts (id, generation, vnum, name, data, location, num) VALUES (?, ?, ?, ?, ?, ?, ?)");

    for(auto &[v, r] : uniqueScripts) {
        if(v != r.second->id) r.second->id = v;
        q.bind(1, v);
        q.bind(2, static_cast<int32_t>(r.first));
        q.bind(3, r.second->vn);
        q.bind(4, r.second->name);
        q.bind(5, jdump(r.second->serializeInstance()));
        q.bind(6, r.second->serializeLocation());
        q.bind(7, r.second->order);
        q.exec();
        q.reset();
    }
}

void dump_state_globalData(const std::filesystem::path &loc) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO globalData (name, data) VALUES (?, ?)");

    std::map<std::string, nlohmann::json> globalData;

    globalData["time"] = time_info.serialize();
    globalData["weather"] = weather_info.serialize();
    auto gRoom = world.find(0);
    if(gRoom != world.end()) {
        if(gRoom->second.script && gRoom->second.script->global_vars) {
            globalData["dgGlobals"] = serializeVars(gRoom->second.script->global_vars);
        }
    }

    for(auto &[v, r] : globalData) {
        q.bind(1, v);
        q.bind(2, jdump(r));
        q.exec();
        q.reset();
    }
}

static void process_dirty_rooms(const std::filesystem::path &loc) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO rooms (id, data) VALUES (?, ?)");
    SQLite::Statement q2(*db, "INSERT OR REPLACE INTO exits (id, direction, destination, data) VALUES (?,?,?,?)");

    for(auto &[v, r] : world) {
        q.bind(1, v);
        q.bind(2, jdump(r.serialize()));
        q.exec();
        q.reset();

        for(auto i = 0; i < NUM_OF_DIRS; i++) {
            if(auto ex = r.dir_option[i]; ex) {
                q2.bind(1, v);
                q2.bind(2, i);
                q2.bind(3, ex->to_room);
                q2.bind(4, jdump(ex->serialize()));
                q2.exec();
                q2.reset();
            }
        }

    }
}

static void process_dirty_item_prototypes(const std::filesystem::path &loc) {
	SQLite::Statement q(*db, "INSERT OR REPLACE INTO itemPrototypes (id, data) VALUES (?,?)");

    for(auto &[v, o] : obj_proto) {
        q.bind(1, v);
        q.bind(2, jdump(o.serializeProto()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_npc_prototypes(const std::filesystem::path &loc) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO npcPrototypes (id, data) VALUES (?, ?)");

    for(auto &[v, n] : mob_proto) {
        q.bind(1, v);
        q.bind(2, jdump(n.serializeProto()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_shops(const std::filesystem::path &loc) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO shops (id, data) VALUES (?,?)");

    for(auto &[v, s] : shop_index) {
        q.bind(1, v);
        q.bind(2, jdump(s.serialize()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_guilds(const std::filesystem::path &loc) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO guilds (id, data) VALUES (?, ?)");

    for(auto &[v, g] : guild_index) {
        q.bind(1, v);
        q.bind(2, jdump(g.serialize()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_zones(const std::filesystem::path &loc) {
	SQLite::Statement q(*db, "INSERT OR REPLACE INTO zones (id, data) VALUES (?, ?)");

    for(auto &[v, z] : zone_table) {
        q.bind(1, v);
        q.bind(2, jdump(z.serialize()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_areas(const std::filesystem::path &loc) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO areas (id, data) VALUES (?, ?)");

    for(auto &[v, a] : areas) {
        q.bind(1, v);
        q.bind(2, jdump(a.serialize()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_dgscript_prototypes(const std::filesystem::path &loc) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO dgScriptPrototypes (id, data) VALUES (?, ?)");

    for(auto &[v, t] : trig_index) {
        q.bind(1, v);
        q.bind(2, jdump(t.serializeProto()));
        q.exec();
        q.reset();
    }
}

static std::vector<std::filesystem::path> getDumpFiles() {
    std::filesystem::path dir = "dumps"; // Change to your directory
    std::vector<std::filesystem::path> files;

    auto pattern = fmt::format("{}-", config::stateDbName);
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().filename().string().starts_with(pattern)) {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end(), std::greater<>());
    return files;
}

static void cleanup_state() {
    auto vecFiles = getDumpFiles();
    std::list<std::filesystem::path> files(vecFiles.begin(), vecFiles.end());

    // If we have more than 20 state files, we want to purge the oldest one(s) until we have just 20.
    while(files.size() > 20) {
        std::filesystem::remove(files.back());
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
    std::filesystem::remove(tempPath);

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
        dump_state_accounts(tempPath);
        dump_state_characters(tempPath);
        dump_state_players(tempPath);
        dump_state_dgscripts(tempPath);
        dump_state_items(tempPath);
        dump_state_globalData(tempPath);
        process_dirty_rooms(tempPath);
        process_dirty_item_prototypes(tempPath);
        process_dirty_npc_prototypes(tempPath);
        process_dirty_shops(tempPath);
        process_dirty_guilds(tempPath);
        process_dirty_zones(tempPath);
        process_dirty_areas(tempPath);
        process_dirty_dgscript_prototypes(tempPath);

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
    return;
}