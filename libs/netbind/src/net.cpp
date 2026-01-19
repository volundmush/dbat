#include "netbind/server.hpp"

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string_view>

#include <nlohmann/json.hpp>

#include "valid/valid.hpp"
#include "dbat/Account.h"
#include "dbat/players.h"
#include "dbat/Character.h"

#include "serde/saveload.h"

namespace dbat::net {

    TlsConfig tls_config;
    Config config;
    JwtConfig jwt_config;

    namespace {
        std::int64_t now_seconds() {
            return static_cast<std::int64_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        }

        nlohmann::json base_claims(const Account& account) {
            nlohmann::json claims;
            claims["sub"] = account.id;
            claims["username"] = account.name;
            claims["iat"] = now_seconds();
            claims["iss"] = jwt_config.issuer;
            claims["aud"] = jwt_config.audience;
            return claims;
        }

        std::string create_access_token(const Account& account) {
            auto claims = base_claims(account);
            claims["token_use"] = "access";
            return dbat::jwt::create(claims, jwt_config.secret, jwt_config.token_expiry);
        }

        std::string create_refresh_token(const Account& account) {
            auto claims = base_claims(account);
            claims["token_use"] = "refresh";
            return dbat::jwt::create(claims, jwt_config.secret, jwt_config.refresh_token_expiry);
        }

        nlohmann::json build_token_response(const Account& account, std::string_view access_token, std::string_view refresh_token) {
            nlohmann::json response_json;
            response_json["token_type"] = "Bearer";
            response_json["access_token"] = access_token;
            response_json["refresh_token"] = refresh_token;
            response_json["expires_in"] = static_cast<int>(jwt_config.token_expiry.count());
            response_json["username"] = account.name;
            response_json["user_id"] = account.id;
            return response_json;
        }

        std::expected<int64_t, HttpError> require_access_subject(dbat::web::RouteContext& data) {
            if (jwt_config.secret.empty()) {
                return std::unexpected(HttpError{http::status::internal_server_error, "JWT secret not configured\n"});
            }

            auto payload_res = web::valid_jwt(data.req, jwt_config.secret);
            if (!payload_res) {
                return std::unexpected(payload_res.error());
            }

            const auto& payload = payload_res.value();
            if (payload.contains("token_use") && payload["token_use"].is_string()) {
                if (payload["token_use"] != "access") {
                    return std::unexpected(HttpError{http::status::unauthorized, "Invalid access token\n"});
                }
            }

            if (!payload.contains("sub") || !payload["sub"].is_number_integer()) {
                return std::unexpected(HttpError{http::status::unauthorized, "Invalid token subject\n"});
            }

            return payload["sub"].get<int64_t>();
        }
    }

    RequestChannel& request_channel() {
        static RequestChannel channel(context().get_executor(), 1024);
        return channel;
    }

    

    boost::asio::awaitable<NetResponse> handle_request(RequestMessage& msg) {

        if(std::holds_alternative<UserRegisterReq>(msg.request)) {
            auto& req = std::get<UserRegisterReq>(msg.request);
            // Handle user registration
            auto result = valid::username(req.username);
            if (!result) {
                co_return HttpError{http::status::bad_request, result.error()};
            }
            // For now, just return a not implemented error
            if(auto account = createAccount(req.username, req.password); !account) {
                co_return HttpError{http::status::bad_request, account.error()};
            } else {
                HttpAnswer answer;
                answer.status = http::status::created;
                answer.content_type = "application/json";
                nlohmann::json response_json;
                response_json["message"] = "User registered successfully";
                response_json["username"] = account.value()->name;
                answer.body = response_json.dump();
                co_return answer;
            }
        } else if(std::holds_alternative<UserLoginReq>(msg.request)) {
            auto& req = std::get<UserLoginReq>(msg.request);
            // Handle user login
            // For now, just return a not implemented error
            
            auto result = valid::username(req.username);
            if (!result) {
                co_return HttpError{http::status::bad_request, result.error()};
            }

            if(auto account = findAccount(*result); !account) {
                co_return HttpError{http::status::unauthorized, "Invalid username or password\n"};
            } else {
                if(!account->check_password(req.password)) {
                    co_return HttpError{http::status::unauthorized, "Invalid username or password\n"};
                } else {
                    HttpAnswer answer;
                    answer.status = http::status::ok;
                    answer.content_type = "application/json";
                    if (jwt_config.secret.empty()) {
                        co_return HttpError{http::status::internal_server_error, "JWT secret not configured\n"};
                    }

                    const auto access_token = create_access_token(*account);
                    const auto refresh_token = create_refresh_token(*account);
                    answer.body = build_token_response(*account, access_token, refresh_token).dump();
                    co_return answer;
                }
            }
        } else if(std::holds_alternative<UserRefreshReq>(msg.request)) {
            auto& req = std::get<UserRefreshReq>(msg.request);

            auto verify_res = dbat::jwt::verify(req.refresh_token, jwt_config.secret);
            if (!verify_res) {
                co_return HttpError{http::status::unauthorized, "Invalid refresh token: " + verify_res.error() + "\n"};
            }

            const auto& payload = verify_res.value();
            if (!payload.contains("token_use") || !payload["token_use"].is_string() || payload["token_use"] != "refresh") {
                co_return HttpError{http::status::unauthorized, "Invalid refresh token type\n"};
            }

            if (payload.contains("iss") && payload["iss"].is_string()) {
                if (payload["iss"].get<std::string>() != jwt_config.issuer) {
                    co_return HttpError{http::status::unauthorized, "Invalid refresh token issuer\n"};
                }
            }

            if (payload.contains("aud") && payload["aud"].is_string()) {
                if (payload["aud"].get<std::string>() != jwt_config.audience) {
                    co_return HttpError{http::status::unauthorized, "Invalid refresh token audience\n"};
                }
            }

            if (!payload.contains("username") || !payload["username"].is_string()) {
                co_return HttpError{http::status::unauthorized, "Invalid refresh token payload\n"};
            }

            const auto sub = payload["sub"].get<int64_t>();
            auto find = accounts.find(sub);
            if (find == accounts.end()) {
                co_return HttpError{http::status::unauthorized, "Account not found\n"};
            }
            auto account = find->second;

            if (payload.contains("sub") && payload["sub"].is_number_integer()) {
                const auto sub = payload["sub"].get<std::int64_t>();
                if (sub != account->id) {
                    co_return HttpError{http::status::unauthorized, "Invalid refresh token subject\n"};
                }
            }

            HttpAnswer answer;
            answer.status = http::status::ok;
            answer.content_type = "application/json";
            const auto access_token = create_access_token(*account);
            const auto refresh_token = create_refresh_token(*account);
            answer.body = build_token_response(*account, access_token, refresh_token).dump();
            co_return answer;
        } else if (std::holds_alternative<CharacterListReq>(msg.request)) {
            auto& req = std::get<CharacterListReq>(msg.request);

            auto account_it = accounts.find(req.account_id);
            if (account_it == accounts.end()) {
                co_return HttpError{http::status::unauthorized, "Account not found\n"};
            }

            auto& account = account_it->second;
            nlohmann::json list = nlohmann::json::array();
            for (auto id : account->characters) {
                const char* name = get_name_by_id(static_cast<long>(id));
                if(!name) continue;
                nlohmann::json entry;
                entry["id"] = id;
                entry["name"] = name ? name : "";
                list.push_back(std::move(entry));
            }

            HttpAnswer answer;
            answer.status = http::status::ok;
            answer.content_type = "application/json";
            nlohmann::json response_json;
            response_json["characters"] = std::move(list);
            answer.body = response_json.dump();
            co_return answer;
        } else if (std::holds_alternative<CharacterDeleteReq>(msg.request)) {
            auto& req = std::get<CharacterDeleteReq>(msg.request);

            auto account_it = accounts.find(req.account_id);
            if (account_it == accounts.end()) {
                co_return HttpError{http::status::unauthorized, "Account not found\n"};
            }

            auto& account = account_it->second;
            const bool owns = std::find(account->characters.begin(), account->characters.end(), req.character_id) != account->characters.end();
            if (!owns) {
                co_return HttpError{http::status::not_found, "Character not found\n"};
            }

            auto it = Character::registry.find(req.character_id);
            if (it == Character::registry.end()) {
                co_return HttpError{http::status::not_found, "Character not loaded\n"};
            }

            auto ch = it->second;
            if (!canDeleteCharacter(ch)) {
                co_return HttpError{http::status::conflict, "Character cannot be deleted right now\n"};
            }

            deletePlayerCharacter(ch);

            HttpAnswer answer;
            answer.status = http::status::ok;
            answer.content_type = "application/json";
            nlohmann::json response_json;
            response_json["deleted"] = true;
            response_json["character_id"] = req.character_id;
            answer.body = response_json.dump();
            co_return answer;
        } else if (std::holds_alternative<CharacterCreateReq>(msg.request)) {
            auto& req = std::get<CharacterCreateReq>(msg.request);

            auto account_it = accounts.find(req.account_id);
            if (account_it == accounts.end()) {
                co_return HttpError{http::status::unauthorized, "Account not found\n"};
            }

            ChargenData chargen;
            try {
                req.character_data.get_to(chargen);
            } catch (const std::exception& e) {
                co_return HttpError{http::status::bad_request, "Invalid character data: " + std::string(e.what()) + "\n"};
            }

            auto result = createPlayerCharacter(account_it->second.get(), chargen);
            if (!result) {
                co_return HttpError{http::status::bad_request, result.error()};
            }

            auto character = result.value();

            HttpAnswer answer;
            answer.status = http::status::created;
            answer.content_type = "application/json";
            nlohmann::json response_json;
            response_json["character_id"] = character->id;
            response_json["character_name"] = character->name;
            answer.body = response_json.dump();
            co_return answer;
        }

        co_return HttpError{http::status::not_implemented, "Request handling not implemented\n"};
    }

    boost::asio::awaitable<std::size_t> drain_request_channel(std::chrono::steady_clock::duration budget) {
        using clock = std::chrono::steady_clock;
        const auto start = clock::now();
        std::size_t processed = 0;

        auto &chan = request_channel();

        for (;;) {
            RequestMessage msg;

            if(!chan.ready()) {
                co_return processed;
            }

            try {
                msg = co_await chan.async_receive(boost::asio::use_awaitable);
            } catch (const boost::system::system_error& e) {
                LERROR("Request channel receive error: {}", e.code().message().c_str());
                co_return processed;
            }

            auto response = co_await handle_request(msg);

            boost::system::error_code send_ec;
            co_await msg.response_channel->async_send(
                send_ec,
                std::move(response),
                boost::asio::use_awaitable
            );

            if (send_ec) {
                LERROR("Request response send error: {}", send_ec.message().c_str());
            }

            ++processed;

            if (clock::now() - start >= budget) {
                co_return processed;
            }
        }
    }

    boost::asio::io_context& context() {
        static boost::asio::io_context ioc;
        return ioc;
    }

    std::expected<boost::asio::ip::address, boost::system::error_code> parse_address(std::string_view addr_str) {
        if(boost::iequals(addr_str, "any") || boost::iequals(addr_str, "*")) {
            return boost::asio::ip::address_v6::any();
        }
        try {
            auto address = boost::asio::ip::make_address(std::string(addr_str));
            return address;
        } catch(const std::exception& e) {
            boost::system::error_code ec = boost::asio::error::invalid_argument;
            return std::unexpected(ec);
        }
    }

    static boost::asio::awaitable<std::expected<std::string, boost::system::error_code>> reverse_lookup(boost::asio::ip::tcp::socket& socket) {
        boost::system::error_code ec;
        auto remote_endpoint = socket.remote_endpoint(ec);
        if(ec) {
            co_return std::unexpected(ec);
        }
        auto remote_address = remote_endpoint.address();
        boost::asio::ip::tcp::resolver resolver(co_await boost::asio::this_coro::executor);
        try {
            auto results = co_await resolver.async_resolve(
            remote_address.to_string(),
            "",
            boost::asio::ip::tcp::resolver::flags::numeric_service,
            boost::asio::use_awaitable
            );
            if(results.empty()) {
                co_return std::unexpected(boost::asio::error::host_not_found);
            }
            co_return results.begin()->host_name();
        } catch(const boost::system::system_error& e) {
            co_return std::unexpected(e.code());
        }
    }

    static boost::asio::awaitable<void> handle_client(boost::asio::ip::tcp::socket socket, std::shared_ptr<boost::asio::ssl::context> tls_context) {
        try {
            auto client_address = socket.remote_endpoint().address().to_string();
            auto client_hostname = client_address;
            LINFO("Incoming connection from %s", client_address.c_str());
            if(auto rev_res = co_await reverse_lookup(socket); rev_res) {
                client_hostname = rev_res.value();
                LINFO("Resolved hostname %s for %s", client_hostname.c_str(), client_address.c_str());
            } else {
                LINFO("Could not resolve hostname for %s: %s", client_address.c_str(), rev_res.error().message().c_str());
            }
            
            if(tls_context) {
                auto ssl_socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(std::move(socket), *tls_context);
                boost::system::error_code ec;
                co_await ssl_socket.async_handshake(boost::asio::ssl::stream_base::server, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                if(ec) {
                    LERROR("TLS handshake failed with %s: %s", client_hostname.c_str(), ec.message().c_str());
                    co_return;
                }
                LINFO("Completed TLS handshake with %s", client_hostname.c_str());
                auto con = Connection{std::move(ssl_socket), true, client_hostname, client_address};
                co_await handle_http(con);
            } else {
                auto con = Connection{std::move(socket), false, client_hostname, client_address};
                co_await handle_http(con);
            }
        }
        catch(const std::exception& e) {
            LERROR("Error handling client: %s", e.what());
        }
    }

    static boost::asio::awaitable<void> run_server(boost::asio::ip::tcp::acceptor acceptor, std::shared_ptr<boost::asio::ssl::context> tls_context) {
        for(;;) {
            boost::system::error_code ec;
            auto socket = co_await acceptor.async_accept(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
            if(ec) {
                LERROR("Accept error: %s", ec.message().c_str());
                continue;
            }
            auto strand = boost::asio::make_strand(context());
            boost::asio::co_spawn(strand,
                handle_client(std::move(socket), tls_context),
                boost::asio::detached
            );
        }
    }

    void bind_server(boost::asio::ip::address address, uint16_t port, std::shared_ptr<boost::asio::ssl::context> tls_context) {
        // let's create an acceptor.
        boost::asio::ip::tcp::endpoint endpoint(address, port);
        // create a stranded acceptor
        auto strand = boost::asio::make_strand(context());
        boost::asio::ip::tcp::acceptor acceptor(strand, endpoint, true);
        boost::asio::co_spawn(strand,
            run_server(std::move(acceptor), tls_context),
            boost::asio::detached
        );
    }

    dbat::web::Router global_router;

    boost::asio::awaitable<dbat::web::Result> send_request_and_reply(dbat::web::RouteContext& data, NetRequest&& request) {
        RequestMessage msg;
        msg.request = std::move(request);

        auto response_channel = msg.response_channel;
        boost::system::error_code send_ec;
        co_await request_channel().async_send(send_ec, std::move(msg), boost::asio::use_awaitable);
        if (send_ec) {
            co_return std::unexpected(HttpError{http::status::internal_server_error, "Failed to send request: " + std::string(send_ec.message()) + "\n"});
        }

        NetResponse response;
        try {
            response = co_await response_channel->async_receive(boost::asio::use_awaitable);
        } catch (const boost::system::system_error& e) {
            co_return std::unexpected(HttpError{http::status::internal_server_error, "Failed to receive response: " + std::string(e.code().message()) + "\n"});
        }

        if (std::holds_alternative<HttpAnswer>(response)) {
            auto& answer = std::get<HttpAnswer>(response);
            co_return answer;
        }

        auto& error = std::get<HttpError>(response);
        co_return std::unexpected(error);
    }

    boost::asio::awaitable<dbat::web::Result> handle_auth_register_post(dbat::web::RouteContext& data) {
        // Handle registration
        // first we'll retrieve and parse the JSON body
        auto json_res = parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }
        // we need both a username (string) and password (string) fields...
        auto j = json_res.value();

        UserRegisterReq req;

        if (!j.contains("username") || !j["username"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'username' field\n"});
        }
        req.username = j["username"];
        if (!j.contains("password") || !j["password"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'password' field\n"});
        }
        req.password = j["password"];

        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});

    }

    boost::asio::awaitable<dbat::web::Result> handle_auth_login_post(dbat::web::RouteContext& data) {
        auto json_res = parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }

        auto j = json_res.value();
        UserLoginReq req;

        if (!j.contains("username") || !j["username"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'username' field\n"});
        }
        req.username = j["username"];

        if (!j.contains("password") || !j["password"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'password' field\n"});
        }
        req.password = j["password"];

        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<dbat::web::Result> handle_auth_refresh_post(dbat::web::RouteContext& data) {
        auto json_res = parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }

        auto j = json_res.value();
        UserRefreshReq req;

        if (!j.contains("refresh_token") || !j["refresh_token"].is_string()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'refresh_token' field\n"});
        }
        req.refresh_token = j["refresh_token"];

        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<dbat::web::Result> handle_character_get(dbat::web::RouteContext& data) {
        auto sub_res = require_access_subject(data);
        if (!sub_res) {
            co_return std::unexpected(sub_res.error());
        }

        CharacterListReq req{sub_res.value()};
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<dbat::web::Result> handle_character_post(dbat::web::RouteContext& data) {
        auto sub_res = require_access_subject(data);
        if (!sub_res) {
            co_return std::unexpected(sub_res.error());
        }

        auto json_res = parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }

        CharacterCreateReq req{sub_res.value(), json_res.value()};
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    boost::asio::awaitable<dbat::web::Result> handle_character_delete(dbat::web::RouteContext& data) {
        auto sub_res = require_access_subject(data);
        if (!sub_res) {
            co_return std::unexpected(sub_res.error());
        }

        auto json_res = parse_json_body(data.req);
        if (!json_res) {
            co_return std::unexpected(json_res.error());
        }

        auto j = json_res.value();
        if (!j.contains("character_id") || !j["character_id"].is_number_integer()) {
            co_return std::unexpected(HttpError{http::status::bad_request, "Missing or invalid 'character_id' field\n"});
        }

        CharacterDeleteReq req{sub_res.value(), j["character_id"].get<int64_t>()};
        co_return co_await send_request_and_reply(data, NetRequest{std::move(req)});
    }

    static void init_router() {
        global_router.add("/auth/register", http::verb::post, handle_auth_register_post);
        global_router.add("/auth/login", http::verb::post, handle_auth_login_post);
        global_router.add("/auth/refresh", http::verb::post, handle_auth_refresh_post);
        global_router.add("/character", http::verb::get, handle_character_get);
        global_router.add("/character", http::verb::post, handle_character_post);
        global_router.add("/character", http::verb::delete_, handle_character_delete);
    }

    void init() {
        auto get_env = [](const char* key) -> const char* {
            const char* value = std::getenv(key);
            return (value && *value) ? value : nullptr;
        };

        if (const char* host = get_env("HTTP_HOST")) {
            auto addr = parse_address(host);
            if (addr) {
                config.tcp_address = *addr;
            } else {
                LERROR("Invalid HTTP_HOST: %s", host);
            }
        }

        if (const char* port = get_env("HTTP_PORT")) {
            char* end = nullptr;
            const long value = std::strtol(port, &end, 10);
            if (end != port && value > 0 && value <= 65535) {
                config.tcp_port = static_cast<uint16_t>(value);
            } else {
                LERROR("Invalid HTTP_PORT: %s", port);
            }
        }

        if (const char* host = get_env("HTTPS_HOST")) {
            auto addr = parse_address(host);
            if (addr) {
                tls_config.address = *addr;
            } else {
                LERROR("Invalid HTTPS_HOST: %s", host);
            }
        }

        if (const char* port = get_env("HTTPS_PORT")) {
            char* end = nullptr;
            const long value = std::strtol(port, &end, 10);
            if (end != port && value > 0 && value <= 65535) {
                tls_config.port = static_cast<uint16_t>(value);
            } else {
                LERROR("Invalid HTTPS_PORT: %s", port);
            }
        }

        if (const char* cert = get_env("TLS_ANSWER_FILE")) {
            tls_config.cert_path = cert;
        } else if (const char* cert = get_env("TLS_CERT_FILE")) {
            tls_config.cert_path = cert;
        }

        if (const char* key = get_env("TLS_KEY_FILE")) {
            tls_config.key_path = key;
        }

        if (!tls_config.cert_path.empty() && !tls_config.key_path.empty()) {
            const bool cert_exists = std::filesystem::exists(tls_config.cert_path);
            const bool key_exists = std::filesystem::exists(tls_config.key_path);
            if (cert_exists && key_exists) {
                try {
                    tls_config.ssl_context = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tls_server);
                    tls_config.ssl_context->set_options(
                        boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::no_sslv2 |
                        boost::asio::ssl::context::no_sslv3 |
                        boost::asio::ssl::context::single_dh_use
                    );
                    tls_config.ssl_context->use_certificate_chain_file(tls_config.cert_path.string());
                    tls_config.ssl_context->use_private_key_file(tls_config.key_path.string(), boost::asio::ssl::context::pem);
                    LINFO("TLS enabled using cert %s", tls_config.cert_path.string().c_str());
                } catch (const std::exception& e) {
                    LERROR("Failed to initialize TLS context: %s", e.what());
                    tls_config.ssl_context.reset();
                }
            } else {
                if (!cert_exists) {
                    LERROR("TLS certificate not found: %s", tls_config.cert_path.string().c_str());
                }
                if (!key_exists) {
                    LERROR("TLS key not found: %s", tls_config.key_path.string().c_str());
                }
                tls_config.ssl_context.reset();
            }
        } else {
            tls_config.ssl_context.reset();
        }

        if (const char* secret = get_env("JWT_SECRET")) {
            jwt_config.secret = secret;
        }

        if (const char* exp = get_env("JWT_EXPIRE_MINUTES")) {
            char* end = nullptr;
            const long value = std::strtol(exp, &end, 10);
            if (end != exp && value > 0) {
                jwt_config.token_expiry = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(value));
            } else {
                LERROR("Invalid JWT_EXPIRE_MINUTES: %s", exp);
            }
        }

        if (const char* exp = get_env("JWT_REFRESH_EXPIRE_MINUTES")) {
            char* end = nullptr;
            const long value = std::strtol(exp, &end, 10);
            if (end != exp && value > 0) {
                jwt_config.refresh_token_expiry = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(value));
            } else {
                LERROR("Invalid JWT_REFRESH_EXPIRE_MINUTES: %s", exp);
            }
        }

        if (const char* iss = get_env("JWT_ISSUER")) {
            jwt_config.issuer = iss;
        }

        if (const char* aud = get_env("JWT_AUDIENCE")) {
            jwt_config.audience = aud;
        }

        init_router();
    }

    void start_servers() {
        bind_server(tls_config.address, tls_config.port, tls_config.ssl_context);
        bind_server(config.tcp_address, config.tcp_port, nullptr);
    }

}