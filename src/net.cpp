#include "net.h"
#include "utils.h"
#include "telnet.h"
#include <regex>

namespace net {
    std::unique_ptr<io_context> io;
    std::unique_ptr<ip::tcp::acceptor> acceptor;
    std::unique_ptr<signal_set> signals;

    std::list<struct descriptor_data*> new_descriptors;

    Message::Message() {
        cmd = "";
        args = nlohmann::json::array();
        kwargs = nlohmann::json::object();
    }

    // Regular expression pattern for HTTP request line: METHOD URL HTTP/VERSION
    static std::regex http_request_line_regex(
            R"(^(OPTIONS|GET|HEAD|POST|PUT|DELETE|TRACE|CONNECT)\s+\S+\s+HTTP/\d\.\d$)",
            std::regex::ECMAScript | std::regex::icase
    );

    static bool isValidHttpRequestLine(const std::string& line) {
        // Check if the line matches the HTTP request line format
        return std::regex_match(line, http_request_line_regex);
    }

    static awaitable<void> runTelnet(boost::beast::tcp_stream conn, const boost::beast::flat_buffer& oldbuf) {
		telnet_data tel(std::move(conn));
        // copy oldbuf to tel.inbuf...
        tel.inbuf = oldbuf;
        co_await tel.run();
        co_return;
    }

    static awaitable<void> runHttp(boost::beast::tcp_stream conn, const boost::beast::flat_buffer& oldbuf) {
		// TODO: handle HTTP...
        co_return;
    }


    // define a function that takes a milliseconds duration...
    static awaitable<void> runTimer(std::chrono::milliseconds duration, bool *timedOut) {
        // create a timer that expires after the given duration...
        boost::asio::steady_timer timer(co_await this_coro::executor, duration);
        // wait for the timer to expire...
        co_await timer.async_wait(use_awaitable);
        // the timer has expired...
        *timedOut = true;
        co_return;
    }

    static awaitable<void> acceptConnection(boost::beast::tcp_stream conn) {
        // Each connection gets 100ms with which to submit its opening handshake... or nothing at all.
        // We are going to accept both MUD telnet and HTTP 1.1/2.x connections (which might be upgraded to websocket).
        // If the client opens up with a valid beginning of an HTTP request then we'll assume it's HTTP.
        // If it doesn't match, or the timeout occurs (using tcp_stream's expires_after), then we'll assume it's telnet.

        // We'll use a timeout of 100ms for the initial handshake.
        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::string_body> req;

        bool is_http = false;

        try {
            // Attempt to read the first line of the HTTP request

            // Use a composable OR awaitable operation combining async_read_until an "\n" and a lambda waiter that
            // sets timedOut to true.
            bool timedOut = false;

            co_await (async_read_until(conn, buffer, "\n", use_awaitable) || runTimer(std::chrono::milliseconds(100), &timedOut));

            if(!timedOut) {
                // Parse the first line of the HTTP request
                std::string first_line = boost::beast::buffers_to_string(buffer.data());

                // Check if the first line is a valid HTTP request line
                if (isValidHttpRequestLine(first_line)) {
                    is_http = true;
                }
            }
        }
        catch (boost::system::error_code& ec) {
            // An error occurred during async_read_until (e.g., the connection was closed or the read timed out)
            if (ec == boost::asio::error::operation_aborted) {
                // TODO: Handle error
                log("net.acceptConnection: %s", ec.message().c_str());
                co_return;
            } else {
                log("net.acceptConnection: %s", ec.message().c_str());
                // Some other error occurred
                // TODO: Handle error
                co_return;
            }
        }

        if(is_http) {
            co_await runHttp(std::move(conn), buffer);
        } else {
            co_await runTelnet(std::move(conn), buffer);
        }

        co_return;
    }

    awaitable<void> runAcceptor() {
        while(true) {
            try {
                // Construct a stream with the io_context of the acceptor
                boost::beast::tcp_stream stream(acceptor->get_executor());

                // Perform the async_accept operation on the stream
                co_await acceptor->async_accept(stream.socket(), use_awaitable);

                // At this point, the stream is connected and can be used for read/write operations
                // spawn acceptConnection on a new strand...
                co_spawn(make_strand(acceptor->get_executor()), [stream = std::move(stream)]() mutable {
                    return acceptConnection(std::move(stream));
                }, detached);

            } catch (boost::system::error_code &e) {
                log("net.runAcceptor: %s", e.what().c_str());
            }
        }
        co_return;
    }

    void connection_data::createDescriptor() {
        desc = new descriptor_data();
        desc->conn = this;
        new_descriptors.push_back(desc);
    }

    void connection_data::sendText(const std::string &text) {
        if(text.empty()) return;
        auto &msg = outMessages.emplace_back();
        msg.cmd = "text";
        msg.args.push_back(text);
    }

}