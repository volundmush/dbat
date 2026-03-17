#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include <nlohmann/json_fwd.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

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

void to_json(nlohmann::json& j, const HasVariables& unit);
void from_json(const nlohmann::json& j, HasVariables& unit);

inline std::string format_as(const HasVariables& unit) {
    if(unit.variables.empty()) return "script variables: <none>";
    std::vector<std::string> vars;
    for(const auto& [key, val] : unit.variables) {
        vars.push_back(fmt::format("{}: {}", key, val));
    }
    return fmt::format("script variables: [{}]", fmt::join(vars, ", "));
}

extern HasVariables dgGlobalVariables;
