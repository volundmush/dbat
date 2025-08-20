#pragma once
#include "sysdep.h"

struct HasMudStrings {
    // make it virtual.
    virtual ~HasMudStrings() = default;
    const char* getName() const;
    const char* getRoomDescription() const;
    const char* getLookDescription() const;
    const char* getShortDescription() const;
    std::string_view getString(const std::string &key) const; // Returns a string from the strings map.
    std::unordered_map<std::string, std::string> strings;
};