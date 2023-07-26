#include "structs.h"
#include "db.h"

vnum area_data::getNextID() {
    vnum vn = 0;
    while(areas.contains(vn)) vn++;
    return vn;
}

area_data::area_data(const nlohmann::json& j) {
    if(j.contains("vn")) vn = j["vn"];
    if(j.contains("name")) name = j["name"];
    if(j.contains("rooms")) {
        // "rooms" is an array of numbers. We need to iterate them and fill the rooms set.
        for(auto& room : j["rooms"]) {
            rooms.insert(room.get<vnum>());
        }
    }
    if(j.contains("gravity")) gravity = j["gravity"];
    if(j.contains("parent")) parent = j["parent"];
    if(j.contains("type")) type = j["type"];
    if(j.contains("objectVnum")) objectVnum = j["objectVnum"];
    if(j.contains("orbit")) orbit = j["orbit"];
    if(j.contains("ether")) ether = j["ether"];
    if(j.contains("moon")) ether = j["moon"];
}

nlohmann::json area_data::serialize() {
    nlohmann::json j;
    j["vn"] = vn;
    if(!name.empty()) j["name"] = name;
    for(auto r : rooms) j["rooms"].push_back(r);
    if(gravity) j["gravity"] = gravity.value();
    if(parent) j["parent"] = parent.value();
    j["type"] = type;
    if(objectVnum) j["objectVnum"] = objectVnum.value();
    if(orbit) j["orbit"] = orbit.value();
    if(ether) j["ether"] = ether;
    if(moon) j["moon"] = moon;

    return j;
}
