#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/mobflags.hpp"

/* MOB_x */
static constexpr std::string_view action_bits_entries[] = {
    "SPEC",
    "SENTINEL",
    "NOSCAVENGER",
    "ISNPC",
    "AWARE",
    "AGGR",
    "STAY-ZONE",
    "WIMPY",
    "AGGR_EVIL",
    "AGGR_GOOD",
    "AGGR_NEUTRAL",
    "MEMORY",
    "HELPER",
    "NO_CHARM",
    "NO_SUMMN",
    "NO_SLEEP",
    "AUTOBALANCE",
    "NO_BLIND",
    "NO_KILL",
    "DEAD", /* You should never see this. */
    "MOUNTABLE",
    "Right Arm",
    "Left Arm",
    "Right Leg",
    "Left Leg",
    "Head",
    "JUST_DESC",
    "HUSK",
    "SPAR",
    "DUMMY",
    "ABSORB",
    "REPAIR",
    "NOPOISON",
    "KNOWKAIO",
    "POWERINGUP"};
static_assert(std::size(action_bits_entries) == flag_count<mob_flag>);
constinit const name_table action_bits(action_bits_entries);
