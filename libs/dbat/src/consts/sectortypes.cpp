#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/sectortypes.hpp"

/* SECT_ */
static constexpr auto sector_types_entries = std::to_array<std::string_view>({
    "Inside",
    "City",
    "Plain",
    "Forest",
    "Hills",
    "Mountains",
    "Shallows",
    "Water (Fly)",
    "Sky",
    "Underwater",
    "$Shop",
    "#Important",
    "Desert",
    "Space",
    "Lava"});
static_assert(std::size(sector_types_entries) == NUM_ROOM_SECTORS);
constinit const name_table sector_types(sector_types_entries);
