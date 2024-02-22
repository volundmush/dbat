#include "dbat/structs.h"

Dimension::Dimension(const nlohmann::json& j) {
    deserialize(j);
}

std::string Dimension::getUnitClass() {
    return "Dimension";
}

bool Dimension::isWorld() {
    return true;
}

bool Dimension::isEnvironment() {
    return true;
}