#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/constants.h"

std::string DrinkContainer::renderAppearanceHelper(GameEntity* viewer) {
    std::string result;
    result += fmt::sprintf("It looks like a drink container.\r\n");

    result += renderDiagnostics(viewer);
    result += fmt::sprintf("It appears to be made of %s, and weighs %s", material_names[GET_OBJ_MATERIAL(this)],
                    add_commas(GET_OBJ_WEIGHT(this)).c_str());
    return result;
}