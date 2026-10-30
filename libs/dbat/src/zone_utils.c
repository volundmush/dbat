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

bool zone_data_flagged(const struct zone_data *zone, int flag) {
    return IS_SET_AR(zone->zone_flags, flag) != 0;
}

void zone_data_set_flag(struct zone_data *zone, int flag, bool on) {
    if (on)
        SET_BIT_AR(zone->zone_flags, flag);
    else
        REMOVE_BIT_AR(zone->zone_flags, flag);
}

bool zone_data_toggle_flag(struct zone_data *zone, int flag) {
    return (TOGGLE_BIT_AR(zone->zone_flags, flag) & Q_BIT(flag)) != 0;
}

void zone_set_flag(zone_rnum rnum, int flag, bool on) {
    zone_data_set_flag(&zone_table[rnum], flag, on);
}

bool zone_toggle_flag(zone_rnum rnum, int flag) {
    return zone_data_toggle_flag(&zone_table[rnum], flag);
}
