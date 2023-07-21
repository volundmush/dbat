#pragma once
#include "sysdep.h"
#include "nlohmann/json.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>


namespace net {
    using namespace boost::asio;
    using namespace boost::beast;

    extern std::unique_ptr<io_context> io;

    extern std::unique_ptr<ip::tcp::acceptor> acceptor;

    extern std::unique_ptr<signal_set> signals;

    extern std::list<struct descriptor_data*> new_descriptors;

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
        std::string cmd;
        nlohmann::json args;
        nlohmann::json kwargs;
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

    struct connection_data {
		connection_data() = default;
        virtual ~connection_data() = default;

        ProtocolCapabilities capabilities;

        //virtual void onNetworkDisconnected();
        virtual awaitable<void> startConnection() = 0;

        void sendText(const std::string& text);

        void createDescriptor();

        virtual awaitable<void> run() = 0;

        std::list<Message> outMessages, inMessages;

        struct descriptor_data* desc{nullptr};

    };

}