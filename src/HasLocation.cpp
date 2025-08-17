#include "dbat/structs.h"


Room *HasLocation::getRoom()
{
    if(auto a = location.getLoc())
    {
        if (auto r = dynamic_cast<Room *>(a))
        {
            return r;
        }
    }
    return nullptr;
}

room_vnum HasLocation::getRoomVnum()
{
    auto r = getRoom();
    return r ? r->getVnum() : NOWHERE;
}

void HasLocation::updateLocation() {
    if (auto a = location.getLoc()) {
        a->addToContents(location.position, getSharedHasLocation());
    }
}

void HasLocation::moveToLocation(const Location& loc) {
    if (auto a = loc.getLoc()) {
        location = loc;
        updateLocation();
    }
}

void HasLocation::leaveLocation()
{
    if (auto a = location.getLoc()) {
        location.al.reset();
        location.position = {};
        a->removeFromContents(getSharedHasLocation());
    } else {
        location.al.reset();
        location.position = {};
    }
}

bool HasLocation::getLocationVisibleTo(Character* viewer) {
    return true;
}

void HasLocation::onMoveToLocation(const Location& loc) {
    
}

void HasLocation::onLeaveLocation(const Location& loc) {
    
}

void HasLocation::onLocationChanged(const Location& oldloc, const Location& newloc) {

}