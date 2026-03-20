#include "dbat/game/DescriptorMode.hpp"
#include "dbat/game/Descriptor.hpp"


void DescriptorMode::handleInput(std::string_view input) {
    // default implementation does nothing.
}

void DescriptorMode::onLaunch() {
    // default implementation does nothing.
}

void DescriptorMode::onClose() {
    // default implementation does nothing.
}

void DescriptorMode::onReplace() {
    // Called when this mode is being replaced by another mode,
    // not necessarily willingly.

    // default implementation does nothing.
}

bool DescriptorMode::isModeValid() {
    // This is called by a GameSystem every main loop to check if the Mode
    // is still valid. If it returns false, the mode will be closed and removed.

    // If it's going to return false, can send a message to the user about why using desc->sendText("...");

    // default implementation returns true.
    return true;
}