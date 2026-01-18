#pragma once
#include <bitset>

#include "logging/Log.hpp"
#include "CharacterShared.h"
#include "Account.h"
#include "HasMisc.h"
#include "HasEquipment.h"
#include "HasInventory.h"
#include "HasLocation.h"
#include "HasMudStrings.h"
#include "HasDgScripts.h"
#include "HasInteractive.h"

#include "WeakBag.h"
#include "Handle.h"

#include "const/AdminFlag.h"
#include "const/PlayerFlag.h"
#include "const/PrefFlag.h"
#include "const/Form.h"

#include "const/Max.h"
#include "const/CharacterProperties.h"
#include "const/Skill.h"
#include "StatHandler.h"

struct PlayerData {
    int id{NOTHING};
    std::string name;
    struct Account *account{};
    struct Character *character{};
    std::vector<struct alias_data> aliases;    /* Character's aliases                  */
    std::unordered_set<int64_t> sense_player;
    std::unordered_set<mob_vnum> sense_memory;
    std::map<int64_t, std::string> dub_names;
    std::unordered_set<zone_vnum> known_zones;
    char *color_choices[NUM_COLOR]{}; /* Choices for custom colors		*/
    struct txt_block *comm_hist[NUM_HIST]{}; /* Player's communications history     */
};

struct Character;

extern StatHandler<Character> charStats;

namespace atk {
    struct Attack;
}

struct Character : public CharacterBase, public HasID, public HasLocation, public HasInteractive, public HasEquipment, public HasInventory, public HasDgScripts, public HasSubscriptions, std::enable_shared_from_this<Character> {
    static int64_t lastID;
    static std::unordered_map<int64_t, std::shared_ptr<Character>> registry;

    Character();
    ~Character();
    void setID(int64_t newID);
    // this constructor below is to be used only for the mob_proto map.

    Character& operator=(CharacterPrototype& proto);

    std::vector<std::string> getInteractivityKeywords(struct Character* viewer) override;
    std::string getDisplayName(struct Character* viewer, bool capitalizeArticle = false) override;
    bool isVisibleTo(Character* viewer) override;

    Sex getApparentSex(Character* viewer);
    Race getApparentRace(Character* viewer);

    vnum getDgVnum() const override;
    UnitType getDgUnitType() const override;

    bool isPC{false};

    std::string title;

    // only used by NPCs.
    std::list<std::weak_ptr<Character>> agg_memory{};

    const char* getDgName() const override;
    std::vector<trig_vnum> getProtoScript() const override;
    void activate();
    void deactivate();
    DgReturn dgCallMember(DgScript* trig, std::string_view field, std::string_view subfield) override;

    std::string getUID(bool active) const override;

    CharacterPrototype* getProto() const;

    bool isActiveInLocation() const override;
    std::shared_ptr<HasLocation> getSharedHasLocation() override;
    void displayLocationInfo(Character* viewer) override;
    std::string getLocationDisplayCategory(Character* viewer) const override;

    void onMoveToLocation(const Location& loc) override;
    void onLeaveLocation(const Location& loc) override;
    void onLocationChanged(const Location& oldloc, const Location& newloc) override;

    void sendText(std::string_view txt);

    template<typename... Args>
    void sendFmt(fmt::string_view format, Args&&... args) {
        try {
            std::string formatted_string = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
            if(formatted_string.empty()) return;
            sendText(formatted_string);
        }
        catch(const fmt::format_error& e) {
            LERROR("SYSERR: Format error in Character::sendFmt: %s", e.what());
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
            LERROR("SYSERR: Format error in Character::send_to: %s", e.what());
            LERROR("Template was: %s", format.data());
            return 0;
        }
    }

    void login();

    void startListening();
    void stopListening();

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
    bool canCarryWeight(double val);

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
    
    ObjectHandle sits{};      /* What am I sitting on? */
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

    FlagHandler<PlayerFlag> player_flags{}; /* act flag for NPC's; player flag for PC's */
    FlagHandler<AdminFlag> admin_flags{};    /* Bitvector for admin privs		*/
    FlagHandler<PrefFlag> pref_flags{};    /* preference flags for PC's.		*/

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

    bool canSeeInDark() const;
    bool canSee(HasInteractive *target, bool skipLightCheck = false);
    
    std::string displayNameFor(Character* viewer);

    std::string_view otherSensePower(Character* other);
    std::string_view otherSenseAlign(Character* other);

};

inline std::string format_as_diagnostic(const Character& ch) {
    std::vector<std::string> parts;
    parts.emplace_back(format_as(static_cast<const HasID&>(ch)));
    parts.emplace_back(format_as(static_cast<const CharacterBase&>(ch)));
    parts.emplace_back(format_as(static_cast<const HasLocation&>(ch)));
    parts.emplace_back(format_as(static_cast<const HasDgScripts&>(ch)));
    parts.emplace_back(format_as(static_cast<const HasSubscriptions&>(ch)));
    return fmt::format("({}) Character:\r\n{}", ch.isPC ? "PC" : "NPC", fmt::join(parts, "\r\n"));
}

extern SubscriptionManager<Character> characterSubscriptions;


extern std::vector<std::weak_ptr<Character>> getAllCharacters();

extern Character *affect_list;
extern Character *affectv_list;

