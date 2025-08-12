#include "dbat/structs.h"
#include "dbat/filter.h"

vnum HasVnum::getVnum() const {
    return vn;
}

Room* HasLocation::getRoom() const {
    return dynamic_cast<Room*>(location.unit);
}

room_vnum HasLocation::getRoomVnum() const {
    auto r = getRoom();
    return r ? r->getVnum() : NOWHERE;
}

void HasLocation::setLocation(const Location& loc) {
    addToLocation(loc);
}

const char* HasMudStrings::getName() const {
    if(auto find = strings.find("name"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const char* HasMudStrings::getRoomDescription() const {
    if(auto find = strings.find("room_description"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const char* HasMudStrings::getLookDescription() const {
    if(auto find = strings.find("look_description"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const char* HasMudStrings::getShortDescription() const {
    if(auto find = strings.find("short_description"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

std::string_view HasMudStrings::getString(const std::string &key) const {
    if(auto it = strings.find(key); it != strings.end()) {
        return it->second;
    }
    return {};
}

const std::vector<ExtraDescription>& HasExtraDescriptions::getExtraDescription() const {
    return extra_descriptions;
}

void HasInventory::addToInventory(Object* obj) {
    addToInventory(obj->shared());
}

void HasInventory::addToInventory(const std::shared_ptr<Object>& obj) {
    inventory.push_back(obj);
    onAddToInventory(obj);
}

void HasInventory::removeFromInventory(Object* obj) {
    removeFromInventory(obj->shared());
}

void HasInventory::removeFromInventory(const std::shared_ptr<Object>& obj) {
    inventory.remove_if([&](const std::weak_ptr<Object>& weak_obj) {
        return weak_obj.lock() == obj;
    });
    onRemoveFromInventory(obj);
}



std::vector<std::weak_ptr<Object>> HasInventory::getInventory() const {
    std::vector<std::weak_ptr<Object>> out;
    for (const auto& weak_obj : inventory) {
        if (auto obj = weak_obj.lock()) {
            out.push_back(obj);
        }
    }
    return out;
}

std::map<int, Object*> HasEquipment::getEquipment() const {
    std::map<int, Object*> out;
    for(auto &[slot, ow] : equipment) {
        if(auto o = ow.lock()) {
            out[slot] = o.get();
        }
    }
    return out;
}

Object* HasEquipment::getEquipSlot(int slot) const {
    if(auto find = equipment.find(slot); find != equipment.end()) {
        if(auto o = find->second.lock()) {
            return o.get();
        }
    }
    return nullptr;
}

void HasEquipment::addToEquip(Object* obj, int slot) {
    addToEquip(obj->shared(), slot);
}

void HasEquipment::addToEquip(const std::shared_ptr<Object>& obj, int slot) {
    if(auto has = getEquipSlot(slot)) {
        throw std::runtime_error(fmt::format("Slot {} already occupied by {}", slot, has->getName()));
    }
    equipment[slot] = obj;
    onAddToEquip(obj, slot);
}



void HasEquipment::removeFromEquip(int slot) {
    if(auto it = equipment.find(slot); it != equipment.end()) {
        if(auto obj = it->second.lock()) {
            onRemoveFromEquip(obj, slot);
        }
        equipment.erase(it);
    }
}

void HasEquipment::removeFromEquip(Object* obj) {
    removeFromEquip(obj->shared());
}

void HasEquipment::removeFromEquip(const std::shared_ptr<Object>& obj) {
    for(auto it = equipment.begin(); it != equipment.end();) {
        if(auto o = it->second.lock()) {
            if(o == obj) {
                it = equipment.erase(it);
                auto slot = it->first;
                onRemoveFromEquip(obj, slot);
            } else {
                ++it;
            }
        } else {
            it = equipment.erase(it);
        }
    }
}


void HasInventory::traverseInventory(const std::function<void(Object*)> &func, bool recurse) {
    for (const auto& obj : filter_raw(inventory)) {
        func(obj);
        if (recurse) {
            obj->traverseInventory(func, true);
        }
    }
}

std::unordered_set<Object*> HasInventory::gatherFromInventory(const std::function<bool(Object*)> &func, bool recurse) {
    std::unordered_set<Object*> out;
    for (const auto& obj : filter_raw(inventory)) {
        if(func(obj)) {
            out.insert(obj);
        }
        if (recurse) {
            auto sub = obj->gatherFromInventory(func, true);
            out.insert(sub.begin(), sub.end());
        }
    }
    return out;
}

Object* HasInventory::searchInventory(const std::function<bool(Object*)> &func, bool recurse) {
    for (const auto& obj : filter_raw(inventory)) {
        if (func(obj)) {
            return obj;
        }
        if (recurse) {
            if (auto sub = obj->searchInventory(func, true)) {
                return sub;
            }
        }
    }
    return nullptr;
}

Object* HasInventory::searchInventory(obj_vnum vnum, bool working, bool recurse) {
    return searchInventory([=](Object* obj) {
        return obj->getVnum() == vnum && (!working || obj->isWorking());
    }, recurse);
}

void HasInventory::activateInventory() {
    for (const auto& obj : filter_raw(inventory)) {
        obj->activate();
    }
}

void HasInventory::deactivateInventory() {
    for (const auto& obj : filter_raw(inventory)) {
        obj->deactivate();
    }
}

void HasEquipment::traverseEquipment(const std::function<void(Object*)> &func, bool recurse) {
    for (const auto& [slot, wobj] : equipment) {
        if (auto obj = wobj.lock()) {
            func(obj.get());
            if (recurse) {
                obj->traverseInventory(func, true);
            }
        }
    }
}

std::unordered_set<Object*> HasEquipment::gatherFromEquipment(const std::function<bool(Object*)> &func, bool recurse) {
    std::unordered_set<Object*> out;
    for (const auto& [slot, wobj] : equipment) {
        if (auto obj = wobj.lock()) {
            if (func(obj.get())) {
                out.insert(obj.get());
            }
            if (recurse) {
                auto sub = obj->gatherFromInventory(func, true);
                out.insert(sub.begin(), sub.end());
            }
        }
    }
    return out;
}

Object* HasEquipment::searchEquipment(const std::function<bool(Object*)> &func, bool recurse) {
    for (const auto& [slot, wobj] : equipment) {
        if (auto obj = wobj.lock()) {
            if (func(obj.get())) {
                return obj.get();
            }
            if (recurse) {
                if (auto sub = obj->searchInventory(func, true)) {
                    return sub;
                }
            }
        }
    }
    return nullptr;
}

Object* HasEquipment::searchEquipment(obj_vnum vnum, bool working, bool recurse) {
    return searchEquipment([=](Object* obj) {
        return obj->getVnum() == vnum && (!working || obj->isWorking());
    }, recurse);
}

void HasEquipment::activateEquipment() {
    for (const auto& [slot, wobj] : equipment) {
        if (auto obj = wobj.lock()) {
            obj->activate();
        }
    }
}

void HasEquipment::deactivateEquipment() {
    for (const auto& [slot, wobj] : equipment) {
        if (auto obj = wobj.lock()) {
            obj->deactivate();
        }
    }
}
