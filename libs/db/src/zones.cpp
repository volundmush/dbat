#include "dbat/db/zones.h"

struct zone_data *zone_table;
zone_rnum top_of_zone_table;
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

/* returns the real number of the zone with given virtual number */
zone_rnum real_zone(zone_vnum vnum)
{
  zone_rnum bot, top, mid, last_top;

  bot = 0;
  top = top_of_zone_table;

  /* perform binary search on zone-table */
  for (;;) {
    last_top = top;
    mid = (bot + top) / 2;

    if ((zone_table + mid)->number == vnum)
      return (mid);
    if (bot >= top)
      return (NOWHERE);
    if ((zone_table + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;

    if (top > last_top)
      return NOWHERE;
  }
}

struct zone_data *zone_by_id(zone_vnum vnum)
{
  return zone_get(vnum);
}
