#pragma once

#include "structs.h"

extern void handle_songs(uint64_t heartPulse, double deltaTime);

extern void fish_update(uint64_t heartPulse, double deltaTime);

extern void disp_rpp_store(Character *ch);

extern void handle_rpp_store(Character *ch, int choice);

extern void rpp_feature(Character *ch, const char *arg);

extern void ash_burn(Character *ch);

// commands
