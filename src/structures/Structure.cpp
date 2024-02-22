#include "dbat/structs.h"
#include "dbat/utils.h"

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
    handleRemoveFromCoordinates(u, u->coords);
}

void Structure::handleAdd(GameEntity *u) {
    
    GameEntity::handleAdd(u);
}

void Structure::handleRemoveFromCoordinates(GameEntity *mover, const Coordinates& c) {
    auto &con = reg.get_or_emplace<CoordinateContents>(ent);
    if(auto found = con.coordinateContents.find(c); found != con.coordinateContents.end()) {
        auto &c = found->second;
        // erase mover from c...
        std::erase_if(c, [mover](auto& p) { return p == mover; });
        if(c.empty())
            con.coordinateContents.erase(found);
    }
}

void Structure::updateCoordinates(GameEntity *mover, std::optional<Coordinates> previous) {
    if(previous) {
        handleRemoveFromCoordinates(mover, previous.value());
    }
    auto &con = reg.get_or_emplace<CoordinateContents>(ent);
    auto &loc = reg.get<Location>(mover->ent);
    con.coordinateContents[loc.coords].push_back(mover);
}

std::vector<std::pair<std::string, Destination>> Structure::getLandingSpotsFor(GameEntity *mover) {
    std::vector<std::pair<std::string, Destination>> ret;

    switch(type) {
        case StructureType::Rooms:
        // this one entails a recursive search through all contained Rooms and Regions.
        for(auto o : getContents()) {
            if(auto r = dynamic_cast<Room*>(o)) {
                if(r->checkFlag(FlagType::Room, ROOM_LANDING)) {
                    ret.push_back({r->getName(), Destination{r}});
                }
            } else if(auto r = dynamic_cast<Region*>(o)) {
                auto res = r->getLandingSpotsFor(mover);
                ret.insert(ret.end(), res.begin(), res.end());
            }
        }
        break;
        case StructureType::Grid3D: {
            if(auto tiles = reg.try_get<Grid3D>(ent); tiles) {
                for(auto& [k, v] : tiles->tiles) {
                    if(v.flags.contains(ROOM_LANDING)) {
                        Destination dest;
                        dest.target = this;
                        dest.coords = k;
                        ret.push_back({withPlaceholder(v.name, getDisplayName(mover)), dest});
                    }
                }
            }
        }
        break;
        case StructureType::Space3D:
        break;
    }
    return ret;
}

std::optional<Destination> Structure::getLaunchDestinationFor(GameEntity *mover) {
    auto loc = reg.try_get<Location>(ent);
    if(loc && loc->location) return Destination(*loc);
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

EntityFamily Structure::getFamily() {
    return EntityFamily::Structure;
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