#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/history.hpp"

static constexpr std::string_view history_types_entries[] = {
    "all",
    "say",
    "ooc",
    "wiznet",
    "tell",
    "shout",
    "grats",
    "holler",
    "newbie",
    "snet"};
static_assert(std::size(history_types_entries) == NUM_HIST);
constinit const name_table history_types(history_types_entries);