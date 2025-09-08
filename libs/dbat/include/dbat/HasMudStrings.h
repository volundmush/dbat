#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

#include <fmt/format.h>

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

inline std::string format_as(const HasMudStrings& unit) {
    return fmt::format("name: {}\r\nshort_description: {}\r\nlook_description: {}\r\nroom_description: {}",
                       unit.getName(),
                       unit.getShortDescription(),
                       unit.getLookDescription(),
                       unit.getRoomDescription());
}