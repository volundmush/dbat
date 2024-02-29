#include "dbat/net.h"
#include "dbat/utils.h"
#include <regex>
#include "dbat/screen.h"
#include "dbat/players.h"
#include "dbat/login.h"
#include "dbat/config.h"
#include "dbat/account.h"
#include "dbat/accmenu.h"

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

namespace net {
    std::unique_ptr<io_context> io;
    std::unique_ptr<signal_set> signals;
    std::unique_ptr<Channel<nlohmann::json>> linkChannel;
    boost::asio::ip::tcp::endpoint thermiteEndpoint;

    std::mutex connectionMutex, pendingConnectionsMutex;
    std::unordered_map<int64_t, std::shared_ptr<Connection>> connections;
    std::set<int64_t> pendingConnections;
    std::unordered_map<int64_t, DisconnectReason> deadConnections;

    std::unique_ptr<Link> link;

    Link::Link(boost::beast::websocket::stream<boost::beast::tcp_stream> ws)
            : conn(std::move(ws)), is_stopped(false) {}

    awaitable<void> Link::run() {
        try {
            co_await (runReader() || runWriter() || runPinger());
        } catch (const boost::system::system_error& e) {
            logger->error("Error in Link::run(): {}", e.what());
            // Handle exceptions here if necessary
        } catch(...) {
            logger->error("Unknown error in Link::run()");
        }
        if(conn.is_open()) {
            try {
                conn.close(boost::beast::websocket::close_code::normal);
            } catch(...) {
                logger->error("Error closing WebSocket gracefully...");
            }
        }
        co_return;
    }

    awaitable<void> Link::runPinger() {
        boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
        while(!is_stopped) {
            timer.expires_after(100ms);
            co_await timer.async_wait(boost::asio::use_awaitable);
            if(!is_stopped) {
                // use async_ping to send a ping...
                co_await conn.async_ping(boost::beast::websocket::ping_data{}, boost::asio::use_awaitable);
            }
        }
        co_return;
    }

    void Link::stop() {
        is_stopped = true;
    }

    awaitable<void> Link::createUpdateClient(const nlohmann::json &j) {
        auto id = j["id"].get<int64_t>();
        const auto& capabilities = j["capabilities"];

        // Do something with id, addr, and capabilities
        if (auto it = connections.find(id); it == connections.end()) {
            // Create a new ClientConnection
            auto ex = co_await boost::asio::this_coro::executor;
            JsonChannel fLink(ex, 200);
            auto cc = std::make_shared<Connection>(id, std::move(fLink));
            cc->capabilities.deserialize(capabilities);
            if(config::usingMultithreading) {
                std::lock_guard<std::mutex> lock(connectionMutex);
                connections[id] = cc;
            } else connections[id] = cc;

            if(config::usingMultithreading) {
                std::lock_guard<std::mutex> lock(pendingConnectionsMutex);
                pendingConnections.insert(id);
            } else pendingConnections.insert(id);
        } else {
            // Update the existing ClientConnection
            auto& client_connection = it->second;
            client_connection->capabilities.deserialize(capabilities);
        }
        co_return;
    }

    awaitable<void> Link::runReader() {
        while (!is_stopped) {
            try {
                // Read a message from the WebSocket
                boost::beast::flat_buffer buffer;
                co_await conn.async_read(buffer, boost::asio::use_awaitable);

                // Deserialize the JSON string
                auto ws_str = boost::beast::buffers_to_string(buffer.data());
                //std::cout << "Received: " << ws_str << std::endl;
                auto j = nlohmann::json::parse(ws_str);


                // Access the "kind" field in the JSON object
                std::string kind = j["kind"];

                // Implement your routing logic here

                if (kind == "client_list") {
                    // This message is sent by Thermite when the game establishes a fresh connection with it.
                    // It should be the first thing a Link sees.
                    //logger->info("Link: Received client_list message");
                    // Get the "data" object
                    auto &data = j["data"];

                    // Iterate over the contents of the "data" object
                    for (const auto &entry : data) {
                        co_await createUpdateClient(entry);
                    }

                } else if (kind == "client_ready") {
                    // This message is sent by Thermite when a new client has connected.
                    auto &data = j["protocol"];
                    co_await createUpdateClient(data);

                } else {
                    // Extract the "id" field from the JSON object
                    int64_t id = j["id"];

                    // Look up the specific ClientConnection in the std::map
                    auto it = connections.find(id);
                    if(it == connections.end()) {
                        logger->info("Link: Received message for unknown client: {}", id);
                        continue;
                    }

                    // Found the client connection
                    auto &client_connection = it->second;

                    if (kind == "client_capabilities") {
                        auto &capabilities = j["capabilities"];
                        client_connection->capabilities.deserialize(capabilities);

                    } else if (kind == "client_data") {
                        try {
                            co_await client_connection->fromLink.async_send(boost::system::error_code{}, j, boost::asio::use_awaitable);
                        } catch (const boost::system::system_error &e) {
                            // Handle exceptions (e.g., WebSocket close or error)
                        }

                    } else if (kind == "client_disconnected") {
                        logger->info("Link: Received client_disconnected message for client: {}", id);
                        deadConnections[id] = DisconnectReason::ConnectionClosed;
                    }
                }
            } catch (const boost::system::system_error &e) {
                logger->error("Link RunReader flopped at: {}", e.what());
                break;
                // Handle exceptions (e.g., WebSocket close or error)
            } catch (...) {
                logger->error("Unknown error in Link RunReader");
                break;
            }
        }
        co_return;
    }

    awaitable<void> Link::runWriter() {
        while (!is_stopped) {
            try {
                // Receive a message from the channel asynchronously
                auto message = co_await linkChannel->async_receive(boost::asio::use_awaitable);

                try {
                    // Serialize the JSON message to text
                    std::string serialized_msg = message.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);

                    // Send the message across the WebSocket
                    co_await conn.async_write(boost::asio::buffer(serialized_msg), boost::asio::use_awaitable);
                } catch (const boost::system::system_error& e) {
                    logger->error("Link runWriter flopped 1: {}", e.what());
                    // Handle exceptions (e.g., WebSocket close or error) when sending the message
                } catch (...) {
                    logger->error("Unknown error in Link runWriter 1");
                }
            } catch (const boost::system::system_error& e) {
                logger->error("Link runWriter flopped 2: {}", e.what());
                // Handle exceptions (e.g., error receiving the message from the channel)
                break;
            } catch (...) {
                logger->error("Unknown error in Link runWriter 2");
                break;
            }
        }
        co_return;
    }

    void ProtocolCapabilities::deserialize(const nlohmann::json& j) {
        if(j.contains("protocol")) {
            auto s = j["protocol"].get<std::string>();
            if(boost::iequals(s, "Telnet")) protocol = Protocol::Telnet;
            else if(boost::iequals(s, "WebSocket")) protocol = Protocol::WebSocket;
        }
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
        j["client_name"] = clientName;
        j["client_version"] = clientVersion;
        j["host_address"] = hostAddress;
        if(hostPort) j["host_port"] = hostPort;
        for(auto &hn : hostNames) {
            j["host_names"].push_back(hn);
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

    std::string ProtocolCapabilities::protocolName() {
        switch(protocol) {
            case Protocol::Telnet: return "telnet";
            case Protocol::WebSocket: return "websocket";
            default:
                return "unknown";
        }
    }

    Message::Message() {
        cmd = "";
        args = nlohmann::json::array();
        kwargs = nlohmann::json::object();
    }

    Message::Message(const nlohmann::json& j) : Message() {
        if(j.contains("cmd")) cmd = j["cmd"].get<std::string>();
        if(j.contains("args")) args = j["args"];
        if(j.contains("kwargs")) kwargs = j["kwargs"];
    }

    awaitable<void> runLinkManager() {
        bool do_standoff = false;
        boost::asio::steady_timer standoff(co_await boost::asio::this_coro::executor);
        while (true) {
            if(do_standoff) {
                standoff.expires_after(5s);
                co_await standoff.async_wait(boost::asio::use_awaitable);
                do_standoff = false;
            }

            auto &endpoint = thermiteEndpoint;

            try {
                logger->info("LinkManager: Connecting to {}:{}...", endpoint.address().to_string(), endpoint.port());
                // Create a new TCP socket
                boost::beast::websocket::stream<boost::beast::tcp_stream> ws(co_await boost::asio::this_coro::executor);

                // Connect to the endpoint
                co_await boost::beast::get_lowest_layer(ws).async_connect(endpoint, boost::asio::use_awaitable);
                // Initialize a WebSocket using the connected socket

                // Perform the WebSocket handshake
                co_await ws.async_handshake(endpoint.address().to_string() + ":" + std::to_string(endpoint.port()), "/", boost::asio::use_awaitable);

                // Construct a Link using the WebSocket
                link = std::make_unique<Link>(std::move(ws));

                // Run the Link
                logger->info("LinkManager: Link established! Running Link...");
                co_await link->run();
                link.reset();

            } catch (const boost::system::system_error& error) {
                // If there was an error, handle it (e.g., log the error message)
                logger->error("Error in LinkManager::run(): {}", error.what());

                // You might want to add a delay before attempting to reconnect, e.g.,
                do_standoff = true;
            }
        }
        co_return;
    }

    void Connection::sendMessage(const Message &msg) {
        nlohmann::json j, d;
        j["kind"] = "client_data";
        j["id"] = this->connId;

        d["cmd"] = msg.cmd;
        d["args"] = msg.args;
        d["kwargs"] = msg.kwargs;

        j["data"].push_back(d);

        linkChannel->try_send(boost::system::error_code{}, j);
    }


    void ConnectionParser::close() {

    }

    void ConnectionParser::sendText(const std::string &txt) {
        conn->sendText(txt);
    }

    void ConnectionParser::start() {

    }

    void ConnectionParser::handleGMCP(const std::string &txt, const nlohmann::json &j) {

    }

    void Connection::sendGMCP(const std::string &cmd, const nlohmann::json &j) {
        if(cmd.empty()) return;
        nlohmann::json j2;
        j2["cmd"] = cmd;
        j2["data"] = j;
        sendEvent("GMCP", j2);
    }

    void Connection::sendText(const std::string &text) {
        if(text.empty()) return;
        nlohmann::json j;
        j["data"] = text;
        sendEvent("CircleText", j);
    }

    void Connection::sendEvent(const std::string &name, const nlohmann::json &data) {
        outQueue.emplace_back(name, jdump(data));
    }

    void Connection::queueMessage(const std::string& event, const std::string& data) {
        inQueue.emplace_back(event, data);
    }

    void Connection::onWelcome() {
        account = nullptr;
        desc = nullptr;
        setParser(new LoginParser(shared_from_this()));
    }

    void Connection::close() {
        state = ConnectionState::Dead;
    }

    void Connection::onNetworkDisconnected() {
        state = ConnectionState::Dead;
    }


    Connection::Connection(int64_t connId, JsonChannel fromLink) : connId(connId), fromLink(std::move(fromLink)) {

    }

    void Connection::cleanup(DisconnectReason reason) {
        if(account) {
            // Remove ourselves from the account's connections list.
            account->connections.erase(this);
            account = nullptr;
        }
        switch(reason) {
            case DisconnectReason::ConnectionLost:
                // The connection to remote host was lost for unknown reasons.
                // The portal is aware of this and already cleaned them up.
                if(desc) {
                    // If we have a desc then we inform it of the unexpected disconnect.
                    // It can handle this as it sees fit.
                    // onConnectionLost must remove the connection from the descriptor.
                    desc->onConnectionLost(connId);
                    desc = nullptr;
                }
                break;
            case DisconnectReason::ConnectionClosed:
                // The remote socket gracefully/voluntarily closed by the remote host.
                // The portal is aware of this and already cleaned them up.
                if(desc) {
                    // If we have a desc then we can attempt a 'quit' on the
                    // connected character. OR however else it wants to handle it.
                    // onConnectionClosed must remove the connection from the descriptor.
                    desc->onConnectionClosed(connId);
                    desc = nullptr;
                }
                break;
            case DisconnectReason::GameLogoff: {
                // The game server is initiating disconnection for some reason or another.
                // The portal must be informed so it can boot the remote host.
                nlohmann::json j;
                j["kind"] = "client_disconnected";
                j["id"] = connId;
                j["reason"] = "logoff";
                linkChannel->try_send(boost::system::error_code{}, j);
                }
                break;
        }
    }


    void Connection::setParser(ConnectionParser *p) {
        if(parser) parser->close();
        parser.reset(p);
        p->start();
    }

    void Connection::handleEvent(const std::string& event, const nlohmann::json& data) {
        if(event == "Command") {
            executeCommand(data["data"].get<std::string>());
        } else if(event == "GMCP") {
            executeGMCP(data["cmd"].get<std::string>(), data["data"]);
        }
    }


    void Connection::onHeartbeat(double deltaTime) {
        if(!inQueue.empty())
            lastActivity = std::chrono::steady_clock::now();

        for(auto &[name, jdata] : inQueue) {
            nlohmann::json j;
            try {
                j = jparse(jdata);
            }
            catch (const nlohmann::json::parse_error &e) {
                basic_mud_log("Error parsing JSON for event %s: %s", name, e.what());
                continue;
            }

            handleEvent(name, j);
        }
        inQueue.clear();
    }

    void Connection::executeGMCP(const std::string &cmd, const nlohmann::json &j) {
        if(parser) parser->handleGMCP(cmd, j);
    }

    void Connection::executeCommand(const std::string &cmd) {
        if(parser) parser->parse(cmd);
    }

}