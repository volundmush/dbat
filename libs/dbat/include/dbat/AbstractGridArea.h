#pragma once
#include <variant>

#include "AbstractLocation.h"
#include "HasMisc.h"
#include "HasMudStrings.h"

struct TileOverride : public HasResetCommands {
    explicit operator bool() const;
    std::string name;
    std::string look_description;
    std::optional<SectorType> sectorType;
    FlagHandler<RoomFlag> roomFlags;
    FlagHandler<WhereFlag> whereFlags;
    int damage{0};
    int groundEffect{0};
    std::map<Direction, Destination> exits;
    // an override to display tiles differently than SectorType
    std::string tileDisplay{};
};

enum class ShapeType : uint8_t {
    Box = 0,
    Round = 1
};

struct AABB {
    Coordinates min; // inclusive
    Coordinates max; // inclusive
    bool contains(const Coordinates& c) const noexcept {
        return (c.x >= min.x && c.x <= max.x &&
                c.y >= min.y && c.y <= max.y &&
                c.z >= min.z && c.z <= max.z);
    }
};

// ---------- Box geometry ----------
struct BoxDim {
    AABB box;

    static BoxDim fromCorners(Coordinates a, Coordinates b) noexcept {
        Coordinates lo{ std::min(a.x,b.x), std::min(a.y,b.y), std::min(a.z,b.z) };
        Coordinates hi{ std::max(a.x,b.x), std::max(a.y,b.y), std::max(a.z,b.z) };
        return BoxDim{ AABB{ lo, hi } };
    }

    // size is a count of tiles (>=1). Even sizes extend “more” toward + side; consistent > symmetric.
    static BoxDim fromCenter(Coordinates c, int sx, int sy, int sz = 1) noexcept {
        auto span = [](int c, int s){ int min = c - s/2; return std::pair{min, min + s - 1}; };
        auto [minx,maxx] = span(c.x, sx);
        auto [miny,maxy] = span(c.y, sy);
        auto [minz,maxz] = span(c.z, sz);
        return BoxDim{ AABB{ {minx,miny,minz}, {maxx,maxy,maxz} } };
    }

    bool contains(const Coordinates& c) const noexcept {
        return box.contains(c);
    }
};

// ---------- Round geometry (disk/cylinder in Z) ----------
struct RoundDim {
    Coordinates center;
    int radius = 0;   // tiles
    int zMin = 0;     // inclusive
    int zMax = 0;     // inclusive
    int r2  = 0;      // cached radius^2

    static RoundDim disk(Coordinates c, int r, int zMin, int zMax) noexcept {
        RoundDim d{c, r, zMin, zMax, r*r};
        return d;
    }

    AABB bounds() const noexcept {
        return AABB{
            { center.x - radius, center.y - radius, zMin },
            { center.x + radius, center.y + radius, zMax }
        };
    }

    bool contains(const Coordinates& c) const noexcept {
        if (c.z < zMin || c.z > zMax) return false;
        auto dx = c.x - center.x;
        auto dy = c.y - center.y;
        return (dx*dx + dy*dy) <= r2;
    }
};

struct Shape;

// the serializable data for shapes.
struct ShapeBase {
    ShapeBase& operator=(const Shape& other);
    ShapeType type{ShapeType::Box};
    int priority{0};
    SectorType sectorType{SectorType::inside};
    std::string name{};
    std::string description{};
    std::variant<BoxDim, RoundDim> geom;
    // an override to display tiles differently.
    std::string tileDisplay{};
};

struct AbstractGridArea;

// This is used for defining "areas" for re-use.
// We can both create Areas/Structures from them, and save one to them.
struct GridTemplate : public HasMudStrings, public HasVnum {
    // used for defining grid templates.
    GridTemplate& operator=(const AbstractGridArea& other);
    using HasMudStrings::getName;
    using HasMudStrings::getLookDescription;

    std::unordered_map<std::string, ShapeBase> shapes;
    std::unordered_map<Coordinates, TileOverride> tileOverrides;
};

// The shape that's used for instances.
struct Shape : public ShapeBase {
    using ShapeBase::operator=;
    Shape() = default;
    explicit Shape(const ShapeBase& b);
    int seq{0};

    // Fast reject
    AABB aabb() const noexcept { return cachedAabb; }
    bool contains(const Coordinates& c) const noexcept;
    void reComputeAABB();
    AABB cachedAabb;      // filled on add/update
};

struct BucketKey {
    int bx, by, bz;
    bool operator==(const BucketKey& o) const noexcept {
        return bx==o.bx && by==o.by && bz==o.bz;
    }
};

namespace std {
    template<>
    struct hash<BucketKey> {
        std::size_t operator()(const BucketKey& coord) const noexcept {
            // decent 3D hash
            uint64_t h = 1469598103934665603ull;
            auto mix=[&](int v){ h ^= uint64_t(uint32_t(v)); h *= 1099511628211ull; };
            mix(coord.bx); mix(coord.by); mix(coord.bz); return size_t(h);
        }
    };
}


struct AbstractGridArea : public HasMudStrings, public AbstractLocation, public HasSubscriptions {
    AbstractGridArea& operator=(const GridTemplate& other);
    using HasMudStrings::getName;
    using HasMudStrings::getLookDescription;

    std::unordered_map<std::string, std::unique_ptr<Shape>> shapes;
    mutable std::unordered_map<Coordinates, TileOverride> tileOverrides;

    std::vector<Shape*> byPriority;
    
    int bucketSize = 32; // tiles per bucket edge
    std::unordered_map<BucketKey, std::vector<Shape*>> buckets;

    // --- Bookkeeping for stable tie-break
    uint64_t nextSeq = 1; // incremented on add
    void rebuildShapeIndex();

    Shape* topShapeAt(const Coordinates& coor) const;

    // Nearly-complete AbstractLocation implementation on AbstractGridArea. Child classes still need further
    // specialization.

    bool validCoordinates(const Coordinates& coor) const override;
    ExtraDescriptionViews getExtraDescription(const Coordinates& coor) const override;
    const char* getName(const Coordinates& coor) const override;
    const char* getLookDescription(const Coordinates& coor) const override;
    std::optional<Destination> getDirection(const Coordinates& coor, Direction dir) override;
    std::map<Direction, Destination> getDirections(const Coordinates& coor) override;
    FlagHandler<RoomFlag>& getRoomFlags(const Coordinates& coor) override;
    SectorType getSectorType(const Coordinates& coor) const override;
    std::optional<std::string> getTileDisplayOverride(const Coordinates& coor) const override;
    void broadcastAt(const Coordinates& coor, const std::string& message) override;
    int getDamage(const Coordinates& coor) const override;
    int setDamage(const Coordinates& coor, int amount) override;
    int modDamage(const Coordinates& coor, int amount) override;
    int getGroundEffect(const Coordinates& coor) const override;
    void setGroundEffect(const Coordinates& coor, int val) override;
    int modGroundEffect(const Coordinates& coor, int val) override;
    SpecialFunc getSpecialFunc(const Coordinates& coor) const override;

    void replaceExit(const Coordinates& coor, const Destination& dest) override;
    void deleteExit(const Coordinates& coor, Direction dir) override;

    bool buildwalk(const Coordinates& coor, Character* ch, Direction dir) override;

    void setString(const Coordinates& coor, const std::string& name, const std::string& value) override;
    void setResetCommands(const Coordinates& coor, const std::vector<ResetCommand>& cmds) override;
    std::vector<ResetCommand> getResetCommands(const Coordinates& coor) override;
    void setSectorType(const Coordinates& coor, SectorType type) override;

};

extern std::unordered_map<int64_t, std::shared_ptr<GridTemplate>> gridTemplates;