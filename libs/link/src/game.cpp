#include <pqxx/pqxx>
#include <memory>

#include "dbat/game/Descriptor.hpp"
#include "dbat/game/db.hpp"
#include "dbat/serde/saveload.hpp"
#include "dbat/game/comm.hpp"

namespace dbat::link {

    std::unique_ptr<pqxx::connection> db_conn;

    std::unordered_map<std::string, std::shared_ptr<GameConnectionInfo>> active_game_connections;
    static std::unordered_set<std::string> pending_game_connections;

    void acceptNewConnections() {
        std::unordered_set<std::string> existing_connections;
        for(auto& [id, info] : active_game_connections) {
            existing_connections.insert(id);
        }

        pqxx::work txn(*db_conn);

        auto rows = txn.exec(
            "SELECT s.*,u.dbat_id AS account_id,c.dbat_id AS character_id"
            "FROM pc_subscriptions AS s LEFT JOIN dbat.users AS u ON s.user_id = u.id"
            "LEFT JOIN dbat.pcs AS c ON s.pc_id = c.id"
            " WHERE id <> ALL($1::uuid[])"
            , existing_connections);

        for(const auto& row : rows) {
            auto conn = std::make_shared<GameConnectionInfo>();
            conn->id = row["id"].as<std::string>();
            conn->pc_id = row["pc_id"].as<std::string>();
            conn->user_id = row["user_id"].as<std::string>();
            conn->account_id = row["account_id"].as<int>();
            conn->character_id = row["character_id"].as<int64_t>();
            conn->created_at = row["created_at"].as<time_t>();
            conn->ip_address = row["ip_address"].as<std::string>();
            active_game_connections.emplace(conn->id, conn);
            pending_game_connections.insert(conn->id);
            create_join_session(conn);
        }

        txn.commit();
    }

    void handleNewOutput() {
        pqxx::work txn(*db_conn);

        auto stream = pqxx::stream_to::table(
            txn,
            {"public", "pc_events"},
            {"pc_id", "event_type", "data"}
        );

        for(auto& [conn_id, desc] : sessions) {
            if(!desc) continue;
            if(desc->processed_output.empty()) continue;

            stream.write_values(desc->pc_id, "text.circle", desc->processed_output);
            desc->processed_output.clear();
        }
        stream.complete();
        txn.commit();
    }

    void handleNewInput() {

        pqxx::work txn(*db_conn);

        auto rows = txn.exec(
            "SELECT e.*,p.dbat_id AS character_id"
            "FROM dbat.incoming_events AS e LEFT JOIN dbat.pcs AS p ON e.pc_id = p.id");

        for(const auto& row : rows) {
            auto character_id = row["character_id"].as<int64_t>();
            auto event_type = row["event_type"].as<std::string>();
            auto data = row["data"].as<std::string>();

            auto it = sessions.find(character_id);
            if(it == sessions.end()) continue;
            auto& desc = it->second;
            if(!desc) continue;
            desc->incoming_events.push_back({event_type, data});
        }

        txn.exec("DELETE FROM dbat.incoming_events");
        txn.commit();
    }

    void handleDisconnections() {
        pqxx::work txn(*db_conn);

        // we need to detect which of our active_game_connections are no longer in pc_subscriptions.
        std::unordered_set<std::string> existing_connections;
        for(auto& [id, info] : active_game_connections) {
            existing_connections.insert(id);
        }

        auto rows = txn.exec(
            "WITH input_ids AS (SELECT unnest($1::uuid[]) AS id)"
            "SELECT i.id FROM input_ids AS i LEFT JOIN pc_subscriptions AS s ON i.id = s.pc_id WHERE s.pc_id IS NULL",
            existing_connections);

        for(const auto& row : rows) {
            auto conn_id = row["id"].as<std::string>();
            if(auto it = active_game_connections.find(conn_id); it != active_game_connections.end()) {
                auto& game_info = it->second;
                if(auto desc_it = sessions.find(game_info->character_id); desc_it != sessions.end()) {
                    auto& desc = desc_it->second;
                    if(desc) {
                        desc->onConnectionLost(game_info->id);
                    }
                }
                active_game_connections.erase(it);
            }
            pending_game_connections.erase(conn_id);
        }

        // if any ids are not in sessions, we can also clean them up.
        std::unordered_set<std::shared_ptr<GameConnectionInfo>> to_erase;
        for(auto &[id, ginfo] : active_game_connections) {
            if (pending_game_connections.contains(id)) {
                continue;
            }
            to_erase.insert(ginfo);
        }

        if(!to_erase.empty()) {
            auto stream = pqxx::stream_to::table(
                txn,
                {"public", "pc_events"},
                {"pc_id", "event_type", "data"}
            );
            for(auto ginfo : to_erase) {
                active_game_connections.erase(ginfo->id);
                stream.write_values(ginfo->pc_id, "circle.quit", "");
            }
            stream.complete();
        }
        txn.commit();
    }

    std::size_t process_requests(std::chrono::steady_clock::duration budget) {
        using clock = std::chrono::steady_clock;
        const auto start = clock::now();
        std::size_t processed = 0;

        pqxx::work txn(*db_conn);

        // Get all pending requests.
        auto rows = txn.exec(
            "SELECT r.*,u.username,u.admin_level"
            "FROM dbat.api_requests AS r LEFT JOIN users AS u ON r.user_id = u.id"
            "WHERE r.response_status = -1"
        );

        for (const auto& row : rows) {

            // TODO: deserialize request data, feed into handler that returns a response...
            auto response = true;

            txn.exec(
                "UPDATE dbat.api_requests"
                "SET response_status = $1, response_data = $2"
                "WHERE id = $3",
                {response ? 200 : 500,
                response ? "OK" : "Internal Server Error",
                row["id"].as<std::string>()}
            );

            ++processed;

            if (clock::now() - start >= budget) {
                break;
            }
        }
        return processed;
    }

    static bool running{false};

    int run_game(double heartbeat_interval, double save_interval) {
        using namespace std::chrono;
        using clock = steady_clock;

        running = true;

        double save_timer = save_interval;

        auto interval = duration_cast<steady_clock::duration>(duration<double>(heartbeat_interval));

        try {
            while(running) {

                // get current timestamp...
                auto start = clock::now();
                dbat::link::acceptNewConnections();
                dbat::link::handleNewInput();
                runOneLoop(heartbeat_interval);
                dbat::link::handleNewOutput();
                dbat::link::handleDisconnections();
                auto end =  clock::now();

                save_timer -= heartbeat_interval;
                if(save_timer <= 0.0) {
                    save_timer = save_interval;
                    saveAll = true;
                }

                if(saveAll) {
                    dbat::save::runSaveSync();
                    saveAll = false;
                }

                auto elapsed = end - start;
                auto remaining = interval - elapsed;

                // This section processes network requests within the remaining time budget.
                // It will process at least one if there is one to do, though.
                start = clock::now();
                auto processed = process_requests(remaining);
                //LDEBUG("Processed {} requests in this tick.", processed);
                end = clock::now();
                elapsed = end - start;
                remaining = remaining - elapsed;

                if (remaining > clock::duration::zero()) {
                    std::this_thread::sleep_for(remaining);
                } else {
                    // avoid hard spinning when behind schedule
                    std::this_thread::yield();
                }
            }
        } catch(const std::exception &e) {
            LERROR("Exception in run_game: {}", e.what());
        }

        LINFO("Game loop has exited.");
        return 0;
    }
}
