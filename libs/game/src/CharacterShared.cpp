#include "dbat/game/CharacterShared.hpp"
#include <nlohmann/json.hpp>

void to_json(nlohmann::json& j, const time_data& unit)
{
    if(unit.birth != 0) {
        j[+"birth"] = unit.birth;
    }
    if(unit.created != 0) {
        j[+"created"] = unit.created;
    }
    if(unit.maxage != 0) {
        j[+"maxage"] = unit.maxage;
    }
    if(unit.logon != 0) {
        j[+"logon"] = unit.logon;
    }
    if(unit.played != 0.0) {
        j[+"played"] = unit.played;
    }
    if(unit.seconds_aged != 0.0) {
        j[+"seconds_aged"] = unit.seconds_aged;
    }
}

void from_json(const nlohmann::json& j, time_data& unit)
{
    if(j.contains(+"birth")) {
        j.at(+"birth").get_to(unit.birth);
    }
    if(j.contains(+"created")) {
        j.at(+"created").get_to(unit.created);
    }
    if(j.contains(+"maxage")) {
        j.at(+"maxage").get_to(unit.maxage);
    }
    if(j.contains(+"logon")) {
        j.at(+"logon").get_to(unit.logon);
    }
    if(j.contains(+"played")) {
        j.at(+"played").get_to(unit.played);
    }
    if(j.contains(+"seconds_aged")) {
        j.at(+"seconds_aged").get_to(unit.seconds_aged);
    }
}

void to_json(nlohmann::json& j, const alias_data& unit)
{
    if(!unit.name.empty()) {
        j[+"name"] = unit.name;
    }
    if(!unit.replacement.empty()) {
        j[+"replacement"] = unit.replacement;
    }
    if(unit.type != 0) {
        j[+"type"] = unit.type;
    }
}

void from_json(const nlohmann::json& j, alias_data& unit)
{
    if(j.contains(+"name")) {
        j.at(+"name").get_to(unit.name);
    }
    if(j.contains(+"replacement")) {
        j.at(+"replacement").get_to(unit.replacement);
    }
    if(j.contains(+"type")) {
        j.at(+"type").get_to(unit.type);
    }
}

void to_json(nlohmann::json& j, const skill_data& unit)
{
    if(unit.level != 0) {
        j[+"level"] = unit.level;
    }
    if(unit.perfs != 0) {
        j[+"perfs"] = unit.perfs;
    }
}

void from_json(const nlohmann::json& j, skill_data& unit)
{
    if(j.contains(+"level")) {
        j.at(+"level").get_to(unit.level);
    }
    if(j.contains(+"perfs")) {
        j.at(+"perfs").get_to(unit.perfs);
    }
}
