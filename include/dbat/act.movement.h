#pragma once

#include "structs.h"

// global variables
extern const char *cmd_door[NUM_DOOR_CMD];

// functions
extern void handle_teleport(Character *ch, Character *tar, int location);

extern std::optional<room_vnum> land_location(char *arg, std::set<room_vnum>& rooms);

extern void carry_drop(Character *ch, int type);

extern int has_o2(Character *ch);

// commands
extern ACMD(do_gen_door);

extern ACMD(do_enter);

extern ACMD(do_leave);

extern ACMD(do_stand);

extern ACMD(do_fly);

extern ACMD(do_sit);

extern ACMD(do_rest);

extern ACMD(do_sleep);

extern ACMD(do_wake);

extern ACMD(do_follow);

extern ACMD(do_flee);

extern ACMD(do_carry);

extern ACMD(do_land);

extern ACMD(do_move);
