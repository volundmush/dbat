#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/admlevel.hpp"

static constexpr auto admin_level_names_entries = std::to_array<std::string_view>({
    "Mortal",
    "Enforcer",
    "1st C. Enforcer",
    "High Enforcer",
    "Vice-Admin",
    "Admin",
    "Implementor",
});
static_assert(std::size(admin_level_names_entries) == ADMLVL_IMPL + 1);
constinit const name_table admin_level_names(admin_level_names_entries);