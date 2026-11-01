#pragma once
#include "dbat/consts/types.h"

room_rnum add_room(struct room_data *);
int delete_room(room_rnum);
int save_rooms(zone_rnum);
int free_room_strings(struct room_data *);
