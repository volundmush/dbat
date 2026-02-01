#pragma once
#include "channel.hpp"

namespace dbat::api {
    boost::asio::awaitable<std::optional<HttpAnswer>> require_client_info(volcano::net::AnyStream &conn, volcano::web::RequestContext& ctx);
    boost::asio::awaitable<std::optional<HttpAnswer>> require_character_id(volcano::net::AnyStream &conn, volcano::web::RequestContext& ctx);
    boost::asio::awaitable<std::optional<HttpAnswer>> require_access_subject(volcano::net::AnyStream &conn, volcano::web::RequestContext& ctx);
    boost::asio::awaitable<std::optional<HttpAnswer>> require_json_body(volcano::net::AnyStream &conn, volcano::web::RequestContext& data);
    boost::asio::awaitable<std::optional<HttpAnswer>> require_username_password(volcano::net::AnyStream &conn, volcano::web::RequestContext& data);
}