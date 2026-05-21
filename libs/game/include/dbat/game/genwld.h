#pragma once
#include "dbat/db/consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

room_rnum add_room(struct room_data *);
int delete_room(room_rnum);
int save_rooms(zone_rnum);
int copy_room(struct room_data *to, struct room_data *from);
room_rnum duplicate_room(room_vnum to, room_rnum from);
int copy_room_strings(struct room_data *dest, struct room_data *source);
int free_room_strings(struct room_data *);


#ifdef __cplusplus
}
#endif
