#pragma once
#include "net.h"
#include <boost/asio/experimental/awaitable_operators.hpp>

namespace net {

	using namespace boost::asio::experimental::awaitable_operators;

    enum TelnetMsgType : char8_t {
        AppData = 0, // random telnet bytes
        Command = 1, // an IAC <something>
        Negotiation = 2, // an IAC WILL/WONT/DO/DONT
        Subnegotiation = 3 // an IAC SB <code> <data> IAC SE
    };

    struct TelnetMessage {
        TelnetMsgType msg_type;
        std::string data;
        char8_t codes[2] = {0, 0};
        std::size_t parse(std::string_view buf);
        std::string toString() const;

    };

    struct telnet_data : connection_data {
		explicit telnet_data(boost::beast::tcp_stream conn);
        boost::beast::tcp_stream conn;
		awaitable<void> runReader();
        awaitable<void> runWriter();
        awaitable<void> runHandshake();
        awaitable<void> run() override;
        awaitable<void> processInbuf();
        awaitable<void> handleTelnetMessage(const TelnetMessage& msg);
        awaitable<void> handleTelnetCommand(char cmd);
        awaitable<void> handleSubnegotiation(char cmd, std::string_view data);
        awaitable<void> handleNegotiation(char cmd, char op);
        awaitable<void> processAppData();
        awaitable<void> startConnection() override;
        awaitable<void> flushMessages();
        awaitable<void> halt(int mode) override;

        nlohmann::json serialize() override;
        void deserialize(const nlohmann::json& j) override;

        flat_buffer inbuf, outbuf, appbuf;
    };

}