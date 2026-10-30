#include "dbat/db/consts/itemdata.h"


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
const char *wear_where[NUM_WEARS+1] = {
  "@c<@CUsed Somewhere     @c>@n ",
  "@c<@CWorn On Finger     @c>@n ",
  "@c<@CWorn On Finger     @c>@n ",
  "@c<@CWorn Around Neck   @c>@n ",
  "@c<@CWorn Around Neck   @c>@n ",
  "@c<@CWorn On Body       @c>@n ",
  "@c<@CWorn On Head       @c>@n ",
  "@c<@CWorn On Legs       @c>@n ",
  "@c<@CWorn On Feet       @c>@n ",
  "@c<@CWorn On Hands      @c>@n ",
  "@c<@CWorn On Arms       @c>@n ",
  "@c<@CWorn Somewhere     @c>@n ",
  "@c<@CWorn About Body    @c>@n ",
  "@c<@CWorn About Waist   @c>@n ",
  "@c<@CWorn Around Wrist  @c>@n ",
  "@c<@CWorn Around Wrist  @c>@n ",
  "@c<@CWielded            @c>@n ",
  "@c<@COffhand            @c>@n ",
  "@c<@CWorn On Back       @c>@n ",
  "@c<@CWorn In Ear        @c>@n ",
  "@c<@CWorn In Ear        @c>@n ",
  "@c<@CWorn On Shoulders  @c>@n ",
  "@c<@CWorn On Eye        @c>@n ",
  "\n"
};


/* WEAR_x - for stat */
const char *equipment_types[NUM_WEARS+1] = {
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "Worn on back",
  "Worn in ear",
  "Worn in ear",
  "Worn on shoulders",
  "Worn on eye",
  "\n"
};


/* ITEM_x (ordinal object types) */
const char *item_types[NUM_ITEM_TYPES+1] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FIRE WEAPON",
  "CAMPFIRE",
  "TREASURE",
  "ARMOR",
  "SENSU",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQCONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "VEHICLE",
  "HATCH",
  "WINDOW",
  "CONTROL",
  "PORTAL",
  "SPELLBOOK",
  "BOARD",
  "CHAIR",
  "BED",
  "YUM",
  "PLANT",
  "FISHINGPOLE",
  "FISHBAIT",
  "\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[NUM_ITEM_WEARS+1] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "BACK",
  "EAR",
  "SHOULDERS",
  "SCOUTER",
  "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[NUM_ITEM_FLAGS+1] = {
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "ANTI_ROSHI",
  "ANTI_PICCOLO",
  "ANTI_KRANE",
  "ANTI_NAIL",
  "NO_SELL",
  "ANTI_TAPION",
  "2H",
  "ANTI_ANDSIX",
  "ANTI_DABURA",
  "ANTI_GINYU",
  "ANTI_HUMAN",
  "ANTI_DWARF",
  "ANTI_ELF",
  "ANTI_GNOME",
  "UNIQUE",
  "BROKEN",
  "UNBREAKABLE",
  "ANTI_BARDOCK",
  "ANTI_KABITO",
  "ANTI_FRIEZA",
  "DOUBLE",
  "ONLY_ROSHI",
  "ONLY_PICCOLO",
  "ONLY_KRANE",
  "ONLY_NAIL",
  "ONLY_TAPION",
  "ONLY_ANDSIX",
  "ONLY_DABURA",
  "ONLY_GINYU",
  "ONLY_HUMAN",
  "ONLY_ICER",
  "ONLY_SAIYAN",
  "ONLY_KONATSU",
  "ONLY_BARDOCK",
  "ONLY_KABITO",
  "ONLY_FRIEZA",
  "ANTI_ARC_ARCH",
  "ANTI_ARC_TRICK",
  "ANTI_KURZAK",
  "ONLY_KURZAK",
  "ANTI_BLACKGUARD",
  "ANTI_DRAG_DIS",
  "ANTI_DUEL",
  "ANTI_DWAR_DEF",
  "ANTI_EL_KNIGHT",
  "ANTI_HIERO",
  "ANTI_HORI_WALK",
  "ANTI_LORE",
  "ANTI_MYSTIC_TH",
  "ANTI_SHADOW",
  "ANTI_THAUM",
  "Basic Scouter",
  "Midi Scouter",
  "Advanced Scouter",
  "Ultra Scouter",
  "Weapon Lvl 1",
  "Weapon Lvl 2",
  "Weapon Lvl 3",
  "Weapon Lvl 4",
  "Weapon Lvl 5",
  "Clan Board",
  "FORGED",
  "SHEATH",
  "ONLY_JINTO",
  "BURIED",
  "SLOT1",
  "SLOT2",
  "Token",
  "ONEFILL",
  "FILLED",
  "RESTRING",
  "CUSTOM",
  "PROTECTED",
  "NORANDOMSTATS",
  "THROW",
  "HOT",
  "DON'T USE",
  "MELTING",
  "DUPLICATE",
  "MATUREPLANT",
  "CARDCASE",
  "MOB_NOPICKUP",
  "NOSTEAL",
  "\n"
};


/* CONT_x */
const char *container_bits[NUM_CONT_FLAGS+1] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};



/* LIQ_x */
const char *drinks[NUM_LIQ_TYPES+1] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/* MATERIAL_ */
const char *material_names[NUM_MATERIALS+1] = {
   "bone",
   "ceramic",
   "copper",
   "diamond",
   "gold",
   "iron",
   "leather",
   "mithril",
   "obsidian",
   "steel",
   "stone",
   "silver",
   "wood",
   "glass",
   "organic material",
   "currency",
   "paper",
   "cotton",
   "satin",
   "silk",
   "burlap",
   "velvet",
   "platinum",
   "adamantine",
   "wool",
   "onyx",
   "ivory",
   "brass",
   "marble",
   "bronze",
   "kachin",
   "ruby",
   "sapphire",
   "emerald",
   "gemstone",
   "granite",
   "energy",
   "hemp",
   "crystal",
   "earth",
   "liquid",
   "cloth",
   "metal",
   "wax",
   "other",
   "food material",
   "oil",
   "\n"
};



/* one-word alias for each drink */
const char *drinknames[NUM_LIQ_TYPES+1] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
/* (Drunk, Hunger, Thirst) */
int drink_aff[NUM_LIQ_TYPES][NUM_CONDITIONS] = {
  {0, 0, 5},
  {3, 1, 1},
  {3, 1, 1},
  {3, 0, 2},
  {4, 0, 2},
  {6, 0, 0},
  {0, 0, 4},
  {6, 0, 0},
  {0, 0, 3},
  {0, 0, 4},
  {0, 1, 3},
  {0, 0, 3},
  {0, 0, 3},
  {0, 0, 2},
  {0, 0, 1},
  {0, 0, 8}
};

/* color of the various drinks */
const char *color_liquid[NUM_LIQ_TYPES+1] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "brown",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const char *fullness[NUM_FULLNESS+1] =
{
  "empty ",
  "less than half ",
  "half full ",
  "more than half full ",
  "full ",
  ""
};
