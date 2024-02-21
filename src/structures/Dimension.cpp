#include "dbat/structs.h"

Dimension::Dimension(const nlohmann::json& j) {
    deserialize(j);
}

std::string Dimension::getUnitClass() {
    return "Dimension";
}

