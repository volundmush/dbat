#include "dbat/db/guilds.h"

struct guild_data *guild_index;
int top_guild = -1;

guild_rnum real_guild(guild_vnum vnum)
{
  if (!guild_index || top_guild < 0)
    return NOTHING;

  for (guild_rnum rnum = 0; rnum <= top_guild; rnum++)
    if (guild_index[rnum].vnum == vnum)
      return rnum;

  return NOTHING;
}
