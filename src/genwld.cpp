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
#include "dbat/dg_olc.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"


/*
 * This function will copy the strings so be sure you free your own
 * copies of the description, title, and such.
 */
room_rnum add_room(Room *room) {

    return 0;
}

/* -------------------------------------------------------------------------- */

int delete_room(room_rnum rnum) {

    return true;
}


int save_rooms(zone_rnum zone_num) {
    return true;
}

int copy_room(Room *to, Room *from) {


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

    return true;
}

int free_room_strings(Room *room) {

}



nlohmann::json Room::serialize() {
    auto j = GameEntity::serialize();

    if(sector_type) j["sector_type"] = sector_type;

    if(timed) j["timed"] = timed;
    if(dmg) j["dmg"] = dmg;
    if(geffect) j["geffect"] = geffect;

    for(auto p : proto_script) {
        if(trig_index.contains(p)) j["proto_script"].push_back(p);
    }

    return j;
}

Room::Room(const nlohmann::json &j) {
    deserialize(j);
}

void Room::deserialize(const nlohmann::json &j) {
    GameEntity::deserialize(j);
    if(j.contains("sector_type")) sector_type = j["sector_type"];
    if(j.contains("timed")) timed = j["timed"];
    if(j.contains("dmg")) dmg = j["dmg"];
    if(j.contains("geffect")) geffect = j["geffect"];
}

bool Room::isEnvironment() {
    return true;
}

std::optional<room_vnum> Room::getLaunchDestination() {
    return {};
}

double Room::getEnvVar(EnvVar v) {
    switch(v) {
        case EnvVar::Gravity: {
            // Check for a gravity generator.
            for(auto c : getInventory()) if(c->gravity) return c->gravity.value();

            // what about area rules? For legacy rooms, this should be a planet or dimension etc.
            if(auto env = getEnvironment(); env) {
                return env->getEnvVar(v);
            }

            // special cases here..
            if (vn >= 64000 && vn <= 64006) {
                return 100.0;
            }
            if (vn >= 64007 && vn <= 64016) {
                return 300.0;
            }
            if (vn >= 64017 && vn <= 64030) {
                return 500.0;
            }
            if (vn >= 64031 && vn <= 64048) {
                return 1000.0;
            }
            if (vn >= 64049 && vn <= 64070) {
                return 5000.0;
            }
            if (vn >= 64071 && vn <= 64096) {
                return 10000.0;
            }
            if (vn == 64097) {
                return 1000.0;
            }

            // safe default.
            return 1.0;
        }
        default:
            return 0.0;
    }
}

bool Room::isActive() {
    return world.contains(vn);
}


int Room::getDamage() {
    return dmg;
}

int Room::setDamage(int amount) {
    auto before = dmg;
    dmg = std::clamp<int>(amount, 0, 100);
    // if(dmg != before) save();
    return dmg;
}

int Room::modDamage(int amount) {
    return setDamage(dmg + amount);
}





bool Room::isSunken() {
    return sector_type == SECT_UNDERWATER || geffect < 0;
}


static const std::set<int> inside_sectors = {SECT_INSIDE, SECT_UNDERWATER, SECT_IMPORTANT, SECT_SHOP, SECT_SPACE};

MoonCheck Room::checkMoon() {
    for(auto f : {ROOM_INDOORS, ROOM_UNDERGROUND, ROOM_SPACE}) if(checkFlag(FlagType::Room, f)) return MoonCheck::NoMoon;
    if(inside_sectors.contains(sector_type)) return MoonCheck::NoMoon;
    auto plan = getPlanet();
    if(!plan) return MoonCheck::NoMoon;
    if(!plan->checkFlag(FlagType::Structure, STRUCTURE_MOON)) return MoonCheck::NoMoon;

    return MOON_TIMECHECK() ? MoonCheck::Full : MoonCheck::NotFull;

}

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

const std::vector<std::string> sky_look = {
                        "sunny",
                        "cloudy",
                        "rainy",
                        "lightning"
                };

DgResults Room::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    std::string lmember = member;
    to_lower(lmember);
    trim(lmember);
    char bitholder[MAX_STRING_LENGTH];

    if(auto d = _dirNames.find(lmember); d != _dirNames.end()) {
        auto exits = getExits();
        auto ex = exits[d->second];
        if(!ex) {
            return "";
        }
        if (!arg.empty()) {
            auto dest = ex->getDestination();
            if (!strcasecmp(arg.c_str(), "vnum")) {
                
                if(ex) return std::to_string(dest->getUID());
                return "";
            }
            else if (!strcasecmp(arg.c_str(), "key"))
                return fmt::format("{}", ex->key);
            else if (!strcasecmp(arg.c_str(), "bits")) {
                snprintf(bitholder, sizeof(bitholder), "%s", ex->getFlagNames(FlagType::Exit));
                return bitholder;
            }
            else if (!strcasecmp(arg.c_str(), "room")) {
                if(dest) return dest;
                return "";
            }
        } else /* no subfield - default to bits */
            {
                snprintf(bitholder, sizeof(bitholder), "%s", ex->getFlagNames(FlagType::Exit));
                return bitholder;
            }
    }

    if(lmember == "name") return getName();
    if(lmember == "sector") return sector_types[sector_type];
    if(lmember == "gravity") return fmt::format("{}", (int64_t)getEnvVar(EnvVar::Gravity));

    if(lmember == "vnum") {
        if(!arg.empty()) {
            auto v = atoll(arg.c_str());
            return vn == v ? "1":"0";
        }
        return fmt::format("{}", vn);
    }

    if(lmember == "contents") {
        if(arg.empty()) {
            if(auto inv = getInventory(); !inv.empty()) return inv.front();
            return "";
        }
        obj_vnum v = atoll(arg.c_str());
        if(auto found = findObjectVnum(v); found) return found;
        return "";
    }

    if(lmember == "people") {
        if(auto p = getPeople(); !p.empty()) return p.front();
        return "";
    }

    if(lmember == "id") return this;

    if(lmember == "weather") return !checkFlag(FlagType::Room, ROOM_INDOORS) ? sky_look[weather_info.sky] : "";

    if(lmember == "fishing") return checkFlag(FlagType::Room, ROOM_FISHING) ? "1" : "0";

    if(lmember == "roomflag") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(room_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return checkFlag(FlagType::Room, flag) ? "1" : "0";
    }

    if(lmember == "varexists") return script->hasVar(arg) ? "1" : "0";

    if(lmember == "zonenumber") return fmt::format("{}", zone);
    if(lmember == "zonename") return zone_table[zone].name;

    if(script->hasVar(lmember)) {
        return script->getVar(lmember);
    } else {
        script_log("Trigger: %s, VNum %d. unknown room field: '%s'",
                               GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), lmember.c_str());
    }

    return "";
}

std::string Room::getUnitClass() {
    return "Room";
}

UnitFamily Room::getFamily() {
    return UnitFamily::Room;
}

void Room::assignTriggers() {

    // remove all duplicates from i->proto_script but do not change its order otherwise.
    std::set<trig_vnum> existVnums;
    std::set<trig_vnum> valid;
    for(auto t : proto_script) valid.insert(t);
    
    for(auto t : script->dgScripts) existVnums.insert(t->parent->vn);
    bool added = false;
    bool removed = false;

    // remove any dgScript instances in i->script->dgScripts that aren't in i->proto_script
    std::list<std::shared_ptr<trig_data>> validScripts;
    for(auto t : script->dgScripts) {
        if(valid.contains(t->parent->vn)) {
            validScripts.push_back(t);
        }
        else {
            removed = true;
        }
    }
    if(removed) script->dgScripts = validScripts;

    for(auto p : proto_script) {
        // only add if they don't already have one...
        if(!existVnums.contains(p)) {
            script->addTrigger(read_trigger(p), -1);
            added = true;
            existVnums.insert(p);
        }
    }

    if(added || removed) {
        // we need to sort i->script->dgScripts by the order of i->proto_script
        std::list<std::shared_ptr<trig_data>> sorted;
        for(auto p : proto_script) {
            for(auto t : script->dgScripts) {
                if(t->parent->vn == p) {
                    sorted.push_back(t);
                    break;
                }
            }
        }
        script->dgScripts = sorted;
    }

}

static const std::set<int> lit_sectors = {SECT_INSIDE, SECT_CITY, SECT_IMPORTANT, SECT_SHOP, SECT_SPACE};


// Check to see whether a room should normally be considered dark as a basic matter of course.
bool Room::isInsideNormallyDark() {
    if(checkFlag(FlagType::Room, ROOM_DARK)) return true;
    if(lit_sectors.contains(sector_type)) return false;
    if(checkFlag(FlagType::Room, ROOM_INDOORS)) return false;

    return false;
}

static const std::set<int> sun_down = {SUN_SET, SUN_DARK};

bool Room::isInsideDark() {

    // If the room is not normally dark, then it's definitely not dark.
    if(!isInsideNormallyDark()) return false;

    // Certain sectors, like cities, provide free light.
    if(lit_sectors.contains(sector_type)) return false;

    // Failing that, maybe the sun is up?
    if(!sun_down.contains(weather_info.sunlight)) return false;

    // welp, now it's time for the most expensive operation of all.
    for(auto u : getContents()) {
        if(u->isProvidingLight()) return false;
    }

    return true;
}

// EXITS below here.
Room* Exit::getDestination() {
    return destination;
}

UnitFamily Exit::getFamily() {
    return UnitFamily::Exit;
}

std::string Exit::getUnitClass() {
    return "Exit";
}

nlohmann::json Exit::serialize() {
    auto j = GameEntity::serialize();

    if(key > 0) j["key"] = key;

    if(dclock) j["dclock"] = dclock;
    if(dchide) j["dchide"] = dchide;
    if(dcskill) j["dcskill"] = dcskill;
    if(dcmove) j["dcmove"] = dcmove;
    if(failsavetype) j["failsavetype"] = failsavetype;
    if(dcfailsave) j["dcfailsave"] = dcfailsave;

    return j;
}

void Exit::deserialize(const nlohmann::json& j) {
    GameEntity::deserialize(j);
    if(j.contains("key")) key = j["key"];
    if(j.contains("dclock")) dclock = j["dclock"];
    if(j.contains("dchide")) dchide = j["dchide"];
    if(j.contains("dcskill")) dcskill = j["dcskill"];
    if(j.contains("dcmove")) dcmove = j["dcmove"];
    if(j.contains("failsavetype")) failsavetype = j["failsavetype"];
    if(j.contains("dcfailsave")) dcfailsave = j["dcfailsave"];

}

nlohmann::json Exit::serializeRelations() {
    auto j = GameEntity::serializeRelations();
    if(destination) j["destination"] = destination->getUIDString();
    if(failroom) j["failroom"] = failroom->getUIDString();
    if(totalfailroom) j["totalfailroom"] = totalfailroom->getUIDString();
    return j;

}

void Exit::deserializeRelations(const nlohmann::json& j) {
    GameEntity::deserializeRelations(j);
    if(j.contains("destination")) {
        destination = dynamic_cast<Room*>(resolveUID(j["destination"].get<std::string>()));
    }
    if(j.contains("failroom")) {
        failroom = dynamic_cast<Room*>(resolveUID(j["failroom"].get<std::string>()));
    }
    if(j.contains("totalfailroom")) {
        totalfailroom = dynamic_cast<Room*>(resolveUID(j["totalfailroom"].get<std::string>()));
    }
}

Exit::Exit(const nlohmann::json &j) : Exit() {
    deserialize(j);
}


std::string Exit::getName() {
    return dirs[locationType];
}

std::vector<std::string> Exit::getKeywords(GameEntity *looker) {
    std::vector<std::string> out;
    out.emplace_back(getName());
    if(auto al = getAlias(); !al.empty()) out.emplace_back(al);
    return out;
}