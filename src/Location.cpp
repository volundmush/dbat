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
    al = get_room(rv)->shared_from_this();
    position = Coordinates{0, 0, 0};
}

Location::Location(Room *room) : Location(room->shared_from_this())
{
    
}

Location::Location(const std::shared_ptr<Room>& room)
{
    al = room;
    position = Coordinates{0, 0, 0};
}

Location::Location(Character *ch)
{
    *this = ch->location;
}

bool Location::operator==(const Location &other) const
{
    if(al.expired() || other.al.expired())
    {
        return false; // If either location is invalid, they are not equal
    }
    return al.lock() == other.al.lock() && position == other.position;
}

bool Location::operator==(const Room *room) const
{
    if(!room || al.expired())
    {
        return false; // If room is null or location is invalid, they are not equal
    }
    return room == al.lock().get();
}

bool Location::operator==(const std::shared_ptr<Room>& room) const
{
    return room == al.lock();
}

bool Location::operator==(const room_vnum rv) const
{
    if(auto a = al.lock())
    if (auto r = std::dynamic_pointer_cast<Room>(a); r)
    {
        return r->getVnum() == rv;
    }
    return false;
}

// the bool operator for ...
Location::operator bool() const
{
    return !al.expired();
}

vnum Location::getVnum() const
{
    if (auto a = al.lock())
    {
        return a->getLocVnum();
    }
    return NOWHERE;
}

Zone *Location::getZone() const
{
    if (auto a = al.lock())
    {
        return a->getLocZone();
    }
    return nullptr;
}

std::string Location::getLocID() const
{
    if(auto a = al.lock()) return a->getLocID();

    return "";
}

const char *Location::getName() const
{
    if (auto a = al.lock())
    {
        return a->getName(position);
    }
    return "Unknown";
}

const char *Location::getLookDescription() const
{
    if (auto a = al.lock())
    {
        return a->getLookDescription(position);
    }
    return "";
}

std::optional<Destination> Location::getExit(Direction dir) const
{
    if (auto a = al.lock())
        return a->getDirection(position, dir);
    return std::nullopt;
}

std::map<Direction, Destination> Location::getExits() const
{
    if (auto a = al.lock())
    {
        return a->getDirections(position);
    }
    return {};
}

const std::vector<ExtraDescription> &Location::getExtraDescription() const
{
    if (auto a = al.lock())
    {
        return a->getExtraDescription(position);
    }
    static std::vector<ExtraDescription> empty;
    return empty;
}

double Location::getEnvironment(int type) const
{
    if (auto a = al.lock())
    {
        return a->getEnvironment(position, type);
    }
    return 0.0;
}

double Location::setEnvironment(int type, double value)
{
    if (auto a = al.lock())
    {
        return a->setEnvironment(position, type, value);
    }
    return 0.0;
}

double Location::modEnvironment(int type, double value)
{
    if (auto a = al.lock())
    {
        return a->modEnvironment(position, type, value);
    }
    return 0.0;
}

void Location::clearEnvironment(int type)
{
    if (auto a = al.lock())
    {
        a->clearEnvironment(position, type);
    }
}

void Location::setRoomFlag(int flag, bool value) const
{
    if (auto a = al.lock())
    {
        a->setRoomFlag(position, flag, value);
    }
}

void Location::setRoomFlag(RoomFlag flag, bool value) const
{
    if (auto a = al.lock())
    {
        a->setRoomFlag(position, flag, value);
    }
}

bool Location::toggleRoomFlag(int flag) const
{
    if (auto a = al.lock())
    {
        return a->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::toggleRoomFlag(RoomFlag flag) const
{
    if (auto a = al.lock())
    {
        return a->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(int flag) const
{
    if (auto a = al.lock())
    {
        return a->getRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(RoomFlag flag) const
{
    if (auto a = al.lock())
    {
        return a->getRoomFlag(position, flag);
    }
    return false;
}

void Location::setWhereFlag(WhereFlag flag, bool value) const
{
    if (auto a = al.lock())
    {
        a->setWhereFlag(position, flag, value);
    }
}

bool Location::toggleWhereFlag(WhereFlag flag) const
{
    if (auto a = al.lock())
    {
        return a->toggleWhereFlag(position, flag);
    }
    return false;
}

bool Location::getWhereFlag(WhereFlag flag) const
{
    if (auto a = al.lock())
    {
        return a->getWhereFlag(position, flag);
    }
    return false;
}

std::string Location::getUID(bool active) const
{
    if(auto a = al.lock())
        if (auto r = std::dynamic_pointer_cast<Room>(a); r)
        {
            return r->getUID(active);
        }
    return "";
}

void Location::sendText(const std::string &message) const
{
    if (auto a = al.lock())
    {
        a->broadcastAt(position, message);
    }
}

std::vector<std::weak_ptr<Object>> Location::getObjects() const
{
    if (auto a = al.lock())
    {
        return a->getObjects(position);
    }
    return {};
}

std::vector<std::weak_ptr<Character>> Location::getPeople() const
{
    if (auto a = al.lock())
    {
        return a->getPeople(position);
    }
    return {};
}

int Location::getDamage() const
{
    if (auto a = al.lock())
    {
        return a->getDamage(position);
    }
    return 0;
}

void Location::setDamage(int value)
{
    if (auto a = al.lock())
    {
        a->setDamage(position, value);
    }
}

int Location::modDamage(int value)
{
    if (auto a = al.lock())
    {
        return a->modDamage(position, value);
    }
    return 0;
}

SectorType Location::getSectorType() const
{
    if (auto a = al.lock())
    {
        return a->getSectorType(position);
    }
    return SectorType::inside;
}

int Location::getTileType() const
{
    return static_cast<int>(getSectorType());
}

int Location::getGroundEffect() const
{
    if (auto a = al.lock())
    {
        return a->getGroundEffect(position);
    }
    return 0;
}

void Location::setGroundEffect(int value)
{
    if (auto a = al.lock())
    {
        a->setGroundEffect(position, value);
    }
}

int Location::modGroundEffect(int value)
{
    if (auto a = al.lock())
    {
        return a->modGroundEffect(position, value);
    }
    return 0;
}

SpecialFunc Location::getSpecialFunc() const
{
    if (auto a = al.lock())
    {
        return a->getSpecialFunc(position);
    }
    return nullptr;
}

bool Location::getIsDark()
{
    if (auto a = al.lock())
    {
        return a->getIsDark(position);
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

int Location::getCookElement()
{
    if (auto a = al.lock())
    {
        return a->getCookElement(position);
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
        std::size_t h1 = std::hash<void *>{}(loc.al.lock().get());
        std::size_t h2 = std::hash<Coordinates>{}(loc.position);

        // Use bit shifting and XOR to combine hashes
        return h1 ^ (h2 << 1);
    }
}



void Location::traverseObjects(const std::function<void(Object *)> &func, bool recurse)
{
    auto objs = getObjects();
    for (const auto &obj : filter_raw(objs))
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
    auto objs = getObjects();
    for (const auto &obj : filter_raw(objs))
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
    auto objs = getObjects();
    for (const auto &obj : filter_raw(objs))
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
    auto objs = getObjects();
    for (const auto &obj : filter_raw(objs))
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



void Location::deleteExit(Direction dir)
{
    if(auto a = al.lock())
        a->deleteExit(position, dir);
}

void Location::replaceExit(const Destination &dest)
{
    if (auto a = al.lock())
    {
        a->replaceExit(position, dest);
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

void Location::displayLookFor(Character* viewer) {
    if(auto a = al.lock()) {
        a->displayLookFor(position, viewer);
    }
}