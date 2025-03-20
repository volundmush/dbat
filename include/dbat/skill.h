#pragma once
#include "dbat/structs.h"

namespace skill {
    extern std::string getName(Skill form);

    double getModifier(struct char_data *ch, Skill skill, int location, int specific = -1);
    double getModifiers(struct char_data *ch, int location, int specific = -1);

}