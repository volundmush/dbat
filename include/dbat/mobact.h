//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"


// functions
extern void remember(BaseCharacter *ch, BaseCharacter *victim);

extern void mobile_activity(uint64_t heartPulse, double deltaTime);

extern void forget(BaseCharacter *ch, BaseCharacter *victim);

extern void mob_taunt(BaseCharacter *ch);

extern void clearMemory(BaseCharacter *ch);