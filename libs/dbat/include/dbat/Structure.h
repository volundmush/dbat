#pragma once
#include <unordered_map>
#include <memory>

#include "AbstractGridArea.h"
#include "HasLocation.h"

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

extern std::unordered_map<int64_t, std::shared_ptr<Structure>> structures;