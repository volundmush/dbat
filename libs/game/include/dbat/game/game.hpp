#pragma once
#include <pqxx/pqxx>
#include <memory>

namespace dbat::link {
    extern std::unique_ptr<pqxx::connection> db_conn;
    int run_game(double heartbeat_interval, double save_interval);
}
