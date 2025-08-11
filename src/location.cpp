#include <functional>
#include "dbat/structs.h"
#include "dbat/filter.h"
#include "dbat/utils.h"
#include "dbat/constants.h"

bool Coordinates::operator==(const Coordinates& other) const {
    return x == other.x && y == other.y && z == other.z;
}

void Coordinates::apply(Direction dir) {
    switch(dir) {
        case Direction::north:    y += 1; break;
        case Direction::east:     x += 1; break;
        case Direction::south:    y -= 1; break;
        case Direction::west:     x -= 1; break;
        case Direction::up:       z += 1; break;
        case Direction::down:     z -= 1; break;
        case Direction::northeast: x += 1; y += 1; break;
        case Direction::southeast: x += 1; y -= 1; break;
        case Direction::southwest: x -= 1; y -= 1; break;
        case Direction::northwest: x -= 1; y += 1; break;
        case Direction::inside:
        case Direction::outside:
            // No movement for inside/outside
            break;
    }
}

bool Location::operator==(const Location& other) const {
    return unit == other.unit && position == other.position;
}

bool Location::operator==(const Room* room) const {
    return room == unit;
}

bool Location::operator==(const room_vnum rv) const {
    if(!unit) return rv == NOWHERE;
    if(unit->type != UnitType::room) return false;
    auto r = static_cast<Room*>(unit);
    return r->getVnum() == rv;
}

// the bool operator for ...
Location::operator bool() const {
    return getType() == UnitType::room;
}

UnitType Location::getType() const {
    return unit ? unit->type : UnitType::unknown;
}

AbstractLocation* Location::getLoc() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
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

Zone* Location::getZone() const {
    if(auto r = dynamic_cast<AbstractLocation*>(unit)) {
        return r->getZone();
    }
    return nullptr;
}

const char* Location::getName() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getName(position);
    }
    return "Unknown";
}

const char* Location::getLookDescription() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getLookDescription(position);
    }
    return "";
}

std::optional<Destination> Location::getExit(Direction dir) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit))
        return l->getDirection(position, dir);
    return std::nullopt;
}

std::map<Direction, Destination> Location::getExits() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getDirections(position);
    }
    return {};
}

const std::vector<ExtraDescription>& Location::getExtraDescription() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getExtraDescription(position);
    }
    static std::vector<ExtraDescription> empty;
    return empty;
}

double Location::getEnvironment(int type) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getEnvironment(position, type);
    }
    return 0.0;
}

double Location::setEnvironment(int type, double value) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->setEnvironment(position, type, value);
    }
    return 0.0;
}

double Location::modEnvironment(int type, double value) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->modEnvironment(position, type, value);
    }
    return 0.0;
}

void Location::clearEnvironment(int type) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        l->clearEnvironment(position, type);
    }
}

void Location::setRoomFlag(int flag, bool value) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        l->setRoomFlag(position, flag, value);
    }
}

void Location::setRoomFlag(RoomFlag flag, bool value) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        l->setRoomFlag(position, flag, value);
    }
}

bool Location::toggleRoomFlag(int flag) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::toggleRoomFlag(RoomFlag flag) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(int flag) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(RoomFlag flag) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getRoomFlag(position, flag);
    }
    return false;
}

void Location::setWhereFlag(WhereFlag flag, bool value) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        l->setWhereFlag(position, flag, value);
    }
}

bool Location::toggleWhereFlag(WhereFlag flag) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->toggleWhereFlag(position, flag);
    }
    return false;
}

bool Location::getWhereFlag(WhereFlag flag) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getWhereFlag(position, flag);
    }
    return false;
}

std::string Location::getUID(bool active) const {
    if(unit) return unit->getUID(active);
    return "";
}

void Location::sendText(const std::string& message) const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        l->broadcastAt(position, message);
    }
}

std::vector<std::weak_ptr<Object>> Location::getObjects() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getObjects(position);
    }
    return {};
}

std::vector<std::weak_ptr<Character>> Location::getPeople() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getPeople(position);
    }
    return {};
}

int Location::getDamage() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getDamage(position);
    }
    return 0;
}

void Location::setDamage(int value) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        l->setDamage(position, value);
    }
}

int Location::modDamage(int value) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->modDamage(position, value);
    }
    return 0;
}

SectorType Location::getSectorType() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getSectorType(position);
    }
    return SectorType::inside;
}

int Location::getTileType() const {
    return static_cast<int>(getSectorType());
}

int Location::getGroundEffect() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getGroundEffect(position);
    }
    return 0;
}

void Location::setGroundEffect(int value) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        l->setGroundEffect(position, value);
    }
}

int Location::modGroundEffect(int value) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->modGroundEffect(position, value);
    }
    return 0;
}

SpecialFunc Location::getSpecialFunc() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit))
        return l->getSpecialFunc(position);
    return nullptr;
}

bool Location::getIsDark() const {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
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
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->getCookElement(position);
    }
    return 0;
}

// Hash function implementations
namespace std {
    std::size_t hash<Coordinates>::operator()(const Coordinates& coord) const noexcept {
        // Combine hash of x, y, and z coordinates
        std::size_t h1 = std::hash<int32_t>{}(coord.x);
        std::size_t h2 = std::hash<int32_t>{}(coord.y);
        std::size_t h3 = std::hash<int32_t>{}(coord.z);

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

Object* AbstractLocation::findObject(const Coordinates& coor, const std::function<bool(Object*)> &func, bool working) {
    auto objs = getObjects(coor);
    for (const auto& obj : filter_raw(objs)) {
        if(working && !obj->isWorking()) continue;
        if (func(obj)) {
            return obj;
        }
    }
    return nullptr;
}

Object* AbstractLocation::findObjectVnum(const Coordinates& coor, obj_vnum objVnum, bool working) {
    return findObject(coor, [objVnum, working](Object* obj) {
        return obj->getVnum() == objVnum && (!working || obj->isWorking());
    });
}

std::unordered_set<Object*> AbstractLocation::gatherObjects(const Coordinates& coor, const std::function<bool(Object*)> &func, bool working) {
    std::unordered_set<Object*> result;
    auto objs = getObjects(coor);
    for (const auto& obj : filter_raw(objs)) {
        if(working && !obj->isWorking()) continue;
        if (func(obj)) {
            result.insert(obj);
        }
    }
    return result;
}

Object* Location::findObject(const std::function<bool(Object*)> &func, bool working) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->findObject(position, func, working);
    }
    return nullptr;
}

Object* Location::findObjectVnum(obj_vnum objVnum, bool working) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->findObjectVnum(position, objVnum, working);
    }
    return nullptr;
}

std::unordered_set<Object*> Location::gatherObjects(const std::function<bool(Object*)> &func, bool working) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        return l->gatherObjects(position, func, working);
    }
    return {};
}

std::optional<Destination> Destination::getReverse() const {
    return getExit(static_cast<Direction>(rev_dir[static_cast<int>(dir)]));
}

void AbstractLocation::setRoomFlag(const Coordinates& coor, int flag, bool value) {
    setRoomFlag(coor, static_cast<RoomFlag>(flag), value);
}

bool AbstractLocation::toggleRoomFlag(const Coordinates& coor, int flag) {
    return toggleRoomFlag(coor, static_cast<RoomFlag>(flag));
}

bool AbstractLocation::getRoomFlag(const Coordinates& coor, int flag) const {
    return getRoomFlag(coor, static_cast<RoomFlag>(flag));
}

int AbstractLocation::getCookElement(const Coordinates& coor) const {
    int found = 0;
    auto con = getObjects(coor);
    for(auto obj : filter_raw(con)) {
        if(GET_OBJ_TYPE(obj) == ITEM_CAMPFIRE) {
            found = 1;
        } else if(obj->getVnum() == 19093) return 2;
    }

    return found;
}

const std::vector<ExtraDescription>& AbstractLocation::getExtraDescription(const Coordinates& coor) const {
    static std::vector<ExtraDescription> extraDescriptions;
    return extraDescriptions;
}

bool AbstractLocation::getIsDark(const Coordinates& coor) const {
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
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        l->deleteExit(position, dir);
    }
}

void Location::replaceExit(const Destination& dest) {
    if(auto l = dynamic_cast<AbstractLocation*>(unit)) {
        l->replaceExit(position, dest);
    }
}

void AbstractLocation::replaceExit(const Coordinates& coor, const Destination& dest) {
    // Implementation for replacing an exit in the location data
}

void AbstractLocation::deleteExit(const Coordinates& coor, Direction dir) {
    // Implementation for deleting an exit in the location data
}