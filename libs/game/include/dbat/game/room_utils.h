#pragma once
#include "dbat/db/consts/types.h"

#include "dbat/db/rooms.h"

#ifdef __cplusplus
extern "C" {
#endif

int num_pc_in_room(struct room_data *room);
bool cook_element(struct room_data* room);
int room_is_dark(room_rnum room);

bool room_is_sunken(struct room_data* room);

#ifdef __cplusplus
}
#endif
