#include "volcano/config/config.hpp"

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

    auto& ioc = dbat::net::context();

    std::shared_ptr<volcano::net::Server> telnet_server, telnets_server;

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
        telnet_server = std::make_shared<volcano::net::Server>(
            std::move(acc),
            nullptr,
            dbat::net::make_telnet_handler()
        );
        LINFO("Starting TELNET server on {}:{}", endpoint.address().to_string(), endpoint.port());
        boost::asio::co_spawn(
            ioc,
            telnet_server->run(),
            boost::asio::detached
        );
    }

    volcano::net::run();

    return 0;
}
