#include "dbat/HasResetCommands.h"
#include "dbat/Character.h"
#include "dbat/Object.h"
#include "dbat/ObjectPrototype.h"
#include "dbat/CharacterPrototype.h"
#include "dbat/Destination.h"
#include "dbat/Room.h"
#include "dbat/filter.h"
#include "dbat/db.h"
#include "dbat/constants.h"
#include "dbat/utils.h"
#include "dbat/dg_scripts.h"
#include "dbat/Shop.h"


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
    if (Random::get<int>(1, 100) > chance)
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
    if (Random::get<int>(1, 100) > chance)
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
    if (Random::get<int>(1, 100) > chance)
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
    if (Random::get<int>(1, 100) > chance)
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
    if (Random::get<int>(1, 100) > chance)
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

std::string HasResetCommands::printResetCommands() const
{
    std::string output;
    for (const auto &cmd : resetCommands)
    {
        output += cmd.print() + "\r\n";
    }
    return output;
}

/*
MOB,<vnum>,<MaxSpawn>,<MaxWorld>,<chance>,<if>
    Places a mob in the location.
OBJ,<vnum>,<MaxSpawn>,<MaxWorld>,<chance>,<if>
    Places an object in the location.
GIVE,<vnum>,<targetVnum>,<chance>,<if>
    Gives an object to the last MOB <targetVnum> spawned in this sequence.
EQUIP,<vnum>,<slot>,<chance>,<if>
    Attempts to equip the last spawned mob with OBJ <vnum>. It will go to
    inventory if that fails.
    // TODO: Command for viewing slot choices.
PUT,<vnum>,<targetVnum>,<chance>,<if>
    Puts object in the inventory of the last OBJ <targetVnum> spawned in this
    sequence.
REMOVE,<vnum>
    Remove one instance of OBJ <vnum> if present.
DOOR,<direction>,<state>
    If exit in <direction> exists, set its state.
    See .choices/direction for Directions.
    <state>: 0 = open, 1 = closed, 2 = closed and locked.
TRIGGER,<vnum>,<type>
    Assign DgScript <vnum> to <type>. 0 = character, 1 = object, 2 = room.
    Will target the last spawned obj/character as appropriate.
VARIABLE,<type>,<name>,<value>
    Set a script variable to the <type> same as TRIGGER sort.
    Name should be alphanumeric with no spaces like "gotfood".
    Commas are not supported in the <value>
*/
Result<ResetCommand> parseResetCommand(std::vector<std::string> sequence) {
    for(auto& s : sequence) {
        boost::trim(s);
    }

    if(sequence.size() < 2) {
        return Err("Not enough arguments for reset command.\r\n");
    }

    auto resType = chooseEnum<ResetCommandType>(sequence[0], "reset command type");
    if(!resType) {
        return Err(resType.err);
    }

    ResetCommand cmd;
    cmd.type = resType.value();
    int argsNeeded = 0;

    switch(cmd.type) {
        case ResetCommandType::MOB:
        case ResetCommandType::OBJ:
            argsNeeded = 6;
            break;
        case ResetCommandType::GIVE:
        case ResetCommandType::EQUIP:
        case ResetCommandType::PUT:
            argsNeeded = 5;
            break;
        case ResetCommandType::VARIABLE:
            argsNeeded = 4;
            break;
        case ResetCommandType::DOOR:
        case ResetCommandType::TRIGGER:
            argsNeeded = 3;
            break;
        case ResetCommandType::REMOVE:
            argsNeeded = 2;
            break;
    }

    // Do a -1 offset because we don't present the first as counting.
    if(sequence.size() < argsNeeded) {
        return Err("Not enough arguments for reset command type '{}'. Expected {} but got {}.\r\n",
                   cmd.type, argsNeeded - 1, sequence.size() - 1);
    }

    switch(cmd.type) {
        case ResetCommandType::REMOVE: {
            auto ovnRes = parseNumber<obj_vnum>(sequence[1], "object vnum");
            if(!ovnRes) {
                return Err(ovnRes.err);
            }
            auto vn = ovnRes.value();
            if(!obj_proto.contains(vn)) {
                return Err("ObjectPrototype {} not found.\r\n", vn);
            }
            cmd.target = vn;
            return Ok(cmd);
        }
        case ResetCommandType::DOOR: {
            auto dirRes = chooseEnum<Direction>(sequence[1], "direction");
            if(!dirRes) {
                return Err(dirRes.err);
            }
            auto dir = dirRes.value();
            cmd.target = static_cast<int>(dir);

            auto stateRes = parseNumber(sequence[2], "door state");
            if(!stateRes) {
                return Err(stateRes.err);
            }
            if(stateRes.value() < 0 || stateRes.value() > 2) {
                return Err("Door state must be 0 (open), 1 (closed), or 2 (closed and locked).\r\n");
            }
            cmd.ex = stateRes.value();
            return Ok(cmd);
        }
        case ResetCommandType::TRIGGER: {
            auto vnRes = parseNumber<obj_vnum>(sequence[1], "vnum");
            if(!vnRes) {
                return Err(vnRes.err);
            }
            auto vn = vnRes.value();
            if(!trig_index.contains(vn)) {
                return Err("ObjectPrototype {} not found.\r\n", vn);
            }
            cmd.target = vn;

            auto typeRes = parseNumber(sequence[2], "trigger type");
            if(!typeRes) {
                return Err(typeRes.err);
            }
            if(typeRes.value() < 0 || typeRes.value() > 2) {
                return Err("Type must be 0 (character), 1 (object), or 2 (room).\r\n");
            }
            cmd.ex = static_cast<int>(typeRes.value());
            return Ok(cmd);
        }
        case ResetCommandType::VARIABLE: {
            auto typeRes = parseNumber(sequence[1], "trigger type");
            if(!typeRes) {
                return Err(typeRes.err);
            }
            if(typeRes.value() < 0 || typeRes.value() > 2) {
                return Err("Type must be 0 (character), 1 (object), or 2 (room).\r\n");
            }
            auto type = typeRes.value();
            cmd.ex = type;
            if(sequence[2].empty()) {
                return Err("Variable name cannot be empty.\r\n");
            }
            cmd.key = sequence[2];
            if(sequence[3].empty()) {
                return Err("Variable value cannot be empty.\r\n");
            }
            cmd.value = sequence[3];
            return Ok(cmd);
        }
        case ResetCommandType::OBJ: {
            auto ovnRes = parseNumber<obj_vnum>(sequence[1], "object vnum");
            if(!ovnRes) {
                return Err(ovnRes.err);
            }
            auto ovn = ovnRes.value();
            if(!obj_proto.contains(ovn)) {
                return Err("ObjectPrototype {} not found.\r\n", ovn);
            }
            cmd.target = ovn;

            auto maxSpawnRes = parseNumber(sequence[2], "MaxSpawn");
            if(!maxSpawnRes) {
                return Err(maxSpawnRes.err);
            }
            auto maxSpawn = maxSpawnRes.value();
            if(maxSpawn < 0) {
                return Err("MaxSpawn must be a non-negative number.\r\n");
            }
            cmd.max_location = maxSpawn;

            auto maxWorldRes = parseNumber(sequence[3], "MaxWorld");
            if(!maxWorldRes) {
                return Err(maxWorldRes.err);
            }
            auto maxWorld = maxWorldRes.value();
            if(maxWorld < 0) {
                return Err("MaxWorld must be a non-negative number.\r\n");
            }
            cmd.max = maxWorld;

            auto chanceRes = parseNumber(sequence[4], "Chance");
            if(!chanceRes) {
                return Err(chanceRes.err);
            }
            auto chance = chanceRes.value();
            if(chance < 0 || chance > 100) {
                return Err("Chance must be between 0 and 100.\r\n");
            }
            cmd.chance = chance;

            auto ifRes = parseNumber(sequence[5], "If");
            if(!ifRes) {
                return Err(ifRes.err);
            }
            cmd.if_flag = ifRes.value() ? 1 : 0;

            return Ok(cmd);
        }
        case ResetCommandType::MOB: {
            auto mvnRes = parseNumber<mob_vnum>(sequence[1], "mob vnum");
            if(!mvnRes) {
                return Err(mvnRes.err);
            }
            auto mvn = mvnRes.value();
            if(!mob_proto.contains(mvn)) {
                return Err("MobPrototype {} not found.\r\n", mvn);
            }
            cmd.target = mvn;

            auto maxSpawnRes = parseNumber(sequence[2], "MaxSpawn");
            if(!maxSpawnRes) {
                return Err(maxSpawnRes.err);
            }
            auto maxSpawn = maxSpawnRes.value();
            if(maxSpawn < 0) {
                return Err("MaxSpawn must be a non-negative number.\r\n");
            }
            cmd.max_location = maxSpawn;

            auto maxWorldRes = parseNumber(sequence[3], "MaxWorld");
            if(!maxWorldRes) {
                return Err(maxWorldRes.err);
            }
            auto maxWorld = maxWorldRes.value();
            if(maxWorld < 0) {
                return Err("MaxWorld must be a non-negative number.\r\n");
            }
            cmd.max = maxWorld;

            auto chanceRes = parseNumber(sequence[4], "Chance");
            if(!chanceRes) {
                return Err(chanceRes.err);
            }
            auto chance = chanceRes.value();
            if(chance < 0 || chance > 100) {
                return Err("Chance must be between 0 and 100.\r\n");
            }
            cmd.chance = chance;

            auto ifRes = parseNumber(sequence[5], "If");
            if(!ifRes) {
                return Err(ifRes.err);
            }
            cmd.if_flag = ifRes.value() ? 1 : 0;

            return Ok(cmd);
        }
        case ResetCommandType::GIVE: {
            auto ovnRes = parseNumber<obj_vnum>(sequence[1], "object vnum");
            if(!ovnRes) {
                return Err(ovnRes.err);
            }
            auto ovn = ovnRes.value();
            if(!obj_proto.contains(ovn)) {
                return Err("ObjectPrototype {} not found.\r\n", ovn);
            }
            cmd.target = ovn;

            auto targetVnRes = parseNumber<mob_vnum>(sequence[2], "target mob vnum");
            if(!targetVnRes) {
                return Err(targetVnRes.err);
            }
            auto targetVn = targetVnRes.value();
            if(!mob_proto.contains(targetVn)) {
                return Err("MobPrototype {} not found.\r\n", targetVn);
            }
            cmd.ex = targetVn;

            auto chanceRes = parseNumber(sequence[3], "Chance");
            if(!chanceRes) {
                return Err(chanceRes.err);
            }
            auto chance = chanceRes.value();
            if(chance < 0 || chance > 100) {
                return Err("Chance must be between 0 and 100.\r\n");
            }
            cmd.chance = chance;

            auto ifRes = parseNumber(sequence[4], "If");
            if(!ifRes) {
                return Err(ifRes.err);
            }
            cmd.if_flag = ifRes.value() ? 1 : 0;

            return Ok(cmd);
        }
        case ResetCommandType::EQUIP: {
            auto ovnRes = parseNumber<obj_vnum>(sequence[1], "object vnum");
            if(!ovnRes) {
                return Err(ovnRes.err);
            }
            auto ovn = ovnRes.value();
            if(!obj_proto.contains(ovn)) {
                return Err("ObjectPrototype {} not found.\r\n", ovn);
            }
            cmd.target = ovn;

            auto slotRes = chooseEnum<WearSlot>(sequence[2], "wear location");
            if(!slotRes) {
                return Err(slotRes.err);
            }
            auto slot = slotRes.value();
            if(slot == WearSlot::Inventory) {
                return Err("Cannot equip items in inventory slot.\r\n");
            }
            cmd.ex = static_cast<int>(slot);

            auto chanceRes = parseNumber(sequence[3], "Chance");
            if(!chanceRes) {
                return Err(chanceRes.err);
            }
            auto chance = chanceRes.value();
            if(chance < 0 || chance > 100) {
                return Err("Chance must be between 0 and 100.\r\n");
            }
            cmd.chance = chance;

            auto ifRes = parseNumber(sequence[4], "If");
            if(!ifRes) {
                return Err(ifRes.err);
            }
            cmd.if_flag = ifRes.value() ? 1 : 0;

            return Ok(cmd);
        }
        case ResetCommandType::PUT: {
            auto ovnRes = parseNumber<obj_vnum>(sequence[1], "object vnum");
            if(!ovnRes) {
                return Err(ovnRes.err);
            }
            auto ovn = ovnRes.value();
            if(!obj_proto.contains(ovn)) {
                return Err("ObjectPrototype {} not found.\r\n", ovn);
            }
            cmd.target = ovn;

            auto containerRes = parseNumber<obj_vnum>(sequence[2], "container vnum");
            if(!containerRes) {
                return Err(containerRes.err);
            }
            auto container = containerRes.value();
            if(!obj_proto.contains(container)) {
                return Err("ObjectPrototype {} not found.\r\n", container);
            }
            cmd.ex = container;

            auto chanceRes = parseNumber(sequence[3], "Chance");
            if(!chanceRes) {
                return Err(chanceRes.err);
            }
            auto chance = chanceRes.value();
            if(chance < 0 || chance > 100) {
                return Err("Chance must be between 0 and 100.\r\n");
            }
            cmd.chance = chance;

            auto ifRes = parseNumber(sequence[4], "If");
            if(!ifRes) {
                return Err(ifRes.err);
            }
            cmd.if_flag = ifRes.value() ? 1 : 0;

            return Ok(cmd);
        }
    }

    return Err("We shouldn't ever reach this. Contact staff.\r\n");
}