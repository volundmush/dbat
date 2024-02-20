#include "dbat/structs.h"
#include "dbat/utils.h"

std::string Food::renderAppearanceHelper(GameEntity* viewer) {
    std::string result;

    if (FOOB(this) >= 4) {
        if (GET_OBJ_VAL(this, VAL_FOOD_FOODVAL) < FOOB(this) / 4) {
            result += fmt::sprintf("Condition of the food: Almost gone.\r\n");
        } else if (GET_OBJ_VAL(this, VAL_FOOD_FOODVAL) < FOOB(this) / 2) {
            result += fmt::sprintf("Condition of the food: Half Eaten.");
        } else if (GET_OBJ_VAL(this, VAL_FOOD_FOODVAL) < FOOB(this)) {
            result += fmt::sprintf("Condition of the food: Partially Eaten.");
        } else if (GET_OBJ_VAL(this, VAL_FOOD_FOODVAL) == FOOB(this)) {
            result += fmt::sprintf("Condition of the food: Whole.");
        }
    } else if (FOOB(this) > 0) {
        if (GET_OBJ_VAL(this, VAL_FOOD_FOODVAL) < FOOB(this)) {
            result += fmt::sprintf("Condition of the food: Almost gone.");
        } else if (GET_OBJ_VAL(this, VAL_FOOD_FOODVAL) == FOOB(this)) {
            result += fmt::sprintf("Condition of the food: Whole.");
        }
    } else {
        result += fmt::sprintf("Condition of the food: Insignificant.");
    }

    return result;
}