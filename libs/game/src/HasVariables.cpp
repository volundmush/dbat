#include "dbat/game/HasVariables.hpp"

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

HasVariables dgGlobalVariables;