#pragma once
#include "sysdep.h"
#include "defs.h"
#include <mutex>
#include <boost/beast/core.hpp>
#include "libtelnet.h"

namespace net {
    using namespace std::chrono_literals;

    extern void init();
    extern void prepareForCopyover();

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
        int16_t hostPort{0};

        ColorType colorType = ColorType::NoColor;

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
        [[nodiscard]] std::string protocolName() const;
    };

    enum class ConnectionState : uint8_t {
        Pending = 0,
        Connected = 1,
        Dead = 2
    };

    enum class DisconnectReason {
        // In these first two examples, the connection is TCP-dead.

        // ConnectionLost means a timeout or similar error on TCP.
        ConnectionLost = 0,
        // ConnectionClosed means we got a 0 on a read(). Whether the user wanted this to happen, we dunnno
        ConnectionClosed = 1,

        // This last disconnectreason was initiated by the game. usually from the QUIT command
        GameLogoff = 2,
    };

    extern std::unordered_map<int, std::shared_ptr<Connection>> connections;
    extern std::unordered_map<int, DisconnectReason> deadConnections;
    extern std::set<int> pendingConnections, pendingOutData, pendingReads, pendingWrites;

    extern int server_fd;
    extern void update(double deltaTime);
    extern void acceptAllIncomingConnections();
    extern void prepareForCopyover();
    extern void telnet_handler(telnet_t *telnet, telnet_event_t *event, void *user_data);

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
        explicit Connection(int connId);
        Connection(int connId, const nlohmann::json& j);
        ~Connection();
        void sendText(const std::string &messg);
        void update(double deltaTime);
        void onNetworkDisconnected();
        void onWelcome();
        void close();

        nlohmann::json serialize();
        void deserialize(const nlohmann::json& j);

        void cleanup(DisconnectReason reason);
        void setParser(ConnectionParser *p);

        bool running{true};
        int connId{};
        account_data *account{};
        int64_t adminLevel{0};

        struct descriptor_data *desc{};

        // Some time structs to handle when we received connections.
        // lastReceivedBytes is used to track network activity.
        std::chrono::system_clock::time_point connected{};
        std::chrono::steady_clock::time_point connectedSteady{}, lastActivity{}, lastReceivedBytes{}, lastMsg{};

        // This is embedded for ease of segmentation but this struct isn't
        // actually used anywhere else.
        ProtocolCapabilities capabilities{};

        std::deque<std::string> pendingCommands;

        std::unique_ptr<ConnectionParser> parser;

        ConnectionState state{ConnectionState::Pending};

        void epollRegister();
        void epollUnregister();
        void readFromSocket();
        void writeToSocket();
        void handleTelnet(telnet_event_t *event);

        void handleAppData();

        void sendTelnetNegotiations();

        void handleTelnetCommand(char cmd);

        void handleTelnetWill(char telopt);
        void handleTelnetDo(char telopt);
        void handleTelnetWont(char telopt);
        void handleTelnetDont(char telopt);

        void handleTelnetSubNegotiate(char telopt, const char *buffer, size_t size);

        void disableCompression();

    protected:
        std::vector<char> outbuf, appbuf;
        telnet_t *teldata;

    };

}
