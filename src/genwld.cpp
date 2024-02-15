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
room_rnum add_room(struct room_data *room) {

    return 0;
}

/* -------------------------------------------------------------------------- */

int delete_room(room_rnum rnum) {

    return true;
}


int save_rooms(zone_rnum zone_num) {
    return true;
}

int copy_room(struct room_data *to, struct room_data *from) {


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

    return true;
}

int free_room_strings(struct room_data *room) {

}

exit_data::~exit_data() {
    if (general_description)
        free(general_description);
    if (keyword)
        free(keyword);
}

nlohmann::json exit_data::serialize() {
    nlohmann::json j;

    if(general_description && strlen(general_description)) j["general_description"] = general_description;
    if(keyword && strlen(keyword)) j["keyword"] = keyword;
    if(exit_info) j["exit_info"] = exit_info;
    if(key > 0) j["key"] = key;
	if(to_room != NOWHERE) j["to_room"] = to_room;
    if(dclock) j["dclock"] = dclock;
    if(dchide) j["dchide"] = dchide;
    if(dcskill) j["dcskill"] = dcskill;
    if(dcmove) j["dcmove"] = dcmove;
    if(failsavetype) j["failsavetype"] = failsavetype;
    if(dcfailsave) j["dcfailsave"] = dcfailsave;
    if(failroom > 0) j["failroom"] = failroom;
    if(totalfailroom > 0) j["totalfailroom"] = totalfailroom;

    return j;
}

exit_data::exit_data(const nlohmann::json &j) : exit_data() {
    if(j.contains("general_description")) general_description = strdup(j["general_description"].get<std::string>().c_str());
    if(j.contains("keyword")) keyword = strdup(j["keyword"].get<std::string>().c_str());
    if(j.contains("exit_info")) exit_info = j["exit_info"].get<int16_t>();
    if(j.contains("key")) key = j["key"];
    if(j.contains("to_room")) to_room = j["to_room"];
    if(j.contains("dclock")) dclock = j["dclock"];
    if(j.contains("dchide")) dchide = j["dchide"];
    if(j.contains("dcskill")) dcskill = j["dcskill"];
    if(j.contains("dcmove")) dcmove = j["dcmove"];
    if(j.contains("failsavetype")) failsavetype = j["failsavetype"];
    if(j.contains("dcfailsave")) dcfailsave = j["dcfailsave"];
    if(j.contains("failroom")) failroom = j["failroom"];
    if(j.contains("totalfailroom")) totalfailroom = j["totalfailroom"];
}

nlohmann::json room_data::serialize() {
    auto j = unit_data::serialize();

    if(sector_type) j["sector_type"] = sector_type;


    if(timed) j["timed"] = timed;
    if(dmg) j["dmg"] = dmg;
    if(geffect) j["geffect"] = geffect;

    for(auto p :proto_script) {
        if(trig_index.contains(p)) j["proto_script"].push_back(p);
    }

    return j;
}

room_data::room_data(const nlohmann::json &j) {
    deserialize(j);

    if(j.contains("sector_type")) sector_type = j["sector_type"];
    if(j.contains("timed")) timed = j["timed"];
    if(j.contains("dmg")) dmg = j["dmg"];
    if(j.contains("geffect")) geffect = j["geffect"];

}



static bool checkGravity(const area_data &a) {
    return a.gravity.has_value();
}

double room_data::getGravity() {
    // check for a gravity generator...
    for(auto c : getInventory()) {
        if(c->gravity) return c->gravity.value();
    }

    // what about area rules?
    if(std::optional<vnum> gravArea = getMatchingArea(checkGravity); gravArea) {
        auto &a = areas[gravArea.value()];
        return a.gravity.value();
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

    return 1.0;
}

bool room_data::isActive() {
    return world.contains(vn);
}


void room_data::save() {

}

int room_data::getDamage() {
    return dmg;
}

int room_data::setDamage(int amount) {
    auto before = dmg;
    dmg = std::clamp<int>(amount, 0, 100);
    // if(dmg != before) save();
    return dmg;
}

int room_data::modDamage(int amount) {
    return setDamage(dmg + amount);
}

struct room_data* exit_data::getDestination() {
    auto found = world.find(to_room);
    if(found != world.end()) return dynamic_cast<room_data*>(found->second);
    return nullptr;
}


bool room_data::isSunken() {
    return sector_type == SECT_UNDERWATER || geffect < 0;
}

std::optional<room_vnum> room_data::getLaunchDestination() {
    if(!area) return NOWHERE;
    if(!areas.contains(area.value())) return NOWHERE;
    auto &a = areas[area.value()];
    return a.getLaunchDestination();
}



static const std::set<int> inside_sectors = {SECT_INSIDE, SECT_UNDERWATER, SECT_IMPORTANT, SECT_SHOP, SECT_SPACE};

MoonCheck room_data::checkMoon() {
    for(auto f : {ROOM_INDOORS, ROOM_UNDERGROUND, ROOM_SPACE}) if(checkFlag(FlagType::Room, f)) return MoonCheck::NoMoon;
    if(inside_sectors.contains(sector_type)) return MoonCheck::NoMoon;
    auto check_planet = getMatchingArea(area_data::isPlanet);
    if(!check_planet) return MoonCheck::NoMoon;
    auto &area = areas[*check_planet];
    if(!area.flags.test(AREA_MOON)) return MoonCheck::NoMoon;

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

DgResults room_data::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    std::string lmember = member;
    to_lower(lmember);
    trim(lmember);
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
                if (auto roomFound = world.find(ex->to_room); roomFound != world.end())
                    return fmt::format("{}", roomFound->second->getUID(false));
                else
                    return "";
            }
        } else /* no subfield - default to bits */
            {
                sprintbit(ex->exit_info, exit_bits, bitholder, MAX_STRING_LENGTH);
                return bitholder;
            }
    }

    if(lmember == "name") return name;
    if(lmember == "sector") return sector_types[sector_type];
    if(lmember == "gravity") return fmt::format("{}", (int64_t)getGravity());

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

std::string room_data::getUnitClass() {
    return "room_data";
}

UnitFamily room_data::getFamily() {
    return UnitFamily::Room;
}

void room_data::assignTriggers() {

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
