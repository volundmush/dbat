/*****************************************************************************
** FEATS.C                                                                  **
** Source code for the Gates of Krynn Feats System.                         **
** Initial code by Paladine (Stephen Squires)                               **
** Created Thursday, September 5, 2002                                      **
**                                                                          **
*****************************************************************************/
#include "dbat/utils.h"

#include "dbat/feats.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/interpreter.h"

/* Local Functions */

static void feato(int featnum, const char *name, int in_game, int can_learn, int can_stack);
static int compare_feats(const void *x, const void *y);

/* Global Variables and Structures */
struct feat_info feat_list[NUM_FEATS_DEFINED];
int feat_sort_info[MAX_FEATS];

/* External variables and structures */

/* External functions*/

static void feato(int featnum, const char *name, int in_game, int can_learn, int can_stack)
{
  feat_list[featnum].name      = name;
  feat_list[featnum].in_game   = in_game;
  feat_list[featnum].can_learn = can_learn;
  feat_list[featnum].can_stack = can_stack;
}

void free_feats(void)
{
  /* Nothing to do right now */
}

void assign_feats(void)
{

  int i;

  // Initialize the list of feats.

  for (i = 0; i < NUM_FEATS_DEFINED; i++) {
    feat_list[i].name      = "Unused Feat";
    feat_list[i].in_game   = false;
    feat_list[i].can_learn = false;
    feat_list[i].can_stack = false;
  }

  // Below are the various feat initializations.
  // First parameter is the feat number, defined in feats.h
  // Second parameter is the displayed name of the feat and argument used to train it
  // Third parameter defines whether or not the feat is in the game or not, and thus can be learned and displayed
  // Fourth parameter defines whether or not the feat can be learned through a trainer or whether it is
  // a feat given automatically to certain classes or races.
  // Fifth parameter defines whether or not the feat can be learned multiple times.

  feato(FEAT_ALERTNESS, "alertness", true, false, false);
  feato(FEAT_ARMOR_PROFICIENCY_HEAVY, "heavy armor proficiency", false, true, false);
  feato(FEAT_ARMOR_PROFICIENCY_LIGHT, "light armor proficiency", false, true, false);
  feato(FEAT_ARMOR_PROFICIENCY_MEDIUM, "medium armor proficiency", false, true, false);
  feato(FEAT_BLIND_FIGHT, "blind fighting", true, true, false);
  feato(FEAT_BREW_POTION, "brew potion", false, true, false);
  feato(FEAT_CLEAVE, "cleave", false, true, false);
  feato(FEAT_COMBAT_CASTING, "combat casting", false, true, false);
  feato(FEAT_COMBAT_REFLEXES, "combat reflexes", false, true, false);
  feato(FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR, "craft magical arms and armor", false, true, false);
  feato(FEAT_CRAFT_ROD, "craft rod", false, true, false);
  feato(FEAT_CRAFT_STAFF, "craft staff", false, true, false);
  feato(FEAT_CRAFT_WAND, "craft wand", false, true, false);
  feato(FEAT_CRAFT_WONDEROUS_ITEM, "craft wonderous item", false, true, false);
  feato(FEAT_DEFLECT_ARROWS, "deflect arrows", false, false, false);
  feato(FEAT_DODGE, "dodge", true, true, false);
  feato(FEAT_EMPOWER_SPELL, "empower spell", false, true, false);
  feato(FEAT_ENDURANCE, "endurance", false, true, false);
  feato(FEAT_ENLARGE_SPELL, "enlarge spell", false, false, false);
  feato(FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD, "weapon proficiency - bastard sword", false, true, false);
  feato(FEAT_EXTEND_SPELL, "extend spell", false, true, false);
  feato(FEAT_EXTRA_TURNING, "extra turning", false, true, false);
  feato(FEAT_FAR_SHOT, "far shot", false, false, false);
  feato(FEAT_FORGE_RING, "forge ring", false, true, false);
  feato(FEAT_GREAT_CLEAVE, "great cleave", false, false, false);
  feato(FEAT_GREAT_FORTITUDE, "great fortitude", true, true, false);
  feato(FEAT_HEIGHTEN_SPELL, "heighten spell", false, true, false);
  feato(FEAT_IMPROVED_BULL_RUSH, "improved bull rush", false, false, false);
  feato(FEAT_IMPROVED_CRITICAL, "improved critical", true, true, true);
  feato(FEAT_IMPROVED_DISARM, "improved disarm", false, true, false);
  feato(FEAT_IMPROVED_INITIATIVE, "improved initiative", true, true, false);
  feato(FEAT_IMPROVED_TRIP, "improved trip", true, true, false);
  feato(FEAT_IMPROVED_TWO_WEAPON_FIGHTING, "improved two weapon fighting", true, true, false);
  feato(FEAT_IMPROVED_UNARMED_STRIKE, "improved unarmed strike", false, false, false);
  feato(FEAT_IRON_WILL, "iron will", true, true, false);
  feato(FEAT_LEADERSHIP, "leadership", false, false, false);
  feato(FEAT_LIGHTNING_REFLEXES, "lightning reflexes", true, true, false);
  feato(FEAT_MARTIAL_WEAPON_PROFICIENCY, "martial weapon proficiency", false, true, false);
  feato(FEAT_MAXIMIZE_SPELL, "maximize spell", false, true, false);
  feato(FEAT_MOBILITY, "mobility", true, true, false);
  feato(FEAT_MOUNTED_ARCHERY, "mounted archery", false, false, false);
  feato(FEAT_MOUNTED_COMBAT, "mounted combat", false, false, false);
  feato(FEAT_POINT_BLANK_SHOT, "point blank shot", false, false, false);
  feato(FEAT_POWER_ATTACK, "power attack", true, true, false);
  feato(FEAT_PRECISE_SHOT, "precise shot", false, false, false);
  feato(FEAT_QUICK_DRAW, "quick draw", false, false, false);
  feato(FEAT_QUICKEN_SPELL, "quicken spell", false, true, false);
  feato(FEAT_RAPID_SHOT, "rapid shot", false, false, false);
  feato(FEAT_RIDE_BY_ATTACK, "ride by attack", false, false, false);
  feato(FEAT_RUN, "run", false, false, false);
  feato(FEAT_SCRIBE_SCROLL, "scribe scroll", false, true, false);
  feato(FEAT_SHOT_ON_THE_RUN, "shot on the run", false, false, false);
  feato(FEAT_SILENT_SPELL, "silent spell", false, true, false);
  feato(FEAT_SIMPLE_WEAPON_PROFICIENCY, "simple weapon proficiency", true, true, false);
  feato(FEAT_SKILL_FOCUS, "skill focus", true, true, true);
  feato(FEAT_SPELL_FOCUS, "spell focus", false, true, true);
  feato(FEAT_SPELL_MASTERY, "spell mastery", false, true, true);
  feato(FEAT_SPELL_PENETRATION, "spell penetration", false, true, false);
  feato(FEAT_SPIRITED_CHARGE, "spirited charge", false, false, false);
  feato(FEAT_SPRING_ATTACK, "spring attack", true, false, false);
  feato(FEAT_STILL_SPELL, "still spell", false, true, false);
  feato(FEAT_STUNNING_FIST, "stunning fist", false, true, false);
  feato(FEAT_SUNDER, "sunder", false, true, false);
  feato(FEAT_TOUGHNESS, "toughness", true, true, true);
  feato(FEAT_TRACK, "track", false, true, false);
  feato(FEAT_TRAMPLE, "trample", false, false, false);
  feato(FEAT_TWO_WEAPON_FIGHTING, "two weapon fighting", true, true, false);
  feato(FEAT_WEAPON_FINESSE, "weapon finesse", true, true, true);
  feato(FEAT_WEAPON_FOCUS, "weapon focus", false, true, true);
  feato(FEAT_WEAPON_SPECIALIZATION, "weapon specialization", false, true, true);
  feato(FEAT_WHIRLWIND_ATTACK, "whirlwind attack", false, true, false);
  feato(FEAT_WEAPON_PROFICIENCY_DRUID, "weapon proficiency - druids", false, false, false);
  feato(FEAT_WEAPON_PROFICIENCY_ROGUE, "weapon proficiency - rogues", false, false, false);
  feato(FEAT_WEAPON_PROFICIENCY_MONK, "weapon proficiency - monks", false, false, false);
  feato(FEAT_WEAPON_PROFICIENCY_WIZARD, "weapon proficiency - wizards", false, false, false);
  feato(FEAT_WEAPON_PROFICIENCY_ELF, "weapon proficiency - elves", false, false, false);
  feato(FEAT_ARMOR_PROFICIENCY_SHIELD, "shield armor proficiency", false, false, false);
  feato(FEAT_SNEAK_ATTACK, "sneak attack", true, false, true);
  feato(FEAT_EVASION, "evasion", true, false, false);
  feato(FEAT_IMPROVED_EVASION, "improved evasion", true, false, false);
  feato(FEAT_ACROBATIC, "acrobatic", true, true, false);
  feato(FEAT_AGILE, "agile", true, true, false);
  feato(FEAT_ALERTNESS, "alertness", true, false, false);
  feato(FEAT_ANIMAL_AFFINITY, "animal affinity", false, true, false);
  feato(FEAT_ATHLETIC, "athletic", true, true, false);
  feato(FEAT_AUGMENT_SUMMONING, "augment summoning", false, false, false);
  feato(FEAT_COMBAT_EXPERTISE, "combat expertise", false, false, false);
  feato(FEAT_DECEITFUL, "deceitful", true, true, false);
  feato(FEAT_DEFT_HANDS, "deft hands", false, true, false);
  feato(FEAT_DIEHARD, "diehard", true, false, false);
  feato(FEAT_DILIGENT, "diligent", true, true, false);
  feato(FEAT_ESCHEW_MATERIALS, "eschew materials", false, false, false);
  feato(FEAT_EXOTIC_WEAPON_PROFICIENCY, "exotic weapon proficiency", false, false, false);
  feato(FEAT_GREATER_SPELL_FOCUS, "greater spell focus", false, false, true);
  feato(FEAT_GREATER_SPELL_PENETRATION, "greater spell penetration", false, false, false);
  feato(FEAT_GREATER_TWO_WEAPON_FIGHTING, "greater two weapon fighting", true, false, false);
  feato(FEAT_GREATER_WEAPON_FOCUS, "greater weapon focus", false, true, true);
  feato(FEAT_GREATER_WEAPON_SPECIALIZATION, "greater weapon specialization", false, true, true);
  feato(FEAT_IMPROVED_COUNTERSPELL, "improved counterspell", false, false, false);
  feato(FEAT_IMPROVED_FAMILIAR, "improved familiar", false, false, false);
  feato(FEAT_IMPROVED_FEINT, "improved feint", false, false, false);
  feato(FEAT_IMPROVED_GRAPPLE, "improved grapple", false, false, false);
  feato(FEAT_IMPROVED_OVERRUN, "improved overrun", false, false, false);
  feato(FEAT_IMPROVED_PRECISE_SHOT, "improved precise shot", false, false, false);
  feato(FEAT_IMPROVED_SHIELD_BASH, "improved shield bash", false, false, false);
  feato(FEAT_IMPROVED_SUNDER, "improved sunder", false, false, false);
  feato(FEAT_IMPROVED_TURNING, "improved turning", false, false, false);
  feato(FEAT_INVESTIGATOR, "investigator", false, true, false);
  feato(FEAT_MAGICAL_APTITUDE, "magical aptitude", false, true, false);
  feato(FEAT_MANYSHOT, "manyshot", false, false, false);
  feato(FEAT_NATURAL_SPELL, "natural spell", false, false, false);
  feato(FEAT_NEGOTIATOR, "negotiator", false, true, false);
  feato(FEAT_NIMBLE_FINGERS, "nimble fingers", false, true, false);
  feato(FEAT_PERSUASIVE, "persuasive", false, true, false);
  feato(FEAT_RAPID_RELOAD, "rapid reload", false, false, false);
  feato(FEAT_SELF_SUFFICIENT, "self sufficient", false, true, false);
  feato(FEAT_STEALTHY, "stealthy", true, true, false);
  feato(FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD, "tower shield armor proficiency", false, false, false);
  feato(FEAT_TWO_WEAPON_DEFENSE, "two weapon defense", false, false, false);
  feato(FEAT_WIDEN_SPELL, "widen spell", false, false, false);
}

// The follwing function is used to check if the character satisfies the various prerequisite(s) (if any)
// of a feat in order to learn it.

bool feat_is_available(struct char_data *ch, int featnum, int iarg, char *sarg)
{
  return false;
}

bool is_proficient_with_armor(const struct char_data *ch, int cmarmor_type)
{
  return true;
}

bool is_proficient_with_weapon(const struct char_data *ch, int cmweapon_type)
{
  return true;
}

static int compare_feats(const void *x, const void *y)
{
  int a = *(const int *)x,
      b = *(const int *)y;

  return strcmp(feat_list[a].name, feat_list[b].name);
}

void sort_feats(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 0; a < NUM_FEATS_DEFINED; a++)
    feat_sort_info[a] = a;

  qsort(&feat_sort_info[0], NUM_FEATS_DEFINED, sizeof(int), compare_feats);
}

int find_feat_num(char *name)
{
  int ftindex, ok;
  char *temp, *temp2;
  char first[MAX_INPUT_LENGTH], first2[MAX_INPUT_LENGTH];

  for (ftindex = 1; ftindex <= NUM_FEATS_DEFINED; ftindex++) {
    if (is_abbrev(name, feat_list[ftindex].name))
      return (ftindex);

    ok = true;
    /* It won't be changed, but other uses of this function elsewhere may. */
    temp  = any_one_arg((char *)feat_list[ftindex].name, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
        ok = false;
      temp  = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2)
      return (ftindex);
  }

  return (-1);
}
