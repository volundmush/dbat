#include "dbat/structs.h"
#include "dbat/send.h"

struct room_data* thing_data::getRoom() const {
    return dynamic_cast<room_data*>(location.unit);
}

room_vnum thing_data::getRoomVnum() const {
    auto r = getRoom();
    return r ? r->getVnum() : NOWHERE;
}

std::string thing_data::getLocationName() const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getName(location.position);
    }
    return "Unknown";
}

std::optional<Destination> thing_data::getLocationExit(Direction dir) const {
    if(auto l = dynamic_cast<location_data*>(location.unit))
        return l->getDirection(location.position, dir);
    return std::nullopt;
}

std::map<Direction, Destination> thing_data::getLocationExits() const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getDirections(location.position);
    }
    return {};
}

double thing_data::getLocationEnvironment(int type) const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getEnvironment(location.position, type);
    }
    return 0.0;
}

double thing_data::setLocationEnvironment(int type, double value) {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->setEnvironment(location.position, type, value);
    }
    return 0.0;
}

double thing_data::modLocationEnvironment(int type, double value) {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->modEnvironment(location.position, type, value);
    }
    return 0.0;
}

void thing_data::clearLocationEnvironment(int type) {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        l->clearEnvironment(location.position, type);
    }
}

void thing_data::setRoomFlag(int flag, bool value) const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        l->setRoomFlag(location.position, flag, value);
    }
}

bool thing_data::toggleRoomFlag(int flag) const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->toggleRoomFlag(location.position, flag);
    }
    return false;
}

bool thing_data::getRoomFlag(int flag) const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getRoomFlag(location.position, flag);
    }
    return false;
}

void thing_data::setWhereFlag(WhereFlag flag, bool value) const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        l->setWhereFlag(location.position, flag, value);
    }
}

bool thing_data::toggleWhereFlag(WhereFlag flag) const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->toggleWhereFlag(location.position, flag);
    }
    return false;
}

bool thing_data::getWhereFlag(WhereFlag flag) const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getWhereFlag(location.position, flag);
    }
    return false;
}

void thing_data::broadcastAtLocation(const std::string& message) const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        l->broadcastAt(location.position, message);
    }
}

std::vector<std::weak_ptr<obj_data>> thing_data::getLocationObjects() const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getObjects(location.position);
    }
    return {};
}

std::vector<std::weak_ptr<char_data>> thing_data::getLocationPeople() const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getPeople(location.position);
    }
    return {};
}

int thing_data::getLocationDamage() const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getDamage(location.position);
    }
    return 0;
}

void thing_data::setLocationDamage(int value) {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        l->setDamage(location.position, value);
    }
}

int thing_data::modLocationDamage(int value) {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->modDamage(location.position, value);
    }
    return 0;
}

SectorType thing_data::getLocationSectorType() const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getSectorType(location.position);
    }
    return SectorType::inside;
}

int thing_data::getLocationTileType() const {
    return static_cast<int>(getLocationSectorType());
}

int thing_data::getLocationGroundEffect() const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->getGroundEffect(location.position);
    }
    return 0;
}

void thing_data::setLocationGroundEffect(int value) {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        l->setGroundEffect(location.position, value);
    }
}

int thing_data::modLocationGroundEffect(int value) {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return l->modGroundEffect(location.position, value);
    }
    return 0;
}

SpecialFunc thing_data::getLocationSpecialFunc() const {
    if(auto r = getRoom())
        return r->func;
    return nullptr;
}

Location thing_data::getLocation() const {
    return location;
}

int thing_data::getCookElement() const {
    if(auto r = getRoom())
        return cook_element(r);
    return 0;
}

bool thing_data::getLocationIsDark() const {
    if(auto r = getRoom())
        return r->isDark();
    return false;
}

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

// the bool operator for Location....
Location::operator bool() const {
    return getType() == UnitType::room;
}

UnitType Location::getType() const {
    return unit ? unit->type : UnitType::unknown;
}

zone_data* Location::getZone() const {
    if(auto r = dynamic_cast<room_data*>(unit)) {
        return r->zone;
    }
    return nullptr;
}