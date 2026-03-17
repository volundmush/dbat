#include "dbat/game/HasResetCommands.hpp"
#include <nlohmann/json.hpp>

void to_json(nlohmann::json& j, const ResetCommand& unit)
{
    j[+"type"] = unit.type;
    if(unit.if_flag) {
        j[+"if_flag"] = unit.if_flag;
    }
    if(unit.target != -1) {
        j[+"target"] = unit.target;
    }
    if(unit.max != 0) {
        j[+"max"] = unit.max;
    }
    if(unit.max_location != 0) {
        j[+"max_location"] = unit.max_location;
    }
    if(unit.ex != 0) {
        j[+"ex"] = unit.ex;
    }
    if(unit.chance != 100) {
        j[+"chance"] = unit.chance;
    }
    if(!unit.key.empty()) {
        j[+"key"] = unit.key;
    }
    if(!unit.value.empty()) {
        j[+"value"] = unit.value;
    }
}

void from_json(const nlohmann::json& j, ResetCommand& unit)
{
    j.at(+"type").get_to(unit.type);
    if(j.contains(+"if_flag")) {
        j.at(+"if_flag").get_to(unit.if_flag);
    }
    if(j.contains(+"target")) {
        j.at(+"target").get_to(unit.target);
    }
    if(j.contains(+"max")) {
        j.at(+"max").get_to(unit.max);
    }
    if(j.contains(+"max_location")) {
        j.at(+"max_location").get_to(unit.max_location);
    }
    if(j.contains(+"ex")) {
        j.at(+"ex").get_to(unit.ex);
    }
    if(j.contains(+"chance")) {
        j.at(+"chance").get_to(unit.chance);
    }
    if(j.contains(+"key")) {
        j.at(+"key").get_to(unit.key);
    }
    if(j.contains(+"value")) {
        j.at(+"value").get_to(unit.value);
    }
}

void to_json(nlohmann::json& j, const HasResetCommands& unit)
{
    if(!unit.resetCommands.empty()) {
        j[+"resetCommands"] = unit.resetCommands;
    }
}

void from_json(const nlohmann::json& j, HasResetCommands& unit)
{
    if(j.contains(+"resetCommands")) {
        j.at(+"resetCommands").get_to(unit.resetCommands);
    }
}
