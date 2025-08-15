#include "dbat/structs.h"


Room *HasLocation::getRoom() const
{
    if(auto a = location.al.lock())
    {
        if (auto r = dynamic_cast<Room *>(a.get()))
        {
            return r;
        }
    }
    return nullptr;
}

room_vnum HasLocation::getRoomVnum() const
{
    auto r = getRoom();
    return r ? r->getVnum() : NOWHERE;
}

void HasLocation::moveToLocation(const Location& loc) {
    if (auto a = loc.al.lock())
        a->addToContents(loc.position, getSharedHasLocation());
}

void HasLocation::leaveLocation()
{
    if (auto a = location.al.lock())
        a->removeFromContents(getSharedHasLocation());
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