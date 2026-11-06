#pragma once
#include "dbat/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

room_rnum add_room(GameWorld *g, struct room_data *);
bool delete_room(GameWorld *g, room_rnum);
bool save_rooms(GameWorld *g, zone_rnum);
bool free_room_strings(struct room_data *);

#ifdef __cplusplus
}
#endif
