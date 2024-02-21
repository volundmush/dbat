#include "dbat/structs.h"

Building::Building(const nlohmann::json& j) {
    deserialize(j);
}

std::string Building::getUnitClass() {
    return "Building";
}

