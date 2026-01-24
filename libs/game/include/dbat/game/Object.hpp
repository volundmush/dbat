#pragma once
#include <memory>
#include <unordered_map>

#include "ObjectShared.hpp"
#include "HasLocation.hpp"
#include "HasInventory.hpp"
#include "HasExtraDescriptions.hpp"
#include "HasMisc.hpp"
#include "HasDgScripts.hpp"
#include "HasMudStrings.hpp"
#include "HasPicky.hpp"
#include "affect.hpp"
#include "HasInteractive.hpp"

#include "const/ItemFlag.hpp"

#include "StatHandler.hpp"
#include "SubscriptionManager.hpp"

struct ObjectPrototype;
struct Object;

extern StatHandler<Object> itemStats;
extern SubscriptionManager<Object> objectSubscriptions;


struct Object : public ObjectBase, public HasID, public HasInteractive, public HasLocation, public HasInventory, public HasDgScripts, public HasSubscriptions, public std::enable_shared_from_this<Object> {
    static int64_t lastID;
    static std::unordered_map<int64_t, std::shared_ptr<Object>> registry;
    Object();
    ~Object();
    Object& operator=(const ObjectPrototype& proto);

    std::vector<std::string> getInteractivityKeywords(struct Character* viewer) override;
    std::string getDisplayName(struct Character* viewer, bool capitalizeArticle = false) override;
    bool isVisibleTo(Character* viewer) override;

    void setID(int64_t newID);
    
    vnum getDgVnum() const override;
    UnitType getDgUnitType() const override;
    const char* getDgName() const override;
    std::vector<trig_vnum> getProtoScript() const override;
    DgReturn dgCallMember(DgScript* trig, std::string_view field, std::string_view subfield) override;
    
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

