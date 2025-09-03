#pragma once
#include <experimental/memory>

struct Zone;

struct HasZone {
    Zone* getZone() const;
    std::experimental::observer_ptr<Zone> zone{nullptr};
};