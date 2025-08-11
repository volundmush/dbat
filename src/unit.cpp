#include "dbat/structs.h"
#include "dbat/dg_scripts.h"
#include "dbat/utils.h"
#include "dbat/filter.h"
#include "dbat/genolc.h"

std::vector<std::weak_ptr<Object>> Entity::getObjects() const {
    std::vector<std::weak_ptr<Object>> out;
    for(const auto &uw : contents) {
        if(auto u = uw.lock()) {
            if(auto o = std::dynamic_pointer_cast<Object>(u)) {
                out.push_back(o);
            }
        }
    }
    out.shrink_to_fit();
    return out;
}

std::string Entity::getUID(bool active) {
    return fmt::format("#{}:{}{}", id, generation, active ? "!" : "");
}

void Entity::activateScripts() {
    for(auto &[vn, t] : scripts) {
        t->activate();
    }
}

void Entity::deactivateScripts() {
    for(auto &[vn, t] : scripts) {
        t->deactivate();
    }
}

void Entity::activateContents() {
    auto con = getObjects();
    for(auto obj : filter_raw(con)) {
        obj->activate();
    }
}

void Entity::deactivateContents() {
    auto con = getObjects();
    for(auto obj : filter_raw(con)) {
        obj->deactivate();
    }
}

std::string ThingPrototype::scriptString() const {
    std::vector<std::string> vnums;
    for(auto p : proto_script) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}

double Entity::getInventoryWeight() {
    double weight = 0;
    for(auto obj : filter_raw(getObjects())) {
        weight += obj->getEffectiveStat("weight_total");
    }
    return weight;
}

int64_t Entity::getInventoryCount() {
    return getObjects().size();
}

Object* Entity::findObject(const std::function<bool(Object*)> &func, bool working) {
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

Object* Entity::findObjectVnum(obj_vnum objVnum, bool working) {
    return findObject([objVnum](auto o) {return o->getVnum() == objVnum;}, working);
}

std::unordered_set<Object*> Entity::gatherObjects(const std::function<bool(Object*)> &func, bool working) {
    std::unordered_set<Object*> out;
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

double Entity::getAffectModifier(uint64_t location, uint64_t specific) {
    return 0.0;
}

Entity::~Entity() {
    extract_script(this, type);
}

std::vector<trig_vnum> Entity::getScriptOrder() {
    if(running_scripts.has_value()) {
        return *running_scripts;
    }
    return getProtoScript();
}

std::vector<std::weak_ptr<DgScript>> Entity::getScripts() {
    std::vector<std::weak_ptr<DgScript>> out;
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

const char* Entity::getName() const {
    if(auto find = strings.find("name"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const char* Entity::getRoomDescription() const {
    if(auto find = strings.find("room_description"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const char* Entity::getLookDescription() const {
    if(auto find = strings.find("look_description"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const char* Entity::getShortDescription() const {
    if(auto find = strings.find("short_description"); find != strings.end()) {
        return find->second.c_str();
    }
    return "undefined";
}

const std::vector<ExtraDescription>& Entity::getExtraDescription() const {
    return extra_descriptions;
}

std::string Entity::scriptString() const {
    std::vector<std::string> vnums;
    auto ps = getProtoScript();
    for(auto p : ps) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}

vnum Entity::getVnum() const {
    return vn;
}

std::string_view Entity::getString(const std::string &key) const {
    if(auto it = strings.find(key); it != strings.end()) {
        return it->second;
    }
    return {};
}

void Entity::sendText(const std::string& txt) {
    // this does nothing on unit_data...
}