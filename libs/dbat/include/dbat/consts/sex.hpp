#pragma once
#include "dbat/name_table.hpp"
#include <string_view>

/* Sex */
constexpr int SEX_NEUTRAL = 0;
constexpr int SEX_MALE    = 1;
constexpr int SEX_FEMALE  = 2;

constexpr int NUM_SEX = 3;

extern const name_table genders; /* NUM_SEX of them */

