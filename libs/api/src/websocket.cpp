#include "api/websocket.hpp"
#include "dbat/Parse.h"

namespace dbat::api
{
    boost::asio::awaitable<dbat::web::Result> handle_play(dbat::net::AnyConnection &conn, dbat::web::RouteContext &data) {
        auto sub = require_access_subject(data);
        if (!sub)
        {
            co_return std::unexpected(HttpError{http::status::unauthorized, "Unauthorized\n"});
        }

        std::optional<int64_t> character_id;
        for (auto const &param : data.url.params())
        {
            if (param.key == "character_id")
            {
                auto result = parseNumber<int64_t>(param.value, "character_id", 0);
                if (!result)
                {
                    co_return std::unexpected(HttpError{http::status::bad_request, result.error()});
                }
                character_id = result.value();
                break;
            }
        }

        if (!character_id.has_value())
        {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing character_id\n"});
        }

        dbat::api::PlayReq play_req;
        play_req.account_id = sub.value();
        play_req.character_id = character_id.value();

        // We have to avoid directly touching the database, so we'll use a NetRequest to validate whether this is allowed.
        auto response = co_await send_request_and_reply(data, NetRequest{std::move(play_req)});
        if (!response)
        {
            co_return response;
        }
        // We got an OK response, so we can proceed with the WebSocket upgrade.

        // let's use std::visit to handle both secure and non-secure connections
        co_return co_await std::visit(
            [&](auto &specific_conn) -> boost::asio::awaitable<dbat::web::Result> {
                using ConnType = std::decay_t<decltype(specific_conn)>;
                boost::beast::websocket::stream<ConnType> ws{std::move(specific_conn)};
                co_return co_await handle_play_helper(conn, data, std::move(ws), sub.value(), character_id.value());
            },
            conn.connection);
    }
}