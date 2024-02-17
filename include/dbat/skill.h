#pragma once
#include "dbat/structs.h"

namespace skill {
    extern std::string getName(SkillID form);

    double getModifier(BaseCharacter *ch, SkillID skill, int location, int specific = -1);
    double getModifiers(BaseCharacter *ch, int location, int specific = -1);

}