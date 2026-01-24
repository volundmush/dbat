#pragma once
#include <cxxabi.h>
#include "json.hpp"
#include <nlohmann/json.hpp>

#include "volcano/util/Enum.hpp"
#include "dbat/game/Flags.hpp"
#include "dbat/game/const/Skill.hpp"

inline std::string demangle(const char* mangled_name) {
    int status = -1;
    char* demangled = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);
    std::string result = (status == 0 && demangled != nullptr) ? demangled : mangled_name;
    std::free(demangled);
    return result;
}

template<typename T>
requires std::is_enum_v<T>
void to_json(json& j, const T& a) {
    j = enchantum::to_string(a);
}

template<typename T>
requires std::is_enum_v<T>
void from_json(const json& j, T& a) {
    auto name = j.get<std::string>();
    auto op = enchantum::cast<T>(name);
    if(op.has_value())
        a = op.value();
    else
        throw std::invalid_argument("Invalid enum value: " + name +
            " for enum type: " + demangle(typeid(T).name()));
}

template <typename Enum, typename Value>
requires std::is_enum_v<Enum>
void to_json(json& j, const std::map<Enum, Value>& m)
{
    j = json::object();
    for (auto const& [key, val] : m) {
        std::string key_str = std::string(enchantum::to_string(key));
        if(key_str.empty()) continue;
        j[key_str] = val; // This calls to_json on 'val' if it’s a type with a known converter
    }
}

template <typename Enum, typename Value>
requires std::is_enum_v<Enum>
void from_json(const json& j, std::map<Enum, Value>& m)
{
    m.clear();
    for (auto const& [key_str, value_json] : j.items()) {
        // Convert string -> Enum
        auto maybe = enchantum::cast<Enum>(key_str);
        if (!maybe.has_value()) {
            if(typeid(Enum) == typeid(Skill)) continue;
            throw std::invalid_argument("Invalid enum key: " + key_str
            + " for enum type: " + demangle(typeid(Enum).name()));
        }
        m[maybe.value()] = value_json.get<Value>();
    }
}

template <typename Enum, typename Value>
requires std::is_enum_v<Enum>
void to_json(json& j, const std::unordered_map<Enum, Value>& m)
{
    j = json::object();
    for (auto const& [key, val] : m) {
        std::string key_str = std::string(enchantum::to_string(key));
        if(key_str.empty()) continue;
        j[key_str] = val; // This calls to_json on 'val' if it’s a type with a known converter
    }
}

template <typename Enum, typename Value>
requires std::is_enum_v<Enum>
void from_json(const json& j, std::unordered_map<Enum, Value>& m)
{
    m.clear();
    for (auto const& [key_str, value_json] : j.items()) {
        // Convert string -> Enum
        auto maybe = enchantum::cast<Enum>(key_str);
        if (!maybe.has_value()) {
            if(typeid(Enum) == typeid(Skill)) continue;
            throw std::invalid_argument("Invalid enum key: " + key_str
                + " for enum type: " + demangle(typeid(Enum).name()));
        }
        m[maybe.value()] = value_json.get<Value>();
    }
}

template <typename Enum>
requires std::is_enum_v<Enum>
void to_json(json& j, const FlagHandler<Enum>& m)
{
    j = json::array();
    for (auto const& key : m.getAll()) {
        std::string key_str = std::string(enchantum::to_string(key));
        if(key_str.empty()) continue;
        j.push_back(key_str); // This calls to_json on 'val' if it’s a type with a known converter
    }
}

template <typename Enum>
requires std::is_enum_v<Enum>
void from_json(const json& j, FlagHandler<Enum>& m)
{
    m.clear();
    for (auto const& key_str : j) {
        // Convert string -> Enum
        auto key = key_str.get<std::string>();
        auto maybe = enchantum::cast<Enum>(key);
        if (!maybe.has_value()) {
            if(typeid(Enum) == typeid(Skill)) continue;
            throw std::invalid_argument("Invalid enum key: " + key
                + " for enum type: " + demangle(typeid(Enum).name()));
        }
        m.set(maybe.value());
    }
}