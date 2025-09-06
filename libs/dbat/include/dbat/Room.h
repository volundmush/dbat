#pragma once
#include "Log.h"
#include "AbstractLocation.h"
#include "HasDgScripts.h"
#include "HasZone.h"
#include "HasMudStrings.h"
#include "StatHandler.h"
#include "HasMisc.h"

struct Room;

extern StatHandler<Room> roomStats;

struct Room : public AbstractLocation, public HasProtoScript, public HasVnum, public HasZone, public HasDgScripts, public HasMudStrings, public HasExtraDescriptions, public HasSubscriptions, public HasResetCommands, std::enable_shared_from_this<Room> {
    static std::unordered_map<int, std::shared_ptr<Room>> registry;
    
    Room();
    
    vnum getDgVnum() const override;
    UnitType getDgUnitType() const override;

    vnum getLocVnum() const override;
    std::string getLocID() const override;

    std::shared_ptr<AbstractLocation> getSharedAbstractLocation() override;

    using HasMudStrings::getName;
    using HasMudStrings::getLookDescription;

    SectorType sector_type{SectorType::inside};            /* sector type (move/hide)            */
    std::map<Direction, Destination> exits{}; /* Directions */
    FlagHandler<RoomFlag> room_flags{};   /* DEATH,DARK ... etc */
    FlagHandler<WhereFlag> where_flags{};
    SpecialFunc func{};

    const char* getDgName() const override;
    std::vector<trig_vnum> getProtoScript() const override;

    int damage{};                     /* How damaged the room is            */
    int ground_effect{};            /* Effect of ground destruction       */
    
    bool isActive() const override;
    void activate();
    void deactivate();
    std::string getUID(bool active) const override;

    int getDamage() const;
    int setDamage(int amount);
    int modDamage(int amount);

    void sendText(const std::string& txt);

    template<typename... Args>
    void sendFmt(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            if(formatted_string.empty()) return;
            sendText(formatted_string);
        }
        catch(const fmt::format_error& e) {
            LERROR("SYSERR: Format error in Room::sendFmt: %s", e.what());
            LERROR("Template was: %s", format.data());
        }
    }

    template<typename... Args>
    size_t send_to(fmt::string_view format, Args&&... args) {
        try {
            // Use fmt::sprintf directly (printf-style).
            std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
            if(formatted_string.empty()) return 0;
            sendText(formatted_string);
            return formatted_string.size();
        }
        catch(const fmt::format_error& e) {
            LERROR("SYSERR: Format error in Room::send_to: %s", e.what());
            LERROR("Template was: %s", format.data());
            return 0;
        }
    }
    
    void deleteExit(Direction dir);
    void replaceExit(const Destination& dest);

    std::optional<std::string> dgCallMember(const std::string& member, const std::string& arg) override;

    template<typename R = double>
    R getBaseStat(const std::string& stat) {
        return roomStats.getBase<R>(this, stat);
    }

    template<typename R = double>
    R setBaseStat(const std::string& stat, double val) {
        return roomStats.setBase<R>(this, stat, val);
    }

    template<typename R = double>
    R modBaseStat(const std::string& stat, double val) {
        return roomStats.modBase<R>(this, stat, val);
    }

    template<typename R = double>
    R gainBaseStat(const std::string& stat, double val, bool applyBonuses = true) {
        return roomStats.gainBase<R>(this, stat, val, applyBonuses);
    }

    template<typename R = double>
    R gainBaseStatPercent(const std::string& stat, double percent, bool applyBonuses = true) {
        return roomStats.gainBasePercent<R>(this, stat, percent, applyBonuses);
    }

    template<typename R = double>
    R getEffectiveStat(const std::string& stat) {
        return roomStats.getEffective<R>(this, stat);
    }

    std::optional<Destination> getDirection(Direction dir) const;
    std::map<Direction, Destination> getDirections() const;

    // overrides for location_data...
    Zone* getLocZone() const override;
    ExtraDescriptionViews getExtraDescription(const Coordinates& coor) const override;
    const char* getName(const Coordinates& coor) const override;
    const char* getLookDescription(const Coordinates& coor) const override;
    std::optional<Destination> getDirection(const Coordinates& coor, Direction dir) override;
    std::map<Direction, Destination> getDirections(const Coordinates& coor) override;

    FlagHandler<RoomFlag>& getRoomFlags(const Coordinates& coor) override;

    SectorType getSectorType(const Coordinates& coor) const override;
    void broadcastAt(const Coordinates& coor, const std::string& message) override;
    int getDamage(const Coordinates& coor) const override;
    int setDamage(const Coordinates& coor, int amount) override;
    int modDamage(const Coordinates& coor, int amount) override;
    int getGroundEffect(const Coordinates& coor) const override;
    void setGroundEffect(const Coordinates& coor, int val) override;
    int modGroundEffect(const Coordinates& coor, int val) override;
    SpecialFunc getSpecialFunc(const Coordinates& coor) const override;

    std::optional<double> getEnvironment(const Coordinates& coor, int type) const override;

    void replaceExit(const Coordinates& coor, const Destination& dest) override;
    void deleteExit(const Coordinates& coor, Direction dir) override;

    bool buildwalk(const Coordinates& coor, Character* ch, Direction dir) override;

    void setString(const Coordinates& coor, const std::string& name, const std::string& value) override;
    void setResetCommands(const Coordinates& coor, const std::vector<ResetCommand>& cmds) override;
    std::vector<ResetCommand> getResetCommands(const Coordinates& coor) override;
    void setSectorType(const Coordinates& coor, SectorType type) override;

};

inline std::string format_as(const Room& r) {
    return fmt::format("Room {} '{}'", r.getVnum(), r.getName());
}

extern SubscriptionManager<Room> roomSubscriptions;