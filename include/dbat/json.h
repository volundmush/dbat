#pragma once
#include "nlohmann/json.hpp"

using json = nlohmann::json;

std::string jdump(const nlohmann::json& j);
nlohmann::json jparse(const std::string& s);
std::string jdump_pretty(const nlohmann::json& j);