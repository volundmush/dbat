/************************************************************************
 * Generic OLC Library - Objects / genobj.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "dbat/genobj.h"

#include "dbat/class.h"
#include "dbat/genolc.h"
#include "dbat/genzon.h"
#include "dbat/utils.h"
#include "dbat/handler.h"
#include "dbat/dg_olc.h"
#include "dbat/shop.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"

int save_objects(zone_rnum zone_num) {
    return true;
}


void obj_affected_type::deserialize(const nlohmann::json &j) {
    if(j.count("location")) location = j["location"];
    if(j.count("modifier")) modifier = j["modifier"];
    if(j.count("specific")) specific = j["specific"];
}

obj_affected_type::obj_affected_type(const nlohmann::json &j) {
    deserialize(j);
}

nlohmann::json obj_affected_type::serialize() {
    nlohmann::json j;

    if(location) j["location"] = location;
    if(modifier != 0.0) j["modifier"] = modifier;
    if(specific) j["specific"] = specific;

    return j;
}

Object::Object(const nlohmann::json& j) {
    deserialize(j);

}

nlohmann::json Object::serialize() {
    auto j = GameEntity::serialize();

    for(auto i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
        if(value[i]) j["value"].push_back(std::make_pair(i, value[i]));
    }

    if(type_flag) j["type_flag"] = type_flag;
    if(level) j["level"] = level;

    if(weight != 0.0) j["weight"] = weight;
    if(cost) j["cost"] = cost;
    if(cost_per_day) j["cost_per_day"] = cost_per_day;

    for(auto & i : affected) {
        if(i.location == APPLY_NONE) continue;
        j["affected"].push_back(i.serialize());
    }

    return j;
}

void Object::deserialize(const nlohmann::json &j) {
    GameEntity::deserialize(j);

    if(j.contains("value")) {
        for(auto & i : j["value"]) {
            value[i[0].get<int>()] = i[1];
        }
    }

    if(j.contains("type_flag")) type_flag = j["type_flag"];
    if(j.contains("level")) level = j["level"];

    if(j.contains("weight")) weight = j["weight"];
    if(j.contains("cost")) cost = j["cost"];
    if(j.contains("cost_per_day")) cost_per_day = j["cost_per_day"];

    if(j.contains("affected")) {
        int counter = 0;
        for(auto & i : j["affected"]) {
            affected[counter].deserialize(i);
            counter++;
        }
    }

}

nlohmann::json Object::serializeRelations() {
    auto j = GameEntity::serializeRelations();

    if(posted_to) j["posted_to"] = posted_to->getUIDString();
    if(fellow_wall) j["fellow_wall"] = fellow_wall->getUIDString();

    return j;
}

void Object::deserializeRelations(const nlohmann::json& j) {
    GameEntity::deserializeRelations(j);
    
    if(j.contains("posted_to")) {
        auto check = resolveUID(j["posted_to"]);
        if(check) posted_to = dynamic_cast<Object*>(check);
    }
    if(j.contains("fellow_wall")) {
        auto check = resolveUID(j["fellow_wall"]);
        if(check) fellow_wall = dynamic_cast<Object*>(check);
    }
}



void Object::activate() {
    if(active) {
        basic_mud_log("Attempted to activate an already active item.");
        return;
    }
    active = true;
    next = object_list;
    object_list = this;

    script->activate();

    if(obj_proto.contains(vn)) {
        insert_vnum(objectVnumIndex, this);
    }

}

void Object::deactivate() {
    if(!active) return;
    active = false;
    Object *temp;
    REMOVE_FROM_LIST(this, object_list, next, temp);

    if(obj_proto.contains(vn)) {
        erase_vnum(objectVnumIndex, this);
    }

    script->deactivate();

}



int Object::getAffectModifier(int location, int specific) {
    int modifier = 0;
    for(auto &aff : affected) {
        if(aff.location == location && (specific == -1 || aff.specific == specific)) {
            modifier += aff.modifier;
        }
    }
    return modifier;
}

weight_t Object::getWeight() {
    return weight;
}

weight_t Object::getTotalWeight() {
    return getWeight() + getInventoryWeight() + (sitting ? sitting->getTotalWeight() : 0);
}

bool Object::isActive() {
    return active;
}

#define LOC_INVENTORY    0
void auto_equip(BaseCharacter *ch, Object *obj, int location) {
    int j;

    /* Lots of checks... */
    if (location > 0) {    /* Was wearing it. */
        switch (j = (location - 1)) {
            case WEAR_UNUSED0:
                j = WEAR_WIELD2;
                break;
            case WEAR_FINGER_R:
            case WEAR_FINGER_L:
                if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
                    location = LOC_INVENTORY;
                break;
            case WEAR_NECK_1:
            case WEAR_NECK_2:
                if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
                    location = LOC_INVENTORY;
                break;
            case WEAR_BODY:
                if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
                    location = LOC_INVENTORY;
                break;
            case WEAR_HEAD:
                if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
                    location = LOC_INVENTORY;
                break;
            case WEAR_LEGS:
                if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
                    location = LOC_INVENTORY;
                break;
            case WEAR_FEET:
                if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
                    location = LOC_INVENTORY;
                break;
            case WEAR_HANDS:
                if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
                    location = LOC_INVENTORY;
                break;
            case WEAR_ARMS:
                if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
                    location = LOC_INVENTORY;
                break;
            case WEAR_UNUSED1:
                if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
                    location = LOC_INVENTORY;
                j = WEAR_WIELD2;
                break;
            case WEAR_ABOUT:
                if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
                    location = LOC_INVENTORY;
                break;
            case WEAR_WAIST:
                if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
                    location = LOC_INVENTORY;
                break;
            case WEAR_WRIST_R:
            case WEAR_WRIST_L:
                if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
                    location = LOC_INVENTORY;
                break;
            case WEAR_WIELD1:
                if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
                    location = LOC_INVENTORY;
                break;
            case WEAR_WIELD2:
                break;
            case WEAR_EYE:
                if (!CAN_WEAR(obj, ITEM_WEAR_EYE))
                    location = LOC_INVENTORY;
                break;
            case WEAR_BACKPACK:
                if (!CAN_WEAR(obj, ITEM_WEAR_PACK))
                    location = LOC_INVENTORY;
                break;
            case WEAR_SH:
                if (!CAN_WEAR(obj, ITEM_WEAR_SH))
                    location = LOC_INVENTORY;
                break;
            case WEAR_EAR_R:
            case WEAR_EAR_L:
                if (!CAN_WEAR(obj, ITEM_WEAR_EAR))
                    location = LOC_INVENTORY;
                break;
            default:
                location = LOC_INVENTORY;
        }

        if (location > 0) {        /* Wearable. */
            if (!GET_EQ(ch, j)) {
                /*
                 * Check the characters's alignment to prevent them from being
                 * zapped through the auto-equipping.
                     */
                if (invalid_align(ch, obj) || invalid_class(ch, obj))
                    location = LOC_INVENTORY;
                else
                    equip_char(ch, obj, j);
            } else {    /* Oops, saved a player with double equipment? */
                mudlog(BRF, ADMLVL_IMMORT, true, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch),
                       location);
                location = LOC_INVENTORY;
            }
        }
    }

    if (location <= 0)    /* Inventory */
        obj->addToLocation(ch);
}





bool Object::isProvidingLight() {
    if(checkFlag(FlagType::Item, ITEM_GLOW)) return true;
    // Equipper is carrying this as a light source.
    if(locationType > 0 && GET_OBJ_TYPE(this) == ITEM_LIGHT && GET_OBJ_VAL(this, VAL_LIGHT_HOURS)) return true;
    if(GET_OBJ_TYPE(this) == ITEM_CAMPFIRE) return true;
    // Flambus stove, dammit.
    if(vn == 19093) return true;
    return false;
}


double Object::currentGravity() {
    return myEnvVar(EnvVar::Gravity);
}

bool Object::isWorking() {
    return !(OBJ_FLAGGED(this, ITEM_BROKEN) || OBJ_FLAGGED(this, ITEM_FORGED));
}

static const std::map<std::string, int> _values = {
    {"val0", 0},
    {"val1", 1},
    {"val2", 2},
    {"val3", 3},
    {"val4", 4},
    {"val5", 5},
    {"val6", 6},
    {"val7", 7}
};

DgResults Object::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    std::string lmember = member;
    to_lower(lmember);
    trim(lmember);
    
    if(lmember == "affects") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(affected_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return checkFlag(FlagType::Affect, flag) ? "1" : "0";
    }

    if(lmember == "cost") {
        if(!arg.empty()) {
            money_t addition = atoll(arg.c_str());
            cost = std::max<money_t>(0, addition + cost);
        }
        return fmt::format("{}", cost);
    }

    if(lmember == "cost_per_day") {
        if(!arg.empty()) {
            money_t addition = atoll(arg.c_str());
            cost_per_day = std::max<money_t>(0, addition + cost_per_day);
        }
        return fmt::format("{}", cost_per_day);
    }

    if(lmember == "count") {
        if(GET_OBJ_TYPE(this) == ITEM_CONTAINER) return fmt::format("{}", item_in_list((char*)arg.c_str(), getInventory()));
        return "0";
    }

    if(lmember == "carried_by") return carried_by;

    if(lmember == "contents") {
        if(arg.empty()) {
            if(auto con = getInventory(); !con.empty()) return con.front();
            return "";
        }
        obj_vnum v = atoll(arg.c_str());
        auto found = findObjectVnum(v);
        if(found) return found;
        return "";
    }

    if(lmember == "extra" || lmember == "itemflag") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(extra_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return checkFlag(FlagType::Item, flag) ? "1" : "0";
    }

    if(lmember == "has_in") {
        if(GET_OBJ_TYPE(this) == ITEM_CONTAINER) return item_in_list((char*)arg.c_str(), getInventory()) ? "1" : "0";
        return "0";
    }

    if(lmember == "health") {
        if(!arg.empty()) {
            int addition = atof(arg.c_str());
            value[VAL_ALL_HEALTH] = std::max<int>(1, addition + value[VAL_ALL_HEALTH]);
            if (OBJ_FLAGGED(this, ITEM_BROKEN) && value[VAL_ALL_HEALTH] >= 100)
                clearFlag(FlagType::Item, ITEM_BROKEN);
        }
        return fmt::format("{}", value[VAL_ALL_HEALTH]);
    }

    if(lmember == "id") return this;

    if(lmember == "in_room") {
        if(auto r = getRoom(); r) return r;
        return "";
    }

    if(lmember == "is_pc") return "-1";

    if(lmember == "level") return fmt::format("{}", GET_OBJ_LEVEL(this));

    if(lmember == "name") {
        if(!arg.empty()) {
            setName(arg);
        }
        return getName();
    }

    if(lmember == "next_in_list") {
        // Okay this one's stupid. We're not a manual linked list anymore, so for this to work we'll have to fake it.
        auto loc = getLocation();
        if(!loc) return "";
        if(locationType != 0) return "";
        auto inv = loc->getInventory();
        // so we should be in here somewhere, and if so we want to return a pointer to the next available item, if any.
        auto found = std::find(inv.begin(), inv.end(), this);
        if(found == inv.end()) return "";
        if(found+1 == inv.end()) return "";
        return *(found+1);
    }

    if(lmember == "room") {
        auto r = getRoom();
        if(r) return r;
        return "";
    }

    if(lmember == "setaffects") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(affected_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        flipFlag(FlagType::Affect, flag);
        return "1";
    }

    if(lmember == "setextra") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(extra_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        flipFlag(FlagType::Item, flag);
        return "1";
    }

    if(lmember == "shortdesc") {
        if(!arg.empty()) {
            char blah[500];
            sprintf(blah, "%s @wnicknamed @D(@C%s@D)@n", getShortDesc().c_str(), arg.c_str());
            setShortDesc(blah);
        }
        return getShortDesc();
    }

    if(lmember == "size") {
        if(!arg.empty()) {
            auto ns = search_block((char*)arg.c_str(), size_names, false);
            if(ns > -1) size = ns;
        }
        return size_names[size];
    }

    if(auto v = _values.find(lmember); v != _values.end()) return fmt::format("{}", value[v->second]);

    if(lmember == "type") return item_types[type_flag];

    if(lmember == "timer") return fmt::format("{}", GET_OBJ_TIMER(this));

    if(lmember == "weight") {
        if(!arg.empty()) {
            auto addition = atof(arg.c_str());
            weight = std::max<double>(0, weight + addition);
        }
        return fmt::format("{}", GET_OBJ_WEIGHT(this));
    }

    if(lmember == "worn_by") {
        if(worn_by) return worn_by;
        return "";
    }

    if(lmember == "vnum") {
        if(!arg.empty()) {
            auto v = atoll(arg.c_str());
            return vn == v ? "1":"0";
        }
        return fmt::format("{}", vn);
    }

    if(script->hasVar(lmember)) {
        return script->getVar(lmember);
    } else {
        script_log("Trigger: %s, VNum %d. unknown object field: '%s'",
                               GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), lmember.c_str());
    }
    return "";

}


std::string Object::getUnitClass() {
    return "obj_data";
}

UnitFamily Object::getFamily() {
    return UnitFamily::Object;
}

Object::~Object() {
    if(auctname) free(auctname);
    if(sbinfo) free(sbinfo);
}


void Object::assignTriggers() {
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


Structure::Structure(const nlohmann::json& j) {
    deserialize(j);
}

nlohmann::json Structure::serialize() {
    return Object::serialize();
}

void Structure::deserialize(const nlohmann::json& j) {
    Object::deserialize(j);
}

std::string Structure::getUnitClass() {
    return "Structure";
}

nlohmann::json Structure::serializeRelations() {
    return Object::serializeRelations();
}

void Structure::deserializeRelations(const nlohmann::json& j) {
    Object::deserializeRelations(j);
}

bool Structure::isEnvironment() {
    return true;
}

bool Structure::isStructure() {
    return true;
}