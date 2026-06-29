#pragma once
#include "consts/types.h"
#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* defines for mudlog() */
#define OFF 0
#define BRF 1
#define NRM 2
#define CMP 3

void core_dump_real(const char *who, int line);
#define core_dump() core_dump_real(__FILE__, __LINE__)

#define CIRCLEMUD_VERSION(major, minor, patchlevel)                            \
  (((major) << 16) + ((minor) << 8) + (patchlevel))

void log_death_trap(struct char_data *ch);
void mud_vlog(const char *format, va_list args);
void mud_log(const char *format, ...)
    __attribute__((format(printf, 1, 2)));
void mudlog(int type, int level, int file, const char *str, ...)
    __attribute__((format(printf, 4, 5)));

void log_imm_action(char *messg, ...) __attribute__((format(printf, 1, 2)));
void log_custom(struct descriptor_data *d, struct obj_data *obj);


#define CREATE(result, type, number)                                           \
  do {                                                                         \
    if ((number) * sizeof(type) <= 0)                                          \
      mud_log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__,          \
          __LINE__);                                                           \
    if (!((result) = (type *)calloc((number), sizeof(type)))) {                \
      perror("SYSERR: malloc failure");                                        \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define RECREATE(result, type, number)                                         \
  do {                                                                         \
    if (!((result) = (type *)realloc((result), sizeof(type) * (number)))) {    \
      perror("SYSERR: realloc failure");                                       \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#ifdef __cplusplus
}
#endif
