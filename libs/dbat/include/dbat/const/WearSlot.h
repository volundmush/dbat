#pragma once
#include <cstdint>


enum class WearSlot : uint8_t {
    Inventory = 0, // not actually equipped, but signifies that something can be in an inventory.
    RightFinger = 1,
    LeftFinger = 2,
    Neck1 = 3,
    Neck2 = 4,
    Body = 5,
    Head = 6,
    Legs = 7,
    Feet = 8,
    Hands = 9,
    Arms = 10,
    About = 12,
    Waist = 13,
    RightWrist = 14,
    LeftWrist = 15,
    Wield1 = 16,
    Wield2 = 17,
    Backpack = 18,
    RightEar = 19,
    LeftEar = 20,
    Shield = 21,
    Eyes = 22
};


/* Character equipment positions: used as index for Character.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
constexpr int WEAR_UNUSED0 = 0;
constexpr int WEAR_FINGER_R = 1;
constexpr int WEAR_FINGER_L = 2;
constexpr int WEAR_NECK_1 = 3;
constexpr int WEAR_NECK_2 = 4;
constexpr int WEAR_BODY = 5;
constexpr int WEAR_HEAD = 6;
constexpr int WEAR_LEGS = 7;
constexpr int WEAR_FEET = 8;
constexpr int WEAR_HANDS = 9;
constexpr int WEAR_ARMS = 10;
constexpr int WEAR_UNUSED1 = 11;
constexpr int WEAR_ABOUT = 12;
constexpr int WEAR_WAIST = 13;
constexpr int WEAR_WRIST_R = 14;
constexpr int WEAR_WRIST_L = 15;
constexpr int WEAR_WIELD1 = 16;
constexpr int WEAR_WIELD2 = 17;
constexpr int WEAR_BACKPACK = 18;
constexpr int WEAR_EAR_R = 19;
constexpr int WEAR_EAR_L = 20;
constexpr int WEAR_SH = 21;
constexpr int WEAR_EYE = 22;
