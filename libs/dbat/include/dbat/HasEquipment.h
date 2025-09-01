#pragma once
#include "sysdep.h"

// Characters have Equipment. Perhaps Objects will too soon.
struct HasEquipment {
    std::map<int, std::weak_ptr<Object>> equipment;

    std::map<int, Object*> getEquipment() const;
    Object* getEquipSlot(int slot) const;

    void addToEquip(Object* obj, int slot);
    void addToEquip(const std::shared_ptr<Object>& obj, int slot);
    virtual void onAddToEquip(const std::shared_ptr<Object>& obj, int slot) = 0;
    void removeFromEquip(int slot);
    void removeFromEquip(Object* obj);
    void removeFromEquip(const std::shared_ptr<Object>& obj);
    virtual void onRemoveFromEquip(const std::shared_ptr<Object>& obj, int slot) = 0;

    void traverseEquipment(const std::function<void(Object*)> &func, bool recurse = true);
    struct Object* searchEquipment(const std::function<bool(Object*)> &func, bool recurse = true);
    struct Object* searchEquipment(obj_vnum vnum, bool working = true, bool recurse = true);
    std::unordered_set<struct Object*> gatherFromEquipment(const std::function<bool(Object*)> &func, bool recurse = true);

    void activateEquipment();
    void deactivateEquipment();
};