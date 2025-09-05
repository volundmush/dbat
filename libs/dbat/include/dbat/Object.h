#pragma once
#include <memory>
#include <unordered_map>

#include "ObjectShared.h"
#include "HasLocation.h"
#include "HasInventory.h"
#include "HasExtraDescriptions.h"
#include "HasMisc.h"
#include "HasDgScripts.h"
#include "HasMudStrings.h"
#include "HasPicky.h"
#include "affect.h"

#include "const/ItemFlag.h"

#include "StatHandler.h"
#include "SubscriptionManager.h"

struct ObjectPrototype;
struct Object;

extern StatHandler<Object> itemStats;
extern SubscriptionManager<Object> objectSubscriptions;


struct Object : public ObjectBase, public HasID, public HasLocation, public HasInventory, public HasExtraDescriptions, public HasDgScripts, public HasMudStrings, public HasAffectFlags, public HasSubscriptions, public HasStats, public HasPicky, public std::enable_shared_from_this<Object> {
    static std::unordered_map<int64_t, std::shared_ptr<Object>> registry;
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

    // used for ki attacks
    struct Character *user{};
    struct Character *target{};

    // for notes
    struct Object *posted_to{};
    int posttype{};
    // for icewalls
    struct Object *fellow_wall{};

    // auction data...
    char *auctname{};
    int64_t aucter{};
    int64_t curBidder{};
    time_t aucTime{};
    int bid{};
    int startbid{};
    

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

