#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/npc.hpp"

/* NPC Personality Types */
static constexpr std::string_view npc_personality_entries[] = {
    "Basic",       /* 0 */
    "Careful",     /* 1 */
    "Aggressive",  /* 2 */
    "Arrogant",    /* 3 */
    "Intelligent", /* 4 */
    };
static_assert(std::size(npc_personality_entries) == MAX_PERSONALITIES);
constinit const name_table npc_personality(npc_personality_entries);
