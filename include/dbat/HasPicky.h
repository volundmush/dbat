#pragma once
#include "defs.h"

struct HasPicky {
    std::unordered_set<MoralAlign> only_alignment, not_alignment;    /* Neutral, lawful, etc.		*/
    std::unordered_set<Sensei> only_sensei, not_sensei;    /* Only these classes can shop here	*/
    std::unordered_set<Race> only_race, not_race;    /* Only these races can shop here	*/
};

using picky_data = HasPicky;