#include "dbat/json.h"

std::string jdump(const nlohmann::json& j) {
    return j.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);
}

std::string jdump_pretty(const nlohmann::json& j) {
    return j.dump(4, ' ', false, nlohmann::json::error_handler_t::ignore);
}

nlohmann::json jparse(const std::string& s) {
    return nlohmann::json::parse(s);
}