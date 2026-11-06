#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// functions
void drive_in_direction(GameWorld *g, struct char_data *ch, struct obj_data *vehicle, int dir);
struct obj_data *find_control(GameWorld *g, struct char_data *ch);
struct obj_data *find_vehicle_by_vnum(GameWorld *g, obj_vnum vnum);

// commands
ACMD(do_warp);
ACMD(do_drive);

#ifdef __cplusplus
}
#endif
