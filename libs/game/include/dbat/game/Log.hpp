#pragma once
#include "dbat/log/Log.hpp"
#include <fmt/format.h>
#include <fmt/printf.h>



template <typename... Args>
inline void basic_mud_log_helper_args(std::source_location location, std::string_view fmtstr, Args&&... args)
{
    try
    {
        std::string line = fmt::sprintf(fmtstr, std::forward<Args>(args)...);
        if (!line.empty())
            // Use compile-time-checked formatting to write the final string
            dbat::log::log(location, SPDLOG_LEVEL_INFO, "{}", line);
    }
    catch (const std::exception &e)
    {
        fmt::string_view fmt_view = fmtstr;
        dbat::log::log(std::source_location::current(), SPDLOG_LEVEL_ERROR, "SYSERR: Format error in basic_mud_log: {} (template: {})", e.what(), fmt_view);
    }
}

#define basic_mud_log(...) basic_mud_log_helper_args(std::source_location::current(), __VA_ARGS__)
