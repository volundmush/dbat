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
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <regex>

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
void parse_trigger(FILE *trig_f, trig_vnum nr) {
    int t[2], k, attach_type;
    char line[256], *cmds, *s, flags[256], errors[MAX_INPUT_LENGTH];
    struct cmdlist_element *cle;
    auto &idx = trig_index[nr];
    auto *trig = &idx;

    trig->vn = nr;

    auto& z = zone_table.at(real_zone_by_thing(nr));
    z.triggers.insert(nr);

    snprintf(errors, sizeof(errors), "trig vnum %d", nr);

    char *buf = fread_string(trig_f, errors);

    trig->name = buf;

    get_line(trig_f, line);
    k = sscanf(line, "%d %s %d", &attach_type, flags, t);
    trig->attach_type = static_cast<UnitType>(attach_type);
    trig->trigger_type = (long) asciiflag_conv(flags);
    trig->narg = (k == 3) ? t[0] : 0;

    buf = fread_string(trig_f, errors);

    trig->arglist = buf ? buf : "";

    free(buf);

    cmds = s = fread_string(trig_f, errors);

    std::vector<std::string> lines;
    boost::split_regex(lines, cmds, boost::regex("\r\n|\r|\n"));

    trig->lines = parse_script(lines);

    free(cmds);
}

/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
std::shared_ptr<DgScript> read_trigger(int nr) {

    auto idx = trig_index.find(nr);
    if(idx == trig_index.end()) return nullptr;

    auto sh = std::make_shared<DgScript>(idx->second);

    return sh;
}


/* for mobs and rooms: */
void dg_read_trigger(FILE *fp, HasDgScripts *proto, UnitType type) {
    char line[READ_SIZE];
    char junk[8];
    int vnum, rnum, count;
    Character *mob;
    Room *room;
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
            case OBJ_TRIGGER:
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (object: %s - %d)",
                       vnum, static_cast<Object*>(proto)->getName(), proto->getVnum());
                break;
            case MOB_TRIGGER:
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (mob: %s - %d)",
                       vnum, GET_NAME(static_cast<Character *>(proto)), GET_MOB_VNUM(static_cast<Character *>(proto)));
                break;
            case WLD_TRIGGER:
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (room:%d)",
                       vnum, GET_ROOM_VNUM(static_cast<Room *>(proto)->getVnum()));
                break;
            default:
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (?)", vnum);
                break;
        }
        return;
    }

    switch(type) {
        case OBJ_TRIGGER:
            if(!obj_proto.contains(proto->getVnum())) {
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (object: %s - %d)",
                       vnum, static_cast<Object*>(proto)->getName(), proto->getVnum());
                return;
            }
            obj_proto.at(proto->getVnum()).proto_script.push_back(rnum);
            break;
        case MOB_TRIGGER:
            if(!mob_proto.contains(proto->getVnum())) {
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (mob: %s - %d)",
                       vnum, GET_NAME(static_cast<Character *>(proto)), GET_MOB_VNUM(static_cast<Character *>(proto)));
                return;
            }
            mob_proto.at(proto->getVnum()).proto_script.push_back(rnum);
            break;
        case WLD_TRIGGER:
            if(!world.contains(proto->getVnum())) {
                mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (room:%d)",
                       vnum, GET_ROOM_VNUM(static_cast<Room *>(proto)->getVnum()));
                return;
            }
            world.at(proto->getVnum())->proto_script.push_back(rnum);
            break;
        default:
            mudlog(BRF, ADMLVL_BUILDER, true,
                   "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (?)", vnum);
            break;
    }
}

void dg_read_trigger(FILE *fp, struct ThingPrototype *proto, UnitType type) {
    char line[READ_SIZE];
    char junk[8];
    int vnum, rnum, count;
    Character *mob;
    Room *room;
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
        mudlog(BRF, ADMLVL_BUILDER, true,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (?)", vnum);
        return;
    }

    proto->proto_script.push_back(rnum);

}

void dg_obj_trigger(char *line, ObjectPrototype *obj) {
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
               vnum, obj->short_description, obj->vn);
        return;
    }

    obj->proto_script.push_back(rnum);
}

void assign_triggers(HasDgScripts *i, UnitType type) {

    if(i->running_scripts.has_value()) {
        // If this value is set, then scripts have been manually assigned via attach or detach.
        // We do not want to overwrite this.
        return;
    }

    auto ps = i->getProtoScript();

    std::unordered_set<trig_vnum> to_remove;
    // step 1: iterate through i->scripts and add any vnums not in ps to to_remove.
    for (const auto &[tvn, t] : i->scripts) {
        if (std::find(ps.begin(), ps.end(), tvn) == ps.end()) {
            to_remove.insert(tvn);
        }
    }

    // step 2: remove any triggers in to_remove from i->scripts.
    for(auto tvn : to_remove) {
        i->scripts.erase(tvn);
    }

    // Step 3: iterate through ps and add any vnums not in i->scripts.
   for(auto &tvn : ps) {
       if(i->scripts.find(tvn) == i->scripts.end()) {
            auto t = read_trigger(tvn);
            SCRIPT_TYPES(i) |= GET_TRIG_TYPE(t);
            i->scripts.emplace(t->getVnum(), t);
            t->owner = i;
            t->activate();
       }
   }
}
