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

struct PathNode {
    PathNode(room_data *src, int dir) {
        this->dir = dir;
        this->exit = src->dir_option[dir];
        if(exit) {
            this->room = exit->getDestination();
        }
    }
    int dir;
    room_direction_data* exit{nullptr};
    room_data* room{nullptr};
    operator bool() const {
        if(!exit || !room) return false;
        if(!CONFIG_TRACK_T_DOORS && IS_SET(exit->exit_info, EX_CLOSED)) return {};
        if(room->room_flags.get(ROOM_NOTRACK)) return {};
        return true;
    }
};

// The searcher may or may not have a TraverseFunc. it's assumed to be true if not.
// This is useful for seeing if a specific character can make the traversal.
using TraverseFunc = std::function<bool(PathNode&)>;


// Now, find_first_step can be implemented by calling find_bfs_path and taking the first step.
std::optional<std::vector<PathNode>> find_bfs_path(room_data* src, room_data* target, TraverseFunc is_valid) {
    // If already at target, return an empty path.
    if (src == target)
        return {};

    // Each queue element holds a current room and the path taken to reach it.
    std::queue<std::pair<room_data*, std::vector<PathNode>>> frontier;
    std::unordered_set<room_data*> visited;

    visited.insert(src);

    // Enqueue initial edges from src.
    for (int d = 0; d < NUM_OF_DIRS; ++d) {
        PathNode node(src, d);
        if (node) {
            // If a TraverseFunc is provided, check if this edge is allowed.
            if (!is_valid || is_valid(node)) {
                room_data* neighbor = node.room;
                if (neighbor) {
                    std::vector<PathNode> path;
                    path.push_back(node);
                    frontier.push({neighbor, path});
                    visited.insert(neighbor);
                }
            }
        }
    }

    // Standard BFS loop.
    while (!frontier.empty()) {
        auto [curr, path] = frontier.front();
        frontier.pop();

        // If we've reached the target, return the full path.
        if (curr == target)
            return path;

        // Otherwise, enqueue all valid neighbors.
        for (int d = 0; d < NUM_OF_DIRS; ++d) {
            PathNode node(curr, d);
            if (node) {
                if (!is_valid || is_valid(node)) {
                    room_data* neighbor = node.room;
                    if (neighbor && visited.find(neighbor) == visited.end()) {
                        std::vector<PathNode> new_path = path;  // copy current path
                        new_path.push_back(node);
                        frontier.push({neighbor, new_path});
                        visited.insert(neighbor);
                    }
                }
            }
        }
    }

    // No path found.
    return std::nullopt;
}

int find_first_step(room_data* src, room_data* target) {
    // Assume these constants are defined:
    constexpr int BFS_ALREADY_THERE = -1;
    constexpr int BFS_NO_PATH = -2;

    auto path_opt = find_bfs_path(src, target, {});
    if (!path_opt.has_value()) {
        return BFS_NO_PATH;
    }
    const auto& path = path_opt.value();
    if (path.empty()) {
        return BFS_ALREADY_THERE;
    }
    // The first step's direction is stored in the first pair.
    return path.front().dir;
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
        {"namek", 42880}
};

ACMD(do_sradar) {
    struct obj_data *vehicle = nullptr, *controls = nullptr;
    int dir = 0, noship = false;
    char arg[MAX_INPUT_LENGTH];
    char planet[20];

    one_argument(argument, arg);

    if (!PLR_FLAGGED(ch, PLR_PILOTING) && GET_ADMLEVEL(ch) < 1) {
        send_to_char(ch, "You are not flying a ship, maybe you want detect?\r\n");
        return;
    }

    if (!(controls = find_control(ch)) && GET_ADMLEVEL(ch) < 1) {
        send_to_char(ch, "@wYou have nothing to control here!\r\n");
        return;
    }

    if (!PLR_FLAGGED(ch, PLR_PILOTING) && GET_ADMLEVEL(ch) >= 1) {
        noship = true;
    } else if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, VAL_CONTROL_VEHICLE_VNUM)))) {
        send_to_char(ch, "@wYou can't find anything to pilot.\r\n");
        return;
    }
    
    if (noship == false && vehicle->getLocationTileType() != SECT_SPACE) {
        send_to_char(ch, "@wYour ship is not in space!\r\n");
        return;
    }
    if (noship == true && ch->getLocationTileType() != SECT_SPACE) {
        send_to_char(ch, "@wYou are not even in space!\r\n");
        return;
    }

    if (!*arg) {
        if (GET_ADMLEVEL(ch) >= 1 && noship == true) {
            printmap(ch->getRoomVnum(), ch, 0, -1);
        } else {
            printmap(IN_ROOM(vehicle), ch, 0, GET_OBJ_VNUM(vehicle));
        }
        /*
         send_to_char(ch, "     @D----------------[@C Radar @D]-----------------@n\r\n");
         send_to_char(ch, "    @D|@GEE@D:@w Earth    @gNN@D:@w Namek   @YVV@D:@w Vegeta       @D|@n\r\n");
         send_to_char(ch, "    @D|@CFF@D:@w Frigid   @mKK@D:@w Konack  @BAA@D:@w Aether       @D|@n\r\n");
         send_to_char(ch, "    @D|@MYY@D:@w Yardrat  @CKK@D:@w Kanassa @mAA@D:@w Arlia        @D|@n\r\n");
         send_to_char(ch, "    @D|@WBB@D:@w Buoy     @m&&@D:@w Nebula  @yQQ@D:@w Asteroid     @D|@n\r\n");
         send_to_char(ch, "    @D|@b@1**@n@D:@w Wormhole @DSS@D:@w Station  @r#@D: @wUnknown Ship @D|@n\n");
         send_to_char(ch, "    @D|@6  @n@D:@w Star                                  @D|@n\n"); */
        return;
    }

    if (GET_PING(ch) > 0) {
        send_to_char(ch, "@wYou need to wait a few more seconds before pinging a destination again.\r\n");
        return;
    }
    std::string argstr(arg);
    boost::to_lower(argstr);

    struct room_data *startRoom;
    if (noship == false) {
        startRoom = vehicle->getRoom();
    } else {
        startRoom = ch->getRoom();
    }

    auto find = planetLocations.find(argstr);
    if(find != planetLocations.end()) {
        dir = find_first_step(startRoom, get_room(find->second));
        sprintf(planet, "%s", argstr.c_str());
    } else {
        if(!strcasecmp(arg, "buoy1")) {
            auto room = get_room(GET_RADAR1(ch));
            if(room) {
                dir = find_first_step(startRoom, room);
            } else {
                send_to_char(ch, "@wYou haven't launched that buoy.\r\n");
                return;
            }
        } else if(!strcasecmp(arg, "buoy2")) {
            auto room = get_room(GET_RADAR2(ch));
            if(room) {
                dir = find_first_step(startRoom, room);
            } else {
                send_to_char(ch, "@wYou haven't launched that buoy.\r\n");
                return;
            }
        } else if(!strcasecmp(arg, "buoy3")) {
            auto room = get_room(GET_RADAR3(ch));
            if(room) {
                dir = find_first_step(startRoom, room);
            } else {
                send_to_char(ch, "@wYou haven't launched that buoy.\r\n");
                return;
            }
        } else {
            send_to_char(ch, "@wThat is not a valid planet.\r\n");
            return;
        }
    }

    switch (dir) {
        case BFS_ERROR:
            send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
            break;
        case BFS_ALREADY_THERE:
            send_to_char(ch, "@wThe radar shows that your are already there.@n\r\n");
            break;
        case BFS_NO_PATH:
            send_to_char(ch, "@wYou should be in space to use the radar.@n\r\n");
            break;
        default:
            send_to_char(ch, "@wYour radar detects @C%s@w to the @G%s@n\r\n", planet, dirs[dir]);
            break;
    }
    ch->setBaseStat<int>("ping", 5);

}


ACMD(do_radar) {
    int dir, found = false, fcount = 0;

    auto dradar = ch->findObjectVnum(12);
    if (!dradar) {
        send_to_char(ch, "You do not even have a dragon radar!\r\n");
        return;
    }

    if (IS_NPC(ch)) {
        send_to_char(ch, "You are a freaking mob!\r\n");
        return;
    }

    auto cr = ch->getRoom();

    WAIT_STATE(ch, PULSE_2SEC);
    act("$n holds up a dragon radar and pushes its button.", false, ch, nullptr, nullptr, TO_ROOM);
    for(auto vn : dbVnums) {
        auto o = objectSubscriptions.first(fmt::format("vnum_{}", vn));
        if(!o) continue;
        auto r = o->getAbsoluteRoom();
        if(!r) continue;
        dir = find_first_step(cr, r);
        switch (dir) {
            case BFS_ERROR:
                send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
                break;
            case BFS_ALREADY_THERE:
                send_to_char(ch, "@D<@G%d@D>@w The radar detects a dragonball right here!\r\n", fcount);
                break;
            case BFS_NO_PATH:
                send_to_char(ch,
                             "@D<@G%d@D>@w The radar detects a faint dragonball signal, but can not direct you further.\r\n",
                             fcount);
                break;
            default:
                send_to_char(ch, "@D<@G%d@D>@w The radar detects a dragonball %s of here.\r\n", fcount,
                             dirs[dir]);
                break;
        }
        found = true;
        break;
    }

    if (found == false) {
        send_to_char(ch, "The radar didn't detect any dragonballs on the planet.\r\n");
        return;
    }
}

static std::string sense_align(struct char_data *vict) {
    auto align = GET_ALIGNMENT(vict);
    if (align > 50 && align < 200) {
        return "You sense slightly pure and good ki from them.\r\n";
    } else if (align > 200 && align < 500) {
        return "You sense a pure and good ki from them.\r\n";
    } else if (align >= 500) {
        return "You sense an extremely pure and good ki from them.\r\n";
    } else if (align < -50 && align > -200) {
        return "You sense slightly sour and evil ki from them.\r\n";
    } else if (align < -200 && align > -500) {
        return "You sense a sour and evil ki from them.\r\n";
    } else if (align <= -500) {
        return "You sense an extremely evil ki from them.\r\n";
    } else if (align > -50 && align < 50) {
        return "You sense slightly mild indefinable ki from them.\r\n";
    } else
        return "You sense an uncertain quality of ki from them.\r\n";
}

static std::string sense_compare(struct char_data *ch, struct char_data *vict) {
    auto hitv = GET_HIT(vict);
    auto hitc = GET_HIT(ch);
    if (hitv > hitc * 50) {
        return "Their power is so huge it boggles your mind and crushes your spirit to fight!\n";
    } else if (hitv > hitc * 25) {
        return "Their power is so much larger than you that you would die like an insect.\n";
    } else if (hitv > hitc * 10) {
        return "Their power is many times larger than your own.\n";
    } else if (hitv > hitc * 5) {
        return "Their power is a great deal larger than your own.\n";
    } else if (hitv > hitc * 2) {
        return "Their power is more than twice as large as your own.\n";
    } else if (hitv > hitc) {
        return "Their power is about twice as large as your own.\n";
    } else if (hitv == hitc) {
        return "Their power is exactly as strong as you.\n";
    } else if (hitv >= hitc * 0.75) {
        return "Their power is about a quarter of your own or larger.\n";
    } else if (hitv >= hitc * 0.5) {
        return "Their power is about half of your own or larger.\n";
    } else if (hitv >= hitc * 0.25) {
        return "Their power is about a quarter of your own or larger.\n";
    } else if (hitv >= hitc * 0.1) {
        return "Their power is about a tenth of your own or larger.\n";
    } else if (hitv >= hitc * 0.01) {
        return "Their power is less than a tenth of your own.\n";
    } else if (hitv < hitc * 0.01) {
        return "Their power is less than 1 percent of your own. What a weakling...\n";
    } else
        return "You sense an uncertain quality of ki from them.\n";
}

ACMD(do_track) {
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;
    struct descriptor_data *i;
    int count = 0, dir;

    /* The character must have the track skill. */
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SENSE)) {
        send_to_char(ch, "You have no idea how.\r\n");
        return;
    }

    auto sup = GET_SUPPRESS(ch);

    if (sup <= 20 && sup > 0) {
        send_to_char(ch,
                     "You are concentrating too hard on suppressing your powerlevel at this level of suppression.\r\n");
        return;
    }

    one_argument(argument, arg);
    if (!*arg && !FIGHTING(ch)) {
        send_to_char(ch, "Whom are you trying to sense?\r\n");
        return;
    } else if (!*arg && FIGHTING(ch)) {
        vict = FIGHTING(ch);
        send_to_char(ch, "You focus on the one your are fighting.\r\n");
        if (AFF_FLAGGED(vict, AFF_NOTRACK) || IS_ANDROID(vict)) {
            send_to_char(ch, "You can't sense them.\r\n");
            return;
        }
        if (!read_sense_memory(ch, vict)) {
            send_to_char(ch, "You will remember their ki signal from now on.\r\n");
            sense_memory_write(ch, vict);
        }
        act("You look at $N@n intently for a moment.", true, ch, nullptr, vict, TO_CHAR);
        act("$n looks at you intently for a moment.", true, ch, nullptr, vict, TO_VICT);
        act("$n looks at $N@n intently for a moment.", true, ch, nullptr, vict, TO_NOTVICT);
        if (!IS_ANDROID(vict)) {
            send_to_char(ch, sense_align(vict).c_str());
            send_to_char(ch, sense_compare(ch, vict).c_str());
        } else {
            send_to_char(ch, "You can't sense their powerlevel as they are a machine.\r\n");
        }
        return;
    }

    /* Scanning the entire planet. */
    if (!strcasecmp(arg, "scan")) {
        for (i = descriptor_list; i; i = i->next) {
            if (STATE(i) != CON_PLAYING) {
                continue;
            } else if (IS_ANDROID(i->character)) {
                continue;
            } else if (i->character == ch) {
                continue;
            } else if (GET_HIT(i->character) < (GET_HIT(ch) * 0.001) + 1) {
                continue;
            } else if (planet_check(ch, i->character)) {
                if (readIntro(ch, i->character) == 1) {
                    send_to_char(ch, "@D[@Y%d@D] @CYou sense @c%s@C.\r\n", (count + 1), get_i_name(ch, i->character));
                } else {
                    send_to_char(ch, "@D[@Y%d@D] @CYou sense an unknown individual.\r\n", (count + 1));
                }
                /* How strong is the one we sense? */
                send_to_char(ch, sense_compare(ch, i->character).c_str());

                /* What's their alignment? */
				send_to_char(ch, sense_align(i->character).c_str());

                const char *blah = sense_location(i->character);
                send_to_char(ch, "@wLastly you sense that they are at... @C%s@n\n", blah);
                ++count;
                //free(blah);
            }
        }
        if (count == 0) {
            send_to_char(ch, "You sense that there is no one important around.@n\n");
        }
        return;
    }

    /* The person can't see the victim. */

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD))) {
        send_to_char(ch, "No one is around by that name.\r\n");
        return;
    }

    /* We can't track the victim. */
    if (AFF_FLAGGED(vict, AFF_NOTRACK) || IS_ANDROID(vict)) {
        send_to_char(ch, "You can't sense them.\r\n");
        return;
    }

    if (GET_HIT(vict) < (GET_HIT(ch) * 0.001) + 1) {
        if (IN_ROOM(ch) == IN_ROOM(vict)) {
            if (!read_sense_memory(ch, vict)) {
                send_to_char(ch,
                             "Their powerlevel is too weak for you to sense properly, but you will recognise their ki signal from now on.\r\n");
                sense_memory_write(ch, vict);
            } else {
                send_to_char(ch, "Their powerlevel is too weak for you to sense properly.\r\n");
            }
        } else {
            send_to_char(ch, "Their powerlevel is too weak for you to sense properly.\r\n");
        }
        return;
    }

    if(GET_SKILL_BASE(ch, SKILL_SENSE) == 100) {
        if(planet_check(ch, vict)) {
            send_to_char(ch, "@WSense@D: %s@n\r\n", sense_location(vict));
        }
    } else {

        if (GET_SKILL(ch, SKILL_SENSE) < rand_number(1, 101)) {
            int tries = 10;
            /* Find a random direction. :) */
            do {
                dir = rand_number(0, NUM_OF_DIRS - 1);
            } while (!CAN_GO(ch, dir) && --tries);
            send_to_char(ch, "You sense them %s faintly from here, but are unsure....\r\n", dirs[dir]);
            improve_skill(ch, SKILL_SENSE, 1);
            improve_skill(ch, SKILL_SENSE, 1);
            improve_skill(ch, SKILL_SENSE, 1);
            return;
        }

        /* They passed the skill check. */
        dir = find_first_step(ch->getRoom(), vict->getRoom());

        switch (dir) {
            case BFS_ERROR:
                send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
                break;
            case BFS_ALREADY_THERE:
                act("You look at $N@n intently for a moment.", true, ch, nullptr, vict, TO_CHAR);
                act("$n looks at you intently for a moment.", true, ch, nullptr, vict, TO_VICT);
                act("$n looks at $N intently for a moment.", true, ch, nullptr, vict, TO_NOTVICT);
                if (!IS_ANDROID(vict)) {
                    send_to_char(ch, sense_align(vict).c_str());
                    send_to_char(ch, sense_compare(ch, vict).c_str());
                } else {
                    send_to_char(ch, "You can't sense their powerlevel as they are a machine.\r\n");
                }
                break;
            case BFS_TO_FAR:
                send_to_char(ch, "You are too far to sense %s accurately from here.\r\n", HMHR(vict));
                break;
            case BFS_NO_PATH:
                send_to_char(ch, "You can't sense %s from here.\r\n", HMHR(vict));
                break;
            default:    /* Success! */
                send_to_char(ch, "You sense them %s from here!\r\n", dirs[dir]);
                if (GET_SKILL_BASE(ch, SKILL_SENSE) >= 75) {
                    send_to_char(ch, "@WSense@D: @Y%s@n\r\n", sense_location(vict));
                }
                WAIT_STATE(ch, PULSE_2SEC);
                improve_skill(ch, SKILL_SENSE, 1);
                improve_skill(ch, SKILL_SENSE, 1);
                improve_skill(ch, SKILL_SENSE, 1);
        }
    }
}

