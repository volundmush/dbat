#pragma once
#include <unordered_map>
#include <memory>

#include "AbstractGridArea.hpp"
#include "HasLocation.hpp"
#include "HasInteractive.hpp"

struct Structure : public AbstractGridArea, public HasID, public HasInteractive, public HasLocation, std::enable_shared_from_this<Structure> {
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

};

extern std::unordered_map<int64_t, std::shared_ptr<Structure>> structures;