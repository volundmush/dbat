/******************************************************************************/
/** OasisOLC - InGame OLC Listings                                     v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/

#include "dbat/structs.h"
#include "dbat/send.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/db.h"
#include "dbat/oasis.h"
#include "dbat/shop.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"
#include "dbat/guild.h"
#include "dbat/races.h"
#include "dbat/class.h"


/******************************************************************************/
/** Internal Functions                                                       **/
/******************************************************************************/
void list_rooms(Character *ch, zone_rnum rnum, room_vnum vmin, room_vnum vmax);

void list_mobiles(Character *ch, zone_rnum rnum, mob_vnum vmin, mob_vnum vmax);

void list_objects(Character *ch, zone_rnum rnum, obj_vnum vmin, obj_vnum vmax);

void list_shops(Character *ch, zone_rnum rnum, shop_vnum vmin, shop_vnum vmax);

void list_triggers(Character *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax);

void list_zones(Character *ch);

void print_zone(Character *ch, zone_vnum vnum);

void list_guilds(Character *ch, zone_rnum rnum, guild_vnum vmin, guild_vnum vmax);


/******************************************************************************/
/** Ingame Commands                                                          **/
/******************************************************************************/
ACMD(do_oasis_list) {
    zone_rnum rzone = NOWHERE;
    room_rnum vmin = NOWHERE;
    room_rnum vmax = NOWHERE;
    char smin[MAX_INPUT_LENGTH];
    char smax[MAX_INPUT_LENGTH];

    two_arguments(argument, smin, smax);

    if (subcmd == SCMD_OASIS_ZLIST) { /* special case */
        if (smin && *smin && is_number(smin))
            print_zone(ch, atoi(smin));
        else
            list_zones(ch);
        return;
    }

    if (!*smin || *smin == '.') {
        rzone = ch->location.getZone()->number;
    } else if (!*smax) {
        rzone = real_zone(atoi(smin));

        if (rzone == NOWHERE) {
                        ch->sendText("Sorry, there's no zone with that number\r\n");
            return;
        }
    } else {
        /** Listing by min vnum / max vnum.  Retrieve the numeric values.          **/
        vmin = atoi(smin);
        vmax = atoi(smax);

        if (vmin + 1500 < vmax) {
                        ch->sendText("Really? Over 1500?! You need to view that many at once? Come on...\r\n");
            return;
        }
        if (vmin > vmax) {
                        ch->send_to("List from %d to %d - Aren't we funny today!\r\n", vmin, vmax);
            return;
        }
    }

    switch (subcmd) {
        case SCMD_OASIS_RLIST:
            list_rooms(ch, rzone, vmin, vmax);
            break;
        case SCMD_OASIS_OLIST:
            list_objects(ch, rzone, vmin, vmax);
            break;
        case SCMD_OASIS_MLIST:
            list_mobiles(ch, rzone, vmin, vmax);
            break;
        case SCMD_OASIS_TLIST:
            list_triggers(ch, rzone, vmin, vmax);
            break;
        case SCMD_OASIS_SLIST:
            list_shops(ch, rzone, vmin, vmax);
            break;
        case SCMD_OASIS_GLIST:
            list_guilds(ch, rzone, vmin, vmax);
            break;
        default:
                        ch->sendText("You can't list that!\r\n");
            mudlog(BRF, ADMLVL_IMMORT, true,
                   "SYSERR: do_oasis_list: Unknown list option: %d", subcmd);
    }
}

ACMD(do_oasis_links) {
    zone_rnum zrnum;
    zone_vnum zvnum;
    room_rnum nr;
    int first, last, j;
    char arg[MAX_INPUT_LENGTH];

    skip_spaces(&argument);
    one_argument(argument, arg);

    if ((!arg || !*arg) || !strcmp(arg, ".")) {
        zvnum = ch->location.getZone()->number;
    } else {
        zvnum = atol(arg);
    }

    auto z = zone_table.find(zvnum);
    if (z == zone_table.end()) {
        ch->sendText("No zone was found with that number.\r\n");
        return;
    }


        ch->send_to("Zone %d is linked to the following zones:\r\n", z->second.number);
    for (auto r : filter_raw(z->second.rooms)) {

        for (auto& [d, e] : r->getDirections()) {
            auto z2 = e.getZone();
            if(z2->number == zvnum) continue;

                        ch->send_to("%3d %-30s at %5d (%-5s) ---> %5d\r\n", z2->number, z2->name, nr, dirs[static_cast<int>(d)], e.getVnum());
        }
    }
}

/******************************************************************************/
/** Helper Functions                                                         **/
/******************************************************************************/


/*
 * List all rooms in a zone.                              
 */
void list_rooms(Character *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax) {
    room_vnum i, j, bottom, top, counter = 0;
    /*
     * Expect a minimum / maximum number if the rnum for the zone is NOWHERE.
     */

    if (rnum != NOWHERE) {
        auto& z = zone_table.at(rnum);
        bottom = z.bot;
        top = z.top;
    } else {
        bottom = vmin;
        top = vmax;
    }

        ch->sendText("@nVNum    Room Name                                Exits\r\n"
                 "------- ---------------------------------------- -----@n\r\n");

    for (auto &[vn, r] : world) {

        /** Check to see if this room is one of the ones needed to be listed.    **/
        if ((vn >= bottom) && (vn <= top)) {
            counter++;

            auto sString = !r->proto_script.empty() ? fmt::format(" {}", r->scriptString()) : "";

                        ch->send_to("[@g%-5d@n] @[1]%-*s@n %s", vn, count_color_chars(r->getName()) + 44, r->getName(), sString.c_str());
            for (auto& [d, e] : r->getDirections()) {
                if (e.getZone() != r->zone)
                                        ch->send_to("(@y%d@n)", e.getVnum());
            }

                        ch->sendText("\r\n");
        }
    }

    if (counter == 0) {
                ch->sendText("No rooms found for zone/range specified.\r\n");
    }
}

/*
 * List all mobiles in a zone.                              
 */
void list_mobiles(Character *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax) {
    mob_vnum i, bottom, top, counter = 0, admg;

    if (rnum != NOWHERE) {
        auto& z = zone_table.at(rnum);
        bottom = z.bot;
        top = z.top;
    } else {
        bottom = vmin;
        top = vmax;
    }

        ch->sendText("@nVnum    Cnt    Mobile Name                    Race      Class     Level\r\n"
                   "------- ----- -------------------------      --------- --------- -----\r\n");

    for (auto &[vn, m] : mob_proto) {
        if (vn >= bottom && vn <= top) {
            counter++;
            auto sString = !m.proto_script.empty() ? fmt::format(" {}", m.scriptString()) : "";
                        ch->send_to("@g%4d@n);[@g%-5d@n] @[3]%-*s @C%-9s @c%-9s @y[%4d]@n %s\r\n",
                         vn, characterSubscriptions.count(fmt::format("vnum_{}", vn)), count_color_chars(m.short_description) + 30,
                         m.short_description, TRUE_RACE(&m), sensei::getName(m.sensei).c_str(),
                         m.getBaseStat<int>("level"),
                         sString.c_str());
        }
    }

    if (counter == 0) {
                ch->sendText("None found.\r\n");
    }
}

/*
 * List all objects in a zone.                              
 */
void list_objects(Character *ch, zone_rnum rnum, room_vnum vmin, room_vnum vmax) {
    obj_vnum i, bottom, top, counter = 0;

    if (rnum != NOWHERE) {
        auto& z = zone_table.at(rnum);
        bottom = z.bot;
        top = z.top;
    } else {
        bottom = vmin;
        top = vmax;
    }

        ch->sendText("@VNum   Cnt   Object Name                                  Object Type\r\n"
                 "------- ----- -------------------------------------------- ----------------\r\n");

    for (auto &[vn, o] : obj_proto) {
        if (vn >= bottom && vn <= top) {
            counter++;
            auto sString = !o.proto_script.empty() ? fmt::format(" {}", o.scriptString()) : "";
                        ch->send_to("@g%4d@n);[@g%-5d@n] @[2]%-*s @y[%s]@n%s\r\n",
                         vn, objectSubscriptions.count(fmt::format("vnum_{}", vn)), count_color_chars(o.short_description) + 44,
                         o.short_description, magic_enum::enum_name(o.type_flag).data(),
                         sString.c_str());
        }
    }

    if (counter == 0) {
                ch->sendText("None found.\r\n");
    }
}


/*
 * List all shops in a zone.                              
 */
void list_shops(Character *ch, zone_rnum rnum, shop_vnum vmin, shop_vnum vmax) {
    int i, j, bottom, top, counter = 0;

    if (rnum != NOWHERE) {
        auto& z = zone_table.at(rnum);
        bottom = z.bot;
        top = z.top;
    } else {
        bottom = vmin;
        top = vmax;
    }

    /****************************************************************************/
    /** Store the header for the shop listing.                                 **/
    /****************************************************************************/
        ch->sendText("Index VNum    Shop Room(s)\r\n"
                 "----- ------- ---------------------------------------------\r\n");

    for (auto &[i, sh] : shop_index) {

        if (i >= bottom && i <= top) {
            counter++;

                        ch->send_to("@g%4d@n);[@g%-5d@n]", counter, i);

            /************************************************************************/
            /** Retrieve the list of rooms for this shop.                          **/
            /************************************************************************/
			j = 0;
            for (auto r : sh.in_room)
                                ch->send_to("%s@c[@y%d@c]@n", (j++ % 8 == 0) ? "\r\n              " : " ", r);

            if (j == 0)
                                ch->sendText("@cNone.@n");

                        ch->sendText("\r\n");
        }
    }

    if (counter == 0)
                ch->sendText("None found.\r\n");
}

/*
 * List all zones in the world (sort of like 'show zones').                              
 */
void list_zones(Character *ch) {

        ch->sendText("VNum  Zone Name                      Builder(s)\r\n"
                 "----- ------------------------------ --------------------------------------\r\n");

    for (auto &z : zone_table)
                ch->send_to("[@g%3d@n] @c%-*s @y%-1s@n\r\n", z.first, count_color_chars(z.second.name.c_str()) + 30, z.second.name.c_str(), !z.second.builders.empty() ? z.second.builders.c_str() : "None.");
}


/*
 * Prints all of the zone information for the selected zone.
 */
void print_zone(Character *ch, zone_vnum vnum) {
    zone_rnum rnum;
    std::size_t size_rooms, size_objects, size_mobiles, i;
    std::size_t size_guilds, size_triggers, size_shops;
    char bits[MAX_STRING_LENGTH];

    if (!zone_table.count(vnum)) {
                ch->send_to("Zone #%d does not exist in the database.\r\n", vnum);
        return;
    }
    auto& z = zone_table.at(vnum);
    sprintf(bits, "%s", z.zone_flags.getFlagNames().c_str());
    size_rooms = z.rooms.size();
    size_objects = z.objects.size();
    size_mobiles = z.mobiles.size();
    size_shops = z.shops.size();
    size_triggers = z.triggers.size();
    size_guilds = z.guilds.size();;


    /****************************************************************************/
    /** Display all of the zone information at once.                           **/
    /****************************************************************************/
        ch->send_to("@gVirtual Number = @c%ld\r\n"
                 "@gName of zone   = @c%s\r\n"
                 "@gBuilders       = @c%s\r\n"
                 "@gLifespan       = @c%ld\r\n"
                 "@gAge            = @c%f\r\n"
                 "@gBottom of Zone = @c%ld\r\n"
                 "@gTop of Zone    = @c%ld\r\n"
                 "@gReset Mode     = @c%s\r\n"
                 "@gMin Level      = @c%ld\r\n"
                 "@gMax Level      = @c%ld\r\n"
                 "@gZone Flags     = @c%s\r\n"
                 "@gSize\r\n"
                 "@g   Rooms       = @c%ld\r\n"
                 "@g   Objects     = @c%ld\r\n"
                 "@g   Mobiles     = @c%ld\r\n"
                 "@g   Shops       = @c%ld\r\n"
                 "@g   Triggers    = @c%ld\r\n"
                 "@g   Guilds      = @c%ld@n\r\n", z.number, z.name, z.builders, z.lifespan, z.age, z.bot, z.top, z.reset_mode ? ((z.reset_mode == 1) ?
                                                "Reset when no players are in zone." : "Normal reset.") : "Never reset", z.min_level, z.max_level, bits, size_rooms, size_objects, size_mobiles, size_shops, size_triggers, size_guilds);
}

/* List code by Ronald Evers - dlanor@xs4all.nl */
void list_triggers(Character *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax) {
    int i, bottom, top, counter = 0;
    char trgtypes[256];

    /** Expect a minimum / maximum number if the rnum for the zone is NOWHERE. **/
    if (rnum != NOWHERE) {
        auto& z = zone_table.at(rnum);
        bottom = z.bot;
        top = z.top;
    } else {
        bottom = vmin;
        top = vmax;
    }


    /** Store the header for the room listing. **/
        ch->sendText("Index VNum    Trigger Name                        Type\r\n"
                 "----- ------- -------------------------------------------------------\r\n");


    /** Loop through the world and find each room. **/
    for (auto &t : trig_index) {
        /** Check to see if this room is one of the ones needed to be listed.    **/
        if ((t.first >= bottom) && (t.first <= top)) {
            counter++;

                        ch->send_to("%4d);[@g%5d@n] @[1]%-45.45s ",
                         counter, t.first, t.second.name);

            if (t.second.attach_type == OBJ_TRIGGER) {
                sprintbit(t.second.trigger_type, otrig_types, trgtypes, sizeof(trgtypes));
                                ch->send_to("obj @y%s@n\r\n", trgtypes);
            } else if (t.second.attach_type == WLD_TRIGGER) {
                sprintbit(t.second.trigger_type, wtrig_types, trgtypes, sizeof(trgtypes));
                                ch->send_to("wld @y%s@n\r\n", trgtypes);
            } else {
                sprintbit(t.second.trigger_type, trig_types, trgtypes, sizeof(trgtypes));
                                ch->send_to("mob @y%s@n\r\n", trgtypes);
            }

        }
    }

    if (counter == 0) {
                ch->send_to("No triggers found from %d to %d.\r\n", vmin, vmax);
    }
}
