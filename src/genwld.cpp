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
#include "dbat/constants.h"
#include "dbat/area.h"
#include "dbat/constants.h"
#include "dbat/filter.h"
#include "dbat/dg_scripts.h"
#include "dbat/send.h"

Room::Room() : AbstractLocation() {
    type = UnitType::room;
}

/*
 * This function will copy the strings so be sure you free your own
 * copies of the description, title, and such.
 */
std::shared_ptr<Room> Room::shared() {
    return shared_from_this();
}

room_rnum add_room(Room *room) {
    Character *tch;
    Object *tobj;
    vnum j, found = false;
    room_rnum i;

    if (!room)
        return NOWHERE;
    
    auto vn = room->getVnum();

    if (world.contains(vn)) {
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

int delete_room(room_rnum rnum) {
    room_rnum i;
    int j;
    Character *ppl, *next_ppl;
    Object *obj, *next_obj;
    Room *room;

    if (!world.count(rnum))    /* Can't delete void yet. */
        return false;
    
    if(rnum <= 0) {
        basic_mud_log("SYSERR: GenOLC: delete_room: Attempt to delete room with vnum <= 0.");
        return false;
    }

    room = get_room(rnum);

    /* This is something you might want to read about in the logs. */
    basic_mud_log("GenOLC: delete_room: Deleting room #%d (%s).", room->getVnum(), room->getName());

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
        obj->clearLocation();
        obj->setLocation(0);
    }
    auto people = room->getPeople();
    for (auto ppl : filter_raw(people)) {
        ppl->clearLocation();
        ppl->setLocation(0);
    }

    extract_script(room, WLD_TRIGGER);
    room->proto_script.clear();

    /*
     * Change any exit going to this room to go the void.
     * Also fix all the exits pointing to rooms above this.
     */

    for(auto &[vn, r] : world) {


    };

    /*
     * Remove this room from all shop lists.
     */
    for (auto &[i, sh] : shop_index) {
        sh.in_room.erase(rnum);
    }

    world.erase(rnum);
    return true;
}


int save_rooms(zone_rnum zone_num) {
    return true;
}

int copy_room(Room *to, Room *from) {
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
int copy_room_strings(Room *dest, Room *source) {
    int i;

    if (dest == nullptr || source == nullptr) {
        basic_mud_log("SYSERR: GenOLC: copy_room_strings: nullptr values passed.");
        return false;
    }

    dest->strings = source->strings;
    dest->extra_descriptions = source->extra_descriptions;

    for (auto& [d, e] : source->exits) {
        dest->exits[d].keyword = e.keyword;
        dest->exits[d].general_description = e.general_description;
    }


    return true;
}

int free_room_strings(Room *room) {
    int i;

    /* Free descriptions. */
    room->strings.clear();
    room->extra_descriptions.clear();

    /* Free exits. */
    for (auto& [d, e] : room->exits) {
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

std::string Room::getUID(bool active) const {
    return fmt::format("#R{}{}", vn, active ? "!" : "");
}

bool Room::isActive() const {
    return true;
}


int Room::getDamage() const {
    return damage;
}

void Room::activate() {
    assign_triggers(this, WLD_TRIGGER);
    
    if(!scripts.empty()) {
        if(SCRIPT_TYPES(this) & OTRIG_RANDOM)
            roomSubscriptions.subscribe("randomTriggers", shared_from_this());
        if(SCRIPT_TYPES(this) & OTRIG_TIME)
            roomSubscriptions.subscribe("timeTriggers", shared_from_this());
    }
    if(damage != 0)
        roomSubscriptions.subscribe("roomRepairDamage", shared_from_this());
}

void Room::deactivate() {
    roomSubscriptions.unsubscribeFromAll(shared_from_this());
}

int Room::setDamage(int amount) {
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

int Room::modDamage(int amount) {
    return setDamage(damage + amount);
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

std::optional<std::string> Room::dgCallMember(const std::string& member, const std::string& arg) {
    std::string lmember = member;
    boost::to_lower(lmember);
    boost::trim(lmember);
    char bitholder[MAX_STRING_LENGTH];

    if(auto d = _dirNames.find(lmember); d != _dirNames.end()) {
        auto ex = getDirection(static_cast<Direction>(d->second));
        if(!ex) {
            return "";
        }
        if (!arg.empty()) {
            if (!strcasecmp(arg.c_str(), "vnum")) {
                return fmt::format("{}", ex->getVnum());
            }
            else if (!strcasecmp(arg.c_str(), "key"))
                return fmt::format("{}", ex->key);
            else if (!strcasecmp(arg.c_str(), "bits")) {
                sprintbit(ex->exit_info, exit_bits, bitholder, MAX_STRING_LENGTH);
                return bitholder;
            }
            else if (!strcasecmp(arg.c_str(), "room")) {
                return fmt::format("{}", ex->getUID(true));
            }
        } else /* no subfield - default to bits */
            {
                sprintbit(ex->exit_info, exit_bits, bitholder, MAX_STRING_LENGTH);
                return bitholder;
            }
    }

    return {};
}

double Room::setEnvironment(int type, double value) {
    environment[type] = value;
    return value;
}

double Room::modEnvironment(int type, double value) {
    environment[type] += value;
    return environment[type];
}

void Room::clearEnvironment(int type) {
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

double Room::getEnvironment(int type) const {
    auto planet = getPlanet(getVnum());
    switch(type) {
        case ENV_GRAVITY: {
            // check for a gravity generator...
            auto con = getObjects();
            for(auto c : filter_raw(con)) {
                if(auto g = c->getBaseStat("gravity"); g > 0.0) return g;
            }

            // check gravityRanges
            for(const auto& [range, grav] : gravityRanges) {
                if(vn >= range.first && vn <= range.second) {
                    return grav;
                }
            }

            if(environment.contains(type))
                return environment.at(type);

            if(planet) {
                if(auto a = getPlanetEnvironment(planet.value(), type); a) {
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
            if(where_flags[WhereFlag::space]) return -1;
            for(auto f : {ROOM_INDOORS, ROOM_UNDERGROUND}) if(room_flags.get(f)) return -1;
            if(inside_sectors.contains(static_cast<int>(sector_type))) return -1;
            return getPlanetEnvironment(planet.value(), type).value();
        }
        case ENV_ETHER_STREAM: {
            if(!planet) return 0.0;
            return getPlanetEnvironment(planet.value(), type).value();
        }
    }
    if(environment.contains(type)) return environment.at(type);
    return 0.0;
}

vnum Room::getLocVnum() const {
    return getVnum();
}

const char* Room::getDgName() const {
    return HasMudStrings::getName();
}

std::vector<trig_vnum> Room::getProtoScript() const {
    return proto_script;
}


// This implementation is problematic because it recursively calls itself.
// Also, since unit_data::getName() takes no arguments, you cannot overload it with a const Coordinates&
// unless you want to provide a new interface. If you want to call the base version, do:

const std::vector<ExtraDescription>& Room::getExtraDescription(const Coordinates& coor) const {
    return HasExtraDescriptions::getExtraDescription();
}   

Zone* Room::getZone() const {
    return zone;
}

const char* Room::getName(const Coordinates& /*coor*/) const {
    return getName();
}

const char* Room::getLookDescription(const Coordinates& coor) const {
    // For rooms, look description does not vary by coordinate inside the room.
    return HasMudStrings::getLookDescription();
}

std::optional<Destination> Room::getDirection(Direction dir) const {
    if(exits.contains(dir)) return exits.at(dir);
    return std::nullopt;
}

std::map<Direction, Destination> Room::getDirections() const {
    std::map<Direction, Destination> out;
    for (const auto& [dir, dest] : exits) {
        if(dest) out[dir] = dest;
    }
    return out;
}

std::optional<Destination> Room::getDirection(const Coordinates& coor, Direction dir) {
    return getDirection(dir);
}

std::map<Direction, Destination> Room::getDirections(const Coordinates& coor) {
    return getDirections();
}

void Room::setRoomFlag(const Coordinates& coor, RoomFlag flag, bool value) {
    room_flags.set(flag, value);
}

bool Room::toggleRoomFlag(const Coordinates& coor, RoomFlag flag) {
    return room_flags.toggle(flag);
}

bool Room::getRoomFlag(const Coordinates& coor, RoomFlag flag) const {
    return room_flags.get(flag);
}

void Room::setWhereFlag(const Coordinates& coor, WhereFlag flag, bool value) {
    where_flags.set(flag, value);
}

bool Room::toggleWhereFlag(const Coordinates& coor, WhereFlag flag) {
    return where_flags.toggle(flag);
}

bool Room::getWhereFlag(const Coordinates& coor, WhereFlag flag) const {
    return where_flags.get(flag);
}

SectorType Room::getSectorType(const Coordinates& coor) const {
    return sector_type;
}

void Room::broadcastAt(const Coordinates& coor, const std::string& message) const {
    auto people = getPeople(coor);
    for (const auto &uw : people) {
        if (auto c = uw.lock()) {
                        c.get()->sendText(message.c_str());
        }
    }
}

int Room::getDamage(const Coordinates& coor) const {
    return getDamage();
}
int Room::setDamage(const Coordinates& coor, int amount) {
    return setDamage(amount);
}
int Room::modDamage(const Coordinates& coor, int amount) {
    return modDamage(amount);
}

int Room::getGroundEffect(const Coordinates& coor) const {
    return ground_effect;
}
void Room::setGroundEffect(const Coordinates& coor, int effect) {
    ground_effect = effect;
}

int Room::modGroundEffect(const Coordinates& coor, int effect) {
    ground_effect += effect;
    return ground_effect;
}

SpecialFunc Room::getSpecialFunc(const Coordinates& coor) const {
    return func;
}

double Room::getEnvironment(const Coordinates& coor, int type) const {
    return getEnvironment(type);
}
double Room::setEnvironment(const Coordinates& coor, int type, double value) {
    return setEnvironment(type, value);
}
double Room::modEnvironment(const Coordinates& coor, int type, double value) {
    return modEnvironment(type, value);
}
void Room::clearEnvironment(const Coordinates& coor, int type) {
    clearEnvironment(type);
}

void Room::sendText(const std::string& txt) {
    auto people = getPeople();
    for(auto i : filter_raw(people)) {
        i->sendText(txt);
    }

    for(auto d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING)
            continue;

        if (PRF_FLAGGED(d->character, PRF_ARENAWATCH)) {
            if (arena_watch(d->character) == vn) {
                d->sendText("@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n" + txt);
            }
        }
        if (auto eaves = GET_EAVESDROP(d->character); eaves > 0) {
            int roll = rand_number(1, 101);
            if (eaves == vn && GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
                d->sendText("@c-----Eavesdrop-----@n\r\n%s\r\n@c-----Eavesdrop-----@n\r\n" + txt);
            }
        }

    }
}

void Room::deleteExit(Direction dir) {
    if (auto find = exits.find(dir); find != exits.end()) {
        exits.erase(find);
    }
}

void Room::replaceExit(const Destination& dest) {
    exits[dest.dir] = dest;
}

void Room::deleteExit(const Coordinates& coor, Direction dir) {
    deleteExit(dir);
}

void Room::replaceExit(const Coordinates& coor, const Destination& dest) {
    replaceExit(dest);
}

void Room::onAddToContents(const Coordinates& coor, const std::shared_ptr<Object>& obj) {
    if (getVnum() == real_room(80)) {
        auc_load(obj.get());
    }
}

void Room::onAddToContents(const Coordinates& coor, const std::shared_ptr<Character>& ch) {

}

void Room::onRemoveFromContents(const std::shared_ptr<Object>& obj) {

}

void Room::onRemoveFromContents(const std::shared_ptr<Character>& ch) {

}
