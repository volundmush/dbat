#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"
#include "dbat/constants.h"
#include "dbat/entity.h"

bool Coordinates::operator==(const Coordinates& rhs) const {
    return x == rhs.x && y == rhs.y && z == rhs.z;
}

bool Destination::operator==(const Destination& rhs) {
    return target == rhs.target && locationType == rhs.locationType && coords == rhs.coords;
}

bool Location::operator==(const Location& rhs) {
    return location == rhs.location && locationType == rhs.locationType && coords == rhs.coords;
}

Coordinates::Coordinates(const nlohmann::json& j) {
    deserialize(j);
}

void Coordinates::deserialize(const nlohmann::json& j) {
    if(j.contains("x")) x = j["x"];
    if(j.contains("y")) y = j["y"];
    if(j.contains("z")) z = j["z"];
}

nlohmann::json Coordinates::serialize() {
    nlohmann::json j;
    if(x != 0) j["x"] = x;
    if(y != 0) j["y"] = y;
    if(z != 0) j["z"] = z;
    return j;
}

void Coordinates::clear() {
    x = 0;
    y = 0;
    z = 0;
}


std::string GameEntity::renderRoomListingFor(GameEntity *viewer) {
    std::vector<std::string> results;
    if(auto pref = renderListPrefixFor(viewer); !pref.empty()) results.push_back(pref);
    if(auto helper = renderRoomListingHelper(viewer); !helper.empty()) results.push_back(helper);
    if(auto modif = renderModifiers(viewer); !modif.empty()) results.emplace_back(modif);

    return join(results, "@n ");
}

std::vector<GameEntity*> GameEntity::getContents() {
    return contents::getContents(ent);
}

std::vector<Room*> GameEntity::getRooms() {
    return contents::getRooms(ent);
}

std::vector<Character*> GameEntity::getPeople() {
    return contents::getPeople(ent);
}

std::vector<Object*> GameEntity::getInventory() {
    return contents::getInventory(ent);
}

std::map<int, Object*> GameEntity::getEquipment() {
    return contents::getEquipment(ent);
}

std::map<int, Exit*> GameEntity::getExits() {
    return contents::getExits(ent);
}

std::map<int, Exit*> GameEntity::getUsableExits() {
    std::map<int, Exit*> out;
    for(auto &[door, o] : getExits()) {
        if(o->checkFlag(FlagType::Exit, EX_CLOSED)) continue;
        auto dest = reg.try_get<Destination>(o->ent);
        if(!dest) continue;
        if(flags::check(dest->target, FlagType::Room, ROOM_DEATH)) continue;
        out[door] = o;
    }
    return out;
}

GameEntity* GameEntity::getLocation() {
    if(auto loc = reg.try_get<Location>(ent); loc) {
        auto &info = reg.get<Info>(loc->location);
        switch(info.family) {
            case EntityFamily::Character:
                return reg.try_get<Character>(loc->location);
            case EntityFamily::Object:
                return reg.try_get<Object>(loc->location);
            case EntityFamily::Room:
                return reg.try_get<Room>(loc->location);
            case EntityFamily::Exit:
                return reg.try_get<Exit>(loc->location);
        }
    }
    return nullptr;
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

    if(uid != NOTHING) j["uid"] = uid;
    if(zone != NOTHING) j["zone"] = zone;

    if(script && (!script->dgScripts.empty() || !script->vars.empty())) {
        j["dgScripts"] = script->serialize();
    }


    for(auto &[v, d] : envVars) {
        j["envVars"].push_back(std::make_pair(v, d));
    }

    return j;
}

GameEntity::~GameEntity() {
    if(ent != entt::null) reg.destroy(ent);
}

void GameEntity::deserialize(const nlohmann::json& j) {
    if(ent == entt::null) {
        ent = reg.create();
    }
    if(j.contains("vn")) vn = j["vn"];

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


std::string GameEntity::scriptString() {
    return proto_script.empty() ? "" : fmt::format("@D[@wT{}@D]@n", fmt::join(proto_script, ","));
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
    return find::object(ent, func, working);
}

Object* GameEntity::findObjectVnum(obj_vnum objVnum, bool working) {
    return findObject([objVnum](auto o) {return o->getVN() == objVnum;}, working);
}

std::set<Object*> GameEntity::gatherObjects(const std::function<bool(Object*)> &func, bool working) {
    return find::gatherObjects(ent, func, working);
}

std::string GameEntity::getUIDString(bool active) {
    return fmt::format("#{}{}", uid, active ? "" : "!");
}

DgResults GameEntity::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    return "";
}

std::string GameEntity::getName() {
    return text::get(ent, "name");
}

void GameEntity::setName(const std::string& n) {
    text::set(ent, "name", n);
}

std::string GameEntity::getAlias() {
    return text::get(ent, "alias");
}

void GameEntity::setAlias(const std::string& n) {
    text::set(ent, "alias", n);
}

std::string GameEntity::getShortDesc() {
    return text::get(ent, "short_description");
}

void GameEntity::setShortDesc(const std::string& n) {
    text::set(ent, "short_description", n);
}

std::string GameEntity::getRoomDesc() {
    return text::get(ent, "room_description");
}

void GameEntity::setRoomDesc(const std::string& n) {
    text::set(ent, "room_description", n);
}

std::string GameEntity::getLookDesc() {
    return text::get(ent, "look_description");
}

void GameEntity::setLookDesc(const std::string& n) {
    text::set(ent, "look_description", n);
}


void GameEntity::checkMyID() {
    if(uid == -1) {
        uid = getNextUID();
        basic_mud_log("Unit Found with ID -1. Automatically fixed to ID %d", uid);
    }
}

std::string GameEntity::getDisplayName(GameEntity* ch) {
    return getName();
}

std::string GameEntity::renderAppearance(GameEntity* ch) {
    return "You see nothing special.";
}

std::string GameEntity::renderLocationFor(GameEntity* viewer) {
    return "";
}

std::string GameEntity::renderListingFor(GameEntity* viewer) {
    return "";
}

std::string GameEntity::renderInventoryListingFor(GameEntity* viewer) {
    return "";
}

std::string GameEntity::renderInventoryListingHelper(GameEntity* viewer) {
    return "";
}

std::string GameEntity::renderModifiers(GameEntity* viewer) {
    return "";
}

std::string GameEntity::renderDiagnostics(GameEntity* viewer) {
    return "";
}

std::map<int, Destination> GameEntity::getDestinations(GameEntity* viewer) {
    return {};
}



std::string GameEntity::renderInventory(GameEntity* viewer) {
    std::vector<std::string> lines;

    for(auto obj : getInventory()) {
        if(viewer->canSee(obj)) lines.push_back(obj->renderListingFor(viewer));
    }

    return join(lines, "@n\r\n");
}

std::string GameEntity::renderRoomListingHelper(GameEntity* viewer) {
    return getRoomDesc();
}

std::vector<std::string> GameEntity::getKeywords(GameEntity* ch) {
    return {};
}


void GameEntity::assignTriggers() {
    // does nothing.
}

std::vector<std::string> GameEntity::getFlagNames(FlagType type) {
    return flags::getNames(ent, type);
}

bool GameEntity::checkFlag(FlagType type, int flag) {
    return flags::check(ent, type, flag);
}

void GameEntity::setFlag(FlagType type, int flag, bool value) {
    flags::set(ent, type, flag, value);
}

void GameEntity::clearFlag(FlagType type, int flag) {
    setFlag(type, flag, false);
}

bool GameEntity::flipFlag(FlagType type, int flag) {
    return flags::flip(ent, type, flag);
}

void GameEntity::addToLocation(const Destination &dest) {
    contents::addTo(ent, dest);
}

void GameEntity::addToLocation(GameEntity *u) {
    contents::addTo(ent, Destination(u));
}

void GameEntity::removeFromLocation() {
    contents::removeFrom(ent);
}


std::vector<GameEntity*> GameEntity::getNeighbors(bool visible) {
    auto loc = getLocation();
    if(!loc) return {};
    return loc->getNeighbors(visible);
}

std::vector<GameEntity*> GameEntity::getNeighborsFor(GameEntity *u, bool visible) {
    std::vector<GameEntity*> out;
    for(auto n : getContents()) {
        if(u == n) continue;
        if(visible && !u->canSee(n)) continue;
        out.push_back(n);
    }

    return out;
}

double GameEntity::myEnvVar(EnvVar v) {
    if(auto env = find::holderType(ent, ITEM_ENVIRONMENT); env != entt::null) return ::getEnvVar(env, v);
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

    for(auto c : getContents()) {
        c->onHolderExtraction();
    }

}

void GameEntity::executeCommand(const std::string& cmd) {
    // does nothing by default... 
}

bool GameEntity::isInsideNormallyDark(GameEntity* viewer) {
    return false;
}

bool GameEntity::isInsideDark(GameEntity* viewer) {
    return false;
}

bool GameEntity::isProvidingLight() {
    return vis::isProvidingLight(ent);
}

bool GameEntity::isInvisible() {
    return vis::isInvisible(ent);
}

bool GameEntity::isAdminInvisible() {
    return vis::isAdminInvisible(ent);
}

bool GameEntity::canSeeInvisible() {
    return vis::canSeeInvisible(ent);
}

bool GameEntity::canSeeInDark() {
    return vis::canSeeInDark(ent);
}

bool GameEntity::isHidden() {
    return vis::isHidden(ent);
}

bool GameEntity::canSeeHidden() {
    return vis::canSeeHidden(ent);
}

bool GameEntity::canSeeAdminInvisible() {
    return vis::canSeeAdminInvisible(ent);
}

bool GameEntity::canSee(GameEntity *u) {
    return vis::canSee(ent, u->ent);
}

void GameEntity::sendEvent(const Event& event) {
    // does nothing...
}

void GameEntity::sendEventContents(const Event& event) {
    for(auto c : getContents()) {
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
    for(auto c : getContents()) {
        c->sendText(text);
    }
}

void GameEntity::sendLineContents(const std::string& text) {
    for(auto c : getContents()) {
        c->sendLine(text);
    }
}

std::optional<std::string> GameEntity::checkAllowInventoryAcesss(GameEntity *u) {
    return "you can't access it!";
}

std::optional<std::string> GameEntity::checkIsGettable(GameEntity *u) {
    return "it cannot be picked up!";
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

int64_t GameEntity::getUID() {
    return ::getUID(ent);
}

vnum GameEntity::getVN() {
    return vn;
}

zone_vnum GameEntity::getZone() {
    return zone;
}

void GameEntity::lookAtLocation() {

}

std::optional<Destination> GameEntity::getDestination(GameEntity* viewer, int direction) {
    auto destinations = getDestinations(viewer);
    if(auto found = destinations.find(direction); found != destinations.end()) {
        return found->second;
    }
    return {};
}

std::string GameEntity::renderListPrefixFor(GameEntity *viewer) {
    return render::listPrefix(ent, viewer->ent);
}

std::vector<std::pair<std::string, Destination>> GameEntity::getLandingSpotsFor(GameEntity *mover) {
    return {};
}

std::optional<Destination> GameEntity::getLaunchDestinationFor(GameEntity *mover) {
    return {};
}

bool GameEntity::moveInDirection(int direction, bool need_specials_check) {
    return false;
}

bool GameEntity::doSimpleMove(int direction, bool need_specials_check) {
    return false;
}

bool GameEntity::checkCanLeave(GameEntity *mover, const Destination& dest, bool need_specials_check) {
    return false;
}

bool GameEntity::checkCanReachDestination(GameEntity *mover, const Destination& dest) {
    return false;
}

bool GameEntity::checkPostEnter(GameEntity *mover, const Location& cameFrom, const Destination& dest) {
    return false;
}
