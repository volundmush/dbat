#pragma once
#include <pqxx/pqxx>

namespace dbat::link {
    int run_game(double heartbeat_interval, double save_interval);
}
