#include "dbat/HasMudStrings.h"


const char *HasMudStrings::getName() const
{
    if (auto find = strings.find("name"); find != strings.end())
    {
        return find->second.c_str();
    }
    return "undefined";
}

const char *HasMudStrings::getRoomDescription() const
{
    if (auto find = strings.find("room_description"); find != strings.end())
    {
        return find->second.c_str();
    }
    return "undefined";
}

const char *HasMudStrings::getLookDescription() const
{
    if (auto find = strings.find("look_description"); find != strings.end())
    {
        return find->second.c_str();
    }
    return "undefined";
}

const char *HasMudStrings::getShortDescription() const
{
    if (auto find = strings.find("short_description"); find != strings.end())
    {
        return find->second.c_str();
    }
    return "undefined";
}

std::string_view HasMudStrings::getString(const std::string &key) const
{
    if (auto it = strings.find(key); it != strings.end())
    {
        return it->second;
    }
    return {};
}