#include "dbat/game/HasLocation.hpp"
#include "dbat/game/Room.hpp"
#include "dbat/game/Location.hpp"
#include <nlohmann/json.hpp>

void to_json(nlohmann::json& j, const HasLocation& unit)
{
    if(unit.location) {
        j[+"location"] = unit.location;
    }
    if(!unit.registeredLocations.empty()) {
        j[+"registeredLocations"] = unit.registeredLocations;
    }
}

void from_json(const nlohmann::json& j, HasLocation& unit)
{
    if(j.contains(+"location")) {
        j.at(+"location").get_to(unit.location);
    }
    if(j.contains(+"registeredLocations")) {
        j.at(+"registeredLocations").get_to(unit.registeredLocations);
    }
}


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
        a->removeFromContents(getSharedHasLocation());
        onLeaveLocation(loc);
    }
    location.al.reset();
    location.position = {};
    location.locationID = "";
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

std::shared_ptr<HasLocation> HasLocation::getSharedHasLocation() {
    return {};
}

std::string HasLocation::getLocationDisplayCategory(Character* viewer) const {
    return "unknown";
}

bool HasLocation::isActiveInLocation() const {
    return true;
}

void HasLocation::displayLocationInfo(Character* viewer) {
    // default implementation does nothing.
}
