#pragma once
//#include "conf.h"
#include "typestubs.h"

#define CIRCLE_GNU_LIBC_MEMORY_TRACK    0    /* 0 = off, 1 = on */

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
#include <sys/stat.h>

#ifndef _WIN32
// #include <strings.h>
// #include <bsd/string.h>
#endif
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
#include <memory>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <random>
#include <chrono>
#include <optional>
#include <filesystem>
#include <array>
#include <iostream>
#include <mutex>
#include <bitset>
#include <variant>
#include <functional>
#include <type_traits>

#define FMT_HEADER_ONLY
#include "fmt/core.h"
#include "fmt/printf.h"
#include <boost/algorithm/string.hpp>
#include "nlohmann/json.hpp"
#include "entt/entt.hpp"
#include "effolkronium/random.hpp"
using Random = effolkronium::random_static;
#include <rfl/json.hpp>
#include <rfl.hpp>
#include <magic_enum/magic_enum_all.hpp>

/* Basic system dependencies *******************************************/
#if CIRCLE_GNU_LIBC_MEMORY_TRACK && !defined(HAVE_MCHECK_H)
#error "Cannot use GNU C library memory tracking without <mcheck.h>"
#endif

#define HAS_RLIMIT

#define CIRCLE_UNSIGNED_INDEX 0    /* 0 = signed, 1 = unsigned */

#if CIRCLE_UNSIGNED_INDEX
#define IDXTYPE    uint32_t
#define NOWHERE    ((IDXTYPE)~0)
#else
#define IDXTYPE	int
#define NOWHERE	(-1)	/* nil reference for rooms	*/
#endif

#define NOTHING    NOWHERE
#define NOBODY    NOWHERE
#define NOFLAG  NOWHERE

#define I64T "lld"
#define SZT "lld"
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

typedef void(*CommandFunc)(Character *ch, char *argument, int cmd, int subcmd);

typedef int(*SpecialFunc)(Character *ch, void *me, int cmd, char *argument);

#define ACMD(name) void (name)(Character *ch, char *argument, int cmd, int subcmd)
#define SPECIAL(name) int (name)(Character *ch, void *me, int cmd, char *argument)

template<typename T = bool>
using OpResult = std::pair<T, std::optional<std::string>>;

using weight_t = double;

extern entt::registry reg;

template <typename Iterator, typename Key = std::function<std::string(typename std::iterator_traits<Iterator>::value_type)>>
Iterator partialMatch(
        const std::string& match_text,
        Iterator begin, Iterator end,
        bool exact = false,
        Key key = [](const auto& val){ return std::to_string(val); }
)
{
    // Use a multimap to automatically sort by the transformed key.
    using ValueType = typename std::iterator_traits<Iterator>::value_type;
    std::multimap<std::string, ValueType> sorted_map;
    std::for_each(begin, end, [&](const auto& val) {
        sorted_map.insert({key(val), val});
    });

    for (const auto& pair : sorted_map)
    {
        if (iequals(pair.first, match_text))
        {
            return std::find(begin, end, pair.second);
        }
        else if (!exact && istarts_with(pair.first, match_text))
        {
            return std::find(begin, end, pair.second);
        }
    }
    return end;
}

template<typename T>
using VnumIndex = std::map<vnum, std::list<T*>>;

template <typename T>
void insert_vnum(VnumIndex<T>& index, T* item) {
    auto &idx = index[item->vn];
    idx.push_back(item);
}

template <typename T>
void erase_vnum(VnumIndex<T>& index, T* item) {
    auto find = index.find(item->vn);
    if(find == index.end()) return;
    find->second.remove(item);
    if(find->second.empty()) index.erase(find);
}

template <typename T>
T* get_last_inserted(const VnumIndex<T>& index, vnum vn) {
    auto it = index.find(vn);
    if (it != index.end() && !it->second.empty()) {
        return it->second.back();
    }
    return nullptr;
}

template <typename T>
std::size_t get_vnum_count(const VnumIndex<T>& index, vnum vn) {
    auto it = index.find(vn);
    if (it != index.end()) {
        return it->second.size();
    }
    return 0;
}

template <typename T>
std::list<T*> get_vnum_list(const VnumIndex<T>& index, vnum vn) {
    auto it = index.find(vn);
    if (it != index.end()) {
        return it->second;
    }
    return {};
}

extern bool isMigrating;

using DgResults = std::variant<std::string, GameEntity*>;
using DgHolder = std::variant<std::string, GameEntity*, std::function<DgResults(struct trig_data*, const std::string& field, const std::string& args)>>;

using Event = std::pair<std::string, nlohmann::json>;

class InternedString {
public:
    InternedString(const std::string& txt);
    ~InternedString();
    std::string get() const;
private:
    std::string txt;
};

template <typename Key, typename T>
class DebugMap : public std::unordered_map<Key, T> {
public:
    T& operator[](const Key& key) {
        if (key < 0) {
            throw std::runtime_error("Invalid key");
        }
        return std::unordered_map<Key, T>::operator[](key);
    }
};

extern DebugMap<int64_t, std::pair<time_t, entt::entity>> entities;

template<typename T = GameEntity>
T* getEntity(int64_t uid) {
    auto it = entities.find(uid);
    if (it != entities.end()) {
        return reg.try_get<T>(it->second.second);
    }
    return nullptr;
}



class ObjectID {
    public:
    ObjectID() = default;
    explicit ObjectID(const nlohmann::json& j);
    ObjectID(int64_t id, time_t time) : id(id), time(time) {}
    int64_t getId() const { return id; }
    time_t getTime() const { return time; }
    bool operator==(const ObjectID& rhs) const;
    operator bool() const;
    entt::entity get() const;
    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);
    protected:
    int64_t id{NOTHING};
    time_t time{0};
};

namespace std {
    template <>
    struct hash<ObjectID> {
        std::size_t operator()(const ObjectID& id) const {
            return std::hash<int64_t>()(id.getId());
        }
    };
}

extern void setEntity(ObjectID oid, entt::entity entity);