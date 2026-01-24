#pragma once
#include <cstdint>

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
enum class Size : int8_t
{
    undefined = -1,
    fine = 0,
    diminutive = 1,
    tiny = 2,
    small = 3,
    medium = 4,
    large = 5,
    huge = 6,
    gargantuan = 7,
    colossal = 8
};

constexpr int SIZE_UNDEFINED = -1;
constexpr int SIZE_FINE = 0;
constexpr int SIZE_DIMINUTIVE = 1;
constexpr int SIZE_TINY = 2;
constexpr int SIZE_SMALL = 3;
constexpr int SIZE_MEDIUM = 4;
constexpr int SIZE_LARGE = 5;
constexpr int SIZE_HUGE = 6;
constexpr int SIZE_GARGANTUAN = 7;
constexpr int SIZE_COLOSSAL = 8;
