#pragma once
#include "flags.h"
int zone_get_min_level(zone_rnum zone);
void zone_set_min_level(zone_rnum zone, int v);
int zone_get_max_level(zone_rnum zone);
void zone_set_max_level(zone_rnum zone, int v);
bool zone_flagged(zone_rnum rnum, int flag);
void zone_set_flag(zone_rnum rnum, int flag, bool on);
bool zone_toggle_flag(zone_rnum rnum, int flag);
/* The same on a zone_data pointer, e.g. an OLC working copy. */
bool zone_data_flagged(const struct zone_data *zone, int flag);
void zone_data_set_flag(struct zone_data *zone, int flag, bool on);
bool zone_data_toggle_flag(struct zone_data *zone, int flag);

#include "dbat/zones.h"
#include "dbat/zone_utils.h"
