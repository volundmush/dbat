//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"


// functions
extern void remember(Character *ch, Character *victim);

extern void mobile_activity(uint64_t heartPulse, double deltaTime);

extern void forget(Character *ch, Character *victim);

extern void mob_taunt(Character *ch);

extern void clearMemory(Character *ch);