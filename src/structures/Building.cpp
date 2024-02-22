#include "dbat/structs.h"

Building::Building(const nlohmann::json& j) {
    deserialize(j);
}

std::string Building::getUnitClass() {
    return "Building";
}

bool Building::isStructure() {
    return true;
}

bool Building::isEnvironment() {
    return true;
}