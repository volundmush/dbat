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
    
    std::list<NonPlayerCharacter*> getKeepers();
    bool isProducing(obj_vnum vn);
    bool isProducing(Object *obj1);
    void runPurge();
    int tradeWith(Object* item);
    Object* getSellingObject(BaseCharacter* ch, const std::string& name, NonPlayerCharacter *keeper, bool msg);
    Object* getPurchaseObject(BaseCharacter* ch, const std::string& name, NonPlayerCharacter *keeper, bool msg);
    std::string listObject(Object *obj, int cnt, int aindex, NonPlayerCharacter *keeper, BaseCharacter *ch);
    int64_t buyPrice(Object *obj, NonPlayerCharacter* keeper, BaseCharacter* ch);
    int64_t sellPrice(Object *obj, NonPlayerCharacter* keeper, BaseCharacter* ch);
    bool executeCommand(NonPlayerCharacter *keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments);
    
    void executeBuy(NonPlayerCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments);
    void executeSell(NonPlayerCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments);
    void executeList(NonPlayerCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments);
    void executeValue(NonPlayerCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments);
    void executeAppraise(NonPlayerCharacter* keeper, BaseCharacter *ch, const std::string &cmd, const std::string &arguments);

    bool isOk(NonPlayerCharacter* keeper, BaseCharacter *ch);
    bool isOkChar(NonPlayerCharacter* keeper, BaseCharacter *ch);
    bool isOkObj(NonPlayerCharacter* keeper, BaseCharacter *ch, Object *obj);
    bool isOpen(NonPlayerCharacter* keeper, bool msg);
    

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
    std::list<NonPlayerCharacter*> getMasters();
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
    std::vector<vnum> characters;
    std::set<descriptor_data*> descriptors;
    std::set<net::Connection*> connections;

    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    void modRPP(int amt);

    bool checkPassword(const std::string& password);
    bool setPassword(const std::string& password);

    static int getNextID();

};

struct player_data {
    player_data() = default;
    explicit player_data(const nlohmann::json& j);
    int64_t id{NOTHING};
    std::string name;
    std::shared_ptr<account_data> account{};
    PlayerCharacter* character{};
    std::vector<struct alias_data> aliases;    /* Character's aliases                  */
    std::set<int64_t> sensePlayer;
    std::set<mob_vnum> senseMemory;
    std::map<int64_t, std::string> dubNames;
    char *color_choices[NUM_COLOR]{}; /* Choices for custom colors		*/
    struct txt_block *comm_hist[NUM_HIST]{}; /* Player's communications history     */

    nlohmann::json serialize();
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


struct obj_affected_type {
    obj_affected_type() = default;
    explicit obj_affected_type(const nlohmann::json& j);
    void deserialize(const nlohmann::json& j);
    nlohmann::json serialize();
    int location{};       /* Which ability to change (APPLY_XXX) */
    int specific{};       /* Some locations have parameters      */
    double modifier{};       /* How much it changes by              */
};

struct obj_spellbook_spell {
    int spellname;    /* Which spell is written */
    int pages;        /* How many pages does it take up */
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

enum class UnitFamily : uint8_t {
    Character = 0,
    Object = 1,
    Room = 2,
    Exit = 3,
};

struct coordinates {
    coordinates() = default;
    explicit coordinates(const nlohmann::json& j);
    double x{0};
    double y{0};
    double z{0};
    void clear();
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
    bool operator==(const coordinates& rhs);
};

namespace std {
    template<>
    struct hash<coordinates> {
        size_t operator()(const coordinates& coord) const noexcept {
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
    GameEntity* location{};
    int locationType{};
    coordinates coords{};
    bool operator==(const Location& rhs);
};

struct Destination {
    Destination() = default;
    Destination(GameEntity* target) : target(target) {};
    Destination(const Location& loc) : target(loc.location), locationType(loc.locationType), coords(loc.coords) {};
    GameEntity* target{};
    Exit* via{};
    int direction{-1};
    int locationType{};
    coordinates coords{};
    bool operator==(const Destination& rhs);
};

struct GameEntity : public std::enable_shared_from_this<GameEntity> {
    GameEntity() = default;
    virtual ~GameEntity();

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

    std::unordered_map<std::string, std::shared_ptr<InternedString>> strings;

    std::vector<extra_descr_data> ex_description{}; /* extra descriptions     */

    bool exists{true}; // used for deleted objects. invalid ones are !exists

    std::shared_ptr<script_data> script{};  /* script info for the object */

    std::vector<GameEntity*> contents{};     /* Contains objects  */

    std::unordered_map<FlagType, std::unordered_set<int>> flags;
    virtual bool checkFlag(FlagType type, int flag);
    virtual void setFlag(FlagType type, int flag, bool value = true);
    virtual void clearFlag(FlagType type, int flag);
    virtual bool flipFlag(FlagType type, int flag);
    std::vector<std::string> getFlagNames(FlagType type);

    weight_t getInventoryWeight();
    int64_t getInventoryCount();

    /* Equipment array			*/
    std::vector<GameEntity*> getContents();
    std::vector<Object*> getInventory();
    std::vector<BaseCharacter*> getPeople();
    std::map<int, Object*> getEquipment();
    std::vector<Room*> getRooms();
    std::map<int, Exit*> getExits();
    std::map<int, Exit*> getUsableExits();

    Room* getAbsoluteRoom();
    Room* getRoom();
    GameEntity* getLocation();

    Location getLocationInfo();

    // removeFromLocation is called first, and handleRemove is called BY removeFromLocation on its location.
    virtual void removeFromLocation();
    virtual void handleRemove(GameEntity *u);

    // As above, the process is to call addToLocation which will then call handleAdd ON the location.
    virtual void addToLocation(const Destination &dest);
    virtual void handleAdd(GameEntity *u);

    // called for this, when mover has changed coordinates within it.
    virtual void updateCoordinates(GameEntity *mover, std::optional<coordinates> previous = std::nullopt);

    // the GameEntity you are located in. Which might be null.
    GameEntity *location{nullptr};
    // LocationType is extra information about how you are inside <location>.
    // For instance, if you are a character, then the locationType of units
    // you contain should represent inventory/equipment slots. 0 is inventory,
    // positive number is equipment slot.
    int16_t locationType{0};
    coordinates coords{};

    // Returns a collection of 'units within reach' for commands like look or get.
    std::vector<GameEntity*> getNeighbors(bool visible = true);

    // The business part of the above.
    virtual std::vector<GameEntity*> getNeighborsFor(GameEntity* u, bool visible = true);

    virtual std::vector<std::pair<std::string, Destination>> getLandingSpotsFor(GameEntity *mover);
    virtual std::optional<Destination> getLaunchDestinationFor(GameEntity *mover);

    virtual bool isInvisible();
    virtual bool isHidden();
    virtual bool isAdminInvisible();

    virtual bool canSee(GameEntity *target);
    virtual bool canSeeInvisible();
    virtual bool canSeeHidden();
    virtual bool canSeeAdminInvisible();

    // whether this has some form of nightvision.
    virtual bool canSeeInDark();

    // NOTE: all functions beginning with check return an empty optional if it's OK.
    // the optional string contains the reason why it's NOT OK, otherwise.

    // Whether u can access this unit's inventory.
    virtual std::optional<std::string> checkAllowInventoryAcesss(GameEntity *u);

    // whether this unit will allow u to take/give/drop it. give/put/drop are identical.
    virtual std::optional<std::string> checkIsGettable(GameEntity *u);
    virtual std::optional<std::string> checkIsGivable(GameEntity *u);

    // if this can store u in its inventory. This includes a room accepting items.
    // Things to check might be weight, item capacity, whether it's appropriate for the space, etc.
    virtual std::optional<std::string> checkCanStore(GameEntity *u);

    // Check to see whether giver can give u to this.
    virtual std::optional<std::string> checkAllowReceive(GameEntity *giver, GameEntity *u);

    // Whether this can be equipped by u.
    virtual std::optional<std::string> checkAllowEquip(GameEntity *u, int location);
    virtual std::optional<std::string> checkAllowRemove(GameEntity *u);

    virtual bool isInsideNormallyDark(GameEntity *viewer);
    virtual bool isInsideDark(GameEntity *viewer);
    virtual bool isProvidingLight();

    // Look at location.
    virtual void lookAtLocation();
    // The business part of the above.
    virtual std::string renderLocationFor(GameEntity* viewer);
    virtual std::string renderListingFor(GameEntity* viewer);
    virtual std::string renderListPrefixFor(GameEntity* viewer);

    // The RoomListingHelper renders the 'room description' for a room listing.
    virtual std::string renderRoomListingHelper(GameEntity* viewer);

    // Although this is virtual, it shouldn't normally need to be overriden.
    virtual std::string renderRoomListingFor(GameEntity* viewer);
    //virtual std::string renderContentsListFor(GameEntity* u);

    virtual std::string renderInventoryListingFor(GameEntity* viewer);
    virtual std::string renderInventoryListingHelper(GameEntity* viewer);

    virtual std::string renderModifiers(GameEntity* viewer);

    virtual std::string renderDiagnostics(GameEntity* viewer);

    virtual std::string renderInventory(GameEntity* viewer);
    virtual std::string renderEquipment(GameEntity* viewer, bool showEmpty = false);

    void activateContents();
    void deactivateContents();

    virtual void deserialize(const nlohmann::json& j);
    virtual void deserializeRelations(const nlohmann::json& j);
    virtual nlohmann::json serialize();
    virtual nlohmann::json serializeRelations();
    
    virtual std::string scriptString();

    std::string getUIDString(bool active = true);
    virtual bool isActive();

    Object* findObjectVnum(obj_vnum objVnum, bool working = true);
    virtual Object* findObject(const std::function<bool(Object*)> &func, bool working = true);
    virtual std::set<Object*> gatherObjects(const std::function<bool(Object*)> &func, bool working = true);
    virtual DgResults dgCallMember(trig_data *trig, const std::string& member, const std::string& arg);

    virtual std::string getName();
    virtual std::string getAlias();
    virtual std::string getShortDesc();
    virtual std::string getRoomDesc();
    virtual std::string getLookDesc();

    virtual void setName(const std::string& desc);
    virtual void setAlias(const std::string& alias);
    virtual void setShortDesc(const std::string& desc);
    virtual void setRoomDesc(const std::string& desc);
    virtual void setLookDesc(const std::string& desc);

    virtual std::string getDisplayName(GameEntity* ch);
    virtual std::vector<std::string> getKeywords(GameEntity* ch);
    virtual std::string renderAppearance(GameEntity* ch);

    virtual UnitFamily getFamily() = 0;
    virtual std::string getUnitClass() = 0;
    
    void checkMyID();

    // The object which provides environmental data for this unit.
    // An example would be current gravity. Rooms are one example of an environment, but
    // rooms might get their environment from a region/structure too.
    GameEntity* getEnvironment();

    virtual bool isEnvironment();

    // The object which is 'roughly where the unit is.' This is likely a planet, dimension,
    // or similar major boundary.
    GameEntity* getRegion();

    virtual bool isRegion();

    // The object which represents a point of entry/exit. This is likely a vehicle, structure, planet, etc.
    // You can generally board/land/enter these things and similarly leave/fly out of them.
    GameEntity* getStructure();

    virtual bool isStructure();

    // Planets are a special kind of structure. They are effectively top-level structures for many areas
    // and have their own special uses like checking for having moons or being able to 'fly space' from them.
    // This can be used for things that are also LIKE planets too, such as moons, large asteroids, floating
    // islands in space, whatever.
    GameEntity* getPlanet();

    virtual bool isPlanet();

    // Replacement for existing command interpreter
    virtual void executeCommand(const std::string& cmd);

    // Returns viable landing locations within this, for u.
    //virtual std::vector<GameEntity*> getLandingLocations(GameEntity *u);

    virtual void assignTriggers();

    virtual void sendEvent(const Event& event);
    virtual void sendEventContents(const Event& event);

    virtual void sendText(const std::string& text);
    void sendLine(const std::string& text);
    virtual void sendTextContents(const std::string& text);
    void sendLineContents(const std::string& text);

    // called by this to know what environment it operates under.
    virtual double myEnvVar(EnvVar v);

    // Called by a thing IN this to know what environment it operates under.
    virtual double getEnvVar(EnvVar v);

    // called by a location to aggregate things affecting the environment.
    virtual std::optional<double> emitEnvVar(EnvVar v);
    std::unordered_map<EnvVar, double> envVars;
    
    virtual std::map<int, Destination> getDestinations(GameEntity* viewer);
    virtual std::optional<Destination> getDestination(GameEntity* viewer, int direction);

    virtual bool moveInDirection(int direction, bool need_specials_check = true);
    virtual bool doSimpleMove(int direction, bool need_specials_check = true);

    // this is used for leaving ANY kind of location. not just the command 'leave'.
    virtual bool checkCanLeave(GameEntity *mover, const Destination& dest, bool need_specials_check);

    virtual bool checkCanReachDestination(GameEntity *mover, const Destination& dest);

    // this should be called as dest.target as this.
    virtual bool checkPostEnter(GameEntity *mover, const Location& cameFrom, const Destination& dest);

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
    Object() = default;
    explicit Object(const nlohmann::json &j);
    ~Object() override;
    
    void extractFromWorld() override;

    void executeCommand(const std::string& argument) override;

    UnitFamily getFamily() override;
    std::string getUnitClass() override;

    nlohmann::json serialize() override;
    nlohmann::json serializeRelations() override;
    void deserializeRelations(const nlohmann::json& j) override;
    void deserialize(const nlohmann::json& j) override;

    std::string renderRoomListingHelper(GameEntity* u) override;
    std::string renderListPrefixFor(GameEntity* viewer) override;
    std::string renderModifiers(GameEntity* viewer);

    std::string renderInventoryListingHelper(GameEntity* viewer) override;
    std::string renderInventoryListingFor(GameEntity* viewer) override;

    std::string renderDiagnostics(GameEntity* viewer) override;

    virtual std::string renderAppearanceHelper(GameEntity* viewer);

    std::string renderAppearance(GameEntity* viewer) override;

    void activate();

    void deactivate();

    int getAffectModifier(int location, int specific = -1);
    DgResults dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) override;

    bool active{false};
    bool isActive() override;

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

    Object *in_obj{};       /* In what object nullptr when none    */
    BaseCharacter *carried_by{};  /* Carried by :nullptr in room/conta   */
    BaseCharacter *worn_by{};      /* Worn by? */
    int16_t worn_on{-1};          /* Worn where?		      */

    Object *next{};         /* For the object list              */

    struct obj_spellbook_spell *sbinfo{};  /* For spellbook info */
    BaseCharacter *sitting{};       /* Who is sitting on me? */
    int scoutfreq{};
    time_t lload{};
    int healcharge{};
    int64_t kicharge{};
    int kitype{};
    BaseCharacter *user{};
    BaseCharacter *target{};
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
    BaseCharacter *owner{};

    std::optional<double> gravity;

    bool isProvidingLight() override;
    double currentGravity();

    void assignTriggers() override;

};
/* ======================================================================= */

// Type: ITEM_PLANT should always use this.
struct Plant : public Object {
    Plant() = default;
    explicit Plant(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderRoomListingHelper(GameEntity* u) override;
};

// Type: Vnum 11 alyways...
struct GravityGenerator : public Object {
    GravityGenerator() = default;
    explicit GravityGenerator(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderRoomListingHelper(GameEntity* u) override;
};

struct GlacialWall : public Object {
    GlacialWall() = default;
    explicit GlacialWall(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderRoomListingHelper(GameEntity* u) override;
};

struct DrinkContainer : public Object {
    DrinkContainer() = default;
    explicit DrinkContainer(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearanceHelper(GameEntity* u) override;
};

struct Food : public Object {
    Food() = default;
    explicit Food(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearanceHelper(GameEntity* u) override;
};

struct Corpse : public Object {
    Corpse() = default;
    explicit Corpse(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearanceHelper(GameEntity* u) override;
};

struct Weapon : public Object {
    Weapon() = default;
    explicit Weapon(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearanceHelper(GameEntity* u) override;
};

// STRUCTURE STUFF BELOW THIS.

// TileDetails is used to store information about a tile. Part of the Grid3D system.
struct TileDetails {
    std::string name;
    std::string description;
    std::optional<int> tile;
    std::set<int> flags;
};

struct Grid3D {
    int defaultSectorFloor{SECT_FIELD};
    int defaultSectorAbove{SECT_FLYING};
    int defaultSectorBelow{SECT_UNDERWATER};
    std::unordered_map<coordinates, TileDetails> tiles;
};

struct CoordinateContents {
    std::unordered_map<coordinates, std::vector<GameEntity*>> coordinateContents;
};

struct Boundaries {
    std::optional<double> maxX, maxY, maxZ, minX, minY, minZ;
};

// A new kind of entity. Entity family Structure.
struct Structure : public GameEntity {
    Structure() = default;
    explicit Structure(const nlohmann::json &j);

    //void extractFromWorld() override;

    void handleRemove(GameEntity *u) override;
    void handleAdd(GameEntity *u) override;
    void updateCoordinates(GameEntity *mover, std::optional<coordinates> previous = std::nullopt) override;

    std::map<int, Destination> getDestinations(GameEntity* viewer) override;
    bool checkCanLeave(GameEntity *mover, const Destination& dest, bool need_specials_check) override;
    bool checkCanReachDestination(GameEntity *mover, const Destination& dest) override;
    bool checkPostEnter(GameEntity *mover, const Location& cameFrom, const Destination& dest) override;
    std::string renderLocationFor(GameEntity* viewer) override;

    UnitFamily getFamily() override;
    std::string getUnitClass() override;

    nlohmann::json serialize() override;
    nlohmann::json serializeRelations() override;
    void deserializeRelations(const nlohmann::json& j) override;
    void deserialize(const nlohmann::json& j) override;

    bool isInsideNormallyDark(GameEntity* viewer) override;
    bool isInsideDark(GameEntity* viewer) override;
    bool isEnvironment() override;
    bool isStructure() override;

    std::vector<std::pair<std::string, Destination>> getLandingSpotsFor(GameEntity *mover) override;
    std::optional<Destination> getLaunchDestinationFor(GameEntity *mover) override;

    StructureType type{StructureType::Rooms};
};

// Vehicles. Spaceships, boats, submarines, fighter jets, cars, buses...
struct Vehicle : public Structure {
    Vehicle() = default;
    explicit Vehicle(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearance(GameEntity* u) override;

    void sendText(const std::string& text) override;

    void executeCommand(const std::string& argument) override;
};

// Note: Also used for moons, planetoids, dwarf planets, gigantic asteroids and... 
// I dunno, floating islands in space? Like the world of Golden Sun?
struct Planet : public Structure {
    Planet() = default;
    explicit Planet(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearance(GameEntity* u) override;
};

// Used for things like space stations, large buildings, possibly elaborate dungeons.
struct Building : public Structure {
    Building() = default;
    explicit Building(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearance(GameEntity* u) override;
};

// Used for regions - usually wilderness areas like forests, plains, deserts, mountaintops, etc.
struct Region : public Structure {
    Region() = default;
    explicit Region(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearance(GameEntity* u) override;
};

struct Dimension : public Structure {
    Dimension() = default;
    explicit Dimension(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearance(GameEntity* u) override;
};


struct Interstellar : public Structure {
    Interstellar() = default;
    explicit Interstellar(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearance(GameEntity* u) override;
};

struct Stellar : public Structure {
    Stellar() = default;
    explicit Stellar(const nlohmann::json &j);

    std::string getUnitClass() override;
    std::string renderAppearance(GameEntity* u) override;
};



/* room-related structures ************************************************/

struct Exit : public GameEntity {
    Exit() = default;
    explicit Exit(const nlohmann::json &j);

    obj_vnum key{NOTHING};        /* Key's number (-1 for no key)		*/
    Room *destination{nullptr};        /* Where direction leads (NOWHERE)	*/
    int dclock{};            /* DC to pick the lock			*/
    int dchide{};            /* DC to find hidden			*/
    int dcskill{};            /* Skill req. to move through exit	*/
    int dcmove{};            /* DC for skill to move through exit	*/
    int failsavetype{};        /* Saving Throw type on skill fail	*/
    int dcfailsave{};        /* DC to save against on fail		*/
    Room *failroom{nullptr};        /* Room # to put char in when fail > 5  */
    Room *totalfailroom{nullptr};        /* Room # if char fails save < 5	*/

    Room* getDestination();

    UnitFamily getFamily() override;
    std::string getUnitClass() override;
    nlohmann::json serializeRelations() override;
    nlohmann::json serialize() override;
    void deserializeRelations(const nlohmann::json& j) override;
    void deserialize(const nlohmann::json& j) override;

    std::string getName() override;
    //std::string getAlias() override;
    std::vector<std::string> getKeywords(GameEntity* ch) override;
};

enum class MoonCheck : uint8_t {
    NoMoon = 0,
    NotFull = 1,
    Full = 2
};


/* ================== Memory Structure for room ======================= */
struct Room : public GameEntity {
    Room() = default;

    UnitFamily getFamily() override;
    std::string getUnitClass() override;

    explicit Room(const nlohmann::json &j);
    int sector_type{};            /* sector type (move/hide)            */
    SpecialFunc func{};
    int timed{};                   /* For timed Dt's                     */
    int dmg{};                     /* How damaged the room is            */
    int geffect{};            /* Effect of ground destruction       */

    std::optional<double> gravity;

    void executeCommand(const std::string& argument) override;

    bool isSunken();

    int getDamage();
    int setDamage(int amount);
    int modDamage(int amount);

    nlohmann::json serialize() override;
    void deserialize(const nlohmann::json& j) override;

    bool isActive() override;

    std::string renderLocationFor(GameEntity* u) override;

    std::string renderExits1(GameEntity* u);
    std::string renderExits2(GameEntity* u);
    std::string generateMap(GameEntity* viewer, int num);
    std::string printMap(GameEntity* viewer, int type, int64_t v);
    std::optional<room_vnum> getLaunchDestination();

    MoonCheck checkMoon();

    DgResults dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) override;

    void assignTriggers() override;

    //Event renderLocationFor(GameEntity* u) override;

    bool isInsideNormallyDark(GameEntity* viewer) override;
    bool isInsideDark(GameEntity* viewer) override;
    bool isEnvironment() override;

    double getEnvVar(EnvVar v) override;

    std::map<int, Destination> getDestinations(GameEntity* viewer) override;
    bool checkPostEnter(GameEntity* mover, const Location& loc, const Destination& dest) override;
    bool checkCanReachDestination(GameEntity *mover, const Destination& dest) override;
    bool checkCanLeave(GameEntity* mover, const Destination& dest, bool need_specials_check = true) override;


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
    BaseCharacter *follower;
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
struct BaseCharacter : public GameEntity {
    BaseCharacter() = default;
    ~BaseCharacter() override;
    // this constructor below is to be used only for the mob_proto map.
    explicit BaseCharacter(const nlohmann::json& j);

    nlohmann::json serialize() override;
    void deserialize(const nlohmann::json& j) override;

    void activate();
    void deactivate();
    void login();

    void executeCommand(const std::string& cmd) override;

    nlohmann::json serializeRelations() override;
    void deserializeRelations(const nlohmann::json& j) override;

    std::vector<std::string> baseKeywordsFor(GameEntity* looker);

    virtual std::string renderRoomListName(GameEntity* looker) = 0;
    std::string renderRoomListingHelper(GameEntity* u) override;
    std::string renderRoomListingFor(GameEntity* u) override;
    virtual std::string renderStatusLines(GameEntity* viewer);

    std::string renderDiagnostics(GameEntity* viewer) override;
    std::string renderAppearance(GameEntity* viewer) override;

    std::optional<std::string> getDubFor(BaseCharacter* target);

    bool moveInDirection(int direction, bool need_specials_check = true) override;
    bool doSimpleMove(int direction, bool need_specials_check = true) override;

    virtual bool isPC() = 0;
    virtual bool isNPC() = 0;
    void lookAtLocation() override;

    bool isActive() override;

    void ageBy(double addedTime);
    void setAge(double newAge);

    DgResults dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) override;

    Object* findObject(const std::function<bool(Object*)> &func, bool working = true) override;
    std::set<Object*> gatherObjects(const std::function<bool(Object*)> &func, bool working = true) override;

    weight_t getWeight(bool base = false);
    weight_t getTotalWeight();
    weight_t getCurrentBurden();
    double getBurdenRatio();
    bool canCarryWeight(Object *obj);
    bool canCarryWeight(BaseCharacter *obj);
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

    room_vnum normalizeLoadRoom(room_vnum in);

    double getAffectModifier(int location, int specific = -1);

    attribute_t get(CharAttribute attr, bool base = false);
    attribute_t set(CharAttribute attr, attribute_t val);
    attribute_t mod(CharAttribute attr, attribute_t val);

    attribute_train_t get(CharTrain attr);
    attribute_train_t set(CharTrain attr, attribute_train_t val);
    attribute_train_t mod(CharTrain attr, attribute_train_t val);

    bool isInvisible() override;
    bool isHidden() override;
    bool isAdminInvisible() override;
    bool canSeeInvisible() override;
    //bool canSeeHidden() override;
    bool canSeeAdminInvisible() override;
    bool canSeeInDark() override;
    bool isProvidingLight() override;

    void sendText(const std::string& text) override;
    void sendEvent(const Event& event) override;

    std::string juggleRaceName(bool capitalized);

    void restore(bool announce);

    void ghostify();

    void restore_by(BaseCharacter *ch);

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

    UnitFamily getFamily() override;

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
    BaseCharacter *next{};    /* For either monster or ppl-list	*/
    BaseCharacter *next_fighting{};
    /* For fighting list			*/
    BaseCharacter *next_affect{};/* For affect wearoff			*/
    BaseCharacter *next_affectv{};
    /* For round based affect wearoff	*/

    struct follow_type *followers{};/* List of chars followers		*/
    BaseCharacter *master{};    /* Who is char following?		*/
    int64_t master_id{};

    BaseCharacter *fighting{};    /* Opponent				*/

    int8_t position{POS_STANDING};        /* Standing, fighting, sleeping, etc.	*/

    int timer{};            /* Timer for update			*/

    Object *sits{};      /* What am I sitting on? */
    BaseCharacter *blocks{};    /* Who am I blocking?    */
    BaseCharacter *blocked{};   /* Who is blocking me?    */
    BaseCharacter *absorbing{}; /* Who am I absorbing */
    BaseCharacter *absorbby{};  /* Who is absorbing me */
    BaseCharacter *carrying{};
    BaseCharacter *carried_by{};

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
    BaseCharacter *drag{};
    BaseCharacter *dragged{};
    BaseCharacter *mindlink{};
    int lasthit{};
    int dcount{};
    char *voice{};                  /* PC's snet voice */
    int limbs[4]{};                 /* 0 Right Arm, 1 Left Arm, 2 Right Leg, 3 Left Leg */
    time_t rewtime{};
    BaseCharacter *grappling{};
    BaseCharacter *grappled{};
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

    BaseCharacter *defender{};
    BaseCharacter *defending{};

    int lifeperc{};
    int gooptime{};
    int blesslvl{};
    BaseCharacter *poisonby{};
    std::set<BaseCharacter*> poisoned;

    int mobcharge{};
    int preference{};
    int aggtimer{};

    int lifebonus{};
    int asb{};
    int regen{};
    int con_sdcooldown{};

    int8_t limb_condition[4]{};

    char *rdisplay{};

    BaseCharacter *original{};

    std::set<BaseCharacter*> clones{};
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

struct NonPlayerCharacter : public BaseCharacter {
    NonPlayerCharacter() = default;
    explicit NonPlayerCharacter(const nlohmann::json &j);

    std::string getUnitClass() override;

    void assignTriggers() override;

    void deserialize(const nlohmann::json& j) override;
    nlohmann::json serialize() override;

    bool isPC() override;
    bool isNPC() override;

    std::string getDisplayName(GameEntity* looker) override;
    std::vector<std::string> getKeywords(GameEntity* looker) override;

    std::string renderRoomListName(GameEntity* looker) override;

    std::shared_ptr<Shop> shopKeeperOf;
    std::shared_ptr<Guild> guildMasterOf;

};

struct PlayerCharacter : public BaseCharacter {
    PlayerCharacter() = default;
    explicit PlayerCharacter(const nlohmann::json &j);

    void onHolderExtraction() override;
    void extractFromWorld() override;

    std::string getUnitClass() override;

    void deserialize(const nlohmann::json& j) override;
    nlohmann::json serialize() override;

    std::string getDisplayName(GameEntity* looker) override;
    std::vector<std::string> getKeywords(GameEntity* looker) override;

    std::string renderRoomListName(GameEntity* looker) override;

    bool isPC() override;
    bool isNPC() override;
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
    BaseCharacter *character{};    /* linked to char			*/
    BaseCharacter *original{};    /* original char if switched		*/
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
                for(auto [id, obj] : world) {
                    out.push_back(obj);
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