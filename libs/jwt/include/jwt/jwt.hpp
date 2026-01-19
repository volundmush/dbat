#pragma once

#include <string>
#include <chrono>
#include <expected>
#include <nlohmann/json_fwd.hpp>

namespace dbat::jwt {
     std::string create(const nlohmann::json& payload, std::string_view secret, std::chrono::seconds expiration = std::chrono::seconds(3600));

     std::expected<nlohmann::json, std::string> verify(std::string_view token, std::string_view secret);
     
     std::string base64UrlEncode(std::string_view input);
     std::string base64UrlDecode(std::string_view input);
} // namespace dbat::jwt