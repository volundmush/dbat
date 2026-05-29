/************************************************************************
 * Generic OLC Library - Guilds / gengld.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/db/guilds.h"
#include "dbat/db/shops.h"
#include "dbat/game/gengld.h"
#include "dbat/game/shop.h"
#include "dbat/game/genolc.h"
#include "dbat/game/genzon.h"
#include "dbat/game/utils.h"
#include "dbat/game/gedit.h"

/*
 * NOTE (gg): Didn't modify sedit much. Don't consider it as 'recent'
 * 	as the other editors with regard to updates or style.
 */

/*-------------------------------------------------------------------*/

void copy_guild(struct guild_data *tgm, struct guild_data *fgm)
{
	int i;

	/*. Copy basic info over . */
	G_NUM(tgm) = G_NUM(fgm);
	G_CHARGE(tgm) = G_CHARGE(fgm);
	G_TRAINER(tgm) = G_TRAINER(fgm);
	for (i = 0; i < SW_ARRAY_MAX; i++)
		G_WITH_WHO(tgm)[i] = G_WITH_WHO(fgm)[i];
	G_OPEN(tgm) = G_OPEN(fgm);
	G_CLOSE(tgm) = G_CLOSE(fgm);
	G_MINLVL(tgm) = G_MINLVL(fgm);
	G_FUNC(tgm) = G_FUNC(fgm);


	/*. Copy the strings over . */
	free_guild_strings(tgm);
	G_NO_SKILL(tgm) = str_udup(G_NO_SKILL(fgm));
	G_NO_GOLD(tgm) = str_udup(G_NO_GOLD(fgm));

	for (i = 0; i < SKILL_TABLE_SIZE; i++)
		G_SK_AND_SP(tgm, i) = G_SK_AND_SP(fgm, i);

        for (i = 0; i < NUM_FEATS_DEFINED; i++)
          G_FEATS(tgm, i) = G_FEATS(fgm, i);
}

/*-------------------------------------------------------------------*/
/*. Free all the character strings in a guild structure . */

void free_guild_strings(struct guild_data *guild)
{
	if (G_NO_SKILL(guild)) {
		free(G_NO_SKILL(guild));
		G_NO_SKILL(guild) = NULL;
	}
	if (G_NO_GOLD(guild)) {
		free(G_NO_GOLD(guild));
		G_NO_GOLD(guild) = NULL;
	}
}

/*-------------------------------------------------------------------*/

/*. Free up the whole guild structure and its contents . */

void free_guild(struct guild_data *guild)
{
	free_guild_strings(guild);
	free(guild);
}

/*-------------------------------------------------------------------*/

/*. Generic string modifyer for guild master messages . */

void gedit_modify_string(char **str, char *new_g)
{
	char *pointer;
	char buf[MAX_STRING_LENGTH];

	/*. Check the '%s' is present, if not, add it . */
	if (*new_g != '%') {
		snprintf(buf, sizeof(buf), "%%s %s", new_g);
		pointer = buf;
	} else
		pointer = new_g;

	if (*str)
		free(*str);
	*str = strdup(pointer);
}

/*-------------------------------------------------------------------*/

int add_guild(struct guild_data *ngld)
{
  struct guild_data *guild;
  int found = 0;
  zone_vnum zv = virtual_zone_by_thing(G_NUM(ngld));

  /*
   * The guild already exists, just update it.
   */
  if (guild = guild_by_id(G_NUM(ngld))) {
    copy_guild(guild, ngld);
    if (zv != NOTHING) {
      add_to_save_list(zv, SL_GLD);
    } else
      mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: GenOLC: Cannot determine guild zone.");
    return guild->vnum;
  }

  mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: GenOLC: Creating new guild.");

  CREATE(guild, struct guild_data, 1);
  copy_guild(guild, ngld);
  guild_put(guild->vnum, guild);

  if (zv != NOTHING) {
    add_to_save_list(zv, SL_GLD);
  }
  else
    mudlog(BRF, ADMLVL_BUILDER, TRUE, "SYSERR: GenOLC: Cannot determine guild zone.");

  return guild->vnum;
}

/*-------------------------------------------------------------------*/

int save_guilds(struct zone_data *zone)
{
  int i, j, rguild;
  FILE *guild_file;
  char fname[64];
  struct guild_data *guild;

  if (!zone)
  {
    log("SYSERR: GenOLC: save_guilds: Invalid zone!");
    return FALSE;
  }

  snprintf(fname, sizeof(fname), "%s%d.gld", GLD_PREFIX, zone->number);
  if (!(guild_file = fopen(fname, "w")))
  {
    mudlog(BRF, ADMLVL_GOD, TRUE, "SYSERR: OLC: Cannot open Guild file!");
    return FALSE;
  }

  /*. Search database for guilds in this zone . */
  for (i = zone->bot; i <= zone->top; i++)
  {
    auto guild = guild_by_id(i);
    if (!guild)
      continue;
    fprintf(guild_file, "#%d~\n", i);

    for (j = 0; j < SKILL_TABLE_SIZE; j++)
      if (G_SK_AND_SP(guild, j))
        fprintf(guild_file, "%d 1\n", j);

    for (j = 0; j < NUM_FEATS_DEFINED; j++)
      if (G_FEATS(guild, j))
        fprintf(guild_file, "%d 2\n", j);

    fprintf(guild_file, "-1\n");

    /*. Save charge . */
    fprintf(guild_file, "%1.2f\n", G_CHARGE(guild));

    /*. Save messages . */
    fprintf(guild_file,
            "%s~\n%s~\n",
            /*. Added some small'n'silly defaults as sanity checks . */
            (G_NO_SKILL(guild) ? G_NO_SKILL(guild) : "%s ERROR"),
            (G_NO_GOLD(guild) ? G_NO_GOLD(guild) : "%s ERROR"));

    /* Write what the GM teaches */
    fprintf(guild_file, "%d\n", G_MINLVL(guild));

    auto keeper = mob_proto_by_id(G_TRAINER(guild));

    /*. Save the rest . */
    fprintf(guild_file, "%d\n%d\n%d\n%d\n",
            keeper ? keeper->vnum : -1,
            G_WITH_WHO(guild)[0],
            G_OPEN(guild),
            G_CLOSE(guild));
    for (j = 1; j < SW_ARRAY_MAX; j++)
      fprintf(guild_file, "%s%d", j == 1 ? "" : " ", G_WITH_WHO(guild)[j]);
    fprintf(guild_file, "\n");
  }
  fprintf(guild_file, "$~\n");
  fclose(guild_file);

  if (in_save_list(zone->number, SL_GLD))
  {
    remove_from_save_list(zone->number, SL_GLD);
    create_world_index(zone->number, "gld");
    log("GenOLC: save_guilds: Saving guilds '%s'", fname);
  }
  return TRUE;
}
