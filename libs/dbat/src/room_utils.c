#include "dbat/rooms.h"
#include "dbat/room_utils.h"
#include "dbat/utils.h"
#include <stdlib.h>
#include <string.h>

int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!char_is_npc(ch))
      i++;

  return (i);
}

/* Is there a campfire in the room? */
int cook_element(room_rnum room) {
 struct obj_data *obj, *next_obj;
 int found = FALSE;

 for (obj = world[room].contents; obj; obj = next_obj) {
  next_obj = obj->next_content;
  if (obj_get_type_flag(obj) == ITEM_CAMPFIRE) {
   found = 1;
  } else if (obj_get_vnum(obj) == 19093) {
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
  if (!rroom_is_valid(room)) {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  if (world[room].light)
    return (FALSE);

  if (cook_element(room))
   return (FALSE);

  if (rroom_flagged(room, ROOM_NOINSTANT) && rroom_flagged(room, ROOM_DARK)) {
   return (TRUE);
  }
  if (rroom_flagged(room, ROOM_NOINSTANT) && !rroom_flagged(room, ROOM_DARK)) {
	return (FALSE);
 }

  if (rroom_flagged(room, ROOM_DARK))
    return (TRUE);

  if (rroom_flagged(room, ROOM_INDOORS))
    return (FALSE);

  if (rroom_sect(room) == SECT_INSIDE || rroom_sect(room) == SECT_CITY || rroom_sect(room) == SECT_IMPORTANT || rroom_sect(room) == SECT_SHOP)
    return (FALSE);

  if (rroom_sect(room) == SECT_SPACE)
    return (FALSE);

  if (weather_info.sunlight == SUN_SET)
    return (TRUE);

  if (weather_info.sunlight == SUN_DARK)
    return (TRUE);

  return (FALSE);
}

/* Formerly macros in room_utils.h. */

int rroom_sect(room_rnum room) {
    return (rroom_is_valid(room) ? world[(room)].sector_type : SECT_INSIDE);
}

bool rroom_sunken(room_rnum room) {
    return (rroom_get_geffect(room) < 0 || rroom_sect(room) == SECT_UNDERWATER);
}

bool rroom_is_light(room_rnum room) {
    return (!room_is_dark(room));
}

bool rroom_is_valid(room_rnum rnum) {
    return ((rnum) != NOWHERE && (rnum) <= top_of_world);
}

room_vnum rroom_get_vnum(room_rnum rnum) {
    return ((room_vnum)(rroom_is_valid(rnum) ? world[(rnum)].number : NOWHERE));
}

spec_proc_func rroom_get_spec(room_rnum room) {
    return (rroom_is_valid(room) ? world[(room)].func : NULL);
}

bool rroom_planet_zenith(room_rnum room) {
    return ((rroom_get_vnum(room) >= 3400 && rroom_get_vnum(room) <= 3599)
           || (rroom_get_vnum(room) >= 62900 && rroom_get_vnum(room) <= 62999)
           || (rroom_get_vnum(room) == 19600));
}

bool rroom_flagged(room_rnum loc, int flag) {
    return (IS_SET_AR(rroom_get_room_flags(loc), (flag)));
}

bool room_exit_flagged(const struct room_direction_data *ex, bitvector_t flag) {
    return (IS_SET((ex)->exit_info, (flag)));
}

/* Field accessors (formerly accessor macros). */

int rroom_get_dmg(room_rnum room) {
    return world[room].dmg;
}

void rroom_set_dmg(room_rnum room, int v) {
    world[room].dmg = v;
}

int rroom_get_geffect(room_rnum room) {
    return world[room].geffect;
}

void rroom_set_geffect(room_rnum room, int v) {
    world[room].geffect = v;
}

int rroom_get_gravity(room_rnum room) {
    return world[room].gravity;
}

void rroom_set_gravity(room_rnum room, int v) {
    world[room].gravity = v;
}

bitvector_t *rroom_get_room_flags(room_rnum room) {
    return world[room].room_flags;
}

struct room_direction_data *rroom_get_dir_option(room_rnum room, int i) {
    return world[room].dir_option[i];
}

void rroom_set_dir_option(room_rnum room, int i, struct room_direction_data *v) {
    world[room].dir_option[i] = v;
}

struct room_direction_data *room_get_dir_option(const struct room_data *room, int i) {
    return room->dir_option[i];
}

void room_set_dir_option(struct room_data *room, int i, struct room_direction_data *v) {
    room->dir_option[i] = v;
}
