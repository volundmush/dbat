#include "dbat/structs.h"
#include "dbat/db.h"
#include "dbat/constants.h"

Room* Exit::getDestination() {
    return destination;
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
    if(auto loc = reg.try_get<Location>(ent); loc) return dirs[loc->locationType];
    return "Nowhere";
}

std::vector<std::string> Exit::getKeywords(GameEntity *looker) {
    std::vector<std::string> out;
    out.emplace_back(getName());
    if(auto al = getAlias(); !al.empty()) out.emplace_back(al);
    return out;
}

