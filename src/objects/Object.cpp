#include "dbat/structs.h"
#include "dbat/db.h"
#include "dbat/utils.h"
#include "dbat/constants.h"
#include "dbat/dg_scripts.h"

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

bool Object::isProvidingLight() {
    if(checkFlag(FlagType::Item, ITEM_GLOW)) return true;
    // Equipper is carrying this as a light source.
    if(GET_OBJ_TYPE(this) == ITEM_LIGHT && GET_OBJ_VAL(this, VAL_LIGHT_HOURS)) return true;
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
        auto loc = reg.try_get<Location>(ent);
        if(!loc) return "";
        if(!loc->location) return "";
        if(loc->locationType != 0) return "";
        auto inv = loc->location->getInventory();
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
    return "Object";
}

EntityFamily Object::getFamily() {
    return EntityFamily::Object;
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

std::string Object::renderRoomListingHelper(GameEntity *viewer) {
    std::string result;

    if (GET_OBJ_POSTTYPE(this) > 0) {
        result += fmt::sprintf("%s@w, has been posted here.@n", getShortDesc());
    } else {
        result += fmt::sprintf("%s@n", getRoomDesc());
    }

    if (type_flag == ITEM_VEHICLE) {
        if (!OBJVAL_FLAGGED(this, CONT_CLOSED) && vn > 19199)
            result += fmt::sprintf("\r\n@c...its outer hatch is open@n");
        else if (!OBJVAL_FLAGGED(this, CONT_CLOSED) && vn <= 19199)
            result += fmt::sprintf("\r\n@c...its door is open@n");
    }
    if (type_flag == ITEM_CONTAINER && !IS_CORPSE(this)) {
        if (!OBJVAL_FLAGGED(this, CONT_CLOSED) && !OBJ_FLAGGED(this, ITEM_SHEATH))
            result += fmt::sprintf(". @D[@G-open-@D]@n");
        else if (!OBJ_FLAGGED(this, ITEM_SHEATH))
            result += fmt::sprintf(". @D[@rclosed@D]@n");
    }
    if (type_flag == ITEM_HATCH) {
        if (!OBJVAL_FLAGGED(this, CONT_CLOSED))
            result += fmt::sprintf(", it is open");
        else if (OBJVAL_FLAGGED(this, CONT_CLOSED))
            result += fmt::sprintf(", it is closed");
        if (OBJVAL_FLAGGED(this, CONT_LOCKED))
            result += fmt::sprintf(" and locked@n");
        else
            result += fmt::sprintf("@n");
    }
    if (type_flag == ITEM_FOOD) {
        if (GET_OBJ_VAL(this, VAL_FOOD_FOODVAL) < FOOB(this)) {
            result += fmt::sprintf(", and it has been ate on@n");
        }
    }
    
    return result;
}

std::string Object::renderInventoryListingFor(GameEntity *viewer) {
    std::vector<std::string> results;
    results.emplace_back(renderListPrefixFor(viewer));
    results.emplace_back(renderInventoryListingHelper(viewer));

    return join(results, " ") + "@n";
}

std::string Object::renderInventoryListingHelper(GameEntity* viewer) {
    std::string result;

    if (viewer->checkFlag(FlagType::Pref, PRF_IHEALTH)) {
        result += fmt::sprintf("@D<@gH@D: @C%d@D>@w %s", GET_OBJ_VAL(this, VAL_ALL_HEALTH), getShortDesc());
    } else {
        result += fmt::sprintf("%s", getShortDesc());
    }
    if (type_flag == ITEM_FOOD) {
        if (GET_OBJ_VAL(this, VAL_FOOD_FOODVAL) < FOOB(this)) {
            result += fmt::sprintf(", and it has been ate on.@n");
        }
    }
    if (vn == 255) {
        switch (GET_OBJ_VAL(this, 0)) {
            case 0:
            case 1:
                result += fmt::sprintf(" @D[@wQuality @RC@D]@n");
                break;
            case 2:
                result += fmt::sprintf(" @D[@wQuality @RC+@D]@n");
                break;
            case 3:
                result += fmt::sprintf(" @D[@wQuality @yC++@D]@n");
                break;
            case 4:
                result += fmt::sprintf(" @D[@wQuality @yB@D]@n");
                break;
            case 5:
                result += fmt::sprintf(" @D[@wQuality @CB+@D]@n");
                break;
            case 6:
                result += fmt::sprintf(" @D[@wQuality @CB++@D]@n");
                break;
            case 7:
                result += fmt::sprintf(" @D[@wQuality @CA@D]@n");
                break;
            case 8:
                result += fmt::sprintf(" @D[@wQuality @GA+@D]@n");
                break;
        }
    }

    if (vn == 3424) {
        result += fmt::sprintf(" @D[@bInk Remaining@D: @w%d@D]@n", GET_OBJ_VAL(this, 6));
    }
    if (vn == 3423) {
        result += fmt::sprintf(" @D[@B%d@D/@B24 Inks@D]@n", GET_OBJ_VAL(this, 6));
    }
    if (OBJ_FLAGGED(this, ITEM_THROW)) {
        result += fmt::sprintf(" @D[@RThrow Only@D]@n");
    }
    if (type_flag == ITEM_PLANT && !OBJ_FLAGGED(this, ITEM_MATURE)) {
        if (GET_OBJ_VAL(this, VAL_WATERLEVEL) < -9) {
            result += fmt::sprintf("@D[@RDead@D]@n");
        } else {
            switch (GET_OBJ_VAL(this, VAL_MATURITY)) {
                case 0:
                    result += fmt::sprintf(" @D[@ySeed@D]@n");
                    break;
                case 1:
                    result += fmt::sprintf(" @D[@GSprout@D]@n");
                    break;
                case 2:
                    result += fmt::sprintf(" @D[@GYoung@D]@n");
                    break;
                case 3:
                    result += fmt::sprintf(" @D[@GMature@D]@n");
                    break;
                case 4:
                    result += fmt::sprintf(" @D[@GBudding@D]@n");
                    break;
                case 5:
                    result += fmt::sprintf("@D[@GClose Harvest@D]@n");
                    break;
                case 6:
                    result += fmt::sprintf("@D[@gHarvest@D]@n");
                    break;
            }
        }
    }
    if (GET_OBJ_TYPE(this) == ITEM_CONTAINER && !IS_CORPSE(this)) {
        if (!OBJVAL_FLAGGED(this, CONT_CLOSED) && !OBJ_FLAGGED(this, ITEM_SHEATH))
            result += fmt::sprintf(" @D[@G-open-@D]@n");
        else if (!OBJ_FLAGGED(this, ITEM_SHEATH))
            result += fmt::sprintf(" @D[@rclosed@D]@n");
    }
    if (OBJ_FLAGGED(this, ITEM_DUPLICATE)) {
        result += fmt::sprintf(" @D[@YDuplicate@D]@n");
    }

    return result;
}

std::string Object::renderAppearanceHelper(GameEntity *viewer) {
    return "";
}

std::string Object::renderAppearance(GameEntity* viewer) {
    std::string result = renderAppearanceHelper(viewer);


    result += renderDiagnostics(viewer);
    result += fmt::sprintf("It appears to be made of %s, and weighs %s", material_names[GET_OBJ_MATERIAL(this)],
                    add_commas(GET_OBJ_WEIGHT(this)).c_str());

    return result;
}

std::string Object::renderDiagnostics(GameEntity *viewer) {
    struct {
        int percent;
        const char *text;
    } diagnosis[] = {
            {100, "is in excellent condition."},
            {90,  "has a few scuffs."},
            {75,  "has some small scuffs and scratches."},
            {50,  "has quite a few scratches."},
            {30,  "has some big nasty scrapes and scratches."},
            {15,  "looks pretty damaged."},
            {0,   "is in awful condition."},
            {-1,  "is in need of repair."},
    };

    int percent, ar_index;
    std::string objs = viewer->canSee(this) ? getShortDesc() : "something";

    if (GET_OBJ_VAL(this, VAL_ALL_MAXHEALTH) > 0)
        percent = (100 * GET_OBJ_VAL(this, VAL_ALL_HEALTH)) / GET_OBJ_VAL(this, VAL_ALL_MAXHEALTH);
    else
        percent = 0;               /* How could MAX_HIT be < 1?? */

    for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
        if (percent >= diagnosis[ar_index].percent)
            break;

    return fmt::sprintf("\r\n%c%s %s\r\n", UPPER(objs[0]), objs.substr(1), diagnosis[ar_index].text);
}

std::string Object::renderModifiers(GameEntity *viewer) {
    std::string result;

    if (OBJ_FLAGGED(this, ITEM_INVISIBLE)) {
        result += fmt::sprintf(" (invisible)");
    }
    if (OBJ_FLAGGED(this, ITEM_BLESS) && viewer->checkFlag(FlagType::Affect, AFF_DETECT_ALIGN)) {
        result += fmt::sprintf(" ..It glows blue!");
    }
    if (OBJ_FLAGGED(this, ITEM_MAGIC) && viewer->checkFlag(FlagType::Affect, AFF_DETECT_MAGIC)) {
        result += fmt::sprintf(" ..It glows yellow!");
    }
    if (OBJ_FLAGGED(this, ITEM_GLOW)) {
        result += fmt::sprintf(" @D(@GGlowing@D)@n");
    }
    if (OBJ_FLAGGED(this, ITEM_HOT)) {
        result += fmt::sprintf(" @D(@RHOT@D)@n");
    }
    if (OBJ_FLAGGED(this, ITEM_HUM)) {
        result += fmt::sprintf(" @D(@RHumming@D)@n");
    }
    if (OBJ_FLAGGED(this, ITEM_SLOT2)) {
        if (OBJ_FLAGGED(this, ITEM_SLOT_ONE) && !OBJ_FLAGGED(this, ITEM_SLOTS_FILLED))
            result += fmt::sprintf(" @D[@m1/2 Tokens@D]@n");
        else if (OBJ_FLAGGED(this, ITEM_SLOTS_FILLED))
            result += fmt::sprintf(" @D[@m2/2 Tokens@D]@n");
        else
            result += fmt::sprintf(" @D[@m0/2 Tokens@D]@n");
    }
    if (OBJ_FLAGGED(this, ITEM_SLOT1)) {
        if (OBJ_FLAGGED(this, ITEM_SLOTS_FILLED))
            result += fmt::sprintf(" @D[@m1/1 Tokens@D]@n");
        else
            result += fmt::sprintf(" @D[@m0/1 Tokens@D]@n");
    }
    if (KICHARGE(this) > 0) {
        int num = (KIDIST(this) * 20) + rand_number(1, 5);
        result += fmt::sprintf(" %d meters away", num);
    }
    if (OBJ_FLAGGED(this, ITEM_CUSTOM)) {
        result += fmt::sprintf(" @D(@YCUSTOM@D)@n");
    }
    if (OBJ_FLAGGED(this, ITEM_RESTRING)) {
        //result += fmt::sprintf(" @D(@R%s@D)@n", GET_ADMLEVEL(ch) > 0 ? !obj->getName().empty() : "*");
    }
    if (OBJ_FLAGGED(this, ITEM_BROKEN)) {
        if (GET_OBJ_VAL(this, VAL_ALL_MATERIAL) == MATERIAL_STEEL ||
            GET_OBJ_VAL(this, VAL_ALL_MATERIAL) == MATERIAL_MITHRIL ||
            GET_OBJ_VAL(this, VAL_ALL_MATERIAL) == MATERIAL_METAL) {
            result += fmt::sprintf(", and appears to be twisted and broken.");
        } else if (GET_OBJ_VAL(this, VAL_ALL_MATERIAL) == MATERIAL_WOOD) {
            result += fmt::sprintf(", and is broken into hundreds of splinters.");
        } else if (GET_OBJ_VAL(this, VAL_ALL_MATERIAL) == MATERIAL_GLASS) {
            result += fmt::sprintf(", and is shattered on the ground.");
        } else if (GET_OBJ_VAL(this, VAL_ALL_MATERIAL) == MATERIAL_STONE) {
            result += fmt::sprintf(", and is a pile of rubble.");
        } else {
            result += fmt::sprintf(", and is broken.");
        }
    } else {
        if (GET_OBJ_TYPE(this) != ITEM_BOARD) {
            if (GET_OBJ_TYPE(this) != ITEM_CONTAINER) {
                result += fmt::sprintf(".");
            }
            if (auto obj2 = GET_OBJ_POSTED(this); obj2 && GET_OBJ_POSTTYPE(this) <= 0) {
                auto dvnum = viewer->checkFlag(FlagType::Pref, PRF_ROOMFLAGS) ? fmt::sprintf("@D[@G%d@D]@w ", GET_OBJ_VNUM(obj2)) : "";
                result += fmt::sprintf("\n...%s%s has been posted to it.", dvnum, obj2->getShortDesc());
            }
        }
    }
    return result;
}

std::string Object::renderRoomListingFor(GameEntity *viewer) {
    std::vector<std::string> results;
    if(auto pref = renderListPrefixFor(viewer); !pref.empty()) results.push_back(pref);
    if(auto helper = renderRoomListingHelper(viewer); !helper.empty()) results.push_back(helper);
    return join(results, "@n ") + renderModifiers(viewer);
}