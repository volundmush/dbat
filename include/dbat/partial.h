#pragma once
#include <boost/algorithm/string.hpp>

template <typename Iterator, typename Key = std::function<std::string(typename std::iterator_traits<Iterator>::value_type)>>
Iterator partialMatch(
        const std::string& match_text,
        Iterator begin, Iterator end,
        bool exact = false,
        Key key = [](const auto& val){ return std::to_string(val); }
)
{
    // Use a multimap to automatically sort by the transformed key.
    using ValueType = typename std::iterator_traits<Iterator>::value_type;
    std::multimap<std::string, ValueType> sorted_map;
    std::for_each(begin, end, [&](const auto& val) {
        sorted_map.insert({key(val), val});
    });

    for (const auto& pair : sorted_map)
    {
        if (boost::iequals(pair.first, match_text))
        {
            return std::find(begin, end, pair.second);
        }
        else if (!exact && boost::istarts_with(pair.first, match_text))
        {
            return std::find(begin, end, pair.second);
        }
    }
    return end;
}