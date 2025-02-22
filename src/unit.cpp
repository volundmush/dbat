#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"

std::vector<std::weak_ptr<obj_data>> unit_data::getObjects() {
    std::vector<std::weak_ptr<obj_data>> out;
    out.reserve(objects.size());
    std::copy(objects.begin(), objects.end(), std::back_inserter(out));
    out.shrink_to_fit();
    return out;
}

std::string unit_data::getUID(bool active) {
    return fmt::format("#{}:{}{}", id, generation, active ? "!" : "");
}

void unit_data::activateScripts() {
    for(auto t = trig_list; t; t = t->next) {
        t->activate();
    }
}

void unit_data::deactivateScripts() {
    for(auto t = trig_list; t; t = t->next) {
        t->deactivate();
    }
}

nlohmann::json unit_data::serializeUnit() {
    nlohmann::json j;

    if(vn != NOTHING) j["vn"] = vn;
    if(id != NOTHING) j["id"] = id;
    if(zone != NOTHING) j["zone"] = zone;

    if(proto) {
        if(name && name != proto->name) j["name"] = name;
        if(room_description && room_description != proto->room_description) j["room_description"] = room_description;
        if(look_description && look_description != proto->look_description) j["look_description"] = look_description;
        if(short_description && short_description != proto->short_description) j["short_description"] = short_description;
    } else {
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
    }

    return j;
}


void unit_data::deserializeUnit(const nlohmann::json& j) {
    if(j.contains("vn")) vn = j["vn"];
    if(j.contains("id")) id = j["id"];
    if(j.contains("zone")) zone = j["zone"];

    if(vn != NOTHING && id != NOTHING)
        switch(getType()) {
            case 0:
                break;
            case 1:
                proto = &(obj_proto.at(vn));
                break;
            case 2:
                proto = &(mob_proto.at(vn));
                break;
        }

    if(proto) {
        name = proto->name;
        room_description = proto->room_description;
        look_description = proto->look_description;
        short_description = proto->short_description;
        ex_description = proto->ex_description;
    }

    if(j.contains("name")) {
        name = strdup(j["name"].get<std::string>().c_str());
    }
    if(j.contains("room_description")) {
        room_description = strdup(j["room_description"].get<std::string>().c_str());
    }
    if(j.contains("look_description")) {
        look_description = strdup(j["look_description"].get<std::string>().c_str());
    }
    if(j.contains("short_description")) {
        short_description = strdup(j["short_description"].get<std::string>().c_str());
    }

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

}

void unit_data::activateContents() {
    auto con = getObjects();
    for(auto obj : filter_raw(con)) {
        obj->activate();
    }
}

void unit_data::deactivateContents() {
    auto con = getObjects();
    for(auto obj : filter_raw(con)) {
        obj->deactivate();
    }
}

std::string unit_data::scriptString() {
    std::vector<std::string> vnums;
    for(auto p : proto_script) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}

double unit_data::getInventoryWeight() {
    double weight = 0;
    for(auto obj : filter_raw(getObjects())) {
        weight += obj->getTotalWeight();
    }
    return weight;
}

int64_t unit_data::getInventoryCount() {
    return getObjects().size();
}

struct obj_data* unit_data::findObject(const std::function<bool(struct obj_data*)> &func, bool working) {
    auto con = getObjects();
    for(auto obj : filter_raw(con)) {
        if(func(obj)) {
            if(working && !obj->isWorking()) continue;
            return obj;
        }
        if(auto p = obj->findObject(func, working); p) return p;
    }
    return nullptr;
}

struct obj_data* unit_data::findObjectVnum(obj_vnum objVnum, bool working) {
    return findObject([objVnum](auto o) {return o->vn == objVnum;}, working);
}

std::unordered_set<struct obj_data*> unit_data::gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working) {
    std::unordered_set<struct obj_data*> out;
    auto con = getObjects();
    for(auto obj : filter_raw(con)) {
        if(func(obj)) {
            if(working && !obj->isWorking()) continue;
            out.insert(obj);
        }
        auto contents = obj->gatherObjects(func, working);
        out.insert(contents.begin(), contents.end());
    }
    return out;
}