//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"

// functions
extern void fly_planet(room_vnum roomVnum, const char *messg, Character *ch);
extern void fly_zone(zone_rnum zone, char *messg, Character *ch);

extern void send_to_scouter(const char *messg, Character *ch, int num, int type);

extern void send_to_sense(int type, const char *messg, Character *ch);

extern void send_to_worlds(Character *ch);
