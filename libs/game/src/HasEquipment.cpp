#include "dbat/game/HasEquipment.hpp"
#include "dbat/game/Object.hpp"

std::map<int, Object *> HasEquipment::getEquipment() const
{
    std::map<int, Object *> out;
    for (auto &[slot, ow] : equipment)
    {
        if (auto o = ow.lock())
        {
            out[slot] = o.get();
        }
    }
    return out;
}

Object *HasEquipment::getEquipSlot(int slot) const
{
    if (auto find = equipment.find(slot); find != equipment.end())
    {
        if (auto o = find->second.lock())
        {
            return o.get();
        }
    }
    return nullptr;
}

void HasEquipment::addToEquip(Object *obj, int slot)
{
    addToEquip(obj->shared_from_this(), slot);
}

void HasEquipment::addToEquip(const std::shared_ptr<Object> &obj, int slot)
{
    if (auto has = getEquipSlot(slot))
    {
        throw std::runtime_error(fmt::format("Slot {} already occupied by {}", slot, has->getName()));
    }
    equipment[slot] = obj;
    onAddToEquip(obj, slot);
}

void HasEquipment::removeFromEquip(int slot)
{
    if (auto it = equipment.find(slot); it != equipment.end())
    {
        if (auto obj = it->second.lock())
        {
            onRemoveFromEquip(obj, slot);
        }
        equipment.erase(it);
    }
}

void HasEquipment::removeFromEquip(Object *obj)
{
    removeFromEquip(obj->shared_from_this());
}

void HasEquipment::removeFromEquip(const std::shared_ptr<Object> &obj)
{
    for (auto it = equipment.begin(); it != equipment.end();)
    {
        if (auto o = it->second.lock())
        {
            if (o == obj)
            {
                it = equipment.erase(it);
                auto slot = it->first;
                onRemoveFromEquip(obj, slot);
            }
            else
            {
                ++it;
            }
        }
        else
        {
            it = equipment.erase(it);
        }
    }
}



void HasEquipment::traverseEquipment(const std::function<void(Object *)> &func, bool recurse)
{
    for (const auto &[slot, wobj] : equipment)
    {
        if (auto obj = wobj.lock())
        {
            func(obj.get());
            if (recurse)
            {
                obj->traverseInventory(func, true);
            }
        }
    }
}

std::unordered_set<Object *> HasEquipment::gatherFromEquipment(const std::function<bool(Object *)> &func, bool recurse)
{
    std::unordered_set<Object *> out;
    for (const auto &[slot, wobj] : equipment)
    {
        if (auto obj = wobj.lock())
        {
            if (func(obj.get()))
            {
                out.insert(obj.get());
            }
            if (recurse)
            {
                auto sub = obj->gatherFromInventory(func, true);
                out.insert(sub.begin(), sub.end());
            }
        }
    }
    return out;
}

Object *HasEquipment::searchEquipment(const std::function<bool(Object *)> &func, bool recurse)
{
    for (const auto &[slot, wobj] : equipment)
    {
        if (auto obj = wobj.lock())
        {
            if (func(obj.get()))
            {
                return obj.get();
            }
            if (recurse)
            {
                if (auto sub = obj->searchInventory(func, true))
                {
                    return sub;
                }
            }
        }
    }
    return nullptr;
}

Object *HasEquipment::searchEquipment(obj_vnum vnum, bool working, bool recurse)
{
    return searchEquipment([=](Object *obj)
                           { return obj->getVnum() == vnum && (!working || obj->isWorking()); }, recurse);
}

void HasEquipment::activateEquipment()
{
    for (const auto &[slot, wobj] : equipment)
    {
        if (auto obj = wobj.lock())
        {
            obj->activate();
        }
    }
}

void HasEquipment::deactivateEquipment()
{
    for (const auto &[slot, wobj] : equipment)
    {
        if (auto obj = wobj.lock())
        {
            obj->deactivate();
        }
    }
}