#include "dbat/db/guilds.h"

struct guild_data *guild_index;
int top_guild = -1;

guild_rnum real_guild(guild_vnum vnum)
{
  guild_rnum bot, top, mid, last_top;

  if (top_guild < 0)
    return NOWHERE;

  bot = 0;
  top = top_guild;

  for (;;) {
    last_top = top;
    mid = (bot + top) / 2;

    if (guild_index[mid].vnum == vnum)
      return mid;
    if (bot >= top)
      return NOWHERE;
    if (guild_index[mid].vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;

    if (top > last_top)
      return NOWHERE;
  }
}

struct guild_data *guild_by_id(guild_vnum vnum)
{
  guild_rnum rnum = real_guild(vnum);

  if (rnum == NOWHERE || !guild_index)
    return nullptr;

  return &guild_index[rnum];
}
