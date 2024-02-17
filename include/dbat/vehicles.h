//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"


// functions
extern void drive_in_direction(BaseCharacter *ch, Object *vehicle, int dir);

Object *find_control(BaseCharacter *ch);

Object *find_vehicle_by_vnum(int vnum);

Object *find_hatch_by_vnum(int vnum);


// commands
extern ACMD(do_drive);

extern ACMD(do_ship_fire);
