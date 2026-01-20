#pragma once
#include "api/channel.hpp"

namespace dbat::api {
    boost::asio::awaitable<void> handle_http(dbat::net::AnyConnection&& conn);
    void init_router();
}