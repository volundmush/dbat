#pragma once
#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <fmt/format.h>

template <typename T>
using Result = std::expected<T, std::string>;

// Plain-text error (no format args)
[[nodiscard]] inline std::unexpected<std::string>
err(std::string_view msg) {
    return std::unexpected<std::string>{std::string(msg)};
}

// Formatted error (one or more format args)
template <class... Args>
requires (sizeof...(Args) > 0)
[[nodiscard]] inline std::unexpected<std::string>
err(fmt::format_string<Args...> fmtstr, Args&&... args) {
    return std::unexpected<std::string>{
        fmt::format(fmtstr, std::forward<Args>(args)...)
    };
}