#pragma once
#include "dbat/consts/types.hpp"

room_vnum add_room(GameWorld *g, struct room_data *);
bool delete_room(GameWorld *g, room_vnum);
bool save_rooms(GameWorld *g, struct zone_data *zone);
bool free_room_strings(struct room_data *);

