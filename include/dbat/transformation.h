#pragma once
#include "structs.h"

namespace trans {
    enum class FormID {
        // Everyone has a base form, which is their normal form.
        // it has no special properties.
        Base = 0,

        SuperSaiyan = 1,
        SuperSaiyan2 = 2,
        SuperSaiyan3 = 3,
        SuperSaiyan4 = 4,
        SuperSaiyanGod = 5,
        SuperSaiyanBlue = 6,

        Oozaru = 7,


    };

    struct Transformation {
        virtual FormID getID() = 0;
        virtual std::string getName() = 0;
        virtual double getBonus(int location, int specific);
    };

}