#pragma once
#include <type_traits>
#include "dbat/defs.h"

enum class StatRound {
    NONE = 0,   // No rounding
    UP = 1,     // Round up
    DOWN = 2,   // Round down
    NEAREST = 3 // Round to nearest
};

enum class NumberType {
    INTEGER = 0,
    DECIMAL = 1
};

// helper template that uses std::optional to maybe return a value, or a useful error.
struct res_error_t { };
inline constexpr res_error_t Error{};

template <typename T>
struct Result {
    std::optional<T> _value;
    std::string err;

    // ctors
    Result() = default;
    Result(T v) : _value(std::move(v)) {}
    Result(res_error_t, std::string e) : _value(std::nullopt), err(std::move(e)) {}

    // static helpers
    static Result ok(T v) { return Result{std::move(v)}; }
    static Result error(std::string e) { return Result{Error, std::move(e)}; }

    // status
    bool has_value() const noexcept { return _value.has_value(); }
    explicit operator bool() const noexcept { return has_value(); }

    // accessors
    T& value() & { return _value.value(); }
    const T& value() const & { return _value.value(); }
    T&& value() && { return std::move(_value).value(); }

    const std::string& error() const & { return err; }

    // convenience
    T value_or(T def) const { return _value.value_or(std::move(def)); }
};

// Free-function helpers so T can be deduced at call-site
template <class T>
Result<T> Ok(T v) { return Result<T>::ok(std::move(v)); }

// ----- Err builder so you don't have to write Err<int>(...) -----
struct ErrBuilder {
    std::string msg;
    template <class T>
    operator Result<T>() const { return Result<T>::error(msg); }
};

// Variadic Err that formats the message (compile-time checked with fmt)
template <class... Args>
ErrBuilder Err(fmt::format_string<Args...> fmtstr, Args&&... args) {
    return ErrBuilder{ fmt::format(fmtstr, std::forward<Args>(args)...) };
}

// Also allow a plain string (no formatting)
inline ErrBuilder Err(std::string s) { return ErrBuilder{ std::move(s) }; }
inline ErrBuilder Err(const char* s) { return ErrBuilder{ std::string{s} }; }

// default key extractor -> std::string
struct default_key_t {
  template <class T>
  std::string operator()(const T& v) const {
    using U = std::decay_t<T>;
    if constexpr (std::is_same_v<U, std::string>) {
      return v;
    } else if constexpr (std::is_arithmetic_v<U>) {
      return std::to_string(v);
    } else if constexpr (requires { v.first; }) {           // pairs / map values
      return (*this)(v.first);
    } else if constexpr (requires { std::string{v}; }) {    // anything convertible to string
      return std::string{v};
    } else {
      static_assert(sizeof(T) == 0,
        "No default key for this type. Provide a key extractor returning std::string.");
    }
  }
};

// ^(\d+)(-(\d+))?$
extern std::regex parseRangeRegex;

template<typename T, typename Container = std::vector<T>>
requires std::is_arithmetic_v<T>
Result<Container> parseRanges(std::string_view txt) {
    Container out;

    std::smatch match;

    // we are given a sequence that looks like 5 7 9 20-25
    // and if given that, should return {5, 7, 9, 20, 21, 22, 23, 24, 25}

    std::vector<std::string> parts;
    boost::split(parts, txt, boost::is_any_of(" "), boost::token_compress_on);

    for (const auto& part : parts) {
        if(!std::regex_search(part, match, parseRangeRegex)) {
            return Err("Invalid range part: '{}'", part);
        }
        // get first number...
        int first = std::stoi(match[1].str());
        if (match[3].matched) {
            int last = std::stoi(match[3].str());
            for (int i = first; i <= last; ++i) {
                out.push_back(i);
            }
        } else {
            out.push_back(first);
        }
    }

    return Ok(std::move(out));
}

template<typename T = int>
requires std::is_arithmetic_v<T>
Result<T> parseNumber(std::string_view arg, std::string_view context, T min_value = T{}) {
    if (arg.empty()) {
        return Err("No {} provided.\r\n", context);
    }

    T value{};
    auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);

    if (ec == std::errc::invalid_argument) {
        return Err("Invalid {}: {}\r\n", context, arg);
    } else if (ec == std::errc::result_out_of_range) {
        return Err("{} out of range: {}\r\n", context, arg);
    } else if (ptr != arg.data() + arg.size()) {
        return Err("Invalid trailing characters in {}: {}\r\n", context, arg);
    }

    if (value < min_value) {
        return Err("{} must be at least {}\r\n", context, min_value);
    }

    return Ok(value);
}



template <class Range,
          class KeyFn = default_key_t>
auto partialMatch(std::string_view match_text,
                  Range&& range,
                  bool exact = false,
                  KeyFn key = {}) 
  -> Result<std::ranges::iterator_t<Range>>
{
  using std::begin; using std::end;
  using It = std::ranges::iterator_t<Range>;

  // collect (key, iterator) so we can return the iterator directly
  std::vector<std::pair<std::string, It>> items;
  for (It it = begin(range); it != end(range); ++it) {
    items.emplace_back(key(*it), it);
  }

  std::sort(items.begin(), items.end(),
            [](auto& a, auto& b) { return a.first < b.first; });

  for (auto& [k, it] : items) {
    if (boost::iequals(k, match_text) ||
        (!exact && boost::istarts_with(k, match_text))) {
      return Ok(it);
    }
  }

  // build "choices" string
  std::string choices;
  for (size_t i = 0; i < items.size(); ++i) {
    if (i) choices += ", ";
    choices += items[i].first;
  }
  return Err("Choices are: {}\r\n", match_text, choices);
}

template<typename FlagEnum, typename MapType = std::unordered_map<std::string, FlagEnum>>
requires std::is_enum_v<FlagEnum>
auto getEnumMap(const std::function<bool(FlagEnum v)>& filter = {}) {
    MapType flag_map;
    for (auto val : magic_enum::enum_values<FlagEnum>())
    {
        if(filter && !filter(val)) continue;
        auto vname = std::string(magic_enum::enum_name(val));
        flag_map[vname] = val;
    }
    return flag_map;
}

template<typename FlagEnum, typename ListType = std::vector<FlagEnum>>
requires std::is_enum_v<FlagEnum>
auto getEnumList(const std::function<bool(FlagEnum v)>& filter = {}) {
    ListType flag_list;
    for (auto val : magic_enum::enum_values<FlagEnum>())
    {
        if(filter && !filter(val)) continue;
        flag_list.emplace_back(val);
    }
    return flag_list;
}

template<typename FlagEnum, typename ListType = std::vector<std::string>>
requires std::is_enum_v<FlagEnum>
auto getEnumNameList(const std::function<bool(FlagEnum v)>& filter = {}) {
    ListType flag_list;
    for (auto val : magic_enum::enum_values<FlagEnum>())
    {
        if(filter && !filter(val)) continue;
        flag_list.emplace_back(magic_enum::enum_name(val));
    }
    return flag_list;
}

template<typename T>
requires std::is_enum_v<T>
Result<T> chooseEnum(std::string_view arg, std::string_view context, const std::function<bool(T)> filter = {}) {
    auto emap = getEnumMap<T>(filter);

    auto res = partialMatch(arg, emap);
    if(!res) {
        return Err("No match found for {} '{}'. {}", context, arg, res.err);
    }
    return Ok(res.value()->second);
}

template<typename T>
std::optional<double> getterStatFunc(T* obj, const std::string& stat_name) {
    // Use the appropriate method to get the stat value
    if(auto find = obj->stats.find(stat_name); find != obj->stats.end()) {
        return find->second;
    }
    return {};
}

template<typename T>
void setterStatFunc(T* obj, const std::string& stat_name, double value) {
    // Use the appropriate method to set the stat value
    obj->stats[stat_name] = value;
}

template<typename T>
double initStatFunc(T* obj, const std::string& stat_name) {
    return 0.0; // Default initialization, can be overridden
}


template<typename T>
class StatDef {
    std::string name;
    // functions for getting and setting the base stat value to the T* object.
    std::function<std::optional<double>(T*, const std::string&)> getter_function{getterStatFunc<T>};
    // set the setter function to null if you want a derived stat that's not persistent.
    std::function<void(T*, const std::string&, double)> setter_function{setterStatFunc<T>};

    std::function<double(T*, const std::string&)> init_func{initStatFunc<T>};
    std::function<void(T*, const std::string&, double*)> pre_effective_func{nullptr}, post_effective_func{nullptr};
    std::function<void(T*, const std::string&, double*)> pre_gain_func{nullptr}, post_gain_func{nullptr};
    
    std::function<void(T*, const std::string&, double, double)> on_change_func{nullptr};

    // puts caps on the values that'll be output and set.
    std::optional<double> min_base_value{};
    std::optional<double> max_base_value{};
    std::optional<double> min_effective_value{};
    std::optional<double> max_effective_value{};
    // the category ID used for this stat's applies using the APPLY_* defs and old stat system.
    uint64_t applyBase{0}, applyMultiplier{0}, applyPostMultiplier{0}, applyGainMultiplier{0}, specific{0};
    // tags used for searching in the future.
    std::unordered_set<std::string> tags;
    // Type of number this stat represents (integer or decimal). This'll be used for set and dgScripts.
    NumberType numberType{NumberType::DECIMAL};
    StatRound roundMode{StatRound::NONE}; // Rounding mode for the stat value

    public:
        StatDef(const std::string& name)
            : name(name) {}

        const std::string& getName() const { return name; }

        StatDef<T>& setGetterFunc(std::function<std::optional<double>(T*, const std::string&)> func) {
            getter_function = std::move(func);
            return *this;
        }

        std::function<std::optional<double>(T*, const std::string&)> getGetterFunc() const {
            return getter_function;
        }

        StatDef<T>& setSetterFunc(std::function<void(T*, const std::string&, double)> func) {
            setter_function = std::move(func);
            return *this;
        }

        std::function<void(T*, const std::string&, double)> getSetterFunc() const {
            return setter_function;
        }

        // The initfunc is used to initialize the stat value when getterFunc returns no value.
        // the best way to do a "derived stat" is to use the getterFunc, though.
        StatDef<T>& setInitFunc(std::function<double(T*, const std::string&)> func) {
            init_func = std::move(func);
            return *this;
        }
        StatDef<T>& setInitFunc(double value) {
            init_func = [value](T* obj, const std::string&) { return value; };
            return *this;
        }
        std::function<double(T*, const std::string&)> getInitFunc() const {
            return init_func;
        }

        StatDef<T>& setPreEffectiveFunc(std::function<void(T*, const std::string&, double*)> func) {
            pre_effective_func = std::move(func);
            return *this;
        }

        std::function<void(T*, const std::string&, double*)> getPreEffectiveFunc() const {
            return pre_effective_func;
        }

        StatDef<T>& setPostEffectiveFunc(std::function<void(T*, const std::string&, double*)> func) {
            post_effective_func = std::move(func);
            return *this;
        }

        std::function<void(T*, const std::string&, double*)> getPostEffectiveFunc() const {
            return post_effective_func;
        }

        StatDef<T>& setPreGainFunc(std::function<void(T*, const std::string&, double*)> func) {
            pre_gain_func = std::move(func);
            return *this;
        }

        std::function<void(T*, const std::string&, double*)> getPreGainFunc() const {
            return pre_gain_func;
        }

        StatDef<T>& setPostGainFunc(std::function<void(T*, const std::string&, double*)> func) {
            post_gain_func = std::move(func);
            return *this;
        }

        std::function<void(T*, const std::string&, double*)> getPostGainFunc() const {
            return post_gain_func;
        }

        StatDef<T>& setOnChangeFunc(std::function<void(T*, const std::string&, double, double)> func) {
            on_change_func = std::move(func);
            return *this;
        }

        std::function<void(T*, const std::string&, double, double)> getOnChangeFunc() const {
            return on_change_func;
        }

        StatDef<T>& setMinBaseValue(double value) { min_base_value = value; return *this; }
        std::optional<double> getMinBaseValue() const { return min_base_value; }
        
        StatDef<T>& setMaxBaseValue(double value) { max_base_value = value; return *this; }
        std::optional<double> getMaxBaseValue() const { return max_base_value; }

        StatDef<T>& setMinEffectiveValue(double value) { min_effective_value = value; return *this; }
        std::optional<double> getMinEffectiveValue() const { return min_effective_value; }

        StatDef<T>& setMaxEffectiveValue(double value) { max_effective_value = value; return *this; }
        std::optional<double> getMaxEffectiveValue() const { return max_effective_value; }

        StatDef<T>& setApplyBase(uint64_t value) { applyBase = value; return *this; }
        uint64_t getApplyBase() const { return applyBase; }

        StatDef<T>& setApplyMultiplier(uint64_t value) { applyMultiplier = value; return *this; }
        uint64_t getApplyMultiplier() const { return applyMultiplier; }

        StatDef<T>& setApplyPostMultiplier(uint64_t value) { applyPostMultiplier = value; return *this; }
        uint64_t getApplyPostMultiplier() const { return applyPostMultiplier; }

        StatDef<T>& setApplyGainMultiplier(uint64_t value) { applyGainMultiplier = value; return *this; }
        uint64_t getApplyGainMultiplier() const { return applyGainMultiplier; }

        StatDef<T>& setSpecific(uint64_t spec) { specific = spec; return *this; }
        uint64_t getSpecific() const { return specific; }

        StatDef<T>& setRoundMode(StatRound mode) { roundMode = mode; return *this; }
        StatRound getRoundMode() const { return roundMode; }

        StatDef<T>& addTag(const std::string& tag) { tags.insert(tag); return *this; }
        const std::unordered_set<std::string>& getTags() const { return tags; }
};

template<typename T>
class StatHandler {

    private:
        std::unordered_map<std::string, StatDef<T>> stats;

        double setBaseHelper(T* target, const StatDef<T>& stat_def, double value) const {
            // Apply min/max constraints
            if(auto min = stat_def.getMinBaseValue(); min.has_value() && value < *min) {
                value = *min;
            }
            if(auto max = stat_def.getMaxBaseValue(); max.has_value() && value > *max) {
                value = *max;
            }

            if(auto func = stat_def.getSetterFunc(); func) {
                func(target, stat_def.getName(), value);
            }

            return value;
        }

        double setBaseHelperObserver(T* target, const StatDef<T>& stat_def, double value, double old_value) const {
            // Apply min/max constraints
            auto new_value = setBaseHelper(target, stat_def, value);
            if(auto func = stat_def.getOnChangeFunc(); func && (new_value != old_value)) {
                // Call the on change function if it exists
                func(target, stat_def.getName(), old_value, new_value);
            }

            return new_value;
        }

        double getBaseHelper(T* target, const StatDef<T>& stat_def) const {
            double base = 0.0;
            bool needInit = true;
            if(auto func = stat_def.getGetterFunc(); func) {
                auto res = func(target, stat_def.getName());
                if (res.has_value()) {
                    base = *res;
                    needInit = false; // We have a value, no need to initialize.
                }
            }
            auto out = base;
            
            if (auto init_func = stat_def.getInitFunc(); needInit && init_func) {
                out = init_func(target, stat_def.getName());
                // since this might be random we need to save the result instantly.
                // We can return here to avoid repeat min/max checks.
                return setBaseHelperObserver(target, stat_def, out, base);
            }
            
            if(auto min = stat_def.getMinBaseValue(); min.has_value() && out < *min) {
                out = *min;
            }
            if(auto max = stat_def.getMaxBaseValue(); max.has_value() && out > *max) {
                out = *max;
            }
            return out;
        }

    public:

        StatHandler() = default;

        StatDef<T>& addStat(const std::string& name) {
            auto &stat = stats.emplace(name, name).first->second;
            return stat;
        }

        template<typename R = double>
        R getBase(T* target, const std::string& stat_name) const {
            double out = 0.0;
            auto &st = stats.at(stat_name);

            out = getBaseHelper(target, st);
            
            return static_cast<R>(out);
        }
        
        template<typename R = double>
        R getBase(const std::shared_ptr<T>& target, const std::string& stat_name) const {
            return getBase<R>(target.get(), stat_name);
        }

        template<typename R = double>
        R getEffective(T* target, const std::string& stat_name) const {
            double total = 0.0;
            auto &stat = stats.at(stat_name);

            total = getBaseHelper(target, stat);

            if (auto func = stat.getPreEffectiveFunc(); func) {
                func(target, stat_name, &total);
            }

            if(stat.getApplyBase() > 0) {
                total += target->getAffectModifier(stat.getApplyBase(), stat.getSpecific());
            }
            if(stat.getApplyMultiplier() > 0) {
                total *= (1.0 + target->getAffectModifier(stat.getApplyMultiplier(), stat.getSpecific()));
            }
            if(stat.getApplyPostMultiplier() > 0) {
                total += target->getAffectModifier(stat.getApplyPostMultiplier(), stat.getSpecific());
            }

            if (auto func = stat.getPostEffectiveFunc(); func) {
                func(target, stat_name, &total);
            }

            if(auto min = stat.getMinEffectiveValue(); min.has_value() && total < *min) {
                total = *min;
            }
            if(auto max = stat.getMaxEffectiveValue(); max.has_value() && total > *max) {
                total = *max;
            }

            return static_cast<R>(total);
        }

        template<typename R = double>
        R getEffective(const std::shared_ptr<T>& target, const std::string& stat_name) const {
            return getEffective<R>(target.get(), stat_name);
        }

        template<typename R = double>
        R setBase(T* target, const std::string& stat_name, double value) {
            auto &st = stats.at(stat_name);
            double current_value = getBaseHelper(target, st);
            return static_cast<R>(setBaseHelperObserver(target, st, value, current_value));
        }

        template<typename R = double>
        R setBase(const std::shared_ptr<T>& target, const std::string& stat_name, double value) {
            return setBase<R>(target.get(), stat_name, value);
        }

        template<typename R = double>
        R modBase(T* target, const std::string& stat_name, double value) {
            auto &st = stats.at(stat_name);
            double current_value = getBaseHelper(target, st);
            return static_cast<R>(setBaseHelperObserver(target, st, current_value + value, current_value));
        }

        template<typename R = double>
        R modBase(const std::shared_ptr<T>& target, const std::string& stat_name, double value) {
            return modBase<R>(target.get(), stat_name, value);
        }

        template<typename R = double>
        R gainBase(T* target, const std::string& stat_name, double value, bool apply_gain_multiplier = true) {
            auto &st = stats.at(stat_name);
            double base = getBaseHelper(target, st);
            if(apply_gain_multiplier) {
                if (auto func = st.getPreGainFunc(); func) {
                    func(target, stat_name, &value);
                }

                // Apply the gain multiplier if it exists
                if (st.getApplyGainMultiplier() > 0) {
                    value *= (1.0 + target->getAffectModifier(st.getApplyGainMultiplier(), st.getSpecific()));
                }

                // use the post func...
                if (auto func = st.getPostGainFunc(); func) {
                    func(target, stat_name, &value);
                }
            }
            
            // Set the new base value with the gain applied

            return static_cast<R>(setBaseHelper(target, st, base + value));
        }

        template<typename R = double>
        R gainBase(const std::shared_ptr<T>& target, const std::string& stat_name, double value, bool apply_gain_multiplier = true) {
            return gainBase<R>(target.get(), stat_name, value, apply_gain_multiplier);
        }

        template<typename R = double>
        R gainBasePercent(T* target, const std::string& stat_name, double percent, bool apply_gain_multiplier = true) {
            auto &st = stats.at(stat_name);
            double current_value = getBaseHelper(target, st);
            double new_value = current_value * percent;
            return gainBase<R>(target, stat_name, new_value, apply_gain_multiplier);
        }

        template<typename R = double>
        R gainBasePercent(const std::shared_ptr<T>& target, const std::string& stat_name, double percent, bool apply_gain_multiplier = true) {
            return gainBasePercent<R>(target.get(), stat_name, percent, apply_gain_multiplier);
        }
        
};


extern StatHandler<Character> charStats;
extern StatHandler<Room> roomStats;
extern StatHandler<Object> itemStats;
extern StatHandler<CharacterPrototype> npcProtoStats;
extern StatHandler<ObjectPrototype> itemProtoStats;

extern void init_stat_handlers();

template<typename FlagEnum>
requires std::is_enum_v<FlagEnum>
struct FlagChangeResults {
    std::unordered_set<FlagEnum> added;
    std::unordered_set<FlagEnum> removed;
    std::unordered_set<std::string> errors;

    std::string printResults() {
        std::string result;
        for(const auto& flag : added) {
            result += fmt::format("  + {}\r\n", magic_enum::enum_name(flag));
        }
        for(const auto& flag : removed) {
            result += fmt::format("  - {}\r\n", magic_enum::enum_name(flag));
        }
        for(const auto& error : errors) {
            result += fmt::format("  ! {}\r\n", error);
        }
        return result;
    }

};

// Generic helper to get a flag.
template<typename FlagEnum>
requires std::is_enum_v<FlagEnum>
class FlagHandler {
    
private:
    std::unordered_set<FlagEnum> flags;

public:
    // Returns true if the flag is set.
    bool get(FlagEnum flag) const {
        return flags.find(flag) != flags.end();
    }

    bool get(int flag) const {
        return get(static_cast<FlagEnum>(flag));
    }

    // Sets the flag if value is true, or removes it if false.
    void set(FlagEnum flag, bool value = true) {
        if (value)
            flags.insert(flag);
        else
            flags.erase(flag);
    }

    void set(int flag, bool value = true) {
        set(static_cast<FlagEnum>(flag), value);
    }

    // Toggles the flag: if set, removes it and returns false; if not set, inserts it and returns true.
    bool toggle(FlagEnum flag) {
        if (get(flag)) {
            flags.erase(flag);
            return false;
        } else {
            flags.insert(flag);
            return true;
        }
    }

    bool toggle(int flag) {
        return toggle(static_cast<FlagEnum>(flag));
    }

    // Clears all flags.
    void clear() {
        flags.clear();
    }

    // Optionally, expose the underlying container (read-only)
    const std::unordered_set<FlagEnum>& getAll() const {
        return flags;
    }

    // bool operator is true if any flags are set.
    operator bool() const {
        return !flags.empty();
    }

    // operator[] with either int or FlagEnum returns bool if flag set...
    bool operator[](FlagEnum flag) const {
        return get(flag);
    }

    bool operator[](int flag) const {
        return get(static_cast<FlagEnum>(flag));
    }

    std::size_t count() const 
    {
        return flags.size();
    }

    std::unordered_map<std::string, FlagEnum> getFlagMap() const {
        return ::getEnumMap<FlagEnum>();
    }

    std::vector<std::string> getAllFlags() const {
        std::vector<std::string> flag_names;
        for (const auto& flag : flags) {
            flag_names.push_back(fmt::format("{}", flag));
        }
        return flag_names;
    }

    std::string getFlagNames(const std::string& delim = ", ") const {
        return fmt::format("{}", fmt::join(getAllFlags(), delim));
    }

    FlagChangeResults<FlagEnum> applyChanges(const std::string& txt) {
        FlagChangeResults<FlagEnum> results;
        auto fmap = getFlagMap();
        // the input txt looks like:
        // +flag1 -flag2 flag3...
        // If no prefix present, assume +.

        auto findFlag = [&](const std::string& name) {
            return partialMatch(name, fmap, false, [](const auto& pair) {
                return pair.first;
            });
        };

        std::istringstream iss(txt);
        std::string token;
        while (iss >> token) {
            if(token.starts_with("-")) {
                if (auto flag = findFlag(token.substr(1)); flag) {
                    results.removed.insert(flag.value()->second);
                    set(flag.value()->second, false);
                } else {
                    results.errors.insert(fmt::format("Unknown flag: {}", token));
                }
            } else if(token.starts_with("+")) {
                if (auto flag = findFlag(token.substr(1)); flag) {
                    results.added.insert(flag.value()->second);
                    set(flag.value()->second, true);
                } else {
                    results.errors.insert(fmt::format("Unknown flag: {}", token));
                }
            } else {
                if (auto flag = findFlag(token); flag) {
                    results.added.insert(flag.value()->second);
                    set(flag.value()->second, true);
                } else {
                    results.errors.insert(fmt::format("Unknown flag: {}", token));
                }
            }
        }

        return results;
    }
};

// Function to collect pointers to objects within a vnum range from a map
template<typename T>
std::vector<T*> collectObjectsInRange(int start_vnum, int end_vnum, const std::map<int, T>& object_map) {
    std::vector<T*> result;
    
    for (int vnum = start_vnum; vnum <= end_vnum; ++vnum) {
        auto it = object_map.find(vnum);
        if (it != object_map.end()) {
            result.push_back(const_cast<T*>(&it->second));
        }
    }
    
    return result;
}


template <typename T>
class SubscriptionManager {

public:
    // Subscribe an entity to a particular service
    void subscribe(const std::string& service, const std::shared_ptr<T>& thing) {
        subscriptions[service].push_front(thing);
        thing->subscriptions.insert(service);
    }

    void subscribe(const std::string& service, T* thing) {
        subscribe(service, thing->shared_from_this());
    }

    T* first(const std::string& service) {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            for (const auto& weak : it->second) {
                if (auto shared = weak.lock()) {
                    return shared.get();
                }
            }
        }
        return nullptr;
    }

    size_t count(const std::string& service) const {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            return std::count_if(it->second.begin(), it->second.end(), [](const std::weak_ptr<T>& weak) {
                return !weak.expired();
            });
        }
        return 0;
    }

    // Unsubscribe an entity from a particular service
    void unsubscribe(const std::string& service, const std::shared_ptr<T>& thing) {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            it->second.remove_if([thing](const auto& weak) {
                return weak.expired() || weak.lock() == thing;
            });
            if (it->second.empty()) {
                subscriptions.erase(it);
            }
        }
        thing->subscriptions.erase(service);
    }

    void unsubscribe(const std::string& service, T* thing) {
        unsubscribe(service, thing->shared_from_this());
    }

    // Get all entities subscribed to a particular service
    std::vector<std::weak_ptr<T>> all(const std::string& service) const {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            std::vector<std::weak_ptr<T>> out;
            out.reserve(it->second.size());
            std::copy_if(it->second.begin(), it->second.end(), std::back_inserter(out), [](const std::weak_ptr<T>& weak) {
                return !weak.expired();
            });
            out.shrink_to_fit();
            return out;
        }
        return {};
    }

    // Check if an entity is subscribed to a particular service
    bool isSubscribed(const std::string& service, const std::shared_ptr<T>& thing) const {
        auto it = subscriptions.find(service);
        if (it != subscriptions.end()) {
            auto weak = std::weak_ptr<T>(thing);
            return it->second.find(weak) != it->second.end();
        }
        return false;
    }

    bool isSubscribed(const std::string& service, T* thing) const {
        return isSubscribed(service, thing->shared());
    }

    void unsubscribeFromAll(const std::shared_ptr<T>& thing) {
        for (auto it = subscriptions.begin(); it != subscriptions.end(); ) {
            it->second.remove_if([thing](const std::weak_ptr<T>& weak) {
                return weak.expired() || weak.lock() == thing;
            });
            if (it->second.empty()) {
                it = subscriptions.erase(it); // Erase and get the next iterator
            } else {
                ++it;
            }
        }
        thing->subscriptions.clear();
    }

    void unsubscribeFromAll(T* thing) {
        unsubscribeFromAll(thing->shared());
    }

private:
    std::unordered_map<std::string, std::list<std::weak_ptr<T>>> subscriptions;
};

template <class Counter, class Container>
Counter getNextID(Counter& counter, const Container& cont) {
    static_assert(std::is_arithmetic_v<Counter>,
                  "Counter must be a numeric type");
    while(cont.contains(counter)) counter++;
    return counter;
};

template <class T>
struct WeakBag {
    using weak_type   = std::weak_ptr<T>;
    using shared_type = std::shared_ptr<T>;

    std::list<weak_type> items;

    // Add
    void add(const shared_type& p) {
        if (p) items.push_back(p);
    }

    // Remove (by shared ptr or raw)
    void remove(const shared_type& p) {
        if (!p) return;
        items.remove_if([&](const weak_type& w) {
            auto sp = w.lock();
            return !sp || sp.get() == p.get();
        });
    }

    // Sweep expired entries
    void prune() {
        items.remove_if([](const weak_type& w){ return w.expired(); });
    }

    // Iterate live items (optionally prune as you go)
    template <class F>
    void for_each(F&& f, bool do_prune = true) {
        for (auto it = items.begin(); it != items.end(); ) {
            if (auto sp = it->lock()) {
                f(sp.get(), sp);      // pass raw* and shared_ptr<T>
                ++it;
            } else if (do_prune) {
                it = items.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Find first that matches predicate
    template <class Pred>
    T* find_if(Pred&& pred) {
        for (auto it = items.begin(); it != items.end(); ) {
            if (auto sp = it->lock()) {
                if (pred(sp.get(), sp)) return sp.get();
                ++it;
            } else {
                it = items.erase(it);
            }
        }
        return nullptr;
    }

    // Snapshot (vector of weak_ptrs or shared_ptrs)
    std::vector<weak_type> snapshot_weak(bool prune_expired = true) {
        if (prune_expired) prune();
        return {items.begin(), items.end()};
    }
    std::vector<shared_type> snapshot_shared(bool prune_expired = true) {
        std::vector<shared_type> out;
        out.reserve(items.size());
        for (auto it = items.begin(); it != items.end(); ) {
            if (auto sp = it->lock()) { out.push_back(std::move(sp)); ++it; }
            else if (prune_expired) it = items.erase(it);
            else ++it;
        }
        return out;
    }

    // Count (live)
    std::size_t live_count(bool prune_expired = false) {
        std::size_t n = 0;
        for (auto it = items.begin(); it != items.end(); ) {
            if (auto sp = it->lock()) { ++n; ++it; }
            else if (prune_expired) it = items.erase(it);
            else ++it;
        }
        return n;
    }

    bool empty() const {
        for (const auto& w : items) {
            if (!w.expired()) return false;
        }
        return true;
    }
};


