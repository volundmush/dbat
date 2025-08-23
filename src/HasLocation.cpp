#include "dbat/HasLocation.h"
#include "dbat/Room.h"
#include "dbat/Location.h"


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
    Location loc = location;
    if (auto a = loc.getLoc()) {
        a->addToContents(loc.position, getSharedHasLocation());
        onMoveToLocation(loc);
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
    Location loc = location;
    if (auto a = loc.getLoc()) {
        location.al.reset();
        location.position = {};
        a->removeFromContents(getSharedHasLocation());
        onLeaveLocation(loc);
    } else {
        location.al.reset();
        location.position = {};
    }
}

bool HasLocation::getLocationVisibleTo(Character* viewer) {
    return true;
}

void HasLocation::onMoveToLocation(const Location& loc) {
    // default implementation does nothing.
}

void HasLocation::onLeaveLocation(const Location& loc) {
    // default implementation does nothing.
}

void HasLocation::onLocationChanged(const Location& oldloc, const Location& newloc) {
    // this isn't used yet.
}