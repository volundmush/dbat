#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/songs.hpp"

static constexpr auto song_types_entries = std::to_array<std::string_view>({
    "\n",
    "Safety",
    "Shielding",
    "Shadow Stitch",
    "Teleportation",
    "Teleportation",
    "Teleportation",
    "Teleportation",
    "Teleportation",
    "Teleportation",
    "Teleportation",
    "Teleportation",
    "\n"});
static_assert(std::size(song_types_entries) == NUM_SONG_NAMES);
constinit const name_table song_types(song_types_entries);