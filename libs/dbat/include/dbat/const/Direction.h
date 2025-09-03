#pragma once
#include <cstdint>


enum class Direction : uint8_t
{
    north = 0,
    east = 1,
    south = 2,
    west = 3,
    up = 4,
    down = 5,
    northwest = 6,
    northeast = 7,
    southeast = 8,
    southwest = 9,
    inside = 10,
    outside = 11
};

constexpr int NORTH = 0;
constexpr int EAST = 1;
constexpr int SOUTH = 2;
constexpr int WEST = 3;
constexpr int UP = 4;
constexpr int DOWN = 5;
constexpr int NORTHWEST = 6;
constexpr int NORTHEAST = 7;
constexpr int SOUTHEAST = 8;
constexpr int SOUTHWEST = 9;
constexpr int INDIR = 10;
constexpr int OUTDIR = 11;