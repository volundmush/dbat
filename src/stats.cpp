#include "fmt/core.h"
#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/filter.h"
#include "dbat/random.h"

StatHandler<struct char_data> charStats;
StatHandler<struct obj_data> itemStats;
StatHandler<struct room_data> roomStats;

static void init_char_stats_attributes() {
    // attributes...

    static const std::vector<std::tuple<std::string, int>> attributes = {
        {"strength", 1 << 0},
        {"agility", 1 << 1},
        {"intelligence", 1 << 2},
        {"wisdom", 1 << 3},
        {"constitution", 1 << 4},
        {"speed", 1 << 5}
    };

    for (const auto& [name, specific] : attributes) {
        auto &stat = charStats.addStat(name);
        stat.setBaseValue(15.0)
            .setMinBaseValue(1.0)
            .setMaxBaseValue(80.0)
            .setMinEffectiveValue(1.0)
            .setMaxEffectiveValue(150.0)
            .setApplyBase(APPLY_CATTR_BASE)
            .setApplyMultiplier(APPLY_CATTR_MULT)
            .setApplyPostMultiplier(APPLY_CATTR_POST)
            .setSpecific(specific)
            .addTag("attribute")
            ;

        auto train_name = fmt::format("train_{}", name);

        auto &train_stat = charStats.addStat(train_name);
        train_stat
            .setBaseValue(0.0)
            .setMinBaseValue(0.0)
            .setSpecific(specific)
            .addTag("attribute_training")
            ;
    }

}

static void init_char_stats_physics() {
    // physics...

    static const std::vector<std::tuple<std::string, int>> physics = {
        {"height", 1 << 0},
        {"weight", 1 << 1}
    };

    for (const auto& [name, specific] : physics) {
        auto &stat = charStats.addStat(name);
        stat.setBaseValue(0.0)
            .setSpecific(specific)
            .setApplyBase(APPLY_CDIM_BASE)
            .setApplyMultiplier(APPLY_CDIM_MULT)
            .setApplyPostMultiplier(APPLY_CDIM_POST)
            .addTag("physics")
            ;
    }

    // Add more physics stats as needed...
}


static void init_char_stats_money() {
    // money...

    static const std::vector<std::tuple<std::string, int>> money_types = {
        {"money_carried", 1 << 0},
        {"money_bank", 1 << 1}
    };

    for (const auto& [name, specific] : money_types) {
        auto &stat = charStats.addStat(name);
        stat.setBaseValue(0.0)
            .setMinBaseValue(0.0)
            .setSpecific(specific)
            .addTag("money")
            ;
    }
}

static void init_char_stats_advancement() {
    // advancement...

    static const std::vector<std::tuple<std::string, int>> advancement_stats = {
        {"experience", 1 << 0},
        {"skill_train", 1 << 1},
        {"practices", 1 << 2},
        {"upgrade_points", 1 << 3}
    };

    for (const auto& [name, specific] : advancement_stats) {
        auto &stat = charStats.addStat(name);
        stat.setBaseValue(0.0)
            .setMinBaseValue(0.0)
            .setSpecific(specific)
            .setApplyBase(APPLY_CSTAT_BASE)
            .setApplyMultiplier(APPLY_CSTAT_MULT)
            .setApplyPostMultiplier(APPLY_CSTAT_POST)
            .setRoundMode(StatRound::DOWN)
            .addTag("advancement")
            ;
    }
}

static void init_char_stats_alignment() {
    // alignment...

    static const std::vector<std::tuple<std::string, int>> alignment_stats = {
        {"good_evil", 1 << 0},
        {"law_chaos", 1 << 1}
    };

    for (const auto& [name, specific] : alignment_stats) {
        auto &stat = charStats.addStat(name);
        stat.setBaseValue(0.0)
            .setMinBaseValue(-1000.0)
            .setMaxBaseValue(1000.0)
            .setMaxEffectiveValue(1000.0)
            .setMinEffectiveValue(-1000.0)
            .setSpecific(specific)
            .addTag("alignment")
            ;
    }
}

static void init_char_stats_vitals() {
    // vitals...

    static const std::vector<std::tuple<std::string, int>> vitals = {
        {"powerlevel", 1 << 0},
        {"ki", 1 << 1},
        {"stamina", 1 << 2},
        {"lifeforce", 1 << 3}
    };

    for (const auto& [name, specific] : vitals) {
        auto &stat = charStats.addStat(name);
        stat.setBaseValue(800.0)
            .setMinBaseValue(0.0)
            //.setMaxBaseValue(100.0)
            .setSpecific(specific)
            .setApplyBase(APPLY_CVIT_BASE)
            .setApplyMultiplier(APPLY_CVIT_MULT)
            .setApplyPostMultiplier(APPLY_CVIT_POST)
            .addTag("vital")
            ;
    }
}

static void init_char_stats_derived() {
    // derived stats...

    charStats.addStat("carry_capacity")
        .setSerialize(false)
        .setBaseValue(0.0)
        .setMinBaseValue(100.0)
        .setSpecific(1 << 0)
        .setApplyBase(APPLY_CDER_BASE)
        .setApplyMultiplier(APPLY_CDER_MULT)
        .setApplyPostMultiplier(APPLY_CDER_POST)
        .addTag("derived")
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example derived stat calculation
            double out = target->getEffectiveStat("weight") + 100.0;
            out += target->getEffectiveStat("strength") * 50.0;
            out += target->getEffectiveStat("powerlevel") / 200.0;
            return out;
        })
        ;
    
    charStats.addStat("weight_inventory")
        .setSerialize(false)
        .setBaseValue(0.0)
        .addTag("derived")
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example derived stat calculation
            double weight = 0;
            auto objects = target->getObjects();
            for(auto obj : filter_raw(objects)) {
                weight += obj->getTotalWeight();
            }
            return weight;
        })
        ;

    charStats.addStat("weight_equipped")
        .setSerialize(false)
        .setBaseValue(0.0)
        .addTag("derived")
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example derived stat calculation
            double total_weight = 0;

            for (int i = 0; i < NUM_WEARS; i++) {
                if (GET_EQ(target, i)) {
                    total_weight += GET_OBJ_WEIGHT(GET_EQ(target, i));
                }
            }
            return total_weight;
        })
        ;
    
    charStats.addStat("weight_carried")
        .setSerialize(false)
        .setBaseValue(0.0)
        .addTag("derived")
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example derived stat calculation
            return target->getEffectiveStat("weight_inventory") + target->getEffectiveStat("weight_equipped") + (target->carrying ? target->carrying->getEffectiveStat("weight_total") : 0);
        })
        ;
    
    charStats.addStat("weight_total")
        .setSerialize(false)
        .setBaseValue(0.0)
        .addTag("derived")
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example derived stat calculation
            return target->getEffectiveStat("weight") + target->getBaseStat("weight_carried");
        })
        ;

    charStats.addStat("carry_available")
        .setSerialize(false)
        .setBaseValue(0.0)
        .setSpecific(1 << 3)
        .addTag("derived")
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example derived stat calculation
            return target->getEffectiveStat("carry_capacity") - target->getEffectiveStat("weight_carried");
        })
        ;
    
    charStats.addStat("speednar")
        .setSerialize(false)
        .setBaseValue(1.0)
        .addTag("derived")
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example derived stat calculation
            double ratio = target->getEffectiveStat("weight_carried") / target->getEffectiveStat("carry_capacity");
            if (ratio >= 0.05) {
                return std::max(0.01, std::min(1.0, 1.0 - ratio));
            }
            return 1.0;
        })
        ;
    charStats.addStat("burden_current")
        .setSerialize(false)
        .setBaseValue(0.0)
        .addTag("derived")
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example derived stat calculation
            auto total = target->getBaseStat("weight_total");
            total *= target->getLocationEnvironment(ENV_GRAVITY);
            return total;
        })
        ;
    
    charStats.addStat("burden_ratio")
        .setSerialize(false)
        .setBaseValue(0.0)
        .addTag("derived")
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example derived stat calculation
            auto total = target->getBaseStat("burden_current");
            auto max = target->getEffectiveStat("carry_capacity");
            if(max == 0) return 0.0;
            return total / max;
        })
        ;
}

static void init_char_stats_combat() {
    // armor works a little weird because of armor_wishes.
    charStats.addStat("armor_wishes")
        .setBaseValue(0.0)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(100.0)
        .addTag("combat")
        ;
    
    charStats.addStat("armor_innate")
        .setBaseValue(0.0)
        .addTag("combat")
        .setEffectiveFunc([](struct char_data* target, const std::string& stat_name, double base_value) {
            // Example effective stat calculation
            double out = target->getBaseStat("armor_wishes") * 5000.0;
            out += base_value;
            return out;
        })
        ;
    
    // The actual thing used in combat calculations.
    charStats.addStat("armor")
        .setSerialize(false)
        .setBaseValue(0.0)
        .addTag("combat")
        .setApplyBase(APPLY_COMBAT_BASE)
        .setApplyMultiplier(APPLY_COMBAT_MULT)
        .setSpecific(1 << 2)
        .setBaseFunc([](struct char_data* target, const std::string& stat_name) {
            // Example effective stat calculation
            return target->getEffectiveStat("armor_innate");
        })
        ;
        
}

static void init_char_stats_misc() {
    for(const auto &s : {"waitTime", "internalGrowth", "lifetimeGrowth", "overGrowth",
    "spellfail", "armorcheck", "armorcheckall", "charge", "chargeto", "barrier", 
    "radar1", "radar2", "radar3"}) {
        charStats.addStat(s)
        .setBaseValue(0.0)
        .setMinBaseValue(0.0)
        .addTag("misc")
        ;
    }

    charStats.addStat("transBonus")
        .setInitFunc([](struct char_data* target, const std::string& stat_name) {
            if(IS_NPC(target)) return 0.0;
            return Random::get<double>(-0.3, 0.3);
        })
        .setMinBaseValue(-0.3)
        .setMaxBaseValue(0.3)
        .addTag("misc")
        ;
    
    charStats.addStat("suppression")
        .setBaseValue(0.0)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(99.0)
        .addTag("misc")
        ;


    for(const auto &s : {"fish_state", "fish_distance"}) {
        charStats.addStat(s)
        .setSerialize(false)
        .setBaseValue(0.0)
        .setMinBaseValue(0.0)
        .addTag("fishing")
        ;
    }
    
    
}

static void init_char_stats() {
    init_char_stats_attributes();
    init_char_stats_physics();
    init_char_stats_money();
    init_char_stats_advancement();
    init_char_stats_alignment();
    init_char_stats_vitals();
    init_char_stats_derived();
    init_char_stats_combat();
    init_char_stats_misc();
    

}

static void init_item_stats() {

}

static void init_room_stats() {

}

void init_stat_handlers() {
    init_char_stats();
    init_item_stats();
    init_room_stats();
}