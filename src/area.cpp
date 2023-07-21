#include "structs.h"
#include "db.h"
#include <fstream>
#include "utils.h"

vnum getNextAreaVnum() {
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

void load_areas() {
        // We must search the directory <cwd>/world/areas for all <number>.json files and load them as areas.

        // Let's start by getting a path into that directory.
        // We'll use the C++ filesystem library.
        std::filesystem::path areas_path = std::filesystem::current_path() / "areas";

        // create it if it doesn't exist.
        if (!std::filesystem::exists(areas_path)) {
            std::filesystem::create_directories(areas_path);
        }

        // Now we need to search for all files with a .json extension, check for a numeric filename, and
        // populate the areas global map.
        for (const auto &entry : std::filesystem::directory_iterator(areas_path)) {
            // We only want to load files with a .json extension.
            if (entry.path().extension() == ".json") {
                // We only want to load files with a numeric filename.
                std::string stem = entry.path().stem().string();

                if (std::all_of(stem.begin(), stem.end(), ::isdigit)) {
                    // We have a numeric filename.  Let's load it.
                    std::ifstream file(entry.path());
                    vnum vn = std::stoi(stem);
                    nlohmann::json j;
                    if (file.is_open()) {
                        file >> j;
                        file.close();
                        auto ait = areas.emplace(vn, j);
                        auto a = ait.first->second;
                        for(auto r : a.rooms) {
                            auto &room = world[r];
                            room.area = vn;
                        }

                    } else {
                        log("Unable to open area file %s.", entry.path().c_str());
                    }
                }
            }
        }

        for(auto &[vn, a] : areas) {
            if(a.parent) {
                auto &parent = areas[a.parent.value()];
                parent.children.insert(vn);
            }
        }
};

void save_areas() {
    // Let's start by getting a path into the areas directory.
    // We'll use the C++ filesystem library.
    std::filesystem::path areas_path = std::filesystem::current_path() / "areas";

    // create it if it doesn't exist.
    if (!std::filesystem::exists(areas_path)) {
        std::filesystem::create_directories(areas_path);
    }

    for(auto &[vn, a] : areas) {
        // for each vn we want to create a <vn>.json file containing a.serialize()
        auto filePath = areas_path / (std::to_string(vn) + ".json");
        std::ofstream file(filePath);
        if (file.is_open()) {
            file << a.serialize().dump(4);
            file.close();
        } else {
            log("Unable to open area file %s.", filePath.c_str());
        }
    }
}