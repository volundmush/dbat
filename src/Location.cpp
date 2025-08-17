#include <functional>
#include "dbat/structs.h"
#include "dbat/filter.h"
#include "dbat/utils.h"
#include "dbat/constants.h"
#include "dbat/fight.h"
#include "dbat/planet.h"
#include "dbat/act.informative.h"

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

Location::Location(const std::string& txt)
{
    *this = resolveLocID(txt);
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
    if(auto a = al.lock()) {
        return a->validCoordinates(position);
    }
    return false;
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
    if(auto a = al.lock()) {
        auto alid = a->getLocID();
        if(position.x != 0 || position.y != 0 || position.z != 0) {
            return fmt::format("{}:{}:{}:{}", alid, position.x, position.y, position.z);
        }
        return a->getLocID();
    }

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
    return !ex->exit_flags[EX_CLOSED];
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

constexpr int EXIT_ISDOOR = (1 << 0);    /* Exit is a door		*/
constexpr int EXIT_CLOSED = (1 << 1);    /* The door is closed	*/
constexpr int EXIT_LOCKED = (1 << 2);    /* The door is locked	*/
constexpr int EXIT_PICKPROOF = (1 << 3); /* Lock can't be picked	*/
constexpr int EXIT_SECRET = (1 << 4);    /* The door is hidden        */

void Destination::legacyExitFlags(int flags) {
    exit_flags.clear();
    exit_flags.set(ExitFlag::isdoor, flags & EXIT_ISDOOR);
    exit_flags.set(ExitFlag::closed, flags & EXIT_CLOSED);
    exit_flags.set(ExitFlag::locked, flags & EXIT_LOCKED);
    exit_flags.set(ExitFlag::pickproof, flags & EXIT_PICKPROOF);
    exit_flags.set(ExitFlag::secret, flags & EXIT_SECRET);
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

FlagHandler<RoomFlag>& Location::getRoomFlags()
{
    if (auto a = al.lock())
    {
        return a->getRoomFlags(position);
    }
    static FlagHandler<RoomFlag> standby;
    return standby;
}

FlagHandler<WhereFlag>& Location::getWhereFlags()
{
    if (auto a = al.lock())
    {
        return a->getWhereFlags(position);
    }
    static FlagHandler<WhereFlag> standby;
    return standby;
}


static void display_room_flags(Location& loc, Character *ch, const std::vector<Zone*>& zones)
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];

    sprintf(buf, "%s", loc.getRoomFlags().getFlagNames().c_str());
    sprinttype(static_cast<int>(loc.getSectorType()), sector_types, buf2, sizeof(buf2));

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("\r\n@wO----------------------------------------------------------------------O@n\r\n");
    }

    std::vector<std::string> zoneNames;
    for(const auto& zone : boost::adaptors::reverse(zones)) {
        zoneNames.push_back(fmt::format("{}@n", zone->name));
    }

    ch->sendFmt("@wRegion:@n {}@n\r\n", fmt::join(zoneNames, " -> "));

    ch->send_to("@wLocation: @G%-70s@w\r\n", loc.getName());

    if(auto r = dynamic_cast<Room*>(loc.al.lock().get()); r)
    {
        if (auto sc = r->getScripts(); !sc.empty())
        {
            ch->sendText("@D[@GTriggers");
            for (auto t : filter_shared(sc))
                ch->send_to(" %d", t->getVnum());
            ch->sendText("@D] ");
        }
    }

    double grav = loc.getEnvironment(ENV_GRAVITY);
    auto g = fmt::format("{}", grav);
    snprintf(buf3, sizeof(buf3), "@D[ @G%s@D] @wSector: @D[ @G%s @D] @wVnum: @D[@G%s@D]@n Gravity: @D[@G%sx@D]@n", buf, buf2, loc.getLocID().c_str(), g.c_str());
    ch->send_to("@wFlags: %-70s@w\r\n", buf3);

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("@wO----------------------------------------------------------------------O@n\r\n");
    }
}

static void display_special_room_descriptions(Location& loc, Character *ch)
{
    auto &rf = loc.getRoomFlags();

    if (rf.get(ROOM_REGEN))
    {
        ch->sendText("@CA feeling of calm and relaxation fills this room.@n\r\n");
    }
    if (rf.get(ROOM_AURA))
    {
        ch->sendText("@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
    }
    if (loc.getWhereFlag(WhereFlag::hyperbolic_time_chamber))
    {
        ch->sendText("@rThis room feels like it operates in a different time frame.@n\r\n");
    }
}

static void display_room_info(Location& loc, Character *ch, const std::vector<Zone*>& zones)
{
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("@wO----------------------------------------------------------------------O@n\r\n");
    }

    std::vector<std::string> zoneNames;
    for(const auto& zone : boost::adaptors::reverse(zones)) {
        zoneNames.push_back(fmt::format("{}@n", zone->name));
    }

    ch->sendFmt("@wRegion:@n {}@n\r\n", fmt::join(zoneNames, " -> "));
    ch->send_to("@wLocation: %-70s@n\r\n", loc.getName());

    double grav = loc.getEnvironment(ENV_GRAVITY);
    if (grav <= 1.0)
    {
        ch->sendText("@wGravity: @WNormal@n\r\n");
    }
    else
    {
        auto g = fmt::format("{}", grav);
        ch->send_to("@wGravity: @W%sx@n\r\n", g.c_str());
    }

    display_special_room_descriptions(loc, ch);

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("@wO----------------------------------------------------------------------O@n\r\n");
    }
}

static void display_damage_description(Character *ch, int dmg, const char *surface)
{
    if (dmg <= 2)
    {
        ch->send_to("@wA small hole with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 4)
    {
        ch->send_to("@wA couple small holes with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 6)
    {
        ch->send_to("@wA few small holes with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 10)
    {
        ch->send_to("@wThere are several small holes with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 20)
    {
        ch->send_to("@wMany holes fill the %s of this area, many of which have burn marks.@n", surface);
    }
    else if (dmg <= 30)
    {
        ch->send_to("@wThe %s is severely damaged with many large holes.@n", surface);
    }
    else if (dmg <= 50)
    {
        ch->sendText("@wBattle damage covers the entire area. Displayed as a tribute to the battles that have been waged here.@n");
    }
    else if (dmg <= 75)
    {
        ch->sendText("@wThis entire area is falling apart, it has been damaged so badly.@n");
    }
    else if (dmg <= 99)
    {
        ch->sendText("@wThis area cannot withstand much more damage. Everything has been damaged so badly it is hard to recognize any particular details about their former quality.@n");
    }
    else if (dmg >= 100)
    {
        ch->sendText("@wThis area is completely destroyed. Nothing is recognizable. Chunks of debris litter the ground, filling up holes, and overflowing onto what is left of the ground. A haze of smoke is wafting through the air, creating a chilling atmosphere.@n");
    }
}

static void display_damage_description_forest(Character *ch, int dmg)
{
    if (dmg <= 2)
    {
        ch->sendText("@wA small tree sits in a little crater here.@n");
    }
    else if (dmg <= 4)
    {
        ch->sendText("@wTrees have been uprooted by craters in the ground.@n");
    }
    else if (dmg <= 6)
    {
        ch->sendText("@wSeveral trees have been reduced to chunks of debris and are laying in a few craters here.@n");
    }
    else if (dmg <= 10)
    {
        ch->sendText("@wA large patch of trees have been destroyed and are laying in craters here.@n");
    }
    else if (dmg <= 20)
    {
        ch->sendText("@wSeveral craters have merged into one large crater in one part of this forest.@n");
    }
    else if (dmg <= 30)
    {
        ch->sendText("@wThe open sky can easily be seen through a hole of trees destroyed and resting at the bottom of several craters here.@n");
    }
    else if (dmg <= 50)
    {
        ch->sendText("@wA good deal of burning tree pieces can be found strewn across the cratered ground here.@n");
    }
    else if (dmg <= 75)
    {
        ch->sendText("@wVery few trees are left standing in this area, replaced instead by large craters.@n");
    }
    else if (dmg <= 99)
    {
        ch->sendText("@wSingle solitary trees can be found still standing here or there in the area. The rest have been almost completely obliterated in recent conflicts.@n");
    }
    else if (dmg >= 100)
    {
        ch->sendText("@wOne massive crater fills this area. This desolate crater leaves no evidence of what used to be found in the area. Smoke slowly wafts into the sky from the central point of the crater, creating an oppressive atmosphere.@n");
    }
}

static void display_damage_description_mountain(Character *ch, int dmg)
{
    if (dmg <= 2)
    {
        ch->sendText("@wA small crater has been burned into the side of this mountain.@n");
    }
    else if (dmg <= 4)
    {
        ch->sendText("@wA couple craters have been burned into the side of this mountain.@n");
    }
    else if (dmg <= 6)
    {
        ch->sendText("@wBurned bits of boulders can be seen lying at the bottom of a few nearby craters.@n");
    }
    else if (dmg <= 10)
    {
        ch->sendText("@wSeveral bad craters can be seen in the side of the mountain here.@n");
    }
    else if (dmg <= 20)
    {
        ch->sendText("@wLarge boulders have rolled down the mountainside and collected in many nearby craters.@n");
    }
    else if (dmg <= 30)
    {
        ch->sendText("@wMany craters are covering the mountainside here.@n");
    }
    else if (dmg <= 50)
    {
        ch->sendText("@wThe mountain side has partially collapsed, shedding rubble down towards its base.@n");
    }
    else if (dmg <= 75)
    {
        ch->sendText("@wA peak of the mountain has been blown off, leaving behind a smoldering tip.@n");
    }
    else if (dmg <= 99)
    {
        ch->sendText("@wThe mountainside here has completely collapsed, shedding dangerous rubble down to its base.@n");
    }
    else if (dmg >= 100)
    {
        ch->sendText("@wHalf the mountain has been blown away, leaving a scarred and jagged rock in its place. Billowing smoke wafts up from several parts of the mountain, filling the nearby skies and blotting out the sun.@n");
    }
}

static void display_room_damage_description(Location& loc, Character *ch)
{
    auto dmg = loc.getDamage();
    auto sect = static_cast<int>(loc.getSectorType());
    auto sunk = loc.getEnvironment(ENV_WATER) >= 100.0;

    if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || loc.getRoomFlag(ROOM_DEATH))
    {
        if (dmg <= 99 || (dmg == 100 && (sect == SECT_WATER_SWIM || sunk || sect == SECT_FLYING || sect == SECT_SHOP || sect == SECT_IMPORTANT)))
        {
            std::string ldesc(loc.getLookDescription());
            if(!ldesc.ends_with("\r\n")) ldesc += "\r\n";
            ch->send_to("@w%s@n", ldesc);
        }

        if (dmg > 0)
        {
            ch->sendText("\r\n");
            switch (sect)
            {
            case SECT_INSIDE:
                display_damage_description(ch, dmg, "floor");
                break;
            case SECT_CITY:
            case SECT_FIELD:
            case SECT_HILLS:
            case SECT_IMPORTANT:
                display_damage_description(ch, dmg, "ground");
                break;
            case SECT_FOREST:
                display_damage_description_forest(ch, dmg);
                break;
            case SECT_MOUNTAIN:
                display_damage_description_mountain(ch, dmg);
                break;
            default:
                break;
            }
            ch->sendText("\r\n");
        }

        auto ge = loc.getGroundEffect();

        if (ge >= 1 && ge <= 5)
        {
            ch->sendText("@rLava@w is pooling in some places here...@n\r\n");
        }
        else if (ge >= 6)
        {
            ch->sendText("@RLava@r covers pretty much the entire area!@n\r\n");
        }
        else if (ge < 0)
        {
            ch->sendText("@cThe entire area is flooded with a @Cmystical@c cube of @Bwater!@n\r\n");
        }
    }
}

static void display_garden_info(Location& loc, Character *ch)
{
    auto con = loc.getObjects();
    auto &rf = loc.getRoomFlags();
    if (rf.get(ROOM_GARDEN1))
    {
        ch->send_to("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", con.size());
    }
    else if (rf.get(ROOM_GARDEN2))
    {
        ch->send_to("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n", con.size());
    }
    else if (rf.get(ROOM_HOUSE))
    {
        ch->send_to("@D[@GItems Stored@D: @g%d@D]@n\r\n", con.size());
    }
}

void Location::displayLookFor(Character* ch) {
    if (!ch->desc)
        return;
    
    auto a = al.lock();
    if(!a) {
        ch->sendText("You can't see anything here.");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_BLIND))
    {
        ch->sendText("You see nothing but infinite darkness...\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_EYEC))
    {
        ch->sendText("You can't see a damned thing, your eyes are closed!\r\n");
        return;
    }

    auto z = getZone();
    auto zones = z->getChain();

    if (PRF_FLAGGED(ch, PRF_ROOMFLAGS))
    {
        display_room_flags(*this, ch, zones);
    }
    else
    {
        display_room_info(*this, ch, zones);
    }

    display_room_damage_description(*this, ch);

    /* autoexits */
    if (PRF_FLAGGED(ch, PRF_NODEC))
    {
        do_auto_exits2(*this, ch);
    }
    else
    {
        do_auto_exits(*this, ch, EXIT_LEV(ch));
    }

    auto validSpots = [](const auto& locs) {
        std::unordered_map<std::string, Location> valid;
        for(const auto& [key, locid] : locs) {
            if(auto l = resolveLocID(locid); l) {
                valid[key] = l;
            }
        }
        return valid;
    };

    auto locid = getLocID();
    auto root_zone = zones.back();
    if(root_zone->launchDestination == locid) {
        // we are in the orbit of a place.
        ch->sendFmt("@wYou are in low orbit above {}@n\r\n", root_zone->name);
        if(ch->location.getLocID().starts_with("S")) {
            // viewer is in a spaceship. We'll display docking spots.
            auto docking_spots = validSpots(root_zone->dockingSpots);
            ch->sendText("@wDocking Spots (for pilot land):@n\r\n");
            for(const auto& [key, loc] : docking_spots) {
                ch->sendFmt("{}\r\n", key);
            }
        } else {
            // viewer is not in a spaceship. We'll display landing spots.
            auto landing_spots = validSpots(root_zone->landingSpots);
            ch->sendText("@wLanding Spots (for land):@n\r\n");
            for(const auto& [key, loc] : landing_spots) {
                ch->sendFmt("{}\r\n", key);
            }
    }
}

    display_garden_info(*this, ch);

    // retrieve all entities at these coordinates.
    auto con = a->getContents<HasLocation>(position);
    auto sh = ch->shared_from_this();
    std::map<std::string, std::vector<std::weak_ptr<HasLocation>>> categorized_contents;
    for (const auto& hl : filter_shared(con)) {
        // Filter by visibility...
        if((hl != sh) && hl->getLocationVisibleTo(ch))
            categorized_contents[hl->getLocationDisplayCategory(ch)].emplace_back(hl);
    }

    for(auto& [cat, entities] : categorized_contents) {
        // it's impossible for entities to be empty here...
        ch->sendFmt("    {}:\r\n", cat);
        for(auto ent : filter_raw(entities)) {
            ent->displayLocationInfo(ch);
            ch->sendText("@n");
        }
    }
}

bool Location::buildwalk(Character* ch, Direction dir) {
    if(auto a = al.lock()) {
        return a->buildwalk(position, ch, dir);
    }
    return false;
}