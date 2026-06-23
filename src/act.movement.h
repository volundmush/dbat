#pragma once
#include "consts/maximums.h"
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// global variables
extern const char *cmd_door[NUM_DOOR_CMD];

// functions
void handle_teleport(struct char_data *ch, struct char_data *tar, int location);
void dismount_char(struct char_data *ch);
void mount_char(struct char_data *ch, struct char_data *mount);
int land_location(struct char_data *ch, char *arg);
void disp_locations(struct char_data *ch);
void carry_drop(struct char_data *ch, int type);
int has_o2(struct char_data *ch);
int do_simple_move(struct char_data *ch, int dir, int need_specials_check);
int perform_move(struct char_data *ch, int dir, int need_specials_check);
void handle_fall(struct char_data *ch);
int do_simple_enter(struct char_data *ch, struct obj_data *obj, int need_specials_check);
int do_simple_leave(struct char_data *ch, struct obj_data *obj, int need_specials_check);

// commands
ACMD(do_gen_door);
ACMD(do_flee);
ACMD(do_carry);

#ifdef __cplusplus
}
#endif
