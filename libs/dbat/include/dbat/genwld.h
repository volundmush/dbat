#pragma once
#include "dbat/consts/types.h"

room_rnum add_room(struct room_data *);
bool delete_room(room_rnum);
bool save_rooms(zone_rnum);
bool free_room_strings(struct room_data *);
