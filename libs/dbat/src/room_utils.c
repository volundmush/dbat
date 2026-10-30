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
  if (GET_OBJ_TYPE(obj) == ITEM_CAMPFIRE) {
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

bool room_is_valid(const struct room_data *room) {
    return room != NULL;
}

room_vnum room_get_vnum(const struct room_data *room) {
    if (!room_is_valid(room)) return NOWHERE;
    return room->number;
}

zone_rnum room_get_zone(const struct room_data *room) {
    if (!room_is_valid(room)) return -1;
    return room->zone;
}

int room_get_sector_type(const struct room_data *room) {
    if (!room_is_valid(room)) return 0;
    return room->sector_type;
}

void room_set_sector_type(struct room_data *room, int type) {
    if (!room_is_valid(room)) return;
    room->sector_type = type;
}

const char *room_get_name(const struct room_data *room) {
    if (!room_is_valid(room)) return NULL;
    return room->name ? room->name : "";
}

const char *room_get_description(const struct room_data *room) {
    if (!room_is_valid(room)) return NULL;
    return room->description ? room->description : "";
}

void room_set_name(struct room_data *room, const char *name) {
    if (!room_is_valid(room) || !name) return;
    if (room->name) free(room->name);
    room->name = strdup(name);
}

void room_set_description(struct room_data *room, const char *desc) {
    if (!room_is_valid(room) || !desc) return;
    if (room->description) free(room->description);
    room->description = strdup(desc);
}

struct extra_descr_data *room_get_ex_description(const struct room_data *room) {
    if (!room_is_valid(room)) return NULL;
    return room->ex_description;
}

void room_set_ex_description(struct room_data *room, struct extra_descr_data *ex_desc) {
    if (!room_is_valid(room)) return;
    room->ex_description = ex_desc;
}

struct room_direction_data *room_get_dir_option(const struct room_data *room, int dir) {
    if (!room_is_valid(room) || dir < 0 || dir >= NUM_OF_DIRS) return NULL;
    return room->dir_option[dir];
}

void room_set_dir_option(struct room_data *room, int dir, struct room_direction_data *dir_option) {
    if (!room_is_valid(room) || dir < 0 || dir >= NUM_OF_DIRS) return;
    room->dir_option[dir] = dir_option;
}

bool room_flagged(const struct room_data *room, int flag) {
    if (!room_is_valid(room)) return false;
    return (room->room_flags[0] & flag) != 0;
}

void room_set_flag(struct room_data *room, int flag, bool value) {
    if (!room_is_valid(room)) return;
    if (value) {
        room->room_flags[0] |= flag;
    } else {
        room->room_flags[0] &= ~flag;
    }
}

void room_remove_flag(struct room_data *room, int flag) {
    if (!room_is_valid(room)) return;
    room->room_flags[0] &= ~flag;
}

int8_t room_get_light(const struct room_data *room) {
    if (!room_is_valid(room)) return 0;
    return room->light;
}

void room_set_light(struct room_data *room, int8_t light) {
    if (!room_is_valid(room)) return;
    room->light = light;
}

struct obj_data *room_get_contents(const struct room_data *room) {
    if (!room_is_valid(room)) return NULL;
    return room->contents;
}

void room_set_contents(struct room_data *room, struct obj_data *contents) {
    if (!room_is_valid(room)) return;
    room->contents = contents;
}

struct char_data *room_get_people(const struct room_data *room) {
    if (!room_is_valid(room)) return NULL;
    return room->people;
}

void room_set_people(struct room_data *room, struct char_data *people) {
    if (!room_is_valid(room)) return;
    room->people = people;
}

int room_get_timed(const struct room_data *room) {
    if (!room_is_valid(room)) return 0;
    return room->timed;
}

void room_set_timed(struct room_data *room, int timed) {
    if (!room_is_valid(room)) return;
    room->timed = timed;
}

int room_get_dmg(const struct room_data *room) {
    if (!room_is_valid(room)) return 0;
    return room->dmg;
}

void room_set_dmg(struct room_data *room, int dmg) {
    if (!room_is_valid(room)) return;
    room->dmg = dmg;
}

int room_get_gravity(const struct room_data *room) {
    if (!room_is_valid(room)) return 0;
    return room->gravity;
}

void room_set_gravity(struct room_data *room, int gravity) {
    if (!room_is_valid(room)) return;
    room->gravity = gravity;
}

int room_get_geffect(const struct room_data *room) {
    if (!room_is_valid(room)) return 0;
    return room->geffect;
}

void room_set_geffect(struct room_data *room, int geffect) {
    if (!room_is_valid(room)) return;
    room->geffect = geffect;
}

/* Formerly macros in room_utils.h. */

int rroom_sect(room_rnum room) {
    return (rroom_is_valid(room) ? world[(room)].sector_type : SECT_INSIDE);
}

bool rroom_sunken(room_rnum room) {
    return (ROOM_EFFECT(room) < 0 || rroom_sect(room) == SECT_UNDERWATER);
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
    return (IS_SET_AR(ROOM_FLAGS(loc), (flag)));
}

bool room_exit_flagged(const struct room_direction_data *ex, bitvector_t flag) {
    return (IS_SET((ex)->exit_info, (flag)));
}
