#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/triggers.hpp"

/* mob trigger types */
static constexpr std::string_view trig_types_entries[] = {
    "Global",
    "Random",
    "Command",
    "Speech",
    "Act",
    "Death",
    "Greet",
    "Greet-All",
    "Entry",
    "Receive",
    "Fight",
    "HitPrcnt",
    "Bribe",
    "Load",
    "Memory",
    "Cast",
    "Leave",
    "Door",
    "UNUSED",
    "Time"};
static_assert(std::size(trig_types_entries) == NUM_MTRIG_TYPES);
constinit const name_table trig_types(trig_types_entries);

/* obj trigger types */
static constexpr std::string_view otrig_types_entries[] = {
    "Global",
    "Random",
    "Command",
    "UNUSED",
    "UNUSED",
    "Timer",
    "Get",
    "Drop",
    "Give",
    "Wear",
    "UNUSED",
    "Remove",
    "UNUSED",
    "Load",
    "UNUSED",
    "Cast",
    "Leave",
    "UNUSED",
    "Consume",
    "Time"};
static_assert(std::size(otrig_types_entries) == NUM_OTRIG_TYPES);
constinit const name_table otrig_types(otrig_types_entries);

/* wld trigger types */
static constexpr std::string_view wtrig_types_entries[] = {
    "Global",
    "Random",
    "Command",
    "Speech",
    "UNUSED",
    "Zone Reset",
    "Enter",
    "Drop",
    "UNUSED",
    "UNUSED",
    "UNUSED",
    "UNUSED",
    "UNUSED",
    "UNUSED",
    "UNUSED",
    "Cast",
    "Leave",
    "Door",
    "UNUSED",
    "Time"};
static_assert(std::size(wtrig_types_entries) == NUM_WTRIG_TYPES);
constinit const name_table wtrig_types(wtrig_types_entries);
