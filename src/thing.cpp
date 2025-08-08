#include "dbat/structs.h"
#include "dbat/send.h"

struct room_data* thing_data::getRoom() const {
    return dynamic_cast<room_data*>(location);
}

room_vnum thing_data::getRoomVnum() const {
    auto r = getRoom();
    return r ? r->getVnum() : NOWHERE;
}

std::string thing_data::getLocationName() const {
    if(auto r = getRoom())
        return r->getName();
    return "Unknown";
}

room_direction_data* thing_data::getLocationExit(int dir) const {
    if(auto r = getRoom())
        return r->dir_option[dir];
    return nullptr;
}

std::map<int, room_direction_data*> thing_data::getLocationExits() const {
    std::map<int, room_direction_data*> exits;
    if(auto r = getRoom())
        for(int i = 0; i < NUM_OF_DIRS; i++)
            if(r->dir_option[i])
                exits[i] = r->dir_option[i];
    return exits;
}

double thing_data::getLocationEnvironment(int type) const {
    if(auto r = getRoom())
        return r->getEnvironment(type);
    return 0.0;
}

double thing_data::setLocationEnvironment(int type, double value) const {
    if(auto r = getRoom())
        return r->setEnvironment(type, value);
    return 0.0;
}

double thing_data::modLocationEnvironment(int type, double value) const {
    if(auto r = getRoom())
        return r->modEnvironment(type, value);
    return 0.0;
}

void thing_data::clearLocationEnvironment(int type) const {
    if(auto r = getRoom())
        r->clearEnvironment(type);
}

void thing_data::setRoomFlag(int flag, bool value) const {
    if(auto r = getRoom())
        r->room_flags.set(flag, value);
}

bool thing_data::toggleRoomFlag(int flag) const {
    if(auto r = getRoom())
        return r->room_flags.toggle(flag);
    return false;
}

bool thing_data::getRoomFlag(int flag) const {
    if(auto r = getRoom())
        return r->room_flags.get(flag);
    return false;
}

void thing_data::setWhereFlag(WhereFlag flag, bool value) const {
    if(auto r = getRoom())
        r->where_flags.set(flag, value);
}

bool thing_data::toggleWhereFlag(WhereFlag flag) const {
    if(auto r = getRoom())
        return r->where_flags.toggle(flag);
    return false;
}

bool thing_data::getWhereFlag(WhereFlag flag) const {
    if(auto r = getRoom())
        return r->where_flags.get(flag);
    return false;
}

void thing_data::broadcastAtLocation(const std::string& message) const {
    if(auto r = getRoom())
        send_to_room(r, message);
}

std::vector<std::weak_ptr<obj_data>> thing_data::getLocationObjects() const {
    if(auto r = getRoom())
        return r->getObjects();
    return {};
}

std::vector<std::weak_ptr<char_data>> thing_data::getLocationPeople() const {
    if(auto r = getRoom())
        return r->getPeople();
    return {};
}

int thing_data::getLocationDamage() const {
    if(auto r = getRoom())
        return r->damage;
    return 0;
}

int thing_data::setLocationDamage(int value) const {
    if(auto r = getRoom())
        return r->damage = value;
    return 0;
}

int thing_data::modLocationDamage(int value) const {
    if(auto r = getRoom())
        return r->damage += value;
    return 0;
}

int thing_data::getLocationTileType() const {
    if(auto r = getRoom())
        return static_cast<int>(r->sector_type);
    return 0;
}

int thing_data::getLocationGroundEffect() const {
    if(auto r = getRoom())
        return r->ground_effect;
    return 0;
}

int thing_data::setLocationGroundEffect(int value) const {
    if(auto r = getRoom())
        return r->ground_effect = value;
    return 0;
}

int thing_data::modLocationGroundEffect(int value) const {
    if(auto r = getRoom())
        return r->ground_effect += value;
    return 0;
}

SpecialFunc thing_data::getLocationSpecialFunc() const {
    if(auto r = getRoom())
        return r->func;
    return nullptr;
}
