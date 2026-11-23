#pragma once
#include <string_view>
#include "dbat/consts/types.hpp"

#define SINFO g->spell_info[spellnum]
extern const std::string_view unused_spellname;

void mag_assign_spells(GameWorld *g);

// Commands

