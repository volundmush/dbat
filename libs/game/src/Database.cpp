#include "dbat/game/Database.hpp"

namespace dbat::db {
    std::unique_ptr<pqxx::connection> conn;
    std::unique_ptr<pqxx::work> txn;
}
