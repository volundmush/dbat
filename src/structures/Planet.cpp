#include "dbat/structs.h"

Planet::Planet(const nlohmann::json& j) {
    deserialize(j);
}

std::string Planet::getUnitClass() {
    return "Planet";
}

bool Planet::isPlanet() {
    return true;
}

bool Planet::isStructure() {
    return true;
}

bool Planet::isEnvironment() {
    return true;
}

bool Planet::isWorld() {
    return true;
}