#pragma once
#include <regex>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "Result.h"

// ^(\d+)(-(\d+))?$


template<typename T, typename Container = std::vector<T>>
requires std::is_arithmetic_v<T>
Result<Container> parseRanges(std::string_view txt) {
    extern std::regex parseRangeRegex;
    
    Container out;

    std::smatch match;

    // we are given a sequence that looks like 5 7 9 20-25
    // and if given that, should return {5, 7, 9, 20, 21, 22, 23, 24, 25}

    std::vector<std::string> parts;
    boost::split(parts, txt, boost::is_any_of(" "), boost::token_compress_on);

    for (const auto& part : parts) {
        if(!std::regex_search(part, match, parseRangeRegex)) {
            return err("Invalid range part: '{}'", part);
        }
        // get first number...
        int first = std::stoi(match[1].str());
        if (match[3].matched) {
            int last = std::stoi(match[3].str());
            for (int i = first; i <= last; ++i) {
                out.push_back(i);
            }
        } else {
            out.push_back(first);
        }
    }

    return std::move(out);
}

template<typename T = int>
requires std::is_arithmetic_v<T>
Result<T> parseNumber(std::string_view arg, std::string_view context, T min_value = T{}) {
    if (arg.empty()) {
        return err("No {} provided.\r\n", context);
    }

    T value{};
    auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);

    if (ec == std::errc::invalid_argument) {
        return err("Invalid {}: {}\r\n", context, arg);
    } else if (ec == std::errc::result_out_of_range) {
        return err("{} out of range: {}\r\n", context, arg);
    } else if (ptr != arg.data() + arg.size()) {
        return err("Invalid trailing characters in {}: {}\r\n", context, arg);
    }

    if (value < min_value) {
        return err("{} must be at least {}\r\n", context, min_value);
    }

    return value;
}