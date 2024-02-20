#include "dbat/structs.h"
#include "dbat/utils.h"
#include "dbat/constants.h"

std::string GlacialWall::renderRoomListingHelper(GameEntity *viewer) {
    auto result = renderListPrefixFor(viewer);
    auto curWeight = GET_OBJ_WEIGHT(this);

    result += fmt::sprintf(
                        "@wA @cG@Cl@wa@cc@Ci@wa@cl @wW@ca@Cl@wl @D[@C%s@D]@w is blocking access to the @G%s@w direction",
                        add_commas(curWeight).c_str(), dirs[cost]);

    return result;

}