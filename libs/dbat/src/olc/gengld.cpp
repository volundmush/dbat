/************************************************************************
 * Generic OLC Library - Guilds / gengld.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/arrays.hpp"
#include "dbat/guild.hpp"
#include "dbat/shop.hpp"
#include "dbat/gengld.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/utils.hpp"
#include "dbat/atomicfile.hpp"
#include "dbat/game_world.hpp"


/*
 * NOTE (gg): Didn't modify sedit much. Don't consider it as 'recent'
 * 	as the other editors with regard to updates or style.
 */

/*-------------------------------------------------------------------*/

void copy_guild(struct guild_data *tgm, const struct guild_data *fgm)
{
  *tgm = *fgm;
}

/*-------------------------------------------------------------------*/

/* The guild numbered vnum, or null. */
struct guild_data *find_guild(GameWorld *g, guild_vnum vnum)
{
  return g->guilds.find(vnum);
}

/*-------------------------------------------------------------------*/

/* Generic string modifier for guild master messages. Returns NULL once stored, or why the message can't be used (the
 * old one is then kept). */
const char *gedit_modify_string(std::string *str, const char *new_g)
{
  const char *pointer;
  std::string buf;

  /*. Check the '%s' is present, if not, add it . */
  if (*new_g != '%') {
    buf     = str_format("%s {}", new_g);
    pointer = buf.c_str();
  } else
    pointer = new_g;

  const char *problem = npc_msg_format_problem(pointer, false);
  if (problem)
    return problem;

  *str = pointer;
  return nullptr;
}

/*-------------------------------------------------------------------*/

struct guild_data *add_guild(GameWorld *g, struct guild_data *ngld)
{
  struct zone_data *rznum  = find_zone_by_thing(g, guild_get_vnum(ngld));
  struct guild_data *guild = find_guild(g, guild_get_vnum(ngld));

  if (!guild) /* otherwise it exists, and is updated */
    guild = g->guilds.emplace(guild_get_vnum(ngld));
  copy_guild(guild, ngld);

  if (rznum != nullptr) {
    add_to_save_list(g, rznum->number, SL_GLD);
  } else
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: GenOLC: Cannot determine guild zone.");

  return guild;
}
