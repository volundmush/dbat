/* Zones: finding one by its number */
#include "dbat/zones.hpp"
#include "dbat/game_world.hpp"

/* The zone numbered vnum, or null. */
struct zone_data *find_zone(GameWorld *g, zone_vnum vnum)
{
  return g->zones.find(vnum);
}
