#pragma once
#include <cstdint>

enum class CharacterFlag : std::uint8_t
{
    tail = 1,
    cyber_right_arm = 2, // Cybernetic Right Arm
    cyber_left_arm = 3,  // Cybernetic Left Arm
    cyber_right_leg = 4, // Cybernetic Right Leg
    cyber_left_leg = 5,  // Cybernetic Left Leg

    sparring = 6,    // This is mob sparring
    powering_up = 7, // Is powering up,
};