#pragma once
#include <cstdint>
#include "Typedefs.h"
struct Character;

// functions
extern void search_replace(char *string, const char *find, const char *replace);

extern void update_space(void);

extern room_rnum find_target_room(Character *ch, char *rawroomstr);

extern void perform_immort_vis(Character *ch);

extern void snoop_check(Character *ch);

extern void copyover_check(uint64_t heartPulse, double deltaTime);

// commands
