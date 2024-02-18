#include "dbat/saveload.h"
#include "dbat/config.h"
#include "dbat/net.h"
#include "dbat/utils.h"
#include "dbat/players.h"
#include "dbat/db.h"
#include "dbat/account.h"
#include "dbat/dg_scripts.h"
#include "dbat/guild.h"
#include "dbat/shop.h"

#include <fstream>
#include <thread>

static void dump_to_file(const std::filesystem::path &loc, const std::string &name, const nlohmann::json &data) {
    if(data.empty()) return;
    //auto startTime = std::chrono::high_resolution_clock::now();
    std::ofstream out(loc / name);
    out << jdump(data);
    out.close();
    //auto endTime = std::chrono::high_resolution_clock::now();
    //auto duration = std::chrono::duration<double>(endTime - startTime).count();
    //basic_mud_log("Dumping %s to disk took %f seconds.", name, duration);
}

static void dump_state_accounts(const std::filesystem::path &loc) {
    nlohmann::json j;

    for(auto &[v, r] : accounts) {
        j.push_back(r->serialize());
    }

    dump_to_file(loc, "accounts.json", j);

}

static void dump_state_players(const std::filesystem::path &loc) {
    nlohmann::json j;

    for(auto &[v, r] : players) {
        j.push_back(r->serialize());
    }
    dump_to_file(loc, "players.json", j);
}


void dump_state_globalData(const std::filesystem::path &loc) {
    nlohmann::json j;

    j["time"] = time_info.serialize();
    j["weather"] = weather_info.serialize();
    j["nextUID"] = nextUID;

    dump_to_file(loc, "globaldata.json", j);
}

static void process_dirty_instances(const std::filesystem::path &loc) {
    nlohmann::json instances;

    for(auto &[v, u] : world) {
        nlohmann::json j;
        j["uid"] = v;
        j["unitClass"] = u->getUnitClass();
        j["data"] = u->serialize();
        instances.push_back(j);
    }
    dump_to_file(loc, "instances.json", instances);
}

static void process_dirty_relations(const std::filesystem::path &loc) {
    nlohmann::json relations;

    for(auto &[v, u] : world) {
        relations.push_back(std::make_pair(v, u->serializeRelations()));
    }
    dump_to_file(loc, "relations.json", relations);
}


static void process_dirty_item_prototypes(const std::filesystem::path &loc) {
    nlohmann::json j;
    for(auto &[v, o] : obj_proto) {
        j.push_back(o);
    }
    dump_to_file(loc, "itemPrototypes.json", j);
}

static void process_dirty_npc_prototypes(const std::filesystem::path &loc) {
    nlohmann::json j;

    for(auto &[v, n] : mob_proto) {
        j.push_back(n);
    }
    dump_to_file(loc, "npcPrototypes.json", j);
}

static void process_dirty_shops(const std::filesystem::path &loc) {
    nlohmann::json j;
    for(auto &[v, s] : shop_index) {
        j.push_back(s.serialize());
    }
    dump_to_file(loc, "shops.json", j);
}

static void process_dirty_guilds(const std::filesystem::path &loc) {
    nlohmann::json j;
    for(auto &[v, g] : guild_index) {
        j.push_back(g.serialize());
    }
    dump_to_file(loc, "guilds.json", j);
}

static void process_dirty_zones(const std::filesystem::path &loc) {
    nlohmann::json j;

    for(auto &[v, z] : zone_table) {
        j.push_back(z.serialize());
    }
    dump_to_file(loc, "zones.json", j);
}

static void process_dirty_dgscript_prototypes(const std::filesystem::path &loc) {
    nlohmann::json j;
    for(auto &[v, t] : trig_index) {
        j.push_back(t->serialize());
    }
    dump_to_file(loc, "dgScriptPrototypes.json", j);
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

    // If we have more than 20 state files, we want to purge the oldest one(s) until we have just 20.
    while(files.size() > 20) {
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
        for(const auto func : {dump_state_accounts,
              dump_state_players,
                           dump_state_globalData,
                          process_dirty_instances, process_dirty_relations, process_dirty_item_prototypes,
                          process_dirty_npc_prototypes, process_dirty_shops,
                          process_dirty_guilds, process_dirty_zones,
                          process_dirty_dgscript_prototypes}) {
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