#pragma once
#include "api/channel.hpp"

namespace dbat::api {
    boost::asio::awaitable<void> run_game(double heartbeat_interval, double save_interval);
}