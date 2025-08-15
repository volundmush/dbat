/************************************************************************
 * Generic OLC Library - Rooms / genwld.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include "dbat/genwld.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/genolc.h"
#include "dbat/shop.h"
#include "dbat/constants.h"
#include "dbat/planet.h"
#include "dbat/constants.h"
#include "dbat/filter.h"
#include "dbat/dg_scripts.h"
#include "dbat/send.h"



room_rnum add_room(Room *room)
{
    Character *tch;
    Object *tobj;
    vnum j, found = false;
    room_rnum i;

    if (!room)
        return NOWHERE;

    auto vn = room->getVnum();

    if (world.contains(vn))
    {
        auto ro = world.at(vn).get();
        extract_script(ro, WLD_TRIGGER);
        copy_room(ro, room);
        basic_mud_log("GenOLC: add_room: Updated existing room #%d.", vn);
        assign_triggers(ro, WLD_TRIGGER);
        return i;
    }
    auto sh = std::shared_ptr<Room>(room);
    world.emplace(vn, sh);
    basic_mud_log("GenOLC: add_room: Added room %d.", vn);

    /*
     * Return what array entry we placed the new room in.
     */
    return found;
}

/* -------------------------------------------------------------------------- */

int delete_room(room_rnum rnum)
{
    room_rnum i;
    int j;
    Character *ppl, *next_ppl;
    Object *obj, *next_obj;
    Room *room;

    if (!world.count(rnum)) /* Can't delete void yet. */
        return false;

    if (rnum <= 0)
    {
        basic_mud_log("SYSERR: GenOLC: delete_room: Attempt to delete room with vnum <= 0.");
        return false;
    }

    room = get_room(rnum);

    /* This is something you might want to read about in the logs. */
    basic_mud_log("GenOLC: delete_room: Deleting room #%d (%s).", room->getVnum(), room->getName());

    if (r_mortal_start_room == rnum)
    {
        basic_mud_log("WARNING: GenOLC: delete_room: Deleting mortal start room!");
        r_mortal_start_room = 0; /* The Void */
    }

    if (r_immort_start_room == rnum)
    {
        basic_mud_log("WARNING: GenOLC: delete_room: Deleting immortal start room!");
        r_immort_start_room = 0; /* The Void */
    }

    if (r_frozen_start_room == rnum)
    {
        basic_mud_log("WARNING: GenOLC: delete_room: Deleting frozen start room!");
        r_frozen_start_room = 0; /* The Void */
    }

    /*
     * Dump the contents of this room into the Void.  We could also just
     * extract the people, mobs, and objects here.
     */

    auto con = room->getObjects();
    for (auto obj : filter_raw(con))
    {
        obj->leaveLocation();
        obj->moveToLocation(0);
    }
    auto people = room->getPeople();
    for (auto ppl : filter_raw(people))
    {
        ppl->leaveLocation();
        ppl->moveToLocation(0);
    }

    extract_script(room, WLD_TRIGGER);
    room->proto_script.clear();

    /*
     * Change any exit going to this room to go the void.
     * Also fix all the exits pointing to rooms above this.
     */

    for (auto &[vn, r] : world)
    {
    };

    /*
     * Remove this room from all shop lists.
     */
    for (auto &[i, sh] : shop_index)
    {
        sh.in_room.erase(rnum);
    }

    world.erase(rnum);
    return true;
}

int save_rooms(zone_rnum zone_num)
{
    return true;
}

int copy_room(Room *to, Room *from)
{
    // TODO: Fix this.
    free_room_strings(to);
    *to = *from;
    copy_room_strings(to, from);

    /* Don't put people and objects in two locations.
       Am thinking this shouldn't be done here... */

    return true;
}

/* -------------------------------------------------------------------------- */

/*
 * Copy strings over so bad things don't happen.  We do not free the
 * existing strings here because copy_room() did a shallow copy previously
 * and we'd be freeing the very strings we're copying.  If this function
 * is used elsewhere, be sure to free_room_strings() the 'dest' room first.
 */
int copy_room_strings(Room *dest, Room *source)
{
    int i;

    if (dest == nullptr || source == nullptr)
    {
        basic_mud_log("SYSERR: GenOLC: copy_room_strings: nullptr values passed.");
        return false;
    }

    dest->strings = source->strings;
    dest->extra_descriptions = source->extra_descriptions;

    for (auto &[d, e] : source->exits)
    {
        dest->exits[d].keyword = e.keyword;
        dest->exits[d].general_description = e.general_description;
    }

    return true;
}

int free_room_strings(Room *room)
{
    int i;

    /* Free descriptions. */
    room->strings.clear();
    room->extra_descriptions.clear();

    /* Free exits. */
    for (auto &[d, e] : room->exits)
    {
        e.keyword.clear();
        e.general_description.clear();
    }

    return true;
}

/*
nlohmann::json Room::serializeDgVars() {
    if(global_vars)
        return serializeVars(global_vars);
    return nlohmann::json::array();
}
*/





