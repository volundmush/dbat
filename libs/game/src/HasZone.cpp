#include "dbat/game/HasZone.hpp"
#include <nlohmann/json.hpp>

void to_json(nlohmann::json& j, const HasZone& p) {
    if(p.zone) j["zone"] = p.zone->number;
}

void from_json(const nlohmann::json& j, HasZone& p) {
    if (j.contains(+"zone"))
        p.zone.reset(zone_table.at(j["zone"].get<zone_vnum>()).get());
}
