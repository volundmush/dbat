#include "dbat/structs.h"
#include "dbat/constants.h"

TileOverride::operator bool() const
{
    // A TileOverride is considered "valid" if it has at least one of the following:
    return sectorType.has_value() || !strings.empty() || roomFlags.count() || whereFlags.count() || !exits.empty() || damage != 0 || groundEffect != 0 || !environment.empty() || !exits.empty();
}

// Helper to fetch or create a TileOverride entry.
static inline TileOverride &ensure_tile(std::unordered_map<Coordinates, TileOverride> &map, const Coordinates &c)
{
    return map.try_emplace(c, TileOverride{}).first->second;
}

static inline const TileOverride *find_tile(const std::unordered_map<Coordinates, TileOverride> &map, const Coordinates &c)
{
    auto it = map.find(c);
    if (it == map.end())
        return nullptr;
    return &it->second;
}
static inline TileOverride *find_tile_nc(std::unordered_map<Coordinates, TileOverride> &map, const Coordinates &c)
{
    auto it = map.find(c);
    if (it == map.end())
        return nullptr;
    return &it->second;
}

bool AbstractGridArea::validCoordinates(const Coordinates& coor) const {

    // check if dest is within bounds. return std::nullopt if not.
    if (maxX && coor.x > *maxX)
        return false;
    if (maxY && coor.y > *maxY)
        return false;
    if (maxZ && coor.z > *maxZ)
        return false;
    if (minX && coor.x < *minX)
        return false;
    if (minY && coor.y < *minY)
        return false;
    if (minZ && coor.z < *minZ)
        return false;

    if (auto it = tileOverrides.find(coor); it != tileOverrides.end() && it->second.sectorType)
    {
        // If the tile we're on has a sector type override.
        // This means it exists.
        return true;
    }

    // next we check the default tile types...
    if (coor.z == 0 && defaultGroundSector)
    {
        return true;
    }
    if (coor.z < 0 && defaultUnderSector)
    {
        return true;
    }
    if (coor.z > 0 && defaultSkySector)
    {
        return true;
    }
    return false;
}

std::optional<Destination> AbstractGridArea::getDirection(const Coordinates &coor, Direction dir)
{
    // First check for an override...
    auto it = tileOverrides.find(coor);
    if (it != tileOverrides.end())
    {
        auto dit = it->second.exits.find(dir);
        if (dit != it->second.exits.end())
        {
            dit->second.generated = false;
            return dit->second;
        }
    }

    // There was no overridden direction, so the fallback logic
    // is to get the coordinates of the destination, determine
    // if it exists and is navigable. If so, we'll create a Destination
    // dynamically.

    switch(dir) {
        case Direction::inside:
        case Direction::outside:
            // These directions are not valid for grid areas.
            return std::nullopt;
    }

    // Apply our direction
    Destination dest;
    dest.al = getSharedAbstractLocation();
    dest.position = coor;
    dest.position.apply(dir);
    dest.dir = dir;
    dest.generated = true;

    if(validCoordinates(dest.position)) {
        return dest;
    }
    return std::nullopt;
}

std::map<Direction, Destination> AbstractGridArea::getDirections(const Coordinates &coor)
{
    std::map<Direction, Destination> out;
    for (auto dir : magic_enum::enum_values<Direction>())
    {
        if (auto dest = getDirection(coor, dir))
        {
            out[dir] = *dest;
        }
    }
    return out;
}

const std::vector<ExtraDescription> &AbstractGridArea::getExtraDescription(const Coordinates &coor) const
{
    static const std::vector<ExtraDescription> empty; // Grid areas do not support per-tile extra descriptions yet.
    return empty;
}

const char *AbstractGridArea::getName(const Coordinates &coor) const
{
    if (auto t = find_tile(tileOverrides, coor))
    {
        if (auto it = t->strings.find("name"); it != t->strings.end())
        {
            return it->second.c_str();
        }
    }
    // Fallback to unit base name.
    return HasMudStrings::getName();
}

const char *AbstractGridArea::getLookDescription(const Coordinates &coor) const
{
    if (auto t = find_tile(tileOverrides, coor))
    {
        if (auto it = t->strings.find("look_description"); it != t->strings.end())
        {
            return it->second.c_str();
        }
    }
    // Fallback to base unit look description.
    return HasMudStrings::getLookDescription();
}

FlagHandler<RoomFlag>& AbstractGridArea::getRoomFlags(const Coordinates& coor)
{
    auto &t = ensure_tile(tileOverrides, coor);
    return t.roomFlags;
}

FlagHandler<WhereFlag>& AbstractGridArea::getWhereFlags(const Coordinates& coor)
{
    auto &t = ensure_tile(tileOverrides, coor);
    return t.whereFlags;
}

SectorType AbstractGridArea::getSectorType(const Coordinates &coor) const
{
    if (auto t = find_tile(tileOverrides, coor))
    {
        if (t->sectorType)
            return *t->sectorType;
    }
    // Default logic by z level
    if (coor.z == 0 && defaultGroundSector)
        return *defaultGroundSector;
    if (coor.z < 0 && defaultUnderSector)
        return *defaultUnderSector;
    if (coor.z > 0 && defaultSkySector)
        return *defaultSkySector;
    return SectorType::inside; // mandated fallback
}

void AbstractGridArea::broadcastAt(const Coordinates &coor, const std::string &message)
{
    auto people = getPeople(coor);
    for (const auto &wp : people)
    {
        if (auto c = wp.lock())
            c->sendText(message);
    }
}

int AbstractGridArea::getDamage(const Coordinates &coor) const
{
    if (auto t = find_tile(tileOverrides, coor))
        return t->damage;
    return 0;
}

int AbstractGridArea::setDamage(const Coordinates &coor, int amount)
{
    auto &t = ensure_tile(tileOverrides, coor);
    t.damage = amount;
    return t.damage;
}

int AbstractGridArea::modDamage(const Coordinates &coor, int amount)
{
    auto &t = ensure_tile(tileOverrides, coor);
    t.damage += amount;
    return t.damage;
}

int AbstractGridArea::getGroundEffect(const Coordinates &coor) const
{
    if (auto t = find_tile(tileOverrides, coor))
        return t->groundEffect;
    return 0;
}

void AbstractGridArea::setGroundEffect(const Coordinates &coor, int val)
{
    auto &t = ensure_tile(tileOverrides, coor);
    t.groundEffect = val;
}

int AbstractGridArea::modGroundEffect(const Coordinates &coor, int val)
{
    auto &t = ensure_tile(tileOverrides, coor);
    t.groundEffect += val;
    return t.groundEffect;
}

SpecialFunc AbstractGridArea::getSpecialFunc(const Coordinates &coor) const
{
    return nullptr; // grid tiles do not have special procs (yet)
}

double AbstractGridArea::getEnvironment(const Coordinates &coor, int type) const
{
    if (auto t = find_tile(tileOverrides, coor))
    {
        if (auto it = t->environment.find(type); it != t->environment.end())
            return it->second;
    }
    return 0.0;
}

double AbstractGridArea::setEnvironment(const Coordinates &coor, int type, double value)
{
    auto &t = ensure_tile(tileOverrides, coor);
    t.environment[type] = value;
    return value;
}

double AbstractGridArea::modEnvironment(const Coordinates &coor, int type, double value)
{
    auto &t = ensure_tile(tileOverrides, coor);
    double &ref = t.environment[type];
    ref += value;
    return ref;
}

void AbstractGridArea::clearEnvironment(const Coordinates &coor, int type)
{
    if (auto t = find_tile_nc(tileOverrides, coor))
    {
        t->environment.erase(type);
    }
}

void AbstractGridArea::replaceExit(const Coordinates &coor, const Destination &dest)
{
    auto &t = ensure_tile(tileOverrides, coor);
    t.exits[dest.dir] = dest;
}

void AbstractGridArea::deleteExit(const Coordinates &coor, Direction dir)
{
    if (auto t = find_tile_nc(tileOverrides, coor))
    {
        t->exits.erase(dir); // idempotent
    }
}


bool AbstractGridArea::buildwalk(const Coordinates& coor, Character* ch, Direction dir) {
    // by the time we reach this function, all permission checks have already been carried out.

    if(validCoordinates(coor)) {
        ch->sendText("You cannot buildwalk in a tile that already exists.\r\n");
        return false;
    }
    auto t = ensure_tile(tileOverrides, coor);
    if(t.sectorType) {
        ch->sendText("You cannot buildwalk in a tile that already has a sector type.\r\n");
        return false;
    }

    if(coor.z == 0) {
        t.sectorType = defaultGroundSector ? *defaultGroundSector : SectorType::inside;
    } else if(coor.z > 0) {
        t.sectorType = defaultSkySector ? *defaultSkySector : SectorType::inside;
    } else if(coor.z < 0) {
        t.sectorType = defaultUnderSector ? *defaultUnderSector : SectorType::inside;
    }

    return true;
}