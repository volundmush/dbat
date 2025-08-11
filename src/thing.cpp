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