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
#include <nlohmann/json.hpp>

#include "netbind/net.hpp"
#include "logging/Log.hpp"
#include "web/web.hpp"

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

using NetRequest = std::variant<UserRegisterReq, UserLoginReq, UserRefreshReq,
                                CharacterListReq, CharacterDeleteReq, CharacterCreateReq>;
using NetResponse = std::variant<HttpAnswer, HttpError>;

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


template<typename T>
struct HttpData {
    Connection<T>& conn;
    HttpRequest&   req;
    HttpResponse&  res;
    Url            url; // by value
};

inline std::expected<nlohmann::json, HttpError> parse_json_body(HttpRequest const& req) {
    return web::parse_json_body(req);
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
