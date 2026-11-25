#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/exitflags.hpp"

static constexpr std::string_view exit_bits_entries[] = {
    "DOOR",
    "CLOSED",
    "LOCKED",
    "PICKPROOF",
    "SECRET"};
static_assert(std::size(exit_bits_entries) == flag_count<exit_flag>);
constinit const name_table exit_bits(exit_bits_entries);