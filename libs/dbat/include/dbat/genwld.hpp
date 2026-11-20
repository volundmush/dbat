#pragma once
#include <vector>

#include "dbat/consts/types.hpp"

room_vnum add_room(GameWorld *g, struct room_data *);
bool delete_room(GameWorld *g, room_vnum);
bool save_rooms(GameWorld *g, struct zone_data *zone);
bool free_room_strings(struct room_data *);
/* Puts room into the world as room vnum of zone, starting with triggers: a new room, or the one there changed */
void redit_save_internally(GameWorld *g, struct room_data &room, room_vnum vnum, struct zone_data *zone, const std::vector<trig_vnum> &triggers);
