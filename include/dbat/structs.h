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
    std::function<double(struct char_data *ch)> func{};

    character_affect_type(int loc, double mod, int spec, std::function<double(struct char_data *ch)> f = {})
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

struct account_data {
    account_data() = default;
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
    std::unordered_map<int64_t, std::string> connections;

    void modRPP(int amt);

    bool canBeDeleted();

    static int getNextID();

};

struct player_data {
    int id{NOTHING};
    std::string name;
    struct account_data *account{};
    struct char_data *character{};
    std::vector<struct alias_data> aliases;    /* Character's aliases                  */
    std::unordered_set<int> sense_player;
    std::unordered_set<mob_vnum> sense_memory;
    std::map<int, std::string> dub_names;
    char *color_choices[NUM_COLOR]{}; /* Choices for custom colors		*/
    struct txt_block *comm_hist[NUM_HIST]{}; /* Player's communications history     */
};

struct cmdlist_element {
    char *cmd{};                /* one line of a trigger */
    struct cmdlist_element *original{};
    struct cmdlist_element *next{};
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

struct trig_proto_data {
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

/* structure for triggers */
struct trig_data : public HasVariables, std::enable_shared_from_this<trig_data> {
    trig_data() = default;
    trig_data(const trig_proto_data &other);
    trig_proto_data* proto{};
    int getVnum() const;
    UnitType getAttachType() const;
    long getTriggerType() const;
    DgScriptState state{DgScriptState::READY}; /* current state of the script */
    std::vector<DepthType> depth_stack{};
    int current_line{};
    double waiting{0.0};    /* event to pause the trigger      */
    unit_data* owner{};

    bool active{false};
    void activate();
    void deactivate();

    std::unordered_set<std::string> subscriptions; // Subscriptions to services.

    std::shared_ptr<trig_data> shared();

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


struct picky_data {
    std::unordered_set<MoralAlign> only_alignment, not_alignment;    /* Neutral, lawful, etc.		*/
    std::unordered_set<Sensei> only_sensei, not_sensei;    /* Only these classes can shop here	*/
    std::unordered_set<Race> only_race, not_race;    /* Only these races can shop here	*/
};

struct unit_data : public HasVariables {
    unit_data& operator=(const proto_data& other);
    virtual ~unit_data();

    // re-adding vnum in and type so we can tell what kind of thing it is for some debugging and functions.
    vnum vn{NOTHING}; // The vnum of the unit.
    UnitType type{UnitType::unknown};

    int id{NOTHING}; /* the unique ID of this entity */
    time_t generation{}; /* creation time for dupe check     */

    virtual vnum getVnum() const; // Returns the vnum of the unit.

    const char* getName() const;
    const char* getRoomDescription() const;
    const char* getLookDescription() const;
    const char* getShortDescription() const;
    std::string_view getString(const std::string &key) const; // Returns a string from the strings map.

    const std::vector<ExtraDescription>& getExtraDescription() const; // Returns the extra description data.

    std::unordered_map<std::string, std::string> strings;
    std::vector<ExtraDescription> extra_descriptions; // Extra descriptions for this unit.
    FlagHandler<AffectFlag> affect_flags{}; /* To set chars bits          */
    
    long trigger_types{};                /* bitvector of trigger types */
    std::optional<std::vector<vnum>> running_scripts; /* list of attached scripts. the order matters. Only used if differs from proto scripts.*/
    std::unordered_map<trig_vnum, std::shared_ptr<trig_data>> scripts; /* list of attached triggers. accessed in order of running_scripts */
    std::unordered_map<std::string, std::string> script_variables;

    void activateScripts();
    void deactivateScripts();
    std::vector<trig_vnum> getScriptOrder(); /* this will return running_scripts if said, or the results of getProtoScripts() */
    std::vector<std::weak_ptr<trig_data>> getScripts();
    virtual std::vector<trig_vnum> getProtoScript() const = 0;
    virtual std::string scriptString() const;

    weight_t getInventoryWeight();
    int64_t getInventoryCount();

    std::list<std::weak_ptr<obj_data>> objects{};
    std::vector<std::weak_ptr<obj_data>> getObjects();

    void activateContents();
    void deactivateContents();

    std::string getUID(bool active = false);
    virtual bool isActive() = 0;

    struct obj_data* findObjectVnum(obj_vnum objVnum, bool working = true);
    virtual struct obj_data* findObject(const std::function<bool(struct obj_data*)> &func, bool working = true);
    virtual std::unordered_set<struct obj_data*> gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working = true);

    virtual double getAffectModifier(uint64_t location, uint64_t specific);

    std::unordered_set<std::string> subscriptions{}; // Subscriptions to services.

    std::unordered_map<std::string, double> stats;

    // Location data - for debugging and future unified location system
    // These aren't used by all units, but putting it here means debug can see them.
    unit_data* location{nullptr};  // What unit contains this unit (room, area, char, obj)
    double pos_x{0.0}, pos_y{0.0}, pos_z{0.0};  // Position within location

};

struct room_direction_data;

struct thing_data : public unit_data {
    struct room_data* getRoom() const;
    room_vnum getRoomVnum() const;

    std::string getLocationName() const;
    room_direction_data* getLocationExit(int dir) const;
    std::map<int, room_direction_data*> getLocationExits() const;

    double getLocationEnvironment(int type) const;
    double setLocationEnvironment(int type, double value) const;
    double modLocationEnvironment(int type, double value) const;
    void clearLocationEnvironment(int type) const;

    void setRoomFlag(int flag, bool value = true) const;
    bool toggleRoomFlag(int flag) const;
    bool getRoomFlag(int flag) const;
    void setWhereFlag(WhereFlag flag, bool value = true) const;
    bool toggleWhereFlag(WhereFlag flag) const;
    bool getWhereFlag(WhereFlag flag) const;

    void broadcastAtLocation(const std::string& message) const;

    std::vector<std::weak_ptr<obj_data>> getLocationObjects() const;
    std::vector<std::weak_ptr<char_data>> getLocationPeople() const;

    int getLocationDamage() const;
    int setLocationDamage(int amount) const;
    int modLocationDamage(int amount) const;

    int getLocationTileType() const;

    int getLocationGroundEffect() const;
    int setLocationGroundEffect(int val) const;
    int modLocationGroundEffect(int val) const;

    SpecialFunc getLocationSpecialFunc() const;

};

// base struct for both npc_proto_data and item_proto_data
struct proto_data {
    virtual ~proto_data();
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

    proto_data& operator=(const proto_data& other);
};

struct item_proto_data : public proto_data, public picky_data {
    item_proto_data() = default;
    item_proto_data(const obj_data& other);
    
    item_proto_data& operator=(const item_proto_data& other);

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

/* ================== Memory Structure for Objects ================== */
struct obj_data : public thing_data, public picky_data, std::enable_shared_from_this<obj_data> {
    obj_data();
    ~obj_data() override;
    obj_data& operator=(const item_proto_data& proto);
    //~obj_data() override = default;
    std::vector<trig_vnum> getProtoScript() const override;
    void deserializeLocation(const std::string& txt, double x, double y, double z);
    void activate();
    void deactivate();
    double getAffectModifier(uint64_t location, uint64_t specific) override;

    void commit_iedit(const item_proto_data &proto);

    item_proto_data* getProto() const;

    bool active{false};
    bool isActive() override;

    struct room_data* getAbsoluteRoom();
    bool isWorking();
    void clearLocation();

    std::shared_ptr<obj_data> shared();

    room_vnum room_loaded{NOWHERE};    /* Room loaded in, for room_max checks	*/

    /* arbitrary named doubles */
    ItemType type_flag{ItemType::unknown};      /* Type of item                        */
    
    FlagHandler<WearFlag> wear_flags{}; /* Where you can wear it     */
    FlagHandler<ItemFlag> item_flags{}; /* If it hums, glows, etc.  */
    
    Size size{Size::medium};           /* Size class of object                */

    std::array<affected_type, MAX_OBJ_AFFECT> affected;  /* affects */

    obj_data *getContainer() const;
    char_data *getCarriedBy() const;
    char_data *getWornBy() const;
    int16_t getWornOn() const;

    std::weak_ptr<char_data> sitting{};       /* Who is sitting on me? */
    struct char_data *user{};
    struct char_data *target{};
    char *auctname{};
    struct obj_data *posted_to{};
    struct obj_data *fellow_wall{};

    int64_t aucter{};
    int64_t curBidder{};
    time_t aucTime{};
    int bid{};
    int startbid{};
    int posttype{};

    bool isProvidingLight();
    double currentGravity();

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
/* ======================================================================= */

/* room-related structures ************************************************/

struct room_direction_data {
    ~room_direction_data();
    char *general_description{};       /* When look DIR.			*/
    char *keyword{};        /* for open/close			*/

    int16_t exit_info{};        /* Exit info			*/
    obj_vnum key{NOTHING};        /* Key's number (-1 for no key)		*/
    room_rnum to_room{NOWHERE};        /* Where direction leads (NOWHERE)	*/
    int dclock{};            /* DC to pick the lock			*/
    int dchide{};            /* DC to find hidden			*/
    int dcskill{};            /* Skill req. to move through exit	*/
    int dcmove{};            /* DC for skill to move through exit	*/
    int failsavetype{};        /* Saving Throw type on skill fail	*/
    int dcfailsave{};        /* DC to save against on fail		*/
    room_vnum failroom{NOWHERE};        /* Room # to put char in when fail > 5  */
    room_vnum totalfailroom{NOWHERE};        /* Room # if char fails save < 5	*/

    struct room_data* getDestination();
};


/* ================== Memory Structure for room ======================= */
struct room_data : public unit_data, std::enable_shared_from_this<room_data> {
    room_data();
    ~room_data() override;
    zone_vnum zone{NOTHING};
    std::vector<trig_vnum> getProtoScript() const override;

    SectorType sector_type{SectorType::inside};            /* sector type (move/hide)            */
    std::array<room_direction_data*, NUM_OF_DIRS> dir_option{}; /* Directions */
    FlagHandler<RoomFlag> room_flags{};   /* DEATH,DARK ... etc */
    FlagHandler<WhereFlag> where_flags{};
    SpecialFunc func{};

    std::vector<trig_vnum> proto_script; /* list of default triggers  */

    std::list<std::weak_ptr<char_data>> characters;    /* List of characters in room          */

    int deathtrap_timer{};                   /* For timed Dt's                     */
    int damage{};                     /* How damaged the room is            */
    int ground_effect{};            /* Effect of ground destruction       */
    
    void activate();
    void deactivate();

    int getDamage();
    int setDamage(int amount);
    int modDamage(int amount);

    bool isActive() override;

    std::shared_ptr<room_data> shared();

    std::optional<room_vnum> getLaunchDestination();

    std::vector<std::weak_ptr<char_data>> getPeople();

    std::optional<std::string> dgCallMember(const std::string& member, const std::string& arg);

    double getEnvironment(int type);
    double setEnvironment(int type, double value);
    double modEnvironment(int type, double value);
    void clearEnvironment(int type);
    std::unordered_map<int, double> environment;
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
    std::list<std::weak_ptr<char_data>> memory{};        /* List of attackers to remember	       */
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

/* Structure used for chars following other chars */
struct follow_type {
    struct char_data *follower;
    struct follow_type *next;
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
    ~trans_data();

    char *description{nullptr};
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
    std::function<bool(struct char_data *ch)> effect;
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
    bool playTopCard(char_data* ch);
    card findCard(std::string);
    void addCardToDeck(std::string, int num = 1);
    void removeCard(std::string);
    void addCardToDeck(card, int num = 1);
    void removeCard(card);
    void initDeck(char_data* ch);
};

struct craftTask {
    struct obj_data *pObject = nullptr;
    int improvementRounds = 0;
};

struct npc_proto_data : public proto_data {
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
struct char_data : public thing_data, std::enable_shared_from_this<char_data> {
    char_data();
    ~char_data() override;
    // this constructor below is to be used only for the mob_proto map.

    char_data& operator=(npc_proto_data& proto);

    std::vector<trig_vnum> getProtoScript() const override;
    void activate();
    void deactivate();

    npc_proto_data* getProto() const;

    void login();

    bool active{false};
    bool isActive() override;

    void ageBy(double addedTime);
    void setAge(double newAge);

    void onAttack(atk::Attack& outgoing);
    void onAttacked(atk::Attack& incoming);

    std::optional<std::string> dgCallMember(const std::string& member, const std::string& arg);

    struct obj_data* findObject(const std::function<bool(struct obj_data*)> &func, bool working = true) override;
    std::unordered_set<struct obj_data*> gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working = true) override;

    std::shared_ptr<char_data> shared();

    char *title{};
    Race race{Race::spirit};
    std::optional<SubRace> subrace{};
    Sensei sensei{Sensei::commoner};
    Sex sex{Sex::male};

    // Base stats for this unit.
    std::unordered_map<Appearance, std::string> appearances{};
    std::string getAppearance(Appearance type, bool withTransform = true);
    const char* getAppearanceStr(Appearance type);

    std::list<std::pair<int, std::string>> wait_input_queue;
    Task task{Task::nothing};
    void setTask(Task t);
    struct craftTask craftingTask;
    struct deck craftingDeck;

    /* PC / NPC's weight                    */
    bool canCarryWeight(struct obj_data *obj);
    bool canCarryWeight(struct char_data *obj);
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

    /* Equipment array			*/
    struct obj_data *equipment[NUM_WEARS]{};

    std::map<int, struct obj_data*> getEquipment();
    struct obj_data* getEquipSlot(int slot);

    struct descriptor_data *desc{};    /* nullptr for mobiles			*/

    struct script_memory *memory{};    /* for mob memory triggers		*/

    /* For fighting list			*/
    struct char_data *next_affect{};/* For affect wearoff			*/
    struct char_data *next_affectv{};
    /* For round based affect wearoff	*/

    struct follow_type *followers{};/* List of chars followers		*/
    std::weak_ptr<obj_data> sits{};      /* What am I sitting on? */

    struct char_data *fighting{};    /* Opponent				*/
    struct char_data *master{};    /* Who is char following?		*/
    
    struct char_data *blocks{};    /* Who am I blocking?    */
    struct char_data *blocked{};   /* Who is blocking me?    */
    struct char_data *absorbing{}; /* Who am I absorbing */
    struct char_data *absorbby{};  /* Who is absorbing me */
    struct char_data *carrying{};
    struct char_data *carried_by{};
    
    struct char_data *drag{};
    struct char_data *dragged{};
    struct char_data *mindlink{};
    struct char_data *grappling{};
    struct char_data *grappled{};
    struct char_data *defender{};
    struct char_data *defending{};
    struct char_data *poisonby{};
    std::list<std::weak_ptr<char_data>> poisoned;
    struct char_data *original{};

    std::list<std::weak_ptr<char_data>> clones{};
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
    char *clan{};
    int crank{}; // clan rank
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

    double getAffectModifier(uint64_t location, uint64_t specific) override;

    // C++ reworking
    std::string juggleRaceName(bool capitalized);

    void restore(bool announce);

    void ghostify();

    void restore_by(char_data *ch);

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

    void teleport_to(IDXTYPE rnum);

    bool in_room_range(IDXTYPE low_rnum, IDXTYPE high_rnum);

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
    struct char_data *character{};    /* linked to char			*/
    struct char_data *original{};    /* original char if switched		*/
    struct descriptor_data *snooping{}; /* Who is this char snooping	*/
    struct descriptor_data *snoop_by{}; /* And who is snooping this char	*/
    struct descriptor_data *next{}; /* link to next descriptor		*/
    struct oasis_olc_data *olc{};   /* OLC info                            */
    struct account_data *account{}; /* Account info                        */
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
    struct obj_data *obj_point{};
    char *title{};
    double timeoutCounter{0};
    void handle_input();
    void start();
    void handleLostLastConnection(bool graceful);
    void sendText(const std::string &txt);
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
    struct trig_data *proto;     /* for triggers... the trigger     */
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





template <typename T>
class SubscriptionManager {

public:
    // Subscribe an entity to a particular service
    void subscribe(const std::string& service, const std::shared_ptr<T>& thing) {
        subscriptions[service].push_front(thing);
        thing->subscriptions.insert(service);
    }

    void subscribe(const std::string& service, T* thing) {
        subscribe(service, thing->shared());
    }

    T* first(const std::string& service) {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            for (const auto& weak : it->second) {
                if (auto shared = weak.lock()) {
                    return shared.get();
                }
            }
        }
        return nullptr;
    }

    size_t count(const std::string& service) const {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            return std::count_if(it->second.begin(), it->second.end(), [](const std::weak_ptr<T>& weak) {
                return !weak.expired();
            });
        }
        return 0;
    }

    // Unsubscribe an entity from a particular service
    void unsubscribe(const std::string& service, const std::shared_ptr<T>& thing) {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            it->second.remove_if([thing](const auto& weak) {
                return weak.expired() || weak.lock() == thing;
            });
            if (it->second.empty()) {
                subscriptions.erase(it);
            }
        }
        thing->subscriptions.erase(service);
    }

    void unsubscribe(const std::string& service, T* thing) {
        unsubscribe(service, thing->shared());
    }

    // Get all entities subscribed to a particular service
    std::vector<std::weak_ptr<T>> all(const std::string& service) const {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            std::vector<std::weak_ptr<T>> out;
            out.reserve(it->second.size());
            std::copy_if(it->second.begin(), it->second.end(), std::back_inserter(out), [](const std::weak_ptr<T>& weak) {
                return !weak.expired();
            });
            out.shrink_to_fit();
            return out;
        }
        return {};
    }

    // Check if an entity is subscribed to a particular service
    bool isSubscribed(const std::string& service, const std::shared_ptr<T>& thing) const {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            auto weak = std::weak_ptr<T>(thing);
            return it->second.find(weak) != it->second.end();
        }
        return false;
    }

    bool isSubscribed(const std::string& service, T* thing) const {
        return isSubscribed(service, thing->shared());
    }

    void unsubscribeFromAll(const std::shared_ptr<T>& thing) {
        for (auto it = subscriptions.begin(); it != subscriptions.end(); ) {
            it->second.remove_if([thing](const std::weak_ptr<T>& weak) {
                return weak.expired() || weak.lock() == thing;
            });
            if (it->second.empty()) {
                it = subscriptions.erase(it); // Erase and get the next iterator
            } else {
                ++it;
            }
        }
        thing->subscriptions.clear();
    }

    void unsubscribeFromAll(T* thing) {
        unsubscribeFromAll(thing->shared());
    }

private:
    std::unordered_map<std::string, std::list<std::weak_ptr<T>>> subscriptions;
};

struct shop_buy_data {
    int type{};
    std::string keywords{};
};

struct org_data : public picky_data {
    int vnum{NOTHING};        /* Virtual number of this shop		*/

    mob_vnum keeper{NOBODY};                   /* GM's vnum */
    std::vector<std::weak_ptr<char_data>> getKeepers();
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
    char *index;      /*Future Use */
    char *keywords;   /*Keyword Place holder and sorter */
    char *entry;      /*Entries for help files with Keywords at very top*/
    int duplicate;    /*Duplicate entries for multple keywords*/
    int min_level;    /*Min Level to read help entry*/
};

/* structure for the reset commands */
struct reset_com {
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
    FlagHandler<ZoneFlag> zone_flags{};          /* Flags for the zone.                */

    void remove_room_commands(room_vnum rv);

    /*
     * Reset mode:
     *   0: Don't reset, and don't update age.
     *   1: Reset if no PC's are located in zone.
     *   2: Just reset.
     */
    std::unordered_set<room_vnum> rooms;
    std::unordered_set<mob_vnum> mobiles;
    std::unordered_set<obj_vnum> objects;
    std::unordered_set<shop_vnum> shops;
    std::unordered_set<trig_vnum> triggers;
    std::unordered_set<guild_vnum> guilds;

    std::list<std::weak_ptr<char_data>> npcsInZone;
    std::list<std::weak_ptr<char_data>> playersInZone;
    std::list<std::weak_ptr<obj_data>> objectsInZone;
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
