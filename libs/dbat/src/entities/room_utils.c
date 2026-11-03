#include "dbat/rooms.h"
#include "dbat/room_utils.h"
#include "dbat/utils.h"
#include <stdlib.h>
#include <string.h>
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"

static bool rroom_is_valid(room_rnum rnum);

int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != nullptr; ch = char_get_next_in_room(ch))
    if (!char_is_npc(ch))
      i++;

  return (i);
}

/* Is there a campfire in the room? */
int cook_element(room_rnum room)
{
  struct obj_data *obj, *next_obj;
  int found = false;

  for (obj = world[room].contents; obj; obj = next_obj) {
    next_obj = obj_get_next_content(obj);
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
bool room_is_dark(room_rnum room)
{
  if (!rroom_is_valid(room)) {
    mud_log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (false);
  }

  if (world[room].light)
    return (false);

  if (cook_element(room))
    return (false);

  if (rroom_flagged(room, ROOM_NOINSTANT) && rroom_flagged(room, ROOM_DARK)) {
    return (true);
  }
  if (rroom_flagged(room, ROOM_NOINSTANT) && !rroom_flagged(room, ROOM_DARK)) {
    return (false);
  }

  if (rroom_flagged(room, ROOM_DARK))
    return (true);

  if (rroom_flagged(room, ROOM_INDOORS))
    return (false);

  if (rroom_sect(room) == SECT_INSIDE || rroom_sect(room) == SECT_CITY || rroom_sect(room) == SECT_IMPORTANT || rroom_sect(room) == SECT_SHOP)
    return (false);

  if (rroom_sect(room) == SECT_SPACE)
    return (false);

  if (weather_info.sunlight == SUN_SET)
    return (true);

  if (weather_info.sunlight == SUN_DARK)
    return (true);

  return (false);
}

/* Formerly macros in room_utils.h. */

int rroom_sect(room_rnum room)
{
  return (rroom_is_valid(room) ? world[(room)].sector_type : SECT_INSIDE);
}

bool rroom_sunken(room_rnum room)
{
  return (rroom_get_geffect(room) < 0 || rroom_sect(room) == SECT_UNDERWATER);
}

bool rroom_is_light(room_rnum room)
{
  return (!room_is_dark(room));
}

static bool rroom_is_valid(room_rnum rnum)
{
  return ((rnum) != NOWHERE && (rnum) <= top_of_world);
}

room_vnum rroom_get_vnum(room_rnum rnum)
{
  return ((room_vnum)(rroom_is_valid(rnum) ? world[(rnum)].number : NOWHERE));
}

spec_proc_func rroom_get_spec(room_rnum room)
{
  return (rroom_is_valid(room) ? world[(room)].func : nullptr);
}

bool rroom_planet_zenith(room_rnum room)
{
  return ((rroom_get_vnum(room) >= 3400 && rroom_get_vnum(room) <= 3599) || (rroom_get_vnum(room) >= 62900 && rroom_get_vnum(room) <= 62999) || (rroom_get_vnum(room) == 19600));
}

bool rroom_flagged(room_rnum loc, int flag)
{
  return (IS_SET_AR(rroom_get_room_flags(loc), (flag)));
}

bool room_exit_flagged(const struct room_direction_data *ex, bitvector_t flag)
{
  return (IS_SET((ex)->exit_info, (flag)));
}

/* Field accessors (formerly accessor macros). */

int rroom_get_dmg(room_rnum room)
{
  return world[room].dmg;
}

void rroom_set_dmg(room_rnum room, int v)
{
  world[room].dmg = v;
}

int rroom_get_geffect(room_rnum room)
{
  return world[room].geffect;
}

void rroom_set_geffect(room_rnum room, int v)
{
  world[room].geffect = v;
}

int rroom_get_gravity(room_rnum room)
{
  return world[room].gravity;
}

void rroom_set_gravity(room_rnum room, int v)
{
  world[room].gravity = v;
}

bitvector_t *rroom_get_room_flags(room_rnum room)
{
  return world[room].room_flags;
}

struct room_direction_data *rroom_get_dir_option(room_rnum room, int i)
{
  return world[room].dir_option[i];
}

void rroom_set_dir_option(room_rnum room, int i, struct room_direction_data *v)
{
  world[room].dir_option[i] = v;
}

struct room_direction_data *room_get_dir_option(const struct room_data *room, int i)
{
  return room->dir_option[i];
}

void room_set_dir_option(struct room_data *room, int i, struct room_direction_data *v)
{
  room->dir_option[i] = v;
}

/* Room flag setters and toggles; the rroom_ versions take a world[] rnum. */

bool room_flagged(const struct room_data *room, int flag)
{
  return IS_SET_AR(room->room_flags, flag) != 0;
}

void room_set_flag(struct room_data *room, int flag, bool on)
{
  if (on)
    SET_BIT_AR(room->room_flags, flag);
  else
    REMOVE_BIT_AR(room->room_flags, flag);
}

bool room_toggle_flag(struct room_data *room, int flag)
{
  return (TOGGLE_BIT_AR(room->room_flags, flag) & Q_BIT(flag)) != 0;
}

void rroom_set_flag(room_rnum room, int flag, bool on)
{
  room_set_flag(&world[room], flag, on);
}

room_rnum rroom_get_exit_to_room(room_rnum room, int dir)
{
  return world[room].dir_option[dir] ? world[room].dir_option[dir]->to_room : NOWHERE;
}

struct obj_data *room_get_contents(const struct room_data *room)
{
  return room->contents;
}

void room_set_contents(struct room_data *room, struct obj_data *v)
{
  room->contents = v;
}

struct obj_data *rroom_get_contents(room_rnum room)
{
  return world[room].contents;
}

void rroom_set_contents(room_rnum room, struct obj_data *v)
{
  world[room].contents = v;
}

char *room_get_description(const struct room_data *room)
{
  return room->description;
}

void room_set_description(struct room_data *room, char *v)
{
  room->description = v;
}

char *rroom_get_description(room_rnum room)
{
  return world[room].description;
}

void rroom_set_description(room_rnum room, char *v)
{
  world[room].description = v;
}

int room_get_dmg(const struct room_data *room)
{
  return room->dmg;
}

struct extra_descr_data *room_get_ex_description(const struct room_data *room)
{
  return room->ex_description;
}

void room_set_ex_description(struct room_data *room, struct extra_descr_data *v)
{
  room->ex_description = v;
}

struct extra_descr_data *rroom_get_ex_description(room_rnum room)
{
  return world[room].ex_description;
}

void rroom_set_ex_description(room_rnum room, struct extra_descr_data *v)
{
  world[room].ex_description = v;
}

spec_proc_func room_get_func(const struct room_data *room)
{
  return room->func;
}

void rroom_set_func(room_rnum room, spec_proc_func v)
{
  world[room].func = v;
}

int room_get_geffect(const struct room_data *room)
{
  return room->geffect;
}

int room_get_gravity(const struct room_data *room)
{
  return room->gravity;
}

int8_t rroom_get_light(room_rnum room)
{
  return world[room].light;
}

void rroom_set_light(room_rnum room, int8_t v)
{
  world[room].light = v;
}

char *room_get_name(const struct room_data *room)
{
  return room->name;
}

void room_set_name(struct room_data *room, char *v)
{
  room->name = v;
}

char *rroom_get_name(room_rnum room)
{
  return world[room].name;
}

void rroom_set_name(room_rnum room, char *v)
{
  world[room].name = v;
}

room_vnum room_get_number(const struct room_data *room)
{
  return room->number;
}

void room_set_number(struct room_data *room, room_vnum v)
{
  room->number = v;
}

void rroom_set_number(room_rnum room, room_vnum v)
{
  world[room].number = v;
}

struct char_data *room_get_people(const struct room_data *room)
{
  return room->people;
}

void room_set_people(struct room_data *room, struct char_data *v)
{
  room->people = v;
}

struct char_data *rroom_get_people(room_rnum room)
{
  return world[room].people;
}

void rroom_set_people(room_rnum room, struct char_data *v)
{
  world[room].people = v;
}

struct trig_proto_list *room_get_proto_script(const struct room_data *room)
{
  return room->proto_script;
}

void room_set_proto_script(struct room_data *room, struct trig_proto_list *v)
{
  room->proto_script = v;
}

struct trig_proto_list *rroom_get_proto_script(room_rnum room)
{
  return world[room].proto_script;
}

void rroom_set_proto_script(room_rnum room, struct trig_proto_list *v)
{
  world[room].proto_script = v;
}

bitvector_t *room_get_room_flags(struct room_data *room)
{
  return room->room_flags;
}

struct script_data *room_get_script(const struct room_data *room)
{
  return room->script;
}

void room_set_script(struct room_data *room, struct script_data *v)
{
  room->script = v;
}

struct script_data *rroom_get_script(room_rnum room)
{
  return world[room].script;
}

int room_get_sector_type(const struct room_data *room)
{
  return room->sector_type;
}

void room_set_sector_type(struct room_data *room, int v)
{
  room->sector_type = v;
}

void rroom_set_sector_type(room_rnum room, int v)
{
  world[room].sector_type = v;
}

int rroom_get_timed(room_rnum room)
{
  return world[room].timed;
}

void rroom_set_timed(room_rnum room, int v)
{
  world[room].timed = v;
}

zone_rnum room_get_zone(const struct room_data *room)
{
  return room->zone;
}

void room_set_zone(struct room_data *room, zone_rnum v)
{
  room->zone = v;
}

zone_rnum rroom_get_zone(room_rnum room)
{
  return world[room].zone;
}

void rroom_set_zone(room_rnum room, zone_rnum v)
{
  world[room].zone = v;
}

struct script_data **room_script_slot(struct room_data *room)
{
  return &room->script;
}

struct script_data **rroom_script_slot(room_rnum room)
{
  return &world[room].script;
}

struct extra_descr_data **room_ex_description_slot(struct room_data *room)
{
  return &room->ex_description;
}

char **room_description_slot(struct room_data *room)
{
  return &room->description;
}

struct obj_data **rroom_contents_slot(room_rnum room)
{
  return &world[room].contents;
}

struct char_data **rroom_people_slot(room_rnum room)
{
  return &world[room].people;
}
