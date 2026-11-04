#pragma once
#include "dbat/consts/types.h"
#include <stdio.h>

mob_rnum delete_mobile(GameWorld *g, mob_rnum);
bool copy_mobile(GameWorld *g, struct char_data *to, struct char_data *from);
mob_rnum add_mobile(GameWorld *g, struct char_data *, mob_vnum);
bool free_mobile(GameWorld *g, struct char_data *mob);
bool save_mobiles(GameWorld *g, zone_rnum rznum);

/* Handy macros. */
