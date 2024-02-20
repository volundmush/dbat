#include "dbat/structs.h"


std::string Vehicle::renderAppearanceHelper(GameEntity* viewer) {
    std::string result;

    result += fmt::sprintf("It looks like a vehicle.\r\n");
    result += fmt::sprintf("@YSyntax@D: @CUnlock hatch\r\n");
    result += fmt::sprintf("@YSyntax@D: @COpen hatch\r\n");
    result += fmt::sprintf("@YSyntax@D: @CClose hatch\r\n");
    result += fmt::sprintf("@YSyntax@D: @CEnter hatch\r\n");


    return result;

}