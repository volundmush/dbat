#pragma once
#include "dbat/db/consts/types.h"
#include "dbat/db/room_utils.h"
#include "dbat/db/rooms.h"

int num_pc_in_room(struct room_data *room);
int cook_element(room_rnum room);
int room_is_dark(room_rnum room);