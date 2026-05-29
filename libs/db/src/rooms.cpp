#include "dbat/db/rooms.h"


/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum)
{
  return room_by_id(vnum) ? vnum : NOWHERE;
}

room_vnum room_vnum_check(room_vnum vnum)
{
    return room_by_id(vnum) ? vnum : NOWHERE;
}
