#pragma once
#include <string>
#include <string_view>
#include <memory>
#include <source_location>


namespace mud::log
{

    extern const char* log_levels[];

    // ---- Primary API (fmt-style, compile-time checked) ----
    void log(std::source_location loc, int lvl, const char* fmtstr, ...);

}

#define LTRACE(messg, ...) ::mud::log::log(std::source_location::current(), 0, messg __VA_OPT__(,) __VA_ARGS__)
#define LDEBUG(messg, ...) ::mud::log::log(std::source_location::current(), 1, messg __VA_OPT__(,) __VA_ARGS__)
#define LINFO(messg, ...) ::mud::log::log(std::source_location::current(), 2, messg __VA_OPT__(,) __VA_ARGS__)
#define LWARN(messg, ...) ::mud::log::log(std::source_location::current(), 3, messg __VA_OPT__(,) __VA_ARGS__)
#define LERROR(messg, ...) ::mud::log::log(std::source_location::current(), 4, messg __VA_OPT__(,) __VA_ARGS__)
#define LCRIT(messg, ...) ::mud::log::log(std::source_location::current(), 5, messg __VA_OPT__(,) __VA_ARGS__)

#define basic_mud_log(messg, ...) \
    ::mud::log::log(std::source_location::current(), 2, \
                    messg __VA_OPT__(,) __VA_ARGS__)


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
