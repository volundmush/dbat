#include "dbat/structs.h"
#include "dbat/constants.h"

// Helper to fetch or create a TileOverride entry.
static inline TileOverride &ensure_tile(std::unordered_map<Coordinates, TileOverride> &map, const Coordinates &c) {
    return map.try_emplace(c, TileOverride{}).first->second;
}

static inline const TileOverride *find_tile(const std::unordered_map<Coordinates, TileOverride> &map, const Coordinates &c) {
    auto it = map.find(c);
    if(it == map.end()) return nullptr;
    return &it->second;
}
static inline TileOverride *find_tile_nc(std::unordered_map<Coordinates, TileOverride> &map, const Coordinates &c) {
    auto it = map.find(c);
    if(it == map.end()) return nullptr;
    return &it->second;
}

std::vector<std::weak_ptr<Object>> AbstractGridArea::getObjects(const Coordinates& coor) const {
    std::vector<std::weak_ptr<Object>> out;
    for(const auto &uw : contents) {
        if(auto u = uw.lock()) {
            if(auto o = std::dynamic_pointer_cast<Object>(u); o && o->location.position == coor) {
                out.push_back(o);
            }
        }
    }
    out.shrink_to_fit();
    return out;
}

std::vector<std::weak_ptr<Character>> AbstractGridArea::getPeople(const Coordinates& coor) const {
    std::vector<std::weak_ptr<Character>> out;
    for(const auto &uw : contents) {
        if(auto u = uw.lock()) {
            if(auto c = std::dynamic_pointer_cast<Character>(u); c && c->location.position == coor) {
                out.push_back(c);
            }
        }
    }
    out.shrink_to_fit();
    return out;
}

std::optional<Destination> AbstractGridArea::getDirection(const Coordinates& coor, Direction dir) {
    // First check for an override...
    auto it = tileOverrides.find(coor);
    if(it != tileOverrides.end()) {
        auto dit = it->second.exits.find(dir);
        if(dit != it->second.exits.end()) {
            return dit->second;
        }
    }

    // There was no overridden direction, so the fallback logic
    // is to get the coordinates of the destination, determine
    // if it exists and is navigable. If so, we'll create a Destination
    // dynamically.

    // Apply our direction
    Destination dest;
    dest.unit = this;
    dest.position = coor;
    dest.position.apply(dir);
    dest.dir = dir;

    // check if dest is within bounds. return std::nullopt if not.
    if(maxX && dest.position.x > *maxX) return std::nullopt;
    if(maxY && dest.position.y > *maxY) return std::nullopt;
    if(maxZ && dest.position.z > *maxZ) return std::nullopt;
    if(minX && dest.position.x < *minX) return std::nullopt;
    if(minY && dest.position.y < *minY) return std::nullopt;
    if(minZ && dest.position.z < *minZ) return std::nullopt;
    
    if(it != tileOverrides.end() && it->second.sectorType) {
        // If the tile we're on has a sector type override.
        // This means it exists.
        return dest;
    }

    // next we check the default tile types...
    if(dest.position.z == 0 && defaultGroundSector) {
        return dest;
    }
    if(dest.position.z < 0 && defaultUnderSector) {
        return dest;
    }
    if(dest.position.z > 0 && defaultSkySector) {
        return dest;
    }

    return std::nullopt;
}

std::map<Direction, Destination> AbstractGridArea::getDirections(const Coordinates& coor) {
    std::map<Direction, Destination> out;
    for(auto dir : magic_enum::enum_values<Direction>()) {
        if (auto dest = getDirection(coor, dir)) {
            out[dir] = *dest;
        }
    }
    return out;
}

const std::vector<ExtraDescription>& AbstractGridArea::getExtraDescription(const Coordinates& coor) const {
    static const std::vector<ExtraDescription> empty; // Grid areas do not support per-tile extra descriptions yet.
    return empty;
}

const char* AbstractGridArea::getName(const Coordinates& coor) const {
    if(auto t = find_tile(tileOverrides, coor)) {
        if(auto it = t->strings.find("name"); it != t->strings.end()) {
            return it->second.c_str();
        }
    }
    // Fallback to unit base name.
    return getName();
}

const char* AbstractGridArea::getLookDescription(const Coordinates& coor) const {
    if(auto t = find_tile(tileOverrides, coor)) {
        if(auto it = t->strings.find("look_description"); it != t->strings.end()) {
            return it->second.c_str();
        }
    }
    // Fallback to base unit look description.
    return Entity::getLookDescription();
}

void AbstractGridArea::setRoomFlag(const Coordinates& coor, RoomFlag flag, bool value) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.roomFlags.set(flag, value);
}

bool AbstractGridArea::toggleRoomFlag(const Coordinates& coor, RoomFlag flag) {
    auto &t = ensure_tile(tileOverrides, coor);
    return t.roomFlags.toggle(flag);
}

bool AbstractGridArea::getRoomFlag(const Coordinates& coor, RoomFlag flag) const {
    if(auto t = find_tile(tileOverrides, coor)) {
        return t->roomFlags.get(flag);
    }
    return false;
}

void AbstractGridArea::setWhereFlag(const Coordinates& coor, WhereFlag flag, bool value) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.whereFlags.set(flag, value);
}

bool AbstractGridArea::toggleWhereFlag(const Coordinates& coor, WhereFlag flag) {
    auto &t = ensure_tile(tileOverrides, coor);
    return t.whereFlags.toggle(flag);
}

bool AbstractGridArea::getWhereFlag(const Coordinates& coor, WhereFlag flag) const {
    if(auto t = find_tile(tileOverrides, coor)) {
        return t->whereFlags.get(flag);
    }
    return false;
}

SectorType AbstractGridArea::getSectorType(const Coordinates& coor) const {
    if(auto t = find_tile(tileOverrides, coor)) {
        if(t->sectorType) return *t->sectorType;
    }
    // Default logic by z level
    if(coor.z == 0 && defaultGroundSector) return *defaultGroundSector;
    if(coor.z < 0 && defaultUnderSector) return *defaultUnderSector;
    if(coor.z > 0 && defaultSkySector) return *defaultSkySector;
    return SectorType::inside; // mandated fallback
}

void AbstractGridArea::broadcastAt(const Coordinates& coor, const std::string& message) const {
    auto people = getPeople(coor);
    for(const auto &wp : people) {
        if(auto c = wp.lock()) c->sendText(message);
    }
}

int AbstractGridArea::getDamage(const Coordinates& coor) const {
    if(auto t = find_tile(tileOverrides, coor)) return t->damage;
    return 0;
}

int AbstractGridArea::setDamage(const Coordinates& coor, int amount) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.damage = amount;
    return t.damage;
}

int AbstractGridArea::modDamage(const Coordinates& coor, int amount) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.damage += amount;
    return t.damage;
}

int AbstractGridArea::getGroundEffect(const Coordinates& coor) const {
    if(auto t = find_tile(tileOverrides, coor)) return t->groundEffect;
    return 0;
}

void AbstractGridArea::setGroundEffect(const Coordinates& coor, int val) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.groundEffect = val;
}

int AbstractGridArea::modGroundEffect(const Coordinates& coor, int val) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.groundEffect += val;
    return t.groundEffect;
}

SpecialFunc AbstractGridArea::getSpecialFunc(const Coordinates& coor) const {
    return nullptr; // grid tiles do not have special procs (yet)
}

double AbstractGridArea::getEnvironment(const Coordinates& coor, int type) const {
    if(auto t = find_tile(tileOverrides, coor)) {
        if(auto it = t->environment.find(type); it != t->environment.end()) return it->second;
    }
    return 0.0;
}

double AbstractGridArea::setEnvironment(const Coordinates& coor, int type, double value) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.environment[type] = value;
    return value;
}

double AbstractGridArea::modEnvironment(const Coordinates& coor, int type, double value) {
    auto &t = ensure_tile(tileOverrides, coor);
    double &ref = t.environment[type];
    ref += value;
    return ref;
}

void AbstractGridArea::clearEnvironment(const Coordinates& coor, int type) {
    if(auto t = find_tile_nc(tileOverrides, coor)) {
        t->environment.erase(type);
    }
}

void AbstractGridArea::replaceExit(const Coordinates& coor, const Destination& dest) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.exits[dest.dir] = dest;
}

void AbstractGridArea::deleteExit(const Coordinates& coor, Direction dir) {
    if(auto t = find_tile_nc(tileOverrides, coor)) {
        t->exits.erase(dir); // idempotent
    }
}

