#include "dbat/game/Structure.hpp"
#include "dbat/game/Character.hpp"

std::unordered_map<int64_t, std::shared_ptr<Structure>> Structure::registry;

std::string Structure::getLocID() const {
    return fmt::format("{}:{}", "S", id);
}

vnum Structure::getLocVnum() const {
    return NOTHING; // Structures do not have a vnum in the same way rooms do.
}

Zone* Structure::getLocZone() const {
    return location.getZone();
}

std::shared_ptr<AbstractLocation> Structure::getSharedAbstractLocation() {
    return shared_from_this();
}


bool Structure::isActiveInLocation() const {
    return true;
}

std::string Structure::getLocationDisplayCategory(Character* viewer) const {
    std::unordered_set<StructureFlag> moveFlags;
    for(auto f : {StructureFlag::space, StructureFlag::air, StructureFlag::ground, StructureFlag::water, StructureFlag::underwater}) {
        if(structure_flags.get(f)) {
            moveFlags.insert(f);
        }
    }

    if(moveFlags.size() == 1) {
        if(moveFlags.contains(StructureFlag::space)) {
            return "Spaceships";
        } else if(moveFlags.contains(StructureFlag::air)) {
            return "Airships";
        } else if(moveFlags.contains(StructureFlag::ground)) {
            return "Groundcraft";
        } else if(moveFlags.contains(StructureFlag::water)) {
            return "Watercraft";
        } else if(moveFlags.contains(StructureFlag::underwater)) {
            return "Submersibles";
        }
    } else if(moveFlags.size() > 1) {
        return "Multi-Environment Vehicles";
    }

    return "Structures";
}

void Structure::displayLocationInfo(Character* viewer) {
    viewer->sendFmt("{} is here.\r\n", getDisplayName(viewer));
}

std::shared_ptr<HasLocation> Structure::getSharedHasLocation() {
    return shared_from_this();
}

bool Structure::isAuthorized(Character *ch, bool ownerOnly) const {
    if(ownerOnly) {
        return owners.contains(ch->id);
    }
    return (owners.contains(ch->id)) || (users.contains(ch->id));
}

std::expected<Location, std::string> Structure::getDockingPortLocation(Structure *vehicle, Character *pilot) {
    return std::unexpected("not yet implemented");
}
