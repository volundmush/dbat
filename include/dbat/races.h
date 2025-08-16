#pragma once

#include "structs.h"

// global variables
extern const struct guild_info_type guild_info[6];

// functions
extern void racial_body_parts(Character *ch);

extern void set_height_and_weight_by_race(Character *ch);

extern int invalid_race(Character *ch, Object *obj);

// C++ conversion

namespace race
{

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
    extern bool isValidGenome(Race id);
    extern std::string defaultAppearance(Character *ch, Appearance type);

    extern double getModifier(Character *ch, int location, int specific);
    extern double getModifierExact(Character *ch, int location, int specific);

    extern std::vector<Race> filterRaces(std::function<bool(Race)> func);

}
