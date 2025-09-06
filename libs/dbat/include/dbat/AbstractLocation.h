#pragma once
#include <map>
#include <optional>

#include "const/RoomFlag.h"
#include "const/WhereFlag.h"

#include "Flags.h"
#include "Command.h"

#include "WeakBag.h"
#include "Coordinates.h"
#include "HasExtraDescriptions.h"
#include "HasResetCommands.h"
#include "Destination.h"

struct HasLocation;
struct Zone;
struct Structure;

struct AbstractLocation {
    WeakBag<HasLocation> contents;

    template<typename T>
    auto getContents() {
        WeakBag<T> result;
        contents.for_each_shared([&](const auto& hl) {
            if(auto item = std::dynamic_pointer_cast<T>(hl)) {
                if (item->isActiveInLocation()) {
                    result.add(item);
                }
            }
        });
        return result;
    }

    template<typename T>
    WeakBag<T> getContents(const Coordinates& coor) {
        WeakBag<T> result;
        contents.for_each_shared([&](const auto& ptr) {
            if(auto item = std::dynamic_pointer_cast<T>(ptr)) {
                if (item->isActiveInLocation() && item->location.position == coor) {
                    result.add(item);
                }
            }
        });
        return result;
    }

    virtual std::shared_ptr<AbstractLocation> getSharedAbstractLocation() = 0;

    virtual std::string getLocID() const = 0;
    virtual vnum getLocVnum() const = 0;
    virtual Zone* getLocZone() const = 0;

    virtual const char* getName(const Coordinates& coor) const = 0;
    virtual const char* getLookDescription(const Coordinates& coor) const = 0; // New
    virtual bool getIsDark(const Coordinates& coor);

    virtual ExtraDescriptionViews getExtraDescription(const Coordinates& coor) const;

    WeakBag<Object> getObjects();
    WeakBag<Character> getPeople();
    WeakBag<Structure> getStructures();

    virtual WeakBag<Structure> getStructures(const Coordinates& coor);
    virtual WeakBag<Object> getObjects(const Coordinates& coor);
    virtual WeakBag<Character> getPeople(const Coordinates& coor);

    virtual std::optional<Destination> getDirection(const Coordinates& coor, Direction dir) = 0;
    virtual std::map<Direction, Destination> getDirections(const Coordinates& coor) = 0;

    virtual FlagHandler<RoomFlag>& getRoomFlags(const Coordinates& coor) = 0;
    void setRoomFlag(const Coordinates& coor, RoomFlag flag, bool value = true);
    bool toggleRoomFlag(const Coordinates& coor, RoomFlag flag);
    bool getRoomFlag(const Coordinates& coor, RoomFlag flag);

    // these are overloadable but base implementations just call the RoomFlag variants.
    void setRoomFlag(const Coordinates& coor, int flag, bool value = true);
    bool toggleRoomFlag(const Coordinates& coor, int flag);
    bool getRoomFlag(const Coordinates& coor, int flag);

    virtual SectorType getSectorType(const Coordinates& coor) const = 0;

    virtual std::optional<std::string> getTileDisplayOverride(const Coordinates& coor) const;

    virtual void broadcastAt(const Coordinates& coor, const std::string& message) = 0;

    virtual int getDamage(const Coordinates& coor) const = 0;
    virtual int setDamage(const Coordinates& coor, int amount) = 0;
    virtual int modDamage(const Coordinates& coor, int amount) = 0;

    virtual int getGroundEffect(const Coordinates& coor) const = 0;
    virtual void setGroundEffect(const Coordinates& coor, int val) = 0;
    virtual int modGroundEffect(const Coordinates& coor, int val) = 0;

    virtual SpecialFunc getSpecialFunc(const Coordinates& coor) const = 0;

    virtual std::optional<double> getEnvironment(const Coordinates& coor, int type) const;

    int getCookElement(const Coordinates& coor);

    // tools for editing the location.
    virtual void replaceExit(const Coordinates& coor, const Destination& dest);
    virtual void deleteExit(const Coordinates& coor, Direction dir);

    void addToContents(const Coordinates& coor, const std::shared_ptr<HasLocation>& hl);
    void removeFromContents(const std::shared_ptr<HasLocation>& hl);

    virtual void onAddToContents(const Coordinates& coor, const std::shared_ptr<HasLocation>& hl);
    virtual void onRemoveFromContents(const std::shared_ptr<HasLocation>& hl);

    virtual bool validCoordinates(const Coordinates& coor) const;

    virtual bool buildwalk(const Coordinates& coor, Character* ch, Direction dir);

    virtual void setString(const Coordinates& coor, const std::string& name, const std::string& value) = 0;
    virtual void setSectorType(const Coordinates& coor, SectorType type) = 0;
    virtual std::vector<ResetCommand> getResetCommands(const Coordinates& coor) = 0;
    virtual void setResetCommands(const Coordinates& coor, const std::vector<ResetCommand>& cmds) = 0;
};