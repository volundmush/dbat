#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

struct HasMudStrings {
    // make it virtual.
    virtual ~HasMudStrings() = default;
    std::string name;
    const char* getName() const;
    std::string room_description;
    const char* getRoomDescription() const;
    std::string look_description;
    const char* getLookDescription() const;
    std::string short_description;
    const char* getShortDescription() const;
};