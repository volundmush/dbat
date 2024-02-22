#include "dbat/structs.h"

Stellar::Stellar(const nlohmann::json& j) {
    deserialize(j);
}

std::string Stellar::getUnitClass() {
    return "Stellar";
}

bool Stellar::isWorld() {
    return true;
}

bool Stellar::isEnvironment() {
    return true;
}