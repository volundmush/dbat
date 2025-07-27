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
#include "dbat/filter.h"

static int copy_object_main(struct obj_data *to, struct obj_data *from, int free_object);


obj_rnum add_object(struct item_proto_data *newobj, obj_vnum ovnum) {
    int found = NOTHING;
    
    /*
     * Write object to internal tables.
     */
    bool exists = obj_proto.contains(ovnum);
    auto& obj = obj_proto.at(ovnum);
    obj = *newobj;
    if (exists) {
        basic_mud_log("GenOLC: add_object: Updated existing object #%d (%s).", ovnum, obj.short_description);
        update_objects(&obj);
    } else {
        basic_mud_log("GenOLC: add_object: Added object #%d (%s).", ovnum, obj.short_description);
        zone_rnum rznum = real_zone_by_thing(ovnum);
        auto& z = zone_table.at(rznum);
        z.objects.insert(ovnum);
    }

    return ovnum;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

/*
 * Fix all existing objects to have these values.
 * We need to run through each and every object currently in the
 * game to see which ones are pointing to this prototype.
 * if object is pointing to this prototype, then we need to replace it
 * with the new one.
 */
int update_objects(struct item_proto_data *refobj) {
    int count = 0;

    auto objects = objectSubscriptions.all(fmt::format("vnum_{}", refobj->vn));
    for (auto obj : filter_raw(objects)) {
        count++;
        assign_triggers(obj, OBJ_TRIGGER);
    }

    return count;
}


int save_objects(zone_rnum zone_num) {
    return true;
}


/*
 * For object instances that are not the prototype.
 */



int delete_object(obj_rnum rnum) {
    obj_rnum i;
    zone_rnum zrnum;
    struct obj_data *tmp;
    int shop, j, zone, cmd_no;

    if (!obj_proto.count(rnum))
        return NOTHING;

    auto obj = &obj_proto.at(rnum);

    zrnum = real_zone_by_thing(rnum);

    /* This is something you might want to read about in the logs. */
    basic_mud_log("GenOLC: delete_object: Deleting object #%d (%s).", obj->vn, obj->short_description);
    auto allobj = objectSubscriptions.all(fmt::format("vnum_{}", obj->vn));
    for (auto tmp : filter_raw(allobj)) {

        /* extract_obj() will just axe contents. */
        if (auto con = tmp->getObjects(); !con.empty()) {
            
            for (auto this_content : filter_raw(con)) {
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

    assert(objectSubscriptions.count(fmt::format("vnum_{}", rnum)) == 0);
    obj_proto.erase(rnum);
    obj_index.erase(rnum);
    save_objects(zrnum);

    return rnum;
}


std::shared_ptr<obj_data> obj_data::shared() {
    return shared_from_this();
}

void obj_data::activate() {
    if(active) {
        basic_mud_log("Attempted to activate an already active item.");
        return;
    }
    active = true;
    std::unordered_set<std::string> services;

    auto vn = getVnum();
    if(obj_proto.contains(vn)) {
        services.insert(fmt::format("vnum_{}", vn));
    }

    assign_triggers(this, OBJ_TRIGGER);

    if(!scripts.empty()) {
        activateScripts();
        if(SCRIPT_TYPES(this) & OTRIG_RANDOM)
            services.insert("randomTriggers");
        if(SCRIPT_TYPES(this) & OTRIG_TIME)
            services.insert("timeTriggers");
    }
    auto sh = shared_from_this();
    services.insert("active");
    if(IS_CORPSE(this))
        services.insert("corpseRotService");
    if(SCRIPT_TYPES(this) && OTRIG_RANDOM)
        services.insert("randomTriggers");
    if(vn == 65)
        services.insert("healTankService");

    for(const auto& s : services) {
        objectSubscriptions.subscribe(s, sh);
    }

    activateContents();
}

void obj_data::deactivate() {
    if(!active) return;
    active = false;

    for(auto &[vn, sc] : scripts) {
        sc->deactivate();
    }

    auto sh = shared_from_this();
    objectSubscriptions.unsubscribeFromAll(sh);
    deactivateContents();
}


double obj_data::getAffectModifier(uint64_t location, uint64_t specific) {
    double modifier = 0;
    for(auto &aff : affected) {
        if(aff.match(location, specific)) {
            modifier += aff.modifier;
        }
    }
    return modifier;
}

weight_t obj_data::getWeight() {
    return weight;
}

weight_t obj_data::getTotalWeight() {
    auto s = sitting.lock();
    return getWeight() + getInventoryWeight() + (s ? s->getBaseStat("weight_total") : 0);
}

bool obj_data::isActive() {
    return active;
}

constexpr int LOC_INVENTORY = 0;
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
    } else if(room) {
        return room->getUID();
    } else {
        return ""; // this should NEVER happen!
    }
}

void obj_data::deserializeLocation(const std::string& txt, int16_t slot) {
    auto check = resolveUID(txt);
    if(!check) return;
    if(auto r = std::dynamic_pointer_cast<room_data>(check); r) {
        obj_to_room(this, r.get());
    } else if(auto o = std::dynamic_pointer_cast<obj_data>(check); o) {
        obj_to_obj(this, o.get());
    } else if(auto c = std::dynamic_pointer_cast<char_data>(check); c) {
        auto_equip(c.get(), this, slot+1);
    }
}

bool obj_data::isProvidingLight() {
    return GET_OBJ_TYPE(this) == ITEM_LIGHT && GET_OBJ_VAL(this, VAL_LIGHT_HOURS);
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
        return room->getEnvironment(ENV_GRAVITY);
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
    else if(room) obj_from_room(this);
}

vnum obj_data::getVnum() const {
    return proto ? proto->vn : NOTHING;
}



char* obj_data::getName() {
    if(name) return name;
    if(proto && proto->name) return proto->name;
    return nullptr;
}

char* obj_data::getRoomDescription() {
    if(room_description) return room_description;
    if(proto && proto->room_description) return proto->room_description;
    return nullptr;
}

char* obj_data::getLookDescription() {
    if(look_description) return look_description;
    if(proto && proto->look_description) return proto->look_description;
    return nullptr;
}

char* obj_data::getShortDescription() {
    if(short_description) return short_description;
    if(proto && proto->short_description) return proto->short_description;
    return nullptr;
}

extra_descr_data* obj_data::getExtraDescription() {
    if(proto && proto->ex_description) return proto->ex_description;
    return nullptr;
}

obj_data::~obj_data() {
    if(auctname) free(auctname);
    if(sbinfo) free(sbinfo);
}

std::vector<trig_vnum> obj_data::getProtoScript() const {
    return proto ? proto->proto_script : std::vector<trig_vnum>{};
}

std::string obj_data::scriptString() const {
    std::vector<std::string> vnums;
    auto proto_script = getProtoScript();
    for(auto p : proto_script) vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}