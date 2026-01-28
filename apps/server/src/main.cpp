#include "volcano/config/config.hpp"
#include "dbat/api/endpoints.hpp"
#include "dbat/api/game.hpp"
#include "dbat/game/db.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/serde/Startup.hpp"
#include "dbat/serde/saveload.hpp"

boost::asio::awaitable<void> run_portal(volcano::net::Server server, std::string_view target_str) {

    auto& ioc = volcano::net::context();

    auto target_res = volcano::web::parse_http_target(target_str);
    if(!target_res) {
        LERROR("", target_res.error());
        // stop the ioc then return!
        co_return;
    }

    boost::asio::co_spawn(
        volcano::net::context(),
        server.run(),
        boost::asio::detached
    );

    

    auto strand = boost::asio::make_strand(ioc);
    boost::asio::co_spawn(strand, dbat::api::run_game(0.05, 300.0), boost::asio::detached);

    co_return;
}

int main(int argc, char** argv) {

    // Pre setup
    auto conf = volcano::config::init("server");
    dbat::api::jwt.secret = conf.jwt.secret;
    dbat::api::jwt.token_expiry = std::chrono::seconds(conf.jwt.expiry_minutes * 60);
    dbat::api::jwt.refresh_token_expiry = std::chrono::seconds(conf.jwt.refresh_expiry_minutes * 60);
    dbat::api::jwt.issuer = conf.jwt.issuer;
    dbat::api::jwt.audience = conf.jwt.audience;
    LINFO("JWT configured: issuer='{}', audience='{}', expiry={} minutes, refresh expiry={} minutes",
        dbat::api::jwt.issuer,
        dbat::api::jwt.audience,
        conf.jwt.expiry_minutes,
        conf.jwt.refresh_expiry_minutes
    );

    std::shared_ptr<boost::asio::ssl::context> ssl_ctx;

    if(!conf.tls.cert_path.empty() && !conf.tls.key_path.empty()) {
        auto tls = volcano::net::create_ssl_context(conf.tls.cert_path, conf.tls.key_path);
        if(!tls) {
            LERROR("Failed to create TLS context: {}", tls.error());
            return 1;
        }
        ssl_ctx = *tls;
        LINFO("TLS enabled on server.");
    } else {
        LINFO("TLS not enabled on server.");
    }

    auto router = dbat::api::init_router();
    auto handler = volcano::web::make_router_handler(router);

    auto &endpoint_config = ssl_ctx ? conf.https : conf.http;
    
    auto endpoint = boost::asio::ip::tcp::endpoint(endpoint_config.address, endpoint_config.port);

    boost::asio::ip::tcp::acceptor acc(
        volcano::net::context(),
        endpoint,
        true
    );

    volcano::net::Server server(
        std::move(acc),
        ssl_ctx,
        handler
    );
    LINFO("Starting {} server on {}:{}", ssl_ctx ? "HTTPS" : "HTTP", endpoint.address().to_string(), endpoint.port());
    

    // game setup
    load_config();
    dbat::init::init();

    boost::asio::co_spawn(
        volcano::net::context(),
        run_portal(std::move(server)),
        boost::asio::detached
    );

    volcano::net::run();

    return 0;
}