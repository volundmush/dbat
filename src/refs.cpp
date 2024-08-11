#include "dbat/structs.h"
#include "dbat/db.h"

ObjRef::ObjRef(int64_t id, time_t generation) : id{id}, generation{generation} {}

ObjRef::ObjRef(const nlohmann::json& j) {
    deserialize(j);
}

ObjRef::ObjRef(obj_data* obj) {
    id = obj->id;
    generation = obj->generation;
}

// Method implementations
int64_t ObjRef::getID() const {
    return id;
}

time_t ObjRef::getGeneration() const {
    return generation;
}

nlohmann::json ObjRef::serialize() const {
    return nlohmann::json{{"id", id}, {"generation", generation}};
}

void ObjRef::deserialize(const nlohmann::json& j) {
    id = j.at("id").get<int64_t>();
    generation = j.at("generation").get<time_t>();
}

obj_data* ObjRef::get(bool checkActive) const {
    auto find = uniqueObjects.find(id);
    if (find == uniqueObjects.end()) return nullptr;
    if (find->second.first != generation) return nullptr;
    if (checkActive && !find->second.second->isActive()) return nullptr;
    return find->second.second;
}

// Operator implementations
bool ObjRef::operator==(const ObjRef& other) const {
    return id == other.id && generation == other.generation;
}

bool ObjRef::operator!=(const ObjRef& other) const {
    return !(*this == other);
}

obj_data* ObjRef::operator->() {
    return get();
}

ObjRef::operator obj_data*() {
    return get();
}

ObjRef& ObjRef::operator=(const obj_data* obj) {
    if (obj) {
        id = obj->id;
        generation = obj->generation;
    } else {
        id = NOTHING;
        generation = 0;
    }
    return *this;
}

ObjRef& ObjRef::operator=(const obj_data& obj) {
    id = obj.id;
    generation = obj.generation;
    return *this;
}


// CHARACTERS
CharRef::CharRef(int64_t id, time_t generation) : id{id}, generation{generation} {}

CharRef::CharRef(const nlohmann::json& j) {
    deserialize(j);
}

CharRef::CharRef(char_data* obj) {
    id = obj->id;
    generation = obj->generation;
}

// Method implementations
int64_t CharRef::getID() const {
    return id;
}

time_t CharRef::getGeneration() const {
    return generation;
}

nlohmann::json CharRef::serialize() const {
    return nlohmann::json{{"id", id}, {"generation", generation}};
}

void CharRef::deserialize(const nlohmann::json& j) {
    id = j.at("id").get<int64_t>();
    generation = j.at("generation").get<time_t>();
}

char_data* CharRef::get(bool checkActive) const {
    auto find = uniqueCharacters.find(id);
    if (find == uniqueCharacters.end()) return nullptr;
    if (find->second.first != generation) return nullptr;
    if (checkActive && !find->second.second->isActive()) return nullptr;
    return find->second.second;
}

// Operator implementations
bool CharRef::operator==(const CharRef& other) const {
    return id == other.id && generation == other.generation;
}

bool CharRef::operator!=(const CharRef& other) const {
    return !(*this == other);
}

char_data* CharRef::operator->() {
    return get();
}

CharRef::operator char_data*() {
    return get();
}

CharRef& CharRef::operator=(const char_data* obj) {
    if (obj) {
        id = obj->id;
        generation = obj->generation;
    } else {
        id = NOTHING;
        generation = 0;
    }
    return *this;
}

CharRef& CharRef::operator=(const char_data& obj) {
    id = obj.id;
    generation = obj.generation;
    return *this;
}

// ROOMS
RoomRef::RoomRef(int64_t id, time_t generation) : id{id}, generation{generation} {}

RoomRef::RoomRef(const nlohmann::json& j) {
    deserialize(j);
}

RoomRef::RoomRef(room_data* obj) {
    id = obj->id;
    generation = obj->generation;
}

// Method implementations
int64_t RoomRef::getID() const {
    return id;
}

time_t RoomRef::getGeneration() const {
    return generation;
}

nlohmann::json RoomRef::serialize() const {
    return nlohmann::json{{"id", id}, {"generation", generation}};
}

void RoomRef::deserialize(const nlohmann::json& j) {
    id = j.at("id").get<int64_t>();
    generation = j.at("generation").get<time_t>();
}

room_data* RoomRef::get(bool checkActive) const {
    auto find = world.find(id);
    if (find == world.end()) return nullptr;
    return &find->second;
}

// Operator implementations
bool RoomRef::operator==(const RoomRef& other) const {
    return id == other.id && generation == other.generation;
}

bool RoomRef::operator!=(const RoomRef& other) const {
    return !(*this == other);
}

room_data* RoomRef::operator->() {
    return get();
}

RoomRef::operator room_data*() {
    return get();
}

RoomRef& RoomRef::operator=(const room_data* obj) {
    if (obj) {
        id = obj->id;
        generation = obj->generation;
    } else {
        id = NOTHING;
        generation = 0;
    }
    return *this;
}

RoomRef& RoomRef::operator=(const room_data& obj) {
    id = obj.id;
    generation = obj.generation;
    return *this;
}