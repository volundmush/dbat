#pragma once
#include "dbat/structs.h"

enum class StatRound {
    NONE = 0,   // No rounding
    UP = 1,     // Round up
    DOWN = 2,   // Round down
    NEAREST = 3 // Round to nearest
};

template<typename T>
class StatDef {
    public:
        StatDef(const std::string& name)
            : name(name) {}

        const std::string& getName() const { return name; }

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

    private:
        std::string name;
        double base_value{0.0};
        std::optional<double> min_base_value{};
        std::optional<double> max_base_value{};
        std::optional<double> min_effective_value{};
        std::optional<double> max_effective_value{};
        // the category ID used for this stat's applies.
        uint64_t applyBase{0}, applyMultiplier{0}, applyPostMultiplier{0};
        // the bits used for this specific stat.
        uint64_t specific{0};
        bool serialize{true}; // Whether to serialize this stat by default
        StatRound roundMode{StatRound::NONE}; // Rounding mode for the stat value
        std::unordered_set<std::string> dependencies; // Other stats that this stat depends on. Beware of circular dependencies!
        std::unordered_set<std::string> tags;
};

template<typename T>
class StatHandler {

    private:
        std::vector<StatDef<T>> stats;

        double getBaseHelper(T* target, const StatDef<T>& stat_def) const {
            if (target == nullptr) {
                return 0.0;
            }
            double out = 0.0;

            if(auto orig = target->base_stats.find(stat_def.getName()); orig != target->base_stats.end()) {
                out = orig->second;
            } else {
                out = stat_def.getBaseValue();
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
            auto &stat = stats.emplace_back(name);
            return stat;
        }

        double getBase(T* target, const std::string& stat_name) const {
            double out = 0.0;

            if(auto st = stats.find(stat_name); st != stats.end()) {
                out = getBaseHelper(target, *st);
            }
            
            return out;
        }

        double getBase(const std::shared_ptr<T>& target, const std::string& stat_name) const {
            if (!target) {
                return 0.0;
            }
            return getBase(target.get(), stat_name);
        }

        double getEffective(T* target, const std::string& stat_name) const {
            double out = 0.0;

            if(auto st = stats.find(stat_name); st != stats.end()) {
                out = getBaseHelper(target, *st);
                // Apply any modifiers or multipliers here if needed.
            }

            return out;
        }

        double getEffective(const std::shared_ptr<T>& target, const std::string& stat_name) const {
            if (!target) {
                return 0.0;
            }
            return getEffective(target.get(), stat_name);
        }

        double setBase(T* target, const std::string& stat_name, double value) {
            if (target == nullptr) {
                return 0.0;
            }

            if(auto st = stats.find(stat_name); st != stats.end()) {
                return setBaseHelper(target, *st, value);
            }

            // If the stat doesn't exist, create it with the given value.
            auto &stat_def = addStat(stat_name);
            stat_def.setBaseValue(value);
            return setBaseHelper(target, stat_def, value);
        }

        double setBase(const std::shared_ptr<T>& target, const std::string& stat_name, double value) {
            if (!target) {
                return 0.0;
            }
            return setBase(target.get(), stat_name, value);
        }

        double modBase(T* target, const std::string& stat_name, double value) {
            if (target == nullptr) {
                return 0.0;
            }

            if(auto st = stats.find(stat_name); st != stats.end()) {
                double current_value = getBaseHelper(target, *st);
                return setBaseHelper(target, *st, current_value + value);
            }

            // If the stat doesn't exist, create it with the given value.
            auto &stat_def = addStat(stat_name);
            stat_def.setBaseValue(value);
            return setBaseHelper(target, stat_def, value);
        }

        double modBase(const std::shared_ptr<T>& target, const std::string& stat_name, double value) {
            if (!target) {
                return 0.0;
            }
            return modBase(target.get(), stat_name, value);
        }
        
};


extern StatHandler<struct char_data> charStats;
extern StatHandler<struct obj_data> itemStats;
extern StatHandler<struct room_data> roomStats;

extern void init_stat_handlers();