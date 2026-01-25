#pragma once
#include "volcano/web/web.hpp"
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include "volcano/jwt/jwt.hpp"
#include "volcano/net/Connection.hpp"

#include <filesystem>
#include <chrono>

namespace dbat::api {
    namespace http = boost::beast::http;
    using HttpRequest  = volcano::web::HttpRequest;
    using HttpResponse = volcano::web::HttpResponse;
    using HttpAnswer   = volcano::web::HttpAnswer;

    extern volcano::jwt::JwtContext jwt;
}