#include <string>
#include <optional>
#include <filesystem>
#include <expected>
#include <string_view>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace dbat::net {

    struct TlsConfig {
        boost::asio::ip::address address = boost::asio::ip::address_v6::any();
        uint16_t port{8080};
        std::filesystem::path cert_path;
        std::filesystem::path key_path;
    };

    struct Config {
        boost::asio::ip::address tcp_address = boost::asio::ip::address_v6::any();
        uint16_t tcp_port{8000};
    };

    template<typename T>
    struct Connection {
        T connection;
        bool isSecure{false};
        std::string hostname, address;
    };

    boost::asio::io_context& context();

    std::expected<boost::asio::ip::address, boost::system::error_code> parse_address(std::string_view addr_str);

    void bind_server(boost::asio::ip::address address, uint16_t port, std::shared_ptr<boost::asio::ssl::context> tls_context);

}