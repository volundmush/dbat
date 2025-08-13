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

Object::Object()
{
    type = UnitType::object;
}

obj_rnum add_object(ObjectPrototype *newobj, obj_vnum ovnum)
{
    int found = NOTHING;

    /*
     * Write object to internal tables.
     */
    bool exists = obj_proto.contains(ovnum);
    auto &obj = obj_proto.at(ovnum);
    obj = *newobj;
    if (exists)
    {
        basic_mud_log("GenOLC: add_object: Updated existing object #%d (%s).", ovnum, obj.short_description);
        update_objects(&obj);
    }
    else
    {
        basic_mud_log("GenOLC: add_object: Added object #%d (%s).", ovnum, obj.short_description);
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
int update_objects(ObjectPrototype *refobj)
{
    int count = 0;

    auto objects = objectSubscriptions.all(fmt::format("vnum_{}", refobj->vn));
    for (auto obj : filter_raw(objects))
    {
        count++;
        assign_triggers(obj, OBJ_TRIGGER);
    }

    return count;
}

/*
 * For object instances that are not the prototype.
 */

int delete_object(obj_rnum rnum)
{
    obj_rnum i;
    Object *tmp;
    int shop, j, zone, cmd_no;

    if (!obj_proto.count(rnum))
        return NOTHING;

    auto obj = &obj_proto.at(rnum);

    /* This is something you might want to read about in the logs. */
    basic_mud_log("GenOLC: delete_object: Deleting object #%d (%s).", obj->vn, obj->short_description);
    auto allobj = objectSubscriptions.all(fmt::format("vnum_{}", obj->vn));
    for (auto tmp : filter_raw(allobj))
    {

        /* Remove from object_list, etc. - handles weight changes, and similar. */
        extract_obj(tmp);
    }

    /* Make sure all are removed. */

    assert(objectSubscriptions.count(fmt::format("vnum_{}", rnum)) == 0);
    obj_proto.erase(rnum);
    obj_index.erase(rnum);

    return rnum;
}

std::shared_ptr<Object> Object::shared()
{
    return shared_from_this();
}

void Object::activate()
{
    if (active)
    {
        basic_mud_log("Attempted to activate an already active item.");
        return;
    }
    active = true;
    std::unordered_set<std::string> services;

    auto vn = getVnum();
    if (obj_proto.contains(vn))
    {
        services.insert(fmt::format("vnum_{}", vn));
    }

    assign_triggers(this, OBJ_TRIGGER);

    if (!scripts.empty())
    {
        activateScripts();
        if (SCRIPT_TYPES(this) & OTRIG_RANDOM)
            services.insert("randomTriggers");
        if (SCRIPT_TYPES(this) & OTRIG_TIME)
            services.insert("timeTriggers");
    }
    auto sh = shared_from_this();
    services.insert("active");
    if (IS_CORPSE(this))
        services.insert("corpseRotService");
    if (SCRIPT_TYPES(this) && OTRIG_RANDOM)
        services.insert("randomTriggers");
    if (vn == 65)
        services.insert("healTankService");

    for (const auto &s : services)
    {
        objectSubscriptions.subscribe(s, sh);
    }

    activateInventory();
}

void Object::deactivate()
{
    if (!active)
        return;
    active = false;

    for (auto &[vn, sc] : scripts)
    {
        sc->deactivate();
    }

    auto sh = shared_from_this();
    objectSubscriptions.unsubscribeFromAll(sh);
    deactivateInventory();
}

double Object::getAffectModifier(uint64_t location, uint64_t specific)
{
    double modifier = 0;
    for (auto &aff : affected)
    {
        if (aff.match(location, specific))
        {
            modifier += aff.modifier;
        }
    }
    return modifier;
}

bool Object::isActive() const
{
    return active;
}

constexpr int LOC_INVENTORY = 0;
void auto_equip(Character *ch, Object *obj, int location)
{
    int j;

    /* Lots of checks... */
    if (location > 0)
    { /* Was wearing it. */
        switch (j = (location - 1))
        {
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

        if (location > 0)
        { /* Wearable. */
            if (!GET_EQ(ch, j))
            {
                /*
                 * Check the characters's alignment to prevent them from being
                 * zapped through the auto-equipping.
                 */
                if (invalid_align(ch, obj) || invalid_class(ch, obj))
                    location = LOC_INVENTORY;
                else
                    equip_char(ch, obj, j);
            }
            else
            { /* Oops, saved a player with double equipment? */
                mudlog(BRF, ADMLVL_IMMORT, true, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch),
                       location);
                location = LOC_INVENTORY;
            }
        }
    }

    if (location <= 0) /* Inventory */
        ch->addToInventory(obj);
}

bool Object::isProvidingLight()
{
    return GET_OBJ_TYPE(this) == ITEM_LIGHT && GET_OBJ_VAL(this, VAL_LIGHT_HOURS);
}

Room *Object::getAbsoluteRoom()
{
    if (auto r = getRoom())
    {
        return r;
    }
    else if (auto c = getCarriedBy())
    {
        return c->getRoom();
    }
    else if (auto c = getWornBy())
    {
        return c->getRoom();
    }
    else if (auto o = getContainer())
    {
        return o->getAbsoluteRoom();
    }

    return nullptr;
}

double Object::currentGravity()
{
    return location.getEnvironment(ENV_GRAVITY);
}

bool Object::isWorking()
{
    return !(OBJ_FLAGGED(this, ITEM_BROKEN) || OBJ_FLAGGED(this, ITEM_FORGED));
}

void Object::clearLocation()
{
}

Object::~Object()
{
    extract_script(this, type);
    if (auctname)
        free(auctname);
}

const char *Object::getDgName() const
{
    return getName();
}

std::vector<trig_vnum> Object::getProtoScript() const
{
    auto v = getVnum();
    if (obj_proto.contains(v))
    {
        return obj_proto.at(v).proto_script;
    }
    return {};
}

ObjectPrototype *Object::getProto() const
{
    if (obj_proto.contains(vn))
    {
        return &obj_proto.at(vn);
    }
    return nullptr;
}

Object *Object::getContainer() const
{
    if (auto o = container.lock())
    {
        return o.get();
    }
    return nullptr;
}

Character *Object::getCarriedBy() const
{
    if (auto c = carrier.lock(); c && worn_on == -1)
    {
        return c.get();
    }
    return nullptr;
}

Character *Object::getWornBy() const
{
    if (auto c = carrier.lock(); c && worn_on != -1)
    {
        return c.get();
    }
    return nullptr;
}

int16_t Object::getWornOn() const
{
    if (auto c = carrier.lock(); c && worn_on != -1)
    {
        return worn_on;
    }
    return -1;
}

void Object::onAddToInventory(const std::shared_ptr<Object> &obj)
{
    obj->container = shared_from_this();
    obj->carrier.reset();
    obj->worn_on = -1;
}

void Object::onRemoveFromInventory(const std::shared_ptr<Object> &obj)
{
    obj->container.reset();
    obj->carrier.reset();
    obj->worn_on = -1;
}

std::string Object::getUID(bool active) const
{
    return fmt::format("#O{}{}", id, active ? "!" : "");
}

Location Object::getAbsoluteLocation() const
{
    if (location)
    {
        return location;
    }
    else if (auto o = getContainer())
    {
        return o->getAbsoluteLocation();
    }
    else if (auto c = getCarriedBy())
    {
        return c->location;
    }
    else if (auto c = getWornBy())
    {
        return c->location;
    }
    return {};
}

void Object::onAddToLocation(const Location &loc)
{
}

void Object::onRemoveFromLocation(const Location &loc)
{
}

void Object::onLocationChanged(const Location &oldloc, const Location &newloc)
{
}

void Object::addToLocation(const Location &loc)
{
    if (!loc.unit)
        return;
    loc.unit->addToContents(loc.position, shared_from_this());
}

void Object::removeFromLocation()
{
    if (!location.unit)
        return;
    location.unit->removeFromContents(shared_from_this());
}