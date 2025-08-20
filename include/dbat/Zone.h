#pragma once
#include "templates.h"


struct Zone {
    zone_vnum number{NOTHING};        /* virtual number of this zone	  */

    zone_vnum parent{NOTHING};
    std::unordered_set<zone_vnum> children;

    std::vector<Zone*> getChain();
    std::vector<Zone*> getAncestors() const;
    std::vector<Zone*> getDescendants() const;
    std::vector<Zone*> getChildren() const;
    Zone* getParent() const;

    void reset();

    std::string name{};            /* name of this zone                  */
    std::string colorName{};   // color name to display.
    std::string builders{};          /* namelist of builders allowed to    */
    /* modify this zone.		  */
    int lifespan{5};           /* how long between resets (minutes)  */
    double age{};                /* current age of this zone (seconds) */

    int reset_mode{2};         /* conditions for reset (see below)   */

    FlagHandler<ZoneFlag> zone_flags{};          /* Flags for the zone.                */

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
    WeakBag<Object> objectsInZone;
    WeakBag<Structure> structuresInZone;

    void sendText(const std::string &txt);

    template<typename... Args>
    void sendFmt(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            if(formatted_string.empty()) return;
            sendText(formatted_string);
        }
        catch(const fmt::format_error& e) {
            basic_mud_log("SYSERR: Format error in Zone::sendFmt: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
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
            basic_mud_log("SYSERR: Format error in Zone::send_to: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
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
};

template <>
struct fmt::formatter<Zone> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Zone& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "Zone {} '{}'", z.number, z.name);
    }
};

#define ZONE_FLAGS(rnum)       (zone_table.at((rnum)).zone_flags)
#define ZONE_MINLVL(rnum)      (zone_table.at((rnum)).min_level)
#define ZONE_MAXLVL(rnum)      (zone_table.at((rnum)).max_level)
#define ZONE_FLAGGED(rnum, flag)   (zone_table.at((rnum)).zone_flags.get((flag)))