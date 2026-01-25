#pragma once
#include "base.hpp"
#include <boost/asio/awaitable.hpp>

namespace dbat::api
{
    boost::asio::awaitable<void> handle_play(volcano::web::WebSocketStream &ws, volcano::web::RequestContext &ctx);
}