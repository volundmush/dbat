#pragma once
#include <string>
#include <unordered_map>
#include <optional>

struct HasVariables {
    std::unordered_map<std::string, std::string> variables; // Subscriptions to services.

    std::optional<std::string> getVariable(const std::string &key) const {
        if(auto it = variables.find(key); it != variables.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void setVariable(const std::string &key, const std::string &value) {
        variables[key] = value;
    }

    template<typename T>
    requires (!std::is_convertible_v<T, const char*>)
    void setVariable(const std::string &key, T u) {
        variables[key] = u->getUID(true);
    }

    bool hasVariable(const std::string &key) const {
        return variables.find(key) != variables.end();
    }

    bool eraseVariable(const std::string &key) {
        return variables.erase(key) > 0;
    }
};