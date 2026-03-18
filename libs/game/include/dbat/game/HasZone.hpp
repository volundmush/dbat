#pragma once
#include <experimental/memory>
#include <nlohmann/json_fwd.hpp>

#include <fmt/format.h>

#include "Zone.hpp"

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

void to_json(nlohmann::json& j, const HasZone& p);
void from_json(const nlohmann::json& j, HasZone& p);
