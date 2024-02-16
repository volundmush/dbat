#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"



extra_descr_data::extra_descr_data(const nlohmann::json& j) {
    deserialize(j);
}

nlohmann::json extra_descr_data::serialize() {
    nlohmann::json j;
    if(!keyword.empty()) j["keyword"] = keyword;
    if(!description.empty()) j["description"] = description;
    return j;
}

std::vector<room_data*> unit_data::getRooms() {
    std::vector<struct room_data*> out;
    for(auto u : contents) {
        if(auto r = dynamic_cast<room_data*>(u); r) out.push_back(r);
    }
    return out;
}

std::vector<char_data*> unit_data::getPeople() {
    std::vector<struct char_data*> out;
    for(auto u : contents) {
        if(auto c = dynamic_cast<char_data*>(u); c) out.push_back(c);
    }
    return out;
}

std::vector<obj_data*> unit_data::getInventory() {
    std::vector<obj_data*> out;
    for(auto u : contents) {
        auto o = dynamic_cast<obj_data*>(u);
        if(o && o->locationType == 0) out.push_back(o);
    }
    return out;
}

std::map<int, obj_data*> unit_data::getEquipment() {
    std::map<int, obj_data*> out;
    for(auto u : contents) {
        auto o = dynamic_cast<obj_data*>(u);
        if(o && o->locationType > 0) out[o->locationType] = o;
    }
    return out;
}

std::map<int, exit_data*> unit_data::getExits() {
    std::map<int, exit_data*> out;
    for(auto u : contents) {
        auto o = dynamic_cast<exit_data*>(u);
        if(!o) continue;
        out[o->locationType] = o;
    }
    return out;
}

std::map<int, exit_data*> unit_data::getUsableExits() {
    std::map<int, exit_data*> out;
    for(auto u : contents) {
        auto o = dynamic_cast<exit_data*>(u);
        if(!o) continue;
        if(o->checkFlag(FlagType::Exit, EX_CLOSED)) continue;
        auto dest = o->getDestination();
        if(!dest) continue;
        if(dest->checkFlag(FlagType::Room, ROOM_DEATH)) continue;
        out[o->locationType] = o;
    }
    return out;
}

unit_data* unit_data::getLocation() {
    return location;
}

room_data* unit_data::getRoom() {
    return dynamic_cast<room_data*>(getLocation());
}

room_data* unit_data::getAbsoluteRoom() {
    if(auto room = getRoom(); room) {
        return room;
    }
    if(auto loc = getLocation(); loc) return loc->getAbsoluteRoom();
    return nullptr;
}

nlohmann::json unit_data::serialize() {
    nlohmann::json j;

    if(vn != NOTHING) j["vn"] = vn;

    if(name && strlen(name)) j["name"] = name;
    if(room_description && strlen(room_description)) j["room_description"] = room_description;
    if(look_description && strlen(look_description)) j["look_description"] = look_description;
    if(short_description && strlen(short_description)) j["short_description"] = short_description;

    for(auto &ex : ex_description) {
        j["ex_description"].push_back(ex.serialize());
    }

    if(uid != NOTHING) j["uid"] = uid;
    if(zone != NOTHING) j["zone"] = zone;

    if(script && (!script->dgScripts.empty() || !script->vars.empty())) {
        j["dgScripts"] = script->serialize();
    }

    if(!flags.empty()) {
        for(auto &[type, f] : flags) {
            if(f.empty()) continue;
            j["flags"].push_back(std::make_pair(type, std::vector<int>(f.begin(), f.end())));
        }
    }

    return j;
}


void unit_data::deserialize(const nlohmann::json& j) {
    if(j.contains("vn")) vn = j["vn"];
    if(j.contains("name")) {
        if(name) free(name);
        name = strdup(j["name"].get<std::string>().c_str());
    }
    if(j.contains("room_description")) {
        if(room_description) free(room_description);
        room_description = strdup(j["room_description"].get<std::string>().c_str());
    }
    if(j.contains("look_description")) {
        if(look_description) free(look_description);
        look_description = strdup(j["look_description"].get<std::string>().c_str());
    }
    if(j.contains("short_description")) {
        if(short_description) free(short_description);
        short_description = strdup(j["short_description"].get<std::string>().c_str());
    }

    if(j.contains("ex_description")) {
        auto &e = j["ex_description"];
        for(auto ej : e) ex_description.emplace_back(ej);
    }

    if(j.contains("uid")) uid = j["uid"];
    if(j.contains("zone")) zone = j["zone"];

    if(j.contains("dgScripts")) {
        if(!script) script = std::make_shared<script_data>(this);
        script->deserialize(j["dgScripts"]);
    }

}

void unit_data::activateContents() {

}

void unit_data::deactivateContents() {

}

std::string unit_data::scriptString() {
    return fmt::format("@D[@wT{}@D]@n", fmt::join(proto_script, ","));
}

double unit_data::getInventoryWeight() {
    double weight = 0;
    for(auto obj : getInventory()) {
        weight += obj->getTotalWeight();
    }
    return weight;
}

int64_t unit_data::getInventoryCount() {
    int64_t total = 0;
    for(auto obj : getInventory()) {
        total++;
    }
    return total;
}

struct obj_data* unit_data::findObject(const std::function<bool(struct obj_data*)> &func, bool working) {
    for(auto obj : getInventory()) {
        if(func(obj)) {
            if(working && !obj->isWorking()) continue;
            return obj;
        }
        if(auto p = obj->findObject(func, working); p) return p;
    }
    return nullptr;
}

struct obj_data* unit_data::findObjectVnum(obj_vnum objVnum, bool working) {
    return findObject([objVnum](auto o) {return o->vn == objVnum;}, working);
}

std::set<struct obj_data*> unit_data::gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working) {
    std::set<struct obj_data*> out;
    for(auto obj : getInventory()) {
        if(func(obj)) {
            if(working && !obj->isWorking()) continue;
            out.insert(obj);
        }
        auto contents = obj->gatherObjects(func, working);
        out.insert(contents.begin(), contents.end());
    }
    return out;
}

std::string unit_data::getUID(bool active) {
    return fmt::format("#{}{}", uid, active ? "" : "!");
}

DgResults unit_data::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    return "";
}

bool unit_data::isActive() {
    return false;
}

void unit_data::save() {

}

std::string unit_data::getName() {
    return name ? name : "";
}

void unit_data::setName(const std::string& n) {
    if(name) free(name);
    name = strdup(n.c_str());
}

std::string unit_data::getShortDesc() {
    return short_description ? short_description : "";
}

void unit_data::setShortDesc(const std::string& n) {
    if(short_description) free(short_description);
    short_description = strdup(n.c_str());
}

std::string unit_data::getRoomDesc() {
    return room_description ? room_description : "";
}

void unit_data::setRoomDesc(const std::string& n) {
    if(room_description) free(room_description);
    room_description = strdup(n.c_str());
}

std::string unit_data::getLookDesc() {
    return look_description ? look_description : "";
}

void unit_data::setLookDesc(const std::string& n) {
    if(look_description) free(look_description);
    look_description = strdup(n.c_str());
}


void unit_data::checkMyID() {
    if(uid == -1) {
        uid = getNextUID();
        basic_mud_log("Unit Found with ID -1. Automatically fixed to ID %d", uid);
    }
}

std::string unit_data::getDisplayName(struct unit_data* ch) {
    return "Nameless";
}

std::string unit_data::renderAppearance(struct unit_data* ch) {
    return "You see nothing special.";
}

std::vector<std::string> unit_data::getKeywords(struct unit_data* ch) {
    return {};
}



unit_data::~unit_data() {
    if(name) free(name);
    if(short_description) free(short_description);
    if(room_description) free(room_description);
    if(look_description) free(look_description);
    if(script) script.reset();
}


void unit_data::assignTriggers() {
    // does nothing.
}

bool unit_data::checkFlag(FlagType type, int flag) {
    if(auto foundType = flags.find(type); foundType != flags.end()) {
        return foundType->second.contains(flag);
    }
    return false;
}

void unit_data::setFlag(FlagType type, int flag, bool value) {
    auto &f = flags[type];
    if(value) {
        f.insert(flag);
    } else {
        f.erase(flag);
    }
}

void unit_data::clearFlag(FlagType type, int flag) {
    setFlag(type, flag, false);
}

bool unit_data::flipFlag(FlagType type, int flag) {
    auto &f = flags[type];
    if(f.contains(flag)) {
        f.erase(flag);
        return false;
    } else {
        f.insert(flag);
        return true;
    }
}


nlohmann::json unit_data::serializeLocation() {
    nlohmann::json j = nlohmann::json::object();

    if(location) {
        j["location"] = location->getUID();
        if(locationType) j["locationType"] = locationType;
        if(auto co = coords.serialize(); !co.empty()) j["coords"] = co;
    }

    return j;
}

nlohmann::json unit_data::serializeRelations() {
    nlohmann::json j = nlohmann::json::object();


    return j;
}


void unit_data::removeFromLocation() {
    if(location) {
        basic_mud_log("Attempted to remove unit '%d: %s' from location, but location was not found.", uid, getName().c_str());
        locationType = -1;
        coords.clear();
        return;
    }
    
    location->handleRemove(this);

    location = nullptr;
    locationType = -1;
    coords.clear();
    
}


void unit_data::handleRemove(unit_data *u) {

    // remove uid from loc->contents
    std::erase_if(contents, [&](auto ud) {return ud == u;});
}

std::vector<unit_data*> unit_data::getNeighbors(bool visible) {
    auto loc = getLocation();
    if(!loc) return {};
    return loc->getNeighbors(visible);
}

bool unit_data::canSee(unit_data *u) {
    return true;
}

std::vector<unit_data*> unit_data::getNeighborsFor(unit_data *u, bool visible) {
    std::vector<unit_data*> out;
    for(auto n : contents) {
        if(u == n) continue;
        if(visible && !u->canSee(n)) continue;
        out.push_back(n);
    }

    return out;
}



Searcher& Searcher::setAllowAll(bool allow) {
    allowAll = allow;
    return *this;
}

Searcher& Searcher::setAllowSelf(bool allow) {
    allowSelf = allow;
    return *this;
}

Searcher& Searcher::setAllowHere(bool allow) {
    allowHere = allow;
    return *this;
} 

Searcher& Searcher::setAllowRecurse(bool allow) {
    allowRecurse = allow;
    return *this;
}


std::vector<unit_data*> Searcher::search() {
    trim(args);
    if(args.empty()) return {};
    if(allowSelf && iequals(args, "self")) return {caller};
    if(allowHere && iequals(args, "here")) return {caller->getLocation()};

    auto candidates = doSearch();
    
    int counter = 0;
    int prefix = 1;
    bool allMode = false;
    std::string targetName;
    
    // args might be formatted like "blah" or like "5.blah" or "all.blah".
    // We need to split by the first . if it exists.
    // Then we check if it's a number or all.

    if(auto dot = args.find('.'); dot != std::string::npos) {
        auto prefixStr = args.substr(0, dot);
        if(iequals(prefixStr, "all")) {
            allMode = allowAll;
            if(!allMode) {
                caller->sendLine("You are not allowed to use 'all' in this context.");
                return {};
            }
        } else {
            prefix = std::stoi(prefixStr);
        }
        targetName = args.substr(dot+1);
    } else {
        targetName = args;
    }

    if(allowAsterisk && iequals(targetName, "*")) {
        return candidates;
    }

    for(auto c : candidates) {
        auto keywords = c->getKeywords(caller);

        for(auto k : keywords) {
            if(iequals(k, targetName)) {
                if(counter == prefix) {
                    return {c};
                }
                counter++;
                break;
            }
        }

    }

    return {};
}

unit_data* Searcher::getOne() {
    auto results = search();
    if(results.size() == 1) return results.front();
    return nullptr;
}


bool unit_data::isEnvironment() {
    return false;
}


bool unit_data::isRegion() {
    return false;
}

bool unit_data::isStructure() {
    return false;
}

unit_data* unit_data::getEnvironment() {
    auto loc = getLocation();
    while(loc) {
        if(loc->isEnvironment()) return loc;
        loc = loc->getLocation();
        if(loc == this) return nullptr;
    }
    return nullptr;
}

unit_data* unit_data::getRegion() {
    auto loc = getLocation();
    while(loc) {
        if(loc->isRegion()) return loc;
        loc = loc->getLocation();
        if(loc == this) return nullptr;
    }
    return nullptr;
}

unit_data* unit_data::getStructure() {
    auto loc = getLocation();
    while(loc) {
        if(loc->isStructure()) return loc;
        loc = loc->getLocation();
        if(loc == this) return nullptr;
    }
    return nullptr;
}

double unit_data::myEnvVar(EnvVar v) {
    if(auto env = getEnvironment(); env) return env->getEnvVar(v);
    return 0.0;
}

double unit_data::getEnvVar(EnvVar v) {
    return 0.0;
}

void unit_data::extractFromWorld() {
    exists = false;
    pendingDeletions.insert(this);
    if(script) script->deactivate();

    for(auto c : contents) {
        c->onHolderExtraction();
    }

}