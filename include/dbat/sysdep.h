#pragma once
//#include "conf.h"
#include "typestubs.h"

// Some C libraires
#include <cstdio>
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <ctime>
#include <cstddef>
#include <cassert>
#include <sys/stat.h>

#include "dbat/stringutils.h"

#ifndef SIGUSR1
#define SIGUSR1 10
#endif

#ifndef SIGUSR2
#define SIGUSR2 12
#endif

// C++ STD libraries
#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <memory>
#include <optional>
#include <array>
//#include <iostream>
#include <bitset>
#include <functional>
#include <stdexcept>
#include <type_traits>


#define FMT_HEADER_ONLY
//#include "fmt/core.h"
//

//#include "magic_enum/magic_enum_all.hpp"

/* Basic system dependencies *******************************************/
#define IDXTYPE	int
#define NOWHERE -1	/* nil reference for rooms	*/
#define NOTHING NOWHERE
#define NOBODY NOWHERE
#define NOFLAG NOWHERE

#define I64T "ld"
#define SZT "ld"
#define TMT "ld"

/* Various virtual (human-reference) number types. */
typedef IDXTYPE vnum;
typedef vnum room_vnum;
typedef vnum obj_vnum;
typedef vnum mob_vnum;
typedef vnum zone_vnum;
typedef vnum shop_vnum;
typedef vnum trig_vnum;
typedef vnum guild_vnum;

/* Various real (array-reference) number types. */
typedef vnum room_rnum;
typedef vnum obj_rnum;
typedef vnum mob_rnum;
typedef vnum zone_rnum;
typedef vnum shop_rnum;
typedef vnum trig_rnum;
typedef vnum guild_rnum;

/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 */
typedef uint32_t bitvector_t;

typedef void(*CommandFunc)(struct char_data *ch, char *argument, int cmd, int subcmd);

typedef int(*SpecialFunc)(struct char_data *ch, unit_data *me, int cmd, char *argument);

#define ACMD(name) void (name)(struct char_data *ch, char *argument, int cmd, int subcmd)
#define SPECIAL(name) int (name)(struct char_data *ch, unit_data *me, int cmd, char *argument)

template<typename T = bool>
using OpResult = std::pair<T, std::optional<std::string>>;

extern bool isMigrating;

template<typename Key, typename T, typename... Args>
class NegativeKeyGuardMap : public std::map<Key, T, Args...> {
    static_assert(std::is_arithmetic<Key>::value, "Key must be numeric");

public:
    using Base = std::map<Key, T, Args...>;

    T& operator[](const Key& key) {
        if (key < 0)
            throw std::invalid_argument("Negative keys not allowed in NegativeKeyGuardMap");
        return Base::operator[](key);
    }

    std::pair<typename Base::iterator, bool> insert(const std::pair<Key, T>& value) {
        if (value.first < 0)
            throw std::invalid_argument("Negative keys not allowed in NegativeKeyGuardMap");
        return Base::insert(value);
    }

    typename Base::iterator insert(typename Base::const_iterator hint, const std::pair<Key, T>& value) {
        if (value.first < 0)
            throw std::invalid_argument("Negative keys not allowed in NegativeKeyGuardMap");
        return Base::insert(hint, value);
    }
};

template<typename Key, typename T, typename... Args>
class NegativeKeyGuardUnorderedMap : public std::unordered_map<Key, T, Args...> {
    static_assert(std::is_arithmetic<Key>::value, "Key must be numeric");

public:
    using Base = std::unordered_map<Key, T, Args...>;

    T& operator[](const Key& key) {
        if (key < 0)
            throw std::invalid_argument("Negative keys not allowed in NegativeKeyGuardUnorderedMap");
        return Base::operator[](key);
    }

    std::pair<typename Base::iterator, bool> insert(const std::pair<Key, T>& value) {
        if (value.first < 0)
            throw std::invalid_argument("Negative keys not allowed in NegativeKeyGuardUnorderedMap");
        return Base::insert(value);
    }

    typename Base::iterator insert(typename Base::const_iterator hint, const std::pair<Key, T>& value) {
        if (value.first < 0)
            throw std::invalid_argument("Negative keys not allowed in NegativeKeyGuardUnorderedMap");
        return Base::insert(hint, value);
    }
};