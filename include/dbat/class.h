#pragma once

#include "structs.h"
#include "races.h"

// global variables


// functions
extern void do_start(struct char_data *ch);

extern int invalid_class(struct char_data *ch, struct obj_data *obj);

extern int64_t level_exp(struct char_data *ch, int level);

extern int load_levels();

extern void cedit_creation(struct char_data *ch);

extern void advance_level(struct char_data *ch);

extern int8_t ability_mod_value(int abil);

extern int8_t dex_mod_capped(struct char_data *ch);

extern int total_skill_levels(struct char_data *ch, int skill);

extern int highest_skill_value(int level, int type, int skill);

extern time_t birth_age(struct char_data *ch);

namespace sensei {

    extern std::string getName(Sensei id);
    extern std::string getAbbr(Sensei id);
    extern std::string getStyle(Sensei id);
    extern bool isValidSenseiForRace(Sensei id, Race race);
    extern bool isPlayable(Sensei id);
    extern room_vnum getStartRoom(Sensei id);
    extern room_vnum getLocation(Sensei id);

    extern double getModifier(struct char_data *ch, int location, int specific = 0);
    extern std::vector<Sensei> filterSenseis(std::function<bool(Sensei)> func);
    extern std::optional<Sensei> findSensei(const std::string& arg, const std::function<bool(Sensei)>& func);
    extern bool exists(Sensei id);
}
