#pragma once
#include <cstdint>

enum class Sex : uint8_t
{
    neutral = 0,
    male = 1,
    female = 2
};

constexpr Sex SEX_NEUTRAL = Sex::neutral;
constexpr Sex SEX_MALE = Sex::male;
constexpr Sex SEX_FEMALE = Sex::female;