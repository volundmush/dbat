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

#include "dbat/defs.h"

// IMPORTANT: Do not use data structures/fields that are not part of the
// C++ standard library. This allows us to keep things neat and clean for
// cross-compatability.

/**********************************************************************
* Structures                                                          *
**********************************************************************/

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
    char *keyword;                 /* Keyword in look/examine          */
    char *description;             /* What to see                      */
    struct extra_descr_data *next; /* Next in list                     */
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

struct obj_spellbook_spell {
    int spellname;    /* Which spell is written */
    int pages;        /* How many pages does it take up */
};

struct account_data {
    account_data() = default;
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
    std::unordered_set<int> sensePlayer;
    std::unordered_set<mob_vnum> senseMemory;
    std::map<int, std::string> dubNames;
    char *color_choices[NUM_COLOR]{}; /* Choices for custom colors		*/
    struct txt_block *comm_hist[NUM_HIST]{}; /* Player's communications history     */
};

struct cmdlist_element {
    char *cmd{};                /* one line of a trigger */
    struct cmdlist_element *original{};
    struct cmdlist_element *next{};
};

struct trig_var_data {
    char *name{};                /* name of variable  */
    char *value{};                /* value of variable */
    long context{};                /* 0: global context */
    struct trig_var_data *next{};
};

/* The event data for the wait command */
struct wait_event_data {
    struct trig_data *trigger{};
    void *go{};
    int type{};
};

/* structure for triggers */
struct trig_data : std::enable_shared_from_this<trig_data> {
    trig_vnum vn{NOTHING};                    /* trigger's rnum                  */
    int8_t attach_type{};            /* mob/obj/wld intentions          */
    int8_t data_type{};                /* type of game_data for trig      */
    char *name{};                    /* name of trigger                 */
    long trigger_type{};            /* type of trigger (for bitvector) */
    struct cmdlist_element *cmdlist{};    /* top of command list             */
    struct cmdlist_element *curr_state{};    /* ptr to current line of trigger  */
    int narg{};                /* numerical argument              */
    char *arglist{};            /* argument list                   */
    int depth{};                /* depth into nest ifs/whiles/etc  */
    int loops{};                /* loop iteration counter          */
    double waiting{0.0};    /* event to pause the trigger      */
    bool purged{};            /* trigger is set to be purged     */
    struct trig_var_data *var_list{};    /* list of local vars for trigger  */
    std::shared_ptr<unit_data> owner{};
    int order{0};
    int countLine(struct cmdlist_element *c) const;

    bool active{false};
    void activate();
    void deactivate();

    int64_t id{NOTHING};
    time_t generation{};

    struct trig_data *next{};
    struct trig_data *next_in_world{};    /* next in the global trigger list */
    
    std::shared_ptr<trig_data> shared();
};

struct unit_data {
    virtual ~unit_data() = default;
    vnum vn{NOTHING}; /* Where in database? Not used by all things. */
    zone_vnum zone{NOTHING};
    
    virtual int getType() const = 0; // 0 is room, 1 is object, 2 is character.

    struct unit_data *proto{};

    char *name{};
    char *room_description{};      /* When thing is listed in room */
    char *look_description{};      /* what to show when looked at */
    char *short_description{};     /* when displayed in list or action message. */

    struct extra_descr_data *ex_description{}; /* extra descriptions     */

    // for DGscripts data.
    std::vector<trig_vnum> proto_script; /* list of default triggers  */
    long trigger_types{};                /* bitvector of trigger types */
    struct trig_data *trig_list{};            /* list of triggers           */
    struct trig_var_data *global_vars{};    /* list of global variables   */
    long script_context{};                /* current context for statics */

    void activateScripts();
    void deactivateScripts();

    weight_t getInventoryWeight();
    int64_t getInventoryCount();

    std::list<std::weak_ptr<obj_data>> objects;
    std::vector<std::weak_ptr<obj_data>> getObjects();

    int id{NOTHING}; /* the unique ID of this entity */
    time_t generation{}; /* creation time for dupe check     */

    void activateContents();
    void deactivateContents();

    std::string scriptString();

    std::string getUID(bool active = false);
    virtual bool isActive() = 0;

    struct obj_data* findObjectVnum(obj_vnum objVnum, bool working = true);
    virtual struct obj_data* findObject(const std::function<bool(struct obj_data*)> &func, bool working = true);
    virtual std::unordered_set<struct obj_data*> gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working = true);

};

struct room_direction_data;

struct thing_data : public unit_data {
    room_rnum in_room{NOWHERE};        /* In what room -1 when conta/carr	*/

    struct room_data* room;
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

/* ================== Memory Structure for Objects ================== */
struct obj_data : public thing_data, std::enable_shared_from_this<obj_data> {
    int getType() const override { return 1; }

    std::string serializeLocation();

    void deserializeLocation(const std::string& txt, int16_t slot);

    void activate();

    void deactivate();

    double getAffectModifier(uint64_t location, uint64_t specific);

    bool active{false};
    bool isActive() override;

    struct room_data* getAbsoluteRoom();
    bool isWorking();
    void clearLocation();

    std::shared_ptr<obj_data> shared();

    room_vnum room_loaded{NOWHERE};    /* Room loaded in, for room_max checks	*/

    /* legacy Values of the item (see VAL_ list in defs.h)    */
    std::unordered_map<std::string, int64_t> value;

    /* arbitrary named doubles */
    std::unordered_map<std::string, double> dvalue;
    ItemType type_flag{ItemType::unknown};      /* Type of item                        */
    int level{}; /* Minimum level of object.            */
    std::unordered_set<MoralAlign> onlyAlignGoodEvil, antiAlignGoodEvil;
    std::unordered_set<SenseiID> onlyClass, antiClass;
    std::unordered_set<RaceID> onlyRace, antiRace;
    
    std::unordered_set<WearFlag> wear_flags{}; /* Where you can wear it     */
    std::unordered_set<ItemFlag> item_flags{}; /* If it hums, glows, etc.  */
    std::bitset<NUM_AFF_FLAGS> bitvector{}; /* To set chars bits          */

    void setWearFlag(int flag, bool value = true);
    bool toggleWearFlag(int flag);
    bool getWearFlag(int flag);

    void setItemFlag(int flag, bool value = true);
    bool toggleItemFlag(int flag);
    bool getItemFlag(int flag);

    void setFlag(WearFlag flag, bool value = true);
    bool toggleFlag(WearFlag flag);
    bool getFlag(WearFlag flag);

    void setFlag(ItemFlag flag, bool value = true);
    bool toggleFlag(ItemFlag flag);
    bool getFlag(ItemFlag flag);

    weight_t weight{};         /* Weight what else                     */
    weight_t getWeight();
    weight_t getTotalWeight();
    int cost{};           /* Value when sold (gp.)               */
    int cost_per_day{};   /* Cost to keep pr. real day           */
    int timer{};          /* Timer for object                    */
    
    Size size{Size::medium};           /* Size class of object                */

    std::array<affected_type, MAX_OBJ_AFFECT> affected;  /* affects */

    struct obj_data *in_obj{};       /* In what object nullptr when none    */
    struct char_data *carried_by{};  /* Carried by :nullptr in room/conta   */
    struct char_data *worn_by{};      /* Worn by? */
    int16_t worn_on{-1};          /* Worn where?		      */

    unit_data *holder{};

    struct obj_spellbook_spell *sbinfo{};  /* For spellbook info */
    std::weak_ptr<char_data> sitting{};       /* Who is sitting on me? */
    int scoutfreq{};
    time_t lload{};
    int64_t kicharge{};
    int kitype{};
    struct char_data *user{};
    struct char_data *target{};
    int distance{};
    int foob{};
    int64_t aucter{};
    int64_t curBidder{};
    time_t aucTime{};
    int bid{};
    int startbid{};
    char *auctname{};
    int posttype{};
    struct obj_data *posted_to{};
    struct obj_data *fellow_wall{};

    std::optional<double> gravity;

    bool isProvidingLight();
    double currentGravity();
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
    ~room_data() override;
    int getType() const override { return 0; }
    int sector_type{};            /* sector type (move/hide)            */
    std::array<room_direction_data*, NUM_OF_DIRS> dir_option{}; /* Directions */
    std::unordered_set<RoomFlag> room_flags{};   /* DEATH,DARK ... etc */
    SpecialFunc func{};

    std::list<std::weak_ptr<char_data>> characters;    /* List of characters in room          */

    int timed{};                   /* For timed Dt's                     */
    int dmg{};                     /* How damaged the room is            */
    int geffect{};            /* Effect of ground destruction       */
    
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

    bool toggleRoomFlag(int flag);
    bool getRoomFlag(int flag);
    void setRoomFlag(int flag, bool value);

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
    double secondsAged{}; // The player's current IC age, in seconds.
    int currentAge();
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
    std::string name;
    std::string replacement;
    int type{};
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
    double timeSpentInForm{0.0};
    int grade = 1;
    bool visible = true;
    bool limitBroken = false;
    bool unlocked = false;

    double vars[5] = {0.0, 0.0, 0.0, 0.0, 0.0};

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


/* ================== Structure for player/non-player ===================== */
struct char_data : public thing_data, std::enable_shared_from_this<char_data> {
    char_data() = default;
    // this constructor below is to be used only for the mob_proto map.
    int getType() const override { return 2; }

    void activate();
    void deactivate();

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
    RaceID race{RaceID::spirit};
    SenseiID chclass{SenseiID::commoner};

    std::list<std::pair<int, std::string>> wait_input_queue;
    Task task = Task::nothing;
    void setTask(Task t);
    struct craftTask craftingTask;
    struct deck craftingDeck;
    double waitTime{0.0};

    /* PC / NPC's weight                    */
    weight_t getWeight(bool base = false);
    weight_t getTotalWeight();
    weight_t getCurrentBurden();
    double getBurdenRatio();
    bool canCarryWeight(struct obj_data *obj);
    bool canCarryWeight(struct char_data *obj);
    bool canCarryWeight(weight_t val);

    dim_t getHeight(bool base = false);
    dim_t setHeight(dim_t val);
    dim_t modHeight(dim_t val);

    std::unordered_map<CharDim, dim_t> dims{};
    dim_t get(CharDim stat, bool base = false);
    dim_t set(CharDim stat, dim_t val);
    dim_t mod(CharDim stat, dim_t val);

    int getArmor();

    std::unordered_map<CharNum, num_t> nums{};
    num_t get(CharNum stat);
    num_t set(CharNum stat, num_t val);
    num_t mod(CharNum stat, num_t val);

    struct mob_special_data mob_specials{};

    int size{SIZE_UNDEFINED};
    int getSize();
    int setSize(int val);

    double getTimeModifier();
    double getPotential();
    void gainGrowth();
    void gainGrowth(double);

    std::unordered_map<CharMoney, money_t> moneys;
    money_t get(CharMoney type);
    money_t set(CharMoney type, money_t val);
    money_t mod(CharMoney type, money_t val);

    std::unordered_map<CharAlign, align_t> aligns;
    align_t get(CharAlign type);
    align_t set(CharAlign type, align_t val);
    align_t mod(CharAlign type, align_t val);

    std::unordered_map<CharAppearance, appearance_t> appearances;
    appearance_t get(CharAppearance type);
    appearance_t set(CharAppearance type, appearance_t val);
    appearance_t mod(CharAppearance type, appearance_t val);

    std::bitset<NUM_AFF_FLAGS> affected_by{};/* Bitvector for current affects	*/

    std::unordered_map<CharVital, vital_t> vitals;
    vital_t get(CharVital type, bool base = true);
    vital_t set(CharVital type, vital_t val);
    vital_t mod(CharVital type, vital_t val);
    double getRegen(CharVital type);

    // Instance-relevant fields below...
    room_vnum was_in_room{NOWHERE};    /* location for linkdead people		*/

    std::unordered_set<AdminFlag> admflags{};    /* Bitvector for admin privs		*/
    room_vnum hometown{NOWHERE};        /* PC Hometown / NPC spawn room         */
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
    
    int64_t master_id{};

    struct memorize_node *memorized{};
    struct innate_node *innate{};

    int8_t position{POS_STANDING};        /* Standing, fighting, sleeping, etc.	*/

    int timer{};            /* Timer for update			*/

    std::weak_ptr<obj_data> sits{};      /* What am I sitting on? */

    struct char_data *fighting;    /* Opponent				*/
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

    int8_t feats[MAX_FEATS + 1]{};    /* Feats (booleans and counters)	*/
    int combat_feats[CFEAT_MAX + 1][FT_ARRAY_MAX]{};
    /* One bitvector array per CFEAT_ type	*/
    int school_feats[SFEAT_MAX + 1]{};/* One bitvector array per CFEAT_ type	*/

    std::map<SkillID, skill_data> skill;

    std::unordered_set<PlayerFlag> playerFlags{}; /* act flag for NPC's; player flag for PC's */
    std::unordered_set<MobFlag> mobFlags{};

    bool getMobFlag(int flag);
    bool toggleMobFlag(int flag);
    void setMobFlag(int flag, bool value);

    bool getPlayerFlag(int flag);
    bool togglePlayerFlag(int flag);
    void setPlayerFlag(int flag, bool value);

    bool getAdminFlag(int flag);
    bool toggleAdminFlag(int flag);
    void setAdminFlag(int flag, bool value);

    bool getPrefFlag(int flag);
    bool togglePrefFlag(int flag);
    void setPrefFlag(int flag, bool value);

    std::bitset<NUM_WEARS> bodyparts{};  /* Bitvector for current bodyparts      */
    int16_t saving_throw[3]{};    /* Saving throw				*/
    int16_t apply_saving_throw[3]{};    /* Saving throw bonuses			*/

    int armor{0};        /* Internally stored *10		*/

    int64_t getExperience();
    int64_t setExperience(int64_t value);
    int64_t modExperience(int64_t value, bool applyBonuses = true);

    std::unordered_map<CharStat, stat_t> stats;
    stat_t get(CharStat type);
    stat_t set(CharStat type, stat_t val);
    stat_t mod(CharStat type, stat_t val);

    int accuracy{};            /* Base hit accuracy			*/
    int accuracy_mod{};        /* Any bonus or penalty to the accuracy	*/
    int damage_mod{};        /* Any bonus or penalty to the damage	*/

    FormID form{FormID::base};        /* Current form of the character		*/
    FormID technique{FormID::base};        /* Current technique form of the character		*/
    std::unordered_set<FormID> permForms;    /* Permanent forms of the character	*/
    double transBonus{0.0};   // Varies from -0.3 to 0.3
    double internalGrowth{0.0};
    double lifetimeGrowth{0.0};
    double overGrowth{0.0};
    void gazeAtMoon();

    // Data stored about different forms.
    std::unordered_map<FormID, trans_data> transforms;

    int genBonus = 0;
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
    
    int lasthit{};
    int dcount{};
    char *voice{};                  /* PC's snet voice */
    int limbs[4]{};                 /* 0 Right Arm, 1 Left Arm, 2 Right Leg, 3 Left Leg */
    time_t rewtime{};
    
    std::array<int, 6> gravAcclim;
    int grap{};
    std::unordered_set<uint8_t> genome{};                /* Bio racial bonus, Genome */
    int combo{};
    int lastattack{};
    int combhits{};
    int ping{};
    int starphase{};
    std::optional<RaceID> mimic{};
    std::bitset<MAX_BONUSES> bonuses{};

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

    int lifeperc{};
    int gooptime{};
    int blesslvl{};
    

    int mobcharge{};
    int preference{};
    int aggtimer{};

    int lifebonus{};
    int asb{};
    int regen{};
    int con_sdcooldown{};

    int8_t limb_condition[4]{};

    char *rdisplay{};

    
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
    std::unordered_set<PrefFlag> pref{};    /* preference flags for PC's.		*/

    room_vnum normalizeLoadRoom(room_vnum in);

    double getAffectModifier(int location, int specific = -1);

    std::unordered_map<CharAttribute, attribute_t> attributes;
    attribute_t get(CharAttribute attr, bool base = false);
    attribute_t set(CharAttribute attr, attribute_t val);
    attribute_t mod(CharAttribute attr, attribute_t val);

    std::unordered_map<CharTrain, attribute_train_t> trains;
    attribute_train_t get(CharTrain attr);
    attribute_train_t set(CharTrain attr, attribute_train_t val);
    attribute_train_t mod(CharTrain attr, attribute_train_t val);

    // C++ reworking
    std::string juggleRaceName(bool capitalized);

    void restore(bool announce);

    void ghostify();

    void restore_by(char_data *ch);

    void gainTail(bool announce = true);
    void loseTail();
    bool hasTail();

    void hideTransform(FormID form, bool hide);
    void addTransform(FormID form);
    bool removeTransform(FormID form);
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

    bool can_tolerate_gravity(int grav);

    int calcTier();

    int64_t calc_soft_cap();

    bool is_soft_cap(int64_t type, long double mult);

    bool is_soft_cap(int64_t type);

    int wearing_android_canister();

    int64_t calcGravCost(int64_t num);

    // Stats stuff

    std::unordered_map<CharVital, double> damages;
    double modCurVitalDam(CharVital type, double dam);
    double setCurVitalDam(CharVital type, double dam);
    double getCurVitalDam(CharVital type);

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

    int64_t getPL();

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

    int64_t getMaxPL();

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





template <typename T>
class SubscriptionManager {

public:
    // Subscribe an entity to a particular service
    void subscribe(const std::string& service, const std::shared_ptr<T>& thing) {
        subscriptions[service].push_front(thing);
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

struct shop_data {
    ~shop_data();
    void add_product(obj_vnum v);
    void remove_product(obj_vnum v);
    shop_vnum vnum{};        /* Virtual number of this shop		*/
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
    bitvector_t bitvector{};    /* Can attack? Use bank? Cast here?	*/
    mob_vnum keeper{NOBODY};    /* The mobile who owns the shop (rnum)	*/
    bitvector_t with_who[SW_ARRAY_MAX]{};/* Who does the shop trade with?	*/
    std::unordered_set<room_vnum> in_room;        /* Where is the shop?			*/
    int open1{}, open2{};        /* When does the shop open?		*/
    int close1{}, close2{};    /* When does the shop close?		*/
    int bankAccount{};        /* Store all gold over 15000 (disabled)	*/
    int lastsort{};        /* How many items are sorted in inven?	*/
    SpecialFunc func{};        /* Secondary spec_proc for shopkeeper	*/
    
    std::vector<std::weak_ptr<char_data>> getKeepers();
    bool isProducing(obj_vnum vn);
    void runPurge();
};

struct guild_data {
    room_vnum vnum{NOBODY};                /* number of the guild */
    void toggle_skill(uint16_t skill_id);
    void toggle_feat(uint16_t skill_id);
    std::unordered_set<uint16_t> skills;  /* array to keep track of which feats things we'll train */
    float charge{1.0};                  /* charge * skill level = how much we'll charge */
    std::string no_such_skill{};           /* message when we don't teach that skill */
    std::string not_enough_gold{};         /* message when the student doesn't have enough gold */
    int minlvl{0};                    /* Minumum level guildmaster will train */
    mob_vnum gm{NOBODY};                   /* GM's vnum */
    bitvector_t with_who[GW_ARRAY_MAX]{};    /* whom we dislike */
    int open{0}, close{28};               /* when we will train */
    SpecialFunc func{};                /* secondary spec_proc for the GM */
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
    bitvector_t zone_flags[ZF_ARRAY_MAX]{};          /* Flags for the zone.                */

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
