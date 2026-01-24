/******************************************************************************/
/** OasisOLC - InGame OLC Listings                                     v2.0  **/
/** Original author: Levork                                                  **/
/** Copyright 1996 Harvey Gilpin                                             **/
/** Copyright 1997-2001 George Greer (greerga@circlemud.org)                 **/
/** Copyright 2002 Kip Potter [Mythran] (kip_potter@hotmail.com)             **/
/******************************************************************************/
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/CharacterPrototype.hpp"
#include "dbat/game/ObjectPrototype.hpp"
#include "dbat/game/Destination.hpp"
#include "dbat/game/Area.hpp"
#include "dbat/game/Structure.hpp"
//#include "dbat/game/send.hpp"
#include "dbat/game/comm.hpp"
#include "dbat/game/interpreter.hpp"
#include "dbat/game/db.hpp"
#include "dbat/game/Shop.hpp"
#include "dbat/game/constants.hpp"
#include "dbat/game/dg_scripts.hpp"
//#include "dbat/game/Guild.hpp"
#include "dbat/game/races.hpp"
#include "dbat/game/class.hpp"
#include "volcano/circle/CircleAnsi.hpp"
#include "dbat/game/utils.hpp"

#include "dbat/game/const/AdminLevel.hpp"

static void list_triggers(Character *ch, trig_vnum vmin, trig_vnum vmax);
void list_guilds(Character *ch, guild_vnum vmin, guild_vnum vmax);
void list_zones(Character *ch);
void list_rooms(Character *ch, room_vnum vmin, room_vnum vmax);
void list_objects(Character *ch, obj_vnum vmin, obj_vnum vmax);
void list_mobiles(Character *ch, mob_vnum vmin, mob_vnum vmax);
void list_shops(Character *ch, shop_vnum vmin, shop_vnum vmax);

/******************************************************************************/
/** Ingame Commands                                                          **/
/******************************************************************************/
ACMD(do_oasis_list)
{
    zone_rnum rzone = NOWHERE;
    room_rnum vmin = NOWHERE;
    room_rnum vmax = NOWHERE;
    char smin[MAX_INPUT_LENGTH];
    char smax[MAX_INPUT_LENGTH];

    two_arguments(argument, smin, smax);

    if (subcmd == SCMD_OASIS_ZLIST)
    { /* special case */
        if (!boost::iequals(smin, "") && is_number(smin))
            print_zone(ch, atoi(smin));
        else
            list_zones(ch);
        return;
    }

    if (boost::iequals(smin, "") || boost::iequals(smin, "."))
    {
        rzone = ch->location.getZone()->number;
    }
    else if (!*smax)
    {
        rzone = real_zone(atoi(smin));

        if (rzone == NOWHERE)
        {
            ch->sendText("Sorry, there's no zone with that number\r\n");
            return;
        }
    }
    else
    {
        /** Listing by min vnum / max vnum.  Retrieve the numeric values.          **/
        vmin = atoi(smin);
        vmax = atoi(smax);

        if (vmin + 9000 < vmax)
        {
            ch->sendText("Really? Over 9000?! You need to view that many at once? Come on...\r\n");
            return;
        }
        if (vmin > vmax)
        {
            ch->send_to("List from %d to %d - Aren't we funny today!\r\n", vmin, vmax);
            return;
        }
    }

    switch (subcmd)
    {
    case SCMD_OASIS_RLIST:
        list_rooms(ch, vmin, vmax);
        break;
    case SCMD_OASIS_OLIST:
        list_objects(ch, vmin, vmax);
        break;
    case SCMD_OASIS_MLIST:
        list_mobiles(ch, vmin, vmax);
        break;
    case SCMD_OASIS_TLIST:
        list_triggers(ch, vmin, vmax);
        break;
    case SCMD_OASIS_SLIST:
        list_shops(ch, vmin, vmax);
        break;
    case SCMD_OASIS_GLIST:
        list_guilds(ch, vmin, vmax);
        break;
    default:
        ch->sendText("You can't list that!\r\n");
        mudlog(BRF, ADMLVL_IMMORT, true,
               "SYSERR: do_oasis_list: Unknown list option: %d", subcmd);
    }
}

ACMD(do_oasis_links)
{
    zone_rnum zrnum;
    zone_vnum zvnum;
    room_rnum nr;
    int first, last, j;
    char arg[MAX_INPUT_LENGTH];

    skip_spaces(&argument);
    one_argument(argument, arg);

    if (boost::iequals(arg, "") || boost::iequals(arg, "."))
    {
        zvnum = ch->location.getZone()->number;
    }
    else
    {
        zvnum = atoi(arg);
    }

    auto z = zone_table.find(zvnum);
    if (z == zone_table.end())
    {
        ch->sendText("No zone was found with that number.\r\n");
        return;
    }

    ch->send_to("Zone %d is linked to the following zones:\r\n", z->second->number);
    z->second->rooms.for_each([&](auto r) {
        for (auto &[d, e] : r->getDirections())
        {
            auto z2 = e.getZone();
            if (z2->number == zvnum)
                return;

            ch->send_to("%3d %-30s at %5d (%-5s) ---> %5d\r\n", z2->number, z2->name, nr, dirs[static_cast<int>(d)], e.getVnum());
        }
    });
}

/******************************************************************************/
/** Helper Functions                                                         **/
/******************************************************************************/

/*
 * List all rooms in a zone.
 */
void list_rooms(Character *ch, zone_vnum vmin, zone_vnum vmax)
{
    room_vnum i, j, counter = 0;

    ch->sendText("@nVNum    Room Name                                Exits\r\n"
                 "------- ---------------------------------------- -----@n\r\n");

    for (auto &[vn, r] : Room::registry)
    {
        if (vn < vmin || vn > vmax)
            continue;

        counter++;

        auto sString = !r->proto_script.empty() ? fmt::format(" {}", r->scriptString()) : "";

        ch->send_to("[@g%-5d@n] @[1]%-*s@n %s", vn, volcano::circle::countColors(r->getName()) + 44, r->getName(), sString.c_str());
        for (auto &[d, e] : r->getDirections())
        {
            if (e.getZone() != r->getZone())
                ch->send_to("(@y%d@n)", e.getVnum());
        }

        ch->sendText("\r\n");
    }

    if (counter == 0)
    {
        ch->sendText("No rooms found for zone/range specified.\r\n");
    }
}

/*
 * List all mobiles in a zone.
 */
void list_mobiles(Character *ch, zone_vnum vmin, zone_vnum vmax)
{
    mob_vnum i, counter = 0;

    ch->sendText("@nVnum    Cnt    Mobile Name                    Race      Class     Level\r\n"
                 "------- ----- -------------------------      --------- --------- -----\r\n");

    for (auto &[vn, m] : mob_proto)
    {
        if (vn < vmin || vn > vmax)
            continue;
        counter++;
        auto sString = !m->proto_script.empty() ? fmt::format(" {}", m->scriptString()) : "";
        ch->send_to("@g%4d@n);[@g%-5d@n] @[3]%-*s @C%-9s @c%-9s @y[%4d]@n %s\r\n",
                    vn, characterSubscriptions.count(fmt::format("vnum_{}", vn)), volcano::circle::countColors(m->short_description) + 30,
                    m->short_description, TRUE_RACE(m), sensei::getName(m->sensei).c_str(),
                    m->getBaseStat<int>("level"),
                    sString.c_str());
    }

    if (counter == 0)
    {
        ch->sendText("None found.\r\n");
    }
}

/*
 * List all objects in a zone.
 */
void list_objects(Character *ch, room_vnum vmin, room_vnum vmax)
{
    obj_vnum i, counter = 0;

    ch->sendText("@VNum   Cnt   Object Name                                  Object Type\r\n"
                 "------- ----- -------------------------------------------- ----------------\r\n");

    for (auto &[vn, o] : obj_proto)
    {
        if (vn < vmin || vn > vmax)
            continue;

        counter++;
        auto sString = !o->proto_script.empty() ? fmt::format(" {}", o->scriptString()) : "";
        ch->send_to("@g%4d@n);[@g%-5d@n] @[2]%-*s @y[%s]@n%s\r\n",
                    vn, objectSubscriptions.count(fmt::format("vnum_{}", vn)), volcano::circle::countColors(o->short_description) + 44,
                    o->short_description, enchantum::to_string(o->type_flag).data(),
                    sString.c_str());
    }

    if (counter == 0)
    {
        ch->sendText("None found.\r\n");
    }
}

/*
 * List all shops in a zone.
 */
void list_shops(Character *ch, shop_vnum vmin, shop_vnum vmax)
{
    int i, j, counter = 0;

    ch->sendText("Index VNum    Shop Room(s)\r\n"
                 "----- ------- ---------------------------------------------\r\n");

    for (auto &[i, sh] : shop_index)
    {
        if (i < vmin || i > vmax)
            continue;

        counter++;
        ch->send_to("@g%4d@n);[@g%-5d@n]", counter, i);

        /************************************************************************/
        /** Retrieve the list of rooms for this shop.                          **/
        /************************************************************************/
        j = 0;
        for (auto r : sh->in_room)
            ch->send_to("%s@c[@y%d@c]@n", (j++ % 8 == 0) ? "\r\n              " : " ", r);

        if (j == 0)
            ch->sendText("@cNone.@n");

        ch->sendText("\r\n");
    }

    if (counter == 0)
        ch->sendText("None found.\r\n");
}

/*
 * List all zones in the world (sort of like 'show zones').
 */
void list_zones(Character *ch)
{
    // New hierarchical zone listing: shows tree structure (parent -> children).
    // Only displays VNum and Name; builders column removed per new requirements.
    // Format uses simple ASCII tree characters for broad client compatibility.

    ch->sendText("VNum  Zone Tree\r\n"
                 "----- ------------------------------\r\n");

    // Collect root zones (those without a parent)
    std::vector<Zone *> roots;
    roots.reserve(zone_table.size());
    for (auto &p : zone_table)
    {
        if (p.second->parent == NOTHING)
            roots.push_back(p.second.get());
    }
    std::sort(roots.begin(), roots.end(), [](const Zone *a, const Zone *b)
              { return a->number < b->number; });

    // Recursive lambda to print a zone and its children.
    struct Printer
    {
        Character *ch;
        void operator()(Zone *z, const std::string &prefix, bool isLast, int depth) const
        {
            std::string linePrefix = prefix;
            if (depth > 0)
                linePrefix += (isLast ? "`-" : "|-");

            ch->sendFmt("{}[@g{:3d}@n] @c{} [Rooms: {}] [Areas: {}]@n\r\n", linePrefix, z->number, z->name, z->rooms.live_count(), z->areas.live_count());

            auto children = z->getChildren();
            if (children.empty())
                return;
            std::sort(children.begin(), children.end(), [](const Zone *a, const Zone *b)
                      { return a->number < b->number; });

            // Build next prefix keeping tree lines where needed.
            std::string nextPrefix = prefix;
            if (depth > 0)
                nextPrefix += (isLast ? "  " : "| ");
            else
                nextPrefix = ""; // depth 0 has no connector yet

            for (std::size_t i = 0; i < children.size(); ++i)
            {
                bool lastChild = (i + 1 == children.size());
                (*this)(children[i], nextPrefix, lastChild, depth + 1);
            }
        }
    } printer{ch};

    for (std::size_t i = 0; i < roots.size(); ++i)
    {
        bool lastRoot = (i + 1 == roots.size());
        printer(roots[i], "", lastRoot, 0);
    }
}

/*
 * Prints all of the zone information for the selected zone.
 */
void print_zone(Character *ch, zone_vnum vnum)
{
    zone_rnum rnum;
    std::size_t size_rooms, size_objects, size_mobiles, i;
    std::size_t size_guilds, size_triggers, size_shops;
    char bits[MAX_STRING_LENGTH];

    if (!zone_table.count(vnum))
    {
        ch->send_to("Zone #%d does not exist in the database.\r\n", vnum);
        return;
    }
    auto &z = zone_table.at(vnum);
    sprintf(bits, "%s", z->zone_flags.getFlagNames().c_str());

    /****************************************************************************/
    /** Display all of the zone information at once.                           **/
    /****************************************************************************/
    ch->send_to("@gVirtual Number = @c%ld\r\n"
                "@gName of zone   = @c%s\r\n"
                "@gBuilders       = @c%s\r\n"
                "@gLifespan       = @c%ld\r\n"
                "@gAge            = @c%f\r\n"
                "@gReset Mode     = @c%s\r\n"
                "@gZone Flags     = @c%s\r\n"
                "@gSize\r\n"
                "@g   Rooms       = @c%ld\r\n",
                z->number, z->name, z->builders, z->lifespan, z->age, z->reset_mode ? ((z->reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset", bits, z->rooms.live_count());


    if(z->rooms) {
        ch->send_to("@gRoom List:\r\n");
        z->rooms.for_each([&](auto r) {
            ch->sendFmt("    @g{}@n) @c{}{}{}\r\n", r->getVnum(), r->getName(), r->proto_script.empty() ? "" : fmt::format(" {}", r->scriptString()), r->resetCommands.empty() ? "" : fmt::format(" [ResetCommands: {}]", r->resetCommands.size()));
        });
    }

    if(z->areas) {
        ch->send_to("@gAreas:\r\n");
        z->areas.for_each([&](auto a) {
            ch->sendFmt("    @g{}@n) @c{}@n\r\n", a->getVnum(), a->getName());
        });
    }
}

/* List code by Ronald Evers - dlanor@xs4all.nl */
static void list_triggers(Character *ch, trig_vnum vmin, trig_vnum vmax)
{
    int i, counter = 0;
    char trgtypes[256];

    /** Store the header for the room listing. **/
    ch->sendText("Index VNum    Trigger Name                        Type\r\n"
                 "----- ------- -------------------------------------------------------\r\n");

    /** Loop through the world and find each room. **/
    for (const auto &[vn, t] : trig_index)
    {
        if (vn < vmin || vn > vmax)
            continue;

        counter++;

        ch->send_to("%4d);[@g%5d@n] @[1]%-45.45s ",
                    counter, vn, t->name);

        if (t->attach_type == OBJ_TRIGGER)
        {
            sprintbit(t->trigger_type, otrig_types, trgtypes, sizeof(trgtypes));
            ch->send_to("obj @y%s@n\r\n", trgtypes);
        }
        else if (t->attach_type == WLD_TRIGGER)
        {
            sprintbit(t->trigger_type, wtrig_types, trgtypes, sizeof(trgtypes));
            ch->send_to("wld @y%s@n\r\n", trgtypes);
        }
        else
        {
            sprintbit(t->trigger_type, trig_types, trgtypes, sizeof(trgtypes));
            ch->send_to("mob @y%s@n\r\n", trgtypes);
        }
    }

    if (counter == 0)
    {
        ch->send_to("No triggers found from %d to %d.\r\n", vmin, vmax);
    }
}
