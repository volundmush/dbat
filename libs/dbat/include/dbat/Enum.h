#pragma once
#include <unordered_map>
#include <functional>
#include <string>

#include "PartialMatch.h"

#include <magic_enum/magic_enum.hpp>


template<typename FlagEnum, typename MapType = std::unordered_map<std::string, FlagEnum>>
requires std::is_enum_v<FlagEnum>
auto getEnumMap(const std::function<bool(FlagEnum v)>& filter = {}) {
    MapType flag_map;
    for (auto val : magic_enum::enum_values<FlagEnum>())
    {
        if(filter && !filter(val)) continue;
        auto vname = std::string(magic_enum::enum_name(val));
        flag_map[vname] = val;
    }
    return flag_map;
}

template<typename FlagEnum, typename ListType = std::vector<FlagEnum>>
requires std::is_enum_v<FlagEnum>
auto getEnumList(const std::function<bool(FlagEnum v)>& filter = {}) {
    ListType flag_list;
    for (auto val : magic_enum::enum_values<FlagEnum>())
    {
        if(filter && !filter(val)) continue;
        flag_list.emplace_back(val);
    }
    return flag_list;
}

template<typename FlagEnum, typename ListType = std::vector<std::string>>
requires std::is_enum_v<FlagEnum>
auto getEnumNameList(const std::function<bool(FlagEnum v)>& filter = {}) {
    ListType flag_list;
    for (auto val : magic_enum::enum_values<FlagEnum>())
    {
        if(filter && !filter(val)) continue;
        flag_list.emplace_back(magic_enum::enum_name(val));
    }
    return flag_list;
}

template<typename T>
requires std::is_enum_v<T>
Result<T> chooseEnum(std::string_view arg, std::string_view context, const std::function<bool(T)> filter = {}) {
    auto emap = getEnumMap<T>(filter);

    auto res = partialMatch(arg, emap);
    if(!res) {
        return err("No match found for {} '{}'. {}", context, arg, res.error());
    }
    return res.value()->second;
}