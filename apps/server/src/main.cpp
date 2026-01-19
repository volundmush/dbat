#include <thread>
#include "netbind/server.hpp"
#include "dbat/db.h"
#include "logging/Log.hpp"
#include "dbat/comm.h"
#include "serde/Startup.h"
#include "serde/saveload.h"
#include "dotenv/dotenv.hpp"



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
            co_await dbat::net::acceptNewConnections();
            co_await dbat::net::handleNewInput();
            runOneLoop(heartbeat_interval);
            co_await dbat::net::handleNewOutput();
            co_await dbat::net::handleDisconnections();
            auto end =  clock::now();

            save_timer -= heartbeat_interval;
            if(save_timer <= 0.0) {
                save_timer = save_interval;
                dbat::save::runSaveSync();
            }

            auto elapsed = end - start;
            auto remaining = interval - elapsed;

            // This section processes network requests within the remaining time budget.
            // It will process at least one if there is one to do, though.
            start = clock::now();
            auto processed = co_await dbat::net::drain_request_channel(remaining);
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

int main(int argc, char** argv) {

    dbat::dotenv::load_env_file(".env", false);
    dbat::dotenv::load_env_file(".env.local", true);

    dbat::log::init();

    dbat::net::init();
    dbat::net::start_servers();

    load_config();
    dbat::init::init();

    auto& ioc = dbat::net::context();

    auto strand = boost::asio::make_strand(ioc);
    boost::asio::co_spawn(strand, run_game(0.05, 300.0), boost::asio::detached);

    ioc.run();

    return 0;
}