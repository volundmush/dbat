#include "dbat/structs.h"
#include "dbat/utils.h"

std::string Plant::renderRoomListingHelper(GameEntity *viewer) {

    int water = GET_OBJ_VAL(this, VAL_WATERLEVEL);
    auto sd = getShortDesc();
    if (water >= 0) {
        
        switch (GET_OBJ_VAL(this, VAL_MATURITY)) {
            case 0:
                return fmt::sprintf("@wA @G%s@y seed@w has been planted here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 1:
                return fmt::sprintf("@wA very young @G%s@w has sprouted from a planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 2:
                return fmt::sprintf("@wA half grown @G%s@w is in a planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 3:
                return fmt::sprintf("@wA mature @G%s@w is growing in a planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 4:
                return fmt::sprintf("@wA mature @G%s@w is flowering in a planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 5:
                return fmt::sprintf("@wA mature @G%s@w that is close to harvestable is here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            case 6:
                return fmt::sprintf("@wA @Rharvestable @G%s@w is in the planter here. @D(@C%d Water Hours@D)@n\r\n",
                             sd, water);
                break;
            default:
                break;
        }
    } else {
        if (water > -4) {
            return fmt::sprintf("@yA @G%s@y that is looking a bit @rdry@y, is here.@n\r\n", sd);
        } else if (water > -10) {
            return fmt::sprintf("@yA @G%s@y that is looking extremely @rdry@y, is here.@n\r\n", sd);
        } else if (water <= -10) {
            return fmt::sprintf("@yA @G%s@y that is completely @rdead@y and @rwithered@y, is here.@n\r\n",
                         sd);
        }
    }

}