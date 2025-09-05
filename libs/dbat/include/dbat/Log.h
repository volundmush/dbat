#pragma once
#include <string>
#include <string_view>
#include <memory>
#include <source_location>
#include <fmt/format.h>
#include <fmt/printf.h>

// If you vendor fmt already, prefer using the external fmt in spdlog.
// In CMake: target_compile_definitions(your_target PRIVATE SPDLOG_FMT_EXTERNAL=1)
#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>

namespace mud::log
{

    enum class Level
    {
        trace = SPDLOG_LEVEL_TRACE,
        debug = SPDLOG_LEVEL_DEBUG,
        info = SPDLOG_LEVEL_INFO,
        warn = SPDLOG_LEVEL_WARN,
        error = SPDLOG_LEVEL_ERROR,
        critical = SPDLOG_LEVEL_CRITICAL,
        off = SPDLOG_LEVEL_OFF
    };

    struct Options
    {
        // Where to write logs:
        bool to_console = true;
        bool to_file = true;
        std::string file_path = "logs/mud.log";
        std::size_t max_file_bytes = 5 * 1024 * 1024; // 5MB
        std::size_t max_files = 3;
        bool to_syslog = false; // *nix only

        // Behavior:
        bool async = true;
        Level level = Level::info;
        Level flush_on = Level::warn;
        std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] %v";
        bool enable_backtrace = true;
        int backtrace_lines = 64;
    };

    // Must be called once at startup (safe to call again; it re-initializes).
    void init(const Options &opts = {});

    // Change level at runtime (e.g., from an admin command).
    void set_level(Level lvl);

    // ---- Primary API (fmt-style, compile-time checked) ----
    template <typename... Args>
    inline void log(std::source_location loc, Level lvl,
                    fmt::format_string<Args...> fmtstr,
                    Args &&...args)
    {
        auto *logger = spdlog::default_logger_raw();
        if (!logger) [[unlikely]]
            return;

        // Skip formatting if level is disabled:
        if (!logger->should_log(static_cast<spdlog::level::level_enum>(lvl)))
            return;

        logger->log(
            spdlog::source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()},
            static_cast<spdlog::level::level_enum>(lvl),
            fmtstr, std::forward<Args>(args)...);
    }

    // Runtime format string (keeps your current call sites happy)
    template <typename... Args>
    inline void log_runtime(std::source_location loc, Level lvl,
                            fmt::string_view fmt_str_or_msg,
                            Args &&...args)
    {
        auto *logger = spdlog::default_logger_raw();
        if (!logger) [[unlikely]]
            return;

        if (!logger->should_log(static_cast<spdlog::level::level_enum>(lvl)))
            return;

        logger->log(
            spdlog::source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()},
            static_cast<spdlog::level::level_enum>(lvl),
            fmt::runtime(fmt_str_or_msg),
            std::forward<Args>(args)...);
    }

    // Convenience wrappers
    template <typename... Args>
    inline void trace(fmt::format_string<Args...> s, Args &&...a) { log(std::source_location::current(), Level::trace, s, std::forward<Args>(a)...); }
    template <typename... Args>
    inline void debug(fmt::format_string<Args...> s, Args &&...a) { log(std::source_location::current(), Level::debug, s, std::forward<Args>(a)...); }
    template <typename... Args>
    inline void info(fmt::format_string<Args...> s, Args &&...a) { log(std::source_location::current(), Level::info, s, std::forward<Args>(a)...); }
    template <typename... Args>
    inline void warn(fmt::format_string<Args...> s, Args &&...a) { log(std::source_location::current(), Level::warn, s, std::forward<Args>(a)...); }
    template <typename... Args>
    inline void error(fmt::format_string<Args...> s, Args &&...a) { log(std::source_location::current(), Level::error, s, std::forward<Args>(a)...); }
    template <typename... Args>
    inline void crit(fmt::format_string<Args...> s, Args &&...a) { log(std::source_location::current(), Level::critical, s, std::forward<Args>(a)...); }

    // ---- Compatibility shims ----
}
// Legacy printf-style logger kept intact (formats with fmt::sprintf)
template <typename... Args>
inline void basic_mud_log(fmt::string_view printf_style_fmt, Args &&...args)
{
    try
    {
        std::string line = fmt::sprintf(printf_style_fmt, std::forward<Args>(args)...);
        if (!line.empty())
            // Use compile-time-checked formatting to write the final string
            mud::log::info("{}", line);
    }
    catch (const std::exception &e)
    {
        mud::log::error("SYSERR: Format error in basic_mud_log: {} (template: {})", e.what(), printf_style_fmt);
    }
}

// Your fmt-style variant can forward directly
template <typename... Args>
inline void template_mud_log(fmt::string_view fmt_style_fmt, Args &&...args)
{
    try
    {
        mud::log::info(fmt_style_fmt, std::forward<Args>(args)...);
    }
    catch (const std::exception &e)
    {
        mud::log::error("SYSERR: Format error in template_mud_log: {} (template: {})", e.what(), fmt_style_fmt);
    }
}

// ---- Handy macros (capture source location automatically) ----
// These mirror spdlog's active-level gating if you set SPDLOG_ACTIVE_LEVEL.
#define LTRACE(...) ::mud::log::trace(__VA_ARGS__)
#define LDEBUG(...) ::mud::log::debug(__VA_ARGS__)
#define LINFO(...) ::mud::log::info(__VA_ARGS__)
#define LWARN(...) ::mud::log::warn(__VA_ARGS__)
#define LERROR(...) ::mud::log::error(__VA_ARGS__)
#define LCRIT(...) ::mud::log::crit(__VA_ARGS__)

// Log once or every N helper macros (simple, thread-safe enough for logs)
#define LOG_WARN_ONCE(msg, ...)                \
    do                                         \
    {                                          \
        static std::atomic<bool> _done{false}; \
        if (!_done.exchange(true))             \
            LOG_WARN(msg, ##__VA_ARGS__);      \
    } while (0)

#define LOG_INFO_EVERY_N(n, msg, ...)         \
    do                                        \
    {                                         \
        static std::atomic<unsigned> _cnt{0}; \
        auto _v = _cnt.fetch_add(1) + 1;      \
        if ((_v % (n)) == 0)                  \
            LOG_INFO(msg, ##__VA_ARGS__);     \
    } while (0)
