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
