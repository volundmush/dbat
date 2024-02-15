#include "dbat/structs.h"
#include "dbat/db.h"
#include "dbat/comm.h"
#include "dbat/utils.h"


area_data::area_data(const nlohmann::json& j) {
    if(j.contains("gravity")) gravity = j["gravity"];
    if(j.contains("type")) type = j["type"];
}

nlohmann::json area_data::serialize() {
    nlohmann::json j;
    if(gravity) j["gravity"] = gravity.value();

    j["type"] = type;


    return j;
}

static std::string areaTypeName(AreaType a) {
    switch(a) {
        case AreaType::Dimension:
            return "Dimen";
        case AreaType::CelestialBody:
            return "ClBod";
        case AreaType::Region:
            return "Regn";
        case AreaType::Structure:
            return "Struc";
        case AreaType::Vehicle:
            return "Vehic";

    }
}
