#pragma once
#include "dbat/structs.h"

namespace skill {
    extern std::string getName(SkillID form);

    double getModifier(Character *ch, SkillID skill, int location, int specific = -1);
    double getModifiers(Character *ch, int location, int specific = -1);

}