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

public:
  /* The entity numbered vnum, or null. */
  T *find(V vnum) const
  {
    auto it = items_.find(vnum);
    return it == items_.end() ? nullptr : it->second.get();
  }

  /* The entity numbered vnum, made value-initialized if there wasn't one. */
  T *emplace(V vnum)
  {
    std::unique_ptr<T> &slot = items_[vnum];
    if (!slot)
      slot = std::make_unique<T>();
    return slot.get();
  }

  /* Adds item as vnum, which must be free: check with find() first. */
  T *insert(V vnum, std::unique_ptr<T> item)
  {
    auto [it, added] = items_.try_emplace(vnum, std::move(item));
    return added ? it->second.get() : nullptr;
  }

  /* Removes the entity numbered vnum, handing it to the caller. */
  std::unique_ptr<T> take(V vnum)
  {
    auto it = items_.find(vnum);
    if (it == items_.end())
      return nullptr;
    std::unique_ptr<T> taken = std::move(it->second);
    items_.erase(it);
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

  /* vnum's place in vnum order, from 0 (what its index in a sorted array would have been), or -1 */
  int place(V vnum) const
  {
    auto it = items_.find(vnum);
    return it == items_.end() ? -1 : static_cast<int>(std::distance(items_.begin(), it));
  }

  void clear() { items_.clear(); }
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
