#include "api/server.hpp"
#include "api/endpoints.hpp"

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <variant>

#include <nlohmann/json.hpp>

#include "valid/valid.hpp"
#include "dbat/Account.h"
#include "dbat/Descriptor.h"
#include "dbat/players.h"
#include "dbat/db.h"
#include "dbat/Character.h"

#include "serde/saveload.h"

namespace dbat::api {

    struct Tokens {
        std::string access_token;
        std::string refresh_token;
        std::string token_response;
    };

    Tokens generate_tokens(const Account& account) {
        Tokens tokens;
        nlohmann::json claims;
        claims["sub"] = account.id;
        claims["username"] = account.name;
        auto claim_copy = claims;
        tokens.access_token = dbat::jwt::create_access_token(std::move(claims));
        tokens.refresh_token = dbat::jwt::create_refresh_token(std::move(claim_copy));
        tokens.token_response = dbat::jwt::build_token_response(tokens.access_token, tokens.refresh_token).dump();
        return tokens;
    };

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
                    if (dbat::jwt::jwt_config.secret.empty()) {
                        co_return HttpError{http::status::internal_server_error, "JWT secret not configured\n"};
                    }

                    auto tokens = generate_tokens(*account);
                    answer.body = tokens.token_response;
                    co_return answer;
                }
            }
        } else if(std::holds_alternative<UserRefreshReq>(msg.request)) {
            auto& req = std::get<UserRefreshReq>(msg.request);

            auto verify_res = dbat::jwt::verify(req.refresh_token, dbat::jwt::jwt_config.secret);
            if (!verify_res) {
                co_return HttpError{http::status::unauthorized, "Invalid refresh token: " + verify_res.error() + "\n"};
            }

            const auto& payload = verify_res.value();
            if (!payload.contains("token_use") || !payload["token_use"].is_string() || payload["token_use"] != "refresh") {
                co_return HttpError{http::status::unauthorized, "Invalid refresh token type\n"};
            }

            if (payload.contains("iss") && payload["iss"].is_string()) {
                if (payload["iss"].get<std::string>() != dbat::jwt::jwt_config.issuer) {
                    co_return HttpError{http::status::unauthorized, "Invalid refresh token issuer\n"};
                }
            }

            if (payload.contains("aud") && payload["aud"].is_string()) {
                if (payload["aud"].get<std::string>() != dbat::jwt::jwt_config.audience) {
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
            auto tokens = generate_tokens(*account);
            answer.body = tokens.token_response;
            answer.status = http::status::ok;
            answer.content_type = "application/json";
            
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

    static void start_servers() {
        if (dbat::net::tls_config.ssl_context) {
            dbat::net::bind_server(dbat::net::tls_config.address, dbat::net::tls_config.port, dbat::net::tls_config.ssl_context, handle_http);
        } else {
            dbat::net::bind_server(dbat::net::tcp_config.tcp_address, dbat::net::tcp_config.tcp_port, nullptr, handle_http);
        }
    }

    void init() {
        init_router();
        start_servers();
    }

}