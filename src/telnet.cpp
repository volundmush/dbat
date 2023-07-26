#include "telnet.h"
#include <string_view>
#include "structs.h"
#include <boost/beast/core/detail/base64.hpp>
#include "login.h"

namespace net {

    namespace telnet {

        constexpr char8_t NUL = 0, BEL = 7, CR = 13, LF = 10, SGA = 3, TELOPT_EOR = 25, NAWS = 31;
        constexpr char8_t LINEMODE = 34, EOR = 239, SE = 240, NOP = 241, GA = 249, SB = 250;
        constexpr char8_t WILL = 251, WONT = 252, DO = 253, DONT = 254, IAC = 255, MNES = 39;
        constexpr char8_t MXP = 91, MSSP = 70, MCCP2 = 86, MCCP3 = 87, GMCP = 201, MSDP = 69;
        constexpr char8_t MTTS = 24;
    }

    class TelnetOption;

    std::size_t TelnetMessage::parse(std::string_view buf) {
        using namespace telnet;
        // return early if nothing to do.
        auto available = buf.length();
        if(!available) return 0;

        // So we do have some data?
        auto begin = buf.begin(), end = buf.end();
        bool escaped = false;

        // first, we read ahead
        if((char8_t)*begin == IAC) {
            // If it begins with an IAC, then it's a Command, Negotiation, or Subnegotiation
            if(available < 2) {
                return 0; // not enough bytes available - do nothing;
            }
            // we have 2 or more bytes!
            auto b = begin;
            b++;
            auto sub = b;
            char8_t option = 0;

            switch((char8_t)*b) {
                case WILL:
                case WONT:
                case DO:
                case DONT:
                    // This is a negotiation.
                    if(available < 3) return 0; // negotiations require at least 3 bytes.
                    msg_type = TelnetMsgType::Negotiation;
                    codes[0] = *b;
                    codes[1] = *(++b);
                    return 3;
                case SB:
                    // This is a subnegotiation. We need at least 5 bytes for it to work.
                    if(available < 5) return 0;

                    option = *(++b);
                    b++;
                    sub = b;
                    // we must seek ahead until we have an unescaped IAC SE. If we don't have one, do nothing.

                    while(b != end) {
                        if(escaped) {
                            escaped = false;
                            b++;
                            continue;
                        }
                        if((char8_t)*b == IAC) {
                            b++;
                            if(b != end && (char8_t)*b == SE) {
                                // we have a winner!
                                msg_type = TelnetMsgType::Subnegotiation;
                                codes[0] = option;
                                b--;
                                std::copy(sub, b, std::back_inserter(data));
                                return 5 + data.size();
                            } else {
                                escaped = true;
                                b--;
                                continue;
                            }

                        } else {
                            b++;
                        }
                    }
                    // if we finished the while loop, we don't have enough data, so...
                    return 0;
                default:
                    // if it's any other kind of IAC, it's a Command.
                    msg_type = TelnetMsgType::Command;
                    codes[0] = (char8_t)*(++b);
                    return 2;
            };
        } else {
            // Data begins on something that isn't an IAC. Scan ahead until we reach one...
            // Send all data up to an IAC, or everything if there is no IAC, as data.
            msg_type = TelnetMsgType::AppData;
            auto check = std::find(begin, end, IAC);
            std::copy(begin, check, std::back_inserter(data));
            return data.size();
        }
    }

    std::string TelnetMessage::toString() const {
        std::string out;

        switch(msg_type) {
            case TelnetMsgType::AppData:
                return data;
            case TelnetMsgType::Command:
                out.push_back(telnet::IAC);
                out.push_back(codes[0]);
                break;
            case TelnetMsgType::Negotiation:
                out.push_back(telnet::IAC);
                out.push_back(codes[0]);
                out.push_back(codes[1]);
                break;
            case TelnetMsgType::Subnegotiation:
                out.push_back(telnet::IAC);
                out.push_back(telnet::SB);
                out.push_back(codes[0]);
                out += data;
                out.push_back(telnet::IAC);
                out.push_back(telnet::SE);
                break;
        }

        return out;
    }

    struct TelnetOptionPerspective {
        bool enabled = false, negotiating = false, answered = false;
    };

    telnet_data::telnet_data(boost::beast::tcp_stream conn) : connection_data(), conn(std::move(conn)) {
        capabilities.protocol = Protocol::Telnet;
        socket = conn.socket().native_handle();
    }

    awaitable<void> telnet_data::startConnection() {
        state = ConnState::Running;
        setParser(new LoginParser(this));
        if(appbuf.size()) co_await processAppData();
        co_return;
    }

    awaitable<void> telnet_data::runHandshake() {
        try {
            bool timedOut = false;
            co_await runTimer(std::chrono::milliseconds(100), &timedOut);
            co_await startConnection();
        } catch(boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted) {
                // The timer was cancelled, so we're good.
                co_return;
            } else {
                // Some other error occurred.
            }
        }

        co_return;
    }

    awaitable<void> telnet_data::run() {
        // start runTimer() first on the same executor/strand as conn...
        if(!desc) co_spawn(co_await boost::asio::this_coro::executor, runHandshake(), detached);
        try {
            co_await (runReader() || runWriter() || runInQueue());
        }
        catch(boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted) {
                // The strand was cancelled, so we're good.
                co_return;
            } else {
                // Some other error occurred.
            }
        }

        if(state == ConnState::Shutdown) {

        } else if(state == ConnState::Copyover) {

        }

        co_return;
    }

    awaitable<void> telnet_data::handleTelnetMessage(const TelnetMessage& msg) {
        // if it's IAC negotiation of some kind then we need to shove it at the right methods for that...
        // Otherwise if it's appdata, shove its data into appbuf...

        if(msg.msg_type == TelnetMsgType::Negotiation) {
            co_await handleNegotiation(msg.codes[0], msg.codes[1]);
        } else if(msg.msg_type == TelnetMsgType::Subnegotiation) {
            co_await handleSubnegotiation(msg.codes[0], msg.data);
        } else if(msg.msg_type == TelnetMsgType::Command) {
            co_await handleTelnetCommand(msg.codes[0]);
        } else if(msg.msg_type == TelnetMsgType::AppData) {
            // append msg.data into appbuf...
            auto p = appbuf.prepare(msg.data.size());
            memcpy(p.data(), msg.data.data(), msg.data.size());
            appbuf.commit(msg.data.size());
            if(state == ConnState::Running) co_await processAppData();
        }
        co_return;
    }

    awaitable<void> telnet_data::processAppData() {
        // Create a string_view of the buffer's contents
        std::string buffer_data = boost::beast::buffers_to_string(appbuf.data());

        // Look for \r\n
        size_t pos = 0;
        while((pos = buffer_data.find("\r\n")) != std::string_view::npos) {
            // Found \r\n, so grab the line
            Message msg;
            msg.cmd = "text";
            msg.args.push_back(buffer_data.substr(0, pos));
            inMessages.try_send(boost::system::error_code{}, msg);

            // Advance past the \r\n
            buffer_data = buffer_data.substr(pos + 2);
        }

        // Now remove the processed part from the buffer.
        appbuf.consume(appbuf.size() - buffer_data.size());

        co_return;
    }

    awaitable<void> telnet_data::processInbuf() {
        while(true) {
            TelnetMessage msg;

            auto result = msg.parse(boost::beast::buffers_to_string(inbuf.data()));
            if(!result) {
                // we don't have enough data to parse a message.
                break;
            }
            // we have a message! advance the buffer...
            inbuf.consume(result);

            // now, we need to process the message.
            co_await handleTelnetMessage(msg);
        }

        co_return;
    }

    awaitable<void> telnet_data::handleTelnetCommand(char cmd) {
		co_return;
    }

    awaitable<void> telnet_data::handleSubnegotiation(char cmd, std::string_view data) {
        co_return;
    }

    awaitable<void> telnet_data::handleNegotiation(char cmd, char op) {
        co_return;
    }

    awaitable<void> telnet_data::runReader() {
        try {
            while(true) {
                auto result = co_await conn.async_read_some(inbuf.prepare(1024), use_awaitable);
                inbuf.commit(result);
                co_await processInbuf();
            }
        }
        catch(boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted) {
                // The strand was cancelled, so we're good.
                co_return;
            } else {
                // Some other error occurred.
            }
        }
        co_return;
    }

    awaitable<void> telnet_data::flushMessages() {
        // We will be converting outgoing messages to bytes and shoving them into outbuf.
        // We will be doing this in a loop until there are no more messages to send.

        while(outMessages.ready()) {
            if(!outMessages.try_receive([&](boost::system::error_code ec, const Message &m) {
                if(!ec) {
                    if(m.cmd == "text") {
                        // iterate over m.args, all of which should be strings, and shove them into outbuf.
                        for(std::string arg : m.args) {
                            auto p = outbuf.prepare(arg.size());
                            memcpy(p.data(), arg.data(), arg.size());
                            outbuf.commit(arg.size());
                        }
                    }
                }
            })) continue;
            // for now let's only worry about text outMessages...
        }
        co_return;
    }

    awaitable<void> telnet_data::runWriter() {
        // Let's create a re-usable timer that we can await on for when there's no data in the outbuf or messages to
        // parse to bytes.
        auto ms = std::chrono::milliseconds(25);
        auto timer = steady_timer(co_await boost::asio::this_coro::executor, ms);
        try {
            while(true) {
                co_await flushMessages();
                if(!outbuf.size()) {
                    // we have no data to send. Wait for 25ms and try again.
                    timer.expires_after(ms);
                    co_await timer.async_wait(use_awaitable);
                    continue;
                }

                // we have data to send. Send it.
                auto result = co_await conn.async_write_some(outbuf.data(), use_awaitable);
                outbuf.consume(result);
            }
        }
        catch(boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted) {
                // The strand was cancelled, so we're good.
                co_return;
            } else {
                // Some other error occurred.
            }
        }
        co_return;
    }

    awaitable<void> telnet_data::halt(int mode) {
        if (mode == 0) {
            // close socket.
            state = ConnState::Disconnecting;
        }
        else if(mode == 1) {
            // copyover...
            state = ConnState::Copyover;
            auto s = conn.release_socket();
            socket = s.release();
        } else if(mode == 2) {
            // shutdown...
            state = ConnState::Shutdown;
        }
        conn.cancel();
        co_return;
    }

    // Convert a flat_buffer to a base64-encoded string
    static std::string toBase64(const boost::beast::flat_buffer& buffer)
    {
        auto const size = buffer.size();
        std::string result(boost::beast::detail::base64::encoded_size(size), '\0');
        boost::beast::detail::base64::encode(&result[0], static_cast<const char*>(buffer.data().data()), size);
        return result;
    }

	// Convert a base64-encoded string to a flat_buffer
    static void fromBase64(const std::string& str, boost::beast::flat_buffer& buffer)
    {
        auto const size = boost::beast::detail::base64::decoded_size(str.size());
        auto p = buffer.prepare(size);
        auto const result = boost::beast::detail::base64::decode(p.data(), str.data(), str.size());
        buffer.commit(result.first);
    }

    nlohmann::json telnet_data::serialize() {
        auto j = connection_data::serialize();

        if(inbuf.size()) j["inbuf"] = toBase64(inbuf);
        if(outbuf.size()) j["outbuf"] = toBase64(outbuf);
        if(appbuf.size()) j["appbuf"] = toBase64(appbuf);

        // TODO: serialize telnet-specific stuff

        return j;
    }

    void telnet_data::deserialize(const nlohmann::json& j) {
        connection_data::deserialize(j);

        if(j.contains("inbuf")) fromBase64(j["inbuf"].get<std::string>(), inbuf);
        if(j.contains("outbuf")) fromBase64(j["outbuf"].get<std::string>(), outbuf);
        if(j.contains("appbuf")) fromBase64(j["appbuf"].get<std::string>(), appbuf);

        // TODO: deserialize telnet-specific stuff
    }

}