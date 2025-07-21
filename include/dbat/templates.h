#pragma once
#include <type_traits>
#include "dbat/defs.h"

enum class StatRound {
    NONE = 0,   // No rounding
    UP = 1,     // Round up
    DOWN = 2,   // Round down
    NEAREST = 3 // Round to nearest
};

template<typename T>
class StatDef {
    std::string name;
    double base_value{0.0};
    std::function<double(T*, const std::string&)> init_func{nullptr};
    std::function<double(T*, const std::string&)> base_func{nullptr};
    std::function<double(T*, const std::string&, double)> effective_func{nullptr};
    std::optional<double> min_base_value{};
    std::optional<double> max_base_value{};
    std::optional<double> min_effective_value{};
    std::optional<double> max_effective_value{};
    // the category ID used for this stat's applies.
    uint64_t applyBase{0}, applyMultiplier{0}, applyPostMultiplier{0}, applyGainMultiplier{0};
    // the bits used for this stat's applies.;
    // the bits used for this specific stat.
    uint64_t specific{0};
    bool serialize{true}; // Whether to serialize this stat by default
    StatRound roundMode{StatRound::NONE}; // Rounding mode for the stat value
    std::unordered_set<std::string> dependencies; // Other stats that this stat depends on. Beware of circular dependencies!
    std::unordered_set<std::string> tags;

    public:
        StatDef(const std::string& name)
            : name(name) {}

        const std::string& getName() const { return name; }

        StatDef<T>& setInitFunc(std::function<double(T*, const std::string&)> func) {
            init_func = std::move(func);
            return *this;
        }

        std::function<double(T*, const std::string&)> getInitFunc() const {
            return init_func;
        }

        StatDef<T>& setBaseFunc(std::function<double(T*, const std::string&)> func) {
            base_func = std::move(func);
            return *this;
        }

        std::function<double(T*, const std::string&)> getBaseFunc() const {
            return base_func;
        }

        StatDef<T>& setEffectiveFunc(std::function<double(T*, const std::string&, double)> func) {
            effective_func = std::move(func);
            return *this;
        }

        std::function<double(T*, const std::string&, double)> getEffectiveFunc() const {
            return effective_func;
        }

        double getBaseValue() const { return base_value; }
        StatDef<T>& setBaseValue(double value) { base_value = value; return *this; }

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

        // Not every stat needs to be serialized. Some might just be temporary values or calculated on-the-fly.
        StatDef<T>& setSerialize(bool value) { serialize = value; return *this; }
        bool getSerialize() const {return serialize; }

        StatDef<T>& setRoundMode(StatRound mode) { roundMode = mode; return *this; }
        StatRound getRoundMode() const { return roundMode; }

        StatDef<T>& addDependency(const std::string& dep) { dependencies.insert(dep); return *this; }
        const std::unordered_set<std::string>& getDependencies() const { return dependencies; }

        StatDef<T>& addTag(const std::string& tag) { tags.insert(tag); return *this; }
        const std::unordered_set<std::string>& getTags() const { return tags; }
};

template<typename T>
class StatHandler {

    private:
        std::unordered_map<std::string, StatDef<T>> stats;

        double getBaseHelper(T* target, const StatDef<T>& stat_def) const {
            if (target == nullptr) {
                return 0.0;
            }

            double out = 0.0;
            if (auto func = stat_def.getBaseFunc(); func) {
                out = func(target, stat_def.getName());
            } else {
                if(auto orig = target->base_stats.find(stat_def.getName()); orig != target->base_stats.end()) {
                    out = orig->second;
                } else {
                    if(auto init_func = stat_def.getInitFunc(); init_func) {
                        out = init_func(target, stat_def.getName());
                        // since this might be random we need to save the result instantly.
                        target->base_stats[stat_def.getName()] = out;
                    } else {
                        out = stat_def.getBaseValue();
                    }
                    
                    
                }
            }
            
            if(auto min = stat_def.getMinBaseValue(); min.has_value() && out < *min) {
                out = *min;
            }
            if(auto max = stat_def.getMaxBaseValue(); max.has_value() && out > *max) {
                out = *max;
            }
            return out;
        }

        double setBaseHelper(T* target, const StatDef<T>& stat_def, double value) {
            if (target == nullptr) {
                return 0.0;
            }

            // Apply min/max constraints
            if(auto min = stat_def.getMinBaseValue(); min.has_value() && value < *min) {
                value = *min;
            }
            if(auto max = stat_def.getMaxBaseValue(); max.has_value() && value > *max) {
                value = *max;
            }

            target->base_stats[stat_def.getName()] = value;
            return value;
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

            if(auto st = stats.find(stat_name); st != stats.end()) {
                out = getBaseHelper(target, st->second);
            }
            
            return static_cast<R>(out);
        }
        
        template<typename R = double>
        R getBase(const std::shared_ptr<T>& target, const std::string& stat_name) const {
            if (!target) {
                return static_cast<R>(0.0);
            }
            return getBase<R>(target.get(), stat_name);
        }

        template<typename R = double>
        R getEffective(T* target, const std::string& stat_name) const {
            double total = 0.0;

            if(auto st = stats.find(stat_name); st != stats.end()) {
                auto &stat = st->second;
                total = getBaseHelper(target, stat);
                if (stat.getEffectiveFunc()) {
                    total = stat.getEffectiveFunc()(target, stat_name, total);
                } else {
                    if(stat.getApplyBase() > 0) {
                        total += target->getAffectModifier(stat.getApplyBase(), stat.getSpecific());
                    }
                    if(stat.getApplyMultiplier() > 0) {
                        total *= (1.0 + target->getAffectModifier(stat.getApplyMultiplier(), stat.getSpecific()));
                    }
                    if(stat.getApplyPostMultiplier() > 0) {
                        total += target->getAffectModifier(stat.getApplyPostMultiplier(), stat.getSpecific());
                    }
                }
            }

            return static_cast<R>(total);
        }

        template<typename R = double>
        R getEffective(const std::shared_ptr<T>& target, const std::string& stat_name) const {
            if (!target) {
                return static_cast<R>(0.0);
            }
            return getEffective<R>(target.get(), stat_name);
        }

        template<typename R = double>
        R setBase(T* target, const std::string& stat_name, double value) {
            if (target == nullptr) {
                return static_cast<R>(0.0);
            }

            if(auto st = stats.find(stat_name); st != stats.end()) {
                return static_cast<R>(setBaseHelper(target, st->second, value));
            }

            // If the stat doesn't exist, create it with the given value.
            auto &stat_def = addStat(stat_name);
            stat_def.setBaseValue(value);
            return static_cast<R>(setBaseHelper(target, stat_def, value));
        }

        template<typename R = double>
        R setBase(const std::shared_ptr<T>& target, const std::string& stat_name, double value) {
            if (!target) {
                return static_cast<R>(0.0);
            }
            return setBase<R>(target.get(), stat_name, value);
        }

        template<typename R = double>
        R modBase(T* target, const std::string& stat_name, double value) {
            if (target == nullptr) {
                return static_cast<R>(0.0);
            }

            if(auto st = stats.find(stat_name); st != stats.end()) {
                double current_value = getBaseHelper(target, st->second);
                return static_cast<R>(setBaseHelper(target, st->second, current_value + value));
            }

            // If the stat doesn't exist, create it with the given value.
            auto &stat_def = addStat(stat_name);
            stat_def.setBaseValue(value);
            return static_cast<R>(setBaseHelper(target, stat_def, value));
        }

        template<typename R = double>
        R modBase(const std::shared_ptr<T>& target, const std::string& stat_name, double value) {
            if (!target) {
                return static_cast<R>(0.0);
            }
            return modBase<R>(target.get(), stat_name, value);
        }

        template<typename R = double>
        R gainBase(T* target, const std::string& stat_name, double value) {
            if (target == nullptr) {
                return static_cast<R>(0.0);
            }

            if(auto st = stats.find(stat_name); st != stats.end()) {
                double value = getBaseHelper(target, st->second);
                // Apply the gain multiplier if it exists
                if (st->second.getApplyGainMultiplier() > 0) {
                    value *= (1.0 + target->getAffectModifier(st->second.getApplyGainMultiplier(), st->second.getSpecific()));
                }
                // Set the new base value with the gain applied

                return static_cast<R>(setBaseHelper(target, st->second, value));
            }
            return static_cast<R>(0.0);
        }

        template<typename R = double>
        R gainBase(const std::shared_ptr<T>& target, const std::string& stat_name, double value) {
            if (!target) {
                return static_cast<R>(0.0);
            }
            return gainBase<R>(target.get(), stat_name, value);
        }

        template<typename R = double>
        R gainBasePercent(T* target, const std::string& stat_name, double percent) {
            if (target == nullptr) {
                return static_cast<R>(0.0);
            }

            if(auto st = stats.find(stat_name); st != stats.end()) {
                double current_value = getBaseHelper(target, st->second);
                double new_value = current_value * percent;
                return static_cast<R>(setBaseHelper(target, st->second, new_value));
            }
            return static_cast<R>(0.0);
        }
        
};


extern StatHandler<struct char_data> charStats;
extern StatHandler<struct obj_data> itemStats;
extern StatHandler<struct room_data> roomStats;

extern void init_stat_handlers();

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
};