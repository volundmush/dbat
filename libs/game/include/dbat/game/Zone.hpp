#pragma once
#include <map>

#include "Log.hpp"
#include "const/ZoneFlag.hpp"
#include "const/WhereFlag.hpp"
#include "Flags.hpp"
#include "Typedefs.hpp"
#include "WeakBag.hpp"
#include "Result.hpp"
#include "Location.hpp"

struct Character;
struct Room;
struct Object;
struct Area;
struct Structure;

struct Zone {
    zone_vnum number{NOTHING};        /* virtual number of this zone	  */

    zone_vnum parent{NOTHING};
    std::unordered_set<zone_vnum> children;

    std::vector<Zone*> getChain();
    std::vector<Zone*> getAncestors() const;
    std::vector<Zone*> getDescendants() const;
    std::vector<Zone*> getChildren() const;
    Zone* getParent() const;
    Zone* getUpZone(int upwards = 1);
    Zone* getRoot();

    void reset();

    std::string name{};            /* name of this zone                  */
    std::string colorName{};   // color name to display.
    std::string builders{};          /* namelist of builders allowed to    */
    /* modify this zone.		  */
    int lifespan{5};           /* how long between resets (minutes)  */
    double age{};                /* current age of this zone (seconds) */

    int reset_mode{2};         /* conditions for reset (see below)   */

    FlagHandler<ZoneFlag> zone_flags{};          /* Flags for the zone.                */
    FlagHandler<WhereFlag> where_flags{};        /* OUTDOORS, etc                      */
    bool getFlag(ZoneFlag flag, bool checkAncestors = false) const;
    bool getFlag(WhereFlag flag, bool checkAncestors = false) const;

    std::unordered_map<int, double> environment;

    double getEnvironment(int type, bool checkAncestors = false) const;

    std::string displayNameFor(Character *ch);

    /*
     * Reset mode:
     *   0: Don't reset, and don't update age.
     *   1: Reset if no PC's are located in zone.
     *   2: Just reset.
     */
    WeakBag<Room> rooms;
    WeakBag<Area> areas;
    WeakBag<Character> npcsInZone;
    WeakBag<Character> playersInZone;
    WeakBag<Character> listeningInZone;
    WeakBag<Object> objectsInZone;
    WeakBag<Structure> structuresInZone;

    template<class F>
    void for_each_room(F&& func, bool descend = true) {
        rooms.for_each(func);
        if(descend)
            for(auto child : getChildren())
                child->for_each_room(func, descend); 
    }

    template<class F>
    void for_each_player(F&& func, bool descend = true) {
        playersInZone.for_each(func);
        if(descend)
            for(auto child : getChildren())
                child->for_each_player(func, descend);
    }

    template<class F>
    void for_each_listening(F&& func, bool descend = true) {
        listeningInZone.for_each(func);
        if(descend)
            for(auto child : getChildren())
                child->for_each_listening(func, descend);
    }

    template<class F>
    void for_each_npc(F&& func, bool descend = true) {
        npcsInZone.for_each(func);
        if(descend)
            for(auto child : getChildren())
                child->for_each_npc(func, descend);
    }

    template<class F>
    void for_each_character(F&& func, bool descend = true) {
        npcsInZone.for_each(func);
        playersInZone.for_each(func);
        if(descend)
            for(auto child : getChildren()) {
                child->for_each_npc(func, descend);
                child->for_each_player(func, descend);
            }
    }

    template<class F>
    void for_each_object(F&& func, bool descend = true) {
        objectsInZone.for_each(func);
        if(descend)
            for(auto child : getChildren())
                child->for_each_object(func, descend);
    }

    template<class F>
    void for_each_area(F&& func, bool descend = true) {
        areas.for_each(func);
        if(descend)
            for(auto child : getChildren())
                child->for_each_area(func, descend);
    }

    template<class F>
    void for_each_structure(F&& func, bool descend = true) {
        structuresInZone.for_each(func);
        if(descend)
            for(auto child : getChildren())
                child->for_each_structure(func, descend);
    }

    void sendText(const std::string &txt);

    template<typename... Args>
    void sendFmt(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            if(formatted_string.empty()) return;
            sendText(formatted_string);
        }
        catch(const fmt::format_error& e) {
            LERROR("SYSERR: Format error in Zone::sendFmt: %s", e.what());
            LERROR("Template was: %s", format.data());
        }
    }

    template<typename... Args>
    size_t send_to(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
            if(formatted_string.empty()) return 0;
            sendText(formatted_string);
            return formatted_string.size();
        }
        catch(const fmt::format_error& e) {
            LERROR("SYSERR: Format error in Zone::send_to: %s", e.what());
            LERROR("Template was: %s", format.data());
            return 0;
        }
    }

    Result<bool> canBeDeletedBy(Character* ch);

    // if a room in this or a descendant zone is OUTDOORS, the "fly space"
    // or "pilot launch" commands will take you here.
    // Stores a LocID like R:50 or A:10:0:0:9
    // the actor will look up the zone chain until it either finds a
    // launchDestination or runs out of Zones to check.
    std::string launchDestination{};

    // Landing spots for the zone. If you're IN launchDestination...
    // Then these are a combination of name->LocID landing spots
    // to display.
    std::unordered_map<std::string, std::string> landingSpots;
    // Specifically for ships landing.
    std::unordered_map<std::string, std::string> dockingSpots;

    std::map<std::string, Location> getLandingSpots();
    std::map<std::string, Location> getDockingSpots();

    void sendToSense(Character *source, const char* messg, bool childrenOnly = false);
    void actToOutside(Character *source, const char* messg, bool childrenOnly = false);
};

inline std::string format_as(const Zone& z) {
    return fmt::format("Zone {} '{}'", z.number, z.name);
}

#define ZONE_FLAGS(rnum)       (zone_table.at((rnum)).zone_flags)
#define ZONE_MINLVL(rnum)      (zone_table.at((rnum)).min_level)
#define ZONE_MAXLVL(rnum)      (zone_table.at((rnum)).max_level)
#define ZONE_FLAGGED(rnum, flag)   (zone_table.at((rnum)).zone_flags.get((flag)))

template<typename T>
std::string renderZoneChain(T& iter, Character *viewer, std::string_view delim = " ->") {
    std::vector<std::string> chain;
    for (auto& zone : iter) {
        chain.push_back(zone->displayNameFor(viewer));
    }
    return fmt::format("{}", fmt::join(chain, delim));
}

extern std::unordered_set<zone_vnum> zone_reset_queue;

extern std::map<zone_vnum, std::shared_ptr<Zone>> zone_table;

extern void reset_zone(zone_rnum zone);

extern void zone_update(uint64_t heartPulse, double deltaTime);

extern int is_empty(zone_rnum zone_nr);

extern zone_rnum real_zone(zone_vnum vnum);

extern std::vector<Zone*> getZoneChildren(zone_vnum parent = NOTHING);

void print_zone(Character *ch, zone_vnum vnum);
void list_zones(Character *ch);