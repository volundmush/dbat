#pragma once
#include <array>
#include <cstdint>
#include <string_view>

#include "dbat/consts/conditions.hpp"
#include "dbat/consts/materials.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
constexpr int WEAR_UNUSED0  = 0;
constexpr int WEAR_FINGER_R = 1;
constexpr int WEAR_FINGER_L = 2;
constexpr int WEAR_NECK_1   = 3;
constexpr int WEAR_NECK_2   = 4;
constexpr int WEAR_BODY     = 5;
constexpr int WEAR_HEAD     = 6;
constexpr int WEAR_LEGS     = 7;
constexpr int WEAR_FEET     = 8;
constexpr int WEAR_HANDS    = 9;
constexpr int WEAR_ARMS     = 10;
constexpr int WEAR_UNUSED1  = 11;
constexpr int WEAR_ABOUT    = 12;
constexpr int WEAR_WAIST    = 13;
constexpr int WEAR_WRIST_R  = 14;
constexpr int WEAR_WRIST_L  = 15;
constexpr int WEAR_WIELD1   = 16;
constexpr int WEAR_WIELD2   = 17;
constexpr int WEAR_BACKPACK = 18;
constexpr int WEAR_EAR_R    = 19;
constexpr int WEAR_EAR_L    = 20;
constexpr int WEAR_SH       = 21;
constexpr int WEAR_EYE      = 22;

constexpr int NUM_WEARS = 23; /* This must be the # of eq positions!! */

extern const name_table equipment_types; /* NUM_WEARS of them */

/* Body parts: the equipment positions a character's body has (char_data.bodyparts), each the WEAR_ position's number */
enum class body_part : uint8_t {
  unused0  = 0,
  finger_r = 1,
  finger_l = 2,
  neck_1   = 3,
  neck_2   = 4,
  body     = 5,
  head     = 6,
  legs     = 7,
  feet     = 8,
  hands    = 9,
  arms     = 10,
  unused1  = 11,
  about    = 12,
  waist    = 13,
  wrist_r  = 14,
  wrist_l  = 15,
  wield1   = 16,
  wield2   = 17,
  backpack = 18,
  ear_r    = 19,
  ear_l    = 20,
  sh       = 21,
  eye      = 22,
  count_   = 23,
};
template <> struct flag_traits<body_part> {
  static constexpr const name_table &names = equipment_types;
};
static_assert(flag_count<body_part> == NUM_WEARS, "a body part for every equipment position");

/* object-related defines ********************************************/

/* Item types: used by obj_data.type_flag */
constexpr int ITEM_LIGHT      = 1;  /* Item is a light source	*/
constexpr int ITEM_SCROLL     = 2;  /* Item is a scroll		*/
constexpr int ITEM_WAND       = 3;  /* Item is a wand		*/
constexpr int ITEM_STAFF      = 4;  /* Item is a staff		*/
constexpr int ITEM_WEAPON     = 5;  /* Item is a weapon		*/
constexpr int ITEM_FIREWEAPON = 6;  /* Unimplemented		*/
constexpr int ITEM_CAMPFIRE   = 7;  /* Burn things for fun!		*/
constexpr int ITEM_TREASURE   = 8;  /* Item is a treasure, not gold	*/
constexpr int ITEM_ARMOR      = 9;  /* Item is armor		*/
constexpr int ITEM_POTION     = 10; /* Item is a potion		*/
constexpr int ITEM_WORN       = 11; /* Unimplemented		*/
constexpr int ITEM_OTHER      = 12; /* Misc object			*/
constexpr int ITEM_TRASH      = 13; /* Trash - shopkeeps won't buy	*/
constexpr int ITEM_TRAP       = 14; /* Unimplemented		*/
constexpr int ITEM_CONTAINER  = 15; /* Item is a container		*/
constexpr int ITEM_NOTE       = 16; /* Item is note 		*/
constexpr int ITEM_DRINKCON   = 17; /* Item is a drink container	*/
constexpr int ITEM_KEY        = 18; /* Item is a key		*/
constexpr int ITEM_FOOD       = 19; /* Item is food			*/
constexpr int ITEM_MONEY      = 20; /* Item is money (gold)		*/
constexpr int ITEM_PEN        = 21; /* Item is a pen		*/
constexpr int ITEM_BOAT       = 22; /* Item is a boat		*/
constexpr int ITEM_FOUNTAIN   = 23; /* Item is a fountain		*/
constexpr int ITEM_VEHICLE    = 24; /* Item is a vehicle            */
constexpr int ITEM_HATCH      = 25; /* Item is a vehicle hatch      */
constexpr int ITEM_WINDOW     = 26; /* Item is a vehicle window     */
constexpr int ITEM_CONTROL    = 27; /* Item is a vehicle control    */
constexpr int ITEM_PORTAL     = 28; /* Item is a portal	        */
constexpr int ITEM_SPELLBOOK  = 29; /* Item is a spellbook	        */
constexpr int ITEM_BOARD      = 30; /* Item is a message board 	*/
constexpr int ITEM_CHAIR      = 31; /* Is a chair                   */
constexpr int ITEM_BED        = 32; /* Is a bed                     */
constexpr int ITEM_YUM        = 33; /* This was good food           */
constexpr int ITEM_PLANT      = 34; /* This will grow!              */
constexpr int ITEM_FISHPOLE   = 35; /* FOR FISHING                  */
constexpr int ITEM_FISHBAIT   = 36; /* DITTO                        */

constexpr int NUM_ITEM_TYPES = 37;

extern const name_table wear_bits; /* flag_count<wear_flag> of them */

/* Take/Wear flags: used by obj_data.wear_flags */
enum class wear_flag : uint8_t {
  take   = 0,   /* Item can be taken         */
  finger = 1,   /* Can be worn on finger     */
  neck   = 2,   /* Can be worn around neck   */
  body   = 3,   /* Can be worn on body       */
  head   = 4,   /* Can be worn on head       */
  legs   = 5,   /* Can be worn on legs       */
  feet   = 6,   /* Can be worn on feet       */
  hands  = 7,   /* Can be worn on hands      */
  arms   = 8,   /* Can be worn on arms       */
  shield = 9,   /* Can be used as a shield   */
  about  = 10,  /* Can be worn about body    */
  waist  = 11,  /* Can be worn around waist  */
  wrist  = 12,  /* Can be worn on wrist      */
  wield  = 13,  /* Can be wielded            */
  hold   = 14,  /* Can be held               */
  pack   = 15,  /* Can be worn as a backpack */
  ear    = 16,  /* Can be worn as an earring */
  sh     = 17,  /* Can be worn as wings      */
  eye    = 18,  /* Can be worn as a mask     */
  count_ = 19,
};
template <> struct flag_traits<wear_flag> {
  static constexpr const name_table &names = wear_bits;
};


extern const name_table extra_bits; /* flag_count<extra_flag> of them */

/* Extra object flags: used by obj_data.extra_flags */
enum class extra_flag : uint8_t {
  glow                  = 0,   /* Item is glowing              */
  hum                   = 1,   /* Item is humming              */
  norent                = 2,   /* Item cannot be rented        */
  nodonate              = 3,   /* Item cannot be donated       */
  noinvis               = 4,   /* Item cannot be made invis    */
  invisible             = 5,   /* Item is invisible            */
  magic                 = 6,   /* Item is magical              */
  nodrop                = 7,   /* Item is cursed: can't drop   */
  bless                 = 8,   /* Item is blessed              */
  anti_good             = 9,   /* Not usable by good people    */
  anti_evil             = 10,  /* Not usable by evil people    */
  anti_neutral          = 11,  /* Not usable by neutral people */
  anti_wizard           = 12,  /* Not usable by mages          */
  anti_cleric           = 13,  /* Not usable by clerics        */
  anti_rogue            = 14,  /* Not usable by thieves        */
  anti_fighter          = 15,  /* Not usable by warriors       */
  nosell                = 16,  /* Shopkeepers won't touch it   */
  anti_druid            = 17,  /* Not usable by druids         */
  two_handed                  = 18,  /* Requires two free hands      */
  anti_bard             = 19,  /* Not usable by bards          */
  anti_ranger           = 20,  /* Not usable by rangers        */
  anti_paladin          = 21,  /* Not usable by paladins       */
  anti_human            = 22,  /* Not usable by humans         */
  anti_icer             = 23,  /* Not usable by dwarves        */
  anti_saiyan           = 24,  /* Not usable by elves          */
  anti_konatsu          = 25,  /* Not usable by gnomes         */
  unique_save           = 26,  /* unique object save           */
  broken                = 27,  /* Item is broken hands         */
  unbreakable           = 28,  /* Item is unbreakable          */
  anti_monk             = 29,  /* Not usable by monks          */
  anti_barbarian        = 30,  /* Not usable by barbarians     */
  anti_sorcerer         = 31,  /* Not usable by sorcerers      */
  double_               = 32,  /* Double weapon                */
  only_wizard           = 33,  /* Only usable by mages         */
  only_cleric           = 34,  /* Only usable by clerics       */
  only_rogue            = 35,  /* Only usable by thieves       */
  only_fighter          = 36,  /* Only usable by warriors      */
  only_druid            = 37,  /* Only usable by druids        */
  only_bard             = 38,  /* Only usable by bards         */
  only_ranger           = 39,  /* Only usable by rangers       */
  only_paladin          = 40,  /* Only usable by paladins      */
  only_human            = 41,  /* Only usable by humans        */
  only_icer             = 42,  /* Only usable by dwarves       */
  only_saiyan           = 43,  /* Only usable by elves         */
  only_konatsu          = 44,  /* Only usable by gnomes        */
  only_monk             = 45,  /* Only usable by monks         */
  only_barbarian        = 46,  /* Only usable by barbarians    */
  only_sorcerer         = 47,  /* Only usable by sorcerers     */
  anti_arcane_archer    = 48,  /* Not usable by Aracane Archers          */
  anti_arcane_trickster = 49,  /* Not usable by Aracane tricksters       */
  anti_archmage         = 50,  /* Not usable by Archmages                */
  anti_assassin         = 51,  /* Not usable by Assassins                */
  anti_blackguard       = 52,  /* Not usable by Blackguards              */
  anti_dragon_disciple  = 53,  /* Not usable by Dragon disciples         */
  anti_duelist          = 54,  /* Not usable by Duelists                 */
  anti_dwarven_defender = 55,  /* Not usable by Dwarven defenders        */
  anti_eldritch_knight  = 56,  /* Not usable by Eldritch knights         */
  anti_hierophant       = 57,  /* Not usable by Hierophants              */
  anti_horizon_walker   = 58,  /* Not usable by Horizon walkers          */
  anti_loremaster       = 59,  /* Not usable by Loremasters              */
  anti_mystic_theurge   = 60,  /* Not usable by Mystic theurges          */
  anti_shadowdancer     = 61,  /* Not useable by shadowdancers           */
  anti_thaumaturgist    = 62,  /* Not useable by thaumaturgists          */
  bscouter              = 63,
  mscouter              = 64,
  ascouter              = 65,
  uscouter              = 66,
  weaplvl1              = 67,
  weaplvl2              = 68,
  weaplvl3              = 69,
  weaplvl4              = 70,
  weaplvl5              = 71,
  cboard                = 72,
  forged                = 73,
  sheath                = 74,
  only_jinto            = 75,
  buried                = 76,
  slot1                 = 77,
  slot2                 = 78,
  token                 = 79,
  slot_one              = 80,
  slots_filled          = 81,
  restring              = 82,
  custom                = 83,
  protected_           = 84,
  norandom              = 85,
  throw_                = 86,
  hot                   = 87,
  purge                 = 88,
  ice                   = 89,
  duplicate             = 90,
  mature                = 91,
  cardcase              = 92,
  nopickup              = 93,
  nosteal               = 94,
  count_                = 95,
};
template <> struct flag_traits<extra_flag> {
  static constexpr const name_table &names = extra_bits;
};


/* Container flags: value[1] (VAL_CONTAINER_FLAGS), as one word; obj_container_flags() reads them */
extern const name_table container_bits; /* flag_count<container_flag> of them */
enum class container_flag : uint8_t {
  closeable = 0, /* Container can be closed	*/
  pickproof = 1, /* Container is pickproof	*/
  closed    = 2, /* Container is closed		*/
  locked    = 3, /* Container is locked		*/
  count_    = 4,
};
template <> struct flag_traits<container_flag> {
  static constexpr const name_table &names = container_bits;
};


/* object-related structures ******************************************/
constexpr int NUM_OBJ_VAL_POSITIONS = 16;

constexpr int VAL_ALL_HEALTH    = 4;
constexpr int VAL_ALL_MAXHEALTH = 5;
constexpr int VAL_ALL_MATERIAL  = 7;
/*
 * Uses for generic object values on specific object types
 * Please use these instead of numbers to prevent overlaps.
 */
constexpr int VAL_LIGHT_TIME           = 0;
constexpr int VAL_LIGHT_UNUSED2        = 1;
constexpr int VAL_LIGHT_HOURS          = 2;
constexpr int VAL_LIGHT_UNUSED4        = 3;
constexpr int VAL_LIGHT_HEALTH         = 4;
constexpr int VAL_LIGHT_MAXHEALTH      = 5;
constexpr int VAL_LIGHT_UNUSED7        = 6;
constexpr int VAL_LIGHT_MATERIAL       = 7;
constexpr int VAL_SCROLL_LEVEL         = 0;
constexpr int VAL_SCROLL_SPELL1        = 1;
constexpr int VAL_SCROLL_SPELL2        = 2;
constexpr int VAL_SCROLL_SPELL3        = 3;
constexpr int VAL_SCROLL_HEALTH        = 4;
constexpr int VAL_SCROLL_MAXHEALTH     = 5;
constexpr int VAL_SCROLL_UNUSED7       = 6;
constexpr int VAL_SCROLL_MATERIAL      = 7;
constexpr int VAL_WAND_LEVEL           = 0;
constexpr int VAL_WAND_MAXCHARGES      = 1;
constexpr int VAL_WAND_CHARGES         = 2;
constexpr int VAL_WAND_SPELL           = 3;
constexpr int VAL_WAND_HEALTH          = 4;
constexpr int VAL_WAND_MAXHEALTH       = 5;
constexpr int VAL_WAND_UNUSED7         = 6;
constexpr int VAL_WAND_MATERIAL        = 7;
constexpr int VAL_STAFF_LEVEL          = 0;
constexpr int VAL_STAFF_MAXCHARGES     = 1;
constexpr int VAL_STAFF_CHARGES        = 2;
constexpr int VAL_STAFF_SPELL          = 3;
constexpr int VAL_STAFF_HEALTH         = 4;
constexpr int VAL_STAFF_MAXHEALTH      = 5;
constexpr int VAL_STAFF_UNUSED7        = 6;
constexpr int VAL_STAFF_MATERIAL       = 7;
constexpr int VAL_WEAPON_SKILL         = 0;
constexpr int VAL_WEAPON_DAMDICE       = 1;
constexpr int VAL_WEAPON_DAMSIZE       = 2;
constexpr int VAL_WEAPON_DAMTYPE       = 3;
constexpr int VAL_WEAPON_HEALTH        = 4;
constexpr int VAL_WEAPON_MAXHEALTH     = 5;
constexpr int VAL_WEAPON_CRITTYPE      = 6;
constexpr int VAL_WEAPON_MATERIAL      = 7;
constexpr int VAL_WEAPON_CRITRANGE     = 8;
constexpr int VAL_FIREWEAPON_UNUSED1   = 0;
constexpr int VAL_FIREWEAPON_UNUSED2   = 1;
constexpr int VAL_FIREWEAPON_UNUSED3   = 2;
constexpr int VAL_FIREWEAPON_UNUSED4   = 3;
constexpr int VAL_FIREWEAPON_HEALTH    = 4;
constexpr int VAL_FIREWEAPON_MAXHEALTH = 5;
constexpr int VAL_FIREWEAPON_UNUSED7   = 6;
constexpr int VAL_FIREWEAPON_MATERIAL  = 7;
constexpr int VAL_MISSILE_UNUSED1      = 0;
constexpr int VAL_MISSILE_UNUSED2      = 1;
constexpr int VAL_MISSILE_UNUSED3      = 2;
constexpr int VAL_MISSILE_UNUSED4      = 3;
constexpr int VAL_MISSILE_HEALTH       = 4;
constexpr int VAL_MISSILE_MAXHEALTH    = 5;
constexpr int VAL_MISSILE_UNUSED7      = 6;
constexpr int VAL_MISSILE_MATERIAL     = 7;
constexpr int VAL_TREASURE_UNUSED1     = 0;
constexpr int VAL_TREASURE_UNUSED2     = 1;
constexpr int VAL_TREASURE_UNUSED3     = 2;
constexpr int VAL_TREASURE_UNUSED4     = 3;
constexpr int VAL_TREASURE_HEALTH      = 4;
constexpr int VAL_TREASURE_MAXHEALTH   = 5;
constexpr int VAL_TREASURE_UNUSED7     = 6;
constexpr int VAL_TREASURE_MATERIAL    = 7;
constexpr int VAL_ARMOR_APPLYAC        = 0;
constexpr int VAL_ARMOR_SKILL          = 1;
constexpr int VAL_ARMOR_MAXDEXMOD      = 2;
constexpr int VAL_ARMOR_CHECK          = 3;
constexpr int VAL_ARMOR_HEALTH         = 4;
constexpr int VAL_ARMOR_MAXHEALTH      = 5;
constexpr int VAL_ARMOR_SPELLFAIL      = 6;
constexpr int VAL_ARMOR_MATERIAL       = 7;
constexpr int VAL_POTION_LEVEL         = 0;
constexpr int VAL_POTION_SPELL1        = 1;
constexpr int VAL_POTION_SPELL2        = 2;
constexpr int VAL_POTION_SPELL3        = 3;
constexpr int VAL_POTION_HEALTH        = 4;
constexpr int VAL_POTION_MAXHEALTH     = 5;
constexpr int VAL_POTION_UNUSED7       = 6;
constexpr int VAL_POTION_MATERIAL      = 7;
constexpr int VAL_WORN_UNUSED1         = 0;
constexpr int VAL_WORN_UNUSED2         = 1;
constexpr int VAL_WORN_UNUSED3         = 2;
constexpr int VAL_WORN_UNUSED4         = 3;
constexpr int VAL_WORN_HEALTH          = 4;
constexpr int VAL_WORN_MAXHEALTH       = 5;
constexpr int VAL_WORN_UNUSED7         = 6;
constexpr int VAL_WORN_MATERIAL        = 7;
constexpr int VAL_OTHER_UNUSED1        = 0;
constexpr int VAL_OTHER_UNUSED2        = 1;
constexpr int VAL_OTHER_UNUSED3        = 2;
constexpr int VAL_OTHER_UNUSED4        = 3;
constexpr int VAL_OTHER_HEALTH         = 4;
constexpr int VAL_OTHER_MAXHEALTH      = 5;
constexpr int VAL_OTHER_UNUSED7        = 6;
constexpr int VAL_OTHER_MATERIAL       = 7;
constexpr int VAL_TRASH_UNUSED1        = 0;
constexpr int VAL_TRASH_UNUSED2        = 1;
constexpr int VAL_TRASH_UNUSED3        = 2;
constexpr int VAL_TRASH_UNUSED4        = 3;
constexpr int VAL_TRASH_HEALTH         = 4;
constexpr int VAL_TRASH_MAXHEALTH      = 5;
constexpr int VAL_TRASH_UNUSED7        = 6;
constexpr int VAL_TRASH_MATERIAL       = 7;
constexpr int VAL_TRAP_SPELL           = 0;
constexpr int VAL_TRAP_HITPOINTS       = 1;
constexpr int VAL_TRAP_UNUSED3         = 2;
constexpr int VAL_TRAP_UNUSED4         = 3;
constexpr int VAL_TRAP_HEALTH          = 4;
constexpr int VAL_TRAP_MAXHEALTH       = 5;
constexpr int VAL_TRAP_UNUSED7         = 6;
constexpr int VAL_TRAP_MATERIAL        = 7;
constexpr int VAL_CONTAINER_CAPACITY   = 0;
constexpr int VAL_CONTAINER_FLAGS      = 1;
constexpr int VAL_CONTAINER_KEY        = 2;
constexpr int VAL_CONTAINER_CORPSE     = 3;
constexpr int VAL_CONTAINER_HEALTH     = 4;
constexpr int VAL_CONTAINER_MAXHEALTH  = 5;
constexpr int VAL_CONTAINER_UNUSED7    = 6;
constexpr int VAL_CONTAINER_MATERIAL   = 7;
constexpr int VAL_CONTAINER_OWNER      = 8;
constexpr int VAL_NOTE_LANGUAGE        = 0;
constexpr int VAL_NOTE_UNUSED2         = 1;
constexpr int VAL_NOTE_UNUSED3         = 2;
constexpr int VAL_NOTE_UNUSED4         = 3;
constexpr int VAL_NOTE_HEALTH          = 4;
constexpr int VAL_NOTE_MAXHEALTH       = 5;
constexpr int VAL_NOTE_UNUSED7         = 6;
constexpr int VAL_NOTE_MATERIAL        = 7;
constexpr int VAL_DRINKCON_CAPACITY    = 0;
constexpr int VAL_DRINKCON_HOWFULL     = 1;
constexpr int VAL_DRINKCON_LIQUID      = 2;
constexpr int VAL_DRINKCON_POISON      = 3;
constexpr int VAL_DRINKCON_HEALTH      = 4;
constexpr int VAL_DRINKCON_MAXHEALTH   = 5;
constexpr int VAL_DRINKCON_UNUSED7     = 6;
constexpr int VAL_DRINKCON_MATERIAL    = 7;
constexpr int VAL_KEY_UNUSED1          = 0;
constexpr int VAL_KEY_UNUSED2          = 1;
constexpr int VAL_KEY_KEYCODE          = 2;
constexpr int VAL_KEY_UNUSED4          = 3;
constexpr int VAL_KEY_HEALTH           = 4;
constexpr int VAL_KEY_MAXHEALTH        = 5;
constexpr int VAL_KEY_UNUSED7          = 6;
constexpr int VAL_KEY_MATERIAL         = 7;
constexpr int VAL_FOOD_FOODVAL         = 0;
constexpr int VAL_FOOD_MAXFOODVAL      = 1;
constexpr int VAL_FOOD_PSBONUS         = 2;
constexpr int VAL_FOOD_POISON          = 3;
constexpr int VAL_FOOD_HEALTH          = 4;
constexpr int VAL_FOOD_MAXHEALTH       = 5;
constexpr int VAL_FOOD_EXPBONUS        = 6;
constexpr int VAL_FOOD_MATERIAL        = 7;
constexpr int VAL_MONEY_SIZE           = 0;
constexpr int VAL_MONEY_UNUSED2        = 1;
constexpr int VAL_MONEY_UNUSED3        = 2;
constexpr int VAL_MONEY_UNUSED4        = 3;
constexpr int VAL_MONEY_HEALTH         = 4;
constexpr int VAL_MONEY_MAXHEALTH      = 5;
constexpr int VAL_MONEY_UNUSED7        = 6;
constexpr int VAL_MONEY_MATERIAL       = 7;
constexpr int VAL_PEN_UNUSED1          = 0;
constexpr int VAL_PEN_UNUSED2          = 1;
constexpr int VAL_PEN_UNUSED3          = 2;
constexpr int VAL_PEN_UNUSED4          = 3;
constexpr int VAL_PEN_HEALTH           = 4;
constexpr int VAL_PEN_MAXHEALTH        = 5;
constexpr int VAL_PEN_UNUSED7          = 6;
constexpr int VAL_PEN_MATERIAL         = 7;
constexpr int VAL_BOAT_UNUSED1         = 0;
constexpr int VAL_BOAT_UNUSED2         = 1;
constexpr int VAL_BOAT_UNUSED3         = 2;
constexpr int VAL_BOAT_UNUSED4         = 3;
constexpr int VAL_BOAT_HEALTH          = 4;
constexpr int VAL_BOAT_MAXHEALTH       = 5;
constexpr int VAL_BOAT_UNUSED7         = 6;
constexpr int VAL_BOAT_MATERIAL        = 7;
constexpr int VAL_FOUNTAIN_CAPACITY    = 0;
constexpr int VAL_FOUNTAIN_HOWFULL     = 1;
constexpr int VAL_FOUNTAIN_LIQUID      = 2;
constexpr int VAL_FOUNTAIN_POISON      = 3;
constexpr int VAL_FOUNTAIN_HEALTH      = 4;
constexpr int VAL_FOUNTAIN_MAXHEALTH   = 5;
constexpr int VAL_FOUNTAIN_UNUSED7     = 6;
constexpr int VAL_FOUNTAIN_MATERIAL    = 7;
constexpr int VAL_VEHICLE_ROOM         = 0;
constexpr int VAL_VEHICLE_UNUSED2      = 1;
constexpr int VAL_VEHICLE_UNUSED3      = 2;
constexpr int VAL_VEHICLE_APPEAR       = 3;
constexpr int VAL_VEHICLE_HEALTH       = 4;
constexpr int VAL_VEHICLE_MAXHEALTH    = 5;
constexpr int VAL_VEHICLE_UNUSED7      = 6;
constexpr int VAL_VEHICLE_MATERIAL     = 7;
constexpr int VAL_HATCH_DEST           = 0;
constexpr int VAL_HATCH_FLAGS          = 1;
constexpr int VAL_HATCH_DCSKILL        = 2;
constexpr int VAL_HATCH_DCMOVE         = 3;
constexpr int VAL_HATCH_HEALTH         = 4;
constexpr int VAL_HATCH_MAXHEALTH      = 5;
constexpr int VAL_HATCH_UNUSED7        = 6;
constexpr int VAL_HATCH_MATERIAL       = 7;
constexpr int VAL_HATCH_DCLOCK         = 8;
constexpr int VAL_HATCH_DCHIDE         = 9;
constexpr int VAL_WINDOW_UNUSED1       = 0;
constexpr int VAL_WINDOW_UNUSED2       = 1;
constexpr int VAL_WINDOW_UNUSED3       = 2;
constexpr int VAL_WINDOW_UNUSED4       = 3;
constexpr int VAL_WINDOW_HEALTH        = 4;
constexpr int VAL_WINDOW_MAXHEALTH     = 5;
constexpr int VAL_WINDOW_UNUSED7       = 6;
constexpr int VAL_WINDOW_MATERIAL      = 7;
constexpr int VAL_CONTROL_UNUSED1      = 0;
constexpr int VAL_CONTROL_UNUSED2      = 1;
constexpr int VAL_CONTROL_UNUSED3      = 2;
constexpr int VAL_CONTROL_UNUSED4      = 3;
constexpr int VAL_CONTROL_HEALTH       = 4;
constexpr int VAL_CONTROL_MAXHEALTH    = 5;
constexpr int VAL_CONTROL_UNUSED7      = 6;
constexpr int VAL_CONTROL_MATERIAL     = 7;
constexpr int VAL_PORTAL_DEST          = 0;
constexpr int VAL_PORTAL_DCSKILL       = 1;
constexpr int VAL_PORTAL_DCMOVE        = 2;
constexpr int VAL_PORTAL_APPEAR        = 3;
constexpr int VAL_PORTAL_HEALTH        = 4;
constexpr int VAL_PORTAL_MAXHEALTH     = 5;
constexpr int VAL_PORTAL_UNUSED7       = 6;
constexpr int VAL_PORTAL_MATERIAL      = 7;
constexpr int VAL_PORTAL_DCLOCK        = 8;
constexpr int VAL_PORTAL_DCHIDE        = 9;
constexpr int VAL_BOARD_READ           = 0;
constexpr int VAL_BOARD_WRITE          = 1;
constexpr int VAL_BOARD_ERASE          = 2;
constexpr int VAL_BOARD_UNUSED4        = 3;
constexpr int VAL_BOARD_HEALTH         = 4;
constexpr int VAL_BOARD_MAXHEALTH      = 5;
constexpr int VAL_BOARD_UNUSED7        = 6;
constexpr int VAL_BOARD_MATERIAL       = 7;
constexpr int VAL_DOOR_DCLOCK          = 8;
constexpr int VAL_DOOR_DCHIDE          = 9;
constexpr int VAL_CORPSE_HEAD          = 8;
constexpr int VAL_CORPSE_RARM          = 9;
constexpr int VAL_CORPSE_LARM          = 10;
constexpr int VAL_CORPSE_RLEG          = 11;
constexpr int VAL_CORPSE_LLEG          = 12;
constexpr int VAL_GROWTH               = 0;
constexpr int VAL_MATGOAL              = 1;
constexpr int VAL_MATURITY             = 2;
constexpr int VAL_MAXMATURE            = 3;
constexpr int VAL_WATERLEVEL           = 6;
constexpr int VAL_SOILQ                = 8;

constexpr int NUM_FULLNESS = 5;


extern const name_table wear_where; /* NUM_WEARS of them */
extern const name_table item_types; /* NUM_ITEM_TYPES of them */

extern const name_table drinks; /* NUM_LIQ_TYPES of them */
extern const name_table drinknames; /* NUM_LIQ_TYPES of them */
extern const name_table color_liquid; /* NUM_LIQ_TYPES of them */
extern const std::array<std::array<int, NUM_CONDITIONS>, NUM_LIQ_TYPES> drink_aff;
extern const name_table material_names; /* NUM_MATERIALS of them */

