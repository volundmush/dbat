#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/assemblytypes.hpp"

static constexpr std::string_view AssemblyTypes_entries[] = {
    "Don't Use",
    "Don't Use",
    "build",
    "Don't Use",
    "Don't Use",
    "Don't Use",
    "Don't Use",
    "Don't Use",
    "Don't Use",
    "Don't Use",
    "Don't Use"};
static_assert(std::size(AssemblyTypes_entries) == MAX_ASSM);
constinit const name_table AssemblyTypes(AssemblyTypes_entries);