#pragma once

#include <string>
#include <chrono>
#include <expected>
#include <nlohmann/json.hpp>

namespace dbat::jwt {
     std::string create(const nlohmann::json& payload, const std::string& secret, std::chrono::seconds expiration = std::chrono::seconds(3600));


     std::expected<nlohmann::json, std::string> verify(const std::string& token, const std::string& secret);

     std::string base64UrlEncode(const std::string& input);
     std::string base64UrlDecode(const std::string& input);
} // namespace dbat::jwt