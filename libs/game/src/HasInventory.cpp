#include "dbat/game/HasInventory.hpp"
#include "dbat/game/Object.hpp"
#include "volcano/util/FilterWeak.hpp"

void HasInventory::addToInventory(Object *obj)
{
    addToInventory(obj->shared_from_this());
}

void HasInventory::addToInventory(const std::shared_ptr<Object> &obj)
{
    inventory.push_back(obj);
    onAddToInventory(obj);
}

void HasInventory::removeFromInventory(Object *obj)
{
    removeFromInventory(obj->shared_from_this());
}

void HasInventory::removeFromInventory(const std::shared_ptr<Object> &obj)
{
    inventory.remove_if([&](const std::weak_ptr<Object> &weak_obj)
                        { return weak_obj.lock() == obj; });
    onRemoveFromInventory(obj);
}

std::vector<std::weak_ptr<Object>> HasInventory::getInventory() const
{
    std::vector<std::weak_ptr<Object>> out;
    for (const auto &weak_obj : inventory)
    {
        if (auto obj = weak_obj.lock())
        {
            out.push_back(obj);
        }
    }
    return out;
}


void HasInventory::traverseInventory(const std::function<void(Object *)> &func, bool recurse)
{
    for (const auto &obj : volcano::util::filter_raw(inventory))
    {
        func(obj);
        if (recurse)
        {
            obj->traverseInventory(func, true);
        }
    }
}

std::unordered_set<Object *> HasInventory::gatherFromInventory(const std::function<bool(Object *)> &func, bool recurse)
{
    std::unordered_set<Object *> out;
    for (const auto &obj : volcano::util::filter_raw(inventory))
    {
        if (func(obj))
        {
            out.insert(obj);
        }
        if (recurse)
        {
            auto sub = obj->gatherFromInventory(func, true);
            out.insert(sub.begin(), sub.end());
        }
    }
    return out;
}

Object *HasInventory::searchInventory(const std::function<bool(Object *)> &func, bool recurse)
{
    for (const auto &obj : volcano::util::filter_raw(inventory))
    {
        if (func(obj))
        {
            return obj;
        }
        if (recurse)
        {
            if (auto sub = obj->searchInventory(func, true))
            {
                return sub;
            }
        }
    }
    return nullptr;
}

Object *HasInventory::searchInventory(obj_vnum vnum, bool working, bool recurse)
{
    return searchInventory([=](Object *obj)
                           { return obj->getVnum() == vnum && (!working || obj->isWorking()); }, recurse);
}

void HasInventory::activateInventory()
{
    for (const auto &obj : volcano::util::filter_raw(inventory))
    {
        obj->activate();
    }
}

void HasInventory::deactivateInventory()
{
    for (const auto &obj : volcano::util::filter_raw(inventory))
    {
        obj->deactivate();
    }
}
