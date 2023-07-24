#pragma once
#include "sysdep.h"
#include "nlohmann/json.hpp"
#include "defs.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <mutex>


namespace net {
    using namespace boost::asio;
    using namespace boost::beast;

    template<typename T>
    using Channel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

    extern std::unique_ptr<io_context> io;

    extern std::unique_ptr<ip::tcp::acceptor> acceptor;

    extern std::unique_ptr<signal_set> signals;

    extern std::unique_ptr<Channel<descriptor_data*>> pending_descriptors;

    extern std::mutex connectionsMutex;

    awaitable<void> runAcceptor();

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

    struct Message {
        Message();
        explicit Message(const nlohmann::json& j);
        std::string cmd;
        nlohmann::json args;
        nlohmann::json kwargs;
        nlohmann::json serialize();
    };

    struct ProtocolCapabilities {
        Protocol protocol{Protocol::Telnet};
        bool encryption = false;
        std::string clientName = "UNKNOWN", clientVersion = "UNKNOWN";
        std::string hostAddress = "UNKNOWN";
        int16_t hostPort{0};
        std::vector<std::string> hostNames{};
        std::string encoding;
        bool utf8 = false;
        ColorType colorType = ColorType::NoColor;
        int width = 80, height = 52;
        bool gmcp = false, msdp = false, mssp = false, mxp = false;
        bool mccp2 = false, mccp2_active = false, mccp3 = false, mccp3_active = false;
        bool ttype = false, naws = true, sga = true, linemode = false;
        bool force_endline = false, oob = false, tls = false;
        bool screen_reader = false, mouse_tracking = false, vt100 = false;
        bool osc_color_palette = false, proxy = false, mnes = false;

        void deserialize(const nlohmann::json& j);
        nlohmann::json serialize();
    };

    class ConnectionParser {
    public:
        explicit ConnectionParser(connection_data *conn) : conn(conn) {}
        virtual ~ConnectionParser() = default;
        virtual void handleMessage(const Message &m);
        virtual void parse(const std::string &txt) = 0;
        virtual void start();
    protected:
        void sendText(const std::string &txt);
        struct connection_data *conn;
    };

    enum class ConnState {
        Pending = 0,
        Connected = 1,
        Running = 2,
        Copyover = 3,
        Shutdown = 4,
        Disconnecting = 5,
        ConnectionLost = 6
    };

    struct connection_data {
		connection_data();
        virtual ~connection_data() = default;
        uint16_t socket{};
        ConnState state{ConnState::Pending};
        ProtocolCapabilities capabilities;
        Channel<Message> outMessages, inMessages;
        struct descriptor_data* desc{nullptr};
        struct account_data *account{nullptr};
        ConnectionParser *parser{nullptr};

        //virtual void onNetworkDisconnected();
        virtual awaitable<void> startConnection() = 0;
        awaitable<void> runInQueue();
        awaitable<void> handleInMessage(const Message &m);
        void sendText(const std::string& text);
        virtual awaitable<void> run() = 0;
        virtual awaitable<void> halt(int mode) = 0;
        void setParser(ConnectionParser *p);

        virtual nlohmann::json serialize();
        virtual void deserialize(const nlohmann::json& j);
    };

    awaitable<void> runTimer(std::chrono::milliseconds duration, bool *timedOut);
	extern std::map<int, connection_data*> connections;
}