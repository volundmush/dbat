#pragma once
#include "sysdep.h"

struct HasZone {
    Zone* getZone() const;
    std::experimental::observer_ptr<Zone> zone{nullptr};
};