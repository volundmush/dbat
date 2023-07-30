#include "dbat/structs.h"
#include "dbat/dg_scripts.h"

nlohmann::json unit_data::serializeUnit() {
    nlohmann::json j;

    if(vn != NOTHING) j["vn"] = vn;

    if(name && strlen(name)) j["name"] = name;
    if(room_description && strlen(room_description)) j["room_description"] = room_description;
    if(look_description && strlen(look_description)) j["look_description"] = look_description;
    if(short_description && strlen(short_description)) j["short_description"] = short_description;

    for(auto ex = ex_description; ex; ex = ex->next) {
        if(ex->keyword && strlen(ex->keyword) && ex->description && strlen(ex->description)) {
            nlohmann::json p;
            p.push_back(ex->keyword);
            p.push_back(ex->description);
            j["ex_description"].push_back(p);
        }
    }

    if(id) j["id"] = id;

    return j;
}

nlohmann::json unit_data::serializeContents() {
    auto j = nlohmann::json::array();

    for(auto c = contents; c; c = c->next_content) {
        j.push_back(c->serializeInstance());
    }

    return j;
}


void unit_data::deserializeUnit(const nlohmann::json& j) {
    if(j.contains("vn")) vn = j["vn"];
    if(j.contains("name")) name = strdup(j["name"].get<std::string>().c_str());
    if(j.contains("room_description")) room_description = strdup(j["room_description"].get<std::string>().c_str());
    if(j.contains("look_description")) look_description = strdup(j["look_description"].get<std::string>().c_str());
    if(j.contains("short_description")) short_description = strdup(j["short_description"].get<std::string>().c_str());

    if(j.contains("ex_description")) {
        auto &e = j["ex_description"];
        for(auto ex = e.rbegin(); ex != e.rend(); ex++) {
            auto new_ex = new extra_descr_data();
            new_ex->keyword = strdup((*ex)[0].get<std::string>().c_str());
            new_ex->description = strdup((*ex)[1].get<std::string>().c_str());
            new_ex->next = ex_description;
            ex_description = new_ex;
        }
    }

    if(j.contains("id")) id = j["id"];

}

void unit_data::activateContents() {
    for(auto obj = contents; obj; obj = obj->next_content) {
        obj->activate();
    }
}

void unit_data::deactivateContents() {
    for(auto obj = contents; obj; obj = obj->next_content) {
        obj->deactivate();
    }
}

std::string unit_data::scriptString() const {
    if(!script) return "";
    std::vector<std::string> vnums;
    for(auto p : proto_script) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}