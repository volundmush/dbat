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

constexpr Direction NORTH = Direction::north;
constexpr Direction EAST = Direction::east;
constexpr Direction SOUTH = Direction::south;
constexpr Direction WEST = Direction::west;
constexpr Direction UP = Direction::up;
constexpr Direction DOWN = Direction::down;
constexpr Direction NORTHWEST = Direction::northwest;
constexpr Direction NORTHEAST = Direction::northeast;
constexpr Direction SOUTHEAST = Direction::southeast;
constexpr Direction SOUTHWEST = Direction::southwest;
constexpr Direction INDIR = Direction::inside;
constexpr Direction OUTDIR = Direction::outside;