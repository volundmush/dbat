#include "dbat/db/guilds.h"


guild_rnum real_guild(guild_vnum vnum)
{
  return guild_by_id(vnum) ? vnum : NOTHING;
}
