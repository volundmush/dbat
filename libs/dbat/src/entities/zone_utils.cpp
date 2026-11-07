#include "dbat/zone_utils.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"

/* Formerly macros in zone_utils.hpp. */

bool zone_flagged(GameWorld *g, struct zone_data *rnum, int flag)
{
  return (IS_SET_AR(((rnum))->zone_flags, flag));
}

/* Field accessors (formerly accessor macros). */

int zone_get_min_level(GameWorld *g, struct zone_data *zone)
{
  return zone->min_level;
}

int zone_get_max_level(GameWorld *g, struct zone_data *zone)
{
  return zone->max_level;
}

bool zone_data_toggle_flag(struct zone_data *zone, int flag)
{
  return (TOGGLE_BIT_AR(zone->zone_flags, flag) & Q_BIT(flag)) != 0;
}

struct reset_com *zone_get_cmd(GameWorld *g, struct zone_data *zone, int n)
{
  return &zone->cmd[n];
}

int zone_place(GameWorld *g, const struct zone_data *zone)
{
  int place = 0;

  for (struct zone_data *z : g->zones.all()) {
    if (z == zone)
      return place;
    place++;
  }
  return -1;
}
