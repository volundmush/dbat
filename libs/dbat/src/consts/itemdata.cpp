#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/itemdata.hpp"

/*
 * WEAR_x - for eq list
 * Not used by type_name() so no \n.
 */
static constexpr std::string_view wear_where_entries[] = {
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
    "@c<@CWorn On Eye        @c>@n "};
static_assert(std::size(wear_where_entries) == NUM_WEARS);
constinit const name_table wear_where(wear_where_entries);

/* WEAR_x - for stat */
static constexpr std::string_view equipment_types_entries[] = {
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
    "Worn on eye"};
static_assert(std::size(equipment_types_entries) == NUM_WEARS);
constinit const name_table equipment_types(equipment_types_entries);

/* ITEM_x (ordinal object types) */
static constexpr std::string_view item_types_entries[] = {
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
    "FISHBAIT"};
static_assert(std::size(item_types_entries) == NUM_ITEM_TYPES);
constinit const name_table item_types(item_types_entries);

/* ITEM_WEAR_ (wear bitvector) */
static constexpr std::string_view wear_bits_entries[] = {
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
    "SCOUTER"};
static_assert(std::size(wear_bits_entries) == flag_count<wear_flag>);
constinit const name_table wear_bits(wear_bits_entries);

/* ITEM_x (extra bits) */
static constexpr std::string_view extra_bits_entries[] = {
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
    "NOSTEAL"};
static_assert(std::size(extra_bits_entries) == flag_count<extra_flag>);
constinit const name_table extra_bits(extra_bits_entries);

/* CONT_x */
static constexpr std::string_view container_bits_entries[] = {
    "CLOSEABLE",
    "PICKPROOF",
    "CLOSED",
    "LOCKED",
};
static_assert(std::size(container_bits_entries) == flag_count<container_flag>);
constinit const name_table container_bits(container_bits_entries);

/* LIQ_x */
static constexpr std::string_view drinks_entries[] = {
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
        "clear water"};
static_assert(std::size(drinks_entries) == NUM_LIQ_TYPES);
constinit const name_table drinks(drinks_entries);

/* MATERIAL_ */
static constexpr std::string_view material_names_entries[] = {
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
    "oil"};
static_assert(std::size(material_names_entries) == NUM_MATERIALS);
constinit const name_table material_names(material_names_entries);

/* one-word alias for each drink */
static constexpr std::string_view drinknames_entries[] = {
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
        "water"};
static_assert(std::size(drinknames_entries) == NUM_LIQ_TYPES);
constinit const name_table drinknames(drinknames_entries);

/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
/* (Drunk, Hunger, Thirst) */
const int drink_aff[NUM_LIQ_TYPES][NUM_CONDITIONS] = {
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
static constexpr std::string_view color_liquid_entries[] = {
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
        "crystal clear"};
static_assert(std::size(color_liquid_entries) == NUM_LIQ_TYPES);
constinit const name_table color_liquid(color_liquid_entries);

/*
 * level of fullness for drink containers
 * Not used by type_name() so no \n.
 */
