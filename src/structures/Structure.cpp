#include "dbat/structs.h"

Structure::Structure(const nlohmann::json& j) {
    deserialize(j);
}

nlohmann::json Structure::serialize() {
    return GameEntity::serialize();
}

void Structure::deserialize(const nlohmann::json& j) {
    GameEntity::deserialize(j);
}

std::string Structure::getUnitClass() {
    return "Structure";
}

nlohmann::json Structure::serializeRelations() {
    return GameEntity::serializeRelations();
}

void Structure::deserializeRelations(const nlohmann::json& j) {
    GameEntity::deserializeRelations(j);
}

bool Structure::isEnvironment() {
    return true;
}

bool Structure::isStructure() {
    return true;
}

void Structure::handleRemove(GameEntity *u) {
    
    GameEntity::handleRemove(u);
}

void Structure::handleAdd(GameEntity *u) {
    
    GameEntity::handleAdd(u);
}

void Structure::updateCoordinates(GameEntity *mover, std::optional<coordinates> previous) {
        
    GameEntity::updateCoordinates(mover, previous);
}

std::vector<std::pair<std::string, Destination>> Structure::getLandingSpotsFor(GameEntity *mover) {
    return {};
}

std::optional<Destination> Structure::getLaunchDestinationFor(GameEntity *mover) {
    return {};
}

bool Structure::isInsideNormallyDark(GameEntity *viewer) {
    return false;
}

bool Structure::isInsideDark(GameEntity *viewer) {
    return false;
}

std::string Structure::renderLocationFor(GameEntity* viewer) {
    return "";
}

UnitFamily Structure::getFamily() {
    return UnitFamily::Structure;
}

std::map<int, Destination> Structure::getDestinations(GameEntity *mover) {
    return {};
}

bool Structure::checkCanLeave(GameEntity *mover, const Destination& dest, bool need_specials_check) {
    return false;
}

bool Structure::checkCanReachDestination(GameEntity *mover, const Destination& dest) {
    return false;
}

bool Structure::checkPostEnter(GameEntity *mover, const Location& cameFrom, const Destination& dest) {
    return false;
}