#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/zoneflags.hpp"

static constexpr std::string_view zone_bits_entries[] = {
    "CLOSED",
    "NO_IMMORT",
    "QUEST",
    "DBALL_LOAD",
    "SPARE2",
    "SPARE3",
    "SPARE4",
    "SPARE5",
    "SPARE6",
    "SPARE7",
    "SPARE8",
    "SPARE9",
    "SPARE10",
    "SPARE11",
    "SPARE12",
    "SPARE13",
    "SPARE14",
    "SPARE15",
    "SPARE16",
    "SPARE17",
    "SPARE18",
    "SPARE19",
    "SPARE20",
    "SPARE21",
    "SPARE22",
    "SPARE23",
    "SPARE24",
    "SPARE25",
    "SPARE26",
    "SPARE27",
    "SPARE28",
    "SPARE29",
    "SPARE30",
    "SPARE31",
    "SPARE32",
    "SPARE33"};
static_assert(std::size(zone_bits_entries) == NUM_ZONE_FLAGS);
constinit const name_table zone_bits(zone_bits_entries);