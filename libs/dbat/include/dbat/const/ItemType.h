#pragma once
#include <cstdint>


enum class ItemType : uint8_t
{
    unknown = 0,          // Item type not defined
    light = 1,            // Item is a light source
    scroll = 2,           // Item is a scroll
    wand = 3,             // Item is a wand
    staff = 4,            // Item is a staff
    weapon = 5,           // Item is a weapon
    fireweapon = 6,       // Unimplemented
    campfire = 7,         // Burn things for fun!
    treasure = 8,         // Item is a treasure, not gold
    armor = 9,            // Item is armor
    potion = 10,          // Item is a potion
    worn = 11,            // Unimplemented
    other = 12,           // Misc object
    trash = 13,           // Trash - shopkeeps won't buy
    trap = 14,            // Unimplemented
    container = 15,       // Item is a container
    note = 16,            // Item is note
    drink_container = 17, // Item is a drink container
    key = 18,             // Item is a key
    food = 19,            // Item is food
    money = 20,           // Item is money (gold)
    pen = 21,             // Item is a pen
    boat = 22,            // Item is a boat
    fountain = 23,        // Item is a fountain
    vehicle = 24,         // Item is a vehicle
    hatch = 25,           // Item is a vehicle hatch
    window = 26,          // Item is a vehicle window
    control = 27,         // Item is a vehicle control
    portal = 28,          // Item is a portal
    spellbook = 29,       // Item is a spellbook
    board = 30,           // Item is a message board
    chair = 31,           // Is a chair
    bed = 32,             // Is a bed
    yum = 33,             // This was good food
    plant = 34,           // This will grow!
    fishing_pole = 35,    // FOR FISHING
    fishing_bait = 36     // DITTO
};

/* Item types: used by Object.type_flag */
constexpr int ITEM_LIGHT = 1;      /* Item is a light source	*/
constexpr int ITEM_SCROLL = 2;     /* Item is a scroll		*/
constexpr int ITEM_WAND = 3;       /* Item is a wand		*/
constexpr int ITEM_STAFF = 4;      /* Item is a staff		*/
constexpr int ITEM_WEAPON = 5;     /* Item is a weapon		*/
constexpr int ITEM_FIREWEAPON = 6; /* Unimplemented		*/
constexpr int ITEM_CAMPFIRE = 7;   /* Burn things for fun!		*/
constexpr int ITEM_TREASURE = 8;   /* Item is a treasure, not gold	*/
constexpr int ITEM_ARMOR = 9;      /* Item is armor		*/
constexpr int ITEM_POTION = 10;    /* Item is a potion		*/
constexpr int ITEM_WORN = 11;      /* Unimplemented		*/
constexpr int ITEM_OTHER = 12;     /* Misc object			*/
constexpr int ITEM_TRASH = 13;     /* Trash - shopkeeps won't buy	*/
constexpr int ITEM_TRAP = 14;      /* Unimplemented		*/
constexpr int ITEM_CONTAINER = 15; /* Item is a container		*/
constexpr int ITEM_NOTE = 16;      /* Item is note 		*/
constexpr int ITEM_DRINKCON = 17;  /* Item is a drink container	*/
constexpr int ITEM_KEY = 18;       /* Item is a key		*/
constexpr int ITEM_FOOD = 19;      /* Item is food			*/
constexpr int ITEM_MONEY = 20;     /* Item is money (gold)		*/
constexpr int ITEM_PEN = 21;       /* Item is a pen		*/
constexpr int ITEM_BOAT = 22;      /* Item is a boat		*/
constexpr int ITEM_FOUNTAIN = 23;  /* Item is a fountain		*/
constexpr int ITEM_VEHICLE = 24;   /* Item is a vehicle            */
constexpr int ITEM_HATCH = 25;     /* Item is a vehicle hatch      */
constexpr int ITEM_WINDOW = 26;    /* Item is a vehicle window     */
constexpr int ITEM_CONTROL = 27;   /* Item is a vehicle control    */
constexpr int ITEM_PORTAL = 28;    /* Item is a portal	        */
constexpr int ITEM_SPELLBOOK = 29; /* Item is a spellbook	        */
constexpr int ITEM_BOARD = 30;     /* Item is a message board 	*/
constexpr int ITEM_CHAIR = 31;     /* Is a chair                   */
constexpr int ITEM_BED = 32;       /* Is a bed                     */
constexpr int ITEM_YUM = 33;       /* This was good food           */
constexpr int ITEM_PLANT = 34;     /* This will grow!              */
constexpr int ITEM_FISHPOLE = 35;  /* FOR FISHING                  */
constexpr int ITEM_FISHBAIT = 36;  /* DITTO                        */
