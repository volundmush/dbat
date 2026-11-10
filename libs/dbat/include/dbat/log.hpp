#pragma once
#include <cstdarg>
#include <cstdio>

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
