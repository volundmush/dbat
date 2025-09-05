#include "dbat/Area.h"

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
