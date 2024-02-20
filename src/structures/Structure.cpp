#include "dbat/structs.h"

Structure::Structure(const nlohmann::json& j) {
    deserialize(j);
}

nlohmann::json Structure::serialize() {
    return Object::serialize();
}

void Structure::deserialize(const nlohmann::json& j) {
    Object::deserialize(j);
}

std::string Structure::getUnitClass() {
    return "Structure";
}

nlohmann::json Structure::serializeRelations() {
    return Object::serializeRelations();
}

void Structure::deserializeRelations(const nlohmann::json& j) {
    Object::deserializeRelations(j);
}

bool Structure::isEnvironment() {
    return true;
}

bool Structure::isStructure() {
    return true;
}