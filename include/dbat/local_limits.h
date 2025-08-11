//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"

// functions
extern void reboot_wizlists();

extern void mutant_limb_regen(Character *ch);

extern void set_title(Character *ch, char *title);

extern void gain_level(Character *ch);

extern void gain_condition(Character *ch, int condition, int value);

extern void point_update(uint64_t heartPulse, double deltaTime);

extern void healTankService(uint64_t heartPulse, double deltaTime);

extern void characterVitalsRecovery(uint64_t heartPulse, double deltaTime);

extern void corpseRotService(uint64_t heartPulse, double deltaTime);

extern void androidAbsorbSystem(uint64_t heartPulse, double deltaTime);

extern void goopTimeService(uint64_t heartPulse, double deltaTime);