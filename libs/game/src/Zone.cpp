#include "dbat/game/Zone.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/Area.hpp"
#include "dbat/game/Random.hpp"

#include "dbat/game/db.hpp"
#include "dbat/game/utils.hpp"
#include "dbat/game/reset.hpp"
#include "dbat/game/dg_scripts.hpp"

#include "dbat/game/const/AdminLevel.hpp"
#include "dbat/game/const/Environment.hpp"

#include "dbat/game/players.hpp"
#include <nlohmann/json.hpp>

std::map<zone_vnum, std::shared_ptr<Zone>> zone_table;    /* zone table			 */
std::unordered_set<zone_vnum> zone_reset_queue;

void to_json(nlohmann::json& j, const Zone& unit)
{
    if(unit.number != NOTHING) {
        j[+"number"] = unit.number;
    }
    if(unit.parent != NOTHING) {
        j[+"parent"] = unit.parent;
    }
    if(!unit.name.empty()) {
        j[+"name"] = unit.name;
    }
    if(!unit.builders.empty()) {
        j[+"builders"] = unit.builders;
    }
    if(unit.lifespan != 5) {
        j[+"lifespan"] = unit.lifespan;
    }
    if(unit.age != 0.0) {
        j[+"age"] = unit.age;
    }
    if(unit.reset_mode != 2) {
        j[+"reset_mode"] = unit.reset_mode;
    }
    if(unit.zone_flags) {
        j[+"zone_flags"] = unit.zone_flags;
    }
    if(!unit.launchDestination.empty()) {
        j[+"launchDestination"] = unit.launchDestination;
    }
    if(!unit.landingSpots.empty()) {
        j[+"landingSpots"] = unit.landingSpots;
    }
    if(!unit.dockingSpots.empty()) {
        j[+"dockingSpots"] = unit.dockingSpots;
    }
}

void from_json(const nlohmann::json& j, Zone& unit)
{
    if(j.contains(+"number")) {
        j.at(+"number").get_to(unit.number);
    }
    if(j.contains(+"parent")) {
        j.at(+"parent").get_to(unit.parent);
    }
    if(j.contains(+"name")) {
        j.at(+"name").get_to(unit.name);
    }
    if(j.contains(+"builders")) {
        j.at(+"builders").get_to(unit.builders);
    }
    if(j.contains(+"lifespan")) {
        j.at(+"lifespan").get_to(unit.lifespan);
    }
    if(j.contains(+"age")) {
        j.at(+"age").get_to(unit.age);
    }
    if(j.contains(+"reset_mode")) {
        j.at(+"reset_mode").get_to(unit.reset_mode);
    }
    if(j.contains(+"zone_flags")) {
        j.at(+"zone_flags").get_to(unit.zone_flags);
    }
    if(j.contains(+"launchDestination")) {
        j.at(+"launchDestination").get_to(unit.launchDestination);
    }
    if(j.contains(+"landingSpots")) {
        j.at(+"landingSpots").get_to(unit.landingSpots);
    }
    if(j.contains(+"dockingSpots")) {
        j.at(+"dockingSpots").get_to(unit.dockingSpots);
    }
}

std::string Zone::displayNameFor(Character *ch) {

    std::string out;
    auto alevel = GET_ADMLEVEL(ch);
    auto isbuilder = alevel >= ADMLVL_BUILDER;
    if(isbuilder) {
        out += fmt::format("@W[{}]@n ", number);
    }
    std::string disp;
    if(colorName.empty()) {
        disp = name;
    } else {
        disp = colorName;
    }
    if(ch->player && !isbuilder) {
        if(ch->player->known_zones.contains(number)) {
            out += disp;
        } else {
            out += "???";
        }
    }
    else {
        out += disp;
    }

    return out;
}

std::vector<Zone *> getZoneChildren(zone_vnum parent)
{
    // This function iterates through zone_table and returns all where
    // z.parent matches our given. that means if we were given an empty
    // parent then we return all which are empty (IE: the 'root zones')
    // Since Zones keep track of their children, if we ARE given a parent
    // we can simply iterate its children.

    if (parent)
    {
        if (auto z = zone_table.find(parent); z != zone_table.end())
        {
            return z->second->getChildren();
        }
    }
    else
    {
        std::vector<Zone *> out;
        for (auto &[vnum, zone] : zone_table)
        {
            if (zone->parent == NOTHING)
            {
                out.emplace_back(zone.get());
            }
        }
        return out;
    }

    return {};
}

void Zone::sendText(const std::string &txt)
{
    for (auto i = descriptor_list; i; i = i->next)
        if (!i->connected && i->character && AWAKE(i->character) &&
            i->character->location.getZone() == this)
            i->sendText(txt);
}

std::vector<Zone *> Zone::getChildren() const
{
    std::vector<Zone *> out;
    for (auto child : children)
    {
        if (auto z = zone_table.find(child); z != zone_table.end())
        {
            out.push_back(z->second.get());
        }
    }
    return out;
}

Zone *Zone::getParent() const
{
    if (parent == NOTHING)
        return nullptr;
    if (auto z = zone_table.find(parent); z != zone_table.end())
    {
        return z->second.get();
    }
    return nullptr;
}


Zone* Zone::getUpZone(int upwards)
{
    if (upwards <= 0) return this;
    if (auto p = getParent()) {
        return p->getUpZone(upwards - 1);
    }
    return this; // no parent, so we are the topmost zone.
}

Zone* Zone::getRoot() {
    if (auto p = getParent()) {
        return p->getRoot();
    }
    return this;
}

std::vector<Zone *> Zone::getAncestors() const
{
    std::vector<Zone *> ancestors;
    if (auto p = getParent())
    {
        ancestors.push_back(p);
        auto parentAncestors = p->getAncestors();
        ancestors.insert(ancestors.end(), parentAncestors.begin(), parentAncestors.end());
    }
    return ancestors;
}

std::vector<Zone *> Zone::getChain()
{
    // like getAncestors but it includes itself as the first
    std::vector<Zone *> chain;
    chain.push_back(this);
    if (auto p = getParent())
    {
        auto parentChain = p->getChain();
        chain.insert(chain.end(), parentChain.begin(), parentChain.end());
    }
    return chain;
}

std::vector<Zone *> Zone::getDescendants() const
{
    std::vector<Zone *> descendants;
    for (auto child : children)
    {
        if (auto z = zone_table.find(child); z != zone_table.end())
        {
            descendants.push_back(z->second.get());
            auto childDescendants = z->second->getDescendants();
            descendants.insert(descendants.end(), childDescendants.begin(), childDescendants.end());
        }
    }
    return descendants;
}

static std::map<std::string, Location>
filterLocations(const std::unordered_map<std::string, std::string>& locations) {
    std::map<std::string, Location> out;
    for (const auto& [name, locid] : locations) {
        if (Location loc(locid); loc) {         // see note below about `if (loc)`
            out.emplace(name, std::move(loc));  // construct in-place; move the value
        }
    }
    return out;                                 // let NRVO/move happen
}

std::map<std::string, Location> Zone::getLandingSpots() {
    return filterLocations(landingSpots);       // elided
}

std::map<std::string, Location> Zone::getDockingSpots() {
    return filterLocations(dockingSpots);       // elided
}

Result<bool> Zone::canBeDeletedBy(Character* ch) {
    return err("Not implemented yet.");
}

Zone* HasZone::getZone() const {
    return zone.get();
}

bool Zone::getFlag(ZoneFlag zf, bool checkAncestors) const {
    if (zone_flags.get(zf)) return true;
    if (checkAncestors) {
        if (auto p = getParent()) {
            return p->getFlag(zf, true);
        }
    }
    return false;
}

bool Zone::getFlag(WhereFlag wf, bool checkAncestors) const {
    if (where_flags.get(wf)) return true;
    if (checkAncestors) {
        if (auto p = getParent()) {
            return p->getFlag(wf, true);
        }
    }
    return false;
}

double Zone::getEnvironment(int type, bool checkAncestors) const {
    if (environment.contains(type)) {
        return environment.at(type);
    }

    if(checkAncestors) {
        if (auto p = getParent()) {
            return p->getEnvironment(type, true);
        }
    }

    switch(type) {
        case ENV_GRAVITY:
            // gravity defaults to 1.0 unless manually overriden by Zone rules.
            return 1.0;
        case ENV_ETHER_STREAM:
            return zone_flags.get(ZoneFlag::ether_stream) ? 100.0 : 0.0;
        case ENV_MOONLIGHT: {
            if(zone_flags.get(ZoneFlag::has_moon)) {
                return MOON_TIMECHECK() ? 100.0 : 0.00;
            }
            return -1.0;
        }
        default:
            return 0.0;
    }

    return 0.0;
}

void Zone::sortRooms() {
    // This function sorts the rooms in this zone by their vnum, and also updates the linked list of rooms in each room to match the new order.
    auto sortedRooms = rooms.snapshot_shared();
    std::sort(sortedRooms.begin(), sortedRooms.end(), [](const std::shared_ptr<Room>& a, const std::shared_ptr<Room>& b) {
        return a->getVnum() < b->getVnum();
    });

    // Clear the current rooms and re-add them in sorted order
    rooms.clear();
    for (auto& room : sortedRooms) {
        rooms.add(room);
    }
}

/* execute the reset command table of a given zone */
void Zone::reset()
{
    age = 0;

    if (!pre_reset(number))
    {
        rooms.for_each_shared([](auto r) {
            if(auto commands = r->resetCommands; !commands.empty()) {
                Location l(r);
                l.executeResetCommands(commands);
            }
        });

        rooms.for_each([](auto r) {
            reset_wtrigger(r);
        });

        areas.for_each_shared([](auto a) {
            Location loc;
            loc.al = a;
            for(auto& [coor, to] : a->tileOverrides) {
                if(auto commands = to.resetCommands; !commands.empty()) {
                    loc.position = coor;
                    loc.locationID = loc.getLocID();
                    loc.executeResetCommands(commands);
                }
            }
        });

    }

    // TODO: Split this off into a function or something based off Location...
    rooms.for_each([](auto r) {
        if (r->room_flags.get(ROOM_AURA) && Random::get<int>(1, 5) >= 4)
        {
            r->sendText("The aura of regeneration covering the surrounding area disappears.\r\n");
            r->room_flags.set(ROOM_AURA, false);
        }

        if (r->sector_type == SectorType::lava)
        {
            r->ground_effect = 5;
        }

        if (r->ground_effect < -1)
        {
            r->sendText("The area loses some of the water flooding it.\r\n");
            r->ground_effect += 1;
        }
        else if (r->ground_effect == -1)
        {
            r->sendText("The area loses the last of the water flooding it in one large rush.\r\n");
            r->ground_effect = 0;
        }

        if (r->ground_effect >= 1 && Random::get<int>(1, 4) == 4 && !r->getEnvironment(Coordinates{}, ENV_WATER) >= 100.0 && r->sector_type != SectorType::lava)
        {
            r->sendText("The lava has cooled and become solid rock.\r\n");
            r->ground_effect = 0;
        }
        else if (r->ground_effect >= 1 && Random::get<int>(1, 2) == 2 && r->getEnvironment(Coordinates{}, ENV_WATER) >= 100.0 &&
                 r->sector_type != SectorType::lava)
        {
            r->sendText("The water has cooled the lava and it has become solid rock.\r\n");
            r->ground_effect = 0;
        }
    });
}
