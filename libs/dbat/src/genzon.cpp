/************************************************************************
 * Generic OLC Library - Zones / genzon.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/Zone.h"
#include "dbat/genzon.h"
#include "dbat/utils.h"

#include "dbat/genolc.h"
#include "dbat/dg_scripts.h"
#include "dbat/Shop.h"
#include "dbat/Guild.h"
#include "dbat/constants.h"

zone_rnum create_new_zone(zone_vnum vzone_num, const char **error)
{
    FILE *fp;
    struct Zone *zone;
    int i;
    zone_rnum rznum;
    char buf[MAX_STRING_LENGTH];

    if (vzone_num < 0)
    {
        *error = "You can't make negative zones.\r\n";
        return NOWHERE;
    }

    if (zone_table.count(vzone_num))
    {
        *error = "That virtual zone already exists.\r\n";
        return NOWHERE;
    }

    /*
     * Create the zone file.
     */
    snprintf(buf, sizeof(buf), "%s%d.zon", ZON_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w")))
    {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new zone file.");
        *error = "Could not write zone file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "#%d\nNew Zone~\n%d 30 2\nS\n$\n", vzone_num, (vzone_num * 100) + 99);

    /*
     * Create the room file.
     */
    snprintf(buf, sizeof(buf), "%s%d.wld", WLD_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w")))
    {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new world file.");
        *error = "Could not write world file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "\nThe Beginning~\nNot much here.\n~\n%d 0 0\nS\n$\n", vzone_num);
    fclose(fp);

    /*
     * Create the mobile file.
     */
    snprintf(buf, sizeof(buf), "%s%d.mob", MOB_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w")))
    {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new mob file.");
        *error = "Could not write mobile file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "$\n");
    fclose(fp);

    /*
     * Create the object file.
     */
    snprintf(buf, sizeof(buf), "%s%d.obj", OBJ_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w")))
    {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new obj file.");
        *error = "Could not write object file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "$\n");
    fclose(fp);

    /*
     * Create the shop file.
     */
    snprintf(buf, sizeof(buf), "%s%d.shp", SHP_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w")))
    {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new shop file.");
        *error = "Could not write shop file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "$~\n");
    fclose(fp);

    /*
     * Create the trigger file.
     */
    snprintf(buf, sizeof(buf), "%s%d.trg", TRG_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w")))
    {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new trigger file");
        *error = "Could not write trigger file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "$~\n");
    fclose(fp);

    /*
     * Create Gld file .
     */
    snprintf(buf, sizeof(buf), "%s/%i.gld", GLD_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w")))
    {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new guild file");
        *error = "Could not write guild file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "$~\n");
    fclose(fp);

    /*
     * Make a new zone in memory. This was the source of all the zedit new
     * crashes reported to the CircleMUD list. It was happily overwriting
     * the stack.  This new loop by Andrew Helm fixes that problem and is
     * more understandable at the same time.
     *
     * The variable is 'top_of_zone_table_table + 2' because we need record 0
     * through top_of_zone (top_of_zone_table + 1 items) and a new one which
     * makes it top_of_zone_table + 2 elements large.
     */
    auto &z = zone_table.at(vzone_num);
    z.number = vzone_num;

    /*
     * Ok, insert the new zone here.
     */
    z.name = "New Zone";
    z.number = vzone_num;
    z.builders = "None";
    z.lifespan = 30;
    z.age = 0;
    z.reset_mode = 2;
    /*
     * No zone commands, just terminate it with an 'S'
     */

    return rznum;
}


/*-------------------------------------------------------------------*/
