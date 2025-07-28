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

        double getBaseHelper(T* target, const StatDef<T>& stat_def) const {
            auto base = 0.0;
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
                auto new_value = setBaseHelper(target, stat_def, out);
                if(base != new_value && stat_def.getOnChangeFunc()) {
                    // Call the on change function if it exists
                    stat_def.getOnChangeFunc()(target, stat_def.getName(), base, new_value);
                }

                return new_value;
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
            return static_cast<R>(setBaseHelper(target, st, value));
        }

        template<typename R = double>
        R setBase(const std::shared_ptr<T>& target, const std::string& stat_name, double value) {
            return setBase<R>(target.get(), stat_name, value);
        }

        template<typename R = double>
        R modBase(T* target, const std::string& stat_name, double value) {
            auto &st = stats.at(stat_name);
            double current_value = getBaseHelper(target, st);
            return static_cast<R>(setBaseHelper(target, st, current_value + value));
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


extern StatHandler<struct char_data> charStats;
extern StatHandler<struct room_data> roomStats;
extern StatHandler<struct obj_data> itemStats;
extern StatHandler<struct npc_proto_data> npcProtoStats;
extern StatHandler<struct item_proto_data> itemProtoStats;

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