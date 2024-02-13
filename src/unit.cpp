#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"

base_proto::~base_proto() {
    if(name) free(name);
    if(short_description) free(short_description);
    if(description) free(description);
    while(ex_description) {
        auto next = ex_description->next;
        delete ex_description;
        ex_description = next;
    }
}

std::list<obj_data *> unit_data::getContents() {
    std::list<obj_data*> out;
    for(auto o = contents; o; o = o->next_content) out.push_back(o);
    return out;
}


nlohmann::json base_proto::serializeBase() {
    nlohmann::json j;
    if(vn != NOTHING) j["vn"] = vn;

    if(name && strlen(name)) j["name"] = name;
    if(short_description && strlen(short_description)) j["short_description"] = short_description;
    if(description && strlen(description)) j["description"] = description;
    for(auto t : proto_script) j["proto_script"].push_back(t);
    return j;
}




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

    if(uid != NOTHING) j["uid"] = uid;
    if(zone != NOTHING) j["zone"] = zone;

    if(script && (!script->dgScripts.empty() || !script->vars.empty())) {
        j["dgScripts"] = script->serialize();
    }

    return j;
}


void unit_data::deserializeUnit(const nlohmann::json& j) {
    if(j.contains("vn")) vn = j["vn"];
    if(j.contains("name")) {
        if(name) free(name);
        name = strdup(j["name"].get<std::string>().c_str());
    }
    if(j.contains("room_description")) {
        if(room_description) free(room_description);
        room_description = strdup(j["room_description"].get<std::string>().c_str());
    }
    if(j.contains("look_description")) {
        if(look_description) free(look_description);
        look_description = strdup(j["look_description"].get<std::string>().c_str());
    }
    if(j.contains("short_description")) {
        if(short_description) free(short_description);
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

    if(j.contains("uid")) uid = j["uid"];
    if(j.contains("zone")) zone = j["zone"];

    if(j.contains("dgScripts")) {
        if(!script) script = std::make_shared<script_data>(this);
        script->deserialize(j["dgScripts"]);
    }

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

std::string unit_data::scriptString() {
    return "";
}

std::string base_proto::scriptString() {
    std::vector<std::string> vnums;
    for(auto p : proto_script) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}

double unit_data::getInventoryWeight() {
    double weight = 0;
    for(auto obj = contents; obj; obj = obj->next_content) {
        weight += obj->getTotalWeight();
    }
    return weight;
}

int64_t unit_data::getInventoryCount() {
    int64_t total = 0;
    for(auto obj = contents; obj; obj = obj->next_content) {
        total++;
    }
    return total;
}

struct obj_data* unit_data::findObject(const std::function<bool(struct obj_data*)> &func, bool working) {
    for(auto obj = contents; obj; obj = obj->next_content) {
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

std::set<struct obj_data*> unit_data::gatherObjects(const std::function<bool(struct obj_data*)> &func, bool working) {
    std::set<struct obj_data*> out;
    for(auto obj = contents; obj; obj = obj->next_content) {
        if(func(obj)) {
            if(working && !obj->isWorking()) continue;
            out.insert(obj);
        }
        auto contents = obj->gatherObjects(func, working);
        out.insert(contents.begin(), contents.end());
    }
    return out;
}

std::string unit_data::getUID(bool active) {
    return fmt::format("#{}{}", uid, active ? "" : "!");
}

DgResults unit_data::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    return "";
}

bool unit_data::isActive() {
    return false;
}

void unit_data::save() {

}

std::string unit_data::getName() {
    return name ? name : "";
}

void unit_data::setName(const std::string& n) {
    if(name) free(name);
    name = strdup(n.c_str());
}

std::string unit_data::getShortDesc() {
    return short_description ? short_description : "";
}

void unit_data::setShortDesc(const std::string& n) {
    if(short_description) free(short_description);
    short_description = strdup(n.c_str());
}

std::string unit_data::getRoomDesc() {
    return room_description ? room_description : "";
}

void unit_data::setRoomDesc(const std::string& n) {
    if(room_description) free(room_description);
    room_description = strdup(n.c_str());
}

std::string unit_data::getLookDesc() {
    return look_description ? look_description : "";
}

void unit_data::setLookDesc(const std::string& n) {
    if(look_description) free(look_description);
    look_description = strdup(n.c_str());
}


void unit_data::checkMyID() {
    if(uid == -1) {
        uid = getNextUID();
        basic_mud_log("Unit Found with ID -1. Automatically fixed to ID %d", uid);
    }
}

std::string unit_data::getDisplayName(struct char_data* ch) {
    return "Nameless";
}

std::string unit_data::renderAppearance(struct char_data* ch) {
    return "You see nothing special.";
}

std::vector<std::string> unit_data::getKeywords(struct char_data* ch) {
    return {};
}

extra_descr_data::~extra_descr_data() {
    if(keyword) free(keyword);
    if(description) free(description);
}

unit_data::~unit_data() {
    if(name) free(name);
    if(short_description) free(short_description);
    if(room_description) free(room_description);
    if(look_description) free(look_description);
    while(ex_description) {
        auto next = ex_description->next;
        delete ex_description;
        ex_description = next;
    }
    if(script) script.reset();
}


void unit_data::assignTriggers() {
    // does nothing.
}