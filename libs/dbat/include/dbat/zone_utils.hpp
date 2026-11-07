#pragma once
#include "dbat/flags.hpp"
#include "dbat/zones.hpp"

int zone_get_min_level(GameWorld *g, struct zone_data *zone);
int zone_get_max_level(GameWorld *g, struct zone_data *zone);
bool zone_flagged(GameWorld *g, struct zone_data *rnum, int flag);
/* The same on a zone_data pointer, e.g. an OLC working copy. */
bool zone_data_toggle_flag(struct zone_data *zone, int flag);

/* A zone's place in vnum order, from 0, -1 if it isn't in the world. */
int zone_place(GameWorld *g, const struct zone_data *zone);
struct reset_com;
struct reset_com *zone_get_cmd(GameWorld *g, struct zone_data *zone, int n);
