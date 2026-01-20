#include <thread>
#include "api/api.hpp"
#include "api/server.hpp"
#include "api/game.hpp"
#include "dbat/db.h"
#include "logging/Log.hpp"
#include "dbat/comm.h"
#include "serde/Startup.h"
#include "serde/saveload.h"
#include "config/config.hpp"


int main(int argc, char** argv) {

    // Pre setup
    dbat::config::init("server");
    dbat::api::init();

    // game setup
    load_config();
    dbat::init::init();

    auto& ioc = dbat::net::context();

    auto strand = boost::asio::make_strand(ioc);
    boost::asio::co_spawn(strand, dbat::api::run_game(0.05, 300.0), boost::asio::detached);

    ioc.run();

    return 0;
}