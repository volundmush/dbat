/************************************************************************
 * Generic OLC Library - Zones / genzon.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "genzon.h"
#include "utils.h"

#include "genolc.h"
#include "dg_scripts.h"

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
    if (vzone_num == NOWHERE) {
#else
        if (vzone_num < 0) {
#endif
        *error = "You can't make negative zones.\r\n";
        return NOWHERE;
    } else if (bottom > top) {
        *error = "Bottom room cannot be greater than top room.\r\n";
        return NOWHERE;
    }

#if _CIRCLEMUD < CIRCLEMUD_VERSION(3, 0, 21)
    /*
     * New with bpl19, the OLC interface should decide whether
     * to allow overlap before calling this function. There
     * are more complicated rules for that but it's not covered
     * here.
     */
    if (vzone_num > 326) {
      *error = "326 is the highest zone allowed.\r\n";
      return NOWHERE;
    }

    /*
     * Make sure the zone does not exist.
     */
    room = vzone_num * 100; /* Old CircleMUD 100-zones. */
    for (i = 0; i <= top_of_zone_table; i++)
      if (genolc_zone_bottom(i) <= room && zone_table[i].top >= room) {
        *error = "A zone already covers that area.\r\n";
        return NOWHERE;
      }
#else
    if(zone_table.count(vzone_num)) {
            *error = "That virtual zone already exists.\r\n";
            return NOWHERE;
        }
#endif

    /*
     * Create the zone file.
     */
    snprintf(buf, sizeof(buf), "%s%d.zon", ZON_PREFIX, vzone_num);
    if (!(fp = fopen(buf, "w"))) {
        mudlog(BRF, ADMLVL_IMPL, true, "SYSERR: OLC: Can't write new zone file.");
        *error = "Could not write zone file.\r\n";
        return NOWHERE;
    }
#if _CIRCLEMUD >= CIRCLEMUD_VERSION(3, 0, 21)
    /* File format changed. */
    fprintf(fp, "#%d\nNone~\nNew Zone~\n%d %d 30 2\nS\n$\n", vzone_num, bottom, top);
#else
    fprintf(fp, "#%d\nNew Zone~\n%d 30 2\nS\n$\n", vzone_num, (vzone_num * 100) + 99);
#endif
    fclose(fp);

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
    z.zone_flags[0] = 0;
    z.zone_flags[1] = 0;
    z.zone_flags[2] = 0;
    z.zone_flags[3] = 0;
    z.min_level = 0;
    z.max_level = ADMLVL_IMPL;
    /*
     * No zone commands, just terminate it with an 'S'
     */
    auto &c = z.cmd.emplace_back();
    c.command = 'S';

    add_to_save_list(zone->number, SL_ZON);
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

void remove_room_zone_commands(zone_rnum zone, room_rnum room_num) {
    auto &z = zone_table[zone];
    std::remove_if(z.cmd.begin(), z.cmd.end(), [&](reset_com &c) {
        switch(c.command) {
            case 'M':
            case 'O':
            case 'T':
            case 'V':
                return room_num == c.arg3;
            case 'D':
            case 'R':
                return room_num == c.arg1;
            default:
                return false;
        }
    });
}

/*-------------------------------------------------------------------*/

/*
 * Save all the zone_table for this zone to disk.  This function now
 * writes simple comments in the form of (<name>) to each record.  A
 * header for each field is also there.
 */
int save_zone(zone_rnum zone_num) {
    int subcmd, arg1 = -1, arg2 = -1, arg3 = -1, arg4 = -1, arg5 = -1;
    char fname[128], oldname[128];
    const char *comment = nullptr;
    FILE *zfile;
    char zbuf1[MAX_STRING_LENGTH];
    char zbuf2[MAX_STRING_LENGTH];
    char zbuf3[MAX_STRING_LENGTH];
    char zbuf4[MAX_STRING_LENGTH];

    if (!zone_table.count(zone_num)) {
        log("SYSERR: GenOLC: save_zone: Invalid real zone number %d.", zone_num);
        return false;
    }
    auto &z = zone_table[zone_num];
    snprintf(fname, sizeof(fname), "%s%d.new", ZON_PREFIX, z.number);
    if (!(zfile = fopen(fname, "w"))) {
        mudlog(BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: save_zones:  Can't write zone %d.",
               z.number);
        return false;
    }

    /*
     * Print zone header to file
     */
    sprintascii(zbuf1, z.zone_flags[0]);
    sprintascii(zbuf2, z.zone_flags[1]);
    sprintascii(zbuf3, z.zone_flags[2]);
    sprintascii(zbuf4, z.zone_flags[3]);

    fprintf(zfile, "@Version: %d\n", CUR_ZONE_VERSION);
    fprintf(zfile, "#%d\n"
                   "%s~\n"
                   "%s~\n"
                   "%d %d %d %d %s %s %s %s %d %d\n",
            z.number,
            (z.builders && *z.builders)
            ? z.builders : "None.",
            (z.name && *z.name)
            ? z.name : "undefined",
            z.bot,
            z.top,
            z.lifespan,
            z.reset_mode,
            zbuf1, zbuf2, zbuf3, zbuf4,
            z.min_level,
            z.max_level
    );

    /*
     * Handy Quick Reference Chart for Zone Values.
     *
     * Field #1    Field #3   Field #4  Field #5           Field #6		Field #7
     * -----------------------------------------------------------------------------------------
     * M (Mobile)  Mob-Vnum   Wld-Max   Room-Vnum          room_max		Percent load failure
     * O (Object)  Obj-Vnum   Wld-Max   Room-Vnum          room_max		Percent load failure
     * G (Give)    Obj-Vnum   Wld-Max   Unused             unused		Percent load failure
     * E (Equip)   Obj-Vnum   Wld-Max   EQ-Position        unused		Percent load failure
     * P (Put)     Obj-Vnum   Wld-Max   Target-Obj-Vnum    unused		Percent load failure
     * D (Door)    Room-Vnum  Door-Dir  Door-State         unused		Percent load failure
     * R (Remove)  Room-Vnum  Obj-Vnum  Unused             unused		Percent load failure
         * T (Trigger) Trig-type  Trig-Vnum Room-Vnum          unused		Percent load failure
         * V (var)     Trig-type  Context   Room-Vnum Varname  unused		Percent load failure
     * -----------------------------------------------------------------------------------------
     */

    for (subcmd = 0; ZCMD(zone_num, subcmd).command != 'S'; subcmd++) {
        switch (ZCMD(zone_num, subcmd).command) {
            case 'M':
                arg1 = mob_index[ZCMD(zone_num, subcmd).arg1].vn;
                arg2 = ZCMD(zone_num, subcmd).arg2;
                arg3 = world[ZCMD(zone_num, subcmd).arg3].vn;
                arg4 = ZCMD(zone_num, subcmd).arg4;
                arg5 = ZCMD(zone_num, subcmd).arg5;
                comment = mob_proto[ZCMD(zone_num, subcmd).arg1].short_description;
                break;
            case 'O':
                arg1 = obj_index[ZCMD(zone_num, subcmd).arg1].vn;
                arg2 = ZCMD(zone_num, subcmd).arg2;
                arg3 = world[ZCMD(zone_num, subcmd).arg3].vn;
                arg4 = ZCMD(zone_num, subcmd).arg4;
                arg5 = ZCMD(zone_num, subcmd).arg5;
                comment = obj_proto[ZCMD(zone_num, subcmd).arg1].short_description;
                break;
            case 'G':
                arg1 = obj_index[ZCMD(zone_num, subcmd).arg1].vn;
                arg2 = ZCMD(zone_num, subcmd).arg2;
                arg3 = -1;
                arg4 = -1;
                arg5 = ZCMD(zone_num, subcmd).arg5;
                comment = obj_proto[ZCMD(zone_num, subcmd).arg1].short_description;
                break;
            case 'E':
                arg1 = obj_index[ZCMD(zone_num, subcmd).arg1].vn;
                arg2 = ZCMD(zone_num, subcmd).arg2;
                arg3 = ZCMD(zone_num, subcmd).arg3;
                arg4 = -1;
                arg5 = ZCMD(zone_num, subcmd).arg5;
                comment = obj_proto[ZCMD(zone_num, subcmd).arg1].short_description;
                break;
            case 'P':
                arg1 = obj_index[ZCMD(zone_num, subcmd).arg1].vn;
                arg2 = ZCMD(zone_num, subcmd).arg2;
                arg3 = obj_index[ZCMD(zone_num, subcmd).arg3].vn;
                arg4 = -1;
                arg5 = ZCMD(zone_num, subcmd).arg5;
                comment = obj_proto[ZCMD(zone_num, subcmd).arg1].short_description;
                break;
            case 'D':
                arg1 = world[ZCMD(zone_num, subcmd).arg1].vn;
                arg2 = ZCMD(zone_num, subcmd).arg2;
                arg3 = ZCMD(zone_num, subcmd).arg3;
                comment = world[ZCMD(zone_num, subcmd).arg1].name;
                break;
            case 'R':
                arg1 = world[ZCMD(zone_num, subcmd).arg1].vn;
                arg2 = obj_index[ZCMD(zone_num, subcmd).arg2].vn;
                comment = obj_proto[ZCMD(zone_num, subcmd).arg2].short_description;
                arg3 = -1;
                break;
            case 'T':
                arg1 = ZCMD(zone_num, subcmd).arg1; /* trigger type */
                arg2 = trig_index[ZCMD(zone_num, subcmd).arg2].vn; /* trigger vnum */
                arg3 = world[ZCMD(zone_num, subcmd).arg3].vn; /* room num */
                arg4 = -1;
                arg5 = ZCMD(zone_num, subcmd).arg5;
                comment = GET_TRIG_NAME(trig_index[real_trigger(arg2)].proto);
                break;
            case 'V':
                arg1 = ZCMD(zone_num, subcmd).arg1; /* trigger type */
                arg2 = ZCMD(zone_num, subcmd).arg2; /* context */
                arg3 = world[ZCMD(zone_num, subcmd).arg3].vn;
                arg4 = -1;
                arg5 = ZCMD(zone_num, subcmd).arg5;
                break;
            case '*':
                /*
                 * Invalid commands are replaced with '*' - Ignore them.
                 */
                continue;
            default:
                mudlog(BRF, ADMLVL_BUILDER, true, "SYSERR: OLC: z_save_to_disk(): Unknown cmd '%c' - NOT saving",
                       ZCMD(zone_num, subcmd).command);
                continue;
        }
        if (ZCMD(zone_num, subcmd).command != 'V')
            fprintf(zfile, "%c %d %d %d %d %d %d \t(%s)\n",
                    ZCMD(zone_num, subcmd).command, ZCMD(zone_num, subcmd).if_flag, arg1, arg2, arg3, arg4, arg5,
                    comment);
        else
            fprintf(zfile, "%c %d %d %d %d %d %d %s %s\n",
                    ZCMD(zone_num, subcmd).command, ZCMD(zone_num, subcmd).if_flag, arg1, arg2, arg3, arg4, arg5,
                    ZCMD(zone_num, subcmd).sarg1, ZCMD(zone_num, subcmd).sarg2);
    }
    fputs("S\n$\n", zfile);
    fclose(zfile);
    snprintf(oldname, sizeof(oldname), "%s%d.zon", ZON_PREFIX, z.number);
    remove(oldname);
    rename(fname, oldname);

    if (in_save_list(z.number, SL_ZON)) {
        remove_from_save_list(z.number, SL_ZON);
        create_world_index(z.number, "zon");
        log("GenOLC: save_zone: Saving zone '%s'", oldname);
    }
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
    int subcmd = 0;
    struct reset_com new_com;
    new_com.command = 'N';

    /* * Error check to ensure users hasn't given too large an index  */
    while (zone->cmd[subcmd].command != 'S')
        subcmd++;

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
