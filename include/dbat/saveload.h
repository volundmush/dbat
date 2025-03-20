#pragma once
#include <filesystem>
#include <cxxabi.h>
#include "dbat/structs.h"
#include "dbat/json.h"
#include "magic_enum/magic_enum_all.hpp"

inline std::string demangle(const char* mangled_name) {
    int status = -1;
    char* demangled = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);
    std::string result = (status == 0 && demangled != nullptr) ? demangled : mangled_name;
    std::free(demangled);
    return result;
}

void runSave();
void load_zones(const std::filesystem::path& loc);
void load_accounts(const std::filesystem::path& loc);
void load_dgscript_prototypes(const std::filesystem::path& loc);

void load_dgscripts_initial(const std::filesystem::path& loc);
void load_dgscripts_finish(const std::filesystem::path& loc);

void load_globaldata(const std::filesystem::path& loc);

void load_shops(const std::filesystem::path& loc);
void load_guilds(const std::filesystem::path& loc);
void load_rooms(const std::filesystem::path& loc);
void load_exits(const std::filesystem::path& loc);

void load_item_prototypes(const std::filesystem::path& loc);
void load_items_initial(const std::filesystem::path& loc);
void load_items_finish(const std::filesystem::path& loc);
void load_npc_prototypes(const std::filesystem::path& loc);
void load_characters_finish(const std::filesystem::path& loc);
void load_characters_initial(const std::filesystem::path& loc);

void load_players(const std::filesystem::path& loc);

template<typename T>
requires std::is_enum_v<T>
void to_json(json& j, const T& a) {
    j = magic_enum::enum_name(a);
}

template<typename T>
requires std::is_enum_v<T>
void from_json(const json& j, T& a) {
    auto name = j.get<std::string>();
    auto op = magic_enum::enum_cast<T>(name);
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
        // Convert Enum -> string via magic_enum
        std::string key_str = std::string(magic_enum::enum_name(key));
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
        auto maybe = magic_enum::enum_cast<Enum>(key_str);
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
        // Convert Enum -> string via magic_enum
        std::string key_str = std::string(magic_enum::enum_name(key));
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
        auto maybe = magic_enum::enum_cast<Enum>(key_str);
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
        // Convert Enum -> string via magic_enum
        std::string key_str = std::string(magic_enum::enum_name(key));
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
        auto maybe = magic_enum::enum_cast<Enum>(key);
        if (!maybe.has_value()) {
            throw std::invalid_argument("Invalid enum key: " + key
                + " for enum type: " + demangle(typeid(Enum).name()));
        }
        m.set(maybe.value());
    }
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(mob_special_data, attack_type, default_pos, damnodice, damsizedice)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(time_data, birth, created, maxage, logon, played, seconds_aged)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(time_info_data, remainder, seconds, minutes, hours, day, month, year)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(weather_data, pressure, change, sky, sunlight)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(shop_buy_data, type, keywords)

void to_json(json& j, const reset_com& r);
void from_json(const json& j, reset_com& r);

void to_json(json& j, const zone_data& z);
void from_json(const json& j, zone_data& z);

void to_json(json& j, const affect_t& a);
void from_json(const json& j, affect_t& a);

void to_json(json& j, const account_data& a);
void from_json(const json& j, account_data& a);

void to_json(json& j, const trig_var_data& t);
void from_json(const json& j, trig_var_data& t);

void to_json(json& j, const trig_data& t);
void from_json(const json& j, trig_data& t);

void to_json(json&j, const shop_data& s);
void from_json(const json& j, shop_data& s);

void to_json(json& j, const guild_data& g);
void from_json(const json& j, guild_data& g);

void to_json(json& j, const unit_data& u);
void from_json(const json& j, unit_data& u);

void to_json(json& j, const room_direction_data &e);
void from_json(const json& j, room_direction_data &e);

void to_json(json& j, const room_data& r);
void from_json(const json& j, room_data& r);

void to_json(json& j, const obj_data& o);
void from_json(const json& j, obj_data& o);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(skill_data, level, perfs)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(alias_data, name, replacement, type)

void to_json(json& j, const trans_data& t);
void from_json(const json& j, trans_data& t);

void to_json(json& j, const affected_type& a);
void from_json(const json& j, affected_type& a);

void to_json(json& j, const char_data& c);
void from_json(const json& j, char_data& c);

void from_json(const json& j, char_data& c);
void to_json(json& j, const char_data& c);

void to_json(json& j, const player_data& p);
void from_json(const json& j, player_data& p);