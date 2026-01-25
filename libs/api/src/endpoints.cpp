#include "dbat/api/endpoints.hpp"
#include "dbat/api/websocket.hpp"
#include "volcano/log/Log.hpp"
#include "dbat/api/guards.hpp"

namespace dbat::api {

    boost::asio::awaitable<volcano::web::HttpAnswer> handle_auth_register_post(volcano::net::AnyStream &conn, volcano::web::RequestContext& data) {
        UserRegisterReq req;
        req.username = data.user_data["username"].get<std::string>();
        req.password = data.user_data["password"].get<std::string>();
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<volcano::web::HttpAnswer> handle_auth_login_post(volcano::net::AnyStream &conn, volcano::web::RequestContext& data) {
        UserLoginReq req;
        req.username = data.user_data["username"].get<std::string>();
        req.password = data.user_data["password"].get<std::string>();
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<volcano::web::HttpAnswer> handle_auth_refresh_post(volcano::net::AnyStream &conn, volcano::web::RequestContext& data) {
        if(auto res = co_await require_json_body(conn, data); res) {
            co_return res.value();
        }
        // we need both a username (string) and password (string) fields...
        auto &j = data.user_data["json_body"];

        UserRefreshReq req;

        if (!j.contains("refresh_token") || !j["refresh_token"].is_string()) {
            co_return HttpAnswer{http::status::bad_request, "Missing or invalid 'refresh_token' field\n"};
        }
        req.refresh_token = j["refresh_token"];

        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<volcano::web::HttpAnswer> handle_character_get(volcano::net::AnyStream &conn, volcano::web::RequestContext& data) {
        auto sub = data.user_data["account_id"].get<int64_t>();

        CharacterListReq req{sub};
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<volcano::web::HttpAnswer> handle_character_post(volcano::net::AnyStream &conn, volcano::web::RequestContext& data) {
        auto sub = data.user_data["account_id"].get<int64_t>();

        auto json_res = volcano::web::parse_json_body(data.request);
        if (!json_res) {
            co_return HttpAnswer{http::status::bad_request, "Invalid JSON body: " + json_res.error() + "\n"};
        }

        CharacterCreateReq req{sub, json_res.value()};
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<volcano::web::HttpAnswer> handle_character_delete(volcano::net::AnyStream &conn, volcano::web::RequestContext& data) {
        auto sub = data.user_data["account_id"].get<int64_t>();

        auto json_res = volcano::web::parse_json_body(data.request);
        if (!json_res) {
            co_return HttpAnswer{http::status::bad_request, "Invalid JSON body: " + json_res.error() + "\n"};
        }

        auto j = json_res.value();
        if (!j.contains("character_id") || !j["character_id"].is_number_integer()) {
            co_return HttpAnswer{http::status::bad_request, "Missing or invalid 'character_id' field\n"};
        }

        CharacterDeleteReq req{sub, j["character_id"].get<int64_t>()};
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    std::shared_ptr<volcano::web::Router> init_router() {

        auto router = std::make_shared<volcano::web::Router>();
        auto &v1 = router->add_router("v1");
        auto &v1auth = v1.add_router("auth");
        v1auth.add_request_handler("register", http::verb::post, require_username_password, handle_auth_register_post);
        v1auth.add_request_handler("login", http::verb::post, require_username_password, handle_auth_login_post);
        v1auth.add_request_handler("refresh", http::verb::post, handle_auth_refresh_post);
        auto &v1character = v1.add_router("character");
        v1character.add_request_handler("", http::verb::get, require_access_subject, handle_character_get);
        v1character.add_request_handler("", http::verb::post, require_access_subject, handle_character_post);
        v1character.add_request_handler("", http::verb::delete_, require_access_subject, handle_character_delete);

        v1.add_websocket_handler("play", require_access_subject, handle_play);
    
        return router;
    }

}