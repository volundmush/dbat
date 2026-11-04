#include "dbat/guild_utils.h"
#include "dbat/guild.h"
#include "dbat/flags.h"
#include "dbat/game_world.h"

room_vnum guild_get_vnum(const struct guild_data *guild)
{
  return guild->vnum;
}

void guild_set_vnum(struct guild_data *guild, room_vnum v)
{
  guild->vnum = v;
}

mob_rnum guild_get_gm(const struct guild_data *guild)
{
  return guild->gm;
}

void guild_set_gm(struct guild_data *guild, mob_rnum v)
{
  guild->gm = v;
}

int guild_get_open(const struct guild_data *guild)
{
  return guild->open;
}

void guild_set_open(struct guild_data *guild, int v)
{
  guild->open = v;
}

int guild_get_close(const struct guild_data *guild)
{
  return guild->close;
}

void guild_set_close(struct guild_data *guild, int v)
{
  guild->close = v;
}

spec_proc_func guild_get_func(const struct guild_data *guild)
{
  return guild->func;
}

void guild_set_func(struct guild_data *guild, spec_proc_func v)
{
  guild->func = v;
}

float guild_get_charge(const struct guild_data *guild)
{
  return guild->charge;
}

void guild_set_charge(struct guild_data *guild, float v)
{
  guild->charge = v;
}

int guild_get_minlvl(const struct guild_data *guild)
{
  return guild->minlvl;
}

void guild_set_minlvl(struct guild_data *guild, int v)
{
  guild->minlvl = v;
}

bitvector_t *guild_get_with_who(struct guild_data *guild)
{
  return guild->with_who;
}

char *guild_get_no_such_skill(const struct guild_data *guild)
{
  return guild->no_such_skill;
}

void guild_set_no_such_skill(struct guild_data *guild, char *v)
{
  guild->no_such_skill = v;
}

char *guild_get_not_enough_gold(const struct guild_data *guild)
{
  return guild->not_enough_gold;
}

void guild_set_not_enough_gold(struct guild_data *guild, char *v)
{
  guild->not_enough_gold = v;
}

int guild_get_skill(const struct guild_data *guild, int n)
{
  return guild->skills[n];
}

void guild_set_skill(struct guild_data *guild, int n, int v)
{
  guild->skills[n] = v;
}

int guild_get_feat(const struct guild_data *guild, int n)
{
  return guild->feats[n];
}

void guild_set_feat(struct guild_data *guild, int n, int v)
{
  guild->feats[n] = v;
}

room_vnum rguild_get_vnum(GameWorld *g, guild_rnum i)
{
  return g->guild_index[i].vnum;
}

void rguild_set_vnum(GameWorld *g, guild_rnum i, room_vnum v)
{
  g->guild_index[i].vnum = v;
}

mob_rnum rguild_get_gm(GameWorld *g, guild_rnum i)
{
  return g->guild_index[i].gm;
}

void rguild_set_gm(GameWorld *g, guild_rnum i, mob_rnum v)
{
  g->guild_index[i].gm = v;
}

int rguild_get_open(GameWorld *g, guild_rnum i)
{
  return g->guild_index[i].open;
}

int rguild_get_close(GameWorld *g, guild_rnum i)
{
  return g->guild_index[i].close;
}

float rguild_get_charge(GameWorld *g, guild_rnum i)
{
  return g->guild_index[i].charge;
}

int rguild_get_minlvl(GameWorld *g, guild_rnum i)
{
  return g->guild_index[i].minlvl;
}

bitvector_t *rguild_get_with_who(GameWorld *g, guild_rnum i)
{
  return g->guild_index[i].with_who;
}

spec_proc_func rguild_get_func(GameWorld *g, guild_rnum i)
{
  return g->guild_index[i].func;
}

void rguild_set_func(GameWorld *g, guild_rnum i, spec_proc_func v)
{
  g->guild_index[i].func = v;
}
