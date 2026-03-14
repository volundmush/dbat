#pragma once
#include "base.hpp"
#include <boost/asio/awaitable.hpp>

namespace dbat::link
{
    boost::asio::awaitable<void> handle_play(dbat::web::WebSocketStream &ws, dbat::web::RequestContext &ctx);
}
