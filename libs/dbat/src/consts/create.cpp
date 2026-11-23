#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/create.hpp"

static constexpr std::string_view creation_methods_entries[] = {
        "[Standard - Random rolls, ordered assignment]",
        "[Player Assigned - Random rolls, player adjust to taste]",
        "[Points Pool - Player assigns scores from points pool]",
        "[Racial Template - All races start from template values, adjusted by class]",
        "[Class Template - All classes start from template values, adjusted by race]"};
static_assert(std::size(creation_methods_entries) == NUM_CREATION_METHODS);
constinit const name_table creation_methods(creation_methods_entries);
