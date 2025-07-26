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
    for(auto t = trig_list; t; t = t->next) {
        t->activate();
    }
}

void unit_data::deactivateScripts() {
    for(auto t = trig_list; t; t = t->next) {
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
    if(name) free(name);
    if(room_description) free(room_description);
    if(look_description) free(look_description);
    if(short_description) free(short_description);
    if(ex_description) free_ex_descriptions(ex_description);
    extract_script(this, 0);
}