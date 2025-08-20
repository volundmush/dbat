/************************************************************************
 * Generic OLC Library - Mobiles / genmob.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/CharacterPrototype.h"
#include "dbat/genmob.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/genolc.h"
#include "dbat/Shop.h"
#include "dbat/genzon.h"
#include "dbat/Guild.h"
#include "dbat/dg_scripts.h"
#include "dbat/handler.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/spells.h"
#include "dbat/players.h"
#include "dbat/Account.h"
#include "dbat/constants.h"
#include "dbat/filter.h"

/* From db.c */


/* local functions */
void extract_mobile_all(mob_vnum vnum);

int add_mobile(CharacterPrototype *mob, mob_vnum vnum)
{
    mob_vnum rnum, found = false;
    Character *live_mob;

    bool exists = mob_proto.contains(vnum);

    if (exists)
    {
        /* Copy over the mobile and free() the old strings. */
        mob_proto.at(rnum) = *mob;
        basic_mud_log("GenOLC: add_mobile: Updated existing mobile #%d.", vnum);
    }
    else
    {
        mob_proto[vnum] = *mob;
        auto &ix = mob_index[vnum];
        ix.vn = vnum;
        basic_mud_log("GenOLC: add_mobile: Added mobile %d.", vnum);
    }

    return vnum;
}

void extract_mobile_all(mob_vnum vnum)
{
    auto mobs = characterSubscriptions.all(fmt::format("vnum_{}", vnum));
    for (auto ch : filter_raw(mobs))
    {
        extract_char(ch);
    }
}

int delete_mobile(mob_rnum refpt)
{
    Character *live_mob;
    int counter, cmd_no;
    mob_vnum vnum;
    zone_rnum zone;

    if (!mob_proto.count(refpt))
    {
        basic_mud_log("SYSERR: GenOLC: delete_mobile: Invalid rnum %d.", refpt);
        return NOBODY;
    }

    vnum = refpt;
    extract_mobile_all(vnum);

    /* Update shop keepers.  */
    for (auto &sh : shop_index)
    {
        /* Find the shop for this keeper and reset it's keeper to
         * -1 to keep the shop so it could be assigned to someone else */
        if (sh.second.keeper == refpt)
        {
            sh.second.keeper = NOBODY;
        }
    }

    /* Update guild masters */
    for (auto &g : guild_index)
    {
        /* Find the guild for this trainer and reset it's trainer to
         * -1 to keep the guild so it could be assigned to someone else */
        if (g.second.keeper == refpt)
        {
            g.second.keeper = NOBODY;
        }
    }

    mob_proto.erase(vnum);
    mob_index.erase(vnum);

    return refpt;
}

#if CONFIG_GENOLC_MOBPROG
int write_mobile_mobprog(mob_vnum mvnum, Character *mob, FILE *fd)
{
    char wmmarg[MAX_STRING_LENGTH], wmmcom[MAX_STRING_LENGTH];
    MPROG_DATA *mob_prog;

    for (mob_prog = GET_MPROG(mob); mob_prog; mob_prog = mob_prog->next)
    {
        wmmarg[MAX_STRING_LENGTH - 1] = '\0';
        wmmcom[MAX_STRING_LENGTH - 1] = '\0';
        strip_cr(strncpy(wmmarg, mob_prog->arglist, MAX_STRING_LENGTH - 1));
        strip_cr(strncpy(wmmcom, mob_prog->comlist, MAX_STRING_LENGTH - 1));
        fprintf(fd, "%s %s~\n"
                    "%s%c\n",
                medit_get_mprog_type(mob_prog), wmmarg,
                wmmcom, STRING_TERMINATOR);
        if (mob_prog->next == nullptr)
            fputs("|\n", fd);
    }
    return TRUE;
}
#endif

