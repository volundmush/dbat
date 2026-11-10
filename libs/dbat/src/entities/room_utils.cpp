#include <cstdlib>
#include <cstring>

#include "dbat/iterate.hpp"
#include "dbat/rooms.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static bool room_is_valid(GameWorld *g, room_vnum rnum);

int num_pc_in_room(GameWorld *g, struct room_data *room)
{
  int i = 0;

  for (char_data *ch : room_people(g, room))
    if (!char_is_npc(ch))
      i++;

  return (i);
}

/* Is there a campfire in the room? */
int cook_element(GameWorld *g, room_vnum room)
{
  int found = false;

  for (obj_data *obj : room_contents(g, room)) {
    if (obj_get_type_flag(obj) == ITEM_CAMPFIRE) {
      found = 1;
    } else if (obj_get_vnum(g, obj) == 19093) {
      found = 2;
    }
  }
  return (found);
}

/* Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.  */
bool room_is_dark(GameWorld *g, room_vnum room)
{
  if (!room_is_valid(g, room)) {
    mud_log_printf("room_is_dark: Invalid room %d.", room);
    return (false);
  }

  if (room_of(g, room)->light)
    return (false);

  if (cook_element(g, room))
    return (false);

  if (room_flagged(g, room, ROOM_NOINSTANT) && room_flagged(g, room, ROOM_DARK)) {
    return (true);
  }
  if (room_flagged(g, room, ROOM_NOINSTANT) && !room_flagged(g, room, ROOM_DARK)) {
    return (false);
  }

  if (room_flagged(g, room, ROOM_DARK))
    return (true);

  if (room_flagged(g, room, ROOM_INDOORS))
    return (false);

  if (room_sect(g, room) == SECT_INSIDE || room_sect(g, room) == SECT_CITY || room_sect(g, room) == SECT_IMPORTANT || room_sect(g, room) == SECT_SHOP)
    return (false);

  if (room_sect(g, room) == SECT_SPACE)
    return (false);

  if (g->weather_info.sunlight == SUN_SET)
    return (true);

  if (g->weather_info.sunlight == SUN_DARK)
    return (true);

  return (false);
}

/* Formerly macros in room_utils.hpp. */

int room_sect(GameWorld *g, room_vnum room)
{
  return (room_is_valid(g, room) ? room_of(g, (room))->sector_type : SECT_INSIDE);
}

bool room_sunken(GameWorld *g, room_vnum room)
{
  return (room_get_geffect(g, room) < 0 || room_sect(g, room) == SECT_UNDERWATER);
}

bool room_is_light(GameWorld *g, room_vnum room)
{
  return (!room_is_dark(g, room));
}

static bool room_is_valid(GameWorld *g, room_vnum rnum)
{
  return room_of(g, rnum) != nullptr;
}

spec_proc_func room_get_spec(GameWorld *g, room_vnum room)
{
  return (room_is_valid(g, room) ? room_of(g, (room))->func : nullptr);
}

bool room_planet_zenith(GameWorld *g, room_vnum room)
{
  return ((room >= 3400 && room <= 3599) || (room >= 62900 && room <= 62999) || (room == 19600));
}

bool room_flagged(GameWorld *g, room_vnum loc, int flag)
{
  struct room_data *r = room_of(g, loc);
  return r && room_flagged(r, flag);
}

bool room_exit_flagged(const struct room_direction_data *ex, bitvector_t flag)
{
  return (IS_SET((ex)->exit_info, (flag)));
}

/* Field accessors (formerly accessor macros). */

int room_get_dmg(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return r->dmg;
  return {};
}

void room_set_dmg(GameWorld *g, room_vnum room, int v)
{
  if (struct room_data *r = room_of(g, room))
    r->dmg = v;
}

int room_get_geffect(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return r->geffect;
  return {};
}

void room_set_geffect(GameWorld *g, room_vnum room, int v)
{
  if (struct room_data *r = room_of(g, room))
    r->geffect = v;
}

int room_get_gravity(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return r->gravity;
  return {};
}

void room_set_gravity(GameWorld *g, room_vnum room, int v)
{
  if (struct room_data *r = room_of(g, room))
    r->gravity = v;
}

bitvector_t *room_get_room_flags(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return r->room_flags;
  return {};
}

struct room_direction_data *room_get_dir_option(GameWorld *g, room_vnum room, int i)
{
  if (struct room_data *r = room_of(g, room))
    return r->dir_option[i].get();
  return {};
}

void room_set_dir_option(GameWorld *g, room_vnum room, int i, struct room_direction_data *v)
{
  if (struct room_data *r = room_of(g, room))
    r->dir_option[i].adopt(v);
}

struct room_direction_data *room_get_dir_option(const struct room_data *room, int i)
{
  return room->dir_option[i].get();
}

void room_set_dir_option(struct room_data *room, int i, struct room_direction_data *v)
{
  room->dir_option[i].adopt(v);
}

const char *exit_keyword(const struct room_direction_data *exit)
{
  return exit->keyword.empty() ? nullptr : exit->keyword.c_str();
}

const char *exit_description(const struct room_direction_data *exit)
{
  return exit->general_description.empty() ? nullptr : exit->general_description.c_str();
}

/* Room flag setters and toggles, on the room or on its vnum. */

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

void room_set_flag(GameWorld *g, room_vnum room, int flag, bool on)
{
  if (struct room_data *r = room_of(g, room))
    room_set_flag(r, flag, on);
}

room_vnum room_get_exit_to_room(GameWorld *g, room_vnum room, int dir)
{
  struct room_data *r = room_of(g, room);
  return r && r->dir_option[dir] ? r->dir_option[dir]->to_room : NOWHERE;
}

const id_list &room_get_contents(const struct room_data *room)
{
  return room->contents;
}

/* A room that doesn't exist has nothing in it. */
const id_list &room_get_contents(GameWorld *g, room_vnum room)
{
  static const id_list none;
  if (struct room_data *r = room_of(g, room))
    return r->contents;
  return none;
}

const char *room_get_description(const struct room_data *room)
{
  return room->description.empty() ? nullptr : room->description.c_str(); /* null for none, as the files have it */
}

void room_set_description(struct room_data *room, const char *v)
{
  room->description = v ? v : "";
}

void room_set_description(struct room_data *room, const std::string &v)
{
  room->description = v;
}

const char *room_get_description(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return room_get_description(r);
  return nullptr;
}

void room_set_description(GameWorld *g, room_vnum room, const char *v)
{
  if (struct room_data *r = room_of(g, room))
    room_set_description(r, v);
}

void room_set_description(GameWorld *g, room_vnum room, const std::string &v)
{
  if (struct room_data *r = room_of(g, room))
    r->description = v;
}

int room_get_dmg(const struct room_data *room)
{
  return room->dmg;
}

const ex_desc_list &room_get_ex_description(const struct room_data *room)
{
  return room->ex_description;
}

const ex_desc_list &room_get_ex_description(GameWorld *g, room_vnum room)
{
  static const ex_desc_list none;
  if (struct room_data *r = room_of(g, room))
    return r->ex_description;
  return none;
}

spec_proc_func room_get_func(const struct room_data *room)
{
  return room->func;
}

void room_set_func(GameWorld *g, room_vnum room, spec_proc_func v)
{
  if (struct room_data *r = room_of(g, room))
    r->func = v;
}

int room_get_geffect(const struct room_data *room)
{
  return room->geffect;
}

int room_get_gravity(const struct room_data *room)
{
  return room->gravity;
}

int8_t room_get_light(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return r->light;
  return {};
}

void room_set_light(GameWorld *g, room_vnum room, int8_t v)
{
  if (struct room_data *r = room_of(g, room))
    r->light = v;
}

const char *room_get_name(const struct room_data *room)
{
  return room->name.empty() ? nullptr : room->name.c_str(); /* null for none, as the files have it */
}

void room_set_name(struct room_data *room, const char *v)
{
  room->name = v ? v : "";
}

void room_set_name(struct room_data *room, const std::string &v)
{
  room->name = v;
}

const char *room_get_name(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return room_get_name(r);
  return nullptr;
}

void room_set_name(GameWorld *g, room_vnum room, const char *v)
{
  if (struct room_data *r = room_of(g, room))
    room_set_name(r, v);
}

void room_set_name(GameWorld *g, room_vnum room, const std::string &v)
{
  if (struct room_data *r = room_of(g, room))
    r->name = v;
}

room_vnum room_get_number(const struct room_data *room)
{
  return room->number;
}

void room_set_number(struct room_data *room, room_vnum v)
{
  room->number = v;
}

void room_set_number(GameWorld *g, room_vnum room, room_vnum v)
{
  if (struct room_data *r = room_of(g, room))
    r->number = v;
}

const id_list &room_get_people(const struct room_data *room)
{
  return room->people;
}

/* A room that doesn't exist has nothing in it. */
const id_list &room_get_people(GameWorld *g, room_vnum room)
{
  static const id_list none;
  if (struct room_data *r = room_of(g, room))
    return r->people;
  return none;
}

struct trig_proto_list *room_get_proto_script(const struct room_data *room)
{
  return room->proto_script;
}

void room_set_proto_script(struct room_data *room, struct trig_proto_list *v)
{
  room->proto_script = v;
}

struct trig_proto_list *room_get_proto_script(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return r->proto_script;
  return {};
}

void room_set_proto_script(GameWorld *g, room_vnum room, struct trig_proto_list *v)
{
  if (struct room_data *r = room_of(g, room))
    r->proto_script = v;
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

struct script_data *room_get_script(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return r->script;
  return {};
}

int room_get_sector_type(const struct room_data *room)
{
  return room->sector_type;
}

void room_set_sector_type(struct room_data *room, int v)
{
  room->sector_type = v;
}

void room_set_sector_type(GameWorld *g, room_vnum room, int v)
{
  if (struct room_data *r = room_of(g, room))
    r->sector_type = v;
}

int room_get_timed(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return r->timed;
  return {};
}

void room_set_timed(GameWorld *g, room_vnum room, int v)
{
  if (struct room_data *r = room_of(g, room))
    r->timed = v;
}

struct zone_data *room_get_zone(const struct room_data *room)
{
  return room->zone;
}

void room_set_zone(struct room_data *room, struct zone_data *v)
{
  room->zone = v;
}

struct zone_data *room_get_zone(GameWorld *g, room_vnum room)
{
  if (struct room_data *r = room_of(g, room))
    return r->zone;
  return {};
}

void room_set_zone(GameWorld *g, room_vnum room, struct zone_data *v)
{
  if (struct room_data *r = room_of(g, room))
    r->zone = v;
}

struct script_data **room_script_slot(struct room_data *room)
{
  return &room->script;
}

struct script_data **room_script_slot(GameWorld *g, room_vnum room)
{
  struct room_data *r = room_of(g, room);
  return r ? &r->script : nullptr;
}

ex_desc_list *room_ex_description_slot(struct room_data *room)
{
  return &room->ex_description;
}

std::string *room_description_slot(struct room_data *room)
{
  return &room->description;
}

id_list *room_contents_slot(GameWorld *g, room_vnum room)
{
  struct room_data *r = room_of(g, room);
  return r ? &r->contents : nullptr;
}

id_list *room_people_slot(GameWorld *g, room_vnum room)
{
  struct room_data *r = room_of(g, room);
  return r ? &r->people : nullptr;
}

struct room_data *room_of(GameWorld *g, room_vnum vnum)
{
  return g->rooms.find(vnum);
}
