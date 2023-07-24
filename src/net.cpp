#include "net.h"
#include "utils.h"
#include "telnet.h"
#include <regex>
#include "screen.h"

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

namespace net {
    std::unique_ptr<io_context> io;
    std::unique_ptr<ip::tcp::acceptor> acceptor;
    std::unique_ptr<signal_set> signals;

    std::unique_ptr<Channel<descriptor_data*>> pending_descriptors;

    std::mutex connectionsMutex;

    std::map<int, connection_data*> connections;

    void ProtocolCapabilities::deserialize(const nlohmann::json& j) {

        if(j.contains("encryption")) encryption = j["encryption"];
        if(j.contains("client_name")) clientName = j["client_name"];
        if(j.contains("client_version")) clientVersion = j["client_version"];
		if(j.contains("host_address")) hostAddress = j["host_address"];
        if(j.contains("host_port")) hostPort = j["host_port"];
        if(j.contains("host_names")) for(auto &hn : j["host_names"]) {
            hostNames.emplace_back(hn.get<std::string>());
        }
        if(j.contains("encoding")) encoding = j["encoding"];
        if(j.contains("utf8")) utf8 = j["utf8"];

        if(j.contains("colorType")) colorType = j["color"].get<ColorType>();

        if(j.contains("width")) width = j["width"];
        if(j.contains("height")) height = j["height"];
        if(j.contains("gmcp")) gmcp = j["gmcp"];
        if(j.contains("msdp")) msdp = j["msdp"];
        if(j.contains("mssp")) mssp = j["mssp"];
        if(j.contains("mxp")) mxp = j["mxp"];
        if(j.contains("mccp2")) mccp2 = j["mccp2"];
        if(j.contains("mccp3")) mccp3 = j["mccp3"];
        if(j.contains("ttype")) ttype = j["ttype"];
        if(j.contains("naws")) naws = j["naws"];
        if(j.contains("sga")) sga = j["sga"];
        if(j.contains("linemode")) linemode = j["linemode"];
        if(j.contains("force_endline")) force_endline = j["force_endline"];
        if(j.contains("oob")) oob = j["oob"];
        if(j.contains("tls")) tls = j["tls"];
        if(j.contains("screen_reader")) screen_reader = j["screen_reader"];
        if(j.contains("mouse_tracking")) mouse_tracking = j["mouse_tracking"];
        if(j.contains("vt100")) vt100 = j["vt100"];
        if(j.contains("osc_color_palette")) osc_color_palette = j["osc_color_palette"];
        if(j.contains("proxy")) proxy = j["proxy"];
        if(j.contains("mnes")) mnes = j["mnes"];
    }

    nlohmann::json ProtocolCapabilities::serialize() {
        nlohmann::json j;

        if(encryption) j["encryption"] = encryption;
        if(clientName != "UNKNOWN") j["client_name"] = clientName;
        if(clientVersion != "UNKNOWN") j["client_version"] = clientVersion;
        if(hostAddress != "UNKNOWN") j["host_address"] = hostAddress;
        if(hostPort) j["host_port"] = hostPort;
        if(!hostNames.empty()) {
            for(auto &hn : hostNames) {
                j["host_names"].push_back(hn);
            }
        }
        if(!encoding.empty()) j["encoding"] = encoding;
        if(utf8) j["utf8"] = utf8;
        if(colorType != ColorType::NoColor) j["colorType"] = colorType;
        if(width != 80) j["width"] = width;
        if(height != 52) j["height"] = height;
        if(gmcp) j["gmcp"] = gmcp;
        if(msdp) j["msdp"] = msdp;
        if(mssp) j["mssp"] = mssp;
        if(mxp) j["mxp"] = mxp;
        if(mccp2) j["mccp2"] = mccp2;
        if(mccp3) j["mccp3"] = mccp3;
        if(ttype) j["ttype"] = ttype;
        if(naws) j["naws"] = naws;
        if(sga) j["sga"] = sga;
        if(linemode) j["linemode"] = linemode;
        if(force_endline) j["force_endline"] = force_endline;
        if(oob) j["oob"] = oob;
        if(tls) j["tls"] = tls;
        if(screen_reader) j["screen_reader"] = screen_reader;
        if(mouse_tracking) j["mouse_tracking"] = mouse_tracking;
        if(vt100) j["vt100"] = vt100;
        if(osc_color_palette) j["osc_color_palette"] = osc_color_palette;
        if(proxy) j["proxy"] = proxy;
        if(mnes) j["mnes"] = mnes;

        return j;
    }

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
        boost::asio::ip::tcp::resolver resolver(co_await this_coro::executor);

        // Do asynchronous name resolution for the remote endpoint
        auto results = co_await resolver.async_resolve(conn.socket().remote_endpoint(), use_awaitable);

        auto tel = new telnet_data(std::move(conn));
        tel->inbuf = oldbuf;

        for(auto &r : results) {
            tel->capabilities.hostNames.emplace_back(r.endpoint().address().to_string());
        }

        {
            std::lock_guard<std::mutex> guard(connectionsMutex);
            connections[tel->socket] = tel;
        }

        co_await tel->run();

        {
            std::lock_guard<std::mutex> guard(connectionsMutex);
            connections.erase(tel->socket);
        }

        co_return;
    }

    static awaitable<void> runHttp(boost::beast::tcp_stream conn, const boost::beast::flat_buffer& oldbuf) {
		// TODO: handle HTTP...
        co_return;
    }


    // define a function that takes a milliseconds duration...
    awaitable<void> runTimer(std::chrono::milliseconds duration, bool *timedOut) {
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

    connection_data::connection_data() : inMessages(*io, 200), outMessages(*io, 200) {}

    void connection_data::sendText(const std::string &text) {
        if(text.empty()) return;
        Message msg;
        msg.cmd = "text";
        if(desc && desc->character) {
            msg.args.push_back(processColors(text, COLOR_ON(desc->character), COLOR_CHOICES(desc->character)));
        } else {
            msg.args.push_back(processColors(text, true, nullptr));
        }
        outMessages.try_send(boost::system::error_code{}, msg);
    }

    awaitable<void> connection_data::handleInMessage(const Message &m) {

    }

    awaitable<void> connection_data::runInQueue() {
        while(true) {
            try {
                auto result = co_await inMessages.async_receive(use_awaitable);
                parser->handleMessage(result);
            } catch(boost::system::error_code& ec) {
                if(ec == boost::asio::error::operation_aborted) {
                    break;
                } else {
                    log("net.connection_data::runInQueue: %s", ec.message().c_str());
                }
            }
        }

        co_return;
    }

    nlohmann::json Message::serialize() {
        nlohmann::json j;
        if(!cmd.empty()) j["cmd"] = cmd;
        if(!args.empty()) j["args"] = args;
        if(!kwargs.empty()) j["kwargs"] = kwargs;

        return j;
    }

    Message::Message(const nlohmann::json& j) : Message() {
        if(j.contains("cmd")) cmd = j["cmd"].get<std::string>();
        if(j.contains("args")) args = j["args"];
        if(j.contains("kwargs")) kwargs = j["kwargs"];
    }

    nlohmann::json connection_data::serialize() {
        nlohmann::json j;

        while(outMessages.ready()) {
            Message m;
            if(!outMessages.try_receive(m)) continue;
            j["outMessages"].push_back(m.serialize());
        }

        while(inMessages.ready()) {
            Message m;
            if(!inMessages.try_receive(m)) continue;
            j["inMessages"].push_back(m.serialize());
        }

        j["capabilities"] = capabilities.serialize();

        return j;
    }

    void connection_data::deserialize(const nlohmann::json& j) {
        if(j.contains("outMessages")) {
            for(auto &m : j["outMessages"]) {
                Message msg(m);
                outMessages.try_send(boost::system::error_code{}, msg);
            }
        }

        if(j.contains("inMessages")) {
            for(auto &m : j["inMessages"]) {
                Message msg(m);
                inMessages.try_send(boost::system::error_code{}, msg);
            }
        }

        if(j.contains("capabilities")) {
            capabilities.deserialize(j["capabilities"]);
        }
    }

    void ConnectionParser::handleMessage(const Message &m) {
        if(m.cmd == "text") {
            for(auto &arg : m.args) {
                if(arg.is_string()) {
                    parse(arg.get<std::string>());
                }
            }
        }
    }

	void ConnectionParser::sendText(const std::string &txt) {
        conn->sendText(txt);
    }

    void ConnectionParser::start() {

    }

    void connection_data::setParser(ConnectionParser *p) {
        delete parser;
        parser = p;
        p->start();
    }

}