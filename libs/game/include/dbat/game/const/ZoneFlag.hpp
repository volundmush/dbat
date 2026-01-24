#pragma once 
#include <cstdint>

enum class ZoneFlag : std::uint8_t
{
    closed = 0,
    no_immortal = 1,
    quest = 2,
    dragon_balls = 3,
    planet = 4,
    ether_stream = 5,
    has_moon = 6
};

constexpr int ZONE_CLOSED = 0;
constexpr int ZONE_NOIMMORT = 1;
constexpr int ZONE_QUEST = 2;
constexpr int ZONE_DBALLS = 3;