#pragma once
#include <cstdint>

enum class UnitType : uint8_t
{
    character = 0,
    object = 1,
    room = 2,
    unknown = 3
};

constexpr UnitType MOB_TRIGGER = UnitType::character;
constexpr UnitType OBJ_TRIGGER = UnitType::object;
constexpr UnitType WLD_TRIGGER = UnitType::room;