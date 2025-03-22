#pragma once

#include "structs.h"

// global variables
extern const struct guild_info_type guild_info[6];

// functions
extern void racial_body_parts(struct char_data *ch);

extern void set_height_and_weight_by_race(struct char_data *ch);

extern int invalid_race(struct char_data *ch, struct obj_data *obj);

// C++ conversion

namespace race {

    extern std::string getName(Race id);
    extern std::string getAbbr(Race id);
    extern bool isPlayable(Race id);
    extern std::vector<Race> getAll();
    extern std::vector<Race> getPlayable();
    extern std::unordered_set<Sex> getValidSexes(Race id);
    extern bool isValidMimic(Race id);
    extern bool isPeople(Race id);
    extern bool hasTail(Race id);
    extern int getRPPCost(Race id);
    extern int getRPPRefund(Race id);
    extern int64_t getSoftCap(Race id, int level);
    extern bool isSenseable(Race id);
    extern int getSize(Race id);
    extern bool exists(Race id);
    extern std::string defaultAppearance(char_data *ch, Appearance type);

    extern double getModifier(struct char_data *ch, int location, int specific);
    extern double getModifierExact(struct char_data *ch, int location, int specific);

    extern std::vector<Race> filterRaces(std::function<bool(Race)> func);
    extern std::optional<Race> findRace(const std::string& arg, std::function<bool(Race)> func);

}
