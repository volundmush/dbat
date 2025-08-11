#include <functional>
#include "dbat/structs.h"
#include "dbat/filter.h"
#include "dbat/utils.h"
#include "dbat/constants.h"

bool Coordinates::operator==(const Coordinates& other) const {
    return x == other.x && y == other.y && z == other.z;
}

bool Location::operator==(const Location& other) const {
    return unit == other.unit && position == other.position;
}

bool Location::operator==(const room_data* room) const {
    return room == unit;
}

bool Location::operator==(const room_vnum rv) const {
    if(!unit) return rv == NOWHERE;
    if(unit->type != UnitType::room) return false;
    auto r = static_cast<room_data*>(unit);
    return r->getVnum() == rv;
}

// the bool operator for ...
Location::operator bool() const {
    return getType() == UnitType::room;
}

UnitType Location::getType() const {
    return unit ? unit->type : UnitType::unknown;
}

location_data* Location::getLoc() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l;
    }
    return nullptr;
}

vnum Location::getVnum() const {
    if(auto l = getLoc(); l) {
        return l->getVnum();
    }
    return NOWHERE;
}

zone_data* Location::getZone() const {
    if(auto r = dynamic_cast<location_data*>(unit)) {
        return r->zone;
    }
    return nullptr;
}

const char* Location::getName() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getName(position);
    }
    return "Unknown";
}

std::optional<Destination> Location::getExit(Direction dir) const {
    if(auto l = dynamic_cast<location_data*>(unit))
        return l->getDirection(position, dir);
    return std::nullopt;
}

std::map<Direction, Destination> Location::getExits() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getDirections(position);
    }
    return {};
}

const std::vector<ExtraDescription>& Location::getExtraDescription() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getExtraDescription(position);
    }
    static std::vector<ExtraDescription> empty;
    return empty;
}

double Location::getEnvironment(int type) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getEnvironment(position, type);
    }
    return 0.0;
}

double Location::setEnvironment(int type, double value) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->setEnvironment(position, type, value);
    }
    return 0.0;
}

double Location::modEnvironment(int type, double value) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->modEnvironment(position, type, value);
    }
    return 0.0;
}

void Location::clearEnvironment(int type) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        l->clearEnvironment(position, type);
    }
}

void Location::setRoomFlag(int flag, bool value) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        l->setRoomFlag(position, flag, value);
    }
}

void Location::setRoomFlag(RoomFlag flag, bool value) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        l->setRoomFlag(position, flag, value);
    }
}

bool Location::toggleRoomFlag(int flag) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::toggleRoomFlag(RoomFlag flag) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(int flag) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(RoomFlag flag) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getRoomFlag(position, flag);
    }
    return false;
}

void Location::setWhereFlag(WhereFlag flag, bool value) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        l->setWhereFlag(position, flag, value);
    }
}

bool Location::toggleWhereFlag(WhereFlag flag) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->toggleWhereFlag(position, flag);
    }
    return false;
}

bool Location::getWhereFlag(WhereFlag flag) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getWhereFlag(position, flag);
    }
    return false;
}

std::string Location::getUID(bool active) const {
    if(unit) return unit->getUID(active);
    return "";
}

void Location::sendText(const std::string& message) const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        l->broadcastAt(position, message);
    }
}

std::vector<std::weak_ptr<obj_data>> Location::getObjects() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getObjects(position);
    }
    return {};
}

std::vector<std::weak_ptr<char_data>> Location::getPeople() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getPeople(position);
    }
    return {};
}

int Location::getDamage() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getDamage(position);
    }
    return 0;
}

void Location::setDamage(int value) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        l->setDamage(position, value);
    }
}

int Location::modDamage(int value) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->modDamage(position, value);
    }
    return 0;
}

SectorType Location::getSectorType() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getSectorType(position);
    }
    return SectorType::inside;
}

int Location::getTileType() const {
    return static_cast<int>(getSectorType());
}

int Location::getGroundEffect() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getGroundEffect(position);
    }
    return 0;
}

void Location::setGroundEffect(int value) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        l->setGroundEffect(position, value);
    }
}

int Location::modGroundEffect(int value) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->modGroundEffect(position, value);
    }
    return 0;
}

SpecialFunc Location::getSpecialFunc() const {
    if(auto l = dynamic_cast<location_data*>(unit))
        return l->getSpecialFunc(position);
    return nullptr;
}

bool Location::getIsDark() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getIsDark(position);
    }
    return false;
}

int Location::countPlayers() const {
    int total = 0;
    auto people = getPeople();
    for (const auto& ch : filter_raw(people)) {
        if(!IS_NPC(ch)) total++;
    }
    return total;
}

bool Location::canGo(int dir) const {
    auto ex = getExit(static_cast<Direction>(dir));
    if(!ex) return false;
    return !IS_SET(ex->exit_info, EX_CLOSED);
}


int Location::getCookElement() const {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->getCookElement(position);
    }
    return 0;
}

// Hash function implementations
namespace std {
    std::size_t hash<Coordinates>::operator()(const Coordinates& coord) const noexcept {
        // Combine hash of x, y, and z coordinates
        std::size_t h1 = std::hash<double>{}(coord.x);
        std::size_t h2 = std::hash<double>{}(coord.y);
        std::size_t h3 = std::hash<double>{}(coord.z);
        
        // Use bit shifting and XOR to combine hashes
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }

    std::size_t hash<Location>::operator()(const Location& loc) const noexcept {
        // Combine hash of unit pointer and position
        std::size_t h1 = std::hash<void*>{}(loc.unit);
        std::size_t h2 = std::hash<Coordinates>{}(loc.position);
        
        // Use bit shifting and XOR to combine hashes
        return h1 ^ (h2 << 1);
    }
}

obj_data* location_data::findObject(const Coordinates& coor, const std::function<bool(obj_data*)> &func, bool working) {
    auto objs = getObjects(coor);
    for (const auto& obj : filter_raw(objs)) {
        if(working && !obj->isWorking()) continue;
        if (func(obj)) {
            return obj;
        }
    }
    return nullptr;
}

obj_data* location_data::findObjectVnum(const Coordinates& coor, obj_vnum objVnum, bool working) {
    return findObject(coor, [objVnum, working](obj_data* obj) {
        return obj->getVnum() == objVnum && (!working || obj->isWorking());
    });
}

std::unordered_set<obj_data*> location_data::gatherObjects(const Coordinates& coor, const std::function<bool(obj_data*)> &func, bool working) {
    std::unordered_set<obj_data*> result;
    auto objs = getObjects(coor);
    for (const auto& obj : filter_raw(objs)) {
        if(working && !obj->isWorking()) continue;
        if (func(obj)) {
            result.insert(obj);
        }
    }
    return result;
}

struct obj_data* Location::findObject(const std::function<bool(struct obj_data*)> &func, bool working) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->findObject(position, func, working);
    }
    return nullptr;
}

struct obj_data* Location::findObjectVnum(obj_vnum objVnum, bool working) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->findObjectVnum(position, objVnum, working);
    }
    return nullptr;
}

std::unordered_set<struct obj_data*> Location::gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        return l->gatherObjects(position, func, working);
    }
    return {};
}

std::optional<Destination> Destination::getReverse() const {
    return getExit(static_cast<Direction>(rev_dir[static_cast<int>(dir)]));
}

void location_data::setRoomFlag(const Coordinates& coor, int flag, bool value) {
    setRoomFlag(coor, static_cast<RoomFlag>(flag), value);
}

bool location_data::toggleRoomFlag(const Coordinates& coor, int flag) {
    return toggleRoomFlag(coor, static_cast<RoomFlag>(flag));
}

bool location_data::getRoomFlag(const Coordinates& coor, int flag) const {
    return getRoomFlag(coor, static_cast<RoomFlag>(flag));
}

int location_data::getCookElement(const Coordinates& coor) const {
    int found = 0;
    auto con = getObjects(coor);
    for(auto obj : filter_raw(con)) {
        if(GET_OBJ_TYPE(obj) == ITEM_CAMPFIRE) {
            found = 1;
        } else if(obj->getVnum() == 19093) return 2;
    }

    return found;
}

const std::vector<ExtraDescription>& location_data::getExtraDescription(const Coordinates& coor) const {
    static std::vector<ExtraDescription> extraDescriptions;
    return extraDescriptions;
}

bool location_data::getIsDark(const Coordinates& coor) const {
    return false; // temporarily disabled.

    auto pe = getPeople(coor);
    for(auto c : filter_raw(pe)) {
        if(c->isProvidingLight()) return false;
    }

    if (getCookElement(coor))
        return false;

    if (getRoomFlag(coor, ROOM_NOINSTANT) && getRoomFlag(coor, ROOM_DARK)) {
        return true;
    }
    if (getRoomFlag(coor, ROOM_NOINSTANT) && !getRoomFlag(coor, ROOM_DARK)) {
        return false;
    }

    if (getRoomFlag(coor, ROOM_DARK))
        return true;

    if (getRoomFlag(coor, ROOM_INDOORS))
        return false;

    const auto tile = static_cast<int>(getSectorType(coor));

    if (tile == SECT_INSIDE || tile == SECT_CITY || tile == SECT_IMPORTANT || tile == SECT_SHOP)
        return false;

    if (tile == SECT_SPACE)
        return false;

    if (weather_info.sunlight == SUN_SET)
        return true;

    if (weather_info.sunlight == SUN_DARK)
        return true;

    return false;
}

void Location::deleteExit(Direction dir) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        l->deleteExit(position, dir);
    }
}

void Location::replaceExit(const Destination& dest) {
    if(auto l = dynamic_cast<location_data*>(unit)) {
        l->replaceExit(position, dest);
    }
}

void location_data::replaceExit(const Coordinates& coor, const Destination& dest) {
    // Implementation for replacing an exit in the location data
}

void location_data::deleteExit(const Coordinates& coor, Direction dir) {
    // Implementation for deleting an exit in the location data
}