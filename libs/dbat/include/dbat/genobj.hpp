#pragma once
#include "dbat/consts/types.hpp"

bool copy_object(GameWorld *g, struct obj_data *to, struct obj_data *from);
bool save_objects(GameWorld *g, struct zone_data *vznum);
obj_vnum add_object(GameWorld *g, struct obj_data *, obj_vnum ovnum);
obj_vnum delete_object(GameWorld *g, obj_vnum);
