#pragma once
#include <memory>
#include <optional>
#include <map>

#include <fmt/format.h>
#include <fmt/printf.h>

#include "const/RoomFlag.h"
#include "const/WhereFlag.h"
#include "const/SectorType.h"

#include "Typedefs.h"
#include "Coordinates.h"
#include "Flags.h"
#include "Command.h"

#include "HasExtraDescriptions.h"

#include "HasResetCommands.h"

#include "Log.h"

struct Room;
struct Character;
struct AbstractLocation;
struct Zone;
struct Destination;
struct Structure;

struct Location {
    Location() = default;
    Location(room_vnum rv);
    Location(Room* room);
    Location(Character* ch);
    Location(const std::shared_ptr<AbstractLocation>& al, const Coordinates& pos = {});
    Location(const std::shared_ptr<Room>& room);
    Location(const std::string& txt);

    Location& operator=(room_vnum rv);
    Location& operator=(Room* room);
    Location& operator=(const std::shared_ptr<Room>& room);

    AbstractLocation* getLoc() const;

    mutable std::string locationID{};
    mutable Coordinates position{};
    mutable std::weak_ptr<AbstractLocation> al{};  // What unit contains this unit (room, area, char, obj)
    
    bool operator==(const Location& other) const;
    bool operator==(const room_vnum rv) const;
    bool operator==(const Room* room) const;
    bool operator==(const std::shared_ptr<Room>& room) const;

    std::string renderDiagnostics(Character *ch) const;
    void setString(const std::string& txt, const std::string& val);

    // Conversion to bool - returns true if location is valid. currently means it is a room.
    explicit operator bool() const;
 
    Zone* getZone() const;
    vnum getVnum() const;

    std::string getLocID() const;

    const char* getName() const;
    const char* getLookDescription() const; // New: coordinate-aware look description
    std::optional<Destination> getExit(Direction dir) const;
    std::map<Direction, Destination> getExits() const;

    ExtraDescriptionViews getExtraDescription() const; // Returns the extra description data.

    double getEnvironment(int type) const;
    double setEnvironment(int type, double value);
    double modEnvironment(int type, double value);
    void clearEnvironment(int type);

    void setRoomFlag(int flag, bool value = true);
    void setRoomFlag(RoomFlag flag, bool value = true);
    bool toggleRoomFlag(int flag);
    bool toggleRoomFlag(RoomFlag flag);
    bool getRoomFlag(int flag) const;
    bool getRoomFlag(RoomFlag flag) const;
    bool getWhereFlag(WhereFlag flag) const;
    FlagHandler<RoomFlag>& getRoomFlags();

    std::string getUID(bool active = false) const;

    void sendText(const std::string& message);
    
    template<typename... Args>
    void sendFmt(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            if(formatted_string.empty()) return;
            sendText(formatted_string);
        }
        catch(const fmt::format_error& e) {
            LERROR("SYSERR: Format error in Location::sendFmt: %s", e.what());
            LERROR("Template was: %s", format.data());
        }
    }

    template<typename... Args>
    size_t send_to(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
            if(formatted_string.empty()) return 0;
            sendText(formatted_string);
            return formatted_string.size();
        }
        catch(const fmt::format_error& e) {
            LERROR("SYSERR: Format error in Location::send_to: %s", e.what());
            LERROR("Template was: %s", format.data());
            return 0;
        }
    }

    std::vector<std::weak_ptr<Object>> getObjects() const;
    std::vector<std::weak_ptr<Character>> getPeople() const;
    std::vector<std::weak_ptr<Structure>> getStructures() const;

    int getDamage() const;
    void setDamage(int amount);
    int modDamage(int amount);

    SectorType getSectorType() const;
    std::optional<std::string> getTileDisplayOverride() const;
    void setSectorType(SectorType type);
    int getTileType() const;

    int getGroundEffect() const;
    void setGroundEffect(int val);
    int modGroundEffect(int val);

    SpecialFunc getSpecialFunc();

    bool getIsDark();
    int getCookElement();

    void traverseObjects(const std::function<void(Object*)> &func, bool recurse = true);
    struct Object* searchObjects(const std::function<bool(Object*)> &func, bool recurse = true);
    struct Object* searchObjects(obj_vnum vnum, bool working = true, bool recurse = true);
    std::unordered_set<struct Object*> gatherFromObjects(const std::function<bool(Object*)> &func, bool recurse = true);

    int countPlayers();
    bool canGo(int dir);

    // location editing
    void replaceExit(const Destination& dest);
    void deleteExit(Direction dir);

    std::vector<ResetCommand> getResetCommands() const;
    void setResetCommands(const std::vector<ResetCommand>& cmds);

    void executeResetCommands(const std::vector<ResetCommand>& cmds);

    void displayLookFor(Character* viewer);

    bool buildwalk(Character* ch, Direction dir);

    Location getLaunchDestination();
    Zone* getLandZone();
};

inline std::string format_as(const Location& loc) {
    return fmt::format("{}", loc.getLocID());
}


namespace std {
    template<>
    struct hash<Location> {
        std::size_t operator()(const Location& loc) const noexcept;
    };
}

extern Location find_target_location(Character *ch, char *rawroomstr);