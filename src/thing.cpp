#include "dbat/structs.h"
#include "dbat/send.h"

Room* AbstractThing::getRoom() const {
    return dynamic_cast<Room*>(location.unit);
}

room_vnum AbstractThing::getRoomVnum() const {
    auto r = getRoom();
    return r ? r->getVnum() : NOWHERE;
}

Location AbstractThing::getAbsoluteLocation() const {
    if(auto l = dynamic_cast<AbstractLocation*>(location.unit)) {
        return location;
    } else {
        auto t = dynamic_cast<AbstractThing*>(location.unit);
        if(t) {
            return t->getAbsoluteLocation();
        } else {
            return {};
        }
    }
}

void AbstractThing::addToInventory(Object* obj) {
    if(!obj) return;
    auto old_loc = obj->location;
    obj->clearLocation();
    Location l;
    l.unit = this;
    l.position.x = -1;
    obj->setLocation(l);
}

std::vector<std::weak_ptr<Object>> AbstractThing::getInventory() const {
    std::vector<std::weak_ptr<Object>> inv;
    for(const auto& obj : contents) {
        if(auto o = obj.lock()) {
            if(o->location.position.x < 0) {
                inv.emplace_back(o);
            }
        }
    }
    return inv;
}

std::map<int, Object *> AbstractThing::getEquipment() {
    std::map<int, Object*> out;
    for(const auto &uw : filter_raw(contents)) {
        if(auto o = dynamic_cast<Object*>(uw))
            if(o->location.position.x >= 0) out[o->location.position.x] = o;
    }
    return out;
}

Object* AbstractThing::getEquipSlot(int slot) {
    auto eq = getEquipment();
    if(eq.contains(slot)) {
        return eq[slot];
    }
    // If we don't have the slot, return nullptr.
    return nullptr;
}