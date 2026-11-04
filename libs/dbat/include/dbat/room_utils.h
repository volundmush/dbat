#pragma once
#include "consts/types.h"
#include "flags.h"
#include "dbat/rooms.h"

int rroom_sect(GameWorld *g, room_rnum room);
int rroom_get_dmg(GameWorld *g, room_rnum room);
void rroom_set_dmg(GameWorld *g, room_rnum room, int v);
int rroom_get_geffect(GameWorld *g, room_rnum room);
void rroom_set_geffect(GameWorld *g, room_rnum room, int v);
int rroom_get_gravity(GameWorld *g, room_rnum room);
void rroom_set_gravity(GameWorld *g, room_rnum room, int v);
bool rroom_sunken(GameWorld *g, room_rnum room);

bool rroom_is_light(GameWorld *g, room_rnum room);

room_vnum rroom_get_vnum(GameWorld *g, room_rnum rnum);
spec_proc_func rroom_get_spec(GameWorld *g, room_rnum room);

/* Minor Planet Defines */
bool rroom_planet_zenith(GameWorld *g, room_rnum room);

bitvector_t *rroom_get_room_flags(GameWorld *g, room_rnum room);
bool rroom_flagged(GameWorld *g, room_rnum loc, int flag);
void rroom_set_flag(GameWorld *g, room_rnum room, int flag, bool on);
bool room_flagged(const struct room_data *room, int flag);
void room_set_flag(struct room_data *room, int flag, bool on);
bool room_toggle_flag(struct room_data *room, int flag);
bool room_exit_flagged(const struct room_direction_data *ex, bitvector_t flag);

struct room_direction_data *rroom_get_dir_option(GameWorld *g, room_rnum room, int i);
void rroom_set_dir_option(GameWorld *g, room_rnum room, int i, struct room_direction_data *v);
struct room_direction_data *room_get_dir_option(const struct room_data *room, int i);
void room_set_dir_option(struct room_data *room, int i, struct room_direction_data *v);


int num_pc_in_room(struct room_data *room);
int cook_element(GameWorld *g, room_rnum room);
bool room_is_dark(GameWorld *g, room_rnum room);

/* The room an exit leads to, or NOWHERE if there is no such exit. */
room_rnum rroom_get_exit_to_room(GameWorld *g, room_rnum room, int dir);

/* Field accessors */
struct obj_data *room_get_contents(const struct room_data *room);
void room_set_contents(struct room_data *room, struct obj_data *v);
struct obj_data *rroom_get_contents(GameWorld *g, room_rnum room);
void rroom_set_contents(GameWorld *g, room_rnum room, struct obj_data *v);
char *room_get_description(const struct room_data *room);
void room_set_description(struct room_data *room, char *v);
char *rroom_get_description(GameWorld *g, room_rnum room);
void rroom_set_description(GameWorld *g, room_rnum room, char *v);
int room_get_dmg(const struct room_data *room);
struct extra_descr_data *room_get_ex_description(const struct room_data *room);
void room_set_ex_description(struct room_data *room, struct extra_descr_data *v);
struct extra_descr_data *rroom_get_ex_description(GameWorld *g, room_rnum room);
void rroom_set_ex_description(GameWorld *g, room_rnum room, struct extra_descr_data *v);
spec_proc_func room_get_func(const struct room_data *room);
void rroom_set_func(GameWorld *g, room_rnum room, spec_proc_func v);
int room_get_geffect(const struct room_data *room);
int room_get_gravity(const struct room_data *room);
int8_t rroom_get_light(GameWorld *g, room_rnum room);
void rroom_set_light(GameWorld *g, room_rnum room, int8_t v);
char *room_get_name(const struct room_data *room);
void room_set_name(struct room_data *room, char *v);
char *rroom_get_name(GameWorld *g, room_rnum room);
void rroom_set_name(GameWorld *g, room_rnum room, char *v);
room_vnum room_get_number(const struct room_data *room);
void room_set_number(struct room_data *room, room_vnum v);
void rroom_set_number(GameWorld *g, room_rnum room, room_vnum v);
struct char_data *room_get_people(const struct room_data *room);
void room_set_people(struct room_data *room, struct char_data *v);
struct char_data *rroom_get_people(GameWorld *g, room_rnum room);
void rroom_set_people(GameWorld *g, room_rnum room, struct char_data *v);
struct trig_proto_list *room_get_proto_script(const struct room_data *room);
void room_set_proto_script(struct room_data *room, struct trig_proto_list *v);
struct trig_proto_list *rroom_get_proto_script(GameWorld *g, room_rnum room);
void rroom_set_proto_script(GameWorld *g, room_rnum room, struct trig_proto_list *v);
bitvector_t *room_get_room_flags(struct room_data *room);
struct script_data *room_get_script(const struct room_data *room);
void room_set_script(struct room_data *room, struct script_data *v);
struct script_data *rroom_get_script(GameWorld *g, room_rnum room);
int room_get_sector_type(const struct room_data *room);
void room_set_sector_type(struct room_data *room, int v);
void rroom_set_sector_type(GameWorld *g, room_rnum room, int v);
int rroom_get_timed(GameWorld *g, room_rnum room);
void rroom_set_timed(GameWorld *g, room_rnum room, int v);
zone_rnum room_get_zone(const struct room_data *room);
void room_set_zone(struct room_data *room, zone_rnum v);
zone_rnum rroom_get_zone(GameWorld *g, room_rnum room);
void rroom_set_zone(GameWorld *g, room_rnum room, zone_rnum v);

/* A field's address, for code that fills the field in later or unlinks through it */
struct script_data **room_script_slot(struct room_data *room);
struct script_data **rroom_script_slot(GameWorld *g, room_rnum room);
struct extra_descr_data **room_ex_description_slot(struct room_data *room);
char **room_description_slot(struct room_data *room);
struct obj_data **rroom_contents_slot(GameWorld *g, room_rnum room);
struct char_data **rroom_people_slot(GameWorld *g, room_rnum room);
