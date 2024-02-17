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

std::vector<Room*> GameEntity::getRooms() {
    std::vector<Room*> out;
    for(auto u : contents) {
        if(auto r = dynamic_cast<Room*>(u); r) out.push_back(r);
    }
    return out;
}

std::vector<BaseCharacter*> GameEntity::getPeople() {
    std::vector<BaseCharacter*> out;
    for(auto u : contents) {
        if(auto c = dynamic_cast<BaseCharacter*>(u); c) out.push_back(c);
    }
    return out;
}

std::vector<Object*> GameEntity::getInventory() {
    std::vector<Object*> out;
    for(auto u : contents) {
        auto o = dynamic_cast<Object*>(u);
        if(o && o->locationType == 0) out.push_back(o);
    }
    return out;
}

std::map<int, Object*> GameEntity::getEquipment() {
    std::map<int, Object*> out;
    for(auto u : contents) {
        auto o = dynamic_cast<Object*>(u);
        if(o && o->locationType > 0) out[o->locationType] = o;
    }
    return out;
}

std::map<int, Exit*> GameEntity::getExits() {
    std::map<int, Exit*> out;
    for(auto u : contents) {
        auto o = dynamic_cast<Exit*>(u);
        if(!o) continue;
        out[o->locationType] = o;
    }
    return out;
}

std::map<int, Exit*> GameEntity::getUsableExits() {
    std::map<int, Exit*> out;
    for(auto u : contents) {
        auto o = dynamic_cast<Exit*>(u);
        if(!o) continue;
        if(o->checkFlag(FlagType::Exit, EX_CLOSED)) continue;
        auto dest = o->getDestination();
        if(!dest) continue;
        if(dest->checkFlag(FlagType::Room, ROOM_DEATH)) continue;
        out[o->locationType] = o;
    }
    return out;
}

GameEntity* GameEntity::getLocation() {
    return location;
}

Room* GameEntity::getRoom() {
    return dynamic_cast<Room*>(getLocation());
}

Room* GameEntity::getAbsoluteRoom() {
    if(auto room = getRoom(); room) {
        return room;
    }
    if(auto loc = getLocation(); loc) return loc->getAbsoluteRoom();
    return nullptr;
}

nlohmann::json GameEntity::serialize() {
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

    for(auto &[type, f] : flags) {
        if(f.empty()) continue;
        j["flags"].push_back(std::make_pair(type, std::vector<int>(f.begin(), f.end())));
    }

    for(auto &[v, d] : envVars) {
        j["envVars"].push_back(std::make_pair(v, d));
    }

    return j;
}


void GameEntity::deserialize(const nlohmann::json& j) {
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

    if(j.contains("envVars")) {
        for(auto &jv : j["envVars"]) {
            envVars[jv[0].get<EnvVar>()] = jv[1].get<double>();
        }
    }

}

void GameEntity::activateContents() {

}

void GameEntity::deactivateContents() {

}

std::string GameEntity::scriptString() {
    return fmt::format("@D[@wT{}@D]@n", fmt::join(proto_script, ","));
}

double GameEntity::getInventoryWeight() {
    double weight = 0;
    for(auto obj : getInventory()) {
        weight += obj->getTotalWeight();
    }
    return weight;
}

int64_t GameEntity::getInventoryCount() {
    int64_t total = 0;
    for(auto obj : getInventory()) {
        total++;
    }
    return total;
}

Object* GameEntity::findObject(const std::function<bool(Object*)> &func, bool working) {
    for(auto obj : getInventory()) {
        if(func(obj)) {
            if(working && !obj->isWorking()) continue;
            return obj;
        }
        if(auto p = obj->findObject(func, working); p) return p;
    }
    return nullptr;
}

Object* GameEntity::findObjectVnum(obj_vnum objVnum, bool working) {
    return findObject([objVnum](auto o) {return o->vn == objVnum;}, working);
}

std::set<Object*> GameEntity::gatherObjects(const std::function<bool(Object*)> &func, bool working) {
    std::set<Object*> out;
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

std::string GameEntity::getUIDString(bool active) {
    return fmt::format("#{}{}", uid, active ? "" : "!");
}

DgResults GameEntity::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    return "";
}

bool GameEntity::isActive() {
    return false;
}

std::string GameEntity::getName() {
    return name ? name : "";
}

void GameEntity::setName(const std::string& n) {
    if(name) free(name);
    name = strdup(n.c_str());
}

std::string GameEntity::getShortDesc() {
    return short_description ? short_description : "";
}

void GameEntity::setShortDesc(const std::string& n) {
    if(short_description) free(short_description);
    short_description = strdup(n.c_str());
}

std::string GameEntity::getRoomDesc() {
    return room_description ? room_description : "";
}

void GameEntity::setRoomDesc(const std::string& n) {
    if(room_description) free(room_description);
    room_description = strdup(n.c_str());
}

std::string GameEntity::getLookDesc() {
    return look_description ? look_description : "";
}

void GameEntity::setLookDesc(const std::string& n) {
    if(look_description) free(look_description);
    look_description = strdup(n.c_str());
}


void GameEntity::checkMyID() {
    if(uid == -1) {
        uid = getNextUID();
        basic_mud_log("Unit Found with ID -1. Automatically fixed to ID %d", uid);
    }
}

std::string GameEntity::getDisplayName(GameEntity* ch) {
    return "Nameless";
}

std::string GameEntity::renderAppearance(GameEntity* ch) {
    return "You see nothing special.";
}

std::vector<std::string> GameEntity::getKeywords(GameEntity* ch) {
    return {};
}



GameEntity::~GameEntity() {
    if(name) free(name);
    if(short_description) free(short_description);
    if(room_description) free(room_description);
    if(look_description) free(look_description);
    if(script) script.reset();
}


void GameEntity::assignTriggers() {
    // does nothing.
}

bool GameEntity::checkFlag(FlagType type, int flag) {
    if(auto foundType = flags.find(type); foundType != flags.end()) {
        return foundType->second.contains(flag);
    }
    return false;
}

void GameEntity::setFlag(FlagType type, int flag, bool value) {
    auto &f = flags[type];
    if(value) {
        f.insert(flag);
    } else {
        f.erase(flag);
    }
}

void GameEntity::clearFlag(FlagType type, int flag) {
    setFlag(type, flag, false);
}

bool GameEntity::flipFlag(FlagType type, int flag) {
    auto &f = flags[type];
    if(f.contains(flag)) {
        f.erase(flag);
        return false;
    } else {
        f.insert(flag);
        return true;
    }
}


nlohmann::json GameEntity::serializeRelations() {
    nlohmann::json j = nlohmann::json::object();

    if(location) {
        j["location"] = location->getUIDString();
        if(locationType) j["locationType"] = locationType;
        if(auto co = coords.serialize(); !co.empty()) j["coords"] = co;
    }

    return j;
}

void GameEntity::deserializeRelations(const nlohmann::json& j) {
    if(j.contains("location")) {
        auto loc = j["location"].get<std::string>();
        auto locUnit = resolveUID(loc);
        if(locUnit) {
            location = locUnit;
            location->contents.push_back(this);
            if(j.contains("locationType")) locationType = j["locationType"];
            if(j.contains("coords")) coords.deserialize(j["coords"]);
        }

    }
}

void GameEntity::addToLocation(GameEntity *u, int locationType, std::optional<coordinates> coords) {
    if(location) {
        basic_mud_log("Attempted to add unit '%d: %s' to location, but location was already found.", uid, getName().c_str());
        return;
    }
    location = u;
    this->locationType = locationType;
    if(coords) this->coords = *coords;
    u->contents.push_back(this);
    u->handleAdd(this);
}

void GameEntity::removeFromLocation() {
    if(!location) {
        basic_mud_log("Attempted to remove unit '%d: %s' from location, but location was not found.", uid, getName().c_str());
        locationType = -1;
        coords.clear();
        return;
    }
    
    std::erase_if(location->contents, [&](auto ud) {return ud == this;});
    location->handleRemove(this);

    location = nullptr;
    locationType = -1;
    coords.clear();
    
}

void GameEntity::handleAdd(GameEntity *u) {
    
}


void GameEntity::handleRemove(GameEntity *u) {

    // remove uid from loc->contents
    
}

std::vector<GameEntity*> GameEntity::getNeighbors(bool visible) {
    auto loc = getLocation();
    if(!loc) return {};
    return loc->getNeighbors(visible);
}

std::vector<GameEntity*> GameEntity::getNeighborsFor(GameEntity *u, bool visible) {
    std::vector<GameEntity*> out;
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


std::vector<GameEntity*> Searcher::search() {
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

GameEntity* Searcher::getOne() {
    auto results = search();
    if(results.size() == 1) return results.front();
    return nullptr;
}


bool GameEntity::isEnvironment() {
    return false;
}


bool GameEntity::isRegion() {
    return false;
}

bool GameEntity::isStructure() {
    return false;
}

bool GameEntity::isPlanet() {
    return false;
}

GameEntity* GameEntity::getEnvironment() {
    auto loc = getLocation();
    while(loc) {
        if(loc->isEnvironment()) return loc;
        loc = loc->getLocation();
        if(loc == this) return nullptr;
    }
    return nullptr;
}

GameEntity* GameEntity::getRegion() {
    auto loc = getLocation();
    while(loc) {
        if(loc->isRegion()) return loc;
        loc = loc->getLocation();
        if(loc == this) return nullptr;
    }
    return nullptr;
}

GameEntity* GameEntity::getStructure() {
    auto loc = getLocation();
    while(loc) {
        if(loc->isStructure()) return loc;
        loc = loc->getLocation();
        if(loc == this) return nullptr;
    }
    return nullptr;
}

GameEntity* GameEntity::getPlanet() {
    auto loc = getLocation();
    while(loc) {
        if(loc->isPlanet()) return loc;
        loc = loc->getLocation();
        if(loc == this) return nullptr;
    }
    return nullptr;

}

double GameEntity::myEnvVar(EnvVar v) {
    if(auto env = getEnvironment(); env) return env->getEnvVar(v);
    return 0.0;
}


std::optional<double> GameEntity::emitEnvVar(EnvVar v) {
    if(auto found = envVars.find(v); found != envVars.end()) {
        return found->second;
    }
    return {};
}

double GameEntity::getEnvVar(EnvVar v) {
    return 0.0;
}

void GameEntity::onHolderExtraction() {
    extractFromWorld();
}

void GameEntity::extractFromWorld() {
    if(!exists) return; // prevent recursion.

    exists = false;
    pendingDeletions.insert(this);
    if(script) script->deactivate();

    for(auto c : contents) {
        c->onHolderExtraction();
    }

}

void GameEntity::executeCommand(const std::string& cmd) {
    // does nothing by default... 
}

bool GameEntity::isInsideNormallyDark() {
    return false;
}

bool GameEntity::isInsideDark() {
    return false;
}

bool GameEntity::isProvidingLight() {
    return false;
}

bool GameEntity::isInvisible() {
    return false;
}

bool GameEntity::isAdminInvisible() {
    return false;
}

bool GameEntity::canSeeInvisible() {
    return false;
}

bool GameEntity::canSeeInDark() {
    return false;
}

bool GameEntity::isHidden() {
    return false;
}

bool GameEntity::canSeeHidden() {
    return false;
}

bool GameEntity::canSeeAdminInvisible() {
    return false;
}

bool GameEntity::canSee(GameEntity *u) {
    if(canSeeAdminInvisible()) return true;
    if(u->isInvisible() && !canSeeInvisible()) return false;
    if(u->isHidden() && !canSeeHidden()) return false;
    return true;
}

void GameEntity::sendEvent(const Event& event) {
    // does nothing...
}

void GameEntity::sendEventContents(const Event& event) {
    for(auto c : contents) {
        c->sendEvent(event);
    }
}

void GameEntity::sendText(const std::string& text) {
    // does nothing by default.
}

void GameEntity::sendLine(const std::string& text) {
    if(text.ends_with("\r\n")) sendText(text);
    else sendText(text + "\r\n");
}

void GameEntity::sendTextContents(const std::string& text) {
    for(auto c : contents) {
        c->sendText(text);
    }
}

void GameEntity::sendLineContents(const std::string& text) {
    for(auto c : contents) {
        c->sendLine(text);
    }
}

std::optional<std::string> GameEntity::checkAllowInventoryAcesss(GameEntity *u) {
    return "you can't access it!";
}

std::optional<std::string> GameEntity::checkIsGettable(GameEntity *u) {
    return "it cannot be picked up!";
}

std::optional<std::string> GameEntity::checkIsDroppable(GameEntity *u) {
    return "it cannot be dropped!";
}

std::optional<std::string> GameEntity::checkIsGivable(GameEntity *u) {
    return "it cannot be given!";
}

std::optional<std::string> GameEntity::checkCanStore(GameEntity *u) {
    return "you can't store it!";
}

std::optional<std::string> GameEntity::checkAllowReceive(GameEntity *giver, GameEntity *u) {
    return "you can't give it!";
}

std::optional<std::string> GameEntity::checkAllowEquip(GameEntity *u, int location) {
    return "you can't equip it!";
}

std::optional<std::string> GameEntity::checkAllowRemove(GameEntity *u) {
    return "you can't remove it!";
}
