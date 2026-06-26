#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// functions — drive_in_direction ported to drive.lua
// void drive_in_direction(struct char_data *ch, struct obj_data *vehicle,
//                         int dir);
struct obj_data *find_control(struct char_data *ch);
struct obj_data *find_vehicle_by_vnum(int vnum);
struct obj_data *find_hatch_by_vnum(int vnum);
struct obj_data *get_obj_in_list_type(int type, struct inventory_data list);

// Hatch-vehicle ID cache: two int val slots pack one int64_t vehicle ID.
// Set by obj_to_room when hatch is placed; read at movement time.
int64_t hatch_vehicle_id_get(struct obj_data *hatch);
void    hatch_vehicle_id_set(struct obj_data *hatch, int64_t id);
// Returns the vehicle this hatch leads to; uses stored ID, falls back to scan.
struct obj_data *hatch_get_vehicle(struct obj_data *hatch);

// commands — ported to Lua
// ACMD(do_warp);   // lua/characters/commands/misc/warp.lua
// ACMD(do_drive);  // lua/characters/commands/misc/drive.lua

#ifdef __cplusplus
}
#endif
