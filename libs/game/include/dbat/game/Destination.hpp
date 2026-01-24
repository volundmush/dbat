#pragma once
#include <unordered_map>
#include <functional>
#include <enchantum/fmt_format.hpp>

#include "Location.hpp"
#include "const/ExitFlag.hpp"


struct Destination : public Location {
    using Location::Location;
    using Location::operator=;
    Destination() = default;
    Destination(const Location& loc) : Location(loc) {}
    Direction dir{Direction::north}; /* Direction of the exit */

    std::string general_description{};       /* When look DIR.			*/
    std::string keyword{};        /* for open/close			*/

    FlagHandler<ExitFlag> exit_flags{}; /* Exit flags			*/

    obj_vnum key{NOTHING};        /* Key's number (-1 for no key)		*/

    int dclock{};            /* DC to pick the lock			*/
    int dchide{};            /* DC to find hidden			*/

    // this field deliberately not serialized.
    bool generated{false};

    std::optional<Destination> getReverse() const;

    void legacyExitFlags(int flags);
};

inline std::string format_as(const Destination& d) {
    return fmt::format("{} Exit {} to {}", d.generated ? "Generated" : "Direct", enchantum::to_string(d.dir), d.getLocID());
}

#define EXIT(ch, door)  ch->location.getExit(static_cast<Direction>(door))
#define W_EXIT(room, num)     get_room(room)->exits.contains(static_cast<Direction>(num))
#define EXIT_FLAGGED(exit, flag) exit->exit_flags[flag]

constexpr int BFS_ALREADY_THERE = -1;
constexpr int BFS_NO_PATH = -2;
constexpr int BFS_ERROR = -3;
constexpr int BFS_TOO_FAR = -4;

extern int find_first_step(Location &src, Location &target);

std::unordered_map<Coordinates, Destination> gatherSurroundings(const Location& loc, Character *ch, int minX = -4, int maxX = 4, int minY = -4, int maxY = 4, const std::function<bool(const Destination&, Character*)> is_valid = {});