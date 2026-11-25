#include <string>
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
static_assert(std::size(trig_types_entries) == flag_count<mob_trig>);
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
static_assert(std::size(otrig_types_entries) == flag_count<obj_trig>);
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
static_assert(std::size(wtrig_types_entries) == flag_count<wld_trig>);
constinit const name_table wtrig_types(wtrig_types_entries);

static constexpr std::string_view ocmd_bits_entries[] = {"EQUIP", "INVEN", "ROOM"};
static_assert(std::size(ocmd_bits_entries) == flag_count<ocmd_where>);
constinit const name_table ocmd_bits(ocmd_bits_entries);

std::string trig_type_names(int attach_type, bitvector_t word)
{
  switch (attach_type) {
    case OBJ_TRIGGER: return flag_mask_names(flag_set<obj_trig>::from_words(std::span(&word, 1)));
    case WLD_TRIGGER: return flag_mask_names(flag_set<wld_trig>::from_words(std::span(&word, 1)));
    default: return flag_mask_names(flag_set<mob_trig>::from_words(std::span(&word, 1)));
  }
}
