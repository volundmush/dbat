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
    int count = 0;

    auto objects = objectSubscriptions.all(fmt::format("vnum_{}", refobj->vn));
    for (auto obj : filter_raw(objects)) {
        count++;
        // TODO: Reimplement this.
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
    if (obj->short_description)
        free(obj->short_description);
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
    if (obj->short_description && obj->short_description != obj_proto[robj_num].short_description)
        free(obj->short_description);
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
    to->short_description = from->short_description ? strdup(from->short_description) : nullptr;
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
    basic_mud_log("GenOLC: delete_object: Deleting object #%d (%s).", GET_OBJ_VNUM(obj), obj->short_description);
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

    if(obj_proto.contains(vn)) {
        services.insert(fmt::format("vnum_{}", vn));
    }

    if(trig_list) {
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

    if(trig_list) {
        struct trig_data *next_trig;
        for (auto trig = trig_list; trig; trig = next_trig) {
            next_trig = trig->next;
            extract_trigger(trig);
        }
        trig_list = nullptr;
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
