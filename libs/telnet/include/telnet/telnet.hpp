#pragma once
#include <string>
#include <string_view>
#include <variant>
#include <expected>
#include <vector>
#include <cstdint>
#include <functional>
#include <chrono>

#include <nlohmann/json.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>

#include "net/net.hpp"

#include "dbat/Descriptor.h"

namespace dbat::telnet {

    template<typename T>
    using Channel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

    namespace codes {
        constexpr char NUL = static_cast<char>(0);
        constexpr char SGA = static_cast<char>(3);
        constexpr char BEL = static_cast<char>(7);
        constexpr char IAC  = static_cast<char>(255);
        constexpr char DONT = static_cast<char>(254);
        constexpr char DO   = static_cast<char>(253);
        constexpr char WONT = static_cast<char>(252);
        constexpr char WILL = static_cast<char>(251);
        constexpr char SB   = static_cast<char>(250);
        constexpr char SE   = static_cast<char>(240);

        // Telnet Commands
        constexpr char NOP = static_cast<char>(241);
        constexpr char AYT = static_cast<char>(246);

        // Telnet Options
        constexpr char TERMINAL_TYPE = static_cast<char>(24);
        constexpr char TELOPT_EOR     = static_cast<char>(25);
        constexpr char NAWS          = static_cast<char>(31);
        constexpr char LINEMODE      = static_cast<char>(34);
        constexpr char MNES         = static_cast<char>(39);
        constexpr char CHARSET      = static_cast<char>(42);
        constexpr char MSSP          = static_cast<char>(70);
        constexpr char MCCP1         = static_cast<char>(85);
        constexpr char MCCP2         = static_cast<char>(86);
        constexpr char MCCP3         = static_cast<char>(87);
        constexpr char MXP          = static_cast<char>(91);
        constexpr char GMCP          = static_cast<char>(201);
    }

    struct TelnetMessageData {
        std::string data;
    };

    struct TelnetMessageSubnegotiation {
        char option; // e.g., TERMINAL TYPE, MCCP, etc.
        std::string data;
    };

    struct TelnetMessageNegotiation {
        char command; // WILL, WONT, DO, DONT
        char option;
    };

    struct TelnetMessageCommand {
        char command; // e.g., NOP, AYT, etc.
    };

    struct TelnetMessageMSSP {
        std::vector<std::pair<std::string, std::string>> variables;
    };

    struct TelnetMessageNAWS {
        uint16_t width;
        uint16_t height;
    };

    struct TelnetMessageCompress {
        char version; // e.g., MCCP1, MCCP2
        bool state;
    };

    struct TelnetMessageGMCP {
        std::string package;
        nlohmann::json data;
    };

    struct TelnetMessageMTTS {
        std::string data;
    };

    struct TelnetMessageCharset {
        char command; // REQUEST, ACCEPTED, REJECTED, etc.
        std::string charsets;
    };

    struct TelnetMessageMNESRequest {
        std::vector<std::string> entries;
    };

    struct TelnetMessageMNESResponse {
        std::unordered_map<std::string, std::string> entries;
    };

    struct TelnetChangeCapabilities {
        nlohmann::json capabilities;
    };

    struct TelnetError {
        std::string message;
    };
    
    using TelnetMessage = std::variant<TelnetMessageData, TelnetMessageSubnegotiation, 
        TelnetMessageNegotiation, TelnetMessageCommand, TelnetMessageMSSP, 
        TelnetMessageNAWS, TelnetMessageCompress, TelnetMessageGMCP, 
        TelnetMessageMTTS, TelnetMessageCharset, TelnetError>;

    // attempts to parse a telnet message from the given data buffer.
    // On success, returns the parsed TelnetMessage and the number of bytes consumed.
    // On failure, returns an error string.
    std::expected<std::pair<TelnetMessage, size_t>, std::string> parseTelnetMessage(std::string_view data);

    struct TelnetConnection;

    struct TelnetOptionState {
        bool enabled{false};
        bool negotiating{false};
    };

    struct TelnetOptionPerspective {
        TelnetOptionState local;   // our side
        TelnetOptionState remote;  // their side
    };

    class TelnetConnection {
        public:
        TelnetConnection(dbat::net::AnyConnection connection);
        
        boost::asio::awaitable<void> run(boost::asio::steady_timer::duration negotiation_timeout);

        private:
        dbat::net::AnyConnection conn_;
        ClientData client_data_;
        nlohmann::json changed_capabilities_;
        
        boost::asio::awaitable<void> runReader();
        boost::asio::awaitable<void> runWriter();
        boost::asio::awaitable<void> runMain(boost::asio::steady_timer::duration negotiation_timeout);
        boost::asio::awaitable<void> processData(std::string_view data);
        boost::asio::awaitable<void> sendAppData(std::string_view app_data);
        boost::asio::awaitable<void> sendSubNegotiation(char option, std::string_view sub_data);

        std::unordered_map<char, TelnetOptionPerspective> option_states_;
    };

}
