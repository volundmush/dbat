/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#pragma once
#include <cstdint>
#include "const/Max.hpp"
#include "const/Position.hpp"

struct Character;
struct Object;

constexpr int DEFAULT_STAFF_LVL = 12;
constexpr int DEFAULT_WAND_LVL = 12;

constexpr int CAST_UNDEFINED = -1;
constexpr int CAST_SPELL = 0;
constexpr int CAST_POTION = 1;
constexpr int CAST_WAND = 2;
constexpr int CAST_STAFF = 3;
constexpr int CAST_SCROLL = 4;
constexpr int CAST_STRIKE = 5;

#define MAG_DAMAGE        (1 << 0)
#define MAG_AFFECTS        (1 << 1)
#define MAG_UNAFFECTS        (1 << 2)
#define MAG_POINTS        (1 << 3)
#define MAG_ALTER_OBJS        (1 << 4)
#define MAG_GROUPS        (1 << 5)
#define MAG_MASSES        (1 << 6)
#define MAG_AREAS        (1 << 7)
#define MAG_SUMMONS        (1 << 8)
#define MAG_CREATIONS        (1 << 9)
#define MAG_MANUAL        (1 << 10)
#define MAG_AFFECTSV        (1 << 11)
#define MAG_ACTION_FREE        (1 << 12)
#define MAG_ACTION_PARTIAL    (1 << 13)
#define MAG_ACTION_FULL        (1 << 14)
#define MAG_NEXTSTRIKE        (1 << 15)
#define MAG_TOUCH_MELEE        (1 << 16)
#define MAG_TOUCH_RANGED    (1 << 17)

#define MAGSAVE_FORT        (1 << 0)
#define MAGSAVE_REFLEX        (1 << 1)
#define MAGSAVE_WILL        (1 << 2)
#define MAGSAVE_HALF        (1 << 3)
#define MAGSAVE_NONE        (1 << 4)
#define MAGSAVE_PARTIAL        (1 << 5)

#define MAGCOMP_DIVINE_FOCUS    (1 << 0)
#define MAGCOMP_EXP_COST    (1 << 1)
#define MAGCOMP_FOCUS        (1 << 2)
#define MAGCOMP_MATERIAL    (1 << 3)
#define MAGCOMP_SOMATIC        (1 << 4)
#define MAGCOMP_VERBAL        (1 << 5)


constexpr int TYPE_UNDEFINED = -1;
constexpr int SPELL_RESERVED_DBC = 0;  /* SKILL NUMBER ZERO -- RESERVED */

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
constexpr int SPELL_MAGE_ARMOR = 1;
constexpr int SPELL_TELEPORT = 2;
constexpr int SPELL_BLESS = 3;
constexpr int SPELL_BLINDNESS = 4;
constexpr int SPELL_BURNING_HANDS = 5;
constexpr int SPELL_CALL_LIGHTNING = 6;
constexpr int SPELL_CHARM = 7;
constexpr int SPELL_CHILL_TOUCH = 8;
constexpr int SPELL_COLOR_SPRAY = 10;
constexpr int SPELL_CONTROL_WEATHER = 11;
constexpr int SPELL_CREATE_FOOD = 12;
constexpr int SPELL_CREATE_WATER = 13;
constexpr int SPELL_REMOVE_BLINDNESS = 14;
constexpr int SPELL_CURE_CRITIC = 15;
constexpr int SPELL_CURE_LIGHT = 16;
constexpr int SPELL_BANE = 17;
constexpr int SPELL_DETECT_ALIGN = 18;
constexpr int SPELL_SEE_INVIS = 19;
constexpr int SPELL_DETECT_MAGIC = 20;
constexpr int SPELL_DETECT_POISON = 21;
constexpr int SPELL_DISPEL_EVIL = 22;
constexpr int SPELL_EARTHQUAKE = 23;
constexpr int SPELL_ENCHANT_WEAPON = 24;
constexpr int SPELL_ENERGY_DRAIN = 25;
constexpr int SPELL_FIREBALL = 26;
constexpr int SPELL_HARM = 27;
constexpr int SPELL_HEAL = 28;
constexpr int SPELL_INVISIBLE = 29;
constexpr int SPELL_LIGHTNING_BOLT = 30;
constexpr int SPELL_LOCATE_OBJECT = 31;
constexpr int SPELL_MAGIC_MISSILE = 32;
constexpr int SPELL_POISON = 33;
constexpr int SPELL_PROT_FROM_EVIL = 34;
constexpr int SPELL_REMOVE_CURSE = 35;
constexpr int SPELL_SANCTUARY = 36;
constexpr int SPELL_SHOCKING_GRASP = 37;
constexpr int SPELL_SLEEP = 38;
constexpr int SPELL_BULL_STRENGTH = 39;
constexpr int SPELL_SUMMON = 40;
constexpr int SPELL_VENTRILOQUATE = 41;
constexpr int SPELL_WORD_OF_RECALL = 42;
constexpr int SPELL_NEUTRALIZE_POISON = 43;
constexpr int SPELL_SENSE_LIFE = 44;
constexpr int SPELL_ANIMATE_DEAD = 45;
constexpr int SPELL_DISPEL_GOOD = 46;
constexpr int SPELL_GROUP_ARMOR = 47;
constexpr int SPELL_MASS_HEAL = 48;
constexpr int SPELL_GROUP_RECALL = 49;
constexpr int SPELL_DARKVISION = 50;
constexpr int SPELL_WATERWALK = 51;
constexpr int SPELL_PORTAL = 52;
constexpr int SPELL_PARALYZE = 53;
constexpr int SPELL_INFLICT_LIGHT = 54;
constexpr int SPELL_INFLICT_CRITIC = 55;
constexpr int SPELL_IDENTIFY = 56;
constexpr int SPELL_FAERIE_FIRE = 57;
constexpr int ABIL_TURNING = 58;
constexpr int ABIL_LAY_HANDS = 59;
constexpr int SPELL_RESISTANCE = 60;
constexpr int SPELL_ACID_SPLASH = 61;
constexpr int SPELL_DAZE = 62;
constexpr int SPELL_FLARE = 63;
constexpr int SPELL_RAY_OF_FROST = 64;
constexpr int SPELL_DISRUPT_UNDEAD = 65;
constexpr int SPELL_LESSER_GLOBE_OF_INVUL = 66;
constexpr int SPELL_STONESKIN = 67;
constexpr int SPELL_MINOR_CREATION = 68;
constexpr int SPELL_SUMMON_MONSTER_I = 69;
constexpr int SPELL_SUMMON_MONSTER_II = 70;
constexpr int SPELL_SUMMON_MONSTER_III = 71;
constexpr int SPELL_SUMMON_MONSTER_IV = 72;
constexpr int SPELL_SUMMON_MONSTER_V = 73;
constexpr int SPELL_SUMMON_MONSTER_VI = 74;
constexpr int SPELL_SUMMON_MONSTER_VII = 75;
constexpr int SPELL_SUMMON_MONSTER_VIII = 76;
constexpr int SPELL_SUMMON_MONSTER_IX = 77;
constexpr int SPELL_FIRE_SHIELD = 78;
constexpr int SPELL_ICE_STORM = 79;
constexpr int SPELL_SHOUT = 80;
constexpr int SPELL_FEAR = 81;
constexpr int SPELL_CLOUDKILL = 82;
constexpr int SPELL_MAJOR_CREATION = 83;
constexpr int SPELL_HOLD_MONSTER = 84;
constexpr int SPELL_CONE_OF_COLD = 85;
constexpr int SPELL_ANIMAL_GROWTH = 86;
constexpr int SPELL_BALEFUL_POLYMORPH = 87;
constexpr int SPELL_PASSWALL = 88;
constexpr int SPELL_BESTOW_CURSE = 89;
constexpr int SPELL_SENSU = 90;
constexpr int SPELL_HAYASA = 91;

#define MIN_LANGUAGES            SKILL_LANG_COMMON
constexpr int SKILL_LANG_COMMON = 141;
constexpr int SKILL_LANG_ELVEN = 142;
constexpr int SKILL_LANG_GNOME = 143;
constexpr int SKILL_LANG_DWARVEN = 144;
constexpr int SKILL_LANG_HALFLING = 145;
constexpr int SKILL_LANG_ORC = 146;
constexpr int SKILL_LANG_DRUIDIC = 147;
constexpr int SKILL_LANG_DRACONIC = 148;
#define MAX_LANGUAGES            SKILL_LANG_DRACONIC


constexpr int SKILL_WP_UNARMED = 179; /* Barehanded weapon group        */

constexpr int SPELL_FIRE_BREATH = 202;
constexpr int SPELL_GAS_BREATH = 203;
constexpr int SPELL_FROST_BREATH = 204;
constexpr int SPELL_ACID_BREATH = 205;
constexpr int SPELL_LIGHTNING_BREATH = 206;

#define MAX_SPELLS            SPELL_SENSU

/*
 * to make an affect induced by dg_affect look correct on 'stat' we need
 * to define it with a 'spellname'.
 */
constexpr int SPELL_DG_AFFECT = 298;

/* WEAPON ATTACK TYPES */

constexpr int TYPE_HIT = 300;
constexpr int TYPE_STING = 301;
constexpr int TYPE_WHIP = 302;
constexpr int TYPE_SLASH = 303;
constexpr int TYPE_BITE = 304;
constexpr int TYPE_BLUDGEON = 305;
constexpr int TYPE_CRUSH = 306;
constexpr int TYPE_POUND = 307;
constexpr int TYPE_CLAW = 308;
constexpr int TYPE_MAUL = 309;
constexpr int TYPE_THRASH = 310;
constexpr int TYPE_PIERCE = 311;
constexpr int TYPE_BLAST = 312;
constexpr int TYPE_PUNCH = 313;
constexpr int TYPE_STAB = 314;
/* new attack types can be added here - up to TYPE_SUFFERING */
constexpr int TYPE_SUFFERING = 399;

constexpr int SKILL_FLEX = 400;
constexpr int SKILL_GENIUS = 401;
constexpr int SKILL_SOLARF = 402;
constexpr int SKILL_MIGHT = 403;
constexpr int SKILL_BALANCE = 404;
constexpr int SKILL_BUILD = 405;
constexpr int SKILL_TSKIN = 406;
constexpr int SKILL_CONCENTRATION = 407;
constexpr int SKILL_KAIOKEN = 408;
constexpr int SKILL_SPOT = 409;
constexpr int SKILL_FIRST_AID = 410;
constexpr int SKILL_DISGUISE = 411;
constexpr int SKILL_ESCAPE_ARTIST = 412;
constexpr int SKILL_APPRAISE = 413;
constexpr int SKILL_HEAL = 414;
constexpr int SKILL_FORGERY = 415;
constexpr int SKILL_HIDE = 416;
constexpr int SKILL_BLESS = 417;
constexpr int SKILL_CURSE = 418;
constexpr int SKILL_LISTEN = 419;
constexpr int SKILL_EAVESDROP = 420;
constexpr int SKILL_POISON = 421;
constexpr int SKILL_CURE = 422;
constexpr int SKILL_OPEN_LOCK = 423;
constexpr int SKILL_VIGOR = 424;
constexpr int SKILL_REGENERATE = 425;
constexpr int SKILL_KEEN = 426;
constexpr int SKILL_SEARCH = 427;
constexpr int SKILL_MOVE_SILENTLY = 428;
constexpr int SKILL_ABSORB = 429;
constexpr int SKILL_SLEIGHT_OF_HAND = 430;
constexpr int SKILL_INGEST = 431;
constexpr int SKILL_REPAIR = 432;
constexpr int SKILL_SENSE = 433;
constexpr int SKILL_SURVIVAL = 434;
constexpr int SKILL_YOIK = 435;
constexpr int SKILL_CREATE = 436;
constexpr int SKILL_SPIT = 437;
constexpr int SKILL_POTENTIAL = 438;
constexpr int SKILL_TELEPATHY = 439;
constexpr int SKILL_RENZO = 440;
constexpr int SKILL_MASENKO = 441;
constexpr int SKILL_DODONPA = 442;
constexpr int SKILL_BARRIER = 443;
constexpr int SKILL_GALIKGUN = 444;
constexpr int SKILL_THROW = 445;
constexpr int SKILL_DODGE = 446;
constexpr int SKILL_PARRY = 447;
constexpr int SKILL_BLOCK = 448;
constexpr int SKILL_PUNCH = 449;
constexpr int SKILL_KICK = 450;
constexpr int SKILL_ELBOW = 451;
constexpr int SKILL_KNEE = 452;
constexpr int SKILL_ROUNDHOUSE = 453;
constexpr int SKILL_UPPERCUT = 454;
constexpr int SKILL_SLAM = 455;
constexpr int SKILL_HEELDROP = 456;
constexpr int SKILL_FOCUS = 457;
constexpr int SKILL_KIBALL = 458;
constexpr int SKILL_KIBLAST = 459;
constexpr int SKILL_BEAM = 460;
constexpr int SKILL_TSUIHIDAN = 461;
constexpr int SKILL_SHOGEKIHA = 462;
constexpr int SKILL_ZANZOKEN = 463;
constexpr int SKILL_KAMEHAMEHA = 464;
constexpr int SKILL_DAGGER = 465;
constexpr int SKILL_SWORD = 466;
constexpr int SKILL_CLUB = 467;
constexpr int SKILL_SPEAR = 468;
constexpr int SKILL_GUN = 469;
constexpr int SKILL_BRAWL = 470;
constexpr int SKILL_INSTANTT = 471;
constexpr int SKILL_DEATHBEAM = 472;
constexpr int SKILL_ERASER = 473;
constexpr int SKILL_TSLASH = 474;
constexpr int SKILL_PSYBLAST = 475;
constexpr int SKILL_HONOO = 476;
constexpr int SKILL_DUALBEAM = 477;
constexpr int SKILL_ROGAFUFUKEN = 478;
constexpr int SKILL_POSE = 479;
constexpr int SKILL_BAKUHATSUHA = 480;
constexpr int SKILL_KIENZAN = 481;
constexpr int SKILL_TRIBEAM = 482;
constexpr int SKILL_SBC = 483;
constexpr int SKILL_FINALFLASH = 484;
constexpr int SKILL_CRUSHER = 485;
constexpr int SKILL_DDSLASH = 486;
constexpr int SKILL_PBARRAGE = 487;
constexpr int SKILL_HELLFLASH = 488;
constexpr int SKILL_HELLSPEAR = 489;
constexpr int SKILL_KAKUSANHA = 490;
constexpr int SKILL_HASSHUKEN = 491;
constexpr int SKILL_SCATTER = 492;
constexpr int SKILL_BIGBANG = 493;
constexpr int SKILL_PSLASH = 494;
constexpr int SKILL_DEATHBALL = 495;
constexpr int SKILL_SPIRITBALL = 496;
constexpr int SKILL_GENKIDAMA = 497;
constexpr int SKILL_GENOCIDE = 498;
constexpr int SKILL_DUALWIELD = 499;
constexpr int SKILL_KURA = 500;
constexpr int SKILL_TAILWHIP = 501;
constexpr int SKILL_KOUSENGAN = 502;
constexpr int SKILL_TAISHA = 503;
constexpr int SKILL_PARALYZE = 505;
constexpr int SKILL_INFUSE = 506;
constexpr int SKILL_ROLL = 507;
constexpr int SKILL_TRIP = 508;
constexpr int SKILL_GRAPPLE = 509;
constexpr int SKILL_WSPIKE = 510;
constexpr int SKILL_SELFD = 511;
constexpr int SKILL_SPIRAL = 512;
constexpr int SKILL_BREAKER = 513;
constexpr int SKILL_ENLIGHTEN = 514;
constexpr int SKILL_COMMUNE = 515;
constexpr int SKILL_MIMIC = 516;
constexpr int SKILL_WRAZOR = 517;
constexpr int SKILL_KOTEIRU = 518;
constexpr int SKILL_DIMIZU = 519;
constexpr int SKILL_HYOGA_KABE = 520;
constexpr int SKILL_WELLSPRING = 521;
constexpr int SKILL_AQUA_BARRIER = 522;
constexpr int SKILL_WARP = 523;
constexpr int SKILL_HSPIRAL = 524;
constexpr int SKILL_ARMOR = 525;
constexpr int SKILL_FIRESHIELD = 526;
constexpr int SKILL_COOKING = 527;
constexpr int SKILL_SEISHOU = 528;
constexpr int SKILL_SILK = 529;
constexpr int SKILL_BASH = 530;
constexpr int SKILL_HEADBUTT = 531;
constexpr int SKILL_ENSNARE = 532;
constexpr int SKILL_STARNOVA = 533;
constexpr int SKILL_PURSUIT = 534;
constexpr int SKILL_ZEN = 535;
constexpr int SKILL_SUNDER = 536;
constexpr int SKILL_WITHER = 537;
constexpr int SKILL_TWOHAND = 538;
constexpr int SKILL_STYLE = 539;
constexpr int SKILL_METAMORPH = 540;
constexpr int SKILL_HEALGLOW = 541;
constexpr int SKILL_RUNIC = 542;
constexpr int SKILL_EXTRACT = 543;
constexpr int SKILL_GARDENING = 544;
constexpr int SKILL_ENERGIZE = 545;
constexpr int SKILL_MALICE = 549;
constexpr int SKILL_HAYASA = 550;
constexpr int SKILL_HANDLING = 551;
constexpr int SKILL_MYSTICMUSIC = 552;
constexpr int SKILL_LIGHTGRENADE = 553;
constexpr int SKILL_MULTIFORM = 554;
constexpr int SKILL_SPIRITCONTROL = 555;
constexpr int SKILL_BALEFIRE = 556;
constexpr int SKILL_BLESSEDHAMMER = 557;

constexpr int ART_STUNNING_FIST = 1000;
constexpr int ART_WHOLENESS_OF_BODY = 1001;
constexpr int ART_ABUNDANT_STEP = 1002;
constexpr int ART_QUIVERING_PALM = 1003;
constexpr int ART_EMPTY_BODY = 1004;

constexpr int SAVING_FORTITUDE = 0;
constexpr int SAVING_REFLEX = 1;
constexpr int SAVING_WILL = 2;

constexpr int SAVING_OBJ_IMPACT = 0;
constexpr int SAVING_OBJ_HEAT = 1;
constexpr int SAVING_OBJ_COLD = 2;
constexpr int SAVING_OBJ_BREATH = 3;
constexpr int SAVING_OBJ_SPELL = 4;

#define TAR_IGNORE      (1 << 0)
#define TAR_CHAR_ROOM   (1 << 1)
#define TAR_CHAR_WORLD  (1 << 2)
#define TAR_FIGHT_SELF  (1 << 3)
#define TAR_FIGHT_VICT  (1 << 4)
#define TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF    (1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     (1 << 7)
#define TAR_OBJ_ROOM    (1 << 8)
#define TAR_OBJ_WORLD   (1 << 9)
#define TAR_OBJ_EQUIP    (1 << 10)

constexpr int SKTYPE_NONE = 0;
#define SKTYPE_SPELL        (1 << 0)
#define SKTYPE_SKILL        (1 << 1)
#define SKTYPE_LANG        (1 << 2)
#define SKTYPE_WEAPON        (1 << 3)
#define SKTYPE_ART        (1 << 4)

#define SKFLAG_NEEDTRAIN    (1 << 0) /* Disallow use of 0 skill with only stat mod */
#define SKFLAG_STRMOD        (1 << 1)
#define SKFLAG_DEXMOD        (1 << 2)
#define SKFLAG_CONMOD        (1 << 3)
#define SKFLAG_INTMOD        (1 << 4)
#define SKFLAG_WISMOD        (1 << 5)
#define SKFLAG_CHAMOD        (1 << 6)
#define SKFLAG_ARMORBAD        (1 << 7)
#define SKFLAG_ARMORALL        (1 << 8)
#define SKFLAG_TIER1            (1 << 9)
#define SKFLAG_TIER2            (1 << 10)
#define SKFLAG_TIER3            (1 << 11)
#define SKFLAG_TIER4            (1 << 12)
#define SKFLAG_TIER5            (1 << 13)

constexpr int SKLEARN_CANT = 0; /* This class can't learn this skill */
constexpr int SKLEARN_CROSSCLASS = 1; /* Cross-class skill for this class */
constexpr int SKLEARN_CLASS = 2; /* Class skill for this class */
constexpr int SKLEARN_BOOL = 3; /* Skill is known or not */

struct spell_info_type {
    Position min_position{POS_DEAD};    /* Position for caster	 */
    int mana_min;        /* Min amount of mana used by a spell (highest lev) */
    int mana_max;    /* Max amount of mana used by a spell (lowest lev) */
    int mana_change;     /* Change in mana used by spell from lev to lev */
    int ki_min;        /* Min amount of mana used by a spell (highest lev) */
    int ki_max;        /* Max amount of mana used by a spell (lowest lev) */
    int ki_change;    /* Change in mana used by spell from lev to lev */

    int min_level[NUM_CLASSES];
    int routines;
    int8_t violent;
    int targets;         /* See below for use with TAR_XXX  */
    const char *name;    /* Input size not limited. Originates from string constants. */
    const char *wear_off_msg;    /* Input size not limited. Originates from string constants. */
    int race_can_learn[NUM_RACES];
    int skilltype;       /* Is it a spell, skill, art, feat, or what? used as bitvector */
    int flags;
    int save_flags;
    int comp_flags;
    int8_t can_learn_skill[NUM_CLASSES];
    int spell_level;
    int school;
    int domain;
};

extern struct spell_info_type spell_info[SKILL_TABLE_SIZE];

/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

constexpr int SPELL_TYPE_SPELL = 0;
constexpr int SPELL_TYPE_POTION = 1;
constexpr int SPELL_TYPE_WAND = 2;
constexpr int SPELL_TYPE_STAFF = 3;
constexpr int SPELL_TYPE_SCROLL = 4;


/* Attacktypes with grammar */

struct attack_hit_type {
    const char *singular;
    const char *plural;
};


#define ASPELL(spellname) \
extern void spellname(int level, Character *ch, \
          Character *victim, Object *obj, const char *arg)

#define MANUAL_SPELL(spellname)    spellname(level, caster, cvict, ovict, arg);

ASPELL(spell_create_water);

ASPELL(spell_recall);

ASPELL(spell_teleport);

ASPELL(spell_summon);

ASPELL(spell_locate_object);

ASPELL(spell_charm);

ASPELL(spell_information);

ASPELL(spell_identify);

ASPELL(spell_enchant_weapon);

ASPELL(spell_detect_poison);

ASPELL(spell_portal);

ASPELL(art_abundant_step);

/* basic magic calling functions */

extern int find_skill_num(char *name, int sktype);

extern int mag_damage(int level, Character *ch, Character *victim,
                      int spellnum);

extern void mag_affects(int level, Character *ch, Character *victim,
                        int spellnum);

extern void mag_groups(int level, Character *ch, int spellnum);

extern void mag_masses(int level, Character *ch, int spellnum);

extern void mag_areas(int level, Character *ch, int spellnum);

extern void mag_summons(int level, Character *ch, Object *obj, int spellnum, char *arg);

extern void mag_points(int level, Character *ch, Character *victim,
                       int spellnum);

extern void mag_unaffects(int level, Character *ch, Character *victim,
                          int spellnum);

extern void mag_alter_objs(int level, Character *ch, Object *obj,
                           int spellnum);

extern void mag_creations(int level, Character *ch, int spellnum);

extern void mag_affectsv(int level, Character *ch, Character *victim,
                         int spellnum);

extern int call_magic(Character *caster, Character *cvict,
                      Object *ovict, int spellnum, int level, int casttype, char *arg);

extern void mag_objectmagic(Character *ch, Object *obj,
                            char *argument);

extern int cast_spell(Character *ch, Character *tch,
                      Object *tobj, int spellnum, char *arg);

extern int mag_newsaves(Character *ch, Character *victim, int spellnum,
                        int level, int cast_stat);


/* other prototypes */
extern void skill_level(int spell, int chclass, int level);

extern void skill_race_class(int spell, int race, int learntype);

extern void skill_class(int skill, int chclass, int learntype);

const char *skill_name(int num);

extern int roll_skill(Character *ch, int snum);

extern int roll_resisted(Character *actor, int sact, Character *resistor, int sres);

extern int skill_type(int skill);
