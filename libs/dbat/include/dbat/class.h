#pragma once
#include <cstdint>
#include <time.h>
#include <vector>
#include <functional>
#include <string>
#include <optional>

#include "const/Sensei.h"
#include "const/Race.h"

#include "Typedefs.h"

struct Character;
struct Object;

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

extern int highest_skill_value(int level, int type, int skill);

extern time_t birth_age(Character *ch);

namespace sensei
{

    extern std::string getName(Sensei id);
    extern std::string getAbbr(Sensei id);
    extern std::string getStyle(Sensei id);
    extern bool isValidSenseiForRace(Sensei id, Race race);
    extern bool isPlayable(Sensei id);
    extern room_vnum getStartRoom(Sensei id);
    extern room_vnum getLocation(Sensei id);

    extern double getModifier(Character *ch, int location, int specific = 0);
    extern std::vector<Sensei> filterSenseis(std::function<bool(Sensei)> func);
    extern std::optional<Sensei> findSensei(const std::string &arg, const std::function<bool(Sensei)> &func);
    extern bool exists(Sensei id);
}
