#include "dbat/guild_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/guild.hpp"
#include "dbat/flags.hpp"
#include "dbat/game_world.hpp"

room_vnum guild_get_vnum(const struct guild_data *guild)
{
  return guild->vnum;
}

void guild_set_vnum(struct guild_data *guild, room_vnum v)
{
  guild->vnum = v;
}

mob_vnum guild_get_gm(const struct guild_data *guild)
{
  return guild->gm;
}

void guild_set_gm(struct guild_data *guild, mob_vnum v)
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

const char *guild_get_no_such_skill(const struct guild_data *guild)
{
  return c_str_or_null(guild->no_such_skill);
}

void guild_set_no_such_skill(struct guild_data *guild, const char *v)
{
  guild->no_such_skill = v ? v : "";
}

const char *guild_get_not_enough_gold(const struct guild_data *guild)
{
  return c_str_or_null(guild->not_enough_gold);
}

void guild_set_not_enough_gold(struct guild_data *guild, const char *v)
{
  guild->not_enough_gold = v ? v : "";
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











