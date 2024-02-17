//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"


// commands
extern void star_phase(BaseCharacter *ch, int type);

extern void oozaru_revert(BaseCharacter *ch);

void advanceClock(uint64_t heartPulse, double deltaTime);

void moonrise();

void moondown();