#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/sex.hpp"

static constexpr auto genders_entries = std::to_array<std::string_view>({
        "neutral",
        "male",
        "female"});
static_assert(std::size(genders_entries) == NUM_SEX);
constinit const name_table genders(genders_entries);