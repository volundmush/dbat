#include "dbat/structs.h"

Planet::Planet(const nlohmann::json& j) {
    deserialize(j);
}

std::string Planet::getUnitClass() {
    return "Planet";
}

