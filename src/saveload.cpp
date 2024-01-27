#include "dbat/saveload.h"
#include "dbat/config.h"

#include "dbat/net.h"
#include "dbat/utils.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "asio/experimental/awaitable_operators.hpp"
#include "asio/experimental/concurrent_channel.hpp"
#include "dbat/players.h"
#include "dbat/db.h"
#include "dbat/account.h"
#include "dbat/dg_scripts.h"
#include "dbat/guild.h"
#include "dbat/shop.h"

static std::vector<std::string> schema = {
        "CREATE TABLE IF NOT EXISTS zones ("
        "	id INTEGER PRIMARY KEY,"
        "	data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS areas ("
        "	id INTEGER PRIMARY KEY,"
        "	data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS itemPrototypes ("
        "	id INTEGER PRIMARY KEY,"
        "	data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS npcPrototypes ("
        "	id INTEGER PRIMARY KEY,"
        "	data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS shops ("
        "	id INTEGER PRIMARY KEY,"
        "	data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS guilds ("
        "	id INTEGER PRIMARY KEY,"
        "	data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS rooms ("
        "	id INTEGER PRIMARY KEY,"
        "	data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS exits ("
        "	id INTEGER NOT NULL,"
        "   direction INTEGER NOT NULL,"
        "   destination INTEGER NOT NULL,"
        "	data TEXT NOT NULL,"
        "   PRIMARY KEY(id, direction)"
        ");",

        "CREATE TABLE IF NOT EXISTS dgScriptPrototypes ("
        "	id INTEGER PRIMARY KEY,"
        "	data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS accounts ("
        "   id INTEGER PRIMARY KEY,"
        "	data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS playerCharacters ("
        "   id INTEGER NOT NULL PRIMARY KEY,"
        "   data TEXT NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS characters ("
        "   id INTEGER PRIMARY KEY,"
        "   generation INTEGER NOT NULL,"
        "   vnum INTEGER NOT NULL DEFAULT -1,"
        "   name TEXT,"
        "   shortDesc TEXT,"
        "   data TEXT NOT NULL,"
        "   location TEXT NOT NULL DEFAULT '{}',"
        "   relations TEXT NOT NULL DEFAULT '{}'"
        ");",

        "CREATE TABLE IF NOT EXISTS items ("
        "   id INTEGER PRIMARY KEY,"
        "   generation INTEGER NOT NULL,"
        "   vnum INTEGER NOT NULL DEFAULT -1,"
        "   name TEXT,"
        "   shortDesc TEXT,"
        "   data TEXT NOT NULL,"
        "   location TEXT NOT NULL DEFAULT '',"
        "   slot INTEGER NOT NULL DEFAULT 0,"
        "   relations TEXT NOT NULL DEFAULT '{}'"
        ");",

        "CREATE TABLE IF NOT EXISTS dgScripts ("
        "	id INTEGER PRIMARY KEY,"
        "   generation INTEGER NOT NULL,"
        "   vnum INTEGER NOT NULL DEFAULT -1,"
        "   name TEXT,"
        "	data TEXT NOT NULL,"
        "   location TEXT NOT NULL,"
        "   num INTEGER NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS globalData ("
        "   name TEXT PRIMARY KEY,"
        "   data TEXT NOT NULL"
        ");"
};

static void runQuery(std::string_view query, const std::shared_ptr<SQLite::Database>& db) {
    try {
        db->exec(query.data());
    }
    catch (const std::exception& e) {
        basic_mud_log("Error executing query: %s", e.what());
        basic_mud_log("For statement: %s", query.data());
        exit(1);
    }
}

void create_schema(const std::shared_ptr<SQLite::Database>& db) {
    for (const auto& query: schema) {
        runQuery(query, db);
    }
}

static void dump_state_accounts(const std::shared_ptr<SQLite::Database>& db) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO accounts (id, data) VALUES (?, ?)");

    for(auto &[v, r] : accounts) {
        q.bind(1, v);
        q.bind(2, jdump(r.serialize()));
        q.exec();
        q.reset();
    }
}

static void dump_state_players(const std::shared_ptr<SQLite::Database>& db) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO playerCharacters (id, data) VALUES (?, ?)");

    for(auto &[v, r] : players) {
        q.bind(1, v);
        q.bind(2, jdump(r.serialize()));
        q.exec();
        q.reset();
    }
}

static void dump_state_characters(const std::shared_ptr<SQLite::Database>& db) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO characters (id, generation, vnum, name, shortDesc, data, location, relations) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

    for(auto &[v, r] : uniqueCharacters) {
        if(v != r.second->id) r.second->id = v;
        q.bind(1, v);
        q.bind(2, r.first);
        q.bind(3, r.second->vn);
        q.bind(4, r.second->name);
        q.bind(5, r.second->short_description);
        q.bind(6, jdump(r.second->serializeInstance()));
        q.bind(7, jdump(r.second->serializeLocation()));
        q.bind(8, jdump(r.second->serializeRelations()));
        q.exec();
        q.reset();
    }

}

static void dump_state_items(const std::shared_ptr<SQLite::Database>& db) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO items (id, generation, vnum, name, shortDesc, data, location, slot, relations) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

    for(auto &[v, r] : uniqueObjects) {
        if(v != r.second->id) r.second->id = v;
        q.bind(1, v);
        q.bind(2, r.first);
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

static void dump_state_dgscripts(const std::shared_ptr<SQLite::Database>& db) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO dgscripts (id, generation, vnum, name, data, location, num) VALUES (?, ?, ?, ?, ?, ?, ?)");

    for(auto &[v, r] : uniqueScripts) {
        if(v != r.second->id) r.second->id = v;
        q.bind(1, v);
        q.bind(2, r.first);
        q.bind(3, r.second->vn);
        q.bind(4, r.second->name);
        q.bind(5, jdump(r.second->serializeInstance()));
        q.bind(6, r.second->serializeLocation());
        q.bind(7, r.second->order);
        q.exec();
        q.reset();
    }
}

void dump_state_globalData(const std::shared_ptr<SQLite::Database>& db) {
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

static void process_dirty_rooms(const std::shared_ptr<SQLite::Database>& db) {
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

static void process_dirty_item_prototypes(const std::shared_ptr<SQLite::Database>& db) {
	SQLite::Statement q(*db, "INSERT OR REPLACE INTO itemPrototypes (id, data) VALUES (?,?)");

    for(auto &[v, o] : obj_proto) {
        q.bind(1, v);
        q.bind(2, jdump(o.serializeProto()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_npc_prototypes(const std::shared_ptr<SQLite::Database>& db) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO npcPrototypes (id, data) VALUES (?, ?)");

    for(auto &[v, n] : mob_proto) {
        q.bind(1, v);
        q.bind(2, jdump(n.serializeProto()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_shops(const std::shared_ptr<SQLite::Database>& db) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO shops (id, data) VALUES (?,?)");

    for(auto &[v, s] : shop_index) {
        q.bind(1, v);
        q.bind(2, jdump(s.serialize()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_guilds(const std::shared_ptr<SQLite::Database>& db) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO guilds (id, data) VALUES (?, ?)");

    for(auto &[v, g] : guild_index) {
        q.bind(1, v);
        q.bind(2, jdump(g.serialize()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_zones(const std::shared_ptr<SQLite::Database>& db) {
	SQLite::Statement q(*db, "INSERT OR REPLACE INTO zones (id, data) VALUES (?, ?)");

    for(auto &[v, z] : zone_table) {
        q.bind(1, v);
        q.bind(2, jdump(z.serialize()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_areas(const std::shared_ptr<SQLite::Database>& db) {
    SQLite::Statement q(*db, "INSERT OR REPLACE INTO areas (id, data) VALUES (?, ?)");

    for(auto &[v, a] : areas) {
        q.bind(1, v);
        q.bind(2, jdump(a.serialize()));
        q.exec();
        q.reset();
    }
}

static void process_dirty_dgscript_prototypes(const std::shared_ptr<SQLite::Database>& db) {
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

asio::awaitable<void> runSave() {
    logger->info("Beginning dump of state to disk.");
    // Open up a new database file as <cwd>/state/<timestamp>.sqlite3 and dump the state into it.
    auto path = std::filesystem::current_path() / "dumps";
    std::filesystem::create_directories(path);
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = *std::localtime(&time_t_now);

    auto tempPath = path / fmt::format("{}.sqlite3", config::stateDbName);
    auto journalPath = path / fmt::format("{}.sqlite3-journal", config::stateDbName);
    std::filesystem::remove(tempPath);
    std::filesystem::remove(journalPath);

    auto newPath = path / fmt::format("{}-{:04}{:02}{:02}{:02}{:02}{:02}.sqlite3",
                                      config::stateDbName,
                                      tm_now.tm_year + 1900,
                                      tm_now.tm_mon + 1,
                                      tm_now.tm_mday,
                                      tm_now.tm_hour,
                                      tm_now.tm_min,
                                      tm_now.tm_sec);

    double duration{};

    try {
        auto db = std::make_shared<SQLite::Database>(tempPath.string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        auto startTime = std::chrono::high_resolution_clock::now();
        SQLite::Transaction trans(*db);
        create_schema(db);
        dump_state_accounts(db);
        dump_state_characters(db);
        dump_state_players(db);
        dump_state_dgscripts(db);
        dump_state_items(db);
        dump_state_globalData(db);
        process_dirty_rooms(db);
        process_dirty_item_prototypes(db);
        process_dirty_npc_prototypes(db);
        process_dirty_shops(db);
        process_dirty_guilds(db);
        process_dirty_zones(db);
        process_dirty_areas(db);
        process_dirty_dgscript_prototypes(db);
        trans.commit();
        auto endTime = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration<double>(endTime - startTime).count();

    } catch (std::exception &e) {
        logger->critical("(GAME HAS NOT BEEN SAVED!) Exception in dump_state(): {}", e.what());
        send_to_all("Warning, a critical error occurred during save! Please alert staff!\r\n");
        co_return;
    }

    std::filesystem::rename(tempPath, newPath);
    logger->info("Finished dumping state to {} in {} seconds.", newPath.string(), duration);
    cleanup_state();
    co_return;
}