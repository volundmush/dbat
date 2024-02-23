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

nlohmann::json mob_special_data::serialize() {
    nlohmann::json j;
    if(attack_type) j["attack_type"] = attack_type;
    if(default_pos != POS_STANDING) j["default_pos"] = default_pos;
    if(damnodice) j["damnodice"] = damnodice;
    if(damsizedice) j["damsizedice"] = damsizedice;

    return j;
}


void mob_special_data::deserialize(const nlohmann::json &j) {
    if(j.contains("attack_type")) attack_type = j["attack_type"];
    if(j.contains("default_pos")) default_pos = j["default_pos"];
    if(j.contains("damnodice")) damnodice = j["damnodice"];
    if(j.contains("damsizedice")) damsizedice = j["damsizedice"];
}

mob_special_data::mob_special_data(const nlohmann::json &j) : mob_special_data() {
    deserialize(j);
}

nlohmann::json Exit::serialize() {
    auto j = GameEntity::serialize();

    if(key > 0) j["key"] = key;

    if(dclock) j["dclock"] = dclock;
    if(dchide) j["dchide"] = dchide;
    if(dcskill) j["dcskill"] = dcskill;
    if(dcmove) j["dcmove"] = dcmove;
    if(failsavetype) j["failsavetype"] = failsavetype;
    if(dcfailsave) j["dcfailsave"] = dcfailsave;

    return j;
}

void Exit::deserialize(const nlohmann::json& j) {
    GameEntity::deserialize(j);
    if(j.contains("key")) key = j["key"];
    if(j.contains("dclock")) dclock = j["dclock"];
    if(j.contains("dchide")) dchide = j["dchide"];
    if(j.contains("dcskill")) dcskill = j["dcskill"];
    if(j.contains("dcmove")) dcmove = j["dcmove"];
    if(j.contains("failsavetype")) failsavetype = j["failsavetype"];
    if(j.contains("dcfailsave")) dcfailsave = j["dcfailsave"];

}