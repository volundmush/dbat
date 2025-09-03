#pragma once
#include <cstdint>


enum class WearFlag
{
    take = 0,       // Item can be taken
    finger = 1,     // Can be worn on finger
    neck = 2,       // Can be worn around neck
    body = 3,       // Can be worn on body
    head = 4,       // Can be worn on head
    legs = 5,       // Can be worn on legs
    feet = 6,       // Can be worn on feet
    hands = 7,      // Can be worn on hands
    arms = 8,       // Can be worn on arms
    shield = 9,     // Can be used as a shield
    about = 10,     // Can be worn about body
    waist = 11,     // Can be worn around waist
    wrist = 12,     // Can be worn on wrist
    wield = 13,     // Can be wielded
    hold = 14,      // Can be held
    back = 15,      // Can be worn as a backpack
    ear = 16,       // Can be worn as an earring
    shoulders = 17, // Can be worn as wings (originally ITEM_WEAR_SH)
    eyes = 18       // Can be worn as a mask
};

/* Take/Wear flags: used by Object.wear_flags */
constexpr int ITEM_WEAR_TAKE = 0;   /* Item can be taken         */
constexpr int ITEM_WEAR_FINGER = 1; /* Can be worn on finger     */
constexpr int ITEM_WEAR_NECK = 2;   /* Can be worn around neck   */
constexpr int ITEM_WEAR_BODY = 3;   /* Can be worn on body       */
constexpr int ITEM_WEAR_HEAD = 4;   /* Can be worn on head       */
constexpr int ITEM_WEAR_LEGS = 5;   /* Can be worn on legs       */
constexpr int ITEM_WEAR_FEET = 6;   /* Can be worn on feet       */
constexpr int ITEM_WEAR_HANDS = 7;  /* Can be worn on hands      */
constexpr int ITEM_WEAR_ARMS = 8;   /* Can be worn on arms       */
constexpr int ITEM_WEAR_SHIELD = 9; /* Can be used as a shield   */
constexpr int ITEM_WEAR_ABOUT = 10; /* Can be worn about body    */
constexpr int ITEM_WEAR_WAIST = 11; /* Can be worn around waist  */
constexpr int ITEM_WEAR_WRIST = 12; /* Can be worn on wrist      */
constexpr int ITEM_WEAR_WIELD = 13; /* Can be wielded            */
constexpr int ITEM_WEAR_HOLD = 14;  /* Can be held               */
constexpr int ITEM_WEAR_PACK = 15;  /* Can be worn as a backpack */
constexpr int ITEM_WEAR_EAR = 16;   /* Can be worn as an earring */
constexpr int ITEM_WEAR_SH = 17;    /* Can be worn as wings      */
constexpr int ITEM_WEAR_EYE = 18;   /* Can be worn as a mask     */
