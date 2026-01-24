#pragma once
#include "AbstractGridArea.hpp"
#include "HasZone.hpp"

struct Area : public AbstractGridArea, public HasVnum, public HasZone, std::enable_shared_from_this<Area> {
    std::string getLocID() const override;
    vnum getLocVnum() const override;
    Zone* getLocZone() const override;
    std::shared_ptr<AbstractLocation> getSharedAbstractLocation() override;

};

extern std::unordered_map<int64_t, std::shared_ptr<Area>> areas;