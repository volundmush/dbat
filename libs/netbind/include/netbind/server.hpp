#include <boost/algorithm/string.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <expected>

#include "netbind/net.hpp"
#include "logging/Log.hpp"

namespace dbat::net {

namespace http = boost::beast::http;

using HttpRequest  = http::request<http::string_body>;
using HttpResponse = http::response<http::string_body>;
using Url          = boost::urls::url_view;

struct HttpError {
    http::status status;
    std::string body;
    http::field content_type_field = http::field::content_type;
    std::string content_type = "text/plain";
};

using HttpResult = std::expected<void, HttpError>;

template<typename T>
struct HttpData {
    Connection<T>& conn;
    HttpRequest&   req;
    HttpResponse&  res;
    Url            url; // by value
};

std::optional<std::string_view> bearer_token(HttpRequest const& req) {
    auto h = req[http::field::authorization];
    std::string_view s(h.data(), h.size());
    constexpr std::string_view p = "Bearer ";
    if (s.size() <= p.size()) return std::nullopt;
    if (s.substr(0, p.size()) != p) return std::nullopt;
    return s.substr(p.size());
}

template <typename T>
boost::asio::awaitable<HttpResult> handle_auth_register(HttpData<T>& data) {
    // Handle registration
    co_return std::unexpected(HttpError{http::status::not_implemented, "Registration not implemented\n"});
}

template <typename T>
boost::asio::awaitable<HttpResult> handle_auth_login(HttpData<T>& data) {
    // Handle login
    co_return std::unexpected(HttpError{http::status::not_implemented, "Login not implemented\n"});
}

template <typename T>
boost::asio::awaitable<HttpResult> handle_auth(HttpData<T>& data) {
    if(data.url.path() == "/auth/register") {
        if(data.req.method() != http::verb::post) {
            co_return std::unexpected(HttpError{http::status::method_not_allowed, "Method Not Allowed\n"});
        }
        co_return co_await handle_auth_register(data);
    } else if(data.url.path() == "/auth/login") {
        if(data.req.method() != http::verb::post) {
            co_return std::unexpected(HttpError{http::status::method_not_allowed, "Method Not Allowed\n"});
        }
        co_return co_await handle_auth_login(data);
    }
    co_return std::unexpected(HttpError{http::status::not_found, "Not Found\n"});
}

template <typename T>
boost::asio::awaitable<HttpResult> handle_root(HttpData<T>& data) {
    if(boost::algorithm::starts_with(data.url.path(), "/auth/")) {
        co_return co_await handle_auth(data);
    }

    co_return std::unexpected(HttpError{http::status::not_found, "Not Found\n"});
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
            HttpData<T> data{conn, req, res, *parsed};

            try {
                auto result = co_await handle_root(data);

                if (!result) {
                    auto& r = result.error();
                    res.result(r.status);
                    res.set(r.content_type_field, r.content_type);
                    res.body() = r.body;
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
