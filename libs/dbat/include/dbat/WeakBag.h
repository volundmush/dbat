#pragma once
#include <memory>
#include <list>
#include <vector>

template <class T>
struct WeakBag {
    using weak_type   = std::weak_ptr<T>;
    using shared_type = std::shared_ptr<T>;

    std::list<weak_type> items;

    // Add
    void add(const shared_type& p) {
        if (p) items.push_back(p);
    }

    // Remove (by shared ptr or raw)
    void remove(const shared_type& p) {
        if (!p) return;
        items.remove_if([&](const weak_type& w) {
            auto sp = w.lock();
            return !sp || sp.get() == p.get();
        });
    }

    // Sweep expired entries
    void prune() {
        items.remove_if([](const weak_type& w){ return w.expired(); });
    }

    // Iterate live items (optionally prune as you go)
    template <class F>
    void for_each(F&& f, bool do_prune = true) {
        for (auto it = items.begin(); it != items.end(); ) {
            if (auto sp = it->lock()) {
                f(sp.get());      // pass raw* and shared_ptr<T>
                ++it;
            } else if (do_prune) {
                it = items.erase(it);
            } else {
                ++it;
            }
        }
    }

    // cannot prune, but allows iteration and removals simultaneously.
    template <class F>
    void for_each_safe(F&& f) {
        auto safe_items = items;
        for (auto it = safe_items.begin(); it != safe_items.end(); ) {
            if (auto sp = it->lock()) {
                f(sp.get());      // pass raw* and shared_ptr<T>
                ++it;
            } else {
                ++it;
            }
        }
    }

    template <class F>
    void for_each_shared(F&& f, bool do_prune = true) {
        for (auto it = items.begin(); it != items.end(); ) {
            if (auto sp = it->lock()) {
                f(sp);      // pass raw* and shared_ptr<T>
                ++it;
            } else if (do_prune) {
                it = items.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Find first that matches predicate
    template <class Pred>
    T* find_if(Pred&& pred) {
        for (auto it = items.begin(); it != items.end(); ) {
            if (auto sp = it->lock()) {
                if (pred(sp.get(), sp)) return sp.get();
                ++it;
            } else {
                it = items.erase(it);
            }
        }
        return nullptr;
    }

    // Snapshot (vector of weak_ptrs or shared_ptrs)
    std::vector<weak_type> snapshot_weak(bool prune_expired = true) {
        if (prune_expired) prune();
        return {items.begin(), items.end()};
    }
    std::vector<shared_type> snapshot_shared(bool prune_expired = true) {
        std::vector<shared_type> out;
        out.reserve(items.size());
        for (auto it = items.begin(); it != items.end(); ) {
            if (auto sp = it->lock()) { out.push_back(std::move(sp)); ++it; }
            else if (prune_expired) it = items.erase(it);
            else ++it;
        }
        return out;
    }

    // Count (live)
    std::size_t live_count(bool prune_expired = false) {
        std::size_t n = 0;
        for (auto it = items.begin(); it != items.end(); ) {
            if (auto sp = it->lock()) { ++n; ++it; }
            else if (prune_expired) it = items.erase(it);
            else ++it;
        }
        return n;
    }

    bool empty() const {
        for (const auto& w : items) {
            if (!w.expired()) return false;
        }
        return true;
    }

    void clear() {
        items.clear();
    }

    operator bool() const {
        return !empty();
    }

    T* head() const {
        for (const auto& w : items) {
            if (auto sp = w.lock()) return sp.get();
        }
        return nullptr;
    }
};