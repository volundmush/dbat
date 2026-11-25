#pragma once
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"
#include "dbat/consts/races.hpp"
#include "dbat/consts/senseis.hpp"
#include "dbat/consts/skills.hpp"

constexpr int DEFAULT_STAFF_LVL = 12;
constexpr int DEFAULT_WAND_LVL  = 12;

constexpr int CAST_UNDEFINED = (-1);
constexpr int CAST_SPELL     = 0;
constexpr int CAST_POTION    = 1;
constexpr int CAST_WAND      = 2;
constexpr int CAST_STAFF     = 3;
constexpr int CAST_SCROLL    = 4;
constexpr int CAST_STRIKE    = 5;

/* What a spell does (spell_info_type::routines) */
extern const name_table routine_bits; /* flag_count<mag_routine> of them */
enum class mag_routine : uint8_t {
  damage         = 0,
  affects        = 1,
  unaffects      = 2,
  points         = 3,
  alter_objs     = 4,
  groups         = 5,
  masses         = 6,
  areas          = 7,
  summons        = 8,
  creations      = 9,
  manual         = 10,
  affectsv       = 11,
  action_free    = 12,
  action_partial = 13,
  action_full    = 14,
  nextstrike     = 15,
  touch_melee    = 16,
  touch_ranged   = 17,
  count_         = 18,
};
template <> struct flag_traits<mag_routine> {
  static constexpr const name_table &names = routine_bits;
};

/* How a spell's victim saves (spell_info_type::save_flags) */
extern const name_table save_bits; /* flag_count<mag_save> of them */
enum class mag_save : uint8_t {
  fort    = 0,
  reflex  = 1,
  will    = 2,
  half    = 3,
  none    = 4,
  partial = 5,
  count_  = 6,
};
template <> struct flag_traits<mag_save> {
  static constexpr const name_table &names = save_bits;
};

/* What casting a spell takes (spell_info_type::comp_flags) */
extern const name_table component_bits; /* flag_count<mag_component> of them */
enum class mag_component : uint8_t {
  divine_focus = 0,
  exp_cost     = 1,
  focus        = 2,
  material     = 3,
  somatic      = 4,
  verbal       = 5,
  count_       = 6,
};
template <> struct flag_traits<mag_component> {
  static constexpr const name_table &names = component_bits;
};

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

/* What a spell can be cast at (spell_info_type::targets) */
extern const name_table target_bits; /* flag_count<spell_target> of them */
enum class spell_target : uint8_t {
  ignore     = 0,
  char_room  = 1,
  char_world = 2,
  fight_self = 3,
  fight_vict = 4,
  self_only  = 5,   /* Only a check, use with i.e. spell_target::char_room */
  not_self   = 6,   /* Only a check, use with i.e. spell_target::char_room */
  obj_inv    = 7,
  obj_room   = 8,
  obj_world  = 9,
  obj_equip  = 10,
  count_     = 11,
};
template <> struct flag_traits<spell_target> {
  static constexpr const name_table &names = target_bits;
};

/* What a skill table entry is (spell_info_type::skilltype): one kind each, though lookups take a set (none for an unused entry) */
extern const name_table skill_kind_bits; /* flag_count<skill_kind> of them */
enum class skill_kind : uint8_t {
  spell  = 0,
  skill  = 1,
  lang   = 2,
  weapon = 3,
  art    = 4,
  count_ = 5,
};
template <> struct flag_traits<skill_kind> {
  static constexpr const name_table &names = skill_kind_bits;
};

/* How a skill is trained and what modifies it (spell_info_type::flags); needtrain disallows using a skill at 0 with only its stat */
extern const name_table skill_flag_bits; /* flag_count<skill_flag> of them */
enum class skill_flag : uint8_t {
  needtrain = 0,
  strmod    = 1,
  dexmod    = 2,
  conmod    = 3,
  intmod    = 4,
  wismod    = 5,
  chamod    = 6,
  armorbad  = 7,
  armorall  = 8,
  tier1     = 9,
  tier2     = 10,
  tier3     = 11,
  tier4     = 12,
  tier5     = 13,
  count_    = 14,
};
template <> struct flag_traits<skill_flag> {
  static constexpr const name_table &names = skill_flag_bits;
};

constexpr int SKLEARN_CANT       = 0; /* This class can't learn this skill */
constexpr int SKLEARN_CROSSCLASS = 1; /* Cross-class skill for this class */
constexpr int SKLEARN_CLASS      = 2; /* Class skill for this class */
constexpr int SKLEARN_BOOL       = 3; /* Skill is known or not */

struct spell_info_type {
  int8_t min_position{}; /* Position for caster	 */
  int mana_min{};        /* Min amount of mana used by a spell (highest lev) */
  int mana_max{};        /* Max amount of mana used by a spell (lowest lev) */
  int mana_change{};     /* Change in mana used by spell from lev to lev */
  int ki_min{};          /* Min amount of mana used by a spell (highest lev) */
  int ki_max{};          /* Max amount of mana used by a spell (lowest lev) */
  int ki_change{};       /* Change in mana used by spell from lev to lev */

  int min_level[NUM_CLASSES]{};
  flag_set<mag_routine> routines;
  bool violent{};
  flag_set<spell_target> targets; /* See below for use with TAR_XXX  */
  std::string_view name{};         /* a string constant */
  std::string_view wear_off_msg{}; /* a string constant, or empty for none */
  int race_can_learn[NUM_RACES]{};
  flag_set<skill_kind> skilltype; /* none for an unused entry */
  flag_set<skill_flag> flags;
  flag_set<mag_save> save_flags;
  flag_set<mag_component> comp_flags;
  int8_t can_learn_skill[NUM_CLASSES]{};
  int spell_level{};
  int school{};
  int domain{};
};

/* Whether the spell has any of these routines */
bool spell_has_routine(GameWorld *g, int spellnum, flag_set<mag_routine> routines);

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
  std::string_view singular{};
  std::string_view plural{};
};

#define ASPELL(spellname)                         \
  void spellname(GameWorld *g, int level, struct char_data *ch, \
                 struct char_data *victim, struct obj_data *obj, std::string_view arg)

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

int find_skill_num(GameWorld *g, std::string_view name, flag_set<skill_kind> sktype);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
               int spellnum);

void mag_affects(GameWorld *g, int level, struct char_data *ch, struct char_data *victim,
                 int spellnum);

void mag_groups(GameWorld *g, int level, struct char_data *ch, int spellnum);

void mag_masses(GameWorld *g, int level, struct char_data *ch, int spellnum);

void mag_areas(GameWorld *g, int level, struct char_data *ch, int spellnum);

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
               struct obj_data *ovict, int spellnum, int level, int casttype, std::string_view arg = {});

void mag_objectmagic(GameWorld *g, struct char_data *ch, struct obj_data *obj, std::string_view argument);

bool mag_newsaves(struct char_data *ch, struct char_data *victim, int spellnum,
                  int level, int cast_stat);

/* other prototypes */
std::string_view skill_name(GameWorld *g, int num);
int roll_skill(GameWorld *g, const struct char_data *ch, int snum);
bool roll_resisted(GameWorld *g, const struct char_data *actor, int sact, const struct char_data *resistor, int sres);
flag_set<skill_kind> skill_type(GameWorld *g, int skill);

