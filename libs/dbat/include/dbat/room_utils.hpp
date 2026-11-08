#pragma once
#include <string>
#include "dbat/extradesc.hpp"
#include "dbat/registry.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/flags.hpp"
#include "dbat/rooms.hpp"

int room_sect(GameWorld *g, room_vnum room);
int room_get_dmg(GameWorld *g, room_vnum room);
void room_set_dmg(GameWorld *g, room_vnum room, int v);
int room_get_geffect(GameWorld *g, room_vnum room);
void room_set_geffect(GameWorld *g, room_vnum room, int v);
int room_get_gravity(GameWorld *g, room_vnum room);
void room_set_gravity(GameWorld *g, room_vnum room, int v);
bool room_sunken(GameWorld *g, room_vnum room);

bool room_is_light(GameWorld *g, room_vnum room);

spec_proc_func room_get_spec(GameWorld *g, room_vnum room);

/* Minor Planet Defines */
bool room_planet_zenith(GameWorld *g, room_vnum room);

bitvector_t *room_get_room_flags(GameWorld *g, room_vnum room);
bool room_flagged(GameWorld *g, room_vnum loc, int flag);
void room_set_flag(GameWorld *g, room_vnum room, int flag, bool on);
bool room_flagged(const struct room_data *room, int flag);
void room_set_flag(struct room_data *room, int flag, bool on);
bool room_toggle_flag(struct room_data *room, int flag);
bool room_exit_flagged(const struct room_direction_data *ex, bitvector_t flag);

struct room_direction_data *room_get_dir_option(GameWorld *g, room_vnum room, int i);
void room_set_dir_option(GameWorld *g, room_vnum room, int i, struct room_direction_data *v); /* as below */
struct room_direction_data *room_get_dir_option(const struct room_data *room, int i);
/* Gives the room the exit v, a new'd one it now owns, or none (null), freeing the old one */
void room_set_dir_option(struct room_data *room, int i, struct room_direction_data *v);
/* An exit's keyword and description, or null for none */
const char *exit_keyword(const struct room_direction_data *exit);
const char *exit_description(const struct room_direction_data *exit);

int num_pc_in_room(GameWorld *g, struct room_data *room);
int cook_element(GameWorld *g, room_vnum room);
bool room_is_dark(GameWorld *g, room_vnum room);

/* The room an exit leads to, or NOWHERE if there is no such exit. */
room_vnum room_get_exit_to_room(GameWorld *g, room_vnum room, int dir);

/* Field accessors */
const id_list &room_get_contents(const struct room_data *room);
const id_list &room_get_contents(GameWorld *g, room_vnum room);
const char *room_get_description(const struct room_data *room);
void room_set_description(struct room_data *room, const char *v);
void room_set_description(struct room_data *room, const std::string &v);
const char *room_get_description(GameWorld *g, room_vnum room);
void room_set_description(GameWorld *g, room_vnum room, const char *v);
void room_set_description(GameWorld *g, room_vnum room, const std::string &v);
int room_get_dmg(const struct room_data *room);
const ex_desc_list &room_get_ex_description(const struct room_data *room);
const ex_desc_list &room_get_ex_description(GameWorld *g, room_vnum room);
spec_proc_func room_get_func(const struct room_data *room);
void room_set_func(GameWorld *g, room_vnum room, spec_proc_func v);
int room_get_geffect(const struct room_data *room);
int room_get_gravity(const struct room_data *room);
int8_t room_get_light(GameWorld *g, room_vnum room);
void room_set_light(GameWorld *g, room_vnum room, int8_t v);
const char *room_get_name(const struct room_data *room);
void room_set_name(struct room_data *room, const char *v);
void room_set_name(struct room_data *room, const std::string &v);
const char *room_get_name(GameWorld *g, room_vnum room);
void room_set_name(GameWorld *g, room_vnum room, const char *v);
void room_set_name(GameWorld *g, room_vnum room, const std::string &v);
room_vnum room_get_number(const struct room_data *room);
void room_set_number(struct room_data *room, room_vnum v);
void room_set_number(GameWorld *g, room_vnum room, room_vnum v);
const id_list &room_get_people(const struct room_data *room);
const id_list &room_get_people(GameWorld *g, room_vnum room);
struct trig_proto_list *room_get_proto_script(const struct room_data *room);
void room_set_proto_script(struct room_data *room, struct trig_proto_list *v);
struct trig_proto_list *room_get_proto_script(GameWorld *g, room_vnum room);
void room_set_proto_script(GameWorld *g, room_vnum room, struct trig_proto_list *v);
bitvector_t *room_get_room_flags(struct room_data *room);
struct script_data *room_get_script(const struct room_data *room);
void room_set_script(struct room_data *room, struct script_data *v);
struct script_data *room_get_script(GameWorld *g, room_vnum room);
int room_get_sector_type(const struct room_data *room);
void room_set_sector_type(struct room_data *room, int v);
void room_set_sector_type(GameWorld *g, room_vnum room, int v);
int room_get_timed(GameWorld *g, room_vnum room);
void room_set_timed(GameWorld *g, room_vnum room, int v);
struct zone_data *room_get_zone(const struct room_data *room);
void room_set_zone(struct room_data *room, struct zone_data *v);
struct zone_data *room_get_zone(GameWorld *g, room_vnum room);
void room_set_zone(GameWorld *g, room_vnum room, struct zone_data *v);

/* A field's address, for code that fills the field in later or unlinks through it */
struct script_data **room_script_slot(struct room_data *room);
struct script_data **room_script_slot(GameWorld *g, room_vnum room);
ex_desc_list *room_ex_description_slot(struct room_data *room);
std::string *room_description_slot(struct room_data *room);
id_list *room_contents_slot(GameWorld *g, room_vnum room);
id_list *room_people_slot(GameWorld *g, room_vnum room);

/* The room numbered vnum, or null. */
struct room_data *room_of(GameWorld *g, room_vnum vnum);
