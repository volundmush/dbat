#include "dbat/structs.h"
#include "dbat/db.h"
#include "dbat/constants.h"

Room* Exit::getDestination() {
    return destination;
}

UnitFamily Exit::getFamily() {
    return UnitFamily::Exit;
}

std::string Exit::getUnitClass() {
    return "Exit";
}

nlohmann::json Exit::serialize() {
    auto j = GameEntity::serialize();

    if(key > 0) j["key"] = key;

    if(dclock) j["dclock"] = dclock;
    if(dchide) j["dchide"] = dchide;
    if(dcskill) j["dcskill"] = dcskill;
    if(dcmove) j["dcmove"] = dcmove;
    if(failsavetype) j["failsavetype"] = failsavetype;
    if(dcfailsave) j["dcfailsave"] = dcfailsave;

    return j;
}

void Exit::deserialize(const nlohmann::json& j) {
    GameEntity::deserialize(j);
    if(j.contains("key")) key = j["key"];
    if(j.contains("dclock")) dclock = j["dclock"];
    if(j.contains("dchide")) dchide = j["dchide"];
    if(j.contains("dcskill")) dcskill = j["dcskill"];
    if(j.contains("dcmove")) dcmove = j["dcmove"];
    if(j.contains("failsavetype")) failsavetype = j["failsavetype"];
    if(j.contains("dcfailsave")) dcfailsave = j["dcfailsave"];

}

nlohmann::json Exit::serializeRelations() {
    auto j = GameEntity::serializeRelations();
    if(destination) j["destination"] = destination->getUIDString();
    if(failroom) j["failroom"] = failroom->getUIDString();
    if(totalfailroom) j["totalfailroom"] = totalfailroom->getUIDString();
    return j;

}

void Exit::deserializeRelations(const nlohmann::json& j) {
    GameEntity::deserializeRelations(j);
    if(j.contains("destination")) {
        destination = dynamic_cast<Room*>(resolveUID(j["destination"].get<std::string>()));
    }
    if(j.contains("failroom")) {
        failroom = dynamic_cast<Room*>(resolveUID(j["failroom"].get<std::string>()));
    }
    if(j.contains("totalfailroom")) {
        totalfailroom = dynamic_cast<Room*>(resolveUID(j["totalfailroom"].get<std::string>()));
    }
}

Exit::Exit(const nlohmann::json &j) : Exit() {
    deserialize(j);
}


std::string Exit::getName() {
    return dirs[locationType];
}

std::vector<std::string> Exit::getKeywords(GameEntity *looker) {
    std::vector<std::string> out;
    out.emplace_back(getName());
    if(auto al = getAlias(); !al.empty()) out.emplace_back(al);
    return out;
}

