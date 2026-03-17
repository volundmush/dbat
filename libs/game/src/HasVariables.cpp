#include "dbat/game/HasVariables.hpp"
#include <nlohmann/json.hpp>

std::optional<std::string> HasVariables::getVariable(std::string_view key) const {
    if(auto it = variables.find(std::string(key)); it != variables.end()) {
        return it->second;
    }
    return std::nullopt;
}

void HasVariables::setVariable(std::string_view key, std::string_view value) {
    variables[std::string(key)] = std::string(value);
}

bool HasVariables::hasVariable(std::string_view key) const {
    return variables.find(std::string(key)) != variables.end();
}

bool HasVariables::eraseVariable(std::string_view key) {
    return variables.erase(std::string(key)) > 0;
}

void to_json(nlohmann::json& j, const HasVariables& unit) {
    if(!unit.variables.empty()) {
        j[+"variables"] = unit.variables;
    }
}

void from_json(const nlohmann::json& j, HasVariables& unit) {
    if(j.contains(+"variables")) {
        j.at(+"variables").get_to(unit.variables);
    }
}

HasVariables dgGlobalVariables;
