#include "dbat/zone_utils.h"
#include "dbat/utils.h"
#include "dbat/db.h"

/* Formerly macros in zone_utils.h. */

bool zone_flagged(zone_rnum rnum, int flag) {
    return (IS_SET_AR(zone_table[(rnum)].zone_flags, flag));
}

/* Field accessors (formerly accessor macros). */

int zone_get_min_level(zone_rnum zone) {
    return zone_table[zone].min_level;
}

void zone_set_min_level(zone_rnum zone, int v) {
    zone_table[zone].min_level = v;
}

int zone_get_max_level(zone_rnum zone) {
    return zone_table[zone].max_level;
}

void zone_set_max_level(zone_rnum zone, int v) {
    zone_table[zone].max_level = v;
}
