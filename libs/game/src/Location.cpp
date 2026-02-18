#include <regex>
#include "dbat/game/Location.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/Area.hpp"
#include "dbat/game/Structure.hpp"
#include "dbat/game/Destination.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "volcano/util/FilterWeak.hpp"
#include "dbat/game/utils.hpp"
#include "dbat/game/constants.hpp"
//#include "dbat/game/fight.hpp"
#include "dbat/game/planet.hpp"
#include "dbat/game/interpreter.hpp"
#include "dbat/game/act.informative.hpp"
#include "dbat/game/const/AutoExit.hpp"
#include "dbat/game/const/Environment.hpp"

Location::Location(room_vnum rv)
{
    al = get_room(rv)->shared_from_this();
    position = Coordinates{0, 0, 0};
    locationID = getLocID();
}

Location::Location(Room *room) : Location()
{
    if(room) {
        al = room->shared_from_this();
        position = Coordinates{0, 0, 0};
        locationID = getLocID();
    } else {
        al.reset();
        locationID.clear();
    }
}

Location::Location(const std::shared_ptr<AbstractLocation>& al, const Coordinates& pos)
{
    this->al = al;
    position = pos;
    locationID = getLocID();
}

Location::Location(const std::shared_ptr<Room>& room)
{
    al = room;
    position = Coordinates{0, 0, 0};
    locationID = getLocID();
}

Location::Location(Character *ch)
{
    *this = ch->location;
}

static std::regex lid_regex(R"(^(R|A|S):(\d+)(:(-?\d+):(-?\d+):(-?\d+))?)", std::regex::icase);


Location::Location(const std::string& lid)
{
    std::smatch match;
    auto trimmed = boost::trim_copy(lid);
    boost::to_upper(trimmed);

    if(is_number(trimmed.c_str()))
        if(auto i = atoi(trimmed.c_str()); i != -1) {
            // we were given a plain number. this could be a room.
            if(auto find = Room::registry.find(i); find != Room::registry.end()) {
                al = find->second;
                locationID = getLocID();
                return;
            }
        }

    if(!std::regex_search(trimmed, match, lid_regex)) {
        return;
    }

    std::string letter = match[1].str();// First capture group
    int64_t id = std::stoll(match[2].str()); // Second capture group
    std::string xStr = match[4].str();
    std::string yStr = match[5].str();
    std::string zStr = match[6].str();

    if(!xStr.empty()) position.x = atoi(xStr.c_str());
    if(!yStr.empty()) position.y = atoi(yStr.c_str());
    if(!zStr.empty()) position.z = atoi(zStr.c_str());

    if(letter == "R") {
        // Room
        if(auto find = Room::registry.find(id); find != Room::registry.end()) {
            al = find->second;
            locationID = getLocID();
            return;
        }
    } else if(letter == "A") {
        // Area.
        if(auto find = areas.find(id); find != areas.end()) {
            al = find->second;
            locationID = getLocID();
            return;
        }
    } else if(letter == "S") {
        // Structure
        if(auto find = Structure::registry.find(id); find != Structure::registry.end()) {
            al = find->second;
            locationID = getLocID();
            return;
        }
    }
}

Location& Location::operator=(room_vnum rv)
{
    if(auto r = get_room(rv)) {
        al = r->shared_from_this();
        position = Coordinates{0, 0, 0};
        locationID = getLocID();
    } else {
        al.reset();
    }
    return *this;
}

Location& Location::operator=(Room* room) {
    if(room) {
        al = room->shared_from_this();
        position = Coordinates{0, 0, 0};
        locationID = getLocID();
    } else {
        al.reset();
    }
    return *this;
}

Location& Location::operator=(const std::shared_ptr<Room>& room) {
    if(room) {
        al = room;
        position = Coordinates{0, 0, 0};
        locationID = getLocID();
    } else {
        al.reset();
    }
    return *this;
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

AbstractLocation* Location::getLoc() const {
    if(auto a = al.lock()) {
        return a.get();
    }

    if(!locationID.empty())
        if(auto res = Location(locationID)) {
            al = res.al;
            position = res.position;
            locationID = res.locationID;
            if(auto a = al.lock()) {
                return a.get();
            }
        }

    return nullptr;
}

// the bool operator for ...
Location::operator bool() const
{
    if(auto a = getLoc()) {
        return a->validCoordinates(position);
    }
    return false;
}

vnum Location::getVnum() const
{
    if (auto a = getLoc())
    {
        return a->getLocVnum();
    }
    return NOWHERE;
}

Zone *Location::getZone() const
{
    if (auto a = getLoc())
    {
        return a->getLocZone();
    }
    return nullptr;
}

std::string Location::getLocID() const
{
    if(auto a = getLoc()) {
        auto alid = a->getLocID();
        if(position) {
            return fmt::format("{}:{}", alid, position);
        }
        return alid;
    }

    return "";
}

const char *Location::getName() const
{
    if (auto a = getLoc())
    {
        return a->getName(position);
    }
    return "Unknown";
}

const char *Location::getLookDescription() const
{
    if (auto a = getLoc())
    {
        return a->getLookDescription(position);
    }
    return "";
}

std::optional<Destination> Location::getExit(Direction dir) const
{
    if (auto a = getLoc())
        return a->getDirection(position, dir);
    return std::nullopt;
}

std::map<Direction, Destination> Location::getExits() const
{
    if (auto a = getLoc())
    {
        return a->getDirections(position);
    }
    return {};
}

ExtraDescriptionViews Location::getExtraDescription() const
{
    if (auto a = getLoc())
    {
        return a->getExtraDescription(position);
    }
    static ExtraDescriptionViews empty;
    return empty;
}

double Location::getEnvironment(int type) const
{
    if (auto a = getLoc())
    {
        auto res = a->getEnvironment(position, type);
        if(res) {
            return res.value();
        }
        auto z = getZone();
        return z->getEnvironment(type, true);
    }
    return 0.0;
}

void Location::setRoomFlag(int flag, bool value)
{
    if (auto a = getLoc())
    {
        a->setRoomFlag(position, flag, value);
    }
}

void Location::setRoomFlag(RoomFlag flag, bool value)
{
    if (auto a = getLoc())
    {
        a->setRoomFlag(position, flag, value);
    }
}

bool Location::toggleRoomFlag(int flag)
{
    if (auto a = getLoc())
    {
        return a->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::toggleRoomFlag(RoomFlag flag)
{
    if (auto a = getLoc())
    {
        return a->toggleRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(int flag) const
{
    if (auto a = getLoc())
    {
        return a->getRoomFlag(position, flag);
    }
    return false;
}

bool Location::getRoomFlag(RoomFlag flag) const
{
    if (auto a = getLoc())
    {
        return a->getRoomFlag(position, flag);
    }
    return false;
}


bool Location::getWhereFlag(WhereFlag flag) const
{
    if (auto z = getZone())
    {
        return z->getFlag(flag);
    }
    return false;
}

std::string Location::getUID(bool active) const
{
    if(auto a = getLoc())
        if (auto r = dynamic_cast<Room*>(a))
        {
            return r->getUID(active);
        }
    return "";
}

void Location::sendText(const std::string &message)
{
    if (auto a = getLoc())
    {
        a->broadcastAt(position, message);
    }
}

std::vector<std::weak_ptr<Object>> Location::getObjects() const
{
    if (auto a = getLoc())
    {
        return a->getObjects(position).snapshot_weak();
    }
    return {};
}

std::vector<std::weak_ptr<Character>> Location::getPeople() const
{
    if (auto a = getLoc())
    {
        return a->getPeople(position).snapshot_weak();
    }
    return {};
}

std::vector<std::weak_ptr<Structure>> Location::getStructures() const
{
    if (auto a = getLoc())
    {
        return a->getStructures(position).snapshot_weak();
    }
    return {};
}

int Location::getDamage() const
{
    if (auto a = getLoc())
    {
        return a->getDamage(position);
    }
    return 0;
}

void Location::setDamage(int value)
{
    if (auto a = getLoc())
    {
        a->setDamage(position, value);
    }
}

int Location::modDamage(int value)
{
    if (auto a = getLoc())
    {
        return a->modDamage(position, value);
    }
    return 0;
}

SectorType Location::getSectorType() const
{
    if (auto a = getLoc())
    {
        return a->getSectorType(position);
    }
    return SectorType::inside;
}

std::optional<std::string> Location::getTileDisplayOverride() const
{
    if (auto a = getLoc())
    {
        return a->getTileDisplayOverride(position);
    }
    return std::nullopt;
}

int Location::getTileType() const
{
    return static_cast<int>(getSectorType());
}

int Location::getGroundEffect() const
{
    if (auto a = getLoc())
    {
        return a->getGroundEffect(position);
    }
    return 0;
}

void Location::setGroundEffect(int value)
{
    if (auto a = getLoc())
    {
        a->setGroundEffect(position, value);
    }
}

int Location::modGroundEffect(int value)
{
    if (auto a = getLoc())
    {
        return a->modGroundEffect(position, value);
    }
    return 0;
}

SpecialFunc Location::getSpecialFunc()
{
    if (auto a = getLoc())
    {
        return a->getSpecialFunc(position);
    }
    return nullptr;
}

bool Location::getIsDark()
{
    if (auto a = getLoc())
    {
        return a->getIsDark(position);
    }
    return false;
}

int Location::countPlayers()
{
    int total = 0;
    auto people = getPeople();
    for (const auto &ch : volcano::util::filter_raw(people))
    {
        if (!IS_NPC(ch))
            total++;
    }
    return total;
}

bool Location::canGo(int dir)
{
    auto ex = getExit(static_cast<Direction>(dir));
    if (!ex)
        return false;
    return !ex->exit_flags[EX_CLOSED];
}

int Location::getCookElement()
{
    if (auto a = getLoc())
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
    for (const auto &obj : volcano::util::filter_raw(objs))
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
    for (const auto &obj : volcano::util::filter_raw(objs))
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
    for (const auto &obj : volcano::util::filter_raw(objs))
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
    for (const auto &obj : volcano::util::filter_raw(objs))
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


void Location::deleteExit(Direction dir)
{
    if(auto a = getLoc())
        a->deleteExit(position, dir);
}

void Location::replaceExit(const Destination &dest)
{
    if (auto a = getLoc())
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
    if (auto a = getLoc())
    {
        return a->getRoomFlags(position);
    }
    static FlagHandler<RoomFlag> standby;
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

    ch->sendFmt("@wRegion:@n {}@n\r\n", renderZoneChain(zones, ch));

    ch->send_to("@wLocation: @G%-70s@w\r\n", loc.getName());

    if(auto r = dynamic_cast<Room*>(loc.getLoc()); r)
    {
        if (auto sc = r->getScripts(); !sc.empty())
        {
            ch->sendText("@D[@GTriggers");
            for (auto t : volcano::util::filter_shared(sc))
                ch->send_to(" %d", t->getVnum());
            ch->sendText("@D] ");
        }
    }

    double grav = loc.getEnvironment(ENV_GRAVITY);
    auto g = fmt::format("{}", grav);
    snprintf(buf3, sizeof(buf3), "@D[ @G%s@D] @wSector: @D[ @G%s @D] LocationID: @D[@G%s@D]@n Gravity: @D[@G%sx@D]@n", buf, buf2, loc.getLocID().c_str(), g.c_str());
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

    ch->sendFmt("@wRegion:@n {}@n\r\n", renderZoneChain(zones, ch));
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

    if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)))
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
    
    auto a = getLoc();
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
    std::reverse(zones.begin(), zones.end());

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

    auto launch = getLaunchDestination();
    auto root_zone = zones.front();
    if(launch == *this) {
        // we are in the orbit of a place.
        ch->sendFmt("@wYou are in low orbit above {}@n\r\n", root_zone->name);
        // ship or just flying?
        auto spots = ch->location.getLocID().starts_with("S") ? root_zone->getDockingSpots() : root_zone->getLandingSpots();
        displayLandSpots(ch, root_zone->name, spots);
    }

    display_garden_info(*this, ch);

    // retrieve all entities at these coordinates.
    auto con = a->getContents<HasLocation>(position).snapshot_weak();
    auto sh = ch->shared_from_this();
    std::map<std::string, std::vector<std::weak_ptr<HasLocation>>> categorized_contents;
    for (const auto& hl : volcano::util::filter_shared(con)) {
        // Filter by visibility...
        if((hl != sh) && hl->getLocationVisibleTo(ch))
            categorized_contents[hl->getLocationDisplayCategory(ch)].emplace_back(hl);
    }

    for(auto& [cat, entities] : categorized_contents) {
        // it's impossible for entities to be empty here...
        ch->sendFmt("    {}:\r\n", cat);
        for(auto ent : volcano::util::filter_raw(entities)) {
            ent->displayLocationInfo(ch);
            ch->sendText("@n");
        }
    }
}

bool Location::buildwalk(Character* ch, Direction dir) {
    if(!ch->pref_flags.get(PRF_BUILDWALK)) return false;
    if(auto a = getLoc()) {
        return a->buildwalk(position, ch, dir);
    }
    return false;
}

Location Location::getLaunchDestination() {
    auto z = getZone();
    while(z) {
        if(!z->launchDestination.empty()) {
            if(auto loc = Location(z->launchDestination)) return loc;
        }
        z = z->getParent();
    }
    // ultimate fallback; invalid/empty location.
    return Location();
}

Zone* Location::getLandZone() {
    // retrieves the Zone that should be checked for landing destinations.
    auto z = getZone();
    while(z) {
        if(!z->launchDestination.empty()) {
            if(auto loc = Location(z->launchDestination)) {
                return z;
            }
        }
        z = z->getParent();
    }
    // ultimate fallback; invalid/empty zone.
    return nullptr;
}

void Location::setResetCommands(const std::vector<ResetCommand>& commands) {
    if(auto a = getLoc()) {
        a->setResetCommands(position, commands);
    }
}

std::vector<ResetCommand> Location::getResetCommands() const {
    if(auto a = getLoc()) return a->getResetCommands(position);
    return {};
}

void Location::setSectorType(SectorType type) {
    if(auto a = getLoc()) a->setSectorType(position, type);
}

void Location::setString(const std::string& txt, const std::string& value) {
    if(auto a = getLoc()) a->setString(position, txt, value);
}

std::string Location::renderDiagnostics(Character* viewer) const {
    return {};
}