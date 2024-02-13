/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/spec_assign.h"
#include "dbat/db.h"
#include "dbat/interpreter.h"
#include "dbat/utils.h"
#include "dbat/spec_procs.h"
#include "dbat/mail.h"

/* local functions */
void ASSIGNROOM(room_vnum room, SPECIAL(fname));

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));

void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname)) {
    mob_rnum rnum;

    if ((rnum = real_mobile(mob)) != NOBODY)
        mob_index[rnum].func = fname;
    else if (!mini_mud)
        basic_mud_log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname)) {
    obj_rnum rnum;

    if ((rnum = real_object(obj)) != NOTHING)
        obj_index[rnum].func = fname;
    else if (!mini_mud)
        basic_mud_log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

void ASSIGNROOM(room_vnum room, SPECIAL(fname)) {
    room_rnum rnum;

    if ((rnum = real_room(room)) != NOWHERE)
        dynamic_cast<room_data*>(world[rnum])->func = fname;
    else if (!mini_mud)
        basic_mud_log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles() {
    ASSIGNMOB(3010, postmaster);

}


/* assign special procedures to objects */
void assign_objects() {
    ASSIGNOBJ(3034, bank);    /* atm */
    ASSIGNOBJ(3036, bank);    /* cashcard */
    ASSIGNOBJ(11, gravity);       /* gravity generator */
    ASSIGNOBJ(65, healtank);      /* Healing Tank */
    ASSIGNOBJ(3, augmenter);      /* Augmenter 9001 */
}


/* assign special procedures to rooms */
void assign_rooms() {
    room_rnum i;

    ASSIGNROOM(5, dump);
    ASSIGNROOM(3, pet_shops);
    ASSIGNROOM(4, pet_shops);
    ASSIGNROOM(81, auction);
    ASSIGNROOM(82, auction);
    ASSIGNROOM(83, auction);
    ASSIGNROOM(84, auction);
    ASSIGNROOM(85, auction);
    ASSIGNROOM(86, auction);
    /* Gauntlet rooms track how far a player progressed into zone  Jamdog - 13th Feb 2006 */

    if (CONFIG_DTS_ARE_DUMPS)
        for (auto &[vn, u] : world) {
            auto r = dynamic_cast<room_data *>(u);
            if (!r) continue;
            if (ROOM_FLAGGED(r, ROOM_DEATH))
                r->func = dump;
        }
}
