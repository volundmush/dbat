#include "fmt/core.h"
#include "dbat/stats.h"

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
        {"carried", 1 << 0},
        {"bank", 1 << 1}
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

static void init_char_stats() {
    init_char_stats_attributes();
    init_char_stats_physics();
    init_char_stats_money();
    init_char_stats_advancement();
    init_char_stats_alignment();
    

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