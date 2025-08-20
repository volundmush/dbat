#pragma once
#include "sysdep.h"

// Characters and Objects have inventories.
// Only Objects can be in inventories.
struct HasInventory {
    std::vector<std::weak_ptr<Object>> getInventory() const;
    std::list<std::weak_ptr<Object>> inventory;

    void addToInventory(Object* obj);
    void addToInventory(const std::shared_ptr<Object>& obj);
    virtual void onAddToInventory(const std::shared_ptr<Object>& obj) = 0;
    void removeFromInventory(Object* obj);
    virtual void removeFromInventory(const std::shared_ptr<Object>& obj);
    virtual void onRemoveFromInventory(const std::shared_ptr<Object>& obj) = 0;

    void traverseInventory(const std::function<void(Object*)> &func, bool recurse = true);
    Object* searchInventory(const std::function<bool(Object*)> &func, bool recurse = true);
    Object* searchInventory(obj_vnum vnum, bool working = true, bool recurse = true);
    std::unordered_set<Object*> gatherFromInventory(const std::function<bool(Object*)> &func, bool recurse = true);

    void activateInventory();
    void deactivateInventory();
};