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
constexpr ItemType ITEM_LIGHT = ItemType::light;      /* Item is a light source	*/
constexpr ItemType ITEM_SCROLL = ItemType::scroll;     /* Item is a scroll		*/
constexpr ItemType ITEM_WAND = ItemType::wand;         /* Item is a wand		*/
constexpr ItemType ITEM_STAFF = ItemType::staff;       /* Item is a staff		*/
constexpr ItemType ITEM_WEAPON = ItemType::weapon;     /* Item is a weapon		*/
constexpr ItemType ITEM_FIREWEAPON = ItemType::fireweapon; /* Unimplemented		*/
constexpr ItemType ITEM_CAMPFIRE = ItemType::campfire;   /* Burn things for fun!		*/
constexpr ItemType ITEM_TREASURE = ItemType::treasure;   /* Item is a treasure, not gold	*/
constexpr ItemType ITEM_ARMOR = ItemType::armor;         /* Item is armor		*/
constexpr ItemType ITEM_POTION = ItemType::potion;       /* Item is a potion		*/
constexpr ItemType ITEM_WORN = ItemType::worn;           /* Unimplemented		*/
constexpr ItemType ITEM_OTHER = ItemType::other;         /* Misc object			*/
constexpr ItemType ITEM_TRASH = ItemType::trash;         /* Trash - shopkeeps won't buy	*/
constexpr ItemType ITEM_TRAP = ItemType::trap;           /* Unimplemented		*/
constexpr ItemType ITEM_CONTAINER = ItemType::container; /* Item is a container		*/
constexpr ItemType ITEM_NOTE = ItemType::note;           /* Item is note 		*/
constexpr ItemType ITEM_DRINKCON = ItemType::drink_container; /* Item is a drink container	*/
constexpr ItemType ITEM_KEY = ItemType::key;             /* Item is a key		*/
constexpr ItemType ITEM_FOOD = ItemType::food;           /* Item is food			*/
constexpr ItemType ITEM_MONEY = ItemType::money;         /* Item is money (gold)		*/
constexpr ItemType ITEM_PEN = ItemType::pen;             /* Item is a pen		*/
constexpr ItemType ITEM_BOAT = ItemType::boat;           /* Item is a boat		*/
constexpr ItemType ITEM_FOUNTAIN = ItemType::fountain;   /* Item is a fountain		*/
constexpr ItemType ITEM_VEHICLE = ItemType::vehicle;     /* Item is a vehicle            */
constexpr ItemType ITEM_HATCH = ItemType::hatch;         /* Item is a vehicle hatch      */
constexpr ItemType ITEM_WINDOW = ItemType::window;       /* Item is a vehicle window     */
constexpr ItemType ITEM_CONTROL = ItemType::control;     /* Item is a vehicle control    */
constexpr ItemType ITEM_PORTAL = ItemType::portal;       /* Item is a portal	        */
constexpr ItemType ITEM_SPELLBOOK = ItemType::spellbook; /* Item is a spellbook	        */
constexpr ItemType ITEM_BOARD = ItemType::board;         /* Item is a message board 	*/
constexpr ItemType ITEM_CHAIR = ItemType::chair;         /* Is a chair                   */
constexpr ItemType ITEM_BED = ItemType::bed;             /* Is a bed                     */
constexpr ItemType ITEM_YUM = ItemType::yum;             /* This was good food           */
constexpr ItemType ITEM_PLANT = ItemType::plant;         /* This will grow!              */
constexpr ItemType ITEM_FISHPOLE = ItemType::fishing_pole;  /* FOR FISHING                  */
constexpr ItemType ITEM_FISHBAIT = ItemType::fishing_bait;  /* DITTO                        */
