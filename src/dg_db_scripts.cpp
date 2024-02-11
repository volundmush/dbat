/* ************************************************************************
*  File: dg_db_scripts.c                         Part of Death's Gate MUD *
*                                                                         *
*  Usage: Contains routines to handle db functions for scripts and trigs  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
************************************************************************ */

#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/dg_event.h"
#include "dbat/comm.h"
#include "dbat/constants.h"

extern bitvector_t asciiflag_conv(char *flag);

/* local functions */
void trig_data_init(trig_data *this_data);

/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
std::shared_ptr<trig_data> read_trigger(int nr) {

    auto proto = trig_index.find(nr);
    if(proto == trig_index.end()) return nullptr;

    auto trig = std::make_shared<trig_data>(proto->second);

    return trig;
}


void trig_data_init(trig_data *this_data) {

}


void trig_data_copy(trig_data *this_data, const trig_data *trg) {

}

/* for mobs and rooms: */
void dg_read_trigger(FILE *fp, struct unit_data *proto, int type) {
    char line[READ_SIZE];
    char junk[8];
    int vnum, rnum, count;
    char_data *mob;
    room_data *room;
    struct trig_proto_list *trg_proto, *new_trg;

    get_line(fp, line);
    count = sscanf(line, "%7s %d", junk, &vnum);

    if (count != 2) {
        mudlog(BRF, ADMLVL_BUILDER, true,
               "SYSERR: Error assigning trigger! - Line was\n  %s", line);
        return;
    }

    rnum = real_trigger(vnum);
    if (rnum == NOTHING) {
        switch (type) {
            case MOB_TRIGGER:
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (mob: %s - %d)",
                       vnum, GET_NAME((char_data *) proto), GET_MOB_VNUM((char_data *) proto));
                break;
            case WLD_TRIGGER:
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (room:%d)",
                       vnum, GET_ROOM_VNUM(((room_data *) proto)->vn));
                break;
            default:
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (?)", vnum);
                break;
        }
        return;
    }

    proto->proto_script.push_back(rnum);
}

void dg_obj_trigger(char *line, struct obj_data *obj) {
    char junk[8];
    int vnum, rnum, count;
    struct trig_proto_list *trg_proto, *new_trg;

    count = sscanf(line, "%s %d", junk, &vnum);

    if (count != 2) {
        mudlog(BRF, ADMLVL_BUILDER, true,
               "SYSERR: dg_obj_trigger() : Error assigning trigger! - Line was:\n  %s", line);
        return;
    }

    rnum = real_trigger(vnum);
    if (rnum == NOTHING) {
        mudlog(BRF, ADMLVL_BUILDER, true,
               "SYSERR: Trigger vnum #%d asked for but non-existant! (Object: %s - %d)",
               vnum, obj->short_description, GET_OBJ_VNUM(obj));
        return;
    }

    obj->proto_script.push_back(rnum);
}

void assign_triggers(struct unit_data *i, int type) {

    // remove all duplicates from i->proto_script but do not change its order otherwise.
    std::set<trig_vnum> existVnums;
    for(auto t : i->script->dgScripts) existVnums.insert(t->parent->vn);

    for(auto p : i->proto_script) {
        // only add if they don't already have one...
        if(!existVnums.contains(p)) {
            i->script->addTrigger(read_trigger(p), -1);
            existVnums.insert(p);
        }
    }
}
