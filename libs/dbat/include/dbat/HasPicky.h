#pragma once
#include <unordered_set>
#include "const/Race.h"
#include "const/Sensei.h"
#include "const/CharacterProperties.h"
#include "Flags.h"

struct HasPicky {
    FlagHandler<MoralAlign> only_alignment, not_alignment;    /* Neutral, lawful, etc.		*/
    FlagHandler<Sensei> only_sensei, not_sensei;    /* Only these classes can shop here	*/
    FlagHandler<Race> only_race, not_race;    /* Only these races can shop here	*/
};

using picky_data = HasPicky;