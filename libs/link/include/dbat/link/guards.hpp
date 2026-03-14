#pragma once
#include "channel.hpp"

namespace dbat::link {
    boost::asio::awaitable<std::optional<HttpAnswer>> require_client_info(dbat::net::AnyStream &conn, dbat::web::RequestContext& ctx);
    boost::asio::awaitable<std::optional<HttpAnswer>> require_character_id(dbat::net::AnyStream &conn, dbat::web::RequestContext& ctx);
    boost::asio::awaitable<std::optional<HttpAnswer>> require_access_subject(dbat::net::AnyStream &conn, dbat::web::RequestContext& ctx);
    boost::asio::awaitable<std::optional<HttpAnswer>> require_json_body(dbat::net::AnyStream &conn, dbat::web::RequestContext& data);
    boost::asio::awaitable<std::optional<HttpAnswer>> require_username_password(dbat::net::AnyStream &conn, dbat::web::RequestContext& data);
}
