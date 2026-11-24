#include "dbat/zone_utils.hpp"
#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"

/* Formerly macros in zone_utils.hpp. */

bool zone_flagged(GameWorld *g, struct zone_data *rnum, zone_flag flag)
{
  return rnum->zone_flags.test(flag);
}

void zone_set_flag(struct zone_data *zone, zone_flag flag, bool on)
{
  zone->zone_flags.set(flag, on);
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

bool zone_data_toggle_flag(struct zone_data *zone, zone_flag flag)
{
  zone->zone_flags.toggle(flag);
  return zone->zone_flags.test(flag);
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
