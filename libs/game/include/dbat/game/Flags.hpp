#pragma once
#include <bitset>
#include <sstream>
#include <unordered_set>
#include <set>
#include <cxxabi.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "dbat/util/Enum.hpp"
#include "Result.hpp"
#include <enchantum/type_name.hpp>
#include <enchantum/fmt_format.hpp>
#include <nlohmann/json.hpp>

template <typename FlagEnum>
    requires std::is_enum_v<FlagEnum>
struct FlagChangeResults
{
    std::unordered_set<FlagEnum> added;
    std::unordered_set<FlagEnum> removed;
    std::unordered_set<std::string> errors;

    std::string printResults()
    {
        std::string result;
        for (const auto &flag : added)
        {
            result += fmt::format("  + {}\r\n", flag);
        }
        for (const auto &flag : removed)
        {
            result += fmt::format("  - {}\r\n", flag);
        }
        for (const auto &error : errors)
        {
            result += fmt::format("  ! {}\r\n", error);
        }
        return result;
    }
};

// Generic helper to get a flag.
template <typename FlagEnum>
    requires std::is_enum_v<FlagEnum>
class FlagHandler
{
private:
    std::bitset<128> bits;

    static constexpr std::size_t idx(FlagEnum e)
    {
        return static_cast<std::size_t>(static_cast<std::underlying_type_t<FlagEnum>>(e));
    }

    static constexpr std::size_t idx_int(int v)
    {
        return static_cast<std::size_t>(v);
    }

    static constexpr bool in_range(std::size_t i)
    {
        return i < 128;
    }

public:
    // Returns true if the flag is set.
    bool get(FlagEnum flag) const
    {
        const auto i = idx(flag);
        return in_range(i) ? bits.test(i) : false;
    }
    bool get(int flag) const
    {
        const auto i = idx_int(flag);
        return in_range(i) ? bits.test(i) : false;
    }

    // Sets the flag if value is true, or clears it if false.
    void set(FlagEnum flag, bool value = true)
    {
        const auto i = idx(flag);
        if (!in_range(i))
            return;
        if (value)
            bits.set(i);
        else
            bits.reset(i);
    }
    void set(int flag, bool value = true)
    {
        const auto i = idx_int(flag);
        if (!in_range(i))
            return;
        if (value)
            bits.set(i);
        else
            bits.reset(i);
    }

    // Toggles the flag and returns the new value.
    bool toggle(FlagEnum flag)
    {
        const auto i = idx(flag);
        if (!in_range(i))
            return false;
        bits.flip(i);
        return bits.test(i);
    }
    bool toggle(int flag)
    {
        const auto i = idx_int(flag);
        if (!in_range(i))
            return false;
        bits.flip(i);
        return bits.test(i);
    }

    // Clears all flags.
    void clear() { bits.reset(); }

    // Bulk helpers
    void set_all(std::initializer_list<FlagEnum> flgs)
    {
        for (auto f : flgs)
            set(f, true);
    }

    // Return all currently set flags as a vector (replaces old getAll()).
    // We iterate known enum values to avoid mapping non-enum holes.
    std::vector<FlagEnum> getAll() const
    {
        std::vector<FlagEnum> out;
        out.reserve(bits.count());
        // If you use magic_enum, you can iterate values reliably:
        // for (auto e : magic_enum::enum_values<FlagEnum>()) { ... }
        // If not, we can scan bits 0..N-1 and static_cast. Assumes values == indices.
        for (std::size_t i = 0; i < 128; ++i)
        {
            if (bits.test(i))
            {
                out.push_back(static_cast<FlagEnum>(static_cast<int>(i)));
            }
        }
        return out;
    }

    std::set<FlagEnum> getAllAsSet() const
    {
        std::set<FlagEnum> out;
        for (std::size_t i = 0; i < 128; ++i)
        {
            if (bits.test(i))
            {
                out.insert(static_cast<FlagEnum>(static_cast<int>(i)));
            }
        }
        return out;
    }

    // bool operator: true if any flags are set.
    explicit operator bool() const { return bits.any(); }

    // operator[] returns whether flag is set.
    bool operator[](FlagEnum flag) const { return get(flag); }
    bool operator[](int flag) const { return get(flag); }

    // Number of flags set.
    std::size_t count() const { return bits.count(); }

    // Optional: expose raw bitset for low-level ops/serialization.
    const std::bitset<128> &raw() const { return bits; }
    std::bitset<128> &raw() { return bits; }

    // Mapping / pretty helpers (assumes you have a formatter for enums)
    std::unordered_map<std::string, FlagEnum> getFlagMap() const
    {
        return dbat::util::getEnumMap<FlagEnum>();
    }

    std::vector<std::string> getAllFlags() const
    {
        std::vector<std::string> names;
        names.reserve(bits.count());
        for (auto f : getAll())
        {
            names.push_back(std::string(enchantum::to_string(f)));
        }
        return names;
    }

    std::string getFlagNames(const std::string &delim = ", ") const
    {
        return fmt::format("{}", fmt::join(getAllFlags(), delim));
    }

    // Parse textual +flag / -flag / flag tokens and apply changes.
    FlagChangeResults<FlagEnum> applyChanges(const std::string &txt)
    {
        FlagChangeResults<FlagEnum> results;
        auto fmap = getFlagMap();

        auto findFlag = [&](const std::string &name)
        {
            return dbat::util::partialMatch(name, fmap, false, [](const auto &pair)
                                { return pair.first; });
        };

        std::istringstream iss(txt);
        std::string token;
        while (iss >> token)
        {
            const bool isMinus = !token.empty() && token[0] == '-';
            const bool isPlus = !token.empty() && token[0] == '+';
            std::string_view core = token;
            if (isMinus || isPlus)
                core.remove_prefix(1);

            if (auto it = findFlag(std::string(core)); it)
            {
                const auto flag = it.value()->second;
                if (isMinus)
                {
                    results.removed.insert(flag);
                    set(flag, false);
                }
                else
                {
                    results.added.insert(flag);
                    set(flag, true);
                }
            }
            else
            {
                results.errors.insert(fmt::format("Unknown flag: {}", token));
            }
        }

        return results;
    }
};


template <std::size_t N>
void to_json(nlohmann::json &j, const std::bitset<N> &bs)
{
    std::vector<std::size_t> set_indexes;
    for (std::size_t i = 0; i < N; ++i)
    {
        if (bs.test(i))
        {
            set_indexes.push_back(i);
        }
    }
    j = set_indexes;
}

template <std::size_t N>
void from_json(const nlohmann::json &j, std::bitset<N> &bs)
{
    bs.reset();
    // Extract the vector of indexes from JSON
    auto indexes = j.get<std::vector<std::size_t>>();
    for (auto idx : indexes)
    {
        if (idx < N)
        { // ensure the index is within range
            bs.set(idx, true);
        }
    }
}

inline std::string demangle(const char* mangled_name) {
    int status = -1;
    char* demangled = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);
    std::string result = (status == 0 && demangled != nullptr) ? demangled : mangled_name;
    std::free(demangled);
    return result;
}

template<typename T>
requires std::is_enum_v<T>
void to_json(nlohmann::json& j, const T& a) {
    j = enchantum::to_string(a);
}

template<typename T>
requires std::is_enum_v<T>
void from_json(const nlohmann::json& j, T& a) {
    auto name = j.get<std::string>();
    auto op = enchantum::cast<T>(name);
    if(op.has_value())
        a = op.value();
    else
        throw std::invalid_argument("Invalid enum value: " + name +
            " for enum type: " + demangle(typeid(T).name()));
}

template <typename Enum, typename Value>
requires std::is_enum_v<Enum>
void to_json(nlohmann::json& j, const std::map<Enum, Value>& m)
{
    j = nlohmann::json::object();
    for (auto const& [key, val] : m) {
        std::string key_str = std::string(enchantum::to_string(key));
        if(key_str.empty()) continue;
        j[key_str] = val; // This calls to_json on 'val' if it’s a type with a known converter
    }
}

template <typename Enum, typename Value>
requires std::is_enum_v<Enum>
void from_json(const nlohmann::json& j, std::map<Enum, Value>& m)
{
    m.clear();
    for (auto const& [key_str, value_json] : j.items()) {
        // Convert string -> Enum
        auto maybe = enchantum::cast<Enum>(key_str);
        if (!maybe.has_value()) {
            //if(typeid(Enum) == typeid(Skill)) continue;
            throw std::invalid_argument("Invalid enum key: " + key_str
            + " for enum type: " + demangle(typeid(Enum).name()));
        }
        m[maybe.value()] = value_json.get<Value>();
    }
}

template <typename Enum, typename Value>
requires std::is_enum_v<Enum>
void to_json(nlohmann::json& j, const std::unordered_map<Enum, Value>& m)
{
    j = nlohmann::json::object();
    for (auto const& [key, val] : m) {
        std::string key_str = std::string(enchantum::to_string(key));
        if(key_str.empty()) continue;
        j[key_str] = val; // This calls to_json on 'val' if it’s a type with a known converter
    }
}

template <typename Enum, typename Value>
requires std::is_enum_v<Enum>
void from_json(const nlohmann::json& j, std::unordered_map<Enum, Value>& m)
{
    m.clear();
    for (auto const& [key_str, value_json] : j.items()) {
        // Convert string -> Enum
        auto maybe = enchantum::cast<Enum>(key_str);
        if (!maybe.has_value()) {
            //if(typeid(Enum) == typeid(Skill)) continue;
            throw std::invalid_argument("Invalid enum key: " + key_str
                + " for enum type: " + demangle(typeid(Enum).name()));
        }
        m[maybe.value()] = value_json.get<Value>();
    }
}

template <typename Enum>
requires std::is_enum_v<Enum>
void to_json(nlohmann::json& j, const FlagHandler<Enum>& m)
{
    j = nlohmann::json::array();
    for (auto const& key : m.getAll()) {
        std::string key_str = std::string(enchantum::to_string(key));
        if(key_str.empty()) continue;
        j.push_back(key_str); // This calls to_json on 'val' if it’s a type with a known converter
    }
}

template <typename Enum>
requires std::is_enum_v<Enum>
void from_json(const nlohmann::json& j, FlagHandler<Enum>& m)
{
    m.clear();
    for (auto const& key_str : j) {
        // Convert string -> Enum
        auto key = key_str.get<std::string>();
        auto maybe = enchantum::cast<Enum>(key);
        if (!maybe.has_value()) {
            //if(typeid(Enum) == typeid(Skill)) continue;
            throw std::invalid_argument("Invalid enum key: " + key
                + " for enum type: " + demangle(typeid(Enum).name()));
        }
        m.set(maybe.value());
    }
}

template <typename FlagEnum>
    requires std::is_enum_v<FlagEnum>
inline std::string format_as(const FlagHandler<FlagEnum>& flags) {
    // e.g. "AffectFlag" (optionally strip namespaces)
    constexpr std::string_view type_name = []{
        std::string_view tn = enchantum::type_name<FlagEnum>;
        if (auto pos = tn.rfind("::"); pos != std::string_view::npos) tn.remove_prefix(pos + 2);
        return tn;
    }();

    if (flags.count() == 0)
        return fmt::format("{}s: <none>", type_name);

    return fmt::format("{}s: [{}]", type_name, fmt::join(flags.getAllFlags(), ", "));
}

template<typename EnumType>
requires std::is_enum_v<EnumType>
Result<std::string> handleFlagOps(FlagHandler<EnumType>& flags, std::string_view arg, std::string_view fieldName) {
    if (arg.empty())
    {
        return fmt::format("Current {}: {}\r\n", fieldName, flags.getFlagNames());
    }
    auto results = flags.applyChanges(std::string(arg));
    return fmt::format("{} changes:\r\n{}", fieldName, results.printResults());
}
