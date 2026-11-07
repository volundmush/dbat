#pragma once
#include "dbat/flags.hpp"
#include "dbat/zones.hpp"

int zone_get_min_level(GameWorld *g, zone_rnum zone);
int zone_get_max_level(GameWorld *g, zone_rnum zone);
bool zone_flagged(GameWorld *g, zone_rnum rnum, int flag);
/* The same on a zone_data pointer, e.g. an OLC working copy. */
bool zone_data_toggle_flag(struct zone_data *zone, int flag);

struct reset_com;
struct reset_com *zone_get_cmd(GameWorld *g, zone_rnum zone, int n);

