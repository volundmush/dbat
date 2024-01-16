#pragma once
#include "sysdep.h"
#include "nlohmann/json.hpp"
#include "defs.h"
#include <mutex>

namespace net {

    struct ProtocolCapabilities {
        std::string protocolName = "UNKNOWN";
        std::string clientName = "UNKNOWN", clientVersion = "UNKNOWN";
        std::string hostAddress = "UNKNOWN";
        std::vector<std::string> hostNames{};
        int16_t hostPort{0};
    };

    extern std::map<int64_t, std::shared_ptr<Connection>> connections;
    extern std::set<int64_t> pendingConnections;

    extern std::set<int64_t> deadConnections;

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
        explicit Connection(int64_t connId);
        void sendGMCP(const std::string &cmd, const nlohmann::json &j);
        void sendText(const std::string &messg);
        void onHeartbeat(double deltaTime);
        void onNetworkDisconnected();
        void onWelcome();
        void close();
        void handleGMCP(const std::string& cmd, const nlohmann::json& j);
        void handleCommand(const std::string& cmd);
        void handleMessage(const nlohmann::json &j);

        void cleanup();
        void setParser(ConnectionParser *p);

        std::list<std::string> inQueue, outQueue;

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

    };

    extern std::shared_ptr<Connection> newConnection();

}