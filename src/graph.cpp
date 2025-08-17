/* ************************************************************************
 *   File: graph.c                                       Part of CircleMUD *
 *  Usage: various graph algorithms                                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include <boost/algorithm/string.hpp>
#include <queue>

#include "dbat/graph.h"
#include "dbat/send.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/spells.h"
#include "dbat/constants.h"
#include "dbat/maputils.h"
#include "dbat/vehicles.h"
#include "dbat/act.informative.h"

/* local functions */

struct PathNode
{
    PathNode(Destination &src, Direction dir)
    {
        this->dir = dir;
        this->exit = src;
    }
    Direction dir;
    Destination exit;
    operator bool() const
    {
        if (!exit)
            return false;
    if (!CONFIG_TRACK_T_DOORS && exit.exit_flags[EX_CLOSED])
            return {};
        if (exit.getRoomFlag(ROOM_NOTRACK))
            return {};
        return true;
    }
};

// The searcher may or may not have a TraverseFunc. it's assumed to be true if not.
// This is useful for seeing if a specific character can make the traversal.
using TraverseFunc = std::function<bool(PathNode &)>;

// Now, find_first_step can be implemented by calling find_bfs_path and taking the first step.
std::optional<std::vector<PathNode>> find_bfs_path(Location &src, Location &target, TraverseFunc is_valid)
{
    // If already at target, return an empty path.
    if (src == target)
        return {};

    // Each queue element holds a current room and the path taken to reach it.
    std::queue<std::pair<Destination, std::vector<PathNode>>> frontier;
    std::unordered_set<Location> visited;

    visited.insert(src);

    // Enqueue initial edges from src.
    for (auto &[door, e] : src.getExits())
    {
        PathNode node(e, door);
        if (node)
        {
            // If a TraverseFunc is provided, check if this edge is allowed.
            if (!is_valid || is_valid(node))
            {
                std::vector<PathNode> path;
                path.push_back(node);
                frontier.push({e, path});
                visited.insert(e);
            }
        }
    }

    // Standard BFS loop.
    while (!frontier.empty())
    {
        auto [curr, path] = frontier.front();
        frontier.pop();

        // If we've reached the target, return the full path.
        if (curr == target)
            return path;

        // Otherwise, enqueue all valid neighbors.
        for (auto &[door, e] : curr.getExits())
        {
            PathNode node(e, door);
            if (node)
            {
                if (!is_valid || is_valid(node))
                {
                    if (visited.find(e) == visited.end())
                    {
                        std::vector<PathNode> new_path = path; // copy current path
                        new_path.push_back(node);
                        frontier.push({e, new_path});
                        visited.insert(e);
                    }
                }
            }
        }
    }

    // No path found.
    return std::nullopt;
}

int find_first_step(Location &src, Location &target)
{
    // Assume these constants are defined:
    constexpr int BFS_ALREADY_THERE = -1;
    constexpr int BFS_NO_PATH = -2;

    auto path_opt = find_bfs_path(src, target, {});
    if (!path_opt.has_value())
    {
        return BFS_NO_PATH;
    }
    const auto &path = path_opt.value();
    if (path.empty())
    {
        return BFS_ALREADY_THERE;
    }
    // The first step's direction is stored in the first pair.
    return static_cast<int>(path.front().dir);
}

/********************************************************
 * Functions and Commands which use the above functions. *
 ********************************************************/
static std::map<std::string, room_vnum> planetLocations = {
    {"earth", 40979},
    {"frigid", 30889},
    {"konack", 27065},
    {"vegeta", 32365},
    {"aether", 41959},
    {"namek", 42880}};

ACMD(do_sradar)
{
    Object *vehicle = nullptr, *controls = nullptr;
    int dir = 0, noship = false;
    char arg[MAX_INPUT_LENGTH];
    char planet[20];

    one_argument(argument, arg);

    if (!PLR_FLAGGED(ch, PLR_PILOTING) && GET_ADMLEVEL(ch) < 1)
    {
        ch->sendText("You are not flying a ship, maybe you want detect?\r\n");
        return;
    }

    if (!(controls = find_control(ch)) && GET_ADMLEVEL(ch) < 1)
    {
        ch->sendText("@wYou have nothing to control here!\r\n");
        return;
    }

    if (!PLR_FLAGGED(ch, PLR_PILOTING) && GET_ADMLEVEL(ch) >= 1)
    {
        noship = true;
    }
    else if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, VAL_CONTROL_VEHICLE_VNUM))))
    {
        ch->sendText("@wYou can't find anything to pilot.\r\n");
        return;
    }

    if (noship == false && vehicle->location.getTileType() != SECT_SPACE)
    {
        ch->sendText("@wYour ship is not in space!\r\n");
        return;
    }
    if (noship == true && ch->location.getTileType() != SECT_SPACE)
    {
        ch->sendText("@wYou are not even in space!\r\n");
        return;
    }

    if (!*arg)
    {
        if (GET_ADMLEVEL(ch) >= 1 && noship == true)
        {
            printmap(ch->location.getVnum(), ch, 0, -1);
        }
        else
        {
            printmap(IN_ROOM(vehicle), ch, 0, GET_OBJ_VNUM(vehicle));
        }
        /*
                  ch->sendText("     @D----------------[@C Radar @D]-----------------@n\r\n");
                  ch->sendText("    @D|@GEE@D:@w Earth    @gNN@D:@w Namek   @YVV@D:@w Vegeta       @D|@n\r\n");
                  ch->sendText("    @D|@CFF@D:@w Frigid   @mKK@D:@w Konack  @BAA@D:@w Aether       @D|@n\r\n");
                  ch->sendText("    @D|@MYY@D:@w Yardrat  @CKK@D:@w Kanassa @mAA@D:@w Arlia        @D|@n\r\n");
                  ch->sendText("    @D|@WBB@D:@w Buoy     @m&&@D:@w Nebula  @yQQ@D:@w Asteroid     @D|@n\r\n");
                  ch->sendText("    @D|@b@1**@n@D:@w Wormhole @DSS@D:@w Station  @r#@D: @wUnknown Ship @D|@n\n");
                  ch->sendText("    @D|@6  @n@D:@w Star                                  @D|@n\n"); */
        return;
    }

    if (GET_PING(ch) > 0)
    {
        ch->sendText("@wYou need to wait a few more seconds before pinging a destination again.\r\n");
        return;
    }
    std::string argstr(arg);
    boost::to_lower(argstr);

    auto startLoc = noship ? ch->location : vehicle->location;
    Location endLoc;

    auto find = planetLocations.find(argstr);
    if (find != planetLocations.end())
    {
        endLoc = find->second;
        dir = find_first_step(startLoc, endLoc);
        sprintf(planet, "%s", argstr.c_str());
    }
    else
    {
        if (!strcasecmp(arg, "buoy1"))
        {
            auto room = get_room(GET_RADAR1(ch));
            if (room)
            {
                endLoc = room;
                dir = find_first_step(startLoc, endLoc);
            }
            else
            {
                ch->sendText("@wYou haven't launched that buoy.\r\n");
                return;
            }
        }
        else if (!strcasecmp(arg, "buoy2"))
        {
            auto room = get_room(GET_RADAR2(ch));
            if (room)
            {
                endLoc = room;
                dir = find_first_step(startLoc, endLoc);
            }
            else
            {
                ch->sendText("@wYou haven't launched that buoy.\r\n");
                return;
            }
        }
        else if (!strcasecmp(arg, "buoy3"))
        {
            auto room = get_room(GET_RADAR3(ch));
            if (room)
            {
                endLoc = room;
                dir = find_first_step(startLoc, endLoc);
            }
            else
            {
                ch->sendText("@wYou haven't launched that buoy.\r\n");
                return;
            }
        }
        else
        {
            ch->sendText("@wThat is not a valid planet.\r\n");
            return;
        }
    }

    switch (dir)
    {
    case BFS_ERROR:
        ch->sendText("Hmm.. something seems to be wrong.\r\n");
        break;
    case BFS_ALREADY_THERE:
        ch->sendText("@wThe radar shows that your are already there.@n\r\n");
        break;
    case BFS_NO_PATH:
        ch->sendText("@wYou should be in space to use the radar.@n\r\n");
        break;
    default:
        ch->send_to("@wYour radar detects @C%s@w to the @G%s@n\r\n", planet, dirs[dir]);
        break;
    }
    ch->setBaseStat<int>("ping", 5);
}

ACMD(do_radar)
{
    int dir, found = false, fcount = 0;

    auto dradar = ch->searchInventory(12);
    if (!dradar)
    {
        ch->sendText("You do not even have a dragon radar!\r\n");
        return;
    }

    if (IS_NPC(ch))
    {
        ch->sendText("You are a freaking mob!\r\n");
        return;
    }

    WAIT_STATE(ch, PULSE_2SEC);
    act("$n holds up a dragon radar and pushes its button.", false, ch, nullptr, nullptr, TO_ROOM);
    for (auto vn : dbVnums)
    {
        auto o = objectSubscriptions.first(fmt::format("vnum_{}", vn));
        if (!o)
            continue;
        auto r = o->getAbsoluteLocation();
        if (!r)
            continue;
        dir = find_first_step(ch->location, r);
        switch (dir)
        {
        case BFS_ERROR:
            ch->sendText("Hmm.. something seems to be wrong.\r\n");
            break;
        case BFS_ALREADY_THERE:
            ch->send_to("@D<@G%d@D>@w The radar detects a dragonball right here!\r\n", fcount);
            break;
        case BFS_NO_PATH:
            ch->send_to("@D<@G%d@D>@w The radar detects a faint dragonball signal, but can not direct you further.\r\n", fcount);
            break;
        default:
            ch->send_to("@D<@G%d@D>@w The radar detects a dragonball %s of here.\r\n", fcount, dirs[dir]);
            break;
        }
        found = true;
        break;
    }

    if (found == false)
    {
        ch->sendText("The radar didn't detect any dragonballs on the planet.\r\n");
        return;
    }
}

static std::string sense_align(Character *vict)
{
    auto align = GET_ALIGNMENT(vict);
    if (align > 50 && align < 200)
    {
        return "You sense slightly pure and good ki from them.\r\n";
    }
    else if (align > 200 && align < 500)
    {
        return "You sense a pure and good ki from them.\r\n";
    }
    else if (align >= 500)
    {
        return "You sense an extremely pure and good ki from them.\r\n";
    }
    else if (align < -50 && align > -200)
    {
        return "You sense slightly sour and evil ki from them.\r\n";
    }
    else if (align < -200 && align > -500)
    {
        return "You sense a sour and evil ki from them.\r\n";
    }
    else if (align <= -500)
    {
        return "You sense an extremely evil ki from them.\r\n";
    }
    else if (align > -50 && align < 50)
    {
        return "You sense slightly mild indefinable ki from them.\r\n";
    }
    else
        return "You sense an uncertain quality of ki from them.\r\n";
}

static std::string sense_compare(Character *ch, Character *vict)
{
    auto hitv = GET_HIT(vict);
    auto hitc = GET_HIT(ch);
    if (hitv > hitc * 50)
    {
        return "Their power is so huge it boggles your mind and crushes your spirit to fight!\n";
    }
    else if (hitv > hitc * 25)
    {
        return "Their power is so much larger than you that you would die like an insect.\n";
    }
    else if (hitv > hitc * 10)
    {
        return "Their power is many times larger than your own.\n";
    }
    else if (hitv > hitc * 5)
    {
        return "Their power is a great deal larger than your own.\n";
    }
    else if (hitv > hitc * 2)
    {
        return "Their power is more than twice as large as your own.\n";
    }
    else if (hitv > hitc)
    {
        return "Their power is about twice as large as your own.\n";
    }
    else if (hitv == hitc)
    {
        return "Their power is exactly as strong as you.\n";
    }
    else if (hitv >= hitc * 0.75)
    {
        return "Their power is about a quarter of your own or larger.\n";
    }
    else if (hitv >= hitc * 0.5)
    {
        return "Their power is about half of your own or larger.\n";
    }
    else if (hitv >= hitc * 0.25)
    {
        return "Their power is about a quarter of your own or larger.\n";
    }
    else if (hitv >= hitc * 0.1)
    {
        return "Their power is about a tenth of your own or larger.\n";
    }
    else if (hitv >= hitc * 0.01)
    {
        return "Their power is less than a tenth of your own.\n";
    }
    else if (hitv < hitc * 0.01)
    {
        return "Their power is less than 1 percent of your own. What a weakling...\n";
    }
    else
        return "You sense an uncertain quality of ki from them.\n";
}

ACMD(do_track)
{
    char arg[MAX_INPUT_LENGTH];
    Character *vict;
    struct descriptor_data *i;
    int count = 0, dir;

    /* The character must have the track skill. */
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SENSE))
    {
        ch->sendText("You have no idea how.\r\n");
        return;
    }

    auto sup = GET_SUPPRESS(ch);

    if (sup <= 20 && sup > 0)
    {
        ch->sendText("You are concentrating too hard on suppressing your powerlevel at this level of suppression.\r\n");
        return;
    }

    one_argument(argument, arg);
    if (!*arg && !FIGHTING(ch))
    {
        ch->sendText("Whom are you trying to sense?\r\n");
        return;
    }
    else if (!*arg && FIGHTING(ch))
    {
        vict = FIGHTING(ch);
        ch->sendText("You focus on the one your are fighting.\r\n");
        if (AFF_FLAGGED(vict, AFF_NOTRACK) || IS_ANDROID(vict))
        {
            ch->sendText("You can't sense them.\r\n");
            return;
        }
        if (!read_sense_memory(ch, vict))
        {
            ch->sendText("You will remember their ki signal from now on.\r\n");
            sense_memory_write(ch, vict);
        }
        act("You look at $N@n intently for a moment.", true, ch, nullptr, vict, TO_CHAR);
        act("$n looks at you intently for a moment.", true, ch, nullptr, vict, TO_VICT);
        act("$n looks at $N@n intently for a moment.", true, ch, nullptr, vict, TO_NOTVICT);
        if (!IS_ANDROID(vict))
        {
            ch->sendText(sense_align(vict).c_str());
            ch->sendText(sense_compare(ch, vict).c_str());
        }
        else
        {
            ch->sendText("You can't sense their powerlevel as they are a machine.\r\n");
        }
        return;
    }

    /* Scanning the entire planet. */
    if (!strcasecmp(arg, "scan"))
    {
        for (i = descriptor_list; i; i = i->next)
        {
            if (STATE(i) != CON_PLAYING)
            {
                continue;
            }
            else if (IS_ANDROID(i->character))
            {
                continue;
            }
            else if (i->character == ch)
            {
                continue;
            }
            else if (GET_HIT(i->character) < (GET_HIT(ch) * 0.001) + 1)
            {
                continue;
            }
            else if (planet_check(ch, i->character))
            {
                if (readIntro(ch, i->character) == 1)
                {
                    ch->send_to("@D[@Y%d@D] @CYou sense @c%s@C.\r\n", (count + 1), get_i_name(ch, i->character));
                }
                else
                {
                    ch->send_to("@D[@Y%d@D] @CYou sense an unknown individual.\r\n", (count + 1));
                }
                /* How strong is the one we sense? */
                ch->sendText(sense_compare(ch, i->character).c_str());

                /* What's their alignment? */
                ch->sendText(sense_align(i->character).c_str());

                const char *blah = sense_location(i->character);
                ch->send_to("@wLastly you sense that they are at... @C%s@n\n", blah);
                ++count;
                // free(blah);
            }
        }
        if (count == 0)
        {
            ch->sendText("You sense that there is no one important around.@n\n");
        }
        return;
    }

    /* The person can't see the victim. */

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD)))
    {
        ch->sendText("No one is around by that name.\r\n");
        return;
    }

    /* We can't track the victim. */
    if (AFF_FLAGGED(vict, AFF_NOTRACK) || IS_ANDROID(vict))
    {
        ch->sendText("You can't sense them.\r\n");
        return;
    }

    if (GET_HIT(vict) < (GET_HIT(ch) * 0.001) + 1)
    {
        if (ch->location == vict->location)
        {
            if (!read_sense_memory(ch, vict))
            {
                ch->sendText("Their powerlevel is too weak for you to sense properly, but you will recognise their ki signal from now on.\r\n");
                sense_memory_write(ch, vict);
            }
            else
            {
                ch->sendText("Their powerlevel is too weak for you to sense properly.\r\n");
            }
        }
        else
        {
            ch->sendText("Their powerlevel is too weak for you to sense properly.\r\n");
        }
        return;
    }

    if (GET_SKILL_BASE(ch, SKILL_SENSE) == 100)
    {
        if (planet_check(ch, vict))
        {
            ch->send_to("@WSense@D: %s@n\r\n", sense_location(vict));
        }
    }
    else
    {

        if (GET_SKILL(ch, SKILL_SENSE) < rand_number(1, 101))
        {
            int tries = 10;
            /* Find a random direction. :) */
            do
            {
                dir = rand_number(0, NUM_OF_DIRS - 1);
            } while (!ch->location.canGo(dir) && --tries);
            ch->send_to("You sense them %s faintly from here, but are unsure....\r\n", dirs[dir]);
            improve_skill(ch, SKILL_SENSE, 1);
            improve_skill(ch, SKILL_SENSE, 1);
            improve_skill(ch, SKILL_SENSE, 1);
            return;
        }

        /* They passed the skill check. */
        dir = find_first_step(ch->location, vict->location);

        switch (dir)
        {
        case BFS_ERROR:
            ch->sendText("Hmm.. something seems to be wrong.\r\n");
            break;
        case BFS_ALREADY_THERE:
            act("You look at $N@n intently for a moment.", true, ch, nullptr, vict, TO_CHAR);
            act("$n looks at you intently for a moment.", true, ch, nullptr, vict, TO_VICT);
            act("$n looks at $N intently for a moment.", true, ch, nullptr, vict, TO_NOTVICT);
            if (!IS_ANDROID(vict))
            {
                ch->sendText(sense_align(vict).c_str());
                ch->sendText(sense_compare(ch, vict).c_str());
            }
            else
            {
                ch->sendText("You can't sense their powerlevel as they are a machine.\r\n");
            }
            break;
        case BFS_TO_FAR:
            ch->send_to("You are too far to sense %s accurately from here.\r\n", HMHR(vict));
            break;
        case BFS_NO_PATH:
            ch->send_to("You can't sense %s from here.\r\n", HMHR(vict));
            break;
        default: /* Success! */
            ch->send_to("You sense them %s from here!\r\n", dirs[dir]);
            if (GET_SKILL_BASE(ch, SKILL_SENSE) >= 75)
            {
                ch->send_to("@WSense@D: @Y%s@n\r\n", sense_location(vict));
            }
            WAIT_STATE(ch, PULSE_2SEC);
            improve_skill(ch, SKILL_SENSE, 1);
            improve_skill(ch, SKILL_SENSE, 1);
            improve_skill(ch, SKILL_SENSE, 1);
        }
    }
}
