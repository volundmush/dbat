#pragma once
#include "Location.h"

// Character, Object, and perhaps later some new vehicles use HasLocation.
struct HasLocation {
    Location location;

    std::unordered_map<std::string, Location> registeredLocations;

    struct Room* getRoom();
    room_vnum getRoomVnum();

    virtual bool isActiveInLocation() const = 0;

    virtual std::shared_ptr<HasLocation> getSharedHasLocation() = 0;

    void moveToLocation(const Location& loc);
    void leaveLocation();
    void updateLocation();
    
    // virtual hooks
    virtual void onMoveToLocation(const Location& loc);
    virtual void onLeaveLocation(const Location& loc);
    virtual void onLocationChanged(const Location& oldloc, const Location& newloc);

    // information rendering and interactivity...

    // the keywords used for things like 'look' and 'get'.
    //virtual std::vector<std::string> getInteractivityKeywords(Character* viewer) = 0;

    // When categorizing this thing in a Look display, what should it show under?
    // Should be plural. Example: Creatures, Items, Vehicles, Furniture, etc.
    virtual std::string getLocationDisplayCategory(Character* viewer) const = 0;

    // Display information about the thing in its current information. used for 'room displays.'
    // Currently doesn't return anything because lots of old code directly calls viewer->sendText()...
    virtual void displayLocationInfo(Character* viewer) = 0;

    virtual bool getLocationVisibleTo(Character* viewer);

    //virtual std::string renderInLocationFor(Character* viewer);
};