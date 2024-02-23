#include "dbat/structs.h"

// Flags
nlohmann::json Flags::serialize() {
    nlohmann::json j;
    for(auto &[ftype, fset] : flags) {
        j["flags"].push_back({ftype, fset});
    }
    return j;
}

void Flags::deserialize(const nlohmann::json& j) {
    if(j.contains("flags")) {
        auto ftypes = j["flags"];
        for(auto& ftype : ftypes) {
            auto ftype = ftype[0].get<FlagType>();
            auto fset = ftype[1].get<std::set<int>>();
            flags.emplace_back(ftype, fset);
        }
    }
}

Flags::Flags(const nlohmann::json& j) {
    deserialize(j);
}


// Extra Descriptions
extra_descr_data::extra_descr_data(const nlohmann::json& j) {
    deserialize(j);
}

void extra_descr_data::deserialize(const nlohmann::json& j) {
    if(j.contains("keyword")) keyword = j["keyword"];
    if(j.contains("description")) description = j["description"];
}

nlohmann::json extra_descr_data::serialize() {
    nlohmann::json j;
    if(!keyword.empty()) j["keyword"] = keyword;
    if(!description.empty()) j["description"] = description;
    return j;
}

ExtraDescriptions::ExtraDescriptions(const nlohmann::json& j) {
    deserialize(j);
}

void ExtraDescriptions::deserialize(const nlohmann::json& j) {
    if(j.contains("ex_description")) {
        auto extras = j["ex_description"];
        for(auto& extra : extras) {
            extras.emplace_back(extra);
        }
    }
}

nlohmann::json ExtraDescriptions::serialize() {
    nlohmann::json j;
    for(auto& extra : extras) {
        j["ex_description"].push_back(extra.serialize());
    }
    return j;
}