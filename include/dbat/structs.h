/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and constants                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#pragma once
#include "dbat/templates.h"
#include "dbat/commands.h"

// IMPORTANT: Do not use data structures/fields that are not part of the
// C++ standard library. This allows us to keep things neat and clean for
// cross-compatability.

typedef struct assembly_data ASSEMBLY;
typedef struct component_data COMPONENT;



struct assembly_data {
    long lVnum;                  /* Vnum of the object assembled. */
    long lNumComponents;         /* Number of components. */
    unsigned char uchAssemblyType;        /* Type of assembly (ASSM_xxx).
*/
    struct component_data *pComponents;          /* Array of component info. */
};

/* Assembly component structure definition. */
struct component_data {
    bool bExtract;               /* Extract the object after use. */
    bool bInRoom;                /* Component in room, not inven. */
    long lVnum;                  /* Vnum of the component object. */
};

/**********************************************************************
* Structures                                                          *
**********************************************************************/

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
    char *keyword;                 /* Keyword in look/examine          */
    char *description;             /* What to see                      */
    struct extra_descr_data *next; /* Next in list                     */
};

// new variant of extra_descr_data that uses std::string
struct ExtraDescription {
    std::string keyword;          /* Keyword in look/examine          */
    std::string description;      /* What to see                      */
};

struct affect_t {
    // DO NOT CHANGE THE ORDER OF THESE FIELDS.
    explicit affect_t() = default;
    affect_t(int loc, double mod, int spec) : location(loc), modifier(mod), specific(spec) {};
    uint64_t location{0};
    double modifier{0.0};
    uint64_t specific{0};
    std::vector<std::string> specificNames();
    std::string locName();
    bool isBitwise();
    bool match(int loc, int spec);
    bool isPercent();
};

struct character_affect_type : affect_t {
    std::function<double(struct Character *ch)> func{};

    character_affect_type(int loc, double mod, int spec, std::function<double(struct Character *ch)> f = {})
            : affect_t{loc, mod, spec}, func{f} {}
};

/* An affect structure. */
struct affected_type : affect_t {
    using affect_t::affect_t;
    int16_t type{};          /* The type of spell that caused this      */
    int16_t duration{};      /* For how long its effects will last      */
    bitvector_t bitvector{}; /* Tells which bits to set (AFF_XXX) */
    struct affected_type *next{};
};

struct Account {
    Account() = default;
    int id{NOTHING};
    std::string name;
    std::string password;
    std::string email;
    time_t created{};
    time_t last_login{};
    time_t last_logout{};
    time_t last_change_password{};
    double playtime{};
    std::string disabled_reason;
    time_t disabled_until{0};
    int admin_level{};
    int rpp{};
    int slots{3};
    std::vector<std::string> customs;
    std::vector<int> characters;
    std::unordered_set<descriptor_data*> descriptors;
    // this is used by Cython.
    std::unordered_map<int64_t, std::string> connections;

    void modRPP(int amt);

    bool canBeDeleted();

    static int getNextID();

};

struct PlayerData {
    int id{NOTHING};
    std::string name;
    struct Account *account{};
    struct Character *character{};
    std::vector<struct alias_data> aliases;    /* Character's aliases                  */
    std::unordered_set<int> sense_player;
    std::unordered_set<mob_vnum> sense_memory;
    std::map<int, std::string> dub_names;
    char *color_choices[NUM_COLOR]{}; /* Choices for custom colors		*/
    struct txt_block *comm_hist[NUM_HIST]{}; /* Player's communications history     */
};

enum class ResetCommandType : uint8_t {
    MOB = 0,
    OBJ = 1,
    GIVE = 2,
    PUT = 3,
    GET = 4,
    EQUIP = 5,
    DOOR = 6,
    REMOVE = 7,
    TRIGGER = 8,
    VARIABLE = 9
};

struct SpawnRegistry {
    std::unordered_map<vnum, std::vector<std::weak_ptr<Character>>> mobiles;
    std::unordered_map<vnum, std::vector<std::weak_ptr<Object>>> objects;
    std::weak_ptr<Object> lastObj;
    std::weak_ptr<Character> lastChar;
};

struct ResetCommand {
    ResetCommandType type;  /* Type of reset command */
    bool if_flag{false};         /* If TRUE: execute only if preceding executed */
    int target{NOTHING}; // vnum for spawning mob/obj/trigger, direction for door
    // The maximum amount of things generated by this spawn rule. used for MOB/OBJ. 0 disables check.
    int max{0};            
    // for MOB/OBJ, will not spawn if x amount already in location. 0 disables check.
    // ALso used for T type with Room targets.
    int max_location{0};       
    int ex{0};             // Flags for D, or TriggerType for T/V. Equip slot for EQUIP
    int chance{100};             // Chance of this happening. Not always used.
    std::string key, value; // used for VARIABLE.

    bool executeAt(Location& loc, SpawnRegistry &reg) const;

    std::string print() const;
};

template <>
struct fmt::formatter<ResetCommand> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ResetCommand& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", z.print());
    }
};

struct HasResetCommands {
    std::vector<ResetCommand> resetCommands;
    std::string printResetCommands() const;
};

struct Zone {
    zone_vnum number{NOTHING};        /* virtual number of this zone	  */

    zone_vnum parent{NOTHING};
    std::unordered_set<zone_vnum> children;

    std::vector<Zone*> getChain();
    std::vector<Zone*> getAncestors() const;
    std::vector<Zone*> getDescendants() const;
    std::vector<Zone*> getChildren() const;
    Zone* getParent() const;

    void reset();

    std::string name{};            /* name of this zone                  */
    std::string colorName{};   // color name to display.
    std::string builders{};          /* namelist of builders allowed to    */
    /* modify this zone.		  */
    int lifespan{5};           /* how long between resets (minutes)  */
    double age{};                /* current age of this zone (seconds) */

    int reset_mode{2};         /* conditions for reset (see below)   */

    FlagHandler<ZoneFlag> zone_flags{};          /* Flags for the zone.                */

    /*
     * Reset mode:
     *   0: Don't reset, and don't update age.
     *   1: Reset if no PC's are located in zone.
     *   2: Just reset.
     */
    WeakBag<Room> rooms;
    WeakBag<Area> areas;
    WeakBag<Character> npcsInZone;
    WeakBag<Character> playersInZone;
    WeakBag<Object> objectsInZone;
    WeakBag<Structure> structuresInZone;

    void sendText(const std::string &txt);

    template<typename... Args>
    void sendFmt(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            if(formatted_string.empty()) return;
            sendText(formatted_string);
        }
        catch(const fmt::format_error& e) {
            basic_mud_log("SYSERR: Format error in Zone::sendFmt: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
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
            basic_mud_log("SYSERR: Format error in Zone::send_to: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
            return 0;
        }
    }

    Result<bool> canBeDeletedBy(Character* ch);

    // if a room in this or a descendant zone is OUTDOORS, the "fly space"
    // or "pilot launch" commands will take you here.
    // Stores a LocID like R:50 or A:10:0:0:9
    // the actor will look up the zone chain until it either finds a
    // launchDestination or runs out of Zones to check.
    std::string launchDestination{};

    // Landing spots for the zone. If you're IN launchDestination...
    // Then these are a combination of name->LocID landing spots
    // to display.
    std::unordered_map<std::string, std::string> landingSpots;
    // Specifically for ships landing.
    std::unordered_map<std::string, std::string> dockingSpots;
};

template <>
struct fmt::formatter<Zone> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Zone& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "Zone {} '{}'", z.number, z.name);
    }
};


struct HasVariables {
    std::unordered_map<std::string, std::string> variables; // Subscriptions to services.

    std::optional<std::string> getVariable(const std::string &key) const {
        if(auto it = variables.find(key); it != variables.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void setVariable(const std::string &key, const std::string &value) {
        variables[key] = value;
    }

    template<typename T>
    requires (!std::is_convertible_v<T, const char*>)
    void setVariable(const std::string &key, T u) {
        variables[key] = u->getUID(true);
    }

    bool hasVariable(const std::string &key) const {
        return variables.find(key) != variables.end();
    }

    bool eraseVariable(const std::string &key) {
        return variables.erase(key) > 0;
    }
};

enum class ScriptLineType : uint8_t {
    COMMAND = 0,
    IF = 1,
    ELSEIF = 2,
    ELSE = 3,
    END = 4,
    SWITCH = 5,
    CASE = 6,
    BREAK = 7,
    DEFAULT = 8,
    WHILE = 9,
    DONE = 10,
    COMMENT = 11
};

using ScriptLine = std::tuple<ScriptLineType, std::string>;

struct DgScriptPrototype {
    trig_vnum vn{NOTHING};
    UnitType attach_type{UnitType::unknown};            /* mob/obj/wld intentions          */
    std::string name{};                    /* name of trigger                 */
    long trigger_type{};            /* type of trigger (for bitvector) */
    std::vector<ScriptLine> lines; /* list of commands in trigger     */
    int narg{};                /* numerical argument              */
    std::string arglist{};            /* argument list                   */

    ScriptLine getLine(int line) const;

    std::string scriptString() const;
    void setBody(const std::string& body);
};

template <>
struct fmt::formatter<DgScriptPrototype> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const DgScriptPrototype& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "({}) DgScript {} '{}'", z.attach_type, z.vn, z.name);
    }
};

using DepthType = std::tuple<ScriptLineType, int, bool, std::string>;

enum class DgScriptState : uint8_t {
    READY = 0,
    RUNNING = 1,
    WAITING = 2,
    PAUSED = 3,
    ERROR = 4,
    DONE = 5
};

class DgScriptError : public std::runtime_error {
public:
    explicit DgScriptError(const std::string& message)
        : std::runtime_error(message) {}
};

struct HasSubscriptions {
    std::unordered_set<std::string> subscriptions{}; // Subscriptions to services.
};

/* structure for triggers */
struct DgScript : public HasVariables, public HasSubscriptions, std::enable_shared_from_this<DgScript> {
    DgScript() = default;
    DgScript(const DgScriptPrototype &other);
    DgScriptPrototype* proto{};
    int getVnum() const;
    UnitType getAttachType() const;
    long getTriggerType() const;
    DgScriptState state{DgScriptState::READY}; /* current state of the script */
    std::vector<DepthType> depth_stack{};
    int current_line{};
    double waiting{0.0};    /* event to pause the trigger      */
    std::experimental::observer_ptr<HasDgScripts> owner{};

    bool active{false};
    void activate();
    void deactivate();

    int execute();
    void reset();

    bool isReady() const;
    void setWaiting(double wait, DgScriptState newState = DgScriptState::WAITING);

private:
    void error(const std::string& message);
    int toReturn{1}; // used to return from the script.
    
    void setState(DgScriptState newState);
    
    void processLine(const ScriptLine& line);

    int locateElseIfElseEnd(int startLine) const;
    int locateCaseDefaultDone(int startLine) const;
    int locateDone(ScriptLineType type, int startLine) const;
    int locateEnd(int startLine) const;

    std::string evaluateExpression(const std::string& expr);
    bool evaluateComparison(const std::string& lhs, const std::string& rhs, const std::string& op);
    void processCommand(const std::string& cmd);
    bool truthy(const std::string& value) const;
    std::string substituteVariables(const std::string& cmd);
};

template <>
struct fmt::formatter<DgScript> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const DgScript& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "({}) DgScript {} '{}'", z.getAttachType(), z.getVnum(), z.proto->name);
    }
};


struct picky_data {
    std::unordered_set<MoralAlign> only_alignment, not_alignment;    /* Neutral, lawful, etc.		*/
    std::unordered_set<Sensei> only_sensei, not_sensei;    /* Only these classes can shop here	*/
    std::unordered_set<Race> only_race, not_race;    /* Only these races can shop here	*/
};

struct Coordinates {
    int32_t x{0}, y{0}, z{0};

    bool operator==(const Coordinates& other) const;
    explicit operator bool() const;

    void apply(Direction dir);
    Coordinates get_direction_offset(Direction dir);
};

template <>
struct fmt::formatter<Coordinates> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Coordinates& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}:{}:{}", z.x, z.y, z.z);
    }
};

struct Location {
    Location() = default;
    Location(room_vnum rv);
    Location(Room* room);
    Location(Character* ch);
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

    const std::vector<ExtraDescription>& getExtraDescription() const; // Returns the extra description data.

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
    void setWhereFlag(WhereFlag flag, bool value = true);
    bool toggleWhereFlag(WhereFlag flag);
    bool getWhereFlag(WhereFlag flag) const;
    FlagHandler<RoomFlag>& getRoomFlags();
    FlagHandler<WhereFlag>& getWhereFlags();

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
            basic_mud_log("SYSERR: Format error in Location::sendFmt: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
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
            basic_mud_log("SYSERR: Format error in Location::send_to: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
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
};

template <>
struct fmt::formatter<Location> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Location& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", z.getLocID());
    }
};

namespace std {
    template<>
    struct hash<Coordinates> {
        std::size_t operator()(const Coordinates& coord) const noexcept;
    };

    template<>
    struct hash<Location> {
        std::size_t operator()(const Location& loc) const noexcept;
    };
}

struct HasVnum {
    vnum vn{NOTHING};
    vnum getVnum() const;
};

struct HasDgScripts : public HasVariables, public HasVnum {

    virtual const char* getDgName() const = 0;

    UnitType type{UnitType::unknown};

    virtual bool isActive() const = 0;

    long trigger_types{};   /* bitvector of trigger types */
    std::optional<std::vector<vnum>> running_scripts; /* list of attached scripts. the order matters. Only used if differs from proto scripts.*/
    std::unordered_map<trig_vnum, std::shared_ptr<DgScript>> scripts; /* list of attached triggers. accessed in order of running_scripts */

    void activateScripts();
    void deactivateScripts();
    std::vector<trig_vnum> getScriptOrder(); /* this will return running_scripts if said, or the results of getProtoScripts() */
    std::vector<std::weak_ptr<DgScript>> getScripts();
    virtual std::vector<trig_vnum> getProtoScript() const = 0;
    std::string scriptString() const;

    // generates the persistent UID which will identify this object when it's saved
    // to a variable and which hopefully survives a reboot.
    virtual std::string getUID(bool active = false) const = 0;

    virtual std::optional<std::string> dgCallMember(const std::string& member, const std::string& arg);

};

struct HasMudStrings {
    const char* getName() const;
    const char* getRoomDescription() const;
    const char* getLookDescription() const;
    const char* getShortDescription() const;
    std::string_view getString(const std::string &key) const; // Returns a string from the strings map.
    std::unordered_map<std::string, std::string> strings;
};

struct HasExtraDescriptions {
    const std::vector<ExtraDescription>& getExtraDescription() const; // Returns the extra description data.    
    std::vector<ExtraDescription> extra_descriptions; // Extra descriptions for this unit.
};

// Character, Object, and perhaps later some new vehicles use HasLocation.
struct HasLocation {
    Location location;

    std::unordered_map<std::string, Location> registeredLocations;

    struct Room* getRoom();
    room_vnum getRoomVnum();

    virtual bool isActiveInLocation() const = 0;

    virtual std::shared_ptr<HasLocation> getSharedHasLocation() = 0;

    void moveToLocation(const Location& loc);
    void leaveLocation();
    void updateLocation();
    
    // virtual hooks
    virtual void onMoveToLocation(const Location& loc);
    virtual void onLeaveLocation(const Location& loc);
    virtual void onLocationChanged(const Location& oldloc, const Location& newloc);

    // information rendering and interactivity...

    // the keywords used for things like 'look' and 'get'.
    //virtual std::vector<std::string> getInteractivityKeywords(Character* viewer) = 0;

    // When categorizing this thing in a Look display, what should it show under?
    // Should be plural. Example: Creatures, Items, Vehicles, Furniture, etc.
    virtual std::string getLocationDisplayCategory(Character* viewer) const = 0;

    // Display information about the thing in its current information. used for 'room displays.'
    // Currently doesn't return anything because lots of old code directly calls viewer->sendText()...
    virtual void displayLocationInfo(Character* viewer) = 0;

    virtual bool getLocationVisibleTo(Character* viewer);
};


// Characters and Objects have inventories.
// Only Objects can be in inventories.
struct HasInventory {
    std::vector<std::weak_ptr<Object>> getInventory() const;
    std::list<std::weak_ptr<Object>> inventory;

    void addToInventory(Object* obj);
    void addToInventory(const std::shared_ptr<Object>& obj);
    virtual void onAddToInventory(const std::shared_ptr<Object>& obj) = 0;
    void removeFromInventory(Object* obj);
    virtual void removeFromInventory(const std::shared_ptr<Object>& obj);
    virtual void onRemoveFromInventory(const std::shared_ptr<Object>& obj) = 0;

    void traverseInventory(const std::function<void(Object*)> &func, bool recurse = true);
    struct Object* searchInventory(const std::function<bool(Object*)> &func, bool recurse = true);
    struct Object* searchInventory(obj_vnum vnum, bool working = true, bool recurse = true);
    std::unordered_set<struct Object*> gatherFromInventory(const std::function<bool(Object*)> &func, bool recurse = true);

    void activateInventory();
    void deactivateInventory();
};

// Characters have Equipment. Perhaps Objects will too soon.
struct HasEquipment {
    std::map<int, std::weak_ptr<Object>> equipment;

    std::map<int, Object*> getEquipment() const;
    Object* getEquipSlot(int slot) const;

    void addToEquip(Object* obj, int slot);
    void addToEquip(const std::shared_ptr<Object>& obj, int slot);
    virtual void onAddToEquip(const std::shared_ptr<Object>& obj, int slot) = 0;
    void removeFromEquip(int slot);
    void removeFromEquip(Object* obj);
    void removeFromEquip(const std::shared_ptr<Object>& obj);
    virtual void onRemoveFromEquip(const std::shared_ptr<Object>& obj, int slot) = 0;

    void traverseEquipment(const std::function<void(Object*)> &func, bool recurse = true);
    struct Object* searchEquipment(const std::function<bool(Object*)> &func, bool recurse = true);
    struct Object* searchEquipment(obj_vnum vnum, bool working = true, bool recurse = true);
    std::unordered_set<struct Object*> gatherFromEquipment(const std::function<bool(Object*)> &func, bool recurse = true);

    void activateEquipment();
    void deactivateEquipment();
};

struct HasAffectFlags {
    FlagHandler<AffectFlag> affect_flags{};
};

struct HasStats {
    std::unordered_map<std::string, double> stats{};
};

struct HasID {
    int64_t id{NOTHING}; /* the unique ID of this entity */
};

// base struct for both npc_proto_data and item_proto_data
struct ThingPrototype {
    virtual ~ThingPrototype();
    vnum vn{NOTHING};
    char *name{};
    char *room_description{};      /* When thing is listed in room */
    char *look_description{};      /* what to show when looked at */
    char *short_description{};     /* when displayed in list or action message. */
    struct extra_descr_data *ex_description{}; /* extra descriptions     */
    std::vector<trig_vnum> proto_script; /* list of default triggers  */
    FlagHandler<AffectFlag> affect_flags{}; /* To set affect bits          */
    std::unordered_map<std::string, double> stats;

    std::string scriptString() const;

    ThingPrototype& operator=(const ThingPrototype& other);
};

struct ObjectPrototype : public ThingPrototype, public picky_data {
    ObjectPrototype() = default;
    ObjectPrototype(const Object& other);
    
    ObjectPrototype& operator=(const ObjectPrototype& other);

    ItemType type_flag{ItemType::unknown};      /* Type of item                        */
    std::array<affected_type, MAX_OBJ_AFFECT> affected;  /* affects */
    FlagHandler<WearFlag> wear_flags{}; /* Where you can wear it     */
    FlagHandler<ItemFlag> item_flags{}; /* If it hums, glows, etc.  */
    Size size{Size::medium};           /* Size class of object                */
    
    template<typename R = double>
    R getBaseStat(const std::string& stat) {
        return itemProtoStats.getBase<R>(this, stat);
    }

    template<typename R = double>
    R setBaseStat(const std::string& stat, double val) {
        return itemProtoStats.setBase<R>(this, stat, val);
    }

    template<typename R = double>
    R modBaseStat(const std::string& stat, double val) {
        return itemProtoStats.modBase<R>(this, stat, val);
    }
};

template <>
struct fmt::formatter<ObjectPrototype> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ObjectPrototype& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "ObjectPrototype {} '{}'", z.vn, z.short_description ? z.short_description : "<unnamed>");
    }
};

/* ================== Memory Structure for Objects ================== */
struct Object : public HasID, public HasLocation, public HasInventory, public HasExtraDescriptions, public HasDgScripts, public HasMudStrings, public HasAffectFlags, public HasSubscriptions, public HasStats,public picky_data, std::enable_shared_from_this<Object> {
    static NegativeKeyGuardUnorderedMap<int64_t, std::shared_ptr<Object>> registry;
    Object();
    ~Object();
    Object& operator=(const ObjectPrototype& proto);
    
    const char* getDgName() const override;
    std::vector<trig_vnum> getProtoScript() const override;
    
    double getAffectModifier(uint64_t location, uint64_t specific);

    void commit_iedit(const ObjectPrototype &proto);

    ObjectPrototype* getProto() const;

    bool active{false};
    bool isActive() const override;
    void activate();
    void deactivate();
    std::string getUID(bool active) const override;

    struct Room* getAbsoluteRoom();
    bool isWorking();

    Location getAbsoluteLocation();

    void clearLocation();

    bool isActiveInLocation() const override;
    void displayLocationInfo(Character* viewer) override;
    std::string getLocationDisplayCategory(Character* viewer) const override;
    std::shared_ptr<HasLocation> getSharedHasLocation() override;

    void onAddToInventory(const std::shared_ptr<Object>& obj) override;
    void onRemoveFromInventory(const std::shared_ptr<Object>& obj) override;

    std::shared_ptr<Object> shared();

    room_vnum room_loaded{NOWHERE};    /* Room loaded in, for room_max checks	*/

    /* arbitrary named doubles */
    ItemType type_flag{ItemType::unknown};      /* Type of item                        */
    
    FlagHandler<WearFlag> wear_flags{}; /* Where you can wear it     */
    FlagHandler<ItemFlag> item_flags{}; /* If it hums, glows, etc.  */
    
    Size size{Size::medium};           /* Size class of object                */

    std::array<affected_type, MAX_OBJ_AFFECT> affected;  /* affects */

    // when equipped or held by a character, they'll be set to this.
    std::weak_ptr<Character> carrier{};
    int16_t worn_on{-1};        /* If the object is worn, where */

    // if the object is inside a container...
    std::weak_ptr<Object> container{};

    Object *getContainer() const;
    Character *getCarriedBy() const;
    Character *getWornBy() const;
    int16_t getWornOn() const;

    std::weak_ptr<Character> sitting{};       /* Who is sitting on me? */
    struct Character *user{};
    struct Character *target{};
    char *auctname{};
    struct Object *posted_to{};
    struct Object *fellow_wall{};

    int64_t aucter{};
    int64_t curBidder{};
    time_t aucTime{};
    int bid{};
    int startbid{};
    int posttype{};

    bool isProvidingLight();
    double currentGravity();

    void onMoveToLocation(const Location& loc) override;
    void onLeaveLocation(const Location& loc) override;
    void onLocationChanged(const Location& oldloc, const Location& newloc) override;

    template<typename R = double>
    R getBaseStat(const std::string& stat) {
        return itemStats.getBase<R>(this, stat);
    }

    template<typename R = double>
    R setBaseStat(const std::string& stat, double val) {
        return itemStats.setBase<R>(this, stat, val);
    }

    template<typename R = double>
    R modBaseStat(const std::string& stat, double val) {
        return itemStats.modBase<R>(this, stat, val);
    }

    template<typename R = double>
    R gainBaseStat(const std::string& stat, double val, bool applyBonuses = true) {
        return itemStats.gainBase<R>(this, stat, val, applyBonuses);
    }

    template<typename R = double>
    R gainBaseStatPercent(const std::string& stat, double percent, bool applyBonuses = true) {
        return itemStats.gainBasePercent<R>(this, stat, percent, applyBonuses);
    }

    template<typename R = double>
    R getEffectiveStat(const std::string& stat) {
        return itemStats.getEffective<R>(this, stat);
    }
};

template <>
struct fmt::formatter<Object> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Object& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "[VN{}] Object {} '{}'", z.vn, z.id, z.getShortDescription());
    }
};

/* ======================================================================= */

/* room-related structures ************************************************/

struct Destination : public Location {
    using Location::Location;
    using Location::operator=;
    Destination() = default;
    Destination(const Location& loc) : Location(loc) {}
    Direction dir{Direction::north}; /* Direction of the exit */

    std::string general_description{};       /* When look DIR.			*/
    std::string keyword{};        /* for open/close			*/

    FlagHandler<ExitFlag> exit_flags{}; /* Exit flags			*/

    obj_vnum key{NOTHING};        /* Key's number (-1 for no key)		*/

    int dclock{};            /* DC to pick the lock			*/
    int dchide{};            /* DC to find hidden			*/

    // this field deliberately not serialized.
    bool generated{false};

    std::optional<Destination> getReverse() const;

    void legacyExitFlags(int flags);
};

template <>
struct fmt::formatter<Destination> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Destination& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{} Exit {} to {}", z.generated ? "Generated" : "Direct", z.dir, z.getLocID());
    }
};

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

    virtual const std::vector<ExtraDescription>& getExtraDescription(const Coordinates& coor) const;

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

    virtual FlagHandler<WhereFlag>& getWhereFlags(const Coordinates& coor) = 0;
    virtual void setWhereFlag(const Coordinates& coor, WhereFlag flag, bool value = true);
    virtual bool toggleWhereFlag(const Coordinates& coor, WhereFlag flag);
    virtual bool getWhereFlag(const Coordinates& coor, WhereFlag flag);

    virtual SectorType getSectorType(const Coordinates& coor) const = 0;

    virtual void broadcastAt(const Coordinates& coor, const std::string& message) = 0;

    virtual int getDamage(const Coordinates& coor) const = 0;
    virtual int setDamage(const Coordinates& coor, int amount) = 0;
    virtual int modDamage(const Coordinates& coor, int amount) = 0;

    virtual int getGroundEffect(const Coordinates& coor) const = 0;
    virtual void setGroundEffect(const Coordinates& coor, int val) = 0;
    virtual int modGroundEffect(const Coordinates& coor, int val) = 0;

    virtual SpecialFunc getSpecialFunc(const Coordinates& coor) const = 0;

    virtual double getEnvironment(const Coordinates& coor, int type) const = 0;
    virtual double setEnvironment(const Coordinates& coor, int type, double value) = 0;
    virtual double modEnvironment(const Coordinates& coor, int type, double value) = 0;
    virtual void clearEnvironment(const Coordinates& coor, int type) = 0;

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

struct TileOverride : public HasResetCommands {
    explicit operator bool() const;
    std::unordered_map<std::string, std::string> strings;
    std::optional<SectorType> sectorType;
    FlagHandler<RoomFlag> roomFlags;
    FlagHandler<WhereFlag> whereFlags;
    int damage{0};
    int groundEffect{0};
    std::map<Direction, Destination> exits;
    // an override to display tiles differently than SectorType
    std::string tileDisplay{};
};

enum class ShapeType : uint8_t {
    Box = 0,
    Round = 1
};

struct AABB {
    Coordinates min; // inclusive
    Coordinates max; // inclusive
    bool contains(const Coordinates& c) const noexcept {
        return (c.x >= min.x && c.x <= max.x &&
                c.y >= min.y && c.y <= max.y &&
                c.z >= min.z && c.z <= max.z);
    }
};

// ---------- Box geometry ----------
struct BoxDim {
    AABB box;

    static BoxDim fromCorners(Coordinates a, Coordinates b) noexcept {
        Coordinates lo{ std::min(a.x,b.x), std::min(a.y,b.y), std::min(a.z,b.z) };
        Coordinates hi{ std::max(a.x,b.x), std::max(a.y,b.y), std::max(a.z,b.z) };
        return BoxDim{ AABB{ lo, hi } };
    }

    // size is a count of tiles (>=1). Even sizes extend “more” toward + side; consistent > symmetric.
    static BoxDim fromCenter(Coordinates c, int sx, int sy, int sz = 1) noexcept {
        auto span = [](int c, int s){ int min = c - s/2; return std::pair{min, min + s - 1}; };
        auto [minx,maxx] = span(c.x, sx);
        auto [miny,maxy] = span(c.y, sy);
        auto [minz,maxz] = span(c.z, sz);
        return BoxDim{ AABB{ {minx,miny,minz}, {maxx,maxy,maxz} } };
    }

    bool contains(const Coordinates& c) const noexcept {
        return box.contains(c);
    }
};

// ---------- Round geometry (disk/cylinder in Z) ----------
struct RoundDim {
    Coordinates center;
    int radius = 0;   // tiles
    int zMin = 0;     // inclusive
    int zMax = 0;     // inclusive
    int r2  = 0;      // cached radius^2

    static RoundDim disk(Coordinates c, int r, int zMin, int zMax) noexcept {
        RoundDim d{c, r, zMin, zMax, r*r};
        return d;
    }

    AABB bounds() const noexcept {
        return AABB{
            { center.x - radius, center.y - radius, zMin },
            { center.x + radius, center.y + radius, zMax }
        };
    }

    bool contains(const Coordinates& c) const noexcept {
        if (c.z < zMin || c.z > zMax) return false;
        auto dx = c.x - center.x;
        auto dy = c.y - center.y;
        return (dx*dx + dy*dy) <= r2;
    }
};

// the serializable data for shapes.
struct ShapeBase {
    ShapeBase& operator=(const Shape& other);
    ShapeType type{ShapeType::Box};
    int priority{0};
    SectorType sectorType{SectorType::inside};
    std::string name{};
    std::string description{};
    std::variant<BoxDim, RoundDim> geom;
    // an override to display tiles differently.
    std::string tileDisplay{};
};

// This is used for defining "areas" for re-use.
// We can both create Areas/Structures from them, and save one to them.
struct GridTemplate : public HasMudStrings, public HasVnum {
    // used for defining grid templates.
    GridTemplate& operator=(const AbstractGridArea& other);
    using HasMudStrings::getName;
    using HasMudStrings::getLookDescription;

    std::unordered_map<std::string, ShapeBase> shapes;
    std::unordered_map<Coordinates, TileOverride> tileOverrides;
};

// The shape that's used for instances.
struct Shape : public ShapeBase {
    using ShapeBase::operator=;
    Shape() = default;
    explicit Shape(const ShapeBase& b);
    int seq{0};

    // Fast reject
    AABB aabb() const noexcept { return cachedAabb; }
    bool contains(const Coordinates& c) const noexcept;
    void reComputeAABB();
    AABB cachedAabb;      // filled on add/update
};

struct BucketKey {
    int bx, by, bz;
    bool operator==(const BucketKey& o) const noexcept {
        return bx==o.bx && by==o.by && bz==o.bz;
    }
};

namespace std {
    template<>
    struct hash<BucketKey> {
        std::size_t operator()(const BucketKey& coord) const noexcept {
            // decent 3D hash
            uint64_t h = 1469598103934665603ull;
            auto mix=[&](int v){ h ^= uint64_t(uint32_t(v)); h *= 1099511628211ull; };
            mix(coord.bx); mix(coord.by); mix(coord.bz); return size_t(h);
        }
    };
}


struct AbstractGridArea : public HasMudStrings, public AbstractLocation, public HasSubscriptions {
    AbstractGridArea& operator=(const GridTemplate& other);
    using HasMudStrings::getName;
    using HasMudStrings::getLookDescription;

    std::unordered_map<std::string, std::unique_ptr<Shape>> shapes;
    mutable std::unordered_map<Coordinates, TileOverride> tileOverrides;

    std::vector<Shape*> byPriority;
    
    int bucketSize = 32; // tiles per bucket edge
    std::unordered_map<BucketKey, std::vector<Shape*>> buckets;

    // --- Bookkeeping for stable tie-break
    uint64_t nextSeq = 1; // incremented on add
    void rebuildShapeIndex();

    Shape* topShapeAt(const Coordinates& coor) const;

    // Nearly-complete AbstractLocation implementation on AbstractGridArea. Child classes still need further
    // specialization.

    bool validCoordinates(const Coordinates& coor) const override;
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

    double getEnvironment(const Coordinates& coor, int type) const override;
    double setEnvironment(const Coordinates& coor, int type, double value) override;
    double modEnvironment(const Coordinates& coor, int type, double value) override;
    void clearEnvironment(const Coordinates& coor, int type) override;

    void replaceExit(const Coordinates& coor, const Destination& dest) override;
    void deleteExit(const Coordinates& coor, Direction dir) override;

    bool buildwalk(const Coordinates& coor, Character* ch, Direction dir) override;

    void setString(const Coordinates& coor, const std::string& name, const std::string& value) override;
    void setResetCommands(const Coordinates& coor, const std::vector<ResetCommand>& cmds) override;
    std::vector<ResetCommand> getResetCommands(const Coordinates& coor) override;
    void setSectorType(const Coordinates& coor, SectorType type) override;

};

struct HasZone {
    Zone* getZone() const;
    std::experimental::observer_ptr<Zone> zone{nullptr};
};

struct Area : public AbstractGridArea, public HasVnum, public HasZone, std::enable_shared_from_this<Area> {
    std::string getLocID() const override;
    vnum getLocVnum() const override;
    Zone* getLocZone() const override;
    std::shared_ptr<AbstractLocation> getSharedAbstractLocation() override;

};

struct Structure : public AbstractGridArea, public HasID, public HasLocation, std::enable_shared_from_this<Structure> {
    std::string getLocID() const override;
    vnum getLocVnum() const override;
    Zone* getLocZone() const override;
    std::string getLocationDisplayCategory(Character* viewer) const override;
    std::shared_ptr<AbstractLocation> getSharedAbstractLocation() override;

    bool isActiveInLocation() const override;
    void displayLocationInfo(Character* viewer) override;
    std::shared_ptr<HasLocation> getSharedHasLocation() override;

};

/* ================== Memory Structure for room ======================= */
struct Room : public AbstractLocation, public HasZone, public HasDgScripts, public HasMudStrings, public HasExtraDescriptions, public HasSubscriptions, public HasResetCommands, std::enable_shared_from_this<Room> {
    static NegativeKeyGuardUnorderedMap<int, std::shared_ptr<Room>> registry;
    
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

    int deathtrap_timer{};                   /* For timed Dt's                     */
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
            basic_mud_log("SYSERR: Format error in Room::sendFmt: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
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
            basic_mud_log("SYSERR: Format error in Room::send_to: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
            return 0;
        }
    }
    
    void deleteExit(Direction dir);
    void replaceExit(const Destination& dest);

    std::shared_ptr<Room> shared();

    std::optional<std::string> dgCallMember(const std::string& member, const std::string& arg) override;

    double getEnvironment(int type) const;
    double setEnvironment(int type, double value);
    double modEnvironment(int type, double value);
    void clearEnvironment(int type);
    std::unordered_map<int, double> environment;

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

    double getEnvironment(const Coordinates& coor, int type) const override;
    double setEnvironment(const Coordinates& coor, int type, double value) override;
    double modEnvironment(const Coordinates& coor, int type, double value) override;
    void clearEnvironment(const Coordinates& coor, int type) override;

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
    auto format(const Room& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "Room {} '{}'", z.getVnum(), z.getName());
    }
};

/* ====================================================================== */


/* char-related structures ************************************************/


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
    time_info_data() = default;
    time_info_data(int64_t timestamp);
    double remainder{};
    int seconds{}, minutes{}, hours{}, day{}, month{};
    int64_t year{};
    // The number of seconds since year 0. Can be negative.
    int64_t current();
};


/* These data contain information about a players time data */
struct time_data {
    time_data() = default;
    int64_t birth{};    /* NO LONGER USED This represents the characters current IC age        */
    time_t created{};    /* This does not change                              */
    int64_t maxage{};    /* This represents death by natural causes (UNUSED) */
    time_t logon{};    /* Time of the last logon (used to calculate played) */
    double played{};    /* This is the total accumulated time played in secs */
    double seconds_aged{}; // The player's current IC age, in seconds.
    int currentAge();
};


/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs. This structure
 * can be changed freely.
 */
struct alias_data {
    std::string name;
    std::string replacement;
    int type{};
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
    std::list<std::weak_ptr<Character>> memory{};        /* List of attackers to remember	       */
    int attack_type{};        /* The Attack Type Bitvector for NPC's     */
    int default_pos{POS_STANDING};        /* Default position for NPC                */
    int damnodice{};          /* The number of damage dice's	       */
    int damsizedice{};        /* The size of the damage dice's           */
    bool newitem{};             /* Check if mob has new inv item       */
};

/* Queued spell entry */
struct queued_act {
    int level;
    int spellnum;
};


enum ResurrectionMode : uint8_t {
    Costless = 0,
    Basic = 1,
    RPP = 2
};

struct skill_data {
    int16_t level{0};
    int16_t perfs{0};
};

struct trans_data {

    std::string description{};
    double time_spent_in_form{0.0};
    int grade = 1;
    bool visible = true;
    bool limit_broken = false;
    bool unlocked = false;

    std::unordered_map<Appearance, std::string> appearances;
    std::unordered_map<std::string, double> vars;

    double blutz{0.0}; // The number of seconds you can spend in Oozaru.
};

enum Task 
{
    nothing = 0,
    meditate = 1,
    situps = 2,
    pushups = 3,
    crafting = 4,

    trainStr = 10,
    trainAgl = 11,
    trainCon = 12,
    trainSpd = 13,
    trainInt = 14,
    trainWis = 15,
};

const std::string DoingTaskName[] {
    "nothing",
    "meditating",
    "situps",
    "pushups",
    "crafting",
    "RES",
    "RES",
    "RES",
    "RES",
    "RES",
    "str training",
    "agl training",
    "con training",
    "spd training",
    "int training",
    "wis training",
};

struct card {
    std::string name = "Default";
    std::function<bool(struct Character *ch)> effect;
    std::string playerAnnounce = "You focus hard on your work.\r\n";
    std::string roomAnnounce = "$n focuses hard on $s work.\r\n";
    bool discard = false;

};

struct deck {
    std::vector<struct card> deck;
    std::vector<struct card> discard;

    void shuffleDeck();
    void discardCard(std::string);
    void discardCard(card card);
    bool playTopCard(Character* ch);
    card findCard(std::string);
    void addCardToDeck(std::string, int num = 1);
    void removeCard(std::string);
    void addCardToDeck(card, int num = 1);
    void removeCard(card);
    void initDeck(Character* ch);
};

struct craftTask {
    struct Object *pObject = nullptr;
    int improvementRounds = 0;
};

struct CharacterPrototype : public ThingPrototype {
    Race race{Race::human};
    std::optional<SubRace> subrace{};
    Sensei sensei{Sensei::commoner};
    Sex sex{Sex::male};
    struct mob_special_data mob_specials{};
    Size size{Size::undefined};
    FlagHandler<CharacterFlag> character_flags{};
    FlagHandler<MobFlag> mob_flags{};
    FlagHandler<Race> bio_genomes{};
    FlagHandler<Mutation> mutations{};
    char *clan{};
    int crank{}; // clan rank

    template<typename R = double>
    R getBaseStat(const std::string& stat) {
        return npcProtoStats.getBase<R>(this, stat);
    }

    template<typename R = double>
    R setBaseStat(const std::string& stat, double val) {
        return npcProtoStats.setBase<R>(this, stat, val);
    }

    template<typename R = double>
    R modBaseStat(const std::string& stat, double val) {
        return npcProtoStats.modBase<R>(this, stat, val);
    }
};

/* ================== Structure for player/non-player ===================== */
struct Character : public HasID, public HasLocation, public HasEquipment, public HasInventory, public HasMudStrings, public HasDgScripts, public HasAffectFlags, public HasSubscriptions, public HasStats, std::enable_shared_from_this<Character> {
    static NegativeKeyGuardUnorderedMap<int64_t, std::shared_ptr<Character>> registry;

    Character();
    ~Character();
    // this constructor below is to be used only for the mob_proto map.

    Character& operator=(CharacterPrototype& proto);

    bool isPC{false};

    const char* getDgName() const override;
    std::vector<trig_vnum> getProtoScript() const override;
    void activate();
    void deactivate();
    std::optional<std::string> dgCallMember(const std::string& member, const std::string& arg) override;

    std::string getUID(bool active) const override;

    CharacterPrototype* getProto() const;

    bool isActiveInLocation() const override;
    std::shared_ptr<HasLocation> getSharedHasLocation();
    void displayLocationInfo(Character* viewer) override;
    std::string getLocationDisplayCategory(Character* viewer) const override;

    void onMoveToLocation(const Location& loc) override;
    void onLeaveLocation(const Location& loc) override;
    void onLocationChanged(const Location& oldloc, const Location& newloc) override;

    void sendText(const std::string& txt);

    template<typename... Args>
    void sendFmt(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            if(formatted_string.empty()) return;
            sendText(formatted_string);
        }
        catch(const fmt::format_error& e) {
            basic_mud_log("SYSERR: Format error in Character::sendFmt: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
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
            basic_mud_log("SYSERR: Format error in Character::send_to: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
            return 0;
        }
    }

    void login();

    bool active{false};
    bool isActive() const override;

    void ageBy(double addedTime);
    void setAge(double newAge);

    void onAttack(atk::Attack& outgoing);
    void onAttacked(atk::Attack& incoming);

    void onAddToInventory(const std::shared_ptr<Object>& obj) override;
    void onRemoveFromInventory(const std::shared_ptr<Object>& obj) override;
    void onAddToEquip(const std::shared_ptr<Object>& obj, int slot) override;
    void onRemoveFromEquip(const std::shared_ptr<Object>& obj, int slot) override;

    void lookAtLocation();
    void lookAtLocation(Location& loc);

    std::shared_ptr<Character> shared();

    Race race{Race::spirit};
    std::optional<SubRace> subrace{};
    Sensei sensei{Sensei::commoner};
    Sex sex{Sex::male};

    // Base stats for this unit.
    std::unordered_map<Appearance, std::string> appearances{};
    std::string getAppearance(Appearance type, bool withTransform = true);
    const char* getAppearanceStr(Appearance type);

    std::list<std::pair<int, CommandData>> wait_input_queue;
    Task task{Task::nothing};
    void setTask(Task t);
    struct craftTask craftingTask;
    struct deck craftingDeck;

    /* PC / NPC's weight                    */
    bool canCarryWeight(struct Object *obj);
    bool canCarryWeight(struct Character *obj);
    bool canCarryWeight(weight_t val);

    template<typename R = double>
    R getBaseStat(const std::string& stat) {
        return charStats.getBase<R>(this, stat);
    }

    template<typename R = double>
    R setBaseStat(const std::string& stat, double val) {
        return charStats.setBase<R>(this, stat, val);
    }

    template<typename R = double>
    R modBaseStat(const std::string& stat, double val) {
        return charStats.modBase<R>(this, stat, val);
    }

    template<typename R = double>
    R gainBaseStat(const std::string& stat, double val, bool applyBonuses = true) {
        return charStats.gainBase<R>(this, stat, val, applyBonuses);
    }

    template<typename R = double>
    R gainBaseStatPercent(const std::string& stat, double percent, bool applyBonuses = true) {
        return charStats.gainBasePercent<R>(this, stat, percent, applyBonuses);
    }

    template<typename R = double>
    R getEffectiveStat(const std::string& stat) {
        return charStats.getEffective<R>(this, stat);
    }

    struct mob_special_data mob_specials{};

    Size size{Size::undefined};
    int getSize();
    int setSize(int val);

    double getTimeModifier();
    double getPotential();
    void gainGrowth();
    void gainGrowth(double);

    // Instance-relevant fields below...
    struct time_data time{};    /* PC's AGE in days			*/
    struct affected_type *affected{};
    /* affected by what spells		*/
    struct affected_type *affectedv{};
    /* affected by what combat spells	*/
    struct queued_act *actq{};    /* queued spells / other actions	*/

    struct descriptor_data *desc{};    /* nullptr for mobiles			*/

    struct script_memory *memory{};    /* for mob memory triggers		*/

    /* For fighting list			*/
    struct Character *next_affect{};/* For affect wearoff			*/
    struct Character *next_affectv{};
    /* For round based affect wearoff	*/

    struct Character *master{};    /* Who is char following? */
    WeakBag<Character> followers{}; /* List of chars followers. master is the reverse */
    
    Handle<Object> sits{};      /* What am I sitting on? */
    struct Character *fighting{};    /* Opponent				*/
    
    struct Character *blocks{};    /* Who am I blocking?    */
    struct Character *blocked{};   /* Who is blocking me?    */
    struct Character *absorbing{}; /* Who am I absorbing */
    struct Character *absorbby{};  /* Who is absorbing me */
    struct Character *carrying{};
    struct Character *carried_by{};
    
    struct Character *drag{};
    struct Character *dragged{};
    struct Character *mindlink{};
    struct Character *grappling{};
    struct Character *grappled{};
    struct Character *defender{};
    struct Character *defending{};
    struct Character *poisonby{};
    WeakBag<Character> poisoned;
    struct Character *original{};

    WeakBag<Character> clones{};
    void mergeClones();
    std::map<Skill, skill_data> skill;

    FlagHandler<CharacterFlag> character_flags{};
    FlagHandler<PlayerFlag> player_flags{}; /* act flag for NPC's; player flag for PC's */
    FlagHandler<MobFlag> mob_flags{};
    FlagHandler<AdminFlag> admin_flags{};    /* Bitvector for admin privs		*/
    FlagHandler<PrefFlag> pref_flags{};    /* preference flags for PC's.		*/
    FlagHandler<Race> bio_genomes{};
    FlagHandler<Mutation> mutations{};

    std::bitset<NUM_WEARS> bodyparts{};  /* Bitvector for current bodyparts      */

    int64_t getExperience();
    int64_t setExperience(int64_t value);
    int64_t modExperience(int64_t value, bool applyBonuses = true);

    Form form{Form::base};        /* Current form of the character		*/
    Form technique{Form::base};        /* Current technique form of the character		*/
    std::unordered_set<Form> permForms;    /* Permanent forms of the character	*/

    void gazeAtMoon();

    // Data stored about different forms.
    std::unordered_map<Form, trans_data> transforms;

    std::array<int, 6> gravAcclim;
    std::optional<Race> mimic{};

    /* All below added by Iovan for sure o.o */
    char *voice{}; /* PC's snet voice */
    char *feature{};
    char *temp_prompt{};
    char *rdisplay{};
    char *poofin{};            /* Description on arrival of a god.     */
    char *poofout{};        /* Description upon a god's exit.       */
    void *last_olc_targ{};        /* olc control                          */
    int timer{};            /* Timer for update			*/

    time_t lboard[5]{};
    int limbs[4]{}; /* 0 Right Arm, 1 Left Arm, 2 Right Leg, 3 Left Leg */
    int8_t limb_condition[4]{};
    int8_t conditions[NUM_CONDITIONS]{};        /* Drunk, full, thirsty			*/

    room_vnum normalizeLoadRoom(room_vnum in);

    double getAffectModifier(uint64_t location, uint64_t specific);

    // C++ reworking
    std::string juggleRaceName(bool capitalized);

    void restore(bool announce);

    void ghostify();

    void restore_by(Character *ch);

    void gainTail(bool announce = true);
    void loseTail();
    bool hasTail();

    void hideTransform(Form form, bool hide);
    void addTransform(Form form);
    bool removeTransform(Form form);
    void attemptLimitBreak();
    void removeLimitBreak();

    void resurrect(ResurrectionMode mode);

    bool hasGravAcclim(int tier);
    void raiseGravAcclim();

    void teleport_to(room_vnum rnum);

    bool in_room_range(room_vnum low_rnum, room_vnum high_rnum);

    bool in_past();

    bool is_newbie();

    bool in_northran();

    int64_t calc_soft_cap();

    bool is_soft_cap(int64_t type, long double mult);

    bool is_soft_cap(int64_t type);

    int wearing_android_canister();

    int64_t calcGravCost(int64_t num);

    // Stats stuff

    double modCurVitalDam(CharVital type, double dam);
    double setCurVitalDam(CharVital type, double dam);
    double getCurVitalDam(CharVital type);
    double getCurVitalMeterPercent(CharVital type);
    int64_t getCurVitalPercent(CharVital type, double amt);
    int64_t getMaxVitalPercent(CharVital type, double amt);
    int64_t getMaxVital(CharVital type);
    int64_t getCurVital(CharVital type);
    int64_t setCurVital(CharVital type, int64_t amt);
    int64_t modCurVital(CharVital type, int64_t amt);
    bool isFullVital(CharVital type);
    bool isFullVitals();
    void restoreVital(CharVital type);

    void restoreHealth(bool announce = true);

    int64_t getPL(bool suppressed = true);
    
    void restoreVitals(bool announce = true);
    void restoreStatus(bool announce = true);
    void restoreLimbs(bool announce = true);

    // status stuff
    void cureStatusKnockedOut(bool announce = true);
    void cureStatusBurn(bool announce = true);

    void cureStatusPoison(bool announce = true);
    void setStatusKnockedOut();

    // stats refactor stuff
    void apply_kaioken(int times, bool announce);
    void remove_kaioken(int8_t announce);
    int getRPP();
    void modRPP(int amt);
    int getPractices();
    void modPractices(int amt);
    bool isProvidingLight();
    double currentGravity();

    // converting functions to methods below...
    bool isSparring() const;
    
};


/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
    char *text;
    int aliased;
    struct txt_block *next;
};


struct txt_q {
    struct txt_block *head;
    struct txt_block *tail;
};


struct descriptor_data {
    int64_t id{NOTHING};
    std::unordered_map<int64_t, std::string> conns;
    void onConnectionLost(int64_t connId);
    void onConnectionClosed(int64_t connId);

    char host[HOST_LENGTH + 1];    /* hostname				*/
    int connected{CON_PLAYING};        /* mode of 'connectedness'		*/

    time_t login_time{time(nullptr)};        /* when the person connected		*/
    std::string* std_str{nullptr}; // for the alternate modify-str system...
    std::string std_backstr{};    // for the alternate modify-str system...
    char **str{};            /* for the modify-str system		*/
    char *backstr{};        /* backup string for modify-str system	*/
    size_t max_str{};            /* maximum size of string in modify-str	*/
    int32_t mail_to{};        /* name for mail system			*/
    bool has_prompt{true};        /* is the user at a prompt?             */
    std::string last_input;        /* the last input			*/
    std::list<std::string> raw_input_queue, input_queue;
    std::string output;        /* ptr to the current output buffer	*/
    std::string processed_output;
    std::list<std::string> history;        /* History of commands, for ! mostly.	*/
    struct Character *character{};    /* linked to char			*/
    struct Character *original{};    /* original char if switched		*/
    struct descriptor_data *snooping{}; /* Who is this char snooping	*/
    struct descriptor_data *snoop_by{}; /* And who is snooping this char	*/
    struct descriptor_data *next{}; /* link to next descriptor		*/
    struct oasis_olc_data *olc{};   /* OLC info                            */
    struct Account *account{}; /* Account info                        */
    int level{};
    char *newsbuf{};
    /*---------------Player Level Object Editing Variables-------------------*/
    int obj_editval{};
    int obj_editflag;
    char *obj_was{};
    char *obj_name{};
    char *obj_short{};
    char *obj_long{};
    int obj_type{};
    int obj_weapon{};
    struct Object *obj_point{};
    char *title{};
    double timeoutCounter{0};
    void handle_input();
    void start();
    void handleLostLastConnection(bool graceful);
    void sendText(const std::string &txt);

    template<typename... Args>
    void sendFmt(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            if(formatted_string.empty()) return;
            sendText(formatted_string);
        }
        catch(const fmt::format_error& e) {
            basic_mud_log("SYSERR: Format error in descriptor_data::sendFmt: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
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
            basic_mud_log("SYSERR: Format error in descriptor_data::send_to: %s", e.what());
            basic_mud_log("Template was: %s", format.data());
            return 0;
        }
    }
};

/* used in the socials */
struct social_messg {
    int act_nr;
    char *command;               /* holds copy of activating command */
    char *sort_as;              /* holds a copy of a similar command or
                               * abbreviation to sort by for the parser */
    int hide;                   /* ? */
    int min_victim_position;    /* Position of victim */
    int min_char_position;      /* Position of char */
    int min_level_char;          /* Minimum level of socialing char */

    /* No argument was supplied */
    char *char_no_arg;
    char *others_no_arg;

    /* An argument was there, and a victim was found */
    char *char_found;
    char *others_found;
    char *vict_found;

    /* An argument was there, as well as a body part, and a victim was found */
    char *char_body_found;
    char *others_body_found;
    char *vict_body_found;

    /* An argument was there, but no victim was found */
    char *not_found;

    /* The victim turned out to be the character */
    char *char_auto;
    char *others_auto;

    /* If the char cant be found search the char's inven and do these: */
    char *char_obj_found;
    char *others_obj_found;
};


struct weather_data {
    int pressure{};    /* How is the pressure ( Mb ) */
    int change{};    /* How fast and what way does it change. */
    int sky{};    /* How is the sky. */
    int sunlight{};    /* And how much sun. */
};


/*
 * Element in monster and object index-tables.
 *
 * NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum)
 */
struct index_data {
    mob_vnum vn{NOTHING};    /* virtual number of this mob/obj		*/
    SpecialFunc func;

    char *farg;         /* string argument for special function     */
    struct DgScript *proto;     /* for triggers... the trigger     */
};

struct guild_info_type {
    int pc_class;
    room_vnum guild_room;
    int direction;
};

/*
 * Config structs
 * 
 */

/*
* The game configuration structure used for configurating the game play
* variables.
*/
struct game_data {
    int pk_allowed;         /* Is player killing allowed? 	  */
    int pt_allowed;         /* Is player thieving allowed?	  */
    int level_can_shout;      /* Level player must be to shout.	  */
    int holler_move_cost;      /* Cost to holler in move points.	  */
    int tunnel_size;        /* Number of people allowed in a tunnel.*/
    int max_exp_gain;       /* Maximum experience gainable per kill.*/
    int max_exp_loss;       /* Maximum experience losable per death.*/
    int max_npc_corpse_time;/* Num tics before NPC corpses decompose*/
    int max_pc_corpse_time; /* Num tics before PC corpse decomposes.*/
    int idle_void;          /* Num tics before PC sent to void(idle)*/
    int idle_rent_time;     /* Num tics before PC is autorented.	  */
    int idle_max_level;     /* Level of players immune to idle.     */
    int dts_are_dumps;      /* Should items in dt's be junked?	  */
    int load_into_inventory;/* Objects load in immortals inventory. */
    int track_through_doors;/* Track through doors while closed?    */
    int level_cap;          /* You cannot level to this level       */
    int stack_mobs;      /* Turn mob stacking on                 */
    int stack_objs;      /* Turn obj stacking on                 */
    int mob_fighting;       /* Allow mobs to attack other mobs.     */
    char *OK;               /* When player receives 'Okay.' text.	  */
    char *NOPERSON;         /* 'No-one by that name here.'	  */
    char *NOEFFECT;         /* 'Nothing seems to happen.'	          */
    int disp_closed_doors;  /* Display closed doors in autoexit?	  */
    int reroll_player;      /* Players can reroll stats on creation */
    int initial_points;      /* Initial points pool size		  */
    int enable_compression; /* Enable MCCP2 stream compression      */
    int enable_languages;   /* Enable spoken languages              */
    int all_items_unique;   /* Treat all items as unique 		  */
    float exp_multiplier;     /* Experience gain  multiplier	  */
};


/*
 * The rent and crashsave options.
 */
struct crash_save_data {
    int free_rent;          /* Should the MUD allow rent for free?  */
    int max_obj_save;       /* Max items players can rent.          */
    int min_rent_cost;      /* surcharge on top of item costs.	  */
    int auto_save;          /* Does the game automatically save ppl?*/
    int autosave_time;      /* if auto_save=TRUE, how often?        */
    int crash_file_timeout; /* Life of crashfiles and idlesaves.    */
    int rent_file_timeout;  /* Lifetime of normal rent files in days*/
};


/*
 * The room numbers. 
 */
struct room_numbers {
    room_vnum mortal_start_room;    /* vnum of room that mortals enter at.  */
    room_vnum immort_start_room;  /* vnum of room that immorts enter at.  */
    room_vnum frozen_start_room;  /* vnum of room that frozen ppl enter.  */
    room_vnum donation_room_1;    /* vnum of donation room #1.            */
    room_vnum donation_room_2;    /* vnum of donation room #2.            */
    room_vnum donation_room_3;    /* vnum of donation room #3.	        */
};


/*
 * The game operational constants.
 */
struct game_operation {
    uint16_t DFLT_PORT;      /* The default port to run the game.  */
    char *DFLT_IP;            /* Bind to all interfaces.		  */
    char *DFLT_DIR;           /* The default directory (lib).	  */
    char *LOGNAME;            /* The file to log messages to.	  */
    int max_playing;          /* Maximum number of players allowed. */
    int max_filesize;         /* Maximum size of misc files.	  */
    int max_bad_pws;          /* Maximum number of pword attempts.  */
    int siteok_everyone;        /* Everyone from all sites are SITEOK.*/
    int nameserver_is_slow;   /* Is the nameserver slow or fast?	  */
    int use_new_socials;      /* Use new or old socials file ?      */
    int auto_save_olc;        /* Does OLC save to disk right away ? */
    char *MENU;               /* The MAIN MENU.			  */
    char *WELC_MESSG;        /* The welcome message.		  */
    char *START_MESSG;        /* The start msg for new characters.  */
    int imc_enabled; /**< Is connection to IMC allowed ? */
};

/*
 * The Autowizard options.
 */
struct autowiz_data {
    int use_autowiz;        /* Use the autowiz feature?		*/
    int min_wizlist_lev;    /* Minimun level to show on wizlist.	*/
};

/* This is for the tick system.
 *
 */

struct tick_data {
    int pulse_violence;
    int pulse_mobile;
    int pulse_zone;
    int pulse_autosave;
    int pulse_idlepwd;
    int pulse_sanity;
    int pulse_usage;
    int pulse_timesave;
    int pulse_current;
};

/*
 * The character advancement (leveling) options.
 */
struct advance_data {
    int allow_multiclass; /* Allow advancement in multiple classes     */
    int allow_prestige;   /* Allow advancement in prestige classes     */
};

/*
 * The new character creation method options.
 */
struct creation_data {
    int method; /* What method to use for new character creation */
};

/*
 * The main configuration structure;
 */
struct config_data {
    char *CONFFILE;    /* config file path	 */
    struct game_data play;        /* play related config   */
    struct crash_save_data csd;        /* rent and save related */
    struct room_numbers room_nums;    /* room numbers          */
    struct game_operation operation;    /* basic operation       */
    struct autowiz_data autowiz;    /* autowiz related stuff */
    struct advance_data advance;   /* char advancement stuff */
    struct tick_data ticks;        /* game tick stuff 	 */
    struct creation_data creation;    /* char creation method	 */
};

/*
 * Data about character aging
 */
struct aging_data {
    int adult;        /* Adulthood */
    int classdice[3][2];    /* Dice info for starting age based on class age type */
    int middle;        /* Middle age */
    int old;        /* Old age */
    int venerable;    /* Venerable age */
    int maxdice[2];    /* For roll to determine natural death beyond venerable */
};


struct shop_buy_data {
    int type{};
    std::string keywords{};
};

struct org_data : public picky_data {
    int vnum{NOTHING};        /* Virtual number of this shop		*/

    mob_vnum keeper{NOBODY};                   /* GM's vnum */
    std::vector<std::weak_ptr<Character>> getKeepers();
    SpecialFunc func{};        /* Secondary spec_proc for keeper	*/
    std::string customerString();
};

struct shop_data : public org_data {
    ~shop_data();
    void add_product(obj_vnum v);
    void remove_product(obj_vnum v);
    std::vector<obj_vnum> producing{};        /* Which item to produce (virtual)	*/
    float profit_buy{};        /* Factor to multiply cost with		*/
    float profit_sell{};        /* Factor to multiply cost with		*/
    std::vector<shop_buy_data> type{};    /* Which items to trade			*/
    char *no_such_item1{};        /* Message if keeper hasn't got an item	*/
    char *no_such_item2{};        /* Message if player hasn't got an item	*/
    char *missing_cash1{};        /* Message if keeper hasn't got cash	*/
    char *missing_cash2{};        /* Message if player hasn't got cash	*/
    char *do_not_buy{};        /* If keeper dosn't buy such things	*/
    char *message_buy{};        /* Message when player buys item	*/
    char *message_sell{};        /* Message when player sells item	*/
    int temper1{};        /* How does keeper react if no money	*/
    FlagHandler<ShopFlag> shop_flags{};    /* Can attack? Use bank? Cast here?	*/

    std::unordered_set<room_vnum> in_room;        /* Where is the shop?			*/
    int open1{}, open2{};        /* When does the shop open?		*/
    int close1{}, close2{};    /* When does the shop close?		*/
    int bankAccount{};        /* Store all gold over 15000 (disabled)	*/
    int lastsort{};        /* How many items are sorted in inven?	*/

    bool isProducing(obj_vnum vn);
    void runPurge();
};

struct guild_data : public org_data {
    void toggle_skill(uint16_t skill_id);
    void toggle_feat(uint16_t skill_id);
    FlagHandler<Skill> skills;  /* array to keep track of which feats things we'll train */
    float charge{1.0};                  /* charge * skill level = how much we'll charge */
    std::string no_such_skill{};           /* message when we don't teach that skill */
    std::string not_enough_gold{};         /* message when the student doesn't have enough gold */
    int minlvl{0};                    /* Minumum level guildmaster will train */
    int open{0}, close{28};               /* when we will train */
    std::unordered_set<uint8_t> feats;  /* array to keep track of which feats things we'll train */
};


struct ban_list_element {
    char site[BANNED_SITE_LENGTH + 1];
    int type;
    time_t date;
    char name[MAX_NAME_LENGTH + 1];
    struct ban_list_element *next;
};

struct help_index_element {
    char *index{};      /*Future Use */
    char *keywords{};   /*Keyword Place holder and sorter */
    char *entry{};      /*Entries for help files with Keywords at very top*/
    int duplicate{};    /*Duplicate entries for multple keywords*/
    int min_level{};    /*Min Level to read help entry*/
};



typedef struct disabled_data DISABLED_DATA;

extern DISABLED_DATA *disabled_first; /* interpreter.c */

/* one disabled command */
struct disabled_data {
    DISABLED_DATA *next;                /* pointer to next node          */
    struct command_info const *command; /* pointer to the command struct */
    char *disabled_by;                  /* name of disabler              */
    int16_t level;                       /* level of disabler             */
    int subcmd;                         /* the subcmd, if any            */
};
