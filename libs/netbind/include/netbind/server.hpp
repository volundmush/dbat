#include <boost/algorithm/string.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <expected>
#include <chrono>
#include <functional>
#include <memory>
#include <variant>
#include <charconv>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "netbind/net.hpp"
#include "logging/Log.hpp"
#include "web/web.hpp"
#include "dbat/Account.h"
#include "dbat/Parse.h"

namespace dbat::net {

namespace http = boost::beast::http;

using HttpRequest  = web::HttpRequest;
using HttpResponse = web::HttpResponse;
using Url          = web::Url;
using HttpAnswer   = web::HttpAnswer;
using HttpError    = web::HttpError;

using HttpResult = std::expected<void, HttpError>;

struct UserRegisterReq {
    std::string username;
    std::string password;
};

struct UserLoginReq {
    std::string username;
    std::string password;
};

struct UserRefreshReq {
    std::string refresh_token;
};

struct CharacterListReq {
    int64_t account_id;
};

struct CharacterDeleteReq {
    int64_t account_id;
    int64_t character_id;
};

struct CharacterCreateReq {
    int64_t account_id;
    nlohmann::json character_data;
};

struct PlayReq {
    int64_t account_id;
    int64_t character_id;
};

using NetRequest = std::variant<UserRegisterReq, UserLoginReq, UserRefreshReq,
                                CharacterListReq, CharacterDeleteReq, CharacterCreateReq, PlayReq>;
using NetResponse = std::variant<HttpAnswer, HttpError>;

template <typename T>
using Channel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

using JsonChannel = Channel<nlohmann::json>;
using ResponseChannel = Channel<NetResponse>;

using ResponseChannelPtr = std::shared_ptr<ResponseChannel>;


struct RequestMessage {
    NetRequest            request;
    ResponseChannelPtr    response_channel;

    RequestMessage()
        : response_channel(std::make_shared<ResponseChannel>(context().get_executor(), 1)) {}

    RequestMessage(NetRequest req)
        : request(std::move(req)), response_channel(std::make_shared<ResponseChannel>(context().get_executor(), 1)) {}

    RequestMessage(NetRequest req, ResponseChannel response)
        : request(std::move(req)), response_channel(std::make_shared<ResponseChannel>(std::move(response))) {}

    RequestMessage(NetRequest req, ResponseChannelPtr response)
        : request(std::move(req)), response_channel(std::move(response)) {}
};

using RequestChannel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, RequestMessage)>;

RequestChannel& request_channel();

boost::asio::awaitable<std::size_t> drain_request_channel(std::chrono::steady_clock::duration budget);

std::expected<int64_t, HttpError> require_access_subject(dbat::web::RouteContext& data);
boost::asio::awaitable<dbat::web::Result> send_request_and_reply(dbat::web::RouteContext& data, NetRequest&& request);

struct GameMessageText {
    std::string text;
};

struct GameMessageJson {
    nlohmann::json data;
};

struct GameMessageDisconnect {
    std::string reason;
};

using ToGameMessage = std::variant<GameMessageText, GameMessageJson, GameMessageDisconnect>;

struct GameConnectionInfo {
    bool isSecure{false};
    std::string hostname, address;
    int64_t account_id;
    int64_t character_id;
    Channel<ToGameMessage> to_game, to_websocket;
    int64_t connection_id;
};

Channel<std::shared_ptr<GameConnectionInfo>>& game_connection_channel();
Channel<std::pair<int64_t, std::string>>& game_disconnection_channel();

extern std::unordered_map<int64_t, std::shared_ptr<GameConnectionInfo>> active_game_connections;

boost::asio::awaitable<void> acceptNewConnections();
boost::asio::awaitable<void> handleNewInput();
boost::asio::awaitable<void> handleNewOutput();
boost::asio::awaitable<void> handleDisconnections();

template <typename T>
struct WebSocketConnection {
    boost::beast::websocket::stream<T>& ws;
    dbat::web::RouteContext& data;
    std::shared_ptr<GameConnectionInfo> game_info;

    boost::asio::awaitable<void> handle_read_message(const nlohmann::json& msg) {
        // we need to determine the "type" field.
        if (!msg.contains("type") || !msg["type"].is_string()) {
            LINFO("Received message without valid 'type' field from connection {}.", game_info->connection_id);
            co_return;
        }
        std::string type = msg["type"].get<std::string>();

        boost::system::error_code ec;
        if(boost::iequals(type, "text")) {
            if (!msg.contains("text") || !msg["text"].is_string()) {
                LINFO("Received 'text' message without valid 'text' field from connection {}.", game_info->connection_id);
                co_return;
            }
            std::string text = msg["text"].get<std::string>();
            GameMessageText gmsg{text};
            co_await game_info->to_game.async_send(ec, gmsg, boost::asio::use_awaitable);
            if (ec) {
                LERROR("Failed to send text message to game server from connection {}: {}", game_info->connection_id, ec.message().c_str());
            }
        } else if(boost::iequals(type, "json")) {
            if (!msg.contains("data") || !msg["data"].is_object()) {
                LINFO("Received 'json' message without valid 'data' field from connection {}.", game_info->connection_id);
                co_return;
            }
            nlohmann::json data = msg["data"];
            GameMessageJson gmsg{data};
            co_await game_info->to_game.async_send(ec, gmsg, boost::asio::use_awaitable);
            if (ec) {
                LERROR("Failed to send json message to game server from connection {}: {}", game_info->connection_id, ec.message().c_str());
            }
        } else if(boost::iequals(type, "disconnect")) {
            std::string reason = "Client requested disconnect.";
            if (msg.contains("reason") && msg["reason"].is_string()) {
                reason = msg["reason"].get<std::string>();
            }
            GameMessageDisconnect gmsg{reason};
            co_await game_info->to_game.async_send(ec, gmsg, boost::asio::use_awaitable);
            if (ec) {
                LERROR("Failed to send disconnect message to game server from connection {}: {}", game_info->connection_id, ec.message().c_str());
            }
        } else {
            LINFO("Received unknown message type '{}' from connection {}.", type,  game_info->connection_id);
        }

        co_return;
    }

    boost::asio::awaitable<void> run_reader() {
        boost::beast::flat_buffer buffer;

        boost::system::error_code ec;
        boost::system::error_code ws_ec;
        try {
            while(true) {
                // read a message
                std::size_t n = co_await ws.async_read(buffer, boost::asio::use_awaitable);
                if(ws.text()) {
                    auto data_ptr = buffer.data().data();
                    auto data_str = std::string_view(static_cast<const char*>(data_ptr), n);
                    nlohmann::json msg = nlohmann::json::parse(data_str, nullptr, false);
                    if (msg.is_discarded()) {
                        LINFO("Received invalid JSON message from connection {}.", game_info->connection_id);
                    } else {
                        co_await handle_read_message(msg);
                    }
                } else {
                    LINFO("Received non-text WebSocket message from connection {}. Ignoring.", game_info->connection_id);
                }
                buffer.consume(n);
            }
        } catch(const boost::system::error_code& e) {
            ws_ec = e;
            LINFO("WebSocket closed for connection {}: {}", game_info->connection_id, e.what());
            
        } catch (const std::exception& e) {
            ws_ec = boost::asio::error::operation_aborted;
            LINFO("WebSocket reader error for connection {}: {}", game_info->connection_id, e.what());
        }

        if(ws_ec) {
            co_await game_disconnection_channel().async_send(ec, std::make_pair(game_info->connection_id, ws_ec.message()), boost::asio::use_awaitable);
        }

        co_return;
    }

    boost::asio::awaitable<void> run_writer() {
        ws.text(true);

        try {
            while(true) {
                // Step 1: wait for a message from the game server
                boost::system::error_code ec;
                auto gmsg = co_await game_info->to_websocket.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                if (ec) {
                    LINFO("Game to WebSocket channel closed for connection {}: {}", game_info->connection_id, ec.message().c_str());
                    break;
                }

                // ready outgoing message...
                nlohmann::json jmsg;

                if(std::holds_alternative<GameMessageText>(gmsg)) {
                    auto& msg = std::get<GameMessageText>(gmsg);
                    jmsg["type"] = "text";
                    jmsg["text"] = msg.text;
                } else if(std::holds_alternative<GameMessageJson>(gmsg)) {
                    auto& msg = std::get<GameMessageJson>(gmsg);
                    jmsg["type"] = "json";
                    jmsg["data"] = msg.data;
                } else if(std::holds_alternative<GameMessageDisconnect>(gmsg)) {
                    auto& msg = std::get<GameMessageDisconnect>(gmsg);
                    jmsg["type"] = "disconnect";
                    jmsg["reason"] = msg.reason;
                } else {
                    LERROR("Unknown message type received from game server for connection {}.", game_info->connection_id);
                    continue;
                }
                auto to_send = jmsg.dump();

                // Step 2: send the message over the WebSocket
                boost::system::error_code ws_ec;
                co_await ws.async_write(boost::asio::buffer(to_send), boost::asio::redirect_error(boost::asio::use_awaitable, ws_ec));
                if (ws_ec) {
                    LINFO("WebSocket write error for connection {}: {}", game_info->connection_id, ws_ec.message().c_str());
                    break;
                }

            }
        } catch (const std::exception& e) {
            LINFO("WebSocket writer error for connection {}: {}", game_info->connection_id, e.what());
        }

        co_return;
    }
    
    boost::asio::awaitable<void> run() {
        // first we announce the new connection to the game server...
        boost::system::error_code ec;
        co_await game_connection_channel().async_send(ec, game_info, boost::asio::use_awaitable);
        if (ec) {
            LERROR("Failed to send new game connection info: {}", ec.message().c_str());
            co_return;
        }

        using namespace boost::asio::experimental::awaitable_operators;

        co_await (run_reader() || run_writer());

        co_return;
    }
};

template <typename T>
boost::asio::awaitable<dbat::web::Result> handle_play(Connection<T>& conn, dbat::web::RouteContext& data) {
    auto sub = require_access_subject(data);
    if (!sub) {
        co_return HttpError{http::status::unauthorized, "Unauthorized\n"};
    }

    std::optional<int64_t> character_id;
    for (auto const& param : data.url.params()) {
        if (param.key == "character_id") {
            auto result = parseNumber<int64_t>(param.value, "character_id", 0);
            if (!result) {
                co_return HttpError{http::status::bad_request, result.error()};
            }
            character_id = result.value();
            break;
        }
    }

    if (!character_id.has_value()) {
        co_return HttpError{http::status::bad_request, "Missing character_id\n"};
    }

    dbat::net::PlayReq play_req;
    play_req.account_id = sub.value();
    play_req.character_id = character_id.value();

    // We have to avoid directly touching the database, so we'll use a NetRequest to validate whether this is allowed.
    auto response = co_await send_request_and_reply(data, NetRequest{std::move(play_req)});
    if (!response) {
        co_return response;
    }
    // We got an OK response, so we can proceed with the WebSocket upgrade.

    boost::beast::websocket::stream<T> ws(std::move(conn.connection));
    ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
    ws.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res) {
            res.set(http::field::server, "dbat");
        }
    ));

    try {
        co_await ws.async_accept(data.req, boost::asio::use_awaitable);
    } catch (const std::exception& e) {
        co_return HttpError{http::status::bad_request, std::string("WebSocket upgrade failed: ") + e.what() + "\n"};
    }

    auto exec = co_await boost::asio::this_coro::executor;

    auto gameinfo = std::make_shared<GameConnectionInfo>(conn.isSecure, conn.hostname, conn.address, sub.value(), 
        character_id.value(), std::move(Channel<ToGameMessage>(exec, 32)), std::move(Channel<ToGameMessage>(exec, 32)), -1);

    WebSocketConnection<T> ws_conn{ws, data, gameinfo};

    co_await ws_conn.run();


    // This is included to satisfy the return type; the handler will ignore it.
    co_return HttpAnswer{http::status::switching_protocols, ""};
}

template <typename T>
boost::asio::awaitable<void> handle_http(Connection<T>& conn) {
    boost::beast::flat_buffer buffer;

    for (;;) {
        HttpRequest req;

        boost::system::error_code ec;
        co_await http::async_read(conn.connection, buffer, req,
                                  boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec) {
            LINFO("HTTP read error with {}: {}", conn.address.c_str(), ec.message().c_str());
            co_return;
        }

        HttpResponse res{http::status::ok, req.version()};
        res.set(http::field::server, "dbat");
        res.keep_alive(req.keep_alive());

        auto parsed = boost::urls::parse_origin_form(req.target());
        if (!parsed) {
            res.result(http::status::bad_request);
            res.set(http::field::content_type, "text/plain");
            res.body() = "Invalid URL\n";
        } else {
            dbat::web::RouteContext web_data{req, res, *parsed, {}};

            try {

                if(web_data.url.path() == "/play") {
                    if(!boost::beast::websocket::is_upgrade(req)) {
                        res.result(http::status::bad_request);
                        res.set(http::field::content_type, "text/plain");
                        res.body() = "Expected WebSocket upgrade request\n";
                    } else {
                        auto ws_result = co_await handle_play(conn, web_data);
                        if (!ws_result) {
                            auto& error = ws_result.error();
                            res.result(error.status);
                            res.set(error.content_type_field, error.content_type);
                            res.body() = error.body;
                        } else {
                            co_return;
                        }
                    }
                } else {
                    auto result = co_await global_router.dispatch(web_data);

                    if (!result) {
                        res.result(http::status::not_found);
                        res.set(http::field::content_type, "text/plain");
                        res.body() = "Not Found\n";
                    } else if (result->has_value()) {
                        auto& answer = result->value();
                        res.result(answer.status);
                        res.set(answer.content_type_field, answer.content_type);
                        res.body() = answer.body;
                    } else {
                        auto& error = result->error();
                        res.result(error.status);
                        res.set(error.content_type_field, error.content_type);
                        res.body() = error.body;
                    }
                }

                
            } catch (const std::exception& e) {
                LINFO("Unhandled exception in handler: {}", e.what());
                res.result(http::status::internal_server_error);
                res.set(http::field::content_type, "text/plain");
                res.body() = "Internal Server Error\n";
            }
        }

        res.prepare_payload();

        co_await http::async_write(conn.connection, res,
                                   boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec) {
            LINFO("HTTP write error with {}: {}", conn.address.c_str(), ec.message().c_str());
            co_return;
        }

        if (!req.keep_alive()) co_return;
    }
    co_return;
}

} // namespace dbat::net
