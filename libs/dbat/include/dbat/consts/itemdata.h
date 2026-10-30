#pragma once
#include "types.h"
#include "materials.h"
#include "conditions.h"

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
constexpr int WEAR_UNUSED0    = 0;
constexpr int WEAR_FINGER_R   = 1;
constexpr int WEAR_FINGER_L   = 2;
constexpr int WEAR_NECK_1     = 3;
constexpr int WEAR_NECK_2     = 4;
constexpr int WEAR_BODY       = 5;
constexpr int WEAR_HEAD       = 6;
constexpr int WEAR_LEGS       = 7;
constexpr int WEAR_FEET       = 8;
constexpr int WEAR_HANDS      = 9;
constexpr int WEAR_ARMS      = 10;
constexpr int WEAR_UNUSED1   = 11;
constexpr int WEAR_ABOUT     = 12;
constexpr int WEAR_WAIST     = 13;
constexpr int WEAR_WRIST_R   = 14;
constexpr int WEAR_WRIST_L   = 15;
constexpr int WEAR_WIELD1    = 16;
constexpr int WEAR_WIELD2    = 17;
constexpr int WEAR_BACKPACK  = 18;
constexpr int WEAR_EAR_R     = 19;
constexpr int WEAR_EAR_L     = 20;
constexpr int WEAR_SH        = 21;
constexpr int WEAR_EYE       = 22;

constexpr int NUM_WEARS      = 23; /* This must be the # of eq positions!! */

/* object-related defines ********************************************/


/* Item types: used by obj_data.type_flag */
constexpr int ITEM_LIGHT      = 1; /* Item is a light source	*/
constexpr int ITEM_SCROLL     = 2; /* Item is a scroll		*/
constexpr int ITEM_WAND       = 3; /* Item is a wand		*/
constexpr int ITEM_STAFF      = 4; /* Item is a staff		*/
constexpr int ITEM_WEAPON     = 5; /* Item is a weapon		*/
constexpr int ITEM_FIREWEAPON = 6; /* Unimplemented		*/
constexpr int ITEM_CAMPFIRE   = 7; /* Burn things for fun!		*/
constexpr int ITEM_TREASURE   = 8; /* Item is a treasure, not gold	*/
constexpr int ITEM_ARMOR      = 9; /* Item is armor		*/
constexpr int ITEM_POTION    = 10; /* Item is a potion		*/
constexpr int ITEM_WORN      = 11; /* Unimplemented		*/
constexpr int ITEM_OTHER     = 12; /* Misc object			*/
constexpr int ITEM_TRASH     = 13; /* Trash - shopkeeps won't buy	*/
constexpr int ITEM_TRAP      = 14; /* Unimplemented		*/
constexpr int ITEM_CONTAINER = 15; /* Item is a container		*/
constexpr int ITEM_NOTE      = 16; /* Item is note 		*/
constexpr int ITEM_DRINKCON  = 17; /* Item is a drink container	*/
constexpr int ITEM_KEY       = 18; /* Item is a key		*/
constexpr int ITEM_FOOD      = 19; /* Item is food			*/
constexpr int ITEM_MONEY     = 20; /* Item is money (gold)		*/
constexpr int ITEM_PEN       = 21; /* Item is a pen		*/
constexpr int ITEM_BOAT      = 22; /* Item is a boat		*/
constexpr int ITEM_FOUNTAIN  = 23; /* Item is a fountain		*/
constexpr int ITEM_VEHICLE   = 24; /* Item is a vehicle            */
constexpr int ITEM_HATCH     = 25; /* Item is a vehicle hatch      */
constexpr int ITEM_WINDOW    = 26; /* Item is a vehicle window     */
constexpr int ITEM_CONTROL   = 27; /* Item is a vehicle control    */
constexpr int ITEM_PORTAL    = 28; /* Item is a portal	        */
constexpr int ITEM_SPELLBOOK = 29; /* Item is a spellbook	        */
constexpr int ITEM_BOARD     = 30; /* Item is a message board 	*/
constexpr int ITEM_CHAIR     = 31; /* Is a chair                   */
constexpr int ITEM_BED       = 32; /* Is a bed                     */
constexpr int ITEM_YUM       = 33; /* This was good food           */
constexpr int ITEM_PLANT     = 34; /* This will grow!              */
constexpr int ITEM_FISHPOLE  = 35; /* FOR FISHING                  */
constexpr int ITEM_FISHBAIT  = 36; /* DITTO                        */

constexpr int NUM_ITEM_TYPES = 37;

/* Take/Wear flags: used by obj_data.wear_flags */
constexpr int ITEM_WEAR_TAKE        = 0; /* Item can be taken         */
constexpr int ITEM_WEAR_FINGER      = 1; /* Can be worn on finger     */
constexpr int ITEM_WEAR_NECK        = 2; /* Can be worn around neck   */
constexpr int ITEM_WEAR_BODY        = 3; /* Can be worn on body       */
constexpr int ITEM_WEAR_HEAD        = 4; /* Can be worn on head       */
constexpr int ITEM_WEAR_LEGS        = 5; /* Can be worn on legs       */
constexpr int ITEM_WEAR_FEET        = 6; /* Can be worn on feet       */
constexpr int ITEM_WEAR_HANDS       = 7; /* Can be worn on hands      */
constexpr int ITEM_WEAR_ARMS        = 8; /* Can be worn on arms       */
constexpr int ITEM_WEAR_SHIELD      = 9; /* Can be used as a shield   */
constexpr int ITEM_WEAR_ABOUT       = 10; /* Can be worn about body    */
constexpr int ITEM_WEAR_WAIST       = 11; /* Can be worn around waist  */
constexpr int ITEM_WEAR_WRIST       = 12; /* Can be worn on wrist      */
constexpr int ITEM_WEAR_WIELD       = 13; /* Can be wielded            */
constexpr int ITEM_WEAR_HOLD        = 14; /* Can be held               */
constexpr int ITEM_WEAR_PACK        = 15; /* Can be worn as a backpack */
constexpr int ITEM_WEAR_EAR         = 16; /* Can be worn as an earring */
constexpr int ITEM_WEAR_SH          = 17; /* Can be worn as wings      */
constexpr int ITEM_WEAR_EYE         = 18; /* Can be worn as a mask     */

constexpr int NUM_ITEM_WEARS = 19;

/* Extra object flags: used by obj_data.extra_flags */
constexpr int ITEM_GLOW            = 0; /* Item is glowing              */
constexpr int ITEM_HUM             = 1; /* Item is humming              */
constexpr int ITEM_NORENT          = 2; /* Item cannot be rented        */
constexpr int ITEM_NODONATE        = 3; /* Item cannot be donated       */
constexpr int ITEM_NOINVIS         = 4; /* Item cannot be made invis    */
constexpr int ITEM_INVISIBLE       = 5; /* Item is invisible            */
constexpr int ITEM_MAGIC           = 6; /* Item is magical              */
constexpr int ITEM_NODROP          = 7; /* Item is cursed: can't drop   */
constexpr int ITEM_BLESS           = 8; /* Item is blessed              */
constexpr int ITEM_ANTI_GOOD       = 9; /* Not usable by good people    */
constexpr int ITEM_ANTI_EVIL       = 10; /* Not usable by evil people    */
constexpr int ITEM_ANTI_NEUTRAL    = 11; /* Not usable by neutral people */
constexpr int ITEM_ANTI_WIZARD     = 12; /* Not usable by mages          */
constexpr int ITEM_ANTI_CLERIC     = 13; /* Not usable by clerics        */
constexpr int ITEM_ANTI_ROGUE      = 14; /* Not usable by thieves        */
constexpr int ITEM_ANTI_FIGHTER    = 15; /* Not usable by warriors       */
constexpr int ITEM_NOSELL          = 16; /* Shopkeepers won't touch it   */
constexpr int ITEM_ANTI_DRUID      = 17; /* Not usable by druids         */
constexpr int ITEM_2H              = 18; /* Requires two free hands      */
constexpr int ITEM_ANTI_BARD       = 19; /* Not usable by bards          */
constexpr int ITEM_ANTI_RANGER     = 20; /* Not usable by rangers        */
constexpr int ITEM_ANTI_PALADIN    = 21; /* Not usable by paladins       */
constexpr int ITEM_ANTI_HUMAN      = 22; /* Not usable by humans         */
constexpr int ITEM_ANTI_ICER       = 23; /* Not usable by dwarves        */
constexpr int ITEM_ANTI_SAIYAN     = 24; /* Not usable by elves          */
constexpr int ITEM_ANTI_KONATSU    = 25; /* Not usable by gnomes         */
constexpr int ITEM_UNIQUE_SAVE     = 26; /* unique object save           */
constexpr int ITEM_BROKEN          = 27; /* Item is broken hands         */
constexpr int ITEM_UNBREAKABLE     = 28; /* Item is unbreakable          */
constexpr int ITEM_ANTI_MONK       = 29; /* Not usable by monks          */
constexpr int ITEM_ANTI_BARBARIAN  = 30; /* Not usable by barbarians     */
constexpr int ITEM_ANTI_SORCERER   = 31; /* Not usable by sorcerers      */
constexpr int ITEM_DOUBLE          = 32; /* Double weapon                */
constexpr int ITEM_ONLY_WIZARD     = 33; /* Only usable by mages         */
constexpr int ITEM_ONLY_CLERIC     = 34; /* Only usable by clerics       */
constexpr int ITEM_ONLY_ROGUE      = 35; /* Only usable by thieves       */
constexpr int ITEM_ONLY_FIGHTER    = 36; /* Only usable by warriors      */
constexpr int ITEM_ONLY_DRUID      = 37; /* Only usable by druids        */
constexpr int ITEM_ONLY_BARD       = 38; /* Only usable by bards         */
constexpr int ITEM_ONLY_RANGER     = 39; /* Only usable by rangers       */
constexpr int ITEM_ONLY_PALADIN    = 40; /* Only usable by paladins      */
constexpr int ITEM_ONLY_HUMAN      = 41; /* Only usable by humans        */
constexpr int ITEM_ONLY_ICER       = 42; /* Only usable by dwarves       */
constexpr int ITEM_ONLY_SAIYAN     = 43; /* Only usable by elves         */
constexpr int ITEM_ONLY_KONATSU    = 44; /* Only usable by gnomes        */
constexpr int ITEM_ONLY_MONK       = 45; /* Only usable by monks         */
constexpr int ITEM_ONLY_BARBARIAN  = 46; /* Only usable by barbarians    */
constexpr int ITEM_ONLY_SORCERER   = 47; /* Only usable by sorcerers     */
constexpr int ITEM_ANTI_ARCANE_ARCHER        = 48; /* Not usable by Aracane Archers          */
constexpr int ITEM_ANTI_ARCANE_TRICKSTER     = 49; /* Not usable by Aracane tricksters       */
constexpr int ITEM_ANTI_ARCHMAGE             = 50; /* Not usable by Archmages                */
constexpr int ITEM_ANTI_ASSASSIN             = 51; /* Not usable by Assassins                */
constexpr int ITEM_ANTI_BLACKGUARD           = 52; /* Not usable by Blackguards              */
constexpr int ITEM_ANTI_DRAGON_DISCIPLE      = 53; /* Not usable by Dragon disciples         */
constexpr int ITEM_ANTI_DUELIST              = 54; /* Not usable by Duelists                 */
constexpr int ITEM_ANTI_DWARVEN_DEFENDER     = 55; /* Not usable by Dwarven defenders        */
constexpr int ITEM_ANTI_ELDRITCH_KNIGHT      = 56; /* Not usable by Eldritch knights         */
constexpr int ITEM_ANTI_HIEROPHANT           = 57; /* Not usable by Hierophants              */ 
constexpr int ITEM_ANTI_HORIZON_WALKER       = 58; /* Not usable by Horizon walkers          */     
constexpr int ITEM_ANTI_LOREMASTER           = 59; /* Not usable by Loremasters              */
constexpr int ITEM_ANTI_MYSTIC_THEURGE       = 60; /* Not usable by Mystic theurges          */
constexpr int ITEM_ANTI_SHADOWDANCER         = 61; /* Not useable by shadowdancers           */ 
constexpr int ITEM_ANTI_THAUMATURGIST        = 62; /* Not useable by thaumaturgists          */     
constexpr int ITEM_BSCOUTER                  = 63;
constexpr int ITEM_MSCOUTER                  = 64;
constexpr int ITEM_ASCOUTER                  = 65;
constexpr int ITEM_USCOUTER                  = 66;
constexpr int ITEM_WEAPLVL1                  = 67;
constexpr int ITEM_WEAPLVL2                  = 68;
constexpr int ITEM_WEAPLVL3                  = 69;
constexpr int ITEM_WEAPLVL4                  = 70;
constexpr int ITEM_WEAPLVL5                  = 71;
constexpr int ITEM_CBOARD                    = 72;
constexpr int ITEM_FORGED                    = 73;
constexpr int ITEM_SHEATH                    = 74;
constexpr int ITEM_ONLY_JINTO                = 75;
constexpr int ITEM_BURIED                    = 76;
constexpr int ITEM_SLOT1                     = 77;
constexpr int ITEM_SLOT2                     = 78;
constexpr int ITEM_TOKEN                     = 79;
constexpr int ITEM_SLOT_ONE                  = 80;
constexpr int ITEM_SLOTS_FILLED              = 81;
constexpr int ITEM_RESTRING                  = 82;
constexpr int ITEM_CUSTOM                    = 83;
constexpr int ITEM_PROTECTED                 = 84;
constexpr int ITEM_NORANDOM                  = 85;
constexpr int ITEM_THROW                     = 86;
constexpr int ITEM_HOT                       = 87;
constexpr int ITEM_PURGE                     = 88;
constexpr int ITEM_ICE                       = 89;
constexpr int ITEM_DUPLICATE                 = 90;
constexpr int ITEM_MATURE                    = 91;
constexpr int ITEM_CARDCASE                  = 92;
constexpr int ITEM_NOPICKUP                  = 93;
constexpr int ITEM_NOSTEAL                   = 94;

constexpr int NUM_ITEM_FLAGS = 95;


/* Container flags - value[1] */
constexpr bitvector_t CONT_CLOSEABLE      = (1u << 0); /* Container can be closed	*/
constexpr bitvector_t CONT_PICKPROOF      = (1u << 1); /* Container is pickproof	*/
constexpr bitvector_t CONT_CLOSED         = (1u << 2); /* Container is closed		*/
constexpr bitvector_t CONT_LOCKED         = (1u << 3); /* Container is locked		*/

constexpr int NUM_CONT_FLAGS = 4;


/* object-related structures ******************************************/
constexpr int NUM_OBJ_VAL_POSITIONS = 16;

constexpr int VAL_ALL_HEALTH                = 4;
constexpr int VAL_ALL_MAXHEALTH             = 5;
constexpr int VAL_ALL_MATERIAL              = 7;
/*
 * Uses for generic object values on specific object types
 * Please use these instead of numbers to prevent overlaps.
 */
constexpr int VAL_LIGHT_TIME                = 0;
constexpr int VAL_LIGHT_UNUSED2             = 1;
constexpr int VAL_LIGHT_HOURS               = 2;
constexpr int VAL_LIGHT_UNUSED4             = 3;
constexpr int VAL_LIGHT_HEALTH              = 4;
constexpr int VAL_LIGHT_MAXHEALTH           = 5;
constexpr int VAL_LIGHT_UNUSED7             = 6;
constexpr int VAL_LIGHT_MATERIAL            = 7;
constexpr int VAL_SCROLL_LEVEL              = 0;
constexpr int VAL_SCROLL_SPELL1             = 1;
constexpr int VAL_SCROLL_SPELL2             = 2;
constexpr int VAL_SCROLL_SPELL3             = 3;
constexpr int VAL_SCROLL_HEALTH             = 4;
constexpr int VAL_SCROLL_MAXHEALTH          = 5;
constexpr int VAL_SCROLL_UNUSED7            = 6;
constexpr int VAL_SCROLL_MATERIAL           = 7;
constexpr int VAL_WAND_LEVEL                = 0;
constexpr int VAL_WAND_MAXCHARGES           = 1;
constexpr int VAL_WAND_CHARGES              = 2;
constexpr int VAL_WAND_SPELL                = 3;
constexpr int VAL_WAND_HEALTH               = 4;
constexpr int VAL_WAND_MAXHEALTH            = 5;
constexpr int VAL_WAND_UNUSED7              = 6;
constexpr int VAL_WAND_MATERIAL             = 7;
constexpr int VAL_STAFF_LEVEL               = 0;
constexpr int VAL_STAFF_MAXCHARGES          = 1;
constexpr int VAL_STAFF_CHARGES             = 2;
constexpr int VAL_STAFF_SPELL               = 3;
constexpr int VAL_STAFF_HEALTH              = 4;
constexpr int VAL_STAFF_MAXHEALTH           = 5;
constexpr int VAL_STAFF_UNUSED7             = 6;
constexpr int VAL_STAFF_MATERIAL            = 7;
constexpr int VAL_WEAPON_SKILL              = 0;
constexpr int VAL_WEAPON_DAMDICE            = 1;
constexpr int VAL_WEAPON_DAMSIZE            = 2;
constexpr int VAL_WEAPON_DAMTYPE            = 3;
constexpr int VAL_WEAPON_HEALTH             = 4;
constexpr int VAL_WEAPON_MAXHEALTH          = 5;
constexpr int VAL_WEAPON_CRITTYPE           = 6;
constexpr int VAL_WEAPON_MATERIAL           = 7;
constexpr int VAL_WEAPON_CRITRANGE          = 8;
constexpr int VAL_FIREWEAPON_UNUSED1        = 0;
constexpr int VAL_FIREWEAPON_UNUSED2        = 1;
constexpr int VAL_FIREWEAPON_UNUSED3        = 2;
constexpr int VAL_FIREWEAPON_UNUSED4        = 3;
constexpr int VAL_FIREWEAPON_HEALTH         = 4;
constexpr int VAL_FIREWEAPON_MAXHEALTH      = 5;
constexpr int VAL_FIREWEAPON_UNUSED7        = 6;
constexpr int VAL_FIREWEAPON_MATERIAL       = 7;
constexpr int VAL_MISSILE_UNUSED1           = 0;
constexpr int VAL_MISSILE_UNUSED2           = 1;
constexpr int VAL_MISSILE_UNUSED3           = 2;
constexpr int VAL_MISSILE_UNUSED4           = 3;
constexpr int VAL_MISSILE_HEALTH            = 4;
constexpr int VAL_MISSILE_MAXHEALTH         = 5;
constexpr int VAL_MISSILE_UNUSED7           = 6;
constexpr int VAL_MISSILE_MATERIAL          = 7;
constexpr int VAL_TREASURE_UNUSED1          = 0;
constexpr int VAL_TREASURE_UNUSED2          = 1;
constexpr int VAL_TREASURE_UNUSED3          = 2;
constexpr int VAL_TREASURE_UNUSED4          = 3;
constexpr int VAL_TREASURE_HEALTH           = 4;
constexpr int VAL_TREASURE_MAXHEALTH        = 5;
constexpr int VAL_TREASURE_UNUSED7          = 6;
constexpr int VAL_TREASURE_MATERIAL         = 7;
constexpr int VAL_ARMOR_APPLYAC             = 0;
constexpr int VAL_ARMOR_SKILL               = 1;
constexpr int VAL_ARMOR_MAXDEXMOD           = 2;
constexpr int VAL_ARMOR_CHECK               = 3;
constexpr int VAL_ARMOR_HEALTH              = 4;
constexpr int VAL_ARMOR_MAXHEALTH           = 5;
constexpr int VAL_ARMOR_SPELLFAIL           = 6;
constexpr int VAL_ARMOR_MATERIAL            = 7;
constexpr int VAL_POTION_LEVEL              = 0;
constexpr int VAL_POTION_SPELL1             = 1;
constexpr int VAL_POTION_SPELL2             = 2;
constexpr int VAL_POTION_SPELL3             = 3;
constexpr int VAL_POTION_HEALTH             = 4;
constexpr int VAL_POTION_MAXHEALTH          = 5;
constexpr int VAL_POTION_UNUSED7            = 6;
constexpr int VAL_POTION_MATERIAL           = 7;
constexpr int VAL_WORN_UNUSED1              = 0;
constexpr int VAL_WORN_UNUSED2              = 1;
constexpr int VAL_WORN_UNUSED3              = 2;
constexpr int VAL_WORN_UNUSED4              = 3;
constexpr int VAL_WORN_HEALTH               = 4;
constexpr int VAL_WORN_MAXHEALTH            = 5;
constexpr int VAL_WORN_UNUSED7              = 6;
constexpr int VAL_WORN_MATERIAL             = 7;
constexpr int VAL_OTHER_UNUSED1             = 0;
constexpr int VAL_OTHER_UNUSED2             = 1;
constexpr int VAL_OTHER_UNUSED3             = 2;
constexpr int VAL_OTHER_UNUSED4             = 3;
constexpr int VAL_OTHER_HEALTH              = 4;
constexpr int VAL_OTHER_MAXHEALTH           = 5;
constexpr int VAL_OTHER_UNUSED7             = 6;
constexpr int VAL_OTHER_MATERIAL            = 7;
constexpr int VAL_TRASH_UNUSED1             = 0;
constexpr int VAL_TRASH_UNUSED2             = 1;
constexpr int VAL_TRASH_UNUSED3             = 2;
constexpr int VAL_TRASH_UNUSED4             = 3;
constexpr int VAL_TRASH_HEALTH              = 4;
constexpr int VAL_TRASH_MAXHEALTH           = 5;
constexpr int VAL_TRASH_UNUSED7             = 6;
constexpr int VAL_TRASH_MATERIAL            = 7;
constexpr int VAL_TRAP_SPELL                = 0;
constexpr int VAL_TRAP_HITPOINTS            = 1;
constexpr int VAL_TRAP_UNUSED3              = 2;
constexpr int VAL_TRAP_UNUSED4              = 3;
constexpr int VAL_TRAP_HEALTH               = 4;
constexpr int VAL_TRAP_MAXHEALTH            = 5;
constexpr int VAL_TRAP_UNUSED7              = 6;
constexpr int VAL_TRAP_MATERIAL             = 7;
constexpr int VAL_CONTAINER_CAPACITY        = 0;
constexpr int VAL_CONTAINER_FLAGS           = 1;
constexpr int VAL_CONTAINER_KEY             = 2;
constexpr int VAL_CONTAINER_CORPSE          = 3;
constexpr int VAL_CONTAINER_HEALTH          = 4;
constexpr int VAL_CONTAINER_MAXHEALTH       = 5;
constexpr int VAL_CONTAINER_UNUSED7         = 6;
constexpr int VAL_CONTAINER_MATERIAL        = 7;
constexpr int VAL_CONTAINER_OWNER           = 8;
constexpr int VAL_NOTE_LANGUAGE             = 0;
constexpr int VAL_NOTE_UNUSED2              = 1;
constexpr int VAL_NOTE_UNUSED3              = 2;
constexpr int VAL_NOTE_UNUSED4              = 3;
constexpr int VAL_NOTE_HEALTH               = 4;
constexpr int VAL_NOTE_MAXHEALTH            = 5;
constexpr int VAL_NOTE_UNUSED7              = 6;
constexpr int VAL_NOTE_MATERIAL             = 7;
constexpr int VAL_DRINKCON_CAPACITY         = 0;
constexpr int VAL_DRINKCON_HOWFULL          = 1;
constexpr int VAL_DRINKCON_LIQUID           = 2;
constexpr int VAL_DRINKCON_POISON           = 3;
constexpr int VAL_DRINKCON_HEALTH           = 4;
constexpr int VAL_DRINKCON_MAXHEALTH        = 5;
constexpr int VAL_DRINKCON_UNUSED7          = 6;
constexpr int VAL_DRINKCON_MATERIAL         = 7;
constexpr int VAL_KEY_UNUSED1               = 0;
constexpr int VAL_KEY_UNUSED2               = 1;
constexpr int VAL_KEY_KEYCODE               = 2;
constexpr int VAL_KEY_UNUSED4               = 3;
constexpr int VAL_KEY_HEALTH                = 4;
constexpr int VAL_KEY_MAXHEALTH             = 5;
constexpr int VAL_KEY_UNUSED7               = 6;
constexpr int VAL_KEY_MATERIAL              = 7;
constexpr int VAL_FOOD_FOODVAL              = 0;
constexpr int VAL_FOOD_MAXFOODVAL           = 1;
constexpr int VAL_FOOD_PSBONUS              = 2;
constexpr int VAL_FOOD_POISON               = 3;
constexpr int VAL_FOOD_HEALTH               = 4;
constexpr int VAL_FOOD_MAXHEALTH            = 5;
constexpr int VAL_FOOD_EXPBONUS             = 6;
constexpr int VAL_FOOD_MATERIAL             = 7;
constexpr int VAL_MONEY_SIZE                = 0;
constexpr int VAL_MONEY_UNUSED2             = 1;
constexpr int VAL_MONEY_UNUSED3             = 2;
constexpr int VAL_MONEY_UNUSED4             = 3;
constexpr int VAL_MONEY_HEALTH              = 4;
constexpr int VAL_MONEY_MAXHEALTH           = 5;
constexpr int VAL_MONEY_UNUSED7             = 6;
constexpr int VAL_MONEY_MATERIAL            = 7;
constexpr int VAL_PEN_UNUSED1               = 0;
constexpr int VAL_PEN_UNUSED2               = 1;
constexpr int VAL_PEN_UNUSED3               = 2;
constexpr int VAL_PEN_UNUSED4               = 3;
constexpr int VAL_PEN_HEALTH                = 4;
constexpr int VAL_PEN_MAXHEALTH             = 5;
constexpr int VAL_PEN_UNUSED7               = 6;
constexpr int VAL_PEN_MATERIAL              = 7;
constexpr int VAL_BOAT_UNUSED1              = 0;
constexpr int VAL_BOAT_UNUSED2              = 1;
constexpr int VAL_BOAT_UNUSED3              = 2;
constexpr int VAL_BOAT_UNUSED4              = 3;
constexpr int VAL_BOAT_HEALTH               = 4;
constexpr int VAL_BOAT_MAXHEALTH            = 5;
constexpr int VAL_BOAT_UNUSED7              = 6;
constexpr int VAL_BOAT_MATERIAL             = 7;
constexpr int VAL_FOUNTAIN_CAPACITY         = 0;
constexpr int VAL_FOUNTAIN_HOWFULL          = 1;
constexpr int VAL_FOUNTAIN_LIQUID           = 2;
constexpr int VAL_FOUNTAIN_POISON           = 3;
constexpr int VAL_FOUNTAIN_HEALTH           = 4;
constexpr int VAL_FOUNTAIN_MAXHEALTH        = 5;
constexpr int VAL_FOUNTAIN_UNUSED7          = 6;
constexpr int VAL_FOUNTAIN_MATERIAL         = 7;
constexpr int VAL_VEHICLE_ROOM              = 0;
constexpr int VAL_VEHICLE_UNUSED2           = 1;
constexpr int VAL_VEHICLE_UNUSED3           = 2;
constexpr int VAL_VEHICLE_APPEAR            = 3;
constexpr int VAL_VEHICLE_HEALTH            = 4;
constexpr int VAL_VEHICLE_MAXHEALTH         = 5;
constexpr int VAL_VEHICLE_UNUSED7           = 6;
constexpr int VAL_VEHICLE_MATERIAL          = 7;
constexpr int VAL_HATCH_DEST                = 0;
constexpr int VAL_HATCH_FLAGS               = 1;
constexpr int VAL_HATCH_DCSKILL             = 2;
constexpr int VAL_HATCH_DCMOVE              = 3;
constexpr int VAL_HATCH_HEALTH              = 4;
constexpr int VAL_HATCH_MAXHEALTH           = 5;
constexpr int VAL_HATCH_UNUSED7             = 6;
constexpr int VAL_HATCH_MATERIAL            = 7;
constexpr int VAL_HATCH_DCLOCK              = 8;
constexpr int VAL_HATCH_DCHIDE              = 9;
constexpr int VAL_WINDOW_UNUSED1            = 0;
constexpr int VAL_WINDOW_UNUSED2            = 1;
constexpr int VAL_WINDOW_UNUSED3            = 2;
constexpr int VAL_WINDOW_UNUSED4            = 3;
constexpr int VAL_WINDOW_HEALTH             = 4;
constexpr int VAL_WINDOW_MAXHEALTH          = 5;
constexpr int VAL_WINDOW_UNUSED7            = 6;
constexpr int VAL_WINDOW_MATERIAL           = 7;
constexpr int VAL_CONTROL_UNUSED1           = 0;
constexpr int VAL_CONTROL_UNUSED2           = 1;
constexpr int VAL_CONTROL_UNUSED3           = 2;
constexpr int VAL_CONTROL_UNUSED4           = 3;
constexpr int VAL_CONTROL_HEALTH            = 4;
constexpr int VAL_CONTROL_MAXHEALTH         = 5;
constexpr int VAL_CONTROL_UNUSED7           = 6;
constexpr int VAL_CONTROL_MATERIAL          = 7;
constexpr int VAL_PORTAL_DEST               = 0;
constexpr int VAL_PORTAL_DCSKILL            = 1;
constexpr int VAL_PORTAL_DCMOVE             = 2;
constexpr int VAL_PORTAL_APPEAR             = 3;
constexpr int VAL_PORTAL_HEALTH             = 4;
constexpr int VAL_PORTAL_MAXHEALTH          = 5;
constexpr int VAL_PORTAL_UNUSED7            = 6;
constexpr int VAL_PORTAL_MATERIAL           = 7;
constexpr int VAL_PORTAL_DCLOCK             = 8;
constexpr int VAL_PORTAL_DCHIDE             = 9;
constexpr int VAL_BOARD_READ                = 0;
constexpr int VAL_BOARD_WRITE               = 1;
constexpr int VAL_BOARD_ERASE               = 2;
constexpr int VAL_BOARD_UNUSED4             = 3;
constexpr int VAL_BOARD_HEALTH              = 4;
constexpr int VAL_BOARD_MAXHEALTH           = 5;
constexpr int VAL_BOARD_UNUSED7             = 6;
constexpr int VAL_BOARD_MATERIAL            = 7;
constexpr int VAL_DOOR_DCLOCK               = 8;
constexpr int VAL_DOOR_DCHIDE               = 9;
constexpr int VAL_CORPSE_HEAD               = 8;
constexpr int VAL_CORPSE_RARM               = 9;
constexpr int VAL_CORPSE_LARM              = 10;
constexpr int VAL_CORPSE_RLEG              = 11;
constexpr int VAL_CORPSE_LLEG              = 12;
constexpr int VAL_GROWTH                    = 0;
constexpr int VAL_MATGOAL                   = 1;
constexpr int VAL_MATURITY                  = 2;
constexpr int VAL_MAXMATURE                 = 3;
constexpr int VAL_WATERLEVEL                = 6;
constexpr int VAL_SOILQ                     = 8;

constexpr int NUM_FULLNESS		= 5;

constexpr int EF_ARRAY_MAX    = 4;
constexpr int TW_ARRAY_MAX    = 4;

extern const char *equipment_types[NUM_WEARS+1];
extern const char *wear_where[NUM_WEARS+1];
extern const char *item_types[NUM_ITEM_TYPES+1];
extern const char *wear_bits[NUM_ITEM_WEARS+1];
extern const char *extra_bits[NUM_ITEM_FLAGS+1];
extern const char *container_bits[NUM_CONT_FLAGS+1];


extern const char *drinks[NUM_LIQ_TYPES+1];
extern const char *drinknames[NUM_LIQ_TYPES+1];
extern const char *color_liquid[NUM_LIQ_TYPES+1];
extern int drink_aff[NUM_LIQ_TYPES][NUM_CONDITIONS];
extern const char *fullness[NUM_FULLNESS+1];
extern const char *material_names[NUM_MATERIALS+1];