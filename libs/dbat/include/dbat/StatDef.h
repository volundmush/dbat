#pragma once
#include <optional>
#include <functional>
#include <string>

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

// default key extractor -> std::string

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