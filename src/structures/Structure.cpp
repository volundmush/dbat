#include "dbat/structs.h"
#include "dbat/utils.h"

bool Structure::isEnvironment() {
    return true;
}

bool Structure::isStructure() {
    return true;
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
