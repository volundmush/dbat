#pragma once
#include "flags.h"
int zone_get_min_level(zone_rnum zone);
void zone_set_min_level(zone_rnum zone, int v);
int zone_get_max_level(zone_rnum zone);
void zone_set_max_level(zone_rnum zone, int v);
bool zone_flagged(zone_rnum rnum, int flag);

#include "dbat/zones.h"
#include "dbat/zone_utils.h"
