#pragma once
#include "sysdep.h"
#include "nlohmann/json.hpp"
#include "defs.h"
#include "shared/net.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast.hpp>
#include <mutex>

namespace net {
    using namespace std::chrono_literals;
    using namespace boost::asio;
    using namespace boost::beast;

    template<typename T>
    using Channel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

    using JsonChannel = Channel<nlohmann::json>;

    extern std::unique_ptr<io_context> io;

    extern std::unique_ptr<signal_set> signals;

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

    awaitable<void> runWebServer();

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

class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
public:
    explicit HttpConnection(boost::beast::tcp_stream socket);

    boost::asio::awaitable<void> run();

protected:
    boost::beast::tcp_stream socket;
    boost::beast::flat_buffer inBuf, outBuf;
    http::request_parser<http::string_body> parser;
    boost::asio::awaitable<void> runWebSocket(boost::beast::websocket::stream<boost::beast::tcp_stream> ws, std::string_view target);
    boost::asio::awaitable<bool> runWeb();
};

    class Connection : public std::enable_shared_from_this<Connection> {
    public:
        Connection(boost::beast::websocket::stream<boost::beast::tcp_stream> ws, const any_io_executor& ex, ProtocolCapabilities cap, int64_t connId);
        void sendGMCP(const std::string &cmd, const nlohmann::json &j);
        void sendText(const std::string &messg);
        boost::asio::awaitable<void> onHeartbeat(double deltaTime);
        void onNetworkDisconnected();
        void onWelcome();
        void close();

        boost::asio::awaitable<void> cleanup(DisconnectReason reason);

        void setParser(ConnectionParser *p);

        boost::asio::awaitable<void> run();

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

        Channel<GameMessage> inChan, outChan;

        std::unique_ptr<ConnectionParser> parser;
    protected:
        boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
        boost::asio::awaitable<void> runReader();
        boost::asio::awaitable<void> runWriter();
        boost::asio::awaitable<void> runPinger();
        void handleMessage(const GameMessage &msg);
        boost::beast::flat_buffer inBuf;
    };


}