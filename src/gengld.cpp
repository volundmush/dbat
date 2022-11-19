/************************************************************************
 * Generic OLC Library - Guilds / gengld.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "gengld.h"
#include "shop.h"
#include "genolc.h"
#include "genzon.h"
#include "utils.h"

/*
 * NOTE (gg): Didn't modify sedit much. Don't consider it as 'recent'
 * 	as the other editors with regard to updates or style.
 */

/*-------------------------------------------------------------------*/

void copy_guild(struct guild_data *tgm, struct guild_data *fgm) {
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

    tgm->skills = fgm->skills;
    tgm->feats = fgm->feats;}

/*-------------------------------------------------------------------*/
/*. Free all the character strings in a guild structure . */

void free_guild_strings(struct guild_data *guild) {
    if (G_NO_SKILL(guild)) {
        free(G_NO_SKILL(guild));
        G_NO_SKILL(guild) = nullptr;
    }
    if (G_NO_GOLD(guild)) {
        free(G_NO_GOLD(guild));
        G_NO_GOLD(guild) = nullptr;
    }
}

/*-------------------------------------------------------------------*/

/*. Free up the whole guild structure and its contents . */

void free_guild(struct guild_data *guild) {
    free_guild_strings(guild);
    free(guild);
}

/*-------------------------------------------------------------------*/

/* returns the real number of the guild with given virtual number 
 *
 * We take so good care to keep it sorted - let's use it :) - Welcor
 */
guild_rnum real_guild(guild_vnum vnum) {
    return guild_index.count(vnum) ? vnum : NOTHING;
}

/*-------------------------------------------------------------------*/

/*. Generic string modifyer for guild master messages . */

void gedit_modify_string(char **str, char *new_g) {
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

int add_guild(struct guild_data *ngld) {
    guild_rnum rguild = G_NUM(ngld);
    int found = 0;
    zone_rnum rznum = real_zone_by_thing(rguild);

    auto exists = guild_index.count(rguild);
    auto &g = guild_index[rguild];

    /*
     * The guild already exists, just update it.
     */
    if (exists) {
        copy_guild(&g, ngld);
        if (rznum != NOWHERE) {
            add_to_save_list(zone_table[rznum].number, SL_GLD);
        } else
            mudlog(BRF, ADMLVL_BUILDER, true, "SYSERR: GenOLC: Cannot determine guild zone.");
        return rguild;
    }

    mudlog(BRF, ADMLVL_BUILDER, true, "SYSERR: GenOLC: Creating new guild.");
    copy_guild(&g, ngld);
    add_to_save_list(zone_table[rznum].number, SL_GLD);
    return rguild;
}

/*-------------------------------------------------------------------*/

int save_guilds(zone_rnum zone_num) {
    vnum j, rguild;
    FILE *guild_file;
    char fname[64];
    struct guild_data *guild;

    if (!zone_table.count(zone_num))
    {
        log("SYSERR: GenOLC: save_guilds: Invalid real zone number %d.", zone_num);
        return false;
    }

    auto &z = zone_table[zone_num];

    snprintf(fname, sizeof(fname), "%s%d.gld", GLD_PREFIX, z.number);
    if (!(guild_file = fopen(fname, "w"))) {
        mudlog(BRF, ADMLVL_GOD, true, "SYSERR: OLC: Cannot open Guild file!");
        return false;
    }

    /*. Search database for guilds in this zone . */
    for (auto i : z.guilds) {
        if ((rguild = real_guild(i)) != NOWHERE) {
            fprintf(guild_file, "#%d~\n", i);
            guild = &guild_index[rguild];

            for (auto j : guild->skills)
                fprintf(guild_file, "%d 1\n", j);

            for (auto j : guild->feats)
                fprintf(guild_file, "%d 2\n", j);

            fprintf(guild_file, "-1\n");

            /*. Save charge . */
            fprintf(guild_file, "%1.2f\n", G_CHARGE(guild));

            /*. Save messages . */
            fprintf(guild_file,
                    "%s~\n%s~\n",
                    /*. Added some small'n'silly defaults as sanity checks . */
                    (G_NO_SKILL(guild) ? G_NO_SKILL(guild) : "%s ERROR"),
                    (G_NO_GOLD(guild) ? G_NO_GOLD(guild) : "%s ERROR")
            );

            /* Write what the GM teaches */
            fprintf(guild_file, "%d\n", G_MINLVL(guild));


            /*. Save the rest . */
            fprintf(guild_file, "%d\n%d\n%d\n%d\n",
                    G_TRAINER(guild) == NOBODY ? -1 : mob_index[G_TRAINER(guild)].vnum,
                    G_WITH_WHO(guild)[0],
                    G_OPEN(guild),
                    G_CLOSE(guild)
            );
            for (j = 1; j < SW_ARRAY_MAX; j++)
                fprintf(guild_file, "%s%d", j == 1 ? "" : " ", G_WITH_WHO(guild)[j]);
            fprintf(guild_file, "\n");
        }
    }
    fprintf(guild_file, "$~\n");
    fclose(guild_file);

    if (in_save_list(z.number, SL_GLD)) {
        remove_from_save_list(z.number, SL_GLD);
        create_world_index(z.number, "gld");
        log("GenOLC: save_guilds: Saving guilds '%s'", fname);
    }
    return true;
}
