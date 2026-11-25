#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/exitflags.hpp"

static constexpr auto exit_bits_entries = std::to_array<std::string_view>({
    "DOOR",
    "CLOSED",
    "LOCKED",
    "PICKPROOF",
    "SECRET"});
static_assert(std::size(exit_bits_entries) == flag_count<exit_flag>);
constinit const name_table exit_bits(exit_bits_entries);