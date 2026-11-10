#pragma once
#include <string>

#include "dbat/consts/types.hpp"

/* the names of the bits set in vector, each followed by a space but the last ("None " if none); a bit past the end of
 * names (ended by "\n") is UNDEFINED */
std::string sprintbit(bitvector_t vector, const char *const names[]);
/* names[type], or UNDEFINED past the end of names */
std::string sprinttype(int type, const char *const names[]);
/* sprintbit() for an array of maxar bitvectors, read up to the end of names; a set bit whose name is "" is left out */
std::string sprintbitarray(bitvector_t bitvector[], const char *const names[], int maxar);

int get_flag_by_name(const char *const flag_list[], char *flag_name);

#define Q_FIELD(x) ((x) / 32)
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

