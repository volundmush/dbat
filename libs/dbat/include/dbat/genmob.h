#pragma once
#include "dbat/consts/types.h"
#include <stdio.h>

int delete_mobile(mob_rnum);
bool copy_mobile(struct char_data *to, struct char_data *from);
int add_mobile(struct char_data *, mob_vnum);
bool free_mobile(struct char_data *mob);
bool save_mobiles(zone_rnum rznum);

/* Handy macros. */
