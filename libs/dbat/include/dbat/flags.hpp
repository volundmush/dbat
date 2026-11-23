#pragma once
#include <span>
#include <string>
#include <string_view>

#include "dbat/consts/types.hpp"

/* the names of the bits set in vector, each followed by a space but the last ("None " if none); a bit past the end of
 * names is UNDEFINED */
std::string sprintbit(bitvector_t vector, std::span<const std::string_view> names);
/* names[type], or UNDEFINED past the end of names (or for a negative type, quirk: the first name) */
std::string sprinttype(int type, std::span<const std::string_view> names);
/* sprintbit() for an array of maxar bitvectors, read up to the end of names; a set bit whose name is "" is left out */
std::string sprintbitarray(bitvector_t bitvector[], std::span<const std::string_view> names, int maxar);

/* The place of flag_name in flag_list, exactly as it is there, or NOFLAG; the list is read up to an empty name */
int get_flag_by_name(std::span<const std::string_view> flag_list, std::string_view flag_name);

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

