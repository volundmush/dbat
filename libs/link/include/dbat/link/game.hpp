#pragma once
#include "channel.hpp"

namespace dbat::link {
    boost::asio::awaitable<void> run_game(double heartbeat_interval, double save_interval);
}
