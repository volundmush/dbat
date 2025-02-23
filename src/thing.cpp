#include "dbat/structs.h"
#include "dbat/utils.h"

struct room_data* thing_data::getRoom() const {
    return room;
}

room_vnum thing_data::getRoomVnum() const {
    return room ? room->vn : NOWHERE;
}

std::string thing_data::getLocationName() const {
    if(room)
        if(room->name && strlen(room->name) > 0)
            return room->name;
    return "Unknown";
}

room_direction_data* thing_data::getLocationExit(int dir) const {
    if(room)
        if(room->dir_option[dir])
            return room->dir_option[dir];
    return nullptr;
}

std::map<int, room_direction_data*> thing_data::getLocationExits() const {
    std::map<int, room_direction_data*> exits;
    if(room)
        for(int i = 0; i < NUM_OF_DIRS; i++)
            if(room->dir_option[i])
                exits[i] = room->dir_option[i];
    return exits;
}

double thing_data::getLocationEnvironment(int type) const {
    if(room)
        return room->getEnvironment(type);
    return 0.0;
}

double thing_data::setLocationEnvironment(int type, double value) const {
    if(room)
        return room->setEnvironment(type, value);
    return 0.0;
}

double thing_data::modLocationEnvironment(int type, double value) const {
    if(room)
        return room->modEnvironment(type, value);
    return 0.0;
}

void thing_data::clearLocationEnvironment(int type) const {
    if(room)
        room->clearEnvironment(type);
}

void thing_data::setRoomFlag(int flag, bool value) const {
    if(room)
        room->room_flags.set(flag, value);
}

bool thing_data::toggleRoomFlag(int flag) const {
    if(room)
        room->room_flags.flip(flag);
        return room->room_flags.test(flag);
    return false;
}

bool thing_data::getRoomFlag(int flag) const {
    if(room)
        return room->room_flags.test(flag);
    return false;
}

void thing_data::broadcastAtLocation(const std::string& message) const {
    if(room)
        send_to_room(room, message);
}

std::vector<std::weak_ptr<obj_data>> thing_data::getLocationObjects() const {
    if(room)
        return room->getObjects();
    return {};
}

std::vector<std::weak_ptr<char_data>> thing_data::getLocationPeople() const {
    if(room)
        return room->getPeople();
    return {};
}

int thing_data::getLocationDamage() const {
    if(room)
        return room->dmg;
    return 0;
}

int thing_data::setLocationDamage(int value) const {
    if(room)
        return room->dmg = value;
    return 0;
}

int thing_data::modLocationDamage(int value) const {
    if(room)
        return room->dmg += value;
    return 0;
}

int thing_data::getLocationTileType() const {
    if(room)
        return room->sector_type;
    return 0;
}

int thing_data::getLocationGroundEffect() const {
    if(room)
        return room->geffect;
    return 0;
}

int thing_data::setLocationGroundEffect(int value) const {
    if(room)
        return room->geffect = value;
    return 0;
}

int thing_data::modLocationGroundEffect(int value) const {
    if(room)
        return room->geffect += value;
    return 0;
}

SpecialFunc thing_data::getLocationSpecialFunc() const {
    if(room)
        return room->func;
    return nullptr;
}