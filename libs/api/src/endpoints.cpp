#include "api/endpoints.hpp"
#include "api/websocket.hpp"
#include "logging/Log.hpp"


namespace dbat::api {

    dbat::web::Router global_router;

    boost::asio::awaitable<dbat::web::Result> handle_auth_register_post(dbat::web::RouteContext& data) {
        // Handle registration
        // first we'll retrieve and parse the JSON body
        auto json_res = web::parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }
        // we need both a username (string) and password (string) fields...
        auto j = json_res.value();

        UserRegisterReq req;

        if (!j.contains("username") || !j["username"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'username' field\n"});
        }
        req.username = j["username"];
        if (!j.contains("password") || !j["password"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'password' field\n"});
        }
        req.password = j["password"];

        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});

    }

    boost::asio::awaitable<dbat::web::Result> handle_auth_login_post(dbat::web::RouteContext& data) {
        auto json_res = web::parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }

        auto j = json_res.value();
        UserLoginReq req;

        if (!j.contains("username") || !j["username"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'username' field\n"});
        }
        req.username = j["username"];

        if (!j.contains("password") || !j["password"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'password' field\n"});
        }
        req.password = j["password"];

        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<dbat::web::Result> handle_auth_refresh_post(dbat::web::RouteContext& data) {
        auto json_res = web::parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }

        auto j = json_res.value();
        UserRefreshReq req;

        if (!j.contains("refresh_token") || !j["refresh_token"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'refresh_token' field\n"});
        }
        req.refresh_token = j["refresh_token"];

        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<dbat::web::Result> handle_character_get(dbat::web::RouteContext& data) {
        auto sub_res = require_access_subject(data);
        if (!sub_res) {
            co_return std::unexpected(sub_res.error());
        }

        CharacterListReq req{sub_res.value()};
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<dbat::web::Result> handle_character_post(dbat::web::RouteContext& data) {
        auto sub_res = require_access_subject(data);
        if (!sub_res) {
            co_return std::unexpected(sub_res.error());
        }

        auto json_res = web::parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }

        CharacterCreateReq req{sub_res.value(), json_res.value()};
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<dbat::web::Result> handle_character_delete(dbat::web::RouteContext& data) {
        auto sub_res = require_access_subject(data);
        if (!sub_res) {
            co_return std::unexpected(sub_res.error());
        }

        auto json_res = web::parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }

        auto j = json_res.value();
        if (!j.contains("character_id") || !j["character_id"].is_number_integer()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'character_id' field\n"});
        }

        CharacterDeleteReq req{sub_res.value(), j["character_id"].get<int64_t>()};
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }


    boost::asio::awaitable<void> handle_http(dbat::net::AnyConnection&& conn) {
        boost::beast::flat_buffer buffer;

        for (;;) {
            HttpRequest req;

            boost::system::error_code ec;
            co_await std::visit(
                [&](auto& stream) -> boost::asio::awaitable<void> {
                    co_await http::async_read(stream, buffer, req,
                        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                },
                conn.connection
            );
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

            co_await std::visit(
                [&](auto& stream) -> boost::asio::awaitable<void> {
                    co_await http::async_write(stream, res,
                        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                },
                conn.connection
            );
            if (ec) {
                LINFO("HTTP write error with {}: {}", conn.address.c_str(), ec.message().c_str());
                co_return;
            }

            if (!req.keep_alive()) co_return;
        }
        co_return;
    }

    void init_router() {
        global_router.add("/auth/register", http::verb::post, handle_auth_register_post);
        global_router.add("/auth/login", http::verb::post, handle_auth_login_post);
        global_router.add("/auth/refresh", http::verb::post, handle_auth_refresh_post);
        global_router.add("/character", http::verb::get, handle_character_get);
        global_router.add("/character", http::verb::post, handle_character_post);
        global_router.add("/character", http::verb::delete_, handle_character_delete);
    }

}