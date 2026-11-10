#pragma once
#include <cstdarg>
#include <cstdio>
#include <string_view>

#include "dbat/format.hpp"
#include "dbat/consts/types.hpp"

/* mudlog_printf() verbosity levels, matched against a player's syslog setting */
enum log_level { LOG_OFF = 0,
                 LOG_BRF = 1,
                 LOG_NRM = 2,
                 LOG_CMP = 3 };

void core_dump_real(const char *who, int line);
void log_backtrace(void);
#define core_dump() core_dump_real(__FILE__, __LINE__)

#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
  (((major) << 16) + ((minor) << 8) + (patchlevel))

void log_death_trap(GameWorld *g, struct char_data *ch);
void mud_vlog_printf(const char *format, va_list args) __attribute__((format(printf, 1, 0)));
void mud_log_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
void mudlog_printf(GameWorld *g, int type, int level, int file, const char *str, ...) __attribute__((format(printf, 5, 6)));

/* mud_log_printf() for text as it is */
void mud_log(std::string_view text);
/* mud_log_printf() for text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void mud_log(std::format_string<Args...> fmt, Args &&...args)
{
  mud_log(str_format(fmt, std::forward<Args>(args)...));
}

/* mudlog_printf() for text as it is */
void mudlog(GameWorld *g, int type, int level, int file, std::string_view text);
/* mudlog_printf() for text formatted as std::format does */
template <class... Args>
  requires(sizeof...(Args) > 0)
void mudlog(GameWorld *g, int type, int level, int file, std::format_string<Args...> fmt, Args &&...args)
{
  mudlog(g, type, level, file, str_format(fmt, std::forward<Args>(args)...));
}
