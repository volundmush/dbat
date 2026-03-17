#include "dbat/game/HasMisc.hpp"
#include <nlohmann/json.hpp>

void to_json(nlohmann::json& j, const HasStats& hs) {
    if(!hs.stats.empty()) {
        j[+"stats"] = hs.stats;
    }
}

void from_json(const nlohmann::json& j, HasStats& hs) {
    if(j.contains(+"stats")) {
        j.at(+"stats").get_to(hs.stats);
    }
}

void to_json(nlohmann::json& j, const HasID& hs) {
    j[+"id"] = hs.id;
}

void from_json(const nlohmann::json& j, HasID& hs) {
    j.at("id").get_to(hs.id);
}
