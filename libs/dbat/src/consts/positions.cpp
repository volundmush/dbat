#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/positions.hpp"

/* POS_x */
static constexpr auto position_types_entries = std::to_array<std::string_view>({
    "Dead",
    "Mortally wounded",
    "Incapacitated",
    "Stunned",
    "Sleeping",
    "Resting",
    "Sitting",
    "Fighting",
    "Standing"});
static_assert(std::size(position_types_entries) == NUM_POSITIONS);
constinit const name_table position_types(position_types_entries);