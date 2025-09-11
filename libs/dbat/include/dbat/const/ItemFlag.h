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
    no_drop = 7,         // Item is cursed: can't drop
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
constexpr ItemFlag ITEM_GLOW = ItemFlag::glow;         /* Item is glowing              */
constexpr ItemFlag ITEM_HUM = ItemFlag::hum;           /* Item is humming              */
constexpr ItemFlag ITEM_NORENT = ItemFlag::no_rent;    /* Item cannot be rented        */
constexpr ItemFlag ITEM_NODONATE = ItemFlag::no_donate;/* Item cannot be donated       */
constexpr ItemFlag ITEM_NOINVIS = ItemFlag::no_invisible;/* Item cannot be made invis    */
constexpr ItemFlag ITEM_INVISIBLE = ItemFlag::invisible;/* Item is invisible            */
constexpr ItemFlag ITEM_MAGIC = ItemFlag::magic;        /* Item is magical              */
constexpr ItemFlag ITEM_NODROP = ItemFlag::no_drop;      /* Item is cursed: can't drop   */
constexpr ItemFlag ITEM_BLESS = ItemFlag::bless;        /* Item is blessed              */
constexpr ItemFlag ITEM_NOSELL = ItemFlag::nosell;      /* Shopkeepers won't touch it   */
constexpr ItemFlag ITEM_2H = ItemFlag::two_hands;      /* Requires two free hands      */
constexpr ItemFlag ITEM_UNIQUE_SAVE = ItemFlag::unique_save; /* unique object save           */
constexpr ItemFlag ITEM_BROKEN = ItemFlag::broken;      /* Item is broken hands         */
constexpr ItemFlag ITEM_UNBREAKABLE = ItemFlag::unbreakable; /* Item is unbreakable          */
constexpr ItemFlag ITEM_DOUBLE = ItemFlag::double_weapon; /* Double weapon                */
constexpr ItemFlag ITEM_CARD = ItemFlag::card;          /* Item is a card              */
constexpr ItemFlag ITEM_CBOARD = ItemFlag::cboard;      /* Item is a cboard           */
constexpr ItemFlag ITEM_FORGED = ItemFlag::forged;      /* Item is a forged           */
constexpr ItemFlag ITEM_SHEATH = ItemFlag::sheath;      /* Item is a sheath           */
constexpr ItemFlag ITEM_BURIED = ItemFlag::buried;      /* Item is buried             */
constexpr ItemFlag ITEM_SLOT1 = ItemFlag::slot_1;      /* Item is in slot 1         */
constexpr ItemFlag ITEM_SLOT2 = ItemFlag::slot_2;      /* Item is in slot 2         */
constexpr ItemFlag ITEM_TOKEN = ItemFlag::token;      /* Item is a token           */
constexpr ItemFlag ITEM_SLOT_ONE = ItemFlag::slot_one;      /* Item is in slot one      */
constexpr ItemFlag ITEM_SLOTS_FILLED = ItemFlag::slots_filled;      /* Item has filled slots    */
constexpr ItemFlag ITEM_RESTRING = ItemFlag::restring;      /* Item is being restringed  */
constexpr ItemFlag ITEM_CUSTOM = ItemFlag::custom;
constexpr ItemFlag ITEM_PROTECTED = ItemFlag::protected_item;
constexpr ItemFlag ITEM_NORANDOM = ItemFlag::norandom;
constexpr ItemFlag ITEM_THROW = ItemFlag::throwable;
constexpr ItemFlag ITEM_HOT = ItemFlag::hot;
constexpr ItemFlag ITEM_PURGE = ItemFlag::purge;
constexpr ItemFlag ITEM_ICE = ItemFlag::ice;
constexpr ItemFlag ITEM_DUPLICATE = ItemFlag::duplicate;
constexpr ItemFlag ITEM_MATURE = ItemFlag::mature;
constexpr ItemFlag ITEM_CARDCASE = ItemFlag::card_case;
constexpr ItemFlag ITEM_NOPICKUP = ItemFlag::no_pickup;
constexpr ItemFlag ITEM_NOSTEAL = ItemFlag::no_steal;
