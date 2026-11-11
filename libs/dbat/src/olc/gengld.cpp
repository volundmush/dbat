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

/*-------------------------------------------------------------------*/

bool save_guilds(GameWorld *g, struct zone_data *zone_num)
{
  struct atomic_file af;
  int j;
  FILE *guild_file;
  std::string fname;
  struct guild_data *guild;

  if (!zone_num) {
    mud_log("SYSERR: no zone given.");
    return false;
  }

  fname = str_format("{}{:d}.gld", GLD_PREFIX, zone_num->number);
  if (!(guild_file = atomic_open(&af, fname.c_str(), false))) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC: Cannot open Guild file!");
    return false;
  }

  /*. Search database for guilds in this zone . */
  for (guild_vnum i : g->guilds.vnums_in(genolc_zone_bottom(g, zone_num), zone_num->top)) {
    if ((guild = find_guild(g, i)) != nullptr) {
      file_print(guild_file, "#{:d}~\n", i);

      for (j = 0; j < SKILL_TABLE_SIZE; j++)
        if (guild_get_skill(guild, j))
          file_print(guild_file, "{:d} 1\n", j);

      for (j = 0; j < NUM_FEATS_DEFINED; j++)
        if (guild_get_feat(guild, j))
          file_print(guild_file, "{:d} 2\n", j);

      file_print(guild_file, "-1\n");

      /*. Save charge . */
      file_print(guild_file, "{:>1.2f}\n", guild_get_charge(guild));

      /*. Save messages . */
      file_print(guild_file,
                 "{}~\n{}~\n",
                 /*. Added some small'n'silly defaults as sanity checks . */
                 (guild_get_no_such_skill(guild) ? guild_get_no_such_skill(guild) : "%s ERROR"),
                 (guild_get_not_enough_gold(guild) ? guild_get_not_enough_gold(guild) : "%s ERROR"));

      /* Write what the GM teaches */
      file_print(guild_file, "{:d}\n", guild_get_minlvl(guild));

      /*. Save the rest . */
      file_print(guild_file, "{:d}\n{:d}\n{:d}\n{:d}\n",
                 guild_get_gm(guild) == NOBODY ? -1 : mob_index_of(g, guild_get_gm(guild))->vnum,
                 (int)guild_get_with_who(guild)[0], /* as signed, as always */
                 guild_get_open(guild),
                 guild_get_close(guild));
      for (j = 1; j < SW_ARRAY_MAX; j++)
        file_print(guild_file, "{}{:d}", j == 1 ? "" : " ", (int)guild_get_with_who(guild)[j]);
      file_print(guild_file, "\n");
    }
  }
  file_print(guild_file, "$~\n");
  atomic_commit(&af);

  if (in_save_list(g, zone_num->number, SL_GLD)) {
    remove_from_save_list(g, zone_num->number, SL_GLD);
    create_world_index(g, zone_num->number, "gld");
    mud_log("GenOLC: save_guilds: Saving guilds '{}'", fname);
  }
  return true;
}
