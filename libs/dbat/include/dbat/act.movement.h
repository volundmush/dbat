#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/maximums.h"

// global variables
extern const char *cmd_door[NUM_DOOR_CMD];

// functions
void handle_teleport(struct char_data *ch, struct char_data *tar, int location);
void carry_drop(struct char_data *ch, int type);
int has_o2(struct char_data *ch);
int do_simple_move(struct char_data *ch, int dir, int need_specials_check);
int perform_move(struct char_data *ch, int dir, int need_specials_check);

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
