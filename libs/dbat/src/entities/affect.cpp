#include <cstdlib>

#include "dbat/affect.hpp"
#include "dbat/consts/applies.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/spells.hpp"
#include "dbat/feats.hpp"
#include "dbat/game_world.hpp"

static void aff_apply_modify(GameWorld *g, struct char_data *ch, int loc, int mod, int spec, const char *msg)
{
  switch (loc) {
    case APPLY_NONE:
      break;

    case APPLY_STR:
      char_set_aff_abils_str(ch, char_get_aff_abils_str(ch) + (mod));
      break;
    case APPLY_DEX:
      char_set_aff_abils_dex(ch, char_get_aff_abils_dex(ch) + (mod));
      break;
    case APPLY_INT:
      char_set_aff_abils_intel(ch, char_get_aff_abils_intel(ch) + (mod));
      break;
    case APPLY_WIS:
      char_set_aff_abils_wis(ch, char_get_aff_abils_wis(ch) + (mod));
      break;
    case APPLY_CON:
      char_set_aff_abils_con(ch, char_get_aff_abils_con(ch) + (mod));
      break;
    case APPLY_CHA:
      char_set_aff_abils_cha(ch, char_get_aff_abils_cha(ch) + (mod));
      break;

    case APPLY_CLASS:
      /* ??? GET_CLASS(ch) += mod; */
      break;

      /*
   * My personal thoughts on these two would be to set the person to the
   * value of the apply.  That way you won't have to worry about people
   * making +1 level things to be imp (you restrict anything that gives
   * immortal level of course).  It also makes more sense to set someone
   * to a class rather than adding to the class number. -gg
   */

    case APPLY_LEVEL:
      /* ??? GET_LEVEL(ch) += mod; */
      break;

    case APPLY_AGE:
      char_set_time_birth(ch, char_get_time_birth(ch) - ((time_t)mod * SECS_PER_MUD_YEAR));
      break;

    case APPLY_CHAR_WEIGHT:
      char_set_weight(ch, char_get_weight(ch) + (mod));
      break;

    case APPLY_CHAR_HEIGHT:
      char_set_height(ch, char_get_height(ch) + (mod));
      break;

    case APPLY_MANA:
    case APPLY_KI:
      char_set_max_ki(ch, char_get_max_ki(ch) + mod);
      break;

    case APPLY_HIT:
      char_set_max_hit(ch, char_get_max_hit(ch) + mod);
      break;

    case APPLY_MOVE:
      char_set_max_move(ch, char_get_max_move(ch) + mod);
      break;

    case APPLY_GOLD:
      break;

    case APPLY_EXP:
      break;

    case APPLY_AC:
      char_set_armor(ch, char_get_armor(ch) + (mod));
      break;

    case APPLY_ACCURACY:
      char_set_accuracy(ch, char_get_accuracy(ch) + (mod));
      break;

    case APPLY_DAMAGE: /* the damage mod it adjusted no longer exists */
      break;

    case APPLY_REGEN:
      char_set_regen(ch, char_get_regen(ch) + (mod));
      break;

    case APPLY_TRAIN:
      char_set_asb(ch, char_get_asb(ch) + (mod));
      break;

    case APPLY_LIFEMAX:
      char_set_raw_lifebonus(ch, char_get_raw_lifebonus(ch) + mod);
      break;
    case APPLY_UNUSED3:
    case APPLY_UNUSED4:
      /* Don't exist anymore */
      break;

    case APPLY_RACE:
      /* ??? GET_RACE(ch) += mod; */
      break;

    case APPLY_TURN_LEVEL:
      char_set_tlevel(g, ch, char_get_tlevel(g, ch) + (mod));
      break;

    case APPLY_SPELL_LVL_0:
    case APPLY_SPELL_LVL_1:
    case APPLY_SPELL_LVL_2:
    case APPLY_SPELL_LVL_3:
    case APPLY_SPELL_LVL_4:
    case APPLY_SPELL_LVL_5:
    case APPLY_SPELL_LVL_6:
    case APPLY_SPELL_LVL_7:
    case APPLY_SPELL_LVL_8:
    case APPLY_SPELL_LVL_9:
      break;

    case APPLY_FORTITUDE:
      break;

    case APPLY_REFLEX:
      break;

    case APPLY_WILL:
      break;

    case APPLY_SKILL:
      char_set_skills_mod(ch, spec, char_get_skills_mod(ch, spec) + mod);
      break;

    case APPLY_FEAT:
      break;

    case APPLY_ALLSAVES:
      break;

    case APPLY_ALL_STATS:
      char_set_aff_abils_str(ch, char_get_aff_abils_str(ch) + (mod));
      char_set_aff_abils_intel(ch, char_get_aff_abils_intel(ch) + (mod));
      char_set_aff_abils_wis(ch, char_get_aff_abils_wis(ch) + (mod));
      char_set_aff_abils_dex(ch, char_get_aff_abils_dex(ch) + (mod));
      char_set_aff_abils_con(ch, char_get_aff_abils_con(ch) + (mod));
      char_set_aff_abils_cha(ch, char_get_aff_abils_cha(ch) + (mod));
      break;

    case APPLY_RESISTANCE:
      break;

    default:
      mud_log("SYSERR: Unknown apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
      break;

  } /* switch */
}

int affect_checked_specific(int location, int specific)
{
  if (location == APPLY_SKILL && specific >= 0 && specific < SKILL_TABLE_SIZE)
    return specific;
  if (location == APPLY_FEAT && specific >= 0 && specific < NUM_FEATS_DEFINED)
    return specific;
  return 0;
}

bool affect_is_loadable(const struct affected_type *af)
{
  constexpr int MAX_LOADED_MODIFIER = 100000000; /* far past any real one, and too small to overflow a stat */

  return af->type >= 0 && af->type < SKILL_TABLE_SIZE && af->bitvector < (bitvector_t)NUM_AFF_FLAGS &&
         af->location >= 0 && af->location <= APPLY_ALL_STATS && abs(af->modifier) <= MAX_LOADED_MODIFIER;
}

static void affect_modify(GameWorld *g, struct char_data *ch, int loc, int mod, int spec, long bitv, bool add)
{
  if (add) {
    if (bitv != AFF_INFRAVISION || !char_is_android(ch)) {
      char_aff_set(ch, bitv, true);
    }
  } else {
    if (bitv != AFF_INFRAVISION || !char_is_android(ch)) {
      char_aff_set(ch, bitv, false);
    }
    mod = -mod;
  }

  aff_apply_modify(g, ch, loc, mod, spec, "affect_modify");
}

void affect_modify_ar(GameWorld *g, struct char_data *ch, int loc, int mod, int spec, bitvector_t bitv[], bool add)
{
  int i, j;

  if (add) {
    for (i = 0; i < AF_ARRAY_MAX; i++)
      for (j = 0; j < 32; j++)
        if (IS_SET_AR(bitv, (i * 32) + j)) {
          if ((i * 32) + j != AFF_INFRAVISION || !char_is_android(ch)) {
            char_aff_set(ch, (i * 32) + j, true);
          }
        }
  } else {
    for (i = 0; i < AF_ARRAY_MAX; i++)
      for (j = 0; j < 32; j++)
        if (IS_SET_AR(bitv, (i * 32) + j)) {
          if ((i * 32) + j != AFF_INFRAVISION || !char_is_android(ch)) {
            char_aff_set(ch, (i * 32) + j, false);
          }
        }
    mod = -mod;
  }

  aff_apply_modify(g, ch, loc, mod, spec, "affect_modify_ar");
}

/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void affect_total(GameWorld *g, struct char_data *ch)
{
  struct affected_type *af;
  int i, j;

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        affect_modify_ar(g, ch, obj_get_affected(char_get_equipment(ch, i))[j].location,
                         obj_get_affected(char_get_equipment(ch, i))[j].modifier,
                         obj_get_affected(char_get_equipment(ch, i))[j].specific,
                         obj_get_bitvector(char_get_equipment(ch, i)), false);
  }

  for (af = char_get_affected(ch); af; af = af->next)
    affect_modify(g, ch, af->location, af->modifier, af->specific, af->bitvector, false);

  char_reset_aff_abils(ch);

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i)) {
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        affect_modify_ar(g, ch, obj_get_affected(char_get_equipment(ch, i))[j].location,
                         obj_get_affected(char_get_equipment(ch, i))[j].modifier,
                         obj_get_affected(char_get_equipment(ch, i))[j].specific,
                         obj_get_bitvector(char_get_equipment(ch, i)), true);
    }
  }

  for (af = char_get_affected(ch); af; af = af->next)
    affect_modify(g, ch, af->location, af->modifier, af->specific, af->bitvector, true);

  /* Make certain values are between 0..100, not < 0 and not > 100! */

  if (char_get_bonuses(ch, BONUS_WIMP) > 0) {
    char_set_aff_abils_str(ch, MAX(0, MIN(char_get_aff_abils_str(ch), 70)));
  } else {
    char_set_aff_abils_str(ch, MAX(0, MIN(char_get_aff_abils_str(ch), 100)));
  }
  if (char_get_bonuses(ch, BONUS_DULL) > 0) {
    char_set_aff_abils_intel(ch, MAX(0, MIN(char_get_aff_abils_intel(ch), 70)));
  } else {
    char_set_aff_abils_intel(ch, MAX(0, MIN(char_get_aff_abils_intel(ch), 100)));
  }
  if (char_get_bonuses(ch, BONUS_FOOLISH) > 0) {
    char_set_aff_abils_wis(ch, MAX(0, MIN(char_get_aff_abils_wis(ch), 70)));
  } else {
    char_set_aff_abils_wis(ch, MAX(0, MIN(char_get_aff_abils_wis(ch), 100)));
  }
  if (char_get_bonuses(ch, BONUS_SLOW) > 0) {
    char_set_aff_abils_cha(ch, MAX(0, MIN(char_get_aff_abils_cha(ch), 70)));
  } else {
    char_set_aff_abils_cha(ch, MAX(0, MIN(char_get_aff_abils_cha(ch), 100)));
  }
  if (char_get_bonuses(ch, BONUS_CLUMSY) > 0) {
    char_set_aff_abils_dex(ch, MAX(0, MIN(char_get_aff_abils_dex(ch), 70)));
  } else {
    char_set_aff_abils_dex(ch, MAX(0, MIN(char_get_aff_abils_dex(ch), 100)));
  }
  if (char_get_bonuses(ch, BONUS_FRAIL) > 0) {
    char_set_aff_abils_con(ch, MAX(0, MIN(char_get_aff_abils_con(ch), 70)));
  } else {
    char_set_aff_abils_con(ch, MAX(0, MIN(char_get_aff_abils_con(ch), 100)));
  }
}

/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(GameWorld *g, struct char_data *ch, struct affected_type *af)
{
  struct affected_type *affected_alloc;

  affected_alloc = new affected_type();

  if (!char_get_affected(ch))
    g->affect_list.add(char_get_id(ch), ch);
  *affected_alloc      = *af;
  affected_alloc->next = char_get_affected(ch);
  char_set_affected(ch, affected_alloc);

  affect_modify(g, ch, af->location, af->modifier, af->specific, af->bitvector, true);
  affect_total(g, ch);
}

/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(GameWorld *g, struct char_data *ch, struct affected_type *af)
{
  struct affected_type *cmtemp;

  if (char_get_affected(ch) == nullptr) {
    core_dump();
    return;
  }

  affect_modify(g, ch, af->location, af->modifier, af->specific, af->bitvector, false);
  REMOVE_FROM_LIST(af, *char_affected_slot(ch), next, cmtemp);
  delete af;
  affect_total(g, ch);
  if (!char_get_affected(ch)) {
    char_unlink_from_affect_list(g, ch);
  }
}

/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(GameWorld *g, struct char_data *ch, int type)
{
  struct affected_type *hjp, *next;

  for (hjp = char_get_affected(ch); hjp; hjp = next) {
    next = hjp->next;
    if (hjp->type == type)
      affect_remove(g, ch, hjp);
  }
}

/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affected_by_spell(struct char_data *ch, int type)
{
  struct affected_type *hjp;

  for (hjp = char_get_affected(ch); hjp; hjp = hjp->next)
    if (hjp->type == type)
      return (true);

  return (false);
}

void affect_join(GameWorld *g, struct char_data *ch, struct affected_type *af,
                 bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp, *next;
  bool found = false;

  for (hjp = char_get_affected(ch); !found && hjp; hjp = next) {
    next = hjp->next;

    if ((hjp->type == af->type) && (hjp->location == af->location) && (hjp->specific == af->specific)) {
      if (add_dur)
        af->duration += hjp->duration;
      if (avg_dur)
        af->duration /= 2;

      if (add_mod)
        af->modifier += hjp->modifier;
      if (avg_mod)
        af->modifier /= 2;

      affect_remove(g, ch, hjp);
      affect_to_char(g, ch, af);
      found = true;
    }
  }
  if (!found)
    affect_to_char(g, ch, af);
}
