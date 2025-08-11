#include "dbat/structs.h"

std::vector<std::weak_ptr<obj_data>> AbstractGridArea::getObjects(const Coordinates& coor) const {
    std::vector<std::weak_ptr<obj_data>> out;
    for(const auto &uw : contents) {
        if(auto u = uw.lock()) {
            if(auto o = std::dynamic_pointer_cast<obj_data>(u); o && o->location.position == coor) {
                out.push_back(o);
            }
        }
    }
    out.shrink_to_fit();
    return out;
}

std::vector<std::weak_ptr<char_data>> AbstractGridArea::getPeople(const Coordinates& coor) const {
    std::vector<std::weak_ptr<char_data>> out;
    for(const auto &uw : contents) {
        if(auto u = uw.lock()) {
            if(auto c = std::dynamic_pointer_cast<char_data>(u); c && c->location.position == coor) {
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

std::map<Direction, Destination> AbstractGridArea::getDirections(const Coordinates& coor) override {
    std::map<Direction, Destination> out;
    for(auto dir : magic_enum::enum_values<Direction>()) {
        if (auto dest = getDirection(coor, dir)) {
            out[dir] = *dest;
        }
    }
    return out;
}

