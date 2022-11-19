/************************************************************************
 * Generic OLC Library - Rooms / genwld.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "genwld.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "genolc.h"
#include "shop.h"
#include "dg_olc.h"
#include "htree.h"


/*
 * This function will copy the strings so be sure you free your own
 * copies of the description, title, and such.
 */
room_rnum add_room(struct room_data *room) {
    struct char_data *tch;
    struct obj_data *tobj;
    vnum j, found = false;
    room_rnum i;

    if (!room)
        return NOWHERE;

    if ((i = real_room(room->number)) != NOWHERE) {
        if (SCRIPT(&world[i]))
            extract_script(&world[i], WLD_TRIGGER);
        tch = world[i].people;
        tobj = world[i].contents;
        copy_room(&world[i], room);
        world[i].people = tch;
        world[i].contents = tobj;
        add_to_save_list(zone_table[room->zone].number, SL_WLD);
        log("GenOLC: add_room: Updated existing room #%d.", room->number);
        return i;
    }

    auto &r = world[room->number];
    r = *room;
    log("GenOLC: add_room: Added room %d.", room->number);
    add_to_save_list(zone_table[room->zone].number, SL_WLD);

    /*
     * Return what array entry we placed the new room in.
     */
    return found;
}

/* -------------------------------------------------------------------------- */

int delete_room(room_rnum rnum) {
    room_rnum i;
    int j;
    struct char_data *ppl, *next_ppl;
    struct obj_data *obj, *next_obj;
    struct room_data *room;

    if (!world.count(rnum))    /* Can't delete void yet. */
        return false;

    room = &world[rnum];

    add_to_save_list(zone_table[room->zone].number, SL_WLD);

    /* This is something you might want to read about in the logs. */
    log("GenOLC: delete_room: Deleting room #%d (%s).", room->number, room->name);

    if (r_mortal_start_room == rnum) {
        log("WARNING: GenOLC: delete_room: Deleting mortal start room!");
        r_mortal_start_room = 0;    /* The Void */
    }
    if (r_immort_start_room == rnum) {
        log("WARNING: GenOLC: delete_room: Deleting immortal start room!");
        r_immort_start_room = 0;    /* The Void */
    }
    if (r_frozen_start_room == rnum) {
        log("WARNING: GenOLC: delete_room: Deleting frozen start room!");
        r_frozen_start_room = 0;    /* The Void */
    }

    /*
     * Dump the contents of this room into the Void.  We could also just
     * extract the people, mobs, and objects here.
     */
    for (obj = world[rnum].contents; obj; obj = next_obj) {
        next_obj = obj->next_content;
        obj_from_room(obj);
        obj_to_room(obj, 0);
    }
    for (ppl = world[rnum].people; ppl; ppl = next_ppl) {
        next_ppl = ppl->next_in_room;
        char_from_room(ppl);
        char_to_room(ppl, 0);
    }

    free_room_strings(room);
    if (SCRIPT(room))
        extract_script(room, WLD_TRIGGER);
    free_proto_script(room, WLD_TRIGGER);

    /*
     * Change any exit going to this room to go the void.
     * Also fix all the exits pointing to rooms above this.
     */

    for(auto &r : world) {
        for (j = 0; j < NUM_OF_DIRS; j++) {
            auto &e = r.second.dir_option[j];
            if (!e || e->to_room != rnum)
                continue;
            if ((!e->keyword || !*e->keyword) &&
                (!e->general_description || !*e->general_description)) {
                /* no description, remove exit completely */
                if (e->keyword)
                    free(e->keyword);
                if (e->general_description)
                    free(e->general_description);
                free(e);
                e = nullptr;
            } else {
                /* description is set, just point to nowhere */
                e->to_room = NOWHERE;
            }
        }

    };

    /*
     * Remove this room from all shop lists.
     */
    {
        for (i = 0; i < top_shop; i++) {
            for (j = 0; SHOP_ROOM(i, j) != NOWHERE; j++) {
                if (SHOP_ROOM(i, j) == rnum)
                    SHOP_ROOM(i, j) = 0; /* set to the void */
            }
        }
    }
    return true;
}


int save_rooms(zone_rnum zone_num) {
    int i;
    struct room_data *room;
    FILE *sf;
    char filename[128];
    char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
    char rbuf1[MAX_STRING_LENGTH], rbuf2[MAX_STRING_LENGTH];
    char rbuf3[MAX_STRING_LENGTH], rbuf4[MAX_STRING_LENGTH];

    if (!zone_table.count(zone_num)) {
        log("SYSERR: GenOLC: save_rooms: Invalid zone number %d passed!", zone_num);
        return false;
    }
    auto &z = zone_table[zone_num];
    log("GenOLC: save_rooms: Saving rooms in zone #%d (%d-%d).",
        z.number, z.bot, z.top);

    snprintf(filename, sizeof(filename), "%s%d.new", WLD_PREFIX, z.number);
    if (!(sf = fopen(filename, "w"))) {
        perror("SYSERR: save_rooms");
        return false;
    }

    for (auto &i : z.rooms) {
        room_rnum rnum;

        if ((rnum = real_room(i)) != NOWHERE) {
            int j;

            room = &world[rnum];

            /*
             * Copy the description and strip off trailing newlines.
             */
            strncpy(buf, room->description ? room->description : "Empty room.", sizeof(buf) - 1);
            strip_cr(buf);

            /*
             * Save the numeric and string section of the file.
             */
            sprintascii(rbuf1, room->room_flags[0]);
            sprintascii(rbuf2, room->room_flags[1]);
            sprintascii(rbuf3, room->room_flags[2]);
            sprintascii(rbuf4, room->room_flags[3]);
            fprintf(sf, "#%d\n"
                        "%s%c\n"
                        "%s%c\n"
                        "%d %s %s %s %s %d\n",
                    room->number,
                    room->name ? room->name : "Untitled", STRING_TERMINATOR,
                    buf, STRING_TERMINATOR,
                    zone_table[room->zone].number,
                    rbuf1, rbuf2, rbuf3, rbuf4, room->sector_type
            );

            /*
             * Now you write out the exits for the room.
             */
            for (j = 0; j < NUM_OF_DIRS; j++) {
                if (R_EXIT(room, j)) {
                    int dflag;
                    if (R_EXIT(room, j)->general_description) {
                        strncpy(buf, R_EXIT(room, j)->general_description, sizeof(buf) - 1);
                        strip_cr(buf);
                    } else
                        *buf = '\0';

                    /*
                     * Figure out door flag.
                     */
                    if (IS_SET(R_EXIT(room, j)->exit_info, EX_ISDOOR)) {
                        if (IS_SET(R_EXIT(room, j)->exit_info, EX_SECRET) &&
                            IS_SET(R_EXIT(room, j)->exit_info, EX_PICKPROOF))
                            dflag = 4;
                        else if (IS_SET(R_EXIT(room, j)->exit_info, EX_SECRET))
                            dflag = 3;
                        else if (IS_SET(R_EXIT(room, j)->exit_info, EX_PICKPROOF))
                            dflag = 2;
                        else
                            dflag = 1;
                    } else
                        dflag = 0;

                    if (R_EXIT(room, j)->keyword)
                        strncpy(buf1, R_EXIT(room, j)->keyword, sizeof(buf1) - 1);
                    else
                        *buf1 = '\0';

                    /*
                     * Now write the exit to the file.
                     */
                    fprintf(sf, "D%d\n"
                                "%s~\n"
                                "%s~\n"
                                "%d %d %d %d %d %d %d %d %d %d %d\n", j, buf, buf1, dflag,
                            R_EXIT(room, j)->key != NOTHING ? R_EXIT(room, j)->key : -1,
                            R_EXIT(room, j)->to_room != NOWHERE ? world[R_EXIT(room, j)->to_room].number : -1,
                            R_EXIT(room, j)->dclock, R_EXIT(room, j)->dchide,
                            R_EXIT(room, j)->dcskill, R_EXIT(room, j)->dcmove,
                            R_EXIT(room, j)->failsavetype, R_EXIT(room, j)->dcfailsave,
                            R_EXIT(room, j)->failroom, R_EXIT(room, j)->totalfailroom);

                }
            }

            if (room->ex_description) {
                struct extra_descr_data *xdesc;

                for (xdesc = room->ex_description; xdesc; xdesc = xdesc->next) {
                    strncpy(buf, xdesc->description, sizeof(buf));
                    strip_cr(buf);
                    fprintf(sf, "E\n"
                                "%s~\n"
                                "%s~\n", xdesc->keyword, buf);
                }
            }
            fprintf(sf, "S\n");
            script_save_to_disk(sf, room, WLD_TRIGGER);
        }
    }

    /*
     * Write the final line and close it.
     */
    fprintf(sf, "$~\n");
    fclose(sf);

    /* Old file we're replacing. */
    snprintf(buf, sizeof(buf), "%s%d.wld", WLD_PREFIX, z.number);

    remove(buf);
    rename(filename, buf);

    if (in_save_list(z.number, SL_WLD)) {
        remove_from_save_list(z.number, SL_WLD);
        create_world_index(z.number, "wld");
        log("GenOLC: save_rooms: Saving rooms '%s'", buf);
    }
    return true;
}

int copy_room(struct room_data *to, struct room_data *from) {
    free_room_strings(to);
    *to = *from;
    copy_room_strings(to, from);

    /* Don't put people and objects in two locations.
       Am thinking this shouldn't be done here... */
    from->people = nullptr;
    from->contents = nullptr;

    return true;
}

/* -------------------------------------------------------------------------- */

/*
 * Copy strings over so bad things don't happen.  We do not free the
 * existing strings here because copy_room() did a shallow copy previously
 * and we'd be freeing the very strings we're copying.  If this function
 * is used elsewhere, be sure to free_room_strings() the 'dest' room first.
 */
int copy_room_strings(struct room_data *dest, struct room_data *source) {
    int i;

    if (dest == nullptr || source == nullptr) {
        log("SYSERR: GenOLC: copy_room_strings: nullptr values passed.");
        return false;
    }

    dest->description = str_udup(source->description);
    dest->name = str_udup(source->name);

    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (!R_EXIT(source, i))
            continue;

        CREATE(R_EXIT(dest, i), struct room_direction_data, 1);
        *R_EXIT(dest, i) = *R_EXIT(source, i);
        if (R_EXIT(source, i)->general_description)
            R_EXIT(dest, i)->general_description = strdup(R_EXIT(source, i)->general_description);
        if (R_EXIT(source, i)->keyword)
            R_EXIT(dest, i)->keyword = strdup(R_EXIT(source, i)->keyword);
    }

    if (source->ex_description)
        copy_ex_descriptions(&dest->ex_description, source->ex_description);

    return true;
}

int free_room_strings(struct room_data *room) {
    int i;

    /* Free descriptions. */
    if (room->name)
        free(room->name);
    if (room->description)
        free(room->description);
    if (room->ex_description)
        free_ex_descriptions(room->ex_description);

    /* Free exits. */
    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (room->dir_option[i]) {
            if (room->dir_option[i]->general_description)
                free(room->dir_option[i]->general_description);
            if (room->dir_option[i]->keyword)
                free(room->dir_option[i]->keyword);
            free(room->dir_option[i]);
            room->dir_option[i] = nullptr;
        }
    }

    return true;
}
