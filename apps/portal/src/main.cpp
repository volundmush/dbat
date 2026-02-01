#include <list>
#include "volcano/log/Log.hpp"
#include "volcano/config/config.hpp"
#include "dbat/portal/Client.hpp"

boost::asio::awaitable<void> run_portal(std::list<volcano::net::Server>& servers, std::string_view target_str) {

    auto& ioc = volcano::net::context();

    auto target_res = co_await volcano::web::parse_http_target(target_str);
    if(!target_res) {
        LERROR("Target {} does not resolve properly: {}", target_str, target_res.error());
        // stop the ioc then return!
        ioc.stop();
        co_return;
    }

    boost::asio::co_spawn(ioc, volcano::portal::run_portal_links(), boost::asio::detached);

    volcano::portal::target = *target_res;

    for(auto& server : servers) {
        boost::asio::co_spawn(
            volcano::net::context(),
            server.run(),
            boost::asio::detached
        );
    };

    co_return;
}

int main(int argc, char** argv) {
    auto conf = volcano::config::init("portal");

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

    auto& ioc = volcano::net::context();

    volcano::portal::handle_refresh_timer = dbat::portal::refresh_jwt;

    std::list<volcano::net::Server> servers;

    if(conf.telnet.port > 0) {
        auto endpoint = boost::asio::ip::tcp::endpoint(
            conf.telnet.address,
            conf.telnet.port
        );
        boost::asio::ip::tcp::acceptor acc(
            ioc,
            endpoint,
            true
        );
        servers.emplace_back(
            std::move(acc),
            nullptr,
            volcano::portal::handle_telnet
        );
        LINFO("Starting TELNET server on {}:{}", endpoint.address().to_string(), endpoint.port());
    }

    if(conf.telnets.port > 0) {
        if(!ssl_ctx) {
            LERROR("TLS is not configured, cannot start TELNETS server.");
            return 1;
        }
        auto endpoint = boost::asio::ip::tcp::endpoint(
            conf.telnets.address,
            conf.telnets.port
        );
        boost::asio::ip::tcp::acceptor acc(
            ioc,
            endpoint,
            true
        );
        servers.emplace_back(
            std::move(acc),
            ssl_ctx,
            volcano::portal::handle_telnet
        );
        LINFO("Starting TELNETS server on {}:{}", endpoint.address().to_string(), endpoint.port());
    }

    boost::asio::co_spawn(
        volcano::net::context(),
        run_portal(servers, conf.server_address),
        boost::asio::detached
    );

    volcano::portal::create_initial_mode_handler = [](volcano::portal::Client& client) {
        return std::make_shared<dbat::portal::LoginMode>(client);
    };

    volcano::net::run(1);

    return 0;
}
