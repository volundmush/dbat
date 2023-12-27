#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/config.h"

static std::set<trig_vnum> vnums;


static void countScripts(const nlohmann::json& j) {
    // each j may have a "proto_script" key which contains a list of ids. if present, add those vnums to vnums.
    if(j.contains("proto_script")) {
        for(auto& id : j["proto_script"]) {
            vnums.insert(id.get<trig_vnum>());
        }
    }
}

int main(int argc, char **argv) {
    setup_log();

    try {
        stateDb = std::make_shared<SQLite::Database>("assets.sqlite3", SQLite::OPEN_READONLY);
    } catch(std::exception& e) {
        logger->critical("Error opening state database: {}", e.what());
        return 1;
    }

    for(auto table : {"npcPrototypes", "itemPrototypes", "rooms"}) {
        // All of these tables have the same structure: (id, data) where data is a json text.
        // For each table, we want to grab its data row and feed it to countScripts as a nlohmann::json object.

        SQLite::Statement q(*stateDb, fmt::format("SELECT id,data FROM {}", table));
        while(q.executeStep()) {
            auto id = q.getColumn(0).getInt64();
            auto data = q.getColumn(1).getString();
            auto j = nlohmann::json::parse(data);
            countScripts(j);
        }
    }

    logger->info("There are {} scripts used by the state database.", vnums.size());

}