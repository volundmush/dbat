#include "dbat/Structure.h"

std::string Structure::getLocID() const {
    return fmt::format("{}:{}", "S", id);
}

vnum Structure::getLocVnum() const {
    return NOTHING; // Structures do not have a vnum in the same way rooms do.
}

Zone* Structure::getLocZone() const {
    return location.getZone();
}

std::shared_ptr<AbstractLocation> Structure::getSharedAbstractLocation() {
    return shared_from_this();
}


bool Structure::isActiveInLocation() const {
    return true;
}

std::string Structure::getLocationDisplayCategory(Character* viewer) const {
    return "Structures";
}

void Structure::displayLocationInfo(Character* viewer) {
    // does nothing yet.
}

std::shared_ptr<HasLocation> Structure::getSharedHasLocation() {
    return shared_from_this();
}