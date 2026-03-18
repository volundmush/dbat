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

void to_json(nlohmann::json &j, const HasProtoScript &s) {
    if(!s.proto_script.empty()) {
        j["proto_script"] = s.proto_script;
    }
}

void from_json(const nlohmann::json &j, HasProtoScript &s) {
    if(j.contains(+"proto_script")) {
        j.at(+"proto_script").get_to(s.proto_script);
    }
}
