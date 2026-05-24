// This file must only be included by .cpp files and never in other headers!

#pragma once

#include <cstddef>
#include <initializer_list>
#include <utility>

#include "dbat/db/consts/types.h"
#include "dbat/db/objects.h"
#include "dbat/db/rooms.h"
#include "dbat/db/characters.h"

namespace dbat::game::search {

template <typename Fn>
bool object_list_each(obj_data *head, Fn &&fn)
{
    auto *current = head;
    while (current) {
        auto *next = current->next_content;
        if (!fn(current)) {
            return false;
        }
        current = next;
    }

    return true;
}

template <typename Fn>
bool object_tree_each(obj_data *head, Fn &&fn)
{
    return object_list_each(head, [&](obj_data *obj) {
        if (!fn(obj)) {
            return false;
        }

        return object_tree_each(obj->contains, fn);
    });
}

template <typename Fn>
bool room_objects_each(room_data *room, bool recursive, Fn &&fn)
{
    if (!room) {
        return true;
    }

    if (recursive) {
        return object_tree_each(room->contents, std::forward<Fn>(fn));
    }

    return object_list_each(room->contents, std::forward<Fn>(fn));
}

template <typename Fn>
bool object_inventory_each(obj_data *obj, bool recursive, Fn &&fn)
{
    if (!obj) {
        return true;
    }

    if (recursive) {
        return object_tree_each(obj->contains, std::forward<Fn>(fn));
    }

    return object_list_each(obj->contains, std::forward<Fn>(fn));
}

template <typename Fn>
bool character_inventory_each(char_data *ch, bool recursive, Fn &&fn)
{
    if (!ch) {
        return true;
    }

    if (recursive) {
        return object_tree_each(ch->carrying, std::forward<Fn>(fn));
    }

    return object_list_each(ch->carrying, std::forward<Fn>(fn));
}

template <typename Fn>
bool room_people_each(room_data *room, Fn &&fn)
{
    if (!room) {
        return true;
    }

    auto *current = room->people;
    while (current) {
        auto *next = current->next_in_room;
        if (!fn(current)) {
            return false;
        }
        current = next;
    }

    return true;
}

template <typename Fn>
bool character_equipment_each(char_data *ch, Fn &&fn)
{
    if (!ch) {
        return true;
    }

    for (std::size_t pos = 0; pos < NUM_WEARS; ++pos) {
        auto *obj = ch->equipment[pos];
        if (obj && !fn(obj, pos)) {
            return false;
        }
    }

    return true;
}

template <typename Each, typename Pred>
obj_data *object_find(Each &&each, Pred &&pred)
{
    obj_data *found = nullptr;
    each([&](obj_data *obj) {
        if (!pred(obj)) {
            return true;
        }

        found = obj;
        return false;
    });

    return found;
}

template <typename Pred>
char_data *room_people_find(room_data *room, Pred &&pred)
{
    char_data *found = nullptr;
    room_people_each(room, [&](char_data *ch) {
        if (!pred(ch)) {
            return true;
        }

        found = ch;
        return false;
    });

    return found;
}

template <typename Pred>
obj_data *object_list_find(obj_data *head, Pred &&pred)
{
    return object_find(
        [&](auto &&visitor) { return object_list_each(head, visitor); },
        std::forward<Pred>(pred));
}

template <typename Pred>
obj_data *room_object_find(room_data *room, bool recursive, Pred &&pred)
{
    return object_find(
        [&](auto &&visitor) { return room_objects_each(room, recursive, visitor); },
        std::forward<Pred>(pred));
}

template <typename Pred>
obj_data *character_inventory_find(char_data *ch, bool recursive, Pred &&pred)
{
    return object_find(
        [&](auto &&visitor) { return character_inventory_each(ch, recursive, visitor); },
        std::forward<Pred>(pred));
}

template <typename Pred>
obj_data *object_inventory_find(obj_data *obj, bool recursive, Pred &&pred)
{
    return object_find(
        [&](auto &&visitor) { return object_inventory_each(obj, recursive, visitor); },
        std::forward<Pred>(pred));
}

template <typename Pred>
obj_data *character_equipment_find(char_data *ch, Pred &&pred)
{
    obj_data *found = nullptr;
    character_equipment_each(ch, [&](obj_data *obj, std::size_t pos) {
        if (!pred(obj, pos)) {
            return true;
        }

        found = obj;
        return false;
    });

    return found;
}

template <typename Each, typename Pred>
bool any(Each &&each, Pred &&pred)
{
    bool matched = false;
    each([&](auto *item) {
        matched = pred(item);
        return !matched;
    });

    return matched;
}

template <typename Each, typename Pred>
std::size_t count_if(Each &&each, Pred &&pred)
{
    std::size_t count = 0;
    each([&](auto *item) {
        if (pred(item)) {
            ++count;
        }

        return true;
    });

    return count;
}

template <typename T, std::size_t N, typename Value>
bool contains_value(const T (&values)[N], const Value &value)
{
    for (const auto &candidate : values) {
        if (candidate == value) {
            return true;
        }
    }

    return false;
}

template <typename T, typename Value>
bool contains_value(std::initializer_list<T> values, const Value &value)
{
    for (const auto &candidate : values) {
        if (candidate == value) {
            return true;
        }
    }

    return false;
}

template <std::size_t N>
obj_data *character_inventory_find_vnum(char_data *ch, const obj_vnum (&vnums)[N], bool recursive = false)
{
    return character_inventory_find(ch, recursive, [&](obj_data *obj) {
        return contains_value(vnums, obj_vnum_get(obj));
    });
}

inline obj_data *character_inventory_find_vnum(char_data *ch, std::initializer_list<obj_vnum> vnums, bool recursive = false)
{
    return character_inventory_find(ch, recursive, [&](obj_data *obj) {
        return contains_value(vnums, obj_vnum_get(obj));
    });
}

template <std::size_t N>
obj_data *object_inventory_find_vnum(obj_data *obj, const obj_vnum (&vnums)[N], bool recursive = false)
{
    return object_inventory_find(obj, recursive, [&](obj_data *contained) {
        return contains_value(vnums, obj_vnum_get(contained));
    });
}

inline obj_data *object_inventory_find_vnum(obj_data *obj, std::initializer_list<obj_vnum> vnums, bool recursive = false)
{
    return object_inventory_find(obj, recursive, [&](obj_data *contained) {
        return contains_value(vnums, obj_vnum_get(contained));
    });
}

template <std::size_t N>
obj_data *room_object_find_vnum(room_data *room, const obj_vnum (&vnums)[N], bool recursive = false)
{
    return room_object_find(room, recursive, [&](obj_data *obj) {
        return contains_value(vnums, obj_vnum_get(obj));
    });
}

inline obj_data *room_object_find_vnum(room_data *room, std::initializer_list<obj_vnum> vnums, bool recursive = false)
{
    return room_object_find(room, recursive, [&](obj_data *obj) {
        return contains_value(vnums, obj_vnum_get(obj));
    });
}

} // namespace dbat::game::search
