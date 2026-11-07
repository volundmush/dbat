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

static void free_guild_strings(struct guild_data *guild);

/*
 * NOTE (gg): Didn't modify sedit much. Don't consider it as 'recent'
 * 	as the other editors with regard to updates or style.
 */

/*-------------------------------------------------------------------*/

void copy_guild(struct guild_data *tgm, struct guild_data *fgm)
{
  int i;

  /*. Copy basic info over . */
  guild_set_vnum(tgm, guild_get_vnum(fgm));
  guild_set_charge(tgm, guild_get_charge(fgm));
  guild_set_gm(tgm, guild_get_gm(fgm));
  for (i = 0; i < SW_ARRAY_MAX; i++)
    guild_get_with_who(tgm)
        [i] = guild_get_with_who(fgm)[i];
  guild_set_open(tgm, guild_get_open(fgm));
  guild_set_close(tgm, guild_get_close(fgm));
  guild_set_minlvl(tgm, guild_get_minlvl(fgm));
  guild_set_func(tgm, guild_get_func(fgm));

  /*. Copy the strings over . */
  free_guild_strings(tgm);
  guild_set_no_such_skill(tgm, str_udup(guild_get_no_such_skill(fgm)));
  guild_set_not_enough_gold(tgm, str_udup(guild_get_not_enough_gold(fgm)));

  for (i = 0; i < SKILL_TABLE_SIZE; i++)
    guild_set_skill(tgm, i, guild_get_skill(fgm, i));

  for (i = 0; i < NUM_FEATS_DEFINED; i++)
    guild_set_feat(tgm, i, guild_get_feat(fgm, i));
}

/*-------------------------------------------------------------------*/
/*. Free all the character strings in a guild structure . */

static void free_guild_strings(struct guild_data *guild)
{
  if (guild_get_no_such_skill(guild)) {
    free(guild_get_no_such_skill(guild));
    guild_set_no_such_skill(guild, nullptr);
  }
  if (guild_get_not_enough_gold(guild)) {
    free(guild_get_not_enough_gold(guild));
    guild_set_not_enough_gold(guild, nullptr);
  }
}

/*-------------------------------------------------------------------*/

/*. Free up the whole guild structure and its contents . */

void free_guild(struct guild_data *guild)
{
  free_guild_strings(guild);
  delete guild;
}

/*-------------------------------------------------------------------*/

/* returns the real number of the guild with given virtual number 
 *
 * We take so good care to keep it sorted - let's use it :) - Welcor
 */
static int guild_order(const void *key, const void *guild)
{
  guild_vnum k = *(const guild_vnum *)key, v = ((const struct guild_data *)guild)->vnum;

  return (k > v) - (k < v);
}

guild_rnum real_guild(GameWorld *g, guild_vnum vnum)
{
  struct guild_data *found;

  if (g->top_guild < 0)
    return NOWHERE;
  found = (struct guild_data *)bsearch(&vnum, g->guild_index, g->top_guild + 1, sizeof(*g->guild_index), guild_order);
  return found ? found - g->guild_index : NOWHERE;
}

/*-------------------------------------------------------------------*/

/* Generic string modifier for guild master messages. Returns NULL once stored, or why the message can't be used (the
 * old one is then kept). */
const char *gedit_modify_string(char **str, char *new_g)
{
  char *pointer;
  char buf[MAX_STRING_LENGTH];

  /*. Check the '%s' is present, if not, add it . */
  if (*new_g != '%') {
    snprintf(buf, sizeof(buf), "%%s %s", new_g);
    pointer = buf;
  } else
    pointer = new_g;

  const char *problem = npc_msg_format_problem(pointer, false);
  if (problem)
    return problem;

  if (*str)
    free(*str);
  *str = strdup(pointer);
  return nullptr;
}

/*-------------------------------------------------------------------*/

guild_rnum add_guild(GameWorld *g, struct guild_data *ngld)
{
  guild_rnum rguild;
  guild_rnum found = 0;
  zone_rnum rznum  = real_zone_by_thing(g, guild_get_vnum(ngld));

  /*
   * The guild already exists, just update it.
   */
  if ((rguild = real_guild(g, guild_get_vnum(ngld))) != NOWHERE) {
    copy_guild(&g->guild_index[rguild], ngld);
    if (rznum != NOWHERE) {
      add_to_save_list(g, g->zones[rznum].number, SL_GLD);
    } else
      mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: GenOLC: Cannot determine guild zone.");
    return rguild;
  }

  mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: GenOLC: Creating new guild.");

  g->top_guild++;
  resize_array(g->guild_index, g->top_guild, g->top_guild + 1);

  for (rguild = g->top_guild; rguild > 0; rguild--) {
    if (ngld->vnum > rguild_get_vnum(g, rguild - 1)) {
      found = rguild;

      /* whatever this slot holds is uninitialized, or was just copied to the next one up */
      g->guild_index[rguild] = {};
      copy_guild(&g->guild_index[rguild], ngld);
      break;
    }
    g->guild_index[rguild] = g->guild_index[rguild - 1];
  }

  if (!found) {
    g->guild_index[0] = {};
    copy_guild(&g->guild_index[0], ngld);
  }

  if (rznum != NOWHERE) {
    add_to_save_list(g, g->zones[rznum].number, SL_GLD);
  } else
    mudlog(g, LOG_BRF, ADMLVL_BUILDER, true, "SYSERR: GenOLC: Cannot determine guild zone.");

  return rguild;
}

/*-------------------------------------------------------------------*/

bool save_guilds(GameWorld *g, zone_rnum zone_num)
{
  struct atomic_file af;
  IDXTYPE i;
  int j;
  guild_rnum rguild;
  FILE *guild_file;
  char fname[64];
  struct guild_data *guild;

  if (zone_num < 0 || zone_num > g->top_zone) {
    mud_log("SYSERR: GenOLC: save_guilds: Invalid real zone number %d. (0-%d)", zone_num, g->top_zone);
    return false;
  }

  snprintf(fname, sizeof(fname), "%s%d.gld", GLD_PREFIX, g->zones[zone_num].number);
  if (!(guild_file = atomic_open(&af, fname, false))) {
    mudlog(g, LOG_BRF, ADMLVL_GOD, true, "SYSERR: OLC: Cannot open Guild file!");
    return false;
  }

  /*. Search database for guilds in this zone . */
  for (i = genolc_zone_bottom(g, zone_num); i <= g->zones[zone_num].top; i++) {
    if ((rguild = real_guild(g, i)) != NOWHERE) {
      fprintf(guild_file, "#%d~\n", i);
      guild = g->guild_index + rguild;

      for (j = 0; j < SKILL_TABLE_SIZE; j++)
        if (guild_get_skill(guild, j))
          fprintf(guild_file, "%d 1\n", j);

      for (j = 0; j < NUM_FEATS_DEFINED; j++)
        if (guild_get_feat(guild, j))
          fprintf(guild_file, "%d 2\n", j);

      fprintf(guild_file, "-1\n");

      /*. Save charge . */
      fprintf(guild_file, "%1.2f\n", guild_get_charge(guild));

      /*. Save messages . */
      fprintf(guild_file,
              "%s~\n%s~\n",
              /*. Added some small'n'silly defaults as sanity checks . */
              (guild_get_no_such_skill(guild) ? guild_get_no_such_skill(guild) : "%s ERROR"),
              (guild_get_not_enough_gold(guild) ? guild_get_not_enough_gold(guild) : "%s ERROR"));

      /* Write what the GM teaches */
      fprintf(guild_file, "%d\n", guild_get_minlvl(guild));

      /*. Save the rest . */
      fprintf(guild_file, "%d\n%d\n%d\n%d\n",
              guild_get_gm(guild) == NOBODY ? -1 : g->mob_index[guild_get_gm(guild)].vnum,
              (int)guild_get_with_who(guild)[0], /* as signed, as always */
              guild_get_open(guild),
              guild_get_close(guild));
      for (j = 1; j < SW_ARRAY_MAX; j++)
        fprintf(guild_file, "%s%d", j == 1 ? "" : " ", (int)guild_get_with_who(guild)[j]);
      fprintf(guild_file, "\n");
    }
  }
  fprintf(guild_file, "$~\n");
  atomic_commit(&af);

  if (in_save_list(g, g->zones[zone_num].number, SL_GLD)) {
    remove_from_save_list(g, g->zones[zone_num].number, SL_GLD);
    create_world_index(g, g->zones[zone_num].number, "gld");
    mud_log("GenOLC: save_guilds: Saving guilds '%s'", fname);
  }
  return true;
}
