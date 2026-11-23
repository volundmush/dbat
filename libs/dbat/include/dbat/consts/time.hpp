#pragma once
#include "dbat/name_table.hpp"
#include <string_view>

constexpr int NUM_WEEK_DAYS = 7;
constexpr int NUM_MONTHS    = 12;

extern const name_table weekdays; /* NUM_WEEK_DAYS of them */
extern const name_table month_name; /* NUM_MONTHS of them */

