#include "dbat/structs.h"
#include "dbat/utils.h"

std::string Corpse::renderAppearanceHelper(GameEntity* viewer) {
    std::string result;

    int mention = false;
    result += fmt::sprintf("This corpse has ");

    if (GET_OBJ_VAL(this, VAL_CORPSE_HEAD) == 0) {
        result += fmt::sprintf("no head,");
        mention = true;
    }

    if (GET_OBJ_VAL(this, VAL_CORPSE_RARM) == 0) {
        result += fmt::sprintf("no right arm, ");
        mention = true;
    } else if (GET_OBJ_VAL(this, VAL_CORPSE_RARM) == 2) {
        result += fmt::sprintf("a broken right arm, ");
        mention = true;
    }

    if (GET_OBJ_VAL(this, VAL_CORPSE_LARM) == 0) {
        result += fmt::sprintf("no left arm, ");
        mention = true;
    } else if (GET_OBJ_VAL(this, VAL_CORPSE_LARM) == 2) {
        result += fmt::sprintf("a broken left arm, ");
        mention = true;
    }

    if (GET_OBJ_VAL(this, VAL_CORPSE_RLEG) == 0) {
        result += fmt::sprintf("no right leg, ");
        mention = true;
    } else if (GET_OBJ_VAL(this, VAL_CORPSE_RLEG) == 2) {
        result += fmt::sprintf("a broken right leg, ");
        mention = true;
    }

    if (GET_OBJ_VAL(this, VAL_CORPSE_LLEG) == 0) {
        result += fmt::sprintf("no left leg, ");
        mention = true;
    } else if (GET_OBJ_VAL(this, VAL_CORPSE_LLEG) == 2) {
        result += fmt::sprintf("a broken left leg, ");
        mention = true;
    }

    if (mention == false) {
        result += fmt::sprintf("nothing missing from it but life.");
    } else {
        result += fmt::sprintf("and is dead.");
    }

    result += fmt::sprintf("\r\n");

    return result;
}