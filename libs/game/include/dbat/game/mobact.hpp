//
// Created by volund on 10/20/21.
//
#pragma once
#include <cstdint>
struct Character;

// functions
extern void mobile_activity(uint64_t heartPulse, double deltaTime);

extern void mob_taunt(Character *ch);
