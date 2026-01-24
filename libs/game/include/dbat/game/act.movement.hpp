#pragma once

#include "const/Max.hpp"
#include "const/Direction.hpp"
struct Character;

// global variables
extern const char *cmd_door[NUM_DOOR_CMD];

// functions
extern void handle_teleport(Character *ch, Character *tar, int location);

extern void carry_drop(Character *ch, int type);

extern int has_o2(Character *ch);

extern int do_simple_move(Character *ch, int dir, int need_specials_check);

extern int perform_move(Character *ch, int dir, int need_specials_check);

// commands
