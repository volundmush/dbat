#pragma once
#include <cstdint>


enum class ItemFlag
{
    glow = 0,           // Item is glowing
    hum = 1,            // Item is humming
    no_rent = 2,        // Item cannot be rented
    no_donate = 3,      // Item cannot be donated
    no_invisible = 4,   // Item cannot be made invis
    invisible = 5,      // Item is invisible
    magic = 6,          // Item is magical
    nodrop = 7,         // Item is cursed: can't drop
    bless = 8,          // Item is blessed
    nosell = 9,         // Shopkeepers won't touch it
    two_hands = 10,     // Requires two free hands
    unique_save = 11,   // unique object save
    broken = 12,        // Item is broken
    unbreakable = 13,   // Item is unbreakable
    double_weapon = 14, // Double weapon
    card = 15,          // Item is a card
    cboard = 16,
    forged = 17,
    sheath = 18,
    buried = 19,
    slot_1 = 20,
    slot_2 = 21,
    token = 22,
    slot_one = 23,
    slots_filled = 24,
    restring = 25,
    custom = 26,
    protected_item = 27,
    norandom = 28,
    throwable = 29, // "throw" is not reserved, but renamed to avoid confusion
    hot = 30,
    purge = 31,
    ice = 32,
    duplicate = 33,
    mature = 34,
    card_case = 35,
    no_pickup = 36,
    no_steal = 37
};

/* Extra object flags: used by Object.extra_flags */
constexpr int ITEM_GLOW = 0;         /* Item is glowing              */
constexpr int ITEM_HUM = 1;          /* Item is humming              */
constexpr int ITEM_NORENT = 2;       /* Item cannot be rented        */
constexpr int ITEM_NODONATE = 3;     /* Item cannot be donated       */
constexpr int ITEM_NOINVIS = 4;      /* Item cannot be made invis    */
constexpr int ITEM_INVISIBLE = 5;    /* Item is invisible            */
constexpr int ITEM_MAGIC = 6;        /* Item is magical              */
constexpr int ITEM_NODROP = 7;       /* Item is cursed: can't drop   */
constexpr int ITEM_BLESS = 8;        /* Item is blessed              */
constexpr int ITEM_NOSELL = 9;       /* Shopkeepers won't touch it   */
constexpr int ITEM_2H = 10;          /* Requires two free hands      */
constexpr int ITEM_UNIQUE_SAVE = 11; /* unique object save           */
constexpr int ITEM_BROKEN = 12;      /* Item is broken hands         */
constexpr int ITEM_UNBREAKABLE = 13; /* Item is unbreakable          */
constexpr int ITEM_DOUBLE = 14;      /* Double weapon                */
constexpr int ITEM_CARD = 15;        /* Item is a card              */
constexpr int ITEM_CBOARD = 16;
constexpr int ITEM_FORGED = 17;
constexpr int ITEM_SHEATH = 18;
constexpr int ITEM_BURIED = 19;
constexpr int ITEM_SLOT1 = 20;
constexpr int ITEM_SLOT2 = 21;
constexpr int ITEM_TOKEN = 22;
constexpr int ITEM_SLOT_ONE = 23;
constexpr int ITEM_SLOTS_FILLED = 24;
constexpr int ITEM_RESTRING = 25;
constexpr int ITEM_CUSTOM = 26;
constexpr int ITEM_PROTECTED = 27;
constexpr int ITEM_NORANDOM = 28;
constexpr int ITEM_THROW = 29;
constexpr int ITEM_HOT = 30;
constexpr int ITEM_PURGE = 31;
constexpr int ITEM_ICE = 32;
constexpr int ITEM_DUPLICATE = 33;
constexpr int ITEM_MATURE = 34;
constexpr int ITEM_CARDCASE = 35;
constexpr int ITEM_NOPICKUP = 36;
constexpr int ITEM_NOSTEAL = 37;
