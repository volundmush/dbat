#include "dbat/api/guards.hpp"
#include "volcano/util/Parse.hpp"

namespace dbat::api {
    boost::asio::awaitable<std::optional<HttpAnswer>> require_access_subject(volcano::net::AnyStream &conn, volcano::web::RequestContext& ctx) {
        auto auth = volcano::web::authorize_bearer(ctx.request, jwt);
        if (!auth) {
            co_return auth.error();
        }
        if(!auth->contains("sub") || !auth->at("sub").is_number_integer()) {
            co_return HttpAnswer{http::status::unauthorized, "Access token missing subject\n"};
        }
        ctx.user_data["account_id"] = auth->at("sub").get<int64_t>();
        co_return std::optional<HttpAnswer>();
    }

    boost::asio::awaitable<std::optional<HttpAnswer>> require_json_body(volcano::net::AnyStream &conn, volcano::web::RequestContext& data) {
        auto json_res = volcano::web::parse_json_body(data.request);
        if (!json_res) {
            co_return HttpAnswer{http::status::bad_request, "Invalid JSON body: " + json_res.error() + "\n"};
        }
        data.user_data["json_body"] = json_res.value();
        co_return std::optional<HttpAnswer>();
    }

    boost::asio::awaitable<std::optional<HttpAnswer>> require_username_password(volcano::net::AnyStream &conn, volcano::web::RequestContext& data) {
        if(auto res = co_await require_json_body(conn, data); res) {
            co_return res;
        }
        // we need both a username (string) and password (string) fields...
        auto &j = data.user_data["json_body"];

        if (!j.contains("username") || !j["username"].is_string()) {
            co_return HttpAnswer{http::status::bad_request, "Missing or invalid 'username' field\n"};
        }
        if (!j.contains("password") || !j["password"].is_string()) {
            co_return HttpAnswer{http::status::bad_request, "Missing or invalid 'password' field\n"};
        }
        data.user_data["username"] = j["username"].get<std::string>();
        data.user_data["password"] = j["password"].get<std::string>();
        co_return std::optional<HttpAnswer>();
    }

    boost::asio::awaitable<std::optional<HttpAnswer>> require_character_id(volcano::net::AnyStream &conn, volcano::web::RequestContext& ctx) {
        if(auto res = co_await require_access_subject(conn, ctx); res) {
            co_return res;
        }

        int64_t character_id = -1;
        for (auto const &param : ctx.query)
        {
            if (param.key == "character_id")
            {
                auto result = volcano::util::parseNumber<int64_t>(param.value, "character_id", 0);
                if (!result)
                {
                    co_return HttpAnswer{http::status::bad_request, result.error()};
                }
                character_id = result.value();
                break;
            }
        }
        if (character_id == -1) {
            co_return HttpAnswer{http::status::bad_request, "Missing character_id\n"};
        }
        ctx.user_data["character_id"] = character_id;

        dbat::api::PlayReq play_req;
        play_req.account_id = ctx.user_data["account_id"].get<int64_t>();
        play_req.character_id = character_id;

        // We have to avoid directly touching the database, so we'll use a NetRequest to validate whether this is allowed.
        auto response = co_await send_request_and_reply(ctx, NetRequest{std::move(play_req)});
        if (response.status != http::status::ok)
        {
            co_return response;
        }

        co_return std::optional<HttpAnswer>();
    }

    boost::asio::awaitable<std::optional<HttpAnswer>> require_client_info(volcano::net::AnyStream &conn, volcano::web::RequestContext& ctx) {
        if(auto res = co_await require_character_id(conn, ctx); !res) {
            co_return res;
        }

        nlohmann::json client_data_json;
        bool json_error = false;
        if (ctx.request.find("X-Client-Info") != ctx.request.end())
        {
            auto client_info_b64 = ctx.request["X-Client-Info"];
            try
            {
                auto client_info_str = volcano::jwt::base64UrlDecode(client_info_b64);
                client_data_json = nlohmann::json::parse(client_info_str, nullptr, false);
            }
            catch (const std::exception &e)
            {
                json_error = true;
            }
        }
        if(json_error) {
            co_return HttpAnswer{http::status::bad_request, "Invalid X-Client-Info header\n"};
        }
        ctx.user_data["client_info"] = client_data_json;
        co_return std::optional<HttpAnswer>();
    }
}