#pragma once
#include <memory>

#include "Typedefs.hpp"

template<class T, class Id = int64_t>
struct Handle {
    Id id{NOTHING}; // stable identifier
    mutable std::weak_ptr<T> cache;          // lazily filled

    // Lock the handle: fast if cache live, else resolve from the registry.
    std::shared_ptr<T> lock() const {
        if (auto sp = cache.lock()) return sp;
        if(auto found = T::registry.find(id); found != T::registry.end()) {
            auto sp = found->second; // your global registry API
            cache = sp;  // cache for next time
            return sp;
        }
        return {};
    }

    void reset() {
        cache.reset();
        id = NOTHING;
    }

    bool empty() const { return !lock(); }

    operator bool() const {
        return lock() ? true : false;
    }

    Handle<T, Id>& operator=(const std::shared_ptr<T>& sp) {
        if(sp) {
            cache = sp;
            id = sp->id;
        } else {
            cache.reset();
            id = NOTHING;
        }
        return *this;
    }

    Handle<T, Id>& operator=(const Handle<T, Id>& other) {
        if (this != &other) {
            id = other.id;
            cache = other.cache;
        }
        return *this;
    }

    Handle<T, Id>& operator=(T* other) {
        if(other) {
            cache = other->shared_from_this();
            id = other->id;
        } else {
            cache.reset();
            id = NOTHING;
        }
        return *this;
    }

    Handle<T, Id>& operator=(Id newId) {
        id = newId;
        cache.reset();
        return *this;
    }

    bool operator==(Id otherId) const {
        return id == otherId;
    }

    bool operator==(T* other) const {
        if (auto sp = lock()) {
            return sp.get() == other;
        }
        return false;
    }

    bool operator==(const std::shared_ptr<T>& other) const {
        if (auto sp = lock()) {
            return sp == other;
        }
        return false;
    }
};

struct Character;
struct Room;
struct Object;

using CharacterHandle = Handle<Character>;
using ObjectHandle = Handle<Object>;
using RoomHandle = Handle<Room, int>;