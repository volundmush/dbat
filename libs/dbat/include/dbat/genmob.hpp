#pragma once
#include <cstdio>

#include "dbat/consts/types.hpp"

mob_vnum delete_mobile(GameWorld *g, mob_vnum);
bool copy_mobile(GameWorld *g, struct char_data *to, struct char_data *from);
mob_vnum add_mobile(GameWorld *g, struct char_data *, mob_vnum);
bool free_mobile(GameWorld *g, struct char_data *mob);
bool save_mobiles(GameWorld *g, struct zone_data *rznum);

/* Handy macros. */

