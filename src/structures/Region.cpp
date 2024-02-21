#include "dbat/structs.h"

Region::Region(const nlohmann::json& j) {
    deserialize(j);
}

std::string Region::getUnitClass() {
    return "Region";
}

