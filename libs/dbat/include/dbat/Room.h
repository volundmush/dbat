#pragma once
#include "Log.h"
#include "AbstractLocation.h"
#include "HasDgScripts.h"
#include "HasZone.h"
#include "HasMudStrings.h"
#include "StatHandler.h"

struct Room;

extern StatHandler<Room> roomStats;

struct Room : public AbstractLocation, public HasZone, public HasDgScripts, public HasMudStrings, public HasExtraDescriptions, public HasSubscriptions, public HasResetCommands, std::enable_shared_from_this<Room> {
    static std::unordered_map<int, std::shared_ptr<Room>> registry;
    
    Room();

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
    std::vector<trig_vnum> proto_script; /* list of default triggers  */
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
    const std::vector<ExtraDescription>& getExtraDescription(const Coordinates& coor) const override;
    const char* getName(const Coordinates& coor) const override;
    const char* getLookDescription(const Coordinates& coor) const override;
    std::optional<Destination> getDirection(const Coordinates& coor, Direction dir) override;
    std::map<Direction, Destination> getDirections(const Coordinates& coor) override;

    FlagHandler<RoomFlag>& getRoomFlags(const Coordinates& coor) override;
    FlagHandler<WhereFlag>& getWhereFlags(const Coordinates& coor) override;

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

template <>
struct fmt::formatter<Room> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Room& z, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "Room {} '{}'", z.getVnum(), z.getName());
    }
};

extern bool WHERE_FLAGGED(room_vnum loc, WhereFlag flag);
extern bool WHERE_FLAGGED(Room *loc, WhereFlag flag);
extern bool ROOM_FLAGGED(room_vnum loc, int flag);
extern bool ROOM_FLAGGED(Room *loc, int flag);

#define SECT(room)    (VALID_ROOM_RNUM(room) ? \
                get_room((room))->sector_type : SECT_INSIDE)
#define ROOM_DAMAGE(room)   (get_room((room))->getDamage())
#define ROOM_EFFECT(room)   (get_room((room))->geffect)
#define ROOM_GRAVITY(room)  (get_room((room))->getGravity())
#define SUNKEN(room)    (ROOM_EFFECT(room) < 0 || SECT(room) == SECT_UNDERWATER)

#define IS_DARK(room)    room_is_dark((room))
#define IS_LIGHT(room)  (!IS_DARK(room))

#define VALID_ROOM_RNUM(rnum)    (Room::registry.contains(rnum) > 0 && rnum != NOWHERE)
#define GET_ROOM_VNUM(rnum) (VALID_ROOM_RNUM(rnum) ? (rnum) : NOWHERE)
#define GET_ROOM_SPEC(room) \
    (VALID_ROOM_RNUM(room) ? get_room((room))->func : nullptr)

    /* Minor Planet Defines */
#define PLANET_ZENITH(room) ((GET_ROOM_VNUM(room) >= 3400 && GET_ROOM_VNUM(room) <= 3599) || (GET_ROOM_VNUM(room) >= 62900 && GET_ROOM_VNUM(room) <= 62999) || \
                (GET_ROOM_VNUM(room) == 19600))

#define ROOM_FLAGS(loc)    (Room::registry.at((loc)).room_flags)

extern const char* sense_location_name(room_vnum roomnum);

extern Room *get_room(room_vnum vn);

extern SubscriptionManager<Room> roomSubscriptions;

extern void repairRoomDamage(uint64_t heartPulse, double deltaTime);

extern room_rnum real_room(room_vnum vnum);