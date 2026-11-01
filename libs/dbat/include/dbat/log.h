#pragma once
#include "dbat/consts/types.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* mudlog() verbosity levels, matched against a player's syslog setting */
enum log_level { LOG_OFF = 0,
                 LOG_BRF = 1,
                 LOG_NRM = 2,
                 LOG_CMP = 3 };

void core_dump_real(const char *who, int line);
void log_backtrace(void);
#define core_dump() core_dump_real(__FILE__, __LINE__)

#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
  (((major) << 16) + ((minor) << 8) + (patchlevel))

void log_death_trap(struct char_data *ch);
void mud_vlog(const char *format, va_list args) __attribute__((format(printf, 1, 0)));
void mud_log(const char *format, ...) __attribute__((format(printf, 1, 2)));
void mudlog(int type, int level, int file, const char *str, ...) __attribute__((format(printf, 4, 5)));

#define CREATE(result, type, number)                                                 \
  do {                                                                               \
    if ((number) * sizeof(type) <= 0)                                                \
      mud_log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__); \
    if (!((result) = (type *)calloc((number), sizeof(type)))) {                      \
      perror("SYSERR: malloc failure");                                              \
      abort();                                                                       \
    }                                                                                \
  } while (0)

#define RECREATE(result, type, number)                                      \
  do {                                                                      \
    if (!((result) = (type *)realloc((result), sizeof(type) * (number)))) { \
      perror("SYSERR: realloc failure");                                    \
      abort();                                                              \
    }                                                                       \
  } while (0)
