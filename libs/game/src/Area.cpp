#include "dbat/game/Area.hpp"
#include <nlohmann/json.hpp>

std::unordered_map<int64_t, std::shared_ptr<Area>> areas;

std::string Area::getLocID() const {
    return fmt::format("{}:{}", "A", getVnum());
}

vnum Area::getLocVnum() const {
    return vn; 
}

Zone* Area::getLocZone() const {
    return zone.get();
}

std::shared_ptr<AbstractLocation> Area::getSharedAbstractLocation() {
    return shared_from_this();
}

void to_json(nlohmann::json& j, const Area& p) {
    to_json(j, static_cast<const AbstractGridArea&>(p));
    to_json(j, static_cast<const HasVnum&>(p));
    to_json(j, static_cast<const HasZone&>(p));
}

void from_json(const nlohmann::json& j, Area& p) {
    from_json(j, static_cast<AbstractGridArea&>(p));
    from_json(j, static_cast<HasVnum&>(p));
    from_json(j, static_cast<HasZone&>(p));
}
