#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "rooms.h"

struct char_data;
struct obj_data;

bool room_is_valid(const struct room_data *room);

room_vnum room_get_vnum(const struct room_data *room);
zone_rnum room_get_zone(const struct room_data *room);

int room_get_sector_type(const struct room_data *room);
void room_set_sector_type(struct room_data *room, int type);

const char *room_get_name(const struct room_data *room);
const char *room_get_description(const struct room_data *room);

void room_set_name(struct room_data *room, const char *name);
void room_set_description(struct room_data *room, const char *desc);

struct extra_descr_data *room_get_ex_description(const struct room_data *room);
void room_set_ex_description(struct room_data *room, struct extra_descr_data *ex_desc);

struct room_direction_data *room_get_dir_option(const struct room_data *room, int dir);
void room_set_dir_option(struct room_data *room, int dir, struct room_direction_data *dir_option);

bool room_flagged(const struct room_data *room, int flag);
void room_set_flag(struct room_data *room, int flag, bool value);
void room_remove_flag(struct room_data *room, int flag);

int8_t room_get_light(const struct room_data *room);
void room_set_light(struct room_data *room, int8_t light);

struct obj_data *room_get_contents(const struct room_data *room);
void room_set_contents(struct room_data *room, struct obj_data *contents);

struct char_data *room_get_people(const struct room_data *room);
void room_set_people(struct room_data *room, struct char_data *people);

int room_get_timed(const struct room_data *room);
void room_set_timed(struct room_data *room, int timed);

int room_get_dmg(const struct room_data *room);
void room_set_dmg(struct room_data *room, int dmg);

int room_get_gravity(const struct room_data *room);
void room_set_gravity(struct room_data *room, int gravity);

int room_get_geffect(const struct room_data *room);
void room_set_geffect(struct room_data *room, int geffect);



#define SECT(room)	(VALID_ROOM_RNUM(room) ? \
				world[(room)].sector_type : SECT_INSIDE)
#define ROOM_DAMAGE(room)   (world[(room)].dmg)
#define ROOM_EFFECT(room)   (world[(room)].geffect)
#define ROOM_GRAVITY(room)  (world[(room)].gravity)
#define SUNKEN(room)    (ROOM_EFFECT(room) < 0 || SECT(room) == SECT_UNDERWATER)

#define IS_DARK(room)	room_is_dark((room))
#define IS_LIGHT(room)  (!IS_DARK(room))

#define VALID_ROOM_RNUM(rnum)	((rnum) != NOWHERE && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) \
	(VALID_ROOM_RNUM(room) ? world[(room)].func : NULL)

/* Minor Planet Defines */
#define PLANET_ZENITH(room) ((GET_ROOM_VNUM(room) >= 3400 && GET_ROOM_VNUM(room) <= 3599) || (GET_ROOM_VNUM(room) >= 62900 && GET_ROOM_VNUM(room) <= 62999) || \
				(GET_ROOM_VNUM(room) == 19600))
    
#define ROOM_FLAGS(loc)	(world[(loc)].room_flags)
#define ROOM_FLAGGED(loc, flag) (IS_SET_AR(ROOM_FLAGS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))

#define W_EXIT(room, num)     (world[(room)].dir_option[(num)])
#define R_EXIT(room, num)     ((room)->dir_option[(num)])