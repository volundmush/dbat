#include "dbat/game/HasMudStrings.hpp"
#include <nlohmann/json.hpp>


const char *HasMudStrings::getName() const
{
    if(!name.empty()) {
        return name.c_str();
    }
    return "undefined";
}

const char *HasMudStrings::getRoomDescription() const
{
    if(!room_description.empty()) {
        return room_description.c_str();
    }
    return "undefined";
}

const char *HasMudStrings::getLookDescription() const
{
    if(!look_description.empty()) {
        return look_description.c_str();
    }
    return "undefined";
}

const char *HasMudStrings::getShortDescription() const
{
    if(!short_description.empty()) {
        return short_description.c_str();
    }
    return "undefined";
}

void to_json(nlohmann::json& j, const HasMudStrings& unit)
{
    if(!unit.name.empty()) {
        j[+"name"] = unit.name;
    }
    if(!unit.room_description.empty()) {
        j[+"room_description"] = unit.room_description;
    }
    if(!unit.look_description.empty()) {
        j[+"look_description"] = unit.look_description;
    }
    if(!unit.short_description.empty()) {
        j[+"short_description"] = unit.short_description;
    }
}

void from_json(const nlohmann::json& j, HasMudStrings& unit)
{
    if(j.contains("+name")) {
        j.at("+name").get_to(unit.name);
    }
    if(j.contains("+room_description")) {
        j.at("+room_description").get_to(unit.room_description);
    }
    if(j.contains("+look_description")) {
        j.at("+look_description").get_to(unit.look_description);
    }
    if(j.contains("+short_description")) {
        j.at("+short_description").get_to(unit.short_description);
    }
}
