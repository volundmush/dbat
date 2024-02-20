#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"


std::string NonPlayerCharacter::getUnitClass() {
    return "NonPlayerCharacter";
}

void NonPlayerCharacter::deserialize(const nlohmann::json& j) {
    BaseCharacter::deserialize(j);
}

nlohmann::json NonPlayerCharacter::serialize() {
    return BaseCharacter::serialize();
}

NonPlayerCharacter::NonPlayerCharacter(const nlohmann::json& j) {
    deserialize(j);
}

bool NonPlayerCharacter::isPC() {
    return false;
}

bool NonPlayerCharacter::isNPC() {
    return true;
}

std::vector<std::string> NonPlayerCharacter::getKeywords(GameEntity* looker) {
    auto out = baseKeywordsFor(looker);

    auto sname = split(getName(), ' ');
    out.insert(out.end(), sname.begin(), sname.end());

    return out;
}

std::string NonPlayerCharacter::getDisplayName(GameEntity* looker) {
    return getShortDesc();
}

void NonPlayerCharacter::assignTriggers() {
    // Nothing to do without a prototype...

    // remove all duplicates from i->proto_script but do not change its order otherwise.
    std::set<trig_vnum> existVnums;
    std::set<trig_vnum> valid;
    for(auto t : proto_script) valid.insert(t);
    
    for(auto t : script->dgScripts) existVnums.insert(t->parent->vn);
    bool added = false;
    bool removed = false;

    // remove any dgScript instances in i->script->dgScripts that aren't in i->proto_script
    std::list<std::shared_ptr<trig_data>> validScripts;
    for(auto t : script->dgScripts) {
        if(valid.contains(t->parent->vn)) {
            validScripts.push_back(t);
        }
        else {
            removed = true;
        }
    }
    if(removed) script->dgScripts = validScripts;

    for(auto p : proto_script) {
        // only add if they don't already have one...
        if(!existVnums.contains(p)) {
            script->addTrigger(read_trigger(p), -1);
            added = true;
            existVnums.insert(p);
        }
    }

    if(added || removed) {
        // we need to sort i->script->dgScripts by the order of i->proto_script
        std::list<std::shared_ptr<trig_data>> sorted;
        for(auto p : proto_script) {
            for(auto t : script->dgScripts) {
                if(t->parent->vn == p) {
                    sorted.push_back(t);
                    break;
                }
            }
        }
        script->dgScripts = sorted;
    }
}

nlohmann::json mob_special_data::serialize() {
    nlohmann::json j;
    if(attack_type) j["attack_type"] = attack_type;
    if(default_pos != POS_STANDING) j["default_pos"] = default_pos;
    if(damnodice) j["damnodice"] = damnodice;
    if(damsizedice) j["damsizedice"] = damsizedice;

    return j;
}


void mob_special_data::deserialize(const nlohmann::json &j) {
    if(j.contains("attack_type")) attack_type = j["attack_type"];
    if(j.contains("default_pos")) default_pos = j["default_pos"];
    if(j.contains("damnodice")) damnodice = j["damnodice"];
    if(j.contains("damsizedice")) damsizedice = j["damsizedice"];
}

mob_special_data::mob_special_data(const nlohmann::json &j) : mob_special_data() {
    deserialize(j);
}

std::string NonPlayerCharacter::renderRoomListName(GameEntity* viewer) {
    return getDisplayName(viewer);
}