#pragma once
#include "flags.h"
#define ZONE_FLAGS(rnum)       (zone_table[(rnum)].zone_flags)
#define ZONE_MINLVL(rnum)      (zone_table[(rnum)].min_level)
#define ZONE_MAXLVL(rnum)      (zone_table[(rnum)].max_level)
#define ZONE_FLAGGED(rnum, flag)   (IS_SET_AR(zone_table[(rnum)].zone_flags, flag))
