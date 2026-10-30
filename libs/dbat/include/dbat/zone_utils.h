#pragma once
#include "flags.h"
#define ZONE_FLAGS(rnum)       (zone_table[(rnum)].zone_flags)
#define ZONE_MINLVL(rnum)      (zone_table[(rnum)].min_level)
#define ZONE_MAXLVL(rnum)      (zone_table[(rnum)].max_level)
bool zone_flagged(zone_rnum rnum, int flag);

#include "dbat/zones.h"
#include "dbat/zone_utils.h"
