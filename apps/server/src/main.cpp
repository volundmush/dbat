#include "netbind/net.hpp"
#include "dbat/db.h"
#include "logging/Log.hpp"
#include "dbat/comm.h"
#include "serde/Startup.h"
#include "serde/saveload.h"


static bool running{false};

boost::asio::awaitable<void> async_distribute_output() {

    co_return;
}

boost::asio::awaitable<void> run_game(double heartbeat_interval, double save_interval) {
    using namespace std::chrono;

    running = true;

    double save_timer = save_interval;

    auto exec = co_await boost::asio::this_coro::executor;

    auto timer = boost::asio::steady_timer(exec);

    auto interval = duration_cast<steady_clock::duration>(duration<double>(heartbeat_interval));

    try {
        while(running) {
            // get current timestamp...
            auto start = high_resolution_clock::now();
            runOneLoop(heartbeat_interval);
            co_await async_distribute_output();
            auto end =  high_resolution_clock::now();

            save_timer -= heartbeat_interval;
            if(save_timer <= 0.0) {
                save_timer = save_interval;
                dbat::save::runSaveSync();
            }

            auto elapsed = end - start;
            auto remaining = interval - elapsed;

            if(remaining > steady_clock::duration::zero()) {
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

    dbat::log::init();

    auto& ioc = dbat::net::context();

    auto address = boost::asio::ip::address_v6::any();

    uint16_t port = 8080;

    std::shared_ptr<boost::asio::ssl::context> tls_context = nullptr;
    tls_context.reset();

    dbat::net::bind_server(address, port, tls_context);

    load_config();
    dbat::init::init();

    auto strand = boost::asio::make_strand(ioc);
    boost::asio::co_spawn(strand, run_game(0.05, 300.0), boost::asio::detached);

    ioc.run();

    return 0;
}