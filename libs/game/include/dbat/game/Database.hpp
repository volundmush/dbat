#pragma once
#include <pqxx/pqxx>
#include <memory>

namespace dbat::db {
    extern std::unique_ptr<pqxx::connection> conn;
    extern std::unique_ptr<pqxx::work> txn;
}
