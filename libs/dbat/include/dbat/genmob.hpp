#pragma once
#include <cstdio>

#include "dbat/consts/types.hpp"

mob_vnum delete_mobile(GameWorld *g, mob_vnum);
bool copy_mobile(GameWorld *g, struct char_data *to, struct char_data *from);
mob_vnum add_mobile(GameWorld *g, struct char_data *, mob_vnum);
bool free_mobile(GameWorld *g, struct char_data *mob);
bool save_mobiles(GameWorld *g, struct zone_data *rznum);
/* Gives each of a mob's strings that is empty, which its file can't hold, a placeholder, and logs it */
void check_mobile_strings(GameWorld *g, struct char_data *mob);

/* Handy macros. */

