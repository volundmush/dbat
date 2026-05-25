#include "dbat/db/rooms.h"
#include "dbat/db/room_utils.h"
#include "dbat/game/utils.h"

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
bool cook_element(struct room_data *room) {
 struct obj_data *obj, *next_obj;
 int found = FALSE;

 for (obj = room->contents; obj; obj = next_obj) {
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

  struct room_data* rm = &world[room];

  if (rm->light)
    return (FALSE);

  if (cook_element(rm))
   return (FALSE);

  if (room_flagged(rm, ROOM_NOINSTANT) && room_flagged(rm, ROOM_DARK)) {
   return (TRUE);
  }
  if (room_flagged(rm, ROOM_NOINSTANT) && !room_flagged(rm, ROOM_DARK)) {
	return (FALSE);
 }

  if (room_flagged(rm, ROOM_DARK))
    return (TRUE);

  if (room_flagged(rm, ROOM_INDOORS))
    return (FALSE);
  
  int sec = room_sector_type_get(rm);

  if (sec == SECT_INSIDE || sec == SECT_CITY || sec == SECT_IMPORTANT || sec == SECT_SHOP)
    return (FALSE);

  if (sec == SECT_SPACE)
    return (FALSE);

  if (weather_info.sunlight == SUN_SET)
    return (TRUE);

  if (weather_info.sunlight == SUN_DARK)
    return (TRUE);

  return (FALSE);
}

bool room_is_sunken(struct room_data* room) {
 if(room_geffect_get(room) < 0) return true;
 if(room_sector_type_get(room) == SECT_UNDERWATER) return true;
 return false;
}