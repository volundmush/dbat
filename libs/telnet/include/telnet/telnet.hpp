#pragma once
#include <string>
#include <string_view>
#include <variant>
#include <expected>
#include <vector>
#include <cstdint>

#include <nlohmann/json.hpp>

namespace dbat::telnet {

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

    struct TelnetError {
        std::string message;
    };
    
    using TelnetMessage = std::variant<TelnetMessageData, TelnetMessageSubnegotiation, 
        TelnetMessageNegotiation, TelnetMessageCommand, TelnetMessageMSSP, 
        TelnetMessageNAWS, TelnetMessageCompress, TelnetMessageGMCP, 
        TelnetMessageMTTS, TelnetMessageCharset, TelnetError>;

    enum class TelnetMode {
        client,
        server
    };

    // attempts to parse a telnet message from the given data buffer.
    // On success, returns the parsed TelnetMessage and the number of bytes consumed.
    // On failure, returns an error string.
    std::expected<std::pair<TelnetMessage, size_t>, std::string> parseTelnetMessage(std::string_view data, TelnetMode mode);
}
