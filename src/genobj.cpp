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
#include "dbat/shop.h"
#include "dbat/filter.h"
#include "dbat/dg_scripts.h"

obj_data::obj_data() : thing_data() {
    type = UnitType::object;
}

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
        if (auto con = tmp->getObjects(); !con.empty() && tmp->location.unit) {

            switch(tmp->location.getType()) {
                case UnitType::room: {
                    auto r = static_cast<room_data*>(tmp->location.unit);
                    for(auto this_content : filter_raw(con)) {
                        /* Transfer stuff from object to room. */
                        obj_from_obj(this_content);
                        obj_to_room(this_content, r);
                    }
                }
                    break;
                case UnitType::character: {
                    auto c = static_cast<char_data*>(tmp->location.unit);
                    for(auto this_content : filter_raw(con)) {
                        /* Transfer stuff from object to person inventory. */
                        obj_from_char(this_content);
                        obj_to_char(this_content, c);
                    }
                }
                    break;
                case UnitType::object: {
                    auto o = static_cast<obj_data*>(tmp->location.unit);
                    for(auto this_content : filter_raw(con)) {
                        /* Transfer stuff from object to containing object. */
                        obj_from_obj(this_content);
                        obj_to_obj(this_content, o);
                    }
                }
                    break;
                default:
                    basic_mud_log("SYSERR: delete_object: Unknown container type for object being deleted.");
                    break;
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


void obj_data::deserializeLocation(const std::string& txt, double x, double y, double z) {
    auto check = resolveUID(txt);
    if(!check) return;
    if(auto r = std::dynamic_pointer_cast<room_data>(check); r) {
        obj_to_room(this, r.get());
    } else if(auto o = std::dynamic_pointer_cast<obj_data>(check); o) {
        obj_to_obj(this, o.get());
    } else if(auto c = std::dynamic_pointer_cast<char_data>(check); c) {
        auto_equip(c.get(), this, x);
    }
}

bool obj_data::isProvidingLight() {
    return GET_OBJ_TYPE(this) == ITEM_LIGHT && GET_OBJ_VAL(this, VAL_LIGHT_HOURS);
}

struct room_data* obj_data::getAbsoluteRoom() {
    auto loc = location.unit;
    while(loc && (loc->type != UnitType::room)) {
        loc = loc->location.unit;
    }
    return (room_data*)loc;
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
    if(!location.unit) return;
    switch(location.getType()) {
        case UnitType::room:
            obj_from_room(this);
            break;
        case UnitType::character:
            if(location.position.x >= 0.0) {
                unequip_char((char_data*)location.unit, location.position.x);
            } else {
                obj_from_char(this);
            }
            break;
        case UnitType::object:
            obj_from_obj(this);
            break;
        default:
            break;
    }
    location.position.x = 0.0;
    location.position.y = 0.0;
    location.position.z = 0.0;
}

void obj_data::setLocation(room_data* room) {
    if(!room) return;
    obj_to_room(this, room);
}

void obj_data::setLocation(room_vnum rv) {
    auto r = real_room(rv);
    setLocation(r);
}


void obj_data::setLocation(const Location& loc) {
    if(!loc.unit) return;
    if(loc.unit->type == UnitType::room) {
        setLocation(static_cast<room_data*>(loc.unit));
    }
}

void obj_data::setLocation(const thing_data* td) {
    if(!td) return;
    setLocation(td->getLocation());

}

obj_data::~obj_data() {
    if(auctname) free(auctname);
}

std::vector<trig_vnum> obj_data::getProtoScript() const {
    auto v = getVnum();
    if(obj_proto.contains(v)) {
        return obj_proto.at(v).proto_script;
    }
    return {};
}

item_proto_data* obj_data::getProto() const {
    if(obj_proto.contains(vn)) {
        return &obj_proto.at(vn);
    }
    return nullptr;
}

obj_data* obj_data::getContainer() const {
    if(location.getType() != UnitType::object) return nullptr;
    return static_cast<obj_data*>(location.unit);
}

char_data* obj_data::getCarriedBy() const {
    if(location.getType() != UnitType::character) return nullptr;
    if(location.position.x != -1.0) return nullptr;
    return static_cast<char_data*>(location.unit);
}

char_data* obj_data::getWornBy() const {
    if(location.getType() != UnitType::character) return nullptr;
    if(location.position.x == -1.0) return nullptr;
    return static_cast<char_data*>(location.unit);
}

int16_t obj_data::getWornOn() const {
    if(location.getType() != UnitType::character) return -1;
    if(location.position.x == -1.0) return -1;
    return location.position.x;
}