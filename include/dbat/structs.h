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
#include <functional>
#include "net.h"
#include <type_traits> // For std::is_base_of


struct trig_data;

/**********************************************************************
* Structures                                                          *
**********************************************************************/
struct shop_buy_data {
    shop_buy_data() = default;
    explicit shop_buy_data(const nlohmann::json& j);
    nlohmann::json serialize();
    int type{};
    std::string keywords{};
};

struct Shop {
    Shop() = default;
    explicit Shop(const nlohmann::json& j);
    nlohmann::json serialize();
    void add_product(obj_vnum v);
    void remove_product(obj_vnum v);
    shop_vnum vnum{NOTHING};        /* Virtual number of this shop		*/
    std::vector<obj_vnum> producing{};        /* Which item to produce (virtual)	*/
    float profit_buy{1.0};        /* Factor to multiply cost with		*/
    float profit_sell{1.0};        /* Factor to multiply cost with		*/
    std::vector<shop_buy_data> type{};    /* Which items to trade			*/
    std::string no_such_item1{};        /* Message if keeper hasn't got an item	*/
    std::string no_such_item2{};        /* Message if player hasn't got an item	*/
    std::string missing_cash1{};        /* Message if keeper hasn't got cash	*/
    std::string missing_cash2{};        /* Message if player hasn't got cash	*/
    std::string do_not_buy{};        /* If keeper dosn't buy such things	*/
    std::string message_buy{};        /* Message when player buys item	*/
    std::string message_sell{};        /* Message when player sells item	*/
    int temper1{};        /* How does keeper react if no money	*/
    std::unordered_set<int> flags{};    /* Can attack? Use bank? Cast here?	*/
    mob_vnum keeper{NOBODY};    /* The mobile who owns the shop (rnum)	*/
    std::unordered_set<int> with_who{};/* Who does the shop trade with?	*/
    std::set<room_vnum> in_room;        /* Where is the shop?			*/
    int open1{}, open2{};        /* When does the shop open?		*/
    int close1{}, close2{};    /* When does the shop close?		*/
    int bankAccount{};        /* Store all gold over 15000 (disabled)	*/
    int lastsort{};        /* How many items are sorted in inven?	*/
    
    std::list<Character*> getKeepers();
    bool isProducing(obj_vnum vn);
    bool isProducing(Object *obj1);
    void runPurge();
    int tradeWith(Object* item);
    Object* getSellingObject(Character* ch, const std::string& name, Character *keeper, bool msg);
    Object* getPurchaseObject(Character* ch, const std::string& name, Character *keeper, bool msg);
    std::string listObject(Object *obj, int cnt, int aindex, Character *keeper, Character *ch);
    int64_t buyPrice(Object *obj, Character* keeper, Character* ch);
    int64_t sellPrice(Object *obj, Character* keeper, Character* ch);
    bool executeCommand(Character *keeper, Character *ch, const std::string &cmd, const std::string &arguments);
    
    void executeBuy(Character* keeper, Character *ch, const std::string &cmd, const std::string &arguments);
    void executeSell(Character* keeper, Character *ch, const std::string &cmd, const std::string &arguments);
    void executeList(Character* keeper, Character *ch, const std::string &cmd, const std::string &arguments);
    void executeValue(Character* keeper, Character *ch, const std::string &cmd, const std::string &arguments);
    void executeAppraise(Character* keeper, Character *ch, const std::string &cmd, const std::string &arguments);

    bool isOk(Character* keeper, Character *ch);
    bool isOkChar(Character* keeper, Character *ch);
    bool isOkObj(Character* keeper, Character *ch, Object *obj);
    bool isOpen(Character* keeper, bool msg);
    

};


struct Guild {
    Guild() = default;
    explicit Guild(const nlohmann::json& j);
    nlohmann::json serialize();
    room_vnum vnum{NOBODY};                /* number of the guild */
    void toggle_skill(uint16_t skill_id);
    void toggle_feat(uint16_t skill_id);
    std::set<uint16_t> skills;  /* array to keep track of which feats things we'll train */
    float charge{1.0};                  /* charge * skill level = how much we'll charge */
    std::string no_such_skill{};           /* message when we don't teach that skill */
    std::string not_enough_gold{};         /* message when the student doesn't have enough gold */
    int minlvl{0};                    /* Minumum level guildmaster will train */
    mob_vnum gm{NOBODY};                   /* GM's vnum */
    std::unordered_set<int> with_who{};/* Who does the shop trade with?	*/
    int open{0}, close{28};               /* when we will train */
    std::set<uint8_t> feats;  /* array to keep track of which feats things we'll train */
    std::list<Character*> getMasters();
};


/* structure for the reset commands */
struct reset_com {
    reset_com() = default;
    explicit reset_com(const nlohmann::json& j);
    char command{};   /* current command                      */

    bool if_flag{};    /* if TRUE: exe only if preceding exe'd */
    int arg1{};        /*                                      */
    int arg2{};        /* Arguments to the command             */
    int arg3{};        /*                                      */
    int arg4{};        /* room_max  default 0			*/
    int arg5{};           /* percentages variable                 */
    int line{};        /* line number this command appears on  */
    std::string sarg1;        /* string argument                      */
    std::string sarg2;        /* string argument                      */

    nlohmann::json serialize();

    /*
     *  Commands:              *
     *  'M': Read a mobile     *
     *  'O': Read an object    *
     *  'G': Give obj to mob   *
     *  'P': Put obj in obj    *
     *  'G': Obj to char       *
     *  'E': Obj to char equip *
     *  'D': Set state of door *
     *  'T': Trigger command   *
         *  'V': Assign a variable *
    */
};

struct zone_data {
    zone_data() = default;
    explicit zone_data(const nlohmann::json& j);
    ~zone_data();
    char *name{};            /* name of this zone                  */
    char *builders{};          /* namelist of builders allowed to    */
    /* modify this zone.		  */
    int lifespan{};           /* how long between resets (minutes)  */
    double age{};                /* current age of this zone (minutes) */
    vnum bot{};           /* starting room number for this zone */
    vnum top{};           /* upper limit for rooms in this zone */

    int reset_mode{};         /* conditions for reset (see below)   */
    zone_vnum number{};        /* virtual number of this zone	  */
    std::vector<struct reset_com> cmd;   /* command table for reset	          */
    int min_level{};           /* Minimum level to enter zone        */
    int max_level{};           /* Max Mortal level to enter zone     */
    bitvector_t zone_flags[ZF_ARRAY_MAX]{};          /* Flags for the zone.                */

    nlohmann::json serialize();

    /*
     * Reset mode:
     *   0: Don't reset, and don't update age.
     *   1: Reset if no PC's are located in zone.
     *   2: Just reset.
     */
    std::set<room_vnum> rooms;
    std::set<mob_vnum> mobiles;
    std::set<obj_vnum> objects;
    std::set<shop_vnum> shops;
    std::set<trig_vnum> triggers;
    std::set<guild_vnum> guilds;

};

struct account_data {
    account_data() = default;
    explicit account_data(const nlohmann::json& j);
    vnum vn{NOTHING};
    std::string name;
    std::string passHash;
    std::string email;
    time_t created{};
    time_t lastLogin{};
    time_t lastLogout{};
    time_t lastPasswordChanged{};
    double totalPlayTime{};
    std::string disabledReason;
    time_t disabledUntil{0};
    int adminLevel{};
    int rpp{};
    int slots{3};
    std::vector<std::string> customs;
    std::vector<int64_t> characters;
    std::set<descriptor_data*> descriptors;
    std::set<net::Connection*> connections;

    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    void modRPP(int amt);

    bool checkPassword(const std::string& password);
    bool setPassword(const std::string& password);

    static int getNextID();

};

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
    extra_descr_data() = default;
    explicit extra_descr_data(const nlohmann::json& j);
    std::string keyword;                 /* Keyword in look/examine          */
    std::string description;             /* What to see                      */
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
};

struct ExtraDescriptions {
    ExtraDescriptions() = default;
    explicit ExtraDescriptions(const nlohmann::json& j);
    std::vector<extra_descr_data> ex_description{}; /* extra descriptions     */
    void deserialize(const nlohmann::json& j);
    nlohmann::json serialize();
};

struct obj_affected_type {
    obj_affected_type() = default;
    explicit obj_affected_type(const nlohmann::json& j);
    void deserialize(const nlohmann::json& j);
    nlohmann::json serialize();
    int location{};       /* Which ability to change (APPLY_XXX) */
    int specific{};       /* Some locations have parameters      */
    double modifier{};       /* How much it changes by              */
};

struct HasVars {
    std::unordered_map<std::string, std::string> vars;
    void addVar(const std::string &name, const std::string &value);
    void addVar(const std::string &name, GameEntity* u);
    DgResults getVar(const std::string& name);
    std::string getRaw(const std::string& name);
    bool hasVar(const std::string& name);
    bool delVar(const std::string& name);

    nlohmann::json serializeVars();

};

struct trig_proto {
    trig_proto() = default;
    explicit trig_proto(const nlohmann::json& j);
    trig_vnum vn{NOTHING};                    /* trigger's rnum                  */
    int8_t attach_type{};            /* mob/obj/wld intentions          */
    int8_t data_type{};                /* type of game_data for trig      */
    std::string name;
    long trigger_type{};            /* type of trigger (for bitvector) */
    int narg{};                /* numerical argument              */
    std::string arglist{};            /* argument list                   */
    std::vector<std::string> lines;
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
};

/* a complete script (composed of several triggers) */
struct script_data : public HasVars {
    script_data() = default;
    explicit script_data(GameEntity *u) : script_data() {
        owner = u;
    };
    long types{};                /* bitvector of trigger types */
    std::list<std::shared_ptr<trig_data>> dgScripts;
    bool purged{};                /* script is set to be purged */
    GameEntity* owner{};
    void activate();
    void deactivate();

    void addTrigger(const std::shared_ptr<trig_data> t, int loc);
    void removeAll();
    void removeScript(const std::string &name);

    void loadScript(const std::shared_ptr<trig_data> t);

    nlohmann::json serialize();
    void deserialize(const nlohmann::json &j);

};

enum class NestType : uint8_t {
    IF = 0,
    WHILE = 1,
    SWITCH = 2
};

enum class DgScriptState : uint8_t {
    DORMANT = 0,
    RUNNING = 1,
    WAITING = 2,
    ERROR = 3,
    DONE = 4,
    PURGED = 5
};

struct trig_data : public HasVars, public std::enable_shared_from_this<trig_data> {
    trig_data(std::shared_ptr<trig_proto> parent);
    trig_data(struct script_data *sc, std::shared_ptr<trig_proto> parent);
    ~trig_data();

    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    std::shared_ptr<trig_proto> parent;
    struct script_data *sc;

    std::vector<std::pair<NestType, std::size_t>> depth; /* depth into nest ifs/whiles/etc  */
    std::size_t lineNumber{0};
    int loops{};                /* loop iteration counter          */
    int totalLoops{};
    double waiting{0.0};    /* event to pause the trigger      */
    bool purged{};            /* trigger is set to be purged     */

    bool active{false};
    DgScriptState state{DgScriptState::DORMANT};
    void activate();
    void deactivate();

    void reset();
    void setState(DgScriptState st);

    int execute();
    int executeBlock(std::size_t start, std::size_t end);
    std::string getLine(std::size_t num);

    bool processIf(const std::string &cond);

    std::string evalExpr(const std::string& expr);
    std::string varSubst(const std::string& expr);
    std::string innerSubst(std::vector<DgHolder> &current, const std::string& expr);
    void handleSubst(std::vector<DgHolder> &current, const std::string& field, const std::string& args);

    std::optional<std::string> evalLhsOpRhs(const std::string& expr);

    std::string evalOp(const std::string& op, const std::string& lhr, const std::string& rhr);

    std::string evalNumericOp(const std::string& op, const std::string &lhs, const std::string &rhs);

    bool truthy(const std::string& expr);

    std::size_t findElseEnd(bool matchElseIf = true, bool matchElse = true);
    std::size_t findEnd();
    std::size_t findDone();
    std::size_t findCase(const std::string& cond);

    void processEval(const std::string& expr);
    void extractValue(const std::string& expr);
    void processContext(const std::string& expr);
    void processGlobal(const std::string& cmd);
    void processRemote(const std::string& cmd);
    void processRdelete(const std::string& cmd);
    void processSet(const std::string& cmd);
    void processUnset(const std::string& cmd);
    void processWait(const std::string& cmd);
    void processAttach(const std::string& cmd);
    void processDetach(const std::string& cmd);

};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
    mob_special_data() = default;
    explicit mob_special_data(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
    memory_rec *memory{};        /* List of attackers to remember	       */
    int attack_type{};        /* The Attack Type Bitvector for NPC's     */
    int default_pos{POS_STANDING};        /* Default position for NPC                */
    int damnodice{};          /* The number of damage dice's	       */
    int damsizedice{};        /* The size of the damage dice's           */
    bool newitem{};             /* Check if mob has new inv item       */
};

enum class EntityFamily : uint8_t {
    Character = 0,
    Object = 1,
    Room = 2,
    Exit = 3,
};


struct Info {
    Info() = default;
    explicit Info(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
    int64_t uid;
    EntityFamily family;
};

struct Proto {
    Proto() = default;
    explicit Proto(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
    vnum vn;
    EntityFamily family;
};

struct Physiology {
    Physiology() = default;
    explicit Physiology(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
    RaceID race{RaceID::Spirit};
    int sex{SEX_NEUTRAL};
};

struct Limb {
    Limb() = default;
    explicit Limb(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
    double health{1.0};
    std::set<int> flags;
};

// used for Characters and Corpses... maybe mechas?
struct Limbs {
    Limbs() = default;
    explicit Limbs(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
    std::map<LimbID, Limb> limbs;
};

// used for dismembered body parts like arms.
struct IsLimb {
    LimbID limbType;
    Limb limbData;
};


struct Mimic : public Physiology {
    Mimic() = default;
    explicit Mimic(const nlohmann::json& j);
};

struct PlayerCharacter {
    PlayerCharacter() = default;
    PlayerCharacter player_data(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    std::shared_ptr<account_data> account{};
    std::vector<struct alias_data> aliases;    /* Character's aliases                  */
    std::set<int64_t> senseEntity;
    std::set<mob_vnum> senseMemory;
    std::map<int64_t, std::string> dubNames;
    char *color_choices[NUM_COLOR]{}; /* Choices for custom colors		*/
    struct txt_block *comm_hist[NUM_HIST]{}; /* Player's communications history     */
    
};

struct NonPlayerCharacter {
    // TODO: certainly there is something about NPCs that is different from PCs...
    bool barf;
};

struct Damage {
    std::map<DamageType, int> levels;
    std::map<DamageType, int64_t> soak;
};

struct Coordinates {
    Coordinates() = default;
    explicit Coordinates(const nlohmann::json& j);
    double x{0};
    double y{0};
    double z{0};
    void clear();
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
    bool operator==(const Coordinates& rhs) const;
};

namespace std {
    template<>
    struct hash<Coordinates> {
        size_t operator()(const Coordinates& coord) const noexcept {
            // A simple yet effective hash combining technique
            std::hash<double> hasher;
            size_t h1 = hasher(coord.x);
            size_t h2 = hasher(coord.y);
            size_t h3 = hasher(coord.z);

            // Combine the hash values
            return h1 ^ (h2 << 1) ^ (h3 << 2); // Shift and XOR for simple mixing
        }

        size_t operator()(Coordinates& coord) const noexcept {
            // A simple yet effective hash combining technique
            std::hash<double> hasher;
            size_t h1 = hasher(coord.x);
            size_t h2 = hasher(coord.y);
            size_t h3 = hasher(coord.z);

            // Combine the hash values
            return h1 ^ (h2 << 1) ^ (h3 << 2); // Shift and XOR for simple mixing
        }
    };
}

struct Location {
    Location() = default;
    explicit Location(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    entt::entity location{entt::null};
    // LocationType is extra information about how you are inside <location>.
    // For instance, if you are a character, then the locationType of units
    // you contain should represent inventory/equipment slots. 0 is inventory,
    // positive number is equipment slot.
    int locationType{};
    Coordinates coords{};
    bool operator==(const Location& rhs);
};

struct Contents {
    std::vector<entt::entity> contents;
};

struct Destination {
    Destination() = default;
    explicit Destination(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    explicit Destination(GameEntity* target);
    explicit Destination(Room* target);
    explicit Destination(Character* target);
    explicit Destination(entt::entity target) : target(target) {};
    explicit Destination(const Location& loc) : target(loc.location), locationType(loc.locationType), coords(loc.coords) {};
    entt::entity target{entt::null};
    entt::entity via{entt::null};
    int direction{-1};
    int locationType{};
    Coordinates coords{};
    bool operator==(const Destination& rhs);
};


// TileDetails is used to store information about a tile. Part of the Grid3D system.
struct TileDetails {
    TileDetails() = default;
    explicit TileDetails(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    std::string name;
    std::string description;
    std::optional<int> tile;
    std::set<int> flags;
};

struct Grid3D {
    Grid3D() = default;
    explicit Grid3D(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    int defaultSectorFloor{SECT_FIELD};
    int defaultSectorAbove{SECT_FLYING};
    int defaultSectorBelow{SECT_UNDERWATER};
    std::unordered_map<Coordinates, TileDetails> tiles;
};

struct CoordinateContents {
    std::unordered_map<Coordinates, std::vector<entt::entity>> coordinateContents;
};

struct Boundaries {
    Boundaries() = default;
    explicit Boundaries(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    std::optional<double> maxX, maxY, maxZ, minX, minY, minZ;
};

struct Flags {
    Flags() = default;
    explicit Flags(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    std::unordered_map<FlagType, std::unordered_set<int>> flags;
};

struct Text {
    Text() = default;
    explicit Text(const nlohmann::json& j);
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    std::unordered_map<std::string, std::shared_ptr<InternedString>> strings;
    
};

// Tag component used to show that an entity is to be deleted.
struct Deleted {

};

struct GameEntity : public std::enable_shared_from_this<GameEntity> {
    GameEntity() = default;
    virtual ~GameEntity();
    virtual void deserialize(const nlohmann::json& j);
    virtual nlohmann::json serialize();

    int64_t getUID();
    vnum getVN();
    zone_vnum getZone();

    // This is not how you're supposed to use entt.
    // Oh well. Now we can attach Component data structures to things at least.
    entt::entity ent{entt::null};

    /* unique id for this unit */
    int64_t uid{NOTHING}; 

    // Many NPCs, items, and rooms have a VN.
    // the Room's VN should be the same as its UID.
    vnum vn{NOTHING};

    // Zones. Many things are in a Zone. It's legacy though. :(
    zone_vnum zone{NOTHING};

    // Stores the scripts? Not sure if I need this, given prototypes...
    std::vector<trig_vnum> proto_script;

    // Used to remove the unit from the world.
    virtual void extractFromWorld();
    // Oh no, the thing you're inside is being extracted. What now?
    virtual void onHolderExtraction();

    bool exists{true}; // used for deleted objects. invalid ones are !exists

    std::shared_ptr<script_data> script{};  /* script info for the object */

    bool checkFlag(FlagType type, int flag);
    void setFlag(FlagType type, int flag, bool value = true);
    void clearFlag(FlagType type, int flag);
    bool flipFlag(FlagType type, int flag);
    std::vector<std::string> getFlagNames(FlagType type);

    weight_t getInventoryWeight();
    int64_t getInventoryCount();

    /* Equipment array			*/
    std::vector<GameEntity*> getContents();
    std::vector<Object*> getInventory();
    std::vector<Character*> getPeople();
    std::map<int, Object*> getEquipment();
    std::vector<Room*> getRooms();
    std::map<int, Exit*> getExits();
    std::map<int, Exit*> getUsableExits();

    Room* getAbsoluteRoom();
    Room* getRoom();
    GameEntity* getLocation();

    // removeFromLocation is called first, and handleRemove is called BY removeFromLocation on its location.
    void removeFromLocation();

    // As above, the process is to call addToLocation which will then call handleAdd ON the location.
    void addToLocation(const Destination &dest);
    void addToLocation(GameEntity* mover);

    // Returns a collection of 'units within reach' for commands like look or get.
    std::vector<GameEntity*> getNeighbors(bool visible = true);

    // The business part of the above.
    std::vector<GameEntity*> getNeighborsFor(GameEntity* u, bool visible = true);

    std::vector<std::pair<std::string, Destination>> getLandingSpotsFor(GameEntity *mover);
    std::optional<Destination> getLaunchDestinationFor(GameEntity *mover);

    bool isInvisible();
    bool isHidden();
    bool isAdminInvisible();
    bool canSee(GameEntity *target);
    bool canSeeInvisible();
    bool canSeeHidden();
    bool canSeeAdminInvisible();
    bool canSeeInDark();

    // NOTE: all functions beginning with check return an empty optional if it's OK.
    // the optional string contains the reason why it's NOT OK, otherwise.

    // Whether u can access this unit's inventory.
    std::optional<std::string> checkAllowInventoryAcesss(GameEntity *u);

    // whether this unit will allow u to take/give/drop it. give/put/drop are identical.
    std::optional<std::string> checkIsGettable(GameEntity *u);
    std::optional<std::string> checkIsGivable(GameEntity *u);

    // if this can store u in its inventory. This includes a room accepting items.
    // Things to check might be weight, item capacity, whether it's appropriate for the space, etc.
    std::optional<std::string> checkCanStore(GameEntity *u);

    // Check to see whether giver can give u to this.
    std::optional<std::string> checkAllowReceive(GameEntity *giver, GameEntity *u);

    // Whether this can be equipped by u.
    std::optional<std::string> checkAllowEquip(GameEntity *u, int location);
    std::optional<std::string> checkAllowRemove(GameEntity *u);

    bool isInsideNormallyDark(GameEntity *viewer);
    bool isInsideDark(GameEntity *viewer);
    bool isProvidingLight();

    // Look at location.
    void lookAtLocation();
    // The business part of the above.
    std::string renderLocationFor(GameEntity* viewer);
    std::string renderListingFor(GameEntity* viewer);
    std::string renderListPrefixFor(GameEntity* viewer);

    // The RoomListingHelper renders the 'room description' for a room listing.
    std::string renderRoomListingHelper(GameEntity* viewer);

    // Although this is virtual, it shouldn't normally need to be overriden.
    std::string renderRoomListingFor(GameEntity* viewer);
    //virtual std::string renderContentsListFor(GameEntity* u);

    std::string renderInventoryListingFor(GameEntity* viewer);
    std::string renderInventoryListingHelper(GameEntity* viewer);

    std::string renderModifiers(GameEntity* viewer);

    std::string renderDiagnostics(GameEntity* viewer);

    std::string renderInventory(GameEntity* viewer);
    std::string renderEquipment(GameEntity* viewer, bool showEmpty = false);

    std::string scriptString();

    std::string getUIDString(bool active = true);


    Object* findObjectVnum(obj_vnum objVnum, bool working = true);
    Object* findObject(const std::function<bool(Object*)> &func, bool working = true);
    std::set<Object*> gatherObjects(const std::function<bool(Object*)> &func, bool working = true);
    virtual DgResults dgCallMember(trig_data *trig, const std::string& member, const std::string& arg);

    std::string getName();
    std::string getAlias();
    std::string getShortDesc();
    std::string getRoomDesc();
    std::string getLookDesc();

    void setName(const std::string& desc);
    void setAlias(const std::string& alias);
    void setShortDesc(const std::string& desc);
    void setRoomDesc(const std::string& desc);
    void setLookDesc(const std::string& desc);

    std::string getDisplayName(GameEntity* ch);
    std::vector<std::string> getKeywords(GameEntity* ch);
    std::string renderAppearance(GameEntity* ch);
    
    void checkMyID();

    // Replacement for existing command interpreter
    virtual void executeCommand(const std::string& cmd);

    // Returns viable landing locations within this, for u.
    //virtual std::vector<GameEntity*> getLandingLocations(GameEntity *u);

    void assignTriggers();

    void sendEvent(const Event& event);
    void sendEventContents(const Event& event);

    void sendText(const std::string& text);
    void sendLine(const std::string& text);
    void sendTextContents(const std::string& text);
    void sendLineContents(const std::string& text);

    // called by this to know what environment it operates under.
    double myEnvVar(EnvVar v);

    // Called by a thing IN this to know what environment it operates under.
    double getEnvVar(EnvVar v);

    // called by a location to aggregate things affecting the environment.
    std::optional<double> emitEnvVar(EnvVar v);
    std::unordered_map<EnvVar, double> envVars;
    
    std::map<int, Destination> getDestinations(GameEntity* viewer);
    std::optional<Destination> getDestination(GameEntity* viewer, int direction);

    bool moveInDirection(int direction, bool need_specials_check = true);
    bool doSimpleMove(int direction, bool need_specials_check = true);

    // this is used for leaving ANY kind of location. not just the command 'leave'.
    bool checkCanLeave(GameEntity *mover, const Destination& dest, bool need_specials_check);

    bool checkCanReachDestination(GameEntity *mover, const Destination& dest);

    // this should be called as dest.target as this.
    bool checkPostEnter(GameEntity *mover, const Location& cameFrom, const Destination& dest);

    template<typename... Args>
    void sendText(fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            sendText(msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in sendText: %s", e.what());
        }
        
    }

    template<typename... Args>
    void sendLineContents(fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            sendLineContents(msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in sendLineContents: %s", e.what());
        }
    }

    template<typename... Args>
    void sendTextContents(fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            sendTextContents(msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in sendTextContents: %s", e.what());
        }
    }

    template<typename... Args>
    void sendLine(fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            sendLine(msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in sendLine: %s", e.what());
        }
    }

    template<typename... Args>
    void sendf(fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::sprintf(format, std::forward<Args>(args)...);
            sendText(msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in sendf: %s", e.what());
        }
    }

    template<typename... Args>
    void sendfContents(fmt::string_view format, Args&&... args) {
        try {
            std::string msg = fmt::sprintf(format, std::forward<Args>(args)...);
            sendTextContents(msg);
        }
        catch(const std::exception &e) {
            basic_mud_log("Error in sendfContents: %s", e.what());
        }
    }

};

/* ================== Memory Structure for Objects ================== */
struct Object : public GameEntity {
    static constexpr auto in_place_delete = true;
    Object() = default;
    explicit Object(const nlohmann::json &j);
    nlohmann::json serialize() override;
    void deserialize(const nlohmann::json& j) override;
    
    void extractFromWorld() override;
    void executeCommand(const std::string& argument) override;

    int getAffectModifier(int location, int specific = -1);
    DgResults dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) override;

    bool isWorking();

    room_vnum room_loaded{NOWHERE};    /* Room loaded in, for room_max checks	*/

    std::array<int64_t, NUM_OBJ_VAL_POSITIONS> value{};   /* Values of the item (see list)    */
    int8_t type_flag{};      /* Type of item                        */
    int level{}; /* Minimum level of object.            */

    weight_t weight{};         /* Weight what else                     */
    weight_t getWeight();
    weight_t getTotalWeight();
    int cost{};           /* Value when sold (gp.)               */
    int cost_per_day{};   /* Cost to keep pr. real day           */
    int timer{};          /* Timer for object                    */
    int size{SIZE_MEDIUM};           /* Size class of object                */

    std::array<obj_affected_type, MAX_OBJ_AFFECT> affected{};  /* affects */

    Object* getInObj();
    Character* getCarriedBy();
    Character* getWornBy();

    Character *sitting{};       /* Who is sitting on me? */
    int scoutfreq{};
    time_t lload{};
    int healcharge{};
    int64_t kicharge{};
    int kitype{};
    Character *user{};
    Character *target{};
    int distance{};
    int foob{};
    int64_t aucter{};
    int64_t curBidder{};
    time_t aucTime{};
    int bid{};
    int startbid{};
    char *auctname{};
    int posttype{};
    Object *posted_to{};
    Object *fellow_wall{};

    std::optional<double> gravity;

};

// A new kind of entity. Entity family Structure.
struct Structure {
    Structure() = default;
    explicit Structure(const nlohmann::json &j);
    void deserialize(const nlohmann::json& j);
    nlohmann::json serialize();

    StructureType type{StructureType::Rooms};
};

/* room-related structures ************************************************/

struct Exit : public GameEntity {
    static constexpr auto in_place_delete = true;
    Exit() = default;
    explicit Exit(const nlohmann::json &j);

    obj_vnum key{NOTHING};        /* Key's number (-1 for no key)		*/
    Room *destination{nullptr};        /* Where direction leads (NOWHERE)	*/
    int dclock{};            /* DC to pick the lock			*/
    int dchide{};            /* DC to find hidden			*/
    int dcskill{};            /* Skill req. to move through exit	*/
    int dcmove{};            /* DC for skill to move through exit	*/

    nlohmann::json serialize() override;
    void deserialize(const nlohmann::json& j) override;
};

enum class MoonCheck : uint8_t {
    NoMoon = 0,
    NotFull = 1,
    Full = 2
};


/* ================== Memory Structure for room ======================= */
struct Room : public GameEntity {
    static constexpr auto in_place_delete = true;
    Room() = default;

    explicit Room(const nlohmann::json &j);
    int sector_type{};            /* sector type (move/hide)            */
    SpecialFunc func{};
    int timed{};                   /* For timed Dt's                     */
    int dmg{};                     /* How damaged the room is            */
    int geffect{};            /* Effect of ground destruction       */
    void executeCommand(const std::string& argument) override;
    std::optional<double> gravity;

    bool isSunken();

    int getDamage();
    int setDamage(int amount);
    int modDamage(int amount);

    nlohmann::json serialize() override;
    void deserialize(const nlohmann::json& j) override;

    std::string renderExits1(entt::entity viewer);
    std::string renderExits2(entt::entity viewer);
    std::string generateMap(entt::entity viewer, int num);
    std::string printMap(entt::entity viewer, int type, int64_t v);
    std::optional<room_vnum> getLaunchDestination();

    MoonCheck checkMoon();

    DgResults dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) override;
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */



/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
    time_info_data() = default;
    explicit time_info_data(int64_t timestamp);
    double remainder{};
    int seconds{}, minutes{}, hours{}, day{}, month{};
    int64_t year{};
    void deserialize(const nlohmann::json& j);
    nlohmann::json serialize();
    // The number of seconds since year 0. Can be negative.
    int64_t current();
};


/* These data contain information about a players time data */
struct time_data {
    time_data() = default;
    explicit time_data(const nlohmann::json &j);
    void deserialize(const nlohmann::json& j);
    int64_t birth{};    /* NO LONGER USED This represents the characters current IC age        */
    time_t created{};    /* This does not change                              */
    int64_t maxage{};    /* This represents death by natural causes (UNUSED) */
    time_t logon{};    /* Time of the last logon (used to calculate played) */
    double played{};    /* This is the total accumulated time played in secs */
    double secondsAged{}; // The player's current IC age, in seconds.
    int currentAge();
    nlohmann::json serialize();
};


/* The pclean_criteria_data is set up in config.c and used in db.c to
   determine the conditions which will cause a player character to be
   deleted from disk if the automagic pwipe system is enabled (see config.c).
*/
struct pclean_criteria_data {
    int level;        /* max level for this time limit	*/
    int days;        /* time limit in days			*/
};


/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs. This structure
 * can be changed freely.
 */
struct alias_data {
    alias_data() = default;
    explicit alias_data(const nlohmann::json &j);
    std::string name;
    std::string replacement;
    int type{};
    nlohmann::json serialize();
};

/* this can be used for skills that can be used per-day */
struct memorize_node {
    int timer;            /* how many ticks till memorized */
    int spell;            /* the spell number */
    struct memorize_node *next;    /* link to the next node */
};

struct innate_node {
    int timer;
    int spellnum;
    struct innate_node *next;
};



/* An affect structure. */
struct affected_type {
    affected_type() = default;
    explicit affected_type(const nlohmann::json& j);
    int16_t type{};          /* The type of spell that caused this      */
    int16_t duration{};      /* For how long its effects will last      */
    double modifier{};         /* This is added to apropriate ability     */
    int location{};         /* Tells which ability to change(APPLY_XXX)*/
    int specific{};         /* Some locations have parameters          */
    bitvector_t bitvector{}; /* Tells which bits to set (AFF_XXX) */
    nlohmann::json serialize();
    struct affected_type *next{};
};

/* Queued spell entry */
struct queued_act {
    int level;
    int spellnum;
};

/* Structure used for chars following other chars */
struct follow_type {
    Character *follower;
    struct follow_type *next;
};


enum ResurrectionMode : uint8_t {
    Costless = 0,
    Basic = 1,
    RPP = 2
};

struct skill_data {
    skill_data() = default;
    explicit skill_data(const nlohmann::json& j);
    int16_t level{0};
    int16_t perfs{0};
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
};

struct trans_data {
    trans_data() = default;
    explicit trans_data(const nlohmann::json& j);

    double timeSpentInForm{0.0};
    bool visible = true;

    double blutz{0.0}; // The number of seconds you can spend in Oozaru.

    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
};


/* ================== Structure for player/non-player base class ===================== */
struct Character : public GameEntity {
    static constexpr auto in_place_delete = true;
    Character() = default;
    // this constructor below is to be used only for the mob_proto map.
    explicit Character(const nlohmann::json& j);

    nlohmann::json serialize() override;
    void deserialize(const nlohmann::json& j) override;

    void executeCommand(const std::string& cmd) override;

    void login();

    bool isPC();
    bool isNPC();

    void ageBy(double addedTime);
    void setAge(double newAge);

    DgResults dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) override;

    std::string juggleRaceName(bool capitalized);

    weight_t getWeight(bool base = false);
    weight_t getTotalWeight();
    weight_t getCurrentBurden();
    double getBurdenRatio();
    bool canCarryWeight(Object *obj);
    bool canCarryWeight(Character *obj);
    bool canCarryWeight(weight_t val);

    int getHeight(bool base = false);
    int setHeight(int val);
    int modHeight(int val);

    int getArmor();

    int getSize();
    int setSize(int val);

    money_t get(CharMoney type);
    money_t set(CharMoney type, money_t val);
    money_t mod(CharMoney type, money_t val);

    align_t get(CharAlign type);
    align_t set(CharAlign type, align_t val);
    align_t mod(CharAlign type, align_t val);

    appearance_t get(CharAppearance type);
    appearance_t set(CharAppearance type, appearance_t val);
    appearance_t mod(CharAppearance type, appearance_t val);

    stat_t get(CharStat type, bool base = true);
    stat_t set(CharStat type, stat_t val);
    stat_t mod(CharStat type, stat_t val);

    int64_t getExperience();
    int64_t setExperience(int64_t value);
    int64_t modExperience(int64_t value, bool applyBonuses = true);

    void gazeAtMoon();

    double getAffectModifier(int location, int specific = -1);

    attribute_t get(CharAttribute attr, bool base = false);
    attribute_t set(CharAttribute attr, attribute_t val);
    attribute_t mod(CharAttribute attr, attribute_t val);

    attribute_train_t get(CharTrain attr);
    attribute_train_t set(CharTrain attr, attribute_train_t val);
    attribute_train_t mod(CharTrain attr, attribute_train_t val);

    void restore(bool announce);

    void ghostify();

    void restore_by(Character *ch);

    void gainTail(bool announce = true);
    void loseTail();
    bool hasTail();

    void hideTransform(FormID form, bool hide);
    void addTransform(FormID form);
    bool removeTransform(FormID form);

    void resurrect(ResurrectionMode mode);

    void teleport_to(IDXTYPE rnum);

    bool in_room_range(IDXTYPE low_rnum, IDXTYPE high_rnum);

    bool in_past();

    bool is_newbie();

    bool in_northran();

    bool can_tolerate_gravity(int grav);

    int calcTier();

    int64_t calc_soft_cap();

    bool is_soft_cap(int64_t type, long double mult);

    bool is_soft_cap(int64_t type);

    int wearing_android_canister();

    int64_t calcGravCost(int64_t num);

    // Stats stuff

    int64_t getCurHealth();

    int64_t getMaxHealth();

    double getCurHealthPercent();

    int64_t getPercentOfCurHealth(double amt);

    int64_t getPercentOfMaxHealth(double amt);

    bool isFullHealth();

    int64_t setCurHealth(int64_t amt);

    int64_t setCurHealthPercent(double amt);

    int64_t incCurHealth(int64_t amt, bool limit_max = true);

    int64_t decCurHealth(int64_t amt, int64_t floor = 0);

    int64_t incCurHealthPercent(double amt, bool limit_max = true);

    int64_t decCurHealthPercent(double amt, int64_t floor = 0);

    void restoreHealth(bool announce = true);

    int64_t healCurHealth(int64_t amt);

    int64_t harmCurHealth(int64_t amt);

    int64_t getMaxPL();

    int64_t getMaxPLTrans();

    int64_t getCurPL();

    int64_t getUnsuppressedPL();

    int64_t getBasePL();

    int64_t getEffBasePL();

    double getCurPLPercent();

    int64_t getPercentOfCurPL(double amt);

    int64_t getPercentOfMaxPL(double amt);

    bool isFullPL();

    int64_t getCurKI();

    int64_t getMaxKI();

    int64_t getBaseKI();

    int64_t getEffBaseKI();

    double getCurKIPercent();

    int64_t getPercentOfCurKI(double amt);

    int64_t getPercentOfMaxKI(double amt);

    bool isFullKI();

    int64_t setCurKI(int64_t amt);

    int64_t setCurKIPercent(double amt);

    int64_t incCurKI(int64_t amt, bool limit_max = true);

    int64_t decCurKI(int64_t amt, int64_t floor = 0);

    int64_t incCurKIPercent(double amt, bool limit_max = true);

    int64_t decCurKIPercent(double amt, int64_t floor = 0);

    void restoreKI(bool announce = true);

    int64_t getCurST();

    int64_t getMaxST();

    int64_t getBaseST();

    int64_t getEffBaseST();

    double getCurSTPercent();

    int64_t getPercentOfCurST(double amt);

    int64_t getPercentOfMaxST(double amt);

    bool isFullST();

    int64_t setCurST(int64_t amt);

    int64_t setCurSTPercent(double amt);

    int64_t incCurST(int64_t amt, bool limit_max = true);

    int64_t decCurST(int64_t amt, int64_t floor = 0);

    int64_t incCurSTPercent(double amt, bool limit_max = true);

    int64_t decCurSTPercent(double amt, int64_t floor = 0);

    void restoreST(bool announce = true);

    int64_t getCurLF();

    int64_t getMaxLF();

    double getCurLFPercent();

    int64_t getPercentOfCurLF(double amt);

    int64_t getPercentOfMaxLF(double amt);

    bool isFullLF();

    int64_t setCurLF(int64_t amt);

    int64_t setCurLFPercent(double amt);

    int64_t incCurLF(int64_t amt, bool limit_max = true);

    int64_t decCurLF(int64_t amt, int64_t floor = 0);

    int64_t incCurLFPercent(double amt, bool limit_max = true);

    int64_t decCurLFPercent(double amt, int64_t floor = 0);

    void restoreLF(bool announce = true);

    bool isFullVitals();

    void restoreVitals(bool announce = true);

    void restoreStatus(bool announce = true);

    void restoreLimbs(bool announce = true);

    int64_t gainBasePL(int64_t amt, bool trans_mult = false);

    int64_t gainBaseKI(int64_t amt, bool trans_mult = false);

    int64_t gainBaseST(int64_t amt, bool trans_mult = false);

    void gainBaseAll(int64_t amt, bool Ftrans_mult = false);

    int64_t loseBasePL(int64_t amt, bool trans_mult = false);

    int64_t loseBaseKI(int64_t amt, bool trans_mult = false);

    int64_t loseBaseST(int64_t amt, bool trans_mult = false);

    void loseBaseAll(int64_t amt, bool trans_mult = false);

    int64_t gainBasePLPercent(double amt, bool trans_mult = false);

    int64_t gainBaseKIPercent(double amt, bool trans_mult = false);

    int64_t gainBaseSTPercent(double amt, bool trans_mult = false);

    void gainBaseAllPercent(double amt, bool trans_mult = false);

    int64_t loseBasePLPercent(double amt, bool trans_mult = false);

    int64_t loseBaseKIPercent(double amt, bool trans_mult = false);

    int64_t loseBaseSTPercent(double amt, bool trans_mult = false);

    void loseBaseAllPercent(double amt, bool trans_mult = false);

    // status stuff
    void cureStatusKnockedOut(bool announce = true);

    void cureStatusBurn(bool announce = true);

    void cureStatusPoison(bool announce = true);

    void setStatusKnockedOut();

    // stats refactor stuff
    weight_t getMaxCarryWeight();

    weight_t getEquippedWeight();

    weight_t getCarriedWeight();

    weight_t getAvailableCarryWeight();

    double speednar();

    int64_t getEffMaxPL();

    bool isWeightedPL();

    void apply_kaioken(int times, bool announce);

    void remove_kaioken(int8_t announce);

    int getRPP();
    void modRPP(int amt);
    int getPractices();
    void modPractices(int amt);

    double currentGravity();

    num_t get(CharNum stat);
    num_t set(CharNum stat, num_t val);
    num_t mod(CharNum stat, num_t val);

    // THE BELOW VARIABLES NEED TO BE MADE PRIVATE EVENTUALLY.
    std::unordered_map<CharNum, num_t> nums{};
    char *title{};
    bool active{false};
    weight_t weight{0};
    struct mob_special_data mob_specials{};
    int size{SIZE_UNDEFINED};
    std::unordered_map<CharMoney, money_t> moneys;
    std::unordered_map<CharAlign, align_t> aligns;
    std::unordered_map<CharAppearance, appearance_t> appearances;
    std::unordered_map<CharStat, stat_t> stats;

    room_vnum was_in_room{NOWHERE};    /* location for linkdead people		*/
    room_vnum hometown{NOWHERE};        /* PC Hometown / NPC spawn room         */
    struct time_data time{};    /* PC's AGE in days			*/
    struct affected_type *affected{};
    /* affected by what spells		*/
    struct affected_type *affectedv{};

    struct descriptor_data *desc{};    /* nullptr for mobiles			*/

    struct script_memory *memory{};    /* for mob memory triggers		*/

    /* For room->people - list		*/
    Character *next{};    /* For either monster or ppl-list	*/
    Character *next_fighting{};
    /* For fighting list			*/
    Character *next_affect{};/* For affect wearoff			*/
    Character *next_affectv{};
    /* For round based affect wearoff	*/

    struct follow_type *followers{};/* List of chars followers		*/
    Character *master{};    /* Who is char following?		*/
    int64_t master_id{};

    Character *fighting{};    /* Opponent				*/

    int8_t position{POS_STANDING};        /* Standing, fighting, sleeping, etc.	*/

    int timer{};            /* Timer for update			*/

    Object *sits{};      /* What am I sitting on? */
    Character *blocks{};    /* Who am I blocking?    */
    Character *blocked{};   /* Who is blocking me?    */
    Character *absorbing{}; /* Who am I absorbing */
    Character *absorbby{};  /* Who is absorbing me */
    Character *carrying{};
    Character *carried_by{};

    int8_t feats[MAX_FEATS + 1]{};    /* Feats (booleans and counters)	*/
    int combat_feats[CFEAT_MAX + 1][FT_ARRAY_MAX]{};
    /* One bitvector array per CFEAT_ type	*/
    int school_feats[SFEAT_MAX + 1]{};/* One bitvector array per CFEAT_ type	*/

    std::map<uint16_t, skill_data> skill;

    std::bitset<NUM_WEARS> bodyparts{};  /* Bitvector for current bodyparts      */

    int armor{0};        /* Internally stored *10		*/

    int64_t exp{};            /* The experience of the player		*/
    int accuracy{};            /* Base hit accuracy			*/
    int accuracy_mod{};        /* Any bonus or penalty to the accuracy	*/
    int damage_mod{};        /* Any bonus or penalty to the damage	*/

    FormID form{FormID::Base};        /* Current form of the character		*/
    double transBonus{0.0};   // Varies from -0.3 to 0.3

    // Data stored about different forms.
    std::unordered_map<FormID, trans_data> transforms;

    int16_t spellfail{};        /* Total spell failure %                 */
    int16_t armorcheck{};        /* Total armorcheck penalty with proficiency forgiveness */
    int16_t armorcheckall{};    /* Total armorcheck penalty regardless of proficiency */

    /* All below added by Iovan for sure o.o */
    int64_t charge{};
    int64_t chargeto{};
    int64_t barrier{};
    char *clan{};
    room_vnum droom{};
    int choice{};
    int sleeptime{};
    int foodr{};
    int altitude{};
    int overf{};
    int spam{};

    room_vnum radar1{};
    room_vnum radar2{};
    room_vnum radar3{};
    int ship{};
    room_vnum shipr{};
    time_t lastpl{};
    time_t lboard[5]{};

    room_vnum listenroom{};
    int crank{};
    int kaioken{};
    int absorbs{};
    int boosts{};
    int upgrade{};
    time_t lastint{};
    int majinize{};
    short fury{};
    short btime{};
    int eavesdir{};
    time_t deathtime{};

    int64_t suppression{};
    Character *drag{};
    Character *dragged{};
    Character *mindlink{};
    int lasthit{};
    int dcount{};
    char *voice{};                  /* PC's snet voice */
    int limbs[4]{};                 /* 0 Right Arm, 1 Left Arm, 2 Right Leg, 3 Left Leg */
    time_t rewtime{};
    Character *grappling{};
    Character *grappled{};
    int grap{};
    int genome[2]{};                /* Bio racial bonus, Genome */
    int combo{};
    int lastattack{};
    int combhits{};
    int ping{};
    int starphase{};
    std::optional<RaceID> mimic{};
    std::bitset<MAX_BONUSES> bonuses{};

    int cooldown{};
    int death_type{};

    int64_t moltexp{};
    int moltlevel{};

    char *loguser{};                /* What user was I last saved as?      */
    int arenawatch{};
    int64_t majinizer{};
    int speedboost{};
    int skill_slots{};
    int tail_growth{};
    int rage_meter{};
    char *feature{};

    int armor_last{};
    int forgeting{};
    int forgetcount{};
    int backstabcool{};
    int con_cooldown{};
    short stupidkiss{};
    char *temp_prompt{};

    int personality{};
    int combine{};
    int linker{};
    int fishstate{};
    int throws{};

    Character *defender{};
    Character *defending{};

    int lifeperc{};
    int gooptime{};
    int blesslvl{};
    Character *poisonby{};
    std::set<Character*> poisoned;

    int mobcharge{};
    int preference{};
    int aggtimer{};

    int lifebonus{};
    int asb{};
    int regen{};
    int con_sdcooldown{};

    int8_t limb_condition[4]{};

    char *rdisplay{};

    Character *original{};

    std::set<Character*> clones{};
    int relax_count{};
    int ingestLearned{};

    int64_t last_tell{-1};        /* idnum of last tell from              */
    void *last_olc_targ{};        /* olc control                          */
    int last_olc_mode{};        /* olc control                          */
    int olc_zone{};            /* Zone where OLC is permitted		*/
    int gauntlet{};                 /* Highest Gauntlet Position */
    char *poofin{};            /* Description on arrival of a god.     */
    char *poofout{};        /* Description upon a god's exit.       */
    int speaking{};            /* Language currently speaking		*/

    int8_t conditions[NUM_CONDITIONS]{};        /* Drunk, full, thirsty			*/
    int practice_points{};        /* Skill points earned from race HD	*/

    int wimp_level{0};        /* Below this # of hit points, flee!	*/
    int8_t freeze_level{};        /* Level of god who froze char, if any	*/
    int16_t invis_level{};        /* level of invisibility		*/
    room_vnum load_room{NOWHERE};        /* Which room to place char in		*/

    std::unordered_map<CharAttribute, attribute_t> attributes;
    std::unordered_map<CharTrain, attribute_train_t> trains;
    double health = 1;
    double energy = 1;
    double stamina = 1;
    double life = 1;
    RaceID race{RaceID::Spirit};
    SenseiID chclass{SenseiID::Commoner};

};

struct ShopKeeper {
    std::shared_ptr<Shop> shopKeeperOf;
};

struct GuildMaster {
    std::shared_ptr<Guild> guildMasterOf;
};

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
    std::unordered_map<std::string, std::shared_ptr<net::Connection>> conns;
    void onConnectionLost(const std::string& connId);
    void onConnectionClosed(const std::string& connId);

    char host[HOST_LENGTH + 1];    /* hostname				*/
    int connected{CON_PLAYING};        /* mode of 'connectedness'		*/

    time_t login_time{time(nullptr)};        /* when the person connected		*/
    char **str{};            /* for the modify-str system		*/
    char *backstr{};        /* backup string for modify-str system	*/
    size_t max_str{};            /* maximum size of string in modify-str	*/
    int32_t mail_to{};        /* name for mail system			*/
    bool has_prompt{true};        /* is the user at a prompt?             */
    std::string last_input;        /* the last input			*/
    std::list<std::string> raw_input_queue, input_queue;
    std::string output;        /* ptr to the current output buffer	*/
    std::list<std::string> history;        /* History of commands, for ! mostly.	*/
    Character *character{};    /* linked to char			*/
    Character *original{};    /* original char if switched		*/
    struct descriptor_data *snooping{}; /* Who is this char snooping	*/
    struct descriptor_data *snoop_by{}; /* And who is snooping this char	*/
    struct descriptor_data *next{}; /* link to next descriptor		*/
    struct oasis_olc_data *olc{};   /* OLC info                            */
    std::shared_ptr<account_data> account{}; /* Account info                        */
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
    Object *obj_point{};
    char *title{};
    double timeoutCounter{0};
    void handle_input();
    void start();
    void handleLostLastConnection(bool graceful);
    void sendText(const std::string &txt);
    void sendEvent(const Event& ev);
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
    nlohmann::json serialize();
    void deserialize(const nlohmann::json &j);
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
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
    int vnum;                             /* vnum of the trigger   */
    struct trig_proto_list *next;         /* next trigger          */
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


enum class SearchType : uint8_t {
    Contents = 0,
    Inventory = 1,
    Equipment = 2,
    Location = 3,
    People = 4,
    World = 5
};

template<typename T>
std::vector<GameEntity*> unitVector(const std::vector<T*>& vec) {
    std::vector<GameEntity*> out;
    for(auto* unit : vec) {
        if(auto o = dynamic_cast<GameEntity*>(unit); o) {
            out.push_back(o);
        }
    }
    return out;
}

template<typename Derived>
class Dispatcher {
    public:
    Dispatcher(GameEntity* caller, const std::string& args) : caller(caller), args(args) {};
    Dispatcher& setFilter(const std::function<bool(GameEntity*)> &f) {
        filter = f;
        return static_cast<Derived&>(*this);
    }
    Dispatcher& setCheckVisible(bool val = true) {
        checkVisible = val;
        return static_cast<Derived&>(*this);
    }
    Dispatcher& addPeople(GameEntity* target) {
        targets.push_back({SearchType::People, target});
        return static_cast<Derived&>(*this);
    }
    Dispatcher& addInventory(GameEntity* target) {
        targets.push_back({SearchType::Inventory, target});
        return static_cast<Derived&>(*this);
    }
    Dispatcher& addEquipment(GameEntity* target) {
        targets.push_back({SearchType::Equipment, target});
        return static_cast<Derived&>(*this);
    }
    Dispatcher& addContents(GameEntity* target) {
        targets.push_back({SearchType::Contents, target});
        return static_cast<Derived&>(*this);
    }
    Dispatcher& addLocation(GameEntity* target) {
        targets.push_back({SearchType::Location, target});
        return static_cast<Derived&>(*this);
    }
    Dispatcher& addWorld() {
        targets.push_back({SearchType::World, nullptr});
        return static_cast<Derived&>(*this);
    }
    protected:
    GameEntity* caller;
    std::string args;
    bool checkVisible{true};
    std::function<bool(GameEntity*)> filter;
    std::vector<std::pair<SearchType, GameEntity*>> targets;
    std::vector<GameEntity*> searchHelper(SearchType type, GameEntity* target) {
        switch(type) {
            case SearchType::Inventory:
                return unitVector(target->getContents());
                break;
            case SearchType::Equipment: {
                std::vector<GameEntity*> out;
                for(auto [id, obj] : target->getEquipment()) {
                    out.push_back(obj);
                }
                return out;
            }
            break;
            case SearchType::Location:
                return unitVector(target->getNeighbors(checkVisible));
                break;
            case SearchType::World: {
                std::vector<GameEntity*> out;
                for(auto [id, obj] : entities) {
                    auto &info = reg.get<Info>(obj);
                    switch(info.family) {
                        case EntityFamily::Character: {
                            auto ch = reg.try_get<Character>(obj);
                            out.push_back(ch);
                        }
                            break;
                        case EntityFamily::Object: {
                            auto o = reg.try_get<Object>(obj);
                            out.push_back(o);
                        }
                            break;
                        case EntityFamily::Room: {
                            auto room = reg.try_get<Room>(obj);
                            out.push_back(room);
                        }
                            break;
                        case EntityFamily::Exit: {
                            auto exit = reg.try_get<Exit>(obj);
                            out.push_back(exit);
                        }
                            break;
                    }
                }
                return out;
            }
            break;
            case SearchType::People:
                return unitVector(target->getPeople());
                break;
        }
    }

    std::vector<GameEntity*> doSearch() {
        std::vector<GameEntity*> out;
        for(auto [type, target] : targets) {
            auto results = searchHelper(type, target);
            // if filter is set, apply it.
            if(filter) {
                std::copy_if(results.begin(), results.end(), std::back_inserter(out), filter);
            } else {
                out.insert(out.end(), results.begin(), results.end());
            }
        }
        std::erase_if(out, [this](auto c) {return c == caller;});
        return out;
    }
};


class Searcher : public Dispatcher<Searcher> {
public:
    Searcher(GameEntity* caller, const std::string& args) : Dispatcher<Searcher>(caller, args) {}

    Searcher& setAllowAll(bool val = true);
    Searcher& setAllowAsterisk(bool val = true);
    Searcher& setAllowSelf(bool val = true);
    Searcher& setAllowHere(bool val = true);
    Searcher& setAllowRecurse(bool val = true);
    std::vector<GameEntity*> search();
    GameEntity* getOne();

    template<typename T>
    std::vector<T*> types() {
        static_assert(std::is_base_of<GameEntity, T>::value, "T must be derived from GameEntity");
        std::vector<T*> filtered;
        for (auto* unit : search()) {
            if (T* casted = dynamic_cast<T*>(unit); casted) {
                filtered.push_back(casted);
            }
        }
        return filtered;
    }

protected:
    bool checkVisible{true};
    bool allowAll{false};
    bool allowAsterisk{false};
    bool allowSelf{false};
    bool allowHere{false};
    bool allowRecurse{false};
};

using MsgVar = std::variant<GameEntity*, std::string>;

class Messager : public Dispatcher<Messager> {
public:
    Messager(GameEntity* caller, const std::string& args) : Dispatcher<Messager>(caller, args) {}
    void deliver();
    void addVar(const std::string& key, MsgVar value);
protected:
    std::unordered_map<std::string, MsgVar> variables;
};