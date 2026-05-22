#include "dbat/db/rooms.h"

struct room_data *world;
room_rnum top_of_world;
struct htree_node *room_htree;

/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum)
{
  room_rnum bot, top, mid, i, last_top;

  i = htree_find(room_htree, vnum);

  if (i != NOWHERE && world[i].number == vnum)
    return i;
  else {
    bot = 0;
    top = top_of_world;

    /* perform binary search on world-table */
    for (;;) {
      last_top = top;
      mid = (bot + top) / 2;

      if ((world + mid)->number == vnum) {
        htree_add(room_htree, vnum, mid);
        return (mid);
      }
      if (bot >= top)
        return (NOWHERE);
      if ((world + mid)->number > vnum)
        top = mid - 1;
      else
        bot = mid + 1;

      if (top > last_top)
        return NOWHERE;
    }
  }
}

struct room_data *room_by_id(room_vnum vnum)
{
  room_rnum rnum = real_room(vnum);

  if (rnum == NOWHERE || !world)
    return nullptr;

  return &world[rnum];
}
