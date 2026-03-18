#pragma once
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <nlohmann/json_fwd.hpp>

#include "const/StructureFlag.hpp"
#include "Flags.hpp"
#include "AbstractGridArea.hpp"
#include "HasLocation.hpp"
#include "HasInteractive.hpp"

struct Structure : public AbstractGridArea, public HasID, public HasInteractive, public HasLocation, public std::enable_shared_from_this<Structure> {
    static std::unordered_map<int64_t, std::shared_ptr<Structure>> registry;
    std::string getLocID() const override;
    vnum getLocVnum() const override;
    Zone* getLocZone() const override;
    std::string getLocationDisplayCategory(Character* viewer) const override;
    std::shared_ptr<AbstractLocation> getSharedAbstractLocation() override;

    std::vector<std::string> getInteractivityKeywords(struct Character* viewer) override;
    std::string getDisplayName(struct Character* viewer, bool capitalizeArticle = false) override;
    bool isVisibleTo(Character* viewer) override;

    bool isActiveInLocation() const override;
    void displayLocationInfo(Character* viewer) override;
    std::shared_ptr<HasLocation> getSharedHasLocation() override;

    std::expected<Location, std::string> getDockingPortLocation(Structure *vehicle, Character *pilot);

    FlagHandler<StructureFlag> structure_flags;

    std::unordered_set<int64_t> owners, users, banned;

    bool isAuthorized(Character *ch, bool ownerOnly = false) const;

};

void to_json(nlohmann::json& j, const Structure& p);
void from_json(const nlohmann::json& j, Structure& p);
