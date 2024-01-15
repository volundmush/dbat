#include "dbat/net.h"
#include "dbat/utils.h"
#include <regex>
#include "dbat/screen.h"
#include "dbat/players.h"
#include "dbat/login.h"
#include "dbat/config.h"
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

namespace net {
    using namespace boost::asio::experimental::awaitable_operators;
    std::unique_ptr<io_context> io;
    std::unique_ptr<signal_set> signals;

    std::mutex connectionsMutex, pendingConnectionsMutex;


    std::map<int64_t, std::shared_ptr<Connection>> connections;
    std::set<int64_t> pendingConnections;

    std::unordered_map<int64_t, DisconnectReason> deadConnections;

    static std::shared_ptr<spdlog::logger> httpLogger;

    static boost::beast::string_view mime_type(boost::beast::string_view path)
    {
        using boost::beast::iequals;
        auto const ext = [&path]
        {
            auto const pos = path.rfind(".");
            if(pos == boost::beast::string_view::npos)
                return boost::beast::string_view{};
            return path.substr(pos);
        }();
        if(iequals(ext, ".htm"))  return "text/html";
        if(iequals(ext, ".html")) return "text/html";
        if(iequals(ext, ".php"))  return "text/html";
        if(iequals(ext, ".css"))  return "text/css";
        if(iequals(ext, ".txt"))  return "text/plain";
        if(iequals(ext, ".js"))   return "application/javascript";
        if(iequals(ext, ".json")) return "application/json";
        if(iequals(ext, ".xml"))  return "application/xml";
        if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
        if(iequals(ext, ".flv"))  return "video/x-flv";
        if(iequals(ext, ".png"))  return "image/png";
        if(iequals(ext, ".jpe"))  return "image/jpeg";
        if(iequals(ext, ".jpeg")) return "image/jpeg";
        if(iequals(ext, ".jpg"))  return "image/jpeg";
        if(iequals(ext, ".gif"))  return "image/gif";
        if(iequals(ext, ".bmp"))  return "image/bmp";
        if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
        if(iequals(ext, ".tiff")) return "image/tiff";
        if(iequals(ext, ".tif"))  return "image/tiff";
        if(iequals(ext, ".svg"))  return "image/svg+xml";
        if(iequals(ext, ".svgz")) return "image/svg+xml";
        return "application/text";
    }

    HttpConnection::HttpConnection(boost::beast::tcp_stream socket) : socket(std::move(socket)) {

    }

    awaitable<void> HttpConnection::runWebSocket(boost::beast::websocket::stream<boost::beast::tcp_stream> ws,
                                                 std::string_view target) {
        static int64_t nextConnID = 1;

        try {
            inBuf.clear();
            auto results = co_await ws.async_read(inBuf, use_awaitable);
            auto buf = boost::beast::buffers_to_string(inBuf.data());
            inBuf.consume(results);

            if(boost::equals(target, "/ws")) {
                if(!ws.got_text())
                    throw std::runtime_error("Invalid request.");

                auto j = nlohmann::json::parse(buf);
                if(!j.contains("type"))
                    throw std::runtime_error("Invalid request.");

                // this is a ::net::GameMessage.
                auto gmsg = ::net::GameMessage(j);
                if(gmsg.type != GameMessageType::Connect)
                    throw std::runtime_error("Invalid request.");
                // We have a valid GameSession!
                ProtocolCapabilities cap;
                cap.deserialize(gmsg.data);
                // generate a connID...
                while(connections.contains(nextConnID)) nextConnID++;
                auto connID = nextConnID;
                auto ex = co_await boost::asio::this_coro::executor;
                auto conn = std::make_shared<Connection>(std::move(ws), ex, std::move(cap), connID);
                {
                    std::lock_guard<std::mutex> lock(connectionsMutex);
                    connections[connID] = conn;
                }
                {
                    std::lock_guard<std::mutex> lock(pendingConnectionsMutex);
                    pendingConnections.insert(connID);
                }

                co_await conn->run();
            }
            else {
                throw std::runtime_error("Unsupported ws path.");
            }
        }
        catch(const boost::system::error_code &ec) {
            if(ec == boost::asio::error::eof) {
                co_return;
            }
            logger->error("Got unknown error for new websocket {}: {}", target, ec.what());
        }
        catch(const std::exception& err) {
            logger->error("Got invalid request for {}: {}", target, err.what());
        }
        catch(...) {
            logger->error("Unknown exception for WS request {}", target);
        }

        co_return;

    }

    static std::string
    path_cat(
            boost::beast::string_view base,
            boost::beast::string_view path)
    {
        if(base.empty())
            return std::string(path);
        std::string result(base);
#ifdef BOOST_MSVC
        char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
        char constexpr path_separator = '/';
        if(result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
#endif
        return result;
    }

    template <class Body, class Allocator>
    http::message_generator
    handle_request(
            http::request<Body, http::basic_fields<Allocator>>&& req)
    {
        // Returns a bad request response
        auto const bad_request =
                [&req](boost::beast::string_view why)
                {
                    http::response<http::string_body> res{http::status::bad_request, req.version()};
                    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.set(http::field::content_type, "text/html");
                    res.keep_alive(req.keep_alive());
                    res.body() = std::string(why);
                    res.prepare_payload();
                    return res;
                };

        // Returns a not found response
        auto const not_found =
                [&req](boost::beast::string_view target)
                {
                    http::response<http::string_body> res{http::status::not_found, req.version()};
                    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.set(http::field::content_type, "text/html");
                    res.keep_alive(req.keep_alive());
                    res.body() = "The resource '" + std::string(target) + "' was not found.";
                    res.prepare_payload();
                    return res;
                };

        // Returns a server error response
        auto const server_error =
                [&req](boost::beast::string_view what)
                {
                    http::response<http::string_body> res{http::status::internal_server_error, req.version()};
                    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.set(http::field::content_type, "text/html");
                    res.keep_alive(req.keep_alive());
                    res.body() = "An error occurred: '" + std::string(what) + "'";
                    res.prepare_payload();
                    return res;
                };

        // Make sure we can handle the method
        if( req.method() != http::verb::get &&
            req.method() != http::verb::head)
            return bad_request("Unknown HTTP-method");

        // Request path must be absolute and not contain "..".
        if( req.target().empty() ||
            req.target()[0] != '/' ||
            req.target().find("..") != boost::beast::string_view::npos)
            return bad_request("Illegal request-target");

        // Build the path to the requested file
        std::string path = path_cat("webroot", req.target());
        if(req.target().back() == '/')
            path.append("index.html");

        // Attempt to open the file
        boost::beast::error_code ec;
        http::file_body::value_type body;
        body.open(path.c_str(), boost::beast::file_mode::scan, ec);

        // Handle the case where the file doesn't exist
        if(ec == boost::beast::errc::no_such_file_or_directory)
            return not_found(req.target());

        // Handle an unknown error
        if(ec)
            return server_error(ec.message());

        // Cache the size since we need it after the move
        auto const size = body.size();

        // Respond to HEAD request
        if(req.method() == http::verb::head)
        {
            http::response<http::empty_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return res;
        }

        // Respond to GET request
        http::response<http::file_body> res{
                std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(http::status::ok, req.version())};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    awaitable<bool> HttpConnection::runWeb() {
        http::message_generator msg = handle_request(std::move(parser.release()));
        // Determine if we should close the connection
        bool keep_alive = msg.keep_alive();

        // Send the response
        co_await boost::beast::async_write(socket, std::move(msg), use_awaitable);

        co_return keep_alive;
    }

    awaitable<void> HttpConnection::run() {
        while(true) {

            try {
                auto transferred = co_await http::async_read(socket, inBuf, parser, use_awaitable);

                auto req = parser.get();
                auto method = req.method();
                auto target = req.target();

                if(websocket::is_upgrade(req)) {
                    httpLogger->info("Handling WebSocket Upgrade to {}", target);
                    auto ws = boost::beast::websocket::stream<boost::beast::tcp_stream>(std::move(socket));
                    co_await ws.async_accept(parser.release(), use_awaitable);
                    httpLogger->info("WebSocket Handshake Successful to {}", target);
                    co_await runWebSocket(std::move(ws), target);
                } else {
                    auto keep_alive = co_await runWeb();
                    if(!keep_alive) break;
                }

            }
            catch(const boost::system::error_code &ec) {
                httpLogger->error("HttpConnection Exception: {}", ec.what());
                if(!socket.socket().is_open()) co_return;
            }
            catch (const std::exception& e) {
                httpLogger->error("HttpConnection Exception: {}", e.what());
                if(!socket.socket().is_open()) co_return;
            }
            catch(...) {
                httpLogger->error("HttpConnection encountered an unknown exception");
                if(!socket.socket().is_open()) co_return;
            }
        }


        co_return;
    }

    static awaitable<void> handleConnection(boost::beast::tcp_stream stream) {

        auto sess = std::make_shared<HttpConnection>(std::move(stream));
        try {
            co_await sess->run();
        }
        catch(const boost::system::error_code &ec) {
            httpLogger->error("Initial Connection Exception: {}", ec.what());
        }
        catch (const std::exception& e) {
            httpLogger->error("Initial Connection Exception: {}", e.what());
        }
        catch(...) {
            httpLogger->error("Initial Connection encountered an unknown exception");
        }


        co_return;
    }

    awaitable<void> runWebServer() {
        httpLogger = setup_logging("webserver", "logs/webserver.log");
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(config::serverAddress), config::serverPort);

        auto ex = co_await boost::asio::this_coro::executor;
        boost::asio::ip::tcp::acceptor acceptor(ex, ep);

        while(true) {
            auto socket = co_await acceptor.async_accept(use_awaitable);
            httpLogger->info("Connection Established: {}", socket.remote_endpoint().address().to_string());
            auto stream = boost::beast::tcp_stream(std::move(socket));
            co_spawn(make_strand(*io), handleConnection(std::move(stream)), detached);
        }

        co_return;
    }

    void Connection::sendGMCP(const std::string &cmd, const nlohmann::json &j) {
        GameMessage msg;
        msg.type = GameMessageType::GMCP;
        msg.data.push_back(cmd);
        msg.data.push_back(j);

        outChan.try_send(boost::system::error_code{}, msg);
    }

    void Connection::sendText(const std::string &text) {
        if(text.empty()) return;
        GameMessage msg;
        msg.type = GameMessageType::Command;

        if(desc && desc->character) {
            auto &p = players[desc->character->id];
            msg.data = processColors(text, COLOR_ON(desc->character), p.color_choices);
        } else {
            msg.data = processColors(text, true, nullptr);
        }
        outChan.try_send(boost::system::error_code{}, msg);
    }


    void Connection::onWelcome() {
        account = nullptr;
        desc = nullptr;
        setParser(new LoginParser(shared_from_this()));
    }

    void Connection::close() {
        deadConnections[this->connId] = DisconnectReason::GameLogoff;
    }

    void Connection::onNetworkDisconnected() {
        deadConnections[this->connId] = DisconnectReason::ConnectionLost;
    }


    Connection::Connection(boost::beast::websocket::stream<boost::beast::tcp_stream> ws, const any_io_executor& ex, ProtocolCapabilities cap, int64_t connId)
    : connId(connId), capabilities(std::move(cap)), ws(std::move(ws)), inChan(ex, 200), outChan(ex, 200) {

    }

    boost::asio::awaitable<void> Connection::cleanup(DisconnectReason reason) {
        if (account) {
            // Remove ourselves from the account's connections list.
            account->connections.erase(this);
            account = nullptr;
        }
        switch (reason) {
            case DisconnectReason::ConnectionLost:
                // The connection to remote host was lost for unknown reasons.
                // The portal is aware of this and already cleaned them up.
                if (desc) {
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
                if (desc) {
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
                if (ws.is_open()) co_await ws.async_close("quit", use_awaitable);
                break;
            }
                // We do not remove ourselves from net::connections; that's done by the main loop.
        }
        co_return;
    }

    boost::asio::awaitable<void> Connection::onHeartbeat(double deltaTime) {
        std::error_code ec;
        GameMessage msg;
        while (inChan.ready()) {
            if(!inChan.try_receive([&](std::error_code ec2, const GameMessage& value2) {
                ec = ec2;
                msg = value2;
            })) {
                break;
            }
            if (ec) {
                // TODO: Handle any errors..
            } else {
                lastActivity = std::chrono::steady_clock::now();
                handleMessage(msg);
            }
        }
        co_return;
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

    void Connection::setParser(ConnectionParser *p) {
        if(parser) parser->close();
        parser.reset(p);
        p->start();
    }

    boost::asio::awaitable<void> Connection::runWriter() {

            try {
                while (true) {
                    auto message = co_await outChan.async_receive(use_awaitable);
                    auto serialized = message.serialize();
                    auto dumped = serialized.dump(4, ' ', false, nlohmann::json::error_handler_t::ignore);
                    co_await ws.async_write(boost::asio::buffer(dumped), use_awaitable);
                }
            }
            catch(const boost::system::error_code &ec) {
                if(ec != boost::asio::error::operation_aborted) logger->error("Connection runWriter Exception: {}", ec.what());
            }
            catch (const std::exception& e) {
                logger->error("Connection runWriter Exception: {}", e.what());
            }
            catch(...) {
                logger->error("Connection runWriter encountered an unknown exception");
            }

        co_return;
    }

    void Connection::handleMessage(const GameMessage &msg) {
        switch(msg.type) {
            case GameMessageType::Update:
                capabilities.deserialize(msg.data);
                break;
            case GameMessageType::Command: {
                auto cmd = msg.data.get<std::string>();
                if(parser) parser->parse(cmd);
            }
                break;
            case GameMessageType::GMCP:
                if(msg.data.is_array() && msg.data.size() == 2) {
                    auto cmd = msg.data[0].get<std::string>();
                    auto data = msg.data[1];
                    if(parser) parser->handleGMCP(cmd, data);
                }
                break;
            case GameMessageType::Disconnect:
                break;
            case GameMessageType::Timeout:
                break;
        }

    }

    boost::asio::awaitable<void> Connection::runReader() {
        bool error = false;
        while(true) {
            try {
                auto results = co_await ws.async_read(inBuf, use_awaitable);
                if(!ws.got_text()) continue;
                auto buf_to_string = boost::beast::buffers_to_string(inBuf.data());
                inBuf.clear();
                auto j = nlohmann::json::parse(buf_to_string);
                if(j.contains("type")) {
                    // this is a ::net::GameMessage.
                    auto gmsg = ::net::GameMessage(j);
                    co_await inChan.async_send(boost::system::error_code{}, gmsg, use_awaitable);
                }
            }
            catch(const boost::system::error_code &ec) {
                if(ec == boost::asio::error::eof) {
                    // End of file or connection closed by peer
                    logger->info("Connection closed by peer");
                } else if(ec == boost::asio::error::operation_aborted) {
                    // Operation cancelled, possibly due to timeout
                    logger->info("Operation aborted, possibly due to timeout");
                } else {
                    // Other errors
                    error = true;
                    logger->error("Connection runReader Exception: {}", ec.message());
                }
                break;
            }
            catch (const std::exception& e) {
                error = true;
                logger->error("Connection runReader Exception: {}", e.what());
                break;
            }
            catch(...) {
                error = true;
                logger->error("Connection runReader encountered an unknown exception");
                break;
            }
        }
        if(error) {
            if(!deadConnections.contains(connId)) deadConnections[connId] = DisconnectReason::ConnectionLost;
            if(ws.is_open()) co_await ws.async_close(boost::beast::websocket::close_reason("runReader() error"), use_awaitable);
        }

        co_return;
    }

    boost::asio::awaitable<void> Connection::runPinger() {
        auto ex = co_await this_coro::executor;
        boost::asio::steady_timer timer(ex, std::chrono::milliseconds(100));
        try {
            while(true) {
                co_await timer.async_wait(use_awaitable);
                if(ws.is_open()) {
                    co_await ws.async_ping({}, use_awaitable);
                }
                timer.expires_after(std::chrono::milliseconds(100));
            }
        }
        catch(const boost::system::error_code &ec) {
            if(ec != boost::asio::error::operation_aborted) {
                logger->error("runPinger got an unknown error_code: {}", ec.what());
            }
        }
        catch(const std::exception& err) {
            logger->error("runPinger got an exception: {}", err.what());
        }
        catch(...) {
            logger->error("runPinger got an unknown exception");
        }


        co_return;
    }


    boost::asio::awaitable<void> Connection::run() {
        try {
            co_await (runPinger() || runReader() || runWriter());
        }
        catch(const boost::system::error_code &ec) {
            logger->error("Connection runReader Exception: {}", ec.what());
        }
        catch (const std::exception& e) {
            logger->error("Connection runReader Exception: {}", e.what());
        }
        catch(...) {
            logger->error("Connection runReader encountered an unknown exception");
        }
        if(ws.is_open()) co_await ws.async_close(boost::beast::websocket::close_reason("Incomplete Close"), use_awaitable);

        co_return;
    }

}