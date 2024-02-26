/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/graph.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/db.h"
#include "dbat/spells.h"
#include "dbat/constants.h"
#include "dbat/maputils.h"
#include "dbat/vehicles.h"
#include "dbat/act.informative.h"
#include "dbat/entity.h"

/* local functions */
static std::list<std::pair<Room*, int>> bfs_queue;

static int VALID_EDGE(Room *x, int y) {
    
    auto d = x->getExits()[y];
    if(!d) return false;
    auto dest = reg.try_get<Destination>(d->ent);
    if(!dest) return false;
    if(CONFIG_TRACK_T_DOORS == false && d->checkFlag(FlagType::Exit, EX_CLOSED)) return false;
    if(flags::check(dest->target, FlagType::Room, ROOM_NOTRACK) || flags::check(dest->target, FlagType::Room, ROOM_BFS_MARK)) return false;
    return true;
}

static void bfs_enqueue(Room *r, int dir) {
    bfs_queue.emplace_back(r, dir);
}


static void bfs_dequeue() {
    if(!bfs_queue.empty()) bfs_queue.pop_front();
}

static void bfs_clear_queue() {
    bfs_queue.clear();
}


/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(Room *src, Room *target) {
    int curr_dir;

    if (src == target)
        return (BFS_ALREADY_THERE);

    /* clear marks first, some OLC systems will save the mark. */
    for (auto &[vn, u] : entities) {
        auto r = reg.try_get<Room>(u);
        if(r) r->clearFlag(FlagType::Room, ROOM_BFS_MARK);
    }
    src->setFlag(FlagType::Room, ROOM_BFS_MARK);

    /* first, enqueue the first steps, saving which direction we're going. */
    for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++) {
        if (VALID_EDGE(src, curr_dir)) {
            auto dest = reg.try_get<Destination>(src->getExits()[curr_dir]->ent);
            if(auto room = reg.try_get<Room>(dest->target); room) {
                room->setFlag(FlagType::Room, ROOM_BFS_MARK);
                bfs_enqueue(room, curr_dir);
            }
        }
    }
    /* now, do the classic BFS. */
    while (!bfs_queue.empty()) {
        auto f = bfs_queue.front();
        if (f.first == target) {
            curr_dir = f.second;
            bfs_clear_queue();
            return (curr_dir);
        } else {
            for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
                if (VALID_EDGE(f.first, curr_dir)) {
                    auto dest = reg.try_get<Destination>(f.first->getExits()[curr_dir]->ent);
                    if(auto room = reg.try_get<Room>(dest->target); room) {
                        room->setFlag(FlagType::Room, ROOM_BFS_MARK);
                        bfs_enqueue(room, f.second);
                    }
                }
            bfs_dequeue();
        }
    }

    return (BFS_NO_PATH);
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
    Object *vehicle = nullptr, *controls = nullptr;
    int dir = 0, noship = false;
    char arg[MAX_INPUT_LENGTH];
    char planet[20];

    one_argument(argument, arg);

    if (!PLR_FLAGGED(ch, PLR_PILOTING) && GET_ADMLEVEL(ch) < 1) {
        ch->sendf("You are not flying a ship, maybe you want detect?\r\n");
        return;
    }

    if (!(controls = find_control(ch)) && GET_ADMLEVEL(ch) < 1) {
        ch->sendf("@wYou have nothing to control here!\r\n");
        return;
    }

    if (!PLR_FLAGGED(ch, PLR_PILOTING) && GET_ADMLEVEL(ch) >= 1) {
        noship = true;
    } else if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0)))) {
        ch->sendf("@wYou can't find anything to pilot.\r\n");
        return;
    }

    if (noship == false && SECT(IN_ROOM(vehicle)) != SECT_SPACE) {
        ch->sendf("@wYour ship is not in space!\r\n");
        return;
    }
    if (noship == true && SECT(IN_ROOM(ch)) != SECT_SPACE) {
        ch->sendf("@wYou are not even in space!\r\n");
        return;
    }

    if (!*arg) {
        if (GET_ADMLEVEL(ch) >= 1 && noship == true) {
            printmap(ch->getRoom()->getUID(), ch, 0, -1);
        } else {
            printmap(IN_ROOM(vehicle), ch, 0, GET_OBJ_VNUM(vehicle));
        }
        /*
         ch->sendf("     @D----------------[@C Radar @D]-----------------@n\r\n");
         ch->sendf("    @D|@GEE@D:@w Earth    @gNN@D:@w Namek   @YVV@D:@w Vegeta       @D|@n\r\n");
         ch->sendf("    @D|@CFF@D:@w Frigid   @mKK@D:@w Konack  @BAA@D:@w Aether       @D|@n\r\n");
         ch->sendf("    @D|@MYY@D:@w Yardrat  @CKK@D:@w Kanassa @mAA@D:@w Arlia        @D|@n\r\n");
         ch->sendf("    @D|@WBB@D:@w Buoy     @m&&@D:@w Nebula  @yQQ@D:@w Asteroid     @D|@n\r\n");
         ch->sendf("    @D|@b@1**@n@D:@w Wormhole @DSS@D:@w Station  @r#@D: @wUnknown Ship @D|@n\n");
         ch->sendf("    @D|@6  @n@D:@w Star                                  @D|@n\n"); */
        return;
    }

    if (GET_PING(ch) > 0) {
        ch->sendf("@wYou need to wait a few more seconds before pinging a destination again.\r\n");
        return;
    }
    std::string argstr(arg);
    to_lower(argstr);

    Room *startRoom;
    if (noship == false) {
        startRoom = vehicle->getRoom();
    } else {
        startRoom = ch->getRoom();
    }

    auto find = planetLocations.find(argstr);
    if(find != planetLocations.end()) {
        dir = find_first_step(startRoom, getEntity<Room>(find->second));
        sprintf(planet, "%s", argstr.c_str());
    } else {
        if(!strcasecmp(arg, "buoy1")) {
            auto room = getEntity<Room>(GET_RADAR1(ch));
            if(room) {
                dir = find_first_step(startRoom, room);
            } else {
                ch->sendf("@wYou haven't launched that buoy.\r\n");
                return;
            }
        } else if(!strcasecmp(arg, "buoy2")) {
            auto room = getEntity<Room>(GET_RADAR2(ch));
            if(room) {
                dir = find_first_step(startRoom, room);
            } else {
                ch->sendf("@wYou haven't launched that buoy.\r\n");
                return;
            }
        } else if(!strcasecmp(arg, "buoy3")) {
            auto room = getEntity<Room>(GET_RADAR3(ch));
            if(room) {
                dir = find_first_step(startRoom, room);
            } else {
                ch->sendf("@wYou haven't launched that buoy.\r\n");
                return;
            }
        } else {
            ch->sendf("@wThat is not a valid planet.\r\n");
            return;
        }
    }

    switch (dir) {
        case BFS_ERROR:
            ch->sendf("Hmm.. something seems to be wrong.\r\n");
            break;
        case BFS_ALREADY_THERE:
            ch->sendf("@wThe radar shows that your are already there.@n\r\n");
            break;
        case BFS_NO_PATH:
            ch->sendf("@wYou should be in space to use the radar.@n\r\n");
            break;
        default:
            ch->sendf("@wYour radar detects @C%s@w to the @G%s@n\r\n", planet, dirs[dir]);
            break;
    }
    GET_PING(ch) = 5;

}


ACMD(do_radar) {
    int dir, found = false, fcount = 0;

    auto dradar = ch->findObjectVnum(12);
    if (!dradar) {
        ch->sendf("You do not even have a dragon radar!\r\n");
        return;
    }

    if (IS_NPC(ch)) {
        ch->sendf("You are a freaking mob!\r\n");
        return;
    }

    auto cr = ch->getRoom();

    WAIT_STATE(ch, PULSE_2SEC);
    act("$n holds up a dragon radar and pushes its button.", false, ch, nullptr, nullptr, TO_ROOM);
    for(auto vn : dbVnums) {
        auto o = get_last_inserted(objectVnumIndex, vn);
        if(!o) continue;
        auto r = o->getAbsoluteRoom();
        if(!r) continue;
        dir = find_first_step(cr, r);
        switch (dir) {
            case BFS_ERROR:
                ch->sendf("Hmm.. something seems to be wrong.\r\n");
                break;
            case BFS_ALREADY_THERE:
                ch->sendf("@D<@G%d@D>@w The radar detects a dragonball right here!\r\n", fcount);
                break;
            case BFS_NO_PATH:
                ch->sendf(
                             "@D<@G%d@D>@w The radar detects a faint dragonball signal, but can not direct you further.\r\n",
                             fcount);
                break;
            default:
                ch->sendf("@D<@G%d@D>@w The radar detects a dragonball %s of here.\r\n", fcount,
                             dirs[dir]);
                break;
        }
        found = true;
        break;
    }

    if (found == false) {
        ch->sendf("The radar didn't detect any dragonballs on the planet.\r\n");
        return;
    }
}

static std::string sense_align(Character *vict) {
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
    }
}

static std::string sense_compare(Character *ch, Character *vict) {
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
    }
}

ACMD(do_track) {
    char arg[MAX_INPUT_LENGTH];
    Character *vict;
    struct descriptor_data *i;
    int count = 0, dir;

    /* The character must have the track skill. */
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SENSE)) {
        ch->sendf("You have no idea how.\r\n");
        return;
    }
    
    if (GET_SUPPRESS(ch) <= 20 && GET_SUPPRESS(ch) > 0) {
        ch->sendf(
                     "You are concentrating too hard on suppressing your powerlevel at this level of suppression.\r\n");
        return;
    }

    one_argument(argument, arg);
    if (!*arg && !FIGHTING(ch)) {
        ch->sendf("Whom are you trying to sense?\r\n");
        return;
    } else if (!*arg && FIGHTING(ch)) {
        vict = FIGHTING(ch);
        ch->sendf("You focus on the one your are fighting.\r\n");
        if (AFF_FLAGGED(vict, AFF_NOTRACK) || IS_ANDROID(vict)) {
            ch->sendf("You can't sense them.\r\n");
            return;
        }
        if (!read_sense_memory(ch, vict)) {
            ch->sendf("You will remember their ki signal from now on.\r\n");
            sense_memory_write(ch, vict);
        }
        act("You look at $N@n intently for a moment.", true, ch, nullptr, vict, TO_CHAR);
        act("$n looks at you intently for a moment.", true, ch, nullptr, vict, TO_VICT);
        act("$n looks at $N@n intently for a moment.", true, ch, nullptr, vict, TO_NOTVICT);
        if (!IS_ANDROID(vict)) {
            ch->sendf(sense_align(vict).c_str());
            ch->sendf(sense_compare(ch, vict).c_str());
        } else {
            ch->sendf("You can't sense their powerlevel as they are a machine.\r\n");
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
                    ch->sendf("@D[@Y%d@D] @CYou sense @c%s@C.\r\n", (count + 1), get_i_name(ch, i->character));
                } else {
                    ch->sendf("@D[@Y%d@D] @CYou sense an unknown individual.\r\n", (count + 1));
                }
                /* How strong is the one we sense? */
                ch->sendf(sense_compare(ch, i->character).c_str());

                /* What's their alignment? */
				ch->sendf(sense_align(i->character).c_str());

                const char *blah = sense_location(i->character);
                ch->sendf("@wLastly you sense that they are at... @C%s@n\n", blah);
                ++count;
                //free(blah);
            }
        }
        if (count == 0) {
            ch->sendf("You sense that there is no one important around.@n\n");
        }
        return;
    }

    /* The person can't see the victim. */

    if (!(vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_WORLD))) {
        ch->sendf("No one is around by that name.\r\n");
        return;
    }

    /* We can't track the victim. */
    if (AFF_FLAGGED(vict, AFF_NOTRACK) || IS_ANDROID(vict)) {
        ch->sendf("You can't sense them.\r\n");
        return;
    }

    if (GET_HIT(vict) < (GET_HIT(ch) * 0.001) + 1) {
        if (IN_ROOM(ch) == IN_ROOM(vict)) {
            if (!read_sense_memory(ch, vict)) {
                ch->sendf(
                             "Their powerlevel is too weak for you to sense properly, but you will recognise their ki signal from now on.\r\n");
                sense_memory_write(ch, vict);
            } else {
                ch->sendf("Their powerlevel is too weak for you to sense properly.\r\n");
            }
        } else {
            ch->sendf("Their powerlevel is too weak for you to sense properly.\r\n");
        }
        return;
    }

    if(GET_SKILL_BASE(ch, SKILL_SENSE) == 100) {
        auto chRegion = find::holderType(ch->ent, ITEM_WORLD);
        auto vRegion = find::holderType(vict->ent, ITEM_WORLD);
        if(chRegion == vRegion) {
            ch->sendf("@WSense@D: %s@n\r\n", render::displayName(vRegion, ch->ent));
        }
    } else {

        if (GET_SKILL(ch, SKILL_SENSE) < rand_number(1, 101)) {
            auto candidates = ch->getRoom()->getUsableExits();

            if(candidates.empty()) {
                ch->sendf("You can't sense them from here.\r\n");
            } else {
                auto dir = Random::get(candidates);
                ch->sendf("You sense them %s faintly from here, but are unsure....\r\n", dirs[dir->first]);
            }
            
            improve_skill(ch, SKILL_SENSE, 1);
            improve_skill(ch, SKILL_SENSE, 1);
            improve_skill(ch, SKILL_SENSE, 1);
            return;           
        }

        /* They passed the skill check. */
        dir = find_first_step(ch->getRoom(), vict->getRoom());

        switch (dir) {
            case BFS_ERROR:
                ch->sendf("Hmm.. something seems to be wrong.\r\n");
                break;
            case BFS_ALREADY_THERE:
                act("You look at $N@n intently for a moment.", true, ch, nullptr, vict, TO_CHAR);
                act("$n looks at you intently for a moment.", true, ch, nullptr, vict, TO_VICT);
                act("$n looks at $N intently for a moment.", true, ch, nullptr, vict, TO_NOTVICT);
                if (!IS_ANDROID(vict)) {
                    ch->sendf(sense_align(vict).c_str());
                    ch->sendf(sense_compare(ch, vict).c_str());
                } else {
                    ch->sendf("You can't sense their powerlevel as they are a machine.\r\n");
                }
                break;
            case BFS_TO_FAR:
                ch->sendf("You are too far to sense %s accurately from here.\r\n", HMHR(vict));
                break;
            case BFS_NO_PATH:
                ch->sendf("You can't sense %s from here.\r\n", HMHR(vict));
                break;
            default:    /* Success! */
                if ((GET_SKILL_BASE(ch, SKILL_SENSE) >= 75)) {
                    
                    ch->sendf("You sense them %s from here!\r\n", dirs[dir]);
                    if(auto reg = find::holderType(vict->ent, ITEM_WORLD); reg != entt::null) ch->sendf("@WSense@D: @Y%s@n\r\n", render::displayName(reg, ch->ent));
                } else {
                    ch->sendf("You sense them %s from here!\r\n", dirs[dir]);
                    break;
                }
                WAIT_STATE(ch, PULSE_2SEC);
                improve_skill(ch, SKILL_SENSE, 1);
                improve_skill(ch, SKILL_SENSE, 1);
                improve_skill(ch, SKILL_SENSE, 1);
        }
    }
}

