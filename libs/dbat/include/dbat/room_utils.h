#pragma once
#include "consts/types.h"
#include "flags.h"

int rroom_sect(room_rnum room);
int rroom_get_dmg(room_rnum room);
void rroom_set_dmg(room_rnum room, int v);
int rroom_get_geffect(room_rnum room);
void rroom_set_geffect(room_rnum room, int v);
int rroom_get_gravity(room_rnum room);
void rroom_set_gravity(room_rnum room, int v);
bool rroom_sunken(room_rnum room);

bool rroom_is_light(room_rnum room);

bool rroom_is_valid(room_rnum rnum);
room_vnum rroom_get_vnum(room_rnum rnum);
spec_proc_func rroom_get_spec(room_rnum room);

/* Minor Planet Defines */
bool rroom_planet_zenith(room_rnum room);
    
bitvector_t *rroom_get_room_flags(room_rnum room);
bool rroom_flagged(room_rnum loc, int flag);
bool room_exit_flagged(const struct room_direction_data *ex, bitvector_t flag);

struct room_direction_data *rroom_get_dir_option(room_rnum room, int i);
void rroom_set_dir_option(room_rnum room, int i, struct room_direction_data *v);
struct room_direction_data *room_get_dir_option(const struct room_data *room, int i);
void room_set_dir_option(struct room_data *room, int i, struct room_direction_data *v);

#include "dbat/consts/types.h"
#include "dbat/rooms.h"

int num_pc_in_room(struct room_data *room);
int cook_element(room_rnum room);
int room_is_dark(room_rnum room);

