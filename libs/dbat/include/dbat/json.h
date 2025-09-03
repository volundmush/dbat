#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string jdumps(const json &j);
json jloads(const std::string &s);
std::string jdumps_pretty(const json &j);
json jobject();