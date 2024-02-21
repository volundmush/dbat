#include "dbat/structs.h"

Stellar::Stellar(const nlohmann::json& j) {
    deserialize(j);
}

std::string Stellar::getUnitClass() {
    return "Stellar";
}

