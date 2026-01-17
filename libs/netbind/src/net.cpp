#include "netbind/server.hpp"

#include <boost/algorithm/string.hpp>

namespace dbat::net {

    boost::asio::io_context& context() {
        static boost::asio::io_context ioc;
        return ioc;
    }

    std::expected<boost::asio::ip::address, boost::system::error_code> parse_address(std::string_view addr_str) {
        if(boost::iequals(addr_str, "any") || boost::iequals(addr_str, "*")) {
            return boost::asio::ip::address_v6::any();
        }
        try {
            auto address = boost::asio::ip::make_address(std::string(addr_str));
            return address;
        } catch(const std::exception& e) {
            boost::system::error_code ec = boost::asio::error::invalid_argument;
            return std::unexpected(ec);
        }
    }

    static boost::asio::awaitable<std::expected<std::string, boost::system::error_code>> reverse_lookup(boost::asio::ip::tcp::socket& socket) {
        boost::system::error_code ec;
        auto remote_endpoint = socket.remote_endpoint(ec);
        if(ec) {
            co_return std::unexpected(ec);
        }
        auto remote_address = remote_endpoint.address();
        boost::asio::ip::tcp::resolver resolver(co_await boost::asio::this_coro::executor);
        try {
            auto results = co_await resolver.async_resolve(
            remote_address.to_string(),
            "",
            boost::asio::ip::tcp::resolver::flags::numeric_service,
            boost::asio::use_awaitable
            );
            if(results.empty()) {
                co_return std::unexpected(boost::asio::error::host_not_found);
            }
            co_return results.begin()->host_name();
        } catch(const boost::system::system_error& e) {
            co_return std::unexpected(e.code());
        }
    }

    static boost::asio::awaitable<void> handle_client(boost::asio::ip::tcp::socket socket, std::shared_ptr<boost::asio::ssl::context> tls_context) {
        try {
            auto client_address = socket.remote_endpoint().address().to_string();
            auto client_hostname = client_address;
            LINFO("Incoming connection from %s", client_address.c_str());
            if(auto rev_res = co_await reverse_lookup(socket); rev_res) {
                client_hostname = rev_res.value();
                LINFO("Resolved hostname %s for %s", client_hostname.c_str(), client_address.c_str());
            } else {
                LINFO("Could not resolve hostname for %s: %s", client_address.c_str(), rev_res.error().message().c_str());
            }
            
            if(tls_context) {
                auto ssl_socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(std::move(socket), *tls_context);
                boost::system::error_code ec;
                co_await ssl_socket.async_handshake(boost::asio::ssl::stream_base::server, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                if(ec) {
                    LERROR("TLS handshake failed with %s: %s", client_hostname.c_str(), ec.message().c_str());
                    co_return;
                }
                LINFO("Completed TLS handshake with %s", client_hostname.c_str());
                auto con = Connection{std::move(ssl_socket), true, client_hostname, client_address};
                co_await handle_http(con);
            } else {
                auto con = Connection{std::move(socket), false, client_hostname, client_address};
                co_await handle_http(con);
            }
        }
        catch(const std::exception& e) {
            LERROR("Error handling client: %s", e.what());
        }
    }

    static boost::asio::awaitable<void> run_server(boost::asio::ip::tcp::acceptor acceptor, std::shared_ptr<boost::asio::ssl::context> tls_context) {
        for(;;) {
            boost::system::error_code ec;
            auto socket = co_await acceptor.async_accept(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
            if(ec) {
                LERROR("Accept error: %s", ec.message().c_str());
                continue;
            }
            auto strand = boost::asio::make_strand(context());
            boost::asio::co_spawn(strand,
                handle_client(std::move(socket), tls_context),
                boost::asio::detached
            );
        }
    }

    void bind_server(boost::asio::ip::address address, uint16_t port, std::shared_ptr<boost::asio::ssl::context> tls_context) {
        // let's create an acceptor.
        boost::asio::ip::tcp::endpoint endpoint(address, port);
        // create a stranded acceptor
        auto strand = boost::asio::make_strand(context());
        boost::asio::ip::tcp::acceptor acceptor(strand, endpoint, true);
        boost::asio::co_spawn(strand,
            run_server(std::move(acceptor), tls_context),
            boost::asio::detached
        );
    }

}