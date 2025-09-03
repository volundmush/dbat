#pragma once
#include <string>
#include <list>
#include <unordered_map>
#include <memory>

template <typename T>
class SubscriptionManager {
    std::unordered_map<std::string, std::list<std::weak_ptr<T>>> subscriptions;
public:
    // Subscribe an entity to a particular service
    void subscribe(const std::string& service, const std::shared_ptr<T>& thing) {
        subscriptions[service].push_front(thing);
        thing->subscriptions.insert(service);
    }

    void subscribe(const std::string& service, T* thing) {
        subscribe(service, thing->shared_from_this());
    }

    T* first(const std::string& service) {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            for (const auto& weak : it->second) {
                if (auto shared = weak.lock()) {
                    return shared.get();
                }
            }
        }
        return nullptr;
    }

    size_t count(const std::string& service) const {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            return std::count_if(it->second.begin(), it->second.end(), [](const std::weak_ptr<T>& weak) {
                return !weak.expired();
            });
        }
        return 0;
    }

    // Unsubscribe an entity from a particular service
    void unsubscribe(const std::string& service, const std::shared_ptr<T>& thing) {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            it->second.remove_if([thing](const auto& weak) {
                return weak.expired() || weak.lock() == thing;
            });
            if (it->second.empty()) {
                subscriptions.erase(it);
            }
        }
        thing->subscriptions.erase(service);
    }

    void unsubscribe(const std::string& service, T* thing) {
        unsubscribe(service, thing->shared_from_this());
    }

    // Get all entities subscribed to a particular service
    std::vector<std::weak_ptr<T>> all(const std::string& service) const {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            std::vector<std::weak_ptr<T>> out;
            out.reserve(it->second.size());
            std::copy_if(it->second.begin(), it->second.end(), std::back_inserter(out), [](const std::weak_ptr<T>& weak) {
                return !weak.expired();
            });
            out.shrink_to_fit();
            return out;
        }
        return {};
    }

    // Check if an entity is subscribed to a particular service
    bool isSubscribed(const std::string& service, const std::shared_ptr<T>& thing) const {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            auto weak = std::weak_ptr<T>(thing);
            return it->second.find(weak) != it->second.end();
        }
        return false;
    }

    bool isSubscribed(const std::string& service, T* thing) const {
        return isSubscribed(service, thing->shared());
    }

    void unsubscribeFromAll(const std::shared_ptr<T>& thing) {
        for (auto it = subscriptions.begin(); it != subscriptions.end(); ) {
            it->second.remove_if([thing](const std::weak_ptr<T>& weak) {
                return weak.expired() || weak.lock() == thing;
            });
            if (it->second.empty()) {
                it = subscriptions.erase(it); // Erase and get the next iterator
            } else {
                ++it;
            }
        }
        thing->subscriptions.clear();
    }

    void unsubscribeFromAll(T* thing) {
        unsubscribeFromAll(thing->shared());
    }

};