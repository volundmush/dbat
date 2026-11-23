#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/sex.hpp"

static constexpr std::string_view genders_entries[] = {
        "neutral",
        "male",
        "female"};
static_assert(std::size(genders_entries) == NUM_SEX);
constinit const name_table genders(genders_entries);