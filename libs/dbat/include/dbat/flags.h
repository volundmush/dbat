#pragma once
#include "consts/types.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t sprintbit(bitvector_t vector, const char *const names[], char *result, size_t reslen);
size_t sprinttype(int type, const char *const names[], char *result, size_t reslen);
size_t sprintbitarray(bitvector_t bitvector[], const char *const names[], int maxar, char *result, size_t reslen);

int get_flag_by_name(const char *const flag_list[], char *flag_name);

#define Q_FIELD(x) ((int)(x) / 32)
#define Q_BIT(x)   (1U << ((x) % 32))

#define IS_SET_AR(var, bit)     ((var)[Q_FIELD(bit)] & Q_BIT(bit))
#define SET_BIT_AR(var, bit)    ((var)[Q_FIELD(bit)] |= Q_BIT(bit))
#define REMOVE_BIT_AR(var, bit) ((var)[Q_FIELD(bit)] &= ~Q_BIT(bit))
#define TOGGLE_BIT_AR(var, bit) ((var)[Q_FIELD(bit)] = \
                                     (var)[Q_FIELD(bit)] ^ Q_BIT(bit))
#define IS_SET(flag, bit)    ((flag) & (bit))
#define SET_BIT(var, bit)    ((var) |= (bit))
#define REMOVE_BIT(var, bit) ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit) ((var) ^= (bit))

#ifdef __cplusplus
}
#endif
