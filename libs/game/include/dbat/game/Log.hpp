#pragma once
#include "volcano/log/Log.hpp"
#include <fmt/format.h>

inline void basic_mud_log(std::string_view message)
{
    if (!message.empty())
        volcano::log::log(std::source_location::current(), SPDLOG_LEVEL_INFO, "{}", message);
}

template <typename... Args>
inline void basic_mud_log(fmt::format_string<Args...> fmtstr, Args&&... args)
{
    try
    {
        std::string line = fmt::format(fmtstr, std::forward<Args>(args)...);
        if (!line.empty())
            // Use compile-time-checked formatting to write the final string
            volcano::log::log(std::source_location::current(), SPDLOG_LEVEL_INFO, "{}", line);
    }
    catch (const std::exception &e)
    {
        fmt::string_view fmt_view = fmtstr;
        volcano::log::log(std::source_location::current(), SPDLOG_LEVEL_ERROR, "SYSERR: Format error in basic_mud_log: {} (template: {})", e.what(), fmt_view);
    }
}