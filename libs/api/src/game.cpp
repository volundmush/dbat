#include "dbat/api/channel.hpp"

#include "dbat/game/Descriptor.hpp"
#include "dbat/game/db.hpp"
#include "dbat/serde/saveload.hpp"
#include "dbat/game/comm.hpp"

namespace dbat::api {

    std::unordered_map<int64_t, std::shared_ptr<GameConnectionInfo>> active_game_connections;
    static std::unordered_set<int64_t> pending_game_connections;

    boost::asio::awaitable<void> acceptNewConnections() {
        // let's accept all new connections. we'll grab as many as we can from game_connection_channel()
        // a mutex isn't necessary because only one strand can be running this at a time.
        static int64_t next_connection_id = 1;

        auto &chan = game_connection_channel();
        while(chan.ready()) {
            auto game_info = co_await chan.async_receive(boost::asio::use_awaitable);
            game_info->connection_id = next_connection_id++;
            active_game_connections.emplace(game_info->connection_id, game_info);
            pending_game_connections.insert(game_info->connection_id);
            create_join_session(game_info->account_id, game_info->character_id, game_info->connection_id, game_info->address);
        }
    }

    boost::asio::awaitable<void> handleNewOutput() {
        for(auto& [conn_id, desc] : sessions) {
            if(!desc) continue;
            if(desc->processed_output.empty()) continue;

            GameMessageText gmsg{desc->processed_output};
            boost::system::error_code ec;

            for(const auto& [id, addr] : desc->conns) {
                if(id == conn_id) {
                    continue;
                }
                auto it = active_game_connections.find(id);
                if(it != active_game_connections.end()) {
                    auto& game_info = it->second;
                    co_await game_info->to_websocket.async_send(ec, gmsg, boost::asio::use_awaitable);
                    if(ec) {
                        LINFO("Error sending processed output to connection {}: {}", id, ec.message());
                    }
                }
            }

            desc->processed_output.clear();
        }
    }

    boost::asio::awaitable<void> handleNewInput() {
        for(auto& [id, ginfo] : active_game_connections) {
            auto it = sessions.find(ginfo->character_id);
            if(it == sessions.end()) continue;
            auto& desc = it->second;
            if(!desc) continue;

            boost::system::error_code ec;
            // retrieve a message from to_game if there is one.
            while(ginfo->to_game.ready()) {
                auto gmsg = co_await ginfo->to_game.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                if(ec) {
                    LINFO("Error receiving message from connection {}: {}", id, ec.message());
                    break;
                }
                if(std::holds_alternative<GameMessageText>(gmsg)) {
                    auto& msg = std::get<GameMessageText>(gmsg);
                    desc->raw_input_queue.push_back(msg.text);
                } else if(std::holds_alternative<GameMessageDisconnect>(gmsg)) {
                    auto& msg = std::get<GameMessageDisconnect>(gmsg);
                    desc->onConnectionLost(id);
                    LINFO("Connection {} disconnected by game server: {}", id, msg.reason);
                } else if(std::holds_alternative<GameMessageGMCP>(gmsg)) {
                    // don't do anything with GMCP just yet...
                }
            }
        }
    }

    boost::asio::awaitable<void> handleDisconnections() {
        // handles disconnections reported from the websocket connections.
        auto &chan = game_disconnection_channel();
        while(chan.ready()) {
            auto [conn_id, reason] = co_await chan.async_receive(boost::asio::use_awaitable);
            auto it = active_game_connections.find(conn_id);
            if(it != active_game_connections.end()) {
                auto& game_info = it->second;
                if(auto desc_it = sessions.find(conn_id); desc_it != sessions.end()) {
                    auto& desc = desc_it->second;
                    if(desc) {
                        desc->onConnectionLost(game_info->connection_id);
                    }
                }
                active_game_connections.erase(it);
            }
        }

        for (auto it = pending_game_connections.begin(); it != pending_game_connections.end(); ) {
            if (sessions.find(*it) != sessions.end()) {
                it = pending_game_connections.erase(it);
            } else {
                ++it;
            }
        }

        // if any ids are not in sessions, we can also clean them up.
        GameMessageDisconnect quit_msg{"quit"};
        ToGameMessage quit_game_msg{quit_msg};
        boost::system::error_code ec;
        std::unordered_set<int64_t> to_erase;
        for(auto &[id, ginfo] : active_game_connections) {
            if (pending_game_connections.contains(id)) {
                continue;
            }
            if(sessions.find(id) == sessions.end()) {
                to_erase.insert(id);
                co_await ginfo->to_websocket.async_send(ec, quit_game_msg, boost::asio::use_awaitable);
                if(ec) {
                    LINFO("Error sending quit message to connection {}: {}", id, ec.message());
                }
            }
        }
        for(auto id : to_erase) {
            active_game_connections.erase(id);
        }
    }

    boost::asio::awaitable<volcano::web::HttpAnswer> handle_request(RequestMessage& msg);

    boost::asio::awaitable<std::size_t> drain_request_channel(std::chrono::steady_clock::duration budget) {
        using clock = std::chrono::steady_clock;
        const auto start = clock::now();
        std::size_t processed = 0;

        auto &chan = request_channel();

        for (;;) {
            RequestMessage msg;

            if(!chan.ready()) {
                co_return processed;
            }

            try {
                msg = co_await chan.async_receive(boost::asio::use_awaitable);
            } catch (const boost::system::system_error& e) {
                LERROR("Request channel receive error: {}", e.code().message().c_str());
                co_return processed;
            }

            auto response = co_await handle_request(msg);

            boost::system::error_code send_ec;
            co_await msg.response_channel->async_send(
                send_ec,
                std::move(response),
                boost::asio::use_awaitable
            );

            if (send_ec) {
                LERROR("Request response send error: {}", send_ec.message().c_str());
            }

            ++processed;

            if (clock::now() - start >= budget) {
                co_return processed;
            }
        }
    }

    static bool running{false};

    boost::asio::awaitable<void> run_game(double heartbeat_interval, double save_interval) {
        using namespace std::chrono;
        using clock = steady_clock;

        running = true;

        double save_timer = save_interval;

        auto exec = co_await boost::asio::this_coro::executor;

        auto timer = boost::asio::steady_timer(exec);

        auto interval = duration_cast<steady_clock::duration>(duration<double>(heartbeat_interval));

        try {
            while(running) {
                
                // get current timestamp...
                auto start = clock::now();
                co_await dbat::api::acceptNewConnections();
                co_await dbat::api::handleNewInput();
                runOneLoop(heartbeat_interval);
                co_await dbat::api::handleNewOutput();
                co_await dbat::api::handleDisconnections();
                auto end =  clock::now();

                save_timer -= heartbeat_interval;
                if(save_timer <= 0.0) {
                    save_timer = save_interval;
                    co_await dbat::save::runSaveAsync();
                }

                auto elapsed = end - start;
                auto remaining = interval - elapsed;

                // This section processes network requests within the remaining time budget.
                // It will process at least one if there is one to do, though.
                start = clock::now();
                auto processed = co_await drain_request_channel(remaining);
                //LDEBUG("Processed {} requests in this tick.", processed);
                end = clock::now();
                elapsed = end - start;
                remaining = remaining - elapsed;

                if(remaining > clock::duration::zero()) {
                    timer.expires_after(remaining);
                    co_await timer.async_wait(boost::asio::use_awaitable);
                } else {
                    // just do a yield to avoid starving other coroutines.
                    co_await boost::asio::post(exec, boost::asio::use_awaitable);
                }
            }
        } catch(const std::exception &e) {
            LERROR("Exception in run_game: {}", e.what());
        }

        LINFO("Game loop has exited.");
        co_return;
    }
}