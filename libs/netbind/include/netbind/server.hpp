#include <boost/algorithm/string.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
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

using JsonChannel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, nlohmann::json)>;

using ResponseChannel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, NetResponse)>;
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

template <typename T>
struct WebSocketConnection {
    boost::beast::websocket::stream<T>& ws;
    bool isSecure{false};
    std::string hostname, address;
    dbat::web::RouteContext& data;
    int64_t account_id;
    int64_t character_id;
    std::shared_ptr<JsonChannel> send_channel, receive_channel;

    boost::asio::awaitable<void> run() {
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

    WebSocketConnection<T> ws_conn{ws, conn.isSecure, conn.hostname, conn.address, data, sub.value(), 
        character_id.value(), std::make_shared<JsonChannel>(exec, 32), std::make_shared<JsonChannel>(exec, 32)};

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
