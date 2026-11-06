#pragma once
#include "dbat/consts/types.h"
#include "dbat/consts/races.h"
#include "dbat/consts/senseis.h"
#include "dbat/consts/skills.h"

#ifdef __cplusplus
extern "C" {
#endif

constexpr int DEFAULT_STAFF_LVL = 12;
constexpr int DEFAULT_WAND_LVL  = 12;

constexpr int CAST_UNDEFINED = (-1);
constexpr int CAST_SPELL     = 0;
constexpr int CAST_POTION    = 1;
constexpr int CAST_WAND      = 2;
constexpr int CAST_STAFF     = 3;
constexpr int CAST_SCROLL    = 4;
constexpr int CAST_STRIKE    = 5;

constexpr int MAG_DAMAGE         = (1 << 0);
constexpr int MAG_AFFECTS        = (1 << 1);
constexpr int MAG_UNAFFECTS      = (1 << 2);
constexpr int MAG_POINTS         = (1 << 3);
constexpr int MAG_ALTER_OBJS     = (1 << 4);
constexpr int MAG_GROUPS         = (1 << 5);
constexpr int MAG_MASSES         = (1 << 6);
constexpr int MAG_AREAS          = (1 << 7);
constexpr int MAG_SUMMONS        = (1 << 8);
constexpr int MAG_CREATIONS      = (1 << 9);
constexpr int MAG_MANUAL         = (1 << 10);
constexpr int MAG_AFFECTSV       = (1 << 11);
constexpr int MAG_ACTION_FREE    = (1 << 12);
constexpr int MAG_ACTION_PARTIAL = (1 << 13);
constexpr int MAG_ACTION_FULL    = (1 << 14);
constexpr int MAG_NEXTSTRIKE     = (1 << 15);
constexpr int MAG_TOUCH_MELEE    = (1 << 16);
constexpr int MAG_TOUCH_RANGED   = (1 << 17);

constexpr int MAGSAVE_FORT    = (1 << 0);
constexpr int MAGSAVE_REFLEX  = (1 << 1);
constexpr int MAGSAVE_WILL    = (1 << 2);
constexpr int MAGSAVE_HALF    = (1 << 3);
constexpr int MAGSAVE_NONE    = (1 << 4);
constexpr int MAGSAVE_PARTIAL = (1 << 5);

constexpr int MAGCOMP_DIVINE_FOCUS = (1 << 0);
constexpr int MAGCOMP_EXP_COST     = (1 << 1);
constexpr int MAGCOMP_FOCUS        = (1 << 2);
constexpr int MAGCOMP_MATERIAL     = (1 << 3);
constexpr int MAGCOMP_SOMATIC      = (1 << 4);
constexpr int MAGCOMP_VERBAL       = (1 << 5);

constexpr int TYPE_UNDEFINED     = (-1);
constexpr int SPELL_RESERVED_DBC = 0; /* SKILL NUMBER ZERO -- RESERVED */

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
constexpr int SPELL_MAGE_ARMOR            = 1;
constexpr int SPELL_TELEPORT              = 2;
constexpr int SPELL_BLESS                 = 3;
constexpr int SPELL_BLINDNESS             = 4;
constexpr int SPELL_BURNING_HANDS         = 5;
constexpr int SPELL_CALL_LIGHTNING        = 6;
constexpr int SPELL_CHARM                 = 7;
constexpr int SPELL_CHILL_TOUCH           = 8;
constexpr int SPELL_COLOR_SPRAY           = 10;
constexpr int SPELL_CONTROL_WEATHER       = 11;
constexpr int SPELL_CREATE_FOOD           = 12;
constexpr int SPELL_CREATE_WATER          = 13;
constexpr int SPELL_REMOVE_BLINDNESS      = 14;
constexpr int SPELL_CURE_CRITIC           = 15;
constexpr int SPELL_CURE_LIGHT            = 16;
constexpr int SPELL_BANE                  = 17;
constexpr int SPELL_DETECT_ALIGN          = 18;
constexpr int SPELL_SEE_INVIS             = 19;
constexpr int SPELL_DETECT_MAGIC          = 20;
constexpr int SPELL_DETECT_POISON         = 21;
constexpr int SPELL_DISPEL_EVIL           = 22;
constexpr int SPELL_EARTHQUAKE            = 23;
constexpr int SPELL_ENCHANT_WEAPON        = 24;
constexpr int SPELL_ENERGY_DRAIN          = 25;
constexpr int SPELL_FIREBALL              = 26;
constexpr int SPELL_HARM                  = 27;
constexpr int SPELL_HEAL                  = 28;
constexpr int SPELL_INVISIBLE             = 29;
constexpr int SPELL_LIGHTNING_BOLT        = 30;
constexpr int SPELL_LOCATE_OBJECT         = 31;
constexpr int SPELL_MAGIC_MISSILE         = 32;
constexpr int SPELL_POISON                = 33;
constexpr int SPELL_PROT_FROM_EVIL        = 34;
constexpr int SPELL_REMOVE_CURSE          = 35;
constexpr int SPELL_SANCTUARY             = 36;
constexpr int SPELL_SHOCKING_GRASP        = 37;
constexpr int SPELL_SLEEP                 = 38;
constexpr int SPELL_BULL_STRENGTH         = 39;
constexpr int SPELL_SUMMON                = 40;
constexpr int SPELL_VENTRILOQUATE         = 41;
constexpr int SPELL_WORD_OF_RECALL        = 42;
constexpr int SPELL_NEUTRALIZE_POISON     = 43;
constexpr int SPELL_SENSE_LIFE            = 44;
constexpr int SPELL_ANIMATE_DEAD          = 45;
constexpr int SPELL_DISPEL_GOOD           = 46;
constexpr int SPELL_GROUP_ARMOR           = 47;
constexpr int SPELL_MASS_HEAL             = 48;
constexpr int SPELL_GROUP_RECALL          = 49;
constexpr int SPELL_DARKVISION            = 50;
constexpr int SPELL_WATERWALK             = 51;
constexpr int SPELL_PORTAL                = 52;
constexpr int SPELL_PARALYZE              = 53;
constexpr int SPELL_INFLICT_LIGHT         = 54;
constexpr int SPELL_INFLICT_CRITIC        = 55;
constexpr int SPELL_IDENTIFY              = 56;
constexpr int SPELL_FAERIE_FIRE           = 57;
constexpr int ABIL_TURNING                = 58;
constexpr int ABIL_LAY_HANDS              = 59;
constexpr int SPELL_RESISTANCE            = 60;
constexpr int SPELL_ACID_SPLASH           = 61;
constexpr int SPELL_DAZE                  = 62;
constexpr int SPELL_FLARE                 = 63;
constexpr int SPELL_RAY_OF_FROST          = 64;
constexpr int SPELL_DISRUPT_UNDEAD        = 65;
constexpr int SPELL_LESSER_GLOBE_OF_INVUL = 66;
constexpr int SPELL_STONESKIN             = 67;
constexpr int SPELL_MINOR_CREATION        = 68;
constexpr int SPELL_SUMMON_MONSTER_I      = 69;
constexpr int SPELL_SUMMON_MONSTER_II     = 70;
constexpr int SPELL_SUMMON_MONSTER_III    = 71;
constexpr int SPELL_SUMMON_MONSTER_IV     = 72;
constexpr int SPELL_SUMMON_MONSTER_V      = 73;
constexpr int SPELL_SUMMON_MONSTER_VI     = 74;
constexpr int SPELL_SUMMON_MONSTER_VII    = 75;
constexpr int SPELL_SUMMON_MONSTER_VIII   = 76;
constexpr int SPELL_SUMMON_MONSTER_IX     = 77;
constexpr int SPELL_FIRE_SHIELD           = 78;
constexpr int SPELL_ICE_STORM             = 79;
constexpr int SPELL_SHOUT                 = 80;
constexpr int SPELL_FEAR                  = 81;
constexpr int SPELL_CLOUDKILL             = 82;
constexpr int SPELL_MAJOR_CREATION        = 83;
constexpr int SPELL_HOLD_MONSTER          = 84;
constexpr int SPELL_CONE_OF_COLD          = 85;
constexpr int SPELL_ANIMAL_GROWTH         = 86;
constexpr int SPELL_BALEFUL_POLYMORPH     = 87;
constexpr int SPELL_PASSWALL              = 88;
constexpr int SPELL_BESTOW_CURSE          = 89;
constexpr int SPELL_SENSU                 = 90;
constexpr int SPELL_HAYASA                = 91;

constexpr int SKILL_LANG_COMMON   = 141;
constexpr int MIN_LANGUAGES       = SKILL_LANG_COMMON;
constexpr int SKILL_LANG_ELVEN    = 142;
constexpr int SKILL_LANG_GNOME    = 143;
constexpr int SKILL_LANG_DWARVEN  = 144;
constexpr int SKILL_LANG_HALFLING = 145;
constexpr int SKILL_LANG_ORC      = 146;
constexpr int SKILL_LANG_DRUIDIC  = 147;
constexpr int SKILL_LANG_DRACONIC = 148;
constexpr int MAX_LANGUAGES       = SKILL_LANG_DRACONIC;

constexpr int SKILL_WP_UNARMED = 179; /* Barehanded weapon group        */

constexpr int SPELL_FIRE_BREATH      = 202;
constexpr int SPELL_GAS_BREATH       = 203;
constexpr int SPELL_FROST_BREATH     = 204;
constexpr int SPELL_ACID_BREATH      = 205;
constexpr int SPELL_LIGHTNING_BREATH = 206;

constexpr int MAX_SPELLS = SPELL_SENSU;

/*
 * to make an affect induced by dg_affect look correct on 'stat' we need
 * to define it with a 'spellname'.
 */
constexpr int SPELL_DG_AFFECT = 298;

/* WEAPON ATTACK TYPES */

constexpr int ART_STUNNING_FIST     = 1000;
constexpr int ART_WHOLENESS_OF_BODY = 1001;
constexpr int ART_ABUNDANT_STEP     = 1002;
constexpr int ART_QUIVERING_PALM    = 1003;
constexpr int ART_EMPTY_BODY        = 1004;

constexpr int SAVING_FORTITUDE = 0;
constexpr int SAVING_REFLEX    = 1;
constexpr int SAVING_WILL      = 2;

constexpr int SAVING_OBJ_IMPACT = 0;
constexpr int SAVING_OBJ_HEAT   = 1;
constexpr int SAVING_OBJ_COLD   = 2;
constexpr int SAVING_OBJ_BREATH = 3;
constexpr int SAVING_OBJ_SPELL  = 4;

constexpr int TAR_IGNORE     = (1 << 0);
constexpr int TAR_CHAR_ROOM  = (1 << 1);
constexpr int TAR_CHAR_WORLD = (1 << 2);
constexpr int TAR_FIGHT_SELF = (1 << 3);
constexpr int TAR_FIGHT_VICT = (1 << 4);
constexpr int TAR_SELF_ONLY  = (1 << 5); /* Only a check, use with i.e. TAR_CHAR_ROOM */
constexpr int TAR_NOT_SELF   = (1 << 6); /* Only a check, use with i.e. TAR_CHAR_ROOM */
constexpr int TAR_OBJ_INV    = (1 << 7);
constexpr int TAR_OBJ_ROOM   = (1 << 8);
constexpr int TAR_OBJ_WORLD  = (1 << 9);
constexpr int TAR_OBJ_EQUIP  = (1 << 10);

constexpr int SKTYPE_NONE   = 0;
constexpr int SKTYPE_SPELL  = (1 << 0);
constexpr int SKTYPE_SKILL  = (1 << 1);
constexpr int SKTYPE_LANG   = (1 << 2);
constexpr int SKTYPE_WEAPON = (1 << 3);
constexpr int SKTYPE_ART    = (1 << 4);

constexpr int SKFLAG_NEEDTRAIN = (1 << 0); /* Disallow use of 0 skill with only stat mod */
constexpr int SKFLAG_STRMOD    = (1 << 1);
constexpr int SKFLAG_DEXMOD    = (1 << 2);
constexpr int SKFLAG_CONMOD    = (1 << 3);
constexpr int SKFLAG_INTMOD    = (1 << 4);
constexpr int SKFLAG_WISMOD    = (1 << 5);
constexpr int SKFLAG_CHAMOD    = (1 << 6);
constexpr int SKFLAG_ARMORBAD  = (1 << 7);
constexpr int SKFLAG_ARMORALL  = (1 << 8);
constexpr int SKFLAG_TIER1     = (1 << 9);
constexpr int SKFLAG_TIER2     = (1 << 10);
constexpr int SKFLAG_TIER3     = (1 << 11);
constexpr int SKFLAG_TIER4     = (1 << 12);
constexpr int SKFLAG_TIER5     = (1 << 13);

constexpr int SKLEARN_CANT       = 0; /* This class can't learn this skill */
constexpr int SKLEARN_CROSSCLASS = 1; /* Cross-class skill for this class */
constexpr int SKLEARN_CLASS      = 2; /* Class skill for this class */
constexpr int SKLEARN_BOOL       = 3; /* Skill is known or not */

struct spell_info_type {
  int8_t min_position; /* Position for caster	 */
  int mana_min;        /* Min amount of mana used by a spell (highest lev) */
  int mana_max;        /* Max amount of mana used by a spell (lowest lev) */
  int mana_change;     /* Change in mana used by spell from lev to lev */
  int ki_min;          /* Min amount of mana used by a spell (highest lev) */
  int ki_max;          /* Max amount of mana used by a spell (lowest lev) */
  int ki_change;       /* Change in mana used by spell from lev to lev */

  int min_level[NUM_CLASSES];
  int routines;
  int8_t violent;
  int targets;              /* See below for use with TAR_XXX  */
  const char *name;         /* Input size not limited. Originates from string constants. */
  const char *wear_off_msg; /* Input size not limited. Originates from string constants. */
  int race_can_learn[NUM_RACES];
  int skilltype; /* Is it a spell, skill, art, feat, or what? used as bitvector */
  int flags;
  int save_flags;
  int comp_flags;
  int8_t can_learn_skill[NUM_CLASSES];
  int spell_level;
  int school;
  int domain;
};


bool spell_has_routine(GameWorld *g, int spellnum, int flag);

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

constexpr int SPELL_TYPE_SPELL  = 0;
constexpr int SPELL_TYPE_POTION = 1;
constexpr int SPELL_TYPE_WAND   = 2;
constexpr int SPELL_TYPE_STAFF  = 3;
constexpr int SPELL_TYPE_SCROLL = 4;

/* Attacktypes with grammar */

struct attack_hit_type {
  const char *singular;
  const char *plural;
};

#define ASPELL(spellname)                         \
  void spellname(GameWorld *g, int level, struct char_data *ch, \
                 struct char_data *victim, struct obj_data *obj, const char *arg)

#define MANUAL_SPELL(g, spellname) spellname(g, level, caster, cvict, ovict, arg);

ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(spell_portal);
ASPELL(art_abundant_step);

/* basic magic calling functions */

int find_skill_num(GameWorld *g, char *name, int sktype);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
               int spellnum);

void mag_affects(GameWorld *g, int level, struct char_data *ch, struct char_data *victim,
                 int spellnum);

void mag_groups(GameWorld *g, int level, struct char_data *ch, int spellnum);

void mag_masses(GameWorld *g, int level, struct char_data *ch, int spellnum);

void mag_areas(GameWorld *g, int level, struct char_data *ch, int spellnum);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj, int spellnum, char *arg);

void mag_points(GameWorld *g, int level, struct char_data *ch, struct char_data *victim,
                int spellnum);

void mag_unaffects(GameWorld *g, int level, struct char_data *ch, struct char_data *victim,
                   int spellnum);

void mag_alter_objs(GameWorld *g, int level, struct char_data *ch, struct obj_data *obj,
                    int spellnum);

void mag_creations(GameWorld *g, int level, struct char_data *ch, int spellnum);

void mag_affectsv(GameWorld *g, int level, struct char_data *ch, struct char_data *victim,
                  int spellnum);

int call_magic(GameWorld *g, struct char_data *caster, struct char_data *cvict,
               struct obj_data *ovict, int spellnum, int level, int casttype, char *arg);

void mag_objectmagic(GameWorld *g, struct char_data *ch, struct obj_data *obj,
                     char *argument);

bool mag_newsaves(struct char_data *ch, struct char_data *victim, int spellnum,
                  int level, int cast_stat);

/* other prototypes */
const char *skill_name(GameWorld *g, int num);
int roll_skill(GameWorld *g, const struct char_data *ch, int snum);
bool roll_resisted(GameWorld *g, const struct char_data *actor, int sact, const struct char_data *resistor, int sres);
int skill_type(GameWorld *g, int skill);

#ifdef __cplusplus
}
#endif
