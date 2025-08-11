#include "dbat/structs.h"
#include "dbat/send.h"

struct room_data* thing_data::getRoom() const {
    return dynamic_cast<room_data*>(location.unit);
}

room_vnum thing_data::getRoomVnum() const {
    auto r = getRoom();
    return r ? r->getVnum() : NOWHERE;
}

Location thing_data::getAbsoluteLocation() const {
    if(auto l = dynamic_cast<location_data*>(location.unit)) {
        return location;
    } else {
        auto t = dynamic_cast<thing_data*>(location.unit);
        if(t) {
            return t->getAbsoluteLocation();
        } else {
            return {};
        }
    }
}