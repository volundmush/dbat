#include "dotenv/dotenv.hpp"
#include "logging/Log.hpp"
#include "api/net.hpp"
#include "config/config.hpp"

int main(int argc, char** argv) {
    dbat::config::init("portal");

    dbat::net::init();
    dbat::net::start_servers();

    auto& ioc = dbat::net::context();
    ioc.run();

    return 0;
}
