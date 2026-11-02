#pragma once
#include "flags.h"
int zone_get_min_level(zone_rnum zone);
int zone_get_max_level(zone_rnum zone);
bool zone_flagged(zone_rnum rnum, int flag);
/* The same on a zone_data pointer, e.g. an OLC working copy. */
bool zone_data_toggle_flag(struct zone_data *zone, int flag);

#include "dbat/zones.h"
#include "dbat/zone_utils.h"

struct reset_com;
struct reset_com *zone_get_cmd(zone_rnum zone, int n);
