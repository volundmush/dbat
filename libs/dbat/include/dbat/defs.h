//
// Created by volund on 10/11/22.
//
#pragma once

#include "sysdep.h"

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD x030100 /* Major/Minor/Patchlevel - MMmmPP */

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD to be complete but we refuse to break binary file
 * compatibility.
 */
constexpr int USE_AUTOEQ = 1; /* TRUE/FALSE aren't defined yet. */

/* CWG Version String */
#define CWG_VERSION "CWG Rasputin - 3.5.31"
#define DBAT_VERSION "DBAT - 3.0"

/* preamble *************************************************************/

/*
 * As of bpl20, it should be safe to use unsigned data types for the
 * various virtual and real number data types.  There really isn't a
 * reason to use signed anymore so use the unsigned types and get
 * 65,535 objects instead of 32,768.
 *
 * NOTE: This will likely be unconditionally unsigned later.
 */

/* zone definition structure. for the 'zone-table'   */
constexpr int CUR_WORLD_VERSION = 1;
constexpr int CUR_ZONE_VERSION = 2;

constexpr int BANNED_SITE_LENGTH = 50;

enum class Race : uint8_t
{
    spirit = 0,
    human = 1,
    saiyan = 2,
    icer = 3,
    konatsu = 4,
    namekian = 5,
    mutant = 6,
    kanassan = 7,
    halfbreed = 8,
    bio_android = 9,
    android = 10,
    demon = 11,
    majin = 12,
    kai = 13,
    tuffle = 14,
    hoshijin = 15,
    animal = 16,
    saiba = 17,
    serpent = 18,
    ogre = 19,
    yardratian = 20,
    arlian = 21,
    dragon = 22,
    mechanical = 23,
};

enum class SubRace : uint8_t
{
    android_model_absorb = 0,
    android_model_repair = 1,
    android_model_sense = 2
};

/* Races */
constexpr Race RACE_HUMAN = Race::human;
constexpr Race RACE_SAIYAN = Race::saiyan;
constexpr Race RACE_ICER = Race::icer;
constexpr Race RACE_KONATSU = Race::konatsu;
constexpr Race RACE_NAMEK = Race::namekian;
constexpr Race RACE_MUTANT = Race::mutant;
constexpr Race RACE_KANASSAN = Race::kanassan;
constexpr Race RACE_HALFBREED = Race::halfbreed;
constexpr Race RACE_BIO = Race::bio_android;
constexpr Race RACE_ANDROID = Race::android;
constexpr Race RACE_DEMON = Race::demon;
constexpr Race RACE_MAJIN = Race::majin;
constexpr Race RACE_KAI = Race::kai;
constexpr Race RACE_TRUFFLE = Race::tuffle;
constexpr Race RACE_GOBLIN = Race::hoshijin;
constexpr Race RACE_ANIMAL = Race::animal;
constexpr Race RACE_SAIBA = Race::saiba;
constexpr Race RACE_SERPENT = Race::serpent;
constexpr Race RACE_OGRE = Race::ogre;
constexpr Race RACE_YARDRATIAN = Race::yardratian;
constexpr Race RACE_ARLIAN = Race::arlian;
constexpr Race RACE_DRAGON = Race::dragon;
constexpr Race RACE_MECHANICAL = Race::mechanical;
constexpr Race RACE_FAERIE = Race::spirit;

constexpr int NUM_RACES = 24;

enum class Sensei : uint8_t
{
    commoner = 0,
    roshi = 1,
    piccolo = 2,
    crane = 3,
    nail = 4,
    bardock = 5,
    ginyu = 6,
    frieza = 7,
    tapion = 8,
    sixteen = 9,
    dabura = 10,
    kibito = 11,
    jinto = 12,
    tsuna = 13,
    kurzak = 14
};

enum class Form : uint16_t
{
    // Universal,
    base = 0,
    custom_1 = 1,
    custom_2 = 2,
    custom_3 = 3,
    custom_4 = 4,
    custom_5 = 5,
    custom_6 = 6,
    custom_7 = 7,
    custom_8 = 8,
    custom_9 = 9,

    // Saiyan'y forms.
    oozaru = 10,
    golden_oozaru = 11,
    super_saiyan_1 = 12,
    super_saiyan_2 = 13,
    super_saiyan_3 = 14,
    super_saiyan_4 = 15,
    super_saiyan_god = 16,
    super_saiyan_blue = 17,

    // LSSJ
    ikari = 20,
    legendary_saiyan = 21,

    // Human'y Forms
    super_human_1 = 30,
    super_human_2 = 31,
    super_human_3 = 32,
    super_human_4 = 33,

    // Icer'y Forms
    icer_1 = 40,
    icer_2 = 41,
    icer_3 = 42,
    icer_4 = 43,
    icer_metal = 44,
    icer_golden = 45,
    icer_black = 46,

    // Konatsu
    shadow_first = 50,
    shadow_second = 51,
    shadow_third = 52,

    // Lycanthrope
    lesser_lycanthrope = 60,
    lycanthrope = 61,
    alpha_lycanthrope = 62,

    // Namekian
    super_namekian_1 = 70,
    super_namekian_2 = 71,
    super_namekian_3 = 72,
    super_namekian_4 = 73,

    // Mutant
    mutate_1 = 80,
    mutate_2 = 81,
    mutate_3 = 82,

    // BioAndroid
    bio_mature = 90,
    bio_semi_perfect = 91,
    bio_perfect = 92,
    bio_super_perfect = 93,

    // Android
    android_1 = 100,
    android_2 = 101,
    android_3 = 102,
    android_4 = 103,
    android_5 = 104,
    android_6 = 105,

    // Majin
    maj_affinity = 110,
    maj_super = 111,
    maj_true = 112,

    // Kai
    mystic_1 = 120,
    mystic_2 = 121,
    mystic_3 = 123,

    // Kai Alt
    divine_halo = 126,

    // Tuffle
    ascend_1 = 130,
    ascend_2 = 131,
    ascend_3 = 132,

    // Demon
    dark_king = 140,

    // Alternate Unbound Forms
    potential_unleashed = 180,
    evil_aura = 181,
    ultra_instinct = 182,

    // Unbound Perm Forms
    potential_unlocked = 210,
    potential_unlocked_max = 211,
    majinized = 212,
    divine_water = 213,

    // Techniques
    kaioken = 300,
    dark_metamorphosis = 301,

    tiger_stance = 302,
    eagle_stance = 303,
    ox_stance = 304,

    spirit_absorption = 305,

    // Hoshijin Shit
    death_phase = 306,
    birth_phase = 307,
    life_phase = 308

};

enum class Skill : uint16_t
{
    flex = 400,
    genius = 401,
    solar_flare = 402,
    might = 403,
    balance = 404,
    build = 405,
    tough_skin = 406,
    concentration = 407,
    kaioken = 408,
    spot = 409,
    first_aid = 410,
    disguise = 411,
    escape_artist = 412,
    appraise = 413,
    heal = 414,
    forgery = 415,
    hide = 416,
    bless = 417,
    curse = 418,
    listen = 419,
    eavesdrop = 420,
    poison = 421,
    cure_poison = 422,
    open_lock = 423,
    vigor = 424,
    regenerate = 425,
    keen_sight = 426,
    search = 427,
    move_silently = 428,
    absorb = 429,
    sleight_of_hand = 430,
    ingest = 431,
    repair = 432,
    sense = 433,
    survival = 434,
    yoikominminken = 435,
    create = 436,
    stone_spit = 437,
    potential_release = 438,
    telepathy = 439,
    renzokou_energy_dan = 440,
    masenko = 441,
    dodonpa = 442,
    barrier = 443,
    galik_gun = 444,
    throw_object = 445,
    dodge = 446,
    parry = 447,
    block = 448,
    punch = 449,
    kick = 450,
    elbow = 451,
    knee = 452,
    roundhouse = 453,
    uppercut = 454,
    slam = 455,
    heeldrop = 456,
    focus = 457,
    ki_ball = 458,
    ki_blast = 459,
    beam = 460,
    tsuihidan = 461,
    shogekiha = 462,
    zanzoken = 463,
    kamehameha = 464,
    dagger = 465,
    sword = 466,
    club = 467,
    spear = 468,
    gun = 469,
    brawl = 470,
    instant_transmission = 471,
    deathbeam = 472,
    eraser = 473,
    twin_slash = 474,
    psyblast = 475,
    honoo = 476,
    dualbeam = 477,
    rogafufuken = 478,
    special_pose = 479,
    bakuhatsuha = 480,
    kienzan = 481,
    tribeam = 482,
    special_beam_cannon = 483,
    final_flash = 484,
    crusher = 485,
    darkness_dragon_slash = 486,
    psychic_barrage = 487,
    hellflash = 488,
    hell_spear_blast = 489,
    kakusanha = 490,
    hasshuken = 491,
    scatter = 492,
    big_bang = 493,
    phoenix_slash = 494,
    deathball = 495,
    spirit_ball = 496,
    genkidama = 497,
    genocide = 498,
    dualwield = 499,
    kuraiiro_seiki = 500,
    tailwhip = 501,
    kousengan = 502,
    taisha_reiki = 503,
    paralyze = 505,
    infuse = 506,
    roll = 507,
    trip = 508,
    grapple = 509,
    water_spike = 510,
    self_destruct = 511,
    spiral_comet = 512,
    star_breaker = 513,
    enlighten = 514,
    commune = 515,
    mimic = 516,
    water_razor = 517,
    koteiru_bakuha = 518,
    dimizu_toride = 519,
    hyogaKabe = 520,
    wellspring = 521,
    aquaBarrier = 522,
    warp_pool = 523,
    hellSpiral = 524,
    nanite_armor = 525,
    fireshield = 526,
    cooking = 527,
    seishou_enko = 528,
    silk = 529,
    bash = 530,
    headbutt = 531,
    ensnare = 532,
    starnova = 533,
    pursuit = 534,
    zen_blade_strike = 535,
    sundering_force = 536,
    wither = 537,
    twohand = 538,
    fighting_arts = 539,
    dark_metamorphosis = 540,
    healing_glow = 541,
    runic = 542,
    extract = 543,
    gardening = 544,
    energize_throwing = 545,
    malice_breaker = 549,
    hayasa = 550,
    handling = 551,
    mystic_music = 552,
    light_grenade = 553,
    multiform = 554,
    spirit_control = 555,
    balefire = 556,
    blessed_hammer = 557,

    divine_halo = 558,
    instinctual_combat = 559,

    tiger_stance = 560,
    eagle_stance = 561,
    ox_stance = 562,

};

template <>
struct magic_enum::customize::enum_range<Skill>
{
    static constexpr int min = 400;
    static constexpr int max = 562;
    // (max - min) must be less than UINT16_MAX.
};

constexpr int SG_MIN = 2; /* Skill gain check must be less than this
                 number in order to be successful.
                 IE: 1% of a skill gain */

/* Ocarina Songs */
constexpr int SONG_SAFETY = 1;
constexpr int SONG_SHIELDING = 2;
constexpr int SONG_SHADOW_STITCH = 3;
constexpr int SONG_TELEPORT_EARTH = 4;
constexpr int SONG_TELEPORT_KONACK = 5;
constexpr int SONG_TELEPORT_ARLIA = 6;
constexpr int SONG_TELEPORT_NAMEK = 7;
constexpr int SONG_TELEPORT_VEGETA = 8;
constexpr int SONG_TELEPORT_FRIGID = 9;
constexpr int SONG_TELEPORT_AETHER = 10;
constexpr int SONG_TELEPORT_KANASSA = 11;

/* Fighting Preferences */
constexpr int PREFERENCE_THROWING = 1;
constexpr int PREFERENCE_H2H = 2;
constexpr int PREFERENCE_KI = 3;
constexpr int PREFERENCE_WEAPON = 4;

/* Ingredient vnums for recipes */
constexpr int RCP_TOMATO = 17212;
constexpr int RCP_POTATO = 17213;
constexpr int RCP_ONION = 17215;
constexpr int RCP_CUCUMBER = 17217;
constexpr int RCP_CHILIPEPPER = 17219;
constexpr int RCP_FOUSTAFI = 17221;
constexpr int RCP_CARROT = 17223;
constexpr int RCP_GREENBEAN = 17225;
constexpr int RCP_NORMAL_MEAT = 1612;
constexpr int RCP_GOOD_MEAT = 6500;
constexpr int RCP_BLACKBASS = 1000;
constexpr int RCP_SILVERTROUT = 1001;
constexpr int RCP_STRIPEDBASS = 1002;
constexpr int RCP_BLUECATFISH = 1003;
constexpr int RCP_FLOUNDER = 1004;
constexpr int RCP_SILVEREEL = 1005;
constexpr int RCP_COBIA = 1006;
constexpr int RCP_TAMBOR = 1007;
constexpr int RCP_NARRI = 1008;
constexpr int RCP_VALBISH = 1009;
constexpr int RCP_GUSBLAT = 1010;
constexpr int RCP_REPEEIL = 1011;
constexpr int RCP_GRAVELREBOI = 1012;
constexpr int RCP_VOOSPIKE = 1013;
constexpr int RCP_SHADOWFISH = 1014;
constexpr int RCP_SHADEEEL = 1015;
constexpr int RCP_BROWNMUSH = 1608;
constexpr int RCP_GARLIC = 1131;
constexpr int RCP_RICE = 1590;
constexpr int RCP_FLOUR = 1591;
constexpr int RCP_LETTUCE = 17227;
constexpr int RCP_APPLEPLUM = 8001;
constexpr int RCP_FROZENBERRY = 4901;
constexpr int RCP_CARAMBOLA = 3416;

/* Meal recipes */
constexpr int RECIPE_TOMATO_SOUP = 1;
constexpr int RECIPE_POTATO_SOUP = 2;
constexpr int RECIPE_VEGETABLE_SOUP = 3;
constexpr int RECIPE_MEAT_STEW = 4;
constexpr int RECIPE_STEAK = 5;
constexpr int RECIPE_ROAST = 6;
constexpr int RECIPE_CHILI_SOUP = 7;
constexpr int RECIPE_GRILLED_NORMFISH = 8;
constexpr int RECIPE_GRILLED_GOODFISH = 9;
constexpr int RECIPE_GRILLED_GREATFISH = 10;
constexpr int RECIPE_GRILLED_BESTFISH = 11;
constexpr int RECIPE_COOKED_RICE = 12;
constexpr int RECIPE_SUSHI = 13;
constexpr int RECIPE_BREAD = 14;
constexpr int RECIPE_SALAD = 15;
constexpr int RECIPE_APPLEPLUM = 16;
constexpr int RECIPE_FBERRY_MUFFIN = 17;
constexpr int RECIPE_CARAMBOLA_BREAD = 18;

/* Completed Meal Object Vnums */
constexpr int MEAL_START = 1220;
constexpr int MEAL_TOMATO_SOUP = 1220;
constexpr int MEAL_STEAK = 1221;
constexpr int MEAL_POTATO_SOUP = 1222;
constexpr int MEAL_VEGETABLE_SOUP = 1223;
constexpr int MEAL_MEAT_STEW = 1224;
constexpr int MEAL_ROAST = 1225;
constexpr int MEAL_CHILI_SOUP = 1226;
constexpr int MEAL_NORM_FISH = 1227;
constexpr int MEAL_GOOD_FISH = 1228;
constexpr int MEAL_GREAT_FISH = 1229;
constexpr int MEAL_BEST_FISH = 1230;
constexpr int MEAL_COOKED_RICE = 1231;
constexpr int MEAL_SUSHI = 1232;
constexpr int MEAL_BREAD = 1233;
constexpr int MEAL_SALAD = 1234;
constexpr int MEAL_APPLEPLUM = 1235;
constexpr int MEAL_FBERRY_MUFFIN = 1236;
constexpr int MEAL_CARAMBOLA_BREAD = 1237;

constexpr int MEAL_LAST = 1234;

/* Fishing Defines */
constexpr int FISH_NOFISH = 0;
constexpr int FISH_BITE = 1;
constexpr int FISH_HOOKED = 2;
constexpr int FISH_REELING = 3;

/* Shadow Dragon Defines */

constexpr int SHADOW_DRAGON1_VNUM = 81;
constexpr int SHADOW_DRAGON2_VNUM = 82;
constexpr int SHADOW_DRAGON3_VNUM = 83;
constexpr int SHADOW_DRAGON4_VNUM = 84;
constexpr int SHADOW_DRAGON5_VNUM = 85;
constexpr int SHADOW_DRAGON6_VNUM = 86;
constexpr int SHADOW_DRAGON7_VNUM = 87;

/* room-related defines *************************************************/

/* The cardinal directions: used as index to Room.dir_option[] */
constexpr int NORTH = 0;
constexpr int EAST = 1;
constexpr int SOUTH = 2;
constexpr int WEST = 3;
constexpr int UP = 4;
constexpr int DOWN = 5;
constexpr int NORTHWEST = 6;
constexpr int NORTHEAST = 7;
constexpr int SOUTHEAST = 8;
constexpr int SOUTHWEST = 9;
constexpr int INDIR = 10;
constexpr int OUTDIR = 11;

/* Room flags: used in Room.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
constexpr int ROOM_DARK = 0;         /* Dark			*/
constexpr int ROOM_DEATH = 1;        /* Death trap		*/
constexpr int ROOM_NOMOB = 2;        /* MOBs not allowed		*/
constexpr int ROOM_INDOORS = 3;      /* Indoors			*/
constexpr int ROOM_PEACEFUL = 4;     /* Violence not allowed	*/
constexpr int ROOM_SOUNDPROOF = 5;   /* Shouts, gossip blocked	*/
constexpr int ROOM_NOTRACK = 6;      /* Track won't go through	*/
constexpr int ROOM_NOINSTANT = 7;    /* IT not allowed		*/
constexpr int ROOM_TUNNEL = 8;       /* room for only 1 pers	*/
constexpr int ROOM_PRIVATE = 9;      /* Can't teleport in		*/
constexpr int ROOM_GODROOM = 10;     /* LVL_GOD+ only allowed	*/
constexpr int ROOM_HOUSE = 11;       /* (R) Room is a house	*/
constexpr int ROOM_ATRIUM = 13;      /* (R) The door to a house	*/
constexpr int ROOM_OLC = 14;         /* (R) Modifyable/!compress	*/
constexpr int ROOM_VEHICLE = 16;     /* Requires a vehicle to pass       */
constexpr int ROOM_UNDERGROUND = 17; /* Room is below ground      */
constexpr int ROOM_HELL = 28;        /* Room is Punishment Hell*/
constexpr int ROOM_REGEN = 29;       /* Better regen */
constexpr int ROOM_CBANK = 35;       /* This room is a clan bank */
constexpr int ROOM_SHIP = 36;        /* This room is a private ship room */
constexpr int ROOM_AURA = 40;        /* This room has an aura around it  */
constexpr int ROOM_BEDROOM = 57;     /* +25% regen                       */
constexpr int ROOM_WORKOUT = 58;     /* Workout Room                     */
constexpr int ROOM_GARDEN1 = 59;     /* 8 plant garden                   */
constexpr int ROOM_GARDEN2 = 60;     /* 20 plant garden                  */
constexpr int ROOM_FERTILE1 = 61;
constexpr int ROOM_FERTILE2 = 62;
constexpr int ROOM_FISHING = 63;
constexpr int ROOM_FISHFRESH = 64;
constexpr int ROOM_CANREMODEL = 65;

constexpr int NUM_ROOM_FLAGS = 69;

enum class WhereFlag : uint8_t
{
    planet_earth = 0,             // Room is on Earth
    earth_orbit = 1,              // Earth Orbit
    planet_vegeta = 2,            // Room is on Vegeta
    vegeta_orbit = 3,             // Vegeta Orbit
    planet_frigid = 4,            // Room is on Frigid
    frigid_orbit = 5,             // Frigid Orbit
    planet_konack = 6,            // Room is on Konack
    konack_orbit = 7,             // Konack Orbit
    planet_namek = 8,             // Room is on Namek
    namek_orbit = 9,              // Namek Orbit
    planet_aether = 10,           // Room is on Aether
    aether_orbit = 11,            // Aether Orbit
    planet_yardrat = 12,          // This room is on planet Yardrat
    yardrat_orbit = 13,           // Yardrat Orbit
    planet_kanassa = 14,          // This room is on planet Kanassa
    kanassa_orbit = 15,           // Kanassa Orbit
    planet_arlia = 16,            // This room is on planet Arlia
    arlia_orbit = 17,             // Arlia Orbit
    planet_cerria = 18,           // This room is on planet Cerria
    cerria_orbit = 19,            // This room is in Cerria's Orbit
    moon_zenith = 20,             // (unspecified)
    zenith_orbit = 21,            // zenith orbit
    neo_nirvana = 22,             // Room is on Neo
    afterlife = 23,               // Room is on AL
    afterlife_hell = 24,          // Room is HELLLLLLL
    hyperbolic_time_chamber = 25, // Room is extra special training area
    pendulum_past = 26,           // Inside the pendulum room
    space = 27,                   // Room is on Space
    nebula = 28,                  // Nebulae
    asteroid = 29,                // Asteroid
    wormhole = 30,                // Wormhole
    space_station = 31,           // Space Station
    star = 32                     // Is a star
};

enum class RoomFlag : uint8_t
{
    dark = 0, // Dark
    // death_trap = 1,            // Death trap
    no_mobiles = 2,              // MOBs not allowed
    indoors = 3,                 // Indoors
    peaceful = 4,                // Violence not allowed
    soundproof = 5,              // Shouts, gossip blocked
    no_track = 6,                // Track won't go through
    no_instant_transmission = 7, // IT not allowed
    tunnel = 8,                  // room for only 1 pers
    private_room = 9,            // Can't teleport in
    god_room = 10,               // LVL_GOD+ only allowed
    house = 11,                  // (R) Room is a house
    // house_crash = 12,     // (R) House needs saving
    atrium = 13, // (R) The door to a house
    olc = 14,    // (R) Modifyable/!compress
    // bfs_mark = 15,        // (R) breath-first srch mrk
    vehicle = 16,     // Requires a vehicle to pass
    underground = 17, // Room is below ground
    // timed_deathtrap = 19,        // Room has a timed death trap
    punishment_hell = 28, // Room is Punishment Hell
    regen = 29,           // Better regen
    clan_bank = 35,       // This room is a clan bank
    ship = 36,            // This room is a private ship room
    healing_aura = 40,    // This room has an aura around it
    bedroom = 57,         // +25% regen
    workout = 58,         // Workout Room
    garden_1 = 59,        // 8 plant garden
    garden_2 = 60,        // 20 plant garden
    fertile_1 = 61,       // (unspecified)
    fertile_2 = 62,       // (unspecified)
    fishing = 63,         // (unspecified)
    fishfresh = 64,       // (unspecified)
    can_remodel = 65,     // (unspecified)
    // save = 67,            // room saves contents
};

// I'm thinking that rooms with lava might automatically have a considerable heat value.
// They might also generate light. Molten lava glows.
// The below are queried by thing_data::getMyEnvironment(int type).
constexpr int ENV_GRAVITY = 0;       // For rooms with special gravity. val is x Gs. 1 is Earth-like.
constexpr int ENV_LAVA = 1;          // For rooms with lava. val is how much? how does one measure lava? 0 means no lava.
constexpr int ENV_WATER = 2;         // For rooms with water. val is how much? 0 means no water. but what should I measure water by?
constexpr int ENV_TEMPERATURE = 3;   // For rooms with heat. val is how much in C. Can be negative, obviously.
constexpr int ENV_LIGHT = 4;         // For rooms with light. val is how much?
constexpr int ENV_PRESSURE = 5;      // Atmospheric pressure. it's in atmospheres. 1 is normal. Kami's Lookout... maybe 0.5? Space is 0.0.
constexpr int ENV_RADIATION = 6;     // Radiation level. measured in rads. 0 is normal.
constexpr int ENV_SUNLIGHT = 7;      // how much sunlight is available. This should be 0 during the night, 100 at zenith, and in-between otherwise.
constexpr int ENV_MOONLIGHT = 8;     // how much moonlight is available. 100 is a full moon. For those Oozaru freaks. And Lycanthropes. And whatever else.
constexpr int ENV_WIND = 9;          // Wind speed in... kmh maybe? 0 is no wind.
constexpr int ENV_HUMIDITY = 10;     // Humidity level. 0 is bone dry, 100 is a rainforest.
constexpr int ENV_OXYGEN = 11;       // Oxygen level. 0 is no oxygen, 100 is normal. Multiply by pressure to get breathability.
constexpr int ENV_TOXICITY = 12;     // The environment is toxic. This can be used for all kinds of systems. 0.0 is no toxicity.
constexpr int ENV_CORROSIVITY = 13;  // The environment is corrosive. This might inflict injury or damage equipment over time. 0.0 is no corrosivity.
constexpr int ENV_ETHER_STREAM = 14; // Ether Stream value for Hoshijin. 0 is no ether stream. 100 is maximum.

/* Zone info: Used in zone_data.zone_flags */
constexpr int ZONE_CLOSED = 0;
constexpr int ZONE_NOIMMORT = 1;
constexpr int ZONE_QUEST = 2;
constexpr int ZONE_DBALLS = 3;

enum class ZoneFlag : uint8_t
{
    closed = 0,
    no_immortal = 1,
    quest = 2,
    dragon_balls = 3,
    planet = 4,
    ether_stream = 5,
    has_moon = 6
};

enum class ExitFlag : uint8_t {
    isdoor = 0,
    closed = 1,
    locked = 2,
    pickproof = 3,
    secret = 4
};

/* Exit info: used in Room.dir_option.exit_info */
constexpr ExitFlag EX_ISDOOR = ExitFlag::isdoor;    /* Exit is a door		*/
constexpr ExitFlag EX_CLOSED = ExitFlag::closed;    /* The door is closed	*/
constexpr ExitFlag EX_LOCKED = ExitFlag::locked;    /* The door is locked	*/
constexpr ExitFlag EX_PICKPROOF = ExitFlag::pickproof; /* Lock can't be picked	*/
constexpr ExitFlag EX_SECRET = ExitFlag::secret;    /* The door is hidden        */

constexpr int NUM_EXIT_FLAGS = 5;

enum class SectorType
{
    inside = 0,       // Indoors
    city = 1,         // In a city
    field = 2,        // In a field
    forest = 3,       // In a forest
    hills = 4,        // In the hills
    mountain = 5,     // On a mountain
    water_swim = 6,   // Swimmable water
    water_noswim = 7, // Water - need a boat
    flying = 8,       // Wheee!
    underwater = 9,   // Underwater
    shop = 10,        // Shop
    important = 11,   // Important Rooms
    desert = 12,      // A desert
    space = 13,       // This is a space room
    lava = 14         // This room always has lava
};

/* Sector types: used in Room.sector_type */
constexpr int SECT_INSIDE = 0;       /* Indoors			*/
constexpr int SECT_CITY = 1;         /* In a city			*/
constexpr int SECT_FIELD = 2;        /* In a field		*/
constexpr int SECT_FOREST = 3;       /* In a forest		*/
constexpr int SECT_HILLS = 4;        /* In the hills		*/
constexpr int SECT_MOUNTAIN = 5;     /* On a mountain		*/
constexpr int SECT_WATER_SWIM = 6;   /* Swimmable water		*/
constexpr int SECT_WATER_NOSWIM = 7; /* Water - need a boat	*/
constexpr int SECT_FLYING = 8;       /* Wheee!			*/
constexpr int SECT_UNDERWATER = 9;   /* Underwater		*/
constexpr int SECT_SHOP = 10;        /* Shop                      */
constexpr int SECT_IMPORTANT = 11;   /* Important Rooms           */
constexpr int SECT_DESERT = 12;      /* A desert                  */
constexpr int SECT_SPACE = 13;       /* This is a space room      */
constexpr int SECT_LAVA = 14;        /* This room always has lava */

constexpr int NUM_ROOM_SECTORS = 15;

/* char and mob-related defines *****************************************/

constexpr int NUM_CLASSES = 31;
constexpr int NUM_NPC_CLASSES = 4;
constexpr int NUM_PRESTIGE_CLASSES = 15;
constexpr int NUM_BASIC_CLASSES = 14;

/* Gauntlet crap */
constexpr int GAUNTLET_ZONE = 24;    /* The gauntlet zone vnum */
constexpr int GAUNTLET_START = 2403; /* The waiting room at the start of the gauntlet */
constexpr int GAUNTLET_END = 2404;   /* The treasure room at the end of the gauntlet  */

/* Death Types for producing corpses with depth */
constexpr int DTYPE_NORMAL = 0; /* Default Death Type */
constexpr int DTYPE_HEAD = 1;   /* Lost their head    */
constexpr int DTYPE_HALF = 2;   /* Blown in half      */
constexpr int DTYPE_VAPOR = 3;  /* Vaporized by attack*/
constexpr int DTYPE_PULP = 4;   /* Beat to a pulp     */

/* Character Creation Styles */
/* Let's define bonuses/negatives */
constexpr int BONUS_THRIFTY = 0;
constexpr int BONUS_PRODIGY = 1;
constexpr int BONUS_QUICK_STUDY = 2;
constexpr int BONUS_DIEHARD = 3;
constexpr int BONUS_BRAWLER = 4;
constexpr int BONUS_DESTROYER = 5;
constexpr int BONUS_HARDWORKER = 6;
constexpr int BONUS_HEALER = 7;
constexpr int BONUS_LOYAL = 8;
constexpr int BONUS_BRAWNY = 9;
constexpr int BONUS_SCHOLARLY = 10;
constexpr int BONUS_SAGE = 11;
constexpr int BONUS_AGILE = 12;
constexpr int BONUS_QUICK = 13;
constexpr int BONUS_STURDY = 14;
constexpr int BONUS_THICKSKIN = 15;
constexpr int BONUS_RECIPE = 16;
constexpr int BONUS_FIREPROOF = 17;
constexpr int BONUS_POWERHIT = 18;
constexpr int BONUS_HEALTHY = 19;
constexpr int BONUS_INSOMNIAC = 20;
constexpr int BONUS_EVASIVE = 21;
constexpr int BONUS_WALL = 22;
constexpr int BONUS_ACCURATE = 23;
constexpr int BONUS_LEECH = 24;
constexpr int BONUS_GMEMORY = 25;
constexpr int BONUS_SOFT = 26;
constexpr int BONUS_LATE = 27;
constexpr int BONUS_IMPULSE = 28;
constexpr int BONUS_SICKLY = 29;
constexpr int BONUS_PUNCHINGBAG = 30;
constexpr int BONUS_PUSHOVER = 31;
constexpr int BONUS_POORDEPTH = 32;
constexpr int BONUS_THINSKIN = 33;
constexpr int BONUS_FIREPRONE = 34;
constexpr int BONUS_INTOLERANT = 35;
constexpr int BONUS_COWARD = 36;
constexpr int BONUS_ARROGANT = 37;
constexpr int BONUS_UNFOCUSED = 38;
constexpr int BONUS_SLACKER = 39;
constexpr int BONUS_SLOW_LEARNER = 40;
constexpr int BONUS_MASOCHISTIC = 41;
constexpr int BONUS_MUTE = 42;
constexpr int BONUS_WIMP = 43;
constexpr int BONUS_DULL = 44;
constexpr int BONUS_FOOLISH = 45;
constexpr int BONUS_CLUMSY = 46;
constexpr int BONUS_SLOW = 47;
constexpr int BONUS_FRAIL = 48;
constexpr int BONUS_SADISTIC = 49;
constexpr int BONUS_LONER = 50;
constexpr int BONUS_BMEMORY = 51;

constexpr int MAX_BONUSES = 52;

constexpr int ALIGN_SAINT = 0;
constexpr int ALIGN_VALIANT = 1;
constexpr int ALIGN_HERO = 2;
constexpr int ALIGN_DOGOOD = 3;
constexpr int ALIGN_NEUTRAL = 4;
constexpr int ALIGN_CROOK = 5;
constexpr int ALIGN_VILLAIN = 6;
constexpr int ALIGN_TERRIBLE = 7;
constexpr int ALIGN_HORRIBLE = 8;

constexpr int NUM_ALIGNS = 9;

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
enum class Size : int8_t
{
    undefined = -1,
    fine = 0,
    diminutive = 1,
    tiny = 2,
    small = 3,
    medium = 4,
    large = 5,
    huge = 6,
    gargantuan = 7,
    colossal = 8
};

constexpr int SIZE_UNDEFINED = -1;
constexpr int SIZE_FINE = 0;
constexpr int SIZE_DIMINUTIVE = 1;
constexpr int SIZE_TINY = 2;
constexpr int SIZE_SMALL = 3;
constexpr int SIZE_MEDIUM = 4;
constexpr int SIZE_LARGE = 5;
constexpr int SIZE_HUGE = 6;
constexpr int SIZE_GARGANTUAN = 7;
constexpr int SIZE_COLOSSAL = 8;

constexpr int NUM_SIZES = 9;

constexpr int WIELD_NONE = 0;
constexpr int WIELD_LIGHT = 1;
constexpr int WIELD_ONEHAND = 2;
constexpr int WIELD_TWOHAND = 3;

/* Number of weapon types */
constexpr int MAX_WEAPON_TYPES = 26;

/* Critical hit types */
constexpr int CRIT_X2 = 0;
constexpr int CRIT_X3 = 1;
constexpr int CRIT_X4 = 2;

constexpr int MAX_CRIT_TYPE = CRIT_X4;
constexpr int NUM_CRIT_TYPES = 3;

/* Sex */
enum class Sex : uint8_t
{
    neutral = 0,
    male = 1,
    female = 2
};

enum class Appearance : uint8_t
{
    hair_style = 0,
    hair_color = 1,
    skin_color = 2,
    eye_color = 3,
    aura_color = 4,
    build = 5,
    posture = 6,
    features = 7,
    seeming = 8
};

constexpr Sex SEX_NEUTRAL = Sex::neutral;
constexpr Sex SEX_MALE = Sex::male;
constexpr Sex SEX_FEMALE = Sex::female;

constexpr int NUM_SEX = 3;
constexpr int NUM_GENDERS = NUM_SEX;

/* Positions */
constexpr int POS_DEAD = 0;      /* dead			*/
constexpr int POS_MORTALLYW = 1; /* mortally wounded	*/
constexpr int POS_INCAP = 2;     /* incapacitated	*/
constexpr int POS_STUNNED = 3;   /* stunned		*/
constexpr int POS_SLEEPING = 4;  /* sleeping		*/
constexpr int POS_RESTING = 5;   /* resting		*/
constexpr int POS_SITTING = 6;   /* sitting		*/
constexpr int POS_FIGHTING = 7;  /* fighting		*/
constexpr int POS_STANDING = 8;  /* standing		*/

constexpr int NUM_POSITIONS = 9;

/* AUCTIONING STATES */
constexpr int AUC_NULL_STATE = 0;  /* not doing anything */
constexpr int AUC_OFFERING = 1;    /* object has been offfered */
constexpr int AUC_GOING_ONCE = 2;  /* object is going once! */
constexpr int AUC_GOING_TWICE = 3; /* object is going twice! */
constexpr int AUC_LAST_CALL = 4;   /* last call for the object! */
constexpr int AUC_SOLD = 5;
/* AUCTION CANCEL STATES */
constexpr int AUC_NORMAL_CANCEL = 6; /* normal cancellation of auction */
constexpr int AUC_QUIT_CANCEL = 7;   /* auction canclled because player quit */
constexpr int AUC_WIZ_CANCEL = 8;    /* auction cancelled by a god */
/* OTHER JUNK */
constexpr int AUC_STAT = 9;
constexpr int AUC_BID = 10;

enum class CharacterFlag : uint8_t
{
    tail = 1,
    cyber_right_arm = 2, // Cybernetic Right Arm
    cyber_left_arm = 3,  // Cybernetic Left Arm
    cyber_right_leg = 4, // Cybernetic Right Leg
    cyber_left_leg = 5,  // Cybernetic Left Leg

    sparring = 6,    // This is mob sparring
    powering_up = 7, // Is powering up,
};

enum class PlayerFlag : uint8_t
{
    player_killer = 0,        // Player is a player-killer
    player_thief = 1,         // Player is a player-thief
    frozen = 2,               // Player is frozen
    writing = 4,              // Player writing (board/mail/olc)
    mailing = 5,              // Player is writing mail
    site_ok = 7,              // Player has been site-cleared
    no_shout = 8,             // Player not allowed to shout/goss
    no_title = 9,             // Player not allowed to set title
    loadroom = 11,            // Player uses nonstandard loadroom
    no_wizlist = 12,          // Player shouldn't be on wizlist
    no_delete = 13,           // Player shouldn't be deleted
    wiz_invisible_start = 14, // Player should enter game wizinvis
    not_dead_yet = 16,        // (R) Player being extracted.
    piloting = 31,            // Player is sitting in the pilots chair
    skillp = 32,              // Player made a good choice in CC
    charging = 34,            // Player is charging
    knocked_out = 45,         // Knocked OUT
    immortal = 51,            // The player is immortal
    eyes_closed = 52,         // The player has their eyes closed
    disguised = 53,           // The player is disguised
    bandaged = 54,            // The player has been bandaged
    healing_tank = 56,        // Is inside a healing tank
    halfbreed_fury = 57,      // Is in fury mode
    ginyu_fighting_pose = 58, // Ginyu Pose Effect
    absorbed = 60,
    killed_by_player = 62,
    two_hand_wielding = 63,
    self_destruct_1 = 64,
    self_dextruct_2 = 65,
    spiral = 66,
    biography_approved = 67,
    repair_learn = 69,
    forgetting_skill = 70,
    transmission = 71,
    fishing = 72,
    majin_goop_state = 73,
    multihit = 74,
    aura_light = 75,
    room_display = 76,
    stolen = 77,
    tail_hide = 78,     // Hides tail for S & HB
    no_regrow_tail = 79 // Halt Growth for S & HB
};

/* Player flags: used by Character.act */
constexpr int PLR_KILLER = 0;      /* Player is a player-killer        */
constexpr int PLR_THIEF = 1;       /* Player is a player-thief         */
constexpr int PLR_FROZEN = 2;      /* Player is frozen                 */
constexpr int PLR_WRITING = 4;     /* Player writing (board/mail/olc)  */
constexpr int PLR_MAILING = 5;     /* Player is writing mail           */
constexpr int PLR_NOSHOUT = 8;     /* Player not allowed to shout/goss */
constexpr int PLR_NOTITLE = 9;     /* Player not allowed to set title  */
constexpr int PLR_DELETED = 10;    /* Player deleted - space reusable  */
constexpr int PLR_LOADROOM = 11;   /* Player uses nonstandard loadroom */
constexpr int PLR_NOWIZLIST = 12;  /* Player shouldn't be on wizlist  	*/
constexpr int PLR_NODELETE = 13;   /* Player shouldn't be deleted     	*/
constexpr int PLR_INVSTART = 14;   /* Player should enter game wizinvis*/
constexpr int PLR_CRYO = 15;       /* Player is cryo-saved (purge prog)*/
constexpr int PLR_NOTDEADYET = 16; /* (R) Player being extracted.     	*/
constexpr int PLR_PILOTING = 31;   /* Player is sitting in the pilots chair */
constexpr int PLR_SKILLP = 32;     /* Player made a good choice in CC  */
constexpr int PLR_CHARGE = 34;     /* Player is charging               */
constexpr int PLR_KNOCKED = 45;    /* Knocked OUT                      */
constexpr int PLR_FPSSJ = 50;      /* Full Power Super Saiyan          */
constexpr int PLR_IMMORTAL = 51;   /* The player is immortal           */
constexpr int PLR_EYEC = 52;       /* The player has their eyes closed */
constexpr int PLR_DISGUISED = 53;  /* The player is disguised          */
constexpr int PLR_BANDAGED = 54;   /* THe player has been bandaged     */
constexpr int PLR_HEALT = 56;      /* Is inside a healing tank         */
constexpr int PLR_FURY = 57;       /* Is in fury mode                  */
constexpr int PLR_POSE = 58;       /* Ginyu Pose Effect                */
constexpr int PLR_ABSORBED = 60;
constexpr int PLR_PDEATH = 62;
constexpr int PLR_THANDW = 63;
constexpr int PLR_SELFD = 64;
constexpr int PLR_SELFD2 = 65;
constexpr int PLR_SPIRAL = 66;
constexpr int PLR_BIOGR = 67;
constexpr int PLR_REPLEARN = 69;
constexpr int PLR_FORGET = 70;
constexpr int PLR_TRANSMISSION = 71;
constexpr int PLR_FISHING = 72;
constexpr int PLR_GOOP = 73;
constexpr int PLR_MULTIHIT = 74;
constexpr int PLR_AURALIGHT = 75;
constexpr int PLR_RDISPLAY = 76;
constexpr int PLR_STOLEN = 77;
constexpr int PLR_TAILHIDE = 78; /* Hides tail for S & HB            */
constexpr int PLR_NOGROW = 79;   /* Halt Growth for S & HB           */

constexpr int NUM_PLR_FLAGS = 80;

/* Mob Personalty */
constexpr int MAX_PERSONALITIES = 5;

enum class MobFlag : uint8_t
{
    special_proc = 0,        // Mob has a callable spec-proc
    sentinel = 1,            // Mob should not move
    no_scavenger = 2,        // Mob won't pick up items from rooms
    aware = 4,               // Mob can't be backstabbed
    aggressive = 5,          // Mob auto-attacks everybody nearby
    stay_zone = 6,           // Mob shouldn't wander out of zone
    wimpy = 7,               // Mob flees if severely injured
    aggressive_evil = 8,     // Auto-attack any evil PC's
    aggressive_good = 9,     // Auto-attack any good PC's
    aggressive_neutral = 10, // Auto-attack any neutral PC's
    memory = 11,             // remember attackers if attacked
    helper = 12,             // attack PCs fighting other NPCs
    no_charm = 13,           // Mob can't be charmed
    no_summon = 14,          // Mob can't be summoned
    no_sleep = 15,           // Mob can't be slept
    autobalance = 16,        // Mob stats autobalance
    no_blind = 17,           // Mob can't be blinded
    no_kill = 18,            // Mob can't be killed
    not_dead_yet = 19,       // (R) Mob being extracted.
    mountable = 20,          // Mob is mountable.
    justdesc = 26,           // Mob doesn't use auto desc
    husk = 27,               // Is an extracted Husk
    dummy = 29,              // This mob will not fight back
    no_poison = 32,          // No poison
    know_kaioken = 33,       // Knows kaioken
};

/* Mobile flags: used by Character.act */
constexpr int MOB_SPEC = 0;          /* Mob has a callable spec-proc   	*/
constexpr int MOB_SENTINEL = 1;      /* Mob should not move            	*/
constexpr int MOB_NOSCAVENGER = 2;   /* Mob won't pick up items from rooms*/
constexpr int MOB_AWARE = 4;         /* Mob can't be backstabbed          */
constexpr int MOB_AGGRESSIVE = 5;    /* Mob auto-attacks everybody nearby	*/
constexpr int MOB_STAY_ZONE = 6;     /* Mob shouldn't wander out of zone  */
constexpr int MOB_WIMPY = 7;         /* Mob flees if severely injured  	*/
constexpr int MOB_AGGR_EVIL = 8;     /* Auto-attack any evil PC's		*/
constexpr int MOB_AGGR_GOOD = 9;     /* Auto-attack any good PC's      	*/
constexpr int MOB_AGGR_NEUTRAL = 10; /* Auto-attack any neutral PC's   	*/
constexpr int MOB_MEMORY = 11;       /* remember attackers if attacked    */
constexpr int MOB_HELPER = 12;       /* attack PCs fighting other NPCs    */
constexpr int MOB_NOCHARM = 13;      /* Mob can't be charmed         	*/
constexpr int MOB_NOSUMMON = 14;     /* Mob can't be summoned             */
constexpr int MOB_NOSLEEP = 15;      /* Mob can't be slept           	*/
constexpr int MOB_AUTOBALANCE = 16;  /* Mob stats autobalance		*/
constexpr int MOB_NOBLIND = 17;      /* Mob can't be blinded         	*/
constexpr int MOB_NOKILL = 18;       /* Mob can't be killed               */
constexpr int MOB_NOTDEADYET = 19;   /* (R) Mob being extracted.          */
constexpr int MOB_MOUNTABLE = 20;    /* Mob is mountable.			*/
constexpr int MOB_JUSTDESC = 26;     /* Mob doesn't use auto desc         */
constexpr int MOB_HUSK = 27;         /* Is an extracted Husk              */
constexpr int MOB_DUMMY = 29;        /* This mob will not fight back      */
constexpr int MOB_NOPOISON = 32;     /* No poison                         */
constexpr int MOB_KNOWKAIO = 33;     /* Knows kaioken                     */

constexpr int NUM_MOB_FLAGS = 35;

enum class PrefFlag
{
    brief = 0,       // Room descs won't normally be shown
    compact = 1,     // No extra CRLF pair before prompts
    deaf = 2,        // Can't hear shouts
    notell = 3,      // Can't receive tells
    disphp = 4,      // Display hit points in prompt
    dispmana = 5,    // Display mana points in prompt
    dispmove = 6,    // Display move points in prompt
    autoexit = 7,    // Display exits in a room
    nohassle = 8,    // Aggr mobs won't attack
    quest = 9,       // On quest
    summonable = 10, // Can be summoned
    norepeat = 11,   // No repetition of comm commands
    holylight = 12,  // Can see in dark
    color = 13,      // Color
    // spare         = 14, // Used to be second color bit
    nowiz = 15,     // Can't hear wizline
    log1 = 16,      // On-line System Log (low bit)
    log2 = 17,      // On-line System Log (high bit)
    noauct = 18,    // Can't hear auction channel
    nogoss = 19,    // Can't hear gossip channel
    nogratz = 20,   // Can't hear grats channel
    roomflags = 21, // Can see room flags (ROOM_x)
    dispauto = 22,  // Show prompt HP, MP, MV when < 30%.
    cls = 23,       // Clear screen in OasisOLC
    buildwalk = 24, // Build new rooms when walking
    afk = 25,       // Player is AFK
    autoloot = 26,  // Loot everything from a corpse
    autogold = 27,  // Loot gold from a corpse
    autosplit = 28, // Split gold with group
    full_exit = 29, // Shows full autoexit details
    autosac = 30,   // Sacrifice a corpse
    automem = 31,   // Memorize spells
    vieworder = 32, // If you want to see the newest first
    // nocompress    = 33, // If you want to force MCCP2 off
    autoassist = 34, // Auto-assist toggle
    dispki = 35,     // Display ki points in prompt
    dispexp = 36,    // Display exp points in prompt
    disptnl = 37,    // Display TNL exp points in prompt
    test = 38,       // Sets triggers safety off for imms
    hide = 39,       // Hide on who from other mortals
    nmwarn = 40,     // No mail warning
    hints = 41,      // Receives hints
    fury = 42,       // Sees fury meter
    nodec = 43,
    noeqsee = 44,
    nomusic = 45,
    lkeep = 46,
    distime = 47, // Part of Prompt Options
    disgold = 48, // Part of Prompt Options
    disprac = 49, // Part of Prompt Options
    noparry = 50,
    dishuth = 51, // Part of Prompt Options
    disperc = 52, // Part of Prompt Options
    carve = 53,
    arenawatch = 54,
    nogive = 55,
    instruct = 56,
    ghealth = 57,
    ihealth = 58,
    energize = 59,
    form = 60,
    tech = 61
};

/*  flags: used by Character.player_specials.pref */
constexpr int PRF_BRIEF = 0;       /* Room descs won't normally be shown	*/
constexpr int PRF_COMPACT = 1;     /* No extra CRLF pair before prompts		*/
constexpr int PRF_DEAF = 2;        /* Can't hear shouts              		*/
constexpr int PRF_NOTELL = 3;      /* Can't receive tells		    	*/
constexpr int PRF_DISPHP = 4;      /* Display hit points in prompt  		*/
constexpr int PRF_DISPMANA = 5;    /* Display mana points in prompt    		*/
constexpr int PRF_DISPMOVE = 6;    /* Display move points in prompt 		*/
constexpr int PRF_AUTOEXIT = 7;    /* Display exits in a room          		*/
constexpr int PRF_NOHASSLE = 8;    /* Aggr mobs won't attack           		*/
constexpr int PRF_QUEST = 9;       /* On quest					*/
constexpr int PRF_SUMMONABLE = 10; /* Can be summoned				*/
constexpr int PRF_NOREPEAT = 11;   /* No repetition of comm commands		*/
constexpr int PRF_HOLYLIGHT = 12;  /* Can see in dark				*/
constexpr int PRF_COLOR = 13;      /* Color					*/
constexpr int PRF_SPARE = 14;      /* Used to be second color bit		*/
constexpr int PRF_NOWIZ = 15;      /* Can't hear wizline			*/
constexpr int PRF_LOG1 = 16;       /* On-line System Log (low bit)		*/
constexpr int PRF_LOG2 = 17;       /* On-line System Log (high bit)		*/
constexpr int PRF_NOAUCT = 18;     /* Can't hear auction channel		*/
constexpr int PRF_NOGOSS = 19;     /* Can't hear gossip channel			*/
constexpr int PRF_NOGRATZ = 20;    /* Can't hear grats channel			*/
constexpr int PRF_ROOMFLAGS = 21;  /* Can see room flags (ROOM_x)		*/
constexpr int PRF_DISPAUTO = 22;   /* Show prompt HP, MP, MV when < 30%.	*/
constexpr int PRF_CLS = 23;        /* Clear screen in OasisOLC 			*/
constexpr int PRF_BUILDWALK = 24;  /* Build new rooms when walking		*/
constexpr int PRF_AFK = 25;        /* Player is AFK				*/
constexpr int PRF_AUTOLOOT = 26;   /* Loot everything from a corpse		*/
constexpr int PRF_AUTOGOLD = 27;   /* Loot gold from a corpse			*/
constexpr int PRF_AUTOSPLIT = 28;  /* Split gold with group			*/
constexpr int PRF_FULL_EXIT = 29;  /* Shows full autoexit details		*/
constexpr int PRF_AUTOSAC = 30;    /* Sacrifice a corpse 			*/
constexpr int PRF_AUTOMEM = 31;    /* Memorize spells				*/
constexpr int PRF_VIEWORDER = 32;  /* if you want to see the newest first 	*/
constexpr int PRF_NOCOMPRESS = 33; /* If you want to force MCCP2 off          	*/
constexpr int PRF_AUTOASSIST = 34; /* Auto-assist toggle                      	*/
constexpr int PRF_DISPKI = 35;     /* Display ki points in prompt 		*/
constexpr int PRF_DISPEXP = 36;    /* Display exp points in prompt 		*/
constexpr int PRF_DISPTNL = 37;    /* Display TNL exp points in prompt 		*/
constexpr int PRF_TEST = 38;       /* Sets triggers safety off for imms         */
constexpr int PRF_HIDE = 39;       /* Hide on who from other mortals            */
constexpr int PRF_NMWARN = 40;     /* No mail warning                           */
constexpr int PRF_HINTS = 41;      /* Receives hints                            */
constexpr int PRF_FURY = 42;       /* Sees fury meter                           */
constexpr int PRF_NODEC = 43;
constexpr int PRF_NOEQSEE = 44;
constexpr int PRF_NOMUSIC = 45;
constexpr int PRF_LKEEP = 46;
constexpr int PRF_DISTIME = 47; /* Part of Prompt Options */
constexpr int PRF_DISGOLD = 48; /* Part of Prompt Options */
constexpr int PRF_DISPRAC = 49; /* Part of Prompt Options */
constexpr int PRF_NOPARRY = 50;
constexpr int PRF_DISHUTH = 51; /* Part of Prompt Options */
constexpr int PRF_DISPERC = 52; /* Part of Prompt Options */
constexpr int PRF_CARVE = 53;
constexpr int PRF_ARENAWATCH = 54;
constexpr int PRF_NOGIVE = 55;
constexpr int PRF_INSTRUCT = 56;
constexpr int PRF_GHEALTH = 57;
constexpr int PRF_IHEALTH = 58;
constexpr int PRF_ENERGIZE = 59;
constexpr int PRF_FORM = 60;
constexpr int PRF_TECH = 61;

constexpr int NUM_PRF_FLAGS = 62;

/* Player autoexit levels: used as an index to exitlevels           */
constexpr int EXIT_OFF = 0;      /* Autoexit off                     */
constexpr int EXIT_NORMAL = 1;   /* Brief display (stock behaviour)  */
constexpr int EXIT_NA = 2;       /* Not implemented - do not use     */
constexpr int EXIT_COMPLETE = 3; /* Full display                     */

#define exitlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch), PRF_AUTOEXIT) ? 1 : 0) + (PRF_FLAGGED((ch), PRF_FULL_EXIT) ? 2 : 0) : 0)
#define EXIT_LEV(ch) (exitlevel(ch))

enum class AffectFlag
{
    // dontuse          = 0,   // DON'T USE!
    blind = 1,           // (R) Char is blind
    invisible = 2,       // Char is invisible
    detect_align = 3,    // Char is sensitive to align
    detect_invis = 4,    // Char can see invis chars
    detect_magic = 5,    // Char is sensitive to magic
    sense_life = 6,      // Char can sense hidden life
    waterwalk = 7,       // Char can walk on water
    sanctuary = 8,       // Char protected by sanct.
    group = 9,           // (R) Char is grouped
    curse = 10,          // Char is cursed
    infravision = 11,    // Char can see in dark
    poison = 12,         // (R) Char is poisoned
    weakened_state = 13, // Char protected from evil?
    protect_good = 14,   // Char protected from good
    sleep = 15,          // (R) Char magically asleep
    no_track = 16,       // Char can't be tracked
    undead = 17,         // Char is undead
    paralyze = 18,       // Char is paralyzed
    sneak = 19,          // Char can move quietly
    hide = 20,           // Char is hidden
    // unused_20        = 21,  // Room for future expansion
    charm = 22,           // Char is charmed
    flying = 23,          // Char is flying
    water_breathing = 24, // Char can breathe non-O2
    angelic = 25,         // Char is an angelic being
    ethereal = 26,        // Char is ethereal
    magic_only = 27,      // Char only hurt by magic
    next_partial = 28,    // Next action cannot be full
    next_no_action = 29,  // Next action cannot attack
    stunned = 30,         // Char is stunned
    tamed = 31,           // Char has been tamed
    creeping_death = 32,  // Char is undergoing creeping death
    spirit = 33,          // Char has no body
    stoneskin = 34,       // Char has temporary DR
    summoned = 35,        // Char is summoned (transient)
    celestial = 36,       // Char is celestial
    fiendish = 37,        // Char is fiendish
    fire_shield = 38,     // Char has fire shield
    low_light = 39,       // Char has low light eyes
    zanzoken = 40,        // Char is ready to zanzoken
    knocked = 41,         // Char is knocked OUT
    might = 42,           // Strength +3
    flex = 43,            // Agility +3
    genius = 44,          // Intelligence +3
    bless = 45,           // Bless for better regen
    burnt = 46,           // Disintegrated corpse
    burned = 47,          // Burned by honoo or similar skill
    mbreak = 48,          // Can't charge while flagged
    hasshuken = 49,       // Does double punch damage
    future = 50,          // Future Sight
    paralyzed = 51,       // Real Paralyze
    infuse = 52,          // Ki-infused attacks
    enlighten = 53,       // Enlighten
    frozen = 54,          // They got frozededed
    fireshield = 55,      // They have a blazing personality
    ensnared = 56,        // Silk ensnaring their arms!
    hayasa = 57,          // They are speedy!
    pursuit = 58,         // Being followed
    wither = 59,          // Their body is withered
    hydrozap = 60,        // Custom Skill Kanso Suru
    position = 61,        // Better combat position
    psychic_shocked = 62, // Psychic Shock
    metamorph = 63,       // Metamorphosis
    healglow = 64,        // Healing Glow
    earmor = 65,          // Ethereal Armor
    echains = 66,         // Ethereal Chains
    wunjo_rune = 67,      // Wunjo rune
    purisaz_rune = 68,    // Purisaz rune
    ashed = 69,           // Leaves ash
    puked = 70,
    liquefied = 71,
    shell = 72,
    immunity = 73,
    spiritcontrol = 74,
    ginyu_pose = 75,
    kyodaika = 76,
    shadowstitch = 77,
    echains_debuff = 78,
    starphase = 79,
    mbreak_debuff = 80,
    limit_breaking = 81
};

/* Affect bits: used in Character.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
constexpr int AFF_DONTUSE = 0;         /* DON'T USE! 		*/
constexpr int AFF_BLIND = 1;           /* (R) Char is blind         */
constexpr int AFF_INVISIBLE = 2;       /* Char is invisible         */
constexpr int AFF_DETECT_ALIGN = 3;    /* Char is sensitive to align*/
constexpr int AFF_DETECT_INVIS = 4;    /* Char can see invis chars  */
constexpr int AFF_DETECT_MAGIC = 5;    /* Char is sensitive to magic*/
constexpr int AFF_SENSE_LIFE = 6;      /* Char can sense hidden life*/
constexpr int AFF_WATERWALK = 7;       /* Char can walk on water    */
constexpr int AFF_SANCTUARY = 8;       /* Char protected by sanct.  */
constexpr int AFF_GROUP = 9;           /* (R) Char is grouped       */
constexpr int AFF_CURSE = 10;          /* Char is cursed            */
constexpr int AFF_INFRAVISION = 11;    /* Char can see in dark      */
constexpr int AFF_POISON = 12;         /* (R) Char is poisoned      */
constexpr int AFF_WEAKENED_STATE = 13; /* Char protected from evil  */
constexpr int AFF_PROTECT_GOOD = 14;   /* Char protected from good  */
constexpr int AFF_SLEEP = 15;          /* (R) Char magically asleep */
constexpr int AFF_NOTRACK = 16;        /* Char can't be tracked     */
constexpr int AFF_UNDEAD = 17;         /* Char is undead 		*/
constexpr int AFF_PARALYZE = 18;       /* Char is paralized		*/
constexpr int AFF_SNEAK = 19;          /* Char can move quietly     */
constexpr int AFF_HIDE = 20;           /* Char is hidden            */
constexpr int AFF_UNUSED20 = 21;       /* Room for future expansion */
constexpr int AFF_CHARM = 22;          /* Char is charmed         	*/
constexpr int AFF_FLYING = 23;         /* Char is flying         	*/
constexpr int AFF_WATERBREATH = 24;    /* Char can breath non O2    */
constexpr int AFF_ANGELIC = 25;        /* Char is an angelic being  */
constexpr int AFF_ETHEREAL = 26;       /* Char is ethereal          */
constexpr int AFF_MAGICONLY = 27;      /* Char only hurt by magic   */
constexpr int AFF_NEXTPARTIAL = 28;    /* Next action cannot be full*/
constexpr int AFF_NEXTNOACTION = 29;   /* Next action cannot attack (took full action between rounds) */
constexpr int AFF_STUNNED = 30;        /* Char is stunned		*/
constexpr int AFF_TAMED = 31;          /* Char has been tamed	*/
constexpr int AFF_CDEATH = 32;         /* Char is undergoing creeping death */
constexpr int AFF_SPIRIT = 33;         /* Char has no body          */
constexpr int AFF_STONESKIN = 34;      /* Char has temporary DR     */
constexpr int AFF_SUMMONED = 35;       /* Char is summoned (i.e. transient */
constexpr int AFF_CELESTIAL = 36;      /* Char is celestial         */
constexpr int AFF_FIENDISH = 37;       /* Char is fiendish          */
constexpr int AFF_FIRE_SHIELD = 38;    /* Char has fire shield      */
constexpr int AFF_LOW_LIGHT = 39;      /* Char has low light eyes   */
constexpr int AFF_ZANZOKEN = 40;       /* Char is ready to zanzoken */
constexpr int AFF_KNOCKED = 41;        /* Char is knocked OUT!      */
constexpr int AFF_MIGHT = 42;          /* Strength +3               */
constexpr int AFF_FLEX = 43;           /* Agility +3                */
constexpr int AFF_GENIUS = 44;         /* Intelligence +3           */
constexpr int AFF_BLESS = 45;          /* Bless for better regen    */
constexpr int AFF_BURNT = 46;          /* Disintergrated corpse     */
constexpr int AFF_BURNED = 47;         /* Burned by honoo or similar skill */
constexpr int AFF_MBREAK = 48;         /* Can't charge while flagged */
constexpr int AFF_HASS = 49;           /* Does double punch damage  */
constexpr int AFF_FUTURE = 50;         /* Future Sight */
constexpr int AFF_PARA = 51;           /* Real Paralyze */
constexpr int AFF_INFUSE = 52;         /* Ki infused attacks */
constexpr int AFF_ENLIGHTEN = 53;      /* Enlighten */
constexpr int AFF_FROZEN = 54;         /* They got frozededed */
constexpr int AFF_FIRESHIELD = 55;     /* They have a blazing personality */
constexpr int AFF_ENSNARED = 56;       /* They have silk ensnaring their arms! */
constexpr int AFF_HAYASA = 57;         /* They are speedy!                */
constexpr int AFF_PURSUIT = 58;        /* Being followed */
constexpr int AFF_WITHER = 59;         /* Their body is withered */
constexpr int AFF_HYDROZAP = 60;       /* Custom Skill Kanso Suru */
constexpr int AFF_POSITION = 61;       /* Better combat position */
constexpr int AFF_SHOCKED = 62;        /* Psychic Shock          */
constexpr int AFF_METAMORPH = 63;      /* Metamorphisis, Demon's Ripoff Custom Skill */
constexpr int AFF_HEALGLOW = 64;       /* Healing Glow */
constexpr int AFF_EARMOR = 65;         /* Ethereal Armor */
constexpr int AFF_ECHAINS = 66;        /* Ethereal Chains */
constexpr int AFF_WUNJO = 67;          /* Wunjo rune */
constexpr int AFF_POTENT = 68;         /* Purisaz rune */
constexpr int AFF_ASHED = 69;          /* Leaves ash */
constexpr int AFF_PUKED = 70;
constexpr int AFF_LIQUEFIED = 71;
constexpr int AFF_SHELL = 72;
constexpr int AFF_IMMUNITY = 73;
constexpr int AFF_SPIRITCONTROL = 74;
constexpr int AFF_POSE = 75;
constexpr int AFF_KYODAIKA = 76;
constexpr int AFF_SHADOWSTITCH = 77;
constexpr int AFF_ECHAINS_DEBUFF = 78;
constexpr int AFF_STARPHASE = 79;
constexpr int AFF_MBREAK_DEBUFF = 80;
constexpr int AFF_LIMIT_BREAKING = 81;

constexpr int NUM_AFF_FLAGS = 82;

/* Modes of connectedness: used by descriptor_data.state */
constexpr int CON_PLAYING = 0;       /* Playing - Nominal state		*/
constexpr int CON_CLOSE = 1;         /* User disconnect, remove character.	*/
constexpr int CON_GET_NAME = 2;      /* By what name ..?			*/
constexpr int CON_NAME_CNFRM = 3;    /* Did I get that right, x?		*/
constexpr int CON_PASSWORD = 4;      /* Password:				*/
constexpr int CON_NEWPASSWD = 5;     /* Give me a password for x		*/
constexpr int CON_CNFPASSWD = 6;     /* Please retype password:		*/
constexpr int CON_QSEX = 7;          /* Sex?					*/
constexpr int CON_QCLASS = 8;        /* Class?				*/
constexpr int CON_RMOTD = 9;         /* PRESS RETURN after MOTD		*/
constexpr int CON_MENU = 10;         /* Your choice: (main menu)		*/
constexpr int CON_EXDESC = 11;       /* Enter a new description:		*/
constexpr int CON_CHPWD_GETOLD = 12; /* Changing passwd: get old		*/
constexpr int CON_CHPWD_GETNEW = 13; /* Changing passwd: get new		*/
constexpr int CON_CHPWD_VRFY = 14;   /* Verify new password			*/
constexpr int CON_DELCNF1 = 15;      /* Delete confirmation 1		*/
constexpr int CON_DELCNF2 = 16;      /* Delete confirmation 2		*/
constexpr int CON_DISCONNECT = 17;   /* In-game link loss (leave character)	*/
constexpr int CON_OEDIT = 18;        /* OLC mode - object editor		*/
constexpr int CON_REDIT = 19;        /* OLC mode - room editor		*/
constexpr int CON_ZEDIT = 20;        /* OLC mode - zone info editor		*/
constexpr int CON_MEDIT = 21;        /* OLC mode - mobile editor		*/
constexpr int CON_SEDIT = 22;        /* OLC mode - shop editor		*/
constexpr int CON_TEDIT = 23;        /* OLC mode - text editor		*/
constexpr int CON_CEDIT = 24;        /* OLC mode - config editor		*/
constexpr int CON_QRACE = 25;        /* Race? 				*/
constexpr int CON_ASSEDIT = 26;      /* OLC mode - Assemblies                */
constexpr int CON_AEDIT = 27;        /* OLC mode - social (action) edit      */
constexpr int CON_TRIGEDIT = 28;     /* OLC mode - trigger edit              */
constexpr int CON_RACE_HELP = 29;    /* Race Help 				*/
constexpr int CON_CLASS_HELP = 30;   /* Class Help 				*/
constexpr int CON_QANSI = 31;        /* Ask for ANSI support     */
constexpr int CON_GEDIT = 32;        /* OLC mode - guild editor 		*/
constexpr int CON_QROLLSTATS = 33;   /* Reroll stats 			*/
constexpr int CON_IEDIT = 34;        /* OLC mode - individual edit		*/
constexpr int CON_LEVELUP = 35;      /* Level up menu			*/
constexpr int CON_QSTATS = 36;       /* Assign starting stats        	*/
constexpr int CON_HAIRL = 37;        /* Choose your hair length        */
constexpr int CON_HAIRS = 38;        /* Choose your hair style         */
constexpr int CON_HAIRC = 39;        /* Choose your hair color         */
constexpr int CON_SKIN = 40;         /* Choose your skin color         */
constexpr int CON_EYE = 41;          /* Choose your eye color          */
constexpr int CON_Q1 = 42;           /* Make a life choice!            */
constexpr int CON_Q2 = 43;           /* Make a second life choice!     */
constexpr int CON_Q3 = 44;           /* Make a third life choice!      */
constexpr int CON_Q4 = 45;           /* Make a fourth life choice!     */
constexpr int CON_Q5 = 46;           /* Make a fifth life choice!      */
constexpr int CON_Q6 = 47;           /* Make a sixth life choice!      */
constexpr int CON_Q7 = 48;           /* Make a seventh life choice!    */
constexpr int CON_Q8 = 49;           /* Make an eighth life choice!    */
constexpr int CON_Q9 = 50;           /* Make a ninth life choice!      */
constexpr int CON_QX = 51;           /* Make a tenth life choice!      */
constexpr int CON_HSEDIT = 52;       /* House Olc                      */
constexpr int CON_ALPHA = 53;        /* Alpha Password                 */
constexpr int CON_ALPHA2 = 54;       /* Alpha Password For Newb        */
constexpr int CON_ANDROID = 55;
constexpr int CON_HEDIT = 56; /* OLC mode - help edit           */
constexpr int CON_GET_USER = 57;
constexpr int CON_GET_EMAIL = 58;
constexpr int CON_UMENU = 59;
constexpr int CON_USER_CONF = 60;
constexpr int CON_DISTFEA = 61;
constexpr int CON_HEIGHT = 62;
constexpr int CON_WEIGHT = 63;
constexpr int CON_AURA = 64;
constexpr int CON_BONUS = 65;
constexpr int CON_NEGATIVE = 66;
constexpr int CON_NEWSEDIT = 67;
constexpr int CON_RACIAL = 68;
constexpr int CON_POBJ = 69;
constexpr int CON_ALIGN = 70;
constexpr int CON_SKILLS = 71;
constexpr int CON_USER_TITLE = 72;
constexpr int CON_GENOME = 73;
constexpr int CON_COPYOVER = 74;
constexpr int CON_LOGIN = 75;
constexpr int CON_QUITGAME = 76;

constexpr int NUM_CON_TYPES = 77;

/* Colors that the player can define */
constexpr int COLOR_NORMAL = 0;
constexpr int COLOR_ROOMNAME = 1;
constexpr int COLOR_ROOMOBJS = 2;
constexpr int COLOR_ROOMPEOPLE = 3;
constexpr int COLOR_HITYOU = 4;
constexpr int COLOR_YOUHIT = 5;
constexpr int COLOR_OTHERHIT = 6;
constexpr int COLOR_CRITICAL = 7;
constexpr int COLOR_HOLLER = 8;
constexpr int COLOR_SHOUT = 9;
constexpr int COLOR_GOSSIP = 10;
constexpr int COLOR_AUCTION = 11;
constexpr int COLOR_CONGRAT = 12;
constexpr int COLOR_TELL = 13;
constexpr int COLOR_YOUSAY = 14;
constexpr int COLOR_ROOMSAY = 15;

constexpr int NUM_COLOR = 16;

/* Character equipment positions: used as index for Character.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
constexpr int WEAR_UNUSED0 = 0;
constexpr int WEAR_FINGER_R = 1;
constexpr int WEAR_FINGER_L = 2;
constexpr int WEAR_NECK_1 = 3;
constexpr int WEAR_NECK_2 = 4;
constexpr int WEAR_BODY = 5;
constexpr int WEAR_HEAD = 6;
constexpr int WEAR_LEGS = 7;
constexpr int WEAR_FEET = 8;
constexpr int WEAR_HANDS = 9;
constexpr int WEAR_ARMS = 10;
constexpr int WEAR_UNUSED1 = 11;
constexpr int WEAR_ABOUT = 12;
constexpr int WEAR_WAIST = 13;
constexpr int WEAR_WRIST_R = 14;
constexpr int WEAR_WRIST_L = 15;
constexpr int WEAR_WIELD1 = 16;
constexpr int WEAR_WIELD2 = 17;
constexpr int WEAR_BACKPACK = 18;
constexpr int WEAR_EAR_R = 19;
constexpr int WEAR_EAR_L = 20;
constexpr int WEAR_SH = 21;
constexpr int WEAR_EYE = 22;

enum class WearSlot : uint8_t {
    Inventory = 0, // not actually equipped, but signifies that something can be in an inventory.
    RightFinger = 1,
    LeftFinger = 2,
    Neck1 = 3,
    Neck2 = 4,
    Body = 5,
    Head = 6,
    Legs = 7,
    Feet = 8,
    Hands = 9,
    Arms = 10,
    About = 12,
    Waist = 13,
    RightWrist = 14,
    LeftWrist = 15,
    Wield1 = 16,
    Wield2 = 17,
    Backpack = 18,
    RightEar = 19,
    LeftEar = 20,
    Shield = 21,
    Eyes = 22
};

constexpr int NUM_WEARS = 23; /* This must be the # of eq positions!! */

constexpr int SPELL_LEVEL_0 = 0;
constexpr int SPELL_LEVEL_1 = 1;
constexpr int SPELL_LEVEL_2 = 2;
constexpr int SPELL_LEVEL_3 = 3;
constexpr int SPELL_LEVEL_4 = 4;
constexpr int SPELL_LEVEL_5 = 5;
constexpr int SPELL_LEVEL_6 = 6;
constexpr int SPELL_LEVEL_7 = 7;
constexpr int SPELL_LEVEL_8 = 8;
constexpr int SPELL_LEVEL_9 = 9;

constexpr int MAX_SPELL_LEVEL = 10;             /* how many spell levels */
constexpr int MAX_MEM = (MAX_SPELL_LEVEL * 10); /* how many total spells */

constexpr int DOMAIN_UNDEFINED = -1;
constexpr int DOMAIN_AIR = 0;
constexpr int DOMAIN_ANIMAL = 1;
constexpr int DOMAIN_CHAOS = 2;
constexpr int DOMAIN_DEATH = 3;
constexpr int DOMAIN_DESTRUCTION = 4;
constexpr int DOMAIN_EARTH = 5;
constexpr int DOMAIN_EVIL = 6;
constexpr int DOMAIN_FIRE = 7;
constexpr int DOMAIN_GOOD = 8;
constexpr int DOMAIN_HEALING = 9;
constexpr int DOMAIN_KNOWLEDGE = 10;
constexpr int DOMAIN_LAW = 11;
constexpr int DOMAIN_LUCK = 12;
constexpr int DOMAIN_MAGIC = 13;
constexpr int DOMAIN_PLANT = 14;
constexpr int DOMAIN_PROTECTION = 15;
constexpr int DOMAIN_STRENGTH = 16;
constexpr int DOMAIN_SUN = 17;
constexpr int DOMAIN_TRAVEL = 18;
constexpr int DOMAIN_TRICKERY = 19;
constexpr int DOMAIN_UNIVERSAL = 20;
constexpr int DOMAIN_WAR = 22;
constexpr int DOMAIN_WATER = 23;
constexpr int DOMAIN_ARTIFACE = 24;
constexpr int DOMAIN_CHARM = 25;
constexpr int DOMAIN_COMMUNITY = 26;
constexpr int DOMAIN_CREATION = 27;
constexpr int DOMAIN_DARKNESS = 28;
constexpr int DOMAIN_GLORY = 29;
constexpr int DOMAIN_LIBERATION = 30;
constexpr int DOMAIN_MADNESS = 31;
constexpr int DOMAIN_NOBILITY = 32;
constexpr int DOMAIN_REPOSE = 33;
constexpr int DOMAIN_RUNE = 34;
constexpr int DOMAIN_SCALYKIND = 35;
constexpr int DOMAIN_WEATHER = 36;

constexpr int NUM_DOMAINS = 37;

constexpr int SCHOOL_UNDEFINED = -1;
constexpr int SCHOOL_ABJURATION = 0;
constexpr int SCHOOL_CONJURATION = 1;
constexpr int SCHOOL_DIVINATION = 2;
constexpr int SCHOOL_ENCHANTMENT = 3;
constexpr int SCHOOL_EVOCATION = 4;
constexpr int SCHOOL_ILLUSION = 5;
constexpr int SCHOOL_NECROMANCY = 6;
constexpr int SCHOOL_TRANSMUTATION = 7;
constexpr int SCHOOL_UNIVERSAL = 8;

constexpr int NUM_SCHOOLS = 10;

constexpr int DEITY_UNDEFINED = -1;

constexpr int NUM_DEITIES = 0;

/* Combat feats that apply to a specific weapon type */
constexpr int CFEAT_IMPROVED_CRITICAL = 0;
constexpr int CFEAT_WEAPON_FINESSE = 1;
constexpr int CFEAT_WEAPON_FOCUS = 2;
constexpr int CFEAT_WEAPON_SPECIALIZATION = 3;
constexpr int CFEAT_GREATER_WEAPON_FOCUS = 4;
constexpr int CFEAT_GREATER_WEAPON_SPECIALIZATION = 5;

constexpr int CFEAT_MAX = 5;

/* Spell feats that apply to a specific school of spells */
constexpr int CFEAT_SPELL_FOCUS = 0;
constexpr int CFEAT_GREATER_SPELL_FOCUS = 1;

constexpr int SFEAT_MAX = 1;

/* object-related defines ********************************************/

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

constexpr int NUM_ITEM_TYPES = 37;

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

constexpr int NUM_ITEM_WEARS = 19;

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

// TODO: after completing migration, reduce this to 39
constexpr int NUM_ITEM_FLAGS = 96;

/* Modifier constants used with obj affects ('A' fields) */
constexpr int APPLY_NONE = 0; /* No effect */

constexpr int APPLY_CATTR_BASE = 1;      /* Bitwise: Flat Modifier applied to Attribute base */
constexpr int APPLY_CATTR_MULT = 2;      /* Bitwise: Modifier for Attribute mult */
constexpr int APPLY_CATTR_POST = 3;      /* Bitwise: Flat modifier added after mult.  */
constexpr int APPLY_CATTR_GAIN_MULT = 4; /* bitwise: other stats gain multiplier */

constexpr int APPLY_CVIT_BASE = 5;       /* Bitwise: Flat modifier applied to vital base */
constexpr int APPLY_CVIT_MULT = 6;       /* Bitwise: modifier for vitals mult */
constexpr int APPLY_CVIT_POST = 7;       /* Bitwise: flat modifier applied after mult */
constexpr int APPLY_CVIT_GAIN_MULT = 8;  /* Bitwise: base gains multiplier */
constexpr int APPLY_CVIT_REGEN_MULT = 9; /* Bitwise: regen multiplier */
constexpr int APPLY_CVIT_DOT_MULT = 10;  /* Bitwise: damage over time multiplier */

constexpr int APPLY_CSTAT_BASE = 11;      /* bitwise: other stats base */
constexpr int APPLY_CSTAT_MULT = 12;      /* bitwise: other stats multiplier */
constexpr int APPLY_CSTAT_POST = 13;      /* bitwise: flat modifier applied after mult */
constexpr int APPLY_CSTAT_GAIN_MULT = 14; /* bitwise: other stats gain multiplier */

constexpr int APPLY_CDIM_BASE = 15; /* bitwise: character dimension base */
constexpr int APPLY_CDIM_MULT = 16; /* bitwise: character dimension multiplier */
constexpr int APPLY_CDIM_POST = 17; /* bitwise: flat modifier applied after mult */

constexpr int APPLY_COMBAT_BASE = 18; /* bitwise: combat base bonuses */
constexpr int APPLY_COMBAT_MULT = 19; /* bitwise: combat multiplicative bonuses */

constexpr int APPLY_DTYPE_RES = 20; /* bitwise: damage type resistance */
constexpr int APPLY_DTYPE_BON = 21; /* bitwise: damage type bonuses */

constexpr int APPLY_ATKTIER_RES = 22;       /* bitwise: attack tier resistance */
constexpr int APPLY_ATKTIER_BON = 23;       /* bitwise: attack tier bonuses */
constexpr int APPLY_TRANS_UPKEEP_CVIT = 24; /* bitwise: cvitals cost modifier for transformations */

constexpr int APPLY_SKILL = 25; /* !bitwise: Apply to a specific skill    */

constexpr int APPLY_CDER_BASE = 26; /* bitwise: character derived base */
constexpr int APPLY_CDER_MULT = 27; /* bitwise: character derived multiplier */
constexpr int APPLY_CDER_POST = 28; /* bitwise: character derived post multiplier */

constexpr int NUM_APPLIES = 29;

// derived stat specifics
constexpr int CDER_CARRY_CAPACITY = 1 << 0;

/* Container flags - value[1] */
constexpr int CONT_CLOSEABLE = (1 << 0); /* Container can be closed	*/
constexpr int CONT_PICKPROOF = (1 << 1); /* Container is pickproof	*/
constexpr int CONT_CLOSED = (1 << 2);    /* Container is closed		*/
constexpr int CONT_LOCKED = (1 << 3);    /* Container is locked		*/

constexpr int NUM_CONT_FLAGS = 4;

/* Some different kind of liquids for use in values of drink containers */
constexpr int LIQ_WATER = 0;
constexpr int LIQ_BEER = 1;
constexpr int LIQ_WINE = 2;
constexpr int LIQ_ALE = 3;
constexpr int LIQ_DARKALE = 4;
constexpr int LIQ_WHISKY = 5;
constexpr int LIQ_LEMONADE = 6;
constexpr int LIQ_FIREBRT = 7;
constexpr int LIQ_LOCALSPC = 8;
constexpr int LIQ_SLIME = 9;
constexpr int LIQ_MILK = 10;
constexpr int LIQ_TEA = 11;
constexpr int LIQ_COFFE = 12;
constexpr int LIQ_BLOOD = 13;
constexpr int LIQ_SALTWATER = 14;
constexpr int LIQ_CLEARWATER = 15;

constexpr int NUM_LIQ_TYPES = 16;

constexpr int MATERIAL_BONE = 0;
constexpr int MATERIAL_CERAMIC = 1;
constexpr int MATERIAL_COPPER = 2;
constexpr int MATERIAL_DIAMOND = 3;
constexpr int MATERIAL_GOLD = 4;
constexpr int MATERIAL_IRON = 5;
constexpr int MATERIAL_LEATHER = 6;
constexpr int MATERIAL_MITHRIL = 7;
constexpr int MATERIAL_OBSIDIAN = 8;
constexpr int MATERIAL_STEEL = 9;
constexpr int MATERIAL_STONE = 10;
constexpr int MATERIAL_SILVER = 11;
constexpr int MATERIAL_WOOD = 12;
constexpr int MATERIAL_GLASS = 13;
constexpr int MATERIAL_ORGANIC = 14;
constexpr int MATERIAL_CURRENCY = 15;
constexpr int MATERIAL_PAPER = 16;
constexpr int MATERIAL_COTTON = 17;
constexpr int MATERIAL_SATIN = 18;
constexpr int MATERIAL_SILK = 19;
constexpr int MATERIAL_BURLAP = 20;
constexpr int MATERIAL_VELVET = 21;
constexpr int MATERIAL_PLATINUM = 22;
constexpr int MATERIAL_ADAMANTINE = 23;
constexpr int MATERIAL_WOOL = 24;
constexpr int MATERIAL_ONYX = 25;
constexpr int MATERIAL_IVORY = 26;
constexpr int MATERIAL_BRASS = 27;
constexpr int MATERIAL_MARBLE = 28;
constexpr int MATERIAL_BRONZE = 29;
constexpr int MATERIAL_KACHIN = 30;
constexpr int MATERIAL_RUBY = 31;
constexpr int MATERIAL_SAPPHIRE = 32;
constexpr int MATERIAL_EMERALD = 33;
constexpr int MATERIAL_GEMSTONE = 34;
constexpr int MATERIAL_GRANITE = 35;
constexpr int MATERIAL_ENERGY = 36;
constexpr int MATERIAL_HEMP = 37;
constexpr int MATERIAL_CRYSTAL = 38;
constexpr int MATERIAL_EARTH = 39;
constexpr int MATERIAL_LIQUID = 40;
constexpr int MATERIAL_CLOTH = 41;
constexpr int MATERIAL_METAL = 42;
constexpr int MATERIAL_WAX = 43;
constexpr int MATERIAL_OTHER = 44;
constexpr int MATERIAL_FOOD = 45;
constexpr int MATERIAL_OIL = 46;

constexpr int NUM_MATERIALS = 47;

/* other miscellaneous defines *******************************************/

/* Player conditions */
constexpr int DRUNK = 0;
constexpr int HUNGER = 1;
constexpr int THIRST = 2;

/* Sun state for weather_data */
constexpr int SUN_DARK = 0;
constexpr int SUN_RISE = 1;
constexpr int SUN_LIGHT = 2;
constexpr int SUN_SET = 3;

/* Sky conditions for weather_data */
constexpr int SKY_CLOUDLESS = 0;
constexpr int SKY_CLOUDY = 1;
constexpr int SKY_RAINING = 2;
constexpr int SKY_LIGHTNING = 3;

/* Rent codes */
constexpr int RENT_UNDEF = 0;
constexpr int RENT_CRASH = 1;
constexpr int RENT_RENTED = 2;
constexpr int RENT_CRYO = 3;
constexpr int RENT_FORCED = 4;
constexpr int RENT_TIMEDOUT = 5;

/* for the 128bits */
constexpr int RF_ARRAY_MAX = 4;
constexpr int PM_ARRAY_MAX = 4;
constexpr int PR_ARRAY_MAX = 4;
constexpr int AF_ARRAY_MAX = 4;
constexpr int TW_ARRAY_MAX = 4;
constexpr int EF_ARRAY_MAX = 4;
constexpr int AD_ARRAY_MAX = 4;
constexpr int FT_ARRAY_MAX = 4;
constexpr int ZF_ARRAY_MAX = 4;
constexpr int SW_ARRAY_MAX = 4;
constexpr int GW_ARRAY_MAX = 4;

/* History */
constexpr int HIST_ALL = 0;
constexpr int HIST_SAY = 1;
constexpr int HIST_GOSSIP = 2;
constexpr int HIST_WIZNET = 3;
constexpr int HIST_TELL = 4;
constexpr int HIST_SHOUT = 5;
constexpr int HIST_GRATS = 6;
constexpr int HIST_HOLLER = 7;
constexpr int HIST_AUCTION = 8;
constexpr int HIST_SNET = 9;

constexpr int NUM_HIST = 10;

/* other #defined constants **********************************************/

/*
 * ADMLVL_IMPL should always be the HIGHEST possible admin level, and
 * ADMLVL_IMMORT should always be the LOWEST immortal level.
 */
constexpr int ADMLVL_NONE = 0;
constexpr int ADMLVL_IMMORT = 1;
constexpr int ADMLVL_BUILDER = 2;
constexpr int ADMLVL_GOD = 3;
constexpr int ADMLVL_VICE = 4;
constexpr int ADMLVL_GRGOD = 5;
constexpr int ADMLVL_IMPL = 6;

/* First character level that forces epic levels */
constexpr int LVL_EPICSTART = 101;

enum class AdminFlag : uint8_t
{
    tell_all = 0,       // Can use 'tell all' to broadcast GOD
    see_invisible = 1,  // Sees other chars inventory IMM
    see_secret = 2,     // Sees secret doors IMM
    know_weather = 3,   // Knows details of weather GOD
    full_where = 4,     // Full output of 'where' command IMM
    money = 5,          // Char has a bottomless wallet GOD
    eat_anything = 6,   // Char can eat anything GOD
    no_poison = 7,      // Char can't be poisoned IMM
    walk_anywhere = 8,  // Char has unrestricted walking IMM
    no_keys = 9,        // Char needs no keys for locks GOD
    instant_kill = 10,  // "kill" command is instant IMPL
    no_steal = 11,      // Char cannot be stolen from IMM
    trans_all = 12,     // Can use 'trans all' GRGOD
    switch_mortal = 13, // Can 'switch' to a mortal PC body IMPL
    force_mass = 14,    // Can force rooms or all GRGOD
    all_houses = 15,    // Can enter any house GRGOD
    no_damage = 16,     // Cannot be damaged IMM
    all_shops = 17,     // Can use all shops GOD
    cedit = 18          // Can use cedit IMPL
};

/*
 * ADM flags - define admin privs for chars
 */
constexpr int ADM_TELLALL = 0;       /* Can use 'tell all' to broadcast GOD */
constexpr int ADM_SEEINV = 1;        /* Sees other chars inventory IMM */
constexpr int ADM_SEESECRET = 2;     /* Sees secret doors IMM */
constexpr int ADM_KNOWWEATHER = 3;   /* Knows details of weather GOD */
constexpr int ADM_FULLWHERE = 4;     /* Full output of 'where' command IMM */
constexpr int ADM_MONEY = 5;         /* Char has a bottomless wallet GOD */
constexpr int ADM_EATANYTHING = 6;   /* Char can eat anything GOD */
constexpr int ADM_NOPOISON = 7;      /* Char can't be poisoned IMM */
constexpr int ADM_WALKANYWHERE = 8;  /* Char has unrestricted walking IMM */
constexpr int ADM_NOKEYS = 9;        /* Char needs no keys for locks GOD */
constexpr int ADM_INSTANTKILL = 10;  /* "kill" command is instant IMPL */
constexpr int ADM_NOSTEAL = 11;      /* Char cannot be stolen from IMM */
constexpr int ADM_TRANSALL = 12;     /* Can use 'trans all' GRGOD */
constexpr int ADM_SWITCHMORTAL = 13; /* Can 'switch' to a mortal PC body IMPL */
constexpr int ADM_FORCEMASS = 14;    /* Can force rooms or all GRGOD */
constexpr int ADM_ALLHOUSES = 15;    /* Can enter any house GRGOD */
constexpr int ADM_NODAMAGE = 16;     /* Cannot be damaged IMM */
constexpr int ADM_ALLSHOPS = 17;     /* Can use all shops GOD */
constexpr int ADM_CEDIT = 18;        /* Can use cedit IMPL */

constexpr int NUM_ADMFLAGS = 19;

/* Level of the 'freeze' command */
constexpr int ADMLVL_FREEZE = ADMLVL_GRGOD;

enum class Direction : uint8_t
{
    north = 0,
    east = 1,
    south = 2,
    west = 3,
    up = 4,
    down = 5,
    northwest = 6,
    northeast = 7,
    southeast = 8,
    southwest = 9,
    inside = 10,
    outside = 11
};

constexpr int NUM_OF_DIRS = 12; /* number of directions in a room (nsewud) */

/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 */
constexpr int OPT_USEC = 100000; /* 10 passes per second */
#define PASSES_PER_SEC (1000000 / OPT_USEC)
#define RL_SEC *1.0
#define CD_TICK RL_SEC

#define PULSE_ZONE (CONFIG_PULSE_ZONE RL_SEC)
#define PULSE_MOBILE (CONFIG_PULSE_MOBILE RL_SEC)
#define PULSE_VIOLENCE (CONFIG_PULSE_VIOLENCE RL_SEC)
#define PULSE_AUCTION (15 RL_SEC)
#define PULSE_AUTOSAVE (CONFIG_PULSE_AUTOSAVE RL_SEC)
#define PULSE_IDLEPWD (CONFIG_PULSE_IDLEPWD RL_SEC)
#define PULSE_SANITY (CONFIG_PULSE_SANITY RL_SEC)
#define PULSE_USAGE (CONFIG_PULSE_SANITY * 60 RL_SEC)       /* 5 mins */
#define PULSE_TIMESAVE (CONFIG_PULSE_TIMESAVE * 300 RL_SEC) /* should be >= SECS_PER_MUD_HOUR */
#define PULSE_CURRENT (CONFIG_PULSE_CURRENT RL_SEC)
#define PULSE_1SEC (1.0 RL_SEC)
#define PULSE_2SEC (2.0 RL_SEC)
#define PULSE_3SEC (3.0 RL_SEC)
#define PULSE_4SEC (4.0 RL_SEC)
#define PULSE_5SEC (5.0 RL_SEC)
#define PULSE_6SEC (6.0 RL_SEC)
#define PULSE_7SEC (7.0 RL_SEC)

/* Cool Down Ticks */
#define PULSE_CD1 (1.0 CD_TICK)
#define PULSE_CD2 (2.0 CD_TICK)
#define PULSE_CD3 (3.0 CD_TICK)
#define PULSE_CD4 (4.0 CD_TICK) /* This and the 3 above are for safety */
#define PULSE_CD5 (5.0 CD_TICK) /* Punch */
#define PULSE_CD6 (6.0 CD_TICK)
#define PULSE_CD7 (7.0 CD_TICK)
#define PULSE_CD8 (8.0 CD_TICK)
#define PULSE_CD9 (9.0 CD_TICK)
#define PULSE_CD10 (10.0 CD_TICK)
#define PULSE_CD11 (11.0 CD_TICK)
#define PULSE_CD12 (12.0 CD_TICK)
/* End CD Ticks    */

/* Variables for the output buffering system */
constexpr int MAX_SOCK_BUF = (96 * 1024); /* Size of kernel's sock buf   */
constexpr int MAX_PROMPT_LENGTH = 1024;   /* Max length of prompt        */
constexpr int GARBAGE_SPACE = 512;        /* Space for **OVERFLOW** etc  */
constexpr int SMALL_BUFSIZE = 6020;       /* Static output buffer size   */
/* Max amount of output that can be buffered */
constexpr int LARGE_BUFSIZE = (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH);

constexpr int HISTORY_SIZE = 5; /* Keep last 5 commands. */
constexpr int MAX_STRING_LENGTH = 64936;
constexpr int MAX_INPUT_LENGTH = 2048;     /* Max length per *line* of
 input */
constexpr int MAX_RAW_INPUT_LENGTH = 4096; /* Max size of *raw* input */
constexpr int MAX_MESSAGES = 100;
constexpr int MAX_NAME_LENGTH = 20;
constexpr int MAX_PWD_LENGTH = 30;
constexpr int MAX_TITLE_LENGTH = 120;
constexpr int HOST_LENGTH = 40;
constexpr int EXDSCR_LENGTH = 16384;
constexpr int MAX_TONGUE = 3;
constexpr int MAX_SKILLS = 200;
constexpr int MAX_AFFECT = 32;
constexpr int MAX_OBJ_AFFECT = 20;
constexpr int MAX_NOTE_LENGTH = 6000; /* arbitrary */
constexpr int SKILL_TABLE_SIZE = 1000;
constexpr int SPELLBOOK_SIZE = 50;
constexpr int MAX_FEATS = 750;
constexpr int MAX_HELP_KEYWORDS = 256;
constexpr int MAX_HELP_ENTRY = MAX_STRING_LENGTH;
constexpr int NUM_FEATS_DEFINED = 252;
constexpr int MAX_ARMOR_TYPES = 5;
constexpr int NUM_CONFIG_SECTIONS = 7;
constexpr int NUM_CREATION_METHODS = 5;
constexpr int NUM_ATTACK_TYPES = 15;
constexpr int NUM_MTRIG_TYPES = 22;
constexpr int NUM_OTRIG_TYPES = 22;
constexpr int NUM_WTRIG_TYPES = 22;
constexpr int NUM_ZONE_FLAGS = 36;
constexpr int NUM_TRADERS = 78;
constexpr int NUM_SHOP_FLAGS = 3;
constexpr int NUM_DOOR_CMD = 5;
constexpr int MAX_ASSM = 11;
constexpr int NUM_FULLNESS = 5;
constexpr int NUM_WEEK_DAYS = 7;
constexpr int NUM_MONTHS = 12;
constexpr int NUM_CONDITIONS = 3;
constexpr int NUM_WIELD_NAMES = 4;

/* define the largest set of commands for a trigger */
constexpr int MAX_CMD_LENGTH = 16384; /* 16k should be plenty and then some */

/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */

/* object-related structures ******************************************/

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

constexpr int LEVELTYPE_CLASS = 1;
constexpr int LEVELTYPE_RACE = 2;

constexpr int MASTERY_THRESHOLD = 5000;
constexpr int LIMIT_THRESHOLD = 100000;
constexpr int LIMITBREAK_THRESHOLD = 5000000;

using attribute_t = uint8_t;
enum class CharAttribute : uint8_t
{
    strength = 1 << 0,     // 1
    agility = 1 << 1,      // 2
    intelligence = 1 << 2, // 4
    wisdom = 1 << 3,       // 8
    constitution = 1 << 4, // 16
    speed = 1 << 5         // 32
};

template <>
struct magic_enum::customize::enum_range<CharAttribute>
{
    static constexpr bool is_flags = true;
};

using attribute_train_t = uint32_t;
enum class CharTrain : uint8_t
{
    strength = 1 << 0,     // 1
    agility = 1 << 1,      // 2
    intelligence = 1 << 2, // 4
    wisdom = 1 << 3,       // 8
    constitution = 1 << 4, // 16
    speed = 1 << 5         // 32
};

using align_t = int16_t;
enum class CharAlign : uint8_t
{
    good_evil = 1 << 0,
    law_chaos = 1 << 1,
};

enum class MoralAlign : uint8_t
{
    evil = 0,
    neutral = 1,
    good = 2
};

using money_t = uint64_t;
enum class CharMoney : uint8_t
{
    carried = 1 << 0,
    bank = 1 << 1
};

enum class CharVital : uint8_t
{
    health = 1 << 0,
    ki = 1 << 1,
    stamina = 1 << 2,
    lifeforce = 1 << 3
};

enum class CharStat : uint8_t
{
    experience = 1 << 0,
    skill_train = 1 << 1,
    practices = 1 << 2,
    upgrade_points = 1 << 3
};

using dim_t = double;
enum class CharDim : uint8_t
{
    height = 1 << 0,
    weight = 1 << 1,
};

using weight_t = dim_t;
using effect_t = uint16_t;

enum class ComStat : uint8_t
{
    accuracy = 1 << 0,
    damage = 1 << 1,
    armor = 1 << 2,
    parry = 1 << 3,
    dodge = 1 << 4,
    block = 1 << 5,
    perfect_dodge = 1 << 6,
    defense = 1 << 7,
};

enum class AtkTier : uint8_t
{
    one = 1 << 0,
    two = 1 << 1,
    three = 1 << 2,
    four = 1 << 3,
    five = 1 << 4,
    six = 1 << 5
};

enum class DamType : uint8_t
{
    physical = 1 << 0,
    ki = 1 << 1,
};

enum class ShopFlag : uint8_t
{
    start_fight = 0,
    bank_money = 1,
    allow_steal = 2,
    no_broken = 3
};

enum class Mutation : uint8_t
{
    extreme_speed = 0,
    increased_cell_regeneration = 1,
    extreme_reflexes = 2,
    infravision = 3,
    natural_camouflage = 4,
    limb_regeneration = 5,
    venomous = 6,
    rubbery_body = 7,
    innate_telepathy = 8,
    natural_energy = 9
};

enum class UnitType : uint8_t
{
    character = 0,
    object = 1,
    room = 2,
    unknown = 3
};

constexpr UnitType MOB_TRIGGER = UnitType::character;
constexpr UnitType OBJ_TRIGGER = UnitType::object;
constexpr UnitType WLD_TRIGGER = UnitType::room;


/* various constants *****************************************************/

/* defines for mudlog() */
constexpr int OFF = 0;
constexpr int BRF = 1;
constexpr int NRM = 2;
constexpr int CMP = 3;

/* get_filename() */
constexpr int CRASH_FILE = 0;
constexpr int ETEXT_FILE = 1;
constexpr int ALIAS_FILE = 2;
constexpr int SCRIPT_VARS_FILE = 3;
constexpr int NEW_OBJ_FILES = 4;
constexpr int PLR_FILE = 5;
constexpr int PET_FILE = 6;
constexpr int IMC_FILE = 7; /**< The IMC2 Data for players */
constexpr int USER_FILE = 8; /* User Account System */
constexpr int INTRO_FILE = 9;
constexpr int SENSE_FILE = 10;
constexpr int CUSTOME_FILE = 11;
constexpr int MAX_FILES = 12;

/* breadth-first searching */
constexpr int BFS_ERROR = -1;
constexpr int BFS_ALREADY_THERE = -2;
constexpr int BFS_TO_FAR = -3;
constexpr int BFS_NO_PATH = -4;

/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time */

constexpr double MUD_TIME_ACCELERATION = 12.0;  // 12 MUD seconds pass per real second.

constexpr double SECONDS_PER_MINUTE = 60.0;
constexpr double MINUTES_PER_HOUR = 60.0;
constexpr double HOURS_PER_DAY = 24.0;
constexpr double DAYS_PER_WEEK = 7.0;
constexpr double DAYS_PER_MONTH = 30.0;
constexpr double MONTHS_PER_YEAR = 12.0;
constexpr double DAYS_PER_YEAR = 365.0;

constexpr double SECS_PER_MINUTE = SECONDS_PER_MINUTE;
constexpr double SECS_PER_HOUR =  (SECONDS_PER_MINUTE*MINUTES_PER_HOUR);
constexpr double SECS_PER_DAY =   (SECS_PER_HOUR*HOURS_PER_DAY);
constexpr double SECS_PER_WEEK =  (SECS_PER_DAY*DAYS_PER_WEEK);
constexpr double SECS_PER_MONTH = (SECS_PER_DAY*DAYS_PER_MONTH);
constexpr double SECS_PER_YEAR =  (SECS_PER_DAY*DAYS_PER_YEAR);
constexpr double SECS_PER_GAME_YEAR = (SECS_PER_MONTH*MONTHS_PER_YEAR);

#define IS_SET(flag, bit)  ((flag) & (bit))
#define SET_BIT(var, bit)  ((var) |= (bit))
#define REMOVE_BIT(var, bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit) ((var) ^= (bit))

#define REMOVE_FROM_LIST(item, head, next, cmtemp)    \
   if ((item) == (head))        \
      (head) = (item)->next;        \
   else {                \
      (cmtemp) = head;            \
      while ((cmtemp) && ((cmtemp)->next != (item))) \
     (cmtemp) = (cmtemp)->next;        \
      if (cmtemp)                \
         (cmtemp)->next = (item)->next;    \
   }                    \

#define REMOVE_FROM_DOUBLE_LIST(item, head, next, prev)\
      if((item) == (head))            \
      {                        \
            (head) = (item)->next;        \
            if(head) (head)->prev = nullptr;        \
      }                        \
      else                    \
      {                        \
        temp = head;                \
          while(temp && (temp->next != (item)))    \
            temp = temp->next;            \
             if(temp)                \
            {                    \
               temp->next = (item)->next;        \
               if((item)->next)            \
                (item)->next->prev = temp;    \
            }                    \
      }                        \

#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
    (((major) << 16) + ((minor) << 8) + (patchlevel))

#define CREATE(result, type, number)  do {\
    if ((number) * sizeof(type) <= 0)    \
        basic_mud_log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);    \
    if (!((result) = (type *) calloc ((number), sizeof(type))))    \
        { perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result, type, number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
        { perror("SYSERR: realloc failure"); abort(); } } while(0)

#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

/* See also: ANA, SANA */
#define AN(string) (strchr("aeiouAEIOU", *(string)) ? "an" : "a")