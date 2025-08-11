#include "fmt/core.h"
#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/filter.h"
#include "dbat/random.h"

StatHandler<Character> charStats;
StatHandler<Object> itemStats;
StatHandler<Room> roomStats;
StatHandler<CharacterPrototype> npcProtoStats;
StatHandler<ObjectPrototype> itemProtoStats;

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
        stat.setInitFunc(15.0)
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
        
        npcProtoStats.addStat(name);

        auto train_name = fmt::format("train_{}", name);

        auto &train_stat = charStats.addStat(train_name);
        train_stat
            .setInitFunc(0.0)
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
        stat.setSpecific(specific)
            .setApplyBase(APPLY_CDIM_BASE)
            .setApplyMultiplier(APPLY_CDIM_MULT)
            .setApplyPostMultiplier(APPLY_CDIM_POST)
            .addTag("physics")
            ;
        
        npcProtoStats.addStat(name);
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
        stat.setMinBaseValue(0.0)
            .setSpecific(specific)
            .addTag("money")
            ;
    }

    npcProtoStats.addStat("money_carried");
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
        stat.setMinBaseValue(0.0)
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

    for (const auto& s : {"good_evil", "law_chaos"}) {
        charStats.addStat(s)
            .setInitFunc(0.0)
            .setMinBaseValue(-1000.0)
            .setMaxBaseValue(1000.0)
            .addTag("alignment")
            ;
        npcProtoStats.addStat(s)
            .setInitFunc(0.0)
            .setMinBaseValue(-1000.0)
            .setMaxBaseValue(1000.0)
            .addTag("alignment")
            ;
    }
}

static void init_char_stats_vitals() {
    // vitals...

    static const std::vector<std::tuple<std::string, int>> vitals = {
        {"health", 1 << 0},
        {"ki", 1 << 1},
        {"stamina", 1 << 2},
        {"lifeforce", 1 << 3}
    };

    for (const auto& [name, specific] : vitals) {
        auto &stat = charStats.addStat(name);
        stat.setInitFunc(800.0)
            .setMinBaseValue(0.0)
            //.setMaxBaseValue(100.0)
            .setSpecific(specific)
            .setApplyBase(APPLY_CVIT_BASE)
            .setApplyMultiplier(APPLY_CVIT_MULT)
            .setApplyPostMultiplier(APPLY_CVIT_POST)
            .addTag("vital")
        
            ;
        charStats.addStat(fmt::format("{}_damage", name))
            .setInitFunc(0.0)
            .setMinBaseValue(0.0)
            .setMaxBaseValue(1.0)
            //.setSpecific(specific)
            .addTag("vital_damage")
            
            ;

        npcProtoStats.addStat(name)
            .setInitFunc(100.0)
            .setMinBaseValue(0.0)
            .addTag("vital")
            ;
    }

    charStats.addStat("lifeforce")
        .setSetterFunc(nullptr)
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            auto lb = GET_LIFEBONUSES(target);

            return (IS_DEMON(target) ? (((GET_MAX_MANA(target) * 0.5) + (GET_MAX_MOVE(target) * 0.5)) * 0.75) + lb
                    : (IS_KONATSU(target) ? (((GET_MAX_MANA(target) * 0.5) + (GET_MAX_MOVE(target) * 0.5)) * 0.85) +
                lb : (GET_MAX_MANA(target) * 0.5) +
                                                                        (GET_MAX_MOVE(target) * 0.5) +
                lb));
        })
        ;

    for (const auto& [statName, recoveryKey] : {
        std::pair{"ki_damage", "characterKiRecovery"},
        std::pair{"stamina_damage", "characterStaminaRecovery"},
        std::pair{"lifeforce_damage", "characterLifeforceRecovery"},
    })
    {
        charStats.addStat(statName)
            .setOnChangeFunc([recoveryKey](Character* target, const std::string& stat_name, double old_value, double new_value) {
                if (new_value > 0.0 && old_value <= 0.0) {
                    characterSubscriptions.subscribe(recoveryKey, target->shared());
                } else if (new_value <= 0.0 && old_value > 0.0) {
                    characterSubscriptions.unsubscribe(recoveryKey, target->shared());
                }
            });
    }

    charStats.addStat("health_damage")
        .setOnChangeFunc([](Character* target, const std::string& stat_name, double old_value, double new_value) {
            // Handle health changes here if needed
            auto lifeperc = GET_LIFEPERC(target);
            auto perc = (1.0 - new_value) * 100.0;
            if(new_value > 0.0 && old_value <= 0.0) {
                characterSubscriptions.subscribe("characterHealthRecovery", target->shared());
                if(!IS_ANDROID(target) && (lifeperc > 0) && (perc < lifeperc)) {
                    characterSubscriptions.subscribe("lifeforceSystem", target->shared());
                }
            } else if(new_value <= 0.0 && old_value > 0.0) {
                characterSubscriptions.unsubscribe("characterHealthRecovery", target->shared());
                if(!IS_ANDROID(target) && (lifeperc > 0) && (perc >= lifeperc)) {
                    characterSubscriptions.unsubscribe("lifeforceSystem", target->shared());
                }
            }
        })
        ;
}

static void init_char_stats_derived() {
    // derived stats...

    charStats.addStat("carry_capacity")
        .setMinBaseValue(100.0)
        .setSpecific(1 << 0)
        .setApplyBase(APPLY_CDER_BASE)
        .setApplyMultiplier(APPLY_CDER_MULT)
        .setApplyPostMultiplier(APPLY_CDER_POST)
        .addTag("derived")
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            // Example derived stat calculation
            double out = target->getEffectiveStat("weight") + 100.0;
            out += target->getEffectiveStat("strength") * 50.0;
            out += target->getEffectiveStat<int64_t>("health") / 200.0;
            return out;
        })
        .setSetterFunc(nullptr)
        ;
    
    charStats.addStat("weight_inventory")
        .addTag("derived")
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            // Example derived stat calculation
            double weight = 0;
            auto objects = target->getObjects();
            for(auto obj : filter_raw(objects)) {
                weight += obj->getEffectiveStat("weight_total");
            }
            return weight;
        })
        .setSetterFunc(nullptr)
        ;
    
    itemStats.addStat("weight_inventory")
        .addTag("derived")
        .setGetterFunc([](Object* target, const std::string& stat_name) {
            // Example derived stat calculation
            double weight = 0;
            auto objects = target->getObjects();
            for(auto obj : filter_raw(objects)) {
                weight += obj->getEffectiveStat("weight_total");
            }
            return weight;
        })
        .setSetterFunc(nullptr)
        ;
    
    itemStats.addStat("weight_total")
        .addTag("derived")
        .setGetterFunc([](Object* target, const std::string& stat_name) {
            // Example derived stat calculation
            return target->getEffectiveStat("weight") + target->getBaseStat("weight_inventory");
        })
        .setSetterFunc(nullptr)
        ;

    charStats.addStat("weight_equipped")
        .addTag("derived")
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            // Example derived stat calculation
            double total_weight = 0;

            for (int i = 0; i < NUM_WEARS; i++) {
                if (GET_EQ(target, i)) {
                    total_weight += GET_OBJ_WEIGHT(GET_EQ(target, i));
                }
            }
            return total_weight;
        })
        .setSetterFunc(nullptr)
        ;
    
    charStats.addStat("weight_carried")
        .addTag("derived")
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            // Example derived stat calculation
            return target->getBaseStat("weight_inventory") + target->getBaseStat("weight_equipped") + (target->carrying ? target->carrying->getEffectiveStat("weight_total") : 0);
        })
        .setSetterFunc(nullptr)
        ;
    
    charStats.addStat("weight_total")
        .addTag("derived")
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            // Example derived stat calculation
            return target->getEffectiveStat("weight") + target->getBaseStat("weight_carried");
        })
        .setSetterFunc(nullptr)
        ;

    charStats.addStat("carry_available")
        .setSpecific(1 << 3)
        .addTag("derived")
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            // Example derived stat calculation
            return target->getEffectiveStat("carry_capacity") - target->getBaseStat("weight_carried");
        })
        .setSetterFunc(nullptr)
        ;
    
    charStats.addStat("speednar")
        .addTag("derived")
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            // Example derived stat calculation
            double ratio = target->getBaseStat("weight_carried") / target->getEffectiveStat("carry_capacity");
            if (ratio >= 0.05) {
                return std::clamp<double>(1.0 - ratio, 0.01, 1.0);
            }
            return 1.0;
        })
        .setSetterFunc(nullptr)
        ;
    
    charStats.addStat("burden_current")
        .addTag("derived")
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            // Example derived stat calculation
            auto total = target->getBaseStat("weight_total");
            total *= target->location.getEnvironment(ENV_GRAVITY);
            return total;
        })
        .setSetterFunc(nullptr)
        ;
    
    charStats.addStat("burden_ratio")
        .addTag("derived")
        .setGetterFunc([](Character* target, const std::string& stat_name) {
            // Example derived stat calculation
            auto total = target->getBaseStat("burden_current");
            auto max = target->getEffectiveStat("carry_capacity");
            if(max == 0) return 0.0;
            return total / max;
        })
        .setSetterFunc(nullptr)
        ;
}

static void init_char_stats_combat() {
    // armor works a little weird because of armor_wishes.
    charStats.addStat("armor_wishes")
        .addTag("combat")
        ;
    
    // The actual thing used in combat calculations.
    charStats.addStat("armor")
        .addTag("combat")
        .setApplyBase(APPLY_COMBAT_BASE)
        .setApplyMultiplier(APPLY_COMBAT_MULT)
        .setSpecific(1 << 2)
        .setPreEffectiveFunc([](Character* target, const std::string& stat_name, double* total) {
            // Example effective stat calculation
            *total += target->getBaseStat("armor_wishes") * 5000.0;
        })
        ;

    npcProtoStats.addStat("armor");
        
}

static void init_char_stats_misc() {
    for(const auto &s : {"internalGrowth", "lifetimeGrowth", "overGrowth",
    "spellfail", "armorcheck", "armorcheckall", "charge", "chargeto", "barrier", 
    "radar1", "radar2", "radar3", "level", "wait", "admin_level", "mystic_melody",
"group_kills", "freeze_level", "invis_level", "wimp_level", "death_type", "altitude",
"listen_room", "last_interest", "last_played", "boosts", "upgrade_points",
"majinize", "majinizer", "death_time", "lasthit", "death_count", "rewtime", "starphase",
"molt_experience", "molt_level", "damage_mod", "pole_bonus", "forgetting_skill", "stupidkiss",
"personality", "mind_linker", "bless_level", "preference", "lifebonus",
"auto_skill_bonus", "regen_rate", "relax_count", "ingest_learned", "gauntlet",
"last_olc_mode", "throws", "gooptime", "mobcharge", "combine"}) {
        charStats.addStat(s)
        .addTag("misc")
        ;
        npcProtoStats.addStat(s);
    }

    charStats.addStat("waitTime")
        .setInitFunc(0.0)
        .setMinBaseValue(0.0)
        .addTag("misc")
        ;
    
    charStats.addStat("life_percent")
        .setInitFunc(75.0)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(100.0)
        .addTag("misc")
        .setOnChangeFunc([](Character* target, const std::string& stat_name, double old_value, double new_value) {
            // Handle life percent changes here if needed
            if(IS_ANDROID(target)) return;
            auto lifeperc = new_value;
            auto perc = target->getCurVitalMeterPercent(CharVital::health) * 100.0;
            if(perc < lifeperc) {
                characterSubscriptions.subscribe("lifeforceSystem", target->shared());
            } else {
                characterSubscriptions.unsubscribe("lifeforceSystem", target->shared());
            }
        })
        ;

    charStats.addStat("absorbs")
        .setInitFunc([](Character* target, const std::string& stat_name) {
            if(target->race == Race::bio_android) return 3.0;
            return 0.0;
        })
        .setMinBaseValue(0.0)
        ;

    charStats.addStat("transBonus")
        .setInitFunc([](Character* target, const std::string& stat_name) {
            if(IS_NPC(target)) return 0.0;
            return Random::get<double>(-0.3, 0.3);
        })
        .setMinBaseValue(-0.3)
        .setMaxBaseValue(0.3)
        .addTag("misc")
        ;
    
    charStats.addStat("speaking")
        .setInitFunc(SKILL_LANG_COMMON);

    for(const auto& s : {"backstab_cooldown", "concentrate_cooldown", "selfdestruct_cooldown"}) {
        charStats.addStat(s)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(10.0)
        .addTag("misc")
        ;
    }
    

    charStats.addStat("position")
        .setInitFunc(8.0)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(8.0)
        ;
    
    charStats.addStat("kaioken")
        .setInitFunc(0.0)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(20.0)
        .addTag("misc")
        ;
    
    charStats.addStat("suppression")
        .setMinBaseValue(0.0)
        .setMaxBaseValue(99.0)
        .addTag("misc")
        ;
    
    charStats.addStat("sleeptime")
        .setInitFunc(8.0)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(8.0)
    ;

    charStats.addStat("food_rejuvenation")
        .setInitFunc(0.0)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(2.0)
        ;
    
    charStats.addStat("fury")
        .setInitFunc(0.0)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(100.0)
        ;
    
    charStats.addStat("tail_growth")
        .setInitFunc(0.0)
        .setMinBaseValue(0.0)
        .setMaxBaseValue(10.0)
        .addTag("misc")
        ;
    
    charStats.addStat("skill_slots")
        .setInitFunc(30.0)
        ;
    
    for(const auto &s : {"was_in_room", "master_id", "hometown", "load_room", "death_room", "listen_direction"}) {
        charStats.addStat(s)
        //.setInitFunc(NOWHERE)
        ;
    }

    for(const auto &s : {"last_tell", "olc_zone"}) {
        charStats.addStat(s)
        .setInitFunc(-1)
        .setMinBaseValue(-1)
        .setMaxBaseValue(9999999999)
        ;
    }
    
    charStats.addStat("aggtimer")
        .setMinBaseValue(0.0)
        .setMaxBaseValue(8.0)
        .setSetterFunc(nullptr) // These are not persistent stats.
        ;

    for(const auto &s : {"fish_state", "fish_distance", "spam", "combo_hits",
    "last_attack", "ping", "speedboost", "arena_watch", "rage_meter"}) {
        charStats.addStat(s)
        .addTag("misc")
        .setSetterFunc(nullptr) // These are not persistent stats.
        ;
    }

    for(const auto &s : {"combo"}) {
        charStats.addStat(s)
        .setInitFunc(-1.0)
        .setSetterFunc(nullptr) // These are not persistent stats.
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
    for(const auto& s : {"health", "max_health", "material", "dc_lock", "dc_hide", "dc_move", "dc_skill",
    "time", "hours", "scroll_level", "spell1", "spell2", "spell3", "max_charges", "charges", "spell",
    "skill", "damage_dice", "damage_size", "damage_type", "critical_type", "critical_range",
"apply_ac", "max_dex_mod", "check", "spell_fail", "scouter_level", "seraf_ink",
"soil_quality", "hit_points", "capacity", "flags", "key", "corpse", "owner",
"head", "right_arm", "left_arm", "right_leg", "left_leg", "language", "how_full", "liquid", "poison",
"foodval", "max_foodval", "psbonus", "poison", "expbonus", "candy_pl", "candy_ki",
"candy_st", "whichattr", "attrchance", "size", "destination_room", "fuel", "fuelcount",
"external_room", "location", "viewport", "default_room", "vehicle_vnum", "speed",
"appear", "read", "write", "erase", "comfort_level", "htank_charge", "mat_goal", "gravity",
"maturity", "max_mature", "water_level", "bait", "level", "weight", "cost", "cost_per_day", "energy", "keycode"}) {
        itemStats.addStat(s)
        .addTag("item")
        ;
        itemProtoStats.addStat(s)
        .addTag("item")
        ;
    }

    itemStats.addStat("scoutfreq")
        .setMaxBaseValue(999.0)
        .setMinBaseValue(0.0)
        .setInitFunc(1.0);


    for(const auto& s : {"scoutfreq", "lload", "kicharge", "kitype", "distance", "foob", "aucter", "curBidder", "aucTime", "bid", "startbid", "posttype"}) {
        itemStats.addStat(s)
        .addTag("item")
        ;
        itemProtoStats.addStat(s)
        .addTag("item")
        ;
    }

    itemStats.addStat("timer")
        .setInitFunc([] (Object* target, const std::string& stat_name) {
            if(target->type_flag == ItemType::portal) {
                return -1.0;
            }
            return 0.0;
        });
    itemProtoStats.addStat("timer")
        .setInitFunc([] (ObjectPrototype* target, const std::string& stat_name) {
            if(target->type_flag == ItemType::portal) {
                return -1.0;
            }
            return 0.0;
        });
}

static void init_room_stats() {

}

void init_stat_handlers() {
    init_char_stats();
    init_item_stats();
    init_room_stats();
}