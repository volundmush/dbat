#pragma once


constexpr const char *VAL_ALL_HEALTH = "health";        // 4
constexpr const char *VAL_ALL_MAXHEALTH = "max_health"; // 5;
constexpr const char *VAL_ALL_MATERIAL = "material";    // 7;
// used by only a few things, but provided here for uniformity.
constexpr const char *VAL_DOOR_DCLOCK = "dc_lock";
constexpr const char *VAL_DOOR_DCHIDE = "dc_hide";
constexpr const char *VAL_DOOR_DCMOVE = "dc_move";
constexpr const char *VAL_DOOR_DCSKILL = "dc_skill";

// ITEM_LIGHT type (1)
constexpr const char *VAL_LIGHT_TIME = "time";   // 0
constexpr const char *VAL_LIGHT_HOURS = "hours"; // 2

// ITEM_SCROLL type (2)
constexpr const char *VAL_SCROLL_LEVEL = "scroll_level"; // 0
constexpr const char *VAL_SCROLL_SPELL1 = "spell1";      // 1
constexpr const char *VAL_SCROLL_SPELL2 = "spell2";      // 2
constexpr const char *VAL_SCROLL_SPELL3 = "spell3";      // 3

// ITEM_WAND type (3)
constexpr const char *VAL_WAND_LEVEL = VAL_SCROLL_LEVEL;   // 0
constexpr const char *VAL_WAND_MAXCHARGES = "max_charges"; // 1
constexpr const char *VAL_WAND_CHARGES = "charges";        // 2
constexpr const char *VAL_WAND_SPELL = "spell";            // 3

// ITEM_STAFF type (4)
constexpr const char *VAL_STAFF_LEVEL = VAL_WAND_LEVEL;           // 0
constexpr const char *VAL_STAFF_MAXCHARGES = VAL_WAND_MAXCHARGES; // 1
constexpr const char *VAL_STAFF_CHARGES = VAL_WAND_CHARGES;       // 2
constexpr const char *VAL_STAFF_SPELL = VAL_WAND_SPELL;           // 3

// ITEM_WEAPON type (5)
constexpr const char *VAL_WEAPON_SKILL = "skill";              // 0
constexpr const char *VAL_WEAPON_DAMDICE = "damage_dice";      // 1
constexpr const char *VAL_WEAPON_DAMSIZE = "damage_size";      // 2
constexpr const char *VAL_WEAPON_DAMTYPE = "damage_type";      // 3
constexpr const char *VAL_WEAPON_CRITTYPE = "critical_type";   // 6
constexpr const char *VAL_WEAPON_CRITRANGE = "critical_range"; // 8
constexpr const char *VAL_WEAPON_LEVEL = VAL_SCROLL_LEVEL;     // 9

// ITEM_FIREWEAPON type (6)
// no values

// ITEM_CAMPFIRE type (7)
// no values

// ITEM_TREASURE type (8)
// no values

// ITEM_ARMOR type (9)
constexpr const char *VAL_ARMOR_APPLYAC = "apply_ac";      // 0
constexpr const char *VAL_ARMOR_SKILL = VAL_WEAPON_SKILL;  // 1
constexpr const char *VAL_ARMOR_MAXDEXMOD = "max_dex_mod"; // 2
constexpr const char *VAL_ARMOR_CHECK = "check";           // 3
constexpr const char *VAL_ARMOR_SPELLFAIL = "spell_fail";  // 6

// ITEM_POTION type (10)
constexpr const char *VAL_POTION_LEVEL = VAL_SCROLL_LEVEL;   // 0
constexpr const char *VAL_POTION_SPELL1 = VAL_SCROLL_SPELL1; // 1
constexpr const char *VAL_POTION_SPELL2 = VAL_SCROLL_SPELL2; // 2
constexpr const char *VAL_POTION_SPELL3 = VAL_SCROLL_SPELL3; // 3

// ITEM_WORN type (11)
constexpr const char *VAL_WORN_SCOUTER = "scouter_level"; // 15

// ITEM_OTHER
// used by seraf ink bottle, object vnum 3424 - OTHER
constexpr const char *VAL_OTHER_SERAF = "seraf_ink";          // 6
constexpr const char *VAL_OTHER_SOILQUALITY = "soil_quality"; // 8

// ITEM_TRASH type (13)
// no values

// ITEM_TRAP type (14)
constexpr const char *VAL_TRAP_SPELL = VAL_WAND_SPELL;     // 0
constexpr const char *VAL_TRAP_HITPOINTS = VAL_ALL_HEALTH; // 1 -- why not use health?

// ITEM_CONTAINER type (15)
constexpr const char *VAL_CONTAINER_CAPACITY = "capacity"; // 0
constexpr const char *VAL_CONTAINER_FLAGS = "flags";       // 1
constexpr const char *VAL_CONTAINER_KEY = "key";           // 2
constexpr const char *VAL_CONTAINER_CORPSE = "corpse";     // 3
constexpr const char *VAL_CONTAINER_OWNER = "owner";       // 8

// technically, corpses are containers. But they have weird bodypart extensions.
constexpr const char *VAL_CORPSE_HEAD = "head";      // 8
constexpr const char *VAL_CORPSE_RARM = "right_arm"; // 9
constexpr const char *VAL_CORPSE_LARM = "left_arm";  // 10
constexpr const char *VAL_CORPSE_RLEG = "right_leg"; // 11
constexpr const char *VAL_CORPSE_LLEG = "left_leg";  // 12

// ITEM_NOTE type (16)
constexpr const char *VAL_NOTE_LANGUAGE = "language"; // 0

// ITEM_DRINKCON type (17)
constexpr const char *VAL_DRINKCON_CAPACITY = VAL_CONTAINER_CAPACITY; // 0
constexpr const char *VAL_DRINKCON_HOWFULL = "how_full";              // 1
constexpr const char *VAL_DRINKCON_LIQUID = "liquid";                 // 2
constexpr const char *VAL_DRINKCON_POISON = "poison";                 // 3

// ITEM_KEY type (18)
constexpr const char *VAL_KEY_KEYCODE = "keycode"; // 2

// ITEM_FOOD type (19)
constexpr const char *VAL_FOOD_FOODVAL = "foodval";        // 0
constexpr const char *VAL_FOOD_MAXFOODVAL = "max_foodval"; // 1
constexpr const char *VAL_FOOD_PSBONUS = "psbonus";        // 2
constexpr const char *VAL_FOOD_POISON = "poison";          // 3
constexpr const char *VAL_FOOD_EXPBONUS = "expbonus";      // 6
constexpr const char *VAL_FOOD_CANDY_PL = "candy_pl";      // 8
constexpr const char *VAL_FOOD_CANDY_KI = "candy_ki";      // 9
constexpr const char *VAL_FOOD_CANDY_ST = "candy_st";      // 10
constexpr const char *VAL_FOOD_WHICHATTR = "whichattr";    // 11
constexpr const char *VAL_FOOD_ATTRCHANCE = "attrchance";  // 12

// ITEM_MONEY type (20)
constexpr const char *VAL_MONEY_SIZE = "size"; // 0

// ITEM_PEN type (21)
// no values

// ITEM_BOAT type (22
// no values

// ITEM_FOUNTAIN type (23)
constexpr const char *VAL_FOUNTAIN_CAPACITY = VAL_DRINKCON_CAPACITY; // 0
constexpr const char *VAL_FOUNTAIN_HOWFULL = VAL_DRINKCON_HOWFULL;   // 1
constexpr const char *VAL_FOUNTAIN_LIQUID = VAL_DRINKCON_LIQUID;     // 2
constexpr const char *VAL_FOUNTAIN_POISON = VAL_DRINKCON_POISON;     // 3

// ITEM_VEHICLE type (24)
// vehicles are also houses apparently?
constexpr const char *VAL_VEHICLE_DEST = "destination_room";   // 0
constexpr const char *VAL_VEHICLE_FLAGS = VAL_CONTAINER_FLAGS; // 1
constexpr const char *VAL_VEHICLE_FUEL = "fuel";               // 2
constexpr const char *VAL_VEHICLE_FUELCOUNT = "fuelcount";     // 3

// ITEM_HATCH type (25)
// VAL_HATCH_DEST is also the room ID of the external house object.
constexpr const char *VAL_HATCH_DEST = VAL_VEHICLE_DEST;     // 0
constexpr const char *VAL_HATCH_FLAGS = VAL_CONTAINER_FLAGS; // 1
constexpr const char *VAL_HATCH_DCSKILL = VAL_DOOR_DCSKILL;  // 2
constexpr const char *VAL_HATCH_EXTROOM = "external_room";   // 3
constexpr const char *VAL_HATCH_LOCATION = "location";       // 6
constexpr const char *VAL_HATCH_DCLOCK = VAL_DOOR_DCLOCK;    // 8
constexpr const char *VAL_HATCH_DCHIDE = VAL_DOOR_DCHIDE;    // 9

// ITEM_WINDOW type (26)
constexpr const char *VAL_WINDOW_VIEWPORT = "viewport";         // 0
constexpr const char *VAL_WINDOW_FLAGS = VAL_CONTAINER_FLAGS;   // 1
constexpr const char *VAL_WINDOW_DEFAULT_ROOM = "default_room"; // 3

// ITEM_CONTROL type (27)
constexpr const char *VAL_CONTROL_VEHICLE_VNUM = "vehicle_vnum";     // 0
constexpr const char *VAL_CONTROL_SPEED = "speed";                   // 1
constexpr const char *VAL_CONTROL_FUEL = VAL_VEHICLE_FUEL;           // 2
constexpr const char *VAL_CONTROL_FUELCOUNT = VAL_VEHICLE_FUELCOUNT; // 3

// ITEM_PORTAL type (28)
constexpr const char *VAL_PORTAL_DEST = VAL_VEHICLE_DEST;     // 0
constexpr const char *VAL_PORTAL_FLAGS = VAL_CONTAINER_FLAGS; // 1
constexpr const char *VAL_PORTAL_DCMOVE = VAL_DOOR_DCMOVE;    // 2
constexpr const char *VAL_PORTAL_APPEAR = "appear";           // 3
constexpr const char *VAL_PORTAL_DCLOCK = VAL_DOOR_DCLOCK;    // 8
constexpr const char *VAL_PORTAL_DCHIDE = VAL_DOOR_DCHIDE;    // 9

// ITEM_SPELLBOOK type (29)
// nothing here...

// ITEM_BOARD type (30)
constexpr const char *VAL_BOARD_READ = "read";   // 0
constexpr const char *VAL_BOARD_WRITE = "write"; // 1
constexpr const char *VAL_BOARD_ERASE = "erase"; // 2

// ITEM_BED type (32)
constexpr const char *VAL_BED_LEVEL = "comfort_level";       // 8
constexpr const char *VAL_BED_HTANK_CHARGE = "htank_charge"; // 9

// ITEM_PLANT type (34)
constexpr const char *VAL_PLANT_SOILQUALITY = VAL_OTHER_SOILQUALITY; // 0
constexpr const char *VAL_PLANT_MATGOAL = "mat_goal";                // 1
constexpr const char *VAL_PLANT_MATURITY = "maturity";               // 2
constexpr const char *VAL_PLANT_MAXMATURE = "max_mature";            // 3
constexpr const char *VAL_PLANT_WATERLEVEL = "water_level";          // 6

// ITEM_FISHPOLE type (35)
constexpr const char *VAL_FISHPOLE_BAIT = "bait"; // 0

// ITEM_FISHBAIT type (36)
// nothing here!
