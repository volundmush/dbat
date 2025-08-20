#include "dbat/Area.h"

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
