#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/constants.h"

static const char *weapon_disp[6] = {
        "Sword",
        "Dagger",
        "Spear",
        "Club",
        "Gun",
        "Brawling"
};

std::string Weapon::renderAppearanceHelper(GameEntity* viewer) {
    std::string result;

    int num = 0;
    if (GET_OBJ_VAL(this, VAL_WEAPON_DAMTYPE) == TYPE_PIERCE - TYPE_HIT) {
        num = 1;
    } else if (GET_OBJ_VAL(this, VAL_WEAPON_DAMTYPE) == TYPE_SLASH - TYPE_HIT) {
        num = 0;
    } else if (GET_OBJ_VAL(this, VAL_WEAPON_DAMTYPE) == TYPE_CRUSH - TYPE_HIT) {
        num = 3;
    } else if (GET_OBJ_VAL(this, VAL_WEAPON_DAMTYPE) == TYPE_STAB - TYPE_HIT) {
        num = 2;
    } else if (GET_OBJ_VAL(this, VAL_WEAPON_DAMTYPE) == TYPE_BLAST - TYPE_HIT) {
        num = 4;
    } else {
        num = 5;
    }
    result += fmt::sprintf("The weapon type of %s@n is '%s'.\r\n", GET_OBJ_SHORT(this), weapon_disp[num]);
    //result += fmt::sprintf("You could wield it %s.\r\n", wield_names[wield_type(get_size(ch), this)]);

    return result;

}