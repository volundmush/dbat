#pragma once
#include "dbat/structs.h"

namespace skill {
    extern std::string getName(Skill form);

    double getModifier(Character *ch, Skill skill, int location, int specific = -1);
    double getModifiers(Character *ch, int location, int specific = -1);

}