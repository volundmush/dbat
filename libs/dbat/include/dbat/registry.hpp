#pragma once
#include <cstddef>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

/*
 * Entities numbered by vnum: shops, guilds, zones, rooms and prototypes. Each one stays at the same address until it is
 * erased, so a pointer to one can be kept, and they are visited in vnum order.
 *
 *   for (shop_data *shop : g->shops.all()) { ... }
 *
 * all() takes a snapshot of the vnums when the loop starts, then passes over any erased before its turn.
 */
template <typename T, typename V> class vnum_table {
  std::map<V, std::unique_ptr<T>> items_;
  /* The same entities by vnum, for lookups that cost one load: find() runs millions of times a minute, and a map is
   * slow in unoptimised builds. Vnums past DENSE_LIMIT are only in the map. */
  std::vector<T *> dense_;
  static constexpr V DENSE_LIMIT = 1 << 20;

  void index(V vnum, T *item)
  {
    if (vnum < 0 || vnum >= DENSE_LIMIT)
      return;
    if (static_cast<std::size_t>(vnum) >= dense_.size())
      dense_.resize(static_cast<std::size_t>(vnum) + 1, nullptr);
    dense_[static_cast<std::size_t>(vnum)] = item;
  }

public:
  /* The entity numbered vnum, or null. */
  T *find(V vnum) const
  {
    auto i = static_cast<std::size_t>(vnum); /* a negative vnum becomes too large */
    if (i < dense_.size())
      return dense_[i];
    if (vnum >= 0 && vnum < DENSE_LIMIT)
      return nullptr;
    auto it = items_.find(vnum);
    return it == items_.end() ? nullptr : it->second.get();
  }

  /* The entity numbered vnum, made value-initialized if there wasn't one. */
  T *emplace(V vnum)
  {
    std::unique_ptr<T> &slot = items_[vnum];
    if (!slot) {
      slot = std::make_unique<T>();
      index(vnum, slot.get());
    }
    return slot.get();
  }

  /* Adds item as vnum, which must be free: check with find() first. */
  T *insert(V vnum, std::unique_ptr<T> item)
  {
    auto [it, added] = items_.try_emplace(vnum, std::move(item));
    if (!added)
      return nullptr;
    index(vnum, it->second.get());
    return it->second.get();
  }

  /* Removes the entity numbered vnum, handing it to the caller. */
  std::unique_ptr<T> take(V vnum)
  {
    auto it = items_.find(vnum);
    if (it == items_.end())
      return nullptr;
    std::unique_ptr<T> taken = std::move(it->second);
    items_.erase(it);
    index(vnum, nullptr);
    return taken;
  }

  /* The neighbours of vnum in vnum order, or null at either end. */
  T *before(V vnum) const
  {
    auto it = items_.lower_bound(vnum);
    return it == items_.begin() ? nullptr : std::prev(it)->second.get();
  }
  T *after(V vnum) const
  {
    auto it = items_.upper_bound(vnum);
    return it == items_.end() ? nullptr : it->second.get();
  }

  /* The vnum in place n of vnum order, from 0; n must be less than size(). */
  V nth(std::size_t n) const { return std::next(items_.begin(), static_cast<std::ptrdiff_t>(n))->first; }

  /* vnum's place in vnum order, from 0 (what its index in a sorted array would have been), or -1 */
  int place(V vnum) const
  {
    auto it = items_.find(vnum);
    return it == items_.end() ? -1 : static_cast<int>(std::distance(items_.begin(), it));
  }

  void clear()
  {
    items_.clear();
    dense_.clear();
  }
  std::size_t size() const { return items_.size(); }
  bool empty() const { return items_.empty(); }

  class members {
    const vnum_table *table_;
    std::vector<V> vnums_;

  public:
    explicit members(const vnum_table *table) : table_(table)
    {
      vnums_.reserve(table->items_.size());
      for (const auto &[vnum, item] : table->items_)
        vnums_.push_back(vnum);
    }

    class iterator {
      const members *m_{};
      std::size_t i_{};
      T *current_{};

      void settle()
      {
        for (; i_ < m_->vnums_.size(); ++i_)
          if ((current_ = m_->table_->find(m_->vnums_[i_])))
            return;
        current_ = nullptr;
      }

    public:
      iterator(const members *m, std::size_t i) : m_(m), i_(i) { settle(); }
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
    iterator end() const { return {this, vnums_.size()}; }
  };

  /* Calls f(vnum, entity) for each, in order, without taking a snapshot: f must not add or remove entities. For
   * read-only walks that run often, like the audit's. */
  template <typename F>
  void visit(F &&f) const
  {
    for (const auto &[vnum, item] : items_)
      f(vnum, item.get());
  }

  /* The vnums from lo to hi that are in use, in order. */
  std::vector<V> vnums_in(V lo, V hi) const
  {
    std::vector<V> out;
    for (auto it = items_.lower_bound(lo); it != items_.end() && it->first <= hi; ++it)
      out.push_back(it->first);
    return out;
  }

  /* Every vnum, in order, as they are now. */
  std::vector<V> vnums() const
  {
    std::vector<V> out;
    out.reserve(items_.size());
    for (const auto &[vnum, item] : items_)
      out.push_back(vnum);
    return out;
  }

  /* Every entity, in vnum order. */
  members all() const { return members(this); }
};

/*
 * Entities that come and go while the game runs, numbered by id: characters and objects. The table doesn't own
 * them. They are kept in the order they were added and visited newest first, as the old lists were, because that
 * order reaches the save files and what players see.
 */
template <typename T>
class id_table {
  struct entry {
    long id{};
    T *item{}; /* null once removed */
  };
  /* The index from id to place in order_, an open-addressing hash on plain arrays: lookups run millions of times a
   * minute, and std::unordered_map is slow in unoptimised builds. Ids are positive; 0 marks a free slot and -1 one
   * whose id was removed. */
  struct slot {
    long id{};
    std::size_t place{};
  };
  std::unique_ptr<slot[]> slots_;
  std::size_t capacity_{}; /* a power of two, or 0 */
  std::size_t occupied_{}; /* slots in use or removed */
  std::size_t live_{};
  std::vector<entry> order_; /* oldest first */
  std::size_t removed_{};

  static std::size_t hash(long id)
  {
    auto h = static_cast<std::size_t>(id) * 0x9E3779B97F4A7C15ULL;
    return h ^ (h >> 31);
  }

  slot *lookup(long id) const
  {
    if (!capacity_ || id <= 0)
      return nullptr;
    for (std::size_t i = hash(id) & (capacity_ - 1);; i = (i + 1) & (capacity_ - 1)) {
      slot *s = &slots_[i];
      if (s->id == id)
        return s;
      if (s->id == 0)
        return nullptr;
    }
  }

  void put(long id, std::size_t place)
  {
    std::size_t i = hash(id) & (capacity_ - 1);
    while (slots_[i].id > 0)
      i = (i + 1) & (capacity_ - 1);
    occupied_ += slots_[i].id == 0;
    slots_[i] = {id, place};
  }

  /* Rebuilds the index and order_ without removed entries, with room to grow. */
  void rebuild(std::size_t capacity)
  {
    std::size_t out = 0;
    for (const entry &e : order_)
      if (e.item)
        order_[out++] = e;
    order_.resize(out);
    removed_  = 0;
    slots_    = std::make_unique<slot[]>(capacity);
    capacity_ = capacity;
    occupied_ = 0;
    for (std::size_t i = 0; i < order_.size(); i++)
      put(order_[i].id, i);
  }

public:
  /* The entity with this id, or null. */
  T *find(long id) const
  {
    slot *s = lookup(id);
    return s ? order_[s->place].item : nullptr;
  }

  /* Adds item as the newest. False if the id is taken, by item or by another, or isn't positive. */
  bool add(long id, T *item)
  {
    if (id <= 0 || lookup(id))
      return false;
    if ((occupied_ + 1) * 4 > capacity_ * 3)
      rebuild(capacity_ && live_ * 2 < capacity_ ? capacity_ : (capacity_ ? capacity_ * 2 : 1024));
    order_.push_back({id, item});
    put(id, order_.size() - 1);
    live_++;
    return true;
  }

  /* Removes item, if it is the one with this id. */
  bool remove(long id, const T *item)
  {
    slot *s = lookup(id);
    if (!s || order_[s->place].item != item)
      return false;
    order_[s->place].item = nullptr;
    s->id                 = -1;
    live_--;
    if (++removed_ > 64 && removed_ > live_)
      rebuild(capacity_);
    return true;
  }

  std::size_t size() const { return live_; }
  bool empty() const { return live_ == 0; }

  /* Every id, newest first, as they are now. */
  std::vector<long> ids() const
  {
    std::vector<long> out;
    out.reserve(live_);
    const entry *e = order_.data(); /* plain indexing: this runs for every loop over all the entities */
    for (std::size_t i = order_.size(); i-- > 0;)
      if (e[i].item)
        out.push_back(e[i].id);
    return out;
  }

  /* Calls f(id, entity) for each, newest first, without taking a snapshot: f must not add or remove entities. */
  template <typename F>
  void visit(F &&f) const
  {
    const entry *e = order_.data();
    for (std::size_t i = order_.size(); i-- > 0;)
      if (e[i].item)
        f(e[i].id, e[i].item);
  }

  void clear()
  {
    slots_.reset();
    capacity_ = occupied_ = live_ = removed_ = 0;
    order_.clear();
  }
};
