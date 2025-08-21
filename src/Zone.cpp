#include "dbat/Zone.h"
#include "dbat/Room.h"
#include "dbat/Descriptor.h"
#include "dbat/Character.h"
#include "dbat/db.h"
#include "dbat/utils.h"

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
            return z->second.getChildren();
        }
    }
    else
    {
        std::vector<Zone *> out;
        for (auto &[vnum, zone] : zone_table)
        {
            if (zone.parent == NOTHING)
            {
                out.emplace_back(&zone);
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
            out.push_back(&z->second);
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
        return &z->second;
    }
    return nullptr;
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
            descendants.push_back(&z->second);
            auto childDescendants = z->second.getDescendants();
            descendants.insert(descendants.end(), childDescendants.begin(), childDescendants.end());
        }
    }
    return descendants;
}

Result<bool> Zone::canBeDeletedBy(Character* ch) {
    return Err("Not implemented yet.");
}

Zone* HasZone::getZone() const {
    return zone.get();
}

bool Zone::getZoneFlag(ZoneFlag zf, bool checkAncestors) const {
    if (zone_flags.get(zf)) return true;
    if (checkAncestors) {
        if (auto p = getParent()) {
            return p->getZoneFlag(zf, true);
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