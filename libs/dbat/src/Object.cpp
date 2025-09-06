#include "dbat/ObjectUtils.h"
#include "dbat/ObjectPrototype.h"
#include "dbat/CharacterUtils.h"
#include "dbat/Zone.h"
#include "dbat/class.h"
#include "dbat/utils.h"
#include "dbat/handler.h"
#include "dbat/Shop.h"
#include "dbat/filter.h"
#include "dbat/dg_scripts.h"
#include "dbat/act.informative.h"
#include "dbat/const/Environment.h"

std::unordered_map<int64_t, std::shared_ptr<Object>> Object::registry;
SubscriptionManager<Object> objectSubscriptions;

Object::Object()
{
    type = UnitType::object;
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
        return obj_proto.at(v)->proto_script;
    }
    return {};
}

ObjectPrototype *Object::getProto() const
{
    if (obj_proto.contains(vn))
    {
        return obj_proto.at(vn).get();
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

Location Object::getAbsoluteLocation()
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

void Object::onMoveToLocation(const Location &loc)
{
    setBaseStat("lload", time(nullptr));
    auto z = loc.getZone();
    z->objectsInZone.add(shared_from_this());
}

void Object::onLeaveLocation(const Location &loc)
{
    auto z = loc.getZone();
    auto sh = shared_from_this();
    z->objectsInZone.remove(sh);
    // clear the spawn data.
    registeredLocations.erase("spawn");

    if (type_flag == ItemType::plant)
        objectSubscriptions.unsubscribe("growingPlants", sh);

    if (auto o = GET_OBJ_POSTED(this); o)
    {
        if (GET_OBJ_POSTTYPE(this) <= 0)
        {
            o->location.send_to("%s@W shakes loose from %s@W.@n\r\n", o->getShortDescription(),
                                getShortDescription());
        }
        else
        {
            o->location.send_to("%s@W comes loose from %s@W.@n\r\n", getShortDescription(),
                                o->getShortDescription());
        }
        GET_OBJ_POSTED(o) = nullptr;
        GET_OBJ_POSTTYPE(o) = 0;
        GET_OBJ_POSTED(this) = nullptr;
        GET_OBJ_POSTTYPE(this) = 0;
    }
}

void Object::onLocationChanged(const Location &oldloc, const Location &newloc)
{
}

bool Object::isActiveInLocation() const {
    return isActive();
}

std::string Object::getLocationDisplayCategory(Character* viewer) const {
    return "Objects";
}

void Object::displayLocationInfo(Character* viewer) {
    show_obj_to_char(this, viewer, SHOW_OBJ_LONG);
}

void Object::clearLocation() {
    // this generally handles... well, whatever it needs to.
    if(location) {
        leaveLocation();
    } else if(auto c = getCarriedBy()) {
        c->removeFromInventory(shared_from_this());
    } else if(auto c = getWornBy()) {
        c->removeFromEquip(shared_from_this());
    } else if(auto o = getContainer()) {
        o->removeFromInventory(shared_from_this());
    }
}

std::shared_ptr<HasLocation> Object::getSharedHasLocation() {
    return shared_from_this();
}

void Object::commit_iedit(const ObjectPrototype &other)
{
    operator=(other);

    // Set the unique save flag
    item_flags.set(ITEM_UNIQUE_SAVE);
}

Object &Object::operator=(const ObjectPrototype &other)
{
    ObjectBase::operator=(static_cast<const ObjectBase&>(other));

    return *this;
}

vnum Object::getDgVnum() const {
    return getVnum();
}

UnitType Object::getDgUnitType() const {
    return UnitType::object;
}