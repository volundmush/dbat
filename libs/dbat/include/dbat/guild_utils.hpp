#pragma once
#include "dbat/consts/types.hpp"

struct guild_data;

/* Field accessors for guilds. */
room_vnum guild_get_vnum(const struct guild_data *guild);
void guild_set_vnum(struct guild_data *guild, room_vnum v);
mob_vnum guild_get_gm(const struct guild_data *guild);
void guild_set_gm(struct guild_data *guild, mob_vnum v);
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
const char *guild_get_no_such_skill(const struct guild_data *guild);
void guild_set_no_such_skill(struct guild_data *guild, const char *v);
const char *guild_get_not_enough_gold(const struct guild_data *guild);
void guild_set_not_enough_gold(struct guild_data *guild, const char *v);
int guild_get_skill(const struct guild_data *guild, int n);
void guild_set_skill(struct guild_data *guild, int n, int v);
int guild_get_feat(const struct guild_data *guild, int n);
void guild_set_feat(struct guild_data *guild, int n, int v);

