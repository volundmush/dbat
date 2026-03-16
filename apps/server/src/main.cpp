#include "dbat/game/Log.hpp"
#include "dbat/game/db.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/serde/Startup.hpp"
#include "dbat/serde/saveload.hpp"
#include "dbat/link/game.hpp"

int main(int argc, char** argv) {

    // Pre setup
    auto log_options = dbat::log::Options();
    log_options.file_path = "logs/dbat.log";
    dbat::log::init(log_options);

    // db setup
    dbat::link::db_conn = std::make_unique<pqxx::connection>("host=postgres port=5432 dbname=muforge user=muforge password=muforge");

    if(!dbat::link::db_conn) {
        LERROR("Failed to connect to database");
        return 1;
    }

    // game setup
    load_config();
    dbat::init::init();


    auto answer = dbat::link::run_game(0.05, 300.0);


    return 0;
}
