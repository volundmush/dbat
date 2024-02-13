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

static int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object);


obj_rnum add_object(struct obj_data *newobj, obj_vnum ovnum) {
    int found = NOTHING;
    zone_rnum rznum = real_zone_by_thing(ovnum);

    /*
     * Write object to internal tables.
     */
    if ((newobj->vn = real_object(ovnum)) != NOTHING) {
        copy_object(&obj_proto[newobj->vn], newobj);
        update_objects(&obj_proto[newobj->vn]);
        return newobj->vn;
    }

    found = insert_object(newobj, ovnum);

    auto &z = zone_table[rznum];
    z.objects.insert(ovnum);
    return found;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Fix all existing objects to have these values.
 * We need to run through each and every object currently in the
 * game to see which ones are pointing to this prototype.
 * if object is pointing to this prototype, then we need to replace it
 * with the new one.
 */
int update_objects(struct obj_data *refobj) {
    struct obj_data *obj, swap;
    int count = 0;

    for (obj = object_list; obj; obj = obj->next) {
        if (obj->vn != refobj->vn)
            continue;

        count++;

        /* Update the existing object but save a copy for private information. */
        swap = *obj;
        *obj = *refobj;

        /* Copy game-time dependent variables over. */
        IN_ROOM(obj) = swap.in_room;
        obj->carried_by = swap.carried_by;
        obj->worn_by = swap.worn_by;
        obj->worn_on = swap.worn_on;
        obj->in_obj = swap.in_obj;
        obj->contents = swap.contents;
        obj->next_content = swap.next_content;
        obj->next = swap.next;
    }

    return count;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Function handle the insertion of an object within the prototype framework.  Note that this does not adjust internal values
 * of other objects, use add_object() for that.
 */
obj_rnum insert_object(struct obj_data *obj, obj_vnum ovnum) {

    auto exists = obj_proto.count(ovnum);
    auto &o = obj_proto[ovnum];
    o = *obj;

    /* Not found, place at 0. */
    return false;
}


/* ------------------------------------------------------------------------------------------------------------------------------ */

int save_objects(zone_rnum zone_num) {
    return true;
}

/*
 * Free all, unconditionally.
 */
void free_object_strings(struct obj_data *obj) {
    if (obj->name)
        free(obj->name);
    if (obj->room_description)
        free(obj->room_description);
    if (obj->look_description)
        free(obj->look_description);
    if (obj->ex_description)
        free_ex_descriptions(obj->ex_description);
}

/*
 * For object instances that are not the prototype.
 */
void free_object_strings_proto(struct obj_data *obj) {
    int robj_num = GET_OBJ_RNUM(obj);

    if (obj->name && obj->name != obj_proto[robj_num].name)
        free(obj->name);
    if (obj->room_description && obj->room_description != obj_proto[robj_num].room_description)
        free(obj->room_description);
    if (obj->look_description && obj->look_description != obj_proto[robj_num].look_description)
        free(obj->look_description);
    if (obj->ex_description) {
        struct extra_descr_data *thised, *plist, *next_one; /* O(horrible) */
        int ok_key, ok_desc, ok_item;
        for (thised = obj->ex_description; thised; thised = next_one) {
            next_one = thised->next;
            for (ok_item = ok_key = ok_desc = 1, plist = obj_proto[robj_num].ex_description; plist; plist = plist->next) {
                if (plist->keyword == thised->keyword)
                    ok_key = 0;
                if (plist->description == thised->description)
                    ok_desc = 0;
                if (plist == thised)
                    ok_item = 0;
            }
            if (thised->keyword && ok_key)
                free(thised->keyword);
            if (thised->description && ok_desc)
                free(thised->description);
            if (ok_item)
                free(thised);
        }
    }
}

void copy_object_strings(struct obj_data *to, struct obj_data *from) {
    to->name = from->name ? strdup(from->name) : nullptr;
    to->room_description = from->room_description ? strdup(from->room_description) : nullptr;
    to->look_description = from->look_description ? strdup(from->look_description) : nullptr;

    if (from->ex_description)
        copy_ex_descriptions(&to->ex_description, from->ex_description);
    else
        to->ex_description = nullptr;
}

int copy_object(struct obj_data *to, struct obj_data *from) {
    free_object_strings(to);
    return copy_object_main(to, from, true);
}

int copy_object_preserve(struct obj_data *to, struct obj_data *from) {
    return copy_object_main(to, from, false);
}

static int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object) {
    *to = *from;
    copy_object_strings(to, from);
    return true;
}

int delete_object(obj_rnum rnum) {
    obj_rnum i;
    zone_rnum zrnum;
    struct obj_data *obj, *tmp;
    int shop, j, zone, cmd_no;

    if (!obj_proto.count(rnum))
        return NOTHING;

    obj = &obj_proto[rnum];

    zrnum = real_zone_by_thing(rnum);

    /* This is something you might want to read about in the logs. */
    basic_mud_log("GenOLC: delete_object: Deleting object #%d (%s).", GET_OBJ_VNUM(obj), obj->getShortDesc());

    for (tmp = object_list; tmp; tmp = tmp->next) {
        if (tmp->vn != obj->vn)
            continue;

        /* extract_obj() will just axe contents. */
        if (tmp->contents) {
            struct obj_data *this_content, *next_content;
            for (this_content = tmp->contents; this_content; this_content = next_content) {
                next_content = this_content->next_content;
                if (IN_ROOM(tmp)) {
                    /* Transfer stuff from object to room. */
                    obj_from_obj(this_content);
                    obj_to_room(this_content, IN_ROOM(tmp));
                } else if (tmp->worn_by || tmp->carried_by) {
                    /* Transfer stuff from object to person inventory. */
                    obj_from_char(this_content);
                    obj_to_char(this_content, tmp->carried_by);
                } else if (tmp->in_obj) {
                    /* Transfer stuff from object to containing object. */
                    obj_from_obj(this_content);
                    obj_to_obj(this_content, tmp->in_obj);
                }
            }
        }
        /* Remove from object_list, etc. - handles weight changes, and similar. */
        extract_obj(tmp);
    }

    /* Make sure all are removed. */

    assert(get_vnum_count(objectVnumIndex, rnum) == 0);
    obj_proto.erase(rnum);
    obj_index.erase(rnum);
    save_objects(zrnum);

    return rnum;
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


nlohmann::json obj_data::serializeBase() {
    auto j = serializeUnit();

    for(auto i = 0; i < NUM_OBJ_VAL_POSITIONS; i++) {
        if(value[i]) j["value"].push_back(std::make_pair(i, value[i]));
    }

    if(type_flag) j["type_flag"] = type_flag;
    if(level) j["level"] = level;

    for(auto i = 0; i < wear_flags.size(); i++)
        if(wear_flags.test(i)) j["wear_flags"].push_back(i);

    for(auto i = 0; i < extra_flags.size(); i++)
        if(extra_flags.test(i)) j["extra_flags"].push_back(i);

    if(weight != 0.0) j["weight"] = weight;
    if(cost) j["cost"] = cost;
    if(cost_per_day) j["cost_per_day"] = cost_per_day;

    for(auto i = 0; i < bitvector.size(); i++)
        if(bitvector.test(i)) j["bitvector"].push_back(i);

    for(auto & i : affected) {
        if(i.location == APPLY_NONE) continue;
        j["affected"].push_back(i.serialize());
    }

    return j;
}


nlohmann::json obj_data::serializeInstance() {
    auto j = serializeBase();
    if(id == -1) {
        id = nextObjID();
        generation = time(nullptr);
        check_unique_id(this);
        add_unique_id(this);
    }

    if(generation) j["generation"] = generation;

    if(!script->vars.empty()) {
        j["dgvariables"] = script->vars;
    }

    if(world.contains(room_loaded)) j["room_loaded"] = room_loaded;

    return j;
}


nlohmann::json obj_data::serializeProto() {
    auto j = serializeBase();

    for(auto p : proto_script) {
        if(trig_index.contains(p)) j["proto_script"].push_back(p);
    }

    return j;
}



void obj_data::deserializeBase(const nlohmann::json &j) {
    deserializeUnit(j);

    if(j.contains("value")) {
        for(auto & i : j["value"]) {
            value[i[0].get<int>()] = i[1];
        }
    }

    if(j.contains("type_flag")) type_flag = j["type_flag"];
    if(j.contains("level")) level = j["level"];

    if(j.contains("wear_flags")) {
        for(auto & i : j["wear_flags"]) {
            wear_flags.set(i.get<int>());
        }
    }

    if(j.contains("extra_flags")) {
        for(auto & i : j["extra_flags"]) {
            extra_flags.set(i.get<int>());
        }
    }

    if(j.contains("weight")) weight = j["weight"];
    if(j.contains("cost")) cost = j["cost"];
    if(j.contains("cost_per_day")) cost_per_day = j["cost_per_day"];

    if(j.contains("bitvector")) {
        for(auto & i : j["bitvector"]) {
            bitvector.set(i.get<int>());
        }
    }

    if(j.contains("affected")) {
        int counter = 0;
        for(auto & i : j["affected"]) {
            affected[counter].deserialize(i);
            counter++;
        }
    }

}


void obj_data::deserializeProto(const nlohmann::json& j) {
    deserializeBase(j);

    if(j.contains("proto_script")) {
        for(auto p : j["proto_script"]) proto_script.emplace_back(p.get<trig_vnum>());
    }
}


obj_data::obj_data(const nlohmann::json &j) : obj_data() {
    deserializeProto(j);

    if ((GET_OBJ_TYPE(this) == ITEM_PORTAL || \
       GET_OBJ_TYPE(this) == ITEM_HATCH) && \
       (!GET_OBJ_VAL(this, VAL_DOOR_DCLOCK) || \
        !GET_OBJ_VAL(this, VAL_DOOR_DCHIDE))) {
        GET_OBJ_VAL(this, VAL_DOOR_DCLOCK) = 20;
        GET_OBJ_VAL(this, VAL_DOOR_DCHIDE) = 20;
    }

    GET_OBJ_SIZE(this) = SIZE_MEDIUM;

/* check to make sure that weight of containers exceeds curr. quantity */
    if (GET_OBJ_TYPE(this) == ITEM_DRINKCON ||
        GET_OBJ_TYPE(this) == ITEM_FOUNTAIN) {
        if (GET_OBJ_WEIGHT(this) < GET_OBJ_VAL(this, 1))
            GET_OBJ_WEIGHT(this) = GET_OBJ_VAL(this, 1) + 5;
    }
    /* *** make sure portal objects have their timer set correctly *** */
    if (GET_OBJ_TYPE(this) == ITEM_PORTAL) {
        GET_OBJ_TIMER(this) = -1;
    }
    
}

std::optional<vnum> obj_data::getMatchingArea(const std::function<bool(const area_data &)>& f) {
    if(auto room = getAbsoluteRoom(); room) {
        return room->getMatchingArea(f);
    }
    return std::nullopt;
}

void obj_data::activate() {
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

    if(contents) activateContents();
}

void obj_data::deactivate() {
    if(!active) return;
    active = false;
    struct obj_data *temp;
    REMOVE_FROM_LIST(this, object_list, next, temp);

    if(obj_proto.contains(vn)) {
        erase_vnum(objectVnumIndex, this);
    }

    script->deactivate();

    if(contents) deactivateContents();
}

void obj_data::deserializeInstance(const nlohmann::json &j, bool isActive) {
    deserializeBase(j);

    if(j.contains("generation")) generation = j["generation"];
    check_unique_id(this);
    add_unique_id(this);

    if(j.contains("dgvariables")) {
        script->vars = j["dgvariables"].get<std::unordered_map<std::string, std::string>>();
    }

    if(j.contains("room_loaded")) room_loaded = j["room_loaded"];

    auto proto = obj_proto.find(vn);
    if(proto != obj_proto.end()) {
        proto_script = proto->second.proto_script;
    }

    if(isActive) activate();

}


int obj_data::getAffectModifier(int location, int specific) {
    int modifier = 0;
    for(auto &aff : affected) {
        if(aff.location == location && (specific == -1 || aff.specific == specific)) {
            modifier += aff.modifier;
        }
    }
    return modifier;
}

weight_t obj_data::getWeight() {
    return weight;
}

weight_t obj_data::getTotalWeight() {
    return getWeight() + getInventoryWeight() + (sitting ? sitting->getTotalWeight() : 0);
}

std::string obj_data::getUID(bool active) {
    return fmt::format("#O{}:{}{}", id, generation, active ? "" : "!");
}

bool obj_data::isActive() {
    return active;
}

#define LOC_INVENTORY    0
void auto_equip(struct char_data *ch, struct obj_data *obj, int location) {
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
        obj_to_char(obj, ch);
}

std::string obj_data::serializeLocation() {
    if(in_obj) {
        return in_obj->getUID();
    } else if(carried_by) {
        return carried_by->getUID();
    } else if(worn_by) {
        return worn_by->getUID();
    } else if(world.contains(in_room)) {
        return world[in_room]->getUID();
    } else {
        return ""; // this should NEVER happen!
    }
}

nlohmann::json obj_data::serializeRelations() {
    auto j = nlohmann::json::object();

    if(posted_to) j["posted_to"] = posted_to->getUID();
    if(fellow_wall) j["fellow_wall"] = fellow_wall->getUID();

    return j;
}

void obj_data::deserializeLocation(const std::string& txt, int16_t slot) {
    auto check = resolveUID(txt);
    if(!check) return;
    auto idx = check->index();
    if(idx == 0) {
        auto &r = std::get<0>(*check);
        obj_to_room(this, r->vn);
    } else if(idx == 1) {
        obj_to_obj(this, std::get<1>(*check));
    } else if(idx == 2) {
        auto &c = std::get<2>(*check);
        auto_equip(c, this, slot+1);
    }
}

void obj_data::deserializeRelations(const nlohmann::json& j) {
    if(j.contains("posted_to")) {
        auto check = resolveUID(j["posted_to"]);
        if(check) posted_to = std::get<1>(*check);
    }
    if(j.contains("fellow_wall")) {
        auto check = resolveUID(j["fellow_wall"]);
        if(check) fellow_wall = std::get<1>(*check);
    }
}

void obj_data::save() {
    if(id == NOTHING) return;
}

bool obj_data::isProvidingLight() {
    return GET_OBJ_TYPE(this) == ITEM_LIGHT && GET_OBJ_VAL(this, VAL_LIGHT_HOURS);
}

struct room_data* obj_data::getRoom() {
    auto roomFound = world.find(in_room);
    if(roomFound != world.end()) return roomFound->second;
    return nullptr;
}

struct room_data* obj_data::getAbsoluteRoom() {
    if(auto room = getRoom(); room) {
        return room;
    }
    if(in_obj) return in_obj->getAbsoluteRoom();
    else if(carried_by) return carried_by->getRoom();
    else if(worn_by) return worn_by->getRoom();
    return nullptr;
}

double obj_data::currentGravity() {
    if(auto room = getAbsoluteRoom(); room) {
        return room->getGravity();
    }
    return 1.0;
}

bool obj_data::isWorking() {
    return !(OBJ_FLAGGED(this, ITEM_BROKEN) || OBJ_FLAGGED(this, ITEM_FORGED));
}

void obj_data::clearLocation() {
    if(in_obj) obj_from_obj(this);
    else if(carried_by) obj_from_char(this);
    else if(worn_by) unequip_char(worn_by, worn_on);
    else if(world.contains(in_room)) obj_from_room(this);
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

DgResults obj_data::dgCallMember(trig_data *trig, const std::string& member, const std::string& arg) {
    std::string lmember = member;
    to_lower(lmember);
    trim(lmember);
    
    if(lmember == "affects") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(affected_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return bitvector.test(flag) ? "1" : "0";
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
        if(GET_OBJ_TYPE(this) == ITEM_CONTAINER) return fmt::format("{}", item_in_list((char*)arg.c_str(), contents));
        return "0";
    }

    if(lmember == "carried_by") return carried_by;

    if(lmember == "contents") {
        if(arg.empty()) return contents;
        obj_vnum v = atoll(arg.c_str());
        auto found = findObjectVnum(v);
        if(found) return found;
        return "";
    }

    if(lmember == "extra" || lmember == "itemflag") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(extra_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        return extra_flags.test(flag) ? "1" : "0";
    }

    if(lmember == "has_in") {
        if(GET_OBJ_TYPE(this) == ITEM_CONTAINER) return item_in_list((char*)arg.c_str(), contents) ? "1" : "0";
        return "0";
    }

    if(lmember == "health") {
        if(!arg.empty()) {
            int addition = atof(arg.c_str());
            value[VAL_ALL_HEALTH] = std::max<int>(1, addition + value[VAL_ALL_HEALTH]);
            if (OBJ_FLAGGED(this, ITEM_BROKEN) && value[VAL_ALL_HEALTH] >= 100)
                extra_flags.reset(ITEM_BROKEN);
        }
        return fmt::format("{}", value[VAL_ALL_HEALTH]);
    }

    if(lmember == "id") return this;

    if(lmember == "in_room") {
        if (auto roomFound = world.find(in_room); roomFound != world.end())
            return roomFound->second;
        return "";
    }

    if(lmember == "is_pc") return "-1";

    if(lmember == "level") return fmt::format("{}", GET_OBJ_LEVEL(this));

    if(lmember == "name") {
        if(!arg.empty()) {
            name = strdup(arg.c_str());
        }
        return name;
    }

    if(lmember == "next_in_list") {
        if(next_content) return next_content;
        return "";
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
        bitvector.flip(flag);
        return "1";
    }

    if(lmember == "setextra") {
        if(arg.empty()) return "0";
        int flag = get_flag_by_name(extra_bits, (char*)arg.c_str());
        if(flag == -1) return "0";
        extra_flags.flip(flag);
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