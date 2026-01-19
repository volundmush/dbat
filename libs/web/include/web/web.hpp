#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <nlohmann/json.hpp>

#include "jwt/jwt.hpp"

namespace dbat::web {
    namespace http = boost::beast::http;

    using HttpRequest  = http::request<http::string_body>;
    using HttpResponse = http::response<http::string_body>;
    using Url          = boost::urls::url_view;

    struct HttpAnswer {
      http::status status;
      std::string body;
      http::field content_type_field = http::field::content_type;
      std::string content_type = "text/plain";
    };

    struct HttpError : public HttpAnswer {
    };

    using Result = std::expected<HttpAnswer, HttpError>;

    using Params = std::unordered_map<std::string, std::string>;

    struct RouteContext {
        HttpRequest& req;
        HttpResponse& res;
        Url url;
        Params params;
    };

    using Handler = std::function<boost::asio::awaitable<Result>(RouteContext&)>;

    inline std::optional<std::string_view> bearer_token(HttpRequest const& req) {
      auto h = req[http::field::authorization];
      std::string_view s(h.data(), h.size());
      constexpr std::string_view p = "Bearer ";
      if (s.size() <= p.size()) return std::nullopt;
      if (s.substr(0, p.size()) != p) return std::nullopt;
      return s.substr(p.size());
    }

    inline std::expected<nlohmann::json, HttpError> verify_jwt(HttpRequest const& req, std::string_view secret) {
      auto token_opt = bearer_token(req);
      if (!token_opt.has_value()) {
        return std::unexpected(HttpError{http::status::unauthorized, "Missing or invalid Authorization header\n"});
      }
      auto token = token_opt.value();
      auto ver_res = dbat::jwt::verify(token, secret);
      if (!ver_res) {
        return std::unexpected(HttpError{http::status::unauthorized, "Invalid token: " + ver_res.error() + "\n"});
      }
      return ver_res.value();
    }

    inline std::expected<nlohmann::json, HttpError> valid_jwt(HttpRequest const& req, std::string_view secret) {
      auto ver_res = verify_jwt(req, secret);
      if (!ver_res) {
        return std::unexpected(ver_res.error());
      }
      auto payload = ver_res.value();
      const auto now = std::chrono::system_clock::now();
      const auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

      if (payload.contains("exp") && payload["exp"].is_number_integer()) {
        const auto exp_seconds = payload["exp"].get<long long>();
        if (now_seconds >= exp_seconds) {
          return std::unexpected(HttpError{http::status::unauthorized, "Token expired\n"});
        }
      }

      return payload;
    }

    inline std::expected<nlohmann::json, HttpError> parse_json_body(HttpRequest const& req) {
      try {
        if(req[http::field::content_type] != "application/json") {
          return std::unexpected(HttpError{http::status::unsupported_media_type, "Content-Type must be application/json\n"});
        }
        auto j = nlohmann::json::parse(req.body());
        return j;
      } catch (const std::exception& e) {
        return std::unexpected(HttpError{http::status::bad_request, "Invalid JSON body: " + std::string(e.what()) + "\n"});
      }
    }

    class Router {
      public:
        Router() = default;

        void add(std::string_view path, http::verb method, Handler handler);

        boost::asio::awaitable<std::optional<Result>> dispatch(RouteContext& ctx) const;

      private:
        struct Node {
            std::unordered_map<std::string, std::unique_ptr<Node>> static_children;
            std::unique_ptr<Node> param_child;
            std::string param_name;
            std::unordered_map<http::verb, Handler> handlers;
        };

        Node root_{};
    };
} // namespace dbat::web
