#pragma once
#include <string>
#include <unordered_map>
#include <optional>

struct HasVariables {
    std::unordered_map<std::string, std::string> variables; // Subscriptions to services.

    std::optional<std::string> getVariable(std::string_view key) const;

    void setVariable(std::string_view key, std::string_view value);

    template<typename T>
    void setUID(std::string_view key, T u) {
        variables[std::string(key)] = u->getUID(true);
    }

    bool hasVariable(std::string_view key) const;

    bool eraseVariable(std::string_view key);
};