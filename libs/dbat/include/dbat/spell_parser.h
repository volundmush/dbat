#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SINFO g->spell_info[spellnum]
extern const char *const unused_spellname;

void mag_assign_spells(GameWorld *g);

// Commands

#ifdef __cplusplus
}
#endif
