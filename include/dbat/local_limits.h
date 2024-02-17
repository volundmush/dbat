//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"

// functions
extern void timed_dt(BaseCharacter *ch);

extern void run_autowiz();

extern void reboot_wizlists();

extern void mutant_limb_regen(BaseCharacter *ch);

extern void set_title(BaseCharacter *ch, char *title);

extern void gain_level(BaseCharacter *ch);

extern void gain_condition(BaseCharacter *ch, int condition, int value);

extern void point_update(uint64_t heartPulse, double deltaTime);

