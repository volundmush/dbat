#include "dbat/structs.h"

std::string GravityGenerator::renderRoomListingHelper(GameEntity *viewer) {
    auto result = renderListPrefixFor(viewer);

    if (auto grav = emitEnvVar(EnvVar::Gravity); grav) {
        result += fmt::sprintf("@wA gravity generator, set to {}x gravity, is built here.", grav.value());
    } else {
        result += fmt::sprintf("@wA gravity generator, currently on standby, is built here.");
    }

    return result;

}