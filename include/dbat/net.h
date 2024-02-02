#pragma once
#include "sysdep.h"
#include "json.hpp"
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

    enum class ConnectionState : uint8_t {
        Negotiating = 0,
        Pending = 1,
        Connected = 2,
        Dead = 3
    };

    extern std::mutex connectionMutex;
    extern std::unordered_map<std::string, std::shared_ptr<Connection>> connections;

    class Connection;

    std::shared_ptr<Connection> newConnection(const std::string& connID);

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
        explicit Connection(const std::string& connId);
        void sendGMCP(const std::string &cmd, const nlohmann::json &j);
        void sendText(const std::string &messg);
        void sendEvent(const std::string &name, const nlohmann::json &data);
        void onHeartbeat(double deltaTime);
        void onNetworkDisconnected();
        void onWelcome();
        void close();
        void executeGMCP(const std::string& cmd, const nlohmann::json& j);
        void executeCommand(const std::string& cmd);

        void cleanup();
        void setParser(ConnectionParser *p);

        bool running{true};
        std::string connId{};
        account_data *account{};
        int64_t adminLevel{0};
        struct descriptor_data *desc{};

        void queueMessage(const std::string& event, const std::string& data);

        void handleEvent(const std::string& event, const nlohmann::json& data);

        std::list<std::pair<std::string, std::string>> outQueue, inQueue;

        // Some time structs to handle when we received connections.
        // These probably need some updating on this and Thermite side...
        std::chrono::system_clock::time_point connected{};
        std::chrono::steady_clock::time_point connectedSteady{}, lastActivity{}, lastMsg{};

        // This is embedded for ease of segmentation but this struct isn't
        // actually used anywhere else.
        ProtocolCapabilities capabilities{};

        std::unique_ptr<ConnectionParser> parser;

        ConnectionState state{ConnectionState::Negotiating};
    protected:

    };

}