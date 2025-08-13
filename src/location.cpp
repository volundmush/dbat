#include <functional>
#include "dbat/structs.h"
#include "dbat/filter.h"
#include "dbat/utils.h"
#include "dbat/constants.h"
#include "dbat/fight.h"

bool Coordinates::operator==(const Coordinates &other) const
{
    return x == other.x && y == other.y && z == other.z;
}

void Coordinates::apply(Direction dir)
{
    switch (dir)
    {
    case Direction::north:
        y += 1;
        break;
    case Direction::east:
        x += 1;
        break;
    case Direction::south:
        y -= 1;
        break;
    case Direction::west:
        x -= 1;
        break;
    case Direction::up:
        z += 1;
        break;
    case Direction::down:
        z -= 1;
        break;
    case Direction::northeast:
        x += 1;
        y += 1;
        break;
    case Direction::southeast:
        x += 1;
        y -= 1;
        break;
    case Direction::southwest:
        x -= 1;
        y -= 1;
        break;
    case Direction::northwest:
        x -= 1;
        y += 1;
        break;
    case Direction::inside:
    case Direction::outside:
        // No movement for inside/outside
        break;
    }
}

Location::Location(room_vnum rv)
{
    unit = get_room(rv);
    position = Coordinates{0, 0, 0};
}

Location::Location(Room *room)
{
    unit = room;
    position = Coordinates{0, 0, 0};
}

Location::Location(Character *ch)
{
    *this = ch->location;
}

bool Location::operator==(const Location &other) const
{
    return unit == other.unit && position == other.position;
}

bool Location::operator==(const Room *room) const
{
    return room == unit;
}

bool Location::operator==(const room_vnum rv) const
{
    if (auto r = dynamic_cast<Room *>(unit); r)
    {
        return r->getVnum() == rv;
    }
    return false;
}

// the bool operator for ...
Location::operator bool() const
{
    return unit != nullptr;
}

vnum Location::getVnum() const
{
    if (unit)
    {
        return unit->getLocVnum();
    }
    return NOWHERE;
}

Zone *Location::getZone() const
{
    if (unit)
    {
        return unit->getZone();
    }
    return nullptr;
}

std::string Location::getLocID() const
{
    if (auto r = dynamic_cast<Room *>(unit); r)
    {
        return fmt::format("{}:{}", "R", r->getVnum());
    }

    return "";
}

const char *Location::getName() const
{
    if (unit)
    {
        return unit->getName(position);
    }
    return "Unknown";
}

const char *Location::getLookDescription() const
{
    if (unit)
    {
        return unit->getLookDescription(position);
    }
    return "";
}

std::optional<Destination> Location::getExit(Direction dir) const
{
    if (unit)
        return unit->getDirection(position, dir);
    return std::nullopt;
}

std::map<Direction, Destination> Location::getExits() const
{
    if (unit)
    {
        return unit->getDirections(position);
    }
    return {};
}

const std::vector<ExtraDescription> &Location::getExtraDescription() const
{
    if (unit)
    {
        return unit->getExtraDescription(position);
    }
    static std::vector<ExtraDescription> empty;
    return empty;
}

double Location::getEnvironment(int type) const
{
    if (unit)
    {
        return unit->getEnvironment(position, type);
    }
    return 0.0;
}

double Location::setEnvironment(int type, double value)
{
    if (unit)
    {
        return unit->setEnvironment(position, type, value);
    }
    return 0.0;
}

double Location::modEnvironment(int type, double value)
{
    if (unit)
    {
        return unit->modEnvironment(position, type, value);
    }
    return 0.0;
}

void Location::clearEnvironment(int type)
{
    if (unit)
    {
        unit->clearEnvironment(position, type);
    }
}

void Location::setRoomFlag(int flag, bool value) const
{
    if (unit)
    {
        unit->setRoomFlag(position, flag, value);
    }
}

void Location::setRoomFlag(RoomFlag flag, bool value) const
{
    if (unit)
    {
        unit->setRoomFlag(position, flag, value);
    }
}

bool Location::toggleRoomFlag(int flag) const
{
    if (unit)
    {
        return unit->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::toggleRoomFlag(RoomFlag flag) const
{
    if (unit)
    {
        return unit->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(int flag) const
{
    if (unit)
    {
        return unit->getRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(RoomFlag flag) const
{
    if (unit)
    {
        return unit->getRoomFlag(position, flag);
    }
    return false;
}

void Location::setWhereFlag(WhereFlag flag, bool value) const
{
    if (unit)
    {
        unit->setWhereFlag(position, flag, value);
    }
}

bool Location::toggleWhereFlag(WhereFlag flag) const
{
    if (unit)
    {
        return unit->toggleWhereFlag(position, flag);
    }
    return false;
}

bool Location::getWhereFlag(WhereFlag flag) const
{
    if (unit)
    {
        return unit->getWhereFlag(position, flag);
    }
    return false;
}

std::string Location::getUID(bool active) const
{
    if (auto r = dynamic_cast<Room *>(unit); r)
    {
        return r->getUID(active);
    }
    return "";
}

void Location::sendText(const std::string &message) const
{
    if (unit)
    {
        unit->broadcastAt(position, message);
    }
}

std::vector<std::weak_ptr<Object>> Location::getObjects() const
{
    if (unit)
    {
        return unit->getObjects(position);
    }
    return {};
}

std::vector<std::weak_ptr<Character>> Location::getPeople() const
{
    if (unit)
    {
        return unit->getPeople(position);
    }
    return {};
}

int Location::getDamage() const
{
    if (unit)
    {
        return unit->getDamage(position);
    }
    return 0;
}

void Location::setDamage(int value)
{
    if (unit)
    {
        unit->setDamage(position, value);
    }
}

int Location::modDamage(int value)
{
    if (unit)
    {
        return unit->modDamage(position, value);
    }
    return 0;
}

SectorType Location::getSectorType() const
{
    if (unit)
    {
        return unit->getSectorType(position);
    }
    return SectorType::inside;
}

int Location::getTileType() const
{
    return static_cast<int>(getSectorType());
}

int Location::getGroundEffect() const
{
    if (unit)
    {
        return unit->getGroundEffect(position);
    }
    return 0;
}

void Location::setGroundEffect(int value)
{
    if (unit)
    {
        unit->setGroundEffect(position, value);
    }
}

int Location::modGroundEffect(int value)
{
    if (unit)
    {
        return unit->modGroundEffect(position, value);
    }
    return 0;
}

SpecialFunc Location::getSpecialFunc() const
{
    if (unit)
    {
        return unit->getSpecialFunc(position);
    }
    return nullptr;
}

bool Location::getIsDark() const
{
    if (unit)
    {
        return unit->getIsDark(position);
    }
    return false;
}

int Location::countPlayers() const
{
    int total = 0;
    auto people = getPeople();
    for (const auto &ch : filter_raw(people))
    {
        if (!IS_NPC(ch))
            total++;
    }
    return total;
}

bool Location::canGo(int dir) const
{
    auto ex = getExit(static_cast<Direction>(dir));
    if (!ex)
        return false;
    return !IS_SET(ex->exit_info, EX_CLOSED);
}

int Location::getCookElement() const
{
    if (unit)
    {
        return unit->getCookElement(position);
    }
    return 0;
}

// Hash function implementations
namespace std
{
    std::size_t hash<Coordinates>::operator()(const Coordinates &coord) const noexcept
    {
        // Combine hash of x, y, and z coordinates
        std::size_t h1 = std::hash<int32_t>{}(coord.x);
        std::size_t h2 = std::hash<int32_t>{}(coord.y);
        std::size_t h3 = std::hash<int32_t>{}(coord.z);

        // Use bit shifting and XOR to combine hashes
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }

    std::size_t hash<Location>::operator()(const Location &loc) const noexcept
    {
        // Combine hash of unit pointer and position
        std::size_t h1 = std::hash<void *>{}(loc.unit);
        std::size_t h2 = std::hash<Coordinates>{}(loc.position);

        // Use bit shifting and XOR to combine hashes
        return h1 ^ (h2 << 1);
    }
}

std::vector<std::weak_ptr<Object>> AbstractLocation::getObjects() const
{
    std::vector<std::weak_ptr<Object>> out;
    for (const auto &obj : filter_shared(objects))
    {
        out.push_back(obj);
    }
    return out;
}

std::vector<std::weak_ptr<Character>> AbstractLocation::getPeople() const
{
    std::vector<std::weak_ptr<Character>> out;
    for (const auto &chara : filter_shared(people))
    {
        out.push_back(chara);
    }
    return out;
}

std::vector<std::weak_ptr<Object>> AbstractLocation::getObjects(const Coordinates &coor) const
{
    std::vector<std::weak_ptr<Object>> out;
    for (const auto &obj : filter_shared(objects))
    {
        if (obj->location.position == coor)
            out.push_back(obj);
    }
    return out;
}

std::vector<std::weak_ptr<Character>> AbstractLocation::getPeople(const Coordinates &coor) const
{
    std::vector<std::weak_ptr<Character>> out;
    for (const auto &chara : filter_shared(people))
    {
        if (chara->location.position == coor)
            out.push_back(chara);
    }
    return out;
}

void Location::traverseObjects(const std::function<void(Object *)> &func, bool recurse)
{
    for (const auto &obj : filter_raw(getObjects()))
    {
        func(obj);
        if (recurse)
        {
            obj->traverseInventory(func);
        }
    }
}

std::unordered_set<Object *> Location::gatherFromObjects(const std::function<bool(Object *)> &func, bool recurse)
{
    std::unordered_set<Object *> found;
    for (const auto &obj : filter_raw(getObjects()))
    {
        if (func(obj))
        {
            found.insert(obj);
        }
        if (recurse)
        {
            auto subFound = obj->gatherFromInventory(func);
            found.insert(subFound.begin(), subFound.end());
        }
    }
    return found;
}

Object *Location::searchObjects(const std::function<bool(Object *)> &func, bool recurse)
{
    for (const auto &obj : filter_raw(getObjects()))
    {
        if (func(obj))
        {
            return obj;
        }
        if (recurse)
        {
            auto found = obj->searchInventory(func);
            if (found)
                return found;
        }
    }
    return nullptr;
}

Object *Location::searchObjects(obj_vnum vnum, bool working, bool recurse)
{
    for (const auto &obj : filter_raw(getObjects()))
    {
        if (obj->getVnum() == vnum && (!working || obj->isWorking()))
        {
            return obj;
        }
        if (recurse)
        {
            auto found = obj->searchInventory(vnum, working);
            if (found)
                return found;
        }
    }
    return nullptr;
}

std::optional<Destination> Destination::getReverse() const
{
    return getExit(static_cast<Direction>(rev_dir[static_cast<int>(dir)]));
}

void AbstractLocation::setRoomFlag(const Coordinates &coor, int flag, bool value)
{
    setRoomFlag(coor, static_cast<RoomFlag>(flag), value);
}

bool AbstractLocation::toggleRoomFlag(const Coordinates &coor, int flag)
{
    return toggleRoomFlag(coor, static_cast<RoomFlag>(flag));
}

bool AbstractLocation::getRoomFlag(const Coordinates &coor, int flag) const
{
    return getRoomFlag(coor, static_cast<RoomFlag>(flag));
}

int AbstractLocation::getCookElement(const Coordinates &coor) const
{
    int found = 0;
    auto con = getObjects(coor);
    for (auto obj : filter_raw(con))
    {
        if (GET_OBJ_TYPE(obj) == ITEM_CAMPFIRE)
        {
            found = 1;
        }
        else if (obj->getVnum() == 19093)
            return 2;
    }

    return found;
}

const std::vector<ExtraDescription> &AbstractLocation::getExtraDescription(const Coordinates &coor) const
{
    static std::vector<ExtraDescription> extraDescriptions;
    return extraDescriptions;
}

bool AbstractLocation::getIsDark(const Coordinates &coor) const
{
    return false; // temporarily disabled.

    auto pe = getPeople(coor);
    for (auto c : filter_raw(pe))
    {
        if (c->isProvidingLight())
            return false;
    }

    if (getCookElement(coor))
        return false;

    if (getRoomFlag(coor, ROOM_NOINSTANT) && getRoomFlag(coor, ROOM_DARK))
    {
        return true;
    }
    if (getRoomFlag(coor, ROOM_NOINSTANT) && !getRoomFlag(coor, ROOM_DARK))
    {
        return false;
    }

    if (getRoomFlag(coor, ROOM_DARK))
        return true;

    if (getRoomFlag(coor, ROOM_INDOORS))
        return false;

    const auto tile = static_cast<int>(getSectorType(coor));

    if (tile == SECT_INSIDE || tile == SECT_CITY || tile == SECT_IMPORTANT || tile == SECT_SHOP)
        return false;

    if (tile == SECT_SPACE)
        return false;

    if (weather_info.sunlight == SUN_SET)
        return true;

    if (weather_info.sunlight == SUN_DARK)
        return true;

    return false;
}

void Location::deleteExit(Direction dir)
{
    if (auto l = dynamic_cast<AbstractLocation *>(unit))
    {
        l->deleteExit(position, dir);
    }
}

void Location::replaceExit(const Destination &dest)
{
    if (auto l = dynamic_cast<AbstractLocation *>(unit))
    {
        l->replaceExit(position, dest);
    }
}

void Location::executeResetCommands(const std::vector<ResetCommand> &cmds)
{
    SpawnRegistry reg;
    bool success = false;
    for (const auto &cmd : cmds)
    {
        if (cmd.if_flag && !success)
            continue;
        success = cmd.executeAt(*this, reg);
    }
}

void AbstractLocation::replaceExit(const Coordinates &coor, const Destination &dest)
{
    // Implementation for replacing an exit in the location data
}

void AbstractLocation::deleteExit(const Coordinates &coor, Direction dir)
{
    // Implementation for deleting an exit in the location data
}

void AbstractLocation::addToContents(const Coordinates &coor, Character *ch)
{
    addToContents(coor, ch->shared());
}

void AbstractLocation::addToContents(const Coordinates &coor, const std::shared_ptr<Character> &ch)
{

    people.push_front(ch);
    ch->location.unit = this;
    ch->location.position = coor;
    auto z = getZone();

    if (IS_NPC(ch))
    {
        z->npcsInZone.push_back(ch);
    }
    else
    {
        z->playersInZone.push_back(ch);
        if (ch->pref_flags.get(PRF_ARENAWATCH))
        {
            ch->pref_flags.set(PRF_ARENAWATCH, false);
            ch->setBaseStat<room_vnum>("arena_watch", -1);
        }
    }

    /* Stop fighting now, if we left. */
    if (FIGHTING(ch) && ch->location != FIGHTING(ch)->location && !ch->affect_flags.get(AFF_PURSUIT))
    {
        stop_fighting(FIGHTING(ch));
        stop_fighting(ch.get());
    }
}

void AbstractLocation::addToContents(const Coordinates &coor, Object *obj)
{
    addToContents(coor, obj->shared());
}

void AbstractLocation::addToContents(const Coordinates &coor, const std::shared_ptr<Object> &obj)
{

    objects.push_front(obj);
    obj->location.unit = this;
    obj->location.position = coor;
    obj->setBaseStat("lload", time(nullptr));

    auto z = getZone();
    z->objectsInZone.push_back(obj);

    onAddToContents(coor, obj);

    /* // Putting this here for safekeeping.

    if (auto ex = EXIT(object, 5); ex &&
        (object->location.getTileType() == SECT_UNDERWATER || object->location.getTileType() == SECT_WATER_NOSWIM)) {
        act("$p @Bsinks to deeper waters.@n", true, nullptr, object, nullptr, TO_ROOM);
        object->clearLocation();
        object->setLocation(*ex);
    }
    if (auto ex = EXIT(object, 5); ex && object->location.getTileType() == SECT_FLYING &&
        (GET_OBJ_VNUM(object) < 80 || GET_OBJ_VNUM(object) > 83)) {
        act("$p @Cfalls down.@n", true, nullptr, object, nullptr, TO_ROOM);
        object->clearLocation();
        object->setLocation(*ex);
        if (object->location.getTileType() != SECT_FLYING) {
            act("$p @Cfalls down and smacks the ground.@n", true, nullptr, object, nullptr, TO_ROOM);
        }
    }

    */
}

void AbstractLocation::removeFromContents(Character *ch)
{
    removeFromContents(ch->shared());
}

void AbstractLocation::removeFromContents(const std::shared_ptr<Character> &ch)
{
    // Implementation for removing a character from the location
    if (!ch)
    {
        basic_mud_log("SYSERR: nullptr character passed to AbstractLocation::removeFrom.");
        return;
    }

    if (ch->location.unit != this)
    {
        basic_mud_log("SYSERR: character not present when passed to AbstractLocation::removeFrom.");
        return;
    }

    if (FIGHTING(ch) && !ch->affect_flags.get(AFF_PURSUIT))
        stop_fighting(ch.get());

    if (ch->affect_flags.get(AFF_PURSUIT) && FIGHTING(ch) == nullptr)
        ch->affect_flags.set(AFF_PURSUIT, false);

    auto sh = ch->shared();
    auto z = getZone();
    if (IS_NPC(ch))
    {
        z->npcsInZone.remove_if([sh](auto &npc)
                                { return npc.expired() || npc.lock() == sh; });
    }
    else
    {
        z->playersInZone.remove_if([sh](auto &npc)
                                   { return npc.expired() || npc.lock() == sh; });
    }

    people.remove_if([sh](auto &c)
                     { return c.expired() || c.lock() == sh; });
    ch->location = {};

    onRemoveFromContents(ch);
}

void AbstractLocation::removeFromContents(Object *obj)
{
    removeFromContents(obj->shared());
}

void AbstractLocation::removeFromContents(const std::shared_ptr<Object> &obj)
{
    if (!obj)
    {
        basic_mud_log("SYSERR: nullptr object passed to AbstractLocation::removeFrom.");
        return;
    }

    if (obj->location.unit != this)
    {
        basic_mud_log("SYSERR: object not in a room passed to AbstractLocation::removeFrom.");
        return;
    }

    if (obj->type_flag == ItemType::plant)
        objectSubscriptions.unsubscribe("growingPlants", obj);

    if (auto o = GET_OBJ_POSTED(obj); o)
    {
        if (GET_OBJ_POSTTYPE(obj) <= 0)
        {
            o->location.send_to("%s@W shakes loose from %s@W.@n\r\n", o->getShortDescription(),
                                obj->getShortDescription());
        }
        else
        {
            o->location.send_to("%s@W comes loose from %s@W.@n\r\n", obj->getShortDescription(),
                                o->getShortDescription());
        }
        GET_OBJ_POSTED(o) = nullptr;
        GET_OBJ_POSTTYPE(o) = 0;
        GET_OBJ_POSTED(obj) = nullptr;
        GET_OBJ_POSTTYPE(obj) = 0;
    }

    auto z = getZone();
    z->objectsInZone.remove_if([obj](auto &o)
                               { return o.expired() || o.lock() == obj; });
    objects.remove_if([obj](auto &o)
                      { return o.expired() || o.lock() == obj; });

    obj->location = {};
    onRemoveFromContents(obj);
}