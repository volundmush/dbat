#pragma once
#include <memory>
#include "ID.hpp"

template<typename T, typename Id = int64_t>
std::shared_ptr<T> createEntityWithID(Id id, bool skipCheck = false) {
    if(!skipCheck && T::registry.contains(id)) {
        throw std::runtime_error("Entity with ID " + std::to_string(id) + " already exists.");
    }
    auto obj = std::make_shared<T>();
    obj->setID(id);
    T::registry.emplace(id, obj);
    return obj;
}

template<typename T>
std::shared_ptr<T> createEntity() {
    auto id = getNextID(T::lastID, T::registry);
    return createEntityWithID<T>(id, true);
}