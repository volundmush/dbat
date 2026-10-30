#include "dbat/zone_utils.h"
#include "dbat/utils.h"
#include "dbat/db.h"

/* Formerly macros in zone_utils.h. */

bool zone_flagged(zone_rnum rnum, int flag) {
    return (IS_SET_AR(zone_table[(rnum)].zone_flags, flag));
}
