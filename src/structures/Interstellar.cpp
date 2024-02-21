#include "dbat/structs.h"

Interstellar::Interstellar(const nlohmann::json& j) {
    deserialize(j);
}

std::string Interstellar::getUnitClass() {
    return "Interstellar";
}

