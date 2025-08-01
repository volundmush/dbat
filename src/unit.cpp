#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"
#include "dbat/filter.h"
#include "dbat/genolc.h"

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
    for(auto &[vn, t] : scripts) {
        t->activate();
    }
}

void unit_data::deactivateScripts() {
    for(auto &[vn, t] : scripts) {
        t->deactivate();
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

std::string proto_data::scriptString() const {
    std::vector<std::string> vnums;
    for(auto p : proto_script) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}

double unit_data::getInventoryWeight() {
    double weight = 0;
    for(auto obj : filter_raw(getObjects())) {
        weight += obj->getEffectiveStat("weight_total");
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
    return findObject([objVnum](auto o) {return o->getVnum() == objVnum;}, working);
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

double unit_data::getAffectModifier(uint64_t location, uint64_t specific) {
    return 0.0;
}

unit_data::~unit_data() {
    extract_script(this, type);
}

std::vector<trig_vnum> unit_data::getScriptOrder() {
    if(running_scripts.has_value()) {
        return *running_scripts;
    }
    return getProtoScript();
}

std::vector<std::shared_ptr<trig_data>> unit_data::getScripts() {
    std::vector<std::shared_ptr<trig_data>> out;
    auto proto = getScriptOrder();
    out.reserve(scripts.size() + proto.size());
    for(const auto &v : proto) {
        if(auto it = scripts.find(v); it != scripts.end()) {
            out.push_back(it->second);
        } else {
            basic_mud_log("Warning: script vnum %d not found in scripts map for %s", v, getName());
        }
    }
    return out;
}

const char* unit_data::getName() const {
    if(auto find = strings.find("name"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const char* unit_data::getRoomDescription() const {
    if(auto find = strings.find("room_description"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const char* unit_data::getLookDescription() const {
    if(auto find = strings.find("look_description"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const char* unit_data::getShortDescription() const {
    if(auto find = strings.find("short_description"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const std::vector<ExtraDescription>& unit_data::getExtraDescription() const {
    return extra_descriptions;
}

std::string unit_data::scriptString() const {
    std::vector<std::string> vnums;
    auto ps = getProtoScript();
    for(auto p : ps) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}

vnum unit_data::getVnum() const {
    return vn;
}

std::string_view unit_data::getString(const std::string &key) const {
    if(auto it = strings.find(key); it != strings.end()) {
        return it->second;
    }
    return {};
}