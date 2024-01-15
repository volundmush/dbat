#pragma once
#include "sysdep.h"
#include "nlohmann/json.hpp"
#include "defs.h"
#include <mutex>

namespace net {

    enum class GameMessageType : uint8_t {
        Connect = 0,
        Update = 1,
        Command = 2,
        GMCP = 3,
        MSSP = 4,
        Disconnect = 5,
        Timeout = 6
    };

    struct GameMessage {
        GameMessage() = default;
        explicit GameMessage(const nlohmann::json& j);
        GameMessageType type{GameMessageType::Connect};
        nlohmann::json data;
        [[nodiscard]] nlohmann::json serialize() const;
    };

    enum class Protocol : uint8_t {
        Telnet = 0,
        WebSocket = 1
    };

    enum class ColorType : uint8_t {
        NoColor = 0,
        Standard = 1,
        Xterm256 = 2,
        TrueColor = 3
    };

    struct ProtocolCapabilities {
        Protocol protocol{Protocol::Telnet};

        std::string clientName = "UNKNOWN", clientVersion = "UNKNOWN";
        std::string hostAddress = "UNKNOWN";
        std::string encoding;
        std::vector<std::string> hostNames{};
        ColorType colorType = ColorType::NoColor;
        int16_t hostPort{0};

        bool encryption = false;
        bool utf8 = false;
        int width = 80, height = 52;
        bool gmcp = false, msdp = false, mssp = false, mxp = false;
        bool mccp2 = false, mccp2_active = false, mccp3 = false, mccp3_active = false;
        bool ttype = false, naws = false, sga = false, linemode = false;
        bool force_endline = false, oob = false, tls = false;
        bool screen_reader = false, mouse_tracking = false, vt100 = false;
        bool osc_color_palette = false, proxy = false, mnes = false;

        void deserialize(const nlohmann::json& j);
        nlohmann::json serialize();
        std::string protocolName();
    };

    enum class DisconnectReason {
        // In these first two examples, the connection is dead on the portal and we have been informed of such.
        ConnectionLost = 0,
        ConnectionClosed = 1,
        // In the remaining enums, the connection is still alive on the portal, but we are disconnecting for some reason.
        // We must inform the portal.
        GameLogoff = 2,
    };

    extern std::mutex connectionsMutex, pendingConnectionsMutex;
    extern std::map<int64_t, std::shared_ptr<Connection>> connections;
    extern std::set<int64_t> pendingConnections;

    extern std::unordered_map<int64_t, DisconnectReason> deadConnections;

    class Connection;

    class ConnectionParser {
    public:
        explicit ConnectionParser(const std::shared_ptr<Connection>& conn) : conn(conn) {}
        virtual ~ConnectionParser() = default;
        virtual void parse(const std::string &txt) = 0;
        virtual void handleGMCP(const std::string &txt, const nlohmann::json &j);
        virtual void start();
        virtual void close();
    protected:
        void sendText(const std::string &txt);
        std::shared_ptr<Connection> conn;
    };

    class Connection : public std::enable_shared_from_this<Connection> {
    public:
        Connection(int64_t connId);
        void sendGMCP(const std::string &cmd, const nlohmann::json &j);
        void sendText(const std::string &messg);
        void onHeartbeat(double deltaTime);
        void onNetworkDisconnected();
        void onWelcome();
        void close();

        void cleanup(DisconnectReason reason);

        void setParser(ConnectionParser *p);

        void run();

        int64_t connId{};
        account_data *account{};
        int64_t adminLevel{0};
        struct descriptor_data *desc{};

        // Some time structs to handle when we received connections.
        // These probably need some updating on this and Thermite side...
        std::chrono::system_clock::time_point connected{};
        std::chrono::steady_clock::time_point connectedSteady{}, lastActivity{}, lastMsg{};

        // This is embedded for ease of segmentation but this struct isn't
        // actually used anywhere else.
        ProtocolCapabilities capabilities{};

        std::unique_ptr<ConnectionParser> parser;
    protected:
        void handleMessage(const GameMessage &msg);
    };


}