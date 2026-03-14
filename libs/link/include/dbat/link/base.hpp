#pragma once
#include "dbat/web/web.hpp"
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include "dbat/jwt/jwt.hpp"
#include "dbat/net/Connection.hpp"

#include <filesystem>
#include <chrono>

namespace dbat::link {
    namespace http = boost::beast::http;
    using HttpRequest  = dbat::web::HttpRequest;
    using HttpResponse = dbat::web::HttpResponse;
    using HttpAnswer   = dbat::web::HttpAnswer;

    extern dbat::jwt::JwtContext jwt;
}
