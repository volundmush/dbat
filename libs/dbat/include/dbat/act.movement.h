#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/maximums.h"

#ifdef __cplusplus
extern "C" {
#endif

// global variables
extern const char *const cmd_door[NUM_DOOR_CMD];

// functions
void handle_teleport(GameWorld *g, struct char_data *ch, struct char_data *tar, room_vnum location);
void carry_drop(GameWorld *g, struct char_data *ch, int type);
bool has_o2(struct char_data *ch);
bool do_simple_move(GameWorld *g, struct char_data *ch, int dir, int need_specials_check);
bool perform_move(GameWorld *g, struct char_data *ch, int dir, int need_specials_check);

// commands
ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_leave);
ACMD(do_stand);
ACMD(do_fly);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);
ACMD(do_follow);
ACMD(do_carry);
ACMD(do_land);
ACMD(do_move);

#ifdef __cplusplus
}
#endif
