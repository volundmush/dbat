#include "dbat/zone_utils.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/game_world.h"

/* Formerly macros in zone_utils.h. */

bool zone_flagged(GameWorld *g, zone_rnum rnum, int flag)
{
  return (IS_SET_AR(g->zones[(rnum)].zone_flags, flag));
}

/* Field accessors (formerly accessor macros). */

int zone_get_min_level(GameWorld *g, zone_rnum zone)
{
  return g->zones[zone].min_level;
}

int zone_get_max_level(GameWorld *g, zone_rnum zone)
{
  return g->zones[zone].max_level;
}

bool zone_data_toggle_flag(struct zone_data *zone, int flag)
{
  return (TOGGLE_BIT_AR(zone->zone_flags, flag) & Q_BIT(flag)) != 0;
}

struct reset_com *zone_get_cmd(GameWorld *g, zone_rnum zone, int n)
{
  return &g->zones[zone].cmd[n];
}
