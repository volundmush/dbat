#pragma once

#include "structs.h"
#include "races.h"

// global variables

extern const char *config_sect[NUM_CONFIG_SECTIONS + 1];
extern const int class_hit_die_size[NUM_CLASSES];

// functions
extern void do_start(Character *ch);

extern int invalid_class(Character *ch, Object *obj);

extern int64_t level_exp(Character *ch, int level);

extern int load_levels();

extern void cedit_creation(Character *ch);

extern void advance_level(Character *ch);

extern int8_t ability_mod_value(int abil);

extern int8_t dex_mod_capped(Character *ch);

extern int total_skill_levels(Character *ch, int skill);

extern int highest_skill_value(int level, int type);

extern time_t birth_age(Character *ch);

namespace sensei {

    extern std::string getName(SenseiID id);
    extern std::string getAbbr(SenseiID id);
    extern std::string getStyle(SenseiID id);
    extern bool isValidSenseiForRace(SenseiID id, RaceID race);
    extern bool isPlayable(SenseiID id);
    extern room_vnum getStartRoom(SenseiID id);
    extern room_vnum getLocation(SenseiID id);

    extern double getModifier(Character *ch, int location, int specific = 0);
    extern std::vector<SenseiID> filterSenseis(std::function<bool(SenseiID)> func);
    extern std::optional<SenseiID> findSensei(const std::string& arg, const std::function<bool(SenseiID)>& func);
    extern bool exists(SenseiID id);
}
