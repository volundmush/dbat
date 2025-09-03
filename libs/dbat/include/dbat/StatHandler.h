#pragma once
#include "StatDef.h"

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

struct Character;
struct Room;
struct Object;
struct CharacterPrototype;
struct ObjectPrototype;

extern StatHandler<Character> charStats;
extern StatHandler<Room> roomStats;
extern StatHandler<Object> itemStats;
extern StatHandler<CharacterPrototype> npcProtoStats;
extern StatHandler<ObjectPrototype> itemProtoStats;

extern void init_stat_handlers();