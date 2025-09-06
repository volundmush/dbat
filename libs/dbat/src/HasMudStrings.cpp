#include "dbat/HasMudStrings.h"


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
