#pragma once
#include "dbat/consts/types.h"

struct guild_data;

/* Field accessors for guilds: g_* take a pointer, rguild_* a real number (an index into guild_index). */
room_vnum guild_get_vnum(const struct guild_data *guild);
void guild_set_vnum(struct guild_data *guild, room_vnum v);
mob_rnum guild_get_gm(const struct guild_data *guild);
void guild_set_gm(struct guild_data *guild, mob_rnum v);
int guild_get_open(const struct guild_data *guild);
void guild_set_open(struct guild_data *guild, int v);
int guild_get_close(const struct guild_data *guild);
void guild_set_close(struct guild_data *guild, int v);
spec_proc_func guild_get_func(const struct guild_data *guild);
void guild_set_func(struct guild_data *guild, spec_proc_func v);
float guild_get_charge(const struct guild_data *guild);
void guild_set_charge(struct guild_data *guild, float v);
int guild_get_minlvl(const struct guild_data *guild);
void guild_set_minlvl(struct guild_data *guild, int v);
bitvector_t *guild_get_with_who(struct guild_data *guild);
char *guild_get_no_such_skill(const struct guild_data *guild);
void guild_set_no_such_skill(struct guild_data *guild, char *v);
char *guild_get_not_enough_gold(const struct guild_data *guild);
void guild_set_not_enough_gold(struct guild_data *guild, char *v);
int guild_get_skill(const struct guild_data *guild, int n);
void guild_set_skill(struct guild_data *guild, int n, int v);
int guild_get_feat(const struct guild_data *guild, int n);
void guild_set_feat(struct guild_data *guild, int n, int v);
room_vnum rguild_get_vnum(guild_rnum i);
void rguild_set_vnum(guild_rnum i, room_vnum v);
mob_rnum rguild_get_gm(guild_rnum i);
void rguild_set_gm(guild_rnum i, mob_rnum v);
int rguild_get_open(guild_rnum i);
int rguild_get_close(guild_rnum i);
float rguild_get_charge(guild_rnum i);
int rguild_get_minlvl(guild_rnum i);
bitvector_t *rguild_get_with_who(guild_rnum i);
spec_proc_func rguild_get_func(guild_rnum i);
void rguild_set_func(guild_rnum i, spec_proc_func v);
