#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include "dbat/character_utils.hpp"
#include "dbat/characters.hpp"
#include "dbat/descriptor.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/game_world.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"

/*
 * Walking the game's entities. Include this from .cpp files only, as it includes game_world.hpp.
 *
 *   for (char_data *ch : room_people(g, room)) { ... }
 *   room_people_each(g, room, [&](char_data *ch) { ...; return true; });  (false stops; a void lambda never stops)
 *
 * Each range takes a snapshot of its members when the loop starts, then visits them in order, skipping any that has
 * been destroyed, or is no longer where it was, by the time its turn comes. Members added during the loop are not
 * visited. So a loop may extract, move or create entities freely, including the ones it hasn't reached yet. How the
 * snapshot is taken is private to this file, so that the way entities are stored can change without touching a loop.
 */

namespace dbat_iter {

/* Where an object is. An object is still in the list being walked if this hasn't changed. */
struct obj_place {
  room_vnum room{};
  long carried_by{}, worn_by{}, in_obj{};
  bool operator==(const obj_place &) const = default;
};

inline obj_place place_of(const obj_data *o)
{
  const char_data *carrier = obj_get_carried_by(o), *wearer = obj_get_worn_by(o);
  const obj_data *container = obj_get_in_obj(o);
  return {obj_get_in_room(o), carrier ? char_get_id(carrier) : 0, wearer ? char_get_id(wearer) : 0,
          container ? obj_get_id(container) : 0};
}

/* The members of a list as they were when it was taken: each one's id, and where it was. */
template <typename T, typename Key, typename Resolve, typename Where> class snapshot {
  struct entry {
    long id{};
    Key key{};
  };
  std::vector<entry> entries_;
  Resolve resolve_; /* id -> entity, or null if it no longer exists */
  Where where_;     /* entity -> where it is now, compared with the key it was taken with */

public:
  snapshot(Resolve resolve, Where where) : resolve_(std::move(resolve)), where_(std::move(where)) {}
  void add(long id, Key key) { entries_.push_back({id, key}); }

  class iterator {
    const snapshot *s_{};
    std::size_t i_{};
    T *current_{};

    void settle()
    {
      for (; i_ < s_->entries_.size(); ++i_) {
        const entry &e = s_->entries_[i_];
        if ((current_ = s_->resolve_(e.id)) && s_->where_(current_) == e.key)
          return;
      }
      current_ = nullptr;
    }

  public:
    iterator(const snapshot *s, std::size_t i) : s_(s), i_(i) { settle(); }
    T *operator*() const { return current_; }
    iterator &operator++()
    {
      ++i_;
      settle();
      return *this;
    }
    bool operator==(const iterator &other) const { return i_ == other.i_; }
  };

  iterator begin() const { return {this, 0}; }
  iterator end() const { return {this, entries_.size()}; }
};

template <typename T, typename Key, typename Resolve, typename Where> auto make_snapshot(Resolve resolve, Where where)
{
  return snapshot<T, Key, Resolve, Where>(std::move(resolve), std::move(where));
}

inline auto char_snapshot(GameWorld *g)
{
  return make_snapshot<char_data, room_vnum>([g](long id) { return char_by_id(g, id); },
                                             [](const char_data *ch) { return char_get_in_room(ch); });
}

inline auto obj_snapshot(GameWorld *g)
{
  return make_snapshot<obj_data, obj_place>([g](long id) { return obj_by_id(g, id); }, place_of);
}

/* Characters in the list starting at head, which must be one room's people. */
inline auto chars_in_room_list(GameWorld *g, char_data *head)
{
  auto s = char_snapshot(g);
  for (char_data *ch = head; ch; ch = char_get_next_in_room(ch))
    s.add(char_get_id(ch), char_get_in_room(ch));
  return s;
}

/* Objects in the list starting at head, linked by next_content: a room's contents, an inventory or a container's. */
inline auto objs_in_content_list(GameWorld *g, obj_data *head)
{
  auto s = obj_snapshot(g);
  for (obj_data *o = head; o; o = obj_get_next_content(o))
    s.add(obj_get_id(o), place_of(o));
  return s;
}

template <typename Fn, typename T> bool call_each(Fn &fn, T *x)
{
  if constexpr (std::is_void_v<std::invoke_result_t<Fn &, T *>>) {
    fn(x);
    return true;
  } else
    return fn(x);
}

template <typename Range, typename Fn> bool each(Range &&range, Fn &fn)
{
  for (auto *x : range)
    if (!call_each(fn, x))
      return false;
  return true;
}

} // namespace dbat_iter

/* Every character in the game, NPCs and players alike. */
inline auto each_char(GameWorld *g)
{
  auto s = dbat_iter::make_snapshot<char_data, int>([g](long id) { return char_by_id(g, id); },
                                                   [](const char_data *) { return 0; });
  g->characters.visit([&s](long id, const auto *) { s.add(id, 0); });
  return s;
}

/* Every object in the game, wherever it is. */
inline auto each_obj(GameWorld *g)
{
  auto s = dbat_iter::make_snapshot<obj_data, int>([g](long id) { return obj_by_id(g, id); },
                                                  [](const obj_data *) { return 0; });
  g->objects.visit([&s](long id, const auto *) { s.add(id, 0); });
  return s;
}

/* Every connection, playing or not. */
inline auto each_desc(GameWorld *g)
{
  auto s = dbat_iter::make_snapshot<descriptor_data, int>(
      [g](long num) {
        for (descriptor_data *d = g->descriptors; d; d = d->next)
          if (d->desc_num == num)
            return d;
        return static_cast<descriptor_data *>(nullptr);
      },
      [](const descriptor_data *) { return 0; });
  for (descriptor_data *d = g->descriptors; d; d = d->next)
    s.add(d->desc_num, 0);
  return s;
}

/* The characters in a room. */
inline auto room_people(GameWorld *g, room_vnum room)
{
  return dbat_iter::chars_in_room_list(g, room == NOWHERE ? nullptr : room_get_people(g, room));
}

/* The objects lying in a room. */
inline auto room_contents(GameWorld *g, room_vnum room)
{
  return dbat_iter::objs_in_content_list(g, room == NOWHERE ? nullptr : room_get_contents(g, room));
}

/* The same, for code holding the room itself */
inline auto room_people(GameWorld *g, const room_data *room)
{
  return dbat_iter::chars_in_room_list(g, room ? room_get_people(room) : nullptr);
}
inline auto room_contents(GameWorld *g, const room_data *room)
{
  return dbat_iter::objs_in_content_list(g, room ? room_get_contents(room) : nullptr);
}

/* What a character carries, not what it wears. */
inline auto char_inventory(GameWorld *g, const char_data *ch)
{
  return dbat_iter::objs_in_content_list(g, char_get_inventory(ch));
}

/* What is inside a container. */
inline auto obj_contents(GameWorld *g, const obj_data *obj)
{
  return dbat_iter::objs_in_content_list(g, obj_get_contains(obj));
}

/* The objects in the list starting at head: any of the three lists above, for code that is handed one. */
inline auto obj_list(GameWorld *g, obj_data *head)
{
  return dbat_iter::objs_in_content_list(g, head);
}

/* The characters in the list starting at head, which must be a room's people, for code that is handed one. */
inline auto char_list(GameWorld *g, char_data *head)
{
  return dbat_iter::chars_in_room_list(g, head);
}

/* The characters following ch. */
inline auto char_followers(GameWorld *g, const char_data *ch)
{
  auto master_id = [g](const char_data *f) {
    const char_data *m = char_get_master(g, f);
    return m ? static_cast<long>(char_get_id(m)) : 0L;
  };
  auto s = dbat_iter::make_snapshot<char_data, long>([g](long id) { return char_by_id(g, id); }, master_id);
  for (follow_type *f = char_get_followers(ch); f; f = f->next)
    s.add(char_get_id(f->follower), master_id(f->follower));
  return s;
}

/* How many members range has. */
template <typename Range> std::size_t count_members(Range &&range)
{
  std::size_t n = 0;
  for ([[maybe_unused]] auto *x : range)
    n++;
  return n;
}

/* The first member of range for which pred is true, or null. */
template <typename Range, typename Pred> auto find_first(Range &&range, Pred &&pred) -> decltype(*range.begin())
{
  for (auto *x : range)
    if (pred(x))
      return x;
  return nullptr;
}

/* The same, as closures: fn(entity) returns false to stop, or returns nothing to see them all. Each returns false if
 * fn stopped it. */
template <typename Fn> bool char_iter_all(GameWorld *g, Fn &&fn) { return dbat_iter::each(each_char(g), fn); }
template <typename Fn> bool obj_iter_all(GameWorld *g, Fn &&fn) { return dbat_iter::each(each_obj(g), fn); }
template <typename Fn> bool desc_iter_all(GameWorld *g, Fn &&fn) { return dbat_iter::each(each_desc(g), fn); }
template <typename Fn>
bool room_people_each(GameWorld *g, room_vnum room, Fn &&fn)
{
  return dbat_iter::each(room_people(g, room), fn);
}
template <typename Fn>
bool room_contents_each(GameWorld *g, room_vnum room, Fn &&fn)
{
  return dbat_iter::each(room_contents(g, room), fn);
}
template <typename Fn> bool char_inventory_each(GameWorld *g, const char_data *ch, Fn &&fn)
{
  return dbat_iter::each(char_inventory(g, ch), fn);
}
template <typename Fn> bool obj_contents_each(GameWorld *g, const obj_data *obj, Fn &&fn)
{
  return dbat_iter::each(obj_contents(g, obj), fn);
}
template <typename Fn> bool char_followers_each(GameWorld *g, const char_data *ch, Fn &&fn)
{
  return dbat_iter::each(char_followers(g, ch), fn);
}

/* What a character wears: fn(obj, wear position) for each occupied position, in position order. */
template <typename Fn> bool char_equipment_each(const char_data *ch, Fn &&fn)
{
  for (int pos = 0; pos < NUM_WEARS; pos++) {
    obj_data *obj = char_get_equipment(ch, pos);
    if (!obj)
      continue;
    if constexpr (std::is_void_v<std::invoke_result_t<Fn &, obj_data *, int>>)
      fn(obj, pos);
    else if (!fn(obj, pos))
      return false;
  }
  return true;
}
