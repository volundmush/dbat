#include "dbat/structs.h"
#include "dbat/genwld.h"
#include "dbat/utils.h"
#include "dbat/db.h"
#include "dbat/handler.h"
#include "dbat/genolc.h"
#include "dbat/shop.h"
#include "dbat/constants.h"
#include "dbat/planet.h"
#include "dbat/constants.h"
#include "dbat/filter.h"
#include "dbat/dg_scripts.h"
#include "dbat/send.h"
#include "dbat/act.wizard.h"

Room::Room() : AbstractLocation()
{
    type = UnitType::room;
}

/*
 * This function will copy the strings so be sure you free your own
 * copies of the description, title, and such.
 */
std::shared_ptr<Room> Room::shared()
{
    return shared_from_this();
}

std::string Room::getUID(bool active) const
{
    return fmt::format("#R{}{}", vn, active ? "!" : "");
}

bool Room::isActive() const
{
    return true;
}

int Room::getDamage() const
{
    return damage;
}

void Room::activate()
{
    assign_triggers(this, WLD_TRIGGER);

    if (!scripts.empty())
    {
        if (SCRIPT_TYPES(this) & OTRIG_RANDOM)
            roomSubscriptions.subscribe("randomTriggers", shared_from_this());
        if (SCRIPT_TYPES(this) & OTRIG_TIME)
            roomSubscriptions.subscribe("timeTriggers", shared_from_this());
    }
    if (damage != 0)
        roomSubscriptions.subscribe("roomRepairDamage", shared_from_this());
}

void Room::deactivate()
{
    roomSubscriptions.unsubscribeFromAll(shared_from_this());
}

int Room::setDamage(int amount)
{
    auto before = damage;
    damage = std::clamp<int>(amount, 0, 100);
    // if(dmg != before) save();
    if (damage == 0)
    {
        roomSubscriptions.unsubscribe("roomRepairDamage", shared_from_this());
    }
    else
    {
        roomSubscriptions.subscribe("roomRepairDamage", shared_from_this());
    }
    return damage;
}

int Room::modDamage(int amount)
{
    return setDamage(damage + amount);
}

static const std::unordered_set<int> inside_sectors = {SECT_INSIDE, SECT_UNDERWATER, SECT_IMPORTANT, SECT_SHOP, SECT_SPACE};

static const std::map<std::string, int> _dirNames = {
    {"north", NORTH},
    {"east", EAST},
    {"south", SOUTH},
    {"west", WEST},
    {"up", UP},
    {"down", DOWN},
    {"northwest", NORTHWEST},
    {"northeast", NORTHEAST},
    {"southwest", SOUTHWEST},
    {"southeast", SOUTHEAST},
    {"inside", INDIR},
    {"outside", OUTDIR}

};

std::optional<std::string> Room::dgCallMember(const std::string &member, const std::string &arg)
{
    std::string lmember = member;
    boost::to_lower(lmember);
    boost::trim(lmember);
    char bitholder[MAX_STRING_LENGTH];

    if (auto d = _dirNames.find(lmember); d != _dirNames.end())
    {
        auto ex = getDirection(static_cast<Direction>(d->second));
        if (!ex)
        {
            return "";
        }
        if (!arg.empty())
        {
            if (!strcasecmp(arg.c_str(), "vnum"))
            {
                return fmt::format("{}", ex->getVnum());
            }
            else if (!strcasecmp(arg.c_str(), "key"))
                return fmt::format("{}", ex->key);
            else if (!strcasecmp(arg.c_str(), "bits"))
            {
                return ex->exit_flags.getFlagNames();
            }
            else if (!strcasecmp(arg.c_str(), "room"))
            {
                return fmt::format("{}", ex->getUID(true));
            }
        }
        else /* no subfield - default to bits */
        {
            return ex->exit_flags.getFlagNames();
        }
    }

    return {};
}

double Room::setEnvironment(int type, double value)
{
    environment[type] = value;
    return value;
}

double Room::modEnvironment(int type, double value)
{
    environment[type] += value;
    return environment[type];
}

void Room::clearEnvironment(int type)
{
    environment.erase(type);
}

static const std::vector<std::pair<std::pair<room_vnum, room_vnum>, double>> gravityRanges = {
    // North Kai's Planet
    {{6100, 6138}, 10.0},

    // Personal Pocket Dimensions
    {{18900, 19899}, 1000.0},

    // HBTC / Room of Spirit and Time - various ranges
    {{64000, 64006}, 100.0},
    {{64007, 64016}, 300.0},
    {{64017, 64030}, 500.0},
    {{64031, 64048}, 1000.0},
    {{64049, 64070}, 5000.0},
    {{64071, 64096}, 10000.0},
    {{64097, 64097}, 1000.0},
};

double Room::getEnvironment(int type) const
{
    auto planet = getPlanet(getVnum());
    switch (type)
    {
    case ENV_GRAVITY:
    {
        // check for a gravity generator...
        // bypass const deliberately here...
        auto con = ((Room*)this)->getObjects();
        for (auto c : filter_raw(con))
        {
            if (auto g = c->getBaseStat("gravity"); g > 0.0)
                return g;
        }

        // check gravityRanges
        for (const auto &[range, grav] : gravityRanges)
        {
            if (vn >= range.first && vn <= range.second)
            {
                return grav;
            }
        }

        if (environment.contains(type))
            return environment.at(type);

        if (planet)
        {
            if (auto a = getPlanetEnvironment(planet.value(), type); a)
            {
                return a.value();
            }
        }

        return 1.0;
    }

    case ENV_WATER:
        if (ground_effect < 0)
            return 100.0;
        switch (static_cast<int>(sector_type))
        {
        case SECT_WATER_SWIM:
            return 50.0;
        case SECT_WATER_NOSWIM:
            return 75.0;
        case SECT_UNDERWATER:
            return 100.0;
        }
        break;
    case ENV_MOONLIGHT:
    {
        if (!planet)
            return -1;
        if (where_flags[WhereFlag::space])
            return -1;
        for (auto f : {ROOM_INDOORS, ROOM_UNDERGROUND})
            if (room_flags.get(f))
                return -1;
        if (inside_sectors.contains(static_cast<int>(sector_type)))
            return -1;
        return getPlanetEnvironment(planet.value(), type).value();
    }
    case ENV_ETHER_STREAM:
    {
        if (!planet)
            return 0.0;
        return getPlanetEnvironment(planet.value(), type).value();
    }
    }
    if (environment.contains(type))
        return environment.at(type);
    return 0.0;
}

vnum Room::getLocVnum() const
{
    return getVnum();
}

std::string Room::getLocID() const {
    return fmt::format("{}:{}", "R", getVnum());
}

const char *Room::getDgName() const
{
    return HasMudStrings::getName();
}

std::vector<trig_vnum> Room::getProtoScript() const
{
    return proto_script;
}

// This implementation is problematic because it recursively calls itself.
// Also, since unit_data::getName() takes no arguments, you cannot overload it with a const Coordinates&
// unless you want to provide a new interface. If you want to call the base version, do:

const std::vector<ExtraDescription> &Room::getExtraDescription(const Coordinates &coor) const
{
    return HasExtraDescriptions::getExtraDescription();
}

Zone *Room::getLocZone() const
{
    return zone.get();
}

std::shared_ptr<AbstractLocation> Room::getSharedAbstractLocation() {
    return shared_from_this();
}

const char *Room::getName(const Coordinates & /*coor*/) const
{
    return getName();
}

const char *Room::getLookDescription(const Coordinates &coor) const
{
    // For rooms, look description does not vary by coordinate inside the room.
    return HasMudStrings::getLookDescription();
}

std::optional<Destination> Room::getDirection(Direction dir) const
{
    if (exits.contains(dir))
        return exits.at(dir);
    return std::nullopt;
}

std::map<Direction, Destination> Room::getDirections() const
{
    std::map<Direction, Destination> out;
    for (const auto &[dir, dest] : exits)
    {
        if (dest)
            out[dir] = dest;
    }
    return out;
}

std::optional<Destination> Room::getDirection(const Coordinates &coor, Direction dir)
{
    return getDirection(dir);
}

std::map<Direction, Destination> Room::getDirections(const Coordinates &coor)
{
    return getDirections();
}



SectorType Room::getSectorType(const Coordinates &coor) const
{
    return sector_type;
}

void Room::broadcastAt(const Coordinates &coor, const std::string &message)
{
    auto people = getPeople(coor);
    for (const auto &c : filter_shared(people))
    {
        c->sendText(message.c_str());
    }
}

int Room::getDamage(const Coordinates &coor) const
{
    return getDamage();
}
int Room::setDamage(const Coordinates &coor, int amount)
{
    return setDamage(amount);
}
int Room::modDamage(const Coordinates &coor, int amount)
{
    return modDamage(amount);
}

int Room::getGroundEffect(const Coordinates &coor) const
{
    return ground_effect;
}
void Room::setGroundEffect(const Coordinates &coor, int effect)
{
    ground_effect = effect;
}

int Room::modGroundEffect(const Coordinates &coor, int effect)
{
    ground_effect += effect;
    return ground_effect;
}

SpecialFunc Room::getSpecialFunc(const Coordinates &coor) const
{
    return func;
}

double Room::getEnvironment(const Coordinates &coor, int type) const
{
    return getEnvironment(type);
}
double Room::setEnvironment(const Coordinates &coor, int type, double value)
{
    return setEnvironment(type, value);
}
double Room::modEnvironment(const Coordinates &coor, int type, double value)
{
    return modEnvironment(type, value);
}
void Room::clearEnvironment(const Coordinates &coor, int type)
{
    clearEnvironment(type);
}

void Room::sendText(const std::string &txt)
{
    auto people = getPeople();
    for (auto i : filter_raw(people))
    {
        i->sendText(txt);
    }

    for (auto d = descriptor_list; d; d = d->next)
    {
        if (STATE(d) != CON_PLAYING)
            continue;

        if (PRF_FLAGGED(d->character, PRF_ARENAWATCH))
        {
            if (arena_watch(d->character) == vn)
            {
                d->sendText("@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n" + txt);
            }
        }
        if (auto eaves = GET_EAVESDROP(d->character); eaves > 0)
        {
            int roll = rand_number(1, 101);
            if (eaves == vn && GET_SKILL(d->character, SKILL_EAVESDROP) > roll)
            {
                d->sendText("@c-----Eavesdrop-----@n\r\n%s\r\n@c-----Eavesdrop-----@n\r\n" + txt);
            }
        }
    }
}

void Room::deleteExit(Direction dir)
{
    if (auto find = exits.find(dir); find != exits.end())
    {
        exits.erase(find);
    }
}

void Room::replaceExit(const Destination &dest)
{
    exits[dest.dir] = dest;
}

void Room::deleteExit(const Coordinates &coor, Direction dir)
{
    deleteExit(dir);
}

void Room::replaceExit(const Coordinates &coor, const Destination &dest)
{
    replaceExit(dest);
}

FlagHandler<WhereFlag>& Room::getWhereFlags(const Coordinates &coor)
{
    return where_flags;
}

FlagHandler<RoomFlag>& Room::getRoomFlags(const Coordinates &coor)
{
    return room_flags;
}

bool Room::buildwalk(const Coordinates& coor, Character* ch, Direction dir) {
    // by the time we reach this function, all permission checks have already been carried out.

    auto r = std::make_shared<Room>();
    r->vn = getNextID(lastRoomID, world);
    r->strings["name"] = "New BuildWalk Room";
    r->strings["look_description"] = fmt::format("This unfinished room was created by {}.\r\n", ch->getName());
    r->zone = zone;

    world[r->vn] = r;

    Destination dest, rdest;
    dest.dir = dir;
    dest.al = r;
    ch->location.replaceExit(dest);

    rdest.dir = static_cast<Direction>(rev_dir[static_cast<int>(dir)]);
    rdest.al = ch->location.al;
    rdest.position = ch->location.position;
    dest.replaceExit(rdest);

    ch->send_to("@yRoom #%d created by BuildWalk.@n\r\n", r->vn);
    update_space();

    return true;
}