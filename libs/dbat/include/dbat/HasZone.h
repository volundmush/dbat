#pragma once
#include <experimental/memory>

#include <fmt/format.h>

struct Zone;

struct HasZone {
    Zone* getZone() const;
    std::experimental::observer_ptr<Zone> zone{nullptr};
};

inline std::string format_as(const HasZone& hz) {
    if(hz.zone) {
        return fmt::format("Zone: {}", *hz.zone);
    } else {
        return "Zone: <none>";
    }
}