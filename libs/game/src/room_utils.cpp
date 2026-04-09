#include "dbat/db/rooms.h"
#include "dbat/db/room_utils.h"
#include "dbat/game/room_utils.h"


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return (i);
}

/* Is there a campfire in the room? */
int cook_element(room_rnum room) {
 struct obj_data *obj, *next_obj;
 int found = FALSE;

 for (obj = world[room].contents; obj; obj = next_obj) {
  next_obj = obj->next_content;
  if (GET_OBJ_TYPE(obj) == ITEM_CAMPFIRE) {
   found = 1;
  } else if (GET_OBJ_VNUM(obj) == 19093) {
   found = 2;
  }
 }
 return (found);
}

/* Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.  */
int room_is_dark(room_rnum room)
{
  if (!VALID_ROOM_RNUM(room)) {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  if (world[room].light)
    return (FALSE);

  if (cook_element(room))
   return (FALSE);

  if (ROOM_FLAGGED(room, ROOM_NOINSTANT) && ROOM_FLAGGED(room, ROOM_DARK)) {
   return (TRUE);
  }
  if (ROOM_FLAGGED(room, ROOM_NOINSTANT) && !ROOM_FLAGGED(room, ROOM_DARK)) {
	return (FALSE);
 }

  if (ROOM_FLAGGED(room, ROOM_DARK))
    return (TRUE);

  if (ROOM_FLAGGED(room, ROOM_INDOORS))
    return (FALSE);

  if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY || SECT(room) == SECT_IMPORTANT || SECT(room) == SECT_SHOP)
    return (FALSE);

  if (SECT(room) == SECT_SPACE)
    return (FALSE);

  if (weather_info.sunlight == SUN_SET)
    return (TRUE);

  if (weather_info.sunlight == SUN_DARK)
    return (TRUE);

  return (FALSE);
}