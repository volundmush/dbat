#pragma once
#include "web/web.hpp"
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

#include <filesystem>
#include <chrono>

namespace dbat::api {
    namespace http = boost::beast::http;
    using HttpRequest  = web::HttpRequest;
    using HttpResponse = web::HttpResponse;
    using Url          = web::Url;
    using HttpAnswer   = web::HttpAnswer;
    using HttpError    = web::HttpError;
    using HttpResult   = web::Result;
}