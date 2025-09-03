#pragma once
#include <unordered_set>
#include "const/Race.h"
#include "const/Sensei.h"
#include "const/CharacterProperties.h"
// #include "Flags.h"

// TODO: Replace these std::unordered_set's with FlagHandler<T>

struct HasPicky {
    std::unordered_set<MoralAlign> only_alignment, not_alignment;    /* Neutral, lawful, etc.		*/
    std::unordered_set<Sensei> only_sensei, not_sensei;    /* Only these classes can shop here	*/
    std::unordered_set<Race> only_race, not_race;    /* Only these races can shop here	*/
};

using picky_data = HasPicky;