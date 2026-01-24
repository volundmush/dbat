#pragma once
#include "Location.hpp"

// Character, Object, and perhaps later some new vehicles use HasLocation.
struct HasLocation {
    Location location;

    std::unordered_map<std::string, Location> registeredLocations;

    struct Room* getRoom();
    room_vnum getRoomVnum();

    virtual bool isActiveInLocation() const;

    virtual std::shared_ptr<HasLocation> getSharedHasLocation();

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
    virtual std::string getLocationDisplayCategory(Character* viewer) const;

    // Display information about the thing in its current information. used for 'room displays.'
    // Currently doesn't return anything because lots of old code directly calls viewer->sendText()...
    virtual void displayLocationInfo(Character* viewer);

    virtual bool getLocationVisibleTo(Character* viewer);

    //virtual std::string renderInLocationFor(Character* viewer);
};

inline std::string format_as(const HasLocation& unit) {
    std::vector<std::string> locs;
    for(const auto& [name, loc] : unit.registeredLocations) {
        locs.push_back(fmt::format("{}: {} ({})", name, loc.getLocID(), loc.getName()));
    }
    if(locs.empty()) locs.push_back("<none>");

    return fmt::format("Location: {} ({})\r\nSaved Location: {}", unit.location.getLocID(), unit.location.getName(), fmt::join(locs, ", "));
}