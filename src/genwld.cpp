/************************************************************************
 * Generic OLC Library - Rooms / genwld.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#include <boost/algorithm/string.hpp>

#include "dbat/genwld.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/genolc.h"
#include "dbat/shop.h"
#include "dbat/dg_olc.h"
#include "dbat/constants.h"
#include "dbat/area.h"
#include "dbat/constants.h"
#include "dbat/filter.h"


/*
 * This function will copy the strings so be sure you free your own
 * copies of the description, title, and such.
 */
std::shared_ptr<room_data> room_data::shared() {
    return shared_from_this();
}

room_rnum add_room(struct room_data *room) {
    struct char_data *tch;
    struct obj_data *tobj;
    vnum j, found = false;
    room_rnum i;

    if (!room)
        return NOWHERE;

    if (world.contains(room->vn)) {
        auto ro = world.at(room->vn).get();
        extract_script(ro, WLD_TRIGGER);
        copy_room(ro, room);
        basic_mud_log("GenOLC: add_room: Updated existing room #%d.", room->vn);
        return i;
    }
    auto sh = std::shared_ptr<room_data>(room);
    world[room->vn] = sh;
    units[room->vn] = sh;
    basic_mud_log("GenOLC: add_room: Added room %d.", room->vn);

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

    room = get_room(rnum);

    /* This is something you might want to read about in the logs. */
    basic_mud_log("GenOLC: delete_room: Deleting room #%d (%s).", room->vn, room->name);

    if (r_mortal_start_room == rnum) {
        basic_mud_log("WARNING: GenOLC: delete_room: Deleting mortal start room!");
        r_mortal_start_room = 0;    /* The Void */
    }
    if (r_immort_start_room == rnum) {
        basic_mud_log("WARNING: GenOLC: delete_room: Deleting immortal start room!");
        r_immort_start_room = 0;    /* The Void */
    }
    if (r_frozen_start_room == rnum) {
        basic_mud_log("WARNING: GenOLC: delete_room: Deleting frozen start room!");
        r_frozen_start_room = 0;    /* The Void */
    }

    /*
     * Dump the contents of this room into the Void.  We could also just
     * extract the people, mobs, and objects here.
     */

    auto con = room->getObjects();
    for (auto obj : filter_raw(con)) {
        obj_from_room(obj);
        obj_to_room(obj, 0);
    }
    auto people = room->getPeople();
    for (auto ppl : filter_raw(people)) {
        char_from_room(ppl);
        char_to_room(ppl, 0);
    }

    extract_script(room, WLD_TRIGGER);
    free_proto_script(room, WLD_TRIGGER);

    /*
     * Change any exit going to this room to go the void.
     * Also fix all the exits pointing to rooms above this.
     */

    for(auto &[vn, r] : world) {
        for (j = 0; j < NUM_OF_DIRS; j++) {
            auto &e = r->dir_option[j];
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
    for (auto &[i, sh] : shop_index) {
        sh.in_room.erase(rnum);
    }

    units.erase(rnum);
    world.erase(rnum);
    return true;
}


int save_rooms(zone_rnum zone_num) {
    return true;
}

int copy_room(struct room_data *to, struct room_data *from) {
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
int copy_room_strings(struct room_data *dest, struct room_data *source) {
    int i;

    if (dest == nullptr || source == nullptr) {
        basic_mud_log("SYSERR: GenOLC: copy_room_strings: nullptr values passed.");
        return false;
    }

    dest->look_description = str_udup(source->look_description);
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
    if (room->look_description)
        free(room->look_description);
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

room_direction_data::~room_direction_data() {
    if (general_description)
        free(general_description);
    if (keyword)
        free(keyword);
}

/*
nlohmann::json room_data::serializeDgVars() {
    if(global_vars)
        return serializeVars(global_vars);
    return nlohmann::json::array();
}
*/

room_data::~room_data() {
    // fields like name are handled by the base destructor...
    // we just need to clean up exits.
    for(auto d : dir_option) {
        delete d;
    }
}

bool room_data::isActive() {
    return world.contains(vn);
}


int room_data::getDamage() {
    return damage;
}

void room_data::activate() {
    if(trig_list) {
        if(SCRIPT_TYPES(this) & OTRIG_RANDOM)
            roomSubscriptions.subscribe("randomTriggers", shared_from_this());
        if(SCRIPT_TYPES(this) & OTRIG_TIME)
            roomSubscriptions.subscribe("timeTriggers", shared_from_this());
    }
    if(damage != 0)
        roomSubscriptions.subscribe("roomRepairDamage", shared_from_this());
}

void room_data::deactivate() {
    roomSubscriptions.unsubscribeFromAll(shared_from_this());
}

int room_data::setDamage(int amount) {
    auto before = damage;
    damage = std::clamp<int>(amount, 0, 100);
    // if(dmg != before) save();
    if(damage == 0) {
        roomSubscriptions.unsubscribe("roomRepairDamage", shared_from_this());
    } else {
        roomSubscriptions.subscribe("roomRepairDamage", shared_from_this());
    }
    return damage;
}

int room_data::modDamage(int amount) {
    return setDamage(damage + amount);
}

struct room_data* room_direction_data::getDestination() {
    return get_room(to_room);
}

std::vector<std::weak_ptr<char_data>> room_data::getPeople() {
    std::vector<std::weak_ptr<char_data>> out;
    out.reserve(characters.size());
    std::copy(characters.begin(), characters.end(), std::back_inserter(out));
    out.shrink_to_fit();
    return out;
}

static const std::unordered_set<int> inside_sectors = {SECT_INSIDE, SECT_UNDERWATER, SECT_IMPORTANT, SECT_SHOP, SECT_SPACE};

static const std::map<std::string, int> _dirNames = {
    {"north", NORTH},
    {"east", EAST},
    {"south", SOUTH},
    {"west", WEST},
    {"up", UP},
    {"down", DOWN},
    {"northwest", NORTHWEST},
    {"northeast", NORTHEAST},
    {"southwest", SOUTHWEST},
    {"southeast", SOUTHEAST},
    {"inside", INDIR},
    {"outside", OUTDIR}

};

std::optional<std::string> room_data::dgCallMember(const std::string& member, const std::string& arg) {
    std::string lmember = member;
    boost::to_lower(lmember);
    boost::trim(lmember);
    char bitholder[MAX_STRING_LENGTH];

    if(auto d = _dirNames.find(lmember); d != _dirNames.end()) {
        auto ex = dir_option[d->second];
        if(!ex) {
            return "";
        }
        if (!arg.empty()) {
            if (!strcasecmp(arg.c_str(), "vnum"))
                return fmt::format("{}", ex->to_room);
            else if (!strcasecmp(arg.c_str(), "key"))
                return fmt::format("{}", ex->key);
            else if (!strcasecmp(arg.c_str(), "bits")) {
                sprintbit(ex->exit_info, exit_bits, bitholder, MAX_STRING_LENGTH);
                return bitholder;
            }
            else if (!strcasecmp(arg.c_str(), "room")) {
                if (auto roomFound = get_room(ex->to_room); roomFound)
                    return fmt::format("{}", roomFound->getUID(true));
                else
                    return "";
            }
        } else /* no subfield - default to bits */
            {
                sprintbit(ex->exit_info, exit_bits, bitholder, MAX_STRING_LENGTH);
                return bitholder;
            }
    }

    return {};
}

double room_data::setEnvironment(int type, double value) {
    environment[type] = value;
    return value;
}

double room_data::modEnvironment(int type, double value) {
    environment[type] += value;
    return environment[type];
}

void room_data::clearEnvironment(int type) {
    environment.erase(type);
}

static const std::vector<std::pair<std::pair<room_vnum, room_vnum>, double>> gravityRanges = {
    // North Kai's Planet
    {{6100, 6138}, 10.0},
    
    // Personal Pocket Dimensions
    {{18900, 19899}, 1000.0}, 

    // HBTC / Room of Spirit and Time - various ranges
    {{64000, 64006}, 100.0},  
    {{64007, 64016}, 300.0},
    {{64017, 64030}, 500.0},
    {{64031, 64048}, 1000.0},
    {{64049, 64070}, 5000.0},
    {{64071, 64096}, 10000.0},
    {{64097, 64097}, 1000.0},
};

double room_data::getEnvironment(int type) {
    auto planet = getPlanet(vn);
    switch(type) {
        case ENV_GRAVITY: {
            // check for a gravity generator...
            auto con = getObjects();
            for(auto c : filter_raw(con)) {
                if(c->gravity) return c->gravity.value();
            }

            // check gravityRanges
            for(const auto& [range, grav] : gravityRanges) {
                if(vn >= range.first && vn <= range.second) {
                    return grav;
                }
            }

            if(environment.contains(type))
                return environment[type];

            if(planet) {
                if(auto a = getPlanetEnvironment(planet, type); a) {
                    return a.value();
                }
            }
            
            return 1.0;
        }

        case ENV_WATER:
            if(ground_effect < 0) 
                return 100.0;
            switch(static_cast<int>(sector_type)) {
                case SECT_WATER_SWIM:
                    return 50.0;
                case SECT_WATER_NOSWIM:
                    return 75.0;
                case SECT_UNDERWATER:
                    return 100.0;
            }
            break;
        case ENV_MOONLIGHT: {
            if(!planet) return -1;
            for(auto f : {ROOM_INDOORS, ROOM_UNDERGROUND, ROOM_SPACE}) if(room_flags.get(f)) return -1;
            if(inside_sectors.contains(static_cast<int>(sector_type))) return -1;
            return getPlanetEnvironment(planet, type).value();
        }
        case ENV_ETHER_STREAM: {
            if(!planet) return 0.0;
            return getPlanetEnvironment(planet, type).value();
        }
    }
    if(environment.contains(type)) return environment[type];
    return 0.0;
}