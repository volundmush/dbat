#include "structs.h"
#include "dg_scripts.h"

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

    for(auto s = proto_script; s; s = s->next) {
        if(s->vnum != NOTHING) j["proto_script"].push_back(s->vnum);
    }

    if(id) j["id"] = id;

    return j;
}

nlohmann::json unit_data::serializeContents() {
    nlohmann::json j;

    for(auto c = contents; c; c = c->next_content) {
        j.push_back(c->serializeUnit());
    }

    return j;
}

nlohmann::json unit_data::serializeScript() {
    nlohmann::json j;

    if(!script) return j;

    auto s = script;
    if(s->global_vars) {
        for(auto v = s->global_vars; v; v = v->next) {
            j["global_vars"].push_back(v->serialize());
        }
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

    if(j.contains("proto_script")) {
        auto &p = j["proto_script"];
        for(auto s = p.rbegin(); s != p.rend(); s++) {
            auto new_s = new trig_proto_list();
            new_s->vnum = *s;
            new_s->next = proto_script;
            proto_script = new_s;
        }
    }

    if(j.contains("id")) id = j["id"];

}