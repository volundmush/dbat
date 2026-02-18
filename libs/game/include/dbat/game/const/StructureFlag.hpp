#pragma once
#include <cstdint>

enum class StructureFlag : std::uint8_t {
    space = 0,
    air = 1,
    ground = 2,
    water = 3,
    underwater = 4,
    capsule = 5,
    warp = 6
};