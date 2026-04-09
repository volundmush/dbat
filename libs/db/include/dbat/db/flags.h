#pragma once
#include "consts/types.h"

int flag_test(const bitvector_t bitvector[], int flag);
void flag_set(bitvector_t bitvector[], int flag, int value);
int flag_toggle(bitvector_t bitvector[], int flag);

size_t sprintbit(bitvector_t vector, const char *names[], char *result, size_t reslen);
size_t sprinttype(int type, const char *names[], char *result, size_t reslen);
size_t sprintbitarray(bitvector_t bitvector[], const char *names[], int maxar, char *result, size_t reslen);

int get_flag_by_name(const char *flag_list[], char *flag_name);
size_t sprinttype(int type, const char *names[], char *result, size_t reslen);

#define Q_FIELD(x)  ((int) (x) / 32)
#define Q_BIT(x)    (1 << ((x) % 32))
 
#define IS_SET_AR(var, bit)       ((var)[Q_FIELD(bit)] & Q_BIT(bit))
#define SET_BIT_AR(var, bit)      ((var)[Q_FIELD(bit)] |= Q_BIT(bit))
#define REMOVE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] &= ~Q_BIT(bit))
#define TOGGLE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] = \
                                   (var)[Q_FIELD(bit)] ^ Q_BIT(bit))
#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) ^= (bit))