#include "dbat/structs.h"
#include "dbat/players.h"
#include "dbat/utils.h"
#include "dbat/constants.h"


std::string PlayerCharacter::renderRoomListName(GameEntity* viewer) {
    if(viewer->canSeeAdminInvisible()) return getName();

    auto otherPC = dynamic_cast<PlayerCharacter*>(viewer);
    if(!otherPC) return getDisplayName(viewer);
    auto p = players[otherPC->getUID()];
    auto found = p->dubNames.find(getUID());
    if(found != p->dubNames.end() && !PLR_FLAGGED(this, PLR_DISGUISED)) return found->second;

    if(PLR_FLAGGED(this, PLR_DISGUISED)) {
        return fmt::sprintf("@wA disguised %s %s", MAFE(this), LRACE(this));
    }

    std::string result;
    if (GET_DISTFEA(this) == DISTFEA_EYE) {
         result += fmt::sprintf("@wA %s eyed %s %s", eye_types[(int) GET_EYE(this)], MAFE(this), LRACE(this));
    }
    else if (GET_DISTFEA(this) == DISTFEA_HAIR) {
        if (IS_MAJIN(this)) {
            result += fmt::sprintf("@wA %s majin, with a %s forelock,", MAFE(this), FHA_types[(int) GET_HAIRL(this)]);
        } else if (IS_NAMEK(this)) {
            result += fmt::sprintf("@wA namek, with %s antennae,", FHA_types[(int) GET_HAIRL(this)]);
        } else if (IS_ARLIAN(this)) {
            result += fmt::sprintf("@wA %s arlian, with %s antennae,", MAFE(this), FHA_types[(int) GET_HAIRL(this)]);
        } else if (IS_ICER(this) || IS_DEMON(this)) {
            result += fmt::sprintf("@wA %s %s, with %s horns", MAFE(this), LRACE(this), FHA_types[(int) GET_HAIRL(this)]);
        } else {
            char blarg[MAX_INPUT_LENGTH];
            sprintf(blarg, "%s %s hair %s", hairl_types[(int) GET_HAIRL(this)], hairc_types[(int) GET_HAIRC(this)],
                    hairs_types[(int) GET_HAIRS(this)]);
            result += fmt::sprintf("@wA %s %s, with %s", MAFE(this), LRACE(this),
                            GET_HAIRL(this) == 0 ? "a bald head" : (blarg));
        }
    } else if (GET_DISTFEA(this) == DISTFEA_SKIN) {
        result += fmt::sprintf("@wA %s skinned %s %s", skin_types[(int) GET_SKIN(this)], MAFE(this), LRACE(this));
    } else if (GET_DISTFEA(this) == DISTFEA_HEIGHT) {
        char *height;
        if (IS_TRUFFLE(this)) {
            if (GET_PC_HEIGHT(this) > 70) {
                height = strdup("very tall");
            } else if (GET_PC_HEIGHT(this) > 55) {
                height = strdup("tall");
            } else if (GET_PC_HEIGHT(this) > 35) {
                height = strdup("average height");
            } else {
                height = strdup("short");
            }
        } else {
            if (GET_PC_HEIGHT(this) > 200) {
                height = strdup("very tall");
            } else if (GET_PC_HEIGHT(this) > 180) {
                height = strdup("tall");
            } else if (GET_PC_HEIGHT(this) > 150) {
                height = strdup("average height");
            } else if (GET_PC_HEIGHT(this) > 120) {
                height = strdup("short");
            } else {
                height = strdup("very short");
            }
        }
        result += fmt::sprintf("@wA %s %s %s", height, MAFE(this), LRACE(this));
        if (height) {
            free(height);
        }
    } else if (GET_DISTFEA(this) == DISTFEA_WEIGHT) {
        char *height;
        auto w = getWeight();
        if (IS_TRUFFLE(this)) {
            if (w > 35) {
                height = strdup("very heavy");
            } else if (w > 25) {
                height = strdup("heavy");
            } else if (w > 15) {
                height = strdup("average weight");
            } else {
                height = strdup("welterweight");
            }
        } else {
            if (w > 120) {
                height = strdup("very heavy");
            } else if (w > 100) {
                height = strdup("heavy");
            } else if (w > 80) {
                height = strdup("average weight");
            } else if (w > 60) {
                height = strdup("lightweight");
            } else {
                height = strdup("welterweight");
            }
        }
        result += fmt::sprintf("@wA %s %s %s", height, MAFE(this), LRACE(this));
        if (height) {
            free(height);
        }
    }
    return result;
}