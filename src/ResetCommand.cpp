#include "dbat/structs.h"
#include "dbat/filter.h"
#include "dbat/db.h"
#include "dbat/constants.h"
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/shop.h"


static const char *getObjShort(obj_vnum target)
{
    if (auto found = obj_proto.find(target); found != obj_proto.end())
    {
        return found->second.short_description;
    }
    return "???";
}

static const char *getMobShort(mob_vnum target)
{
    if (auto found = mob_proto.find(target); found != mob_proto.end())
    {
        return found->second.short_description;
    }
    return "???";
}

static const char *getTrigName(trig_vnum target)
{
    if (auto found = trig_index.find(target); found != trig_index.end())
    {
        return found->second.name.c_str();
    }
    return "???";
}

std::string ResetCommand::print() const
{
    switch (type)
    {
    case ResetCommandType::MOB:
        return fmt::sprintf("%sLoad MOB %s@y [@c%d@y], MaxSpawn : %d, MaxLoc : %d, Chance : %d\r\n", if_flag ? " then " : "", getMobShort(target), target, max, max_location, chance);
    case ResetCommandType::OBJ:
        return fmt::sprintf("%sLoad OBJ %s@y [@c%d@y], MaxSpawn : %d, MaxLoc : %d, Chance : %d\r\n", if_flag ? " then " : "", getObjShort(target), target, max, max_location, chance);
    case ResetCommandType::GIVE:
        return fmt::sprintf("%sGive OBJ %s@y [@c%d@y] to MOB %s@y [@c%d@y], Chance : %d\r\n", if_flag ? " then " : "", getObjShort(target), target, getMobShort(ex), ex, chance);
    case ResetCommandType::EQUIP:
        return fmt::sprintf("%sEquip MOB %s@y [@c%d@y] with OBJ %s@y [@c%d@y], Chance : %d\r\n", if_flag ? " then " : "", getMobShort(target), target, getObjShort(ex), ex, chance);
    case ResetCommandType::PUT:
        return fmt::sprintf("%sPut OBJ %s@y [@c%d@y] in OBJ %s@y [@c%d@y], Chance : %d\r\n", if_flag ? " then " : "", getObjShort(target), target, getObjShort(ex), ex, chance);
    case ResetCommandType::REMOVE:
        return fmt::sprintf("%sRemove OBJ %s@y [@c%d@y] from location.\r\n", if_flag ? " then " : "", getObjShort(target), target);
    case ResetCommandType::DOOR:
        return fmt::sprintf("%sSet door %s as %s.\r\n", if_flag ? " then " : "", dirs[target], ex ? ((ex == 1) ? "closed" : "locked") : "open");
    case ResetCommandType::TRIGGER:
        return fmt::sprintf("%sAttach trigger @c%s@y [@c%d@y] to %s\r\n", if_flag ? " then " : "", getTrigName(target), target, ((ex == static_cast<int>(MOB_TRIGGER)) ? "mobile" : ((ex == static_cast<int>(OBJ_TRIGGER)) ? "object" : ((ex == static_cast<int>(WLD_TRIGGER)) ? "room" : "????"))));
    case ResetCommandType::VARIABLE:
        return fmt::sprintf("%sAssign global %s:%d to %s = %s\r\n", if_flag ? " then " : "", key.c_str(), ex, ((ex == static_cast<int>(MOB_TRIGGER)) ? "mobile" : ((ex == static_cast<int>(OBJ_TRIGGER)) ? "object" : ((ex == static_cast<int>(WLD_TRIGGER)) ? "room" : "????"))), value.c_str());
    }
    return "";
}

static bool resetSpawnMob(Location &loc, SpawnRegistry &reg, mob_vnum target, int max_spawn, int max_loc, int chance)
{
    if (rand_number(1, 100) > chance)
    {
        return false;
    }
    if (!mob_proto.contains(target))
    {
        return false;
    }
    auto index = fmt::format("vnum_{}", target);
    if (max_spawn || max_loc)
    {
        int count_spawn = 0;
        int count_loc = 0;
        // Retrieve set of NPCs that use this vnum...
        auto npcs = characterSubscriptions.all(index);
        for (auto ch : filter_raw(npcs))
        {

            if (auto spawnedat = ch->registeredLocations.find("spawn"); spawnedat != ch->registeredLocations.end())
            {
                if (spawnedat->second == loc)
                    if (++count_spawn >= max_spawn)
                    {
                        return false;
                    }
            }
            if (ch->location == loc)
                if (++count_loc >= max_loc)
                {
                    return false;
                }
        }
    }

    auto mob = read_mobile(target, VIRTUAL);
    mob->moveToLocation(loc);
    mob->registeredLocations["spawn"] = loc;
    auto sh = mob->shared_from_this();
    reg.lastChar = sh;
    reg.lastObj.reset();
    reg.mobiles[target].push_back(sh);

    load_mtrigger(mob);

    return true;
}

static bool resetSpawnObj(Location &loc, SpawnRegistry &reg, obj_vnum target, int max_spawn, int max_loc, int chance)
{
    if (rand_number(1, 100) > chance)
    {
        return false;
    }
    if (!obj_proto.contains(target))
    {
        return false;
    }
    auto index = fmt::format("vnum_{}", target);
    if (max_spawn || max_loc)
    {
        int count_spawn = 0;
        int count_loc = 0;
        // Retrieve set of objects that use this vnum...
        auto objs = objectSubscriptions.all(index);
        for (auto obj : filter_raw(objs))
        {

            if (auto spawnedat = obj->registeredLocations.find("spawn"); spawnedat != obj->registeredLocations.end())
            {
                if (spawnedat->second == loc)
                    if (++count_spawn >= max_spawn)
                    {
                        return false;
                    }
            }
            if (obj->location == loc)
                if (++count_loc >= max_loc)
                {
                    return false;
                }
        }
    }

    auto obj = read_object(target, VIRTUAL);
    obj->moveToLocation(loc);
    obj->registeredLocations["spawn"] = loc;
    auto sh = obj->shared_from_this();
    reg.lastObj = sh;
    reg.lastChar.reset();
    reg.objects[target].push_back(sh);

    load_otrigger(obj);

    return true;
}

static bool resetGiveObj(Location &loc, SpawnRegistry &reg, obj_vnum target, int chance)
{
    if (rand_number(1, 100) > chance)
    {
        return false;
    }
    if (!obj_proto.contains(target))
    {
        return false;
    }
    if (auto mob = reg.lastChar.lock())
    {
        auto obj = read_object(target, VIRTUAL);
        mob->addToInventory(obj);
        if (GET_MOB_SPEC(mob) != shop_keeper)
        {
            randomize_eq(obj);
        }
        load_otrigger(obj);
        auto sh = obj->shared_from_this();
        reg.lastObj = sh;
        reg.lastChar.reset();
        reg.objects[target].push_back(sh);
        return true;
    }
    return false;
}

static bool resetEquipObj(Location &loc, SpawnRegistry &reg, obj_vnum target, int slot, int chance)
{
    if (rand_number(1, 100) > chance)
    {
        return false;
    }
    if (!obj_proto.contains(target))
    {
        return false;
    }
    if (slot < 0 || slot >= NUM_WEARS)
    {
        return false;
    }
    if (auto mob = reg.lastChar.lock())
    {
        auto obj = read_object(target, VIRTUAL);
        randomize_eq(obj);
        obj->location = loc;
        load_otrigger(obj);
        if (wear_otrigger(obj, mob.get(), slot))
        {
            obj->location.al.reset();
            obj->location.position = {};
            equip_char(mob.get(), obj, slot);
        }
        else
            mob->addToInventory(obj);

        auto sh = obj->shared_from_this();
        reg.lastObj = sh;
        reg.lastChar.reset();
        reg.objects[target].push_back(sh);
        return true;
    }
    return false;
}

static bool resetPutObj(Location &loc, SpawnRegistry &reg, obj_vnum target, obj_vnum into, int chance)
{
    if (rand_number(1, 100) > chance)
    {
        return false;
    }
    if (!obj_proto.contains(target))
    {
        return false;
    }

    if (!reg.objects.contains(into))
    {
        return false;
    }

    if (auto to = reg.objects[into].back().lock())
    {
        auto obj = read_object(target, VIRTUAL);
        to->addToInventory(obj);
        load_otrigger(obj);
        auto sh = obj->shared_from_this();
        reg.lastObj = sh;
        reg.lastChar.reset();
        reg.objects[target].push_back(sh);
        return true;
    }
    return false;
}

static bool resetRemoveObj(Location &loc, SpawnRegistry &reg, obj_vnum target)
{
    // if Location contains an object of target vnum, remove first instance.
    if (auto obj = loc.searchObjects(target))
    {
        extract_obj(obj);
        reg.lastChar.reset();
        reg.lastObj.reset();
        return true;
    }
    return false;
}

static bool resetDoor(Location &loc, SpawnRegistry &reg, int dir, int state)
{
    if (dir < 0 || dir >= NUM_OF_DIRS)
    {
        return false;
    }
    auto cdir = static_cast<Direction>(dir);
    if (auto dest = loc.getExit(cdir))
    {
        switch (state)
        {
        case 0:
            dest->exit_flags.set(EX_LOCKED, false);
            dest->exit_flags.set(EX_CLOSED, false);
            break;
        case 1:
            dest->exit_flags.set(EX_CLOSED);
            dest->exit_flags.set(EX_LOCKED, false);
            break;
        case 2:
            dest->exit_flags.set(EX_LOCKED);
            dest->exit_flags.set(EX_CLOSED);
            break;
        default:
            // this shouldn't happen... it's a fail condition.
            return false;
        }
        loc.replaceExit(*dest);
        return true;
    }

    return false;
}

static bool resetTrigger(Location &loc, SpawnRegistry &reg, trig_vnum target, int extype)
{
    auto u = static_cast<UnitType>(extype);
    switch (u)
    {
    case UnitType::character:
    {
        if (auto mob = reg.lastChar.lock())
        {
            add_trigger(mob.get(), read_trigger(target), -1);
        }
        return true;
        break;
    }
    case UnitType::object:
    {
        if (auto obj = reg.lastObj.lock())
        {
            add_trigger(obj.get(), read_trigger(target), -1);
            return true;
        }
        break;
    }
    case UnitType::room:
    {   
        if(auto a = loc.getLoc())
        {
            if (auto r = dynamic_cast<Room*>(a))
            {
                add_trigger(r, read_trigger(target), -1);
                return true;
            }
        }
        break;
    }
    }
    return false;
}

static bool resetVariable(Location &loc, SpawnRegistry &reg, int extype, const std::string &key, const std::string &value)
{
    auto u = static_cast<UnitType>(extype);
    switch (u)
    {
    case UnitType::character:
    {
        if (auto mob = reg.lastChar.lock())
        {
            mob->setVariable(key, value);
        }
        return true;
    }
    case UnitType::object:
    {
        if (auto obj = reg.lastObj.lock())
        {
            obj->setVariable(key, value);
        }
        return true;
        break;
    }
    case UnitType::room:
    {
        if(auto a = loc.getLoc())
        {
            if (auto r = dynamic_cast<Room *>(a))
            {
                r->setVariable(key, value);
                return true;
            }
        }
        break;
    }
    }
    return false;
}

bool ResetCommand::executeAt(Location &loc, SpawnRegistry &reg) const
{
    switch (type)
    {
    case ResetCommandType::MOB:
        return resetSpawnMob(loc, reg, target, max, max_location, chance);
    case ResetCommandType::OBJ:
        return resetSpawnObj(loc, reg, target, max, max_location, chance);
    case ResetCommandType::GIVE:
        return resetGiveObj(loc, reg, target, chance);
    case ResetCommandType::EQUIP:
        return resetEquipObj(loc, reg, target, ex, chance);
    case ResetCommandType::PUT:
        return resetPutObj(loc, reg, target, ex, chance);
    case ResetCommandType::REMOVE:
        return resetRemoveObj(loc, reg, target);
    case ResetCommandType::DOOR:
        return resetDoor(loc, reg, target, ex);
    case ResetCommandType::TRIGGER:
        return resetTrigger(loc, reg, target, ex);
    case ResetCommandType::VARIABLE:
        return resetVariable(loc, reg, ex, key, value);
    }
    return false;
}