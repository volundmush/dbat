#pragma once
#include <unordered_set>
#include <string>
#include "dbat/game/Typedefs.hpp"

namespace dbat::dirty {
    extern std::unordered_set<std::string> players;
    extern std::unordered_set<vnum> zones, dgproto, shops, guilds, rooms, areas, structures, nproto, assemblies;
    extern void saveDirty();
    extern void dirtyAll();
};