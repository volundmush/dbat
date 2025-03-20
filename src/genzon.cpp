/************************************************************************
 * Generic OLC Library - Zones / genzon.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include <fstream>
#include "dbat/genzon.h"
#include "dbat/utils.h"

#include "dbat/genolc.h"
#include "dbat/dg_scripts.h"
#include "dbat/shop.h"
#include "dbat/guild.h"
#include "dbat/constants.h"

/* real zone of room/mobile/object/shop given */
zone_rnum real_zone_by_thing(room_vnum vznum) {
    for(auto &z : zone_table) if(vznum >= z.second.bot && vznum <= z.second.top) return z.first;
    return NOWHERE;
}

zone_rnum create_new_zone(zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error) {
    FILE *fp;
    struct zone_data *zone;
    int i;
    zone_rnum rznum;
    char buf[MAX_STRING_LENGTH];

#if CIRCLE_UNSIGNED_INDEX
    if (vzone_num == NOWHERE)
#else
        if (vzone_num < 0)
#endif
    {
        *error = "You can't make negative zones.\r\n";
        return NOWHERE;
    } else if (bottom > top) {
        *error = "Bottom room cannot be greater than top room.\r\n";
        return NOWHERE;
    }

    if(zone_table.count(vzone_num)) {
            *error = "That virtual zone already exists.\r\n";
            return NOWHERE;
        }

    /*
     * Create the zone file.
     */
    snprintf(buf, sizeof(buf), "%s%d.zon", ZON_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w"))) {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new zone file.");
        *error = "Could not write zone file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "#%d\nNew Zone~\n%d 30 2\nS\n$\n", vzone_num, (vzone_num * 100) + 99);

    /*
     * Create the room file.
     */
    snprintf(buf, sizeof(buf), "%s%d.wld", WLD_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w"))) {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new world file.");
        *error = "Could not write world file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "#%d\nThe Beginning~\nNot much here.\n~\n%d 0 0\nS\n$\n", bottom, vzone_num);
    fclose(fp);

    /*
     * Create the mobile file.
     */
    snprintf(buf, sizeof(buf), "%s%d.mob", MOB_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w"))) {
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
    if (!(fp = fopen(buf, "w"))) {
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
    if (!(fp = fopen(buf, "w"))) {
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
    if (!(fp = fopen(buf, "w"))) {
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
    if (!(fp = fopen(buf, "w"))) {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new guild file");
        *error = "Could not write guild file.\r\n";
        return NOWHERE;
    }
    fprintf(fp, "$~\n");
    fclose(fp);

    /*
     * Update index files.
     */
    create_world_index(vzone_num, "zon");
    create_world_index(vzone_num, "wld");
    create_world_index(vzone_num, "mob");
    create_world_index(vzone_num, "obj");
    create_world_index(vzone_num, "shp");
    create_world_index(vzone_num, "trg");
    create_world_index(vzone_num, "gld");

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
    auto &z = zone_table[vzone_num];
    z.number = vzone_num;
    
    /*
     * Ok, insert the new zone here.
     */
    z.name = strdup("New Zone");
    z.number = vzone_num;
    z.builders = strdup("None");
#if _CIRCLEMUD >= CIRCLEMUD_VERSION(3, 0, 21)
    z.bot = bottom;
    z.top = top;
#else
    zone->top = (vzone_num * 100) + 99;
#endif
    z.lifespan = 30;
    z.age = 0;
    z.reset_mode = 2;
    z.min_level = 0;
    z.max_level = ADMLVL_IMPL;
    /*
     * No zone commands, just terminate it with an 'S'
     */
    auto &c = z.cmd.emplace_back();
    c.command = 'S';

    return rznum;
}

/*-------------------------------------------------------------------*/

void create_world_index(int znum, const char *type) {
    FILE *newfile, *oldfile;
    char new_name[32], old_name[32], *prefix;
    int num, found = false;
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];

    switch (*type) {
        case 'z':
            prefix = ZON_PREFIX;
            break;
        case 'w':
            prefix = WLD_PREFIX;
            break;
        case 'o':
            prefix = OBJ_PREFIX;
            break;
        case 'm':
            prefix = MOB_PREFIX;
            break;
        case 's':
            prefix = SHP_PREFIX;
            break;
        case 't':
            prefix = TRG_PREFIX;
            break;
        case 'g':
            prefix = GLD_PREFIX;
            break;
        default:
            /*
             * Caller messed up
             */
            return;
    }

    snprintf(old_name, sizeof(old_name), "%s/index", prefix);
    snprintf(new_name, sizeof(new_name), "%s/newindex", prefix);

    if (!(oldfile = fopen(old_name, "r"))) {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Failed to open %s.", old_name);
        return;
    } else if (!(newfile = fopen(new_name, "w"))) {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Failed to open %s.", new_name);
        fclose(oldfile);
        return;
    }

    /*
     * Index contents must be in order: search through the old file for the
     * right place, insert the new file, then copy the rest over.
     */
    snprintf(buf1, sizeof(buf1), "%d.%s", znum, type);
    while (get_line(oldfile, buf)) {
        if (*buf == '$') {
            /*
             * The following used to add a blank line, thanks to Brian Taylor for the fix... (Mythran)
             */
            fprintf(newfile, "%s", (!found ? strncat(buf1, "\n$\n", sizeof(buf1) - 1) : "$\n"));
            break;
        } else if (!found) {
            sscanf(buf, "%d", &num);
            if (num == znum) {
                found = true;
            } else if (num > znum) {
                found = true;
                fprintf(newfile, "%s\n", buf1);
            }
        }
        fprintf(newfile, "%s\n", buf);
    }

    fclose(newfile);
    fclose(oldfile);
    /*
     * Out with the old, in with the new.
     */
    remove(old_name);
    rename(new_name, old_name);
}



/*-------------------------------------------------------------------*/

/*
 * Save all the zone_table for this zone to disk.  This function now
 * writes simple comments in the form of (<name>) to each record.  A
 * header for each field is also there.
 */
int save_zone(zone_rnum zone_num) {
    return true;
}

/*-------------------------------------------------------------------*/

/*
 * Some common code to count the number of comands in the list.
 */
int count_commands(struct reset_com *list) {
    int count = 0;

    while (list[count].command != 'S')
        count++;

    return count;
}

/*-------------------------------------------------------------------*/

/*
 * Adds a new reset command into a list.  Takes a pointer to the list
 * so that it may play with the memory locations.
 */

/*-------------------------------------------------------------------*/


/*-------------------------------------------------------------------*/

/*
 * Error check user input and then add new (blank) command  
 */
int new_command(struct zone_data *zone, int pos) {
    int subcmd = zone->cmd.size();
    struct reset_com new_com;
    new_com.command = 'N';

    /* * Error check to ensure users hasn't given too large an index  */

    if (pos < 0 || pos > subcmd)
        return 0;
    zone->cmd.insert(zone->cmd.begin()+pos, new_com);
    return 1;
}

/*-------------------------------------------------------------------*/

/*
 * Error check user input and then remove command  
 */
void delete_zone_command(struct zone_data *zone, int pos) {
    zone->cmd.erase(zone->cmd.begin()+pos);
}

/*-------------------------------------------------------------------*/


zone_data::~zone_data() {
    if(name) free(name);
    if(builders) free(builders);
}


void zone_data::remove_room_commands(room_vnum rv) {
    room_vnum cmd_room = NOTHING;
    for(auto &c : cmd) {
        if(c.command == 'S') break;
        switch(c.command) {
            case 'M':
            case 'O':
            case 'T':
            case 'V':
                cmd_room = c.arg3;
                break;
            case 'D':
            case 'R':
                cmd_room = c.arg1;
                break;
            default:
                break;
        }
        if(cmd_room == rv) c.command = 'X';
    }
    // now filter out any where c.command == 'X'
    cmd.erase(std::remove_if(cmd.begin(), cmd.end(), [](const reset_com &c) { return c.command == 'X'; }), cmd.end());
}