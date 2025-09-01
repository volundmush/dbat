#pragma once
#include "templates.h"

struct HasSubscriptions {
    std::unordered_set<std::string> subscriptions{}; // Subscriptions to services.
};

struct HasVnum {
    vnum vn{NOTHING};
    vnum getVnum() const;
};

struct HasAffectFlags {
    FlagHandler<AffectFlag> affect_flags{};
};

struct HasStats {
    std::unordered_map<std::string, double> stats{};
};

struct HasID {
    int64_t id{NOTHING}; /* the unique ID of this entity */
};