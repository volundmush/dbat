#pragma once
#include <experimental/memory>

#include <fmt/format.h>

#include "Zone.h"

struct HasZone {
    Zone* getZone() const;
    std::experimental::observer_ptr<Zone> zone{nullptr};
};

inline std::string format_as(const HasZone& hz) {
    if(auto z = hz.getZone()) {
        return fmt::format("Zone: {}", *z);
    } else {
        return "Zone: <none>";
    }
}