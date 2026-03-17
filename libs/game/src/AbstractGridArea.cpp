#include "dbat/game/AbstractGridArea.hpp"
#include "dbat/game/Destination.hpp"
#include "dbat/game/Character.hpp"
#include <nlohmann/json.hpp>

std::unordered_map<int64_t, std::shared_ptr<GridTemplate>> gridTemplates;

void to_json(nlohmann::json& j, const RoundDim& unit)
{
    if(unit.center) {
        j[+"center"] = unit.center;
    }
    if(unit.radius != 0) {
        j[+"radius"] = unit.radius;
    }
    if(unit.zMin != 0) {
        j[+"zMin"] = unit.zMin;
    }
    if(unit.zMax != 0) {
        j[+"zMax"] = unit.zMax;
    }
    if(unit.r2 != 0) {
        j[+"r2"] = unit.r2;
    }
}

void from_json(const nlohmann::json& j, RoundDim& unit)
{
    if(j.contains(+"center")) {
        j.at(+"center").get_to(unit.center);
    }
    if(j.contains(+"radius")) {
        j.at(+"radius").get_to(unit.radius);
    }
    if(j.contains(+"zMin")) {
        j.at(+"zMin").get_to(unit.zMin);
    }
    if(j.contains(+"zMax")) {
        j.at(+"zMax").get_to(unit.zMax);
    }
    if(j.contains(+"r2")) {
        j.at(+"r2").get_to(unit.r2);
    }
}

void to_json(nlohmann::json& j, const AABB& unit)
{
    if(unit.min) {
        j[+"min"] = unit.min;
    }
    if(unit.max) {
        j[+"max"] = unit.max;
    }
}

void from_json(const nlohmann::json& j, AABB& unit)
{
    if(j.contains(+"min")) {
        j.at(+"min").get_to(unit.min);
    }
    if(j.contains(+"max")) {
        j.at(+"max").get_to(unit.max);
    }
}

void to_json(nlohmann::json& j, const BoxDim& unit)
{
    if(unit.box.min || unit.box.max) {
        j[+"box"] = unit.box;
    }
}

void from_json(const nlohmann::json& j, BoxDim& unit)
{
    if(j.contains(+"box")) {
        j.at(+"box").get_to(unit.box);
    }
}

TileOverride::operator bool() const
{
    // A TileOverride is considered "valid" if it has at least one of the following:
    return sectorType.has_value() || !name.empty() || !look_description.empty() || roomFlags.count() || whereFlags.count() || !exits.empty() || damage != 0 || groundEffect != 0 || !exits.empty();
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

static inline int floor_div_bucket(int v, int B) {
    // floor division that works for negatives too
    return (v >= 0) ? (v / B) : ((v - (B - 1)) / B);
}

static inline void indexIntoBuckets(
    std::unordered_map<BucketKey, std::vector<Shape*>>& buckets,
    int bucketSize,
    Shape* s)
{
    if (bucketSize <= 0 || !s) return;
    const auto& a = s->aabb();
    const int bx0 = floor_div_bucket(a.min.x, bucketSize);
    const int by0 = floor_div_bucket(a.min.y, bucketSize);
    const int bz0 = floor_div_bucket(a.min.z, bucketSize);
    const int bx1 = floor_div_bucket(a.max.x, bucketSize);
    const int by1 = floor_div_bucket(a.max.y, bucketSize);
    const int bz1 = floor_div_bucket(a.max.z, bucketSize);

    for (int bx = bx0; bx <= bx1; ++bx)
    for (int by = by0; by <= by1; ++by)
    for (int bz = bz0; bz <= bz1; ++bz) {
        buckets[{bx,by,bz}].push_back(s);
    }
}

void AbstractGridArea::rebuildShapeIndex() {
    // clear derived structures
    buckets.clear();
    byPriority.clear();
    byPriority.reserve(shapes.size());

    // recompute cached AABBs and collect pointers
    for (auto& [_, up] : shapes) {
        Shape& s = *up;
        s.reComputeAABB();
        byPriority.push_back(&s);
    }

    // sort: higher priority first; for ties, newer (higher seq) wins
    std::sort(byPriority.begin(), byPriority.end(),
              [](const Shape* a, const Shape* b){
                  if (a->priority != b->priority) return a->priority > b->priority;
                  return a->seq > b->seq; // "last write wins"
              });

    // fill spatial buckets
    if (bucketSize > 0) {
        for (auto* s : byPriority) {
            indexIntoBuckets(buckets, bucketSize, s);
        }
    }
}

AbstractGridArea& AbstractGridArea::operator=(const GridTemplate& other) {
    shapes.clear();
    for(const auto& [name, shapeBase] : other.shapes) {
        shapes[name] = std::make_unique<Shape>(shapeBase);
    }
    rebuildShapeIndex();
    return *this;
}

 Shape* AbstractGridArea::topShapeAt(const Coordinates& c) const {

    auto fromBuckets = [&]() -> Shape* {
        if (bucketSize <= 0) return nullptr;
        auto toB = [&](int v){ return (v>=0 ? v : v - (bucketSize-1)) / bucketSize; };
        BucketKey k{ toB(c.x), toB(c.y), toB(c.z) };
        auto it = buckets.find(k);
        if (it == buckets.end()) return nullptr;

        Shape* best = nullptr;
        for (auto* s : it->second) {
            if (!s->aabb().contains(c)) continue;
            if (!s->contains(c)) continue;
            if (!best ||
                s->priority > best->priority ||
               (s->priority == best->priority && s->seq > best->seq)) { // last-wins
                best = s;
            }
        }
        return best;
    };

    if (auto* picked = fromBuckets()) return picked;

    // Fallback: linear by priority order
    for (auto* s : byPriority) {
        if (s->aabb().contains(c) && s->contains(c)) return s;
    }
    return nullptr;
}

bool AbstractGridArea::validCoordinates(const Coordinates& coor) const {

    if (auto it = tileOverrides.find(coor); it != tileOverrides.end() && it->second.sectorType)
    {
        // If the tile we're on has a sector type override.
        // This means it exists.
        return true;
    }

    if(auto shp = topShapeAt(coor)) {
        // If there's a shape at this coordinate, it exists.
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
        default:
            break;
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
    for (const auto &[dir, name] : enchantum::entries<Direction>)
    {
        if (auto dest = getDirection(coor, dir))
        {
            out[dir] = *dest;
        }
    }
    return out;
}

ExtraDescriptionViews AbstractGridArea::getExtraDescription(const Coordinates &coor) const
{
    static ExtraDescriptionViews empty; // Grid areas do not support per-tile extra descriptions yet.
    return empty;
}

const char *AbstractGridArea::getName(const Coordinates &coor) const
{
    if (auto t = find_tile(tileOverrides, coor))
    {
        if (!t->name.empty()) return t->name.c_str();
    }

    if(auto shp = topShapeAt(coor)) {
        if(!shp->name.empty()) return shp->name.c_str();
    }

    // Fallback to unit base name.
    return HasMudStrings::getName();
}

const char *AbstractGridArea::getLookDescription(const Coordinates &coor) const
{
    if (auto t = find_tile(tileOverrides, coor))
    {
        if(!t->look_description.empty()) return t->look_description.c_str();
    }
    if(auto shp = topShapeAt(coor)) {
        if(!shp->description.empty()) return shp->description.c_str();
    }
    // Fallback to base unit look description.
    return HasMudStrings::getLookDescription();
}

FlagHandler<RoomFlag>& AbstractGridArea::getRoomFlags(const Coordinates& coor)
{
    auto &t = ensure_tile(tileOverrides, coor);
    return t.roomFlags;
}

SectorType AbstractGridArea::getSectorType(const Coordinates &coor) const
{
    // This should only be called on a coordinate that definitely exists.

    if (auto t = find_tile(tileOverrides, coor))
    {
        if (t->sectorType)
            return *t->sectorType;
    }
    if(auto shp = topShapeAt(coor)) {
        return shp->sectorType;
    }
    return SectorType::inside; // mandated fallback
}

std::optional<std::string> AbstractGridArea::getTileDisplayOverride(const Coordinates &coor) const
{
    if (auto t = find_tile(tileOverrides, coor))
    {
        if(!t->tileDisplay.empty()) return t->tileDisplay;
    }
    if(auto shp = topShapeAt(coor); shp && !shp->tileDisplay.empty()) {
        return shp->tileDisplay;
    }
    return std::nullopt;
}

void AbstractGridArea::broadcastAt(const Coordinates &coor, const std::string &message)
{
    auto people = getPeople(coor);
    people.for_each([&](auto c) {c->sendText(message);});
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

void AbstractGridArea::replaceExit(const Coordinates &coor, const Destination &dest)
{
    auto &t = ensure_tile(tileOverrides, coor);
    auto d = dest;
    d.generated = false;
    t.exits[dest.dir] = std::move(d);
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

    t.sectorType = SectorType::inside;

    return true;
}

void AbstractGridArea::setString(const Coordinates& coor, const std::string& name, const std::string& value) {
    auto &t = ensure_tile(tileOverrides, coor);
    if (name == "name") t.name = value;
    else if (name == "look_description") t.look_description = value;
}

void AbstractGridArea::setResetCommands(const Coordinates& coor, const std::vector<ResetCommand>& cmds) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.resetCommands = cmds;
}

void AbstractGridArea::setSectorType(const Coordinates& coor, SectorType type) {
    auto &t = ensure_tile(tileOverrides, coor);
    t.sectorType = type;
}

std::vector<ResetCommand> AbstractGridArea::getResetCommands(const Coordinates& coor) {
    auto &t = ensure_tile(tileOverrides, coor);
    return t.resetCommands;
}

void to_json(json &j, const ShapeBase &p)
{
    j["type"] = p.type;
    j["priority"] = p.priority;
    j["sectorType"] = p.sectorType;
    if(!p.name.empty()) j["name"] = p.name;
    if(!p.description.empty()) j["description"] = p.description;
    j["geom"] = std::visit([](auto const& g) {
        return json(g); // relies on to_json for BoxDim / RoundDim
    }, p.geom);
    if(!p.tileDisplay.empty()) j["tileDisplay"] = p.tileDisplay;
}

void from_json(const json &j, ShapeBase &r) {
    if(j.contains(+"type")) r.type = j["type"].get<ShapeType>();
    if(j.contains(+"priority")) r.priority = j["priority"].get<int>();
    if(j.contains(+"sectorType")) r.sectorType = j["sectorType"].get<SectorType>();
    if(j.contains(+"name")) r.name = j["name"].get<std::string>();
    if(j.contains(+"description")) r.description = j["description"].get<std::string>();
    if (j.contains(+"geom")) {
        const auto& g = j.at(+"geom");
        switch (r.type) { // <- r.type (not p.type)
            case ShapeType::Box:
                r.geom = g.get<BoxDim>();
                break;
            case ShapeType::Round:
                r.geom = g.get<RoundDim>();
                break;
        }
    } else {
        // optional: default the variant based on type
        switch (r.type) {
            case ShapeType::Box:   r.geom = BoxDim{};   break;
            case ShapeType::Round: r.geom = RoundDim{}; break;
        }
    }
    if(j.contains(+"tileDisplay")) r.tileDisplay = j["tileDisplay"];
}

void to_json(json &j, const Shape &p) {
    to_json(j, static_cast<const ShapeBase&>(p));
}

void from_json(const json &j, Shape &r) {
    from_json(j, static_cast<ShapeBase&>(r));
}
