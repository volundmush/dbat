#pragma once
#include <string>
#include <unordered_set>

#include "Typedefs.hpp"

#include "const/AffectFlag.hpp"
#include "Flags.hpp"

struct HasSubscriptions {
    std::unordered_set<std::string> subscriptions{}; // Subscriptions to services.
};

inline std::string format_as(const HasSubscriptions& unit) {
    return fmt::format("service subscriptions: [{}]", fmt::join(unit.subscriptions, ", "));
}

struct HasVnum {
    vnum vn{NOTHING};
    vnum getVnum() const;
};

inline std::string format_as(const HasVnum& unit) {
    return fmt::format("vnum: {}", unit.getVnum());
}

struct HasStats {
    std::unordered_map<std::string, double> stats{};
};

inline std::string format_as(const HasStats& unit) {
    std::vector<std::string> stats;
    for(const auto& [name, val] : unit.stats) {
        stats.push_back(fmt::format("{}: {}", name, val));
    }
    if(stats.empty()) stats.push_back("<none>");
    return fmt::format("stats: [{}]", fmt::join(stats, ", "));
}

struct HasID {
    int64_t id{NOTHING}; /* the unique ID of this entity */
};

inline std::string format_as(const HasID& unit) {
    return fmt::format("id: {}", unit.id);
}

struct HasProtoScript {
    std::vector<trig_vnum> proto_script; /* list of default triggers  */
    std::string scriptString() const;
};

inline std::string format_as(const HasProtoScript& unit) {
    if(unit.proto_script.empty()) return "proto_script: <none>";
    return fmt::format("proto_script: [{}]", fmt::join(unit.proto_script, ", "));
}